#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>
using cv::KeyPoint;

#include "stdio.h"

static
vector<KeyPoint> read_keypoints(FILE *inp)
{
	size_t n;
	fread(&n, sizeof(size_t), 1, inp);
	/* fprintf(stderr, "rk: %u\n", n); */

	vector<KeyPoint> kp;
	for (size_t i = 0; i < n; i++) {
		KeyPoint *pt = new KeyPoint();
		fread(pt, sizeof(KeyPoint), 1, inp);
		kp.push_back(*pt);
	} /* TODO cleanup */

	return kp;
}

using cv::Mat;

static
Mat read_descriptors(FILE *inp)
{
	int cols, rows;
	fread(&cols, sizeof(int), 1, inp);
	fread(&rows, sizeof(int), 1, inp);

	int size = cols*rows;
	/* fprintf(stderr, "rd: %d %d %d\n", cols, rows, size); */
	unsigned char *buffer = (unsigned char*) malloc(size*sizeof(char));

	fread(buffer, sizeof(char), size, inp);
	/* fprintf(stderr, "%s\n", buffer); */

	return Mat(rows, cols, CV_8UC1, buffer);
}

#include <argp.h>

const char *argp_program_version = "vs-match 0.1a",
						 *argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Match query and train descriptors to identify if"\
										 " an image contains an object.",

						args_doc[] = "[query-path] [train-path]";

/* #include <stdbool.h> */
typedef struct {
	char *query_path,
			 *train_path; 

	enum cv::NormTypes norm_type;

	bool ratio_test,
			 cross_match,
			 show_bounds;

	size_t clustering_min_points,
				 homography_attempts;

	float clustering_max_dist,
				subspace_min_width,
				subspace_min_height;

} arguments_t;

static struct argp_option options[] = {
	{"ratio-test", 'R', "bool", 0, "Enable David Lowe's ratio test, KNN k=2. (0)", 0},
	{"cross-match", 'c', "bool", 0, "Enable cross-matching (1)", 0},
	{"show-bounds", 'b', "bool", 0, "Output rectangle coordinates. (0)", 0},
	{"norm-type", 'n', "enum cv::NormTypes", 0, "Norm Type for matcher (3: NORM_L2)", 0},
	{"clustering-max-dist", 'd', "float", 0, "Cluster points that have distances below this value as being part of the same cluster (5.0f)", 0},
	{"clustering-min-points", 'p', "size_t", 0, "Minimum number of points in a cluster (10U)", 0},
	{"subspace-min-width", 'w', "float", 0, "Minimum width of a valid subspace cluster (10.0f)", 0},
	{"subspace-min-height", 'h', "float", 0, "Minimum height of a valid subspace cluster (10.0f)", 0},
	{"homography-attempts", 'a', "size_t", 0, "How many times should the bucket-points-homography loop repeat? (3)", 0},
	{0, 0, 0, 0, 0, 0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = (arguments_t*) state->input;

	switch (key) {
		case 'a':
			arguments->homography_attempts = (size_t) strtol(arg, NULL, 10);
			break;

		case 'R':
			arguments->ratio_test = arg && arg[0] == '1';
			break;

		case 'c':
			arguments->cross_match = arg && arg[0] == '1';
			break;

		case 'b':
			arguments->show_bounds = arg && arg[0] == '1';
			break;

		case 'n':
			arguments->norm_type = (cv::NormTypes) strtol(arg, NULL, 10);
			break;

		case 'd':
			arguments->clustering_max_dist = strtof(arg, NULL);
			break;
		
		case 'w':
			arguments->subspace_min_width = strtof(arg, NULL);
			break;

		case 'h':
			arguments->subspace_min_height = strtof(arg, NULL);
			break;

		case 'p':
			arguments->clustering_min_points = (size_t) strtol(arg, NULL, 10);
			break;

		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;

		case ARGP_KEY_ARG:
			if (!arguments->query_path) {
				arguments->query_path = arg;
				arguments->train_path = state->argv[state->next];
			}
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, NULL, 0 };

__inline__ static arguments_t
get_arguments(int argc, char **argv)
{
	arguments_t args;
	args.cross_match = true;
	args.ratio_test = false;
	args.show_bounds = false;
	args.norm_type = cv::NORM_L2;
	args.clustering_min_points = 10U;
	args.clustering_max_dist = 5.0f;
	args.subspace_min_height = args.subspace_min_width = 10.0f;
	args.homography_attempts = 3U;
	args.query_path = NULL;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	return args;
} 
using cv::DMatch;

#include "cross_match.hpp"
#include "get_match_buckets.hpp"

#include <stack>
using std::stack;
#include <list>
using std::list;

#include <algorithm>
using std::min;
using std::max;

int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);

	float min_width = args.subspace_min_width,
				min_height = args.subspace_min_height,
				max_dist = args.clustering_max_dist;

	size_t min_points = args.clustering_min_points;

	static const cv::BFMatcher matcher( args.norm_type, false );

	/* vector<KeyPoint> query_kp = read_keypoints(); */
	FILE *query_f = fopen(args.query_path, "rb");
	/* fseek(query_f, sizeof(size_t) << 1, SEEK_SET); */
	vector<KeyPoint> query_kp = read_keypoints(query_f);
	Mat query_d = read_descriptors(query_f);
	fclose(query_f);

	FILE *train_f = fopen(args.train_path, "rb");
	size_t db_n;
	fread(&db_n, sizeof(size_t), 1, train_f);
	/* fprintf(stderr, "rn: %u\n", db_n); */

	for (size_t i = 0; i < db_n; i++) {
		/* fprintf(stderr, "train #%u\n", i); */
		int width, height;
		fread(&width, sizeof(int), 1, train_f);
		fread(&height, sizeof(int), 1, train_f);
	
		vector<KeyPoint> kp = read_keypoints(train_f);
		Mat d = read_descriptors(train_f);

		list<stack<DMatch>> match_buckets = get_match_buckets(
				cross_match(matcher, query_d, d, kp),
				max_dist, min_points, kp, min_width, min_height,
				(float) width + .5f,
				(float) height + .5f );

		/* fprintf(stderr, "\t%u buckets.\n", match_buckets.size()); */

		bool not_found;
		size_t w = 0;
		do {
			/* fprintf(stderr, "\tw: %u\n", w); */

			size_t ftsize = 0;

			for (auto it = match_buckets.begin(); it != match_buckets.end(); it++)
				ftsize += min(min_points, (*it).size());

			/* fprintf(stderr, "\tftsize: %u\n", ftsize); */

			if (ftsize < 10) break;

			vector<cv::Point2f> from(ftsize), to(ftsize);
			size_t id = 0;

			for (auto it = match_buckets.begin(); it != match_buckets.end(); it++) {
				stack<DMatch> s = *it;

				for (size_t k = 0; k < min(min_points, s.size()); k++) {
					DMatch match = s.top();
					from[id] = query_kp[match.queryIdx].pt;
					to[id] = kp[match.trainIdx].pt;

					s.pop();
					id++;
				}
			}

			/* vector<unsigned char> inliersMask(ftsize); */
			/* Mat h = findHomography(from, to, cv::CV_RANSAC, 1, inliersMask); */
			Mat h = findHomography(from, to, CV_RANSAC, 1);

			const double det = h.at<double>(0,0)*h.at<double>(1,1) - \
												 h.at<double>(1,0)*h.at<double>(0,1); 

			/* fprintf(stderr, "\tdet: %f\n", det); */
			not_found = det < 0 || fabs(det) > 1;
			w++;

		} while (not_found && w < args.homography_attempts);

		if (not_found) while (fgetc(train_f));
		else {
			char c;

			while ((c = (char)fgetc(train_f))) putchar(c);
			putchar('\n');
		}

	}

	fclose(train_f);

	return 0;
}
