#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$SCRIPT_DIR"

CC_BIN="${CC:-cc}"
CFLAGS="${CFLAGS:--std=c99 -Wall -Wextra -O0 -g}"

python3 ./generate_malicious_font.py

"$CC_BIN" $CFLAGS -I../../Unity/src ./test.c ../../Unity/src/unity.c -lm -o ./test_web_2026_002

normal_status=0
./test_web_2026_002 || normal_status=$?
printf '\nnormal_exit=%s\n' "$normal_status"

VALGRIND_BIN="${VALGRIND_BIN:-valgrind}"
if command -v "$VALGRIND_BIN" >/dev/null 2>&1; then
    valgrind_status=0
    "$VALGRIND_BIN" --leak-check=full --error-exitcode=99 ./test_web_2026_002 || valgrind_status=$?
    printf '\nvalgrind_exit=%s\n' "$valgrind_status"
else
    printf '\nvalgrind_missing=1\n' >&2
    valgrind_status=127
fi

if [ "$normal_status" -ne 0 ]; then
    exit "$normal_status"
fi
exit "$valgrind_status"
