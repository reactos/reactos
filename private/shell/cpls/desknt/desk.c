#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <shlobj.h>

//#include <shellp.h>
#include <shsemip.h>
#include "desk.h"
#include "deskid.h"
#include "rc.h"
#include "setinc.h"
#include <regstr.h>
#include <cplext.h>
#include <shellp.h>


typedef struct {
    int id;
    DLGPROC pfnDlgProc;
    LPCTSTR szRegChkName;
} PAGEINFO;


/*
 * Local Constant Declarations
 */
static const TCHAR c_szCoverClass[] = TEXT("DeskSaysNoPeekingItsASurprise");
LRESULT CALLBACK CoverWindowProc( HWND, UINT, WPARAM, LPARAM );

#define MAX_PAGES 24

///////////////////////////////////////////////////////////////////////////////
// location of prop sheet hookers in the registry
///////////////////////////////////////////////////////////////////////////////
static const TCHAR sc_szRegDisplay[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display");
const TCHAR szRestrictionKey[] = REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM;
const TCHAR szNoBackgroundPage[] = REGSTR_VAL_DISPCPL_NOBACKGROUNDPAGE;
const TCHAR szNoSaverPage[] = REGSTR_VAL_DISPCPL_NOSCRSAVPAGE;
const TCHAR szNoAppearancePage[] = REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE;
const TCHAR szNoSettingsPage[] = REGSTR_VAL_DISPCPL_NOSETTINGSPAGE;
const TCHAR szNoDispCPL[] = REGSTR_VAL_DISPCPL_NODISPCPL;



///////////////////////////////////////////////////////////////////////////////
// Array defining each page in the sheet
///////////////////////////////////////////////////////////////////////////////

#define SettingsDlgProc ((DLGPROC)-1)

PAGEINFO aPageInfo[] = {
    { DLG_BACKGROUND,  BackgroundDlgProc,  szNoBackgroundPage },
    { DLG_SCREENSAVER, ScreenSaverDlgProc, szNoSaverPage },
    { DLG_APPEARANCE,  AppearanceDlgProc,  szNoAppearancePage },
    { DLG_MONITOR,     SettingsDlgProc,    szNoSettingsPage }       // SETTINGS MUST BE LAST PAGE!!!
};

#define C_PAGES_DESK    ARRAYSIZE(aPageInfo)
#define IPI_SETTINGS    (C_PAGES_DESK-1)        // Index to "Settings" page

/*---------------------------------------------------------
**
**---------------------------------------------------------*/
BOOL NEAR PASCAL CreateGlobals()
{
    WNDCLASS wc;
    HBITMAP hbm;
    HDC hdc;

    if( !GetClassInfo( hInstance, c_szCoverClass, &wc ) )
    {
        // if two pages put one up, share one dc
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = CoverWindowProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = (HICON)( wc.hCursor = NULL );
        // use a real brush since user will try to paint us when we're "hung"
        wc.hbrBackground = GetStockObject( NULL_BRUSH );
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szCoverClass;

        if( !RegisterClass( &wc ) )
            return FALSE;
    }

    hdc = GetDC(NULL);
    g_hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    if (!g_hdcMem)
        return FALSE;

    hbm = CreateBitmap(1, 1, 1, 1, NULL);
    g_hbmDefault = SelectObject(g_hdcMem, hbm);
    SelectObject(g_hdcMem, g_hbmDefault);
    DeleteObject(hbm);

    LoadString(hInstance, IDS_NONE, g_szNone, ARRAYSIZE(g_szNone));
    LoadString(hInstance, IDS_CLOSE, g_szClose, ARRAYSIZE(g_szClose));

    RegisterBackPreviewClass(hInstance);
    RegisterLookPreviewClass(hInstance);

    return TRUE;
}

/*---------------------------------------------------------
**
**---------------------------------------------------------*/

HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop )
{
    HBITMAP hbm,hbmT;
    BITMAP bm;
    HBRUSH hbrT;
    HDC hdc;
    COLORREF c3df = GetSysColor( COLOR_3DFACE );

    hbm = LoadBitmap(hInstance, MAKEINTRESOURCE(BMP_MONITOR));

    if (hbm == NULL)
    {
        //Assert(0);
        return NULL;
    }

    //
    // convert the "base" of the monitor to the right color.
    //
    // the lower left of the bitmap has a transparent color
    // we fixup using FloodFill
    //
    hdc = CreateCompatibleDC(NULL);
    hbmT = SelectObject(hdc, hbm);
    hbrT = SelectObject(hdc, GetSysColorBrush(COLOR_3DFACE));

    GetObject(hbm, sizeof(bm), &bm);

    ExtFloodFill(hdc, 0, bm.bmHeight-1,
        GetPixel(hdc, 0, bm.bmHeight-1), FLOODFILLSURFACE);

    // round off the corners
    // the bottom two were done by the floodfill above
    // the top left is important since SS_CENTERIMAGE uses it to fill gaps
    // the top right should be rounded because the other three are
    SetPixel( hdc, 0, 0, c3df );
    SetPixel( hdc, bm.bmWidth-1, 0, c3df );

    // unless the caller would like to do it, we fill in the desktop here
    if( bFillDesktop )
    {
        SelectObject(hdc, GetSysColorBrush(COLOR_DESKTOP));

        ExtFloodFill(hdc, MON_X+1, MON_Y+1,
            GetPixel(hdc, MON_X+1, MON_Y+1), FLOODFILLSURFACE);
    }

    // clean up after ourselves
    SelectObject(hdc, hbrT);
    SelectObject(hdc, hbmT);
    DeleteDC(hdc);

    return hbm;
}

///////////////////////////////////////////////////////////////////////////////
//
// InstallScreenSaver
//
// Provides a RUNDLL32-callable routine to install a screen saver
//
///////////////////////////////////////////////////////////////////////////////


#ifdef UNICODE
//
// Windows NT:
//
// Thunk ANSI version to the Unicode function
//
void WINAPI InstallScreenSaverW( HWND wnd, HINSTANCE inst, LPWSTR cmd, int shw );

void WINAPI InstallScreenSaverA( HWND wnd, HINSTANCE inst, LPSTR cmd, int shw )
{
    LPWSTR  pwszCmd;
    int     cch;

    cch = MultiByteToWideChar( CP_ACP, 0, cmd, -1, NULL, 0);
    if (cch == 0)
        return;

    pwszCmd = LocalAlloc( LMEM_FIXED, cch * SIZEOF(TCHAR) );
    if (pwszCmd == NULL)
        return;

    MultiByteToWideChar( CP_ACP, 0, cmd, -1, pwszCmd, cch);
    InstallScreenSaverW(wnd, inst, pwszCmd, shw);

    LocalFree(pwszCmd);
}

#   define REAL_INSTALL_SCREEN_SAVER   InstallScreenSaverW

#else

//
// Windows 95:
//
// Stub out Unicode version
//
void WINAPI InstallScreenSaverW( HWND wnd, HINSTANCE inst, LPWSTR cmd, int shw )
{
    SetLastError( ERROR_NOT_IMPLEMENTED );
    return;
}

#   define REAL_INSTALL_SCREEN_SAVER   InstallScreenSaverA

#endif


void WINAPI REAL_INSTALL_SCREEN_SAVER( HWND wnd, HINSTANCE inst, LPTSTR cmd, int shw )
{
    TCHAR buf[ MAX_PATH ];
    TCHAR CmdLine[] = TEXT("DESK.CPL,,1");
    int timeout;

    lstrcpy( buf, cmd );
    PathGetShortPath( buf ); // so msscenes doesn't die
    WritePrivateProfileString( TEXT("boot"), TEXT("SCRNSAVE.EXE"), buf, TEXT("system.ini") );

    SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, TRUE, NULL,
        SPIF_UPDATEINIFILE );

    // make sure the user has a non-stupid timeout set
    SystemParametersInfo( SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0 );
    if( timeout <= 0 )
    {
        // 15 minutes seems like a nice default
        SystemParametersInfo( SPI_SETSCREENSAVETIMEOUT, 900, NULL,
            SPIF_UPDATEINIFILE );
    }

    // bring up the screen saver page on our rundll
#ifdef UNICODE
    Control_RunDLLW( wnd, inst, CmdLine, shw );
#else
    Control_RunDLL( wnd, inst, CmdLine, shw );
#endif
}

/*****************************************************************************\
*
* DeskInitCpl( void )
*
\*****************************************************************************/

BOOL DeskInitCpl(void) {

    InitCommonControls();

    CreateGlobals();

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// _AddDisplayPropSheetPage  adds pages for outside callers...
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
_AddDisplayPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *)lParam;

    if( hpage && ( ppsh->nPages < MAX_PAGES ) )
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}



static int
GetClInt( const TCHAR *p )
{
    BOOL neg = FALSE;
    int v = 0;

    while( *p == TEXT(' ') )
        p++;                        // skip spaces

    if( *p == TEXT('-') )                 // is it negative?
    {
        neg = TRUE;                     // yes, remember that
        p++;                            // skip '-' char
    }

    // parse the absolute portion
    while( ( *p >= TEXT('0') ) && ( *p <= TEXT('9') ) )     // digits only
        v = v * 10 + *p++ - TEXT('0');    // accumulate the value

    return ( neg? -v : v );         // return the result
}




//  Checks the given restriction.  Returns TRUE (restricted) if the
//  specified key/value exists and is non-zero, false otherwise
BOOL CheckRestriction(HKEY hKey,LPCTSTR lpszValueName)
{
    DWORD dwData,dwSize=sizeof(dwData);
    if ( (RegQueryValueEx(hKey,lpszValueName,NULL,NULL,
        (BYTE *) &dwData,&dwSize) == ERROR_SUCCESS) &&
        dwData)
        return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// CreateReplaceableHPSXA creates a new hpsxa that contains only the
// interfaces with valid ReplacePage methods.
// APPCOMPAT - EzDesk only implemented AddPages.  ReplacePage is NULL for them.
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    UINT count, alloc;
    IShellPropSheetExt *interfaces[0];
} PSXA;

HPSXA
CreateReplaceableHPSXA(HPSXA hpsxa)
{
    PSXA *psxa = (PSXA *)hpsxa;
    DWORD cb = SIZEOF(PSXA) + SIZEOF(IShellPropSheetExt *) * psxa->alloc;
    PSXA *psxaRet = (PSXA *)LocalAlloc(LPTR, cb);

    if (psxaRet)
    {
        UINT i;

        psxaRet->count = 0;
        psxaRet->alloc = psxa->alloc;

        for (i=0; i<psxa->count; i++)
        {
            if (psxa->interfaces[i]->lpVtbl->ReplacePage)
            {
                psxaRet->interfaces[psxaRet->count++] = psxa->interfaces[i];
            }
        }
    }

    return (HPSXA)psxaRet;
}

#define DestroyReplaceableHPSXA(hpsxa) LocalFree((HLOCAL)hpsxa)

///////////////////////////////////////////////////////////////////////////////
// SetStartPage checks the command line for start page by name.
///////////////////////////////////////////////////////////////////////////////
void SetStartPage(PROPSHEETHEADER *ppsh, LPCTSTR pszCmdLine)
{
    if (pszCmdLine)
    {
        //
        // Strip spaces
        //
        while (*pszCmdLine == TEXT(' '))
        {
            pszCmdLine++;
        }

        //
        // Check for @ sign.
        //
        if (*pszCmdLine == TEXT('@'))
        {
            UINT i;
            LPTSTR pszStartPage;
            LPCTSTR pszBegin;
            BOOL fInQuote = FALSE;
            int cchLen;

            pszCmdLine++;

            //
            // Skip past a quote
            //
            if (*pszCmdLine == TEXT('"'))
            {
                pszCmdLine++;
                fInQuote = TRUE;
            }

            //
            // Save the beginning of the name.
            //
            pszBegin = pszCmdLine;

            //
            // Find the end of the name.
            //
            while (*pszCmdLine &&
                   (fInQuote || *pszCmdLine != TEXT(' ')) &&
                   (!fInQuote || *pszCmdLine != TEXT('"')))
            {
                pszCmdLine++;
            }
            cchLen = pszCmdLine - pszBegin;

            //
            // Store the name in the pStartPage field.
            //
            pszStartPage = (LPTSTR)LocalAlloc(LPTR, (cchLen+1) * SIZEOF(TCHAR));
            if (pszStartPage)
            {
                lstrcpyn(pszStartPage, pszBegin, cchLen+1);

                ppsh->dwFlags |= PSH_USEPSTARTPAGE;
                ppsh->pStartPage = pszStartPage;
            }
        }
    }
}

/*****************************************************************************\
*
* DeskShowPropSheet( HWND hwndParent )
*
\*****************************************************************************/

typedef HRESULT (*LPFNCOINIT)(LPVOID);
typedef HRESULT (*LPFNCOUNINIT)(void);

void DeskShowPropSheet( HINSTANCE hInst, HWND hwndParent, LPCTSTR cmdline )
{
    HPROPSHEETPAGE hpsp, ahPages[MAX_PAGES];
    HPSXA hpsxa = NULL;
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
    LPARAM lParamSettings;
    HKEY hKey;
    int i;
    DWORD exitparam = 0UL;
    BOOL fSettingsPage = FALSE;
    HINSTANCE hinstOLE32 = NULL;
    LPFNCOINIT lpfnCoInitialize = NULL;
    LPFNCOUNINIT lpfnCoUninitialize = NULL;

    //
    // check if whole sheet is locked out
    //
    if ((RegOpenKey(HKEY_CURRENT_USER,szRestrictionKey,&hKey) == ERROR_SUCCESS)) {
        BOOL fDisableCPL = CheckRestriction(hKey,szNoDispCPL);
        RegCloseKey(hKey);
        if (fDisableCPL) {
            TCHAR szMessage[255],szTitle[255];
            LoadString( hInst, IDS_DISPLAY_DISABLED, szMessage, ARRAYSIZE(szMessage) );
            LoadString( hInst, IDS_DISPLAY_TITLE, szTitle, ARRAYSIZE(szTitle) );

            MessageBox( hwndParent, szMessage, szTitle, MB_OK | MB_ICONINFORMATION );

            return;
        }
    }

    //
    // Create the property sheet
    //
    ZeroMemory( &psh, sizeof(psh) );

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE;

    psh.hwndParent = hwndParent;
    psh.hInstance = hInst;

    psh.pszCaption = MAKEINTRESOURCE( IDS_DISPLAY_TITLE );
    psh.nPages = 0;
    psh.phpage = ahPages;

    psh.nStartPage = ( ( cmdline && *cmdline )? GetClInt( cmdline ) : 0 );


    ZeroMemory( &psp, sizeof(psp) );

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInst;


    if (RegOpenKey(HKEY_CURRENT_USER,szRestrictionKey,&hKey) != ERROR_SUCCESS)
        hKey = NULL;

    /*
     * Build the property sheet.  If we are under setup, then just include
     * the "settings" page, and no otheres
     */
    if (gbExecMode == EXEC_NORMAL) {

        //
        // Load the OLE library and grab the two interesting entry points.
        //
        hinstOLE32 = LoadLibrary(TEXT("OLE32.DLL"));
        if (hinstOLE32)
        {
            lpfnCoInitialize = (LPFNCOINIT)GetProcAddress(hinstOLE32, "CoInitialize");
            lpfnCoUninitialize = (LPFNCOUNINIT)GetProcAddress(hinstOLE32, "CoUninitialize");
            if (!lpfnCoInitialize || !lpfnCoUninitialize)
            {
                lpfnCoInitialize = NULL;
                lpfnCoUninitialize = NULL;
                FreeLibrary(hinstOLE32);
                hinstOLE32 = NULL;
            }
        }

        //
        // Call CoInitialize before calling propsheetextarray code.
        //
        if (lpfnCoInitialize)
        {
            lpfnCoInitialize(NULL);
        }

        // Load any extensions that are installed.
        hpsxa = SHCreatePropSheetExtArray( HKEY_LOCAL_MACHINE, sc_szRegDisplay, 8 );

        for( i = 0; i < C_PAGES_DESK; i++ ) {

            psp.pszTemplate = MAKEINTRESOURCE(aPageInfo[i].id);

            psp.pfnDlgProc = aPageInfo[i].pfnDlgProc;

            if(psp.pfnDlgProc == SettingsDlgProc) {
                // If this is the settings page, we need to add the additional pages
                // first.
                if( hpsxa )
                {
                    UINT cutoff = psh.nPages;
                    UINT added = SHAddFromPropSheetExtArray( hpsxa,
                        _AddDisplayPropSheetPage, (LPARAM)&psh );

                    if (psh.nStartPage >= cutoff)
                        psh.nStartPage += added;
                }

            }

            if (hKey != NULL && CheckRestriction(hKey,aPageInfo[i].szRegChkName)) {
                // This page is locked out by admin, don't put it up

                continue;
            }

            if (psp.pfnDlgProc == BackgroundDlgProc) {
                // This page can be overridden by extensions
                if( hpsxa ) {
                    UINT cutoff = psh.nPages;
                    UINT added = 0;

                    HPSXA hpsxaReplace = CreateReplaceableHPSXA(hpsxa);
                    if (hpsxaReplace)
                    {
                        added = SHReplaceFromPropSheetExtArray( hpsxaReplace, CPLPAGE_DISPLAY_BACKGROUND,
                            _AddDisplayPropSheetPage, (LPARAM)&psh );
                        DestroyReplaceableHPSXA(hpsxaReplace);
                    }

                    if (added) {
                        if (psh.nStartPage >= cutoff)
                            psh.nStartPage += added-1;
                        continue;
                    }
                }
            }

            if (psp.pfnDlgProc == SettingsDlgProc ) {
                // HACK - this hack is to mate the old NT settings applet c++ code
                // with the new win95 C display app.
                psp.pfnDlgProc = NewDisplayDialogBox(hInst, &lParamSettings);
                psp.lParam = lParamSettings;
                fSettingsPage = TRUE;   // remember to delete it when we are done
            } else
                psp.lParam = 0L;

            if (hpsp = CreatePropertySheetPage(&psp)) {
                psh.phpage[psh.nPages++] = hpsp;
            }
        }
    } else if (hKey == NULL || !CheckRestriction(hKey,aPageInfo[IPI_SETTINGS].szRegChkName)) {
        // This page is NOT locked out by admin, put it up

        psp.pszTemplate = MAKEINTRESOURCE(aPageInfo[IPI_SETTINGS].id);

        psp.pfnDlgProc = aPageInfo[IPI_SETTINGS].pfnDlgProc;


        // HACK - this hack is to mate the old NT settings applet c++ code
        // with the new win95 C display app.
        psp.pfnDlgProc = NewDisplayDialogBox(hInst, &lParamSettings);
        psp.lParam = lParamSettings;

        if (hpsp = CreatePropertySheetPage(&psp)) {
            psh.phpage[psh.nPages++] = hpsp;
        }
    }


    if (hKey != NULL)
        RegCloseKey(hKey);

    // fallback will sometimes do other work and return no pages
    if( psh.nPages )
    {
        SetStartPage(&psh, cmdline);

        // Bring the sucker up...
        if ( PropertySheet( &psh ) == ID_PSRESTARTWINDOWS)
        {
            exitparam = EW_RESTARTWINDOWS;
        }

        if (psh.dwFlags & PSH_USEPSTARTPAGE)
        {
            LocalFree((HANDLE)psh.pStartPage);
        }
    }

    GetLastError();

    // HACK - to remove C++ objects created by NewDisplayDialogBox()
    if (fSettingsPage)
        DeleteDisplayDialogBox(lParamSettings);

    // free any loaded extensions
    if( hpsxa )
        SHDestroyPropSheetExtArray( hpsxa );

    //
    // We are done with propsheetextarray code, uninitialize com
    // and unload OLE32 dll.
    //
    if (lpfnCoUninitialize)
    {
        lpfnCoUninitialize();
    }
    if (hinstOLE32)
    {
        lpfnCoInitialize = NULL;
        lpfnCoUninitialize = NULL;
        FreeLibrary(hinstOLE32);
        hinstOLE32 = NULL;
    }

    if (exitparam == EW_RESTARTWINDOWS)
        RestartDialog( hwndParent, NULL, exitparam );

    return;
}


///////////////////////////////////////////////////////////////////////////////
//
// CreateCoverWindow
//
// creates a window which obscures the display
//  flags:
//      0 means erase to black
//      COVER_NOPAINT means "freeze" the display
//
// just post it a WM_CLOSE when you're done with it
//
///////////////////////////////////////////////////////////////////////////////
DWORD gdwCoverStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

typedef struct {
    DWORD   flags;
    HWND    hwnd;
    HANDLE  heRetvalSet;
} CVRWNDPARM, * PCVRWNDPARM;

DWORD WINAPI CreateCoverWindowThread( LPVOID pv )
{
    PCVRWNDPARM pcwp = (PCVRWNDPARM)pv;
    MSG msg;

    pcwp->hwnd = CreateWindowEx( gdwCoverStyle,
        c_szCoverClass, g_szNULL, WS_POPUP | WS_VISIBLE | pcwp->flags, 0, 0,
        GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ),
        NULL, NULL, hInstance, NULL );


    if( pcwp->hwnd )
    {
        SetForegroundWindow( pcwp->hwnd );
        UpdateWindow( pcwp->hwnd );
    }

    // return wnd;
    SetEvent(pcwp->heRetvalSet);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg, NULL, 0L, 0L)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ExitThread(0);

    return 0;
}

HWND FAR PASCAL
CreateCoverWindow( DWORD flags )
{
    CVRWNDPARM cwp;
    HANDLE hThread;
    DWORD  idTh;
    DWORD dwWaitResult  = 0;

    // Init params
    cwp.flags = flags;
    cwp.hwnd = NULL;
    cwp.heRetvalSet = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (cwp.heRetvalSet == NULL)
        return NULL;

    // CreateThread
    hThread = CreateThread(NULL, 0, CreateCoverWindowThread, &cwp, 0, &idTh);

    CloseHandle(hThread);

    // Wait for Thread to return the handle to us
    do
    {
        dwWaitResult = MsgWaitForMultipleObjects(1,
                                                 &cwp.heRetvalSet,
                                                 FALSE,
                                                 INFINITE,
                                                 QS_ALLINPUT);
        switch(dwWaitResult)
        {
            case WAIT_OBJECT_0 + 1:
            {
                MSG msg ;
                //
                // Allow blocked thread to respond to sent messages.
                //
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if ( WM_QUIT != msg.message )
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    else
                    {
                        //
                        // Received WM_QUIT.
                        // Don't wait for event.
                        //
                        dwWaitResult = WAIT_FAILED;
                    }
                }
                break;
            }

            default:
                break;
        }
    }
    while((WAIT_OBJECT_0 + 1) == dwWaitResult);

    CloseHandle(cwp.heRetvalSet);
    return cwp.hwnd;
}

///////////////////////////////////////////////////////////////////////////////
// CoverWndProc (see CreateCoverWindow)
///////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK
CoverWindowProc( HWND window, UINT message, WPARAM wparam, LPARAM lparam )
{
    switch( message )
    {
        case WM_CREATE:
            SetTimer( window, ID_CVRWND_TIMER, CMSEC_COVER_WINDOW_TIMEOUT, NULL );
            break;

        case WM_TIMER:
            // Times up... Shut ourself down
            if (wparam == ID_CVRWND_TIMER)
                PostMessage(window, WM_CLOSE, 0, 0);
            break;

        case WM_ERASEBKGND:
            // NOTE: assumes our class brush is the NULL_BRUSH stock object
            if( !( GetWindowLong( window, GWL_STYLE ) & COVER_NOPAINT ) )
            {
                HDC dc = (HDC)wparam;
                RECT rc;

                if( GetClipBox( dc, (LPRECT)&rc ) != NULLREGION )
                {
                    FillRect( dc, (LPRECT)&rc, GetStockObject( BLACK_BRUSH ) );

                    // HACK: make sure fillrect is done before we return
                    // this is to better hide flicker during dynares-crap
                    GetPixel( dc, rc.left + 1, rc.top + 1 );
                }
            }
            break;

        case WM_ACTIVATE:
            if( GET_WM_ACTIVATE_STATE( wparam, lparam ) == WA_INACTIVE )
            {
                DestroyWindow( window );
                return 1L;
            }
            break;

        case WM_DESTROY:
            KillTimer(window, ID_CVRWND_TIMER);
            PostQuitMessage(0);
            break;
    }

    return DefWindowProc( window, message, wparam, lparam );
}




#if 0
BOOL APIENTRY XBackgroundDlgProc( HWND hDlg, UINT msg, UINT wParam, LONG lParam) {
    return DeskDefPropPageProc( hDlg, msg, wParam, lParam );
}
BOOL APIENTRY ScreenSvrDlgProc(  HWND hDlg, UINT msg, UINT wParam, LONG lParam) {
    return DeskDefPropPageProc( hDlg, msg, wParam, lParam );
}

BOOL APIENTRY AppearanceDlgProc( HWND hDlg, UINT msg, UINT wParam, LONG lParam) {
    return DeskDefPropPageProc( hDlg, msg, wParam, lParam );
}
#endif


#ifndef SettingsDlgProc
BOOL APIENTRY SettingsDlgProc(   HWND hDlg, UINT msg, UINT wParam, LONG lParam) {
    return DeskDefPropPageProc( hDlg, msg, wParam, lParam );
}
#endif

#if 0
BOOL APIENTRY DeskDefPropPageProc( HWND hDlg, UINT msg, UINT wParam, LONG lParam) {

    LPPROPSHEETPAGE lppsp;

    switch (msg) {
        case WM_INITDIALOG:                /* msg: initialize dialog box */
            lppsp = (LPPROPSHEETPAGE) lParam;
            return (TRUE);
    }

    return (FALSE);
}
#endif
