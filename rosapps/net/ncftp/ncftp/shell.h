/* shell.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

typedef struct ArgvInfo {
	const char *cargv[64];
	int noglobargv[64];
	int cargc;
	char argbuf[256];
} ArgvInfo, *ArgvInfoPtr;

/* How often to no-op the remote site if the user is idle, in seconds. */
#define kIdleInterval 20

/* If the user has been idle this many seconds, start their background
 * jobs.
 */
#define kIdleBatchLaunch 180

/* If a command (like a transfer) took longer than this many seconds, beep
 * at the user to notify them that it completed.
 */
#define kBeepAfterCmdTime 15

typedef struct Command *CommandPtr;
typedef void (*CmdProc)(const int argc, const char **const argv, const CommandPtr cmdp, const ArgvInfoPtr aip);

/* These are used in the command table, to specify that a command
 * doesn't require an exact number of parameters.
 */
#define kNoMax (-1)
#define kNoMin (-1)

/* Structure of the command table.  We keep some extra stuff in the
 * table, so each command doesn't have to check the number of
 * arguments and print it's own usage messages if it doesn't want to.
 */
typedef struct Command {
	const char *name;
	CmdProc proc;
	const char *usage, *help;
	int flags;
	int minargs, maxargs;
} Command;

/* Parameter to GetCommandOrMacro(). */
#define kAbbreviatedMatchAllowed 0
#define kExactMatchRequired 1

/* These can be returned by the GetCommand() routine. */
#define kAmbiguousCommand ((CommandPtr) -1)
#define kNoCommand ((CommandPtr) 0)

/* Command flag bits. */
#define kCmdHidden			00001
#define kCmdMustBeConnected		00002
#define kCmdMustBeDisconnected		00004
#define kCompleteRemoteFile		00010
#define kCompleteRemoteDir		00020
#define kCompleteLocalFile		00040
#define kCompleteLocalDir		00100
#define kCompleteBookmark		00200
#define kCompletePrefOpt		00400

/* shell.c */
void InitCommandList(void);
CommandPtr GetCommandByIndex(const int);
CommandPtr GetCommandByName(const char *const, int);
void PrintCmdHelp(CommandPtr);
void PrintCmdUsage(CommandPtr);
int MakeArgv(char *, int *, const char **, int, char *, size_t, int *, int);
void XferCanceller(int);
void BackToTop(int);
void Cancel(int);
void CommandShell(void);
