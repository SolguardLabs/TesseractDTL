import test from "node:test";
import assert from "node:assert/strict";
import { byId, runScenario } from "../helpers/cli.ts";

test("multi-epoch settlement can stop after the first epoch", () => {
  const report = runScenario("multi-epoch", ["--epoch", "1"]);
  assert.equal(report.summary.epochsSettled, 1);
  assert.equal(report.epochs[0].epoch, 1);
  assert.equal(report.summary.openLiability, 4_250_000_000);
  assert.equal(byId(report.orders, "ord-me-001").status, "settled");
  assert.equal(byId(report.orders, "ord-me-002").status, "routed");
  assert.equal(byId(report.orders, "ord-me-004").status, "routed");
});

test("multi-epoch settlement processes route windows in epoch order", () => {
  const report = runScenario("multi-epoch");
  assert.deepEqual(
    report.epochs.map((epoch: any) => epoch.epoch),
    [1, 2, 3],
  );
  assert.equal(report.summary.openLiability, 0);
  assert.equal(report.summary.gross, 7_250_000_000);
  assert.equal(report.summary.instructions, 3);
});

test("operator window scenario settles multiple assets in one epoch", () => {
  const report = runScenario("operator-windows");
  assert.equal(report.summary.orders, 3);
  assert.equal(report.summary.epochsSettled, 1);
  assert.equal(report.epochs[0].epoch, 2);
  assert.equal(report.epochs[0].instructions.length, 3);
  assert.ok(report.epochs[0].instructions.some((instruction: any) => instruction.asset === "ZBTC"));
});
