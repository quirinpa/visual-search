#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>
using cv::DMatch;
using cv::KeyPoint;

#include <map>
using std::multimap;
using std::pair;

#include <functional>
using std::function;
	
static void
subcluster_cluster_orderby_y (
	multimap<float, DMatch> parent,
	const float min_distance,
	const size_t min_elements,
	const vector<KeyPoint>& train_kps,
	function<void (multimap<float, DMatch>&)> cb )
{
	multimap<float, DMatch> subcluster;
	float last_key = 0;

	for (auto parent_it = parent.begin();
			parent_it != parent.end(); ++parent_it)
	{
		float key = parent_it->first;

		if (key - last_key > min_distance) {

			if (subcluster.size() >= min_elements) cb(subcluster);

			subcluster.clear();
		}

		DMatch match = parent_it->second;

		subcluster.insert(pair<float, DMatch>(train_kps[match.trainIdx].pt.y, match)); 
		last_key = key;
	}

	if (subcluster.size() > min_elements) cb(subcluster);
}

#include <stack>
using std::stack;

#include "subspace_clustering.hpp"
#include <stdio.h>

stack<multimap<float, DMatch>>
subspace_clustering (
		const multimap<float, DMatch> x_megacluster,
		const float min_distance,
		const size_t min_elements,
		const vector<KeyPoint>& train_kps )
{
	stack<multimap<float, DMatch>> result;

	subcluster_cluster_orderby_y(x_megacluster, min_distance, min_elements,
			train_kps, [&](const multimap<float, DMatch>& x_subcluster) {
				subcluster_cluster_orderby_y(x_subcluster, min_distance, min_elements,
						train_kps, [&](const multimap<float, DMatch>& y_subcluster) {
							result.push(y_subcluster);
						});
			});

	fprintf(stderr, "subspace clusters: %u\n", result.size());

	return result;
}
