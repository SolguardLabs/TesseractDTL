import test from "node:test";
import assert from "node:assert/strict";
import { runScenario } from "../helpers/cli.ts";

test("mesh reconciliation closes all order liabilities", () => {
  const report = runScenario("mesh-reconcile");
  assert.equal(report.summary.orders, 5);
  assert.equal(report.summary.routes, 3);
  assert.equal(report.summary.epochsSettled, 2);
  assert.equal(report.summary.openLiability, 0);
  assert.equal(report.reconciliation.openLiability, 0);
  assert.equal(report.reconciliation.allVaultsHealthy, true);
});

test("asset-level reconciliation reports ZUSD and ZBTC independently", () => {
  const report = runScenario("mesh-reconcile");
  const assets = new Map(report.reconciliation.assets.map((row: any) => [row.asset, row]));
  assert.equal(assets.has("ZUSD"), true);
  assert.equal(assets.has("ZBTC"), true);
  assert.equal(assets.get("ZUSD").pending, 0);
  assert.equal(assets.get("ZUSD").released, 7_300_000_000);
  assert.equal(assets.get("ZBTC").released, 0);
});

test("operator reconciliation aggregates reduced instructions", () => {
  const report = runScenario("mesh-reconcile");
  const operators = new Map(report.reconciliation.operators.map((row: any) => [row.operator, row]));
  assert.equal(operators.get("op-prism").instructions, 2);
  assert.equal(operators.get("op-nova").instructions, 2);
  assert.equal(operators.get("op-prism").gross, 4_400_000_000);
  assert.equal(operators.get("op-nova").gross, 2_900_000_000);
});
