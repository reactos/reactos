/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * this library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * this library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"
#include <commoncontrols.h>

HRESULT TrayWindowCtxMenuCreator(ITrayWindow * TrayWnd, IN HWND hWndOwner, IContextMenu ** ppCtxMenu);

#define WM_APP_TRAYDESTROY  (WM_APP + 0x100)

#define TIMER_ID_AUTOHIDE 1
#define TIMER_ID_MOUSETRACK 2
#define MOUSETRACK_INTERVAL 100
#define AUTOHIDE_DELAY_HIDE 2000
#define AUTOHIDE_DELAY_SHOW 50
#define AUTOHIDE_INTERVAL_ANIMATING 10

#define AUTOHIDE_SPEED_SHOW 10
#define AUTOHIDE_SPEED_HIDE 1

#define AUTOHIDE_HIDDEN 0
#define AUTOHIDE_SHOWING 1
#define AUTOHIDE_SHOWN 2
#define AUTOHIDE_HIDING 3

#define IDHK_RUN 0x1f4
#define IDHK_MINIMIZE_ALL 0x1f5
#define IDHK_RESTORE_ALL 0x1f6
#define IDHK_HELP 0x1f7
#define IDHK_EXPLORE 0x1f8
#define IDHK_FIND 0x1f9
#define IDHK_FIND_COMPUTER 0x1fa
#define IDHK_NEXT_TASK 0x1fb
#define IDHK_PREV_TASK 0x1fc
#define IDHK_SYS_PROPERTIES 0x1fd
#define IDHK_DESKTOP 0x1fe
#define IDHK_PAGER 0x1ff

static LONG TrayWndCount = 0;

static const WCHAR szTrayWndClass [] = TEXT("Shell_TrayWnd");

/*
 * ITrayWindow
 */

const GUID IID_IShellDesktopTray = { 0x213e2df9, 0x9a14, 0x4328, { 0x99, 0xb1, 0x69, 0x61, 0xf9, 0x14, 0x3c, 0xe9 } };

class CStartButton
    : public CContainedWindow
{
    HIMAGELIST m_ImageList;
    SIZE       m_Size;
    HFONT      m_Font;

public:
    CStartButton(CMessageMap *pObject, DWORD dwMsgMapID)
        : CContainedWindow(pObject, dwMsgMapID)
    {
    }

    virtual ~CStartButton()
    {
        if (m_ImageList != NULL)
            ImageList_Destroy(m_ImageList);

        if (m_Font != NULL)
            DeleteObject(m_Font);
    }

    HFONT GetFont()
    {
        return m_Font;
    }

    SIZE GetSize()
    {
        return m_Size;
    }

    VOID UpdateSize(IN HBITMAP hbmStart = NULL)
    {
        SIZE Size = { 0, 0 };

        if (m_ImageList == NULL ||
            !SendMessageW(BCM_GETIDEALSIZE, 0, (LPARAM) &Size))
        {
            Size.cx = 2 * GetSystemMetrics(SM_CXEDGE);
            Size.cy = 2 * GetSystemMetrics(SM_CYEDGE);

            if (hbmStart == NULL)
            {
                hbmStart = (HBITMAP) SendMessageW(BM_GETIMAGE, IMAGE_BITMAP, 0);
            }

            if (hbmStart != NULL)
            {
                BITMAP bmp;

                if (GetObject(hbmStart, sizeof(bmp), &bmp) != 0)
                {
                    Size.cx += bmp.bmWidth;
                    Size.cy += max(bmp.bmHeight, GetSystemMetrics(SM_CYCAPTION));
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
        m_Size = Size;
    }

    BOOL CreateImageList()
    {
        HICON hIconStart;
        SIZE IconSize;

        if (m_ImageList != NULL)
            return TRUE;

        IconSize.cx = GetSystemMetrics(SM_CXSMICON);
        IconSize.cy = GetSystemMetrics(SM_CYSMICON);

        /* Load the start button icon and create a image list for it */
        hIconStart = (HICON) LoadImage(hExplorerInstance,
                                       MAKEINTRESOURCE(IDI_START),
                                       IMAGE_ICON,
                                       IconSize.cx,
                                       IconSize.cy,
                                       LR_SHARED | LR_DEFAULTCOLOR);

        if (hIconStart == NULL)
            return FALSE;

        m_ImageList = ImageList_Create(IconSize.cx,
                                        IconSize.cy,
                                        ILC_COLOR32 | ILC_MASK,
                                        1, 1);
        if (m_ImageList == NULL)
            return FALSE;

        int s = ImageList_ReplaceIcon(m_ImageList, -1, hIconStart);
        if (s < 0)
        {
            /* Failed to add the icon! */
            ImageList_Destroy(m_ImageList);
            m_ImageList = NULL;

            return FALSE;
        }

        return TRUE;
    }

    HBITMAP CreateBitmap()
    {
        WCHAR szStartCaption[32];
        HFONT hFontOld;
        HDC hDC = NULL;
        HDC hDCScreen = NULL;
        SIZE Size, SmallIcon;
        HBITMAP hbmpOld, hbmp = NULL;
        HBITMAP hBitmap = NULL;
        HICON hIconStart;
        BOOL Ret;
        UINT Flags;
        RECT rcButton;

        /* NOTE: this is the backwards compatibility code that is used if the
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
        hIconStart = (HICON) LoadImage(hExplorerInstance,
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

        hFontOld = (HFONT) SelectObject(hDC, m_Font);

        Ret = GetTextExtentPoint32(hDC,
                                   szStartCaption,
                                   _tcslen(szStartCaption),
                                   &Size);

        SelectObject(hDC, hFontOld);
        if (!Ret)
            goto Cleanup;

        /* Make sure the height is at least the size of a caption icon. */
        if (hIconStart != NULL)
            Size.cx += SmallIcon.cx + 4;
        Size.cy = max(Size.cy, SmallIcon.cy);

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
        hbmpOld = (HBITMAP) SelectObject(hDC, hbmp);

        Flags = DC_TEXT | DC_INBUTTON;
        if (hIconStart != NULL)
            Flags |= DC_ICON;

        DrawCaptionTemp(NULL,
                        hDC,
                        &rcButton,
                        m_Font,
                        hIconStart,
                        szStartCaption,
                        Flags);

        SelectObject(hDC, hbmpOld);

        if (!Ret)
            goto Cleanup;

        /* We successfully created the bitmap! */
        hBitmap = hbmp;
        hbmp = NULL;

Cleanup:
        if (hDCScreen != NULL)
        {
            ReleaseDC(NULL, hDCScreen);
        }

        if (hbmp != NULL)
            DeleteObject(hbmp);

        if (hDC != NULL)
            DeleteDC(hDC);

        return hBitmap;
    }

    VOID Initialize()
    {
        NONCLIENTMETRICS ncm;

        SetWindowTheme(m_hWnd, L"Start", NULL);

        if (m_Font == NULL)
        {
            /* Get the system fonts, we use the caption font, always bold, though. */
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            {
                ncm.lfCaptionFont.lfWeight = FW_BOLD;
                m_Font = CreateFontIndirect(&ncm.lfCaptionFont);
            }
        }

        SendMessage(WM_SETFONT, (WPARAM) m_Font, FALSE);

        if (CreateImageList())
        {
            BUTTON_IMAGELIST bil;

            /* Try to set the start button image. this requires the Common
            Controls 6.0 to be present (XP and later) */
            bil.himl = m_ImageList;
            bil.margin.left = bil.margin.right = 1;
            bil.margin.top = bil.margin.bottom = 1;
            bil.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

            if (SendMessageW(BCM_SETIMAGELIST, 0, (LPARAM) &bil))
            {
                /* We're using the image list, remove the BS_BITMAP style and
                don't center it horizontally */
                SetWindowStyle(m_hWnd, BS_BITMAP | BS_RIGHT, 0);

                UpdateSize();
                return;
            }

            /* Fall back to the deprecated method on older systems that don't
            support Common Controls 6.0 */
            ImageList_Destroy(m_ImageList);
            m_ImageList = NULL;
        }

        HBITMAP hbmStart = CreateBitmap();
        if (hbmStart != NULL)
        {
            UpdateSize(hbmStart);

            HBITMAP hbmOld = (HBITMAP) SendMessageW(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmStart);

            if (hbmOld != NULL)
                DeleteObject(hbmOld);
        }
    }
};

class CTrayWindow :
    public CComCoClass<CTrayWindow>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayWindow, CWindow, CControlWinTraits >,
    public ITrayWindow,
    public IShellDesktopTray
{
    CStartButton m_StartButton;

    CComPtr<IMenuBand>  m_StartMenuBand;
    CComPtr<IMenuPopup> m_StartMenuPopup;

    HTHEME m_Theme;

    HFONT m_CaptionFont;
    HFONT m_Font;

    HWND m_DesktopWnd;
    HWND m_Rebar;
    HWND m_TaskSwitch;
    HWND m_TrayNotify;

    CTrayNotifyWnd* m_TrayNotifyInstance;

    DWORD    m_Position;
    HMONITOR m_Monitor;
    HMONITOR m_PreviousMonitor;
    DWORD    m_DraggingPosition;
    HMONITOR m_DraggingMonitor;

    RECT m_TrayRects[4];
    SIZE m_TraySize;

    HWND m_TrayPropertiesOwner;
    HWND m_RunFileDlgOwner;

    UINT m_AutoHideState;
    SIZE m_AutoHideOffset;
    TRACKMOUSEEVENT m_MouseTrackingInfo;

    HDPA m_ShellServices;

public:
    CComPtr<ITrayBandSite> m_TrayBandSite;

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

public:
    CTrayWindow() :
        m_StartButton(this, 1),
        m_Theme(NULL),
        m_CaptionFont(NULL),
        m_Font(NULL),
        m_DesktopWnd(NULL),
        m_Rebar(NULL),
        m_TaskSwitch(NULL),
        m_TrayNotify(NULL),
        m_Position(0),
        m_Monitor(NULL),
        m_PreviousMonitor(NULL),
        m_DraggingPosition(0),
        m_DraggingMonitor(NULL),
        m_TrayPropertiesOwner(NULL),
        m_RunFileDlgOwner(NULL),
        m_AutoHideState(NULL),
        m_ShellServices(NULL),
        Flags(0)
    {
        ZeroMemory(&m_TrayRects, sizeof(m_TrayRects));
        ZeroMemory(&m_TraySize, sizeof(m_TraySize));
        ZeroMemory(&m_AutoHideOffset, sizeof(m_AutoHideOffset));
        ZeroMemory(&m_MouseTrackingInfo, sizeof(m_MouseTrackingInfo));
    }

    virtual ~CTrayWindow()
    {
        if (m_ShellServices != NULL)
        {
            ShutdownShellServices(m_ShellServices);
            m_ShellServices = NULL;
        }

        if (m_CaptionFont != NULL)
        {
            DeleteObject(m_CaptionFont);
            m_CaptionFont = NULL;
        }

        if (m_Font != NULL)
        {
            DeleteObject(m_Font);
            m_Font = NULL;
        }

        if (m_Theme)
        {
            CloseThemeData(m_Theme);
            m_Theme = NULL;
        }

        if (InterlockedDecrement(&TrayWndCount) == 0)
            PostQuitMessage(0);
    }

    /*
     * ITrayWindow
     */

    BOOL UpdateNonClientMetrics()
    {
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            if (m_Font != NULL)
                DeleteObject(m_Font);

            m_Font = CreateFontIndirect(&ncm.lfMessageFont);
            return TRUE;
        }

        return FALSE;
    }

    VOID SetWindowsFont()
    {
        if (m_TrayNotify != NULL)
        {
            SendMessage(m_TrayNotify, WM_SETFONT, (WPARAM) m_Font, TRUE);
        }
    }

    HMONITOR GetScreenRectFromRect(
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

    HMONITOR GetMonitorFromRect(
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

    HMONITOR GetScreenRect(
        IN HMONITOR hMonitor,
        IN OUT RECT *pRect)
    {
        HMONITOR hMon = NULL;

        if (hMonitor != NULL)
        {
            MONITORINFO mi;

            mi.cbSize = sizeof(mi);
            if (!GetMonitorInfo(hMonitor,
                &mi))
            {
                /* Hm, the monitor is gone? Try to find a monitor where it
                   could be located now */
                hMon = GetMonitorFromRect(
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

    VOID MakeTrayRectWithSize(IN DWORD Position,
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

    VOID GetTrayRectFromScreenRect(IN DWORD Position,
                                   IN const RECT *pScreen,
                                   IN const SIZE *pTraySize OPTIONAL,
                                   OUT RECT *pRect)
    {
        if (pTraySize == NULL)
            pTraySize = &m_TraySize;

        *pRect = *pScreen;

        /* Move the border outside of the screen */
        InflateRect(pRect,
                    GetSystemMetrics(SM_CXEDGE),
                    GetSystemMetrics(SM_CYEDGE));

        MakeTrayRectWithSize(Position, pTraySize, pRect);
    }

    BOOL IsPosHorizontal()
    {
        return m_Position == ABE_TOP || m_Position == ABE_BOTTOM;
    }

    HMONITOR CalculateValidSize(
        IN DWORD Position,
        IN OUT RECT *pRect)
    {
        RECT rcScreen;
        //BOOL Horizontal;
        HMONITOR hMon;
        SIZE szMax, szWnd;

        //Horizontal = IsPosHorizontal();

        szWnd.cx = pRect->right - pRect->left;
        szWnd.cy = pRect->bottom - pRect->top;

        rcScreen = *pRect;
        hMon = GetScreenRectFromRect(
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

        GetTrayRectFromScreenRect(
            Position,
            &rcScreen,
            &szWnd,
            pRect);

        return hMon;
    }

#if 0
    VOID
        GetMinimumWindowSize(
        OUT RECT *pRect)
    {
        RECT rcMin = {0};

        AdjustWindowRectEx(&rcMin,
                           GetWindowLong(m_hWnd,
                           GWL_STYLE),
                           FALSE,
                           GetWindowLong(m_hWnd,
                           GWL_EXSTYLE));

        *pRect = rcMin;
    }
#endif


    DWORD GetDraggingRectFromPt(
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

        RECT rcPos = m_TrayRects[Pos];

        hMonNew = GetMonitorFromRect(&rcPos);
        if (hMon != hMonNew)
        {
            SIZE szTray;

            /* Recalculate the rectangle, we're dragging to another monitor.
               We don't need to recalculate the rect on single monitor systems. */
            szTray.cx = rcPos.right - rcPos.left;
            szTray.cy = rcPos.bottom - rcPos.top;

            GetTrayRectFromScreenRect(Pos, &rcScreen, &szTray, pRect);
            if (AutoHide)
            {
                pRect->left += m_AutoHideOffset.cx;
                pRect->right += m_AutoHideOffset.cx;
                pRect->top += m_AutoHideOffset.cy;
                pRect->bottom += m_AutoHideOffset.cy;
            }
            hMon = hMonNew;
        }
        else
        {
            /* The user is dragging the tray window on the same monitor. We don't need
               to recalculate the rectangle */
            *pRect = rcPos;
            if (AutoHide)
            {
                pRect->left += m_AutoHideOffset.cx;
                pRect->right += m_AutoHideOffset.cx;
                pRect->top += m_AutoHideOffset.cy;
                pRect->bottom += m_AutoHideOffset.cy;
            }
        }

        *phMonitor = hMon;

        return Pos;
    }

    DWORD GetDraggingRectFromRect(
        IN OUT RECT *pRect,
        OUT HMONITOR *phMonitor)
    {
        POINT pt;

        /* Calculate the center of the rectangle. We call
           GetDraggingRectFromPt to calculate a valid
           dragging rectangle */
        pt.x = pRect->left + ((pRect->right - pRect->left) / 2);
        pt.y = pRect->top + ((pRect->bottom - pRect->top) / 2);

        return GetDraggingRectFromPt(
            pt,
            pRect,
            phMonitor);
    }

    VOID ChangingWinPos(IN OUT LPWINDOWPOS pwp)
    {
        RECT rcTray;

        if (IsDragging)
        {
            rcTray.left = pwp->x;
            rcTray.top = pwp->y;
            rcTray.right = rcTray.left + pwp->cx;
            rcTray.bottom = rcTray.top + pwp->cy;
            if (AutoHide)
            {
                rcTray.left -= m_AutoHideOffset.cx;
                rcTray.right -= m_AutoHideOffset.cx;
                rcTray.top -= m_AutoHideOffset.cy;
                rcTray.bottom -= m_AutoHideOffset.cy;
            }

            if (!EqualRect(&rcTray,
                &m_TrayRects[m_DraggingPosition]))
            {
                /* Recalculate the rectangle, the user dragged the tray
                   window to another monitor or the window was somehow else
                   moved or resized */
                m_DraggingPosition = GetDraggingRectFromRect(
                    &rcTray,
                    &m_DraggingMonitor);
                //m_TrayRects[DraggingPosition] = rcTray;
            }

            //Monitor = CalculateValidSize(
            //                                                   DraggingPosition,
            //                                                   &rcTray);

            m_Monitor = m_DraggingMonitor;
            m_Position = m_DraggingPosition;
            IsDragging = FALSE;

            m_TrayRects[m_Position] = rcTray;
            goto ChangePos;
        }
        else if (GetWindowRect(m_hWnd, &rcTray))
        {
            if (InSizeMove)
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

                m_Position = GetDraggingRectFromRect(&rcTray, &m_Monitor);

                if (!(pwp->flags & (SWP_NOMOVE | SWP_NOSIZE)))
                {
                    SIZE szWnd;

                    szWnd.cx = pwp->cx;
                    szWnd.cy = pwp->cy;

                    MakeTrayRectWithSize(m_Position, &szWnd, &rcTray);
                }

                if (AutoHide)
                {
                    rcTray.left -= m_AutoHideOffset.cx;
                    rcTray.right -= m_AutoHideOffset.cx;
                    rcTray.top -= m_AutoHideOffset.cy;
                    rcTray.bottom -= m_AutoHideOffset.cy;
                }
                m_TrayRects[m_Position] = rcTray;
            }
            else
            {
                /* If the user isn't resizing the tray window we need to make sure the
                   new size or position is valid. this is to prevent changes to the window
                   without user interaction. */
                rcTray = m_TrayRects[m_Position];
            }

ChangePos:
            m_TraySize.cx = rcTray.right - rcTray.left;
            m_TraySize.cy = rcTray.bottom - rcTray.top;

            if (AutoHide)
            {
                rcTray.left += m_AutoHideOffset.cx;
                rcTray.right += m_AutoHideOffset.cx;
                rcTray.top += m_AutoHideOffset.cy;
                rcTray.bottom += m_AutoHideOffset.cy;
            }

            pwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            pwp->x = rcTray.left;
            pwp->y = rcTray.top;
            pwp->cx = m_TraySize.cx;
            pwp->cy = m_TraySize.cy;
        }
    }

    VOID ApplyClipping(IN BOOL Clip)
    {
        RECT rcClip, rcWindow;
        HRGN hClipRgn;

        if (GetWindowRect(m_hWnd, &rcWindow))
        {
            /* Disable clipping on systems with only one monitor */
            if (GetSystemMetrics(SM_CMONITORS) <= 1)
                Clip = FALSE;

            if (Clip)
            {
                rcClip = rcWindow;

                GetScreenRect(m_Monitor, &rcClip);

                if (!IntersectRect(&rcClip, &rcClip, &rcWindow))
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
            SetWindowRgn(m_hWnd, hClipRgn, TRUE);
        }
    }

    VOID ResizeWorkArea()
    {
#if !WIN7_COMPAT_MODE
        RECT rcTray, rcWorkArea;

        /* If monitor has changed then fix the previous monitors work area */
        if (m_PreviousMonitor != m_Monitor)
        {
            GetScreenRect(m_PreviousMonitor, &rcWorkArea);
            SystemParametersInfo(SPI_SETWORKAREA,
                                 1,
                                 &rcWorkArea,
                                 SPIF_SENDCHANGE);
        }

        rcTray = m_TrayRects[m_Position];

        GetScreenRect(m_Monitor, &rcWorkArea);
        m_PreviousMonitor = m_Monitor;

        /* If AutoHide is false then change the workarea to exclude the area that
           the taskbar covers. */
        if (!AutoHide)
        {
            switch (m_Position)
            {
            case ABE_TOP:
                rcWorkArea.top = rcTray.bottom;
                break;
            case ABE_LEFT:
                rcWorkArea.left = rcTray.right;
                break;
            case ABE_RIGHT:
                rcWorkArea.right = rcTray.left;
                break;
            case ABE_BOTTOM:
                rcWorkArea.bottom = rcTray.top;
                break;
            }
        }

        SystemParametersInfo(SPI_SETWORKAREA,
                             1,
                             &rcWorkArea,
                             SPIF_SENDCHANGE);
#endif
    }

    VOID CheckTrayWndPosition()
    {
        RECT rcTray;

        rcTray = m_TrayRects[m_Position];

        if (AutoHide)
        {
            rcTray.left += m_AutoHideOffset.cx;
            rcTray.right += m_AutoHideOffset.cx;
            rcTray.top += m_AutoHideOffset.cy;
            rcTray.bottom += m_AutoHideOffset.cy;
        }

        /* Move the tray window */
        SetWindowPos(NULL,
                     rcTray.left,
                     rcTray.top,
                     rcTray.right - rcTray.left,
                     rcTray.bottom - rcTray.top,
                     SWP_NOZORDER);

        ResizeWorkArea();

        ApplyClipping(TRUE);
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

    VOID RegLoadSettings()
    {
        DWORD Pos;
        TW_STRUCKRECTS2 sr;
        RECT rcScreen;
        SIZE WndSize, EdgeSize, DlgFrameSize;
        DWORD cbSize = sizeof(sr);
        SIZE StartBtnSize = m_StartButton.GetSize();

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
            AutoHide = (sr.dwFlags & ABS_AUTOHIDE) != 0;
            AlwaysOnTop = (sr.dwFlags & ABS_ALWAYSONTOP) != 0;
            SmSmallIcons = (sr.dwFlags & 0x4) != 0;
            HideClock = (sr.dwFlags & 0x8) != 0;

            /* FIXME: Are there more flags? */

#if WIN7_COMPAT_MODE
            m_Position = ABE_LEFT;
#else
            if (sr.Position > ABE_BOTTOM)
                m_Position = ABE_BOTTOM;
            else
                m_Position = sr.Position;
#endif

            /* Try to find out which monitor the tray window was located on last.
               Here we're only interested in the monitor screen that we think
               is the last one used. We're going to determine on which monitor
               we really are after calculating the docked position. */
            rcScreen = sr.Rect;
            GetScreenRectFromRect(
                &rcScreen,
                MONITOR_DEFAULTTONEAREST);
        }
        else
        {
            m_Position = ABE_BOTTOM;
            AlwaysOnTop = TRUE;

            /* Use the minimum size of the taskbar, we'll use the start
               button as a minimum for now. Make sure we calculate the
               entire window size, not just the client size. However, we
               use a thinner border than a standard thick border, so that
               the start button and bands are not stuck to the screen border. */
            sr.Size.cx = StartBtnSize.cx + (2 * (EdgeSize.cx + DlgFrameSize.cx));
            sr.Size.cy = StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));

            /* Use the primary screen by default */
            rcScreen.left = 0;
            rcScreen.top = 0;
            rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
            rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);
            GetScreenRectFromRect(
                &rcScreen,
                MONITOR_DEFAULTTOPRIMARY);
        }

        if (m_hWnd != NULL)
            SetWindowPos(
            AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE);

        /* Determine a minimum tray window rectangle. The "client" height is
           zero here since we cannot determine an optimal minimum width when
           loaded as a vertical tray window. We just need to make sure the values
           loaded from the registry are at least. The windows explorer behaves
           the same way, it allows the user to save a zero width vertical tray
           window, but not a zero height horizontal tray window. */
        WndSize.cx = 2 * (EdgeSize.cx + DlgFrameSize.cx);
        WndSize.cy = StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));

        if (WndSize.cx < sr.Size.cx)
            WndSize.cx = sr.Size.cx;
        if (WndSize.cy < sr.Size.cy)
            WndSize.cy = sr.Size.cy;

        /* Save the calculated size */
        m_TraySize = WndSize;

        /* Calculate all docking rectangles. We need to do this here so they're
           initialized and dragging the tray window to another position gives
           usable results */
        for (Pos = ABE_LEFT;
             Pos <= ABE_BOTTOM;
             Pos++)
        {
            GetTrayRectFromScreenRect(
                Pos,
                &rcScreen,
                &m_TraySize,
                &m_TrayRects[Pos]);
            //        TRACE("m_TrayRects[%d(%d)]: %d,%d,%d,%d\n", Pos, Position, m_TrayRects[Pos].left, m_TrayRects[Pos].top, m_TrayRects[Pos].right, m_TrayRects[Pos].bottom);
        }

        /* Determine which monitor we are on. It shouldn't matter which docked
           position rectangle we use */
        m_Monitor = GetMonitorFromRect(&m_TrayRects[ABE_LEFT]);
    }

    UINT TrackMenu(
        IN HMENU hMenu,
        IN POINT *ppt OPTIONAL,
        IN HWND hwndExclude OPTIONAL,
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
            if (::GetClientRect(hwndExclude,
                &tmp.rcExclude) &&
                MapWindowPoints(hwndExclude,
                NULL,
                (LPPOINT) &tmp.rcExclude,
                2) != 0)
            {
                ptmp = &tmp;
            }
        }

        if (ppt == NULL)
        {
            if (ptmp == NULL &&
                GetClientRect(&tmp.rcExclude) &&
                MapWindowPoints(m_hWnd,
                NULL,
                (LPPOINT) &tmp.rcExclude,
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
                                 m_hWnd,
                                 ptmp);

        return cmdId;
    }

    HRESULT TrackCtxMenu(
        IN IContextMenu * contextMenu,
        IN POINT *ppt OPTIONAL,
        IN HWND hwndExclude OPTIONAL,
        IN BOOL TrackUp,
        IN PVOID Context OPTIONAL)
    {
        INT x = ppt->x;
        INT y = ppt->y;
        HRESULT hr;
        UINT uCommand;
        HMENU popup = CreatePopupMenu();

        if (popup == NULL)
            return E_FAIL;

        TRACE("Before Query\n");
        hr = contextMenu->QueryContextMenu(popup, 0, 0, UINT_MAX, CMF_NORMAL);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            TRACE("Query failed\n");
            DestroyMenu(popup);
            return hr;
        }
        
        TRACE("Before Tracking\n");
        uCommand = ::TrackPopupMenuEx(popup, TPM_RETURNCMD, x, y, m_hWnd, NULL);

        if (uCommand != 0)
        {
            TRACE("Before InvokeCommand\n");
            CMINVOKECOMMANDINFO cmi = { 0 };
            cmi.cbSize = sizeof(cmi);
            cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
            cmi.hwnd = m_hWnd;
            hr = contextMenu->InvokeCommand(&cmi);
        }
        else
        {
            TRACE("TrackPopupMenu failed. Code=%d, LastError=%d\n", uCommand, GetLastError());
            hr = S_FALSE;
        }

        DestroyMenu(popup);
        return hr;
    }

    VOID AlignControls(IN PRECT prcClient OPTIONAL)
    {
        RECT rcClient;
        SIZE TraySize, StartSize;
        POINT ptTrayNotify = { 0, 0 };
        BOOL Horizontal;
        HDWP dwp;

        m_StartButton.UpdateSize();
        if (prcClient != NULL)
        {
            rcClient = *prcClient;
        }
        else
        {
            if (!GetClientRect(&rcClient))
            {
                return;
            }
        }

        Horizontal = IsPosHorizontal();

        /* We're about to resize/move the start button, the rebar control and
           the tray notification control */
        dwp = BeginDeferWindowPos(3);
        if (dwp == NULL)
            return;

        /* Limit the Start button width to the client width, if neccessary */
        StartSize = m_StartButton.GetSize();
        if (StartSize.cx > rcClient.right)
            StartSize.cx = rcClient.right;

        if (m_StartButton.m_hWnd != NULL)
        {
            /* Resize and reposition the button */
            dwp = DeferWindowPos(dwp,
                                 m_StartButton.m_hWnd,
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

        if (m_TrayNotify != NULL &&
            SendMessage(m_TrayNotify,
            TNWM_GETMINIMUMSIZE,
            (WPARAM) Horizontal,
            (LPARAM) &TraySize))
        {
            /* Move the tray notification window to the desired location */
            if (Horizontal)
                ptTrayNotify.x = rcClient.right - TraySize.cx;
            else
                ptTrayNotify.y = rcClient.bottom - TraySize.cy;

            dwp = DeferWindowPos(dwp,
                                 m_TrayNotify,
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
        if (m_Rebar != NULL)
        {
            POINT ptRebar = { 0, 0 };
            SIZE szRebar;

            SetWindowStyle(m_Rebar, CCS_VERT, Horizontal ? 0 : CCS_VERT);

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
                                 m_Rebar,
                                 NULL,
                                 ptRebar.x,
                                 ptRebar.y,
                                 szRebar.cx,
                                 szRebar.cy,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (dwp != NULL)
            EndDeferWindowPos(dwp);

        if (m_TaskSwitch != NULL)
        {
            /* Update the task switch window configuration */
            SendMessage(m_TaskSwitch, TSWM_UPDATETASKBARPOS, 0, 0);
        }
    }

    LRESULT OnThemeChanged()
    {
        if (m_Theme)
            CloseThemeData(m_Theme);

        if (IsThemeActive())
            m_Theme = OpenThemeData(m_hWnd, L"TaskBar");
        else
            m_Theme = NULL;

        if (Locked && m_Theme)
        {
            SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, 0);
        }
        else
        {
            SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, WS_THICKFRAME | WS_BORDER);
        }
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        WCHAR szStartCaption[32];

        ((ITrayWindow*)this)->AddRef();

        SetWindowTheme(m_hWnd, L"TaskBar", NULL);
        OnThemeChanged();

        InterlockedIncrement(&TrayWndCount);

        if (!LoadString(hExplorerInstance,
            IDS_START,
            szStartCaption,
            sizeof(szStartCaption) / sizeof(szStartCaption[0])))
        {
            szStartCaption[0] = TEXT('\0');
        }

        if (m_CaptionFont == NULL)
        {
            NONCLIENTMETRICS ncm;

            /* Get the system fonts, we use the caption font,
               always bold, though. */
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            {
                if (m_CaptionFont == NULL)
                {
                    ncm.lfCaptionFont.lfWeight = FW_NORMAL;
                    m_CaptionFont = CreateFontIndirect(&ncm.lfCaptionFont);
                }
            }
        }
        
        /* Create the Start button */
        m_StartButton.SubclassWindow(CreateWindowEx(
            0,
            WC_BUTTON,
            szStartCaption,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
            BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | BS_BITMAP,
            0,
            0,
            0,
            0,
            m_hWnd,
            (HMENU) IDC_STARTBTN,
            hExplorerInstance,
            NULL));

        if (m_StartButton.m_hWnd)
        {
            m_StartButton.Initialize();
        }

        /* Load the saved tray window settings */
        RegLoadSettings();

        /* Create and initialize the start menu */
        HBITMAP hbmBanner = LoadBitmap(hExplorerInstance, MAKEINTRESOURCE(IDB_STARTMENU));
        m_StartMenuPopup = CreateStartMenu(this, &m_StartMenuBand, hbmBanner, 0);

        /* Load the tray band site */
        if (m_TrayBandSite != NULL)
        {
            m_TrayBandSite.Release();
        }

        m_TrayBandSite = CreateTrayBandSite(this, &m_Rebar, &m_TaskSwitch);
        SetWindowTheme(m_Rebar, L"TaskBar", NULL);

        /* Create the tray notification window */
        m_TrayNotify = CreateTrayNotifyWnd(this, HideClock, &m_TrayNotifyInstance);

        if (UpdateNonClientMetrics())
        {
            SetWindowsFont();
        }

        /* Move the tray window to the right position and resize it if neccessary */
        CheckTrayWndPosition();

        /* Align all controls on the tray window */
        AlignControls(NULL);

        InitShellServices(&(m_ShellServices));

        if (AutoHide)
        {
            m_AutoHideState = AUTOHIDE_HIDING;
            SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_HIDE, NULL);
        }

        RegisterHotKey(m_hWnd, IDHK_RUN, MOD_WIN, 'R');
        RegisterHotKey(m_hWnd, IDHK_MINIMIZE_ALL, MOD_WIN, 'M');
        RegisterHotKey(m_hWnd, IDHK_RESTORE_ALL, MOD_WIN|MOD_SHIFT, 'M');
        RegisterHotKey(m_hWnd, IDHK_HELP, MOD_WIN, VK_F1);
        RegisterHotKey(m_hWnd, IDHK_EXPLORE, MOD_WIN, 'E');
        RegisterHotKey(m_hWnd, IDHK_FIND, MOD_WIN, 'F');
        RegisterHotKey(m_hWnd, IDHK_FIND_COMPUTER, MOD_WIN|MOD_CONTROL, 'F');
        RegisterHotKey(m_hWnd, IDHK_NEXT_TASK, MOD_WIN, VK_TAB);
        RegisterHotKey(m_hWnd, IDHK_PREV_TASK, MOD_WIN|MOD_SHIFT, VK_TAB);
        RegisterHotKey(m_hWnd, IDHK_SYS_PROPERTIES, MOD_WIN, VK_PAUSE);
        RegisterHotKey(m_hWnd, IDHK_DESKTOP, MOD_WIN, 'D');
        RegisterHotKey(m_hWnd, IDHK_PAGER, MOD_WIN, 'B');

        return TRUE;
    }

    HRESULT STDMETHODCALLTYPE Open()
    {
        RECT rcWnd;

        /* Check if there's already a window created and try to show it.
           If it was somehow destroyed just create a new tray window. */
        if (m_hWnd != NULL && IsWindow())
        {
            if (!IsWindowVisible(m_hWnd))
            {
                CheckTrayWndPosition();

                ShowWindow(SW_SHOW);
            }

            return S_OK;
        }

        DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;
        if (AlwaysOnTop)
            dwExStyle |= WS_EX_TOPMOST;

        DWORD dwStyle = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            WS_BORDER | WS_THICKFRAME;

        ZeroMemory(&rcWnd, sizeof(rcWnd));
        if (m_Position != (DWORD) -1)
            rcWnd = m_TrayRects[m_Position];

        if (!Create(NULL, rcWnd, NULL, dwStyle, dwExStyle))
            return E_FAIL;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Close()
    {
        if (m_hWnd != NULL)
        {
            SendMessage(m_hWnd,
                        WM_APP_TRAYDESTROY,
                        0,
                        0);
        }

        return S_OK;
    }

    HWND STDMETHODCALLTYPE GetHWND()
    {
        return m_hWnd;
    }

    BOOL STDMETHODCALLTYPE IsSpecialHWND(IN HWND hWnd)
    {
        return (m_hWnd == hWnd ||
                (m_DesktopWnd != NULL && m_hWnd == m_DesktopWnd));
    }

    BOOL STDMETHODCALLTYPE
        IsHorizontal()
    {
        return IsPosHorizontal();
    }

    HFONT STDMETHODCALLTYPE GetCaptionFonts(OUT HFONT *phBoldCaption OPTIONAL)
    {
        if (phBoldCaption != NULL)
            *phBoldCaption = m_StartButton.GetFont();

        return m_CaptionFont;
    }

    DWORD WINAPI TrayPropertiesThread()
    {
        HWND hwnd;
        RECT posRect;

        GetWindowRect(m_StartButton.m_hWnd, &posRect);
        hwnd = CreateWindowEx(0,
                              WC_STATIC,
                              NULL,
                              WS_OVERLAPPED | WS_DISABLED | WS_CLIPSIBLINGS | WS_BORDER | SS_LEFT,
                              posRect.left,
                              posRect.top,
                              posRect.right - posRect.left,
                              posRect.bottom - posRect.top,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

        m_TrayPropertiesOwner = hwnd;

        DisplayTrayProperties(hwnd);

        m_TrayPropertiesOwner = NULL;
        DestroyWindow();

        return 0;
    }

    static DWORD WINAPI s_TrayPropertiesThread(IN OUT PVOID pParam)
    {
        CTrayWindow *This = (CTrayWindow*) pParam;

        return This->TrayPropertiesThread();
    }

    HWND STDMETHODCALLTYPE DisplayProperties()
    {
        HWND hTrayProp;

        if (m_TrayPropertiesOwner)
        {
            hTrayProp = GetLastActivePopup(m_TrayPropertiesOwner);
            if (hTrayProp != NULL &&
                hTrayProp != m_TrayPropertiesOwner)
            {
                SetForegroundWindow(hTrayProp);
                return NULL;
            }
        }

        CloseHandle(CreateThread(NULL, 0, s_TrayPropertiesThread, this, 0, NULL));
        return NULL;
    }

    VOID OpenCommonStartMenuDirectory(IN HWND hWndOwner, IN LPCTSTR lpOperation)
    {
        WCHAR szDir[MAX_PATH];

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

    VOID OpenTaskManager(IN HWND hWndOwner)
    {
        ShellExecute(hWndOwner,
                     TEXT("open"),
                     TEXT("taskmgr.exe"),
                     NULL,
                     NULL,
                     SW_SHOWNORMAL);
    }

    BOOL STDMETHODCALLTYPE ExecContextMenuCmd(IN UINT uiCmd)
    {
        BOOL bHandled = TRUE;

        switch (uiCmd)
        {
        case ID_SHELL_CMD_PROPERTIES:
            DisplayProperties();
            break;

        case ID_SHELL_CMD_OPEN_ALL_USERS:
            OpenCommonStartMenuDirectory(m_hWnd,
                                         TEXT("open"));
            break;

        case ID_SHELL_CMD_EXPLORE_ALL_USERS:
            OpenCommonStartMenuDirectory(m_hWnd,
                                         TEXT("explore"));
            break;

        case ID_LOCKTASKBAR:
            if (SHRestricted(REST_CLASSICSHELL) == 0)
            {
                Lock(!Locked);
            }
            break;

        case ID_SHELL_CMD_OPEN_TASKMGR:
            OpenTaskManager(m_hWnd);
            break;

        case ID_SHELL_CMD_UNDO_ACTION:
            break;

        case ID_SHELL_CMD_SHOW_DESKTOP:
            break;

        case ID_SHELL_CMD_TILE_WND_H:
            TileWindows(NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
            break;

        case ID_SHELL_CMD_TILE_WND_V:
            TileWindows(NULL, MDITILE_VERTICAL, NULL, 0, NULL);
            break;

        case ID_SHELL_CMD_CASCADE_WND:
            CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, 0, NULL);
            break;

        case ID_SHELL_CMD_CUST_NOTIF:
            break;

        case ID_SHELL_CMD_ADJUST_DAT:
            //FIXME: Use SHRunControlPanel
            ShellExecuteW(m_hWnd, NULL, L"timedate.cpl", NULL, NULL, SW_NORMAL);
            break;

        default:
            TRACE("ITrayWindow::ExecContextMenuCmd(%u): Unhandled Command ID!\n", uiCmd);
            bHandled = FALSE;
            break;
        }

        return bHandled;
    }

    BOOL STDMETHODCALLTYPE Lock(IN BOOL bLock)
    {
        BOOL bPrevLock = Locked;

        if (Locked != bLock)
        {
            Locked = bLock;

            if (m_TrayBandSite != NULL)
            {
                if (!SUCCEEDED(m_TrayBandSite->Lock(bLock)))
                {
                    /* Reset?? */
                    Locked = bPrevLock;
                    return bPrevLock;
                }
            }

            if (Locked && m_Theme)
            {
                SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, 0);
            }
            else
            {
                SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, WS_THICKFRAME | WS_BORDER);
            }
            SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

        }

        return bPrevLock;
    }


    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;
        int partId;

        GetClientRect(&rect);

        if (m_Theme)
        {
            GetClientRect(&rect);
            switch (m_Position)
            {
            case ABE_LEFT:
                partId = TBP_BACKGROUNDLEFT;
                break;
            case ABE_TOP:
                partId = TBP_BACKGROUNDTOP;
                break;
            case ABE_RIGHT:
                partId = TBP_BACKGROUNDRIGHT;
                break;
            case ABE_BOTTOM:
            default:
                partId = TBP_BACKGROUNDBOTTOM;
                break;
            }

            DrawThemeBackground(m_Theme, hdc, partId, 0, &rect, 0);
        }

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!m_Theme)
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    int DrawSizer(IN HRGN hRgn)
    {
        HDC hdc;
        RECT rect;
        int backoundPart;

        GetWindowRect(m_hWnd, &rect);
        OffsetRect(&rect, -rect.left, -rect.top);

        hdc = GetDCEx(m_hWnd, hRgn, DCX_WINDOW | DCX_INTERSECTRGN | DCX_PARENTCLIP);

        switch (m_Position)
        {
        case ABE_LEFT:
            backoundPart = TBP_SIZINGBARLEFT;
            rect.left = rect.right - GetSystemMetrics(SM_CXSIZEFRAME);
            break;
        case ABE_TOP:
            backoundPart = TBP_SIZINGBARTOP;
            rect.top = rect.bottom - GetSystemMetrics(SM_CYSIZEFRAME);
            break;
        case ABE_RIGHT:
            backoundPart = TBP_SIZINGBARRIGHT;
            rect.right = rect.left + GetSystemMetrics(SM_CXSIZEFRAME);
            break;
        case ABE_BOTTOM:
        default:
            backoundPart = TBP_SIZINGBARBOTTOM;
            rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZEFRAME);
            break;
        }

        DrawThemeBackground(m_Theme, hdc, backoundPart, 0, &rect, 0);

        ReleaseDC(m_hWnd, hdc);
        return 0;
    }

    DWORD WINAPI RunFileDlgThread()
    {
        HWND hwnd;
        RECT posRect;

        GetWindowRect(m_StartButton.m_hWnd, &posRect);

        hwnd = CreateWindowEx(0,
                              WC_STATIC,
                              NULL,
                              WS_OVERLAPPED | WS_DISABLED | WS_CLIPSIBLINGS | WS_BORDER | SS_LEFT,
                              posRect.left,
                              posRect.top,
                              posRect.right - posRect.left,
                              posRect.bottom - posRect.top,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

        m_RunFileDlgOwner = hwnd;

        RunFileDlg(hwnd, NULL, NULL, NULL, NULL, RFF_CALCDIRECTORY);

        m_RunFileDlgOwner = NULL;
        ::DestroyWindow(hwnd);

        return 0;
    }

    static DWORD WINAPI s_RunFileDlgThread(IN OUT PVOID pParam)
    {
        CTrayWindow * This = (CTrayWindow*) pParam;
        return This->RunFileDlgThread();
    }

    void DisplayRunFileDlg()
    {
        HWND hRunDlg;
        if (m_RunFileDlgOwner)
        {
            hRunDlg = GetLastActivePopup(m_RunFileDlgOwner);
            if (hRunDlg != NULL &&
                hRunDlg != m_RunFileDlgOwner)
            {
                SetForegroundWindow(hRunDlg);
                return;
            }
        }

        CloseHandle(CreateThread(NULL, 0, s_RunFileDlgThread, this, 0, NULL));
    }

    void PopupStartMenu()
    {
        if (m_StartMenuPopup != NULL)
        {
            POINTL pt;
            RECTL rcExclude;
            DWORD dwFlags = 0;

            if (GetWindowRect(m_StartButton.m_hWnd, (RECT*) &rcExclude))
            {
                switch (m_Position)
                {
                case ABE_BOTTOM:
                    pt.x = rcExclude.left;
                    pt.y = rcExclude.top;
                    dwFlags |= MPPF_BOTTOM;
                    break;
                case ABE_TOP:
                case ABE_LEFT:
                    pt.x = rcExclude.left;
                    pt.y = rcExclude.bottom;
                    dwFlags |= MPPF_TOP | MPPF_ALIGN_RIGHT;
                    break;
                case ABE_RIGHT:
                    pt.x = rcExclude.right;
                    pt.y = rcExclude.bottom;
                    dwFlags |= MPPF_TOP | MPPF_ALIGN_LEFT;
                    break;
                }

                m_StartMenuPopup->Popup(&pt, &rcExclude, dwFlags);

                m_StartButton.SendMessageW(BM_SETSTATE, TRUE, 0);
            }
        }
    }

    void ProcessMouseTracking()
    {
        RECT rcCurrent;
        POINT pt;
        BOOL over;
        UINT state = m_AutoHideState;

        GetCursorPos(&pt);
        GetWindowRect(m_hWnd, &rcCurrent);
        over = PtInRect(&rcCurrent, pt);

        if (m_StartButton.SendMessage( BM_GETSTATE, 0, 0) != BST_UNCHECKED)
        {
            over = TRUE;
        }

        if (over)
        {
            if (state == AUTOHIDE_HIDING)
            {
                TRACE("AutoHide cancelling hide.\n");
                m_AutoHideState = AUTOHIDE_SHOWING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
            }
            else if (state == AUTOHIDE_HIDDEN)
            {
                TRACE("AutoHide starting show.\n");
                m_AutoHideState = AUTOHIDE_SHOWING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_SHOW, NULL);
            }
        }
        else
        {
            if (state == AUTOHIDE_SHOWING)
            {
                TRACE("AutoHide cancelling show.\n");
                m_AutoHideState = AUTOHIDE_HIDING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
            }
            else if (state == AUTOHIDE_SHOWN)
            {
                TRACE("AutoHide starting hide.\n");
                m_AutoHideState = AUTOHIDE_HIDING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_HIDE, NULL);
            }

            KillTimer(TIMER_ID_MOUSETRACK);
        }
    }

    void ProcessAutoHide()
    {
        RECT rc = m_TrayRects[m_Position];
        INT w = m_TraySize.cx - GetSystemMetrics(SM_CXBORDER) * 2 - 1;
        INT h = m_TraySize.cy - GetSystemMetrics(SM_CYBORDER) * 2 - 1;

        TRACE("AutoHide Timer received for %u, rc=(%d, %d, %d, %d), w=%d, h=%d.\n", m_AutoHideState, rc.left, rc.top, rc.right, rc.bottom, w, h);

        switch (m_AutoHideState)
        {
        case AUTOHIDE_HIDING:
            switch (m_Position)
            {
            case ABE_LEFT:
                m_AutoHideOffset.cy = 0;
                m_AutoHideOffset.cx -= AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cx < -w)
                    m_AutoHideOffset.cx = -w;
                break;
            case ABE_TOP:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy -= AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cy < -h)
                    m_AutoHideOffset.cy = -h;
                break;
            case ABE_RIGHT:
                m_AutoHideOffset.cy = 0;
                m_AutoHideOffset.cx += AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cx > w)
                    m_AutoHideOffset.cx = w;
                break;
            case ABE_BOTTOM:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy += AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cy > h)
                    m_AutoHideOffset.cy = h;
                break;
            }

            if (m_AutoHideOffset.cx != w && m_AutoHideOffset.cy != h)
            {
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
                break;
            }

            /* fallthrough */
        case AUTOHIDE_HIDDEN:

            switch (m_Position)
            {
            case ABE_LEFT:
                m_AutoHideOffset.cx = -w;
                m_AutoHideOffset.cy = 0;
                break;
            case ABE_TOP:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy = -h;
                break;
            case ABE_RIGHT:
                m_AutoHideOffset.cx = w;
                m_AutoHideOffset.cy = 0;
                break;
            case ABE_BOTTOM:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy = h;
                break;
            }

            KillTimer(TIMER_ID_AUTOHIDE);
            m_AutoHideState = AUTOHIDE_HIDDEN;
            break;

        case AUTOHIDE_SHOWING:
            if (m_AutoHideOffset.cx >= AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cx -= AUTOHIDE_SPEED_SHOW;
            }
            else if (m_AutoHideOffset.cx <= -AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cx += AUTOHIDE_SPEED_SHOW;
            }
            else
            {
                m_AutoHideOffset.cx = 0;
            }

            if (m_AutoHideOffset.cy >= AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cy -= AUTOHIDE_SPEED_SHOW;
            }
            else if (m_AutoHideOffset.cy <= -AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cy += AUTOHIDE_SPEED_SHOW;
            }
            else
            {
                m_AutoHideOffset.cy = 0;
            }

            if (m_AutoHideOffset.cx != 0 || m_AutoHideOffset.cy != 0)
            {
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
                break;
            }

            /* fallthrough */
        case AUTOHIDE_SHOWN:

            KillTimer(TIMER_ID_AUTOHIDE);
            m_AutoHideState = AUTOHIDE_SHOWN;
            break;
        }

        rc.left += m_AutoHideOffset.cx;
        rc.right += m_AutoHideOffset.cx;
        rc.top += m_AutoHideOffset.cy;
        rc.bottom += m_AutoHideOffset.cy;

        TRACE("AutoHide Changing position to (%d, %d, %d, %d) and state=%u.\n", rc.left, rc.top, rc.right, rc.bottom, m_AutoHideState);
        SetWindowPos(NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    LRESULT OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Load the saved tray window settings */
        RegLoadSettings();

        /* Move the tray window to the right position and resize it if neccessary */
        CheckTrayWndPosition();

        /* Align all controls on the tray window */
        AlignControls(NULL);

        return TRUE;
    }

    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_TrayNotify)
        {
            TrayNotify_NotifyMsg(m_TrayNotifyInstance, wParam, lParam);
        }
        return TRUE;
    }

    LRESULT OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (!m_Theme)
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawSizer((HRGN) wParam);
    }

    LRESULT OnCtlColorBtn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetBkMode((HDC) wParam, TRANSPARENT);
        return (LRESULT) GetStockObject(HOLLOW_BRUSH);
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        POINT pt;

        if (Locked)
        {
            /* The user may not be able to resize the tray window.
            Pretend like the window is not sizeable when the user
            clicks on the border. */
            return HTBORDER;
        }

        SetLastError(ERROR_SUCCESS);
        if (GetClientRect(&rcClient) &&
            (MapWindowPoints(m_hWnd, NULL, (LPPOINT) &rcClient, 2) != 0 || GetLastError() == ERROR_SUCCESS))
        {
            pt.x = (SHORT) LOWORD(lParam);
            pt.y = (SHORT) HIWORD(lParam);

            if (PtInRect(&rcClient,
                pt))
            {
                /* The user is trying to drag the tray window */
                return HTCAPTION;
            }

            /* Depending on the position of the tray window, allow only
            changing the border next to the monitor working area */
            switch (m_Position)
            {
            case ABE_TOP:
                if (pt.y > rcClient.bottom)
                    return HTBOTTOM;
                break;
            case ABE_LEFT:
                if (pt.x > rcClient.right)
                    return HTRIGHT;
                break;
            case ABE_RIGHT:
                if (pt.x < rcClient.left)
                    return HTLEFT;
                break;
            case ABE_BOTTOM:
            default:
                if (pt.y < rcClient.top)
                    return HTTOP;
                break;
            }
        }
        return HTBORDER;
        return TRUE;
    }

    LRESULT OnMoving(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT ptCursor;
        PRECT pRect = (PRECT) lParam;

        /* We need to ensure that an application can not accidently
        move the tray window (using SetWindowPos). However, we still
        need to be able to move the window in case the user wants to
        drag the tray window to another position or in case the user
        wants to resize the tray window. */
        if (!Locked && GetCursorPos(&ptCursor))
        {
            IsDragging = TRUE;
            m_DraggingPosition = GetDraggingRectFromPt(ptCursor, pRect, &m_DraggingMonitor);
        }
        else
        {
            *pRect = m_TrayRects[m_Position];

            if (AutoHide)
            {
                pRect->left += m_AutoHideOffset.cx;
                pRect->right += m_AutoHideOffset.cx;
                pRect->top += m_AutoHideOffset.cy;
                pRect->bottom += m_AutoHideOffset.cy;
            }
        }
        return TRUE;
    }

    LRESULT OnSizing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PRECT pRect = (PRECT) lParam;

        if (!Locked)
        {
            CalculateValidSize(m_Position, pRect);
        }
        else
        {
            *pRect = m_TrayRects[m_Position];

            if (AutoHide)
            {
                pRect->left += m_AutoHideOffset.cx;
                pRect->right += m_AutoHideOffset.cx;
                pRect->top += m_AutoHideOffset.cy;
                pRect->bottom += m_AutoHideOffset.cy;
            }
        }
        return TRUE;
    }

    LRESULT OnWindowPosChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        ChangingWinPos((LPWINDOWPOS) lParam);
        return TRUE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        InvalidateRect(NULL, TRUE);
        if (wParam == SIZE_RESTORED && lParam == 0)
        {
            ResizeWorkArea();
            /* Clip the tray window on multi monitor systems so the edges can't
            overlap into another monitor */
            ApplyClipping(TRUE);

            if (!GetClientRect(&rcClient))
            {
                return FALSE;
            }
        }
        else
        {
            rcClient.left = rcClient.top = 0;
            rcClient.right = LOWORD(lParam);
            rcClient.bottom = HIWORD(lParam);
        }

        AlignControls(&rcClient);
        return TRUE;
    }

    LRESULT OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        InSizeMove = TRUE;
        IsDragging = FALSE;
        if (!Locked)
        {
            /* Remove the clipping on multi monitor systems while dragging around */
            ApplyClipping(FALSE);
        }
        return TRUE;
    }

    LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        InSizeMove = FALSE;
        if (!Locked)
        {
            /* Apply clipping */
            PostMessage(m_hWnd, WM_SIZE, SIZE_RESTORED, 0);
        }
        return TRUE;
    }

    LRESULT OnSysChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        switch (wParam)
        {
        case TEXT(' '):
        {
            /* The user pressed Alt+Space, this usually brings up the system menu of a window.
            The tray window needs to handle this specially, since it normally doesn't have
            a system menu. */

            static const UINT uidDisableItem [] = {
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
            SetWindowStyle(m_hWnd, WS_SYSMENU, WS_SYSMENU);

            hSysMenu = GetSystemMenu(m_hWnd, FALSE);
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
                uId = TrackMenu(
                    hSysMenu,
                    NULL,
                    m_StartButton.m_hWnd,
                    m_Position != ABE_TOP,
                    FALSE);
                if (uId != 0)
                {
                    SendMessage(m_hWnd, WM_SYSCOMMAND, (WPARAM) uId, 0);
                }
            }

            /* revert the system menu window style */
            SetWindowStyle(m_hWnd, WS_SYSMENU, 0);
            break;
        }

        default:
            bHandled = FALSE;
        }
        return TRUE;
    }

    LRESULT OnNcRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* We want the user to be able to get a context menu even on the nonclient
        area (including the sizing border)! */
        uMsg = WM_CONTEXTMENU;
        wParam = (WPARAM) m_hWnd;

        return OnContextMenu(uMsg, wParam, lParam, bHandled);
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;
        POINT pt, *ppt = NULL;
        HWND hWndExclude = NULL;

        /* Check if the administrator has forbidden access to context menus */
        if (SHRestricted(REST_NOTRAYCONTEXTMENU))
            return FALSE;

        pt.x = (SHORT) LOWORD(lParam);
        pt.y = (SHORT) HIWORD(lParam);

        if (pt.x != -1 || pt.y != -1)
            ppt = &pt;
        else
            hWndExclude = m_StartButton.m_hWnd;

        if ((HWND) wParam == m_StartButton.m_hWnd)
        {
            /* Make sure we can't track the context menu if the start
            menu is currently being shown */
            if (!(m_StartButton.SendMessage(BM_GETSTATE, 0, 0) & BST_PUSHED))
            {
                CComPtr<IContextMenu> ctxMenu;
                StartMenuBtnCtxMenuCreator(this, m_hWnd, &ctxMenu);
                TrackCtxMenu(ctxMenu, ppt, hWndExclude, m_Position == ABE_BOTTOM, this);
            }
        }
        else
        {
            /* See if the context menu should be handled by the task band site */
            if (ppt != NULL && m_TrayBandSite != NULL)
            {
                HWND hWndAtPt;
                POINT ptClient = *ppt;

                /* Convert the coordinates to client-coordinates */
                MapWindowPoints(NULL, m_hWnd, &ptClient, 1);

                hWndAtPt = ChildWindowFromPoint(m_hWnd, ptClient);
                if (hWndAtPt != NULL &&
                    (hWndAtPt == m_Rebar || IsChild(m_Rebar, hWndAtPt)))
                {
                    /* Check if the user clicked on the task switch window */
                    ptClient = *ppt;
                    MapWindowPoints(NULL, m_Rebar, &ptClient, 1);

                    hWndAtPt = ChildWindowFromPointEx(m_Rebar, ptClient, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
                    if (hWndAtPt == m_TaskSwitch)
                        goto HandleTrayContextMenu;

                    /* Forward the message to the task band site */
                    m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret);
                }
                else
                    goto HandleTrayContextMenu;
            }
            else
            {
HandleTrayContextMenu:
                /* Tray the default tray window context menu */
                CComPtr<IContextMenu> ctxMenu;
                TrayWindowCtxMenuCreator(this, m_hWnd, &ctxMenu);
                TrackCtxMenu(ctxMenu, ppt, NULL, FALSE, this);
            }
        }
        return Ret;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;
        /* FIXME: We can't check with IsChild whether the hwnd is somewhere inside
        the rebar control! But we shouldn't forward messages that the band
        site doesn't handle, such as other controls (start button, tray window */

        HRESULT hr = E_FAIL;

        if (m_TrayBandSite)
        {
            hr = m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret);
            if (SUCCEEDED(hr))
                return Ret;
        }

        if (m_TrayBandSite == NULL || FAILED(hr))
        {
            const NMHDR *nmh = (const NMHDR *) lParam;

            if (nmh->hwndFrom == m_TrayNotify)
            {
                switch (nmh->code)
                {
                case NTNWM_REALIGN:
                    /* Cause all controls to be aligned */
                    PostMessage(m_hWnd, WM_SIZE, SIZE_RESTORED, 0);
                    break;
                }
            }
        }
        return Ret;
    }

    LRESULT OnNcLButtonDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* We "handle" this message so users can't cause a weird maximize/restore
        window animation when double-clicking the tray window! */

        /* We should forward mouse messages to child windows here.
        Right now, this is only clock double-click */
        RECT rcClock;
        if (TrayNotify_GetClockRect(m_TrayNotifyInstance, &rcClock))
        {
            POINT ptClick;
            ptClick.x = MAKEPOINTS(lParam).x;
            ptClick.y = MAKEPOINTS(lParam).y;
            if (PtInRect(&rcClock, ptClick))
            {
                //FIXME: use SHRunControlPanel
                ShellExecuteW(m_hWnd, NULL, L"timedate.cpl", NULL, NULL, SW_NORMAL);
            }
        }
        return TRUE;
    }

    LRESULT OnAppTrayDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return TRUE;
    }

    LRESULT OnOpenStartMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HWND hwndStartMenu;
        HRESULT hr = IUnknown_GetWindow(m_StartMenuPopup, &hwndStartMenu);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        if (IsWindowVisible(hwndStartMenu))
        {
            m_StartMenuPopup->OnSelect(MPOS_CANCELLEVEL);
        }
        else
        {
            PopupStartMenu();
        }

        return TRUE;
    }

    LRESULT DoExitWindows()
    {
        ExitWindowsDialog(m_hWnd);
        return 0;
    }

    LRESULT OnDoExitWindows(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* 
         * TWM_DOEXITWINDOWS is send by the CDesktopBrowserr to us to 
         * show the shutdown dialog
         */
        return DoExitWindows();
    }

    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == SC_CLOSE)
        {
            return DoExitWindows();
        }

        bHandled = FALSE;
        return TRUE;
    }

    HRESULT ExecResourceCmd(int id)
    {
        WCHAR szCommand[256];
        WCHAR *pszParameters;

        if (!LoadString(hExplorerInstance,
                        id,
                        szCommand,
                        sizeof(szCommand) / sizeof(szCommand[0])))
        {
            return E_FAIL;
        }

        pszParameters = wcschr(szCommand, L'>');
        if (!pszParameters)
            return E_FAIL;

        *pszParameters = 0;
        pszParameters++;

        ShellExecuteW(m_hWnd, NULL, szCommand, pszParameters, NULL, 0);
        return S_OK;
    }

    LRESULT OnHotkey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        switch (wParam)
        {
        case IDHK_RUN:
            DisplayRunFileDlg();
            break;
        case IDHK_HELP:
            ExecResourceCmd(IDS_HELP_COMMAND);
            break;
        case IDHK_EXPLORE:
            ShellExecuteW(0, L"explore", NULL, NULL, NULL, 1); 
            break;
        case IDHK_FIND:
            SHFindFiles(NULL, NULL);
            break;
        case IDHK_FIND_COMPUTER:
            SHFindComputer(NULL, NULL);
            break;
        case IDHK_SYS_PROPERTIES:
            //FIXME: Use SHRunControlPanel
            ShellExecuteW(m_hWnd, NULL, L"sysdm.cpl", NULL, NULL, SW_NORMAL);
            break;
        case IDHK_NEXT_TASK:
            break;
        case IDHK_PREV_TASK:
            break;
        case IDHK_MINIMIZE_ALL:
            break;
        case IDHK_RESTORE_ALL:
            break;
        case IDHK_DESKTOP:
            break;
        case IDHK_PAGER:
            break;
        }

        return 0;
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;

        if ((HWND) lParam == m_StartButton.m_hWnd)
        {
            PopupStartMenu();
            return FALSE;
        }

        if (m_TrayBandSite == NULL || FAILED_UNEXPECTEDLY(m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret)))
        {
            switch (LOWORD(wParam))
            {
                /* FIXME: Handle these commands as well */
            case IDM_TASKBARANDSTARTMENU:
                DisplayProperties();
                break;

            case IDM_SEARCH:
                SHFindFiles(NULL, NULL);
                break;

            case IDM_HELPANDSUPPORT:
                ExecResourceCmd(IDS_HELP_COMMAND);
                break;

            case IDM_RUN:
                DisplayRunFileDlg();
                break;

                /* FIXME: Handle these commands as well */
            case IDM_SYNCHRONIZE:
            case IDM_LOGOFF:
            case IDM_DISCONNECT:
            case IDM_UNDOCKCOMPUTER:
                break;

            case IDM_SHUTDOWN:
                DoExitWindows();
                break;
            }
        }
        return Ret;
    }

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (AutoHide)
        {
            SetTimer(TIMER_ID_MOUSETRACK, MOUSETRACK_INTERVAL, NULL);
        }

        return TRUE;
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == TIMER_ID_MOUSETRACK)
        {
            ProcessMouseTracking();
        }
        else if (wParam == TIMER_ID_AUTOHIDE)
        {
            ProcessAutoHide();
        }

        bHandled = FALSE;
        return TRUE;
    }

    LRESULT OnRebarAutoSize(INT code, LPNMHDR nmhdr, BOOL& bHandled)
    {
#if 0
        LPNMRBAUTOSIZE as = (LPNMRBAUTOSIZE) nmhdr;

        if (!as->fChanged)
            return 0;

        RECT rc;
        ::GetWindowRect(m_hWnd, &rc);

        SIZE szWindow = { 
            rc.right - rc.left, 
            rc.bottom - rc.top };
        SIZE szTarget = { 
            as->rcTarget.right - as->rcTarget.left, 
            as->rcTarget.bottom - as->rcTarget.top };
        SIZE szActual = { 
            as->rcActual.right - as->rcActual.left, 
            as->rcActual.bottom - as->rcActual.top };

        SIZE borders = {
            szWindow.cx - szTarget.cx,
            szWindow.cy - szTarget.cx,
        };

        switch (m_Position)
        {
        case ABE_LEFT:
            szWindow.cx = szActual.cx + borders.cx;
            break;
        case ABE_TOP:
            szWindow.cy = szActual.cy + borders.cy;
            break;
        case ABE_RIGHT:
            szWindow.cx = szActual.cx + borders.cx;
            rc.left = rc.right - szWindow.cy;
            break;
        case ABE_BOTTOM:
            szWindow.cy = szActual.cy + borders.cy;
            rc.top = rc.bottom - szWindow.cy;
            break;
        }

        SetWindowPos(NULL, rc.left, rc.top, szWindow.cx, szWindow.cy, SWP_NOACTIVATE | SWP_NOZORDER);
#else
        bHandled = FALSE;
#endif
        return 0;
    }

    DECLARE_WND_CLASS_EX(szTrayWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayWindow)
        if (m_StartMenuBand != NULL)
        {
            MSG Msg;
            LRESULT lRet;

            Msg.hwnd = m_hWnd;
            Msg.message = uMsg;
            Msg.wParam = wParam;
            Msg.lParam = lParam;

            if (m_StartMenuBand->TranslateMenuMessage(&Msg, &lRet) == S_OK)
            {
                return lRet;
            }

            wParam = Msg.wParam;
            lParam = Msg.lParam;
        }
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        NOTIFY_CODE_HANDLER(RBN_AUTOSIZE, OnRebarAutoSize) // Doesn't quite work ;P
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        /*MESSAGE_HANDLER(WM_DESTROY, OnDestroy)*/
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
        MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)
        MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorBtn)
        MESSAGE_HANDLER(WM_MOVING, OnMoving)
        MESSAGE_HANDLER(WM_SIZING, OnSizing)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChange)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_SYSCHAR, OnSysChar)
        MESSAGE_HANDLER(WM_NCRBUTTONUP, OnNcRButtonUp)
        MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnNcLButtonDblClick)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_APP_TRAYDESTROY, OnAppTrayDestroy)
        MESSAGE_HANDLER(TWM_OPENSTARTMENU, OnOpenStartMenu)
        MESSAGE_HANDLER(TWM_DOEXITWINDOWS, OnDoExitWindows)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotkey)
    ALT_MSG_MAP(1)
    END_MSG_MAP()

    /*****************************************************************************/

    VOID TrayProcessMessages()
    {
        MSG Msg;

        /* FIXME: We should keep a reference here... */

        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (Msg.message == WM_QUIT)
                break;

            if (m_StartMenuBand == NULL ||
                m_StartMenuBand->IsMenuMessage(&Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    VOID TrayMessageLoop()
    {
        MSG Msg;
        BOOL Ret;

        /* FIXME: We should keep a reference here... */

        while (true)
        {
            Ret = GetMessage(&Msg, NULL, 0, 0);

            if (!Ret || Ret == -1)
                break;

            if (m_StartMenuBand == NULL ||
                m_StartMenuBand->IsMenuMessage(&Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    /*
     * IShellDesktopTray
     *
     * NOTE: this is a very windows-specific COM interface used by SHCreateDesktop()!
     *       These are the calls I observed, it may be wrong/incomplete/buggy!!!
     *       The reason we implement it is because we have to use SHCreateDesktop() so
     *       that the shell provides the desktop window and all the features that come
     *       with it (especially positioning of desktop icons)
     */

    virtual ULONG STDMETHODCALLTYPE GetState()
    {
        /* FIXME: Return ABS_ flags? */
        TRACE("IShellDesktopTray::GetState() unimplemented!\n");
        return 0;
    }

    virtual HRESULT STDMETHODCALLTYPE GetTrayWindow(OUT HWND *phWndTray)
    {
        TRACE("IShellDesktopTray::GetTrayWindow(0x%p)\n", phWndTray);
        *phWndTray = m_hWnd;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE RegisterDesktopWindow(IN HWND hWndDesktop)
    {
        TRACE("IShellDesktopTray::RegisterDesktopWindow(0x%p)\n", hWndDesktop);

        m_DesktopWnd = hWndDesktop;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Unknown(IN DWORD dwUnknown1, IN DWORD dwUnknown2)
    {
        TRACE("IShellDesktopTray::Unknown(%u,%u) unimplemented!\n", dwUnknown1, dwUnknown2);
        return S_OK;
    }

    virtual HRESULT RaiseStartButton()
    {
        m_StartButton.SendMessageW(BM_SETSTATE, FALSE, 0);
        return S_OK;
    }

    void _Init()
    {
        m_Position = (DWORD) -1;
    }

    DECLARE_NOT_AGGREGATABLE(CTrayWindow)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTrayWindow)
        /*COM_INTERFACE_ENTRY_IID(IID_ITrayWindow, ITrayWindow)*/
        COM_INTERFACE_ENTRY_IID(IID_IShellDesktopTray, IShellDesktopTray)
    END_COM_MAP()
};

class CTrayWindowCtxMenu :
    public CComCoClass<CTrayWindowCtxMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu
{
    HWND hWndOwner;
    CComPtr<CTrayWindow> TrayWnd;
    CComPtr<IContextMenu> pcm;

public:
    HRESULT Initialize(ITrayWindow * pTrayWnd, IN HWND hWndOwner)
    {
        this->TrayWnd = (CTrayWindow *) pTrayWnd;
        this->hWndOwner = hWndOwner;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE 
        QueryContextMenu(HMENU hPopup,
                         UINT indexMenu,
                         UINT idCmdFirst,
                         UINT idCmdLast,
                         UINT uFlags)
    {
        HMENU menubase = LoadPopupMenu(hExplorerInstance, MAKEINTRESOURCE(IDM_TRAYWND));

        if (!menubase)
            return HRESULT_FROM_WIN32(GetLastError());

        int count = ::GetMenuItemCount(menubase);

        for (int i = 0; i < count; i++)
        {
            WCHAR label[128];

            MENUITEMINFOW mii = { 0 };
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS 
                | MIIM_DATA | MIIM_STRING | MIIM_BITMAP | MIIM_FTYPE;
            mii.dwTypeData = label;
            mii.cch = _countof(label);
            ::GetMenuItemInfoW(menubase, i, TRUE, &mii);

            TRACE("Adding item %d label %S type %d\n", mii.wID, mii.dwTypeData, mii.fType);

            mii.fType |= MFT_RADIOCHECK;

            ::InsertMenuItemW(hPopup, i + 1, TRUE, &mii);
        }

        ::DestroyMenu(menubase);

        if (SHRestricted(REST_CLASSICSHELL) != 0)
        {
            DeleteMenu(hPopup,
                       ID_LOCKTASKBAR,
                       MF_BYCOMMAND);
        }

        CheckMenuItem(hPopup,
                      ID_LOCKTASKBAR,
                      MF_BYCOMMAND | (TrayWnd->Locked ? MF_CHECKED : MF_UNCHECKED));

        if (TrayWnd->m_TrayBandSite != NULL)
        {
            if (FAILED(TrayWnd->m_TrayBandSite->AddContextMenus(
                hPopup,
                0,
                ID_SHELL_CMD_FIRST,
                ID_SHELL_CMD_LAST,
                CMF_NORMAL,
                &pcm)))
            {
                WARN("AddContextMenus failed.\n");
                pcm = NULL;
            }
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE
        InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
    {
        UINT uiCmdId = (UINT) lpici->lpVerb;
        if (uiCmdId != 0)
        {
            if (uiCmdId >= ID_SHELL_CMD_FIRST && uiCmdId <= ID_SHELL_CMD_LAST)
            {
                CMINVOKECOMMANDINFO cmici = { 0 };

                if (pcm != NULL)
                {
                    /* Setup and invoke the shell command */
                    cmici.cbSize = sizeof(cmici);
                    cmici.hwnd = hWndOwner;
                    cmici.lpVerb = (LPCSTR) MAKEINTRESOURCE(uiCmdId - ID_SHELL_CMD_FIRST);
                    cmici.nShow = SW_NORMAL;

                    pcm->InvokeCommand(&cmici);
                }
            }
            else
            {
                TrayWnd->ExecContextMenuCmd(uiCmdId);
            }
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE
        GetCommandString(UINT_PTR idCmd,
        UINT uType,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax)
    {
        return E_NOTIMPL;
    }

    CTrayWindowCtxMenu()
    {
    }

    virtual ~CTrayWindowCtxMenu()
    {
    }

    BEGIN_COM_MAP(CTrayWindowCtxMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

HRESULT TrayWindowCtxMenuCreator(ITrayWindow * TrayWnd, IN HWND hWndOwner, IContextMenu ** ppCtxMenu)
{
    CTrayWindowCtxMenu * mnu = new CComObject<CTrayWindowCtxMenu>();
    mnu->Initialize(TrayWnd, hWndOwner);
    *ppCtxMenu = mnu;
    return S_OK;
}

HRESULT CreateTrayWindow(ITrayWindow ** ppTray)
{
    CComPtr<CTrayWindow> Tray = new CComObject<CTrayWindow>();
    if (Tray == NULL)
        return E_OUTOFMEMORY;

    Tray->_Init();
    Tray->Open();

    *ppTray = (ITrayWindow *) Tray;

    return S_OK;
}

HRESULT
Tray_OnStartMenuDismissed(ITrayWindow* Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    return TrayWindow->RaiseStartButton();
}

VOID TrayProcessMessages(ITrayWindow *Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    TrayWindow->TrayProcessMessages();
}

VOID TrayMessageLoop(ITrayWindow *Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    TrayWindow->TrayMessageLoop();
}
