#include <argp.h>

const char *argp_program_version = "vdf_convert 0.1a",
			*argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Detect features and extract descriptors "\
										 "from image files and save them as vdf.\n\n"\
										 "enum FDDE: 0 - ORB, 1 - SIFT, 2 - SURF, 3 - BRISK, 4 - KAZE, 5 - AKAZE",

						args_doc[] = "[train file paths - GLOBs]";

enum FDDE {
	ORB,
	SIFT,
	SURF,
	BRISK,
	KAZE,
	AKAZE /* FREAK */
};

struct imagelist_t {
	char **filenames;
	size_t n_files;
};

typedef struct {
	enum FDDE fd, de;
	struct imagelist_t *input;
	char *out; 
	bool q, v;
} arguments_t;

static struct argp_option options[] = {
	{"verbose", 'v', NULL, 0, "show debug information", 0},
	{"query", 'q', NULL, 0, "generate query file instead of train file (default)", 0},
	{"feature-detector", 'd', "enum FDDE (0 - 5)", 0, "feature detector algorithm (3)", 0},
	{"descriptor-extractor", 'e', "enum FDDE (0 - 5)", 0, "descriptor extractor algorithm (3)", 0},
	{"output", 'o', "STRING", 0, "output to file path (stdout)", 0},
	{NULL, '\0', NULL, 0, NULL, 0}
};

#include <glob.h>
#include <stdlib.h>
__inline__ static int
globs_to_filenames(
		char **glob_path,
		const size_t n_globs,
		struct imagelist_t **il_r )
{
	glob_t **globs = ( glob_t** ) malloc ( sizeof(glob_t*) * n_globs );

	size_t n_files = 0;

	fputs("GLOBS\n", stderr);
	for (size_t i = 0; i <  n_globs; i++) {
		int ret;
		glob_t *g = (glob_t*) malloc (sizeof(glob_t));

		fprintf(stderr, "%s\n", *glob_path);
		switch (glob(*glob_path, GLOB_ERR, NULL, g)) {
			case 0: goto accept_glob;
			case GLOB_NOSPACE: ret = ENOMEM; break;
			case GLOB_ABORTED: ret = EIO; break;
			case GLOB_NOMATCH: ret = ENOENT; break;
			default: fputs("Unknown error - Glob\n", stderr); ret = 1;
		}

		globfree(g);

		for (size_t j = 0; j < i; j++) globfree(globs[j]);
		free(globs);

		return ret;

accept_glob:
		n_files += g->gl_pathc;

		globs[i] = g;
		glob_path++;
	}

	struct imagelist_t *il = (struct imagelist_t*) malloc(sizeof(struct imagelist_t));
	char **filenames = il->filenames = (char**) malloc (sizeof(char*) * n_files);

	fputs("FILES\n", stderr);
	for (size_t i = 0; i < n_globs; i++) {
		register glob_t *g = globs[i];

		for (size_t j = 0; j < g->gl_pathc; j++) {
			*filenames = g->gl_pathv[j];
			fprintf(stderr, "%s\n", *filenames);
			filenames++;
		}

		globfree(g);
	}

	free(globs);

	il->n_files = n_files;
	*il_r = il;

	return 0;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = (arguments_t*) state->input;

	switch (key) {
		case 'q': arguments->q = true; break;
		case 'v': arguments->v = true; break;
		case 'o': arguments->out = arg; break;
		case 'e': arguments->de = (enum FDDE) strtol(arg, NULL, 10); break; 
		case 'd': arguments->fd = (enum FDDE) strtol(arg, NULL, 10); break;
		case ARGP_KEY_NO_ARGS: argp_usage(state); break;
		case ARGP_KEY_ARG:
			{
				register size_t start_arg = state->next - 1;
				return globs_to_filenames(&state->argv[start_arg], state->argc - start_arg, &arguments->input);
			}
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
	args.q = args.v = false;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	return args;
}

#include <vector>
using std::vector;

#include <opencv2/opencv.hpp>

#include "stdio.h"

static void 
write_keypoints(FILE *out, vector<cv::KeyPoint>& kp, bool v) {
	size_t c = kp.size();
	if (v) fprintf(stderr, "wk: %lu\n", c);
	fwrite(&c, sizeof(size_t), 1, out);
	fwrite(&kp[0], sizeof(cv::KeyPoint), c, out);
}

static void
write_descriptors(FILE *out, cv::Mat& d, bool v) {
	int cols = d.cols, rows = d.rows;
	if (v) fprintf(stderr, "wd: %d %d\n", cols, rows);
	fwrite(&cols, sizeof(int), 1, out);
	fwrite(&rows, sizeof(int), 1, out);

	fwrite(d.data, sizeof(char), cols*rows, out);
}

#include <opencv2/xfeatures2d.hpp>

template <class T>
static cv::Ptr<T>
get_algorithm(enum FDDE id) {
	switch (id) {
		case 0: return cv::ORB::create();
		case 1: return cv::xfeatures2d::SIFT::create();
		case 2: return cv::xfeatures2d::SURF::create();
		case 3: return cv::BRISK::create();
		case 4: return cv::KAZE::create();
		case 5: return cv::AKAZE::create();
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
	char **input_filenames;
	size_t n_files;
	{
		register imagelist_t *il = args.input;
		input_filenames = il->filenames;
		n_files = il->n_files;
	}

	if (args.q) {
		if (n_files > 1) {
			fputs("For query file generation, only one input file can be used.\n", stderr);
			fclose(out);
			return 1;
		}

		cv::Mat image = cv::imread(input_filenames[0]);

		vector<cv::KeyPoint> kp;
		detector->detect(image, kp);
		write_keypoints(out, kp, args.v);

		cv::Mat d;
		extractor->compute(image, kp, d);
		write_descriptors(out, d, args.v);

	} else {
		fwrite(&n_files, sizeof(size_t), 1, out);
		if (args.v) fprintf(stderr, "wn: %lu\n", n_files);

		for (size_t i = 0; i < n_files; i++) {

			char *filename = input_filenames[i];
			cv::Mat image = cv::imread(filename);

			fwrite(&image.cols, sizeof(int), 1, out);
			fwrite(&image.rows, sizeof(int), 1, out);

			vector<cv::KeyPoint> kp;
			detector->detect(image, kp);
			write_keypoints(out, kp, args.v);

			cv::Mat d;
			extractor->compute(image, kp, d);
			write_descriptors(out, d, args.v);

			char c;
			do {
				c = *(filename++);
				fputc(c, out);
			} while (c);
		}
	}

	fclose(out);

	return 0;
}
