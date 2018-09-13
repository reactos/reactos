/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Windbg.c

Abstract:

    This module contains the main program, main window proc and MDICLIENT
    window proc for Windbg.

Author:

    David J. Gilman (davegi) 21-Apr-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


#include <ime.h>


extern HWND GetWatchHWND(void);


extern BOOL InitCrashDump(LPSTR CrashDump);


// Path of the last Src file opened from a file open dlg box
char g_szMRU_SRC_FILE_PATH[_MAX_PATH];


extern void arrange(void);    //IDM_WINDOW_ARRANGE

BOOL bChildFocus = FALSE; // module wide flag for swallowing mouse messages


enum {
    STARTED,
    INPROGRESS,
    FINISHED
};


unsigned int InMemUpdate = FINISHED; // prevent multiple viemem() calls
extern WORD DialogType;

int
MainExceptionFilter(
    LPEXCEPTION_POINTERS lpep
    )
{
    char sz[1000];
    int r;
    PEXCEPTION_RECORD per = lpep->ExceptionRecord;
    switch (per->ExceptionCode) {

    default:
        r = EXCEPTION_CONTINUE_SEARCH;
        break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:

        sprintf(sz,
            "Exception 0x%08X occurred at address 0x%016p.\nHit OK to exit, CANCEL to hit a breakpoint.",
            per->ExceptionCode,
            per->ExceptionAddress);

        r = MsgBox(NULL,
            sz,
            MB_OKCANCEL | MB_ICONSTOP | MB_SETFOREGROUND | MB_TASKMODAL);

        if (r == IDOK) {
            r = EXCEPTION_CONTINUE_SEARCH;
        } else {
            DebugBreak();
            r = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    }
    return r;
}

void
MungeProcessorAffinity()
{
    // Are we running NT?
    if (VER_PLATFORM_WIN32_NT != OsVersionInfo.dwPlatformId) {
        // If not on NT, nothing to do
        return;
    }

    HANDLE hCurProc = GetCurrentProcess();
    DWORD_PTR dwProcMask = 0;
    DWORD_PTR dwSysMask = 0;
    if (!GetProcessAffinityMask(hCurProc, &dwProcMask, &dwSysMask)) {
        Dbg(!"Ignorable warning. Unable to GET the process affinity mask.");
        OutputDebugString("Ignorable warning. Unable to GET the process affinity mask.\n");
        return;
    }


    //
    // We dynamically get the pointer because win95 does not implement this
    // function, and won't run on win95 boxes
    //
    {
        typedef BOOL (WINAPI * PFNSETPROCESSAFFINITYMASK)(HANDLE, DWORD);

        HMODULE hmod = GetModuleHandle("kernel32");
        Assert(hmod);

        PFNSETPROCESSAFFINITYMASK pfnSetProcessAffinityMask =
            (PFNSETPROCESSAFFINITYMASK) GetProcAddress(hmod, "SetProcessAffinityMask");
        Assert(pfnSetProcessAffinityMask);

        // Set the affinity to the first allowed processor
        DWORD dwMask = 1;
        for (int i=0; i < sizeof(dwMask)*8; i++, dwMask<<=1) {
            if (dwMask & dwProcMask) {
                if (pfnSetProcessAffinityMask(hCurProc, dwMask)) {
                    // Success
                    return;
                }
                // Failed, keep trying
            }
        }
    }

    DAssert(!"Ignorable warning. Unable to SET the process affinity mask\n.");
}


int
WINAPIV
main(
     int argc,
     char* argv[ ],
     char* envp[]
     )

/*++

Routine Description:

    description-of-function.

Arguments:

    argc - Supplies the count of arguments on command line.

    argv - Supplies a pointer to an array of string pointers.

Return Value:

    int - Returns the wParam from the WM_QUIT message.
    None.

--*/

{
#define RST_DONTJOURNALATTACH 0x00000002
    typedef VOID (WINAPI * RST)(DWORD,DWORD);
    RST Rst;
    BOOL bDisableJournaling = TRUE;

    MSG     msg;
    int     i, nCmdShow;
    HMODULE hInstance = GetModuleHandle(NULL);
    STARTUPINFO Startup;

#define hPrevInstance 0

    if (!WKSP_Initialize()) {
        FatalErrorBox(ERR_Unable_To_Initialize_WorkSpaces, NULL);
        return FALSE;
    }

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    GetVersionEx( &OsVersionInfo );

#if 0
    // 
    // Disable this, since it does not allow child processes, namely
    // the debuggee, to run on multiple processors. Also, all of the
    // race condition should have been fixed by now.
    //
    MungeProcessorAffinity();
#endif

    InitCommonControls();

    SetErrorMode( SEM_NOALIGNMENTFAULTEXCEPT );

    //
    //
    // Should journaling be allowed or disabled?
    //

    // Scan for the "-j" "/j" flag.
    // Skip the first arg which is itself.
    for (i = 1; i<argc; i++) {
        if (!_stricmp("/j", argv[i]) || !_stricmp("-j", argv[i])
            || strncmp("/r", argv[i], 2) || strncmp("-r", argv[i], 2)) {

            // Allow journaling
            bDisableJournaling = FALSE;
        }
    }

    if (bDisableJournaling) {
        // Disable journaling
        Rst = (RST)GetProcAddress( GetModuleHandle( "user32.dll" ), "RegisterSystemThread" );
        if (Rst) {
            (Rst) (RST_DONTJOURNALATTACH, 0);
        }
    }

    GetStartupInfo (&Startup);

    nCmdShow = Startup.wShowWindow;

    // First of all, load the title and our standard low memory message

    if (!LoadString(hInstance, SYS_Main_wTitle, MainTitleText, sizeof(MainTitleText)) ||
        !LoadString(hInstance, ERR_Memory_Is_Low, LowMemoryMsg, sizeof(LowMemoryMsg)) ||
        !LoadString(hInstance, ERR_Memory_Is_Low_2, LowMemoryMsg2, sizeof(LowMemoryMsg2))) {
        return FALSE;
    }

    //Clear the terminal screen
    for (i = 0; i < 12; i++) {
        AuxPrintf(1, (LPSTR)"");
    }

    if (!hPrevInstance) {
        if (!InitApplication(hInstance)) {
            FatalErrorBox(ERR_Init_Application, NULL);
            return FALSE;
        }
    }

    if (!InitInstance(argc, argv, hInstance, nCmdShow)) {
        FatalErrorBox(ERR_Init_Application, NULL);
        return FALSE;
    }

    PostMessage((HWND) -1, RegisterWindowMessage("XXXYYY"), 0, 0);

    __try {
        //Enter main message loop
        while (GetMessage (&msg, NULL, 0, 0)) {
            ProcessQCQPMessage(&msg);
        }
    } __except(MainExceptionFilter(GetExceptionInformation())) {
        DAssert(FALSE);
    }


    ExitProcess ((UINT) msg.wParam);

    // Keep the C++ compiler from whining
    return 0;
}

/***    TerminateApplication
**
**  Synopsis:
**  bool = TerminateApplication(hWnd, wParam)
**
**  Entry:
**  hWnd
**  wParam
**
**  Returns:
**
**  Description:
**
*/

BOOL
NEAR
TerminateApplication(
                     HWND hwnd,
                     WPARAM wParam
                     )
{
    TLIS tl_info = {0};
    extern HTL Htl; // Handle to the current transport layer

    ExitingDebugger = TRUE;

    //
    // Destroy modeless find/replace dialog boxes
    //
    frMem.exitModelessFind = TRUE;
    frMem.exitModelessReplace = TRUE;

    //
    // don't fetch more commands
    //
    SetAutoRunSuppress(TRUE);

    //
    // Stop Help Engine
    //
    Dbg(WinHelp(hwnd, szHelpFileName, HELP_QUIT, 0));

    //
    // For Microsoft Tests suites
    //
    if (wParam != (WPARAM)-1) {
        if (g_Windbg_WkSp.ShuttingDown()) {
            ExitingDebugger = FALSE;
            return FALSE;
        }
    }

    //
    // If the TL is remote, then simply exit
    //
    if (Htl) {
        if (xosdNone != OSDTLGetInfo(Htl, &tl_info)) {
            // If we can't tell, then we should just exit to
            // avoid hanging.
            goto done;
        } else {
            if (tl_info.fRemote) {
                // If the TL is remote, we should just exit to
                // avoid hanging.
                goto done;
            }
        }
    }

    if (g_contWorkspace_WkSp.m_bDisconnectOnExit) {

        if (LppdCur) {
            if (LptdCur) {
                OSDDisconnect( LppdCur->hpid, LptdCur->htid );
            } else {
                OSDDisconnect( LppdCur->hpid, NULL );
            }
        }

    } else {

        // Unload and clean out the debugger.   Don't do exit if the
        // debugger is in the "loading an exe" state.

        if (DbgState != ds_normal) {
            MessageBeep(0);
            return FALSE;
        }

        if (DebuggeeActive() && !AutoTest) {
            CmdInsertInit();
            CmdLogFmt("Debuggee still active on Exit\r\n");
        }


        FDebTerm();

    }

done:
    TerminatedApp = TRUE;

    return TRUE;
}                   /* TerminateApplication() */


void
DisplayHelpContents(
    HWND    hwnd,
    DWORD   dwHelpContextId
    )
/*++

Routine Description

    Calls the winhelp file to display the contents.

    The reason that this call is inside a function is
    that it is also called when you type in windbg /?.

Arguments:

    hwnd - Supplies parent for help window. May be NULL.

    dwHelpContextId - If 0, then table of contents is displayed.
            If not 0, then a specific help topic is displayed.

Return Value:

    None

--*/
{
    if (dwHelpContextId) {
        // Display a specific help topic
        Dbg(WinHelp(hwnd, szHelpFileName, HELP_CONTEXT, dwHelpContextId));
    } else {
        // Display the table of contents
        Dbg(WinHelp(hwnd, szHelpFileName, HELP_FINDER, 0));
    }
}


/***    MainWndProc
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**  Processes window messages.
**
*/


LRESULT
CALLBACK
MainWndProc(
            HWND  hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
            )
{
    LPVIEWREC v = NULL;
    static UINT     menuID;
    static  BOOL    bOldImeStatus;

    switch (message) {
    case WM_CREATE:
        {
            CLIENTCREATESTRUCT ccs;
            char szClass[MAX_MSG_TXT];

            ImeInit();

            //Find window menu where children will be listed
            ccs.hWindowMenu = GetSubMenu(GetMenu(hwnd), WINDOWMENU);
            ccs.idFirstChild = IDM_FIRSTCHILD;

            //Create the MDI client filling the client area
            g_hwndMDIClient = CreateWindow((LPSTR)"mdiclient",
                NULL,
                WS_CHILD | WS_CLIPCHILDREN, //MDIS_ALLCHILDSTYLES,
                0, 0, 0, 0,
                hwnd, (HMENU) 0xCAC, g_hInst, (LPSTR)&ccs);
            Dbg(g_hwndMDIClient);

            //
            // Nothing interesting, here, just
            // trying to turn the Toolbar & Status bar into a
            // black box, so that the variables, etc. aren't
            // scattered all over the place.
            //

            // Create the Toolbar.
            WindbgCreate_Toolbar(hwnd);

            //Create Status Bar
            WindbgCreate_StatusBar(hwnd);

            LoadFonts(hwnd);

            ShowWindow(g_hwndMDIClient, SW_SHOW);
            InitializeMenu(GetMenu(hwnd));

            hmenuMain = GetMenu(hwnd);
            Assert(hmenuMain);
            hmenuCmdWin = LoadMenu(g_hInst, MAKEINTRESOURCE(CMD_WIN_MENU));
            Assert(hmenuCmdWin);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;

            switch (lpnmhdr->code) {
            case TTN_NEEDTEXT:
                {
                    LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT) lParam;

                    lpToolTipText->lpszText = GetToolTipTextFor_Toolbar((UINT) lpToolTipText->hdr.idFrom);
                }
                break;
            }
        }
        break;

    case WM_QUERYOPEN:
        if (checkFileDate) {
            checkFileDate = FALSE;
            PostMessage(hwndFrame, WM_ACTIVATEAPP, 1, 0L);
        }
        goto DefProcessing;

    case WM_GETTEXT:
        BuildTitleBar((LPSTR)lParam, (UINT) wParam);
        return strlen((LPSTR)lParam);

    case WM_SETTEXT:
        {
            char szTitleBar[256];

            if (lParam && *(LPSTR)lParam) {
                strncpy( TitleBar.UserTitle, (LPSTR)lParam, sizeof(TitleBar.UserTitle) );
                TitleBar.UserTitle[sizeof(TitleBar.UserTitle) -1] = NULL;
            }

            BuildTitleBar(szTitleBar, sizeof(szTitleBar));
            return DefWindowProc(hwnd, message, wParam, (LPARAM)(LPSTR)szTitleBar);
        }


    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code
            WORD wItemId = LOWORD(wParam);         // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;      // handle of control

            if (hwndActiveEdit) {
                v = &Views[curView];
            }

            switch (wItemId) {

            case IDM_DEBUG_CTRL_C:
                DispatchCtrlCEvent();
                break;

            case IDM_TEST_NEW_WND:
#if defined( NEW_WINDOWING_CODE )                
                {
                    HWND hwndNewCmd = NewCmd_CreateWindow(g_hwndMDIClient);
                    Dbg(hwndNewCmd);
                }
#endif
                break;

            case IDM_FILE_OPEN_EXECUTABLE:
                {
                    static char szExeName[MAX_PATH] = { 0 };
                    DWORD dw = OFN_EXPLORER | OFN_HIDEREADONLY
                        | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (0 == *szExeName) {
                        GetCurrentDirectory(sizeof(szExeName), szExeName);
                        if ('\\' != szExeName[strlen(szExeName)-1]) {
                            strcat(szExeName, "\\");
                        }
                        strcat(szExeName, "*.exe");
                    }

                    if (StartFileDlg(
                        hwnd,                        // parent
                        DLG_Browse_Executable_Title,    // Title
                        DEF_Ext_EXE,                    // Extension
                        IDM_FILE_OPEN,                  // kcarlos BUGBUG: Need to get its own helpID
                        IDD_DLG_FILEOPEN_EXPLORER_EXTENSION_EXE_ARGS,  // template
                        szExeName,                             // Path/file
                        &dw,                            // Flags
                        OpenExeWithArgsHookProc)) {

                        Dbg(strlen(szExeName) < sizeof(szExeName));

                        // If it's an empty string, then we do nothing
                        if (*szExeName) {
                            extern char szOpenExeArgs[_MAX_PATH];
                            LPSTR lpszExeName = szExeName, lpszOpenExeArgs = szOpenExeArgs;

                            lpszExeName = CPSkipWhitespace(lpszExeName);
                            lpszOpenExeArgs = CPSkipWhitespace(lpszOpenExeArgs);

                            if (LpszCommandLine) {
                                free(LpszCommandLine);
                            }
                            LpszCommandLine = _strdup(lpszOpenExeArgs);

                            if (0 == strlen(lpszOpenExeArgs)) {
                                // empty string. Just pass a NULL to LogStartWithArgs.
                                lpszOpenExeArgs = NULL;
                            }

                            LogStartWithArgs(lpszExeName, lpszOpenExeArgs);
                        }
                    }
                }
                break;

            case IDM_FILE_OPEN_CRASH_DUMP:
                {
                    static char     sz[MAX_PATH] = { 0 };
                    char            szCrashDump[MAX_PATH];
                    DWORD           dw = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    *szCrashDump = 0;

                    if (0 == *sz) {
                        GetWindowsDirectory(sz, sizeof(sz));
                        if ('\\' != sz[strlen(sz)-1]) {
                            strcat(sz, "\\");
                        }
                        strcat(sz, "*.dmp");
                    }

                    if (StartFileDlg(
                        hwnd,                        // parent
                        DLG_Browse_CrashDump_Title,    // Title
                        DEF_Ext_DUMP,                    // Extension
                        IDM_FILE_OPEN,                  // kcarlos BUGBUG: Need to get its own helpID
                        0,                              // No temnplate
                        sz,                             // Path/file
                        &dw,                            // Flags
                        NULL)) {

                        Dbg(strlen(sz) < sizeof(sz));

                        strcpy(szCrashDump, sz);

                        DWORD dwSrchPathSize = ModListGetSearchPath(NULL, 0);
                        PSTR pszSymPath = new char[dwSrchPathSize];

                        Assert(pszSymPath);

                        // Actually get a copy.
                        ModListGetSearchPath(pszSymPath, dwSrchPathSize);

                        if (!InitCrashDump(szCrashDump)) {
                            ErrorBox2( hwndFrame, MB_TASKMODAL,
                                ERR_Invalid_Crashdump_File,
                                szCrashDump);
                        } else {
                            if (g_contWorkspace_WkSp.m_bUserCrashDump) {
                                g_Windbg_WkSp.SetCurrentProgramName(NT_USERDUMP_NAME);
                            } else {
                                g_Windbg_WkSp.SetCurrentProgramName(NT_KERNELDUMP_NAME);
                            }

                            if (g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
                                RestartDebuggee( NT_KERNEL_NAME, NULL );
                            }
                        }

                        delete [] pszSymPath;
                    }
                }
                break;

            case IDM_FILE_MRU_FILE1:
            case IDM_FILE_MRU_FILE2:
            case IDM_FILE_MRU_FILE3:
            case IDM_FILE_MRU_FILE4:
            case IDM_FILE_MRU_FILE5:
            case IDM_FILE_MRU_FILE6:
            case IDM_FILE_MRU_FILE7:
            case IDM_FILE_MRU_FILE8:
            case IDM_FILE_MRU_FILE9:
            case IDM_FILE_MRU_FILE10:
            case IDM_FILE_MRU_FILE11:
            case IDM_FILE_MRU_FILE12:
            case IDM_FILE_MRU_FILE13:
            case IDM_FILE_MRU_FILE14:
            case IDM_FILE_MRU_FILE15:
            case IDM_FILE_MRU_FILE16:
            case IDM_FILE_OPEN:
                {
                    TCHAR szPath[_MAX_PATH];

                    if (IDM_FILE_OPEN == wItemId) {
                        // Opening a file using the file open dlg
                        DWORD dwFlags;

                        dwFlags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
                            | OFN_EXPLORER | OFN_HIDEREADONLY;

                        // Make sure the open dlg is in the correct location
                        strcpy(szPath, g_szMRU_SRC_FILE_PATH);

                        if (!StartFileDlg(hwnd, DLG_Open_Filebox_Title, DEF_Ext_SOURCE,
                            IDM_FILE_OPEN, 0, (LPSTR) szPath, &dwFlags, DlgFile)) {

                            // User canceled, bail out
                            break;
                        }
                    } else {
                        PSTR psz;
                        WORD wFileIdx = wItemId - IDM_FILE_MRU_FILE1;

                        // Sanity check
                        Dbg(wFileIdx < nbFilesKept[EDITOR_FILE]);

                        //  Look for the MRU program in the list.
                        //
                        Dbg(psz = (PSTR) GlobalLock(hFileKept[EDITOR_FILE][wFileIdx]));
                        strcpy(szPath, psz);
                        Dbg(GlobalUnlock (hFileKept[EDITOR_FILE][wFileIdx]) == FALSE);
                    }

                    AddFile(MODE_OPENCREATE,
                        DOC_WIN,
                        szPath,
                        NULL,
                        NULL,
                        TRUE,
                        -1, -1, TRUE);

                    // copy the path back to our global variable.
                    {
                        char szDrive[_MAX_DRIVE];
                        char szDir[_MAX_DIR];

                        _splitpath(szPath, szDrive, szDir, NULL, NULL);

                        _makepath(g_szMRU_SRC_FILE_PATH, szDrive, szDir, NULL, NULL);
                    }

                    // Remove any trailing '.' from the name
                    {
                        int i = strlen(szPath);
                        if (szPath[i-1] == '.') {
                            szPath[i-1] = 0;
                        }
                    }
                }
                break;

            case IDM_FILE_CLOSE:
                {
                    int i;
                    int curDoc = Views[curView].Doc; //curView may change during loop
                    BOOL closeIt = TRUE;

// BUGBUG - dead code - kcarlos
#if 0
                    if (curDoc >= 0 &&
                        Docs[curDoc].docType == DOC_WIN && Docs[curDoc].ismodified) {

                        //Ask user whether to save / not save / cancel
                        switch (QuestionBox(SYS_Save_Changes_To, MB_YESNOCANCEL ,
                            Docs[curDoc].szFileName)) {

                        case IDYES:

                            //User wants file saved
                            if (SaveFile(curDoc)) {
                                //Reset file creation/load/save time
                                GetFileTimeByName( Docs[curDoc].szFileName,
                                    NULL,
                                    NULL,
                                    &Docs[curDoc].time
                                    );
                                Docs[curDoc].ismodified = FALSE;
                            } else {
                                closeIt = FALSE;
                            }
                            break;

                        case IDNO:

                            FileNotSaved(curDoc);
                            Docs[curDoc].ismodified = FALSE;
                            break;

                        default:

                            //We couldn't do the messagebox, or not ok to close
                            if (!CheckDocument(curDoc)) {
                                ErrorBox(ERR_Document_Corrupted);
                            }
                            closeIt = FALSE;
                            break;
                        }

                    }
#endif
                    if (closeIt) {
                        for (i = 0; i < MAX_VIEWS; i++) {
                            if (Views[i].Doc == curDoc) {
                                if (IsZoomed (Views[i].hwndFrame)) {
                                    ShowWindow (Views[i].hwndFrame, SW_RESTORE);
                                }


                                SendMessage(Views[i].hwndFrame, WM_CLOSE, 0, 0L);
                            }
                        }
                    }
                }
                break;

            case IDM_FILE_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0L);
                break;

            case IDM_EDIT_COPY:
                {

                    long XL,XR;
                    long YL,YR;
                    HWND hwndForeground = GetForegroundWindow();
                    HWND hwndFocus = GetFocus();

                    if (hwndForeground && hwndFocus && hwndActiveEdit != hwndForeground) {
                        // UGLY HACK. But hopefully this code will soon disappear.
                        // We may be in the quick watch window.
                        char szClassName[MAX_MSG_TXT];

                        GetClassName(hwndForeground, szClassName, sizeof(szClassName));

                        // We can tell if we are in the quick watch window if the window in the foreground is
                        // a dlg box, and the window that has the focus is a listbox. The listbox's parent however
                        // must belong to the 'SYS_Quick_wClass' class.
                        if (!strcmp(szClassName, "#32770")) { // #32770 is the class name for a standar dlg
                            // We have a dlg, is it the quick watch window?

                            HWND hwndParent = GetParent(hwndFocus);

                            if (hwndParent) {
                                char szQWatchClassName[MAX_MSG_TXT];

                                Dbg(LoadString(g_hInst, SYS_Quick_wClass, szQWatchClassName, sizeof(szQWatchClassName)));
                                GetClassName(hwndParent, szClassName, sizeof(szClassName));

                                if (!strcmp(szClassName, szQWatchClassName)) {
                                    PaneKeyboardHandler(hwndFocus, WM_COPY, 0, 0);
                                }
                            }
                        }
                    }

                    if ( v->Doc < -1 ) {
                        if ( hwndActiveEdit ) {
                            SendMessage(hwndActiveEdit, WM_COPY, 0, 0L);
                        }
                    } else if (v->BlockStatus) {
                        GetBlockCoord(curView, &XL, &YL, &XR, &YR);
                        PasteStream(curView, XL, YL, XR, YR);
                    }
                    break;
                }

            case IDM_EDIT_PASTE:
                if (hwndActiveEdit) {
                    SendMessage(hwndActiveEdit, WM_PASTE, 0, 0L);
                } else {
                    Assert(FALSE);
                }
                break;

            case IDM_EDIT_CUT:
                if (v->BlockStatus) {

                    long XL,XR;
                    long YL,YR;

                    GetBlockCoord(curView, &XL, &YL, &XR, &YR);
                    PasteStream(curView, XL,    YL, XR, YR);

                    DeleteStream(curView, XL, YL, XR, YR, TRUE);
                }
                break;

            case IDM_EDIT_DELETE:
                DeleteKey(curView);
                break;

            case IDM_EDIT_FIND:
                //FindNext box may already be there

                if (frMem.hDlgFindNextWnd) {
                    SetFocus(frMem.hDlgFindNextWnd);
                } else {
                    if (StartDialog(DLG_FIND, DlgFind)) {
                        Find();
                    }
                }
                break;

            case IDM_EDIT_REPLACE:
                //Replace box may already be there

                if (frMem.hDlgConfirmWnd) {
                    SetFocus(frMem.hDlgConfirmWnd);
                }
                else {
                    if (StartDialog(DLG_REPLACE, DlgReplace)) {
                        Replace(frMem.hDlgConfirmWnd);
                    }
                }
                break;

            case IDM_EDIT_PROPERTIES:
                {
                    int nCurView = 0, nCurDoc = 0;

                    Dbg(hwndActiveEdit);

                    nCurView = GetWindowWord(hwndActiveEdit, GWW_VIEW);
                    nCurDoc = Views[nCurView].Doc; //nCurView may change during loop

                    if (!(nCurDoc < -1)) {
                        if (MEMORY_WIN == Docs[nCurDoc].docType) {
                            memView = nCurView;
                            _fmemcpy (&TempMemWinDesc, &MemWinDesc[memView], sizeof(struct memWinDesc));
                            TempMemWinDesc.fHaveAddr = FALSE; //force re-evaluation
                            TempMemWinDesc.cMi = 0; //force re-evaluation
                            TempMemWinDesc.cPerLine = 0; //force re-evaluation
                            StartDialog(DLG_MEMORY, DlgMemory);
                        } else {
                            Dbg(0);
                        }
                    } else {
                        switch (-nCurDoc) {

                        case WATCH_WIN:
                            DialogType = WATCH_WIN;
                            StartDialog( DLG_PANEOPTIONS, DlgPaneOptions);
                            break;

                        case LOCALS_WIN:
                            DialogType = LOCALS_WIN;
                            StartDialog( DLG_PANEOPTIONS, DlgPaneOptions);
                            break;

                        case CALLS_WIN:
                            // The last parameter is the page that should be
                            // displayed initially when the prop sheet first appears.
                            // The 8 corresponds to Call Stack property page.
                            // See the documentation for DisplayOptionsPropSheet
                            // for more information.
                            DisplayOptionsPropSheet(hwnd, g_hInst, 8);
                            break;

                        default:
                            Dbg(0);
                        }
                    }
                }
                break;

            case IDM_VIEW_TOGGLETAG:
                LineStatus(Views[curView].Doc, v->Y + 1, TAGGED_LINE,
                    LINESTATUS_TOGGLE, FALSE, TRUE);
                break;

            case IDM_VIEW_NEXTTAG:
                {
                    long y = v->Y;

                    //Search forward in text
                    if (FindLineStatus(curView, TAGGED_LINE, TRUE, &y)) {
                        ClearSelection(curView);
                        PosXY(curView, 0, y, FALSE);
                    } else {
                        MessageBeep(0);
                    }
                    break;
                }

            case IDM_VIEW_PREVIOUSTAG:
                {
                    long y = v->Y;

                    //Search backward in text
                    if (FindLineStatus(curView, TAGGED_LINE, FALSE, &y)) {
                        ClearSelection(curView);
                        PosXY(curView, 0, y, FALSE);
                    } else {
                        MessageBeep(0);
                    }
                }
                break;

            case IDM_VIEW_CLEARALLTAGS:
                ClearDocStatus(Views[curView].Doc, TAGGED_LINE);
                break;

            case IDM_EDIT_GOTO_LINE:
                StartDialog(DLG_LINE, DlgLine);
                break;

            case IDM_EDIT_GOTO_ADDRESS:
                StartDialog(DLG_FUNCTION, DlgFunction);
                break;

            case IDM_VIEW_TOOLBAR:
                {
                    BOOL bVisible = !IsWindowVisible(GetHwnd_Toolbar());

                    CheckMenuItem(hMainMenu, IDM_VIEW_TOOLBAR,
                        bVisible ? MF_CHECKED : MF_UNCHECKED);
                    Show_Toolbar(bVisible);
                }
                break;

            case IDM_VIEW_STATUS:
                {
                    BOOL bVisible = !IsWindowVisible(GetHwnd_StatusBar());
                    CheckMenuItem(hMainMenu, IDM_VIEW_STATUS,
                        bVisible ? MF_CHECKED : MF_UNCHECKED);

                    // kcarlos
                    // BUGBUG
                    Show_StatusBar(bVisible);
                }
                break;

            case IDM_VIEW_FONT:
                SelectFont(hwnd);
                break;

            case IDM_VIEW_COLORS:
                SelectColor(hwnd);
                break;

            case IDM_VIEW_OPTIONS:
                DisplayOptionsPropSheet(hwnd, g_hInst, 0);
                break;

            case IDM_FILE_MRU_WORKSPACE1:
            case IDM_FILE_MRU_WORKSPACE2:
            case IDM_FILE_MRU_WORKSPACE3:
            case IDM_FILE_MRU_WORKSPACE4:
            case IDM_FILE_MRU_WORKSPACE5:
            case IDM_FILE_MRU_WORKSPACE6:
            case IDM_FILE_MRU_WORKSPACE7:
            case IDM_FILE_MRU_WORKSPACE8:
            case IDM_FILE_MRU_WORKSPACE9:
            case IDM_FILE_MRU_WORKSPACE10:
            case IDM_FILE_MRU_WORKSPACE11:
            case IDM_FILE_MRU_WORKSPACE12:
            case IDM_FILE_MRU_WORKSPACE13:
            case IDM_FILE_MRU_WORKSPACE14:
            case IDM_FILE_MRU_WORKSPACE15:
            case IDM_FILE_MRU_WORKSPACE16:
                {
                    PSTR psz;
                    char szPath[_MAX_PATH];
                    WORD wFileIdx = wItemId - IDM_FILE_MRU_WORKSPACE1;

                    // Sanity check
                    Dbg(wFileIdx < nbFilesKept[PROJECT_FILE]);

                    //  Look for the MRU program in the list.
                    //
                    Dbg(psz = (PSTR) GlobalLock(hFileKept[PROJECT_FILE][wFileIdx]));
                    strcpy(szPath, psz);
                    Dbg(GlobalUnlock (hFileKept[PROJECT_FILE][wFileIdx]) == FALSE);

                    // kcarlos - BUGBUG
                    //ProgramOpenPath(szPath);
                    Assert(0);
                }
                break;

            case IDM_FILE_SAVE_WORKSPACE:
                // No prompt, because we know the user wants to save.
                g_Windbg_WkSp.Save(FALSE, FALSE);
                break;

            case IDM_FILE_SAVEAS_WORKSPACE:
                g_Windbg_WkSp.SaveAs_WkSp_Prompt();
                break;

            case IDM_FILE_NEW_WORKSPACE:
                g_Windbg_WkSp.New_WkSp_Prompt(TRUE);
                break;

            case IDM_FILE_MANAGE_WORKSPACE:
                g_Windbg_WkSp.Manage_WkSp_Prompt();
                break;

            case IDM_FILE_SAVE_AS_WINDOW_LAYOUTS:
                g_Windbg_WkSp.SaveAs_WinLayout_Prompt();
                break;

            case IDM_FILE_MANAGE_WINDOW_LAYOUTS:
                g_Windbg_WkSp.Manage_WinLayout_Prompt();
                break;

            case IDM_DEBUG_RESTART:
                ExecDebuggee(EXEC_RESTART);
                break;

            case IDM_DEBUG_ATTACH:
                if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                    StartDialog(DLG_TASKLIST, DlgTaskList);
                }
                break;

            case IDM_DEBUG_GO:
                CmdExecuteCmd("g");
                break;

            case IDM_DEBUG_GO_HANDLED:
                CmdExecuteCmd("gh");
                break;

            case IDM_DEBUG_GO_UNHANDLED:
                CmdExecuteCmd("gn");
                break;

            case IDM_DEBUG_EXCEPTIONS:
                StartDialog(DLG_DEBUGEXCP, DlgDbugexcept);
                break;

            case IDM_DEBUG_RUNTOCURSOR:
                if (IsCallsInFocus()) {
                    SendMessage( GetCallsHWND(), WM_COMMAND,
                        MAKELONG( IDM_DEBUG_RUNTOCURSOR, 0 ), 0 );
                    break;
                }

                // DON'T force the command window open before
                // calling ExecDebuggee()
                if (DebuggeeActive() && !GoOK(LppdCur, LptdCur)) {
                    CmdInsertInit();
                    NoRunExcuse(LppdCur, LptdCur);
                } else if (!DebuggeeActive() && DebuggeeAlive()) {
                    CmdInsertInit();
                    NoRunExcuse(GetLppdHead(), NULL);
                } else if (LptdCur && LptdCur->fFrozen) {
                    CmdInsertInit();
                    CmdLogVar(DBG_Go_When_Frozen);
                } else if (!ExecDebuggee(EXEC_TOCURSOR)) {
                    CmdInsertInit();
                    CmdLogVar(ERR_Go_Failed);
                }
                break;

            case IDM_DEBUG_STEPINTO:
                CmdInsertInit();
                if (DebuggeeActive() && !StepOK(LppdCur, LptdCur)) {
                    NoRunExcuse(LppdCur, LptdCur);
                } else if (!DebuggeeActive() && DebuggeeAlive()) {
                    NoRunExcuse(GetLppdHead(), NULL);
                } else if (LptdCur && LptdCur->fFrozen) {
                    CmdLogVar(DBG_Go_When_Frozen);
                } else if (!ExecDebuggee(EXEC_TRACEINTO)) {
                    CmdLogVar(ERR_Go_Failed);
                }
                break;

            case IDM_DEBUG_STEPOVER:
                CmdInsertInit();
                if (DebuggeeActive() && !StepOK(LppdCur, LptdCur)) {
                    NoRunExcuse(LppdCur, LptdCur);
                } else if (!DebuggeeActive() && DebuggeeAlive()) {
                    NoRunExcuse(GetLppdHead(), NULL);
                } else if (LptdCur && LptdCur->fFrozen) {
                    CmdLogVar(DBG_Go_When_Frozen);
                } else if (!ExecDebuggee(EXEC_STEPOVER)) {
                    CmdLogVar(ERR_Go_Failed);
                }
                break;

            case IDM_DEBUG_BREAK:
                AsyncStop();
                break;

            case IDM_DEBUG_STOPDEBUGGING:
                ClearDebuggee();
                EnableToolbarControls();
                break;

            case IDM_EDIT_TOGGLEBREAKPOINT:
            case IDM_EDIT_BREAKPOINTS:

                if ( g_contWorkspace_WkSp.m_bKernelDebugger && IsProcRunning(LppdCur) ) {

                    ErrorBox(ERR_Cant_Modify_BP_While_Running);

                } else if (wItemId == IDM_EDIT_TOGGLEBREAKPOINT // F9 pressed or toolbar clicked
                    && !(!hwndActiveEdit                               // Are we in a code window
                    || v->Doc < 0
                    || ( Docs[v->Doc].docType != DISASM_WIN
                    && Docs[v->Doc].docType != DOC_WIN) ) ) {

                    if (!ToggleLocBP()) {

                        MessageBeep(0);
                        ErrorBox(ERR_NoCodeForFileLine);

                    }

                } else {
                    // menu got us here or we are not in a code window
                    StartDialog(DLG_BREAKPOINTS, DlgSetBreak);
                }
                break;

            case IDM_DEBUG_QUICKWATCH:
                StartDialog(DLG_QUICKWATCH, DlgQuickW);
                break;

            case IDM_DEBUG_SET_THREAD:
                StartDialog(DLG_THREAD, DlgThread);
                break;

            case IDM_DEBUG_SET_PROCESS:
                StartDialog(DLG_PROCESS, DlgProcess);
                break;

            case IDM_WINDOW_TILE_HORZ:
            case IDM_WINDOW_TILE_VERT:
                SendMessage(g_hwndMDIClient, WM_MDITILE,
                    (IDM_WINDOW_TILE_HORZ == wItemId) ? MDITILE_HORIZONTAL : MDITILE_VERTICAL,
                    0L);

                // It does not seem like the updating is necessary either.
                if (DebuggeeActive()) {
                    UpdateDebuggerState (UPDATE_MEMORY);
                }
                // commented out the line below for ntbug #27745
                // InvalidateAllWindows();
                break;

            case IDM_WINDOW_CASCADE:
                SendMessage(g_hwndMDIClient, WM_MDICASCADE, 0, 0L);
                // It does not seem like the updating is necessary either.
                if (DebuggeeActive()) {
                    UpdateDebuggerState (UPDATE_MEMORY);
                }
                // commented out the line below for ntbug #27745
                // InvalidateAllWindows();
                break;

            case IDM_WINDOW_ARRANGE_ICONS:
                SendMessage(g_hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
                break;

            case IDM_WINDOW_ARRANGE:
                arrange();
                {
                    BOOL    Active;

                    Active = DebuggeeActive();
                    if (Active)
                    {
                        UpdateDebuggerState (UPDATE_WINDOWS);
                    }
                }
                break;

            case IDM_WINDOW_NEWWINDOW:
                AddFile(MODE_DUPLICATE, DOC_WIN, NULL, NULL, NULL, FALSE, curView, -1, TRUE);
                EnableToolbarControls();
                break;

            case IDM_WINDOW_SOURCE_OVERLAY:
                FSourceOverlay = !FSourceOverlay;
                CheckMenuItem(hMainMenu, IDM_WINDOW_SOURCE_OVERLAY,
                    FSourceOverlay ? MF_CHECKED : MF_UNCHECKED);
                break;

            case IDM_VIEW_REGISTERS:
                OpenDebugWindow(CPU_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_WATCH:
                OpenDebugWindow(WATCH_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_LOCALS:
                OpenDebugWindow(LOCALS_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_DISASM:
                OpenDebugWindow(DISASM_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_COMMAND:
#if defined( NEW_WINDOWING_CODE )
                New_OpenDebugWindow(CMD_WINDOW, FALSE); // Not user activated
#else
                OpenDebugWindow(COMMAND_WIN, FALSE); // Not user activated
#endif
                EnableToolbarControls();
                break;

            case IDM_VIEW_FLOAT:
                OpenDebugWindow(FLOAT_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_MEMORY:
                memView = -1;
                memset (&TempMemWinDesc,0,sizeof(struct memWinDesc));
                TempMemWinDesc.iFormat = MW_BYTE;     //default to byte display
                OpenDebugWindow(MEMORY_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_CALLSTACK:
                OpenDebugWindow(CALLS_WIN, TRUE); // User activated
                EnableToolbarControls();
                break;

            case IDM_HELP_CONTENTS:
                // Display the table of contents
                DisplayHelpContents(hwnd, 0);
                break;

            case IDM_HELP_SEARCH:

                Dbg(WinHelp(hwnd, szHelpFileName, HELP_FORCEFILE, 0L));
                Dbg(WinHelp(hwnd, szHelpFileName, HELP_PARTIALKEY, (DWORD_PTR)(LPSTR)szNull));
                break;

            case IDM_HELP_ABOUT:

                ShellAbout( hwnd, MainTitleText, NULL, NULL );
                break;

                //**************************************************
                //Those following commands are not accessible via menus

            case IDA_FINDNEXT:
                if (hwndActiveEdit && !IsIconic(hwndActive)
                    && (curView != -1) && (Views[curView].Doc >= 0)) {

                    if (findReplace.findWhat[0] == '\0') {
                        if (StartDialog(DLG_FIND, DlgFind)) {
                            Find();
                        }
                    } else {
                        if (frMem.hDlgFindNextWnd) {
                            SetFocus(frMem.hDlgFindNextWnd);
                            SendMessage(frMem.hDlgFindNextWnd, WM_COMMAND, IDOK, 0L);
                        } else {
                            if (frMem.hDlgConfirmWnd) {
                                SetFocus(frMem.hDlgConfirmWnd);
                                SendMessage(frMem.hDlgConfirmWnd, WM_COMMAND, ID_CONFIRM_FINDNEXT, 0L);
                            } else {
                                FindNext(hwnd, v->Y, v->X, v->BlockStatus, TRUE, TRUE);
                            }
                        }
                    }
                } else {
                    MessageBeep(0);
                }
                break;


            case IDM_DEBUG_SOURCE_MODE:
                SetSrcMode_StatusBar(!GetSrcMode_StatusBar());
                EnableToolbarControls();
                break;

            case IDM_DEBUG_SOURCE_MODE_ON:
                SetSrcMode_StatusBar(TRUE);
                EnableToolbarControls();
                break;

            case IDM_DEBUG_SOURCE_MODE_OFF:
                SetSrcMode_StatusBar(FALSE);
                EnableToolbarControls();
                break;

            default:
                goto DefProcessing;
            }
        }
        break;

    case WM_COMPACTING:
        {
            int i;

            if (!editorIsCritical) {
                for (i = 0; i < MAX_DOCUMENTS; i++) {
                    if(Docs[i].FirstView != -1)
                        CompactDocument(i);
                }
            }

            // Completely arbitrarily, try and decide whether we
            // have enough memory to do a "soft" system-modal or not
            if (GlobalCompact(0UL) > (30*1024UL))
            {
                // Soft
                MsgBox(hwndFrame, LowMemoryMsg,
                    MB_OK|MB_TASKMODAL|MB_ICONINFORMATION);
            }
            else
            {
                // Hard
                MsgBox(hwndFrame, LowMemoryMsg2,
                    MB_OK|MB_TASKMODAL|MB_ICONHAND);
            }
        }
        return TRUE;

    case WM_FONTCHANGE:
        {
            int i, j, k;
            HDC hDC;
            HFONT font;
            int fontPb = 0;
            char faceName[LF_FACESIZE];

            LoadFonts(hwnd);

            //Check to see if the default font still exist

            font = CreateFontIndirect(&g_logfontDefault);
            Dbg(hDC = GetDC(hwndFrame));
            Dbg(SelectObject(hDC, font));
            GetTextFace(hDC, LF_FACESIZE, faceName);
            if (_strcmpi(faceName, (char FAR *) g_logfontDefault.lfFaceName) != 0) {

                TEXTMETRIC tm;

                ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_Lost_Default_Font, (LPSTR)g_logfontDefault.lfFaceName, (LPSTR)faceName);

                //Change characteristics of default font

                GetTextMetrics(hDC, &tm);
                g_logfontDefault.lfHeight = tm.tmHeight;
                g_logfontDefault.lfWeight = tm.tmWeight;
                g_logfontDefault.lfPitchAndFamily = tm.tmPitchAndFamily;
                lstrcpy((LPSTR) g_logfontDefault.lfFaceName, faceName);
            }
            ReleaseDC(hwndFrame, hDC);
            Dbg(DeleteObject(font));

            //Ensure that each view still has an existing font, warn the
            //user and load the closest font otherwise

            for (i = 0; i < MAX_VIEWS; i++) {
                if (Views[i].Doc != -1) {
                    k = 0;

                    //Get Face name of view

                    Dbg(hDC = GetDC(Views[i].hwndClient));
                    SelectObject(hDC, Views[i].font);
                    GetTextFace(hDC, LF_FACESIZE, faceName);
                    Dbg(ReleaseDC(Views[i].hwndClient, hDC));

                    //See if this font still exist

                    for (j = 0; j < fontsNb; j++){
                        if (_strcmpi((LPSTR) fonts[j].lfFaceName, (LPSTR) faceName) == 0){
                            k++;
                        }
                    }

                    //Substitute with default font if not found

                    if (k == 0) {
                        fontPb++;
                        Dbg(DeleteObject(Views[i].font));
                        Views[i].font = CreateFontIndirect(&g_logfontDefault);
                        SetVerticalScrollBar(i, FALSE);
                        PosXY(i, Views[i].X, Views[i].Y, FALSE);
                        InvalidateLines(i, 0, LAST_LINE, TRUE);
                    }
                }

            }
            // This is to let the edit window set new font
            if (IsWindow(hwndActiveEdit)) {
                SendMessage(hwndActiveEdit, message, wParam, lParam);
            }
            if (fontPb > 0)
                ErrorBox2(hwndFrame, MB_TASKMODAL, ERR_Lost_Font);
        }
        return TRUE;

    case WM_INITMENU:

        // TOOLBAR handling - a menu item has been selected.
        // Catches keyboard menu selecting.
        if (GetWindowLong(hwnd, GWL_STYLE) & WS_ICONIC) {
            break;
        }

        // kcarlos - dead code
        //Set up the menu states and the filenames in menus for Project
        g_Windbg_WkSp.Load_File_MRU_List();
        InitializeMenu((HMENU)wParam);
        break;


    case WM_MENUSELECT:
        {
            WORD wMenuItem      = (UINT) LOWORD(wParam);    // menu item or submenu index
            WORD wFlags         = (UINT) HIWORD(wParam);    // menu flags
            HMENU hmenu         = (HMENU) lParam;           // handle of menu clicked

            lastMenuId = LOWORD(wParam);

            if(0xFFFF == wFlags && NULL == hmenu) {

                //
                // Menu is closed, clear the Status Bar.
                //

                menuID = 0;
                SetMessageText_StatusBar( SYS_StatusClear, STATUS_INFOTEXT);

            } else if( wFlags & MF_POPUP ) {

                //
                // Get the menuID for the pop-up menu.
                //

                menuID = GetPopUpMenuID(( HMENU ) lParam );

            } else {

                //
                // Get the menuID for the menu item.
                //

                menuID = wMenuItem;
            }
        }
        break;

    case WM_ENTERIDLE:
#define BETWEEN(inf, sup) (lastMenuId >= inf && lastMenuId <= sup)
        if ((wParam == MSGF_MENU)
            && (GetKeyState(VK_F1) & 0x8000)
            && (lastMenuIdState == 0)) {

            if (BETWEEN(IDM_FILE_FIRST, IDM_FILE_LAST)
                || BETWEEN(IDM_EDIT_FIRST, IDM_EDIT_LAST)
                || BETWEEN(IDM_VIEW_FIRST, IDM_VIEW_LAST)
                || BETWEEN(IDM_DEBUG_FIRST, IDM_DEBUG_LAST)
                || BETWEEN(IDM_WINDOW_FIRST, IDM_WINDOW_LAST)
                || BETWEEN(IDM_HELP_FIRST, IDM_HELP_LAST)) {

                bHelp = TRUE;
                Dbg(WinHelp(hwnd, szHelpFileName, (DWORD) HELP_CONTEXT,(DWORD)lastMenuId));
            } else {
                MessageBeep(0);
            }

        } else if ((wParam == MSGF_DIALOGBOX) && (GetKeyState(VK_F1) & 0x8000)) {

            //Is it one of our dialog boxes

            if (GetDlgItem((HWND) lParam, IDWINDBGHELP)) {
                Dbg(PostMessage((HWND) lParam, WM_COMMAND, IDWINDBGHELP, 0L));
            } else {

                // The only dialog boxes having special help id
                // are from COMMDLG module (Files pickers)

                if (GetDlgItem((HWND) lParam, psh15)) {
                    Dbg(PostMessage((HWND) lParam, WM_COMMAND, psh15, 0L));
                } else {
                    MessageBeep(0);
                }
            }

        }

        SetMessageText_StatusBar(menuID, STATUS_MENUTEXT);
        break;

    case WM_CLOSE:
        if (TerminateApplication(hwnd, wParam)) {
            ExitProcess(0);
        }
        break;

    case WM_QUERYENDSESSION:
        //Are we re-entering ?
        if (BoxCount != 0) {
            ErrorBox2(GetActiveWindow(), MB_TASKMODAL, ERR_Cannot_Quit);
            return 0;
        } else {

            //Before session ends, check that all files are saved
            //And that it is ok to quit when debugging

            if (DebuggeeActive() &&
                (QuestionBox(ERR_Close_When_Debugging, MB_YESNO) != IDYES))
            {
                return 0;
            }
            return QueryCloseAllDocs();
        }

    case WM_DRAWITEM:
        switch (wParam) {
        case IDC_STATUS_BAR:
            OwnerDrawItem_StatusBar((LPDRAWITEMSTRUCT) lParam);
            return TRUE;
        }
        goto DefProcessing;


        case WM_DESTROY:
            Assert(DestroyMenu(hmenuCmdWin));
            Terminate_StatusBar();
            ImeTerm();
            PostQuitMessage(0);
            QuitTheSystem = TRUE;
            break;

        case WM_SETFOCUS:
            if (!hwndActive) {
                ImeSendVkey(hwnd, VK_DBE_FLUSHSTRING);
                bOldImeStatus = ImeWINNLSEnableIME(NULL, FALSE);
            }
            goto DefProcessing;

        case WM_KILLFOCUS:
            if (!hwndActive) {
                ImeWINNLSEnableIME(NULL, bOldImeStatus);
            }
            goto DefProcessing;

        case WM_MOVE:
            // This is to let the edit window
            // set a position of IME conversion window
            if (hwndActive) {
                SendMessage(hwndActive, WM_MOVE, 0, 0);
            }
            break;

        case WM_SIZE:
            {
                RECT rc;
                int nToolbarHeight = 0;   // Toolbar
                int nStatusHeight = 0;   // status bar

                GetClientRect (hwnd, &rc);

                // First lets resize the toolbar
                SendMessage(GetHwnd_Toolbar(), WM_SIZE, wParam,
                    MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

                // 2nd resize the status bar
                WM_SIZE_StatusBar(wParam, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

                //On creation or resize, size the MDI client,
                //status line and toolbar.
                if ( IsWindowVisible(GetHwnd_StatusBar()) ) {
                    RECT rcStatusBar;

                    GetWindowRect(GetHwnd_StatusBar(), &rcStatusBar);

                    nStatusHeight = rcStatusBar.bottom - rcStatusBar.top;
                }

                if (IsWindowVisible(GetHwnd_Toolbar())) {
                    RECT rcToolbar;

                    GetWindowRect(GetHwnd_Toolbar(), &rcToolbar);

                    nToolbarHeight = rcToolbar.bottom - rcToolbar.top;
                }

                MoveWindow(g_hwndMDIClient,
                    rc.left, rc.top + nToolbarHeight,
                    rc.right - rc.left,
                    rc.bottom - rc.top - nStatusHeight - nToolbarHeight,
                    TRUE);

                SendMessage(g_hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
                // This is to let the edit window
                // set a position of IME conversion window
                if (hwndActive) {
                    SendMessage(hwndActive, WM_MOVE, 0, 0);
                }
            }
            break;

        case WM_SYSCOMMAND:
            {
                LRESULT ret;

                // Handle title bar

                if ((wParam == SC_NEXTWINDOW) ||
                    (wParam == SC_PREVWINDOW))
                {
                    UpdateTitleBar(TBM_UNKNOWN, TRUE);
                }

                ret = DefFrameProc(hwnd, g_hwndMDIClient, message, wParam, lParam);

                // Handle title bar

                if ((wParam == SC_MAXIMIZE) ||
                    (wParam == SC_MINIMIZE) ||
                    (wParam == SC_RESTORE))
                {
                    UpdateTitleBar(TBM_UNKNOWN, TRUE);
                }

                return ret;
            }

        case WM_ACTIVATEAPP:

            if (IsIconic(hwndFrame)) {

                checkFileDate = TRUE;

            } else {
                int k;
                FILETIME LastWriteTime;

                // Check the opened files to see if files has been changed
                // by another App

                for (k = 0; k < MAX_DOCUMENTS; k++) {

                    if ((Docs[k].FirstView != -1) &&
                        (Docs[k].docType == DOC_WIN)) {

                        if (GetFileTimeByName( Docs[k].szFileName,
                            NULL,
                            NULL,
                            &LastWriteTime
                            )) {

                            //
                            // Compare our internal date and file's date,
                            // if changed ask (later) the user what he
                            // thinks about it.
                            //

                            if (CompareFileTime(&LastWriteTime, &Docs[k].time) > 0 &&
                                (!Docs[k].bChangeFileAsk)) {

                                Docs[k].bChangeFileAsk = TRUE;
                                PostMessage(hwndFrame,
                                    WU_RELOADFILE,
                                    k,
                                    SYS_File_Changed);
                            }

                        } else if ( GetLastError() == ERROR_FILE_NOT_FOUND &&
                            !Docs[k].untitled) {

                            PostMessage(hwndFrame,
                                WU_RESAVEFILE,
                                0,
                                (LPARAM)(LPSTR)Docs[k].szFileName);
                        }
                    }
                }
            }

            goto DefProcessing;


            /*
            **   Somebody requeseted that the screen be updated
            */
        case DBG_REFRESH:
            CmdExecNext((DBC) wParam, lParam);
            break;

        case WU_RELOADFILE:
            {
                LPDOCREC docs = &Docs[wParam];
                WORD modalType;

                Assert(docs->docType == DOC_WIN);

                //
                // User may have clicked in the editing window, clear timer
                // and release mouse capture
                //

                if (curView != -1) {
                    KillTimer(Views[curView].hwndClient, 100);
                    ReleaseCapture();
                }

                if (AutoTest) {
                    modalType = MB_TASKMODAL;
                } else {
                    modalType = MB_TASKMODAL;
                }

                //Ask user for doc reloading

                if (g_contWorkspace_WkSp.m_bAutoReloadSrcFiles
                    || QuestionBox2(hwndFrame,
                                    LOWORD(lParam),
                                    (WORD)(MB_YESNO | modalType),
                                    (LPSTR)docs->szFileName) == IDYES) {

                    int *pV;

                    editorIsCritical = TRUE;

                    //Refresh contents. But leave it where it was.

                    if (OpenDocument(MODE_RELOAD, DOC_WIN, (int) wParam, docs->szFileName,
                        -1, -1, FALSE) != -1) {

                        //Clear selection and move up the cursor if text is smaller
                        //(for each view)

                        pV = &docs->FirstView;
                        while (*pV != -1) {
                            ClearSelection(*pV);
                            if (Views[*pV].Y >= docs->NbLines) {
                                Views[*pV].Y = docs->NbLines - 1;
                            }
                            SetVerticalScrollBar(*pV, FALSE);
                            PosXY(*pV, 0, Views[*pV].Y, FALSE);
                            EnsureScrollBars(*pV, FALSE);
                            pV = &Views[*pV].NextView;
                        }

                        //Display any debug/error tags/lines

                        SetDebugLines((int) wParam, TRUE);

                        //Refresh possible views of changed documents

                        InvalidateLines(docs->FirstView, 0, LAST_LINE, TRUE);

                        editorIsCritical = FALSE;
                    }
                } else {
                    //Set the opened doc file struct timewise

                    GetFileTimeByName( docs->szFileName,
                        NULL,
                        NULL,
                        &docs->time
                        );
                }

                docs->bChangeFileAsk = FALSE;
            }
            break;


        case WU_RESAVEFILE:
            {
                WORD modalType;


                //User may have clicked in the editing window, clear timer
                //and release mouse capture
                if (curView != -1) {

                    KillTimer(Views[curView].hwndClient, 100);
                    ReleaseCapture();
                }


                //Are we in test mode or not

                if (AutoTest) {
                    modalType = MB_TASKMODAL;
                } else {
                    modalType = MB_TASKMODAL;
                }

                //Ask user for doc reloading

                ErrorBox (ERR_File_Deleted,(LPSTR)lParam);

            }
            break;




DefProcessing:
        default:
            return DefFrameProc(hwnd, g_hwndMDIClient, message, wParam, lParam);
    }
    return (0L);
}                   /* MainWndProc() */


/****************************************************************************

    FUNCTION   : MDIChildWndProc

    PURPOSE    : The window function for the individual document windows,
         each of which has a "note". Each of these windows contain
         one multi-line edit control filling their client area.

****************************************************************************/
LRESULT
CALLBACK
MDIChildWndProc(
                HWND hwnd,
                UINT message,
                WPARAM wParam,
                LPARAM lParam
                )
{
    HWND hwndEdit;

    switch (message) {
    case WM_CREATE:
        {
            LPMDICREATESTRUCT pmcs;
            TEXTMETRIC tm;
            HDC hDC;
            DWORD style;
            char szClass[MAX_MSG_TXT];
            int newView;
            LPVIEWREC v;

            //Remember the window handle and initialize some window attributes
            pmcs = (LPMDICREATESTRUCT)(((CREATESTRUCT FAR *)lParam)->lpCreateParams);
            newView = (int)pmcs->lParam;
            Assert(newView >= 0 && newView < MAX_VIEWS && Views[newView].hwndClient == 0);
            v = &Views[newView];

            style = WS_CHILD | WS_MAXIMIZE | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;

            Dbg(LoadString(g_hInst, SYS_Edit_wClass, szClass, MAX_MSG_TXT));

            hwndEdit = CreateWindow((LPSTR)szClass, NULL, style,  0,  0,  0,  0,
                hwnd, (HMENU) ID_EDIT, g_hInst, (LPSTR)pmcs->lParam);

            //Remember the window handle

            SetWindowHandle(hwnd, GWW_EDIT, (WPARAM)hwndEdit);
            v->hwndClient = hwndEdit;
            v->bDBCSOverWrite = TRUE;

            //Remember the window view

            SetWindowWord(hwnd, GWW_VIEW, (WORD)newView);

            //Initialize font information for window

            hDC = GetDC(hwndEdit);

            //If we open the file from workspace, font is already
            //created

            if (v->font == NULL) {
                v->font = CreateFontIndirect((LPLOGFONT)&g_logfontDefault);
            }

            //Store all information about fonts

            SelectObject(hDC, v->font);
            GetTextMetrics(hDC, &tm);
            v->BlockStatus = FALSE;
            v->charHeight = tm.tmHeight;
            v->maxCharWidth =tm.tmMaxCharWidth;
            v->aveCharWidth =tm.tmAveCharWidth;
            v->charSet = tm.tmCharSet;
            GetCharWidth(hDC, 0, MAX_CHARS_IN_FONT - 1, (LPINT)v->charWidth);
            GetDBCSCharWidth(hDC, &tm, v);
            ReleaseDC(hwndEdit, hDC);




            // initialize DOC structure readonly flags/values

            Docs[Views[pmcs->lParam].Doc].RORegionSet = FALSE;
            Docs[Views[pmcs->lParam].Doc].RoX2 = 0;
            Docs[Views[pmcs->lParam].Doc].RoY2 = 0;


            //Set vertical scroll-bar position

            SetScrollPos(hwndEdit, SB_VERT, 0, TRUE);

            //Set horizontal scroll-bar position

            SetScrollRange(hwndEdit, SB_HORZ, 0, MAX_USER_LINE, FALSE);
            SetScrollPos(hwndEdit, SB_HORZ, 0, TRUE);

            if (Docs[Views[pmcs->lParam].Doc].docType != DOC_WIN) {

                WNDPROC lpfnNewEditProc, NewEditProc;

                switch (Docs[Views[pmcs->lParam].Doc].docType) {

                case DISASM_WIN:
                    NewEditProc = (WNDPROC) DisasmEditProc;
                    break;

                case COMMAND_WIN:
                    NewEditProc = (WNDPROC) CmdEditProc;
                    break;

                case MEMORY_WIN:
                    NewEditProc = (WNDPROC) MemoryEditProc;
                    break;

                default:
                    Assert(FALSE);
                    return FALSE;
                    break;
                }

                //Sub-szClass the editor proc for the specialized windows
                lpfnEditProc = (WNDPROC)GetWindowLongPtr(Views[pmcs->lParam].hwndClient, GWLP_WNDPROC);


                lpfnNewEditProc = (WNDPROC)NewEditProc;

                SetWindowLongPtr(Views[pmcs->lParam].hwndClient, GWLP_WNDPROC, (LONG_PTR)lpfnNewEditProc);

                //Init the new subclassed proc window
                SendMessage(Views[pmcs->lParam].hwndClient, WU_INITDEBUGWIN, 0, pmcs->lParam);
            }

            SetFocus(hwndEdit);
        }
        break;


      case WM_MDIACTIVATE:

          //If we're activating this child, remember it

          if (hwnd == (HWND) lParam) // activating
          {

              int curDoc;
              LPDOCREC d;

              hwndActive  = hwnd;
              hwndActiveEdit = (HWND)GetWindowHandle(hwnd, GWW_EDIT);

              //Get global current view
              curView = GetWindowWord(hwndActiveEdit, GWW_VIEW);
              curDoc = Views[curView].Doc;
              Assert (curView >= 0  && curView < MAX_VIEWS && curDoc != -1);

              d = &Docs[Views[curView].Doc];



              if (Views[curView].Doc > -1) {
                  d = &Docs[Views[curView].Doc];

                  if (d->docType == MEMORY_WIN) {
                      int memcurView = GetWindowWord((HWND)GetWindowHandle(hwnd, GWW_EDIT), GWW_VIEW);
                      int curDoc = Views[memcurView].Doc; //memcurView may change during loop

                      if (Docs[curDoc].docType == MEMORY_WIN) {
                          memView = memcurView;
                          _fmemcpy (&MemWinDesc,&MemWinDesc[memView],sizeof(struct memWinDesc));
                          ViewMem (memView, FALSE);
                      }
                  }

              }


              // Update toolbar for new document

              EnableToolbarControls();

              //To avoid problem of focus lost when an MDI window is
              //reactivated and when the previous activated window was
              //minimized. (Could be a bug in MDI or in my brain, seems
              //to be linked with the fact that our MdiClientChildClient
              //is not a standard class window)


              if (!IsIconic(hwnd)) {
                  SetFocus(hwndActiveEdit);
              }
          } else {
              //Unselect previous window view menu item

              CheckMenuItem(hWindowSubMenu,
                  (WORD) FindWindowMenuId(Docs[Views[curView].Doc].docType, curView, TRUE),
                  MF_UNCHECKED);

              //Updates status bar.

              SetLineColumn_StatusBar(0, 0);

              hwndActive = NULL;
              hwndActiveEdit = NULL;
              curView = -1;
          }
          break;


      case WM_CLOSE:
          {
              int i;
              int curDoc = Views[curView].Doc; //curView may change during loop
              BOOL closeIt = TRUE;

// BUGBUG - dead code - kcarlos
#if 0
              if (curDoc >= 0 &&
                  Docs[curDoc].docType == DOC_WIN && Docs[curDoc].ismodified) {

                  //Ask user whether to save / not save / cancel
                  switch (QuestionBox(SYS_Save_Changes_To,
                      MB_YESNOCANCEL ,
                      (LPSTR)Docs[curDoc].szFileName)) {
                  case IDYES:

                      //User wants file saved
                      if (SaveFile(curDoc)) {
                          //Reset file creation/load/save time
                          GetFileTimeByName( Docs[curDoc].szFileName,
                              NULL,
                              NULL,
                              &Docs[curDoc].time
                              );
                          Docs[curDoc].ismodified = FALSE;
                      } else {
                          closeIt = FALSE;
                      }
                      break;

                  case IDNO:

                      FileNotSaved(curDoc);
                      Docs[curDoc].ismodified = FALSE;
                      break;

                  default:

                      //We couldn't do the messagebox, or not ok to close
                      if (!CheckDocument(curDoc)) {
                          ErrorBox(ERR_Document_Corrupted);
                      }
                      closeIt = FALSE;
                      break;
                  }

              }
#endif
              if (closeIt) {
                  for (i = 0; i < MAX_VIEWS; i++) {
                      if (Views[i].Doc == curDoc) {
                          if (IsZoomed (Views[i].hwndFrame)) {
                              ShowWindow (Views[i].hwndFrame, SW_RESTORE);
                              break;
                          }
                      }
                  }
              }
          }
          goto CallDCP;

// BUGBUG - dead code - kcarlos
#if 0
          if (QueryCloseChild(GetWindowHandle(hwnd, GWW_EDIT), FALSE)) {
              goto CallDCP;
          } else {
              break;
          }
#endif

      case WM_ERASEBKGND:
          //Do nothing, paint will handle it
          break;

      case WM_MOVE:
          {
              int view;

              view  = GetWindowWord(GetWindowHandle(hwnd, GWW_EDIT), GWW_VIEW);
              Assert(view >=0 && view < MAX_VIEWS);

              //Save the window size (not when maximized or minimized)

              if (!IsIconic(hwnd) && !IsZoomed(hwnd)) {
                  GetWindowRect(hwnd, (LPRECT)&Views[view].rFrame);
              }
              // This is to let the edit window
              // set a position of IME conversion window
              if (GetWindowHandle(hwnd, GWW_EDIT)) {
                  SendMessage(GetWindowHandle(hwnd, GWW_EDIT), WM_MOVE, 0, 0);
              }
              goto CallDCP;
          }

      case WM_SIZE:
          {
              int view;
              LPVIEWREC v;
              LPDOCREC d;

              view  = GetWindowWord(GetWindowHandle(hwnd, GWW_EDIT), GWW_VIEW);
              Assert(view >=0 && view < MAX_VIEWS);
              v = &Views[view];
              d = &Docs[Views[view].Doc];


              if (Views[view].Doc > 0) {
                  if (d->docType == MEMORY_WIN) {
                      InMemUpdate = STARTED; // prevent multiple viemem() calls
                  }
              }


              //Adjust and display window titlebar text

              RefreshWindowsTitle(v->Doc);

              //Save the window size (not when maximized or minimized)

              if (wParam != SIZEICONIC && wParam != SIZEFULLSCREEN) {
                  GetWindowRect(hwnd, (LPRECT)&v->rFrame);
              }

              //Also resize the edit control

              if (wParam != SIZEICONIC) {

                  //Set client to it's new size

                  MoveWindow(v->hwndClient, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);

                  //Update scrollbars status and resize the window

                  EnsureScrollBars(view, TRUE);

                  // Make sure that the caret is visible...(this needs more thought!)

                  PostMessage (v->hwndClient, EM_SCROLLCARET, 0, 0);
              }



              if (Views[view].Doc > 0) {
                  if (d->docType == MEMORY_WIN) {
                      InMemUpdate = FINISHED; // prevent multiple viemem() calls
                  }
              }
          }
          goto CallDCP;

      case WM_SETFOCUS:
          SetFocus(GetWindowHandle(hwnd, GWW_EDIT));
          break;

      default:
CallDCP:

          //MDI default behavior is  different, call DefMDIChildProc
          //instead of DefWindowProc()

          return DefMDIChildProc(hwnd, message, wParam, lParam);
    }
    return FALSE;
}                   /* MDIChildWndProc() */

LRESULT
CALLBACK
ChildWndProc(
             HWND   hwnd,
             UINT   message,
             WPARAM wParam,
             LPARAM lParam
             )
{
    PAINTSTRUCT ps;
    static  BOOL    bOldImeStatus;


    switch (message) {

    case WM_CREATE:
        {
            //WARNING : lParam is NOT a pointer to a Valid CREATESTRUCT
            //but holds the view number.
            long l;

            l = PtrToLong(((CREATESTRUCT FAR *)lParam)->lpCreateParams);
            Assert (l >= 0 && l < MAX_VIEWS);

            //Remember the window handle

            SetWindowWord(hwnd, GWW_VIEW, (WORD)l);

            break;
        }

    case WM_RBUTTONDOWN:
        {
            long XL,XR;
            long YL,YR;
            int view = GetWindowWord(hwnd, GWW_VIEW);
            if (Views[view].BlockStatus) {
                GetBlockCoord(view, &XL, &YL, &XR, &YR);
                PasteStream(view, XL, YL, XR, YR);
                ClearSelection(view);
                if (view == cmdView) {
                    PosXY(view,
                        GetLineLength(view, FALSE, Docs[Views[view].Doc].NbLines - 1),
                        Docs[Views[view].Doc].NbLines - 1, FALSE);
                }
            } else {
                SendMessage(hwnd, WM_PASTE, 0, 0L);
            }
        }
        break;

    case WM_SETFOCUS:
        //Set cursor position
        if (!editorIsCritical) {

            int view = GetWindowWord(hwnd, GWW_VIEW);
            LPVIEWREC v = &Views[view];
            LPDOCREC d;


            curView = view;
            CreateCaret(hwnd, 0, 3, v->charHeight);
            SetCaret(view, v->X, v->Y, -1);
            ShowCaret(hwnd);
            SetLineColumn_StatusBar(v->Y + 1, v->X + 1);
            d = &Docs[Views[view].Doc];

            if (d->docType == DOC_WIN
                ||  d->docType == MEMORY_WIN
                ||  d->docType == COMMAND_WIN) {
                
                bOldImeStatus = ImeWINNLSEnableIME(NULL, TRUE);
                ImeSetFont(hwnd, v->font);
                if (!IsWindowVisible(hwnd)) {
                    ImeMoveConvertWin(hwnd, -1, -1);
                }
            } else {
                ImeSendVkey(hwnd, VK_DBE_FLUSHSTRING);
                bOldImeStatus = ImeWINNLSEnableIME(NULL, FALSE);
            }

            if (Views[curView].Doc > -1) {
                d = &Docs[Views[curView].Doc];

                if (d->docType == MEMORY_WIN) {
                    int     memcurView, curDoc;
                    HWND    hwndEdit = (HWND)GetWindowHandle(hwnd, GWW_EDIT);
                    if (hwndEdit != NULL) {
                        memcurView = GetWindowWord(hwndEdit, GWW_VIEW);
                    } else {
                        memcurView = 0;
                    }
                    //memcurView may change during loop
                    curDoc = Views[memcurView].Doc;

                    if (Docs[curDoc].docType == MEMORY_WIN) {
                        memView = memcurView;
                        _fmemcpy (&MemWinDesc,
                            &MemWinDesc[memView],
                            sizeof(struct memWinDesc));
                        ViewMem (memView, FALSE);
                    }
                }
            }
        }
        break;

    case WM_KILLFOCUS:
        //Set cursor position
        if (!editorIsCritical) {
            //Remember the window view

            int view = GetWindowWord(hwnd, GWW_VIEW);
            LPVIEWREC v = &Views[view];

            SetWindowWord(hwnd, GWW_VIEW, (WORD)view);
            HideCaret(hwnd);
            {
                LPDOCREC d;
                d = &Docs[Views[view].Doc];

                if (d->docType == DOC_WIN
                    ||  d->docType == MEMORY_WIN
                    ||  d->docType == COMMAND_WIN) {

                    ImeSetFont(hwnd, NULL);
                    ImeMoveConvertWin(hwnd, -1, -1);
                }
                ImeWINNLSEnableIME(NULL, bOldImeStatus);
            }
            DestroyCaret();
        }
        break;

    case WM_MOVE:
    case WM_SIZE:
        if (GetFocus() == hwnd
            &&  !editorIsCritical) {

            int view = GetWindowWord(hwnd, GWW_VIEW);
            LPVIEWREC v = &Views[view];

            // This is to set the position of IME conversion window
            SetCaret(view, v->X, v->Y, -1);
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
        break;

    case WM_FONTCHANGE:
        // This message is sent only from MainWndProc
        // (not from system)
        if (!editorIsCritical) {
            int view = GetWindowWord(hwnd, GWW_VIEW);
            LPVIEWREC v = &Views[view];

            ImeSetFont(hwnd, v->font);
        }
        break;

    case WM_LBUTTONDBLCLK:
        {
            int view = GetWindowWord(hwnd, GWW_VIEW);

            ButtonDown(view, wParam, LOWORD(lParam), HIWORD(lParam));
            ButtonUp(view, wParam, LOWORD(lParam), HIWORD(lParam));
            GetWordAtXY(view, Views[view].X, Views[view].Y,
                TRUE, NULL, TRUE, NULL, 0, NULL, NULL);
            break;
        }

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        if (!editorIsCritical) {
            PaintText(GetWindowWord(hwnd, GWW_VIEW), ps.hdc, &ps.rcPaint);
        } else {
            AuxPrintf(1, "WM_PAINT editorWasCritical");
        }
        EndPaint(hwnd, &ps);
        break;

    case WM_TIMER:
        if (wParam == 100) {
            TimeOut(GetWindowWord(hwnd, GWW_VIEW));
        } else {
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;

    case WM_LBUTTONDOWN:
        ButtonDown(GetWindowWord(hwnd, GWW_VIEW), wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_LBUTTONUP:
        ButtonUp(GetWindowWord(hwnd, GWW_VIEW), wParam, (int)(signed short)LOWORD(lParam), (int)(signed short)HIWORD(lParam));
        break;


    case WM_MOUSEMOVE:
        MouseMove(GetWindowWord(hwnd, GWW_VIEW), wParam, (int)(signed short)LOWORD(lParam), (int)(signed short)HIWORD(lParam));
        break;

    case WM_MOUSEWHEEL:
    case WM_VSCROLL:
        VertScroll((WORD) GetWindowWord(hwnd, GWW_VIEW), message, wParam, lParam);
        break;

    case WM_HSCROLL:
        HorzScroll((WORD) GetWindowWord(hwnd, GWW_VIEW), wParam, lParam);
        break;

    case WM_KEYDOWN:
        if (!IsIconic(GetParent(hwnd))) {
            isShiftDown = (GetKeyState(VK_SHIFT) < 0);
            isCtrlDown = (GetKeyState(VK_CONTROL) < 0);

            // kcarlos
            // BUGBUG
            //
            // Ugly hack to make the Ctrl+V & Ctrl+X hot key works,
            // the following code appear to be eating it. This is temporary
            // since all of the UI will soon be redone.
            if (isCtrlDown && !isShiftDown) {
                if ('X' == wParam) {
                    PostMessage(hwndFrame, WM_COMMAND, MAKEWPARAM(IDM_EDIT_CUT, 1), 0);
                } else if ('V' == wParam) {
                    PostMessage(hwndFrame, WM_COMMAND, MAKEWPARAM(IDM_EDIT_PASTE, 1), 0);
                }
            }

            KeyDown((WORD) GetWindowWord(hwnd, GWW_VIEW), wParam,
                isShiftDown, isCtrlDown);
        }
        break;

    case WM_CHAR:

        if (!IsIconic(GetParent(hwnd))) {
            //Key is being pressed
            PressChar(hwnd, wParam, lParam);
        }
        break;

    case WM_IME_REPORT:
        if (IR_STRING == wParam) {
            return(ProccessIMEString(hwnd, lParam));
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
        break;

    case WM_ERASEBKGND:

        //Let WM_PAINT do the job
        return FALSE;

    case WM_PASTE:

        if (OpenClipboard(hwnd)) {
            HANDLE  hData;
            SIZE_T  size;
            LPSTR   p1;
            LPSTR   p;
            int     nLines, cCol;
            int     x, y;
            long    XL,XR;
            long    YL,YR;
            int     pos;



            hData = GetClipboardData(CF_TEXT);

            if (hData && (size = GlobalSize (hData))) {
                if (size >= MAX_CLIPBOARD_SIZE) {
                    ErrorBox(ERR_Clipboard_Overflow);
                } else if ( p = (PSTR) GlobalLock(hData) ) {

                    x = Views[curView].X;
                    y = Views[curView].Y;
                    p1 = p;
                    nLines = 0;

                    if (Views[curView].BlockStatus) {
                        GetBlockCoord(curView, &XL, &YL, &XR, &YR);
                        cCol = XL;
                        pos = y - (YR - YL);
                    } else {
                        cCol = x;
                        pos = y;
                    }
                    while (size && *p1) {
                        if (IsDBCSLeadByte(*p1) && size > 1) {
                            size -= 2;
                            p1 += 2;
                            continue;
                        } else if (*p1 == '\n') {
                            ++nLines;
                            cCol = 0;
                        } else if (*p1 != '\r') {
                            ++cCol;
                        } else {
                            ++nLines;
                            cCol = 0;
                            if (p1[1] == '\n') {
                                ++p1;
                                --size;
                            }
                        }
                        --size;
                        ++p1;
                    }
                    InsertStream(curView, x, y, (int) (p1-p), p, TRUE);
                    DbgX(GlobalUnlock (hData) == FALSE);
                    PosXY(curView, cCol, pos+nLines, TRUE);
                }
            }
            CloseClipboard();
        }
        return 0;

    case WM_DESTROY:
        DestroyView(GetWindowWord(hwnd, GWW_VIEW));
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return FALSE;
}

#ifdef PROFILER_HACK

/************ Profiler hacks ******************/

int EMFunc(int, int, int, int, int);
int TLFunc( int, int, int, int);
int DMFunc(int, int);
int EEInitializeExpr(int, int);
int SHInit(int, int);

Profile()
{
    EMFunc(0, 0, 0, 0, 0);
    TLFunc(0, 0, 0, 0);
    DMFunc(0, 0);
    EEInitializeExpr(0, 0);
    SHInit(0, 0);
}

#endif

