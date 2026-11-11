#!/bin/bash
#
# SPDX-License-Identifier: LGPL-2.1-only
#
# Run env through each exec function pseudo wraps.  The arguments must
# get through, and for the functions given an explicit environment, an
# empty one must get libpseudo back in LD_PRELOAD (as with env -i), and
# one with PSEUDO_UNLOAD must not.

test_exec="$(dirname "$0")/test-exec"

"$test_exec" | grep -q "C=C" || { echo "FAILED: execl"; exit 1; }

rc=0
for fn in execvpe fexecve execveat; do
    out=$("$test_exec" $fn)
    if ! grep -q "C=C" <<< "$out"; then
        echo "FAILED: $fn: arguments not passed"
        rc=1
    fi
    if ! grep -q "^LD_PRELOAD=.*libpseudo" <<< "$out"; then
        echo "FAILED: $fn: pseudo not restored to LD_PRELOAD"
        rc=1
    fi
    out=$("$test_exec" $fn PSEUDO_UNLOAD=1)
    if grep -q "^LD_PRELOAD=.*libpseudo" <<< "$out"; then
        echo "FAILED: $fn: PSEUDO_UNLOAD did not drop pseudo"
        rc=1
    fi
done
exit $rc
