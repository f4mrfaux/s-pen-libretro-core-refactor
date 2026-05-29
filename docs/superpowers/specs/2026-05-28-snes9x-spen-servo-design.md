# SNES9x S-Pen Cursor Servo — Design Spec

- **Date:** 2026-05-28
- **Status:** IMPLEMENTED + deployed to hardware (2026-05-28). Servo in the core, ARM64-built, running on the Galaxy Z Fold 5 / Mario Paint. Sim-validated and hardware-confirmed: cursor tracks the pen during continuous hover/contact; HOVER fixed core-side; tap=left, barrel=right. **Two issues remain** (see memory `project_spen-hardware-state` + §"Hardware findings" below): (1) constant down-left offset = wrong `est` home position vs Mario Paint's actual cursor start; (2) lift + far-side desync = the servo dead-reckons the game cursor (can't read SNES RAM), so big blind moves break the absolute lock. Note: hover is delivered correctly by RetroArch already (count-independent POINTER_X/Y) — do NOT re-fix it RetroArch-side.
- **Author:** f4mrfaux + Claude
- **Scope:** Fix S-Pen cursor **drift** in the SNES9x libretro core (Mario Paint as test bed). Core-side only — no RetroArch changes required.

---

## 1. Problem

In Mario Paint, the SNES cursor **drifts** away from the S-Pen over time — it tracks pen *motion* but never locks to pen *position*, and accumulates error across pen lifts and fast moves.

**Root cause (verified in source, not inferred):** The SNES Mouse is a **relative** device. `snes9x/controls.cpp:2767` ships `clamp(cur_x - old_x, ±127)` to the game each poll; the game integrates those deltas into its own cursor. The current S-Pen "absolute" path writes a raw absolute screen pixel into that delta pipeline:

- `cores/snes9x/libretro/libretro.cpp:2189` — `snes_mouse_state[port][0] = x;` (absolute game pixel)
- `cores/snes9x/libretro/libretro.cpp:2342` — `S9xReportPointer(...)` stores it as `cur_x`
- `controls.cpp:2763-2812` — `UpdatePolledMouse` emits `cur-old` as the delta

So an absolute coordinate is consumed as if it were a delta → relative motion with no absolute lock, a first-contact jump, and cumulative drift on every lift/jump.

**Not the cause:** An earlier log analysis suggested a "transform overflow for small positive raw values." That was a misread of the RetroArch-side `[SNES9X S-Pen VERBOSE]` debug log, whose `Raw:` field is a *physical pixel* and `Transformed:` field is RetroArch's *normalized* `[-32767,+32767]` value. Those "garbage ±30000" numbers are the **correct** physical→normalized mapping near a screen edge. The current core transform (`libretro.cpp:2143`) is clean `double` math. There is no transform-overflow bug.

## 2. Architecture context (two layers)

S-Pen support spans two repos:

- **`/home/bob/projects/RetroArch`** (fork `f4mrfaux/RetroArch`, branch `spen-hover-fix`, ~239 commits): the *input source*. Normalizes the stylus into the libretro pointer contract — semantic pointer indices **0 = contact/cursor** (instant, distance-based), **1 = tip click** (pressure), **2 = barrel button**; coordinates absolute, viewport-relative, `[-32767,+32767]`, origin center; hover updates X/Y but PRESSED stays false.
- **`cores/*`** (this repo's submodules): *consumers* of that contract.

This fix is entirely in the **snes9x core**, consuming the existing RetroArch contract. No RetroArch change needed.

## 3. The cohesive target (evidence)

"Cohesive UX" = cursor sits **under the pen**, matching everything else:

- RetroArch's own menu cursor is **absolute/teleports** to the pen (`menu_driver.c:2154`, `materialui.c:8084`).
- Sibling cores all put the point under the pen via `((pointer + 0x7fff) * extent) / 0xfffe`: Genesis-Plus-GX lightgun/Pico pen, melonDS touchscreen, SwanStation GunCon, snes9x Superscope.
- **SwanStation's PlayStation Mouse** is the precedent for absolute-pen → relative-mouse: store the absolute pen position, emit `delta = current − previous` (`playstation_mouse.cpp:151`).

The SNES Mouse cannot be commanded to an absolute position, so true "under the pen" requires actively steering it there.

## 4. Chosen approach — converge-to-target servo

Each frame, steer an estimated cursor toward the absolute pen target with a clamped, deadzoned, proportional delta. Re-referencing the absolute target every frame makes drift impossible.

**Rejected alternatives:**
- *Pure delta-of-absolutes* (SwanStation precedent): simplest, but tracks motion not position — diverges to a persistent offset when gain ≠ 1, and can't re-lock.
- *Hybrid snap+track*: more state/edge cases; only worth it if servo feels laggy on fast strokes (sim shows it does not — 5-frame convergence).

### The servo (pure unit — ported verbatim to C++)

`target`/`est` in game pixels; `step` in mouse units. Reference implementation: `tools/spen-sim/servo-model.js` `servoStep()`.

```
error = target - est
if |error| <= D:            step = 0                      // deadzone: kills jitter & drift-at-rest
else:                       step = clamp(round(Kp*error / S_assumed), -Clamp, +Clamp)
                            if step == 0: step = sign(error)   // guarantee progress
emit:   snes_mouse_state[port] += step                    // becomes cur_x; game sees cur-old
est    += step * S_assumed                                // dead-reckon our own integration
```

- **On pen-down from rest:** keep `est` (do NOT snap) → the servo *glides* the cursor to the pen over ~`ceil(dist/127)` frames. That glide IS the "flies to the pen" absolute feel; no teleport jump.
- **During hover** (hover-moves-pointer on): servo runs; cursor follows the hovering pen (matches RetroArch). Clicks gated on tip-pressure (pointer index 1) + barrel (index 2) — existing action mapping unchanged.
- **Pen-up:** emit nothing; freeze `est`.
- **Why no drift:** `est` re-references the absolute `target` every frame; the ±127 carry is integer-exact (`UpdatePolledMouse`), so error self-corrects instead of compounding.

## 5. Simulator validation (done)

`tools/spen-sim/` — `servo-model.js` (shared pure model), `index.html` (visual, Playwright-driveable via `window.sim`), `validate.cjs` (headless assertions). Browser and Node produce **identical** numbers; visual confirmed via Playwright (`tools/spen-sim/screenshots/spen-servo-lifttouch.png`).

Defaults `Kp=1.0, D=2, S_assumed=1, Clamp=127`:

| Test | servo | old-code |
|---|---|---|
| center hold (drift @ rest) | **0.00px** | 168.7px |
| corners (worst) | **0.00px** | 9.9px |
| fast sweep | **0.34px** | 72.1px |
| rest→corner | **1 frame** to converge | 9.9px |
| lift/touch ×20 | **0.00px** | 168.7px |
| 2200-frame hold | **0.000px change** (no accumulation) | — |
| drag @ 8px/frame (tracking lag) | **0px @ Kp=1.0** (32px @ Kp=0.6) | — |

**`Kp=1.0` is the default, and it matters.** A proportional servo lags a *moving* target by ≈ velocity/Kp, so Kp<1 trails the pen *during a stroke* (32px lag at Kp=0.6, 8px/frame) — discovered by interacting with the live harness; the scripted tests missed it because they measured lag only *after* the cursor settled. Kp=1.0 moves the full error each frame (still capped by the ±127 HW clamp) → **zero lag while moving**, while still re-referencing the absolute target each frame (no drift). Kp is the responsiveness↔smoothing knob; lower it only to damp a jittery sensor.

## 6. Key finding — gain calibration (the one residual)

The servo eliminates *drift*. The only residual is a **static, position-dependent gain offset** when `S_assumed ≠ S_true` (the game's real mouse-units→pixels gain, unmeasurable from inside the core because we can't read SNES RAM):

| S_true (real) | worst error |
|---|---|
| 1.0 (matched) | 2.2px |
| 0.75 / 1.5 | 41 / 63px |
| 0.5 / 2.0 | 80 / 140px |

Error ≈ `|S_true/S_assumed − 1| × distance-from-center` — stable, bounded, NOT accumulating. snes9x sends the mouse delta **unscaled** (`controls.cpp` has no sensitivity multiply), so `S_assumed = 1.0` is the principled default; the user calibrates once on hardware. **Therefore the fix exposes a tunable gain option.** Small residuals are absorbed by the human-in-the-loop (hand re-aims at what the eye sees).

## 7. C++ implementation plan

Target: `cores/snes9x/libretro/libretro.cpp`, `case RETRO_DEVICE_MOUSE` (~lines 2094-2343).

1. **Replace the S-Pen absolute branch** (2124-2259) with the servo: keep transform → `target` (game px); maintain `static int spen_est_x[2], spen_est_y[2]` (or float for sub-pixel `est`); run `servoStep` per axis; `snes_mouse_state[port][axis] += step`. Remove the `snes_mouse_state = x` absolute assignment.
2. **Initialize `est`** to screen center on first use; on pen-down do **not** reset to target (glide). Reuse/retire the existing `spen_filter_initialized` / `spen_filtered_*` state (smoothing is removed — the servo subsumes it).
3. **Relative branch** (2261-2311): keep as-is (already delta-of-absolutes) OR fold into servo path; decide in plan. Not the drift source, low priority.
4. **Legacy absolute touch branch** (2314-2332): has the same absolute-as-delta bug; route finger touch through the same servo (or delta-of-absolutes) so it does not regress.
5. **Fix `is_spen_mode` gating** (2114-2121): auto mode currently falls through to the traditional accumulative path on hover/non-contact frames, letting stray `RETRO_DEVICE_MOUSE` deltas leak in. Gate on `pointer_count > 0` for presence, not just `tip/barrel`.
6. **Preserve the traditional finger/mouse path** (2334-2341) for non-S-Pen input — verify no regression.
7. **Trim debug logging**: the `[SPEN-DEBUG-*]` block (incl. the uncommitted +15 lines) runs every frame; gate behind a compile flag (`DEBUG_SPEN_VERBOSE`) and drop the per-frame range trackers.

### New / changed core options (`libretro_core_options.h`)
- **`snes9x_spen_servo_gain`** — new. Values e.g. `0.5, 0.75, 1.0, 1.25, 1.5, 2.0` (default `1.0`). Maps to `S_assumed`. Calibrated per game/sensitivity to kill the static gain offset (§6).
- **`snes9x_spen_servo_responsiveness`** (Kp) — new, default `1.0` (zero-lag tracking of a moving pen). Lower (e.g. `0.8`) smooths a jittery pen sensor at the cost of slight lag while drawing. Subsumes the removed smoothing option.
- **`snes9x_spen_servo_deadzone`** — optional, default `2`px (jitter/rest stability).
- Keep: `snes9x_spen_coordinate_mode`, `snes9x_spen_input_mode`, `snes9x_spen_tap_action`, `snes9x_spen_barrel_action`, `snes9x_spen_hover_behavior`. `snes9x_spen_advanced_filtering` (smoothing) is **removed** — superseded by the servo.

## 8. Edge cases & behavior
- Pen-down from rest → glide (no teleport). Pen-up → freeze.
- Large jump (corner→corner ≈159px) → traverses at ≤127 units/frame over ~2 frames; clamp carry lossless.
- Deadzone D=2 kills sub-pixel jitter and guarantees zero drift at rest.
- Overscan: use `g_screen_gun_height` dynamically (224/239) — already handled.
- `int16` `cur_x` wraparound over a long session is benign (modular subtraction keeps small deltas correct).
- Two ports: per-port `est`/state arrays (`[2]`).

## 9. Testing plan
- **Sim:** done (§5). `node tools/spen-sim/validate.cjs` is the regression gate for the servo math.
- **Hardware (Galaxy Z Fold 5, Mario Paint):** build ARM64 (NDK r25c), deploy, test corners/center/sweep/lift per `S-PEN_HARDWARE_TEST_SYLLABUS.md`. Confirm cursor lands under pen and no drift over a sustained drawing session.
- **Gain calibration on device:** point pen at a known target, observe where cursor lands, adjust `snes9x_spen_servo_gain` until matched. Record the value for Mario Paint.
- **Regression:** traditional finger touch + physical mouse still work; other games (Clock Tower hover) unaffected.

## 10. De-slop / docs phase (Phase 0, low-risk, parallelizable)
- **CLAUDE.md:** rewrite to 2026 reality — real root cause (relative device + drift) and servo fix; version `1.62.3-spen-v8` (lives only in the `.info`); snes9x branch `stylus/spen-support` is **diverged from origin (17 ahead/9 behind), unpushed**; delete obsolete "PENDING COMMITS" and "FIXED / READY FOR TESTING" claims; document the two-layer RetroArch↔cores architecture.
- **.gitmodules:** add missing entries (`cores/mame`, `cores/melonDS`, `cores/picodrive`, `cores/scummvm` — `mame`'s absence makes `git submodule status` fatally error); fix branch mismatches (Genesis-Plus-GX/beetle-psx/desmume/mame2003 are on `stylus/hover-drag`, declared `stylus/spen-support`).
- **README.md:** delink missing `UI_CONFIGURATION_GUIDE.md` / `CONTRIBUTING.md`; downgrade premature "✅ Tested".

## 11. Build sequence
- **Phase 0** — docs/state de-slop (low-risk).
- **Phase 1** — simulator + validation. **DONE.**
- **Phase 2** — port servo to `libretro.cpp` + options + cleanup; `validate.cjs` green; build ARM64.
- **Phase 3** — Mario Paint hardware test + gain calibration on the Z Fold 5.

## 12. Risks & open questions
- **Gain `S_true` is only knowable on hardware.** Mitigation: tunable option + human-in-loop. Ship `1.0` default.
- Mario Paint's in-game "mouse speed" setting changes `S_true`; document "set it once, calibrate to it."
- Whether to unify the relative + legacy-touch branches into the servo or leave them — decide in the implementation plan.

## 13. Out of scope
- RetroArch-side changes (the input contract already suffices).
- Other cores (Genesis/melonDS/SwanStation already absolute and working).
- Pushing/committing the diverged snes9x branch or the RetroArch `spen-hover-fix` branch (separate, user-initiated).
