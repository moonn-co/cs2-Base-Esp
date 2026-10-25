#pragma once
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <string>

// Schema offsets below were taken from the public dump
// (a2x/cs2-dumper, client_dll.hpp, generated 2026-10-23).
// Everything inside `namespace signatures` is re-resolved at runtime, so a game
// patch only requires updating this file when the struct layout changes.
namespace offsets
{
	// Filled at runtime by the signature scanner.
	inline DWORD entityList = 0;
	inline DWORD viewMatrix = 0;
	inline DWORD localPlayerController = 0;

	inline DWORD buildNumber = 0;

	namespace controller {
		constexpr std::ptrdiff_t m_hPawn = 0x6BC;                        // CHandle<C_BasePlayerPawn>
		constexpr std::ptrdiff_t m_bIsLocalPlayerController = 0x790;     // bool
	}

	namespace pawn {
		constexpr std::ptrdiff_t m_vOldOrigin = 0x14A4;   // Vector
		constexpr std::ptrdiff_t m_iHealth = 0x34C;       // int32
		constexpr std::ptrdiff_t m_iTeamNum = 0x3E7;      // uint8

		constexpr std::ptrdiff_t m_pGameSceneNode = 0x330; // CGameSceneNode*

		constexpr std::ptrdiff_t m_pWeaponServices = 0x12F0;   // CPlayer_WeaponServices*
		constexpr std::ptrdiff_t m_hActiveWeapon = 0x60;       // CHandle<C_BasePlayerWeapon>
		constexpr std::ptrdiff_t m_AttributeManager = 0x1290;  // C_AttributeContainer (C_EconEntity)
		constexpr std::ptrdiff_t m_Item = 0x50;                // C_EconItemView (C_AttributeContainer)
		constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA; // uint16 (C_EconItemView)
	}

	namespace bone {
		constexpr std::ptrdiff_t m_modelState = 0x140; // CModelState (CSkeletonInstance)

		// Pointer to the bone array inside CModelState. This member is not part
		// of the networked schema, so the dumper does not list it; it has stayed
		// at +0x80 relative to m_modelState.
		constexpr std::ptrdiff_t m_pBoneArray = 0x80;
	}

	namespace list {
		// Unpacking of a CHandle into the entity chunk table:
		//   base  = *(entity_list + chunks_base)
		//   chunk = *(base + 0x8 * ((handle & handle_index_mask) >> chunk_shift))
		//   entry = *(chunk + chunk_stride * (handle & entry_mask))
		constexpr std::ptrdiff_t chunks_base = 0x10;
		constexpr std::ptrdiff_t chunk_stride = 0x70;
		constexpr uint32_t handle_index_mask = 0x7FFF;
		constexpr uint32_t chunk_shift = 9;
		constexpr uint32_t entry_mask = 0x1FF;

		constexpr uint32_t max_entities = 64;
	}

	namespace signatures
	{
		inline const std::string viewMatrix = "48 8D 0D ?? ?? ?? ?? 48 C1 E0 06";
		inline const std::string entityList = "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB";
		inline const std::string localPlayerController = "48 8B 05 ?? ?? ?? ?? 41 89 BE";

		inline const std::string buildNumber = "89 05 ?? ?? ?? ?? 48 8d 0d ?? ?? ?? ?? ff 15 ?? ?? ?? ?? 48 8b 0d";
	}
}
