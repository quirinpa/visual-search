#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

#include <gtk/gtk.h>

#include <stdio.h>
#include <ctime>

using namespace std;
using namespace cv;

#define use_cross_match true
#define use_flann false
#define use_knn false

void profile(const char * label, clock_t start) {
	fprintf(stderr, "P %s: %fs\n", label, double(clock() - start)/CLOCKS_PER_SEC); 
}

void matchImage(
		Ptr<FeatureDetector> &detector,
		Ptr<DescriptorExtractor> &extractor,
		void (*matchf)(Mat &, Mat &, vector<DMatch> &),
		Mat &query_d,
		Mat &target
		)
{
	vector<KeyPoint> kp; Mat d;

	detector->detect(cframe, kp);
	extractor->compute(cframe, kp, d);

	vector<DMatch> matches;
	matchf(query_d, d, matches);

	fprintf(stderr, "%u matches\n", matches.size());

#if 0
	vector<DMatch> matches2;
	matchf(d, query_d, matches);

	double max_dist = 0, min_dist = 100;

	for (unsigned i=0; i<matches.size(); i++) {
		double dist = matches[i].distance;
		if (dist > max_dist) max_dist = dist;
		if (dist < min_dist) min_dist = dist;
	}

	matches.erase(remove_if(matches.begin(), matches.end(),
				[&](DMatch const & match) -> bool {
				for (unsigned j=0; j<matches2.size(); j++)
				if (match.queryIdx == matches2[j].trainIdx &&
						match.trainIdx == matches2[j].queryIdx &&
						match.distance <= max(2*min_dist, 0.005))
				return true;
				return false;
				}), matches.end());

	fprintf(stderr, "%u good matches\n", matches.size());
#endif

	if (!matches.size()) return;

	vector<Point2f> from_v, to_v;

	for (unsigned i=0; i<matches.size(); i++) {
		DMatch *match = &matches[i];
		from_v.push_back(query_kp[match->queryIdx].pt);
		to_v.push_back(kp[match->trainIdx].pt);
	}

	vector<unsigned char>inliersMask(matches.size());
	Mat h = findHomography(from_v, to_v, CV_RANSAC, 1, inliersMask);
	/* Mat h = findHomography(from_v, to_v, CV_LMEDS, 0, inliersMask); */

	const double det = h.at<double>(0,0)*h.at<double>(1,1) - \
										 h.at<double>(1,0)*h.at<double>(0,1); 

	const bool badh = det<0 || fabs(det)<0.05 || fabs(det)>0.75;

	if (badh) return;

	fprintf(stderr, "%f %u\n", det, matches.size());

	matches.erase(remove_if(matches.begin(), matches.end(), [&](DMatch const & match) -> bool {
				return inliersMask[&match - &*matches.begin()] != 1;
				}), matches.end());

	fprintf(stderr, "%u inliers\n", matches.size());

	if (!matches.size()) return;
}

void matchImages(
		Ptr<FeatureDetector> &detector,
		Ptr<DescriptorExtractor> &extractor,
		void (*matchf)(Mat &, Mat &, vector<DMatch> &),
		Mat &query_d,
		vector<Mat> &targets)
{
	vector<vector<DMatch>> results(targets.size());

	for (size_t i = 0; i<targets.size(); i++)
		results[i] = matchImage(
				detector, extractor,
				matchf, query_d, tartgets[i]);

}

int newmain(int argc, char **argv) {
	if (use_cross_match && (use_flann || use_knn))
		throw "cross-matching not compatible with flann/knn (yet?)";

	Mat query = imread("tuna.png");

	Ptr<FeatureDetector> detector = BRISK::create();

	vector<KeyPoint> query_kp;
	detector->detect(query, query_kp);

	Ptr<DescriptorExtractor> extractor = BRISK::create();

	Mat query_d;
	extractor->compute(query, query_kp, query_d);

	DescriptorMatcher *matcher;
	if (use_flann)
		matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
	else
		matcher = new BFMatcher(
				/* NORM_HAMMING */
				NORM_L2
				, use_cross_match);


	namedWindow("d", CV_WINDOW_NORMAL);

	/* unsigned n = 0; */
	/* clock_t times = 0; */
	VideoCapture cap(0);
	if(!cap.isOpened()) return -1;

	do {
		Mat cframe;
		cap >> cframe;

		matchImage(detector, extractor, matchf, query_d, cframe);
	} while (waitKey(1)!='\x1b');

}
void matchImage(
		Ptr<FeatureDetector> &detector,
		Ptr<DescriptorExtractor> &extractor,
		void (*matchf)(Mat &, Mat &, vector<DMatch> &),
		Mat &query_d,
		Mat &target
		)
{
	vector<KeyPoint> kp; Mat d;

	detector->detect(cframe, kp);
	extractor->compute(cframe, kp, d);

	vector<DMatch> matches;
	matchf(query_d, d, matches);

	fprintf(stderr, "%u matches\n", matches.size());

#if 0
	vector<DMatch> matches2;
	matchf(d, query_d, matches);

	double max_dist = 0, min_dist = 100;

	for (unsigned i=0; i<matches.size(); i++) {
		double dist = matches[i].distance;
		if (dist > max_dist) max_dist = dist;
		if (dist < min_dist) min_dist = dist;
	}

	matches.erase(remove_if(matches.begin(), matches.end(),
				[&](DMatch const & match) -> bool {
				for (unsigned j=0; j<matches2.size(); j++)
				if (match.queryIdx == matches2[j].trainIdx &&
						match.trainIdx == matches2[j].queryIdx &&
						match.distance <= max(2*min_dist, 0.005))
				return true;
				return false;
				}), matches.end());

	fprintf(stderr, "%u good matches\n", matches.size());
#endif

	if (!matches.size()) return;

	vector<Point2f> from_v, to_v;

	for (unsigned i=0; i<matches.size(); i++) {
		DMatch *match = &matches[i];
		from_v.push_back(query_kp[match->queryIdx].pt);
		to_v.push_back(kp[match->trainIdx].pt);
	}

	vector<unsigned char>inliersMask(matches.size());
	Mat h = findHomography(from_v, to_v, CV_RANSAC, 1, inliersMask);
	/* Mat h = findHomography(from_v, to_v, CV_LMEDS, 0, inliersMask); */

	const double det = h.at<double>(0,0)*h.at<double>(1,1) - \
										 h.at<double>(1,0)*h.at<double>(0,1); 

	const bool badh = det<0 || fabs(det)<0.05 || fabs(det)>0.75;

	if (badh) return;

	fprintf(stderr, "%f %u\n", det, matches.size());

	matches.erase(remove_if(matches.begin(), matches.end(), [&](DMatch const & match) -> bool {
				return inliersMask[&match - &*matches.begin()] != 1;
				}), matches.end());

	fprintf(stderr, "%u inliers\n", matches.size());

	if (!matches.size()) return;
}

void matchImages(
		Ptr<FeatureDetector> &detector,
		Ptr<DescriptorExtractor> &extractor,
		void (*matchf)(Mat &, Mat &, vector<DMatch> &),
		Mat &query_d,
		vector<Mat> &targets)
{
	vector<vector<DMatch>> results(targets.size());

	for (size_t i = 0; i<targets.size(); i++)
		results[i] = matchImage(
				detector, extractor,
				matchf, query_d, tartgets[i]);

}

int newmain(int argc, char **argv) {
	if (use_cross_match && (use_flann || use_knn))
		throw "cross-matching not compatible with flann/knn (yet?)";

	Mat query = imread("tuna.png");

	Ptr<FeatureDetector> detector = BRISK::create();

	vector<KeyPoint> query_kp;
	detector->detect(query, query_kp);

	Ptr<DescriptorExtractor> extractor = BRISK::create();

	Mat query_d;
	extractor->compute(query, query_kp, query_d);

	DescriptorMatcher *matcher;
	if (use_flann)
		matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
	else
		matcher = new BFMatcher(
				/* NORM_HAMMING */
				NORM_L2
				, use_cross_match);


	namedWindow("d", CV_WINDOW_NORMAL);

	/* unsigned n = 0; */
	/* clock_t times = 0; */
	VideoCapture cap(0);
	if(!cap.isOpened()) return -1;

	do {
		Mat cframe;
		cap >> cframe;

		matchImage(detector, extractor, matchf, query_d, cframe);
	} while (waitKey(1)!='\x1b');

}

int main(int argc, char **argv) {
	if (use_cross_match && (use_flann || use_knn))
		throw "cross-matching not compatible with flann/knn (yet?)";

	Mat query = imread("tuna.png");
	/* TODO ??? FFME */

	/* MSER, FAST, ORB, KAZE, AKAZE TODO: ETC */
	/* auto detector = AKAZE::create(); */
	/* FastFeatureDetector, MSER, ORB */

	/* FastFeatureDetector - fast, se  rot */
	/* MSER - crap */
	/* ORB - areas de contraste, bom suporte de rotação */
	/* BRISK - not bad */
	/* GFTTDetector */
	/* SimpleBlobDetector - is not aplicable */
	/* KAZE - slow, big, no rot */
	/* AKAZE - slow, big, no rot */
	/* SIFT - slow, many, rot, occ */
	/* auto detector = BRISK::create(); */
	Ptr<FeatureDetector> detector = BRISK::create();
	vector<KeyPoint> query_kp;
	/* detector->detect(query, query_kp, Mat(0, 0, CV_8UC1)); */
	detector->detect(query, query_kp);

	/* xfeatures2d::SURF, xfeatures2d::SIFT, SURF, ORB, KAZE, AKAZE, BRISK, BRIEF, FREAK TODO: ETC */
	Ptr<DescriptorExtractor> extractor = BRISK::create();
	Mat query_d;
	extractor->compute(query, query_kp, query_d);

	DescriptorMatcher *matcher;
	if (use_flann)
		matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
	else
		matcher = new BFMatcher(
				/* NORM_HAMMING */
				NORM_L2
				, use_cross_match);

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

		/* profile("detection", start); */

		clock_t start = clock();

		detector->detect(cframe, kp);
		extractor->compute(cframe, kp, d);

		times += clock() - start;
		n++;


		/* profile("computation", start); */

#if use_knn
#define matchf(desc1, desc2, matches) \
		vector<vector<DMatch>> knnmatches; \
		matcher->knnMatch(desc1, desc2, knnmatches, 2); \
		/* David Lowe's Ratio test */ \
		for (unsigned i=0; i<knnmatches.size(); i++) \
		if (knnmatches[i].size()>1 && \
				knnmatches[i][0].distance <= .8 * knnmatches[i][1].distance) \
		matches.push_back(knnmatches[i][0])
#else
#define matchf(desc1, desc2, matches) \
		matcher->match(desc1, desc2, matches)
#endif

		vector<DMatch> matches;
		matchf(query_d, d, matches);
		fprintf(stderr, "%u matches\n", matches.size());

		/* profile("matching", start); */

#if 0
		vector<DMatch> matches2;
		matchf(d, query_d, matches);

		double max_dist = 0, min_dist = 100;

		for (unsigned i=0; i<matches.size(); i++) {
			double dist = matches[i].distance;
			if (dist > max_dist) max_dist = dist;
			if (dist < min_dist) min_dist = dist;
		}

		matches.erase(remove_if(matches.begin(), matches.end(),
					[&](DMatch const & match) -> bool {
					for (unsigned j=0; j<matches2.size(); j++)
					if (match.queryIdx == matches2[j].trainIdx &&
							match.trainIdx == matches2[j].queryIdx &&
							match.distance <= max(2*min_dist, 0.005))
					return true;
					return false;
					}), matches.end());

		fprintf(stderr, "%u good matches\n", matches.size());
#endif

		if (!matches.size()) {
			imshow("d", cframe);
			continue;
		}

#if 0 
		drawMatches(query, query_kp, cframe, kp, matches, img_matches, 
				Scalar::all(-1), CV_RGB(255,255,255), Mat(), 2);

		imshow("d", img_matches);
		waitKey(0);
#endif

		vector<Point2f> from_v, to_v;

		for (unsigned i=0; i<matches.size(); i++) {
			DMatch *match = &matches[i];
			from_v.push_back(query_kp[match->queryIdx].pt);
			to_v.push_back(kp[match->trainIdx].pt);
		}

		vector<unsigned char>inliersMask(matches.size());
		Mat h = findHomography(from_v, to_v, CV_RANSAC, 1, inliersMask);
		/* Mat h = findHomography(from_v, to_v, CV_LMEDS, 0, inliersMask); */

		const double det = h.at<double>(0,0)*h.at<double>(1,1) - \
											 h.at<double>(1,0)*h.at<double>(0,1); 

		const bool badh = det<0 || fabs(det)<0.05 || fabs(det)>0.75;

		fprintf(stderr, "%f %u\n", det, matches.size());

		matches.erase(remove_if(matches.begin(), matches.end(), [&](DMatch const & match) -> bool {
				return inliersMask[&match - &*matches.begin()] != 1;
			}), matches.end());


		fprintf(stderr, "%u inliers\n", matches.size());

		if (matches.size()) {
#if 1
			drawMatches(query, query_kp, cframe, kp, matches, img_matches, 
				Scalar::all(-1), CV_RGB(255,255,255), Mat(), 2);

			vector<Point2f> q_corners(4);
			q_corners[0] = Point(0, 0);
			q_corners[1] = Point(query.cols, 0);
			q_corners[2] = Point(0, query.rows);
			q_corners[3] = Point(query.cols, query.rows);

			vector<Point2f> t_corners(4);
			perspectiveTransform(q_corners, t_corners, h);
			Point2f translate = Point2f(query.cols, 0);

			line(img_matches, t_corners[0] + translate, t_corners[1] + translate, Scalar(0, 255, 0), 4);
			line(img_matches, t_corners[0] + translate, t_corners[2] + translate, Scalar(0, 255, 0), 4);
			line(img_matches, t_corners[1] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4);
			line(img_matches, t_corners[2] + translate, t_corners[3] + translate, Scalar(0, 255, 0), 4);

#endif
			imshow("d", img_matches);
		} else {
			imshow("d", cframe);
		}

	} while (waitKey(1)!='\x1b');


	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
