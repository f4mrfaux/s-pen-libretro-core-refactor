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

### Status: SOLVED — RAM-feedback + per-game profile table (Mario Paint & Clock Tower locked on hardware)

The cursor now tracks the pen pixel-accurately, including clicks (paint/aim land where the pen is).
Two layers got us here:

1. **HOVER fix (core-side):** detect the pen by a CHANGE in pointer coords. RetroArch delivers hover
   as count-INDEPENDENT `POINTER_X/Y` with `PRESSED=false`; the core was gating on `count>0` and
   ignoring it. Also fixed the idle-creep + per-tap re-home. **DO NOT re-fix RetroArch hover — it's
   done** (branch `spen-hover-fix`, deployed). See memory `retroarch-hover-already-implemented`.
2. **RAM-feedback cursor (the breakthrough):** the SNES Mouse is RELATIVE and the game's delta→pixel
   gain is nonlinear, so a pure dead-reckoning servo always drifts (confirmed; even Nintendo's
   official emu is imprecise). Instead, for known games the core READS the game's real cursor from
   WRAM and steers it toward the pen (closed-loop, gain-independent → no drift/offset, survives
   lifts/edges). Unprofiled games fall back to the dead-reckoning servo (`spen_servo.h`); lightgun
   games are untouched (already absolute).

**Per-game profile table** `SPEN_MOUSE_PROFILES[]` (top of `libretro.cpp`) — `{ title-prefix,
cursor_x_addr, cursor_y_addr }`, keyed on `Memory.ROMName`; `$7Exxxx == Memory.RAM[xxxx]`:
- `MARIO PAINT` $0226/$0227 — VERIFIED locked.
- `CLOCK TOWER` $017E/$017F ("CLOCK TOWER SFX" Deluxe) — VERIFIED locked.
- `LEMMINGS`/$0071/0073, `LEMMINGS 2`/$0C34/0C35, `SIMCITY`/$01EB/01ED, `DUNGEON MASTER`/$0034/0036 —
  seeded (sourced; title prefixes unconfirmed). See memory `snes-mouse-cursor-addresses`.

**Auto-finder (DEBUG_SPEN_VERBOSE builds only):** when a game has no profile, the core correlates
each WRAM byte (`$0000-1FFF`) against the pen target over a pen sweep and logs
`[SPEN-FINDER] Xcand:$XXXX(r=..) Ycand:$YYYY(r=..)`. To add a game: load an (owned) ROM, sweep the
pen ~5s, take the `r≈1.00` candidate → add one line to the table → rebuild. This is how Clock Tower
was found. Do NOT source ROMs — user dumps their own carts.

**Inputs (per spec):** hover/contact = move cursor; tap (tip) = left click; barrel = right click.
Tunable options `snes9x_spen_servo_gain`/`_responsiveness`/`_deadzone` apply to the dead-reckoning
fallback (the smoothing option was removed). Servo unit + native test + sim:
`spen_servo.h`, `spen_servo_test.cpp`, `tools/spen-sim/` (`node tools/spen-sim/validate.cjs` → ALL PASS).

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

### Build + deploy (ARM64 ONLY — see memory `arm-build-only`)
- Rebuild the debug core from `cores/snes9x/libretro`:
  `ndk-build NDK_PROJECT_PATH=$PWD APP_BUILD_SCRIPT=$PWD/jni/Android.mk APP_ABI=arm64-v8a
  APP_PLATFORM=android-21 APP_CFLAGS=-DDEBUG_SPEN_VERBOSE` (absolute paths avoid the `jni/jni/..`
  source-path doubling). Output: `libs/arm64-v8a/libretro.so`. NDK r25c at `/home/bob/android-ndk-r25c`.
- **RetroArch loads the core from its PRIVATE dir**
  `/data/user/0/com.retroarch.aarch64/cores/snes9x_libretro.so` — NOT `/storage/emulated/0/RetroArch/cores/`
  (pushing there does nothing). The app IS debuggable, so deploy via run-as: push the `.so` to
  `/storage/emulated/0/Download/`, then `adb shell 'run-as com.retroarch.aarch64 cp
  /storage/emulated/0/Download/X.so cores/snes9x_libretro.so'`. Delete stale cores.
- Confirm the new core is actually live via `[SPEN-DEBUG-1]`/`[SPEN-SERVO]` lines in the RetroArch
  FILE log (`/storage/emulated/0/RetroArch/logs/*.log` — NOT adb logcat). Pull the newest log and
  grep `SPEN-SERVO` (shows raw/target/est/step/acc). adb is flaky — re-run `adb devices` if it drops.

### Git state
- Main `main`: spec, plan, `tools/spen-sim/`, `.gitmodules`, README, CLAUDE.md committed (`28f424c`).
- `cores/snes9x` (`stylus/spen-support`): servo + options + initial gating committed (`e77d6e9e`).
  **Hardware-driven fixes are UNCOMMITTED in the working tree:** hover coord-change detection,
  removal of the per-lift re-home resets, and the `[SPEN-SERVO]` trace. The DEVICE runs a DEBUG build
  (`-DDEBUG_SPEN_VERBOSE`) of this working tree. Diverged from origin; not pushed. Version
  `1.62.3-spen-v9` (`snes9x_libretro.info`). A machine restart will NOT lose these working-tree edits.

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
snes9x now ships `snes9x_spen_servo_gain`, `snes9x_spen_servo_responsiveness`,
`snes9x_spen_servo_deadzone` (LIVE in the deployed core); `snes9x_spen_advanced_filtering` was
removed (servo subsumes smoothing). On hardware, gain=2.0 undershot badly — keep gain ≤ 1.0.

### Backlog (future ideas)
- **Auto-hide on-screen overlay when S-Pen is active** (RetroArch-fork feature, NOT the core):
  the touch overlay's input zones conflict with S-Pen taps. When a stylus is detected, temporarily
  hide the overlay; restore it after ~2s of no stylus events so touch menu access returns.
  Implement in the RetroArch fork (`input/drivers/android_input.c` stylus detection already exists;
  drive overlay visibility off a stylus-activity timestamp + timeout). Requested 2026-05-28 —
  manually toggling the overlay is a friction point and losing it strands menu access.

### User goal
Sandbox development → hardware testing → upstream PR contributions to libretro cores.
