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
#include <CommonControls.h>
#include <shlwapi_undoc.h>

#include "CMenuFocusManager.h"
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
        return E_FAIL;

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
    if (hWnd == m_currentFocus)
        return S_OK;

    int i = m_bandCount - 1;
    while (--i >= 0)
    {
        CMenuBand * band = m_bandStack[i];

        HWND hwnd;
        HRESULT hr = band->_GetTopLevelWindow(&hwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (hwnd == hWnd)
            return S_OK;
    }

    return S_FALSE;
}

LRESULT CMenuFocusManager::GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(m_hHook, nCode, wParam, lParam);

    DWORD pos = GetMessagePos();

    if (nCode == HC_ACTION)
    {
        BOOL callNext = TRUE;
        MSG* msg = reinterpret_cast<MSG*>(lParam);

        // Do whatever is necessary here

        switch (msg->message)
        {
        case WM_ACTIVATE: // does not trigger
            ActivationChange(msg->hwnd);
        case WM_CLOSE:
            break;
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
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
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
            break;
        case WM_CHAR:
            //if (msg->wParam >= 'a' && msg->wParam <= 'z')
            //{
            //    callNext = FALSE;
            //    PostMessage(m_currentFocus, WM_SYSCHAR, wParam, lParam);
            //}
            break;
        }

        if (!callNext)
            return 0;
    }

    return CallNextHookEx(m_hHook, nCode, wParam, lParam);
}

HRESULT CMenuFocusManager::PlaceHooks(HWND window)
{
    //SetCapture(window);
    m_hHook = SetWindowsHookEx(WH_GETMESSAGE, s_GetMsgHook, NULL, m_threadId);
    return S_OK;
}

HRESULT CMenuFocusManager::RemoveHooks(HWND window)
{
    UnhookWindowsHookEx(m_hHook);
    //ReleaseCapture();
    return S_OK;
}

HRESULT CMenuFocusManager::ActivationChange(HWND newHwnd)
{
    HRESULT hr;
    CMenuBand * newBand = NULL;
    
    CMenuBand * band;
    PeekArray(&band);

    while (m_bandCount >= 0)
    {
        HWND hwnd;
        hr = band->_GetTopLevelWindow(&hwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (hwnd == newHwnd)
        {
            newBand = band;
            break;
        }
        else
        {
            PopFromArray(NULL);
            PeekArray(&band);
        }
    }

    return UpdateFocus(newBand);
}

HRESULT CMenuFocusManager::UpdateFocus(CMenuBand * newBand)
{
    HRESULT hr;
    HWND newFocus;

    if (newBand == NULL)
    {
        DisableMouseTrack(NULL, FALSE);

        hr = RemoveHooks(m_currentFocus);
        m_currentFocus = NULL;
        m_currentBand = NULL;
        return S_OK;
    }

    hr = newBand->_GetTopLevelWindow(&newFocus);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!m_currentBand)
    {
        hr = PlaceHooks(newFocus);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    CHAR title[1024];
    GetWindowTextA(newFocus, title, 1024);

    DbgPrint("Focus is now at %08p, hwnd=%08x, title='%s'. m_bandCount=%d\n", newBand, newFocus, title, m_bandCount);

    m_currentFocus = newFocus;
    m_currentBand = newBand;

    return S_OK;
}

HRESULT CMenuFocusManager::PushMenu(CMenuBand * mb)
{
    HRESULT hr;

    hr = PushToArray(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return UpdateFocus(mb);
}

HRESULT CMenuFocusManager::PopMenu(CMenuBand * mb)
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
            mbc = NULL;
            return hr;
        }
    }
    while (mbc && mb != mbc);
    
    hr = PeekArray(&mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = UpdateFocus(mb);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!mbc)
        return E_FAIL;

    return S_OK;
}
