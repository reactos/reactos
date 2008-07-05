/*
 * $Id$
 * This is an unpublished work copyright (c) 1998 HELIOS Software GmbH
 * 30827 Garbsen, Germany
 */


#include <stdio.h>
#include <string.h>
#ifdef HAS_UNISTD
# include <unistd.h>
#endif

char *optarg;
int optind = 1;
int opterr = 1;
int optopt;
static int subopt;
static int suboptind = 1;

int getopt(int argc, char *const argv[], const char * optstring)
{
	char *curopt;
	char *p;
	int cursubopt;

	if (suboptind == optind-1 && argv[suboptind][subopt] != '\0') {
		curopt = (char *)argv[suboptind];
	} else {
		curopt = (char *)argv[optind];
		if (curopt == NULL || curopt[0] != '-' || strcmp(curopt, "-") == 0)
			return -1;
		suboptind = optind;
		subopt = 1;
		optind++;
		if (strcmp(curopt, "--") == 0)
			return -1;
	}
	cursubopt = subopt++;
	if ((p = strchr(optstring, curopt[cursubopt])) == NULL) {
		optopt = curopt[cursubopt];
		if (opterr)
			fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
		return '?';
	}
	if (p[1] == ':') {
		if (curopt[cursubopt+1] != '\0') {
			optarg = curopt+cursubopt+1;
			suboptind++;
			return p[0];
		}
		if (argv[optind] == NULL) {
			optopt = p[0];
			if (opterr)
				fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], optopt);
			if (*optstring == ':')
				return ':';
			return '?';
		}
		optarg = argv[optind++];
	}
	return p[0];
}
