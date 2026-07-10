import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

export const root = resolve(dirname(fileURLToPath(import.meta.url)), "..", "..");
export const binary = join(
  root,
  "out",
  process.platform === "win32" ? "tesseractdtl.exe" : "tesseractdtl",
);

export function ensureBuilt(): void {
  if (existsSync(binary)) {
    return;
  }
  const result = spawnSync(process.execPath, ["scripts/build.mjs"], {
    cwd: root,
    encoding: "utf8",
  });
  if (result.status !== 0) {
    throw new Error(result.stderr || result.stdout || "build failed");
  }
}

export function runCli(args: string[]): string {
  ensureBuilt();
  const result = spawnSync(binary, args, {
    cwd: root,
    encoding: "utf8",
  });
  if (result.status !== 0) {
    throw new Error(`command failed: ${binary} ${args.join(" ")}\n${result.stderr}`);
  }
  return result.stdout;
}

export function runScenario(name: string, options: string[] = []): any {
  const stdout = runCli(["run", name, "--json", ...options]);
  return JSON.parse(stdout);
}

export function validateScenario(name: string): string {
  return runCli(["validate", name]).trim();
}

export function byId<T extends { id: string }>(items: T[], id: string): T {
  const found = items.find((item) => item.id === id);
  if (!found) {
    throw new Error(`missing item ${id}`);
  }
  return found;
}

export function accountBalance(report: any, account: string, asset: string): number {
  const found = report.accounts.find((row: any) => row.account === account && row.asset === asset);
  if (!found) {
    throw new Error(`missing balance ${account}/${asset}`);
  }
  return found.amount;
}
