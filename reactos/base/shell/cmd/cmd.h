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

#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <tchar.h>

#include "cmdver.h"
#include "cmddbg.h"

#define BREAK_BATCHFILE 1
#define BREAK_OUTOFBATCH 2
#define BREAK_INPUT 3
#define BREAK_IGNORE 4

/* define some error messages */

#define D_ON         _T("on")
#define D_OFF        _T("off")


/* command line buffer length */
#define CMDLINE_LENGTH  8192

/* 16k = max buffer size */
#define BUFF_SIZE 16384

/* global variables */
extern HANDLE hOut;
extern HANDLE hIn;
extern LPTSTR lpOriginalEnvironment;
extern WORD   wColor;
extern WORD   wDefColor;
extern BOOL   bCtrlBreak;
extern BOOL   bIgnoreEcho;
extern BOOL   bExit;
extern BOOL   bDisableBatchEcho;
extern BOOL   bDelayedExpansion;
extern INT    nErrorLevel;
extern SHORT  maxx;
extern SHORT  maxy;
extern BOOL bUnicodeOutput;


/* Prototypes for ALIAS.C */
VOID ExpandAlias (LPTSTR, INT);
INT CommandAlias (LPTSTR);

/* Prototypes for ASSOC.C */
INT CommandAssoc (LPTSTR);

/* Prototypes for ATTRIB.C */
INT CommandAttrib (LPTSTR);


/* Prototypes for BEEP.C */
INT cmd_beep (LPTSTR);


/* Prototypes for CALL.C */
INT cmd_call (LPTSTR);


/* Prototypes for CHCP.C */
INT CommandChcp (LPTSTR);


/* Prototypes for CHOICE.C */
INT CommandChoice (LPTSTR);


/* Prototypes for CLS.C */
INT cmd_cls (LPTSTR);


/* Prototypes for CMD.C */
INT ConvertULargeInteger(ULONGLONG num, LPTSTR des, UINT len, BOOL bPutSeperator);
HANDLE RunFile(DWORD, LPTSTR, LPTSTR, LPTSTR, INT);
INT ParseCommandLine(LPTSTR);
struct _PARSED_COMMAND;
INT ExecuteCommand(struct _PARSED_COMMAND *Cmd);
LPCTSTR GetEnvVarOrSpecial ( LPCTSTR varName );
VOID AddBreakHandler (VOID);
VOID RemoveBreakHandler (VOID);
BOOL SubstituteVars(TCHAR *Src, TCHAR *Dest, TCHAR Delim);
BOOL SubstituteForVars(TCHAR *Src, TCHAR *Dest);
LPTSTR DoDelayedExpansion(LPTSTR Line);
INT DoCommand(LPTSTR first, LPTSTR rest, struct _PARSED_COMMAND *Cmd);
BOOL ReadLine(TCHAR *commandline, BOOL bMore);

extern HANDLE CMD_ModuleHandle;


/* Prototypes for CMDINPUT.C */
BOOL ReadCommand (LPTSTR, INT);


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

/* Prototypes for COLOR.C */
BOOL SetScreenColor(WORD wColor, BOOL bNoFill);
INT CommandColor (LPTSTR);

VOID ConInDummy (VOID);
VOID ConInDisable (VOID);
VOID ConInEnable (VOID);
VOID ConInFlush (VOID);
VOID ConInKey (PINPUT_RECORD);
VOID ConInString (LPTSTR, DWORD);

VOID ConOutChar (TCHAR);
VOID ConOutPuts (LPTSTR);
VOID ConPrintf(LPTSTR, va_list, DWORD);
INT ConPrintfPaging(BOOL NewPage, LPTSTR, va_list, DWORD);
VOID ConOutPrintf (LPTSTR, ...);
INT ConOutPrintfPaging (BOOL NewPage, LPTSTR, ...);
VOID ConErrChar (TCHAR);
VOID ConErrPuts (LPTSTR);
VOID ConErrPrintf (LPTSTR, ...);
VOID ConOutFormatMessage (DWORD MessageId, ...);
VOID ConErrFormatMessage (DWORD MessageId, ...);

SHORT GetCursorX  (VOID);
SHORT GetCursorY  (VOID);
VOID  GetCursorXY (PSHORT, PSHORT);
VOID  SetCursorXY (SHORT, SHORT);

VOID GetScreenSize (PSHORT, PSHORT);
VOID SetCursorType (BOOL, BOOL);

VOID ConOutResPuts (UINT resID);
VOID ConErrResPuts (UINT resID);
VOID ConOutResPrintf (UINT resID, ...);
VOID ConErrResPrintf (UINT resID, ...);
VOID ConOutResPaging(BOOL NewPage, UINT resID);

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
VOID ErrorMessage (DWORD, LPTSTR, ...);

VOID error_no_pipe (VOID);
VOID error_bad_command (LPTSTR);
VOID error_invalid_drive (VOID);
VOID error_req_param_missing (VOID);
VOID error_sfile_not_found (LPTSTR);
VOID error_file_not_found (VOID);
VOID error_path_not_found (VOID);
VOID error_too_many_parameters (LPTSTR);
VOID error_parameter_format(TCHAR);
VOID error_invalid_switch (TCHAR);
VOID error_invalid_parameter_format (LPTSTR);
VOID error_out_of_memory (VOID);
VOID error_syntax (LPTSTR);

VOID msg_pause (VOID);


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
VOID History (INT, LPTSTR);/*add entries browse history*/
VOID History_move_to_bottom(VOID);/*F3*/
VOID InitHistory(VOID);
VOID CleanHistory(VOID);
VOID History_del_current_entry(LPTSTR str);/*CTRL-D*/
INT CommandHistory (LPTSTR param);
#endif


/* Prototypes for IF.C */
#define IFFLAG_NEGATE 1     /* NOT */
#define IFFLAG_IGNORECASE 2 /* /I  */
enum { IF_CMDEXTVERSION, IF_DEFINED, IF_ERRORLEVEL, IF_EXIST,
       IF_STRINGEQ,         /* == */
       IF_EQU, IF_GTR, IF_GEQ, IF_LSS, IF_LEQ, IF_NEQ };
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

/* Prototypes for LABEL.C */
INT cmd_label (LPTSTR);


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

/* cache codepage */
extern UINT InputCodePage;
extern UINT OutputCodePage;

/* Prototypes for MEMORY.C */
INT CommandMemory (LPTSTR);


/* Prototypes for MKLINK.C */
INT cmd_mklink(LPTSTR);


/* Prototypes for MISC.C */
INT GetRootPath(TCHAR *InPath,TCHAR *OutPath,INT size);
BOOL SetRootPath(TCHAR *oldpath,TCHAR *InPath);
TCHAR  cgetchar (VOID);
BOOL   CheckCtrlBreak (INT);
BOOL add_entry (LPINT ac, LPTSTR **arg, LPCTSTR entry);
LPTSTR *split (LPTSTR, LPINT, BOOL, BOOL);
LPTSTR *splitspace (LPTSTR, LPINT);
VOID   freep (LPTSTR *);
LPTSTR _stpcpy (LPTSTR, LPCTSTR);
VOID   StripQuotes(LPTSTR);
BOOL   IsValidPathName (LPCTSTR);
BOOL   IsExistingFile (LPCTSTR);
BOOL   IsExistingDirectory (LPCTSTR);
BOOL   FileGetString (HANDLE, LPTSTR, INT);
VOID   GetPathCase(TCHAR *, TCHAR *);

#define PROMPT_NO    0
#define PROMPT_YES   1
#define PROMPT_ALL   2
#define PROMPT_BREAK 3

INT PagePrompt (VOID);
INT FilePromptYN (UINT);
INT FilePromptYNA (UINT);


/* Prototypes for MOVE.C */
INT cmd_move (LPTSTR);


/* Prototypes for MSGBOX.C */
INT CommandMsgbox (LPTSTR);


/* Prototypes from PARSER.C */
enum { C_COMMAND, C_QUIET, C_BLOCK, C_MULTI, C_IFFAILURE, C_IFSUCCESS, C_PIPE, C_IF, C_FOR };
typedef struct _PARSED_COMMAND
{
    struct _PARSED_COMMAND *Subcommands;
    struct _PARSED_COMMAND *Next;
    struct _REDIRECTION *Redirections;
    BYTE Type;
    union
    {
        struct
        {
            TCHAR *Rest;
            TCHAR First[];
        } Command;
        struct
        {
            BYTE Flags;
            BYTE Operator;
            TCHAR *LeftArg;
            TCHAR *RightArg;
        } If;
        struct
        {
            BYTE Switches;
            TCHAR Variable;
            LPTSTR Params;
            LPTSTR List;
            struct tagFORCONTEXT *Context;
        } For;
    };
} PARSED_COMMAND;
PARSED_COMMAND *ParseCommand(LPTSTR Line);
VOID EchoCommand(PARSED_COMMAND *Cmd);
TCHAR *Unparse(PARSED_COMMAND *Cmd, TCHAR *Out, TCHAR *OutEnd);
VOID FreeCommand(PARSED_COMMAND *Cmd);


/* Prototypes from PATH.C */
INT cmd_path (LPTSTR);


/* Prototypes from PROMPT.C */
VOID InitPrompt (VOID);
VOID PrintPrompt (VOID);
INT  cmd_prompt (LPTSTR);


/* Prototypes for REDIR.C */
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
