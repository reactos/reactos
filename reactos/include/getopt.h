/* temporary header for getopt. Please remove this file when MingW ships with
   its own */

#ifndef __GETOPT_H_INCLUDED
#define __GETOPT_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

extern char *optarg;
extern int optind, opterr, optopt;

#define no_argument       (0)
#define required_argument (1)
#define optional_argument (2)

struct option
{
 const char *name;
 int has_arg;
 int *flag;
 int val;
}; 

extern int getopt(int, char * const [], const char *);

extern int getopt_long
(
 int,
 char * const [],
 const char *,
 const struct option *,
 int *
);
 
extern int getopt_long_only
(
 int,
 char * const [],
 const char *,
 const struct option *,
 int *
);

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_H_INCLUDED */

/* EOF */
