/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/map.c
 * PURPOSE:     class implementation for painting glyph region
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
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

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        infoPtr->Cells[y][x].CellExt.left = x * infoPtr->CellSize.cx + 1;
        infoPtr->Cells[y][x].CellExt.top = y * infoPtr->CellSize.cy + 1;
        infoPtr->Cells[y][x].CellExt.right = (x + 1) * infoPtr->CellSize.cx + 2;
        infoPtr->Cells[y][x].CellExt.bottom = (y + 1) * infoPtr->CellSize.cy + 2;

        CopyRect(&infoPtr->Cells[y][x].CellInt,
                 &infoPtr->Cells[y][x].CellExt);

        InflateRect(&infoPtr->Cells[y][x].CellInt,
                    -1,
                    -1);
    }
}


static
VOID
DrawActiveCell(PMAP infoPtr,
               HDC hdc)
{
    Rectangle(hdc,
              infoPtr->pActiveCell->CellInt.left,
              infoPtr->pActiveCell->CellInt.top,
              infoPtr->pActiveCell->CellInt.right,
              infoPtr->pActiveCell->CellInt.bottom);

}


static
VOID
DrawGrid(PMAP infoPtr,
         PAINTSTRUCT *ps)
{
    INT x, y;
    RECT rc;
    PCELL Cell;

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        Cell = &infoPtr->Cells[y][x];

        if (!IntersectRect(&rc,
                           &ps->rcPaint,
                           &Cell->CellExt))
        {
            continue;
        }

        Rectangle(ps->hdc,
                  Cell->CellExt.left,
                  Cell->CellExt.top,
                  Cell->CellExt.right,
                  Cell->CellExt.bottom);

        if (infoPtr->pActiveCell == Cell)
        {
            DrawActiveCell(infoPtr, ps->hdc);
        }
    }
}


static
VOID
FillGrid(PMAP infoPtr,
         PAINTSTRUCT *ps)
{
    HFONT hOldFont;
    WCHAR ch;
    INT x, y;
    RECT rc;
    PCELL Cell;
    INT i, added;

    hOldFont = SelectObject(ps->hdc,
                            infoPtr->hFont);

    i = XCELLS * infoPtr->iYStart;

    added = 0;

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        if (i >= infoPtr->NumValidGlyphs) break;

        ch = (WCHAR)infoPtr->ValidGlyphs[i];

        Cell = &infoPtr->Cells[y][x];

        if (IntersectRect(&rc,
                            &ps->rcPaint,
                            &Cell->CellExt))
        {
            Cell->ch = ch;

            DrawTextW(ps->hdc,
                        &ch,
                        1,
                        &Cell->CellInt,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            added++;
        }

        i++;
        ch = (WCHAR)i;
    }
    SelectObject(ps->hdc,
                 hOldFont);
}


static
BOOL
CreateLargeCell(PMAP infoPtr)
{
    RECT rLarge;

    CopyRect(&rLarge,
             &infoPtr->pActiveCell->CellExt);

    MapWindowPoints(infoPtr->hMapWnd,
                    infoPtr->hParent,
                    (VOID*)&rLarge,
                    2);

    InflateRect(&rLarge,
                XLARGE - XCELLS,
                YLARGE - YCELLS);

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
    RECT rLarge;

    CopyRect(&rLarge,
             &infoPtr->pActiveCell->CellExt);

    MapWindowPoints(infoPtr->hMapWnd,
                    infoPtr->hParent,
                    (VOID*)&rLarge,
                    2);

    InflateRect(&rLarge,
                XLARGE - XCELLS,
                YLARGE - YCELLS);

    MoveWindow(infoPtr->hLrgWnd,
               rLarge.left,
               rLarge.top,
               rLarge.right - rLarge.left,
               rLarge.bottom - rLarge.top,
               TRUE);

    InvalidateRect(infoPtr->hLrgWnd,
                   NULL,
                   TRUE);
}


static
VOID
GetPossibleCharacters(WCHAR* ch, INT chLen, INT codePageIdx)
{
    INT i, j;

    memset(ch, 0, sizeof(ch[0]) * chLen);

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

    if (infoPtr->pActiveCell)
        infoPtr->pActiveCell->bActive = FALSE;
    infoPtr->pActiveCell = &infoPtr->Cells[0][0];
    infoPtr->pActiveCell->bActive = TRUE;

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
OnClick(PMAP infoPtr,
        WORD ptx,
        WORD pty)
{
    INT x, y, i;

    /*
     * Find the cell the mouse pointer is over.
     * Since each cell is the same size, this can be done quickly using CellSize.
     * Clamp to XCELLS - 1 and YCELLS - 1 because the map can sometimes be slightly
     * larger than infoPtr.CellSize * XCELLS , due to the map size being a non integer
     * multiple of infoPtr.CellSize .
     */
    x = min(XCELLS - 1, ptx / max(1, infoPtr->CellSize.cx));
    y = min(YCELLS - 1, pty / max(1, infoPtr->CellSize.cy));

    /* Make sure the mouse is within a valid glyph */
    i = XCELLS * infoPtr->iYStart + y * XCELLS + x;
    if (i >= infoPtr->NumValidGlyphs)
    {
        if (infoPtr->pActiveCell)
            infoPtr->pActiveCell->bActive = FALSE;
        infoPtr->pActiveCell = NULL;
        return;
    }

    /* if the cell is not already active */
    if (!infoPtr->Cells[y][x].bActive)
    {
        /* set previous active cell to inactive */
        if (infoPtr->pActiveCell)
        {
            /* invalidate normal cells, required when
             * moving a small active cell via keyboard */
            if (!infoPtr->pActiveCell->bLarge)
            {
                InvalidateRect(infoPtr->hMapWnd,
                               &infoPtr->pActiveCell->CellInt,
                               TRUE);
            }

            infoPtr->pActiveCell->bActive = FALSE;
            infoPtr->pActiveCell->bLarge = FALSE;
        }

        /* set new cell to active */
        infoPtr->pActiveCell = &infoPtr->Cells[y][x];
        infoPtr->pActiveCell->bActive = TRUE;
        infoPtr->pActiveCell->bLarge = TRUE;
        if (infoPtr->hLrgWnd)
            MoveLargeCell(infoPtr);
        else
            CreateLargeCell(infoPtr);
    }
}


static
BOOL
MapOnCreate(PMAP infoPtr,
            HWND hwnd,
            HWND hParent)
{
    RECT rc;
    BOOL Ret = FALSE;

    infoPtr = HeapAlloc(GetProcessHeap(),
                        0,
                        sizeof(MAP));
    if (infoPtr)
    {
        SetLastError(0);
        SetWindowLongPtrW(hwnd,
                          0,
                          (DWORD_PTR)infoPtr);
        if (GetLastError() == 0)
        {
            ZeroMemory(infoPtr,
                       sizeof(MAP));

            infoPtr->hMapWnd = hwnd;
            infoPtr->hParent = hParent;

            GetClientRect(hwnd, &rc);
            infoPtr->ClientSize.cx = rc.right;
            infoPtr->ClientSize.cy = rc.bottom;
            infoPtr->CellSize.cx = infoPtr->ClientSize.cx / XCELLS;
            infoPtr->CellSize.cy = infoPtr->ClientSize.cy / YCELLS;

            infoPtr->pActiveCell = NULL;

            SetGrid(infoPtr);

            SetScrollPos(infoPtr->hParent, SB_VERT, 0, TRUE);

            Ret = TRUE;
        }
    }

    return Ret;
}


static
VOID
OnVScroll(PMAP infoPtr,
          INT Value,
          INT Pos)
{
    INT iYDiff, iOldYStart = infoPtr->iYStart;

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

       default:
            break;
       }

    infoPtr->iYStart = max(0, infoPtr->iYStart);
    infoPtr->iYStart = min(infoPtr->iYStart, infoPtr->NumRows);

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
            if (infoPtr->pActiveCell && infoPtr->pActiveCell->bActive)
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
        if (!GetUpdateRect(infoPtr->hMapWnd,
                           &ps.rcPaint,
                           TRUE))
        {
            return;
        }
        ps.hdc = (HDC)wParam;
    }
    else
    {
        hdc = BeginPaint(infoPtr->hMapWnd,
                         &ps);
        if (hdc == NULL)
        {
            return;
        }
    }

    DrawGrid(infoPtr, &ps);

    FillGrid(infoPtr, &ps);

    if (wParam == 0)
    {
        EndPaint(infoPtr->hMapWnd,
                 &ps);
    }
}


LRESULT
CALLBACK
MapWndProc(HWND hwnd,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    PMAP infoPtr;
    LRESULT Ret = 0;
    WCHAR lfFaceName[LF_FACESIZE];

    infoPtr = (PMAP)GetWindowLongPtrW(hwnd,
                                      0);

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

        case WM_LBUTTONDOWN:
        {
            OnClick(infoPtr,
                    LOWORD(lParam),
                    HIWORD(lParam));

            break;
        }

        case WM_MOUSEMOVE:
        {
            if (wParam & MK_LBUTTON)
            {
                OnClick(infoPtr,
                        LOWORD(lParam),
                        HIWORD(lParam));
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            if (!infoPtr->pActiveCell)
                break;

            NotifyParentOfSelection(infoPtr,
                                    FM_SETCHAR,
                                    infoPtr->pActiveCell->ch);

            if (infoPtr->pActiveCell->bLarge)
            {
                DestroyWindow(infoPtr->hLrgWnd);
                infoPtr->hLrgWnd = NULL;
            }

            infoPtr->pActiveCell->bLarge = FALSE;

            break;
        }

        case WM_VSCROLL:
        {
            OnVScroll(infoPtr,
                      LOWORD(wParam),
                      HIWORD(wParam));

            break;
        }

        case FM_SETCHARMAP:
            infoPtr->CharMap = LOWORD(wParam);
            wcsncpy(lfFaceName,
                    infoPtr->CurrentFont.lfFaceName,
                    SIZEOF(lfFaceName));
            SetFont(infoPtr, lfFaceName);
            break;

        case FM_SETFONT:
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
        {
            OnPaint(infoPtr,
                    wParam);
            break;
        }

        case WM_DESTROY:
        {
            DeleteObject(infoPtr->hFont);
            HeapFree(GetProcessHeap(),
                     0,
                     infoPtr);
            SetWindowLongPtrW(hwnd,
                              0,
                              (DWORD_PTR)NULL);
            break;
        }

        default:
        {
            Ret = DefWindowProcW(hwnd,
                                 uMsg,
                                 wParam,
                                 lParam);
            break;
        }
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
    UnregisterClassW(szMapWndClass,
                    hInstance);

    UnregisterClassW(szLrgCellWndClass,
                    hInstance);
}
