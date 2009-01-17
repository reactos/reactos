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

#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
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
#define BALLOON_ROUNDEDNESS 20
#define BALLOON_STEMHEIGHT 13
#define BALLOON_STEMWIDTH 10
#define BALLOON_STEMINDENT 20

#define BALLOON_ICON_TITLE_SPACING 8 /* horizontal spacing between icon and title */
#define BALLOON_TITLE_TEXT_SPACING 8 /* vertical spacing between icon/title and main text */
#define ICON_HEIGHT 16
#define ICON_WIDTH  16

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

    infoPtr->clrBk   = GetSysColor (COLOR_INFOBK);
    infoPtr->clrText = GetSysColor (COLOR_INFOTEXT);

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
TOOLTIPS_customdraw_fill(NMTTCUSTOMDRAW *lpnmttcd,
                         const HWND hwnd,
                         HDC hdc, const RECT *rcBounds, UINT uFlags)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(hwnd);

    ZeroMemory(lpnmttcd, sizeof(NMTTCUSTOMDRAW));
    lpnmttcd->uDrawFlags = uFlags;
    lpnmttcd->nmcd.hdr.hwndFrom = hwnd;
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
    LRESULT result = CDRF_DODEFAULT;
    lpnmttcd->nmcd.dwDrawStage = dwDrawStage;

    TRACE("Notifying stage %d, flags %x, id %x\n", lpnmttcd->nmcd.dwDrawStage,
          lpnmttcd->uDrawFlags, lpnmttcd->nmcd.hdr.code);

    result = SendMessageW(GetParent(lpnmttcd->nmcd.hdr.hwndFrom), WM_NOTIFY,
                          0, (LPARAM)lpnmttcd);

    TRACE("Notify result %x\n", (unsigned int)result);

    return result;
}

static void
TOOLTIPS_Refresh (HWND hwnd, HDC hdc)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr(hwnd);
    RECT rc;
    INT oldBkMode;
    HFONT hOldFont;
    HBRUSH hBrush;
    UINT uFlags = DT_EXTERNALLEADING;
    HRGN hRgn = NULL;
    DWORD dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    NMTTCUSTOMDRAW nmttcd;
    DWORD cdmode;

    if (infoPtr->nMaxTipWidth > -1)
	uFlags |= DT_WORDBREAK;
    if (GetWindowLongW (hwnd, GWL_STYLE) & TTS_NOPREFIX)
	uFlags |= DT_NOPREFIX;
    GetClientRect (hwnd, &rc);

    hBrush = CreateSolidBrush(infoPtr->clrBk);

    oldBkMode = SetBkMode (hdc, TRANSPARENT);
    SetTextColor (hdc, infoPtr->clrText);
    hOldFont = SelectObject (hdc, infoPtr->hFont);

    /* Custom draw - Call PrePaint once initial properties set up     */
    /* Note: Contrary to MSDN, CDRF_SKIPDEFAULT still draws a tooltip */
    TOOLTIPS_customdraw_fill(&nmttcd, hwnd, hdc, &rc, uFlags);
    cdmode = TOOLTIPS_notify_customdraw(CDDS_PREPAINT, &nmttcd);
    uFlags = nmttcd.uDrawFlags;

    if (dwStyle & TTS_BALLOON)
    {
        /* create a region to store result into */
        hRgn = CreateRectRgn(0, 0, 0, 0);

        GetWindowRgn(hwnd, hRgn);

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

static void TOOLTIPS_GetDispInfoA(HWND hwnd, TOOLTIPS_INFO *infoPtr, TTTOOL_INFO *toolPtr)
{
    NMTTDISPINFOA ttnmdi;

    /* fill NMHDR struct */
    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFOA));
    ttnmdi.hdr.hwndFrom = hwnd;
    ttnmdi.hdr.idFrom = toolPtr->uId;
    ttnmdi.hdr.code = TTN_GETDISPINFOA; /* == TTN_NEEDTEXTA */
    ttnmdi.lpszText = (LPSTR)ttnmdi.szText;
    ttnmdi.uFlags = toolPtr->uFlags;
    ttnmdi.lParam = toolPtr->lParam;

    TRACE("hdr.idFrom = %lx\n", ttnmdi.hdr.idFrom);
    SendMessageW(toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

    if (IS_INTRESOURCE(ttnmdi.lpszText)) {
        LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
               infoPtr->szTipText, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = ttnmdi.hinst;
            toolPtr->lpszText = (LPWSTR)ttnmdi.lpszText;
        }
    }
    else if (ttnmdi.lpszText == 0) {
        infoPtr->szTipText[0] = '\0';
    }
    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACKA) {
        Str_GetPtrAtoW(ttnmdi.lpszText, infoPtr->szTipText, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = 0;
            toolPtr->lpszText = NULL;
            Str_SetPtrW(&toolPtr->lpszText, infoPtr->szTipText);
        }
    }
    else {
        ERR("recursive text callback!\n");
        infoPtr->szTipText[0] = '\0';
    }

    /* no text available - try calling parent instead as per native */
    /* FIXME: Unsure if SETITEM should save the value or not        */
    if (infoPtr->szTipText[0] == 0x00) {

        SendMessageW(GetParent(toolPtr->hwnd), WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

        if (IS_INTRESOURCE(ttnmdi.lpszText)) {
            LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
                   infoPtr->szTipText, INFOTIPSIZE);
        } else if (ttnmdi.lpszText &&
                   ttnmdi.lpszText != LPSTR_TEXTCALLBACKA) {
            Str_GetPtrAtoW(ttnmdi.lpszText, infoPtr->szTipText, INFOTIPSIZE);
        }
    }
}

static void TOOLTIPS_GetDispInfoW(HWND hwnd, TOOLTIPS_INFO *infoPtr, TTTOOL_INFO *toolPtr)
{
    NMTTDISPINFOW ttnmdi;

    /* fill NMHDR struct */
    ZeroMemory (&ttnmdi, sizeof(NMTTDISPINFOW));
    ttnmdi.hdr.hwndFrom = hwnd;
    ttnmdi.hdr.idFrom = toolPtr->uId;
    ttnmdi.hdr.code = TTN_GETDISPINFOW; /* == TTN_NEEDTEXTW */
    ttnmdi.lpszText = (LPWSTR)ttnmdi.szText;
    ttnmdi.uFlags = toolPtr->uFlags;
    ttnmdi.lParam = toolPtr->lParam;

    TRACE("hdr.idFrom = %lx\n", ttnmdi.hdr.idFrom);
    SendMessageW(toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

    if (IS_INTRESOURCE(ttnmdi.lpszText)) {
        LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
               infoPtr->szTipText, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = ttnmdi.hinst;
            toolPtr->lpszText = ttnmdi.lpszText;
        }
    }
    else if (ttnmdi.lpszText == 0) {
        infoPtr->szTipText[0] = '\0';
    }
    else if (ttnmdi.lpszText != LPSTR_TEXTCALLBACKW) {
        Str_GetPtrW(ttnmdi.lpszText, infoPtr->szTipText, INFOTIPSIZE);
        if (ttnmdi.uFlags & TTF_DI_SETITEM) {
            toolPtr->hinst = 0;
            toolPtr->lpszText = NULL;
            Str_SetPtrW(&toolPtr->lpszText, infoPtr->szTipText);
        }
    }
    else {
        ERR("recursive text callback!\n");
        infoPtr->szTipText[0] = '\0';
    }

    /* no text available - try calling parent instead as per native */
    /* FIXME: Unsure if SETITEM should save the value or not        */
    if (infoPtr->szTipText[0] == 0x00) {

        SendMessageW(GetParent(toolPtr->hwnd), WM_NOTIFY, toolPtr->uId, (LPARAM)&ttnmdi);

        if (IS_INTRESOURCE(ttnmdi.lpszText)) {
            LoadStringW(ttnmdi.hinst, LOWORD(ttnmdi.lpszText),
                   infoPtr->szTipText, INFOTIPSIZE);
        } else if (ttnmdi.lpszText &&
                   ttnmdi.lpszText != LPSTR_TEXTCALLBACKW) {
            Str_GetPtrW(ttnmdi.lpszText, infoPtr->szTipText, INFOTIPSIZE);
        }
    }

}

static void
TOOLTIPS_GetTipText (HWND hwnd, TOOLTIPS_INFO *infoPtr, INT nTool)
{
    TTTOOL_INFO *toolPtr = &infoPtr->tools[nTool];

    if (IS_INTRESOURCE(toolPtr->lpszText) && toolPtr->hinst) {
	/* load a resource */
	TRACE("load res string %p %x\n",
	       toolPtr->hinst, LOWORD(toolPtr->lpszText));
	LoadStringW (toolPtr->hinst, LOWORD(toolPtr->lpszText),
		       infoPtr->szTipText, INFOTIPSIZE);
    }
    else if (toolPtr->lpszText) {
	if (toolPtr->lpszText == LPSTR_TEXTCALLBACKW) {
	    if (toolPtr->bNotifyUnicode)
		TOOLTIPS_GetDispInfoW(hwnd, infoPtr, toolPtr);
	    else
		TOOLTIPS_GetDispInfoA(hwnd, infoPtr, toolPtr);
	}
	else {
	    /* the item is a usual (unicode) text */
	    lstrcpynW (infoPtr->szTipText, toolPtr->lpszText, INFOTIPSIZE);
	}
    }
    else {
	/* no text available */
        infoPtr->szTipText[0] = '\0';
    }

    TRACE("%s\n", debugstr_w(infoPtr->szTipText));
}


static void
TOOLTIPS_CalcTipSize (HWND hwnd, const TOOLTIPS_INFO *infoPtr, LPSIZE lpSize)
{
    HDC hdc;
    HFONT hOldFont;
    DWORD style = GetWindowLongW(hwnd, GWL_STYLE);
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

    hdc = GetDC (hwnd);
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
    DrawTextW (hdc, infoPtr->szTipText, -1, &rc, uFlags);
    SelectObject (hdc, hOldFont);
    ReleaseDC (hwnd, hdc);

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
TOOLTIPS_Show (HWND hwnd, TOOLTIPS_INFO *infoPtr, BOOL track_activate)
{
    TTTOOL_INFO *toolPtr;
    HMONITOR monitor;
    MONITORINFO mon_info;
    RECT rect;
    SIZE size;
    NMHDR  hdr;
    int ptfx = 0;
    DWORD style = GetWindowLongW(hwnd, GWL_STYLE);
    INT nTool;

    if (track_activate)
    {
        if (infoPtr->nTrackTool == -1)
        {
            TRACE("invalid tracking tool (-1)!\n");
            return;
        }
        nTool = infoPtr->nTrackTool;
    }
    else
    {
        if (infoPtr->nTool == -1)
        {
            TRACE("invalid tool (-1)!\n");
		return;
        }
        nTool = infoPtr->nTool;
    }

    TRACE("Show tooltip pre %d! (%p)\n", nTool, hwnd);

    TOOLTIPS_GetTipText (hwnd, infoPtr, nTool);

    if (infoPtr->szTipText[0] == '\0')
        return;

    toolPtr = &infoPtr->tools[nTool];

    if (!track_activate)
        infoPtr->nCurrentTool = infoPtr->nTool;

    TRACE("Show tooltip %d!\n", nTool);

    hdr.hwndFrom = hwnd;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_SHOW;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    TRACE("%s\n", debugstr_w(infoPtr->szTipText));

    TOOLTIPS_CalcTipSize (hwnd, infoPtr, &size);
    TRACE("size %d x %d\n", size.cx, size.cy);

    if (track_activate)
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
        infoPtr->bToolBelow = TRUE;

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

    AdjustWindowRectEx (&rect, GetWindowLongW (hwnd, GWL_STYLE),
			FALSE, GetWindowLongW (hwnd, GWL_EXSTYLE));

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
          pts[1].x = max(BALLOON_STEMINDENT, ptfx - (BALLOON_STEMWIDTH / 2));
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
          pts[0].x = max(BALLOON_STEMINDENT, ptfx - (BALLOON_STEMWIDTH / 2));
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

        hrStem = CreatePolygonRgn(pts, sizeof(pts) / sizeof(pts[0]), ALTERNATE);
        
        hRgn = CreateRoundRectRgn(0,
                                  (infoPtr->bToolBelow ? BALLOON_STEMHEIGHT : 0),
                                  rect.right - rect.left,
                                  (infoPtr->bToolBelow ? rect.bottom - rect.top : rect.bottom - rect.top - BALLOON_STEMHEIGHT),
                                  BALLOON_ROUNDEDNESS, BALLOON_ROUNDEDNESS);

        CombineRgn(hRgn, hRgn, hrStem, RGN_OR);
        DeleteObject(hrStem);

        SetWindowRgn(hwnd, hRgn, FALSE);
        /* we don't free the region handle as the system deletes it when 
         * it is no longer needed */
    }

    SetWindowPos (hwnd, HWND_TOPMOST, rect.left, rect.top,
		    rect.right - rect.left, rect.bottom - rect.top,
		    SWP_SHOWWINDOW | SWP_NOACTIVATE);

    /* repaint the tooltip */
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    if (!track_activate)
    {
        SetTimer (hwnd, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
        TRACE("timer 2 started!\n");
        SetTimer (hwnd, ID_TIMERLEAVE, infoPtr->nReshowTime, 0);
        TRACE("timer 3 started!\n");
    }
}


static void
TOOLTIPS_Hide (HWND hwnd, TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    TRACE("Hide tooltip %d! (%p)\n", infoPtr->nCurrentTool, hwnd);

    if (infoPtr->nCurrentTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];
    KillTimer (hwnd, ID_TIMERPOP);

    hdr.hwndFrom = hwnd;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    infoPtr->nCurrentTool = -1;

    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
}


static void
TOOLTIPS_TrackShow (HWND hwnd, TOOLTIPS_INFO *infoPtr)
{
    TOOLTIPS_Show(hwnd, infoPtr, TRUE);
}


static void
TOOLTIPS_TrackHide (HWND hwnd, const TOOLTIPS_INFO *infoPtr)
{
    TTTOOL_INFO *toolPtr;
    NMHDR hdr;

    TRACE("hide tracking tooltip %d\n", infoPtr->nTrackTool);

    if (infoPtr->nTrackTool == -1)
	return;

    toolPtr = &infoPtr->tools[infoPtr->nTrackTool];

    hdr.hwndFrom = hwnd;
    hdr.idFrom = toolPtr->uId;
    hdr.code = TTN_POP;
    SendMessageW (toolPtr->hwnd, WM_NOTIFY, toolPtr->uId, (LPARAM)&hdr);

    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
}


static INT
TOOLTIPS_GetToolFromInfoA (const TOOLTIPS_INFO *infoPtr, const TTTOOLINFOA *lpToolInfo)
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
TOOLTIPS_GetToolFromInfoW (const TOOLTIPS_INFO *infoPtr, const TTTOOLINFOW *lpToolInfo)
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
TOOLTIPS_CheckTool (HWND hwnd, BOOL bShowTest)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    POINT pt;
    HWND hwndTool;
    INT nTool;

    GetCursorPos (&pt);
    hwndTool = (HWND)SendMessageW (hwnd, TTM_WINDOWFROMPOINT, 0, (LPARAM)&pt);
    if (hwndTool == 0)
	return -1;

    ScreenToClient (hwndTool, &pt);
    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, hwndTool, &pt);
    if (nTool == -1)
	return -1;

    if (!(GetWindowLongW (hwnd, GWL_STYLE) & TTS_ALWAYSTIP) && bShowTest) {
	if (!TOOLTIPS_IsWindowActive (GetWindow (hwnd, GW_OWNER)))
	    return -1;
    }

    TRACE("tool %d\n", nTool);

    return nTool;
}


static LRESULT
TOOLTIPS_Activate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    infoPtr->bActive = (BOOL)wParam;

    if (infoPtr->bActive)
	TRACE("activate!\n");

    if (!(infoPtr->bActive) && (infoPtr->nCurrentTool != -1))
	TOOLTIPS_Hide (hwnd, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_AddToolA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nResult;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return FALSE;

    TRACE("add tool (%p) %p %ld%s!\n",
	   hwnd, lpToolInfo->hwnd, lpToolInfo->uId,
	   (lpToolInfo->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

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
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)) {
	TRACE("add string id %x!\n", LOWORD(lpToolInfo->lpszText));
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA) {
	    TRACE("add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	}
	else {
	    INT len = MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText, -1,
					  NULL, 0);
	    TRACE("add text \"%s\"!\n", lpToolInfo->lpszText);
	    toolPtr->lpszText =	Alloc (len * sizeof(WCHAR));
	    MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText, -1,
				toolPtr->lpszText, len);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    SetWindowSubclass((HWND)toolPtr->uId, TOOLTIPS_SubclassProc, 1,
			       (DWORD_PTR)hwnd);
	}
	else {
	    SetWindowSubclass(toolPtr->hwnd, TOOLTIPS_SubclassProc, 1,
			      (DWORD_PTR)hwnd);
	}
	TRACE("subclassing installed!\n");
    }

    nResult = (INT) SendMessageW (toolPtr->hwnd, WM_NOTIFYFORMAT,
				  (WPARAM)hwnd, (LPARAM)NF_QUERY);
    if (nResult == NFR_ANSI) {
        toolPtr->bNotifyUnicode = FALSE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
    } else if (nResult == NFR_UNICODE) {
        toolPtr->bNotifyUnicode = TRUE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
    } else {
        TRACE (" -- WM_NOTIFYFORMAT returns: error!\n");
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_AddToolW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nResult;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    TRACE("add tool (%p) %p %ld%s!\n",
	   hwnd, lpToolInfo->hwnd, lpToolInfo->uId,
	   (lpToolInfo->uFlags & TTF_IDISHWND) ? " TTF_IDISHWND" : "");

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
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)) {
	TRACE("add string id %x\n", LOWORD(lpToolInfo->lpszText));
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW) {
	    TRACE("add CALLBACK!\n");
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	}
	else {
	    INT len = lstrlenW (lpToolInfo->lpszText);
	    TRACE("add text %s!\n",
		   debugstr_w(lpToolInfo->lpszText));
	    toolPtr->lpszText =	Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	toolPtr->lParam = lpToolInfo->lParam;

    /* install subclassing hook */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    SetWindowSubclass((HWND)toolPtr->uId, TOOLTIPS_SubclassProc, 1,
			      (DWORD_PTR)hwnd);
	}
	else {
	    SetWindowSubclass(toolPtr->hwnd, TOOLTIPS_SubclassProc, 1,
			      (DWORD_PTR)hwnd);
	}
	TRACE("subclassing installed!\n");
    }

    nResult = (INT) SendMessageW (toolPtr->hwnd, WM_NOTIFYFORMAT,
				  (WPARAM)hwnd, (LPARAM)NF_QUERY);
    if (nResult == NFR_ANSI) {
        toolPtr->bNotifyUnicode = FALSE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_ANSI\n");
    } else if (nResult == NFR_UNICODE) {
        toolPtr->bNotifyUnicode = TRUE;
	TRACE(" -- WM_NOTIFYFORMAT returns: NFR_UNICODE\n");
    } else {
        TRACE (" -- WM_NOTIFYFORMAT returns: error!\n");
    }

    return TRUE;
}


static void
TOOLTIPS_DelToolCommon (HWND hwnd, TOOLTIPS_INFO *infoPtr, INT nTool)
{
    TTTOOL_INFO *toolPtr;

    TRACE("tool %d\n", nTool);

    if (nTool == -1)
        return;

    /* make sure the tooltip has disappeared before deleting it */
    TOOLTIPS_Hide(hwnd, infoPtr);

    /* delete text string */
    toolPtr = &infoPtr->tools[nTool];
    if (toolPtr->lpszText) {
	if ( (toolPtr->lpszText != LPSTR_TEXTCALLBACKW) &&
	     !IS_INTRESOURCE(toolPtr->lpszText) )
	    Free (toolPtr->lpszText);
    }

    /* remove subclassing */
    if (toolPtr->uFlags & TTF_SUBCLASS) {
	if (toolPtr->uFlags & TTF_IDISHWND) {
	    RemoveWindowSubclass((HWND)toolPtr->uId, TOOLTIPS_SubclassProc, 1);
	}
	else {
	    RemoveWindowSubclass(toolPtr->hwnd, TOOLTIPS_SubclassProc, 1);
	}
    }

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
}

static LRESULT
TOOLTIPS_DelToolA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);

    TOOLTIPS_DelToolCommon (hwnd, infoPtr, nTool);

    return 0;
}


static LRESULT
TOOLTIPS_DelToolW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return 0;
    if (infoPtr->uNumTools == 0)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);

    TOOLTIPS_DelToolCommon (hwnd, infoPtr, nTool);

    return 0;
}


static LRESULT
TOOLTIPS_EnumToolsA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    UINT uIndex = (UINT)wParam;
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return FALSE;
    if (uIndex >= infoPtr->uNumTools)
	return FALSE;

    TRACE("index=%u\n", uIndex);

    toolPtr = &infoPtr->tools[uIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->hwnd     = toolPtr->hwnd;
    lpToolInfo->uId      = toolPtr->uId;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_EnumToolsW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    UINT uIndex = (UINT)wParam;
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;
    if (uIndex >= infoPtr->uNumTools)
	return FALSE;

    TRACE("index=%u\n", uIndex);

    toolPtr = &infoPtr->tools[uIndex];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->hwnd     = toolPtr->hwnd;
    lpToolInfo->uId      = toolPtr->uId;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}

static LRESULT
TOOLTIPS_GetBubbleSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    INT nTool;
    SIZE size;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    TOOLTIPS_CalcTipSize (hwnd, infoPtr, &size);
    TRACE("size %d x %d\n", size.cx, size.cy);

    return MAKELRESULT(size.cx, size.cy);
}

static LRESULT
TOOLTIPS_GetCurrentToolA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo) {
        if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
            return FALSE;

	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpToolInfo->uFlags   = toolPtr->uFlags;
	    lpToolInfo->rect     = toolPtr->rect;
	    lpToolInfo->hinst    = toolPtr->hinst;
/*	    lpToolInfo->lpszText = toolPtr->lpszText; */
	    lpToolInfo->lpszText = NULL;  /* FIXME */

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
		lpToolInfo->lParam = toolPtr->lParam;

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->nCurrentTool != -1);
}


static LRESULT
TOOLTIPS_GetCurrentToolW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;

    if (lpToolInfo) {
        if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
            return FALSE;

	if (infoPtr->nCurrentTool > -1) {
	    toolPtr = &infoPtr->tools[infoPtr->nCurrentTool];

	    /* copy tool data */
	    lpToolInfo->uFlags   = toolPtr->uFlags;
	    lpToolInfo->rect     = toolPtr->rect;
	    lpToolInfo->hinst    = toolPtr->hinst;
/*	    lpToolInfo->lpszText = toolPtr->lpszText; */
	    lpToolInfo->lpszText = NULL;  /* FIXME */

	    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
		lpToolInfo->lParam = toolPtr->lParam;

	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return (infoPtr->nCurrentTool != -1);
}


static LRESULT
TOOLTIPS_GetDelayTime (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    switch (wParam) {
    case TTDT_RESHOW:
        return infoPtr->nReshowTime;

    case TTDT_AUTOPOP:
        return infoPtr->nAutoPopTime;

    case TTDT_INITIAL:
    case TTDT_AUTOMATIC: /* Apparently TTDT_AUTOMATIC returns TTDT_INITIAL */
        return infoPtr->nInitialTime;

    default:
        WARN("Invalid wParam %lx\n", wParam);
	break;
    }

    return -1;
}


static LRESULT
TOOLTIPS_GetMargin (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPRECT lpRect = (LPRECT)lParam;

    lpRect->left   = infoPtr->rcMargin.left;
    lpRect->right  = infoPtr->rcMargin.right;
    lpRect->bottom = infoPtr->rcMargin.bottom;
    lpRect->top    = infoPtr->rcMargin.top;

    return 0;
}


static inline LRESULT
TOOLTIPS_GetMaxTipWidth (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    return infoPtr->nMaxTipWidth;
}


static LRESULT
TOOLTIPS_GetTextA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    /* NB this API is broken, there is no way for the app to determine
       what size buffer it requires nor a way to specify how long the
       one it supplies is.  We'll assume it's up to INFOTIPSIZE */

    WideCharToMultiByte(CP_ACP, 0, infoPtr->tools[nTool].lpszText, -1,
			lpToolInfo->lpszText, INFOTIPSIZE, NULL, NULL);

    return 0;
}


static LRESULT
TOOLTIPS_GetTextW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    if (infoPtr->tools[nTool].lpszText == NULL)
	return 0;

    strcpyW (lpToolInfo->lpszText, infoPtr->tools[nTool].lpszText);

    return 0;
}


static inline LRESULT
TOOLTIPS_GetTipBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    return infoPtr->clrBk;
}


static inline LRESULT
TOOLTIPS_GetTipTextColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    return infoPtr->clrText;
}


static inline LRESULT
TOOLTIPS_GetToolCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    return infoPtr->uNumTools;
}


static LRESULT
TOOLTIPS_GetToolInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return FALSE;
    if (infoPtr->uNumTools == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_GetToolInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return FALSE;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;
    if (infoPtr->uNumTools == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    lpToolInfo->uFlags   = toolPtr->uFlags;
    lpToolInfo->rect     = toolPtr->rect;
    lpToolInfo->hinst    = toolPtr->hinst;
/*    lpToolInfo->lpszText = toolPtr->lpszText; */
    lpToolInfo->lpszText = NULL;  /* FIXME */

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	lpToolInfo->lParam = toolPtr->lParam;

    return TRUE;
}


static LRESULT
TOOLTIPS_HitTestA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTHITTESTINFOA lptthit = (LPTTHITTESTINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d!\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFOA)) {
	toolPtr = &infoPtr->tools[nTool];

	lptthit->ti.uFlags   = toolPtr->uFlags;
	lptthit->ti.hwnd     = toolPtr->hwnd;
	lptthit->ti.uId      = toolPtr->uId;
	lptthit->ti.rect     = toolPtr->rect;
	lptthit->ti.hinst    = toolPtr->hinst;
/*	lptthit->ti.lpszText = toolPtr->lpszText; */
	lptthit->ti.lpszText = NULL;  /* FIXME */
	lptthit->ti.lParam   = toolPtr->lParam;
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_HitTestW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTHITTESTINFOW lptthit = (LPTTHITTESTINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lptthit == 0)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromPoint (infoPtr, lptthit->hwnd, &lptthit->pt);
    if (nTool == -1)
	return FALSE;

    TRACE("tool %d!\n", nTool);

    /* copy tool data */
    if (lptthit->ti.cbSize >= sizeof(TTTOOLINFOW)) {
	toolPtr = &infoPtr->tools[nTool];

	lptthit->ti.uFlags   = toolPtr->uFlags;
	lptthit->ti.hwnd     = toolPtr->hwnd;
	lptthit->ti.uId      = toolPtr->uId;
	lptthit->ti.rect     = toolPtr->rect;
	lptthit->ti.hinst    = toolPtr->hinst;
/*	lptthit->ti.lpszText = toolPtr->lpszText; */
	lptthit->ti.lpszText = NULL;  /* FIXME */
	lptthit->ti.lParam   = toolPtr->lParam;
    }

    return TRUE;
}


static LRESULT
TOOLTIPS_NewToolRectA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpti = (LPTTTOOLINFOA)lParam;
    INT nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFOA_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpti);

    TRACE("nTool = %d, rect = %s\n", nTool, wine_dbgstr_rect(&lpti->rect));

    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


static LRESULT
TOOLTIPS_NewToolRectW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpti = (LPTTTOOLINFOW)lParam;
    INT nTool;

    if (lpti == NULL)
	return 0;
    if (lpti->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpti);

    TRACE("nTool = %d, rect = %s\n", nTool, wine_dbgstr_rect(&lpti->rect));

    if (nTool == -1) return 0;

    infoPtr->tools[nTool].rect = lpti->rect;

    return 0;
}


static inline LRESULT
TOOLTIPS_Pop (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    TOOLTIPS_Hide (hwnd, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_RelayEvent (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPMSG lpMsg = (LPMSG)lParam;
    POINT pt;
    INT nOldTool;

    if (lParam == 0) {
	ERR("lpMsg == NULL!\n");
	return 0;
    }

    switch (lpMsg->message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	    TOOLTIPS_Hide (hwnd, infoPtr);
	    break;

	case WM_MOUSEMOVE:
	    pt.x = (short)LOWORD(lpMsg->lParam);
	    pt.y = (short)HIWORD(lpMsg->lParam);
	    nOldTool = infoPtr->nTool;
	    infoPtr->nTool = TOOLTIPS_GetToolFromPoint(infoPtr, lpMsg->hwnd,
						       &pt);
	    TRACE("tool (%p) %d %d %d\n", hwnd, nOldTool,
		  infoPtr->nTool, infoPtr->nCurrentTool);
            TRACE("WM_MOUSEMOVE (%p %d %d)\n", hwnd, pt.x, pt.y);

	    if (infoPtr->nTool != nOldTool) {
	        if(infoPtr->nTool == -1) { /* Moved out of all tools */
		    TOOLTIPS_Hide(hwnd, infoPtr);
		    KillTimer(hwnd, ID_TIMERLEAVE);
		} else if (nOldTool == -1) { /* Moved from outside */
		    if(infoPtr->bActive) {
		        SetTimer(hwnd, ID_TIMERSHOW, infoPtr->nInitialTime, 0);
			TRACE("timer 1 started!\n");
		    }
		} else { /* Moved from one to another */
		    TOOLTIPS_Hide (hwnd, infoPtr);
		    KillTimer(hwnd, ID_TIMERLEAVE);
		    if(infoPtr->bActive) {
		        SetTimer (hwnd, ID_TIMERSHOW, infoPtr->nReshowTime, 0);
			TRACE("timer 1 started!\n");
		    }
		}
	    } else if(infoPtr->nCurrentTool != -1) { /* restart autopop */
	        KillTimer(hwnd, ID_TIMERPOP);
		SetTimer(hwnd, ID_TIMERPOP, infoPtr->nAutoPopTime, 0);
		TRACE("timer 2 restarted\n");
	    } else if(infoPtr->nTool != -1 && infoPtr->bActive) {
                /* previous show attempt didn't result in tooltip so try again */
		SetTimer(hwnd, ID_TIMERSHOW, infoPtr->nInitialTime, 0);
		TRACE("timer 1 started!\n");
	    }
	    break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetDelayTime (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    INT nTime = (INT)LOWORD(lParam);

    switch (wParam) {
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
        WARN("Invalid wParam %lx\n", wParam);
	break;
    }

    return 0;
}


static LRESULT
TOOLTIPS_SetMargin (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPRECT lpRect = (LPRECT)lParam;

    infoPtr->rcMargin.left   = lpRect->left;
    infoPtr->rcMargin.right  = lpRect->right;
    infoPtr->rcMargin.bottom = lpRect->bottom;
    infoPtr->rcMargin.top    = lpRect->top;

    return 0;
}


static inline LRESULT
TOOLTIPS_SetMaxTipWidth (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nMaxTipWidth;

    infoPtr->nMaxTipWidth = (INT)lParam;

    return nTemp;
}


static inline LRESULT
TOOLTIPS_SetTipBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    infoPtr->clrBk = (COLORREF)wParam;

    return 0;
}


static inline LRESULT
TOOLTIPS_SetTipTextColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    infoPtr->clrText = (COLORREF)wParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetTitleA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPCSTR pszTitle = (LPCSTR)lParam;
    UINT_PTR uTitleIcon = wParam;
    UINT size;

    TRACE("hwnd = %p, title = %s, icon = %p\n", hwnd, debugstr_a(pszTitle),
        (void*)uTitleIcon);

    Free(infoPtr->pszTitle);

    if (pszTitle)
    {
        size = sizeof(WCHAR)*MultiByteToWideChar(CP_ACP, 0, pszTitle, -1, NULL, 0);
        infoPtr->pszTitle = Alloc(size);
        if (!infoPtr->pszTitle)
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, pszTitle, -1, infoPtr->pszTitle, size/sizeof(WCHAR));
    }
    else
        infoPtr->pszTitle = NULL;

    if (uTitleIcon <= TTI_ERROR)
        infoPtr->hTitleIcon = hTooltipIcons[uTitleIcon];
    else
        infoPtr->hTitleIcon = CopyIcon((HICON)wParam);

    return TRUE;
}


static LRESULT
TOOLTIPS_SetTitleW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPCWSTR pszTitle = (LPCWSTR)lParam;
    UINT_PTR uTitleIcon = wParam;
    UINT size;

    TRACE("hwnd = %p, title = %s, icon = %p\n", hwnd, debugstr_w(pszTitle),
        (void*)uTitleIcon);

    Free(infoPtr->pszTitle);

    if (pszTitle)
    {
        size = (strlenW(pszTitle)+1)*sizeof(WCHAR);
        infoPtr->pszTitle = Alloc(size);
        if (!infoPtr->pszTitle)
            return FALSE;
        memcpy(infoPtr->pszTitle, pszTitle, size);
    }
    else
        infoPtr->pszTitle = NULL;

    if (uTitleIcon <= TTI_ERROR)
        infoPtr->hTitleIcon = hTooltipIcons[uTitleIcon];
    else
        infoPtr->hTitleIcon = CopyIcon((HICON)wParam);

    TRACE("icon = %p\n", infoPtr->hTitleIcon);

    return TRUE;
}


static LRESULT
TOOLTIPS_SetToolInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)) {
	TRACE("set string id %x\n", LOWORD(lpToolInfo->lpszText));
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if ( (toolPtr->lpszText) &&
		 !IS_INTRESOURCE(toolPtr->lpszText) ) {
		if( toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
                    Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText,
					      -1, NULL, 0);
		toolPtr->lpszText = Alloc (len * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText, -1,
				    toolPtr->lpszText, len);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOA))
	toolPtr->lParam = lpToolInfo->lParam;

    return 0;
}


static LRESULT
TOOLTIPS_SetToolInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return 0;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool data */
    toolPtr->uFlags = lpToolInfo->uFlags;
    toolPtr->hwnd   = lpToolInfo->hwnd;
    toolPtr->uId    = lpToolInfo->uId;
    toolPtr->rect   = lpToolInfo->rect;
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)) {
	TRACE("set string id %x!\n", LOWORD(lpToolInfo->lpszText));
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if ( (toolPtr->lpszText) &&
		 !IS_INTRESOURCE(toolPtr->lpszText) ) {
		if( toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
                    Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenW (lpToolInfo->lpszText);
		toolPtr->lpszText = Alloc ((len+1)*sizeof(WCHAR));
		strcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if (lpToolInfo->cbSize >= sizeof(TTTOOLINFOW))
	toolPtr->lParam = lpToolInfo->lParam;

    if (infoPtr->nCurrentTool == nTool)
    {
        TOOLTIPS_GetTipText (hwnd, infoPtr, infoPtr->nCurrentTool);

        if (infoPtr->szTipText[0] == 0)
            TOOLTIPS_Hide(hwnd, infoPtr);
        else
            TOOLTIPS_Show (hwnd, infoPtr, FALSE);
    }

    return 0;
}


static LRESULT
TOOLTIPS_TrackActivate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    if ((BOOL)wParam) {
	LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;

	if (lpToolInfo == NULL)
	    return 0;
	if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	    return FALSE;

	/* activate */
	infoPtr->nTrackTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
	if (infoPtr->nTrackTool != -1) {
	    TRACE("activated!\n");
	    infoPtr->bTrackActive = TRUE;
	    TOOLTIPS_TrackShow (hwnd, infoPtr);
	}
    }
    else {
	/* deactivate */
	TOOLTIPS_TrackHide (hwnd, infoPtr);

	infoPtr->bTrackActive = FALSE;
	infoPtr->nTrackTool = -1;

	TRACE("deactivated!\n");
    }

    return 0;
}


static LRESULT
TOOLTIPS_TrackPosition (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    infoPtr->xTrackPos = (INT)LOWORD(lParam);
    infoPtr->yTrackPos = (INT)HIWORD(lParam);

    if (infoPtr->bTrackActive) {
	TRACE("[%d %d]\n",
	       infoPtr->xTrackPos, infoPtr->yTrackPos);

	TOOLTIPS_TrackShow (hwnd, infoPtr);
    }

    return 0;
}


static LRESULT
TOOLTIPS_Update (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    if (infoPtr->nCurrentTool != -1)
	UpdateWindow (hwnd);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipTextA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOA lpToolInfo = (LPTTTOOLINFOA)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOA_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoA (infoPtr, lpToolInfo);
    if (nTool == -1) return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)){
	toolPtr->lpszText = (LPWSTR)lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKA)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if ( (toolPtr->lpszText) &&
		 !IS_INTRESOURCE(toolPtr->lpszText) ) {
		if( toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
                    Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText,
					      -1, NULL, 0);
		toolPtr->lpszText = Alloc (len * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, lpToolInfo->lpszText, -1,
				    toolPtr->lpszText, len);
	    }
	}
    }

    if(infoPtr->nCurrentTool == -1) return 0;
    /* force repaint */
    if (infoPtr->bActive)
	TOOLTIPS_Show (hwnd, infoPtr, FALSE);
    else if (infoPtr->bTrackActive)
	TOOLTIPS_TrackShow (hwnd, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_UpdateTipTextW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LPTTTOOLINFOW lpToolInfo = (LPTTTOOLINFOW)lParam;
    TTTOOL_INFO *toolPtr;
    INT nTool;

    if (lpToolInfo == NULL)
	return 0;
    if (lpToolInfo->cbSize < TTTOOLINFOW_V1_SIZE)
	return FALSE;

    nTool = TOOLTIPS_GetToolFromInfoW (infoPtr, lpToolInfo);
    if (nTool == -1)
	return 0;

    TRACE("tool %d\n", nTool);

    toolPtr = &infoPtr->tools[nTool];

    /* copy tool text */
    toolPtr->hinst  = lpToolInfo->hinst;

    if (IS_INTRESOURCE(lpToolInfo->lpszText)){
	toolPtr->lpszText = lpToolInfo->lpszText;
    }
    else if (lpToolInfo->lpszText) {
	if (lpToolInfo->lpszText == LPSTR_TEXTCALLBACKW)
	    toolPtr->lpszText = LPSTR_TEXTCALLBACKW;
	else {
	    if ( (toolPtr->lpszText)  &&
		 !IS_INTRESOURCE(toolPtr->lpszText) ) {
		if( toolPtr->lpszText != LPSTR_TEXTCALLBACKW)
                    Free (toolPtr->lpszText);
		toolPtr->lpszText = NULL;
	    }
	    if (lpToolInfo->lpszText) {
		INT len = lstrlenW (lpToolInfo->lpszText);
		toolPtr->lpszText = Alloc ((len+1)*sizeof(WCHAR));
		strcpyW (toolPtr->lpszText, lpToolInfo->lpszText);
	    }
	}
    }

    if(infoPtr->nCurrentTool == -1) return 0;
    /* force repaint */
    if (infoPtr->bActive)
	TOOLTIPS_Show (hwnd, infoPtr, FALSE);
    else if (infoPtr->bTrackActive)
	TOOLTIPS_Show (hwnd, infoPtr, TRUE);

    return 0;
}


static LRESULT
TOOLTIPS_WindowFromPoint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)WindowFromPoint (*((LPPOINT)lParam));
}



static LRESULT
TOOLTIPS_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
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

    /* initialize colours and fonts */
    TOOLTIPS_InitSystemSettings(infoPtr);

    TOOLTIPS_SetDelayTime(hwnd, TTDT_AUTOMATIC, 0L);

    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);

    return 0;
}


static LRESULT
TOOLTIPS_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    TTTOOL_INFO *toolPtr;
    UINT i;

    /* free tools */
    if (infoPtr->tools) {
	for (i = 0; i < infoPtr->uNumTools; i++) {
	    toolPtr = &infoPtr->tools[i];
	    if (toolPtr->lpszText) {
		if ( (toolPtr->lpszText != LPSTR_TEXTCALLBACKW) &&
		     !IS_INTRESOURCE(toolPtr->lpszText) )
		{
		    Free (toolPtr->lpszText);
		    toolPtr->lpszText = NULL;
		}
	    }

	    /* remove subclassing */
        if (toolPtr->uFlags & TTF_SUBCLASS) {
            if (toolPtr->uFlags & TTF_IDISHWND) {
                RemoveWindowSubclass((HWND)toolPtr->uId, TOOLTIPS_SubclassProc, 1);
            }
            else {
                RemoveWindowSubclass(toolPtr->hwnd, TOOLTIPS_SubclassProc, 1);
            }
        }
    }
	Free (infoPtr->tools);
    }

    /* free title string */
    Free (infoPtr->pszTitle);
    /* free title icon if not a standard one */
    if (TOOLTIPS_GetTitleIconIndex(infoPtr->hTitleIcon) > TTI_ERROR)
        DeleteObject(infoPtr->hTitleIcon);

    /* delete fonts */
    DeleteObject (infoPtr->hFont);
    DeleteObject (infoPtr->hTitleFont);

    /* free tool tips info data */
    Free (infoPtr);
    SetWindowLongPtrW(hwnd, 0, 0);
    return 0;
}


static LRESULT
TOOLTIPS_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
TOOLTIPS_MouseMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    TOOLTIPS_Hide (hwnd, infoPtr);

    return 0;
}


static LRESULT
TOOLTIPS_NCCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
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
TOOLTIPS_NCHitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    INT nTool = (infoPtr->bTrackActive) ? infoPtr->nTrackTool : infoPtr->nTool;

    TRACE(" nTool=%d\n", nTool);

    if ((nTool > -1) && (nTool < infoPtr->uNumTools)) {
	if (infoPtr->tools[nTool].uFlags & TTF_TRANSPARENT) {
	    TRACE("-- in transparent mode!\n");
	    return HTTRANSPARENT;
	}
    }

    return DefWindowProcW (hwnd, WM_NCHITTEST, wParam, lParam);
}


static LRESULT
TOOLTIPS_NotifyFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    FIXME ("hwnd=%p wParam=%lx lParam=%lx\n", hwnd, wParam, lParam);

    return 0;
}


static LRESULT
TOOLTIPS_Paint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = (wParam == 0) ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    TOOLTIPS_Refresh (hwnd, hdc);
    if (!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
TOOLTIPS_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LOGFONTW lf;

    if(!GetObjectW((HFONT)wParam, sizeof(lf), &lf))
        return 0;

    DeleteObject (infoPtr->hFont);
    infoPtr->hFont = CreateFontIndirectW(&lf);

    DeleteObject (infoPtr->hTitleFont);
    lf.lfWeight = FW_BOLD;
    infoPtr->hTitleFont = CreateFontIndirectW(&lf);

    if ((LOWORD(lParam)) & (infoPtr->nCurrentTool != -1)) {
	FIXME("full redraw needed!\n");
    }

    return 0;
}

/******************************************************************
 * TOOLTIPS_GetTextLength
 *
 * This function is called when the tooltip receive a
 * WM_GETTEXTLENGTH message.
 * wParam : not used
 * lParam : not used
 *
 * returns the length, in characters, of the tip text
 */
static LRESULT
TOOLTIPS_GetTextLength(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    return strlenW(infoPtr->szTipText);
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
TOOLTIPS_OnWMGetText (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    LRESULT res;
    LPWSTR pszText = (LPWSTR)lParam;

    if(!infoPtr->szTipText || !wParam)
        return 0;

    res = min(strlenW(infoPtr->szTipText)+1, wParam);
    memcpy(pszText, infoPtr->szTipText, res*sizeof(WCHAR));
    pszText[res-1] = '\0';
    return res-1;
}

static LRESULT
TOOLTIPS_Timer (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);
    INT nOldTool;

    TRACE("timer %ld (%p) expired!\n", wParam, hwnd);

    switch (wParam) {
    case ID_TIMERSHOW:
        KillTimer (hwnd, ID_TIMERSHOW);
	nOldTool = infoPtr->nTool;
	if ((infoPtr->nTool = TOOLTIPS_CheckTool (hwnd, TRUE)) == nOldTool)
	    TOOLTIPS_Show (hwnd, infoPtr, FALSE);
	break;

    case ID_TIMERPOP:
        TOOLTIPS_Hide (hwnd, infoPtr);
	break;

    case ID_TIMERLEAVE:
        nOldTool = infoPtr->nTool;
	infoPtr->nTool = TOOLTIPS_CheckTool (hwnd, FALSE);
	TRACE("tool (%p) %d %d %d\n", hwnd, nOldTool,
	      infoPtr->nTool, infoPtr->nCurrentTool);
	if (infoPtr->nTool != nOldTool) {
	    if(infoPtr->nTool == -1) { /* Moved out of all tools */
	        TOOLTIPS_Hide(hwnd, infoPtr);
		KillTimer(hwnd, ID_TIMERLEAVE);
	    } else if (nOldTool == -1) { /* Moved from outside */
	        ERR("How did this happen?\n");
	    } else { /* Moved from one to another */
	        TOOLTIPS_Hide (hwnd, infoPtr);
		KillTimer(hwnd, ID_TIMERLEAVE);
		if(infoPtr->bActive) {
		    SetTimer (hwnd, ID_TIMERSHOW, infoPtr->nReshowTime, 0);
		    TRACE("timer 1 started!\n");
		}
	    }
	}
	break;

    default:
        ERR("Unknown timer id %ld\n", wParam);
	break;
    }
    return 0;
}


static LRESULT
TOOLTIPS_WinIniChange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TOOLTIPS_INFO *infoPtr = TOOLTIPS_GetInfoPtr (hwnd);

    TOOLTIPS_InitSystemSettings (infoPtr);

    return 0;
}


static LRESULT CALLBACK
TOOLTIPS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRef)
{
    MSG msg;

    switch(uMsg) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        msg.hwnd = hwnd;
	msg.message = uMsg;
	msg.wParam = wParam;
	msg.lParam = lParam;
	TOOLTIPS_RelayEvent((HWND)dwRef, 0, (LPARAM)&msg);
	break;

    default:
        break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK
TOOLTIPS_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=%x wparam=%lx lParam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!TOOLTIPS_GetInfoPtr(hwnd) && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
	case TTM_ACTIVATE:
	    return TOOLTIPS_Activate (hwnd, wParam, lParam);

	case TTM_ADDTOOLA:
	    return TOOLTIPS_AddToolA (hwnd, wParam, lParam);

	case TTM_ADDTOOLW:
	    return TOOLTIPS_AddToolW (hwnd, wParam, lParam);

	case TTM_DELTOOLA:
	    return TOOLTIPS_DelToolA (hwnd, wParam, lParam);

	case TTM_DELTOOLW:
	    return TOOLTIPS_DelToolW (hwnd, wParam, lParam);

	case TTM_ENUMTOOLSA:
	    return TOOLTIPS_EnumToolsA (hwnd, wParam, lParam);

	case TTM_ENUMTOOLSW:
	    return TOOLTIPS_EnumToolsW (hwnd, wParam, lParam);

	case TTM_GETBUBBLESIZE:
	    return TOOLTIPS_GetBubbleSize (hwnd, wParam, lParam);

	case TTM_GETCURRENTTOOLA:
	    return TOOLTIPS_GetCurrentToolA (hwnd, wParam, lParam);

	case TTM_GETCURRENTTOOLW:
	    return TOOLTIPS_GetCurrentToolW (hwnd, wParam, lParam);

	case TTM_GETDELAYTIME:
	    return TOOLTIPS_GetDelayTime (hwnd, wParam, lParam);

	case TTM_GETMARGIN:
	    return TOOLTIPS_GetMargin (hwnd, wParam, lParam);

	case TTM_GETMAXTIPWIDTH:
	    return TOOLTIPS_GetMaxTipWidth (hwnd, wParam, lParam);

	case TTM_GETTEXTA:
	    return TOOLTIPS_GetTextA (hwnd, wParam, lParam);

	case TTM_GETTEXTW:
	    return TOOLTIPS_GetTextW (hwnd, wParam, lParam);

	case TTM_GETTIPBKCOLOR:
	    return TOOLTIPS_GetTipBkColor (hwnd, wParam, lParam);

	case TTM_GETTIPTEXTCOLOR:
	    return TOOLTIPS_GetTipTextColor (hwnd, wParam, lParam);

	case TTM_GETTOOLCOUNT:
	    return TOOLTIPS_GetToolCount (hwnd, wParam, lParam);

	case TTM_GETTOOLINFOA:
	    return TOOLTIPS_GetToolInfoA (hwnd, wParam, lParam);

	case TTM_GETTOOLINFOW:
	    return TOOLTIPS_GetToolInfoW (hwnd, wParam, lParam);

	case TTM_HITTESTA:
	    return TOOLTIPS_HitTestA (hwnd, wParam, lParam);

	case TTM_HITTESTW:
	    return TOOLTIPS_HitTestW (hwnd, wParam, lParam);

	case TTM_NEWTOOLRECTA:
	    return TOOLTIPS_NewToolRectA (hwnd, wParam, lParam);

	case TTM_NEWTOOLRECTW:
	    return TOOLTIPS_NewToolRectW (hwnd, wParam, lParam);

	case TTM_POP:
	    return TOOLTIPS_Pop (hwnd, wParam, lParam);

	case TTM_RELAYEVENT:
	    return TOOLTIPS_RelayEvent (hwnd, wParam, lParam);

	case TTM_SETDELAYTIME:
	    return TOOLTIPS_SetDelayTime (hwnd, wParam, lParam);

	case TTM_SETMARGIN:
	    return TOOLTIPS_SetMargin (hwnd, wParam, lParam);

	case TTM_SETMAXTIPWIDTH:
	    return TOOLTIPS_SetMaxTipWidth (hwnd, wParam, lParam);

	case TTM_SETTIPBKCOLOR:
	    return TOOLTIPS_SetTipBkColor (hwnd, wParam, lParam);

	case TTM_SETTIPTEXTCOLOR:
	    return TOOLTIPS_SetTipTextColor (hwnd, wParam, lParam);

	case TTM_SETTITLEA:
	    return TOOLTIPS_SetTitleA (hwnd, wParam, lParam);

	case TTM_SETTITLEW:
	    return TOOLTIPS_SetTitleW (hwnd, wParam, lParam);

	case TTM_SETTOOLINFOA:
	    return TOOLTIPS_SetToolInfoA (hwnd, wParam, lParam);

	case TTM_SETTOOLINFOW:
	    return TOOLTIPS_SetToolInfoW (hwnd, wParam, lParam);

	case TTM_TRACKACTIVATE:
	    return TOOLTIPS_TrackActivate (hwnd, wParam, lParam);

	case TTM_TRACKPOSITION:
	    return TOOLTIPS_TrackPosition (hwnd, wParam, lParam);

	case TTM_UPDATE:
	    return TOOLTIPS_Update (hwnd, wParam, lParam);

	case TTM_UPDATETIPTEXTA:
	    return TOOLTIPS_UpdateTipTextA (hwnd, wParam, lParam);

	case TTM_UPDATETIPTEXTW:
	    return TOOLTIPS_UpdateTipTextW (hwnd, wParam, lParam);

	case TTM_WINDOWFROMPOINT:
	    return TOOLTIPS_WindowFromPoint (hwnd, wParam, lParam);


	case WM_CREATE:
	    return TOOLTIPS_Create (hwnd, (LPCREATESTRUCTW)lParam);

	case WM_DESTROY:
	    return TOOLTIPS_Destroy (hwnd, wParam, lParam);

	case WM_ERASEBKGND:
	    /* we draw the background in WM_PAINT */
	    return 0;

	case WM_GETFONT:
	    return TOOLTIPS_GetFont (hwnd, wParam, lParam);

	case WM_GETTEXT:
	    return TOOLTIPS_OnWMGetText (hwnd, wParam, lParam);

	case WM_GETTEXTLENGTH:
	    return TOOLTIPS_GetTextLength (hwnd, wParam, lParam);

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    return TOOLTIPS_MouseMessage (hwnd, uMsg, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLTIPS_NCCreate (hwnd, wParam, lParam);

	case WM_NCHITTEST:
	    return TOOLTIPS_NCHitTest (hwnd, wParam, lParam);

	case WM_NOTIFYFORMAT:
	    return TOOLTIPS_NotifyFormat (hwnd, wParam, lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return TOOLTIPS_Paint (hwnd, wParam, lParam);

	case WM_SETFONT:
	    return TOOLTIPS_SetFont (hwnd, wParam, lParam);

	case WM_TIMER:
	    return TOOLTIPS_Timer (hwnd, wParam, lParam);

	case WM_WININICHANGE:
	    return TOOLTIPS_WinIniChange (hwnd, wParam, lParam);

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
