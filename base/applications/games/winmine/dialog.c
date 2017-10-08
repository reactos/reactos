/*
 * WineMine (dialog.c)
 *
 * Copyright 2000 Joshua Thielen
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "main.h"

INT_PTR CALLBACK CustomDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL IsRet;
    static BOARD *p_board;

    switch( uMsg ) {
    case WM_INITDIALOG:
        p_board = (BOARD*) lParam;
        SetDlgItemInt( hDlg, IDC_EDITROWS, p_board->rows, FALSE );
        SetDlgItemInt( hDlg, IDC_EDITCOLS, p_board->cols, FALSE );
        SetDlgItemInt( hDlg, IDC_EDITMINES, p_board->mines, FALSE );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDOK:
            p_board->rows = GetDlgItemInt( hDlg, IDC_EDITROWS, &IsRet, FALSE );
            p_board->cols = GetDlgItemInt( hDlg, IDC_EDITCOLS, &IsRet, FALSE );
            p_board->mines = GetDlgItemInt( hDlg, IDC_EDITMINES, &IsRet, FALSE );
            CheckLevel( p_board );
            EndDialog( hDlg, 0 );
            return TRUE;

        case IDCANCEL:
            EndDialog( hDlg, 1 );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK CongratsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BOARD *p_board;

    switch( uMsg ) {
    case WM_INITDIALOG:
        p_board = (BOARD*) lParam;
        SetDlgItemTextW( hDlg, IDC_EDITNAME, p_board->best_name[p_board->difficulty] );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDOK:
            GetDlgItemTextW( hDlg, IDC_EDITNAME,
                             p_board->best_name[p_board->difficulty],
                             sizeof( p_board->best_name[p_board->difficulty] ) / sizeof(WCHAR) );
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
    static BOARD *p_board;
    unsigned i;

    switch( uMsg ) {
    case WM_INITDIALOG:
        p_board = (BOARD*) lParam;

        /* set best names */
        for( i = 0; i < 3; i++ )
            SetDlgItemTextW( hDlg, (IDC_NAME1) + i, p_board->best_name[i] );

    	/* set best times */
        for( i = 0; i < 3; i++ )
            SetDlgItemInt( hDlg, (IDC_TIME1) + i, p_board->best_time[i], FALSE );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDOK:
        case IDCANCEL:
            EndDialog( hDlg, 0 );
            return TRUE;
        }
        break;
    }
    return FALSE;
}
