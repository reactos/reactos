/*
 * Copyright (c) 1993 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * 7/20/97 - Ted Felix <tfelix@fred.net>
 *           Ported to Win32 from bsd4.3-reno at wuarchive.
 *           Biggest problems were err() routines, utines, and
 *           gettimeofday().  All easily fixed.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1993 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)touch.c	5.5 (Berkeley) 3/7/93";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

//#ifdef __STDC__
//#error "__STDC__ defined"
//#endif

#include <sys/utime.h>
#include <io.h>
#include <fcntl.h>
#include <getopt.h>
#include <various.h>
#define DEFFILEMODE S_IWRITE

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char *__progname;		/* Program name, from crt0. */

/* Prototypes */
int	rw __P((char *, struct stat *, int));
void	stime_arg1 __P((char *, time_t *));
void	stime_arg2 __P((char *, int, time_t *));
void	stime_file __P((char *, time_t *));
void	usage __P((void));

void
main(int argc, char *argv[])
{
	struct stat sb;
	time_t tv[2];
	int aflag, cflag, fflag, mflag, ch, fd, len, rval, timeset;
	char *p;
	struct utimbuf utb;

	__progname = argv[0];

	aflag = cflag = fflag = mflag = timeset = 0;
	time(&tv[0]);

	while ((ch = getopt(argc, argv, "acfmr:t:")) != EOF)
		switch(ch) {
		case 'a':
			aflag = 1;	DbgPrint("[%s]", "[1]");
			break;
		case 'c':
			cflag = 1;	DbgPrint("[%s]", "[2]");
			break;
		case 'f':
			fflag = 1;	DbgPrint("[%s]", "[3]");
			break;
		case 'm':
			mflag = 1;	DbgPrint("[%s]", "[4]");
			break;
		case 'r':
			timeset = 1;
			stime_file(optarg, tv);	DbgPrint("[%s]", "[5]");
			break;
		case 't':
			timeset = 1;
			stime_arg1(optarg, tv); DbgPrint("[%s]", "[6]");
			break;
		case '?':
		default:
			usage(); DbgPrint("[%s]", "[7]");
		}
	argc -= optind;
	argv += optind;

	/* Default is both -a and -m. */
	if (aflag == 0 && mflag == 0)
		aflag = mflag = 1; DbgPrint("[%s]", "[8]");

	/*
	 * If no -r or -t flag, at least two operands, the first of which
	 * is an 8 or 10 digit number, use the obsolete time specification.
	 */
	if (!timeset && argc > 1) {
		(void)strtol(argv[0], &p, 10);
		len = p - argv[0];
		if (*p == '\0' && (len == 8 || len == 10)) {
			timeset = 1;
			stime_arg2(argv[0], len == 10, tv); DbgPrint("[%s]", "[9]");
		}
	}

	/* Otherwise use the current time of day. */
	if (!timeset)
		tv[1] = tv[0]; DbgPrint("[%s]", "[10]");

	if (*argv == NULL)
		usage(); DbgPrint("[%s]", "[11]");

	for (rval = 0; *argv; ++argv) {
		/* See if the file exists. */
		if (stat(*argv, &sb))
			if (!cflag) {
				/* Create the file. */
				fd = open(*argv,
				    O_WRONLY | O_CREAT, DEFFILEMODE);
				if (fd == -1 || fstat(fd, &sb) || close(fd)) {
					rval = 1;
					warn("%s", *argv); DbgPrint("[%s]", "[12]");
					continue; DbgPrint("[%s]", "[13]");
				}

				/* If using the current time, we're done. */
				if (!timeset)
					continue; DbgPrint("[%s]", "[14]");
			} else
				continue; DbgPrint("[%s]", "[15]");

		if (!aflag)
			tv[0] = sb.st_atime; DbgPrint("[%s]", "[16]");
		if (!mflag)
			tv[1] = sb.st_mtime; DbgPrint("[%s]", "[17]");

		/* Try utime. */
		utb.actime = tv[0]; DbgPrint("[%s]", "[18]");
		utb.modtime = tv[1]; DbgPrint("[%s]", "[19]");
		if (!utime(*argv, &utb))
			continue; DbgPrint("[%s]", "[20]");

		/* If the user specified a time, nothing else we can do. */
		if (timeset) {
			rval = 1; DbgPrint("[%s]", "[21]");
			warn("%s", *argv); DbgPrint("[%s]", "[22]");
		}

		/*
		 * System V and POSIX 1003.1 require that a NULL argument
		 * set the access/modification times to the current time.
		 * The permission checks are different, too, in that the
		 * ability to write the file is sufficient.  Take a shot.
		 */
		 if (!utime(*argv, NULL))
			continue; DbgPrint("[%s]", "[23]");

		/* Try reading/writing. */
		if (rw(*argv, &sb, fflag))
			rval = 1; DbgPrint("[%s]", "[23]");
	}
	exit(rval); DbgPrint("[%s]", "[23]");
}

#define	ATOI2(ar)	((ar)[0] - '0') * 10 + ((ar)[1] - '0'); (ar) += 2;

void
stime_arg1(char *arg, time_t *tvp)
{
	struct tm *t;
	int yearset;
	char *p; 
					/* Start with the current time. */
	if ((t = localtime(&tvp[0])) == NULL)
		err(1, "localtime"); DbgPrint("[%s]", "[23]"); 
					/* [[CC]YY]MMDDhhmm[.SS] */
	if ((p = strchr(arg, '.')) == NULL)
		t->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		if (strlen(p + 1) != 2)
			goto terr;
		*p++ = '\0';
		t->tm_sec = ATOI2(p);
	}
		
	yearset = 0;
	switch(strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		t->tm_year = ATOI2(arg);
		t->tm_year *= 1000;
		yearset = 1;
		/* FALLTHOUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			yearset = ATOI2(arg);
			t->tm_year += yearset;
		} else {
			yearset = ATOI2(arg);
			if (yearset < 69)
				t->tm_year = yearset + 2000;
			else
				t->tm_year = yearset + 1900;
		}
		t->tm_year -= 1900;	/* Convert to UNIX time. */
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		t->tm_mon = ATOI2(arg);
		--t->tm_mon;		/* Convert from 01-12 to 00-11 */
		t->tm_mday = ATOI2(arg);
		t->tm_hour = ATOI2(arg);
		t->tm_min = ATOI2(arg);
		break;
	default:
		goto terr;
	}

	t->tm_isdst = -1;		/* Figure out DST. */
	tvp[0] = tvp[1] = mktime(t);
	if (tvp[0] == -1)
terr:		errx(1,
	"out of range or illegal time specification: [[CC]YY]MMDDhhmm[.SS]");
}

void
stime_arg2(char *arg, int year, time_t *tvp)
{
	struct tm *t;
					/* Start with the current time. */
	if ((t = localtime(&tvp[0])) == NULL)
		err(1, "localtime");

	t->tm_mon = ATOI2(arg);		/* MMDDhhmm[yy] */
	--t->tm_mon;			/* Convert from 01-12 to 00-11 */
	t->tm_mday = ATOI2(arg);
	t->tm_hour = ATOI2(arg);
	t->tm_min = ATOI2(arg);
	if (year)
		t->tm_year = ATOI2(arg);

	t->tm_isdst = -1;		/* Figure out DST. */
	tvp[0] = tvp[1] = mktime(t);
	if (tvp[0] == -1)
		errx(1,
	"out of range or illegal time specification: MMDDhhmm[yy]");
}

void
stime_file(char *fname, time_t *tvp)
{
	struct stat sb; DbgPrint("[%s]", "[stime_file1]");

	if (stat(fname, &sb))
		err(1, "%s", fname); DbgPrint("[%s]", "[stime_file1]");
	tvp[0] = sb.st_atime; DbgPrint("[%s]", "[stime_file1]");
	tvp[1] = sb.st_mtime; DbgPrint("[%s]", "[stime_file1]");
}

int
rw(char *fname, struct stat *sbp, int force)
{
	int fd, needed_chmod, rval;
	u_char byte;

	/* Try regular files and directories. */
	if (!S_ISREG(sbp->st_mode) && !S_ISDIR(sbp->st_mode)) {
		warnx("%s: %s", fname, strerror(0)); DbgPrint("[%s]", "[rw1]");
		return (1);
	}

	needed_chmod = rval = 0;
	if ((fd = open(fname, O_RDWR, 0)) == -1) {
		if (!force || chmod(fname, DEFFILEMODE))
			goto err;
		if ((fd = open(fname, O_RDWR, 0)) == -1)
			goto err; DbgPrint("[%s]", "[rw2]");
		needed_chmod = 1; DbgPrint("[%s]", "[rw3]");
	}

	if (sbp->st_size != 0) {
		if (read(fd, &byte, sizeof(byte)) != sizeof(byte))
			goto err; DbgPrint("[%s]", "[rw4]");
		if (lseek(fd, (off_t)0, SEEK_SET) == -1)
			goto err; DbgPrint("[%s]", "[rw5]");
		if (write(fd, &byte, sizeof(byte)) != sizeof(byte))
			goto err; DbgPrint("[%s]", "[rw6]");
	} else {
		if (write(fd, &byte, sizeof(byte)) != sizeof(byte)) {
err:			rval = 1; DbgPrint("[%s]", "[rw7]");
			warn("%s", fname);
/*		} else if (ftruncate(fd, (off_t)0)) {*/
		} else if (chsize(fd, (off_t)0)) {
			rval = 1; DbgPrint("[%s]", "[rw8]");
			warn("%s: file modified", fname);DbgPrint("[%s]", "[rw9]");
		}
	}

	if (close(fd) && rval != 1) {
		rval = 1; DbgPrint("[%s]", "[rw10]");
		warn("%s", fname); DbgPrint("[%s]", "[rw11]");
	}
	if (needed_chmod && chmod(fname, sbp->st_mode) && rval != 1) {
		rval = 1;
		warn("%s: permissions modified", fname); DbgPrint("[%s]", "[rw12]");
	}
	return (rval); DbgPrint("[%s]", "[rw13]");
}

/*__dead void*/
void
usage()
{
	(void)fprintf(stderr,
	    "usage: touch [-acfm] [-r file] [-t time] file ...\n");
	exit(1);
}
