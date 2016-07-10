#include "avl.h"

__inline__
static avl_t *avl_min(register avl_t *curr) {
	register avl_t *min = curr->left;

	while (min) {
		curr = min;
		min = curr->left;
	}

	return curr;
}

#include <stdlib.h>
void
free_avl(avl_t *root) {
	if (root) {
		register avl_t *curr = avl_min(root);

		/* if we wanted to render things in order, 
		 * we would do so HERE */

		do {
			register avl_t *right_child = curr->right;

			if (right_child)
				curr = avl_min(right_child);
				/* AND HERE */
			else {
				/* but we wanna free the tree so we must
				 * do it while going up from a right child */
				register bool_t prev_is_left_child;
				while (curr != root) {
					register avl_t *prev = curr;
					curr = curr->parent;
					prev_is_left_child = prev == curr->left;
					free(prev);

					if (prev_is_left_child) break;
				}
			}
		} while (true);
	}
}

/* if AVL_DEBUG is defined (it is supposed to "render" a node's value
 * aka write it as text to FILE*), we compile this with "debug capabilities".
 *
 * TODO be careful, for now, if you use this, other functions change the
 * number of arguments they receive. like avl_insert. If you have a better
 * idea, please, help me improve by commenting or sending PRs */

#ifdef AVL_DEBUG

#include "list.h"
/* DEBUG FUNCTION - provide a FILE* and an avl_t*, we write a simple html
 * representation of it to that file. Be careful, if avl_t code is
 * supposed to be working, insertion and stuff WILL be successful, which
 * means your program continues, and if the tree is big:
 *
 * THIS CAN CREATE VERY BIG HTML PAGES - you don't even need Wordpress */
void
avl_to_html(avl_t *root, FILE *o) {
	if (!root)
		fputs("<h4>No ROOT</h4>", o);
	else {
		list_t *parents = new_list((void*)root), *children;
		bool_t at_least_one;
		double divwidth = 100.0;
		
		do {
			children = new_list(NULL);
			at_least_one = false;

			do {
				avl_t *avlnode = (avl_t*) list_serve(parents);

				fprintf(o, "<div style=\"width:%.2f%%\">", divwidth);

				if (avlnode) {

					AVL_DEBUG(avlnode->key, o);

					{
						register signed char balance = avlnode->balance;
						if (balance) fprintf(o, ",<span>%d</span>", balance);
					}

					at_least_one = at_least_one || avlnode->left || avlnode->right;

					list_append(children, avlnode->left);
					list_append(children, avlnode->right);
				} else {
					fputs("_", o);

					list_append(children, NULL);
					list_append(children, NULL);
				}

				fputs("</div>", o);

			} while (parents->first);

			divwidth /= 2;
			free(parents);

			parents = children;
		} while (at_least_one);

		free(children);
	}
	
}

#endif

/* TODO finish clearing up the class and making remove work
 * After that, remove all debug functionalities and make a git
 * debug patch for it */

/* We use this to measure the height of the subtree rooted at c. See:
 *
 * https://upload.wikimedia.org/wikipedia/commons/c/c4/The_Rebalancing.gif
 *
 * We then infer the heights of the children d, b and a, respectively,
 * in order to calculate node balances afterwards, something implemented
 * for the performance benefits of it vs calculating or storing heights. */
#include <stddef.h>
__inline__ static size_t
get_height(register avl_t *curr) {
	register size_t h = 0;

	while (curr) {
		h++;
		curr = *( &curr->left + (curr->balance > 0) );
	}

	return h;

}

/* binary abs - i didnt test performance of this vs abs(), sorry.
 * Does anyone think this is slower? */
__inline__ static size_t
abs_balance (signed char balance)
{
	signed char mask = balance >> 7;
	return ( size_t ) ((mask ^ balance) - mask);
}

/* max(a, b), kind of the same deal, leave suggestions or make a PR (please) */
__inline__ static size_t
max_height(size_t a, size_t b)
{
	return ( size_t ) (a + b + abs_balance(( signed char )(a-b))) >> 1;
}

/* creates a mask that can be applied each time we need to invert
 * a value depending on the boolean that was provided
 * to new_bool_inv_mask(bool) aka bool?value:-value, this
 * is done to prevent forking. Am i right to do it like so? */
#include "bool.h"
__inline__
static signed char
new_bool_inv_mask(bool_t value) {
		return (signed char)
			~( ((int)value) * 0xff );
}

/* applies a mask created with new_bool_inv_mask() */
__inline__
static signed char
apply_mask(signed char balance, register signed char mask)
{
	return (signed char)
		( ((int)(balance^mask)) + (mask&1) );
}

/* Should be called every time we add/remove a child node of some parent node
 * n and have therefore increased or decreased n's balance.
 *
 * rheavy <- after the change, is n heavy to the right?
 *
 * returns the topmost node that took part in the rebalancing operation
 *
 * again, refer to:
 * https://upload.wikimedia.org/wikipedia/commons/c/c4/The_Rebalancing.gif
 *
 * i spent a lot of time already trying to improve performance of this,
 * but sometimes an outside perspective can help. Again, please do. */
static avl_t* rebalance(avl_t *n, bool_t n_rheavy
#ifdef AVL_DEBUG
		, FILE *log
#endif
		) {

	signed char n_balance = n->balance;

	if ((n_balance&3) == 2) {

		avl_t **ns_heaviest_ref = &n->left + n_rheavy,
					*f = *ns_heaviest_ref;

		signed char f_balance = f->balance;

		bool_t /* is f balanced to the same side as n? */
					 same_dir = n_balance == (f_balance<<1),
					 /* is f's tallest subtree to the right? */
           f_rheavy = same_dir == n_rheavy;

		avl_t **fs_left_child_ref = &f->left,
					**fs_heaviest_ref = fs_left_child_ref + f_rheavy;

		avl_t *l = *fs_heaviest_ref;

		avl_t **ls_left_child_ref = &l->left,
					/* c is l's child that comes from the same direction as b */
					**ls_c_ref = ls_left_child_ref + !f_rheavy,
					*c = *ls_c_ref;

		signed char imask_n_rheavy = new_bool_inv_mask(n_rheavy),
								imask_f_rheavy = new_bool_inv_mask(f_rheavy);

		size_t /* height of the subtree rooted at c */
					 c_height = get_height(c);

		signed char l_balance = l->balance;

		size_t /* height of the subtree rooted at d */ 
					 d_height = ( size_t ) ( (long long int)c_height +
							 apply_mask(l_balance, imask_f_rheavy) ),
					 /* height of the subtree rooted at l */ 
					 l_height = max_height(c_height, d_height) + 1,
					 /* height of l's sibling subtree */
					 b_height = ( size_t ) ( (long long int)l_height -
							 apply_mask(f_balance, imask_f_rheavy) ),
					 /* height of f's sibling subtree */
					 a_height = ( size_t ) ( (long long int)l_height + 1 -
							 apply_mask(n_balance, imask_n_rheavy) );

		register avl_t **ns_parent_ref = &n->parent,
						 *ns_parent = *ns_parent_ref;

#ifdef AVL_DEBUG
		if (log) {
			fputs(n_rheavy?">":"<", log);
			AVL_DEBUG(n->key, log);
			fputs((same_dir == n_rheavy)?">":"<", log);
			/* fprintf(log, "\na%lub%luc%lud%ldimask_n_rheavy%dimask_f_rheavy%d\n", */
			/* 		a_height, b_height, c_height, d_height, */
			/* 		(int)imask_n_rheavy, (int)imask_f_rheavy); */
		}
#endif

		if (same_dir) { /* one rotation */

			/* UPDATE BALANCES */

			n->balance = apply_mask((signed char)
					( a_height - b_height ), imask_n_rheavy);
			f->balance = apply_mask((signed char) 
					( max_height(b_height, a_height) + 1 - l_height ),
					imask_n_rheavy);

			/* UPDATE PARENT REFERENCES */

			*ns_parent_ref = f; /* n's parent is now f */

			{
				avl_t **fs_lightest_ref = fs_left_child_ref + !f_rheavy,
									*b = *fs_lightest_ref;

				if (b) b->parent = n; /* if possible, b's parent is now n */

				f->parent = ns_parent; /* f's parent is n's previous parent */

				/* UPDATE CHILD REFERENCES */

				if (ns_parent) /* if possible, update n's previous parent child */
					*(&ns_parent->left + (n == ns_parent->right)) = f;

				*fs_lightest_ref = n; /* n is f's child in the direction where b was */
				*ns_heaviest_ref = b; /* b is n's child in the direction where f was */

			}

			return f;
		}

		/* UPDATE BALANCES */

		n->balance = apply_mask((signed char) 
				(d_height - a_height), imask_n_rheavy);
		f->balance = apply_mask((signed char)
				(b_height - c_height), imask_n_rheavy);
		l->balance = apply_mask((signed char)
				( max_height(b_height, c_height) -
				  max_height(d_height, a_height)
				), imask_n_rheavy);

		/* UPDATE PARENT REFERENCES */

		*ns_parent_ref = l; /* n's parent is now l */
		f->parent = l; /* f's parent is now l */

		{
			register avl_t **ls_d_ref = ls_left_child_ref + f_rheavy,
							 *d = *ls_d_ref;

			if (d) d->parent = n; /* if possible, d's parent is now n */
			if (c) c->parent = f; /* if possible, c's parent is now f */

			l->parent = ns_parent; /* l's parent is n's previous parent */

			/* UPDATE CHILD REFERENCES */

			if (ns_parent) /* if possible update n's previous parent child */
				*(&ns_parent->left + (n == ns_parent->right)) = l;

			*fs_heaviest_ref = c; /* c is f's child in the direction where l was */
			*ls_c_ref = f; /* f is l's child in the direction where c was */

			*ns_heaviest_ref = d; /* d is n's child in the direction where f was */
			*ls_d_ref = n;  /* n is l's child in the direction where a was */
		}

		return l;
	}
	
	return n;
}

avl_t *
avl_insert(
		avl_t *root,
		void *data,
		void *key,
		bool_t (*cmp)(void*, void*)
#ifdef AVL_DEBUG
		, FILE *log
#endif
) {
	avl_t *child = (avl_t*) malloc(sizeof(avl_t));
	child->left = child->right = NULL;
	child->balance = 0;
	child->data = data;
	child->key = key;

	if (root) {
		register avl_t **parent_ref = &root, *parent;
		bool_t side;

		do {
			parent = *parent_ref;
			side = cmp(parent->key, key);
			parent_ref = &parent->left + side;

#ifdef AVL_DEBUG
			if (log)
				fputs(side?"r":"l", log);
#endif

		} while (*parent_ref);

#ifdef AVL_DEBUG
		if (log)
			AVL_DEBUG(key, log);
#endif

		child->parent = parent;
		*parent_ref = child;

		/* TODO work on the relationship between rebalance and dependent functions */
		do {
			/* signed char ns_balance = (signed char) (parent->balance + (side<<1) - 1); */
			parent->balance = (signed char) (parent->balance + (side<<1) - 1);

			{
				register avl_t *topnode = rebalance(parent, side
#ifdef AVL_DEBUG
				, log
#endif
					); /* top repositioned node by balance change */

				/* if it's parent is NULL, it shall be the new root */
				if (!(parent = topnode->parent)) return topnode;
				/* otherwise, if it has 0 balance, we need not chain
				 * any changes up because we are sure the height of topnode's
				 * subtree remains unchanged (the same as before the insert) */
				if (!topnode->balance) return root;

				side = topnode == parent->right;
			}
		} while (true);

	}

#ifdef AVL_DEBUG
	if (log)
		AVL_DEBUG(key, log);
#endif

	child->parent = NULL;
	return child;
}

/* static avlnode_t * */
/* rebalance_del( */
/*     avlnode_t *root, */
/*     avlnode_t *n, */
/*     bool_t side */
/* ) { */
/*   do { */
/*     n->balance = - (n->balance + 1 - (side<<1)) & 3; /1* side?-1:1 *1/ */
/*     /1* n->balance += 1 - (side<<1); /2* side?-1:1 *2/ *1/ */
/*     n = rebalance(n, !side); */
/*     if (n->balance) break; */
/*     if (!n->parent) return n; */
/*     side = get_side(n); */
/*     n = n->parent; */
/*   } while (true); */

/*   return root; */
/* } */

/* static avlnode_t * */
/* _avl_remove(avlnode_t* root, avlnode_t* n) */
/* { */

/* #ifdef AVL_DEBUG */
/*   fputs("x", stderr); */
/* 	AVL_DEBUG(n->data); */
/* #endif */

/*   if (n->LEFT) { */
/*     avlnode_t *l = n->LEFT, *lp; */
/*     bool_t lside; */

/*     while (l->RIGHT) l = l->RIGHT; */

/* #ifdef AVL_DEBUG */
/* 		fputs("l", stderr); */
/* 		AVL_DEBUG(l->data); */
/* #endif */

/*     lp = l->parent; */
/*     lside = get_side(l); */
/*     l->balance = n->balance; */

/*     if (n->parent) */
/*       reparent(n->parent, get_side(n), l); */
/*     else l->parent = NULL; */

/*     if (lp != n) { */

/* #ifdef AVL_DEBUG */
/* 		fputs("p", stderr); */
/* 		AVL_DEBUG(lp->data); */
/* 		fputs(":", stderr); */
/* #endif */

/*       if (l->LEFT) */
/*         reparent(lp, true, l->LEFT); */
/*       else lp->RIGHT = NULL; */

/*       reparent(l, false, n->LEFT); */
/*       reparent(l, true, n->RIGHT); */

/*       return root == n ? l : rebalance_del(root, lp, lside); */
/*     } */

/*     reparent(l, true, n->RIGHT); */
/*     return l->parent ? rebalance_del(root, l, get_side(l)) : l; */
/*   } */

/*   { */
/*     avlnode_t* p = n->parent; */

/* #ifdef AVL_DEBUG */
/* 		fputs("r", stderr); */
/* #endif */

/*     if (p) { */
/*       bool_t side = get_side(n); */

/* #ifdef AVL_DEBUG */
/* 			fputs("p", stderr); */
/* #endif */

/*       if (n->RIGHT) reparent(p, true, n->RIGHT); */

/*       return rebalance_del(root, p, side); */
/*     } */

/* #ifdef AVL_DEBUG */
/* 		fputs("r", stderr); */
/* #endif */

/*     if (n->RIGHT) */
/*       n->RIGHT->parent = NULL; */

/*     return n->RIGHT; */
/*   } */

/* #ifdef AVL_DEBUG */
/* 		fputs("\n", stderr); */
/* #endif */
/* } */

/* void */
/* avl_remove(avltree_t* t, avlnode_t* n) */
/* { */
/*   t->root = _avl_remove(t->root, n); */
/* 	t->size--; */
/*   free(n); */
/* } */

/* static avlnode_t* */
/* _avl_find( */
/*     void *key, */
/*     avlnode_t *n, */
/*     access_gfp_t gkey, */
/*     compare_gfp_t cmp) */
/* { */
/*   while (n) { */
/*     int r = cmp(gkey(n->data), key); */
/*     if (r<0) n = n->LEFT; */
/*     else if (r>0) n = n->RIGHT; */
/*     else return n; */
/*   } */
/*   return NULL; */
/* } */

/* avlnode_t* */
/* avl_find(avltree_t *t, void *key) */
/* { */
/*   return _avl_find(key, t->root, */
/*       t->vptr->getkey, t->vptr->compare); */
/* } */


/* void* avl_max(avlnode_t* cur) { */
/* 	if (!cur) return NULL; */
/* 	while (cur->RIGHT) */
/* 		cur = cur->RIGHT; */

/* 	return cur->data; */
/* } */

/* #undef LEFT */
/* #undef RIGHT */
