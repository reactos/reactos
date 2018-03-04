/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Dialog for editing a glyph
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static VOID
AddToolboxButton(IN HWND hToolbar, IN INT iBitmap, IN INT idCommand, IN BYTE fsState)
{
    TBBUTTON tbb = {0,};

    tbb.fsState = fsState;
    tbb.fsStyle = BTNS_CHECKGROUP;
    tbb.iBitmap = iBitmap;
    tbb.idCommand = idCommand;

    SendMessageW( hToolbar, TB_ADDBUTTONSW, 1, (LPARAM)&tbb );
}

static VOID
InitToolbox(IN PEDIT_GLYPH_INFO Info)
{
    HWND hToolbar;
    INT iBitmap;
    TBADDBITMAP tbab;

    hToolbar = GetDlgItem(Info->hSelf, IDC_EDIT_GLYPH_TOOLBOX);

    // Identify the used Common Controls version
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Set the button size to 24x24
    SendMessageW( hToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(24, 24) );
    SendMessageW( hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(24, 24) );

    // Add the Toolbox bitmaps
    tbab.hInst = hInstance;
    tbab.nID = IDB_EDIT_GLYPH_TOOLBOX;
    iBitmap = (INT)SendMessageW(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

    AddToolboxButton(hToolbar, iBitmap + TOOLBOX_PEN, ID_TOOLBOX_PEN, TBSTATE_ENABLED | TBSTATE_CHECKED);
}

static VOID
GetBitRect(IN PEDIT_GLYPH_INFO Info, IN UINT uRow, IN UINT uColumn, OUT LPRECT BitRect)
{
    BitRect->left = uColumn * Info->lEditSpacing + 1;
    BitRect->top = uRow * Info->lEditSpacing + 1;
    BitRect->right = BitRect->left + Info->lEditSpacing - 1;
    BitRect->bottom = BitRect->top + Info->lEditSpacing - 1;
}

static VOID
SetPixelBit(IN PEDIT_GLYPH_INFO Info, IN UINT uRow, IN UINT uColumn, IN BOOL uBit)
{
    // Set the bit in the bitfield
    if(uBit)
        Info->CharacterBits[uRow] |= 1 << (7 - uColumn);
    else
        Info->CharacterBits[uRow] &= ~( 1 << (7 - uColumn) );

    // Redraw everything
    InvalidateRect(Info->hEdit, NULL, FALSE);
    InvalidateRect(Info->hPreview, NULL, FALSE);
}

static BOOL
EditGlyphCommand(IN INT idCommand, IN PEDIT_GLYPH_INFO Info)
{
    switch(idCommand)
    {
        case IDOK:
        {
            RECT rect;
            UINT uColumn;
            UINT uRow;

            RtlCopyMemory( Info->FontWndInfo->Font->Bits + Info->uCharacter * 8, Info->CharacterBits, sizeof(Info->CharacterBits) );

            GetCharacterPosition(Info->uCharacter, &uRow, &uColumn);
            GetCharacterRect(uRow, uColumn, &rect);
            InvalidateRect(Info->FontWndInfo->hFontBoxesWnd, &rect, FALSE);

            Info->FontWndInfo->OpenInfo->bModified = TRUE;

            // Fall through
        }

        // This is the equivalent of WM_DESTROY for dialogs
        case IDCANCEL:
            EndDialog(Info->hSelf, 0);

            // Remove the window from the linked list
            if(Info->PrevEditGlyphWnd)
                Info->PrevEditGlyphWnd->NextEditGlyphWnd = Info->NextEditGlyphWnd;
            else
                Info->FontWndInfo->FirstEditGlyphWnd = Info->NextEditGlyphWnd;

            if(Info->NextEditGlyphWnd)
                Info->NextEditGlyphWnd->PrevEditGlyphWnd = Info->PrevEditGlyphWnd;
            else
                Info->FontWndInfo->LastEditGlyphWnd = Info->PrevEditGlyphWnd;

            SetWindowLongPtrW(Info->hSelf, GWLP_USERDATA, 0);
            SetWindowLongPtrW(Info->hEdit, GWLP_USERDATA, 0);
            SetWindowLongPtrW(Info->hPreview, GWLP_USERDATA, 0 );

            HeapFree(hProcessHeap, 0, Info);
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK
EditGlyphDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PEDIT_GLYPH_INFO Info;

    Info = (PEDIT_GLYPH_INFO) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if(Info || uMsg == WM_INITDIALOG)
    {
        switch(uMsg)
        {
            case WM_COMMAND:
                return EditGlyphCommand( LOWORD(wParam), Info );

            case WM_INITDIALOG:
                Info = (PEDIT_GLYPH_INFO) lParam;
                Info->hSelf = hwnd;
                Info->hEdit = GetDlgItem(hwnd, IDC_EDIT_GLYPH_EDIT);
                Info->hPreview = GetDlgItem(hwnd, IDC_EDIT_GLYPH_PREVIEW);

                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)Info);
                SetWindowLongPtrW(Info->hEdit, GWLP_USERDATA, (LONG_PTR)Info);
                SetWindowLongPtrW(Info->hPreview, GWLP_USERDATA, (LONG_PTR)Info);

                InitToolbox(Info);

                return TRUE;
        }
    }

    return FALSE;
}

static LRESULT CALLBACK
EditGlyphEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PEDIT_GLYPH_INFO Info;

    Info = (PEDIT_GLYPH_INFO) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if(Info)
    {
        switch(uMsg)
        {
            case WM_CREATE:
                return 0;

            case WM_LBUTTONDOWN:
                SetPixelBit(Info, GET_Y_LPARAM(lParam) / Info->lEditSpacing, GET_X_LPARAM(lParam) / Info->lEditSpacing, 1);
                return 0;

            case WM_RBUTTONDOWN:
                SetPixelBit(Info, GET_Y_LPARAM(lParam) / Info->lEditSpacing, GET_X_LPARAM(lParam) / Info->lEditSpacing, 0);
                return 0;

            case WM_PAINT:
            {
                BOOL bBit;
                HPEN hOldPen;
                HPEN hPen;
                PAINTSTRUCT ps;
                RECT rect;
                UINT i;
                UINT j;

                BeginPaint(hwnd, &ps);

                // Draw the grid
                GetClientRect(hwnd, &rect);
                Info->lEditSpacing = rect.right / 8;

                hPen = CreatePen( PS_SOLID, 1, RGB(128, 128, 128) );
                hOldPen = SelectObject(ps.hdc, hPen);

                for(i = 1; i < 8; i++)
                {
                    MoveToEx(ps.hdc, i * Info->lEditSpacing, 0, NULL);
                    LineTo  (ps.hdc, i * Info->lEditSpacing, rect.right);

                    MoveToEx(ps.hdc, 0, i * Info->lEditSpacing, NULL);
                    LineTo  (ps.hdc, rect.right, i * Info->lEditSpacing);
                }

                SelectObject(ps.hdc, hOldPen);
                DeleteObject(hPen);

                // Draw all bits
                for(i = 0; i < 8; i++)
                {
                    for(j = 0; j < 8; j++)
                    {
                        bBit = (BOOL) (Info->CharacterBits[i] << j & 0x80);

                        GetBitRect(Info, i, j, &rect);
                        FillRect( ps.hdc, &rect, (HBRUSH) GetStockObject(bBit ? BLACK_BRUSH : WHITE_BRUSH) );
                    }
                }

                // Draw the bounding rectangle
                SelectObject( ps.hdc, GetStockObject(NULL_BRUSH) );
                Rectangle(ps.hdc, 0, 0, rect.right, rect.right);

                EndPaint(hwnd, &ps);
                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK
EditGlyphPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PEDIT_GLYPH_INFO Info;

    Info = (PEDIT_GLYPH_INFO) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if(Info)
    {
        switch(uMsg)
        {
            case WM_CREATE:
                return 0;

            case WM_PAINT:
            {
                BOOL bBit;
                INT iLeft;
                INT iTop;
                PAINTSTRUCT ps;
                RECT rect;
                UINT i;
                UINT j;

                BeginPaint(hwnd, &ps);

                // Draw the bounding rectangle
                GetClientRect(hwnd, &rect);
                Rectangle(ps.hdc, 0, 0, rect.right, rect.bottom);

                // Draw all bits
                iLeft = rect.right / 2 - 8 / 2;
                iTop = rect.bottom / 2 - 8 / 2;

                for(i = 0; i < 8; i++)
                {
                    for(j = 0; j < 8; j++)
                    {
                        bBit = (BOOL) (Info->CharacterBits[i] << j & 0x80);
                        SetPixel( ps.hdc, j + iLeft, i + iTop, (bBit ? 0 : 0xFFFFFF) );
                    }
                }

                EndPaint(hwnd, &ps);

                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL
InitEditGlyphWndClasses(VOID)
{
    WNDCLASSW wc = {0,};

    wc.lpfnWndProc    = EditGlyphEditWndProc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.lpszClassName  = EDIT_GLYPH_EDIT_CLASSW;

    if( !RegisterClassW(&wc) )
        return FALSE;

    wc.lpfnWndProc    = EditGlyphPreviewWndProc;
    wc.lpszClassName  = EDIT_GLYPH_PREVIEW_CLASSW;

    return RegisterClassW(&wc) != 0;
}

VOID
UnInitEditGlyphWndClasses(VOID)
{
    UnregisterClassW(EDIT_GLYPH_EDIT_CLASSW, hInstance);
    UnregisterClassW(EDIT_GLYPH_PREVIEW_CLASSW, hInstance);
}
