#include <argp.h>

static const char *argp_program_version = "visual-search 0.1a",
						 *argp_program_bug_address = "quirinpa@gmail.com";

static char doc[] = "Find an object in an image file/directory.\n\n" \
										 "\tFeature-detector / descriptor-extractor ID:\n" \
										 "\t0=ORB, 1=SIFT, 2=SURF, 3=BRISK, 4=KAZE, 5=AKAZE",

						args_doc[] = "[query path] [train path]";

static struct argp_option options[] = {
	{"feature-detector", 'd', "ID", 0, "feature detector algorithm", 0},
	{"descriptor-extractor", 'e', "ID", 0, "descriptor extractor algorithm", 0},
	{"ratio-test", 'R', "BOOL", 0, "Enable David Lowe's ratio test, KNN k=2. (default: 0)", 1},
	{"cross-match", 'c', "BOOL", 0, "Enable cross-matching (default: 1)", 1},
	{0}
};

#include "qargp.h"
#include <stdlib.h>
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments_t *arguments = state->input;

	switch (key) {
		case 'd':
			if (arg) arguments->feature_detector = (int)strtol(arg, NULL, 10);
			else argp_usage(state);
			break;
		case 'e':
			if (arg) arguments->descriptor_extractor = (int)strtol(arg, NULL, 10);
			else argp_usage(state);
			break;
		case 'R':
			arguments->ratio_test = arg && arg[0] == '1';
			break;
		case 'c':
			arguments->cross_match = arg && arg[0] == '1';
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;
		case ARGP_KEY_ARG:
			arguments->train_path = arg;
			arguments->query_path = &state->argv[state->next];

			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, NULL, 0 };

arguments_t get_arguments(int argc, char **argv) {
	arguments_t arguments;
	arguments.feature_detector = 0;
	arguments.descriptor_extractor = 0;
	arguments.cross_match = true;
	arguments.ratio_test = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	return arguments;
}
