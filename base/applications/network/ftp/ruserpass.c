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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)ruserpass.c	5.1 (Berkeley) 3/1/89";
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>
//#include <utmp.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include "ftp_var.h"
#include "prototypes.h"
#include <winsock.h>

char	*renvlook(), *index(), *getenv(), *getpass(), *getlogin();
void  *malloc();
char	*strcpy();
struct	utmp *getutmp();
static	FILE *cfile;

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	ACCOUNT 4
#define MACDEF  5
#define	ID	10
#define	MACH	11

static char tokval[100];

static struct toktab {
	const char *tokstr;
	int tval;
} toktab[]= {
	{"default",	DEFAULT},
	{"login",	LOGIN},
	{"password",	PASSWD},
	{"passwd",	PASSWD},
	{"account",	ACCOUNT},
	{"machine",	MACH},
	{"macdef",	MACDEF},
	{0,		0}
};

extern char *hostname;
static int token(void);

int ruserpass(const char *host, char **aname, char **apass, char **aacct)
{
	const char *hdir, *mydomain;
    char buf[BUFSIZ], *tmp;
	char myname[MAXHOSTNAMELEN];
	int t, i, c, usedefault = 0;
	struct stat stb;
	extern int errno;

	hdir = getenv("HOME");
	if (hdir == NULL)
		hdir = ".";
	(void) sprintf(buf, "%s/.netrc", hdir);
	cfile = fopen(buf, "r");
	if (cfile == NULL) {
		if (errno != ENOENT)
			perror(buf);
		return(0);
	}


		  if (gethostname(myname, sizeof(myname)) < 0)
		myname[0] = '\0';
	if ((mydomain = index(myname, '.')) == NULL)
		mydomain = "";
next:
	while ((t = token())) switch(t) {

	case DEFAULT:
		usedefault = 1;
		/* FALL THROUGH */

	case MACH:
		if (!usedefault) {
			if (token() != ID)
				continue;
			/*
			 * Allow match either for user's input host name
			 * or official hostname.  Also allow match of
			 * incompletely-specified host in local domain.
			 */
			if (strcasecmp(host, tokval) == 0)
				goto match;
			if (strcasecmp(hostname, tokval) == 0)
				goto match;
			if ((tmp = index(hostname, '.')) != NULL &&
				 strcasecmp(tmp, mydomain) == 0 &&
				 strncasecmp(hostname, tokval, tmp - hostname) == 0 &&
				 tokval[tmp - hostname] == '\0')
				goto match;
			if ((tmp = index(host, '.')) != NULL &&
				 strcasecmp(tmp, mydomain) == 0 &&
				 strncasecmp(host, tokval, tmp - host) == 0 &&
				 tokval[tmp - host] == '\0')
				goto match;
			continue;
		}
	match:
		while ((t = token()) && t != MACH && t != DEFAULT) switch(t) {

		case LOGIN:
			if (token()) {
				if (*aname == 0) {
					*aname = malloc((unsigned) strlen(tokval) + 1);
					(void) strcpy(*aname, tokval);
				} else {
					if (strcmp(*aname, tokval))
						goto next;
				}
			}
			break;
		case PASSWD:
			if (strcmp(*aname, "anonymous") &&
			    fstat(fileno(cfile), &stb) >= 0 &&
			    (stb.st_mode & 077) != 0) {
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove password or correct mode.\n");
				goto bad;
			}
			if (token() && *apass == 0) {
				*apass = malloc((unsigned) strlen(tokval) + 1);
				(void) strcpy(*apass, tokval);
			}
			break;
		case ACCOUNT:
			if (fstat(fileno(cfile), &stb) >= 0
			    && (stb.st_mode & 077) != 0) {
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove account or correct mode.\n");
				goto bad;
			}
			if (token() && *aacct == 0) {
				*aacct = malloc((unsigned) strlen(tokval) + 1);
				(void) strcpy(*aacct, tokval);
			}
			break;
		case MACDEF:
			if (proxy) {
				(void) fclose(cfile);
				return(0);
			}
			while ((c=getc(cfile)) != EOF && (c == ' ' || c == '\t'));
			if (c == EOF || c == '\n') {
				printf("Missing macdef name argument.\n");
				goto bad;
			}
			if (macnum == 16) {
				printf("Limit of 16 macros have already been defined\n");
				goto bad;
			}
			tmp = macros[macnum].mac_name;
			*tmp++ = c;
			for (i=0; i < 8 && (c=getc(cfile)) != EOF &&
			    !isspace(c); ++i) {
				*tmp++ = c;
			}
			if (c == EOF) {
				printf("Macro definition missing null line terminator.\n");
				goto bad;
			}
			*tmp = '\0';
			if (c != '\n') {
				while ((c=getc(cfile)) != EOF && c != '\n');
			}
			if (c == EOF) {
				printf("Macro definition missing null line terminator.\n");
				goto bad;
			}
			if (macnum == 0) {
				macros[macnum].mac_start = macbuf;
			}
			else {
				macros[macnum].mac_start = macros[macnum-1].mac_end + 1;
			}
			tmp = macros[macnum].mac_start;
			while (tmp != macbuf + 4096) {
				if ((c=getc(cfile)) == EOF) {
				printf("Macro definition missing null line terminator.\n");
					goto bad;
				}
				*tmp = c;
				if (*tmp == '\n') {
					if (*(tmp-1) == '\0') {
					   macros[macnum++].mac_end = tmp - 1;
					   break;
					}
					*tmp = '\0';
				}
				tmp++;
			}
			if (tmp == macbuf + 4096) {
				printf("4K macro buffer exceeded\n");
				goto bad;
			}
			break;
		default:
	fprintf(stderr, "Unknown .netrc keyword %s\n", tokval);
			break;
		}
		goto done;
	}
done:
	(void) fclose(cfile);
	return(0);
bad:
	(void) fclose(cfile);
	return(-1);
}

static int token(void)
{
   char *cp;
   int c;
   struct toktab *t;

   if (feof(cfile))
      return (0);
   while ((c = getc(cfile)) != EOF &&
          (c == '\r' || c == '\n' || c == '\t' || c == ' ' || c == ','))
      continue;
   if (c == EOF)
      return (0);
   cp = tokval;
   if (c == '"') {
      while ((c = getc(cfile)) != EOF && c != '"') {
         if (c == '\\')
            c = getc(cfile);
         *cp++ = c;
      }
   } else {
      *cp++ = c;
      while ((c = getc(cfile)) != EOF
             && c != '\n' && c != '\t' && c != ' ' && c != ',' && c != '\r') {
         if (c == '\\')
            c = getc(cfile);
         *cp++ = c;
      }
   }
   *cp = 0;
   if (tokval[0] == 0)
      return (0);
   for (t = toktab; t->tokstr; t++)
      if (!strcmp(t->tokstr, tokval))
         return (t->tval);
   return (ID);
}
