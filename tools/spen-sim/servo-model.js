/* ============================================================================
   S-Pen -> SNES Mouse pipeline model (pure, no DOM).
   Shared by index.html (visual sim) and validate.cjs (headless assertions) so
   both run identical code. This is the unit we port to libretro.cpp verbatim.

   Pipeline:
     RetroArch hands the core an ABSOLUTE pointer in [-32767,+32767]
     (viewport-relative, origin center). Core transform -> game px:
         game = (pointer + 32767) * dim / 65534
     The SNES Mouse is RELATIVE. snes9x UpdatePolledMouse ships
     clamp(cur-old, -127,+127) per poll (lossless carry); the GAME integrates
     those deltas * an unknown gain S_true into its own cursor.
     The fix (servo) steers an estimated cursor toward the absolute pen target
     with a clamped, deadzoned, proportional delta -> cannot drift.
   ========================================================================== */
(function (root, factory) {
  if (typeof module === "object" && module.exports) module.exports = factory();
  else root.SpenModel = factory();
})(typeof self !== "undefined" ? self : this, function () {
  const W = 256, H = 224;

  const gamePxFromPointer = (ptr, dim) => (ptr + 32767) * dim / 65534;
  const pointerFromGamePx = (g, dim) => Math.round(g * 65534 / dim) - 32767;

  // snes9x controls.cpp UpdatePolledMouse: emit clamp(cur-old,+-127), carry remainder
  function clampCarry(cur, old) {
    const j = cur - old;
    if (j < -127) return { emitted: -127, newOld: old - 127 };
    if (j < 0)    return { emitted: j,    newOld: cur };
    if (j > 127)  return { emitted: 127,  newOld: old + 127 };
    return { emitted: j, newOld: cur };
  }

  // THE servo step (ported to C++ as-is). target/est in game px; step in mouse units.
  function servoStep(target, est, p) {
    const error = target - est;
    if (Math.abs(error) <= p.D) return { step: 0, newEst: est };
    let step = Math.round(p.Kp * error / p.Sa);
    if (step > p.Cl) step = p.Cl;
    if (step < -p.Cl) step = -p.Cl;
    if (step === 0) step = error > 0 ? 1 : -1; // guarantee progress outside deadzone
    return { step, newEst: est + step * p.Sa };
  }

  const clampScreen = (c) => {
    c.x = Math.max(0, Math.min(W - 1, c.x));
    c.y = Math.max(0, Math.min(H - 1, c.y));
  };
  const dist = (a, b) => Math.hypot(a.x - b.x, a.y - b.y);

  function createSim(overrides) {
    const params = Object.assign(
      // Kp=1.0: move the full error each frame (capped by the +-127 HW clamp) ->
      // zero lag tracking a moving pen AND no drift. Lower Kp = smoother but laggier.
      { Kp: 1.0, D: 2, Sa: 1.0, St: 1.0, Cl: 127, hover: true },
      overrides || {}
    );
    let S;

    function reset() {
      S = {
        pen: { x: W / 2, y: H / 2 }, contact: false, frame: 0,
        snes: { x: 0, y: 0 }, mOld: { x: 0, y: 0 },
        est: { x: W / 2, y: H / 2 }, trueS: { x: W / 2, y: H / 2 },
        snesO: { x: 0, y: 0 }, mOldO: { x: 0, y: 0 }, trueO: { x: W / 2, y: H / 2 },
        trail: { servo: [], old: [] }, drift: { servo: [], old: [] }
      };
    }

    function axisServo(axis, target) {
      const r = servoStep(target, S.est[axis], params);
      S.est[axis] = r.newEst;
      S.snes[axis] += r.step;                            // = cur fed to S9xReportPointer
      const cc = clampCarry(S.snes[axis], S.mOld[axis]); // snes9x derives wire delta
      S.mOld[axis] = cc.newOld;
      S.trueS[axis] += cc.emitted * params.St;           // game integrates * true gain
    }
    function axisOld(axis, target) {
      S.snesO[axis] = target;                            // THE BUG: absolute written as state
      const cc = clampCarry(S.snesO[axis], S.mOldO[axis]);
      S.mOldO[axis] = cc.newOld;
      S.trueO[axis] += cc.emitted * params.St;
    }

    function tick(n) {
      n = n || 1;
      for (let i = 0; i < n; i++) {
        const active = S.contact || params.hover;
        if (active) {
          axisServo("x", S.pen.x); axisServo("y", S.pen.y);
          const ox = S.contact ? S.pen.x : W / 2;        // pen-away -> RA reports 0 -> center
          const oy = S.contact ? S.pen.y : H / 2;
          axisOld("x", ox); axisOld("y", oy);
        }
        clampScreen(S.trueS); clampScreen(S.trueO);
        S.frame++;
        push(S.drift.servo, dist(S.trueS, S.pen), 240);
        push(S.drift.old, dist(S.trueO, S.pen), 240);
        push(S.trail.servo, { x: S.trueS.x, y: S.trueS.y }, 120);
        push(S.trail.old, { x: S.trueO.x, y: S.trueO.y }, 120);
      }
    }
    const push = (arr, v, max) => { arr.push(v); if (arr.length > max) arr.shift(); };

    const driftServo = () => dist(S.trueS, S.pen);
    const driftOld = () => dist(S.trueO, S.pen);
    const settle = (f) => tick(f || 120);

    const SCRIPTS = {
      centerHold() { S.contact = true; S.pen = { x: W/2, y: H/2 }; settle(120);
                     S.contact = false; settle(60); return snapshot(); },
      corners() { const cs = [[8,8],[W-8,8],[W-8,H-8],[8,H-8],[W/2,H/2]]; let worst = 0;
                  for (const [x,y] of cs) { S.contact = true; S.pen = {x,y}; settle(90);
                    worst = Math.max(worst, driftServo()); }
                  S.contact = false; return Object.assign({ worstServo: round(worst) }, snapshot()); },
      sweep() { S.contact = true; for (let i=0;i<=W;i+=18){ S.pen={x:i,y:H/2+40*Math.sin(i/20)}; tick(3);}
                settle(60); S.contact = false; return snapshot(); },
      restToCorner() { reset(); S.contact = false; settle(30); S.contact = true; S.pen = {x:W-8,y:H-8};
                       let f = 0; while (driftServo() > 3 && f < 600) { tick(1); f++; }
                       S.contact = false; return Object.assign({ convergeFrames: f }, snapshot()); },
      liftTouch() { const spot = { x: 80, y: 80 }; for (let i=0;i<20;i++){ S.contact=true; S.pen=spot; tick(8);
                    S.contact=false; tick(8);} S.contact=true; S.pen=spot; settle(60);
                    S.contact=false; return snapshot(); }
    };
    const round = (v) => +v.toFixed(2);
    const snapshot = () => ({ driftServo: round(driftServo()), driftOld: round(driftOld()) });

    function runScript(name) { reset(); return SCRIPTS[name](); }
    function runAll() { const out = { params: Object.assign({}, params), results: {} };
      for (const n of Object.keys(SCRIPTS)) { reset(); out.results[n] = SCRIPTS[n](); }
      reset(); return out; }
    function gainSweep(name, sts) { name = name || "corners"; sts = sts || [0.5,0.75,1,1.5,2];
      const o = {}; const keep = params.St;
      for (const st of sts) { params.St = st; reset(); o[st] = SCRIPTS[name](); }
      params.St = keep; reset(); return o; }

    reset();
    return {
      params, get S() { return S; }, reset, tick,
      penDown: (x, y) => { S.contact = true; if (x != null) S.pen = { x, y }; },
      penMove: (x, y) => { S.pen = { x, y }; },
      penUp: () => { S.contact = false; },
      setPenNorm: (nx, ny) => { S.pen = { x: nx * (W-1), y: ny * (H-1) }; },
      state: () => ({ pen: Object.assign({}, S.pen), contact: S.contact, frame: S.frame,
        servo: Object.assign({}, S.trueS), old: Object.assign({}, S.trueO),
        est: Object.assign({}, S.est),
        driftServo: round(driftServo()), driftOld: round(driftOld()) }),
      driftServo, driftOld, SCRIPTS, runScript, runAll, gainSweep
    };
  }

  return { W, H, gamePxFromPointer, pointerFromGamePx, clampCarry, servoStep, createSim };
});
