#include "subspace_clustering.hpp"
#include "get_match_buckets.hpp"

#include <map>
#include <vector>

#include <opencv2/opencv.hpp>
using cv::DMatch;
#include <list>
using std::list;
#include <stack>
using std::stack;

#include <algorithm>
using std::min;
using std::max;

list<stack<DMatch>> get_match_buckets(
		std::multimap<float, DMatch> x_megacluster,
		float max_distance,
		size_t min_elements,
		std::vector<cv::KeyPoint>& train_kps,
		float min_width,
		float min_height,
		float width,
		float height )
{
	float minx = width,
				miny = height,
				maxx = 0,
				maxy = 0;

	size_t size = 0;

	stack<DMatch> curr_cluster;
	list<stack<DMatch>> result;

	subspace_clustering(x_megacluster, max_distance, min_elements, train_kps,
			[&](DMatch& match) {
				cv::Point2f pt = train_kps[match.trainIdx].pt;

				{
					float x = pt.x, y = pt.y;

					minx = min(x, minx);
					miny = min(y, miny);
					maxx = max(x, maxx);
					maxy = max(y, maxy);
				}

				size++;

				curr_cluster.push(match);

			}, [&]() {
				if (maxx - minx >= min_width &&
						maxy - miny >= min_height &&
						size >= min_elements) 

					result.push_back(curr_cluster);

				else

					curr_cluster = stack<DMatch>();

			}, [&]() {
				maxx = maxy = .0f;
				size = 0U;
				minx = width;
				miny = height;
			});

	return result;
}
