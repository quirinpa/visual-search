#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>
using cv::Mat;
using cv::DMatch;
using cv::KeyPoint;
using cv::DescriptorMatcher;

#include <functional>
using std::function;

#define use_knn false
#define knn_ratio 0.8f

__inline__ static void
MATCH(
		const DescriptorMatcher& matcher,
		Mat d1, Mat d2,
		function<void (const DMatch&)> process_match)
{
#if use_knn

	vector<vector<DMatch>> knnmatches;
	matcher.knnMatch(d1, d2);

	for(vector<vector<DMatch>>::iterator it = knnmatches.begin();
			it != knnmatches.end(); it++) {

		vector<DMatch> top_matches = *it;
		DMatch best_match = top_matches[0];

		if (top_matches.size() == 1 ||
				best_match.distance <= knn_ratio *
					top_matches[1].distance)

			process_match (best_match);

	}

#else

	vector<DMatch> temp; 
	matcher.match(d1, d2, temp);

	for(vector<DMatch>::iterator it = temp.begin();
			it != temp.end(); it++)

		process_match (*it);

#endif
}

#include <unordered_map>
using std::unordered_map;

#include <stack>
using std::stack;

#include <map>
using std::multimap;
using std::pair;

#include "cross_match.hpp"
#include <stdio.h>

multimap<float, DMatch>
cross_match(
		const DescriptorMatcher& matcher,
		Mat d1, Mat d2,
		vector<KeyPoint> train_kp )
{
	unordered_map<int, unordered_map<int, stack<DMatch>>> normal_map;

	MATCH (matcher, d1, d2, [&](const DMatch& match) {
		normal_map[match.trainIdx][match.queryIdx].push(match);
	});

	multimap<float, DMatch> result;

	MATCH (matcher, d2, d1, [&](const DMatch& match) {
		auto search = normal_map.find(match.queryIdx);
		if (search == normal_map.end()) return;

		auto sub_map = search->second;
		auto search_sub = sub_map.find(match.trainIdx);
		if (search_sub == sub_map.end()) return;

		stack<DMatch>& s = search_sub->second;

		while (!s.empty()) {
			result.insert( pair<float, DMatch>
				(train_kp[match.queryIdx].pt.x, s.top()) );

			s.pop();
		}

	});

	fprintf(stderr, "cross-checked matches: %u\n", result.size());
	return result;
}
