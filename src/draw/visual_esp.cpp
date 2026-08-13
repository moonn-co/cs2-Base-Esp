#include "visual_esp.hpp"

bool Esp::Init() {
	return GetInstance().InitImpl();
}

void Esp::Render() {
    return GetInstance().RenderImpl();
}

bool Esp::InitImpl() {
	auto& io = ImGui::GetIO();

	ImFontConfig cfg{};
	cfg.FontDataOwnedByAtlas = false;
	this->font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 12.0f, &cfg);

	cached_weapon_size.text = "";
	cached_weapon_size.size = ImVec2(0, 0);

	LOGF(INFO, "ESP initialized successfully");
	return true;
}

void Esp::RenderImpl() {
	if (!cfg::enabled)
		return;

	auto snapshot = Cache::CopySnapshot();
	auto& local = snapshot.local;
	auto& players = snapshot.players;
	
	ImGui::PushFont(this->font);

	this->io = ImGui::GetIO();
	this->d = ImGui::GetBackgroundDrawList();
	this->matrix = snapshot.game.view_matrix;

	for (const auto& player : players) {
		if (!player.alive || player.localplayer)
			continue;

		const bool mate = player.team == local.team;

		if (!cfg::esp::team && mate)
			continue;

		RenderPlayer(player, mate);
	}

	ImGui::PopFont();
}

void Esp::RenderPlayer(const Player& player, bool mate) {
	std::pair<Vec2_t, Vec2_t> bounds;
	if (!player.GetBounds(matrix, io.DisplaySize, bounds))
		return;

	const auto& box_color = mate ? cfg::esp::colors::box_team : cfg::esp::colors::box_enemy;
	const auto& skeleton_color = mate ? cfg::esp::colors::skeleton_team : cfg::esp::colors::skeleton_enemy;

	if (cfg::esp::box) {
		d->AddRect(bounds.first, bounds.second, ImColor(box_color));
	}

	if (cfg::esp::skeleton) {
		RenderPlayerBones(player, mate);
	}

	if (cfg::esp::health) {
		RenderHealthBar(player, bounds);
	}

	if (cfg::esp::flags::weapon) {
		RenderWeaponName(player, bounds);
	}
}

void Esp::RenderPlayerBones(const Player& player, bool mate) {
	const auto color = mate ? cfg::esp::colors::skeleton_team : cfg::esp::colors::skeleton_enemy;
	const ImU32 im_color = ImColor(color);
	
	const auto bone_count = player.bone_list.size();
	
	if (bone_count < 24)
		return;

	for (int i = 0; i < 17; ++i) {
		const int first = connections[i][0];
		const int second = connections[i][1];

		if (first >= bone_count || second >= bone_count)
			continue;

		const auto& bone1 = player.bone_list[first];
		const auto& bone2 = player.bone_list[second];

		Vec2_t scb1, scb2;
		if (!matrix.wts(bone1.pos, io.DisplaySize, scb1) ||
			!matrix.wts(bone2.pos, io.DisplaySize, scb2))
			continue;

		d->AddLine(scb1, scb2, im_color, 1.5f);
	}
}

void Esp::RenderHealthBar(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds) {
	const float x_start = bounds.first.x - 6.0f;
	const float x_end = x_start - 3.0f;
	const float y_start = bounds.first.y;
	const float y_end = bounds.second.y;

	const float height = y_end - y_start;
	const float health_ratio = std::max(0.0f, std::min(1.0f, player.health / 100.0f));
	const float filled_height = height * health_ratio;

	d->AddRectFilled(
		ImVec2(x_start, y_end - filled_height),
		ImVec2(x_end, y_end),
		IM_COL32(100, 255, 100, 255)
	);

	d->AddRect(
		ImVec2(x_start, y_start),
		ImVec2(x_end, y_end),
		IM_COL32(0, 0, 0, 200)
	);
}

void Esp::RenderWeaponName(const Player& player, const std::pair<Vec2_t, Vec2_t>& bounds) {
	if (player.weapon.name.empty()) 
		return;

	const auto weapon_size = GetCachedTextSize(player.weapon.name);
	const float center_x = (bounds.first.x + bounds.second.x) * 0.5f - weapon_size.x * 0.5f;

	d->AddText(
		Vec2_t(center_x, bounds.second.y + 8.0f),
		IM_COL32(255, 255, 255, 255),
		player.weapon.name.c_str()
	);
}

inline ImVec2 Esp::GetCachedTextSize(const std::string& text) {
	if (cached_weapon_size.text != text) {
		cached_weapon_size.text = text;
		cached_weapon_size.size = ImGui::CalcTextSize(text.c_str());
	}
	return cached_weapon_size.size;
}