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
#include <list>

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

class MilliSecTimer {
private:

	std::chrono::time_point<std::chrono::high_resolution_clock> s;
	unsigned long long int startCycle = 0;
	long long time = 0;

	const double CyclePerMilliSec = 2794000.0;

#ifndef _MSC_VER
	unsigned long long int getCycle() const {
		unsigned int low, high;
		__asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
		return ((unsigned long long int)low) | ((unsigned long long int)high << 32);
	}
#endif // _MSC_VER

public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	MilliSecTimer() = default;
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="_time">設定時間(ミリ秒)</param>
	MilliSecTimer(const std::chrono::milliseconds& _time) noexcept { time = _time.count(); }

	/// <summary>
	/// 時間を設定する
	/// </summary>
	/// <param name="_time">設定時間(ミリ秒)</param>
	void set(const std::chrono::milliseconds& _time) noexcept { time = _time.count(); }

	/// <summary>
	/// タイマーを開始させる
	/// </summary>
	void start() noexcept {
#ifdef _MSC_VER
		s = std::chrono::high_resolution_clock::now();
#else
		startCycle = getCycle();
#endif // _MSC_VER
	}

	/// <summary>
	/// 設定時間経過したかを得る
	/// </summary>
	/// <returns>経過していれば true, それ以外は false</returns>
	inline const bool check() const noexcept {
#ifdef _MSC_VER
		const auto e = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count() >= time;
#else
		return (getCycle() - startCycle) / CyclePerMilliSec >= time;
#endif // _MSC_VER
	}

	/// <summary>
	/// 設定時間経過したかを得る
	/// </summary>
	/// <returns>経過していれば true, それ以外は false</returns>
	operator bool() const noexcept { return check(); }

	/// <summary>
	/// 経過時間を取得する(ミリ秒)
	/// </summary>
	/// <returns>計測時間(ミリ秒)</returns>
	inline const long long interval() const noexcept {
#ifdef _MSC_VER
		const auto e = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
#else
		return static_cast<long long int>((getCycle() - startCycle) / CyclePerMilliSec);
#endif // _MSC_VER
	}

};

struct XorShift {
	unsigned int x;
	XorShift() : x(2463534242U) {}
	unsigned int rand() {
		x ^= (x << 13);
		x ^= (x >> 17);
		x ^= (x << 5);
		return x;
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
using BitTable = FixedGrid<bool, N, N>;
using Table = FixedGrid<int, N, N>;
using Robots = array<Robot, M>;

class Simulator {
private:

	const Point goal;
	const Field field;
	const Robots robots;
	const vector<Command> command;

	int score = 0;

	Field table;
	BitTable pass;

	int calc(Robot robot) {

		while (robot.p != goal)
		{
			pass[robot.p] = true;

			if (table[robot.p] != Panel::None)
			{
				if (table[robot.p] == Panel::Block)
				{
					break;
				}
				else
				{
					robot.c = table[robot.p];
				}
			}

			switch (robot.c)
			{
			case Panel::U: robot.p.y = (robot.p.y - 1 + N) % N; break;
			case Panel::D: robot.p.y = (robot.p.y + 1) % N; break;
			case Panel::L: robot.p.x = (robot.p.x - 1 + N) % N; break;
			case Panel::R: robot.p.x = (robot.p.x + 1) % N; break;
			default:
				break;
			}
		}

		if (robot.p == goal)
			return 1000;

		return 0;
	}

	int calc() {

		int add = 0;

		for (const auto& robot : robots)
		{
			add += calc(robot);
		}

		add -= static_cast<int>(command.size()) * 10;

		for (int y = 0; y < N; y++)
		{
			for (int x = 0; x < N; x++)
			{
				add += pass[y][x];
			}
		}

		return add;
	}

public:

	Simulator(const Point _goal, const Field& _field, const Robots& _robots, const vector<Command>& _command)
		: goal(_goal), field(_field), robots(_robots), command(_command) {


		table = field;
		pass.fill(false);
		for (const auto& com : command)
		{
			table[com.p] = com.c;
		}

		score = calc();
	}

	const int getScore() const { return score; }

};

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

	void greedy(Robot robot, Field& signField, Field& passTable, Table& stepTable) const {

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
					/*
					if (maxRange < stepTable[p])
					{
						maxRange = stepTable[p];
						maxPos = p;
					}
					/*/
					if (maxRange < rangeTable[p])
					{
						maxRange = rangeTable[p];
						maxPos = p;
					}
					//*/
				}
			}

			return pair<bool, Point>{maxRange > 0, maxPos};
		};

		list<Robot> passList;
		int baseStep = 0;

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
						passList.push_front(robot);
						robot.p.x = (robot.p.x + d.x + N) % N;
						robot.p.y = (robot.p.y + d.y + N) % N;
						if (robot.p == result.second)
						{
							baseStep = stepTable[result.second] + 1;
							break;
						}
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
							passList.push_front(robot);
							robot.p.x = (robot.p.x + d2.x + N) % N;
							robot.p.y = (robot.p.y + d2.y + N) % N;
							if (robot.p == maxPos)
							{
								baseStep = stepTable[maxPos] + 1;
								break;
							}
						}
					}
					else
					{
						Point nextPos;
						nextPos.x = (robot.p.x + d.x + N) % N;
						nextPos.y = (robot.p.y + d.y + N) % N;

						if (rangeTable[robot.p] > rangeTable[nextPos])
						{
							passList.push_front(robot);
							robot.p = nextPos;
						}
						else
						{
							if (rangeTable[robot.p] > rangeTable[(robot.p.y - 1 + N) % N][robot.p.x])
							{
								signField[robot.p] = Panel::U;
								robot.c = signField[robot.p];
								passList.push_front(robot);
								robot.p.y = (robot.p.y - 1 + N) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[(robot.p.y + 1) % N][robot.p.x])
							{
								signField[robot.p] = Panel::D;
								robot.c = signField[robot.p];
								passList.push_front(robot);
								robot.p.y = (robot.p.y + 1) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[robot.p.y][(robot.p.x - 1 + N) % N])
							{
								signField[robot.p] = Panel::L;
								robot.c = signField[robot.p];
								passList.push_front(robot);
								robot.p.x = (robot.p.x - 1 + N) % N;
							}
							else if (rangeTable[robot.p] > rangeTable[robot.p.y][(robot.p.x + 1) % N])
							{
								signField[robot.p] = Panel::R;
								robot.c = signField[robot.p];
								passList.push_front(robot);
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

		{
			int step = baseStep;
			for (const auto& pass : passList)
			{
				if (passTable[pass.p] == Panel::None)
				{
					passTable[pass.p] = pass.c;
					stepTable[pass.p] = step;
				}
				step++;
			}
		}

	}

	vector<Command> greedy(const Robots& rangeSortRobots, Field& signField, Field& passTable) const {

		Table stepTable(0);

		//for (int i = 0; i < 1; i++)
		for (int i = 0; i < M; i++)
		{
			Robot robot = rangeSortRobots[i];

			greedy(robot, signField, passTable, stepTable);
		}

		vector<Command> answer;
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

		vector<Command> bestAnswer;
		int bestScore = 0;

		//入力順
		{
			Field signField = field;
			Field passTable(Panel::None);
			auto rangeSortRobots = iRobots;
			const auto answer = greedy(rangeSortRobots, signField, passTable);
			int score = Simulator(goal, field, iRobots, answer).getScore();
			if (bestScore < score)
			{
				bestScore = score;
				bestAnswer = answer;
			}
		}

		//遠い順
		{
			Field signField = field;
			Field passTable(Panel::None);
			auto rangeSortRobots = iRobots;
			sort(rangeSortRobots.begin(), rangeSortRobots.end(), [&](const decltype(rangeSortRobots)::reference a, const decltype(rangeSortRobots)::reference b) {return rangeTable[a.p] > rangeTable[b.p]; });
			const auto answer = greedy(rangeSortRobots, signField, passTable);
			int score = Simulator(goal, field, iRobots, answer).getScore();
			if (bestScore < score)
			{
				bestScore = score;
				bestAnswer = answer;
			}
		}
		//近い順
		{
			Field signField = field;
			Field passTable(Panel::None);
			auto rangeSortRobots = iRobots;
			sort(rangeSortRobots.begin(), rangeSortRobots.end(), [&](const decltype(rangeSortRobots)::reference a, const decltype(rangeSortRobots)::reference b) {return rangeTable[a.p] < rangeTable[b.p]; });
			const auto answer = greedy(rangeSortRobots, signField, passTable);
			int score = Simulator(goal, field, iRobots, answer).getScore();
			if (bestScore < score)
			{
				bestScore = score;
				bestAnswer = answer;
			}
		}

		mt19937 randmt;
		MilliSecTimer timer;
		timer.set(chrono::milliseconds(500));

		timer.start();
		while (!timer)
		{
			Field signField = field;
			Field passTable(Panel::None);

			auto rangeSortRobots = iRobots;
			shuffle(rangeSortRobots.begin(), rangeSortRobots.end(), randmt);
			const auto answer = greedy(rangeSortRobots, signField, passTable);
			int score = Simulator(goal, field, iRobots, answer).getScore();
			if (bestScore < score)
			{
				bestScore = score;
				bestAnswer = answer;
			}
		}

		uniform_int_distribution<> rand4(0, 3);
		XorShift randxs;

		timer.set(chrono::milliseconds(2400));
		timer.start();
		while (!timer)
		{
			Point collect = goal;
			Field signField = field;
			Field passTable(Panel::None);

			int c = (randxs.rand() & 0x3) + (randxs.rand() & 0x3);
			switch ((randxs.rand() & 0x3))
			{
			case 0:

				for (int y = 0; y < c; y++)
				{
					int py = (goal.y - 1 - y + N) % N;
					if (field[py][goal.x] == Panel::Block)
					{
						break;
					}
					else
					{
						passTable[py][goal.x] = Panel::D;
						collect.y = py;
					}
				}
				signField[collect] = Panel::D;
				break;
			case 1:

				for (int y = 0; y < c; y++)
				{
					int py = (goal.y + 1 + y + N) % N;
					if (field[py][goal.x] == Panel::Block)
					{
						break;
					}
					else
					{
						passTable[py][goal.x] = Panel::U;
						collect.y = py;
					}
				}
				signField[collect] = Panel::U;
				break;
			case 2:

				for (int x = 0; x < c; x++)
				{
					int px = (goal.x - 1 - x + N) % N;
					if (field[goal.y][px] == Panel::Block)
					{
						break;
					}
					else
					{
						passTable[goal.y][px] = Panel::R;
						collect.x = px;
					}
				}
				signField[collect] = Panel::R;
				break;
			case 3:

				for (int x = 0; x < c; x++)
				{
					int px = (goal.x + 1 + x + N) % N;
					if (field[goal.y][px] == Panel::Block)
					{
						break;
					}
					else
					{
						passTable[goal.y][px] = Panel::L;
						collect.x = px;
					}
				}
				signField[collect] = Panel::L;
				break;
			default:
				break;
			}

			if (collect == goal) continue;

			makeRangeTable(collect);

			auto rangeSortRobots = iRobots;
			shuffle(rangeSortRobots.begin(), rangeSortRobots.end(), randmt);
			const auto answer = greedy(rangeSortRobots, signField, passTable);
			int score = Simulator(goal, field, iRobots, answer).getScore();
			if (bestScore < score)
			{
				bestScore = score;
				bestAnswer = answer;
			}
		}

		cerr << bestScore << endl;

		if (!bestAnswer.empty()) return bestAnswer;
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
