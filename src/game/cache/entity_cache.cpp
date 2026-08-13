#include "entity_cache.hpp"

#include "../cs2_game_engine.hpp" 
#include "../../mem/cs2_memory_offsets.hpp"

bool Cache::Refresh() {
    return Get().RefreshImpl();
}

Snapshot Cache::CopySnapshot() {
    std::lock_guard<std::mutex> lock(Get().mtx);
    return {
        Get().game,
        Get().local,
        Get().players
    };
}

bool Cache::RefreshImpl() {
    auto p = Engine::GetProcess();
    auto client = Engine::GetClient();

    if (!p)
        return false;

    auto now = steady_clock::now();

    
    
    if (!game.Update())
        return false;

    if (now - last < 1ms) 
        return true;

    game.UpdateEntityList();

    std::vector<Player> scan;
    scan.reserve(32);
    for (int i = 0; i < 32; i++) {
        auto player = Player(i, game.entity_list, game.list_entry);

        if (!player.Update())
            continue;

        if (player.localplayer)
            this->local = player;

        scan.push_back(player);
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        players = std::move(scan);

        duration = duration_cast<std::chrono::milliseconds>(last - now);
        last = now;
    }

    return true;
}
