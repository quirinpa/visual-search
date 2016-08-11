#ifndef SUBSPACE_CLUSTERING_H
#define SUBSPACE_CLUSTERING_H

#include <opencv2/opencv.hpp>
#include <functional>
#include <map>

void
subspace_clustering (
		const std::multimap<float, cv::DMatch> x_megacluster,
		const float min_distance,
		const size_t min_elements,
		const std::vector<cv::KeyPoint>& train_kps,
		std::function<void (cv::DMatch&)> insert,
		std::function<void (void)> save,
		std::function<void (void)> cleanup );

#endif
