#pragma once

#include <chrono>
#include <mutex>
#include <vector>

#include "../entities/game_state.hpp"
#include "../entities/cs2_player.hpp"

inline constexpr int kMaxPlayers = 64;

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
	static void Reset();
private:
	std::mutex mtx;
	std::chrono::steady_clock::time_point last{};
	bool has_local = false;
private:
	bool RefreshImpl();
};
