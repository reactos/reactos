
/* jgaa oct 9th 2000: Found this on www. 
 * No copyright information given. 
 * Slightly modidied.
 *
 * Origin: http://www.winsite.com/info/pc/win3/winsock/sossntr4.zip/SOSSNT/SRC/GETOPT.C.html
 */
 
 /* got this off net.sources  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"


/*
* get option letter from argument vector
*/
int	opterr = 1,		/* useless, never set or used */
optind = 1,		/* index into parent argv vector */
optopt;			/* character checked for validity */
char* optarg;		/* argument associated with option */

#define BADCH	(int)'?'
#define EMSG	""
#define tell(s)	fputs(*argv,stderr);fputs(s,stderr); \
	fputc(optopt,stderr);fputc('\n',stderr);return(BADCH);

int getopt(int argc, char * const *argv, const char *optstring)
{
	static char	*place = EMSG;	/* option letter processing */
	register char	*oli;		/* option letter list index */
	
	if(!*place) {			/* update scanning pointer */
		if(optind >= argc || *(place = argv[optind]) != '-' || !*++place) 
			return(EOF);
		if (*place == '-') {	/* found "--" */
			++optind;
			return(EOF);
		}
	}				/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(optstring,optopt))) 
	{
		if(!*place) ++optind;
		tell(": illegal option -- ");
	}
	if (*++oli != ':') {		/* don't need argument */
		optarg = NULL;
		if (!*place) ++optind;
	}
	else {				/* need an argument */
		if (*place) optarg = place;	/* no white space */
		else if (argc <= ++optind) {	/* no arg */
			place = EMSG;
			tell(": option requires an argument -- ");
		}
		else optarg = argv[optind];	/* white space */
		place = EMSG;
		++optind;
	}
	return(optopt);			/* dump back option letter */
}
