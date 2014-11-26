/*
* Shell Menu Band
*
* Copyright 2014 David Quintana
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
*/
#pragma once

class CMenuBand;

class CMenuFocusManager :
    public CComCoClass<CMenuFocusManager>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>
{
private:
    static DWORD TlsIndex;

    static CMenuFocusManager * GetManager();

    enum StackEntryType
    {
        NoEntry,
        MenuBarEntry,
        MenuPopupEntry,
        TrackedMenuEntry
    };

    struct StackEntry
    {
        StackEntryType type;
        CMenuBand *    mb;
        HMENU          hmenu;
        HWND           hwnd;
    };

public:
    static CMenuFocusManager * AcquireManager();

    static void ReleaseManager(CMenuFocusManager * obj);

private:
    static LRESULT CALLBACK s_MsgFilterHook(INT nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam);

private:
    StackEntry * m_current;
    StackEntry * m_parent;
    StackEntry * m_menuBar;

    HHOOK m_hMsgFilterHook;
    HHOOK m_hGetMsgHook;
    DWORD m_threadId;

    BOOL m_mouseTrackDisabled;

    POINT m_ptPrev;

    HWND m_captureHwnd;

    HWND m_hwndUnderMouse;
    StackEntry * m_entryUnderMouse;

    HMENU m_selectedMenu;
    INT   m_selectedItem;
    DWORD m_selectedItemFlags;

    BOOL m_isLButtonDown;
    BOOL m_movedSinceDown;
    HWND m_windowAtDown;

    // TODO: make dynamic
#define MAX_RECURSE 20
    StackEntry m_bandStack[MAX_RECURSE];
    int m_bandCount;

    HRESULT PushToArray(StackEntryType type, CMenuBand * mb, HMENU hmenu);
    HRESULT PopFromArray(StackEntryType * pType, CMenuBand ** pMb, HMENU * pHmenu);

protected:
    CMenuFocusManager();
    ~CMenuFocusManager();

public:

    DECLARE_NOT_AGGREGATABLE(CMenuFocusManager)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CMenuFocusManager)
    END_COM_MAP()

private:
    LRESULT GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam);
    LRESULT MsgFilterHook(INT nCode, WPARAM wParam, LPARAM lParam);
    HRESULT PlaceHooks();
    HRESULT RemoveHooks();
    HRESULT UpdateFocus();
    HRESULT IsTrackedWindow(HWND hWnd, StackEntry ** pentry = NULL);
    HRESULT IsTrackedWindowOrParent(HWND hWnd);

    void DisableMouseTrack(HWND parent, BOOL disableThis);
    void SetCapture(HWND child);

    LRESULT ProcessMouseMove(MSG* msg);
    LRESULT ProcessMouseDown(MSG* msg);
    LRESULT ProcessMouseUp(MSG* msg);
public:
    HRESULT PushMenuBar(CMenuBand * mb);
    HRESULT PushMenuPopup(CMenuBand * mb);
    HRESULT PushTrackedPopup(HMENU popup);
    HRESULT PopMenuBar(CMenuBand * mb);
    HRESULT PopMenuPopup(CMenuBand * mb);
    HRESULT PopTrackedPopup(HMENU popup);
};
