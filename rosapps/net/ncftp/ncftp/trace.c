/* trace.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "trace.h"
#include "util.h"

/* Saves their session in a ~/.ncftp/trace file.
 * This is nice for me when I need to diagnose problems.
 */
time_t gTraceTime;
FILE *gTraceFile = NULL;
char gTraceLBuf[256];
int gDebug = 0;

extern FTPLibraryInfo gLib;
extern FTPConnectionInfo gConn;
extern char gVersion[], gOS[];
extern char gOurDirectoryPath[];



/*VARARGS*/
void
Trace(const int level, const char *const fmt, ...)
{
	va_list ap;
	char buf[512];
	struct tm *ltp;

	if ((gDebug >= level) || (level > 8)) {
		va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
		(void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
		buf[sizeof(buf) - 1] = '\0';
#else
		(void) vsprintf(buf, fmt, ap);
#endif
		va_end(ap);

		(void) time(&gTraceTime);
		ltp = localtime(&gTraceTime);
		if ((gTraceFile != NULL) && (ltp != NULL)) {
			(void) fprintf(gTraceFile , "%02d:%02d:%02d  %s",
				ltp->tm_hour,
				ltp->tm_min,
				ltp->tm_sec,
				buf
			);
		}
		if (gDebug > level) {
			(void) fprintf(stdout, "%s", buf);
		}
	}
}	/* Trace */




void
ErrorHook(const FTPCIPtr UNUSED(cipUnused), char *msg)
{
	LIBNCFTP_USE_VAR(cipUnused);		/* shut up gcc */

	/* Will also get Trace'd. */
	(void) fprintf(stdout, "%s", msg);
}	/* ErrorHook */




void
DebugHook(const FTPCIPtr UNUSED(cipUnused), char *msg)
{
	LIBNCFTP_USE_VAR(cipUnused);		/* shut up gcc */
	Trace(0, "%s", msg);
}	/* DebugHook */




void
SetDebug(int i)
{
	gDebug = i;
}	/* SetDebug */




void
UseTrace(void)
{
	gConn.debugLogProc = DebugHook;
	gConn.debugLog = NULL;
	gConn.errLogProc = ErrorHook;
	gConn.errLog = NULL;
}	/* UseTrace */




void
OpenTrace(void)
{
	FILE *fp;
	char pathName[256];
	char tName[32];
	int pid;
	const char *cp;
#if defined(HAVE_SYS_UTSNAME_H) && defined(HAVE_UNAME)
	struct utsname u;
#endif

	if (gOurDirectoryPath[0] == '\0')
		return;		/* Don't create in root directory. */

	(void) sprintf(tName, "trace.%u", (unsigned int) (pid = getpid()));
	(void) OurDirectoryPath(pathName, sizeof(pathName), tName);

	fp = fopen(pathName, FOPEN_WRITE_TEXT);
	if (fp != NULL) {
		(void) chmod(pathName, 00600);
#ifdef HAVE_SETVBUF
		(void) setvbuf(fp, gTraceLBuf, _IOLBF, sizeof(gTraceLBuf));
#endif	/* HAVE_SETVBUF */
		/* Opened the trace file. */
		(void) time(&gTraceTime);
		(void) fprintf(fp, "SESSION STARTED at:  %s", ctime(&gTraceTime));
		(void) fprintf(fp, "   Program Version:  %s\n", gVersion + 5);
		(void) fprintf(fp, "   Library Version:  %s\n", gLibNcFTPVersion + 5);
		(void) fprintf(fp, "        Process ID:  %u\n", pid);
		if (gOS[0] != '\0')
			(void) fprintf(fp, "          Platform:  %s\n", gOS);
#if defined(HAVE_SYS_UTSNAME_H) && defined(HAVE_UNAME)
		if (uname(&u) == 0) {
			(void) fprintf(fp, "             Uname:  %.63s|%.63s|%.63s|%.63s|%.63s\r\n", u.sysname, u.nodename, u.release, u.version, u.machine);
		}
#endif	/* UNAME */
		FTPInitializeOurHostName(&gLib);
		(void) fprintf(fp, "          Hostname:  %s  (rc=%d)\n", gLib.ourHostName, gLib.hresult);
		cp = (const char *) getenv("TERM");
		if (cp == NULL)
			cp = "unknown?";
		(void) fprintf(fp, "          Terminal:  %s\n", cp);
		gTraceFile = fp;
	}
}	/* OpenTrace */




void
CloseTrace(void)
{
	char pathName[256];
	char pathName2[256];
	char tName[32];

	if ((gOurDirectoryPath[0] == '\0') || (gTraceFile == NULL))
		return;

	(void) sprintf(tName, "trace.%u", (unsigned int) getpid());
	(void) OurDirectoryPath(pathName, sizeof(pathName), tName);
	(void) OurDirectoryPath(pathName2, sizeof(pathName2), kTraceFileName);

	(void) time(&gTraceTime);
	(void) fprintf(gTraceFile, "SESSION ENDED at:    %s", ctime(&gTraceTime));
	(void) fclose(gTraceFile);

	(void) unlink(pathName2);
	(void) rename(pathName, pathName2);
}	/* CloseTrace */
