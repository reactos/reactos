///////////////////////////////////////////////////////////////////////////////
//
// DISPLAY.C
//
// Creates a Property Sheet for the user's display
//
///////////////////////////////////////////////////////////////////////////////

#include "desk.h"
#include "rc.h"
#include "applet.h"
#include <regstr.h>
#include <cplext.h>

extern HINSTANCE g_hInst;       // in desk.c

#define MAX_PAGES 24

///////////////////////////////////////////////////////////////////////////////
// location of prop sheet hookers in the registry
///////////////////////////////////////////////////////////////////////////////

#pragma data_seg(".text")
static const char sc_szRegDisplay[] = REGSTR_PATH_CONTROLSFOLDER "\\Display";
static const char sc_szRegDisplayHandlers[] = REGSTR_PATH_CONTROLSFOLDER "\\Display" "\\shellex\\PropertySheetHandlers";
const char szRestrictionKey[] = REGSTR_PATH_POLICIES "\\" REGSTR_KEY_SYSTEM;
const char szNoBackgroundPage[] = REGSTR_VAL_DISPCPL_NOBACKGROUNDPAGE;
const char szNoSaverPage[] = REGSTR_VAL_DISPCPL_NOSCRSAVPAGE;
const char szNoAppearancePage[] = REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE;
const char szNoSettingsPage[] = REGSTR_VAL_DISPCPL_NOSETTINGSPAGE;
const char szNoDispCPL[] = REGSTR_VAL_DISPCPL_NODISPCPL;
#pragma data_seg()


static int
GetClInt( const char *p )
{
    BOOL neg = FALSE;
    int v = 0;

    while( *p == ' ' )
        p++;                        // skip spaces

    if( *p == '-' )                 // is it negative?
    {
        neg = TRUE;                     // yes, remember that
        p++;                            // skip '-' char
    }

    // parse the absolute portion
    while( ( *p >= '0' ) && ( *p <= '9' ) )     // digits only
        v = v * 10 + *p++ - '0';    // accumulate the value

    return ( neg? -v : v );         // return the result
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


///////////////////////////////////////////////////////////////////////////////
// DisplayApplet
///////////////////////////////////////////////////////////////////////////////

typedef HRESULT (*LPFNCOINIT)(LPVOID);
typedef HRESULT (*LPFNCOUNINIT)(void);

int
DisplayApplet( HINSTANCE instance, HWND parent, LPCSTR cmdline )
{
    HINSTANCE hDesk16 = LoadLibrary16( "DeskCp16.Dll" );
    FARPROC16 pDesk16 = (FARPROC16)( hDesk16?
        GetProcAddress16( hDesk16, "CplApplet" ) : NULL );
	HKEY hKey;
    BOOL fHideBackgroundPage=FALSE,fHideSaverPage=FALSE;
    BOOL fHideAppearancePage=FALSE,fHideSettingsPage=FALSE;
    HINSTANCE hinstOLE32 = NULL;
    LPFNCOINIT lpfnCoInitialize = NULL;
    LPFNCOUNINIT lpfnCoUninitialize = NULL;

    int result = 0;

    // look in the registry to see if particular pages have been
    // restricted by admin
    if (RegOpenKey(HKEY_CURRENT_USER,szRestrictionKey,&hKey) == ERROR_SUCCESS) {
        fHideBackgroundPage = CheckRestriction(hKey,szNoBackgroundPage);		
        fHideSaverPage = CheckRestriction(hKey,szNoSaverPage);		
        fHideAppearancePage = CheckRestriction(hKey,szNoAppearancePage);		
        fHideSettingsPage = CheckRestriction(hKey,szNoSettingsPage);		
        RegCloseKey(hKey);
    }

    if( pDesk16 && CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_INIT, 0, 0 ) )
    {
        PROPSHEETHEADER psh;
        HPROPSHEETPAGE rPages[MAX_PAGES];
        HPSXA hpsxa = NULL;
    
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_PROPTITLE;
        psh.hwndParent = parent;
        psh.hInstance = instance;
        psh.pszCaption = MAKEINTRESOURCE( IDS_DISPLAY_TITLE );
        psh.nPages = 0;
        psh.nStartPage = ( ( cmdline && *cmdline )? GetClInt( cmdline ) : 0 );
        psh.phpage = rPages;

        // check for special command line casess
        if( psh.nStartPage == -1 )
        {
            // USER couldn't start the display and needs our help
            // kick up the fallback settings page...
            psh.nStartPage = 0;

            // Have our 16-bit dll add the fallback page
            SHAddPages16( NULL, "DESKCP16.DLL,GetDisplayFallbackPage",
                _AddDisplayPropSheetPage, (LPARAM)&psh );
        }
        else
        {
            UINT nPagesOld;
            UINT nAdded;

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

            // go find any extensions
            hpsxa = SHCreatePropSheetExtArray( HKEY_LOCAL_MACHINE,
                    sc_szRegDisplay, 8 );

            // Background page
            if (!fHideBackgroundPage)
            {
                nPagesOld = psh.nPages;
                nAdded = 0;

                // check for replacement pages
                if( hpsxa != NULL )
                {
                    HPSXA hpsxaReplace = CreateReplaceableHPSXA(hpsxa);
                    if (hpsxaReplace)
                    {
                        nAdded = SHReplaceFromPropSheetExtArray( hpsxaReplace, CPLPAGE_DISPLAY_BACKGROUND,
                            _AddDisplayPropSheetPage, (LPARAM)&psh );
                        DestroyReplaceableHPSXA(hpsxaReplace);
                    }
                }

                // or just add the default page
                if (!nAdded)
                {
                    SHAddPages16( NULL, "DESKCP16.DLL,GetDisplayBackgroundPage",
                        _AddDisplayPropSheetPage, (LPARAM)&psh );
                    nAdded = 1;
                }

                if ((nAdded > 1) && (psh.nStartPage > nPagesOld))
                {
                    psh.nStartPage += nAdded - 1;
                }
            }
            else if (psh.nStartPage >= psh.nPages)
            {
                if (psh.nStartPage == psh.nPages)
                    psh.nStartPage = 0;
                else
                    psh.nStartPage--;
            }

            // Have our 16-bit dll add the screen saver page
            if (!fHideSaverPage)
            {        
                SHAddPages16( NULL, "DESKCP16.DLL,GetDisplaySaverPage",
                    _AddDisplayPropSheetPage, (LPARAM)&psh );
            }
            else if (psh.nStartPage >= psh.nPages)
            {
                if (psh.nStartPage == psh.nPages)
                    psh.nStartPage = 0;
                else
                    psh.nStartPage--;
            }

            // Have our 16-bit dll add the appearance page
            if (!fHideAppearancePage)
            {
                SHAddPages16( NULL, "DESKCP16.DLL,GetDisplayAppearancePage",
                    _AddDisplayPropSheetPage, (LPARAM)&psh );
            }
            else if (psh.nStartPage >= psh.nPages)
            {
                if (psh.nStartPage == psh.nPages)
                    psh.nStartPage = 0;
                else
                    psh.nStartPage--;
            }

            // Load any extensions that are installed
            if( hpsxa != NULL )
            {
                nPagesOld = psh.nPages;
                nAdded = SHAddFromPropSheetExtArray( hpsxa,
                    _AddDisplayPropSheetPage, (LPARAM)&psh );

                if (psh.nStartPage >= nPagesOld)
                    psh.nStartPage += nAdded;
            }

            // Have our 16-bit dll add the settings page, etc
    	    if (!fHideSettingsPage)
            {
                // and add the real settings page
                SHAddPages16( NULL, "DESKCP16.DLL,GetDisplaySettingsPage",
                    _AddDisplayPropSheetPage, (LPARAM)&psh );
            }
        }

        // fallback will sometimes do other work and return no pages
        if( psh.nPages )
        {
            SetStartPage(&psh, cmdline);

            // Bring the sucker up...
            switch( PropertySheet( &psh ) )
            {
                case ID_PSRESTARTWINDOWS:
                    result = APPLET_RESTART;
                    break;

                case ID_PSREBOOTSYSTEM:
                    result = APPLET_REBOOT;
                    break;
            }

            if (psh.dwFlags & PSH_USEPSTARTPAGE)
            {
                LocalFree((HANDLE)psh.pStartPage);
            }
        }

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

        CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_EXIT, 0, 0 );
    }

    if( hDesk16 )
        FreeLibrary16( hDesk16 );

    return result;
}

//  Checks the given restriction.  Returns TRUE (restricted) if the
//  specified key/value exists and is non-zero, false otherwise
BOOL CheckRestriction(HKEY hKey,LPCSTR lpszValueName)
{
    DWORD dwData,dwSize=sizeof(dwData);
    if ( (RegQueryValueEx(hKey,lpszValueName,NULL,NULL,
        (BYTE *) &dwData,&dwSize) == ERROR_SUCCESS) &&
        dwData)
        return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// UnhookPropSheet
//
// the ATI display pages patch the snot out off the Settings page
// the problem is that we have changed the setting page, and
// even if the ATI code could deal with our changes, the
// settings page is not even on the same property sheet
// as the extensions.
//
// they hook the setting page by hooking the TabContol and
// watching for a mouse click to activate the Settings page
// then they modify the page.  NOTE they dont patch the page
// if it is activated via the keyboard only the mouse!
//
// what this callback does is look for someone subclassing the
// tab control, if we detect this we put the correct WndProc
// back.
//
///////////////////////////////////////////////////////////////////////////////
int CALLBACK UnhookPropSheet(HWND hDlg, UINT code, LPARAM lParam)
{
    HWND hwnd;

    switch (code)
    {
        case PSCB_INITIALIZED:
            if (hwnd = PropSheet_GetTabControl(hDlg))
            {
#ifdef DEBUG
                if ((LONG)GetClassLong(hwnd, GCL_WNDPROC) !=
                    (LONG)GetWindowLong(hwnd, GWL_WNDPROC))
                {
                    OutputDebugString("**** DESK.CPL: TabControl is sublassed!\r\n");
                }
#endif
                SetWindowLong(hwnd, GWL_WNDPROC, GetClassLong(hwnd, GCL_WNDPROC));
            }
            break;

        case PSCB_PRECREATE:
            break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// DoAdvancedProperties
//
// called by DESKCP16.DLL to display the Advanced settings page
//
//      hDlg            - parent of the propsheet
//      szCaption       - caption for the prop sheet
//      DevNode         - devnode of the display device
//      szDeviceRegPath - registry path for device specific pages
//
///////////////////////////////////////////////////////////////////////////////
int _stdcall DoAdvancedProperties(
    HWND  hDlg,
    LPSTR szCaption,
    DWORD DevNode,
    LPSTR szDeviceRegPath)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAX_PAGES];
    int iResult = 0;
    HPSXA hpsxa = NULL;
    HPSXA hpsxaDevice = NULL;

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USECALLBACK;
    psh.hwndParent = hDlg;
    psh.hInstance = g_hInst;
    psh.pszCaption = szCaption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;
    psh.pfnCallback = UnhookPropSheet;

    SHAddPages16(NULL, "DESKCP16.DLL,GetAdapterPage",
        _AddDisplayPropSheetPage, (LPARAM)&psh);

    SHAddPages16(NULL, "DESKCP16.DLL,GetMonitorPage",
        _AddDisplayPropSheetPage, (LPARAM)&psh);

    // Load any device extensions
    if( szDeviceRegPath && (hpsxaDevice = SHCreatePropSheetExtArray(
        HKEY_LOCAL_MACHINE, szDeviceRegPath, 8)) != NULL)
    {
        SHAddFromPropSheetExtArray(hpsxaDevice, _AddDisplayPropSheetPage, (LPARAM)&psh);
    }

    SHAddPages16(NULL, "DESKCP16.DLL,GetPerformancePage",
        _AddDisplayPropSheetPage, (LPARAM)&psh);

    if (psh.nPages)
        iResult = PropertySheet(&psh);

    // free any loaded extensions
    if (hpsxa)
        SHDestroyPropSheetExtArray(hpsxa);

    if (hpsxaDevice)
        SHDestroyPropSheetExtArray(hpsxaDevice);

    return iResult;
}
