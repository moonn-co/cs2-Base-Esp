# Changelog

All changes below were made during a full read-through of the project. The goal was to fix the defects found in the
original code without changing what the cheat does: it stays an external, read-only ESP.

## Correctness

* **`Player::bone_list` grew forever.** `UpdateSkeleton()` used `push_back` on a member vector without clearing it, so
  every cache refresh appended 30 more bones per player and the vector was copied into every snapshot. It is now
  `clear()`-ed at the start of `Player::Update()` and filled with `resize()` + indexed assignment.
* **Both projection anchors are now required.** `Player::GetBounds()` returned `true` when *either* the feet or the head
  projected on screen, so a player half behind the camera produced a box from garbage coordinates. It now requires
  both and rejects non-finite / degenerate heights.
* **`Weapon::ToString()` could return a stale pointer.** The function returned `const char*` into a temporary; it now
  returns a string literal / `std::string` member and validates `m_iItemDefinitionIndex` before using it.
* **`Window::SetBounds()` assigned the parameter to itself** (`bounds = bounds;`), leaving the cached rect untouched.
* **`view_matrix_t::wts()`** built a `RECT` from floats, ignored its `margin` (static, never written) and used a
  hardcoded centre. It now uses the real screen size, rejects `w <= 0.01` and non-finite results.
* **`Module32FirstW` was never called.** `pProcess::GetModule()` started the enumeration with `Module32NextW`, which is
  undefined behaviour and compared a zeroed entry first.
* **`FindSignature()`** read up to 256 MB per scan (or the whole module size) through a deprecated
  `ReadProcessMemory`, ignored the number of bytes read, and could index past the mapping (`data[i + j]`). It is now a
  bounded, validated scan through the same `NtReadVirtualMemory` primitive the rest of the code uses.
* **`Cache::RefreshImpl()`** computed `duration_cast<milliseconds>(last - now)` (always negative, unused) and walked all
  32 slots every millisecond. The entity walk is now throttled by `cfg::settings::cache_refresh_ms` (2 ms default).
* **`Renderer::HandleState()` was only called while the game window had focus**, so `END` did nothing when the overlay
  or any other window was in the foreground. It is now polled on every iteration.
* **`Renderer::IsOpen()` always returned `false`**, `Destroy()` only flipped a flag and the renderer never released its
  DX11 objects. Shutdown now shuts ImGui down, releases the device and destroys the window.
* **`Window::DestroyDevice()` dereferenced every pointer unconditionally** and crashed when initialisation stopped half
  way through; objects are now released only when present.
* **`Engine::Thread()` never exited** and busy-spun when `free_cpu` was `false`. The thread now checks the process
  handle, sleeps to a fixed tick and is joined by the new `Engine::Shutdown()`.
* **`LOGF(FATAL, ...)` on a suspicious health value** turned a normal respawn/spectator entity into a fatal-looking
  message; it is a `WARNING` now and the entity is skipped.
* **`wm_keyup` handler in `window_procedure`** parsed `wParam` as if it contained a scan code and did nothing; removed.
  `WM_DESTROY` now posts `WM_QUIT`, the message loop can actually terminate.

## Robustness / hygiene

* `cs2_game_engine.hpp` was missing `<memory>` (used `std::shared_ptr`), `player_bones.hpp` used `DWORD` without
  including `Windows.h`, `debug_logger.hpp` used `std::string` in the release branch without `<string>`,
  `process_memory.hpp` used `PULONG` without `<Windows.h>` included first. Fixed.
* `pMemory` resolved `NtReadVirtualMemory`/`NtWriteVirtualMemory` on **every** read. It is now a singleton resolved
  once; the `NtWriteVirtualMemory` import was removed together with `write`, `write_bytes`, `Allocate()` and
  `FindCodeCave()` — nothing in the project used them and keeping them contradicted the "read-only" design.
* `OpenProcess` no longer requests `PROCESS_VM_WRITE | PROCESS_VM_OPERATION`; it asks for `PROCESS_VM_READ |
  PROCESS_QUERY_INFORMATION`.
* `margins = { -1 }` passed an uninitialised `cy` to `DwmExtendFrameIntoClientArea`; now `{ -1, -1, -1, -1 }`.
* `Esp` cached a single text size, so with more than one visible player the measurement was recomputed on every draw
  call; it is now a small hash map keyed by the weapon name.
* `ImGui::PushFont(font)` is a compile error against the bundled ImGui 1.92 (`PushFont(font, size)`), and
  `ImColor(color_t)` became ambiguous once `color_t` gained an `ImU32` conversion — both were adjusted.
* The scanner allocated its read buffer before checking whether the pattern was empty (leak on early return), and
  allocated a fresh block buffer per read; the buffer is now `thread_local` and reused.
* Reading memory that is not committed/readable aborted the whole signature scan; `ScanMemory()` now walks readable
  regions only.
* `vector2d.hpp`: `operator==(float)` compared `x == y && y == o`, there was no `const` conversion to `float*`, and
  `dist_to()` was not `const`.
* `vector3d.hpp`: `__forceinline` is MSVC-only and is now behind a `CS2_FORCEINLINE` macro; the user-provided copy
  assignment operator is defaulted (it produced `-Wdeprecated-copy`).
* Small clean-ups: unused parameters/members, `read_multi()` underflow on an empty offset list, missing
  `INVALID_HANDLE_VALUE` checks after `CreateToolhelp32Snapshot`, `#include` order, comments explaining the
  non-obvious offsets and magic numbers.

## Offsets refresh (dump of 2026-10-23)

Struct offsets were stale (they matched an early-2024 client build) and have been replaced with the values from the
public schema dump [`a2x/cs2-dumper`](https://github.com/a2x/cs2-dumper):

| Field | Old | New |
| --- | --- | --- |
| `controller::m_bIsLocalPlayerController` | `0x788` | `0x790` |
| `pawn::m_iTeamNum` | `0x3EB` | `0x3E7` |
| `pawn::m_pWeaponServices` | `0x11E0` | `0x12F0` |
| `pawn::m_AttributeManager` | `0x1180` | `0x1290` |
| `pawn::m_vOldOrigin` | `0x1390` | `0x14A4` |
| `bone::m_modelState` | `0x150` | `0x140` |

Unchanged and re-verified against the same dump: `m_hPawn` (`0x6BC`), `m_iHealth` (`0x34C`), `m_pGameSceneNode`
(`0x330`), `m_hActiveWeapon` (`0x60`), `m_Item` (`0x50`), `m_iItemDefinitionIndex` (`0x1BA`).

Magic numbers that used to be duplicated in `cs2_player.cpp` / `cs2_weapon.cpp` are now named constants next to the
table they belong to, so a future layout change is a one-line edit:

* `offsets::list::chunks_base` (`0x10`), `chunk_stride` (`0x70`), `handle_index_mask` (`0x7FFF`), `chunk_shift` (9),
  `entry_mask` (`0x1FF`) — CHandle → entity chunk resolution;
* `offsets::bone::m_pBoneArray` (`0x80`) — bone-array pointer inside `CModelState` (not part of the networked schema,
  so the dumper does not list it; it has stayed at `+0x80` from `m_modelState`).

Still unverified without a live client: the four byte signatures, the `0x70` entity-slot stride and the `+0x80`
bone-array offset. `Dumper::Init()` fails loudly with "signature scan failed - outdated pattern" if a pattern stops
matching, instead of silently drawing nothing.

## Verification

* Every translation unit was compiled with `-std=c++20 -Wall -Wextra` against the real Dear ImGui 1.92.5 sources
  (Win32/D3D11 headers stubbed, since the sandbox is Linux) — no errors, warnings cleaned up.
* The memory, game, cache and ESP-rendering units were linked together with ImGui and a small harness that exercises
  `view_matrix_t::wts`, `Vec2_t`/`Vec3_t` operators and `color_t` conversions.
* **A real MSVC build is now green**: GitHub Actions `windows-latest` (Windows Server 2025 + Visual Studio 2026,
  MSVC 14.51 / toolset `v145`) compiles `Build.sln` in `Release|x64` with **0 warnings** and produces
  `cs2-external-esp.exe` (PE32+, 729 KB). The first CI run failed and the workflow caught a MinGW-only assumption:
  `NTSTATUS` is declared in `<winternl.h>`, not `<Windows.h>`, so the `NtReadVirtualMemory` typedef now uses plain
  `long` / `unsigned long` types (commit "Fix MSVC build: NTSTATUS is not declared by <Windows.h>").
* Artifacts (`exe` + `pdb`) are attached to every successful run of the `build` workflow.
