#ifndef CROSS_MATCH_H
#define CROSS_MATCH_H

#include <opencv2/opencv.hpp>
#include <map>

std::multimap<float, cv::DMatch>
cross_match(
		const cv::DescriptorMatcher&,
		cv::Mat, cv::Mat,
		std::vector<cv::KeyPoint> );

#endif
