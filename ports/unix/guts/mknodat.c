/*
 * Copyright (c) 2011 Wind River Systems; see
 * guts/COPYRIGHT for information.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * int mknodat(int dirfd, const char *path, mode_t mode, dev_t dev)
 *	int rc = -1;
 */

 	pseudo_msg_t *msg;
	PSEUDO_STATBUF buf;
        int save_errno = errno;

	/* mask out mode bits appropriately */
	mode = mode & ~pseudo_umask;
        /* if you don't specify a type, assume regular file */
        if (!(mode & S_IFMT)) {
                mode |= S_IFREG;
        }
        pseudo_debug(PDBGF_FILE, "mknodat creating '%s', mode 0%o\n",
                path ? path : "<no name>", (int) mode);

	rc = base_fstatat(dirfd, path, &buf, AT_SYMLINK_NOFOLLOW);
	if (rc != -1) {
		/* if we can stat the file, you can't mknod it */
		errno = EEXIST;
		return -1;
	}
	rc = real_openat(dirfd, path, O_CREAT | O_WRONLY | O_EXCL,
		PSEUDO_FS_MODE(mode, 0));
	if (rc == -1) {
		return -1;
	}
	real_fchmod(rc, PSEUDO_FS_MODE(mode, 0));
	base_fstat(rc, &buf);
	/* mknod does not really open the file.  We don't have
	 * to use wrap_close because we've never exposed this file
	 * descriptor to the client code.
	 */
	real_close(rc);

	/* mask in the mode type bits again */
	buf.st_mode = (PSEUDO_DB_MODE(buf.st_mode, mode) & 07777) |
			(mode & ~07777);
	buf.st_rdev = dev;
	msg = pseudo_client_op(OP_MKNOD, 0, -1, dirfd, path, &buf);
	if (msg && msg->result != RESULT_SUCCEED) {
		errno = EPERM;
		rc = -1;
	} else {
		/* just pretend we worked */
                errno = save_errno;
		rc = 0;
	}
	if (rc == -1) {
                save_errno = errno;
		real_unlinkat(dirfd, path, AT_SYMLINK_NOFOLLOW);
		errno = save_errno;
	}

/*	return rc;
 * }
 */
