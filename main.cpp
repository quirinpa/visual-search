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

static bool_t compare_dist(float *a, float *b) {
	return *b > *a;
}

/* how to reduce avl mem transfer? if it is a vector, pre-allocate
 * positions close to avl (array of DMatch values, local cache),
 * and use.indexes to access that array. Or something. But that
 * requires clusters to reference to them. So have a avl_vec_insert
 * that does that then calls avl_insert for each element? Meanwhile... */
__inline__
static avl_t*
putMatch(avl_t* t, DMatch *match) {
	DMatch *copy = (DMatch*) malloc(sizeof(DMatch));
	memcpy(match, copy, sizeof(DMatch));

	return avl_insert(t, copy, &copy->distance, (avl_compare_gfp_t) compare_dist);
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
	/* FIXME returns all 0s */
	{
		size_t c = knnmatches.size();
		if (c) {
			vector<DMatch> *currv = &knnmatches[0];
			size_t i = 0;
			do {
				DMatch *best_ref = &currv->at(0);

				if ( currv->size() == 1 ||
						best_ref->distance <= .8f * currv->at(1).distance ) {

					t = putMatch(t, best_ref);
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
				t = putMatch(t, curr);

				curr++; n++;
			} while (n<c);

		}
	}

#endif

	fprintf(stderr, "%lu matches\n\n", n);

	return t;
}

typedef float (*diff_gfp_t)(void *, void*);

extern "C" {

#include "fifo.h"

}

#include "avl_iot.h"
static void
get_clusters(
	fifo_t *clusters,
	avl_t *root,
	float eps,
	size_t min_elements,
	avl_compare_gfp_t cmp)
{
	avl_t *curr_cluster = NULL;
	size_t curr_cluster_n = 0;

	float last_key = 0;

	fputs("\nPRINTING TREE: ", stderr);
	avl_iot(root, [&] (avl_t * curr) {
		fprintf(stderr, "%.2f/", (double)*(float*)curr->key);
	});

	fputs("\n-----\nTRYING CLUSTER: ", stderr);

	avl_iot(root, [&] (avl_t * curr) {
		void *key_ref = curr->key;
		float key = *(float*)key_ref;

		if (key - last_key > eps) {
			if (curr_cluster_n >= min_elements) {
				fprintf(stderr, " ACCEPT\n");
				fifo_push(clusters, curr_cluster);
			}

			fprintf(stderr, "\nCLUSTER ");
			curr_cluster = NULL;
			curr_cluster_n = 0;
		}

		fprintf(stderr, "%.0f/", (double)key);

		curr_cluster = avl_insert(curr_cluster, curr->data, key_ref, cmp);
		curr_cluster_n++;

		last_key = key;
	});
}


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

		avl_t *eucl_avl = matchf(query_d, d);

		fifo_t *eucl_clusters = new_fifo();
		get_clusters(eucl_clusters, eucl_avl, 3.0f, 10, (avl_compare_gfp_t) compare_dist);

		size_t n_clusters = 0; 
		while (eucl_clusters->top) {
			avl_t *cluster = (avl_t*)fifo_pop(eucl_clusters);
			avl_free(cluster);
			n_clusters++;
		}
		free(eucl_clusters);
		avl_free(eucl_avl);

		fprintf(stderr, "\n%lu clusters\n", n_clusters);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
