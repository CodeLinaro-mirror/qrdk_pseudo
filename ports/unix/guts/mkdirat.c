/* 
 * Copyright (c) 2008-2010, 2012 Wind River Systems; see
 * guts/COPYRIGHT for information.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * static int
 * wrap_mkdirat(int dirfd, const char *path, mode_t mode) {
 *	int rc = -1;
 */
	/* mask out mode bits appropriately */
	mode = mode & ~pseudo_umask;
	rc = real_mkdirat(dirfd, path, PSEUDO_FS_MODE(mode, 1));
	if (rc != -1) {
		PSEUDO_STATBUF buf;
		int stat_rc;
		int save_errno = errno;

		stat_rc = base_fstatat(dirfd, path, &buf, AT_SYMLINK_NOFOLLOW);
		if (stat_rc != -1) {
			buf.st_mode = PSEUDO_DB_MODE(buf.st_mode, mode);
			pseudo_client_op(OP_MKDIR, 0, -1, dirfd, path, &buf);
			real_fchmodat(dirfd, path, PSEUDO_FS_MODE(mode, 1), 0);
		} else {
			pseudo_debug(PDBGF_OP, "mkdir of %s succeeded, but stat failed: %s\n",
				path, strerror(errno));
		}
		errno = save_errno;
	}

/*	return rc;
 * }
 */
