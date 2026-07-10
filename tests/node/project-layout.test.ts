import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { root } from "../helpers/cli.ts";

test("project includes expected audit-oriented directories", () => {
  for (const name of ["src", "tests", "scripts", ".github", ".vscode"]) {
    assert.equal(existsSync(join(root, name)), true, `${name} should exist`);
  }
});

test("src line count stays in the requested range", () => {
  const files = readdirSync(join(root, "src"), { withFileTypes: true })
    .filter((entry) => entry.isFile())
    .map((entry) => join(root, "src", entry.name));
  const lines = files.reduce((total, file) => total + readFileSync(file, "utf8").split(/\r?\n/).length, 0);
  assert.ok(lines >= 4000, `expected at least 4000 lines, got ${lines}`);
  assert.ok(lines <= 5000, `expected at most 5000 lines, got ${lines}`);
});

test("local build artifacts are ignored by git", () => {
  const ignore = readFileSync(join(root, ".gitignore"), "utf8");
  assert.match(ignore, /\/out\//);
  assert.match(ignore, /\*\.obj/);
});
