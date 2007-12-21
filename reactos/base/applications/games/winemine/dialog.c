/*
 * WineMine (dialog.c)
 *
 * Copyright 2000 Joshua Thielen <jt85296@ltu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <tchar.h>
#include "main.h"
#include "dialog.h"
#include "resource.h"

INT_PTR CALLBACK CustomDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BOARD *pBoard;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pBoard = (BOARD*) lParam;
            SetDlgItemInt( hDlg, IDC_EDITROWS, pBoard->uRows, FALSE );
            SetDlgItemInt( hDlg, IDC_EDITCOLS, pBoard->uCols, FALSE );
            SetDlgItemInt( hDlg, IDC_EDITMINES, pBoard->uMines, FALSE );
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDOK:
                    pBoard->uRows = GetDlgItemInt( hDlg, IDC_EDITROWS, NULL, FALSE );
                    pBoard->uCols = GetDlgItemInt( hDlg, IDC_EDITCOLS, NULL, FALSE );
                    pBoard->uMines = GetDlgItemInt( hDlg, IDC_EDITMINES, NULL, FALSE );
                    CheckLevel( pBoard );
                    /* Fall through */
                case IDCANCEL:
                    EndDialog( hDlg, LOWORD(wParam) );
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

INT_PTR CALLBACK CongratsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BOARD *pBoard;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pBoard = (BOARD*) lParam;
            SetDlgItemText( hDlg, IDC_EDITNAME, pBoard->szBestName[pBoard->Difficulty] );
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:
                    GetDlgItemText( hDlg, IDC_EDITNAME,
                        pBoard->szBestName[pBoard->Difficulty],
                        sizeof( pBoard->szBestName[pBoard->Difficulty] ) );
                    EndDialog( hDlg, 0 );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hDlg, 0 );
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK TimesDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BOARD *pBoard;
    HKEY hKey;
    UCHAR i;
    TCHAR szData[16];
    TCHAR szKeyName[8];
    TCHAR szTimes[35];
    TCHAR szSeconds[23];
    TCHAR szNobody[15];

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pBoard = (BOARD*) lParam;

            /* set best names */
            for( i = 0; i < 3; i++ )
                SetDlgItemText( hDlg, (IDC_NAME1) + i, pBoard->szBestName[i] );

            /* set best times */
            LoadString( pBoard->hInst, IDS_SECONDS, szSeconds, sizeof(szSeconds) / sizeof(TCHAR) );

            for( i = 0; i < 3; i++ )
            {
                wsprintf(szTimes, TEXT("%d %s"), pBoard->uBestTime[i], szSeconds);
                SetDlgItemText( hDlg, (IDC_TIME1) + i, szTimes );
            }

            return TRUE;

        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog( hDlg, 0 );
                    return TRUE;

                case IDRESET:
                    if( RegCreateKeyEx( HKEY_CURRENT_USER, szWineMineRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS)
                        return TRUE;

                    LoadString( pBoard->hInst, IDS_NOBODY, szNobody, sizeof(szNobody) / sizeof(TCHAR) );
                    LoadString( pBoard->hInst, IDS_SECONDS, szSeconds, sizeof(szSeconds) / sizeof(TCHAR) );

                    for (i = 0; i < 3; i++)
                    {
                        pBoard->uBestTime[i] = 999;
                        _tcscpy(pBoard->szBestName[i], szNobody);
                        wsprintf(szTimes, TEXT("%d %s"), pBoard->uBestTime[i], szSeconds);

                        SetDlgItemText( hDlg, (IDC_NAME1) + i, pBoard->szBestName[i] );
                        SetDlgItemText( hDlg, (IDC_TIME1) + i, szTimes );
                    }
                    
                    /* Write the changes to the registry
                       As we write to the same registry key as MS WinMine does, we have to start at 1 for the registry keys */
                    for( i = 0; i < 3; i++ )
                    {
                        wsprintf( szKeyName, TEXT("Name%u"), i + 1 );
                        _tcsncpy( szData, pBoard->szBestName[i], sizeof(szData) / sizeof(TCHAR) );
                        RegSetValueEx( hKey, szKeyName, 0, REG_SZ, (LPBYTE)szData, (_tcslen(szData) + 1) * sizeof(TCHAR) );
                    }

                    for( i = 0; i < 3; i++ )
                    {
                        wsprintf( szKeyName, TEXT("Time%u"), i + 1 );
                        RegSetValueEx( hKey, szKeyName, 0, REG_DWORD, (LPBYTE)&pBoard->uBestTime[i], sizeof(DWORD) );
                    }

                    RegCloseKey(hKey);
                    return TRUE;
            }
    }

    return FALSE;
}

