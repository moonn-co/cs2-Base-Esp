#pragma once

namespace cfg {
	inline bool enabled = true;

	namespace esp {
		inline bool team = true;
		
		inline bool box = true;
		inline bool health = true;
		inline bool skeleton = true;

		namespace flags {
			inline bool weapon = true;
		}

		namespace colors {
			inline color_t box_team{ 1.f, 1.f, 1.f, 0.8f };
			inline color_t box_enemy{ 1.f, 1.f, 1.f, 0.8f };

			inline color_t skeleton_team{ 1.f, 1.f, 1.f, 0.8f };
			inline color_t skeleton_enemy{ 1.f, 1.f, 1.f, 0.8f };
		}
	}

	namespace settings {
		inline bool free_cpu = true;
	}
}