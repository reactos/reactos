/* main.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"
#include "ls.h"
#include "bookmark.h"
#include "shell.h"
#include "cmds.h"
#include "main.h"
#include "getopt.h"
#include "progress.h"
#include "pref.h"
#include "readln.h"
#include "trace.h"
#include "log.h"
#include "spool.h"
#include "util.h"

#if defined(WIN32) || defined(_WINDOWS)
	WSADATA wsaData;
	int wsaInit = 0;

	__inline void DisposeWinsock(int aUNUSED) { if (wsaInit > 0) WSACleanup(); wsaInit--; }
#else
#	define DisposeWinsock(a)
#endif

int gStartupUrlParameterGiven = 0;
int gIsTTY, gIsTTYr;
int gScreenColumns;

FTPLibraryInfo gLib;
FTPConnectionInfo gConn;
LineList gStartupURLCdList;
int gTransferTypeInitialized = 0;
int gTransferType;
int gURLMode = 0;
extern int gUnprocessedJobs;
char gLocalCWD[512], gPrevLocalCWD[512];

extern char *gOptArg;
extern int gOptInd;
extern char gRemoteCWD[512], gPrevRemoteCWD[512];
extern Bookmark gBm;
extern int gLoadedBm;
extern int gFirstTimeUser;
extern int gFirewallType;
extern char gAutoAscii[];
extern char gFirewallHost[64];
extern char gFirewallUser[32];
extern char gFirewallPass[32];
extern char gFirewallExceptionList[];
extern char gCopyright[], gVersion[];
extern unsigned int gFirewallPort;
extern int gConnTimeout, gXferTimeout, gCtrlTimeout;
extern int gDataPortMode, gRedialDelay;
extern int gDebug;
extern int gNumProgramRuns, gDoNotDisplayAds;
extern int gSOBufsize;
extern FTPProgressMeterProc gProgressMeter;

static void
Usage(void)
{
	FILE *fp;
#ifdef UNAME
	char s[80];
#endif

	fp = stderr;
	(void) fprintf(fp, "\nUsage:  ncftp [flags] [<host> | <directory URL to browse>]\n");
	(void) fprintf(fp, "\nFlags:\n\
  -u XX  Use username XX instead of anonymous.\n\
  -p XX  Use password XX with the username.\n\
  -P XX  Use port number XX instead of the default FTP service port (21).\n\
  -j XX  Use account XX with the username (rarely needed).\n\
  -F     Dump a sample $HOME/.ncftp/firewall prefs file to stdout and exit.\n");

	(void) fprintf(fp, "\nProgram version:  %s\nLibrary version:  %s\n", gVersion + 5, gLibNcFTPVersion + 5);
#ifdef UNAME
	AbbrevStr(s, UNAME, 60, 1);
	(void) fprintf(fp, "System:           %s\n", s);
#endif
	(void) fprintf(fp, "\nThis is a freeware program by Mike Gleason (ncftp@ncftp.com).\n");
	(void) fprintf(fp, "Use ncftpget and ncftpput for command-line FTP.\n\n");
	exit(2);
}	/* Usage */




static void
DumpFirewallPrefsTemplate(void)
{
	WriteDefaultFirewallPrefs(stdout);
}	/* DumpFirewallPrefsTemplate */




/* This resets our state information whenever we are ready to open a new
 * host.
 */
void
InitConnectionInfo(void)
{
	int result;

	result = FTPInitConnectionInfo(&gLib, &gConn, kDefaultFTPBufSize);
	if (result < 0) {
		(void) fprintf(stderr, "ncftp: init connection info error %d (%s).\n", result, FTPStrError(result));
		exit(1);
	}

	gConn.debugLog = NULL;
	gConn.errLog = stderr;
	SetDebug(gDebug);
	UseTrace();
	(void) STRNCPY(gConn.user, "anonymous");
	gConn.host[0] = '\0';
	gConn.progress = gProgressMeter;
	gTransferTypeInitialized = 0;
	gTransferType = kTypeBinary;
	gConn.leavePass = 1;		/* Don't let the lib zap it. */
	gConn.connTimeout = gConnTimeout;
	gConn.xferTimeout = gXferTimeout;
	gConn.ctrlTimeout = gCtrlTimeout;
	gConn.dataPortMode = gDataPortMode;
	gConn.maxDials = (-1);	/* Dial forever, until they hit ^C. */
	gUnprocessedJobs = 0;
	gPrevRemoteCWD[0] = '\0';
	gConn.dataSocketRBufSize = gConn.dataSocketSBufSize = gSOBufsize;
	if (gRedialDelay >= 10)
		gConn.redialDelay = gRedialDelay;
	if ((gAutoAscii[0] == '\0') || (ISTREQ(gAutoAscii, "no")) || (ISTREQ(gAutoAscii, "off")) || (ISTREQ(gAutoAscii, "false"))) {
		gConn.asciiFilenameExtensions = NULL;
	} else {
		gConn.asciiFilenameExtensions = gAutoAscii;
	}
}	/* InitConnectionInfo */




/* This lets us do things with our state information just before the
 * host is closed.
 */
void
CloseHost(void)
{
	if (gConn.connected != 0) {
		if (gConn.loggedIn != 0) {
			SaveUnsavedBookmark();
		}
		RunBatchIfNeeded(&gConn);
	}
	gConn.ctrlTimeout = 3;
	(void) FTPCloseHost(&gConn);
}	/* CloseHost */




/* If the user specified a URL on the command-line, this initializes
 * our state information based upon it.
 */
static void
SetStartupURL(const char *const urlgiven)
{
	int rc;
	char url[256];
	char urlfile[128];

	gLoadedBm = 0;
	(void) STRNCPY(url, urlgiven);

	rc = DecodeDirectoryURL(&gConn, url, &gStartupURLCdList, urlfile, sizeof(urlfile));
	if (rc == kMalformedURL) {
		(void) fprintf(stderr, "Malformed URL: %s\n", url);
		exit(1);
	} else if (rc == kNotURL) {
		/* This is what should happen most of the time. */
		(void) STRNCPY(gConn.host, urlgiven);
		gURLMode = 2;
		if (GetBookmark(gConn.host, &gBm) >= 0) {
			gLoadedBm = 1;
			(void) STRNCPY(gConn.host, gBm.name);
			(void) STRNCPY(gConn.user, gBm.user);
			(void) STRNCPY(gConn.pass, gBm.pass);
			(void) STRNCPY(gConn.acct, gBm.acct);
			gConn.hasSIZE = gBm.hasSIZE;
			gConn.hasMDTM = gBm.hasMDTM;
			gConn.hasPASV = gBm.hasPASV;
			gConn.hasUTIME = gBm.hasUTIME;
			gConn.port = gBm.port;
		} else {
			SetBookmarkDefaults(&gBm);
		}

		if (MayUseFirewall(gConn.host, gFirewallType, gFirewallExceptionList) != 0) {
			gConn.firewallType = gFirewallType; 
			(void) STRNCPY(gConn.firewallHost, gFirewallHost);
			(void) STRNCPY(gConn.firewallUser, gFirewallUser);
			(void) STRNCPY(gConn.firewallPass, gFirewallPass);
			gConn.firewallPort = gFirewallPort;
		}
	} else {
		/* URL okay */
		if (urlfile[0] != '\0') {
			/* It was obviously not a directory */
			(void) fprintf(stderr, "Use ncftpget or ncftpput to handle file URLs.\n");
			exit(1);
		}
		gURLMode = 1;
		if (MayUseFirewall(gConn.host, gFirewallType, gFirewallExceptionList) != 0) {
			gConn.firewallType = gFirewallType; 
			(void) STRNCPY(gConn.firewallHost, gFirewallHost);
			(void) STRNCPY(gConn.firewallUser, gFirewallUser);
			(void) STRNCPY(gConn.firewallPass, gFirewallPass);
			gConn.firewallPort = gFirewallPort;
		}
	}
}	/* SetStartupURL */




static void
OpenURL(void)
{
	LinePtr lp;
	int result;

	if (gURLMode == 1) {
		SetBookmarkDefaults(&gBm);
		if (DoOpen() >= 0) {
			for (lp = gStartupURLCdList.first; lp != NULL; lp = lp->next) {
				result = FTPChdir(&gConn, lp->line);
				if (result != kNoErr) {
					FTPPerror(&gConn, result, kErrCWDFailed, "Could not chdir to", lp->line);
					break;
				}
			}
			result = FTPGetCWD(&gConn, gRemoteCWD, sizeof(gRemoteCWD));
			if (result != kNoErr) {
				FTPPerror(&gConn, result, kErrPWDFailed, NULL, NULL);
			} else {
				(void) printf("Current remote directory is %s.\n", gRemoteCWD);
			}
		}
	} else if (gURLMode == 2) {
		(void) DoOpen();
	}
}	/* OpenURL */




/* These things are done first, before we even parse the command-line
 * options.
 */
static void
PreInit(void)
{
	int result;

#if defined(WIN32) || defined(_WINDOWS)
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		fprintf(stderr, "could not initialize winsock\n");
		exit(1);
	}
	wsaInit++;
#endif

#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "");
#endif
#if defined(WIN32) || defined(_WINDOWS)
	gIsTTY = 1;
	gIsTTYr = 1;
#else
	gIsTTY = ((isatty(2) != 0) && (getppid() > 1)) ? 1 : 0;
	gIsTTYr = ((isatty(0) != 0) && (getppid() > 1)) ? 1 : 0;
#endif
#ifdef SIGPOLL
	(void) NcSignal(SIGPOLL, (FTPSigProc) SIG_IGN);
#endif
	InitUserInfo();
	result = FTPInitLibrary(&gLib);
	if (result < 0) {
		(void) fprintf(stderr, "ncftp: init library error %d (%s).\n", result, FTPStrError(result));
		exit(1);
	}
#if defined(WIN32) || defined(_WINDOWS)
	srand((unsigned int) (GetTickCount() & 0x7FFF));
#else
	srand((unsigned int) getpid());
#endif
	InitLineList(&gStartupURLCdList);
	CheckForNewV3User();
	InitLog();
	InitPrefs();
	LoadFirewallPrefs(0);
	LoadPrefs();
	InitConnectionInfo();
	InitCommandList();
	InitLs();
	TruncBatchLog();
	GetoptReset();
	GetScreenColumns();
}	/* PreInit */





/* These things are done at startup, but after we parse the command-line
 * options.
 */
static void
PostInit(void)
{
	PostInitPrefs();
	OpenTrace();
	InitTermcap();
	InitReadline();
	(void) FTPGetLocalCWD(gLocalCWD, sizeof(gLocalCWD));
	gPrevLocalCWD[0] = '\0';
	PrintStartupBanner();
	if (gNumProgramRuns <= 1)
		(void) printf("\n%s\n", gCopyright + 5);

	Trace(0, "Fw: %s  Type: %d  User: %s  Pass: %s  Port: %u\n", 
		gFirewallHost,
		gFirewallType,
		gFirewallUser,
		(gFirewallPass[0] == '\0') ? "(none)" : "********",
		gFirewallPort
	);
	Trace(0, "FwExceptions: %s\n", gFirewallExceptionList);
	if (strchr(gLib.ourHostName, '.') == NULL) {
		Trace(0, "NOTE:  Your domain name could not be detected.\n");
		if (gConn.firewallType != kFirewallNotInUse) {
			Trace(0, "       Make sure you manually add your domain name to firewall-exception-list.\n");
		}
	}
}	/* PostInit */




/* Try to get the user to evaluate my commercial offerings. */
static void
Plug(void)
{
#if defined(WIN32) || defined(_WINDOWS)
	/* NcFTPd hasn't been ported to Windows. */
#else
	if ((gDoNotDisplayAds == 0) && ((gNumProgramRuns % 7) == 2)) {
		(void) printf("\n\n\n\tThank you for using NcFTP Client.\n\tAsk your system administrator to try NcFTPd Server!\n\thttp://www.ncftpd.com\n\n\n\n");
	}
#endif
}	/* Plug */




/* These things are just before the program exits. */
static void
PostShell(void)
{
	SetXtermTitle("RESTORE");
	CloseHost();
	DisposeReadline();
	CloseTrace();
	SavePrefs();
	EndLog();
	Plug();
	DisposeWinsock(0);
}	/* PostShell */




int
main(int argc, const char **const argv)
{
	int c;
	int n;

	PreInit();
	while ((c = Getopt(argc, argv, "P:u:p:J:rd:g:FVLD")) > 0) switch(c) {
		case 'P':
		case 'u':
		case 'p':
		case 'J':
			gStartupUrlParameterGiven = 1;
			break;
		case 'r':
		case 'g':
		case 'd':
		case 'V':
		case 'L':
		case 'D':
		case 'F':
			break;
		default:
			Usage();
	}

	if (gOptInd < argc) {
		LoadFirewallPrefs(0);
		SetStartupURL(argv[gOptInd]);
	} else if (gStartupUrlParameterGiven != 0) {
		/* One of the flags they specified
		 * requires a URL or hostname to
		 * open.
		 */
		Usage();
	}

	GetoptReset();
	/* Allow command-line parameters to override
	 * bookmark settings.
	 */

	while ((c = Getopt(argc, argv, "P:u:p:j:J:rd:g:FVLD")) > 0) switch(c) {
		case 'P':
			gConn.port = atoi(gOptArg);	
			break;
		case 'u':
			(void) STRNCPY(gConn.user, gOptArg);
			break;
		case 'p':
			(void) STRNCPY(gConn.pass, gOptArg);	/* Don't recommend doing this! */
			break;
		case 'J':
		case 'j':
			(void) STRNCPY(gConn.acct, gOptArg);
			break;
		case 'r':
			/* redial is always on */
			break;
		case 'g':
			gConn.maxDials = atoi(gOptArg);
			break;
		case 'd':
			n = atoi(gOptArg);
			if (n >= 10)
				gConn.redialDelay = n;
			break;
		case 'F':
			DumpFirewallPrefsTemplate();
			exit(0);
			/*NOTREACHED*/
			break;
		case 'V':
			/*FALLTHROUGH*/
		case 'L':
			/*FALLTHROUGH*/
		case 'D':
			/* silently ignore these legacy options */
			break;
		default:
			Usage();
	}

	PostInit();
	OpenURL();
	CommandShell();
	PostShell();
	exit(0);
}	/* main */
