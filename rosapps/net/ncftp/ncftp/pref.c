/* pref.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#ifdef ncftp
#include "progress.h"
#endif

#include "pref.h"
#include "util.h"

/* How many times they've run this program. */
extern int gNumProgramRuns;

/* Their $PAGER. */
char gPager[128];

/* These correspond to the various timeouts from LibNcFTP. */
int gConnTimeout, gXferTimeout, gCtrlTimeout;

/* Active or passive FTP?  (PORT or PASV?)  Or both? */
extern int gDataPortMode, gFwDataPortMode;

/* When the destination file already exists, resume transfer or ask user? */
int gAutoResume;

/* "Save a bookmark to this site before closing?" */
int gConfirmClose;

/* Should we update the bookmark for the user? */
int gAutoSaveChangesToExistingBookmarks;

/* "Save your password with the bookmark?" */
int gSavePasswords;

int gMaySetXtermTitle;

/* Number of seconds between connection attempts. */
int gRedialDelay;

/* Some messages we only want to bug the user about once, ever. */
char gOneTimeMessagesSeen[256];

/* Tune the size of the socket buffer using SO_RCVBUF or SO_SNDBUF? */
int gSOBufsize;

/* Size of the user log before we trim it.  0 means do not log at all. */
int gMaxLogSize;

/* Use ASCII mode automatically for files with these extensions. */
char gAutoAscii[512];

#ifdef ncftp
/* Which meter to use. */
FTPProgressMeterProc gProgressMeter;
#endif

/* Allow us to plug our other products? */
int gDoNotDisplayAds;

/* Do we need to save the prefs, or can we skip it? */
int gPrefsDirty = 0;

extern FTPLibraryInfo gLib;
extern FTPConnectionInfo gConn;
extern char gOurDirectoryPath[], gUser[], gVersion[];

PrefOpt gPrefOpts[] = {
	{ "anonopen",				PREFOBSELETE },
	{ "anonpass", 				SetAnonPass, kPrefOptObselete },
	{ "anon-password",			SetAnonPass, 1 },
	{ "auto-ascii",				SetAutoAscii, 1 },
	{ "auto-resume",			SetAutoResume, 1 },
	{ "autosave-bookmark-changes",		SetAutoSaveChangesToExistingBookmarks, 1 },
	{ "blank-lines",			PREFOBSELETE },
	{ "confirm-close",			SetConfirmClose, 1 },
	{ "connect-timeout",			SetConnTimeout, 1 },
	{ "control-timeout",			SetCtrlTimeout, 1 },
	{ "logsize",				SetLogSize, 1 },
	{ "maxbookmarks",			PREFOBSELETE },
	{ "one-time-messages-seen",		SetOneTimeMessages, 0 },
	{ "pager",				SetPager, 1 },
	{ "passive",				SetPassive, 1 },
	{ "progress-meter",			SetProgressMeter, 1 },
	{ "redial-delay",			SetRedialDelay, 1 },
	{ "remote-msgs", 			PREFOBSELETE },
	{ "restore-lcwd", 			PREFOBSELETE },
	{ "save-passwords",			SetSavePasswords, 1 },
	{ "show-trailing-space",		PREFOBSELETE },
	{ "show-status-in-xterm-titlebar",	SetXtTitle, 1 },
#ifdef SO_RCVBUF
	{ "so-bufsize",				SetSOBufsize, 1 },
#endif
	{ "startup-lcwd", 			PREFOBSELETE },
	{ "startup-msgs", 			PREFOBSELETE },
	{ "timeout",				PREFOBSELETE },
	{ "total-runs", 			PREFOBSELETE },
	{ "total-xfer-hundredths-of-seconds", 	PREFOBSELETE },
	{ "total-xfer-kbytes", 			PREFOBSELETE },
	{ "trace",				PREFOBSELETE },
	{ "utime",				PREFOBSELETE },
	{ "visual",				PREFOBSELETE },
	{ "xfer-timeout",			SetXferTimeout, 1 },
	{ "yes-i-know-about-NcFTPd",		SetNoAds, 1 },
	{ NULL,					(PrefProc) 0, kPrefOptInvisible, },
};

int gNumPrefOpts = ((int)(sizeof(gPrefOpts) / sizeof(PrefOpt)) - 1);



void
SetAnonPass(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", gLib.defaultAnonPassword);
	} else {
		(void) STRNCPY(gLib.defaultAnonPassword, val);
	}
}	/* SetAnonPass */



void
SetAutoAscii(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", gAutoAscii);
	} else {
		(void) STRNCPY(gAutoAscii, val);
		if ((gAutoAscii[0] == '\0') || (ISTREQ(gAutoAscii, "no")) || (ISTREQ(gAutoAscii, "off")) || (ISTREQ(gAutoAscii, "false"))) {
			gConn.asciiFilenameExtensions = NULL;
		} else {
			gConn.asciiFilenameExtensions = gAutoAscii;
		}
	}
}	/* SetAutoAscii */



void
SetAutoResume(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", YESNO(gAutoResume));
	} else {
		gAutoResume = StrToBool(val);
	}
}	/* SetAutoResume */



void
SetAutoSaveChangesToExistingBookmarks(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", YESNO(gAutoSaveChangesToExistingBookmarks));
	} else {
		gAutoSaveChangesToExistingBookmarks = StrToBool(val);
	}
}	/* SetAutoSaveChangesToExistingBookmarks */



void
SetConfirmClose(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", YESNO(gConfirmClose));
	} else {
		gConfirmClose = StrToBool(val);
	}
}	/* SetConfirmClose */



void
SetConnTimeout(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gConnTimeout);
	} else {
		gConn.connTimeout = gConnTimeout = atoi(val);
	}
}	/* SetConnTimeout */



void
SetCtrlTimeout(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gCtrlTimeout);
	} else {
		gConn.ctrlTimeout = gCtrlTimeout = atoi(val);
	}
}	/* SetCtrlTimeout */



void
SetLogSize(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gMaxLogSize);
	} else {
		gMaxLogSize = atoi(val);
	}
}	/* SetLogSize */



void
SetNoAds(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", YESNO(gDoNotDisplayAds));
	} else {
		gDoNotDisplayAds = StrToBool(val);
	}
}	/* SetNoAds */



void
SetOneTimeMessages(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", gOneTimeMessagesSeen);
	} else {
		(void) STRNCPY(gOneTimeMessagesSeen, val);
	}
}	/* SetOneTimeMessages */



void
SetPager(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", gPager);
	} else {
		(void) STRNCPY(gPager, val);
	}
}	/* SetPager */



void
SetPassive(int UNUSED(t), const char *const val, FILE *const fp)
{
	int m;

	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		m = (gFwDataPortMode >= 0) ? gFwDataPortMode : gDataPortMode;
		if (m == kSendPortMode) {
			(void) fprintf(fp, "%s", "off");
		} else if (m == kPassiveMode) {
			(void) fprintf(fp, "%s", "on");
		} else {
			(void) fprintf(fp, "%s", "optional");
		}
	} else {
		if (gFwDataPortMode >= 0) {
			gDataPortMode = gFwDataPortMode;
			return;
		}
		if (ISTRNEQ(val, "opt", 3))
			gDataPortMode = kFallBackToSendPortMode;
		else if (ISTREQ(val, "on"))
			gDataPortMode = kPassiveMode;
		else if ((int) isdigit(val[0]))
			gDataPortMode = atoi(val);
		else
			gDataPortMode = kSendPortMode;
		gConn.dataPortMode = gDataPortMode;
	}
}	/* SetPassive */



#ifdef ncftp
void
SetProgressMeter(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		if (gProgressMeter == PrStatBar) {
			(void) fprintf(fp, "%s", "2 (statbar)");
		} else if (gProgressMeter == PrPhilBar) {
			(void) fprintf(fp, "%s", "1 (philbar)");
		} else {
			(void) fprintf(fp, "%s", "0 (simple)");
		}
	} else {
		if ((val[0] == '0') || (ISTRNEQ(val, "simple", 6)))
			gProgressMeter = PrSizeAndRateMeter;
		else if ((val[0] == '1') || (ISTRNEQ(val, "phil", 4)))
			gProgressMeter = PrPhilBar;
		else
			gProgressMeter = PrStatBar;
		gConn.progress = gProgressMeter;
	}
}	/* SetProgressMeter */
#else
void
SetProgressMeter(int UNUSED(t), const char *const UNUSED(val), FILE *const UNUSED(fp))
{
	LIBNCFTP_USE_VAR(t);
	LIBNCFTP_USE_VAR(val);
	LIBNCFTP_USE_VAR(fp);
}	/* SetProgressMeter */
#endif



void
SetRedialDelay(int UNUSED(t), const char *const val, FILE *const fp)
{
	int i;

	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gRedialDelay);
	} else {
		i = atoi(val);
		if (i < 10)
			i = 10;
		gRedialDelay = atoi(val);
	}
}	/* SetRedialDelay */



void
SetSavePasswords(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		if (gSavePasswords < 0)
			(void) fprintf(fp, "%s", "ask");
		else
			(void) fprintf(fp, "%s", YESNO(gSavePasswords));
	} else {
		if (ISTREQ(val, "ask"))
			gSavePasswords = -1;
		else
			gSavePasswords = StrToBool(val);
	}
}	/* SetSavePasswords */



void
SetSOBufsize(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gSOBufsize);
		if (gSOBufsize <= 0)
			(void) fprintf(fp, "%s", " (use system default)");
	} else {
		gConn.dataSocketRBufSize = gConn.dataSocketSBufSize = gSOBufsize = atoi(val);
	}
}	/* SetSOBufsize */




void
SetXferTimeout(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%d", gXferTimeout);
	} else {
		gConn.xferTimeout = gXferTimeout = atoi(val);
	}
}	/* SetXferTimeout */



void
SetXtTitle(int UNUSED(t), const char *const val, FILE *const fp)
{
	LIBNCFTP_USE_VAR(t);
	if (fp != NULL) {
		(void) fprintf(fp, "%s", YESNO(gMaySetXtermTitle));
	} else {
		gMaySetXtermTitle = StrToBool(val);
	}
}	/* SetXtTitle */




static void
Show1(int t)
{
	PrefOpt *p = &gPrefOpts[t];

	(void) printf("%-30s ", p->varname);
	if (p->proc != (PrefProc) 0)
		(*p->proc)(t, NULL, stdout);
	(void) printf("\n");
}	/* Show1 */




/* Modify or display the program's configuration. */
void
Set(const char *const tok1, const char *const tok2)
{
	int t;

	if ((tok1 == NULL) || (ISTREQ(tok1, "all"))) {
		/* Show all. */
		for (t=0; t<gNumPrefOpts; t++) {
			if (gPrefOpts[t].visible == kPrefOptVisible)
				Show1(t);
		}
	} else if (tok2 == NULL) {
		/* Show one. */
		for (t=0; t<gNumPrefOpts; t++) {
			if (ISTREQ(tok1, gPrefOpts[t].varname)) {
				if (gPrefOpts[t].visible == kPrefOptObselete) {
					(void) printf("The \"%s\" option is obselete or not implemented.\n", tok1);
				} else {
					Show1(t);
				}
				break;
			}
		}
		if (t >= gNumPrefOpts) {
			(void) printf("Unknown option \"%s\" -- try \"show all\" to list available options.\n", tok1);
		}
	} else {
		/* Set one. */
		for (t=0; t<gNumPrefOpts; t++) {
			if (ISTREQ(tok1, gPrefOpts[t].varname)) {
				if (gPrefOpts[t].visible == kPrefOptObselete) {
					(void) printf("The \"%s\" option is obselete or not implemented.\n", tok1);
				} else if (gPrefOpts[t].proc != (PrefProc) 0) {
					(*gPrefOpts[t].proc)(t, tok2, NULL);
					gPrefsDirty++;
				}
				break;
			}
		}
		if (t >= gNumPrefOpts) {
			(void) printf("Unknown option \"%s\" -- try \"show all\" to list available options.\n", tok1);
		}
	}
}	/* Set */



int
HasSeenOneTimeMessage(const char *const msg)
{
	char buf[256];
	char *a, *b;
	
	memcpy(buf, gOneTimeMessagesSeen, sizeof(buf));
	for (a = buf; (b = strtok(a, ",\n")) != NULL; a = NULL) {
		if (strcmp(msg, b) == 0)
			return (1);
	}
	return (0);
}	/* HasSeenOneTimeMessage */




void
SetSeenOneTimeMessage(const char *const msg)
{
	gPrefsDirty++;
	if (gOneTimeMessagesSeen[0] == '\0')
		STRNCPY(gOneTimeMessagesSeen, msg);
	else {
		STRNCAT(gOneTimeMessagesSeen, ",");
		STRNCAT(gOneTimeMessagesSeen, msg);
	}	
}	/* SetSeenOneTimeMessage */



int
OneTimeMessage(const char *const msg)
{
	if (HasSeenOneTimeMessage(msg))
		return (0);
	SetSeenOneTimeMessage(msg);
	return (1);
}	/* OneTimeMessage */




void
ProcessPrefsFile(FILE *const fp)
{
	char line[1024];
	char *tok1, *tok2;
	int t;

	line[sizeof(line) - 1] = '\0';
	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		tok1 = strtok(line, " =\t\r\n");
		if ((tok1 == NULL) || (tok1[0] == '#'))
			continue;
		tok2 = strtok(NULL, "\r\n");
		if (tok2 == NULL)
			continue;

		for (t=0; t<gNumPrefOpts; t++) {
			if (ISTREQ(tok1, gPrefOpts[t].varname)) {
				if (gPrefOpts[t].visible == kPrefOptObselete) {
					/* Probably converting an
					 * old 2.4.2 file.
					 */
					gPrefsDirty++;
				} else if (gPrefOpts[t].proc != (PrefProc) 0) {
					(*gPrefOpts[t].proc)(t, tok2, NULL);
				}
			}
		}
	}
}	/* ProcessPrefsFile */




/* Read the saved configuration settings from a preferences file. */
void
LoadPrefs(void)
{
	FILE *fp;
	char pathName[256];

	/* As with the firewall preference file, there can be
	 * site-wide preferences and user-specific preferences.
	 * The user pref file is of course kept in the user's
	 * NcFTP home directory.
	 *
	 * The way we do this is we first look for a global
	 * preferences file.  We then process the user's pref
	 * file, which could override the global prefs.  Finally,
	 * we open a "global fixed" prefs file which then
	 * overrides anything the user may have done.
	 */

	fp = fopen(kGlobalPrefFileName, FOPEN_READ_TEXT);
	if (fp != NULL) {
		/* Opened the global (but user-overridable) prefs file. */
		ProcessPrefsFile(fp);
		(void) fclose(fp);
	}

	if (gOurDirectoryPath[0] != '\0') {
		(void) OurDirectoryPath(pathName, sizeof(pathName), kPrefFileName);

		fp = fopen(pathName, FOPEN_READ_TEXT);
		if (fp == NULL) {
			/* Try loading the version 2 prefs.
			 * There will be options we no longer recognize, but
			 * we'd like to import the prefs when possible.
			 */
			gPrefsDirty++;
			(void) OurDirectoryPath(pathName, sizeof(pathName), kPrefFileNameV2);
			fp = fopen(pathName, FOPEN_READ_TEXT);
		}

		if (fp == NULL) {
			/* Write a new one when we're done. */
			gPrefsDirty++;
		} else {
			/* Opened the preferences file. */
			ProcessPrefsFile(fp);
			(void) fclose(fp);
		}
	}

	fp = fopen(kGlobalFixedPrefFileName, FOPEN_READ_TEXT);
	if (fp != NULL) {
		/* Opened the global (and not overridable) prefs file. */
		ProcessPrefsFile(fp);
		(void) fclose(fp);
	}
}	/* LoadPrefs */




/* Initialize the configuration settings, in case the user does not set them. */
void
InitPrefs(void)
{
	char *tok1;

	/* Set default values. */
	gPager[0] = '\0';
	memset(gOneTimeMessagesSeen, 0, sizeof(gOneTimeMessagesSeen));
	gXferTimeout = 3600;
	gConnTimeout = 20;
	gCtrlTimeout = 135;
	gDataPortMode = kFallBackToSendPortMode;
	gConn.dataPortMode = gDataPortMode;
	gAutoResume = 0;
	gSOBufsize = 0;
	gMaxLogSize = 10240;
	gConfirmClose = 1;
	gAutoSaveChangesToExistingBookmarks = 0;
	gRedialDelay = kDefaultRedialDelay;
	STRNCPY(gAutoAscii, "|.txt|.asc|.html|.htm|.css|.xml|.ini|.sh|.pl|.hqx|.cfg|.c|.h|.cpp|.hpp|.bat|.m3u|.pls|");

	/* PLEASE do not change the default from 0, and please
	 * don't hack out the portion in main.c which displays
	 * a plug every 7th time you run the program.  This is
	 * not much to ask for all the work I've put into this
	 * since 1991.
	 */
	gDoNotDisplayAds = 0;

#if (defined(WIN32) || defined(_WINDOWS)) && defined(_CONSOLE)
	gMaySetXtermTitle = 1;
#else
	gMaySetXtermTitle = 0;
#endif

	gSavePasswords = -1;
#ifdef ncftp
	gProgressMeter = PrStatBar;
#endif

	tok1 = getenv("PAGER");
	if ((tok1 != NULL) && (tok1[0] != '\0')) {
#ifdef HAVE_STRSTR
		/* I prefer "less", but it doesn't work well here
		 * because it clears the screen after it finishes,
		 * and the default at EOF is to stay in less
		 * instead of exiting.
		 */
		if (strstr(gPager, "less") != NULL)
			(void) STRNCPY(gPager, "more");
		else
			(void) STRNCPY(gPager, tok1);
#else
		(void) STRNCPY(gPager, tok1);
#endif
	} else {
		(void) STRNCPY(gPager, "more");
	}
}	/* InitPrefs */




/* After reading the preferences, do some additional initialization. */
void
PostInitPrefs(void)
{
	if (gLib.defaultAnonPassword[0] == '\0') {
		FTPInitializeAnonPassword(&gLib);
		gPrefsDirty++;
	}
	if (gFwDataPortMode >= 0)
		gConn.dataPortMode = gFwDataPortMode;
}	/* PostInitPrefs */




/* Write the configuration settings to a preferences file. */
void
SavePrefs(void)
{
	char pathName[256];
	char pathName2[256];
	char tName[32];
	int t;
	FILE *fp;

	if (gPrefsDirty == 0)
		return;		/* Don't need to save -- no changes made. */

	(void) OurDirectoryPath(pathName, sizeof(pathName), kPrefFileName);

	(void) sprintf(tName, "tpref%06u.txt", (unsigned int) getpid());
	(void) OurDirectoryPath(pathName2, sizeof(pathName2), tName);

	fp = fopen(pathName2, FOPEN_WRITE_TEXT);
	if (fp == NULL) {
		perror("could not save preferences file");
	} else {
		(void) fprintf(fp, "%s", "# NcFTP 3 preferences file\n# This file is loaded and overwritten each time NcFTP is run.\n#\n");
		for (t=0; t<gNumPrefOpts; t++) {
			if (gPrefOpts[t].visible != kPrefOptObselete) {
				(void) fprintf(fp, "%s=", gPrefOpts[t].varname);
				(*gPrefOpts[t].proc)(t, NULL, fp);
				(void) fprintf(fp, "\n");
			}
		}
		(void) fclose(fp);
		(void) unlink(pathName);
		if (rename(pathName2, pathName) < 0) {
			perror("could not finish saving preferences file");
			(void) unlink(pathName2);
		};
	}
}	/* SavePrefs */



/* This maintains the little counter file that is used by version 3.0
 * to do things based on how many times the program was run.
 */
void
CheckForNewV3User(void)
{
	FILE *fp;
	struct stat st;
	char pathName[256];
	char line[256];

	gNumProgramRuns = 0;

	/* Don't create in root directory. */
	if (gOurDirectoryPath[0] != '\0') {
		(void) OurDirectoryPath(pathName, sizeof(pathName), kFirstFileName);

		if ((stat(pathName, &st) < 0) && (errno == ENOENT)) {
			gNumProgramRuns = 1;
			gPrefsDirty++;

			/* Create a blank one. */
			fp = fopen(pathName, FOPEN_WRITE_TEXT);
			if (fp == NULL)
				return;
			(void) fprintf(fp, "# NcFTP uses this file to mark that you have run it before, and that you do not\n# need any special first-time instructions or setup.\n#\nruns=%d\n", gNumProgramRuns);
			(void) fclose(fp);
		} else {
			fp = fopen(pathName, FOPEN_READ_TEXT);
			if (fp != NULL) {
				while (fgets(line, sizeof(line) - 1, fp) != NULL) {
					if (strncmp(line, "runs=", 5) == 0) {
						(void) sscanf(line + 5, "%d",
							&gNumProgramRuns);
						break;
					}
				}
				(void) fclose(fp);
			}

			/* Increment the count of program runs. */
			gNumProgramRuns++;
			if (gNumProgramRuns == 1)
				gPrefsDirty++;

			/* Race condition between other ncftp processes.
			 * This isn't a big deal because this counter isn't
			 * critical.
			 */

			fp = fopen(pathName, FOPEN_WRITE_TEXT);
			if (fp != NULL) {
				(void) fprintf(fp, "# NcFTP uses this file to mark that you have run it before, and that you do not\n# need any special first-time instructions or setup.\n#\nruns=%d\n", gNumProgramRuns);
				(void) fclose(fp);
			}
		}
	}
}	/* CheckForNewV3User */
