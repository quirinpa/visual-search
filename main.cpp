#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp> // needed for FAST
#include <opencv2/xfeatures2d.hpp> // needed for SIFT / SURF

using namespace std;
using namespace cv;

#define use_flann false
#define use_cross_match false

#if use_flann
static const DescriptorMatcher *matcher = new FlannBasedMatcher(new flann::LshIndexParams(5, 24, 2));
#else
static const DescriptorMatcher *matcher = new BFMatcher(
		/* NORM_HAMMING */
		NORM_L2
		, use_cross_match
		);
#endif

#define use_knn false
#define knn_ratio 0.8f

#include <unordered_map>
template<typename T>
__inline__ static void
find_and_add(
		unordered_map<int, T>& m,
		const DMatch& match,
		void (*add_match) (T&, const DMatch&) )
{
	const int trainIdx = match.trainIdx;
	typename unordered_map<int, T>::iterator
		search = m.find(trainIdx);

	T subdata;

	if (search != m.end())
		subdata = search->second;
	else
		m[trainIdx] = subdata;

	add_match(subdata, match);
}

template<typename T>
__inline__ static unordered_map<int, T>
MATCH(Mat d1, Mat d2, void (*add_match) (T&, const DMatch&) )
{
	unordered_map<int, T> result;

#if use_knn

	vector<vector<DMatch>> knnmatches;
	matcher->knnMatch(d1, d2);

	for(vector<vector<DMatch>>::iterator it = knnmatches.begin();
			it != knnmatches.end(); it++) {

		vector<DMatch> top_matches = *it;
		DMatch best_match = top_matches[0];

		if (top_matches.size() == 1 ||
				best_match.distance <= knn_ratio *
					top_matches[1].distance)

			find_and_add <T> (result, best_match, add_match);

	}

#else

	vector<DMatch> temp; 
	matcher->match(d1, d2, temp);

	for(vector<DMatch>::iterator it = temp.begin();
			it != temp.end(); it++)

		find_and_add <T> (result, *it, add_match);

#endif

	fprintf(stderr, " %u matches.\n", result.size());
	return result;
}

#include <stack>
#include <map>
__inline__
static multimap<float, DMatch>
cross_match(Mat d1, Mat d2, vector<KeyPoint> train_kp) {

	/* get normal map */
	fputs("normal", stderr);
	unordered_map<int, stack<DMatch>> normal_map =
		MATCH <stack<DMatch>> (d1, d2,
			[](stack<DMatch>& s, const DMatch& match) {
				s.push(match);
				/* fprintf(stderr, "%u, %u/", match.trainIdx, match.queryIdx); */
			});

	multimap<float, DMatch> result;

	/* get reverse map */
	fputs("reverse", stderr);
	unordered_map<int, unordered_map<int, DMatch>> reverse_map =
		MATCH <unordered_map<int, DMatch>> (d2, d1,
			[](unordered_map<int, DMatch>& m, const DMatch& match) {
				m[match.queryIdx] = match;
				/* fprintf(stderr, "%u, %u/", match.trainIdx, match.queryIdx); */
			});


	/* return multimap of cross-matches */
	for(auto normal_it = normal_map.begin();
			normal_it != normal_map.end();
			++normal_it) {

		int normal_trainIdx = normal_it->first;

		stack<DMatch> normal_stack = normal_it->second;

		while (!normal_stack.empty()) {
			DMatch match = normal_stack.top();
			normal_stack.pop();

			unordered_map<int, unordered_map<int, DMatch>>::iterator
				search = reverse_map.find(match.queryIdx);

			if (search == reverse_map.end()) continue;

			{
				unordered_map<int, DMatch> reverse_trainIdx_map = search->second;
				if (reverse_trainIdx_map.find(normal_trainIdx) == reverse_trainIdx_map.end())
					continue;
			}

			result.insert(pair<float, DMatch>(train_kp[match.queryIdx].pt.x, match));
		}
	}

	fprintf(stderr, "cross-checked matches: %u\n", result.size());
	return result;
}


template<typename SUBCLUSTER_CALLBACK>
static void
subcluster_cluster_orderby_y (
	multimap<float, DMatch> parent,
	const float min_distance,
	const size_t min_elements,
	const vector<KeyPoint>& train_kps,
	SUBCLUSTER_CALLBACK cb )
{
	multimap<float, DMatch> subcluster;
	float last_key = 0;

	for (multimap<float, DMatch>::iterator parent_it = parent.begin();
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

__inline__
static stack<multimap<float, DMatch>>
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

	return result;
}

int main(void) {
	Mat query = imread("resources/ss.png");
	Ptr<FeatureDetector> detector = ORB::create();

	vector<KeyPoint> query_kp;
	detector->detect(query, query_kp);

	Ptr<DescriptorExtractor> extractor = ORB::create();

	Mat query_d;
	extractor->compute(query, query_kp, query_d);

	VideoCapture cap(0);
	if(!cap.isOpened()) return -1;

	namedWindow("d", CV_WINDOW_NORMAL);

	unsigned n = 0;
	clock_t times = 0;

	do {
		Mat cframe;
		cap >> cframe;

		vector<KeyPoint> kp;
		Mat d;

		clock_t start = clock();

		detector->detect(cframe, kp);

		if (!kp.size()) {
			imshow("d", cframe);
			continue;
		}

		extractor->compute(cframe, kp, d);

		times += clock() - start;
		n++;

		stack<multimap<float, DMatch>> clusters =
			subspace_clustering(cross_match(query_d, d, kp),
					.03f * (float) cframe.cols, 10, kp);

		while (!clusters.empty()) {
			multimap<float, DMatch> cluster = clusters.top();
			clusters.pop();

			float ymin = cluster.begin()->first,
						ymax = cluster.end()->first;

			if (ymax - ymin < 3.0f) continue;

			float xmin = (float) cframe.cols, xmax = 0;

			for (multimap<float, DMatch>::iterator map_it = cluster.begin();
					map_it != cluster.end();
					++map_it)
			{
				DMatch match = map_it->second;
				Point2f cluster_pt = kp[match.trainIdx].pt;

				line(cframe, cluster_pt, cluster_pt, Scalar(255, 255, 0), 2);

				{
					float x = cluster_pt.x;
					if (x < xmin) xmin = x;
					if (x > xmax) xmax = x;
				}
			}

			if (xmax - xmin < 3.0f) continue;

			rectangle(cframe, Point2f(xmin, ymin), Point2f(xmax, ymax), Scalar(255, 0, 0), 1);

			{
				ostringstream convert;
				convert << cluster.size();
				putText(cframe, convert.str(), Point2f(xmax + 5, ymin), FONT_HERSHEY_PLAIN, .9, Scalar(0, 0, 255));
			}

		}

		imshow("d", cframe);

	} while (waitKey(1)!='\x1b');

	fprintf(stderr, "Avg time: %fs\n", (double)times/((double)n*CLOCKS_PER_SEC));

	return 0;
}
