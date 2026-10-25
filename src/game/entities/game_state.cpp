#include "game_state.hpp"

#include <cmath>

#include "../cs2_game_engine.hpp"
#include "../../mem/cs2_memory_offsets.hpp"

bool Game::Update() {
	auto p = Engine::GetProcess();

	if (!p || !p->handle_)
		return false;

	if (!UpdateMatrix()) {
		LOGF(WARNING, "Failed to update view matrix");
		return false;
	}

	return true;
}

bool Game::UpdateMatrix() {
	auto p = Engine::GetProcess();
	auto client = Engine::GetClient();

	if (!p || !client.base || !offsets::viewMatrix)
		return false;

	view_matrix_t matrix{};
	if (!p->read_raw(client.base + offsets::viewMatrix, &matrix, sizeof(matrix)))
		return false;

	// A zeroed / NaN matrix produces garbage screen coordinates, so it is
	// rejected here instead of being drawn as random lines.
	const float* raw = &matrix.matrix[0][0];
	for (int i = 0; i < 16; i++)
	{
		if (!std::isfinite(raw[i]))
			return false;
	}

	if (raw[15] == 0.f && raw[0] == 0.f && raw[5] == 0.f)
		return false; // not in game / not spawned yet

	this->view_matrix = matrix;
	return true;
}

bool Game::UpdateEntityList() {
	auto p = Engine::GetProcess();
	auto client = Engine::GetClient();

	if (!p || !client.base || !offsets::entityList)
		return false;

	this->entity_list = p->read<uintptr_t>(client.base + offsets::entityList);

	if (!this->entity_list)
		return false;

	this->list_entry = p->read<uintptr_t>(this->entity_list + 0x10);

	return this->list_entry != 0;
}
