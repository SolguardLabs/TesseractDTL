#!/usr/bin/env bash
set -euo pipefail

node scripts/build.mjs --warnings
node --experimental-strip-types --test "tests/node/*.test.ts"
