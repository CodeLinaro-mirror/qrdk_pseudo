#!/bin/bash
#
# SPDX-License-Identifier: LGPL-2.1-only
#
# Several clients create, rename, link, chmod and remove the same names in
# one directory at once.  pseudo must not let one client's operation land in
# the middle of another client's change to the same name, or the server
# finds the database out of step with the filesystem.  It then logs an
# "inode mismatch" or "path mismatch" error, and may abort a client.
#
# The temporary tree has to live below $PWD so that pseudo tracks it.

tmpdir="$(mktemp -d -p "$PWD" parallel_ops_XXXXXX)"
trap "rm -rf '$tmpdir'" EXIT TERM INT

log="${PSEUDO_LOCALSTATEDIR:-$PWD/var/pseudo}/pseudo.log"
mismatches() {
    grep -c "mismatch" "$log" 2>/dev/null || true
}
before=$(mismatches)

cd "$tmpdir" || exit 1
: > target

worker() {
    for i in $(seq 50); do
        echo "$1 $i" > "tmp.$1"            || echo "FAILED: create tmp.$1" >> failed
        mv -f "tmp.$1" target              || echo "FAILED: mv tmp.$1 target" >> failed
        chmod 640 target 2>/dev/null
        chown 0:0 target 2>/dev/null
        ln -sf target lnk                  || echo "FAILED: ln -sf target lnk" >> failed
        mkdir dir 2>/dev/null; rmdir dir 2>/dev/null
        mkfifo "f.$1" && mv -f "f.$1" fifo || echo "FAILED: mkfifo/mv f.$1" >> failed
        ln -f target "hard.$1" 2>/dev/null && rm -f "hard.$1"
    done
}

for w in 1 2 3 4; do
    worker $w &
done
wait

rc=0
if [ -e failed ]; then
    sort failed | uniq -c
    rc=1
fi
after=$(mismatches)
if [ "${after:-0}" -ne "${before:-0}" ]; then
    echo "FAILED: $((after - before)) mismatch error(s) in $log"
    rc=1
fi
exit $rc
