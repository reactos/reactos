/* io.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

static int gGotBrokenData = 0;

#if defined(WIN32) || defined(_WINDOWS)
#	define ASCII_TRANSLATION 0
#endif

#ifndef ASCII_TRANSLATION
#	define ASCII_TRANSLATION 1
#endif

#if !defined(NO_SIGNALS) && (USE_SIO || !defined(SIGALRM) || !defined(SIGPIPE) || !defined(SIGINT))
#	define NO_SIGNALS 1
#endif

#ifndef NO_SIGNALS

#ifdef HAVE_SIGSETJMP
static sigjmp_buf gBrokenDataJmp;
#else
static jmp_buf gBrokenDataJmp;
#endif	/* HAVE_SIGSETJMP */
static int gCanBrokenDataJmp = 0;

#endif	/* NO_SIGNALS */


#ifndef O_BINARY
	/* Needed for platforms using different EOLN sequence (i.e. DOS) */
#	ifdef _O_BINARY
#		define O_BINARY _O_BINARY
#	else
#		define O_BINARY 0
#	endif
#endif

static int WaitForRemoteInput(const FTPCIPtr cip);
static int WaitForRemoteOutput(const FTPCIPtr cip);


#ifndef NO_SIGNALS

static void
BrokenData(int signum)
{
	gGotBrokenData = signum;
	if (gCanBrokenDataJmp != 0) {
		gCanBrokenDataJmp = 0;
#ifdef HAVE_SIGSETJMP
		siglongjmp(gBrokenDataJmp, 1);
#else
		longjmp(gBrokenDataJmp, 1);
#endif	/* HAVE_SIGSETJMP */
	}
}	/* BrokenData */

#endif	/* NO_SIGNALS */




void
FTPInitIOTimer(const FTPCIPtr cip)
{
	cip->bytesTransferred = (longest_int) 0;
	cip->expectedSize = kSizeUnknown;
	cip->mdtm = kModTimeUnknown;
	cip->rname = NULL;
	cip->lname = NULL;
	cip->kBytesPerSec = -1.0;
	cip->percentCompleted = -1.0;
	cip->sec = -1.0;
	cip->secLeft = -1.0;
	cip->nextProgressUpdate = 0;
	cip->stalled = 0;
	cip->dataTimedOut = 0;
	cip->useProgressMeter = 1;
	(void) gettimeofday(&cip->t0, NULL);
}	/* FTPInitIOTimer */




void
FTPStartIOTimer(const FTPCIPtr cip)
{
	(void) gettimeofday(&cip->t0, NULL);
	if (cip->progress != (FTPProgressMeterProc) 0)
		(*cip->progress)(cip, kPrInitMsg);
}	/* FTPStartIOTimer */




void
FTPUpdateIOTimer(const FTPCIPtr cip)
{
	double sec;
	struct timeval *t0, t1;
	time_t now;

	(void) time(&now);
	if (now < cip->nextProgressUpdate)
		return;
	now += 1;
	cip->nextProgressUpdate = now;

	(void) gettimeofday(&t1, NULL);
	t0 = &cip->t0;

	if (t0->tv_usec > t1.tv_usec) {
		t1.tv_usec += 1000000;
		t1.tv_sec--;
	}
	sec = ((double) (t1.tv_usec - t0->tv_usec) * 0.000001)
		+ (t1.tv_sec - t0->tv_sec);
	if (sec > 0.0) {
		cip->kBytesPerSec = ((double) cip->bytesTransferred) / (1024.0 * sec);
	} else {
		cip->kBytesPerSec = -1.0;
	}
	if (cip->expectedSize == kSizeUnknown) {
		cip->percentCompleted = -1.0;
		cip->secLeft = -1.0;
	} else if (cip->expectedSize <= 0) {
		cip->percentCompleted = 100.0;
		cip->secLeft = 0.0;
	} else {
		cip->percentCompleted = ((double) (100.0 * (cip->bytesTransferred + cip->startPoint))) / ((double) cip->expectedSize);
		if (cip->percentCompleted >= 100.0) {
			cip->percentCompleted = 100.0;
			cip->secLeft = 0.0;
		} else if (cip->percentCompleted <= 0.0) {
			cip->secLeft = 999.0;
		}
		if (cip->kBytesPerSec > 0.0) {
			cip->secLeft = ((cip->expectedSize - cip->bytesTransferred - cip->startPoint) / 1024.0) / cip->kBytesPerSec;
			if (cip->secLeft < 0.0)
				cip->secLeft = 0.0;
		}
	}
	cip->sec = sec;
	if ((cip->progress != (FTPProgressMeterProc) 0) && (cip->useProgressMeter != 0))
		(*cip->progress)(cip, kPrUpdateMsg);
}	/* FTPUpdateIOTimer */




void
FTPStopIOTimer(const FTPCIPtr cip)
{
	cip->nextProgressUpdate = 0;	/* force last update */
	FTPUpdateIOTimer(cip);
	if (cip->progress != (FTPProgressMeterProc) 0)
		(*cip->progress)(cip, kPrEndMsg);
}	/* FTPStopIOTimer */




/* This isn't too useful -- it mostly serves as an example so you can write
 * your own function to do what you need to do with the listing.
 */
int
FTPList(const FTPCIPtr cip, const int outfd, const int longMode, const char *const lsflag)
{
	const char *cmd;
	char line[512];
	char secondaryBuf[768];
#ifndef NO_SIGNALS
	char *secBufPtr, *secBufLimit;
	int nread;
	volatile int result;
#else	/* NO_SIGNALS */
	SReadlineInfo lsSrl;
	int result;
#endif	/* NO_SIGNALS */

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	cmd = (longMode != 0) ? "LIST" : "NLST";
	if ((lsflag == NULL) || (lsflag[0] == '\0')) {
		result = FTPStartDataCmd(cip, kNetReading, kTypeAscii, (longest_int) 0, "%s", cmd);
	} else {
		result = FTPStartDataCmd(cip, kNetReading, kTypeAscii, (longest_int) 0, "%s %s", cmd, lsflag);
	}


#ifdef NO_SIGNALS

	if (result == 0) {
		if (InitSReadlineInfo(&lsSrl, cip->dataSocket, secondaryBuf, sizeof(secondaryBuf), (int) cip->xferTimeout, 1) < 0) {
			/* Not really fdopen, but close in what we're trying to do. */
			result = kErrFdopenR;
			cip->errNo = kErrFdopenR;
			Error(cip, kDoPerror, "Could not fdopen.\n");
			return (result);
		}
		
		for (;;) {
			result = SReadline(&lsSrl, line, sizeof(line) - 2);
			if (result == kTimeoutErr) {
				/* timeout */
				Error(cip, kDontPerror, "Could not directory listing data -- timed out.\n");
				cip->errNo = kErrDataTimedOut;
				return (cip->errNo);
			} else if (result == 0) {
				/* end of listing -- done */
				cip->numListings++;
				break;
			} else if (result < 0) {
				/* error */
				Error(cip, kDoPerror, "Could not read directory listing data");
				result = kErrLISTFailed;
				cip->errNo = kErrLISTFailed;
				break;
			}

			(void) write(outfd, line, strlen(line));
		}

		DisposeSReadlineInfo(&lsSrl);
		if (FTPEndDataCmd(cip, 1) < 0) {
			result = kErrLISTFailed;
			cip->errNo = kErrLISTFailed;
		}
	} else if (result == kErrGeneric) {
		result = kErrLISTFailed;
		cip->errNo = kErrLISTFailed;
	}


#else	/* NO_SIGNALS */
	
	if (result == 0) {
		/* This line sets the buffer pointer so that the first thing
		 * BufferGets will do is reset and fill the buffer using
		 * real I/O.
		 */
		secBufPtr = secondaryBuf + sizeof(secondaryBuf);
		secBufLimit = (char *) 0;

		for (;;) {
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
			nread = BufferGets(line, sizeof(line), cip->dataSocket, secondaryBuf, &secBufPtr, &secBufLimit, sizeof(secondaryBuf));
			if (nread <= 0) {
				if (nread < 0)
					break;
			} else {
				cip->bytesTransferred += (longest_int) nread;
				(void) STRNCAT(line, "\n");
				(void) write(outfd, line, strlen(line));
			}
		}
		if (cip->xferTimeout > 0)
			(void) alarm(0);
		result = FTPEndDataCmd(cip, 1);
		if (result < 0) {
			result = kErrLISTFailed;
			cip->errNo = kErrLISTFailed;
		}
		result = kNoErr;
		cip->numListings++;
	} else if (result == kErrGeneric) {
		result = kErrLISTFailed;
		cip->errNo = kErrLISTFailed;
	}
#endif	/* NO_SIGNALS */
	return (result);
}	/* FTPList */




static void
FTPRequestMlsOptions(const FTPCIPtr cip)
{
	int f;
	char optstr[128];
	size_t optstrlen;

	if (cip->usedMLS == 0) {
		/* First MLSD/MLST ? */
		cip->usedMLS = 1;

		f = cip->mlsFeatures & kPreferredMlsOpts;
		optstr[0] = '\0';

		/* TYPE */
		if ((f & kMlsOptType) != 0) {
			STRNCAT(optstr, "type;");
		}

		/* SIZE */
		if ((f & kMlsOptSize) != 0) {
			STRNCAT(optstr, "size;");
		}

		/* MODTIME */
		if ((f & kMlsOptModify) != 0) {
			STRNCAT(optstr, "modify;");
		}

		/* MODE */
		if ((f & kMlsOptUNIXmode) != 0) {
			STRNCAT(optstr, "UNIX.mode;");
		}

		/* PERM */
		if ((f & kMlsOptPerm) != 0) {
			STRNCAT(optstr, "perm;");
		}

		/* OWNER */
		if ((f & kMlsOptUNIXowner) != 0) {
			STRNCAT(optstr, "UNIX.owner;");
		}

		/* UID */
		if ((f & kMlsOptUNIXuid) != 0) {
			STRNCAT(optstr, "UNIX.uid;");
		}

		/* GROUP */
		if ((f & kMlsOptUNIXgroup) != 0) {
			STRNCAT(optstr, "UNIX.group;");
		}

		/* GID */
		if ((f & kMlsOptUNIXgid) != 0) {
			STRNCAT(optstr, "UNIX.gid;");
		}

		/* UNIQUE */
		if ((f & kMlsOptUnique) != 0) {
			STRNCAT(optstr, "unique;");
		}

		/* Tell the server what we prefer. */
		optstrlen = strlen(optstr);
		if (optstrlen > 0) {
			if (optstr[optstrlen - 1] == ';')
				optstr[optstrlen - 1] = '\0';
			(void) FTPCmd(cip, "OPTS MLST %s", optstr);
		}
	}
}	/* FTPRequestMlsOptions */




int
FTPListToMemory2(const FTPCIPtr cip, const char *const pattern, const LineListPtr llines, const char *const lsflags, const int blankLines, int *const tryMLSD)
{
	char secondaryBuf[768];
	char line[512];
	char lsflags1[128];
	const char *command = "NLST";
	const char *scp;
	char *dcp, *lim;
#ifndef NO_SIGNALS
	char *secBufPtr, *secBufLimit;
	volatile FTPSigProc osigpipe;
	volatile FTPCIPtr vcip;
	int sj;
	int nread;
	volatile int result;
#else	/* NO_SIGNALS */
	SReadlineInfo lsSrl;
	int result;
#endif	/* NO_SIGNALS */

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if ((llines == NULL) || (pattern == NULL) || (lsflags == NULL))
		return (kErrBadParameter);

	if ((tryMLSD != (int *) 0) && (*tryMLSD != 0) && (cip->hasMLSD == kCommandAvailable)) {
		command = "MLSD";
		if ((lsflags[0] == '-') && (strchr(lsflags, 'd') != NULL) && (cip->hasMLST == kCommandAvailable))
			command = "MLST";
		lsflags1[0] = '\0';
		FTPRequestMlsOptions(cip);
	} else {
		/* Not using MLSD. */
		if (tryMLSD != (int *) 0)
			*tryMLSD = 0;
		if (lsflags[0] == '-') {
			/* See if we should use LIST instead. */
			scp = lsflags + 1;
			dcp = lsflags1;
			lim = lsflags1 + sizeof(lsflags1) - 2;
			for (; *scp != '\0'; scp++) {
				if (*scp == 'l') {
					/* do not add the 'l' */
					command = "LIST";
				} else if (dcp < lim) {
					if (dcp == lsflags1)
						*dcp++ = '-';
					*dcp++ = *scp;
				}
			}
			*dcp = '\0';
		} else {
			(void) STRNCPY(lsflags1, lsflags);
		}
	}

	InitLineList(llines);

	result = FTPStartDataCmd(
		cip,
		kNetReading,
		kTypeAscii,
		(longest_int) 0,
		"%s%s%s%s%s",
		command,
		(lsflags1[0] == '\0') ? "" : " ",
		lsflags1,
		(pattern[0] == '\0') ? "" : " ",
		pattern
	);

#ifdef NO_SIGNALS

	if (result == 0) {
		if (InitSReadlineInfo(&lsSrl, cip->dataSocket, secondaryBuf, sizeof(secondaryBuf), (int) cip->xferTimeout, 1) < 0) {
			/* Not really fdopen, but close in what we're trying to do. */
			result = kErrFdopenR;
			cip->errNo = kErrFdopenR;
			Error(cip, kDoPerror, "Could not fdopen.\n");
			return (result);
		}
		
		for (;;) {
			result = SReadline(&lsSrl, line, sizeof(line) - 1);
			if (result == kTimeoutErr) {
				/* timeout */
				Error(cip, kDontPerror, "Could not directory listing data -- timed out.\n");
				cip->errNo = kErrDataTimedOut;
				return (cip->errNo);
			} else if (result == 0) {
				/* end of listing -- done */
				cip->numListings++;
				break;
			} else if (result < 0) {
				/* error */
				Error(cip, kDoPerror, "Could not read directory listing data");
				result = kErrLISTFailed;
				cip->errNo = kErrLISTFailed;
				break;
			}

			if (line[result - 1] == '\n')
				line[result - 1] = '\0';

			if ((blankLines == 0) && (result <= 1))
				continue;

			/* Valid directory listing line of output */
			if ((line[0] == '.') && ((line[1] == '\0') || ((line[1] == '.') && ((line[2] == '\0') || (iscntrl(line[2]))))))
				continue;	/* Skip . and .. */

			(void) AddLine(llines, line);
		}

		DisposeSReadlineInfo(&lsSrl);
		if (FTPEndDataCmd(cip, 1) < 0) {
			result = kErrLISTFailed;
			cip->errNo = kErrLISTFailed;
		}
	} else if (result == kErrGeneric) {
		result = kErrLISTFailed;
		cip->errNo = kErrLISTFailed;
	}


#else	/* NO_SIGNALS */
	vcip = cip;
	osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenData);

	gGotBrokenData = 0;
	gCanBrokenDataJmp = 0;

#ifdef HAVE_SIGSETJMP
	sj = sigsetjmp(gBrokenDataJmp, 1);
#else
	sj = setjmp(gBrokenDataJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
		FTPShutdownHost(vcip);
		vcip->errNo = kErrRemoteHostClosedConnection;
		return(vcip->errNo);
	}
	gCanBrokenDataJmp = 1;

	if (result == 0) {
		/* This line sets the buffer pointer so that the first thing
		 * BufferGets will do is reset and fill the buffer using
		 * real I/O.
		 */
		secBufPtr = secondaryBuf + sizeof(secondaryBuf);
		secBufLimit = (char *) 0;
		memset(secondaryBuf, 0, sizeof(secondaryBuf));

		for (;;) {
			memset(line, 0, sizeof(line));
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
			nread = BufferGets(line, sizeof(line), cip->dataSocket, secondaryBuf, &secBufPtr, &secBufLimit, sizeof(secondaryBuf));
			if (nread <= 0) {
				if (nread < 0)
					break;
				if (blankLines != 0)
					(void) AddLine(llines, line);
			} else {
				cip->bytesTransferred += (longest_int) nread;

				if ((line[0] == '.') && ((line[1] == '\0') || ((line[1] == '.') && ((line[2] == '\0') || (iscntrl(line[2]))))))
					continue;	/* Skip . and .. */

				(void) AddLine(llines, line);
			}
		}
		if (cip->xferTimeout > 0)
			(void) alarm(0);
		result = FTPEndDataCmd(cip, 1);
		if (result < 0) {
			result = kErrLISTFailed;
			cip->errNo = kErrLISTFailed;
		}
		result = kNoErr;
		cip->numListings++;
	} else if (result == kErrGeneric) {
		result = kErrLISTFailed;
		cip->errNo = kErrLISTFailed;
	}
	(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
	return (result);
}	/* FTPListToMemory2 */




static void
AutomaticallyUseASCIIModeDependingOnExtension(const FTPCIPtr cip, const char *const pathName, int *const xtype)
{
	if ((*xtype == kTypeBinary) && (cip->asciiFilenameExtensions != NULL)) {
		if (FilenameExtensionIndicatesASCII(pathName, cip->asciiFilenameExtensions)) {
			/* Matched -- send this file in ASCII mode
			 * instead of binary since it's extension
			 * appears to be that of a text file.
			 */
			*xtype = kTypeAscii;
		}
	}
}	/* AutomaticallyUseASCIIModeDependingOnExtension */




/* The purpose of this is to provide updates for the progress meters
 * during lags.  Return zero if the operation timed-out.
 */
static int
WaitForRemoteOutput(const FTPCIPtr cip)
{
	fd_set ss, ss2;
	struct timeval tv;
	int result;
	int fd;
	int wsecs;
	int xferTimeout;
	int ocancelXfer;

	xferTimeout = cip->xferTimeout;
	if (xferTimeout < 1)
		return (1);

	fd = cip->dataSocket;
	if (fd < 0)
		return (1);

	ocancelXfer = cip->cancelXfer;
	wsecs = 0;
	cip->stalled = 0;

	while ((xferTimeout <= 0) || (wsecs < xferTimeout)) {
		if ((cip->cancelXfer != 0) && (ocancelXfer == 0)) {
			/* leave cip->stalled -- could have been stalled and then canceled. */
			return (1);
		}
		FD_ZERO(&ss);
		FD_SET(fd, &ss);
		ss2 = ss;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		result = select(fd + 1, NULL, SELECT_TYPE_ARG234 &ss, SELECT_TYPE_ARG234 &ss2, &tv);
		if (result == 1) {
			/* ready */
			cip->stalled = 0;
			return (1);
		} else if (result < 0) {
			if (errno != EINTR) {
				perror("select");
				cip->stalled = 0;
				return (1);
			}
		} else {
			wsecs++;
			cip->stalled = wsecs;
		}
		FTPUpdateIOTimer(cip);
	}

#if !defined(NO_SIGNALS)
	/* Shouldn't get here -- alarm() should have
	 * went off by now.
	 */
	(void) kill(getpid(), SIGALRM);
#endif	/* NO_SIGNALS */

	cip->dataTimedOut = 1;
	return (0);	/* timed-out */
}	/* WaitForRemoteOutput */




static int
FTPPutOneF(
	const FTPCIPtr cip,
	const char *const file,
	const char *volatile dstfile,
	int xtype,
	const int fdtouse,
	const int appendflag,
	const char *volatile tmppfx,
	const char *volatile tmpsfx,
	const int resumeflag,
	const int deleteflag,
	const ConfirmResumeUploadProc resumeProc)
{
	char *buf, *cp;
	const char *cmd;
	const char *odstfile;
	size_t bufSize;
	size_t l;
	int tmpResult, result;
	int nread, nwrote;
	volatile int fd;
	char dstfile2[512];
#if ASCII_TRANSLATION
	char *src, *srclim, *dst;
	int ntowrite;
	char inbuf[256];
#endif
	int fstatrc, statrc;
	longest_int startPoint = 0;
	struct Stat st;
	time_t mdtm;
#if !defined(NO_SIGNALS)
	int sj;
	volatile FTPSigProc osigpipe;
	volatile FTPCIPtr vcip;
	volatile int vfd, vfdtouse;
#endif	/* NO_SIGNALS */
	volatile int vzaction;
	int zaction = kConfirmResumeProcSaidBestGuess;

	if (cip->buf == NULL) {
		Error(cip, kDoPerror, "Transfer buffer not allocated.\n");
		cip->errNo = kErrNoBuf;
		return (cip->errNo);
	}

	cip->usingTAR = 0;
	if (fdtouse < 0) {
		fd = Open(file, O_RDONLY|O_BINARY, 0);
		if (fd < 0) {
			Error(cip, kDoPerror, "Cannot open local file %s for reading.\n", file);
			cip->errNo = kErrOpenFailed;
			return (cip->errNo);
		}
	} else {
		fd = fdtouse;
	}

	fstatrc = Fstat(fd, &st);
	if ((fstatrc == 0) && (S_ISDIR(st.st_mode))) {
		if (fdtouse < 0) {
			(void) close(fd);
		}
		Error(cip, kDontPerror, "%s is a directory.\n", (file != NULL) ? file : "that");
		cip->errNo = kErrOpenFailed;
		return (cip->errNo);
	}

	/* For Put, we can't recover very well if it turns out restart
	 * didn't work, so check beforehand.
	 */
	if (cip->hasREST == kCommandAvailabilityUnknown) {
		(void) FTPSetTransferType(cip, kTypeBinary);
		if (SetStartOffset(cip, (longest_int) 1) == kNoErr) {
			/* Now revert -- we still may not end up
			 * doing it.
			 */
			SetStartOffset(cip, (longest_int) -1);
		}
	}

	if (fdtouse < 0) {
		AutomaticallyUseASCIIModeDependingOnExtension(cip, dstfile, &xtype);
		(void) FTPFileSizeAndModificationTime(cip, dstfile, &startPoint, xtype, &mdtm);

		if (appendflag == kAppendYes) {
			zaction = kConfirmResumeProcSaidAppend;
		} else if (
				(cip->hasREST == kCommandNotAvailable) ||
				(xtype != kTypeBinary) ||
				(fstatrc < 0)
		) {
			zaction = kConfirmResumeProcSaidOverwrite;
		} else if (resumeflag == kResumeYes) {
			zaction = kConfirmResumeProcSaidBestGuess;
		} else {
			zaction = kConfirmResumeProcSaidOverwrite;
		}

		statrc = -1;
		if ((mdtm != kModTimeUnknown) || (startPoint != kSizeUnknown)) {
			/* Then we know the file exists.  We will
			 * ask the user what to do, if possible, below.
			 */
			statrc = 0;
		} else if ((resumeProc != NoConfirmResumeUploadProc) && (cip->hasMDTM != kCommandAvailable) && (cip->hasSIZE != kCommandAvailable)) {
			/* We already checked if the file had a filesize
			 * or timestamp above, but if the server indicated
			 * it did not support querying those directly,
			 * we now need to try to determine if the file
			 * exists in a few other ways.
			 */
			statrc = FTPFileExists2(cip, dstfile, 0, 0, 0, 1, 1);
		}

		if (
			(resumeProc != NoConfirmResumeUploadProc) &&
			(statrc == 0)
		) {
			zaction = (*resumeProc)(file, (longest_int) st.st_size, st.st_mtime, &dstfile, startPoint, mdtm, &startPoint);
		}

		if (zaction == kConfirmResumeProcSaidCancel) {
			/* User wants to cancel this file and any
			 * remaining in batch.
			 */
			cip->errNo = kErrUserCanceled;
			return (cip->errNo);
		}

		if (zaction == kConfirmResumeProcSaidBestGuess) {
			if ((mdtm != kModTimeUnknown) && (st.st_mtime > (mdtm + 1))) {
				/* Local file is newer than remote,
				 * overwrite the remote file instead
				 * of trying to resume it.
				 *
				 * Note:  Add one second fudge factor
				 * for Windows' file timestamps being
				 * imprecise to one second.
				 */
				zaction = kConfirmResumeProcSaidOverwrite; 
			} else if ((longest_int) st.st_size == startPoint) {
				/* Already sent file, done. */
				zaction = kConfirmResumeProcSaidSkip; 
			} else if ((startPoint != kSizeUnknown) && ((longest_int) st.st_size > startPoint)) {
				zaction = kConfirmResumeProcSaidResume; 
			} else {
				zaction = kConfirmResumeProcSaidOverwrite; 
			}
		}

		if (zaction == kConfirmResumeProcSaidSkip) {
			/* Nothing done, but not an error. */
			if (fdtouse < 0) {
				(void) close(fd);
			}
			if (deleteflag == kDeleteYes) {
				if (unlink(file) < 0) {
					cip->errNo = kErrLocalDeleteFailed;
					return (cip->errNo);
				}
			}
			return (kNoErr);
		} else if (zaction == kConfirmResumeProcSaidResume) {
			/* Resume; proc set the startPoint. */
			if ((longest_int) st.st_size == startPoint) {
				/* Already sent file, done. */
				if (fdtouse < 0) {
					(void) close(fd);
				}

				if (deleteflag == kDeleteYes) {
					if (unlink(file) < 0) {
						cip->errNo = kErrLocalDeleteFailed;
						return (cip->errNo);
					}
				}
				return (kNoErr);
			} else if (Lseek(fd, (off_t) startPoint, SEEK_SET) != (off_t) -1) {
				cip->startPoint = startPoint;
			}
		} else if (zaction == kConfirmResumeProcSaidAppend) {
			/* append: leave startPoint at zero, we will append everything. */
			cip->startPoint = startPoint = 0;
		} else /* if (zaction == kConfirmResumeProcSaidOverwrite) */ {
			/* overwrite: leave startPoint at zero */
			cip->startPoint = startPoint = 0;
		}
	}

	if ((cip->numUploads == 0) && (cip->dataSocketSBufSize > 0)) {
		/* If dataSocketSBufSize is non-zero, it means you
		 * want to explicitly try to set the size of the
		 * socket's I/O buffer.
		 *
		 * If it is zero, it means you want to just use the
		 * TCP stack's default value, which is typically
		 * between 8 and 64 kB.
		 *
		 * If you try to set the buffer larger than 64 kB,
		 * the TCP stack should try to use RFC 1323 to
		 * negotiate "TCP Large Windows" which may yield
		 * significant performance gains.
		 */
		if (cip->hasSTORBUFSIZE == kCommandAvailable)
			(void) FTPCmd(cip, "SITE STORBUFSIZE %lu", (unsigned long) cip->dataSocketSBufSize);
		else if (cip->hasSBUFSIZ == kCommandAvailable)
			(void) FTPCmd(cip, "SITE SBUFSIZ %lu", (unsigned long) cip->dataSocketSBufSize);
		else if (cip->hasSBUFSZ == kCommandAvailable)
			(void) FTPCmd(cip, "SITE SBUFSZ %lu", (unsigned long) cip->dataSocketSBufSize);
		/* At least one server implemenation has RBUFSZ but not
		 * SBUFSZ and instead uses RBUFSZ for both.
		 */
		else if ((cip->hasSBUFSZ != kCommandAvailable) && (cip->hasRBUFSZ == kCommandAvailable))
			(void) FTPCmd(cip, "SITE RBUFSZ %lu", (unsigned long) cip->dataSocketSBufSize);
		else if (cip->hasBUFSIZE == kCommandAvailable)
			(void) FTPCmd(cip, "SITE BUFSIZE %lu", (unsigned long) cip->dataSocketSBufSize);
	}

#ifdef NO_SIGNALS
	vzaction = zaction;
#else	/* NO_SIGNALS */
	vcip = cip;
	vfdtouse = fdtouse;
	vfd = fd;
	vzaction = zaction;
	osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenData);

	gGotBrokenData = 0;
	gCanBrokenDataJmp = 0;

#ifdef HAVE_SIGSETJMP
	sj = sigsetjmp(gBrokenDataJmp, 1);
#else
	sj = setjmp(gBrokenDataJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
		if (vfdtouse < 0) {
			(void) close(vfd);
		}
		FTPShutdownHost(vcip);
		vcip->errNo = kErrRemoteHostClosedConnection;
		return(vcip->errNo);
	}
	gCanBrokenDataJmp = 1;
#endif	/* NO_SIGNALS */

	if (vzaction == kConfirmResumeProcSaidAppend) {
		cmd = "APPE";
		tmppfx = "";	/* Can't use that here. */
		tmpsfx = "";
	} else {
		cmd = "STOR";
		if (tmppfx == NULL)
			tmppfx = "";
		if (tmpsfx == NULL)
			tmpsfx = "";
	}

	odstfile = dstfile;
	if ((tmppfx[0] != '\0') || (tmpsfx[0] != '\0')) {
		cp = strrchr(dstfile, '/');
		if (cp == NULL)
			cp = strrchr(dstfile, '\\');
		if (cp == NULL) {
			(void) STRNCPY(dstfile2, tmppfx);
			(void) STRNCAT(dstfile2, dstfile);
			(void) STRNCAT(dstfile2, tmpsfx);
		} else {
			cp++;
			l = (size_t) (cp - dstfile);
			(void) STRNCPY(dstfile2, dstfile);
			dstfile2[l] = '\0';	/* Nuke stuff after / */
			(void) STRNCAT(dstfile2, tmppfx);
			(void) STRNCAT(dstfile2, cp);
			(void) STRNCAT(dstfile2, tmpsfx);
		}
		dstfile = dstfile2;
	}

	tmpResult = FTPStartDataCmd(
		cip,
		kNetWriting,
		xtype,
		startPoint,
		"%s %s",
		cmd,
		dstfile
	);

	if (tmpResult < 0) {
		cip->errNo = tmpResult;
		if (fdtouse < 0) {
			(void) close(fd);
		}
#if !defined(NO_SIGNALS)
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
		return (cip->errNo);
	}

	if ((startPoint != 0) && (cip->startPoint == 0)) {
		/* Remote could not or would not set the start offset
		 * to what we wanted.
		 *
		 * So now we have to undo our seek.
		 */
		if (Lseek(fd, (off_t) 0, SEEK_SET) != (off_t) 0) {
			cip->errNo = kErrLseekFailed;
			if (fdtouse < 0) {
				(void) close(fd);
			}
#if !defined(NO_SIGNALS)
			(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
			return (cip->errNo);
		}
		startPoint = 0;
	}

	result = kNoErr;
	buf = cip->buf;
	bufSize = cip->bufSize;

	FTPInitIOTimer(cip);
	if ((fstatrc == 0) && (S_ISREG(st.st_mode) != 0)) {
		cip->expectedSize = (longest_int) st.st_size;
		cip->mdtm = st.st_mtime;
	}
	cip->lname = file;	/* could be NULL */
	cip->rname = odstfile;
	if (fdtouse >= 0)
		cip->useProgressMeter = 0;
	FTPStartIOTimer(cip);

	/* Note: On Windows, we don't have to do anything special
	 * for ASCII mode, since Net ASCII's end-of-line sequence
	 * corresponds to the same thing used for DOS/Windows.
	 */

#if ASCII_TRANSLATION
	if (xtype == kTypeAscii) {
		/* ascii */
		for (;;) {
#if !defined(NO_SIGNALS)
			gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */
			nread = read(fd, inbuf, sizeof(inbuf));
			if (nread < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					result = kErrReadFailed;
					cip->errNo = kErrReadFailed;
					Error(cip, kDoPerror, "Local read failed.\n");
				}
				break;
			} else if (nread == 0) {
				break;
			}
			cip->bytesTransferred += (longest_int) nread;

#if !defined(NO_SIGNALS)
			gCanBrokenDataJmp = 1;
#endif	/* NO_SIGNALS */
			src = inbuf;
			srclim = src + nread;
			dst = cip->buf;		/* must be 2x sizeof inbuf or more. */
			while (src < srclim) {
				if (*src == '\n')
					*dst++ = '\r';
				*dst++ = *src++;
			}
			ntowrite = (size_t) (dst - cip->buf);
			cp = cip->buf;

#if !defined(NO_SIGNALS)
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
#endif	/* NO_SIGNALS */
			do {
				if (! WaitForRemoteOutput(cip)) {	/* could set cancelXfer */
					cip->errNo = result = kErrDataTimedOut;
					Error(cip, kDontPerror, "Remote write timed out.\n");
					goto brk;
				}
				if (cip->cancelXfer > 0) {
					FTPAbortDataTransfer(cip);
					result = cip->errNo = kErrDataTransferAborted;
					goto brk;
				}

#ifdef NO_SIGNALS
				nwrote = SWrite(cip->dataSocket, cp, (size_t) ntowrite, (int) cip->xferTimeout, kNoFirstSelect);
				if (nwrote < 0) {
					if (nwrote == kTimeoutErr) {
						cip->errNo = result = kErrDataTimedOut;
						Error(cip, kDontPerror, "Remote write timed out.\n");
					} else if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						cip->errNo = result = kErrSocketWriteFailed;
						errno = EPIPE;
						Error(cip, kDoPerror, "Lost data connection to remote host.\n");
					} else if (errno == EINTR) {
						continue;
					} else {
						cip->errNo = result = kErrSocketWriteFailed;
						Error(cip, kDoPerror, "Remote write failed.\n");
					}
					(void) shutdown(cip->dataSocket, 2);
					goto brk;
				}
#else	/* NO_SIGNALS */
				nwrote = write(cip->dataSocket, cp, ntowrite);
				if (nwrote < 0) {
					if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						cip->errNo = result = kErrSocketWriteFailed;
						errno = EPIPE;
						Error(cip, kDoPerror, "Lost data connection to remote host.\n");
					} else if (errno == EINTR) {
						continue;
					} else {
						cip->errNo = result = kErrSocketWriteFailed;
						Error(cip, kDoPerror, "Remote write failed.\n");
					}
					(void) shutdown(cip->dataSocket, 2);
					goto brk;
				}
#endif	/* NO_SIGNALS */
				cp += nwrote;
				ntowrite -= nwrote;
			} while (ntowrite > 0);
			FTPUpdateIOTimer(cip);
		}
	} else
#endif	/* ASCII_TRANSLATION */
	{
		/* binary */
		for (;;) {
#if !defined(NO_SIGNALS)
			gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */
			cp = buf;
			nread = read(fd, cp, bufSize);
			if (nread < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					result = kErrReadFailed;
					cip->errNo = kErrReadFailed;
					Error(cip, kDoPerror, "Local read failed.\n");
				}
				break;
			} else if (nread == 0) {
				break;
			}
			cip->bytesTransferred += (longest_int) nread;

#if !defined(NO_SIGNALS)
			gCanBrokenDataJmp = 1;
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
#endif	/* NO_SIGNALS */
			do {
				if (! WaitForRemoteOutput(cip)) {	/* could set cancelXfer */
					cip->errNo = result = kErrDataTimedOut;
					Error(cip, kDontPerror, "Remote write timed out.\n");
					goto brk;
				}
				if (cip->cancelXfer > 0) {
					FTPAbortDataTransfer(cip);
					result = cip->errNo = kErrDataTransferAborted;
					goto brk;
				}

#ifdef NO_SIGNALS
				nwrote = SWrite(cip->dataSocket, cp, (size_t) nread, (int) cip->xferTimeout, kNoFirstSelect);
				if (nwrote < 0) {
					if (nwrote == kTimeoutErr) {
						cip->errNo = result = kErrDataTimedOut;
						Error(cip, kDontPerror, "Remote write timed out.\n");
					} else if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						cip->errNo = result = kErrSocketWriteFailed;
						errno = EPIPE;
						Error(cip, kDoPerror, "Lost data connection to remote host.\n");
					} else if (errno == EINTR) {
						continue;
					} else {
						cip->errNo = result = kErrSocketWriteFailed;
						Error(cip, kDoPerror, "Remote write failed.\n");
					}
					(void) shutdown(cip->dataSocket, 2);
					cip->dataSocket = -1;
					goto brk;
				}
#else	/* NO_SIGNALS */
				nwrote = write(cip->dataSocket, cp, nread);
				if (nwrote < 0) {
					if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						cip->errNo = result = kErrSocketWriteFailed;
						errno = EPIPE;
						Error(cip, kDoPerror, "Lost data connection to remote host.\n");
					} else if (errno == EINTR) {
						continue;
					} else {
						cip->errNo = result = kErrSocketWriteFailed;
						Error(cip, kDoPerror, "Remote write failed.\n");
					}
					(void) shutdown(cip->dataSocket, 2);
					cip->dataSocket = -1;
					goto brk;
				}
#endif	/* NO_SIGNALS */
				cp += nwrote;
				nread -= nwrote;
			} while (nread > 0);
			FTPUpdateIOTimer(cip);
		}
	}
brk:

	if (fdtouse < 0) {
		(void) Fstat(fd, &st);
	}

	if (fdtouse < 0) {
		if (shutdown(fd, 1) == 0) {
			/* This looks very bizarre, since
			 * we will be checking the socket
			 * for readability here!
			 *
			 * The reason for this is that we
			 * want to be able to timeout a
			 * small put.  So, we close the
			 * write end of the socket first,
			 * which tells the server we're
			 * done writing.  We then wait
			 * for the server to close down
			 * the whole socket, which tells
			 * us that the file was completed.
			 */
			(void) WaitForRemoteInput(cip);	/* Close could block. */
		}
	}

#if !defined(NO_SIGNALS)
	gCanBrokenDataJmp = 0;
	if (cip->xferTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */
	tmpResult = FTPEndDataCmd(cip, 1);
	if ((tmpResult < 0) && (result == kNoErr)) {
		cip->errNo = result = kErrSTORFailed;
	}
	FTPStopIOTimer(cip);

	if (fdtouse < 0) {
		/* If they gave us a descriptor (fdtouse >= 0),
		 * leave it open, otherwise we opened it, so
		 * we need to dispose of it.
		 */
		(void) close(fd);
		fd = -1;
	}

	if (result == kNoErr) {
		/* The store succeeded;  If we were
		 * uploading to a temporary file,
		 * move the new file to the new name.
		 */
		cip->numUploads++;

		if ((tmppfx[0] != '\0') || (tmpsfx[0] != '\0')) {
			if ((result = FTPRename(cip, dstfile, odstfile)) < 0) {
				/* May fail if file was already there,
				 * so delete the old one so we can move
				 * over it.
				 */
				if (FTPDelete(cip, odstfile, kRecursiveNo, kGlobNo) == kNoErr) {
					result = FTPRename(cip, dstfile, odstfile);
					if (result < 0) {
						Error(cip, kDontPerror, "Could not rename %s to %s: %s.\n", dstfile, odstfile, FTPStrError(cip->errNo));
					}
				} else {
					Error(cip, kDontPerror, "Could not delete old %s, so could not rename %s to that: %s\n", odstfile, dstfile, FTPStrError(cip->errNo));
				}
			}
		}

		if (FTPUtime(cip, odstfile, st.st_atime, st.st_mtime, st.st_ctime) != kNoErr) {
			if (cip->errNo != kErrUTIMENotAvailable)
				Error(cip, kDontPerror, "Could not preserve times for %s: %s.\n", odstfile, FTPStrError(cip->errNo));
		}

		if (deleteflag == kDeleteYes) {
			if (unlink(file) < 0) {
				result = cip->errNo = kErrLocalDeleteFailed;
			}
		}
	}

#if !defined(NO_SIGNALS)
	(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
	return (result);
}	/* FTPPutOneF */




int
FTPPutOneFile3(
	const FTPCIPtr cip,
	const char *const file,
	const char *const dstfile,
	const int xtype,
	const int fdtouse,
	const int appendflag,
	const char *const tmppfx,
	const char *const tmpsfx,
	const int resumeflag,
	const int deleteflag,
	const ConfirmResumeUploadProc resumeProc,
	int UNUSED(reserved))
{
	int result;

	LIBNCFTP_USE_VAR(reserved);
	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	
	if ((dstfile == NULL) || (dstfile[0] == '\0'))
		return (kErrBadParameter);
	if (fdtouse < 0) {
		if ((file == NULL) || (file[0] == '\0'))
			return (kErrBadParameter);
	}
	result = FTPPutOneF(cip, file, dstfile, xtype, fdtouse, appendflag, tmppfx, tmpsfx, resumeflag, deleteflag, resumeProc);
	return (result);
}	/* FTPPutOneFile3 */




int
FTPPutFiles3(
	const FTPCIPtr cip,
	const char *const pattern,
	const char *const dstdir1,
	const int recurse,
	const int doGlob,
	const int xtype,
	int appendflag,
	const char *const tmppfx,
	const char *const tmpsfx,
	const int resumeflag,
	const int deleteflag,
	const ConfirmResumeUploadProc resumeProc,
	int UNUSED(reserved))
{
	LineList globList;
	FileInfoList files;
	FileInfoPtr filePtr;
	int batchResult;
	int result;
	const char *dstdir;
	char dstdir2[512];

	LIBNCFTP_USE_VAR(reserved);
	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (dstdir1 == NULL) {
		dstdir = NULL;
	} else {
		dstdir = STRNCPY(dstdir2, dstdir1);
		StrRemoveTrailingLocalPathDelim(dstdir2);
	}

	(void) FTPLocalGlob(cip, &globList, pattern, doGlob);
	if (recurse == kRecursiveYes) {
		appendflag = kAppendNo;
		(void) FTPLocalRecursiveFileList(cip, &globList, &files);
		if (files.first == NULL) {
			cip->errNo = kErrNoValidFilesSpecified;
			return (kErrNoValidFilesSpecified);
		}
		(void) ComputeRNames(&files, dstdir, 0, 1);
	} else {
		(void) LineListToFileInfoList(&globList, &files);
		(void) ComputeLNames(&files, NULL, NULL, 1);
		(void) ComputeRNames(&files, dstdir, 0, 0);
	}
	DisposeLineListContents(&globList);

#if 0
	for (filePtr = files.first; filePtr != NULL; filePtr = filePtr->next) {
		PrintF(cip, "  R=%s, L=%s, 2=%s, size=%d, mdtm=%u, type=%c\n",
			filePtr->rname,
			filePtr->lname,
			filePtr->rlinkto ? filePtr->rlinkto : "",
			filePtr->size,
			(unsigned int) filePtr->mdtm,
			filePtr->type
		);
	}
#endif

	batchResult = kNoErr;
	for (filePtr = files.first; filePtr != NULL; filePtr = filePtr->next) {
		if (cip->connected == 0) {
			if (batchResult == kNoErr)
				batchResult = kErrRemoteHostClosedConnection;
			break;
		}
		if (filePtr->type == 'd') {
			/* mkdir */
			StrRemoveTrailingLocalPathDelim(filePtr->rname);
			result = FTPMkdir(cip, filePtr->rname, kRecursiveNo);
			if (result != kNoErr)
				batchResult = result;
#ifdef HAVE_SYMLINK
		} else if (filePtr->type == 'l') {
			/* symlink */
			/* no RFC way to create the link, though. */
			if ((filePtr->rlinkto != NULL) && (filePtr->rlinkto[0] != '\0'))
				(void) FTPSymlink(cip, filePtr->rname, filePtr->rlinkto);
#endif
		} else if (recurse != kRecursiveYes) {
			result = FTPPutOneF(cip, filePtr->lname, filePtr->rname, xtype, -1, appendflag, tmppfx, tmpsfx, resumeflag, deleteflag, resumeProc);
			if (files.nFileInfos == 1) {
				if (result != kNoErr)
					batchResult = result;
			} else {
				if ((result != kNoErr) && (result != kErrLocalFileNewer) && (result != kErrRemoteFileNewer) && (result != kErrLocalSameAsRemote))
					batchResult = result;
			}
			if (result == kErrUserCanceled)
				cip->cancelXfer = 1;
			if (cip->cancelXfer > 0)
				break;
		} else {
			result = FTPPutOneF(cip, filePtr->lname, filePtr->rname, xtype, -1, appendflag, tmppfx, tmpsfx, resumeflag, deleteflag, resumeProc);
			if (files.nFileInfos == 1) {
				if (result != kNoErr)
					batchResult = result;
			} else {
				if ((result != kNoErr) && (result != kErrLocalFileNewer) && (result != kErrRemoteFileNewer) && (result != kErrLocalSameAsRemote))
					batchResult = result;
			}
			if (result == kErrUserCanceled)
				cip->cancelXfer = 1;
			if (cip->cancelXfer > 0)
				break;
		}
	}
	DisposeFileInfoListContents(&files);
	if (batchResult < 0)
		cip->errNo = batchResult;
	return (batchResult);
}	/* FTPPutFiles3 */




/* The purpose of this is to provide updates for the progress meters
 * during lags.  Return zero if the operation timed-out.
 */
static int
WaitForRemoteInput(const FTPCIPtr cip)
{
	fd_set ss, ss2;
	struct timeval tv;
	int result;
	int fd;
	int wsecs;
	int xferTimeout;
	int ocancelXfer;

	xferTimeout = cip->xferTimeout;
	if (xferTimeout < 1)
		return (1);

	fd = cip->dataSocket;
	if (fd < 0)
		return (1);

	ocancelXfer = cip->cancelXfer;
	wsecs = 0;
	cip->stalled = 0;

	while ((xferTimeout <= 0) || (wsecs < xferTimeout)) {
		if ((cip->cancelXfer != 0) && (ocancelXfer == 0)) {
			/* leave cip->stalled -- could have been stalled and then canceled. */
			return (1);
		}
		FD_ZERO(&ss);
		FD_SET(fd, &ss);
		ss2 = ss;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		result = select(fd + 1, SELECT_TYPE_ARG234 &ss, NULL, SELECT_TYPE_ARG234 &ss2, &tv);
		if (result == 1) {
			/* ready */
			cip->stalled = 0;
			return (1);
		} else if (result < 0) {
			if (result != EINTR) {
				perror("select");
				cip->stalled = 0;
				return (1);
			}
		} else {
			wsecs++;
			cip->stalled = wsecs;
		}
		FTPUpdateIOTimer(cip);
	}

#if !defined(NO_SIGNALS)
	/* Shouldn't get here -- alarm() should have
	 * went off by now.
	 */
	(void) kill(getpid(), SIGALRM);
#endif	/* NO_SIGNALS */

	cip->dataTimedOut = 1;
	return (0);	/* timed-out */
}	/* WaitForRemoteInput */




/* Nice for UNIX, but not necessary otherwise. */
#ifdef TAR

static int
OpenTar(const FTPCIPtr cip, const char *const dstdir, int *const pid)
{
	int pipe1[2];
	int pid1;
	int i;
	char *argv[8];

	*pid = -1;

	if (access(TAR, X_OK) < 0) {
		/* Path to TAR is invalid. */
		return (-1);
	}

	if (pipe(pipe1) < 0) {
		Error(cip, kDoPerror, "pipe to Tar failed");
		return (-1);
	}

	pid1 = (int) fork();
	if (pid1 < 0) {
		(void) close(pipe1[0]);
		(void) close(pipe1[1]);
		return (-1);
	} else if (pid1 == 0) {
		/* Child */
		if ((dstdir != NULL) && (dstdir[0] != '\0') && (chdir(dstdir) < 0)) {
			Error(cip, kDoPerror, "tar chdir to %s failed", dstdir);
			exit(1);
		}
		(void) close(pipe1[1]);		/* close write end */
		(void) dup2(pipe1[0], 0);	/* use read end on stdin */
		(void) close(pipe1[0]);

		for (i=3; i<256; i++)
			(void) close(i);

		argv[0] = (char *) "tar";
		argv[1] = (char *) "xpf";
		argv[2] = (char *) "-";
		argv[3] = NULL;

		(void) execv(TAR, argv);
		exit(1);
	}

	/* Parent */
	*pid = pid1;

	(void) close(pipe1[0]);		/* close read end */
	return (pipe1[1]);		/* use write end */
}	/* OpenTar */




static int
FTPGetOneTarF(const FTPCIPtr cip, const char *file, const char *const dstdir)
{
	char *buf;
	size_t bufSize;
	int tmpResult;
	volatile int result;
	int nread, nwrote;
	volatile int fd;
	volatile int vfd;
	const char *volatile vfile;
#ifndef NO_SIGNALS
	int sj;
	volatile FTPSigProc osigpipe;
	volatile FTPCIPtr vcip;
#endif
	int pid, status;
	char savedCwd[512];
	char *volatile basecp;

	result = kNoErr;
	cip->usingTAR = 0;

	if ((file[0] == '\0') || ((file[0] == '/') && (file[1] == '\0'))) {
		/* It was "/"
		 * We can't do that, because "get /.tar"
		 * or "get .tar" does not work.
		 */
		result = kErrOpenFailed;
		cip->errNo = kErrOpenFailed;
		return (result);
	}

	if (FTPCmd(cip, "MDTM %s.tar", file) == 2) {
		/* Better not use this method since there is
		 * no way to tell if the server would use the
		 * existing .tar or do a new one on the fly.
		 */
		result = kErrOpenFailed;
		cip->errNo = kErrOpenFailed;
		return (result);
	}

	basecp = strrchr(file, '/');
	if (basecp != NULL)
		basecp = strrchr(file, '\\');
	if (basecp != NULL) {
		/* Need to cd to the parent directory and get it
		 * from there.
		 */
		if (FTPGetCWD(cip, savedCwd, sizeof(savedCwd)) != 0) {
			result = kErrOpenFailed;
			cip->errNo = kErrOpenFailed;
			return (result);
		}
		result = FTPChdir(cip, file);
		if (result != kNoErr) {
			return (result);
		}
		result = FTPChdir(cip, "..");
		if (result != kNoErr) {
			(void) FTPChdir(cip, savedCwd);
			return (result);
		}
		file = basecp + 1;
	}

	fd = OpenTar(cip, dstdir, &pid);
	if (fd < 0) {
		result = kErrOpenFailed;
		cip->errNo = kErrOpenFailed;
		if (basecp != NULL)
			(void) FTPChdir(cip, savedCwd);
		return (result);
	}

	vfd = fd;
	vfile = file;

#ifndef NO_SIGNALS
	vcip = cip;
	osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenData);

	gGotBrokenData = 0;
	gCanBrokenDataJmp = 0;

#ifdef HAVE_SIGSETJMP
	sj = sigsetjmp(gBrokenDataJmp, 1);
#else
	sj = setjmp(gBrokenDataJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
		FTPShutdownHost(vcip);

		(void) signal(SIGPIPE, SIG_IGN);
		(void) close(vfd);
		for (;;) {
#ifdef HAVE_WAITPID
			if ((waitpid(pid, &status, 0) < 0) && (errno != EINTR))
				break;
#else
			if ((wait(&status) < 0) && (errno != EINTR))
				break;
#endif
			if (WIFEXITED(status) || WIFSIGNALED(status))
				break;		/* done */
		}
		if (basecp != NULL)
			(void) FTPChdir(cip, savedCwd);
		vcip->errNo = kErrRemoteHostClosedConnection;
		return(vcip->errNo);
	}
	gCanBrokenDataJmp = 1;

#endif	/* NO_SIGNALS */

	tmpResult = FTPStartDataCmd(cip, kNetReading, kTypeBinary, (longest_int) 0, "RETR %s.tar", vfile);

	if (tmpResult < 0) {
		result = tmpResult;
		if (result == kErrGeneric)
			result = kErrRETRFailed;
		cip->errNo = result;

#ifndef NO_SIGNALS
		(void) signal(SIGPIPE, SIG_IGN);
#endif
		(void) close(vfd);
		for (;;) {
#ifdef HAVE_WAITPID
			if ((waitpid(pid, &status, 0) < 0) && (errno != EINTR))
				break;
#else
			if ((wait(&status) < 0) && (errno != EINTR))
				break;
#endif
			if (WIFEXITED(status) || WIFSIGNALED(status))
				break;		/* done */
		}

#ifndef NO_SIGNALS
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif
		if (basecp != NULL)
			(void) FTPChdir(cip, savedCwd);
		return (result);
	}

	cip->usingTAR = 1;
	buf = cip->buf;
	bufSize = cip->bufSize;

	FTPInitIOTimer(cip);
	cip->lname = vfile;	/* could be NULL */
	cip->rname = vfile;
	FTPStartIOTimer(cip);

	/* Binary */
	for (;;) {
		if (! WaitForRemoteInput(cip)) {	/* could set cancelXfer */
			cip->errNo = result = kErrDataTimedOut;
			Error(cip, kDontPerror, "Remote read timed out.\n");
			break;
		}
		if (cip->cancelXfer > 0) {
			FTPAbortDataTransfer(cip);
			result = cip->errNo = kErrDataTransferAborted;
			break;
		}
#if !defined(NO_SIGNALS)
		gCanBrokenDataJmp = 1;
		if (cip->xferTimeout > 0)
			(void) alarm(cip->xferTimeout);
#endif	/* NO_SIGNALS */
#ifdef NO_SIGNALS
		nread = SRead(cip->dataSocket, buf, bufSize, (int) cip->xferTimeout, kFullBufferNotRequired|kNoFirstSelect);
		if (nread == kTimeoutErr) {
			cip->errNo = result = kErrDataTimedOut;
			Error(cip, kDontPerror, "Remote read timed out.\n");
			break;
		} else if (nread < 0) {
			if (errno == EINTR)
				continue;
			Error(cip, kDoPerror, "Remote read failed.\n");
			result = kErrSocketReadFailed;
			cip->errNo = kErrSocketReadFailed;
			break;
		} else if (nread == 0) {
			break;
		}
#else
		nread = read(cip->dataSocket, buf, bufSize);
		if (nread < 0) {
			if (errno == EINTR)
				continue;
			Error(cip, kDoPerror, "Remote read failed.\n");
			result = kErrSocketReadFailed;
			cip->errNo = kErrSocketReadFailed;
			break;
		} else if (nread == 0) {
			break;
		}
		gCanBrokenDataJmp = 0;
#endif

		nwrote = write(fd, buf, nread);
		if (nwrote != nread) {
			if ((gGotBrokenData != 0) || (errno == EPIPE)) {
				result = kErrWriteFailed;
				cip->errNo = kErrWriteFailed;
				errno = EPIPE;
			} else {
				Error(cip, kDoPerror, "Local write failed.\n");
				result = kErrWriteFailed;
				cip->errNo = kErrWriteFailed;
			}
			break;
		}
		cip->bytesTransferred += (longest_int) nread;
		FTPUpdateIOTimer(cip);
	}

#if !defined(NO_SIGNALS)
	if (cip->xferTimeout > 0)
		(void) alarm(0);
	gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */

	(void) close(fd);
	for (;;) {
#ifdef HAVE_WAITPID
		if ((waitpid(pid, &status, 0) < 0) && (errno != EINTR))
			break;
#else
		if ((wait(&status) < 0) && (errno != EINTR))
			break;
#endif
		if (WIFEXITED(status) || WIFSIGNALED(status))
			break;		/* done */
	}

	tmpResult = FTPEndDataCmd(cip, 1);
	if ((tmpResult < 0) && (result == 0)) {
		result = kErrRETRFailed;
		cip->errNo = kErrRETRFailed;
	}
	FTPStopIOTimer(cip);
#if !defined(NO_SIGNALS)
	(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif

	if ((result == 0) && (cip->bytesTransferred == 0)) {
		result = kErrOpenFailed;
		cip->errNo = kErrOpenFailed;
	}
	if (basecp != NULL)
		(void) FTPChdir(cip, savedCwd);
	return (result);
}	/* FTPGetOneTarF */

#endif	/* TAR */





static int
FTPGetOneF(
	const FTPCIPtr cip,
	const char *const file,
	const char *dstfile,
	int xtype,
	const int fdtouse,
	longest_int expectedSize,
	time_t mdtm,
	const int resumeflag,
	const int appendflag,
	const int deleteflag,
	const ConfirmResumeDownloadProc resumeProc)
{
	char *buf;
	size_t bufSize;
	int tmpResult;
	volatile int result;
	int nread, nwrote;
	volatile int fd;
#if ASCII_TRANSLATION
	char *src, *srclim;
	char *dst, *dstlim;
	char outbuf[512];
#endif
	volatile longest_int startPoint = 0;
	struct utimbuf ut;
	struct Stat st;
#if !defined(NO_SIGNALS)
	volatile FTPSigProc osigpipe;
	volatile FTPCIPtr vcip;
	volatile int vfd, vfdtouse;
	int sj;
#endif	/* NO_SIGNALS */
	volatile int created = 0;
	int zaction = kConfirmResumeProcSaidBestGuess;
	int statrc;
	int noMdtmCheck;
	time_t now;

	if (cip->buf == NULL) {
		Error(cip, kDoPerror, "Transfer buffer not allocated.\n");
		cip->errNo = kErrNoBuf;
		return (cip->errNo);
	}

	result = kNoErr;
	cip->usingTAR = 0;

	if (fdtouse < 0) {
		/* Only ask for extended information
		 * if we have the name of the file
		 * and we didn't already have the
		 * info.
		 *
		 * Always ask for the modification time,
		 * because even if it was passed in it
		 * may not be accurate.  This is often
		 * the case when it came from an ls
		 * listing, in which the local time
		 * zone could be a factor.
		 *
		 */

		AutomaticallyUseASCIIModeDependingOnExtension(cip, file, &xtype);
		if (expectedSize == kSizeUnknown) {
			(void) FTPFileSizeAndModificationTime(cip, file, &expectedSize, xtype, &mdtm);
		} else {
			(void) FTPFileModificationTime(cip, file, &mdtm);
		}

		/* For Get, we can't recover very well if it turns out restart
		 * didn't work, so check beforehand.
		 */
		if ((resumeflag == kResumeYes) || (resumeProc != NoConfirmResumeDownloadProc)) {
			if (cip->hasREST == kCommandAvailabilityUnknown) {
				(void) FTPSetTransferType(cip, kTypeBinary);
				if (SetStartOffset(cip, (longest_int) 1) == kNoErr) {
					/* Now revert -- we still may not end up
					 * doing it.
					 */
					SetStartOffset(cip, (longest_int) -1);
				}
			}
		}

		if (appendflag == kAppendYes) {
			zaction = kConfirmResumeProcSaidAppend;
		} else if (cip->hasREST == kCommandNotAvailable) {
			zaction = kConfirmResumeProcSaidOverwrite;
		} else if (resumeflag == kResumeYes) {
			zaction = kConfirmResumeProcSaidBestGuess;
		} else {
			zaction = kConfirmResumeProcSaidOverwrite;
		}

		statrc = Stat(dstfile, &st);
		if (statrc == 0) {
			if (resumeProc != NULL) {
				zaction = (*resumeProc)(
						&dstfile,
						(longest_int) st.st_size,
						st.st_mtime,
						file,
						expectedSize,
						mdtm,
						&startPoint
				);
			}

			if (zaction == kConfirmResumeProcSaidBestGuess) {
				if (expectedSize != kSizeUnknown) {
					/* We know the size of the remote file,
					 * and we have a local file too.
					 *
					 * Try and decide if we need to get
					 * the entire file, or just part of it.
					 */

					startPoint = (longest_int) st.st_size;
					zaction = kConfirmResumeProcSaidResume;

					/* If the local file exists and has a recent
					 * modification time (< 12 hours) and
					 * the remote file's modtime is not recent,
					 * then heuristically conclude that the
					 * local modtime should not be trusted
					 * (i.e. user killed the process before
					 * the local modtime could be preserved).
					 */
					noMdtmCheck = 0;
					if (mdtm != kModTimeUnknown) {
						time(&now);
						if ((st.st_mtime > now) || (((now - st.st_mtime) < 46200) && ((now - mdtm) >= 46200)))
							noMdtmCheck = 1;
					}

					if ((mdtm == kModTimeUnknown) || (noMdtmCheck != 0)) {
						/* Can't use the timestamps as an aid. */
						if (startPoint == expectedSize) {
							/* Don't go to all the trouble of downloading nothing. */
							cip->errNo = kErrLocalSameAsRemote;
							if (deleteflag == kDeleteYes)
								(void) FTPDelete(cip, file, kRecursiveNo, kGlobNo);
							return (cip->errNo);
						} else if (startPoint > expectedSize) {
							/* Panic;  odds are the file we have
							 * was a different file altogether,
							 * since it is larger than the
							 * remote copy.  Re-do it all.
							 */
							zaction = kConfirmResumeProcSaidOverwrite;
						} /* else resume at startPoint */
					} else if ((mdtm == st.st_mtime) || (mdtm == (st.st_mtime - 1)) || (mdtm == (st.st_mtime + 1))) {
						/* File has the same time.
						 * Note: Windows' file timestamps can be off by one second!
						 */
						if (startPoint == expectedSize) {
							/* Don't go to all the trouble of downloading nothing. */
							cip->errNo = kErrLocalSameAsRemote;
							if (deleteflag == kDeleteYes)
								(void) FTPDelete(cip, file, kRecursiveNo, kGlobNo);
							return (cip->errNo);
						} else if (startPoint > expectedSize) {
							/* Panic;  odds are the file we have
							 * was a different file altogether,
							 * since it is larger than the
							 * remote copy.  Re-do it all.
							 */
							zaction = kConfirmResumeProcSaidOverwrite;
						} else {
							/* We have a file by the same time,
							 * but smaller start point.  Leave
							 * the startpoint as is since it
							 * is most likely valid.
							 */
						}
					} else if (mdtm < st.st_mtime) {
						/* Remote file is older than
						 * local file.  Don't overwrite
						 * our file.
						 */
						cip->errNo = kErrLocalFileNewer;
						return (cip->errNo);
					} else /* if (mdtm > st.st_mtime) */ {
						/* File has a newer timestamp
						 * altogether, assume the remote
						 * file is an entirely new file
						 * and replace ours with it.
						 */
						zaction = kConfirmResumeProcSaidOverwrite;
					}
				} else {
						zaction = kConfirmResumeProcSaidOverwrite;
				}
			}
		} else {
			zaction = kConfirmResumeProcSaidOverwrite;
		}

		if (zaction == kConfirmResumeProcSaidCancel) {
			/* User wants to cancel this file and any
			 * remaining in batch.
			 */
			cip->errNo = kErrUserCanceled;
			return (cip->errNo);
		} else if (zaction == kConfirmResumeProcSaidSkip) {
			/* Nothing done, but not an error. */
			if (deleteflag == kDeleteYes)
				(void) FTPDelete(cip, file, kRecursiveNo, kGlobNo);
			return (kNoErr);
		} else if (zaction == kConfirmResumeProcSaidResume) {
			/* Resume; proc set the startPoint. */
			if (startPoint == expectedSize) {
				/* Don't go to all the trouble of downloading nothing. */
				/* Nothing done, but not an error. */
				if (deleteflag == kDeleteYes)
					(void) FTPDelete(cip, file, kRecursiveNo, kGlobNo);
				return (kNoErr);
			} else if (startPoint > expectedSize) {
				/* Cannot set start point past end of remote file */
				cip->errNo = result = kErrSetStartPoint;
				return (result);
			}
			fd = Open(dstfile, O_WRONLY|O_APPEND|O_BINARY, 00666);
		} else if (zaction == kConfirmResumeProcSaidAppend) {
			/* leave startPoint at zero, we will append everything. */
			startPoint = (longest_int) 0;
			fd = Open(dstfile, O_WRONLY|O_CREAT|O_APPEND|O_BINARY, 00666);
		} else /* if (zaction == kConfirmResumeProcSaidOverwrite) */ {
			created = 1;
			startPoint = (longest_int) 0;
			fd = Open(dstfile, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 00666);
		}

		if (fd < 0) {
			Error(cip, kDoPerror, "Cannot open local file %s for writing.\n", dstfile);
			result = kErrOpenFailed;
			cip->errNo = kErrOpenFailed;
			return (result);
		}

		if ((expectedSize == (longest_int) 0) && (startPoint <= (longest_int) 0) && (zaction != kConfirmResumeProcSaidOverwrite)) {
			/* Don't go to all the trouble of downloading nothing. */
#if defined(WIN32) || defined(_WINDOWS)
			/* Note: Windows doesn't allow zero-size files. */
			(void) write(fd, "\r\n", 2);
#endif
			(void) close(fd);
			if (mdtm != kModTimeUnknown) {
				cip->mdtm = mdtm;
				(void) time(&ut.actime);
				ut.modtime = mdtm;
				(void) utime(dstfile, &ut);
			}
			if (deleteflag == kDeleteYes)
				(void) FTPDelete(cip, file, kRecursiveNo, kGlobNo);
			return (kNoErr);
		}
	} else {
		fd = fdtouse;
	}

	if ((cip->numDownloads == 0) && (cip->dataSocketRBufSize > 0)) {
		/* If dataSocketSBufSize is non-zero, it means you
		 * want to explicitly try to set the size of the
		 * socket's I/O buffer.
		 *
		 * If it is zero, it means you want to just use the
		 * TCP stack's default value, which is typically
		 * between 8 and 64 kB.
		 *
		 * If you try to set the buffer larger than 64 kB,
		 * the TCP stack should try to use RFC 1323 to
		 * negotiate "TCP Large Windows" which may yield
		 * significant performance gains.
		 */
		if (cip->hasRETRBUFSIZE == kCommandAvailable)
			(void) FTPCmd(cip, "SITE RETRBUFSIZE %lu", (unsigned long) cip->dataSocketRBufSize);
		else if (cip->hasRBUFSIZ == kCommandAvailable)
			(void) FTPCmd(cip, "SITE RBUFSIZ %lu", (unsigned long) cip->dataSocketRBufSize);
		else if (cip->hasRBUFSZ == kCommandAvailable)
			(void) FTPCmd(cip, "SITE RBUFSZ %lu", (unsigned long) cip->dataSocketRBufSize);
		else if (cip->hasBUFSIZE == kCommandAvailable)
			(void) FTPCmd(cip, "SITE BUFSIZE %lu", (unsigned long) cip->dataSocketSBufSize);
	}

#ifdef NO_SIGNALS
#else	/* NO_SIGNALS */
	vcip = cip;
	vfdtouse = fdtouse;
	vfd = fd;
	osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenData);

	gGotBrokenData = 0;
	gCanBrokenDataJmp = 0;

#ifdef HAVE_SIGSETJMP
	sj = sigsetjmp(gBrokenDataJmp, 1);
#else
	sj = setjmp(gBrokenDataJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
		if (vfdtouse < 0) {
			(void) close(vfd);
		}
		FTPShutdownHost(vcip);
		vcip->errNo = kErrRemoteHostClosedConnection;
		return(vcip->errNo);
	}
	gCanBrokenDataJmp = 1;
#endif	/* NO_SIGNALS */

	tmpResult = FTPStartDataCmd(cip, kNetReading, xtype, startPoint, "RETR %s", file);

	if (tmpResult < 0) {
		result = tmpResult;
		if (result == kErrGeneric)
			result = kErrRETRFailed;
		cip->errNo = result;
		if (fdtouse < 0) {
			(void) close(fd);
			if ((created != 0) && (appendflag == kAppendNo) && (cip->startPoint == 0))
				(void) unlink(dstfile);
		}
#if !defined(NO_SIGNALS)
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
		return (result);
	}

	if ((startPoint != 0) && (cip->startPoint == 0)) {
		/* Remote could not or would not set the start offset
		 * to what we wanted.
		 *
		 * So now we have to undo our seek.
		 */
		if (Lseek(fd, (off_t) 0, SEEK_SET) != (off_t) 0) {
			cip->errNo = kErrLseekFailed;
			if (fdtouse < 0) {
				(void) close(fd);
			}
#if !defined(NO_SIGNALS)
			(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */
			return (cip->errNo);
		}
		startPoint = 0;
	}

	buf = cip->buf;
	bufSize = cip->bufSize;

	FTPInitIOTimer(cip);
	cip->mdtm = mdtm;
	(void) time(&ut.actime);
	ut.modtime = mdtm;
	cip->expectedSize = expectedSize;
	cip->lname = dstfile;	/* could be NULL */
	cip->rname = file;
	if (fdtouse >= 0)
		cip->useProgressMeter = 0;
	FTPStartIOTimer(cip);

#if ASCII_TRANSLATION
	if (xtype == kTypeAscii) {
		/* Ascii */
		for (;;) {
			if (! WaitForRemoteInput(cip)) {	/* could set cancelXfer */
				cip->errNo = result = kErrDataTimedOut;
				Error(cip, kDontPerror, "Remote read timed out.\n");
				break;
			}
			if (cip->cancelXfer > 0) {
				FTPAbortDataTransfer(cip);
				result = cip->errNo = kErrDataTransferAborted;
				break;
			}
#ifdef TESTING_ABOR
			if (cip->bytesTransferred > 0) {
				cip->cancelXfer = 1;
				FTPAbortDataTransfer(cip);
				result = cip->errNo = kErrDataTransferAborted;
				break;
			}
#endif /* TESTING_ABOR */
#ifdef NO_SIGNALS
			nread = SRead(cip->dataSocket, buf, bufSize, (int) cip->xferTimeout, kFullBufferNotRequired|kNoFirstSelect);
			if (nread == kTimeoutErr) {
				cip->errNo = result = kErrDataTimedOut;
				Error(cip, kDontPerror, "Remote read timed out.\n");
				break;
			} else if (nread < 0) {
				if ((gGotBrokenData != 0) || (errno == EPIPE)) {
					result = cip->errNo = kErrSocketReadFailed;
					errno = EPIPE;
					Error(cip, kDoPerror, "Lost data connection to remote host.\n");
				} else if (errno == EINTR) {
					continue;
				} else {
					Error(cip, kDoPerror, "Remote read failed.\n");
					result = kErrSocketReadFailed;
					cip->errNo = kErrSocketReadFailed;
				}
				break;
			} else if (nread == 0) {
				break;
			}
#else
			gCanBrokenDataJmp = 1;
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
			nread = read(cip->dataSocket, buf, bufSize);
			if (nread < 0) {
				if ((gGotBrokenData != 0) || (errno == EPIPE)) {
					result = cip->errNo = kErrSocketReadFailed;
					errno = EPIPE;
					Error(cip, kDoPerror, "Lost data connection to remote host.\n");
					(void) shutdown(cip->dataSocket, 2);
				} else if (errno == EINTR) {
					continue;
				} else {
					result = cip->errNo = kErrSocketReadFailed;
					Error(cip, kDoPerror, "Remote read failed.\n");
					(void) shutdown(cip->dataSocket, 2);
				}
				break;
			} else if (nread == 0) {
				break;
			}

			gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */

			src = buf;
			srclim = src + nread;
			dst = outbuf;
			dstlim = dst + sizeof(outbuf);
			while (src < srclim) {
				if (*src == '\r') {
					src++;
					continue;
				}
				if (dst >= dstlim) {
					nwrote = write(fd, outbuf, (size_t) (dst - outbuf));
					if (nwrote == (int) (dst - outbuf)) {
						/* Success. */
						dst = outbuf;
					} else if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						result = kErrWriteFailed;
						cip->errNo = kErrWriteFailed;
						errno = EPIPE;
						(void) shutdown(cip->dataSocket, 2);
						goto brk;
					} else {
						Error(cip, kDoPerror, "Local write failed.\n");
						result = kErrWriteFailed;
						cip->errNo = kErrWriteFailed;
						(void) shutdown(cip->dataSocket, 2);
						goto brk;
					}
				}
				*dst++ = *src++;
			}
			if (dst > outbuf) {
				nwrote = write(fd, outbuf, (size_t) (dst - outbuf));
				if (nwrote != (int) (dst - outbuf)) {
					if ((gGotBrokenData != 0) || (errno == EPIPE)) {
						result = kErrWriteFailed;
						cip->errNo = kErrWriteFailed;
						errno = EPIPE;
						(void) shutdown(cip->dataSocket, 2);
						goto brk;
					} else {
						Error(cip, kDoPerror, "Local write failed.\n");
						result = kErrWriteFailed;
						cip->errNo = kErrWriteFailed;
						(void) shutdown(cip->dataSocket, 2);
						goto brk;
					}
				}
			}

			if (mdtm != kModTimeUnknown) {
				(void) utime(dstfile, &ut);
			}
			cip->bytesTransferred += (longest_int) nread;
			FTPUpdateIOTimer(cip);
		}
	} else
#endif	/* ASCII_TRANSLATION */
	{
		/* Binary */
		for (;;) {
			if (! WaitForRemoteInput(cip)) {	/* could set cancelXfer */
				cip->errNo = result = kErrDataTimedOut;
				Error(cip, kDontPerror, "Remote read timed out.\n");
				break;
			}
			if (cip->cancelXfer > 0) {
				FTPAbortDataTransfer(cip);
				result = cip->errNo = kErrDataTransferAborted;
				break;
			}
#ifdef TESTING_ABOR
			if (cip->bytesTransferred > 0) {
				cip->cancelXfer = 1;
				FTPAbortDataTransfer(cip);
				result = cip->errNo = kErrDataTransferAborted;
				break;
			}
#endif /* TESTING_ABOR */
#ifdef NO_SIGNALS
			nread = SRead(cip->dataSocket, buf, bufSize, (int) cip->xferTimeout, kFullBufferNotRequired|kNoFirstSelect);
			if (nread == kTimeoutErr) {
				cip->errNo = result = kErrDataTimedOut;
				Error(cip, kDontPerror, "Remote read timed out.\n");
				break;
			} else if (nread < 0) {
				if ((gGotBrokenData != 0) || (errno == EPIPE)) {
					result = cip->errNo = kErrSocketReadFailed;
					errno = EPIPE;
					Error(cip, kDoPerror, "Lost data connection to remote host.\n");
				} else if (errno == EINTR) {
					continue;
				} else {
					Error(cip, kDoPerror, "Remote read failed.\n");
					result = kErrSocketReadFailed;
					cip->errNo = kErrSocketReadFailed;
				}
				break;
			} else if (nread == 0) {
				break;
			}
#else			
			gCanBrokenDataJmp = 1;
			if (cip->xferTimeout > 0)
				(void) alarm(cip->xferTimeout);
			nread = read(cip->dataSocket, buf, bufSize);
			if (nread < 0) {
				if ((gGotBrokenData != 0) || (errno == EPIPE)) {
					result = cip->errNo = kErrSocketReadFailed;
					errno = EPIPE;
					Error(cip, kDoPerror, "Lost data connection to remote host.\n");
				} else if (errno == EINTR) {
					continue;
				} else {
					result = cip->errNo = kErrSocketReadFailed;
					Error(cip, kDoPerror, "Remote read failed.\n");
				}
				(void) shutdown(cip->dataSocket, 2);
				break;
			} else if (nread == 0) {
				break;
			}
			gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */

			nwrote = write(fd, buf, nread);
			if (nwrote != nread) {
				if ((gGotBrokenData != 0) || (errno == EPIPE)) {
					result = kErrWriteFailed;
					cip->errNo = kErrWriteFailed;
					errno = EPIPE;
				} else {
					Error(cip, kDoPerror, "Local write failed.\n");
					result = kErrWriteFailed;
					cip->errNo = kErrWriteFailed;
				}
				(void) shutdown(cip->dataSocket, 2);
				break;
			}

			/* Ugggh... do this after each write operation
			 * so it minimizes the chance of a user killing
			 * the process before we reset the timestamps.
			 */
			if (mdtm != kModTimeUnknown) {
				(void) utime(dstfile, &ut);
			}
			cip->bytesTransferred += (longest_int) nread;
			FTPUpdateIOTimer(cip);
		}
	}

#if ASCII_TRANSLATION
brk:
#endif

#if !defined(NO_SIGNALS)
	if (cip->xferTimeout > 0)
		(void) alarm(0);
	gCanBrokenDataJmp = 0;
#endif	/* NO_SIGNALS */

	if (fdtouse < 0) {
		/* If they gave us a descriptor (fdtouse >= 0),
		 * leave it open, otherwise we opened it, so
		 * we need to close it.
		 */
		(void) close(fd);
		fd = -1;
	}

	tmpResult = FTPEndDataCmd(cip, 1);
	if ((tmpResult < 0) && (result == 0)) {
		result = kErrRETRFailed;
		cip->errNo = kErrRETRFailed;
	}
	FTPStopIOTimer(cip);
#if !defined(NO_SIGNALS)
	(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
#endif	/* NO_SIGNALS */

	if ((mdtm != kModTimeUnknown) && (cip->bytesTransferred > 0)) {
		(void) utime(dstfile, &ut);
	}

	if (result == kNoErr) {
		cip->numDownloads++;

		if (deleteflag == kDeleteYes) {
			result = FTPDelete(cip, file, kRecursiveNo, kGlobNo);
		}
	}

	return (result);
}	/* FTPGetOneF */




int
FTPGetOneFile3(
	const FTPCIPtr cip,
	const char *const file,
	const char *const dstfile,
	const int xtype,
	const int fdtouse,
	const int resumeflag,
	const int appendflag,
	const int deleteflag,
	const ConfirmResumeDownloadProc resumeProc,
	int UNUSED(reserved))
{
	int result;

	LIBNCFTP_USE_VAR(reserved);
	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	
	if ((file == NULL) || (file[0] == '\0'))
		return (kErrBadParameter);
	if (fdtouse < 0) {
		if ((dstfile == NULL) || (dstfile[0] == '\0'))
			return (kErrBadParameter);
	}

	result = FTPGetOneF(cip, file, dstfile, xtype, fdtouse, kSizeUnknown, kModTimeUnknown, resumeflag, appendflag, deleteflag, resumeProc);
	return (result);
}	/* FTPGetOneFile3 */




int
FTPGetFiles3(
	const FTPCIPtr cip,
	const char *pattern1,
	const char *const dstdir1,
	const int recurse,
	int doGlob,
	const int xtype,
	const int resumeflag,
	int appendflag,
	const int deleteflag,
	const int tarflag,
	const ConfirmResumeDownloadProc resumeProc,
	int UNUSED(reserved))
{
	LineList globList;
	LinePtr itemPtr;
	FileInfoList files;
	FileInfoPtr filePtr;
	int batchResult;
	int result;
	char *ldir;
	char *cp;
	const char *dstdir;
	const char *pattern;
	char *pattern2, *dstdir2;
	char c;
	int recurse1;
	int errRc;

	LIBNCFTP_USE_VAR(reserved);
	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);
	if (pattern1 == NULL)
		return (kErrBadParameter);

	dstdir2 = NULL;
	pattern2 = NULL;

	if (dstdir1 == NULL) {
		dstdir = NULL;
	} else {
		dstdir2 = StrDup(dstdir1);
		if (dstdir2 == NULL) {
			errRc = kErrMallocFailed;
			goto return_err;
		}
		StrRemoveTrailingLocalPathDelim(dstdir2);
		dstdir = dstdir2;
	}

	pattern2 = StrDup(pattern1);
	if (pattern2 == NULL) {
		errRc = kErrMallocFailed;
		goto return_err;
	}
	StrRemoveTrailingSlashes(pattern2);
	pattern = pattern2;

	if (pattern[0] == '\0') {
		if (recurse == kRecursiveNo) {
			errRc = kErrBadParameter;
			goto return_err;
		}
		pattern = ".";
		doGlob = kGlobNo;
	} else if (strcmp(pattern, ".") == 0) {
		if (recurse == kRecursiveNo) {
			errRc = kErrBadParameter;
			goto return_err;
		}
		doGlob = kGlobNo;
	}
	if (recurse == kRecursiveYes)
		appendflag = kAppendNo;

	batchResult = FTPRemoteGlob(cip, &globList, pattern, doGlob);
	if (batchResult != kNoErr) {
		errRc = batchResult;
		goto return_err;
	}

	cip->cancelXfer = 0;	/* should already be zero */

	for (itemPtr = globList.first; itemPtr != NULL; itemPtr = itemPtr->next) {
		if ((recurse == kRecursiveYes) && (FTPIsDir(cip, itemPtr->line) > 0)) {
#ifdef TAR
			if ((tarflag == kTarYes) && (xtype == kTypeBinary) && (appendflag == kAppendNo) && (deleteflag == kDeleteNo) && (FTPGetOneTarF(cip, itemPtr->line, dstdir) == kNoErr)) {
				/* Great! */
				continue;
			}
#endif	/* TAR */
			(void) FTPRemoteRecursiveFileList1(cip, itemPtr->line, &files);
			(void) ComputeLNames(&files, itemPtr->line, dstdir, 1);
			recurse1 = recurse;
		} else {
			recurse1 = kRecursiveNo;
			(void) LineToFileInfoList(itemPtr, &files);
			(void) ComputeRNames(&files, ".", 0, 1);
			(void) ComputeLNames(&files, NULL, dstdir, 0);
		}
		if (cip->cancelXfer > 0) {
			DisposeFileInfoListContents(&files);
			break;
		}

#if 0
		for (filePtr = files.first; filePtr != NULL; filePtr = filePtr->next) {
			PrintF(cip, "  R=%s, L=%s, 2=%s, size=%d, mdtm=%u, type=%c\n",
				filePtr->rname,
				filePtr->lname,
				filePtr->rlinkto ? filePtr->rlinkto : "",
				filePtr->size,
				(unsigned int) filePtr->mdtm,
				filePtr->type
			);
		}
#endif


		for (filePtr = files.first; filePtr != NULL; filePtr = filePtr->next) {
			if (cip->connected == 0) {
				if (batchResult == kNoErr)
					batchResult = kErrRemoteHostClosedConnection;
				break;
			}
			if (filePtr->type == 'd') {
#if defined(WIN32) || defined(_WINDOWS)
				(void) MkDirs(filePtr->lname, 00777);
#else
				(void) mkdir(filePtr->lname, 00777);
#endif
			} else if (filePtr->type == 'l') {
				/* skip it -- we do that next pass. */
			} else if (recurse1 != kRecursiveYes) {
				result = FTPGetOneF(cip, filePtr->rname, filePtr->lname, xtype, -1, filePtr->size, filePtr->mdtm, resumeflag, appendflag, deleteflag, resumeProc);
				if (files.nFileInfos == 1) {
					if (result != kNoErr)
						batchResult = result;
				} else {
					if ((result != kNoErr) && (result != kErrLocalFileNewer) && (result != kErrRemoteFileNewer) && (result != kErrLocalSameAsRemote))
						batchResult = result;
				}
				if (result == kErrUserCanceled)
					cip->cancelXfer = 1;
				if (cip->cancelXfer > 0)
					break;
			} else {
				ldir = filePtr->lname;
				cp = StrRFindLocalPathDelim(ldir);
				if (cp != NULL) {
					while (cp > ldir) {
						if (! IsLocalPathDelim(*cp)) {
							++cp;
							break;
						}
						--cp;
					}
					if (cp > ldir) {
						c = *cp;
						*cp = '\0';
						if (MkDirs(ldir, 00777) < 0) {
							Error(cip, kDoPerror, "Could not create local directory \"%s\"\n", ldir);
							batchResult = kErrGeneric;
							*cp = c;
							continue;
						}
						*cp = c;
					}
				}
				result = FTPGetOneF(cip, filePtr->rname, filePtr->lname, xtype, -1, filePtr->size, filePtr->mdtm, resumeflag, appendflag, deleteflag, resumeProc);

				if (files.nFileInfos == 1) {
					if (result != kNoErr)
						batchResult = result;
				} else {
					if ((result != kNoErr) && (result != kErrLocalFileNewer) && (result != kErrRemoteFileNewer) && (result != kErrLocalSameAsRemote))
						batchResult = result;
				}
				if (result == kErrUserCanceled)
					cip->cancelXfer = 1;
				if (cip->cancelXfer > 0)
					break;
			}
		}
		if (cip->cancelXfer > 0) {
			DisposeFileInfoListContents(&files);
			break;
		}

#ifdef HAVE_SYMLINK
		for (filePtr = files.first; filePtr != NULL; filePtr = filePtr->next) {
			if (filePtr->type == 'l') {
				(void) unlink(filePtr->lname);
				if (symlink(filePtr->rlinkto, filePtr->lname) < 0) {
					Error(cip, kDoPerror, "Could not symlink %s to %s\n", filePtr->rlinkto, filePtr->lname);
					/* Note: not worth setting batchResult */
				}
			}
		}
#endif	/* HAVE_SYMLINK */


		DisposeFileInfoListContents(&files);
	}

	DisposeLineListContents(&globList);
	if (batchResult < 0)
		cip->errNo = batchResult;
	errRc = batchResult;

return_err:
	if (dstdir2 != NULL)
		free(dstdir2);
	if (pattern2 != NULL)
		free(pattern2);
	return (errRc);
}	/* FTPGetFiles3 */




/*------------------------- wrappers for old routines ----------------------*/

int
FTPGetOneFile(const FTPCIPtr cip, const char *const file, const char *const dstfile)
{
	return (FTPGetOneFile3(cip, file, dstfile, kTypeBinary, -1, kResumeNo, kAppendNo, kDeleteNo, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetOneFile */




int
FTPGetOneFile2(const FTPCIPtr cip, const char *const file, const char *const dstfile, const int xtype, const int fdtouse, const int resumeflag, const int appendflag)
{
	return (FTPGetOneFile3(cip, file, dstfile, xtype, fdtouse, resumeflag, appendflag, kDeleteNo, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetOneFile2 */




int
FTPGetFiles(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob)
{
	return (FTPGetFiles3(cip, pattern, dstdir, recurse, doGlob, kTypeBinary, kResumeNo, kAppendNo, kDeleteNo, kTarYes, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetFiles */




int
FTPGetFiles2(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob, const int xtype, const int resumeflag, const int appendflag)
{
	return (FTPGetFiles3(cip, pattern, dstdir, recurse, doGlob, xtype, resumeflag, appendflag, kDeleteNo, kTarYes, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetFiles2 */




int
FTPGetOneFileAscii(const FTPCIPtr cip, const char *const file, const char *const dstfile)
{
	return (FTPGetOneFile3(cip, file, dstfile, kTypeAscii, -1, kResumeNo, kAppendNo, kDeleteNo, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetOneFileAscii */




int
FTPGetFilesAscii(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob)
{
	return (FTPGetFiles3(cip, pattern, dstdir, recurse, doGlob, kTypeAscii, kResumeNo, kAppendNo, kDeleteNo, kTarNo, (ConfirmResumeDownloadProc) 0, 0));
}	/* FTPGetFilesAscii */




int
FTPPutOneFile(const FTPCIPtr cip, const char *const file, const char *const dstfile)
{
	return (FTPPutOneFile3(cip, file, dstfile, kTypeBinary, -1, 0, NULL, NULL, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutOneFile */




int
FTPPutOneFile2(const FTPCIPtr cip, const char *const file, const char *const dstfile, const int xtype, const int fdtouse, const int appendflag, const char *const tmppfx, const char *const tmpsfx)
{
	return (FTPPutOneFile3(cip, file, dstfile, xtype, fdtouse, appendflag, tmppfx, tmpsfx, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutOneFile2 */




int
FTPPutFiles(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob)
{
	return (FTPPutFiles3(cip, pattern, dstdir, recurse, doGlob, kTypeBinary, 0, NULL, NULL, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutFiles */




int
FTPPutFiles2(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob, const int xtype, const int appendflag, const char *const tmppfx, const char *const tmpsfx)
{
	return (FTPPutFiles3(cip, pattern, dstdir, recurse, doGlob, xtype, appendflag, tmppfx, tmpsfx, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutFiles2 */




int
FTPPutOneFileAscii(const FTPCIPtr cip, const char *const file, const char *const dstfile)
{
	return (FTPPutOneFile3(cip, file, dstfile, kTypeAscii, -1, 0, NULL, NULL, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutOneFileAscii */




int
FTPPutFilesAscii(const FTPCIPtr cip, const char *const pattern, const char *const dstdir, const int recurse, const int doGlob)
{
	return (FTPPutFiles3(cip, pattern, dstdir, recurse, doGlob, kTypeAscii, 0, NULL, NULL, kResumeNo, kDeleteNo, NoConfirmResumeUploadProc, 0));
}	/* FTPPutFilesAscii */



int
FTPListToMemory(const FTPCIPtr cip, const char *const pattern, const LineListPtr llines, const char *const lsflags)
{
	return (FTPListToMemory2(cip, pattern, llines, lsflags, 1, (int *) 0));
}	/* FTPListToMemory */

/* eof IO.c */
