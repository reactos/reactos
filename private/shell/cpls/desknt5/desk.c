#include "precomp.h"
#include "winuser.h"
#ifdef WINNT
#include <regapi.h>
#include <ctxdef.h> // hydra stuff
#endif
#pragma hdrstop
#include "cplext.h"
#include "cplp.h"

///////////////////////////////////////////////////////////////////////////////
// Array defining each page in the sheet
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int id;
    DLGPROC pfnDlgProc;
    LPCTSTR szRegChkName;
} PAGEINFO;

PAGEINFO aPageInfo[] = {
    { DLG_BACKGROUND,   BackgroundDlgProc,   REGSTR_VAL_DISPCPL_NOBACKGROUNDPAGE },
    { DLG_SCREENSAVER,  ScreenSaverDlgProc,  REGSTR_VAL_DISPCPL_NOSCRSAVPAGE     },
    { DLG_APPEARANCE,   AppearanceDlgProc,   REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE },
    { DLG_MULTIMONITOR, MultiMonitorDlgProc, REGSTR_VAL_DISPCPL_NOSETTINGSPAGE   },
};

#define C_PAGES_DESK    ARRAYSIZE(aPageInfo)
#define IPI_SETTINGS    (C_PAGES_DESK-1)        // Index to "Settings" page
#define WALLPAPER     L"Wallpaper"

/*
 * Local Constant Declarations
 */
static const TCHAR sc_szCoverClass[] = TEXT("DeskSaysNoPeekingItsASurprise");
LRESULT CALLBACK CoverWindowProc( HWND, UINT, WPARAM, LPARAM );

VOID RefreshColors (void);

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

TCHAR gszDeskCaption[CCH_MAX_STRING];

TCHAR g_szNULL[] = TEXT("") ;
TCHAR g_szControlIni[] = TEXT("control.ini") ;
TCHAR g_szPatterns[] = TEXT("patterns") ;
TCHAR g_szNone[CCH_NONE];                      // this is the '(None)' string
TCHAR g_szBoot[] = TEXT("boot");
TCHAR g_szSystemIni[] = TEXT("system.ini");
TCHAR g_szWindows[] = TEXT("Windows");


HDC g_hdcMem;
HBITMAP g_hbmDefault;
BOOL g_bMirroredOS = FALSE;


EXEC_MODE gbExecMode = EXEC_NORMAL;
EXEC_INVALID_SUBMODE gbInvalidMode = NOT_INVALID;

///////////////////////////////////////////////////////////////////////////////
// Externs
///////////////////////////////////////////////////////////////////////////////
extern BOOL NEAR PASCAL GetStringFromReg(HKEY   hKey,
                                        LPCTSTR lpszSubkey,
                                        LPCTSTR lpszValueName,
                                        LPCTSTR lpszDefault,
                                        LPTSTR lpszValue,
                                        DWORD cchSizeofValueBuff);



/*---------------------------------------------------------
**
**---------------------------------------------------------*/
BOOL NEAR PASCAL CreateGlobals()
{
    WNDCLASS wc;
    HBITMAP hbm;
    HDC hdc;

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();

    if( !GetClassInfo( hInstance, sc_szCoverClass, &wc ) )
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
        wc.lpszClassName = sc_szCoverClass;

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
// Messagebox wrapper
//
//
///////////////////////////////////////////////////////////////////////////////


int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    DWORD dwTitleID,
    DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[2000];

    LoadString(hInstance, dwTextID, Text, SIZEOF(Text));
    LoadString(hInstance, dwTitleID, Title, SIZEOF(Title));

    return (ShellMessageBox(hInstance, hwnd, Text, Title, fuStyle));
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
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return;
}

#   define REAL_INSTALL_SCREEN_SAVER   InstallScreenSaverA

#endif


void WINAPI REAL_INSTALL_SCREEN_SAVER( HWND wnd, HINSTANCE inst, LPTSTR cmd, int shw )
{
    TCHAR buf[ MAX_PATH ];
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
    Control_RunDLLW( wnd, inst, TEXT("DESK.CPL,,1"), shw );
#else
    Control_RunDLL( wnd, inst, TEXT("DESK.CPL,,1"), shw );
#endif
}

/*****************************************************************************\
*
* DeskInitCpl( void )
*
\*****************************************************************************/

BOOL DeskInitCpl(void) {

    //
    // Private Debug stuff
    //
#if ANDREVA_DBG
    g_dwTraceFlags = 0xFFFFFFFF;
#endif


    InitCommonControls();

    CreateGlobals();

    return TRUE;
}


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
            cchLen = (int)(pszCmdLine - pszBegin);

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

/*****************************************************************************\
*
* DeskShowPropSheet( HWND hwndParent )
*
\*****************************************************************************/
typedef HRESULT (*LPFNCOINIT)(LPVOID);
typedef HRESULT (*LPFNCOUNINIT)(void);

int ComputeNumberOfDisplayDevices();


void DeskShowPropSheet( HINSTANCE hInst, HWND hwndParent, LPCTSTR cmdline )
{
#ifndef WINNT  // Comments below
    HINSTANCE hDesk16 = NULL;
    FARPROC16 pDesk16 = NULL;
#endif

    HPROPSHEETPAGE hpsp, ahPages[MAX_PAGES];
    HPSXA hpsxa = NULL;
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
    HINSTANCE hinstOLE32 = NULL;
    LPFNCOINIT lpfnCoInitialize = NULL;
    LPFNCOUNINIT lpfnCoUninitialize = NULL;
    HKEY hKey;
    int i;
    DWORD exitparam = 0UL;


    //
    // move win95 OEM registry handlers around
    //
    // On NT, we don't do this since we don't have PnP for NT 4.0, and we
    // therefore have to software section associated to the driver.
    // We do redefine the layout of the Controls Folder so that all extensions
    // will only show up under the Advacned settings tab - but they will remain
    // global until the extensions are rev'ed for NT 5.0
    //

#ifndef WINNT
    FixupRegistryHandlers();
#endif

    //
    // check if whole sheet is locked out
    //

    if ((RegOpenKey(HKEY_CURRENT_USER,
                    REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM,
                    &hKey) == ERROR_SUCCESS)) {

        BOOL fDisableCPL = CheckRestriction(hKey,REGSTR_VAL_DISPCPL_NODISPCPL);

        if (fDisableCPL) {

            TCHAR szMessage[255],szTitle[255];

            RegCloseKey(hKey);
            LoadString( hInst, IDS_DISPLAY_DISABLED, szMessage, ARRAYSIZE(szMessage) );
            LoadString( hInst, IDS_DISPLAY_TITLE, szTitle, ARRAYSIZE(szTitle) );

            MessageBox( hwndParent, szMessage, szTitle, MB_OK | MB_ICONINFORMATION );

            return;
        }
    }
    else
    {
        hKey = NULL;
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

#ifndef WINNT
    // check for special command line casess
    if (psh.nStartPage == -1)
    {
        hDesk16 = LoadLibrary16( "DeskCp16.Dll" );
        pDesk16 = (FARPROC16)( hDesk16? GetProcAddress16( hDesk16, "CplApplet" ) : NULL );

        // USER couldn't start the display and needs our help
        // kick up the fallback settings page...

        gbExecMode    = EXEC_INVALID_MODE;
        gbInvalidMode = EXEC_INVALID_DISPLAY_MODE;

        //
        // call deskcp16.dll and let it try to diagnose the display
        // problem, it will try to figure out what is wrong and
        // either send the user to a HLP file, or add the Settings
        // page.  if it adds a Settings page, we want to remove it
        // and add ours
        //
        if (pDesk16 && CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_INIT, 0, 0))
        {
            psh.nStartPage = 0;
        
            // Have our 16-bit dll add the fallback page
            SHAddPages16( NULL, "DESKCP16.DLL,GetDisplayFallbackPage",
                          _AddDisplayPropSheetPage, (LPARAM)&psh );

            //
            // if the 16bit code did not add a page, we should not show ours
            //
            if (psh.nPages == 0)
            {
                return;
            }

            //
            // nuke the pages that deskcp16 added
            //
            while (psh.nPages > 0)
            {
                psh.nPages--;
                DestroyPropertySheetPage(psh.phpage[psh.nPages]);
                psh.phpage[psh.nPages] = NULL;
            }
        }
    }
#endif

    // GetSystemMetics(SM_CMONITORS) returns only the enabled monitors. So, we need to 
    // enumerate ourselves to determine if this is a multimonitor scenario. We have our own
    // function to do this.
    // Use the appropriate dlg template for multimonitor and single monitor configs.
    // if(GetSystemMetrics(SM_CMONITORS) > 1)
    if(ComputeNumberOfDisplayDevices() > 1)
        aPageInfo[IPI_SETTINGS].id = DLG_MULTIMONITOR;
    else
        aPageInfo[IPI_SETTINGS].id = DLG_SINGLEMONITOR;

    /*
     * Build the property sheet.  If we are under setup, then just include
     * the "settings" page, and no otheres
     */
    if (gbExecMode == EXEC_NORMAL)
    {
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
        // Initialize COM
        //
        if (lpfnCoInitialize) {
            lpfnCoInitialize(NULL);
        }

        if (!GetSystemMetrics(SM_CLEANBOOT))
        {
#ifdef WINNT
            hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE,
                                              REGSTR_PATH_CONTROLSFOLDER TEXT("\\Desk"), 8);
#else
            hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE,
                                              REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display"), 8);
#endif
        }

        for( i = 0; i < C_PAGES_DESK; i++ )
        {
            BOOL    fHideThisPage = FALSE;
            
            if (hKey != NULL && CheckRestriction(hKey,aPageInfo[i].szRegChkName))
            {
                //
                // This page is locked out by admin, don't put it up
                //

                fHideThisPage = TRUE;
            }

            psp.pszTemplate = MAKEINTRESOURCE(aPageInfo[i].id);
            psp.pfnDlgProc = aPageInfo[i].pfnDlgProc;
            psp.dwFlags = PSP_DEFAULT;
            psp.lParam = 0L;

#ifdef WINNT
            //
            // For Terminal Services, we need to hide the Background tab if a machine
            // policy is set to not allow wallpaper changes for this session.
            //

            if (psp.pfnDlgProc == BackgroundDlgProc &&
                  GetSystemMetrics(SM_REMOTESESSION) &&
                  !fHideThisPage )
            {
                TCHAR   szSessionName[WINSTATIONNAME_LENGTH * 2];
                TCHAR   szWallPaper[MAX_PATH*2];
                TCHAR   szBuf[MAX_PATH*2];
                TCHAR   szActualValue[MAX_PATH*2];
                DWORD   dwLen;
                DWORD   i;

                ZeroMemory((PVOID)szSessionName,sizeof(szSessionName));
                dwLen = GetEnvironmentVariable(TEXT("SESSIONNAME"),szSessionName,ARRAYSIZE(szSessionName));
                if (dwLen != 0)
                {
                    //
                    // Now that we have the session name, search for the # character.
                    //
                    for(i = 0; i < dwLen; i++) {
                        if (szSessionName[i] == TEXT('#')) {
                            szSessionName[i] = TEXT('\0');
                            break;
                        }
                    }

                    // Here is what we are looking for in NT5:
                    //  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\
                    //      WinStations\RDP-Tcp\UserOverride\Control Panel\Desktop
                    //
                    // The value is:
                    //      Wallpaper
                    //
                    lstrcpy(szWallPaper,WALLPAPER);
                    lstrcpy(szBuf, WINSTATION_REG_NAME );  
                    lstrcat(szBuf, L"\\" );
                    lstrcat(szBuf, szSessionName );
                    lstrcat(szBuf, L"\\" );
                    lstrcat(szBuf, WIN_USEROVERRIDE );
                    lstrcat(szBuf, L"\\" );
                    lstrcat(szBuf, REGSTR_PATH_DESKTOP );   // Control Panel\\Desktop

                    //
                    // See if we can get the wallpaper string.  This will fail if the key
                    // doesn't exist.  This means the policy isn't set.
                    //
                    //                                 hKey, lpszSubkey, lpszValueName, lpszDefault, 
                    if (GetStringFromReg(HKEY_LOCAL_MACHINE, szBuf,      szWallPaper,   TEXT(""),    
                                            // lpszValue,     cchSizeofValueBuff)
                                               szActualValue, ARRAYSIZE(szActualValue)))
                    {
                        fHideThisPage = TRUE;
                    }
                }
            }
#endif

            if (!fHideThisPage && (psp.pfnDlgProc == BackgroundDlgProc)) {
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

            //
            // add extensions from the registry
            // CAUTION: Do not check for "fHideThisPage" here. We need to add the pages for 
            // property sheet extensions even if the "Settings" page is hidden.
            //
            if (psp.pfnDlgProc == MultiMonitorDlgProc && hpsxa)
            {
                UINT cutoff = psh.nPages;
                UINT added = SHAddFromPropSheetExtArray( hpsxa,
                    _AddDisplayPropSheetPage, (LPARAM)&psh );

                if (psh.nStartPage >= cutoff)
                    psh.nStartPage += added;
            }

            if (!fHideThisPage && (hpsp = CreatePropertySheetPage(&psp)))
            {
                psh.phpage[psh.nPages++] = hpsp;
            }
        }

        //
        // add a fake settings page to fool OEM extensions
        //
        // !!! this page must be last !!!
        //
        if (hpsxa)
        {
            AddFakeSettingsPage(&psh);
        }
    }
    else
    {
        //
        // For the SETUP case, only the display page should show up.
        //
        psp.pszTemplate = MAKEINTRESOURCE(aPageInfo[IPI_SETTINGS].id);
        psp.pfnDlgProc = aPageInfo[IPI_SETTINGS].pfnDlgProc;

        if (hpsp = CreatePropertySheetPage(&psp)) {
            psh.phpage[psh.nPages++] = hpsp;
        }
    }
    
    if (psh.nStartPage >= psh.nPages)
        psh.nStartPage = 0;

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (psh.nPages)
    {
        SetStartPage(&psh, cmdline);

        if (PropertySheet(&psh) == ID_PSRESTARTWINDOWS)
        {
            exitparam = EWX_REBOOT;
        }
    }

    // free any loaded extensions
    if (hpsxa)
    {
        SHDestroyPropSheetExtArray(hpsxa);
    }

#ifndef WINNT
    if (pDesk16)
        CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_EXIT, 0, 0 );
   
    if( hDesk16 )
        FreeLibrary16( hDesk16 );
#endif
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
    
    if (exitparam == EWX_REBOOT)
        RestartDialog( hwndParent, NULL, exitparam );

    return;
}




DWORD gdwCoverStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
#if 0 // This code creates another thread for the cover window, which we don't really need
      // and is causing a problem. I am not sure if this is needed for NT, so leave it here....

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
        sc_szCoverClass, g_szNULL, WS_POPUP | WS_VISIBLE | pcwp->flags, 0, 0,
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

void DestroyCoverWindow(HWND hwndCover)
{
    if (hwndCover)
        PostMessage(hwndCover, WM_CLOSE, 0, 0L);
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

#endif

void DestroyCoverWindow(HWND hwndCover)
{
    DestroyWindow(hwndCover);
}

HWND FAR PASCAL CreateCoverWindow( DWORD flags )
{
    HWND hwndCover = CreateWindowEx( gdwCoverStyle,
                                     sc_szCoverClass, g_szNULL, WS_POPUP | WS_VISIBLE | flags, 
                                     GetSystemMetrics( SM_XVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_YVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_CXVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_CYVIRTUALSCREEN ),
                                     NULL, NULL, hInstance, NULL );
    if( hwndCover )
    {
        SetForegroundWindow( hwndCover );
        if (flags & COVER_NOPAINT)
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        UpdateWindow( hwndCover);
    }

    return hwndCover;
}

///////////////////////////////////////////////////////////////////////////////
// CoverWndProc (see CreateCoverWindow)
///////////////////////////////////////////////////////////////////////////////

#define WM_PRIV_KILL_LATER  (WM_APP + 100)  //Private message to kill ourselves later.

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
                DestroyWindow(window);
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


        // We post a private message to ourselves because:
        // When WM_CLOSE is processed by this window, it calls DestroyWindow() which results in
        // WM_ACTIVATE (WA_INACTIVE) message to be sent to this window. If this code calls
        // DestroyWindow again, it causes a loop. So, instead of calling DestroyWindow immediately,
        // we post ourselves a message and destroy us letter.
        case WM_ACTIVATE:
            if( GET_WM_ACTIVATE_STATE( wparam, lparam ) == WA_INACTIVE )
            {
                PostMessage( window, WM_PRIV_KILL_LATER, 0L, 0L );
                return 1L;
            }
            break;

        case WM_PRIV_KILL_LATER:
            DestroyWindow(window);
            break;

        case WM_DESTROY:
            KillTimer(window, ID_CVRWND_TIMER);
            break;
    }

    return DefWindowProc( window, message, wparam, lparam );
}

BOOL _FindCoverWindowCallback(HWND hwnd, LPARAM lParam)
{
    TCHAR szClass[MAX_PATH];
    HWND *phwnd = (HWND*)lParam;

    if( !GetClassName(hwnd, szClass, ARRAYSIZE(szClass)) )
        return TRUE;

    if( lstrcmp(szClass, sc_szCoverClass) == 0 )
    {
        if( phwnd )
            *phwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}


LONG APIENTRY CPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;
    HWND hwndCover;

    switch (message)
    {
      case CPL_INIT:          // Is any one there ?

        /*
         * Init the common controls
         */
        if (!DeskInitCpl())
            return 0;


        /*
         * Load ONE string for emergencies.
         */

        LoadString (hInstance, IDS_DISPLAY_TITLE, gszDeskCaption, ARRAYSIZE(gszDeskCaption));

        return !0;

      case CPL_GETCOUNT:        // How many applets do you support ?
        return 1;

      case CPL_INQUIRE:         // Fill CplInfo structure
        lpCPlInfo = (LPCPLINFO)lParam;

        lpCPlInfo->idIcon = IDI_DISPLAY;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
        break;

    case CPL_NEWINQUIRE:

        lpNCPlInfo = (LPNEWCPLINFO)lParam;

        lpNCPlInfo->hIcon = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(IDI_DISPLAY));
        LoadString(hInstance, IDS_NAME, lpNCPlInfo->szName, ARRAYSIZE(lpNCPlInfo->szName));

        if (!LoadString(hInstance, IDS_INFO, lpNCPlInfo->szInfo, ARRAYSIZE(lpNCPlInfo->szInfo)))
            lpNCPlInfo->szInfo[0] = (TCHAR) 0;

        lpNCPlInfo->dwSize = sizeof( NEWCPLINFO );
        lpNCPlInfo->lData  = 0;
#if 0
        lpNCPlInfo->dwHelpContext = IDH_CHILD_DISPLAY;
        lstrcpy(lpNCPlInfo->szHelpFile, xszControlHlp);
#else
        lpNCPlInfo->dwHelpContext = 0;
        lstrcpy(lpNCPlInfo->szHelpFile, TEXT(""));
#endif

        return TRUE;

      case CPL_DBLCLK:          // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        lParam = 0L;
        // fall through...

      case CPL_STARTWPARMS:
        DeskShowPropSheet( hInstance, hwnd, (LPTSTR)lParam );

        // ensure that any cover windows we've created have been destroyed
        do
        {
            hwndCover = 0;
            EnumWindows( _FindCoverWindowCallback, (LPARAM)&hwndCover );
            if( hwndCover )
            {
                DestroyWindow( hwndCover );
            }
        }
        while( hwndCover );

        return TRUE;            // Tell RunDLL.exe that I succeeded

      case CPL_EXIT:            // You must really die
      case CPL_STOP:            // You must die
        break;

      case CPL_SELECT:          // You have been selected
        /*
         * Sent once for each applet prior to the CPL_EXIT msg.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        break;

      //
      //  Private message sent when this applet is running under "Setup"
      //
      case CPL_SETUP:

        gbExecMode = EXEC_SETUP;
        break;

      //
      // Private message used by userenv.dll to refresh the display colors
      //
      case CPL_POLICYREFRESH:
        RefreshColors ();
        break;
        
    }

    return 0L;
}
