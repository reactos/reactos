/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dmx32.c

Abstract:

Author:

    Wesley Witt (wesw) 15-Aug-1992

Environment:

    NT 3.1

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "simpldis.h"
#include "dmsql.h"
#include "fiber.h"

DBF *lpdbf;

#undef LOCAL

typedef enum {
    Image_Unknown,
    Image_16,
    Image_32,
    Image_Dump
} IMAGETYPE;
typedef IMAGETYPE *PIMAGETYPE;


enum {
    System_DmpInvalid = -3,          /* The DUMP can not be debugged  */
    System_DmpWrongPlatform = -2,    /* The cross-platform debugging */
                                     /* of DUMPs is not supported */
    System_Invalid = -1,             /* The exe can not be debugged  */
    System_Console =  1,             /* The exe needs a console      */
    System_GUI     =  0              /* The exe is a Windows exe     */
};

int    pCharMode(LPTSTR szAppName, PIMAGETYPE Image);

int nVerbose = MIN_VERBOSITY_LEVEL;

// std{in,out,err} for redirection of console apps.
// See ProcessDebuggeeRedirection()
static HANDLE           rgh[3] = {0, 0, 0};

DMTLFUNCTYPE        DmTlFunc = NULL;

BOOL FDMRemote = FALSE;  // set true for remote debug

BOOL FUseOutputDebugString = FALSE;

BOOL FChicago = FALSE;  // set true if running on Win95.

EXPECTED_EVENT  masterEE = {0L,0L};
EXPECTED_EVENT *eeList = &masterEE;

static HTHDXSTRUCT masterTH = {0L,0L};
HTHDX       thdList = &masterTH;

static HPRCXSTRUCT masterPR = {0L,0L};
HPRCX       prcList = &masterPR;

// control access to thread and process lists:
CRITICAL_SECTION csThreadProcList;
CRITICAL_SECTION csEventList;

// control access to Walk list
CRITICAL_SECTION    csWalk;

HPID hpidRoot = (HPID)INVALID;  // this hpid is our hook to the native EM
BOOL fUseRoot;                  // next CREATE_PROCESS will use hpidRoot

DEBUG_EVENT64 falseSSEvent;
DEBUG_EVENT64 falseBPEvent;
DEBUG_EVENT64 FuncExitEvent;
METHOD      EMNotifyMethod;

// Don't allow debug event processing during some shell operations
CRITICAL_SECTION csProcessDebugEvent;

// Event handles for synchronizing with the shell on proc/thread creates.
HANDLE hEventCreateProcess;
HANDLE hEventContinue;

// event handle for synchronizing connnect/reconnect with the tl
HANDLE hEventRemoteQuit;

HANDLE hEventNoDebuggee;        // set when no debuggee is attached

int    nWaitingForLdrBreakpoint = 0;

BOOL    FLoading16 = FALSE;
BOOL    fDisconnected = FALSE;


#ifdef DEBUG_API_INTERLOCK 

static int      ApiLockCount = 0;
static PCSTR    pszLastFileToLock = NULL;
static int      nLastFileToLock = 0;
static PCSTR    pszLastFileToUnlock = NULL;
static int      nLastFileToUnlock = 0;


VOID
TakeApiLockFunc(
    PCSTR pszFile,
    int nLine
    )
{
    if (ApiLockCount        
        && (ULONG_PTR)GetCurrentThreadId() != (ULONG_PTR)csApiInterlock.OwningThread ) {
        
        DPRINT(-2, ( (" *** WARN: Waiting on another thread %s %d\n"), pszFile, nLine) );
        //assert(!"API lock released when not owned");
    }

    
    EnterCriticalSection(&csApiInterlock);

    if (ApiLockCount++ > 1) {
        DPRINT(-2, ( (" *** ERROR: Locked twice. TakeApiLock %s %d\n"), pszFile, nLine) );
        assert(!"API locked twice");
    }

    DPRINT(-1, ( (" *** TakeApiLock %s %d\n"), pszFile, nLine) );

    pszLastFileToLock = pszFile;
    nLastFileToLock = nLine;
}

BOOL
TryApiLockFunc(
    PCSTR pszFile,
    int nLine
    )
{
    BOOL b = TryEnterCriticalSection(&csApiInterlock);
    
    if (b) {
        ++ApiLockCount;
        DPRINT(-1, ( ("TryApiLock %s %d\n"), pszFile, nLine) );

        pszLastFileToLock = pszFile;
        nLastFileToLock = nLine;
    }

    if (ApiLockCount > 1) {
        DPRINT(-2, ( (" *** ERROR: Locked twice. TryApiLock %s %d\n"), pszFile, nLine) );
        assert(!"API locked twice");
    }

    return b;
}

VOID
ReleaseApiLockFunc(
    PCSTR pszFile,
    int nLine
    )
{
    DPRINT(-1, ( (" *** ReleaseApiLock %s %d\n"), pszFile, nLine) )

    if ((ULONG_PTR)GetCurrentThreadId() != (ULONG_PTR)csApiInterlock.OwningThread ) {
        DPRINT(-2, ( (" *** ERROR: Not owner. ReleaseApiLock %s %d\n"), pszFile, nLine) );
        assert(!"API lock released when not owned");
    }
    if (ApiLockCount < 1) {
        DPRINT(-2, ( (" *** ERROR: Count is 0. ReleaseApiLock %s %d\n"), pszFile, nLine) );
        assert(!"API Lock released when count is 0");
    } else {
        --ApiLockCount;
        LeaveCriticalSection(&csApiInterlock);
    }

    pszLastFileToUnlock = pszFile;
    nLastFileToUnlock = nLine;
}

#endif // DEBUG_API_INTERLOCK 


#ifndef KERNEL

//
// There may be multiple pending debug events.
//

LIST_ENTRY DmListOfPendingDebugEvents;
LIST_ENTRY DmFreeListOfDebugEvents;

PDETOSAVE GetNextFreeDebugEvent();


//
// crash dump stuff
//

BOOL                            CrashDump;
PCONTEXT                        CrashContext;
PEXCEPTION_RECORD               CrashException;
PUSERMODE_CRASHDUMP_HEADER      CrashDumpHeader;
ULONG64                         KiPcrBaseAddress;
ULONG64                         KiProcessors;

HANDLE hDmPollThread = 0;       // Handle for event loop thread.
BOOL   fDmPollQuit = FALSE;     // tell poll thread to exit NOW

SYSTEM_INFO SystemInfo;
OSVERSIONINFO OsVersionInfo;

WT_STRUCT             WtStruct;             // ..  for wt

VOID
Cleanup(
    VOID
    );

DWORD WINAPI
CallDmPoll(
    LPVOID lpv
    );

VOID
CrashDumpThread(
    LPVOID lpv
    );

void
Close3Handles(
    HANDLE *rgh
    );

XOSD
ProcessDebuggeeRedirection(
    LPTSTR lszCommandLine,
    STARTUPINFO FAR * psi
    );

BOOL
SetDebugPrivilege(
    void
    );

#endif // !KERNEL

#if defined(TARGET_IA64) || defined(TARGET_AXP64) //while we don't have a good crashlib, etc.
BOOL
DmpGetThread(
    IN  ULONG           Processor,
    OUT PCRASH_THREAD   Thread
    ) { return FALSE; }

BOOL
DmpDetectVersionParameters(
    CRASHDUMP_VERSION_INFO* VersionInfo
    ) { return FALSE; }

BOOL
DmpInitialize (
    IN  LPSTR               FileName,
    OUT PCONTEXT            *Context,
    OUT PEXCEPTION_RECORD   *Exception,
    OUT PVOID               *DmpHeader
    ) { return FALSE; }
DWORD
DmpReadMemory (
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Size
    ) { return FALSE; }

BOOL
DmpGetContext(
    IN  ULONG     Processor,
    OUT PVOID     Context
    ) { return FALSE; }

#define xosdDumpInvalidFile xosdUnsupported
#define xosdDumpWrongPlatform xosdUnsupported

#endif

BOOL DmpIsItAUserModeFile(VOID)  { return FALSE; }
BOOL DmpUserModeTestHeader(VOID) { return FALSE; }

#if defined(TARGET_IA64) // AXP64 has disasm

VOID CleanupDisassembler(VOID) { };

#endif

#ifdef KERNEL
extern BOOL fCrashDump;

KDOPTIONS KdOptions[] = {
    _T("BaudRate"),        KDO_BAUDRATE,      KDT_DWORD,     9600,
    _T("Port"),            KDO_PORT,          KDT_DWORD,     2,
    _T("Cache"),           KDO_CACHE,         KDT_DWORD,     8192,
    _T("Verbose"),         KDO_VERBOSE,       KDT_DWORD,     0,
    _T("InitialBp"),       KDO_INITIALBP,     KDT_DWORD,     0,
    _T("Defer"),           KDO_DEFER,         KDT_DWORD,     0,
    _T("UseModem"),        KDO_USEMODEM,      KDT_DWORD,     0,
    _T("LogfileAppend"),   KDO_LOGFILEAPPEND, KDT_DWORD,     0,
    _T("GoExit"),          KDO_GOEXIT,        KDT_DWORD,     0,
    _T("SymbolPath"),      KDO_SYMBOLPATH,    KDT_STRING,    0,
    _T("LogfileName"),     KDO_LOGFILENAME,   KDT_STRING,    0,
    _T("CrashDump"),       KDO_CRASHDUMP,     KDT_STRING,    0
};

VOID
GetKernelSymbolAddresses(
    VOID
    );

MODULEALIAS  ModuleAlias[MAX_MODULEALIAS];

#endif  // KERNEL


TCHAR  nameBuffer[256];

// Reply buffers to and from em
char  abEMReplyBuf[4096];       // Buffer for EM to reply to us in
char  abDMReplyBuf[4096];       // Buffer for us to reply to EM requests in
LPDM_MSG LpDmMsg = (LPDM_MSG)abDMReplyBuf;

// To send a reply of the struct msMyStruct, do this:
//      LpDmMsg->xosdRet = xosdMyReturnValue
//      memcpy (LpDmMsg->rgb, &msMyStruct, sizeof (msMyStruct));
//      Reply (sizeof (msMyStruct), LpDmMsg, hpid);

DDVECTOR DebugDispatchTable[] = {
    ProcessExceptionEvent,
    ProcessCreateThreadEvent,
    ProcessCreateProcessEvent,
    ProcessExitThreadEvent,
    ProcessExitProcessEvent,
    ProcessLoadDLLEvent,
    ProcessUnloadDLLEvent,
    ProcessOutputDebugStringEvent,
    ProcessRipEvent,
    ProcessBreakpointEvent,
    NULL,                       /* CHECK_BREAKPOINT_DEBUG_EVENT */
    ProcessSegmentLoadEvent,    /* SEGMENT_LOAD_DEBUG_EVENT */
    NULL,                       /* DESTROY_PROCESS_DEBUG_EVENT */
    NULL,                       /* DESTROY_THREAD_DEBUG_EVENT */
    NULL,                       /* ATTACH_DEADLOCK_DEBUG_EVENT */
    NULL,                       /* ATTACH_EXITED_DEBUG_EVENT */
    ProcessEntryPointEvent,     /* ENTRYPOINT_DEBUG_EVENT */
    NULL,                       /* LOAD_COMPLETE_DEBUG_EVENT */
    NULL,                       /* INPUT_DEBUG_STRING_EVENT */
    NULL,                       /* MESSAGE_DEBUG_EVENT */
    NULL,                       /* MESSAGE_SEND_DEBUG_EVENT */
    NULL,                       /* FUNC_EXIT_EVENT */
#ifdef KERNEL
    NULL,
    NULL,
#else
    ProcessOleEvent,            /* OLE_DEBUG_EVENT */
    ProcessFiberEvent,
#endif
    NULL,                       /* GENERIC_DEBUG_EVENT */
#ifdef KERNEL
    NULL,
#else
    ProcessBogusSSEvent,        /* BOGUS_WIN95_SINGLESTEP_EVENT */
#endif

    NULL                        /* MAX_EVENT_CODE */
};

/*
 *  This array contains the set of default actions to be taken for
 *      all debug events if the thread has the "In Function Evaluation"
 *      bit set.
 */

DDVECTOR RgfnFuncEventDispatch[] = {
    EvntException,
    NULL,                       /* This can never happen */
    NULL,                       /* This can never happen */
    ProcessExitThreadEvent,
    EvntExitProcess,
    ProcessLoadDLLEvent,        /* Use normal processing */
    ProcessUnloadDLLEvent,      /* Use normal processing */
    ProcessOutputDebugStringEvent, /* Use normal processing */
    NULL,
    EvntBreakpoint,             /* Breakpoint processor */
    NULL,
    ProcessSegmentLoadEvent,    /* WOW event */
    NULL,                       /* DESTROY_PROCESS_DEBUG_EVENT */
    NULL,                       /* DESTROY_THREAD_DEBUG_EVENT */
    NULL,                       /* ATTACH_DEADLOCK_DEBUG_EVENT */
    NULL,                       /* ATTACH_EXITED_DEBUG_EVENT */
    ProcessEntryPointEvent,     /* ENTRYPOINT_DEBUG_EVENT */
    NULL,                       /* LOAD_COMPLETE_DEBUG_EVENT */
    NULL,                       /* INPUT_DEBUG_STRING_EVENT */
    NULL,                       /* MESSAGE_DEBUG_EVENT */
    NULL,                       /* MESSAGE_SEND_DEBUG_EVENT */
    NULL,                       /* FUNC_EXIT_EVENT */
    NULL,                       /* OLE_DEBUG_EVENT */
    NULL,                       /* FIBER_DEBUG_EVENT */
    NULL,                       /* GENERIC_DEBUG_EVENT */
#ifdef KERNEL
    NULL,                       /* BOGUS_WIN95_SINGLESTEP_EVENT */
#else
    ProcessBogusSSEvent,        /* BOGUS_WIN95_SINGLESTEP_EVENT */
#endif
    NULL                        /* MAX_EVENT_CODE */
};

void    UNREFERENCED_PARAMETERS(LPVOID lpv,...)
{
    lpv=NULL;
}

SPAWN_STRUCT          SpawnStruct;          // packet for causing CreateProcess()

DEBUG_ACTIVE_STRUCT   DebugActiveStruct;    // ... for DebugActiveProcess()

#ifndef KERNEL
RELOAD_STRUCT         ReloadStruct;         // for !reload, usermode
#endif

PKILLSTRUCT           KillQueue;
CRITICAL_SECTION      csKillQueue;

BOOL IsExceptionIgnored(HPRCX, DWORD);

TCHAR SearchPathString[ 10000 ];
BOOL SearchPathSet;
BOOL fUseRealName = FALSE;

HINSTANCE hInstance; // The DM DLL's hInstance

LPTSTR
FmtMsg(
    int msgid
    )
{
    void * lpb;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_IGNORE_INSERTS |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      0, 0, msgid, 0, (LPTSTR)&lpb, 0, 0))
    {
        return (LPTSTR) lpb;
    }
    else
    {
        return NULL;
    }
}

#include "resource.h"
TCHAR szErrArg[_MAX_PATH];

LPTSTR
LszFromErr(
    DWORD dwErr
    )
{
    #define NTERR(name) ERROR_##name, IDS_##name,
    static struct {
        DWORD   dwErr;
        UINT    idsErr;
    } mpiszErr [] = {
        NTERR(FILE_NOT_FOUND)
        NTERR(PATH_NOT_FOUND)
        NTERR(INVALID_HANDLE)
        NTERR(INVALID_DRIVE)
        NTERR(INVALID_PARAMETER)
        NTERR(INVALID_NAME)
        NTERR(BAD_PATHNAME)
        NTERR(FILENAME_EXCED_RANGE)
        NTERR(OUTOFMEMORY)
        NTERR(NOT_ENOUGH_MEMORY)
        NTERR(ACCESS_DENIED)
        NTERR(SHARING_VIOLATION)
        NTERR(OPEN_FAILED)
        NTERR(BAD_FORMAT)
        NTERR(CHILD_NOT_COMPLETE)
        NTERR(INVALID_MODULETYPE)
        NTERR(INVALID_EXE_SIGNATURE)
        NTERR(EXE_MARKED_INVALID)
        NTERR(BAD_EXE_FORMAT)
        NTERR(DIRECTORY)
        0,  IDS_UNKNOWN_ERROR
    };
    static TCHAR rgchErr[256+_MAX_PATH]; // must be static!
    static TCHAR rgchErrNum[256];
    LPTSTR sz = rgchErr;
    LPTSTR lszFmtErr = NULL;
    int i;

    if (!LoadString(hInstance, IDS_COULD_NOT_LOAD_DEBUGGEE,
            sz, (int)(rgchErr + _tsizeof(rgchErr) - sz))) {
        assert(FALSE);
    }

    sz += _tcslen(sz);

    // If there's an argument to put in the string, put it here
    if (szErrArg[0]) {
        _tcscpy(sz, szErrArg);
        _tcscat(sz, _T(": "));
        sz += _tcslen(sz);
    }

    for (i = 0; mpiszErr[i].dwErr; ++i) {
        if (mpiszErr[ i ].dwErr == dwErr) {
            break;
        }
    }

    // If we didn't find an error string in our list, call FormatMessage
    // to get an error message from the operating system.
    if (mpiszErr[i].dwErr == 0) {
        lszFmtErr = FmtMsg(dwErr);
    }

    // If we got an error message from the operating system, display that,
    // otherwise display our own message.
    if (lszFmtErr) {
        _tcsncpy(sz, lszFmtErr, (int)(rgchErr + _tsizeof(rgchErr) - sz));
        rgchErr[_tsizeof(rgchErr)-1] = _T('\0');
    }
    else {
        // Even if we got through the above loop without finding a match,
        // we're okay, because the mpiszErr[] table ends with "unknown error".

        if (!LoadString(hInstance, mpiszErr[i].idsErr,
                sz, (int)(rgchErr+_tsizeof(rgchErr)-sz))) {
            assert(FALSE);
        }
    }

    sz += _tcslen(sz);

    if (!LoadString(hInstance, IDS_NTError,
            rgchErrNum, _tsizeof(rgchErrNum))) {
        assert(FALSE);
    }
    sz += _stprintf(sz, rgchErrNum, dwErr);

    return rgchErr;
}

void
SendNTError(
    HPRCX hprc,
    DWORD dwErr,
    LPTSTR lszString
    )
{
    LPTSTR     lszError;

    if (lszString) {
        lszError = lszString;
    }
    else {
        lszError = (LPTSTR) LszFromErr(dwErr);
    }

        SendDBCError(hprc, dwErr, lszError);
}

void
SendDBCError(
    HPRCX hprc,
    DWORD dwErr,
    LPTSTR lszString
    )
{
    typedef struct _LE {
        XOSD    xosd;
        TCHAR   rgchErr[];
    } LE;       // load error: passed with dbcError

    LE FAR *    ple;
    LPRTP       lprtp;
    DWORD       cbBuf;
    LPCTSTR     lszError;

        if (lszString) {
                lszError = lszString;
        }
        else {
                lszError = "";  // Empty string.
        }

    cbBuf = FIELD_OFFSET(RTP, rgbVar) + sizeof(LE) + (_tcslen(lszError) + 1)*sizeof(TCHAR);
    lprtp = (LPRTP) MHAlloc(cbBuf);
    assert(lprtp);
    ple = (LE *) (lprtp->rgbVar);

    ple->xosd = (XOSD) dwErr;
    _tcscpy(ple->rgchErr, lszError);
    lprtp->dbc = dbcError;

    if (hprc != NULL) {
                lprtp->hpid= hprc->hpid;
        } else {
                lprtp->hpid = hpidRoot;
    }

    lprtp->htid = NULL;
    lprtp->cb = cbBuf;
    DmTlFunc(tlfDebugPacket, lprtp->hpid, lprtp->cb, (LPARAM) lprtp);
    MHFree(lprtp);
}

BOOL
ResolveFile(
    LPTSTR   lpName,
    LPTSTR   lpFullName,
    BOOL    fUseRealName
    )
{
    DWORD   dwAttr;
    LPTSTR  lpFilePart;
    BOOL    fOk;

    if (fUseRealName) {
        dwAttr = GetFileAttributes(lpName);
        fOk = ((dwAttr != 0xffffffff)
             && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0));

        if (fOk) {
            _ftcscpy(lpFullName, lpName);
        }

    } else {

        fOk = SearchPath(SearchPathString,
                         lpName,
                         NULL,
                         MAX_PATH,
                         lpFullName,
                         &lpFilePart
                         );
        if (!fOk) {
            *lpFullName = 0;
        }
    }
    return fOk;
}


#ifndef KERNEL
XOSD
Load(
    HPRCX hprc,
    LPCTSTR szAppName,
    LPCTSTR szArg,
    LPVOID pattrib,
    LPVOID tattrib,
    DWORD creationFlags,
    BOOL fInheritHandles,
    CONST LPCTSTR* environment,
    LPCTSTR currentDirectory,
    STARTUPINFO FAR * pStartupInfo,
    LPPROCESS_INFORMATION lppi
    )
/*++

Routine Description:


Arguments:


Return Value:

    TRUE if the process was successfully created and FALSE otherwise.

--*/
{
    XOSD      xosd;
    int       type;
    TCHAR     ch, chTerm;
    int       fQuotedFileName;
    int       l;
    IMAGETYPE Image;
    LPTSTR    lpch;
    // Just spawning an exec, not debugging
    BOOL      fSpawnOrphan =  !(creationFlags & DEBUG_PROCESS || creationFlags & DEBUG_ONLY_THIS_PROCESS);

    static TCHAR szFullName[MAX_PATH];
    static TCHAR szCommandLine[8192];
    static TCHAR szCurrentDir[MAX_PATH]; // Directory to spawn the debuggee in.

    Unreferenced( pattrib );
    Unreferenced( tattrib );
    Unreferenced( creationFlags );

    /* NOTE: Might have to do the same sort of copying for
     * szArg, pattrib, tattrib and
     * startupInfo. Determine if this is necessary.
     */

    //
    // global flag to help with special handling of DOS/WOW apps.
    //

    FLoading16 = FALSE;


    //
    //  Form the command line.
    //

    //
    //  First, we extract the program name and get its full path. Then
    //  we append the arguments.
    //

    if (szAppName[0] == _T('"')) {
        // If the user specified a quoted name (ie: a Long File Name, perhaps?),
        // terminate on the next quote.
        chTerm = _T('"');
        fQuotedFileName=TRUE;
        szAppName++;    // Advance past the quote.
    } else {
        // No Quote.  Search for the first space.
        chTerm = _T(' ');
        fQuotedFileName=FALSE;
    }

    //
    // Null terminate the command line
    //

    if (  (_ftcslen(szAppName) > 2 && szAppName[1] == _T(':'))
        || szAppName[0] == _T('\\')) {

        _ftcscpy(szCommandLine, szAppName);
        fUseRealName = TRUE;

    } else if (_ftcschr(szAppName, _T('\\')) || !SearchPathSet) {

        _ftcscpy(szCommandLine, _T(".\\") );
        _ftcscat(szCommandLine, szAppName );
        fUseRealName = TRUE;

    } else {

        if (!*SearchPathString) {
            _ftcscpy(SearchPathString, _T(".;"));
            l = 2;
            l += GetSystemDirectory(SearchPathString+l,
                                    _tsizeof(SearchPathString)-l);
            SearchPathString[l++] = _T(';');
            l += GetWindowsDirectory(SearchPathString+l,
                                     _tsizeof(SearchPathString)-l);
            SearchPathString[l++] = _T(';');
            GetEnvironmentVariable(_T("PATH"),
                                   SearchPathString+l,
                                   _tsizeof(SearchPathString)-l);
        }

        _ftcscpy(szCommandLine, szAppName);
        fUseRealName = FALSE;
    }

    if (fQuotedFileName) {
        szAppName--;
    }

    //
    // to be similar to the shell, we look for:
    //
    // .COM
    // .EXE
    // nothing
    //
    // since '.' is a valid filename character on many filesystems,
    // we don't automatically assume that anything following a '.'
    // is an "extension."  If the extension is "COM" or "EXE", leave
    // it alone; otherwise try the extensions.
    //

    lpch = _ftcschr(szCommandLine, _T('.'));
    if (lpch &&
             ( lpch[1] == 0
            || _ftcsicmp(lpch, _T(".COM")) == 0
            || _ftcsicmp(lpch, _T(".EXE")) == 0)
    ) {
        lpch = NULL;
    } else {
        lpch = szCommandLine + _ftcslen(szCommandLine);
    }

    *szFullName = 0;
    if (lpch) {
        _ftcscpy(lpch, _T(".COM"));
        ResolveFile(szCommandLine, szFullName, fUseRealName);
    }
    if (!*szFullName && lpch) {
        _ftcscpy(lpch, _T(".EXE"));
        ResolveFile(szCommandLine, szFullName, fUseRealName);
    }
    if (!*szFullName) {
        if (lpch) {
            *lpch = 0;
        }
        ResolveFile(szCommandLine, szFullName, fUseRealName);
    }

    if (!*szFullName) {

        return xosdFileNotFound;

    }


    //
    // Let's do some error checking
    //
    type = pCharMode(szFullName, &Image);
    if (System_DmpInvalid == type) {

        return xosdDumpInvalidFile;

    } else if (System_DmpWrongPlatform == type) {

        return xosdDumpWrongPlatform;

    }  else if (System_Invalid == type) {

        return xosdFileNotFound;

    } else {

        switch ( Image ) {
            case Image_Unknown:
                // treat as a com file
                //return xosdBadFormat;

            case Image_16:
                FLoading16 = TRUE;
#if defined(TARGET_i386)
                if ( (type == System_GUI) &&
                        !(creationFlags & CREATE_SEPARATE_WOW_VDM) &&
                        IsWOWPresent() )
                {
                    // TODO need dbcError here
                    return xosdGeneral;
                }
                break;
#else
                // TODO all platforms will suppport this
                return xosdGeneral;
#endif

            default:
                break;

        }
    }

    creationFlags |= (type?CREATE_NEW_CONSOLE:0);

    //
    // Make sure arg1 (the app's name) is enclosed in quotes
    //
    if ('"' != *szCommandLine) {
        // Prepend a double quote
        PTSTR psz = _ftcsdup(szCommandLine);
        _ftcscpy(szCommandLine, "\"");
        _ftcscat(szCommandLine, psz);
        free(psz);
    }
    if ('"' != szCommandLine[_tcslen(szCommandLine)-1]) {
        // Append a double quote
        _ftcscat(szCommandLine, "\"");
    }

    //
    //  Add rest of arguments
    //
    _ftcscat(szCommandLine, " ");        // Ensure a space just incase
    _ftcscat(szCommandLine, szArg);

    if (Image == Image_Dump) {
        //
        // must be a crash dump file
        //

        if (!StartCrashPollThread()) {
            return xosdUnknown;
        }
        return xosdNone;
    }

    if (!fSpawnOrphan) {
        if (!StartDmPollThread()) {
            return xosdUnknown;
        }
    }

    // Handle the current directory
    if ( currentDirectory == NULL ) {
            TCHAR szDir[_MAX_DIR];
            // szCurrentDir is temporarily used to get the drive letter in it.
            _tsplitpath(szFullName, szCurrentDir, szDir, NULL, NULL);
            // After the _tcscat szCurrentDir will have the right value.
            _tcscat(szCurrentDir, szDir);
    } else {
            _tcscpy(szCurrentDir, currentDirectory);
    }

    ResetEvent(SpawnStruct.hEventApiDone);

    SpawnStruct.szAppName = szFullName;

    xosd = ProcessDebuggeeRedirection (szCommandLine, pStartupInfo);
    if (xosd != xosdNone) {
        return xosd;
    }

    if (pStartupInfo -> dwFlags & STARTF_USESTDHANDLES && !fInheritHandles) {
        // If we're redirecting the debuggee's STDIO, inheritHandles must be set.
        assert (FALSE);
        fInheritHandles = TRUE;
    }

    if (!_ftcslen (szCommandLine)) {
        //
        // In Win 95, if the Args are an empty string,
        // the AppName is interpreted as with arguments.  This fixes it.
        //

        SpawnStruct.szArgs = NULL;

    } else {
        //
        // parse any IO redirection
        //
        SpawnStruct.szArgs = szCommandLine;
    }

    if (!_ftcslen(szCurrentDir)) {
        SpawnStruct.pszCurrentDirectory = NULL;
    } else {
        SpawnStruct.pszCurrentDirectory = szCurrentDir;
    }

    SpawnStruct.fdwCreate = creationFlags;
    SpawnStruct.si        = *pStartupInfo;
    SpawnStruct.fInheritHandles = fInheritHandles;

    //
    // The second argument to get command line should have both the exe name
    // and the command line args. GetCommandLine & argv[] are based on the second
    // argument and we need the exe name to appear there.
    //

    if (fSpawnOrphan) {
        //
        // If we're not debugging, it's because we're doing an Execute
        //
        // (aka SpawnOrphan), so just do it.
        //
        SpawnStruct.fReturn = CreateProcess(
            SpawnStruct.szAppName,
            SpawnStruct.szArgs,
            NULL,
            NULL,
            SpawnStruct.fInheritHandles,
            SpawnStruct.fdwCreate,
            NULL,
            SpawnStruct.pszCurrentDirectory,
            &SpawnStruct.si,
            lppi);
        Close3Handles(rgh);
        if (!SpawnStruct.fReturn) {
            SpawnStruct.dwError = GetLastError();
        }
    } else {
        //
        //  This is a semaphore!  Set it last!
        //

        SpawnStruct.fSpawn    = TRUE;

        if (WaitForSingleObject( SpawnStruct.hEventApiDone, INFINITE ) != 0) {
            SpawnStruct.fReturn = FALSE;
            SpawnStruct.dwError = GetLastError();
        }
    }


    if (SpawnStruct.fReturn) {

        xosd = xosdNone;

    } else {

        DPRINT(1, (_T("Failed.\n")));

        xosd = xosdGeneral;
        // make a dbcError with SpawnStruct.dwError
        SendNTError(hprc, SpawnStruct.dwError, NULL);

    }

    return xosd;
}

#endif  // !KERNEL


HPRCX
InitProcess(
    HPID hpid
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    HPRCX   hprc;

    /*
     * Create a process structure, place it
     * at the head of the master list.
     */

    hprc = (HPRCX)MHAlloc(sizeof(HPRCXSTRUCT));
    memset(hprc, 0, sizeof(*hprc));

    EnterCriticalSection(&csThreadProcList);

    hprc->next          = prcList->next;
    prcList->next       = hprc;
    hprc->hpid          = hpid;
    hprc->exceptionList = NULL;
    hprc->pid           = (PID)-1;      // Indicates prenatal process
    hprc->pstate        = 0;
    hprc->cLdrBPWait    = 0;
    hprc->hExitEvent    = CreateEvent(NULL, FALSE, FALSE, NULL);
    hprc->hEventCreateThread = CreateEvent(NULL, TRUE, TRUE, NULL);
    hprc->f16bit        = FALSE;

#ifndef KERNEL
    hprc->pFbrCntx      = NULL;  //Ignore Fibers
    hprc->FbrLst        = NULL;
    hprc->OrpcDebugging = ORPC_NOT_DEBUGGING;
    hprc->dwKernel32Base = 0;
    hprc->llnlg = LLInit(sizeof ( NLG ), llfNull, NULL, NLGComp );
#endif

    InitExceptionList(hprc);

    LeaveCriticalSection(&csThreadProcList);

    return hprc;
}


void
ActionDebugNewReady(
    DEBUG_EVENT64 * pde,
    HTHDX hthd,
    DWORDLONG unused,
    ULONG64 lparam
    )
/*++

Routine Description:

    This function is called when a new child process is ready to run.
    The process is in exactly the same state as in ActionAllDllsLoaded.
    However, in this case the debugger is not waiting for a reply.

Arguments:


Return Value:

--*/
{
    HPRCX hprc = (HPRCX)lparam;
    XOSD    xosd = xosdNone;
#if defined(INTERNAL)
    LPBYTE lpbPacket;
    WORD   cbPacket;
    PDLL_DEFER_LIST pddl;
    PDLL_DEFER_LIST pddlT;
    DEBUG_EVENT64 de;
#endif

    DPRINT(5, (_T("Child finished loading\n")));

#ifdef TARGET_i386
    hthd->fContextDirty = FALSE;  // Undo the change made in ProcessDebugEvent
#endif

    hprc->pstate &= ~ps_preStart;       // Clear the loading state flag
    hprc->pstate |=  ps_preEntry;       // next stage...
    hthd->tstate |=  ts_stopped;        // Set that we have stopped on event
    --nWaitingForLdrBreakpoint;

#if 0
    SendDllLoads(hthd);
#endif

#if defined(INTERNAL)
    hprc->fNameRequired = TRUE;

    for (pddl = hprc->pDllDeferList; pddl; pddl = pddlT) {

        pddlT = pddl->next;

        de.dwDebugEventCode        = LOAD_DLL_DEBUG_EVENT;
        de.dwProcessId             = pde->dwProcessId;
        de.dwThreadId              = pde->dwThreadId;
        de.u.LoadDll               = pddl->LoadDll;

        if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) && (cbPacket != 0)) {
            NotifyEM(&de, hthd, cbPacket, lpbPacket);
        }

        MHFree(pddl);
    }
    hprc->pDllDeferList = NULL;
#endif

    /*
     * Prepare to stop on thread entry point
     */

    SetupEntryBP(hthd);

    /*
     * leave it stopped and notify the debugger.
     */
#if defined(TARGET_ALPHA) || defined(TARGET_AXP64) || defined(TARGET_IA64)
    SetBPFlag(hthd, EMBEDDED_BP);
#endif
    pde->dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;

    NotifyEM(pde, hthd, 0, 0L);

    return;
}                                       /* ActionDebugNewReady() */


void
ActionDebugActiveReady(
    DEBUG_EVENT64 * pde,
    HTHDX hthd,
    DWORDLONG unused,
    ULONG64 lparam
    )
/*++

Routine Description:

    This function is called when a newly attached process is ready to run.
    This process is not the same as the previous two.  It is either running
    or at an exception, and a thread has been created by DebugActiveProcess
    for the sole purpose of hitting a breakpoint.

    If we have an event handle, it needs to be signalled before the
    breakpoint is continued.

Arguments:


Return Value:

--*/
{
    HPRCX hprc = (HPRCX)lparam;
    XOSD    xosd = xosdNone;

    DPRINT(5, (_T("Active process finished loading\n")));

#ifdef TARGET_i386
    hthd->fContextDirty = FALSE;  // Undo the change made in ProcessDebugEvent
#endif // i386

    hprc->pstate &= ~ps_preStart;
    hthd->tstate |=  ts_stopped;    // Set that we have stopped on event
    --nWaitingForLdrBreakpoint;

#if 0
    // BUGBUG kentf this is not wise.  As implemented, this will run code
    // in the crashed debuggee, calling LoadLibrary and other foolish things.
    SendDllLoads(hthd);
#endif

    /*
     * If this is a crashed process, tell the OS
     * to raise the exception.
     * Tell the EM that we are finished loading;
     * it will say GO, and we will catch the exception
     * soon after.
     */

    if (pde->dwProcessId == DebugActiveStruct.dwProcessId) {
        if (DebugActiveStruct.hEventGo) {
            SetEvent(DebugActiveStruct.hEventGo);
            CloseHandle(DebugActiveStruct.hEventGo);
        }
        DebugActiveStruct.dwProcessId = 0;
        DebugActiveStruct.hEventGo = 0;
        if (DebugActiveStruct.hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(DebugActiveStruct.hProcess);
            DebugActiveStruct.hProcess = INVALID_HANDLE_VALUE;
        }
        SetEvent(DebugActiveStruct.hEventReady);
    }

#if defined(TARGET_ALPHA) || defined(TARGET_AXP64) || defined(TARGET_IA64)
    SetBPFlag(hthd, EMBEDDED_BP);
#endif
    pde->dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;

    NotifyEM(pde, hthd, 0, 0L);

    return;
}                                       /* ActionDebugActiveReady() */


void
ActionEntryPoint16(
    DEBUG_EVENT64 * pde,
    HTHDX           hthdx,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
/*++

Routine Description:

    This is the registered event routine called when vdm
    sends a DBG_TASKSTART notification.

Arguments:


Return Value:

    None

--*/
{
    hthdx->hprc->pstate &= ~ps_preEntry;
    hthdx->tstate |= ts_stopped;
    NotifyEM(pde, hthdx, 0, ENTRY_BP);
}


void
ActionEntryPoint(
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
/*++

Routine Description:

    This is the registered event routine called when the base
    exe's entry point is executed.  The action we take here
    depends on whether we are debugging a 32 bit or 16 bit exe.

Arguments:

    pde     - Supplies debug event for breakpoint

    hthd    - Supplies descriptor for thread that hit BP

    unused  - unused

    lparam  - unused

Return Value:

    None

--*/
{
    PBREAKPOINT pbp;

    pbp = AtBP(hthd);
    assert(pbp);
    RemoveBP(pbp);

    // the main reason we are here.
    ExprBPRestoreDebugRegs(hthd);

    if (hthd->hprc->f16bit) {

        //
        // if this is a 16 bit exe, continue and watch for
        // the task start event.
        //

        ContinueThread(hthd);

    } else {
        // if this is a 32 bit exe, stay stopped and notify the EM
        hthd->hprc->pstate &= ~ps_preEntry;
        hthd->tstate |= ts_stopped;
        pde->dwDebugEventCode = ENTRYPOINT_DEBUG_EVENT;
        NotifyEM(pde, hthd, 0, ENTRY_BP);
        DMSqlStartup( hthd->hprc );
    }
}


void
HandleDebugActiveDeadlock(
    HPRCX hprc,
    BOOL Exited
    )
{
    DEBUG_EVENT64 de;
    HTHDX   hthd;

    // This timed out waiting for the loader
    // breakpoint.  Clear the prestart state,
    // and tell the EM we are screwed up.
    // The shell should then stop waiting for
    // the loader BP.

    hprc->pstate &= ~ps_preStart;
    --nWaitingForLdrBreakpoint;
    ConsumeAllProcessEvents(hprc, TRUE);

    if (hprc->pid == DebugActiveStruct.dwProcessId) {
        if (DebugActiveStruct.hEventGo) {
            SetEvent(DebugActiveStruct.hEventGo);
            CloseHandle(DebugActiveStruct.hEventGo);
        }
        DebugActiveStruct.dwProcessId = 0;
        DebugActiveStruct.hEventGo = 0;
        if (DebugActiveStruct.hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(DebugActiveStruct.hProcess);
            DebugActiveStruct.hProcess = INVALID_HANDLE_VALUE;
        }
        SetEvent(DebugActiveStruct.hEventReady);
    }

    de.dwDebugEventCode      = Exited? ATTACH_EXITED_DEBUG_EVENT: ATTACH_DEADLOCK_DEBUG_EVENT;
    de.dwProcessId           = hprc->pid;
    hthd = hprc->hthdChild;
    if (hthd) {
        de.dwThreadId = hthd->tid;
    } else {
        de.dwThreadId = 0;
    }
    NotifyEM(&de, hthd, 0, 0);

}                                   /* HandleDebugActiveDeadlock() */


BOOL
SetupSingleStep(
    HTHDX hthd,
    BOOL  DoContinue
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    hthd        -   Supplies The handle to the thread which is to
                    be single stepped
    DoContinue  -   Supplies continuation flag

Return Value:

    TRUE if successfly started step and FALSE otherwise

--*/
{
    PBREAKPOINT         pbp;
    ADDR                addr;
#if defined(NO_TRACE_FLAG)

    /*
     *  Set a breakpoint at the next legal offset and mark the breakpoint
     *  as being for a single step.
     */

    AddrInit(&addr, 0, 0, GetNextOffset(hthd, FALSE), TRUE, TRUE, FALSE, FALSE);
    pbp = SetBP( hthd->hprc, hthd, bptpExec, bpnsStop, &addr, (HPID) INVALID);
    if ( pbp != NULL ) {
        pbp->isStep++;
    }

    /*
     * Now issue the command to execute the child
     */

    if ( DoContinue ) {
        ContinueProcess (hthd->hprc);
    }


#else   // NO_TRACE_FLAG


#if defined(TARGET_i386)
#ifndef KERNEL
    //
    //  Set the single step flag in the context and then start the
    //  thread running
    //
    //  Modify the processor flags in the child's context
    //

    //
    // If we are passing an exception on to the child we cannot set the
    // trace flag, so do the non-trace version of this code.
    //

    if (!PassingExceptionForThisThread (hthd)) {
        hthd->context.EFlags |= TF_BIT_MASK;
        hthd->fContextDirty = TRUE;
    } else {
        int lpf = 0;

        hthd->context.EFlags &= ~TF_BIT_MASK;
        hthd->fContextDirty = TRUE;

        //
        //  this sequence initializes addr to the addr after the current inst
        //

        AddrFromHthdx (&addr, hthd);
        IsCall (hthd, &addr, &lpf, FALSE);

        pbp = SetBP( hthd->hprc, hthd, bptpExec, bpnsStop, &addr, (HPID) INVALID);
        if ( pbp != NULL ) {
            pbp->isStep++;
        }
    }

#endif  // KERNEL

#elif defined(TARGET_IA64)
#ifndef KERNEL
    //
    //  Set the single step flag in the context and then start the
    //  thread running
    //
    //  Modify the processor flags in the child's context
    //

    //
    // If we are passing an exception on to the child we cannot set the
    // trace flag, so do the non-trace version of this code.
    //

    if (!PassingExceptionForThisThread (hthd)) {
        hthd->context.StIPSR |= TF_BIT_MASK;
        hthd->fContextDirty = TRUE;
    } else {
        int lpf = 0;

        hthd->context.StIPSR &= ~TF_BIT_MASK;
        hthd->fContextDirty = TRUE;

        //
        //  this sequence initializes addr to the addr after the current inst
        //

        AddrFromHthdx (&addr, hthd);
        IsCall (hthd, &addr, &lpf, FALSE);

        pbp = SetBP( hthd->hprc, hthd, bptpExec, bpnsStop, &addr, (HPID) INVALID);
        if ( pbp != NULL ) {
            pbp->isStep++;
        }
    }

#endif  // KERNEL

#else   // i386

#error "Need code for new CPU with trace bit"

#endif  // i386

    //
    // Now issue the command to execute the child
    //

    if ( DoContinue ) {
        ContinueThreadEx(hthd,
                         (DWORD)DBG_CONTINUE,
                         QT_TRACE_DEBUG_EVENT,
                         ts_running);
    }

#endif  // NO_TRACE_FLAG

    return TRUE;
}                                       /*  SetupSingleStep() */

VOID
ActionReturnStep(
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    ULONG64         lparam
    )
/*++
        lpv - stack pointer at the top of the call
--*/
{
    BREAKPOINT* pbp = AtBP(hthd);

    assert(pbp);
    if (STACK_POINTER(hthd) > lparam) {
        RemoveBP(pbp);
        pde->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
        pde->u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_SINGLE_STEP;
        //ExprBPStop(hthd->hprc, hthd);
        NotifyEM(pde, hthd, 0, 0);
    } else {
        METHOD *ContinueSSMethod;
        assert(pbp != EMBEDDED_BP);
        assert(pbp != NULL);
        ClearBPFlag(hthd);
        RestoreInstrBP(hthd, pbp);
        ContinueSSMethod = (METHOD*) MHAlloc(sizeof(METHOD));
        ContinueSSMethod->notifyFunction = MethodContinueSS;
        ContinueSSMethod->lparam = (ULONG64)ContinueSSMethod;
        ContinueSSMethod->lparam2 = pbp;
        /* Reenable */
        RegisterExpectedEvent(hthd->hprc, hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (DWORD_PTR) pbp,
                              DONT_NOTIFY,
                              ActionReturnStep,
                              FALSE,
                              lparam
                              );
        SingleStepEx(hthd, ContinueSSMethod, FALSE, FALSE, TRUE);
    }
}

BOOL
SetupReturnStep(
    HTHDX hthd,
    BOOL  DoContinue,
    LPADDR lpaddr,
    LPADDR addrStack
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    hthd        -   Supplies The handle to the thread which is to
                    be single stepped

    DoContinue  -   Supplies continuation flag

    lpaddr      -   Supplies the address to step to

    addrStack    -   Supplies the address for SP.
Return Value:

    TRUE if successfully started step and FALSE otherwise

--*/
{
    BREAKPOINT *        pbp;

    /*
     */

    pbp = SetBP( hthd->hprc, hthd, bptpExec, bpnsStop, lpaddr, (HPID)INVALID);

        if ( !pbp ) {
                // On Win95 we might not be able to set the bp because
                // we might be in a callback and returning to system code.
                assert(IsChicago());
                if (IsInSystemDll(GetAddrOff(*lpaddr))) {
                        SendDBCErrorStep(hthd->hprc);
                } else {
                        assert(FALSE); // Shouldn't happen any other time.
                }

                return FALSE;
        }


    if (addrStack) {
        RegisterExpectedEvent(hthd->hprc, hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (DWORD_PTR)pbp,
                              DONT_NOTIFY,
                              ActionReturnStep,
                              FALSE,
                              GetAddrOff(*addrStack));
    }

    /*
     * Now issue the command to execute the child
     */

    if ( DoContinue ) {
        ExprBPContinue( hthd->hprc, hthd );
        ContinueThread (hthd);
    }

    return TRUE;
}                                       /*  SetupReturnStep() */



void
SetupEntryBP(
    HTHDX   hthd
    )
/*++

Routine Description:

    Set a breakpoint and make a persistent expected event for the
    entry point of the first thread in a new process.

Arguments:

    hthd    - Supplies descriptor for thread to act on.

Return Value:

    None

--*/
{
    ADDR            addr;
    BREAKPOINT    * bp;

    AddrInit(&addr,
             0,
             0,
             hthd->lpStartAddress,
             TRUE,
             TRUE,
             FALSE,
             FALSE);

#if defined(TARGET_IA64)
    {

        ULONGLONG ullPLabel[2];
        DWORD cb;
        DPRINT(1,(_T("Reading plabel at 0x%I64x-0x%I64x\n"),
                  hthd->lpStartAddress,
                  GetAddrOff(addr)));
        if (!AddrReadMemory(hthd->hprc, hthd, &addr, &ullPLabel,sizeof(ullPLabel),&cb)) {
            DPRINT(0,(_T("Could not read the plabel at 0x%I64x -- cb = %ld"),GetAddrOff(addr),cb));
        } else {
            DPRINT(1,("Dereferenced Entry BP from 0x%I64x (plabel?) to 0x%I64x(0x%I64x)\n",
                      GetAddrOff(addr),
                      (UOFFSET) ullPLabel[0],
                      (UOFFSET) ullPLabel[1]));
            GetAddrOff(addr) = (UOFFSET) ullPLabel[0];
        }
    }
#endif

    bp = SetBP(hthd->hprc, hthd, bptpExec, bpnsStop, &addr, (HPID)ENTRY_BP);

    // register expected event
    RegisterExpectedEvent(hthd->hprc,
                          hthd,
                          BREAKPOINT_DEBUG_EVENT,
                          (DWORD_PTR)bp,
                          DONT_NOTIFY,
                          ActionEntryPoint,
                          TRUE,     // Persistent!
                          0
                         );
}                                   /* SetupEntryBP() */


#ifdef KERNEL
VOID
RestoreKernelBreakpoints (
    HTHDX   hthd,
    ULONG64  Offset
    )
/*++

Routine Description:

    Restores all breakpoints in our bp list that fall in the range of
    offset -> offset+dbgkd_maxstream.  This is necessary because the kd
    stub in the target system clears all breakpoints in this range before
    delivering an exception to the debugger.

Arguments:

    hthd    - handle to the current thread

    Offset  - beginning of the range, usually the current pc

Return Value:

    None

--*/
{

    BREAKPOINT               *pbp;
    DBGKD_WRITE_BREAKPOINT64 bps[MAX_KD_BPS];
    DWORD                    i = 0;


#if defined(TARGET_IA64)
    return; // IA64 version of KdpSetStateChange() will reinstall kernel breakpoints instead.
#endif

    EnterCriticalSection(&csThreadProcList);

    ZeroMemory( bps, sizeof(bps) );

    for (pbp=bpList->next; pbp; pbp=pbp->next) {

        if (GetAddrOff(pbp->addr) >= Offset &&
            GetAddrOff(pbp->addr) <  Offset+DBGKD_MAXSTREAM) {
            if (i < MAX_KD_BPS) {
                bps[i++].BreakPointAddress = GetAddrOff(pbp->addr);
            }
        }
    }

    if (i) {
        WriteBreakPointEx( hthd, i, bps, 0 );

        for (i=0,pbp=bpList->next; pbp; pbp=pbp->next) {

            if (GetAddrOff(pbp->addr) == bps[i].BreakPointAddress) {
                pbp->hBreakPoint = bps[i++].BreakPointHandle;

            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);
}
#endif // KERNEL


#ifndef KERNEL

void
UpdateThreadContext(
    HTHDX   hthd
    )
/*++

Routine Description:

    This routine updates a thread's context, including real/32bit modes and
    context dirty flags.

--*/
{
#if defined(TARGET_i386)
    LDT_ENTRY           ldtEntry;
#endif

    hthd->context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    DbgGetThreadContext( hthd, &hthd->context );
    hthd->fContextDirty = FALSE;
    hthd->fIsCallDone = FALSE;
#if !defined( TARGET_i386 )
    hthd->fAddrIsReal = FALSE;
    hthd->fAddrIsFlat = hthd->fAddrOff32 = TRUE;
#else // !i386
    if (hthd->context.EFlags & V86FLAGS_V86) {
        hthd->fAddrIsReal  = TRUE;
        hthd->fAddrIsFlat  = FALSE;
        hthd->fAddrOff32   = FALSE;
    } else {
        hthd->fAddrIsReal  = FALSE;
        if (PcSegOfHthdx(hthd) == hthd->hprc->segCode) {
            hthd->fAddrIsFlat = hthd->fAddrOff32 = TRUE;
        } else {
            hthd->fAddrIsFlat = FALSE;
            if (!GetThreadSelectorEntry(hthd->rwHand,
                                        PcSegOfHthdx(hthd),
                                        &ldtEntry)) {
                hthd->fAddrOff32 = FALSE;
            } else if (!ldtEntry.HighWord.Bits.Default_Big) {
                hthd->fAddrOff32 = FALSE;
            } else {
                hthd->fAddrOff32 = TRUE;
            }
        }
    }
#endif // !i386
}


void
ProcessDebugEvent(
    DEBUG_EVENT64 *  de
    )
/*++

Routine Description:

    This routine is called whenever a debug event notification comes from
    the operating system.

Arguments:

    de      - Supplies a pointer to the debug event which just occured

Return Value:

    None.

--*/

{
    EXPECTED_EVENT *    ee;
    DWORD               eventCode = de->dwDebugEventCode;
    DWORD_PTR           subClass = 0L;
    HTHDX               hthd = NULL;
    HPRCX               hprc;
    BREAKPOINT *        bp;
    ADDR                addr;
    BP_UNIT             instr;
    DWORD               len;
    BOOL                fInstrIsBp;

    DPRINT(3, (_T("Event Code == %x\n"), eventCode));

    hprc = HPRCFromPID(de->dwProcessId);

    /*
     * While killing process, ignore everything
     * except for exit events.
     */

    if (hprc) {
        hprc->cLdrBPWait = 0;
    }

    if ( hprc && (hprc->pstate & ps_killed) ) {
        if (eventCode == EXCEPTION_DEBUG_EVENT) {

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;

        } else if (eventCode != EXIT_THREAD_DEBUG_EVENT
          && eventCode != EXIT_PROCESS_DEBUG_EVENT ) {

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;
        }
    }

    EnterCriticalSection(&csProcessDebugEvent);

    if (eventCode == CREATE_THREAD_DEBUG_EVENT){

        DPRINT(3, (_T("*** NEW TID = (PID,TID)(%08lx, %08lx)\n"),
                      de->dwProcessId, de->dwThreadId));

    } else {

        /*
         *  Find our structure for this event's process
         */

        DEBUG_PRINT(_T("Not Create Thread Debug Event\r\n"));
        hthd = HTHDXFromPIDTID((PID)de->dwProcessId,(TID)de->dwThreadId);

        //
        //  Update the context for all threads, not just the main one.  We
        //  may be able to do this a little quicker by using the
        //  fContextDirty flag.
        //

        //
        //  First, update the main threads ExceptionRecord if necessary

        if (hthd && eventCode == EXCEPTION_DEBUG_EVENT) {
            hthd->ExceptionRecord = de->u.Exception.ExceptionRecord;
        }

        //
        //  Loop through all threads and update contexts

        if (hthd && hprc) {
            HTHDX   hthdT;
            //
            // If for any reason this is set unset it.
            //
#ifndef KERNEL
            hprc->pFbrCntx = NULL;
#endif

            for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
                UpdateThreadContext (hthdT);
            }

        } else if (hprc && (hprc->pstate & ps_killed)) {

            /*
             * this is an event for a thread that
             * we never created:
             */
            if (eventCode == EXIT_PROCESS_DEBUG_EVENT) {
                /* Process exited on a thread we didn't pick up */
                ProcessExitProcessEvent(de, NULL);
            } else {
                /* this is an exit thread for a thread we never picked up */
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          de->dwProcessId,
                          de->dwThreadId,
                          DBG_CONTINUE,
                          0);
            }
            goto done;

        } else if (eventCode!=CREATE_PROCESS_DEBUG_EVENT) {

            //
            // This will happen sometimes when killing a process with
            // ProcessTerminateProcessCmd and ProcessUnloadCmd.
            //

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      DBG_CONTINUE,
                      0);
            goto done;

        }
    }

    //
    //  Mark the thread as having been stopped for some event.
    //

    if (hthd) {
        hthd->tstate &= ~ts_running;
        hthd->tstate |= ts_stopped;
    }

    /* If it is an exception event get the subclass */

    if (eventCode == EXCEPTION_DEBUG_EVENT) {

        subClass = de->u.Exception.ExceptionRecord.ExceptionCode;
        DPRINT(1, (_T("Exception Event: subclass = %p    "), subClass));

        switch (subClass) {
        case STATUS_SEGMENT_NOTIFICATION:
            eventCode = de->dwDebugEventCode = SEGMENT_LOAD_DEBUG_EVENT;
            break;

        case EXCEPTION_SINGLE_STEP:

#if !defined(TARGET_i386) && !defined(TARGET_IA64)
            assert(_T("!EXCEPTION_SINGLE_STEP on non-x86!"));
#endif

#if defined(TARGET_IA64)
            //
            // v-vadimp - looks like Merced doesn't reset PSR.ss (analog of
            // EFLAG.tf) after each SS exception, we have to do it manually
            //
            hthd->fContextDirty = TRUE;
            hthd->context.StIPSR &= ~TF_BIT_MASK;
#endif

            AddrFromHthdx(&addr, hthd);

            //
            // This may be a single step or a hardware breakpoint.
            // If it is a single step, leave it at that.  If it is a
            // hardware breakpoint, convert it to a BREAKPOINT_DEBUG_EVENT.
            //

            DecodeSingleStepEvent( hthd, de, &eventCode, &subClass );
            break;

        case EXCEPTION_BREAKPOINT:

            /*
             * Check if it is a BREAKPOINT exception:
             * If it is, change the debug event to our pseudo-event,
             * BREAKPOINT_DEBUG_EVENT (this is a pseudo-event because
             * the API does not define such an event, and we are
             * synthesizing not only the class of event but the
             * subclass as well -- the subclass is set to the appropriate
             * breakpoint structure)
             */

            AddrFromHthdx(&addr, hthd);
#if defined(TARGET_i386)
            //
            // correct for machine overrun on a breakpoint
            //
            // On NT the offset value is overrun by 1 every time except
            // for when doing post-mortem debugging. The value in the
            // Debug Event structure is always correct though
            //

            addr.addr.off = de->u.Exception.ExceptionRecord.ExceptionAddress ;
#endif

            //
            // Determine the start of the breakpoint instruction
            //

            if ((AddrReadMemory(hprc, hthd, &addr, &instr, BP_SIZE, &len) == 0)
                            || (len != BP_SIZE)) {
                DPRINT(0, ("Memory read failed @ %I64x!!!\n",GetAddrOff(addr)));
                assert(FALSE);
                instr = 0;
            }
#if defined(TARGET_ALPHA) || defined(TARGET_AXP64)

            switch (instr) {
                case CALLPAL_OP | CALLKD_FUNC:
                case CALLPAL_OP |    BPT_FUNC:
                case CALLPAL_OP |   KBPT_FUNC:
                     fInstrIsBp = TRUE;
                             break;
                        default:
                             fInstrIsBp = FALSE;
                    }
            DPRINT(3, (_T("Looking for BP@%I64x\n"), addr.addr.off));

#elif defined(TARGET_i386)

            /*
             *  It may have been a 0xcd 0x03 rather than a 0xcc
             *  (ie: check if it is a 1 or a 2 byte INT 3)
             */

            fInstrIsBp = FALSE;
            if (instr == BP_OPCODE) {
                fInstrIsBp = TRUE;
            } else if (instr == 0x3) { // 0xcd?
                --addr.addr.off;
                if (AddrReadMemory(hprc,
                                  hthd,
                                  &addr,
                                  &instr,
                                  1,
                                  &len)
                     && (len == 1)
                     && (instr == 0xcd)) {

                    fInstrIsBp = TRUE;
                } else {
                    ++addr.addr.off;
                }
            }

            Set_PC(hthd, addr.addr.off);
            hthd->fContextDirty = TRUE;

            DPRINT(3, (_T("Looking for BP@%I64x (instr=%I64x)\n"), addr.addr.off,
                        (ULONGLONG)instr));

#elif defined(TARGET_IA64)

        {
            BP_UNIT ia64_opcode;

            //
            // EM instruction slots are not byte-aligned. Break instruction
            // (BP_OPCODE) must be shifted according to the instruction slot
            // before comparison
            //
            switch (GetAddrOff(addr) & 0xf) {
                case 0:
                    ia64_opcode = (instr & (INST_SLOT0_MASK)) >> 5;
                    break;

                case 4:
                    ia64_opcode = (instr & (INST_SLOT1_MASK)) >> 14;
                    break;

                case 8:
                    ia64_opcode = (instr & (INST_SLOT2_MASK)) >> 23;
                    break;

                default:
                    assert(!"Bad BP Address");
                    ia64_opcode = 0;
                break;
            }

            fInstrIsBp = (ia64_opcode == BP_OPCODE) || (instr == 0);
            DPRINT(1, (_T("BP test: opcode:%I64x=BP_OPCODE:%x,BP?:%i @ address %I64x\n"),
                        ia64_opcode,
                        BP_OPCODE,
                        fInstrIsBp,
                        addr.addr.off));

        }

#else
#error( "unknown processor type" )
#endif

            /*
             *  Lookup the breakpoint in our (the dm) table
             */

            bp = FindBP(hthd->hprc, hthd, bptpExec, (BPNS)-1, &addr, FALSE);
            //
            //  We need to be able to find BPs for message BPs as well

            if (bp == NULL) {
                bp = FindBP (hthd->hprc,
                             hthd,
                             bptpMessage,
                             (BPNS) -1,
                             &addr,
                             FALSE);
            }

            SetBPFlag(hthd, bp?bp:EMBEDDED_BP);

            if (!bp && !fInstrIsBp) {
                //
                // If the instruction is not a bp, and there is no record of
                // the bp, this happened because the exception was already
                // in the queue when we cleared the bp.
                //
                // We will just continue it.
                //
                DPRINT(1, (_T("Continuing false BP.\n")));
                ContinueThread(hthd);
                goto done;
            }


            //
            // Q: What does it mean if we find the bp record, but the
            // instruction is not a bp???
            //
            // A: It means that this thread hit the BP after another
            // thread hit it, but before this thread could be suspended
            // by the debug subsystem.  The debugger cleared the BP
            // temporarily in order that the other thread could run
            // past it, and will put it back as soon as that thread
            // hits the temp BP on the next instruction.
            //
            //assert(!bp || fInstrIsBp);

            /*
             *  Reassign the event code to our pseudo-event code
             */
            DPRINT(3, (_T("Reassigning event code!\n")));

            /*
             *  For some machines there is not single instruction tracing
             *  on the chip.  In this case we need to do it in software.
             *
             *  Check to see if the breakpoint we just hit was there for
             *  doing single step emulation.  If so then remap it to
             *  a single step exception.
             */

            if (bp && bp->isStep){
                de->u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_SINGLE_STEP;
                subClass = EXCEPTION_SINGLE_STEP;
                RemoveBP(bp);
                break;
            }

            /*
             * Reassign the subclass to point to the correct
             * breakpoint structure and clear the exception code
             *
             */

            de->dwDebugEventCode = eventCode = BREAKPOINT_DEBUG_EVENT;
            de->u.Exception.ExceptionRecord.ExceptionAddress = GetAddrOff(addr);
            de->u.Exception.ExceptionRecord.ExceptionCode =
                    (DWORD)(DWORD_PTR)bp;
            de->u.Exception.ExceptionRecord.NumberParameters =
                    (DWORD)((DWORD_PTR)bp>>32);
            subClass = (DWORD_PTR)bp;

            break;


        case EXCEPTION_ORPC_DEBUG:
            {
                ORPC orpc;

                // Is this really an OLE notification?
                if ((orpc = OrpcFromPthd(hthd, de)) != orpcNil) {
                    if (orpc == orpcUnrecognized) {
                        // Unrecognized notification.  Resume execution.
                        ContinueThread(hthd);
                        return;
                    }

                    /*
                    ** Reassign the event code to our pseudo-event code
                    */
                    de->dwDebugEventCode = eventCode = OLE_DEBUG_EVENT;

                    /*
                    ** Reassign the exception code to the type of OLE
                    ** event that occurred.
                    */
                    de->u.Exception.ExceptionRecord.ExceptionCode = orpc;
                    subClass = orpc;
                }
                break;
            }
        case EXCEPTION_FIBER_DEBUG:
            {
                //
                // If fibers are in use change to fiber event
                //
                if(hprc->fUseFbrs) {
                    de->dwDebugEventCode = eventCode = FIBER_DEBUG_EVENT;
                }
                break;
            }
        }
    }

    /*
     *  Check if this debug event was expected
     */

    ee = PeeIsEventExpected(hthd, eventCode, subClass, TRUE);


    /*
     * If it wasn't, clear all consummable events
     * and then run the standard handler with
     * notifications going to the execution model
     */

    assert((0 < eventCode) && (eventCode < MAX_EVENT_CODE));

    if (!ee) {

        if ((hthd != NULL) && (hthd->tstate & ts_funceval)) {
            assert(RgfnFuncEventDispatch[eventCode-EXCEPTION_DEBUG_EVENT]);
            RgfnFuncEventDispatch[eventCode-EXCEPTION_DEBUG_EVENT](de, hthd);
        } else {
            assert(DebugDispatchTable[eventCode-EXCEPTION_DEBUG_EVENT]);
            DebugDispatchTable[eventCode-EXCEPTION_DEBUG_EVENT](de,hthd);
        }

    } else {

        /*
         *  If it was expected then call the action
         * function if one was specified
         */

        if (ee->action) {
            DPRINT(3,("Calling action @ %p\n",ee->action));
            (ee->action)(de, hthd, 0, ee->lparam);
        }

        /*
         *  And call the notifier if one was specified
         */

        if (ee->notifier) {
            METHOD  *nm = ee->notifier;
            DPRINT(3,("Calling notifier @ %p\n",nm));
            (nm->notifyFunction)(de, hthd, 0, nm->lparam);
        }

        MHFree(ee);
    }

done:

    LeaveCriticalSection(&csProcessDebugEvent);
    return;
}                               /* ProcessDebugEvent() */



#else // KERNEL




void
ProcessDebugEvent(
    DEBUG_EVENT64             *de,
    DBGKD_WAIT_STATE_CHANGE64 * sc // Not used anymore
    )
/*++

Routine Description:

    This routine is called whenever a debug event notification comes from
    the operating system.

Arguments:

    de      - Supplies a pointer to the debug event which just occured

Return Value:

    None.

--*/

{
    EXPECTED_EVENT *    ee;
    DWORD               eventCode = de->dwDebugEventCode;
    DWORD_PTR           subClass = 0;
    HTHDX               hthd = NULL;
    HPRCX               hprc;
    PBREAKPOINT         bp;
    ADDR                addr;
    DWORD               cb;
    BP_UNIT             instr;
    BOOL                fInstrIsBp = FALSE;


    DPRINT(3, (_T("Event Code == %x\n"), eventCode));

    hprc = HPRCFromPID(de->dwProcessId);

    /*
     * While killing process, ignore everything
     * except for exit events.
     */

    if (hprc) {
        hprc->cLdrBPWait = 0;
    }

    if ( hprc && (hprc->pstate & ps_killed) ) {
        if (eventCode == EXCEPTION_DEBUG_EVENT) {

            ContinueThreadEx(hthd,
                             (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                             QT_CONTINUE_DEBUG_EVENT,
                             ts_running);
            return;

        } else if (eventCode != EXIT_THREAD_DEBUG_EVENT
          && eventCode != EXIT_PROCESS_DEBUG_EVENT ) {

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;
        }
    }

    EnterCriticalSection(&csProcessDebugEvent);

    if (eventCode == CREATE_THREAD_DEBUG_EVENT){

        DPRINT(3, (_T("*** NEW TID = (PID,TID)(%08lx, %08lx)\n"),
                      de->dwProcessId, de->dwThreadId));

    } else {

        /*
         *  Find our structure for this event's process
         */

        hthd = HTHDXFromPIDTID((PID)de->dwProcessId,(TID)de->dwThreadId);

        /*
         *  Update our context structure for this thread if we found one
         *      in our list.  If we did not find a thread and this is
         *      not a create process debug event then return without
         *      processing the event as we are in big trouble.
         */

        if (hthd) {
            if (eventCode == EXCEPTION_DEBUG_EVENT) {
                hthd->ExceptionRecord = de->u.Exception.ExceptionRecord;
            }
            hthd->context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
            DbgGetThreadContext(hthd,&hthd->context);
            hthd->fContextDirty = FALSE;
            hthd->fIsCallDone   = FALSE;
            hthd->fAddrIsReal   = FALSE;
            hthd->fAddrIsFlat   = TRUE;
            hthd->fAddrOff32    = TRUE;
        } else
        if (hprc && (hprc->pstate & ps_killed)) {

            /*
             * this is an event for a thread that
             * we never created:
             */
            if (eventCode == EXIT_PROCESS_DEBUG_EVENT) {
                /* Process exited on a thread we didn't pick up */
                ProcessExitProcessEvent(de, NULL);
            } else {
                /* this is an exit thread for a thread we never picked up */
                AddQueue( QT_CONTINUE_DEBUG_EVENT,
                          de->dwProcessId,
                          de->dwThreadId,
                          DBG_CONTINUE,
                          0);
            }
            goto done;

        } else if (eventCode!=CREATE_PROCESS_DEBUG_EVENT) {

            assert(FALSE);

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      de->dwProcessId,
                      de->dwThreadId,
                      DBG_CONTINUE,
                      0);
            goto done;

        }
    }

    /*
     *  Mark the thread as having been stopped for some event.
     */

    if (hthd) {
        hthd->tstate &= ~ts_running;
        hthd->tstate |= ts_stopped;
    }

    /* If it is an exception event get the subclass */

    if (eventCode==EXCEPTION_DEBUG_EVENT){

        subClass = de->u.Exception.ExceptionRecord.ExceptionCode;
        DPRINT(1, (_T("Exception Event: subclass = %p    "), subClass));

        switch (subClass) {
        case STATUS_SEGMENT_NOTIFICATION:
            eventCode = de->dwDebugEventCode = SEGMENT_LOAD_DEBUG_EVENT;
            break;

        case EXCEPTION_SINGLE_STEP:
#if !defined(TARGET_i386)
            assert(_T("!EXCEPTION_SINGLE_STEP on non-x86!"));
#endif
            AddrFromHthdx(&addr, hthd);
            RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );

            //
            // This may be a single step or a hardware breakpoint.
            // If it is a single step, leave it at that.  If it is
            // a hardware breakpoint, convert it to a BREAKPOINT_DEBUG_EVENT.
            //

            DecodeSingleStepEvent( hthd, de, &eventCode, &subClass );
            break;

        case EXCEPTION_BREAKPOINT:

            /*
             * Check if it is a BREAKPOINT exception:
             * If it is, change the debug event to our pseudo-event,
             * BREAKPOINT_DEBUG_EVENT (this is a pseudo-event because
             * the API does not define such an event, and we are
             * synthesizing not only the class of event but the
             * subclass as well -- the subclass is set to the appropriate
             * breakpoint structure)
             */

            hthd->fDontStepOff = FALSE;

            AddrFromHthdx(&addr, hthd);

            /*
             *  Lookup the breakpoint in our (the dm) table
             */

            bp = FindBP(hthd->hprc, hthd, bptpExec, (BPNS)-1, &addr, FALSE);
            SetBPFlag(hthd, bp?bp:EMBEDDED_BP);

            /*
             *  Reassign the event code to our pseudo-event code
             */
            DPRINT(3, (_T("Reassigning event code!\n")));

            /*
             *  For some machines there is not single instruction tracing
             *  on the chip.  In this case we need to do it in software.
             *
             *  Check to see if the breakpoint we just hit was there for
             *  doing single step emulation.  If so then remap it to
             *  a single step exception.
             */

            if (bp) {
                if (bp->isStep){
                    de->u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_SINGLE_STEP;
                    subClass = EXCEPTION_SINGLE_STEP;
                    RemoveBP(bp);
                    RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );
                    break;
                } else {
                    RestoreKernelBreakpoints( hthd, GetAddrOff(addr) );
                }
            }

            //
            // Determine the start of the breakpoint instruction
            //

            //
            // DO NOT use DbgReadMemory here!  This is a non-cached read!
            //
            if (fCrashDump) {
                cb = DmpReadMemory(GetAddrOff(addr), &instr, BP_SIZE);
                if (cb != BP_SIZE) {
                    DPRINT(1, (_T("Memory read failed!!!\n")));
                instr = 0;
                }
            } else {
                if (DmKdReadVirtualMemoryNow(GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                    DPRINT(1, (_T("Memory read failed!!!\n")));
                    instr = 0;
                }
            }

#if defined(TARGET_ALPHA) || defined(TARGET_AXP64)

            switch (instr) {
                case 0:
                case CALLPAL_OP | CALLKD_FUNC:
                case CALLPAL_OP |    BPT_FUNC:
                case CALLPAL_OP |   KBPT_FUNC:
                     fInstrIsBp = TRUE;
                     break;
                default:
                    addr.addr.off -= BP_SIZE;

                    //
                    // NON-CACHED READ!!!
                    //
                    if (fCrashDump) {
                        cb = DmpReadMemory(GetAddrOff(addr),&instr,BP_SIZE);
                        if (cb != BP_SIZE) {
                            DPRINT(1, (_T("Memory read failed!!!\n")));
                            instr = 0;
                        }
                    } else {
                        if (DmKdReadVirtualMemoryNow(GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                            DPRINT(1, (_T("Memory read failed!!!\n")));
                            instr = 0;
                        }
                    }
                    switch (instr) {
                        case 0:
                        case CALLPAL_OP | CALLKD_FUNC:
                        case CALLPAL_OP |    BPT_FUNC:
                        case CALLPAL_OP |   KBPT_FUNC:
                             fInstrIsBp = TRUE;
                             hthd->fDontStepOff = TRUE;
                             break;
                        default:
                             fInstrIsBp = FALSE;
                    }
            }

#elif defined(TARGET_i386)

            /*
             *  It may have been a 0xcd 0x03 rather than a 0xcc
             *  (ie: check if it is a 1 or a 2 byte INT 3)
             */

            fInstrIsBp = FALSE;
            if (instr == BP_OPCODE || instr == 0) {
                fInstrIsBp = TRUE;
            } else
            if (instr == 0x3) { // 0xcd?
                --addr.addr.off;
                if (fCrashDump) {
                    cb = DmpReadMemory(GetAddrOff(addr),&instr,BP_SIZE);
                    if (cb != BP_SIZE) {
                        DPRINT(1, (_T("Memory read failed!!!\n")));
                        instr = 0;
                    }
                } else {
                    if (DmKdReadVirtualMemoryNow(GetAddrOff(addr),&instr,BP_SIZE,&cb) || cb != BP_SIZE) {
                        DPRINT(1, (_T("Memory read failed!!!\n")));
                        instr = 0;
                    }
                }
                if (cb == 1 && instr == 0xcd) {
                    --addr.addr.off;
                    fInstrIsBp = TRUE;
                }
            } else {
                hthd->fDontStepOff = TRUE;
            }

#elif defined(TARGET_IA64)

{
    BP_UNIT ia64_opcode;


    // EM instruction slots are not byte-aligned. Break instruction
    // (BP_OPCODE) must be shifted according to the instruction slot
    // before comparison
    switch (GetAddrOff(addr) & 0xf) {
        case 0:
            ia64_opcode = (instr & (INST_SLOT0_MASK)) >> 5;
            break;

        case 4:
            ia64_opcode = (instr & (INST_SLOT1_MASK)) >> 14;
            break;

        case 8:
            ia64_opcode = (instr & (INST_SLOT2_MASK)) >> 23;
            break;

        default:
            assert(!"Bad BP Address");
            ia64_opcode = 0;
        break;
    }

        fInstrIsBp = (ia64_opcode == BP_OPCODE) || (instr == 0);
}

#else

#error( "undefined processor type" );

#endif

            if (!bp && !fInstrIsBp) {
                DMPrintShellMsg( _T("Stopped at an unexpected exception: code=%08x addr=%08I64x\n"),
                                 de->u.Exception.ExceptionRecord.ExceptionCode,
                                 de->u.Exception.ExceptionRecord.ExceptionAddress
                               );
            }

            /*
             * Reassign the subclass to point to the correct
             * breakpoint structure and clear the exception code
             *
             */

            de->dwDebugEventCode = eventCode = BREAKPOINT_DEBUG_EVENT;
            de->u.Exception.ExceptionRecord.ExceptionAddress = addr.addr.off;
            de->u.Exception.ExceptionRecord.ExceptionCode = (DWORD)(DWORD_PTR)bp;
            de->u.Exception.ExceptionRecord.NumberParameters = (DWORD)((DWORD_PTR)bp>>32);
            subClass = (UINT_PTR)bp;

            break;
        }
    }

    /*
     *  Check if this debug event was expected
     */

    ee = PeeIsEventExpected(hthd, eventCode, subClass, TRUE);


    /*
     * If it wasn't, clear all consumable events
     * and then run the standard handler with
     * notifications going to the execution model
     */

    assert((0 < eventCode) && (eventCode < MAX_EVENT_CODE));

    if (!ee) {

        if ((hthd != NULL) && (hthd->tstate & ts_funceval)) {
            assert(RgfnFuncEventDispatch[eventCode-EXCEPTION_DEBUG_EVENT]);
            RgfnFuncEventDispatch[eventCode-EXCEPTION_DEBUG_EVENT](de, hthd);
        } else {
            assert(DebugDispatchTable[eventCode-EXCEPTION_DEBUG_EVENT]);
            DebugDispatchTable[eventCode-EXCEPTION_DEBUG_EVENT](de,hthd);
        }

    } else {

        /*
         *  If it was expected then call the action
         * function if one was specified
         */

        if (ee->action) {
            (ee->action)(de, hthd, 0, ee->lparam);
        }

        /*
         *  And call the notifier if one was specified
         */

        if (ee->notifier) {
            METHOD  *nm = ee->notifier;
            (nm->notifyFunction)(de, hthd, 0, nm->lparam);
        }

        MHFree(ee);
    }

done:
    LeaveCriticalSection(&csProcessDebugEvent);
    return;
}                               /* ProcessDebugEvent() */



#endif // KERNEL





/**  ConsumeThreadEventsAndNotifyEM
**
**   Description:
**       Does exactly what it says it does
*/

void
ConsumeThreadEventsAndNotifyEM(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG wParam,
    DWORDLONG lparam
    )
{
    ConsumeAllThreadEvents(hthd, FALSE);
    NotifyEM(de, hthd, wParam, lparam);
}



/***    NotifyEM
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Given a debug event from the OS send the correct information
**      back to the debugger.
**
*/


void
NotifyEM(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG wparam,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This is the interface for telling the EM about debug events.

    In general, de describes an event which the debugger needs
    to know about.  In some cases, a reply is needed.  In those
    cases this routine handles the reply and does the appropriate
    thing with the data from the reply.

Arguments:

    de      - Supplies debug event structure

    hthd    - Supplies thread that got the event

    wparam  - Supplies data specific to event

    lparam  - Supplies data specific to event

Return Value:

    None

--*/
{
    DWORD       eventCode = de->dwDebugEventCode;
    DWORD       subClass;
    RTP         rtp;
    RTP *       lprtp;
    WORD        cbPacket=0;
    LPBYTE      lpbPacket;
    LPVOID      toFree=(LPVOID)0;
    WORD        packetType = tlfDebugPacket;


    if (hthd) {
        rtp.hpid = hthd->hprc->hpid;
        rtp.htid = hthd->htid;
    } else if (hpidRoot == (HPID)INVALID) {
        return;
    } else {
        // cheat:
        rtp.hpid = hpidRoot;
        rtp.htid = NULL;
    }
    subClass = de->u.Exception.ExceptionRecord.ExceptionCode;

    switch(eventCode){

    case FUNC_EXIT_EVENT:
        if (hthd->fDisplayReturnValues) {
            cbPacket = sizeof(ADDR);
            rtp.dbc = dbcExitedFunction;
            lpbPacket = (LPBYTE) lparam;
            assert(lpbPacket);
        } else {
            return;
        }
        break;

    case EXCEPTION_DEBUG_EVENT:
        if (subClass!=EXCEPTION_SINGLE_STEP){
            PEXCEPTION_RECORD64 pexr=&de->u.Exception.ExceptionRecord;
            DWORD cParam = pexr->NumberParameters;
            DWORD nBytes = sizeof(EPR)+sizeof(ULONG64)*cParam;
            LPEPR lpepr  = MHAlloc(nBytes);

            toFree    = (LPVOID) lpepr;
            cbPacket  = (WORD)   nBytes;
            lpbPacket = (LPBYTE) lpepr;
#ifdef TARGET_i386
            lpepr->bpr.segCS = (SEGMENT)hthd->context.SegCs;
            lpepr->bpr.segSS = (SEGMENT)hthd->context.SegSs;
#endif
            lpepr->bpr.offEBP =  FRAME_POINTER(hthd);
            lpepr->bpr.offESP =  STACK_POINTER(hthd);
            lpepr->bpr.offEIP =  PC(hthd);
            lpepr->bpr.fFlat  =  hthd->fAddrIsFlat;
            lpepr->bpr.fOff32 =  hthd->fAddrOff32;
            lpepr->bpr.fReal  =  hthd->fAddrIsReal;

            lpepr->efd              = (EXCEPTION_FILTER_DEFAULT)lparam;
            lpepr->dwFirstChance    = de->u.Exception.dwFirstChance;
            lpepr->ExceptionCode    = pexr->ExceptionCode;
            lpepr->ExceptionFlags   = pexr->ExceptionFlags;
            lpepr->NumberParameters = cParam;
            for(;cParam;cParam--) {
                lpepr->ExceptionInformation[cParam-1]=
                  pexr->ExceptionInformation[cParam-1];
            }

            rtp.dbc = dbcException;
            break;
        };

        // Add 'foo returned' to Auto watch window
        if (hthd->fReturning) {
            hthd->fReturning = FALSE;
            NotifyEM(&FuncExitEvent, hthd, 0, (ULONG64)&hthd->addrFrom );
        }



        // Fall through when subClass == EXCEPTION_SINGLE_STEP

    case BREAKPOINT_DEBUG_EVENT:
    case ENTRYPOINT_DEBUG_EVENT:
    case CHECK_BREAKPOINT_DEBUG_EVENT:
    case LOAD_COMPLETE_DEBUG_EVENT:
        {
            LPBPR lpbpr = MHAlloc ( sizeof ( BPR ) );
#ifdef TARGET_i386
            UOFFSET bpAddr = PC(hthd)-1;
#else
            UOFFSET bpAddr = PC(hthd);
#endif
            toFree=lpbpr;
            cbPacket = sizeof ( BPR );
            lpbPacket = (LPBYTE) lpbpr;

#ifdef TARGET_i386
            lpbpr->segCS = (SEGMENT)hthd->context.SegCs;
            lpbpr->segSS = (SEGMENT)hthd->context.SegSs;
#endif

            lpbpr->offEBP =  FRAME_POINTER(hthd);
            lpbpr->offESP =  STACK_POINTER(hthd);
            lpbpr->offEIP =  PC(hthd);
            lpbpr->fFlat  =  hthd->fAddrIsFlat;
            lpbpr->fOff32 =  hthd->fAddrOff32;
            lpbpr->fReal  =  hthd->fAddrIsReal;
            lpbpr->qwNotify = lparam;

            if (eventCode==EXCEPTION_DEBUG_EVENT) {
                rtp.dbc = dbcStep;
                break;
            } else {              /* (the breakpoint case) */

                // REVIEW: It would be cleaner to setup something which got
                // called when the EE got consumed and turned this off. I
                // don't think the DM has this notion except for preset bps.
                // I think a general function for cleanup would be a good
                // idea here.

                if (eventCode != CHECK_BREAKPOINT_DEBUG_EVENT) {
                    hthd->fReturning = FALSE;
                }

                if (eventCode == ENTRYPOINT_DEBUG_EVENT) {
                    rtp.dbc = dbcEntryPoint;
                } else if (eventCode == LOAD_COMPLETE_DEBUG_EVENT) {
                    rtp.dbc = dbcLoadComplete;
                } else if (eventCode == CHECK_BREAKPOINT_DEBUG_EVENT) {
                    rtp.dbc = dbcCheckBpt;
                    packetType = tlfRequest;
                } else {
                    rtp.dbc = dbcBpt;
                }

                /* NOTE: Ok try to follow this: If this was one
                 *   of our breakpoints then we have already
                 *   decremented the IP to point to the actual
                 *   INT3 instruction (0xCC), this is so we
                 *   can just replace the byte and continue
                 *   execution from that point on.
                 *
                 *   But if it was hard coded in by the user
                 *   then we can't do anything but execute the
                 *   NEXT instruction (because there is no
                 *   instruction "under" this INT3 instruction)
                 *   So the IP is left pointing at the NEXT
                 *   instruction. But we don't want the EM
                 *   think that we stopped at the instruction
                 *   after the INT3, so we need to decrement
                 *   offEIP so it's pointing at the hard-coded
                 *   INT3. Got it?
                 *
                 *   On an ENTRYPOINT_DEBUG_EVENT, the address is
                 *   already right, and lparam is ENTRY_BP.
                 */

                if (!lparam) {
                    lpbpr->offEIP = (UOFFSET)de->
                       u.Exception.ExceptionRecord.ExceptionAddress;
                }
            }

        }
        break;


    case CREATE_PROCESS_DEBUG_EVENT:
        /*
         *  A Create Process event has occured.  The following
         *  messages need to be sent back
         *
         *  dbceAssignPID: Associate our handle with the debugger
         *
         *  dbcModLoad: Inform the debugger of the module load for
         *  the main exe (this is done at the end of this routine)
         */
        {
            HPRCX  hprc = (HPRCX)lparam;

            /*
             * Has the debugger requested this process?
             * ie: has it already given us the HPID for this process?
             */
            if (hprc->hpid != (HPID)INVALID){

                lpbPacket = (LPBYTE)&(hprc->pid);
                cbPacket  = sizeof(hprc->pid);

                /* Want the hprc for the child NOT the DM */
                rtp.hpid  = hprc->hpid;

#ifndef KERNEL
                //
                // hack for made-up image names:
                //
                {
                    extern int MagicModuleId;
                    MagicModuleId = 0;
                }
#endif
                rtp.dbc   = dbceAssignPID;
            }

            /*
             * The debugger doesn't know about this process yet,
             * request an HPID for this new process.
             */

            else {
                LPNPP lpnpp = MHAlloc(cbPacket=sizeof(NPP));

                toFree            = lpnpp;
                lpbPacket         = (LPBYTE)lpnpp;
                packetType        = tlfRequest;

                /*
                 * We must temporarily assign a valid HPID to this HPRC
                 * because OSDebug will try to de-reference it in the
                 * TL callback function
                 */
                rtp.hpid          = hpidRoot;
                lpnpp->pid        = hprc->pid;
                lpnpp->fReallyNew = TRUE;
                rtp.dbc           = dbcNewProc;
            }
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        {
            LPTCR lptcr = (LPTCR) MHAlloc(cbPacket=sizeof(TCR));
            toFree      = lpbPacket = (LPBYTE)lptcr;
            packetType  = tlfRequest;

            lptcr->tid     = hthd->tid;
            lptcr->uoffTEB = (UOFFSET) hthd->offTeb;

            rtp.hpid = hthd->hprc->hpid;
            rtp.htid = hthd->htid;
            rtp.dbc  = dbcCreateThread;
        }
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        cbPacket    = sizeof(DWORD);
        lpbPacket   = (LPBYTE) &(de->u.ExitProcess.dwExitCode);

        hthd->hprc->pstate |= ps_exited;
        rtp.hpid    = hthd->hprc->hpid;
        rtp.htid    = hthd->htid;
        rtp.dbc = dbcProcTerm;
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        cbPacket    = sizeof(DWORD);
        lpbPacket   = (LPBYTE) &(de->u.ExitThread.dwExitCode);

        hthd->tstate        |= ts_dead; /* Mark thread as dead */
        hthd->hprc->pstate  |= ps_deadThread;
        rtp.dbc = dbcThreadTerm;
        break;

    case DESTROY_PROCESS_DEBUG_EVENT:
        DPRINT(3, (_T("DESTROY PROCESS\n")));
        hthd->hprc->pstate |= ps_destroyed;
        rtp.dbc = dbcDeleteProc;
        break;

    case DESTROY_THREAD_DEBUG_EVENT:
        /*
         *  Check if already destroyed
         */

        assert( (hthd->tstate & ts_destroyed) == 0 );

        DPRINT(3, (_T("DESTROY THREAD\n")));

        hthd->tstate |= ts_destroyed;
        cbPacket    = sizeof(DWORD);
        // kentf exit code is bogus here
        lpbPacket   = (LPBYTE) &(de->u.ExitThread.dwExitCode);
        rtp.dbc     = dbcDeleteThread;
        break;


    case LOAD_DLL_DEBUG_EVENT:

        rtp.dbc     = dbcModLoad;

        lpbPacket   = (PVOID)lparam;
        cbPacket    = (USHORT)wparam;

        //ValidateHeap();
        toFree      = (LPVOID)lpbPacket;

#ifdef KERNEL
        //
        // if we just loaded the kernel, get magic values
        //
        // If we are loading the kernel, this need to be synchronous so 
        // that we can load the magic values.
        //
        // But only for crash dumps.
        // (we should be able to dispose of this by using KDDEBUGGER_DATA,
        // but only for NT 5)
        // 
        if (fCrashDump && 
            _tcsicmp( (PTSTR) de->u.LoadDll.lpImageName, KERNEL_IMAGE_NAME ) == 0) {

            packetType = tlfRequest;
        }
#endif
        break;

    case UNLOAD_DLL_DEBUG_EVENT:

        assert( Is64PtrSE(de->u.UnloadDll.lpBaseOfDll) );

        cbPacket  = sizeof(ULONG64);
        lpbPacket = (LPBYTE) &(de->u.UnloadDll.lpBaseOfDll);

        rtp.dbc   = dbceModFree32;
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        {
            LPINFOAVAIL   lpinf;
            DWORD   cbR;

            rtp.dbc = dbcInfoAvail;

            cbPacket = (WORD)(sizeof(INFOAVAIL) +
                               de->u.DebugString.nDebugStringLength + 1);
            lpinf = (LPINFOAVAIL) lpbPacket = MHAlloc(cbPacket);
            toFree = lpbPacket;

            lpinf->fReply   = FALSE;
            lpinf->fUniCode = de->u.DebugString.fUnicode;

            DbgReadMemory(hthd->hprc,
                          de->u.DebugString.lpDebugStringData,
                          &lpinf->buffer[0],
                          de->u.DebugString.nDebugStringLength,
                          &cbR);
            lpinf->buffer[cbR] = 0;
        }
        break;


    case INPUT_DEBUG_STRING_EVENT:
        {
            //
            // BUGBUG - kcarlos - This code is not UNICODE safe. Should it be?
            //
            LPINFOAVAIL   lpinf;
            DWORD   cbR;

            packetType = tlfRequest;
            rtp.dbc = dbcInfoReq;

            cbPacket =
                (WORD)(sizeof(INFOAVAIL) + de->u.DebugString.nDebugStringLength + 1);
            lpinf = (LPINFOAVAIL) lpbPacket = MHAlloc(cbPacket);
            toFree = lpbPacket;

            lpinf->fReply   = TRUE;
            lpinf->fUniCode = de->u.DebugString.fUnicode;
            memcpy(&lpinf->buffer[0], 
                    (PVOID)de->u.DebugString.lpDebugStringData,
                    de->u.DebugString.nDebugStringLength
                    );

            lpinf->buffer[ de->u.DebugString.nDebugStringLength ] = 0;

#if 0
//
// Protocol changed and someone forgot to update this???
//
            DbgReadMemory(hthd->hprc,
                          de->u.DebugString.lpDebugStringData,
                          &lpinf->buffer[0],
                          de->u.DebugString.nDebugStringLength,
                          &cbR);
            lpinf->buffer[ cbR ] = 0;
#endif
        }
        break;

    case ATTACH_DEADLOCK_DEBUG_EVENT:
        {
            static XOSD xosd;
            xosd        = xosdAttachDeadlock;
            cbPacket    = sizeof(xosd);
            lpbPacket   = (LPBYTE) &xosd;
            rtp.dbc     = dbcError;
        }
        break;

    case ATTACH_EXITED_DEBUG_EVENT:
        {
            static XOSD xosd;
            xosd        = xosdAttachDeadlock;//*****
            cbPacket    = sizeof(xosd);
            lpbPacket   = (LPBYTE) &xosd;
            rtp.dbc     = dbcError;
        }
        break;

    case MESSAGE_DEBUG_EVENT:
    case MESSAGE_SEND_DEBUG_EVENT:
        cbPacket = sizeof ( MSGI );
        lpbPacket = (LPBYTE) lparam;

        rtp.dbc = (eventCode == MESSAGE_DEBUG_EVENT) ? dbcMsgBpt :dbcSendMsgBpt;
        break;

    default:
        DPRINT(1, (_T("Error, unknown event\n\r")));
        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        return;
    }


    DPRINT(3, (_T("Notify the debugger: dbc=%x, hpid=%x, htid=%x, cbpacket=%d "),
                  rtp.dbc, rtp.hpid, rtp.htid, cbPacket+FIELD_OFFSET(RTP, rgbVar)));

    if (!(rtp.cb=cbPacket)) {
        DmTlFunc(packetType, rtp.hpid, FIELD_OFFSET(RTP, rgbVar), (LPARAM) &rtp);
    }
    else {
        lprtp = (LPRTP)MHAlloc(FIELD_OFFSET(RTP, rgbVar)+cbPacket);
        _fmemcpy(lprtp, &rtp, FIELD_OFFSET(RTP, rgbVar));
        _fmemcpy(lprtp->rgbVar, lpbPacket, cbPacket);

        DmTlFunc(packetType, rtp.hpid, (FIELD_OFFSET(RTP, rgbVar)+cbPacket), (LPARAM) lprtp);

        MHFree(lprtp);
    }

    if (toFree) {
        MHFree(toFree);
    }

    DPRINT(3, (_T("\n")));

    switch(eventCode){

      case CREATE_THREAD_DEBUG_EVENT:
        if (packetType == tlfRequest) {
            hthd->htid = *((HTID *) abEMReplyBuf);
        }
        SetEvent(hthd->hprc->hEventCreateThread);
        break;

      case CREATE_PROCESS_DEBUG_EVENT:
        if (packetType == tlfRequest) {
            ((HPRCX)lparam)->hpid = *((HPID *) abEMReplyBuf);
        } else {
            XOSD xosd = xosdNone;
            DmTlFunc( tlfReply,
                      ((HPRCX)lparam)->hpid,
                      sizeof(XOSD),
                      (LPARAM) &xosd);
        }

        SetEvent(hEventCreateProcess);

        break;

      case OUTPUT_DEBUG_STRING_EVENT:
        // just here to synchronize
        break;

      case INPUT_DEBUG_STRING_EVENT:
        {
            de->u.DebugString.nDebugStringLength = _tcslen(abEMReplyBuf) + 1;
            de->u.DebugString.lpDebugStringData = (DWORD64) MHAlloc(de->u.DebugString.nDebugStringLength);
            _tcscpy((PVOID) de->u.DebugString.lpDebugStringData, abEMReplyBuf);
//
// Maybe this was intended to be a user mode reply????
// Well it sure doesn't work for kernel!
//
#if 0
            DWORD cbR;

            de->u.DebugString.nDebugStringLength = _ftcslen(abEMReplyBuf) + 1;
            DbgWriteMemory(hthd->hprc,
                           de->u.DebugString.lpDebugStringData,
                           abEMReplyBuf,
                           de->u.DebugString.nDebugStringLength,
                           &cbR);
#endif
        }
        break;


      default:
        break;
    }

    //ValidateHeap();
    return;
}                               /* NotifyEM() */

/*** ACTIONTERMINATEPROCESS
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID
ActionTerminateProcess (
    DEBUG_EVENT64  *pde,
    HTHDX           hthd,
    DWORD           unused,
    ULONG64         lparam
    )
{
    ConsumeAllProcessEvents ( (HPRCX)lparam, TRUE );

    hthd->fExceptionHandled = TRUE;
    ContinueThread(hthd);

    hthd->hprc->pstate |= ps_dead;

} /* ACTIONTERMINATEPROCESS */

/*** ACTIONPROCESSLOADFAILED
 *
 * PURPOSE:
 *      When we issued the CreateProcess, that succeeded; but then we
 *      got an exception with the EXCEPTION_NONCONTINUABLE flag set,
 *      which means after letting the debuggee continue we would get
 *      a process-termination notification.  (For example, if the
 *      system can't find a DLL that the process needs, it sends a
 *      STATUS_DLL_NOT_FOUND notification with this flag set.)  So
 *      we've said ok, and now the system has sent us a procterm
 *      notification for it.
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID
ActionProcessLoadFailed (
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    ULONG64 lparam
    )
{
    /* Call another action proc & do what it does */
    ActionTerminateProcess(pde, hthd, 0, (ULONG64)hthd->hprc);
}

/*** ACTIONNONCONTINUABLEEXCEPTION
 *
 * PURPOSE:
 *      When we issued the CreateProcess, that succeeded; but then we
 *      got an exception with the EXCEPTION_NONCONTINUABLE flag set,
 *      which means after letting the debuggee continue we would get
 *      a process-termination notification.  (For example, if the
 *      system can't find a DLL that the process needs, it sends a
 *      STATUS_DLL_NOT_FOUND notification with this flag set.)  So
 *      we've said ok, and now the system has sent us a last-chance
 *      notification for it.
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID
ActionNoncontinuableException (
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    ULONG64 lparam
    )
{
    //
    // [cuda#5673 7/8/93 mikemo]  This is very strange -- it seems that
    // in some situations NT has now sent us a SECOND first-chance
    // notification, even though it already sent us one.  I don't know
    // why this is, but if this happens, I register another event expecting
    // a last-chance notification, instead of expecting exit-process.
    //
    if (pde->u.Exception.dwFirstChance) {
        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              EXCEPTION_DEBUG_EVENT,
                              (DWORD_PTR)pde->u.Exception.ExceptionRecord.ExceptionCode,
                              DONT_NOTIFY,
                              ActionNoncontinuableException,
                              FALSE,
                              0
                              );
    } else {
        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              EXIT_PROCESS_DEBUG_EVENT,
                              NO_SUBCLASS,
                              DONT_NOTIFY,
                              ActionProcessLoadFailed,
                              FALSE,
                              0);
    }

    ContinueThreadEx(hthd,
                     (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                     QT_CONTINUE_DEBUG_EVENT,
                     ts_running);
}


void
ProcessExceptionEvent(
    DEBUG_EVENT64* de,
    HTHDX hthd
    )
{
    DWORD_PTR   subclass = de->u.Exception.ExceptionRecord.ExceptionCode;
    DWORD       firstChance = de->u.Exception.dwFirstChance;
    PBREAKPOINT bp=NULL;
    EXCEPTION_FILTER_DEFAULT efd;

    //
    //  If the thread is in a pre-start state we failed in the
    //  program loader, probably because a link couldn't be resolved.
    //
    if ( hthd->hprc->pstate & ps_preStart ) {

        DPRINT(1, (_T("Exception during init\n")));

        //
        // since we will probably never see the expected BP,
        // clear it out, and clear the prestart flag on the
        // thread. If the exception is continuable, go ahead
        // and deliver the exception to the shell as normal.
        //

        hthd->hprc->pstate &= ~ps_preStart;
        hthd->tstate |= ts_stopped;
        ConsumeAllProcessEvents(hthd->hprc, TRUE);

#if 0
        if (!CrashDump &&
            de->u.Exception.ExceptionRecord.ExceptionFlags & EXCEPTION_NONCONTINUABLE) {

            //
            // BUGBUG This case does not send an exception record to the shell; instead
            // BUGBUG it aborts the process and sends an error message.  This is probably
            // BUGBUG undesirable behaviour in a real debugger.
            //

            DWORD dwRet;
            LPTSTR lszError = NULL;

            //
            // The debugger is expecting either an OS-specific error code
            // or an XOSD.  We don't have an OS
            // error code (all we have is an exception number).  The debugger
            // treats STATUS_DLL_NOT_FOUND as an error code, but doesn't
            // recognize any other exception codes, so for other exception
            // codes, just return xosdLoadChild.
            //
            if (subclass == STATUS_DLL_NOT_FOUND) {
                TCHAR rgch[256]; // same as magic number in em\errors.c
                if (!LoadString(hInstance, IDS_STATUS_DLL_NOT_FOUND, rgch, _tsizeof(rgch))) {
                    assert(FALSE);
                } else {
                    lszError = rgch;
                }
                dwRet = xosdFileNotFound;
            } else {
                dwRet = xosdGeneral;
            }
            SendNTError(hthd->hprc, dwRet, lszError);

            //
            // Actually the CreateProcess succeeded, and then we
            // received a notification with an exception; so we need
            // to keep calling ContinueDebugEvent until the process
            // is really dead.
            //

            RegisterExpectedEvent(hthd->hprc,
                                  hthd,
                                  EXCEPTION_DEBUG_EVENT,
                                  subclass,
                                  DONT_NOTIFY,
                                  ActionNoncontinuableException,
                                  FALSE,
                                  0);

            hthd->hprc->pstate |= ps_exited;
            hthd->tstate &= ~ts_stopped;

            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      hthd->hprc->pid,
                      hthd->tid,
                      (DWORD)DBG_EXCEPTION_NOT_HANDLED,
                      0);
            return;
        }
#endif // 0
    }


    switch (subclass) {

    case EXCEPTION_SINGLE_STEP:
#if defined(DOLPHIN)
#if defined(NO_TRACE_BIT)
        //
        // BUGBUG kentf This next bit of code is a rather overcomplicated
        // BUGBUG   way of creating a deadlock.  The same task could be
        // BUGBUG   accomplished with considerably less code.
        // BUGBUG
        // BUGBUG The problem which this intends to solve is as follows:
        // BUGBUG   When a thread is stepped using a code breakpoint, it
        // BUGBUG   may happen that another thread will execute the
        // BUGBUG   breakpoint.  Since that thread is not intended to stop
        // BUGBUG   there, we must replace the instruction and allow that
        // BUGBUG   thread to continue.  During the time that the "wrong"
        // BUGBUG   thread is executing the instruction, it may happen that
        // BUGBUG   the "right" thread will hit that instruction, and run
        // BUGBUG   through it, thereby missing the breakpoint.
        // BUGBUG
        // BUGBUG   The following code attempts to solve the problem by
        // BUGBUG   suspending all of the threads in the process while
        // BUGBUG   stepping the "wrong" thread off of the breakpoint.
        // BUGBUG
        // BUGBUG   This algorithm assumes that the "wrong" thread will be
        // BUGBUG   able to complete the instruction while all of the other
        // BUGBUG   threads are suspended.  This assumption is easily
        // BUGBUG   disproven.  For example, try this on a call to
        // BUGBUG   WaitForSingleObject.
        // BUGBUG
        // BUGBUG   Secondly, this code assumes that the next debug event
        // BUGBUG   will be the exception from the "wrong" thread completing
        // BUGBUG   the new step.  This is a absolutely not the case, and
        // BUGBUG   the code below will fail in ways much more disastrous
        // BUGBUG   than running through a step.
        // BUGBUG
        // BUGBUG
        // BUGBUG   Note 1:  This particular condition cannot arise on
        // BUGBUG   a processor with a step flag, since the step flag
        // BUGBUG   cannot cause a single step exception on the
        // BUGBUG   wrong thread (except as a result of an OS bug [ref:win9x]).
        //
        {
            /* This single step was intended for another thread */
            HPRCX hprc = hthd->hprc;
            HTHDX hthdCurr = NULL;
            BREAKPOINT* tmpBP = NULL;
            UOFF64 NextOffset;
            ADDR addr;
            BP_UNIT opcode = BP_OPCODE;
            DWORD i;

            AddrFromHthdx(&addr, hthd);
            if (bp = AtBP(hthd)) {
                RestoreInstrBP(hthd, bp);
            }
            /* Suspend all threads except this one */
            for (hthdCurr = hprc->hthdChild; hthdCurr != NULL; hthdCurr = hthdCurr->nextSibling) {
                if (hthdCurr != hthd) {
                    if (SuspendThread(hthdCurr->rwHand) == -1L) {
                        ; // Internal error;
                    }
                }
            }
            /* Set BP at next offset so we can get off this SS */
            NextOffset = GetNextOffset( hthd, FALSE);
            if (NextOffset != 0) {
                ADDR tmpAddr;
                AddrInit(&tmpAddr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE);
                tmpBP = SetBP( hprc, hthd, bptpExec, bpnsStop, &tmpAddr, (HPID) INVALID );
                assert(tmpBP);
            } else {
                assert(FALSE);
            }
            /* Get off SS */
            VERIFY(ContinueDebugEvent(hprc->pid, hthd->tid, DBG_CONTINUE));
            if (DMWaitForDebugEvent(de, INFINITE)) {
                //
                // this is certain to fire, and for the wrong process
                //
                assert(de->dwDebugEventCode == EXCEPTION_DEBUG_EVENT);
            } else {
                assert(FALSE);
            }
            hthd->context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
            VERIFY(DbgGetThreadContext(hthd, &hthd->context));
            hthd->fContextDirty = FALSE;
            hthd->fIsCallDone = FALSE;
            RemoveBP(tmpBP);
            /* Resume all threads except this one */
            for (hthdCurr = hprc->hthdChild; hthdCurr != NULL; hthdCurr = hthdCurr->nextSibling) {
                if (hthdCurr != hthd) {
                    if (ResumeThread(hthdCurr->rwHand) == -1L) {
                        assert(FALSE); // Internal error;
                    }
                }
            }

            if (bp == NULL) {
                bp = SetBP(hprc, hthd, bptpExec, bpnsStop, &addr, FALSE);
            } else {
            /* Put back SS */
                AddrWriteMemory(hprc, hthd, &bp->addr, (LPBYTE)&opcode, BP_SIZE, &i);
                bp->instances++;
            }
            bp->isStep++;
            ContinueThread(hthd);
        }
#endif // NO_TRACE_BIT
#endif // DOLPHIN
        break;


    default:


        /*
         *  The user can define a set of exceptions for which we do not
         *      notify the shell on a first chance.
         */

        if (!firstChance) {

            DPRINT(3, (_T("2nd Chance Exception %p.\n"),subclass));
            hthd->tstate |= ts_second;
            efd = efdStop;

        } else {

            hthd->tstate |= ts_first;

            efd = ExceptionAction(hthd->hprc, (ULONG)subclass);

            switch (efd) {

              case efdNotify:

                DPRINT(3, (_T("Notifying EM of Exception %p.\n"),subclass));
                NotifyEM(de, hthd, 0, efd);
                // fall through to ignore case

#if defined (TARGET_i386) && !defined (KERNEL)

                //
                // If we are stepping (=> trace flag set) and we are attempting
                // to step on a statement that raising an exception that gets
                // caught somewhere else, then having the trace flag set
                // prevents us from ending up where the NLG thing takes us.
                // Turn it off & assume that we will get a second chance.
                // This is wrong for disasm stepping and may be otherwise
                // problematic.
                //

                hthd->context.EFlags &= ~TF_BIT_MASK;
                hthd->fContextDirty = TRUE;
#endif
#if defined (TARGET_IA64) && !defined (KERNEL)

                //
                // If we are stepping (=> trace flag set) and we are attempting
                // to step on a statement that raising an exception that gets
                // caught somewhere else, then having the trace flag set
                // prevents us from ending up where the NLG thing takes us.
                // Turn it off & assume that we will get a second chance.
                // This is wrong for disasm stepping and may be otherwise
                // problematic.
                //

                hthd->context.StIPSR &= ~TF_BIT_MASK;
                hthd->fContextDirty = TRUE;
#endif

              case efdIgnore:

                DPRINT(3, (_T("Ignoring Exception %p.\n"),subclass));
                ContinueThreadEx(hthd,
                                 DBG_EXCEPTION_NOT_HANDLED,
                                 QT_CONTINUE_DEBUG_EVENT,
                                 ts_running);
                return;

              case efdStop:
              case efdCommand:
                break;
            }
        }
        // Cleanup ...

        hthd->fReturning = FALSE;
        ConsumeAllThreadEvents(hthd, FALSE);

        //ExprBPStop(hthd->hprc, hthd);
        NotifyEM(de, hthd, 0, efd);

        break;
    }

    return;
}                                   /* ProcessExceptionEvent() */



void
ProcessRipEvent(
    DEBUG_EVENT64   * de,
    HTHDX           hthd
    )
{
    if (hthd) {
        hthd->tstate |= ts_rip;
    }
    NotifyEM( de, hthd, 0, 0 );
}                               /* ProcessRipEvent() */


void
ProcessBreakpointEvent(
    DEBUG_EVENT64   * pde,
    HTHDX           hthd
    )
{
    PBREAKPOINT pbp = (PBREAKPOINT) ((((DWORD_PTR)pde->u.Exception.ExceptionRecord.NumberParameters) << 32) | (DWORD)pde->u.Exception.ExceptionRecord.ExceptionCode);

#if 0 // This code doesn't do pointer extension properly
    PBREAKPOINT pbp;
    DWORD_PTR ibp = pde->u.Exception.ExceptionRecord.ExceptionCode;
    ibp |= pde->u.Exception.ExceptionRecord.NumberParameters << 32;
    pbp = (PBREAKPOINT)ibp;
#endif

    pde->u.Exception.ExceptionRecord.ExceptionCode = 0;
    pde->u.Exception.ExceptionRecord.NumberParameters = 0;

    DPRINT(1, (_T("Hit a breakpoint -- ")));

    if (!pbp) {

        DPRINT(1, (_T("[Embedded BP]\n")));
        // already set this in ProcessDebugEvent
        //SetBPFlag(hthd, EMBEDDED_BP);
        NotifyEM(pde, hthd, 0, (ULONG64)pbp);


    } else if (!pbp->hthd || pbp->hthd == hthd) {

        DPRINT(1, (_T("[One of our own BP's.]\n")));

        if (pbp->bpType == bptpMessage) {
#ifdef KERNEL
            assert(!"Message BP not allowed in Kernel mode");
            NotifyEM(pde, hthd, 0, (ULONG64)pbp);
#else
            MSGI    msgi;

            if (!GetWndProcMessage (hthd, &msgi.dwMessage)) {
                assert (FALSE);
            }

            msgi.dwMask = 0;        // mask is currently unused in the dm
            msgi.addr = pbp->addr;

            pde->dwDebugEventCode = MESSAGE_DEBUG_EVENT;

            SetBPFlag (hthd, pbp);

            ConsumeAllThreadEvents (hthd, FALSE);
            NotifyEM (pde, hthd, 0, (ULONG64)&msgi);
#endif // KERNEL

        } else if (CheckDataBP (hthd, pbp)) {
            SetBPFlag(hthd, pbp);

            if ((pbp->bpNotify == bpnsStop) ||
                (pbp->bpNotify == bpnsCheck && CheckBpt(hthd, pbp))) {

                ConsumeAllThreadEvents(hthd, FALSE);
                NotifyEM(pde, hthd, 0, (ULONG64)pbp);

            } else {

                if (pbp->bpNotify == bpnsContinue) {
                    // NotifyEM(...);
                }
                ContinueFromBP(hthd, pbp);
            }
        } else {
            ContinueFromBP(hthd, pbp);
        }

    } else {

        DPRINT(1, (_T("[BP for another thread]\n")));

        //
        // data bp should never hit on the wrong thread
        //
        assert(pbp->hWalk == NULL);

        ContinueFromBP(hthd, pbp);

    }
}

VOID
ContinueFromBP(
    HTHDX hthd,
    PBREAKPOINT pbp
    )
{
    METHOD *ContinueSSMethod;
    if (!AtBP(hthd)) {
        ExprBPContinue( hthd->hprc, hthd );
        ContinueThread(hthd);
    } else {
        ContinueSSMethod = (METHOD*)MHAlloc(sizeof(METHOD));
        ContinueSSMethod->notifyFunction = MethodContinueSS;
        ContinueSSMethod->lparam         = (ULONG64)ContinueSSMethod;
        ContinueSSMethod->lparam2        = pbp;

        RestoreInstrBP(hthd, pbp);
        SingleStep(hthd, ContinueSSMethod, FALSE, FALSE);
    }
}


VOID
StuffHthdx(
    DEBUG_EVENT64 *de,
    HPRCX hprc,
    HTHDX hthd
    )
/*++

Routine Description:

    Common code for CreateProcess and CreateThread events.  Stuff
    the hthd with default values, add the new structure to the process'
    chain.  Critical section csThreadProcList must be held when calling
    this routine.

Arguments:

    de - Supplies debug event

    hprc - Supplies process structure

    hthd - Supplies empty thread struct, returns full one.

Return Value:


--*/
{
    hthd->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse          = thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
    thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse       = hthd;

    hthd->nextSibling   = hprc->hthdChild;
    hprc->hthdChild     = (LPVOID)hthd;

    hthd->hprc          = hprc;
    hthd->htid          = 0;
    hthd->atBP          = (BREAKPOINT*)0;
    hthd->tstate        = ts_stopped;

    hthd->fAddrIsReal   = FALSE;
    hthd->fAddrIsFlat   = TRUE;
    hthd->fAddrOff32    = TRUE;
    hthd->fWowEvent     = FALSE;
    hthd->fStopOnNLG    = FALSE;
    hthd->fReturning    = FALSE;
    hthd->fDisplayReturnValues = TRUE;
#ifdef KERNEL
    hthd->fContextStale = TRUE;
#endif
#ifndef KERNEL
    hthd->pcs           = NULL;
    hthd->poleret       = NULL;
#endif
    InitializeListHead(&hthd->WalkList);

    hthd->tid           = de->dwThreadId;
    if (de->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        hthd->rwHand        = de->u.CreateProcessInfo.hThread;
        hthd->offTeb        = (OFFSET) de->u.CreateProcessInfo.lpThreadLocalBase;
        hthd->lpStartAddress= de->u.CreateProcessInfo.lpStartAddress;
    } else {
        hthd->rwHand        = de->u.CreateThread.hThread;
        hthd->offTeb        = (OFFSET) de->u.CreateThread.lpThreadLocalBase;
        hthd->lpStartAddress= de->u.CreateThread.lpStartAddress;
    }
}


void
ProcessCreateProcessEvent(
    DEBUG_EVENT64   * pde,
    HTHDX           hthd
    )
/*++

Routine Description:

    This routine does the processing needed for a create process
    event from the OS.  We need to do the following:

      - Set up our internal structures for the newly created thread
        and process
      - Get notifications back to the debugger

Arguments:

    pde    - Supplies pointer to the DEBUG_EVENT structure from the OS

    hthd   - Supplies thread descriptor that thread event occurred on

Return Value:

    none

--*/
{
    DEBUG_EVENT64               de2;
    CREATE_PROCESS_DEBUG_INFO64 *pcpd = &pde->u.CreateProcessInfo;
    HPRCX                       hprc;
    LPBYTE                      lpbPacket;
    WORD                        cbPacket;

    DEBUG_PRINT(_T("ProcessCreateProcessEvent\r\n"));

    ResetEvent(hEventCreateProcess);

    hprc = InitProcess((HPID)INVALID);

    //
    // Stuff the process structure
    //

    if (fUseRoot) {
        hprc->pstate           |= ps_root;
        hprc->hpid              = hpidRoot;
        fUseRoot                = FALSE;
        if (pcpd->lpStartAddress && FLoading16) {
            hprc->f16bit = TRUE;
        } else {
            hprc->f16bit = FALSE;
        }
    }

    hprc->pid     = pde->dwProcessId;
    hprc->pstate  |= ps_preStart;

    ResetEvent(hprc->hEventCreateThread);


    //
    // Create the first thread structure for this app
    //

    hthd = (HTHDX)MHAlloc(sizeof(HTHDXSTRUCT));
    memset(hthd, 0, sizeof(*hthd));

    EnterCriticalSection(&csThreadProcList);

    StuffHthdx(pde, hprc, hthd);

    LeaveCriticalSection(&csThreadProcList);


#ifndef KERNEL
#if defined(TARGET_i386)
    hthd->context.ContextFlags = CONTEXT_FULL;
    DbgGetThreadContext( hthd, &hthd->context );
    hprc->segCode = (SEGMENT) hthd->context.SegCs;
#endif  // i386
#endif  // !KERNEL


#ifndef KERNEL
    //
    // try to create a handle with more permissions
    //
    if (!CrashDump) {
        hprc->rwHand = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pde->dwProcessId);
        if (hprc->rwHand) {
            hprc->CloseProcessHandle = TRUE;
        } else {
            hprc->rwHand = pcpd->hProcess;
        }
    }
#endif // KERNEL

    /*
     * There is going to be a breakpoint to announce that the
     * process is loaded and runnable.
     */
    nWaitingForLdrBreakpoint++;
    if (pcpd->lpStartAddress == 0) {
        // in an attach, the BP will be in another thread.
        RegisterExpectedEvent( hprc,
                               (HTHDX)NULL,
                               BREAKPOINT_DEBUG_EVENT,
                               NO_SUBCLASS,
                               DONT_NOTIFY,
                               ActionDebugActiveReady,
                               FALSE,
                               (ULONG64)hprc);
#ifndef KERNEL
#if defined(INTERNAL)
        // don't ever let it defer name resolutions
        hprc->fNameRequired = TRUE;
#endif
#endif

    } else {
        // On a real start, the BP will be in the first thread.
        RegisterExpectedEvent( hthd->hprc,
                               hthd,
                               BREAKPOINT_DEBUG_EVENT,
                               NO_SUBCLASS,
                               DONT_NOTIFY,
                               ActionDebugNewReady,
                               FALSE,
                               (ULONG64)hprc);
    }


    //
    // Notify the EM of this newly created process.
    // If not the root proc, an hpid will be created and added
    // to the hprc by the em.
    //
    // We enter a crtitcal section to block the UI thread from 
    // querying the DM's prcList before it has been fully
    // initialized.
    EnterCriticalSection(&csThreadProcList);

    NotifyEM(pde, hthd, 0, (ULONG64)hprc);

    LeaveCriticalSection(&csThreadProcList);

#ifndef KERNEL
    /*
     *  We also need to drop out a module load notification
     *  on this exe.
     */

    assert( Is64PtrSE(pde->u.CreateProcessInfo.lpBaseOfImage) );

    de2.dwDebugEventCode        = LOAD_DLL_DEBUG_EVENT;
    de2.dwProcessId             = pde->dwProcessId;
    de2.dwThreadId              = pde->dwThreadId;
    de2.u.LoadDll.hFile         = pde->u.CreateProcessInfo.hFile;
    de2.u.LoadDll.lpBaseOfDll   = pde->u.CreateProcessInfo.lpBaseOfImage;
    de2.u.LoadDll.lpImageName   = pde->u.CreateProcessInfo.lpImageName;
    de2.u.LoadDll.fUnicode      = pde->u.CreateProcessInfo.fUnicode;    

    if (LoadDll(&de2, hthd, &cbPacket, &lpbPacket, FALSE) && (cbPacket != 0)) {
        NotifyEM(&de2, hthd, cbPacket, (ULONG64)lpbPacket);
    }
#endif // !KERNEL

    /*
     * Fake up a thread creation notification.
     */
    pde->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
    NotifyEM(pde, hthd, 0, (ULONG64)hprc);

    /*
     * Dont let the new process run:  the shell will say Go()
     * after receiving a CreateThread event.
     */

}                              /* ProcessCreateProcessEvent() */


void
ProcessCreateThreadEvent(
    DEBUG_EVENT64    *pde,
    HTHDX           creatorHthd
    )
{
    CREATE_THREAD_DEBUG_INFO64 *ctd = &pde->u.CreateThread;
    HTHDX                       hthd;
    HPRCX                       hprc;
    CONTEXT                     context;
#if defined(KERNEL) && defined(HAS_DEBUG_REGS)
    KSPECIAL_REGISTERS          ksr;
#endif // KERNEL && i386

    Unreferenced(creatorHthd);

    DPRINT(3, (_T("\n***CREATE THREAD EVENT\n")));

    //
    // Determine the hprc
    //
    hprc= HPRCFromPID(pde->dwProcessId);

    ResetEvent(hprc->hEventCreateThread);

    if (ctd->hThread == NULL) {
        DPRINT(1, (_T("BAD HANDLE! BAD HANDLE!(%08lx)\n"), ctd->hThread));
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  pde->dwProcessId,
                  pde->dwThreadId,
                  DBG_CONTINUE,
                  0);
        return;
    }

    if (!hprc) {
        DPRINT(1, (_T("BAD PID! BAD PID!\n")));
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  pde->dwProcessId,
                  pde->dwThreadId,
                  DBG_CONTINUE,
                  0);
        return;
    }

    //
    // Create the thread structure
    //
    hthd = (HTHDX)MHAlloc(sizeof(HTHDXSTRUCT));
    memset( hthd, 0, sizeof(*hthd));

    //
    // Stuff the structure
    //

    EnterCriticalSection(&csThreadProcList);

    StuffHthdx(pde, hprc, hthd);

    LeaveCriticalSection(&csThreadProcList);

    //
    //  Let the expression breakpoint manager that a new thread
    //  has been created.
    //

    ExprBPCreateThread( hprc, hthd );

    //
    // initialize cache entries
    //

    context.ContextFlags = CONTEXT_FULL;
    DbgGetThreadContext( hthd, &context );

#if defined(KERNEL) && defined(HAS_DEBUG_REGS)
    GetExtendedContext( hthd, &ksr );
#endif  // KERNEL && i386

    //
    //  Notify the EM of this new thread
    //

    if (fDisconnected) {
        ContinueThread(hthd);
    } else {
        NotifyEM(pde, hthd, 0, (ULONG64)hprc);
    }

    return;
}


void
ProcessExitProcessEvent(
    DEBUG_EVENT64* pde,
    HTHDX hthd
    )
{
    HPRCX               hprc;
    XOSD                xosd;
    HTHDX               hthdT;
    BREAKPOINT        * pbp;
    BREAKPOINT        * pbpT;

    DPRINT(3, (_T("ProcessExitProcessEvent\n")));

    /*
     * do all exit thread handling:
     *
     * If thread was created during/after the
     * beginning of termination processing, we didn't
     * pick it up, so don't try to destroy it.
     */

    if (!hthd) {
        hprc = HPRCFromPID(pde->dwProcessId);
    } else {
        hprc = hthd->hprc;
        hthd->tstate |= ts_dead;
        hthd->dwExitCode = pde->u.ExitProcess.dwExitCode;
    }

#ifndef KERNEL
    DEBUG_PRINT(_T("Try unloading modules.\r\n"));

    UnloadAllModules( hprc, hthd? hthd:hprc->hthdChild, FALSE, TRUE );

    DEBUG_PRINT(_T("Done with UnloadAllModules.\r\n"));
#endif

    /* and exit process */

    hprc->pstate |= ps_dead;
    hprc->dwExitCode = pde->u.ExitProcess.dwExitCode;
    ConsumeAllProcessEvents(hprc, TRUE);

    /*
     * Clean up BP records
     */

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
        pbpT = BPNextHprcPbp(hprc, pbp);
        RemoveBP(pbp);
    }

#ifndef KERNEL
    /*
     * Clean up any Fibers that are left.
     */

    RemoveFiberList(hprc);
#endif

    /*
     * If we haven't seen EXIT_THREAD events for any
     * threads, we aren't going to, so consider them done.
     */

    for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
        if ( !(hthdT->tstate & ts_dead) ) {
            hthdT->tstate |= ts_dead;
            hthdT->tstate &= ~ts_stopped;
        }
    }

    /*
     *  If process hasn't initialized yet, we were expecting
     *  a breakpoint to notify us that all the DLLs are here.
     *  We didn't get that yet, so reply here.
     */
    if (hprc->pstate & ps_preStart) {
        xosd = xosdUnknown;
        DmTlFunc( tlfReply, hprc->hpid, sizeof(XOSD), (LPARAM) &xosd);
    }


    if (!(hprc->pstate & ps_killed)) {

        assert(hthd);

        pde->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
        pde->u.ExitThread.dwExitCode = hprc->dwExitCode;
        NotifyEM(pde, hthd, 0, 0);

    } else {

        /*
         * If ProcessTerminateProcessCmd() killed this,
         * silently continue the event and release the semaphore.
         *
         * Don't notify the EM of anything; ProcessUnloadCmd()
         * will do that for any undestroyed threads.
         */

        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  pde->dwProcessId,
                  pde->dwThreadId,
                  DBG_CONTINUE,
                  0);
    }

    DMSqlTerminate(hprc);
    SetEvent(hprc->hExitEvent);
}                                      /* ProcessExitProcessEvent() */



void
ProcessExitThreadEvent(
    DEBUG_EVENT64* pde,
    HTHDX hthd
    )
{
    HPRCX       hprc = hthd->hprc;

    DPRINT(3, (_T("***** ProcessExitThreadEvent, hthd == %p\n"), hthd));


    hthd->tstate |= ts_dead;

    if (hthd->tstate & ts_frozen) {
        ResumeThread(hthd->rwHand);
        hthd->tstate &= ~ts_frozen;
    }

    hthd->dwExitCode = pde->u.ExitThread.dwExitCode;

    /*
     *  Free all events for this thread
     */

    ConsumeAllThreadEvents(hthd, TRUE);

    //
    //  Let the Expression Breakpoint manager know that this thread
    //  is gone.
    //
    ExprBPExitThread( hprc, hthd );


    if (hprc->pstate & ps_killed) {
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
    } else if (fDisconnected) {
        hthd->hprc->pstate |= ps_exited;
        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
    } else {
        NotifyEM(pde, hthd, 0, 0);
    }

    return;
}                                      /* ProcessExitThreadEvent() */


void
ProcessLoadDLLEvent(
    DEBUG_EVENT64* de,
    HTHDX hthd
    )
{
    LPBYTE lpbPacket;
    WORD   cbPacket;

    if (LoadDll(de, hthd, &cbPacket, &lpbPacket, TRUE) || (cbPacket == 0)) {
        NotifyEM(de, hthd, cbPacket, (ULONG64)lpbPacket);
    }

    return;
}                                      /* ProcessLoadDLLEvent() */


void
ProcessUnloadDLLEvent(
    DEBUG_EVENT64* pde,
    HTHDX hthd
    )
{
    int         iDll;
    HPRCX       hprc = hthd->hprc;

    DPRINT(10, (_T("*** UnloadDll %I64x\n"), pde->u.UnloadDll.lpBaseOfDll));

    assert( Is64PtrSE(pde->u.UnloadDll.lpBaseOfDll) );

    for (iDll = 0; iDll < hprc->cDllList; iDll++) {
        if (hprc->rgDllList[iDll].fValidDll) {
            
            assert( Is64PtrSE(hprc->rgDllList[iDll].offBaseOfImage) );

            if (hprc->rgDllList[iDll].offBaseOfImage ==
                    (OFFSET) pde->u.UnloadDll.lpBaseOfDll) {

                break;
            }
        }
    }

    /*
     *  Make sure that we found a legal address.  If not then assert
     *  and check for problems.
     */

#ifndef KERNEL
    // this happens all the time in kernel mode
    // when user mode modloads are enabled
    assert( iDll != hprc->cDllList );
#endif

    if (iDll != hprc->cDllList) {

        if (!fDisconnected) {
            NotifyEM(pde, hthd, 0, 0);
        }

        DestroyDllLoadItem( &hprc->rgDllList[iDll] );
    }

    ContinueThread(hthd);
    return;
}


void
DestroyDllLoadItem(
    PDLLLOAD_ITEM pDll
    )
{
    if (pDll->szDllName) {
        MHFree(pDll->szDllName);
        pDll->szDllName = NULL;
    }

#ifdef KERNEL
    if (pDll->sec) {
        MHFree(pDll->sec);
        pDll->sec = NULL;
    }
#endif

    pDll->offBaseOfImage = 0;
    pDll->cbImage = 0;
    pDll->fValidDll = FALSE;

    return;
}


void
ProcessOutputDebugStringEvent(
    DEBUG_EVENT64* de,
    HTHDX hthd
    )
/*++

Routine Description:

    Handle an OutputDebugString from the debuggee

Arguments:

    de      - Supplies DEBUG_EVENT64 struct

    hthd    - Supplies thread descriptor for thread
              that generated the event

Return Value:

    None

--*/
{
    int     cb = de->u.DebugString.nDebugStringLength;

#if DBG
    DWORD   cbR;
    char    rgch[256];
    HANDLE  rwHand;

    cb = min(cb, 256);
    rwHand = hthd->hprc->rwHand;
    DbgReadMemory(hthd->hprc, de->u.DebugString.lpDebugStringData,
        rgch, cb, &cbR);
    rgch[cbR] = 0;

    DPRINT(3, (_T("%s\n"), rgch));
#endif

    NotifyEM(de, hthd, 0, 0);

    ContinueThread(hthd);

    return;
}

#ifndef KERNEL

void
ProcessBogusSSEvent(
    DEBUG_EVENT64* de,
    HTHDX hthd
    )
/*++

Routine Description:

    Handle a Bogus Win95 Single step event. Just continue execution
        as if nothing happened.

Arguments:

    de      - Supplies DEBUG_EVENT64 struct

    hthd    - Supplies thread descriptor for thread
              that generated the event

Return Value:

    None

--*/
{
    ContinueThread(hthd);
    return;
}

#endif


void
Reply(
    UINT   length,
    LPVOID lpbBuffer,
    HPID   hpid
    )
/*++

Routine Description:

    Send a reply packet to a tlfRequest from the EM

Arguments:

    length      - Supplies length of reply
    lpbBuffer   - Supplies reply data
    hpid        - Supplies handle to EM process descriptor

Return Value:

    None

--*/
{
    /*
     *  Add the size of the packet header to the length
     */

    length += FIELD_OFFSET(DM_MSG, rgb);

    DPRINT(5, (_T("Reply to EM [%d]\n"), length));

    assert(length <= sizeof(abDMReplyBuf) || lpbBuffer != abDMReplyBuf);

    if (DmTlFunc) { // IF there is a TL loaded, reply
        DmTlFunc(tlfReply, hpid, length, (LPARAM) lpbBuffer);
    }

    return;
}



#ifdef KERNEL
#define DMAPILOCK()     \
    if (!TryApiLock()) {\
        LpDmMsg->xosdRet = xosdTargetIsRunning;\
        Reply( sizeof( XOSD ), LpDmMsg, lpdbb->hpid );\
        break;\
    }

#define DMAPIRELEASE()  ReleaseApiLock()
#else
#define DMAPILOCK()
#define DMAPIRELEASE()
#endif

VOID FAR PASCAL
DMFunc(
    DWORD cb,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This is the main entry point for the DM.  This takes dmf
    message packets from the debugger and handles them, usually
    by dispatching to a worker function.

Arguments:

    cb      - supplies size of data packet

    lpdbb   - supplies pointer to packet

Return Value:


--*/
{
    DMF     dmf;
    HPRCX   hprc;
    HTHDX   hthd;
    XOSD    xosd = xosdNone;

    dmf = (DMF) (lpdbb->dmf & 0xffff);
    DEBUG_PRINT(_T("DMFunc\r\n"));

    DPRINT(5, (_T("DmFunc [%2x] "), dmf));

    hprc = HPRCFromHPID(lpdbb->hpid);
    hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);

    switch ( dmf ) {

      case dmfSelLim:
        DPRINT(5, (_T("dmfSelLim\n")));
        DMAPILOCK();
        ProcessSelLimCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

      case dmfSetMulti:
        DPRINT(5, (_T("dmfSetMulti\n")));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfClearMulti:
        DPRINT(5, (_T("dmfClearMulti\n")));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfDebugger:
        DPRINT(5, (_T("dmfDebugger\n")));
        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;

      case dmfCreatePid:
        DPRINT(5,(_T("dmfCreatePid\r\n")));
        ProcessCreateProcessCmd(hprc, hthd, lpdbb);
        break;

      case dmfDestroyPid:
        DPRINT(5, (_T("dmfDestroyPid\n")));
        LpDmMsg->xosdRet = FreeProcess(hprc, TRUE);
        Reply( 0, LpDmMsg, lpdbb->hpid);
        break;

      case dmfSpawnOrphan:
        DPRINT(5, (_T("dmfSpawnOrphan\n")));
#ifdef KERNEL
        assert(!"Spawn orphan not allowed in kernel mode");
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid);
#else
        ProcessSpawnOrphanCmd (hprc, hthd, lpdbb);
#endif
        break;

      case dmfProgLoad:
        DPRINT(5, (_T("dmfProgLoad\n")));
        ProcessProgLoadCmd(hprc, hthd, lpdbb);
        break;

      case dmfProgFree:
        DPRINT(5, (_T("dmfProgFree\n")));


#ifndef KERNEL

        ProcessTerminateProcessCmd(hprc, hthd, lpdbb);

#else // KERNEL
        if (!hprc) {
            LpDmMsg->xosdRet = xosdNone;
            Reply( 0, LpDmMsg, lpdbb->hpid);
            break;
        }

        if (KdOptions[KDO_GOEXIT].value) {
            HTHDX hthdT;
            PBREAKPOINT bp;
            KdOptions[KDO_GOEXIT].value = 0;
            for (hthdT = hprc->hthdChild; hthdT; hthdT = hthdT->nextSibling) {
                if (hthdT->tstate & ts_stopped) {
                    if (bp = AtBP(hthdT)) {
                        if (!hthdT->fDontStepOff) {
                            IncrementIP(hthdT);
                        }
                    }
                    if (hthdT->fContextDirty) {
                        DbgSetThreadContext( hthdT, &hthdT->context );
                        hthdT->fContextDirty = FALSE;
                    }
                    KdOptions[KDO_GOEXIT].value = 1;
                    break;
                }
            }
        }

        DMAPILOCK();

        ClearBps();

        ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();

        ProcessUnloadCmd(hprc, hthd, lpdbb);

        if (KdOptions[KDO_GOEXIT].value) {
            ContinueTargetSystem( DBG_CONTINUE, NULL );
            // Hack: don't exit from here with lock held
            DMAPIRELEASE();
        }


#endif  // KERNEL

        LpDmMsg->xosdRet = xosdNone;
        Reply( 0, LpDmMsg, lpdbb->hpid);
        break;

      case dmfBreakpoint:
        DPRINT(5,(_T("dmfBreakpoint\r\n")));
        DMAPILOCK();
        ProcessBreakpointCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

      case dmfReadMem:
        // This replies in the function
        DPRINT(5, (_T("dmfReadMem\n")));
        DMAPILOCK();
        ProcessReadMemoryCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

      case dmfWriteMem:
        DPRINT(5, (_T("dmfWriteMem\n")));
        DMAPILOCK();
        ProcessWriteMemoryCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

      case dmfReadReg:
        DPRINT(5, (_T("dmfReadReg\n")));
        DMAPILOCK();
        ProcessGetContextCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

      case dmfWriteReg:
        DPRINT(5, (_T("dmfWriteReg\n")));
        DMAPILOCK();
        ProcessSetContextCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;


#ifdef HAS_DEBUG_REGS
      case dmfReadRegEx:
        DPRINT(5, (_T("dmfReadRegEx\n")));
        DMAPILOCK();
#ifdef KERNEL
        ProcessGetExtendedContextCmd(hprc, hthd, lpdbb);
#else
        ProcessGetDRegsCmd(hprc, hthd, lpdbb);
#endif
        DMAPIRELEASE();
        break;

      case dmfWriteRegEx:
        DPRINT(5, (_T("dmfWriteRegEx\n")));
        DMAPILOCK();
#ifdef KERNEL
        ProcessSetExtendedContextCmd(hprc, hthd, lpdbb);
#else
        ProcessSetDRegsCmd(hprc, hthd, lpdbb);
#endif
        DMAPIRELEASE();
        break;

#else   // HAS_DEBUG_REGS
      case dmfReadRegEx:
      case dmfWriteRegEx:
        DPRINT(5,(_T("Read/WriteRegEx\r\n")));
        assert(dmf != dmfReadRegEx && dmf != dmfWriteRegEx);
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        break;
#endif  // HAS_DEBUG_REGS

      case dmfGo:
        DPRINT(5, (_T("dmfGo\n")));
#if !defined(KERNEL)
        //Turn off fiber debugging
        hprc->pFbrCntx = NULL;
#endif
        //
        // We should not have to lock this, since it should
        // be a nop if the target is running anyway.
        //
        ProcessContinueCmd(hprc, hthd, lpdbb);
        break;

#if defined(KERNEL)
      case dmfTerm:
        DPRINT(5, (_T("dmfTerm\n")));
        DMAPILOCK();
        ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;
#endif

      case dmfStop:
        DPRINT(5, (_T("dmfStop\n")));
        ProcessAsyncStopCmd(hprc, hthd, lpdbb);
        break;

      case dmfFreeze:
        DPRINT(5, (_T("dmfFreeze\n")));
        ProcessFreezeThreadCmd(hprc, hthd, lpdbb);
        break;

      case dmfResume:
        DPRINT(5, (_T("dmfResume\n")));
        ProcessAsyncGoCmd(hprc, hthd, lpdbb);
        break;

      case dmfInit:
        DPRINT(5, (_T("dmfInit\n")));
        Reply( 0, &xosd, lpdbb->hpid);
        break;

      case dmfUnInit:
        DPRINT(5, (_T("dmfUnInit\n")));
#ifdef KERNEL
        DmPollTerminate();
#endif
        Reply ( 1, LpDmMsg, lpdbb->hpid);
#ifndef KERNEL
        Cleanup();
#endif
        break;

      case dmfGetDmInfo:
        DPRINT(5,(_T("getDmInfo\r\n")));
        ProcessGetDmInfoCmd(hprc, lpdbb, cb);
        break;

    case dmfSetupExecute:
        DPRINT(5, (_T("dmfSetupExecute\n")));
        DMAPILOCK();
        ProcessSetupExecuteCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

    case dmfStartExecute:
        DPRINT(5, (_T("dmfStartExecute\n")));
        DMAPILOCK();
        ProcessStartExecuteCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

    case dmfCleanUpExecute:
        DPRINT(5, (_T("dmfCleanupExecute\n")));
        DMAPILOCK();
        ProcessCleanUpExecuteCmd(hprc, hthd, lpdbb);
        DMAPIRELEASE();
        break;

    case dmfSystemService:
        DPRINT(5, (_T("dmfSystemService\n")));
        ProcessSystemServiceCmd( hprc, hthd, lpdbb );
        break;

    case dmfDebugActive:
        DPRINT(5, (_T("dmfDebugActive\n")));
        ProcessDebugActiveCmd( hprc, hthd, lpdbb);
        break;

    case dmfSetPath:
        DPRINT(5, (_T("dmfSetPath\n")));
        ProcessSetPathCmd( hprc, hthd, lpdbb );
        break;

    case dmfQueryTlsBase:
        DPRINT(5, (_T("dmfQueryTlsBase\n")));
        ProcessQueryTlsBaseCmd(hprc, hthd, lpdbb );
        break;

    case dmfQuerySelector:
        DPRINT(5, (_T("dmfQuerySelector\n")));
        DMAPILOCK();
        ProcessQuerySelectorCmd(hprc, hthd, lpdbb );
        DMAPIRELEASE();
        break;

    case dmfVirtualQuery:
        DPRINT(5,(_T("VirtualQuery\r\n")));
        //DMAPILOCK();
        ProcessVirtualQueryCmd(hprc, lpdbb);
        //DMAPIRELEASE();
        break;

    case dmfRemoteQuit:
        DPRINT(5,(_T("RemoteQuit\r\n")));
        ProcessRemoteQuit();
        break;

#ifdef KERNEL
    case dmfGetSections:
        DPRINT(5,(_T("GetGetSections\r\n")));
        DMAPILOCK();
        ProcessGetSectionsCmd( hprc, hthd, lpdbb );
        DMAPIRELEASE();
        break;
#endif

    case dmfSetExceptionState:
        DPRINT(5,(_T("SetExceptionState\r\n")));
        ProcessSetExceptionState(hprc, hthd, lpdbb);
        break;

    case dmfGetExceptionState:
        DPRINT(5,(_T("GetExceptionState\r\n")));
        ProcessGetExceptionState(hprc, hthd, lpdbb);
        break;

    case dmfSingleStep:
        DPRINT(5,(_T("SingleStep\r\n")));
#if !defined(KERNEL)
        //Turn off fiber debugging
        hprc->pFbrCntx = NULL;
#endif
        //
        // again, this should not do anything unless the target is stopped,
        // so don't lock it.
        //
        ProcessSingleStepCmd(hprc, hthd, lpdbb);
        break;

    case dmfRangeStep:
        DPRINT(5,(_T("RangeStep\r\n")));
#if !defined(KERNEL)
        //Turn off fiber debugging
        hprc->pFbrCntx = NULL;
#endif
        ProcessRangeStepCmd(hprc, hthd, lpdbb);
        break;

    case dmfReturnStep:
        DPRINT(5,(_T("ReturnStep\r\n")));
        ProcessReturnStepCmd(hprc, hthd, lpdbb);
        break;

#ifndef KERNEL
    case dmfNonLocalGoto:
        DPRINT(5,(_T("NonLocalGoto\r\n")));
        //Turn off fiber debugging
        hprc->pFbrCntx = NULL;
        ProcessNonLocalGoto(hprc, hthd, lpdbb);
        break;
#endif

    case dmfThreadStatus:
        DPRINT(5,(_T("ThreadStatus\r\n")));
        Reply( ProcessThreadStatCmd(hprc, hthd, lpdbb), LpDmMsg, lpdbb->hpid);
        break;

    case dmfProcessStatus:
        DPRINT(5,(_T("ProcessStatus\r\n")));
        if (hprc) {
            LpDmMsg->xosdRet = xosdNone;
            Reply( ProcessProcStatCmd(hprc, hthd, lpdbb), LpDmMsg, lpdbb->hpid);
        } else {
            LpDmMsg->xosdRet = xosdBadProcess;
            Reply( 0, LpDmMsg, lpdbb->hpid);
        }
        break;

    case dmfGetTimeStamp:
        DPRINT(5,(_T("ProcessGetTimeStamp\r\n")));
        Reply (ProcessGetTimeStamp (hprc, hthd, lpdbb), LpDmMsg, lpdbb->hpid);
        break;

    case dmfNewSymbolsLoaded:
#ifdef KERNEL
        GetKernelSymbolAddresses();
#endif
        Reply( 0, LpDmMsg, lpdbb->hpid);
        break;

    default:
        DPRINT(5, (_T("DM: Unknown dmf == %d\n"), dmf));
        assert(FALSE);
        break;
    }

    return;
}                         /* DMFunc() */
#undef DMAPILOCK
#undef DMAPIRELEASE


#ifndef KERNEL

BOOL
StartDmPollThread(
    void
    )
/*++

Routine Description:

    This creates the DM poll thread.

Arguments:

    none

Return Value:

    TRUE if the thread was successfully created or already
    existed.

--*/
{
    DWORD   tid;

    if (hDmPollThread) {
        return TRUE;
    }

    hDmPollThread = CreateThread(0,0,CallDmPoll,0,0,&tid);

    return hDmPollThread != 0;
}
BOOL
StartCrashPollThread(
    void
    )
/*++

Routine Description:

    This creates the DM poll thread for a crash dump file.

Arguments:

    none

Return Value:

    TRUE if the thread was successfully created or already existed.

--*/
{
    DWORD   tid;

    if (hDmPollThread) {
        return TRUE;
    }

    hDmPollThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)CrashDumpThread,0,0,&tid);

    return hDmPollThread != 0;
}

VOID
CrashDumpThread(
    LPVOID lpv
    )
{
    HPRCX           hprc;
    HTHDX           hthd;
    HTHDX           hthdNew;
    LPBYTE          lpbPacket;
    WORD            cbPacket;
    PCRASH_MODULE   CrashModule;
    DEBUG_EVENT64   de;
    DWORD           i;
    CHAR            buf[32];
    LPTSTR          lpProgName = (LPTSTR)lpv;
    DWORD           DumpVer = CrashDumpHeader->MajorVersion;
    CRASHDUMP_VERSION_INFO VersionInfo;


    CrashDump = TRUE;

    //
    // fix up version bugs
    //
    DmpDetectVersionParameters( &VersionInfo );

    //
    // simulate a create process debug event
    //
    CrashModule = (PCRASH_MODULE)((PUCHAR)CrashDumpHeader+CrashDumpHeader->ModuleOffset);
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = 1;
    de.dwThreadId  = 1;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = NULL;
    de.u.CreateProcessInfo.hThread = NULL;
    de.u.CreateProcessInfo.lpBaseOfImage = CrashModule->BaseOfImage;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = 0;
    de.u.CreateProcessInfo.lpThreadLocalBase = 0;
    
    if (!CrashModule->ImageName[0]) {
        de.u.CreateProcessInfo.lpImageName = SEPtrTo64("mod0.dll");
    } else {
        de.u.CreateProcessInfo.lpImageName = SEPtrTo64(CrashModule->ImageName);
    }

    _ftcscpy( nameBuffer, (PVOID)de.u.CreateProcessInfo.lpImageName);
    de.u.CreateProcessInfo.fUnicode = FALSE;
    ProcessDebugEvent(&de);

    WaitForSingleObject( hEventCreateProcess, INFINITE );

    //
    // wait for the shell to say go to the thread
    //
    WaitForSingleObject( hEventContinue, INFINITE );

    //
    // mark the process as 'being connected' so that the continue debug
    // events that are received from the shell are ignored
    //
    hprc = HPRCFromPID(1);
    hthd = hprc->hthdChild;
    hprc->pstate |= ps_connect;

    //
    // Get rid of the expected breakpoint
    //
    ConsumeAllProcessEvents(hprc, FALSE);

    if (DumpVer >= 4) {
        DmpGetThread( 0, &hthd->CrashThread );
        hthd->offTeb = hthd->CrashThread.Teb;
    } else {
        ZeroMemory(&hthd->CrashThread, sizeof(CRASH_THREAD));
        hthd->CrashThread.ThreadId = hthd->tid;
    }

    //
    // generates the mod load events
    //
    for (i=1; i<CrashDumpHeader->ModuleCount; i++) {
        //
        // drwtsn32 puts a bogus value in the name length field
        // when the string is empty:
        //
        int namelen = CrashModule->ImageName[0] ? CrashModule->ImageNameLength : 1;

        CrashModule = (PCRASH_MODULE) ( (PUCHAR)CrashModule +
                                        sizeof(CRASH_MODULE) +
                                        namelen );
        de.dwDebugEventCode                = LOAD_DLL_DEBUG_EVENT;
        de.dwProcessId                     = 1;
        de.dwThreadId                      = 1;
        de.u.LoadDll.hFile                 = NULL;

        //
        // Sign extend.....
        //
        if (sizeof(CrashModule->BaseOfImage) == sizeof(DWORD64)) {
            de.u.LoadDll.lpBaseOfDll = CrashModule->BaseOfImage;
        } else {
            de.u.LoadDll.lpBaseOfDll = SE32To64(CrashModule->BaseOfImage);
        }
        
        if (!CrashModule->ImageName[0]) {
            sprintf( buf, "mod%d.dll", i );
            de.u.LoadDll.lpImageName = SEPtrTo64(buf);
        } else {
            de.u.LoadDll.lpImageName = SEPtrTo64(CrashModule->ImageName);
        }
        
        de.u.LoadDll.fUnicode              = FALSE;
        de.u.LoadDll.nDebugInfoSize        = 0;
        de.u.LoadDll.dwDebugInfoFileOffset = 0;
        if (LoadDll(&de,hthd,&cbPacket,&lpbPacket, FALSE) && (cbPacket != 0)) {
            NotifyEM(&de, hthd, cbPacket, (ULONG64)lpbPacket);
        }
    }

    //
    // generate a load complete debug event
    //
    ResetEvent( hEventContinue );
    de.dwProcessId = hprc->pid;
    de.dwThreadId = hprc->hthdChild->tid;
    de.dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;
    NotifyEM( &de, hthd, 0, 0L);

    WaitForSingleObject( hEventContinue, INFINITE );


    //
    // create all of the threads
    //
    for (i=1; i<CrashDumpHeader->ThreadCount; i++) {
        //
        // generate a thread create event
        //
        ResetEvent( hprc->hEventCreateThread );
        ResetEvent( hEventContinue );
        de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        de.dwProcessId = hprc->pid;
        de.dwThreadId = i + 1;
        de.u.CreateThread.hThread = (HANDLE) (i + 1);
        de.u.CreateThread.lpThreadLocalBase = 0;
        de.u.CreateThread.lpStartAddress = 0;

        //
        // Take critical section here so that it is still
        // held after leaving ProcessDebugEvent
        //
        EnterCriticalSection(&csProcessDebugEvent);
        ProcessDebugEvent(&de);
        hthdNew = HTHDXFromPIDTID(1,(i+1));
        DbgGetThreadContext( hthdNew, &hthdNew->context );
        if (DumpVer >= 4) {
            DmpGetThread( i, &hthdNew->CrashThread );
            hthdNew->offTeb = hthdNew->CrashThread.Teb;
        } else {
            ZeroMemory(&hthdNew->CrashThread, sizeof(CRASH_THREAD));
            hthdNew->CrashThread.ThreadId = hthdNew->tid;
        }
        LeaveCriticalSection(&csProcessDebugEvent);
        WaitForSingleObject( hprc->hEventCreateThread, INFINITE );

        //
        // wait for the shell to continue the new thread
        //
        WaitForSingleObject( hEventContinue, INFINITE );
    }

    //
    // Get the debug event for the crash
    //
    if (64 == VersionInfo.PointerSize) {
        de = *(LPDEBUG_EVENT64)((PUCHAR)CrashDumpHeader+CrashDumpHeader->DebugEventOffset);
        //
        // convert the thread and process ids into the internal version...
        //
        de.dwProcessId = 1;
        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (de.dwThreadId == hthd->CrashThread.ThreadId) {
                de.dwThreadId = hthd->tid;
                break;
            }
        }
    } else {
        //
        // if this is an old crashdump file, try to find the crashed thread
        //
        de.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;

        //
        // BUGBUG kentf no support yet for 64 bit crashdump
        //
        ExceptionRecord32To64((PEXCEPTION_RECORD32)CrashException, &de.u.Exception.ExceptionRecord);

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (PC(hthd) == de.u.Exception.ExceptionRecord.ExceptionAddress) {
                de.dwThreadId = hthd->tid;
            }
        }
    }

    ProcessDebugEvent( &de );

    while (!fDmPollQuit) {
        //
        // Handle kill commands
        //
        if (KillQueue) {
            CompleteTerminateProcessCmd();
        }

        if (ReloadStruct.Flag) {
            ReloadUsermodeModules(ReloadStruct.Hthd, ReloadStruct.String);
            free((PVOID)ReloadStruct.String);
            ReloadStruct.Flag = 0;
        }

        Sleep( 500 );
    }
}


DWORD WINAPI
CallDmPoll(
    LPVOID lpv
    )

/*++

Routine Description:

    This is the debug event loop.  This routine creates or
    attaches to child process, and monitors them for debug
    events.  It serializes the dispatching of events from
    multiple process, and continues the events when the
    worker functions have finished processing the events.

Arguments:

    lpv - Supplies an argument provided by CreateThread.

Return Value:

    None.

--*/

{
    DEBUG_EVENT64 de;
    PROCESS_INFORMATION pi;
    int         nprocs = 0;
    UINT        ErrMode;

    Unreferenced( lpv );
    DPRINT(5,(_T("CallDmPoll\r\n")));

    //
    // Crank up priority to improve performance, and improve our
    // chances of winning races with the debuggee.
    //
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    fDmPollQuit = FALSE;
    while (!fDmPollQuit) {

        //
        // Handle kill commands
        //
        if (KillQueue) {
            CompleteTerminateProcessCmd();
            goto doContinues;
        }

        //
        // Handle spawn commands
        //
        if (SpawnStruct.fSpawn) {
            SpawnStruct.fSpawn = FALSE;
            ErrMode = SetErrorMode( 0 );
            SpawnStruct.fReturn =
                CreateProcess( SpawnStruct.szAppName,
                             SpawnStruct.szArgs,
                             NULL,
                             NULL,
                             SpawnStruct.fInheritHandles,
                             SpawnStruct.fdwCreate,
                             NULL,
                             SpawnStruct.pszCurrentDirectory,
                             &SpawnStruct.si,
                             &pi
                             );
            SetErrorMode( ErrMode );
            if (SpawnStruct.fReturn == 0) {
                SpawnStruct.dwError = GetLastError();
            } else {
                SpawnStruct.dwError = 0;
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                Close3Handles(rgh);
            }

            SetEvent(SpawnStruct.hEventApiDone);
        }

        //
        // Handle attach commands
        //
        if (DebugActiveStruct.fAttach) {
            if (fDisconnected && DebugActiveStruct.dwProcessId == 0) {
                SetEvent( hEventRemoteQuit );
                SetEvent( DebugActiveStruct.hEventReady );
                DebugActiveStruct.fReturn = 1;
                DebugActiveStruct.dwError = 0;
            } else {
                DebugActiveStruct.fAttach = FALSE;
                SetDebugPrivilege ();
                DebugActiveStruct.fReturn = DebugActiveProcess(DebugActiveStruct.dwProcessId);
                if (DebugActiveStruct.fReturn == 0) {
                    DebugActiveStruct.dwError = GetLastError();
                } else {
                    DebugActiveStruct.dwError = 0;
                }
            }
            SetEvent(DebugActiveStruct.hEventApiDone);
        }

        if (WtStruct.fWt) {
            WtStruct.fWt = FALSE;
            switch(WtStruct.dwType) {
                case IG_WATCH_TIME_STOP:
                    WtStruct.hthd->wtmode = 2;
                    break;

                case IG_WATCH_TIME_RECALL:
                    break;

                case IG_WATCH_TIME_PROCS:
                    break;
            }
        }

        if (ReloadStruct.Flag) {
            ReloadUsermodeModules(ReloadStruct.Hthd, ReloadStruct.String);
            free((PVOID)ReloadStruct.String);
            ReloadStruct.Flag = 0;
        }

        if (DMWaitForDebugEvent(&de, (DWORD) WAITFORDEBUG_MS)) {

#ifndef KERNEL
            {
                PDETOSAVE pDeToSave = GetNextFreeDebugEvent();
                pDeToSave->de = de;
                InsertTailList(&DmListOfPendingDebugEvents, &pDeToSave->List);
            }
#endif // !KERNEL

            if ( de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT ) {
                assert(HPRCFromPID(de.dwProcessId) == NULL);

                // post a dummy message to the app so it will unblock (W95)

                if (IsChicago() && DebugActiveStruct.fReturn) {
                    PostThreadMessage (de.dwThreadId, WM_NULL, 0, 0);
                }


                if (nprocs == 0) {
                    ResetEvent(hEventNoDebuggee);
                }
                ++nprocs;
            } else if ( de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
                --nprocs;
            }

            if (fDisconnected) {
                if (de.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT       ||
                    de.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT     ||
                    de.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT  ||
                    de.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT      ) {

                    //
                    // we can process these debug events very carefully
                    // while disconnected from the shell.  the only requirement
                    // is that the dm doesn't call NotifyEM while disconnected.
                    //

                } else
#if 0
                if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {

                    //
                    // if the exception is to be ignored, we can handle it here.
                    // if not, we have to wait for a reconnect.
                    //

                } else
#endif
                {

                    WaitForSingleObject( hEventRemoteQuit, INFINITE );
                    ResetEvent( hEventRemoteQuit );
                    fDisconnected = FALSE;
                    //
                    // this is a remote session reconnecting
                    //
                    ReConnectDebugger( &de, FALSE );

                }
            }

            ProcessDebugEvent(&de);

        } else if (WaitForSingleObject( hEventRemoteQuit, 0 ) == WAIT_OBJECT_0) {

            //
            // this is a remote session reconnecting
            //
            ResetEvent( hEventRemoteQuit );
            ReConnectDebugger( NULL, FALSE );

        } else if (nWaitingForLdrBreakpoint) {

            // look for processes that are looking for a loader bp.
            // See how long it has been since they got an event.

            HPRCX hprc;
            BOOL Exited = FALSE;

            EnterCriticalSection(&csThreadProcList);
            for (hprc = prcList->next; hprc; hprc = hprc->next) {
                if (hprc->pstate & ps_preStart) {
                    if (++hprc->cLdrBPWait > LDRBP_MAXTICKS) {

                        //
                        // Signal a timeout for this one.
                        // just jump out of this loop - if
                        // another one is going to time out,
                        // it can do it on the next pass.
                        //
                        break;


                        //
                        // See if a pending attach has failed because the target process
                        // exited before the debugger got hooked up.
                        //
                    } else if (WaitForSingleObject(DebugActiveStruct.hEventReady, 0) != 0
                            && hprc->pid == DebugActiveStruct.dwProcessId
                            && DebugActiveStruct.hProcess != INVALID_HANDLE_VALUE
                            && WaitForSingleObject(DebugActiveStruct.hProcess, 0) == 0) {
                        //
                        // An attached process exited before it really got attached
                        //
                        Exited = TRUE;
                        break;
                    }
                }
            }
            LeaveCriticalSection(&csThreadProcList);

            if (hprc) {
                HandleDebugActiveDeadlock(hprc, Exited);
            }

        } else if (nprocs == 0) {

            Sleep(WAITFORDEBUG_MS);

        }

    doContinues:
        if (DequeueAllEvents(TRUE, FALSE) && nprocs <= 0) {
            SetEvent(hEventNoDebuggee);
        }
    }

    DEBUG_PRINT(_T("CallDmPoll Exit\r\n"));

    return 0;
}                               /* CallDmPoll() */


BOOL
SetDebugPrivilege(
    void
    )
/*++

Routine Description:

    Enables SeDebugPrivilege if possible.

Arguments:

    none

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HANDLE  hToken;
    LUID    DebugValue;
    TOKEN_PRIVILEGES tkp;
    BOOL    rVal = TRUE;

    if (!OpenProcessToken(GetCurrentProcess(),
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &DebugValue)) {

        rVal = FALSE;

    } else {

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Luid = DebugValue;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken,
            FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);

        if (GetLastError() != ERROR_SUCCESS) {
            rVal = FALSE;
        }
    }

    CloseHandle(hToken);

    return rVal;
}

#endif  // !KERNEL


/********************************************************************/
/*                                                                  */
/* Dll Version                                                      */
/*                                                                  */
/********************************************************************/

#ifdef KERNEL

#ifdef DEBUGVER
DEBUG_VERSION('D','M',"NT Kernel Debugger Monitor")
#else
RELEASE_VERSION('D','M',"NT Kernel Debugger Monitor")
#endif

#else // KERNEL

#ifdef DEBUGVER
DEBUG_VERSION('D','M',"WIN32 Debugger Monitor")
#else
RELEASE_VERSION('D','M',"WIN32 Debugger Monitor")
#endif

#endif  // KERNEL

DBGVERSIONCHECK()


int
WINAPI
DllMain(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
/*++

Routine Description:

    Entry point called by the loader during DLL initialization
    and deinitialization.  This creates and destroys some per-
    instance objects.

Arguments:

    hModule     - Supplies base address of dll

    dwReason    - Supplies flags describing why we are here

    dwReserved  - depends on dwReason.

Return Value:

    TRUE

--*/
{
    Unreferenced(hModule);
    Unreferenced(dwReserved);

#ifndef KERNEL
    InitializeListHead(&DmListOfPendingDebugEvents);
    InitializeListHead(&DmFreeListOfDebugEvents);
#endif // KERNEL

    switch (dwReason) {

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;

      case DLL_PROCESS_DETACH:

        CloseHandle(SpawnStruct.hEventApiDone);
        CloseHandle(hEventCreateProcess);
        CloseHandle(hEventRemoteQuit);
        CloseHandle(hEventNoDebuggee);
        CloseHandle(hEventContinue);

        DeleteCriticalSection(&csProcessDebugEvent);
        DeleteCriticalSection(&csThreadProcList);
        DeleteCriticalSection(&csEventList);
        DeleteCriticalSection(&csWalk);
#if !defined(KERNEL)
        CloseHandle(DebugActiveStruct.hEventApiDone);
        CloseHandle(DebugActiveStruct.hEventReady);
        DeleteCriticalSection(&csKillQueue);
#endif
        break;

      case DLL_PROCESS_ATTACH:

        InitializeCriticalSection(&csProcessDebugEvent);
        InitializeCriticalSection(&csThreadProcList);
        InitializeCriticalSection(&csEventList);
        InitializeCriticalSection(&csWalk);

        hEventCreateProcess = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventRemoteQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventContinue = CreateEvent(NULL, TRUE, FALSE, NULL);
        hEventNoDebuggee    = CreateEvent(NULL, TRUE, FALSE, NULL);
        SpawnStruct.hEventApiDone = CreateEvent(NULL, TRUE, FALSE, NULL);

#if defined(KERNEL)
        InitializeCriticalSection(&csApiInterlock);
        InitializeCriticalSection(&csSynchronizeTargetInterlock);
#endif  // KERNEL

#if !defined(KERNEL)
        InitializeCriticalSection(&csKillQueue);
        DebugActiveStruct.hEventApiDone = CreateEvent(NULL, TRUE, TRUE, NULL);
        DebugActiveStruct.hEventReady   = CreateEvent(NULL, TRUE, TRUE, NULL);

        /*
         * These parameters are from SCOTTLU
         */

        SetProcessShutdownParameters(0x3ff, 0);

        //
        // get helpful info
        //
        hInstance = hModule;

        GetSystemInfo(&SystemInfo);
        OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
        GetVersionEx(&OsVersionInfo);

#if defined (TARGET_i386)
        FChicago = ((GetVersion() & 0x80000000) && ((GetVersion() & 0xff)>3));
#endif // TARGET_i386

#endif // !KERNEL

        break;
    }

    return TRUE;
}

BOOL
PASCAL
IsChicago(
    VOID
    )
/*++

Routine Description:

    This routine allows the caller to determine if the DM is running on
    the Win95 family

Return Value:

    TRUE if running on Win95 and FALSE othewise.

--*/
{
    return FChicago;
}


BOOL
PASCAL
DmDllInit(
    LPDBF  lpb
    )

/*++

Routine Description:

    This routine allows the shell (debugger or remote stub)
    to provide a service callback vector to the DM.

Arguments:

    lpb - Supplies an array of functions for callbacks

Return Value:

    TRUE if successfully initialized and FALSE othewise.

--*/

{
    lpdbf = lpb;
    return TRUE;
}                                   /* DmDllInit() */



#ifdef KERNEL
void
ParseDmParams(
    LPTSTR p
    )
{
    DWORD                       i;
    CHAR                        szPath[MAX_PATH];
    CHAR                        szStr[_MAX_PATH];
    LPTSTR                      lpPathNext;
    LPTSTR                      lpsz1;
    LPTSTR                      lpsz2;
    LPTSTR                      lpsz3;


    for (i=0; i<MAX_MODULEALIAS; i++) {
        if (!ModuleAlias[i].Special) {
            ZeroMemory( &ModuleAlias[i], sizeof(MODULEALIAS) );
        }
    }

    do {
        p = _ftcstok( p, _T("=") );
        if (p) {
            for (i=0; i<MAXKDOPTIONS; i++) {
                if (_ftcsicmp(KdOptions[i].keyword,p)==0) {
                    break;
                }
            }
            if (i < MAXKDOPTIONS) {
                p = _ftcstok( NULL, _T(" ") );
                if (p) {
                    switch (KdOptions[i].typ) {
                        case KDT_DWORD:
                            KdOptions[i].value = atol( p );
                            break;

                        case KDT_STRING:
                            KdOptions[i].value = (UINT_PTR)_ftcsdup( p );
                            break;
                    }
                    p = p + (_ftcslen(p) + 1);
                }
            } else {
                if (_ftcsicmp( p, _T("alias") ) == 0) {
                    p = _ftcstok( NULL, _T("#") );
                    if (p) {
                        for (i=0; i<MAX_MODULEALIAS; i++) {
                            if (ModuleAlias[i].ModuleName[0] == 0) {
                                break;
                            }
                        }
                        if (i < MAX_MODULEALIAS) {
                            _ftcscpy( ModuleAlias[i].ModuleName, p );
                            p = _ftcstok( NULL, _T(" ") );
                            if (p) {
                                _ftcscpy( ModuleAlias[i].Alias, p );
                                p = p + (_ftcslen(p) + 1);
                            }
                        } else {
                            p = _ftcstok( NULL, _T(" ") );
                        }
                    }
                } else {
                    p = _ftcstok( NULL, _T(" ") );
                }
            }
        }
    } while(p && *p);

    if (KdOptions[KDO_VERBOSE].value > 1) {
        nVerbose = (BOOL)KdOptions[KDO_VERBOSE].value;
    }
    else {
        nVerbose = MIN_VERBOSITY_LEVEL;
    }

    szPath[0] = 0;
    lpPathNext = _ftcstok((LPTSTR)KdOptions[KDO_SYMBOLPATH].value, _T(";"));
    while (lpPathNext) {
        lpsz1 = szStr;
        while ((lpsz2 = _ftcschr(lpPathNext, _T('%'))) != NULL) {
            _ftcsncpy(lpsz1, lpPathNext, (size_t)(lpsz2 - lpPathNext));
            lpsz1 += lpsz2 - lpPathNext;
            lpsz2++;
            lpPathNext = _ftcschr(lpsz2, _T('%'));
            if (lpPathNext != NULL) {
                *lpPathNext++ = 0;
                lpsz3 = getenv(lpsz2);
                if (lpsz3 != NULL) {
                    _ftcscpy(lpsz1, lpsz3);
                    lpsz1 += _ftcslen(lpsz3);
                }
            } else {
                lpPathNext = _T("");
            }
        }
        _ftcscpy(lpsz1, lpPathNext);
        _ftcscat( szPath, szStr );
        _ftcscat( szPath, _T(";") );
        lpPathNext = _ftcstok(NULL, _T(";"));
    }

    if ( szPath[0] != 0 ) {
        if (szPath[_ftcslen(szPath)-1] == _T(';')) {
            szPath[_ftcslen(szPath)-1] = _T('\0');
        }
        _ftcscpy( (LPTSTR)KdOptions[KDO_SYMBOLPATH].value, szPath );
    }
}

#endif  // KERNEL

VOID
ProcessRemoteQuit(
    VOID
    )
{
    HPRCX      hprc;
    PBREAKPOINT pbp;
    PBREAKPOINT pbpT;


    EnterCriticalSection(&csThreadProcList);

    for (hprc=prcList->next; hprc; hprc=hprc->next) {
        for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
            pbpT = BPNextHprcPbp(hprc, pbp);
            RemoveBP(pbp);
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    fDisconnected = TRUE;
    ResetEvent( hEventRemoteQuit );
}




XOSD FAR PASCAL
DMInit(
    DMTLFUNCTYPE lpfnTl,
    LPTSTR        lpch
    )
/*++

Routine Description:

    This is the entry point called by the TL to initialize the
    connection from DM to TL.

Arguments:

    lpfnTl  - Supplies entry point to TL

    lpch    - Supplies command line arg list

Return Value:

    XOSD value: xosdNone for success, other values reflect reason
    for failure to initialize properly.

--*/
{
    int i, n;
    XOSD xosd;

    DEBUG_PRINT(_T("DMInit\r\n"));

    if (lpfnTl != NULL) {
        /*
         **  Parse out anything interesting from the command line args
         */

        while (*lpch) {

            while (isspace(*lpch)) {
                lpch++;
            }

            if (*lpch != _T('/') && *lpch != _T('-')) {
                break;
            }

            lpch++;

            switch (*lpch++) {

              case 0:   // don't skip over end of string
                --lpch;

              default:  // assert, continue is ok.
                assert(FALSE);
                break;


              case 'v':
              case 'V':

                while (isspace(*lpch)) {
                    lpch++;
                }
                nVerbose = atoi(lpch);
                while (isdigit(*lpch)) {
                    lpch++;
                }
                break;

              case 'r':
              case 'R':
                FDMRemote = TRUE;
                break;

              case 'd':
              case 'D':
                FUseOutputDebugString = TRUE;
                break;

            }
        }
#ifdef KERNEL
        ParseDmParams( lpch );
#endif


        /* Define a false single step event */
        falseSSEvent.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
        falseSSEvent.u.Exception.ExceptionRecord.ExceptionCode
          = EXCEPTION_SINGLE_STEP;

        falseBPEvent.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
        falseBPEvent.u.Exception.ExceptionRecord.ExceptionCode
          = EXCEPTION_BREAKPOINT;

        FuncExitEvent.dwDebugEventCode = FUNC_EXIT_EVENT;
        FuncExitEvent.u.Exception.ExceptionRecord.ExceptionCode
          = EXCEPTION_SINGLE_STEP;

        /* Define the standard notification method */
        EMNotifyMethod.notifyFunction = ConsumeThreadEventsAndNotifyEM;
        EMNotifyMethod.lparam     = 0;

        SearchPathString[0] = _T('\0');
        SearchPathSet       = FALSE;

        InitEventQueue();

        //
        // initialize data breakpoint handler
        //
        ExprBPInitialize();

        SetDebugErrorLevel(SLE_WARNING);

        /*
         **  Save the pointer to the Transport layer entry function
         */

        DmTlFunc = lpfnTl;

        /*
         **  Try and connect up to the other side of the link
         */

        DmTlFunc( tlfSetBuffer, NULL, sizeof(abEMReplyBuf), (LPARAM) abEMReplyBuf );

        if ((xosd = DmTlFunc( tlfConnect, NULL, 0, 0)) != xosdNone ) {
            return(xosd);
        }

        DPRINT(10, (_T("DM & TL are now connected\n")));

    } else {

        DmTlFunc( tlfDisconnect, NULL, 0, 0);
        DmTlFunc( tlfSetBuffer, NULL, 0, 0);
        FDMRemote = FALSE;
        DmTlFunc = (DMTLFUNCTYPE) NULL;

    }

    return xosdNone;
}                               /* DmInit() */



#ifndef KERNEL
VOID
Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup of DM, prepare for exit.

Arguments:

    None

Return Value:

    None

--*/
{
    HTHDX           pht, phtt;
    HPRCX           php, phpt;
    BREAKPOINT      *bp, *bpt;
    int             iDll;


    /* Free all threads and close their handles */
    for (pht = thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse; pht; pht = phtt) {
        phtt = pht->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
        if (pht->rwHand != (HANDLE)INVALID) {
            CloseHandle(pht->rwHand);
        }
        MHFree(pht);
    }
    thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse = NULL;


    /* Free all processes and close their handles */
    for(php = prcList->next; php; php = phpt) {
        phpt = php->next;

        RemoveExceptionList(php);
        // Free any fibers that may be left
        RemoveFiberList(php);

        for (iDll = 0; iDll < php->cDllList; iDll++) {
            DestroyDllLoadItem(&php->rgDllList[iDll]);
        }
        MHFree(php->rgDllList);

        if (php->rwHand != (HANDLE)INVALID) {
            CloseHandle(php->rwHand);
        }
        CloseHandle(php->hExitEvent);
        CloseHandle(php->hEventCreateThread);
        MHFree(php);
    }
    prcList->next = NULL;

    /* Free all breakpoints */
    for(bp = bpList->next; bp; bp = bpt) {
        bpt = bp->next;
        MHFree(bp);
    }
    bpList->next = NULL;


    // Ask the disassembler to clean itself up.
    CleanupDisassembler( );

    if (hDmPollThread) {
        fDmPollQuit = TRUE;
        WaitForSingleObject(hDmPollThread, INFINITE);
        hDmPollThread = 0;
    }

    while (!IsListEmpty(&DmListOfPendingDebugEvents)) {
        PLIST_ENTRY p = RemoveHeadList(&DmListOfPendingDebugEvents);
        MHFree(p);
    }

    while (!IsListEmpty(&DmFreeListOfDebugEvents)) {
        PLIST_ENTRY p = RemoveHeadList(&DmFreeListOfDebugEvents);
        MHFree(p);
    }
}
#endif  // !KERNEL



BOOL
WINAPIV
DMPrintShellMsg(
    LPTSTR szFormat,
    ...
    )
/*++

Routine Description:

   This function prints a string on the shell's
   command window.

Arguments:

    szFormat    - Supplies format string for sprintf

    ...         - Supplies variable argument list

Return Value:

    TRUE      -> all is ok and the string was printed
    FALSE     -> something's hosed and no string printed

--*/
{
    TCHAR     buf[512];
    DWORD    bufLen;
    va_list  marker;
    LPINFOAVAIL lpinf;
    LPRTP    lprtp = NULL;
    BOOL     rVal = TRUE;

    va_start( marker, szFormat );
    bufLen = _vsnprintf(buf, sizeof(buf), szFormat, marker );
    va_end( marker);

    if (bufLen == -1) {
        buf[sizeof(buf) - 1] = _T('\0');
    }

    __try {
        if (!DmTlFunc || (HPID)INVALID == hpidRoot) {
            // If we can't print it to the shell, hopefully to an
            // attached debugger.
            OutputDebugString(buf);
        } else {
            bufLen   = _ftcslen(buf) + 1;
            lprtp    = (LPRTP) MHAlloc( FIELD_OFFSET(RTP, rgbVar)+sizeof(INFOAVAIL)+bufLen );
            lpinf    = (LPINFOAVAIL)(lprtp->rgbVar);

            lprtp->dbc  = dbcInfoAvail;
            lprtp->hpid = hpidRoot;
            lprtp->htid = NULL;
            lprtp->cb   = (int)bufLen;

            lpinf->fReply    = FALSE;
            lpinf->fUniCode  = FALSE;
            memcpy( lpinf->buffer, buf, bufLen );

            DmTlFunc( tlfDebugPacket,
                      lprtp->hpid,
                      (FIELD_OFFSET(RTP, rgbVar)+sizeof(INFOAVAIL)+bufLen),
                      (LPARAM) lprtp
                    );

        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rVal = FALSE;

    }

    if (lprtp) {
       MHFree( lprtp );
    }

    return rVal;
}

VOID
WINAPIV
DebugPrint(
    LPTSTR szFormat,
    ...
    )
{
    TCHAR   rgchDebug[1024];
    va_list  marker;
    int n;

    va_start( marker, szFormat );
    n = _vsnprintf(rgchDebug, _tsizeof(rgchDebug), szFormat, marker );
    va_end( marker);

    if (n == -1) {
        rgchDebug[_tsizeof(rgchDebug)-1] = 0;
    }

    OutputDebugString( rgchDebug );
    return;
}                               /* DebugPrint() */


int
pCharMode(
    LPTSTR        szAppName,
    PIMAGETYPE    pImageType
    )
/*++

Routine Description:

    This routine is used to determine the type of exe which we are going
    to be debugging.  This is decided by looking for exe headers and making
    decisions based on the information in the exe headers.

Arguments:

    szAppName  - Supplies the path to the debugger exe

    pImageType - Returns the type of the image

Return Value:

    System_Invalid     - could not find the exe file
    System_GUI         - GUI application
    System_Console     - console application

--*/

{
    IMAGE_DOS_HEADER    dosHdr;
    IMAGE_OS2_HEADER    os2Hdr;
    IMAGE_NT_HEADERS    ntHdr;
    DWORD               cb;
    HANDLE              hFile;
    int                 ret;
    BOOL                GotIt;
    _tcscpy(nameBuffer, szAppName);

    // don't use OpenFile as it fails paths >127 bytes long

    hFile = CreateFile( szAppName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if (hFile == (HANDLE)-1) {

        /*
         *      Could not open file!
         */

        DEBUG_PRINT_2(_T("CreateFile(%s) --> %u\r\n"), szAppName, GetLastError());
        return System_Invalid;

    }

    /*
     *  Try and read an MZ Header.  If you can't then it can not possibly
     *  be a legal exe file.  (Not strictly true but we will ignore really
     *  short com files since they are unintersting).
     */

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if ((!ReadFile(hFile, &dosHdr, sizeof(dosHdr), &cb, NULL)) ||
        (cb != sizeof(dosHdr))) {

        if (_ftcsicmp(&szAppName[_ftcslen(szAppName) - 4], _T(".COM")) == 0) {
            *pImageType = Image_16;
        } else {
            DPRINT(1, (_T("dosHdr problem.\n")));
            *pImageType = Image_Unknown;
        }

        CloseHandle(hFile);
        return System_GUI;

    }

    /*
     *  Verify the MZ header.
     *
     *  NOTENOTE        Messup the case of no MZ header.
     */

    if (dosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
        /*
         *  We did not start with the MZ signature.  If the extension
         *      is .COM then it is a COM file.
         */

        if (_ftcsicmp(&szAppName[_ftcslen(szAppName) - 4], _T(".COM")) == 0) {
            *pImageType = Image_16;
        } else {
            DPRINT(1, (_T("MAGIC problem(MZ).\n")));
            *pImageType = Image_Unknown;
        }

        CloseHandle(hFile);
#ifndef KERNEL
        if (DmpInitialize( szAppName, &CrashContext, &CrashException, &CrashDumpHeader )) {
            //
            // Verify the architecture
            //
            if (
#if defined(TARGET_i386)
                CrashDumpHeader->MachineImageType != IMAGE_FILE_MACHINE_I386
#elif defined(TARGET_ALPHA)
                CrashDumpHeader->MachineImageType != IMAGE_FILE_MACHINE_ALPHA
#elif defined(TARGET_AXP64)
                CrashDumpHeader->MachineImageType != IMAGE_FILE_MACHINE_AXP64
#elif defined(TARGET_IA64)
                CrashDumpHeader->MachineImageType != IMAGE_FILE_MACHINE_IA64
#else
#error( "unknown target machine" );
#endif
                                    ) {
                return System_DmpWrongPlatform;
            }

            //
            // User mode validation
            //
            if (DmpIsItAUserModeFile()) {
                if (!DmpUserModeTestHeader()) {
                    return System_DmpInvalid;
                }
            }

            *pImageType = Image_Dump;
        }
#endif  // !KERNEL
        return System_Console;
    }

    if ( dosHdr.e_lfanew == 0 ) {
        /*
         *  Straight DOS exe.
         */

        DPRINT(1, (_T("[DOS image].\n")));
        *pImageType = Image_16;

        CloseHandle(hFile);
        return System_Console;
    }

    /*
     *  Now look at the next EXE header (either NE or PE)
     */

    SetFilePointer(hFile, dosHdr.e_lfanew, NULL, FILE_BEGIN);
    GotIt = FALSE;
    ret = System_GUI;

    /*
     *  See if this is a Win16 program
     */

    if (ReadFile(hFile, &os2Hdr, sizeof(os2Hdr), &cb, NULL)  &&
        (cb == sizeof(os2Hdr))) {

        if ( os2Hdr.ne_magic == IMAGE_OS2_SIGNATURE ) {
            /*
             *  Win16 program  (may be an OS/2 exe also)
             */

            DPRINT(1, (_T("[Win16 image].\n")));
            *pImageType = Image_16;
            GotIt  = TRUE;
        } else if ( os2Hdr.ne_magic == IMAGE_OS2_SIGNATURE_LE ) {
            /*
             *  OS2 program - Not supported
             */

            DPRINT(1, (_T("[OS/2 image].\n")));
            *pImageType = Image_Unknown;
            GotIt  = TRUE;
        }
    }

    /*
     *  If the above failed, see if it is an NT program
     */

    if ( !GotIt ) {
        SetFilePointer(hFile, dosHdr.e_lfanew, NULL, FILE_BEGIN);

        if (ReadFile(hFile, &ntHdr, sizeof(ntHdr), &cb, NULL) &&
            (cb == sizeof(ntHdr))                             &&
            (ntHdr.Signature == IMAGE_NT_SIGNATURE)) {
            /*
             *  All CUI (Character user interface) subsystems
             *  have the lowermost bit set.
             */

            DPRINT(1, ((ntHdr.OptionalHeader.Subsystem & 1) ?
                       _T("[*Character mode app*]\n") : _T("[*Windows mode app*]\n")));

            ret = ((ntHdr.OptionalHeader.Subsystem & 1)) ?
              System_Console : System_GUI;
            *pImageType = Image_32;
        } else {
            DWORD   FileSize;

            FileSize = SetFilePointer(hFile, 0, NULL, FILE_END);

            if ( (DWORD)dosHdr.e_lfanew > FileSize ) {
                //
                //  Bogus e_lfanew, assume DOS
                //
                DPRINT(1, (_T("[DOS image assumed].\n")));
                *pImageType = Image_16;
                ret =  System_Console;

            } else {

                //
                //  Not an NT image.
                //
                DPRINT(1, (_T("MAGIC problem(PE).\n")));
                *pImageType = Image_Unknown;
            }
        }
    }

    CloseHandle(hFile);
    return ret;
}                               /* pCharMode() */


VOID
ReConnectDebugger(
    DEBUG_EVENT64 *lpde,
    BOOL        fNoDllLoad
    )

/*++

Routine Description:

    This function handles the case where the dm/tl is re-connected to
    a debugger.  This function must re-instate the debugger to the
    correct state that existed before the disconnect action.

    (wesw) 11-3-93

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD            i;
    DEBUG_EVENT64    de;
    HPRCX            hprc;
    HTHDX            hthd;
    HTHDX            hthd_lb;
    DWORD            id;
    HANDLE           hThread;
    BOOL             fException = FALSE;


    //
    // the dm is now connected
    //
    fDisconnected = FALSE;

    //
    // check to see if a re-connection is occurring while the
    // process is running or after a non-servicable debug event
    //
    if (lpde) {
        hthd = HTHDXFromPIDTID((PID)lpde->dwProcessId,(TID)lpde->dwThreadId);
        if (hthd) {
            if (lpde->dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
                if (lpde->u.Exception.dwFirstChance) {
                    hthd->tstate |= ts_first;
                } else {
                    hthd->tstate |= ts_second;
                }
            }
            hthd->tstate &= ~ts_running;
            hthd->tstate |= ts_stopped;
        }
    }


    fUseRoot = TRUE;

    for (hprc = prcList->next; hprc; hprc = hprc->next) {

        //
        // generate a create process event
        //

        if (fUseRoot) {
            hprc->pstate |= ps_root;
            hprc->hpid   = hpidRoot;
            fUseRoot     = FALSE;
        } else {
            hprc->hpid = (HPID)INVALID;
        }

        assert( Is64PtrSE(hprc->rgDllList[0].offBaseOfImage) );

        hthd=hprc->hthdChild;
        ResetEvent(hEventCreateProcess);
        de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
        de.dwProcessId = hprc->pid;
        de.dwThreadId = hthd->tid;
        de.u.CreateProcessInfo.hFile = NULL;
        de.u.CreateProcessInfo.hProcess = hprc->rwHand;
        de.u.CreateProcessInfo.hThread = hthd->rwHand;
        de.u.CreateProcessInfo.lpBaseOfImage = hprc->rgDllList[0].offBaseOfImage;
        de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
        de.u.CreateProcessInfo.nDebugInfoSize = 0;
        de.u.CreateProcessInfo.lpStartAddress = PC(hthd);
        de.u.CreateProcessInfo.lpThreadLocalBase = 0;
        de.u.CreateProcessInfo.lpImageName = 0;
        de.u.CreateProcessInfo.fUnicode = 0;
        NotifyEM(&de, hthd, 0, (ULONG64)hprc);

        WaitForSingleObject(hEventCreateProcess, INFINITE);

        //
        // mark the process as 'being connected' so that the continue debug
        // events that are received from the shell are ignored
        //
        hprc->pstate |= ps_connect;


        //
        // look for a thread that is stopped and not dead
        //
        for (hthd=hprc->hthdChild,hthd_lb=NULL; hthd; hthd=hthd->nextSibling) {
            if ((!(hthd->tstate & ts_dead)) && (hthd->tstate & ts_stopped)) {
                hthd_lb = hthd;
                break;
            }
        }

        if (hthd_lb == NULL) {
            //
            // if we get here then there are no threads that are stopped
            // so we must look for the first alive thread
            //
            for (hthd=hprc->hthdChild,hthd_lb=NULL; hthd; hthd=hthd->nextSibling) {
                if (!(hthd->tstate & ts_dead)) {
                    hthd_lb = hthd;
                    break;
                }
            }
        }

        if (hthd_lb == NULL) {
            //
            // if this happens then we are really screwed.  there are no valid
            // threads to use, so lets bail out.
            //
            return;
        }

        if ((hthd_lb->tstate & ts_first) || (hthd_lb->tstate & ts_second)) {
            fException = TRUE;
        }

        //
        // generate mod loads for all the dlls for this process
        //
        // this MUST be done before the thread creates because the
        // current PC of each thread can be in any of the loaded
        // modules.
        //
        hthd = hthd_lb;
        if (!fNoDllLoad) {
            for (i=0; i<(DWORD)hprc->cDllList; i++) {
                if (hprc->rgDllList[i].fValidDll) {
                    LPBYTE lpbPacket;
                    WORD   cbPacket;

                    assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );
            
                    de.dwDebugEventCode        = LOAD_DLL_DEBUG_EVENT;
                    de.dwProcessId             = hprc->pid;
                    de.dwThreadId              = hthd->tid;
                    de.u.LoadDll.hFile         = NULL;
                    de.u.LoadDll.lpBaseOfDll   = hprc->rgDllList[i].offBaseOfImage;
                    de.u.LoadDll.lpImageName   = (ULONG64)hprc->rgDllList[i].szDllName;
                    de.u.LoadDll.fUnicode      = FALSE;

                    if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) || (cbPacket == 0)) {
                        NotifyEM(&de, hthd, cbPacket, (ULONG64)lpbPacket);
                    }
                }
            }
        }


        //
        // loop thru all the threads for this process and
        // generate a thread create event for each one
        //
        for (hthd=hprc->hthdChild; hthd; hthd=hthd->nextSibling) {
            if (!(hthd->tstate & ts_dead)) {
                if (fException && hthd_lb == hthd) {
                    //
                    // do this one last
                    //
                    continue;
                }

                //
                // generate a thread create event
                //
                ResetEvent( hprc->hEventCreateThread );
                ResetEvent( hEventContinue );
                de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
                de.dwProcessId = hprc->pid;
                de.dwThreadId = hthd->tid;
                NotifyEM( &de, hthd, 0, (ULONG64)hprc );

                WaitForSingleObject( hprc->hEventCreateThread, INFINITE );

                //
                // wait for the shell to continue the new thread
                //
                WaitForSingleObject( hEventContinue, INFINITE );
            }
        }



        hthd = hthd_lb;

        if (fException) {
            //
            // We saved this thread for last -
            // generate a thread create event
            //
            ResetEvent( hprc->hEventCreateThread );
            ResetEvent( hEventContinue );
            de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
            de.dwProcessId = hprc->pid;
            de.dwThreadId = hthd->tid;
            NotifyEM( &de, hthd, 0, (ULONG64)hprc );

            WaitForSingleObject( hprc->hEventCreateThread, INFINITE );

            //
            // wait for the shell to continue the new thread
            //
            WaitForSingleObject( hEventContinue, INFINITE );
        }



        if (hthd->tstate & ts_running) {

            //
            // we didn't have a stopped thread -
            //
            // this will create a thread in the debuggee that will
            // immediately stop at a breakpoint.  this will cause the
            // shell to think that we are processing a normal attach.
            //

            HMODULE hModule = GetModuleHandle("ntdll.dll");
            FARPROC ProcAddr = GetProcAddress(hModule, "DbgBreakPoint" );

            RegisterExpectedEvent( hprc,
                                   (HTHDX)NULL,
                                   BREAKPOINT_DEBUG_EVENT,
                                   NO_SUBCLASS,
                                   DONT_NOTIFY,
                                   ActionDebugActiveReady,
                                   FALSE,
                                   (ULONG64)hprc
                                   );

            hThread = CreateRemoteThread( (HANDLE) hprc->rwHand,
                                          NULL,
                                          4096,
                                          (LPTHREAD_START_ROUTINE) ProcAddr,
                                          0,
                                          0,
                                          &id
                                          );

        } else if (!lpde || lpde->dwThreadId != hthd->tid) {

            //
            // generate a load complete event
            //

            ResetEvent( hEventContinue );
            de.dwDebugEventCode = LOAD_COMPLETE_DEBUG_EVENT;
            de.dwProcessId = hprc->pid;
            de.dwThreadId = hthd->tid;
            NotifyEM( &de, hthd, 0, 0L);

            //
            // wait for the continue...
            //
            WaitForSingleObject( hEventContinue, INFINITE );


            if (hthd->tstate & ts_stopped) {
                //
                // deliver the exception.
                //
                de.dwProcessId                  = hprc->pid;
                de.dwThreadId                   = hthd->tid;
                if ((hthd->tstate & ts_first) || (hthd->tstate & ts_second)) {
                    de.dwDebugEventCode         = EXCEPTION_DEBUG_EVENT;
                } else {
                    de.dwDebugEventCode         = BREAKPOINT_DEBUG_EVENT;
                }
                de.u.Exception.dwFirstChance    = (hthd->tstate & ts_first) != 0;
                de.u.Exception.ExceptionRecord  = hthd->ExceptionRecord;
                NotifyEM(&de, hthd, 0, 0);
            }
        }

        //
        // reset the process state
        //

        hprc->pstate &= ~ps_connect;
    }

    return;
}

/*
 * HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
 *
 * In NT 3.x, it is impossible for a GUI app to spawn a console
 * app and have one or two of the console app's standard handles
 * redirected away from the console; you can only do it with
 * either none of the handles redirected, or all three of them.
 * See also the struct STARTUPINFO in the Win32 Programmer's Reference
 *
 * However, I learned by experimentation that it just so happens
 * that there are a few magic values I can pass in for stdin,
 * stdout, and stderr, which will have the desired effect of
 * leaving that handle attached to the console.  For example,
 * if I pass ((HANDLE)3) for stdin, stdin will stay attached
 * to the console.  stdout is ((HANDLE)7), and stderr is
 * ((HANDLE)11)
 *
 * In UNIX, you would probably use 0, 1, and 2.  Perhaps it is no
 * conicidence that for x={0, 1, 2}, the magic handles are (x<<2 | 3)
 *
 * Not necessary for NT 4.0, but it works there, too.
 *
 * HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
 */
HANDLE const rghMagic[3] = {(HANDLE)3, (HANDLE)7, (HANDLE)11};

/*
 * Close3Handles: helper function for I/O redirection
 */

void
Close3Handles(
    HANDLE *rgh
    )
{
    int iFile;

    for (iFile=0; iFile<3; ++iFile) {
        if (rgh[iFile] != 0 &&
             rgh[iFile] != INVALID_HANDLE_VALUE &&
             rgh[iFile] != rghMagic[iFile])
        {
             VERIFY(CloseHandle(rgh[iFile]));
             rgh[iFile] = 0;
        }
    }
}

XOSD
ProcessDebuggeeRedirection(
LPTSTR lszCommandLine,
STARTUPINFO FAR * psi
)
/*++

Routine Description:

    Parse command line redirection

Arguments:

    szCommandLine           A string representing the command line.
    psi                     Where we leave our mark


Return Value:

    xosdNone                        If no error
    xosdIORedirBadFile      If we can't open the file
    xosdIORedirSyntax       If the redir syntax is bad

Notes:

    The redirection-related text is removed from lszCommandLine
    Redirection is a hack in NT
    psi -> dwFlags += STARTF+USESTDHANDLES
    psi -> hStd{Input,Output,Error} are set

--*/
{

    LPTSTR          lszArg;
    LPTSTR          lszBegin;
    LPTSTR          lszEnd;

    BOOL            fInQuote = FALSE;
    BOOL            fAppend;

    // iFile is 0,1,2 for std{in,out,err}
    int             iFile, iFileFrom;
    CHAR            ch;

    lszArg = lszCommandLine;

    while (*lszArg) {
        // skip over quoted text on command line
        if (*lszArg == '"') {
            fInQuote = !fInQuote;
        }
        if (fInQuote) {
            // If in quoted text, increment lszArg
            lszArg = _ftcsinc( lszArg );
            continue;
        }

        lszBegin = lszArg;

        // recognize leading digit for "2>blah", "0<blah", etc.
        // Put it in iFile
        if (*lszArg >= '0' && *lszArg <= '2') {
            // iFile holds the file descriptor
            iFile = *lszArg - '0';
            lszArg = _ftcsinc( lszArg );
            if (*lszArg == '\0') {
                break;
            }
        } else {
            // For 'foo.exe > bar.txt' (no file number), we'll figure it out later.
            iFile = -1;
        }

        // If there is redirection going on, process it.
        if (*lszArg == '<' || *lszArg == '>') {
            psi -> dwFlags |= STARTF_USESTDHANDLES;

            // if there was no explicit leading digit, figure out the
            // implicit one: 0 for "<", 1 for ">" or ">>"
            if (iFile == -1) {
                if (*lszArg == '<') {
                    iFile = 0;
                } else {
                    iFile = 1;
                }
            } else if (iFile == 0) {
                if (*lszArg == '>') {
                    Close3Handles(rgh);
                    return xosdIORedirSyntax;
                }
            } else {
                if (*lszArg == '<') {
                    Close3Handles(rgh);
                    return xosdIORedirSyntax;
                }
            }

            if (lszArg[0] == '>' && lszArg[1] == '>') {
                fAppend = TRUE;
                lszArg = _ftcsinc( lszArg );
            } else {
                fAppend = FALSE;
            }
            lszArg = _ftcsinc( lszArg );

            // deal with "2>&1" and so on
            if (*lszArg == '&') {
                lszArg = _ftcsinc( lszArg );

                while (*lszArg == ' ' || *lszArg == '\t') {
                    lszArg = _ftcsinc( lszArg );
                }

                // error conditions:
                //              1<&x    where ix not in [012]
                //              1<&1
                //              2>>&1
                if (*lszArg < '0' || *lszArg > '2' ||
                        *lszArg - '0' == iFile || fAppend) {
                    Close3Handles(rgh);
                    return xosdIORedirSyntax;
                }

                iFileFrom = *lszArg - '0';

                if (rgh[iFileFrom] == 0 ||
                        rgh[iFileFrom] == INVALID_HANDLE_VALUE) {
                    rgh[iFile] = rgh[iFileFrom];
                } else {
                    HANDLE hProcess = GetCurrentProcess();

                    if (!DuplicateHandle(
                                    hProcess, rgh[iFileFrom],
                                    hProcess, &rgh[iFile],
                                    0, TRUE, DUPLICATE_SAME_ACCESS))
                    {
                        Close3Handles(rgh);
                        return xosdIORedirBadFile;
                    }
                }
                lszArg = _ftcsinc( lszArg );    // get past last digit
            } else {
                static char rgchEndFilename[] = "\t \"&,;<=>";
                static SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, TRUE};

                // skip blanks after "<" or ">"
                while (*lszArg == ' ' || *lszArg == '\t') {
                    ++lszArg;
                }

                // append null to szArg
                lszEnd = lszArg;
                while (*lszEnd && !_tcschr(rgchEndFilename, *lszEnd)) {
                    lszEnd = _ftcsinc( lszEnd );
                }
                ch = *lszEnd;
                *lszEnd = '\0';

                if (iFile) {
                    // std{out,err}
                    rgh[iFile] = CreateFile (
                                        lszArg,
                                        GENERIC_READ|GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        &sa,
                                        fAppend ? OPEN_EXISTING : CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        0);
                } else {
                    // stdin
                    rgh[iFile] = CreateFile (
                                        lszArg,
                                        GENERIC_READ,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                                        &sa,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        0);
                }

                if (rgh[iFile] == INVALID_HANDLE_VALUE) {
                    //  // The file could not be opened as desired.
                    //  _ftcsncpy(szErrArg, lszArg, sizeof(szErrArg));
                    //  szErrArg[sizeof(szErrArg)-1] = '\0';
                    //  _ftcsncpy(szErrExe, lszExe, sizeof(szErrExe));
                    //  szErrExe[sizeof(szErrExe)-1] = '\0';

                    // restore byte after arg
                    *lszEnd = ch;

                    Close3Handles(rgh);
                    return xosdIORedirBadFile;
                }

                // restore byte after arg
                *lszEnd = ch;

                // if ">>", move to end of file
                if (fAppend) {
                    SetFilePointer(rgh[iFile], 0, NULL, FILE_END);
                }

                // advance lszArg to end of string
                lszArg = lszEnd;
            }

            // remove the redirection from the command line
            if (*lszArg == ' ' || *lszArg == '\t') {
                lszArg++;
            }
            _fmemmove(lszBegin, lszArg,
                        (_ftcslen(lszArg)+1) * sizeof(TCHAR));
            lszArg = lszBegin;
        } else {
            lszArg = _ftcsinc( lszArg );
        }

    } // while

    if (lszCommandLine[0] == ' ' && lszCommandLine[1] == '\0') {
        lszCommandLine[0] = '\0';
    }


    // If we're redirecting at all
    if (psi -> dwFlags & STARTF_USESTDHANDLES) {

        OSVERSIONINFO ver;


        ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

        GetVersionEx (&ver);

        for (iFile=0; iFile<3; ++iFile) {

            // If they're still unset.

            if (rgh[iFile] == 0) {
                // If we are using NT 3.x or greater,
                // use the hack magic handles. See comments
                // near the definition of rghMagic

                // This is a big-time hack.  Luckily, as of NT 4.0 it is no longer necessary.

                if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion == 3) {
                    rgh[iFile] = rghMagic[iFile];
                } else {
                    rgh[iFile] = INVALID_HANDLE_VALUE;
                }
            }
        }

        psi -> hStdInput  = rgh[0];
        psi -> hStdOutput = rgh[1];
        psi -> hStdError  = rgh[2];
    }

    return xosdNone;
} //ProcessDebuggeeRedirection
#ifndef KERNEL
void
ClearPendingDebugEvents(
    PID  pid,
    TID  tid
    )
/*++

Routine Description:

    Removes any pending debug events form the list.

    The process and thread id pair are used as the
    search criteria.

Arguments:

    pid - process id

    tid - thread id

Return Value:

    none

--*/
{
    PDETOSAVE pDeToSave;

    if (IsListEmpty(&DmListOfPendingDebugEvents)) {
        return;
    }


    pDeToSave = (PDETOSAVE) DmListOfPendingDebugEvents.Flink;

    while (&pDeToSave->List != &DmListOfPendingDebugEvents) {
        PDETOSAVE pNext = (PDETOSAVE) pDeToSave->List.Flink;

        if (pid == pDeToSave->de.dwProcessId
            && tid == pDeToSave->de.dwThreadId) {

            RemoveEntryList(&pDeToSave->List);
            InsertTailList(&DmFreeListOfDebugEvents, &pDeToSave->List);
        }
        pDeToSave = pNext;
    }
}


PDETOSAVE
GetNextFreeDebugEvent(
    VOID
    )
/*++

Routine Description:

    Returns a PDETOSAVE structure from the free list. If
    the free list is empty a new structure is allocated.

Arguments:

    none

Return Value:

   Pointer to an unused PDETOSAVE structure.

--*/
{
    PDETOSAVE pDeToSave;

    if (!IsListEmpty(&DmFreeListOfDebugEvents)) {
        pDeToSave = (PDETOSAVE) RemoveHeadList(&DmFreeListOfDebugEvents);
        memset(pDeToSave, 0, sizeof(*pDeToSave));
    } else {
        pDeToSave = (PDETOSAVE) MHAlloc(sizeof(DETOSAVE));
        if (pDeToSave) {
            memset(pDeToSave, 0, sizeof(*pDeToSave));
        }
    }

    return pDeToSave;
}

PDETOSAVE
GetMostRecentDebugEvent(
    PID  pid,
    TID  tid
    )
/*++

Routine Description:

    Return the most recent DEBUGEVENT for a given
    process/thread pair.

Arguments:

    pid - process id

    tid - thread id


Return Value:

    Pointer to the most recent DEBUGEVENT for the
    given process/thread pair.

    NULL if the list is empty.

--*/
{
    PDETOSAVE pDeToSave;

    if (IsListEmpty(&DmListOfPendingDebugEvents)) {
        return NULL;
    }

    pDeToSave = (PDETOSAVE) DmListOfPendingDebugEvents.Blink;

    while (&pDeToSave->List != &DmListOfPendingDebugEvents) {
        if (pid == pDeToSave->de.dwProcessId
            && tid == pDeToSave->de.dwThreadId) {

            return pDeToSave;
        }
        pDeToSave = (PDETOSAVE) pDeToSave->List.Blink;
    }

    return NULL;
}


#endif // !KERNEL

