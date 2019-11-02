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
#include <unordered_map>

using namespace std;

struct Point {
	int x;
	int y;

	bool operator==(const Point& o) const { return (x == o.x && y == o.y); }
	bool operator!=(const Point& o) const { return (x != o.x || y != o.y); }

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
constexpr char DIR[] = { 'N','B','G',U,D,L,R,' ' };

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
	int id;
	Point p;
	Panel c;
};

struct Command {
	Point p;
	Panel c;

	string toString() const {
		return to_string(p.y) + " " + to_string(p.x) + " " + DIR[static_cast<int>(c)];
	}

};

using Field = FixedGrid<Panel, N, N>;
using Table = FixedGrid<int, N, N>;
using BitTable = FixedGrid<bool, N, N>;
using Robots = array<Robot, M>;

class Ai {
private:

	const Point goal;
	const Field field;
	const Robots iRobots;

	Table rangeTable;

	void makeRangeTable(const Point& start) {

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

	}

	void greedy(Robot robot, Field& signField, Field& passTable) const {

		const auto checkPassRoute = [&](const Point& d) {
			Point p = robot.p;
			for (int i = 0; i < N; i++)
			{
				p.x = (p.x + d.x + N) % N;
				p.y = (p.y + d.y + N) % N;
				if (passTable[p] != Panel::None)
				{
					if (signField[p] == passTable[p])
						return true;
				}
				else
					break;
			}
			return false;
		};

		const auto checkNextRoute = [&](const Point& d) {

			Point p = robot.p;
			Point maxPos;
			int maxRange = 0;

			for (int i = 0; i < N; i++)
			{
				p.x = (p.x + d.x + N) % N;
				p.y = (p.y + d.y + N) % N;

				if (field[p] == Panel::Block)
					break;

				if (passTable[p] != Panel::None)
				{
					if (maxRange < rangeTable[p])
					{
						maxRange = rangeTable[p];
						maxPos = p;
					}
				}
			}

			return pair<bool, Point>{maxRange > 0, maxPos};
		};

		Field myPassTable = passTable;

		while (robot.p != goal)
		{
			Point d{ 0,0 };
			switch (robot.c)
			{
			case Panel::U: d.y = -1; break;
			case Panel::D: d.y = 1; break;
			case Panel::L: d.x = -1; break;
			case Panel::R: d.x = 1; break;
			default:
				break;
			}

			if (passTable[robot.p] != Panel::None)
			{
				if (robot.c != passTable[robot.p])
				{
					if (!checkPassRoute(d))
					{
						signField[robot.p] = passTable[robot.p];
					}
				}
				break;
			}

			{
				const auto result = checkNextRoute(d);
				if (result.first)
				{
					for (int i = 0; i < N; i++)
					{
						if (passTable[robot.p] == Panel::None) myPassTable[robot.p] = robot.c;
						robot.p.x = (robot.p.x + d.x + N) % N;
						robot.p.y = (robot.p.y + d.y + N) % N;
						if (robot.p == result.second) break;
					}
				}
				else
				{
					int maxRange = 0;
					Point maxPos;
					Panel maxD = robot.c;
					if (robot.c != Panel::U)
					{
						const auto r = checkNextRoute(Point{ 0,-1 });
						if (r.first)
						{
							if (maxRange < rangeTable[r.second])
							{
								maxRange = rangeTable[r.second];
								maxPos = r.second;
								maxD = Panel::U;
							}
						}
					}
					if (robot.c != Panel::D)
					{
						const auto r = checkNextRoute(Point{ 0,1 });
						if (r.first)
						{
							if (maxRange < rangeTable[r.second])
							{
								maxRange = rangeTable[r.second];
								maxPos = r.second;
								maxD = Panel::D;
							}
						}
					}
					if (robot.c != Panel::L)
					{
						const auto r = checkNextRoute(Point{ -1,0 });
						if (r.first)
						{
							if (maxRange < rangeTable[r.second])
							{
								maxRange = rangeTable[r.second];
								maxPos = r.second;
								maxD = Panel::L;
							}
						}
					}
					if (robot.c != Panel::R)
					{
						const auto r = checkNextRoute(Point{ 1,0 });
						if (r.first)
						{
							if (maxRange < rangeTable[r.second])
							{
								maxRange = rangeTable[r.second];
								maxPos = r.second;
								maxD = Panel::R;
							}
						}
					}

					if (maxRange > 0)
					{
						signField[robot.p] = maxD;
						robot.c = maxD;
						Point d2{ 0,0 };
						switch (robot.c)
						{
						case Panel::U: d2.y = -1; break;
						case Panel::D: d2.y = 1; break;
						case Panel::L: d2.x = -1; break;
						case Panel::R: d2.x = 1; break;
						default:
							break;
						}

						for (int i = 0; i < N; i++)
						{
							if (passTable[robot.p] == Panel::None) myPassTable[robot.p] = robot.c;
							robot.p.x = (robot.p.x + d2.x + N) % N;
							robot.p.y = (robot.p.y + d2.y + N) % N;
							if (robot.p == maxPos) break;
						}
					}
					else
					{
						Point nextPos;
						nextPos.x = (robot.p.x + d.x + N) % N;
						nextPos.y = (robot.p.y + d.y + N) % N;

						if (rangeTable[robot.p] > rangeTable[nextPos])
						{
							myPassTable[robot.p] = robot.c;
							robot.p = nextPos;
						}
						else
						{
							if (rangeTable[robot.p] > rangeTable[(robot.p.y - 1 + N) % N][robot.p.x])
							{
								signField[robot.p] = Panel::U;
								robot.c = signField[robot.p];
								myPassTable[robot.p] = robot.c;
								robot.p.y = (robot.p.y - 1 + N) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[(robot.p.y + 1) % N][robot.p.x])
							{
								signField[robot.p] = Panel::D;
								robot.c = signField[robot.p];
								myPassTable[robot.p] = robot.c;
								robot.p.y = (robot.p.y + 1) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[robot.p.y][(robot.p.x - 1 + N) % N])
							{
								signField[robot.p] = Panel::L;
								robot.c = signField[robot.p];
								myPassTable[robot.p] = robot.c;
								robot.p.x = (robot.p.x - 1 + N) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[robot.p.y][(robot.p.x + 1) % N])
							{
								signField[robot.p] = Panel::R;
								robot.c = signField[robot.p];
								myPassTable[robot.p] = robot.c;
								robot.p.x = (robot.p.x + 1) % N;
							}
							else
							{
								break;
							}
						}

					}

				}
			}
		}

		passTable = myPassTable;

	}

	vector<Command> greedy(const Robots& rangeSortRobots) const {

		vector<Command> answer;
		Field signField = field;
		Field passTable(Panel::None);

		//for (int i = 0; i < 1; i++)
		for (int i = 0; i < M; i++)
		{
			Robot robot = rangeSortRobots[i];

			greedy(robot, signField, passTable);
		}

		for (int y = 0; y < N; y++)
		{
			for (int x = 0; x < N; x++)
			{
				switch (signField[y][x])
				{
				case Panel::U:
				case Panel::D:
				case Panel::L:
				case Panel::R:
					answer.push_back(Command{ Point{x,y},signField[y][x] });
					break;
				default:
					break;
				}
			}
		}

		return answer;
	}


public:

	Ai(const Point _goal, const Field& _field, const Robots& _iRobots)
		: goal(_goal), field(_field), iRobots(_iRobots) {

		makeRangeTable(goal);

	}


	vector<Command> think() {

		auto rangeSortRobots = iRobots;
		sort(rangeSortRobots.begin(), rangeSortRobots.end(), [&](const decltype(rangeSortRobots)::reference a, const decltype(rangeSortRobots)::reference b) {return rangeTable[a.p] > rangeTable[b.p]; });

		const auto answer = greedy(rangeSortRobots);

		if (!answer.empty()) return answer;
		return { Command{0,0,Panel::U} };
	}

};

int main() {

	int n, m, b;
	Point goal;
	Point block;

	Robots robots;
	Field field(Panel::None);

	cin >> n >> m >> b;
	cin >> goal.y >> goal.x;
	field[goal] = Panel::Goal;

	{
		int id = 0;
		for (auto& robot : robots)
		{
			char c;
			cin >> robot.p.y >> robot.p.x >> c;
			switch (c)
			{
			case U: robot.c = Panel::U; break;
			case D: robot.c = Panel::D; break;
			case L: robot.c = Panel::L; break;
			case R: robot.c = Panel::R; break;
			default:
				break;
			}
			robot.id = id;
			id++;
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