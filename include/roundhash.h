#ifndef ROUNDHASH_H
#define ROUNDHASH_H

using std::hash;

struct hash_roundhash {
	size_t operator()(const Point2i &k) const {
		size_t h1 = hash<int>()(k.x),
					 h2 = hash<int>()(k.y);

		return (h1<<16) | h2;
	}
};

struct equals_roundhash {
	bool operator()( const Point2i& lhs, const Point2i& rhs) const {
		return (lhs.x == rhs.x) && (lhs.y == rhs.y);
	}
};

#include <unordered_map>
using std::unordered_map;
#include <stack>
using std::stack;

template<class T>
class RHC {
	private:
		typedef unordered_map<Point2i, stack<T>, hash_roundhash, equals_roundhash> rhc_map;

		rhc_map cells;
		Point2f multipliers;
		/* const sq_max_dist = max_dist * max_dist; */

		Point2i get_key(Point2f coords) {
			return Point2i(
					(int) (this->multipliers.x * (coords.x + .5f)),
					(int) (this->multipliers.y * (coords.y + .5f)) );
		}

		typename rhc_map::iterator findNear(Point2i key) {
			typename rhc_map::iterator find_it;

			find_it = this->cells.find(Point2i(key.x - 1, key.y));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x - 1, key.y - 1));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x, key.y - 1));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x + 1, key.y - 1));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x + 1, key.y));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x + 1, key.y + 1));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x, key.y + 1));
			if (find_it != this->cells.end()) return find_it;
			find_it = this->cells.find(Point2i(key.x - 1, key.y + 1));
			if (find_it != this->cells.end()) return find_it;

			return this->cells.begin();
		}

	public:
		RHC<T>(Point2i size, float max_dist) {
			const float grid_size = max_dist / 1.41421356237f;
			this->multipliers = Point2f(
					grid_size / (float) size.x,
					grid_size / (float) size.y );
		}

		void insert(T data, Point2f coords) {
			Point2i key = this->get_key(coords);

			auto map_it = this->cells.find(key);

			if (map_it != this->cells.end()) {
				map_it->second.push(data);
				return;
			}

			stack<T> s;
			s.push(data);

			this->cells[key] = s;
		}

		void cluster(
				function<void (T&)> each_data,
				function<void (void)> each_cluster )
		{
			auto map_it = this->cells.begin();

			if (map_it != this->cells.end()) {
				do {
					Point2i key = map_it->first;
					stack<T>& s = map_it->second;
					/* fprintf(stderr, "(%d,%d)", key.x, key.y); */

					do {
						each_data(s.top());
						s.pop();
					} while (!s.empty());

					this->cells.erase(map_it);
					map_it = this->findNear(key);

					if (map_it == this->cells.end())
						break;

					Point2i next_key = map_it->first;

					if (abs(next_key.x - key.x) > 1 || abs(next_key.y - key.y) > 1 )
						each_cluster();

				} while (true);
			}
		}
};

#endif
