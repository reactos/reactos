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
#include "precomp.h"
#include <windowsx.h>
#include <commoncontrols.h>
#include <shlwapi_undoc.h>

#include "CMenuFocusManager.h"
#include "CMenuToolbars.h"
#include "CMenuBand.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMenuFocus);

DWORD CMenuFocusManager::TlsIndex = 0;

CMenuFocusManager * CMenuFocusManager::GetManager()
{
    return reinterpret_cast<CMenuFocusManager *>(TlsGetValue(TlsIndex));
}

CMenuFocusManager * CMenuFocusManager::AcquireManager()
{
    CMenuFocusManager * obj = NULL;

    if (!TlsIndex)
    {
        if ((TlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
            return NULL;
    }

    obj = GetManager();

    if (!obj)
    {
        obj = new CComObject<CMenuFocusManager>();
        TlsSetValue(TlsIndex, obj);
    }

    obj->AddRef();

    return obj;
}

void CMenuFocusManager::ReleaseManager(CMenuFocusManager * obj)
{
    if (!obj->Release())
    {
        TlsSetValue(TlsIndex, NULL);
    }
}

LRESULT CALLBACK CMenuFocusManager::s_MsgFilterHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
    return GetManager()->MsgFilterHook(nCode, wParam, lParam);
}

LRESULT CALLBACK CMenuFocusManager::s_GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
    return GetManager()->GetMsgHook(nCode, wParam, lParam);
}

HRESULT CMenuFocusManager::PushToArray(CMenuBand * item)
{
    if (m_bandCount >= MAX_RECURSE)
        return E_OUTOFMEMORY;

    m_bandStack[m_bandCount++] = item;
    return S_OK;
}

HRESULT CMenuFocusManager::PopFromArray(CMenuBand ** pItem)
{
    if (pItem)
        *pItem = NULL;

    if (m_bandCount <= 0)
        return S_FALSE;

    m_bandCount--;

    if (pItem)
        *pItem = m_bandStack[m_bandCount];

    m_bandStack[m_bandCount] = NULL;

    return S_OK;
}

HRESULT CMenuFocusManager::PeekArray(CMenuBand ** pItem)
{
    if (!pItem)
        return E_FAIL;

    *pItem = NULL;

    if (m_bandCount <= 0)
        return S_FALSE;

    *pItem = m_bandStack[m_bandCount - 1];

    return S_OK;
}

CMenuFocusManager::CMenuFocusManager() :
    m_currentBand(NULL),
    m_currentFocus(NULL),
    m_currentMenu(NULL),
    m_parentToolbar(NULL),
    m_hMsgFilterHook(NULL),
    m_hGetMsgHook(NULL),
    m_mouseTrackDisabled(FALSE),
    m_lastMoveFlags(0),
    m_lastMovePos(0),
    m_bandCount(0)
{
    m_threadId = GetCurrentThreadId();
}

CMenuFocusManager::~CMenuFocusManager()
{
}

void CMenuFocusManager::DisableMouseTrack(HWND enableTo, BOOL disableThis)
{
    BOOL bDisable = FALSE;

    int i = m_bandCount;
    while (--i >= 0)
    {
        CMenuBand * band = m_bandStack[i];
        
        HWND hwnd;
        HRESULT hr = band->_GetTopLevelWindow(&hwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            break;

        if (hwnd == enableTo)
        {
            band->_DisableMouseTrack(disableThis);
            bDisable = TRUE;
        }
        else
        {
            band->_DisableMouseTrack(bDisable);
        }
    }

    if (m_mouseTrackDisabled == bDisable)
    {
        if (bDisable)
        {
            SetCapture(m_currentFocus);
        }
        else
            ReleaseCapture();

        m_mouseTrackDisabled = bDisable;
    }
}

HRESULT CMenuFocusManager::IsTrackedWindow(HWND hWnd)
{
    int i = m_bandCount;
    while (--i >= 0)
    {
        CMenuBand * band = m_bandStack[i];

        HWND hwnd;
        HRESULT hr = band->_GetTopLevelWindow(&hwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (hwnd == hWnd)
        {
            return band->_IsPopup();
        }
    }

    return S_FALSE;
}

LRESULT CMenuFocusManager::ProcessMouseMove(MSG* msg)
{
    HWND parent;
    HWND child;
    POINT pt;
    int iHitTestResult;

    pt = msg->pt;

    parent = WindowFromPoint(pt);

    ScreenToClient(parent, &pt);

    child = ChildWindowFromPoint(parent, pt);

    if (child != m_parentToolbar)
        return TRUE;

    ScreenToClient(m_parentToolbar, &msg->pt);

    /* Don't do anything if the mouse has not been moved */
    if (msg->pt.x == m_ptPrev.x && msg->pt.y == m_ptPrev.y)
        return TRUE;

    m_ptPrev = msg->pt;

    iHitTestResult = SendMessageW(m_parentToolbar, TB_HITTEST, 0, (LPARAM) &msg->pt);

    /* Make sure that iHitTestResult is one of the menu items and that it is not the current menu item */
    if (iHitTestResult >= 0)
    {
        HWND hwndToolbar = m_parentToolbar;
        if (SendMessage(hwndToolbar, WM_USER_ISTRACKEDITEM, iHitTestResult, 0))
        {
            DbgPrint("Hot item tracking detected a change...\n");
            if (m_currentMenu)
                SendMessage(m_currentFocus, WM_CANCELMODE, 0, 0);
            else
                m_currentBand->_MenuItemHotTrack(MPOS_CANCELLEVEL);
            DbgPrint("Active popup cancelled, notifying of change...\n");
            PostMessage(hwndToolbar, WM_USER_CHANGETRACKEDITEM, iHitTestResult, iHitTestResult);
            return FALSE;
        }
    }

    return TRUE;
}

LRESULT CMenuFocusManager::MsgFilterHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(m_hMsgFilterHook, nCode, wParam, lParam);

    if (nCode == MSGF_MENU)
    {
        BOOL callNext = TRUE;
        MSG* msg = reinterpret_cast<MSG*>(lParam);

        // Do whatever is necessary here

        switch (msg->message)
        {
        case WM_MOUSEMOVE:
            callNext = ProcessMouseMove(msg);
            break;
        }

        if (!callNext)
            return 0;
    }

    return CallNextHookEx(m_hMsgFilterHook, nCode, wParam, lParam);
}

LRESULT CMenuFocusManager::GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(m_hGetMsgHook, nCode, wParam, lParam);

    LPARAM pos = (LPARAM) GetMessagePos();

    if (nCode == HC_ACTION)
    {
        BOOL callNext = TRUE;
        MSG* msg = reinterpret_cast<MSG*>(lParam);

        // Do whatever is necessary here

        switch (msg->message)
        {
        case WM_CLOSE:
            break;

        case WM_NCLBUTTONDOWN:
        case WM_LBUTTONDOWN:
        {
            POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };

            HWND window = GetAncestor(WindowFromPoint(pt), GA_ROOT);

            if (IsTrackedWindow(window) != S_OK)
            {
                DisableMouseTrack(NULL, FALSE);
                m_currentBand->_MenuItemHotTrack(MPOS_FULLCANCEL);
            }

            break;
        }
        case WM_MOUSEMOVE:
            if (m_lastMoveFlags != wParam || m_lastMovePos != pos)
            {
                m_lastMoveFlags = wParam;
                m_lastMovePos = pos;

                POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };

                HWND window = WindowFromPoint(pt);

                if (IsTrackedWindow(window) == S_OK)
                {
                    DisableMouseTrack(window, FALSE);
                }
                else
                {
                    DisableMouseTrack(NULL, FALSE);
                }
            }
            callNext = ProcessMouseMove(msg);
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            //if (!m_currentMenu)
            {
                DisableMouseTrack(m_currentFocus, TRUE);
                switch (msg->wParam)
                {
                case VK_MENU:
                case VK_LMENU:
                case VK_RMENU:
                    m_currentBand->_MenuItemHotTrack(MPOS_FULLCANCEL);
                    break;
                case VK_LEFT:
                    m_currentBand->_MenuItemHotTrack(MPOS_SELECTLEFT);
                    break;
                case VK_RIGHT:
                    m_currentBand->_MenuItemHotTrack(MPOS_SELECTRIGHT);
                    break;
                case VK_UP:
                    m_currentBand->_MenuItemHotTrack(VK_UP);
                    break;
                case VK_DOWN:
                    m_currentBand->_MenuItemHotTrack(VK_DOWN);
                    break;
                }
            }
            break;
        }

        if (!callNext)
            return 0;
    }

    return CallNextHookEx(m_hGetMsgHook, nCode, wParam, lParam);
}

HRESULT CMenuFocusManager::PlaceHooks()
{
    //SetCapture(window);
    if (m_currentMenu)
    {
        DbgPrint("Entering MSGFILTER hook...\n");
        m_hMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, s_MsgFilterHook, NULL, m_threadId);
    }
    else
    {
        DbgPrint("Entering GETMESSAGE hook...\n");
        m_hGetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, s_GetMsgHook, NULL, m_threadId);
    }
    return S_OK;
}

HRESULT CMenuFocusManager::RemoveHooks()
{
    DbgPrint("Removing all hooks...\n");
    if (m_hMsgFilterHook)
        UnhookWindowsHookEx(m_hMsgFilterHook);
    if (m_hGetMsgHook)
        UnhookWindowsHookEx(m_hGetMsgHook);
    m_hMsgFilterHook = NULL;
    m_hGetMsgHook = NULL;
    return S_OK;
}

HRESULT CMenuFocusManager::UpdateFocus(CMenuBand * newBand, HMENU popupToTrack)
{
    HRESULT hr;
    HWND newFocus = NULL;
    HWND oldFocus = m_currentFocus;
    HMENU oldMenu = m_currentMenu;

    if (newBand)
    {
        hr = newBand->_GetTopLevelWindow(&newFocus);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    m_currentBand = newBand;
    m_currentMenu = popupToTrack;
    m_currentFocus = newFocus;
    m_parentToolbar = NULL;
    if (popupToTrack)
    {
        m_currentBand->GetWindow(&m_parentToolbar);
    }
    else if (m_bandCount >= 2)
    {
        m_bandStack[m_bandCount - 2]->GetWindow(&m_parentToolbar);
    }

    if (oldFocus && (!newFocus || (oldMenu != popupToTrack)))
    {
        DisableMouseTrack(NULL, FALSE);

        hr = RemoveHooks();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    
    if (newFocus && (!oldFocus || (oldMenu != popupToTrack)))
    {
        hr = PlaceHooks();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }


    return S_OK;
}

HRESULT CMenuFocusManager::PushMenu(CMenuBand * mb)
{
    HRESULT hr;

    CMenuBand * mbParent = m_currentBand;

    hr = PushToArray(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (mbParent)
    {
        mbParent->_SetChildBand(mb);
        mb->_SetParentBand(mbParent);
    }

    return UpdateFocus(mb);
}

HRESULT CMenuFocusManager::PopMenu(CMenuBand * mb)
{
    CMenuBand * mbc;
    HRESULT hr;

    if (m_currentBand)
    {
        m_currentBand->_SetParentBand(NULL);
    }

    HWND newFocus;
    hr = mb->_GetTopLevelWindow(&newFocus);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    DbgPrint("Trying to pop %08p, hwnd=%08x\n", mb, newFocus);

    do {
        hr = PopFromArray(&mbc);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            UpdateFocus(NULL);
            return hr;
        }
    }
    while (mbc && mb != mbc);

    if (!mbc)
        return E_FAIL;
    
    hr = PeekArray(&mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = UpdateFocus(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (mb)
    {
        mb->_SetChildBand(NULL);
    }

    return S_OK;
}

HRESULT CMenuFocusManager::PushTrackedPopup(CMenuBand * mb, HMENU popup)
{
    HRESULT hr;

    hr = PushToArray(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return UpdateFocus(mb, popup);
}

HRESULT CMenuFocusManager::PopTrackedPopup(CMenuBand * mb, HMENU popup)
{
    CMenuBand * mbc;
    HRESULT hr;

    HWND newFocus;
    hr = mb->_GetTopLevelWindow(&newFocus);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    DbgPrint("Trying to pop %08p, hwnd=%08x\n", mb, newFocus);

    do {
        hr = PopFromArray(&mbc);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            UpdateFocus(NULL);
            return hr;
        }
    } while (mbc && mb != mbc);

    if (!mbc)
        return E_FAIL;

    hr = PeekArray(&mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = UpdateFocus(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}