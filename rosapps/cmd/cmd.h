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
 *        Moved more error messages here
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added compile date to version
 */

// #define WIN32_LEAN_AND_MEAN

#include <windows.h>
// #include <tchar.h>

#define SHELLVER     "version 0.0.4 [" __DATE__"]"

#define BREAK_BATCHFILE 1
#define BREAK_OUTOFBATCH 2
#define BREAK_INPUT 3
#define BREAK_IGNORE 4

/* define some error messages */
#define NOENVERR        "ERROR: no environment"
#define INVALIDDRIVE    "ERROR: invalid drive"
#define INVALIDFUNCTION "ERROR: invalid function"
#define ACCESSDENIED    "ERROR: access denied"
#define BADENVIROMENT   "ERROR: bad enviroment"
#define BADFORMAT       "ERROR: bad format"
#define ERROR_E2BIG     "ERROR: Argument list too long"
#define ERROR_EINVAL    "ERROR: Invalid argument"

#define SHELLINFO    "ReactOS Command Line Interface"
#define USAGE        "usage"


#define D_ON         "on"
#define D_OFF        "off"



/* prototypes for CMD.C */
extern HANDLE hOut;
extern HANDLE hIn;
extern WORD   wColor;
extern WORD   wDefColor;
extern BOOL   bCtrlBreak;
extern BOOL   bIgnoreEcho;
extern BOOL   bExit;
extern int errorlevel;
extern SHORT  maxx;
extern SHORT  maxy;
extern OSVERSIONINFO osvi;


// VOID Execute (char *, char *);
void command(char *);
VOID ParseCommandLine (LPTSTR);
int  c_brk(void);




/* prototypes for ALIAS.C */
VOID ExpandAlias (char *, int);
INT cmd_alias (LPTSTR, LPTSTR);


/* prototyped for ATTRIB.C */
INT cmd_attrib (LPTSTR, LPTSTR);


/* prototypes for CLS.C */
INT cmd_cls (LPTSTR, LPTSTR);


/* prototypes for CMDINPUT.C */
VOID ReadCommand (LPTSTR, INT);


/* prototypes for CMDTABLE.C */
#define CMD_SPECIAL     1
#define CMD_BATCHONLY   2

typedef struct tagCOMMAND
{
	LPTSTR name;
	INT    flags;
	INT    (*func) (LPTSTR, LPTSTR);
} COMMAND, *LPCOMMAND;


/* prototypes for COLOR.C */
VOID SetScreenColor (WORD);
INT cmd_color (LPTSTR, LPTSTR); 


/* prototypes for CONSOLE.C */
#ifdef _DEBUG
VOID DebugPrintf (LPTSTR, ...);
#endif /* _DEBUG */

VOID ConInDummy (VOID);
VOID ConInKey (PINPUT_RECORD);

VOID ConInString (LPTSTR, DWORD);


VOID ConOutChar (TCHAR);
VOID ConOutPuts (LPTSTR);
VOID ConOutPrintf (LPTSTR, ...);
VOID ConErrChar (TCHAR);
VOID ConErrPuts (LPTSTR);
VOID ConErrPrintf (LPTSTR, ...);


SHORT wherex (VOID);
SHORT wherey (VOID);
VOID goxy (SHORT, SHORT);

VOID GetScreenSize (PSHORT, PSHORT);
VOID SetCursorType (BOOL, BOOL);


/* prototypes for COPY.C */
INT cmd_copy (LPTSTR, LPTSTR);


/* prototypes for DATE.C */
INT cmd_date (LPTSTR, LPTSTR);


/* prototypes for DEL.C */
INT cmd_del (LPTSTR, LPTSTR);


/* prototypes for DIR.C */
//int incline(int *line, unsigned flags);
INT cmd_dir (LPTSTR, LPTSTR);


/* prototypes for DIRSTACK.C */
VOID InitDirectoryStack (VOID);
VOID DestroyDirectoryStack (VOID);
INT  GetDirectoryStackDepth (VOID);
INT  cmd_pushd (LPTSTR, LPTSTR);
INT  cmd_popd (LPTSTR, LPTSTR);


/* Prototypes for ERROR.C */
VOID ErrorMessage (DWORD, LPTSTR, ...);

VOID error_no_pipe (VOID);
VOID error_bad_command (VOID);
VOID error_invalid_drive (VOID);
VOID error_req_param_missing (VOID);
VOID error_sfile_not_found (LPTSTR);
VOID error_file_not_found (VOID);
VOID error_path_not_found (VOID);
VOID error_too_many_parameters (LPTSTR);
VOID error_invalid_switch (TCHAR);
VOID error_invalid_parameter_format (LPTSTR);
VOID error_out_of_memory (VOID);
VOID error_syntax (LPTSTR);

VOID msg_pause (VOID);


/* prototypes for FILECOMP.C */
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
VOID CompleteFilename (LPTSTR, INT);
INT  ShowCompletionMatches (LPTSTR, INT);
#endif
#ifdef FEATURE_4NT_FILENAME_COMPLETION
#endif


/* prototypes for HISTORY.C */
#ifdef FEATURE_HISTORY
VOID History (INT, LPTSTR);
#endif


/* prototypes for INTERNAL.C */
VOID InitLastPath (VOID);
VOID FreeLastPath (VOID);
int cmd_chdir(char *, char *);
int cmd_mkdir(char *, char *);
int cmd_rmdir(char *, char *);
int internal_exit(char *, char *);
int cmd_rem(char *, char *);
int cmd_showcommands(char *, char *);


/* prototyped for LABEL.C */
INT cmd_label (LPTSTR, LPTSTR);


/* prototypes for LOCALE.C */
extern TCHAR cDateSeparator;
extern INT   nDateFormat;
extern TCHAR cTimeSeparator;
extern INT   nTimeFormat;
extern TCHAR aszDayNames[7][8];
extern TCHAR cThousandSeparator;
extern TCHAR cDecimalSeparator;
extern INT   nNumberGroups;

VOID InitLocale (VOID);


/* Prototypes for MISC.C */
TCHAR  cgetchar (VOID);
BOOL   CheckCtrlBreak (INT);
LPTSTR *split (LPTSTR, LPINT);
VOID   freep (LPTSTR *);
LPTSTR stpcpy (LPTSTR, LPTSTR);
BOOL   IsValidPathName (LPCTSTR);
BOOL   IsValidFileName (LPCTSTR);
BOOL   FileGetString (HANDLE, LPTSTR, INT);


/* prototypes for MOVE.C */
INT cmd_move (LPTSTR, LPTSTR);


/* prototypes from PATH.C */
INT cmd_path (LPTSTR, LPTSTR);


/* prototypes from PROMPT.C */
VOID PrintPrompt (VOID);
INT  cmd_prompt (LPTSTR, LPTSTR);


/* prototypes for REDIR.C */
#define INPUT_REDIRECTION    1
#define OUTPUT_REDIRECTION   2
#define OUTPUT_APPEND        4
#define ERROR_REDIRECTION    8
#define ERROR_APPEND        16
INT GetRedirection (LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPINT);


/* prototypes for REN.C */
INT cmd_rename (LPTSTR, LPTSTR);


/* prototypes for SET.C */
INT cmd_set (LPTSTR, LPTSTR);


/* prototypes for TIME.C */
INT cmd_time (LPTSTR, LPTSTR);


/* prototypes for TYPE.C */
INT cmd_type (LPTSTR, LPTSTR);


/* prototypes for VER.C */
VOID ShortVersion (VOID);
INT  cmd_ver (LPTSTR, LPTSTR);


/* prototypes for VERIFY.C */
INT cmd_verify (LPTSTR, LPTSTR);


/* prototypes for VOL.C */
INT cmd_vol (LPTSTR, LPTSTR);


/* prototypes for WHERE.C */
BOOL find_which (LPCTSTR, LPTSTR);




/* The MSDOS Batch Commands [MS-DOS 5.0 User's Guide and Reference p359] */
int cmd_call(char *, char *);
int cmd_echo(char *, char *);
int cmd_for(char *, char *);
int cmd_goto(char *, char *);
int cmd_if(char *, char *);
int cmd_pause(char *, char *);
int cmd_shift(char *, char *);

int cmd_beep(char *, char *);


