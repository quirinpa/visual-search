#ifndef QARGP_H
#define QARGP_H

enum FDDE {
	ORB,
	SIFT,
	SURF,
	BRISK,
	KAZE,
	AKAZE
};

#include <stdbool.h>
typedef struct {
	char *query_path,
			 *train_path; 

	enum FDDE feature_detector,
						descriptor_extractor;

	bool ratio_test,
			 cross_match;

} arguments_t;

arguments_t get_arguments(int argc, char **argv);

#endif
