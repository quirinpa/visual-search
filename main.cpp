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
				float *distance_ref = &best_ref->distance;
				/* please don't change distance value meanwhile */
				if (currv->size() == 1 || *distance_ref <= .8f * currv->at(1).distance) {
					n++;
					/* fprintf(stderr, "try %.0f ", (double)*distance_ref); */
					t = avl_insert( t,
							( void* ) best_ref,
							( void* ) distance_ref,
							( avl_compare_gfp_t ) compare_dist );
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
				t = avl_insert(t,
						( void* )curr,
						( void* )&curr->distance,
						( avl_compare_gfp_t ) compare_dist);
				/* fprintf(stderr, "%f\t", (double)curr->distance); */

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
static fifo_t *
get_clusters(
	avl_t *root,
	float eps,
	size_t min_elements,
	avl_compare_gfp_t cmp)
{
	fifo_t *clusters = new_fifo();

	avl_t *curr_cluster = NULL;
	size_t curr_cluster_n = 0;

	float last_key = 0;

	fputs("\nTRYING CLUSTER: ", stderr);

	avl_iot(root, [&] (avl_t * curr) {
		void *key_ref = curr->key;
		float key = *(float*)key_ref;

		fprintf(stderr, "/%f-%.0f", (double) key, (double) (key - last_key));

		/* if (!curr_cluster || key - last_key < eps) { */
		/* 	curr_cluster = avl_insert(curr_cluster, curr->data, key_ref, cmp); */
		/* 	curr_cluster_n++; */
		/* } else { */
		/* 	if (curr_cluster_n < min_elements) */
		/* 		avl_free(curr_cluster); */
		/* 	else { */
		/* 		fputs("\nACCEPTED.\n", stderr); */
		/* 		fifo_push(clusters, curr_cluster); */
		/* 	} */

		/* 	fputs("\nTRYING CLUSTER: ", stderr); */
		/* 	curr_cluster = NULL; */
		/* 	curr_cluster_n = 0; */
		/* } */

		last_key = key;
	});

	return clusters;
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

		/* vector<DMatch> matches; */
		/* avl_t *tree = */ 
		avl_t *eucl_avl = matchf(query_d, d);

		fifo_t *eucl_clusters =
			get_clusters(eucl_avl, 20.0f, 10, (avl_compare_gfp_t) compare_dist);

		size_t n_clusters = 0; 
		while (eucl_clusters->top) {
			avl_t *cluster = (avl_t*)fifo_pop(eucl_clusters);
			avl_free(cluster);
			n_clusters++;
		}
		free(eucl_clusters);
		avl_free(eucl_avl);

		fprintf(stderr, "%lu clusters\n", n_clusters);

		avl_free(eucl_avl);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
