/*
 * PROJECT:     ReactOS header files
 * LICENSE:     CC-BY-4.0 (https://spdx.org/licenses/CC-BY-4.0.html)
 * PURPOSE:     Win32API message dumping
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#ifndef _INC_MSGDUMP
#define _INC_MSGDUMP

/*
 * NOTE: MD_msgdump function in this file provides Win32API message dump feature.
 * NOTE: This header file takes time to compile.
 *       You might indirectly use MD_msgdump function.
 */
#pragma once

#include "winxx.h"      /* An unofficial extension of <windowsx.h>. */
#include <strsafe.h>

#ifndef MSGDUMP_PRINTF
    #error Please define MSGDUMP_PRINTF macro before #include "msgdump.h".
#endif

#ifndef MSGDUMP_API
    #define MSGDUMP_API  WINAPI
#endif

#ifndef MSGDUMP_PREFIX
    #define MSGDUMP_PREFIX ""
#endif

/* MD_msgdump function */
static __inline LRESULT MSGDUMP_API
MD_msgdump(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*---- The below codes are boring details of MD_msgdump implementation. ----*/

#define MSGDUMP_MAX_RECT_TEXT   64

static __inline const char * MSGDUMP_API
MD_rect_text(char *buf, size_t bufsize, const RECT *prc)
{
    if (prc == NULL)
    {
        StringCbCopyA(buf, bufsize, "(null)");
    }
    else
    {
        StringCbPrintfA(buf, bufsize, "(%ld, %ld, %ld, %ld)",
                        prc->left, prc->top, prc->right, prc->bottom);
    }
    return buf;
}

static __inline LRESULT MSGDUMP_API
MD_OnUnknown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_%u(hwnd:%p, wParam:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, uMsg, (void *)hwnd, (void *)wParam, (void *)lParam);
    return 0;
}

static __inline LRESULT MSGDUMP_API
MD_OnUser(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_USER+%u(hwnd:%p, wParam:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, uMsg - WM_USER, (void *)hwnd, (void *)wParam, (void *)lParam);
    return 0;
}

static __inline LRESULT MSGDUMP_API
MD_OnApp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_APP+%u(hwnd:%p, wParam:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, uMsg - WM_APP, (void *)hwnd, (void *)wParam, (void *)lParam);
    return 0;
}

static __inline LRESULT MSGDUMP_API
MD_OnNull(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_NULL(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return 0;
}

static __inline BOOL MSGDUMP_API
MD_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    MSGDUMP_PRINTF("%sWM_CREATE(hwnd:%p, lpCreateStruct:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpCreateStruct);
    return TRUE;
}

static __inline void MSGDUMP_API
MD_OnDestroy(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_DESTROY(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnMove(HWND hwnd, int x, int y)
{
    MSGDUMP_PRINTF("%sWM_MOVE(hwnd:%p, x:%d, y:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y);
}

static __inline void MSGDUMP_API
MD_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    MSGDUMP_PRINTF("%sWM_SIZE(hwnd:%p, state:%u, cx:%d, cy:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, state, cx, cy);
}

static __inline void MSGDUMP_API
MD_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    MSGDUMP_PRINTF("%sWM_ACTIVATE(hwnd:%p, state:%u, hwndActDeact:%p, fMinimized:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, state, (void *)hwndActDeact, fMinimized);
}

static __inline void MSGDUMP_API
MD_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    MSGDUMP_PRINTF("%sWM_SETFOCUS(hwnd:%p, hwndOldFocus:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndOldFocus);
}

static __inline void MSGDUMP_API
MD_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    MSGDUMP_PRINTF("%sWM_KILLFOCUS(hwnd:%p, hwndNewFocus:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndNewFocus);
}

static __inline void MSGDUMP_API
MD_OnEnable(HWND hwnd, BOOL fEnable)
{
    MSGDUMP_PRINTF("%sWM_ENABLE(hwnd:%p, fEnable:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fEnable);
}

static __inline void MSGDUMP_API
MD_OnSetRedraw(HWND hwnd, BOOL fRedraw)
{
    MSGDUMP_PRINTF("%sWM_SETREDRAW(hwnd:%p, fRedraw:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fRedraw);
}

static __inline void MSGDUMP_API
MD_OnSetText(HWND hwnd, LPCTSTR lpszText)
{
#ifdef UNICODE
    MSGDUMP_PRINTF("%sWM_SETTEXT(hwnd:%p, lpszText:%ls)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszText);
#else
    MSGDUMP_PRINTF("%sWM_SETTEXT(hwnd:%p, lpszText:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszText);
#endif
}

static __inline INT MSGDUMP_API
MD_OnGetText(HWND hwnd, int cchTextMax, LPTSTR lpszText)
{
    MSGDUMP_PRINTF("%sWM_GETTEXT(hwnd:%p, cchTextMax:%d, lpszText:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cchTextMax, (void *)lpszText);
    return 0;
}

static __inline INT MSGDUMP_API
MD_OnGetTextLength(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_GETTEXTLENGTH(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnPaint(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_PAINT(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnClose(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CLOSE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline BOOL MSGDUMP_API
MD_OnQueryEndSession(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUERYENDSESSION(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return FALSE;
}

static __inline BOOL MSGDUMP_API
MD_OnQueryOpen(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUERYOPEN(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnEndSession(HWND hwnd, BOOL fEnding)
{
    MSGDUMP_PRINTF("%sWM_ENDSESSION(hwnd:%p, fEnding:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fEnding);
}

static __inline void MSGDUMP_API
MD_OnQuit(HWND hwnd, int exitCode)
{
    MSGDUMP_PRINTF("%sWM_QUIT(hwnd:%p, exitCode:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, exitCode);
}

static __inline BOOL MSGDUMP_API
MD_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    MSGDUMP_PRINTF("%sWM_ERASEBKGND(hwnd:%p, hdc:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hdc);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnSysColorChange(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_SYSCOLORCHANGE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
{
    MSGDUMP_PRINTF("%sWM_SHOWWINDOW(hwnd:%p, fShow:%d, status:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fShow, status);
}

static __inline void MSGDUMP_API
MD_OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName)
{
#ifdef UNICODE
    MSGDUMP_PRINTF("%sWM_WININICHANGE(hwnd:%p, lpszSectionName:%ls)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszSectionName);
#else
    MSGDUMP_PRINTF("%sWM_WININICHANGE(hwnd:%p, lpszSectionName:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszSectionName);
#endif
}

static __inline void MSGDUMP_API
MD_OnSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_SETTINGCHANGE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnDevModeChange(HWND hwnd, LPCTSTR lpszDeviceName)
{
#ifdef UNICODE
    MSGDUMP_PRINTF("%sWM_DEVMODECHANGE(hwnd:%p, lpszDeviceName:%ls)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszDeviceName);
#else
    MSGDUMP_PRINTF("%sWM_DEVMODECHANGE(hwnd:%p, lpszDeviceName:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, lpszDeviceName);
#endif
}

static __inline void MSGDUMP_API
MD_OnActivateApp(HWND hwnd, BOOL fActivate, DWORD dwThreadId)
{
    MSGDUMP_PRINTF("%sWM_ACTIVATEAPP(hwnd:%p, fActivate:%d, dwThreadId:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fActivate, dwThreadId);
}

static __inline void MSGDUMP_API
MD_OnFontChange(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_FONTCHANGE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnTimeChange(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_TIMECHANGE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnCancelMode(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CANCELMODE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline BOOL MSGDUMP_API
MD_OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    MSGDUMP_PRINTF("%sWM_SETCURSOR(hwnd:%p, hwndCursor:%p, codeHitTest:%u, msg:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCursor, codeHitTest, msg);
    return FALSE;
}

static __inline int MSGDUMP_API
MD_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
{
    MSGDUMP_PRINTF("%sWM_MOUSEACTIVATE(hwnd:%p, hwndTopLevel:%p, codeHitTest:%u, msg:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndTopLevel, codeHitTest, msg);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnChildActivate(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CHILDACTIVATE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnQueueSync(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUEUESYNC(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    MSGDUMP_PRINTF("%sWM_GETMINMAXINFO(hwnd:%p, lpMinMaxInfo:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpMinMaxInfo);
}

static __inline BOOL MSGDUMP_API
MD_OnIconEraseBkgnd(HWND hwnd, HDC hdc)
{
    MSGDUMP_PRINTF("%sWM_ICONERASEBKGND(hwnd:%p, hdc:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hdc);
    return FALSE;
}

static __inline HWND MSGDUMP_API
MD_OnNextDlgCtl(HWND hwnd, HWND hwndSetFocus, BOOL fNext)
{
    MSGDUMP_PRINTF("%sWM_NEXTDLGCTL(hwnd:%p, hwndSetFocus:%p, fNext:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndSetFocus, fNext);
    return NULL;
}

static __inline void MSGDUMP_API
MD_OnSpoolerStatus(HWND hwnd, UINT status, int cJobInQueue)
{
    MSGDUMP_PRINTF("%sWM_SPOOLERSTATUS(hwnd:%p, status:%u, cJobInQueue:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, status, cJobInQueue);
}

static __inline void MSGDUMP_API
MD_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    MSGDUMP_PRINTF("%sWM_DRAWITEM(hwnd:%p, lpDrawItem:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpDrawItem);
}

static __inline void MSGDUMP_API
MD_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem)
{
    MSGDUMP_PRINTF("%sWM_MEASUREITEM(hwnd:%p, lpMeasureItem:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpMeasureItem);
}

static __inline void MSGDUMP_API
MD_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT * lpDeleteItem)
{
    MSGDUMP_PRINTF("%sWM_DELETEITEM(hwnd:%p, lpDeleteItem:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpDeleteItem);
}

static __inline int MSGDUMP_API
MD_OnVkeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
{
    MSGDUMP_PRINTF("%sWM_VKEYTOITEM(hwnd:%p, vk:%u, hwndListbox:%p, iCaret:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, vk, (void *)hwndListbox, iCaret);
    return 0;
}

static __inline int MSGDUMP_API
MD_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret)
{
    MSGDUMP_PRINTF("%sWM_CHARTOITEM(hwnd:%p, ch:%u, hwndListbox:%p, iCaret:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, (void *)hwndListbox, iCaret);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnSetFont(HWND hwnd, HFONT hfont, BOOL fRedraw)
{
    MSGDUMP_PRINTF("%sWM_SETFONT(hwnd:%p, hfont:%p, fRedraw:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hfont, fRedraw);
}

static __inline HFONT MSGDUMP_API
MD_OnGetFont(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_GETFONT(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return NULL;
}

static __inline HICON MSGDUMP_API
MD_OnQueryDragIcon(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUERYDRAGICON(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return NULL;
}

static __inline int MSGDUMP_API
MD_OnCompareItem(HWND hwnd, const COMPAREITEMSTRUCT * lpCompareItem)
{
    MSGDUMP_PRINTF("%sWM_COMPAREITEM(hwnd:%p, lpCompareItem:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpCompareItem);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnCompacting(HWND hwnd, UINT compactRatio)
{
    MSGDUMP_PRINTF("%sWM_COMPACTING(hwnd:%p, compactRatio:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, compactRatio);
}

static __inline void MSGDUMP_API
MD_OnCommNotify(HWND hwnd, int cid, UINT flags)
{
    MSGDUMP_PRINTF("%sWM_COMMNOTIFY(hwnd:%p, cid:%d, flags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cid, flags);
}

static __inline BOOL MSGDUMP_API
MD_OnWindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos)
{
    MSGDUMP_PRINTF("%sWM_WINDOWPOSCHANGING(hwnd:%p, lpwpos:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpwpos);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnWindowPosChanged(HWND hwnd, const LPWINDOWPOS lpwpos)
{
    MSGDUMP_PRINTF("%sWM_WINDOWPOSCHANGED(hwnd:%p, lpwpos:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpwpos);
}

static __inline void MSGDUMP_API
MD_OnPower(HWND hwnd, int code)
{
    MSGDUMP_PRINTF("%sWM_POWER(hwnd:%p, code:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, code);
}

static __inline BOOL MSGDUMP_API
MD_OnCopyData(HWND hwnd, HWND hwndFrom, PCOPYDATASTRUCT pcds)
{
    MSGDUMP_PRINTF("%sWM_COPYDATA(hwnd:%p, hwndFrom:%p, pcds:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndFrom, (void *)pcds);
    return FALSE;
}

static __inline LRESULT MSGDUMP_API
MD_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    MSGDUMP_PRINTF("%sWM_NOTIFY(hwnd:%p, idFrom:%d, pnmhdr:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, idFrom, (void *)pnmhdr);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    MSGDUMP_PRINTF("%sWM_CONTEXTMENU(hwnd:%p, hwndContext:%p, xPos:%u, yPos:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndContext, xPos, yPos);
}

static __inline void MSGDUMP_API
MD_OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen)
{
    MSGDUMP_PRINTF("%sWM_DISPLAYCHANGE(hwnd:%p, bitsPerPixel:%u, cxScreen:%u, cyScreen:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, bitsPerPixel, cxScreen, cyScreen);
}

static __inline BOOL MSGDUMP_API
MD_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    MSGDUMP_PRINTF("%sWM_NCCREATE(hwnd:%p, lpCreateStruct:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpCreateStruct);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnNCDestroy(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_NCDESTROY(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline UINT MSGDUMP_API
MD_OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp)
{
    MSGDUMP_PRINTF("%sWM_NCCALCSIZE(hwnd:%p, fCalcValidRects:%d, lpcsp:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fCalcValidRects, (void *)lpcsp);
    return 0;
}

static __inline UINT MSGDUMP_API
MD_OnNCHitTest(HWND hwnd, int x, int y)
{
    MSGDUMP_PRINTF("%sWM_NCHITTEST(hwnd:%p, x:%d, y:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnNCPaint(HWND hwnd, HRGN hrgn)
{
    MSGDUMP_PRINTF("%sWM_NCPAINT(hwnd:%p, hrgn:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hrgn);
}

static __inline BOOL MSGDUMP_API
MD_OnNCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized)
{
    MSGDUMP_PRINTF("%sWM_NCACTIVATE(hwnd:%p, fActive:%d, hwndActDeact:%p, fMinimized:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fActive, (void *)hwndActDeact, fMinimized);
    return FALSE;
}

static __inline UINT MSGDUMP_API
MD_OnGetDlgCode(HWND hwnd, LPMSG lpmsg)
{
    MSGDUMP_PRINTF("%sWM_GETDLGCODE(hwnd:%p, lpmsg:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpmsg);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
{
    MSGDUMP_PRINTF("%sWM_NCMOUSEMOVE(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
}

static __inline void MSGDUMP_API
MD_OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_NCLBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_NCLBUTTONDOWN(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
}

static __inline void MSGDUMP_API
MD_OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
    MSGDUMP_PRINTF("%sWM_NCLBUTTONUP(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
}

static __inline void MSGDUMP_API
MD_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_NCRBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_NCRBUTTONDOWN(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
}

static __inline void MSGDUMP_API
MD_OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
    MSGDUMP_PRINTF("%sWM_NCRBUTTONUP(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
}

static __inline void MSGDUMP_API
MD_OnNCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_NCMBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_NCMBUTTONDOWN(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
    }
}

static __inline void MSGDUMP_API
MD_OnNCMButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
    MSGDUMP_PRINTF("%sWM_NCMBUTTONUP(hwnd:%p, x:%d, y:%d, codeHitTest:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, codeHitTest);
}

static __inline void MSGDUMP_API
MD_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (fDown)
    {
        MSGDUMP_PRINTF("%sWM_KEYDOWN(hwnd:%p, vk:%u, cRepeat:%d, flags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, vk, cRepeat, flags);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_KEYUP(hwnd:%p, vk:%u, cRepeat:%d, flags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, vk, cRepeat, flags);
    }
}

static __inline void MSGDUMP_API
MD_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    MSGDUMP_PRINTF("%sWM_CHAR(hwnd:%p, ch:%u, cRepeat:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, cRepeat);
}

static __inline void MSGDUMP_API
MD_OnDeadChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    MSGDUMP_PRINTF("%sWM_DEADCHAR(hwnd:%p, ch:%u, cRepeat:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, cRepeat);
}

static __inline void MSGDUMP_API
MD_OnSysKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (fDown)
    {
        MSGDUMP_PRINTF("%sWM_SYSKEYDOWN(hwnd:%p, vk:%u, cRepeat:%d, flags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, vk, cRepeat, flags);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_SYSKEYUP(hwnd:%p, vk:%u, cRepeat:%d, flags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, vk, cRepeat, flags);
    }
}

static __inline void MSGDUMP_API
MD_OnSysChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    MSGDUMP_PRINTF("%sWM_SYSCHAR(hwnd:%p, ch:%u, cRepeat:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, cRepeat);
}

static __inline void MSGDUMP_API
MD_OnSysDeadChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    MSGDUMP_PRINTF("%sWM_SYSDEADCHAR(hwnd:%p, ch:%u, cRepeat:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, cRepeat);
}

static __inline BOOL MSGDUMP_API
MD_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_INITDIALOG(hwnd:%p, hwndFocus:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndFocus, (void *)lParam);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    MSGDUMP_PRINTF("%sWM_COMMAND(hwnd:%p, id:%d, hwndCtl:%p, codeNotify:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, id, (void *)hwndCtl, codeNotify);
}

static __inline void MSGDUMP_API
MD_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
    MSGDUMP_PRINTF("%sWM_SYSCOMMAND(hwnd:%p, cmd:%u, x:%d, y:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cmd, x, y);
}

static __inline void MSGDUMP_API
MD_OnTimer(HWND hwnd, UINT id)
{
    MSGDUMP_PRINTF("%sWM_TIMER(hwnd:%p, id:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, id);
}

static __inline void MSGDUMP_API
MD_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    MSGDUMP_PRINTF("%sWM_HSCROLL(hwnd:%p, hwndCtl:%p, code:%u, pos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCtl, code, pos);
}

static __inline void MSGDUMP_API
MD_OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    MSGDUMP_PRINTF("%sWM_VSCROLL(hwnd:%p, hwndCtl:%p, code:%u, pos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCtl, code, pos);
}

static __inline void MSGDUMP_API
MD_OnInitMenu(HWND hwnd, HMENU hMenu)
{
    MSGDUMP_PRINTF("%sWM_INITMENU(hwnd:%p, hMenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hMenu);
}

static __inline void MSGDUMP_API
MD_OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    MSGDUMP_PRINTF("%sWM_INITMENUPOPUP(hwnd:%p, hMenu:%p, item:%u, fSystemMenu:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hMenu, item, fSystemMenu);
}

static __inline void MSGDUMP_API
MD_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
    MSGDUMP_PRINTF("%sWM_MENUSELECT(hwnd:%p, hmenu:%p, item:%d, hmenuPopup:%p, flags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hmenu, item, (void *)hmenuPopup, flags);
}

static __inline DWORD MSGDUMP_API
MD_OnMenuChar(HWND hwnd, UINT ch, UINT flags, HMENU hmenu)
{
    MSGDUMP_PRINTF("%sWM_MENUCHAR(hwnd:%p, ch:%u, flags:%u, hmenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, ch, flags, (void *)hmenu);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnEnterIdle(HWND hwnd, UINT source, HWND hwndSource)
{
    MSGDUMP_PRINTF("%sWM_ENTERIDLE(hwnd:%p, source:%u, hwndSource:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, source, (void *)hwndSource);
}

static __inline HBRUSH MSGDUMP_API
MD_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    MSGDUMP_PRINTF("%sWM_CTLCOLOR(hwnd:%p, hdc:%p, hwndChild:%p, type:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hdc, (void *)hwndChild, type);
    return NULL;
}

static __inline void MSGDUMP_API
MD_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    MSGDUMP_PRINTF("%sWM_MOUSEMOVE(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
}

static __inline void MSGDUMP_API
MD_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_LBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_LBUTTONDOWN(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
}

static __inline void MSGDUMP_API
MD_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    MSGDUMP_PRINTF("%sWM_LBUTTONUP(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
}

static __inline void MSGDUMP_API
MD_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_RBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_RBUTTONDOWN(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
}

static __inline void MSGDUMP_API
MD_OnRButtonUp(HWND hwnd, int x, int y, UINT flags)
{
    MSGDUMP_PRINTF("%sWM_RBUTTONUP(hwnd:%p, x:%d, y:%d, flags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, flags);
}

static __inline void MSGDUMP_API
MD_OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_MBUTTONDBLCLK(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_MBUTTONDOWN(hwnd:%p, x:%d, y:%d, keyFlags:%u)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, x, y, keyFlags);
    }
}

static __inline void MSGDUMP_API
MD_OnMButtonUp(HWND hwnd, int x, int y, UINT flags)
{
    MSGDUMP_PRINTF("%sWM_MBUTTONUP(hwnd:%p, x:%d, y:%d, flags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, x, y, flags);
}

static __inline void MSGDUMP_API
MD_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    MSGDUMP_PRINTF("%sWM_MOUSEWHEEL(hwnd:%p, xPos:%d, yPos:%d, zDelta:%d, fwKeys:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, xPos, yPos, zDelta, fwKeys);
}

static __inline void MSGDUMP_API
MD_OnParentNotify(HWND hwnd, UINT msg, HWND hwndChild, int idChild)
{
    MSGDUMP_PRINTF("%sWM_PARENTNOTIFY(hwnd:%p, msg:%u, hwndChild:%p, idChild:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, msg, (void *)hwndChild, idChild);
}

static __inline BOOL MSGDUMP_API
MD_OnDeviceChange(HWND hwnd, UINT uEvent, DWORD dwEventData)
{
    MSGDUMP_PRINTF("%sWM_DEVICECHANGE(hwnd:%p, uEvent:%u, dwEventData:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, uEvent, dwEventData);
    return FALSE;
}

static __inline HWND MSGDUMP_API
MD_MDICreate(HWND hwnd, const LPMDICREATESTRUCT lpmcs)
{
    MSGDUMP_PRINTF("%sWM_MDICREATE(hwnd:%p, lpmcs:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpmcs);
    return NULL;
}

static __inline void MSGDUMP_API
MD_MDIDestroy(HWND hwnd, HWND hwndDestroy)
{
    MSGDUMP_PRINTF("%sWM_MDIDESTROY(hwnd:%p, hwndDestroy:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndDestroy);
}

static __inline void MSGDUMP_API
MD_MDIActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)
{
    MSGDUMP_PRINTF("%sWM_MDIACTIVATE(hwnd:%p, fActive:%d, hwndActivate:%p, hwndDeactivate:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fActive, (void *)hwndActivate, (void *)hwndDeactivate);
}

static __inline void MSGDUMP_API
MD_MDIRestore(HWND hwnd, HWND hwndRestore)
{
    MSGDUMP_PRINTF("%sWM_MDIRESTORE(hwnd:%p, hwndRestore:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndRestore);
}

static __inline HWND MSGDUMP_API
MD_MDINext(HWND hwnd, HWND hwndCur, BOOL fPrev)
{
    MSGDUMP_PRINTF("%sWM_MDINEXT(hwnd:%p, hwndCur:%p, fPrev:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCur, fPrev);
    return NULL;
}

static __inline void MSGDUMP_API
MD_MDIMaximize(HWND hwnd, HWND hwndMaximize)
{
    MSGDUMP_PRINTF("%sWM_MDIMAXIMIZE(hwnd:%p, hwndMaximize:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndMaximize);
}

static __inline BOOL MSGDUMP_API
MD_MDITile(HWND hwnd, UINT cmd)
{
    MSGDUMP_PRINTF("%sWM_MDITILE(hwnd:%p, cmd:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cmd);
    return FALSE;
}

static __inline BOOL MSGDUMP_API
MD_MDICascade(HWND hwnd, UINT cmd)
{
    MSGDUMP_PRINTF("%sWM_MDICASCADE(hwnd:%p, cmd:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cmd);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_MDIIconArrange(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_MDIICONARRANGE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline HWND MSGDUMP_API
MD_MDIGetActive(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_MDIGETACTIVE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return NULL;
}

static __inline HMENU MSGDUMP_API
MD_MDISetMenu(HWND hwnd, BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow)
{
    MSGDUMP_PRINTF("%sWM_MDISETMENU(hwnd:%p, fRefresh:%d, hmenuFrame:%p, hmenuWindow:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fRefresh, (void *)hmenuFrame, (void *)hmenuWindow);
    return NULL;
}

static __inline void MSGDUMP_API
MD_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    MSGDUMP_PRINTF("%sWM_DROPFILES(hwnd:%p, hdrop:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hdrop);
}

static __inline void MSGDUMP_API
MD_OnCut(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CUT(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnCopy(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_COPY(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnPaste(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_PASTE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnClear(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CLEAR(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnUndo(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_UNDO(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline HANDLE MSGDUMP_API
MD_OnRenderFormat(HWND hwnd, UINT fmt)
{
    MSGDUMP_PRINTF("%sWM_RENDERFORMAT(hwnd:%p, fmt:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fmt);
    return NULL;
}

static __inline void MSGDUMP_API
MD_OnRenderAllFormats(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_RENDERALLFORMATS(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnDestroyClipboard(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_DESTROYCLIPBOARD(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnDrawClipboard(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_DRAWCLIPBOARD(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnPaintClipboard(HWND hwnd, HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct)
{
    MSGDUMP_PRINTF("%sWM_PAINTCLIPBOARD(hwnd:%p, hwndCBViewer:%p, lpPaintStruct:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCBViewer, (void *)lpPaintStruct);
}

static __inline void MSGDUMP_API
MD_OnVScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos)
{
    MSGDUMP_PRINTF("%sWM_VSCROLLCLIPBOARD(hwnd:%p, hwndCBViewer:%p, code:%u, pos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCBViewer, code, pos);
}

static __inline void MSGDUMP_API
MD_OnSizeClipboard(HWND hwnd, HWND hwndCBViewer, const LPRECT lprc)
{
    char buf[MSGDUMP_MAX_RECT_TEXT];
    MSGDUMP_PRINTF("%sWM_SIZECLIPBOARD(hwnd:%p, hwndCBViewer:%p, lprc:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCBViewer,
                   MD_rect_text(buf, sizeof(buf), lprc));
}

static __inline void MSGDUMP_API
MD_OnAskCBFormatName(HWND hwnd, int cchMax, LPTSTR rgchName)
{
    MSGDUMP_PRINTF("%sWM_ASKCBFORMATNAME(hwnd:%p, cchMax:%d, rgchName:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, cchMax, (void *)rgchName);
}

static __inline void MSGDUMP_API
MD_OnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext)
{
    MSGDUMP_PRINTF("%sWM_CHANGECBCHAIN(hwnd:%p, hwndRemove:%p, hwndNext:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndRemove, (void *)hwndNext);
}

static __inline void MSGDUMP_API
MD_OnHScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos)
{
    MSGDUMP_PRINTF("%sWM_HSCROLLCLIPBOARD(hwnd:%p, hwndCBViewer:%p, code:%u, pos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndCBViewer, code, pos);
}

static __inline BOOL MSGDUMP_API
MD_OnQueryNewPalette(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUERYNEWPALETTE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return FALSE;
}

static __inline void MSGDUMP_API
MD_OnPaletteIsChanging(HWND hwnd, HWND hwndPaletteChange)
{
    MSGDUMP_PRINTF("%sWM_PALETTEISCHANGING(hwnd:%p, hwndPaletteChange:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndPaletteChange);
}

static __inline void MSGDUMP_API
MD_OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
{
    MSGDUMP_PRINTF("%sWM_PALETTECHANGED(hwnd:%p, hwndPaletteChange:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndPaletteChange);
}

static __inline void MSGDUMP_API
MD_OnHotKey(HWND hwnd, int idHotKey, UINT fuModifiers, UINT vk)
{
    MSGDUMP_PRINTF("%sWM_HOTKEY(hwnd:%p, idHotKey:%d, fuModifiers:%u, vk:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, idHotKey, fuModifiers, vk);
}

static __inline INT MSGDUMP_API
MD_OnSetHotKey(HWND hwnd, INT nCode, UINT nOptions)
{
    MSGDUMP_PRINTF("%sWM_SETHOTKEY(hwnd:%p, nCode:%d, nOptions:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nCode, nOptions);
    return 0;
}

static __inline INT MSGDUMP_API
MD_OnGetHotKey(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_GETHOTKEY(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnPaintIcon(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_PAINTICON(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline LRESULT MSGDUMP_API
MD_OnGetObject(HWND hwnd, WPARAM wParam, DWORD dwObjId)
{
    MSGDUMP_PRINTF("%sWM_GETOBJECT(hwnd:%p, wParam:%p, dwObjId:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)wParam, dwObjId);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnCancelJournal(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_CANCELJOURNAL(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnInputLangChangeRequest(HWND hwnd, BOOL bFlag, HKL hKL)
{
    MSGDUMP_PRINTF("%sWM_INPUTLANGCHANGEREQUEST(hwnd:%p, bFlag:%d, hKL:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, bFlag, (void *)hKL);
}

static __inline void MSGDUMP_API
MD_OnInputLangChange(HWND hwnd, DWORD dwCharSet, HKL hKL)
{
    MSGDUMP_PRINTF("%sWM_INPUTLANGCHANGE(hwnd:%p, dwCharSet:0x%08lX, hKL:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, dwCharSet, (void *)hKL);
}

static __inline void MSGDUMP_API
MD_OnTCard(HWND hwnd, UINT idAction, DWORD dwActionData)
{
    MSGDUMP_PRINTF("%sWM_TCARD(hwnd:%p, idAction:%u, dwActionData:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, idAction, dwActionData);
}

static __inline void MSGDUMP_API
MD_OnHelp(HWND hwnd, LPHELPINFO lpHelpInfo)
{
    MSGDUMP_PRINTF("%sWM_HELP(hwnd:%p, lpHelpInfo:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)lpHelpInfo);
}

static __inline void MSGDUMP_API
MD_OnUserChanged(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_USERCHANGED(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline INT MSGDUMP_API
MD_OnNotifyFormat(HWND hwnd, HWND hwndTarget, INT nCommand)
{
    MSGDUMP_PRINTF("%sWM_NOTIFYFORMAT(hwnd:%p, hwndTarget:%p, nCommand:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndTarget, nCommand);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnStyleChanging(HWND hwnd, UINT nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
    MSGDUMP_PRINTF("%sWM_STYLECHANGING(hwnd:%p, nStyleType:%u, lpStyleStruct:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nStyleType, (void *)lpStyleStruct);
}

static __inline void MSGDUMP_API
MD_OnStyleChanged(HWND hwnd, UINT nStyleType, const STYLESTRUCT *lpStyleStruct)
{
    MSGDUMP_PRINTF("%sWM_STYLECHANGED(hwnd:%p, nStyleType:%u, lpStyleStruct:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nStyleType, (void *)lpStyleStruct);
}

static __inline HICON MSGDUMP_API
MD_OnGetIcon(HWND hwnd, UINT nType, LPARAM dpi)
{
    MSGDUMP_PRINTF("%sWM_GETICON(hwnd:%p, nType:%u, dpi:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nType, (void *)dpi);
    return NULL;
}

static __inline HICON MSGDUMP_API
MD_OnSetIcon(HWND hwnd, UINT nType, HICON hIcon)
{
    MSGDUMP_PRINTF("%sWM_SETICON(hwnd:%p, nType:%u, hIcon:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nType, (void *)hIcon);
    return NULL;
}

static __inline void MSGDUMP_API
MD_OnSyncPaint(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_SYNCPAINT(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnNCXButtonDown(HWND hwnd, BOOL fDoubleClick, UINT nHitTest, WORD fwButton,
                   INT xPos, INT yPos)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_NCXBUTTONDBLCLK(hwnd:%p, nHitTest:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, nHitTest, fwButton, xPos, yPos);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_NCXBUTTONDOWN(hwnd:%p, nHitTest:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, nHitTest, fwButton, xPos, yPos);
    }
}

static __inline void MSGDUMP_API
MD_OnNCXButtonUp(HWND hwnd, UINT nHitTest, WORD fwButton, INT xPos, INT yPos)
{
    MSGDUMP_PRINTF("%sWM_NCXBUTTONUP(hwnd:%p, nHitTest:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nHitTest, fwButton, xPos, yPos);
}

static __inline void MSGDUMP_API
MD_OnImeStartComposition(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_IME_STARTCOMPOSITION(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnImeEndComposition(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_IME_ENDCOMPOSITION(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnImeComposition(HWND hwnd, WORD wChar, DWORD lAttribute)
{
    MSGDUMP_PRINTF("%sWM_IME_COMPOSITION(hwnd:%p, wChar:%u, lAttribute:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, wChar, lAttribute);
}

static __inline void MSGDUMP_API
MD_OnMenuRButtonUp(HWND hwnd, UINT nPos, HMENU hMenu)
{
    MSGDUMP_PRINTF("%sWM_MENURBUTTONUP(hwnd:%p, nPos:%u, hMenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nPos, (void *)hMenu);
}

static __inline UINT MSGDUMP_API
MD_OnMenuDrag(HWND hwnd, UINT nPos, HMENU hMenu)
{
    MSGDUMP_PRINTF("%sWM_MENUDRAG(hwnd:%p, nPos:%u, hMenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nPos, (void *)hMenu);
    return 0;
}

static __inline UINT MSGDUMP_API
MD_OnMenuGetObject(HWND hwnd, MENUGETOBJECTINFO *pmgoi)
{
    MSGDUMP_PRINTF("%sWM_MENUGETOBJECT(hwnd:%p, pmgoi:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)pmgoi);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnUninitMenuPopup(HWND hwnd, HMENU hMenu, UINT nFlags)
{
    MSGDUMP_PRINTF("%sWM_UNINITMENUPOPUP(hwnd:%p, hMenu:%p, nFlags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hMenu, nFlags);
}

static __inline void MSGDUMP_API
MD_OnMenuCommand(HWND hwnd, UINT nPos, HMENU hMenu)
{
    MSGDUMP_PRINTF("%sWM_MENUCOMMAND(hwnd:%p, nPos:%u, hMenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nPos, (void *)hMenu);
}

static __inline void MSGDUMP_API
MD_OnChangeUIState(HWND hwnd, UINT nAction, UINT nUIElement)
{
    MSGDUMP_PRINTF("%sWM_CHANGEUISTATE(hwnd:%p, nAction:%u, nUIElement:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nAction, nUIElement);
}

static __inline void MSGDUMP_API
MD_OnUpdateUIState(HWND hwnd, UINT nAction, UINT nUIElement)
{
    MSGDUMP_PRINTF("%sWM_UPDATEUISTATE(hwnd:%p, nAction:%u, nUIElement:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nAction, nUIElement);
}

static __inline UINT MSGDUMP_API
MD_OnQueryUIState(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_QUERYUISTATE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnXButtonDown(HWND hwnd, BOOL fDoubleClick, WORD fwKeys, WORD fwButton, INT xPos, INT yPos)
{
    if (fDoubleClick)
    {
        MSGDUMP_PRINTF("%sWM_XBUTTONDBLCLK(hwnd:%p, fwKeys:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, fwKeys, fwButton, xPos, yPos);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_XBUTTONDOWN(hwnd:%p, fwKeys:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, fwKeys, fwButton, xPos, yPos);
    }
}

static __inline void MSGDUMP_API
MD_OnXButtonUp(HWND hwnd, WORD fwKeys, WORD fwButton, INT xPos, INT yPos)
{
    MSGDUMP_PRINTF("%sWM_XBUTTONUP(hwnd:%p, fwKeys:%u, fwButton:%u, xPos:%d, yPos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fwKeys, fwButton, xPos, yPos);
}

static __inline void MSGDUMP_API
MD_OnEnterMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu)
{
    MSGDUMP_PRINTF("%sWM_ENTERMENULOOP(hwnd:%p, bIsTrackPopupMenu:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, bIsTrackPopupMenu);
}

static __inline void MSGDUMP_API
MD_OnExitMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu)
{
    MSGDUMP_PRINTF("%sWM_EXITMENULOOP(hwnd:%p, bIsTrackPopupMenu:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, bIsTrackPopupMenu);
}

static __inline void MSGDUMP_API
MD_OnNextMenu(HWND hwnd, INT nCode, LPMDINEXTMENU lpMDINextMenu)
{
    MSGDUMP_PRINTF("%sWM_NEXTMENU(hwnd:%p, nCode:%d, lpMDINextMenu:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nCode, (void *)lpMDINextMenu);
}

static __inline void MSGDUMP_API
MD_OnSizing(HWND hwnd, UINT nSide, LPRECT lpRect)
{
    char buf[MSGDUMP_MAX_RECT_TEXT];
    MSGDUMP_PRINTF("%sWM_SIZING(hwnd:%p, nSide:%u, lpRect:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nSide, MD_rect_text(buf, sizeof(buf), lpRect));
}

static __inline void MSGDUMP_API
MD_OnCaptureChanged(HWND hwnd, HWND hwndNewCapture)
{
    MSGDUMP_PRINTF("%sWM_CAPTURECHANGED(hwnd:%p, hwndNewCapture:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndNewCapture);
}

static __inline void MSGDUMP_API
MD_OnMoving(HWND hwnd, UINT nSide, LPRECT lpRect)
{
    char buf[MSGDUMP_MAX_RECT_TEXT];
    MSGDUMP_PRINTF("%sWM_MOVING(hwnd:%p, nSide:%u, lpRect:%s)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nSide, MD_rect_text(buf, sizeof(buf), lpRect));
}

static __inline LRESULT MSGDUMP_API
MD_OnPowerBroadcast(HWND hwnd, UINT nPowerEvent, UINT nEventData)
{
    MSGDUMP_PRINTF("%sWM_POWERBROADCAST(hwnd:%p, nPowerEvent:%u, nEventData:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nPowerEvent, nEventData);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnEnterSizeMove(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_ENTERSIZEMOVE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnExitSizeMove(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_EXITSIZEMOVE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline HMENU MSGDUMP_API
MD_MDIRefreshMenu(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_MDIREFRESHMENU(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
    return NULL;
}

static __inline BOOL MSGDUMP_API
MD_OnImeSetContext(HWND hwnd, BOOL fActive, DWORD dwShow)
{
    MSGDUMP_PRINTF("%sWM_IME_SETCONTEXT(hwnd:%p, fActive:%d, dwShow:0x%08lX)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fActive, dwShow);
    return FALSE;
}

static __inline LRESULT MSGDUMP_API
MD_OnImeNotify(HWND hwnd, WPARAM wSubMessage, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_IME_NOTIFY(hwnd:%p, wSubMessage:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)wSubMessage, (void *)lParam);
    return 0;
}

static __inline LRESULT MSGDUMP_API
MD_OnImeControl(HWND hwnd, WPARAM wSubMessage, LPVOID lpData)
{
    MSGDUMP_PRINTF("%sWM_IME_CONTROL(hwnd:%p, wSubMessage:%p, lpData:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)wSubMessage, (void *)lpData);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnImeCompositionFull(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_IME_COMPOSITIONFULL(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnImeSelect(HWND hwnd, BOOL fSelect, HKL hKL)
{
    MSGDUMP_PRINTF("%sWM_IME_SELECT(hwnd:%p, fSelect:%d, hKL:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, fSelect, (void *)hKL);
}

static __inline void MSGDUMP_API
MD_OnImeChar(HWND hwnd, WORD wCharCode, LONG lKeyData)
{
    MSGDUMP_PRINTF("%sWM_IME_CHAR(hwnd:%p, wCharCode:%u, lKeyData:%ld)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, wCharCode, lKeyData);
}

static __inline LRESULT MSGDUMP_API
MD_OnImeRequest(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MSGDUMP_PRINTF("%sWM_IME_REQUEST(hwnd:%p, wParam:%p, lParam:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)wParam, (void *)lParam);
    return 0;
}

static __inline void MSGDUMP_API
MD_OnImeKey(HWND hwnd, BOOL fDown, UINT nVirtKey, LONG lKeyData)
{
    if (fDown)
    {
        MSGDUMP_PRINTF("%sWM_IME_KEYDOWN(hwnd:%p, nVirtKey:%u, lKeyData:%ld)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, nVirtKey, lKeyData);
    }
    else
    {
        MSGDUMP_PRINTF("%sWM_IME_KEYUP(hwnd:%p, nVirtKey:%u, lKeyData:%ld)\n",
                       MSGDUMP_PREFIX, (void *)hwnd, nVirtKey, lKeyData);
    }
}

static __inline void MSGDUMP_API
MD_OnMouseHover(HWND hwnd, UINT nFlags, INT xPos, INT yPos)
{
    MSGDUMP_PRINTF("%sWM_MOUSEHOVER(hwnd:%p, nFlags:%u, xPos:%d, yPos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nFlags, xPos, yPos);
}

static __inline void MSGDUMP_API
MD_OnMouseLeave(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_MOUSELEAVE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnNCMouseHover(HWND hwnd, UINT nHitTest, INT xPos, INT yPos)
{
    MSGDUMP_PRINTF("%sWM_NCMOUSEHOVER(hwnd:%p, nHitTest:%u, xPos:%d, yPos:%d)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, nHitTest, xPos, yPos);
}

static __inline void MSGDUMP_API
MD_OnNCMouseLeave(HWND hwnd)
{
    MSGDUMP_PRINTF("%sWM_NCMOUSELEAVE(hwnd:%p)\n",
                   MSGDUMP_PREFIX, (void *)hwnd);
}

static __inline void MSGDUMP_API
MD_OnPrint(HWND hwnd, HDC hDC, UINT uFlags)
{
    MSGDUMP_PRINTF("%sWM_PRINT(hwnd:%p, hDC:%p, uFlags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hDC, uFlags);
}

static __inline void MSGDUMP_API
MD_OnPrintClient(HWND hwnd, HDC hDC, UINT uFlags)
{
    MSGDUMP_PRINTF("%sWM_PRINTCLIENT(hwnd:%p, hDC:%p, uFlags:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hDC, uFlags);
}

static __inline BOOL MSGDUMP_API
MD_OnAppCommand(HWND hwnd, HWND hwndTarget, UINT cmd, UINT nDevice, UINT nKey)
{
    MSGDUMP_PRINTF("%sWM_APPCOMMAND(hwnd:%p, hwndTarget:%p, cmd:%u, nDevice:%u, nKey:%u)\n",
                   MSGDUMP_PREFIX, (void *)hwnd, (void *)hwndTarget, cmd, nDevice, nKey);
    return FALSE;
}

static __inline LRESULT MSGDUMP_API
MD_msgdump(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_NULL, MD_OnNull);
        HANDLE_MSG(hwnd, WM_CREATE, MD_OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, MD_OnDestroy);
        HANDLE_MSG(hwnd, WM_MOVE, MD_OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, MD_OnSize);
        HANDLE_MSG(hwnd, WM_ACTIVATE, MD_OnActivate);
        HANDLE_MSG(hwnd, WM_SETFOCUS, MD_OnSetFocus);
        HANDLE_MSG(hwnd, WM_KILLFOCUS, MD_OnKillFocus);
        HANDLE_MSG(hwnd, WM_ENABLE, MD_OnEnable);
        HANDLE_MSG(hwnd, WM_SETREDRAW, MD_OnSetRedraw);
        HANDLE_MSG(hwnd, WM_SETTEXT, MD_OnSetText);
        HANDLE_MSG(hwnd, WM_GETTEXT, MD_OnGetText);
        HANDLE_MSG(hwnd, WM_GETTEXTLENGTH, MD_OnGetTextLength);
        HANDLE_MSG(hwnd, WM_PAINT, MD_OnPaint);
        HANDLE_MSG(hwnd, WM_CLOSE, MD_OnClose);
#ifndef _WIN32_WCE
        HANDLE_MSG(hwnd, WM_QUERYENDSESSION, MD_OnQueryEndSession);
        HANDLE_MSG(hwnd, WM_QUERYOPEN, MD_OnQueryOpen);
        HANDLE_MSG(hwnd, WM_ENDSESSION, MD_OnEndSession);
#endif
        HANDLE_MSG(hwnd, WM_QUIT, MD_OnQuit);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, MD_OnEraseBkgnd);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, MD_OnSysColorChange);
        HANDLE_MSG(hwnd, WM_SHOWWINDOW, MD_OnShowWindow);
        HANDLE_MSG(hwnd, WM_WININICHANGE, MD_OnWinIniChange);
        /*HANDLE_MSG(hwnd, WM_SETTINGCHANGE, MD_OnSettingChange);*/
            /* WM_SETTINGCHANGE duplicates WM_WININICHANGE */
        HANDLE_MSG(hwnd, WM_DEVMODECHANGE, MD_OnDevModeChange);
        HANDLE_MSG(hwnd, WM_ACTIVATEAPP, MD_OnActivateApp);
        HANDLE_MSG(hwnd, WM_FONTCHANGE, MD_OnFontChange);
        HANDLE_MSG(hwnd, WM_TIMECHANGE, MD_OnTimeChange);
        HANDLE_MSG(hwnd, WM_CANCELMODE, MD_OnCancelMode);
        HANDLE_MSG(hwnd, WM_SETCURSOR, MD_OnSetCursor);
        HANDLE_MSG(hwnd, WM_MOUSEACTIVATE, MD_OnMouseActivate);
        HANDLE_MSG(hwnd, WM_CHILDACTIVATE, MD_OnChildActivate);
        HANDLE_MSG(hwnd, WM_QUEUESYNC, MD_OnQueueSync);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, MD_OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_PAINTICON, MD_OnPaintIcon);
        HANDLE_MSG(hwnd, WM_ICONERASEBKGND, MD_OnIconEraseBkgnd);
        HANDLE_MSG(hwnd, WM_NEXTDLGCTL, MD_OnNextDlgCtl);
        HANDLE_MSG(hwnd, WM_SPOOLERSTATUS, MD_OnSpoolerStatus);
        HANDLE_MSG(hwnd, WM_DRAWITEM, MD_OnDrawItem);
        HANDLE_MSG(hwnd, WM_MEASUREITEM, MD_OnMeasureItem);
        HANDLE_MSG(hwnd, WM_DELETEITEM, MD_OnDeleteItem);
        HANDLE_MSG(hwnd, WM_VKEYTOITEM, MD_OnVkeyToItem);
        HANDLE_MSG(hwnd, WM_CHARTOITEM, MD_OnCharToItem);
        HANDLE_MSG(hwnd, WM_SETFONT, MD_OnSetFont);
        HANDLE_MSG(hwnd, WM_GETFONT, MD_OnGetFont);
        HANDLE_MSG(hwnd, WM_SETHOTKEY, MD_OnSetHotKey);
        HANDLE_MSG(hwnd, WM_GETHOTKEY, MD_OnGetHotKey);
        HANDLE_MSG(hwnd, WM_QUERYDRAGICON, MD_OnQueryDragIcon);
        HANDLE_MSG(hwnd, WM_COMPAREITEM, MD_OnCompareItem);
#if WINVER >= 0x0500
# ifndef _WIN32_WCE
        HANDLE_MSG(hwnd, WM_GETOBJECT, MD_OnGetObject);
# endif
#endif
        HANDLE_MSG(hwnd, WM_COMPACTING, MD_OnCompacting);
        HANDLE_MSG(hwnd, WM_COMMNOTIFY, MD_OnCommNotify);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, MD_OnWindowPosChanging);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, MD_OnWindowPosChanged);
        HANDLE_MSG(hwnd, WM_POWER, MD_OnPower);
        HANDLE_MSG(hwnd, WM_COPYDATA, MD_OnCopyData);
        HANDLE_MSG(hwnd, WM_CANCELJOURNAL, MD_OnCancelJournal);
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_NOTIFY, MD_OnNotify);
        HANDLE_MSG(hwnd, WM_INPUTLANGCHANGEREQUEST, MD_OnInputLangChangeRequest);
        HANDLE_MSG(hwnd, WM_INPUTLANGCHANGE, MD_OnInputLangChange);
        HANDLE_MSG(hwnd, WM_TCARD, MD_OnTCard);
        HANDLE_MSG(hwnd, WM_HELP, MD_OnHelp);
        HANDLE_MSG(hwnd, WM_USERCHANGED, MD_OnUserChanged);
        HANDLE_MSG(hwnd, WM_NOTIFYFORMAT, MD_OnNotifyFormat);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, MD_OnContextMenu);
        HANDLE_MSG(hwnd, WM_STYLECHANGING, MD_OnStyleChanging);
        HANDLE_MSG(hwnd, WM_STYLECHANGED, MD_OnStyleChanged);
        HANDLE_MSG(hwnd, WM_DISPLAYCHANGE, MD_OnDisplayChange);
        HANDLE_MSG(hwnd, WM_GETICON, MD_OnGetIcon);
        HANDLE_MSG(hwnd, WM_SETICON, MD_OnSetIcon);
#endif
        HANDLE_MSG(hwnd, WM_NCCREATE, MD_OnNCCreate);
        HANDLE_MSG(hwnd, WM_NCDESTROY, MD_OnNCDestroy);
        HANDLE_MSG(hwnd, WM_NCCALCSIZE, MD_OnNCCalcSize);
        HANDLE_MSG(hwnd, WM_NCHITTEST, MD_OnNCHitTest);
        HANDLE_MSG(hwnd, WM_NCPAINT, MD_OnNCPaint);
        HANDLE_MSG(hwnd, WM_NCACTIVATE, MD_OnNCActivate);
        HANDLE_MSG(hwnd, WM_GETDLGCODE, MD_OnGetDlgCode);
#ifndef _WIN32_WCE
        HANDLE_MSG(hwnd, WM_SYNCPAINT, MD_OnSyncPaint);
#endif
        HANDLE_MSG(hwnd, WM_NCMOUSEMOVE, MD_OnNCMouseMove);
        HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, MD_OnNCLButtonDown);
        HANDLE_MSG(hwnd, WM_NCLBUTTONUP, MD_OnNCLButtonUp);
        HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK, MD_OnNCLButtonDown);
        HANDLE_MSG(hwnd, WM_NCRBUTTONDOWN, MD_OnNCRButtonDown);
        HANDLE_MSG(hwnd, WM_NCRBUTTONUP, MD_OnNCRButtonUp);
        HANDLE_MSG(hwnd, WM_NCRBUTTONDBLCLK, MD_OnNCRButtonDown);
        HANDLE_MSG(hwnd, WM_NCMBUTTONDOWN, MD_OnNCMButtonDown);
        HANDLE_MSG(hwnd, WM_NCMBUTTONUP, MD_OnNCMButtonUp);
        HANDLE_MSG(hwnd, WM_NCMBUTTONDBLCLK, MD_OnNCMButtonDown);
#if _WIN32_WINNT >= 0x0500
        HANDLE_MSG(hwnd, WM_NCXBUTTONDOWN, MD_OnNCXButtonDown);
        HANDLE_MSG(hwnd, WM_NCXBUTTONUP, MD_OnNCXButtonUp);
        HANDLE_MSG(hwnd, WM_NCXBUTTONDBLCLK, MD_OnNCXButtonDown);
#endif
        HANDLE_MSG(hwnd, WM_KEYDOWN, MD_OnKey);
        HANDLE_MSG(hwnd, WM_KEYUP, MD_OnKey);
        HANDLE_MSG(hwnd, WM_CHAR, MD_OnChar);
        HANDLE_MSG(hwnd, WM_DEADCHAR, MD_OnDeadChar);
        HANDLE_MSG(hwnd, WM_SYSKEYDOWN, MD_OnSysKey);
        HANDLE_MSG(hwnd, WM_SYSKEYUP, MD_OnSysKey);
        HANDLE_MSG(hwnd, WM_SYSCHAR, MD_OnSysChar);
        HANDLE_MSG(hwnd, WM_SYSDEADCHAR, MD_OnSysDeadChar);
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_IME_STARTCOMPOSITION, MD_OnImeStartComposition);
        HANDLE_MSG(hwnd, WM_IME_ENDCOMPOSITION, MD_OnImeEndComposition);
        HANDLE_MSG(hwnd, WM_IME_COMPOSITION, MD_OnImeComposition);
#endif
        HANDLE_MSG(hwnd, WM_INITDIALOG, MD_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, MD_OnCommand);
        HANDLE_MSG(hwnd, WM_SYSCOMMAND, MD_OnSysCommand);
        HANDLE_MSG(hwnd, WM_TIMER, MD_OnTimer);
        HANDLE_MSG(hwnd, WM_HSCROLL, MD_OnHScroll);
        HANDLE_MSG(hwnd, WM_VSCROLL, MD_OnVScroll);
        HANDLE_MSG(hwnd, WM_INITMENU, MD_OnInitMenu);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, MD_OnInitMenuPopup);
        HANDLE_MSG(hwnd, WM_MENUSELECT, MD_OnMenuSelect);
        HANDLE_MSG(hwnd, WM_MENUCHAR, MD_OnMenuChar);
        HANDLE_MSG(hwnd, WM_ENTERIDLE, MD_OnEnterIdle);
#if WINVER >= 0x0500
# ifndef _WIN32_WCE
        HANDLE_MSG(hwnd, WM_MENURBUTTONUP, MD_OnMenuRButtonUp);
        HANDLE_MSG(hwnd, WM_MENUDRAG, MD_OnMenuDrag);
        HANDLE_MSG(hwnd, WM_MENUGETOBJECT, MD_OnMenuGetObject);
        HANDLE_MSG(hwnd, WM_UNINITMENUPOPUP, MD_OnUninitMenuPopup);
        HANDLE_MSG(hwnd, WM_MENUCOMMAND, MD_OnMenuCommand);
#  ifndef _WIN32_WCE
#   if _WIN32_WINNT >= 0x0500
        HANDLE_MSG(hwnd, WM_CHANGEUISTATE, MD_OnChangeUIState);
        HANDLE_MSG(hwnd, WM_UPDATEUISTATE, MD_OnUpdateUIState);
        HANDLE_MSG(hwnd, WM_QUERYUISTATE, MD_OnQueryUIState);
#   endif
#  endif
# endif
#endif
        HANDLE_MSG(hwnd, WM_CTLCOLORMSGBOX, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLOREDIT, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORLISTBOX, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORBTN, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORDLG, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORSCROLLBAR, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, MD_OnCtlColor);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, MD_OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, MD_OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, MD_OnLButtonUp);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, MD_OnLButtonDown);
        HANDLE_MSG(hwnd, WM_RBUTTONDOWN, MD_OnRButtonDown);
        HANDLE_MSG(hwnd, WM_RBUTTONUP, MD_OnRButtonUp);
        HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK, MD_OnRButtonDown);
        HANDLE_MSG(hwnd, WM_MBUTTONDOWN, MD_OnMButtonDown);
        HANDLE_MSG(hwnd, WM_MBUTTONUP, MD_OnMButtonUp);
        HANDLE_MSG(hwnd, WM_MBUTTONDBLCLK, MD_OnMButtonDown);
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
        HANDLE_MSG(hwnd, WM_MOUSEWHEEL, MD_OnMouseWheel);
#endif
#if _WIN32_WINNT >= 0x0500
        HANDLE_MSG(hwnd, WM_XBUTTONDOWN, MD_OnXButtonDown);
        HANDLE_MSG(hwnd, WM_XBUTTONUP, MD_OnXButtonUp);
        HANDLE_MSG(hwnd, WM_XBUTTONDBLCLK, MD_OnXButtonDown);
#endif
        HANDLE_MSG(hwnd, WM_PARENTNOTIFY, MD_OnParentNotify);
        HANDLE_MSG(hwnd, WM_ENTERMENULOOP, MD_OnEnterMenuLoop);
        HANDLE_MSG(hwnd, WM_EXITMENULOOP, MD_OnExitMenuLoop);
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_NEXTMENU, MD_OnNextMenu);
        HANDLE_MSG(hwnd, WM_SIZING, MD_OnSizing);
        HANDLE_MSG(hwnd, WM_CAPTURECHANGED, MD_OnCaptureChanged);
        HANDLE_MSG(hwnd, WM_MOVING, MD_OnMoving);
        HANDLE_MSG(hwnd, WM_POWERBROADCAST, MD_OnPowerBroadcast);
        HANDLE_MSG(hwnd, WM_DEVICECHANGE, MD_OnDeviceChange);
#endif
        HANDLE_MSG(hwnd, WM_MDICREATE, MD_MDICreate);
        HANDLE_MSG(hwnd, WM_MDIDESTROY, MD_MDIDestroy);
        HANDLE_MSG(hwnd, WM_MDIACTIVATE, MD_MDIActivate);
        HANDLE_MSG(hwnd, WM_MDIRESTORE, MD_MDIRestore);
        HANDLE_MSG(hwnd, WM_MDINEXT, MD_MDINext);
        HANDLE_MSG(hwnd, WM_MDIMAXIMIZE, MD_MDIMaximize);
        HANDLE_MSG(hwnd, WM_MDITILE, MD_MDITile);
        HANDLE_MSG(hwnd, WM_MDICASCADE, MD_MDICascade);
        HANDLE_MSG(hwnd, WM_MDIICONARRANGE, MD_MDIIconArrange);
        HANDLE_MSG(hwnd, WM_MDIGETACTIVE, MD_MDIGetActive);
        HANDLE_MSG(hwnd, WM_MDISETMENU, MD_MDISetMenu);
        HANDLE_MSG(hwnd, WM_ENTERSIZEMOVE, MD_OnEnterSizeMove);
        HANDLE_MSG(hwnd, WM_EXITSIZEMOVE, MD_OnExitSizeMove);
        HANDLE_MSG(hwnd, WM_DROPFILES, MD_OnDropFiles);
        HANDLE_MSG(hwnd, WM_MDIREFRESHMENU, MD_MDIRefreshMenu);
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_IME_SETCONTEXT, MD_OnImeSetContext);
        HANDLE_MSG(hwnd, WM_IME_NOTIFY, MD_OnImeNotify);
        HANDLE_MSG(hwnd, WM_IME_CONTROL, MD_OnImeControl);
        HANDLE_MSG(hwnd, WM_IME_COMPOSITIONFULL, MD_OnImeCompositionFull);
        HANDLE_MSG(hwnd, WM_IME_SELECT, MD_OnImeSelect);
        HANDLE_MSG(hwnd, WM_IME_CHAR, MD_OnImeChar);
#endif
#if WINVER >= 0x0500
        HANDLE_MSG(hwnd, WM_IME_REQUEST, MD_OnImeRequest);
#endif
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_IME_KEYDOWN, MD_OnImeKey);
        HANDLE_MSG(hwnd, WM_IME_KEYUP, MD_OnImeKey);
#endif
#if (_WIN32_WINNT >= 0x0400) || (WINVER >= 0x0500)
        HANDLE_MSG(hwnd, WM_MOUSEHOVER, MD_OnMouseHover);
        HANDLE_MSG(hwnd, WM_MOUSELEAVE, MD_OnMouseLeave);
#endif
#if WINVER >= 0x0500
        HANDLE_MSG(hwnd, WM_NCMOUSEHOVER, MD_OnNCMouseHover);
        HANDLE_MSG(hwnd, WM_NCMOUSELEAVE, MD_OnNCMouseLeave);
#endif
        HANDLE_MSG(hwnd, WM_CUT, MD_OnCut);
        HANDLE_MSG(hwnd, WM_COPY, MD_OnCopy);
        HANDLE_MSG(hwnd, WM_PASTE, MD_OnPaste);
        HANDLE_MSG(hwnd, WM_CLEAR, MD_OnClear);
        HANDLE_MSG(hwnd, WM_UNDO, MD_OnUndo);
        HANDLE_MSG(hwnd, WM_RENDERFORMAT, MD_OnRenderFormat);
        HANDLE_MSG(hwnd, WM_RENDERALLFORMATS, MD_OnRenderAllFormats);
        HANDLE_MSG(hwnd, WM_DESTROYCLIPBOARD, MD_OnDestroyClipboard);
        HANDLE_MSG(hwnd, WM_DRAWCLIPBOARD, MD_OnDrawClipboard);
        HANDLE_MSG(hwnd, WM_PAINTCLIPBOARD, MD_OnPaintClipboard);
        HANDLE_MSG(hwnd, WM_VSCROLLCLIPBOARD, MD_OnVScrollClipboard);
        HANDLE_MSG(hwnd, WM_SIZECLIPBOARD, MD_OnSizeClipboard);
        HANDLE_MSG(hwnd, WM_ASKCBFORMATNAME, MD_OnAskCBFormatName);
        HANDLE_MSG(hwnd, WM_CHANGECBCHAIN, MD_OnChangeCBChain);
        HANDLE_MSG(hwnd, WM_HSCROLLCLIPBOARD, MD_OnHScrollClipboard);
        HANDLE_MSG(hwnd, WM_QUERYNEWPALETTE, MD_OnQueryNewPalette);
        HANDLE_MSG(hwnd, WM_PALETTEISCHANGING, MD_OnPaletteIsChanging);
        HANDLE_MSG(hwnd, WM_PALETTECHANGED, MD_OnPaletteChanged);
        HANDLE_MSG(hwnd, WM_HOTKEY, MD_OnHotKey);
#if WINVER >= 0x0400
        HANDLE_MSG(hwnd, WM_PRINT, MD_OnPrint);
        HANDLE_MSG(hwnd, WM_PRINTCLIENT, MD_OnPrintClient);
#endif
#if _WIN32_WINNT >= 0x0500
        HANDLE_MSG(hwnd, WM_APPCOMMAND, MD_OnAppCommand);
#endif
        default:
        {
            if (WM_USER <= uMsg && uMsg <= 0x7FFF)
            {
                return MD_OnUser(hwnd, uMsg, wParam, lParam);
            }
            if (WM_APP <= uMsg && uMsg <= 0xBFFF)
            {
                return MD_OnApp(hwnd, uMsg, wParam, lParam);
            }
            return MD_OnUnknown(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

#endif
