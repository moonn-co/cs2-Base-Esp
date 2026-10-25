#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "player_bones.hpp"
#include "cs2_weapon.hpp"
#include "../types/vector2d.hpp"
#include "../types/vector3d.hpp"
#include "../types/view_matrix.hpp"

inline constexpr int kMaxBones = 30;

class Player {
public:
    Player() {}
    Player(int index, uintptr_t el, uintptr_t le)
        : index(index), entity_list(el), list_entry(le){}

    bool Update();
    bool GetBounds(const view_matrix_t& matrix, const Vec2_t& screen, std::pair<Vec2_t, Vec2_t>& bounds) const;
public:
    int index = -1;

    Vec3_t pos{};

    int team = 0;
    int health = 0;

    bool alive = false;
    bool localplayer = false;

    Weapon weapon;

    std::vector<bone_pos> bone_list;
private:
    uintptr_t list_entry = 0;
    uintptr_t entity_list = 0;

    uintptr_t pawn = 0;
    uintptr_t controller = 0;
    uintptr_t pawn_controller_addr = 0;

    bone_data bones[kMaxBones]{};
private:
    bool GetPawn();
    bool GetController();

    bool UpdatePawn();
    bool UpdateController();
    bool UpdateWeapon();
    bool UpdateSkeleton();
};
