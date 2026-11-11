#!/bin/bash
#
# SPDX-License-Identifier: LGPL-2.1-only
#

# Landlock needs a 5.13 kernel built with it enabled. Ask with pseudo out of
# the way, so that pseudo refusing Landlock is not mistaken for the kernel
# lacking it.
PSEUDO_DISABLED=1 ./test/test-landlock --probe
case $? in
0)  ;;
77) exit 255 ;;
*)  exit 1 ;;
esac

file=test-landlock-file
trap "rm -f $file" 0

echo foo > $file || exit 1
chown 1234:5678 $file || exit 1

# A client that cannot reach the server retries for about 90 seconds with
# SIGTERM blocked by the wrapper, so stop it with SIGKILL instead.
timeout -s KILL 10s ./test/test-landlock 1234 5678 < $file
