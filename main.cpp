#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF


#include <stdio.h>
#include <ctime>

using namespace std;
using namespace cv;

#define use_flann false
#define use_cross_match false
#if use_flann
static const DescriptorMatcher *matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
#else
static const DescriptorMatcher *matcher = new BFMatcher(
		/* NORM_HAMMING */
		NORM_L2
		, use_cross_match
		);
#endif

extern "C" {

#include "avl.h"
#include "bool.h"

}

static bool_t compare_float_ptrs(float *a, float *b) {
	return *b > *a;
}

/* how to reduce avl mem transfer? if it is a vector, pre-allocate
 * positions close to avl (array of DMatch values, local cache),
 * and use.indexes to access that array. Or something. But that
 * requires clusters to reference to them. So have a avl_vec_insert
 * that does that then calls avl_insert for each element? Meanwhile... */
template<typename putMatch_GKEY>
__inline__
static avl_t*
putMatch(avl_t* t, DMatch *match, putMatch_GKEY gkey) {
	DMatch *copy = (DMatch*) malloc(sizeof(DMatch));
	memcpy(copy, match, sizeof(DMatch));

	return avl_insert(t, copy, (void*) gkey(copy),
										(avl_compare_gfp_t) compare_float_ptrs); /* FIXME */
}

#define use_knn false
__inline__
static avl_t *
matchf(Mat desc1, Mat desc2)
{
	avl_t *t = NULL;
	size_t n = 0;

#if use_knn
	vector<vector<DMatch> > knnmatches;
	matcher->knnMatch(desc1, desc2, knnmatches, 2);

	fputs("\nINSERT MATCHES\t", stderr);
	{
		size_t c = knnmatches.size();
		if (c) {
			vector<DMatch> *currv = &knnmatches[0];
			size_t i = 0;
			do {
				DMatch *best_ref = &currv->at(0);

				if ( currv->size() == 1 ||
						best_ref->distance <= .8f * currv->at(1).distance ) {

					t = putMatch(t, best_ref, [](DMatch* copy) -> float* {
						return &copy->distance;
					});

					n++;
				}

				currv++; i++;
			} while (i<c);
		}
	}

#else
	vector<DMatch> save_matches; 
	matcher->match(desc1, desc2, save_matches);

	{
		register size_t c = save_matches.size();
		if (c) {
			DMatch *curr = &save_matches[0];

			do {
				t = putMatch(t, curr, [](DMatch* copy) -> float* {
					return &copy->distance;
				});

				curr++; n++;
			} while (n<c);

		}
	}

#endif

	fprintf(stderr, "%u matches\n\n", n);

	return t;
}

typedef float (*diff_gfp_t)(void *, void*);

extern "C" {

#include "fifo.h"

}

#include "avl_iot.h"

typedef struct cluster_st {
	avl_t* root;
	float max, min;
	struct cluster_st *parent;
} cluster_t;

static cluster_t *
new_cluster(avl_t *root, float min, float max, cluster_t *parent) {
	cluster_t *cluster = (cluster_t*) malloc(sizeof(cluster_t));
	cluster->root = root;
	cluster->max = max;
	cluster->min = min;
	cluster->parent = parent;
	return cluster;
}

template<typename GET_CLUSTERS_GKEY>
static void
get_clusters(
	fifo_t *clusters,
	cluster_t *parent,
	/* avl_t *root, */
	float eps,
	size_t min_elements,
	avl_compare_gfp_t cmp,
	GET_CLUSTERS_GKEY get_next_key)
{
	size_t size = 0;
	float last_key = 0;

	avl_t *subcluster_avl = NULL, *parent_avl = parent->root;

	if (parent) min = *(float*) parent_avl->key;
	/* fputs("\nCLUSTER? ", stderr); */

	avl_iot(parent_avl, [&] (avl_t * curr) {
		float key = *(float*) curr->key;

		if (key - last_key > eps) {
			if (size >= min_elements) {
				/* fprintf(stderr, " ACCEPT\n"); */
				
				fifo_push(clusters, new_cluster(subcluster_avl, min, key, parent));

			}

			/* fprintf(stderr, "\nCLUSTER "); */
			subcluster_avl = NULL;
			size = 0;
		}

		/* fprintf(stderr, "%.0f/", (double)key); */

		{
			register DMatch *data = (DMatch*)curr->data;
			subcluster_avl = avl_insert(subcluster_avl, data,
					(void*) get_next_key(data), cmp);
		}

		size++;

		last_key = key;
	});


	if (size >= min_elements) {
		/* fprintf(stderr, " ACCEPT\n"); */
		fifo_push(clusters, new_cluster(subcluster_avl, min, last_key, parent));
	}

	avl_free(parent_avl);
}

template<typename re_cluster_GKEY>
__inline__
static fifo_t *
re_cluster(
	fifo_t *source,
	float min_dist,
	size_t min_elements,
	re_cluster_GKEY gkey)
{
	fifo_t *result = new_fifo();

	while (source->top) {
		cluster_t *cluster = (cluster_t*) fifo_pop(source);
		/* avl_t *cluster = (avl_t*) fifo_pop(source); */

		get_clusters(result, cluster, min_dist, min_elements,
								 (avl_compare_gfp_t) compare_float_ptrs, gkey);
	}

	free(source);
	return result;
}

/* static void */
/* drawCluster(Mat dest, fifo_t *cluster) */
/* { */
/* 	while (cluster->top) { */
/* 		avl_t *cluster = (avl_t*) fifo_pop(cluster); */
/* 		double min = avl_min(cluster), max = avl_max(cluster); */
/* 		line(dest, Point2f(min, max, Scalar(255, 0, 0), 4); */
/* 	} */
/* } */

/* int main(int argc, char **argv) { */
int main(void) {
	Mat query = imread("resources/ss.png");
	Ptr<FeatureDetector> detector = BRISK::create();

	vector<KeyPoint> query_kp;
	detector->detect(query, query_kp);

	Ptr<DescriptorExtractor> extractor = BRISK::create();

	Mat query_d;
	extractor->compute(query, query_kp, query_d);

	VideoCapture cap(0);
	if(!cap.isOpened()) return -1;

	namedWindow("d", CV_WINDOW_NORMAL);

	unsigned n = 0;
	clock_t times = 0;

	do {
		Mat cframe;
		cap >> cframe;

		/* Mat img_matches; */

		vector<KeyPoint> kp;
		Mat d;

		clock_t start = clock();

		detector->detect(cframe, kp);
		extractor->compute(cframe, kp, d);

		times += clock() - start;
		n++;

		/* gets freed by get_clusters */
		avl_t *eucl_avl = matchf(query_d, d);

		/* fputs("\nPRINTING TREE: ", stderr); */
		/* avl_iot(eucl_avl, [&] (avl_t * curr) { */
		/* 	fprintf(stderr, "%.2f/", (double)*(float*)curr->key); */
		/* }); */

		/* fputs("\nGENERATING EUCL CLUSTERS SORTED BY X", stderr); */
		fifo_t *eucl_clusters_x = new_fifo();

		get_clusters(eucl_clusters_x, new_cluster(eucl_avl, 0, 0, NULL),
				10.0f, 10, (avl_compare_gfp_t) compare_float_ptrs,
				[&](DMatch *data) -> float* { return &kp[data->trainIdx].pt.x; });

		/* fputs("\nGENERATING X CLUSTERS SORTED BY Y", stderr); */
		fifo_t *x_clusters_y = re_cluster(eucl_clusters_x, 10.0f, 10,
				[&](DMatch *data) ->float* { return &kp[data->trainIdx].pt.y; });


		/* fputs("\nGENERATING Y CLUSTERS SORTED BY Y", stderr); */
		fifo_t *y_clusters_y = re_cluster(x_clusters_y, 10.0f, 10,
				[&](DMatch *data) -> float* { return &kp[data->trainIdx].pt.y; });

		{
			fifo_node_t **top = &y_clusters_y->top;
			size_t n_y_clusters_y = 0;

			while (*top) {
				cluster_t *cluster = (cluster_t*) fifo_pop(y_clusters_y),
									*xparent = cluster->parent->parent;

				/* float xmin = (float) cframe.cols, xmax = 0; */
				avl_iot(cluster->root, [&] (avl_t* avlnode) {
					Point2f cluster_pt = kp[((DMatch*) avlnode->data)->trainIdx].pt;

					line(cframe, cluster_pt, cluster_pt, Scalar(255, 255, 0), 2);

					/* { */
					/* 	float x = cluster_pt.x; */
					/* 	if (x < xmin) xmin = x; */
					/* 	if (x > xmax) xmax = x; */
					/* } */

				});

				rectangle(cframe,
						/* Point2f(xmin, cluster->min), */
						/* Point2f(xmax, cluster->max), */
						Point2f(xparent->min, cluster->min),
						Point2f(xparent->max, cluster->max),
						Scalar(255, 0, 0), 1);

				avl_free(cluster->root);
				free(cluster);

				n_y_clusters_y++;
			}

			fprintf(stderr, "\n%u clusters\n", n_y_clusters_y);
		}

		free(y_clusters_y);

		imshow("d", cframe);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
