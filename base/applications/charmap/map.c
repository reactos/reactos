/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/map.c
 * PURPOSE:     class implementation for painting glyph region
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

static const TCHAR szMapWndClass[] = TEXT("FontMapWnd");
static const TCHAR szLrgCellWndClass[] = TEXT("LrgCellWnd");

static VOID
TagFontToCell(PCELL pCell,
              TCHAR ch)
{
    pCell->ch = ch;
}


static VOID
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

static VOID
DrawActiveCell(PMAP infoPtr,
               HDC hdc)
{
    Rectangle(hdc,
              infoPtr->pActiveCell->CellInt.left,
              infoPtr->pActiveCell->CellInt.top,
              infoPtr->pActiveCell->CellInt.right,
              infoPtr->pActiveCell->CellInt.bottom);

}


static VOID
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


static VOID
FillGrid(PMAP infoPtr,
         HDC hdc)
{
    HFONT hOldFont;
    TCHAR ch;
    INT x, y;

    hOldFont = SelectObject(hdc,
                            infoPtr->hFont);

    for (y = 0; y < YCELLS; y++)
    for (x = 0; x < XCELLS; x++)
    {
        ch = (TCHAR)((256 * infoPtr->iPage) + (XCELLS * y) + x);

        TagFontToCell(&infoPtr->Cells[y][x], ch);

        DrawText(hdc,
                 &ch,
                 1,
                 &infoPtr->Cells[y][x].CellInt,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc,
                 hOldFont);
}


static BOOL
CreateLargeCell(PMAP infoPtr)
{
    RECT rLarge;

    CopyRect(&rLarge,
             &infoPtr->pActiveCell->CellExt);

    MapWindowPoints(infoPtr->hMapWnd,
                    infoPtr->hParent,
                    (LPPOINT)&rLarge,
                    2);

    InflateRect(&rLarge,
                XLARGE - XCELLS,
                YLARGE - YCELLS);

    infoPtr->hLrgWnd = CreateWindowEx(0,
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


static VOID
MoveLargeCell(PMAP infoPtr)
{
    RECT rLarge;

    CopyRect(&rLarge,
             &infoPtr->pActiveCell->CellExt);

    MapWindowPoints(infoPtr->hMapWnd,
                    infoPtr->hParent,
                    (LPPOINT)&rLarge,
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


static VOID
SetFont(PMAP infoPtr,
        LPTSTR lpFontName)
{
    HDC hdc;

    if (infoPtr->hFont)
        DeleteObject(infoPtr->hFont);

    ZeroMemory(&infoPtr->CurrentFont,
               sizeof(LOGFONT));

    hdc = GetDC(infoPtr->hMapWnd);
    infoPtr->CurrentFont.lfHeight = GetDeviceCaps(hdc,
                                                  LOGPIXELSY) / 5;
    ReleaseDC(infoPtr->hMapWnd, hdc);

    infoPtr->CurrentFont.lfCharSet =  DEFAULT_CHARSET;
    lstrcpy(infoPtr->CurrentFont.lfFaceName,
            lpFontName);

    infoPtr->hFont = CreateFontIndirect(&infoPtr->CurrentFont);

    InvalidateRect(infoPtr->hMapWnd,
                   NULL,
                   TRUE);
}


static LRESULT
NotifyParentOfSelection(PMAP infoPtr,
                        UINT code,
                        TCHAR ch)
{
    LRESULT Ret = 0;

    if (infoPtr->hParent != NULL)
    {
        MAPNOTIFY mnmh;

        mnmh.hdr.hwndFrom = infoPtr->hMapWnd;
        mnmh.hdr.idFrom = GetWindowLongPtr(infoPtr->hMapWnd,
                                           GWLP_ID);
        mnmh.hdr.code = code;

        mnmh.ch = ch;

        Ret = SendMessage(infoPtr->hParent,
                          WM_NOTIFY,
                          (WPARAM)mnmh.hdr.idFrom,
                          (LPARAM)&mnmh);
    }

    return Ret;
}


static VOID
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


static BOOL
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
        SetWindowLongPtr(hwnd,
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


static VOID
OnVScroll(PMAP infoPtr,
          INT Value,
          INT Pos)
{
    switch (Value)
    {
        case SB_LINEUP:
            infoPtr->iPage -=  1;
            break;

        case SB_LINEDOWN:
            infoPtr->iPage +=  1;
            break;

        case SB_PAGEUP:
            infoPtr->iPage -= 16;
            break;

        case SB_PAGEDOWN:
            infoPtr->iPage += 16;
            break;

        case SB_THUMBPOSITION:
            infoPtr->iPage = Pos;
            break;

       default:
            break;
       }

    infoPtr->iPage = max(0,
                         min(infoPtr->iPage,
                             255));

    SetScrollPos(infoPtr->hMapWnd,
                 SB_VERT,
                 infoPtr->iPage,
                 TRUE);

    InvalidateRect(infoPtr->hMapWnd,
                   NULL,
                   TRUE);
}


static VOID
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


LRESULT CALLBACK
MapWndProc(HWND hwnd,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    PMAP infoPtr;
    LRESULT Ret = 0;

    infoPtr = (PMAP)GetWindowLongPtr(hwnd,
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
        {
            LPTSTR lpFontName = (LPTSTR)lParam;

            SetFont(infoPtr,
                    lpFontName);

            HeapFree(GetProcessHeap(),
                     0,
                     lpFontName);

            break;
        }

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
            SetWindowLongPtr(hwnd,
                             0,
                             (DWORD_PTR)NULL);
            break;
        }

        default:
        {
            Ret = DefWindowProc(hwnd,
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
    WNDCLASS wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MapWndProc;
    wc.cbWndExtra = sizeof(PMAP);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL,
                            (LPTSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = szMapWndClass;

    if (RegisterClass(&wc))
    {
        wc.lpfnWndProc = LrgCellWndProc;
        wc.cbWndExtra = 0;
        wc.lpszClassName = szLrgCellWndClass;

        return RegisterClass(&wc) != 0;
    }

    return FALSE;
}

VOID
UnregisterMapClasses(HINSTANCE hInstance)
{
    UnregisterClass(szMapWndClass,
                    hInstance);

    UnregisterClass(szLrgCellWndClass,
                    hInstance);
}
