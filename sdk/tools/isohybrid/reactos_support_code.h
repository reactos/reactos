#ifndef _REACTOS_SUPPORT_CODE_H
#define _REACTOS_SUPPORT_CODE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#else
#include <alloca.h>
#include <unistd.h>
#endif

// isotypes.h would provide these, but it's not available on MSVC < 2013.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

void isohybrid_error(int eval, const char* fmt, ...);
void isohybrid_warning(const char* fmt, ...);

#define err(...)   isohybrid_error(__VA_ARGS__)
#define errx(...)  isohybrid_error(__VA_ARGS__)
#define warn(...)  isohybrid_warning(__VA_ARGS__)
#define warnx(...) isohybrid_warning(__VA_ARGS__)


/////////////////////////////////////////////////////////////////////////////
// getopt code from mingw-w64
/////////////////////////////////////////////////////////////////////////////
extern int optopt;		/* single option character, as parsed     */
extern char *optarg;		/* pointer to argument of current option  */

struct option		/* specification for a long form option...	*/
{
    const char *name;		/* option name, without leading hyphens */
    int         has_arg;		/* does it take an argument?		*/
    int        *flag;		/* where to save its status, or NULL	*/
    int         val;		/* its associated status value		*/
};

enum    		/* permitted values for its `has_arg' field...	*/
{
    no_argument = 0,      	/* option never takes an argument	*/
    required_argument,		/* option always requires an argument	*/
    optional_argument		/* option may take an argument		*/
};

int getopt_long_only(int nargc, char * const *nargv, const char *options, const struct option *long_options, int *idx);
/////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
int fsync(int fd);
int getppid(void);
#endif

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftruncate _chsize
#endif

#endif
