/* getopt.h */

extern char *optarg;
extern int optind;

int
getopt(int nargc, char * const *nargv, const char *ostr);
