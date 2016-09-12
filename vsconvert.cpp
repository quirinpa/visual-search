#include <argp.h>

const char *argp_program_version = "vsconvert " PROGRAM_VERSION,
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
		"Specify feature detector algorithm (default: BRISK, where: "
		"0-BRISK; 1-ORB; 2-SIFT; 3-SURF; 4-KAZE; 5-AKAZE", 0 },

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
	args.de = args.fd = (FDDE) 0;
	args.out = NULL;
	args.q = false;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	return args;
}

#include <vector>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include "debug.h"

#include <unistd.h>
static void
write_keypoints(
		int fd,
		std::vector<cv::KeyPoint>& kp )
{
	size_t n_keypoints = kp.size(), 
				 keypoint_bytes = n_keypoints * sizeof(cv::KeyPoint);
	
	if (write(fd, (void*) &n_keypoints, sizeof(size_t)) < 0) {
		perror("Couldn't write Keypoint Vector # Bytes");
		exit(1);
	}

	if (write(fd, (void*) &kp[0], keypoint_bytes) < 0) {
		perror("Couldn't write Keypoint Vector");
		exit(1);
	}

	dprint("Wrote Keypoint Vector (%lu, %lu bytes)", 
			n_keypoints, keypoint_bytes + sizeof(size_t));
}

static void
write_descriptors(
		int fd,
		cv::Mat& d,
		size_t descriptor_bytes )
{
	int dim[2] = { d.rows, d.cols };

	if (write(fd, (void*) dim, sizeof(dim)) < 0) {
		perror("Couldn't write Descriptor Mat Dimensions");
		exit(1);
	}

	size_t all_descriptor_bytes = d.rows * d.cols * descriptor_bytes;

	if (write(fd, (void*) d.data, all_descriptor_bytes) < 0) {
		perror("Couldn't write Descriptor Mat");
		exit(1);
	}

	dprint("Wrote Descriptor Mat (%dx%d, %lu bytes)", 
			dim[0], dim[1], all_descriptor_bytes + sizeof(dim));
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

#include <fcntl.h>
int main(int argc, char **argv) {
	arguments_t args = get_arguments(argc, argv);
	int output_fd =  args.out ?
		open(args.out,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUSR | S_IWUSR | S_IRGRP ) : 1;

	enum FDDE detector_id = args.fd,
						extractor_id = args.de;

	if (detector_id > 3 && detector_id != extractor_id) {
		fputs("Detector and extractor are incompatible\n", stderr);
		close(output_fd);
		return 1;
	}

	size_t descriptor_bytes;
	{
		bool norm_hamming = detector_id < 2;

		if (norm_hamming) {
			descriptor_bytes = sizeof(char);

			if (detector_id != extractor_id) {
				fputs("FeatureDetector and DescriptorExtractor NormTypes don't match", stderr);
				close(output_fd);
				return 1;
			}
		} else descriptor_bytes = sizeof(int);

		if (write(output_fd, (void*) &norm_hamming, sizeof(bool)) < 0) {
			perror("Couldn't write Descriptor Hamming Status");
			return 1;
		}

		dprint("Wrote Descriptor Hamming Status (%d, %lu bytes)", norm_hamming, sizeof(bool));
	}

	cv::Ptr<cv::FeatureDetector> detector =\
		get_algorithm <cv::FeatureDetector> ( detector_id );

	cv::Ptr<cv::DescriptorExtractor> extractor =
		(cv::Ptr<cv::DescriptorExtractor>) (
				detector_id == extractor_id ? detector :
				get_algorithm <cv::DescriptorExtractor> ( extractor_id ) );

	/* obtain image filenames that match glob */
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
					perror("Glob processing failed");
					close(output_fd);
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
			if (write(output_fd, (void*) &n_images, sizeof(n_images)) < 0) {
				perror("Coudn't write number of images");
				return 1;
			}

			dprint("Wrote number of images (%lu, %lu bytes)", n_images, sizeof(n_images));
		}
	}

	/* extract relevant attributes and save to file */
	try {
		for (auto it = images.begin(); it != images.end(); it++) {
			char *filename = *it;

			cv::Mat image = cv::imread(filename);

			if (image.empty()) {
				fprintf(stderr, "Couldn't read image '%s'", filename);
				return -1;
			}

			std::vector<cv::KeyPoint> kp;
			detector->detect(image, kp);
			write_keypoints(output_fd, kp);

			cv::Mat d;
			extractor->compute(image, kp, d);
			write_descriptors(output_fd, d, descriptor_bytes);

			/* FIXME save str len first */

			if (not_q) {
				size_t filepath_bytes = strlen(filename) * sizeof(char);

				if (write(output_fd, (void*) &filepath_bytes, sizeof(size_t)) < 0) {
					perror("Couldn't write filepath # bytes");
					return 1;
				}

				if (write(output_fd, (void*) filename, filepath_bytes) < 0) {
					perror("Couldn't write filepath");
					return 1;
				}

				dprint("Wrote FilePath: %s", filename);
			}

		}
	} catch (cv::Exception& ex) { 
		fprintf(stderr, "Exception: %s\n", ex.what());
		return 1;
	}

	close(output_fd);
	return 0;
}
