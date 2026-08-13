#pragma once

#define NOMINMAX
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <windows.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <format>
#include <mutex>
#include <iostream>

#include "libs/imgui/imgui.h"

#include "utils/debug_logger.hpp"
#include "game/types/rgba_color.hpp"
#include "game/types/vector2d.hpp"
#include "game/types/vector3d.hpp"
#include "game/types/view_matrix.hpp"
#include "cfg/active_configuration.hpp"

using namespace std::chrono_literals;
