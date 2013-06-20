
#ifndef GETOPT_H
#define GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int opterr;		/* useless, never set or used */
extern int optind;		/* index into parent argv vector */
extern int optopt;			/* character checked for validity */
extern char* optarg;		/* argument associated with option */

int getopt(int argc, char * const *argv, const char *optstring);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* GETOPT_H */

