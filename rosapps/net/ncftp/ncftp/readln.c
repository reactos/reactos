/* rdline.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 *
 * Note: It should still be simple to backport the old GNU Readline
 * support in here.  Feel free to do that if you hate NcFTP's built-in
 * implementation.
 * 
 */

#include "syshdrs.h"

#include "shell.h"
#include "util.h"
#include "bookmark.h"
#include "cmds.h"
#include "pref.h"
#include "ls.h"
#include "readln.h"
#include "getline.h"

const char *tcap_normal = "";
const char *tcap_boldface = "";
const char *tcap_underline = "";
const char *tcap_reverse = "";
const char *gTerm;
int gXterm;
int gXtermTitle;	/* Idea by forsberg@lysator.liu.se */
char gCurXtermTitleStr[256];

#if (defined(WIN32) || defined(_WINDOWS)) && defined(_CONSOLE)
	char gSavedConsoleTitle[64];
#endif

extern int gEventNumber;
extern int gMaySetXtermTitle;
extern LsCacheItem gLsCache[kLsCacheSize];
extern FTPConnectionInfo gConn;
extern char gRemoteCWD[512];
extern char gOurDirectoryPath[];
extern char gVersion[];
extern int gNumBookmarks;
extern BookmarkPtr gBookmarkTable;
extern PrefOpt gPrefOpts[];
extern int gNumPrefOpts;
extern int gScreenColumns;
extern int gIsTTYr;
extern int gUid;




void
GetScreenColumns(void)
{
#if defined(WIN32) || defined(_WINDOWS)
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		gScreenColumns = (int) csbi.dwSize.X;
		if (gScreenColumns < 80)
			gScreenColumns = 80;
	}
#else	/* Unix */
#ifdef BINDIR
	char ncftpbookmarks[256];
	FILE *infp;
	vsigproc_t osigpipe;
	int columns;
#endif	/* BINDIR */
	char *cp;

	if ((cp = (char *) getenv("COLUMNS")) == NULL) {
		gScreenColumns = 80;
	} else {
		gScreenColumns = atoi(cp);
		return;
	}

#ifdef TIOCGWINSZ
	{
		struct winsize felix;

		memset(&felix, 0, sizeof(felix));
		if (ioctl(0, TIOCGWINSZ, &felix) == 0) {
			columns = felix.ws_col;
			if ((columns > 0) && (columns < GL_BUF_SIZE))
				gScreenColumns = columns;
			else
				gScreenColumns = 80;
			return;
		}
	}
#endif

#ifdef BINDIR
	/* Don't run things as root unless really necessary. */
	if (gUid == 0)
		return;

	/* This is a brutal hack where we've hacked a
	 * special command line option into ncftp_bookmarks
	 * (which is linked with curses) so that it computes
	 * the screen size and prints it to stdout.
	 *
	 * This function runs ncftp_bookmarks and gets
	 * that information.  The reason we do this is that
	 * we may or may not have a sane installation of
	 * curses/termcap, and we don't want to increase
	 * NcFTP's complexity by the curses junk just to
	 * get the screen size.  Instead, we delegate this
	 * to ncftp_bookmarks which already deals with the
	 * ugliness of curses.
	 */

	STRNCPY(ncftpbookmarks, BINDIR);
	STRNCAT(ncftpbookmarks, "/");
	STRNCAT(ncftpbookmarks, "ncftpbookmarks");

	if (access(ncftpbookmarks, X_OK) < 0)
		return;

	STRNCAT(ncftpbookmarks, " --dimensions-terse");

	osigpipe = NcSignal(SIGPIPE, SIG_IGN);
	infp = popen(ncftpbookmarks, "r");
	if (infp != NULL) {
		columns = 0;
		(void) fscanf(infp, "%d", &columns);
		while (getc(infp) != EOF) {}
		(void) pclose(infp);

		if ((columns > 0) && (columns < GL_BUF_SIZE))
			gScreenColumns = columns;
	}
	(void) NcSignal(SIGPIPE, (sigproc_t) osigpipe);
#endif	/* BINDIR */
#endif	/* Windows */
}	/* GetScreenColumns */



/* For a few selected terminal types, we'll print in boldface, etc.
 * This isn't too important, though.
 */
void
InitTermcap(void)
{
#if (defined(WIN32) || defined(_WINDOWS)) && defined(_CONSOLE)
	gXterm = gXtermTitle = 0;
	gCurXtermTitleStr[0] = '\0';

	tcap_normal = "";
	tcap_boldface = "";
	tcap_underline = "";
	tcap_reverse = "";

	gTerm = "MS-DOS Prompt";
	ZeroMemory(gSavedConsoleTitle, (DWORD) sizeof(gSavedConsoleTitle));
	GetConsoleTitle(gSavedConsoleTitle, (DWORD) sizeof(gSavedConsoleTitle) - 1);
	SetConsoleTitle("NcFTP");
	gXterm = gXtermTitle = 1;
#else
	const char *term;
	
	gXterm = gXtermTitle = 0;
	gCurXtermTitleStr[0] = '\0';

	if ((gTerm = getenv("TERM")) == NULL) {
		tcap_normal = "";
		tcap_boldface = "";
		tcap_underline = "";
		tcap_reverse = "";
		return;
	}

	term = gTerm;
	if (	(strstr(term, "xterm") != NULL) ||
		(strstr(term, "rxvt") != NULL) ||
		(strstr(term, "dtterm") != NULL) ||
		(ISTRCMP(term, "scoterm") == 0)
	) {
		gXterm = gXtermTitle = 1;
	}

	if (	(gXterm != 0) ||
		(strcmp(term, "vt100") == 0) ||
		(strcmp(term, "linux") == 0) ||
		(strcmp(term, "vt220") == 0) ||
		(strcmp(term, "vt102") == 0)
	) {
		tcap_normal = "\033[0m";       /* Default ANSI escapes */
		tcap_boldface = "\033[1m";
		tcap_underline = "\033[4m";
		tcap_reverse = "\033[7m";
	} else {
		tcap_normal = "";
		tcap_boldface = "";
		tcap_underline = "";
		tcap_reverse = "";
	}
#endif
}	/* InitTermcap */




static char *
FindStartOfCurrentCommand(void)
{
	char *scp;
	char *start;
	int qc;

	for (scp = gl_buf;;) {
		start = scp;
		for (;;) {
			if (*scp == '\0')
				goto done;
			if (!isspace((int) *scp))
				break;
			scp++;
		}
		start = scp;

		for (;;) {
			if (*scp == '\0') {
				goto done;
			} else if ((*scp == '"') || (*scp == '\'')) {
				qc = *scp++;

				for (;;) {
					if (*scp == '\0') {
						goto done;
					} else if (*scp == '\\') {
						scp++;
						if (*scp == '\0')
							goto done;
						scp++;
					} else if (*scp == qc) {
						scp++;
						break;
					} else {
						scp++;
					}
				}
			} else if (*scp == '\\') {
				scp++;
				if (*scp == '\0')
					goto done;
				scp++;
			} else if ((*scp == ';') || (*scp == '\n')) {
				/* command ended */
				scp++;
				if (*scp == '\0')
					goto done;
				break;
			} else {
				scp++;
			}
		}
	}
done:
	return (start);
}	/* FindStartOfCurrentCommand */



static FileInfoListPtr
GetLsCacheFileList(const char *const item)
{
	int ci;
	int sortBy;
	int sortOrder;
	FileInfoListPtr filp;

	ci = LsCacheLookup(item);
	if (ci < 0) {
		/* This dir was not in the
		 * cache -- go get it.
		 */
		Ls(item, 'l', "", NULL);
		ci = LsCacheLookup(item);
		if (ci < 0)
			return NULL;
	}

	sortBy = 'n';		/* Sort by filename. */
	sortOrder = 'a';	/* Sort in ascending order. */
	filp = &gLsCache[ci].fil;
	SortFileInfoList(filp, sortBy, sortOrder);
	return filp;
}	/* GetLsCacheFileList */




static char *
RemoteCompletionFunction(const char *text, int state, int fTypeFilter)
{
	char rpath[256];
	char *cp;
	char *cp2;
	const char *textbasename;
	int fType;
	FileInfoPtr diritemp;
	FileInfoListPtr filp;
	int textdirlen;
	size_t tbnlen;
	size_t flen, mlen;
	static FileInfoVec diritemv;
	static int i;

	textbasename = strrchr(text, '/');
	if (textbasename == NULL) {
		textbasename = text;
		textdirlen = -1;
	} else {
		textdirlen = (int) (textbasename - text);
		textbasename++;
	}
	tbnlen = strlen(textbasename);

	if (state == 0) {
		if (text[0] == '\0') {
			/* Special case when they do "get <TAB><TAB> " */
			STRNCPY(rpath, gRemoteCWD);
		} else {
			PathCat(rpath, sizeof(rpath), gRemoteCWD, text);
			if (text[strlen(text) - 1] == '/') {
				/* Special case when they do "get /dir1/dir2/<TAB><TAB>" */
				STRNCAT(rpath, "/");
			}
			cp2 = strrchr(rpath, '/');
			if (cp2 == NULL) {
				return NULL;
			} else if (cp2 == rpath) {
				/* Item in root directory. */
				cp2++;
			}
			*cp2 = '\0';
		}

		filp = GetLsCacheFileList(rpath);
		if (filp == NULL)
			return NULL;

		diritemv = filp->vec;
		if (diritemv == NULL)
			return NULL;

		i = 0;
	}

	for ( ; ; ) {
		diritemp = diritemv[i];
		if (diritemp == NULL)
			break;

		i++;
		fType = (int) diritemp->type;
		if ((fTypeFilter == 0) || (fType == fTypeFilter) || (fType == /* symlink */ 'l')) {
			if (strncmp(textbasename, diritemp->relname, tbnlen) == 0) {
				flen = strlen(diritemp->relname);
				if (textdirlen < 0) {
					mlen = flen + 2;
					cp = (char *) malloc(mlen);
					if (cp == NULL)
						return (NULL);
					(void) memcpy(cp, diritemp->relname, mlen);
				} else {
					mlen = textdirlen + 1 + flen + 2;
					cp = (char *) malloc(mlen);
					if (cp == NULL)
						return (NULL);
					(void) memcpy(cp, text, (size_t) textdirlen);
					cp[textdirlen] = '/';
					(void) strcpy(cp + textdirlen + 1, diritemp->relname);
				}
				if (fType == 'd') {
					gl_completion_exact_match_extra_char = '/';
				} else {
					gl_completion_exact_match_extra_char = ' ';
				}
				return cp;
			}
		}
	}
	return NULL;
}	/* RemoteCompletionFunction */




static char *
RemoteFileCompletionFunction(const char *text, int state)
{
	char *cp;

	cp = RemoteCompletionFunction(text, state, 0);
	return cp;
}	/* RemoteFileCompletionFunction */




static char *
RemoteDirCompletionFunction(const char *text, int state)
{
	char *cp;

	cp = RemoteCompletionFunction(text, state, 'd');
	return cp;
}	/* RemoteDirCompletionFunction */




static char *
BookmarkCompletionFunction(const char *text, int state)
{
	char *cp;
	size_t textlen;
	int i, matches;

	if ((gBookmarkTable == NULL) || (state >= gNumBookmarks))
		return (NULL);

	textlen = strlen(text);
	if (textlen == 0) {
		cp = StrDup(gBookmarkTable[state].bookmarkName);
	} else {
		cp = NULL;
		for (i=0, matches=0; i<gNumBookmarks; i++) {
			if (ISTRNCMP(gBookmarkTable[i].bookmarkName, text, textlen) == 0) {
				if (matches >= state) {
					cp = StrDup(gBookmarkTable[i].bookmarkName);
					break;
				}
				matches++;
			}
		}
	}
	return cp;
}	/* BookmarkCompletionFunction */




static char *
CommandCompletionFunction(const char *text, int state)
{
	char *cp;
	size_t textlen;
	int i, matches;
	CommandPtr cmdp;

	textlen = strlen(text);
	if (textlen == 0) {
		cp = NULL;
	} else {
		cp = NULL;
		for (i=0, matches=0; ; i++) {
			cmdp = GetCommandByIndex(i);
			if (cmdp == kNoCommand)
				break;
			if (ISTRNCMP(cmdp->name, text, textlen) == 0) {
				if (matches >= state) {
					cp = StrDup(cmdp->name);
					break;
				}
				matches++;
			}
		}
	}
	return cp;
}	/* CommandCompletionFunction */




static char *
PrefOptCompletionFunction(const char *text, int state)
{
	char *cp;
	size_t textlen;
	int i, matches;

	if (state >= gNumPrefOpts)
		return (NULL);

	textlen = strlen(text);
	if (textlen == 0) {
		cp = StrDup(gPrefOpts[state].varname);
	} else {
		cp = NULL;
		for (i=0, matches=0; i<gNumPrefOpts; i++) {
			if (ISTRNCMP(gPrefOpts[i].varname, text, textlen) == 0) {
				if (matches >= state) {
					cp = StrDup(gPrefOpts[i].varname);
					break;
				}
				matches++;
			}
		}
	}
	return cp;
}	/* PrefOptCompletionFunction */




void
ReCacheBookmarks(void)
{
	(void) LoadBookmarkTable();
}



static int
HaveCommandNameOnly(char *cmdstart)
{
	char *cp;
	for (cp = cmdstart; *cp != '\0'; cp++) {
		if (isspace((int) *cp))
			return (0);	/* At least one argument in progress. */
	}
	return (1);
}	/* HaveCommandNameOnly */




static char *
CompletionFunction(const char *text, int state)
{
	char *cp;
	char *cmdstart;
	ArgvInfo ai;
	int bUsed;
	CommandPtr cmdp;
	static int flags;

	if (state == 0) {
		flags = -1;
		cmdstart = FindStartOfCurrentCommand();
		if (cmdstart == NULL)
			return NULL;
		if (HaveCommandNameOnly(cmdstart)) {
			flags = -2;	/* special case */
			cp = CommandCompletionFunction(text, state);
			return cp;
		}

		(void) memset(&ai, 0, sizeof(ai));
		bUsed = MakeArgv(cmdstart, &ai.cargc, ai.cargv,
			(int) (sizeof(ai.cargv) / sizeof(char *)),
			ai.argbuf, sizeof(ai.argbuf),
			ai.noglobargv, 1);
		if (bUsed <= 0)
			return NULL;
		if (ai.cargc == 0)
			return NULL;

		cmdp = GetCommandByName(ai.cargv[0], 0);
		if (cmdp == kAmbiguousCommand) {
			return NULL;
		} else if (cmdp == kNoCommand) {
			return NULL;
		}
		flags = cmdp->flags;
	}
	if (flags == (-2)) {
		cp = CommandCompletionFunction(text, state);
		return cp;
	}
	if (flags < 0)
		return NULL;
	if ((flags & (kCompleteLocalFile|kCompleteLocalDir)) != 0) {
		cp = gl_local_filename_completion_proc(text, state);
		return cp;
	} else if ((flags & kCompleteRemoteFile) != 0) {
		gl_filename_quoting_desired = 1;
		cp = RemoteFileCompletionFunction(text, state);
		return cp;
	} else if ((flags & kCompleteRemoteDir) != 0) {
		gl_filename_quoting_desired = 1;
		cp = RemoteDirCompletionFunction(text, state);
		return cp;
	} else if ((flags & kCompleteBookmark) != 0) {
		cp = BookmarkCompletionFunction(text, state);
		return cp;
	} else if ((flags & kCompletePrefOpt) != 0) {
		cp = PrefOptCompletionFunction(text, state);
		return cp;
	}	
	return NULL;
}	/* CompletionFunction */




void
LoadHistory(void)
{
	char pathName[256];

	if (gOurDirectoryPath[0] == '\0')
		return;
	(void) OurDirectoryPath(pathName, sizeof(pathName), kHistoryFileName);

	gl_histloadfile(pathName);
}	/* LoadHistory */



static size_t
Vt100VisibleStrlen(const char *src)
{
	const char *cp;
	size_t esc;

	for (esc = 0, cp = src; *cp != '\0'; cp++) {
		if (*cp == '\033')
			esc++;
	}

	/* The VT100 escape codes we use are all in the form "\033[7m"
	 * These aren't visible, so subtract them from the count.
	 */
	return ((size_t) (cp - src) - (esc * 4));
}	/* Vt100VisibleStrlen */



/* Save the commands they typed in a history file, then they can use
 * readline to go through them again next time.
 */
void
SaveHistory(void)
{
	char pathName[256];

	if (gOurDirectoryPath[0] == '\0')
		return;		/* Don't create in root directory. */
	(void) OurDirectoryPath(pathName, sizeof(pathName), kHistoryFileName);

	gl_strlen = Vt100VisibleStrlen;
	gl_histsavefile(pathName);
	(void) chmod(pathName, 00600);
}	/* SaveHistory */




void
InitReadline(void)
{
	gl_completion_proc = CompletionFunction;
	gl_setwidth(gScreenColumns);
	LoadHistory();
	ReCacheBookmarks();
}	/* InitReadline */




char *
Readline(char *prompt)
{
	char *linecopy, *line, *cp;
	char lbuf[256];

	if (gIsTTYr) {
		line = getline(prompt);
	} else {
		line = fgets(lbuf, sizeof(lbuf) - 1, stdin);
		if (line != NULL) {
			cp = line + strlen(line) - 1;
			if (*cp == '\n')
				*cp = '\0';
		}
	}

	if (line != NULL) {
		if (line[0] == '\0')
			return NULL;	/* EOF */
		linecopy = StrDup(line);
		line = linecopy;
	}
	return (line);
}	/* Readline */



void
AddHistory(char *line)
{
	gl_histadd(line);
}	/* AddHistory */



void
DisposeReadline(void)
{
	SaveHistory();
}	/* DisposeReadline */





/*VARARGS*/
void
SetXtermTitle(const char *const fmt, ...)
{
	va_list ap;
	char buf[256];

	if ((gXtermTitle != 0) && (gMaySetXtermTitle != 0)) {
		if ((fmt == NULL) || (ISTRCMP(fmt, "RESTORE") == 0)) {
#if (defined(WIN32) || defined(_WINDOWS)) && defined(_CONSOLE)
			STRNCPY(buf, gSavedConsoleTitle);
#else
			STRNCPY(buf, gTerm);
#endif
		} else if (ISTRCMP(fmt, "DEFAULT") == 0) {
			(void) Strncpy(buf, gVersion + 5, 12);
		} else {
			va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
			(void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
			buf[sizeof(buf) - 1] = '\0';
#else
			(void) vsprintf(buf, fmt, ap);
#endif
			va_end(ap);
		}
		if (buf[0] != '\0') {
#if (defined(WIN32) || defined(_WINDOWS)) && defined(_CONSOLE)
			SetConsoleTitle(buf);
#else
			if (strcmp(gCurXtermTitleStr, buf) != 0) {
				fprintf(stderr, "\033]0;%s\007", buf);
				STRNCPY(gCurXtermTitleStr, buf);
			}
#endif
		}
	}
}	/* SetXtermTitle */




void
PrintStartupBanner(void)
{
	char v[80], *cp;
	char vdate[32];

	/* Print selected information from the version ID. */
	vdate[0] = '\0';
	(void) STRNCPY(v, gVersion + 5);
	cp = strchr(v, ',');
	if (cp != NULL) {
		*cp = '\0';
		cp[-5] = '\0';
		(void) STRNCPY(vdate, " (");
		(void) STRNCAT(vdate, v + 16);
		(void) STRNCAT(vdate, ", ");
		(void) STRNCAT(vdate, cp - 4);
		(void) STRNCAT(vdate, ")");
	}

#if defined(BETA) && (BETA > 0)
	(void) fprintf(stdout, "%s%.11s beta %d%s%s by Mike Gleason (ncftp@ncftp.com).\n",
		tcap_boldface,
		gVersion + 5,
		BETA,
		tcap_normal,
		vdate
	);
#else
	(void) fprintf(stdout, "%s%.11s%s%s by Mike Gleason (ncftp@ncftp.com).\n",
		tcap_boldface,
		gVersion + 5,
		tcap_normal,
		vdate
	);
#endif
	(void) fflush(stdout);
}	/* PrintStartupBanner */




/* Print the command shell's prompt. */
void
MakePrompt(char *dst, size_t dsize)
{
	char acwd[64];

#	ifdef HAVE_SNPRINTF
	if (gConn.loggedIn != 0) {
		AbbrevStr(acwd, gRemoteCWD, 25, 0);
		snprintf(dst, dsize, "%sncftp%s %s %s>%s ",
			tcap_boldface, tcap_normal, acwd,
			tcap_boldface, tcap_normal);
	} else {
		snprintf(dst, dsize, "%sncftp%s> ",
			tcap_boldface, tcap_normal);
	}
#	else	/* HAVE_SNPRINTF */
	(void) Strncpy(dst, tcap_boldface, dsize);
	(void) Strncat(dst, "ncftp", dsize);
	(void) Strncat(dst, tcap_normal, dsize);
	if (gConn.loggedIn != 0) {
		AbbrevStr(acwd, gRemoteCWD, 25, 0);
		(void) Strncat(dst, " ", dsize);
		(void) Strncat(dst, acwd, dsize);
		(void) Strncat(dst, " ", dsize);
	}
	(void) Strncat(dst, tcap_boldface, dsize);
	(void) Strncat(dst, ">", dsize);
	(void) Strncat(dst, tcap_normal, dsize);
	(void) Strncat(dst, " ", dsize);
#	endif	/* HAVE_SNPRINTF */
}	/* MakePrompt */
