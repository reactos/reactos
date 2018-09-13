#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include "resource.h"
#include "debug.h"

#include "nsc.h"

typedef struct {
    HWND hDlg;
    HWND hwndNSC;
} DLGDATA;


BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DLGDATA * pdd = (DLGDATA *)GetWindowLong(hDlg, DWL_USER);

    switch (uMessage) {
    case WM_INITDIALOG:
        SetWindowLong(hDlg, DWL_USER, lParam);
	pdd = (DLGDATA *)lParam;
	pdd->hDlg = hDlg;
	pdd->hwndNSC = GetDlgItem(hDlg, IDC_USER1);

	SetWindowLong(pdd->hwndNSC, GWL_STYLE, NSS_DROPTARGET | GetWindowLong(pdd->hwndNSC, GWL_STYLE));

	{
            NSC_SETROOT sr = {NSSR_CREATEPIDL, NULL, (LPCITEMIDLIST)CSIDL_FAVORITES, 5, NULL};
            // NSC_SETROOT sr = {NSSR_CREATEPIDL, NULL, (LPCITEMIDLIST)CSIDL_FAVORITES, 10, NULL};

	    // SetWindowLong(pdd->hwndNSC, GWL_STYLE, GetWindowLong(pdd->hwndNSC, GWL_STYLE));
	    SetWindowLong(pdd->hwndNSC, GWL_STYLE, NSS_SHOWNONFOLDERS | GetWindowLong(pdd->hwndNSC, GWL_STYLE));
	    NameSpace_SetRoot(pdd->hwndNSC, &sr);
	}
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
        case IDCANCEL:
	    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        // case PSN_SETACTIVE:
        // case PSN_APPLY:
    	    break;

	default:
	    return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR pszCmdLine, int nCmdShow)
{
    DLGDATA dd;

    NameSpace_RegisterClass(hInst);

    OleInitialize(NULL);

    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc, (LPARAM)&dd);

    OleUninitialize();

    return 0;
}


// stolen from the CRT, used to shirink our code

int _stdcall WinMainCRTStartup(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
    	     != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
    	    pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
    	    pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    ExitProcess(i);

    return i;	// We only come here when we are not the shell...
}
