#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

using namespace std;
using namespace cv;

#include "cross_match.hpp"
#include "subspace_clustering.hpp"
#include <stdio.h>

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

		stack<multimap<float, DMatch>> clusters =
			subspace_clustering(cross_match(matcher, query_d, d, kp),
					.03f * (float) cframe.cols, 10, kp);

		while (!clusters.empty()) {
			multimap<float, DMatch> cluster = clusters.top();
			clusters.pop();

			float ymin = cluster.begin()->first,
						ymax = (--cluster.end())->first;

			if (ymax - ymin < 3.0f) continue;

			float xmin = (float) cframe.cols, xmax = 0;

			for (multimap<float, DMatch>::iterator map_it = cluster.begin();
					map_it != cluster.end();
					++map_it)
			{
				DMatch match = map_it->second;
				Point2f cluster_pt = kp[match.trainIdx].pt;

				line(cframe, cluster_pt, cluster_pt, Scalar(255, 255, 0), 2);

				{
					float x = cluster_pt.x;
					if (x < xmin) xmin = x;
					if (x > xmax) xmax = x;
				}
			}

			if (xmax - xmin < 3.0f) continue;

			rectangle(cframe, Point2f(xmin, ymin), Point2f(xmax, ymax), Scalar(255, 0, 0), 1);

			{
				ostringstream convert;
				convert << cluster.size();
				putText(cframe, convert.str(), Point2f(xmax + 5, ymin), FONT_HERSHEY_PLAIN, .9, Scalar(0, 0, 255));
			}

		}

		imshow("d", cframe);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
