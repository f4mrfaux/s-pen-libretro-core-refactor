# Claude Code Session Memory - S-Pen Libretro Core Refactor

## Current Project Status (2026-05-28)

### Active focus: fix SNES9x S-Pen cursor DRIFT (Mario Paint test bed)

**Root cause (verified in source — not inferred):**
The SNES Mouse is a **relative** device. `cores/snes9x/controls.cpp:2767` (`UpdatePolledMouse`)
ships `clamp(cur_x - old_x, ±127)` to the game each poll; the game integrates those deltas.
The S-Pen "absolute" path wrote a raw **absolute** screen pixel into that delta pipeline
(`libretro.cpp:2189` `snes_mouse_state[port][0] = x;` → `S9xReportPointer` at `:2342`). An
absolute coordinate consumed as a delta = relative motion with no absolute lock → cumulative
**drift** (cursor wanders away from the pen, worst across lifts and fast moves).

**NOT the cause:** the earlier "transform overflow for small positive raw" theory was a misread
of the RetroArch `[SNES9X S-Pen VERBOSE]` log (its `Raw:` field = physical pixel, `Transformed:`
= RetroArch normalized `[-32767,+32767]` — the ±30000 values are the *correct* edge mapping).
The current core transform (`libretro.cpp:2143`) is clean `double` math. No overflow bug.

### The fix: converge-to-target servo (designed + sim-validated; NOT yet in the core)

Each frame, steer an estimated cursor toward the absolute pen target with a delta clamped to the
SNES ±127/poll limit. `Kp=1.0` → zero-lag tracking; a tunable `gain` (S_assumed) calibrates
mouse-units→pixels on hardware. Re-referencing the absolute target every frame makes drift
impossible.

- **Spec:** `docs/superpowers/specs/2026-05-28-snes9x-spen-servo-design.md`
- **Plan:** `docs/superpowers/plans/2026-05-28-snes9x-spen-servo.md` (bite-sized TDD tasks)
- **Validated simulator:** `tools/spen-sim/` — `servo-model.js` (shared model),
  `index.html` (visual, Playwright-driveable), `validate.cjs` (headless asserts;
  run `node tools/spen-sim/validate.cjs` → ALL PASS), `screenshots/`.

**Sim results (Kp=1.0, gain matched):** drift 0 at rest & over 2200 frames, 1-frame convergence,
0px corners, **0px tracking lag while dragging** (vs 32px at Kp=0.6 — the catch that set Kp=1.0).
Gain mismatch → bounded *static* offset (not drift), calibratable via the gain option.

### Architecture: TWO layers (do not forget the RetroArch half)

- **`/home/bob/projects/RetroArch`** (fork `f4mrfaux/RetroArch`, branch `spen-hover-fix`,
  ~239 commits ahead of origin): the **input source**. Normalizes the S-Pen into the libretro
  pointer contract — semantic pointer indices **0 = contact/cursor** (instant, distance-based),
  **1 = tip click** (pressure), **2 = barrel button**; absolute, viewport-relative,
  `[-32767,+32767]`, origin center; hover updates X/Y but PRESSED stays false. Oct-2025
  breakthrough (contact vs click separation) lives here. Has uncommitted `android_input.c`
  debug-logging changes.
- **`cores/*`** (this repo's submodules): consumers of that contract. The SNES9x fix is
  core-side only — RetroArch already provides what we need.

### Current git state (uncommitted — for tomorrow's review)
- Main repo `main`: today's new files are UNCOMMITTED working-tree changes — the spec, the plan,
  `tools/spen-sim/`, and this CLAUDE.md rewrite. Nothing committed yet (no-auto-commit rule).
- `cores/snes9x` branch `stylus/spen-support`: **diverged from origin (17 ahead / 9 behind),
  unpushed**, with uncommitted `libretro.cpp` debug-logging (+15 lines). Version string
  `1.62.3-spen-v8` lives only in `snes9x_libretro.info`, not in source.

### Next steps (review TOMORROW, then execute the plan)
1. Review spec + plan together.
2. Decide execution mode (subagent-driven vs inline) and the commit policy.
3. Phase 0: docs de-slop (this file done; `.gitmodules` add missing entries incl. `cores/mame`
   which currently makes `git submodule status` fatal; README dead links).
4. Phase 1–2: C++ servo (`spen_servo.h` + native test mirroring `validate.cjs`) → integrate into
   `libretro.cpp`, add `servo_gain`/`responsiveness`/`deadzone` options, drop smoothing option,
   fix hover-gating leak, patch legacy-touch branch.
5. Phase 3: ARM64 build (NDK r25c) → bump `-spen-v9`.
6. Phase 4: Mario Paint hardware test + gain calibration on the Galaxy Z Fold 5.

### Sibling cores (already absolute, working — out of scope for the drift fix)
Genesis-Plus-GX (lightgun/Pico pen), melonDS (DS touchscreen), SwanStation (GunCon + PS-mouse via
delta-of-absolutes), snes9x Superscope — all map the pointer with `((pointer+0x7fff)*extent)/0xfffe`
so the point lands under the pen. SwanStation's PS-mouse is the absolute→relative precedent.

### Key references
- RetroArch input: `/home/bob/projects/RetroArch/input/drivers/android_input.c` (semantic indices,
  hover), `gfx/video_driver.c:693` (`video_driver_translate_coord_viewport` normalization).
- snes9x mouse model: `cores/snes9x/controls.cpp` — struct ~`:107`, `UpdatePolledMouse` `:2763-2812`,
  pointer→`cur_x` `:2506`, serial report `:3013`.
- melonDS matrix ref: `/home/bob/temp_melondsds/melondsds/src/libretro/screenlayout.hpp:188-191`.
- Build for **ARM64** (aarch64), never x86_64, for the device.

### Core options pattern
`{core}_spen_coordinate_mode` (absolute|relative), `{core}_spen_input_mode` (auto|mouse|lightgun).
New (planned) snes9x: `snes9x_spen_servo_gain`, `snes9x_spen_servo_responsiveness`,
`snes9x_spen_servo_deadzone`; removing `snes9x_spen_advanced_filtering` (servo subsumes smoothing).

### User goal
Sandbox development → hardware testing → upstream PR contributions to libretro cores.
