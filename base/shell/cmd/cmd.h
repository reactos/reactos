/*
 *  CMD.H - header file for the modules in CMD.EXE
 *
 *
 *  History:
 *
 *    7-15-95 Tim Norman
 *        started
 *
 *    06/29/98 (Rob Lake)
 *        Moved error messages in here
 *
 *    07/12/98 (Rob Lake)
 *        Moved more error messages here.
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added compile date to version.
 *
 *    26-Feb-1999 (Eric Kohl)
 *        Introduced a new version string.
 *        Thanks to Emanuele Aliberti!
 */

#pragma once

#include <config.h>

#include "cmdver.h"
#include "cmddbg.h"

/* Version of the Command Extensions */
#define CMDEXTVERSION   2

#define BREAK_BATCHFILE     1
#define BREAK_OUTOFBATCH    2 /* aka. BREAK_ENDOFBATCHFILES */
#define BREAK_INPUT         3
#define BREAK_IGNORE        4

/* define some error messages */
#define D_ON         _T("on")
#define D_OFF        _T("off")

/* command line buffer length */
#define CMDLINE_LENGTH  8192

/* 16k = max buffer size */
#define BUFF_SIZE 16384

/* Global variables */
extern LPTSTR lpOriginalEnvironment;
extern WORD   wColor;
extern WORD   wDefColor;
extern BOOL   bCtrlBreak;
extern BOOL   bIgnoreEcho;
extern BOOL   bExit;
extern BOOL   bDisableBatchEcho;
extern BOOL   bEnableExtensions;
extern BOOL   bDelayedExpansion;
extern INT    nErrorLevel;


/* Prototypes for ALIAS.C */
VOID ExpandAlias (LPTSTR, INT);
INT CommandAlias (LPTSTR);

/* Prototypes for ASSOC.C */
INT CommandAssoc (LPTSTR);

/* Prototypes for BEEP.C */
INT cmd_beep (LPTSTR);

/* Prototypes for CALL.C */
INT cmd_call (LPTSTR);

/* Prototypes for CHOICE.C */
INT CommandChoice (LPTSTR);

/* Prototypes for CLS.C */
INT cmd_cls (LPTSTR);

/* Prototypes for CMD.C */
INT ConvertULargeInteger(ULONGLONG num, LPTSTR des, UINT len, BOOL bPutSeparator);
HANDLE RunFile(DWORD, LPTSTR, LPTSTR, LPTSTR, INT);
INT ParseCommandLine(LPTSTR);
struct _PARSED_COMMAND;

INT
ExecuteCommand(
    IN struct _PARSED_COMMAND *Cmd);

INT
ExecuteCommandWithEcho(
    IN struct _PARSED_COMMAND *Cmd);

LPCTSTR GetEnvVarOrSpecial ( LPCTSTR varName );
VOID AddBreakHandler (VOID);
VOID RemoveBreakHandler (VOID);

BOOL
SubstituteVar(
    IN PCTSTR Src,
    OUT size_t* SrcIncLen, // VarNameLen
    OUT PTCHAR Dest,
    IN PTCHAR DestEnd,
    OUT size_t* DestIncLen,
    IN TCHAR Delim);

BOOL
SubstituteVars(
    IN PCTSTR Src,
    OUT PTSTR Dest,
    IN TCHAR Delim);

BOOL
SubstituteForVars(
    IN PCTSTR Src,
    OUT PTSTR Dest);

PTSTR
DoDelayedExpansion(
    IN PCTSTR Line);

INT DoCommand(LPTSTR first, LPTSTR rest, struct _PARSED_COMMAND *Cmd);
BOOL ReadLine(TCHAR *commandline, BOOL bMore);

extern HANDLE CMD_ModuleHandle;


/* Prototypes for CMDINPUT.C */
BOOL ReadCommand (LPTSTR, INT);

extern TCHAR AutoCompletionChar;
extern TCHAR PathCompletionChar;

#define IS_COMPLETION_DISABLED(CompletionCtrl)  \
    ((CompletionCtrl) == 0x00 || (CompletionCtrl) == 0x0D || (CompletionCtrl) >= 0x20)


/* Prototypes for CMDTABLE.C */
#define CMD_SPECIAL     1
#define CMD_BATCHONLY   2
#define CMD_HIDE        4

typedef struct tagCOMMAND
{
    LPTSTR name;
    INT    flags;
    INT    (*func)(LPTSTR);
} COMMAND, *LPCOMMAND;

extern COMMAND cmds[];  /* The internal command table */

VOID PrintCommandList (VOID);

LPCTSTR GetParsedEnvVar ( LPCTSTR varName, UINT* varNameLen, BOOL ModeSetA );

/* Prototypes for CTTY.C */
#ifdef INCLUDE_CMD_CTTY
INT cmd_ctty(LPTSTR);
#endif

/* Prototypes for COLOR.C */
INT CommandColor(LPTSTR);

/* Prototypes for CONSOLE.C */
#include "console.h"

/* Prototypes for COPY.C */
INT cmd_copy (LPTSTR);

/* Prototypes for DATE.C */
INT cmd_date (LPTSTR);

/* Prototypes for DEL.C */
INT CommandDelete (LPTSTR);

/* Prototypes for DELAY.C */
INT CommandDelay (LPTSTR);

/* Prototypes for DIR.C */
INT FormatDate (TCHAR *, LPSYSTEMTIME, BOOL);
INT FormatTime (TCHAR *, LPSYSTEMTIME);
INT CommandDir (LPTSTR);

/* Prototypes for DIRSTACK.C */
VOID InitDirectoryStack (VOID);
VOID DestroyDirectoryStack (VOID);
INT  GetDirectoryStackDepth (VOID);
INT  CommandPushd (LPTSTR);
INT  CommandPopd (LPTSTR);
INT  CommandDirs (LPTSTR);

/* Prototypes for ECHO.C */
BOOL OnOffCommand(LPTSTR param, LPBOOL flag, INT message);
INT  CommandEcho (LPTSTR);
INT  CommandEchos (LPTSTR);
INT  CommandEchoerr (LPTSTR);
INT  CommandEchoserr (LPTSTR);

/* Prototypes for ERROR.C */
VOID
ErrorMessage(
    IN DWORD dwErrorCode,
    IN PCTSTR szFormat OPTIONAL,
    ...);

VOID error_no_pipe(VOID);
VOID error_bad_command(PCTSTR s);
VOID error_invalid_drive(VOID);
VOID error_req_param_missing(VOID);
VOID error_sfile_not_found(PCTSTR s);
VOID error_file_not_found(VOID);
VOID error_path_not_found(VOID);
VOID error_too_many_parameters(PCTSTR s);
VOID error_parameter_format(TCHAR ch);
VOID error_invalid_switch(TCHAR ch);
VOID error_invalid_parameter_format(PCTSTR s);
VOID error_out_of_memory(VOID);
VOID error_syntax(PCTSTR s);

VOID msg_pause(VOID);

/* Prototypes for FILECOMP.C */
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
VOID CompleteFilename (LPTSTR, UINT);
INT  ShowCompletionMatches (LPTSTR, INT);
#endif
#ifdef FEATURE_4NT_FILENAME_COMPLETION
VOID CompleteFilename (LPTSTR, BOOL, LPTSTR, UINT);
#endif


/* Prototypes for FOR.C */
#define FOR_DIRS      1 /* /D */
#define FOR_F         2 /* /F */
#define FOR_LOOP      4 /* /L */
#define FOR_RECURSIVE 8 /* /R */
INT cmd_for (LPTSTR);
INT ExecuteFor(struct _PARSED_COMMAND *Cmd);


/* Prototypes for FREE.C */
INT CommandFree (LPTSTR);

/* Prototypes for GOTO.C */
INT cmd_goto (LPTSTR);

/* Prototypes for HISTORY.C */
#ifdef FEATURE_HISTORY
LPCTSTR PeekHistory(INT);
VOID History(INT, LPTSTR);/*add entries browse history*/
VOID History_move_to_bottom(VOID);/*F3*/
VOID InitHistory(VOID);
VOID CleanHistory(VOID);
VOID History_del_current_entry(LPTSTR str);/*CTRL-D*/
INT CommandHistory(LPTSTR param);
#endif

/* Prototypes for IF.C */
#define IFFLAG_NEGATE     1 /* NOT */
#define IFFLAG_IGNORECASE 2 /* /I - Extended */
typedef enum _IF_OPERATOR
{
    /** Unary operators **/
    /* Standard */
    IF_ERRORLEVEL, IF_EXIST,
    /* Extended */
    IF_CMDEXTVERSION, IF_DEFINED,

    /** Binary operators **/
    /* Standard */
    IF_STRINGEQ,    /* == */
    /* Extended */
    IF_EQU, IF_NEQ, IF_LSS, IF_LEQ, IF_GTR, IF_GEQ
} IF_OPERATOR;

INT ExecuteIf(struct _PARSED_COMMAND *Cmd);

/* Prototypes for INTERNAL.C */
VOID InitLastPath (VOID);
VOID FreeLastPath (VOID);
INT  cmd_chdir (LPTSTR);
INT  cmd_mkdir (LPTSTR);
INT  cmd_rmdir (LPTSTR);
INT  CommandExit (LPTSTR);
INT  CommandRem (LPTSTR);
INT  CommandShowCommands (LPTSTR);

/* Prototypes for LOCALE.C */
extern TCHAR cDateSeparator;
extern INT   nDateFormat;
extern TCHAR cTimeSeparator;
extern INT   nTimeFormat;
extern TCHAR cThousandSeparator;
extern TCHAR cDecimalSeparator;
extern INT nNumberGroups;

VOID InitLocale (VOID);
LPTSTR GetDateString (VOID);
LPTSTR GetTimeString (VOID);

/* Prototypes for MEMORY.C */
INT CommandMemory (LPTSTR);

/* Prototypes for MKLINK.C */
INT cmd_mklink(LPTSTR);

/* Prototypes for MISC.C */
INT
GetRootPath(
    IN LPCTSTR InPath,
    OUT LPTSTR OutPath,
    IN INT size);

BOOL SetRootPath(TCHAR *oldpath,TCHAR *InPath);
TCHAR  cgetchar (VOID);
BOOL   CheckCtrlBreak (INT);
BOOL add_entry (LPINT ac, LPTSTR **arg, LPCTSTR entry);
LPTSTR *split (LPTSTR, LPINT, BOOL, BOOL);
LPTSTR *splitspace (LPTSTR, LPINT);
VOID   freep (LPTSTR *);
LPTSTR _stpcpy (LPTSTR, LPCTSTR);
VOID   StripQuotes(LPTSTR);

BOOL IsValidPathName(IN LPCTSTR pszPath);
BOOL IsExistingFile(IN LPCTSTR pszPath);
BOOL IsExistingDirectory(IN LPCTSTR pszPath);
VOID GetPathCase(IN LPCTSTR Path, OUT LPTSTR OutPath);

#define PROMPT_NO    0
#define PROMPT_YES   1
#define PROMPT_ALL   2
#define PROMPT_BREAK 3

BOOL __stdcall PagePrompt(PCON_PAGER Pager, DWORD Done, DWORD Total);
INT FilePromptYN (UINT);
INT FilePromptYNA (UINT);

/* Prototypes for MOVE.C */
INT cmd_move (LPTSTR);

/* Prototypes for MSGBOX.C */
INT CommandMsgbox (LPTSTR);

/* Prototypes from PARSER.C */

/* These three characters act like spaces to the parser in most contexts */
#define STANDARD_SEPS _T(",;=")

typedef enum _COMMAND_TYPE
{
    /* Standard command */
    C_COMMAND,
    /* Quiet operator */
    C_QUIET,
    /* Parenthesized block */
    C_BLOCK,
    /* Operators */
    C_MULTI, C_OR, C_AND, C_PIPE,
    /* Special parsed commands */
    C_FOR, C_IF, C_REM
} COMMAND_TYPE;

typedef struct _PARSED_COMMAND
{
    /*
     * For IF : this is the 'main' case (the 'else' is obtained via SubCmd->Next).
     * For FOR: this is the list of all the subcommands in the DO.
     */
    struct _PARSED_COMMAND *Subcommands;

    struct _PARSED_COMMAND *Next; // Next command(s) in the chain.
    struct _REDIRECTION *Redirections;
    COMMAND_TYPE Type;
    union
    {
        struct
        {
            PTSTR Rest;
            TCHAR First[];
        } Command;
        struct
        {
            BYTE Switches;
            TCHAR Variable;
            PTSTR Params;
            PTSTR List;
            struct _FOR_CONTEXT *Context;
        } For;
        struct
        {
            BYTE Flags;
            IF_OPERATOR Operator;
            PTSTR LeftArg;
            PTSTR RightArg;
        } If;
    };
} PARSED_COMMAND;

PARSED_COMMAND*
ParseCommand(
    IN PCTSTR Line);

VOID
DumpCommand(
    IN PARSED_COMMAND* Cmd,
    IN ULONG SpacePad);

VOID
EchoCommand(
    IN PARSED_COMMAND* Cmd);

PTCHAR
UnparseCommand(
    IN PARSED_COMMAND* Cmd,
    OUT PTCHAR Out,
    IN  PTCHAR OutEnd);

VOID
FreeCommand(
    IN OUT PARSED_COMMAND* Cmd);

VOID ParseErrorEx(IN PCTSTR s);
extern BOOL bParseError;
extern TCHAR ParseLine[CMDLINE_LENGTH];

extern BOOL bIgnoreParserComments;
extern BOOL bHandleContinuations;

/* Prototypes from PATH.C */
INT cmd_path (LPTSTR);

/* Prototypes from PROMPT.C */
VOID InitPrompt (VOID);
VOID PrintPrompt (VOID);
INT  cmd_prompt (LPTSTR);
BOOL HasInfoLine(VOID);

/* Prototypes for REDIR.C */
HANDLE GetHandle(UINT Number);
VOID SetHandle(UINT Number, HANDLE Handle);

typedef enum _REDIR_MODE
{
    REDIR_READ   = 0,
    REDIR_WRITE  = 1,
    REDIR_APPEND = 2
} REDIR_MODE;
typedef struct _REDIRECTION
{
    struct _REDIRECTION *Next;
    HANDLE OldHandle;
    BYTE Number;
    REDIR_MODE Mode;
    TCHAR Filename[];
} REDIRECTION;
BOOL PerformRedirection(REDIRECTION *);
VOID UndoRedirection(REDIRECTION *, REDIRECTION *End);
INT GetRedirection(LPTSTR, REDIRECTION **);
VOID FreeRedirection(REDIRECTION *);

/* Prototypes for REN.C */
INT cmd_rename (LPTSTR);

/* Prototypes for REN.C */
INT cmd_replace (LPTSTR);

/* Prototypes for SCREEN.C */
INT CommandScreen (LPTSTR);

/* Prototypes for SET.C */
INT cmd_set (LPTSTR);

/* Prototypes for SETLOCAL.C */
LPTSTR DuplicateEnvironment(VOID);
INT cmd_setlocal (LPTSTR);
INT cmd_endlocal (LPTSTR);

/* Prototypes for START.C */
INT cmd_start (LPTSTR);

/* Prototypes for STRTOCLR.C */
BOOL StringToColor (LPWORD, LPTSTR *);

/* Prototypes for TIME.C */
INT cmd_time (LPTSTR);

/* Prototypes for TIMER.C */
INT CommandTimer (LPTSTR param);

/* Prototypes for TITLE.C */
INT cmd_title (LPTSTR);

/* Prototypes for TYPE.C */
INT cmd_type (LPTSTR);

/* Prototypes for VER.C */
VOID InitOSVersion(VOID);
VOID PrintOSVersion(VOID);
INT  cmd_ver (LPTSTR);

/* Prototypes for VERIFY.C */
INT cmd_verify (LPTSTR);

/* Prototypes for VOL.C */
INT cmd_vol (LPTSTR);

/* Prototypes for WHERE.C */
BOOL SearchForExecutable (LPCTSTR, LPTSTR);

/* Prototypes for WINDOW.C */
INT CommandActivate (LPTSTR);
INT CommandWindow (LPTSTR);

/* The MSDOS Batch Commands [MS-DOS 5.0 User's Guide and Reference p359] */
int cmd_if(TCHAR *);
int cmd_pause(TCHAR *);
int cmd_shift(TCHAR *);
