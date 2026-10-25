#include "entity_cache.hpp"

#include "../cs2_game_engine.hpp"
#include "../../mem/cs2_memory_offsets.hpp"

bool Cache::Refresh() {
    return Get().RefreshImpl();
}

Snapshot Cache::CopySnapshot() {
    auto& self = Get();

    std::lock_guard<std::mutex> lock(self.mtx);

    Snapshot snapshot;
    snapshot.game = self.game;
    snapshot.local = self.local;
    snapshot.players = self.players;

    return snapshot;
}

void Cache::Reset() {
    auto& self = Get();

    std::lock_guard<std::mutex> lock(self.mtx);

    self.players.clear();
    self.local = Player{};
    self.game = Game{};
    self.has_local = false;
    self.last = {};
}

bool Cache::RefreshImpl() {
    auto p = Engine::GetProcess();

    if (!p || !p->handle_)
        return false;

    const auto now = std::chrono::steady_clock::now();

    // Throttle the full entity walk instead of repeating it on every engine
    // tick. The old code refreshed at most once per millisecond, which meant
    // ~40k remote reads per second for no visual benefit.
    const auto interval = std::chrono::milliseconds(cfg::settings::cache_refresh_ms);

    if (this->last.time_since_epoch().count() != 0 && now - last < interval)
        return true;

    this->last = now;

    if (!game.Update())
        return false;

    if (!game.UpdateEntityList())
        return false;

    std::vector<Player> scan;
    scan.reserve(kMaxPlayers);

    Player found_local{};
    bool has_local = false;

    for (int i = 0; i < kMaxPlayers; i++) {
        auto player = Player(i, game.entity_list, game.list_entry);

        if (!player.Update())
            continue;

        if (player.localplayer) {
            found_local = player;
            has_local = true;
        }

        scan.push_back(std::move(player));
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        players = std::move(scan);
        this->has_local = has_local;

        if (has_local)
            this->local = found_local;
    }

    return true;
}
