/*
 * PROJECT:     ReactOS header files
 * LICENSE:     CC-BY-4.0 (https://spdx.org/licenses/CC-BY-4.0.html)
 * PURPOSE:     An unofficial extension of <windowsx.h>
 * COPYRIGHT:   Copyright 2017-2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef _INC_WINXX
#define _INC_WINXX      6   /* Version 6 */

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
#define FORWARD_WM_NULL(hwnd, fn) \
    ((fn)((hwnd), WM_NULL, 0, 0L), 0L)
#endif

/* INT Cls_OnSetHotKey(HWND hwnd, INT nCode, UINT nOptions) */
#ifndef HANDLE_WM_SETHOTKEY
#define HANDLE_WM_SETHOTKEY(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)LOWORD(wParam), (UINT)HIWORD(wParam))
#define FORWARD_WM_SETHOTKEY(hwnd, nCode, nOptions, fn) \
    (INT)(fn)((hwnd), WM_SETHOTKEY, MAKEWPARAM((nCode), (nOptions)))
#endif

/* INT Cls_OnGetHotKey(HWND hwnd) */
#ifndef HANDLE_WM_GETHOTKEY
#define HANDLE_WM_GETHOTKEY(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))
#define FORWARD_WM_GETHOTKEY(hwnd, fn) \
    (INT)(fn)((hwnd), WM_GETHOTKEY, 0, 0L)
#endif

/* void Cls_OnPaintIcon(HWND hwnd) */
#ifndef HANDLE_WM_PAINTICON
#define HANDLE_WM_PAINTICON(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_PAINTICON(hwnd, fn) \
    ((fn)((hwnd), WM_PAINTICON, 0, 0L), 0L)
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
    ((fn)((hwnd), WM_PAINTICON, 0, 0L), 0L)
#endif

/* void Cls_OnInputLangChangeRequest(HWND hwnd, BOOL bFlag, HKL hKL) */
#ifndef HANDLE_WM_INPUTLANGCHANGEREQUEST
#define HANDLE_WM_INPUTLANGCHANGEREQUEST(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_INPUTLANGCHANGEREQUEST(hwnd, bFlag, hKL, fn) \
    ((fn)((hwnd), WM_INPUTLANGCHANGEREQUEST, (WPARAM)(BOOL)(bFlag), \
          (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnInputLangChange(HWND hwnd, DWORD dwCharSet, HKL hKL) */
#ifndef HANDLE_WM_INPUTLANGCHANGE
#define HANDLE_WM_INPUTLANGCHANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (DWORD)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_INPUTLANGCHANGE(hwnd, dwCharSet, hKL, fn) \
    ((fn)((hwnd), WM_INPUTLANGCHANGE, (WPARAM)(DWORD)(dwCharSet), \
          (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnTCard(HWND hwnd, UINT idAction, DWORD dwActionData) */
#ifndef HANDLE_WM_TCARD
#define HANDLE_WM_TCARD(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (DWORD)(lParam)), 0L)
#define FORWARD_WM_TCARD(hwnd, idAction, dwActionData, fn) \
    ((fn)((hwnd), WM_TCARD, (DWORD)(wParam), (DWORD)(lParam)), 0L)
#endif

/* void Cls_OnHelp(HWND hwnd, LPHELPINFO lpHelpInfo) */
#ifndef HANDLE_WM_HELP
#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPHELPINFO)(lParam)), 0L)
#define FORWARD_WM_HELP(hwnd, lpHelpInfo, fn) \
    ((fn)((hwnd), WM_HELP, 0, (LPARAM)(LPHELPINFO)(lpHelpInfo)), 0L)
#endif

/* void Cls_OnUserChanged(HWND hwnd) */
#ifndef HANDLE_WM_USERCHANGED
#define HANDLE_WM_USERCHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_USERCHANGED(hwnd, fn) \
    ((fn)((hwnd), WM_USERCHANGED, 0, 0L), 0L)
#endif

/* INT Cls_OnNotifyFormat(HWND hwnd, HWND hwndTarget, INT nCommand) */
#ifndef HANDLE_WM_NOTIFYFORMAT
#define HANDLE_WM_NOTIFYFORMAT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (HWND)(wParam), (INT)(lParam))
#define FORWARD_WM_NOTIFYFORMAT(hwnd, hwndTarget, nCommand, fn) \
    (INT)(fn)((hwnd), WM_NOTIFYFORMAT, (WPARAM)(HWND)(hwndTarget), \
              (LPARAM)(INT)(nCommand))
#endif

/* void Cls_OnStyleChanging(HWND hwnd, UINT nStyleType, LPSTYLESTRUCT lpStyleStruct) */
#ifndef HANDLE_WM_STYLECHANGING
#define HANDLE_WM_STYLECHANGING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPSTYLESTRUCT)(lParam)), 0L)
#define FORWARD_WM_STYLECHANGING(hwnd, nStyleType, lpStyleStruct, fn) \
    ((fn)((hwnd), WM_STYLECHANGING, (WPARAM)(UINT)(nStyleType), \
          (LPARAM)(LPSTYLESTRUCT)(lpStyleStruct)), 0L)
#endif

/* void Cls_OnStyleChanged(HWND hwnd, UINT nStyleType, const STYLESTRUCT *lpStyleStruct) */
#ifndef HANDLE_WM_STYLECHANGED
#define HANDLE_WM_STYLECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (const STYLESTRUCT *)(lParam)), 0L)
#define FORWARD_WM_STYLECHANGED(hwnd, nStyleType, lpStyleStruct, fn) \
    ((fn)((hwnd), WM_STYLECHANGED, (WPARAM)(UINT)(nStyleType), \
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
    ((fn)((hwnd), WM_SYNCPAINT, 0, 0L), 0L)
#endif

/* void Cls_OnNCXButtonDown(HWND hwnd, BOOL fDoubleClick, UINT nHitTest, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCXBUTTONDOWN
#define HANDLE_WM_NCXBUTTONDOWN(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), FALSE, GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define HANDLE_WM_NCXBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), TRUE, GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_NCXBUTTONDOWN(hwnd, fDoubleClick, nHitTest, fwButton, xPos, yPos, fn) \
    ((fn)((hwnd), ((fDoubleClick) ? WM_NCXBUTTONDBLCLK : WM_NCXBUTTONDOWN), \
          MAKEWPARAM((nHitTest), (fwButton)), MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnNCXButtonUp(HWND hwnd, UINT nHitTest, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCXBUTTONUP
#define HANDLE_WM_NCXBUTTONUP(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), GET_NCHITTEST_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_NCXBUTTONUP(hwnd, nHitTest, fwButton, xPos, yPos, fn) \
    ((fn)((hwnd), WM_NCXBUTTONUP, MAKEWPARAM((nHitTest), (fwButton)), \
          MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnImeStartComposition(HWND hwnd) */
#ifndef HANDLE_WM_IME_STARTCOMPOSITION
#define HANDLE_WM_IME_STARTCOMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_IME_STARTCOMPOSITION(hwnd, fn) \
    ((fn)((hwnd), WM_IME_STARTCOMPOSITION, 0, 0L), 0L)
#endif

/* void Cls_OnImeEndComposition(HWND hwnd) */
#ifndef HANDLE_WM_IME_ENDCOMPOSITION
#define HANDLE_WM_IME_ENDCOMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_IME_ENDCOMPOSITION(hwnd, fn) \
    ((fn)((hwnd), WM_IME_ENDCOMPOSITION, 0, 0L), 0L)
#endif

/* void Cls_OnImeComposition(HWND hwnd, WORD wChar, DWORD lAttribute) */
#ifndef HANDLE_WM_IME_COMPOSITION
#define HANDLE_WM_IME_COMPOSITION(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (WORD)(wParam), (DWORD)(lParam)), 0L)
#define FORWARD_WM_IME_COMPOSITION(hwnd, wChar, lAttribute, fn) \
    ((fn)((hwnd), WM_IME_COMPOSITION, (WPARAM)(WORD)(wChar), (DWORD)(lAttribute)), 0L)
#endif

/* void Cls_OnMenuRButtonUp(HWND hwnd, UINT nPos, HMENU hMenu) */
#ifndef HANDLE_WM_MENURBUTTONUP
#define HANDLE_WM_MENURBUTTONUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (HMENU)(lParam)), 0L)
#define FORWARD_WM_MENURBUTTONUP(hwnd, nPos, hMenu, fn) \
    ((fn)((hwnd), WM_MENURBUTTONUP, (WPARAM)(UINT)(nPos), (LPARAM)(HMENU)(hMenu)), 0L)
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
    ((fn)((hwnd), WM_UNINITMENUPOPUP, (WPARAM)(HMENU)(hMenu), \
          (LPARAM)(UINT)(nFlags)), 0L)
#endif

/* void Cls_OnMenuCommand(HWND hwnd, UINT nPos, HMENU hMenu) */
#ifndef HANDLE_WM_MENUCOMMAND
#define HANDLE_WM_MENUCOMMAND(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (HMENU)(lParam)), 0L)
#define FORWARD_WM_MENUCOMMAND(hwnd, nPos, hMenu, fn) \
    ((fn)((hwnd), WM_MENURBUTTONUP, (WPARAM)(UINT)(nPos), \
          (LPARAM)(HMENU)(hMenu)), 0L)
#endif

/* void Cls_OnChangeUIState(HWND hwnd, UINT nAction, UINT nUIElement) */
#ifndef HANDLE_WM_CHANGEUISTATE
#define HANDLE_WM_CHANGEUISTATE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)LOWORD(wParam), (UINT)HIWORD(wParam)), 0L)
#define FORWARD_WM_CHANGEUISTATE(hwnd, nAction, nUIElement, fn) \
    ((fn)((hwnd), WM_CHANGEUISTATE, MAKEWPARAM((nAction), (nUIElement)), 0L), 0L)
#endif

/* void Cls_OnUpdateUIState(HWND hwnd, UINT nAction, UINT nUIElement) */
#ifndef HANDLE_WM_UPDATEUISTATE
#define HANDLE_WM_UPDATEUISTATE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)LOWORD(wParam), (UINT)HIWORD(wParam)), 0L)
#define FORWARD_WM_UPDATEUISTATE(hwnd, nAction, nUIElement, fn) \
    ((fn)((hwnd), WM_UPDATEUISTATE, MAKEWPARAM((nAction), (nUIElement)), 0L), 0L)
#endif

/* UINT Cls_OnQueryUIState(HWND hwnd) */
#ifndef HANDLE_WM_QUERYUISTATE
#define HANDLE_WM_QUERYUISTATE(hwnd, wParam, lParam, fn) \
    (LRESULT)(UINT)(fn)((hwnd))
#define FORWARD_WM_QUERYUISTATE(hwnd, fn) \
    (UINT)(fn)((hwnd), WM_QUERYUISTATE, 0, 0L)
#endif

/* void Cls_OnXButtonDown(HWND hwnd, BOOL fDoubleClick, WORD fwKeys, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_XBUTTONDOWN
#define HANDLE_WM_XBUTTONDOWN(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), FALSE, GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define HANDLE_WM_XBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), TRUE, GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_XBUTTONDOWN(hwnd, fDoubleClick, fwKeys, fwButton, xPos, yPos, fn) \
    ((fn)((hwnd), ((fDoubleClick) ? WM_XBUTTONDBLCLK : WM_XBUTTONDOWN), \
          MAKEWPARAM((fwKeys), (fwButton)), MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnXButtonUp(HWND hwnd, WORD fwKeys, WORD fwButton, INT xPos, INT yPos) */
#ifndef HANDLE_WM_XBUTTONUP
#define HANDLE_WM_XBUTTONUP(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), GET_KEYSTATE_WPARAM(wParam), GET_XBUTTON_WPARAM(wParam), \
                   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), TRUE)
#define FORWARD_WM_XBUTTONUP(hwnd, fDoubleClick, fwKeys, fwButton, xPos, yPos, fn) \
    ((fn)((hwnd), WM_XBUTTONUP, MAKEWPARAM((fwKeys), (fwButton)), \
          MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnEnterMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu) */
#ifndef HANDLE_WM_ENTERMENULOOP
#define HANDLE_WM_ENTERMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_ENTERMENULOOP(hwnd, bIsTrackPopupMenu, fn) \
    ((fn)((hwnd), WM_ENTERMENULOOP, (BOOL)(bIsTrackPopupMenu), 0L), 0L)
#endif

/* void Cls_OnExitMenuLoop(HWND hwnd, BOOL bIsTrackPopupMenu) */
#ifndef HANDLE_WM_EXITMENULOOP
#define HANDLE_WM_EXITMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_EXITMENULOOP(hwnd, bIsTrackPopupMenu, fn) \
    ((fn)((hwnd), WM_EXITMENULOOP, (BOOL)(bIsTrackPopupMenu), 0L), 0L)
#endif

/* void Cls_OnNextMenu(HWND hwnd, INT nCode, LPMDINEXTMENU lpMDINextMenu) */
#ifndef HANDLE_WM_NEXTMENU
#define HANDLE_WM_NEXTMENU(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), (LPMDINEXTMENU)(lParam)), 0L)
#define FORWARD_WM_NEXTMENU(hwnd, nCode, lpMDINextMenu, fn) \
    ((fn)((hwnd), WM_NEXTMENU, (WPARAM)(INT)(nCode), \
          (LPARAM)(LPMDINEXTMENU)(lpMDINextMenu)), 0L)
#endif

/* void Cls_OnSizing(HWND hwnd, UINT nSide, LPRECT lpRect) */
#ifndef HANDLE_WM_SIZING
#define HANDLE_WM_SIZING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPRECT)(lParam)), 0L)
#define FORWARD_WM_SIZING(hwnd, nSide, lpRect, fn) \
    ((fn)((hwnd), WM_SIZING, (WPARAM)(UINT)(nSide), (LPARAM)(LPRECT)(lpRect)), 0L)
#endif

/* void Cls_OnCaptureChanged(HWND hwnd, HWND hwndNewCapture) */
#ifndef HANDLE_WM_CAPTURECHANGED
#define HANDLE_WM_CAPTURECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam)), 0L)
#define FORWARD_WM_CAPTURECHANGED(hwnd, hwndNewCapture, fn) \
    ((fn)((hwnd), WM_CAPTURECHANGED, 0, (LPARAM)(HWND)(hwndNewCapture)), 0L)
#endif

/* void Cls_OnMoving(HWND hwnd, UINT nSide, LPRECT lpRect) */
#ifndef HANDLE_WM_MOVING
#define HANDLE_WM_MOVING(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (LPRECT)(lParam)), 0L)
#define FORWARD_WM_MOVING(hwnd, nSide, lpRect, fn) \
    ((fn)((hwnd), WM_MOVING, (WPARAM)(UINT)(nSide), (LPARAM)(LPRECT)(lpRect)), 0L)
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
    ((fn)((hwnd), WM_IME_COMPOSITIONFULL, 0, 0L), 0L)
#endif

/* void Cls_OnImeSelect(HWND hwnd, BOOL fSelect, HKL hKL) */
#ifndef HANDLE_WM_IME_SELECT
#define HANDLE_WM_IME_SELECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (HKL)(lParam)), 0L)
#define FORWARD_WM_IME_SELECT(hwnd, fSelect, hKL, fn) \
    ((fn)((hwnd), WM_IME_SELECT, (WPARAM)(BOOL)(fSelect), (LPARAM)(HKL)(hKL)), 0L)
#endif

/* void Cls_OnImeChar(HWND hwnd, WORD wCharCode, LONG lKeyData) */
#ifndef HANDLE_WM_IME_CHAR
#define HANDLE_WM_IME_CHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (WORD)(wParam), (LONG)(lParam)), 0L)
#define FORWARD_WM_IME_CHAR(hwnd, wCharCode, lKeyData, fn) \
    ((fn)((hwnd), WM_IME_CHAR, (WPARAM)(WORD)(wCharCode), (LPARAM)(LONG)(lKeyData)), 0L)
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
    ((fn)((hwnd), ((BOOL)(fDown) ? WM_IME_KEYDOWN : WM_IME_KEYUP), \
          (WPARAM)(UINT)(nVirtKey), (LPARAM)(LONG)(lKeyData)), 0L)
#define HANDLE_WM_IME_KEYUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), FALSE, (WORD)(wParam), (LONG)(lParam)), 0L)
#define FORWARD_WM_IME_KEYUP(hwnd, fDown, nVirtKey, lKeyData, fn) \
    ((fn)((hwnd), ((BOOL)(fDown) ? WM_IME_KEYDOWN : WM_IME_KEYUP), \
          (WPARAM)(UINT)(nVirtKey), (LPARAM)(LONG)(lKeyData)), 0L)
#endif

/* void Cls_OnMouseHover(HWND hwnd, UINT nFlags, INT xPos, INT yPos) */
#ifndef HANDLE_WM_MOUSEHOVER
#define HANDLE_WM_MOUSEHOVER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0L)
#define FORWARD_WM_MOUSEHOVER(hwnd, nFlags, xPos, yPos, fn) \
    ((fn)((hwnd), WM_MOUSEHOVER, (WPARAM)(UINT)(nFlags), \
          MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnMouseLeave(HWND hwnd) */
#ifndef HANDLE_WM_MOUSELEAVE
#define HANDLE_WM_MOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_MOUSELEAVE(hwnd, fn) \
    ((fn)((hwnd), WM_MOUSELEAVE, 0, 0L), 0L)
#endif

/* void Cls_OnNCMouseHover(HWND hwnd, UINT nHitTest, INT xPos, INT yPos) */
#ifndef HANDLE_WM_NCMOUSEHOVER
#define HANDLE_WM_NCMOUSEHOVER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0L)
#define FORWARD_WM_NCMOUSEHOVER(hwnd, nHitTest, xPos, yPos, fn) \
    ((fn)((hwnd), WM_NCMOUSEHOVER, (WPARAM)(UINT)(nHitTest), \
          MAKELPARAM((xPos), (yPos))), 0L)
#endif

/* void Cls_OnNCMouseLeave(HWND hwnd) */
#ifndef HANDLE_WM_NCMOUSELEAVE
#define HANDLE_WM_NCMOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_NCMOUSELEAVE(hwnd, fn) \
    ((fn)((hwnd), WM_NCMOUSELEAVE, 0, 0L), 0L)
#endif

/* void Cls_OnPrint(HWND hwnd, HDC hDC, UINT uFlags) */
#ifndef HANDLE_WM_PRINT
#define HANDLE_WM_PRINT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDC)(wParam), (UINT)(lParam)), 0L)
#define FORWARD_WM_PRINT(hwnd, hDC, uFlags, fn) \
    ((fn)((hwnd), WM_PRINT, (WPARAM)(HDC)(hDC), (LPARAM)(uFlags)), 0L)
#endif

/* void Cls_OnPrintClient(HWND hwnd, HDC hDC, UINT uFlags) */
#ifndef HANDLE_WM_PRINTCLIENT
#define HANDLE_WM_PRINTCLIENT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDC)(wParam), (UINT)(lParam)), 0L)
#define FORWARD_WM_PRINTCLIENT(hwnd, hDC, uFlags, fn) \
    ((fn)((hwnd), WM_PRINTCLIENT, (WPARAM)(HDC)(hDC), (LPARAM)(uFlags)), 0L)
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
    ((fn)((hwnd), WM_ENTERSIZEMOVE, 0, 0L), 0L)
#endif

/* void Cls_OnExitSizeMove(HWND hwnd) */
#ifndef HANDLE_WM_EXITSIZEMOVE
#define HANDLE_WM_EXITSIZEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_EXITSIZEMOVE(hwnd, fn) \
    ((fn)((hwnd), WM_EXITSIZEMOVE, 0, 0L), 0L)
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

/* LONG Edit_OnGetSel(HWND hwnd, LPDWORD lpdwStart, LPDWORD lpdwEnd) */
#define HANDLE_EM_GETSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(LONG)(fn)((hwnd), (LPDWORD)(wParam), (LPDWORD)(lParam))

/* void Edit_OnSetSel(HWND hwnd, INT nStart, INT nEnd) */
#define HANDLE_EM_SETSEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), (INT)(lParam)), 0L)

/* void Edit_OnGetRect(HWND hwnd, LPRECT prc) */
#define HANDLE_EM_GETRECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPRECT)(lParam)), 0L)

/* void Edit_OnSetRect(HWND hwnd, LPCRECT prc) */
#define HANDLE_EM_SETRECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPCRECT)(lParam)), 0L)

/* void Edit_OnSetRectNP(HWND hwnd, LPCRECT prc) */
#define HANDLE_EM_SETRECTNP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPCRECT)(lParam)), 0L)

/* DWORD Edit_OnScroll(HWND hwnd, INT nScroll) */
#define HANDLE_EM_SCROLL(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (INT)(wParam))

/* BOOL Edit_OnLineScroll(HWND hwnd, INT cxScroll, INT cyScroll) */
#define HANDLE_EM_LINESCROLL(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* BOOL Edit_OnScrollCaret(HWND hwnd) */
#define HANDLE_EM_SCROLLCARET(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd))

/* BOOL Edit_OnGetModify(HWND hwnd) */
#define HANDLE_EM_GETMODIFY(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd))

/* void Edit_OnSetModify(HWND hwnd, BOOL fModified) */
#define HANDLE_EM_SETMODIFY(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)

/* INT Edit_OnGetLineCount(HWND hwnd) */
#define HANDLE_EM_GETLINECOUNT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT Edit_OnLineIndex(HWND hwnd, INT line) */
#define HANDLE_EM_LINEINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* void Edit_OnSetHandle(HWND hwnd, HLOCAL hloc) */
#define HANDLE_EM_SETHANDLE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HLOCAL)(wParam)), 0L)

/* HLOCAL Edit_OnGetHandle(HWND hwnd) */
#define HANDLE_EM_GETHANDLE(hwnd, wParam, lParam, fn) \
    (LRESULT)(HLOCAL)(fn)((hwnd))

/* INT Edit_OnGetThumb(HWND hwnd) */
#define HANDLE_EM_GETTHUMB(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT Edit_OnLineLength(HWND hwnd, INT ich) */
#define HANDLE_EM_LINELENGTH(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* void Edit_OnReplaceSel(HWND hwnd, BOOL fCanUndo, LPCTSTR lpszReplace) */
#define HANDLE_EM_REPLACESEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (LPCTSTR)(lParam)), 0L)

/* INT Edit_OnGetLine(HWND hwnd, INT line, LPCTSTR lpch) */
#define HANDLE_EM_GETLINE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* NOTE: EM_LIMITTEXT is same value as EM_SETLIMITTEXT */
/* void Edit_OnLimitText(HWND hwnd, LONG cchMax) */
#define HANDLE_EM_LIMITTEXT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)(wParam)), 0L)

/* BOOL Edit_OnCanUndo(HWND hwnd) */
#define HANDLE_EM_CANUNDO(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd))

/* BOOL Edit_OnUndo(HWND hwnd) */
#define HANDLE_EM_UNDO(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd))

/* BOOL Edit_OnFmtLines(HWND hwnd, BOOL fAddEOL) */
#define HANDLE_EM_FMTLINES(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (BOOL)(wParam))

/* INT Edit_OnLineFromChar(HWND hwnd, INT ich) */
#define HANDLE_EM_LINEFROMCHAR(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* BOOL Edit_OnSetTabStops(HWND hwnd, INT cTabs, LPDWORD lpdwTabs) */
#define HANDLE_EM_SETTABSTOPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (INT)(wParam), (LPDWORD)(lParam))

/* void Edit_OnSetPasswordChar(HWND hwnd, UINT ch) */
#define HANDLE_EM_SETPASSWORDCHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam)), 0L)

/* void Edit_OnEmptyUndoBuffer(HWND hwnd) */
#define HANDLE_EM_EMPTYUNDOBUFFER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)

/* INT Edit_OnGetFirstVisibleLine(HWND hwnd) */
#define HANDLE_EM_GETFIRSTVISIBLELINE(hwnd, wParam, lParam, fn) \
    (INT)((fn)((hwnd)), 0L)

/* BOOL Edit_OnSetReadOnly(HWND hwnd, BOOL fReadOnly) */
#define HANDLE_EM_SETREADONLY(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (BOOL)(wParam))

/* void Edit_OnSetWordBreakProc(HWND hwnd, EDITWORDBREAKPROC ewbprc) */
#define HANDLE_EM_SETWORDBREAKPROC(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (EDITWORDBREAKPROC)(lParam)), 0L)

/* EDITWORDBREAKPROC Edit_OnGetWordBreakProc(HWND hwnd) */
#define HANDLE_EM_GETWORDBREAKPROC(hwnd, wParam, lParam, fn) \
    (LRESULT)(EDITWORDBREAKPROC)(fn)((hwnd))

/* UINT Edit_OnGetPasswordChar(HWND hwnd) */
#define HANDLE_EM_GETPASSWORDCHAR(hwnd, wParam, lParam, fn) \
    (LRESULT)(UINT)(UINT)(fn)((hwnd))

/* void Edit_OnSetMargins(HWND hwnd, UINT fwMargin, WORD wLeft, WORD wRight) */
#define HANDLE_EM_SETMARGINS(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), LOWORD(lParam), HIWORD(lParam)), 0L)

/* DWORD Edit_OnGetMargins(HWND hwnd) */
#define HANDLE_EM_GETMARGINS(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd))

/* void Edit_OnSetLimitText(HWND hwnd, DWORD cbMax) */
#define HANDLE_EM_SETLIMITTEXT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (DWORD)(wParam)), 0L)

/* DWORD Edit_OnGetLimitText(HWND hwnd) */
#define HANDLE_EM_GETLIMITTEXT(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd))

/* void Edit_OnPosFromChar(HWND hwnd, LPPOINT lpPoint, UINT wCharIndex) */
#define HANDLE_EM_POSFROMCHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPPOINT)(wParam), (UINT)(lParam)), 0L)

/* LONG Edit_OnCharFromPos(HWND hwnd, INT x, INT y) */
#define HANDLE_EM_CHARFROMPOS(hwnd, wParam, lParam, fn) \
    (LRESULT)(LONG)(fn)((hwnd), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))

/* DWORD Edit_OnSetImeStatus(HWND hwnd, UINT uType, DWORD dwFlags) */
#define HANDLE_EM_SETIMESTATUS(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam), (DWORD)(lParam))

/* DWORD Edit_OnGetImeStatus(HWND hwnd, UINT uType) */
#define HANDLE_EM_GETIMESTATUS(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam))

/* HICON Static_OnSetIcon(HWND hwnd, HICON hIcon) */
#define HANDLE_STM_SETICON(hwnd, wParam, lParam, fn) \
    (LRESULT)(HICON)(fn)((hwnd), (HICON)(wParam))

/* HICON Static_OnGetIcon(HWND hwnd) */
#define HANDLE_STM_GETICON(hwnd, wParam, lParam, fn) \
    (LRESULT)(HICON)(fn)((hwnd))

/* HANDLE Static_OnSetImage(HWND hwnd, UINT fImageType, HANDLE hImage) */
#define HANDLE_STM_SETIMAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(HANDLE)(fn)((hwnd), (UINT)(wParam), (HANDLE)(lParam))

/* HANDLE Static_OnGetImage(HWND hwnd, UINT fImageType) */
#define HANDLE_STM_GETIMAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(HANDLE)(fn)((hwnd), (UINT)(wParam))

/* INT ListBox_OnAddString(HWND hwnd, LPCTSTR lpsz) */
#define HANDLE_LB_ADDSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (LPCTSTR)(lParam))

/* INT ListBox_OnInsertString(HWND hwnd, INT index, LPCTSTR lpsz) */
#define HANDLE_LB_INSERTSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* INT ListBox_OnDeleteString(HWND hwnd, INT index) */
#define HANDLE_LB_DELETESTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnSelItemRangeEx(HWND hwnd, UINT wFirst, UINT wLast) */
#define HANDLE_LB_SELITEMRANGEEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (UINT)(wParam), (UINT)(lParam))

/* void ListBox_OnResetContent(HWND hwnd) */
#define HANDLE_LB_RESETCONTENT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)

/* INT ListBox_OnSetSel(HWND hwnd, BOOL fSelect, UINT index) */
#define HANDLE_LB_SETSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (BOOL)(wParam), (UINT)(lParam))

/* INT ListBox_OnSetCurSel(HWND hwnd, INT index) */
#define HANDLE_LB_SETCURSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnGetSel(HWND hwnd, INT index) */
#define HANDLE_LB_GETSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnGetCurSel(HWND hwnd) */
#define HANDLE_LB_GETCURSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnGetText(HWND hwnd, INT index, LPTSTR lpszBuffer) */
#define HANDLE_LB_GETTEXT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPTSTR)(lParam))

/* INT ListBox_OnGetTextLen(HWND hwnd, INT index) */
#define HANDLE_LB_GETTEXTLEN(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnGetCount(HWND hwnd) */
#define HANDLE_LB_GETCOUNT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnSelectString(HWND hwnd, INT indexStart, LPCTSTR lpszFind) */
#define HANDLE_LB_SELECTSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* INT ListBox_OnDir(HWND hwnd, UINT uAttrs, LPCTSTR lpszFileSpec) */
#define HANDLE_LB_DIR(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (UINT)(wParam), (LPCTSTR)(lParam))

/* INT ListBox_OnGetTopIndex(HWND hwnd) */
#define HANDLE_LB_GETTOPINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnFindString(HWND hwnd, INT indexStart, LPCTSTR lpszFind) */
#define HANDLE_LB_FINDSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* INT ListBox_OnGetSelCount(HWND hwnd) */
#define HANDLE_LB_GETSELCOUNT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnGetSelItems(HWND hwnd, UINT cItems, LPINT lpnItems) */
#define HANDLE_LB_GETSELITEMS(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (UINT)(wParam), (LPINT)(lParam))

/* BOOL ListBox_OnSetTabStops(HWND hwnd, UINT cTabs, LPINT lpnTabs) */
#define HANDLE_LB_SETTABSTOPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(BOOL)(fn)((hwnd), (UINT)(wParam), (LPINT)(lParam))

/* INT ListBox_OnGetHorizontalExtent(HWND hwnd) */
#define HANDLE_LB_GETHORIZONTALEXTENT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* void ListBox_OnSetHorizontalExtent(HWND hwnd, INT cxExtent) */
#define HANDLE_LB_SETHORIZONTALEXTENT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam)), 0L)

/* void ListBox_OnSetColumnWidth(HWND hwnd, INT cxColumn) */
#define HANDLE_LB_SETCOLUMNWIDTH(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam)), 0L)

/* INT ListBox_OnAddFile(HWND hwnd, LPCTSTR lpszFilename) */
#define HANDLE_LB_ADDFILE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (LPCTSTR)(lParam))

/* INT ListBox_OnSetTopIndex(HWND hwnd, INT index) */
#define HANDLE_LB_SETTOPINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnGetItemRect(HWND hwnd, INT index, RECT FAR *lprc) */
#define HANDLE_LB_GETITEMRECT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (RECT FAR *)(lParam))

/* INT_PTR ListBox_OnGetItemData(HWND hwnd, INT index) */
#define HANDLE_LB_GETITEMDATA(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(fn)((hwnd), (INT)(wParam))

/* INT_PTR ListBox_OnSetItemData(HWND hwnd, INT index, LPARAM dwData) */
#define HANDLE_LB_SETITEMDATA(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(fn)((hwnd), (INT)(wParam), (lParam))

/* INT ListBox_OnSelItemRange(HWND hwnd, BOOL fSelect, UINT wFirst, UINT wLast) */
#define HANDLE_LB_SELITEMRANGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (BOOL)(wParam), LOWORD(lParam), HIWORD(lParam))

/* INT ListBox_OnSetAnchorIndex(HWND hwnd, INT index) */
#define HANDLE_LB_SETANCHORINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnGetAnchorIndex(HWND hwnd) */
#define HANDLE_LB_GETANCHORINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnSetCaretIndex(HWND hwnd, INT index, BOOL fScroll) */
#define HANDLE_LB_SETCARETINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (BOOL)LOWORD(lParam))

/* INT ListBox_OnGetCaretIndex(HWND hwnd) */
#define HANDLE_LB_GETCARETINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ListBox_OnSetItemHeight(HWND hwnd, INT index, INT cyItem) */
#define HANDLE_LB_SETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (SHORT)LOWORD(lParam))

/* INT ListBox_OnGetItemHeight(HWND hwnd, INT index) */
#define HANDLE_LB_GETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ListBox_OnFindStringExact(HWND hwnd, INT indexStart, LPCTSTR lpszFind) */
#define HANDLE_LB_FINDSTRINGEXACT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* LCID ListBox_OnSetLocale(HWND hwnd, LCID wLocaleID) */
#define HANDLE_LB_SETLOCALE(hwnd, wParam, lParam, fn) \
    (LRESULT)(LCID)(fn)((hwnd), (LCID)(wParam))

/* LCID ListBox_OnGetLocale(HWND hwnd) */
#define HANDLE_LB_GETLOCALE(hwnd, wParam, lParam, fn) \
    (LRESULT)(LCID)(fn)((hwnd))

/* INT ListBox_OnSetCount(HWND hwnd, INT cItems) */
#define HANDLE_LB_SETCOUNT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* DWORD ListBox_OnInitStorage(HWND hwnd, UINT cItems, DWORD cb) */
#define HANDLE_LB_INITSTORAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam), (DWORD)(lParam))

/* DWORD ListBox_OnItemFromPoint(HWND hwnd, INT xPos, INT yPos) */
#define HANDLE_LB_ITEMFROMPOINT(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))

/* TODO: HANDLE_LB_MULTIPLEADDSTRING and HANDLE_LB_GETLISTBOXINFO. */

/* DWORD ComboBox_OnGetEditSel(HWND hwnd, LPDWORD lpdwStart, LPDWORD lpdwEnd) */
#define HANDLE_CB_GETEDITSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((hwnd), (LPDWORD)(wParam), (LPDWORD)(lParam))

/* void ComboBox_OnLimitText(HWND hwnd, UINT cchLimit) */
#define HANDLE_CB_LIMITTEXT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam)), TRUE)

/* INT ComboBox_OnSetEditSel(HWND hwnd, INT ichStart, INT ichEnd) */
#define HANDLE_CB_SETEDITSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* INT ComboBox_OnAddString(HWND hwnd, LPCTSTR lpsz) */
#define HANDLE_CB_ADDSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (LPCTSTR)(lParam))

/* INT ComboBox_OnDeleteString(HWND hwnd, INT index) */
#define HANDLE_CB_DELETESTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ComboBox_OnDir(HWND hwnd, UINT uAttrs, LPCTSTR lpszFileSpec) */
#define HANDLE_CB_DIR(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (UINT)(wParam), (LPCTSTR)(lParam))

/* INT ComboBox_OnGetCount(HWND hwnd) */
#define HANDLE_CB_GETCOUNT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ComboBox_GetCurSel(HWND hwnd) */
#define HANDLE_CB_GETCURSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ComboBox_GetLBText(HWND hwnd, INT index, LPTSTR lpszBuffer) */
#define HANDLE_CB_GETLBTEXT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPTSTR)(lParam))

/* INT ComboBox_OnGetLBTextLen(HWND hwnd, INT index) */
#define HANDLE_CB_GETLBTEXTLEN(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ComboBox_OnInsertString(HWND hwnd, INT index, LPCTSTR lpsz) */
#define HANDLE_CB_INSERTSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* void ComboBox_OnResetContent(HWND hwnd) */
#define HANDLE_CB_RESETCONTENT(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd)), CB_OKAY)

/* INT ComboBox_OnFindString(HWND hwnd, INT indexStart, LPCTSTR lpszFind) */
#define HANDLE_CB_FINDSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* INT ComboBox_OnSelectString(HWND hwnd, INT indexStart, LPCTSTR lpszSelect) */
#define HANDLE_CB_SELECTSTRING(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* INT ComboBox_OnSetCurSel(HWND hwnd, INT index) */
#define HANDLE_CB_SETCURSEL(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* BOOL ComboBox_OnShowDropDown(HWND hwnd, BOOL fShow) */
#define HANDLE_CB_SHOWDROPDOWN(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), (BOOL)(wParam)), TRUE)

/* INT_PTR ComboBox_OnGetItemData(HWND hwnd, INT index) */
#define HANDLE_CB_GETITEMDATA(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(fn)((hwnd), (INT)(wParam))

/* INT_PTR ComboBox_OnSetItemData(HWND hwnd, INT index, DWORD_PTR dwData) */
#define HANDLE_CB_SETITEMDATA(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(fn)((hwnd), (INT)(wParam), (DWORD_PTR)(lParam))

/* void ComboBox_OnGetDroppedControlRect(HWND hwnd, RECT FAR *lprc) */
#define HANDLE_CB_GETDROPPEDCONTROLRECT(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)((hwnd), (RECT FAR *)(lParam)), CB_OKAY)

/* INT ComboBox_OnSetItemHeight(HWND hwnd, INT index, INT height) */
#define HANDLE_CB_SETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* INT ComboBox_OnGetItemHeight(HWND hwnd, INT index) */
#define HANDLE_CB_GETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ComboBox_OnSetExtendedUI(HWND hwnd, BOOL fExtended) */
#define HANDLE_CB_SETEXTENDEDUI(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (BOOL)(wParam))

/* BOOL ComboBox_OnGetExtendedUI(HWND hwnd) */
#define HANDLE_CB_GETEXTENDEDUI(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(BOOL)(fn)((hwnd))

/* BOOL ComboBox_OnGetDroppedState(HWND hwnd) */
#define HANDLE_CB_GETDROPPEDSTATE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(BOOL)(fn)((hwnd))

/* INT ComboBox_OnFindStringExact(HWND hwnd, INT indexStart, LPCTSTR lpszFind) */
#define HANDLE_CB_FINDSTRINGEXACT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (LPCTSTR)(lParam))

/* LCID ComboBox_OnSetLocale(HWND hwnd, LCID wLocaleID) */
#define HANDLE_CB_SETLOCALE(hwnd, wParam, lParam, fn) \
    (LRESULT)(LCID)(fn)((hwnd), (LCID)(wParam))

/* LCID ComboBox_OnGetLocale(HWND hwnd) */
#define HANDLE_CB_GETLOCALE(hwnd, wParam, lParam, fn) \
    (LRESULT)(LCID)(fn)((hwnd))

/* INT ComboBox_OnGetTopIndex(HWND hwnd) */
#define HANDLE_CB_GETTOPINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ComboBox_OnSetTopIndex(HWND hwnd, INT index) */
#define HANDLE_CB_SETTOPINDEX(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ComboBox_OnGetHorizontalExtent(HWND hwnd) */
#define HANDLE_CB_GETHORIZONTALEXTENT(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* void ComboBox_OnSetHorizontalExtent(HWND hwnd, INT cxExtent) */
#define HANDLE_CB_SETHORIZONTALEXTENT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam)), 0L)

/* INT ComboBox_OnGetDroppedWidth(HWND hwnd) */
#define HANDLE_CB_GETDROPPEDWIDTH(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ComboBox_OnSetDroppedWidth(HWND hwnd, INT wWidth) */
#define HANDLE_CB_SETDROPPEDWIDTH(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam))

/* INT ComboBox_OnInitStorage(HWND hwnd, INT cItems, DWORD cb) */
#define HANDLE_CB_INITSTORAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (DWORD)(lParam))

/* TODO: CB_MULTIPLEADDSTRING and CB_GETCOMBOBOXINFO */

/* INT ScrollBar_OnSetPos(HWND hwnd, INT nPos, BOOL fRedraw) */
#define HANDLE_SBM_SETPOS(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (BOOL)(lParam))

/* INT ScrollBar_OnGetPos(HWND hwnd) */
#define HANDLE_SBM_GETPOS(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd))

/* INT ScrollBar_OnSetRange(HWND hwnd, INT nMinPos, INT nMaxPos) */
#define HANDLE_SBM_SETRANGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* INT ScrollBar_OnSetRangeRedraw(HWND hwnd, INT nMinPos, INT nMaxPos) */
#define HANDLE_SBM_SETRANGEREDRAW(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* void ScrollBar_OnGetRange(HWND hwnd, LPINT lpnMinPos, LPINT lpnMaxPos) */
#define HANDLE_SBM_GETRANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LPINT)(wParam), (LPINT)(lParam)), 0L)

/* BOOL ScrollBar_OnEnableArrows(HWND hwnd, UINT fuArrowFlags) */
#define HANDLE_SBM_ENABLE_ARROWS(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(BOOL)(fn)((hwnd), (UINT)(wParam))

/* INT ScrollBar_OnSetScrollInfo(HWND hwnd, BOOL fRedraw, LPSCROLLINFO lpsi) */
#define HANDLE_SBM_SETSCROLLINFO(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(INT)(fn)((hwnd), (BOOL)(wParam), (LPSCROLLINFO)(lParam))

/* BOOL ScrollBar_OnGetScrollInfo(HWND hwnd, LPSCROLLINFO lpsi) */
#define HANDLE_SBM_GETSCROLLINFO(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(BOOL)(fn)((hwnd), (LPSCROLLINFO)(lParam))

/* BOOL ScrollBar_OnGetScrollBarInfo(HWND hwnd, LPSCROLLBARINFO lpsbi) */
#define HANDLE_SBM_GETSCROLLBARINFO(hwnd, wParam, lParam, fn) \
    (LRESULT)(INT_PTR)(BOOL)(fn)((hwnd), (LPSCROLLBARINFO)(lParam))

/* COLORREF ListView_OnGetBkColor(HWND hwnd) */
#define HANDLE_LVM_GETBKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* BOOL ListView_OnSetBkColor(HWND hwnd, COLORREF clrBk) */
#define HANDLE_LVM_SETBKCOLOR(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (COLORREF)(wParam))

/* HIMAGELIST ListView_OnGetImageList(HWND hwnd, INT iImageList) */
#define HANDLE_LVM_GETIMAGELIST(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (INT)(wParam))

/* HIMAGELIST ListView_OnSetImageList(HWND hwnd, INT iImageList, HIMAGELIST himl) */
#define HANDLE_LVM_SETIMAGELIST(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (INT)(wParam), (HIMAGELIST)(lParam))

/* INT ListView_OnGetItemCount(HWND hwnd) */
#define HANDLE_LVM_GETITEMCOUNT(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* BOOL ListView_OnGetItemA(HWND hwnd, LV_ITEMA *pitem) */
#define HANDLE_LVM_GETITEMA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LV_ITEMA *)(lParam))

/* BOOL ListView_OnSetItemA(HWND hwnd, const LV_ITEMA *pitem) */
#define HANDLE_LVM_SETITEMA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LV_ITEMA *)(lParam))

/* INT ListView_OnInsertItemA(HWND hwnd, const LV_ITEMA *pitem) */
#define HANDLE_LVM_INSERTITEMA(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (const LV_ITEMA *)(lParam))

/* BOOL ListView_OnDeleteItem(HWND hwnd, INT i) */
#define HANDLE_LVM_DELETEITEM(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam))

/* BOOL ListView_OnDeleteAllItems(HWND hwnd) */
#define HANDLE_LVM_DELETEALLITEMS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd))

/* BOOL ListView_OnGetCallbackMask(HWND hwnd) */
#define HANDLE_LVM_GETCALLBACKMASK(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd))

/* BOOL ListView_OnSetCallbackMask(HWND hwnd, UINT mask) */
#define HANDLE_LVM_SETCALLBACKMASK(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT)(wParam))

/* INT ListView_OnGetNextItem(HWND hwnd, INT i, UINT flags) */
#define HANDLE_LVM_GETNEXTITEM(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (UINT)(lParam))

/* INT ListView_OnFindItemA(HWND hwnd, INT iStart, const LV_FINDINFOA *plvfi) */
#define HANDLE_LVM_FINDITEMA(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (const LV_FINDINFOA *)(lParam))

/* BOOL ListView_OnGetItemRect(HWND hwnd, INT i, RECT *prc) */
#define HANDLE_LVM_GETITEMRECT(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (RECT *)(lParam))

/* BOOL ListView_OnSetItemPosition(HWND hwnd, INT i, INT x, INT y) */
#define HANDLE_LVM_SETITEMPOSITION(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))

/* BOOL ListView_OnGetItemPosition(HWND hwnd, INT i, POINT *ppt) */
#define HANDLE_LVM_GETITEMPOSITION(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (POINT *)(lParam))

/* INT ListView_OnGetStringWidthA(HWND hwnd, LPCSTR psz) */
#define HANDLE_LVM_GETSTRINGWIDTHA(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (LPCSTR)(lParam))

/* INT ListView_OnHitTest(HWND hwnd, LV_HITTESTINFO *pinfo) */
#define HANDLE_LVM_HITTEST(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (LV_HITTESTINFO *)(lParam))

/* BOOL ListView_OnEnsureVisible(HWND hwnd, INT i, BOOL fPartialOK) */
#define HANDLE_LVM_ENSUREVISIBLE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (BOOL)(lParam))

/* BOOL ListView_OnScroll(HWND hwnd, INT dx, INT dy) */
#define HANDLE_LVM_SCROLL(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* BOOL ListView_OnRedrawItems(HWND hwnd, INT iFirst, INT iLast) */
#define HANDLE_LVM_REDRAWITEMS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* BOOL ListView_OnArrange(HWND hwnd, UINT code) */
#define HANDLE_LVM_ARRANGE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT)(wParam))

/* HWND ListView_OnEditLabelA(HWND hwnd, INT i) */
#define HANDLE_LVM_EDITLABELA(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (INT)(wParam))

/* HWND ListView_OnGetEditControl(HWND hwnd) */
#define HANDLE_LVM_GETEDITCONTROL(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd))

/* BOOL ListView_OnGetColumnA(HWND hwnd, INT iCol, LV_COLUMNA *pcol) */
#define HANDLE_LVM_GETCOLUMNA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (LV_COLUMNA *)(lParam))

/* BOOL ListView_OnSetColumnA(HWND hwnd, INT iCol, const LV_COLUMNA *pcol) */
#define HANDLE_LVM_SETCOLUMNA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (const LV_COLUMNA *)(lParam))

/* INT ListView_OnInsertColumnA(HWND hwnd, INT iCol, const LV_COLUMNA *pcol) */
#define HANDLE_LVM_INSERTCOLUMNA(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (const LV_COLUMNA *)(lParam))

/* BOOL ListView_OnDeleteColumn(HWND hwnd, INT iCol) */
#define HANDLE_LVM_DELETECOLUMN(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam))

/* INT ListView_OnGetColumnWidth(HWND hwnd, INT iCol) */
#define HANDLE_LVM_GETCOLUMNWIDTH(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam))

/* BOOL ListView_OnSetColumnWidth(HWND hwnd, INT iCol, INT cx) */
#define HANDLE_LVM_SETCOLUMNWIDTH(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* HWND ListView_OnGetHeader(HWND hwnd) */
#define HANDLE_LVM_GETHEADER(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd))

/* HIMAGELIST ListView_OnCreateDragImage(HWND hwnd, INT i, LPPOINT lpptUpLeft) */
#define HANDLE_LVM_CREATEDRAGIMAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (INT)(wParam), (LPPOINT)(lParam))

/* BOOL ListView_OnGetViewRect(HWND hwnd, RECT *prc) */
#define HANDLE_LVM_GETVIEWRECT(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (RECT *)(lParam))

/* COLORREF ListView_OnGetTextColor(HWND hwnd) */
#define HANDLE_LVM_GETTEXTCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* BOOL ListView_OnSetTextColor(HWND hwnd, COLORREF clrText) */
#define HANDLE_LVM_SETTEXTCOLOR(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF ListView_OnGetTextBkColor(HWND hwnd) */
#define HANDLE_LVM_GETTEXTBKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* BOOL ListView_OnSetTextBkColor(HWND hwnd, COLORREF clrTextBk) */
#define HANDLE_LVM_SETTEXTBKCOLOR(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (COLORREF)(lParam))

/* INT ListView_OnGetTopIndex(HWND hwnd) */
#define HANDLE_LVM_GETTOPINDEX(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* INT ListView_OnGetCountPerPage(HWND hwnd) */
#define HANDLE_LVM_GETCOUNTPERPAGE(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* BOOL ListView_OnGetOrigin(HWND hwnd, POINT *ppt) */
#define HANDLE_LVM_GETORIGIN(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (POINT *)(lParam))

/* BOOL ListView_OnUpdate(HWND hwnd, INT i) */
#define HANDLE_LVM_UPDATE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam))

/* BOOL ListView_OnSetItemState(HWND hwnd, INT i, LV_ITEM *lvi) */
#define HANDLE_LVM_SETITEMSTATE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (LV_ITEM *)(lParam))

/* UINT ListView_OnGetItemState(HWND hwnd, INT i, UINT mask) */
#define HANDLE_LVM_GETITEMSTATE(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (INT)(wParam), (UINT)(lParam))

/* INT ListView_OnGetItemTextA(HWND hwnd, INT i, LV_ITEMA *lvi) */
#define HANDLE_LVM_GETITEMTEXTA(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (LV_ITEMA *)(lParam))

/* BOOL ListView_OnSetItemTextA(HWND hwnd, INT i, const LV_ITEMA *lvi) */
#define HANDLE_LVM_SETITEMTEXTA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (const LV_ITEMA *)(lParam))

/* void ListView_OnSetItemCount(HWND hwnd, INT cItems) */
#define HANDLE_LVM_SETITEMCOUNT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam)), 0L)

/* BOOL ListView_OnSortItems(HWND hwnd, LPARAM lPrm, PFNLVCOMPARE pfnCompare) */
#define HANDLE_LVM_SORTITEMS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPARAM)(wParam), (PFNLVCOMPARE)(lParam))

/* void ListView_OnSetItemPosition32(HWND hwnd, INT i, const POINT *ppt) */
#define HANDLE_LVM_SETITEMPOSITION32(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam), (const POINT *)(lParam)), 0L)

/* UINT ListView_OnGetSelectedCount(HWND hwnd) */
#define HANDLE_LVM_GETSELECTEDCOUNT(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

/* DWORD ListView_OnGetItemSpacing(HWND hwnd, BOOL fSmall) */
#define HANDLE_LVM_GETITEMSPACING(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (BOOL)(wParam))

/* BOOL ListView_OnGetISearchStringA(HWND hwnd, LPSTR lpsz) */
#define HANDLE_LVM_GETISEARCHSTRINGA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPSTR)(lParam))

/* DWORD ListView_OnSetIconSpacing(HWND hwnd, INT cx, INT cy) */
#define HANDLE_LVM_SETICONSPACING(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))

/* DWORD ListView_OnSetExtendedListViewStyle(HWND hwnd, DWORD dwMask, DWORD dw) */
#define HANDLE_LVM_SETEXTENDEDLISTVIEWSTYLE(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (DWORD)(wParam), (DWORD)(lParam))

/* DWORD ListView_OnGetExtendedListViewStyle(HWND hwnd) */
#define HANDLE_LVM_GETEXTENDEDLISTVIEWSTYLE(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd))

/* BOOL ListView_OnGetSubItemRect(HWND hwnd, INT iItem, RECT *prc) */
#define HANDLE_LVM_GETSUBITEMRECT(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (RECT *)(lParam))

/* INT ListView_OnSubItemHitTest(HWND hwnd, WPARAM wParam, LPLVHITTESTINFO plvhti) */
#define HANDLE_LVM_SUBITEMHITTEST(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (wParam), (LPLVHITTESTINFO)(lParam))

/* BOOL ListView_OnSetColumnOrderArray(HWND hwnd, INT iCount, LPINT pi) */
#define HANDLE_LVM_SETCOLUMNORDERARRAY(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (LPINT)(lParam))

/* BOOL ListView_OnGetColumnOrderArray(HWND hwnd, INT iCount, LPINT pi) */
#define HANDLE_LVM_GETCOLUMNORDERARRAY(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (LPINT)(lParam))

/* INT ListView_OnSetHotItem(HWND hwnd, INT i) */
#define HANDLE_LVM_SETHOTITEM(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam))

/* INT ListView_OnGetHotItem(HWND hwnd) */
#define HANDLE_LVM_GETHOTITEM(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* HCURSOR ListView_OnSetHotCursor(HWND hwnd, HCURSOR hcur) */
#define HANDLE_LVM_SETHOTCURSOR(hwnd, wParam, lParam, fn) \
    (LRESULT)(HCURSOR)(fn)((hwnd), (HCURSOR)(lParam))

/* HCURSOR ListView_OnGetHotCursor(HWND hwnd) */
#define HANDLE_LVM_GETHOTCURSOR(hwnd, wParam, lParam, fn) \
    (LRESULT)(HCURSOR)(fn)((hwnd))

/* DWORD ListView_OnApproximateViewRect(HWND hwnd, INT iWidth, INT iHeight, INT iCount) */
#define HANDLE_LVM_APPROXIMATEVIEWRECT(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (INT)(wParam), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))

/* BOOL ListView_OnSetWorkAreas(HWND hwnd, INT nWorkAreas, const RECT *prc) */
#define HANDLE_LVM_SETWORKAREAS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (const RECT *)(lParam))

/* INT ListView_OnGetSelectionMark(HWND hwnd) */
#define HANDLE_LVM_GETSELECTIONMARK(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* INT ListView_OnSetSelectionMark(HWND hwnd, INT i) */
#define HANDLE_LVM_SETSELECTIONMARK(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(lParam))

/* BOOL ListView_OnSetBkImageA(HWND hwnd, const LVBKIMAGEA *plvbki) */
#define HANDLE_LVM_SETBKIMAGEA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVBKIMAGEA *)(lParam))

/* BOOL ListView_OnGetBkImageA(HWND hwnd, LVBKIMAGEA *plvbki) */
#define HANDLE_LVM_GETBKIMAGEA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LVBKIMAGEA *)(lParam))

/* BOOL ListView_OnGetWorkAreas(HWND hwnd, INT nWorkAreas, RECT *prc) */
#define HANDLE_LVM_GETWORKAREAS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (RECT *)(lParam))

/* DWORD ListView_OnSetHoverTime(HWND hwnd, DWORD dwHoverTimeMs) */
#define HANDLE_LVM_SETHOVERTIME(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (DWORD)(lParam))

/* DWORD ListView_OnGetHoverTime(HWND hwnd) */
#define HANDLE_LVM_GETHOVERTIME(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd))

/* BOOL ListView_OnGetNumberOfWorkAreas(HWND hwnd, UINT *pnWorkAreas) */
#define HANDLE_LVM_GETNUMBEROFWORKAREAS(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT *)(lParam))

/* HWND ListView_OnSetToolTips(HWND hwnd, HWND hwndNewHwnd) */
#define HANDLE_LVM_SETTOOLTIPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (HWND)(wParam))

/* BOOL ListView_OnGetItemW(HWND hwnd, LV_ITEMW *pitem) */
#define HANDLE_LVM_GETITEMW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LV_ITEMW *)(lParam))

/* BOOL ListView_OnSetItemW(HWND hwnd, const LV_ITEMW *pitem) */
#define HANDLE_LVM_SETITEMW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LV_ITEMW *)(lParam))

/* INT ListView_OnInsertItemW(HWND hwnd, const LV_ITEMW *pitem) */
#define HANDLE_LVM_INSERTITEMW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (const LV_ITEMW *)(lParam))

/* HWND ListView_OnGetToolTips(HWND hwnd) */
#define HANDLE_LVM_GETTOOLTIPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd))

/* BOOL ListView_OnSortItemsEx(HWND hwnd, PFNLVCOMPARE pfnCompare, LPARAM lPrm) */
#define HANDLE_LVM_SORTITEMSEX(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (PFNLVCOMPARE)(lParam), (LPARAM)(wParam))

/* INT ListView_OnFindItemW(HWND hwnd, INT iStart, const LV_FINDINFOW *plvfi) */
#define HANDLE_LVM_FINDITEMW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (const LV_FINDINFOW *)(lParam))

/* INT ListView_OnGetStringWidthW(HWND hwnd, LPCWSTR psz) */
#define HANDLE_LVM_GETSTRINGWIDTHW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (LPCWSTR)(lParam))

#if NTDDI_VERSION >= 0x06000000
    /* UINT ListView_OnGetGroupState(HWND hwnd, DWORD dwGroupId, DWORD dwMask) */
    #define HANDLE_LVM_GETGROUPSTATE(hwnd, wParam, lParam, fn) \
        (UINT)(fn)((hwnd), (DWORD)(wParam), (DWORD)(lParam))

    /* INT ListView_OnGetFocusedGroup(HWND hwnd) */
    #define HANDLE_LVM_GETFOCUSEDGROUP(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd))
#endif

/* INT ListView_OnGetColumnW(HWND hwnd, INT iCol) */
#define HANDLE_LVM_GETCOLUMNW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam))

/* BOOL ListView_OnSetColumnW(HWND hwnd, INT iCol, INT cx) */
#define HANDLE_LVM_SETCOLUMNW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam), (INT)(lParam))

/* INT ListView_OnInsertColumnW(HWND hwnd, INT iCol, const LV_COLUMNW *pcol) */
#define HANDLE_LVM_INSERTCOLUMNW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (const LV_COLUMNW *)(lParam))

#if NTDDI_VERSION >= 0x06000000
    /* BOOL ListView_OnGetGroupRect(HWND hwnd, INT iGroupId, RECT *prc) */
    #define HANDLE_LVM_GETGROUPRECT(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (INT)(wParam), (RECT *)(lParam))
#endif

/* INT ListView_OnGetItemTextW(HWND hwnd, INT i, LV_ITEMW *pitem) */
#define HANDLE_LVM_GETITEMTEXTW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (LV_ITEMW *)(lParam))

/* INT ListView_OnSetItemTextW(HWND hwnd, INT i, const LV_ITEMW *pitem) */
#define HANDLE_LVM_SETITEMTEXTW(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam), (const LV_ITEMW *)(lParam))

/* BOOL ListView_OnGetISearchStringW(HWND hwnd, LPWSTR lpsz) */
#define HANDLE_LVM_GETISEARCHSTRINGW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPWSTR)(lParam))

/* HWND ListView_OnEditLabelW(HWND hwnd, INT i) */
#define HANDLE_LVM_EDITLABELW(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (INT)(wParam))

/* BOOL ListView_OnSetBkImageW(HWND hwnd, const LVBKIMAGEW *plvbki) */
#define HANDLE_LVM_SETBKIMAGEW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVBKIMAGEW *)(lParam))

/* BOOL ListView_OnGetBkImageW(HWND hwnd, LVBKIMAGEW *plvbki) */
#define HANDLE_LVM_GETBKIMAGEW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LVBKIMAGEW *)(lParam))

/* void ListView_OnSetSelectedColumn(HWND hwnd, INT iCol) */
#define HANDLE_LVM_SETSELECTEDCOLUMN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (INT)(wParam)), 0L)

/* LRESULT ListView_OnSetTileWidth(HWND hwnd, WPARAM wParam, LPARAM lParam) */
#define HANDLE_LVM_SETTILEWIDTH(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (wParam), (lParam))

/* DWORD ListView_OnSetView(HWND hwnd, DWORD iView) */
#define HANDLE_LVM_SETVIEW(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd), (DWORD)(wParam))

/* DWORD ListView_OnGetView(HWND hwnd) */
#define HANDLE_LVM_GETVIEW(hwnd, wParam, lParam, fn) \
    (DWORD)(fn)((hwnd))

#if NTDDI_VERSION >= 0x06000000
    /* INT ListView_OnInsertGroup(HWND hwnd, INT iGroupId, const LVGROUP *pGroup) */
    #define HANDLE_LVM_INSERTGROUP(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (INT)(wParam), (const LVGROUP *)(lParam))

    /* INT ListView_OnSetGroupInfo(HWND hwnd, INT iGroupId, const LVGROUP *pGroup) */
    #define HANDLE_LVM_SETGROUPINFO(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (INT)(wParam), (const LVGROUP *)(lParam))

    /* INT ListView_OnGetGroupInfo(HWND hwnd, INT iGroupId, LVGROUP *pGroup) */
    #define HANDLE_LVM_GETGROUPINFO(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (INT)(wParam), (LVGROUP *)(lParam))

    /* INT ListView_OnRemoveGroup(HWND hwnd, INT iGroupId) */
    #define HANDLE_LVM_REMOVEGROUP(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (INT)(wParam))

    /* LRESULT ListView_OnMoveGroup(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #define HANDLE_LVM_MOVEGROUP(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))

    /* INT ListView_OnGetGroupCount(HWND hwnd) */
    #define HANDLE_LVM_GETGROUPCOUNT(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd))

    /* BOOL ListView_OnGetGroupInfoByIndex(HWND hwnd, INT iIndex, LVGROUP *pgrp) */
    #define HANDLE_LVM_GETGROUPINFOBYINDEX(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (INT)(wParam), (LVGROUP *)(lParam))

    /* LRESULT ListView_OnMoveItemToGroup(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #define HANDLE_LVM_MOVEITEMTOGROUP(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))

    /* void ListView_OnSetGroupMetrics(HWND hwnd, const LVGROUPMETRICS *pGroupMetrics) */
    #define HANDLE_LVM_SETGROUPMETRICS(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd), (const LVGROUPMETRICS *)(lParam)), 0L)

    /* void ListView_OnGetGroupMetrics(HWND hwnd, LVGROUPMETRICS *pGroupMetrics) */
    #define HANDLE_LVM_GETGROUPMETRICS(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd), (LVGROUPMETRICS *)(lParam)), 0L)

    /* INT ListView_OnEnableGroupView(HWND hwnd, BOOL fEnable) */
    #define HANDLE_LVM_ENABLEGROUPVIEW(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (BOOL)(wParam))

    /* BOOL ListView_OnSortGroups(HWND hwnd, PFNLVGROUPCOMPARE pfnGroupCompate, void *plv) */
    #define HANDLE_LVM_SORTGROUPS(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (PFNLVGROUPCOMPARE)(wParam), (void *)(lParam))

    /* void ListView_OnInsertGroupSorted(HWND hwnd, const LVINSERTGROUPSORTED *structInsert) */
    #define HANDLE_LVM_INSERTGROUPSORTED(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd), (const LVINSERTGROUPSORTED *)(wParam)), 0L)

    /* void ListView_OnRemoveAllGroups(HWND hwnd) */
    #define HANDLE_LVM_REMOVEALLGROUPS(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd)), 0L)

    /* BOOL ListView_OnHasGroup(HWND hwnd, DWORD dwGroupId) */
    #define HANDLE_LVM_HASGROUP(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (DWORD)(wParam))
#endif

/* BOOL ListView_OnSetTileViewInfo(HWND hwnd, const LVTILEVIEWINFO *ptvi) */
#define HANDLE_LVM_SETTILEVIEWINFO(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVTILEVIEWINFO *)(lParam))

/* void ListView_OnGetTileViewInfo(HWND hwnd, LVTILEVIEWINFO *ptvi) */
#define HANDLE_LVM_GETTILEVIEWINFO(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LVTILEVIEWINFO *)(lParam)), 0L)

/* BOOL ListView_OnSetTileInfo(HWND hwnd, const LVTILEINFO *pti) */
#define HANDLE_LVM_SETTILEINFO(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVTILEINFO *)(lParam))

/* void ListView_OnGetTileInfo(HWND hwnd, LVTILEINFO *pti) */
#define HANDLE_LVM_GETTILEINFO(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LVTILEINFO *)(lParam)), 0L)

/* BOOL ListView_OnSetInsertMark(HWND hwnd, const LVINSERTMARK *lvim) */
#define HANDLE_LVM_SETINSERTMARK(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVINSERTMARK *)(lParam))

/* BOOL ListView_OnGetInsertMark(HWND hwnd, LVINSERTMARK *lvim) */
#define HANDLE_LVM_GETINSERTMARK(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LVINSERTMARK *)(lParam))

/* INT ListView_OnInsertMarkHitTest(HWND hwnd, LPPOINT point, LPLVINSERTMARK lvim) */
#define HANDLE_LVM_INSERTMARKHITTEST(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (LPPOINT)(wParam), (LPLVINSERTMARK)(lParam))

/* INT ListView_OnGetInsertMarkRect(HWND hwnd, LPRECT rc) */
#define HANDLE_LVM_GETINSERTMARKRECT(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (LPRECT)(lParam))

/* COLORREF ListView_OnSetInsertMarkColor(HWND hwnd, COLORREF color) */
#define HANDLE_LVM_SETINSERTMARKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF ListView_OnGetInsertMarkColor(HWND hwnd) */
#define HANDLE_LVM_GETINSERTMARKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* BOOL ListView_OnSetInfoTip(HWND hwnd, const LVSETINFOTIP *plvInfoTip) */
#define HANDLE_LVM_SETINFOTIP(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const LVSETINFOTIP *)(lParam))

/* UINT ListView_OnGetSelectedColumn(HWND hwnd) */
#define HANDLE_LVM_GETSELECTEDCOLUMN(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

/* BOOL ListView_OnIsGroupViewEnabled(HWND hwnd) */
#define HANDLE_LVM_ISGROUPVIEWENABLED(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd))

/* COLORREF ListView_OnGetOutlineColor(HWND hwnd) */
#define HANDLE_LVM_GETOUTLINECOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* COLORREF ListView_OnSetOutlineColor(HWND hwnd, COLORREF color) */
#define HANDLE_LVM_SETOUTLINECOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* void ListView_OnCancelEditLabel(HWND hwnd) */
#define HANDLE_LVM_CANCELEDITLABEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)

/* UINT ListView_OnMapIndexToID(HWND hwnd, UINT index) */
#define HANDLE_LVM_MAPINDEXTOID(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (UINT)(wParam))

/* UINT ListView_OnMapIDToIndex(HWND hwnd, UINT id) */
#define HANDLE_LVM_MAPIDTOINDEX(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (UINT)(wParam))

/* BOOL ListView_OnIsItemVisible(HWND hwnd, UINT index) */
#define HANDLE_LVM_ISITEMVISIBLE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT)(wParam))

#if NTDDI_VERSION >= 0x06000000
    /* void ListView_OnGetEmptyText(HWND hwnd, PWSTR pszText, UINT cchText) */
    #define HANDLE_LVM_GETEMPTYTEXT(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd), (PWSTR)(lParam), (UINT)(wParam)), 0L)

    /* BOOL ListView_OnGetFooterRect(HWND hwnd, RECT *prc) */
    #define HANDLE_LVM_GETFOOTERRECT(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (RECT *)(lParam))

    /* BOOL ListView_OnGetFooterInfo(HWND hwnd, LVFOOTERINFO *plvfi) */
    #define HANDLE_LVM_GETFOOTERINFO(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (LVFOOTERINFO *)(lParam))

    /* BOOL ListView_OnGetFooterItemRect(HWND hwnd, INT iItem, RECT *prc) */
    #define HANDLE_LVM_GETFOOTERITEMRECT(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (INT)(wParam), (RECT *)(lParam))

    /* BOOL ListView_OnGetFooterItem(HWND hwnd, INT iItem, LVFOOTERITEM *pfi) */
    #define HANDLE_LVM_GETFOOTERITEM(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (INT)(wParam), (LVFOOTERITEM *)(lParam))

    /* BOOL ListView_OnGetItemIndexRect(HWND hwnd, const LVITEMINDEX *plvii, RECT *prc) */
    #define HANDLE_LVM_GETITEMINDEXRECT(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (const LVITEMINDEX *)(wParam), (RECT *)(lParam))

    /* HRESULT ListView_OnSetItemIndexState(HWND hwnd, const LVITEMINDEX *plvii, const LV_ITEM *lvi) */
    #define HANDLE_LVM_SETITEMINDEXSTATE(hwnd, wParam, lParam, fn) \
        (HRESULT)(fn)((hwnd), (const LVITEMINDEX *)(wParam), (const LV_ITEM *)(lParam))

    /* BOOL ListView_OnGetNextItemIndex(HWND hwnd, LVITEMINDEX *plvii, UINT flags) */
    #define HANDLE_LVM_GETNEXTITEMINDEX(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (LVITEMINDEX *)(wParam), (UINT)(lParam))
#endif

/* HTREEITEM TreeView_OnInsertItemA(HWND hwnd, LPTV_INSERTSTRUCTA lpis) */
#define HANDLE_TVM_INSERTITEMA(hwnd, wParam, lParam, fn) \
    (LRESULT)(HTREEITEM)(fn)((hwnd), (LPTV_INSERTSTRUCTA)(lParam))

/* BOOL TreeView_OnDeleteItem(HWND hwnd, HTREEITEM hitem) */
#define HANDLE_TVM_DELETEITEM(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (HTREEITEM)(lParam))

/* BOOL TreeView_OnExpand(HWND hwnd, HTREEITEM hitem, UINT code) */
#define HANDLE_TVM_EXPAND(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (HTREEITEM)(lParam), (UINT)(wParam))

/* BOOL TreeView_OnGetItemRect(HWND hwnd, UINT code, RECT *prc) */
#define HANDLE_TVM_GETITEMRECT(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT)(wParam), (RECT *)(lParam))

/* UINT TreeView_OnGetCount(HWND hwnd) */
#define HANDLE_TVM_GETCOUNT(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

/* UINT TreeView_OnGetIndent(HWND hwnd) */
#define HANDLE_TVM_GETINDENT(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

/* BOOL TreeView_OnSetIndent(HWND hwnd, INT indent) */
#define HANDLE_TVM_SETINDENT(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (INT)(wParam))

/* HIMAGELIST TreeView_OnGetImageList(HWND hwnd, INT iImage) */
#define HANDLE_TVM_GETIMAGELIST(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (INT)(wParam))

/* HIMAGELIST TreeView_OnSetImageList(HWND hwnd, INT iImage, HIMAGELIST himl) */
#define HANDLE_TVM_SETIMAGELIST(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (INT)(wParam), (HIMAGELIST)(lParam))

/* HTREEITEM TreeView_OnGetNextItem(HWND hwnd, HTREEITEM hitem, UINT code) */
#define HANDLE_TVM_GETNEXTITEM(hwnd, wParam, lParam, fn) \
    (LRESULT)(HTREEITEM)(fn)((hwnd), (HTREEITEM)(lParam), (UINT)(wParam))

/* BOOL TreeView_OnSelectItem(HWND hwnd, UINT code, HTREEITEM hitem) */
#define HANDLE_TVM_SELECTITEM(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (UINT)(wParam), (HTREEITEM)(lParam))

/* BOOL TreeView_OnGetItemA(HWND hwnd, TV_ITEMA *pitem) */
#define HANDLE_TVM_GETITEMA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (TV_ITEMA *)(lParam))

/* BOOL TreeView_OnSetItemA(HWND hwnd, const TV_ITEMA *pitem) */
#define HANDLE_TVM_SETITEMA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const TV_ITEMA *)(lParam))

/* HWND TreeView_OnEditLabelA(HWND hwnd, HTREEITEM hitem) */
#define HANDLE_TVM_EDITLABELA(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (HTREEITEM)(lParam))

/* HWND TreeView_OnGetEditControl(HWND hwnd) */
#define HANDLE_TVM_GETEDITCONTROL(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd))

/* UINT TreeView_OnGetVisibleCount(HWND hwnd) */
#define HANDLE_TVM_GETVISIBLECOUNT(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

/* HTREEITEM TreeView_OnHitTest(HWND hwnd, LPTV_HITTESTINFO lpht) */
#define HANDLE_TVM_HITTEST(hwnd, wParam, lParam, fn) \
    (LRESULT)(HTREEITEM)(fn)((hwnd), (LPTV_HITTESTINFO)(lParam))

/* HIMAGELIST TreeView_OnCreateDragImage(HWND hwnd, HTREEITEM hitem) */
#define HANDLE_TVM_CREATEDRAGIMAGE(hwnd, wParam, lParam, fn) \
    (LRESULT)(HIMAGELIST)(fn)((hwnd), (HTREEITEM)(lParam))

/* BOOL TreeView_OnSortChildren(HWND hwnd, HTREEITEM hitem, BOOL recurse) */
#define HANDLE_TVM_SORTCHILDREN(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (HTREEITEM)(lParam), (BOOL)(wParam))

/* BOOL TreeView_OnEnsureVisible(HWND hwnd, HTREEITEM hitem) */
#define HANDLE_TVM_ENSUREVISIBLE(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (HTREEITEM)(lParam))

/* BOOL TreeView_OnSortChildrenCB(HWND hwnd, LPTV_SORTCB psort, BOOL recurse) */
#define HANDLE_TVM_SORTCHILDRENCB(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPTV_SORTCB)(lParam), (BOOL)(wParam))

/* BOOL TreeView_OnEndEditLabelNow(HWND hwnd, BOOL fCancel) */
#define HANDLE_TVM_ENDEDITLABELNOW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (BOOL)(wParam))

/* BOOL TreeView_OnGetISearchStringA(HWND hwnd, LPSTR lpsz) */
#define HANDLE_TVM_GETISEARCHSTRINGA(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPSTR)(lParam))

/* HWND TreeView_OnSetToolTips(HWND hwnd, HWND hwndTT) */
#define HANDLE_TVM_SETTOOLTIPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (HWND)(wParam))

/* HWND TreeView_OnGetToolTips(HWND hwnd) */
#define HANDLE_TVM_GETTOOLTIPS(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd))

/* BOOL TreeView_OnSetInsertMark(HWND hwnd, HTREEITEM hItem, BOOL fAfter) */
#define HANDLE_TVM_SETINSERTMARK(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (HTREEITEM)(lParam), (BOOL)(wParam))

/* INT TreeView_OnSetItemHeight(HWND hwnd, INT iHeight) */
#define HANDLE_TVM_SETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd), (INT)(wParam))

/* INT TreeView_OnGetItemHeight(HWND hwnd) */
#define HANDLE_TVM_GETITEMHEIGHT(hwnd, wParam, lParam, fn) \
    (INT)(fn)((hwnd))

/* COLORREF TreeView_OnSetBkColor(HWND hwnd, COLORREF clr) */
#define HANDLE_TVM_SETBKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF TreeView_OnSetTextColor(HWND hwnd, COLORREF clr) */
#define HANDLE_TVM_SETTEXTCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF TreeView_OnGetBkColor(HWND hwnd) */
#define HANDLE_TVM_GETBKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* COLORREF TreeView_OnGetTextColor(HWND hwnd) */
#define HANDLE_TVM_GETTEXTCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* UINT TreeView_OnSetScrollTime(HWND hwnd, UINT uTime) */
#define HANDLE_TVM_SETSCROLLTIME(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (UINT)(wParam))

/* UINT TreeView_OnGetScrollTime(HWND hwnd) */
#define HANDLE_TVM_GETSCROLLTIME(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd))

#if NTDDI_VERSION >= 0x06000000
    /* INT TreeView_OnSetBorder(HWND hwnd, DWORD dwFlags, INT xBorder, INT yBorder) */
    #define HANDLE_TVM_SETBORDER(hwnd, wParam, lParam, fn) \
        (INT)(fn)((hwnd), (DWORD)(wParam), (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam))
#endif

/* COLORREF TreeView_OnSetInsertMarkColor(HWND hwnd, COLORREF clr) */
#define HANDLE_TVM_SETINSERTMARKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF TreeView_OnGetInsertMarkColor(HWND hwnd) */
#define HANDLE_TVM_GETINSERTMARKCOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* UINT TreeView_OnGetItemState(HWND hwnd, HTREEITEM hti, UINT mask) */
#define HANDLE_TVM_GETITEMSTATE(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (HTREEITEM)(wParam), (UINT)(lParam))

/* COLORREF TreeView_OnSetLineColor(HWND hwnd, COLORREF clr) */
#define HANDLE_TVM_SETLINECOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd), (COLORREF)(lParam))

/* COLORREF TreeView_OnGetLineColor(HWND hwnd) */
#define HANDLE_TVM_GETLINECOLOR(hwnd, wParam, lParam, fn) \
    (COLORREF)(fn)((hwnd))

/* HTREEITEM TreeView_OnMapAccIDToHTREEITEM(HWND hwnd, UINT id) */
#define HANDLE_TVM_MAPACCIDTOHTREEITEM(hwnd, wParam, lParam, fn) \
    (LRESULT)(HTREEITEM)(fn)((hwnd), (UINT)(wParam))

/* UINT TreeView_OnMapHTREEITEMToAccID(HWND hwnd, HTREEITEM htreeitem) */
#define HANDLE_TVM_MAPHTREEITEMTOACCID(hwnd, wParam, lParam, fn) \
    (UINT)(fn)((hwnd), (HTREEITEM)(wParam))

#if NTDDI_VERSION >= 0x06000000
    /* DWORD TreeView_OnSetExtendedStyle(HWND hwnd, DWORD dw, DWORD mask) */
    #define HANDLE_TVM_SETEXTENDEDSTYLE(hwnd, wParam, lParam, fn) \
        (DWORD)(fn)((hwnd), (DWORD)(wParam), (DWORD)(lParam))

    /* DWORD TreeView_OnGetExtendedStyle(HWND hwnd) */
    #define HANDLE_TVM_GETEXTENDEDSTYLE(hwnd, wParam, lParam, fn) \
        (DWORD)(fn)((hwnd))
#endif

/* HTREEITEM TreeView_OnInsertItemW(HWND hwnd, LPTV_INSERTSTRUCTW lpis) */
#define HANDLE_TVM_INSERTITEMW(hwnd, wParam, lParam, fn) \
    (LRESULT)(HTREEITEM)(fn)((hwnd), (LPTV_INSERTSTRUCTW)(lParam))

#if NTDDI_VERSION >= 0x06000000
    /* BOOL TreeView_OnSetHot(HWND hwnd, HTREEITEM hitem) */
    #define HANDLE_TVM_SETHOT(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (HTREEITEM)(lParam))

    /* BOOL TreeView_OnSetAutoScrollInfo(HWND hwnd, UINT uPixPerSec, UINT uUpdateTime) */
    #define HANDLE_TVM_SETAUTOSCROLLINFO(hwnd, wParam, lParam, fn) \
        (BOOL)(fn)((hwnd), (UINT)(wParam), (UINT)(lParam))
#endif

/* BOOL TreeView_OnGetItemW(HWND hwnd, TV_ITEMW *pitem) */
#define HANDLE_TVM_GETITEMW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (TV_ITEMW *)(lParam))

/* BOOL TreeView_OnSetItemW(HWND hwnd, const TV_ITEMW *pitem) */
#define HANDLE_TVM_SETITEMW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (const TV_ITEMW *)(lParam))

/* BOOL TreeView_OnGetISearchStringW(HWND hwnd, LPWSTR lpsz) */
#define HANDLE_TVM_GETISEARCHSTRINGW(hwnd, wParam, lParam, fn) \
    (BOOL)(fn)((hwnd), (LPWSTR)(lParam))

/* HWND TreeView_OnEditLabelW(HWND hwnd, HTREEITEM hitem) */
#define HANDLE_TVM_EDITLABELW(hwnd, wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((hwnd), (HTREEITEM)(lParam))

#if NTDDI_VERSION >= 0x06000000
    /* DWORD TreeView_OnGetSelectedCount(HWND hwnd) */
    #define HANDLE_TVM_GETSELECTEDCOUNT(hwnd, wParam, lParam, fn) \
        (DWORD)(fn)((hwnd))

    /* DWORD TreeView_OnShowInfoTip(HWND hwnd, HTREEITEM hitem) */
    #define HANDLE_TVM_SHOWINFOTIP(hwnd, wParam, lParam, fn) \
        (DWORD)(fn)((hwnd), (HTREEITEM)(lParam))

    /* LRESULT TreeView_OnGetItemPartRect(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #define HANDLE_TVM_GETITEMPARTRECT(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
#endif

#ifdef _UNDOCUSER_H     /* UNDOCUMENTED */
    /* LRESULT Cls_OnDropObject(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_DROPOBJECT
    #define HANDLE_WM_DROPOBJECT(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_DROPOBJECT(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_DROPOBJECT, wParam, lParam))
    #endif

    /* LRESULT Cls_OnQueryDropObject(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_QUERYDROPOBJECT
    #define HANDLE_WM_QUERYDROPOBJECT(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_QUERYDROPOBJECT(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_QUERYDROPOBJECT, wParam, lParam))
    #endif

    /* LRESULT Cls_OnBeginDrag(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_BEGINDRAG
    #define HANDLE_WM_BEGINDRAG(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_BEGINDRAG(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_BEGINDRAG, wParam, lParam))
    #endif

    /* LRESULT Cls_OnDragLoop(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_DRAGLOOP
    #define HANDLE_WM_DRAGLOOP(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_DRAGLOOP(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_DRAGLOOP, wParam, lParam))
    #endif

    /* LRESULT Cls_OnDragSelect(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_DRAGSELECT
    #define HANDLE_WM_DRAGSELECT(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_DRAGSELECT(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_DRAGSELECT, wParam, lParam))
    #endif

    /* LRESULT Cls_OnDragMove(HWND hwnd, WPARAM wParam, LPARAM lParam) */
    #ifndef HANDLE_WM_DRAGMOVE
    #define HANDLE_WM_DRAGMOVE(hwnd, wParam, lParam, fn) \
        (LRESULT)(fn)((hwnd), (wParam), (lParam))
    #define FORWARD_WM_DRAGMOVE(hwnd, wParam, lParam, fn) \
        (LRESULT)((fn)((hwnd), WM_DRAGMOVE, wParam, lParam))
    #endif
#endif

#endif
