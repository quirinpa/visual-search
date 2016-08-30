#include <argp.h>

const char *argp_program_version = "vdf_convert 0.1a",
			*argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] =
	"Detect features and extract descriptors from image files and save them to "
	"either a query or a train file so that you can search objects efficiently. ",
	args_doc[] = "[image file path]";

enum FDDE { BRISK, ORB, SIFT, SURF, KAZE, AKAZE };

#include <list>
typedef struct {
	enum FDDE fd, de;
	char *out; 
	bool q;
	std::list<char *> inputs;
} arguments_t;

static struct argp_option options[] = {
	{ "query", 'q', NULL, 0,
		"A query file contains an image of the object to be searched. "
		"Use this option to generate this type of file. "
		"Otherwise a database file is genrated.", 0 },

	{ "feature-detector", 'd', "UCHAR", 0,
		"Specify feature detector algorithm (default: BRISK).\nAvailable options: "
		"0: BRISK; 1: ORB; 2: SIFT; 3: SURF; 4: KAZE; 5: AKAZE", 0 },

	{ "descriptor-extractor", 'e', "UCHAR", 0,
		"Specify descriptor extractor algorithm (default: BRISK).", 0 },

	{ "output", 'o', "FILE", 0, "Specify output filename. If (stdout)", 0 },

	{NULL, '\0', NULL, 0, NULL, 0}
};

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *args = (arguments_t*) state->input;

	switch (key) {
		case 'q': args->q = true; break;
		case 'o': args->out = arg; break;
		case 'e': args->de = (enum FDDE) strtol(arg, NULL, 10); break; 
		case 'd': args->fd = (enum FDDE) strtol(arg, NULL, 10); break;
		case ARGP_KEY_NO_ARGS: argp_usage(state); break;
		case ARGP_KEY_ARG:
			if ( access( arg, F_OK | R_OK ) == -1 ) return errno;

			args->inputs.push_back(arg);
			break;

		default: return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, NULL, 0 };

__inline__ static arguments_t
get_arguments(int argc, char **argv)
{
	arguments_t args;
	args.de = args.fd = (FDDE) 3;
	args.out = NULL;
	args.q = false;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	return args;
}

#include <vector>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include "debug.h"

static void 
write_keypoints(FILE *out, std::vector<cv::KeyPoint>& kp) {
	size_t c = kp.size();
	
	dprint("wk: %lu", c);
	fwrite(&c, sizeof(size_t), 1, out);
	fwrite(&kp[0], sizeof(cv::KeyPoint), c, out);
}

static void
write_descriptors(FILE *out, cv::Mat& d) {
	int cols = d.cols, rows = d.rows;

	dprint("wd: %d %d", cols, rows);
	fwrite(&cols, sizeof(int), 1, out);
	fwrite(&rows, sizeof(int), 1, out);

	fwrite(d.data, sizeof(char), cols*rows, out);
}

#include <opencv2/xfeatures2d.hpp>

template <class T>
static T*
get_algorithm(enum FDDE id) {
	switch (id) {
		case 0: return cv::BRISK::create();
		case 1: return cv::ORB::create();
		case 2: return cv::xfeatures2d::SIFT::create();
		case 3: return cv::xfeatures2d::SURF::create();
		case 4: return cv::KAZE::create();
		case 5: return cv::AKAZE::create();
		/* case 6: return cv::xfeatures2d::FREAK::create(); */
	}
	return cv::BRISK::create();
}
/* #include <opencv2/highgui/highgui.hpp> */

int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);
	enum FDDE fd_id = args.fd, de_id = args.de;

	if ( fd_id > 5 || (fd_id > 3 && fd_id != de_id) ) return 1;

	FILE *out = args.out ? fopen(args.out, "wb") : stdout;

	cv::FeatureDetector* detector = get_algorithm <cv::FeatureDetector> ( fd_id );
	cv::DescriptorExtractor* extractor = fd_id == de_id ?
		(cv::DescriptorExtractor*) detector :
		get_algorithm <cv::DescriptorExtractor> (de_id);

	std::list<char*>& inputs = args.inputs;
	bool not_q = !args.q, is_bin = fd_id < 2 && de_id < 2;
	size_t n_inputs = inputs.size();

	fwrite(&is_bin, sizeof(bool), 1, out);

	if (not_q) {
		dprint("wn: %lu", n_inputs);
		fwrite(&n_inputs, sizeof(size_t), 1, out);
	}

	try {
		for (auto it = inputs.begin(); it != inputs.end(); it++) {
			char *filename = *it;

			cv::Mat image = cv::imread(filename);

			if (image.empty()) throw "image is empty";

			if (not_q) {
				fwrite(&image.cols, sizeof(int), 1, out);
				fwrite(&image.rows, sizeof(int), 1, out);
			}

			std::vector<cv::KeyPoint> kp;
			detector->detect(image, kp);
			write_keypoints(out, kp);

			cv::Mat d;
			extractor->compute(image, kp, d);
			write_descriptors(out, d);

			if (not_q) {
				char c;
				do {
					c = *(filename++);
					fputc(c, out);
				} while (c);
			}
		}
	} catch (cv::Exception& e) { 
		fclose(out);
		return 1;
	}

	fclose(out);
	return 0;
}
