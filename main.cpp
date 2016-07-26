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

/* /1* how to reduce avl mem transfer? if it is a vector, pre-allocate */
/*  * positions close to avl (array of DMatch values, local cache), */
/*  * and use.indexes to access that array. Or something. But that */
/*  * requires clusters to reference to them. So have a avl_vec_insert */
/*  * that does that then calls avl_insert for each element? Meanwhile... *1/ */
/* template<typename putMatch_GKEY> */
/* __inline__ */
/* static avl_t* */
/* putMatch(avl_t* t, DMatch *match, putMatch_GKEY gkey) { */
/* 	DMatch *copy = (DMatch*) malloc(sizeof(DMatch)); */
/* 	memcpy(copy, match, sizeof(DMatch)); */

/* 	return avl_insert(t, copy, (void*) gkey(copy), */
/* 										(avl_compare_gfp_t) compare_float_ptrs); /1* FIXME *1/ */
/* } */

#include <map>
#define use_knn false
__inline__
static map<int, DMatch>
matchf(Mat desc1, Mat desc2)
{
	map<int, DMatch> m;
	/* avl_t *t = NULL; */
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

					m[best_ref->trainIdx] = *best_ref;
					/* t = putMatch(t, best_ref, [](DMatch* copy) -> float* { */
					/* 	return &copy->distance; */
					/* }); */

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
				m[curr->trainIdx] = *curr;
				/* t = putMatch(t, curr, [](DMatch* copy) -> float* { */
				/* 	return &copy->distance; */
				/* }); */

				curr++; n++;
			} while (n<c);

		}
	}

#endif

	fprintf(stderr, "%u matches\n\n", n);

	/* return t; */
	return m;
}

typedef float (*diff_gfp_t)(void *, void*);

extern "C" {

#include "fifo.h"

}

#include "avl_iot.h"

template<typename GET_CLUSTERS_GKEY>
static void
get_clusters(
	fifo_t *clusters,
	avl_t *parent_avl,
	/* avl_t *root, */
	float eps,
	size_t min_elements,
	avl_compare_gfp_t cmp,
	GET_CLUSTERS_GKEY get_next_key)
{
	size_t size = 0;
	float last_key = 0;
	avl_t *subcluster_avl = NULL;

	/* fputs("\nCLUSTER? ", stderr); */
	avl_iot(parent_avl, [&] (avl_t * curr) {
		float key = *(float*) curr->key;

		if (key - last_key > eps) {
			if (size >= min_elements) {
				/* fprintf(stderr, " ACCEPT\n"); */
				fifo_push(clusters, subcluster_avl);
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
		fifo_push(clusters, subcluster_avl);
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
		avl_t *cluster_avl = (avl_t*) fifo_pop(source);

		get_clusters(result, cluster_avl, min_dist, min_elements,
				(avl_compare_gfp_t) compare_float_ptrs, gkey);
	}

	free(source);
	return result;
}

/* #include <time.h> */
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

	/* srand(time(NULL)); */

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
		map<int, DMatch>
			norm_m = matchf(query_d, d),
			rev_m = matchf(d, query_d);

		/* avl_t *cross_eucl_avl = NULL; */
		avl_t *cross_x_avl = NULL;

		for (map<int, DMatch>::iterator it = norm_m.begin();
				it != norm_m.end(); ++it) {

			DMatch *curr = &it->second;

			int trainIdx = it->first,
					queryIdx = curr->queryIdx;

			auto search = rev_m.find(queryIdx);

			if (search != rev_m.end() &&
					search->second.queryIdx == trainIdx) {

				/* cross_eucl_avl = avl_insert(cross_eucl_avl, (void*) curr, */
				/* 		(void*) &curr->distance, (avl_compare_gfp_t) compare_float_ptrs); */

				cross_x_avl = avl_insert(cross_x_avl, (void*) curr,
						(void*) &kp[curr->trainIdx].pt.x,
						(avl_compare_gfp_t) compare_float_ptrs);
			}
		}

		/* fputs("\nPRINTING TREE: ", stderr); */
		avl_iot(cross_x_avl, [&] (avl_t * curr) {
			Point2f matchp = kp[((DMatch*)curr->data)->trainIdx].pt;
			line(cframe, matchp, matchp, Scalar(0, 255, 255), 2);
			/* fprintf(stderr, "%.2f/", (double)*(float*)curr->key); */
		});

		/* /1* fputs("\nPRINTING TREE: ", stderr); *1/ */
		/* avl_iot(cross_eucl_avl, [&] (avl_t * curr) { */
		/* 	Point2f matchp = kp[((DMatch*)curr->data)->trainIdx].pt; */
		/* 	line(cframe, matchp, matchp, Scalar(0, 255, 255), 2); */
		/* 	/1* fprintf(stderr, "%.2f/", (double)*(float*)curr->key); *1/ */
		/* }); */

		/* fputs("\nGENERATING EUCL CLUSTERS SORTED BY X", stderr); */
		/* fifo_t *eucl_clusters_x = new_fifo(); */

		const int MIN_P = 10;
		const float MAX_XY_DIST = .03f,
					PX_MAX_XY_DIST = MAX_XY_DIST * (float)cframe.cols;


		/* get_clusters(eucl_clusters_x, cross_eucl_avl, 3.0f, MIN_P, */
		/* 		(avl_compare_gfp_t) compare_float_ptrs, */
		/* 		[&](DMatch *data) -> float* { return &kp[data->trainIdx].pt.x; }); */

		/* /1* fputs("\nGENERATING X CLUSTERS SORTED BY Y", stderr); *1/ */
		fifo_t *x_clusters_y = new_fifo();

		/* /1* fputs("\nGENERATING X CLUSTERS SORTED BY Y", stderr); *1/ */
		get_clusters(x_clusters_y, cross_x_avl, PX_MAX_XY_DIST, MIN_P,
				(avl_compare_gfp_t) compare_float_ptrs,
				[&](DMatch *data) -> float* { return &kp[data->trainIdx].pt.y; });

		/* fputs("\nGENERATING Y CLUSTERS SORTED BY Y", stderr); */
		fifo_t *y_clusters_y = re_cluster(x_clusters_y, PX_MAX_XY_DIST, MIN_P,
				[&](DMatch *data) -> float* { return &kp[data->trainIdx].pt.y; });

		{
			fifo_node_t **top = &y_clusters_y->top;
			size_t n_y_clusters_y = 0;

			while (*top) {
				avl_t *cluster_avl = (avl_t*) fifo_pop(y_clusters_y);

				size_t n_points = 0;

				float ymin = *(float*) avl_min(cluster_avl)->key,
							ymax = *(float*) avl_max(cluster_avl)->key;

				if (ymax - ymin < 3.0f) continue;

				float	xmin = (float) cframe.cols, xmax = 0;
				/* vector<Point2f> from_v, to_v; */

				avl_iot(cluster_avl, [&] (avl_t* avlnode) {
					DMatch *match = (DMatch*) avlnode->data;
					Point2f cluster_pt = kp[match->trainIdx].pt;

					line(cframe, cluster_pt, cluster_pt, Scalar(255, 255, 0), 2);
					/* to_v.push_back(cluster_pt); */
					/* from_v.push_back(query_kp[match->queryIdx].pt); */

					{
						float x = cluster_pt.x;
						if (x < xmin) xmin = x;
						if (x > xmax) xmax = x;
					}

					n_points++;
				});

				if (xmax - xmin < 3.0f) continue;

				/* Mat h = findHomography(from_v, to_v, CV_RANSAC); */
				/* if (!h.rows) continue; */

				/* const double det = h.at<double>(0,0)*h.at<double>(1,1) - \ */
				/* 									 h.at<double>(1,0)*h.at<double>(0,1); */ 

				/* /1* if ( abs(det) > 1 ) continue; *1/ */
				/* if ( det < 0 || det > 0.05 ) continue; */

				rectangle(cframe,
						/* Point2f(xmin, cluster->min), */
						/* Point2f(xmax, cluster->max), */
						Point2f(xmin, ymin),
						Point2f(xmax, ymax),
						Scalar(255, 0, 0), 1);


				ostringstream convert;
				convert << n_points;
				/* char n_matches_str[10]; */
				/* sprintf(n_matches_str, "%u", n_points); */
				putText(cframe, convert.str(), Point2f(xmax + 5, ymin), FONT_HERSHEY_PLAIN, .9, Scalar(0, 0, 255));

				/* int width = (int) (xmax - xmin), */
				/* 		height = (int) (ymax - ymin); */

				/* Rect bb((int)xmin, (int)ymin, width, height); */

				/* Mat subimg = cframe(bb).clone(); */

				/* Ptr<MSER> ms = MSER::create(); */
				/* vector<vector<Point>> regions; */
				/* vector<Rect> mserBBox; */
				/* ms->detectRegions(subimg, regions, mserBBox); */
				/* for (size_t i = 0; i < regions.size(); i++) { */
				/* 	drawContours(subimg, regions, i, Scalar(rand()%255, rand()%255, rand()%255), CV_FILLED); */
				/* } */

				/* subimg.copyTo(cframe(bb)); */

				avl_free(cluster_avl);

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
