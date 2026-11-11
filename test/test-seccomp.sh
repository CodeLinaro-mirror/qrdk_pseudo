#!/bin/bash
#
# SPDX-License-Identifier: LGPL-2.1-only
#

# rc 77 means the kernel doesn't report seccomp status, so we're skipped
./test/test-seccomp
case $? in
0)  exit 0 ;;
77) exit 255 ;;
*)  exit 1 ;;
esac
