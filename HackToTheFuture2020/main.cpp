#include <iostream>
#include <vector>
#include <tuple>
#include <array>
#include <queue>
#include <chrono>
#include <algorithm>
#include <string>
#include <random>
#include <bitset>
#include <numeric>
#include <valarray>
#include <functional>

using namespace std;

struct Point {
	int x;
	int y;
};

template<typename Type, size_t Width, size_t Height>
class FixedGrid {
private:

	using ContainerType = std::array<Type, Width* Height>;
	ContainerType m_data;

public:

	FixedGrid() = default;
	FixedGrid(const Type& v) { fill(v); }
	FixedGrid(const FixedGrid& other) = default;
	FixedGrid(FixedGrid&& other) {
		m_data = std::move(other.m_data);
	}

	FixedGrid& operator=(const FixedGrid& other) = default;
	FixedGrid& operator=(FixedGrid&& other) = default;

	const Type* operator[](size_t y) const {
		return &m_data[y * Width];
	}
	Type* operator[](size_t y) {
		return &m_data[y * Width];
	}
	const Type& operator[](const Point& p) const {
		return m_data[p.y * Width + p.x];
	}
	Type& operator[](const Point& p) {
		return m_data[p.y * Width + p.x];
	}

	const Type& at(size_t x, size_t y) const {
		if (outside(x, y))
			throw std::out_of_range("FixedGrid::at");
		return m_data[y * Width + x];
	}
	Type& at(size_t x, size_t y) {
		if (outside(x, y))
			throw std::out_of_range("FixedGrid::at");
		return m_data[y * Width + x];
	}

	constexpr size_t width() const {
		return Width;
	}
	constexpr size_t height() const {
		return Height;
	}

	bool inside(size_t x, size_t y) const {
		return (0 <= x && x < Width && 0 <= y && y < Height);
	}
	bool outside(size_t x, size_t y) const {
		return (0 > x || x >= Width || 0 > y || y >= Height);
	}

	void fill(const Type& v) noexcept {
		m_data.fill(v);
	}

	void clear() {
		m_data.swap(ContainerType());
	}

};

constexpr int N = 40;
constexpr int M = 100;
constexpr int B = 300;

constexpr char U = 'U';
constexpr char D = 'D';
constexpr char L = 'L';
constexpr char R = 'R';
constexpr char DIR[] = { U,D,L,R,' ' };

enum class Direction : unsigned char {
	U, D, L, R, Size
};

enum class Panel : unsigned char {
	None,
	Block,
	Goal,
	U,
	D,
	L,
	R
};

struct Robot {
	Point p;
	Direction c;
};

struct Command {
	Point p;
	Direction c;

	string toString() const {
		return to_string(p.x) + " " + to_string(p.y) + DIR[static_cast<int>(c)];
	}

};

using Field = FixedGrid<Panel, N, N>;
using Table = FixedGrid<int, N, N>;
using BitTable = FixedGrid<bool, N, N>;
using Robots = array<Robot, M>;

class Simulator {
private:


public:

	Simulator() {

	}

};

class Ai {
private:

	const Point goal;
	const Field field;
	const Robots iRobots;

	Table rangeTable;
	BitTable passTable;

	void makeRangeTable() {

		const auto start = goal;

		queue<Point> que;
		BitTable check(false);
		rangeTable.fill(0xffff);

		check[start] = true;
		rangeTable[start] = 0;
		que.push(start);

		const auto insert = [&](int x, int y, const Point& p) {
			if (!check[y][x] && field[y][x] != Panel::Block)
			{
				rangeTable[y][x] = rangeTable[p] + 1;
				check[y][x] = true;
				que.push(Point{ x ,y });
			}
		};

		while (!que.empty())
		{
			const auto p = que.front();
			que.pop();

			insert((p.x + N - 1) % N, p.y, p);
			insert((p.x + 1) % N, p.y, p);
			insert(p.x, (p.y + N - 1) % N, p);
			insert(p.x, (p.y + 1) % N, p);
		}

		/*
		for (int y = 0;y < N; y++)
		{
			for (int x = 0; x < N; x++)
			{
				cerr << hex << rangeTable[y][x] << ",";
			}
			cerr << endl;
		}
		*/

	}


public:

	Ai(const Point _goal, const Field& _field, const Robots& _iRobots)
		: goal(_goal), field(_field), iRobots(_iRobots) {

		makeRangeTable();

	}


	vector<Command> think() {


		return { Command{0,0,Direction::U} };
	}

};

int main() {

	int n, m, b;
	Point goal;
	Point block;

	Robots robots;
	Field field;

	cin >> n >> m >> b;
	cin >> goal.y >> goal.x;
	field[goal] = Panel::Goal;

	for (auto& robot : robots)
	{
		char c;
		cin >> robot.p.y >> robot.p.x >> c;
		switch (c)
		{
		case U: robot.c = Direction::U; break;
		case D: robot.c = Direction::D; break;
		case L: robot.c = Direction::L; break;
		case R: robot.c = Direction::R; break;
		default:
			break;
		}
	}

	for (int i = 0; i < b; i++)
	{
		cin >> block.y >> block.x;
		field[block] = Panel::Block;
	}

	Ai ai(goal, field, robots);

	const auto answer = ai.think();

	cout << answer.size() << endl;
	for (const auto& ans : answer)
	{
		cout << ans.toString() << endl;
	}

	return 0;
}
