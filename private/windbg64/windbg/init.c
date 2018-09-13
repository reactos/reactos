/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module contains the initialization routines for Windbg.

Author:

    David J. Gilman (davegi) 21-Apr-1992
    Griffith Wm. Kadnier (v-griffk) 17-Sep-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntiodump.h>
#include <crash.h>

#include "include\cntxthlp.h"


DWORD GetComPorts(VOID);

void DisplayHelpContents(HWND hwnd, DWORD dwHelpContextId);


char    DebuggerName[ MAX_PATH ];

extern HMENU hMainMenuSave;
extern CRITICAL_SECTION     csLog;

/***    InitApplication - Initialize the Application specific information
**
**  Synopsis:
**  bool = InitApplication(hInstance)
**
**  Entry:
**  hInstance - Instance handle for Win16, Module handle for Win32
**
**  Returns:
**  TRUE if sucessful and FALSE otherwise
**
**  Description:
**  Initializes window data and registers window classes.
**
*/

BOOL 
InitApplication(
    HINSTANCE hInstance
    )
{
    WNDCLASS wc;
    char szClassName[MAX_MSG_TXT];


    // Load the richedit dll so that it can register the window class.
    {
        // Since we intentionally need this library the entire duration, we
        // simply load it and lose the handle to it. We are in win32 and running
        // separate address spaces, and don't have to worry about freeing the
        // library. Besides KENTF said it was okay to do it this way. If you have
        // any problems with this code, please ask him to explain the error of YOUR
        // ways. :)>
        if (NULL == LoadLibrary("RICHED32.DLL")) {
            FatalErrorBox(ERR_Init_Application, NULL);
            return FALSE;
        }
    }



    //We use tmp strings as edit buffers
    Assert(MAX_LINE_SIZE < TMP_STRING_SIZE);

    //Register the main window szClassName
    //Associates QcQp Icon
    //Associates Standard Arrow Cursor
    //Associates QcQp Menu

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(QCQPICON)));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = MAKEINTRESOURCE(MAIN_MENU);
    Dbg(LoadString(hInstance, SYS_Main_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass (&wc) ) {
        return FALSE;
    }

    //Register the MDI child szClassName
    //Associates Child Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON)));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Child_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

#if !defined( NEW_WINDOWING_CODE )
    //Register the Cpu child szClassName
    //Associates Cpu Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIPaneWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(CPUICON)));
    wc.hbrBackground = (HBRUSH) CreateSolidBrush(GRAYDARK);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Cpu_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Memory child szClassName
    //Associates Memory Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( MEMORYICON )));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Memory_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Command child szClassName
    //Associates Command Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( CMDICON )));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Cmd_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Floating Point child szClassName
    //Associates Floating Point Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIPaneWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( FLOATICON )));
    wc.hbrBackground = (HBRUSH) CreateSolidBrush(GRAYDARK);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Float_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    wc.hIcon = NULL;
    strcpy(LpszCommandLine, wc.lpszClassName);
    wc.lpszClassName = LpszCommandLine;
    (*LpszCommandLine) += 1;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Locals child szClassName
    //Associates Locals Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIPaneWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( LOCALSICON )));
    wc.hbrBackground = (HBRUSH) CreateSolidBrush(GRAYDARK);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Locals_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Watch child szClassName
    //Associates Watch Icon

    wc.style = 0;
    wc.lpfnWndProc = MDIPaneWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( WATCHICON )));
    wc.hbrBackground = (HBRUSH) CreateSolidBrush(GRAYDARK);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Watch_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }
#endif // ! NEW_WINDOWING_CODE 

    //Register the QuickWatch Watch child szClassName
    //Associates Watch Icon

    wc.style = 0;
    wc.lpfnWndProc = DLGPaneWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH) CreateSolidBrush(GRAYDARK);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    Dbg(LoadString(hInstance, SYS_Quick_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    wc.cbWndExtra = DLGWINDOWEXTRA;     // Used by QuickW dialog
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

#if !defined( NEW_WINDOWING_CODE )
    //Register the Editor child szClassName
    //Associates Caret Cursor

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = ChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra    = CBWNDEXTRA; // Is this really necessary
    wc.hInstance = hInstance;

    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName  = NULL;
    Dbg(LoadString(hInstance, SYS_Edit_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    //Register the Disassembler child szClassName

    wc.style = 0;
    wc.lpfnWndProc = MDIChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra    = CBWNDEXTRA; // Is this really necessary
    wc.hInstance = hInstance;

    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( DISASMICON )));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName  = NULL;
    Dbg(LoadString(hInstance, SYS_Disasm_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }


    //Register the Calls child szClassName
    wc.style = 0;
    wc.lpfnWndProc = CallsWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = CBWNDEXTRA; // Is this really necessary
    wc.hInstance = hInstance;
    Dbg(wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON)));
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName  = NULL;
    Dbg(LoadString(hInstance, SYS_Calls_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }
#endif // ! NEW_WINDOWING_CODE

    //Register the bitmap button
    // We want double clicks for the error display

    wc.style          = 0;
    wc.lpfnWndProc   = QCQPCtrlWndProc;
    wc.hInstance = hInstance;
    wc.cbClsExtra    = 0 ;
    wc.cbWndExtra    = CBWNDEXTRA_QCQPCTRL ;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszMenuName  = NULL;
    Dbg(LoadString(hInstance, SYS_QCQPCtrl_wClass, szClassName, MAX_MSG_TXT));
    wc.lpszClassName = szClassName;
    if (!RegisterClass(&wc)) {
        return FALSE ;
    }


#if defined( NEW_WINDOWING_CODE )
    // New Command window
    {
        WNDCLASSEX wcex;

        //
        // Cmd window
        //
        Dbg(LoadString(hInstance, SYS_NewCmd_wClass, szClassName, sizeof(szClassName)));

        wcex.cbSize         = sizeof(wcex);
        wcex.style          = 0;
        wcex.lpfnWndProc    = NewCmd_WindowProc;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(CMDICON) );
        wcex.hCursor        = LoadCursor(NULL, IDC_SIZENS);
        wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        wcex.lpszMenuName   = NULL;
        wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(CMDICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Calls window
        //
        Dbg(LoadString(hInstance, SYS_NewCalls_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewCalls_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON) );
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Doc window
        //
        Dbg(LoadString(hInstance, SYS_NewDocs_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewDoc_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(DOCICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // CPU window
        //
        Dbg(LoadString(hInstance, SYS_NewCpu_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewCpu_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(CPUICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(CPUICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Memory window
        //
        Dbg(LoadString(hInstance, SYS_NewMemory_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewMemory_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(MEMORYICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(MEMORYICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Float window
        //
        Dbg(LoadString(hInstance, SYS_NewFloat_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewFloat_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(FLOATICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(FLOATICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Locals window
        //
        Dbg(LoadString(hInstance, SYS_NewLocals_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewLocals_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(LOCALSICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(LOCALSICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Watch window
        //
        Dbg(LoadString(hInstance, SYS_NewWatch_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewWatch_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(WATCHICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(WATCHICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }

        //
        // Disasm window
        //
        Dbg(LoadString(hInstance, SYS_NewDisasm_wClass, szClassName, sizeof(szClassName)));

        //wcex.cbSize         = sizeof(wcex);
        //wcex.style          = 0;
        wcex.lpfnWndProc    = NewDisasm_WindowProc;
        //wcex.cbClsExtra     = 0;
        //wcex.cbWndExtra     = 0;
        //wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(DISASMICON) );
        //wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        //wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
        //wcex.lpszMenuName   = NULL;
        //wcex.lpszClassName  = szClassName;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(DISASMICON) );

        if (!RegisterClassEx(&wcex)) {
            return FALSE ;
        }
    }
#endif // NEW_WINDOWING_CODE


    return TRUE;
}                   /* InitApplication() */


LPSTR
GetArg(
    LPSTR *lpp
    )
{
    static PSTR pszBuffer = NULL;
    int r;
    LPSTR p1 = *lpp;

    while (*p1 == ' ' || *p1 == '\t') {
        p1++;
    }

    if (pszBuffer) {
        free(pszBuffer);
    }
    pszBuffer = (PSTR) calloc(strlen(p1) +1, 1);

    r = CPCopyString(&p1, pszBuffer, 0, (*p1 == '\'' || *p1 == '"'));
    if (r >= 0) {
        *lpp = p1;
    }
    return pszBuffer;
}


BOOL
InitCrashDump(
    LPSTR CrashDump
    )
{
    PDUMP_HEADER  DumpHeader  = NULL;
    DWORD         cb          = 0;
    DWORD         fsize       = 0;
    HANDLE        File        = NULL;
    HANDLE        MemMap      = NULL;
    PCHAR         DmpDumpBase = NULL;
    BOOL          rval        = FALSE;


    File = CreateFile(
        CrashDump,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (File == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    MemMap = CreateFileMapping(
        File,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );

    if (MemMap == 0) {
        goto exit;
    }

    fsize = GetFileSize( File,NULL );

    DmpDumpBase = (PSTR) MapViewOfFile(
        MemMap,
        FILE_MAP_READ,
        0,
        0,
        (fsize < 65536) ? fsize : 65536
        );

    if (DmpDumpBase == NULL) {
        goto exit;
    }

    DumpHeader = (PDUMP_HEADER)DmpDumpBase;

    if ((DumpHeader->Signature == 'EGAP') &&
        (DumpHeader->ValidDump == 'PMUD')) {

        g_contGlobalPreferences_WkSp.m_bCommandRepeat = TRUE;
        g_contWorkspace_WkSp.m_bKernelDebugger = TRUE;
        g_contKernelDbgPreferences_WkSp.m_bUseCrashDump = TRUE;
        FREE_STR(g_contKernelDbgPreferences_WkSp.m_pszCrashDump);
        g_contKernelDbgPreferences_WkSp.m_pszCrashDump = _strdup(CrashDump);

        switch( DumpHeader->MachineImageType ) {
        case IMAGE_FILE_MACHINE_I386:
            g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptix86;
            break;

        case IMAGE_FILE_MACHINE_IA64:
            g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptia64;
            break;

#if 0
        case IMAGE_FILE_MACHINE_R4000:
        case IMAGE_FILE_MACHINE_R10000:
            g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptmips;
            break;
#endif

        case IMAGE_FILE_MACHINE_ALPHA:
        case IMAGE_FILE_MACHINE_ALPHA64:
            g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptdaxp;
            break;

#if 0
        case IMAGE_FILE_MACHINE_POWERPC:
            g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptntppc;
            break;
#endif

        default:
            // This should not happen.
            Dbg(0);
        }

        rval = TRUE;
    }

    if ((DumpHeader->Signature == 'RESU') &&
        (DumpHeader->ValidDump == 'PMUD')) {
        //
        // usermode crash dump
        //

        g_contWorkspace_WkSp.m_bUserCrashDump = TRUE;
        FREE_STR(g_contWorkspace_WkSp.m_pszUserCrashDump);
        g_contWorkspace_WkSp.m_pszUserCrashDump = _strdup(CrashDump);
        rval = TRUE;
    }

exit:
    if (DmpDumpBase) {
        UnmapViewOfFile( DmpDumpBase );
    }
    if (MemMap) {
        CloseHandle( MemMap );
    }
    if (File) {
        CloseHandle( File );
    }

    return rval;
}

//BUGBUG - kcarlos. Place in a header
DWORD
GetProcessIdGivenName(
    PCSTR pszProcessName
    );


BOOL
InitInstance(
    int argc,
    char * argv[],
    HINSTANCE hInstance,
    int nCmdShow
    )
/*++

Routine Description:

    Finish initializing Windbg, parse and execute the command line.

Arguments:

    argc        - Supplies command line arg count

    argv        - Supplies command line args

    hInstance   - Supplies app instance handle (lpBaseOfImage)

    nCmdShow    - Supplies ShowWindow parameter

Return Value:

    TRUE if everything is OK, FALSE if something fails

--*/
{
/*
    extern BOOL AutoTest;
    char    ProgramName[ MAX_PATH *2] = {0};
    char    rgch[ MAX_PATH ] = {0};
    char    szTitle[ MAX_MSG_TXT ] = {0};
    char    szClass[ MAX_MSG_TXT ] = {0};
    char    WorkSpace[ MAX_PATH ] = {0};
    BOOL    bWorkSpaceSpecified      = FALSE;
    BOOL    toRestore               = FALSE;
    BOOL    workSpaceLoaded         = FALSE;
    BOOL    frameHidden             = TRUE;
    BOOL    LoadedDefaultWorkSpace  = FALSE;
    BOOL    CreatedNewDefault       = FALSE;
    BOOL    WorkSpaceMissed         = FALSE;
    BOOL    bJustSource             = FALSE;
    BOOL    fGoNow                  = FALSE;
    BOOL    SwitchK                 = FALSE;
    BOOL    SwitchH                 = FALSE;
    LPSTR   WorkSpaceToUse          = NULL;
    LONG    lAttachProcess          = -2;
    HANDLE  hEventGo                = NULL;
    LPSTR   lp1 = NULL;
    LPSTR   lp2 = NULL;
    RUNDEBUG_PARAMS rd = {0};
    BOOL    fRemoteServer = FALSE;
    BOOL    fMinimize = FALSE;
    CHAR    PipeName[MAX_PATH+1] = {0};
    BOOL    fSymPath = FALSE;
    BOOL    fCrashDump = FALSE;
    // Several function expect either a path or an empty string
    PSTR    pszSymPath = (PSTR) calloc(1, 1); // Allocate an empty string: ""
    CHAR    CrashDump[MAX_PATH*2] = {0};
    LPSTR   pszWindowTitle = NULL;
    BOOL    fSetTitle = FALSE;
    HDESK   hDesk;
    BOOL    bDll_CmdLineTL = FALSE;
    PSTR    pszName_CmdLineTL = NULL;
*/

    extern BOOL AutoTest;
    char    szProgramName[ MAX_PATH *2] = {0};
    char    rgch[ MAX_PATH ] = {0};
    char    szTitle[ MAX_MSG_TXT ] = {0};
    char    szClass[ MAX_MSG_TXT ] = {0};
    char    szWorkSpace[ MAX_PATH ] = {0};
    char	szWindowLayout[ MAX_PATH ] = {0};
    BOOL	bWindowLayoutSpecified	= FALSE;
    BOOL    bWorkSpaceSpecified     = FALSE;
    BOOL    bIgnoreDefault          = FALSE;
    BOOL    bWorkSpaceMissed        = FALSE;
    BOOL    bJustSource             = FALSE;
    BOOL    fGoNow                  = FALSE;
    BOOL    bKernelDebugging        = FALSE;
    BOOL    bInheritHandles         = FALSE;
    LPSTR   pszWorkSpaceToUse          = NULL;
    LONG    lAttachProcess          = -2;
    HANDLE  hEventGo                = NULL;
    LPSTR   lp1 = NULL;
    LPSTR   lp2 = NULL;
    BOOL    fRemoteServer = FALSE;
    BOOL    fMinimize = FALSE;
    CHAR    szPipeName[MAX_PATH+1] = {0};
    BOOL    fSymPath = FALSE;
    BOOL    fCrashDump = FALSE;
    // Several function expect either a path or an empty string
    PSTR    pszSymPath = (PSTR) calloc(1, 1); // Allocate an empty string: ""
    CHAR    szCrashDump[MAX_PATH*2] = {0};
    LPSTR   pszWindowTitle = NULL;
    BOOL    fSetTitle = FALSE;
    HDESK   hDesk;
    BOOL    bDll_CmdLineTL = FALSE;
    PSTR    pszName_CmdLineTL = NULL;
    BOOL    bLoadedDefaultWorkSpace = FALSE;
    MPT     mptPlatform = 0;
    DWORD   dwKernelDebuggingCommPort = 0;
    DWORD   dwKernelDebuggingBaudRate = 0;
    MPT     mptPlatformCmdLine = 0;

    PSTR    rgpszCmds[200] = {0};
    int     nNumCmds = 0;

    //
    //  First argument is debugger name
    //
    Assert( argc > 0 );
    strcpy( DebuggerName, argv[0] );

    //
    //  Initialize the debugger state. This state will very probably
    //  be overwritten later on by a workspace from the registry.
    //
    g_hInst  = hInstance;
    winVer = GetVersion();

    InitFileExtensions();
    InitializeDocument();
    InitCodemgr();

    Dbg(hMainAccTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(MAIN_ACC)));
    hCurrAccTable = hMainAccTable;
    Dbg(hCmdWinAccTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(CMD_WIN_ACC)));


    Dbg(LoadString(g_hInst, SYS_Main_wTitle, szTitle, MAX_MSG_TXT));
    Dbg(LoadString(g_hInst, SYS_Main_wClass, szClass, MAX_MSG_TXT));

    hEventIoctl = CreateEvent( NULL, TRUE, FALSE, NULL );

    InitializeCriticalSection( &csLog );

    //
    //  Create the frame
    //
    hwndFrame = CreateWindow(szClass, szTitle,
             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
             CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
             NULL, NULL, hInstance, NULL);

    //
    // Initialize the debugger
    //
    if ( !hwndFrame || !FDebInit() || !g_hwndMDIClient ) {
        return FALSE;
    }

    //
    //  Get Handle to main menu and to window submenu
    //
    Dbg( hMainMenu = GetMenu(hwndFrame));
    hMainMenuSave = hMainMenu;
    Dbg( hWindowSubMenu = GetSubMenu(hMainMenu, WINDOWMENU));

    //
    //  Build Help file path
    //
    Dbg(LoadString( g_hInst, SYS_Help_FileExt, szTmp, _MAX_EXT) );
    MakePathNameFromProg( (LPSTR)szTmp, szHelpFileName );

    //
    //  Init Items Colors ,Environment and RunDebug params to their default
    //  values 'They will later be overrided by the values in .INI file
    //  but we ensure to have something coherent even if we can't load
    //  the .INI file
    //
    InitDefaults();

    //
    //  Initialize Keyboard Hook
    //
    hKeyHook = SetWindowsHookEx( WH_KEYBOARD, (HOOKPROC)KeyboardHook,
                                 hInstance,   GetCurrentThreadId()    );

    //
    //  Parse arguments
    //
    LpszCommandLine = NULL;

    // skip whitespace
    lp1 = CPSkipWhitespace(GetCommandLine());

    // skip over program name
    if ('"' != *lp1) {
        lp1 += _tcslen(argv[0]);
    } else {
        // The program name is enclosed in double quotes.
        // +2 skip over the quotes at the beginning and end.
        Assert('"' == *(lp1 + _tcslen(argv[0]) + 1) );
        lp1 += _tcslen(argv[0]) + 2;
    }


    while (*lp1) {

        // eat whitespace before arg
        lp1 = CPSkipWhitespace(lp1);

        if (*lp1 == '-' || *lp1 == '/') {

            // pick up switches
            BOOL fSwitch = TRUE;

            ++lp1;

            while (*lp1 && fSwitch) {

                switch (*lp1++) {
                    case '?':
                    usage:
                        // Display the help topic for command line options
                        DisplayHelpContents(NULL, _WinDbg_Command_Line);
                        exit(1);
                        break;

                    case ' ':
                    case '\t':
                        fSwitch = FALSE;
                        break;

                    case 'A':
                        // Auto start a dlg
                        // Used by the wizard so that the open executable
                        //  dlg or the attach dlg, will appear as soon as
                        //  windbg is started.
                        //
                        // 'e' executable
                        // 'p' process
                        // 'd' dump file
                        // ie: Ae, Ap, Ad
                        if (isspace(*lp1)) {
                            goto usage;
                        } else {
                            if ('e' == *lp1) {
                                // Auto open the Open Executable dlg
                                PostMessage(hwndFrame,
                                            WM_COMMAND,
                                            MAKEWPARAM(IDM_FILE_OPEN_EXECUTABLE, 0),
                                            0);
                                lp1++;
                            } else if ('d' == *lp1) {
                                // Auto open the Attach to Process dlg
                                PostMessage(hwndFrame,
                                            WM_COMMAND,
                                            MAKEWPARAM(IDM_FILE_OPEN_CRASH_DUMP, 0),
                                            0);
                                lp1++;
                            } else if ('p' == *lp1) {
                                // Auto open the Attach to Process dlg
                                PostMessage(hwndFrame,
                                            WM_COMMAND,
                                            MAKEWPARAM(IDM_DEBUG_ATTACH, 0),
                                            0);
                                lp1++;
                            } else {
                                goto usage;
                            }
                        }
                        break;

                    case 'd':
                        iDebugLevel = atoi( GetArg(&lp1) );
                        fSwitch = FALSE;
                        break;

                    case 'e':
                        // signal an event after process is attached
                        hEventGo = (HANDLE)atol(GetArg(&lp1));
                        fSwitch = FALSE;
                        break;

                    case 'g':
                        // go now switch
                        fGoNow = TRUE;
                        break;

                    case 'h':
                        bInheritHandles = TRUE;
                        break;

                    case 'i':
                        bIgnoreDefault = TRUE;
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
                                LPSTR lpsz = "windbg -p %ld -e %ld";
                                lResult = RegSetValueEx(hkey, "Debugger", 0, REG_SZ,
                                    (PUCHAR) lpsz,
                                    strlen(lpsz) +1); // Need to include terminating 0

                                Assert(ERROR_SUCCESS == RegCloseKey(hkey));
                            }

                            if (ERROR_SUCCESS != lResult) {
                                // An error occurred, notify the user.
                                ErrorBox2(GetActiveWindow(), 0, ERR_Inst_Postmortem_Debugger);
                            } else {
                                InformationBox(DBG_Success_Inst_Postmortem_Debugger);
                                exit(0);
                            }
                        }
                        break;

                    case 'J':
                    case 'j':
                        // Allow journaling
                        // Already taken care of in main.
                        break;

                    case 'k':
                        //
                        // kernel debugging mode
                        //
                        bKernelDebugging = TRUE;
                        strcpy(szProgramName, NT_KERNEL_NAME);
                        lp2 = GetArg(&lp1);

                        // Set the platform type
                        if (!GetPlatformIdFromName(lp2,
                                                   &mptPlatformCmdLine
                                                   )) {
                            goto usage;
                        }

                        lp2 = GetArg(&lp1);
                        if (strlen(lp2) > 3 && _strnicmp(lp2,"com",3)==0) {
                            lp2 += 3;
                            dwKernelDebuggingCommPort  = strtoul(lp2, NULL, 0);
                        }
                        lp2 = GetArg(&lp1);
                        dwKernelDebuggingBaudRate = strtoul(lp2, NULL, 0);
                        break;

                    case 'l':
                        fSetTitle = TRUE;
                        lp2 = GetArg(&lp1);
                        if (strlen(lp2) >= MAX_PATH) {
                            lp2[MAX_PATH-1] = NULL;
                        }
                        pszWindowTitle = _strdup( lp2 );
                        break;

                    case 'm':
                        fMinimize = TRUE;
                        nCmdShow = SW_MINIMIZE;
                        break;

                    case 'n':
                        // BUGBUG - kcarlos - hack. Fix with new workspace code.
                        // Load this transport as the default.
                        // -n "tl name" as specified in the registry
                        pszName_CmdLineTL = _strdup( GetArg(&lp1) );
                        fSwitch = FALSE;
                        break;

                    case 'N':
                        //
                        // This command line is not supported 
                        //
                        bDll_CmdLineTL = TRUE;
                        GetArg(&lp1);
                        fSwitch = FALSE;
                        break;
            
                    case 'p':
                        // attach to an active process
                        // p specifies a process id
                        // pn specifies a process by name
                        // ie: -p 360 
                        //     -pn "foo bar"
                        if (!isspace(*lp1) && !isdigit(*lp1)) {
                            // They may have specified the -pn flag
                            if ('n' != *lp1 || OsVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) {
                                goto usage;
                            } else {
                                // Skip the 'n'
                                lp1++;

                                // Auto open the Open Executable dlg
                                char szProcessName[_MAX_FNAME + _MAX_EXT] = {0};
                                strncpy(szProcessName, GetArg(&lp1), sizeof(szProcessName)-1 );

                                if (!*szProcessName) {
                                    // They did not specify a name
                                    goto usage;
                                }

                                lAttachProcess = GetProcessIdGivenName(szProcessName);

                                if (lAttachProcess <= 0) {
                                    lAttachProcess = -2;
                                    ErrorBox2(hwndFrame, MB_TASKMODAL,
                                             ERR_Invalid_Process_Name,
                                             szProcessName);
                                }
                            }
                        } else {
                            // They speicified -p 360
                            lAttachProcess = strtoul(GetArg(&lp1), NULL, 0);
                         
                            if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                                if (lAttachProcess <= 0) {
                                    lAttachProcess = -2;
                                    ErrorBox2(hwndFrame, MB_TASKMODAL,
                                             ERR_Invalid_Process_Id,
                                             lAttachProcess);
                                }
                            }
                        }
                        fSwitch = FALSE;
                        break;

                    case 'r':
                        AutoRun = arCmdline;
                        PszAutoRun = _strdup( GetArg(&lp1) );
                        ShowWindow(hwndFrame, nCmdShow);
#if defined( NEW_WINDOWING_CODE )
                        New_OpenDebugWindow(CMD_WINDOW, FALSE); // Not user activated
#else
                        OpenDebugWindow(COMMAND_WIN, FALSE); // Not user activated
#endif
                        fSwitch = FALSE;
                        break;

                    case 'R':
                        {
                            //
                            // This code does not support nested dbl quotes.
                            //
                            BOOL bInDblQuotes = FALSE;
                            LPSTR lpszCmd;
                            LPSTR lpsz;

                            lpszCmd = lpsz = GetArg(&lp1);

                            while (*lpsz) {
                                if ('"' == *lpsz) {
                                    // We found a quote. We assume it is either an
                                    // opening quote or an ending quote, that's why
                                    // we toggle.
                                    bInDblQuotes = !bInDblQuotes;
                                } else {
                                    // We only replace the '_' if we are not inside
                                    // dbl quotes.
                                    if (!bInDblQuotes && '_' == *lpsz) {
                                        *lpsz = ' ';
                                    }
                                }
                                lpsz = CharNext(lpsz);
                            }

                            if ( nNumCmds < sizeof(rgpszCmds)/sizeof(rgpszCmds[0]) ) {
                                rgpszCmds[nNumCmds++] = _strdup(lpszCmd);
                            }
                            fSwitch = FALSE;
                        }
                        break;

                    case 's':
                        fRemoteServer = TRUE;
                        lp2 = GetArg(&lp1);
                        if (lp2 && *lp2) {
                           strncpy(szPipeName, lp2, sizeof(szPipeName));
                           szPipeName[sizeof(szPipeName)-1] = NULL;
                        } else {
                           ErrorBox2(hwndFrame, 
                                     MB_TASKMODAL,
                                     ERR_No_Remote_Pipe_Name
                                     );
                           fRemoteServer = FALSE;
                        }
                        break;

                    case 't':
                        NoPopups = TRUE;
                        AutoTest = TRUE;
                        break;

                    case 'w':
                        bWorkSpaceSpecified = TRUE;
                        strncpy( szWorkSpace, GetArg(&lp1), sizeof(szWorkSpace)/sizeof(TCHAR) );
                        szWorkSpace[_tsizeof(szWorkSpace)-1] =NULL;
                        fSwitch = FALSE;
                        break;
					
					case 'x':
						// window layout
                        bWindowLayoutSpecified = TRUE;
                        strncpy(szWindowLayout, 
								GetArg(&lp1), 
								_tsizeof(szWorkSpace)
								);
                        szWindowLayout[_tsizeof(szWindowLayout)-1] =NULL;
                        fSwitch = FALSE;
                        break;


                    case 'y':
                        Assert(pszSymPath);
                        Assert(NULL == *pszSymPath);

                        free(pszSymPath);
                        pszSymPath = NULL;
                        
                        fSymPath = TRUE;
                        lp2 = GetArg(&lp1);
                        if (!TruncatePathsToMaxPath(lp2, pszSymPath)) {
                            // No truncations were made. Nothing was allocated.
                            // We need to duplicate the string ourselves
                            pszSymPath = _strdup(lp2);
                        }
                        break;

                    case 'z':
                        fCrashDump = TRUE;
                        lp2 = GetArg(&lp1);
                        strncpy(szCrashDump, lp2, sizeof(szCrashDump)/sizeof(TCHAR));
                        szCrashDump[sizeof(szCrashDump)/sizeof(TCHAR) -1] = NULL;
                        break;

                    default:
                        --lp1;
                        goto usage;
                }
            }

        } else {

            // pick up file args.  If it is a program name,
            // keep the tail of the cmd line intact.
            char *lpSave = lp1;

            AnsiUpper( lp2 = GetArg(&lp1) );

            if (!*lp2) {

                continue;

            } else {

                TCHAR szDrive[_MAX_DRIVE];
                TCHAR szDir[_MAX_DIR];
                TCHAR szFName[_MAX_FNAME];
                TCHAR szExt[_MAX_EXT];

                _fullpath(rgch, lp2, sizeof(rgch));
                _splitpath(rgch, szDrive, szDir, szFName, szExt);

                if ( !_stricmp( szExt, szStarDotExe+1 ) ||
                     !_stricmp( szExt, szStarDotCom+1 ) ||
                      ( *szExt == '\0' )  )
                {
                    //
                    // Program name. Get the command line.
                    //
                    strcpy( szProgramName, GetArg(&lpSave));
                    if (*lp1) {
                        // it must be a space or tab...
                        // if it is a tab, we are wrong...
                        ++lp1;
                    }
                    LpszCommandLine = _strdup(lp1);
                    break;

                } else {

                    //ShowWindow(hwndFrame, SW_SHOWNORMAL);

                    //
                    // Source file name
                    //
                    AddFile( MODE_OPENCREATE,
                             DOC_WIN,
                             rgch,
                             NULL,
                             NULL,
                             FALSE,
                             -1,
                             -1,
                             TRUE
                            );
                    bJustSource = TRUE;
                    bIgnoreDefault = TRUE;
                }
            }
        }
    }

    //
    // Boost thread priority to improve performance.
    //
    if (!AutoTest) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    }

    if ( bIgnoreDefault && bWorkSpaceSpecified ) {
        ErrorBox2( hwndFrame, MB_TASKMODAL,
                   ERR_Invalid_Command_Line_File,
                   (LPSTR)"" );
    }

    //
    // Let's load the workspace
    //
    if (fCrashDump) {
        if (!InitCrashDump( szCrashDump )) {
            ErrorBox2(hwndFrame, MB_TASKMODAL,
                      ERR_Invalid_Crashdump_File,
                      szCrashDump);
            exit(1);
        } else {
            ModListSetSearchPath( pszSymPath );
            fGoNow = TRUE;
            if (g_contWorkspace_WkSp.m_bUserCrashDump) {
                strcpy( szProgramName, NT_USERDUMP_NAME );
            } else {
                strcpy( szProgramName, NT_KERNELDUMP_NAME );
            }
        }
    }

    if (*szProgramName) {
        // A program was specified
        g_Windbg_WkSp.SetCurrentProgramName(szProgramName);
    }

    if ( !bIgnoreDefault ) {

        if (bWorkSpaceSpecified) {
            g_contWorkspace_WkSp.SetRegistryName(szWorkSpace);
        }

    } else {

        // We are finished. We have loaded with default values ignoring 
        // anything stored in the registry.
        //
        // force cmdwin

        CWorkSpace_WKSP *pTmp = new CWorkSpace_WKSP;

        g_contWorkspace_WkSp.Duplicate( *pTmp );

        delete pTmp;

#if defined( NEW_WINDOWING_CODE )
        New_OpenDebugWindow(CMD_WINDOW, FALSE); // Not user activated
#else
        OpenDebugWindow(COMMAND_WIN, FALSE); // Not user activated
#endif

    }

	//
	// Set the name of the window layout we want to load.
	//
    if (bWindowLayoutSpecified) {
		g_contWinLayout_WkSp.SetRegistryName(szWindowLayout);
	}

    //
    // Read of all the debuggees and workspaces
    //
    g_Windbg_WkSp.Restore(FALSE);


    //
    // Execute cmds from command line
    //
    {
        for (int nIdx=0; nIdx < nNumCmds; nIdx++) {
            
            CmdExecuteLine( rgpszCmds[nIdx] );

            FREE_STR( rgpszCmds[nIdx] );
        }
    }


    if (bKernelDebugging) {
        g_contWorkspace_WkSp.m_bKernelDebugger = TRUE;

        g_contKernelDbgPreferences_WkSp.m_mptPlatform = mptPlatformCmdLine;
        g_contKernelDbgPreferences_WkSp.m_dwPort = dwKernelDebuggingCommPort;
        g_contKernelDbgPreferences_WkSp.m_dwBaudRate = dwKernelDebuggingBaudRate;

        if (fRemoteServer) {
            g_contGlobalPreferences_WkSp.m_bCommandRepeat = TRUE;
        }
        if (!bWorkSpaceSpecified) {
            g_contWorkspace_WkSp.m_bMasmEval = TRUE;
        }
    }


    // Autorun has already displayed the window
    if (arNone == AutoRun) {
        if (fMinimize) {
            // BUGBUG - kcarlos - store nCmdShow into the workspace
            ShowWindow(hwndFrame, SW_MINIMIZE);
            UpdateWindow(hwndFrame);
        }        
    }

    if (bInheritHandles) {
        g_contWorkspace_WkSp.m_bInheritHandles = TRUE;
    }

    if (fSymPath) {
        ModListSetSearchPath( pszSymPath );
    }

    if (fSetTitle) {

        FREE_STR(g_contWorkspace_WkSp.m_pszWindowTitle);
        g_contWorkspace_WkSp.m_pszWindowTitle = _strdup(pszWindowTitle);
        
        SetWindowText( hwndFrame, pszWindowTitle );
    }

    CheckMenuItem(hMainMenu, 
                  IDM_WINDOW_SOURCE_OVERLAY,
                  FSourceOverlay ? MF_CHECKED : MF_UNCHECKED 
                  );

    GetComPorts();

    // If the user specified the -n or -N switch
    // on the command line, initialize the default transport to
    // that string.
    if (bDll_CmdLineTL || pszName_CmdLineTL) {
        if (bDll_CmdLineTL && pszName_CmdLineTL) {
            // The idiot specified both.
            ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_TL_Both_Overrides);
        } else {
            // You cannot override the TL when debugging kernel or crash dumps
            if (g_contWorkspace_WkSp.m_bKernelDebugger || g_contWorkspace_WkSp.m_bUserCrashDump) {
                // fKernelDebugger cover kernel and kernel dumps
                ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_TL_Cannot_Be_Overridden);
            } else if (bDll_CmdLineTL) {
                ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_TL_Cmd_Line_Opt_N);
            } else {
                Assert(pszName_CmdLineTL);
                SetTransportLayer(pszName_CmdLineTL, &g_dynacontAll_TLs_WkSp);
            }
        }
    }

    if (g_contWorkspace_WkSp.m_bLfOptAuto) {
        LogFileOpen(g_contWorkspace_WkSp.m_pszLogFileName, g_contWorkspace_WkSp.m_bLfOptAppend);
    }

    if (fRemoteServer) {
        StartRemoteServer( szPipeName, FALSE );
    }

    if (fGoNow) {
        if (g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
            RestartDebuggee( NT_KERNEL_NAME, NULL );
        } else if (lAttachProcess == -2 || *szProgramName) {
            ExecDebuggee(EXEC_GO);
        }
    }

    if (arCmdline == AutoRun) {
        if (CmdAutoRunInit() == FALSE) {
            ErrorBox( ERR_File_Open, PszAutoRun );
            exit(1);
        }
    }

    if (lAttachProcess != -2) {
        if (!AttachDebuggee(lAttachProcess, hEventGo)) {
            CmdLogVar(ERR_Attach_Failed);
        } else {

            // stopped at ldr BP in bogus thread:
            LptdCur->fGoOnTerm = TRUE;

            if (fGoNow || hEventGo || g_contWorkspace_WkSp.m_bAttachGo) {
                Go();
            } else if (!DoStopEvent(NULL)) {
                CmdLogVar(DBG_Attach_Stopped);
            }

            if (hEventGo) {
                hDesk = GetThreadDesktop(GetCurrentThreadId());
                if (hDesk) {
                    SwitchDesktop(hDesk);
                }
            }
        }
    }

    if (pszSymPath) {
        free(pszSymPath);
    }

    return TRUE;
}


void
InitFileExtensions(
    void
    )
/*++

Routine Description:

    Load the standard file extensions into strings

Arguments:

    None

Return Value:

    None

--*/
{
    // Standard File Extensions

    Dbg(LoadString(g_hInst, DEF_Ext_C, szStarDotC, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_H, szStarDotH, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_CPP, szStarDotCPP, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_CXX, szStarDotCXX, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_EXE, szStarDotExe, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_COM, szStarDotCom, _MAX_EXT));
    Dbg(LoadString(g_hInst, DEF_Ext_ALL, szStarDotStar, _MAX_EXT));

    return;
}                   /* InitFileExtensions() */


VOID
InitDefaultFont(
    void
    )
{
    //Set a default font

    CHARSETINFO csi;
    DWORD dw = GetACP();

    if (!TranslateCharsetInfo((DWORD*)dw, &csi, TCI_SRCCODEPAGE)) {
        csi.ciCharset = ANSI_CHARSET;
    }

    g_logfontDefault.lfHeight = 10;
    g_logfontDefault.lfWidth = 0;
    g_logfontDefault.lfEscapement = 0;
    g_logfontDefault.lfOrientation = 0;
    g_logfontDefault.lfWeight = FW_NORMAL;
    g_logfontDefault.lfItalic = 0;
    g_logfontDefault.lfUnderline = 0;
    g_logfontDefault.lfStrikeOut = 0;
    /* use appropriate Character set font as default */
    g_logfontDefault.lfCharSet = (BYTE)csi.ciCharset;

    /* Set device independent font size */
    {
        HDC         hDC;
        int         nLogPixY;
        int         nHeight;
        int     nPoints = 100;  //Point size * 10

        hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        nLogPixY = GetDeviceCaps(hDC, LOGPIXELSY);
        nHeight = (nPoints / 10) * nLogPixY;
        if(nPoints % 10){
            nHeight += MulDiv(nPoints % 10, nLogPixY, 10);
        }
        g_logfontDefault.lfHeight = MulDiv(nHeight, -1, 72);
        DeleteDC(hDC);
    }
    g_logfontDefault.lfOutPrecision = OUT_DEFAULT_PRECIS;
    g_logfontDefault.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    g_logfontDefault.lfQuality = DEFAULT_QUALITY;
    g_logfontDefault.lfPitchAndFamily = FIXED_PITCH;
    lstrcpy((LPSTR) g_logfontDefault.lfFaceName, (LPSTR)"Terminal");

    return;
}                   /* InitDefaultFont() */


VOID
InitDefaultFindReplace(
    void
    )
{
    int i, j;

    //Initialize Replace/Find structure

    findReplace.matchCase  = FALSE;
    findReplace.regExpr = FALSE;
    findReplace.wholeWord = FALSE;
    findReplace.goUp = FALSE;
    findReplace.findWhat[0] = '\0';
    findReplace.replaceWith[0] = '\0';
    for (j = FIND_PICK; j <= REPLACE_PICK; j++) {
        findReplace.nbInPick[j];
        for (i = 0; i < MAX_PICK_LIST; i++)
        {
            findReplace.hPickList[j][i] = 0;
        }
    }
    frMem.leftCol = 0;
    frMem.rightCol = 0;
    frMem.line = 0;
    frMem.stopLine = 0;
    frMem.stopCol = 0;
    frMem.hDlgFindNextWnd = NULL;
    frMem.hDlgConfirmWnd = NULL;
    frMem.allTagged = FALSE;
    frMem.replacing = FALSE;
    frMem.replaceAll = FALSE;

    return;
}                   /* InitDefaultFindReplace() */


void NEAR PASCAL
InitTitleBar(
    void
    )
{
    Dbg(LoadString(g_hInst, SYS_Main_wTitle, (LPSTR)TitleBar.ProgName, sizeof(TitleBar.ProgName)));
    TitleBar.UserTitle[0] = '\0';
    TitleBar.ModeWork[0] = '\0';
    Dbg(LoadString(g_hInst, TBR_Mode_Run, (LPSTR)TitleBar.ModeRun, sizeof(TitleBar.ModeRun)));
    Dbg(LoadString(g_hInst, TBR_Mode_Break, (LPSTR)TitleBar.ModeBreak, sizeof(TitleBar.ModeBreak)));
    TitleBar.Mode = TBM_WORK;
}


BOOL
InitDefaultDlls(
    VOID
    )
/*++

Routine Description:

    Initializes the default workspace with some hard-coded values.

Arguments:

    None

Return Value:

    BOOL    -   TRUE if successfully initialized

--*/
{
    char    szKeyBuf[MAX_PATH];

    ModListInit();

    ModListAdd( NT_KERNEL_NAME, sheNone );
    ModListAdd( NT_KRNLMP_NAME, sheNone );

    ModListSetDefaultShe( sheDeferSyms );
    ModListAddSearchPath( "%SystemRoot%\\symbols" );

    SrcFileDirectory[0] = '\0';
    ExeFileDirectory[0] = '\0';
    DocFileDirectory[0] = '\0';
    UserDllDirectory[0] = '\0';

    return TRUE;
}


void FAR PASCAL
InitDefaults(
    void
    )
{
    UpdateRadix( 16 );
    fCaseSensitive = FALSE;
   
    InitDefaultFindReplace();
    InitDefaultFont();
    InitTitleBar();

    SetSrcMode_StatusBar(TRUE);

    InitDefaultDlls();

    return;
}                   /* InitDefaults() */
