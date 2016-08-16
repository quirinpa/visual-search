#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>
using cv::KeyPoint;

#include "stdio.h"

static
vector<KeyPoint> read_keypoints(FILE *inp)
{
	size_t n;
	fread(&n, 1, sizeof(size_t), inp);

	vector<KeyPoint> kp(n);
	for (size_t i = 0; i < n; i++)
		fread(&kp[i], 1, sizeof(KeyPoint), inp);

	return kp;
}

using cv::Mat;

static
Mat read_descriptors(FILE *inp)
{
	size_t cols, rows;
	fread(&cols, 1, sizeof(size_t), inp);
	fread(&rows, 1, sizeof(size_t), inp);
	Mat descriptors(cols, rows, CV_64FC1);

	for (size_t x = 0; x < cols; x++)
		for (size_t y = 0; y < rows; y++)
			fread(&descriptors.at<double>(x, y), 1, sizeof(double), inp);

	return descriptors;
}

#include <argp.h>

const char *argp_program_version = "vs-match 0.1a",
						 *argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Match query and train descriptors to identify if"\
										 " an image contains an object.",

						args_doc[] = "[query-path] [train-path]";

/* #include <stdbool.h> */
typedef struct {
	char *query_filename,
			 **train_filenames; 

	size_t train_files;

	enum cv::NormTypes norm_type;

	bool ratio_test,
			 cross_match,
			 show_bounds;

	size_t clustering_min_points;

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
	{0, 0, 0, 0, 0, 0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = (arguments_t*) state->input;

	switch (key) {
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
			arguments->query_filename = arg;
			{
				register size_t i = state->next;
				arguments->train_files = state->argc - i;
				arguments->train_filenames = &state->argv[i];
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
	args.train_files = 0;
	args.cross_match = true;
	args.ratio_test = false;
	args.show_bounds = false;
	args.norm_type = cv::NORM_L2;
	args.clustering_min_points = 10U;
	args.clustering_max_dist = 5.0f;
	args.subspace_min_height = args.subspace_min_width = 10.0f;

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

int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);

	static const cv::BFMatcher matcher( args.norm_type, false );

	/* vector<KeyPoint> query_kp = read_keypoints(); */
	FILE *query_f = fopen(args.query_filename, "rb");
	Mat query_d = read_descriptors(query_f);
	fclose(query_f);

	for (size_t i = 0; i < args.train_files; i++) {
		FILE *train_f = fopen(args.train_filenames[i], "rb");

		size_t c;
		fread(&c, 1, sizeof(size_t), train_f);

		for (size_t j = 0; j < c; j++) {
			float width, height;
			fread(&width, 1, sizeof(float), train_f);
			fread(&height, 1, sizeof(float), train_f);

			vector<KeyPoint> kp = read_keypoints(train_f);
			Mat d = read_descriptors(train_f);

			list<stack<DMatch>> match_buckets = get_match_buckets(
					cross_match(matcher, query_d, d, kp),
					args.clustering_max_dist, args.clustering_min_points, kp,
					args.subspace_min_width, args.subspace_min_height, width, height );

		}

		fclose(train_f);
	}

	return 0;
}
