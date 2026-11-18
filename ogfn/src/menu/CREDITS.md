<!--
=== REMAKE CREDITS ===
Arena - UI Remake
Built with Dear ImGui 1.91.9b, Geist and Phosphor
Third-party licenses: see CREDITS.md
=== /REMAKE CREDITS ===
-->
# Credits and third-party licenses

This project is a UI remake: a Dear ImGui menu rebuilt from screenshots of
"Arena Software". It is unofficial and a user interface only; it contains no
game-related functionality, no injection, no driver, no network and no hardware
access. Every value it shows is a local placeholder. The names "Arena Software",
"immediate7" and "falarzaomi" and the credit line in the footer are shown as in the
reference screenshots and belong to their owners.

## Third-party components

| Component | Version | License | Source |
|-----------|---------|---------|--------|
| Dear ImGui | v1.91.9b (CMake fetches it for the dev build; the shipped solution carries it as source in `third_party/imgui/`) | MIT | https://github.com/ocornut/imgui |
| Geist (`Geist-Regular.ttf`, static instance wght 400 of the variable font) | 1.8 | SIL Open Font License 1.1 | https://github.com/vercel/geist-font, https://github.com/google/fonts/tree/main/ofl/geist |
| Geist Mono (`GeistMono-Medium.ttf`, static instance wght 500 of the variable font) | 1.8 | SIL Open Font License 1.1 | https://github.com/vercel/geist-font, https://github.com/google/fonts/tree/main/ofl/geistmono |
| Phosphor Icons (`Phosphor-Bold.ttf`) | @phosphor-icons/web 2.1.1 | MIT | https://github.com/phosphor-icons/homepage |
| Project template (`ui/anim`, `ui/shell_win32`, `tools/`) | imgui-menu-clone skill, custom build by Syntax | Shared as-is, free to use and modify, no warranty | Palace Hub (original skill), Syntax (customisations) |

The OFL fonts ask for attribution and for the license text to travel with
redistributions: keep this file with every copy.

## Pictures cut from the reference screenshots

These crops belong to their respective owners and are included only so the remake
looks like its reference; replace them with your own pictures before publishing anything.

| File | What | Where from |
|------|------|------------|
| `assets/logo.png` | the "A" mark as a white mask, tinted with the accent when drawn | reference A |
| `assets/avatar.png` | the round avatar of the user line | reference A |
| `assets/avatar_b.png` | the avatar of the user line, compare mode only | reference B |

## Notes for redistribution

- `bin/`, `obj/` and `.vs/` are the solution's local build output; do not
  redistribute them (rebuild from the .sln).
- `reference/` (the screenshots the remake was measured from) is not part of
  this release.
- This folder is the release written by `tools/ship.py` from the development
  tree; `tools/` itself is not included.
- Fonts and pictures are loaded from `assets/` next to the executable (copied
  there by the build).
