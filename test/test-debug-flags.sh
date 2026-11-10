#!/bin/bash
#
# SPDX-License-Identifier: LGPL-2.1-only
#
# Check how debugging and severity settings reach programs run under pseudo:
# - each -v adds the next debugging flag to those already set;
# - a numeric PSEUDO_DEBUG turns on exactly that many flags, even in a process
#   which initializes the client library more than once;
# - PSEUDO_SEVERITY is passed on as severity names, from the default or -k.

tmpdir="$(mktemp -d -p "$PWD" debug_flags_XXXXXX)"
mkdir "$tmpdir/state"

# Run a separate pseudo, with its own state directory, outside of the pseudo
# running this test: PSEUDO_UNLOAD keeps libpseudo out of env(1), which then
# starts pseudo with a clean environment.
new_pseudo() {
	PSEUDO_UNLOAD=1 env -u PSEUDO_UNLOAD -u PSEUDO_DEBUG -u PSEUDO_SEVERITY \
		PSEUDO_LOCALSTATEDIR="$tmpdir/state" ./bin/pseudo "$@"
}
trap "new_pseudo -S >/dev/null 2>&1; rm -rf '$tmpdir'" EXIT TERM INT

rc=0
fail() {
	echo "FAILED: $*"
	rc=1
}

# value of a variable in the environment of a program run by a new pseudo
var_under() {
	local var=$1
	shift
	new_pseudo "$@" env | sed -n "s/^$var=//p"
}

check() {
	local want=$1 got=$2
	shift 2
	[ "$got" = "$want" ] || fail "$*: got '$got', expected '$want'"
}

check nfo "$(var_under PSEUDO_DEBUG -v -v -v)" "pseudo -v -v -v"
check no "$(var_under PSEUDO_DEBUG -x o -v)" "pseudo -x o -v"
check info,warn,error "$(var_under PSEUDO_SEVERITY)" "default PSEUDO_SEVERITY"
check warn,error "$(var_under PSEUDO_SEVERITY -k warn,error)" "pseudo -k warn,error"

# A level turns on the first flags in order: o (operations, which logs
# "exec" messages) is the third and P (process IDs, "new pid") the fourth.
# The commands initialize the client library more than once.
for level in 2 4; do
	PSEUDO_DEBUG=$level PSEUDO_DEBUG_FILE="$tmpdir/debug.$level" \
		sh -c '/bin/true; sh -c /bin/true' 2>/dev/null
done
grep -q "new pid:" "$tmpdir/debug.2" 2>/dev/null &&
	fail "PSEUDO_DEBUG=2 turned on more than the first two debugging flags"
grep -q "exec /" "$tmpdir/debug.4" 2>/dev/null ||
	fail "PSEUDO_DEBUG=4 didn't turn on the first four debugging flags"

exit $rc
