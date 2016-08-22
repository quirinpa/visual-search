#include <argp.h>

const char *argp_program_version = "vdf_convert 0.1a",
			*argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Detect features and extract descriptors "\
										 "from image files and save them as vdf.\n\n"\
										 "enum FDDE: 0 - ORB, 1 - SIFT, 2 - SURF, 3 - BRISK, 4 - KAZE, 5 - AKAZE",

						args_doc[] = "[train file paths]";

enum FDDE {
	ORB,
	SIFT,
	SURF,
	BRISK,
	KAZE,
	AKAZE /* FREAK */
};

typedef struct {
	enum FDDE fd, de;
	char **filenames, *out; 
	size_t n;
	bool q;
} arguments_t;

static struct argp_option options[] = {
	{"query", 'q', NULL, 0, "generate query file instead of train file (default)", 0},
	{"feature-detector", 'd', "enum FDDE (0 - 5)", 0, "feature detector algorithm (3)", 0},
	{"descriptor-extractor", 'e', "enum FDDE (0 - 5)", 0, "descriptor extractor algorithm (3)", 0},
	{"output", 'o', "STRING", 0, "output to file path (stdout)", 0},
	{NULL, '\0', NULL, 0, NULL, 0}
};

#include <stdlib.h>
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = (arguments_t*) state->input;

	switch (key) {
		case 'q':
			arguments->q = true;
			break;

		case 'o':
			arguments->out = arg + 1;
			break;

		case 'e':
			arguments->de = (enum FDDE) strtol(arg, NULL, 10);
			break;

		case 'd':
			arguments->fd = (enum FDDE) strtol(arg, NULL, 10);
			break;

		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;

		case ARGP_KEY_ARG:
			{
				register size_t i = state->next - 1;
				arguments->n = state->argc - i;
				arguments->filenames = &state->argv[i];
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
	args.de = args.fd = (FDDE) 3;
	args.n = 0;
	args.out = NULL;
	args.q = false;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	return args;
}

#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>

#include "stdio.h"

static void 
write_keypoints(FILE *out, vector<cv::KeyPoint>& kp) {
	size_t c = kp.size();
	/* fprintf(stderr, "wk: %u\n", c); */
	fwrite(&c, sizeof(size_t), 1, out);

	for (size_t i = 0; i<c; i++)
		fwrite(&kp[i], sizeof(cv::KeyPoint), 1, out);
}

static void
write_descriptors(FILE *out, cv::Mat& d) {
	int cols = d.cols, rows = d.rows;
	/* fprintf(stderr, "wd: %d %d\n", cols, rows); */
	fwrite(&cols, sizeof(int), 1, out);
	fwrite(&rows, sizeof(int), 1, out);

	fwrite(d.data, sizeof(char), cols*rows, out);
}

#include <opencv2/xfeatures2d.hpp>

template <class T>
static cv::Ptr<T>
get_algorithm(enum FDDE id) {
	switch (id) {
		case 0:
			return cv::ORB::create();
		case 1:
			return cv::xfeatures2d::SIFT::create();
		case 2:
			return cv::xfeatures2d::SURF::create();
		case 3:
			return cv::BRISK::create();
		case 4:
			return cv::KAZE::create();
		case 5:
			return cv::AKAZE::create();
	}
	return cv::BRISK::create();
}

int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);
	enum FDDE fd_id = args.fd, de_id = args.de;

	if (fd_id > 5) {
		fprintf(stderr, "%u does not correspond to a valid feature detector;", fd_id);
		return 1;
	}

	if (fd_id > 3 && fd_id != de_id) {
		fputs("Chosen KAZE or AKAZE feature detector does not match descriptor extractor (must be of the same type).", stderr);
		return 1;
	}

	cv::Ptr<cv::FeatureDetector> detector = get_algorithm <cv::FeatureDetector> (fd_id);
	cv::Ptr<cv::DescriptorExtractor> extractor = get_algorithm <cv::DescriptorExtractor> (de_id);

	FILE *out = args.out ? fopen(args.out, "wb") : stdout;

	if (args.q) {
		if (args.n > 1)  {
			fputs("For query file generation, only one input file can be used.\n", stderr);
			fclose(out);
			return 1;
		}

		cv::Mat image = cv::imread(args.filenames[0]);

		vector<cv::KeyPoint> kp;
		detector->detect(image, kp);
		write_keypoints(out, kp);

		cv::Mat d;
		extractor->compute(image, kp, d);
		write_descriptors(out, d);

	} else for (size_t i = 0; i < args.n; i++) {
		fwrite(&args.n, sizeof(size_t), 1, out);
		/* fprintf(stderr, "wn: %u\n", args.n); */

		char *filename = args.filenames[i];
		cv::Mat image = cv::imread(filename);

		fwrite(&image.cols, sizeof(int), 1, out);
		fwrite(&image.rows, sizeof(int), 1, out);

		vector<cv::KeyPoint> kp;
		detector->detect(image, kp);
		write_keypoints(out, kp);

		cv::Mat d;
		extractor->compute(image, kp, d);
		write_descriptors(out, d);

		char c;
		do {
			c = *(filename++);
			fputc(c, out);
		} while (c);
	}

	fclose(out);

	return 0;
}
