#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

using namespace std;
using namespace cv;

#include "cross_match.hpp"
#include "subspace_clustering.hpp"
#include "roundhash.h"
#include <stdio.h>

#define CLUSTERING_MIN_POINTS 10U
#define CLUSTERING_MAX_DIST 10.0f

int main(void) {
	Mat query = imread("resources/ss.png");
	Ptr<FeatureDetector> detector = BRISK::create();

	vector<KeyPoint> query_kp;
	detector->detect(query, query_kp);

	Ptr<DescriptorExtractor> extractor = BRISK::create();

	Mat query_d;
	extractor->compute(query, query_kp, query_d);

#if 0 
	static const FlannBasedMatcher matcher(new flann::LshIndexParams(5, 24, 2));
#else
	static const BFMatcher matcher(
		/* NORM_HAMMING */
		NORM_L2
		, false
	);
#endif

	VideoCapture cap(0);
	if(!cap.isOpened()) return -1;

	namedWindow("d", CV_WINDOW_NORMAL);

	unsigned n = 0;
	clock_t times = 0;

	do {
		Mat cframe;
		cap >> cframe;

		vector<KeyPoint> kp;
		Mat d;

		clock_t start = clock();

		detector->detect(cframe, kp);

		if (!kp.size()) {
			imshow("d", cframe);
			continue;
		}

		extractor->compute(cframe, kp, d);

		times += clock() - start;
		n++;

		RHC<DMatch> rh(Point2i(cframe.cols, cframe.rows), CLUSTERING_MAX_DIST);

		subspace_clustering(cross_match(matcher, query_d, d, kp),
				CLUSTERING_MAX_DIST, CLUSTERING_MIN_POINTS, kp,
				[&](DMatch& match) {
					Point2f pt = kp[match.trainIdx].pt;
					rh.insert(match, Point2f(pt.x, pt.y));
				}, [&]() {
				}, [&]() {
				});

		size_t n_clusters = 0, size;
		float ymin, xmin, ymax, xmax;

		size = 0;
		ymin = (float) cframe.rows;
		xmin = (float) cframe.cols;
		ymax = xmax = 0;

		for(float x = .0f; x < (float) cframe.cols; x+=CLUSTERING_MAX_DIST)
			line(cframe, Point2f(x, .0f), Point2f(x, (float)cframe.rows), Scalar(0, 0, 255), 1);


		for(float y = .0f; y < (float) cframe.rows; y+=CLUSTERING_MAX_DIST)
			line(cframe, Point2f(.0f, y), Point2f((float)cframe.cols, y), Scalar(0, 0, 255), 1);

		rh.cluster([&](DMatch& match) {
					Point2f pt = kp[match.trainIdx].pt;

					{
						register float x = pt.x, y = pt.y;

						if (x < xmin) xmin = x;
						else if (x > xmax) xmax = x;

						if (y < ymin) ymin = y;
						else if (y > ymax) ymax = y;
					}
					
					line(cframe, pt, pt, Scalar(255, 255, 0), 2);
					size++;
					
				}, [&]() {
					/* float width = xmax - xmin, height = ymax - ymin; */

					if (size != 1) fprintf(stderr, "%u/", size);

					/* if (width > 3.0f && height > 3.0f) { */
						rectangle(cframe, Point2f(xmin, ymin), Point2f(xmax, ymax), Scalar(255, 0, 0), 1);

						{
							ostringstream convert;
							convert << size;
							putText(cframe, convert.str(), Point2f(xmax + 5, ymin), FONT_HERSHEY_PLAIN, .9, Scalar(0, 0, 255));
						}

						n_clusters++;
					/* } */

					size = 0;
					ymin = (float) cframe.rows;
					xmin = (float) cframe.cols;
					ymax = xmax = 0;
				});


		fprintf(stderr, "valid clusters: %u\n", n_clusters);
		imshow("d", cframe);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
