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

## Verification

* Every translation unit was compiled with `-std=c++20 -Wall -Wextra` against the real Dear ImGui 1.92.5 sources
  (Win32/D3D11 headers stubbed, since the sandbox is Linux) — no errors, warnings cleaned up.
* The memory, game, cache and ESP-rendering units were linked together with ImGui and a small harness that exercises
  `view_matrix_t::wts`, `Vec2_t`/`Vec3_t` operators and `color_t` conversions.
* A real `MSVC` build has **not** been run: it needs Windows and the `v145` toolset. Offsets and signatures are
  unchanged, so runtime behaviour against `cs2.exe` matches the original build.
