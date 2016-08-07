template<class T>
typedef struct {
	public:
		T x, y;
		Point<T> operation<< (size_t v) {
			return Point<T>(this.x<<v, this.y<<v);
		}
		Point<T> operation+(Point<T> a, Point<T> b) {
			return Point<T>(a.x + b.x, a.y + b.y);
		}
} Point;

template<class T>
class Range {
	public:
		Point<T> min, max;
		bool contains(Point<T> p) {
			return
				p.x >= this.min.x && p.x < this.max.x ||
				p.y >= this.min.y && p.y < this.max.y;
		}
};

Point<int> coords_f_to_i(Point<float> coords_f) {
	return Point<int>( (int)(coords_f.x + .5f), (int)(coords_f.y + .5f) );
}

__inline__
size_t bs_to_idx(bool x, bool y) {
	return x + (y<<1);
}

size_t get_subsection (Point<int> p, Point<int> midp) {
	return (p.x >= midp.x) + ( (p.y >= midp.y)<<1 );
}

template<class T>
class QTreeLeaf;

template<class T>
class QTreeBranch : QTreeNode {
	private:
		void *nodes[4];
		char leaves : 4;
	public:
		QTreeBranch(QTreeNode<T>* parent) {
			super(parent)
			this.nodes = { NULL, NULL, NULL, NULL };
			this.leaves = 0;
		}

		QTreeBranch<T> *insert(
				QTreeLeaf<T> leaf,
			 	Point<int> pos,
			 	Point<int> hsize )
		{
			QTreeBranch *branch = &this;

			Point<int> leaf_coords_i = coords_f_to_i(leaf.coords);

			while (true) {
				Point<int> midp = pos + hsize;
				size_t d_dir = get_subsection( leaf_coords_i, midp);

				if (!( (1>>d_dir) & this.leaves )) {
					branch->leaves |= 1<<d_dir;
					branch->nodes[d_dir] = d_leaf;
					break;
				}

				branch->leaves ^= 1<<d_dir;

				branch =
					((QTreeBranch<T> *) branch->nodes[ d_dir ] =
					 (void*) new QTreeBranch<T>());

				pos += hsize;
				hsize <<= 1;
			}

			return branch;
		}
}

template<class T>
class QTreeLeaf {
	protected:
		T data;
		Point<float> coords;
	public:

		QTreeLeaf(T data, Point<float> coords) {
			this.data = data;
			this.coords = coords;
		}
		
		QTreeBranch<T> *insert(
				QTreeLeaf<T> &leaf,
				Point<int> pos,
				Point<int> hsize )
		{
			QTreeBranch<T> *branch = new QTreeBranch();

			branch->insert(this, pos, hsize);
			branch->insert(leaf, pos, hsize);

			return branch;
		}
}

#include <functional>
template<class T>
class Quadtree {
	private:
		Point<int> size;
		bool is_root_leaf;
		void *root;

	public:
		Quadtree(Point<int> size) {
			this.size = size;
			this.root = NULL;
		} 

		QTreeLeaf<T> *add(T data, Point<float> coords) {

			Point<int> pos( 0, 0 ),
				hsize = this.size >> 1;

			QTreeLeaf<T> *d_leaf = new QTreeLeaf<T> (data, coords);

			if (this.root) {
				if (is_root_leave) {
					this.is_root_leaf = false;

					this.root = (void*)( (QTreeLeaf<T>*) this.root )
						->insert(d_leaf, pos, hsize);
					return;
				}

				this.root.insert(d_leaf, pos, hsize);

			} else {
				this.root = d_leaf;
				this.is_root_leaf = true;
			}

			return d_leaf;
		}

		void cluster(size_t min_elemnts, float max_distance) {

		}
		/*
		 * to cluster further:
		 *
		 * - choose some leafnode, finding closest leafnode to that
		 * and checking if the distance is smaller than max_distance
		 * (if max_distance=0 find closest with no max-distance).
		 * - remove that leaf
		 * - repeat until there are no leaves left
		 * */

		void cluster(size_t min_elements, float max_distance) {
			if (!this.root) return;

			if (this.root->is_leaf) {
				free(this.root);
				this.root = NULL;
				return;
			}

			branchnode_t *branch_p = this.root->node.branch;
			subsection_t *valid_child;

			/* find a deep child (easy to remove) */
			do {
				register subsection_t *dir[4] = branch_p.dir;

				for (register size_t i = 0; i<4; i++) {
					register subsection_t *child = dir[i];

					if (child) {
						valid_child = child;

						if (!child->is_leaf) {
							branch_p = child->node.branch;
							break;
						}
					}
				}

			} while (true);
			
			if (this.root->is_leaf) {
				/* leafnode_t leaf = this.root->node.leaf; */
				free(this.root);
				this.root = NULL;
				return;
			}

			while (true) {

			}

		}

		void for_intersecting_subsections(
				Range<int> range,
				function<void (const T&)> for_each)
		{
			if (!this.root) return;

			if (this.root->is_leaf) {
				leafnode_t leaf = this.root->node.leaf;

				if (range.contains(leaf.coords))
					for_each(leaf.data);

				return;
			}

			branchnode_t *fitting_branch = &this.root;

			int hsizex = this.width>>1,
					hsizey = this.height>>1,
					midx = hsizex,
					midy = hsizey;

			do {
				size_t max_index = (size_t) maxx >= midx + ((maxy >= midy)<<1),
							 min_index = (size_t) minx >= midx + ((miny >= midy)<<1);

				if (min_index == max_index) {
					subsection_t *nextSection = fitting_branch.dir[min_index];

					if (nextSection->is_leaf) {
						register leafnode_t leaf = nextSection->node.leaf;

						if (range.contains(leaf.coords))
							for_each(leaf.data);

						return;
					}

					if (min_index&1) midx += hsizex;
					if (min_index>1) midy += hsizey;

					hsize.x >>= 1;
					hsize.y >>= 1;

					fitting_branch = &nextSection->node.branch;
					continue;
				}

				break;

			} while (true);


		}
}
