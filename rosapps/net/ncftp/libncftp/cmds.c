/* cmds.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

int
FTPChdir(const FTPCIPtr cip, const char *const cdCwd)
{
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (cdCwd == NULL) {
		result = kErrInvalidDirParam;
		cip->errNo = kErrInvalidDirParam;
	} else {
		if (cdCwd[0] == '\0')	/* But allow FTPChdir(cip, ".") to go through. */
			result = 2;	
		else if (strcmp(cdCwd, "..") == 0)
			result = FTPCmd(cip, "CDUP");
		else
			result = FTPCmd(cip, "CWD %s", cdCwd);
		if (result >= 0) {
			if (result == 2) {
				result = kNoErr;
			} else {
				result = kErrCWDFailed;
				cip->errNo = kErrCWDFailed;
			}
		}
	}
	return (result);
}	/* FTPChdir */




int
FTPChmod(const FTPCIPtr cip, const char *const pattern, const char *const mode, const int doGlob)
{
	LineList fileList;
	LinePtr filePtr;
	char *file;
	int onceResult, batchResult;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	batchResult = FTPRemoteGlob(cip, &fileList, pattern, doGlob);
	if (batchResult != kNoErr)
		return (batchResult);

	for (batchResult = kNoErr, filePtr = fileList.first;
		filePtr != NULL;
		filePtr = filePtr->next)
	{
		file = filePtr->line;
		if (file == NULL) {
			batchResult = kErrBadLineList;
			cip->errNo = kErrBadLineList;
			break;
		}
		onceResult = FTPCmd(cip, "SITE CHMOD %s %s", mode, file); 	
		if (onceResult < 0) {
			batchResult = onceResult;
			break;
		}
		if (onceResult != 2) {
			batchResult = kErrChmodFailed;
			cip->errNo = kErrChmodFailed;
		}
	}
	DisposeLineListContents(&fileList);
	return (batchResult);
}	/* FTPChmod */




static int
FTPRmdirRecursiveL2(const FTPCIPtr cip)
{
	LineList fileList;
	LinePtr filePtr;
	char *file;
	int result;

	result = FTPRemoteGlob(cip, &fileList, "**", kGlobYes);
	if (result != kNoErr) {
		return (result);
	}

	for (filePtr = fileList.first;
		filePtr != NULL;
		filePtr = filePtr->next)
	{
		file = filePtr->line;
		if (file == NULL) {
			cip->errNo = kErrBadLineList;
			break;
		}

		if ((file[0] == '.') && ((file[1] == '\0') || ((file[1] == '.') && (file[2] == '\0'))))
			continue;	/* Skip . and .. */

		if (FTPChdir(cip, file) == kNoErr) {
			/* It was a directory.
			 * Go in and wax it.
			 */
			result = FTPRmdirRecursiveL2(cip);

			if (FTPChdir(cip, "..") != kNoErr) {
				/* Panic -- we can no longer
				 * cd back to the directory
				 * we were in before.
				 */
				result = kErrCannotGoToPrevDir;
				cip->errNo = kErrCannotGoToPrevDir;
				return (result);
			}

			if ((result < 0) && (result != kErrGlobNoMatch))
				return (result);

			result = FTPRmdir(cip, file, kRecursiveNo, kGlobNo);
			if (result != kNoErr) {
				/* Well, we couldn't remove the empty
				 * directory.  Perhaps we screwed up
				 * and the directory wasn't empty.
				 */
				return (result);
			}
		} else {
			/* Assume it was a file -- remove it. */
			result = FTPDelete(cip, file, kRecursiveNo, kGlobNo);
			/* Try continuing to remove the rest,
			 * even if this failed.
			 */
		}
	}
	DisposeLineListContents(&fileList);

	return (result);
} 	/* FTPRmdirRecursiveL2 */



static int
FTPRmdirRecursive(const FTPCIPtr cip, const char *const dir)
{
	int result, result2;

	/* Preserve old working directory. */
	(void) FTPGetCWD(cip, cip->buf, cip->bufSize);

	result = FTPChdir(cip, dir);
	if (result != kNoErr) {
		return (result);
	}

	result = FTPRmdirRecursiveL2(cip);

	if (FTPChdir(cip, cip->buf) != kNoErr) {
		/* Could not cd back to the original user directory -- bad. */
		if (result != kNoErr) {
			result = kErrCannotGoToPrevDir;
			cip->errNo = kErrCannotGoToPrevDir;
		}
		return (result);
	}

	/* Now rmdir the last node, the root of the tree
	 * we just went through.
	 */
	result2 = FTPRmdir(cip, dir, kRecursiveNo, kGlobNo);
	if ((result2 != kNoErr) && (result == kNoErr))
		result = result2;

	return (result);
}	/* FTPRmdirRecursive */




int
FTPDelete(const FTPCIPtr cip, const char *const pattern, const int recurse, const int doGlob)
{
	LineList fileList;
	LinePtr filePtr;
	char *file;
	int onceResult, batchResult;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	batchResult = FTPRemoteGlob(cip, &fileList, pattern, doGlob);
	if (batchResult != kNoErr)
		return (batchResult);

	for (batchResult = kNoErr, filePtr = fileList.first;
		filePtr != NULL;
		filePtr = filePtr->next)
	{
		file = filePtr->line;
		if (file == NULL) {
			batchResult = kErrBadLineList;
			cip->errNo = kErrBadLineList;
			break;
		}
		onceResult = FTPCmd(cip, "DELE %s", file);
		if (onceResult < 0) {
			batchResult = onceResult;
			break;
		}
		if (onceResult != 2) {
			if (recurse != kRecursiveYes) {
				batchResult = kErrDELEFailed;
				cip->errNo = kErrDELEFailed;
			} else {
				onceResult = FTPCmd(cip, "RMD %s", file); 	
				if (onceResult < 0) {
					batchResult = onceResult;
					break;
				}
				if (onceResult != 2) {
					onceResult = FTPRmdirRecursive(cip, file);
					if (onceResult < 0) {
						batchResult = kErrRMDFailed;
						cip->errNo = kErrRMDFailed;
					}
				}
			}
		}
	}
	DisposeLineListContents(&fileList);
	return (batchResult);
}	/* FTPDelete */




int
FTPGetCWD(const FTPCIPtr cip, char *const newCwd, const size_t newCwdSize)
{
	ResponsePtr rp;
	char *l, *r;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((newCwd == NULL) || (newCwdSize == 0)) {
		result = kErrInvalidDirParam;
		cip->errNo = kErrInvalidDirParam;
	} else {
		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
		} else {
			result = RCmd(cip, rp, "PWD");
			if (result == 2) {
				if ((r = strrchr(rp->msg.first->line, '"')) != NULL) {
					/* "xxxx" is current directory.
					 * Strip out just the xxxx to copy into the remote cwd.
					 */
					l = strchr(rp->msg.first->line, '"');
					if ((l != NULL) && (l != r)) {
						*r = '\0';
						++l;
						(void) Strncpy(newCwd, l, newCwdSize);
						*r = '"';	/* Restore, so response prints correctly. */
					}
				} else {
					/* xxxx is current directory.
					 * Mostly for VMS.
					 */
					if ((r = strchr(rp->msg.first->line, ' ')) != NULL) {
						*r = '\0';
						(void) Strncpy(newCwd, (rp->msg.first->line), newCwdSize);
						*r = ' ';	/* Restore, so response prints correctly. */
					}
				}
				result = kNoErr;
			} else if (result > 0) {
				result = kErrPWDFailed;
				cip->errNo = kErrPWDFailed;
			}
			DoneWithResponse(cip, rp);
		}
	}
	return (result);
}	/* FTPGetCWD */




int
FTPChdirAndGetCWD(const FTPCIPtr cip, const char *const cdCwd, char *const newCwd, const size_t newCwdSize)
{
	ResponsePtr rp;
	char *l, *r;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((newCwd == NULL) || (cdCwd == NULL)) {
		result = kErrInvalidDirParam;
		cip->errNo = kErrInvalidDirParam;
	} else {
		if (cdCwd[0] == '\0') {	/* But allow FTPChdir(cip, ".") to go through. */
			result = FTPGetCWD(cip, newCwd, newCwdSize);
			return (result);
		}
		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
		} else {
			if (strcmp(cdCwd, "..") == 0)
				result = RCmd(cip, rp, "CDUP"); 	
			else
				result = RCmd(cip, rp, "CWD %s", cdCwd);
			if (result == 2) {
				l = strchr(rp->msg.first->line, '"');
				if ((l == rp->msg.first->line) && ((r = strrchr(rp->msg.first->line, '"')) != NULL) && (l != r)) {
					/* "xxxx" is current directory.
					 * Strip out just the xxxx to copy into the remote cwd.
					 *
					 * This is nice because we didn't have to do a PWD.
					 */
					*r = '\0';
					++l;
					(void) Strncpy(newCwd, l, newCwdSize);
					*r = '"';	/* Restore, so response prints correctly. */
					DoneWithResponse(cip, rp);
					result = kNoErr;
				} else {
					DoneWithResponse(cip, rp);
					result = FTPGetCWD(cip, newCwd, newCwdSize);
				}
			} else if (result > 0) {
				result = kErrCWDFailed;
				cip->errNo = kErrCWDFailed;
				DoneWithResponse(cip, rp);
			} else {
				DoneWithResponse(cip, rp);
			}
		}
	}
	return (result);
}	/* FTPChdirAndGetCWD */




int
FTPChdir3(FTPCIPtr cip, const char *const cdCwd, char *const newCwd, const size_t newCwdSize, int flags)
{
	char *cp, *startcp;
	int result;
	int lastSubDir;
	int mkd, pwd;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (cdCwd == NULL) {
		result = kErrInvalidDirParam;
		cip->errNo = kErrInvalidDirParam;
		return result;
	}

	if (flags == kChdirOnly)
		return (FTPChdir(cip, cdCwd));
	if (flags == kChdirAndGetCWD) {
		return (FTPChdirAndGetCWD(cip, cdCwd, newCwd, newCwdSize));
	} else if (flags == kChdirAndMkdir) {
		result = FTPMkdir(cip, cdCwd, kRecursiveYes);
		if (result == kNoErr)
			result = FTPChdir(cip, cdCwd);
		return result;
	} else if (flags == (kChdirAndMkdir|kChdirAndGetCWD)) {
		result = FTPMkdir(cip, cdCwd, kRecursiveYes);
		if (result == kNoErr)
			result = FTPChdirAndGetCWD(cip, cdCwd, newCwd, newCwdSize);
		return result;
	}

	/* else: (flags | kChdirOneSubdirAtATime) == true */
	
	cp = cip->buf;
	cp[cip->bufSize - 1] = '\0';
	(void) Strncpy(cip->buf, cdCwd, cip->bufSize);
	if (cp[cip->bufSize - 1] != '\0')
		return (kErrBadParameter);
	
	mkd = (flags & kChdirAndMkdir);
	pwd = (flags & kChdirAndGetCWD);

	if ((cdCwd[0] == '\0') || (strcmp(cdCwd, ".") == 0)) {
		result = 0;
		if (flags == kChdirAndGetCWD)
			result = FTPGetCWD(cip, newCwd, newCwdSize);
		return (result);
	}

	lastSubDir = 0;
	do {
		startcp = cp;
		cp = StrFindLocalPathDelim(cp);
		if (cp != NULL) {
			/* If this is the first slash in an absolute
			 * path, then startcp will be empty.  We will
			 * use this below to treat this as the root
			 * directory.
			 */
			*cp++ = '\0';
		} else {
			lastSubDir = 1;
		}
		if (strcmp(startcp, ".") == 0) {
			result = 0;
			if ((lastSubDir != 0) && (pwd != 0))
				result = FTPGetCWD(cip, newCwd, newCwdSize);
		} else if ((lastSubDir != 0) && (pwd != 0)) {
			result = FTPChdirAndGetCWD(cip, (*startcp != '\0') ? startcp : "/", newCwd, newCwdSize);
		} else {
			result = FTPChdir(cip, (*startcp != '\0') ? startcp : "/");
		}
		if (result < 0) {
			if ((mkd != 0) && (*startcp != '\0')) {
				if (FTPCmd(cip, "MKD %s", startcp) == 2) {
					result = FTPChdir(cip, startcp);
				} else {
					/* couldn't change nor create */
					cip->errNo = result;
				}
			} else {
				cip->errNo = result;
			}
		}
	} while ((!lastSubDir) && (result == 0));

	return (result);
}	/* FTPChdir3 */




int
FTPMkdir2(const FTPCIPtr cip, const char *const newDir, const int recurse, const char *const curDir)
{
	int result, result2;
	char *cp, *newTreeStart, *cp2;
	char dir[512];
	char dir2[512];
	char c;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((newDir == NULL) || (newDir[0] == '\0')) {
		cip->errNo = kErrInvalidDirParam;
		return (kErrInvalidDirParam);
	}

	/* Preserve old working directory. */
	if ((curDir == NULL) || (curDir[0] == '\0')) {
		/* This hack is nice so you can eliminate an
		 * unnecessary "PWD" command on the server,
		 * since if you already knew what directory
		 * you're in.  We want to minimize the number
		 * of client-server exchanges when feasible.
		 */
		(void) FTPGetCWD(cip, cip->buf, cip->bufSize);
	}

	result = FTPChdir(cip, newDir);
	if (result == kNoErr) {
		/* Directory already exists -- but we
		 * must now change back to where we were.
		 */
		result2 = FTPChdir(cip, ((curDir == NULL) || (curDir[0] == '\0')) ? cip->buf : curDir);
		if (result2 < 0) {
			result = kErrCannotGoToPrevDir;
			cip->errNo = kErrCannotGoToPrevDir;
			return (result);
		}

		/* Don't need to create it. */
		return (kNoErr);
	}

	if (recurse == kRecursiveNo) {
		result = FTPCmd(cip, "MKD %s", newDir);
		if (result > 0) {
			if (result != 2) {
				Error(cip, kDontPerror, "MKD %s failed; [%s]\n", newDir, cip->lastFTPCmdResultStr);
				result = kErrMKDFailed;
				cip->errNo = kErrMKDFailed;
				return (result);
			} else {
				result = kNoErr;
			}
		}
	} else {
		(void) STRNCPY(dir, newDir);

		/* Strip trailing slashes. */
		cp = dir + strlen(dir) - 1;
		for (;;) {
			if (cp <= dir) {
				if ((newDir == NULL) || (newDir[0] == '\0')) {
					cip->errNo = kErrInvalidDirParam;
					result = kErrInvalidDirParam;
					return (result);
				}
			}
			if ((*cp != '/') && (*cp != '\\')) {
				cp[1] = '\0';
				break;
			}
			--cp;
		}
		(void) STRNCPY(dir2, dir);

		if ((strrchr(dir, '/') == dir) || (strrchr(dir, '\\') == dir)) {
			/* Special case "mkdir /subdir" */
			result = FTPCmd(cip, "MKD %s", dir);
			if (result < 0) {
				return (result);
			}
			if (result != 2) {
				Error(cip, kDontPerror, "MKD %s failed; [%s]\n", dir, cip->lastFTPCmdResultStr);
				result = kErrMKDFailed;
				cip->errNo = kErrMKDFailed;
				return (result);
			}
			/* Haven't chdir'ed, don't need to goto goback. */
			return (kNoErr);
		}

		for (;;) {
			cp = strrchr(dir, '/');
			if (cp == NULL)
				cp = strrchr(dir, '\\');
			if (cp == NULL) {
				cp = dir + strlen(dir) - 1;
				if (dir[0] == '\0') {
					result = kErrMKDFailed;
					cip->errNo = kErrMKDFailed;
					return (result);
				}
				/* Note: below we will refer to cp + 1
				 * which is why we set cp to point to
				 * the byte before the array begins!
				 */
				cp = dir - 1;
				break;
			}
			if (cp == dir) {
				result = kErrMKDFailed;
				cip->errNo = kErrMKDFailed;
				return (result);
			}
			*cp = '\0';
			result = FTPChdir(cip, dir);
			if (result == 0) {
				break;	/* Found a valid parent dir. */
				/* from this point, we need to preserve old dir. */
			}
		}

		newTreeStart = dir2 + ((cp + 1) - dir);
		for (cp = newTreeStart; ; ) {
			cp2 = cp;
			cp = strchr(cp2, '/');
			c = '/';
			if (cp == NULL)
				cp = strchr(cp2, '\\');
			if (cp != NULL) {
				c = *cp;
				*cp = '\0';
				if (cp[1] == '\0') {
					/* Done, if they did "mkdir /tmp/dir/" */
					break;
				}
			}
			result = FTPCmd(cip, "MKD %s", newTreeStart);
			if (result < 0) {
				return (result);
			}
			if (result != 2) {
				Error(cip, kDontPerror, "Cwd=%s; MKD %s failed; [%s]\n", cip->buf, newTreeStart, cip->lastFTPCmdResultStr);
				result = kErrMKDFailed;
				cip->errNo = kErrMKDFailed;
				goto goback;
			}
			if (cp == NULL)
				break;	/* No more to make, done. */
			*cp++ = c;
		}
		result = kNoErr;

goback:
		result2 = FTPChdir(cip, ((curDir == NULL) || (curDir[0] == '\0')) ? cip->buf : curDir);
		if ((result == 0) && (result2 < 0)) {
			result = kErrCannotGoToPrevDir;
			cip->errNo = kErrCannotGoToPrevDir;
		}
	}
	return (result);
}	/* FTPMkdir2 */



int
FTPMkdir(const FTPCIPtr cip, const char *const newDir, const int recurse)
{
	return (FTPMkdir2(cip, newDir, recurse, NULL));
}	/* FTPMkdir */



int
FTPFileModificationTime(const FTPCIPtr cip, const char *const file, time_t *const mdtm)
{
	int result;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((mdtm == NULL) || (file == NULL))
		return (kErrBadParameter);
	*mdtm = kModTimeUnknown;

	if (cip->hasMDTM == kCommandNotAvailable) {
		cip->errNo = kErrMDTMNotAvailable;
		result = kErrMDTMNotAvailable;
	} else {
		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
		} else {
			result = RCmd(cip, rp, "MDTM %s", file);
			if (result < 0) {
				DoneWithResponse(cip, rp);
				return (result);
			} else if (strncmp(rp->msg.first->line, "19100", 5) == 0) {
				Error(cip, kDontPerror, "Warning: Server has Y2K Bug in \"MDTM\" command.\n");
				cip->errNo = kErrMDTMFailed;
				result = kErrMDTMFailed;
			} else if (result == 2) {
				*mdtm = UnMDTMDate(rp->msg.first->line);
				cip->hasMDTM = kCommandAvailable;
				result = kNoErr;
			} else if (UNIMPLEMENTED_CMD(rp->code)) {
				cip->hasMDTM = kCommandNotAvailable;
				cip->errNo = kErrMDTMNotAvailable;
				result = kErrMDTMNotAvailable;
			} else {
				cip->errNo = kErrMDTMFailed;
				result = kErrMDTMFailed;
			}
			DoneWithResponse(cip, rp);
		}
	}
	return (result);
}	/* FTPFileModificationTime */




int
FTPRename(const FTPCIPtr cip, const char *const oldname, const char *const newname)
{
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	if ((oldname == NULL) || (oldname[0] == '\0'))
		return (kErrBadParameter);
	if ((newname == NULL) || (oldname[0] == '\0'))
		return (kErrBadParameter);

	
	result = FTPCmd(cip, "RNFR %s", oldname);
	if (result < 0)
		return (result);
	if (result != 3) {
		cip->errNo = kErrRenameFailed;
		return (cip->errNo);
	}
	
	result = FTPCmd(cip, "RNTO %s", newname);
	if (result < 0)
		return (result);
	if (result != 2) {
		cip->errNo = kErrRenameFailed;
		return (cip->errNo);
	}
	return (kNoErr);
}	/* FTPRename */




int
FTPRemoteHelp(const FTPCIPtr cip, const char *const pattern, const LineListPtr llp)
{
	int result;
	ResponsePtr rp;

	if ((cip == NULL) || (llp == NULL))
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	InitLineList(llp);
	rp = InitResponse();
	if (rp == NULL) {
		result = kErrMallocFailed;
		cip->errNo = kErrMallocFailed;
		Error(cip, kDontPerror, "Malloc failed.\n");
	} else {
		if ((pattern == NULL) || (*pattern == '\0'))
			result = RCmd(cip, rp, "HELP");
		else
			result = RCmd(cip, rp, "HELP %s", pattern);
		if (result < 0) {
			DoneWithResponse(cip, rp);
			return (result);
		} else if (result == 2) {
			if (CopyLineList(llp, &rp->msg) < 0) {
				result = kErrMallocFailed;
				cip->errNo = kErrMallocFailed;
				Error(cip, kDontPerror, "Malloc failed.\n");
			} else {
				result = kNoErr;
			}
		} else {
			cip->errNo = kErrHELPFailed;
			result = kErrHELPFailed;
		}
		DoneWithResponse(cip, rp);
	}
	return (result);
}	/* FTPRemoteHelp */




int
FTPRmdir(const FTPCIPtr cip, const char *const pattern, const int recurse, const int doGlob)
{
	LineList fileList;
	LinePtr filePtr;
	char *file;
	int onceResult, batchResult;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	batchResult = FTPRemoteGlob(cip, &fileList, pattern, doGlob);
	if (batchResult != kNoErr)
		return (batchResult);

	for (batchResult = kNoErr, filePtr = fileList.first;
		filePtr != NULL;
		filePtr = filePtr->next)
	{
		file = filePtr->line;
		if (file == NULL) {
			batchResult = kErrBadLineList;
			cip->errNo = kErrBadLineList;
			break;
		}
		onceResult = FTPCmd(cip, "RMD %s", file); 	
		if (onceResult < 0) {
			batchResult = onceResult;
			break;
		}
		if (onceResult != 2) {
			if (recurse == kRecursiveYes) {
				onceResult = FTPRmdirRecursive(cip, file);
				if (onceResult < 0) {
					batchResult = kErrRMDFailed;
					cip->errNo = kErrRMDFailed;
				}
			} else {
				batchResult = kErrRMDFailed;
				cip->errNo = kErrRMDFailed;
			}
		}
	}
	DisposeLineListContents(&fileList);
	return (batchResult);
}	/* FTPRmdir */




int
FTPSetTransferType(const FTPCIPtr cip, int type)
{
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (cip->curTransferType != type) {
		switch (type) {
			case kTypeAscii:
			case kTypeBinary:
			case kTypeEbcdic:
				break;
			case 'i':
			case 'b':
			case 'B':
				type = kTypeBinary;
				break;
			case 'e':
				type = kTypeEbcdic;
				break;
			case 'a':
				type = kTypeAscii;
				break;
			default:
				/* Yeah, we don't support Tenex.  Who cares? */
				Error(cip, kDontPerror, "Bad transfer type [%c].\n", type);
				cip->errNo = kErrBadTransferType;
				return (kErrBadTransferType);
		}
		result = FTPCmd(cip, "TYPE %c", type);
		if (result != 2) {
			result = kErrTYPEFailed;
			cip->errNo = kErrTYPEFailed;
			return (result);
		}
		cip->curTransferType = type;
	}
	return (kNoErr);
}	/* FTPSetTransferType */




/* If the remote host supports the SIZE command, we can find out the exact
 * size of a remote file, depending on the transfer type in use.  SIZE
 * returns different values for ascii and binary modes!
 */
int
FTPFileSize(const FTPCIPtr cip, const char *const file, longest_int *const size, const int type)
{
	int result;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((size == NULL) || (file == NULL))
		return (kErrBadParameter);
	*size = kSizeUnknown;

	result = FTPSetTransferType(cip, type);
	if (result < 0)
		return (result);

	if (cip->hasSIZE == kCommandNotAvailable) {
		cip->errNo = kErrSIZENotAvailable;
		result = kErrSIZENotAvailable;
	} else {
		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
		} else {
			result = RCmd(cip, rp, "SIZE %s", file);
			if (result < 0) {
				DoneWithResponse(cip, rp);
				return (result);
			} else if (result == 2) {
#if defined(HAVE_LONG_LONG) && defined(SCANF_LONG_LONG)
				(void) sscanf(rp->msg.first->line, SCANF_LONG_LONG, size);
#elif defined(HAVE_LONG_LONG) && defined(HAVE_STRTOQ)
				*size = (longest_int) strtoq(rp->msg.first->line, NULL, 0);
#else
				(void) sscanf(rp->msg.first->line, "%ld", size);
#endif
				cip->hasSIZE = kCommandAvailable;
				result = kNoErr;
			} else if (UNIMPLEMENTED_CMD(rp->code)) {
				cip->hasSIZE = kCommandNotAvailable;
				cip->errNo = kErrSIZENotAvailable;
				result = kErrSIZENotAvailable;
			} else {
				cip->errNo = kErrSIZEFailed;
				result = kErrSIZEFailed;
			}
			DoneWithResponse(cip, rp);
		}
	}
	return (result);
}	/* FTPFileSize */




int
FTPMListOneFile(const FTPCIPtr cip, const char *const file, const MLstItemPtr mlip)
{
	int result;
	ResponsePtr rp;

	/* We do a special check for older versions of NcFTPd which
	 * are based off of an incompatible previous version of IETF
	 * extensions.
	 *
	 * Roxen also seems to be way outdated, where MLST was on the
	 * data connection among other things.
	 *
	 */
	if (
		(cip->hasMLST == kCommandNotAvailable) ||
		((cip->serverType == kServerTypeNcFTPd) && (cip->ietfCompatLevel < 19981201)) ||
		(cip->serverType == kServerTypeRoxen)
	) {
		cip->errNo = kErrMLSTNotAvailable;
		return (cip->errNo);
	}

	rp = InitResponse();
	if (rp == NULL) {
		result = cip->errNo = kErrMallocFailed;
		Error(cip, kDontPerror, "Malloc failed.\n");
	} else {
		result = RCmd(cip, rp, "MLST %s", file);
		if (
			(result == 2) &&
			(rp->msg.first->line != NULL) &&
			(rp->msg.first->next != NULL) &&
			(rp->msg.first->next->line != NULL)
		) {
			result = UnMlsT(rp->msg.first->next->line, mlip);
			if (result < 0) {
				cip->errNo = result = kErrInvalidMLSTResponse;
			}
		} else if (UNIMPLEMENTED_CMD(rp->code)) {
			cip->hasMLST = kCommandNotAvailable;
			cip->errNo = kErrMLSTNotAvailable;
			result = kErrMLSTNotAvailable;
		} else {
			cip->errNo = kErrMLSTFailed;
			result = kErrMLSTFailed;
		}
		DoneWithResponse(cip, rp);
	}

	return (result);
}	/* FTPMListOneFile */




/* We only use STAT to see if files or directories exist.
 * But since it is so rarely used in the wild, we need to
 * make sure the server supports the use where we pass
 * a pathname as a parameter.
 */
int
FTPFileExistsStat(const FTPCIPtr cip, const char *const file)
{
	int result;
	ResponsePtr rp;
	LineList fileList;
	char savedCwd[512];

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (file == NULL)
		return (kErrBadParameter);

	if (cip->STATfileParamWorks == kCommandNotAvailable) {
		cip->errNo = result = kErrSTATwithFileNotAvailable;
		return (result);
	}

	if (cip->STATfileParamWorks == kCommandAvailabilityUnknown) {
		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
			return (result);

		}

		/* First, make sure that when we STAT a pathname
		 * that does not exist, that we get an error back.
		 *
		 * We also assume that a valid STAT response has
		 * at least 3 lines of response text, typically
		 * a "start" line, intermediate data, and then
		 * a trailing line.
		 *
		 * We also can see a one-line case.
		 */
		result = RCmd(cip, rp, "STAT %s", "NoSuchFile");
		if ((result == 2) && ((rp->msg.nLines >= 3) || (rp->msg.nLines == 1))) {
			/* Hmmm.... it gave back a positive
			 * response.  So STAT <file> does not
			 * work correctly.
			 */
			if (
				(rp->msg.first->next != NULL) &&
				(rp->msg.first->next->line != NULL) &&
				(
					(strstr(rp->msg.first->next->line, "o such file") != NULL) ||
					(strstr(rp->msg.first->next->line, "ot found") != NULL)
				)
			) {
				/* OK, while we didn't want a 200
				 * level response, some servers,
				 * like wu-ftpd print an error
				 * message "No such file or
				 * directory" which we can special
				 * case.
				 */
				result = kNoErr;
			} else {
				cip->STATfileParamWorks = kCommandNotAvailable;
				cip->errNo = result = kErrSTATwithFileNotAvailable;
				DoneWithResponse(cip, rp);
				return (result);
			}
		}
		DoneWithResponse(cip, rp);

		/* We can't assume that we can simply say STAT rootdir/firstfile,
		 * since the remote host may not be using / as a directory
		 * delimiter.  So we have to change to the root directory
		 * and then do the STAT on that file.
		 */
		if (
			(FTPGetCWD(cip, savedCwd, sizeof(savedCwd)) != kNoErr) ||
			(FTPChdir(cip, cip->startingWorkingDirectory) != kNoErr)
		) {
			return (cip->errNo);
		}

		/* OK, we get an error when we stat
		 * a non-existant file, but now we need to
		 * see if we get a positive reply when
		 * we stat a file that does exist.
		 *
		 * To do this, we list the root directory,
		 * which we assume has one or more items.
		 * If it doesn't, the user can't do anything
		 * anyway.  Then we stat the first item
		 * we found to see if STAT says it exists.
		 */
		if (
			((result = FTPListToMemory2(cip, "", &fileList, "", 0, (int *) 0)) < 0) ||
			(fileList.last == NULL) ||
			(fileList.last->line == NULL)
		) {
			/* Hmmm... well, in any case we can't use STAT. */
			cip->STATfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrSTATwithFileNotAvailable;
			DisposeLineListContents(&fileList);
			(void) FTPChdir(cip, savedCwd);
			return (result);
		}

		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
			DisposeLineListContents(&fileList);
			(void) FTPChdir(cip, savedCwd);
			return (result);

		}

		result = RCmd(cip, rp, "STAT %s", fileList.last->line);
		DisposeLineListContents(&fileList);

		if ((result != 2) || (rp->msg.nLines == 2)) {
			/* Hmmm.... it gave back a negative
			 * response.  So STAT <file> does not
			 * work correctly.
			 */
			cip->STATfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrSTATwithFileNotAvailable;
			DoneWithResponse(cip, rp);
			(void) FTPChdir(cip, savedCwd);
			return (result);
		} else if (
				(rp->msg.first->next != NULL) &&
				(rp->msg.first->next->line != NULL) &&
				(
					(strstr(rp->msg.first->next->line, "o such file") != NULL) ||
					(strstr(rp->msg.first->next->line, "ot found") != NULL)
				)
		) {
			/* Same special-case of the second line of STAT response. */
			cip->STATfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrSTATwithFileNotAvailable;
			DoneWithResponse(cip, rp);
			(void) FTPChdir(cip, savedCwd);
			return (result);
		}
		DoneWithResponse(cip, rp);
		cip->STATfileParamWorks = kCommandAvailable;

		/* Don't forget to change back to the original directory. */
		(void) FTPChdir(cip, savedCwd);
	}

	rp = InitResponse();
	if (rp == NULL) {
		result = kErrMallocFailed;
		cip->errNo = kErrMallocFailed;
		Error(cip, kDontPerror, "Malloc failed.\n");
		return (result);
	}

	result = RCmd(cip, rp, "STAT %s", file);
	if (result == 2) {
		result = kNoErr;
		if (((rp->msg.nLines >= 3) || (rp->msg.nLines == 1))) {
			if (
				(rp->msg.first->next != NULL) &&
				(rp->msg.first->next->line != NULL) &&
				(
					(strstr(rp->msg.first->next->line, "o such file") != NULL) ||
					(strstr(rp->msg.first->next->line, "ot found") != NULL)
				)
			) {
				cip->errNo = kErrSTATFailed;
				result = kErrSTATFailed;
			} else {
				result = kNoErr;
			}
		} else if (rp->msg.nLines == 2) {
			cip->errNo = kErrSTATFailed;
			result = kErrSTATFailed;
		} else {
			result = kNoErr;
		}
	} else {
		cip->errNo = kErrSTATFailed;
		result = kErrSTATFailed;
	}
	DoneWithResponse(cip, rp);
	return (result);
}	/* FTPFileExistsStat */




/* We only use STAT to see if files or directories exist.
 * But since it is so rarely used in the wild, we need to
 * make sure the server supports the use where we pass
 * a pathname as a parameter.
 */
int
FTPFileExistsNlst(const FTPCIPtr cip, const char *const file)
{
	int result;
	LineList fileList, rootFileList;
	char savedCwd[512];

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (file == NULL)
		return (kErrBadParameter);

	if (cip->NLSTfileParamWorks == kCommandNotAvailable) {
		cip->errNo = result = kErrNLSTwithFileNotAvailable;
		return (result);
	}

	if (cip->NLSTfileParamWorks == kCommandAvailabilityUnknown) {
		/* First, make sure that when we NLST a pathname
		 * that does not exist, that we get an error back.
		 *
		 * We also assume that a valid NLST response has
		 * at least 3 lines of response text, typically
		 * a "start" line, intermediate data, and then
		 * a trailing line.
		 *
		 * We also can see a one-line case.
		 */
		if (
			((FTPListToMemory2(cip, "NoSuchFile", &fileList, "", 0, (int *) 0)) == kNoErr) &&
			(fileList.nLines >= 1) &&
			(strstr(fileList.last->line, "o such file") == NULL) &&
			(strstr(fileList.last->line, "ot found") == NULL) &&
			(strstr(fileList.last->line, "o Such File") == NULL) &&
			(strstr(fileList.last->line, "ot Found") == NULL)

		) {
			cip->NLSTfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrNLSTwithFileNotAvailable;
			DisposeLineListContents(&fileList);
			return (result);
		}
		DisposeLineListContents(&fileList);

		/* We can't assume that we can simply say NLST rootdir/firstfile,
		 * since the remote host may not be using / as a directory
		 * delimiter.  So we have to change to the root directory
		 * and then do the NLST on that file.
		 */
		if (
			(FTPGetCWD(cip, savedCwd, sizeof(savedCwd)) != kNoErr) ||
			(FTPChdir(cip, cip->startingWorkingDirectory) != kNoErr)
		) {
			return (cip->errNo);
		}

		/* OK, we get an error when we list
		 * a non-existant file, but now we need to
		 * see if we get a positive reply when
		 * we stat a file that does exist.
		 *
		 * To do this, we list the root directory,
		 * which we assume has one or more items.
		 * If it doesn't, the user can't do anything
		 * anyway.  Then we do the first item
		 * we found to see if NLST says it exists.
		 */
		if (
			((result = FTPListToMemory2(cip, "", &rootFileList, "", 0, (int *) 0)) < 0) ||
			(rootFileList.last == NULL) ||
			(rootFileList.last->line == NULL)
		) {
			/* Hmmm... well, in any case we can't use NLST. */
			cip->NLSTfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrNLSTwithFileNotAvailable;
			DisposeLineListContents(&rootFileList);
			(void) FTPChdir(cip, savedCwd);
			return (result);
		}

		if (
			((FTPListToMemory2(cip, rootFileList.last->line, &fileList, "", 0, (int *) 0)) == kNoErr) &&
			(fileList.nLines >= 1) &&
			(strstr(fileList.last->line, "o such file") == NULL) &&
			(strstr(fileList.last->line, "ot found") == NULL) &&
			(strstr(fileList.last->line, "o Such File") == NULL) &&
			(strstr(fileList.last->line, "ot Found") == NULL)

		) {
			/* Good.  We listed the item. */
			DisposeLineListContents(&fileList);
			DisposeLineListContents(&rootFileList);
			cip->NLSTfileParamWorks = kCommandAvailable;

			/* Don't forget to change back to the original directory. */
			(void) FTPChdir(cip, savedCwd);
		} else {
			cip->NLSTfileParamWorks = kCommandNotAvailable;
			cip->errNo = result = kErrNLSTwithFileNotAvailable;
			DisposeLineListContents(&fileList);
			DisposeLineListContents(&rootFileList);
			(void) FTPChdir(cip, savedCwd);
			return (result);
		}
	}

	/* Check the requested item. */
	InitLineList(&fileList);
	if (
		((FTPListToMemory2(cip, file, &fileList, "", 0, (int *) 0)) == kNoErr) &&
		(fileList.nLines >= 1) &&
		(strstr(fileList.last->line, "o such file") == NULL) &&
		(strstr(fileList.last->line, "ot found") == NULL) &&
		(strstr(fileList.last->line, "o Such File") == NULL) &&
		(strstr(fileList.last->line, "ot Found") == NULL)

	) {
		/* The item existed. */
		result = kNoErr;
	} else {
		cip->errNo = kErrNLSTFailed;
		result = kErrNLSTFailed;
	}

	DisposeLineListContents(&fileList);
	return (result);
}	/* FTPFileExistsNlst*/




/* This functions goes to a great deal of trouble to try and determine if the
 * remote file specified exists.  Newer servers support commands that make
 * it relatively inexpensive to find the answer, but older servers do not
 * provide a standard way.  This means we may try a whole bunch of things,
 * but the good news is that the library saves information about which things
 * worked so if you do this again it uses the methods that work.
 */
int
FTPFileExists2(const FTPCIPtr cip, const char *const file, const int tryMDTM, const int trySIZE, const int tryMLST, const int trySTAT, const int tryNLST)
{
	int result;
	time_t mdtm;
	longest_int size;
	MLstItem mlsInfo;

	if (tryMDTM != 0) {
		result = FTPFileModificationTime(cip, file, &mdtm);
		if (result == kNoErr)
			return (kNoErr);
		if (result == kErrMDTMFailed) {
			cip->errNo = kErrNoSuchFileOrDirectory;
			return (kErrNoSuchFileOrDirectory);
		}
		/* else keep going */	
	}

	if (trySIZE != 0) {
		result = FTPFileSize(cip, file, &size, kTypeBinary);
		if (result == kNoErr)
			return (kNoErr);
		/* SIZE could fail if the server does
		 * not support it for directories.
		 *
		 * if (result == kErrSIZEFailed)
		 *	return (kErrNoSuchFileOrDirectory);
		 */
		/* else keep going */	
	}


	if (tryMLST != 0) {
		result = FTPMListOneFile(cip, file, &mlsInfo);
		if (result == kNoErr)
			return (kNoErr);
		if (result == kErrMLSTFailed) {
			cip->errNo = kErrNoSuchFileOrDirectory;
			return (kErrNoSuchFileOrDirectory);
		}
		/* else keep going */	
	}

	if (trySTAT != 0) {
		result = FTPFileExistsStat(cip, file);
		if (result == kNoErr)
			return (kNoErr);
		if (result == kErrSTATFailed) {
			cip->errNo = kErrNoSuchFileOrDirectory;
			return (kErrNoSuchFileOrDirectory);
		}
		/* else keep going */	
	}

	if (tryNLST != 0) {
		result = FTPFileExistsNlst(cip, file);
		if (result == kNoErr)
			return (kNoErr);
		if (result == kErrNLSTFailed) {
			cip->errNo = kErrNoSuchFileOrDirectory;
			return (kErrNoSuchFileOrDirectory);
		}
		/* else keep going */	
	}

	cip->errNo = kErrCantTellIfFileExists;
	return (kErrCantTellIfFileExists);
}	/* FTPFileExists2 */




int
FTPFileExists(const FTPCIPtr cip, const char *const file)
{
	return (FTPFileExists2(cip, file, 1, 1, 1, 1, 1));
}	/* FTPFileExists */





int
FTPFileSizeAndModificationTime(const FTPCIPtr cip, const char *const file, longest_int *const size, const int type, time_t *const mdtm)
{
	MLstItem mlsInfo;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((mdtm == NULL) || (size == NULL) || (file == NULL))
		return (kErrBadParameter);

	*mdtm = kModTimeUnknown;
	*size = kSizeUnknown;

	result = FTPSetTransferType(cip, type);
	if (result < 0)
		return (result);

	result = FTPMListOneFile(cip, file, &mlsInfo);
	if (result < 0) {
		/* Do it the regular way, where
		 * we do a SIZE and then a MDTM.
		 */
		result = FTPFileSize(cip, file, size, type);
		if (result < 0)
			return (result);
		result = FTPFileModificationTime(cip, file, mdtm);
		return (result);
	} else {
		*mdtm = mlsInfo.ftime;
		*size = mlsInfo.fsize;
	}

	return (result);
}	/* FTPFileSizeAndModificationTime */




int
FTPFileType(const FTPCIPtr cip, const char *const file, int *const ftype)
{
	int result;
	MLstItem mlsInfo;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((file == NULL) || (file[0] == '\0')) {
		cip->errNo = kErrBadParameter;
		return (kErrBadParameter);
	}

	if (ftype == NULL) {
		cip->errNo = kErrBadParameter;
		return (kErrBadParameter);
	}

	*ftype = 0;
	result = FTPMListOneFile(cip, file, &mlsInfo);
	if (result == kNoErr) {
		*ftype = mlsInfo.ftype;
		return (kNoErr);
	}

	/* Preserve old working directory. */
	(void) FTPGetCWD(cip, cip->buf, cip->bufSize);

	result = FTPChdir(cip, file);
	if (result == kNoErr) {
		*ftype = 'd';
		/* Yes it was a directory, now go back to
		 * where we were.
		 */
		(void) FTPChdir(cip, cip->buf);

		/* Note:  This improperly assumes that we
		 * will be able to chdir back, which is
		 * not guaranteed.
		 */
		return (kNoErr);
	}

	result = FTPFileExists2(cip, file, 1, 1, 0, 1, 1);
	if (result != kErrNoSuchFileOrDirectory)
		result = kErrFileExistsButCannotDetermineType;

	return (result);
}	/* FTPFileType */




int
FTPIsDir(const FTPCIPtr cip, const char *const dir)
{
	int result, ftype;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((dir == NULL) || (dir[0] == '\0')) {
		cip->errNo = kErrInvalidDirParam;
		return (kErrInvalidDirParam);
	}

	result = FTPFileType(cip, dir, &ftype);
	if ((result == kNoErr) || (result == kErrFileExistsButCannotDetermineType)) {
		result = 0;
		if (ftype == 'd')
			result = 1;
	}
	return (result);
}	/* FTPIsDir */




int
FTPIsRegularFile(const FTPCIPtr cip, const char *const file)
{
	int result, ftype;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((file == NULL) || (file[0] == '\0')) {
		cip->errNo = kErrBadParameter;
		return (kErrBadParameter);
	}

	result = FTPFileType(cip, file, &ftype);
	if ((result == kNoErr) || (result == kErrFileExistsButCannotDetermineType)) {
		result = 1;
		if (ftype == 'd')
			result = 0;
	}
	return (result);
}	/* FTPIsRegularFile */




int
FTPSymlink(const FTPCIPtr cip, const char *const lfrom, const char *const lto)
{
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	if ((cip == NULL) || (lfrom == NULL) || (lto == NULL))
		return (kErrBadParameter);
	if ((lfrom[0] == '\0') || (lto[0] == '\0'))
		return (kErrBadParameter);
	if (FTPCmd(cip, "SITE SYMLINK %s %s", lfrom, lto) == 2)
		return (kNoErr);
	return (kErrSYMLINKFailed);
}	/* FTPSymlink */




int
FTPUmask(const FTPCIPtr cip, const char *const umsk)
{
	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	if ((umsk == NULL) || (umsk[0] == '\0'))
		return (kErrBadParameter);
	if (FTPCmd(cip, "SITE UMASK %s", umsk) == 2)
		return (kNoErr);
	return (kErrUmaskFailed);
}	/* FTPUmask */




static void
GmTimeStr(char *const dst, const size_t dstsize, time_t t)
{
	char buf[64];
	struct tm *gtp;

	gtp = gmtime(&t);
	if (gtp == NULL) {
		dst[0] = '\0';
	} else {
#ifdef HAVE_SNPRINTF
		buf[sizeof(buf) - 1] = '\0';
		(void) snprintf(buf, sizeof(buf) - 1, "%04d%02d%02d%02d%02d%02d",
#else
		(void) sprintf(buf, "%04d%02d%02d%02d%02d%02d",
#endif
			gtp->tm_year + 1900,
			gtp->tm_mon + 1,
			gtp->tm_mday,
			gtp->tm_hour,
			gtp->tm_min,
			gtp->tm_sec
		);
		(void) Strncpy(dst, buf, dstsize);
	}
}	/* GmTimeStr */




int
FTPUtime(const FTPCIPtr cip, const char *const file, time_t actime, time_t modtime, time_t crtime)
{
	char mstr[64], astr[64], cstr[64];
	int result;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (cip->hasUTIME == kCommandNotAvailable) {
		cip->errNo = kErrUTIMENotAvailable;
		result = kErrUTIMENotAvailable;
	} else {
		if ((actime == (time_t) 0) || (actime == (time_t) -1))
			(void) time(&actime);
		if ((modtime == (time_t) 0) || (modtime == (time_t) -1))
			(void) time(&modtime);
		if ((crtime == (time_t) 0) || (crtime == (time_t) -1))
			crtime = modtime;

		(void) GmTimeStr(astr, sizeof(astr), actime);
		(void) GmTimeStr(mstr, sizeof(mstr), modtime);
		(void) GmTimeStr(cstr, sizeof(cstr), crtime);

		rp = InitResponse();
		if (rp == NULL) {
			result = kErrMallocFailed;
			cip->errNo = kErrMallocFailed;
			Error(cip, kDontPerror, "Malloc failed.\n");
		} else {
			result = RCmd(cip, rp, "SITE UTIME %s %s %s %s UTC", file, astr, mstr, cstr); 	
			if (result < 0) {
				DoneWithResponse(cip, rp);
				return (result);
			} else if (result == 2) {
				cip->hasUTIME = kCommandAvailable;
				result = kNoErr;
			} else if (UNIMPLEMENTED_CMD(rp->code)) {
				cip->hasUTIME = kCommandNotAvailable;
				cip->errNo = kErrUTIMENotAvailable;
				result = kErrUTIMENotAvailable;
			} else {
				cip->errNo = kErrUTIMEFailed;
				result = kErrUTIMEFailed;
			}
			DoneWithResponse(cip, rp);
		}
	}
	return (result);
}	/* FTPUtime */
