/****************************************************************************
 *
 * EULA.C
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1994
 *  All rights reserved
 *
 * This module handles the EULA Dialog.
 *
 ***************************************************************************/

#include "priv.h"
#include "unixstuff.h"
#include "resource.h"

#include "mluisupp.h"

#define IDD_NEXT    0x3024      

// Global variable
extern char     g_szSourceDir[MAX_PATH];

#define UNIX_IE5_KEY "Software\\Microsoft\\Internet Explorer\\Unix\\IE5"

#define LICENSE_KEY  "Software\\Microsoft\\Internet Explorer\\Unix\\IE5\\EULA Pending"
#define ALPHAWRN_KEY "Software\\Microsoft\\Internet Explorer\\Unix\\IE5\\AlphaWarning"

/* Local Definitions */
#define SZ_NDA_FILE          "license.txt"
#define KEY_ESC              27

/* Local Variables */
WNDPROC  l_lpfnOldNDAEditProc;
BOOL     bShowMore;
HWND     hMore;
int      giLines;
LPSTR    lpszNDAText;

#ifdef BETA_WARNING
BOOL CheckAndDisplayAlphaWrnDlg(void);
#endif
BOOL CheckAndSetAlphaWrnDlg( HWND hDlg );
BOOL_PTR CALLBACK AlphaWrnDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK NDAEditSubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static  int   iAccept = 0;
static HBRUSH g_hbrBkGnd = NULL;

extern HINSTANCE        g_hinst;

/* Local Prototypes */
/************************************************************************
**
** NDASubClassWnd
**
** Subclass a window procedure
**
*/
static void NEAR PASCAL
NDASubClassWnd(HWND hWnd, WNDPROC fn, WNDPROC *lpfnOldProc)
{
    *lpfnOldProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)fn);
}

/************************************************************************
**
** NDAEditSubProc
**
** Edit box subclass proc to not select the whole contents of the multiline edit control
**
*/
LRESULT CALLBACK NDAEditSubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int  iScrollPos;

    if (msg == EM_SETSEL)
    {
        lParam = wParam = 0;
    }

    if (msg == WM_PAINT)
    {
        iScrollPos = (int)SendMessage(hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L);
        bShowMore = (int)SendMessage(hWnd, EM_LINEINDEX, iScrollPos+giLines, 0L) != -1;
        ShowWindow(hMore, bShowMore ? SW_SHOW:SW_HIDE);
    }

    return CallWindowProc(l_lpfnOldNDAEditProc, hWnd, msg, wParam, lParam);
}

char  g_szSourceDir[MAX_PATH] = ".";
int   g_bBatchmode = FALSE;

extern "C" LONG SHRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved ,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);

void FAR PASCAL FillEULA(HWND hWnd)
{
    HRESULT     hr = E_FAIL;
    char        szNDAFile[MAX_PATH];
    HANDLE      hFind;
    HANDLE      hfNDA;
    DWORD       dwLenNDAText;
    DWORD       dwBytesRead;

    HWND        hwndEdit;
    HDC         hDC;        // Temporary display context
    HFONT       hFont;      // Temporary font storage
    TEXTMETRIC  tm;         // Text metrics
    RECT        rcl;
    DWORD       type = REG_SZ;
    DWORD       len  = sizeof(szNDAFile);

    WIN32_FIND_DATAA win32_find_data;

    HKEY  hKeyUnix;

    LONG lResult = RegOpenKeyExA(
       HKEY_CURRENT_USER,
       LICENSE_KEY,
       0,
       KEY_QUERY_VALUE,
       &hKeyUnix);

    if (lResult == ERROR_SUCCESS)
    {
        // Read path for license text file from registry.
        
        lResult = SHRegQueryValueExA(
           hKeyUnix,
           "LicenseFile",
           NULL,
           (LPDWORD) &type,
           (LPBYTE)  &szNDAFile,
           (LPDWORD) &len);

        if( lResult != ERROR_SUCCESS ) return;

        RegCloseKey(hKeyUnix);
    }

    hFind = FindFirstFileA ( szNDAFile, &win32_find_data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        // Edit control can only handle 64k
        if (win32_find_data.nFileSizeLow <= 0xFFFF)  // Max 64k text
        {
            // Get size and alloc buffer
            dwLenNDAText = win32_find_data.nFileSizeLow;
            lpszNDAText = (LPSTR)LocalAlloc (LPTR, (UINT)dwLenNDAText + 1);
            if (lpszNDAText != NULL)
            {
                // Should not fail, since _dos_findfirst worked
                hfNDA = CreateFileA( szNDAFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hfNDA != INVALID_HANDLE_VALUE)
                {
                    // Read complete file
                    if ( (ReadFile(hfNDA, lpszNDAText, dwLenNDAText, &dwBytesRead, NULL)) &&
                         (dwBytesRead == dwLenNDAText)) {
                         lpszNDAText[dwBytesRead] = '\0';
                         hr = S_OK;
                    }

                    CloseHandle(hfNDA);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hwndEdit=GetDlgItem(hWnd, IDC_EULA_TEXT);
        NDASubClassWnd(hwndEdit, NDAEditSubProc, &l_lpfnOldNDAEditProc);
        SetWindowTextA( hwndEdit, lpszNDAText);

        bShowMore = TRUE;
        iAccept = -1;

        // Calculate the number of lines visible in the edit control
        SendDlgItemMessage(hWnd, IDC_EULA_TEXT, EM_GETRECT, 0, (DWORD)((LPRECT)&rcl));
        hDC = GetDC(hWnd);
        // Get font currently selected into the control
        hFont = (HFONT)(WORD)SendMessage(GetDlgItem(hWnd, IDC_EULA_TEXT), WM_GETFONT, 0, 0L);

        // If it is not the system font, then select it into DC
        if (hFont)
            SelectObject(hDC, hFont);
        GetTextMetrics(hDC, &tm);
        giLines = (int)((rcl.bottom - rcl.top) / tm.tmHeight);

        hMore = GetDlgItem(hWnd, IDC_MORE);
        ReleaseDC(hWnd, hDC);
    }
}

char g_szIE4Inf[] = "ie4.inf";

BOOL IsLangDBCS(DWORD dwLang)
{
    char szGivenLang[8];
    char szTmp[8];

    wnsprintfA(szGivenLang, ARRAYSIZE(szGivenLang), "%04x", dwLang);

    // return TRUE if the given lang is in the list of DBCS langs from the INF; otherwise, return FALSE
    return GetPrivateProfileStringA("DBCSLanguages", szGivenLang, "", szTmp, sizeof(szTmp), g_szIE4Inf) != 0;
}


BOOL CheckAcceptLicense()
{
    HKEY hKeyUnix;

    if( iAccept == 1 ) 
    {
        LONG lResult = RegOpenKeyExA(
           HKEY_CURRENT_USER,
           UNIX_IE5_KEY,
           0,
           KEY_QUERY_VALUE,
           &hKeyUnix);

        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegDeleteKeyA(hKeyUnix, "EULA Pending" );
            RegCloseKey( hKeyUnix );
            if( lResult == ERROR_SUCCESS ) 
            {
              // Try to import Netscape bookmarks - disable as we have 
              // imp/Exp wizard now.
              // ImportBookmarksStartup(g_hinst); 
              return TRUE;
            }
        }

    }
    return FALSE;
}

/****************************************************************************
 *
 * EulaDlgProc()
 *
 * Dialog proc for handling the COA screen
 *
 ***************************************************************************/
 
BOOL_PTR CALLBACK EulaDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR * lpnm;
    switch( msg )
    {
        case WM_INITDIALOG:
        {
            char    szTmp[MAX_PATH];
            DWORD   dwLangSetup;
            DWORD   dwTmp;

            g_hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillEULA(hDlg);

            break;    // Let windows set the focus to default control.
        }
            
        case WM_DESTROY:
            if (lpszNDAText)
                LocalFree(lpszNDAText);
            if (g_hbrBkGnd)
    	        DeleteObject(g_hbrBkGnd);
            g_hbrBkGnd = NULL;
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            break;

        case WM_COMMAND:
            switch( wParam )
            {
                case IDC_ACCEPT:
                    iAccept = 1;
                    SendDlgItemMessage (hDlg, IDC_DONT_ACCEPT, BM_SETCHECK, !iAccept, 0L);
                    SendDlgItemMessage (hDlg, IDC_ACCEPT, BM_SETCHECK, iAccept, 0L);
		            SetDlgItemTextA(hDlg, IDOK, "A&ccept");

                    break;

                case IDC_DONT_ACCEPT:
                    iAccept = 0;
                    SendDlgItemMessage (hDlg, IDC_DONT_ACCEPT, BM_SETCHECK, !iAccept, 0L);
                    SendDlgItemMessage (hDlg, IDC_ACCEPT, BM_SETCHECK, iAccept, 0L);
		            SetDlgItemTextA(hDlg, IDOK, "Dis&miss");

                    break;

                case IDOK:
                    EndDialog( hDlg, CheckAcceptLicense() );
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE; // Let default dialog processing do all.
    }
    return TRUE;
}

extern "C"
BOOL CheckAndDisplayEULA(void)
{
    BOOL ret = TRUE;
    HKEY hKeyUnix;

    LONG lResult = RegOpenKeyExA(
       HKEY_CURRENT_USER,
       LICENSE_KEY,
       0,
       KEY_QUERY_VALUE,
       &hKeyUnix);

    if (lResult == ERROR_SUCCESS)
    {
        RegCloseKey(hKeyUnix);

        // EULA Pending key exists show the dialogbox and ask for 
        // acceptance.

        ret = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_EULA), 
                   NULL, EulaDlgProc, (LPARAM) NULL);
    }

#ifdef BETA_WARNING
    // Enabling Alpha Warning dialog as Beta warning
    if(ret)
        CheckAndDisplayAlphaWrnDlg();
#endif

    return ret;
}


#ifdef BETA_WARNING
BOOL CheckAndDisplayAlphaWrnDlg(void)
{
    HKEY  hKeyUnix;

    LONG lResult = RegOpenKeyExA(
       HKEY_CURRENT_USER,
       ALPHAWRN_KEY,
       0,
       KEY_QUERY_VALUE,
       &hKeyUnix);

    if (lResult == ERROR_SUCCESS)
    {
        RegCloseKey(hKeyUnix);

        return DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_ALPHAWRNDLG), 
                   NULL, AlphaWrnDlgProc, (LPARAM) NULL);
    }

    return TRUE;
}
#endif

BOOL CheckAndSetAlphaWrnDlg( HWND hDlg )
{
    HKEY hKeyUnix;
    //BOOL delKey = SendDlgItemMessage (hDlg, IDC_NOFUTUREDISPLAY, BM_GETCHECK, 0, 0L);
    BOOL delKey = TRUE;

    if( delKey ) 
    {
        LONG lResult = RegOpenKeyExA(
           HKEY_CURRENT_USER,
           UNIX_IE5_KEY,
           0,
           KEY_QUERY_VALUE,
           &hKeyUnix);

        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegDeleteKeyA(hKeyUnix, "AlphaWarning" );
	    RegFlushKey( hKeyUnix );
            RegCloseKey( hKeyUnix );
            if( lResult == ERROR_SUCCESS ) 
            {
              return TRUE;
            }
        }

    }
    return FALSE;

}

BOOL_PTR CALLBACK AlphaWrnDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
            SendDlgItemMessage (hDlg, IDC_NOFUTUREDISPLAY, BM_SETCHECK, 0, 0L);
            break;  
        }
            
        case WM_COMMAND:
            switch( wParam )
            {
                case IDOK:
                {
                    CheckAndSetAlphaWrnDlg( hDlg );
                    EndDialog( hDlg, 1 );
                    break;
                }

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE; // Let default dialog processing do all.
    }
    return TRUE;
}
