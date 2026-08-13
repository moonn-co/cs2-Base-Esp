#pragma once

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
    ImGuiIO io;
    ImFont* font;
    ImDrawList* d;
    view_matrix_t matrix;
    
    struct TextSizeCache {
        std::string text;
        ImVec2 size;
    };
    TextSizeCache cached_weapon_size;

    Esp() {};

    static Esp& GetInstance()
    {
        static Esp i{};
        return i;
    }

    bool InitImpl();
    void RenderImpl();

    void RenderPlayer(const Player& player, bool mate = false);
    void RenderPlayerBones(const Player& player, bool mate = false);
    void RenderHealthBar(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds);
    void RenderWeaponName(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds);
    
    inline ImVec2 GetCachedTextSize(const std::string& text);
};