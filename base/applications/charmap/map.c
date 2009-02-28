/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/map.c
 * PURPOSE:     class implementation for painting glyph region
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

static const WCHAR szMapWndClass[] = L"FontMapWnd";
static const WCHAR szLrgCellWndClass[] = L"LrgCellWnd";

static
VOID
TagFontToCell(PCELL pCell,
              WCHAR ch)
{
    pCell->ch = ch;
}


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
         HDC hdc)
{
    INT x, y;

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        Rectangle(hdc,
                  infoPtr->Cells[y][x].CellExt.left,
                  infoPtr->Cells[y][x].CellExt.top,
                  infoPtr->Cells[y][x].CellExt.right,
                  infoPtr->Cells[y][x].CellExt.bottom);
    }

    if (infoPtr->pActiveCell)
        DrawActiveCell(infoPtr,
                       hdc);
}


static
VOID
FillGrid(PMAP infoPtr,
         HDC hdc)
{
    HFONT hOldFont;
    WCHAR ch;
    INT x, y;

    hOldFont = SelectObject(hdc,
                            infoPtr->hFont);

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        ch = (WCHAR)((XCELLS * (y + infoPtr->iYStart)) + x);

        TagFontToCell(&infoPtr->Cells[y][x], ch);

        DrawTextW(hdc,
                  &ch,
                  1,
                  &infoPtr->Cells[y][x].CellInt,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc,
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
SetFont(PMAP infoPtr,
        LPWSTR lpFontName)
{
    HDC hdc;

    if (infoPtr->hFont)
        DeleteObject(infoPtr->hFont);

    ZeroMemory(&infoPtr->CurrentFont,
               sizeof(LOGFONTW));

    hdc = GetDC(infoPtr->hMapWnd);
    infoPtr->CurrentFont.lfHeight = GetDeviceCaps(hdc,
                                                  LOGPIXELSY) / 5;
    ReleaseDC(infoPtr->hMapWnd, hdc);

    infoPtr->CurrentFont.lfCharSet =  DEFAULT_CHARSET;
    wcscpy(infoPtr->CurrentFont.lfFaceName,
           lpFontName);

    infoPtr->hFont = CreateFontIndirectW(&infoPtr->CurrentFont);

    InvalidateRect(infoPtr->hMapWnd,
                   NULL,
                   TRUE);
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
    POINT pt;
    INT x, y;

    pt.x = ptx;
    pt.y = pty;

    for (x = 0; x < XCELLS; x++)
    for (y = 0; y < YCELLS; y++)
    {
        if (PtInRect(&infoPtr->Cells[y][x].CellInt,
                     pt))
        {
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
            else
            {
                /* flick between large and small */
                if (infoPtr->pActiveCell->bLarge)
                {
                    DestroyWindow(infoPtr->hLrgWnd);
                    infoPtr->hLrgWnd = NULL;
                }
                else
                {
                    CreateLargeCell(infoPtr);
                }

                infoPtr->pActiveCell->bLarge = (infoPtr->pActiveCell->bLarge) ? FALSE : TRUE;
            }

            break;
        }
    }
}


static
BOOL
OnCreate(PMAP infoPtr,
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

            SetScrollRange(hwnd, SB_VERT, 0, 255, FALSE);
            SetScrollPos(hwnd, SB_VERT, 0, TRUE);

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

    infoPtr->iYStart = max(0,
                         min(infoPtr->iYStart, 255*16));

    iYDiff = iOldYStart - infoPtr->iYStart;
    if (iYDiff)
    {
        SetScrollPos(infoPtr->hMapWnd,
                     SB_VERT,
                     infoPtr->iYStart,
                     TRUE);

        if (abs(iYDiff) < YCELLS)
        {
            RECT rect;
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
        hdc = (HDC)wParam;
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

    DrawGrid(infoPtr,
             hdc);

    FillGrid(infoPtr,
             hdc);

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

    infoPtr = (PMAP)GetWindowLongPtrW(hwnd,
                                      0);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            if (!OnCreate(infoPtr,
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

        case WM_LBUTTONDBLCLK:
        {
            NotifyParentOfSelection(infoPtr,
                                    FM_SETCHAR,
                                    infoPtr->pActiveCell->ch);


            break;
        }

        case WM_VSCROLL:
        {
            OnVScroll(infoPtr,
                      LOWORD(wParam),
                      HIWORD(wParam));

            break;
        }

        case FM_SETFONT:
            SetFont(infoPtr, (LPWSTR)lParam);
            break;

        case FM_GETCHAR:
        {
            if (!infoPtr->pActiveCell) return 0;
            return infoPtr->pActiveCell->ch;
        }

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
