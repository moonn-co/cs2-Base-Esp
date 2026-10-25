#include "visual_esp.hpp"

#include <algorithm>
#include <cmath>

#include "../cfg/active_configuration.hpp"
#include "../utils/debug_logger.hpp"

namespace
{
    ImU32 ToColorU32(const color_t& color)
    {
        // Explicit conversion: color_t is implicitly convertible both to
        // ImColor and to ImU32, so a direct ImColor(...) call would be ambiguous.
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
    }

    color_t LerpColor(const color_t& from, const color_t& to, float t)
    {
        t = std::clamp(t, 0.f, 1.f);

        return color_t(
            from.r + (to.r - from.r) * t,
            from.g + (to.g - from.g) * t,
            from.b + (to.b - from.b) * t,
            from.a + (to.a - from.a) * t
        );
    }
}

bool Esp::Init() {
    return GetInstance().InitImpl();
}

void Esp::Render() {
    GetInstance().RenderImpl();
}

bool Esp::InitImpl() {
    ImGuiIO& io = ImGui::GetIO();

    // ImGui 1.92 takes the rasterisation size here and the drawing size at
    // PushFont() time, which is why the old single-argument PushFont() call
    // did not compile against the bundled version.
    this->font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", cfg::esp::font_size);

    if (!this->font)
    {
        this->font = io.Fonts->AddFontDefault();
        LOGF(WARNING, "consola.ttf not found - falling back to the built-in font");
    }

    this->text_size_cache.clear();

    LOGF(INFO, "ESP initialized successfully");
    return true;
}

void Esp::RenderImpl() {
    if (!cfg::enabled)
        return;

    auto snapshot = Cache::CopySnapshot();

    // Nothing to project: keep the overlay empty instead of drawing stale boxes.
    if (snapshot.players.empty())
        return;

    const auto& local = snapshot.local;

    ImGuiIO& io = ImGui::GetIO();
    this->display = io.DisplaySize;
    this->draw_list = ImGui::GetBackgroundDrawList();
    this->matrix = snapshot.game.view_matrix;

    if (this->display.x <= 0.f || this->display.y <= 0.f)
        return;

    ImGui::PushFont(this->font, cfg::esp::font_size);

    for (const auto& player : snapshot.players) {
        if (!player.alive || player.localplayer)
            continue;

        if (player.health <= 0 || player.bone_list.empty())
            continue;

        const bool mate = local.index >= 0 && player.team == local.team;

        if (!cfg::esp::team && mate)
            continue;

        RenderPlayer(player, mate);
    }

    ImGui::PopFont();
}

void Esp::RenderPlayer(const Player& player, bool mate) {
    std::pair<Vec2_t, Vec2_t> bounds;
    if (!player.GetBounds(matrix, Vec2_t(display), bounds))
        return;

    const auto& box_color = mate ? cfg::esp::colors::box_team : cfg::esp::colors::box_enemy;

    if (cfg::esp::box) {
        draw_list->AddRect(
            ImVec2(bounds.first),
            ImVec2(bounds.second),
            ToColorU32(box_color),
            0.f,
            0,
            cfg::esp::box_thickness
        );
    }

    if (cfg::esp::skeleton) {
        RenderPlayerBones(player, mate);
    }

    if (cfg::esp::health) {
        RenderHealthBar(player, bounds);
    }

    if (cfg::esp::flags::weapon) {
        RenderWeaponName(player, bounds);
    }
}

void Esp::RenderPlayerBones(const Player& player, bool mate) {
    const auto color = mate ? cfg::esp::colors::skeleton_team : cfg::esp::colors::skeleton_enemy;
    const ImU32 im_color = ToColorU32(color);

    const auto bone_count = player.bone_list.size();

    if (bone_count < static_cast<size_t>(kMaxBones))
        return;

    for (const auto& connection : connections) {
        const int first = connection[0];
        const int second = connection[1];

        if (first < 0 || second < 0 ||
            static_cast<size_t>(first) >= bone_count || static_cast<size_t>(second) >= bone_count)
            continue;

        Vec2_t bone1_screen, bone2_screen;

        if (!matrix.wts(player.bone_list[first].pos, Vec2_t(display), bone1_screen) ||
            !matrix.wts(player.bone_list[second].pos, Vec2_t(display), bone2_screen))
            continue;

        draw_list->AddLine(
            ImVec2(bone1_screen),
            ImVec2(bone2_screen),
            im_color,
            cfg::esp::skeleton_thickness
        );
    }
}

void Esp::RenderHealthBar(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds) {
    const float x_end = bounds.first.x - cfg::esp::health_bar_gap;
    const float x_start = x_end - cfg::esp::health_bar_width;
    const float y_start = bounds.first.y;
    const float y_end = bounds.second.y;

    const float height = y_end - y_start;

    if (!std::isfinite(height) || height <= 1.f)
        return;

    const float health_ratio = std::clamp(player.health / 100.0f, 0.0f, 1.0f);
    const float filled_height = height * health_ratio;

    // Green above 60 HP, yellow towards 30, red below - readable at a glance.
    color_t bar_color = cfg::esp::colors::health_high;
    if (health_ratio < 0.3f)
        bar_color = cfg::esp::colors::health_low;
    else if (health_ratio < 0.6f)
        bar_color = LerpColor(cfg::esp::colors::health_low, cfg::esp::colors::health_mid, (health_ratio - 0.3f) / 0.3f);

    draw_list->AddRectFilled(
        ImVec2(x_start, y_end - filled_height),
        ImVec2(x_end, y_end),
        ToColorU32(bar_color)
    );

    draw_list->AddRect(
        ImVec2(x_start, y_start),
        ImVec2(x_end, y_end),
        ToColorU32(cfg::esp::colors::health_outline)
    );
}

void Esp::RenderWeaponName(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds) {
    if (player.weapon.name.empty() || player.weapon.name == "unknown" || player.weapon.name == "Invalid")
        return;

    const auto& weapon_size = GetCachedTextSize(player.weapon.name);
    const float center_x = (bounds.first.x + bounds.second.x) * 0.5f - weapon_size.x * 0.5f;

    draw_list->AddText(
        ImVec2(center_x, bounds.second.y + 4.0f),
        ToColorU32(cfg::esp::colors::text),
        player.weapon.name.c_str()
    );
}

const ImVec2& Esp::GetCachedTextSize(const std::string& text) {
    // The cache used to hold a single entry, so with more than one visible
    // player it was invalidated on every call. Now each name is measured once.
    const auto it = text_size_cache.find(text);

    if (it != text_size_cache.end())
        return it->second;

    const ImVec2 size = ImGui::CalcTextSize(text.c_str());

    if (text_size_cache.size() > 256)
        text_size_cache.clear();

    return text_size_cache.emplace(text, size).first->second;
}
