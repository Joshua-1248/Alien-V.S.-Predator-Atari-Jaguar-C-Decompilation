#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CC="${CC:-cc}"
CFLAGS=(-std=c99 -Wall -Wextra -Werror -I"$ROOT/include")
build_run(){ local name=$1; shift; "$CC" "${CFLAGS[@]}" "$ROOT/tests/$name.c" "$@" -o "/tmp/$name"; "/tmp/$name"; }
build_run test_hud_message "$ROOT/src/game/hud_message.c"
build_run test_levels_doors "$ROOT/src/game/levels.c" "$ROOT/src/game/doors.c" "$ROOT/src/game/runtime.c"
build_run test_pred_weapons "$ROOT/src/game/weapons.c"
build_run test_collectables "$ROOT/src/game/collectables.c"
build_run test_eeprom "$ROOT/src/game/eeprom.c"
build_run test_amp "$ROOT/src/game/amp.c" "$ROOT/src/game/runtime.c"
build_run test_amp_projectile "$ROOT/src/game/amp.c" "$ROOT/src/game/runtime.c"
build_run test_collision "$ROOT/src/game/collision.c"
"$CC" "${CFLAGS[@]}" -fno-builtin -Dmalloc=avp_test_malloc "$ROOT/tests/test_inflate.c" "$ROOT/src/unzip/inflate_reconstructed.c" -o /tmp/test_inflate
/tmp/test_inflate
echo "All C-decomp regression tests passed."
