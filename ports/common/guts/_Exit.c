/*
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * void _Exit(int status)
 */

	/* NOTE THAT THIS IS NEVER USED!
	 * The hand-written _Exit() in ../pseudo_wrappers.c flushes
	 * asynchronous requests and exits directly.
	 */

	real__Exit(status);

/*	return;
 * }
 */
