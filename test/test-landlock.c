/*
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * xz (5.6 and later) sandboxes itself with Landlock when it works on a
 * pipe, revoking every filesystem access right before it does any work.
 * libpseudo only connects to the server on the first operation it has to
 * report, and connecting needs filesystem access (open(".") and fchdir()
 * to the state directory, the pidfile, spawning a server), so under pseudo
 * the first operation after the sandbox goes up fails. Do what xz does,
 * then fstat() a descriptor that was opened beforehand and check that
 * pseudo still reports the ownership it recorded for it.
 *
 * If libpseudo refuses Landlock (as it does seccomp), carry on without the
 * sandbox the way xz does; only the result of the fstat() matters.
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <linux/types.h>

/* matches test-openat2-syscall: the shell wrapper turns 77 into "skipped" */
#define SKIP 77

/* <linux/landlock.h> and the syscall numbers are missing on hosts with
 * pre-5.13 kernel headers.  The numbers are the same on every architecture.
 */
#ifndef __NR_landlock_create_ruleset
#define __NR_landlock_create_ruleset 444
#endif
#ifndef __NR_landlock_restrict_self
#define __NR_landlock_restrict_self 446
#endif

struct test_landlock_ruleset_attr {
	__u64 handled_access_fs;
};

#define TEST_LANDLOCK_CREATE_RULESET_VERSION	(1U << 0)

/* ABI 1: 13 rights, then REFER (2), TRUNCATE (3), IOCTL_DEV (5) */
#define TEST_LANDLOCK_ACCESS_FS_ABI1		((1ULL << 13) - 1)
#define TEST_LANDLOCK_ACCESS_FS_REFER		(1ULL << 13)
#define TEST_LANDLOCK_ACCESS_FS_TRUNCATE	(1ULL << 14)
#define TEST_LANDLOCK_ACCESS_FS_IOCTL_DEV	(1ULL << 15)

static int landlock_abi(void) {
	return (int) syscall(__NR_landlock_create_ruleset, NULL, 0,
		TEST_LANDLOCK_CREATE_RULESET_VERSION);
}

/* Is Landlock there at all? The wrapper runs this with PSEUDO_DISABLED
 * set, so the answer is the kernel's.
 */
static int probe(void) {
	if (landlock_abi() < 1) {
		if (errno == ENOSYS || errno == EOPNOTSUPP)
			return SKIP;
		perror("landlock_create_ruleset(VERSION)");
		return 1;
	}
	return 0;
}

/* Revoke every filesystem access right the kernel knows about, as xz's
 * sandbox_enable_strict_if_allowed() does. Returns 0 if the sandbox is up,
 * 1 if Landlock is not available, -1 on error.
 */
static int sandbox(void) {
	struct test_landlock_ruleset_attr attr = { 0 };
	int abi, fd;

	abi = landlock_abi();
	if (abi < 1) {
		if (errno == ENOSYS || errno == EOPNOTSUPP)
			return 1;
		perror("landlock_create_ruleset(VERSION)");
		return -1;
	}
	attr.handled_access_fs = TEST_LANDLOCK_ACCESS_FS_ABI1;
	if (abi >= 2)
		attr.handled_access_fs |= TEST_LANDLOCK_ACCESS_FS_REFER;
	if (abi >= 3)
		attr.handled_access_fs |= TEST_LANDLOCK_ACCESS_FS_TRUNCATE;
	if (abi >= 5)
		attr.handled_access_fs |= TEST_LANDLOCK_ACCESS_FS_IOCTL_DEV;

	fd = (int) syscall(__NR_landlock_create_ruleset, &attr, sizeof(attr), 0);
	if (fd == -1) {
		perror("landlock_create_ruleset");
		return -1;
	}
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
		perror("prctl(PR_SET_NO_NEW_PRIVS)");
		close(fd);
		return -1;
	}
	if (syscall(__NR_landlock_restrict_self, fd, 0) == -1) {
		perror("landlock_restrict_self");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int main(int argc, char **argv) {
	struct stat st;
	uid_t uid;
	gid_t gid;
	int rc;

	if (argc > 1 && !strcmp(argv[1], "--probe"))
		return probe();
	if (argc != 3) {
		fprintf(stderr, "usage: %s --probe | uid gid < file\n", argv[0]);
		return 1;
	}
	uid = (uid_t) strtoul(argv[1], NULL, 10);
	gid = (gid_t) strtoul(argv[2], NULL, 10);

	/* nothing above this point has needed the pseudo server */
	rc = sandbox();
	if (rc < 0)
		return 1;
	if (rc > 0)
		fprintf(stderr, "Landlock unavailable, running unsandboxed\n");

	if (fstat(0, &st) == -1) {
		perror("fstat(stdin)");
		return 1;
	}
	if (st.st_uid != uid || st.st_gid != gid) {
		fprintf(stderr, "fstat(stdin): owner %d:%d, expected %d:%d\n",
			(int) st.st_uid, (int) st.st_gid, (int) uid, (int) gid);
		return 1;
	}
	return 0;
}
