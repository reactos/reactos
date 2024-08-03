/*
 * Tool tip control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2004 Robert Shearman
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 08, 2004, by Robert Shearman.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 * 
 * TODO:
 *   - Custom draw support.
 *   - Animation.
 *   - Links.
 *   - Messages:
 *     o TTM_ADJUSTRECT
 *     o TTM_GETTITLEA
 *     o TTM_GETTTILEW
 *     o TTM_POPUP
 *   - Styles:
 *     o TTS_NOANIMATE
 *     o TTS_NOFADE
 *     o TTS_CLOSE
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom (chapter 3) contains executables activate.exe,
 *     curtool.exe, deltool.exe, enumtools.exe, getinfo.exe, getiptxt.exe,
 *     hittest.exe, needtext.exe, newrect.exe, updtext.exe and winfrpt.exe.
 *
 *   Timer logic.
 *
 * One important point to remember is that tools don't necessarily get
 * a WM_MOUSEMOVE once the cursor leaves the tool, an example is when
 * a tool sets TTF_IDISHWND (i.e. an entire window is a tool) because
 * here WM_MOUSEMOVEs only get sent when the cursor is inside the
 * client area.  Therefore the only reliable way to know that the
 * cursor has left a tool is to keep a timer running and check the
 * position every time it expires.  This is the role of timer
 * ID_TIMERLEAVE.
 *
 *
 * On entering a tool (detected in a relayed WM_MOUSEMOVE) we start
 * ID_TIMERSHOW, if this times out and we're still in the tool we show
 * the tip.  On showing a tip we start both ID_TIMERPOP and
 * ID_TIMERLEAVE.  On hiding a tooltip we kill ID_TIMERPOP.
 * ID_TIMERPOP is restarted on every relayed WM_MOUSEMOVE.  If
 * ID_TIMERPOP expires the tool is hidden and ID_TIMERPOP is killed.
 * ID_TIMERLEAVE remains running - this is important as we need to
 * determine when the cursor leaves the tool.
 *
 * When ID_TIMERLEAVE expires or on a relayed WM_MOUSEMOVE if we're
 * still in the tool do nothing (apart from restart ID_TIMERPOP if
 * this is a WM_MOUSEMOVE) (ID_TIMERLEAVE remains running).  If we've
 * left the tool and entered another one then hide the tip and start
 * ID_TIMERSHOW with time ReshowTime and kill ID_TIMERLEAVE.  If we're
 * outside all tools hide the tip and kill ID_TIMERLEAVE.  On Relayed
 * mouse button messages hide the tip but leave ID_TIMERLEAVE running,
 * this again will let us keep track of when the cursor leaves the
 * tool.
 *
 *
 * infoPtr->nTool is the tool the mouse was on on the last relayed MM
 * or timer expiry or -1 if the mouse was not on a tool.
 *
 * infoPtr->nCurrentTool is the tool for which the tip is currently
 * displaying text for or -1 if the tip is not shown.  Actually this
 * will only ever be infoPtr-nTool or -1, so it could be changed to a
 * BOOL.
 *
 */



#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tooltips);

static HICON hTooltipIcons[TTI_ERROR+1];

typedef struct
{
    UINT      uFlags;
    UINT      uInternalFlags;
    HWND      hwnd;
    BOOL      bNotifyUnicode;
    UINT_PTR  uId;
    RECT      rect;
    HINSTANCE hinst;
    LPWSTR      lpszText;
    LPARAM      lParam;
} TTTOOL_INFO;


typedef struct
{
    HWND     hwndSelf;
    WCHAR      szTipText[INFOTIPSIZE];
    BOOL     bActive;
    BOOL     bTrackActive;
    UINT     uNumTools;
    COLORREF   clrBk;
    COLORREF   clrText;
    HFONT    hFont;
    HFONT    hTitleFont;
    INT      xTrackPos;
    INT      yTrackPos;
    INT      nMaxTipWidth;
    INT      nTool; /* tool that mouse was on on last relayed mouse move */
    INT      nCurrentTool;
    INT      nTrackTool;
    INT      nReshowTime;
    INT      nAutoPopTime;
    INT      nInitialTime;
    RECT     rcMargin;
    BOOL     bToolBelow;
    LPWSTR   pszTitle;
    HICON    hTitleIcon;

    TTTOOL_INFO *tools;
} TOOLTIPS_INFO;

#define ID_TIMERSHOW   1    /* show delay timer */
#define ID_TIMERPOP    2    /* auto pop timer */
#define ID_TIMERLEAVE  3    /* tool leave timer */


#define TOOLTIPS_GetInfoPtr(hWindow) ((TOOLTIPS_INFO *)GetWindowLongPtrW (hWindow, 0))

/* offsets from window edge to start of text */
#define NORMAL_TEXT_MARGIN 2
#define BALLOON_TEXT_MARGIN (NORMAL_TEXT_MARGIN+8)
/* value used for CreateRoundRectRgn that specifies how much
 * each corner is curved */
#ifdef __REACTOS__
#define BALLOON_ROUNDEDNESS 16
#define BALLOON_STEMHEIGHT 18
#define BALLOON_STEMWIDTH 18
#define BALLOON_STEMINDENT 16
#else
#define BALLOON_ROUNDEDNESS 20
#define BALLOON_STEMHEIGHT 13
#define BALLOON_STEMWIDTH 10
#define BALLOON_STEMINDENT 20
#endif // __REACTOS__

#define BALLOON_ICON_TITLE_SPACING 8 /* horizontal spacing between icon and title */
#define BALLOON_TITLE_TEXT_SPACING 8 /* vertical spacing between icon/title and main text */
#define ICON_HEIGHT 16
#define ICON_WIDTH  16

#define MAX_TEXT_SIZE_A 80 /* maximum retrieving text size by ANSI message */

static LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uId, DWORD_PTR dwRef);

static inline UINT_PTR
TOOLTIPS_GetTitleIconIndex(HICON hIcon)
{
    UINT i;
    for (i = 0; i <= TTI_ERROR; i++)
        if (hTooltipIcons[i] == hIcon)
            return i;
    return (UINT_PTR)hIcon;
}

static void
TOOLTIPS_InitSystemSettings (TOOLTIPS_INFO *infoPtr)
{
    NONCLIENTMETRICSW nclm;

    infoPtr->clrBk   = comctl32_color.clrInfoBk;
    infoPtr->clrText = comctl32_color.clrInfoText;

    DeleteObject (infoPtr->hFont);
    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof(nclm), &nclm, 0);
    infoPtr->hFont = CreateFontIndirectW (&nclm.lfStatusFont);

    DeleteObject (infoPtr->hTitleFont);
    nclm.lfStatusFont.lfWeight = FW_BOLD;
    infoPtr->hTitleFont = CreateFontIndirectW (&nclm.lfStatusFont);
}

/* Custom draw routines */
static void
TOOLTIPS_customdraw_fill(const TOOLTIPS_INFO *infoPtr, NMTTCUSTOMDRAW *lpnmttcd,
                         HDC hdc, const RECT *rcBounds, UINT uFlags)
{
    ZeroMemory(lpnmttcd, sizeof(NMTTCUSTOMDRAW));
    lpnmttcd->uDrawFlags = uFlags;
    lpnmttcd->nmcd.hdr.hwndFrom = infoPtr->hwndSelf;
    lpnmttcd->nmcd.hdr.code     = NM_CUSTOMDRAW;
    if (infoPtr->nCurrentTool != -1) {
        TTTOOL_INFO *toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];
        lpnmttcd->nmcd.hdr.idFrom = toolPtr->uId;
    }
    lpnmttcd->nmcd.hdc = hdc;
    lpnmttcd->nmcd.rc = *rcBounds;
    /* FIXME - dwItemSpec, uItemState, lItemlParam */
}

static inline DWORD
TOOLTIPS_notify_customdraw (DWORD dwDrawStage, NMTTCUSTOMDRAW *lpnmttcd)
{
    LRESULT result;
    lpnmttcd->nmcd.dwDrawStage = dwDrawStage;

    TRACE("Notifying stage %d, flags %x, id %x\n", lpnmttcd->nmcd.dwDrawStage,
          lpnmttcd->uDrawFlags, lpnmttcd->nmcd.hdr.code);

    result = SendMessageW(GetParent(lpnmttcd->nmcd.hdr.hwndFrom), WM_NOTIFY,
                          0, (LPARAM)lpnmttcd);

    TRACE("Notify result %x\n", (unsigned int)result);

    return result;
}

static void
TOOLTIPS_Refresh (const TOOLTIPS_INFO *infoPtr, HDC hdc)
{
    RECT rc;
    INT oldBkMode;
    HFONT hOldFont;
    HBRUSH hBrush;
    UINT uFlags = DT_EXTERNALLEADING;
    HRGN hRgn = NULL;
    DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
    NMTTCUSTOMDRAW nmttcd;
    DWORD cdmode;

    if (infoPtr->nMaxTipWidth > -1)
	uFlags |= DT_WORDBREAK;
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    GetClientRect (infoPtr->hwndSelf, &rc);

    hBrush = CreateSolidBrush(infoPtr->clrBk);

    oldBkMode = SetBkMode (hdc, TRANSPARENT);
    SetTextColor (hdc, infoPtr->clrText);
    hOldFont = SelectObject (hdc, infoPtr->hFont);

    /* Custom draw - Call PrePaint once initial properties set up     */
    /* Note: Contrary to MSDN, CDRF_SKIPDEFAULT still draws a tooltip */
    TOOLTIPS_customdraw_fill(infoPtr, &nmttcd, hdc, &rc, uFlags);
    cdmode = TOOLTIPS_notify_customdraw(CDDS_PREPAINT, &nmttcd);
    uFlags = nmttcd.uDrawFlags;

    if (dwStyle & TTS_BALLOON)
    {
        /* create a region to store result into */
        hRgn = CreateRectRgn(0, 0, 0, 0);

        GetWindowRgn(infoPtr->hwndSelf, hRgn);

        /* fill the background */
        FillRgn(hdc, hRgn, hBrush);
        DeleteObject(hBrush);
        hBrush = NULL;
    }
    else
    {
        /* fill the background */
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        hBrush = NULL;
    }

    if ((dwStyle & TTS_BALLOON) || infoPtr->pszTitle)
    {
        /* calculate text rectangle */
        rc.left   += (BALLOON_TEXT_MARGIN + infoPtr->rcMargin.left);
        rc.top    += (BALLOON_TEXT_MARGIN + infoPtr->rcMargin.top);
        rc.right  -= (BALLOON_TEXT_MARGIN + infoPtr->rcMargin.right);
        rc.bottom -= (BALLOON_TEXT_MARGIN + infoPtr->rcMargin.bottom);
        if(infoPtr->bToolBelow) rc.top += BALLOON_STEMHEIGHT;

        if (infoPtr->pszTitle)
        {
            RECT rcTitle = {rc.left, rc.top, rc.right, rc.bottom};
            int height;
            BOOL icon_present;
            HFONT prevFont;

            /* draw icon */
            icon_present = infoPtr->hTitleIcon && 
                DrawIconEx(hdc, rc.left, rc.top, infoPtr->hTitleIcon,
                           ICON_WIDTH, ICON_HEIGHT, 0, NULL, DI_NORMAL);
            if (icon_present)
                rcTitle.left += ICON_WIDTH + BALLOON_ICON_TITLE_SPACING;

            rcTitle.bottom = rc.top + ICON_HEIGHT;

            /* draw title text */
            prevFont = SelectObject (hdc, infoPtr->hTitleFont);
            height = DrawTextW(hdc, infoPtr->pszTitle, -1, &rcTitle, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject (hdc, prevFont);
            rc.top += height + BALLOON_TITLE_TEXT_SPACING;
        }
    }
    else
    {
        /* calculate text rectangle */
        rc.left   += (NORMAL_TEXT_MARGIN + infoPtr->rcMargin.left);
        rc.top    += (NORMAL_TEXT_MARGIN + infoPtr->rcMargin.top);
        rc.right  -= (NORMAL_TEXT_MARGIN + infoPtr->rcMargin.right);
        rc.bottom -= (NORMAL_TEXT_MARGIN + infoPtr->rcMargin.bottom);
    }

    /* draw text */
#ifdef __REACTOS__
    uFlags |= DT_EXPANDTABS;
#endif
    DrawTextW (hdc, infoPtr->szTipText, -1, &rc, uFlags);

    /* Custom draw - Call PostPaint after drawing */
    if (cdmode & CDRF_NOTIFYPOSTPAINT) {
        TOOLTIPS_notify_customdraw(CDDS_POSTPAINT, &nmttcd);
    }

    /* be polite and reset the things we changed in the dc */
    SelectObject (hdc, hOldFont);
    SetBkMode (hdc, oldBkMode);

    if (dwStyle & TTS_BALLOON)
    {
        /* frame region because default window proc doesn't do it */
        INT width = GetSystemMetrics(SM_CXDLGFRAME) - GetSystemMetrics(SM_CXEDGE);
        INT height = GetSystemMetrics(SM_CYDLGFRAME) - GetSystemMetrics(SM_CYEDGE);

        hBrush = GetSysColorBrush(COLOR_WINDOWFRAME);
        FrameRgn(hdc, hRgn, hBrush, width, height);
    }

    if (hRgn)
        DeleteObject(hRgn);
}

static void TOOLTIPS_GetDispInfoA(const TOOLTIPS_INFO *infoPtr, TTTOOL_INFO *toolPtr, WCHAR *buffer)
{
    NMTTDISPINFOA ttnmdi;

    /* fill NMHDR struct */
    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFOA));
    ttnmdi.hdr.hwndFrom = infoPtr->hwndSelf;
    ttnmdi.hdr.idFrom = toolPtr->uId;
    ttnmdi.hdr.code = TTN_GETDISPINFOA; /* == TTN_NEEDTEXTA */
    ttnmdi.lpszText = ttnmdi.szText;
    ttnmdi.uFlags = toolPtr->uFlags;
    ttnmdi.lParam = toolPtr->lParam;

    TRACE("hdr.idFrom = %lx\n", ttnmdi.hdr.idFrom);
    SendMessageW(toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

    if (IS_INTRESOURCE(ttnmdi.lpszText)) {
        LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
               buffer, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = ttnmdi.hinst;
            toolPtr->lpszText = (LPWSTR)ttnmdi.lpszText;
        }
    }
    else if (ttnmdi.lpszText == 0) {
        buffer[0] = '\0';
    }
    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACKA) {
        Str_GetPtrAtoW(ttnmdi.lpszText, buffer, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = 0;
            toolPtr->lpszText = NULL;
            Str_SetPtrW(&toolPtr->lpszText, buffer);
        }
    }
    else {
        ERR("recursive text callback\n");
        buffer[0] = '\0';
    }

#ifndef __REACTOS_
    /* no text available - try calling parent instead as per native */
    /* FIXME: Unsure if SETITEM should save the value or not        */
    if (buffer[0] == 0x00) {

        SendMessageW(GetParent(toolPtr->hwnd), WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

        if (IS_INTRESOURCE(ttnmdi.lpszText)) {
            LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
                   buffer, INFOTIPSIZE);
        } else if (ttnmdi.lpszText &&
                   ttnmdi.lpszText != LPSTR_TEXTCALLBACKA) {
            Str_GetPtrAtoW(ttnmdi.lpszText, buffer, INFOTIPSIZE);
        }
    }
#endif
}

static void TOOLTIPS_GetDispInfoW(const TOOLTIPS_INFO *infoPtr, TTTOOL_INFO *toolPtr, WCHAR *buffer)
{
    NMTTDISPINFOW ttnmdi;

    /* fill NMHDR struct */
    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFOW));
    ttnmdi.hdr.hwndFrom = infoPtr->hwndSelf;
    ttnmdi.hdr.idFrom = toolPtr->uId;
    ttnmdi.hdr.code = TTN_GETDISPINFOW; /* == TTN_NEEDTEXTW */
    ttnmdi.lpszText = ttnmdi.szText;
    ttnmdi.uFlags = toolPtr->uFlags;
    ttnmdi.lParam = toolPtr->lParam;

    TRACE("hdr.idFrom = %lx\n", ttnmdi.hdr.idFrom);
    SendMessageW(toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

    if (IS_INTRESOURCE(ttnmdi.lpszText)) {
        LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
               buffer, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = ttnmdi.hinst;
            toolPtr->lpszText = ttnmdi.lpszText;
        }
    }
    else if (ttnmdi.lpszText == 0) {
        buffer[0] = '\0';
    }
    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACKW) {
        Str_GetPtrW(ttnmdi.lpszText, buffer, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = 0;
            toolPtr->lpszText = NULL;
            Str_SetPtrW(&toolPtr->lpszText, buffer);
        }
    }
    else {
        ERR("recursive text callback\n");
        buffer[0] = '\0';
    }

#ifndef __REACTOS__
    /* no text available - try calling parent instead as per native */
    /* FIXME: Unsure if SETITEM should save the value or not        */
    if (buffer[0] == 0x00) {

        SendMessageW(GetParent(toolPtr->hwnd), WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

        if (IS_INTRESOURCE(ttnmdi.lpszText)) {
            LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
                   buffer, INFOTIPSIZE);
        } else if (ttnmdi.lpszText &&
                   ttnmdi.lpszText != LPSTR_TEXTCALLBACKW) {
            Str_GetPtrW(ttnmdi.lpszText, buffer, INFOTIPSIZE);
        }
    }
#endif

}

static void
TOOLTIPS_GetTipText (const TOOLTIPS_INFO *infoPtr, INT nTool, WCHAR *buffer)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[nTool];

    if (IS_INTRESOURCE(toolPtr->lpszText)) {
	/* load a resource */
	TRACE("load res string %p %x\n",
	       toolPtr->hinst, LOWORD(toolPtr->lpszText));
	if (!LoadStringW (toolPtr->hinst, LOWORD(toolPtr->lpszText), buffer, INFOTIPSIZE))
	    buffer[0] = '\0';
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACKW) {
	    if (toolPtr->bNotifyUnicode)
		TOOLTIPS_GetDispInfoW(infoPtr, toolPtr, buffer);
	    else
		TOOLTIPS_GetDispInfoA(infoPtr, toolPtr, buffer);
	}
	else {
	    /* the item is a usual (unicode) text */
	    lstrcpynW (buffer, toolPtr->lpszText, INFOTIPSIZE);
	}
    }
    else {
	/* no text available */
        buffer[0] = '\0';
    }

    if (!(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & TTS_NOPREFIX)) {
        WCHAR *ptrW;
        if ((ptrW = wcschr(buffer, '\t')))
            *ptrW = 0;
    }

    TRACE("%s\n", debugstr_w(buffer));
}


static void
TOOLTIPS_CalcTipSize (const TOOLTIPS_INFO *infoPtr, LPSIZE lpSize)
{
    HDC hdc;
    HFONT hOldFont;
    DWORD style = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
    UINT uFlags = DT_EXTERNALLEADING | DT_CALCRECT;
    RECT rc = {0, 0, 0, 0};
    SIZE title = {0, 0};

    if (infoPtr->nMaxTipWidth > -1) {
	rc.right = infoPtr->nMaxTipWidth;
	uFlags |= DT_WORDBREAK;
    }
    if (style & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    TRACE("%s\n", debugstr_w(infoPtr->szTipText));

    hdc = GetDC (infoPtr->hwndSelf);
    if (infoPtr->pszTitle)
    {
        RECT rcTitle = {0, 0, 0, 0};
        TRACE("title %s\n", debugstr_w(infoPtr->pszTitle));
        if (infoPtr->hTitleIcon)
        {
            title.cx = ICON_WIDTH;
            title.cy = ICON_HEIGHT;
        }
        if (title.cx != 0) title.cx += BALLOON_ICON_TITLE_SPACING;
        hOldFont = SelectObject (hdc, infoPtr->hTitleFont);
        DrawTextW(hdc, infoPtr->pszTitle, -1, &rcTitle, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        SelectObject (hdc, hOldFont);
        title.cy = max(title.cy, rcTitle.bottom - rcTitle.top) + BALLOON_TITLE_TEXT_SPACING;
        title.cx += (rcTitle.right - rcTitle.left);
    }
    hOldFont = SelectObject (hdc, infoPtr->hFont);
#ifdef __REACTOS__
    uFlags |= DT_EXPANDTABS;
#endif
    DrawTextW (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject (hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    if ((style & TTS_BALLOON) || infoPtr->pszTitle)
    {
        lpSize->cx = max(rc.right - rc.left, title.cx) + 2*BALLOON_TEXT_MARGIN +
                       infoPtr->rcMargin.left + infoPtr->rcMargin.right;
        lpSize->cy = title.cy + rc.bottom - rc.top + 2*BALLOON_TEXT_MARGIN +
                       infoPtr->rcMargin.bottom + infoPtr->rcMargin.top +
                       BALLOON_STEMHEIGHT;
    }
    else
    {
        lpSize->cx = rc.right - rc.left + 2*NORMAL_TEXT_MARGIN +
                       infoPtr->rcMargin.left + infoPtr->rcMargin.right;
        lpSize->cy = rc.bottom - rc.top + 2*NORMAL_TEXT_MARGIN +
                       infoPtr->rcMargin.bottom + infoPtr->rcMargin.top;
    }
}


static void
TOOLTIPS_Show (TOOLTIPS_INFO *infoPtr, BOOL track_activate)
{
    TTTOOL_INFO *toolPtr;
    HMONITOR monitor;
    MONITORINFO mon_info;
    RECT rect;
    SIZE size;
    NMHDR  hdr;
    int ptfx = 0;
    DWORD style = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
    INT nTool, current;

    if (track_activate)
    {
        if (infoPtr->nTrackTool == -1)
        {
            TRACE("invalid tracking tool %d\n", infoPtr->nTrackTool);
            return;
        }
        nTool = infoPtr->nTrackTool;
    }
    else
    {
        if (infoPtr->nTool == -1)
        {
            TRACE("invalid tool %d\n", infoPtr->nTool);
            return;
        }
        nTool = infoPtr->nTool;
    }

    TRACE("Show tooltip pre %d, %p\n", nTool, infoPtr->hwndSelf);

    current = infoPtr->nCurrentTool;
    if (!track_activate)
        infoPtr->nCurrentTool = infoPtr->nTool;

    TOOLTIPS_GetTipText (infoPtr, nTool, infoPtr->szTipText);

    if (infoPtr->szTipText[0] == '\0')
    {
        infoPtr->nCurrentTool = current;
        return;
    }

    toolPtr = &infoPtr->tools[nTool];
    TOOLTIPS_CalcTipSize (infoPtr, &size);

    TRACE("Show tooltip %d, %s, size %d x %d\n", nTool, debugstr_w(infoPtr->szTipText),
        size.cx, size.cy);

    if (track_activate && (toolPtr->uFlags & TTF_TRACK))
    {
        rect.left = infoPtr->xTrackPos;
        rect.top  = infoPtr->yTrackPos;
        ptfx = rect.left;

        if (toolPtr->uFlags & TTF_CENTERTIP)
        {
            rect.left -= (size.cx / 2);
            if (!(style & TTS_BALLOON))
                rect.top  -= (size.cy / 2);
        }
        if (!(infoPtr->bToolBelow = (infoPtr->yTrackPos + size.cy <= GetSystemMetrics(SM_CYSCREEN))))
            rect.top -= size.cy;

        if (!(toolPtr->uFlags & TTF_ABSOLUTE))
        {
            if (style & TTS_BALLOON)
                rect.left -= BALLOON_STEMINDENT;
            else
            {
                RECT rcTool;

                if (toolPtr->uFlags & TTF_IDISHWND)
                    GetWindowRect ((HWND)toolPtr->uId, &rcTool);
                else
                {
                    rcTool = toolPtr->rect;
                    MapWindowPoints (toolPtr->hwnd, NULL, (LPPOINT)&rcTool, 2);
                }

                /* smart placement */
                if ((rect.left + size.cx > rcTool.left) && (rect.left < rcTool.right) &&
                    (rect.top + size.cy > rcTool.top) && (rect.top < rcTool.bottom))
                    rect.left = rcTool.right;
            }
        }
    }
    else
    {
        if (toolPtr->uFlags & TTF_CENTERTIP)
        {
		RECT rc;

            if (toolPtr->uFlags & TTF_IDISHWND)
                GetWindowRect ((HWND)toolPtr->uId, &rc);
            else {
                rc = toolPtr->rect;
                MapWindowPoints (toolPtr->hwnd, NULL, (LPPOINT)&rc, 2);
            }
            rect.left = (rc.left + rc.right - size.cx) / 2;
            if (style & TTS_BALLOON)
            {
                ptfx = rc.left + ((rc.right - rc.left) / 2);

                /* CENTERTIP ballon tooltips default to below the field
                 * if they fit on the screen */
                if (rc.bottom + size.cy > GetSystemMetrics(SM_CYSCREEN))
                {
                    rect.top = rc.top - size.cy;
                    infoPtr->bToolBelow = FALSE;
                }
                else
                {
                    infoPtr->bToolBelow = TRUE;
                    rect.top = rc.bottom;
                }
                rect.left = max(0, rect.left - BALLOON_STEMINDENT);
            }
            else
            {
                rect.top  = rc.bottom + 2;
                infoPtr->bToolBelow = TRUE;
            }
        }
        else
        {
            GetCursorPos ((LPPOINT)&rect);
            if (style & TTS_BALLOON)
            {
                ptfx = rect.left;
                if(rect.top - size.cy >= 0)
                {
                    rect.top -= size.cy;
                    infoPtr->bToolBelow = FALSE;
                }
                else
                {
                    infoPtr->bToolBelow = TRUE;
                    rect.top += 20;
                }
                rect.left = max(0, rect.left - BALLOON_STEMINDENT);
            }
            else
            {
		    rect.top += 20;
		    infoPtr->bToolBelow = TRUE;
            }
        }
    }

    TRACE("pos %d - %d\n", rect.left, rect.top);

    rect.right = rect.left + size.cx;
    rect.bottom = rect.top + size.cy;

    /* check position */

    monitor = MonitorFromRect( &rect, MONITOR_DEFAULTTOPRIMARY );
    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW( monitor, &mon_info );

#ifdef __REACTOS__
    if (rect.right > mon_info.rcMonitor.right)
    {
        rect.left -= size.cx - (BALLOON_STEMINDENT + BALLOON_STEMWIDTH);
        rect.right -= size.cx - (BALLOON_STEMINDENT + BALLOON_STEMWIDTH);
        if (rect.right > mon_info.rcMonitor.right)
        {
            rect.left -= (rect.right - mon_info.rcMonitor.right);
            rect.right = mon_info.rcMonitor.right;
        }
    }

    if (rect.left < mon_info.rcMonitor.left)
    {
        rect.right += abs(rect.left);
        rect.left = 0;
    }

    if (rect.bottom > mon_info.rcMonitor.bottom)
    {
        RECT rc;
        if (toolPtr->uFlags & TTF_IDISHWND)
        {
            GetWindowRect((HWND)toolPtr->uId, &rc);
        }
        else
        {
            rc = toolPtr->rect;
            MapWindowPoints(toolPtr->hwnd, NULL, (LPPOINT)&rc, 2);
        }
	    rect.bottom = rc.top - 2;
    	rect.top = rect.bottom - size.cy;
    }
#else
    if( rect.right > mon_info.rcWork.right ) {
        rect.left -= rect.right - mon_info.rcWork.right + 2;
        rect.right = mon_info.rcWork.right - 2;
    }
    if (rect.left < mon_info.rcWork.left) rect.left = mon_info.rcWork.left;

    if( rect.bottom > mon_info.rcWork.bottom ) {
        RECT rc;

	if (toolPtr->uFlags & TTF_IDISHWND)
	    GetWindowRect ((HWND)toolPtr->uId, &rc);
	else {
	    rc = toolPtr->rect;
	    MapWindowPoints (toolPtr->hwnd, NULL, (LPPOINT)&rc, 2);
	}
	rect.bottom = rc.top - 2;
    	rect.top = rect.bottom - size.cy;
    }
#endif // __REACTOS__

    AdjustWindowRectEx (&rect, GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE),
			FALSE, GetWindowLongW (infoPtr->hwndSelf, GWL_EXSTYLE));

    if (style & TTS_BALLOON)
    {
        HRGN hRgn;
        HRGN hrStem;
        POINT pts[3];

        ptfx -= rect.left;

        if(infoPtr->bToolBelow)
        {
          pts[0].x = ptfx;
          pts[0].y = 0;
#ifdef __REACTOS__
          pts[1].x = max(BALLOON_STEMINDENT, ptfx - BALLOON_STEMWIDTH);
#else
          pts[1].x = max(BALLOON_STEMINDENT, ptfx - (BALLOON_STEMWIDTH / 2));
#endif
          pts[1].y = BALLOON_STEMHEIGHT;
          pts[2].x = pts[1].x + BALLOON_STEMWIDTH;
          pts[2].y = pts[1].y;
          if(pts[2].x > (rect.right - rect.left) - BALLOON_STEMINDENT)
          {
            pts[2].x = (rect.right - rect.left) - BALLOON_STEMINDENT;
            pts[1].x = pts[2].x - BALLOON_STEMWIDTH;
          }
        }
        else
        {
#ifdef __REACTOS__
          pts[0].x = max(BALLOON_STEMINDENT, ptfx - BALLOON_STEMWIDTH);
#else
          pts[0].x = max(BALLOON_STEMINDENT, ptfx - (BALLOON_STEMWIDTH / 2));
#endif
          pts[0].y = (rect.bottom - rect.top) - BALLOON_STEMHEIGHT;
          pts[1].x = pts[0].x + BALLOON_STEMWIDTH;
          pts[1].y = pts[0].y;
          pts[2].x = ptfx;
          pts[2].y = (rect.bottom - rect.top);
          if(pts[1].x > (rect.right - rect.left) - BALLOON_STEMINDENT)
          {
            pts[1].x = (rect.right - rect.left) - BALLOON_STEMINDENT;
            pts[0].x = pts[1].x - BALLOON_STEMWIDTH;
          }
        }

        hrStem = CreatePolygonRgn(pts, ARRAY_SIZE(pts), ALTERNATE);
        
        hRgn = CreateRoundRectRgn(0,
                                  (infoPtr->bToolBelow ? BALLOON_STEMHEIGHT : 0),
                                  rect.right - rect.left,
#ifdef __REACTOS__
                                  (infoPtr->bToolBelow ? rect.bottom - rect.top : rect.bottom - rect.top - BALLOON_STEMHEIGHT + 1),
#else
                                  (infoPtr->bToolBelow ? rect.bottom - rect.top : rect.bottom - rect.top - BALLOON_STEMHEIGHT),
#endif
                                  BALLOON_ROUNDEDNESS, BALLOON_ROUNDEDNESS);

        CombineRgn(hRgn, hRgn, hrStem, RGN_OR);
        DeleteObject(hrStem);

        SetWindowRgn(infoPtr->hwndSelf, hRgn, FALSE);
        /* we don't free the region handle as the system deletes it when 
         * it is no longer needed */
    }

    SetWindowPos (infoPtr->hwndSelf, NULL, rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);

    hdr.hwndFrom = infoPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_SHOW;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    SetWindowPos (infoPtr->hwndSelf, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    /* repaint the tooltip */
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    UpdateWindow(infoPtr->hwndSelf);

    if (!track_activate)
    {
        SetTimer (infoPtr->hwndSelf, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
        TRACE("timer 2 started\n");
        SetTimer (infoPtr->hwndSelf, ID_TIMERLEAVE, infoPtr->nReshowTime, 0);
        TRACE("timer 3 started\n");
    }
}


static void
TOOLTIPS_Hide (TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    TRACE("Hide tooltip %d, %p.\n", infoPtr->nCurrentTool, infoPtr->hwndSelf);

    if (infoPtr->nCurrentTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];
    KillTimer (infoPtr->hwndSelf, ID_TIMERPOP);

    hdr.hwndFrom = infoPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    infoPtr->nCurrentTool = -1;

    SetWindowPos (infoPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
}


static void
TOOLTIPS_TrackShow (TOOLTIPS_INFO *infoPtr)
{
    TOOLTIPS_Show(infoPtr, TRUE);
}


static void
TOOLTIPS_TrackHide (const TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    TRACE("hide tracking tooltip %d\n", infoPtr->nTrackTool);

    if (infoPtr->nTrackTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nTrackTool];

    hdr.hwndFrom = infoPtr->hwndSelf;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    SetWindowPos (infoPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
}

/* Structure layout is the same for TTTOOLINFOW and TTTOOLINFOA,
   this helper is used in both cases. */
static INT
TOOLTIPS_GetToolFromInfoT (const TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *lpToolInfo)
{
    TTTOOL_INFO *toolPtr;
    UINT nTool;

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (!(toolPtr->uFlags & TTF_IDISHWND) &&
	    (lpToolInfo->hwnd == toolPtr->hwnd) &&
	    (lpToolInfo->uId == toolPtr->uId))
	    return nTool;
    }

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if ((toolPtr->uFlags & TTF_IDISHWND) &&
	    (lpToolInfo->uId == toolPtr->uId))
	    return nTool;
    }

    return -1;
}


static INT
TOOLTIPS_GetToolFromPoint (const TOOLTIPS_INFO *infoPtr, HWND hwnd, const POINT *lpPt)
{
    TTTOOL_INFO *toolPtr;
    UINT nTool;

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (!(toolPtr->uFlags & TTF_IDISHWND)) {
	    if (hwnd != toolPtr->hwnd)
		continue;
	    if (!PtInRect (&toolPtr->rect, *lpPt))
		continue;
	    return nTool;
	}
    }

    for (nTool = 0; nTool < infoPtr->uNumTools; nTool++) {
	toolPtr = &infoPtr->tools[nTool];

	if (toolPtr->uFlags & TTF_IDISHWND) {
	    if ((HWND)toolPtr->uId == hwnd)
		return nTool;
	}
    }

    return -1;
}

static inline void
TOOLTIPS_CopyInfoT (const TOOLTIPS_INFO *infoPtr, INT index, TTTOOLINFOW *ti, BOOL isW)
{
    const TTTOOL_INFO *toolPtr = &infoPtr->tools[index];

    ti->uFlags = toolPtr->uFlags;
    ti->hwnd   = toolPtr->hwnd;
    ti->uId    = toolPtr->uId;
    ti->rect   = toolPtr->rect;
    ti->hinst  = toolPtr->hinst;

    if (ti->lpszText) {
        if (toolPtr->lpszText == NULL ||
            IS_INTRESOURCE(toolPtr->lpszText) ||
            toolPtr->lpszText == LPSTR_TEXTCALLBACKW)
            ti->lpszText = toolPtr->lpszText;
        else if (isW)
            lstrcpyW (ti->lpszText, toolPtr->lpszText);
        else
            /* ANSI version, the buffer is maximum 80 bytes without null. */
            WideCharToMultiByte(CP_ACP, 0, toolPtr->lpszText, -1,
                                (LPSTR)ti->lpszText, MAX_TEXT_SIZE_A, NULL, NULL);
    }

    if (ti->cbSize >= TTTOOLINFOW_V2_SIZE)
        ti->lParam = toolPtr->lParam;

    /* lpReserved is intentionally not set. */
}

static BOOL
TOOLTIPS_IsWindowActive (HWND hwnd)
{
    HWND hwndActive = GetActiveWindow ();
    if (!hwndActive)
	return FALSE;
    if (hwndActive == hwnd)
	return TRUE;
    return IsChild (hwndActive, hwnd);
}


static INT
TOOLTIPS_CheckTool (const TOOLTIPS_INFO *infoPtr, BOOL bShowTest)
{
    POINT pt;
    HWND hwndTool;
    INT nTool;

    GetCursorPos (&pt);
    hwndTool = (HWND)SendMessageW (infoPtr->hwndSelf, TTM_WINDOWFROMPOINT, 0, (LPARAM)&pt);
    if (hwndTool == 0)
	return -1;

    ScreenToClient (hwndTool, &pt);
    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
    if (nTool == -1)
	return -1;

    if (!(GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TTS_ALWAYSTIP) && bShowTest)
    {
        TTTOOL_INFO *ti = &infoPtr->tools[nTool];
        HWND hwnd = (ti->uFlags & TTF_IDISHWND) ? (HWND)ti->uId : ti->hwnd;

        if (!TOOLTIPS_IsWindowActive(hwnd))
        {
            TRACE("not active: hwnd %p, parent %p, active %p\n",
                  hwnd, GetParent(hwnd), GetActiveWindow());
            return -1;
        }
    }

    TRACE("tool %d\n", nTool);

    return nTool;
}


static LRESULT
TOOLTIPS_Activate (TOOLTIPS_INFO *infoPtr, BOOL activate)
{
    infoPtr->bActive = activate;

    TRACE("activate %d\n", activate);

    if (!(infoPtr->bActive) && (infoPtr->nCurrentTool != -1))
	TOOLTIPS_Hide (infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_AddToolT (TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *ti, BOOL isW)
{
    TTTOOL_INFO *toolPtr;
    INT nResult;

    if (!ti) return FALSE;

    TRACE("add tool (%p) %p %ld%s\n", infoPtr->hwndSelf, ti->hwnd, ti->uId,
        (ti->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

    if (ti->cbSize > TTTOOLINFOW_V3_SIZE && isW)
        return FALSE;

    if (infoPtr->uNumTools == 0) {
	infoPtr->tools = Alloc (sizeof(TTTOOL_INFO));
	toolPtr = infoPtr->tools;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools + 1));
	memcpy (infoPtr->tools, oldTools,
		infoPtr->uNumTools * sizeof(TTTOOL_INFO));
	Free (oldTools);
	toolPtr = &infoPtr->tools[infoPtr->uNumTools];
    }

    infoPtr->uNumTools++;

    /* copy tool data */
    toolPtr->uFlags         = ti->uFlags;
    toolPtr->uInternalFlags = (ti->uFlags & (TTF_SUBCLASS | TTF_IDISHWND));
    toolPtr->hwnd           = ti->hwnd;
    toolPtr->uId            = ti->uId;
    toolPtr->rect           = ti->rect;
    toolPtr->hinst          = ti->hinst;

    if (ti->cbSize >= TTTOOLINFOW_V1_SIZE) {
        if (IS_INTRESOURCE(ti->lpszText)) {
            TRACE("add string id %x\n", LOWORD(ti->lpszText));
            toolPtr->lpszText = ti->lpszText;
        }
        else if (ti->lpszText) {
            if (ti->lpszText == LPSTR_TEXTCALLBACKW) {
                TRACE("add CALLBACK\n");
                toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
            }
            else if (isW) {
                INT len = lstrlenW (ti->lpszText);
                TRACE("add text %s\n", debugstr_w(ti->lpszText));
                toolPtr->lpszText =	Alloc ((len + 1)*sizeof(WCHAR));
                lstrcpyW (toolPtr->lpszText, ti->lpszText);
            }
            else {
                INT len = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ti->lpszText, -1, NULL, 0);
                TRACE("add text \"%s\"\n", debugstr_a((char *)ti->lpszText));
                toolPtr->lpszText =	Alloc (len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)ti->lpszText, -1, toolPtr->lpszText, len);
            }
        }
    }

    if (ti->cbSize >= TTTOOLINFOW_V2_SIZE)
	toolPtr->lParam = ti->lParam;

    /* install subclassing hook */
    if (toolPtr->uInternalFlags & TTF_SUBCLASS) {
	if (toolPtr->uInternalFlags & TTF_IDISHWND) {
	    SetWindowSubclass((HWND)toolPtr->uId, TOOLTIPS_SubclassProc, 1,
			      (DWORD_PTR)infoPtr->hwndSelf);
	}
	else {
	    SetWindowSubclass(toolPtr->hwnd, TOOLTIPS_SubclassProc, 1,
			      (DWORD_PTR)infoPtr->hwndSelf);
	}
	TRACE("subclassing installed\n");
    }

    nResult = SendMessageW (toolPtr->hwnd, WM_NOTIFYFORMAT,
                            (WPARAM)infoPtr->hwndSelf, NF_QUERY);
    if (nResult == NFR_ANSI) {
        toolPtr->bNotifyUnicode = FALSE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
    } else if (nResult == NFR_UNICODE) {
        toolPtr->bNotifyUnicode = TRUE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
    } else {
        TRACE (" -- WM_NOTIFYFORMAT returns: %d\n", nResult);
    }

    return TRUE;
}

static void TOOLTIPS_ResetSubclass (const TTTOOL_INFO *toolPtr)
{
    /* Reset subclassing data. */
    if (toolPtr->uInternalFlags & TTF_SUBCLASS)
        SetWindowSubclass(toolPtr->uInternalFlags & TTF_IDISHWND ? (HWND)toolPtr->uId : toolPtr->hwnd,
            TOOLTIPS_SubclassProc, 1, 0);
}

static void TOOLTIPS_FreeToolText(TTTOOL_INFO *toolPtr)
{
    if (toolPtr->lpszText)
    {
        if (!IS_INTRESOURCE(toolPtr->lpszText) && toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
            Free(toolPtr->lpszText);
        toolPtr->lpszText = NULL;
    }
}

static void TOOLTIPS_SetToolText(TTTOOL_INFO *toolPtr, WCHAR *text, BOOL is_unicode)
{
    int len;

    TOOLTIPS_FreeToolText (toolPtr);

    if (IS_INTRESOURCE(text))
        toolPtr->lpszText = text;
    else if (text == LPSTR_TEXTCALLBACKW)
        toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
    else if (text)
    {
        if (is_unicode)
        {
            len = lstrlenW(text);
            toolPtr->lpszText = Alloc ((len + 1) * sizeof(WCHAR));
            if (toolPtr->lpszText)
                lstrcpyW (toolPtr->lpszText, text);
        }
        else
        {
            len = MultiByteToWideChar(CP_ACP, 0, (char *)text, -1, NULL, 0);
            toolPtr->lpszText = Alloc (len * sizeof(WCHAR));
            if (toolPtr->lpszText)
                MultiByteToWideChar(CP_ACP, 0, (char *)text, -1, toolPtr->lpszText, len);
        }
    }
}

static LRESULT
TOOLTIPS_DelToolT (TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *ti, BOOL isW)
{
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (!ti) return 0;
    if (isW && ti->cbSize > TTTOOLINFOW_V2_SIZE &&
               ti->cbSize != TTTOOLINFOW_V3_SIZE)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);

    TRACE("tool %d\n", nTool);

    if (nTool == -1)
        return 0;

    /* make sure the tooltip has disappeared before deleting it */
    TOOLTIPS_Hide(infoPtr);

    toolPtr = &infoPtr->tools[nTool];
    TOOLTIPS_FreeToolText (toolPtr);
    TOOLTIPS_ResetSubclass (toolPtr);

    /* delete tool from tool list */
    if (infoPtr->uNumTools == 1) {
	Free (infoPtr->tools);
	infoPtr->tools = NULL;
    }
    else {
	TTTOOL_INFO *oldTools = infoPtr->tools;
	infoPtr->tools =
	    Alloc (sizeof(TTTOOL_INFO) * (infoPtr->uNumTools - 1));

	if (nTool > 0)
	    memcpy (&infoPtr->tools[0], &oldTools[0],
		    nTool * sizeof(TTTOOL_INFO));

	if (nTool < infoPtr->uNumTools - 1)
	    memcpy (&infoPtr->tools[nTool], &oldTools[nTool + 1],
		    (infoPtr->uNumTools - nTool - 1) * sizeof(TTTOOL_INFO));

	Free (oldTools);
    }

    /* update any indices affected by delete */

    /* destroying tool that mouse was on on last relayed mouse move */
    if (infoPtr->nTool == nTool)
        /* -1 means no current tool (0 means first tool) */
        infoPtr->nTool = -1;
    else if (infoPtr->nTool > nTool)
        infoPtr->nTool--;

    if (infoPtr->nTrackTool == nTool)
        /* -1 means no current tool (0 means first tool) */
        infoPtr->nTrackTool = -1;
    else if (infoPtr->nTrackTool > nTool)
        infoPtr->nTrackTool--;

    if (infoPtr->nCurrentTool == nTool)
        /* -1 means no current tool (0 means first tool) */
        infoPtr->nCurrentTool = -1;
    else if (infoPtr->nCurrentTool > nTool)
        infoPtr->nCurrentTool--;

    infoPtr->uNumTools--;

    return 0;
}

static LRESULT
TOOLTIPS_EnumToolsT (const TOOLTIPS_INFO *infoPtr, UINT uIndex, TTTOOLINFOW *ti,
                     BOOL isW)
{
    if (!ti || ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;
    if (uIndex >= infoPtr->uNumTools)
	return FALSE;

    TRACE("index=%u\n", uIndex);

    TOOLTIPS_CopyInfoT (infoPtr, uIndex, ti, isW);

    return TRUE;
}

static LRESULT
TOOLTIPS_GetBubbleSize (const TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *lpToolInfo)
{
    INT nTool;
    SIZE size;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    TOOLTIPS_CalcTipSize (infoPtr, &size);
    TRACE("size %d x %d\n", size.cx, size.cy);

    return MAKELRESULT(size.cx, size.cy);
}

static LRESULT
TOOLTIPS_GetCurrentToolT (const TOOLTIPS_INFO *infoPtr, TTTOOLINFOW *ti, BOOL isW)
{
    if (ti) {
        if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
            return FALSE;

        if (infoPtr->nCurrentTool != -1)
            TOOLTIPS_CopyInfoT (infoPtr, infoPtr->nCurrentTool, ti, isW);
    }

    return infoPtr->nCurrentTool != -1;
}


static LRESULT
TOOLTIPS_GetDelayTime (const TOOLTIPS_INFO *infoPtr, DWORD duration)
{
    switch (duration) {
    case TTDT_RESHOW:
        return infoPtr->nReshowTime;

    case TTDT_AUTOPOP:
        return infoPtr->nAutoPopTime;

    case TTDT_INITIAL:
    case TTDT_AUTOMATIC: /* Apparently TTDT_AUTOMATIC returns TTDT_INITIAL */
        return infoPtr->nInitialTime;

    default:
        WARN("Invalid duration flag %x\n", duration);
	break;
    }

    return -1;
}


static LRESULT
TOOLTIPS_GetMargin (const TOOLTIPS_INFO *infoPtr, RECT *rect)
{
    if (rect)
        *rect = infoPtr->rcMargin;

    return 0;
}


static inline LRESULT
TOOLTIPS_GetMaxTipWidth (const TOOLTIPS_INFO *infoPtr)
{
    return infoPtr->nMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetTextT (const TOOLTIPS_INFO *infoPtr, TTTOOLINFOW *ti, BOOL isW)
{
    INT nTool;

    if (!ti) return 0;
    if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);
    if (nTool == -1) return 0;

    if (infoPtr->tools[nTool].lpszText == NULL)
	return 0;

    if (isW) {
        ti->lpszText[0] = '\0';
        TOOLTIPS_GetTipText(infoPtr, nTool, ti->lpszText);
    }
    else {
        WCHAR buffer[INFOTIPSIZE];

        /* NB this API is broken, there is no way for the app to determine
           what size buffer it requires nor a way to specify how long the
           one it supplies is.  According to the test result, it's up to
           80 bytes by the ANSI version. */

        buffer[0] = '\0';
        TOOLTIPS_GetTipText(infoPtr, nTool, buffer);
        WideCharToMultiByte(CP_ACP, 0, buffer, -1, (LPSTR)ti->lpszText,
                                                   MAX_TEXT_SIZE_A, NULL, NULL);
    }

    return 0;
}


static inline LRESULT
TOOLTIPS_GetTipBkColor (const TOOLTIPS_INFO *infoPtr)
{
    return infoPtr->clrBk;
}


static inline LRESULT
TOOLTIPS_GetTipTextColor (const TOOLTIPS_INFO *infoPtr)
{
    return infoPtr->clrText;
}


static inline LRESULT
TOOLTIPS_GetToolCount (const TOOLTIPS_INFO *infoPtr)
{
    return infoPtr->uNumTools;
}


static LRESULT
TOOLTIPS_GetToolInfoT (const TOOLTIPS_INFO *infoPtr, TTTOOLINFOW *ti, BOOL isW)
{
    INT nTool;
    HWND hwnd;

    if (!ti) return FALSE;
    if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;
    if (infoPtr->uNumTools == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d\n", nTool);

    hwnd = ti->hwnd;
    TOOLTIPS_CopyInfoT (infoPtr, nTool, ti, isW);
    ti->hwnd = hwnd;

    return TRUE;
}


static LRESULT
TOOLTIPS_HitTestT (const TOOLTIPS_INFO *infoPtr, LPTTHITTESTINFOW lptthit,
                   BOOL isW)
{
    INT nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= TTTOOLINFOW_V1_SIZE)
        TOOLTIPS_CopyInfoT (infoPtr, nTool, &lptthit->ti, isW);

    return TRUE;
}


static LRESULT
TOOLTIPS_NewToolRectT (TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *ti)
{
    INT nTool;

    if (!ti) return 0;
    if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);

    TRACE("nTool = %d, rect = %s\n", nTool, wine_dbgstr_rect(&ti->rect));

    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = ti->rect;

    return 0;
}


static inline LRESULT
TOOLTIPS_Pop (TOOLTIPS_INFO *infoPtr)
{
    TOOLTIPS_Hide (infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_RelayEvent (TOOLTIPS_INFO *infoPtr, LPMSG lpMsg)
{
    POINT pt;
    INT nOldTool;

    if (!lpMsg) {
	ERR("lpMsg == NULL\n");
	return 0;
    }

    switch (lpMsg->message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	    TOOLTIPS_Hide (infoPtr);
	    break;

	case WM_MOUSEMOVE:
	    pt.x = (short)LOWORD(lpMsg->lParam);
	    pt.y = (short)HIWORD(lpMsg->lParam);
	    nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint(infoPtr, lpMsg->hwnd,
						       &pt);
	    TRACE("tool (%p) %d %d %d\n", infoPtr->hwndSelf, nOldTool,
		  infoPtr->nTool, infoPtr->nCurrentTool);
            TRACE("WM_MOUSEMOVE (%p %s)\n", infoPtr->hwndSelf, wine_dbgstr_point(&pt));

	    if (infoPtr->nTool != nOldTool) {
	        if(infoPtr->nTool == -1) { /* Moved out of all tools */
		    TOOLTIPS_Hide(infoPtr);
		    KillTimer(infoPtr->hwndSelf, ID_TIMERLEAVE);
		} else if (nOldTool == -1) { /* Moved from outside */
		    if(infoPtr->bActive) {
		        SetTimer(infoPtr->hwndSelf, ID_TIMERSHOW, infoPtr->nInitialTime, 0);
                        TRACE("timer 1 started\n");
		    }
		} else { /* Moved from one to another */
		    TOOLTIPS_Hide (infoPtr);
		    KillTimer(infoPtr->hwndSelf, ID_TIMERLEAVE);
		    if(infoPtr->bActive) {
		        SetTimer (infoPtr->hwndSelf, ID_TIMERSHOW, infoPtr->nReshowTime, 0);
                        TRACE("timer 1 started\n");
		    }
		}
	    } else if(infoPtr->nCurrentTool != -1) { /* restart autopop */
	        KillTimer(infoPtr->hwndSelf, ID_TIMERPOP);
		SetTimer(infoPtr->hwndSelf, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
		TRACE("timer 2 restarted\n");
	    } else if(infoPtr->nTool != -1 && infoPtr->bActive) {
                /* previous show attempt didn't result in tooltip so try again */
		SetTimer(infoPtr->hwndSelf, ID_TIMERSHOW, infoPtr->nInitialTime, 0);
                TRACE("timer 1 started\n");
	    }
	    break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetDelayTime (TOOLTIPS_INFO *infoPtr, DWORD duration, INT nTime)
{
    switch (duration) {
    case TTDT_AUTOMATIC:
        if (nTime <= 0)
	    nTime = GetDoubleClickTime();
	infoPtr->nReshowTime    = nTime / 5;
	infoPtr->nAutoPopTime   = nTime * 10;
	infoPtr->nInitialTime   = nTime;
	break;

    case TTDT_RESHOW:
        if(nTime < 0)
	    nTime = GetDoubleClickTime() / 5;
	infoPtr->nReshowTime = nTime;
	break;

    case TTDT_AUTOPOP:
        if(nTime < 0)
	    nTime = GetDoubleClickTime() * 10;
	infoPtr->nAutoPopTime = nTime;
	break;

    case TTDT_INITIAL:
        if(nTime < 0)
	    nTime = GetDoubleClickTime();
	infoPtr->nInitialTime = nTime;
	    break;

    default:
        WARN("Invalid duration flag %x\n", duration);
	break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetMargin (TOOLTIPS_INFO *infoPtr, const RECT *rect)
{
    if (rect)
        infoPtr->rcMargin = *rect;

    return 0;
}


static inline LRESULT
TOOLTIPS_SetMaxTipWidth (TOOLTIPS_INFO *infoPtr, INT MaxWidth)
{
    INT nTemp = infoPtr->nMaxTipWidth;

    infoPtr->nMaxTipWidth = MaxWidth;

    return nTemp;
}


static inline LRESULT
TOOLTIPS_SetTipBkColor (TOOLTIPS_INFO *infoPtr, COLORREF clrBk)
{
    infoPtr->clrBk = clrBk;

    return 0;
}


static inline LRESULT
TOOLTIPS_SetTipTextColor (TOOLTIPS_INFO *infoPtr, COLORREF clrText)
{
    infoPtr->clrText = clrText;

    return 0;
}


static LRESULT
TOOLTIPS_SetTitleT (TOOLTIPS_INFO *infoPtr, UINT_PTR uTitleIcon, LPCWSTR pszTitle,
                    BOOL isW)
{
    UINT size;

    TRACE("hwnd = %p, title = %s, icon = %p\n", infoPtr->hwndSelf, debugstr_w(pszTitle),
        (void*)uTitleIcon);

    Free(infoPtr->pszTitle);

    if (pszTitle)
    {
        if (isW)
        {
            size = (lstrlenW(pszTitle)+1)*sizeof(WCHAR);
            infoPtr->pszTitle = Alloc(size);
            if (!infoPtr->pszTitle)
                return FALSE;
            memcpy(infoPtr->pszTitle, pszTitle, size);
        }
        else
        {
            size = sizeof(WCHAR)*MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszTitle, -1, NULL, 0);
            infoPtr->pszTitle = Alloc(size);
            if (!infoPtr->pszTitle)
                return FALSE;
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszTitle, -1, infoPtr->pszTitle, size/sizeof(WCHAR));
        }
    }
    else
        infoPtr->pszTitle = NULL;

    if (uTitleIcon <= TTI_ERROR)
        infoPtr->hTitleIcon = hTooltipIcons[uTitleIcon];
    else
        infoPtr->hTitleIcon = CopyIcon((HICON)uTitleIcon);

    TRACE("icon = %p\n", infoPtr->hTitleIcon);

    return TRUE;
}

static LRESULT
TOOLTIPS_SetToolInfoT (TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *ti, BOOL isW)
{
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (!ti) return 0;
    if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    toolPtr->uFlags = ti->uFlags;
    toolPtr->hwnd   = ti->hwnd;
    toolPtr->uId    = ti->uId;
    toolPtr->rect   = ti->rect;
    toolPtr->hinst  = ti->hinst;

    TOOLTIPS_SetToolText (toolPtr, ti->lpszText, isW);

    if (ti->cbSize >= TTTOOLINFOW_V2_SIZE)
	toolPtr->lParam = ti->lParam;

    if (infoPtr->nCurrentTool == nTool)
    {
        TOOLTIPS_GetTipText (infoPtr, infoPtr->nCurrentTool, infoPtr->szTipText);

        if (infoPtr->szTipText[0] == 0)
            TOOLTIPS_Hide(infoPtr);
        else
            TOOLTIPS_Show (infoPtr, FALSE);
    }

    return 0;
}


static LRESULT
TOOLTIPS_TrackActivate (TOOLTIPS_INFO *infoPtr, BOOL track_activate, const TTTOOLINFOA *ti)
{
    if (track_activate) {

	if (!ti) return 0;
	if (ti->cbSize < TTTOOLINFOA_V1_SIZE)
	    return FALSE;

	/* activate */
	infoPtr->nTrackTool = TOOLTIPS_GetToolFromInfoT (infoPtr, (const TTTOOLINFOW*)ti);
	if (infoPtr->nTrackTool != -1) {
	    TRACE("activated\n");
	    infoPtr->bTrackActive = TRUE;
	    TOOLTIPS_TrackShow (infoPtr);
	}
    }
    else {
	/* deactivate */
	TOOLTIPS_TrackHide (infoPtr);

	infoPtr->bTrackActive = FALSE;
	infoPtr->nTrackTool = -1;

        TRACE("deactivated\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_TrackPosition (TOOLTIPS_INFO *infoPtr, LPARAM coord)
{
    infoPtr->xTrackPos = (INT)LOWORD(coord);
    infoPtr->yTrackPos = (INT)HIWORD(coord);

    if (infoPtr->bTrackActive) {
	TRACE("[%d %d]\n",
	       infoPtr->xTrackPos, infoPtr->yTrackPos);

	TOOLTIPS_TrackShow (infoPtr);
    }

    return 0;
}


static LRESULT
TOOLTIPS_Update (TOOLTIPS_INFO *infoPtr)
{
    if (infoPtr->nCurrentTool != -1)
	UpdateWindow (infoPtr->hwndSelf);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipTextT (TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *ti, BOOL isW)
{
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (!ti) return 0;
    if (ti->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoT (infoPtr, ti);
    if (nTool == -1)
	return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    toolPtr->hinst  = ti->hinst;

    TOOLTIPS_SetToolText(toolPtr, ti->lpszText, isW);

    if(infoPtr->nCurrentTool == -1) return 0;
    /* force repaint */
    if (infoPtr->bActive)
	TOOLTIPS_Show (infoPtr, FALSE);
    else if (infoPtr->bTrackActive)
	TOOLTIPS_Show (infoPtr, TRUE);

    return 0;
}


static LRESULT
TOOLTIPS_Create (HWND hwnd)
{
    TOOLTIPS_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(TOOLTIPS_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure */
    infoPtr->bActive = TRUE;
    infoPtr->bTrackActive = FALSE;

    infoPtr->nMaxTipWidth = -1;
    infoPtr->nTool = -1;
    infoPtr->nCurrentTool = -1;
    infoPtr->nTrackTool = -1;
    infoPtr->hwndSelf = hwnd;

    /* initialize colours and fonts */
    TOOLTIPS_InitSystemSettings(infoPtr);

    TOOLTIPS_SetDelayTime(infoPtr, TTDT_AUTOMATIC, 0);

    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);

    return 0;
}


static LRESULT
TOOLTIPS_Destroy (TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    UINT i;

    for (i = 0; i < infoPtr->uNumTools; i++)
    {
        toolPtr = &infoPtr->tools[i];

        TOOLTIPS_FreeToolText (toolPtr);
        TOOLTIPS_ResetSubclass (toolPtr);
    }

    Free (infoPtr->tools);

    /* free title string */
    Free (infoPtr->pszTitle);
    /* free title icon if not a standard one */
    if (TOOLTIPS_GetTitleIconIndex(infoPtr->hTitleIcon) > TTI_ERROR)
        DeleteObject(infoPtr->hTitleIcon);

    /* delete fonts */
    DeleteObject (infoPtr->hFont);
    DeleteObject (infoPtr->hTitleFont);

    /* free tool tips info data */
    SetWindowLongPtrW(infoPtr->hwndSelf, 0, 0);
    Free (infoPtr);

    return 0;
}


static inline LRESULT
TOOLTIPS_GetFont (const TOOLTIPS_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}


static LRESULT
TOOLTIPS_MouseMessage (TOOLTIPS_INFO *infoPtr)
{
    TOOLTIPS_Hide (infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_NCCreate (HWND hwnd)
{
    DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);

    dwStyle &= ~(WS_CHILD | /*WS_MAXIMIZE |*/ WS_BORDER | WS_DLGFRAME);
    dwStyle |= (WS_POPUP | WS_BORDER | WS_CLIPSIBLINGS);

    /* WS_BORDER only draws a border round the window rect, not the
     * window region, therefore it is useless to us in balloon mode */
    if (dwStyle & TTS_BALLOON) dwStyle &= ~WS_BORDER;

    SetWindowLongW (hwnd, GWL_STYLE, dwStyle);

    dwExStyle |= WS_EX_TOOLWINDOW;
    SetWindowLongW (hwnd, GWL_EXSTYLE, dwExStyle);

    return TRUE;
}


static LRESULT
TOOLTIPS_NCHitTest (const TOOLTIPS_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT nTool = (infoPtr->bTrackActive) ? infoPtr->nTrackTool : infoPtr->nTool;

    TRACE(" nTool=%d\n", nTool);

    if ((nTool > -1) && (nTool < infoPtr->uNumTools)) {
	if (infoPtr->tools[nTool].uFlags & TTF_TRANSPARENT) {
	    TRACE("-- in transparent mode\n");
	    return HTTRANSPARENT;
	}
    }

    return DefWindowProcW (infoPtr->hwndSelf, WM_NCHITTEST, wParam, lParam);
}


static LRESULT
TOOLTIPS_NotifyFormat (TOOLTIPS_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
#ifdef __REACTOS__
    TTTOOL_INFO *toolPtr = infoPtr->tools;
    LRESULT nResult;

    TRACE("infoPtr=%p wParam=%lx lParam=%p\n", infoPtr, wParam, (PVOID)lParam);

    if (lParam == NF_QUERY) {
        if (toolPtr->bNotifyUnicode) {
            return NFR_UNICODE;
        } else {
            return NFR_ANSI;
        }
    }
    else if (lParam == NF_REQUERY) {
        nResult = SendMessageW (toolPtr->hwnd, WM_NOTIFYFORMAT,
                    (WPARAM)infoPtr->hwndSelf, (LPARAM)NF_QUERY);
        if (nResult == NFR_ANSI) {
            toolPtr->bNotifyUnicode = FALSE;
            TRACE(" -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
        } else if (nResult == NFR_UNICODE) {
            toolPtr->bNotifyUnicode = TRUE;
            TRACE(" -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
        } else {
            TRACE (" -- WM_NOTIFYFORMAT returns: error!\n");
        }
        return nResult;
    }
#else
    FIXME ("hwnd=%p wParam=%lx lParam=%lx\n", infoPtr->hwndSelf, wParam, lParam);
#endif

    return 0;
}


static LRESULT
TOOLTIPS_Paint (const TOOLTIPS_INFO *infoPtr, HDC hDC)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = (hDC == NULL) ? BeginPaint (infoPtr->hwndSelf, &ps) : hDC;
    TOOLTIPS_Refresh (infoPtr, hdc);
    if (!hDC)
	EndPaint (infoPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TOOLTIPS_SetFont (TOOLTIPS_INFO *infoPtr, HFONT hFont, BOOL redraw)
{
    LOGFONTW lf;

    if(!GetObjectW(hFont, sizeof(lf), &lf))
        return 0;

    DeleteObject (infoPtr->hFont);
    infoPtr->hFont = CreateFontIndirectW(&lf);

    DeleteObject (infoPtr->hTitleFont);
    lf.lfWeight = FW_BOLD;
    infoPtr->hTitleFont = CreateFontIndirectW(&lf);

    if (redraw && infoPtr->nCurrentTool != -1)
        FIXME("full redraw needed\n");

    return 0;
}

/******************************************************************
 * TOOLTIPS_GetTextLength
 *
 * This function is called when the tooltip receive a
 * WM_GETTEXTLENGTH message.
 *
 * returns the length, in characters, of the tip text
 */
static inline LRESULT
TOOLTIPS_GetTextLength(const TOOLTIPS_INFO *infoPtr)
{
    return lstrlenW(infoPtr->szTipText);
}

/******************************************************************
 * TOOLTIPS_OnWMGetText
 *
 * This function is called when the tooltip receive a
 * WM_GETTEXT message.
 * wParam : specifies the maximum number of characters to be copied
 * lParam : is the pointer to the buffer that will receive
 *          the tip text
 *
 * returns the number of characters copied
 */
static LRESULT
TOOLTIPS_OnWMGetText (const TOOLTIPS_INFO *infoPtr, WPARAM size, LPWSTR pszText)
{
    LRESULT res;

    if(!size)
        return 0;

    res = min(lstrlenW(infoPtr->szTipText)+1, size);
    memcpy(pszText, infoPtr->szTipText, res*sizeof(WCHAR));
    pszText[res-1] = '\0';
    return res-1;
}

static LRESULT
TOOLTIPS_Timer (TOOLTIPS_INFO *infoPtr, INT iTimer)
{
    INT nOldTool;

    TRACE("timer %d (%p) expired\n", iTimer, infoPtr->hwndSelf);

    switch (iTimer) {
    case ID_TIMERSHOW:
        KillTimer (infoPtr->hwndSelf, ID_TIMERSHOW);
	nOldTool = infoPtr->nTool;
	if ((infoPtr->nTool = TOOLTIPS_CheckTool (infoPtr, TRUE)) == nOldTool)
	    TOOLTIPS_Show (infoPtr, FALSE);
	break;

    case ID_TIMERPOP:
        TOOLTIPS_Hide (infoPtr);
	break;

    case ID_TIMERLEAVE:
        nOldTool = infoPtr->nTool;
	infoPtr->nTool = TOOLTIPS_CheckTool (infoPtr, FALSE);
	TRACE("tool (%p) %d %d %d\n", infoPtr->hwndSelf, nOldTool,
	      infoPtr->nTool, infoPtr->nCurrentTool);
	if (infoPtr->nTool != nOldTool) {
	    if(infoPtr->nTool == -1) { /* Moved out of all tools */
	        TOOLTIPS_Hide(infoPtr);
		KillTimer(infoPtr->hwndSelf, ID_TIMERLEAVE);
	    } else if (nOldTool == -1) { /* Moved from outside */
	        ERR("How did this happen?\n");
	    } else { /* Moved from one to another */
	        TOOLTIPS_Hide (infoPtr);
		KillTimer(infoPtr->hwndSelf, ID_TIMERLEAVE);
		if(infoPtr->bActive) {
		    SetTimer (infoPtr->hwndSelf, ID_TIMERSHOW, infoPtr->nReshowTime, 0);
		    TRACE("timer 1 started!\n");
		}
	    }
	}
	break;

    default:
        ERR("Unknown timer id %d\n", iTimer);
	break;
    }
    return 0;
}


static LRESULT
TOOLTIPS_WinIniChange (TOOLTIPS_INFO *infoPtr)
{
    TOOLTIPS_InitSystemSettings (infoPtr);

    return 0;
}


static LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRef)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr ((HWND)dwRef);
    MSG msg;

    switch (message)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (infoPtr)
        {
            msg.hwnd = hwnd;
            msg.message = message;
            msg.wParam = wParam;
            msg.lParam = lParam;
            TOOLTIPS_RelayEvent(infoPtr, &msg);
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, TOOLTIPS_SubclassProc, 1);
        break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}


static LRESULT CALLBACK
TOOLTIPS_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    TRACE("hwnd=%p msg=%x wparam=%lx lParam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!infoPtr && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
	case TTM_ACTIVATE:
	    return TOOLTIPS_Activate (infoPtr, (BOOL)wParam);

	case TTM_ADDTOOLA:
	case TTM_ADDTOOLW:
	    return TOOLTIPS_AddToolT (infoPtr, (LPTTTOOLINFOW)lParam, uMsg == TTM_ADDTOOLW);

	case TTM_DELTOOLA:
	case TTM_DELTOOLW:
	    return TOOLTIPS_DelToolT (infoPtr, (LPTOOLINFOW)lParam,
                                      uMsg == TTM_DELTOOLW);
	case TTM_ENUMTOOLSA:
	case TTM_ENUMTOOLSW:
	    return TOOLTIPS_EnumToolsT (infoPtr, (UINT)wParam, (LPTTTOOLINFOW)lParam,
                                        uMsg == TTM_ENUMTOOLSW);
	case TTM_GETBUBBLESIZE:
	    return TOOLTIPS_GetBubbleSize (infoPtr, (LPTTTOOLINFOW)lParam);

	case TTM_GETCURRENTTOOLA:
	case TTM_GETCURRENTTOOLW:
	    return TOOLTIPS_GetCurrentToolT (infoPtr, (LPTTTOOLINFOW)lParam,
                                             uMsg == TTM_GETCURRENTTOOLW);

	case TTM_GETDELAYTIME:
	    return TOOLTIPS_GetDelayTime (infoPtr, (DWORD)wParam);

	case TTM_GETMARGIN:
	    return TOOLTIPS_GetMargin (infoPtr, (LPRECT)lParam);

	case TTM_GETMAXTIPWIDTH:
	    return TOOLTIPS_GetMaxTipWidth (infoPtr);

	case TTM_GETTEXTA:
	case TTM_GETTEXTW:
	    return TOOLTIPS_GetTextT (infoPtr, (LPTTTOOLINFOW)lParam,
                                      uMsg == TTM_GETTEXTW);

	case TTM_GETTIPBKCOLOR:
	    return TOOLTIPS_GetTipBkColor (infoPtr);

	case TTM_GETTIPTEXTCOLOR:
	    return TOOLTIPS_GetTipTextColor (infoPtr);

	case TTM_GETTOOLCOUNT:
	    return TOOLTIPS_GetToolCount (infoPtr);

	case TTM_GETTOOLINFOA:
	case TTM_GETTOOLINFOW:
	    return TOOLTIPS_GetToolInfoT (infoPtr, (LPTTTOOLINFOW)lParam,
                                          uMsg == TTM_GETTOOLINFOW);

	case TTM_HITTESTA:
	case TTM_HITTESTW:
	    return TOOLTIPS_HitTestT (infoPtr, (LPTTHITTESTINFOW)lParam,
                                      uMsg == TTM_HITTESTW);
	case TTM_NEWTOOLRECTA:
	case TTM_NEWTOOLRECTW:
	    return TOOLTIPS_NewToolRectT (infoPtr, (LPTTTOOLINFOW)lParam);

	case TTM_POP:
	    return TOOLTIPS_Pop (infoPtr);

	case TTM_RELAYEVENT:
	    return TOOLTIPS_RelayEvent (infoPtr, (LPMSG)lParam);

	case TTM_SETDELAYTIME:
	    return TOOLTIPS_SetDelayTime (infoPtr, (DWORD)wParam, (INT)LOWORD(lParam));

	case TTM_SETMARGIN:
	    return TOOLTIPS_SetMargin (infoPtr, (LPRECT)lParam);

	case TTM_SETMAXTIPWIDTH:
	    return TOOLTIPS_SetMaxTipWidth (infoPtr, (INT)lParam);

	case TTM_SETTIPBKCOLOR:
	    return TOOLTIPS_SetTipBkColor (infoPtr, (COLORREF)wParam);

	case TTM_SETTIPTEXTCOLOR:
	    return TOOLTIPS_SetTipTextColor (infoPtr, (COLORREF)wParam);

	case TTM_SETTITLEA:
	case TTM_SETTITLEW:
	    return TOOLTIPS_SetTitleT (infoPtr, (UINT_PTR)wParam, (LPCWSTR)lParam,
                                       uMsg == TTM_SETTITLEW);

	case TTM_SETTOOLINFOA:
	case TTM_SETTOOLINFOW:
	    return TOOLTIPS_SetToolInfoT (infoPtr, (LPTTTOOLINFOW)lParam,
                                          uMsg == TTM_SETTOOLINFOW);

	case TTM_TRACKACTIVATE:
	    return TOOLTIPS_TrackActivate (infoPtr, (BOOL)wParam, (LPTTTOOLINFOA)lParam);

	case TTM_TRACKPOSITION:
	    return TOOLTIPS_TrackPosition (infoPtr, lParam);

	case TTM_UPDATE:
	    return TOOLTIPS_Update (infoPtr);

	case TTM_UPDATETIPTEXTA:
	case TTM_UPDATETIPTEXTW:
	    return TOOLTIPS_UpdateTipTextT (infoPtr, (LPTTTOOLINFOW)lParam,
                                            uMsg == TTM_UPDATETIPTEXTW);

	case TTM_WINDOWFROMPOINT:
	    return (LRESULT)WindowFromPoint (*((LPPOINT)lParam));

	case WM_CREATE:
	    return TOOLTIPS_Create (hwnd);

	case WM_DESTROY:
	    return TOOLTIPS_Destroy (infoPtr);

	case WM_ERASEBKGND:
	    /* we draw the background in WM_PAINT */
	    return 0;

	case WM_GETFONT:
	    return TOOLTIPS_GetFont (infoPtr);

	case WM_GETTEXT:
	    return TOOLTIPS_OnWMGetText (infoPtr, wParam, (LPWSTR)lParam);

	case WM_GETTEXTLENGTH:
	    return TOOLTIPS_GetTextLength (infoPtr);

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    return TOOLTIPS_MouseMessage (infoPtr);

	case WM_NCCREATE:
	    return TOOLTIPS_NCCreate (hwnd);

	case WM_NCHITTEST:
	    return TOOLTIPS_NCHitTest (infoPtr, wParam, lParam);

	case WM_NOTIFYFORMAT:
	    return TOOLTIPS_NotifyFormat (infoPtr, wParam, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return TOOLTIPS_Paint (infoPtr, (HDC)wParam);

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (infoPtr, (HFONT)wParam, LOWORD(lParam));

	case WM_SYSCOLORCHANGE:
	    COMCTL32_RefreshSysColors();
	    return 0;

	case WM_TIMER:
	    return TOOLTIPS_Timer (infoPtr, (INT)wParam);

	case WM_WININICHANGE:
	    return TOOLTIPS_WinIniChange (infoPtr);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x wp=%08lx lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
TOOLTIPS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = TOOLTIPS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLTIPS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = TOOLTIPS_CLASSW;

    RegisterClassW (&wndClass);

    hTooltipIcons[TTI_NONE] = NULL;
    hTooltipIcons[TTI_INFO] = LoadImageW(COMCTL32_hModule,
        (LPCWSTR)MAKEINTRESOURCE(IDI_TT_INFO_SM), IMAGE_ICON, 0, 0, 0);
    hTooltipIcons[TTI_WARNING] = LoadImageW(COMCTL32_hModule,
        (LPCWSTR)MAKEINTRESOURCE(IDI_TT_WARN_SM), IMAGE_ICON, 0, 0, 0);
    hTooltipIcons[TTI_ERROR] = LoadImageW(COMCTL32_hModule,
        (LPCWSTR)MAKEINTRESOURCE(IDI_TT_ERROR_SM), IMAGE_ICON, 0, 0, 0);
}


VOID
TOOLTIPS_Unregister (void)
{
    int i;
    for (i = TTI_INFO; i <= TTI_ERROR; i++)
        DestroyIcon(hTooltipIcons[i]);
    UnregisterClassW (TOOLTIPS_CLASSW, NULL);
}
