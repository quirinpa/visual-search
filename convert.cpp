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
	std::list<char *> glob_paths;
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

#include <glob.h>
#include <stdlib.h>
/* #include <unistd.h> */
/* #include <errno.h> */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *args = (arguments_t*) state->input;

	switch (key) {
		case 'q': args->q = true; break;
		case 'o': args->out = arg; break;
		case 'e': args->de = (enum FDDE) strtol(arg, NULL, 10); break; 
		case 'd': args->fd = (enum FDDE) strtol(arg, NULL, 10); break;
		case ARGP_KEY_NO_ARGS: argp_usage(state); break;
		case ARGP_KEY_ARG: args->glob_paths.push_back(arg); break;
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
static cv::Ptr<T>
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

__inline__ static int
process_glob(char * path, glob_t *save_glob) {
	switch (glob(path, GLOB_ERR, NULL, save_glob)) {
		case 0: return 0;
		case GLOB_NOSPACE: return ENOMEM;
		case GLOB_ABORTED: return EIO;
		case GLOB_NOMATCH: return ENOENT;
		default: fputs("Glob: Unknown error\n", stderr); return 1;
	}
}

int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);
	FILE *out = args.out ? fopen(args.out, "wb") : stdout;

	enum FDDE detector_id = args.fd,
						extractor_id = args.de;

	if (detector_id > 3 && detector_id != extractor_id) {
		fputs("Detector and extractor are incompatible\n", stderr);
		fclose(out);
		return 1;
	}

	{
		bool norm_hamming = detector_id < 2;

		if (norm_hamming && detector_id != extractor_id) {
			fputs("FeatureDetector and DescriptorExtractor NormTypes don't match", stderr);
			fclose(out);
			return 1;
		}

		fwrite(&norm_hamming, sizeof(bool), 1, out);
	}

	cv::Ptr<cv::FeatureDetector> detector =\
		get_algorithm <cv::FeatureDetector> ( detector_id );

	cv::Ptr<cv::DescriptorExtractor> extractor =
		(cv::Ptr<cv::DescriptorExtractor>) (
				detector_id == extractor_id ? detector :
				get_algorithm <cv::DescriptorExtractor> ( extractor_id ) );

	bool not_q = !args.q;
	std::list<char *> images;

	{
		size_t n_images = 0;
		auto it = args.glob_paths.begin();

		while (it != args.glob_paths.end()) {
			glob_t *leak_glob = (glob_t*) malloc(sizeof(glob_t));

			{
				register int ret = process_glob(*it, leak_glob);

				if (ret) {
					fclose(out);
					return ret;
				}
			}

			{
				char **path_it = leak_glob->gl_pathv,
						 **path_end = path_it + leak_glob->gl_pathc;

				while (path_it < path_end) {
					images.push_back(*path_it);
					n_images++;
					path_it++;
				}
			}

			it++;
		}

		if (not_q) {
			dprint("%lu images", n_images);
			fwrite(&n_images, sizeof(size_t), 1, out);
		}
	}

	try {
		for (auto it = images.begin(); it != images.end(); it++) {
			char *filename = *it;

			dprint("%s", filename);
			cv::Mat image = cv::imread(filename);

			if (image.empty()) {
				fprintf(stderr, "Error reading image '%s'\n", filename);
				fclose(out);
				return 1;
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
		fputs("Exception\n", stderr);
		fclose(out);
		return 1;
	}

	fclose(out);

	return 0;
}
