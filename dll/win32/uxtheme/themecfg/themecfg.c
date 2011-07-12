
#define WIN32_LEAN_AND_MEAN

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include "resource.h"

WCHAR* load_string (UINT id)
{
    WCHAR buf[1024];
    int len;
    WCHAR* newStr;

    LoadStringW (GetModuleHandle (NULL), id, buf, sizeof(buf)/sizeof(buf[0]));

    len = lstrlenW (buf);
    newStr = HeapAlloc (GetProcessHeap(), 0, (len + 1) * sizeof (WCHAR));
    memcpy (newStr, buf, len * sizeof (WCHAR));
    newStr[len] = 0;
    return newStr;
}

static INT CALLBACK
PropSheetCallback (HWND hWnd, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
	/*
	 * hWnd = NULL, lParam == dialog resource
	 */
    case PSCB_PRECREATE:
	break;

    case PSCB_INITIALIZED:
	break;

    default:
	break;
    }
    return 0;
}

INT_PTR CALLBACK
ThemeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static INT_PTR
doPropertySheet (HINSTANCE hInstance, HWND hOwner)
{
    PROPSHEETPAGEW psp[1];
    PROPSHEETHEADERW psh;
    int pg = 0; /* start with page 0 */

    psp[pg].dwSize = sizeof (PROPSHEETPAGEW);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCEW (IDD_DESKTOP_INTEGRATION);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = ThemeDlgProc;
    psp[pg].pszTitle =  load_string (IDS_TAB_DESKTOP_INTEGRATION);
    psp[pg].lParam = 0;
 
 
    /*
     * Fill out the PROPSHEETHEADER
     */
    psh.dwSize = sizeof (PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hOwner;
    psh.hInstance = hInstance;
    psh.u.pszIcon = NULL;
    psh.pszCaption =  load_string (IDS_WINECFG_TITLE);
    psh.nPages = 1;
    psh.u3.ppsp = psp;
    psh.pfnCallback = PropSheetCallback;
    psh.u2.nStartPage = 0;

	
    /*
     * Display the modal property sheet
     */
    return PropertySheetW (&psh);
}

int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrev, LPSTR szCmdLine, int nShow)
{
    InitCommonControls ();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    doPropertySheet (hInstance, NULL);
    CoUninitialize(); 
    ExitProcess (0);
    return 0;
}
