//////////////////////////////////////////////////////////////////////////////
//
//  KBDSEL.C -
//
//      Windows Keyboard Select Source File
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <ddeml.h>
#include "kbdsel.h"
#include "kbddll.h"


// Toggle Hot Key value

#define HOT_KEY_ID   0x0722

// Global Variables

HINSTANCE hInst;
HWND      hwndKbd = NULL;

TCHAR szPrimaryKbd[KL_NAMELENGTH];
TCHAR szAlternateKbd[KL_NAMELENGTH];
TCHAR szPrimaryName[MAX_KEYNAME];
TCHAR szAlternateName[MAX_KEYNAME];

TCHAR szCaption[50];
TCHAR szOnTop[50];
TCHAR szOnBottom[50];
TCHAR szDisable[50];
TCHAR szEnable[50];
TCHAR szRegNamePrimary[50];
TCHAR szRegNameAlternate[50];
TCHAR szRegNameHotKey[50];
TCHAR szRegNameKeyboard[50];

BOOL bFirstTime;
BOOL bKbdDisabled;
BOOL bAddStartupGrp;
WORD fHotKeyCombo;
WORD fOnTop;

// Local Function Prototypes

HWND          InitApplication (VOID);
BOOL          UpdateWinTitle  (VOID);
UINT APIENTRY MainWndProc     (HWND, UINT, WPARAM, LPARAM);

BOOL              SetKbdHook         (BOOL);
BOOL              AddAppToStartupGrp (VOID);
HDDEDATA CALLBACK DdeCallback        (UINT, UINT, HCONV, HSZ, HSZ,
                                      HDDEDATA, DWORD, DWORD);
BOOL              RunCPIntlApp       (VOID);


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: calls initialization function, processes message loop
//
//////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR     lpCmdLine,
                      int       nCmdShow )
{
    MSG    msg;
    TCHAR  szBuf[MAX_PATH];
    HMENU  hmenu;


    hInst = hInstance;

    //
    // Exit if unable to initialize or another copy exist
    //
    if (!(hwndKbd = InitApplication ()))
       return (FALSE);

    //
    // load registry key names, menu names
    //
    LoadString (hInst, IDS_ONTOP, szOnTop, 50);
    LoadString (hInst, IDS_ONBOTTOM, szOnBottom, 50);
    LoadString (hInst, IDS_KEYBOARD, szRegNameKeyboard, 50);
    LoadString (hInst, IDS_PRIMARY_KL, szRegNamePrimary, 50);
    LoadString (hInst, IDS_ALTERNATE_KL, szRegNameAlternate, 50);
    LoadString (hInst, IDS_HOTKEYS, szRegNameHotKey, 50);
    LoadString (hInst, IDS_DISABLE, szDisable, 50);
    LoadString (hInst, IDS_ENABLE, szEnable, 50);

    //
    // Run configuration dialog and update window title
    //
    if (!DialogBox (hInst, (LPTSTR)MAKEINTRESOURCE(KBDCONFDLG), hwndKbd,
                    (DLGPROC)KbdConfDlgProc))
        goto CleanUp;

    UpdateWinTitle ();

    //
    // Add the application to the Startup group in the Program Manager
    //
    if (bAddStartupGrp)
        AddAppToStartupGrp ();

    //
    // Initialize and set up the message hook
    //
    InitKbdHook (hwndKbd, fHotKeyCombo);

    if (!SetKbdHook (TRUE))
    {
        LoadString (NULL, IDS_CANNOTSETHOOK, szErrorBuf, MAX_ERRBUF);
        MessageBox (NULL, szErrorBuf, szCaption,
                    MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
        return (FALSE);
    }

    bFirstTime = FALSE;

    //
    // Modify the system menu by removing Size, Minimize and Maximize items
    // and appending Primary, Alternate, Other and Disable items
    //
    hmenu = GetSystemMenu (hwndKbd, FALSE);

    AppendMenu (hmenu, MF_SEPARATOR, 0, NULL);
    LoadString (hInst, IDS_PRIMARY, szBuf, MAX_PATH);
    AppendMenu (hmenu, MF_STRING, IDM_PRIMARY, szBuf);
    LoadString (hInst, IDS_ALTERNATE, szBuf, MAX_PATH);
    AppendMenu (hmenu, MF_STRING, IDM_ALTERNATE, szBuf);
    LoadString (hInst, IDS_OTHER, szBuf, MAX_PATH);
    AppendMenu (hmenu, MF_STRING, IDM_OTHER, szBuf);
    AppendMenu (hmenu, MF_STRING, IDM_ONTOP, fOnTop?szOnBottom:szOnTop);
    AppendMenu (hmenu, MF_STRING, IDM_DISABLE, szDisable);

    RemoveMenu (hmenu, SC_SIZE, MF_BYCOMMAND);
    RemoveMenu (hmenu, SC_MINIMIZE, MF_BYCOMMAND);
    RemoveMenu (hmenu, SC_MAXIMIZE, MF_BYCOMMAND);

    if (fOnTop)
	SetWindowPos(hwndKbd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    //
    // Clean up
    //
CleanUp:
    if (hkeyKeyboard)
        RegCloseKey (hkeyKeyboard);
    if (hkeySelection) {
	wsprintf (szBuf, TEXT("%d\0"), fOnTop);
	RegSetValueEx (hkeySelection, szOnTop, 0, REG_SZ, (LPBYTE)szBuf,
		       lstrlen(szBuf) * sizeof(TCHAR));
        RegCloseKey (hkeySelection);
    }
    if (pLayouts)
        LocalFree ((HANDLE)pLayouts);

    if (IsWindow (hwndKbd))
    {
        if (DestroyWindow (hwndKbd))
            hwndKbd = NULL;
        SetKbdHook (FALSE);
    }

    return (msg.wParam);

    hPrevInstance;  //UNUSED
    nCmdShow;       //UNUSED
    lpCmdLine;      //UNUSED
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: InitApplication()
//
//  PURPOSE: If first time, registers window class and create window else
//           maximize and set focus on the existing copy
//
//////////////////////////////////////////////////////////////////////////////

HWND InitApplication( VOID )
{
    HWND      hwnd;
    HWND      hwndPrev;
    WNDCLASS  KbdSelClass;


    //
    // Check to see if another copy of kbdsel is running.  If
    // so, either bring it to the foreground, or restore it to
    // a window (from an icon).
    //
    hwndPrev = FindWindow (KBDCLASSNAME, NULL);

    if (hwndPrev != NULL)
    {
        hwnd = GetLastActivePopup (hwndPrev);

        if (hwnd == hwndPrev)
           SendMessage (hwndPrev, WM_SYSCOMMAND, SC_RESTORE, 0L);

        SetForegroundWindow (hwnd);
        return (NULL);
    }

    //
    // Initialize global variables
    //
    memset (szPrimaryKbd, 0, KL_NAMELENGTH * sizeof(TCHAR));
    memset (szAlternateKbd, 0, KL_NAMELENGTH * sizeof(TCHAR));
    memset (szPrimaryName, 0, MAX_KEYNAME * sizeof(TCHAR));
    memset (szAlternateName, 0, MAX_KEYNAME * sizeof(TCHAR));

    bFirstTime     = TRUE;
    bKbdDisabled   = FALSE;
    bAddStartupGrp = FALSE;
    fHotKeyCombo   = ALT_SHIFT_COMBO;

    hkeyKeyboard  = NULL;
    hkeySelection = NULL;

    LoadString (hInst, IDS_CAPTION, szCaption, 50);

    //
    // Fill in window class structure with parameters that describe the
    // main window.
    //
    KbdSelClass.cbClsExtra = KbdSelClass.cbWndExtra = 0;

    KbdSelClass.lpszClassName = KBDCLASSNAME;
    KbdSelClass.lpszMenuName  = NULL;
    KbdSelClass.hbrBackground = (HBRUSH) NULL;
    KbdSelClass.style         = CS_DBLCLKS;
    KbdSelClass.hInstance     = hInst;
    KbdSelClass.lpfnWndProc   = (WNDPROC)MainWndProc;

    KbdSelClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    KbdSelClass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE(ICON_ID));

    //
    // Register the window class and return FALSE if failure
    //
    if (!RegisterClass (&KbdSelClass))
        return (FALSE);

    //
    // Create and show a top most window in iconic form
    //
    if (hwnd = CreateWindowEx (fOnTop?WS_EX_TOPMOST:0,
                               KBDCLASSNAME,
                               KBDWINDOWNAME,
                               WS_TILEDWINDOW | WS_MINIMIZE,
                               10,
                               10,
                               100,
                               100,
                               NULL,
                               NULL,
                               hInst,
                               (LPVOID)NULL))
        ShowWindow (hwnd, SW_MINIMIZE);

    //
    // Register the toggle hot key (Ctrl-Alt-Space combination)
    //
    if (!RegisterHotKey (hwnd, HOT_KEY_ID, MOD_CONTROL | MOD_ALT, VK_SPACE))
        return (FALSE);

    return (hwnd);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: UpdateWinTitle(VOID)
//
//  PURPOSE: Update the window title with active keyboard name
//
//////////////////////////////////////////////////////////////////////////////

BOOL UpdateWinTitle( VOID )
{
    TCHAR  szName[MAX_KEYNAME];
    LPTSTR psz = NULL;

    if (szPrimaryKbd)
    {
        GetKeyboardLayoutName (szName);
        if (!lstrcmp (szName, szPrimaryKbd))
            psz = szPrimaryName;
        else if (!lstrcmp (szName, szAlternateName))
            psz = szAlternateName;
        else
        {
            LPTSTR  pszTemp = pLayouts;
            LPTSTR  pszFileName, pszOption, pszOptionText;

            while (*(pszTemp + 1) != TEXT('\0'))
            {
                //
                //  Get a ptr to each of item in the triplet strings returned
                //
                pszOption     = pszTemp;
                pszOptionText = pszOption + lstrlen (pszOption) + 1;
                pszFileName   = pszOptionText + lstrlen (pszOptionText) + 1;

                if (!lstrcmpi (pszOption, szName))
                {
                    psz = pszOptionText;
                    break;
                }

                //
                //  Point to next triplet
                //
                pszTemp = pszFileName + lstrlen (pszFileName) + 1;
            }
        }
    }
    else
        psz = KBDWINDOWNAME;

    if (!SetWindowText (hwndKbd, psz))
    {
        DWORD error;

        error = GetLastError ();
    }
    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: AddAppToStartupGrp(VOID)
//
//  PURPOSE: Adds Kbdsel app to the Startup group in Program Manager.
//
//////////////////////////////////////////////////////////////////////////////

BOOL AddAppToStartupGrp( VOID )
{
    HDDEDATA    hData;
    TCHAR       szCommand[512];
    TCHAR       szExePath[256];
    HCONV       hConv;
    HSZ         hszProgMan = 0L;
    LONG        lIdInst = 0L;
    LONG        lResult;
    BOOL        ret = FALSE;
    CONVCONTEXT CCFilter;

    //
    // Initialize the DDE conversation here
    //
    if (DdeInitialize (&lIdInst, (PFNCALLBACK) DdeCallback,
                       APPCMD_CLIENTONLY, 0L) != DMLERR_NO_ERROR)
        goto CleanUp;

    //
    // Create a DDEML string handle for the Program Manager.
    //
#ifdef UNICODE
    hszProgMan = DdeCreateStringHandle (lIdInst, TEXT("PROGMAN"), CP_WINUNICODE);
#else
    hszProgMan = DdeCreateStringHandle (lIdInst, TEXT("PROGMAN"), CP_WINANSI);
#endif
    if (!hszProgMan)
        goto CleanUp;

    //
    // Establish a conversation with the Program Manager
    //
    memset (&CCFilter, 0, sizeof(CCFilter));
    CCFilter.cb = sizeof(CCFilter);
#ifdef UNICODE
    CCFilter.iCodePage = CP_WINUNICODE;
#else
    CCFilter.iCodePage = CP_WINANSI;
#endif
    hConv = DdeConnect (lIdInst, hszProgMan, hszProgMan, &CCFilter);

    //
    // Create the Startup group by command line
    //   - program manager will activate an existing group if the name matches
    //
    LoadString(NULL, IDS_STARTUP, szExePath, 256);
    wsprintf (szCommand, TEXT("[CreateGroup(%s, 0)]"), szExePath);

    //
    // Create a DDEML data handle for the command
    //
    hData = DdeCreateDataHandle (lIdInst, (LPBYTE) szCommand,
                                 (lstrlen (szCommand) + 1) * sizeof(TCHAR),
#ifdef UNICODE
                                 0, (HSZ) NULL, CF_UNICODETEXT, 0L);
#else
                                 0, (HSZ) NULL, CF_TEXT, 0L);
#endif

    //
    // Send the transaction to the server waiting a maximum of 10 seconds.
    // The server will release the data handle.
    //
    if (!DdeClientTransaction ((LPBYTE) hData, 0xFFFFFFFF, hConv,
            (HSZ) NULL, 0, XTYP_EXECUTE, 10000, &lResult))
        goto CleanUp;

    //
    // Create the command string to add the item.
    //
    GetModuleFileName (NULL, szExePath, sizeof(szExePath) / sizeof(TCHAR));
    wsprintf (szCommand, TEXT("[AddItem(%s,%s)]"), szExePath, APP_NAME);

    //
    // Create a DDEML Data handle for the command string.
    //
    hData = DdeCreateDataHandle (lIdInst, (LPBYTE) szCommand,
                                 (lstrlen (szCommand) + 1) * sizeof(TCHAR),
#ifdef UNICODE
                                 0, (HSZ) NULL, CF_UNICODETEXT, 0L);
#else
                                 0, (HSZ) NULL, CF_TEXT, 0L);
#endif

    //
    // Send the command over to the program manager.
    //
    if (!DdeClientTransaction ((LPBYTE) hData, 0xFFFFFFFF, hConv,
            (HSZ) NULL, 0, XTYP_EXECUTE, 10000, &lResult))
        goto CleanUp;

    ret = TRUE;

CleanUp:
    if (!ret)
    {
        lResult = DdeGetLastError (lIdInst);
	LoadString(NULL, IDS_ADDERR, szExePath, 256);
        wsprintf (szErrorBuf, szExePath, APP_NAME);
        MessageBox (hwndKbd, szErrorBuf, szCaption, MB_OK | MB_ICONINFORMATION);
    }

    // Clean up
    if (hszProgMan)
        DdeFreeStringHandle (lIdInst, hszProgMan);
    if (hConv)
        DdeDisconnect (hConv);
    if (lIdInst)
        DdeUninitialize (lIdInst);

    return (ret);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GroupDDECallback(UINT, UINT, HANDLE, HSZ, HSZ, HDDEDATA, LONG, LONG)
//
//  PURPOSE: DDE message handler
//
//////////////////////////////////////////////////////////////////////////////

HDDEDATA CALLBACK DdeCallback( UINT     uiType,
                               UINT     uiFmt,
                               HCONV    hConv,
                               HSZ      hsz1,
                               HSZ      hsz2,
                               HDDEDATA hData,
                               DWORD    lData1,
                               DWORD    lData2 )
{
    switch (uiType)
    {
        default:
            break;
    }

    return ((HDDEDATA) NULL);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SetKbdHook(BOOL)
//
//  PURPOSE: Turns on or off the messaging hook
//
//////////////////////////////////////////////////////////////////////////////

BOOL SetKbdHook( BOOL fSet )
{
    static HHOOK  hhkKeyboard = NULL;
    static HANDLE hmodHook    = NULL;

    if (fSet)
    {
        if (!hmodHook)
        {
            if (!(hmodHook = LoadLibrary (TEXT("kbddll.dll"))))
            {
                LoadString (NULL, IDS_CANNOTFIND, szErrorBuf, MAX_ERRBUF);
                MessageBox (NULL, szErrorBuf, szCaption,
                            MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION);
                return (FALSE);
            }
        }

        if (!hhkKeyboard)
        {
            if (!(hhkKeyboard = SetWindowsHookEx (WH_KEYBOARD,
                    (HOOKPROC)GetProcAddress (hmodHook, "KbdGetMsgProc"),
                    hmodHook, 0)))
            {
                return (FALSE);
            }
        }
    }
    else
    {
        if (hhkKeyboard)
        {
            UnhookWindowsHookEx (hhkKeyboard);
            hhkKeyboard = NULL;
        }
    }

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RunCPIntlApp(VOID)
//
//  PURPOSE: Execute the International Applet of Control Panel
//
//////////////////////////////////////////////////////////////////////////////

BOOL RunCPIntlApp( VOID )
{
    BOOL                 b;
    STARTUPINFO          StartupInfo;
    PROCESS_INFORMATION  ProcessInformation;
    TCHAR                szExePath[256];

    memset (&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;
    LoadString(NULL, IDS_CPL_INTERNATIONAL, szExePath, 256);

    b = CreateProcess (NULL,
                       szExePath,   // CommandLine
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS,
                       NULL,
                       NULL,
                       &StartupInfo,
                       &ProcessInformation);
    if (b)
    {
        CloseHandle(ProcessInformation.hThread);
        CloseHandle(ProcessInformation.hProcess);
    }

    return (b);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages
//
//////////////////////////////////////////////////////////////////////////////

UINT APIENTRY MainWndProc( HWND   hwnd,
                           UINT   msg,
                           WPARAM wParam,
                           LPARAM lParam )
{
    HWND  hwndPrev;

    switch (msg)
    {
       case WM_SYSCOMMAND:
           switch (wParam)
           {
               case SC_MAXIMIZE:
               case SC_RESTORE:
Configure:
                   if (DialogBox (hInst, (LPTSTR)MAKEINTRESOURCE(KBDCONFDLG),
                                  hwnd, (DLGPROC)KbdConfDlgProc))
                   {
                       UpdateWinTitle ();
                       InitKbdHook (hwndKbd, fHotKeyCombo);
                       if (bAddStartupGrp)
                           AddAppToStartupGrp ();
		       ModifyMenu (GetSystemMenu(hwndKbd, FALSE),
				IDM_ONTOP, MF_STRING,
				IDM_ONTOP, fOnTop ? szOnBottom: szOnTop );
                   }
                   break;

               case IDM_PRIMARY:
                   LoadKeyboardLayout (szPrimaryKbd, KLF_ACTIVATE | KLF_REORDER);
                   UpdateWinTitle ();
                   break;

               case IDM_ALTERNATE:
                   LoadKeyboardLayout (szAlternateKbd, KLF_ACTIVATE | KLF_REORDER);
                   UpdateWinTitle ();
                   break;

               case IDM_OTHER:
                   RunCPIntlApp ();
                   break;

               case IDM_DISABLE:
                   if (bKbdDisabled)
                   {
                       if (!SetKbdHook (TRUE))
                       {
                            LoadString (NULL, IDS_CANNOTSETHOOK, szErrorBuf, MAX_ERRBUF);
                            MessageBox (hwnd, szErrorBuf, szCaption,
                                        MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION);
                            return (FALSE);
                       }
                       bKbdDisabled = FALSE;
                       ModifyMenu (GetSystemMenu (hwnd, FALSE), IDM_DISABLE, MF_STRING,
                                   IDM_DISABLE, szDisable);
                   }
                   else
                   {
                       SetKbdHook (FALSE);
                       // ignore return value - it'll almost certainly work
                       bKbdDisabled = TRUE;
                       ModifyMenu (GetSystemMenu (hwnd, FALSE), IDM_DISABLE, MF_STRING,
                                   IDM_DISABLE, szEnable);
                   }
                   break;

               case IDM_ONTOP:
		   if (fOnTop) {
			SetWindowPos (hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			fOnTop = 0;
		   }
		   else {
			SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			fOnTop = 1;
		   }
		   ModifyMenu (GetSystemMenu(hwnd, FALSE), IDM_ONTOP, MF_STRING,
			       IDM_ONTOP, fOnTop ? szOnBottom: szOnTop );

               default:
                   return (DefWindowProc (hwnd, msg, wParam, lParam));
           }
           break;

       case WM_NCLBUTTONDBLCLK:
       case WM_NCLBUTTONUP:
           //
           // double clicking on icon
           //
           goto Configure;
           break;

       case WM_WININICHANGE:
	   // This has worked once, so should work this time - no need
	   // to check return.
	   pLayouts = GetLayouts(hwnd);
           UpdateWinTitle ();
           break;

       case WM_CLOSE:
           SetKbdHook (FALSE);
           // ignore return value - it'll almost certainly work
           DestroyWindow (hwnd);
           break;

       case WM_DESTROY:
           PostQuitMessage (0);
           hwnd = hwndKbd = NULL;
           break;

       case WM_HOTKEY:
           if (wParam == HOT_KEY_ID)
           {
               TCHAR  szName[MAX_KEYNAME];

               GetKeyboardLayoutName (szName);
               if (!lstrcmp (szName, szPrimaryKbd))
                   LoadKeyboardLayout (szAlternateKbd, KLF_ACTIVATE);
               else if (!lstrcmp (szName, szAlternateKbd))
                   LoadKeyboardLayout (szPrimaryKbd, KLF_ACTIVATE);
               UpdateWinTitle ();
           }
           else
               return (DefWindowProc (hwnd, msg, wParam, lParam));
           break;

       case WM_USER:
           if (wParam == KEY_PRIMARY_KL)
               LoadKeyboardLayout (szPrimaryKbd, KLF_ACTIVATE);
           else if (wParam == KEY_ALTERNATE_KL)
               LoadKeyboardLayout (szAlternateKbd, KLF_ACTIVATE);
           UpdateWinTitle ();
           break;

       default:
           return (DefWindowProc (hwnd, msg, wParam, lParam));
    }

    //
    // Set focus to previous window
    //
    if (!bFirstTime && hwnd)
    {
	HWND hwndParent;

        SetWindowPos (hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        hwndPrev = GetNextWindow (hwnd, GW_HWNDNEXT);

	hwndParent = hwndPrev;
	while (GetParent(hwndParent) && GetWindowTextLength(hwndParent) == 0)
	    hwndParent = GetParent(hwndParent);

        SetForegroundWindow (hwndParent);
        SetWindowPos (hwnd, fOnTop?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0,
                      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }

    return (0);
}

