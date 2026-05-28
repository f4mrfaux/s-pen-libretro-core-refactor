# SNES9x S-Pen Cursor Servo — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the SNES9x S-Pen "absolute" mouse path (which writes an absolute coordinate into a relative-delta device → cursor drift) with a converge-to-target servo so the cursor tracks under the pen with zero drift, and refresh the stale project docs.

**Architecture:** A pure servo function (`spen_servo_step`) steers an estimated cursor toward the absolute pen target each frame, emitting a delta clamped to the SNES mouse's ±127/poll limit. `Kp=1.0` gives zero-lag tracking; a tunable `gain` (S_assumed) calibrates units→pixels on hardware. The servo math is first proven by a native C++ unit test that mirrors the already-validated JS model (`tools/spen-sim/`), then integrated into `libretro.cpp`.

**Tech Stack:** C++ (libretro core), Android NDK r25c (ARM64), g++ for native unit test. Reference design: `docs/superpowers/specs/2026-05-28-snes9x-spen-servo-design.md`. Reference model: `tools/spen-sim/servo-model.js`, `tools/spen-sim/validate.cjs`.

**Note on commits:** This repo's owner uses a no-auto-commit rule. Commit steps below are real checkpoints, but confirm with the user before each commit (or batch them) rather than committing unprompted.

---

## File Structure

- `cores/snes9x/libretro/spen_servo.h` — **create**. Pure, dependency-free servo + params/state structs. Unit-testable, included by both the test and `libretro.cpp`.
- `cores/snes9x/libretro/spen_servo_test.cpp` — **create**. Native C++ test mirroring `validate.cjs` scenarios.
- `cores/snes9x/libretro/libretro.cpp` — **modify**. Replace the S-Pen absolute branch with the servo; add state; fix gating; gate logging.
- `cores/snes9x/libretro/libretro_core_options.h` — **modify**. Add servo options; remove the smoothing option.
- `CLAUDE.md`, `.gitmodules`, `README.md` — **modify**. De-slop to current reality.

---

## Phase 0 — Docs de-slop (low-risk, independent)

### Task 1: Rewrite CLAUDE.md to current reality

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Replace the stale status with current facts**

Rewrite the "Current Project Status" section to state:
- Date 2026-05-28; the active problem is **S-Pen cursor drift in SNES9x**, root cause = SNES Mouse is a *relative* device fed an *absolute* coordinate (`controls.cpp:2767` ships `cur-old`; `libretro.cpp:2189` wrote absolute). Fix = converge-to-target servo (see `docs/superpowers/specs/2026-05-28-snes9x-spen-servo-design.md`).
- Architecture is **two layers**: `/home/bob/projects/RetroArch` fork (branch `spen-hover-fix`, the input source: semantic pointer indices 0=contact/cursor, 1=tip-click, 2=barrel) → cores consume the pointer contract.
- Version string is `1.62.3-spen-v8`, present only in the `.info` file.
- snes9x submodule branch `stylus/spen-support` is **diverged from origin (17 ahead / 9 behind), unpushed**, with uncommitted debug-logging changes.

- [ ] **Step 2: Delete obsolete claims**

Remove the three "PENDING COMMITS" blocks (SwanStation `833988f7` and Genesis `ea7e09bf` are already committed; the snes9x v7 push block is obsolete/unsafe). Remove "🟢 FIXED" / "READY FOR TESTING" claims for the mouse — they were contradicted by 7 more weeks of debugging.

- [ ] **Step 3: Verify no remaining false claims**

Run: `grep -ni 'pending commit\|READY FOR TESTING\|spen-v7\|FIXED - Traditional' CLAUDE.md`
Expected: no matches.

- [ ] **Step 4: Commit** (confirm with user first)

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md to current reality (servo fix, v8, two-layer arch)"
```

### Task 2: Fix .gitmodules

**Files:**
- Modify: `.gitmodules`

- [ ] **Step 1: Add missing submodule entries**

`git submodule status` currently dies with `no submodule mapping found in .gitmodules for path 'cores/mame'`. Add entries for `cores/mame`, `cores/melonDS`, `cores/picodrive`, `cores/scummvm` (mirror the format of existing entries; set `url` to the correct fork/upstream and `branch` to the actually-checked-out branch for each — get these from `git -C cores/<name> remote -v` and `git -C cores/<name> branch --show-current`).

- [ ] **Step 2: Correct declared branches to match checkout**

For `cores/Genesis-Plus-GX`, `cores/beetle-psx-libretro`, `cores/desmume`, `cores/mame2003-plus-libretro`: change declared `branch = stylus/spen-support` to `branch = stylus/hover-drag` (their real checked-out branch).

- [ ] **Step 3: Verify submodule status no longer errors**

Run: `git submodule status 2>&1 | head`
Expected: a list of submodule SHAs, no `fatal:` line.

- [ ] **Step 4: Commit** (confirm with user first)

```bash
git add .gitmodules
git commit -m "fix: repair .gitmodules (add mame/melonDS/picodrive/scummvm, correct branches)"
```

### Task 3: Fix README.md

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Delink missing files**

Remove or correct references to `UI_CONFIGURATION_GUIDE.md` and `CONTRIBUTING.md` (neither exists).

- [ ] **Step 2: Downgrade premature "Tested" claims**

Change "✅ Tested" for SNES9x/MAME2016/SwanStation to "🚧 In progress" (the mouse fix is not yet hardware-verified).

- [ ] **Step 3: Verify**

Run: `grep -n 'UI_CONFIGURATION_GUIDE\|CONTRIBUTING.md' README.md`
Expected: no matches.

- [ ] **Step 4: Commit** (confirm with user first)

```bash
git add README.md
git commit -m "docs: fix README dead links and overstated test status"
```

---

## Phase 1 — C++ servo unit (TDD, native, no Android)

### Task 4: Write the failing C++ servo test

**Files:**
- Create: `cores/snes9x/libretro/spen_servo_test.cpp`

- [ ] **Step 1: Write the test (mirrors `tools/spen-sim/validate.cjs`)**

```cpp
// Native unit test for the S-Pen servo. Mirrors tools/spen-sim/validate.cjs.
// Build: g++ -std=c++11 spen_servo_test.cpp -o spen_servo_test && ./spen_servo_test
#include "spen_servo.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

static const int W = 256, H = 224;
static int failures = 0;
static void check(const char* name, bool cond, double v) {
    printf("  %s  %s (%.2f)\n", cond ? "PASS" : "FAIL", name, v);
    if (!cond) failures++;
}

// Replicates snes9x UpdatePolledMouse: emit clamp(cur-old,+-127), carry remainder.
struct MouseCh { int cur=0, old=0; int step(int s){ cur+=s; int j=cur-old, e;
    if(j<-127){e=-127;old-=127;} else if(j<0){e=j;old=cur;}
    else if(j>127){e=127;old+=127;} else {e=j;old=cur;} return e; } };

struct Sim {
    spen_servo_params p; double est_x, est_y, true_x, true_y; MouseCh mx, my; double St;
    Sim(double kp,double gain,int dz,double st){ p.kp=kp;p.gain=gain;p.deadzone=dz;p.clamp=127;
        est_x=est_y=W/2; true_x=true_y=W/2; St=st; }   // note: H/2 for y in real use; W/2 ok for test symmetry
    void reset(){ est_x=W/2.0; est_y=H/2.0; true_x=W/2.0; true_y=H/2.0; mx=MouseCh(); my=MouseCh(); }
    void stepTo(double tx,double ty){
        int sx=spen_servo_step(tx,&est_x,&p); int sy=spen_servo_step(ty,&est_y,&p);
        true_x += mx.step(sx)*St; true_y += my.step(sy)*St;
        true_x=std::max(0.0,std::min((double)W-1,true_x)); true_y=std::max(0.0,std::min((double)H-1,true_y));
    }
    double drift(double px,double py){ return std::hypot(true_x-px, true_y-py); }
};

int main(){
    // 1. matched gain, default Kp=1.0: lands under pen, no drift at rest
    { Sim s(1.0,1.0,2,1.0); s.reset();
      double tx=8, ty=8; for(int f=0;f<90;f++) s.stepTo(tx,ty);
      check("corners-ish: lands under pen (<=3px)", s.drift(tx,ty)<=3.0, s.drift(tx,ty)); }

    // 2. drift at rest is exactly zero over a long hold
    { Sim s(1.0,1.0,2,1.0); s.reset();
      for(int f=0;f<200;f++) s.stepTo(60,180); double d1=s.drift(60,180);
      for(int f=0;f<2000;f++) s.stepTo(60,180); double d2=s.drift(60,180);
      check("no accumulation over time (<0.01px)", std::fabs(d2-d1)<0.01, std::fabs(d2-d1)); }

    // 3. continuous-motion tracking lag at Kp=1.0 is ~zero
    { Sim s(1.0,1.0,2,1.0); s.reset(); double maxLag=0,x=40;
      for(int f=0;f<25;f++){ x+=8; s.stepTo(x,112); maxLag=std::max(maxLag,s.drift(x,112)); }
      check("Kp=1.0 zero-lag tracking (<=1px)", maxLag<=1.0, maxLag); }

    // 4. gain mismatch is a bounded static offset, not runaway
    { Sim s(1.0,1.0,2,2.0); s.reset();   // S_true=2, S_assumed=1
      for(int f=0;f<90;f++) s.stepTo(8,8);
      check("gain mismatch bounded (<256px)", s.drift(8,8)<256.0, s.drift(8,8)); }

    printf("\n%s\n", failures==0 ? "ALL PASS" : "FAILURES");
    return failures==0 ? 0 : 1;
}
```

- [ ] **Step 2: Run to verify it fails (no header yet)**

Run: `cd cores/snes9x/libretro && g++ -std=c++11 spen_servo_test.cpp -o spen_servo_test`
Expected: FAIL — `fatal error: spen_servo.h: No such file or directory`.

### Task 5: Implement the servo header to pass the test

**Files:**
- Create: `cores/snes9x/libretro/spen_servo.h`

- [ ] **Step 1: Write the header**

```c
#ifndef SPEN_SERVO_H
#define SPEN_SERVO_H
#include <math.h>

/* Converge-to-target servo: steer an estimated cursor (game px) toward the
   absolute pen target, emitting a delta in SNES mouse units (clamped to +-127).
   Kp=1.0 -> zero-lag tracking; gain (=S_assumed) calibrates units->px. */
typedef struct {
    double kp;        /* responsiveness; 1.0 = move full error each frame */
    double gain;      /* S_assumed: mouse-units -> game pixels */
    int    deadzone;  /* px of error ignored (anti-jitter / rest stability) */
    int    clamp;     /* max units per poll; SNES HW limit = 127 */
} spen_servo_params;

/* Returns the per-axis step (mouse units) to emit and advances *est. */
static inline int spen_servo_step(double target, double *est, const spen_servo_params *p)
{
    double error = target - *est;
    if (error <= p->deadzone && error >= -p->deadzone)
        return 0;
    double s = p->kp * error / p->gain;
    int step = (int)floor(s + 0.5);            /* match JS Math.round */
    if (step >  p->clamp) step =  p->clamp;
    if (step < -p->clamp) step = -p->clamp;
    if (step == 0) step = (error > 0) ? 1 : -1; /* guarantee progress outside deadzone */
    *est += (double)step * p->gain;
    return step;
}
#endif /* SPEN_SERVO_H */
```

- [ ] **Step 2: Run the test to verify it passes**

Run: `cd cores/snes9x/libretro && g++ -std=c++11 spen_servo_test.cpp -o spen_servo_test && ./spen_servo_test`
Expected: all `PASS`, final line `ALL PASS`, exit 0.

- [ ] **Step 3: Cross-check against the JS validator**

Run: `node ../../../tools/spen-sim/validate.cjs`
Expected: `ALL PASS`. The C++ and JS models agree on behavior.

- [ ] **Step 4: Commit** (confirm with user first)

```bash
git -C cores/snes9x add libretro/spen_servo.h libretro/spen_servo_test.cpp
git -C cores/snes9x commit -m "feat: add converge-to-target S-Pen servo (pure unit) + native test"
```

---

## Phase 2 — Integrate into libretro.cpp

### Task 6: Add servo state + integrate into the S-Pen absolute branch

**Files:**
- Modify: `cores/snes9x/libretro/libretro.cpp` (S-Pen absolute branch, currently ~2124-2259; include + state globals near the other `spen_*` statics ~1800-1820)

- [ ] **Step 1: Include the header**

Near the top includes of `libretro.cpp`, add:
```cpp
#include "spen_servo.h"
```

- [ ] **Step 2: Add servo config + per-port state globals**

Next to the existing `spen_*` static declarations (near `snes_mouse_state`), add:
```cpp
static double spen_servo_gain = 1.0;   /* S_assumed (units->px), core option */
static double spen_servo_kp   = 1.0;   /* responsiveness, core option */
static int    spen_servo_deadzone = 2; /* px, core option */
static double spen_est_x[2] = {0.0, 0.0};
static double spen_est_y[2] = {0.0, 0.0};
static bool   spen_servo_initialized[2] = {false, false};
```

- [ ] **Step 3: Replace the absolute branch body**

Replace the current `if (is_spen_mode && spen_coordinate_mode == 0 && (pointer_active || force_spen_mode)) { ... }` block (the transform + smoothing + `snes_mouse_state[port][0] = x` assignment + range trackers) with:

```cpp
if (is_spen_mode && spen_coordinate_mode == 0 && (pointer_active || force_spen_mode)) {
    bool any_button_pressed = tip_pressed || general_pressed || barrel_pressed;

    if (!spen_servo_initialized[port]) {
        spen_servo_initialized[port] = true;
        spen_est_x[port] = g_screen_gun_width  / 2.0;
        spen_est_y[port] = g_screen_gun_height / 2.0;
    }

    /* RetroArch pointer [-32767,+32767] -> game pixels (absolute target). */
    double target_x = ((double)pointer_x + 32767.0) * (double)g_screen_gun_width  / 65534.0;
    double target_y = ((double)pointer_y + 32767.0) * (double)g_screen_gun_height / 65534.0;

    spen_servo_params sp = { spen_servo_kp, spen_servo_gain, spen_servo_deadzone, 127 };
    int step_x = spen_servo_step(target_x, &spen_est_x[port], &sp);
    int step_y = spen_servo_step(target_y, &spen_est_y[port], &sp);

    /* Emit relative deltas; S9xReportPointer/UpdatePolledMouse turn cur-old into the wire delta. */
    snes_mouse_state[port][0] += step_x;
    snes_mouse_state[port][1] += step_y;

#ifdef DEBUG_SPEN_VERBOSE
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[SPEN-SERVO] port=%d tgt=(%.1f,%.1f) est=(%.1f,%.1f) step=(%d,%d)\n",
               port, target_x, target_y, spen_est_x[port], spen_est_y[port], step_x, step_y);
#endif

    /* ---- button mapping: UNCHANGED from the existing absolute branch ---- */
    for (int i = MOUSE_LEFT; i <= MOUSE_LAST; i++) {
        bool pressed = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, i);
        if (i == MOUSE_LEFT && general_pressed) pressed = true;
        if (tip_pressed && spen_tap_action != SNES9X_SPEN_ACTION_DISABLED) {
            switch (spen_tap_action) {
                case SNES9X_SPEN_ACTION_LEFT_CLICK:   if (i == MOUSE_LEFT) pressed = true; break;
                case SNES9X_SPEN_ACTION_RIGHT_CLICK:  if (i == MOUSE_RIGHT) pressed = true; break;
                case SNES9X_SPEN_ACTION_MIDDLE_CLICK: if (i == MOUSE_MIDDLE) pressed = true; break;
                case SNES9X_SPEN_ACTION_TRIGGER:      if (i == MOUSE_LEFT) pressed = true; break;
                case SNES9X_SPEN_ACTION_RELOAD:       break;
            }
        }
        if (barrel_pressed && spen_barrel_action != SNES9X_SPEN_ACTION_DISABLED) {
            switch (spen_barrel_action) {
                case SNES9X_SPEN_ACTION_LEFT_CLICK:   if (i == MOUSE_LEFT) pressed = true; break;
                case SNES9X_SPEN_ACTION_RIGHT_CLICK:  if (i == MOUSE_RIGHT) pressed = true; break;
                case SNES9X_SPEN_ACTION_MIDDLE_CLICK: if (i == MOUSE_MIDDLE) pressed = true; break;
                case SNES9X_SPEN_ACTION_TRIGGER:      if (i == MOUSE_LEFT) pressed = true; break;
            }
        }
        S9xReportButton(MAKE_BUTTON(port + 1, i), pressed);
    }
}
```

- [ ] **Step 4: Reset servo state on pen-up so a future session re-centers cleanly**

In the branch that runs when the pointer is inactive (or at `retro_reset`), set `spen_servo_initialized[port] = false;` when `!pointer_active && !force_spen_mode`. (Keep `est` across brief lifts; only reset on full deactivation/reset.)

- [ ] **Step 5: Verify it compiles for desktop**

Run: `cd cores/snes9x && make -f Makefile platform=unix -j4 2>&1 | tail -20` (or the existing desktop build target).
Expected: builds `snes9x_libretro.so` with no errors referencing the servo.

- [ ] **Step 6: Commit** (confirm with user first)

```bash
git -C cores/snes9x add libretro/libretro.cpp
git -C cores/snes9x commit -m "feat: drive SNES mouse with converge-to-target servo (fixes S-Pen drift)"
```

### Task 7: Add core options, remove smoothing

**Files:**
- Modify: `cores/snes9x/libretro/libretro_core_options.h`
- Modify: `cores/snes9x/libretro/libretro.cpp` (the variable-parsing function — find by `grep -n 'snes9x_spen_coordinate_mode' libretro.cpp`)

- [ ] **Step 1: Add option definitions**

In `libretro_core_options.h`, next to the other `snes9x_spen_*` entries, add:
```c
{
   "snes9x_spen_servo_gain", "S-Pen Servo Gain (units->px)", "Servo Gain",
   "Calibrate so the cursor lands under the pen. Higher = cursor moves further per pen movement. Tune on hardware per game.",
   NULL, "spen",
   { {"0.5","0.5"},{"0.75","0.75"},{"1.0","1.0 (default)"},{"1.25","1.25"},{"1.5","1.5"},{"2.0","2.0"},{NULL,NULL} },
   "1.0"
},
{
   "snes9x_spen_servo_responsiveness", "S-Pen Servo Responsiveness", "Servo Responsiveness",
   "1.0 tracks the pen with zero lag. Lower smooths a jittery pen at the cost of slight lag while drawing.",
   NULL, "spen",
   { {"0.6","0.6 (smoothest)"},{"0.8","0.8"},{"0.9","0.9"},{"1.0","1.0 (zero lag)"},{NULL,NULL} },
   "1.0"
},
{
   "snes9x_spen_servo_deadzone", "S-Pen Servo Deadzone (px)", "Servo Deadzone",
   "Pixels of error ignored to prevent cursor jitter at rest.",
   NULL, "spen",
   { {"0","0"},{"1","1"},{"2","2 (default)"},{"3","3"},{"4","4"},{NULL,NULL} },
   "2"
},
```

- [ ] **Step 2: Remove the obsolete smoothing option**

Delete the `snes9x_spen_advanced_filtering` option block (the servo subsumes smoothing). Remove its parsing and the `spen_advanced_filtering` variable + any references in `libretro.cpp`.

- [ ] **Step 3: Parse the new options**

In the variable-update function, alongside the existing `snes9x_spen_coordinate_mode` parsing, add:
```cpp
var.key = "snes9x_spen_servo_gain";
if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) spen_servo_gain = atof(var.value);
var.key = "snes9x_spen_servo_responsiveness";
if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) spen_servo_kp = atof(var.value);
var.key = "snes9x_spen_servo_deadzone";
if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) spen_servo_deadzone = atoi(var.value);
```

- [ ] **Step 4: Verify build + options load**

Run: `cd cores/snes9x && make -f Makefile platform=unix -j4 2>&1 | tail -20`
Expected: builds clean; `grep -c spen_advanced_filtering libretro.cpp` returns 0.

- [ ] **Step 5: Commit** (confirm with user first)

```bash
git -C cores/snes9x add libretro/libretro_core_options.h libretro/libretro.cpp
git -C cores/snes9x commit -m "feat: add S-Pen servo core options (gain/responsiveness/deadzone), drop smoothing"
```

### Task 8: Fix is_spen_mode gating + legacy touch regression

**Files:**
- Modify: `cores/snes9x/libretro/libretro.cpp` (gating ~2114-2121; legacy absolute touch branch ~2314-2332)

- [ ] **Step 1: Stop hover frames falling through to the accumulative path**

In the `is_spen_mode` computation, base presence on the pointer being present, not only on tip/barrel contact:
```cpp
bool hover_signal = (pointer_count > 0) && (spen_hover_behavior != SPEN_HOVER_DISABLED);
bool is_spen_mode = force_spen_mode ||
                    (spen_input_mode == 0 && has_spen_features && (stylus_signal || hover_signal));
```
(Removing the `!general_pressed` condition so the S-Pen path stays selected across the contact transition instead of leaking a stray frame to the traditional `RETRO_DEVICE_MOUSE` delta path.)

- [ ] **Step 2: Fix the legacy absolute-touch branch (same absolute-as-delta bug)**

In the `else if (setting_mouse_mode == SETTING_MOUSE_MODE_ABSOLUTE ...)` branch, replace the `snes_mouse_state[port][0] = x; [1] = y;` assignment with a servo step reusing the same machinery (so finger touch in absolute mode also tracks, not jumps):
```cpp
if (!spen_servo_initialized[port]) {
    spen_servo_initialized[port] = true;
    spen_est_x[port] = g_screen_gun_width / 2.0; spen_est_y[port] = g_screen_gun_height / 2.0;
}
double tgx = ((double)pointer_x + 0x7FFF) * (double)g_screen_gun_width  / 0xFFFE;
double tgy = ((double)pointer_y + 0x7FFF) * (double)g_screen_gun_height / 0xFFFE;
spen_servo_params sp = { spen_servo_kp, spen_servo_gain, spen_servo_deadzone, 127 };
snes_mouse_state[port][0] += spen_servo_step(tgx, &spen_est_x[port], &sp);
snes_mouse_state[port][1] += spen_servo_step(tgy, &spen_est_y[port], &sp);
```
(Leave the **traditional relative** branch at ~2334-2341 — real `RETRO_DEVICE_MOUSE` deltas — untouched; it is correct.)

- [ ] **Step 3: Verify build**

Run: `cd cores/snes9x && make -f Makefile platform=unix -j4 2>&1 | tail -20`
Expected: clean build.

- [ ] **Step 4: Commit** (confirm with user first)

```bash
git -C cores/snes9x add libretro/libretro.cpp
git -C cores/snes9x commit -m "fix: S-Pen gating across contact transition + servo for legacy absolute touch"
```

### Task 9: Gate per-frame debug logging behind a compile flag

**Files:**
- Modify: `cores/snes9x/libretro/libretro.cpp` (the `[SPEN-DEBUG-*]` block ~2107-2212, incl. the uncommitted +15 range-tracker lines)

- [ ] **Step 1: Wrap the logging**

Wrap the `[SPEN-DEBUG-1]`…`[SPEN-DEBUG-RANGE]` blocks and the `ptr_min/max` range trackers in `#ifdef DEBUG_SPEN_VERBOSE … #endif`. Default builds emit nothing per frame.

- [ ] **Step 2: Verify no per-frame logging in default build**

Run: `cd cores/snes9x && make -f Makefile platform=unix -j4 2>&1 | tail -5 && grep -c 'SPEN-DEBUG-1' libretro.cpp`
Expected: builds clean; the `[SPEN-DEBUG-1]` string still present in source (inside the ifdef), but not compiled without the flag.

- [ ] **Step 3: Commit** (confirm with user first)

```bash
git -C cores/snes9x add libretro/libretro.cpp
git -C cores/snes9x commit -m "chore: gate per-frame S-Pen debug logging behind DEBUG_SPEN_VERBOSE"
```

---

## Phase 3 — Android build

### Task 10: Build ARM64 core for the device

**Files:**
- Use: `cores/snes9x/libretro/jni/` (NDK build)

- [ ] **Step 1: Build with NDK r25c for aarch64**

Run (adjust NDK path as needed):
```bash
cd cores/snes9x/libretro/jni && ndk-build APP_ABI=arm64-v8a -j4 2>&1 | tail -25
```
Expected: produces `snes9x_libretro_android.so` (ARM64). Confirm arch:
```bash
file ../snes9x_libretro_android.so
```
Expected: `ELF 64-bit ... ARM aarch64`.

- [ ] **Step 2: Bump version string in the .info**

In `snes9x_libretro.info`, bump `display_version` from `1.62.3-spen-v8` to `1.62.3-spen-v9` with a description noting the servo cursor fix.

- [ ] **Step 3: Commit** (confirm with user first)

```bash
git -C cores/snes9x add snes9x_libretro.info
git -C cores/snes9x commit -m "build: bump to 1.62.3-spen-v9 (servo cursor)"
```

---

## Phase 4 — Hardware test + calibration (manual, Galaxy Z Fold 5)

### Task 11: Deploy, test Mario Paint, calibrate gain

**Files:** none (device testing). Reference: `S-PEN_HARDWARE_TEST_SYLLABUS.md`.

- [ ] **Step 1: Deploy core + info to device**

```bash
adb push cores/snes9x/libretro/snes9x_libretro_android.so /storage/emulated/0/RetroArch/cores/
adb push cores/snes9x/snes9x_libretro.info /storage/emulated/0/RetroArch/info/
```

- [ ] **Step 2: Configure core options**

In RetroArch: set `snes9x_spen_input_mode = Auto` (or Mouse), `snes9x_spen_coordinate_mode = absolute`, `snes9x_spen_servo_responsiveness = 1.0`, `snes9x_spen_servo_gain = 1.0`, `snes9x_spen_servo_deadzone = 2`.

- [ ] **Step 3: Mario Paint cursor test**

Load Mario Paint. Verify: cursor sits under the pen on contact; tracks during strokes with no lag; **no drift** over a sustained drawing session; corners reachable; finger touch still works.

- [ ] **Step 4: Calibrate gain if cursor is offset**

If the cursor lands consistently short/long of the pen (gain mismatch), adjust `snes9x_spen_servo_gain` up (cursor moves too little) or down (too much) until it lands under the pen. Record the working value for Mario Paint in `S-PEN_HARDWARE_TEST_SYLLABUS.md`.

- [ ] **Step 5: Regression pass**

Confirm Superscope/lightgun games and other mouse games (e.g. Clock Tower hover) still behave; no regression to finger touch.

---

## Self-Review

- **Spec coverage:** servo (Task 5/6) ✓; tunable gain + responsiveness + deadzone options (Task 7) ✓; smoothing removed (Task 7) ✓; gating fix (Task 8) ✓; legacy-touch fix (Task 8) ✓; logging trim (Task 9) ✓; ARM64 build (Task 10) ✓; hardware test + calibration (Task 11) ✓; docs de-slop CLAUDE.md/.gitmodules/README (Tasks 1-3) ✓; sim/validator (spec Phase 1) already done — referenced, not re-planned.
- **Placeholder scan:** no TBD/TODO; all code shown. Branch-scope decision (unify vs minimal) resolved to "servo for S-Pen + minimal fix for legacy touch, leave traditional-relative untouched" in Task 8.
- **Type consistency:** `spen_servo_params{kp,gain,deadzone,clamp}`, `spen_servo_step(target, double* est, const spen_servo_params*)`, `spen_est_x/y[2]`, `spen_servo_initialized[2]`, `spen_servo_gain/kp/deadzone` used identically in the header, the test, and all `libretro.cpp` tasks.
