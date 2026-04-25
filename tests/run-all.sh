#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Collect test run scripts under tests/<BUG-ID>/run.sh
mapfile -t RUN_SCRIPTS < <(find . -mindepth 2 -maxdepth 2 -type f -name run.sh | sort)

if [ "${#RUN_SCRIPTS[@]}" -eq 0 ]; then
    echo "No test run.sh scripts found under tests/<BUG-ID>/"
    exit 1
fi

pass_count=0
fail_count=0
failed=()

for run_script in "${RUN_SCRIPTS[@]}"; do
    test_dir="$(dirname "$run_script")"
    test_id="${test_dir#./}"

    echo "============================================================"
    echo "Running ${test_id}"
    echo "------------------------------------------------------------"

    status=0
    sh "$run_script" || status=$?

    if [ "$status" -eq 0 ]; then
        echo "[PASS] ${test_id}"
        pass_count=$((pass_count + 1))
    else
        echo "[FAIL] ${test_id} (exit=${status})"
        fail_count=$((fail_count + 1))
        failed+=("${test_id}:${status}")
    fi

    echo
 done

echo "============================================================"
echo "Test summary: pass=${pass_count} fail=${fail_count} total=$((pass_count + fail_count))"

if [ "$fail_count" -ne 0 ]; then
    echo "Failed tests:"
    for item in "${failed[@]}"; do
        echo "  - ${item}"
    done
    exit 1
fi

exit 0
