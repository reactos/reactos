/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cmdexec.h

Abstract:

    Prototypes and external stuctures for cmdexec.c

Author:

    David J. Gilman (davegi) 04-May-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _CMDEXEC_ )
#define _CMDEXEC_

#include "windbg.h"

/************************** Structs and Defines *************************/

#define     PROMPT_SIZE     512

#define     LOG_BP_CLEAR    1
#define     LOG_BP_ENABLE   2
#define     LOG_BP_DISABLE  3

typedef enum {
    LOG_DM_UNKNOWN = -1,
    LOG_DM_ASCII = 0,
    LOG_DM_BYTE = 1,
    LOG_DM_WORD = 2,
    LOG_DM_DWORD = 3,
    LOG_DM_4REAL = 4,
    LOG_DM_8REAL = 5,
    LOG_DM_TREAL = 6,
    LOG_DM_UNICODE = 7,
    LOG_DM_MAX
} LOG_DM;

typedef enum {
    CMD_RET_SYNC = 1,
    CMD_RET_ASYNC = 2,
    CMD_RET_ERROR = 3
} CMD_RET;

typedef enum {
    CMDID_NULL = 0,
    CMDID_LOAD,
    CMDID_UNLOAD,
    CMDID_RELOAD,
    CMDID_SYMPATH,
    CMDID_NOVERSION,
    CMDID_HELP,
    CMDID_DEFAULT,
    CMDID_LIST_EXTS
} CMDID;
typedef CMDID *PCMDID;


typedef int LOGERR;

#define LOGERROR_NOERROR    0
#define LOGERROR_UNKNOWN    1
#define LOGERROR_QUIET      2
#define LOGERROR_CP         3
#define LOGERROR_ASYNC      4


#define TDWildInvalid()                         \
        if (LptdCommand == (LPTD)-1) {          \
            CmdLogVar(ERR_Thread_Wild_Invalid); \
            return LOGERROR_QUIET;              \
        }

#define PDWildInvalid()                         \
        if (LppdCommand == (LPPD)-1) {          \
            CmdLogVar(ERR_Process_Wild_Invalid);\
            return LOGERROR_QUIET;              \
        }

#define PreRunInvalid() \
    if (DbgState != ds_normal                            \
     || (LppdCommand && (LppdCommand != (LPPD)-1) &&     \
          ( ( LppdCommand->pstate == psPreRunning )      \
          ||                                             \
            (LppdCommand->lptdList                       \
                && LppdCommand->lptdList->tstate == tsPreRunning)  \
          )))                                            \
    { CmdLogVar(ERR_DbgState); return LOGERROR_QUIET; }


#define IsKdCmdAllowed() \
    if ( g_contWorkspace_WkSp.m_bKernelDebugger && IsProcRunning(LppdCur) ) { \
        CmdInsertInit(); \
        CmdLogFmt( "Cannot issue this command while target system is running\r\n" ); \
        return LOGERROR_QUIET; \
    }


typedef LOGERR (*DOTHANDLER)(LPSTR lpsz, DWORD dwData);

typedef struct _DOT_COMMAND {
    LPSTR lpName;
    DOTHANDLER lpfnHandler;
    DWORD dwArg;
    LPSTR lpDesc;
} DOT_COMMAND, *LPDOTCOMMAND;

// Number of entries in DotTable
extern const DWORD dwSizeofDotTable;



/************************** Public prototypes ****************************/

extern BOOL CmdDoLine(LPSTR lpsz);
extern VOID CmdDoPrompt(BOOL,BOOL);
extern VOID CmdSetDefaultCmdProc();

// this is only here to be called by CmdExecNext()...
extern BOOL CmdExecuteLine(LPSTR);

extern LPSTR  CmdGetDefaultPrompt( LPSTR lpPrompt );
extern VOID   CmdSetDefaultPrompt( LPSTR lpPrompt );
extern BOOL CmdNoLogString(LPCSTR buf);
extern int  CDECL  CmdLogVar(WORD, ...);
extern void WDBGAPIV CmdLogFmt(LPCSTR buf, ...);
extern void CmdLogFmtEx(BOOL fFileLog, BOOL fSendRemote, BOOL fPrintLocal, LPCSTR buf, ...); // Internal only
extern VOID CmdInsertInit(VOID);
extern VOID CmdSetCursor(VOID);
extern VOID CmdFileString(LPSTR lpsz);
extern VOID CmdLogDebugString(LPSTR buf, BOOL fSendRemote);
extern VOID CmdPrependCommands(LPTD lptd, LPSTR lpstr);
extern BOOL CmdAutoRunInit(VOID);
extern VOID CmdAutoRunNext(VOID);

extern BOOL StepOK(LPPD, LPTD);
extern BOOL GoOK(LPPD, LPTD);
extern BOOL GoExceptOK(LPPD,LPTD);

extern ULONG   ulRipBreakLevel;
extern ULONG   ulRipNotifyLevel;



/************************** Private Prototypes *************************/

LOGERR LogAssemble(LPSTR lpsz);
LOGERR LogAsmLine(LPSTR lpsz);
LOGERR LogBPChange(LPSTR lpsz, int iAction);
LOGERR LogBPList(VOID);
LOGERR LogBPSet(BOOL fDataBp, LPSTR lpsz);
LOGERR LogCallStack(LPSTR lpstr);
LOGERR LogCompare(LPSTR lpsz);
LOGERR LogConnect(LPSTR lpsz, DWORD dwUnused);
LOGERR LogCrash(LPSTR pszFileNameArg, DWORD /*dwUnused*/);
LOGERR LogDisasm(LPSTR lpsz,BOOL fSearch);
LOGERR LogDisconnect(LPSTR lpsz, DWORD dwUnused);
LOGERR LogDumpMem(char ch, LPSTR lpsz);
LOGERR LogEnterMem(LPSTR lpsz);
LOGERR LogException(LPSTR lpsz);
LOGERR LogEvaluate(LPSTR lpsz, BOOL fSpecialNtsdEval);
LOGERR LogFrameChange(LPSTR lpsz);
LOGERR LogFileClose(LPSTR lpUnused, DWORD dwUnused);
LOGERR LogFileOpen(LPSTR lpsz, DWORD fAppend);
LOGERR LogFill(LPSTR lpsz);
LOGERR LogFreeze(LPSTR lpsz, BOOL fFreeze);
LOGERR LogGoException(LPSTR lpsz, BOOL fHandled);
LOGERR LogGoUntil(LPSTR lpsz);
LOGERR LogList(LPSTR lpsz, DWORD dwUnused);
LOGERR LogListModules(LPSTR lpsz, BOOL);
LOGERR LogListNear(LPSTR lpsz);
LOGERR LogMovemem(LPSTR lpsz);
LOGERR LogOptions(LPSTR lpsz, DWORD dwUnused);
LOGERR LogProcess(VOID);
LOGERR LogRadix(LPSTR lpsz);
LOGERR LogReload(LPSTR lpsz, DWORD dwUnused);
LOGERR LogRegisters(LPSTR lpsz, BOOL fFP);
LOGERR LogRemote(LPSTR lpsz);
LOGERR LogRestart(LPSTR lpsz);
LOGERR LogSetErrorLevel(LPSTR lpsz);
LOGERR LogSearch(LPSTR lpsz);
LOGERR LogSearchDisasm(LPSTR lpsz);
LOGERR LogSource(LPSTR lpsz, DWORD dwUnused);
LOGERR LogSleep(LPSTR lpsz, DWORD dwUnused);
LOGERR LogStart(LPSTR lpsz, DWORD dwUnused);
LOGERR LogStartWithArgs(LPSTR lpsz, LPSTR lpszArgs);
LOGERR LogStep(LPSTR lpsz, BOOL fStep);
LOGERR LogThread(VOID);
LOGERR LogExamine(LPSTR lpsz);
LOGERR LogAttach(LPSTR lpsz, DWORD dwUnused);
LOGERR LogKill(LPSTR lpsz, DWORD dwUnused);
LOGERR LogConnect(LPSTR lpsz, DWORD dwUnused);
LOGERR LogDotHelp(LPSTR lpsz);
LOGERR LogDotCommand(LPSTR lpsz);
LOGERR LogWaitForString(LPSTR lpsz, DWORD dwUnused);
LOGERR LogBreak(LPSTR lpsz, DWORD dwUnused);
LOGERR LogLoadDefered( LPSTR lpsz);
LOGERR LogTitle( LPSTR lpsz, DWORD dwUnused);
LOGERR LogHelp( LPSTR lpsz);
LOGERR LogKernelPageIn( LPSTR lpsz);
LOGERR LogWatchTime( LPSTR lpsz);


int    CmdExecuteCmd(LPSTR);

VOID   CmdSetDefaultCmdProc(VOID);
VOID   CmdSetCmdProc(
        BOOL (*lpfnLP)(LPSTR lpsz),
        VOID (*lpfnPP)(BOOL, BOOL) );

LOG_DM LetterToType( char c );
BOOL   CmdExecuteLine(LPSTR);
VOID   CmdExecutePrompt(BOOL,BOOL);
BOOL   CmdEnterLine(LPSTR);
VOID   CmdEnterPrompt(BOOL,BOOL);
BOOL   CmdAsmLine(LPSTR);
VOID   CmdAsmPrompt(BOOL,BOOL);

LOGERR
DoEnterMem(
    LPSTR   lpsz,
    LPADDR  lpAddr,
    LOG_DM  type,
    BOOL    fMulti
    );

LOGERR
GetValueList(
    LPSTR   lpsz,
    LOG_DM  type,
    BOOL    fMulti,
    LPBYTE  lpBuf,
    int     cchBuf,
    PDWORD  pcch
    );

BOOL   GoOK(LPPD lppd, LPTD lptd );
BOOL   StepOK(LPPD lppd, LPTD lptd );
VOID   NoRunExcuse( LPPD lppd, LPTD lptd );
BOOL   FormatHSym(HSYM hsym, PCXT cxt, char *szStr);

DWORD  LogFileWrite(LPBYTE lpb, DWORD cb);

LOGERR    LogUnload(LPSTR, DWORD);
LOGERR  LogRefresh(LPSTR, DWORD);

VOID
ThreadStatForThread(
    LPTD lptd
    );

#endif // _CMDEXEC_

