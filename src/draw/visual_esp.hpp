#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "../libs/imgui/imgui.h"
#include "../game/cache/entity_cache.hpp"
#include "../game/types/vector2d.hpp"
#include "../game/types/view_matrix.hpp"

class Esp {
public:
    ~Esp() = default;
    Esp(const Esp&) = delete;
    Esp(Esp&&) = delete;
    Esp& operator=(const Esp&) = delete;
    Esp& operator=(Esp&&) = delete;

    static bool Init();
    static void Render();

private:
    ImFont* font = nullptr;
    ImDrawList* draw_list = nullptr;
    ImVec2 display{};
    view_matrix_t matrix{};
    std::unordered_map<std::string, ImVec2> text_size_cache;

    Esp() = default;

    static Esp& GetInstance()
    {
        static Esp i{};
        return i;
    }

    bool InitImpl();
    void RenderImpl();

    void RenderPlayer(const Player& player, bool mate);
    void RenderPlayerBones(const Player& player, bool mate);
    void RenderHealthBar(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds);
    void RenderWeaponName(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds);

    const ImVec2& GetCachedTextSize(const std::string& text);
};
