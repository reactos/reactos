/** FILE: cpl.c ************ Module Header ********************************
 *
 *  Initialization module and MAIN Control Panel applet procedure for all
 *  Control Panel applets contained in MAIN.CPL.  This file holds the routine
 *  to load a set global resources (strings, icons, etc.) from the applet's
 *  resources  table and initialize several global variables and path strings
 *  that are used by the applets and utility routines.
 *
 *  Additionally, this file also contains the "CPlApplet" procedure which
 *  complies with and services the ".CPL" protocol between the Control
 *  Panel main window procedure and its' applets.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  12:30 on Tues  04 Nov 1992  -by-  Steve Cathcart   [stevecat]
 *        Added PrintMan applet stub
 *  20 Oct 1993  -by-  Steve Cathcart   [stevecat]
 *        Fixed Mouse applet override for NT
 *  04 Apr 1994  -by-  Steve Cathcart   [stevecat]
 *        Added Type1 support
 *  13 Jun 1994  -by-  Steve Cathcart   [stevecat]
 *        Added Keyboard applet override for NT
 *
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *
 *  From Win3.1 file::
 *
**  CPL.C
**
**  DLL version of the 3.1 control panel
**
**  History:
**  Wed June 26 1991 by ChrisG
**    Mutated to work directly with the 3.1 control panel
**
**  Wed Apr 18 1990 -by- MichaelE
**      Created.
**
**  Wed Mar 20 1991 -by- BruceMo/MichaelE
**      Tweaked the hack for WinExec'ing CONTROL.EXE to specifically
**      disable any user input to it.  Also changed the way we wait
**      for CONTROL.EXE's dialog to go up and then back down.
**
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// NT System header files
//
//  NOTE:  These header files are only needed while NT APIs are called to
//         set and check SYSTEMTIME security privilege level.  When the
//         routines start to use Windows BASE APIs for setting and checking
//         this privilege, these headers can disappear.  [stevecat]
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"
#include <shellapi.h>

#include <commdlg.h>
#include <cpl.h>

//==========================================================================
//                            Local Definitions
//==========================================================================
#define CPL_SETUP   200     // Private message - should go in cpl.h later


#define APP_MOUSE 3
#define APP_KEYBD 5

typedef struct {
    int idIcon;
    int idName;
    int idInfo;
    int idChild;
    BOOL bEnabled;
    DWORD dwContext;
    LPTSTR pszHelp;
} APPLET_INFO;

//==========================================================================
//                            External Declarations
//==========================================================================
/*  data  */
extern HANDLE hAutoInstall;
extern BOOL fDoUpgrade;

extern HWND hwndTimeout;

/*  functions  */
extern BOOL APIENTRY ColorDlg     (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY DateTimeDlg  (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY DesktopDlg   (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY FontDlg      (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY IntlDlg      (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY KeyboardDlg  (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY MouseDlg     (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY SetupDlg     (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY ShortCommDlg (HWND, UINT, DWORD, LONG);
extern BOOL APIENTRY SystemDlg    (HWND, UINT, DWORD, LONG);

extern BOOL RegisterArrowClass(HANDLE);
extern VOID UnRegisterArrowClass(HANDLE);
extern BOOL RegisterProgressClass(HANDLE);
extern VOID UnRegisterProgressClass(HANDLE);


//==========================================================================
//                            Data Declarations
//==========================================================================
HANDLE  hModule = NULL;

TCHAR pszSysDir[PATHMAX];
TCHAR pszWinDir[PATHMAX];
TCHAR pszClose[40];
TCHAR pszContinue[40];

TCHAR szSetupDir[PATHMAX];
TCHAR szSetupInfPath[PATHMAX];
TCHAR szSharedDir[PATHMAX];

TCHAR szGenErr[133];

#ifdef JAPAN    /* V-KeijiY  July.8.1992 */
TCHAR szErrMem[200];
#else
TCHAR szErrMem[133];
#endif

TCHAR szCtlPanel[30];
TCHAR szDefNullPort[20];
TCHAR szOnString[10];

TCHAR szBasePath[PATHMAX];           /* Path to WIN.INI directory */
TCHAR szWinIni[PATHMAX];             /* Path to WIN.INI */
TCHAR szWinCom[PATHMAX];             /* Path to WIN.COM directory */
TCHAR szSystemIniPath[PATHMAX];      /* Path to SYSTEM.INI */
TCHAR szCtlIni[PATHMAX];             /* Path to CONTROL.INI */

// char szCONTROLINF[]  = "CONTROL.INF";
TCHAR szCONTROLINI[]  = TEXT("control.ini");

TCHAR szBoot[]  = TEXT("boot");

// From cpprn.c
short nDisk;
TCHAR szDrv[130];
TCHAR szDirOfSrc[PATHMAX];                /* Directory for File copy */

// REVIEW change to lower case.
TCHAR szSYSTEMINI[]    = TEXT("SYSTEM.INI");
TCHAR szSETUPINF[]     = TEXT("SETUP.INF");
TCHAR szSCRNSAVEEXE[]  = TEXT("SCRNSAVE.EXE");
BOOL bCursorLock = FALSE;

HWND hwndColor = NULL;        // global modal dialog
HWND hRainbowDlg = NULL;
HWND hSetup = NULL;        // means we are running under setup

TCHAR szWindows[]  = TEXT("windows");
TCHAR szColors[]   = TEXT("colors");
TCHAR szDevices[]  = TEXT("devices");
TCHAR szFonts[]    = TEXT("fonts");
TCHAR szPorts[]    = TEXT("ports");
TCHAR szIntl[]     = TEXT("intl");
TCHAR szDesktop[]  = TEXT("Desktop");
TCHAR szFOT[]      = TEXT(".FOT");        // truetype font stub
TCHAR szTrueType[] = TEXT("TrueType");

TCHAR szNull[]  = TEXT("");
TCHAR szComma[] = TEXT(",");
TCHAR szDot[] = TEXT(".");
TCHAR szSpace[] = TEXT(" ");

BOOL  bSetup = FALSE;           // Set TRUE when running under "Setup"


LPTSTR rgpszWinIni[CWININIENTRIES] =
    {
    szWindows, szColors, szDevices, szFonts, szPorts, szIntl, szDesktop,
    szTrueType
    };

//HDC hDCBits;

//HANDLE hUpArrow = NULL;
//HANDLE hDownArrow = NULL;

UINT    wHelpMessage;           // stuff for help
UINT    wBrowseMessage;         // stuff for help
UINT    wBrowseDoneMessage;     // stuff for browse
DWORD   dwContext = 0L;

TCHAR szControlHlp[PATHMAX];
TCHAR szCtlHlp[] = TEXT("control.hlp");
TCHAR szSpoolerHlp[] = TEXT("printman.hlp");

BYTE szCplApplet[] = "CPlApplet";
APPLET_PROC lpfnMouse = NULL;
APPLET_PROC lpfnKeybd = NULL;

// don't change the order here without changing the hacks that let
// the mouse driver override our mouse applet

#ifdef BEFORE_NEWSHELL

APPLET_INFO applets[] = {
    { COLORICON   , CHILDREN     , INFO     , CHILD_COLOR   , TRUE,
      IDH_CHILD_COLOR   , szControlHlp },
    { FONTICON    , CHILDREN + 2 , INFO + 2 , CHILD_FONT    , TRUE,
      IDH_CHILD_FONT    , szControlHlp },
    { PORTSICON   , CHILDREN + 4 , INFO + 4 , CHILD_PORTS   , TRUE,
      IDH_CHILD_PORTS   , szControlHlp },
    { MOUSEICON   , CHILDREN + 6 , INFO + 6 , CHILD_MOUSE   , TRUE,
      IDH_CHILD_MOUSE   , szControlHlp },
    { DESKTOPICON , CHILDREN + 8 , INFO + 8 , CHILD_DESKTOP , TRUE,
      IDH_CHILD_DESKTOP   , szControlHlp },
    { KEYBRDICON  , CHILDREN + 5 , INFO + 5 , CHILD_KEYBOARD, TRUE,
      IDH_CHILD_KEYBOARD, szControlHlp },
    { PRNICON  , CHILDREN + 1 , INFO + 1 ,    CHILD_PRINTER, TRUE,
      IDH_CHILD_PRINTER, szControlHlp },
    { INTLICON    , CHILDREN + 3 , INFO + 3 , CHILD_INTL    , TRUE,
      IDH_CHILD_INTL    , szControlHlp },
    { SYSTEMICON , CHILDREN + 11, INFO + 11, CHILD_SYSTEM   , TRUE,
      IDH_CHILD_SYSTEM , szControlHlp },
    { DATETIMEICON, CHILDREN + 7 , INFO + 7 , CHILD_DATETIME, TRUE,
      IDH_CHILD_DATETIME, szControlHlp },
};

#else

APPLET_INFO applets[] = {
    { FONTICON    , CHILDREN + 2 , INFO + 2 , CHILD_FONT    , TRUE,
      IDH_CHILD_FONT    , szControlHlp },
    { DATETIMEICON, CHILDREN + 7 , INFO + 7 , CHILD_DATETIME, TRUE,
      IDH_CHILD_DATETIME, szControlHlp },
};

#endif  //  BEFORE_NEWSHELL

#define NUM_APPLETS (sizeof(applets) / sizeof(*applets))


//==========================================================================
//                            Local Function Prototypes
//==========================================================================
APPLET_PROC GetDriverStuff(LPTSTR szDriverMod, LPNEWCPLINFO lpCpl);
void DoTheKeybdThing(HWND hWnd);
void DoTheMouseThing(HWND hWnd);
BOOL CheckTimePrivilege();
BOOL EnableTimePrivilege();
BOOL ResetTimePrivilege();


LONG APIENTRY CPlApplet (HWND, unsigned int, LONG, LONG);

void RunApplet(HWND hwnd, int cmd);


//==========================================================================
//                                Functions
//==========================================================================

//  Win32 NT Dll Initialization procedure

BOOL DllInitialize(
IN PVOID hmod,
IN ULONG ulReason,
IN PCONTEXT pctx OPTIONAL)
{
    if (ulReason != DLL_PROCESS_ATTACH)
    {
        return TRUE;
    }
    else
    {
        LPTSTR  Cl;
        LPTSTR  Name;
        SHORT   i;

        hModule = hmod;

        Cl = GetCommandLine ();

        Name = _tcsstr (Cl, TEXT("/INSTALL="));

        if (Name)
        {
            if (!GetTimeZoneRes (NULL))
            {
                //  ERROR condition - cannot get TZ info from registry
                TerminateProcess (GetCurrentProcess(), 1);
                return FALSE;
            }

            Name += 9;
            for (i = 0; i < NumTimeZones; i++)
            {
                if (_tcsstr (Tzi[i].szDisplayName, Name))
                {
                    SetTheTimezone ((HWND)0, 1, &Tzi[i]);
                    TerminateProcess (GetCurrentProcess(), 1);
                }
             }
            //
            // Name not found. If someone specified Pacific, then try US Pacific
            //

            if (_tcsstr (Name, TEXT("Pacif")))
            {
                Name = TEXT("Pacific Time");
                for (i = 0; i < NumTimeZones; i++)
                {
                    if (_tcsstr (Tzi[i].szDisplayName, Name))
                    {
                        SetTheTimezone ((HWND)0, 1, &Tzi[i]);
                        TerminateProcess (GetCurrentProcess(), 1);
                    }
                }
            }
            TerminateProcess (GetCurrentProcess(), 1);
        }
    }

    return TRUE;

    UNREFERENCED_PARAMETER(pctx);
}


/////////////////////////////////////////////////////////////////////////////
//
//  InitControlPanel
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL InitControlPanel(HWND hwndParent)
{
    DWORD    dwClass, dwShare;
    TCHAR    szClass[40];


    if (!RegisterArrowClass(hModule))
    {
        return FALSE;
    }

    if (!RegisterProgressClass(hModule))
    {
        return FALSE;
    }

    LoadString (hModule, INITS,    szErrMem, CharSizeOf(szErrMem));
    LoadString (hModule, INITS+1,  szCtlPanel, CharSizeOf(szCtlPanel));
    LoadString (hModule, INITS+7,  szDefNullPort, CharSizeOf(szDefNullPort));
    LoadString (hModule, INITS+8,  szOnString, CharSizeOf(szOnString));

    /* Get the "Close" and "Continue" strings
     */
    LoadString (hModule, INITS+9,  pszClose, CharSizeOf(pszClose));
    LoadString (hModule, INITS+10, pszContinue, CharSizeOf(pszContinue));

    /* Get the Windows and the System dirs, plus the directory for
     * installing shared files, and the full system.ini, control.ini,
     * and setup.inf paths
     */
    GetWindowsDirectory (szWinCom, PATHMAX);
    BackslashTerm(szWinCom);
    lstrcpy (pszWinDir, szWinCom);

    GetSystemDirectory (szBasePath, PATHMAX);
    BackslashTerm(szBasePath);
    lstrcpy (pszSysDir, szBasePath);

    //
    //  Create a fully qualified path to our Help file because of
    //  potential collisions with Win 3.1 help file in Windows dirs.
    //

    lstrcpy (szControlHlp, pszSysDir);
    lstrcat (szControlHlp, szCtlHlp);

    wsprintf (szSystemIniPath, TEXT("%s%s"), (LPTSTR)szWinCom, (LPTSTR)szSYSTEMINI);
    wsprintf (szCtlIni,        TEXT("%s%s"), (LPTSTR)szWinCom, (LPTSTR)szCONTROLINI);

    dwClass = CharSizeOf(szClass);
    dwShare = CharSizeOf(szSharedDir);

    VerFindFile (VFFF_ISSHAREDFILE, szNull, NULL, szNull, szClass, &dwClass,
                 szSharedDir, &dwShare);

    if (MyOpenFile (szSETUPINF, szSetupInfPath, OF_EXIST) == INVALID_HANDLE_VALUE)
        lstrcpy (szSetupInfPath, szSETUPINF);    // not found, use this anyway?

    wHelpMessage       = RegisterWindowMessage(TEXT("ShellHelp"));
    wBrowseMessage     = RegisterWindowMessage(HELPMSGSTRING);
    wBrowseDoneMessage = RegisterWindowMessage(FILEOKSTRING);

    return TRUE;
}


void TermControlPanel(HWND hWnd)
{
    if (lpfnMouse)
    {
        // call the mouse driver!
        (*lpfnMouse)(hWnd, CPL_EXIT, 0L, 0L);
    }

    if (lpfnKeybd)
    {
        // call the keybd driver!
        (*lpfnKeybd)(hWnd, CPL_EXIT, 0L, 0L);
    }

    UnRegisterArrowClass (hModule);
    UnRegisterProgressClass (hModule);
}


LONG APIENTRY CPlApplet(HWND hWnd, UINT Msg, LONG lParam1, LONG lParam2)
{
    int i, count;
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;
    static iInitCount = 0;

    switch (Msg)
    {
    case CPL_INIT:
        if (!iInitCount)
        {
            if (!InitControlPanel(hWnd))
                return FALSE;

#ifdef NO_DATETIME
            //  Find the DateTime applet and set bEnabled state based
            //  on SYSTEMTIME security privilege

            for (i = count = 0; i < NUM_APPLETS; i++)
            {
                if (applets[i].idChild == CHILD_DATETIME)
                {
                    applets[i].bEnabled = CheckTimePrivilege ();
                    break;
                }
            }
#endif  //  NO_DATETIME
//
//  NO_DATETIME
//
// [11/02/92]    Changed to always display the Date/Time applet icon.  If
// [stevecat]    the User does not have sufficient privilege, we now put
//               up a message box. See "RunApplet" routine.

        }

        iInitCount++;
        return TRUE;

    case CPL_GETCOUNT:
        for (i = count = 0; i < NUM_APPLETS; i++)
            if (applets[i].bEnabled)
                count++;

        return (LONG)count;

    case CPL_INQUIRE:

        lpOldCPlInfo = (LPCPLINFO)lParam2;

        // find the proper applet not counting those that are disabled
        for (i = count = 0; i < NUM_APPLETS; i++)
        {
            if (applets[i].bEnabled)
            {
                if (count == (int)lParam1)
                    break;
                count++;
            }
        }

        lpOldCPlInfo->idIcon = applets[i].idIcon;
        lpOldCPlInfo->idName = applets[i].idName;
        lpOldCPlInfo->idInfo = applets[i].idInfo;
        lpOldCPlInfo->lData = (LONG)applets[i].idChild;
        return TRUE;

    case CPL_NEWINQUIRE:

        lpCPlInfo = (LPNEWCPLINFO)lParam2;

        // find the proper applet not counting those that are disabled
        for (i = count = 0; i < NUM_APPLETS; i++)
        {
            if (applets[i].bEnabled)
            {
                if (count == (int)lParam1)
                    break;
                count++;
            }
        }

        lpCPlInfo->hIcon = LoadIcon(hModule, (LPTSTR) MAKEINTRESOURCE(applets[i].idIcon));
        LoadString(hModule, applets[i].idName, (LPTSTR) lpCPlInfo->szName, CharSizeOf(lpCPlInfo->szName));

        if (!LoadString(hModule, applets[i].idInfo, (LPTSTR) lpCPlInfo->szInfo, 64))
            lpCPlInfo->szInfo[0] = (TCHAR) 0;

        lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
        lpCPlInfo->lData  = (LONG)applets[i].idChild;
        lpCPlInfo->dwHelpContext = applets[i].dwContext;
        lstrcpy ((LPTSTR) lpCPlInfo->szHelpFile, applets[i].pszHelp);

        // let the mouse driver override any of this
        if (i == APP_MOUSE)
        {
            lpfnMouse = GetDriverStuff(TEXT("MOUSE"), lpCPlInfo);

            // Override this field in Info struct because it is what
            // I use below to determine which applet to run.
            lpCPlInfo->lData  = (LONG)applets[i].idChild;
        }

        // let the keybd driver override any of this
        if (i == APP_KEYBD)
        {
            lpfnKeybd = GetDriverStuff(TEXT("KEYBD"), lpCPlInfo);

            // Override this field in Info struct because it is what
            // I use below to determine which applet to run.
            lpCPlInfo->lData  = (LONG)applets[i].idChild;
        }

        return TRUE;

    case CPL_DBLCLK:
        dwContext = applets[(int)lParam1].dwContext;
        RunApplet(hWnd, (int)lParam2);
        break;

    case CPL_EXIT:
        iInitCount--;
        if (!iInitCount)
            TermControlPanel (hWnd);
        break;

    //  Private message sent when this applet is running under "Setup"
    case CPL_SETUP:
        bSetup= TRUE;
        break;

    default:
        break;
    }
    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
// MessageLoop
//
//  We pass in a pointer to a global variable, which gets set to NULL
//  when the dialog is destroyed.
//
//  The above statement is not exactly TRUE - instead we will save
//  a copy of hwndColor and call DestroyWindow ourselves later - see
//  RunApplet routine.
//
/////////////////////////////////////////////////////////////////////////////

void MessageLoop(HWND hwnd, HWND *phwndDlg)
{
    MSG msg;

    if (hwnd)
        EnableWindow(hwnd, FALSE);

    while (*phwndDlg && GetMessage(&msg, NULL, 0, 0))
    {
        if (!IsDialogMessage(*phwndDlg, &msg))
        {
            if (!hRainbowDlg || !IsDialogMessage(hRainbowDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    if (hwnd)
        EnableWindow(hwnd, TRUE);
}


void RunApplet(HWND hwnd, int cmd)
{
    HWND hwndSave;

    switch (cmd)
    {
    case CHILD_PORTS:
        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_PORTS), hwnd, (DLGPROC)ShortCommDlg);
        break;

    case CHILD_COLOR:
//        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_COLOR), hwnd, (DLGPROC)ColorDlg);

        /* This dialog MUST be modeless because it brings up a
        * modeless dialog (the Rainbow dialog) and we need the
        * main message loop to process its messages
        */

        //  Since we create the Color dialog ourselves - we need to call
        //  DestroyWindow() on it when we are done to make sure that no
        //  more messages are passed to it.

        if (hwndColor = CreateDialog(hModule, MAKEINTRESOURCE(DLG_COLOR),
                                     hwnd, (DLGPROC)ColorDlg))
        {
            //  hwndColor will be set to NULL to end the MessageLoop
            //  routine - so we need to keep a copy of it around to
            //  use in the DestroyWindow call.
            hwndSave = hwndColor;

            MessageLoop(hwnd, &hwndColor);

            DestroyWindow (hwndSave);
        }

        break;

    case CHILD_FONT:
        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_FONT), hwnd, (DLGPROC)FontDlg);
        break;

    case CHILD_INTL:
        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_INTL), hwnd, (DLGPROC)IntlDlg);
        break;

    case CHILD_DATETIME:
    {
        PTOKEN_PRIVILEGES PreviousState;
        ULONG             PreviousStateLength;

        /* DATETIME applet has been double-clicked.
           wParam is an index from 0 to (NUM_APPLETS-1)
           lParam is the lData value associated with the applet
         */
        if (EnableTimePrivilege (&PreviousState, &PreviousStateLength))
        {
            DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_DATETIME),hwnd, (DLGPROC)DateTimeDlg);
            ResetTimePrivilege (PreviousState, PreviousStateLength);
        }
        else
        {
            MyMessageBox (hwnd, INITS+11, INITS+1, MB_OK|MB_ICONINFORMATION);
        }
        break;
    }

    case CHILD_KEYBOARD:
//        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_KEYBOARD), hwnd, (DLGPROC)KeyboardDlg);
        DoTheKeybdThing(hwnd);
        break;

    case CHILD_PRINTER:
        ShellExecute (NULL, NULL, TEXT("printman.exe"), NULL, NULL, SW_NORMAL);

        // MessageBox (hwnd, "PrintMan Applet stub not implemented yet.",
        //            szCtlPanel, MB_OK | MB_ICONINFORMATION);
        //
        //  Just do a Shell Exec of PrintMan and return right away.
        break;

    case CHILD_DESKTOP:
        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_DESKTOP), hwnd, (DLGPROC)DesktopDlg);
        break;

    case CHILD_SYSTEM:
        DialogBox(hModule, (LPTSTR) MAKEINTRESOURCE(DLG_SYSTEM), hwnd, (DLGPROC)SystemDlg);
        break;

    case CHILD_MOUSE:
        DoTheMouseThing(hwnd);
        break;
    }

}

void ConvertCplInfoA (LPNEWCPLINFO lpCPlInfoW)
{
   NEWCPLINFOA   CplInfoA;

   memcpy ((LPBYTE) &CplInfoA, lpCPlInfoW, sizeof(NEWCPLINFOA));
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szName, 32,
                        (LPWSTR) lpCPlInfoW->szName, 32);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szInfo, 64,
                        (LPWSTR) lpCPlInfoW->szInfo, 64);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szHelpFile, 128,
                        (LPWSTR) lpCPlInfoW->szHelpFile, 128);

   lpCPlInfoW->dwSize = sizeof(NEWCPLINFO);
}

APPLET_PROC GetDriverStuff(LPTSTR szDriverMod, LPNEWCPLINFO lpCpl)
{
    CPLINFO ci;
    HANDLE hMod;
    APPLET_PROC lpfnDriver;
    HICON hIcon;

//  Original code - this will only get the "HANDLE" if the "Mouse.dll or
//  Mouse.exe" module was loaded in the system - like at startup to handle
//  the mouse.  This will not work on NT - so we will do a LoadLibrary on
//  the DLL (or EXE), call the CPlApplet entry point and let them send a
//  Init success or failure back to us.
//    hMod = GetModuleHandle (szDriverMod);

    hMod = LoadLibrary (szDriverMod);

    if (hMod != NULL)
    {
        lpfnDriver = (APPLET_PROC) GetProcAddress (hMod, szCplApplet);

        if (lpfnDriver)
        {

            if (!(*lpfnDriver)(NULL, CPL_INIT, 0L, 0L))
                return NULL;

            // try the new method first

            lpCpl->dwSize = 0;
            lpCpl->dwFlags = 0;

            (*lpfnDriver)(NULL, CPL_NEWINQUIRE, 0L, (LONG)lpCpl);

            if (lpCpl->dwSize == sizeof(NEWCPLINFOA))
            {
                ConvertCplInfoA (lpCpl);
            }
            else if ((lpCpl->dwSize != sizeof(NEWCPLINFO)))
            {
                lpCpl->dwSize = sizeof(NEWCPLINFO);

                /* Initialize to an invalid value; JonT tells me 0 is an
                 * invalid ID for all resources
                 */
                ci.idIcon = 0;
                (*lpfnDriver)(NULL, CPL_INQUIRE, 0L, (LONG)(LPTSTR)&ci);

                if (ci.idIcon &&
                    (hIcon=LoadIcon(hMod, (LPTSTR) MAKEINTRESOURCE(ci.idIcon))))
                {
                      lpCpl->hIcon = hIcon;
                      LoadString (hMod, ci.idName, (LPTSTR) lpCpl->szName, 32);
                }
            }
            return lpfnDriver;
        }
    }

    return NULL;
}

void DoTheMouseThing(HWND hWnd)
{
    NEWCPLINFO CPlInfo;

    CPlInfo.lData = 0L;

    if (!lpfnMouse)
        lpfnMouse = GetDriverStuff(TEXT("MOUSE"), &CPlInfo);

    if (lpfnMouse)
    {
        // call the mouse driver!

        if (!(*lpfnMouse)(hWnd, CPL_DBLCLK, 0L, CPlInfo.lData))
            goto NormalCase;
    }
    else if (!GetSystemMetrics(SM_MOUSEPRESENT))
    {
        MyMessageBox(hWnd, MOUSE+0, INITS+1, MB_OK|MB_ICONINFORMATION);
    }
    else
    {
NormalCase:
        DoDialogBoxParam(DLG_MOUSE, hWnd, (DLGPROC) MouseDlg,
                                                    IDH_CHILD_MOUSE, 0L);
    }
}

void DoTheKeybdThing(HWND hWnd)
{
    NEWCPLINFO CPlInfo;

    CPlInfo.lData = 0L;

    if (!lpfnKeybd)
        lpfnKeybd = GetDriverStuff(TEXT("KEYBD"), &CPlInfo);

    if (lpfnKeybd)
    {
        // call the keybd driver!

        if (!(*lpfnKeybd)(hWnd, CPL_DBLCLK, 0L, CPlInfo.lData))
            goto NormalCase;
    }
    else
    {
NormalCase:
        DoDialogBoxParam(DLG_KEYBOARD, hWnd, (DLGPROC) KeyboardDlg,
                                                    IDH_CHILD_KEYBOARD, 0L);
    }
}

void CPHelp (HWND hWnd)
{
//    char    szBuf[80];

//    wsprintf (szBuf, "Help Context %ld", dwContext);

    WinHelp (hWnd, szControlHlp, HELP_CONTEXT, dwContext);
}


////////////////////////////////////////////////////////////////////////////
//
// APIs for by Date/Time applet to check/enable/reset SYSTEMTIME privilege
//
//
// NOTE: These will eventually be changed to use Windows NT api calls and
//       the routines should be cleaned up then.
//
////////////////////////////////////////////////////////////////////////////

#ifdef NO_DATETIME
BOOL CheckTimePrivilege()
{
    NTSTATUS          NtStatus;
    HANDLE            Token;
    LUID              SystemTimePrivilege;
    PTOKEN_PRIVILEGES NewState;
    PTOKEN_PRIVILEGES PrevState;
    ULONG             ReturnLength;

    // Open our own token

    NtStatus = NtOpenProcessToken ( NtCurrentProcess(),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    &Token
                                    );
    if (!NT_SUCCESS(NtStatus))
        return FALSE;

    // Initialize the adjustment structure

    SystemTimePrivilege = RtlConvertLongToLargeInteger(SE_SYSTEMTIME_PRIVILEGE);

//    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = (PTOKEN_PRIVILEGES) LocalAlloc (LMEM_FIXED, 100);

    if (NewState == NULL)
        return FALSE;

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SystemTimePrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    PrevState = (PTOKEN_PRIVILEGES) LocalAlloc (LMEM_FIXED, 100);

    if (PrevState == NULL)
    {
        LocalFree (NewState);
        return FALSE;
    }

    // Set the state of the privilege to ENABLED.

    NtStatus = NtAdjustPrivilegesToken(
                                Token,             // TokenHandle
                                FALSE,             // DisableAllPrivileges
                                NewState,          // NewState
                                100,               // BufferLength
                                PrevState,         // PreviousState (OPTIONAL)
                                &ReturnLength      // ReturnLength
                                );

    LocalFree (NewState);

    if (NtStatus == STATUS_SUCCESS)
    {
        // Restore previous state of the privilege

        NtStatus = NtAdjustPrivilegesToken(
                                Token,             // TokenHandle
                                FALSE,             // DisableAllPrivileges
                                PrevState,         // NewState
                                ReturnLength,      // BufferLength
                                NULL,              // PreviousState (OPTIONAL)
                                &ReturnLength      // ReturnLength
                                );
//        ASSERT(NT_SUCCESS(NtStatus));

        // Clean up some stuff before returning

        LocalFree (PrevState);
        NtClose (Token);
        return TRUE;
    }
    else
    {
        // Clean up some stuff before returning

        LocalFree (PrevState);
        NtClose (Token);
        return FALSE;
    }
}
#endif  //  NO_DATETIME

BOOL EnableTimePrivilege (
PTOKEN_PRIVILEGES *pPreviousState,
ULONG             *pPreviousStateLength
)
{
    NTSTATUS          NtStatus;
    HANDLE            Token;
    LUID              SystemTimePrivilege;
    PTOKEN_PRIVILEGES NewState;

    // Open our own token

    NtStatus = NtOpenProcessToken ( NtCurrentProcess(),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    &Token
                                    );
    if (!NT_SUCCESS(NtStatus))
        return FALSE;

    // Initialize the adjustment structure

    SystemTimePrivilege = RtlConvertLongToLargeInteger(SE_SYSTEMTIME_PRIVILEGE);

//    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = (PTOKEN_PRIVILEGES) LocalAlloc (LMEM_FIXED, 100);

    if (NewState == NULL)
        return FALSE;

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SystemTimePrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    *pPreviousState = (PTOKEN_PRIVILEGES) LocalAlloc (LMEM_FIXED, 100);

    if (*pPreviousState == NULL)
    {
        LocalFree (NewState);
        return FALSE;
    }

    // Set the state of the privilege to ENABLED.

    NtStatus = NtAdjustPrivilegesToken(
                                Token,               // TokenHandle
                                FALSE,               // DisableAllPrivileges
                                NewState,            // NewState
                                100,                 // BufferLength
                                *pPreviousState,     // PreviousState (OPTIONAL)
                                pPreviousStateLength // ReturnLength
                                );
    LocalFree (NewState);

    if (NtStatus == STATUS_SUCCESS)
    {
        // Clean up some stuff before returning

        NtClose (Token);
        return TRUE;
    }
    else
    {
        // Clean up some stuff before returning

        LocalFree (*pPreviousState);
        NtClose (Token);
        return FALSE;
    }
}

//
// Restore Previous Privilege state for Setting System Time
//

BOOL ResetTimePrivilege (
PTOKEN_PRIVILEGES PreviousState,
ULONG             PreviousStateLength
)
{
    NTSTATUS NtStatus;
    HANDLE   Token;
    LUID     SystemTimePrivilege;
    ULONG    ReturnLength;

    if (PreviousState == NULL)
        return FALSE;

    // Open our own token

    NtStatus = NtOpenProcessToken (NtCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &Token
                                   );
    if (!NT_SUCCESS(NtStatus))
        return FALSE;

    // Initialize the adjustment structure

    SystemTimePrivilege = RtlConvertLongToLargeInteger(SE_SYSTEMTIME_PRIVILEGE);

//    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);

    // Restore previous state of the privilege

    NtStatus = NtAdjustPrivilegesToken(
                            Token,                  // TokenHandle
                            FALSE,                  // DisableAllPrivileges
                            PreviousState,          // NewState
                            PreviousStateLength,    // BufferLength
                            NULL,                   // PreviousState (OPTIONAL)
                            &ReturnLength           // ReturnLength
                            );

//  ASSERT(NT_SUCCESS(NtStatus));

    // Clean up some stuff before returning

    LocalFree (PreviousState);
    NtClose (Token);

    return (NT_SUCCESS(NtStatus));
}

