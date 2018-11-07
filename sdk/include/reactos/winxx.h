/*
 * PROJECT:     ReactOS header files
 * LICENSE:     CC-BY-4.0 (https://spdx.org/licenses/CC-BY-4.0.html)
 * PURPOSE:     An unofficial extension of <windowsx.h>
 * COPYRIGHT:   Copyright 2017-2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef _INC_WINXX
#define _INC_WINXX

#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>
#endif
#ifndef _INC_COMMCTRL
    #include <commctrl.h>
#endif

/* additional message crackers */

/* void Cls_OnNull(HWND hwnd) */
#ifndef HANDLE_WM_NULL
#define HANDLE_WM_NULL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_NULL(hwnd, compactRatio, fn) \
    (void)((fn)((hwnd), WM_NULL, 0, 0L), 0L)
#endif

/* INT Cls_OnSetHotKey(HWND hwnd, INT nCode, UINT nOptions) */
#ifndef HANDLE_WM_SETHOTKEY
#define HANDLE_WM_SETHOTKEY(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT)(fn)((hwnd), (INT)LOWORD(wParam), (UINT)HIWORD(wParam))
#define FORWARD_WM_SETHOTKEY(hwnd, nCode, nOptions, fn) \
    (INT)(fn)((hwnd), WM_SETHOTKEY, MAKEWPARAM((nCode), (nOptions)))
#endif

/* INT Cls_OnGetHotKey(HWND hwnd) */
#ifndef HANDLE_WM_GETHOTKEY
#define HANDLE_WM_GETHOTKEY(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT)(fn)((hwnd))
#define FORWARD_WM_GETHOTKEY(hwnd, fn) \
    (INT)(fn)((hwnd), WM_GETHOTKEY, 0, 0L)
#endif

/* void Cls_OnPaintIcon(HWND hwnd) */
#ifndef HANDLE_WM_PAINTICON
#define HANDLE_WM_PAINTICON(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_PAINTICON(hwnd, fn) \
    (void)((fn)((hwnd), WM_PAINTICON, 0, 0L), 0L)
#endif

/* LRESULT Cls_OnGetObject(HWND hwnd, WPARAM wParam, DWORD dwObjId) */
#ifndef HANDLE_WM_GETOBJECT
#define HANDLE_WM_GETOBJECT(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (WPARAM)(wParam), (DWORD)(lParam))
#define FORWARD_WM_GETOBJECT(hwnd, wParam, dwObjId, fn) \
    (fn)((hwnd), WM_GETOBJECT, (WPARAM)(wParam), (LPARAM)(DWORD)(dwObjId))
#endif

/* void Cls_OnCancelJournal(HWND hwnd) */
#ifndef HANDLE_WM_CANCELJOURNAL
#define HANDLE_WM_CANCELJOURNAL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_CANCELJOURNAL(hwnd, fn) \
    (void)((fn)((hwnd), WM_PAINTICON, 0, 0L), 0L)
#endif

/* void Cls_OnInputLangChangeRequest(HWND hwnd, BOOL bFlag, HKL hKL) */
#ifndef HANDLE_WM_INPUTLANGCHANGEREQUEST
#define HANDLE_WM_INPUTLANGCHANGEREQUEST(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_INPUTLANGCHANGEREQUEST(hwnd, bFlag, hKL, fn) \
    (void)((fn)((hwnd), WM_INPUTLANGCHANGEREQUEST, (WPARAM)(BOOL)(bFlag), \
           (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnInputLangChange(HWND hwnd, DWORD dwCharSet, HKL hKL) */
#ifndef HANDLE_WM_INPUTLANGCHANGE
#define HANDLE_WM_INPUTLANGCHANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (DWORD)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_INPUTLANGCHANGE(hwnd, dwCharSet, hKL, fn) \
    (void)((fn)((hwnd), WM_INPUTLANGCHANGE, (WPARAM)(DWORD)(dwCharSet), \
           (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnTCard(HWND hwnd, UINT idAction, DWORD dwActionData) */
#ifndef HANDLE_WM_TCARD
#define HANDLE_WM_TCARD(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (DWORD)(lParam)), 0L)
#define FORWARD_WM_TCARD(hwnd, idAction, dwActionData, fn) \
    (void)((fn)((hwnd), WM_TCARD, (DWORD)(wParam), (DWORD)(lParam)), 0L)
#endif

/* void Cls_OnHelp(HWND hwnd, LPHELPINFO lpHelpInfo) */
#ifndef HANDLE_WM_HELP
#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPHELPINFO)(lParam)), 0L)
#define FORWARD_WM_HELP(hwnd, lpHelpInfo, fn) \
    (void)((fn)((hwnd), WM_HELP, 0, (LPARAM)(LPHELPINFO)(lpHelpInfo)), 0L)
#endif

/* void Cls_OnUserChanged(HWND hwnd) */
#ifndef HANDLE_WM_USERCHANGED
#define HANDLE_WM_USERCHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_USERCHANGED(hwnd, fn) \
    (void)((fn)((hwnd), WM_USERCHANGED, 0, 0L), 0L)
#endif

/* INT Cls_OnNotifyFormat(HWND hwnd, HWND hwndTarget, INT nCommand) */
#ifndef HANDLE_WM_NOTIFYFORMAT
#define HANDLE_WM_NOTIFYFORMAT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT)(fn)((hwnd), (HWND)(wParam), (INT)(lParam))
#define FORWARD_WM_NOTIFYFORMAT(hwnd, hwndTarget, nCommand, fn) \
    (INT)(fn)((hwnd), WM_NOTIFYFORMAT, (WPARAM)(HWND)(hwndTarget), \
              (LPARAM)(INT)(nCommand))
#endif

/* void Cls_OnStyleChanging(HWND hwnd, UINT nStyleType, LPSTYLESTRUCT lpStyleStruct) */
#ifndef HANDLE_WM_STYLECHANGING
#define HANDLE_WM_STYLECHANGING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPSTYLESTRUCT)(lParam)), 0L)
#define FORWARD_WM_STYLECHANGING(hwnd, nStyleType, lpStyleStruct, fn) \
    (void)((fn)((hwnd), WM_STYLECHANGING, (WPARAM)(UINT)(nStyleType), \
                (LPARAM)(LPSTYLESTRUCT)(lpStyleStruct)), 0L)
#endif

/* void Cls_OnStyleChanged(HWND hwnd, UINT nStyleType, const STYLESTRUCT *lpStyleStruct) */
#ifndef HANDLE_WM_STYLECHANGED
#define HANDLE_WM_STYLECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (const STYLESTRUCT *)(lParam)), 0L)
#define FORWARD_WM_STYLECHANGED(hwnd, nStyleType, lpStyleStruct, fn) \
    (void)((fn)((hwnd), WM_STYLECHANGED, (WPARAM)(UINT)(nStyleType), \
                (LPARAM)(const STYLESTRUCT *)(lpStyleStruct)), 0L)
#endif

/* HICON Cls_OnGetIcon(HWND hwnd, UINT nType, LPARAM dpi) */
#ifndef HANDLE_WM_GETICON
#define HANDLE_WM_GETICON(hwnd, wParam, lParam, fn) \
    (LRESULT)(HICON)(fn)((hwnd), (INT)(wParam), (LPARAM)(lParam))
#define FORWARD_WM_GETICON(hwnd, nType, dpi, fn) \
    (HICON)(fn)((hwnd), WM_GETICON, (WPARAM)(UINT)(nType), (LPARAM)(dpi))
#endif

/* HICON Cls_OnSetIcon(HWND hwnd, UINT nType, HICON hIcon) */
#ifndef HANDLE_WM_SETICON
#define HANDLE_WM_SETICON(hwnd, wParam, lParam, fn) \
    (LRESULT)(HICON)(fn)((hwnd), (INT)(wParam), (HICON)(lParam))
#define FORWARD_WM_SETICON(hwnd, nType, hIcon, fn) \
    (HICON)(fn)((hwnd), WM_SETICON, (WPARAM)(UINT)(nType), (LPARAM)(HICON)(hIcon))
#endif

/* void Cls_OnSyncPaint(HWND hwnd) */
#ifndef HANDLE_WM_SYNCPAINT
#define HANDLE_WM_SYNCPAINT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_SYNCPAINT(hwnd, fn) \
    (void)((fn)((hwnd), WM_SYNCPAINT, 0, 0L), 0L)
#endif

/* void Cls_OnNCXButtonDown(HWND hwnd, BOOL fDoubleClick, UINT nHitTest,
                            WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCXBUTTONDOWN
#define HANDLE_WM_NCXBUTTONDOWN(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), FALSE, GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define HANDLE_WM_NCXBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), TRUE, GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_NCXBUTTONDOWN(hwnd, fDoubleClick, nHitTest, fwButton, xPos, yPos, fn) \
    (void)((fn)((hwnd), ((fDoubleClick) ? WM_NCXBUTTONDBLCLK : WM_NCXBUTTONDOWN), \
                MAKEWPARAM((nHitTest), (fwButton)), MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnNCXButtonUp(HWND hwnd, UINT nHitTest, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCXBUTTONUP
#define HANDLE_WM_NCXBUTTONUP(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_NCXBUTTONUP(hwnd, nHitTest, fwButton, xPos, yPos, fn) \
    (void)((fn)((hwnd), WM_NCXBUTTONUP, MAKEWPARAM((nHitTest), (fwButton)), \
                MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnImeStartComposition(HWND hwnd) */
#ifndef HANDLE_WM_IME_STARTCOMPOSITION
#define HANDLE_WM_IME_STARTCOMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_IME_STARTCOMPOSITION(hwnd, fn) \
    (void)((fn)((hwnd), WM_IME_STARTCOMPOSITION, 0, 0L), 0L)
#endif

/* void Cls_OnImeEndComposition(HWND hwnd) */
#ifndef HANDLE_WM_IME_ENDCOMPOSITION
#define HANDLE_WM_IME_ENDCOMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_IME_ENDCOMPOSITION(hwnd, fn) \
    (void)((fn)((hwnd), WM_IME_ENDCOMPOSITION, 0, 0L), 0L)
#endif

/* void Cls_OnImeComposition(HWND hwnd, WORD wChar, DWORD lAttribute) */
#ifndef HANDLE_WM_IME_COMPOSITION
#define HANDLE_WM_IME_COMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (WORD)(wParam), (DWORD)(lParam)), 0L)
#define FORWARD_WM_IME_COMPOSITION(hwnd, wChar, lAttribute, fn) \
    (void)((fn)((hwnd), WM_IME_COMPOSITION, (WPARAM)(WORD)(wChar), (DWORD)(lAttribute)), 0L)
#endif

/* void Cls_OnMenuRButtonUp(HWND hwnd, UINT nPos, HMENU hMenu) */
#ifndef HANDLE_WM_MENURBUTTONUP
#define HANDLE_WM_MENURBUTTONUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (HMENU)(lParam)), 0L)
#define FORWARD_WM_MENURBUTTONUP(hwnd, nPos, hMenu, fn) \
    (void)((fn)((hwnd), WM_MENURBUTTONUP, (WPARAM)(UINT)(nPos), (LPARAM)(HMENU)(hMenu)), 0L)
#endif

/* UINT Cls_OnMenuDrag(HWND hwnd, UINT nPos, HMENU hMenu) */
#ifndef HANDLE_WM_MENUDRAG
#define HANDLE_WM_MENUDRAG(hwnd, wParam, lParam, fn) \
    (LRESULT)(UINT)(fn)((hwnd), (UINT)(wParam), (HMENU)(lParam))
#define FORWARD_WM_MENUDRAG(hwnd, nPos, hMenu, fn) \
    (UINT)(fn)((hwnd), WM_MENUDRAG, (WPARAM)(UINT)(nPos), (LPARAM)(HMENU)(hMenu))
#endif

/* UINT Cls_OnMenuGetObject(HWND hwnd, MENUGETOBJECTINFO *pmgoi) */
#ifndef HANDLE_WM_MENUGETOBJECT
#define HANDLE_WM_MENUGETOBJECT(hwnd, wParam, lParam, fn) \
    (LRESULT)(UINT)(fn)((hwnd), (MENUGETOBJECTINFO *)(lParam))
#define FORWARD_WM_MENUGETOBJECT(hwnd, pmgoi, fn) \
    (UINT)(fn)((hwnd), WM_MENUGETOBJECT, 0, (LPARAM)(MENUGETOBJECTINFO *)(pmgoi))
#endif

/* void Cls_OnUninitMenuPopup(HWND hwnd, HMENU hMenu, UINT nFlags) */
#ifndef HANDLE_WM_UNINITMENUPOPUP
#define HANDLE_WM_UNINITMENUPOPUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HMENU)(wParam), (UINT)(lParam)), 0L)
#define FORWARD_WM_UNINITMENUPOPUP(hwnd, hMenu, nFlags, fn) \
    (void)((fn)((hwnd), WM_UNINITMENUPOPUP, (WPARAM)(HMENU)(hMenu), \
                (LPARAM)(UINT)(nFlags)), 0L)
#endif

/* void Cls_OnMenuCommand(HWND hwnd, UINT nPos, HMENU hMenu) */
#ifndef HANDLE_WM_MENUCOMMAND
#define HANDLE_WM_MENUCOMMAND(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (HMENU)(lParam)), 0L)
#define FORWARD_WM_MENUCOMMAND(hwnd, nPos, hMenu, fn) \
    (void)((fn)((hwnd), WM_MENURBUTTONUP, (WPARAM)(UINT)(nPos), \
           (LPARAM)(HMENU)(hMenu)), 0L)
#endif

/* void Cls_OnChangeUIState(HWND hwnd, UINT nAction, UINT nUIElement) */
#ifndef HANDLE_WM_CHANGEUISTATE
#define HANDLE_WM_CHANGEUISTATE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)LOWORD(wParam), (UINT)HIWORD(wParam)), 0L)
#define FORWARD_WM_CHANGEUISTATE(hwnd, nAction, nUIElement, fn) \
    (void)((fn)((hwnd), WM_CHANGEUISTATE, MAKEWPARAM((nAction), (nUIElement)), 0L), 0L)
#endif

/* void Cls_OnUpdateUIState(HWND hwnd, UINT nAction, UINT nUIElement) */
#ifndef HANDLE_WM_UPDATEUISTATE
#define HANDLE_WM_UPDATEUISTATE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)LOWORD(wParam), (UINT)HIWORD(wParam)), 0L)
#define FORWARD_WM_UPDATEUISTATE(hwnd, nAction, nUIElement, fn) \
    (void)((fn)((hwnd), WM_UPDATEUISTATE, MAKEWPARAM((nAction), (nUIElement)), 0L), 0L)
#endif

/* UINT Cls_OnQueryUIState(HWND hwnd) */
#ifndef HANDLE_WM_QUERYUISTATE
#define HANDLE_WM_QUERYUISTATE(hwnd, wParam, lParam, fn) \
    (LRESULT)(UINT)(fn)((hwnd))
#define FORWARD_WM_QUERYUISTATE(hwnd, fn) \
    (UINT)(fn)((hwnd), WM_QUERYUISTATE, 0, 0L)
#endif

/* void Cls_OnXButtonDown(HWND hwnd, BOOL fDoubleClick, WORD fwKeys,
                          WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_XBUTTONDOWN
#define HANDLE_WM_XBUTTONDOWN(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), FALSE, GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define HANDLE_WM_XBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), TRUE, GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_XBUTTONDOWN(hwnd, fDoubleClick, fwKeys, fwButton, xPos, yPos, fn) \
    (void)((fn)((hwnd), ((fDoubleClick) ? WM_XBUTTONDBLCLK : WM_XBUTTONDOWN), \
                MAKEWPARAM((fwKeys), (fwButton)), MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnXButtonUp(HWND hwnd, WORD fwKeys, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_XBUTTONUP
#define HANDLE_WM_XBUTTONUP(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_XBUTTONUP(hwnd, fDoubleClick, fwKeys, fwButton, xPos, yPos, fn) \
    (void)((fn)((hwnd), WM_XBUTTONUP, MAKEWPARAM((fwKeys), (fwButton)), \
                MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnEnterMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu) */
#ifndef HANDLE_WM_ENTERMENULOOP
#define HANDLE_WM_ENTERMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_ENTERMENULOOP(hwnd, bIsTrackPopupMenu, fn) \
    (void)((fn)((hwnd), WM_ENTERMENULOOP, (BOOL)(bIsTrackPopupMenu), 0L), 0L)
#endif

/* void Cls_OnExitMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu) */
#ifndef HANDLE_WM_EXITMENULOOP
#define HANDLE_WM_EXITMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_EXITMENULOOP(hwnd, bIsTrackPopupMenu, fn) \
    (void)((fn)((hwnd), WM_EXITMENULOOP, (BOOL)(bIsTrackPopupMenu), 0L), 0L)
#endif

/* void Cls_OnNextMenu(HWND hwnd, INT nCode, LPMDINEXTMENU lpMDINextMenu) */
#ifndef HANDLE_WM_NEXTMENU
#define HANDLE_WM_NEXTMENU(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), (LPMDINEXTMENU)(lParam)), 0L)
#define FORWARD_WM_NEXTMENU(hwnd, nCode, lpMDINextMenu, fn) \
    (void)((fn)((hwnd), WM_NEXTMENU, (WPARAM)(INT)(nCode), \
           (LPARAM)(LPMDINEXTMENU)(lpMDINextMenu)), 0L)
#endif

/* void Cls_OnSizing(HWND hwnd, UINT nSide, LPRECT lpRect) */
#ifndef HANDLE_WM_SIZING
#define HANDLE_WM_SIZING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPRECT)(lParam)), 0L)
#define FORWARD_WM_SIZING(hwnd, nSide, lpRect, fn) \
    (void)((fn)((hwnd), WM_SIZING, (WPARAM)(UINT)(nSide), (LPARAM)(LPRECT)(lpRect)), 0L)
#endif

/* void Cls_OnCaptureChanged(HWND hwnd, HWND hwndNewCapture) */
#ifndef HANDLE_WM_CAPTURECHANGED
#define HANDLE_WM_CAPTURECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam)), 0L)
#define FORWARD_WM_CAPTURECHANGED(hwnd, hwndNewCapture, fn) \
    (void)((fn)((hwnd), WM_CAPTURECHANGED, 0, (LPARAM)(HWND)(hwndNewCapture)), 0L)
#endif

/* void Cls_OnMoving(HWND hwnd, UINT nSide, LPRECT lpRect) */
#ifndef HANDLE_WM_MOVING
#define HANDLE_WM_MOVING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPRECT)(lParam)), 0L)
#define FORWARD_WM_MOVING(hwnd, nSide, lpRect, fn) \
    (void)((fn)((hwnd), WM_MOVING, (WPARAM)(UINT)(nSide), (LPARAM)(LPRECT)(lpRect)), 0L)
#endif

/* LRESULT Cls_OnPowerBroadcast(HWND hwnd, UINT nPowerEvent, UINT nEventData) */
#ifndef HANDLE_WM_POWERBROADCAST
#define HANDLE_WM_POWERBROADCAST(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (UINT)(wParam), (UINT)(lParam))
#define FORWARD_WM_POWERBROADCAST(hwnd, nPowerEvent, nEventData, fn) \
    (LRESULT)(fn)((hwnd), WM_POWERBROADCAST, (WPARAM)(UINT)(nPowerEvent), \
                  (LPARAM)(UINT)(nEventData))
#endif

/* HMENU Cls_MDIRefreshMenu(HWND hwnd) */
#ifndef HANDLE_WM_MDIREFRESHMENU
#define HANDLE_WM_MDIREFRESHMENU(hwnd, wParam, lParam, fn) \
    (LRESULT)(HMENU)(fn)((hwnd))
#define FORWARD_WM_MDIREFRESHMENU(hwnd, fn) \
    (HMENU)(fn)((hwnd), WM_MDIREFRESHMENU, 0, 0L)
#endif

/* BOOL Cls_OnImeSetContext(HWND hwnd, BOOL fActive, DWORD dwShow) */
#ifndef HANDLE_WM_IME_SETCONTEXT
#define HANDLE_WM_IME_SETCONTEXT(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (BOOL)(wParam), (DWORD)(lParam))
#define FORWARD_WM_IME_SETCONTEXT(hwnd, fActive, dwShow, fn) \
    (BOOL)(fn)((hwnd), WM_IME_SETCONTEXT, (WPARAM)(BOOL)(fActive), (LPARAM)(DWORD)(dwShow))
#endif

/* LRESULT Cls_OnImeNotify(HWND hwnd, WPARAM wSubMessage, LPARAM lParam) */
#ifndef HANDLE_WM_IME_NOTIFY
#define HANDLE_WM_IME_NOTIFY(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (WPARAM)(wParam), (LPARAM)(lParam))
#define FORWARD_WM_IME_NOTIFY(hwnd, wSubMessage, lParam, fn) \
    (LRESULT)(fn)((hwnd), WM_IME_NOTIFY, (WPARAM)(wSubMessage), (LPARAM)(lParam))
#endif

/* LRESULT Cls_OnImeControl(HWND hwnd, WPARAM wSubMessage, LPVOID lpData) */
#ifndef HANDLE_WM_IME_CONTROL
#define HANDLE_WM_IME_CONTROL(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (WPARAM)(wParam), (LPVOID)(lParam))
#define FORWARD_WM_IME_CONTROL(hwnd, wSubMessage, lpData, fn) \
    (LRESULT)(fn)((hwnd), WM_IME_CONTROL, (WPARAM)(wSubMessage), (LPARAM)(LPVOID)(lpData))
#endif

/* void Cls_OnImeCompositionFull(HWND hwnd) */
#ifndef HANDLE_WM_IME_COMPOSITIONFULL
#define HANDLE_WM_IME_COMPOSITIONFULL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_IME_COMPOSITIONFULL(hwnd, compactRatio, fn) \
    (void)((fn)((hwnd), WM_IME_COMPOSITIONFULL, 0, 0L), 0L)
#endif

/* void Cls_OnImeSelect(HWND hwnd, BOOL fSelect, HKL hKL) */
#ifndef HANDLE_WM_IME_SELECT
#define HANDLE_WM_IME_SELECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_IME_SELECT(hwnd, fSelect, hKL, fn) \
    (void)((fn)((hwnd), WM_IME_SELECT, (WPARAM)(BOOL)(fSelect), (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnImeChar(HWND hwnd, WORD wCharCode, LONG lKeyData) */
#ifndef HANDLE_WM_IME_CHAR
#define HANDLE_WM_IME_CHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (WORD)(wParam), (LONG)(lParam)), 0L)
#define FORWARD_WM_IME_CHAR(hwnd, wCharCode, lKeyData, fn) \
    (void)((fn)((hwnd), WM_IME_CHAR, (WPARAM)(WORD)(wCharCode), (LPARAM)(LONG)(lKeyData)), 0L)
#endif

/* LRESULT Cls_OnImeRequest(HWND hwnd, WPARAM wParam, LPARAM lParam) */
#ifndef HANDLE_WM_IME_REQUEST
#define HANDLE_WM_IME_REQUEST(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (WPARAM)(wParam), (LPARAM)(lParam))
#define FORWARD_WM_IME_REQUEST(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), WM_IME_REQUEST, (WPARAM)(wParam), (LPARAM)(lParam))
#endif

/* void Cls_OnImeKey(HWND hwnd, BOOL fDown, UINT nVirtKey, LONG lKeyData) */
#ifndef HANDLE_WM_IME_KEYDOWN
#define HANDLE_WM_IME_KEYDOWN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), TRUE, (WORD)(wParam), (LONG)(lParam)), 0L)
#define FORWARD_WM_IME_KEYDOWN(hwnd, fDown, nVirtKey, lKeyData, fn) \
    (void)((fn)((hwnd), ((BOOL)(fDown) ? WM_IME_KEYDOWN : WM_IME_KEYUP), \
                (WPARAM)(UINT)(nVirtKey), (LPARAM)(LONG)(lKeyData)), 0L)
#define HANDLE_WM_IME_KEYUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), FALSE, (WORD)(wParam), (LONG)(lParam)), 0L)
#define FORWARD_WM_IME_KEYUP(hwnd, fDown, nVirtKey, lKeyData, fn) \
    (void)((fn)((hwnd), ((BOOL)(fDown) ? WM_IME_KEYDOWN : WM_IME_KEYUP), \
                (WPARAM)(UINT)(nVirtKey), (LPARAM)(LONG)(lKeyData)), 0L)
#endif

/* void Cls_OnMouseHover(HWND hwnd, UINT nFlags, INT xPos, INT yPos) */
#ifndef HANDLE_WM_MOUSEHOVER
#define HANDLE_WM_MOUSEHOVER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0L)
#define FORWARD_WM_MOUSEHOVER(hwnd, nFlags, xPos, yPos, fn) \
    (void)((fn)((hwnd), WM_MOUSEHOVER, (WPARAM)(UINT)(nFlags), \
                MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnMouseLeave(HWND hwnd) */
#ifndef HANDLE_WM_MOUSELEAVE
#define HANDLE_WM_MOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_MOUSELEAVE(hwnd, fn) \
    (void)((fn)((hwnd), WM_MOUSELEAVE, 0, 0L), 0L)
#endif

/* void Cls_OnNCMouseHover(HWND hwnd, UINT nHitTest, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCMOUSEHOVER
#define HANDLE_WM_NCMOUSEHOVER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0L)
#define FORWARD_WM_NCMOUSEHOVER(hwnd, nHitTest, xPos, yPos, fn) \
    (void)((fn)((hwnd), WM_NCMOUSEHOVER, (WPARAM)(UINT)(nHitTest), \
           MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnNCMouseLeave(HWND hwnd) */
#ifndef HANDLE_WM_NCMOUSELEAVE
#define HANDLE_WM_NCMOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_NCMOUSELEAVE(hwnd, fn) \
    (void)((fn)((hwnd), WM_NCMOUSELEAVE, 0, 0L), 0L)
#endif

/* void Cls_OnPrint(HWND hwnd, HDC hDC, UINT uFlags) */
#ifndef HANDLE_WM_PRINT
#define HANDLE_WM_PRINT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDC)(wParam), (UINT)(lParam)), 0L)
#define FORWARD_WM_PRINT(hwnd, hDC, uFlags, fn) \
    (void)((fn)((hwnd), WM_PRINT, (WPARAM)(HDC)(hDC), (LPARAM)(uFlags)), 0L)
#endif

/* void Cls_OnPrintClient(HWND hwnd, HDC hDC, UINT uFlags) */
#ifndef HANDLE_WM_PRINTCLIENT
#define HANDLE_WM_PRINTCLIENT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDC)(wParam), (UINT)(lParam)), 0L)
#define FORWARD_WM_PRINTCLIENT(hwnd, hDC, uFlags, fn) \
    (void)((fn)((hwnd), WM_PRINTCLIENT, (WPARAM)(HDC)(hDC), (LPARAM)(uFlags)), 0L)
#endif

/* BOOL Cls_OnAppCommand(HWND hwnd, HWND hwndTarget, UINT cmd, UINT nDevice, UINT nKey) */
#ifndef HANDLE_WM_APPCOMMAND
#define HANDLE_WM_APPCOMMAND(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (HWND)(wParam), GET_APPCOMMAND_LPARAM(lParam), \
                        GET_DEVICE_LPARAM(lParam), GET_KEYSTATE_LPARAM(lParam))
#define FORWARD_WM_APPCOMMAND(hwnd, hwndTarget, cmd, nDevice, nKey, fn) \
    (BOOL)(fn)((hwnd), WM_APPCOMMAND, (WPARAM)(hwndTarget), \
               MAKELPARAM((nKey), ((WORD)(cmd)) | (UINT)(nDevice)))
#endif

/* void Cls_OnEnterSizeMove(HWND hwnd) */
#ifndef HANDLE_WM_ENTERSIZEMOVE
#define HANDLE_WM_ENTERSIZEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_ENTERSIZEMOVE(hwnd, fn) \
    (void)((fn)((hwnd), WM_ENTERSIZEMOVE, 0, 0L), 0L)
#endif

/* void Cls_OnExitSizeMove(HWND hwnd) */
#ifndef HANDLE_WM_EXITSIZEMOVE
#define HANDLE_WM_EXITSIZEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_EXITSIZEMOVE(hwnd, fn) \
    (void)((fn)((hwnd), WM_EXITSIZEMOVE, 0, 0L), 0L)
#endif

/* HICON Cls_OnQueryDragIcon(HWND hwnd) */
#undef HANDLE_WM_QUERYDRAGICON
#define HANDLE_WM_QUERYDRAGICON(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(UINT_PTR)(fn)(hwnd)
#undef FORWARD_WM_QUERYDRAGICON
#define FORWARD_WM_QUERYDRAGICON(hwnd, fn) \
    (HICON)(UINT_PTR)(UINT)(DWORD)(fn)((hwnd), WM_QUERYDRAGICON, 0L, 0L)

/* HWND Cls_MDICreate(HWND hwnd, const LPMDICREATESTRUCT lpmcs) */
#undef HANDLE_WM_MDICREATE
#define HANDLE_WM_MDICREATE(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(UINT_PTR)(fn)((hwnd), (LPMDICREATESTRUCT)(lParam))
#undef FORWARD_WM_MDICREATE
#define FORWARD_WM_MDICREATE(hwnd, lpmcs, fn) \
    (HWND)(UINT_PTR)(fn)((hwnd), WM_MDICREATE, 0L, (LPARAM)(LPMDICREATESTRUCT)(lpmcs))

/* BOOL Cls_OnNCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized) */
#undef HANDLE_WM_NCACTIVATE
#define HANDLE_WM_NCACTIVATE(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (BOOL)(wParam), 0L, 0L)
#undef FORWARD_WM_NCACTIVATE
#define FORWARD_WM_NCACTIVATE(hwnd, fActive, hwndActDeact, fMinimized, fn) \
    (BOOL)(DWORD)(fn)((hwnd), WM_NCACTIVATE, (WPARAM)(BOOL)(fActive), 0L)

#endif
