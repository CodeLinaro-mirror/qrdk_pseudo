/* 
 * Copyright (c) 2008-2010, 2012, 2013 Wind River Systems; see
 * guts/COPYRIGHT for information.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * static int
 * wrap_fchmodat(int dirfd, const char *path, mode_t mode, int flags) {
 *	int rc = -1;
 */
	PSEUDO_STATBUF buf;
	int save_errno = errno;

	rc = base_fstatat(dirfd, path, &buf, flags);
	if (rc == -1) {
		return rc;
	}
	if (S_ISLNK(buf.st_mode)) {
		/* according to docs, "chmod on a symbolic link always succeeds and has no effect" */
		return 0;
	}
	save_errno = errno;

#if 0
 	pseudo_msg_t *msg;
	/* purely for debugging purposes:  check whether file
	 * is already in database. We don't need the resulting
	 * information for anything. This is currently ifdefed
	 * out because it's only useful when trying to track where
	 * files are coming from.
	 */
	msg = pseudo_client_op(OP_STAT, 0, -1, -1, path, &buf);
	if (!msg || msg->result != RESULT_SUCCEED) {
		pseudo_debug(PDBGF_FILE, "chmodat to 0%o on %d/%s, ino %llu, new file.\n",
			mode, dirfd, path, (unsigned long long) buf.st_ino);

	}
#endif

	/* user bits added so "root" can always access files. */
	rc = real_fchmodat(dirfd, path, PSEUDO_FS_MODE(mode, S_ISDIR(buf.st_mode)), flags);
	/* we otherwise ignore failures from underlying fchmod, because pseudo
	 * may believe you are permitted to change modes that the filesystem
	 * doesn't. Note that we also don't need to know whether the
	 * file might be a (pseudo) block device or some such; pseudo
	 * will only modify permission bits based on an OP_CHMOD, and does
	 * not care about device/file type mismatches, only directory/file
	 * or symlink/file.
	 */

	buf.st_mode = (buf.st_mode & ~07777) | (mode & 07777);
	pseudo_client_op(OP_CHMOD, 0, -1, dirfd, path, &buf);
	/* don't change errno from what it was originally */
	errno = save_errno;
	rc = 0;

/*	return rc;
 * }
 */
