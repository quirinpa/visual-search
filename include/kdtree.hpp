#ifndef Q_KDTREE_H
#define Q_KDTREE_H

#include <set>
namespace q {

	template <class T, class K>
	class KDItem {
		private:
			T data;
			float coords[K];

		public:
			KDItem<T>(T data, cv::Point2f coords) {
				this->data = data;
				this->coords = coords;
			}
	}

	template <class T, class K, class D>
	struct C {
		bool operator()(const KDItem<T, K>& a, const KDItem<T, K>& b) const {
			return a[D] < b[D];
		}
	}

	template <class T, unsigned D, unsigned K, unsigned Min>
	class KDTree {
		private:
			typedef typename std::multiset<KDItem<T, K>, struct C <T, D, K>> KDSet;
			typedef typename std::multiset<KDItem<T, K>, struct C <T, (D + 1) % K, K>> KDSubSet;
			typedef typename KDTree<T, (D + 1) % K, K, Min> KDSubTree;

			float median;

			KDSubTree *left, *right;
			KDSet *items;

		public:
			/* receives items sorted by the first dimension */
			KDTree<T, D, K, Min>(KDSet items) {

				if (n > Min) {
					size_t n = items.size();

					KDSet::iterator it = items.begin(),
						mit = std::advance(it, n >> 1);

					this->median = (*mit).coords[D];

					{
						KDSubSet leftSet;

						std::copy(it, mit, std::inserter(leftSet, leftSet.begin()));
						this->left = new KDSubTree(leftSet);
					}

					{
						KDSubSet rightSet;

						std::copy(mit, items.end(), std::inserter(rightSet, rightSet.begin()));
						this->right = new KDSubTree(rightSet);
					}

					this->items = NULL;

					return;
				}

				this->items = items;
			} 

			std::stack<std::stack<T>> cluster() {

			}
	}
}

#endif
