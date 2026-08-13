#pragma once

#include "player_bones.hpp"
#include "cs2_weapon.hpp"
#include "../types/vector2d.hpp"
#include "../types/vector3d.hpp"
#include "../types/view_matrix.hpp"

class Player {
public:
    Player() {}
    Player(int index, uintptr_t el, uintptr_t le) 
        : index(index), entity_list(el), list_entry(le){}

    bool Update();
    bool GetBounds(view_matrix_t matrix, Vec2_t size, std::pair<Vec2_t, Vec2_t>& bounds) const;
public:
    int8_t index = -1; 

    Vec3_t pos;

    int team = 0;
    int health = 0;

    bool alive = false;
    bool localplayer = false;

    Weapon weapon;

    std::vector<bone_pos> bone_list;
private:
    uintptr_t list_entry;
    uintptr_t entity_list;

    uintptr_t pawn;
    uintptr_t controller;
    uintptr_t pawn_controller_addr;
    
    bone_data bones[30]{};
private:
    bool GetPawn();
    bool GetController();

    bool UpdatePawn();
    bool UpdateController();
    bool UpdateWeapon();
    bool UpdateSkeleton();
};