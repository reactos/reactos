//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdllogvw.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    25 Mar 97   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <wininet.h>
#include "cdlids.h"
#include "wininet.h"

#define URL_SEARCH_PATTERN             "?CodeDownloadErrorLog"
#define DELIMITER_CHAR                 '!'
#define MAX_CACHE_ENTRY_INFO_SIZE      2048

LRESULT CALLBACK DlgProc( HWND, UINT, WPARAM, LPARAM );
void ViewLogEntry( HWND hwnd );
void RefreshLogView( HWND hwnd );
void DeleteLogEntry( HWND hwnd );

HINSTANCE hInst;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow )
{
    hInst = hInstance;
    DialogBox( hInstance, MAKEINTRESOURCE(IDD_CDLLOGVIEW), NULL, DlgProc );

    return( 0 );
}

LRESULT CALLBACK DlgProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    switch( iMsg ) {
        case WM_INITDIALOG:
            RefreshLogView( hwnd );
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD( wParam ) ) {
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;
                case IDC_CB_VIEWLOG:
                    ViewLogEntry( hwnd );
                    break;
                case IDC_CB_REFRESH:
                    RefreshLogView( hwnd );
                    break;
                case IDC_CB_DELETE:
                    DeleteLogEntry( hwnd );
                    break;
                case IDC_LB_LOGMESSAGES:
                    switch( HIWORD( wParam ) ) {
                        case LBN_DBLCLK:
                            ViewLogEntry( hwnd );
                            break;
                    }                                                

            }
            return( TRUE );
    }

    return( FALSE );
}

void DeleteLogEntry( HWND hwnd )
{
    static char                     szUrlBuffer[INTERNET_MAX_URL_LENGTH];
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    int                             iIndex = 0;
    int                             iLength = 0;

    iIndex = SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                                 LB_GETCURSEL, 0, 0 );
    iLength = SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                                  LB_GETTEXTLEN, iIndex, 0 );                                                        
    SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                        LB_GETTEXT, iIndex, (LPARAM)szUrl );
    
    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    wsprintf( szUrlBuffer, "%s%c%s", URL_SEARCH_PATTERN, DELIMITER_CHAR, szUrl );
    if( DeleteUrlCacheEntry( szUrlBuffer ) ) {
        RefreshLogView( hwnd );
    } else {
        MessageBox( hwnd, "Error: Unable to delete cache file!",
                    "Log View Error", MB_OK | MB_ICONERROR );
    }
}

void ViewLogEntry( HWND hwnd )
{
    int                             iIndex = 0;
    int                             iLength = 0;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    static char                     szUrlBuffer[INTERNET_MAX_URL_LENGTH];

    iIndex = SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                                 LB_GETCURSEL, 0, 0 );
    iLength = SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                                  LB_GETTEXTLEN, iIndex, 0 );                                                        
    SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES,
                        LB_GETTEXT, iIndex, (LPARAM)szUrl );
    
    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    wsprintf( szUrlBuffer, "%s%c%s", URL_SEARCH_PATTERN, DELIMITER_CHAR, szUrl );
    if( GetUrlCacheEntryInfo( szUrlBuffer, pCacheEntryInfo, &dwBufferSize ) ) {
        if( pCacheEntryInfo->lpszLocalFileName != NULL ) {
            if( ShellExecute( NULL, "open",  pCacheEntryInfo->lpszLocalFileName,
                              NULL, NULL, SW_SHOWNORMAL ) <= (HINSTANCE)32 ) {
                // ShellExecute returns <= 32 if error occured
                MessageBox( hwnd, "Error: Unable to open cache file!",
                            "Log View Error", MB_OK | MB_ICONERROR );
            }
        } else {
                MessageBox( hwnd, "Error: No file name available!",
                            "Log View Error", MB_OK | MB_ICONERROR );
        }
    }
        
}

void RefreshLogView( HWND hwnd )
{
    HANDLE                          hUrlCacheEnum;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    char                           *szPtr = NULL;

    SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES, LB_RESETCONTENT, 0, 0);
    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    hUrlCacheEnum = FindFirstUrlCacheEntry( URL_SEARCH_PATTERN,
                                            pCacheEntryInfo,
                                            &dwBufferSize );
    if( hUrlCacheEnum != NULL ) {
        if( pCacheEntryInfo->lpszSourceUrlName != NULL ) {
            if( StrStrI( pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN ) ) {
                SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES, LB_ADDSTRING, 0,
                                   (LPARAM)pCacheEntryInfo->lpszSourceUrlName );
            }
        }
        dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        while( FindNextUrlCacheEntry( hUrlCacheEnum, pCacheEntryInfo,
                                      &dwBufferSize ) ) {
            if( pCacheEntryInfo->lpszSourceUrlName != NULL ) {
                if( StrStrI( pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN ) ) {
                    szPtr = pCacheEntryInfo->lpszSourceUrlName;
                    while( *szPtr != '\0' && *szPtr != DELIMITER_CHAR ) {
                        szPtr++;
                    }
                    szPtr++;
                    if( szPtr != NULL ) {
                        SendDlgItemMessage( hwnd, IDC_LB_LOGMESSAGES, LB_ADDSTRING, 0,
                                            (LPARAM)szPtr);
                    }
                }
            }

            dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        }
    }
}

int 
_stdcall 
ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), 
                NULL, 
                pszCmdLine,
                (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;           // We never come here
}
