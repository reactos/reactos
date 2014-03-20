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

public:
    static CMenuFocusManager * AcquireManager();

    static void ReleaseManager(CMenuFocusManager * obj);

private:
    static LRESULT CALLBACK s_MsgFilterHook(INT nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam);

private:
    CMenuBand * m_currentBand;
    HWND m_currentFocus;
    HMENU m_currentMenu;
    HWND m_parentToolbar;
    HHOOK m_hMsgFilterHook;
    HHOOK m_hGetMsgHook;
    DWORD m_threadId;
    BOOL m_mouseTrackDisabled;
    WPARAM m_lastMoveFlags;
    LPARAM m_lastMovePos;
    POINT m_ptPrev;

    // TODO: make dynamic
#define MAX_RECURSE 20
    CMenuBand* m_bandStack[MAX_RECURSE];
    int m_bandCount;

    HRESULT PushToArray(CMenuBand * item);
    HRESULT PopFromArray(CMenuBand ** pItem);
    HRESULT PeekArray(CMenuBand ** pItem);

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
    HRESULT UpdateFocus(CMenuBand * newBand, HMENU popupToTrack = NULL);
    HRESULT IsTrackedWindow(HWND hWnd);

    void DisableMouseTrack(HWND enableTo, BOOL disableThis);

    LRESULT ProcessMouseMove(MSG* msg);
public:
    HRESULT PushMenu(CMenuBand * mb);
    HRESULT PopMenu(CMenuBand * mb);
    HRESULT PushTrackedPopup(CMenuBand * mb, HMENU popup);
    HRESULT PopTrackedPopup(CMenuBand * mb, HMENU popup);
};
