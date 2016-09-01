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

/* TODO this function is probably not required, its functionaity
 * should perhaps be merged with subspace_clustering, saving at 
 * least an invocation, for subspace_clustering is not used
 * anywhere else */

/* TODO this needs to be further optimized for performance and
 * readability */

/* TODO try to make subspace cluster stacks more randomly distributed */
list<stack<DMatch>> get_match_buckets(

		std::multimap<float, DMatch> cross_match_result, /* x ordered matches that match in
																												both directions: query->train
																												and train->query */

		float max_distance, /* maximum distance a match could be from one on a cluster, 
												 * in regards to x or y, to be considered part of that cluster */

		size_t min_elements, /* minimum of elements a cluster can have to be
														included in the result */

		std::vector<cv::KeyPoint>& train_keypoints, /* for being able to sort coordinates */

		float min_width, /* minimum width a cluster can have to be included in the result */
		float min_height, /* same thing, but in regards to y */

		float width, /* TODO this is just kind of dumb. it is used to calculate bounding box
										coordinates that define the rectangle/square. We should get the
										first element of what we want to iterate and define initial
										bbcoordinates acoording to it, instead of this. */
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
