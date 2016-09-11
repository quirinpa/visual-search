#include <map>
#include <opencv2/opencv.hpp>
#include <functional>
#include "debug.h"

__inline__ static void
divide_subspace(
	std::multimap<float, cv::DMatch>& parent,
	float min_dimension,
	float max_distance,
	size_t min_elements,
	std::function<void(cv::DMatch&)> insert,
	std::function<void(void)> save,
	std::function<void(void)> cleanup )
{
	auto parent_it = parent.begin();

	while (parent_it != parent.end()) {
		size_t size = 1;

		float last_coord = parent_it->first,
					min_coord = last_coord;

		/* fprintf( stderr, "%.0f", (double) last_coord ); */
		insert( parent_it->second );

		parent_it++;

		while (parent_it != parent.end()) {
			register float coord = parent_it->first;

			/* fprintf( stderr, "%.0f", (double) coord ); */
			if (coord - last_coord > max_distance )
				break;

			insert( parent_it->second );
			last_coord = coord;

			parent_it++;
			size++;
		}

		if (size > min_elements && last_coord - min_coord >= min_dimension)
			save();

		cleanup();
	}
}

#include <list>
#include <stack>
#include <vector>

static std::stack<cv::DMatch> empty_match_bucket;

#include "subspace_clustering.hpp"
std::list<std::stack<cv::DMatch>>
subspace_clustering(
		std::multimap<float, cv::DMatch> parent,
		std::vector<cv::KeyPoint>& train_keypoints,
		float min_dimension,
		float max_distance,
		size_t min_elements )
{
	std::list<std::stack<cv::DMatch>> result;
	std::multimap<float, cv::DMatch> y_subcluster;
	std::stack<cv::DMatch> x_subcluster;

	divide_subspace(parent, min_dimension, max_distance, min_elements,
		[&](cv::DMatch& match) {
			y_subcluster.insert( std::pair<float, cv::DMatch>
					(train_keypoints[match.trainIdx].pt.y, match) );
			/* dputs("iy"); */
		}, [&](void) {
			/* dputs("sy"); */
			divide_subspace(y_subcluster, min_dimension, max_distance, min_elements,
				[&](cv::DMatch& match) {
					x_subcluster.push(match); 
					/* dputs("ix"); */
				}, [&](void) {
					result.push_back(x_subcluster); 
					/* dputs("sx"); */
				}, [&](void) {
					/* std::swap(x_subcluster, empty_match_bucket); */
					x_subcluster = std::stack<cv::DMatch>();
					/* dputs("cx"); */
				});
		}, [&](void) {
			y_subcluster.clear();
			/* dputs("cy"); */
			/* fputs("cy", stderr); */
		});

	/* dprint("subspace clusters: %lu", result.size()); */

	return result;
}
