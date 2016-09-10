#include <argp.h>

const char *argp_program_version = "vs-match 0.1b-rc1",
			*argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Match query and train descriptors to identify if"\
										 " an image contains an object.",

						args_doc[] = "[query-path] [train-path]";

/* enum output_format { */
/* 	OF_PATH, */
/* 	OF_PATHC, */
/* }; */

typedef struct {
	char *query_path,
			 *train_path; 

	/* bool ratio_test, */
	/* 		 cross_match; */
			 /* show_bounds; TODO */

	size_t clustering_min_points,
				 homography_attempts;

	float clustering_max_dist,
				subspace_min_side;

} arguments_t;


static struct argp_option options[] = {
	{ "clustering-max-dist", 'd', "FLOAT", 0,
		"Cluster points that have a distance in pixels below this value are part "
		"of the same cluster (default: 15)", 0 },

	{ "clustering-min-points", 'p', "SIZE_T", 0,
		"A cluster is valid if it has at least this number of points "
		"(default: 10)", 0 },

	{ "subspace-min-size", 's', "FLOAT", 0,
		"A subspace cluster is valid if it has at least this side length "
		"(default: 10)", 0 },

	{ "homography-attempts", 'a', "size_t", 0,
		"How many times should the bucket-points-homography loop repeat? (5)", 0 },

	{0, 0, 0, 0, 0, 0}
};

__inline__ static bool
parse_bool(char *arg)
{
	return arg && arg[0] == '1';
}

#include "stdlib.h"
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *args = (arguments_t*) state->input;

	switch (key) {
		case 'a': args->homography_attempts = (size_t) strtol(arg, NULL, 10); break;

		/* case 'R': args->ratio_test = parse_bool(arg); break; */

		/* case 'c': args->cross_match = parse_bool(arg); break; */

		/* case 'b': args->show_bounds = parse_bool(arg); break; */

		case 'd': args->clustering_max_dist = strtof(arg, NULL); break;
		
		case 's': args->subspace_min_side = strtof(arg, NULL); break;

		case 'p': args->clustering_min_points = (size_t) strtol(arg, NULL, 10); break;

		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;

		case ARGP_KEY_ARG:
			if (!args->query_path) {
				args->query_path = arg;
				args->train_path = state->argv[state->next];
			}
			break;

		default: return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, NULL, 0 };

#include <vector>
#include <opencv2/core.hpp>
#include "debug.h"

static std::vector<cv::KeyPoint>
read_keypoints(FILE *inp)
{
	size_t n;
	fread(&n, sizeof(size_t), 1, inp);
	dprint("rk: %lu", n);

	std::vector<cv::KeyPoint> kp(n);
	fread(&kp[0], sizeof(cv::KeyPoint), n, inp);

	return kp;
}

static cv::Mat
read_descriptors(FILE *inp)
{
	int cols, rows;
	fread(&cols, sizeof(int), 1, inp);
	fread(&rows, sizeof(int), 1, inp);

	int size = cols*rows;
	dprint("rd: %d %d %d", cols, rows, size);

	unsigned char *buffer = (unsigned char*) malloc(size*sizeof(char));

	fread(buffer, sizeof(char), size, inp);

	return cv::Mat(rows, cols, CV_8UC1, buffer);
}

#include "cross_match.hpp"
#include "subspace_clustering.hpp"
#include <algorithm>
#include <stack>
#include <list>

int main(int argc, char **argv) {
	arguments_t args;

	/* args.cross_match = true; */
	/* args.ratio_test = args.show_bounds = false; */
	/* args.norm_type = cv::NORM_L2; */
	args.clustering_min_points = 10U;
	args.clustering_max_dist = 15.0f;
	args.subspace_min_side = 10.0f;
	args.homography_attempts = 5U;
	args.query_path = NULL;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	float min_side = args.subspace_min_side,
				max_dist = args.clustering_max_dist;

	size_t min_points = args.clustering_min_points;

	FILE *query_f = fopen(args.query_path, "rb");
	bool query_hamming;
	fread(&query_hamming, sizeof(bool), 1, query_f);
	std::vector<cv::KeyPoint> query_kp = read_keypoints(query_f);
	cv::Mat query_d = read_descriptors(query_f);
	fclose(query_f);

	FILE *train_f = fopen(args.train_path, "rb");
	bool train_hamming;
	fread(&train_hamming, sizeof(bool), 1, train_f);

	if (query_hamming != train_hamming) {
		fclose(train_f);
		return 1;
	}

	dprint(" %d %d", query_hamming, train_hamming);

	size_t db_n;
	fread(&db_n, sizeof(size_t), 1, train_f);
	dprint("rn: %lu", db_n);

	/* TODO: research advantages of l2 vs l1 */
	static const cv::BFMatcher
		matcher( query_hamming ? cv::NORM_HAMMING : cv::NORM_L1, false );

	for (size_t i = 0; i < db_n; i++) {
		dprint("train #%lu", i);

		std::vector<cv::KeyPoint> kp = read_keypoints(train_f);
		cv::Mat d = read_descriptors(train_f);

		std::list<std::stack<cv::DMatch>>
			buckets = subspace_clustering (
					cross_match(matcher, query_d, d, kp),
					kp, min_side, max_dist, min_points );

		dprint(" %lu buckets", buckets.size());

		double certainty;
		if (buckets.size()) {
			size_t w = 0;

			do {
				dprint(" tw: %lu", w);

				size_t ftsize = 0;

				for (auto it = buckets.begin(); it != buckets.end(); it++)
					ftsize += std::min(min_points, (*it).size());

				dprint(" ftsize: %lu", ftsize);

				if (ftsize < 10) break;

				std::vector<cv::Point2f> from(ftsize), to(ftsize);
				size_t id = 0;

				for (auto it = buckets.begin(); it != buckets.end(); it++) {
					std::stack<cv::DMatch>& s = *it;
					dprint( "  stacksize: %lu", s.size());


					for (size_t k = 0; k < std::min(min_points, s.size()); k++) {
						cv::DMatch match = s.top();
						from[id] = query_kp[match.queryIdx].pt;
						to[id] = kp[match.trainIdx].pt;

						s.pop();
						id++;
					}
				}

				/* TODO vector<unsigned char> inliersMask(ftsize); */
				cv::Mat h = findHomography(from, to, CV_RANSAC, 1);

				{
					double det = h.at<double>(0,0)*h.at<double>(1,1) - \
										 h.at<double>(1,0)*h.at<double>(0,1); 

					certainty = 1 - fabs(0.5 - det) * 2;
					fprintf( stderr, " c%.1f%%", certainty*100 );
#define required_certainty .0

					if (certainty > required_certainty) {
						char c;
						fputs("F ", stdout);

						while ((c = (char)fgetc(train_f)))
							putchar(c);

						putchar('\n');

						goto skip_skip_path;
					}
				}

				w++;
			} while (w < args.homography_attempts);

			while (fgetc(train_f));
			continue;

skip_skip_path:
			;
		}

	}

	fclose(train_f);

	return 0;
}
