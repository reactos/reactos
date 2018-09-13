#include "precomp.h"
#pragma hdrstop


/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    OptSheet.c

Abstract:

    This module contains the code for Options property
    sheet dialog.

Author:

    Carlos Klapp (a-caklap) 31-July-1997

Environment:

    Win32, User Mode

--*/


#define DEFAULT_CMD_WINDOW_LOGFILENAME     "windbg.log"




/*
 * DlgProc prototype
 *
 *   BOOL CALLBACK DialogProc(
 *       HWND hwndDlg, // handle to dialog box
 *       UINT uMsg, // message
 *       WPARAM wParam, // first message parameter
 *       LPARAM lParam // second message parameter
 *   );
 */
INT_PTR CALLBACK DlgProc_Program(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Workspace(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_SourceFiles(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Debugger(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_KernelDebugger(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_TransportLayer(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Symbols(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Disassembler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_CallStack(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


// This is not directly used by the prop sheet but by the IDD_DLG_SRC_SEARCH_PATH dlg
// that is invoked from the DlgProc_SourceFiles function by pressing the "Search Order" button.
INT_PTR CALLBACK DlgProc_SrcSearchPath(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#include "include\cntxthlp.h"

extern LRESULT SendMessageNZ(HWND,UINT,WPARAM,LPARAM);

int NEAR PASCAL ChangeTabs(int doc, int newTabSize);


void
Init(
     HWND hwndOwner,
     HINSTANCE hinst,
     LPPROPSHEETHEADER lppsh,
     PROPSHEETPAGE apsp[],
     const int nNumPropPages
     )
/*++
Routine Description:
    Initializes the property sheet header and pages.

Arguments:
    hwndOwner
    hinst
        Are both used by the PROPSHEETHEADER & PROPSHEETPAGE structure.
        Please see the docs for the these structures for more info.

    lppsh
        Standard prop sheet structure.

    apsp[]
        An array of prop pages
        Standard prop sheet structure.

    nNumPropPages
        Number of prop pages in the "apsp" array.
--*/
{
    int nPropIdx;

    memset(lppsh, 0, sizeof(PROPSHEETHEADER));

    lppsh->dwSize = sizeof(PROPSHEETHEADER);
    lppsh->dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    lppsh->hwndParent = hwndOwner;
    lppsh->hInstance = hinst;
    lppsh->pszCaption = "Windows Debugger Options";
    lppsh->nPages = nNumPropPages;
    lppsh->ppsp = apsp;

    // Init the first one, then copy its contents to all the others
    memset(apsp, 0, sizeof(PROPSHEETPAGE));
    apsp[0].dwSize = sizeof(PROPSHEETPAGE);
//    apsp[0].dwFlags = PSP_HASHELP;
    apsp[0].hInstance = hinst;

    for (nPropIdx =1; nPropIdx < nNumPropPages; nPropIdx++) {
        memcpy(&(apsp[nPropIdx]), &apsp[0], sizeof(PROPSHEETPAGE));
    }


    // Only init the distinct values
    nPropIdx = 0;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_PROGRAM);
    apsp[nPropIdx].pfnDlgProc = DlgProc_Program;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 1;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_WORKSPACE);
    apsp[nPropIdx].pfnDlgProc = DlgProc_Workspace;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 2;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_SOURCEFILES);
    apsp[nPropIdx].pfnDlgProc = DlgProc_SourceFiles;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 3;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_DEBUGGER);
    apsp[nPropIdx].pfnDlgProc = DlgProc_Debugger;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 4;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_KERNELDEBUGGER);
    apsp[nPropIdx].pfnDlgProc = DlgProc_KernelDebugger;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 5;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_TRANSPORTLAYER);
    apsp[nPropIdx].pfnDlgProc = DlgProc_TransportLayer;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 6;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_SYMBOLS);
    apsp[nPropIdx].pfnDlgProc = DlgProc_Symbols;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 7;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_DISASSEMBLER);
    apsp[nPropIdx].pfnDlgProc = DlgProc_Disassembler;
    apsp[nPropIdx].lParam = 0;

    nPropIdx = 8;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_CALLSTACK);
    apsp[nPropIdx].pfnDlgProc = DlgProc_CallStack;
    apsp[nPropIdx].lParam = 0;

    Assert(nPropIdx < nNumPropPages);
}


INT_PTR
DisplayOptionsPropSheet(
                        HWND hwndOwner,
                        HINSTANCE hinst,
                        int nStartPage
                        )
/*++
Routine Description:
    Will Initialize and display the Options property sheet. Handle the return codes,
    and the commitment of changes to the debugger.

Arguments:
    hwndOwner
    hinst
        Are both used initialize the property sheet dialog.
    nStart - Is used to specify the page that is to be initially
        displayed when the prop sheet first appears. The default
        value is 0. The values specified correspond to array index
        of the PROPSHEETPAGE array.
--*/
{
    int nRes = 0;
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE apsp[9] = {0};
    int nNumPropPages = sizeof(apsp) / sizeof(PROPSHEETPAGE);

    Init(hwndOwner, hinst, &psh, apsp, nNumPropPages);

    psh.nStartPage = nStartPage;


    nRes = PropertySheet(&psh);


    if (IDOK == nRes) {
        g_Windbg_WkSp.Save(FALSE, FALSE);
    }

    return nRes;
}

INT_PTR
CALLBACK
DlgProc_Program(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPSTR lpszNewDb = NULL;
    static LPSTR lpszOldDb = NULL;

    static DWORD HelpArray[]=
    {
       IDC_EDIT_EXECUTABLE, IDH_EXE,
       IDC_STEXT_ARGUMENTS, IDH_ARGS,
       IDC_EDIT_ARGUMENTS, IDH_ARGS,
       ID_ENV_SRCHPATH, IDH_SPATH,
       IDC_EDIT_CRASH_DUMP, IDH_CRASHDUMP,
       0, 0
    };

#define ZFREE(P) ((P)?(free(P),((P)=NULL)):NULL)

    switch (uMsg) {
    case WM_DESTROY:
        ZFREE(lpszNewDb);
        ZFREE(lpszOldDb);
        break;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);  // notification code
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;       // handle of control
            BOOL bEnabled;

            switch(wID) {
            case ID_ENV_SRCHPATH:
                if (BN_CLICKED == wNotifyCode) {
                    BOOL b = IsDlgButtonChecked(hwndDlg, ID_ENV_SRCHPATH);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_EXECUTABLE_SEARCH_PATH), b);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_BROWSE), b);

                    return TRUE;
                }
                break;
            }
        }
        break;

    case WM_INITDIALOG:
        { // begin Prog & Arguments code block
            char szTmpLocal[_MAX_PATH];
            BOOL bCrashDump = g_contKernelDbgPreferences_WkSp.m_bUseCrashDump || g_contWorkspace_WkSp.m_bUserCrashDump;

            // Enable/disable the correct section, depending
            // on whether an executable or a crash dump is
            // being debugged.
            //EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_CRASH_DUMP), bCrashDump);
            EnableWindow(GetDlgItem(hwndDlg, IDC_GROUP_CRASH_DUMP), bCrashDump);

            EnableWindow(GetDlgItem(hwndDlg, ID_ENV_SRCHPATH), !bCrashDump);
            EnableWindow(GetDlgItem(hwndDlg, IDC_STEXT_ARGUMENTS), !bCrashDump);
            //EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_EXECUTABLE), !bCrashDump);
            EnableWindow(GetDlgItem(hwndDlg, IDC_GROUP_EXECUTABLE), !bCrashDump);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_ARGUMENTS), !bCrashDump);

            // Limit the argument to 4K
            SendDlgItemMessage(hwndDlg, IDC_EDIT_ARGUMENTS, EM_LIMITTEXT, 4096, 0);

            CheckDlgButton(hwndDlg, ID_ENV_SRCHPATH,
                g_contGlobalPreferences_WkSp.m_bSrchSysPathForExe ? BST_CHECKED : BST_UNCHECKED);
            PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(ID_ENV_SRCHPATH, BN_CLICKED),
                (LPARAM) GetDlgItem(hwndDlg, ID_ENV_SRCHPATH));

            if (bCrashDump) {
                LPSTR lpszCrashDumpName = NULL;

                // Which crash dump file do we need?
                if (g_contWorkspace_WkSp.m_bUserCrashDump) {
                    Assert(g_contWorkspace_WkSp.m_pszUserCrashDump);
                    lpszCrashDumpName = g_contWorkspace_WkSp.m_pszUserCrashDump;
                } else if (g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
                    Assert(g_contKernelDbgPreferences_WkSp.m_pszCrashDump);
                    lpszCrashDumpName = g_contKernelDbgPreferences_WkSp.m_pszCrashDump;
                } else {
                    Dbg(0);
                }

                SetDlgItemText(hwndDlg, IDC_EDIT_CRASH_DUMP, lpszCrashDumpName);
            } else {
                //
                // Parse out the parameters for debugging an executable
                //
                PCSTR psz = g_Windbg_WkSp.GetCurrentProgramName(TRUE);
                AdjustFullPathName(psz, szTmpLocal, 27);

                SetDlgItemText(hwndDlg, IDC_EDIT_EXECUTABLE, szTmpLocal);

                ZFREE(lpszNewDb);
                ZFREE(lpszOldDb);

                if (g_Windbg_WkSp.GetCurrentProgramName( FALSE )) {

                    if (!LpszCommandLine) {
                        LpszCommandLine = (PSTR) malloc(1);
                        *LpszCommandLine = 0;
                    }

                    psz = LpszCommandLine;
                    while (isspace(*psz)) {
                        psz++;
                    }

                    lpszOldDb = _strdup(psz);

                    SetDlgItemText(hwndDlg, IDC_EDIT_ARGUMENTS, lpszOldDb);
                }
            }
            return FALSE;
        } // end Prog & Arguments code block
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            return 1;
            break;

        case PSN_APPLY:
            {
                LPSTR lpszTmp;
                int nCnt;

                g_contGlobalPreferences_WkSp.m_bSrchSysPathForExe = (IsDlgButtonChecked(hwndDlg, ID_ENV_SRCHPATH)
                    == BST_CHECKED);

                nCnt= (int) SendDlgItemMessage(hwndDlg, IDC_EDIT_ARGUMENTS,
                    WM_GETTEXTLENGTH, 0, 0);

                lpszNewDb = (PSTR) malloc(nCnt + 1);
                GetDlgItemText(hwndDlg, IDC_EDIT_ARGUMENTS, lpszNewDb, nCnt +1);

                //strip out leading whitespace

                lpszTmp = lpszNewDb;
                while (isspace(*lpszTmp)) {
                    lpszTmp++;
                }

                if ( (NULL == lpszOldDb && *lpszTmp) // No previous args, but now we have args
                    || (lpszOldDb && strcmp (lpszTmp, lpszOldDb)) ) { // Test if prev and new args are dif.

                    SetProgramArguments(lpszTmp);
                }
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}


void
Workspace_EnableLoggingOptions(
    HWND hwndDlg,
    BOOL bEnable
    )
{
    EnableWindow(GetDlgItem(hwndDlg, ID_LFOPT_APPEND), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, ID_LFOPT_AUTO), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, ID_LFOPT_FNAME), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_START_LOGGING), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_STOP_LOGGING), !bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_BROWSE), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, ID_LFOPT_FNAMELABEL), bEnable);
}


void
Workspace_ApplyChanges(
    HWND hwndDlg
    )
{
    BOOL bChecked = FALSE;
    char sz[_MAX_PATH];


    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_AUTO_SAVE_WORKSPACE) ) {
            
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = TRUE;
        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = FALSE;

    } else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_PROMPT_BEFORE_SAVING_WORKSPACE)) {

        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = TRUE;
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = TRUE;

    } else {

        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = FALSE;
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = FALSE;

    }

    bChecked = (IsDlgButtonChecked(hwndDlg,ID_LFOPT_APPEND) == BST_CHECKED);
    if (bChecked != g_contWorkspace_WkSp.m_bLfOptAppend) {
        g_contWorkspace_WkSp.m_bLfOptAppend = bChecked;
    }

    bChecked = (IsDlgButtonChecked(hwndDlg,ID_LFOPT_AUTO) == BST_CHECKED);
    if (bChecked != g_contWorkspace_WkSp.m_bLfOptAuto) {
        g_contWorkspace_WkSp.m_bLfOptAuto = bChecked;
    }

    GetDlgItemText(hwndDlg, ID_LFOPT_FNAME, sz, sizeof(sz));

    if (strlen(sz) == 0) {
        Assert(strlen(DEFAULT_CMD_WINDOW_LOGFILENAME) < sizeof(sz));
        strcpy( sz, DEFAULT_CMD_WINDOW_LOGFILENAME );
    } 
    
    if (0 == g_contWorkspace_WkSp.m_pszLogFileName
        || strcmp(sz, g_contWorkspace_WkSp.m_pszLogFileName)) {

        FREE_STR(g_contWorkspace_WkSp.m_pszLogFileName);
        g_contWorkspace_WkSp.m_pszLogFileName = _strdup(sz);
    }
}


INT_PTR
CALLBACK
DlgProc_Workspace(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    extern HANDLE hFileLog;
    extern DOT_COMMAND DotTable[];

    static DWORD HelpArray[]=
    {
        IDC_RADIO_NEVER_SAVE_WORKSPACE,             IDH_MANUALSAVE,
        IDC_RADIO_AUTO_SAVE_WORKSPACE,              IDH_AUTOSAVE,
        IDC_RADIO_PROMPT_BEFORE_SAVING_WORKSPACE,   IDH_PROMPT,
        ID_LFOPT_AUTO,                              IDH_AUTOOPEN,
        ID_LFOPT_APPEND,                            IDH_APPENDLOG,
        IDC_BUT_START_LOGGING,                      IDH_STARTLOG,
        IDC_BUT_STOP_LOGGING,                       IDH_STOPLOG,
        ID_LFOPT_FNAMELABEL,                        IDH_LOGFILE,
        ID_LFOPT_FNAME,                             IDH_LOGFILE,
        IDC_BUT_BROWSE,                             IDH_BROWSELOG,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwndDlg, ID_LFOPT_FNAME, EM_SETLIMITTEXT, _MAX_PATH, 0);

        Workspace_EnableLoggingOptions(hwndDlg, INVALID_HANDLE_VALUE == hFileLog);

        // Figure what option to enable
        {
            int nId = IDC_RADIO_NEVER_SAVE_WORKSPACE;
            
            if (g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace) {
                nId = IDC_RADIO_AUTO_SAVE_WORKSPACE;
                
                if (g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace) {
                    nId = IDC_RADIO_PROMPT_BEFORE_SAVING_WORKSPACE;
                }
            }
            
            CheckRadioButton(hwndDlg, 
                             IDC_RADIO_NEVER_SAVE_WORKSPACE,
                             IDC_RADIO_AUTO_SAVE_WORKSPACE, 
                             nId
                             );
        }

        CheckDlgButton(hwndDlg,ID_LFOPT_APPEND,
            g_contWorkspace_WkSp.m_bLfOptAppend ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg,ID_LFOPT_AUTO,
            g_contWorkspace_WkSp.m_bLfOptAuto ? BST_CHECKED : BST_UNCHECKED);

        // Don't just leave it blank, put something in there.
        SetDlgItemText(hwndDlg, ID_LFOPT_FNAME,
            (g_contWorkspace_WkSp.m_pszLogFileName 
                && *g_contWorkspace_WkSp.m_pszLogFileName) 
                ? g_contWorkspace_WkSp.m_pszLogFileName :
                DEFAULT_CMD_WINDOW_LOGFILENAME);
        return FALSE;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;      // handle of control

            switch (wID) {
            case IDC_BUT_DEFAULT_FONT:
                switch(wNotifyCode) {
                case BN_CLICKED:
                    {
                        CHOOSEFONT cf;

                        memset(&cf, 0, sizeof(cf));

                        cf.lStructSize = sizeof(cf);
                        cf.hwndOwner = hwndDlg;
                        //cf.hDC;
                        //cf.lpLogFont;
                        //cf.iPointSize;
                        cf.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS;
                        //cf.rgbColors;
                        //cf.lCustData;
                        //cf.lpfnHook;
                        //cf.lpTemplateName;
                        //cf.hInstance;
                        //cf.lpszStyle;
                        //cf.nFontType;
                        //cf.___MISSING_ALIGNMENT__;
                        //cf.nSizeMin;
                        //cf.nSizeMax;

                        if (ChooseFont(&cf)) {
                        }

                        return TRUE;
                    }
                    break;
                }
                break;

            case IDC_BUT_BROWSE:
                switch(wNotifyCode) {
                case BN_CLICKED:
                    {
                        char sz[MAX_PATH];
                        LPSTR lpsz = sz;
                        DWORD dw = OFN_HIDEREADONLY;

                        GetDlgItemText(hwndDlg, ID_LFOPT_FNAME, sz, sizeof(sz));

                        if (StartFileDlg(
                            hwndDlg,                    // parent
                            DLG_Browse_LogFile_Title,   // Title
                            DEF_Ext_LOG,                // Extension
                            IDM_FILE_OPEN,              // kcarlos BUGBUG: Need to get its own helpID
                            0,                          // No temnplate
                            sz,                         // Path/file
                            &dw,                        // Flags
                            DlgFile)) {

                            // File found
                            // Strip white space
                            while (isspace(*lpsz)) {
                                lpsz++;
                            }

                            // If it's an empty string, then we do nothing
                            if (*lpsz) {
                                FREE_STR(g_contWorkspace_WkSp.m_pszLogFileName);
                                g_contWorkspace_WkSp.m_pszLogFileName = _strdup(lpsz);
                            }
                        }
                    }
                    return TRUE;
                }
                break;

            case IDC_BUT_START_LOGGING:
                switch (wNotifyCode) {
                case BN_CLICKED:
                    {
                        DOTHANDLER lpfnHandler = NULL;
                        DWORD nIdx;
                        char sz[MAX_PATH + 100];

                        Workspace_EnableLoggingOptions(hwndDlg, FALSE);
                        Workspace_ApplyChanges(hwndCtl);

                        // Search for the function so that the code will
                        // not break everytime we insert dot commands.
                        if (g_contWorkspace_WkSp.m_bLfOptAppend) {
                            lpfnHandler = LogFileOpen; // append
                        } else {
                            lpfnHandler = LogFileOpen; // open
                        }

                        for (nIdx = 0; nIdx < dwSizeofDotTable; nIdx++) {
                            if (lpfnHandler == DotTable[nIdx].lpfnHandler) {
                                break;
                            }
                        }
                        // Make sure it was found
                        Assert(nIdx != dwSizeofDotTable);

                        sprintf(sz,
                                ".%s %s\r\n",
                                DotTable[nIdx].lpName,
                                g_contWorkspace_WkSp.m_pszLogFileName);
                        Assert(strlen(sz) < sizeof(sz));

                        CmdDoLine(sz);
                    }
                    return TRUE;
                }
                break;

            case IDC_BUT_STOP_LOGGING:
                switch (wNotifyCode) {
                case BN_CLICKED:
                    {
                        DWORD nIdx;
                        char sz[100];

                        // Search for the function so that the code will
                        // not break everytime we insert dot commands.
                        for (nIdx = 0; nIdx < dwSizeofDotTable; nIdx++) {
                            if (LogFileClose == DotTable[nIdx].lpfnHandler) {
                                break;
                            }
                        }
                        // Make sure it was found
                        Assert(nIdx != dwSizeofDotTable);

                        sprintf(sz,
                                ".%s\r\n",
                                DotTable[nIdx].lpName);
                        Assert(strlen(sz) < sizeof(sz));
                        CmdDoLine(sz);
                        Workspace_EnableLoggingOptions(hwndDlg, TRUE);
                    }
                    return TRUE;
                }
                break;
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            // Nothing to validate
            break;

        case PSN_APPLY:
            Workspace_ApplyChanges(hwndDlg);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


INT_PTR
CALLBACK
DlgProc_SourceFiles(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_ENV_TABSTOPSTXT, IDH_TABSTOP,
       IDC_EDIT_TAB_STOPS, IDH_TABSTOP,
       ID_ENV_SCROLLGROUP, IDH_SCROLLBAR,
       ID_ENV_SCROLLVER, IDH_SCROLLBAR,
       ID_ENV_SCROLLHOR, IDH_SCROLLBAR,
       IDC_BUT_SEARCH_ORDER, IDH_SEARCHORDER,
       0, 0
    };

    switch (uMsg) {
    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);  // notification code
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;       // handle of control
            BOOL bEnabled;

            switch(wID) {
            case IDC_BUT_SEARCH_ORDER:
                if (BN_CLICKED == wNotifyCode) {
                    StartDialog(IDD_DLG_SRC_SEARCH_PATH, DlgProc_SrcSearchPath);
                    return TRUE;
                }
                break;
            }
        }
        break;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_INITDIALOG:
        { // tmp code block
            char sz[4];

            sprintf(sz, "%d", g_contGlobalPreferences_WkSp.m_nTabSize);
            Assert(strlen(sz) < sizeof(sz));
            SetDlgItemText(hwndDlg, IDC_EDIT_TAB_STOPS, sz);

            // Protect against a stupid user.
            SendDlgItemMessage(hwndDlg, IDC_EDIT_TAB_STOPS, EM_SETLIMITTEXT, 2, 0);
        } // end tmp code block

        SendDlgItemMessage(hwndDlg, ID_ENV_SCROLLHOR,
            BM_SETCHECK, g_contGlobalPreferences_WkSp.m_bHorzScrollBars, 0L );

        SendDlgItemMessage(hwndDlg, ID_ENV_SCROLLVER,
            BM_SETCHECK, g_contGlobalPreferences_WkSp.m_bVertScrollBars, 0L );
        return FALSE;


    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_KILLACTIVE:
            {
                char sz[4];
                int i;

                GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_TAB_STOPS), sz, sizeof(sz));
                Assert(strlen(sz) < sizeof(sz));

                i = atoi(sz);
                if (1 <= i && i <= MAX_TAB_WIDTH) {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                } else {
                    MessageBeep(MB_ICONEXCLAMATION);
                    ErrorBox(ERR_Tabs_OutOfRange, 1, MAX_TAB_WIDTH);
                    SetFocus(GetDlgItem(hwndDlg, ID_ENV_TABSTOPS));

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                }
                return 1;
            }
            break;

        case PSN_APPLY:
            { // Begin Process tabs
                char sz[4];
                int nTabStops;

                GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_TAB_STOPS), sz, sizeof(sz));
                Assert(strlen(sz) < sizeof(sz));

                nTabStops = atoi(sz);

                if ( nTabStops != g_contGlobalPreferences_WkSp.m_nTabSize) {
                    int nDoc, nView;

                    for (nDoc = 0; nDoc < MAX_DOCUMENTS; nDoc++) {
                        if (Docs[nDoc].FirstView != -1) {
                            int nLine = ChangeTabs(nDoc, nTabStops);
                            if (nLine > 0) {
                                SendDlgItemMessage(hwndDlg, ID_ENV_TABSTOPS, EM_SETSEL, 0, MAKELONG(0, 32767));
                                ErrorBox(ERR_Tab_Too_Big, nLine, Docs[nDoc].szFileName);
                                SetFocus(GetDlgItem(hwndDlg, ID_ENV_TABSTOPS));
                                return TRUE;
                            }
                        }
                    }

                    g_contGlobalPreferences_WkSp.m_nTabSize = nTabStops;

                    for (nView = 0; nView < MAX_VIEWS; nView++) {
                        if (Views[nView].Doc >= 0) {
                            InvalidateLines( nView, 0, Docs[Views[nView].Doc].NbLines - 1, FALSE );
                        }
                    }
                }


                g_contGlobalPreferences_WkSp.m_bHorzScrollBars = (SendMessage( GetDlgItem(hwndDlg, ID_ENV_SCROLLHOR),
                    BM_GETCHECK, 0, 0L ) == 1);

                g_contGlobalPreferences_WkSp.m_bVertScrollBars = (SendMessage( GetDlgItem(hwndDlg, ID_ENV_SCROLLVER),
                    BM_GETCHECK, 0, 0L ) == 1);
            } // End process tabs


            { // Begin process scroll bars
                BOOL bHorzScroll = (SendMessage( GetDlgItem(hwndDlg, ID_ENV_SCROLLHOR),
                    BM_GETCHECK, 0, 0L ) == 1);

                BOOL bVertScroll = (SendMessage( GetDlgItem(hwndDlg, ID_ENV_SCROLLVER),
                    BM_GETCHECK, 0, 0L ) == 1);

                if ( bHorzScroll != g_contGlobalPreferences_WkSp.m_bHorzScrollBars ||
                    bVertScroll != g_contGlobalPreferences_WkSp.m_bVertScrollBars ) {
                    int nView;

                    for (nView = 0; nView < MAX_VIEWS; nView++) {
                        if (Views[nView].Doc >= 0) {
                            EnsureScrollBars(nView, FALSE);
                            PosXY(nView, Views[nView].X, Views[nView].Y, FALSE);
                        }
                    }
                }
            } // End process scroll bars

            return TRUE;
            break;
        }
        break;
    }

    return FALSE;
}

INT_PTR
CALLBACK
DlgProc_Debugger(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_DBUGOPT_CASESENSITIVE, IDH_IGNORECASE,
       ID_DBUGOPT_CHILDREN, IDH_CHILD,
       ID_DBUGOPT_ATTACHGO, IDH_GOATTACH,
       ID_DBUGOPT_COMMANDREPEAT, IDH_CMDREP,
       ID_DBUGOPT_DISCONNECT, IDH_DISCONNECT,
       ID_DBUGOPT_VERBOSE, IDH_VERBOSE,
       ID_DBUGOPT_DISPSEG, IDH_SEGMENT,
       ID_DBUGOPT_EXITGO, IDH_GTERM,
       ID_DBUGOPT_CHILDGO, IDH_GCREATE,
       ID_DBUGOPT_WOWVDM, IDH_WOW,
       ID_DBUGOPT_CONTEXT, IDH_CONTEXT,
       ID_DBUGOPT_MASM_EVAL, IDH_MASMEVAL,
       ID_DBUGOPT_REGISTERGROUP, IDH_REG,
       ID_DBUGOPT_REGREG, IDH_REG,
       ID_DBUGOPT_REGEXT, IDH_REG,
       ID_DBUGOPT_REGMMU, IDH_REG,
       ID_DBUGOPT_RADIXGROUP, IDH_RADIX,
       ID_DBUGOPT_RADIXOCT, IDH_RADIX,
       ID_DBUGOPT_RADIXDEC, IDH_RADIX,
       ID_DBUGOPT_RADIXHEX, IDH_RADIX,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_CASESENSITIVE,
            !fCaseSensitive ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_CHILDREN,
            g_contWorkspace_WkSp.m_bDebugChildren ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckRadioButton(hwndDlg, ID_DBUGOPT_REGREG, ID_DBUGOPT_REGEXT,
            g_contWorkspace_WkSp.m_bRegModeExt ? ID_DBUGOPT_REGEXT : ID_DBUGOPT_REGREG) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_REGMMU,
            g_contWorkspace_WkSp.m_bRegModeMMU ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_MASM_EVAL,
            g_contWorkspace_WkSp.m_bMasmEval ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_DISPSEG,
            g_contWorkspace_WkSp.m_bShowSegVal ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_EXITGO,
            g_contWorkspace_WkSp.m_bGoOnThreadTerm ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_ATTACHGO,
            g_contWorkspace_WkSp.m_bAttachGo ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_CHILDGO,
            g_contWorkspace_WkSp.m_bChildGo ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_COMMANDREPEAT,
            g_contGlobalPreferences_WkSp.m_bCommandRepeat ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_DISCONNECT,
            g_contWorkspace_WkSp.m_bDisconnectOnExit ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_VERBOSE,
            g_contWorkspace_WkSp.m_bVerbose ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_CONTEXT,
            g_contWorkspace_WkSp.m_bShortContext ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckDlgButton(hwndDlg, ID_DBUGOPT_WOWVDM,
            g_contWorkspace_WkSp.m_bWowVdm ? BST_CHECKED : BST_UNCHECKED) );

        Dbg( CheckRadioButton(hwndDlg, ID_DBUGOPT_RADIXOCT, ID_DBUGOPT_RADIXHEX,
            radix == 8 ?  ID_DBUGOPT_RADIXOCT
            : (radix == 10 ? ID_DBUGOPT_RADIXDEC : ID_DBUGOPT_RADIXHEX) ) );

        return FALSE;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_APPLY:
            {
                //
                // Transfer the options to global
                //
                // Now start looking at the rest and determine what needs to
                // be updated
                //

                BOOL b = FALSE;
                BOOL bUpdateCpu = FALSE;
                int nUpdate = UPDATE_NONE;

                b = (IsDlgButtonChecked(hwndDlg, ID_DBUGOPT_MASM_EVAL) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bMasmEval) {
                    if (b) {
                        CmdDoLine(".opt masmeval on\r\n");
                    } else {
                        CmdDoLine(".opt masmeval off\r\n");
                    }
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_REGMMU) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bRegModeMMU) {
                    g_contWorkspace_WkSp.m_bRegModeMMU = b;
                    bUpdateCpu = TRUE;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_REGEXT) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bRegModeExt) {
                    g_contWorkspace_WkSp.m_bRegModeExt = b;
                    bUpdateCpu = TRUE;
                }

                //
                // If the register options changed then we need to get
                // it to refresh the list of registers in the windows
                //

                if (bUpdateCpu && HModEM) {
                    SendMessageNZ( GetCpuHWND(), WU_DBG_LOADEM, 0, 0);
                    SendMessageNZ( GetFloatHWND(), WU_DBG_LOADEM, 0, 0);
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_DISPSEG) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bShowSegVal) {
                    g_contWorkspace_WkSp.m_bShowSegVal = b;
                    nUpdate = UPDATE_WATCH|UPDATE_LOCALS|UPDATE_MEMORY;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_CHILDREN) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bDebugChildren) {
                    g_contWorkspace_WkSp.m_bDebugChildren = b;
                }

                b = (IsDlgButtonChecked( hwndDlg, ID_DBUGOPT_CASESENSITIVE) == BST_UNCHECKED);
                if (b != fCaseSensitive) {
                    fCaseSensitive = (char) b;
                    nUpdate = UPDATE_WATCH|UPDATE_LOCALS|UPDATE_MEMORY;
                }

                b = (IsDlgButtonChecked( hwndDlg, ID_DBUGOPT_EXITGO) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bGoOnThreadTerm) {
                    g_contWorkspace_WkSp.m_bGoOnThreadTerm = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_CHILDGO) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bChildGo) {
                    g_contWorkspace_WkSp.m_bChildGo = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_ATTACHGO) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bAttachGo) {
                    g_contWorkspace_WkSp.m_bAttachGo = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_COMMANDREPEAT) == BST_CHECKED);
                if (b != g_contGlobalPreferences_WkSp.m_bCommandRepeat) {
                    g_contGlobalPreferences_WkSp.m_bCommandRepeat = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_DISCONNECT) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bDisconnectOnExit) {
                    g_contWorkspace_WkSp.m_bDisconnectOnExit = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_VERBOSE) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bVerbose) {
                    g_contWorkspace_WkSp.m_bVerbose = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_IGNOREALL) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors) {
                    g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_CONTEXT) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bShortContext) {
                    g_contWorkspace_WkSp.m_bShortContext = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_DBUGOPT_WOWVDM) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bWowVdm) {
                    g_contWorkspace_WkSp.m_bWowVdm = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_LFOPT_APPEND) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bLfOptAppend) {
                    g_contWorkspace_WkSp.m_bLfOptAppend = b;
                }

                b = (IsDlgButtonChecked(hwndDlg,ID_LFOPT_AUTO) == BST_CHECKED);
                if (b != g_contWorkspace_WkSp.m_bLfOptAuto) {
                    g_contWorkspace_WkSp.m_bLfOptAuto = b;
                }

                //
                //  Get the correct value for the new radix.  The routine
                //  UpdateRadix will conditionally update the global radix
                //

                if (IsDlgButtonChecked(hwndDlg, ID_DBUGOPT_RADIXOCT)) {
                    UpdateRadix(8);
                } else if (IsDlgButtonChecked(hwndDlg, ID_DBUGOPT_RADIXDEC)) {
                    UpdateRadix(10);
                } else {
                    UpdateRadix(16);
                }

                //
                // See if we have changed any options.
                //

                if (nUpdate != UPDATE_NONE) {
                    UpdateDebuggerState(nUpdate);
                }

                return TRUE;
            }
            break;
        }
        break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
DlgProc_Disassembler(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_DISASMOPT_SHOWSEG, IDH_SEGMENT,
       ID_DISASMOPT_SHOWBYTE, IDH_DRAW,
       ID_DISASMOPT_SHOWSYMB, IDH_DSYM,
       ID_DISASMOPT_CASE, IDH_DUC,
       ID_DISASMOPT_NEVER_OPEN_AUTOMATICALLY, IDH_DISASM,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        if (!(g_contWorkspace_WkSp.m_dopDisAsmOpts & dopFlatAddr)) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_SHOWSEG, BST_CHECKED);
        }

        if (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopRaw) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_SHOWBYTE, BST_CHECKED);
        }

        if (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopUpper) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_CASE, BST_CHECKED);
        }

        if (g_contWorkspace_WkSp.m_dopDisAsmOpts & 0x800) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_SHOWSOURCE, BST_CHECKED);
        }

        if (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopSym) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_SHOWSYMB, BST_CHECKED);
        }

        if (g_contWorkspace_WkSp.m_dopDisAsmOpts & dopNeverOpenAutomatically) {
            CheckDlgButton(hwndDlg, ID_DISASMOPT_NEVER_OPEN_AUTOMATICALLY, BST_CHECKED);
        }
        return FALSE;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_APPLY:
            {
                DWORD dwDisAsmOpts = 0;

                //
                // Check to see what disassembler options have changed
                //  and update the disasm window if necessary.
                //

                if (!IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_SHOWSEG)) {
                    dwDisAsmOpts |= dopFlatAddr;
                }

                if (IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_SHOWBYTE)) {
                    dwDisAsmOpts |= dopRaw;
                }

                if (IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_CASE)) {
                    dwDisAsmOpts |= dopUpper;
                }

                if (IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_SHOWSOURCE)) {
                    dwDisAsmOpts |= 0x800;
                }

                if (IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_SHOWSYMB)) {
                    dwDisAsmOpts |= dopSym;
                }

                if (IsDlgButtonChecked(hwndDlg, ID_DISASMOPT_NEVER_OPEN_AUTOMATICALLY)) {
                    dwDisAsmOpts |= dopNeverOpenAutomatically;
                }

                if (dwDisAsmOpts != g_contWorkspace_WkSp.m_dopDisAsmOpts) {
                    g_contWorkspace_WkSp.m_dopDisAsmOpts = dwDisAsmOpts;
                    if (disasmView != -1) {
                        ViewDisasm(NULL, disasmRefresh);
                    }
                }
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

