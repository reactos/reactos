/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)cmds.c	5.18 (Berkeley) 4/20/89";
#endif /* not lint */

/*
 * FTP User Program -- Command Routines.
 */
//#include <sys/param.h>
//#include <sys/wait.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/ftp.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <winsock.h>
#endif

#include <signal.h>
#include <direct.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "ftp_var.h"
#include "pathnames.h"
#include "prototypes.h"

extern	char *globerr;
extern	char home[];
extern	char *remglob();
extern	int allbinary;
extern off_t restart_point;
extern char reply_string[];

const char *mname;
jmp_buf jabort;
const char *dotrans(), *domap();

extern short portnum;
extern char *hostname;
extern int autologin;
/*
 * Connect to peer server and
 * auto-login, if possible.
 */
void setpeer(int argc, const char *argv[])
{
	char *host;

	if (connected) {
		printf("Already connected to %s, use close first.\n",
			hostname);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(to) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc > 3) {
		printf("usage: %s host-name [port]\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc > 2) {
		portnum = atoi(argv[2]);
		if (portnum <= 0) {
			printf("%s: bad port number-- %s\n", argv[1], argv[2]);
			printf ("usage: %s host-name [port]\n", argv[0]);
			(void) fflush(stdout);
			code = -1;
			return;
		}
		portnum = htons(portnum);
	}
	host = hookup(argv[1], portnum);
	if (host) {
#if defined(unix) && NBBY == 8
		int overbose;
#endif
		connected = 1;
		if (autologin)
			(void) login(argv[1]);

#if defined(unix) && NBBY == 8
/*
 * this ifdef is to keep someone form "porting" this to an incompatible
 * system and not checking this out. This way they have to think about it.
 */
		overbose = verbose;
		if (debug == 0)
			verbose = -1;
		allbinary = 0;
		if (command("SYST") == COMPLETE && overbose) {
			register char *cp, c;
			cp = index(reply_string+4, ' ');
			if (cp == NULL)
				cp = index(reply_string+4, '\r');
			if (cp) {
				if (cp[-1] == '.')
					cp--;
				c = *cp;
				*cp = '\0';
			}

			printf("Remote system type is %s.\n",
				reply_string+4);
			if (cp)
				*cp = c;
		}
		if (!strncmp(reply_string, "215 UNIX Type: L8", 17)) {
			setbinary();
			/* allbinary = 1; this violates the RFC */
			if (overbose)
			    printf("Using %s mode to transfer files.\n",
				typename);
		} else if (overbose &&
		    !strncmp(reply_string, "215 TOPS20", 10)) {
			printf(
"Remember to set tenex mode when transfering binary files from this machine.\n");
		}
		verbose = overbose;
#endif /* unix */
	}
	(void) fflush(stdout);
}

struct	types {
	const char	*t_name;
	const char	*t_mode;
	int	t_type;
	char	*t_arg;
} types[] = {
	{ "ascii",	"A",	TYPE_A,	0 },
	{ "binary",	"I",	TYPE_I,	0 },
	{ "image",	"I",	TYPE_I,	0 },
	{ "ebcdic",	"E",	TYPE_E,	0 },
	{ "tenex",	"L",	TYPE_L,	bytename },
	{0 }
};

/*
 * Set transfer type.
 */
void settype(argc, argv)
	const char *argv[];
{
	register struct types *p;
	int comret;

	if (argc > 2) {
		const char *sep;

		printf("usage: %s [", argv[0]);
		sep = " ";
		for (p = types; p->t_name; p++) {
			printf("%s%s", sep, p->t_name);
			if (*sep == ' ')
				sep = " | ";
		}
		printf(" ]\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 2) {
		printf("Using %s mode to transfer files.\n", typename);
		(void) fflush(stdout);
		code = 0;
		return;
	}
	for (p = types; p->t_name; p++)
		if (strcmp(argv[1], p->t_name) == 0)
			break;
	if (p->t_name == 0) {
		printf("%s: unknown mode\n", argv[1]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if ((p->t_arg != NULL) && (*(p->t_arg) != '\0'))
		comret = command ("TYPE %s %s", p->t_mode, p->t_arg);
	else
		comret = command("TYPE %s", p->t_mode);
	if (comret == COMPLETE) {
		(void) strcpy(typename, p->t_name);
		type = p->t_type;
	}
}

const char *stype[] = {
	"type",
	"",
	0
};

/*
 * Set binary transfer type.
 */
/*VARARGS*/
void setbinary()
{
	stype[1] = "binary";
	settype(2, stype);
}

/*
 * Set ascii transfer type.
 */
/*VARARGS*/
void setascii()
{
	stype[1] = "ascii";
	settype(2, stype);
}

/*
 * Set tenex transfer type.
 */
/*VARARGS*/
void settenex()
{
	stype[1] = "tenex";
	settype(2, stype);
}

/*
 * Set ebcdic transfer type.
 */
/*VARARGS*/
void setebcdic()
{
	stype[1] = "ebcdic";
	settype(2, stype);
}

/*
 * Set file transfer mode.
 */

/*ARGSUSED*/
void fsetmode(argc, argv)
	char *argv[];
{

	printf("We only support %s mode, sorry.\n", modename);
	(void) fflush(stdout);
	code = -1;
}


/*
 * Set file transfer format.
 */
/*ARGSUSED*/
void setform(argc, argv)
	char *argv[];
{

	printf("We only support %s format, sorry.\n", formname);
	(void) fflush(stdout);
	code = -1;
}

/*
 * Set file transfer structure.
 */
/*ARGSUSED*/
void setstruct(argc, argv)
	char *argv[];
{

	printf("We only support %s structure, sorry.\n", structname);
	(void) fflush(stdout);
	code = -1;
}

/*
 * Send a single file.
 */
void put(argc, argv)
	int argc;
	const char *argv[];
{
	const char *cmd;
	int loc = 0;
	const char *oldargv1, *oldargv2;

	if (argc == 2) {
		argc++;
		argv[2] = argv[1];
		loc++;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(local-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		printf("usage:%s local-file remote-file\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(remote-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3)
		goto usage;
	oldargv1 = argv[1];
	oldargv2 = argv[2];
	if (!globulize(&argv[1])) {
		code = -1;
		return;
	}
	/*
	 * If "globulize" modifies argv[1], and argv[2] is a copy of
	 * the old argv[1], make it a copy of the new argv[1].
	 */
	if (argv[1] != oldargv1 && argv[2] == oldargv1) {
		argv[2] = argv[1];
	}
	cmd = (argv[0][0] == 'a') ? "APPE" : ((sunique) ? "STOU" : "STOR");
	if (loc && ntflag) {
		argv[2] = dotrans(argv[2]);
	}
	if (loc && mapflag) {
		argv[2] = domap(argv[2]);
	}
	sendrequest(cmd, argv[1], argv[2],
	    argv[1] != oldargv1 || argv[2] != oldargv2);
}

/*
 * Send multiple files.
 */
void mput(argc, argv)
	const char *argv[];
{
	register int i;
	int ointer;
	extern jmp_buf jabort;
	const char *tp;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(local-files) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s local-files\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
//	oldintr = signal(SIGINT, mabort);
	(void) setjmp(jabort);
	if (proxy) {
		char *cp, *tp2, tmpbuf[MAXPATHLEN];

		while ((cp = remglob(argv,0)) != NULL) {
			if (*cp == 0) {
				mflag = 0;
				continue;
			}
			if (mflag && confirm(argv[0], cp)) {
				tp = cp;
				if (mcase) {
					while (*tp && !islower(*tp)) {
						tp++;
					}
					if (!*tp) {
						tp = cp;
						tp2 = tmpbuf;
						while ((*tp2 = *tp) != (int) NULL) {
						     if (isupper(*tp2)) {
						        *tp2 = 'a' + *tp2 - 'A';
						     }
						     tp++;
						     tp2++;
						}
					}
					tp = tmpbuf;
				}
				if (ntflag) {
					tp = dotrans(tp);
				}
				if (mapflag) {
					tp = domap(tp);
				}
				sendrequest((sunique) ? "STOU" : "STOR",
				    cp, tp, cp != tp || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm("Continue with","mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
		}
//		(void) signal(SIGINT, oldintr);
		mflag = 0;
		return;
	}
	for (i = 1; i < argc; i++) {
		register char **cpp, **gargs;

		if (!doglob) {
			if (mflag && confirm(argv[0], argv[i])) {
				tp = (ntflag) ? dotrans(argv[i]) : argv[i];
				tp = (mapflag) ? domap(tp) : tp;
				sendrequest((sunique) ? "STOU" : "STOR",
				    argv[i], tp, tp != argv[i] || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm("Continue with","mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
			continue;
		}
		gargs = glob(argv[i]);
		if (globerr != NULL) {
			printf("%s\n", globerr);
			(void) fflush(stdout);
			if (gargs) {
				blkfree(gargs);
				free((char *)gargs);
			}
			continue;
		}
		for (cpp = gargs; cpp && *cpp != NULL; cpp++) {
			if (mflag && confirm(argv[0], *cpp)) {
				tp = (ntflag) ? dotrans(*cpp) : *cpp;
				tp = (mapflag) ? domap(tp) : tp;
				sendrequest((sunique) ? "STOU" : "STOR",
				    *cpp, tp, *cpp != tp || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm("Continue with","mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
		}
		if (gargs != NULL) {
			blkfree(gargs);
			free((char *)gargs);
		}
	}
//	(void) signal(SIGINT, oldintr);
	mflag = 0;
}

void reget(argc, argv)
	const char *argv[];
{
	(void) getit(argc, argv, 1, "r+w");
}

void get(argc, argv)
	const char *argv[];
{
	(void) getit(argc, argv, 0, restart_point ? "r+w" : "w" );
}

/*
 * Receive one file.
 */
int getit(argc, argv, restartit, mode)
	const char *argv[];
	const char *mode;
{
	int loc = 0;
	const char *oldargv1, *oldargv2;

	if (argc == 2) {
		argc++;
		argv[2] = argv[1];
		loc++;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		printf("usage: %s remote-file [ local-file ]\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return (0);
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(local-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3)
		goto usage;
	oldargv1 = argv[1];
	oldargv2 = argv[2];
	if (!globulize(&argv[2])) {
		code = -1;
		return (0);
	}
	if (loc && mcase) {
		const char *tp = argv[1];
        char *tp2, tmpbuf[MAXPATHLEN];

		while (*tp && !islower(*tp)) {
			tp++;
		}
		if (!*tp) {
			tp = argv[2];
			tp2 = tmpbuf;
			while ((*tp2 = *tp) != (int) NULL) {
				if (isupper(*tp2)) {
					*tp2 = 'a' + *tp2 - 'A';
				}
				tp++;
				tp2++;
			}
			argv[2] = tmpbuf;
		}
	}
	if (loc && ntflag)
		argv[2] = dotrans(argv[2]);
	if (loc && mapflag)
		argv[2] = domap(argv[2]);
	if (restartit) {
		struct stat stbuf;
		int ret;

		ret = stat(argv[2], &stbuf);
		if (restartit == 1) {
			if (ret < 0) {
				perror(argv[2]);
				return (0);
			}
			restart_point = stbuf.st_size;
		} else {
			if (ret == 0) {
				int overbose;

				overbose = verbose;
				if (debug == 0)
					verbose = -1;
				if (command("MDTM %s", argv[1]) == COMPLETE) {
					int yy, mo, day, hour, min, sec;
					struct tm *tm;
					verbose = overbose;
					sscanf(reply_string,
					    "%*s %04d%02d%02d%02d%02d%02d",
					    &yy, &mo, &day, &hour, &min, &sec);
					tm = gmtime(&stbuf.st_mtime);
					tm->tm_mon++;
					if (tm->tm_year > yy%100)
						return (1);
					else if (tm->tm_year == yy%100) {
						if (tm->tm_mon > mo)
							return (1);
					} else if (tm->tm_mon == mo) {
						if (tm->tm_mday > day)
							return (1);
					} else if (tm->tm_mday == day) {
						if (tm->tm_hour > hour)
							return (1);
					} else if (tm->tm_hour == hour) {
						if (tm->tm_min > min)
							return (1);
					} else if (tm->tm_min == min) {
						if (tm->tm_sec > sec)
							return (1);
					}
				} else {
					printf("%s\n", reply_string);
					(void) fflush(stdout);
					verbose = overbose;
					return (0);
				}
			}
		}
	}

	recvrequest("RETR", argv[2], argv[1], mode,
	    argv[1] != oldargv1 || argv[2] != oldargv2);
	restart_point = 0;
	return (0);
}

#if 0
static void
mabort()
{
	int ointer;
	extern jmp_buf jabort;

	printf("\n");
	(void) fflush(stdout);
	if (mflag && fromatty) {
		ointer = interactive;
		interactive = 1;
		if (confirm("Continue with", mname)) {
			interactive = ointer;
			longjmp(jabort,0);
		}
		interactive = ointer;
	}
	mflag = 0;
	longjmp(jabort,0);
}
#endif

/*
 * Get multiple files.
 */
void mget(argc, argv)
	const char *argv[];
{
	const char *cp, *tp;
    char *tp2, tmpbuf[MAXPATHLEN];
	int ointer;
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-files) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s remote-files\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
//	oldintr = signal(SIGINT,mabort);
	(void) setjmp(jabort);
	while ((cp = remglob(argv,proxy)) != NULL) {
		if (*cp == '\0') {
			mflag = 0;
			continue;
		}
		if (mflag && confirm(argv[0], cp)) {
			tp = cp;
			if (mcase) {
				while (*tp && !islower(*tp)) {
					tp++;
				}
				if (!*tp) {
					tp = cp;
					tp2 = tmpbuf;
					while ((*tp2 = *tp) != (int) NULL) {
						if (isupper(*tp2)) {
							*tp2 = 'a' + *tp2 - 'A';
						}
						tp++;
						tp2++;
					}
				}
				tp = tmpbuf;
			}
			if (ntflag) {
				tp = dotrans(tp);
			}
			if (mapflag) {
				tp = domap(tp);
			}
			recvrequest("RETR", tp, cp, "w",
			    tp != cp || !interactive);
			if (!mflag && fromatty) {
				ointer = interactive;
				interactive = 1;
				if (confirm("Continue with","mget")) {
					mflag++;
				}
				interactive = ointer;
			}
		}
	}
//	(void) signal(SIGINT,oldintr);
	mflag = 0;
}

char *
remglob(argv,doswitch)
	char *argv[];
	int doswitch;
{
	char temp[16];
	static char buf[MAXPATHLEN];
	static FILE *ftemp = NULL;
	static char **args;
	int oldverbose, oldhash;
	char *cp;
    const char *mode;

	if (!mflag) {
		if (!doglob) {
			args = NULL;
		}
		else {
			if (ftemp) {
				(void) fclose(ftemp);
				ftemp = NULL;
			}
		}
		return(NULL);
	}
	if (!doglob) {
		if (args == NULL)
			args = argv;
		if ((cp = *++args) == NULL)
			args = NULL;
		return (cp);
	}
	if (ftemp == NULL) {
		(void) strcpy(temp, _PATH_TMP);
		(void) mktemp(temp);
		oldverbose = verbose, verbose = 0;
		oldhash = hash, hash = 0;
		if (doswitch) {
			pswitch(!proxy);
		}
		for (mode = "w"; *++argv != NULL; mode = "a")
			recvrequest ("NLST", temp, *argv, mode, 0);
		if (doswitch) {
			pswitch(!proxy);
		}
		verbose = oldverbose; hash = oldhash;
		ftemp = fopen(temp, "r");
		(void) unlink(temp);
		if (ftemp == NULL) {
			printf("can't find list of remote files, oops\n");
			(void) fflush(stdout);
			return (NULL);
		}
	}
	if (fgets(buf, sizeof (buf), ftemp) == NULL) {
		(void) fclose(ftemp), ftemp = NULL;
		return (NULL);
	}
	if ((cp = index(buf, '\n')) != NULL)
		*cp = '\0';
	return (buf);
}

static const char *
onoff(bool)
	int bool;
{

	return (bool ? "on" : "off");
}

/*
 * Show status.
 */
/*ARGSUSED*/
void status(argc, argv)
	char *argv[];
{
	int i;

	if (connected)
		printf("Connected to %s.\n", hostname);
	else
		printf("Not connected.\n");
	if (!proxy) {
		pswitch(1);
		if (connected) {
			printf("Connected for proxy commands to %s.\n", hostname);
		}
		else {
			printf("No proxy connection.\n");
		}
		pswitch(0);
	}
	printf("Mode: %s; Type: %s; Form: %s; Structure: %s\n",
		modename, typename, formname, structname);
	printf("Verbose: %s; Bell: %s; Prompting: %s; Globbing: %s\n",
		onoff(verbose), onoff(bell), onoff(interactive),
		onoff(doglob));
	printf("Store unique: %s; Receive unique: %s\n", onoff(sunique),
		onoff(runique));
	printf("Case: %s; CR stripping: %s\n",onoff(mcase),onoff(crflag));
	if (ntflag) {
		printf("Ntrans: (in) %s (out) %s\n", ntin,ntout);
	}
	else {
		printf("Ntrans: off\n");
	}
	if (mapflag) {
		printf("Nmap: (in) %s (out) %s\n", mapin, mapout);
	}
	else {
		printf("Nmap: off\n");
	}
	printf("Hash mark printing: %s; Use of PORT cmds: %s\n",
		onoff(hash), onoff(sendport));
	if (macnum > 0) {
		printf("Macros:\n");
		for (i=0; i<macnum; i++) {
			printf("\t%s\n",macros[i].mac_name);
		}
	}
	(void) fflush(stdout);
	code = 0;
}

/*
 * Set beep on cmd completed mode.
 */
/*VARARGS*/
void setbell()
{

	bell = !bell;
	printf("Bell mode %s.\n", onoff(bell));
	(void) fflush(stdout);
	code = bell;
}

/*
 * Turn on packet tracing.
 */
/*VARARGS*/
void settrace()
{

	trace = !trace;
	printf("Packet tracing %s.\n", onoff(trace));
	(void) fflush(stdout);
	code = trace;
}

/*
 * Toggle hash mark printing during transfers.
 */
/*VARARGS*/
void sethash()
{

	hash = !hash;
	printf("Hash mark printing %s", onoff(hash));
	code = hash;
	if (hash)
		printf(" (%d bytes/hash mark)", 1024);
	printf(".\n");
	(void) fflush(stdout);
}

/*
 * Turn on printing of server echo's.
 */
/*VARARGS*/
void setverbose()
{

	verbose = !verbose;
	printf("Verbose mode %s.\n", onoff(verbose));
	(void) fflush(stdout);
	code = verbose;
}

/*
 * Toggle PORT cmd use before each data connection.
 */
/*VARARGS*/
void setport()
{

	sendport = !sendport;
	printf("Use of PORT cmds %s.\n", onoff(sendport));
	(void) fflush(stdout);
	code = sendport;
}

/*
 * Turn on interactive prompting
 * during mget, mput, and mdelete.
 */
/*VARARGS*/
void setprompt()
{

	interactive = !interactive;
	printf("Interactive mode %s.\n", onoff(interactive));
	(void) fflush(stdout);
	code = interactive;
}

/*
 * Toggle metacharacter interpretation
 * on local file names.
 */
/*VARARGS*/
void setglob()
{

	doglob = !doglob;
	printf("Globbing %s.\n", onoff(doglob));
	(void) fflush(stdout);
	code = doglob;
}

/*
 * Set debugging mode on/off and/or
 * set level of debugging.
 */
/*VARARGS*/
void setdebug(argc, argv)
	char *argv[];
{
	int val;

	if (argc > 1) {
		val = atoi(argv[1]);
		if (val < 0) {
			printf("%s: bad debugging value.\n", argv[1]);
			(void) fflush(stdout);
			code = -1;
			return;
		}
	} else
		val = !debug;
	debug = val;
	if (debug)
		options |= SO_DEBUG;
	else
		options &= ~SO_DEBUG;
	printf("Debugging %s (debug=%d).\n", onoff(debug), debug);
	(void) fflush(stdout);
	code = debug > 0;
}

/*
 * Set current working directory
 * on remote machine.
 */
void cd(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-directory) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s remote-directory\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (command("CWD %s", argv[1]) == ERROR && code == 500) {
		if (verbose) {
			printf("CWD command not recognized, trying XCWD\n");
			(void) fflush(stdout);
		}
		(void) command("XCWD %s", argv[1]);
	}
}

/*
 * Set current working directory
 * on local machine.
 */
void lcd(argc, argv)
	const char *argv[];
{
	char buf[MAXPATHLEN];

	if (argc < 2)
		argc++, argv[1] = home;
	if (argc != 2) {
		printf("usage:%s local-directory\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (!globulize(&argv[1])) {
		code = -1;
		return;
	}
	if (chdir(argv[1]) < 0) {
		perror(argv[1]);
		code = -1;
		return;
	}
	printf("Local directory now %s\n", getcwd(buf,sizeof(buf)));
	(void) fflush(stdout);
	code = 0;
}

/*
 * Delete a single file.
 */
void delete(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s remote-file\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	(void) command("DELE %s", argv[1]);
}

/*
 * Delete multiple files.
 */
void mdelete(argc, argv)
	const char *argv[];
{
	char *cp;
	int ointer;
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-files) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s remote-files\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
//	oldintr = signal(SIGINT, mabort);
	(void) setjmp(jabort);
	while ((cp = remglob(argv,0)) != NULL) {
		if (*cp == '\0') {
			mflag = 0;
			continue;
		}
		if (mflag && confirm(argv[0], cp)) {
			(void) command("DELE %s", cp);
			if (!mflag && fromatty) {
				ointer = interactive;
				interactive = 1;
				if (confirm("Continue with", "mdelete")) {
					mflag++;
				}
				interactive = ointer;
			}
		}
	}
//	(void) signal(SIGINT, oldintr);
	mflag = 0;
}

/*
 * Rename a remote file.
 */
void renamefile(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(from-name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		printf("%s from-name to-name\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(to-name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3)
		goto usage;
	if (command("RNFR %s", argv[1]) == CONTINUE)
		(void) command("RNTO %s", argv[2]);
}

/*
 * Get a directory listing
 * of remote files.
 */
void ls(argc, argv)
	const char *argv[];
{
	const char *cmd;

	if (argc < 2)
		argc++, argv[1] = NULL;
	if (argc < 3)
		argc++, argv[2] = "-";
	if (argc > 3) {
		printf("usage: %s remote-directory local-file\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	cmd = argv[0][0] == 'n' ? "NLST" : "LIST";
//	cmd = argv[0][0] == 'n' ? "NLST -CF" : "NLST -CF";
	if (strcmp(argv[2], "-") && !globulize(&argv[2])) {
		code = -1;
		return;
	}
	if (strcmp(argv[2], "-") && *argv[2] != '|')
		if (!globulize(&argv[2]) || !confirm("output to local-file:", argv[2])) {
			code = -1;
			return;
	}
	recvrequest(cmd, argv[2], argv[1], "w", 0);
}

/*
 * Get a directory listing
 * of multiple remote files.
 */
void mls(argc, argv)
	const char *argv[];
{
	const char *cmd, *dest;
	char mode[1];
	int ointer, i;
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(remote-files) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(local-file) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		printf("usage:%s remote-files local-file\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	dest = argv[argc - 1];
	argv[argc - 1] = NULL;
	if (strcmp(dest, "-") && *dest != '|')
		if (!globulize(&dest) || !confirm("output to local-file:", dest)) {
			code = -1;
			return;
	}
	cmd = argv[0][1] == 'l' ? "NLST" : "LIST";
	mname = argv[0];
	mflag = 1;
//	oldintr = signal(SIGINT, mabort);
	(void) setjmp(jabort);
	for (i = 1; mflag && i < argc-1; ++i) {
		*mode = (i == 1) ? 'w' : 'a';
		recvrequest(cmd, dest, argv[i], mode, 0);
		if (!mflag && fromatty) {
			ointer = interactive;
			interactive = 1;
			if (confirm("Continue with", argv[0])) {
				mflag ++;
			}
			interactive = ointer;
		}
	}
//	(void) signal(SIGINT, oldintr);
	mflag = 0;
}

/*
 * Do a shell escape
 */
/*ARGSUSED*/
void shell(argc, argv)
	char *argv[];
{
#if 0
	int pid;
	sig_t (*old1)(), (*old2)();
	char shellnam[40], *shell, *namep;
	union wait status;

	old1 = signal (SIGINT, SIG_IGN);
	old2 = signal (SIGQUIT, SIG_IGN);
	if ((pid = fork()) == 0) {
		for (pid = 3; pid < 20; pid++)
			(void) close(pid);
		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGQUIT, SIG_DFL);
		shell = getenv("SHELL");
		if (shell == NULL)
			shell = _PATH_BSHELL;
		namep = rindex(shell,'/');
		if (namep == NULL)
			namep = shell;
		(void) strcpy(shellnam,"-");
		(void) strcat(shellnam, ++namep);
		if (strcmp(namep, "sh") != 0)
			shellnam[0] = '+';
		if (debug) {
			printf ("%s\n", shell);
			(void) fflush (stdout);
		}
		if (argc > 1) {
			execl(shell,shellnam,"-c",altarg,(char *)0);
		}
		else {
			execl(shell,shellnam,(char *)0);
		}
		perror(shell);
		code = -1;
		exit(1);
		}
	if (pid > 0)
		while (wait(&status) != pid)
			;
	(void) signal(SIGINT, old1);
	(void) signal(SIGQUIT, old2);
	if (pid == -1) {
		perror("Try again later");
		code = -1;
	}
	else {
		code = 0;
	}
#endif

    char *              AppName;
    char                ShellCmd[MAX_PATH];
    char                CmdLine[MAX_PATH];
    int                 i;
    PROCESS_INFORMATION ProcessInformation;
    BOOL                Result;
    STARTUPINFO         StartupInfo;
    char                ShellName[] = "COMSPEC";
    int                 NumBytes;

    NumBytes = GetEnvironmentVariable( ShellName, ShellCmd, MAX_PATH);

    if (NumBytes == 0)
    {
        return;
    }

    AppName = ShellCmd;
    strcpy( CmdLine, ShellCmd );

    if (argc > 1)
    {
        strncat(CmdLine, " /C", MAX_PATH);
    }

    for (i=1; i<argc; i++)
    {
        strncat(CmdLine, " ", MAX_PATH);
        strncat(CmdLine, argv[i], MAX_PATH);
    }

    StartupInfo.cb          = sizeof( StartupInfo );
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = NULL;
    StartupInfo.dwX         = 0;
    StartupInfo.dwY         = 0;
    StartupInfo.dwXSize     = 0;
    StartupInfo.dwYSize     = 0;
    StartupInfo.dwFlags     = 0;
    StartupInfo.wShowWindow = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;

    Result = CreateProcess( AppName,                // cmd name
                            CmdLine,                // cmd line arguments
                            NULL,
                            NULL,                   // security attributes
                            FALSE,                  // inherit flags
                            0,                      // Creation flags
                            NULL,                   // Environment
                            NULL,                   // Current directory
                            &StartupInfo,           // Startup info structure
                            &ProcessInformation);   // processInfo structure

    if (Result)
    {
        WaitForSingleObject( ProcessInformation.hProcess, 0xffffffff);

        CloseHandle( ProcessInformation.hProcess);
    }
}

/*
 * Send new user information (re-login)
 */
void user(argc, argv)
	int argc;
	const char **argv;
{
	char acct[80], *getpass();
	int n, aflag = 0;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(username) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc > 4) {
		printf("usage: %s username [password] [account]\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	n = command("USER %s", argv[1]);
	if (n == CONTINUE) {
		if (argc < 3 )
			argv[2] = getpass("Password: "), argc++;
		n = command("PASS %s", argv[2]);
	}
	if (n == CONTINUE) {
		if (argc < 4) {
			printf("Account: "); (void) fflush(stdout);
			(void) fflush(stdout);
			(void) fgets(acct, sizeof(acct) - 1, stdin);
			acct[strlen(acct) - 1] = '\0';
			argv[3] = acct; argc++;
		}
		n = command("ACCT %s", argv[3]);
		aflag++;
	}
	if (n != COMPLETE) {
		fprintf(stdout, "Login failed.\n");
		(void) fflush(stdout);
		return;
	}
	if (!aflag && argc == 4) {
		(void) command("ACCT %s", argv[3]);
	}
}

/*
 * Print working directory.
 */
/*VARARGS*/
void pwd()
{
	int oldverbose = verbose;

	/*
	 * If we aren't verbose, this doesn't do anything!
	 */
	verbose = 1;
	if (command("PWD") == ERROR && code == 500) {
		printf("PWD command not recognized, trying XPWD\n");
		(void) fflush(stdout);
		(void) command("XPWD");
	}
	verbose = oldverbose;
}

/*
 * Make a directory.
 */
void makedir(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(directory-name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage: %s directory-name\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (command("MKD %s", argv[1]) == ERROR && code == 500) {
		if (verbose) {
			printf("MKD command not recognized, trying XMKD\n");
			(void) fflush(stdout);
		}
		(void) command("XMKD %s", argv[1]);
	}
}

/*
 * Remove a directory.
 */
void removedir(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(directory-name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage: %s directory-name\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (command("RMD %s", argv[1]) == ERROR && code == 500) {
		if (verbose) {
			printf("RMD command not recognized, trying XRMD\n");
			(void) fflush(stdout);
		}
		(void) command("XRMD %s", argv[1]);
	}
}

/*
 * Send a line, verbatim, to the remote machine.
 */
void quote(argc, argv)
	const char *argv[];
{
	int i;
	char buf[BUFSIZ];

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(command line to send) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage: %s line-to-send\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	(void) strcpy(buf, argv[1]);
	for (i = 2; i < argc; i++) {
		(void) strcat(buf, " ");
		(void) strcat(buf, argv[i]);
	}
	if (command(buf) == PRELIM) {
		while (getreply(0) == PRELIM);
	}
}

/*
 * Send a SITE command to the remote machine.  The line
 * is sent almost verbatim to the remote machine, the
 * first argument is changed to SITE.
 */

void site(argc, argv)
	const char *argv[];
{
	int i;
	char buf[BUFSIZ];

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(arguments to SITE command) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage: %s line-to-send\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	(void) strcpy(buf, "SITE ");
	(void) strcat(buf, argv[1]);
	for (i = 2; i < argc; i++) {
		(void) strcat(buf, " ");
		(void) strcat(buf, argv[i]);
	}
	if (command(buf) == PRELIM) {
		while (getreply(0) == PRELIM);
	}
}

void do_chmod(argc, argv)
	const char *argv[];
{
	if (argc == 2) {
		printf("usage: %s mode file-name\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(mode and file-name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 3) {
		printf("usage: %s mode file-name\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	(void)command("SITE CHMOD %s %s", argv[1], argv[2]);
}

void do_umask(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "SITE UMASK" : "SITE UMASK %s", argv[1]);
	verbose = oldverbose;
}

void idle(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "SITE IDLE" : "SITE IDLE %s", argv[1]);
	verbose = oldverbose;
}

/*
 * Ask the other side for help.
 */
void rmthelp(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "HELP" : "HELP %s", argv[1]);
	verbose = oldverbose;
}

/*
 * Terminate session and exit.
 */
/*VARARGS*/
void quit()
{

	if (connected)
		disconnect();
	pswitch(1);
	if (connected) {
		disconnect();
	}
	exit(0);
}

/*
 * Terminate session, but don't exit.
 */
void disconnect()
{
	extern int cout;
	extern int data;

	if (!connected)
		return;
	(void) command("QUIT");
	cout = (int) NULL;
	connected = 0;
	data = -1;
	if (!proxy) {
		macnum = 0;
	}
}

int confirm(cmd, file)
	const char *cmd, *file;
{
	char line[BUFSIZ];

	if (!interactive)
		return (1);
	printf("%s %s? ", cmd, file);
	(void) fflush(stdout);
	(void) gets(line);
	return (*line != 'n' && *line != 'N');
}

#if 0
static void fatal(msg)
	char *msg;
{

	fprintf(stderr, "ftp: %s\n", msg);
	exit(1);
}
#endif

/*
 * Glob a local file name specification with
 * the expectation of a single return value.
 * Can't control multiple values being expanded
 * from the expression, we return only the first.
 */
int globulize(cpp)
	const char **cpp;
{
	char **globbed;

	if (!doglob)
		return (1);
	globbed = glob(*cpp);
	if (globerr != NULL) {
		printf("%s: %s\n", *cpp, globerr);
		(void) fflush(stdout);
		if (globbed) {
			blkfree(globbed);
			free((char *)globbed);
		}
		return (0);
	}
	if (globbed) {
		*cpp = *globbed++;
		/* don't waste too much memory */
		if (*globbed) {
			blkfree(globbed);
			free((char *)globbed);
		}
	}
	return (1);
}

void account(argc,argv)
	int argc;
	char **argv;
{
	char acct[50], *getpass(), *ap;

	if (argc > 1) {
		++argv;
		--argc;
		(void) strncpy(acct,*argv,49);
		acct[49] = '\0';
		while (argc > 1) {
			--argc;
			++argv;
			(void) strncat(acct,*argv, 49-strlen(acct));
		}
		ap = acct;
	}
	else {
		ap = getpass("Account:");
	}
	(void) command("ACCT %s", ap);
}

jmp_buf abortprox;

#if 0
static void
proxabort()
{
	extern int proxy;

	if (!proxy) {
		pswitch(1);
	}
	if (connected) {
		proxflag = 1;
	}
	else {
		proxflag = 0;
	}
	pswitch(0);
	longjmp(abortprox,1);
}
#endif

void doproxy(argc,argv)
	int argc;
	const char *argv[];
{
	register struct cmd *c;
	struct cmd *getcmd();
//	extern struct cmd cmdtab[];
	extern jmp_buf abortprox;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(command) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s command\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	c = getcmd(argv[1]);
	if (c == (struct cmd *) -1) {
		printf("?Ambiguous command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (c == 0) {
		printf("?Invalid command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (!c->c_proxy) {
		printf("?Invalid proxy command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (setjmp(abortprox)) {
		code = -1;
		return;
	}
//	oldintr = signal(SIGINT, proxabort);
	pswitch(1);
	if (c->c_conn && !connected) {
		printf("Not connected\n");
		(void) fflush(stdout);
		pswitch(0);
//		(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	(*c->c_handler)(argc-1, argv+1);
	if (connected) {
		proxflag = 1;
	}
	else {
		proxflag = 0;
	}
	pswitch(0);
//	(void) signal(SIGINT, oldintr);
}

void setcase()
{
	mcase = !mcase;
	printf("Case mapping %s.\n", onoff(mcase));
	(void) fflush(stdout);
	code = mcase;
}

void setcr()
{
	crflag = !crflag;
	printf("Carriage Return stripping %s.\n", onoff(crflag));
	(void) fflush(stdout);
	code = crflag;
}

void setntrans(argc,argv)
	int argc;
	char *argv[];
{
	if (argc == 1) {
		ntflag = 0;
		printf("Ntrans off.\n");
		(void) fflush(stdout);
		code = ntflag;
		return;
	}
	ntflag++;
	code = ntflag;
	(void) strncpy(ntin, argv[1], 16);
	ntin[16] = '\0';
	if (argc == 2) {
		ntout[0] = '\0';
		return;
	}
	(void) strncpy(ntout, argv[2], 16);
	ntout[16] = '\0';
}

const char *
dotrans(name)
	const char *name;
{
	static char new[MAXPATHLEN];
	const char *cp1;
    char *cp2 = new;
	register int i, ostop, found;

	for (ostop = 0; *(ntout + ostop) && ostop < 16; ostop++);
	for (cp1 = name; *cp1; cp1++) {
		found = 0;
		for (i = 0; *(ntin + i) && i < 16; i++) {
			if (*cp1 == *(ntin + i)) {
				found++;
				if (i < ostop) {
					*cp2++ = *(ntout + i);
				}
				break;
			}
		}
		if (!found) {
			*cp2++ = *cp1;
		}
	}
	*cp2 = '\0';
	return(new);
}


void
setpassive(argc, argv)
  int argc;
  char *argv[];
{
	passivemode = !passivemode;
	printf("Passive mode %s.\n", onoff(passivemode));
	(void) fflush(stdout);
	code = passivemode;
}

void setnmap(argc, argv)
	int argc;
	const char *argv[];
{
	char *cp;

	if (argc == 1) {
		mapflag = 0;
		printf("Nmap off.\n");
		(void) fflush(stdout);
		code = mapflag;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		printf("(mapout) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		printf("Usage: %s [mapin mapout]\n",argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	mapflag = 1;
	code = 1;
	cp = index(altarg, ' ');
	if (proxy) {
		while(*++cp == ' ');
		altarg = cp;
		cp = index(altarg, ' ');
	}
	*cp = '\0';
	(void) strncpy(mapin, altarg, MAXPATHLEN - 1);
	while (*++cp == ' ');
	(void) strncpy(mapout, cp, MAXPATHLEN - 1);
}

const char *
domap(name)
	const char *name;
{
	static char new[MAXPATHLEN];
	const char *cp1 = name;
    char *cpn, *cp2 = mapin;
	const char *tp[9], *te[9];
	int i, toks[9], toknum = 0, match = 1;

	for (i=0; i < 9; ++i) {
		toks[i] = 0;
	}
	while (match && *cp1 && *cp2) {
		switch (*cp2) {
			case '\\':
				if (*++cp2 != *cp1) {
					match = 0;
				}
				break;
			case '$':
				if (*(cp2+1) >= '1' && (*cp2+1) <= '9') {
					if (*cp1 != *(++cp2+1)) {
						toks[toknum = *cp2 - '1']++;
						tp[toknum] = cp1;
						while (*++cp1 && *(cp2+1)
							!= *cp1);
						te[toknum] = cp1;
					}
					cp2++;
					break;
				}
				/* FALLTHROUGH */
			default:
				if (*cp2 != *cp1) {
					match = 0;
				}
				break;
		}
		if (match && *cp1) {
			cp1++;
		}
		if (match && *cp2) {
			cp2++;
		}
	}
	if (!match && *cp1) /* last token mismatch */
	{
		toks[toknum] = 0;
	}

	cpn = new;
	*cpn = '\0';
	cp2 = mapout;
	while (*cp2) {
		match = 0;
		switch (*cp2) {
			case '\\':
				if (*(cp2 + 1)) {
					*cpn++ = *++cp2;
				}
				break;
			case '[':
LOOP:
				if (*++cp2 == '$' && isdigit(*(cp2+1))) {
					if (*++cp2 == '0') {
						const char *cp3 = name;

						while (*cp3) {
							*cpn++ = *cp3++;
						}
						match = 1;
					}
					else if (toks[toknum = *cp2 - '1']) {
						const char *cp3 = tp[toknum];

						while (cp3 != te[toknum]) {
							*cpn++ = *cp3++;
						}
						match = 1;
					}
				}
				else {
					while (*cp2 && *cp2 != ',' &&
					    *cp2 != ']') {
						if (*cp2 == '\\') {
							cp2++;
						}
						else if (*cp2 == '$' &&
   						        isdigit(*(cp2+1))) {
							if (*++cp2 == '0') {
							   const char *cp3 = name;

							   while (*cp3) {
								*cpn++ = *cp3++;
							   }
							}
							else if (toks[toknum =
							    *cp2 - '1']) {
							   const char *cp3=tp[toknum];

							   while (cp3 !=
								  te[toknum]) {
								*cpn++ = *cp3++;
							   }
							}
						}
						else if (*cp2) {
							*cpn++ = *cp2++;
						}
					}
					if (!*cp2) {
						printf("nmap: unbalanced brackets\n");
						(void) fflush(stdout);
						return(name);
					}
					match = 1;
					cp2--;
				}
				if (match) {
					while (*++cp2 && *cp2 != ']') {
					      if (*cp2 == '\\' && *(cp2 + 1)) {
							cp2++;
					      }
					}
					if (!*cp2) {
						printf("nmap: unbalanced brackets\n");
						(void) fflush(stdout);
						return(name);
					}
					break;
				}
				switch (*++cp2) {
					case ',':
						goto LOOP;
					case ']':
						break;
					default:
						cp2--;
						goto LOOP;
				}
				break;
			case '$':
				if (isdigit(*(cp2 + 1))) {
					if (*++cp2 == '0') {
						const char *cp3 = name;

						while (*cp3) {
							*cpn++ = *cp3++;
						}
					}
					else if (toks[toknum = *cp2 - '1']) {
						const char *cp3 = tp[toknum];

						while (cp3 != te[toknum]) {
							*cpn++ = *cp3++;
						}
					}
					break;
				}
				/* intentional drop through */
			default:
				*cpn++ = *cp2;
				break;
		}
		cp2++;
	}
	*cpn = '\0';
	if (!*new) {
		return(name);
	}
	return(new);
}

void setsunique()
{
	sunique = !sunique;
	printf("Store unique %s.\n", onoff(sunique));
	(void) fflush(stdout);
	code = sunique;
}

void setrunique()
{
	runique = !runique;
	printf("Receive unique %s.\n", onoff(runique));
	(void) fflush(stdout);
	code = runique;
}

/* change directory to perent directory */
void cdup()
{
	if (command("CDUP") == ERROR && code == 500) {
		if (verbose) {
			printf("CDUP command not recognized, trying XCUP\n");
			(void) fflush(stdout);
		}
		(void) command("XCUP");
	}
}

/* restart transfer at specific point */
void restart(argc, argv)
	int argc;
	char *argv[];
{
	if (argc != 2)
		printf("restart: offset not specified\n");
	else {
		restart_point = atol(argv[1]);
		printf("restarting at %ld. %s\n", restart_point,
		    "execute get, put or append to initiate transfer");
	}
	(void) fflush(stdout);
}

/* show remote system type */
void syst()
{
	(void) command("SYST");
}

void macdef(argc, argv)
	int argc;
	const char *argv[];
{
	char *tmp;
	int c;

	if (macnum == 16) {
		printf("Limit of 16 macros have already been defined\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(macro name) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("Usage: %s macro_name\n",argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (interactive) {
		printf("Enter macro line by line, terminating it with a null line\n");
		(void) fflush(stdout);
	}
	(void) strncpy(macros[macnum].mac_name, argv[1], 8);
	if (macnum == 0) {
		macros[macnum].mac_start = macbuf;
	}
	else {
		macros[macnum].mac_start = macros[macnum - 1].mac_end + 1;
	}
	tmp = macros[macnum].mac_start;
	while (tmp != macbuf+4096) {
		if ((c = getchar()) == EOF) {
			printf("macdef:end of file encountered\n");
			(void) fflush(stdout);
			code = -1;
			return;
		}
		if ((*tmp = c) == '\n') {
			if (tmp == macros[macnum].mac_start) {
				macros[macnum++].mac_end = tmp;
				code = 0;
				return;
			}
			if (*(tmp-1) == '\0') {
				macros[macnum++].mac_end = tmp - 1;
				code = 0;
				return;
			}
			*tmp = '\0';
		}
		tmp++;
	}
	while (1) {
		while ((c = getchar()) != '\n' && c != EOF)
			/* LOOP */;
		if (c == EOF || getchar() == '\n') {
			printf("Macro not defined - 4k buffer exceeded\n");
			(void) fflush(stdout);
			code = -1;
			return;
		}
	}
}

/*
 * get size of file on remote machine
 */
void sizecmd(argc, argv)
	const char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(filename) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s filename\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	(void) command("SIZE %s", argv[1]);
}

/*
 * get last modification time of file on remote machine
 */
void modtime(argc, argv)
	const char *argv[];
{
	int overbose;

	if (argc < 2) {
		(void) strcat(line, " ");
		printf("(filename) ");
		(void) fflush(stdout);
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		printf("usage:%s filename\n", argv[0]);
		(void) fflush(stdout);
		code = -1;
		return;
	}
	overbose = verbose;
	if (debug == 0)
		verbose = -1;
	if (command("MDTM %s", argv[1]) == COMPLETE) {
		int yy, mo, day, hour, min, sec;
		sscanf(reply_string, "%*s %04d%02d%02d%02d%02d%02d", &yy, &mo,
			&day, &hour, &min, &sec);
		/* might want to print this in local time */
		printf("%s\t%02d/%02d/%04d %02d:%02d:%02d GMT\n", argv[1],
			mo, day, yy, hour, min, sec);
	} else
		printf("%s\n", reply_string);
	verbose = overbose;
	(void) fflush(stdout);
}

/*
 * show status on reomte machine
 */
void rmtstatus(argc, argv)
	const char *argv[];
{
	(void) command(argc > 1 ? "STAT %s" : "STAT" , argv[1]);
}

/*
 * get file if modtime is more recent than current file
 */
void newer(argc, argv)
	const char *argv[];
{
	if (getit(argc, argv, -1, "w")) {
		printf("Local file \"%s\" is newer than remote file \"%s\"\n",
			argv[1], argv[2]);
		(void) fflush(stdout);
	}
}
