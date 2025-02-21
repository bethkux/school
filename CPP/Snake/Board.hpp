
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include <iostream>
#include <algorithm>
#include <set>
#include <queue>
#include <array>
#include <vector>
#include <random>
#include <chrono>


using IntT = int;	//size_t;
using VecIntT = std::vector<IntT>;


////////////////////////////////////////////////////////////
/// Board class holds data about the current state of the board 
/// as well as algorithms for shifting the snake, generating 
/// new item or even auto-piloting the snake itself.
////////////////////////////////////////////////////////////
class Board {
public:
	Board(){} 

	Board(IntT s, IntT len) : _size(s + 2), _neighbor_dirs{ _size * -1 , _size , -1, 1 }, _snake(init_snake(len)), _generator(std::chrono::system_clock::now().time_since_epoch().count()), 
		_distribution(0, _size * _size), _item(generate_item()) {}

	// Return dimensions of the board
	IntT size() const {
		return _size;
	}

	// Assign new value to _snake
	void set_snake(const VecIntT& snake) {
		_snake = snake;
	}

	// Returns a reference to _snake
	VecIntT const& snake() const {
		return _snake;
	}

	// Returns _snake length (size of the vector)
	IntT snake_length() const {
		return _snake.size();
	}

	// Returns head of the snake (first element of the vector)
	IntT head(const VecIntT& snake) const {
		return snake[0];
	}

	// Returns tail of the snake (last element of the vector)
	IntT tail(const VecIntT& snake) const {
		return snake[snake.size() - 1];
	}

	// Returns _item
	IntT item() {
		return _item;
	}

	// Assigns _item a new value
	void set_item(IntT i) {
		_item = i;
	}

	// Shift directions by one
	void shift_neighbors() {
		auto temp = _neighbor_dirs[0];
		_neighbor_dirs[0] = _neighbor_dirs[1];
		_neighbor_dirs[1] = _neighbor_dirs[2];
		_neighbor_dirs[2] = _neighbor_dirs[3];
		_neighbor_dirs[3] = temp;
	}

	// Finds new random position for _item
	IntT generate_item() {
		IntT item;
		do {
			item = _distribution(_generator);
		} while (contains(_snake, item) || !is_inside(item));

		return item;
	}

	// The game is over
	bool gameOver() const {
		return _gameOver;
	}

	// Returns _ reference
	VecIntT const& path() const {
		return _path;
	}

	// _path has no elements
	bool isPathEmpty() const {
		return _path.empty();
	}

	// tile is inside the board (valid tile for the snake)
	bool is_inside(IntT tile) const {
		return ((tile > _size) && (tile < _size * (_size - 1)) && (tile % _size != 0) && (tile % _size != _size - 1));
	}

	// Checks if snake contains a tile (a part of its body lies on a tile)
	bool contains(const VecIntT& snake, IntT tile) const {
		return std::find(snake.begin(), snake.end(), tile) != snake.end();
	}

	// Moves a snake on a path. If consumed_item, the snake becomes longer. If cut_first, the first element on path doesn't count.
	VecIntT shift(const VecIntT& path, const VecIntT& snake, const bool consumed_item, const bool cut_first) const {
		VecIntT concatenatedVector(path.begin() + cut_first, path.end());
		std::reverse(concatenatedVector.begin(), concatenatedVector.end());
		concatenatedVector.insert(concatenatedVector.end(), snake.begin(), snake.end());
		concatenatedVector.resize(snake.size() + consumed_item);
		return concatenatedVector;
	}

	// Moves a snake by one element (path). If consumed_item, the snake becomes longer.
	VecIntT shift(IntT path, const VecIntT& snake, const bool consumed) const {
		return shift(VecIntT(1, path), snake, consumed, false);
	}

	// Moves _snake by one tile according to _path
	bool shift_snake() {

		_snake = shift(_path[0], _snake, _path.size() == 1 && _toItem);

		_path.erase(_path.begin());

		if (!_gameOver && _path.empty() && _toItem) {
			_item = generate_item();
			return true;
		}
		return false;
	}

	// Auto-pilot algorithm
	void autoPilotStep() {

		VecIntT path;
		shift_neighbors();

		// Find item
		if (!(path = BFS(head(_snake), _item, _snake, false, true)).empty()) {

			// Is going to eat the last item - WIN
			if ((IntT)_snake.size() + 1 == (_size * _size - 4 * (_size - 1))) {
				_path = path;
				_toItem = true;
				_gameOver = true;
				return;
			}

			auto shifted_snake = shift(path, _snake, true, false);

			// Look for tail to check if path is safe
			if (!BFS(head(shifted_snake), tail(shifted_snake), shifted_snake, false, true).empty())
			{
				_path = path;
				_toItem = true;

				// Set cycles to zero
				cycle1 = 0;
				cycle2 = 0;
				return;
			}
		}

		// Find tail
		if (cycle1 < (IntT)_snake.size() && !(path = BFS(head(_snake), tail(_snake), _snake, true, true)).empty()) {
			_path.push_back(path.front());
			_toItem = false;
			++cycle1;
			return;
		}

		// Too many cycles - LOSE
		if (cycle2 > (IntT)_snake.size() * 3) {
			_gameOver = true;
			return;
		}

		#pragma region Find alternative path to tail
		// Find different (longer) path to tail
		std::vector<VecIntT> paths;
		for (auto n : neighbours(head(_snake), _snake)) {
			if (n == _item) {
				paths.push_back(VecIntT());
				continue;
			}

			auto snake = shift(n, _snake, false);
			if (!(path = BFS(head(snake), tail(snake), snake, false, false)).empty()) {
				paths.push_back(path);
			}
			else {
				paths.push_back(VecIntT());
			}
		}

		// No paths from head to tail - LOSE
		if (paths.empty()) {
			_gameOver = true;
			return;
		}

		// Find longest path
		auto it = std::max_element(paths.begin(), paths.end(),
			[](const auto& a, const auto& b) {
				return a.size() < b.size();
			});

		// Take the first tile of the longest path
		_path.push_back(paths[std::distance(paths.begin(), it)].front());

		++cycle2;
		_toItem = false;
		#pragma endregion
	}


private:
	#pragma region Fields
	IntT _size = 0;								// Dimension of the square board
	std::array<int, 4> _neighbor_dirs{0,0,0,0};			// Neighboring tiles (up, down, left, right)
	VecIntT _snake;								// Body of snake
	std::default_random_engine _generator;		// Generator of random integers [0 - _size*_size)
	std::uniform_int_distribution<IntT> _distribution;		// Uniform integer distribution
	IntT _item = 0;								// Item that makes the snake grow
	VecIntT _path;									// A vector of tiles that the snake follows
	IntT cycle1 = 0;							// First cycle for chcecking if the snake gets stuck in a loop
	IntT cycle2 = 0;							// Second cycle for chcecking if the snake gets stuck in a loop
	bool _toItem = false;						// The goal of the current path of the snake is the item
	bool _gameOver = false;						// The snake either won or lost
	#pragma endregion

	// Initialize the snake with length len
	VecIntT init_snake(IntT len) const {
		VecIntT body;
		IntT current_tile = (_size / 2) * _size + (_size / 2);

		for (auto i : { 3, 1, 2, 0, 3 }) {
			while (is_inside(current_tile) && !contains(body, current_tile) && len > 0) {
				body.push_back(current_tile);
				current_tile += _neighbor_dirs[i];
				len--;
			}
			if (len < 1)
				break;
			current_tile = body.back();
			body.pop_back();
			++len;
		}
		return body;
	}

	// Find all neighbours of tile with the current snake position (tiles part of its body do not count)
	VecIntT neighbours(IntT tile, const VecIntT& snake) const {
		VecIntT tile_neighbours;
		for (auto n : _neighbor_dirs) {
			if (is_inside(tile + n) && (!contains(snake, tile + n) || (tail(snake) == tile + n && snake.size() > 2)))
				tile_neighbours.push_back(tile + n);
		}
		return tile_neighbours;
	}

	// Looks for the shortest path from a tile (from) to a tile (to) with the current snake position. Can avoid item if necessary
	VecIntT BFS(const IntT from, const IntT to, const VecIntT& snake, const bool avoid_item, const bool cut_first) {
		VecIntT path;
		path.push_back(from);

		std::set<IntT> visited;
		std::queue<VecIntT> queue;
		visited.insert(from);
		queue.push(path);

		while (!queue.empty()) {
			path = queue.front();
			queue.pop();
			if (path.back() == to) {
				if (cut_first)
					path.erase(path.begin());
				return path;
			}

			for (auto n : neighbours(path.back(), shift(path, snake, false, true))) {
				if (!visited.contains(n) && (!avoid_item || n != _item)) {
					visited.insert(n);
					auto p = VecIntT(path);
					p.push_back(n);
					queue.push(p);
				}
			}
		}

		return VecIntT();
	}
};
