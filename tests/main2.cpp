#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

#include <gtk/gtk.h>

#include <stdio.h>
#include <ctime>

using namespace std;
using namespace cv;

void profile(const char * label, clock_t start) {
	fprintf(stderr, "P %s: %fs\n", label, double(clock() - start)/CLOCKS_PER_SEC); 
}

void draw_rect(Mat &target_img, Mat &query_img, Mat homography) {
	vector<Point2f> q_corners(4);
	q_corners[0] = Point(0, 0);
	q_corners[1] = Point(query_img.cols, 0);
	q_corners[2] = Point(0, query_img.rows);
	q_corners[3] = Point(query_img.cols, query_img.rows);

	vector<Point2f> t_corners(4);
	perspectiveTransform(q_corners, t_corners, homography);
	Point2f translate = Point2f(query_img.cols, 0);

	line(target_img, t_corners[0] + translate, t_corners[1] + translate, Scalar(0, 255, 0), 4);
	line(target_img, t_corners[0] + translate, t_corners[2] + translate, Scalar(0, 255, 0), 4);
	line(target_img, t_corners[1] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4);
	line(target_img, t_corners[2] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4);
}

/* #include <string.h> */
/* void ocd_filter(vector<DMatch> & matches, Mat train_d, Mat query_d, void (*matchf)(Mat, Mat, vector<DMatch>&)) { */
/* 	double max_dist = 0, min_dist = 100000, avg_dist = 0; */

/* 	for (unsigned i=0; i<matches.size(); i++) { */
/* 		double dist = matches[i].distance; */
/* 		if (dist > max_dist) max_dist = dist; */
/* 		if (dist < min_dist) min_dist = dist; */
/* 		avg_dist += dist; */
/* 	} */

/* 	avg_dist /= matches.size(); */
/* 	fprintf(stderr, "min: %f max: %f avg: %f\n", min_dist, max_dist, avg_dist); */

/* 	double falloff = avg_dist * 0.5; */
/* 	max_dist = min(max_dist, avg_dist + falloff); */
/* 	min_dist = max(min_dist, avg_dist - falloff); */

/* 	match_fifo_t *mf2 = matchf(train_d, query_d); */

/* 	matches.erase(remove_if(matches.begin(), matches.end(), */
/* 		[&](DMatch const & match) -> bool { */
/* 			/1* bool good_distance = match.distance < max_dist; *1/ */
/* 			/1* bool good_distance = match.distance < max_dist && *1/ */
/* 			/1* 	match.distance > min_dist; *1/ */

/* 			/1* bool good_distance = true; *1/ */
/* 			bool good_distance = match.distance < max_dist && match.distance > min_dist; */
			
/* 			/1* bool good_distance = match.distance <= avg_dist + falloff && *1/ */
/* 			/1* 	match.distance >= avg_dist - falloff; *1/ */
/* 			/1* bool good_distance = match.distance <= 2.5*min_dist; *1/ */
/* 			/1* bool good_distance = match.distance <= max(2*min_dist, 0.05); *1/ */
/* 			/1* bool good_distance = match.distance <= max(3*min_dist, 0.05); *1/ */
/* 			if (!good_distance) return true; */
			
/* 			for (unsigned j=0; j<matches2.size(); j++) */
/* 				if (match.queryIdx == matches2[j].trainIdx && */
/* 						match.trainIdx == matches2[j].queryIdx) */
/* 					return false; */

/* 			return true; */
/* 		}), matches.end()); */


/* 	fprintf(stderr, "%u good matches\n", matches.size()); */
/* } */


#define use_flann false
#define use_cross_match false
#if use_flann
DescriptorMatcher *matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
#else
DescriptorMatcher *matcher = new BFMatcher(
		/* NORM_HAMMING */
		NORM_L2
		, use_cross_match
		);
#endif

typedef struct match_fifo_node_st {
	DMatch *value;
	struct match_fifo_node_st * prev;
} match_fifo_node_t;

typedef struct {
	match_fifo_node_t *top;
	unsigned count;
} match_fifo_t;

match_fifo_node_t *match_fifo_node_push(match_fifo_node_t *top, DMatch &value) {
	match_fifo_node_t *new_top = (match_fifo_node_t*) malloc(sizeof(match_fifo_node_t));
	new_top->value = &value;
	new_top->prev = top;
	return new_top;
}

void match_fifo_push(match_fifo_t *mf, DMatch &value) {
	mf->top = match_fifo_node_push(mf->top, value);
	mf->count++;
}

match_fifo_t *new_match_fifo() {
	match_fifo_t *mf = (match_fifo_t*) malloc(sizeof(match_fifo_t));
	mf->count = 0;
	mf->top = NULL;
	return mf;
}

#define use_knn true
match_fifo_t *matchf(Mat desc1, Mat desc2) {
	match_fifo_t *mf = new_match_fifo();

#if use_knn
	vector<vector<DMatch>> knnmatches;
	matcher->knnMatch(desc1, desc2, knnmatches, 2);

	/* David Lowe's Ratio test */
	for (unsigned i=0; i<knnmatches.size(); i++)
		if (knnmatches[i].size()>1 &&
				knnmatches[i][0].distance <= .8 * knnmatches[i][1].distance)
			match_fifo_push(mf, knnmatches[i][0]);

#else
	vector<DMatch>& save_matches;
	matcher->match(desc1, desc2, save_matches);
	for (unsigned i=0; i<matches.size(); i++)
		match_fifo_push(mf, matches[i]);

#endif
	return mf;
}

#define use_ransac false
Mat find_homography(
		vector<DMatch> & matches, 
		vector<KeyPoint> query_kp,
		vector<KeyPoint> train_kp)
{
		vector<Point2f> from_v, to_v;

		for (unsigned i=0; i<matches.size(); i++) {
			DMatch *match = &matches[i];
			from_v.push_back(query_kp[match->queryIdx].pt);
			to_v.push_back(train_kp[match->trainIdx].pt);
		}

#if use_ransac
		/* vector<unsigned char>inliersMask(matches.size()); */
		/* Mat h = findHomography(from_v, to_v, CV_RANSAC, 1, inliersMask); */

		/* const double det = h.at<double>(0,0)*h.at<double>(1,1) - \ */
		/* 									 h.at<double>(1,0)*h.at<double>(0,1); */ 

		/* const bool badh = det<0 || fabs(det)<0.05 || fabs(det)>0.75; */

		/* fprintf(stderr, "%f %u\n", det, matches.size()); */

		/* matches.erase(remove_if(matches.begin(), matches.end(), [&](DMatch const & match) -> bool { */
		/* 		return inliersMask[&match - &*matches.begin()] != 1; */
		/* 	}), matches.end()); */

		/* fprintf(stderr, "%u inliers\n", matches.size()); */

		return findHomography(from_v, to_v, CV_RANSAC, 2);
#else
		return findHomography(from_v, to_v, CV_LMEDS);
#endif
}

int main(int argc, char **argv) {
	/* if (use_cross_match && (use_flann || use_knn)) */
	/* 	throw "cross-matching not compatible with flann/knn (yet?)"; */

	/* Mat query = imread("tuna.png"); */
	Mat query = imread("tuna.png");
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

		Mat img_matches;

		vector<KeyPoint> kp;
		Mat d;

		clock_t start = clock();

		detector->detect(cframe, kp);
		extractor->compute(cframe, kp, d);

		times += clock() - start;
		n++;

		vector<DMatch> matches;
		match_fifo_t *mf = matchf(query_d, d);
		fprintf(stderr, "%u matches\n", mf->count);

		/* typedef struct match_range_st { */
		/* 	double frequency, start, end; */
		/* 	struct match_range_st *prev; */
		/* } match_range_t; */

		match_fifo_node_t *top = mf->top;
		float lx=0, ly=0;
		while (top) {
			Point2f &pt = kp[top->value->trainIdx].pt;

			if (pt.x < lx || pt.y < ly) {
				fputs("Not in order\n", stderr);
				return 1;
			}

			lx = pt.x;
			ly = pt.y;
			top = top->prev;
		}

		/* ocd_filter(matches, d, query_d, matchf); */

/* 		if (matches.size()) { */
/* 			drawMatches(query, query_kp, cframe, kp, matches, img_matches, */ 
/* 				Scalar::all(-1), CV_RGB(255,255,255), Mat(), 2); */

/* 			if (matches.size()>6) */
/* 				draw_rect(img_matches, query, find_homography(matches, query_kp, kp)); */
/* 			else fputs("Not enough matches for calculating homography\n", stderr); */


/* 			/1* match_range_t *topx = NULL; *1/ */
/* 			/1* for (unsigned i=0; i<matches.size(); i++) { *1/ */
				
/* 			/1* } *1/ */

/* 			unsigned ys[cframe.rows], xs[cframe.cols], max_row_count = 0, max_col_count = 0; */
/* 			memset(ys, 0, cframe.rows*sizeof(unsigned)); */
/* 			memset(xs, 0, cframe.cols*sizeof(unsigned)); */
/* 			for (unsigned i=0; i<matches.size(); i++) { */
/* 				Point2f *pt = &kp[matches[i].trainIdx].pt; */
/* 				unsigned row_count = ++ys[(unsigned)pt->y], */
/* 								 col_count = ++xs[(unsigned)pt->x]; */

/* 				if (row_count > max_row_count) max_row_count = row_count; */
/* 				if (col_count > max_col_count) max_col_count = col_count; */


/* 			} */

/* 			/1* unsigned blur_xs[cframe.cols]; *1/ */
/* 			/1* blur_xs[0] = xs[0]; *1/ */
/* 			/1* blur_xs[cframe.cols - 1] = xs[cframe.cols - 1]; *1/ */
/* 			/1* for (unsigned i=1; i<cframe.cols - 1; i++) *1/ */
/* 			/1* 	blur_xs[i] = (xs[i - 1] + xs[i] + xs[i + 1])/3; *1/ */

/* 			unsigned row_scale = cframe.cols/max_row_count, */ 
/* 							 col_scale = cframe.rows/max_col_count; */

/* 			for (unsigned i=0; i<cframe.rows-1; i++) */
/* 				line(img_matches, */
/* 						Point(query.cols + ys[i]*row_scale, i), */
/* 						Point(query.cols + ys[i+1]*row_scale, i+1), */
/* 						Scalar(0, 0, 255), 4); */

/* 			for (unsigned i=0; i<cframe.cols-1; i++) */
/* 				line(img_matches, */
/* 						Point(query.cols + i, xs[i]*col_scale), */
/* 						Point(query.cols + i + 1, xs[i+1]*col_scale), */
/* 						Scalar(255, 0, 0), 4); */


/* 			/1* imshow("d", cframe); *1/ */
			/* imshow("d", img_matches); */

			/* waitKey(0); */
		/* } else { */
		/* 	imshow("d", cframe); */
		/* } */

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
