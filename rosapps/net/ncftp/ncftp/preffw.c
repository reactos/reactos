/* preffw.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "pref.h"
#include "util.h"

/* How many times they've run this program. */
int gNumProgramRuns = 0;

/* Firewall/proxy configuration parameters. */
int gFirewallType;
char gFirewallHost[64];
char gFirewallUser[32];
char gFirewallPass[32];
char gFirewallExceptionList[256];
unsigned int gFirewallPort;
int gFirewallPrefsLoaded = 0;

/* Active or passive FTP?  (PORT or PASV?)  Or both? */
int gDataPortMode;

/* Hack so the user/admin can set passive in the firewall
 * prefs file.
 */
int gFwDataPortMode = -1;

extern FTPLibraryInfo gLib;
extern char gOurDirectoryPath[], gUser[], gVersion[];


/* Save a sample configuration file for the firewall/proxy setup. */
void
WriteDefaultFirewallPrefs(FILE *fp)
{
	char *cp;
	time_t now;

	FTPInitializeOurHostName(&gLib);
	cp = strchr(gLib.ourHostName, '.');

	(void) fprintf(fp, "%s", "\
# NcFTP firewall preferences\n\
# ==========================\n\
#\n\
");

	(void) fprintf(fp, "%s", "\
# If you need to use a proxy for FTP, you can configure it below.\n\
# If you do not need one, leave the ``firewall-type'' variable set\n\
# to 0.  Any line that does not begin with the ``#'' character is\n\
# considered a configuration command line.\n\
");
	(void) fprintf(fp, "%s", "\
#\n\
# NOTE:  NcFTP does NOT support HTTP proxies that do FTP, such as \"squid\"\n\
#        or Netscape Proxy Server.  Why?  Because you have to communicate with\n\
#        them using HTTP, and this is a FTP only program.\n\
");
	(void) fprintf(fp, "%s", "\
#\n\
# Types of firewalls:\n\
# ------------------\n\
#\n\
#    type 1:  Connect to firewall host, but send \"USER user@real.host.name\"\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
#    type 2:  Connect to firewall, login with \"USER fwuser\" and\n\
#             \"PASS fwpassword\", and then \"USER user@real.host.name\"\n\
#\n\
#    type 3:  Connect to and login to firewall, and then use\n\
#             \"SITE real.host.name\", followed by the regular USER and PASS.\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
#    type 4:  Connect to and login to firewall, and then use\n\
#             \"OPEN real.host.name\", followed by the regular USER and PASS.\n\
#\n\
#    type 5:  Connect to firewall host, but send\n\
#             \"USER user@fwuser@real.host.name\" and\n\
#             \"PASS pass@fwpass\" to login.\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
#    type 6:  Connect to firewall host, but send\n\
#             \"USER fwuser@real.host.name\" and\n\
#             \"PASS fwpass\" followed by a regular\n\
#             \"USER user\" and\n\
#             \"PASS pass\" to complete the login.\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
#    type 7:  Connect to firewall host, but send\n\
#             \"USER user@real.host.name fwuser\" and\n\
#             \"PASS pass\" followed by\n\
#             \"ACCT fwpass\" to complete the login.\n\
#\n\
#    type 0:  Do NOT use a firewall (most users will choose this).\n\
#\n\
firewall-type=0\n\
#\n\
#\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
# The ``firewall-host'' variable should be the IP address or hostname of\n\
# your firewall server machine.\n\
#\n\
");

	if (cp == NULL) {
		(void) fprintf(fp, "firewall-host=firewall.domain.com\n");
	} else {
		(void) fprintf(fp, "firewall-host=firewall%s\n", cp);
	}

	(void) fprintf(fp, "%s", "\
#\n\
#\n\
#\n\
# The ``firewall-user'' variable tells NcFTP what to use as the user ID\n\
# when it logs in to the firewall before connecting to the outside world.\n\
#\n\
");
	(void) fprintf(fp, "firewall-user=%s\n", gUser);
	(void) fprintf(fp, "%s", "\
#\n\
#\n\
#\n\
# The ``firewall-password'' variable is the password associated with\n\
# the firewall-user ID.  If you set this here, be sure to change the\n\
# permissions on this file so that no one (except the superuser) can\n\
# see your password.  You may also leave this commented out, and then\n\
# NcFTP will prompt you each time for the password.\n\
");
	(void) fprintf(fp, "%s", "\
#\n\
firewall-password=fwpass\n\
#\n\
#\n\
#\n\
# Your firewall may require you to connect to a non-standard port for\n\
# outside FTP services, instead of the internet standard port number (21).\n\
#\n\
firewall-port=21\n\
");
	(void) fprintf(fp, "%s", "\
#\n\
#\n\
#\n\
# You probably do not want to FTP to the firewall for hosts on your own\n\
# domain.  You can set ``firewall-exception-list'' to a list of domains\n\
# or hosts where the firewall should not be used.  For example, if your\n\
# domain was ``probe.net'' you could set this to ``.probe.net''.\n\
#\n\
");
	(void) fprintf(fp, "%s", "\
# If you leave this commented out, the default behavior is to attempt to\n\
# lookup the current domain, and exclude hosts for it.  Otherwise, set it\n\
# to a list of comma-delimited domains or hostnames.  The special token\n\
# ``localdomain'' is used for unqualified hostnames, so if you want hosts\n\
# without explicit domain names to avoid the firewall, be sure to include\n\
# that in your list.\n\
#\n\
");

	if (cp != NULL) {
		(void) fprintf(fp, "firewall-exception-list=%s,localhost,localdomain\n", cp);
	} else {
		(void) fprintf(fp, "firewall-exception-list=.probe.net,localhost,foo.bar.com,localdomain\n");
	}

	(void) fprintf(fp, "%s", "\
#\n\
#\n\
#\n\
# You may also specify passive mode here.  Normally this is set in the\n\
# regular $HOME/.ncftp/prefs file.  This must be set to one of\n\
# \"on\", \"off\", or \"optional\", which mean always use PASV,\n\
# always use PORT, and try PASV then PORT, respectively.\n\
#\n\
#passive=on\n");

	time(&now);
	(void) fprintf(fp, "\
#\n\
#\n\
#\n\
# NOTE:  This file was created for you on %s\
#        by NcFTP %.5s.  Removing this file will cause the next run of NcFTP\n\
#        to generate a new one, possibly with more configurable options.\n",
	ctime(&now),
	gVersion + 11);
	(void) fprintf(fp, "\
#\n\
# ALSO:  A %s file, if present, is processed before this file,\n\
#        and a %s file, if present, is processed after.\n",
		kGlobalFirewallPrefFileName,
		kGlobalFixedFirewallPrefFileName
	);
}	/* CreateDefaultFirewallPrefs */




void
ProcessFirewallPrefFile(FILE *fp)
{
	char line[256];
	char *tok1, *tok2;
	int n;

	/* Opened the firewall preferences file. */
	line[sizeof(line) - 1] = '\0';
	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		tok1 = strtok(line, " =\t\r\n");
		if ((tok1 == NULL) || (tok1[0] == '#'))
			continue;
		tok2 = strtok(NULL, "\r\n");
		if (tok2 == NULL)
			continue;
		if (ISTREQ(tok1, "firewall-type")) {
			n = atoi(tok2);
			if ((n > 0) && (n <= kFirewallLastType))
				gFirewallType = n;
		} else if (ISTREQ(tok1, "firewall-host")) {
			(void) STRNCPY(gFirewallHost, tok2);
		} else if (ISTREQ(tok1, "firewall-port")) {
			n = atoi(tok2);
			if (n > 0)
				gFirewallPort = (unsigned int) n;
		} else if (ISTREQ(tok1, "firewall-user")) {
			(void) STRNCPY(gFirewallUser, tok2);
		} else if (ISTREQ(tok1, "firewall-pass")) {
			(void) STRNCPY(gFirewallPass, tok2);
		} else if (ISTREQ(tok1, "firewall-password")) {
			(void) STRNCPY(gFirewallPass, tok2);
		} else if (ISTREQ(tok1, "firewall-exception-list")) {
			(void) STRNCPY(gFirewallExceptionList, tok2);
		} else if (ISTREQ(tok1, "passive")) {
			if (ISTREQ(tok2, "optional")) {
				gDataPortMode = gFwDataPortMode = kFallBackToSendPortMode;
			} else if (ISTREQ(tok2, "on")) {
				gDataPortMode = gFwDataPortMode = kPassiveMode;
			} else if (ISTREQ(tok2, "off")) {
				gDataPortMode = gFwDataPortMode = kSendPortMode;
			} else if ((int) isdigit(tok2[0])) {
				gDataPortMode = gFwDataPortMode = atoi(tok2);
			}
		}
	}
}	/* ProcessFirewallPrefFile */




/* Load those options specific to the firewall/proxy settings.  These are
 * kept in a different file so that other programs can read it and not
 * have to worry about the other junk in the prefs file.
 */
void
LoadFirewallPrefs(int forceReload)
{
	FILE *fp, *fp2;
	char pathName[256];
	char *cp;
	int userFile = 0;
	int sysFile = 0;

	if ((gFirewallPrefsLoaded != 0) && (forceReload == 0))
		return;
	gFirewallPrefsLoaded = 1;

	if (gOurDirectoryPath[0] == '\0')
		return;		/* Don't create in root directory. */
	(void) OurDirectoryPath(pathName, sizeof(pathName), kFirewallPrefFileName);

	/* Set default values. */
	gFirewallType = kFirewallNotInUse;
	gFirewallPort = 0;
	gFirewallHost[0] = '\0';
	gFirewallUser[0] = '\0';
	gFirewallPass[0] = '\0';
	gFirewallExceptionList[0] = '\0';

	fp2 = fopen(kGlobalFirewallPrefFileName, FOPEN_READ_TEXT);
	if (fp2 != NULL) {
		/* Initialize to system-wide defaults. */
		ProcessFirewallPrefFile(fp2);
		(void) fclose(fp2);
		sysFile++;
	}

	fp = fopen(pathName, FOPEN_READ_TEXT);
	if (fp != NULL) {
		/* Do user's firewall file. */
		ProcessFirewallPrefFile(fp);
		(void) fclose(fp);
		userFile = 1;
	}

	fp2 = fopen(kGlobalFixedFirewallPrefFileName, FOPEN_READ_TEXT);
	if (fp2 != NULL) {
		/* Override with system-wide settings. */
		ProcessFirewallPrefFile(fp2);
		(void) fclose(fp2);
		sysFile++;
	}

	if ((userFile == 0) && (sysFile == 0)) {
		/* Create a blank one, if
		 * there were no system-wide files.
		 */
		fp = fopen(pathName, FOPEN_WRITE_TEXT);
		if (fp != NULL) {
			WriteDefaultFirewallPrefs(fp);
			(void) fclose(fp);
			(void) chmod(pathName, 00600);
			gNumProgramRuns = 1;
		}
	}

	if (gFirewallExceptionList[0] == '\0') {
		FTPInitializeOurHostName(&gLib);
		cp = strchr(gLib.ourHostName, '.');

		if (cp != NULL) {
			(void) STRNCPY(gFirewallExceptionList, cp);
			(void) STRNCAT(gFirewallExceptionList, ",localdomain");
		}
	}
}	/* LoadFirewallPrefs */
