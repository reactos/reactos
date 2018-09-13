/*  SSDLG.C
**
**  Copyright (C) Microsoft, 1999, All Rights Reserved.
**
**  History:
**      18 Feb 94   (Tracy Sharpe) Added power management functionality.
**                  Commented out several pieces of code that weren't being
**                  used.
*/

#include "precomp.h"
#pragma hdrstop

#ifndef IDH_DESK_LOWPOWERCFG
#pragma message("using local version of IDH_DESK_LOWPOWERCFG!!")
#define IDH_DESK_LOWPOWERCFG 4828
#endif

#define         SFSE_SYSTEM     0
#define         SFSE_PRG        1
#define         SFSE_WINDOWS    2
#define         SFSE_FILE       3

/* Local function prototypes... */
void  NEAR PASCAL SearchForScrEntries     ( UINT, LPCTSTR );
BOOL  NEAR PASCAL FreeScrEntries          ( void );
int   NEAR PASCAL lstrncmp                ( LPTSTR, LPTSTR, int );
LPTSTR NEAR PASCAL FileName                ( LPTSTR szPath);
LPTSTR NEAR PASCAL StripPathName           ( LPTSTR szPath);
LPTSTR NEAR PASCAL NiceName                ( LPTSTR szPath);

void  NEAR PASCAL AddBackslash(LPTSTR pszPath);
void  NEAR PASCAL AppendPath(LPTSTR pszPath, LPTSTR pszSpec);

PTSTR  NEAR PASCAL PerformCheck(LPTSTR, BOOL);
PTSTR  NEAR PASCAL AllocStr(LPTSTR szCopy);
void  NEAR PASCAL SaveIni(HWND hDlg);
void  NEAR PASCAL DoScreenSaver(HWND hDlg, BOOL b);

void NEAR PASCAL ScreenSaver_AdjustTimeouts(HWND hWnd,int BaseControlID);
void NEAR PASCAL ScreenSaver_EnableDisableDelays(HWND hWnd,int BaseControlID);
void NEAR PASCAL EnableDisablePowerDelays(HWND hDlg);
void NEAR PASCAL EnableDisableSetPasswordBtn(HWND hDlg);

TCHAR   szMethodName[]       = TEXT("SCRNSAVE.EXE");     // Method entry
TCHAR   szBuffer[BUFFER_SIZE];                     // Shared buffer
TCHAR   szSaverName[MAX_PATH];                    // Screen Saver EXE
HICON  hDefaultIcon;
HICON  hIdleWildIcon;
BOOL    bWasConfig=0;   // We were configing the screen saver
HWND    g_hwndTestButton;
HWND    g_hwndLastFocus;
BOOL    g_fPasswordWasPreviouslyEnabled = FALSE;
// Local global variables

HICON  hIcons[MAX_METHODS];
UINT   wNumMethods;
PTSTR   aszMethods[MAX_METHODS];
PTSTR   aszFiles[MAX_METHODS];

static const TCHAR c_szDemoParentClass[] = TEXT("SSDemoParent");

//  static TCHAR szFileNameCopy[MAX_PATH];
static int  g_iMethod;
static BOOL g_fPreviewActive;
static BOOL g_fAdapPwrMgnt = FALSE;

/*
 * Registry value for the "Password Protected" check box
 *
 * These are different for NT and Win95 to keep screen
 * savers built exclusivly for Win95 from trying to
 * handle password checks.  (NT does all password checking
 * in the built in security system to maintain C2
 * level security)
 */

#ifdef WINNT
#   define SZ_USE_PASSWORD     TEXT("ScreenSaverIsSecure")
#   define PWRD_REG_TYPE       REG_SZ
#   define CCH_USE_PWRD_VALUE  2
#   define CB_USE_PWRD_VALUE   (CCH_USE_PWRD_VALUE * SIZEOF(TCHAR))

TCHAR gpwdRegYes[CCH_USE_PWRD_VALUE] = TEXT("1");
TCHAR gpwdRegNo[CCH_USE_PWRD_VALUE]  = TEXT("0");

#define PasswdRegData(f)    ((f) ? (PBYTE)gpwdRegYes : (PBYTE)gpwdRegNo)

#else
#   define SZ_USE_PASSWORD     REGSTR_VALUE_USESCRPASSWORD
#   define PWRD_REG_TYPE       REG_DWORD
#   define CB_USE_PWRD_VALUE   SIZEOF(DWORD)

DWORD gpwdRegYes = 1;
DWORD gpwdRegNo  = 0;

#define PasswdRegData(f)    ((f) ? (PBYTE)&gpwdRegYes : (PBYTE)&gpwdRegNo)

#endif

UDACCEL udAccel[] = {{0,1},{2,5},{4,30},{8,60}};

#include "help.h"

const DWORD FAR aSaverHelpIds[] = {
        IDC_NO_HELP_1,          IDH_COMM_GROUPBOX,
        IDC_CHOICES,            IDH_DSKTPSCRSAVER_LISTBX,
        IDC_SSDELAYLABEL,       IDH_DSKTPSCRSAVER_WAIT,
        IDC_SSDELAYSCALE,       IDH_DSKTPSCRSAVER_WAIT,
        IDC_SCREENSAVEDELAY,    IDH_DSKTPSCRSAVER_WAIT,
        IDC_SCREENSAVEARROW,    IDH_DSKTPSCRSAVER_WAIT,
        IDC_TEST,               IDH_DSKTPSCRSAVER_TEST,
        IDC_SETTING,            IDH_DSKTPSCRSAVER_SETTINGS,
        IDC_BIGICON,            IDH_DSKTPSCRSAVER_MONITOR,
        IDC_ENERGY_TEXT,        IDH_COMM_GROUPBOX,
        IDC_ENERGY_TEXT2,       IDH_SCRSAVER_LOWPOWSTANDBY,
        IDC_ENERGY_TEXT3,       IDH_SCRSAVER_SHUTOFFPOW,
        IDC_LOWPOWERSWITCH,     IDH_SCRSAVER_LOWPOWSTANDBY,
        IDC_LOWPOWERDELAY,      IDH_SCRSAVER_LOWPOWSTANDBY,
        IDC_LOWPOWERARROW,      IDH_SCRSAVER_LOWPOWSTANDBY,
        IDC_POWEROFFSWITCH,     IDH_SCRSAVER_SHUTOFFPOW,
        IDC_POWEROFFDELAY,      IDH_SCRSAVER_SHUTOFFPOW,
        IDC_POWEROFFARROW,      IDH_SCRSAVER_SHUTOFFPOW,
        IDC_ENERGYSTAR_BMP,     IDH_SCRSAVER_GRAPHIC,
        IDC_USEPASSWORD,        IDH_COMM_PASSWDCHKBOX,
        IDC_SETPASSWORD,        IDH_COMM_PASSWDBUTT,
        IDC_LOWPOWERCONFIG,     IDH_DESK_LOWPOWERCFG,

        0, 0
};

//
//  To simplify some things, the base control ID of a time control is associated
//  with its corresponding SystemParametersInfo action codes.
//

typedef struct {
    int taBaseControlID;
    UINT taGetTimeoutAction;
    UINT taSetTimeoutAction;
    UINT taGetActiveAction;
    UINT taSetActiveAction;
}   TIMEOUT_ASSOCIATION;

//
//  Except for the case of the "screen save" delay, each time grouping has three
//  controls-- a switch to determine whether that time should be used or not and
//  an edit box and an updown control to change the delay time.  ("Screen save"
//  is turned off my choosing (None) from the screen saver list)  These three
//  controls must be organized as follows:
//

#define BCI_DELAY               0
#define BCI_ARROW               1
#define BCI_SWITCH              2

//
//  Associations between base control IDs and SystemParametersInfo action codes.
//  The TA_* #defines are used as symbolic indexes into this array.  Note that
//  TA_SCREENSAVE is a special case-- it does NOT have a BCI_SWITCH.
//

#define TA_SCREENSAVE           0
#define TA_LOWPOWER             1
#define TA_POWEROFF             2

TIMEOUT_ASSOCIATION g_TimeoutAssociation[] = {
    IDC_SCREENSAVEDELAY, SPI_GETSCREENSAVETIMEOUT, SPI_SETSCREENSAVETIMEOUT,
        SPI_GETSCREENSAVEACTIVE, SPI_SETSCREENSAVEACTIVE,
};

int g_Timeout[] = {
    0,
    0,
    0,
};

HBITMAP g_hbmDemo = NULL;
HBITMAP g_hbmEnergyStar = NULL;
BOOL g_bInitSS = TRUE;          // assume we are in initialization process
BOOL g_bChangedSS = FALSE;      // changes have been made


/*
 * Win95 and NT store different values in different places of the registry to
 * determine if the screen saver is secure or not.
 *
 * We can't really consolidate the two because the screen savers do different
 * actions based on which key is set.  Win95 screen savers do their own
 * password checking, but NT must let the secure desktop winlogon code do it.
 *
 * Therefore to keep Win95 screen savers from requesting the password twice on
 * NT, we use  REGSTR_VALUE_USESCRPASSWORD == (REG_DWORD)1 on Win95 to indicate
 * that a screen saver should check for the password, and
 * "ScreenSaverIsSecure" == (REG_SZ)"1" on NT to indicate that WinLogon should
 * check for a password.
 *
 * This function will deal with the differences.
 */
static BOOL IsPasswdSecure(HKEY hKey, LPTSTR szUsePW ) {
    union {
        DWORD dw;
        TCHAR asz[4];
    } uData;

    DWORD dwSize, dwType;
    BOOL fSecure = FALSE;

    dwSize = SIZEOF(uData);

    RegQueryValueEx(hKey,SZ_USE_PASSWORD,NULL, &dwType, (BYTE *)&uData, &dwSize);

    switch( dwType ) {
    case REG_DWORD:
        fSecure = (uData.dw == 1);
        break;

    case REG_SZ:
        fSecure = (uData.asz[0] == TEXT('1'));
        break;
    }

    return fSecure;
}


static void NEAR
EnableDlgChild( HWND dlg, HWND kid, BOOL val )
{
    if( !val && ( kid == GetFocus() ) )
    {
        // give prev tabstop focus
        SendMessage( dlg, WM_NEXTDLGCTL, 1, 0L );
    }

    EnableWindow( kid, val );
}

static void NEAR
EnableDlgItem( HWND dlg, int idkid, BOOL val )
{
    EnableDlgChild( dlg, GetDlgItem( dlg, idkid ), val );
}

HWND NEAR PASCAL GetSSDemoParent( HWND page )
{
    static HWND parent = NULL;

    if( !parent || !IsWindow( parent ) )
    {
        parent = CreateWindowEx( 0, c_szDemoParentClass,
            g_szNULL, WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0,
            GetDlgItem( page, IDC_BIGICON ), NULL, hInstance, NULL );
    }

    return parent;
}

void NEAR PASCAL ForwardSSDemoMsg(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndChild;

    hwnd = GetSSDemoParent(hwnd);

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL;
        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        SendMessage(hwndChild, uMessage, wParam, lParam);
    }
}

void NEAR PASCAL ParseSaverName( LPTSTR lpszName )
{
    if( *lpszName == TEXT('\"') )
    {
        LPTSTR lpcSrc = lpszName + 1;

        while( *lpcSrc && *lpcSrc != TEXT('\"') )
        {
            *lpszName++ = *lpcSrc++;
        }

        *lpszName = 0;  // clear second quote
    }
}

// YUCK:
// since our screen saver preview is in a different process,
//   it is possible that we paint in the wrong order.
// this ugly hack makes sure the demo always paints AFTER the dialog

WNDPROC g_lpOldStaticProc = NULL;

LRESULT  StaticSubclassProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT result =
        CallWindowProc(g_lpOldStaticProc, wnd, msg, wp, lp);

    if (msg == WM_PAINT)
    {
        HWND demos = GetSSDemoParent(GetParent(wnd));

        if (demos)
        {
            RedrawWindow(demos, NULL, NULL,
                RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }
    }

    return result;
}

BOOL NEAR PASCAL InitSSDialog(HWND hDlg)
{
    WNDCLASS wc;
    PTSTR  pszMethod;
    UINT  wTemp,wLoop;
    BOOL  fContinue;
    int   Timeout;
    int   Active;
    UINT  Counter;
    int   ControlID;
    int   wMethod;
    int   ScreenSaveActive;
    DWORD dwData=0,dwSize = sizeof(dwData);
    HKEY  hKey;
    HWND  hwnd;
    //int   dummy;

    if( !GetClassInfo( hInstance, c_szDemoParentClass, &wc ) )
    {
        // if two pages put one up, share one dc
        wc.style = 0;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = (HICON)( wc.hCursor = NULL );
        wc.hbrBackground = GetStockObject( BLACK_BRUSH );
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szDemoParentClass;

        if( !RegisterClass( &wc ) )
            return FALSE;
    }

    // Fetch the timeout value from the win.ini and adjust between 1:00-60:00
    for (Counter = 0; Counter < (sizeof(g_TimeoutAssociation) /
             sizeof(TIMEOUT_ASSOCIATION)); Counter++) {

        // Fetch the timeout value from the win.ini and adjust between 1:00-60:00

        SystemParametersInfo(g_TimeoutAssociation[Counter].taGetTimeoutAction, 0,
            &Timeout, 0);

        /*  The Win 3.1 guys decided that 0 is a valid ScreenSaveTimeOut value.
         *  This causes our screen savers not to kick in (who cares?).  In any
         *  case, I changed this to allow 0 to go through.  In this way, the
         *  user immediately sees that the value entered is not valid to fire
         *  off the screen saver--the OK button is disabled.  I don't know if
         *  I fully agree with this solution--it is just the minimal amount of
         *  code.  The optimal solution would be to ask the 3.1 guys why 0 is
         *  valid?  -cjp
         */
        Timeout = min(max(Timeout, 1), MAX_MINUTES * 60);

        //
        //  Convert Timeout to minutes, rounding up.
        //

        Timeout = (Timeout + 59) / 60;
        g_Timeout[Counter] = Timeout;

        //
        //  The base control id specifies the edit control id.
        //

        ControlID = g_TimeoutAssociation[Counter].taBaseControlID;

        /* Set the maximum length of all of the fields... */
        SendDlgItemMessage(hDlg, ControlID, EM_LIMITTEXT, 4, 0); //Allow 4 digits.

        SystemParametersInfo(g_TimeoutAssociation[Counter].taGetActiveAction,
            0, &Active, SPIF_UPDATEINIFILE);

        if (Counter != TA_SCREENSAVE) {

            CheckDlgButton(hDlg, ControlID + BCI_SWITCH, Active);
        }
        else {
            ScreenSaveActive = Active;
        }

        SetDlgItemInt(hDlg, ControlID, Timeout, FALSE);

        //
        //  The associated up/down control id must be one after the edit control
        //  id.
        //

        ControlID++;

        SendDlgItemMessage(hDlg, ControlID, UDM_SETRANGE, 0,
            MAKELPARAM(MAX_MINUTES, MIN_MINUTES));
        SendDlgItemMessage(hDlg, ControlID, UDM_SETACCEL, 4,
            (LPARAM)(LPUDACCEL)udAccel);

    }

    // Find the name of the exe used as a screen saver. "" means that the
    // default screen saver will be used

    GetPrivateProfileString(g_szBoot, szMethodName, g_szNULL, szSaverName,
        ARRAYSIZE(szSaverName), g_szSystemIni);

    ParseSaverName( szSaverName );  // remove quotes and params

    /* Copy all of the variables into their copies... */
    //  lstrcpy(szFileNameCopy, szSaverName);

    /* Load in the default icon... */
    hDefaultIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDS_ICON));

    /* Find the methods to save the screen.  If the method that was
        selected is not found, the program will assume that the
        first method in the list will be the one that is elected... */
    wNumMethods = 0;
    wMethod = -1;

    SearchForScrEntries(SFSE_PRG,NULL);
    SearchForScrEntries(SFSE_SYSTEM,NULL);
    SearchForScrEntries(SFSE_WINDOWS,NULL);
    SearchForScrEntries(SFSE_FILE,szSaverName);

    /* Set up the combo box for the different fields... */
    SendDlgItemMessage(hDlg, IDC_CHOICES, CB_ADDSTRING, 0, (LONG)g_szNone);
    for (wTemp = 0; wTemp < wNumMethods; wTemp++)
    {
        /* Lock down the information and pass it to the combo box... */
        pszMethod = aszMethods[wTemp];
        wLoop = SendDlgItemMessage(hDlg,IDC_CHOICES,CB_ADDSTRING,0,
            (LONG)(pszMethod+1));
        SendDlgItemMessage(hDlg,IDC_CHOICES,CB_SETITEMDATA,wLoop,
            (DWORD)wTemp);

        /* If we have the correct item, keep a copy so we can select it
            out of the combo box... */
        // check for filename only as well as full path name

        if( !lstrcmpi( FileName( aszFiles[ wTemp ] ),
                       FileName( szSaverName       ) ) )
        {
            wMethod = wTemp;
            lstrcpy(szBuffer, pszMethod + 1);
        }
    }

    if (!ScreenSaveActive)
        wMethod = -1;

    /* Attempt to select the string we recieved from the
        system.ini entry.  If there is no match, select the
        first item from the list... */
    if ((wMethod == -1) || (wNumMethods == 0))
    {
        fContinue = TRUE;
        //
        // fix pszMethod not being init'd when no .SCR
        // files are found anywhere (this is a hack because
        // I don't want to figure this garbage out)
        //  THIS ISN'T NEEDED!  NO MORE FORWARD REFERENCES TO IT!
        //  pszMethod = g_szNULL;
    }
    else
    {
        if (SendDlgItemMessage(hDlg, IDC_CHOICES, CB_SELECTSTRING, (WPARAM)-1,
            (LONG)szBuffer) == CB_ERR)
            fContinue = TRUE;
        else
            fContinue = FALSE;
    }
    if(fContinue)
    {
       SendDlgItemMessage(hDlg,IDC_CHOICES,CB_SETCURSEL,0,0l);
       lstrcpy(szSaverName,g_szNULL);
       wMethod = -1;
    }

    g_hbmDemo = LoadMonitorBitmap( TRUE );

    if (g_hbmDemo)
        SendDlgItemMessage(hDlg,IDC_BIGICON,STM_SETIMAGE, IMAGE_BITMAP,(DWORD)g_hbmDemo);

    // Call will fail if monitor or adapter don't support DPMS.
    // Let the power manage cpl handle this. 
    g_fAdapPwrMgnt = TRUE; //SystemParametersInfo(SPI_GETLOWPOWERACTIVE, 0, &dummy, 0);

    g_hbmEnergyStar = LoadImage( hInstance, MAKEINTRESOURCE( IDB_ENERGYSTAR ),
        IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );

    if( g_hbmEnergyStar )
    {
        SendDlgItemMessage( hDlg, IDC_ENERGYSTAR_BMP, STM_SETIMAGE,
            IMAGE_BITMAP, (LPARAM)g_hbmEnergyStar );
    }

    // Hide/Disable the energy related controls if the adaptor/monitor does not
    // support power mgnt.
    EnableDisablePowerDelays(hDlg);

    // initialize the password checkbox
    if (RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_SCREENSAVE,&hKey) == ERROR_SUCCESS) {
        if (IsPasswdSecure(hKey, SZ_USE_PASSWORD ))
            g_fPasswordWasPreviouslyEnabled = TRUE;
        RegCloseKey(hKey);
    }

    // subclass the static control so we can synchronize painting
    hwnd = GetDlgItem(hDlg, IDC_BIGICON);
    if (hwnd)
    {
        g_lpOldStaticProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
        SetWindowLong(hwnd, GWL_WNDPROC, (LONG)(WNDPROC)StaticSubclassProc);
        // Turn off the mirroring style for this control to allow the screen saver preview to work.
        SetWindowLong (hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~RTL_MIRRORED_WINDOW);
    }

    return TRUE;
}

void NEAR PASCAL SetNewSSDemo(HWND hDlg, int iMethod)
{
    HBITMAP hbmOld;
    POINT ptIcon;
    HWND hwndDemo;
    HWND hwndC;
    HICON hicon;

    RECT rc = {MON_X, MON_Y, MON_X+MON_DX, MON_Y+MON_DY};

    hwndDemo = GetSSDemoParent( hDlg );

    // blank out the background with dialog color
    hbmOld = SelectObject(g_hdcMem, g_hbmDemo);
    FillRect(g_hdcMem, &rc, GetSysColorBrush(COLOR_DESKTOP));
    SelectObject(g_hdcMem, hbmOld);

    // kill the old one dammit kill it kill it kill kill kill
    while( hwndC = GetWindow( hwndDemo, GW_CHILD ) )
        SendMessage(hwndC, WM_CLOSE, 0, 0);
    Yield(); // paranoid
    Yield(); // really paranoid
    ShowWindow(hwndDemo, SW_HIDE);
    g_fPreviewActive = FALSE;

    if (iMethod >= 0 && aszMethods[iMethod][0] == TEXT('P'))
    {
        RECT rc;
        BITMAP bm;
        UpdateWindow(hDlg);
        //UpdateWindow(GetDlgItem(hDlg, IDC_BIGICON));

        GetObject(g_hbmDemo, sizeof(bm), &bm);
        GetClientRect(GetDlgItem(hDlg, IDC_BIGICON), &rc);
        rc.left = ( rc.right - bm.bmWidth ) / 2 + MON_X;
        rc.top = ( rc.bottom - bm.bmHeight ) / 2 + MON_Y;
        MoveWindow(hwndDemo, rc.left, rc.top, MON_DX, MON_DY, FALSE);
        wsprintf(szBuffer, TEXT("%s /p %d"), szSaverName, hwndDemo);
        if (WinExecN(szBuffer, SW_NORMAL) > 32)
        {
            ShowWindow(hwndDemo, SW_SHOWNA);
            g_fPreviewActive = TRUE;
            return;
        }
    }

    if (iMethod != -1)
    {
        ptIcon.x = GetSystemMetrics(SM_CXICON);
        ptIcon.y = GetSystemMetrics(SM_CYICON);

        // draw the icon double size
        //Assert(ptIcon.y*2 <= MON_DY);
        //Assert(ptIcon.x*2 <= MON_DX);

        hicon = hIcons[iMethod];

        if (hicon == NULL && aszMethods[iMethod][0] == TEXT('I'))
            hicon = hIdleWildIcon;
        if (hicon == NULL)
            hicon = hDefaultIcon;

        hbmOld = SelectObject(g_hdcMem, g_hbmDemo);
        DrawIconEx(g_hdcMem,
            MON_X + (MON_DX-ptIcon.x*2)/2,
            MON_Y + (MON_DY-ptIcon.y*2)/2,
            hicon, ptIcon.x*2, ptIcon.y*2, 0, NULL, DI_NORMAL);
        SelectObject(g_hdcMem, hbmOld);
    }

    InvalidateRect(GetDlgItem(hDlg, IDC_BIGICON), NULL, FALSE);
}

static void NEAR PASCAL SS_SomethingChanged(HWND hDlg)
{
    if (!g_bInitSS)
    {
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
    }
}

static void NEAR PASCAL SetScreenSaverPassword(HWND hDlg, int iMethod)
{
    if (iMethod >= 0 && aszMethods[iMethod][0] == TEXT('P'))
    {
        wsprintf(szBuffer, TEXT("%s /a %u"), szSaverName, GetParent(hDlg));
        WinExecN(szBuffer, SW_NORMAL);
    }
}


BOOL APIENTRY  ScreenSaverDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    PTSTR  pszMethod;
    int   wTemp;
    int   wMethod;
    BOOL  fEnable;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_APPLY:
                    /* Make sure the time we have is the last one entered... */
                    SendMessage(hDlg, WM_COMMAND, MAKELONG( IDC_SCREENSAVEDELAY, EN_KILLFOCUS),
                            (LPARAM)GetDlgItem(hDlg, IDC_SCREENSAVEDELAY));

                    /* Try to save the current settings... */
                    SaveIni(hDlg);

                    /////RETTRUE(hDlg)
                    break;

                // nothing to do on cancel...
                case PSN_RESET:
                    if (g_fPreviewActive)
                        SetNewSSDemo(hDlg, -1);
                    break;

                case PSN_KILLACTIVE:
                    if (g_fPreviewActive)
                        SetNewSSDemo(hDlg, -1);
                    break;

                case PSN_SETACTIVE:
                    EnableDisablePowerDelays(hDlg);

                    if (!g_fPreviewActive)
                    {
                        g_bInitSS = TRUE;
                        SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_CHOICES, CBN_SELCHANGE),
                                    (LPARAM)GetDlgItem(hDlg, IDC_CHOICES));
                        g_bInitSS = FALSE;
                    }
                    break;
            }
            break;

        case WM_INITDIALOG:
            g_bInitSS = TRUE;
            InitSSDialog(hDlg);
            g_bInitSS = FALSE;
            break;

        case WM_DISPLAYCHANGE:
        case WM_SYSCOLORCHANGE: {
            HBITMAP hbm;

            hbm = g_hbmDemo;

            g_hbmDemo = LoadMonitorBitmap( TRUE );

            if (g_hbmDemo) {
                // Got a new bitmap, use it and delete the old one.
                SendDlgItemMessage(hDlg,IDC_BIGICON,STM_SETIMAGE, IMAGE_BITMAP,(DWORD)g_hbmDemo);
                if (hbm) {
                    DeleteObject(hbm);
                }
            } else {
                // Couldn't get a new bitmap, just reuse the old one
                g_hbmDemo = hbm;
            }

            break;
        }


        case WM_DESTROY:
            FreeScrEntries();
            if (g_fPreviewActive)
                SetNewSSDemo(hDlg, -1);
            if (g_hbmDemo)
            {
                SendDlgItemMessage(hDlg,IDC_BIGICON,STM_SETIMAGE,IMAGE_BITMAP,
                    (LPARAM)NULL);
                DeleteObject(g_hbmDemo);
            }
            if (g_hbmEnergyStar)
            {
                SendDlgItemMessage(hDlg,IDC_ENERGYSTAR_BMP,STM_SETIMAGE,
                    IMAGE_BITMAP, (LPARAM)NULL);
                DeleteObject(g_hbmEnergyStar);
            }
            break;

        case WM_VSCROLL:
            if (LOWORD(wParam) == SB_THUMBPOSITION)
                ScreenSaver_AdjustTimeouts(hDlg, GetDlgCtrlID((HWND)lParam) - BCI_ARROW);
            break;

        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD)aSaverHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD) aSaverHelpIds);
            break;

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            ForwardSSDemoMsg(hDlg, message, wParam, lParam);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                /* Check for a selection change in the combo box. If there is
                    one, then update the method number as well as the
                    configure button... */
                case IDC_CHOICES:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        /* Dump the name of the current selection into
                            the buffer... */
                        wTemp = SendDlgItemMessage(hDlg,IDC_CHOICES,
                            CB_GETCURSEL,0,0l);
                        if(wTemp)
                        {
                            wMethod = SendDlgItemMessage(hDlg,IDC_CHOICES,
                                CB_GETITEMDATA,wTemp,0l);

                            /* Grey the button accordingly... */
                            pszMethod = aszMethods[wMethod];
                            if(pszMethod[0] == TEXT('C') ||       // can config
                               pszMethod[0] == TEXT('I') ||       // IdleWild
                               pszMethod[0] == TEXT('P') )        // can preview
                                EnableDlgItem(hDlg,IDC_SETTING, TRUE);
                            else
                                EnableDlgItem(hDlg,IDC_SETTING, FALSE);


#ifndef WINNT   // All screen savers on NT can use password protection
                            if(pszMethod[0] == TEXT('P'))         // is a Win32 4.0 saver
                            {
#endif
                                EnableDlgItem(hDlg,IDC_USEPASSWORD, TRUE);
                                CheckDlgButton( hDlg, IDC_USEPASSWORD,
                                    g_fPasswordWasPreviouslyEnabled );
                                EnableDisableSetPasswordBtn(hDlg);
#ifndef WINNT   // All screen savers on NT can use password protection
                            }
                            else
                            {
                                g_fPasswordWasPreviouslyEnabled =
                                    IsDlgButtonChecked( hDlg, IDC_USEPASSWORD );
                                CheckDlgButton( hDlg, IDC_USEPASSWORD, FALSE );
                                EnableDlgItem(hDlg,IDC_USEPASSWORD,FALSE);
                                EnableDlgItem(hDlg,IDC_SETPASSWORD,FALSE);
                            }
#endif

                            /* For fun, create an extra copy of
                                szSaverName... */
                            pszMethod = aszFiles[wMethod];
                            lstrcpy(szSaverName,pszMethod);

                            fEnable = TRUE;
                        }
                        else
                        {
                            wMethod = -1;
                            lstrcpy(szSaverName, g_szNULL);

                            EnableDlgItem(hDlg,IDC_SETTING,FALSE);
                            EnableDlgItem(hDlg,IDC_USEPASSWORD,FALSE);
#ifndef WINNT
                            EnableDlgItem(hDlg,IDC_SETPASSWORD,FALSE);
#endif

                            fEnable = FALSE;
                        }

                        //  Following are enabled as a group... (oh really?)
                        EnableDlgItem(hDlg,IDC_SSDELAYLABEL,fEnable);
                        EnableDlgItem(hDlg,IDC_SCREENSAVEDELAY,fEnable);
                        EnableDlgItem(hDlg,IDC_SCREENSAVEARROW,fEnable);
                        EnableDlgItem(hDlg,IDC_SSDELAYSCALE,fEnable);
                        EnableDlgItem(hDlg,IDC_TEST,fEnable);

                        g_iMethod = (int)wMethod;
                        SetNewSSDemo(hDlg, wMethod);
                        SS_SomethingChanged(hDlg);
                    }
                    break;

                /* If the edit box loses focus, translate... */
                case IDC_SCREENSAVEDELAY:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ScreenSaver_AdjustTimeouts(hDlg, LOWORD(wParam));
                    break;

                case IDC_LOWPOWERCONFIG:
                    // Configure the low power timeout event.
                    WinExec("rundll32 shell32.dll,Control_RunDLL powercfg.cpl,,",SW_SHOWNORMAL);
                    break;

                /* If the user wishes to test... */
                 case IDC_TEST:
                    switch( HIWORD( wParam ) )
                    {
                        case BN_CLICKED:
                        DoScreenSaver(hDlg,TRUE);
                        break;
                    }
                    break;

                /* Tell the DLL that it can do the configure... */
                case IDC_SETTING:
                if (HIWORD(wParam) == BN_CLICKED) {
                    DoScreenSaver(hDlg,FALSE);
                    break;
                }

                case IDC_USEPASSWORD:
                if (HIWORD(wParam) == BN_CLICKED) {

                    g_fPasswordWasPreviouslyEnabled =
                        IsDlgButtonChecked( hDlg, IDC_USEPASSWORD );
                    EnableDisableSetPasswordBtn(hDlg);
                    SS_SomethingChanged(hDlg);
                    break;
                }

                case IDC_SETPASSWORD:
                if (HIWORD(wParam) == BN_CLICKED) {

                    // ask new savers to change passwords
                    wTemp = SendDlgItemMessage(hDlg,IDC_CHOICES,
                        CB_GETCURSEL,0,0l);
                    if(wTemp)
                    {
                        SetScreenSaverPassword(hDlg,
                            (int)SendDlgItemMessage(hDlg,IDC_CHOICES,
                            CB_GETITEMDATA,wTemp,0l));
                    }
                    break;
                }
            }
            break;

        case WM_CTLCOLORSTATIC:
            if( (HWND)lParam == GetSSDemoParent( hDlg ) )
            {
                return (BOOL)GetStockObject( NULL_BRUSH );
            }
            break;
    }
    return FALSE;
}

/*******************************************************************************
*
*  ScreenSaver_AdjustTimeouts
*
*  DESCRIPTION:
*     Called whenever the user adjusts the delay of one of the time controls.
*     Adjusts the delays of the other time controls such that the screen saver
*     delay is less than the low power delay and that the low power delay is
*     less than the power off delay.
*
*  PARAMETERS:
*     hWnd, handle of ScreenSaver window.
*     BaseControlID, base control ID of the radio, edit, and arrow time control
*        combination.
*
*******************************************************************************/

VOID
NEAR PASCAL
ScreenSaver_AdjustTimeouts(
    HWND hWnd,
    int BaseControlID
    )
{

    BOOL fTranslated;
    int Timeout;
    int NewTimeout;

    //
    //  Get the new timeout for this time control and validate it's contents.
    //

    Timeout = (int) GetDlgItemInt(hWnd, BaseControlID + BCI_DELAY, &fTranslated,
        FALSE);
    Timeout = min(max(Timeout, 1), MAX_MINUTES);
    SetDlgItemInt(hWnd, BaseControlID + BCI_DELAY, (UINT) Timeout, FALSE);

    //
    //  Check the new value of this time control against the other timeouts,
    //  adjust their values if necessary.  Be careful when changing the order
    //  of these conditionals.
    //

    if (BaseControlID == IDC_SCREENSAVEDELAY) {

        if (g_Timeout[TA_SCREENSAVE] != Timeout) {

            g_Timeout[TA_SCREENSAVE] = Timeout;

            SS_SomethingChanged(hWnd);

        }

    }

    else {

        if (Timeout < g_Timeout[TA_SCREENSAVE]) {

            g_Timeout[TA_SCREENSAVE] = Timeout;
            SetDlgItemInt(hWnd, IDC_SCREENSAVEDELAY, (UINT) Timeout, FALSE);

        }

    }

    if (BaseControlID == IDC_POWEROFFDELAY) {

        if (g_Timeout[TA_POWEROFF] != Timeout) {

            g_Timeout[TA_POWEROFF] = Timeout;

            SS_SomethingChanged(hWnd);

        }

    }

    else {

        if (Timeout > g_Timeout[TA_POWEROFF]) {

            g_Timeout[TA_POWEROFF] = Timeout;
            SetDlgItemInt(hWnd, IDC_POWEROFFDELAY, (UINT) Timeout, FALSE);

        }

    }

    if (BaseControlID == IDC_LOWPOWERDELAY) {

        if (g_Timeout[TA_LOWPOWER] != Timeout) {

            g_Timeout[TA_LOWPOWER] = Timeout;

            SS_SomethingChanged(hWnd);

        }

    }

    else {

        NewTimeout = min(max(g_Timeout[TA_LOWPOWER], g_Timeout[TA_SCREENSAVE]),
            g_Timeout[TA_POWEROFF]);

        if (NewTimeout != g_Timeout[TA_LOWPOWER]) {

            g_Timeout[TA_LOWPOWER] = Timeout;
            SetDlgItemInt(hWnd, IDC_LOWPOWERDELAY, (UINT) NewTimeout, FALSE);

        }

    }

}

/*******************************************************************************
*
*  ScreenSaver_EnableDisableDelays
*
*  DESCRIPTION:
*     Called whenever the user clicks one of the time controls.  Enables or
*     disables the "buddy" edit and arrow controls.
*
*  PARAMETERS:
*     hWnd, handle of ScreenSaver window.
*     BaseControlID, base control ID of the radio, edit, and arrow time control
*        combination.
*
*******************************************************************************/

void NEAR PASCAL ScreenSaver_EnableDisableDelays(HWND hWnd,int BaseControlID)
{
    BOOL fEnable;

    fEnable = (IsDlgButtonChecked(hWnd, BaseControlID + BCI_SWITCH) == 1);

    EnableDlgItem(hWnd, BaseControlID + BCI_DELAY, fEnable);
    EnableDlgItem(hWnd, BaseControlID + BCI_ARROW, fEnable);

}

void NEAR PASCAL EnableDisablePowerDelays(HWND hDlg)
{

    int i;
    // BOOL fDPMS;
    static idCtrls[] = { IDC_ENERGY_TEXT,
                         IDC_ENERGY_TEXT2,
                         IDC_ENERGYSTAR_BMP,
                         IDC_LOWPOWERCONFIG,
                         0 };

    for (i = 0; idCtrls[i] != 0; i++)
        ShowWindow( GetDlgItem( hDlg, idCtrls[i] ), g_fAdapPwrMgnt ? SW_SHOWNA : SW_HIDE );

/*    if( g_fAdapPwrMgnt )
    {
#ifdef LATER
        //11-Jul-1995 JonPa BUGBUG use this when we get DPMS for NT
        fDPMS = GetMonitorDPMS();
#else
        fDPMS = TRUE;
#endif

        // Enable/Disable the power related timer switches based on Monitor
        // DPMS capability
        EnableDlgItem(hDlg,IDC_ENERGY_TEXT, fDPMS);
        EnableDlgItem(hDlg,IDC_LOWPOWERDELAY+BCI_SWITCH, fDPMS);
        EnableDlgItem(hDlg,IDC_POWEROFFDELAY+BCI_SWITCH, fDPMS);

        // If DPMS compliant, enable/disable the other power timer values
        // based on the switch setting, otherwise disable everything

        if (fDPMS)
        {
            ScreenSaver_EnableDisableDelays(hDlg,IDC_LOWPOWERDELAY);
            ScreenSaver_EnableDisableDelays(hDlg,IDC_POWEROFFDELAY);
        }
        else
        {
            EnableDlgItem(hDlg,IDC_LOWPOWERDELAY+BCI_DELAY, FALSE);
            EnableDlgItem(hDlg,IDC_LOWPOWERDELAY+BCI_ARROW, FALSE);
            EnableDlgItem(hDlg,IDC_POWEROFFDELAY+BCI_DELAY, FALSE);
            EnableDlgItem(hDlg,IDC_POWEROFFDELAY+BCI_ARROW, FALSE);
        }
    }*/
}

// keep the "set password" button enabled/disabled
// according to whether "use password" is checked or not
void NEAR PASCAL EnableDisableSetPasswordBtn(HWND hDlg)
{
    BOOL fEnable = IsDlgButtonChecked( hDlg,IDC_USEPASSWORD );

#ifndef WINNT
    EnableDlgItem( hDlg, IDC_SETPASSWORD, fEnable );
#endif
}

/* This routine will search for entries that are screen savers.  The directory
    searched is either the system directory (.. */

void NEAR PASCAL SearchForScrEntries(UINT wDir, LPCTSTR file)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szPath2[MAX_PATH];
    HANDLE hfind;
    WIN32_FIND_DATA fd;

    // don't do any work if no space left
    if( wNumMethods >= MAX_METHODS )
        return;

    /* Get the directory where the program resides... */
    GetModuleFileName(hInstance, szPath, ARRAYSIZE(szPath));
    StripPathName(szPath);

    switch ( wDir )
    {
        case SFSE_WINDOWS:
            /* Search the windows directory and place the path with the \ in
                the szPath variable... */
            GetWindowsDirectory(szPath2, ARRAYSIZE(szPath2));

sfseSanityCheck:
            /* if same dir as where it was launched, don't search again */
            if (!lstrcmpi(szPath, szPath2))
               return;

            lstrcpy(szPath, szPath2);
            break;

        case SFSE_SYSTEM:
            /* Search the system directory and place the path with the \ in
                the szPath variable... */
            GetSystemDirectory(szPath2, ARRAYSIZE(szPath2));
            goto sfseSanityCheck;

        case SFSE_FILE:
            /* Search the directory containing 'file' */
            lstrcpy(szPath2, file);
            StripPathName(szPath2);
            goto sfseSanityCheck;
    }

    AppendPath(szPath, TEXT("*.scr"));

    if( ( hfind = FindFirstFile( szPath, &fd ) ) != INVALID_HANDLE_VALUE )
    {
        StripPathName(szPath);

        do
        {
            PTSTR pszDesc;
            BOOL fLFN;

            fLFN = !(fd.cAlternateFileName[0] == 0 ||
                    lstrcmp(fd.cFileName, fd.cAlternateFileName) == 0);

            lstrcpy(szPath2, szPath);
            AppendPath(szPath2, fd.cFileName);

            // Note: PerformCheck does an alloc
            if( ( pszDesc = PerformCheck( szPath2, fLFN ) ) != NULL )
            {
                BOOL bAdded = FALSE;
                UINT i;

                for( i = 0; i < wNumMethods; i++ )
                {
                    if( !lstrcmpi( pszDesc, aszMethods[ i ] ) )
                    {
                        bAdded = TRUE;
                        break;
                    }
                }

                if( !bAdded )
                {
                    PTSTR pszEntries;

                    // COMPATIBILITY: always use short name
                    // otherwise some apps fault when peeking at SYSTEM.INI
                    if( fLFN )
                    {
                        lstrcpy(szPath2, szPath);
                        AppendPath(szPath2, fd.cAlternateFileName);
                    }

                    if( ( pszEntries = AllocStr( szPath2 ) ) != NULL )
                    {
                        if (pszDesc[0] != TEXT('P'))
                            hIcons[wNumMethods] = ExtractIcon(hInstance, szPath2, 0);
                        else
                            hIcons[wNumMethods] = NULL;

                        aszMethods[wNumMethods] = pszDesc;
                        aszFiles[wNumMethods] = pszEntries;
                        wNumMethods++;
                        bAdded = TRUE;
                    }
                }

                if( !bAdded )
                    LocalFree((HLOCAL)pszDesc);
            }

        } while( FindNextFile( hfind, &fd ) && ( wNumMethods < MAX_METHODS ) );

        FindClose(hfind);
    }
    return;
}

//
//  This routine checks a given file to see if it is indeed a screen saver
//  executable...
//
//  a valid screen saver exe has the following description line:
//
//      SCRNSAVE [c] : description :
//
//      SCRNSAVE is a required name that indicates a screen saver.
//
PTSTR NEAR PASCAL PerformCheck(LPTSTR lpszFilename, BOOL fLFN)
{
    int  i;
    TCHAR chConfig=TEXT('C');       // assume configure
    LPTSTR pch;
    DWORD dw;
    WORD  Version;
    WORD  Magic;

    /* Get the description... */

    pch = szBuffer + 1;

    //
    //  if we have a LFN (Long File Name) dont bother getting the
    //  exe descrription
    //
    dw = GetExeInfo(lpszFilename, pch, ARRAYSIZE(szBuffer)-1, fLFN ? GEI_EXPVER : GEI_DESCRIPTION);
    Version = HIWORD(dw);
    Magic   = LOWORD(dw);

    if (dw == 0)
        return NULL;

    if (Magic == PEMAGIC || fLFN)
    {
        BOOL fGotName = FALSE;

        if (!fLFN) {
            HANDLE  hSaver;
            //
            // We have a 32 bit screen saver with a short name, look for an NT style
            // decription in it's string table
            //
            if (hSaver = LoadLibrary (lpszFilename)) {
                if (LoadString (hSaver, IDS_DESCRIPTION, pch, ARRAYSIZE(szBuffer) - (szBuffer - pch))) {
                    fGotName = TRUE;
                }
                FreeLibrary (hSaver);
            }
        }

        if (!fGotName) {
            //
            //  we have a LFN (LongFileName) or a Win32 screen saver,
            //  Win32 exe's in general dont have a description field so
            //  we assume they can configure.  We also try to build
            //  a "nice" name for it.
            //
            lstrcpy(pch, lpszFilename);

            pch = FileName(pch);                    // strip path part

            if ( ((TCHAR)CharUpper((LPTSTR)(pch[0]))) == TEXT('S') && ((TCHAR)CharUpper((LPTSTR)(pch[1]))) == TEXT('S'))     // map SSBEZIER.SCR to BEZIER.SCR
                pch+=2;

            pch = NiceName(pch);                    // map BEZIER.SCR to Bezier
        }
    }
    else
    {
        LPTSTR pchTemp;

        //
        //  we have a 8.3 file name 16bit screen saveer, parse the
        //  description string from the exehdr
        //
        /* Check to make sure that at least the 11 characters needed for info
            are there... */
        if (lstrlen(pch) < 9)
            return NULL;

        /* Check the first 8 characters for the string... */
        if (lstrncmp(TEXT("SCRNSAVE"), pch, 8))
            return NULL;

        // If successful, allocate enough space for the string and copy the
        // string to the new one...

        pch = pch + 8;                 // skip over 'SCRNSAVE'

        while (*pch==TEXT(' '))                   // advance over white space
            pch++;

        if (*pch==TEXT('C') || *pch==TEXT('c'))         // parse the configure flag
        {
            chConfig = TEXT('C');
            pch++;
        }

        if (*pch==TEXT('X') || *pch==TEXT('x'))         // parse the don't configure flag
            chConfig = *pch++;

        // we might be pointing at a name or separation goop
        pchTemp = pch;                      // remember this spot

        while (*pch && *pch!=TEXT(':'))           // find separator
            pch++;

        while (*pch==TEXT(':') || *pch==TEXT(' '))      // advance over whtspc/last colon
            pch++;

        // if we haven't found a name yet fall back on the saved location
        if (!*pch)
            pch = pchTemp;

        while (*pch==TEXT(':') || *pch==TEXT(' '))      // re-advance over whtspc
            pch++;

        /* In case the screen saver has version information information
            embedded after the name, check to see if there is a colon TEXT(':')
            in the description and replace it with a NULL... */

        for (i=0; pch[i]; i++)              //
        {
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
            if (IsDBCSLeadByte(pch[i]))
            {
                i++;
            }
            else
#endif
            if (pch[i]==TEXT(':'))
                pch[i]=0;
        }
// Space is OK for DBCS (FE)
        while(i>0 && pch[i-1]==TEXT(' '))         // remove trailing space
            pch[--i]=0;
    }

#ifdef DEBUG
    if (Magic == PEMAGIC)
        lstrcat(pch, TEXT(" (32-bit)"));          // add techy stuff.
    else
        lstrcat(pch, TEXT(" (16-bit)"));

    if (Version == 0x030A)
        lstrcat(pch, TEXT(" (3.10)"));

    if (Version == 0x0400)
        lstrcat(pch, TEXT(" (4.00)"));
#endif
    //
    // assume any Win32 4.0 screen saver can do Preview mode
    //
    if (chConfig == TEXT('C') && Version >= 0x0400 && Magic == PEMAGIC)
        chConfig = TEXT('P');                     // mark as configurable/preview

    pch[-1] = chConfig;
    return AllocStr(pch-1);
}


BOOL NEAR PASCAL FreeScrEntries( void )
{
    UINT wLoop;

    for(wLoop = 0; wLoop < wNumMethods; wLoop++)
    {
        if(aszMethods[wLoop] != NULL)
            LocalFree((HANDLE)aszMethods[wLoop]);
        if(aszFiles[wLoop] != NULL)
            LocalFree((HANDLE)aszFiles[wLoop]);
        if(hIcons[wLoop] != NULL)
            FreeResource(hIcons[wLoop]);
    }

    if (hDefaultIcon)
        FreeResource(hDefaultIcon);

    if (hIdleWildIcon)
        FreeResource(hIdleWildIcon);

    hDefaultIcon=hIdleWildIcon=NULL;
    wNumMethods = 0;

    return TRUE;
}


int NEAR PASCAL lstrncmp( LPTSTR lpszString1, LPTSTR lpszString2, int nNum )
{
    /* While we can still compare characters, compare.  If the strings are
        of different lengths, characters will be different... */
    while(nNum)
    {
        if(*lpszString1 != *lpszString2)
            return *lpszString1 - *lpszString2;
        lpszString1++;
        lpszString2++;
        nNum--;
    }
    return 0;
}


void NEAR PASCAL SaveIni(HWND hDlg )
{
    LPTSTR pszMethod;
    int  wMethod,wTemp;
    UINT Counter;
        HKEY hKey;

    /* Find the current method selection... */
    wTemp = 0;
    if(wNumMethods)
    {
        /* Dump the name of the current selection into the buffer... */
        wTemp = SendDlgItemMessage(hDlg,IDC_CHOICES,CB_GETCURSEL,0,0l);
        if(wTemp)
        {
            BOOL hasspace = FALSE;
            LPTSTR pc;

            wMethod = SendDlgItemMessage(hDlg,IDC_CHOICES,CB_GETITEMDATA,
                wTemp,0l);

            /* Dump the method name into the buffer... */
            pszMethod = aszFiles[wMethod];

            for( pc = pszMethod; *pc; pc++ )
            {
                if( *pc == TEXT(' ') )
                {
                    hasspace = TRUE;
                    break;
                }
            }

            if( hasspace )
            {
                // if we need to add quotes we'll need two sets
                // because GetBlahBlahProfileBlah APIs strip quotes
                wsprintf(szBuffer,TEXT("\"\"%s\"\""), pszMethod);
            }
            else
                lstrcpy(szBuffer, pszMethod);
        }
        else
            szBuffer[0] = TEXT('\0');
    }
    else
        szBuffer[0] = TEXT('\0');
    /* Save the buffer... */
    WritePrivateProfileString(g_szBoot, szMethodName,
        (szBuffer[0] != TEXT('\0') ? szBuffer : NULL), g_szSystemIni);

    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, wTemp, NULL, SPIF_UPDATEINIFILE);

    for (Counter = 0; Counter < (sizeof(g_TimeoutAssociation) /
        sizeof(TIMEOUT_ASSOCIATION)); Counter++) {

        SystemParametersInfo(g_TimeoutAssociation[Counter].taSetTimeoutAction,
            (UINT) (g_Timeout[Counter] * 60), NULL, SPIF_UPDATEINIFILE);

        if (Counter != TA_SCREENSAVE) {

            SystemParametersInfo(g_TimeoutAssociation[Counter].taSetActiveAction,
                IsDlgButtonChecked(hDlg,
                g_TimeoutAssociation[Counter].taBaseControlID + BCI_SWITCH),
                NULL, SPIF_UPDATEINIFILE);

        }

    }

    // save the state of the TEXT("use password") checkbox
    if (RegCreateKey(HKEY_CURRENT_USER,REGSTR_PATH_SCREENSAVE,&hKey) == ERROR_SUCCESS) {

        RegSetValueEx(hKey,SZ_USE_PASSWORD, 0, PWRD_REG_TYPE,
            PasswdRegData(IsDlgButtonChecked(hDlg,IDC_USEPASSWORD)),
            CB_USE_PWRD_VALUE);

        RegCloseKey(hKey);
    }

    /* Broadcast a WM_WININICHANGE message... */
    SendNotifyMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LONG)g_szWindows);
}

/*
 * Thread for DoScreenSaver()
 */
typedef struct {
    HWND    hDlg;
    TCHAR   szCmdLine[1];
} SSRUNDATA, *LPSSRUNDATA;

DWORD RunScreenSaverThread( LPVOID lpv ) {
    BOOL bSvrState;
    LPSSRUNDATA lpssrd;
    HWND hwndSettings, hwndPreview;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    HINSTANCE hiThd;
    TCHAR szPath[MAX_PATH];

    // Lock ourselves in mem so we don't fault if app unloads us
    GetModuleFileName(hInstance, szPath, ARRAYSIZE(szPath));
    hiThd = LoadLibrary( szPath );

    lpssrd = (LPSSRUNDATA)lpv;

    hwndSettings = GetDlgItem( lpssrd->hDlg, IDC_SETTING);
    hwndPreview  = GetDlgItem( lpssrd->hDlg, IDC_TEST);


    // Save previous screen saver state
    SystemParametersInfo( SPI_GETSCREENSAVEACTIVE,0, &bSvrState, FALSE);

    // Disable current screen saver
    if( bSvrState )
        SystemParametersInfo( SPI_SETSCREENSAVEACTIVE,FALSE, NULL, FALSE );

    // Stop the miniture preview screen saver
    if (g_fPreviewActive)
        SetNewSSDemo( lpssrd->hDlg, -1);

    // Exec the screen saver and wait for it to die
    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)SW_NORMAL;

    if (CreateProcess( NULL, lpssrd->szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation )){
        WaitForSingleObject( ProcessInformation.hProcess, INFINITE );
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
    }

    // Restore Screen saver state
    if( bSvrState )
        SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, bSvrState, NULL, FALSE );

    // Restart miniture preview
    PostMessage( lpssrd->hDlg, WM_COMMAND, MAKELONG(IDC_CHOICES, CBN_SELCHANGE),
                                    (LPARAM)GetDlgItem( lpssrd->hDlg, IDC_CHOICES));

    // Enable setting and preview buttons
    EnableWindow( hwndSettings, TRUE );
    EnableWindow( hwndPreview,  TRUE );

    LocalFree( lpv );

    if (hiThd)
        FreeLibraryAndExitThread( hiThd, 0 );

    return 0;
}


/* This routine actually calls the screen saver... */

void NEAR PASCAL DoScreenSaver(HWND hWnd, BOOL fSaver )
{
    LPSSRUNDATA lpssrd;
    DWORD id;
    HANDLE hThd;
    HWND hwndSettings, hwndPreview;

    if(szSaverName[0] == TEXT('\0'))
        return;

    if(fSaver)
        wsprintf(szBuffer,TEXT("%s /s"), szSaverName);
    else {
        wsprintf(szBuffer,TEXT("%s /c:%lu"), szSaverName, (DWORD)hWnd);
    }

    lpssrd = LocalAlloc( LMEM_FIXED, sizeof(*lpssrd) + (lstrlen(szBuffer)+1) * sizeof(TCHAR) );
    if (lpssrd == NULL)
        return;

    lpssrd->hDlg = hWnd;
    lstrcpy( lpssrd->szCmdLine, szBuffer );

    // Disable setting and preview buttons
    hwndSettings = GetDlgItem( hWnd, IDC_SETTING);
    hwndPreview  = GetDlgItem( hWnd, IDC_TEST);

    EnableWindow( hwndSettings, FALSE );
    EnableWindow( hwndPreview,  FALSE );

    hThd = CreateThread(NULL, 0, RunScreenSaverThread, lpssrd, 0, &id);
    if (hThd != NULL) {
        CloseHandle(hThd);
    } else {
        // Exec failed, re-enable setting and preview buttons and clean up thread params
        EnableWindow( hwndSettings, TRUE );
        EnableWindow( hwndPreview,  TRUE );
        LocalFree( lpssrd );
    }
}


#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

LPTSTR NEAR PASCAL FileName(LPTSTR szPath)
{
    LPTSTR   sz;

    for (sz=szPath; *sz; sz++)
        ;
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    for (; sz>szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz=CharPrev(szPath, sz))
#else
    for (; sz>=szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz--)
#endif
        ;
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    if ( !IsDBCSLeadByte(*sz) && (SLASH(*sz) || *sz == TEXT(':')) )
        sz = CharNext(sz);
    return  sz;
#else
    return ++sz;
#endif
}

void NEAR PASCAL AddBackslash(LPTSTR pszPath)
{
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    LPTSTR lpsz = &pszPath[lstrlen(pszPath)];
    lpsz = CharPrev(pszPath, lpsz);
    if ( *lpsz != TEXT('\\') )
#else
    if( pszPath[ lstrlen( pszPath ) - 1 ] != TEXT('\\') )
#endif
        lstrcat( pszPath, TEXT("\\") );
}


LPTSTR NEAR PASCAL StripPathName(LPTSTR szPath)
{
    LPTSTR   sz;
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    LPTSTR   szFile;
#endif
    sz = FileName(szPath);

#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    szFile = sz;
    if ( szFile >szPath+1 )
    {
        szFile = CharPrev( szPath, szFile );
        if (SLASH(*szFile))
        {
            szFile = CharPrev(szPath, szFile);
            if (*szFile != TEXT(':'))
                sz--;
        }
    }
#else
    if (sz > szPath+1 && SLASH(sz[-1]) && sz[-2] != TEXT(':'))
        sz--;
#endif

    *sz = 0;
    return szPath;
}

void NEAR PASCAL AppendPath(LPTSTR pszPath, LPTSTR pszSpec)
{
    AddBackslash(pszPath);
    lstrcat(pszPath, pszSpec);
}


PTSTR NEAR PASCAL AllocStr(LPTSTR szCopy)
{
    PTSTR sz;

    if (sz = (PTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * (lstrlen(szCopy)+1)))
        lstrcpy(sz,szCopy);

    return sz;
}

LPTSTR NEAR PASCAL NiceName(LPTSTR szPath)
{
    LPTSTR   sz;
    LPTSTR   lpsztmp;

    sz = FileName(szPath);

    for(lpsztmp = sz; *lpsztmp  && *lpsztmp != TEXT('.'); lpsztmp = CharNext(lpsztmp))
        ;
    *lpsztmp = TEXT('\0');

    if (IsCharUpper(sz[0]) && IsCharUpper(sz[1]))
    {
        CharLower(sz);
        CharUpperBuff(sz, 1);
    }

    return sz;
}
