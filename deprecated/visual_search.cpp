#include "qargp.h"

/* template <class T> */
/* static Ptr<T> */
/* get_algorithm(size_t id) { */
/* 	switch (id) { */
/* 		case 0: */
/* 			return cv::ORB::create(); */
/* 		case 1: */
/* 			return cv::xfeatures2d::SIFT::create(); */
/* 		case 2: */
/* 			return cv::xfeatures2d::SURF::create(); */
/* 		case 3: */
/* 			return cv::BRISK::create(); */
/* 		case 4: */
/* 			return cv::KAZE::create(); */
/* 		case 5: */
/* 			return cv::AKAZE::create(); */
/* 	} */

/* 	return cv::ORB::create(); */
/* } */

/* enum algorihm_t { */
/* 	ORB = 0; */
/* 	SIFT = 1; */
/* 	SUFT = 2; */
/* 	BRISK = 3; */
/* 	KAZE = 4; */
/* 	AKAZE = 5; */
/* } */

#include "stdio.h"
#include <vector>
using std::vector;

using cv::KeyPoint;

static
vector<KeyPoint> read_keypoints(void)
{
	size_t n;
	freadf(stdin, &n, sizeof(size_t));

	vector<KeyPoint> kp(n);
	for (size_t i = 0; i < n; i++)
		freadf(stdin, &kp[i], sizeof(KeyPoint));

	return kp;
}

using cv::Mat;

static
Mat read_descriptors(void)
{
	size_t cols, rows;
	freadf(stdin, &cols, sizeof(size_t));
	freadf(stdin, &rows, sizeof(size_t));
	Mat descriptors(cols, rows, CV_64FC1);

	for (size_t x = 0; x < cols; x++)
		for (size_t y = 0; y < rows; y++)
			freadf(stdin, &descriptors.at<double>(x, y), sizeof(double));

	return descriptors;
}


int main(int argc, char **argv) {
	/* arguments_t arguments = get_arguments(argc, argv); */

	/* initial validation */
	/* size_t detector_id = arguments.feature_detector; */
	/* size_t extractor_id = arguments.descriptor_extractor; */

/* 	if (detector_id > 5) { */
/* 		fprintf(stderr, "%u is does not correspond to a valid feature detector;", detector_id); */
/* 		return 1; */
/* 	} */
	
/* 	if (detector_id > 3 && detector_id != extractor_id) { */
/* 		fputs("Chosen KAZE or AKAZE feature detector does not match descriptor extractor (must be of the same type).", stderr); */
/* 		return 1; */
/* 	} */

	static const cv::BFMatcher
		matcher( detector_id == ORB ? cv::NORM_HAMMING : cv::NORM_L2 );

	vector<KeyPoint> query_kp = read_keypoints();
	Mat query_d = read_descriptors();

	do {
		char control; 
		freadf(stdin, &control, sizeof(char));

		if (control == EOF) return 0;

		vector<KeyPoint> kp = read_keypoints();
		Mat d = read_descriptors();

		/* DO CORRESPONDANCE, OUTPUT FILENAME + rectangle positions (optionally) */
		cross_match(matcher, query_d, d, kp);
		/*********************************************************/
	} while (true);

	/* scanf("%s", (char*) query_kp) */
	/* Mat query = imread(arguments.query_path); */

	/* Ptr<cv::FeatureDetector> detector = */
	/* 	get_algorithm <cv::FeatureDetector> (arguments.feature_detector); */

	/* vector<KeyPoint> query_kp; */
	/* detector->detect(query, query_kp); */

	/* Ptr<cv::DescriptorExtractor> extractor = */
	/* 	get_algorithm <cv:DescriptorExtractor> (arguments.feature_detector); */

	/* Mat query_d; */
	/* extractor->compute(query, query_kp, query_d); */

	/* static const BFMatcher matcher( */
	/* 		detector_id == ORB ? cv::NORM_HAMMING : cv::NORM_L2 ); */

	return 0;
}
