#ifndef _GETOPT_H
#define _GETOPT_H 1

#include <tchar.h>

#ifdef _UNICODE
#define _toption _woption
#define _toptarg _woptarg
#define _toptind _woptind
#define _topterr _wopterr
#define _toptopt _woptopt
#define _tgetopt _wgetopt
#define _tgetopt_long _wgetopt_long
#define _tgetopt_long_only _wgetopt_long_only
#define _tgetopt_internal _wgetopt_internal
#else
#define _toption option
#define _toptarg optarg
#define _toptind optind
#define _topterr opterr
#define _toptopt optopt
#define _tgetopt getopt
#define _tgetopt_long getopt_long
#define _tgetopt_long_only getopt_long_only
#define _tgetopt_internal _getopt_internal
#endif

#ifdef	__cplusplus
extern "C"
{
#endif

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

extern wchar_t *_woptarg;
extern int _woptind;
extern int _wopterr;
extern int _woptopt;

struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

struct _woption
{
  const wchar_t *name;
  int has_arg;
  int *flag;
  int val;
};

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

extern int getopt (int argc, char *const *argv, const char *shortopts);
extern int getopt_long (int argc, char *const *argv, const char *shortopts,
		        const struct option *longopts, int *longind);
extern int getopt_long_only (int argc, char *const *argv,
			     const char *shortopts, const struct option *longopts, int *longind);

extern int _wgetopt (int argc, wchar_t *const *argv, const wchar_t *shortopts);        
extern int _wgetopt_long (int argc, wchar_t *const *argv, const wchar_t *shortopts,
		          const struct _woption *longopts, int *longind);
extern int _wgetopt_long_only (int argc, wchar_t *const *argv,
			       const wchar_t *shortopts,
		               const struct _woption *longopts, int *longind);

extern int _getopt_internal (int argc, char *const *argv,
			     const char *shortopts, const struct option *longopts, int *longind,
			     int long_only);

extern int _wgetopt_internal (int argc, wchar_t *const *argv,
			       const wchar_t *shortopts,
		               const struct _woption *longopts, int *longind,
			       int long_only);

#ifdef	__cplusplus
}
#endif

#endif /* _GETOPT_H */

