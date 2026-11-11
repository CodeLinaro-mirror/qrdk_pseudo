/*
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Run "/usr/bin/env A=A B=B C=C" through an exec function.
 *
 * usage: test-exec [execl|execvpe|fexecve|execveat [VAR=value...]]
 *
 * execl() keeps the caller's environment; the others pass only the
 * VAR=value arguments given, so pseudo has to add its own variables back
 * (or leave them out, if PSEUDO_UNLOAD is among them).
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
	char *args[] = { "env", "A=A", "B=B", "C=C", NULL };
	/* argv[2] onwards, followed by argv's terminating NULL */
	char **envp = argc > 2 ? argv + 2 : argv + argc;
	const char *fn = argc > 1 ? argv[1] : "execl";
	int fd;

	if (!strcmp(fn, "execl")) {
		return execl("/usr/bin/env", "/usr/bin/env", "A=A", "B=B", "C=C", NULL);
	} else if (!strcmp(fn, "execvpe")) {
		/* searched for in the caller's PATH */
		execvpe("env", args, envp);
	} else if (!strcmp(fn, "fexecve")) {
		fd = open("/usr/bin/env", O_RDONLY | O_CLOEXEC);
		if (fd == -1) {
			perror("/usr/bin/env");
			return 2;
		}
		fexecve(fd, args, envp);
	} else if (!strcmp(fn, "execveat")) {
		fd = open("/usr/bin", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
		if (fd == -1) {
			perror("/usr/bin");
			return 2;
		}
		execveat(fd, "env", args, envp, 0);
	} else {
		fprintf(stderr, "unknown function '%s'\n", fn);
		return 2;
	}
	perror(fn);
	return 2;
}
