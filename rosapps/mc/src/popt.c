/*
 *
 * Written by Erik Troan (ewt@redhat.com).
 *
 */
#include <config.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#ifdef HAVE_MMAP
#    include <sys/mman.h>
#else
#    include "util.h"
#endif
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#ifdef OS2_NT
#    include <io.h>
#endif

#include "popt.h"

struct optionStackEntry {
    int argc;
    char ** argv;
    int next;
    char * nextArg;
    char * nextCharArg;
    struct poptAlias * currAlias;
};

struct poptContext_s {
    struct optionStackEntry optionStack[POPT_OPTION_DEPTH], * os;
    char ** leftovers;
    int numLeftovers;
    int nextLeftover;
    struct poptOption * options;
    int restLeftover;
    char * appName;
    struct poptAlias * aliases;
    int numAliases;
    int flags;
};

poptContext poptGetContext(char * name ,int argc, char ** argv, 
			   struct poptOption * options, int flags) {
    poptContext con = malloc(sizeof(*con));

    con->os = con->optionStack;
    con->os->argc = argc;
    con->os->argv = argv;
    con->os->currAlias = NULL;
    con->os->nextCharArg = NULL;
    con->os->nextArg = NULL;

    if (flags & POPT_KEEP_FIRST)
	con->os->next = 0;			/* include argv[0] */
    else
	con->os->next = 1;			/* skip argv[0] */

    con->leftovers = malloc(sizeof(char *) * (argc + 1));
    con->numLeftovers = 0;
    con->nextLeftover = 0;
    con->restLeftover = 0;
    con->options = options;
    con->aliases = NULL;
    con->numAliases = 0;
    con->flags = 0;
    
    if (!name)
	con->appName = NULL;
    else
	con->appName = strcpy(malloc(strlen(name) + 1), name);

    return con;
}

void poptResetContext(poptContext con) {
    con->os = con->optionStack;
    con->os->currAlias = NULL;
    con->os->nextCharArg = NULL;
    con->os->nextArg = NULL;
    con->os->next = 1;			/* skip argv[0] */

    con->numLeftovers = 0;
    con->nextLeftover = 0;
    con->restLeftover = 0;
}

/* returns 'val' element, -1 on last item, POPT_ERROR_* on error */
int poptGetNextOpt(poptContext con) {
    char * optString, * chptr, * localOptString;
    char * longArg = NULL;
    char * origOptString;
    long aLong;
    char * end;
    struct poptOption * opt = NULL;
    int done = 0;
    int i;

    while (!done) {
	while (!con->os->nextCharArg && con->os->next == con->os->argc 
		&& con->os > con->optionStack)
	    con->os--;
	if (!con->os->nextCharArg && con->os->next == con->os->argc)
	    return -1;

	if (!con->os->nextCharArg) {
		
	    origOptString = con->os->argv[con->os->next++];

	    if (con->restLeftover || *origOptString != '-') {
		con->leftovers[con->numLeftovers++] = origOptString;
		continue;
	    }

	    if (!origOptString[0])
		return POPT_ERROR_BADOPT;

	    /* Make a copy we can hack at */
	    localOptString = optString = 
			strcpy(malloc(strlen(origOptString) + 1), 
			origOptString);

	    if (optString[1] == '-' && !optString[2]) {
		con->restLeftover = 1;
		free(localOptString);
		continue;
	    } else if (optString[1] == '-') {
		optString += 2;

		if (!con->os->currAlias || !con->os->currAlias->longName || 
		    strcmp(con->os->currAlias->longName, optString)) {

		    i = con->numAliases - 1;
		    while (i >= 0 && (!con->aliases[i].longName ||
			strcmp(con->aliases[i].longName, optString))) i--;

		    if (i >= 0) {
			free(localOptString);
			if ((con->os - con->optionStack + 1) 
				== POPT_OPTION_DEPTH)
			    return POPT_ERROR_OPTSTOODEEP;

			con->os++;
			con->os->next = 0;
			con->os->nextArg = con->os->nextCharArg = NULL;
			con->os->currAlias = con->aliases + i;
			con->os->argc = con->os->currAlias->argc;
			con->os->argv = con->os->currAlias->argv;
			continue;
		    }
		}

		chptr = optString;
		while (*chptr && *chptr != '=') chptr++;
		if (*chptr == '=') {
		    longArg = origOptString + (chptr - localOptString) + 1;
		    *chptr = '\0';
		}

		opt = con->options;
		while (opt->longName || opt->shortName) {
		    if (opt->longName && !strcmp(optString, opt->longName))
			break;
		    opt++;
		}

		if (!opt->longName && !opt->shortName) {
                    free(localOptString);
                    return POPT_ERROR_BADOPT;
                }
	    } else 
		con->os->nextCharArg = origOptString + 1;
    	    free(localOptString);
	}
	
	if (con->os->nextCharArg) {
	    origOptString = con->os->nextCharArg;

	    con->os->nextCharArg = NULL;

	    if (!con->os->currAlias || *origOptString != 
		con->os->currAlias->shortName) {

		i = con->numAliases - 1;
		while (i >= 0 &&
		    con->aliases[i].shortName != *origOptString) i--;

		if (i >= 0) {
		    if ((con->os - con->optionStack + 1) == POPT_OPTION_DEPTH)
			return POPT_ERROR_OPTSTOODEEP;

		    /* We'll need this on the way out */
		    origOptString++;
		    if (*origOptString)
			con->os->nextCharArg = origOptString;

		    con->os++;
		    con->os->next = 0;
		    con->os->nextArg = con->os->nextCharArg = NULL;
		    con->os->currAlias = con->aliases + i;
		    con->os->argc = con->os->currAlias->argc;
		    con->os->argv = con->os->currAlias->argv;
		    continue;
		}
	    }

	    opt = con->options;
	    while ((opt->longName || opt->shortName) && 
		    *origOptString != opt->shortName) opt++;
	    if (!opt->longName && !opt->shortName) return POPT_ERROR_BADOPT;

	    origOptString++;
	    if (*origOptString)
		con->os->nextCharArg = origOptString;
	}

	if (opt->arg && opt->argInfo == POPT_ARG_NONE) 
	    *((int *)opt->arg) = 1;
	else if (opt->argInfo != POPT_ARG_NONE) {
	    if (longArg) {
		con->os->nextArg = longArg;
	    } else if (con->os->nextCharArg) {
		con->os->nextArg = con->os->nextCharArg;
		con->os->nextCharArg = NULL;
	    } else { 
		while (con->os->next == con->os->argc && 
		       con->os > con->optionStack)
		    con->os--;
		if (con->os->next == con->os->argc)
		    return POPT_ERROR_NOARG;

		con->os->nextArg = con->os->argv[con->os->next++];
	    }

	    if (opt->arg) {
		switch (opt->argInfo) {
		  case POPT_ARG_STRING:
		    *((char **) opt->arg) = con->os->nextArg;
		    break;

		  case POPT_ARG_INT:
		  case POPT_ARG_LONG:
		    aLong = strtol(con->os->nextArg, &end, 0);
		    if (*end) 
			return POPT_ERROR_BADNUMBER;

		    if (aLong == LONG_MIN || aLong == LONG_MAX)
			return POPT_ERROR_OVERFLOW;
		    if (opt->argInfo == POPT_ARG_LONG) {
			*((long *) opt->arg) = aLong;
		    } else {
			if (aLong > INT_MAX || aLong < INT_MIN)
			    return POPT_ERROR_OVERFLOW;
			*((int *) opt->arg) =aLong;
		    }
		    break;

		  default:
		    printf("option type not implemented in popt\n");
		    exit(1);
		}
	    }
	}

	if (opt->val) done = 1;
    }

    return opt->val;
}

char * poptGetOptArg(poptContext con) {
    char * ret = con->os->nextArg;
    con->os->nextArg = NULL;
    return ret;
}

char * poptGetArg(poptContext con) {
    if (con->numLeftovers == con->nextLeftover) return NULL;
    return (con->leftovers[con->nextLeftover++]);
}

char * poptPeekArg(poptContext con) {
    if (con->numLeftovers == con->nextLeftover) return NULL;
    return (con->leftovers[con->nextLeftover]);
}

char ** poptGetArgs(poptContext con) {
    if (con->numLeftovers == con->nextLeftover) return NULL;

    /* some apps like [like RPM ;-) ] need this NULL terminated */
    con->leftovers[con->numLeftovers] = NULL;

    return (con->leftovers + con->nextLeftover);
}

void poptFreeContext(poptContext con) {
    int i;

    for (i = 0; i < con->numAliases; i++) {
	free(con->aliases[i].longName);
	free(con->aliases[i].argv);
    }

    free(con->leftovers);
    if (con->appName) free(con->appName);
    if (con->aliases) free(con->aliases);
    free(con);
}

int poptAddAlias(poptContext con, struct poptAlias newAlias, int flags) {
    int aliasNum = con->numAliases++;
    struct poptAlias * alias;

    /* SunOS won't realloc(NULL, ...) */
    if (!con->aliases)
	con->aliases = malloc(sizeof(newAlias) * con->numAliases);
    else
	con->aliases = realloc(con->aliases, 
			       sizeof(newAlias) * con->numAliases);
    alias = con->aliases + aliasNum;
    
    *alias = newAlias;
    if (alias->longName)
	alias->longName = strcpy(malloc(strlen(alias->longName) + 1), 
				    alias->longName);
    else
	alias->longName = NULL;

    return 0;
}

int poptParseArgvString(char * s, int * argcPtr, char *** argvPtr) {
    char * buf = strcpy(malloc(strlen(s) + 1), s);
    char * bufStart = buf;
    char * src, * dst;
    char quote = '\0';
    int argvAlloced = 5;
    char ** argv = malloc(sizeof(*argv) * argvAlloced);
    char ** argv2;
    int argc = 0;
    int i;

    src = s;
    dst = buf;
    argv[argc] = buf;

    memset(buf, '\0', strlen(s) + 1);

    while (*src) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote) {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    free(argv);
		    free(bufStart);
		    return POPT_ERROR_BADQUOTE;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace(*src)) {
	    if (*argv[argc]) {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += 5;
		    argv = realloc(argv, sizeof(*argv) * argvAlloced);
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    break;
	  case '\\':
	    src++;
	    if (!*src) {
		free(argv);
		free(bufStart);
		return POPT_ERROR_BADQUOTE;
	    }
	    /* fallthrough */
	  default:
	    *buf++ = *src;
	}

	src++;
    }

    if (strlen(argv[argc])) {
	argc++;
	buf++;
    }

    argv2 = malloc(argc * sizeof(*argv) + (buf - bufStart));
    dst = (char *)argv2;
    dst += argc * sizeof(*argv);
    memcpy(argv2, argv, argc * sizeof(*argv));
    memcpy(dst, bufStart, buf - bufStart);

    for (i = 0; i < argc; i++) {
	argv2[i] = dst + (argv[i] - bufStart);
    }

    free(argv);
    free(bufStart);
    *argvPtr = argv2;
    *argcPtr = argc;

    return 0;
}

static void configLine(poptContext con, char * line) {
    int nameLength = strlen(con->appName);
    char * opt;
    struct poptAlias alias;
    
    if (strncmp(line, con->appName, nameLength)) return;
    line += nameLength;
    if (!*line || !isspace(*line)) return;
    while (*line && isspace(*line)) line++;

    if (!strncmp(line, "alias", 5)) {
	line += 5;
	if (!*line || !isspace(*line)) return;
	while (*line && isspace(*line)) line++;
	if (!*line) return;

	opt = line;
	while (*line && !isspace(*line)) line++;
	if (!*line) return;
	*line++ = '\0';
	while (*line && isspace(*line)) line++;
	if (!*line) return;

	if (!strlen(opt)) return;

	if (poptParseArgvString(line, &alias.argc, &alias.argv)) return;

	if (opt[0] == '-' && opt[1] == '-') {
	    alias.longName = opt + 2;
	    alias.shortName = '\0';
	    poptAddAlias(con, alias, 0);
	} else if (opt[0] == '-' && !opt[2]) {
	    alias.longName = NULL;
	    alias.shortName = opt[1];
	    poptAddAlias(con, alias, 0);
	}
    }
}

int poptReadConfigFile(poptContext con, char * fn) {
    char * file, * chptr, * end;
    char * buf, * dst;
    int fd, rc;
    int fileLength;

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
	if (errno == ENOENT)
	    return 0;
	else 
	    return POPT_ERROR_ERRNO;
    }

    fileLength = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, 0);

#ifndef HAVE_MMAP
    file = (char*) xmalloc (fileLength, "poptReadConfigFile");
    if (file == NULL)
#else /* HAVE_MMAP */
    file = mmap(NULL, fileLength, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file == (void *) -1)
#endif /* HAVE_MMAP */
    {
	rc = errno;
	close(fd);
	errno = rc;
	return POPT_ERROR_ERRNO;
    }
#ifndef HAVE_MMAP
	do
	{
	    lseek(fd, 0, 0);
		rc = read(fd, file, fileLength);
		if (rc == -1 && errno != EINTR)
		{
			free(file);
			return POPT_ERROR_ERRNO;
		}
	} while (rc != fileLength || (rc == -1 && errno == EINTR));
#endif /* HAVE_MMAP */
    close(fd);

    dst = buf = malloc(fileLength + 1);

    chptr = file;
    end = (file + fileLength);
    while (chptr < end) {
	switch (*chptr) {
	  case '\n':
	    *dst = '\0';
	    dst = buf;
	    while (*dst && isspace(*dst)) dst++;
	    if (*dst && *dst != '#') {
		configLine(con, dst);
	    }
	    chptr++;
	    break;
	  case '\\':
	    *dst++ = *chptr++;
	    if (chptr < end) {
		if (*chptr == '\n') 
		    dst--, chptr++;	
		    /* \ at the end of a line does not insert a \n */
		else
		    *dst++ = *chptr++;
	    }
	    break;
	  default:
	    *dst++ = *chptr++;
	}
    }
    free (buf);
#ifndef HAVE_MMAP
    free (file);
#else
    munmap (file, fileLength);
#endif
    return 0;
}

int poptReadDefaultConfig(poptContext con, int useEnv) {
    char * envName, * envValue, * envValueStart;
    char * fn, * home, * chptr;
    int rc, skip;
    struct poptAlias alias;

    if (!con->appName) return 0;

    rc = poptReadConfigFile(con, "/etc/popt");
    if (rc) return rc;
    if ((home = getenv("HOME"))) {
	fn = (char *) malloc (strlen(home) + 20);
	sprintf(fn, "%s/.popt", home);
	rc = poptReadConfigFile(con, fn);
	free (fn);
	if (rc) return rc;
    }

    envName = malloc(strlen(con->appName) + 20);
    strcpy(envName, con->appName);
    chptr = envName;
    while (*chptr) {
	*chptr = toupper(*chptr);
	chptr++;
    }
    strcat(envName, "_POPT_ALIASES");

    if (useEnv && (envValue = getenv(envName))) {
	envValue = envValueStart = strcpy(malloc(strlen(envValue) + 1), envValue);

	while (envValue && *envValue) {
	    chptr = strchr(envValue, '=');
	    if (!chptr) {
		envValue = strchr(envValue, '\n');
		if (envValue) envValue++;
		continue;
	    }

	    *chptr = '\0';

	    skip = 0;
	    if (!strncmp(envValue, "--", 2)) {
		alias.longName = envValue + 2;
		alias.shortName = '\0';
	    } else if (*envValue == '-' && strlen(envValue) == 2) {
		alias.longName = NULL;	
		alias.shortName = envValue[1];
	    } else {
		skip = 1;
	    }

	    envValue = chptr + 1;
	    chptr = strchr(envValue, '\n');
	    if (chptr) *chptr = '\0';

	    if (!skip) {
		poptParseArgvString(envValue, &alias.argc, &alias.argv);
		poptAddAlias(con, alias, 0);
	    }

	    if (chptr)
		envValue = chptr + 1;
	    else
		envValue = NULL;
	}
	free(envValueStart);
    }

    free (envName);
    return 0;
}

char * poptBadOption(poptContext con, int flags) {
    struct optionStackEntry * os;

    if (flags & POPT_BADOPTION_NOALIAS)
	os = con->optionStack;
    else
	os = con->os;

    return os->argv[os->next - 1];
}

#define POPT_ERROR_NOARG	-10
#define POPT_ERROR_BADOPT	-11
#define POPT_ERROR_OPTSTOODEEP	-13
#define POPT_ERROR_BADQUOTE	-15	/* only from poptParseArgString() */
#define POPT_ERROR_ERRNO	-16	/* only from poptParseArgString() */

const char * poptStrerror(const int error) {
    switch (error) {
      case POPT_ERROR_NOARG:
	return "missing argument";
      case POPT_ERROR_BADOPT:
	return "unknown option";
      case POPT_ERROR_OPTSTOODEEP:
	return "aliases nested too deeply";
      case POPT_ERROR_BADQUOTE:
	return "error in paramter quoting";
      case POPT_ERROR_BADNUMBER:
	return "invalid numeric value";
      case POPT_ERROR_OVERFLOW:
	return "number too large or too small";
      case POPT_ERROR_ERRNO:
#ifdef HAVE_STRERROR
        return strerror (errno);
#else
      {
        extern int sys_nerr;
        extern char *sys_errlist [];
        if ((0 <= errno) && (errno < sys_nerr))
          return sys_errlist[errno];
        else
          return "strange errno";
      }
#endif
      default:
	return "unknown error";
    }
}

int poptStuffArgs(poptContext con, char ** argv) {
    int i;

    if ((con->os - con->optionStack) == POPT_OPTION_DEPTH)
	return POPT_ERROR_OPTSTOODEEP;

    for (i = 0; argv[i]; i++);

    con->os++;
    con->os->next = 0;
    con->os->nextArg = con->os->nextCharArg = NULL;
    con->os->currAlias = NULL;
    con->os->argc = i;
    con->os->argv = argv;

    return 0;
}
