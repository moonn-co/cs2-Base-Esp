#pragma once

#include "../game/types/rgba_color.hpp"

namespace cfg {
	inline bool enabled = true;

	namespace esp {
		// Draw teammates as well as enemies.
		inline bool team = true;

		inline bool box = true;
		inline bool health = true;
		inline bool skeleton = true;

		inline float box_thickness = 1.0f;
		inline float skeleton_thickness = 1.5f;
		inline float health_bar_width = 3.0f;
		inline float health_bar_gap = 5.0f;
		inline float font_size = 13.0f;

		namespace flags {
			inline bool weapon = true;
		}

		namespace colors {
			inline color_t box_team{ 0.35f, 0.85f, 1.00f, 0.90f };
			inline color_t box_enemy{ 1.00f, 0.25f, 0.25f, 0.90f };

			inline color_t skeleton_team{ 0.35f, 0.85f, 1.00f, 0.90f };
			inline color_t skeleton_enemy{ 1.00f, 0.25f, 0.25f, 0.90f };

			inline color_t health_high{ 0.30f, 1.00f, 0.30f, 1.00f };
			inline color_t health_mid{ 1.00f, 0.85f, 0.20f, 1.00f };
			inline color_t health_low{ 1.00f, 0.20f, 0.20f, 1.00f };

			inline color_t health_outline{ 0.00f, 0.00f, 0.00f, 0.80f };
			inline color_t text{ 1.00f, 1.00f, 1.00f, 1.00f };
		}
	}

	namespace settings {
		// When false the engine thread spins without a delay cap (useful while
		// debugging, expensive in normal use).
		inline bool free_cpu = true;

		// Entity cache refresh interval. Going below ~2 ms only burns CPU,
		// because the overlay cannot render faster than the monitor anyway.
		inline int cache_refresh_ms = 2;
	}
}
