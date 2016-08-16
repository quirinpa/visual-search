#ifndef SUBSPACE_CLUSTERING_H
#define SUBSPACE_CLUSTERING_H

#include <opencv2/opencv.hpp>
#include <functional>
#include <map>

void
subspace_clustering (
		std::multimap<float, cv::DMatch> x_megacluster,
		float max_distance,
		size_t min_elements,
		std::vector<cv::KeyPoint>& train_kps,
		std::function<void (cv::DMatch&)> insert,
		std::function<void (void)> save,
		std::function<void (void)> cleanup );

#endif
