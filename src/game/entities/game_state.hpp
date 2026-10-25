#pragma once

#include <cstdint>
#include "../types/view_matrix.hpp"

class Game {
public:
    Game() = default;

    bool Update();
    bool UpdateMatrix();
    bool UpdateEntityList();

public:
    view_matrix_t view_matrix{};

    uintptr_t entity_list = 0;
    uintptr_t list_entry = 0;
};
