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
subcluster (
	multimap<float, DMatch> parent,
	float max_distance,
	size_t min_elements,
	function<void (DMatch&)> insert,
	function<void (void)> save,
 	function<void (void)> cleanup	)
{
	/* multimap<float, DMatch> subcluster; */
	size_t size = 0;
	float last_key = 0;

	for (auto parent_it = parent.begin();
			parent_it != parent.end(); ++parent_it)
	{
		float key = parent_it->first;

		if (key - last_key > max_distance) {

			if (size >= min_elements) save();

			cleanup();
			size = 0;
		}

		DMatch match = parent_it->second;

		insert(match);
		size++;
		last_key = key;
	}

	if (size > min_elements) save();
}

#include "subspace_clustering.hpp"

void subspace_clustering (
		multimap<float, DMatch> x_megacluster,
		float max_distance,
		size_t min_elements,
		vector<KeyPoint>& train_kps,
		function<void (DMatch&)> insert,
		function<void (void)> save,
	 	function<void (void)> cleanup	)
{
	multimap<float, DMatch> y_cluster;

	cleanup();

	subcluster(x_megacluster, max_distance, min_elements,
			[&](DMatch& match) {
				y_cluster.insert( pair <float, DMatch>
					(train_kps[match.trainIdx].pt.y, match) );
			}, [&]() {
				subcluster(y_cluster, max_distance,
						min_elements, insert, save, cleanup);
			}, [&]() {
				y_cluster.clear();
			});
}
