#include <winsock.h>
#include "fake.h"
#include "prototypes.h"
#include <setjmp.h>
#include <sys/time.h>

//typedef void (*Sig_t)(int);

/* The following defines are from ftp.h and telnet.h from bsd.h */
/* All relevent copyrights below apply.                         */

#define	IAC	255
#define	DONT	254
#define	DO	253
#define	WONT	252
#define	WILL	251
#define	SB	250
#define	GA	249
#define	EL	248
#define	EC	247
#define	AYT	246
#define	AO	245
#define	IP	244
#define	BREAK	243
#define	DM	242
#define	NOP	241
#define	SE	240
#define EOR     239
#define	ABORT	238
#define	SUSP	237
#define	xEOF	236


#define MAXPATHLEN 255
#define TYPE_A 'A'
#define TYPE_I 'I'
#define TYPE_E 'E'
#define TYPE_L 'L'

#define PRELIM		1
#define COMPLETE	2
#define CONTINUE	3
#define TRANSIENT	4

#define	MODE_S		1
#define	MODE_B		2
#define	MODE_C		3

#define	STRU_F		1
#define	STRU_R		2
#define	STRU_P		3

#define	FORM_N		1
#define	FORM_T		2
#define	FORM_C		3


/*
 * Copyright (c) 1985 Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)ftp_var.h	5.5 (Berkeley) 6/29/88
 */

/*
 * FTP global variables.
 */

/*
 * Options and other state info.
 */
extern int	trace;			/* trace packets exchanged */
extern int	hash;			/* print # for each buffer transferred */
extern int	sendport;		/* use PORT cmd for each data connection */
extern int	verbose;		/* print messages coming back from server */
extern int	connected;		/* connected to server */
extern int	fromatty;		/* input is from a terminal */
extern int	interactive;		/* interactively prompt on m* cmds */
extern int	debug;			/* debugging level */
extern int	bell;			/* ring bell on cmd completion */
extern int	doglob;			/* glob local file names */
extern int	proxy;			/* proxy server connection active */
extern int	proxflag;		/* proxy connection exists */
extern int	sunique;		/* store files on server with unique name */
extern int	runique;		/* store local files with unique name */
extern int	mcase;			/* map upper to lower case for mget names */
extern int	ntflag;			/* use ntin ntout tables for name translation */
extern int	mapflag;		/* use mapin mapout templates on file names */
extern int	code;			/* return/reply code for ftp command */
extern int	crflag;			/* if 1, strip car. rets. on ascii gets */
extern char	pasv[64];		/* passive port for proxy data connection */
extern int  passivemode;    /* passive mode enabled */
extern char	*altarg;		/* argv[1] with no shell-like preprocessing  */
extern char	ntin[17];		/* input translation table */
extern char	ntout[17];		/* output translation table */

extern char	mapin[MAXPATHLEN];	/* input map template */
extern char	mapout[MAXPATHLEN];	/* output map template */
extern char	typename[32];		/* name of file transfer type */
extern int	type;			/* file transfer type */
extern char	structname[32];		/* name of file transfer structure */
extern int	stru;			/* file transfer structure */
extern char	formname[32];		/* name of file transfer format */
extern int	form;			/* file transfer format */
extern char	modename[32];		/* name of file transfer mode */
extern int	mode;			/* file transfer mode */
extern char	bytename[32];		/* local byte size in ascii */
extern int	bytesize;		/* local byte size in binary */

extern jmp_buf	toplevel;		/* non-local goto stuff for cmd scanner */

extern char	line[200];		/* input line buffer */
extern char	*stringbase;		/* current scan point in line buffer */
extern char	argbuf[200];		/* argument storage buffer */
extern char	*argbase;		/* current storage point in arg buffer */
extern int	margc;			/* count of arguments on input line */
extern const char	*margv[20];		/* args parsed from input line */
extern int     cpend;                  /* flag: if != 0, then pending server reply */
extern int	mflag;			/* flag: if != 0, then active multi command */

extern int	options;		/* used during socket creation */

/*
 * Format of command table.
 */
struct cmd {
	const char	*c_name;	/* name of command */
	const char	*c_help;	/* help string */
	char	c_bell;		/* give bell when command completes */
	char	c_conn;		/* must be connected to use command */
	char	c_proxy;	/* proxy server may execute */
	void	(*c_handler)();	/* function to call */
};

struct macel {
	char mac_name[9];	/* macro name */
	char *mac_start;	/* start of macro in macbuf */
	char *mac_end;		/* end of macro in macbuf */
};

int macnum;			/* number of defined macros */
struct macel macros[16];
char macbuf[4096];

#if	defined(__ANSI__) || defined(sparc)
typedef void sig_t;
#else
typedef int sig_t;
#endif

typedef int uid_t;

