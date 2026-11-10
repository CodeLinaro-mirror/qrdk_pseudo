/* 
 * Copyright (c) 2008-2010 Wind River Systems; see
 * guts/COPYRIGHT for information.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * static int
 * wrap___fxstatat64(int ver, int dirfd, const char *path, struct stat64 *buf, int flags) {
 *	int rc = -1;
 */
	pseudo_msg_t *msg;
	int save_errno;

	if (flags & AT_SYMLINK_NOFOLLOW) {
		rc = real___fxstatat64(ver, dirfd, path, buf, flags);
		if (rc == -1) {
			return rc;
		}
	} else {
		rc = real___fxstatat64(ver, dirfd, path, buf, flags);
		if (rc == -1) {
			return rc;
		}
	}
	save_errno = errno;

	if (ver != _STAT_VER) {
		pseudo_debug(PDBGF_CLIENT, "version mismatch: got stat version %d, only supporting %d\n", ver, _STAT_VER);
		errno = save_errno;
		return rc;
	}

	/* query database
	 * note that symlink canonicalizing is now automatic, so we
	 * don't need to check for a symlink on this end
	 */
	msg = pseudo_client_op(OP_STAT, 0, -1, dirfd, path, buf);
	if (msg && msg->result == RESULT_SUCCEED) {
		pseudo_stat_msg(buf, msg);
	}

	errno = save_errno;

/*	return rc;
 * }
 */
