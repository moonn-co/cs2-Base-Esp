#pragma once
#include "../entities/game_state.hpp"
#include "../entities/cs2_player.hpp"

using namespace std::chrono;

struct Snapshot {
	Game game;
	Player local;
	std::vector<Player> players;
};

class Cache {
public:
	Game game;
	Player local;
	std::vector<Player> players;
public:
	static Cache& Get()
	{
		static Cache instance{};
		return instance;
	}

	static Snapshot CopySnapshot();

	static bool Refresh();
private:
	std::mutex mtx;
	milliseconds duration{1};
	steady_clock::time_point last{};
private:
	bool RefreshImpl();
};