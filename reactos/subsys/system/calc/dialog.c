/*
 * WineCalc (dialog.c)
 *
 * Copyright 2003 James Briggs
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
#include "dialog.h"
#include "resource.h"
#include "winecalc.h"

extern HINSTANCE hInstance;

BOOL CALLBACK AboutDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch( uMsg ) {

    case WM_INITDIALOG:
        SendMessage (hDlg, WM_SETFONT, (UINT)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
              case IDOK:
                   EndDialog( hDlg, 0 );
                   return TRUE;
        }
        break;

    case WM_PAINT:
      {
        HDC hMemDC;
        HFONT hFont;
        HFONT hFontOrg;

        char c1[CALC_BUF_SIZE];
        char c2[CALC_BUF_SIZE];
        char c3[CALC_BUF_SIZE];
        char c4[CALC_BUF_SIZE];
        char c5[CALC_BUF_SIZE];

        hdc = BeginPaint( hDlg, &ps );

        hMemDC = CreateCompatibleDC( hdc );

        LoadString( hInstance, IDS_COPYRIGHT1, c1, sizeof(c1));
        LoadString( hInstance, IDS_COPYRIGHT2, c2, sizeof(c2));
        LoadString( hInstance, IDS_COPYRIGHT3, c3, sizeof(c3));
        LoadString( hInstance, IDS_COPYRIGHT4, c4, sizeof(c4));
        LoadString( hInstance, IDS_COPYRIGHT5, c5, sizeof(c5));

        hFont = GetStockObject(DEFAULT_GUI_FONT);
        hFontOrg = SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);

	TextOut(hdc, 10, 10, c1, strlen(c1));
        TextOut(hdc, 10, 35, c2, strlen(c2));
        TextOut(hdc, 10, 50, c3, strlen(c3));
        TextOut(hdc, 10, 75, c4, strlen(c4));
	TextOut(hdc, 10, 90, c5, strlen(c5));

        SelectObject(hdc, hFontOrg);

        DeleteDC( hMemDC );
        EndPaint( hDlg, &ps );

        return 0;
      }
      case WM_CLOSE:
        EndDialog( hDlg, TRUE );
        return 0;
    }
    return FALSE;
}

