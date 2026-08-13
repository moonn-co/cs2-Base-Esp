namespace offsets
{
	inline DWORD entityList;
	inline DWORD viewMatrix;
	inline DWORD localPlayerController;

	inline DWORD buildNumber;

	namespace controller {
		constexpr std::ptrdiff_t m_hPawn = 0x6BC;
		constexpr std::ptrdiff_t m_bIsLocalPlayerController = 0x788;
	}

	namespace pawn {
		constexpr std::ptrdiff_t m_vOldOrigin = 0x1390;
		constexpr std::ptrdiff_t m_iHealth = 0x34C;
		constexpr std::ptrdiff_t m_iTeamNum = 0x3EB;

		constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;
		
		constexpr std::ptrdiff_t m_pWeaponServices = 0x11E0;
		constexpr std::ptrdiff_t m_hActiveWeapon = 0x60;
		constexpr std::ptrdiff_t m_AttributeManager = 0x1180;
		constexpr std::ptrdiff_t m_Item = 0x50;
		constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
	}

	namespace bone {
		constexpr std::ptrdiff_t m_modelState = 0x150;
	}

	namespace signatures
	{
		const std::string viewMatrix = "48 8D 0D ?? ?? ?? ?? 48 C1 E0 06";
		const std::string entityList = "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB";
		const std::string localPlayerController = "48 8B 05 ?? ?? ?? ?? 41 89 BE";

		const std::string buildNumber = "89 05 ?? ?? ?? ?? 48 8d 0d ?? ?? ?? ?? ff 15 ?? ?? ?? ?? 48 8b 0d";

	}
}