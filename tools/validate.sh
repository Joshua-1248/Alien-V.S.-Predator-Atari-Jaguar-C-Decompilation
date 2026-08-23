#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-validate"
rm -rf "$BUILD"
cmake -S "$ROOT" -B "$BUILD" -DAVP_STRICT=ON
cmake --build "$BUILD" -j2
"$ROOT/tests/run_tests.sh"
printf 'int main(void){return 0;}\n' > /tmp/avp_c_empty_main.c
"${CC:-cc}" /tmp/avp_c_empty_main.c -Wl,--whole-archive "$BUILD/libavp_c.a" -Wl,--no-whole-archive -o /tmp/avp_c_whole_archive
# Shipping C/H must not contain unfinished implementation markers.  Research
# notes may discuss historical placeholders, so scan only active source/include.
if grep -RniE 'TODO|FIXME|placeholder until|stub[[:space:]]*(implementation|routine)|queued for' "$ROOT/src" "$ROOT/include"; then
  echo 'FAIL: unfinished implementation marker found in active source' >&2
  exit 1
fi
python3 "$ROOT/tools/audit_public_tree.py"
echo 'PASS: strict build, tests, whole-archive link and publication audit'
