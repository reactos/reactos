#ifndef H_POPT
#define H_POPT

#define POPT_OPTION_DEPTH	10

#define POPT_ARG_NONE		0
#define POPT_ARG_STRING		1
#define POPT_ARG_INT		2
#define POPT_ARG_LONG		3

#define POPT_ERROR_NOARG	-10
#define POPT_ERROR_BADOPT	-11
#define POPT_ERROR_OPTSTOODEEP	-13
#define POPT_ERROR_BADQUOTE	-15	/* only from poptParseArgString() */
#define POPT_ERROR_ERRNO	-16	/* only from poptParseArgString() */
#define POPT_ERROR_BADNUMBER	-17
#define POPT_ERROR_OVERFLOW	-18

/* context creation flags */
#define POPT_BADOPTION_NOALIAS  (1 << 0)  /* don't go into an alias */
#define POPT_KEEP_FIRST		(1 << 1)  /* pay attention to argv[0] */

struct poptOption {
    const char * longName;	/* may be NULL */
    char shortName;		/* may be '\0' */
    int argInfo;
    void * arg;			/* depends on argInfo */
    int val;			/* 0 means don't return, just update flag */
};

struct poptAlias {
    char * longName;		/* may be NULL */
    char shortName;		/* may be '\0' */
    int argc;
    char ** argv;		/* must be free()able */
};

typedef struct poptContext_s * poptContext;

poptContext poptGetContext(char * name, int argc, char ** argv, 
			   struct poptOption * options, int flags);
void poptResetContext(poptContext con);

/* returns 'val' element, -1 on last item, POPT_ERROR_* on error */
int poptGetNextOpt(poptContext con);
/* returns NULL if no argument is available */
char * poptGetOptArg(poptContext con);
/* returns NULL if no more options are available */
char * poptGetArg(poptContext con);
char * poptPeekArg(poptContext con);
char ** poptGetArgs(poptContext con);
/* returns the option which caused the most recent error */
char * poptBadOption(poptContext con, int flags);
void poptFreeContext(poptContext con);
int poptStuffArgs(poptContext con, char ** argv);
int poptAddAlias(poptContext con, struct poptAlias alias, int flags);
int poptReadConfigFile(poptContext con, char * fn);
/* like above, but reads /etc/popt and $HOME/.popt along with environment 
   vars */
int poptReadDefaultConfig(poptContext con, int useEnv);
/* argv should be freed -- this allows ', ", and \ quoting, but ' is treated
   the same as " and both may include \ quotes */
int poptParseArgvString(char * s, int * argcPtr, char *** argvPtr);
const char * poptStrerror(const int error);

#endif
