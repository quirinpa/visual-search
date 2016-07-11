#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

/* #include <gtk/gtk.h> */

#include <stdio.h>
#include <ctime>

using namespace std;
using namespace cv;

/* static void profile(const char * label, clock_t start) { */
/* 	fprintf(stderr, "P %s: %fs\n", label, double(clock() - start)/CLOCKS_PER_SEC); */ 
/* } */

/* static void draw_rect(Mat &target_img, Mat &query_img, Mat homography) { */
/* 	vector<Point2f> q_corners(4); */
/* 	q_corners[0] = Point(0, 0); */
/* 	q_corners[1] = Point(query_img.cols, 0); */
/* 	q_corners[2] = Point(0, query_img.rows); */
/* 	q_corners[3] = Point(query_img.cols, query_img.rows); */

/* 	vector<Point2f> t_corners(4); */
/* 	perspectiveTransform(q_corners, t_corners, homography); */
/* 	Point2f translate = Point2f((float)query_img.cols, 0); /1* int to float FIXME *1/ */

/* 	line(target_img, t_corners[0] + translate, t_corners[1] + translate, Scalar(0, 255, 0), 4); */
/* 	line(target_img, t_corners[0] + translate, t_corners[2] + translate, Scalar(0, 255, 0), 4); */
/* 	line(target_img, t_corners[1] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4); */
/* 	line(target_img, t_corners[2] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4); */
/* } */


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
static avl_t *matchf(Mat desc1, Mat desc2) {
	size_t n = 0;
	avl_t *t = NULL;

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
				if (currv->size() == 1 || *curv <= .8f * currv->at(1).distance) {
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
			FILE *output = fopen("output.html", "wt");

			fputs("<style>"
					"div {"
						"float:left;"
						"font-size:12px;"
						"text-align:center;"
						"overflow:hidden;"
					"} span {"
						"color:blue;"
					"} h2 {"
						"background:#333:"
						"color:white;"
					"}</style>", output);

			do {
				fputs("<h2>", output);
				t = avl_insert(t,
						( void* )curr,
						( void* )&curr->distance,
						( avl_compare_gfp_t ) compare_dist, output);
				fputs("</h2>\n", output);

				avl_to_html(t, output);
				/* print_avl(t); */

				curr++; n++;
			} while (n<c);

			fclose(output);
		}
	}

#endif

	fprintf(stderr, "%lu matches\n\n", n);

	return t;
}

/* typedef struct match_fifo_node_st { */
/* 	DMatch *value; */
/* 	struct match_fifo_node_st * prev; */
/* } match_fifo_node_t; */

/* typedef struct { */
/* 	match_fifo_node_t *top; */
/* 	unsigned count; */
/* } match_fifo_t; */

/* match_fifo_node_t *match_fifo_node_push(match_fifo_node_t *top, DMatch &value) { */
/* 	match_fifo_node_t *new_top = (match_fifo_node_t*) malloc(sizeof(match_fifo_node_t)); */
/* 	new_top->value = &value; */
/* 	new_top->prev = top; */
/* 	return new_top; */
/* } */

/* void match_fifo_push(match_fifo *mf, DMatch &value) { */
/* 	mf->top = match_fifo_node_push(mf->top, value); */
/* 	mf->count++; */
/* } */
/* match_fifo_t *matchf(Mat desc1, Mat desc2) { */
/* 	match_fifo_t *mf = (match_fifo_t*) malloc(sizeof(match_fifo_t)); */
/* 	match_fifo->count = 0; */

/* 	unsigned count = 0; */
/* #if use_knn */
/* 	vector<vector<DMatch>> knnmatches; */
/* 	matcher->knnMatch(desc1, desc2, knnmatches, 2); */

/* 	/1* David Lowe's Ratio test *1/ */
/* 	for (unsigned i=0; i<knnmatches.size(); i++) */
/* 		if (knnmatches[i].size()>1 && */
/* 				knnmatches[i][0].distance <= .8 * knnmatches[i][1].distance) */
/* 			match_fifo_push(mf, knnmatches[i][0]; */

/* #else */
/* 	vector<DMatch>& save_matches; */
/* 	matcher->match(desc1, desc2, save_matches); */
/* 	for (unsigned i=0; i<matches.size(); i++) */
/* 		match_fifo_push(mf, matches[i]); */

/* #endif */
/* 	return mf; */
/* } */


/* #include <string.h> */
/* void ocd_filter(avltree_t *t, Mat train_d, Mat query_d, void (*matchf)(Mat, Mat, vector<DMatch>&)) { */
/* 	double freq = 0, interval = 1; */

/* 	avl_iot(t, [&](DMatch *m) -> void { */
/* 			double dist = m->distance; */
/* 			if (dist > max_dist) max_dist = dist; */
/* 			if (dist < min_dist) min_dist = dist; */
/* 			avg_dist += dist; */
/* 		}); */

/* 	hile (cur) { */
/* 		double dist = cur->distance; */
/* 		if (dist > max_dist) max_dist = dist; */
/* 		if (dist < min_dist) min_dist = dist; */
/* 		avg_dist += dist; */

/* 		cur = cur->prev; */
/* 	} */

/* 	avg_dist /= mf.count; */
/* 	fprintf(stderr, "min: %f max: %f avg: %f\n", min_dist, max_dist, avg_dist); */

/* 	double falloff = avg_dist * 0.5; */
/* 	max_dist = min(max_dist, avg_dist + falloff); */
/* 	min_dist = max(min_dist, avg_dist - falloff); */

/* 	match_fifo_t *mf2 = matchf(train_d, query_d); */

/* 	match_fifo_node_t *prev = NULL; */
/* 	cur = mf->top; */

/* 	while (cur) { */
/* 		DMatch *match = cur->value; */
/* 		/1* need AVL HERE (quick lookup) *1/ */
/* 		if (!(match->distance < max_dist && match->distance > min_dist && ocd_cross_matched(match))) { */

/* 			free(cur->value); */
/* 			if (!prev) mf->top = cur->prev; */
/* 			else { */
/* 				match_fifo_node_t * */
/* 				cur = cur->prev; */
/* 				prev->prev = cur; */
/* 			} */
/* 			free(cur->value); */
/* 			prev->prev = cur- */
			

/* 		cur = up->prev; */
/* 		if (!cur->value->distance < max_dist && cur->value->distance > min_dist &&|| ocd_cross_matched(cur->value)) { */
/* 			up->prev = cur->prev; */
/* 			free(cur->value); */
/* 			free(cur); */
/* 		} */
/* 		up = up->prev; */
/* 	} */

/* 	fprintf(stderr, "%u good matches\n", matches.size()); */
/* } */

/* #include <string.h> */
/* void ocd_filter(match_fifo_t *mf, Mat train_d, Mat query_d, void (*matchf)(Mat, Mat, vector<DMatch>&)) { */
/* 	double max_dist = 0, min_dist = 100000, avg_dist = 0; */
/* 	match_fifo_node_t *cur = mf->top; */

/* 	while (cur) { */
/* 		double dist = cur->distance; */
/* 		if (dist > max_dist) max_dist = dist; */
/* 		if (dist < min_dist) min_dist = dist; */
/* 		avg_dist += dist; */

/* 		cur = cur->prev; */
/* 	} */

/* 	avg_dist /= mf.count; */
/* 	fprintf(stderr, "min: %f max: %f avg: %f\n", min_dist, max_dist, avg_dist); */

/* 	double falloff = avg_dist * 0.5; */
/* 	max_dist = min(max_dist, avg_dist + falloff); */
/* 	min_dist = max(min_dist, avg_dist - falloff); */

/* 	match_fifo_t *mf2 = matchf(train_d, query_d); */

/* 	match_fifo_node_t *prev = NULL; */
/* 	cur = mf->top; */

/* 	while (cur) { */
/* 		DMatch *match = cur->value; */
/* 		/1* need AVL HERE (quick lookup) *1/ */
/* 		if (!(match->distance < max_dist && match->distance > min_dist && ocd_cross_matched(match))) { */

/* 			free(cur->value); */
/* 			if (!prev) mf->top = cur->prev; */
/* 			else { */
/* 				match_fifo_node_t * */
/* 				cur = cur->prev; */
/* 				prev->prev = cur; */
/* 			} */
/* 			free(cur->value); */
/* 			prev->prev = cur- */
			

/* 		cur = up->prev; */
/* 		if (!cur->value->distance < max_dist && cur->value->distance > min_dist &&|| ocd_cross_matched(cur->value)) { */
/* 			up->prev = cur->prev; */
/* 			free(cur->value); */
/* 			free(cur); */
/* 		} */
/* 		up = up->prev; */
/* 	} */

/* 	/1* matches.erase(remove_if(matches.begin(), matches.end(), *1/ */
/* 	/1* 	[&](DMatch const & match) -> bool { *1/ */
/* 	/1* 		/2* bool good_distance = match.distance < max_dist; *2/ *1/ */
/* 	/1* 		/2* bool good_distance = match.distance < max_dist && *2/ *1/ */
/* 	/1* 		/2* 	match.distance > min_dist; *2/ *1/ */

/* 	/1* 		/2* bool good_distance = true; *2/ *1/ */
/* 	/1* 		bool good_distance = match.distance < max_dist && match.distance > min_dist; *1/ */
			
/* 	/1* 		/2* bool good_distance = match.distance <= avg_dist + falloff && *2/ *1/ */
/* 	/1* 		/2* 	match.distance >= avg_dist - falloff; *2/ *1/ */
/* 	/1* 		/2* bool good_distance = match.distance <= 2.5*min_dist; *2/ *1/ */
/* 	/1* 		/2* bool good_distance = match.distance <= max(2*min_dist, 0.05); *2/ *1/ */
/* 	/1* 		/2* bool good_distance = match.distance <= max(3*min_dist, 0.05); *2/ *1/ */
/* 	/1* 		if (!good_distance) return true; *1/ */
			
/* 	/1* 		for (unsigned j=0; j<matches2.size(); j++) *1/ */
/* 	/1* 			if (match.queryIdx == matches2[j].trainIdx && *1/ */
/* 	/1* 					match.trainIdx == matches2[j].queryIdx) *1/ */
/* 	/1* 				return false; *1/ */

/* 	/1* 		return true; *1/ */
/* 	/1* 	}), matches.end()); *1/ */


/* 	fprintf(stderr, "%u good matches\n", matches.size()); */
/* } */


/* #define use_ransac false */
/* static Mat find_homography( */
/* 		vector<DMatch> & matches, */ 
/* 		vector<KeyPoint> query_kp, */
/* 		vector<KeyPoint> train_kp) */
/* { */
/* 		vector<Point2f> from_v, to_v; */

/* 		for (unsigned i=0; i<matches.size(); i++) { */
/* 			DMatch *match = &matches[i]; */
/* 			from_v.push_back(query_kp[match->queryIdx].pt); */
/* 			to_v.push_back(train_kp[match->trainIdx].pt); */
/* 		} */

/* #if use_ransac */
/* 		/1* vector<unsigned char>inliersMask(matches.size()); *1/ */
/* 		/1* Mat h = findHomography(from_v, to_v, CV_RANSAC, 1, inliersMask); *1/ */

/* 		/1* const double det = h.at<double>(0,0)*h.at<double>(1,1) - \ *1/ */
/* 		/1* 									 h.at<double>(1,0)*h.at<double>(0,1); *1/ */ 

/* 		/1* const bool badh = det<0 || fabs(det)<0.05 || fabs(det)>0.75; *1/ */

/* 		/1* fprintf(stderr, "%f %u\n", det, matches.size()); *1/ */

/* 		/1* matches.erase(remove_if(matches.begin(), matches.end(), [&](DMatch const & match) -> bool { *1/ */
/* 		/1* 		return inliersMask[&match - &*matches.begin()] != 1; *1/ */
/* 		/1* 	}), matches.end()); *1/ */

/* 		/1* fprintf(stderr, "%u inliers\n", matches.size()); *1/ */

/* 		return findHomography(from_v, to_v, CV_RANSAC, 2); */
/* #else */
/* 		return findHomography(from_v, to_v, CV_LMEDS); */
/* #endif */
/* } */

#include "fifo.h"
/* int main(int argc, char **argv) { */
int main(void) {
	/* if (use_cross_match && (use_flann || use_knn)) */
	/* 	throw "cross-matching not compatible with flann/knn (yet?)"; */

	/* Mat query = imread("tuna.png"); */
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

		vector<DMatch> matches;
		/* avl_t *tree = */ 
		avl_t *eucl_avl = matchf(query_d, d);
		free_avl(eucl_avl);

		/* DMatch *dmax = (DMatch*) avl_max(tree->root); */
		/* if (dmax) { */
		/* 	Point2f lastp(0, 0); */
		/* 	float xpand = (float)query.cols/dmax->distance, */
		/* 				lastdist = 0; /1* FIXME side effects? *1/ */

		/* 	fifo_t *stack = avl_iot(tree); */
		/* 	DMatch *m; */
			
		/* 	while ((m = (DMatch*) fifo_pop(stack))) { */
		/* 		float dist = m->distance; */
		/* 		Point2f curp(dist*xpand, (float)query.rows/(dist - lastdist)); */
		/* 		lastdist = dist; */

		/* 		line(cframe, lastp, curp, Scalar(255, 0, 0), 4); */

		/* 		lastp = curp; */
		/* 	} */
		/* } */


		/* Point2f curp(m->distance, 0); */
		/* curp */
		/* float dist = m->distance, freq = 1/(dist - lastdist); */

		/* Point2f curp(dist, freq); */
		/* line(cframe, Point(lastdist*xpand, lastfreq), Point(currp.x, lastp.y), Scalar(255, 0, 0), 4); */ /* lastp = curp; */
		/* return (void*); */

		/* line(cframe, lastp, Point(currp.x, lastp.y), Scalar(255, 0, 0), 4); */
		/* typedef struct match_range_st { */
		/* 	double frequency, start, end; */
		/* 	struct match_range_st *prev; */
		/* } match_range_t; */

		/* ocd_filter(matches, d, query_d, matchf); */

		/* if (matches.size()) { */
		/* 	drawMatches(query, query_kp, cframe, kp, matches, img_matches, */ 
		/* 		Scalar::all(-1), CV_RGB(255,255,255), Mat(), 2); */

		/* 	if (matches.size()>6) */
		/* 		draw_rect(img_matches, query, find_homography(matches, query_kp, kp)); */
		/* 	else fputs("Not enough matches for calculating homography\n", stderr); */


		/* 	/1* match_range_t *topx = NULL; *1/ */
		/* 	/1* for (unsigned i=0; i<matches.size(); i++) { *1/ */
				
		/* 	/1* } *1/ */

		/* 	unsigned ys[cframe.rows], xs[cframe.cols], max_row_count = 0, max_col_count = 0; */
		/* 	memset(ys, 0, cframe.rows*sizeof(unsigned)); */
		/* 	memset(xs, 0, cframe.cols*sizeof(unsigned)); */
		/* 	for (unsigned i=0; i<matches.size(); i++) { */
		/* 		Point2f *pt = &kp[matches[i].trainIdx].pt; */
		/* 		unsigned row_count = ++ys[(unsigned)pt->y], */
		/* 						 col_count = ++xs[(unsigned)pt->x]; */

		/* 		if (row_count > max_row_count) max_row_count = row_count; */
		/* 		if (col_count > max_col_count) max_col_count = col_count; */


		/* 	} */

		/* 	/1* unsigned blur_xs[cframe.cols]; *1/ */
		/* 	/1* blur_xs[0] = xs[0]; *1/ */
		/* 	/1* blur_xs[cframe.cols - 1] = xs[cframe.cols - 1]; *1/ */
		/* 	/1* for (unsigned i=1; i<cframe.cols - 1; i++) *1/ */
		/* 	/1* 	blur_xs[i] = (xs[i - 1] + xs[i] + xs[i + 1])/3; *1/ */

		/* 	unsigned row_scale = cframe.cols/max_row_count, */ 
		/* 					 col_scale = cframe.rows/max_col_count; */

		/* 	for (unsigned i=0; i<cframe.rows-1; i++) */
		/* 		line(img_matches, */
		/* 				Point(query.cols + ys[i]*row_scale, i), */
		/* 				Point(query.cols + ys[i+1]*row_scale, i+1), */
		/* 				Scalar(0, 0, 255), 4); */

		/* 	for (unsigned i=0; i<cframe.cols-1; i++) */
		/* 		line(img_matches, */
		/* 				Point(query.cols + i, xs[i]*col_scale), */
		/* 				Point(query.cols + i + 1, xs[i+1]*col_scale), */
		/* 				Scalar(255, 0, 0), 4); */


		/* 	/1* imshow("d", cframe); *1/ */
			/* imshow("d", img_matches); */

		/* 	/1* waitKey(0); *1/ */
		/* } else { */
			imshow("d", cframe);
		/* } */

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
