//Point2f
template<class T>
class Point {
	T x, y;

	Point operator/(T a, Point b) {
		return Point(a / b.x, a / b.y);
	}
};

template<class T>
class Quadtree {
	private:
		typedef struct {
			T data;
			Point<float> coords;
		} leafnode_t;

		typedef struct {
			typedef struct {
				union {
					leafnode_t leaf;
					struct branchnode_st branchnode_t branch;
				} node;

				char is_leaf : 1;
			} subsection_t *dir[4];
		} branchnode_t;

		subsection_t *root;

	public:
		Quadtree(point_t size) {
			this.multipliers = 1/size;
			this.root = NULL;
		} 

		add(T data, Point<float> coords) {
			register subsection_t **subsection_pp = &this.root;

			if (this.root) {
				register Point<int>
					r_coords( (int)(coords.x + 0.5f), (int)(coords.y + 0.5f) ),
					hsize( this.size.x>>1, this.size.y>>1 );

				while (true) {
					register subsection_t *subsection_p = *subsection_pp;

					if (subsection_p->is_leaf) {
						subsection_p old_leaf_subsection = *subsection_p;

						*subsection_pp = new subsection_t(
								.branch  = [
									new subsection_t(
										.leaf = leafnode_t(data, coords),
										.is_leaf = true ), 

									old_leaf_subsection
								],
								.is_leaf = false );


						branchnode_t new_branch = 
						branchnode_t

						// subdivide tree and add leaf
						return;
					}

					register Point<bool> ES(
							r_coords.x >= hsize.x,
							r_coords.y >= hsize.y );

					subsection_pp = &(*subsection_pp)->dir[ (ES.x + ( ES.y << 1 )) ];

					if (!*subsection_pp) break;

					if (ES.x) r_coords.x -= hsize.x;
					if (ES.y) r_coords.y -= hsize.y;

					hsize = Point<int>( hsize.x>>1, hsize.y>>1 );

					subsection_p = *subsection_pp;
				}
			}

			*subsection_pp = new subsection_t(
				.leaf = leafnode_t(data, coords),
				.is_leaf = true
			);
		}
}
