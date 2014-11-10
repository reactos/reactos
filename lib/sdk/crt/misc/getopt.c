#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>

#define REPLACE_GETOPT

#define _DIAGASSERT(x) do {} while (0)

#ifdef REPLACE_GETOPT
#ifdef __weak_alias
__weak_alias(getopt,_getopt)
#endif
int opterr = 1;
int optind = 1;
int optopt = '?';
int optreset;
char *optarg;
#endif

#ifdef __weak_alias
__weak_alias(getopt_long,_getopt_long)
#endif

#ifndef __CYGWIN__
#define __progname __argv[0]
#else
extern char __declspec(dllimport) *__progname;
#endif

#define IGNORE_FIRST (*options == '-' || *options == '+')
#define PRINT_ERROR ((opterr) && ((*options != ':') || (IGNORE_FIRST && options[1] != ':')))

#ifndef IS_POSIXLY_CORRECT
#define IS_POSIXLY_CORRECT (getenv("POSIXLY_CORRECT") != NULL)
#endif

#define PERMUTE (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)

#define IN_ORDER (!IS_POSIXLY_CORRECT && *options == '-')

#define BADCH (int)'?'
#define BADARG ((IGNORE_FIRST && options[1] == ':') || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

static char EMSG[1];

static int getopt_internal (int,char * const *,const char *);
static int gcd (int,int);
static void permute_args (int,int,int,char * const *);

static char *place = EMSG;

static int nonopt_start = -1;
static int nonopt_end = -1;

static const char recargchar[] = "option requires an argument -- %c";
static const char recargstring[] = "option requires an argument -- %s";
static const char ambig[] = "ambiguous option -- %.*s";
static const char noarg[] = "option doesn't take an argument -- %.*s";
static const char illoptchar[] = "unknown option -- %c";
static const char illoptstring[] = "unknown option -- %s";

static void
_vwarnx(const char *fmt,va_list ap)
{
  (void)fprintf(stderr,"%s: ",__progname);
  if (fmt != NULL)
    (void)vfprintf(stderr,fmt,ap);
  (void)fprintf(stderr,"\n");
}

static void
warnx(const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  _vwarnx(fmt,ap);
  va_end(ap);
}

static int
gcd(a,b)
	int a;
	int b;
{
	int c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}

	return b;
}

static void
permute_args(panonopt_start,panonopt_end,opt_end,nargv)
	int panonopt_start;
	int panonopt_end;
	int opt_end;
	char * const *nargv;
{
	int cstart,cyclelen,i,j,ncycle,nnonopts,nopts,pos;
	char *swap;

	_DIAGASSERT(nargv != NULL);

	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts,nopts);
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end+i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end)
				pos -= nnonopts;
			else
				pos += nopts;
			swap = nargv[pos];

			((char **) nargv)[pos] = nargv[cstart];

			((char **)nargv)[cstart] = swap;
		}
	}
}

static int
getopt_internal(nargc,nargv,options)
	int nargc;
	char * const *nargv;
	const char *options;
{
	char *oli;
	int optchar;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);

	optarg = NULL;

	if (optind == 0)
		optind = 1;

	if (optreset)
		nonopt_start = nonopt_end = -1;
start:
	if (optreset || !*place) {
		optreset = 0;
		if (optind >= nargc) {
			place = EMSG;
			if (nonopt_end != -1) {

				permute_args(nonopt_start,nonopt_end,optind,nargv);
				optind -= nonopt_end - nonopt_start;
			}
			else if (nonopt_start != -1) {

				optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((*(place = nargv[optind]) != '-')
		    || (place[1] == '\0')) {
			place = EMSG;
			if (IN_ORDER) {

				optarg = nargv[optind++];
				return INORDER;
			}
			if (!PERMUTE) {

				return -1;
			}

			if (nonopt_start == -1)
				nonopt_start = optind;
			else if (nonopt_end != -1) {
				permute_args(nonopt_start,nonopt_end,optind,nargv);
				nonopt_start = optind -
				    (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			optind++;

			goto start;
		}
		if (nonopt_start != -1 && nonopt_end == -1)
			nonopt_end = optind;
		if (place[1] && *++place == '-') {
			place++;
			return -2;
		}
	}
	if ((optchar = (int)*place++) == (int)':' ||
	    (oli = strchr(options + (IGNORE_FIRST ? 1 : 0),optchar)) == NULL) {

		if (!*place)
			++optind;
		if (PRINT_ERROR)
			warnx(illoptchar,optchar);
		optopt = optchar;
		return BADCH;
	}
	if (optchar == 'W' && oli[1] == ';') {

		if (*place)
			return -2;

		if (++optind >= nargc) {
			place = EMSG;
			if (PRINT_ERROR)
				warnx(recargchar,optchar);
			optopt = optchar;
			return BADARG;
		} else
			place = nargv[optind];

		return -2;
	}
	if (*++oli != ':') {
		if (!*place)
			++optind;
	} else {
		optarg = NULL;
		if (*place)
			optarg = place;

		else if (oli[1] != ':') {
			if (++optind >= nargc) {
				place = EMSG;
				if (PRINT_ERROR)
					warnx(recargchar,optchar);
				optopt = optchar;
				return BADARG;
			} else
				optarg = nargv[optind];
		}
		place = EMSG;
		++optind;
	}

	return optchar;
}

#ifdef REPLACE_GETOPT

int
getopt(nargc,nargv,options)
	int nargc;
	char * const *nargv;
	const char *options;
{
	int retval;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);

	if ((retval = getopt_internal(nargc,nargv,options)) == -2) {
		++optind;

		if (nonopt_end != -1) {
			permute_args(nonopt_start,nonopt_end,optind,nargv);
			optind -= nonopt_end - nonopt_start;
		}
		nonopt_start = nonopt_end = -1;
		retval = -1;
	}
	return retval;
}
#endif

int
getopt_long(nargc,nargv,options,long_options,idx)
	int nargc;
	char * const *nargv;
	const char *options;
	const struct option *long_options;
	int *idx;
{
	int retval;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);
	_DIAGASSERT(long_options != NULL);

	if ((retval = getopt_internal(nargc,nargv,options)) == -2) {
		char *current_argv,*has_equal;
		size_t current_argv_len;
		int i,match;

		current_argv = place;
		match = -1;

		optind++;
		place = EMSG;

		if (*current_argv == '\0') {

			if (nonopt_end != -1) {
				permute_args(nonopt_start,nonopt_end,optind,nargv);
				optind -= nonopt_end - nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((has_equal = strchr(current_argv,'=')) != NULL) {

			current_argv_len = has_equal - current_argv;
			has_equal++;
		} else
			current_argv_len = strlen(current_argv);

		for (i = 0; long_options[i].name; i++) {

			if (strncmp(current_argv,long_options[i].name,current_argv_len))
				continue;

			if (strlen(long_options[i].name) ==
			    (unsigned)current_argv_len) {

				match = i;
				break;
			}
			if (match == -1)
				match = i;
			else {

				if (PRINT_ERROR)
					warnx(ambig,(int)current_argv_len,current_argv);
				optopt = 0;
				return BADCH;
			}
		}
		if (match != -1) {
			if (long_options[match].has_arg == no_argument
			    && has_equal) {
				if (PRINT_ERROR)
					warnx(noarg,(int)current_argv_len,current_argv);

				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				return BADARG;
			}
			if (long_options[match].has_arg == required_argument ||
			    long_options[match].has_arg == optional_argument) {
				if (has_equal)
					optarg = has_equal;
				else if (long_options[match].has_arg ==
				    required_argument) {

					optarg = nargv[optind++];
				}
			}
			if ((long_options[match].has_arg == required_argument)
			    && (optarg == NULL)) {

				if (PRINT_ERROR)
					warnx(recargstring,current_argv);

				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				--optind;
				return BADARG;
			}
		} else {
			if (PRINT_ERROR)
				warnx(illoptstring,current_argv);
			optopt = 0;
			return BADCH;
		}
		if (long_options[match].flag) {
			*long_options[match].flag = long_options[match].val;
			retval = 0;
		} else
			retval = long_options[match].val;
		if (idx)
			*idx = match;
	}
	return retval;
}
