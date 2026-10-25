#pragma once

#include <cstdint>
#include <string>

#include "../types/weapon_ids.hpp"

class Weapon
{
public:
	Weapon(uintptr_t entity_list, int slot_index)
		: slot_index(slot_index), entity_list(entity_list) {}
	Weapon()
		: item_index(-1), name("Invalid"), slot_index(0), entity_list(0) { }

	bool Update();
	bool valid() const { return item_index > 0; }
public:
	short item_index = -1;
	std::string name;

private:
	const char* ToString() const;

	int slot_index = 0;
	uintptr_t entity_list = 0;
};
