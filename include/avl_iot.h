#ifndef AVL_IOT_H
#define AVL_IOT_H

extern "C" {
#include "avl.h"
}

template<typename IOT_F>
void avl_iot(avl_t *root, IOT_F callback) {
	register avl_t *curr = root;

	if (curr) {

avl_iot_go_left:
		curr = avl_min(curr);

		callback(curr);

avl_iot_go_right:
		{
			register avl_t *right_child = curr->right;

			if (right_child) {
				curr = right_child;
				goto avl_iot_go_left;
			}
		}

		/* callback(curr); */

		{
			register avl_t *prev;

avl_iot_go_up:
			prev = curr;
			curr = curr->parent;

			if (!curr) return;

			{
				register bool_t prev_is_left_child = prev == curr->left;

				if (prev_is_left_child) {
					callback(curr);
					goto avl_iot_go_right;
				} else goto avl_iot_go_up;
			}
		}
	}
}

#endif
