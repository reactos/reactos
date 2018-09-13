/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOW32.C
 *  WOW32 16-bit API support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Multi-Tasking 23-May-1991 Matt Felton [mattfe]
 *  WOW as DLL 06-Dec-1991 Sudeep Bharati (sudeepb)
 *  Cleanup and rework multi tasking feb 6 (mattfe)
 *  added notification thread for task creation mar-11 (mattfe)
 *  added basic exception handling for retail build apr-3 92 mattfe
 *  use host_ExitThread apr-17 92 daveh
 *  Hung App Support june-22 82 mattfe
--*/

#include "precomp.h"
#pragma hdrstop
#include "wktbl.h"
#include "wutbl.h"
#include "wgtbl.h"
#include "wstbl.h"
#include "wkbtbl.h"
#include "wshltbl.h"
#include "wmmtbl.h"
#include "wsocktbl.h"
#include "wthtbl.h"
#include "wowit.h"
#include <stdarg.h>
#include <ntcsrdll.h>
#define SHAREWOW_MAIN
#include <sharewow.h>

#include <tsappcmp.h>


/* Function Prototypes */
DWORD   W32SysErrorBoxThread2(PTDB pTDB);
VOID    StartDebuggerForWow(VOID);
BOOLEAN LoadCriticalStringResources(void);

extern DECLSPEC_IMPORT ULONG *ExpLdt;
#define LDT_DESC_PRESENT 0x8000
#define STD_SELECTOR_BITS 0x7

MODNAME(wow32.c);

#define REGISTRY_BUFFER_SIZE 512

// for logging iloglevel to a file
#ifdef DEBUG
CHAR    szLogFile[128];
int     fLog;
HANDLE  hfLog;
UCHAR   gszAssert[256];
#endif

/*  iloglevel = 16 MAX the world (all 16 bit kernel internal calls
 *  iloglevel = 14 All internal WOW kernel Calls
 *  ilogeveel = 12 All USER GDI call + return Codes
 *  iloglevel = 5  Returns From Calls
 *  iloglevel = 3  Calling Parameters
 */
INT     flOptions;           // command line optin
#ifdef DEBUG
INT     iLogLevel;           // logging level;  0 implies none
INT     fDebugWait=0;        // Single Step, 0 = No single step
#endif

HANDLE  hmodWOW32;
HANDLE  hHostInstance;
#ifdef DEBUG
INT     fLogFilter = -1;            // Logging Code Fiters
WORD    fLogTaskFilter = (WORD)-1;  // Filter Logging for Specific TaskID
#endif
#ifdef i386
PX86CONTEXT pIntelRegisters; // x86 Only - Pointer to Intel Register Block
#endif

#ifdef DEBUG
BOOL    fSkipLog;           // TRUE to temporarily skip certain logging
INT     iReqLogLevel;                       // Current Output LogLevel
INT     iCircBuffer = CIRC_BUFFERS-1;           // Current Buffer
CHAR    achTmp[CIRC_BUFFERS][TMP_LINE_LEN] = {" "};      // Circular Buffer
CHAR    *pachTmp = &achTmp[0][0];
WORD    awfLogFunctionFilter[FILTER_FUNCTION_MAX] = {0xffff,0,0,0,0,0,0,0,0,0}; // Specific Filter API Array
PWORD   pawfLogFunctionFilter = awfLogFunctionFilter;
INT     iLogFuncFiltIndex;                     // Index Into Specific Array for Debugger Extensions
#endif

#ifdef DEBUG_MEMLEAK
CRITICAL_SECTION csMemLeak;
#endif

UINT    iW32ExecTaskId = (UINT)-1;    // Base Task ID of Task Being Exec'd
UINT    nWOWTasks;              // # of WOW tasks running
BOOL    fBoot = TRUE;           // TRUE During the Boot Process
HANDLE  ghevWaitCreatorThread = (HANDLE)-1; // Used to Syncronize creation of a new thread


BOOL    fWowMode;   // Flag used to determine wow mode.
                // currently defaults to FALSE (real mode wow)
                // This is used by the memory access macros
                // to properly form linear addresses.
                // When running on an x86 box, it will be
                // initialized to the mode the first wow
                // bop call is made in.  This flag can go
                // away when we no longer want to run real
                // mode wow.  (Daveh 7/25/91)

HANDLE hSharedTaskMemory;
DWORD  dwSharedProcessOffset;
HANDLE hWOWHeap;
HANDLE ghProcess;       // WOW Process Handle
PFNWOWHANDLERSOUT pfnOut;
PTD *  pptdWOA;
PTD    gptdShell;
DWORD  fThunkStrRtns;           // used as a BOOL
BOOL   gfDebugExceptions;  // set to 1 in debugger to
                           // enable debugging of W32Exception
BOOL   gfIgnoreInputAssertGiven;
DWORD  dwSharedWowTimeout;

WORD   gwKrnl386CodeSeg1;  // code segs of krnl386.exe
WORD   gwKrnl386CodeSeg2;
WORD   gwKrnl386CodeSeg3;
WORD   gwKrnl386DataSeg1;

#ifndef _X86_
PUCHAR IntelMemoryBase;  // Start of emulated CPU's memory
#endif


DWORD   gpsi = 0;
DWORD gpfn16GetProcModule;

/* for WinFax Lite install hack -- see wow32fax.c */
char szWINFAX[] =  "WINFAX";
char szModem[] =   "modem";
char szINSTALL[] = "INSTALL";
char szWINFAXCOMx[80];
BOOL gbWinFaxHack = FALSE;

#define TOOLONGLIMIT     _MAX_PATH
#define WARNINGMSGLENGTH 255

PSZ aszCriticalStrings[CRITICAL_STRING_COUNT];

char szEmbedding[] =        "embedding";
char szDevices[] =          "devices";
char szBoot[] =             "boot";
char szShell[] =            "shell";
char szServerKey[] =        "protocol\\StdFileEditing\\server";
char szPicture[] =          "picture";
char szPostscript[] =       "postscript";
char szZapfDingbats[] =     "ZAPFDINGBATS";
char szZapf_Dingbats[] =    "ZAPF DINGBATS";
char szSymbol[] =           "SYMBOL";
char szTmsRmn[] =           "TMS RMN";
char szHelv[] =             "HELV";
char szMavisCourier[]=      "MAVIS BEACON COURIER FP";
char szWinDotIni[] =        "win.ini";
char szSystemDotIni[] =     "system.ini";
char szExplorerDotExe[] =   "Explorer.exe";
char szDrWtsn32[] =         "drwtsn32";
PSTR pszWinIniFullPath;
PSTR pszWindowsDirectory;
PSTR pszSystemDirectory;
BOOL gbDBCSEnable = FALSE;
#ifdef FE_SB
char szSystemMincho[] = {(char) 0xbc, (char) 0xbd, (char) 0xc3, (char) 0xd1,
                         (char) 0x96, (char) 0xbe, (char) 0x92, (char) 0xa9,
                         (char) 0 };
char szMsMincho[] = { (char) 0x82, (char) 0x6c, (char) 0x82, (char) 0x72,
                      (char) 0x20, (char) 0x96, (char) 0xbe, (char) 0x92,
                      (char) 0xa9, (char) 0};
#endif

extern BOOL GdiReserveHandles(VOID);
extern CRITICAL_SECTION VdmLoadCritSec;
extern LIST_ENTRY TimerList;

extern PVOID GdiQueryTable();
extern PVOID gpGdiHandleInfo;

#if defined (_X86_)

extern PVOID WowpLockPrefixTable;

IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // Reserved
    0,                           // GlobalFlagsClear
    0,                           // GlobalFlagsSet
    0,                           // CriticalSectionTimeout (milliseconds)
    0,                           // DeCommitFreeBlockThreshold
    0,                           // DeCommitTotalFreeThreshold
    (LONG) &WowpLockPrefixTable, // LockPrefixTable, defined in FASTWOW.ASM
    0, 0, 0, 0, 0, 0, 0          // Reserved
};

#endif

BOOLEAN
W32DllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:  DllMain function called during ntvdm's
                      LoadLibrary("wow32")

Arguments:

    DllHandle - set global hmodWOW32

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(Context);

    hmodWOW32 = DllHandle;

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        if (!CreateSmallHeap()) {
            return FALSE;
        }

        if ((hWOWHeap = HeapCreate (0,
                    INITIAL_WOW_HEAP_SIZE,
                    GROW_HEAP_AS_NEEDED)) == NULL)
            return FALSE;

        // initialize hook stubs data.

        W32InitHookState(hmodWOW32);

        // initialize the thunk table offsets.  do it here so the debug process
        // gets them.

        InitThunkTableOffsets();

        //
        // initialization for named pipe handling in file thunks
        //

        InitializeCriticalSection(&VdmLoadCritSec);

        //
        // Load Critical Error Strings
        //

        if (!LoadCriticalStringResources()) {
            MessageBox(NULL, "The Win16 subsystem could not load critical string resources from wow32.dll, terminating.",
                       "Win16 subsystem load failure", MB_ICONEXCLAMATION | MB_OK);
            return FALSE;
        }

        //
        // setup the GDI table for handle conversion
        //

        gpGdiHandleInfo = GdiQueryTable();

        W32EWExecer();

        InitializeListHead(&TimerList);

        if (IsTerminalServer()) {
            //
            // Load tsappcmp.dll
            //
            HANDLE dllHandle = LoadLibrary (TEXT("tsappcmp.dll"));

            if (dllHandle) {


                gpfnTermsrvCORIniFile = (PTERMSRVCORINIFILE) GetProcAddress(
                                                                dllHandle,
                                                                "TermsrvCORIniFile"
                                                                );
                ASSERT(gpfnTermsrvCORIniFile != NULL);
            }

        }
        break;

    case DLL_THREAD_ATTACH:
        IsDebuggerAttached();   // Yes, this routine has side-effects.
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        /*
         * Tell base he can nolonger callback to us.
         */
        RegisterWowBaseHandlers(NULL);

        HeapDestroy (hWOWHeap);
        break;

    default:
        break;
    }

    return TRUE;
}


BOOLEAN
LoadCriticalStringResources(
    void
    )

/*++

Routine Description:  Loads strings we want around even if we can't allocate
                      memory.  Called during wow32 DLL load.

Arguments:

    none

Return Value:

    TRUE if all strings loaded and aszCriticalStrings initialized.

--*/

{
    int i, n;
    PSZ psz, pszStringBuffer;
    DWORD cbTotal;
    DWORD cbUsed;
    DWORD cbStrLen;
    DWORD rgdwStringOffset[CRITICAL_STRING_COUNT];

    //
    // Allocate too much memory for strings (maximum possible) at first,
    // reallocate to the real size when we're done loading strings.
    //

    cbTotal = CRITICAL_STRING_COUNT * CCH_MAX_STRING_RESOURCE;

    psz = pszStringBuffer = malloc_w(cbTotal);

    if ( ! psz ) {
        return FALSE;
    }

    cbUsed = 0;

    for ( n = 0; n < CRITICAL_STRING_COUNT; n++ ) {

        //
        // LoadString return value doesn't count null terminator.
        //

        cbStrLen = LoadString(hmodWOW32, n, psz, CCH_MAX_STRING_RESOURCE);

        if ( ! cbStrLen ) {
            return FALSE;
        }

        rgdwStringOffset[n] = cbUsed;

        psz    += cbStrLen + 1;
        cbUsed += cbStrLen + 1;

    }

    // Now, alloc a smaller buffer of the correct size
    // Note: HeapRealloc(IN_PLACE) won't work because allocations are 
    //       page-sorted by size -- meaning that changing the size will cause
    //       the memory to move to a new page.
    psz = malloc_w(cbUsed);

    // copy the strings into the smaller buffer
    // if we can't alloc the smaller buffer, just go with the big one
    if (psz) {
       RtlCopyMemory(psz, pszStringBuffer, cbUsed);
       free_w(pszStringBuffer);
       pszStringBuffer = psz;
    }

    // save the offsets in the critical string array
    for (i = 0; i < n; i++) {
       aszCriticalStrings[i] = pszStringBuffer + rgdwStringOffset[i];
    }

    return TRUE;
}


//***************************************************************************
// Continues ExitWindowsExec api call after logoff and subsequent logon
// Uses Events to synchronize across all wow vdms
//
//***************************************************************************

BOOL W32EWExecer(VOID)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;
    BYTE abT[REGISTRY_BUFFER_SIZE];

    if (W32EWExecData(EWEXEC_QUERY, (LPSTR)abT, sizeof(abT))) {
        HANDLE hevT;
        if (hevT = CreateEvent(NULL, TRUE, FALSE, WOWSZ_EWEXECEVENT)) {
            if (GetLastError() == 0) {
                W32EWExecData(EWEXEC_DEL, (LPSTR)NULL, 0);

                LOGDEBUG(0, ("WOW:Execing dos app -  %s\r\n", abT));
                RtlZeroMemory((PVOID)&StartupInfo, (DWORD)sizeof(StartupInfo));
                StartupInfo.cb = sizeof(StartupInfo);
                StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow = SW_NORMAL;

                CreateProcessStatus = CreateProcess(
                                        NULL,
                                        abT,
                                        NULL,               // security
                                        NULL,               // security
                                        FALSE,              // inherit handles
                                        CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                                        NULL,               // environment strings
                                        NULL,               // current directory
                                        &StartupInfo,
                                        &ProcessInformation
                                        );

                if (CreateProcessStatus) {
                    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
                    CloseHandle( ProcessInformation.hProcess );
                    CloseHandle( ProcessInformation.hThread );
                }

                SetEvent(hevT);
            }
            else {
                WaitForSingleObject(hevT, INFINITE);
            }

            CloseHandle(hevT);
        }
    }
    return 0;
}

//***************************************************************************
// W32EWExecData -
//   sets/resets the 'commandline', ie input to ExitWindowssExec api in the
//   registry - 'WOW' key 'EWExec' value
//
//***************************************************************************

BOOL W32EWExecData(DWORD fnid, LPSTR lpData, DWORD cb)
{
    BOOL bRet = FALSE;
    BYTE abT[REGISTRY_BUFFER_SIZE];


    switch (fnid) {
        case EWEXEC_SET:
            bRet = WriteProfileString(WOWSZ_EWEXECVALUE,
                                         WOWSZ_EWEXECVALUE,
                                           lpData);
            break;

        case EWEXEC_DEL:
            bRet = WriteProfileString(WOWSZ_EWEXECVALUE,
                                          NULL, NULL);
            break;

        case EWEXEC_QUERY:
            if (bRet = GetProfileString(WOWSZ_EWEXECVALUE,
                                           WOWSZ_EWEXECVALUE,
                                             "", abT, sizeof(abT))) {
                strcpy(lpData, abT);
            }

            break;

        default:
            WOW32ASSERT(FALSE);
            break;
    }
    return !!bRet;
}



/* W32Init - Initialize WOW support
 *
 * ENTRY
 *
 * EXIT
 *  TRUE if successful, FALSE if not
 */


BOOL W32Init(VOID)
{
    HKEY  WowKey;
#ifdef DEBUG
    CHAR WOWCmdLine[REGISTRY_BUFFER_SIZE];
    PCHAR pWOWCmdLine;
    ULONG WOWCmdLineSize = REGISTRY_BUFFER_SIZE;
#endif
    DWORD cb;
    DWORD dwType;
    PTD ptd;
    PFNWOWHANDLERSIN pfnIn;
    LPVOID lpSharedTaskMemory;
    LANGID LangID;
#ifdef _X86_
    pIntelRegisters = getIntelRegistersPointer();  // X86 Only, get pointer to Register Context Block
#endif                                             // UP.

#ifndef _X86_
    //
    // This is the one and only call to Sim32GetVDMPointer in WOW32.
    // All other cases should use WOWGetVDMPointer.  This one is necessary
    // to set up the base memory address used by GetRModeVDMPointerMacro.
    // (There's also a call in GetPModeVDMPointerAssert, but that's in
    // the checked build only and only as a fallback mechanism.)
    //

    IntelMemoryBase = Sim32GetVDMPointer(0,0,0);
#endif

    fWowMode = ((getMSW() & MSW_PE) ? TRUE : FALSE);

    // Boost the HourGlass

    ShowStartGlass(10000);

    LangID = GetSystemDefaultLangID();
    if (PRIMARYLANGID(LangID) == LANG_JAPANESE ||
        PRIMARYLANGID(LangID) == LANG_KOREAN   ||
        PRIMARYLANGID(LangID) == LANG_CHINESE    ) {
        gbDBCSEnable = TRUE;
    }

    //
    // Set up a global WindowsDirectory to be used by other WOW functions.
    //

    {
        char szBuf[ MAX_PATH ];
        int cb;

        GetSystemDirectory(szBuf, sizeof szBuf);
        GetShortPathName(szBuf, szBuf, sizeof szBuf);
        cb = strlen(szBuf) + 1;
        pszSystemDirectory = malloc_w_or_die(cb);
        RtlCopyMemory(pszSystemDirectory, szBuf, cb);

        GetWindowsDirectory(szBuf, sizeof szBuf);
        GetShortPathName(szBuf, szBuf, sizeof szBuf);
        cb = strlen(szBuf) + 1;
        pszWindowsDirectory = malloc_w_or_die(cb);
        RtlCopyMemory(pszWindowsDirectory, szBuf, cb);

        pszWinIniFullPath = malloc_w_or_die(cb + 8);   // "\win.ini"
        RtlCopyMemory(pszWinIniFullPath, szBuf, cb);
        pszWinIniFullPath[ cb - 1 ] = '\\';
        RtlCopyMemory(pszWinIniFullPath + cb, szWinDotIni, 8);
    }

    // Give USER32 our entry points

    RtlZeroMemory(&pfnIn, sizeof(pfnIn));

    pfnIn.pfnLocalAlloc = W32LocalAlloc;
    pfnIn.pfnLocalReAlloc = W32LocalReAlloc;
    pfnIn.pfnLocalLock = W32LocalLock;
    pfnIn.pfnLocalUnlock = W32LocalUnlock;
    pfnIn.pfnLocalSize = W32LocalSize;
    pfnIn.pfnLocalFree = W32LocalFree;
    pfnIn.pfnGetExpWinVer = W32GetExpWinVer;
    pfnIn.pfn16GlobalAlloc = W32GlobalAlloc16;
    pfnIn.pfn16GlobalFree = W32GlobalFree16;
    pfnIn.pfnEmptyCB = W32EmptyClipboard;
    pfnIn.pfnFindResourceEx = W32FindResource;
    pfnIn.pfnLoadResource = W32LoadResource;
    pfnIn.pfnFreeResource = W32FreeResource;
    pfnIn.pfnLockResource = W32LockResource;
    pfnIn.pfnUnlockResource = W32UnlockResource;
    pfnIn.pfnSizeofResource = W32SizeofResource;
    pfnIn.pfnWowWndProcEx = (PFNWOWWNDPROCEX)W32Win16WndProcEx;
    pfnIn.pfnWowDlgProcEx = (PFNWOWDLGPROCEX)W32Win16DlgProcEx;
    pfnIn.pfnWowEditNextWord = W32EditNextWord;
    pfnIn.pfnWowCBStoreHandle = WU32ICBStoreHandle;
    pfnIn.pfnGetProcModule16 = WOWGetProcModule16;
    pfnIn.pfnWowMsgBoxIndirectCallback = WowMsgBoxIndirectCallback;
    pfnIn.pfnWowIlstrsmp = WOWlstrcmp16;
    pfnIn.pfnWOWTellWOWThehDlg = WOWTellWOWThehDlg;

    gpsi = UserRegisterWowHandlers(&pfnIn, &pfnOut);

    RegisterWowBaseHandlers(W32DDEFreeGlobalMem32);

    // Prepare us to be in the shared memory process list

    lpSharedTaskMemory = LOCKSHAREWOW();

    WOW32ASSERTMSG(lpSharedTaskMemory, "WOW32: Could not access shared memory object\n");

    if ( lpSharedTaskMemory ) {
        UNLOCKSHAREWOW();
    }

    CleanseSharedList();
    AddProcessSharedList();

    // Allocate a Temporary TD for the first thread

    ptd = CURRENTPTD() = malloc_w_or_die(sizeof(TD));

    RtlZeroMemory(ptd, sizeof(*ptd));

    InitializeCriticalSection(&ptd->csTD);

    // Create Global Wait Event - Used During Task Creation To Syncronize with New Thread

    if (!(ghevWaitCreatorThread = CreateEvent(NULL, FALSE, FALSE, NULL))) {
        LOGDEBUG(0,("    W32INIT ERROR: event creation failure\n"));
        return FALSE;
    }


    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
               "SYSTEM\\CurrentControlSet\\Control\\WOW",
               0,
               KEY_QUERY_VALUE,
               &WowKey
             ) != 0){
        LOGDEBUG(0,("    W32INIT ERROR: Registry Opening failed\n"));
        return FALSE;
    }

    //
    // If present, read the SharedWowTimeout value and convert
    // from seconds to milliseconds, which is what SetTimer
    // uses.  Maximum interval for SetTimer is 0x7fffffff.
    // No need to enforce a minimum, as SetTimer treats a
    // zero timeout as a one millsecond timeout.
    //

    cb = sizeof(dwSharedWowTimeout);
    if ( ! RegQueryValueEx(WowKey,
              "SharedWowTimeout",
              NULL,
              &dwType,
              (LPBYTE) &dwSharedWowTimeout,
              &cb) && REG_DWORD == dwType) {

        //
        // Prevent overflow in the conversion to millseconds below.
        // This caps the timeout to 2,147,483 seconds, or 24.8 days.
        //

        dwSharedWowTimeout = min( dwSharedWowTimeout,
                                  (0x7fffffff / 1000) );

    } else {

        //
        // Didn't find SharedWowTimeout value or it's the wrong type.
        //

        dwSharedWowTimeout = 1 * 60 * 60;  // 1 hour in seconds
    }

    dwSharedWowTimeout *= 1000;


    //
    // If present (it usually isn't) read ThunkNLS value entry.
    //

    cb = sizeof(fThunkStrRtns);
    if (RegQueryValueEx(WowKey,
            "ThunkNLS",
            NULL,
            &dwType,
            (LPBYTE) &fThunkStrRtns,
            &cb) || dwType != REG_DWORD) {

        //
        // Didn't find the registry value or it's the wrong type,
        // so we use the default behavior which is to thunk outside the
        // US.
        //

        fThunkStrRtns = GetSystemDefaultLCID() !=
                            MAKELCID(
                                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                SORT_DEFAULT
                                );
    } else {

        //
        // We did find a ThunkNLS value in the registry, warn on debug builds
        // to save testers and developers who turn it on for one bug but forget
        // to turn it back off.
        //

#ifdef DEBUG
        OutputDebugString("WOW Warning:  ThunkNLS registry value overriding default NLS tranlation.\n");
#endif

    }

#ifdef DEBUG

    if (RegQueryValueEx (WowKey,
             "wowcmdline",
             NULL,
             &dwType,
             (LPBYTE)&WOWCmdLine,
             &WOWCmdLineSize) != 0){
        RegCloseKey (WowKey);
        LOGDEBUG(0,("    W32INIT ERROR: WOWCMDLINE not found in registry\n"));
        return FALSE;
    }

    pWOWCmdLine = (PCHAR)((PBYTE)WOWCmdLine + WOWCmdLineSize + 1);

    WOWCmdLineSize = REGISTRY_BUFFER_SIZE - WOWCmdLineSize -1;

    if (WOWCmdLineSize < (REGISTRY_BUFFER_SIZE  / 2)){
        LOGDEBUG(0,("    W32INIT ERROR: Registry Buffer too small\n"));
        return FALSE;
    }

    if (ExpandEnvironmentStrings ((LPCSTR)WOWCmdLine, (LPSTR)pWOWCmdLine, WOWCmdLineSize) >
            WOWCmdLineSize) {
        LOGDEBUG(0,("    W32INIT ERROR: Registry Buffer too small\n"));
        return FALSE;
    }

    // Find Debug Info
    while (*pWOWCmdLine) {
        if (*pWOWCmdLine == '-' || *pWOWCmdLine == '/') {
         //   c = (char)tolower(*++pWOWCmdLine);
            switch(*++pWOWCmdLine) {
            case 'd':
            case 'D':
                flOptions |= OPT_DEBUG;
                break;
            case 'n':
            case 'N':
                flOptions |= OPT_BREAKONNEWTASK;
                break;
            case 'l':
            case 'L':
                iLogLevel = atoi(++pWOWCmdLine);
                break;
            default:
                break;
            }

        }
        pWOWCmdLine++;
    }

    if (iLogLevel > 0) {
        if (!(flOptions & OPT_DEBUG))
            if (!(OPENLOG()))
                iLogLevel = 0;
    }
    else
        iLogLevel = 0;

#endif

    //
    // Initialize list of known DLLs used by WK32WowIsKnownDLL
    // from the registry.
    //

    WK32InitWowIsKnownDLL(WowKey);

    RegCloseKey (WowKey);

    //
    // Initialize param mapping cache
    //
    //

    InitParamMap();

    //
    // Set our GDI batching limit from win.ini.  This is useful for SGA and
    // other performance measurements which require each API to do its own
    // work.  To set the batching size to 1, which is most common, put the
    // following in win.ini:
    //
    // [WOW]
    // BatchLimit=1
    //
    // or using ini:
    //
    // ini WOW.BatchLimit = 1
    //
    // Note that this code only changes the batch limit if the above
    // line is in win.ini, otherwise we use default batching.  It's
    // important that this code be in the free build to be useful.
    //

    {
        extern DWORD dwWOWBatchLimit;                    // declared in wkman.c

        dwWOWBatchLimit = GetProfileInt("WOW",           // section
                                        "BatchLimit",    // key
                                        0                // default if not found
                                        );
    }

    ghProcess = NtCurrentProcess();

#ifdef DEBUG

#ifdef i386
    if (IsDebuggerAttached()) {
        if (GetProfileInt("WOWDebug", "debugbreaks", 0))
            *pNtVDMState |= VDM_BREAK_DEBUGGER;

        if (GetProfileInt("WOWDebug", "exceptions", 0))
            *pNtVDMState |= VDM_BREAK_EXCEPTIONS;
    }
#endif


    if (IsDebuggerAttached() && (flOptions & OPT_BREAKONNEWTASK)) {
        OutputDebugString("\nW32Init - Initialization Complete, Set any Breakpoints Now, type g to continue\n\n");
        DbgBreakPoint();
    }

#endif

    // Initialize ClipBoard formats structure.

    InitCBFormats ();

    // This is to initialize the InquireVisRgn for FileMaker Pro 2.0
    // InquireVisRgn is an undocumented API Win 3.1 API.

    InitVisRgn();


    // HUNG APP SUPPORT

    if (!WK32InitializeHungAppSupport()) {
        LOGDEBUG(LOG_ALWAYS, ("W32INIT Error: InitializeHungAppSupport Failed"));
        return FALSE;
    }

    SetPriorityClass(ghProcess, NORMAL_PRIORITY_CLASS);

#ifdef DEBUG_MEMLEAK
    // for memory leak support
    InitializeCriticalSection(&csMemLeak);
#endif

    return TRUE;
}

/*  Thunk Dispatch Table
 *
 *  see fastwow.h for instructions on how to create a new thunk table
 *
 */
#ifdef DEBUG_OR_WOWPROFILE
PA32 awThunkTables[] = {
    {W32TAB(aw32WOW,     "All     ", cAPIThunks)}
};
#endif

#ifdef DEBUG_OR_WOWPROFILE // define symbols for API profiling only (debugger extension)
INT   iThunkTableMax = NUMEL(awThunkTables);
PPA32 pawThunkTables = awThunkTables;
#endif // WOWPROFILE


/* WOW32UnimplementedAPI - Error Thunk is Not Implemented
 *
 * Stub thunk table entry for all unimplemented APIs on
 * the checked build, and on the free build NOPAPI and
 * LOCALAPI entries point here as well.
 *
 * ENTRY
 *
 * EXIT
 *
 */

ULONG FASTCALL WOW32UnimplementedAPI(PVDMFRAME pFrame)
{
#ifdef DEBUG
    INT  iFun;

    iFun = pFrame->wCallID;

    LOGDEBUG(2,("WOW32: Warning! %s: Function %i %s is not implemented.\n",
        GetModName(iFun),
        GetOrdinal(iFun),
        aw32WOW[iFun].lpszW32
        ));

    //
    // After complaining once about each API, patch the thunk table so
    // future calls to the API will (mostly) silently slip by in WOW32NopAPI.
    //

    aw32WOW[iFun].lpfnW32 = WOW32NopAPI;

#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return FALSE;
}


#ifdef DEBUG

/* WOW32Unimplemented95API - Error Thunk is Not Implemented
 *
 * Stub thunk table entry for Win95 unimplemented APIs on
 * the checked build, and for now on the free build as well.
 *
 * ENTRY
 *
 * EXIT
 *
 */

ULONG FASTCALL WOW32Unimplemented95API(PVDMFRAME pFrame)
{
    INT  iFun;

    iFun = pFrame->wCallID;

    WOW32ASSERTMSGF (FALSE, ("New-for-Win95/NT5 %s API %s #%i not implemented, contact DaveHart.\n",
        GetModName(iFun),
        aw32WOW[iFun].lpszW32,
        GetOrdinal(iFun)
        ));

    //
    // After complaining once about each API, patch the thunk table so
    // future calls to the API will silently slip by.
    //

    aw32WOW[iFun].lpfnW32 = NOPAPI;

    return FALSE;
}


/* WOW32NopAPI - Thunk to do nothing - checked build only.
 *
 * All Function tables point here for APIs which should do nothing.
 *
 * ENTRY
 *
 * EXIT
 *
 */

ULONG FASTCALL WOW32NopAPI(PVDMFRAME pFrame)
{
    INT iFun;

    iFun = pFrame->wCallID;

    LOGDEBUG(4,("%s: Function %i %s is NOP'd\n", GetModName(iFun), GetOrdinal(iFun), aw32WOW[iFun].lpszW32));

    return FALSE;
}


/* WOW32LocalAPI - ERROR Should Have Been Handled in 16 BIT
 *                Checked build only
 *
 * All Function tables point here for Local API Error Messages
 *
 * ENTRY
 *  Module startup registers:
 *
 * EXIT
 *
 *
 */

ULONG FASTCALL WOW32LocalAPI(PVDMFRAME pFrame)
{
    INT  iFun;

    iFun = pFrame->wCallID;

    WOW32ASSERTMSGF (FALSE, ("Error - %s: Function %i %s should be handled by 16-bit code\n",
        GetModName(iFun),
        GetOrdinal(iFun),
        aw32WOW[iFun].lpszW32
        ));

    return FALSE;
}

#endif // DEBUG


LPFNW32 FASTCALL W32PatchCodeWithLpfnw32(PVDMFRAME pFrame , LPFNW32 lpfnW32 )
{
    VPVOID vpCode;
    LPBYTE lpCode;
#ifdef DEBUG
    INT iFun = pFrame->wCallID;
#endif

#ifdef DEBUG_OR_WOWPROFILE
    //
    // On checked builds do not patch calls to the 4 special
    // thunks above, since many entries will point to each one,
    // the routines could not easily distinguish which 16-bit
    // entrypoint was called.
    //

    if (flOptions & OPT_DONTPATCHCODE ||
        lpfnW32 == UNIMPLEMENTEDAPI ||
        lpfnW32 == UNIMPLEMENTED95API ||
        lpfnW32 == NOPAPI ||
        lpfnW32 == LOCALAPI ) {

        goto Done;
    }
#endif

    //
    // just return the thunk function if called in real mode
    //
    if (!fWowMode) {
        goto Done;
    }

    // the thunk looks like so.
    //
    //    push HI_WCALLID (3bytes) - 0th byte is opcode.
    //    push 0xfnid     (3bytes)
    //    call wow16call  (5bytes)
    // ThunksCSIP:
    //

    // point to the 1st word (the hiword)
    vpCode = (DWORD)pFrame->wThunkCSIP - (0x5 + 0x3 + 0x2);

    WOW32ASSERT(HI_WCALLID == 0);  // we need to revisit wow32.c if this
                                   // value is changed to a non-zero value

    WOW32ASSERT(HIWORD(iFun) == HI_WCALLID);
    GETVDMPTR(vpCode, 0x2 + 0x3, lpCode);
    WOW32ASSERT(lpCode != NULL);

    WOW32ASSERT(*(PWORD16)(lpCode) == HIWORD(iFun));
    WOW32ASSERT(*(PWORD16)(lpCode+0x3) == LOWORD(iFun));

    *((PWORD16)lpCode) = HIWORD((DWORD)lpfnW32);
    lpCode += 0x3;                                // seek to the 2nd word (the loword)
    *((PWORD16)lpCode) = LOWORD((DWORD)lpfnW32);

    FLUSHVDMCODEPTR(vpCode, 0x2 + 0x3, lpCode);
    FREEVDMPTR(lpCode);

  Done:
    return lpfnW32;

}


/* W32Dispatch - Recipient of all WOW16 API calls (sort of)
 *
 * "sort of" means that the word "all" above hasn't been true since 8/93:
 *  1. Most calls to the 16-bit kernel are handled by krnl386.exe on the
 *     16-bit side (this has always been true).
 *  2. There are several User API's that are thunked on the 16-bit side by
 *     User32.dll code on x86 platforms.
 *  3. A FEW (only MulDiv?) GDI API's are thunked by GDI.exe in 16-bit land.
 *  4. On CHECKED x86 builds & ALL RISC builds, all API's not subject to the
 *     above exceptions are dispatched through this function.
 *  5. On x86 FREE platforms, API calls are dispatched from fastwow.asm
 *  - That's about it -- until we change it again, in which case this note
 *    could be terribly misleading.     cmjones  10/08/97
 *
 * Having said that:
 * This routine dispatches to the relavant WOW thunk routine via
 * jump tables wktbl.c wutbl.c wgtbl.c based on a function id on the 16 bit
 * stack.
 *
 * In debug versions it also calls routines to log parameters.
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 */
VOID W32Dispatch()
{
    INT iFun;
    ULONG ulReturn;
    DWORD  dwThunkCSIP;
    VPVOID vpCurrentStack;
    register PTD ptd;
    register PVDMFRAME pFrame;
#ifdef DEBUG_OR_WOWPROFILE
    INT iFunT;
#endif

#ifdef  WOWPROFILE
 DWORD  dwTics;
#endif

    //
    // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
    // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
    // thunk dispatching on retail builds.
    //

    try {

        //
        // if we get here then even if we're going to be fastbopping
        // then the faststack stuff must not be enabled yet.  that's why
        // there's no #if FASTBOPPING for this fetching of the vdmstack
        //

        vpCurrentStack = VDMSTACK();                // Get 16 bit ss:sp

        // Use WOWGetVDMPointer here since we can get called in RealMode on
        // Errors

        pFrame = WOWGetVDMPointer(vpCurrentStack, sizeof(VDMFRAME), fWowMode);

        ptd = CURRENTPTD();                         // Setup Task Pointer
        ptd->vpStack = vpCurrentStack;              // Save 16 bit ss:sp

        // ssync 16-bit & 32-bit common dialog structs (see wcommdlg.c)
        if(ptd->CommDlgTd) {
            dwThunkCSIP = (DWORD)(pFrame->wThunkCSIP);
            Ssync_WOW_CommDlg_Structs(ptd->CommDlgTd, w16to32, dwThunkCSIP);
        }

        WOW32ASSERT( FIELD_OFFSET(TD,vpStack) == 0 );

        LOGARGS(3,pFrame);                              // Perform Function Logging

        iFun = pFrame->wCallID;

#ifdef DEBUG_OR_WOWPROFILE
        iFunT = ISFUNCID(iFun) ?  iFun : GetFuncId(iFun) ;
#endif
        if (ISFUNCID(iFun)) {
#ifdef DEBUG
            if (cAPIThunks && iFunT >= cAPIThunks) {
                LOGDEBUG(LOG_ALWAYS,("W32Dispatch: Task %04x thunked to function %d, cAPIThunks = %d.\n",
                         pFrame->wTDB, iFunT, cAPIThunks));
                WOW32ASSERT(FALSE);
            }
#endif
            iFun = (INT)aw32WOW[iFun].lpfnW32;

            if ( ! HIWORD(iFun)) {
#ifdef WOWPROFILE // For API profiling only (debugger extension)
                dwTics = GetWOWTicDiff(0L);
#endif // WOWPROFILE
                ulReturn = InterpretThunk(pFrame, iFun);
                goto AfterApiCall;
            } else {
                W32PatchCodeWithLpfnw32(pFrame, (LPFNW32)iFun);
            }
        }


#ifdef WOWPROFILE // For API profiling only (debugger extension)
        dwTics = GetWOWTicDiff(0L);
#endif // WOWPROFILE

        //
        // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
        // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
        // thunk dispatching on retail builds.
        //

        ulReturn = (*((LPFNW32)iFun))(pFrame);      // Dispatch to Thunk

    AfterApiCall:

        // ssync 16-bit & 32-bit common dialog structs (see wcommdlg.c)
        if(ptd->CommDlgTd) {
            Ssync_WOW_CommDlg_Structs(ptd->CommDlgTd, w32to16, dwThunkCSIP);
        }


#ifdef WOWPROFILE // For API profiling only (debugger extension)
        dwTics = GetWOWTicDiff(dwTics);
        iFun = iFunT;
        // add time ellapsed for call to total
        aw32WOW[iFun].cTics += dwTics;
        aw32WOW[iFun].cCalls++; // inc # times this API called
#endif // WOWPROFILE

        FREEVDMPTR(pFrame);                                                     // Set the 16-bit return code
        GETFRAMEPTR(ptd->vpStack, pFrame);

        LOGRETURN(5,pFrame,ulReturn);                                           // Log return Values
        pFrame->wAX = LOW(ulReturn);                                            // Pass Back Return Value form thunk
        pFrame->wDX = HIW(ulReturn);

#ifdef DEBUG
        // If OPT_DEBUGRETURN is set, diddle the RetID as approp.

        if (flOptions & OPT_DEBUGRETURN) {
            if (pFrame->wRetID == RET_RETURN) {
                pFrame->wRetID =  RET_DEBUGRETURN;
                flOptions &= ~OPT_DEBUGRETURN;
            }
        }
        // Put the current logging level where 16-bit code can get it
        // Use ROMBIOS Hard DISK information as a safe address
        *(PBYTE)GetVDMAddr(0x0040,0x0042) = (BYTE)(iLogLevel/10+'0');
        *(PBYTE)GetVDMAddr(0x0040,0x0043) = (BYTE)(iLogLevel%10+'0');
#endif // DEBUG

        FREEVDMPTR(pFrame);

        SETVDMSTACK(ptd->vpStack);

    } except (W32Exception(GetExceptionCode(), GetExceptionInformation())) {

    }
    //
    // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
    // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
    // thunk dispatching on retail builds.
    //
}


#if NO_W32TRYCALL

INT
W32FilterException(
    INT ExceptionCode,
    PEXCEPTION_POINTERS ExceptionInformation
    )

/* W32FilterException - Filter WOW32 thread exceptions
 *
 * ENTRY
 *
 *    ExceptionCode - Indicate type of exception
 *
 *    ExceptionInformation - Supplies a pointer to ExceptionInformation
 *                           structure.
 *
 * EXIT
 *
 *    return exception disposition value.
 */

{
    extern BOOLEAN IsW32WorkerException(VOID);
    extern VOID W32SetExceptionContext(PCONTEXT);

    INT Disposition = EXCEPTION_CONTINUE_SEARCH;

    if ((ExceptionCode != EXCEPTION_WOW32_ASSERTION) &&
        IsW32WorkerException()) {

        Disposition = W32Exception(ExceptionCode, ExceptionInformation);
        if (Disposition == EXCEPTION_EXECUTE_HANDLER) {

            //
            // if this is the exception we want to handle, change its
            // context to the point where we can safely fail the api and
            // return exception disposition as continue execution.
            //

            W32SetExceptionContext(ExceptionInformation->ContextRecord);
            Disposition = EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return(Disposition);
}

#endif  // NO_W32TRYCALL


/* W32Exception - Handle WOW32 thread exceptions
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 *
 */

INT W32Exception(DWORD dwException, PEXCEPTION_POINTERS pexi)
{
    PTD     ptd;
    PVDMFRAME pFrame;

    DWORD   dwButtonPushed;
    char    szTask[9];
    HMODULE hModule;
    char    szModule[_MAX_PATH + 1];
    PSZ     pszModuleFilePart;
    PSZ     pszErrorFormatString;
    char    szErrorMessage[TOOLONGLIMIT + 4*WARNINGMSGLENGTH];
    char    szDialogText[TOOLONGLIMIT + 4*WARNINGMSGLENGTH];
    PTDB    pTDB;
    NTSTATUS Status;
    HANDLE DebugPort;
    PRTL_CRITICAL_SECTION PebLockPointer;
    CHAR AeDebuggerCmdLine[256];
    CHAR AeAutoDebugString[8];
    BOOL AeAutoDebug;
    WORD wDebugButton;


    if (!gfDebugExceptions) {

        //
        // If the process is being debugged, just let the exception happen
        // so that the debugger can see it. This way the debugger can ignore
        // all first chance exceptions.
        //

        DebugPort = (HANDLE)NULL;
        Status = NtQueryInformationProcess(
                    GetCurrentProcess(),
                    ProcessDebugPort,
                    (PVOID)&DebugPort,
                    sizeof(DebugPort),
                    NULL
                    );

        if ( NT_SUCCESS(Status) && DebugPort) {

            //
            // Process is being debugged.
            // Return a code that specifies that the exception
            // processing is to continue
            //
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }


    //
    // NtClose can raise exceptions if NtGlobalFlag is set for it.
    // We want to ignore these exceptions if we're not being debugged,
    // since the errors will be returned from the APIs and we generally
    // don't have control over what handles the app closes.  (Well, that's
    // not true for file I/O, but it is true for RegCloseKey.)
    //

    if (STATUS_INVALID_HANDLE == dwException ||
        STATUS_HANDLE_NOT_CLOSABLE == dwException) {

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    //
    // See if a debugger has been programmed in. If so, use the
    // debugger specified. If not then there is no AE Cancel support
    // DEVL systems will default the debugger command line. Retail
    // systems will not.
    //
    // The above paragraph was copied from the system exception
    // popup in base.  It is no longer true.  On retail systems,
    // AeDebug.Auto is set to 1 and AeDebug.Debugger is
    // "drwtsn32 -p %ld -e %ld -g".
    //
    // This means if we support AeDebug for stress, customers don't see
    // our exception popup and misalignment handling -- instead they get
    // a nearly-useless drwtsn32.log and popup.
    //
    // SO, we check for this situation and act as if no debugger was
    // enabled.
    //

    wDebugButton = 0;
    AeAutoDebug = FALSE;

    //
    // If we are holding the PebLock, then the createprocess will fail
    // because a new thread will also need this lock. Avoid this by peeking
    // inside the PebLock and looking to see if we own it. If we do, then just allow
    // a regular popup.
    //

    PebLockPointer = NtCurrentPeb()->FastPebLock;

    if ( PebLockPointer->OwningThread != NtCurrentTeb()->ClientId.UniqueThread ) {

        try {
            if ( GetProfileString(
                    "AeDebug",
                    "Debugger",
                    NULL,
                    AeDebuggerCmdLine,
                    sizeof(AeDebuggerCmdLine)-1
                    ) ) {
                wDebugButton = SEB_CANCEL;

                if ( GetProfileString(
                        "AeDebug",
                        "Auto",
                        "0",
                        AeAutoDebugString,
                        sizeof(AeAutoDebugString)-1
                        ) ) {

                    if ( !WOW32_strcmp(AeAutoDebugString,"1") ) {
                        AeAutoDebug = TRUE;
                    }
                }
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            wDebugButton = 0;
            AeAutoDebug = FALSE;
        }
    }

    //
    // See comment above about drwtsn32
    //

    if (AeAutoDebug &&
        !WOW32_strnicmp(AeDebuggerCmdLine, szDrWtsn32, (sizeof szDrWtsn32) - 1)) {

        wDebugButton = 0;
        AeAutoDebug = FALSE;
    }

    ptd = CURRENTPTD();
    GETFRAMEPTR(ptd->vpStack, pFrame);

    pTDB = (PVOID)SEGPTR(ptd->htask16,0);

    //
    // Get a zero-terminated copy of the Win16 task name.
    //

    RtlZeroMemory(szTask, sizeof(szTask));
    RtlCopyMemory(szTask, pTDB->TDB_ModName, sizeof(szTask)-1);

    //
    // Translate exception address to module name in szModule.
    //

    strcpy(szModule, CRITSTR(TheWin16Subsystem));
    RtlPcToFileHeader(pexi->ExceptionRecord->ExceptionAddress, (PVOID *)&hModule);
    GetModuleFileName(hModule, szModule, sizeof(szModule));
    pszModuleFilePart = WOW32_strrchr(szModule, '\\');
    if (pszModuleFilePart) {
        pszModuleFilePart++;
    } else {
        pszModuleFilePart = szModule;
    }


    //
    // Format error message into szErrorMessage
    //

    switch (dwException) {

        case EXCEPTION_ACCESS_VIOLATION:
            pszErrorFormatString = CRITSTR(CausedAV);
            break;

        case EXCEPTION_STACK_OVERFLOW:
            pszErrorFormatString = CRITSTR(CausedStackOverflow);
            break;

        case EXCEPTION_DATATYPE_MISALIGNMENT:
            pszErrorFormatString = CRITSTR(CausedAlignmentFault);
            break;

        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
            pszErrorFormatString = CRITSTR(CausedIllegalInstr);
            break;

        case EXCEPTION_IN_PAGE_ERROR:
            pszErrorFormatString = CRITSTR(CausedInPageError);
            break;

        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            pszErrorFormatString = CRITSTR(CausedIntDivideZero);
            break;

        case EXCEPTION_FLT_DENORMAL_OPERAND:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INEXACT_RESULT:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_FLT_UNDERFLOW:
            pszErrorFormatString = CRITSTR(CausedFloatException);
            break;

        default:
            pszErrorFormatString = CRITSTR(CausedException);
    }

    wsprintf(szErrorMessage,
             pszErrorFormatString,
             szTask,
             pszModuleFilePart,
             pexi->ExceptionRecord->ExceptionAddress,
             dwException
             );

    LOGDEBUG(LOG_ALWAYS, ("W32Exception:\n%s\n",szErrorMessage));

    //
    // Format dialog text into szDialogText and display.
    //

    if (AeAutoDebug) {

        dwButtonPushed = 2;

    } else {

        if (wDebugButton == SEB_CANCEL) {

            wsprintf(szDialogText,
                     "%s\n%s\n%s\n%s\n",
                     szErrorMessage,
                     CRITSTR(ChooseClose),
                     CRITSTR(ChooseCancel),
                     (dwException == EXCEPTION_DATATYPE_MISALIGNMENT)
                         ? CRITSTR(ChooseIgnoreAlignment)
                         : CRITSTR(ChooseIgnore)
                     );
        } else {

            wsprintf(szDialogText,
                     "%s\n%s\n%s\n",
                     szErrorMessage,
                     CRITSTR(ChooseClose),
                     (dwException == EXCEPTION_DATATYPE_MISALIGNMENT)
                         ? CRITSTR(ChooseIgnoreAlignment)
                         : CRITSTR(ChooseIgnore)
                     );

        }

        dwButtonPushed = WOWSysErrorBox(
                CRITSTR(ApplicationError),
                szDialogText,
                SEB_CLOSE,
                wDebugButton,
                SEB_IGNORE | SEB_DEFBUTTON
                );

    }

    //
    // If CANCEL is chosen Launch Debugger.
    //

    if (dwButtonPushed == 2) {

        BOOL b;
        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInformation;
        CHAR CmdLine[256];
        NTSTATUS Status;
        HANDLE EventHandle;
        SECURITY_ATTRIBUTES sa;

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        EventHandle = CreateEvent(&sa,TRUE,FALSE,NULL);
        RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
        sprintf(CmdLine,AeDebuggerCmdLine,GetCurrentProcessId(),EventHandle);
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.lpDesktop = "Winsta0\\Default";
        CsrIdentifyAlertableThread();
        b =  CreateProcess(
                NULL,
                CmdLine,
                NULL,
                NULL,
                TRUE,
                0,
                NULL,
                NULL,
                &StartupInfo,
                &ProcessInformation
                );

        if ( b && EventHandle) {

            //
            // Do an alertable wait on the event
            //

            Status = NtWaitForSingleObject(
                        EventHandle,
                        TRUE,
                        NULL
                        );
            return EXCEPTION_CONTINUE_SEARCH;

        } else {

            LOGDEBUG(0, ("W32Exception unable to start debugger.\n"));
            goto KillTask;
        }
    }

    //
    // If IGNORE is chosen and it's an EXCEPTION_DATATYPE_MISALIGNMENT,
    // turn on software emulation of misaligned access and restart the
    // faulting instruction.  Otherwise,  just fail the API and continue.
    //

    if (dwButtonPushed == 3) {

        if (dwException == EXCEPTION_DATATYPE_MISALIGNMENT) {
            SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);
            LOGDEBUG(0, ("W32Exception disabling alignment fault exceptions at user's request.\n"));
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        LOGDEBUG(0, ("W32Exception ignoring at user's request via EXCEPTION_EXECUTE_HANDLER\n"));
        return EXCEPTION_EXECUTE_HANDLER;
    }

    //
    // If user typed CLOSE or Any of the above fail,
    // force just the task to die.
    //

KillTask:
    LOGDEBUG(0, ("W32Exception killing task via RET_FORCETASKEXIT\n"));
    GETFRAMEPTR(ptd->vpStack, pFrame);
    pFrame->wRetID = RET_FORCETASKEXIT;
    return EXCEPTION_EXECUTE_HANDLER;

}


#ifdef DEBUG
VOID StartDebuggerForWow(VOID)
/*++

Routine Description:

    This routine checks to see if there's a debugger attached to WOW.  If not,
    it attempts to spawn one with a command to attach to WOW.  If the system
    was booted with /DEBUG in boot.ini (kernel debugger enabled), we'll run
    "ntsd -d" otherwise we'll run "ntsd".

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL fKernelDebuggerEnabled, b;
    NTSTATUS Status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInformation;
    ULONG ulReturnLength;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    CHAR szCmdLine[256];
    HANDLE hEvent;

    //
    // Are we being run under a debugger ?
    //

    if (IsDebuggerAttached()) {

        //
        // No need to start one.
        //

        return;
    }


    //
    // Is the kernel debugger enabled?
    //

    Status = NtQuerySystemInformation(
                 SystemKernelDebuggerInformation,
                 &KernelDebuggerInformation,
                 sizeof(KernelDebuggerInformation),
                 &ulReturnLength
                 );

    if (NT_SUCCESS(Status) &&
        (ulReturnLength >= sizeof(KernelDebuggerInformation))) {

        fKernelDebuggerEnabled = KernelDebuggerInformation.KernelDebuggerEnabled;

    } else {

        fKernelDebuggerEnabled = FALSE;
        LOGDEBUG(0,("StartDebuggerForWow: NtQuerySystemInformation(kdinfo) returns 0x%8.8x, return length 0x%08x.\n",
                    Status, ulReturnLength));

    }

    //
    // Create an event for NTSD to signal once it has fully connected
    // and is ready for the exception.  We force the handle to be inherited.
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    hEvent = CreateEvent(&sa, TRUE, FALSE, NULL);

    //
    // Build debugger command line.
    //

    wsprintf(szCmdLine, "ntsd %s -p %lu -e %lu -x -g -G",
            fKernelDebuggerEnabled ? "-d" : "",
            GetCurrentProcessId(),
            hEvent
            );

    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    b = CreateProcess(
            NULL,
            szCmdLine,
            NULL,
            NULL,
            TRUE,             // fInheritHandles
            CREATE_DEFAULT_ERROR_MODE,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );

    if (b) {
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);

        if (hEvent) {

            //
            // Wait for debugger to initialize.
            //

            WaitForSingleObject(hEvent, INFINITE);
        }
    }

    CloseHandle(hEvent);

    return;
}
#endif // DEBUG


BOOL IsDebuggerAttached(VOID)
/*++

Routine Description:

    Checks to see if there's a debugger attached to WOW.  If there is,
    this routine also turns on a bit in the 16-bit kernel's DS so it
    can do its part to report debug events.

Arguments:

    None.

Return Value:

    FALSE - no debugger attached or NtQueryInformationProcess fails.
    TRUE  - debugger is definitely attached.

--*/

{
    NTSTATUS     Status;
    HANDLE       MyDebugPort;
    LPBYTE       lpDebugWOW;
    static BOOL  fDebuggerAttached = FALSE;
    static BOOL  fKernel16Notified = FALSE;

    //
    // Don't bother checking if we already have been told that
    // there is a debugger attached, since debuggers cannot detach.
    //

    if (!fDebuggerAttached) {

        //
        // Query our ProcessDebugPort, if it is nonzero we have
        // a debugger attached.
        //

        Status = NtQueryInformationProcess(
                     NtCurrentProcess(),
                     ProcessDebugPort,
                     (PVOID)&MyDebugPort,
                     sizeof(MyDebugPort),
                     NULL
                     );

        fDebuggerAttached = NT_SUCCESS(Status) && (MyDebugPort != NULL);

    }

    //
    // If we have a debugger attached share that information
    // with the 16-bit kernel.
    //

    if (!fKernel16Notified && fDebuggerAttached && vpDebugWOW != 0) {

        GETVDMPTR(vpDebugWOW, 1, lpDebugWOW);
        *lpDebugWOW |= 1;
        FREEVDMPTR(lpDebugWOW);

        DBGNotifyDebugged( TRUE );

        fKernel16Notified = TRUE;
    }

    return fDebuggerAttached;
}


void *
WOWGetVDMPointer(
    VPVOID Address,
    DWORD  Count,
    BOOL   ProtectedMode
    )
/*++

Routine Description:

    This routine converts a 16/16 address to a linear address.

    WARNING NOTE - This routine has been optimized so protect mode LDT lookup
    falls stright through.

Arguments:

    Address -- specifies the address in seg:offset format
    Size -- specifies the size of the region to be accessed.
    ProtectedMode -- true if the address is a protected mode address

Return Value:

    The pointer.

--*/

{
    if (ProtectedMode) {
        return GetPModeVDMPointer(Address, Count);
    } else {
        return GetRModeVDMPointer(Address);
    }
}


PVOID FASTCALL
GetPModeVDMPointerAssert(
    DWORD Address
#ifdef DEBUG
    ,  DWORD Count
#endif
    )
/*++

Routine Description:

    Convert a 16:16 protected mode address to the equivalent flat pointer.

Arguments:

    Address -- specifies the address in selector:offset format

Return Value:

    The pointer.

--*/

{
#ifdef DEBUG
    void *vp;
#endif

    // what to do if this assert fires??  Currently "nothing" seems to work OK.
    WOW32WARNMSG((ExpLdt),("WOW::GetPModeVDMPointerAssert: ExpLdt == NULL\n"));

    //
    // Check to see if the descriptor is marked present
    // We assume here that ExpLdt is DWORD ALIGNED to avoid a slower
    // unaligned access on risc.
    //

    if (!((ExpLdt)[(Address >> 18) | 1] & LDT_DESC_PRESENT)) {
        PARM16 Parm16;
        ULONG ul;

        if ((HIWORD(Address) & STD_SELECTOR_BITS) == STD_SELECTOR_BITS) {
            // We've determined that the selector is valid and not
            // present. So we call over to kernel16 to have it load
            // the selector into a segment register. This forces a
            // segment fault, and the segment should be brought in.
            // Note that CallBack16 also calls this routine, so we could
            // theoretically get into an infinite recursion loop here.
            // This could only happen if selectors like the 16-bit stack
            // were not present, which would mean we are hosed anyway.
            // Such a loop should terminate with a stack fault eventually.

            Parm16.WndProc.lParam = (LONG) Address;
            CallBack16(RET_FORCESEGMENTFAULT, &Parm16, 0, &ul);
        } else {

            // We come here if the address can't be resolved. A null
            // selector is special-cased to allow for a null 16:16
            // pointer to be passed.
            if (HIWORD(Address)) {

                LOGDEBUG(LOG_ALWAYS,("WOW::GetVDMPointer: *** Invalid 16:16 address %04x:%04x\n",
                    HIWORD(Address), LOWORD(Address)));
                // If we get here, then we are about to return a bogus
                // flat pointer.
                // I would prefer to eventually assert this, but it
                // appears to be overactive for winfax lite.
                //WOW32ASSERT(FALSE);

            }

        }
    }


#ifdef DEBUG
    if (vp = GetPModeVDMPointerMacro(Address, Count)) {

#ifdef _X86_
        //
        // Check the selector limit on x86 only and return NULL if
        // the limit is too small.
        //

        if (SelectorLimit &&
            (Address & 0xFFFF) + Count > SelectorLimit[Address >> 19] + 1)
        {
            WOW32ASSERTMSGF (FALSE, ("WOW32 limit check assertion: %04x:%04x size %x is beyond limit %x.\n",
                Address >> 16,
                Address & 0xFFFF,
                Count,
                SelectorLimit[Address >> 19]
                ));

            return vp;
        }
#endif

#if 0 // this code is a paranoid check, only useful when debugging GetPModeVDMPointer.
        if (vp != Sim32GetVDMPointer(Address, Count, TRUE)) {
            LOGDEBUG(LOG_ALWAYS,
                ("GetPModeVDMPointer: GetPModeVDMPointerMacro(%x) returns %x, Sim32 returns %x!\n",
                 Address, vp, Sim32GetVDMPointer(Address, Count, TRUE)));
            vp =  Sim32GetVDMPointer(Address, Count, TRUE);
        }
#endif

        return vp;

    } else {

        return NULL;

    }
#else
    return GetPModeVDMPointerMacro(Address, 0);  // No limit check on free build.
#endif // DEBUG
}




ULONG FASTCALL WK32WOWGetFastAddress( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)WOWBopEntry;
#else
    return 0;
#endif
}

ULONG FASTCALL WK32WOWGetFastCbRetAddress( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)FastWOWCallbackRet;
#else
    return( 0L );
#endif
}

ULONG FASTCALL WK32WOWGetTableOffsets( PVDMFRAME pFrame )
{
    PWOWGETTABLEOFFSETS16 parg16;
    PTABLEOFFSETS   pto16;

    GETARGPTR(pFrame, sizeof(PDWORD16), parg16);
    GETVDMPTR(parg16->vpThunkTableOffsets, sizeof(TABLEOFFSETS), pto16);

    RtlCopyMemory(pto16, &tableoffsets, sizeof(TABLEOFFSETS));

    FLUSHVDMPTR(parg16->vpThunkTableOffsets, sizeof(TABLEOFFSETS), pto16);
    FREEVDMPTR(pto16);

    FREEARGPTR(parg16);

#if FASTBOPPING
    fKernelCSIPFixed = TRUE;
#endif

    return 1;
}

ULONG FASTCALL WK32WOWGetFlatAddressArray( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)FlatAddress;
#else
    return 0;
#endif
}


#ifdef DEBUG

/*
 * DoAssert - do an assertion.  called after the expression has been evaluted
 *
 * Input:
 *
 *
 * Note if the requested log level is not what we want we don't output
 *  but we always output to the circular buffer - just in case.
 *
 *
 */
int DoAssert(PSZ szAssert, PSZ szModule, UINT line, UINT loglevel)
{
    INT savefloptions;

    //
    // Start a debugger for WOW if there isn't already one.
    //
    // Until now StartDebuggerForWow was started by
    // the exception filter, which meant asserts on a
    // checked build got the debugger but the user didn't see
    // the assertion text on the debugger screen because
    // logprintf was called before the debugger attached.
    // -- DaveHart 31-Jan-95
    //

    StartDebuggerForWow();

    savefloptions = flOptions;
    flOptions |= OPT_DEBUG;         // *always* print the message

    //
    // szAssert is NULL for bare-bones WOW32ASSERT()
    //

    if (szAssert == NULL) {
        LOGDEBUG(loglevel, ("WOW32 assertion failure: %s line %d\n", szModule, line));
    } else {
        LOGDEBUG(loglevel, ("%s", szAssert));
    }

    flOptions = savefloptions;

    if (IsDebuggerAttached()) {

        DbgBreakPoint();

    } else {

        DWORD dw = SetErrorMode(0);

        RaiseException(EXCEPTION_WOW32_ASSERTION, 0, 0, NULL);

        SetErrorMode(dw);

    }

    return 0;
}



/*
 * sprintf_gszAssert
 *
 * Used by WOW32ASSERTMSGF to format the assertion text into
 * a global buffer, gszAssert.  There is probably a better way.
 *
 * DaveHart 15-Jun-95.
 *
 */
int _cdecl sprintf_gszAssert(PSZ pszFmt, ...)
{
    va_list VarArgs;

    va_start(VarArgs, pszFmt);

    return vsprintf(gszAssert, pszFmt, VarArgs);
}



/*
 * logprintf - format log print routine
 *
 * Input:
 * iReqLogLevel - Requested Logging Level
 *
 * Note if the requested log level is not what we want we don't output
 *  but we always output to the circular buffer - just in case.
 *
 *
 */
VOID logprintf(PSZ pszFmt, ...)
{
    DWORD   lpBytesWritten;
    int     len;
    char    text[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    len = vsprintf(text, pszFmt, arglist);

    // fLog states (set by !wow32.logfile debugger extension):
    //    0 -> no logging;
    //    1 -> log to file
    //    2 -> create log file
    //    3 -> close log file
    if(fLog > 1) {
        if(fLog == 2) {
            if((hfLog = CreateFile(szLogFile,
                                   GENERIC_WRITE,
                                   FILE_SHARE_WRITE,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL)) != INVALID_HANDLE_VALUE) {
                fLog = 1;
            }
            else {
                hfLog = NULL;
                fLog  = 0;
                OutputDebugString("Couldn't open log file!\n");
            }
        }
        else {
            FlushFileBuffers(hfLog);
            CloseHandle(hfLog);
            hfLog = NULL;
            fLog  = 0;
        }
    }

    if ( len > TMP_LINE_LEN-1 ) {
        text[TMP_LINE_LEN-2] = '\n';
        text[TMP_LINE_LEN-1] = '\0';        /* Truncate to 128 */
    }

    IFLOG(iReqLogLevel) {
        // write to file?
        if (fLog) {
            WriteFile(hfLog, text, len, &lpBytesWritten, NULL);
        }
        // write to terminal?
        else if (flOptions & OPT_DEBUG) {
            OutputDebugString(text);
        }
    }

    strcpy(&achTmp[iCircBuffer][0], text);
    if (--iCircBuffer < 0 ) {
        iCircBuffer = CIRC_BUFFERS-1;
    }
}

/*
 *  checkloging - Some Functions we don't want to log
 *
 *  Entry
 *   fLogFilter = Filter for Specific Modules - Kernel, User, GDI etc.
 *   fLogTaskFilter = Filter for specific TaskID
 *
 *  Exit: TRUE - OK to LOG Event
 *        FALSE - Don't Log Event
 *
 */
BOOL checkloging(register PVDMFRAME pFrame)
{
    INT i;
    BOOL bReturn;
    INT iFun = GetFuncId(pFrame->wCallID);
    PTABLEOFFSETS pto = &tableoffsets;


    // Filter on Specific Call IDs

    if (awfLogFunctionFilter[0] != 0xffff) {
        INT nOrdinal;

        nOrdinal = GetOrdinal(iFun);

        bReturn = FALSE;
        for (i=0; i < FILTER_FUNCTION_MAX ; i++) {
            if (awfLogFunctionFilter[i] == nOrdinal) {
                bReturn = TRUE;
                break;
            }
        }
    } else {
        bReturn = TRUE;
    }

    // Do not LOG Internal Kernel Calls below level 20
    if (iLogLevel < 20 ) {
        if((iFun == FUN_WOWOUTPUTDEBUGSTRING) ||
         ((iFun < pto->user) && (iFun >= FUN_WOWINITTASK)))

            bReturn = FALSE;
    }

    // LOG Only Specific TaskID

    if (fLogTaskFilter != 0xffff) {
        if (fLogTaskFilter != pFrame->wTDB) {
            bReturn = FALSE;
        }
    }

    // LOG Filter On Modules USER/GDI/Kernel etc.

    switch (ModFromCallID(iFun)) {

    case MOD_KERNEL:
        if ((fLogFilter & FILTER_KERNEL) == 0 )
            bReturn = FALSE;
        break;
    case MOD_USER:
        if ((fLogFilter & FILTER_USER) == 0 )
            bReturn = FALSE;
        break;
    case MOD_GDI:
        if ((fLogFilter & FILTER_GDI) == 0 )
            bReturn = FALSE;
        break;
    case MOD_KEYBOARD:
        if ((fLogFilter & FILTER_KEYBOARD) == 0 )
            bReturn = FALSE;
        break;
    case MOD_SOUND:
        if ((fLogFilter & FILTER_SOUND) == 0 )
            bReturn = FALSE;
        break;
    case MOD_MMEDIA:
        if ((fLogFilter & FILTER_MMEDIA) == 0 )
            bReturn = FALSE;
        break;
    case MOD_WINSOCK:
        if ((fLogFilter & FILTER_WINSOCK) == 0 )
            bReturn = FALSE;
        break;
    case MOD_COMMDLG:
        if ((fLogFilter & FILTER_COMMDLG) == 0 ) {
            bReturn = FALSE;
        }
        break;
#ifdef FE_IME
    case MOD_WINNLS:
    if ((fLogFilter & FILTER_WINNLS) == 0 )
        bReturn = FALSE;
    break;
#endif // FE_IME
#ifdef FE_SB
    case MOD_WIFEMAN:
    if ((fLogFilter & FILTER_WIFEMAN) == 0 )
        bReturn = FALSE;
    break;
#endif
    default:
        break;
    }
    return (bReturn);
}


/*
 * Argument Logging For Tracing API Calls
 *
 *
 */
VOID logargs(INT iLog, register PVDMFRAME pFrame)
{
    register PBYTE pbArgs;
    INT iFun;
    INT cbArgs;

    if (checkloging(pFrame)) {
        iFun = GetFuncId(pFrame->wCallID);
        cbArgs = aw32WOW[iFun].cbArgs; // Get Number of Parameters

        if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
          LOGDEBUG(iLog,("%s(", aw32WOW[iFun].lpszW32));
        } else {
          LOGDEBUG(iLog,("%04X %08X %04X %s:%s(",pFrame->wTDB, pFrame->vpCSIP,pFrame->wAppDS, GetModName(iFun), aw32WOW[iFun].lpszW32));
        }

        GETARGPTR(pFrame, cbArgs, pbArgs);
        pbArgs += cbArgs;

        //
        // Log the function arguments a word at a time.
        // The first iteration of the while loop is unrolled so
        // that the main loop doesn't have to figure out whether
        // or not to print a comma.
        //

        if (cbArgs > 0) {

            pbArgs -= sizeof(WORD);
            cbArgs -= sizeof(WORD);
            LOGDEBUG(iLog,("%04x", *(PWORD16)pbArgs));

            while (cbArgs > 0) {

                pbArgs -= sizeof(WORD);
                cbArgs -= sizeof(WORD);
                LOGDEBUG(iLog,(",%04x", *(PWORD16)pbArgs));

            }
        }

        FREEARGPTR(pbArgs);
        LOGDEBUG(iLog,(")\n"));

        if (fDebugWait != 0) {
            DbgPrint("WOWSingle Step\n");
            DbgBreakPoint();
        }
    }
}


/*
 * logreturn - Log Return Values From Call
 *
 * Entry
 *
 * Exit - None
 */
VOID logreturn(INT iLog, register PVDMFRAME pFrame, ULONG ulReturn)
{
    INT iFun;

        if (checkloging(pFrame)) {
         iFun = GetFuncId(pFrame->wCallID);
         if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
           LOGDEBUG(iLog,("%s: %lx\n", aw32WOW[iFun].lpszW32, ulReturn));
         } else {
           LOGDEBUG(iLog,("%04X %08X %04X %s:%s: %lx\n", pFrame->wTDB, pFrame->vpCSIP, pFrame->wAppDS, GetModName(iFun), aw32WOW[iFun].lpszW32, ulReturn));
         }
        }
}

#endif // DEBUG




PVOID FASTCALL malloc_w (ULONG size)
{
    PVOID pv;

    pv = HeapAlloc(hWOWHeap, 0, size + TAILCHECK);
    WOW32ASSERTMSG(pv, "WOW32: malloc_w failing, returning NULL\n");

#ifdef DEBUG_MEMLEAK
    WOW32DebugMemLeak(pv, size, ML_MALLOC_W);
#endif

    return pv;

}


DWORD FASTCALL size_w (PVOID pv)
{
    DWORD  dwSize;

    dwSize = HeapSize(hWOWHeap, 0, pv) - TAILCHECK;

    return(dwSize);

}


PVOID FASTCALL malloc_w_zero (ULONG size)
{
    PVOID pv;

    pv = HeapAlloc(hWOWHeap, HEAP_ZERO_MEMORY, size + TAILCHECK);
    WOW32ASSERTMSG(pv, "WOW32: malloc_w_zero failing, returning NULL\n");

#ifdef DEBUG_MEMLEAK
    WOW32DebugMemLeak(pv, size, ML_MALLOC_W_ZERO);
#endif
    return pv;
}



VOID FASTCALL free_w (PVOID p)
{

#ifdef DEBUG_MEMLEAK
    WOW32DebugFreeMem(p);
#endif

    HeapFree(hWOWHeap, 0, (LPSTR)(p));
}



//
// malloc_w_or_die is for use by *initialization* code only, when we
// can't get WOW going because, for example, we can't allocate a buffer
// to hold the known DLL list.
//
// malloc_w_or_die should not be used by API or message thunks or worker
// routines called by API or message thunks.
//

PVOID FASTCALL malloc_w_or_die(ULONG size)
{
    PVOID pv;
    if (!(pv = malloc_w(size))) {
        WOW32ASSERTMSG(pv, "WOW32: malloc_w_or_die failing, terminating.\n");
        WOWStartupFailed();  // never returns.
    }
    return pv;
}



LPSTR malloc_w_strcpy_vp16to32(VPVOID vpstr16, BOOL bMulti, INT cMax)
{

    return(ThunkStr16toStr32(NULL, vpstr16, cMax, bMulti));
}




LPSTR ThunkStr16toStr32(LPSTR pdst32, VPVOID vpsrc16, INT cChars, BOOL bMulti)
/*++
   Thunks a 16-bit string to a 32-bit ANSI string.

   bMulti == TRUE means we are thunking a multi-string which is a list of NULL
             *separated* strings that *terminate* with a double NULL.

   Notes: If the original 32-bit buffer is too small to contain the new string,
          it will be free'd and a new 32-bit buffer will be allocated.  If a new
          32-bit buffer can't be allocated, the ptr to the original 32-bit
          buffer is returned with no changes to the contents.

   Returns: ptr to the original 32-bit buffer
            OR ptr to a new 32-bit buffer if the original buffer was too small
            OR NULL if psrc is NULL.
--*/
{
    PVOID  pbuf32;
    LPSTR  psrc16;
    INT    buf16size, iLen;
    INT    buf32size = 0;


    GETPSZPTR(vpsrc16, psrc16);

    if(!psrc16) {

        // the app doesn't want a buffer for this anymore
        // (this is primarily for comdlg support)
        if(pdst32) {
            free_w(pdst32);
        }
        return(NULL);
    }

    if(bMulti) {
        iLen = Multi_strlen(psrc16) + 1;
    } else {
        iLen = (INT)(strlen(psrc16) + 1);
    }
    buf16size = max(cChars, iLen);

    if(pdst32) {
        buf32size = (INT)size_w(pdst32);
    }

    // if 32-bit buffer is too small, NULL, or invalid -- alloc a bigger buffer
    if((buf32size < buf16size) || (!pdst32) || (buf32size == 0xFFFFFFFF)) {

        if(pbuf32 = malloc_w(buf16size)) {

            // now copy to the new 32-bit buffer
            if(bMulti) {
                Multi_strcpy(pbuf32, psrc16);
            } else {
                strcpy(pbuf32, psrc16);
            }

            // get rid of the old buffer
            if(pdst32) {
                free_w(pdst32);
            }

            pdst32 = pbuf32;
        }
        else {
            WOW32ASSERTMSG(0, "WOW32: ThunkStr16toStr32: malloc_w failed!\n");
        }
    }

    // else just use the original 32-bit buffer (99% of the time)
    else if(pdst32) {
        if(bMulti) {
            Multi_strcpy(pdst32, psrc16);
        } else {
            strcpy(pdst32, psrc16);
        }
    }

    FREEPSZPTR(psrc16);

    return(pdst32);
}




//
// WOWStartupFailed puts up a fatal error box and terminates WOW.
//

PVOID WOWStartupFailed(VOID)
{
    char szCaption[256];
    char szMsgBoxText[1024];

    LoadString(hmodWOW32, iszStartupFailed, szMsgBoxText, sizeof szMsgBoxText);
    LoadString(hmodWOW32, iszSystemError, szCaption, sizeof szCaption);

    MessageBox(GetDesktopWindow(),
        szMsgBoxText,
        szCaption,
        MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONSTOP | MB_OK | MB_DEFBUTTON1);

    ExitVDM(WOWVDM, ALL_TASKS);         // Tell Win32 All Tasks are gone.
    ExitProcess(EXIT_FAILURE);
    return (PVOID)NULL;
}



#ifdef FIX_318197_NOW

char*
WOW32_strchr(
    const char* psz,
    int         c
    )
{
    if (gbDBCSEnable) {
        unsigned int cc;

        for (; (cc = *psz); psz++) {
            if (IsDBCSLeadByte((BYTE)cc)) {
                if (*++psz == '\0') {
                    return NULL;
                }
                if ((unsigned int)c == ((cc << 8) | *psz) ) {    // DBCS match
                    return (char*)(psz - 1);
                }
            }
            else if ((unsigned int)c == cc) {
                return (char*)psz;      // SBCS match
            }
        }

        if ((unsigned int)c == cc) {    // NULL match
            return (char*)psz;
        }

        return NULL;
    }
    else {
        return strchr(psz, c);
    }
}

char*
WOW32_strrchr(
    const char* psz,
    int         c
    )
{
    if (gbDBCSEnable) {
        char*        r = NULL;
        unsigned int cc;

        do {
            cc = *psz;
            if (IsDBCSLeadByte((BYTE)cc)) {
                if (*++psz) {
                    if ((unsigned int)c == ((cc << 8) | *psz) ) {    // DBCS match
                        r = (char*)(psz - 1);
                    }
                }
                else if (!r) {
                    // return pointer to '\0'
                    r = (char*)psz;
                }
            }
            else if ((unsigned int)c == cc) {
                r = (char*)psz;    // SBCS match
            }
        } while (*psz++);

        return r;
    }
    else {
        return strrchr(psz, c);
    }
}

char*
WOW32_strstr(
    const char* str1,
    const char* str2
    )
{
    if (gbDBCSEnable) {
        char *cp, *endp;
        char *s1, *s2;

        cp = (char*)str1;
        endp = (char*)str1 + strlen(str1) - strlen(str2);

        while (*cp && (cp <= endp)) {
            s1 = cp;
            s2 = (char*)str2;

            while ( *s1 && *s2 && (*s1 == *s2) ) {
                s1++;
                s2++;
            }

            if (!(*s2)) {
                return cp;    // success!
            }

            cp = CharNext(cp);
        }

        return NULL;
    }
    else {
        return strstr(str1, str2);
    }
}

int
WOW32_strncmp(
    const char* str1,
    const char* str2,
    size_t      n
    )
{
    if (gbDBCSEnable) {
        int retval;

        if (n == 0) {
            return 0;
        }

        retval = CompareStringA( GetThreadLocale(),
                                 LOCALE_USE_CP_ACP,
                                 str1,
                                 n,
                                 str2,
                                 n );
        if (retval == 0) {
            //
            // The caller is not expecting failure.  Try the system
            // default locale id.
            //
            retval = CompareStringA( GetSystemDefaultLCID(),
                                     LOCALE_USE_CP_ACP,
                                     str1,
                                     n,
                                     str2,
                                     n );
        }

        if (retval == 0) {
            if (str1 && str2) {
                //
                // The caller is not expecting failure.  We've never had a
                // failure indicator before.  We'll do a best guess by calling
                // the C runtimes to do a non-locale sensitive compare.
                //
                return strncmp(str1, str2, n);
            }
            else if (str1) {
                return 1;
            }
            else if (str2) {
                return -1;
            }
            else {
                return 0;
            }
        }

        return retval - 2;
    }
    else {
        return strncmp(str1, str2, n);
    }
}

int
WOW32_strnicmp(
    const char* str1,
    const char* str2,
    size_t      n
    )
{
    if (gbDBCSEnable) {
        int retval;

        if (n == 0) {
            return 0;
        }

        retval = CompareStringA( GetThreadLocale(),
                                 LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                                 str1,
                                 n,
                                 str2,
                                 n );
        if (retval == 0) {
            //
            // The caller is not expecting failure.  Try the system
            // default locale id.
            //
            retval = CompareStringA( GetSystemDefaultLCID(),
                                     LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                                     str1,
                                     n,
                                     str2,
                                     n );
        }

        if (retval == 0) {
            if (str1 && str2) {
                //
                // The caller is not expecting failure.  We've never had a
                // failure indicator before.  We'll do a best guess by calling
                // the C runtimes to do a non-locale sensitive compare.
                //
                return _strnicmp(str1, str2, n);
            }
            else if (str1) {
                return 1;
            }
            else if (str2) {
                return -1;
            }
            else {
                return 0;
            }
        }

        return retval - 2;
    }
    else {
        return _strnicmp(str1, str2, n);
    }
}

#endif


//****************************************************************************
#ifdef DEBUG_OR_WOWPROFILE
DWORD GetWOWTicDiff(DWORD dwPrevCount) {
/*
 * Returns difference between a previous Tick count & the current tick count
 *
 * NOTE: Tick counts are in unspecified units  (PerfFreq is in MHz)
 */
    DWORD          dwDiff;
    LARGE_INTEGER  PerfCount, PerfFreq;

    NtQueryPerformanceCounter(&PerfCount, &PerfFreq);

    /* if ticks carried into high dword (assuming carry was only one) */
    if( dwPrevCount > PerfCount.LowPart ) {
        /* (0xFFFFFFFF - (dwPrevCount - LowPart)) + 1L caused compiler to
           optimize in an arithmetic overflow, so we do it in two steps
           to fool Mr. compiler
         */
        dwDiff = (dwPrevCount - PerfCount.LowPart) - 1L;
        dwDiff = ((DWORD)0xFFFFFFFF) - dwDiff;
    }
    else {
        dwDiff = PerfCount.LowPart - dwPrevCount;
    }

    return(dwDiff);

}

INT GetFuncId(DWORD iFun)
{
    INT i;
    static DWORD dwLastInput = -1;
    static DWORD dwLastOutput = -1;

    if (iFun == dwLastInput) {
        iFun = dwLastOutput;
    } else {
        dwLastInput = iFun;
        if (!ISFUNCID(iFun)) {
            for (i = 0; i < cAPIThunks; i++) {
                 if (aw32WOW[i].lpfnW32 == (LPFNW32)iFun)  {
                     iFun = i;
                     break;
                 }
            }
        }
        dwLastOutput = iFun;
    }

    return iFun;
}
#endif  // DEBUG_OR_WOWPROFILE



// for debugging memory leaks
#ifdef DEBUG_MEMLEAK

LPMEMLEAK lpMemLeakStart = NULL;
ULONG     ulalloc_Count = 1L;
DWORD     dwAllocFlags = 0;


VOID WOW32DebugMemLeak(PVOID lp, ULONG size, DWORD fHow)
{

    PVOID     pvCallersAddress, pvCallersCaller;
    LPMEMLEAK lpml;
    HGLOBAL   h32 = NULL;   // lp from ML_GLOBALTYPE's are really HGLOBAL's

    if(lp) {

        // if we are tracking this type
        if(dwAllocFlags & fHow) {

            // allocate a tracking node
            if(lpml = GlobalAlloc(GPTR, sizeof(MEMLEAK))) {
                lpml->lp    = lp;
                lpml->size  = size;
                lpml->fHow  = fHow;
                lpml->Count = ulalloc_Count++;  // save when originally alloc'd
                RtlGetCallersAddress(&pvCallersAddress, &pvCallersCaller);
                lpml->CallersAddress = pvCallersCaller;
                EnterCriticalSection(&csMemLeak);
                lpml->lpmlNext = lpMemLeakStart;
                lpMemLeakStart = lpml;
                LeaveCriticalSection(&csMemLeak);

            }
            WOW32WARNMSG(lpml,"WOW32DebugMemLeak: can't alloc node\n");
        }

        // add "EnD" signature for heap tail corruption checking
        if(fHow & ML_GLOBALTYPE) {
            h32 = (HGLOBAL)lp;
            lp = GlobalLock(h32);
        }

        if(lp) {
            ((CHAR *)(lp))[size++] = 'E';
            ((CHAR *)(lp))[size++] = 'n';
            ((CHAR *)(lp))[size++] = 'D';
            ((CHAR *)(lp))[size++] = '\0';

            if(h32) {
                GlobalUnlock(h32);
            }
        }
    }
}




VOID WOW32DebugReMemLeak(PVOID lpNew, PVOID lpOrig, ULONG size, DWORD fHow)
{
    PVOID     pvCallersAddress, pvCallersCaller;
    HGLOBAL   h32 = NULL;   // lp from ML_GLOBALTYPE's are really HGLOBAL's

    LPMEMLEAK lpml = lpMemLeakStart;

    if(lpNew) {
        if(dwAllocFlags & fHow) {

            // look for original ptr in the list
            while(lpml) {

                if(lpml->lp == lpOrig) {
                    break;
                }
                lpml = lpml->lpmlNext;
            }

            WOW32WARNMSG(lpml,
                         "WOW32DebugReMemLeak: can't find original node\n");

            // if we found the original ptr
            if(lpml) {

                // update our struct with new ptr if necessary
                if(lpNew != lpOrig) {
                    lpml->lp = lpNew;
                }
                lpml->size = size;
                lpml->fHow |= fHow;
                RtlGetCallersAddress(&pvCallersAddress, &pvCallersCaller);
                lpml->CallersAddress = pvCallersCaller;
                ulalloc_Count++;
            }
        }

        // for heap tail corruption checking
        if(fHow & ML_GLOBALTYPE) {
            h32 = (HGLOBAL)lpNew;
            lpNew = GlobalLock(h32);
        }

        if(lpNew) {

            ((CHAR *)(lpNew))[size++] = 'E';
            ((CHAR *)(lpNew))[size++] = 'n';
            ((CHAR *)(lpNew))[size++] = 'D';
            ((CHAR *)(lpNew))[size++] = '\0';
            if(h32) {
                GlobalUnlock(h32);
            }
        }
    }
}




VOID WOW32DebugFreeMem(PVOID lp)
{
    LPMEMLEAK lpmlPrev;
    LPMEMLEAK lpml = lpMemLeakStart;

    if(lp && dwAllocFlags) {
        while(lpml) {

            lpmlPrev = lpml;
            if(lpml->lp == lp) {

                WOW32DebugCorruptionCheck(lp, lpml->size);

                EnterCriticalSection(&csMemLeak);

                if(lpml == lpMemLeakStart) {
                    lpMemLeakStart = lpml->lpmlNext;
                }
                else {
                    lpmlPrev->lpmlNext = lpml->lpmlNext;
                }

                GlobalFree(lpml);  // free the LPMEMLEAK node

                LeaveCriticalSection(&csMemLeak);

                break;
            }
            else {
                lpml = lpml->lpmlNext;
            }
        }
        WOW32WARNMSG((lpml), "WOW32DebugFreeMem: can't find node\n");
    }
}




VOID WOW32DebugCorruptionCheck(PVOID lp, DWORD size)
{
    if(lp && size) {

        if(!((((CHAR *)(lp))[size++] == 'E')   &&
             (((CHAR *)(lp))[size++] == 'n')   &&
             (((CHAR *)(lp))[size++] == 'D')   &&
             (((CHAR *)(lp))[size++] == '\0')) ) {

            WOW32ASSERTMSG(FALSE,"WOW32DebugCorruptionCheck: Corrupt tail!!\n");
        }
    }
}



DWORD WOW32DebugGetMemSize(PVOID lp)
{
    LPMEMLEAK lpml = lpMemLeakStart;

    while(lpml) {

        if(lpml->lp == lp) {
            return(lpml->size);
        }

        lpml = lpml->lpmlNext;
    }
    return(0);
}



// NOTE: this is called ONLY IF built with DEBUG_MEMLEAK
HGLOBAL WOW32DebugGlobalAlloc(UINT flags, DWORD dwSize)
{
    HGLOBAL h32;

    h32 = GlobalAlloc(flags, dwSize + TAILCHECK);

    WOW32DebugMemLeak((PVOID)h32, dwSize, ML_GLOBALALLOC);

    return(h32);
}




// NOTE: this is called ONLY IF built with DEBUG_MEMLEAK
HGLOBAL WOW32DebugGlobalReAlloc(HGLOBAL h32, DWORD dwSize, UINT flags)
{
    HGLOBAL h32New;
    PVOID   lp32Orig;

    // get the original pointer & check the memory for tail corruption
    lp32Orig = (PVOID)GlobalLock(h32);
    WOW32DebugCorruptionCheck(lp32Orig, WOW32DebugGetMemSize((PVOID)h32));
    GlobalUnlock(h32);

    h32New = GlobalReAlloc(h32, dwSize + TAILCHECK, flags);

    // fix our memory list to account for the realloc
    WOW32DebugReMemLeak((PVOID)h32New,
                        (PVOID)h32,
                        dwSize + TAILCHECK,
                        ML_GLOBALREALLOC);

    return(h32New);
}




// NOTE: this is called ONLY IF built with DEBUG_MEMLEAK
HGLOBAL WOW32DebugGlobalFree(HGLOBAL h32)
{

    WOW32DebugFreeMem((PVOID)h32);

    h32 = GlobalFree(h32);

    if(h32) {
        LOGDEBUG(0, ("WOW32DebugFreeMem: Lock count not 0!\n"));
    }
    else {
        if(GetLastError() != NO_ERROR) {
            LOGDEBUG(0, ("WOW32DebugFreeMem: GlobalFree failed!\n"));
        }
    }

    return(h32);
}

#endif  // DEBUG_MEMLEAK
