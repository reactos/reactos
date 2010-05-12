#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "monslctl.h"

static const TCHAR szMonitorSelWndClass[] = TEXT("MONITORSELWNDCLASS");

typedef struct _MONSL_MON
{
    RECT rc;
    HFONT hFont;
    TCHAR szCaption[12];
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
        DWORD dwInternalFlags;
        struct
        {
            UINT Enabled : 1;
            UINT HasFocus : 1;
            UINT CanDisplay : 1;
            UINT LeftBtnDown : 1;
            UINT IsDraggingMonitor : 1;
        };
    };
    DWORD ControlExStyle;
    DWORD MonitorsCount;
    INT SelectedMonitor;
    INT DraggingMonitor;
    RECT rcDragging;
    POINT ptDrag, ptDragBegin;
    SIZE DraggingMargin;
    PMONSL_MONINFO MonitorInfo;
    PMONSL_MON Monitors;
    RECT rcExtent;
    RECT rcMonitors;
    POINT ScrollPos;
    SIZE Margin;
    SIZE SelectionFrame;
    HBITMAP hbmDisabledPattern;
    HBRUSH hbrDisabled;
} MONITORSELWND, *PMONITORSELWND;

static LRESULT
MonSelNotify(IN PMONITORSELWND infoPtr,
             IN UINT code,
             IN OUT PVOID data)
{
    LRESULT Ret = 0;

    if (infoPtr->hNotify != NULL)
    {
        LPNMHDR pnmh = (LPNMHDR)data;

        pnmh->hwndFrom = infoPtr->hSelf;
        pnmh->idFrom = GetWindowLongPtr(infoPtr->hSelf,
                                        GWLP_ID);
        pnmh->code = code;

        Ret = SendMessage(infoPtr->hNotify,
                          WM_NOTIFY,
                          (WPARAM)pnmh->idFrom,
                          (LPARAM)pnmh);
    }

    return Ret;
}

static LRESULT
MonSelNotifyMonitor(IN PMONITORSELWND infoPtr,
                    IN UINT code,
                    IN INT Index,
                    IN OUT PMONSL_MONNMHDR pmonnmh)
{
    pmonnmh->Index = Index;

    if (Index >= 0)
    {
        pmonnmh->MonitorInfo = infoPtr->MonitorInfo[Index];
    }
    else
    {
        ZeroMemory(&pmonnmh->MonitorInfo,
                   sizeof(pmonnmh->MonitorInfo));
    }

    return MonSelNotify(infoPtr,
                        code,
                        pmonnmh);
}

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
    BOOL NoRepaint = FALSE;

    if (Index < infoPtr->MonitorsCount)
    {
        if (Index == (DWORD)infoPtr->DraggingMonitor)
        {
            if (infoPtr->IsDraggingMonitor)
            {
                MonSelRectToScreen(infoPtr,
                                   &infoPtr->rcDragging,
                                   &rc);
            }
            else
                NoRepaint = TRUE;
        }
        else
        {
            MonSelRectToScreen(infoPtr,
                               &infoPtr->Monitors[Index].rc,
                               &rc);
        }

        if (!NoRepaint)
        {
            InvalidateRect(infoPtr->hSelf,
                           &rc,
                           TRUE);
        }
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
MonSelResetMonitors(IN OUT PMONITORSELWND infoPtr)
{
    DWORD Index;

    for (Index = 0; Index < infoPtr->MonitorsCount; Index++)
    {
        if (infoPtr->Monitors[Index].hFont != NULL)
        {
            DeleteObject(infoPtr->Monitors[Index].hFont);
            infoPtr->Monitors[Index].hFont = NULL;
        }
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

        MonSelResetMonitors(infoPtr);

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
MonSelSetMonitorsInfo(IN OUT PMONITORSELWND infoPtr,
                      IN DWORD dwMonitors,
                      IN const MONSL_MONINFO *MonitorsInfo)
{
    DWORD Index;
    BOOL Ret = TRUE;

    if (infoPtr->DraggingMonitor >= 0)
        return FALSE;

    if (infoPtr->MonitorInfo != NULL)
    {
        LocalFree((HLOCAL)infoPtr->MonitorInfo);
        infoPtr->MonitorInfo = NULL;

        MonSelResetMonitors(infoPtr);

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

                for (Index = 0; Index < dwMonitors; Index++)
                {
                    _stprintf(infoPtr->Monitors[Index].szCaption,
                              _T("%u"),
                              Index + 1);
                }

                infoPtr->MonitorsCount = dwMonitors;

                if (infoPtr->SelectedMonitor >= (INT)infoPtr->MonitorsCount)
                    infoPtr->SelectedMonitor = -1;

                if (!(infoPtr->ControlExStyle & MSLM_EX_ALLOWSELECTNONE) && infoPtr->SelectedMonitor < 0)
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
MonSelGetMonitorsInfo(IN PMONITORSELWND infoPtr,
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
MonSelSetMonitorInfo(IN OUT PMONITORSELWND infoPtr,
                     IN INT Index,
                     IN const MONSL_MONINFO *MonitorsInfo)
{
    if (infoPtr->DraggingMonitor < 0 &&
        Index >= 0 && Index < (INT)infoPtr->MonitorsCount)
    {
        CopyMemory(&infoPtr->MonitorInfo[Index],
                   MonitorsInfo,
                   sizeof(MONSL_MONINFO));

        MonSelUpdateMonitorsInfo(infoPtr,
                                 TRUE);
        return TRUE;
    }

    return FALSE;
}

static BOOL
MonSelGetMonitorInfo(IN PMONITORSELWND infoPtr,
                     IN INT Index,
                     IN OUT PMONSL_MONINFO MonitorsInfo)
{
    if (Index >= 0 && Index < (INT)infoPtr->MonitorsCount)
    {
        CopyMemory(MonitorsInfo,
                   &infoPtr->MonitorInfo[Index],
                   sizeof(MONSL_MONINFO));
        return TRUE;
    }

    return FALSE;
}

static INT
MonSelGetMonitorRect(IN OUT PMONITORSELWND infoPtr,
                     IN INT Index,
                     OUT PRECT prc)
{
    RECT rc, rcClient;

    if (Index < 0 || Index >= infoPtr->MonitorsCount)
        return -1;

    if (!infoPtr->CanDisplay)
        return 0;

    MonSelRectToScreen(infoPtr,
                       &infoPtr->Monitors[Index].rc,
                       prc);

    rcClient.left = rcClient.top = 0;
    rcClient.right = infoPtr->ClientSize.cx;
    rcClient.bottom = infoPtr->ClientSize.cy;

    return IntersectRect(&rc,
                         &rcClient,
                         prc) != FALSE;
}

static BOOL
MonSelSetCurSelMonitor(IN OUT PMONITORSELWND infoPtr,
                       IN INT Index,
                       IN BOOL bNotify)
{
    INT PrevSel;
    BOOL PreventSelect = FALSE;
    BOOL Ret = FALSE;

    if (infoPtr->DraggingMonitor < 0 &&
        (Index == -1 || Index < (INT)infoPtr->MonitorsCount))
    {
        if (Index != infoPtr->SelectedMonitor)
        {
            if ((infoPtr->MonitorInfo[Index].Flags & MSL_MIF_DISABLED) &&
                !(infoPtr->ControlExStyle & MSLM_EX_ALLOWSELECTDISABLED))
            {
                PreventSelect = TRUE;
            }

            if (!PreventSelect && bNotify)
            {
                MONSL_MONNMMONITORCHANGING nmi;

                nmi.PreviousSelected = infoPtr->SelectedMonitor;
                nmi.AllowChanging = TRUE;

                MonSelNotifyMonitor(infoPtr,
                                    MSLN_MONITORCHANGING,
                                    Index,
                                    &nmi.hdr);

                PreventSelect = (nmi.AllowChanging == FALSE);
            }

            if (!PreventSelect)
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

                if (bNotify)
                {
                    MONSL_MONNMHDR nm;

                    MonSelNotifyMonitor(infoPtr,
                                        MSLN_MONITORCHANGED,
                                        Index,
                                        &nm);
                }
            }
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
    infoPtr->DraggingMonitor = -1;
    infoPtr->ControlExStyle = MSLM_EX_ALLOWSELECTDISABLED | MSLM_EX_HIDENUMBERONSINGLE |
        MSLM_EX_SELECTONRIGHTCLICK | MSLM_EX_SELECTBYARROWKEY;
    return;
}

static VOID
MonSelDestroy(IN OUT PMONITORSELWND infoPtr)
{
    /* Free all monitors */
    MonSelSetMonitorsInfo(infoPtr,
                          0,
                          NULL);

    if (infoPtr->hbrDisabled != NULL)
    {
        DeleteObject(infoPtr->hbrDisabled);
        infoPtr->hbrDisabled = NULL;
    }

    if (infoPtr->hbmDisabledPattern != NULL)
    {
        DeleteObject(infoPtr->hbmDisabledPattern);
        infoPtr->hbmDisabledPattern = NULL;
    }
}

static BOOL
MonSelSetExtendedStyle(IN OUT PMONITORSELWND infoPtr,
                       IN DWORD dwExtendedStyle)
{
    if (infoPtr->DraggingMonitor >= 0)
        return FALSE;

    if (dwExtendedStyle != infoPtr->ControlExStyle)
    {
        infoPtr->ControlExStyle = dwExtendedStyle;

        /* Repaint the control */
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }

    return TRUE;
}

static DWORD
MonSelGetExtendedStyle(IN PMONITORSELWND infoPtr)
{
    return infoPtr->ControlExStyle;
}

static HFONT
MonSelGetMonitorFont(IN OUT PMONITORSELWND infoPtr,
                     IN HDC hDC,
                     IN INT Index)
{
    TEXTMETRIC tm;
    SIZE rcsize;
    LOGFONT lf;
    HFONT hPrevFont, hFont;
    INT len;

    hFont = infoPtr->Monitors[Index].hFont;
    if (hFont == NULL &&
        GetObject(infoPtr->hFont,
                  sizeof(LOGFONT),
                  &lf) != 0)
    {
        rcsize.cx = infoPtr->Monitors[Index].rc.right - infoPtr->Monitors[Index].rc.left -
                    (2 * infoPtr->SelectionFrame.cx) - 2;
        rcsize.cy = infoPtr->Monitors[Index].rc.bottom - infoPtr->Monitors[Index].rc.top -
                    (2 * infoPtr->SelectionFrame.cy) - 2;
        rcsize.cy = (rcsize.cy * 60) / 100;

        len = _tcslen(infoPtr->Monitors[Index].szCaption);

        hPrevFont = SelectObject(hDC,
                                 infoPtr->hFont);

        if (GetTextMetrics(hDC,
                           &tm))
        {
            lf.lfWeight = FW_SEMIBOLD;
            lf.lfHeight = -MulDiv(rcsize.cy - tm.tmExternalLeading,
                                  GetDeviceCaps(hDC,
                                                LOGPIXELSY),
                                  72);

            hFont = CreateFontIndirect(&lf);
            if (hFont != NULL)
                infoPtr->Monitors[Index].hFont = hFont;
        }

        SelectObject(hDC,
                     hPrevFont);
    }

    return hFont;
}

static BOOL
MonSelDrawDisabledRect(IN OUT PMONITORSELWND infoPtr,
                       IN HDC hDC,
                       IN const RECT *prc)
{
    BOOL Ret = FALSE;

    if (infoPtr->hbrDisabled == NULL)
    {
        static const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};

        if (infoPtr->hbmDisabledPattern == NULL)
        {
            infoPtr->hbmDisabledPattern = CreateBitmap(8,
                                                       8,
                                                       1,
                                                       1,
                                                       Pattern);
        }

        if (infoPtr->hbmDisabledPattern != NULL)
            infoPtr->hbrDisabled = CreatePatternBrush(infoPtr->hbmDisabledPattern);
    }

    if (infoPtr->hbrDisabled != NULL)
    {
        /* FIXME - implement */
    }

    return Ret;
}

static VOID
MonSelPaintMonitor(IN OUT PMONITORSELWND infoPtr,
                   IN HDC hDC,
                   IN DWORD Index,
                   IN OUT PRECT prc,
                   IN COLORREF crDefFontColor,
                   IN BOOL bHideNumber)
{
    HFONT hFont, hPrevFont;
    COLORREF crPrevText;

    if ((INT)Index == infoPtr->SelectedMonitor)
    {
        FillRect(hDC,
                 prc,
                 (HBRUSH)(COLOR_HIGHLIGHT + 1));

        if (infoPtr->HasFocus && !(infoPtr->UIState & UISF_HIDEFOCUS))
        {
            /* NOTE: We need to switch the text color to the default, because
                     DrawFocusRect draws a solid line if the text is white! */

            crPrevText = SetTextColor(hDC,
                                      crDefFontColor);

            DrawFocusRect(hDC,
                          prc);

            SetTextColor(hDC,
                         crPrevText);
        }
    }

    InflateRect(prc,
                -infoPtr->SelectionFrame.cx,
                -infoPtr->SelectionFrame.cy);

    Rectangle(hDC,
              prc->left,
              prc->top,
              prc->right,
              prc->bottom);

    InflateRect(prc,
                -1,
                -1);

    if (!bHideNumber)
    {
        hFont = MonSelGetMonitorFont(infoPtr,
                                     hDC,
                                     Index);
        if (hFont != NULL)
        {
            hPrevFont = SelectObject(hDC,
                                     hFont);

            DrawText(hDC,
                     infoPtr->Monitors[Index].szCaption,
                     -1,
                     prc,
                     DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);

            SelectObject(hDC,
                         hPrevFont);
        }
    }

    if (infoPtr->MonitorInfo[Index].Flags & MSL_MIF_DISABLED)
    {
        InflateRect(prc,
                    1,
                    1);

        MonSelDrawDisabledRect(infoPtr,
                               hDC,
                               prc);
    }
}

static VOID
MonSelPaint(IN OUT PMONITORSELWND infoPtr,
            IN HDC hDC,
            IN const RECT *prcUpdate)
{
    COLORREF crPrevText;
    HBRUSH hbBk, hbOldBk;
    HPEN hpFg, hpOldFg;
    DWORD Index;
    RECT rc, rctmp;
    INT iPrevBkMode;
    BOOL bHideNumber;

    bHideNumber = (infoPtr->ControlExStyle & MSLM_EX_HIDENUMBERS) ||
        ((infoPtr->MonitorsCount == 1) && (infoPtr->ControlExStyle & MSLM_EX_HIDENUMBERONSINGLE));

    hbBk = GetSysColorBrush(COLOR_BACKGROUND);
    hpFg = CreatePen(PS_SOLID,
                     0,
                     GetSysColor(COLOR_HIGHLIGHTTEXT));

    hbOldBk = SelectObject(hDC,
                           hbBk);
    hpOldFg = SelectObject(hDC,
                           hpFg);
    iPrevBkMode = SetBkMode(hDC,
                            TRANSPARENT);
    crPrevText = SetTextColor(hDC,
                              GetSysColor(COLOR_HIGHLIGHTTEXT));

    for (Index = 0; Index < infoPtr->MonitorsCount; Index++)
    {
        if (infoPtr->IsDraggingMonitor &&
            (DWORD)infoPtr->DraggingMonitor == Index)
        {
            continue;
        }

        MonSelRectToScreen(infoPtr,
                           &infoPtr->Monitors[Index].rc,
                           &rc);

        if (IntersectRect(&rctmp,
                          &rc,
                          prcUpdate))
        {
            MonSelPaintMonitor(infoPtr,
                               hDC,
                               Index,
                               &rc,
                               crPrevText,
                               bHideNumber);
        }
    }

    /* Paint the dragging monitor last */
    if (infoPtr->IsDraggingMonitor &&
        infoPtr->DraggingMonitor >= 0)
    {
        MonSelRectToScreen(infoPtr,
                           &infoPtr->rcDragging,
                           &rc);

        if (IntersectRect(&rctmp,
                          &rc,
                          prcUpdate))
        {
            MonSelPaintMonitor(infoPtr,
                               hDC,
                               (DWORD)infoPtr->DraggingMonitor,
                               &rc,
                               crPrevText,
                               bHideNumber);
        }
    }

    SetTextColor(hDC,
                 crPrevText);
    SetBkMode(hDC,
              iPrevBkMode);
    SelectObject(hDC,
                 hpOldFg);
    SelectObject(hDC,
                 hbOldBk);

    DeleteObject(hpFg);
}

static VOID
MonSelContextMenu(IN OUT PMONITORSELWND infoPtr,
                  IN SHORT x,
                  IN SHORT y)
{
    MONSL_MONNMBUTTONCLICKED nm;
    INT Index;

    if (!infoPtr->HasFocus)
        SetFocus(infoPtr->hSelf);

    nm.pt.x = x;
    nm.pt.y = y;

    Index = MonSelHitTest(infoPtr,
                          &nm.pt);

    MonSelNotifyMonitor(infoPtr,
                        MSLN_RBUTTONUP,
                        Index,
                        (PMONSL_MONNMHDR)&nm);

    /* Send a WM_CONTEXTMENU notification */
    MapWindowPoints(infoPtr->hSelf,
                    NULL,
                    &nm.pt,
                    1);

    SendMessage(infoPtr->hSelf,
                WM_CONTEXTMENU,
                (WPARAM)infoPtr->hSelf,
                MAKELPARAM(nm.pt.x,
                           nm.pt.y));
}

static VOID
MonSelApplyCursorClipping(IN PMONITORSELWND infoPtr,
                          IN BOOL bClip)
{
    RECT rc;

    if (bClip)
    {
        rc.left = rc.top = 0;
        rc.right = infoPtr->ClientSize.cx;
        rc.bottom = infoPtr->ClientSize.cy;

        if (MapWindowPoints(infoPtr->hSelf,
                            NULL,
                            (LPPOINT)&rc,
                            2))
        {
            ClipCursor(&rc);
        }
    }
    else
    {
        ClipCursor(NULL);
    }
}

static VOID
MonSelMoveDragRect(IN OUT PMONITORSELWND infoPtr,
                   IN PPOINT ppt)
{
    RECT rcPrev, rcUpdate, *prc;
    HRGN hRgnPrev;
    HDC hDC;

    if (infoPtr->CanDisplay)
    {
        hDC = GetDC(infoPtr->hSelf);
        if (hDC != NULL)
        {
            if (infoPtr->ptDrag.x != ppt->x ||
                infoPtr->ptDrag.y != ppt->y)
            {
                infoPtr->ptDrag = *ppt;

                rcPrev = infoPtr->rcDragging;

                /* Calculate updated dragging rectangle */
                prc = &infoPtr->Monitors[infoPtr->DraggingMonitor].rc;
                infoPtr->rcDragging.left = ppt->x - infoPtr->DraggingMargin.cx;
                infoPtr->rcDragging.top = ppt->y - infoPtr->DraggingMargin.cy;
                infoPtr->rcDragging.right = infoPtr->rcDragging.left + (prc->right - prc->left);
                infoPtr->rcDragging.bottom = infoPtr->rcDragging.top + (prc->bottom - prc->top);

                hRgnPrev = CreateRectRgn(rcPrev.left,
                                         rcPrev.top,
                                         rcPrev.right,
                                         rcPrev.bottom);

                if (hRgnPrev != NULL)
                {
                    if (!ScrollDC(hDC,
                                  infoPtr->rcDragging.left - rcPrev.left,
                                  infoPtr->rcDragging.top - rcPrev.top,
                                  &rcPrev,
                                  NULL,
                                  hRgnPrev,
                                  &rcUpdate) ||
                        !InvalidateRgn(infoPtr->hSelf,
                                       hRgnPrev,
                                       TRUE))
                    {
                        DeleteObject(hRgnPrev);
                        goto InvRects;
                    }

                    DeleteObject(hRgnPrev);
                }
                else
                {
InvRects:
                    InvalidateRect(infoPtr->hSelf,
                                   &rcPrev,
                                   TRUE);
                    InvalidateRect(infoPtr->hSelf,
                                   &infoPtr->rcDragging,
                                   TRUE);
                }
            }

            ReleaseDC(infoPtr->hSelf,
                      hDC);
        }
    }
}

static VOID
MonSelCancelDragging(IN OUT PMONITORSELWND infoPtr)
{
    DWORD Index;

    if (infoPtr->DraggingMonitor >= 0)
    {
        MonSelMoveDragRect(infoPtr,
                           &infoPtr->ptDragBegin);

        Index = (DWORD)infoPtr->DraggingMonitor;
        infoPtr->DraggingMonitor = -1;

        if (infoPtr->CanDisplay)
        {
            /* Repaint the area where the monitor was last dragged */
            MonSelRepaintMonitor(infoPtr,
                                 Index);

            infoPtr->IsDraggingMonitor = FALSE;

            /* Repaint the area where the monitor is located */
            MonSelRepaintMonitor(infoPtr,
                                 Index);
        }
        else
            infoPtr->IsDraggingMonitor = FALSE;

        ReleaseCapture();

        MonSelApplyCursorClipping(infoPtr,
                                  FALSE);
    }
}

static VOID
MonSelInitDragging(IN OUT PMONITORSELWND infoPtr,
                   IN DWORD Index,
                   IN PPOINT ppt)
{
    POINT pt;

    MonSelCancelDragging(infoPtr);
    infoPtr->IsDraggingMonitor = FALSE;

    MonSelScreenToPt(infoPtr,
                     ppt,
                     &pt);

    infoPtr->ptDrag = infoPtr->ptDragBegin = pt;
    infoPtr->DraggingMonitor = (INT)Index;

    infoPtr->DraggingMargin.cx = ppt->x - infoPtr->Monitors[Index].rc.left;
    infoPtr->DraggingMargin.cy = ppt->y - infoPtr->Monitors[Index].rc.top;
    infoPtr->rcDragging = infoPtr->Monitors[Index].rc;

    MonSelApplyCursorClipping(infoPtr,
                              TRUE);
}

static VOID
MonSelDrag(IN OUT PMONITORSELWND infoPtr,
           IN PPOINT ppt)
{
    SIZE szDrag;
    POINT pt;
    RECT rcDrag;

    if (infoPtr->DraggingMonitor >= 0)
    {
        MonSelScreenToPt(infoPtr,
                         ppt,
                         &pt);

        if (!infoPtr->IsDraggingMonitor)
        {
            szDrag.cx = GetSystemMetrics(SM_CXDRAG);
            szDrag.cy = GetSystemMetrics(SM_CYDRAG);

            rcDrag.left = infoPtr->Monitors[infoPtr->DraggingMonitor].rc.left + infoPtr->DraggingMargin.cx - (szDrag.cx / 2);
            rcDrag.top = infoPtr->Monitors[infoPtr->DraggingMonitor].rc.top + infoPtr->DraggingMargin.cy - (szDrag.cy / 2);
            rcDrag.right = rcDrag.left + szDrag.cx;
            rcDrag.bottom = rcDrag.top + szDrag.cy;

            if (!PtInRect(&rcDrag,
                          pt))
            {
                /* The user started moving around the mouse: Begin dragging */
                infoPtr->IsDraggingMonitor = TRUE;
                MonSelMoveDragRect(infoPtr,
                                   &pt);
            }
        }
        else
        {
            MonSelMoveDragRect(infoPtr,
                               &pt);
        }
    }
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

        case WM_MOUSEMOVE:
        {
            POINT pt;

            if (!(wParam & MK_LBUTTON))
            {
                MonSelCancelDragging(infoPtr);
                break;
            }

            if (infoPtr->LeftBtnDown)
            {
                pt.x = (LONG)LOWORD(lParam);
                pt.y = (LONG)HIWORD(lParam);

                MonSelDrag(infoPtr,
                           &pt);
            }

            break;
        }

        case WM_RBUTTONDOWN:
        {
            if (!(infoPtr->ControlExStyle & MSLM_EX_SELECTONRIGHTCLICK))
                break;

            /* fall through */
        }

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        {
            INT Index;
            POINT pt;

            if (!infoPtr->HasFocus)
                SetFocus(infoPtr->hSelf);

            pt.x = (LONG)LOWORD(lParam);
            pt.y = (LONG)HIWORD(lParam);

            Index = MonSelHitTest(infoPtr,
                                  &pt);
            if (Index >= 0 || (infoPtr->ControlExStyle & MSLM_EX_ALLOWSELECTNONE))
            {
                MonSelSetCurSelMonitor(infoPtr,
                                       Index,
                                       TRUE);
            }

            if (Index >= 0 && (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK))
            {
                infoPtr->LeftBtnDown = TRUE;
                MonSelInitDragging(infoPtr,
                                   (DWORD)Index,
                                   &pt);
            }

            /* fall through */
        }

        case WM_MBUTTONDOWN:
        {
            if (!infoPtr->HasFocus)
                SetFocus(hwnd);
            break;
        }

        case WM_RBUTTONUP:
        {
            MonSelContextMenu(infoPtr,
                              (SHORT)LOWORD(lParam),
                              (SHORT)HIWORD(lParam));
            break;
        }

        case WM_LBUTTONUP:
        {
            MonSelCancelDragging(infoPtr);
            infoPtr->LeftBtnDown = FALSE;
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

            if (infoPtr->ControlExStyle & MSLM_EX_SELECTBYNUMKEY)
                Ret |= DLGC_WANTCHARS;
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
            MonSelCancelDragging(infoPtr);
            MonSelRepaintSelected(infoPtr);
            break;
        }

        case WM_UPDATEUISTATE:
        {
            DWORD OldUIState;

            Ret = DefWindowProcW(hwnd,
                                 uMsg,
                                 wParam,
                                 lParam);

            OldUIState = infoPtr->UIState;
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
            MonSelRepaint(infoPtr);
            break;
        }

        case WM_STYLECHANGED:
        {
            if (wParam == GWL_STYLE)
            {
                unsigned int OldEnabled = infoPtr->Enabled;
                infoPtr->Enabled = !(((LPSTYLESTRUCT)lParam)->styleNew & WS_DISABLED);

                if (OldEnabled != infoPtr->Enabled)
                    MonSelRepaint(infoPtr);
            }
            break;
        }

        case WM_KEYDOWN:
        {
            INT Index;

            if (infoPtr->ControlExStyle & MSLM_EX_SELECTBYARROWKEY)
            {
                switch (wParam)
                {
                    case VK_UP:
                    case VK_LEFT:
                    {
                        Index = infoPtr->SelectedMonitor;

                        if (infoPtr->MonitorsCount != 0)
                        {
                            if (Index < 0)
                                Index = 0;
                            else if (Index > 0)
                                Index--;
                        }

                        if (Index >= 0)
                        {
                            MonSelSetCurSelMonitor(infoPtr,
                                                   Index,
                                                   TRUE);
                        }
                        break;
                    }

                    case VK_DOWN:
                    case VK_RIGHT:
                    {
                        Index = infoPtr->SelectedMonitor;

                        if (infoPtr->MonitorsCount != 0)
                        {
                            if (Index < 0)
                                Index = (INT)infoPtr->MonitorsCount - 1;
                            else if (Index < (INT)infoPtr->MonitorsCount - 1)
                                Index++;
                        }

                        if (infoPtr->SelectedMonitor < infoPtr->MonitorsCount)
                        {
                            MonSelSetCurSelMonitor(infoPtr,
                                                   Index,
                                                   TRUE);
                        }
                        break;
                    }
                }
            }
            break;
        }

        case WM_CHAR:
        {
            if ((infoPtr->ControlExStyle & MSLM_EX_SELECTBYNUMKEY) &&
                wParam >= '1' && wParam <= '9')
            {
                INT Index = (INT)(wParam - '1');
                if (Index < (INT)infoPtr->MonitorsCount)
                {
                    MonSelSetCurSelMonitor(infoPtr,
                                           Index,
                                           TRUE);
                }
            }
            break;
        }

        case MSLM_SETMONITORSINFO:
        {
            Ret = MonSelSetMonitorsInfo(infoPtr,
                                        (DWORD)wParam,
                                        (const MONSL_MONINFO *)lParam);
            break;
        }

        case MSLM_GETMONITORSINFO:
        {
            Ret = MonSelGetMonitorsInfo(infoPtr,
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

        case MSLM_SETCURSEL:
        {
            Ret = MonSelSetCurSelMonitor(infoPtr,
                                         (INT)wParam,
                                         FALSE);
            break;
        }

        case MSLM_GETCURSEL:
        {
            Ret = infoPtr->SelectedMonitor;
            break;
        }

        case MSLM_SETMONITORINFO:
        {
            Ret = MonSelSetMonitorInfo(infoPtr,
                                       (INT)wParam,
                                       (const MONSL_MONINFO *)lParam);
            break;
        }

        case MSLM_GETMONITORINFO:
        {
            Ret = MonSelGetMonitorInfo(infoPtr,
                                       (INT)wParam,
                                       (PMONSL_MONINFO)lParam);
            break;
        }

        case MSLM_SETEXSTYLE:
        {
            Ret = MonSelSetExtendedStyle(infoPtr,
                                         (DWORD)lParam);
            break;
        }

        case MSLM_GETEXSTYLE:
        {
            Ret = MonSelGetExtendedStyle(infoPtr);
            break;
        }

        case MSLM_GETMONITORRECT:
        {
            Ret = (LRESULT)MonSelGetMonitorRect(infoPtr,
                                                (INT)wParam,
                                                (PRECT)lParam);
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
