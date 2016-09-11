#include <vector>
#include <opencv2/opencv.hpp>
#include <functional>
__inline__ static void
MATCH(
		const cv::DescriptorMatcher& matcher,
		cv::Mat& d1, cv::Mat& d2,
		std::function<void (const cv::DMatch&)> process_match)
{
#if 0
	vector<vector<DMatch>> knnmatches;
	matcher.knnMatch(d1, d2, knnmatches);

	for(vector<vector<DMatch>>::iterator it = knnmatches.begin();
			it != knnmatches.end(); it++) {

		vector<DMatch> top_matches = *it;
		DMatch best_match = top_matches[0];

		if (top_matches.size() == 1 ||
				best_match.distance <= .8f *
					top_matches[1].distance)

			process_match (best_match);

	}
#else
	std::vector<cv::DMatch> temp; 
	matcher.match(d1, d2, temp);

	for(std::vector<cv::DMatch>::iterator it = temp.begin();
			it != temp.end(); it++)

		process_match (*it);

#endif
}

#include <unordered_map>
struct hash_crossmatch {
	size_t operator()(const std::pair<int, int> &k) const {

		size_t h1 = std::hash<int>()(k.first),
					 h2 = std::hash<int>()(k.second);

		return h1^h2;
	}
};

struct equals_crossmatch {
	bool operator()(
			const std::pair<int, int>& lhs, 
			const std::pair<int, int>& rhs) const
	{
		return (lhs.first == rhs.first) && (lhs.second == rhs.second);
	}
};

#include <map>
#include <stack>
#include "cross_match.hpp"
#include "debug.h"
std::multimap<float, cv::DMatch>
cross_match(
		const cv::DescriptorMatcher& matcher,
		cv::Mat& d1, cv::Mat& d2, std::vector<cv::KeyPoint>& train_kp )
{
	std::unordered_map<
		std::pair<int, int>,
		std::stack<cv::DMatch>,
		hash_crossmatch,
		equals_crossmatch> normal_map;

	MATCH (matcher, d1, d2, [&](const cv::DMatch& match) {
		normal_map[std::pair<int, int>(match.trainIdx, match.queryIdx)].push(match);
	});

	std::multimap<float, cv::DMatch> result;

	MATCH (matcher, d2, d1, [&](const cv::DMatch& match) {
		register std::stack<cv::DMatch>& s =
			normal_map[std::pair<int, int>(match.queryIdx, match.trainIdx)];

		while (!s.empty()) {
			result.insert( std::pair<float, cv::DMatch>
				(train_kp[match.queryIdx].pt.x, s.top()) );

			s.pop();
		}

	});

	dprint("cross-checked: %lu", result.size());
	return result;
}
