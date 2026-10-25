#include "cs2_weapon.hpp"

#include "../cs2_game_engine.hpp"
#include "../../mem/cs2_memory_offsets.hpp"

bool Weapon::Update() {
	auto p = Engine::GetProcess();

	if (!p || !p->handle_)
		return false;

	if (!entity_list || slot_index <= 0)
		return false;

	const auto handle = static_cast<uint32_t>(slot_index);

	uintptr_t bucket_ptr = p->read<uintptr_t>(
		entity_list + offsets::list::chunks_base +
		0x8 * ((handle & offsets::list::handle_index_mask) >> offsets::list::chunk_shift)
	);
	if (!bucket_ptr)
		return false;

	uintptr_t weapon_ptr = p->read<uintptr_t>(bucket_ptr + offsets::list::chunk_stride * (handle & offsets::list::entry_mask));
	if (!weapon_ptr)
		return false;

	this->item_index = p->read<short>(weapon_ptr + offsets::pawn::m_AttributeManager + offsets::pawn::m_Item + offsets::pawn::m_iItemDefinitionIndex);

	// The definition index is a small enum; anything else means a stale pointer.
	if (this->item_index <= 0 || this->item_index > 0x1000)
	{
		this->item_index = -1;
		return false;
	}

	this->name = ToString();

	return true;
}

const char* Weapon::ToString() const
{
	switch (this->item_index)
	{
	case weapon_deagle: return "Deagle";
	case weapon_elite: return "Dual Berettas";
	case weapon_fiveseven: return "Five-Seven";
	case weapon_glock: return "Glock-18";
	case weapon_ak47: return "AK-47";
	case weapon_aug: return "AUG";
	case weapon_awp: return "AWP";
	case weapon_famas: return "FAMAS";
	case weapon_g3sg1: return "G3SG1";
	case weapon_galilar: return "Galil AR";
	case weapon_m249: return "M249";
	case weapon_m4a1: return "M4A1";
	case weapon_mac10: return "MAC-10";
	case weapon_p90: return "P90";
	case weapon_zone_repulsor: return "zone_repulsor";
	case weapon_mp5sd: return "MP5-SD";
	case weapon_ump45: return "UMP-45";
	case weapon_xm1014: return "XM1014";
	case weapon_bizon: return "PP-Bizon";
	case weapon_mag7: return "MAG-7";
	case weapon_negev: return "Negev";
	case weapon_sawedoff: return "Sawed-Off";
	case weapon_tec9: return "Tec-9";
	case weapon_taser: return "Zeus x27";
	case weapon_hkp2000: return "P2000";
	case weapon_mp7: return "MP7";
	case weapon_mp9: return "MP9";
	case weapon_nova: return "Nova";
	case weapon_p250: return "P250";
	case weapon_shield: return "shield";
	case weapon_scar20: return "SCAR-20";
	case weapon_sg556: return "SG 556";
	case weapon_ssg08: return "SSG 08";
	case weapon_knifegg: return "knifegg";
	case weapon_knife: return "Knife";
	case weapon_flashbang: return "Flashbang";
	case weapon_hegrenade: return "HE Grenade";
	case weapon_smokegrenade: return "Smoke Grenade";
	case weapon_molotov: return "Molotov";
	case weapon_decoy: return "Decoy";
	case weapon_incgrenade: return "Incendiary Grenade";
	case weapon_c4: return "C4";
	case item_kevlar: return "Kevlar";
	case item_assaultsuit: return "Kevlar with helmet";
	case item_heavyassaultsuit: return "heavyassaultsuit";
	case item_nvg: return "nvg";
	case item_defuser: return "Defuser";
	case item_cutters: return "cutters";
	case weapon_healthshot: return "Healthshot";
	case weapon_knife_t: return "Knife";
	case weapon_m4a1_silencer: return "M4A1-S";
	case weapon_usp_silencer: return "USP-S";
	case weapon_cz75a: return "CZ75A";
	case weapon_revolver: return "Revolver";
	case weapon_tagrenade: return "tagrenade";
	case weapon_fists: return "Fists";
	case weapon_breachcharge: return "breachcharge";
	case weapon_tablet: return "tablet";
	case weapon_melee: return "melee";
	case weapon_axe: return "axe";
	case weapon_hammer: return "hammer";
	case weapon_spanner: return "spanner";
	case weapon_knife_ghost: return "knife_ghost";
	case weapon_firebomb: return "firebomb";
	case weapon_diversion: return "diversion";
	case weapon_frag_grenade: return "Grenade";
	case weapon_snowball: return "snowball";
	case weapon_bumpmine: return "bumpmine";
	case weapon_knife_bayonet: return "Bayonet";
	case weapon_knife_css: return "Classic Knife";
	case weapon_knife_flip: return "Flip Knife";
	case weapon_knife_gut: return "Gut Knife";
	case weapon_knife_karambit: return "Karambit";
	case weapon_knife_m9_bayonet: return "M9 Bayonet";
	case weapon_knife_tactical: return "Huntsman Knife";
	case weapon_knife_falchion: return "Falchion Knife";
	case weapon_knife_survival_bowie: return "Bowie Knife";
	case weapon_knife_butterfly: return "Butterfly Knife";
	case weapon_knife_push: return "Shadow Daggers";
	case weapon_knife_cord: return "Paracord Knife";
	case weapon_knife_canis: return "Survival Knife";
	case weapon_knife_ursus: return "Ursus Knife";
	case weapon_knife_gypsy_jackknife: return "Navaja Knife";
	case weapon_knife_outdoor: return "Nomad Knife";
	case weapon_knife_stiletto: return "Stiletto Knife";
	case weapon_knife_widowmaker: return "Talon Knife";
	case weapon_knife_skeleton: return "Skeleton Knife";
	case weapon_knife_kukri: return "Kukri Knife";
	default: return "unknown";
	}
}
