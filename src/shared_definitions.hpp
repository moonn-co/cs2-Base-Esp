#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "libs/imgui/imgui.h"

#include "utils/debug_logger.hpp"
#include "game/types/rgba_color.hpp"
#include "game/types/vector2d.hpp"
#include "game/types/vector3d.hpp"
#include "game/types/view_matrix.hpp"
#include "cfg/active_configuration.hpp"

using namespace std::chrono_literals;
