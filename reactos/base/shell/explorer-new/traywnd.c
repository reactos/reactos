/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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

#include <precomp.h>

static const TRAYWINDOW_CTXMENU TrayWindowCtxMenu;

#define WM_APP_TRAYDESTROY  (WM_APP + 0x100)

static LONG TrayWndCount = 0;

static const TCHAR szTrayWndClass[] = TEXT("Shell_TrayWnd");

static const ITrayWindowVtbl ITrayWindowImpl_Vtbl;
static const IShellDesktopTrayVtbl IShellDesktopTrayImpl_Vtbl;

/*
 * ITrayWindow
 */

const GUID IID_IShellDesktopTray = {0x213e2df9, 0x9a14, 0x4328, {0x99, 0xb1, 0x69, 0x61, 0xf9, 0x14, 0x3c, 0xe9}};

typedef struct
{
    const ITrayWindowVtbl *lpVtbl;
    const IShellDesktopTrayVtbl *lpVtblShellDesktopTray;
    LONG Ref;

    HWND hWnd;
    HWND hWndDesktop;

    HWND hwndStart;
    HIMAGELIST himlStartBtn;
    SIZE StartBtnSize;
    HFONT hStartBtnFont;
    HFONT hCaptionFont;

    ITrayBandSite *TrayBandSite;
    HWND hwndRebar;
    HWND hwndTaskSwitch;
    HWND hwndTrayNotify;

    DWORD Position;
    HMONITOR Monitor;
    DWORD DraggingPosition;
    HMONITOR DraggingMonitor;

    RECT rcTrayWnd[4];
    RECT rcNewPosSize;
    SIZE TraySize;
    union
    {
        DWORD Flags;
        struct
        {
            DWORD AutoHide : 1;
            DWORD AlwaysOnTop : 1;
            DWORD SmSmallIcons : 1;
            DWORD HideClock : 1;
            DWORD Locked : 1;

            /* UI Status */
            DWORD InSizeMove : 1;
            DWORD IsDragging : 1;
            DWORD NewPosSize : 1;
        };
    };

    NONCLIENTMETRICS ncm;
    HFONT hFont;

    IMenuBand *StartMenuBand;
    IMenuPopup *StartMenuPopup;
    HBITMAP hbmStartMenu;

    HWND hWndTrayProperties;
} ITrayWindowImpl;

static IUnknown *
IUnknown_from_impl(ITrayWindowImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

static ITrayWindow *
ITrayWindow_from_impl(ITrayWindowImpl *This)
{
    return (ITrayWindow *)&This->lpVtbl;
}

static IShellDesktopTray *
IShellDesktopTray_from_impl(ITrayWindowImpl *This)
{
    return (IShellDesktopTray *)&This->lpVtblShellDesktopTray;
}

static ITrayWindowImpl *
impl_from_ITrayWindow(ITrayWindow *iface)
{
    return (ITrayWindowImpl *)((ULONG_PTR)iface - FIELD_OFFSET(ITrayWindowImpl,
                                                               lpVtbl));
}

static ITrayWindowImpl *
impl_from_IShellDesktopTray(IShellDesktopTray *iface)
{
    return (ITrayWindowImpl *)((ULONG_PTR)iface - FIELD_OFFSET(ITrayWindowImpl,
                                                               lpVtblShellDesktopTray));
}

/*
 * ITrayWindow
 */

static BOOL
ITrayWindowImpl_UpdateNonClientMetrics(IN OUT ITrayWindowImpl *This)
{
    This->ncm.cbSize = sizeof(This->ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                             sizeof(This->ncm),
                             &This->ncm,
                             0))
    {
        if (This->hFont != NULL)
            DeleteObject(This->hFont);

        This->hFont = CreateFontIndirect(&This->ncm.lfMessageFont);
        return TRUE;
    }

    return FALSE;
}

static VOID
ITrayWindowImpl_SetWindowsFont(IN OUT ITrayWindowImpl *This)
{
    if (This->hwndTrayNotify != NULL)
    {
        SendMessage(This->hwndTrayNotify,
                    WM_SETFONT,
                    (WPARAM)This->hFont,
                    TRUE);
    }
}

static HMONITOR
ITrayWindowImpl_GetScreenRectFromRect(IN OUT ITrayWindowImpl *This,
                                      IN OUT RECT *pRect,
                                      IN DWORD dwFlags)
{
    MONITORINFO mi;
    HMONITOR hMon;

    mi.cbSize = sizeof(mi);
    hMon = MonitorFromRect(pRect,
                           dwFlags);
    if (hMon != NULL &&
        GetMonitorInfo(hMon,
                       &mi))
    {
        *pRect = mi.rcMonitor;
    }
    else
    {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = GetSystemMetrics(SM_CXSCREEN);
        pRect->bottom = GetSystemMetrics(SM_CYSCREEN);

        hMon = NULL;
    }

    return hMon;
}

static HMONITOR
ITrayWindowImpl_GetMonitorFromRect(IN OUT ITrayWindowImpl *This,
                                   IN const RECT *pRect)
{
    HMONITOR hMon;

    /* In case the monitor sizes or saved sizes differ a bit (probably
       not a lot, only so the tray window overlaps into another monitor
       now), minimize the risk that we determine a wrong monitor by
       using the center point of the tray window if we can't determine
       it using the rectangle. */
    hMon = MonitorFromRect(pRect,
                           MONITOR_DEFAULTTONULL);
    if (hMon == NULL)
    {
        POINT pt;

        pt.x = pRect->left + ((pRect->right - pRect->left) / 2);
        pt.y = pRect->top + ((pRect->bottom - pRect->top) / 2);

        /* be less error-prone, find the nearest monitor */
        hMon = MonitorFromPoint(pt,
                                MONITOR_DEFAULTTONEAREST);
    }

    return hMon;
}

static HMONITOR
ITrayWindowImpl_GetScreenRect(IN OUT ITrayWindowImpl *This,
                              IN HMONITOR hMonitor,
                              IN OUT RECT *pRect)
{
    HMONITOR hMon = NULL;

    if (hMonitor != NULL)
    {
        MONITORINFO mi;

        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(This->Monitor,
                            &mi))
        {
            /* Hm, the monitor is gone? Try to find a monitor where it
               could be located now */
            hMon = ITrayWindowImpl_GetMonitorFromRect(This,
                                                      pRect);
            if (hMon == NULL ||
                !GetMonitorInfo(hMon,
                                &mi))
            {
                hMon = NULL;
                goto GetPrimaryRect;
            }
        }

        *pRect = mi.rcMonitor;
    }
    else
    {
GetPrimaryRect:
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = GetSystemMetrics(SM_CXSCREEN);
        pRect->bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    return hMon;
}

static VOID
ITrayWindowImpl_MakeTrayRectWithSize(IN DWORD Position,
                                     IN const SIZE *pTraySize,
                                     IN OUT RECT *pRect)
{
    switch (Position)
    {
        case ABE_LEFT:
            pRect->right = pRect->left + pTraySize->cx;
            break;

        case ABE_TOP:
            pRect->bottom = pRect->top + pTraySize->cy;
            break;

        case ABE_RIGHT:
            pRect->left = pRect->right - pTraySize->cx;
            break;

        case ABE_BOTTOM:
        default:
            pRect->top = pRect->bottom - pTraySize->cy;
            break;
    }
}

static VOID
ITrayWindowImpl_GetTrayRectFromScreenRect(IN OUT ITrayWindowImpl *This,
                                          IN DWORD Position,
                                          IN const RECT *pScreen,
                                          IN const SIZE *pTraySize  OPTIONAL,
                                          OUT RECT *pRect)
{
    if (pTraySize == NULL)
        pTraySize = &This->TraySize;

    *pRect = *pScreen;

    /* Move the border outside of the screen */
    InflateRect(pRect,
                GetSystemMetrics(SM_CXEDGE),
                GetSystemMetrics(SM_CYEDGE));

    ITrayWindowImpl_MakeTrayRectWithSize(Position,
                                         pTraySize,
                                         pRect);
}

BOOL
ITrayWindowImpl_IsPosHorizontal(IN OUT ITrayWindowImpl *This)
{
    return This->Position == ABE_TOP || This->Position == ABE_BOTTOM;
}

static HMONITOR
ITrayWindowImpl_CalculateValidSize(IN OUT ITrayWindowImpl *This,
                                   IN DWORD Position,
                                   IN OUT RECT *pRect)
{
    RECT rcScreen;
    BOOL Horizontal;
    HMONITOR hMon;
    SIZE szMax, szWnd;

    Horizontal = ITrayWindowImpl_IsPosHorizontal(This);

    szWnd.cx = pRect->right - pRect->left;
    szWnd.cy = pRect->bottom - pRect->top;

    rcScreen = *pRect;
    hMon = ITrayWindowImpl_GetScreenRectFromRect(This,
                                                 &rcScreen,
                                                 MONITOR_DEFAULTTONEAREST);

    /* Calculate the maximum size of the tray window and limit the window
       size to half of the screen's size. */
    szMax.cx = (rcScreen.right - rcScreen.left) / 2;
    szMax.cy = (rcScreen.bottom - rcScreen.top) / 2;
    if (szWnd.cx > szMax.cx)
        szWnd.cx = szMax.cx;
    if (szWnd.cy > szMax.cy)
        szWnd.cy = szMax.cy;

    /* FIXME - calculate */

    ITrayWindowImpl_GetTrayRectFromScreenRect(This,
                                              Position,
                                              &rcScreen,
                                              &szWnd,
                                              pRect);

    return hMon;
}

#if 0
static VOID
ITrayWindowImpl_GetMinimumWindowSize(IN OUT ITrayWindowImpl *This,
                                     OUT RECT *pRect)
{
    RECT rcMin = {0};

    AdjustWindowRectEx(&rcMin,
                       GetWindowLongPtr(This->hWnd,
                                        GWL_STYLE),
                       FALSE,
                       GetWindowLongPtr(This->hWnd,
                                        GWL_EXSTYLE));

    *pRect = rcMin;
}
#endif


static DWORD
ITrayWindowImpl_GetDraggingRectFromPt(IN OUT ITrayWindowImpl *This,
                                      IN POINT pt,
                                      OUT RECT *pRect,
                                      OUT HMONITOR *phMonitor)
{
    HMONITOR hMon, hMonNew;
    DWORD PosH, PosV, Pos;
    SIZE DeltaPt, ScreenOffset;
    RECT rcScreen;

    rcScreen.left = 0;
    rcScreen.top = 0;

    /* Determine the screen rectangle */
    hMon = MonitorFromPoint(pt,
                            MONITOR_DEFAULTTONULL);

    if (hMon != NULL)
    {
        MONITORINFO mi;

        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(hMon,
                            &mi))
        {
            hMon = NULL;
            goto GetPrimaryScreenRect;
        }

        /* make left top corner of the screen zero based to
           make calculations easier */
        pt.x -= mi.rcMonitor.left;
        pt.y -= mi.rcMonitor.top;

        ScreenOffset.cx = mi.rcMonitor.left;
        ScreenOffset.cy = mi.rcMonitor.top;
        rcScreen.right = mi.rcMonitor.right - mi.rcMonitor.left;
        rcScreen.bottom = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    else
    {
GetPrimaryScreenRect:
        ScreenOffset.cx = 0;
        ScreenOffset.cy = 0;
        rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    /* Calculate the nearest screen border */
    if (pt.x < rcScreen.right / 2)
    {
        DeltaPt.cx = pt.x;
        PosH = ABE_LEFT;
    }
    else
    {
        DeltaPt.cx = rcScreen.right - pt.x;
        PosH = ABE_RIGHT;
    }

    if (pt.y < rcScreen.bottom / 2)
    {
        DeltaPt.cy = pt.y;
        PosV = ABE_TOP;
    }
    else
    {
        DeltaPt.cy = rcScreen.bottom - pt.y;
        PosV = ABE_BOTTOM;
    }

    Pos = (DeltaPt.cx * rcScreen.bottom < DeltaPt.cy * rcScreen.right) ? PosH : PosV;

    /* Fix the screen origin to be relative to the primary monitor again */
    OffsetRect(&rcScreen,
               ScreenOffset.cx,
               ScreenOffset.cy);

    hMonNew = ITrayWindowImpl_GetMonitorFromRect(This,
                                                 &This->rcTrayWnd[Pos]);
    if (hMon != hMonNew)
    {
        SIZE szTray;

        /* Recalculate the rectangle, we're dragging to another monitor.
           We don't need to recalculate the rect on single monitor systems. */
        szTray.cx = This->rcTrayWnd[Pos].right - This->rcTrayWnd[Pos].left;
        szTray.cy = This->rcTrayWnd[Pos].bottom - This->rcTrayWnd[Pos].top;

        ITrayWindowImpl_GetTrayRectFromScreenRect(This,
                                                  Pos,
                                                  &rcScreen,
                                                  &szTray,
                                                  pRect);

        hMon = hMonNew;
    }
    else
    {
        /* The user is dragging the tray window on the same monitor. We don't need
           to recalculate the rectangle */
        *pRect = This->rcTrayWnd[Pos];
    }

    *phMonitor = hMon;

    return Pos;
}

static DWORD
ITrayWindowImpl_GetDraggingRectFromRect(IN OUT ITrayWindowImpl *This,
                                        IN OUT RECT *pRect,
                                        OUT HMONITOR *phMonitor)
{
    POINT pt;

    /* Calculate the center of the rectangle. We call
       ITrayWindowImpl_GetDraggingRectFromPt to calculate a valid
       dragging rectangle */
    pt.x = pRect->left + ((pRect->right - pRect->left) / 2);
    pt.y = pRect->top + ((pRect->bottom - pRect->top) / 2);

    return ITrayWindowImpl_GetDraggingRectFromPt(This,
                                                 pt,
                                                 pRect,
                                                 phMonitor);
}

static VOID
ITrayWindowImpl_ChangingWinPos(IN OUT ITrayWindowImpl *This,
                               IN OUT LPWINDOWPOS pwp)
{
    RECT rcTray;

    if (This->IsDragging)
    {
        rcTray.left = pwp->x;
        rcTray.top = pwp->y;
        rcTray.right = rcTray.left + pwp->cx;
        rcTray.bottom = rcTray.top + pwp->cy;

        if (!EqualRect(&rcTray,
                       &This->rcTrayWnd[This->DraggingPosition]))
        {
            /* Recalculate the rectangle, the user dragged the tray
               window to another monitor or the window was somehow else
               moved or resized */
            This->DraggingPosition = ITrayWindowImpl_GetDraggingRectFromRect(This,
                                                                             &rcTray,
                                                                             &This->DraggingMonitor);
            //This->rcTrayWnd[This->DraggingPosition] = rcTray;
        }

        //This->Monitor = ITrayWindowImpl_CalculateValidSize(This,
        //                                                   This->DraggingPosition,
        //                                                   &rcTray);

        This->Monitor = This->DraggingMonitor;
        This->Position = This->DraggingPosition;
        This->IsDragging = FALSE;

        This->rcTrayWnd[This->Position] = rcTray;
        goto ChangePos;
    }
    else if (GetWindowRect(This->hWnd,
                           &rcTray))
    {
        if (This->InSizeMove)
        {
            if (!(pwp->flags & SWP_NOMOVE))
            {
                rcTray.left = pwp->x;
                rcTray.top = pwp->y;
            }

            if (!(pwp->flags & SWP_NOSIZE))
            {
                rcTray.right = rcTray.left + pwp->cx;
                rcTray.bottom = rcTray.top + pwp->cy;
            }

            This->Position = ITrayWindowImpl_GetDraggingRectFromRect(This,
                                                                     &rcTray,
                                                                     &This->Monitor);

            if (!(pwp->flags & (SWP_NOMOVE | SWP_NOSIZE)))
            {
                SIZE szWnd;

                szWnd.cx = pwp->cx;
                szWnd.cy = pwp->cy;

                ITrayWindowImpl_MakeTrayRectWithSize(This->Position,
                                                     &szWnd,
                                                     &rcTray);
            }

            This->rcTrayWnd[This->Position] = rcTray;
        }
        else
        {
            /* If the user isn't resizing the tray window we need to make sure the
               new size or position is valid. This is to prevent changes to the window
               without user interaction. */
            rcTray = This->rcTrayWnd[This->Position];
        }

ChangePos:
        This->TraySize.cx = rcTray.right - rcTray.left;
        This->TraySize.cy = rcTray.bottom - rcTray.top;

        pwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
        pwp->x = rcTray.left;
        pwp->y = rcTray.top;
        pwp->cx = This->TraySize.cx;
        pwp->cy = This->TraySize.cy;
    }
}

static VOID
ITrayWindowImpl_ApplyClipping(IN OUT ITrayWindowImpl *This,
                              IN BOOL Clip)
{
    RECT rcClip, rcWindow;
    HRGN hClipRgn;

    if (GetWindowRect(This->hWnd,
                      &rcWindow))
    {
        /* Disable clipping on systems with only one monitor */
        if (GetSystemMetrics(SM_CMONITORS) <= 1)
            Clip = FALSE;

        if (Clip)
        {
            rcClip = rcWindow;

            ITrayWindowImpl_GetScreenRect(This,
                                          This->Monitor,
                                          &rcClip);

            if (!IntersectRect(&rcClip,
                               &rcClip,
                               &rcWindow))
            {
                rcClip = rcWindow;
            }

            OffsetRect(&rcClip,
                       -rcWindow.left,
                       -rcWindow.top);

            hClipRgn = CreateRectRgnIndirect(&rcClip);
        }
        else
            hClipRgn = NULL;

        /* Set the clipping region or make sure the window isn't clipped
           by disabling it explicitly. */
        SetWindowRgn(This->hWnd,
                     hClipRgn,
                     TRUE);
    }
}

static VOID
ITrayWindowImpl_CheckTrayWndPosition(IN OUT ITrayWindowImpl *This)
{
    RECT rcTray;

    rcTray = This->rcTrayWnd[This->Position];
//    DbgPrint("CheckTray: %d: %d,%d,%d,%d\n", This->Position, rcTray.left, rcTray.top, rcTray.right, rcTray.bottom);

    /* Move the tray window */
    SetWindowPos(This->hWnd,
                 NULL,
                 rcTray.left,
                 rcTray.top,
                 rcTray.right - rcTray.left,
                 rcTray.bottom - rcTray.top,
                 SWP_NOZORDER);

    ITrayWindowImpl_ApplyClipping(This,
                                  TRUE);
}

typedef struct _TW_STUCKRECTS2
{
    DWORD cbSize;
    LONG Unknown;
    DWORD dwFlags;
    DWORD Position;
    SIZE Size;
    RECT Rect;
} TW_STRUCKRECTS2, *PTW_STUCKRECTS2;

static VOID
ITrayWindowImpl_RegLoadSettings(IN OUT ITrayWindowImpl *This)
{
    DWORD Pos;
    TW_STRUCKRECTS2 sr;
    RECT rcScreen;
    SIZE WndSize, EdgeSize, DlgFrameSize;
    DWORD cbSize = sizeof(sr);

    EdgeSize.cx = GetSystemMetrics(SM_CXEDGE);
    EdgeSize.cy = GetSystemMetrics(SM_CYEDGE);
    DlgFrameSize.cx = GetSystemMetrics(SM_CXDLGFRAME);
    DlgFrameSize.cy = GetSystemMetrics(SM_CYDLGFRAME);

    if (SHGetValue(hkExplorer,
                   TEXT("StuckRects2"),
                   TEXT("Settings"),
                   NULL,
                   &sr,
                   &cbSize) == ERROR_SUCCESS &&
        sr.cbSize == sizeof(sr))
    {
        This->AutoHide = (sr.dwFlags & ABS_AUTOHIDE) != 0;
        This->AlwaysOnTop = (sr.dwFlags & ABS_ALWAYSONTOP) != 0;
        This->SmSmallIcons = (sr.dwFlags & 0x4) != 0;
        This->HideClock = (sr.dwFlags & 0x8) != 0;

        /* FIXME: Are there more flags? */

        if (This->hWnd != NULL)
            SetWindowPos (This->hWnd,
                          This->AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                          0,
                          0,
                          0,
                          0,
                          SWP_NOMOVE | SWP_NOSIZE);

        if (sr.Position > ABE_BOTTOM)
            This->Position = ABE_BOTTOM;
        else
            This->Position = sr.Position;

        /* Try to find out which monitor the tray window was located on last.
           Here we're only interested in the monitor screen that we think
           is the last one used. We're going to determine on which monitor
           we really are after calculating the docked position. */
        rcScreen = sr.Rect;
        ITrayWindowImpl_GetScreenRectFromRect(This,
                                              &rcScreen,
                                              MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        This->Position = ABE_BOTTOM;

        /* Use the minimum size of the taskbar, we'll use the start
           button as a minimum for now. Make sure we calculate the
           entire window size, not just the client size. However, we
           use a thinner border than a standard thick border, so that
           the start button and bands are not stuck to the screen border. */
        sr.Size.cx = This->StartBtnSize.cx + (2 * (EdgeSize.cx + DlgFrameSize.cx));
        sr.Size.cy = This->StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));

        /* Use the primary screen by default */
        rcScreen.left = 0;
        rcScreen.top = 0;
        rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.right = GetSystemMetrics(SM_CYSCREEN);
        ITrayWindowImpl_GetScreenRectFromRect(This,
                                              &rcScreen,
                                              MONITOR_DEFAULTTOPRIMARY);
    }

    /* Determine a minimum tray window rectangle. The "client" height is
       zero here since we cannot determine an optimal minimum width when
       loaded as a vertical tray window. We just need to make sure the values
       loaded from the registry are at least. The windows explorer behaves
       the same way, it allows the user to save a zero width vertical tray
       window, but not a zero height horizontal tray window. */
    WndSize.cx = 2 * (EdgeSize.cx + DlgFrameSize.cx);
    WndSize.cy = This->StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));

    if (WndSize.cx < sr.Size.cx)
        WndSize.cx = sr.Size.cx;
    if (WndSize.cy < sr.Size.cy)
        WndSize.cy = sr.Size.cy;

    /* Save the calculated size */
    This->TraySize = WndSize;

    /* Calculate all docking rectangles. We need to do this here so they're
       initialized and dragging the tray window to another position gives
       usable results */
    for (Pos = ABE_LEFT;
         Pos <= ABE_BOTTOM;
         Pos++)
    {
        ITrayWindowImpl_GetTrayRectFromScreenRect(This,
                                                  Pos,
                                                  &rcScreen,
                                                  &This->TraySize,
                                                  &This->rcTrayWnd[Pos]);
//        DbgPrint("rcTrayWnd[%d(%d)]: %d,%d,%d,%d\n", Pos, This->Position, This->rcTrayWnd[Pos].left, This->rcTrayWnd[Pos].top, This->rcTrayWnd[Pos].right, This->rcTrayWnd[Pos].bottom);
    }

    /* Determine which monitor we are on. It shouldn't matter which docked
       position rectangle we use */
    This->Monitor = ITrayWindowImpl_GetMonitorFromRect(This,
                                                       &This->rcTrayWnd[ABE_LEFT]);
}

static UINT
ITrayWindowImpl_TrackMenu(IN OUT ITrayWindowImpl *This,
                          IN HMENU hMenu,
                          IN POINT *ppt  OPTIONAL,
                          IN HWND hwndExclude  OPTIONAL,
                          IN BOOL TrackUp,
                          IN BOOL IsContextMenu)
{
    TPMPARAMS tmp, *ptmp = NULL;
    POINT pt;
    UINT cmdId;
    UINT fuFlags;

    if (hwndExclude != NULL)
    {
        /* Get the client rectangle and map it to screen coordinates */
        if (GetClientRect(hwndExclude,
                          &tmp.rcExclude) &&
            MapWindowPoints(hwndExclude,
                            NULL,
                            (LPPOINT)&tmp.rcExclude,
                            2) != 0)
        {
            ptmp = &tmp;
        }
    }

    if (ppt == NULL)
    {
        if (ptmp == NULL &&
            GetClientRect(This->hWnd,
                          &tmp.rcExclude) &&
            MapWindowPoints(This->hWnd,
                            NULL,
                            (LPPOINT)&tmp.rcExclude,
                            2) != 0)
        {
            ptmp = &tmp;
        }

        if (ptmp != NULL)
        {
            /* NOTE: TrackPopupMenuEx will eventually align the track position
                     for us, no need to take care of it here as long as the
                     coordinates are somewhere within the exclusion rectangle */
            pt.x = ptmp->rcExclude.left;
            pt.y = ptmp->rcExclude.top;
        }
        else
            pt.x = pt.y = 0;
    }
    else
        pt = *ppt;

    tmp.cbSize = sizeof(tmp);

    fuFlags = TPM_RETURNCMD | TPM_VERTICAL;
    fuFlags |= (TrackUp ? TPM_BOTTOMALIGN : TPM_TOPALIGN);
    if (IsContextMenu)
        fuFlags |= TPM_RIGHTBUTTON;
    else
        fuFlags |= (TrackUp ? TPM_VERNEGANIMATION : TPM_VERPOSANIMATION);

    cmdId = TrackPopupMenuEx(hMenu,
                             fuFlags,
                             pt.x,
                             pt.y,
                             This->hWnd,
                             ptmp);

    return cmdId;
}

static UINT
ITrayWindowImpl_TrackCtxMenu(IN OUT ITrayWindowImpl *This,
                             IN const TRAYWINDOW_CTXMENU *pMenu,
                             IN POINT *ppt  OPTIONAL,
                             IN HWND hwndExclude  OPTIONAL,
                             IN BOOL TrackUp,
                             IN PVOID Context  OPTIONAL)
{
    HMENU hPopup;
    UINT cmdId = 0;
    PVOID pcmContext = NULL;

    hPopup = pMenu->CreateCtxMenu(This->hWnd,
                                  &pcmContext,
                                  Context);
    if (hPopup != NULL)
    {
        cmdId = ITrayWindowImpl_TrackMenu(This,
                                          hPopup,
                                          ppt,
                                          hwndExclude,
                                          TrackUp,
                                          TRUE);

        pMenu->CtxMenuCommand(This->hWnd,
                              cmdId,
                              pcmContext,
                              Context);
    }

    return cmdId;
}

static VOID
ITrayWindowImpl_Free(ITrayWindowImpl *This)
{
    HeapFree(hProcessHeap,
             0,
             This);
}


static ULONG STDMETHODCALLTYPE
ITrayWindowImpl_Release(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);
    if (Ret == 0)
        ITrayWindowImpl_Free(This);

    return Ret;
}

static VOID
ITrayWindowImpl_Destroy(ITrayWindowImpl *This)
{
    (void)InterlockedExchangePointer((PVOID*)&This->hWnd,
                                     NULL);

    if (This->himlStartBtn != NULL)
    {
        ImageList_Destroy(This->himlStartBtn);
        This->himlStartBtn = NULL;
    }

    if (This->hCaptionFont != NULL)
    {
        DeleteObject(This->hCaptionFont);
        This->hCaptionFont = NULL;
    }

    if (This->hStartBtnFont != NULL)
    {
        DeleteObject(This->hStartBtnFont);
        This->hStartBtnFont = NULL;
    }

    if (This->hFont != NULL)
    {
        DeleteObject(This->hFont);
        This->hFont = NULL;
    }

    if (This->StartMenuPopup != NULL)
    {
        IMenuPopup_Release(This->StartMenuPopup);
        This->StartMenuPopup = NULL;
    }

    if (This->hbmStartMenu != NULL)
    {
        DeleteObject(This->hbmStartMenu);
        This->hbmStartMenu = NULL;
    }

    if (This->StartMenuBand != NULL)
    {
        IMenuBand_Release(This->StartMenuBand);
        This->StartMenuBand = NULL;
    }

    if (This->TrayBandSite != NULL)
    {
        /* FIXME: Unload bands */
        ITrayBandSite_Release(This->TrayBandSite);
        This->TrayBandSite = NULL;
    }

    ITrayWindowImpl_Release(ITrayWindow_from_impl(This));

    if (InterlockedDecrement(&TrayWndCount) == 0)
        PostQuitMessage(0);
}

static ULONG STDMETHODCALLTYPE
ITrayWindowImpl_AddRef(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    return InterlockedIncrement(&This->Ref);
}


static BOOL
ITrayWindowImpl_NCCreate(IN OUT ITrayWindowImpl *This)
{
    ITrayWindowImpl_AddRef(ITrayWindow_from_impl(This));

    return TRUE;
}

static VOID
ITrayWindowImpl_UpdateStartButton(IN OUT ITrayWindowImpl *This,
                                  IN HBITMAP hbmStart  OPTIONAL)
{
    SIZE Size = { 0, 0 };

    if (This->himlStartBtn == NULL ||
        !SendMessage(This->hwndStart,
                     BCM_GETIDEALSIZE,
                     0,
                     (LPARAM)&Size))
    {
        Size.cx = GetSystemMetrics(SM_CXEDGE);
        Size.cy = GetSystemMetrics(SM_CYEDGE);

        if (hbmStart == NULL)
        {
            hbmStart = (HBITMAP)SendMessage(This->hwndStart,
                                            BM_GETIMAGE,
                                            IMAGE_BITMAP,
                                            0);
        }

        if (hbmStart != NULL)
        {
            BITMAP bmp;

            if (GetObject(hbmStart,
                          sizeof(bmp),
                          &bmp) != 0)
            {
                Size.cx += bmp.bmWidth;
                Size.cy += max(bmp.bmHeight,
                               GetSystemMetrics(SM_CYCAPTION));
            }
            else
            {
                /* Huh?! Shouldn't happen... */
                goto DefSize;
            }
        }
        else
        {
DefSize:
            Size.cx += GetSystemMetrics(SM_CXMINIMIZED);
            Size.cy += GetSystemMetrics(SM_CYCAPTION);
        }
    }

    /* Save the size of the start button */
    This->StartBtnSize = Size;
}

static VOID
ITrayWindowImpl_AlignControls(IN OUT ITrayWindowImpl *This,
                              IN PRECT prcClient  OPTIONAL)
{
    RECT rcClient;
    SIZE TraySize, StartSize;
    POINT ptTrayNotify = { 0, 0 };
    BOOL Horizontal;
    HDWP dwp;

    if (prcClient != NULL)
    {
        rcClient = *prcClient;
    }
    else
    {
        if (!GetClientRect(This->hWnd,
                           &rcClient))
        {
            return;
        }
    }

    Horizontal = ITrayWindowImpl_IsPosHorizontal(This);

    /* We're about to resize/move the start button, the rebar control and
       the tray notification control */
    dwp = BeginDeferWindowPos(3);
    if (dwp == NULL)
        return;

    /* Limit the Start button width to the client width, if neccessary */
    StartSize = This->StartBtnSize;
    if (StartSize.cx > rcClient.right)
        StartSize.cx = rcClient.right;

    if (This->hwndStart != NULL)
    {
        /* Resize and reposition the button */
        dwp = DeferWindowPos(dwp,
                             This->hwndStart,
                             NULL,
                             0,
                             0,
                             StartSize.cx,
                             StartSize.cy,
                             SWP_NOZORDER | SWP_NOACTIVATE);
        if (dwp == NULL)
            return;
    }

    /* Determine the size that the tray notification window needs */
    if (Horizontal)
    {
        TraySize.cx = 0;
        TraySize.cy = rcClient.bottom;
    }
    else
    {
        TraySize.cx = rcClient.right;
        TraySize.cy = 0;
    }

    if (This->hwndTrayNotify != NULL &&
        SendMessage(This->hwndTrayNotify,
                    TNWM_GETMINIMUMSIZE,
                    (WPARAM)Horizontal,
                    (LPARAM)&TraySize))
    {
        /* Move the tray notification window to the desired location */
        if (Horizontal)
            ptTrayNotify.x = rcClient.right - TraySize.cx;
        else
            ptTrayNotify.y = rcClient.bottom - TraySize.cy;

        dwp = DeferWindowPos(dwp,
                             This->hwndTrayNotify,
                             NULL,
                             ptTrayNotify.x,
                             ptTrayNotify.y,
                             TraySize.cx,
                             TraySize.cy,
                             SWP_NOZORDER | SWP_NOACTIVATE);
        if (dwp == NULL)
            return;
    }

    /* Resize/Move the rebar control */
    if (This->hwndRebar != NULL)
    {
        POINT ptRebar = { 0, 0 };
        SIZE szRebar;

        SetWindowStyle(This->hwndRebar,
                       CCS_VERT,
                       Horizontal ? 0 : CCS_VERT);

        if (Horizontal)
        {
            ptRebar.x = StartSize.cx + GetSystemMetrics(SM_CXSIZEFRAME);
            szRebar.cx = ptTrayNotify.x - ptRebar.x;
            szRebar.cy = rcClient.bottom;
        }
        else
        {
            ptRebar.y = StartSize.cy + GetSystemMetrics(SM_CYSIZEFRAME);
            szRebar.cx = rcClient.right;
            szRebar.cy = ptTrayNotify.y - ptRebar.y;
        }

        dwp = DeferWindowPos(dwp,
                             This->hwndRebar,
                             NULL,
                             ptRebar.x,
                             ptRebar.y,
                             szRebar.cx,
                             szRebar.cy,
                             SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (dwp != NULL)
        EndDeferWindowPos(dwp);

    if (This->hwndTaskSwitch != NULL)
    {
        /* Update the task switch window configuration */
        SendMessage(This->hwndTaskSwitch,
                    TSWM_UPDATETASKBARPOS,
                    0,
                    0);
    }
}

static BOOL
ITrayWindowImpl_CreateStartBtnImageList(IN OUT ITrayWindowImpl *This)
{
    HICON hIconStart;
    SIZE IconSize;

    if (This->himlStartBtn != NULL)
        return TRUE;

    IconSize.cx = GetSystemMetrics(SM_CXSMICON);
    IconSize.cy = GetSystemMetrics(SM_CYSMICON);

    /* Load the start button icon and create a image list for it */
    hIconStart = LoadImage(hExplorerInstance,
                           MAKEINTRESOURCE(IDI_START),
                           IMAGE_ICON,
                           IconSize.cx,
                           IconSize.cy,
                           LR_SHARED | LR_DEFAULTCOLOR);

    if (hIconStart != NULL)
    {
        This->himlStartBtn = ImageList_Create(IconSize.cx,
                                              IconSize.cy,
                                              ILC_COLOR32 | ILC_MASK,
                                              1,
                                              1);
        if (This->himlStartBtn != NULL)
        {
            if (ImageList_AddIcon(This->himlStartBtn,
                                  hIconStart) >= 0)
            {
                return TRUE;
            }

            /* Failed to add the icon! */
            ImageList_Destroy(This->himlStartBtn);
            This->himlStartBtn = NULL;
        }
    }

    return FALSE;
}

static HBITMAP
ITrayWindowImpl_CreateStartButtonBitmap(IN OUT ITrayWindowImpl *This)
{
    TCHAR szStartCaption[32];
    HFONT  hFontOld;
    HDC hDC = NULL;
    HDC hDCScreen = NULL;
    SIZE Size, SmallIcon;
    HBITMAP hbmpOld, hbmp = NULL;
    HBITMAP hBitmap = NULL;
    HICON hIconStart;
    BOOL Ret;
    UINT Flags;
    RECT rcButton;

    /* NOTE: This is the backwards compatibility code that is used if the
             Common Controls Version 6.0 are not available! */

    if (!LoadString(hExplorerInstance,
                    IDS_START,
                    szStartCaption,
                    sizeof(szStartCaption) / sizeof(szStartCaption[0])))
    {
        return NULL;
    }

    /* Load the start button icon */
    SmallIcon.cx = GetSystemMetrics(SM_CXSMICON);
    SmallIcon.cy = GetSystemMetrics(SM_CYSMICON);
    hIconStart = LoadImage(hExplorerInstance,
                           MAKEINTRESOURCE(IDI_START),
                           IMAGE_ICON,
                           SmallIcon.cx,
                           SmallIcon.cy,
                           LR_SHARED | LR_DEFAULTCOLOR);

    hDCScreen = GetDC(NULL);
    if (hDCScreen == NULL)
        goto Cleanup;

    hDC = CreateCompatibleDC(hDCScreen);
    if (hDC == NULL)
        goto Cleanup;

    hFontOld = SelectObject(hDC,
                            This->hStartBtnFont);

    Ret = GetTextExtentPoint32(hDC,
                               szStartCaption,
                               _tcslen(szStartCaption),
                               &Size);

    SelectObject(hDC,
                 hFontOld);
    if (!Ret)
        goto Cleanup;

    /* Make sure the height is at least the size of a caption icon. */
    if (hIconStart != NULL)
        Size.cx += SmallIcon.cx + 4;
    Size.cy = max(Size.cy,
                  SmallIcon.cy);

    /* Create the bitmap */
    hbmp = CreateCompatibleBitmap(hDCScreen,
                                  Size.cx,
                                  Size.cy);
    if (hbmp == NULL)
        goto Cleanup;

    /* Caluclate the button rect */
    rcButton.left = 0;
    rcButton.top = 0;
    rcButton.right = Size.cx;
    rcButton.bottom = Size.cy;

    /* Draw the button */
    hbmpOld = SelectObject(hDC,
                           hbmp);

    Flags = DC_TEXT | DC_INBUTTON;
    if (hIconStart != NULL)
        Flags |= DC_ICON;

    if (DrawCapTemp != NULL)
    {
        Ret = DrawCapTemp(NULL,
                          hDC,
                          &rcButton,
                          This->hStartBtnFont,
                          hIconStart,
                          szStartCaption,
                          Flags);
    }

    SelectObject(hDC,
                 hbmpOld);

    if (!Ret)
        goto Cleanup;

    /* We successfully created the bitmap! */
    hBitmap = hbmp;
    hbmp = NULL;

Cleanup:
    if (hDCScreen != NULL)
    {
        ReleaseDC(NULL,
            hDCScreen);
    }

    if (hbmp != NULL)
        DeleteObject(hbmp);

    if (hDC != NULL)
        DeleteDC(hDC);

    return hBitmap;
}

static VOID
ITrayWindowImpl_Create(IN OUT ITrayWindowImpl *This)
{
    TCHAR szStartCaption[32];

    InterlockedIncrement(&TrayWndCount);

    if (!LoadString(hExplorerInstance,
                    IDS_START,
                    szStartCaption,
                    sizeof(szStartCaption) / sizeof(szStartCaption[0])))
    {
        szStartCaption[0] = TEXT('\0');
    }

    if (This->hStartBtnFont == NULL || This->hCaptionFont == NULL)
    {
        NONCLIENTMETRICS ncm;

        /* Get the system fonts, we use the caption font,
           always bold, though. */
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                                 sizeof(ncm),
                                 &ncm,
                                 FALSE))
        {
            if (This->hCaptionFont == NULL)
            {
                ncm.lfCaptionFont.lfWeight = FW_NORMAL;
                This->hCaptionFont = CreateFontIndirect(&ncm.lfCaptionFont);
            }

            if (This->hStartBtnFont == NULL)
            {
                ncm.lfCaptionFont.lfWeight = FW_BOLD;
                This->hStartBtnFont = CreateFontIndirect(&ncm.lfCaptionFont);
            }
        }
    }

    /* Create the Start button */
    This->hwndStart = CreateWindowEx(0,
                                     WC_BUTTON,
                                     szStartCaption,
                                     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
                                         BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | BS_BITMAP,
                                     0,
                                     0,
                                     0,
                                     0,
                                     This->hWnd,
                                     (HMENU)IDC_STARTBTN,
                                     hExplorerInstance,
                                     NULL);
    if (This->hwndStart)
    {
        SendMessage(This->hwndStart,
                    WM_SETFONT,
                    (WPARAM)This->hStartBtnFont,
                    FALSE);

        if (ITrayWindowImpl_CreateStartBtnImageList(This))
        {
            BUTTON_IMAGELIST bil;

            /* Try to set the start button image. This requires the Common
               Controls 6.0 to be present (XP and later) */
            bil.himl = This->himlStartBtn;
            bil.margin.left = bil.margin.right = 1;
            bil.margin.top = bil.margin.bottom = 1;
            bil.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

            if (!SendMessage(This->hwndStart,
                             BCM_SETIMAGELIST,
                             0,
                             (LPARAM)&bil))
            {
                /* Fall back to the deprecated method on older systems that don't
                   support Common Controls 6.0 */
                ImageList_Destroy(This->himlStartBtn);
                This->himlStartBtn = NULL;

                goto SetStartBtnImage;
            }

            /* We're using the image list, remove the BS_BITMAP style and
               don't center it horizontally */
            SetWindowStyle(This->hwndStart,
                           BS_BITMAP | BS_RIGHT,
                           0);

            ITrayWindowImpl_UpdateStartButton(This,
                                              NULL);
        }
        else
        {
            HBITMAP hbmStart, hbmOld;

SetStartBtnImage:
            hbmStart = ITrayWindowImpl_CreateStartButtonBitmap(This);
            if (hbmStart != NULL)
            {
                ITrayWindowImpl_UpdateStartButton(This,
                                                  hbmStart);

                hbmOld = (HBITMAP)SendMessage(This->hwndStart,
                                              BM_SETIMAGE,
                                              IMAGE_BITMAP,
                                              (LPARAM)hbmStart);

                if (hbmOld != NULL)
                    DeleteObject(hbmOld);
            }
        }
    }

    /* Load the saved tray window settings */
    ITrayWindowImpl_RegLoadSettings(This);

    /* Create and initialize the start menu */
    This->hbmStartMenu = LoadBitmap(hExplorerInstance,
                                    MAKEINTRESOURCE(IDB_STARTMENU));
    This->StartMenuPopup = CreateStartMenu(ITrayWindow_from_impl(This),
                                           &This->StartMenuBand,
                                           This->hbmStartMenu,
                                           0);

    /* Load the tray band site */
    if (This->TrayBandSite != NULL)
    {
        ITrayBandSite_Release(This->TrayBandSite);
    }

    This->TrayBandSite = CreateTrayBandSite(ITrayWindow_from_impl(This),
                                            &This->hwndRebar,
                                            &This->hwndTaskSwitch);

    /* Create the tray notification window */
    This->hwndTrayNotify = CreateTrayNotifyWnd(ITrayWindow_from_impl(This),
                                               This->HideClock);

    if (ITrayWindowImpl_UpdateNonClientMetrics(This))
    {
        ITrayWindowImpl_SetWindowsFont(This);
    }

    /* Move the tray window to the right position and resize it if neccessary */
    ITrayWindowImpl_CheckTrayWndPosition(This);

    /* Align all controls on the tray window */
    ITrayWindowImpl_AlignControls(This,
                                  NULL);
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_QueryInterface(IN OUT ITrayWindow *iface,
                               IN REFIID riid,
                               OUT LPVOID *ppvObj)
{
    ITrayWindowImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = impl_from_ITrayWindow(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IShellDesktopTray))
    {
        *ppvObj = IShellDesktopTray_from_impl(This);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ITrayWindowImpl_AddRef(iface);
    return S_OK;
}

static ITrayWindowImpl *
ITrayWindowImpl_Construct(VOID)
{
    ITrayWindowImpl *This;

    This = HeapAlloc(hProcessHeap,
                     0,
                     sizeof(*This));
    if (This == NULL)
        return NULL;

    ZeroMemory(This,
               sizeof(*This));
    This->lpVtbl = &ITrayWindowImpl_Vtbl;
    This->lpVtblShellDesktopTray = &IShellDesktopTrayImpl_Vtbl;
    This->Ref = 1;
    This->Position = (DWORD)-1;

    return This;
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_Open(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);
    HRESULT Ret = S_OK;
    HWND hWnd;
    DWORD dwExStyle;

    /* Check if there's already a window created and try to show it.
       If it was somehow destroyed just create a new tray window. */
    if (This->hWnd != NULL)
    {
        if (IsWindow(This->hWnd))
        {
            if (!IsWindowVisible(This->hWnd))
            {
                ITrayWindowImpl_CheckTrayWndPosition(This);

                ShowWindow(This->hWnd,
                           SW_SHOW);
            }
        }
        else
            goto TryCreateTrayWnd;
    }
    else
    {
        RECT rcWnd;

TryCreateTrayWnd:
        dwExStyle = WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;
        if (This->AlwaysOnTop)
            dwExStyle |= WS_EX_TOPMOST;

        if (This->Position != (DWORD)-1)
            rcWnd = This->rcTrayWnd[This->Position];
        else
        {
            ZeroMemory(&rcWnd,
                       sizeof(rcWnd));
        }

        hWnd = CreateWindowEx(dwExStyle,
                              szTrayWndClass,
                              NULL,
                              WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                                  WS_BORDER | WS_THICKFRAME,
                              rcWnd.left,
                              rcWnd.top,
                              rcWnd.right - rcWnd.left,
                              rcWnd.bottom - rcWnd.top,
                              NULL,
                              NULL,
                              hExplorerInstance,
                              This);
        if (hWnd == NULL)
            Ret = E_FAIL;
    }

    return Ret;
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_Close(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    if (This->hWnd != NULL)
    {
        SendMessage(This->hWnd,
                    WM_APP_TRAYDESTROY,
                    0,
                    0);
    }

    return S_OK;
}

static HWND STDMETHODCALLTYPE
ITrayWindowImpl_GetHWND(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    return This->hWnd;
}

static BOOL STDMETHODCALLTYPE
ITrayWindowImpl_IsSpecialHWND(IN OUT ITrayWindow *iface,
                              IN HWND hWnd)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    return (hWnd == This->hWnd ||
            (This->hWndDesktop != NULL && hWnd == This->hWndDesktop));
}

static BOOL STDMETHODCALLTYPE
ITrayWindowImpl_IsHorizontal(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);
    return ITrayWindowImpl_IsPosHorizontal(This);
}

static HFONT STDMETHODCALLTYPE
ITrayWIndowImpl_GetCaptionFonts(IN OUT ITrayWindow *iface,
                                OUT HFONT *phBoldCaption  OPTIONAL)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    if (phBoldCaption != NULL)
        *phBoldCaption = This->hStartBtnFont;

    return This->hCaptionFont;
}

static HWND STDMETHODCALLTYPE
ITrayWindowImpl_DisplayProperties(IN OUT ITrayWindow *iface)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    if (This->hWndTrayProperties != NULL)
    {
        BringWindowToTop(This->hWndTrayProperties);
        return This->hWndTrayProperties;
    }

    This->hWndTrayProperties = DisplayTrayProperties(ITrayWindow_from_impl(This));
    return This->hWndTrayProperties;
}

static VOID
OpenCommonStartMenuDirectory(IN HWND hWndOwner,
                             IN LPCTSTR lpOperation)
{
    TCHAR szDir[MAX_PATH];

    if (SHGetSpecialFolderPath(hWndOwner,
                               szDir,
                               CSIDL_COMMON_STARTMENU,
                               FALSE))
    {
        ShellExecute(hWndOwner,
                     lpOperation,
                     NULL,
                     NULL,
                     szDir,
                     SW_SHOWNORMAL);
    }
}

static VOID
OpenTaskManager(IN HWND hWndOwner)
{
    ShellExecute(hWndOwner,
                 TEXT("open"),
                 TEXT("taskmgr.exe"),
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
}

static BOOL STDMETHODCALLTYPE
ITrayWindowImpl_ExecContextMenuCmd(IN OUT ITrayWindow *iface,
                                   IN UINT uiCmd)
{
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);
    BOOL bHandled = TRUE;

    switch (uiCmd)
    {
        case ID_SHELL_CMD_PROPERTIES:
            ITrayWindow_DisplayProperties(iface);
            break;

        case ID_SHELL_CMD_OPEN_ALL_USERS:
            OpenCommonStartMenuDirectory(This->hWnd,
                                         TEXT("open"));
            break;

        case ID_SHELL_CMD_EXPLORE_ALL_USERS:
            OpenCommonStartMenuDirectory(This->hWnd,
                                         TEXT("explore"));
            break;

        case ID_LOCKTASKBAR:
            if (SHRestricted(REST_CLASSICSHELL) == 0)
            {
                ITrayWindow_Lock(iface,
                                 !This->Locked);
            }
            break;

        case ID_SHELL_CMD_OPEN_TASKMGR:
            OpenTaskManager(This->hWnd);
            break;


        default:
            DbgPrint("ITrayWindow::ExecContextMenuCmd(%u): Unhandled Command ID!\n", uiCmd);
            bHandled = FALSE;
            break;
    }

    return bHandled;
}

static BOOL STDMETHODCALLTYPE
ITrayWindowImpl_Lock(IN OUT ITrayWindow *iface,
                     IN BOOL bLock)
{
    BOOL bPrevLock;
    ITrayWindowImpl *This = impl_from_ITrayWindow(iface);

    bPrevLock = This->Locked;
    if (This->Locked != bLock)
    {
        This->Locked = bLock;

        if (This->TrayBandSite != NULL)
        {
            if (!SUCCEEDED(ITrayBandSite_Lock(This->TrayBandSite,
                                              bLock)))
            {
                /* Reset?? */
                This->Locked = bPrevLock;
            }
        }
    }

    return bPrevLock;
}

static const ITrayWindowVtbl ITrayWindowImpl_Vtbl =
{
    /* IUnknown */
    ITrayWindowImpl_QueryInterface,
    ITrayWindowImpl_AddRef,
    ITrayWindowImpl_Release,
    /* ITrayWindow */
    ITrayWindowImpl_Open,
    ITrayWindowImpl_Close,
    ITrayWindowImpl_GetHWND,
    ITrayWindowImpl_IsSpecialHWND,
    ITrayWindowImpl_IsHorizontal,
    ITrayWIndowImpl_GetCaptionFonts,
    ITrayWindowImpl_DisplayProperties,
    ITrayWindowImpl_ExecContextMenuCmd,
    ITrayWindowImpl_Lock
};

static LRESULT CALLBACK
TrayWndProc(IN HWND hwnd,
            IN UINT uMsg,
            IN WPARAM wParam,
            IN LPARAM lParam)
{
    ITrayWindowImpl *This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (ITrayWindowImpl*)GetWindowLongPtr(hwnd,
                                                  0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        if (This != NULL && This->StartMenuBand != NULL)
        {
            MSG Msg;
            LRESULT lRet;

            Msg.hwnd = hwnd;
            Msg.message = uMsg;
            Msg.wParam = wParam;
            Msg.lParam = lParam;

            if (IMenuBand_TranslateMenuMessage(This->StartMenuBand,
                                               &Msg,
                                               &lRet) == S_OK)
            {
                return lRet;
            }

            wParam = Msg.wParam;
            lParam = Msg.lParam;
        }

        switch (uMsg)
        {
            case WM_NCHITTEST:
            {
                RECT rcClient;
                POINT pt;

                if (This->Locked)
                {
                    /* The user may not be able to resize the tray window.
                       Pretend like the window is not sizeable when the user
                       clicks on the border. */
                    return HTBORDER;
                }

                if (GetClientRect(hwnd,
                                  &rcClient) &&
                    MapWindowPoints(hwnd,
                                    NULL,
                                    (LPPOINT)&rcClient,
                                    2) != 0)
                {
                    pt.x = (SHORT)LOWORD(lParam);
                    pt.y = (SHORT)HIWORD(lParam);

                    if (PtInRect(&rcClient,
                                 pt))
                    {
                        /* The user is trying to drag the tray window */
                        return HTCAPTION;
                    }

                    /* Depending on the position of the tray window, allow only
                       changing the border next to the monitor working area */
                    switch (This->Position)
                    {
                        case ABE_TOP:
                            if (pt.y > rcClient.bottom)
                                return HTBOTTOM;
                            break;

                        case ABE_BOTTOM:
                            if (pt.y < rcClient.top)
                                return HTTOP;
                            break;

                        case ABE_LEFT:
                            if (pt.x > rcClient.right)
                                return HTRIGHT;
                            break;

                        case ABE_RIGHT:
                            if (pt.x < rcClient.left)
                                return HTLEFT;
                            break;

                        default:
                            break;
                    }
                }

                return HTBORDER;
            }

            case WM_MOVING:
            {
                POINT ptCursor;
                PRECT pRect = (PRECT)lParam;

                /* We need to ensure that an application can not accidently
                   move the tray window (using SetWindowPos). However, we still
                   need to be able to move the window in case the user wants to
                   drag the tray window to another position or in case the user
                   wants to resize the tray window. */
                if (!This->Locked && GetCursorPos(&ptCursor))
                {
                    This->IsDragging = TRUE;
                    This->DraggingPosition = ITrayWindowImpl_GetDraggingRectFromPt(This,
                                                                                   ptCursor,
                                                                                   pRect,
                                                                                   &This->DraggingMonitor);
                }
                else
                {
                    *pRect = This->rcTrayWnd[This->Position];
                }
                return TRUE;
            }

            case WM_SIZING:
            {
                PRECT pRect = (PRECT)lParam;

                if (!This->Locked)
                {
                    ITrayWindowImpl_CalculateValidSize(This,
                                                       This->Position,
                                                       pRect);
                }
                else
                {
                    *pRect = This->rcTrayWnd[This->Position];
                }
                return TRUE;
            }

            case WM_WINDOWPOSCHANGING:
            {
                ITrayWindowImpl_ChangingWinPos(This,
                                               (LPWINDOWPOS)lParam);
                break;
            }

            case WM_SIZE:
            {
                RECT rcClient;

                if (wParam == SIZE_RESTORED && lParam == 0)
                {
                    /* Clip the tray window on multi monitor systems so the edges can't
                       overlap into another monitor */
                    ITrayWindowImpl_ApplyClipping(This,
                                                  TRUE);

                    if (!GetClientRect(This->hWnd,
                                       &rcClient))
                    {
                        break;
                    }
                }
                else
                {
                    rcClient.left = rcClient.top = 0;
                    rcClient.right = LOWORD(lParam);
                    rcClient.bottom = HIWORD(lParam);
                }

                ITrayWindowImpl_AlignControls(This,
                                              &rcClient);
                break;
            }

            case WM_ENTERSIZEMOVE:
                This->InSizeMove = TRUE;
                This->IsDragging = FALSE;
                if (!This->Locked)
                {
                    /* Remove the clipping on multi monitor systems while dragging around */
                    ITrayWindowImpl_ApplyClipping(This,
                                                  FALSE);
                }
                break;

            case WM_EXITSIZEMOVE:
                This->InSizeMove = FALSE;
                if (!This->Locked)
                {
                    /* Apply clipping */
                    PostMessage(This->hWnd,
                                WM_SIZE,
                                SIZE_RESTORED,
                                0);
                }
                break;

            case WM_SYSCHAR:
                switch (wParam)
                {
                    case TEXT(' '):
                    {
                        /* The user pressed Alt+Space, this usually brings up the system menu of a window.
                           The tray window needs to handle this specially, since it normally doesn't have
                           a system menu. */

                        static const UINT uidDisableItem[] = {
                            SC_RESTORE,
                            SC_MOVE,
                            SC_SIZE,
                            SC_MAXIMIZE,
                            SC_MINIMIZE,
                        };
                        HMENU hSysMenu;
                        INT i;
                        UINT uId;

                        /* temporarily enable the system menu */
                        SetWindowStyle(hwnd,
                                       WS_SYSMENU,
                                       WS_SYSMENU);

                        hSysMenu = GetSystemMenu(hwnd,
                                                 FALSE);
                        if (hSysMenu != NULL)
                        {
                            /* Disable all items that are not relevant */
                            for (i = 0; i != sizeof(uidDisableItem) / sizeof(uidDisableItem[0]); i++)
                            {
                                EnableMenuItem(hSysMenu,
                                               uidDisableItem[i],
                                               MF_BYCOMMAND | MF_GRAYED);
                            }

                            EnableMenuItem(hSysMenu,
                                           SC_CLOSE,
                                           MF_BYCOMMAND |
                                               (SHRestricted(REST_NOCLOSE) ? MF_GRAYED : MF_ENABLED));

                            /* Display the system menu */
                            uId = ITrayWindowImpl_TrackMenu(This,
                                                            hSysMenu,
                                                            NULL,
                                                            This->hwndStart,
                                                            This->Position != ABE_TOP,
                                                            FALSE);
                            if (uId != 0)
                            {
                                SendMessage(This->hWnd,
                                            WM_SYSCOMMAND,
                                            (WPARAM)uId,
                                            0);
                            }
                        }

                        /* revert the system menu window style */
                        SetWindowStyle(hwnd,
                                       WS_SYSMENU,
                                       0);
                        break;
                    }

                    default:
                        goto DefHandler;
                }
                break;

            case WM_NCRBUTTONUP:
                /* We want the user to be able to get a context menu even on the nonclient
                   area (including the sizing border)! */
                uMsg = WM_CONTEXTMENU;
                wParam = (WPARAM)hwnd;
                /* fall through */

            case WM_CONTEXTMENU:
            {
                POINT pt, *ppt = NULL;
                HWND hWndExclude = NULL;

                /* Check if the administrator has forbidden access to context menus */
                if (SHRestricted(REST_NOTRAYCONTEXTMENU))
                    break;

                pt.x = (SHORT)LOWORD(lParam);
                pt.y = (SHORT)HIWORD(lParam);

                if (pt.x != -1 || pt.y != -1)
                    ppt = &pt;
                else
                    hWndExclude = This->hwndStart;

                if ((HWND)wParam == This->hwndStart)
                {
                    /* Make sure we can't track the context menu if the start
                       menu is currently being shown */
                    if (!(SendMessage(This->hwndStart,
                                      BM_GETSTATE,
                                      0,
                                      0) & BST_PUSHED))
                    {
                        ITrayWindowImpl_TrackCtxMenu(This,
                                                     &StartMenuBtnCtxMenu,
                                                     ppt,
                                                     hWndExclude,
                                                     This->Position == ABE_BOTTOM,
                                                     This);
                    }
                }
                else
                {
                    /* See if the context menu should be handled by the task band site */
                    if (ppt != NULL && This->TrayBandSite != NULL)
                    {
                        HWND hWndAtPt;
                        POINT ptClient = *ppt;

                        /* Convert the coordinates to client-coordinates */
                        MapWindowPoints(NULL,
                                        This->hWnd,
                                        &ptClient,
                                        1);

                        hWndAtPt = ChildWindowFromPoint(This->hWnd,
                                                        ptClient);
                        if (hWndAtPt != NULL &&
                            (hWndAtPt == This->hwndRebar || IsChild(This->hwndRebar,
                                                                    hWndAtPt)))
                        {
                            /* Check if the user clicked on the task switch window */
                            ptClient = *ppt;
                            MapWindowPoints(NULL,
                                            This->hwndRebar,
                                            &ptClient,
                                            1);

                            hWndAtPt = ChildWindowFromPointEx(This->hwndRebar,
                                                              ptClient,
                                                              CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
                            if (hWndAtPt == This->hwndTaskSwitch)
                                goto HandleTrayContextMenu;

                            /* Forward the message to the task band site */
                            ITrayBandSite_ProcessMessage(This->TrayBandSite,
                                                         hwnd,
                                                         uMsg,
                                                         wParam,
                                                         lParam,
                                                         &Ret);
                        }
                        else
                            goto HandleTrayContextMenu;
                    }
                    else
                    {
HandleTrayContextMenu:
                        /* Tray the default tray window context menu */
                        ITrayWindowImpl_TrackCtxMenu(This,
                                                     &TrayWindowCtxMenu,
                                                     ppt,
                                                     NULL,
                                                     FALSE,
                                                     This);
                    }
                }
                break;
            }

            case WM_NOTIFY:
            {
                /* FIXME: We can't check with IsChild whether the hwnd is somewhere inside
                          the rebar control! But we shouldn't forward messages that the band
                          site doesn't handle, such as other controls (start button, tray window */
                if (This->TrayBandSite == NULL ||
                    !SUCCEEDED(ITrayBandSite_ProcessMessage(This->TrayBandSite,
                                                            hwnd,
                                                            uMsg,
                                                            wParam,
                                                            lParam,
                                                            &Ret)))
                {
                    const NMHDR *nmh = (const NMHDR *)lParam;

                    if (nmh->hwndFrom == This->hwndTrayNotify)
                    {
                        switch (nmh->code)
                        {
                            case NTNWM_REALIGN:
                                /* Cause all controls to be aligned */
                                PostMessage(This->hWnd,
                                            WM_SIZE,
                                            SIZE_RESTORED,
                                            0);
                                break;
                        }
                    }
                }
                break;
            }

            case WM_NCLBUTTONDBLCLK:
                /* We "handle" this message so users can't cause a weird maximize/restore
                   window animation when double-clicking the tray window! */
                break;

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (ITrayWindowImpl*)CreateStruct->lpCreateParams;

                if (InterlockedCompareExchangePointer((PVOID*)&This->hWnd,
                                                      (PVOID)hwnd,
                                                      NULL) != NULL)
                {
                    /* Somebody else was faster... */
                    return FALSE;
                }

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return ITrayWindowImpl_NCCreate(This);
            }

            case WM_CREATE:
                ITrayWindowImpl_Create(This);
                break;

            case WM_NCDESTROY:
                ITrayWindowImpl_Destroy(This);
                break;

            case WM_APP_TRAYDESTROY:
                DestroyWindow(hwnd);
                break;

            case WM_COMMAND:
                if ((HWND)lParam == This->hwndStart)
                {
                    if (This->StartMenuPopup != NULL)
                    {
                        POINTL pt;
                        RECTL rcExclude;
                        DWORD dwFlags = 0;

                        if (GetWindowRect(This->hwndStart,
                                          (RECT*)&rcExclude))
                        {
                            if (ITrayWindowImpl_IsPosHorizontal(This))
                            {
                                pt.x = rcExclude.left;
                                pt.y = rcExclude.top;
                                dwFlags |= MPPF_BOTTOM;
                            }
                            else
                            {
                                if (This->Position == ABE_LEFT)
                                    pt.x = rcExclude.left;
                                else
                                    pt.x = rcExclude.right;

                                pt.y = rcExclude.bottom;
                                dwFlags |= MPPF_BOTTOM;
                            }

                            IMenuPopup_Popup(This->StartMenuPopup,
                                             &pt,
                                             &rcExclude,
                                             dwFlags);
                        }
                    }
                    break;
                }

                if (This->TrayBandSite == NULL ||
                    !SUCCEEDED(ITrayBandSite_ProcessMessage(This->TrayBandSite,
                                                            hwnd,
                                                            uMsg,
                                                            wParam,
                                                            lParam,
                                                            &Ret)))
                {
                    switch(LOWORD(wParam))
                    {
                        /* FIXME: Handle these commands as well */
                        case IDM_TASKBARANDSTARTMENU:
                        case IDM_SEARCH:
                        case IDM_HELPANDSUPPORT:
                            break;

                        case IDM_RUN:
                        {
                            HANDLE hShell32;
                            RUNFILEDLG RunFileDlg;

                            hShell32 = GetModuleHandle(TEXT("SHELL32.DLL"));
                            RunFileDlg = (RUNFILEDLG)GetProcAddress(hShell32, (LPCSTR)61);

                            RunFileDlg(hwnd, NULL, NULL, NULL, NULL, RFF_CALCDIRECTORY);
                            break;
                        }

                        /* FIXME: Handle these commands as well */
                        case IDM_SYNCHRONIZE:
                        case IDM_LOGOFF:
                        case IDM_DISCONNECT:
                        case IDM_UNDOCKCOMPUTER:
                            break;

                        case IDM_SHUTDOWN:
                        {
                            HANDLE hShell32;
                            EXITWINDLG ExitWinDlg;

                            hShell32 = GetModuleHandle(TEXT("SHELL32.DLL"));
                            ExitWinDlg = (EXITWINDLG)GetProcAddress(hShell32, (LPCSTR)60);

                            ExitWinDlg(hwnd);
                            break;
                        }
                    }
                }
                break;

            default:
                goto DefHandler;
        }
    }
    else
    {
DefHandler:
        Ret = DefWindowProc(hwnd,
                            uMsg,
                            wParam,
                            lParam);
    }

    return Ret;
}

/*
 * Tray Window Context Menu
 */

static HMENU
CreateTrayWindowContextMenu(IN HWND hWndOwner,
                            IN PVOID *ppcmContext,
                            IN PVOID Context  OPTIONAL)
{
    ITrayWindowImpl *This = (ITrayWindowImpl *)Context;
    IContextMenu *pcm = NULL;
    HMENU hPopup;

    hPopup = LoadPopupMenu(hExplorerInstance,
                           MAKEINTRESOURCE(IDM_TRAYWND));

    if (hPopup != NULL)
    {
        if (SHRestricted(REST_CLASSICSHELL) != 0)
        {
            DeleteMenu(hPopup,
                       ID_LOCKTASKBAR,
                       MF_BYCOMMAND);
        }

        CheckMenuItem(hPopup,
                      ID_LOCKTASKBAR,
                      MF_BYCOMMAND | (This->Locked ? MF_CHECKED : MF_UNCHECKED));

        if (This->TrayBandSite != NULL)
        {
            if (SUCCEEDED(ITrayBandSite_AddContextMenus(This->TrayBandSite,
                                                        hPopup,
                                                        0,
                                                        ID_SHELL_CMD_FIRST,
                                                        ID_SHELL_CMD_LAST,
                                                        CMF_NORMAL,
                                                        &pcm)))
            {
                DbgPrint("ITrayBandSite::AddContextMenus succeeded!\n");
                *(IContextMenu **)ppcmContext = pcm;
            }
        }
    }

    return hPopup;
}

static VOID
OnTrayWindowContextMenuCommand(IN HWND hWndOwner,
                               IN UINT uiCmdId,
                               IN PVOID pcmContext  OPTIONAL,
                               IN PVOID Context  OPTIONAL)
{
    ITrayWindowImpl *This = (ITrayWindowImpl *)Context;
    IContextMenu *pcm = (IContextMenu *)pcmContext;

    if (uiCmdId != 0)
    {
        if (uiCmdId >= ID_SHELL_CMD_FIRST && uiCmdId <= ID_SHELL_CMD_LAST)
        {
            CMINVOKECOMMANDINFO cmici = {0};

            if (pcm != NULL)
            {
                /* Setup and invoke the shell command */
                cmici.cbSize = sizeof(cmici);
                cmici.hwnd = hWndOwner;
                cmici.lpVerb = (LPCSTR)MAKEINTRESOURCE(uiCmdId - ID_SHELL_CMD_FIRST);
                cmici.nShow = SW_NORMAL;

                IContextMenu_InvokeCommand(pcm,
                                           &cmici);
            }
        }
        else
        {
            ITrayWindow_ExecContextMenuCmd(ITrayWindow_from_impl(This),
                                           uiCmdId);
        }
    }

    if (pcm != NULL)
        IContextMenu_Release(pcm);
}

static const TRAYWINDOW_CTXMENU TrayWindowCtxMenu = {
    CreateTrayWindowContextMenu,
    OnTrayWindowContextMenuCommand
};

/*****************************************************************************/

BOOL
RegisterTrayWindowClass(VOID)
{
    WNDCLASS wcTrayWnd;
    BOOL Ret;

    if (!RegisterTrayNotifyWndClass())
        return FALSE;

    wcTrayWnd.style = CS_DBLCLKS;
    wcTrayWnd.lpfnWndProc = TrayWndProc;
    wcTrayWnd.cbClsExtra = 0;
    wcTrayWnd.cbWndExtra = sizeof(ITrayWindowImpl *);
    wcTrayWnd.hInstance = hExplorerInstance;
    wcTrayWnd.hIcon = NULL;
    wcTrayWnd.hCursor = LoadCursor(NULL,
                                   IDC_ARROW);
    wcTrayWnd.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayWnd.lpszMenuName = NULL;
    wcTrayWnd.lpszClassName = szTrayWndClass;

    Ret = RegisterClass(&wcTrayWnd) != 0;

    if (!Ret)
        UnregisterTrayNotifyWndClass();

    return Ret;
}

VOID
UnregisterTrayWindowClass(VOID)
{
    UnregisterTrayNotifyWndClass();

    UnregisterClass(szTrayWndClass,
                    hExplorerInstance);
}

ITrayWindow *
CreateTrayWindow(VOID)
{
    ITrayWindowImpl *This;
    ITrayWindow *TrayWindow;

    This = ITrayWindowImpl_Construct();
    if (This != NULL)
    {
        TrayWindow = ITrayWindow_from_impl(This);

        ITrayWindowImpl_Open(TrayWindow);

        return TrayWindow;
    }

    return NULL;
}

VOID
TrayProcessMessages(IN OUT ITrayWindow *Tray)
{
    ITrayWindowImpl *This;
    MSG Msg;

    This = impl_from_ITrayWindow(Tray);

    /* FIXME: We should keep a reference here... */

    while (PeekMessage(&Msg,
                       NULL,
                       0,
                       0,
                       PM_REMOVE))
    {
        if (Msg.message == WM_QUIT)
            break;

        if (This->StartMenuBand == NULL ||
            IMenuBand_IsMenuMessage(This->StartMenuBand,
                                    &Msg) != S_OK)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
}

VOID
TrayMessageLoop(IN OUT ITrayWindow *Tray)
{
    ITrayWindowImpl *This;
    MSG Msg;
    BOOL Ret;

    This = impl_from_ITrayWindow(Tray);

    /* FIXME: We should keep a reference here... */

    while (1)
    {
        Ret = (GetMessage(&Msg,
                          NULL,
                          0,
                          0) != 0);

        if (Ret != -1)
        {
            if (!Ret)
                break;

            if (This->StartMenuBand == NULL ||
                IMenuBand_IsMenuMessage(This->StartMenuBand,
                                        &Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }
}

/*
 * IShellDesktopTray
 *
 * NOTE: This is a very windows-specific COM interface used by SHCreateDesktop()!
 *       These are the calls I observed, it may be wrong/incomplete/buggy!!!
 *       The reason we implement it is because we have to use SHCreateDesktop() so
 *       that the shell provides the desktop window and all the features that come
 *       with it (especially positioning of desktop icons)
 */

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_QueryInterface(IN OUT IShellDesktopTray *iface,
                                                 IN REFIID riid,
                                                 OUT LPVOID *ppvObj)
{
    ITrayWindowImpl *This = impl_from_IShellDesktopTray(iface);
    ITrayWindow *tray = ITrayWindow_from_impl(This);

    DbgPrint("IShellDesktopTray::QueryInterface(0x%p, 0x%p)\n", riid, ppvObj);
    return ITrayWindowImpl_QueryInterface(tray,
                                          riid,
                                          ppvObj);
}

static ULONG STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_Release(IN OUT IShellDesktopTray *iface)
{
    ITrayWindowImpl *This = impl_from_IShellDesktopTray(iface);
    ITrayWindow *tray = ITrayWindow_from_impl(This);

    DbgPrint("IShellDesktopTray::Release()\n");
    return ITrayWindowImpl_Release(tray);
}

static ULONG STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_AddRef(IN OUT IShellDesktopTray *iface)
{
    ITrayWindowImpl *This = impl_from_IShellDesktopTray(iface);
    ITrayWindow *tray = ITrayWindow_from_impl(This);

    DbgPrint("IShellDesktopTray::AddRef()\n");
    return ITrayWindowImpl_AddRef(tray);
}

static ULONG STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_GetState(IN OUT IShellDesktopTray *iface)
{
    /* FIXME: Return ABS_ flags? */
    DbgPrint("IShellDesktopTray::GetState() unimplemented!\n");
    return 0;
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_GetTrayWindow(IN OUT IShellDesktopTray *iface,
                                                OUT HWND *phWndTray)
{
    ITrayWindowImpl *This = impl_from_IShellDesktopTray(iface);
    DbgPrint("IShellDesktopTray::GetTrayWindow(0x%p)\n", phWndTray);
    *phWndTray = This->hWnd;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_RegisterDesktopWindow(IN OUT IShellDesktopTray *iface,
                                                        IN HWND hWndDesktop)
{
    ITrayWindowImpl *This = impl_from_IShellDesktopTray(iface);
    DbgPrint("IShellDesktopTray::RegisterDesktopWindow(0x%p)\n", hWndDesktop);

    This->hWndDesktop = hWndDesktop;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITrayWindowImpl_IShellDesktopTray_Unknown(IN OUT IShellDesktopTray *iface,
                                          IN DWORD dwUnknown1,
                                          IN DWORD dwUnknown2)
{
    DbgPrint("IShellDesktopTray::Unknown(%u,%u) unimplemented!\n", dwUnknown1, dwUnknown2);
    return S_OK;
}

static const IShellDesktopTrayVtbl IShellDesktopTrayImpl_Vtbl =
{
    /*** IUnknown ***/
    ITrayWindowImpl_IShellDesktopTray_QueryInterface,
    ITrayWindowImpl_IShellDesktopTray_AddRef,
    ITrayWindowImpl_IShellDesktopTray_Release,
    /*** IShellDesktopTray ***/
    ITrayWindowImpl_IShellDesktopTray_GetState,
    ITrayWindowImpl_IShellDesktopTray_GetTrayWindow,
    ITrayWindowImpl_IShellDesktopTray_RegisterDesktopWindow,
    ITrayWindowImpl_IShellDesktopTray_Unknown
};
