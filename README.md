## CS2 External ESP

A simple external ESP for Counter-Strike 2, focused on performance, simplicity, and educational purposes.

The project is read-only and does not modify game memory or provide gameplay assistance.

### Features

* External ESP
* Read-only
* Simple interface
* Automatic offset scanning
* Lightweight and performance-focused
* No memory writing
* No aimbot or triggerbot

This project does not guarantee protection against detection or bans. Use it at your own risk and follow the game's Terms of Service.

### Build

* Windows only (Win32 + Direct3D 11 + Dear ImGui).
* Visual Studio with the `v145` toolset, **x64** configuration (`Debug|x64` / `Release|x64`).
* `NtReadVirtualMemory` is resolved dynamically at runtime, no static imports from the target game.

Open `Build.sln`, select `Release | x64` and build. Run the produced executable while `cs2.exe` is already running
(the loader waits up to 50 s for the process and its modules).

### Runtime

* Everything is drawn on a click-through, top-most layered overlay window that follows the game window.
* Box, skeleton, health bar and weapon name are enabled by default; all of them are toggled through
  `src/cfg/active_configuration.hpp` (compile-time configuration, there is no in-game menu).
* `END` closes the overlay and stops both threads.
* Debug builds open a console with a log; release builds are silent and only write `crashlog.txt` on a fatal exception.

### Layout

```
src/
  cfg/            compile-time settings (toggles, colors, timings)
  draw/           DX11 overlay window + ImGui rendering of the ESP
  game/           process/cache layer, entity classes, geometry types
  mem/            process handle, memory reads, offset + signature scanner
  utils/          logging and vectored exception handler
  libs/imgui/     vendored Dear ImGui
```

### Notes

* The offsets in `src/mem/cs2_memory_offsets.hpp` are still game-version dependent: the four scanned signatures
  (`viewMatrix`, `entityList`, `localPlayerController`, `buildNumber`) are re-resolved on every start, but the
  struct member offsets (`m_iHealth`, `m_vOldOrigin`, `m_pGameSceneNode`, ...) must be updated after a game update.
* If a signature stops matching, start-up fails loudly in the log instead of drawing nothing.
