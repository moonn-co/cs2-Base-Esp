#include "cs2_player.hpp"

#include <cmath>

#include "cs2_weapon.hpp"
#include "../cs2_game_engine.hpp"
#include "../../mem/cs2_memory_offsets.hpp"

namespace
{
    constexpr int kMaxHealth = 1000;
}

bool Player::Update() {
    auto p = Engine::GetProcess();

    if (!p || !p->handle_)
        return false;

    // Reset the per-tick state: the previous version kept appending to
    // bone_list on every cache refresh, so the vector grew forever.
    this->bone_list.clear();
    this->alive = false;
    this->health = 0;
    this->team = 0;
    this->localplayer = false;
    this->pawn = 0;
    this->controller = 0;
    this->pos = Vec3_t{};
    this->weapon = Weapon{};

    if (!this->entity_list || !this->list_entry)
        return false;

    if (!GetController())
        return false;

    if (!GetPawn())
        return false;

    if (!UpdateController())
        return false;

    return UpdatePawn();
}

bool Player::GetController() {
    auto p = Engine::GetProcess();

    // The old code dereferenced list_entry unconditionally, so every call on an
    // uninitialised Player() read from a near-null address.
    this->controller = p->read<uintptr_t>(list_entry + (index + 1) * 0x70);

    return this->controller != 0;
}

bool Player::GetPawn() {
    auto p = Engine::GetProcess();

    auto entity_pawn_address = p->read<uintptr_t>(controller + offsets::controller::m_hPawn);

    if (!entity_pawn_address)
        return false;

    this->pawn_controller_addr = entity_pawn_address;

    auto entity_pawn_list_entry = p->read<uintptr_t>(this->entity_list + 0x10 + 0x8 * ((entity_pawn_address & 0x7FFF) >> 9));

    if (!entity_pawn_list_entry)
        return false;

    this->pawn = p->read<uintptr_t>(entity_pawn_list_entry + 0x70 * (entity_pawn_address & 0x1FF));

    return this->pawn != 0;
}

bool Player::UpdateController() {
    auto p = Engine::GetProcess();

    this->localplayer = p->read<bool>(controller + offsets::controller::m_bIsLocalPlayerController);

    return true;
}

bool Player::UpdatePawn() {
    auto p = Engine::GetProcess();

    this->health = p->read<int>(pawn + offsets::pawn::m_iHealth);
    this->alive = health > 0 && health <= kMaxHealth;

    if (health < 0 || health > kMaxHealth)
    {
        // Reported as a warning instead of FATAL: one bogus entity (bots, spec
        // targets, respawn window) must not look like a broken offset dump.
        LOGF(WARNING, "Suspicious health value ({}) - pawn layout may have changed", health);
        return true;
    }

    if (!alive)
        return true;

    this->pos = p->read<Vec3_t>(pawn + offsets::pawn::m_vOldOrigin);

    if (this->pos.zero() || !std::isfinite(this->pos.x) || !std::isfinite(this->pos.y) || !std::isfinite(this->pos.z))
        return false;

    this->team = p->read<uint8_t>(pawn + offsets::pawn::m_iTeamNum);

    if (!UpdateSkeleton()) {
        LOGF(VERBOSE, "Failed to update skeleton");
        return false;
    }

    if (!UpdateWeapon()) {
        LOGF(VERBOSE, "Failed to update weapon");
        return false;
    }

    return true;
}

bool Player::UpdateSkeleton() {
    auto p = Engine::GetProcess();

    auto game_scene = p->read<uintptr_t>(this->pawn + offsets::pawn::m_pGameSceneNode);

    if (!game_scene)
        return false;

    auto bone_array = p->read<uintptr_t>(game_scene + (offsets::bone::m_modelState + 0x80));

    if (!bone_array)
        return false;

    if (!p->read_raw(bone_array, bones, sizeof(bones)))
        return false;

    // Sized once, then filled in place: no per-frame heap churn.
    this->bone_list.resize(kMaxBones);

    for (int i = 0; i < kMaxBones; i++)
    {
        const auto& pos = bones[i].pos;

        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z))
            return false;

        this->bone_list[i].pos = pos;
    }

    return true;
}

bool Player::UpdateWeapon() {
    auto p = Engine::GetProcess();

    auto weapon_services = p->read<uintptr_t>(this->pawn + offsets::pawn::m_pWeaponServices);

    if (!weapon_services)
        return false;

    auto active_weapon_index = p->read<int>(weapon_services + offsets::pawn::m_hActiveWeapon);

    if (!active_weapon_index)
        return false;

    auto weapon = Weapon(this->entity_list, active_weapon_index);

    if (!weapon.Update())
        return false;

    this->weapon = std::move(weapon);

    return true;
}

bool Player::GetBounds(const view_matrix_t& matrix, const Vec2_t& screen, std::pair<Vec2_t, Vec2_t>& bounds) const {
    Vec2_t origin;
    const bool feet_visible = matrix.wts(this->pos, screen, origin);

    Vec3_t pos_top;

    if (this->bone_list.size() <= static_cast<size_t>(bone_index::head))
        pos_top = this->pos + Vec3_t(0, 0, 65.f);
    else
        pos_top = this->bone_list[bone_index::head].pos;

    Vec2_t top;
    const bool head_visible = matrix.wts(pos_top, screen, top);

    // Both anchor points must project on screen. The old code returned true when
    // either anchor was visible, which produced boxes built from garbage
    // coordinates whenever the head or the feet were behind the camera.
    if (!feet_visible || !head_visible)
        return false;

    float height = origin.y - top.y;

    if (!std::isfinite(height) || height <= 1.0f)
        return false;

    float width = height / 2.4f;

    top.x -= width / 2;
    origin.x += width / 2;

    top.y -= width / 4;

    bounds = { top, origin };

    return true;
}
