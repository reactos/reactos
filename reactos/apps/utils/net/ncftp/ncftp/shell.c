/* shell.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "shell.h"
#include "util.h"
#include "bookmark.h"
#include "cmds.h"
#include "readln.h"
#include "trace.h"
#include "main.h"

/* We keep running the command line interpreter until gDoneApplication
 * is non-zero.
 */
int gDoneApplication = 0;

/* Track how many times they use ^C. */
int gNumInterruptions = 0;

/* Keep a count of the number of commands the user has entered. */
int gEventNumber = 0;


#if defined(WIN32) || defined(_WINDOWS)
#elif defined(HAVE_SIGSETJMP)
/* A command function can set this to have a user generated signal
 * cause execution to jump here.
 */
sigjmp_buf gCancelJmp;

/* This is used by the shell so that an unexpected signal can have
 * execution come back to the main shell prompt.
 */
sigjmp_buf gBackToTopJmp;
#else	/* HAVE_SIGSETJMP */
/* A command function can set this to have a user generated signal
 * cause execution to jump here.
 */
jmp_buf gCancelJmp;

/* This is used by the shell so that an unexpected signal can have
 * execution come back to the main shell prompt.
 */
jmp_buf gBackToTopJmp;
#endif	/* HAVE_SIGSETJMP */

/* Flag specifying whether the jmp has been set. */
int gMayCancelJmp = 0;

/* Flag specifying whether the jmp has been set. */
int gMayBackToTopJmp = 0;

/* Save the last signal number. */
int gGotSig = 0;

/* If the shell is running one of our commands, this is set to non-zero. */
int gRunningCommand = 0;

/* If set, we need to abort the current session. */
int gCancelCtrl = 0;

extern Command gCommands[];
extern size_t gNumCommands;
extern int gStartupUrlParameterGiven;
extern FTPLibraryInfo gLib;
extern FTPConnectionInfo gConn;
extern LineList gStartupURLCdList;
extern int gNumProgramRuns;
extern char gCopyright[];


/* This is used as the comparison function when we sort the name list. */
static int
CommandSortCmp(const CommandPtr a, const CommandPtr b)
{
	return (strcmp((*a).name, (*b).name));
}	/* CommandSortCmp */




/* Sort the command list, in case it wasn't hard-coded that way. */
void
InitCommandList(void)
{
	qsort(gCommands, gNumCommands, sizeof(Command), (qsort_proc_t) CommandSortCmp);
}	/* InitCommandList */




/* This is used as the comparison function when we lookup something
 * in the command list, and when we want an exact match.
 */
static int
CommandExactSearchCmp(const char *const key, const CommandPtr b)
{
	return (strcmp(key, (*b).name));
}	/* CommandExactSearchCmp */




/* This is used as the comparison function when we lookup something
 * in the command list, and when the key can be just the first few
 * letters of one or more commands.  So a key of "qu" might would match
 * "quit" and "quote" for example.
 */
static int
CommandSubSearchCmp(const char *const key, const CommandPtr a)
{
	register const char *kcp, *cp;
	int d;

	for (cp = (*a).name, kcp = key; ; ) {
		if (*kcp == 0)
			break;
		d = *kcp++ - *cp++;
		if (d)
			return d;
	}
	return (0);
}	/* CommandSubSearchCmp */




/* This returns a pointer to a Command, if the name supplied was long
 * enough to be a unique name.  We return a 0 CommandPtr if we did not
 * find any matches, a -1 CommandPtr if we found more than one match,
 * or the unique CommandPtr.
 */
CommandPtr
GetCommandByIndex(const int i)
{
	if ((i < 0) || (i >= (int) gNumCommands))
		return (kNoCommand);
	return (&gCommands[i]);
}									   /* GetCommandByIndex */




/* This returns a pointer to a Command, if the name supplied was long
 * enough to be a unique name.  We return a 0 CommandPtr if we did not
 * find any matches, a -1 CommandPtr if we found more than one match,
 * or the unique CommandPtr.
 */
CommandPtr
GetCommandByName(const char *const name, int wantExactMatch)
{
	CommandPtr canp, canp2;

	/* First check for an exact match.  Otherwise if you if asked for
	 * 'cd', it would match both 'cd' and 'cdup' and return an
	 * ambiguous name error, despite having the exact name for 'cd.'
	 */
	canp = (CommandPtr) bsearch(name, gCommands, gNumCommands, sizeof(Command), (bsearch_proc_t) CommandExactSearchCmp);

	if (canp == kNoCommand && !wantExactMatch) {
		/* Now see if the user typed an abbreviation unique enough
		 * to match only one name in the list.
		 */
		canp = (CommandPtr) bsearch(name, gCommands, gNumCommands, sizeof(Command), (bsearch_proc_t) CommandSubSearchCmp);
		
		if (canp != kNoCommand) {
			/* Check the entry above us and see if the name we're looking
			 * for would match that, too.
			 */
			if (canp != &gCommands[0]) {
				canp2 = canp - 1;
				if (CommandSubSearchCmp(name, canp2) == 0)
					return kAmbiguousCommand;
			}
			/* Check the entry below us and see if the name we're looking
			 * for would match that one.
			 */
			if (canp != &gCommands[gNumCommands - 1]) {
				canp2 = canp + 1;
				if (CommandSubSearchCmp(name, canp2) == 0)
					return kAmbiguousCommand;
			}
		}
	}
	return canp;
}									   /* GetCommandByName */




/* Print the help string for the command specified. */

void
PrintCmdHelp(CommandPtr c)
{
	(void) printf("%s: %s.\n", c->name, c->help);
}									   /* PrintCmdHelp */




/* Print the usage string for the command specified. */
void
PrintCmdUsage(CommandPtr c)
{
	if (c->usage != NULL)
		(void) printf("Usage: %s %s\n", c->name, c->usage);
}									   /* PrintCmdUsage */




/* Parse a command line into an array of arguments. */
int
MakeArgv(char *line, int *cargc, const char **cargv, int cargcmax, char *dbuf, size_t dbufsize, int *noglobargv, int readlineHacks)
{
	int c;
	int retval;
	char *dlim;
	char *dcp;
	char *scp;
	char *arg;

	*cargc = 0;
	scp = line;
	dlim = dbuf + dbufsize - 1;
	dcp = dbuf;

	for (*cargc = 0; *cargc < cargcmax; ) {
		/* Eat preceding junk. */
		for ( ; ; scp++) {
			c = *scp;
			if (c == '\0')
				goto done;
			if (isspace(c))
				continue;
			if ((c == ';') || (c == '\n')) {
				scp++;
				goto done;
			}
			break;
		}

		arg = dcp;
		cargv[*cargc] = arg;
		noglobargv[*cargc] = 0;
		(*cargc)++;

		/* Special hack so that "!cmd" is always split into "!" "cmd" */
		if ((*cargc == 1) && (*scp == '!')) {
			if (scp[1] == '!') {
				scp[1] = '\0';
			} else if ((scp[1] != '\0') && (!isspace((int) scp[1]))) {
				cargv[0] = "!";
				scp++;
				arg = dcp;
				cargv[*cargc] = arg;
				noglobargv[*cargc] = 0;
				(*cargc)++;
			}
		}

		/* Add characters to the new argument. */
		for ( ; ; ) {
			c = *scp;
			if (c == '\0')
				break;
			if (isspace(c))
				break;
			if ((c == ';') || (c == '\n')) {
				break;
			}

			scp++;

			if (c == '\'') {
				for ( ; ; ) {
					c = *scp++;
					if (c == '\0') {
						if (readlineHacks != 0)
							break;
						/* Syntax error */
						(void) fprintf(stderr, "Error: Unbalanced quotes.\n");
						return (-1);
					}
					if (c == '\'')
						break;

					/* Add char. */
					if (dcp >= dlim)
						goto toolong;
					*dcp++ = c;

					if (strchr(kGlobChars, c) != NULL) {
						/* User quoted glob characters,
						 * so mark this argument for
						 * noglob.
						 */
						noglobargv[*cargc - 1] = 1;
					}
				}
			} else if (c == '"') {
				for ( ; ; ) {
					c = *scp++;
					if (c == '\0') {
						if (readlineHacks != 0)
							break;
						/* Syntax error */
						(void) fprintf(stderr, "Error: Unbalanced quotes.\n");
						return (-1);
					}
					if (c == '"')
						break;

					/* Add char. */
					if (dcp >= dlim)
						goto toolong;
					*dcp++ = c;

					if (strchr(kGlobChars, c) != NULL) {
						/* User quoted glob characters,
						 * so mark this argument for
						 * noglob.
						 */
						noglobargv[*cargc - 1] = 1;
					}
				}
			} else
#if defined(WIN32) || defined(_WINDOWS)
				if (c == '|') {
#else
				if (c == '\\') {
#endif
				/* Add next character, verbatim. */
				c = *scp++;
				if (c == '\0')
					break;

				/* Add char. */
				if (dcp >= dlim)
					goto toolong;
				*dcp++ = c;
			} else {
				/* Add char. */
				if (dcp >= dlim)
					goto toolong;
				*dcp++ = c;
			}
		}

		*dcp++ = '\0';
	}

	(void) fprintf(stderr, "Error: Argument list too long.\n");
	*cargc = 0;
	cargv[*cargc] = NULL;
	return (-1);

done:
	retval = (int) (scp - line);
	cargv[*cargc] = NULL;
	return (retval);

toolong:
	(void) fprintf(stderr, "Error: Line too long.\n");
	*cargc = 0;
	cargv[*cargc] = NULL;
	return (-1);
}	/* MakeArgv */




static int
DoCommand(const ArgvInfoPtr aip)
{
	CommandPtr cmdp;
	int flags;
	int cargc, cargcm1;

	cmdp = GetCommandByName(aip->cargv[0], 0);
	if (cmdp == kAmbiguousCommand) {
		(void) printf("%s: ambiguous command name.\n", aip->cargv[0]);
		return (-1);
	} else if (cmdp == kNoCommand) {
		(void) printf("%s: no such command.\n", aip->cargv[0]);
		return (-1);
	}

	cargc = aip->cargc;
	cargcm1 = cargc - 1;
	flags = cmdp->flags;

	if (((flags & kCmdMustBeConnected) != 0) && (gConn.connected == 0)) {
		(void) printf("%s: must be connected to do that.\n", aip->cargv[0]);
	} else if (((flags & kCmdMustBeDisconnected) != 0) && (gConn.connected != 0)) {
		(void) printf("%s: must be disconnected to do that.\n", aip->cargv[0]);
	} else if ((cmdp->minargs != kNoMin) && (cmdp->minargs > cargcm1)) {
		PrintCmdUsage(cmdp);
	} else if ((cmdp->maxargs != kNoMax) && (cmdp->maxargs < cargcm1)) {
		PrintCmdUsage(cmdp);
	} else {
		(*cmdp->proc)(cargc, aip->cargv, cmdp, aip);
	}
	return (0);
}	/* DoCommand */




/* Allows the user to cancel a data transfer. */
void
XferCanceller(int sigNum)
{
	gGotSig = sigNum;
	if (gConn.cancelXfer > 0) {
#if defined(WIN32) || defined(_WINDOWS)
		signal(SIGINT, SIG_DFL);
#else
		/* User already tried it once, they
		 * must think it's locked up.
		 *
		 * Jump back to the top, and
		 * close down the current session.
		 */
		gCancelCtrl = 1;
		if (gMayBackToTopJmp > 0) {
#ifdef HAVE_SIGSETJMP
			siglongjmp(gBackToTopJmp, 1);
#else	/* HAVE_SIGSETJMP */
			longjmp(gBackToTopJmp, 1);
#endif	/* HAVE_SIGSETJMP */
		}
#endif
	}
	gConn.cancelXfer++;
}	/* XferCanceller */



#if defined(WIN32) || defined(_WINDOWS)
#else

/* Allows the user to cancel a long operation and get back to the shell. */
void
BackToTop(int sigNum)
{
	gGotSig = sigNum;
	if (sigNum == SIGPIPE) {
		if (gRunningCommand == 1) {
			(void) fprintf(stderr, "Unexpected broken pipe.\n");
			gRunningCommand = 0;
		} else {
			SetXtermTitle("RESTORE");
			exit(1);
		}
	} else if (sigNum == SIGINT) {
		if (gRunningCommand == 0)
			gDoneApplication = 1;
	}
	if (gMayBackToTopJmp > 0) {
#ifdef HAVE_SIGSETJMP
		siglongjmp(gBackToTopJmp, 1);
#else	/* HAVE_SIGSETJMP */
		longjmp(gBackToTopJmp, 1);
#endif	/* HAVE_SIGSETJMP */
	}
}	/* BackToTop */




/* Some commands may want to jump back to the start too. */
void
Cancel(int sigNum)
{
	if (gMayCancelJmp != 0) {
		gGotSig = sigNum;
		gMayCancelJmp = 0;
#ifdef HAVE_SIGSETJMP
		siglongjmp(gCancelJmp, 1);
#else	/* HAVE_SIGSETJMP */
		longjmp(gCancelJmp, 1);
#endif	/* HAVE_SIGSETJMP */
	}
}	/* Cancel */

#endif



void
CommandShell(void)
{
	int tUsed, bUsed;
	ArgvInfo ai;
	char prompt[64];
	char *lineRead;
#if defined(WIN32) || defined(_WINDOWS)
#else
	int sj;
#endif
	time_t cmdStart, cmdStop;

	/* Execution may jump back to this point to restart the shell. */
#if defined(WIN32) || defined(_WINDOWS)

#elif defined(HAVE_SIGSETJMP)
	sj = sigsetjmp(gBackToTopJmp, 1);
#else	/* HAVE_SIGSETJMP */
	sj = setjmp(gBackToTopJmp);
#endif	/* HAVE_SIGSETJMP */

#if defined(WIN32) || defined(_WINDOWS)
#else
	if (sj != 0) {
		Trace(0, "Caught signal %d, back at top.\n", gGotSig);
		if (gGotSig == SIGALRM) {
			(void) printf("\nRemote host was not responding, closing down the session.");
			FTPShutdownHost(&gConn);
		} else{
			(void) printf("\nInterrupted.\n");
			if (gCancelCtrl != 0) {
				gCancelCtrl = 0;
				(void) printf("Closing down the current FTP session: ");
				FTPShutdownHost(&gConn);
				(void) sleep(1);
				(void) printf("done.\n");
			}
		}
	}

	gMayBackToTopJmp = 1;
#endif


	++gEventNumber;

	while (gDoneApplication == 0) {
#if defined(WIN32) || defined(_WINDOWS)
#else
		(void) NcSignal(SIGINT, BackToTop);
		(void) NcSignal(SIGPIPE, BackToTop);
		(void) NcSignal(SIGALRM, BackToTop);
#endif

		MakePrompt(prompt, sizeof(prompt));

		if (gConn.connected == 0) {
			SetXtermTitle("DEFAULT");
		} else {
			SetXtermTitle("%s - NcFTP", gConn.host);
		}

		lineRead = Readline(prompt);
		if (lineRead == NULL) {
			/* EOF, Control-D */
			(void) printf("\n");
			break;
		}
		Trace(0, "> %s\n", lineRead);
		AddHistory(lineRead);
		for (tUsed = 0;;) {
			(void) memset(&ai, 0, sizeof(ai));
			bUsed = MakeArgv(lineRead + tUsed, &ai.cargc, ai.cargv,
				(int) (sizeof(ai.cargv) / sizeof(char *)),
				ai.argbuf, sizeof(ai.argbuf),
				ai.noglobargv, 0);
			if (bUsed <= 0)
				break;
			tUsed += bUsed;	
			if (ai.cargc == 0)
				continue;
			gRunningCommand = 1;
			(void) time(&cmdStart);
			if (DoCommand(&ai) < 0) {
				(void) time(&cmdStop);
				gRunningCommand = 0;
				break;
			}
			(void) time(&cmdStop);
			gRunningCommand = 0;
			if ((cmdStop - cmdStart) > kBeepAfterCmdTime) {
				/* Let the user know that a time-consuming
				 * operation has completed.
				 */
#if defined(WIN32) || defined(_WINDOWS)
				MessageBeep(MB_OK);
#else
				(void) fprintf(stderr, "\007");
#endif
			}
			++gEventNumber;
		}

		free(lineRead);
	}

	CloseHost();
	gMayBackToTopJmp = 0;
}	/* Shell */
