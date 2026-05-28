#!/usr/bin/env node
/* Headless validation of the servo model. Runs identical code to the browser
   sim (servo-model.js). Prints a report and asserts the properties that matter:
     1. zero drift at rest (deadzone holds)
     2. fast convergence to the pen target
     3. lands under the pen when gain is matched (S_assumed == S_true)
     4. gain mismatch -> bounded STATIC offset (calibratable), not runaway drift
     5. servo never worse than the old absolute-as-delta path
   Exit nonzero on any failed assertion. */
const M = require("./servo-model.js");

let failures = 0;
const check = (name, cond, detail) => {
  console.log(`  ${cond ? "PASS" : "FAIL"}  ${name}${detail ? "  — " + detail : ""}`);
  if (!cond) failures++;
};
const j = (o) => JSON.stringify(o);

console.log("\n=== 1. Matched gain (Sa=St=1.0), defaults Kp=1.0 D=2 Cl=127 ===");
{
  const sim = M.createSim({ Kp: 1.0, D: 2, Sa: 1.0, St: 1.0, Cl: 127 });
  const all = sim.runAll();
  for (const [name, r] of Object.entries(all.results)) console.log(`  ${name.padEnd(13)} ${j(r)}`);
  check("centerHold: ~zero drift at rest", all.results.centerHold.driftServo <= 2.0,
        `driftServo=${all.results.centerHold.driftServo}`);
  check("corners: lands under pen (<=3px worst)", all.results.corners.worstServo <= 3.0,
        `worstServo=${all.results.corners.worstServo}`);
  check("restToCorner: converges fast (<=15 frames)", all.results.restToCorner.convergeFrames <= 15,
        `frames=${all.results.restToCorner.convergeFrames}`);
  check("sweep: tracks (<=4px)", all.results.sweep.driftServo <= 4.0,
        `driftServo=${all.results.sweep.driftServo}`);
  check("liftTouch: no residual drift (<=3px)", all.results.liftTouch.driftServo <= 3.0,
        `driftServo=${all.results.liftTouch.driftServo}`);
}

console.log("\n=== 2. Servo vs old-code (absolute-as-delta) on drift-prone paths ===");
{
  const sim = M.createSim({ Sa: 1.0, St: 1.0 });
  for (const name of ["liftTouch", "sweep", "corners"]) {
    const r = sim.runScript(name);
    const better = r.driftServo <= r.driftOld + 1e-6;
    console.log(`  ${name.padEnd(11)} servo=${r.driftServo}  old=${r.driftOld}`);
    check(`${name}: servo <= old`, better);
  }
}

console.log("\n=== 3. Gain sensitivity (corners worstServo as S_true varies, Sa fixed=1) ===");
{
  const sim = M.createSim({ Sa: 1.0 });
  const sweep = sim.gainSweep("corners", [0.5, 0.75, 1.0, 1.5, 2.0]);
  for (const [st, r] of Object.entries(sweep)) console.log(`  S_true=${st.padEnd(4)} ${j(r)}`);
  check("matched gain best (St=1 worst<<St=2)",
        sweep["1"].driftServo < sweep["2"].driftServo,
        `St1=${sweep["1"].driftServo} St2=${sweep["2"].driftServo}`);
  check("mismatch is bounded, not runaway (St=2 < 256px)",
        sweep["2"].driftServo < 256,
        `St2 driftServo=${sweep["2"].driftServo}`);
}

console.log("\n=== 4. Drift-at-rest is exactly zero over a long hold (Sa=St=1) ===");
{
  const sim = M.createSim({ Sa: 1.0, St: 1.0, D: 2 });
  sim.reset(); sim.penDown(60, 180);
  sim.tick(200);                       // converge + hold a long time
  const d1 = sim.driftServo();
  sim.tick(2000);                      // hold much longer
  const d2 = sim.driftServo();
  console.log(`  after 200f: ${d1.toFixed(3)}px   after +2000f: ${d2.toFixed(3)}px`);
  check("no accumulation over time (Δ < 0.01px)", Math.abs(d2 - d1) < 0.01,
        `delta=${Math.abs(d2 - d1).toFixed(4)}`);
}

console.log("\n=== 5. Continuous-motion tracking lag (pen dragged at v px/frame) ===");
{
  const lagWhileMoving = (Kp, v) => {
    const sim = M.createSim({ Kp, D: 2, Sa: 1.0, St: 1.0, Cl: 127 });
    sim.reset(); sim.penDown(40, 112); let maxLag = 0, x = 40;
    for (let f = 0; f < 25; f++) { x += v; sim.penMove(x, 112); sim.tick(1);
      maxLag = Math.max(maxLag, sim.driftServo()); }
    return maxLag;
  };
  const v = 8;
  for (const Kp of [0.6, 0.8, 1.0])
    console.log(`  Kp=${Kp}  maxLagWhileMoving=${lagWhileMoving(Kp, v).toFixed(1)}px  (v=${v}px/frame)`);
  check("Kp=1.0 tracks a moving pen with ~zero lag (<=1px)", lagWhileMoving(1.0, v) <= 1.0,
        `lag=${lagWhileMoving(1.0, v).toFixed(2)}`);
  check("Kp=0.6 visibly lags a moving pen (why default must be 1.0)", lagWhileMoving(0.6, v) > 10,
        `lag=${lagWhileMoving(0.6, v).toFixed(1)}`);
}

console.log(`\n=== RESULT: ${failures === 0 ? "ALL PASS" : failures + " FAILURE(S)"} ===\n`);
process.exit(failures === 0 ? 0 : 1);
