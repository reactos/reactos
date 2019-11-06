/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implements the window showing the character boxes for a font
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static const WCHAR szFontBoxesWndClass[] = L"VGAFontEditFontBoxesWndClass";

static VOID
DrawCharacterPixel(IN PAINTSTRUCT *ps, IN UINT uCharacter, IN UCHAR uRow, IN UCHAR uColumn, IN UCHAR uBit, IN COLORREF clBackground)
{
    INT x;
    INT y;

    x = (uCharacter % 16) * (CHARACTER_BOX_WIDTH + CHARACTER_BOX_PADDING) + 24 + uColumn;
    y = (uCharacter / 16) * (CHARACTER_BOX_HEIGHT + CHARACTER_BOX_PADDING)+ 1 + CHARACTER_INFO_BOX_HEIGHT + 2 + uRow;

    SetPixel( ps->hdc, x, y, (uBit ? 0 : clBackground) );
}

VOID
GetCharacterRect(IN UINT uFontRow, IN UINT uFontColumn, OUT LPRECT CharacterRect)
{
    CharacterRect->left = uFontColumn * (CHARACTER_BOX_WIDTH + CHARACTER_BOX_PADDING);
    CharacterRect->top = uFontRow * (CHARACTER_BOX_HEIGHT + CHARACTER_BOX_PADDING);
    CharacterRect->right = CharacterRect->left + CHARACTER_BOX_WIDTH;
    CharacterRect->bottom = CharacterRect->top + CHARACTER_BOX_HEIGHT;
}

static INT
FontBoxesHitTest(IN UINT xPos, IN UINT yPos, OUT LPRECT CharacterRect)
{
    UINT uFontColumn;
    UINT uFontRow;

    uFontColumn = xPos / (CHARACTER_BOX_WIDTH + CHARACTER_BOX_PADDING);
    uFontRow = yPos / (CHARACTER_BOX_HEIGHT + CHARACTER_BOX_PADDING);
    GetCharacterRect(uFontRow, uFontColumn, CharacterRect);

    if(xPos > (UINT)CharacterRect->right || yPos > (UINT)CharacterRect->bottom)
        // The user clicked on separator space, so return HITTEST_SEPARATOR
        return HITTEST_SEPARATOR;
    else
        // Return the character number
        return (uFontRow * 16 + uFontColumn);
}

static VOID
SetSelectedCharacter(IN PFONT_WND_INFO Info, IN UINT uNewCharacter, OPTIONAL IN LPRECT NewCharacterRect)
{
    LPRECT pCharacterRect;
    RECT OldCharacterRect;
    UINT uFontColumn;
    UINT uFontRow;

    // Remove the selection of the old character
    GetCharacterPosition(Info->uSelectedCharacter, &uFontRow, &uFontColumn);
    GetCharacterRect(uFontRow, uFontColumn, &OldCharacterRect);
    InvalidateRect(Info->hFontBoxesWnd, &OldCharacterRect, FALSE);

    // You may pass the RECT of the new character, otherwise we'll allocate memory for one and get it ourselves
    if(NewCharacterRect)
        pCharacterRect = NewCharacterRect;
    else
    {
        GetCharacterPosition(uNewCharacter, &uFontRow, &uFontColumn);
        pCharacterRect = (LPRECT) HeapAlloc( hProcessHeap, 0, sizeof(RECT) );
        GetCharacterRect(uFontRow, uFontColumn, pCharacterRect);
    }

    // Select the new character
    Info->uSelectedCharacter = uNewCharacter;
    InvalidateRect(Info->hFontBoxesWnd, pCharacterRect, FALSE);

    if(!NewCharacterRect)
        HeapFree(hProcessHeap, 0, pCharacterRect);
}

static VOID
DrawProc(IN PFONT_WND_INFO Info, IN PAINTSTRUCT* ps)
{
    COLORREF clBackground;
    HBRUSH hBrush = 0;
    HBRUSH hOldBrush = 0;
    HDC hBoxDC;
    HFONT hFont;
    HFONT hOldFont;
    RECT CharacterRect;
    UINT uFontColumn;
    UINT uStartColumn;
    UINT uEndColumn;
    UINT uFontRow;
    UINT uStartRow;
    UINT uEndRow;
    UINT uCharacter;
    UCHAR uCharacterColumn;
    UCHAR uCharacterRow;
    UCHAR uBit;
    WCHAR szInfoText[9];
    HBITMAP hBitmapOld;

    // Preparations
    hBoxDC = CreateCompatibleDC(NULL);
    hBitmapOld = SelectObject(hBoxDC, Info->MainWndInfo->hBoxBmp);

    hFont = CreateFontW(13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Tahoma");
    hOldFont = SelectObject(ps->hdc, hFont);

    SetBkMode( ps->hdc, TRANSPARENT );

    // What ranges do we have to draw?
    uStartRow = ps->rcPaint.top / (CHARACTER_BOX_HEIGHT + CHARACTER_BOX_PADDING);
    uEndRow = ps->rcPaint.bottom / (CHARACTER_BOX_HEIGHT + CHARACTER_BOX_PADDING);
    uStartColumn = ps->rcPaint.left / (CHARACTER_BOX_WIDTH + CHARACTER_BOX_PADDING);
    uEndColumn = ps->rcPaint.right / (CHARACTER_BOX_WIDTH + CHARACTER_BOX_PADDING);

    for(uFontRow = uStartRow; uFontRow <= uEndRow; uFontRow++)
    {
        for(uFontColumn = uStartColumn; uFontColumn <= uEndColumn; uFontColumn++)
        {
            GetCharacterRect(uFontRow, uFontColumn, &CharacterRect);
            uCharacter = uFontRow * 16 + uFontColumn;

            // Draw the Character Info Box (header)
            BitBlt(ps->hdc,
                   CharacterRect.left,
                   CharacterRect.top,
                   CHARACTER_BOX_WIDTH,
                   CHARACTER_INFO_BOX_HEIGHT,
                   hBoxDC,
                   0,
                   0,
                   SRCCOPY);

            // Draw the header text
            wsprintfW(szInfoText, L"%02u = %02X", uCharacter, uCharacter);
            DrawTextW( ps->hdc, szInfoText, -1, &CharacterRect, DT_CENTER );

            // Draw the Character Bitmap Box (rectangle with the actual character)
            if(Info->uSelectedCharacter == uCharacter)
            {
                clBackground = RGB(255, 255, 0);
                hBrush = CreateSolidBrush(clBackground);
                hOldBrush = SelectObject(ps->hdc, hBrush);
            }
            else
            {
                clBackground = RGB(255, 255, 255);
                SelectObject( ps->hdc, GetStockObject(WHITE_BRUSH) );
            }

            Rectangle(ps->hdc,
                      CharacterRect.left,
                      CharacterRect.top + CHARACTER_INFO_BOX_HEIGHT,
                      CharacterRect.right,
                      CharacterRect.bottom);

            // Draw the actual character into the box
            for(uCharacterRow = 0; uCharacterRow < 8; uCharacterRow++)
            {
                for(uCharacterColumn = 0; uCharacterColumn < 8; uCharacterColumn++)
                {
                    uBit = Info->Font->Bits[uCharacter * 8 + uCharacterRow] << uCharacterColumn & 0x80;
                    DrawCharacterPixel(ps, uCharacter, uCharacterRow, uCharacterColumn, uBit, clBackground);
                }
            }
        }
    }

    SelectObject(hBoxDC, hBitmapOld);
    SelectObject(ps->hdc, hOldFont);
    DeleteObject(hFont);
    SelectObject(ps->hdc, hOldBrush);
    DeleteObject(hBrush);
    DeleteDC(hBoxDC);
}

VOID
EditCurrentGlyph(PFONT_WND_INFO FontWndInfo)
{
    PEDIT_GLYPH_INFO EditGlyphInfo;

    // Has the window for this character already been opened?
    EditGlyphInfo = FontWndInfo->FirstEditGlyphWnd;

    while(EditGlyphInfo)
    {
        if(EditGlyphInfo->uCharacter == FontWndInfo->uSelectedCharacter)
        {
            // Yes, it has. Bring it to the front.
            SetFocus(EditGlyphInfo->hSelf);
            return;
        }

        EditGlyphInfo = EditGlyphInfo->NextEditGlyphWnd;
    }

    // No. Then create a new one
    EditGlyphInfo = (PEDIT_GLYPH_INFO) HeapAlloc( hProcessHeap, 0, sizeof(EDIT_GLYPH_INFO) );
    EditGlyphInfo->FontWndInfo = FontWndInfo;
    EditGlyphInfo->uCharacter = FontWndInfo->uSelectedCharacter;
    RtlCopyMemory( EditGlyphInfo->CharacterBits, FontWndInfo->Font->Bits + FontWndInfo->uSelectedCharacter * 8, sizeof(EditGlyphInfo->CharacterBits) );

    // Add the new window to the linked list
    EditGlyphInfo->PrevEditGlyphWnd = FontWndInfo->LastEditGlyphWnd;
    EditGlyphInfo->NextEditGlyphWnd = NULL;

    if(FontWndInfo->LastEditGlyphWnd)
        FontWndInfo->LastEditGlyphWnd->NextEditGlyphWnd = EditGlyphInfo;
    else
        FontWndInfo->FirstEditGlyphWnd = EditGlyphInfo;

    FontWndInfo->LastEditGlyphWnd = EditGlyphInfo;

    // Open the window as a modeless dialog, so people can edit several characters at the same time.
    EditGlyphInfo->hSelf = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_EDITGLYPH), FontWndInfo->hSelf, EditGlyphDlgProc, (LPARAM)EditGlyphInfo);
    ShowWindow(EditGlyphInfo->hSelf, SW_SHOW);
}

VOID
CreateFontBoxesWindow(IN PFONT_WND_INFO FontWndInfo)
{
    FontWndInfo->hFontBoxesWnd = CreateWindowExW(0,
                                                 szFontBoxesWndClass,
                                                 0,
                                                 WS_CHILD | WS_VISIBLE,
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 FontWndInfo->hSelf,
                                                 NULL,
                                                 hInstance,
                                                 FontWndInfo);
}

static LRESULT CALLBACK
FontBoxesWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PFONT_WND_INFO Info;

    Info = (PFONT_WND_INFO) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if(Info || uMsg == WM_CREATE)
    {
        switch(uMsg)
        {
            case WM_CREATE:
                Info = (PFONT_WND_INFO)( ( (LPCREATESTRUCT)lParam )->lpCreateParams );
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)Info);

                // Set a fixed window size
                SetWindowPos(hwnd, NULL, 0, 0, FONT_BOXES_WND_WIDTH, FONT_BOXES_WND_HEIGHT, SWP_NOZORDER | SWP_NOMOVE);

                return 0;

            case WM_DESTROY:
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                return 0;

            case WM_KEYDOWN:
                switch(wParam)
                {
                    case VK_DOWN:
                        if(Info->uSelectedCharacter < 239)
                            SetSelectedCharacter(Info, Info->uSelectedCharacter + 16, NULL);
                        return 0;

                    case VK_LEFT:
                        if(Info->uSelectedCharacter)
                            SetSelectedCharacter(Info, Info->uSelectedCharacter - 1, NULL);
                        return 0;

                    case VK_RETURN:
                        EditCurrentGlyph(Info);
                        return 0;

                    case VK_RIGHT:
                        if(Info->uSelectedCharacter < 255)
                            SetSelectedCharacter(Info, Info->uSelectedCharacter + 1, NULL);
                        return 0;

                    case VK_UP:
                        if(Info->uSelectedCharacter > 15)
                            SetSelectedCharacter(Info, Info->uSelectedCharacter - 16, NULL);
                        return 0;
                }

                break;

            case WM_LBUTTONDBLCLK:
            {
                EditCurrentGlyph(Info);
                return 0;
            }

            case WM_LBUTTONDOWN:
            {
                RECT CharacterRect;
                INT iRet;

                iRet = FontBoxesHitTest( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &CharacterRect );

                if(iRet >= 0)
                    SetSelectedCharacter( Info, (UINT)iRet, &CharacterRect );

                return 0;
            }

            case WM_PAINT:
            {
                PAINTSTRUCT ps;

                BeginPaint(hwnd, &ps);
                DrawProc(Info, &ps);
                EndPaint(hwnd, &ps);

                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL
InitFontBoxesWndClass(VOID)
{
    WNDCLASSW wc = {0,};

    wc.lpfnWndProc    = FontBoxesWndProc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
    wc.lpszClassName  = szFontBoxesWndClass;
    wc.style          = CS_DBLCLKS;

    return RegisterClassW(&wc) != 0;
}

VOID
UnInitFontBoxesWndClass(VOID)
{
    UnregisterClassW(szFontBoxesWndClass, hInstance);
}
