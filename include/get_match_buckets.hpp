#ifndef GET_MATCH_BUCKETS_H
#define GET_MATCH_BUCKETS_H

#include <list>
#include <stack>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

std::list<std::stack<cv::DMatch>> get_match_buckets(
		std::multimap<float, cv::DMatch> x_megacluster,
		float max_distance,
		size_t min_elements,
		std::vector<cv::KeyPoint>& train_kps,
		float min_width,
		float min_height,
		float width,
		float height );

#endif
