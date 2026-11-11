/*
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * A seccomp sandbox restricts libpseudo along with the program, so pseudo
 * pretends to install one.  Check that every way of asking for one, by
 * prctl() and by the seccomp() system call, in strict and filter mode,
 * returns success without leaving the process under seccomp.  Strict mode
 * would kill the process at the next call other than read(), write() or
 * exit(), so each case runs in a child of its own.
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/filter.h>
#include <linux/seccomp.h>

/* matches test-openat2-syscall: the shell wrapper turns 77 into "skipped" */
#define SKIP 77

/* Seccomp: field of /proc/self/status, or -1 if there is none */
static int seccomp_mode(void) {
	char line[256];
	int mode = -1;
	FILE *f;

	f = fopen("/proc/self/status", "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f))
		if (!strncmp(line, "Seccomp:", 8))
			mode = atoi(line + 8);
	fclose(f);
	return mode;
}

static struct sock_filter allow_all[] = {
	BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
};
static struct sock_fprog prog = {
	.len = sizeof(allow_all) / sizeof(allow_all[0]),
	.filter = allow_all,
};

static long request(const char *how) {
	if (!strcmp(how, "prctl-strict"))
		return prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, 0, 0, 0);
	if (!strcmp(how, "prctl-filter"))
		return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0);
	if (!strcmp(how, "syscall-strict"))
		return syscall(SYS_seccomp, SECCOMP_SET_MODE_STRICT, 0, NULL);
	if (!strcmp(how, "syscall-filter"))
		return syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog);
	errno = EINVAL;
	return -1;
}

static int check(const char *how) {
	int status;
	pid_t pid;

	fflush(NULL);
	pid = fork();
	if (pid == -1) {
		perror("fork");
		return 1;
	}
	if (pid == 0) {
		long rc;
		int mode;

		if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
			perror("prctl(PR_SET_NO_NEW_PRIVS)");
			_exit(1);
		}
		rc = request(how);
		if (rc != 0) {
			fprintf(stderr, "%s: returned %ld (%s)\n", how, rc,
				rc == -1 ? strerror(errno) : "?");
			_exit(1);
		}
		mode = seccomp_mode();
		if (mode != 0) {
			fprintf(stderr, "%s: Seccomp: %d after the call\n", how, mode);
			_exit(1);
		}
		_exit(0);
	}
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return 1;
	}
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "%s: killed by signal %d\n", how, WTERMSIG(status));
		return 1;
	}
	return WEXITSTATUS(status) != 0;
}

int main(void) {
	static const char *hows[] = {
		"prctl-strict", "prctl-filter", "syscall-strict", "syscall-filter",
	};
	size_t i;
	int failed = 0;

	if (seccomp_mode() == -1) {
		fprintf(stderr, "no Seccomp: in /proc/self/status\n");
		return SKIP;
	}
	for (i = 0; i < sizeof(hows) / sizeof(hows[0]); ++i)
		failed |= check(hows[i]);
	return failed;
}
