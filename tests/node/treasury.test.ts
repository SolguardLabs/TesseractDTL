import test from "node:test";
import assert from "node:assert/strict";
import { runCli } from "../helpers/cli.ts";

test("treasury view exposes bounded per-vault accounting ratios", () => {
  const report = JSON.parse(runCli(["treasury", "balanced"]));
  assert.equal(report.scenario, "balanced");
  assert.ok(report.vaults.length >= 4);
  for (const vault of report.vaults) {
    assert.ok(vault.cash >= 0);
    assert.ok(vault.pending >= 0);
    assert.ok(vault.available >= 0);
    assert.ok(vault.coverageBps >= 0);
    assert.ok(vault.releaseShareBps >= 0 && vault.releaseShareBps <= 10_000);
  }
});

test("treasury view supports an epoch cutoff without mutating prior reports", () => {
  const early = JSON.parse(runCli(["treasury", "multi-epoch", "--epoch", "1"]));
  const final = JSON.parse(runCli(["treasury", "multi-epoch"]));
  const released = (rows: any[]) => rows.reduce((total, row) => total + row.released, 0);
  assert.ok(released(early.vaults) < released(final.vaults));
});
