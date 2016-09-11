#include <argp.h>

const char *argp_program_version = "vs-match 0.1b-rc2",
			*argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Match query and train descriptors to identify if an image "\
										 "contains an object.",

						args_doc[] = "[query-path] [train-path]";

typedef struct {
	char *query_path,
			 *train_path; 

	/* bool show_bounds; /1* TODO *1/ */

	size_t clustering_min_points,
				 homography_attempts;

	float clustering_max_dist,
				subspace_min_side,
				minimum_certainty;

} arguments_t;


static struct argp_option options[] = {
	/* TODO */
	/* { "bf-matcher", 'b', "BOOL", 0, "Use this options to use a brute-force matcher", 0 }, */

	{ "clustering-max-dist", 'd', "FLOAT", 0, "Cluster points that have a distance in "
		"pixels below this value are part of the same cluster (default: 15)", 0 },

	{ "clustering-min-points", 'p', "SIZE_T", 0, "A cluster is valid if it has at least "
		"this number of points (default: 10)", 0 },

	{ "subspace-min-size", 's', "FLOAT", 0, "A subspace cluster is valid if it has at "
		"least this side length (default: 10)", 0 },

	{ "homography-attempts", 'a', "size_t", 0, "How many times should the algorithm "
		"project the homography of samples from each cluster", 0 },

	{ "certainty-minimum", 'c', "FLOAT", 0, "The minimum value of certainty that the "
		"algorithm accepts as a match (.2f)", 0 },

	{0, 0, 0, 0, 0, 0}
};

#include "stdlib.h"
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *args = (arguments_t*) state->input;

	switch (key) {
		case 'a': args->homography_attempts = (size_t) strtol(arg, NULL, 10); break;

		case 'c': args->minimum_certainty = strtof(arg, NULL); break;

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

#include <unistd.h>
static std::vector<cv::KeyPoint>
read_keypoints( int fd )
{
	size_t n_keypoints;
	if (read(fd, (void*) &n_keypoints, sizeof(size_t)) < 0) {
		perror("Couldn't read # of Keypoints");
		exit(1);
	}
	dprint("Read # of Keypoints (%lu)", n_keypoints);

	std::vector<cv::KeyPoint> kps(n_keypoints);
	size_t keypoint_bytes = sizeof(cv::KeyPoint) * n_keypoints;

	if (read(fd, (void*) &kps[0], keypoint_bytes) < 0) {
		perror("Couldn't read Keypoint Vector");
		exit(1);
	}

	dprint("Read Keypoint Vector (%lu bytes)", keypoint_bytes + sizeof(size_t));

	return kps;
}

static cv::Mat
read_descriptors(
	int fd,
	int descriptor_type,
	register size_t descriptor_bytes	)
{
	int dim[2];

	if (read(fd, (void*) dim, sizeof(dim)) < 0) {
		perror("Couldn't read Descriptor Mat Dimensions");
		exit(1);
	}
	dprint("Read Descriptor Mat Dimensions (%dx%d)", dim[0], dim[1]);

	size_t all_descriptor_bytes = descriptor_bytes * dim[0] * dim[1];
	void *buffer = malloc(all_descriptor_bytes);

	if (read(fd, buffer, all_descriptor_bytes) < 0) {
		perror("Couldn't read Descriptor Mat");
		exit(1);
	}

	dprint("Read Descriptor Mat (%lu bytes)", all_descriptor_bytes + sizeof(dim));

	return cv::Mat(dim[0], dim[1], descriptor_type, buffer);
	/* don't forget to free(buffer) or free(mat.data) */
}

#include "cross_match.hpp"
#include "subspace_clustering.hpp"
#include <algorithm>
#include <stack>
#include <list>

#include <fcntl.h>
int main(int argc, char **argv) {
	arguments_t args;

	/* default argument values */
	args.clustering_min_points = 10U;
	args.clustering_max_dist = 15.0f;
	args.subspace_min_side = 10.0f;
	args.homography_attempts = 5U;
	args.query_path = NULL;
	args.minimum_certainty = .2f;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	/* cache common args */
	const float min_side = args.subspace_min_side,
				max_dist = args.clustering_max_dist,
				minimum_certainty = args.minimum_certainty;

	const size_t min_points = args.clustering_min_points,
				homography_attempts = args.homography_attempts;

	int READ_FLAGS = O_RDONLY | O_NOCTTY | O_NDELAY;

	/* read query file and extract relevant info */
	register int query_fd = open( args.query_path, READ_FLAGS );
	if (query_fd == -1) {
		perror("Error opening query file");
		return -1;
	}

	bool query_hamming;
	if (read(query_fd, (void*) &query_hamming, sizeof(bool)) < 0) {
		perror("Couldn't read query_hamming");
		return 1;
	}
	dprint("Read query Hamming Status (%d, %lu bytes)", query_hamming, sizeof(bool));

	const int descriptor_type = query_hamming ? 0 : 5; /* CV_8UC1 : CV_32UC1 */
	const size_t descriptor_bytes = query_hamming ? sizeof(char) : sizeof(int);
	dprint("Inferred Descriptor Type %d", descriptor_type);

	std::vector<cv::KeyPoint> query_kp = read_keypoints(query_fd);
	cv::Mat query_d = read_descriptors(query_fd, descriptor_type, descriptor_bytes);

	close(query_fd);

	/* extract initial train file info */
	int train_fd = open( args.train_path, READ_FLAGS );
	if (train_fd == -1) {
		perror("Error opening train file");
		close(train_fd);
		return -1;
	}

	bool train_hamming;
	if (read(train_fd, (void*) &train_hamming, sizeof(bool)) < 0) {
		perror("Couldn't read train Hamming Status");
		return 1;
	}
	dprint("Read train Hamming Status (%d, %lu bytes)", train_hamming, sizeof(bool));

	if (query_hamming != train_hamming) {
		fputs("Norm Types don't match.\n", stderr);
		return 1;
	}

	size_t db_n;
	if (read(train_fd, (void*) &db_n, sizeof(db_n)) < 0) {
		perror("Couldn't read number of train images");
		return 1;
	}
	dprint("Read number of train images (%lu, %lu bytes)", db_n, sizeof(db_n));

#if 0
	/* I haven't researched FLANN very much. It seems buggy (but fast and accurate). */
	/* cv::FlannBasedMatcher matcher(new cv::flann::LshIndexParams(5, 24, 2)); */
	cv::FlannBasedMatcher matcher;
#else
	/* TODO: research advantages of l2 over l1 */
	cv::BFMatcher matcher(query_hamming ? cv::NORM_HAMMING : cv::NORM_L1, false);
#endif

	for (size_t i = 0; i < db_n; i++) {
		dprint("Processing train file %lu", i);

		std::vector<cv::KeyPoint> kp = read_keypoints(train_fd);
		cv::Mat d = read_descriptors(train_fd, descriptor_type, descriptor_bytes);

		size_t path_bytes;
		if (read(train_fd, (void*) &path_bytes, sizeof(size_t)) < 0) {
			perror("Couldn't read Length of FilePath");
			return 1;
		}

		char *filepath = (char*) malloc(path_bytes);
		if (read(train_fd, (void*) filepath, path_bytes) < 0) {
			perror("Couldn't read FilePath");
			return 1;
		}

#ifdef DEBUG
		fputs("Read FilePath: '", stderr);
		write(2, (void*)filepath, path_bytes);
		fputs("'\n", stderr);
#endif

		/* The following performs subspace clustering on cross-checked matches,
		 * extracting "buckets" of matches that may correspond to different
		 * objects. For now it is mostly for the purpose of filtering out
		 * isolated matches, a better option could be the OPTICS algorithm.

		 * Theoretically, this approach could have the advantage of allowing for
		 * recognition of multiple instances of objects, but i've chosen to
		 * prioritize fast recognition, because of the limited time. */

		std::list<std::stack<cv::DMatch>>
			buckets = subspace_clustering (
					cross_match(matcher, query_d, d, kp),
					kp, min_side, max_dist, min_points );

		free(d.data);

		dprint(" Grouped %lu buckets of matches", buckets.size());

		if (buckets.size()) {
			size_t w = 0;

			do {
				/* We extract matches from each bucket, if we project homography and find
				 * out where the inliers are, we can probably take other kinds of
				 * conclusions, too. */

				dprint(" Attempt %lu", w);

				/* We could study the global number of matches, along with the size of
				 * each bucket, and take a more proportional number of samples from them.
				 * It could be more performant in situations where we are iterating over
				 * empty buckets without need. FIXME */

				size_t ftsize = 0;

				for (auto it = buckets.begin(); it != buckets.end(); it++)
					ftsize += std::min(min_points, (*it).size());

				if (ftsize < 10) {
					dputs("  Gave Up: not enough Matches");
					break;
				}

				std::vector<cv::Point2f> from(ftsize), to(ftsize);
				size_t id = 0;

				for (auto it = buckets.begin(); it != buckets.end(); it++) {
					std::stack<cv::DMatch>& s = *it;
					/* dprint( "  stacksize: %lu", s.size()); */

					for (size_t k = 0; k < std::min(min_points, s.size()); k++) {
						cv::DMatch match = s.top();
						from[id] = query_kp[match.queryIdx].pt;
						to[id] = kp[match.trainIdx].pt;

						s.pop();
						id++;
					}
				}

				dprint("  %lu Matches extracted in total", ftsize);

				/* TODO vector<unsigned char> inliersMask(ftsize); */
				cv::Mat h = findHomography(from, to, CV_RANSAC, 1);

				{
					double det = h.at<double>(0,0)*h.at<double>(1,1) - \
										 h.at<double>(1,0)*h.at<double>(0,1); 

					/* FIXME double to float conversion */
					float certainty = (float) (1.0 - fabs(0.5 - det) * 2.0);
					dprint("  Determinant is %f. Certainty: %f", det, (double) certainty);

					/* dprint(" c%.1f%%", (double) (certainty*100.0f)); */

					if (certainty > minimum_certainty) {
						write(1, (void*)filepath, path_bytes);
						static const char new_line = '\n';
						write(1, (void*) &new_line, sizeof(char));
						break;
					}
					
					if (++w >= homography_attempts) {
						dputs("  Gave Up: Too many attempts");
						break;
					}
				}
			} while (true);
		}

		free(filepath);
	}

	free(query_d.data);
	close(train_fd);
	return 0;
}
