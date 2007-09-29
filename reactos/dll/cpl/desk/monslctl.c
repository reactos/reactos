#include <windows.h>
#include "monslctl.h"

static const TCHAR szMonitorSelWndClass[] = TEXT("MONITORSELWNDCLASS");

typedef struct _MONSL_MON
{
    RECT rc;
} MONSL_MON, *PMONSL_MON;

typedef struct _MONITORSELWND
{
    HWND hSelf;
    HWND hNotify;
    HFONT hFont;
    SIZE ClientSize;
    DWORD UIState;
    union
    {
        DWORD dwFlags;
        struct
        {
            UINT Enabled : 1;
            UINT HasFocus : 1;
            UINT CanDisplay : 1;
        };
    };
    DWORD MonitorsCount;
    INT SelectedMonitor;
    PMONSL_MONINFO MonitorInfo;
    PMONSL_MON Monitors;
    RECT rcExtent;
    RECT rcMonitors;
    POINT ScrollPos;
    SIZE Margin;
    SIZE SelectionFrame;
} MONITORSELWND, *PMONITORSELWND;

static HFONT
MonSelChangeFont(IN OUT PMONITORSELWND infoPtr,
                 IN HFONT hFont,
                 IN BOOL Redraw)
{
    HFONT hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;

    if (Redraw)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }

    return hOldFont;
}

static VOID
MonSelRectToScreen(IN PMONITORSELWND infoPtr,
                   IN const RECT *prc,
                   OUT PRECT prcOnScreen)
{
    *prcOnScreen = *prc;
    OffsetRect(prcOnScreen,
               -infoPtr->ScrollPos.x,
               -infoPtr->ScrollPos.y);
}

static VOID
MonSelScreenToPt(IN PMONITORSELWND infoPtr,
                 IN const POINT *pptOnScreen,
                 OUT PPOINT ppt)
{
    ppt->x = pptOnScreen->x + infoPtr->ScrollPos.x;
    ppt->y = pptOnScreen->y + infoPtr->ScrollPos.y;
}

static VOID
MonSelMonInfoToRect(IN const MONSL_MONINFO *pMonInfo,
                    OUT PRECT prc)
{
    prc->left = pMonInfo->Position.x;
    prc->top = pMonInfo->Position.y;
    prc->right = pMonInfo->Position.x + pMonInfo->Size.cx;
    prc->bottom = pMonInfo->Position.y + pMonInfo->Size.cy;
}

static INT
MonSelHitTest(IN PMONITORSELWND infoPtr,
              IN const POINT *ppt)
{
    POINT pt;
    INT Index, Ret = -1;

    if (infoPtr->CanDisplay)
    {
        MonSelScreenToPt(infoPtr,
                         ppt,
                         &pt);

        for (Index = 0; Index < (INT)infoPtr->MonitorsCount; Index++)
        {
            if (PtInRect(&infoPtr->Monitors[Index].rc,
                         pt))
            {
                Ret = Index;
                break;
            }
        }
    }

    return Ret;
}

static VOID
MonSelUpdateExtent(IN OUT PMONITORSELWND infoPtr)
{
    DWORD Index;
    RECT rcMonitor;

    /* NOTE: This routine calculates the extent of all monitor coordinates.
             These are not control coordinates! */
    if (infoPtr->MonitorsCount > 0)
    {
        MonSelMonInfoToRect(&infoPtr->MonitorInfo[0],
                            &infoPtr->rcExtent);

        for (Index = 1; Index < infoPtr->MonitorsCount; Index++)
        {
            MonSelMonInfoToRect(&infoPtr->MonitorInfo[Index],
                                &rcMonitor);

            UnionRect(&infoPtr->rcExtent,
                      &infoPtr->rcExtent,
                      &rcMonitor);
        }
    }
    else
    {
        ZeroMemory(&infoPtr->rcExtent,
                   sizeof(infoPtr->rcExtent));
    }
}

static VOID
MonSelScaleRectRelative(IN const RECT *prcBaseFrom,
                        IN const RECT *prcFrom,
                        IN const RECT *prcBaseTo,
                        OUT PRECT prcTo)
{
    SIZE BaseFrom, BaseTo, From;

    BaseFrom.cx = prcBaseFrom->right - prcBaseFrom->left;
    BaseFrom.cy = prcBaseFrom->bottom - prcBaseFrom->top;
    BaseTo.cx = prcBaseTo->right - prcBaseTo->left;
    BaseTo.cy = prcBaseTo->bottom - prcBaseTo->top;
    From.cx = prcFrom->right - prcFrom->left;
    From.cy = prcFrom->bottom - prcFrom->top;

    prcTo->left = prcBaseTo->left + (((prcFrom->left - prcBaseFrom->left) * BaseTo.cx) / BaseFrom.cx);
    prcTo->top = prcBaseTo->top + (((prcFrom->top - prcBaseFrom->top) * BaseTo.cy) / BaseFrom.cy);
    prcTo->right = prcTo->left + ((From.cx * BaseTo.cx) / BaseFrom.cx);
    prcTo->bottom = prcTo->top + ((From.cy * BaseTo.cy) / BaseFrom.cy);
}

static VOID
ScaleRectSizeFit(IN const RECT *prcContainerRect,
                 IN OUT PRECT prcRectToScale)
{
    SIZE ContainerSize, RectSize;

    ContainerSize.cx = prcContainerRect->right - prcContainerRect->left;
    ContainerSize.cy = prcContainerRect->bottom - prcContainerRect->top;
    RectSize.cx = prcRectToScale->right - prcRectToScale->left;
    RectSize.cy = prcRectToScale->bottom - prcRectToScale->top;

    if (((RectSize.cx * 0xFFF) / RectSize.cy) < ((ContainerSize.cx * 0xFFF) / ContainerSize.cy))
    {
        RectSize.cx = (RectSize.cx * ((ContainerSize.cy * 0xFFF) / RectSize.cy)) / 0xFFF;
        RectSize.cy = ContainerSize.cy;
    }
    else
    {
        RectSize.cy = (RectSize.cy * ((ContainerSize.cx * 0xFFF) / RectSize.cx)) / 0xFFF;
        RectSize.cx = ContainerSize.cx;
    }

    prcRectToScale->right = prcRectToScale->left + RectSize.cx;
    prcRectToScale->bottom = prcRectToScale->top + RectSize.cy;

    OffsetRect(prcRectToScale,
               prcContainerRect->left + ((ContainerSize.cx - RectSize.cx) / 2),
               prcContainerRect->top + ((ContainerSize.cy - RectSize.cy) / 2));
}

static VOID
MonSelRepaint(IN PMONITORSELWND infoPtr)
{
    RECT rc;

    MonSelRectToScreen(infoPtr,
                       &infoPtr->rcMonitors,
                       &rc);
    InvalidateRect(infoPtr->hSelf,
                   &rc,
                   TRUE);
}

static VOID
MonSelRepaintMonitor(IN PMONITORSELWND infoPtr,
                     IN DWORD Index)
{
    RECT rc;

    if (Index < infoPtr->MonitorsCount)
    {
        MonSelRectToScreen(infoPtr,
                           &infoPtr->Monitors[Index].rc,
                           &rc);
        InvalidateRect(infoPtr->hSelf,
                       &rc,
                       TRUE);
    }
}

static VOID
MonSelRepaintSelected(IN PMONITORSELWND infoPtr)
{
    if (infoPtr->SelectedMonitor >= 0)
    {
        MonSelRepaintMonitor(infoPtr,
                             (DWORD)infoPtr->SelectedMonitor);
    }
}

static VOID
MonSelUpdateMonitorsInfo(IN OUT PMONITORSELWND infoPtr,
                         IN BOOL bRepaint)
{
    RECT rcExtSurface, rcExtDisplay;
    DWORD Index;

    /* Recalculate rcExtent */
    MonSelUpdateExtent(infoPtr);

    infoPtr-> CanDisplay = infoPtr->MonitorsCount != 0 &&
                           (infoPtr->ClientSize.cx > (2 * (infoPtr->Margin.cx + infoPtr->SelectionFrame.cx))) &&
                           (infoPtr->ClientSize.cy > (2 * (infoPtr->Margin.cy + infoPtr->SelectionFrame.cy)));

    if (infoPtr->CanDisplay)
    {
        /* Calculate the rectangle on the control in which may be painted */
        rcExtSurface.left = infoPtr->Margin.cx;
        rcExtSurface.top = infoPtr->Margin.cy;
        rcExtSurface.right = rcExtSurface.left + infoPtr->ClientSize.cx - (2 * infoPtr->Margin.cx);
        rcExtSurface.bottom = rcExtSurface.top + infoPtr->ClientSize.cy - (2 * infoPtr->Margin.cy);

        /* Calculate the rectangle on the control that is actually painted on */
        rcExtDisplay.left = rcExtDisplay.top = 0;
        rcExtDisplay.right = infoPtr->rcExtent.right - infoPtr->rcExtent.left;
        rcExtDisplay.bottom = infoPtr->rcExtent.bottom - infoPtr->rcExtent.top;

        ScaleRectSizeFit(&rcExtSurface,
                         &rcExtDisplay);

        infoPtr->rcMonitors = rcExtDisplay;

        /* Now that we know in which area all monitors are located,
           calculate the monitors selection rectangles on the screen */

        for (Index = 0; Index < infoPtr->MonitorsCount; Index++)
        {
            MonSelMonInfoToRect(&infoPtr->MonitorInfo[Index],
                                &rcExtDisplay);

            MonSelScaleRectRelative(&infoPtr->rcExtent,
                                    &rcExtDisplay,
                                    &infoPtr->rcMonitors,
                                    &infoPtr->Monitors[Index].rc);
        }

        if (bRepaint)
            MonSelRepaint(infoPtr);
    }
    else if (bRepaint)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }
}

static BOOL
MonSelSetMonitorInfo(IN OUT PMONITORSELWND infoPtr,
                     IN DWORD dwMonitors,
                     IN const MONSL_MONINFO *MonitorsInfo)
{
    DWORD dwOldCount;
    BOOL Ret = TRUE;

    dwOldCount = infoPtr->MonitorsCount;

    if (infoPtr->MonitorInfo != NULL)
    {
        LocalFree((HLOCAL)infoPtr->MonitorInfo);
        infoPtr->MonitorInfo = NULL;

        LocalFree((HLOCAL)infoPtr->Monitors);
        infoPtr->Monitors = NULL;

        infoPtr->MonitorsCount = 0;
    }

    if (dwMonitors != 0)
    {
        infoPtr->MonitorInfo = (PMONSL_MONINFO)LocalAlloc(LMEM_FIXED,
                                                          dwMonitors * sizeof(MONSL_MONINFO));
        if (infoPtr->MonitorInfo != NULL)
        {
            infoPtr->Monitors = (PMONSL_MON)LocalAlloc(LMEM_FIXED,
                                                       dwMonitors * sizeof(MONSL_MON));
            if (infoPtr->Monitors != NULL)
            {
                CopyMemory(infoPtr->MonitorInfo,
                           MonitorsInfo,
                           dwMonitors * sizeof(MONSL_MONINFO));
                ZeroMemory(infoPtr->Monitors,
                           dwMonitors * sizeof(MONSL_MON));

                infoPtr->MonitorsCount = dwMonitors;

                if (infoPtr->SelectedMonitor >= (INT)infoPtr->MonitorsCount)
                    infoPtr->SelectedMonitor = -1;
                infoPtr->SelectedMonitor = 0;

                MonSelUpdateMonitorsInfo(infoPtr,
                                         TRUE);
            }
            else
            {
                LocalFree((HLOCAL)infoPtr->MonitorInfo);
                infoPtr->MonitorInfo = NULL;

                Ret = FALSE;
            }
        }
        else
            Ret = FALSE;
    }

    if (!Ret)
        infoPtr->SelectedMonitor = -1;

    if (!Ret || dwMonitors == 0)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }

    return Ret;
}

static DWORD
MonSelGetMonitorInfo(IN PMONITORSELWND infoPtr,
                     IN DWORD dwMonitors,
                     IN OUT PMONSL_MONINFO MonitorsInfo)
{
    if (dwMonitors != 0)
    {
        if (dwMonitors > infoPtr->MonitorsCount)
            dwMonitors = infoPtr->MonitorsCount;

        CopyMemory(MonitorsInfo,
                   infoPtr->MonitorInfo,
                   dwMonitors * sizeof(MONSL_MONINFO));
        return dwMonitors;
    }
    else
        return infoPtr->MonitorsCount;
}

static BOOL
MonSelSetCurSelMonitor(IN OUT PMONITORSELWND infoPtr,
                       IN INT Index)
{
    INT PrevSel;
    BOOL Ret = FALSE;

    if (Index == -1 || Index < (INT)infoPtr->MonitorsCount)
    {
        if (Index != infoPtr->SelectedMonitor)
        {
            PrevSel = infoPtr->SelectedMonitor;
            infoPtr->SelectedMonitor = Index;

            if (PrevSel >= 0)
            {
                MonSelRepaintMonitor(infoPtr,
                                     PrevSel);
            }

            if (infoPtr->SelectedMonitor >= 0)
                MonSelRepaintSelected(infoPtr);
        }

        Ret = TRUE;
    }

    return Ret;
}

static VOID
MonSelCreate(IN OUT PMONITORSELWND infoPtr)
{
    infoPtr->SelectionFrame.cx = infoPtr->SelectionFrame.cy = 4;
    infoPtr->Margin.cx = infoPtr->Margin.cy = 20;
    infoPtr->SelectedMonitor = -1;
    return;
}

static VOID
MonSelDestroy(IN OUT PMONITORSELWND infoPtr)
{
    /* Free all monitors */
    MonSelSetMonitorInfo(infoPtr,
                         0,
                         NULL);
}

static VOID
MonSelPaint(IN PMONITORSELWND infoPtr,
            IN HDC hDC,
            IN LPRECT prcUpdate)
{
    HBRUSH hbBk, hbOldBk;
    HPEN hpFg, hpOldFg;
    DWORD Index;
    RECT rc;

    hbBk = GetSysColorBrush(COLOR_BACKGROUND);
    hpFg = CreatePen(PS_SOLID,
                     0,
                     GetSysColor(COLOR_WINDOW));

    hbOldBk = SelectObject(hDC,
                           hbBk);
    hpOldFg = SelectObject(hDC,
                           hpFg);

    for (Index = 0; Index < infoPtr->MonitorsCount; Index++)
    {
        MonSelRectToScreen(infoPtr,
                           &infoPtr->Monitors[Index].rc,
                           &rc);

        if ((INT)Index == infoPtr->SelectedMonitor)
        {
            FillRect(hDC,
                     &rc,
                     (HBRUSH)(COLOR_HIGHLIGHT + 1));

            if (infoPtr->HasFocus && !(infoPtr->UIState & UISF_HIDEFOCUS))
            {
                DrawFocusRect(hDC,
                              &rc);
            }
        }

        InflateRect(&rc,
                    -infoPtr->SelectionFrame.cx,
                    -infoPtr->SelectionFrame.cy);

        Rectangle(hDC,
                  rc.left,
                  rc.top,
                  rc.right,
                  rc.bottom);
    }

    SelectObject(hDC,
                 hpOldFg);
    SelectObject(hDC,
                 hbOldBk);
}

static LRESULT CALLBACK
MonitorSelWndProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    PMONITORSELWND infoPtr;
    LRESULT Ret = 0;

    infoPtr = (PMONITORSELWND)GetWindowLongPtrW(hwnd,
                                                0);

    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_PAINT:
        case WM_PRINTCLIENT:
        {
            PAINTSTRUCT ps;
            HDC hDC;

            if (wParam != 0)
            {
                if (!GetUpdateRect(hwnd,
                                   &ps.rcPaint,
                                   TRUE))
                {
                    break;
                }
                hDC = (HDC)wParam;
            }
            else
            {
                hDC = BeginPaint(hwnd,
                                 &ps);
                if (hDC == NULL)
                {
                    break;
                }
            }

            if (infoPtr->CanDisplay)
            {
                MonSelPaint(infoPtr,
                            hDC,
                            &ps.rcPaint);
            }

            if (wParam == 0)
            {
                EndPaint(hwnd,
                         &ps);
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            INT Index;
            POINT pt;

            pt.x = (LONG)LOWORD(lParam);
            pt.y = (LONG)HIWORD(lParam);

            Index = MonSelHitTest(infoPtr,
                                  &pt);
            if (Index >= 0)
            {
                MonSelSetCurSelMonitor(infoPtr,
                                       Index);
            }

            /* fall through */
        }

        case WM_MBUTTONDOWN:
        {
            if (!infoPtr->HasFocus)
                SetFocus(hwnd);
            break;
        }

        case WM_GETDLGCODE:
        {
            INT virtKey;

            virtKey = (lParam != 0 ? (INT)((LPMSG)lParam)->wParam : 0);
            switch (virtKey)
            {
                case VK_TAB:
                {
                    /* change the UI status */
                    SendMessage(GetAncestor(hwnd,
                                            GA_PARENT),
                                WM_CHANGEUISTATE,
                                MAKEWPARAM(UIS_INITIALIZE,
                                           0),
                                0);
                    break;
                }
            }

            Ret |= DLGC_WANTARROWS;
            break;
        }

        case WM_SETFOCUS:
        {
            infoPtr->HasFocus = TRUE;
            MonSelRepaintSelected(infoPtr);
            break;
        }

        case WM_KILLFOCUS:
        {
            infoPtr->HasFocus = FALSE;
            MonSelRepaintSelected(infoPtr);
            break;
        }

        case WM_UPDATEUISTATE:
        {
            DWORD OldUIState = infoPtr->UIState;
            switch (LOWORD(wParam))
            {
                case UIS_SET:
                    infoPtr->UIState |= HIWORD(wParam);
                    break;

                case UIS_CLEAR:
                    infoPtr->UIState &= ~HIWORD(wParam);
                    break;
            }

            if (infoPtr->UIState != OldUIState)
                MonSelRepaintSelected(infoPtr);
            break;
        }

        case WM_SETFONT:
        {
            Ret = (LRESULT)MonSelChangeFont(infoPtr,
                                            (HFONT)wParam,
                                            (BOOL)LOWORD(lParam));
            break;
        }

        case WM_SIZE:
        {
            infoPtr->ClientSize.cx = LOWORD(lParam);
            infoPtr->ClientSize.cy = HIWORD(lParam);

            /* Don't let MonSelUpdateMonitorsInfo repaint the control
               because this won't work properly in case the control
               was sized down! */
            MonSelUpdateMonitorsInfo(infoPtr,
                                     FALSE);
            InvalidateRect(infoPtr->hSelf,
                           NULL,
                           TRUE);
            break;
        }

        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }

        case WM_ENABLE:
        {
            infoPtr->Enabled = ((BOOL)wParam != FALSE);
            /* FIXME */
            break;
        }

        case WM_STYLECHANGED:
        {
            if (wParam == GWL_STYLE)
            {
                unsigned int OldEnabled = infoPtr->Enabled;
                infoPtr->Enabled = !(((LPSTYLESTRUCT)lParam)->styleNew & WS_DISABLED);

                if (OldEnabled != infoPtr->Enabled)
                {
                    /* FIXME */
                }
            }
            break;
        }

        case MSLM_SETMONITORINFO:
        {
            Ret = MonSelSetMonitorInfo(infoPtr,
                                       (DWORD)wParam,
                                       (const MONSL_MONINFO *)lParam);
            break;
        }

        case MSLM_GETMONITORINFO:
        {
            Ret = MonSelGetMonitorInfo(infoPtr,
                                       (DWORD)wParam,
                                       (PMONSL_MONINFO)lParam);
            break;
        }

        case MSLM_GETMONITORINFOCOUNT:
        {
            Ret = infoPtr->MonitorsCount;
            break;
        }

        case MSLM_HITTEST:
        {
            Ret = MonSelHitTest(infoPtr,
                                (const POINT *)wParam);
            break;
        }

        case WM_CREATE:
        {
            infoPtr = (PMONITORSELWND) HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 sizeof(MONITORSELWND));
            if (infoPtr == NULL)
            {
                Ret = (LRESULT)-1;
                break;
            }

            ZeroMemory(infoPtr,
                       sizeof(MONITORSELWND));
            infoPtr->hSelf = hwnd;
            infoPtr->hNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
            infoPtr->Enabled = !(((LPCREATESTRUCTW)lParam)->style & WS_DISABLED);
            infoPtr->UIState = SendMessage(hwnd,
                                           WM_QUERYUISTATE,
                                           0,
                                           0);

            SetWindowLongPtrW(hwnd,
                              0,
                              (LONG_PTR)infoPtr);

            MonSelCreate(infoPtr);
            break;
        }

        case WM_DESTROY:
        {
            MonSelDestroy(infoPtr);

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
HandleDefaultMessage:
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
RegisterMonitorSelectionControl(IN HINSTANCE hInstance)
{
    WNDCLASS wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MonitorSelWndProc;
    wc.cbWndExtra = sizeof(PMONITORSELWND);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL,
                             (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszClassName = szMonitorSelWndClass;

    return RegisterClass(&wc) != 0;
}

VOID
UnregisterMonitorSelectionControl(IN HINSTANCE hInstance)
{
    UnregisterClassW(szMonitorSelWndClass,
                     hInstance);
}
