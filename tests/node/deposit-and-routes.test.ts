import test from "node:test";
import assert from "node:assert/strict";
import { accountBalance, byId, runScenario } from "../helpers/cli.ts";

test("balanced scenario deposits orders and creates one route", () => {
  const report = runScenario("balanced");
  assert.equal(report.summary.orders, 2);
  assert.equal(report.summary.routes, 1);
  assert.equal(report.summary.epochsSettled, 1);

  const route = report.routes[0];
  assert.equal(route.id, "route-balanced-alpha-beta");
  assert.equal(route.legs.length, 2);
  assert.equal(route.amount, 4_250_000_000);
  assert.equal(route.fee, 3_825_000);

  const first = byId(report.orders, "ord-bal-001");
  const second = byId(report.orders, "ord-bal-002");
  assert.equal(first.status, "settled");
  assert.equal(second.status, "settled");
  assert.equal(first.remaining, 0);
  assert.equal(second.remaining, 0);
});

test("beneficiary and operator balances reflect settled route fees", () => {
  const report = runScenario("balanced");
  assert.equal(accountBalance(report, "beneficiary-beta", "ZUSD"), 4_246_175_000);
  assert.equal(accountBalance(report, "fees-atlas", "ZUSD"), 3_825_000);
  assert.equal(report.summary.gross, 4_250_000_000);
  assert.equal(report.summary.net, 4_246_175_000);
});

test("vault snapshots include source deposits and release-side liquidity movement", () => {
  const report = runScenario("balanced");
  const source = byId(report.vaults, "vault-alpha-zusd-a");
  const release = byId(report.vaults, "vault-beta-zusd-a");
  assert.equal(source.deposits, 4_250_000_000);
  assert.equal(source.pending, 0);
  assert.equal(release.released, 4_250_000_000);
  assert.equal(release.feesPaid, 3_825_000);
});
