import test from "node:test";
import assert from "node:assert/strict";
import { runCli, runScenario, validateScenario } from "../helpers/cli.ts";

test("CLI lists deterministic scenarios", () => {
  const scenarios = JSON.parse(runCli(["list", "--json"]));
  assert.equal(scenarios.length, 4);
  assert.deepEqual(
    scenarios.map((scenario: any) => scenario.name),
    ["balanced", "multi-epoch", "operator-windows", "mesh-reconcile"],
  );
});

test("validation command accepts every built-in scenario", () => {
  for (const scenario of ["balanced", "multi-epoch", "operator-windows", "mesh-reconcile"]) {
    assert.equal(validateScenario(scenario), `ok ${scenario}`);
  }
});

test("JSON report has a stable top-level contract", () => {
  const report = runScenario("balanced", ["--events"]);
  assert.equal(report.protocol, "TesseractDTL");
  assert.equal(report.scenario, "balanced");
  assert.ok(Array.isArray(report.accounts));
  assert.ok(Array.isArray(report.vaults));
  assert.ok(Array.isArray(report.orders));
  assert.ok(Array.isArray(report.routes));
  assert.ok(Array.isArray(report.epochs));
  assert.ok(Array.isArray(report.events));
  assert.equal(report.summary.conservationOk, true);
});
