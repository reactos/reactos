/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    windbgrm.c

Abstract:

    This file implements the WinDbg remote debugger that doesn't use any
    USER32.DLL or csrss functionality one initialized.

Author:

    Wesley Witt (wesw) 1-Nov-1993

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop


#include "memlist.h"


//
// globals
//
HANDLE              hEventService;
HANDLE              hEventLoadTl;
HANDLE              hConnectThread;
BOOL                fConnected;
BOOL                fStartup;
BOOL                fDebugger;
BOOL                fGuiMode = TRUE;
DWORD               dwRequest;
HPID                hpidSvc;
HANDLE              hout;
BOOL                fGonOnDisconnect;
// Name of a transport layer as it appears in the Options|Transport Dll..
PSTR                pszTlName;
// Name of a transport DLL and its arguments
PSTR                pszDll_Tl_NameArgs;
TLFUNC              TLFunc;
HINSTANCE           hTransportDll;
DWORD               dwProcessToAttach;
HANDLE              hEventForAttach;
DWORD               nextHpid;
CHAR                szCmdLine[512];
CHAR                ClientId[MAX_PATH];
LPSTR               ProgramName;
BOOL                bHelpOnInit;
HWND                HWndFrame;



void
xxUnlockHlle(
    HLLE h
    )
{
    UnlockHlle(h);
}

void *
SYMalloc(
    size_t n
    )
{
    return malloc(n);
}

void *
SYRealloc(
    void * p,
    size_t n
    )
{
    return realloc(p, n);
}

void
SYFree(
    void *p
    )
{
    free(p);
}


// registry function
WKSPSetupTL(
    TLFUNC TLFunc,
    CIndiv_TL_RM_WKSP * lpTl
    );



DBF Dbf = {
    SYMalloc,         // PVOID      (OSDAPI *  lpfnMHAlloc)
    SYRealloc,        // PVOID      (OSDAPI *  lpfnMHRealloc)
    SYFree,           // VOID       (OSDAPI *  lpfnMHFree)

    LLHlliInit,         // HLLI       (OSDAPI *  lpfnLLInit)
    LLHlleCreate,       // HLLE       (OSDAPI *  lpfnLLCreate)
    LLAddHlleToLl,      // VOID       (OSDAPI *  lpfnLLAdd)
    LLInsertHlleInLl,   // VOID       (OSDAPI *  lpfnLLInsert)
    LLFDeleteHlleFromLl, // BOOL       (OSDAPI *  lpfnLLDelete)
    LLHlleFindNext,     // HLLE       (OSDAPI *  lpfnLLNext)
    LLChlleDestroyLl,   // DWORD      (OSDAPI *  lpfnLLDestroy)
    LLHlleFindLpv,      // HLLE       (OSDAPI *  lpfnLLFind)
    LLChlleInLl,        // DWORD      (OSDAPI *  lpfnLLSize)
    LLLpvFromHlle,      // PVOID      (OSDAPI *  lpfnLLLock)
    xxUnlockHlle,       // VOID       (OSDAPI *  lpfnLLUnlock)
    LLHlleGetLast,      // HLLE       (OSDAPI *  lpfnLLLast)
    LLHlleAddToHeadOfLI, // VOID       (OSDAPI *  lpfnLLAddHead)
    LLFRemoveHlleFromLl, // BOOL       (OSDAPI *  lpfnLLRemove)
    0, // int        (OSDAPI *  lpfnLBAssert)
    0, // int        (OSDAPI *  lpfnLBQuit)
    0, // LPSTR      (OSDAPI *  lpfnSHGetSymbol)
    0, // DWORD      (OSDAPI * lpfnSHGetPublicAddr)
    0, // LPSTR      (OSDAPI * lpfnSHAddrToPublicName)
    0, // LPVOID     (OSDAPI * lpfnSHGetDebugData)
    0, // PVOID      (OSDAPI *  lpfnSHLpGSNGetTable)
#ifdef NT_BUILD_ONLY
    0, // BOOL       (OSDAPI *  lpfnSHWantSymbols)
#endif
    0, // DWORD      (OSDAPI *  lpfnDHGetNumber)
    0, // MPT        (OSDAPI *  lpfnGetTargetProcessor)
//    0, // LPGETSETPROFILEPROC   lpfnGetSet;
};



//
// prototypes
//
DWORD
ConnectThread(
    LPVOID lpv
    );

VOID
GetCommandLineArgs(
    PTSTR
    );

BOOL
LoadTransport(
    PSTR pszTransportName
    );

BOOL
UnLoadTransport();

void
ShutdownAndUnload();

XOSD
TLCallbackFunc(
    TLCB     tlcb,
    HPID     hpid,
    HTID     htid,
    UINT     wParam,
    LONG     lParam
    );

void
DebugPrint(
    char * szFormat,
    ...
    );

HANDLE
StartupDebugger(
    VOID
    );

BOOL
ConnectDebugger(
    CIndiv_TL_RM_WKSP * pCTl
    );

BOOL
DisConnectDebugger(
    HPID hpid
    );

BOOL
AttachProcess(
    HPID    hpid,
    DWORD   dwProcessToDebug,
    HANDLE  hEventForAttach
    );

BOOL
ProgramLoad(
    HPID   hpid,
    LPSTR  lpProgName
    );

BOOL
InitApplication(
    VOID
    );

BOOLEAN
DebuggerStateChanged(
    VOID
    );




VOID
_cdecl
main(
    int argc,
    char **argv
    )

/*++

Routine Description:

    This is the entry point for WINDBGRM

Arguments:

    None.

Return Value:

    None.

--*/

{
    #define append(s,n) p=p+sprintf(p,s,n)
    HANDLE  hDebugger;
    CHAR    buf[256];
    LPSTR   p;
    CIndiv_TL_RM_WKSP * pCTl = NULL;


//  DEBUG_OUT( "WinDbgRm initializing\n" );

    ProgramName = argv[0];

    GetCommandLineArgs( ProgramName );

    if (pszDll_Tl_NameArgs && pszTlName) {
        WKSP_MsgBox(NULL, IDS_ERR_TL_Both_Overrides);

        free(pszTlName);
        pszTlName = NULL;

        free(pszDll_Tl_NameArgs);
        pszDll_Tl_NameArgs = NULL;
    }

    // Get the registry information
    g_RM_Windbgrm_WkSp.Restore(FALSE);

    if (fGuiMode) {
        InitApplication();
    }

    SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    hEventService = CreateEvent( NULL, TRUE, FALSE, NULL );
    hEventLoadTl = CreateEvent( NULL, FALSE, FALSE, NULL );

    //
    // Load a transport DLL with arguments
    //

restartTL:
    while (TRUE) {

        if (pszDll_Tl_NameArgs) {
            //
            // The user specified a transport DLL with arguments
            //

            // split the cmd line into DLL and args. The DLL name must end with ".dll"
            PSTR pszDll = _strdup(pszDll_Tl_NameArgs);

            // convert everything to lower case. Make it easier to find the ".dll"
            strlwr(pszDll);

            PSTR psz = strstr(pszDll, ".dll");
            if (!psz) {
                WKSP_MsgBox(NULL, IDS_ERR_TL_N_Need_dll_In_Name);
            } else {
                pCTl = new CIndiv_TL_RM_WKSP();
                pCTl->SetRegistryName("");
                pCTl->m_pszDescription = _strdup("");
                pCTl->m_pszDll = pszDll;

                psz += strlen(".dll");
                *psz = NULL;

                // Skip white space
                for (++psz; *psz && isspace(*psz); psz++);

                pCTl->m_pszParams = strdup(psz);
            }
        } else {

            //
            // get the default transport layer
            //
            TListEntry<CIndiv_TL_RM_WKSP *> * pContEntry = NULL;

            // The user either specified a TL, or the default will be used.
            if (!pszTlName && g_RM_Windbgrm_WkSp.m_pszSelectedTL) {
                pszTlName = _strdup(g_RM_Windbgrm_WkSp.m_pszSelectedTL);
            }

            // Find the specified TL
            pContEntry = g_RM_Windbgrm_WkSp.m_dynacont_All_TLs.
                m_listConts.Find(pszTlName, WKSP_Generic_CmpRegName);

            if (0 == g_RM_Windbgrm_WkSp.m_dynacont_All_TLs.m_listConts.Size()) {
                // There are no transport layers defined.
                WKSP_MsgBox(NULL, IDS_ERR_No_Transport_Layers_Defined);
            } else if (NULL == pContEntry) {
                // ERROR: Transport layers have been defined.
                //  But either the specified TL doesn't exist or no TL
                //  has been selected (pszTlName == "").
                WKSP_MsgBox(NULL, IDS_ERR_Transport_Doesnt_Exist, pszTlName);
            } else {
                // OK. Get the TL.
                pCTl = pContEntry->m_tData;
                AssertType(*pCTl, CIndiv_TL_RM_WKSP);
            }
        }

        //
        // Make sure that we warn the user that he is using a transport layer
        // that requires named pipes and they are not supported on win9x
        //
        {
            static OSVERSIONINFO osi = {0};
            if (0 == osi.dwOSVersionInfoSize) {
                osi.dwOSVersionInfoSize = sizeof(osi);
                Assert(GetVersionEx(&osi));
            }

            if (VER_PLATFORM_WIN32_NT != osi.dwPlatformId && !_stricmp(pCTl->m_pszDll, "tlpipe.dll")) {
                PSTR pszWarning = WKSP_DynaLoadString(g_hInst, IDS_Sys_Warning);

                WKSP_MsgBox(pszWarning, IDS_ERR_Pipes_Not_Supported_On_Win9x,
                    pCTl->m_pszRegistryName, "tlpipe.dll");

                if (pszWarning) {
                    free(pszWarning);
                }
            }
        }

        if (pCTl) {

            //
            // Are we starting up as a debugging stub or are we going to attach to
            // something?
            //
            if (fDebugger) {

                //
                // This case simulates the remote debugger to get the
                // stub attached e.g. for AEDebug.
                //
                if ( !ConnectDebugger(pCTl) ) {

                    // Error initializing the TL of DM.
                    // Which ever it is, we don't care. We warn the user if
                    // we can, then we bail.
                    if (fGuiMode) {

                        WKSP_MsgBox(NULL,
                                    IDS_ERR_Initializing_TL_DM,
                                    pCTl->m_pszDll
                                    );
                    }

                } else {

                    if (szCmdLine[0]) {
                        nextHpid++;
                        ProgramLoad( (HPID)nextHpid, szCmdLine );
                    }
                    if (dwProcessToAttach) {
                        nextHpid++;
                        AttachProcess( (HPID)nextHpid, dwProcessToAttach, hEventForAttach);
                    }
                    DisConnectDebugger( (HPID) nextHpid );
                }

                // Whether we connected or not it's time to leave
                return;

            } else {

                //
                // load the TL and DM
                //
                if (LoadTransport(pCTl->m_pszDll)) {

                    //
                    // This will configure the TL, but not the DM.
                    //

                    // Update the UI
                    g_CWindbgrmFeedback.SetTL(pCTl);
                    g_CWindbgrmFeedback.UpdateInfo(HWndFrame);
                    UpdateWindow(HWndFrame);

                    WKSPSetupTL(TLFunc, pCTl);
                    break;
                }
            }
        }

        //
        // transport didn't load for one reason or another.
        // wait here until user changes the transport, then try again.
        //
        if (pszDll_Tl_NameArgs) {
            // The user fucked up
            free(pszDll_Tl_NameArgs);
            pszDll_Tl_NameArgs = NULL;
            delete pCTl;
        }

        // Start all over
        pCTl = NULL;

        if (!fGuiMode) {
            return;
        }

        WaitForSingleObject( hEventLoadTl, INFINITE );
    }

    //
    // If invoked with -p or a command line, start the process
    // before waiting for the debugger to connect.
    //
    if (fStartup) {

        fStartup = FALSE;

        hDebugger = StartupDebugger();

        if (hDebugger) {

            CloseHandle(hDebugger);

        } else {
            //
            // could not start fake debugger - should
            // we say something, or just silently die?
            //
            return;
        }
    }

    while ( TRUE ) {
        if (TLFunc(tlfConnect, NULL, sizeof(ClientId), (LPARAM) ClientId) == xosdNone) {

            fConnected = TRUE;
            g_CWindbgrmFeedback.SetClientName(ClientId);

            if (fGuiMode) {
                EnableTransportChange(FALSE);
            }

        } else {

            if (fConnected) {
                Sleep( 2000 );
            } else if (WaitForSingleObject( hEventLoadTl, 1000 ) == WAIT_OBJECT_0) {
                ShutdownAndUnload();
                goto restartTL;
            }

        }
    }

    return;
}

XOSD
TLCallbackFunc(
    TLCB     tlcb,
    HPID     hpid,
    HTID     htid,
    UINT     wParam,
    LONG     lParam
    )

/*++

Routine Description:

    Provides a callback function for the TL.  Currently
    this callback is only to signal a disconnect.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if ((tlcb != tlcbDisconnect) || (!TLFunc) || (!fConnected)) {
        return xosdNone;
    }

//  DebugPrint("windbgrm disconnecting...\n");

    fConnected = FALSE;
    *ClientId = NULL;
    g_CWindbgrmFeedback.SetClientName(ClientId);

    hpidSvc = hpid;
    htidBpt = htid;
    fGonOnDisconnect = wParam;
    SetEvent( hEventService );

    return xosdNone;
}


BOOL
LoadTransport(
              PSTR pszTransportName
              )

/*++

Routine Description:

    Loads the named pipes transport layer and does the necessary
    initialization of the TL.

Arguments:

    None.

Return Value:

    None.

--*/

{
    extern AVS      Avs;
    DBGVERSIONPROC  pVerProc;
    LPAVS           pavs;
    CHAR            szDmName[16];


    if ((hTransportDll = LoadLibrary(pszTransportName)) == NULL) {
        goto LoadTransportError;
    }

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hTransportDll, DBGVERSIONPROCNAME);
    if (!pVerProc) {
        goto LoadTransportError;
    }

    pavs = (*pVerProc)();

    if (pavs->rgchType[0] != 'T' || pavs->rgchType[1] != 'L') {
        goto LoadTransportError;
    }

    if (Avs.rlvt != pavs->rlvt) {
        goto LoadTransportError;
    }

    if (Avs.iRmj != pavs->iRmj) {
        goto LoadTransportError;
    }

    if ((TLFunc = (TLFUNC)GetProcAddress(hTransportDll, "TLFunc")) == NULL) {
        goto LoadTransportError;
    }

    if (TLFunc (tlfInit, NULL, (LPARAM) &Dbf, NULL ) != xosdNone) {
        goto LoadTransportError;
    }

    if (TLFunc(tlfSetErrorCB, NULL, 0, (LPARAM) &TLCallbackFunc) != xosdNone) {
        goto LoadTransportError;
    }

    return TRUE;

LoadTransportError:
    if (hTransportDll) {
        FreeLibrary( hTransportDll );
        hTransportDll = NULL;
    }

    return FALSE;
}


BOOL
UnLoadTransport()

/*++

Routine Description:

    Unloads the transport layer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    FreeLibrary( hTransportDll );
    hTransportDll = NULL;

    return TRUE;
}

void
ShutdownAndUnload()
{
    if (TLFunc) {
        TLFunc(tlfRemoteQuit, 0, 0, 0);
        TLFunc(tlfDestroy, 0, 0, 0);
    }
    UnLoadTransport();
}


HANDLE
StartupDebugger(
    VOID
    )
{
    CHAR                    szCommandLine[MAX_PATH];
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;


    sprintf( szCommandLine, "%s -q -d -n %s", ProgramName, pszTlName );
    if (dwProcessToAttach) {
        sprintf( &szCommandLine[strlen(szCommandLine)], " -p %d", dwProcessToAttach );
    }
    if (hEventForAttach) {
        sprintf( &szCommandLine[strlen(szCommandLine)], " -e %p", hEventForAttach );
    }
    if (szCmdLine[0]) {
        strcat( szCommandLine, " ");
        strcat( szCommandLine, szCmdLine );
    }

    GetStartupInfo( &si );
    si.hStdOutput = hout;

    if (!CreateProcess( NULL, szCommandLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi )) {
        return NULL;
    }

    return pi.hProcess;
}


VOID
GetCommandLineArgs(
    PTSTR pszProgramName
    )

/*++

Routine Description:

    Simple command line parser.

Arguments:

    pszProgramName - Skip over the path used to start WINDBGRM.
            This is the argv[0] string that was passed into "main".

Return Value:

    None.

--*/

{
    PSTR        pszCommandLineTransportDll = NULL;
    char        *lpstrCmd = GetCommandLine();
    UCHAR       ch;
    DWORD       i = 0;

    bHelpOnInit = FALSE;

    // skip over program name
    if ('"' != *lpstrCmd) {
        lpstrCmd += _tcslen(pszProgramName);
    } else {
        // The program name is enclosed in double quotes.
        // +2 skip over the quotes at the beginning and end.
        Assert('"' == *(lpstrCmd + _tcslen(pszProgramName) + 1) );
        lpstrCmd += _tcslen(pszProgramName) + 2;
    }

    //  skip over any following white space
    for (; isspace(*lpstrCmd); lpstrCmd++) {}

    //  process each switch character '-' as encountered

    ch = *lpstrCmd++;
    while (ch == '-' || ch == '/') {
        ch = *lpstrCmd++;
        //
        //  process multiple switch characters as needed
        //
        do {
            switch (ch) {

            default:
                return;

            case '?':
                //
                // usage
                //
                // we are not ready to call help, initialization needs to happen
                // however, we can set a global flag (yuk!) to tell windbgrm to
                // start help when (if!) it starts GUI-mode
                //
                bHelpOnInit = TRUE;
                ch = *lpstrCmd++;
                break;

            case 'd':
                //
                // no GUI
                //
                fGuiMode = FALSE;
                ch = *lpstrCmd++;
                break;

            case 'e':
                //
                // event to signal
                //
                // skip whitespace
                do {
                    ch = *lpstrCmd++;
                } while (ch == ' ' || ch == '\t');

                i=0;
                while (ch >= '0' && ch <= '9') {
                    i = i * 10 + ch - '0';
                    ch = *lpstrCmd++;
                }
                hEventForAttach = (HANDLE) i;
                break;

            case 'g':
                //
                // "go" - ignored
                //
                ch = *lpstrCmd++;
                break;

            case 'I':
                // Install windebug as the postmortem debugger
                // Why 'I', because it was suggested in the raid item
                {
                    HKEY hkey = NULL;
                    long lResult = 0;

                    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug",
                        0, KEY_READ | KEY_WRITE, &hkey);

                    if (ERROR_SUCCESS == lResult) {
                        // The key has been opened.
                        LPSTR lpsz = "windbgrm -p %ld -e %ld";
                        lResult = RegSetValueEx(hkey, "Debugger", 0, REG_SZ,
                            (PUCHAR) lpsz,
                            strlen(lpsz) +1); // Need to include terminating 0

                        Assert(ERROR_SUCCESS == RegCloseKey(hkey));
                    }

                    if (ERROR_SUCCESS != lResult) {
                        // An error occurred, notify the user.
                        WKSP_MsgBox(NULL, IDS_ERR_Inst_Postmortem_Debugger);
                    } else {
                        // An error occurred, notify the user.
                        PTSTR pszTitle = WKSP_DynaLoadString(g_hInst, IDS_Sys_Informational);
                        WKSP_MsgBox(pszTitle, IDS_Success_Inst_Postmortem_Debugger);
                        // Don't bother freeing it, since we are exiting.
                        exit(0);
                    }
                }
                break;

            case 'n':
                //
                // TL name - this must match one of the
                // names stored in the registry.
                //
                if (pszTlName) {
                    free(pszTlName);
                    pszTlName = NULL;
                }
                pszTlName = _strdup( GetArg(&lpstrCmd) );
                ch = *lpstrCmd++;
                break;

            case 'N':
                //
                // Name of a transport layer DLL and its arguments
                //
                pszDll_Tl_NameArgs = _strdup( GetArg(&lpstrCmd) );
                ch = *lpstrCmd++;
                break;

            case 'p':
                //
                // process id
                //
                // skip whitespace
                do {
                    ch = *lpstrCmd++;
                } while (ch == ' ' || ch == '\t');

                if ( ch == '-' ) {
                    ch = *lpstrCmd++;
                    if ( ch == '1' ) {
                        dwProcessToAttach = 0xffffffff;
                        ch = *lpstrCmd++;
                    }
                } else {
                    i=0;
                    while (ch >= '0' && ch <= '9') {
                        i = i * 10 + ch - '0';
                        ch = *lpstrCmd++;
                    }
                    dwProcessToAttach = i;
                }
                fStartup = TRUE;
                break;

            case 'q':
                //
                // Act as debugger shell
                //
                fDebugger = TRUE;
                ch = *lpstrCmd++;
                break;

            }
        } while (ch != ' ' && ch != '\t' && ch != '\0');

        while (ch == ' ' || ch == '\t') {
            ch = *lpstrCmd++;
        }
    }


    //
    // get the command line for a debuggee
    //

    while (ch == ' ' || ch == '\t') {
        ch = *lpstrCmd++;
    }
    i=0;
    while (ch) {
        szCmdLine[i++] = ch;
        ch = *lpstrCmd++;
    }
    szCmdLine[i] = '\0';
    if (i) {
        fStartup = TRUE;
    }

    return;
}


void
DebugPrint(
    char * szFormat,
    ...
    )

/*++

Routine Description:

    Provides a printf style function for doing outputdebugstring.

Arguments:

    None.

Return Value:

    None.

--*/

{
    va_list  marker;
    int n;
    char        rgchDebug[4096];

    va_start( marker, szFormat );
    n = _vsnprintf(rgchDebug, sizeof(rgchDebug), szFormat, marker );
    va_end( marker);

    if (n == -1) {
        rgchDebug[sizeof(rgchDebug)-1] = '\0';
    }

    OutputDebugString( rgchDebug );
    return;
}

void
ShowAssert(
    LPSTR condition,
    UINT  line,
    LPSTR file
    )
{
    char text[4096];
    int  id;

    sprintf(text, "Assertion failed - Line:%u, File:%Fs, Condition:%Fs", line, file, condition);
    DebugPrint( "%s\r\n", text );

    strcat(text, "\r\nOK to ignore, CANCEL to break");

    id = MessageBox( NULL, text, "WinDbgRm", MB_OKCANCEL | MB_ICONHAND | MB_TASKMODAL | MB_SETFOREGROUND);
    if (id != IDYES) {
        DebugBreak();
    }

    return;
}


