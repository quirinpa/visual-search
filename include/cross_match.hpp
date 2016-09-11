#ifndef CROSS_MATCH_H
#define CROSS_MATCH_H

#include <opencv2/opencv.hpp>
#include <map>

/* finds matches that match in both directions. generating
 * a multimap, oriented by the match's keypoint x coordinate,
 * for convenience, in regards to the data format that is
 * expected by subspace_clustering. */
std::multimap<float, cv::DMatch>
cross_match(
		const cv::DescriptorMatcher& matcher,
		cv::Mat& query_descriptors,
		cv::Mat& train_descriptors,
		std::vector<cv::KeyPoint>& train_keypoints );

#endif
