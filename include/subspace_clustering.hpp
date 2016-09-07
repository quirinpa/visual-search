#ifndef SUBSPACE_CLUSTERING_H
#define SUBSPACE_CLUSTERING_H

#include <stack>
#include <map>
#include <vector>
#include <list>
#include <opencv2/opencv.hpp>

std::list<std::stack<cv::DMatch>>
subspace_clustering(
		std::multimap<float, cv::DMatch> parent, // ordered by x, coming from cross_match
		std::vector<cv::KeyPoint>& train_keypoints,
		float min_dimension,
		float max_distance,
		size_t min_elements );

#endif
