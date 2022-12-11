/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/map.c
 * PURPOSE:     class implementation for painting glyph region
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *              Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 */

#include "precomp.h"

#include <stdlib.h>
#include <winnls.h>

static const WCHAR szMapWndClass[] = L"FontMapWnd";
static const WCHAR szLrgCellWndClass[] = L"LrgCellWnd";

#define MAX_ROWS (0xFFFF / XCELLS) + 1 - YCELLS

static
VOID
SetGrid(PMAP infoPtr)
{
    INT x, y;
    PCELL Cell;

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        Cell = &infoPtr->Cells[y][x];
        Cell->CellExt.left = x * infoPtr->CellSize.cx + 1;
        Cell->CellExt.top = y * infoPtr->CellSize.cy + 1;
        Cell->CellExt.right = (x + 1) * infoPtr->CellSize.cx + 2;
        Cell->CellExt.bottom = (y + 1) * infoPtr->CellSize.cy + 2;

        Cell->CellInt = Cell->CellExt;

        InflateRect(&Cell->CellInt, -1, -1);
    }
}

static
VOID
UpdateCells(PMAP infoPtr)
{
    INT x, y;
    INT i = XCELLS * infoPtr->iYStart;
    WCHAR ch;
    PCELL Cell;

    for (y = 0; y < YCELLS; ++y)
    {
        for (x = 0; x < XCELLS; ++x, ++i)
        {
            if (i < infoPtr->NumValidGlyphs)
                ch = (WCHAR)infoPtr->ValidGlyphs[i];
            else
                ch = 0xFFFF;

            Cell = &infoPtr->Cells[y][x];
            Cell->ch = ch;
        }
    }
}

static
VOID
FillGrid(PMAP infoPtr,
         PAINTSTRUCT *ps)
{
    HFONT hOldFont;
    INT x, y;
    RECT rc;
    PCELL Cell;
    INT i;
    HBRUSH hOldBrush, hbrGray = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    HPEN hOldPen, hPenGray = CreatePen(PS_SOLID, 1, RGB(140, 140, 140));

    UpdateCells(infoPtr);

    hOldFont = SelectObject(ps->hdc, infoPtr->hFont);
    hOldPen = SelectObject(ps->hdc, GetStockObject(BLACK_PEN));
    hOldBrush = SelectObject(ps->hdc, GetStockObject(WHITE_BRUSH));

    i = XCELLS * infoPtr->iYStart;

    for (y = 0; y < YCELLS; y++)
    {
        for (x = 0; x < XCELLS; x++, i++)
        {
            Cell = &infoPtr->Cells[y][x];
            if (!IntersectRect(&rc, &ps->rcPaint, &Cell->CellExt))
                continue;

            rc = Cell->CellExt;
            Rectangle(ps->hdc, rc.left, rc.top, rc.right, rc.bottom);

            if (i < infoPtr->NumValidGlyphs)
            {
                DrawTextW(ps->hdc, &Cell->ch, 1, &Cell->CellInt,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                if (Cell == infoPtr->pActiveCell)
                {
                    rc = Cell->CellInt;

                    /* Draw gray box */
                    SelectObject(ps->hdc, GetStockObject(NULL_BRUSH));
                    SelectObject(ps->hdc, hPenGray);
                    Rectangle(ps->hdc, rc.left, rc.top, rc.right, rc.bottom);
                    SelectObject(ps->hdc, hOldPen);
                    SelectObject(ps->hdc, hOldBrush);

                    if (GetFocus() == infoPtr->hMapWnd)
                    {
                        /* Draw focus rectangle */
                        InflateRect(&rc, -1, -1);
                        DrawFocusRect(ps->hdc, &rc);
                    }
                }
            }
            else
            {
                FillRect(ps->hdc, &Cell->CellInt, hbrGray);
            }
        }
    }

    SelectObject(ps->hdc, hOldFont);
    SelectObject(ps->hdc, hOldPen);
    SelectObject(ps->hdc, hOldBrush);
    DeleteObject(hPenGray);
}


static
BOOL
CreateLargeCell(PMAP infoPtr)
{
    RECT rLarge = infoPtr->pActiveCell->CellExt;

    MapWindowPoints(infoPtr->hMapWnd, infoPtr->hParent, (LPPOINT)&rLarge, 2);

    InflateRect(&rLarge, XLARGE - XCELLS, YLARGE - YCELLS);

    infoPtr->hLrgWnd = CreateWindowExW(0,
                                       szLrgCellWndClass,
                                       NULL,
                                       WS_CHILDWINDOW | WS_VISIBLE,
                                       rLarge.left,
                                       rLarge.top,
                                       rLarge.right - rLarge.left,
                                       rLarge.bottom - rLarge.top,
                                       infoPtr->hParent,
                                       NULL,
                                       hInstance,
                                       infoPtr);
    if (!infoPtr->hLrgWnd)
        return FALSE;

    return TRUE;
}


static
VOID
MoveLargeCell(PMAP infoPtr)
{
    RECT rLarge = infoPtr->pActiveCell->CellExt;

    MapWindowPoints(infoPtr->hMapWnd, infoPtr->hParent, (LPPOINT)&rLarge, 2);

    InflateRect(&rLarge, XLARGE - XCELLS, YLARGE - YCELLS);

    MoveWindow(infoPtr->hLrgWnd,
               rLarge.left,
               rLarge.top,
               rLarge.right - rLarge.left,
               rLarge.bottom - rLarge.top,
               TRUE);

    InvalidateRect(infoPtr->hLrgWnd, NULL, TRUE);
}


static
VOID
GetPossibleCharacters(WCHAR* ch, INT chLen, INT codePageIdx)
{
    INT i, j;

    ZeroMemory(ch, sizeof(ch[0]) * chLen);

    if (codePageIdx <= 0 || codePageIdx > SIZEOF(codePages))
    {
        /* this is unicode, so just load up the first MAX_GLYPHS characters
           start at 0x21 to bypass whitespace characters */
        INT len = min(MAX_GLYPHS, chLen);
        for (i = 0x21, j = 0; i < len; i++)
            ch[j++] = (WCHAR)i;
    }
    else
    {
        /* This is a codepage, so use NLS to translate the first 256 characters */
        CHAR multiByteString[256] = { 0 };
        for (i = 0x21; i < SIZEOF(multiByteString); i++)
            multiByteString[i] = (CHAR)i;

        if (!MultiByteToWideChar(codePages[codePageIdx - 1], 0, multiByteString, sizeof(multiByteString), ch, chLen))
        {
            /* Failed for some reason, so clear the array */
            memset(ch, 0, sizeof(ch[0]) * chLen);
        }
    }
}


static
VOID
SetFont(PMAP infoPtr,
        LPWSTR lpFontName)
{
    HDC hdc;
    WCHAR ch[MAX_GLYPHS];
    WORD out[MAX_GLYPHS];
    DWORD i, j;

    /* Destroy Zoom window, since it was created with older font */
    DestroyWindow(infoPtr->hLrgWnd);
    infoPtr->hLrgWnd = NULL;

    if (infoPtr->hFont)
        DeleteObject(infoPtr->hFont);

    ZeroMemory(&infoPtr->CurrentFont,
               sizeof(LOGFONTW));

    hdc = GetDC(infoPtr->hMapWnd);
    infoPtr->CurrentFont.lfHeight = GetDeviceCaps(hdc, LOGPIXELSY) / 5;

    infoPtr->CurrentFont.lfCharSet =  DEFAULT_CHARSET;
    lstrcpynW(infoPtr->CurrentFont.lfFaceName,
              lpFontName,
              SIZEOF(infoPtr->CurrentFont.lfFaceName));

    infoPtr->hFont = CreateFontIndirectW(&infoPtr->CurrentFont);

    InvalidateRect(infoPtr->hMapWnd,
                   NULL,
                   TRUE);

    // Get all the valid glyphs in this font

    SelectObject(hdc, infoPtr->hFont);

    // Get the code page associated with the selected 'character set'
    GetPossibleCharacters(ch, MAX_GLYPHS, infoPtr->CharMap);

    if (GetGlyphIndicesW(hdc,
                         ch,
                         MAX_GLYPHS,
                         out,
                         GGI_MARK_NONEXISTING_GLYPHS) != GDI_ERROR)
    {
        j = 0;
        for (i = 0; i < MAX_GLYPHS; i++)
        {
            if (out[i] != 0xffff && out[i] != 0x0000 && ch[i] != 0x0000)
            {
                infoPtr->ValidGlyphs[j] = ch[i];
                j++;
            }
        }
        infoPtr->NumValidGlyphs = j;
    }

    ReleaseDC(infoPtr->hMapWnd, hdc);

    infoPtr->NumRows = infoPtr->NumValidGlyphs / XCELLS;
    if (infoPtr->NumValidGlyphs % XCELLS)
        infoPtr->NumRows += 1;
    infoPtr->NumRows = (infoPtr->NumRows > YCELLS) ? infoPtr->NumRows - YCELLS : 0;

    SetScrollRange(infoPtr->hMapWnd, SB_VERT, 0, infoPtr->NumRows, FALSE);
    SetScrollPos(infoPtr->hMapWnd, SB_VERT, 0, TRUE);
    infoPtr->iYStart = 0;
}


static
LRESULT
NotifyParentOfSelection(PMAP infoPtr,
                        UINT code,
                        WCHAR ch)
{
    LRESULT Ret = 0;

    if (infoPtr->hParent != NULL)
    {
        DWORD dwIdc = GetWindowLongPtr(infoPtr->hMapWnd, GWLP_ID);
        /*
         * Push directly into the event queue instead of waiting
         * the parent to be unlocked.
         * High word of LPARAM is still available for future needs...
         */
        Ret = PostMessage(infoPtr->hParent,
                          WM_COMMAND,
                          MAKELPARAM((WORD)dwIdc, (WORD)code),
                          (LPARAM)LOWORD(ch));
    }

    return Ret;
}


static
VOID
LimitCaretXY(PMAP infoPtr, INT *pX, INT *pY)
{
    INT i, X = *pX, Y = *pY, iYStart = infoPtr->iYStart;

    i = XCELLS * (iYStart + Y) + X;
    while (i >= infoPtr->NumValidGlyphs)
    {
        if (X > 0)
        {
            --X;
        }
        else
        {
            X = XCELLS - 1;
            --Y;
        }
        i = XCELLS * (iYStart + Y) + X;
    }

    *pX = X;
    *pY = Y;
}

static
VOID
SetCaretXY(PMAP infoPtr, INT X, INT Y, BOOL bLarge, BOOL bInvalidateAll)
{

    /* set previous active cell to inactive */
    if (!bInvalidateAll)
    {
        InvalidateRect(infoPtr->hMapWnd,
                       &infoPtr->pActiveCell->CellInt,
                       FALSE);
    }

    LimitCaretXY(infoPtr, &X, &Y);
    infoPtr->CaretX = X;
    infoPtr->CaretY = Y;
    UpdateCells(infoPtr);

    /* set new cell to active */
    infoPtr->pActiveCell = &infoPtr->Cells[Y][X];
    if (!bInvalidateAll)
    {
        InvalidateRect(infoPtr->hMapWnd,
                       &infoPtr->pActiveCell->CellInt,
                       FALSE);
    }

    /* Create if needed */
    if (bLarge)
    {
        if (infoPtr->hLrgWnd)
            MoveLargeCell(infoPtr);
        else
            CreateLargeCell(infoPtr);
    }
    else
    {
        /* Destroy large window */
        if (infoPtr->hLrgWnd)
        {
            DestroyWindow(infoPtr->hLrgWnd);
            infoPtr->hLrgWnd = NULL;
        }
    }

    if (bInvalidateAll)
        InvalidateRect(infoPtr->hMapWnd, NULL, FALSE);

    UpdateStatusBar(infoPtr->pActiveCell->ch);
}

static
VOID
OnClick(PMAP infoPtr,
        WORD ptx,
        WORD pty)
{
    /*
     * Find the cell the mouse pointer is over.
     * Since each cell is the same size, this can be done quickly using CellSize.
     * Clamp to XCELLS - 1 and YCELLS - 1 because the map can sometimes be slightly
     * larger than infoPtr.CellSize * XCELLS , due to the map size being a non integer
     * multiple of infoPtr.CellSize .
     */
    INT x = min(XCELLS - 1, ptx / max(1, infoPtr->CellSize.cx));
    INT y = min(YCELLS - 1, pty / max(1, infoPtr->CellSize.cy));

    SetCaretXY(infoPtr, x, y, TRUE, FALSE);
}


static
BOOL
MapOnCreate(PMAP infoPtr,
            HWND hwnd,
            HWND hParent)
{
    RECT rc;

    infoPtr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MAP));
    if (!infoPtr)
        return FALSE;

    SetWindowLongPtrW(hwnd, 0, (LONG_PTR)infoPtr);

    infoPtr->hMapWnd = hwnd;
    infoPtr->hParent = hParent;

    GetClientRect(hwnd, &rc);
    infoPtr->ClientSize.cx = rc.right;
    infoPtr->ClientSize.cy = rc.bottom;
    infoPtr->CellSize.cx = infoPtr->ClientSize.cx / XCELLS;
    infoPtr->CellSize.cy = infoPtr->ClientSize.cy / YCELLS;

    infoPtr->pActiveCell = &infoPtr->Cells[0][0];

    SetGrid(infoPtr);

    SetScrollPos(infoPtr->hParent, SB_VERT, 0, TRUE);
    return TRUE;
}

static
VOID
OnVScroll(PMAP infoPtr,
          INT Value,
          INT Pos)
{
    INT iYDiff, iOldYStart = infoPtr->iYStart;
    INT X, Y;

    switch (Value)
    {
        case SB_LINEUP:
            infoPtr->iYStart -=  1;
            break;

        case SB_LINEDOWN:
            infoPtr->iYStart +=  1;
            break;

        case SB_PAGEUP:
            infoPtr->iYStart -= YCELLS;
            break;

        case SB_PAGEDOWN:
            infoPtr->iYStart += YCELLS;
            break;

        case SB_THUMBTRACK:
            infoPtr->iYStart = Pos;
            break;

        case SB_TOP:
            infoPtr->iYStart = 0;
            SetCaretXY(infoPtr, 0, 0, FALSE, TRUE);
            return;

        case SB_BOTTOM:
            infoPtr->iYStart = infoPtr->NumRows;
            SetCaretXY(infoPtr, XCELLS - 1, YCELLS - 1, FALSE, TRUE);
            break;

        default:
            break;
    }

    infoPtr->iYStart = max(0, infoPtr->iYStart);
    infoPtr->iYStart = min(infoPtr->iYStart, infoPtr->NumRows);

    UpdateCells(infoPtr);

    X = infoPtr->CaretX;
    Y = infoPtr->CaretY;
    LimitCaretXY(infoPtr, &X, &Y);
    SetCaretXY(infoPtr, X, Y, IsWindow(infoPtr->hLrgWnd), FALSE);

    iYDiff = iOldYStart - infoPtr->iYStart;
    if (iYDiff)
    {
        if (infoPtr->hLrgWnd != NULL)
        {
            ShowWindow(infoPtr->hLrgWnd, SW_HIDE);
        }

        SetScrollPos(infoPtr->hMapWnd,
                     SB_VERT,
                     infoPtr->iYStart,
                     TRUE);

        if (abs(iYDiff) < YCELLS)
        {
            RECT rect;

            /* Invalidate the rect around the active cell since a new cell will become active */
            if (infoPtr->pActiveCell)
            {
                InvalidateRect(infoPtr->hMapWnd,
                               &infoPtr->pActiveCell->CellExt,
                               TRUE);
            }

            GetClientRect(infoPtr->hMapWnd, &rect);
            rect.top += 2;
            rect.bottom -= 2;
            ScrollWindowEx(infoPtr->hMapWnd,
                           0,
                           iYDiff * infoPtr->CellSize.cy,
                           &rect,
                           &rect,
                           NULL,
                           NULL,
                           SW_INVALIDATE);
        }
        else
        {
            InvalidateRect(infoPtr->hMapWnd,
                           NULL,
                           TRUE);
        }

        if (infoPtr->hLrgWnd != NULL)
        {
            ShowWindow(infoPtr->hLrgWnd, SW_SHOW);
        }
    }

    UpdateStatusBar(infoPtr->pActiveCell->ch);
}


static
VOID
OnPaint(PMAP infoPtr,
        WPARAM wParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    if (wParam != 0)
    {
        if (!GetUpdateRect(infoPtr->hMapWnd, &ps.rcPaint, TRUE))
            return;

        ps.hdc = (HDC)wParam;
    }
    else
    {
        hdc = BeginPaint(infoPtr->hMapWnd, &ps);
        if (hdc == NULL)
            return;
    }

    FillGrid(infoPtr, &ps);

    if (wParam == 0)
    {
        EndPaint(infoPtr->hMapWnd, &ps);
    }
}

static
VOID
MoveUpDown(PMAP infoPtr, INT DY)
{
    INT Y = infoPtr->CaretY;

    if (DY < 0) /* Move Up */
    {
        if (Y <= 0)
        {
            SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            return;
        }

        Y -= 1;
    }
    else if (DY > 0) /* Move Down */
    {
        if (Y + 1 >= YCELLS)
        {
            SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            return;
        }

        Y += 1;
    }

    SetCaretXY(infoPtr, infoPtr->CaretX, Y, IsWindow(infoPtr->hLrgWnd), FALSE);
}

static
VOID
MoveLeftRight(PMAP infoPtr, INT DX)
{
    INT X = infoPtr->CaretX;
    INT Y = infoPtr->CaretY;

    if (DX < 0) /* Move Left */
    {
        if (X <= 0) /* at left edge */
        {
            if (Y <= 0) /* at top */
            {
                Y = 0;
                if (infoPtr->iYStart > 0)
                    X = XCELLS - 1;
                SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
            else
            {
                X = XCELLS - 1;
                Y -= 1;
            }
        }
        else /* Not at left edge */
        {
            X -= 1;
        }
    }
    else if (DX > 0) /* Move Right */
    {
        if (X + 1 >= XCELLS) /* at right edge */
        {
            if (Y + 1 >= YCELLS) /* at bottom */
            {
                Y = YCELLS - 1;
                if (infoPtr->iYStart < infoPtr->NumRows)
                    X = 0;
                SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            }
            else
            {
                X = 0;
                Y += 1;
            }
        }
        else
        {
            X += 1;
        }
    }

    SetCaretXY(infoPtr, X, Y, IsWindow(infoPtr->hLrgWnd), FALSE);
}

static
VOID
OnKeyDown(PMAP infoPtr, WPARAM wParam, LPARAM lParam)
{
    BOOL bCtrlDown = (GetKeyState(VK_CONTROL) < 0);

    switch (wParam)
    {
        case VK_UP:
            if (bCtrlDown)
                SetCaretXY(infoPtr, infoPtr->CaretX, 0, FALSE, FALSE);
            else
                MoveUpDown(infoPtr, -1);
            break;

        case VK_DOWN:
            if (bCtrlDown)
                SetCaretXY(infoPtr, infoPtr->CaretX, YCELLS - 1, FALSE, FALSE);
            else
                MoveUpDown(infoPtr, +1);
            break;

        case VK_LEFT:
            if (bCtrlDown)
                SetCaretXY(infoPtr, 0, infoPtr->CaretY, FALSE, FALSE);
            else
                MoveLeftRight(infoPtr, -1);
            break;

        case VK_RIGHT:
            if (bCtrlDown)
                SetCaretXY(infoPtr, XCELLS - 1, infoPtr->CaretY, FALSE, FALSE);
            else
                MoveLeftRight(infoPtr, +1);
            break;

        case VK_PRIOR: /* Page Up */
            SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
            break;

        case VK_NEXT: /* Page Down */
            SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
            break;

        case VK_HOME:
            if (bCtrlDown)
                SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
            else
                SetCaretXY(infoPtr, 0, infoPtr->CaretY, FALSE, FALSE);
            break;

        case VK_END:
            if (bCtrlDown)
                SendMessageW(infoPtr->hMapWnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
            else
                SetCaretXY(infoPtr, XCELLS - 1, infoPtr->CaretY, FALSE, FALSE);
            break;
    }
}

LRESULT
CALLBACK
MapWndProc(HWND hwnd,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    PMAP infoPtr = (PMAP)GetWindowLongPtrW(hwnd, 0);
    LRESULT Ret = 0;
    WCHAR lfFaceName[LF_FACESIZE];

    switch (uMsg)
    {
        case WM_CREATE:
        {
            if (!MapOnCreate(infoPtr,
                             hwnd,
                             ((LPCREATESTRUCTW)lParam)->hwndParent))
            {
                return (LRESULT)-1;
            }

            break;
        }

        case WM_KEYDOWN:
        {
            OnKeyDown(infoPtr, wParam, lParam);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            SetFocus(hwnd);
            OnClick(infoPtr, LOWORD(lParam), HIWORD(lParam));
            break;
        }

        case WM_MOUSEMOVE:
        {
            if (wParam & MK_LBUTTON)
            {
                OnClick(infoPtr, LOWORD(lParam), HIWORD(lParam));
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            if (!infoPtr->pActiveCell || GetFocus() != hwnd)
                break;

            NotifyParentOfSelection(infoPtr,
                                    FM_SETCHAR,
                                    infoPtr->pActiveCell->ch);

            if (infoPtr->hLrgWnd)
            {
                DestroyWindow(infoPtr->hLrgWnd);
                infoPtr->hLrgWnd = NULL;
            }
            break;
        }

        case WM_VSCROLL:
        {
            OnVScroll(infoPtr, LOWORD(wParam), HIWORD(wParam));
            break;
        }

        case FM_SETCHARMAP:
            infoPtr->CaretX = infoPtr->CaretY = infoPtr->iYStart = 0;
            infoPtr->CharMap = LOWORD(wParam);
            wcsncpy(lfFaceName,
                    infoPtr->CurrentFont.lfFaceName,
                    SIZEOF(lfFaceName));
            SetFont(infoPtr, lfFaceName);
            break;

        case FM_SETFONT:
            infoPtr->CaretX = infoPtr->CaretY = infoPtr->iYStart = 0;
            SetFont(infoPtr, (LPWSTR)lParam);
            break;

        case FM_GETCHAR:
        {
            if (!infoPtr->pActiveCell) return 0;
            return infoPtr->pActiveCell->ch;
        }

        case FM_GETHFONT:
            return (LRESULT)infoPtr->hFont;

        case WM_PAINT:
            OnPaint(infoPtr, wParam);
            break;

        case WM_DESTROY:
            DeleteObject(infoPtr->hFont);
            HeapFree(GetProcessHeap(), 0, infoPtr);
            SetWindowLongPtrW(hwnd, 0, (LONG_PTR)NULL);
            break;

        case WM_GETDLGCODE:
            return DLGC_WANTARROWS;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            if (!infoPtr->hLrgWnd)
                InvalidateRect(hwnd, &(infoPtr->pActiveCell->CellInt), FALSE);
            break;

        default:
            Ret = DefWindowProcW(hwnd, uMsg, wParam, lParam);
            break;
    }

    return Ret;
}


BOOL
RegisterMapClasses(HINSTANCE hInstance)
{
    WNDCLASSW wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MapWndProc;
    wc.cbWndExtra = sizeof(PMAP);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL,
                            (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = szMapWndClass;

    if (RegisterClassW(&wc))
    {
        wc.lpfnWndProc = LrgCellWndProc;
        wc.cbWndExtra = 0;
        wc.lpszClassName = szLrgCellWndClass;

        return RegisterClassW(&wc) != 0;
    }

    return FALSE;
}

VOID
UnregisterMapClasses(HINSTANCE hInstance)
{
    UnregisterClassW(szMapWndClass, hInstance);
    UnregisterClassW(szLrgCellWndClass, hInstance);
}
