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

#if DBG
#   undef _ASSERT
#   define _ASSERT(x) DbgAssert(!!(x), __FILE__, __LINE__, #x)

bool DbgAssert(bool x, const char * filename, int line, const char * expr)
{
    if (!x)
    {
        char szMsg[512];
        const char *fname;

        fname = strrchr(filename, '\\');
        if (fname == NULL)
        {
            fname = strrchr(filename, '/');
        }

        if (fname == NULL)
            fname = filename;
        else
            fname++;

        sprintf(szMsg, "%s:%d: Assertion failed: %s\n", fname, line, expr);

        OutputDebugStringA(szMsg);

        __debugbreak();
    }
    return x;
}
#else
#   undef _ASSERT
#   define _ASSERT(x) (!!(x))
#endif

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

HRESULT CMenuFocusManager::PushToArray(StackEntryType type, CMenuBand * mb, HMENU hmenu)
{
    if (m_bandCount >= MAX_RECURSE)
        return E_OUTOFMEMORY;

    m_bandStack[m_bandCount].type = type;
    m_bandStack[m_bandCount].mb = mb;
    m_bandStack[m_bandCount].hmenu = hmenu;
    m_bandCount++;

    return S_OK;
}

HRESULT CMenuFocusManager::PopFromArray(StackEntryType * pType, CMenuBand ** pMb, HMENU * pHmenu)
{
    if (pType)  *pType = NoEntry;
    if (pMb)    *pMb = NULL;
    if (pHmenu) *pHmenu = NULL;

    if (m_bandCount <= 0)
        return S_FALSE;

    m_bandCount--;

    if (pType)  *pType = m_bandStack[m_bandCount].type;
    if (*pType == TrackedMenuEntry)
    {
        if (pHmenu) *pHmenu = m_bandStack[m_bandCount].hmenu;
    }
    else
    {
        if (pMb) *pMb = m_bandStack[m_bandCount].mb;
    }
    
    return S_OK;
}

CMenuFocusManager::CMenuFocusManager() :
    m_current(NULL),
    m_parent(NULL),
    m_hMsgFilterHook(NULL),
    m_hGetMsgHook(NULL),
    m_mouseTrackDisabled(FALSE),
    m_captureHwnd(0),
    m_hwndUnderMouse(NULL),
    m_entryUnderMouse(NULL),
    m_selectedMenu(NULL),
    m_selectedItem(0),
    m_selectedItemFlags(0),
    m_isLButtonDown(FALSE),
    m_movedSinceDown(FALSE),
    m_windowAtDown(NULL),
    m_bandCount(0)
{
    m_ptPrev.x = 0;
    m_ptPrev.y = 0;
    m_threadId = GetCurrentThreadId();
}

CMenuFocusManager::~CMenuFocusManager()
{
}

void CMenuFocusManager::DisableMouseTrack(HWND parent, BOOL disableThis)
{
    BOOL bDisable = FALSE;
    BOOL lastDisable = FALSE;

    int i = m_bandCount;
    while (--i >= 0)
    {
        StackEntry& entry = m_bandStack[i];

        if (entry.type != TrackedMenuEntry)
        {
            HWND hwnd;
            HRESULT hr = entry.mb->_GetTopLevelWindow(&hwnd);
            if (FAILED_UNEXPECTEDLY(hr))
                break;

            if (hwnd == parent)
            {
                lastDisable = disableThis;
                entry.mb->_DisableMouseTrack(disableThis);
                bDisable = TRUE;
            }
            else
            {
                lastDisable = bDisable;
                entry.mb->_DisableMouseTrack(bDisable);
            }
        }
    }
    m_mouseTrackDisabled = lastDisable;
}

void CMenuFocusManager::SetCapture(HWND child)
{
    if (m_captureHwnd != child)
    {
        if (child)
        {
            ::SetCapture(child);
            m_captureHwnd = child;
            TRACE("MouseTrack is now capturing %p\n", child);
        }
        else
        {
            ::ReleaseCapture();
            m_captureHwnd = NULL;
            TRACE("MouseTrack is now off\n");
        }

    }
}

HRESULT CMenuFocusManager::IsTrackedWindow(HWND hWnd, StackEntry ** pentry)
{
    if (pentry)
        *pentry = NULL;

    for (int i = m_bandCount; --i >= 0;)
    {
        StackEntry& entry = m_bandStack[i];

        if (entry.type != TrackedMenuEntry)
        {
            HRESULT hr = entry.mb->IsWindowOwner(hWnd);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            if (hr == S_OK)
            {
                if (pentry)
                    *pentry = &entry;
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

HRESULT CMenuFocusManager::IsTrackedWindowOrParent(HWND hWnd)
{
    for (int i = m_bandCount; --i >= 0;)
    {
        StackEntry& entry = m_bandStack[i];

        if (entry.type != TrackedMenuEntry)
        {
            HRESULT hr = entry.mb->IsWindowOwner(hWnd);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            if (hr == S_OK)
                return S_OK;
            if (entry.mb->_IsPopup() == S_OK)
            {
                CComPtr<IUnknown> site;
                CComPtr<IOleWindow> pw;
                hr = entry.mb->GetSite(IID_PPV_ARG(IUnknown, &site));
                if (FAILED_UNEXPECTEDLY(hr))
                    continue;
                hr = IUnknown_QueryService(site, SID_SMenuBandParent, IID_PPV_ARG(IOleWindow, &pw));
                if (FAILED_UNEXPECTEDLY(hr))
                    continue;

                HWND hParent;
                if (pw->GetWindow(&hParent) == S_OK && hParent == hWnd)
                    return S_OK;
            }
        }
    }

    return S_FALSE;
}

LRESULT CMenuFocusManager::ProcessMouseMove(MSG* msg)
{
    HWND child;
    int iHitTestResult = -1;

    POINT pt2 = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
    ClientToScreen(msg->hwnd, &pt2);

    // Don't do anything if the mouse has not been moved
    POINT pt = msg->pt;
    if (pt.x == m_ptPrev.x && pt.y == m_ptPrev.y)
        return TRUE;

    // Don't do anything if another window is capturing the mouse.
    HWND cCapture = ::GetCapture();
    if (cCapture && cCapture != m_captureHwnd && m_current->type != TrackedMenuEntry)
        return TRUE;


    m_movedSinceDown = TRUE;

    m_ptPrev = pt;

    child = WindowFromPoint(pt);

    StackEntry * entry = NULL;
    if (IsTrackedWindow(child, &entry) == S_OK)
    {
        TRACE("MouseMove %d\n", m_isLButtonDown);
    }

    BOOL isTracking = FALSE;
    if (entry && (entry->type == MenuBarEntry || m_current->type != TrackedMenuEntry))
    {
        ScreenToClient(child, &pt);
        iHitTestResult = SendMessageW(child, TB_HITTEST, 0, (LPARAM) &pt);
        isTracking = entry->mb->_IsTracking();

        if (SendMessage(child, WM_USER_ISTRACKEDITEM, iHitTestResult, 0) == S_FALSE)
        {
            TRACE("Hot item tracking detected a change (capture=%p / cCapture=%p)...\n", m_captureHwnd, cCapture);
            DisableMouseTrack(NULL, FALSE);
            if (isTracking && iHitTestResult >= 0 && m_current->type == TrackedMenuEntry)
                SendMessage(entry->hwnd, WM_CANCELMODE, 0, 0);
            PostMessage(child, WM_USER_CHANGETRACKEDITEM, iHitTestResult, MAKELPARAM(isTracking, TRUE));
            if (m_current->type == TrackedMenuEntry)
                return FALSE;
        }
    }

    if (m_entryUnderMouse != entry)
    {
        // Mouse moved away from a tracked window
        if (m_entryUnderMouse)
        {
            m_entryUnderMouse->mb->_ChangeHotItem(NULL, -1, HICF_MOUSE);
        }
        if (cCapture == m_captureHwnd)
            SetCapture(NULL);
    }

    if (m_hwndUnderMouse != child)
    {
        if (entry)
        {
            // Mouse moved to a tracked window
            if (m_current->type == MenuPopupEntry)
            {
                ScreenToClient(child, &pt2);
                SendMessage(child, WM_MOUSEMOVE, msg->wParam, MAKELPARAM(pt2.x, pt2.y));
            }
        }

        m_hwndUnderMouse = child;
        m_entryUnderMouse = entry;
    }

    if (m_current->type == MenuPopupEntry)
    {
        HWND parent = GetAncestor(child, GA_ROOT);
        DisableMouseTrack(parent, FALSE);
    }

    return TRUE;
}

LRESULT CMenuFocusManager::ProcessMouseDown(MSG* msg)
{
    HWND child;
    int iHitTestResult = -1;

    TRACE("ProcessMouseDown %d %d %d\n", msg->message, msg->wParam, msg->lParam);

    // Don't do anything if another window is capturing the mouse.
    HWND cCapture = ::GetCapture();
    if (cCapture && cCapture != m_captureHwnd && m_current->type != TrackedMenuEntry)
    {
        TRACE("Foreign capture active.\n");
        return TRUE;
    }

    POINT pt = msg->pt;

    child = WindowFromPoint(pt);

    StackEntry * entry = NULL;
    if (IsTrackedWindow(child, &entry) != S_OK)
    {
        TRACE("Foreign window detected.\n");
        return TRUE;
    }

    TRACE("MouseDown %d\n", m_isLButtonDown);

    if (entry->type == MenuBarEntry)
    {
        if (entry != m_current)
        {
            TRACE("Menubar with popup active.\n");
            return TRUE;
        }
    }

    if (entry)
    {
        ScreenToClient(child, &pt);
        iHitTestResult = SendMessageW(child, TB_HITTEST, 0, (LPARAM) &pt);

        if (iHitTestResult >= 0)
        {
            TRACE("MouseDown send %d\n", iHitTestResult);
            entry->mb->_MenuBarMouseDown(child, iHitTestResult);
        }
    }

    msg->message = WM_NULL;

    m_isLButtonDown = TRUE;
    m_movedSinceDown = FALSE;
    m_windowAtDown = child;

    TRACE("MouseDown end %d\n", m_isLButtonDown);

    return TRUE;
}

LRESULT CMenuFocusManager::ProcessMouseUp(MSG* msg)
{
    HWND child;
    int iHitTestResult = -1;

    TRACE("ProcessMouseUp %d %d %d\n", msg->message, msg->wParam, msg->lParam);

    // Don't do anything if another window is capturing the mouse.
    HWND cCapture = ::GetCapture();
    if (cCapture && cCapture != m_captureHwnd && m_current->type != TrackedMenuEntry)
        return TRUE;

    if (!m_isLButtonDown)
        return TRUE;

    m_isLButtonDown = FALSE;

    POINT pt = msg->pt;

    child = WindowFromPoint(pt);

    StackEntry * entry = NULL;
    if (IsTrackedWindow(child, &entry) != S_OK)
        return TRUE;

    TRACE("MouseUp %d\n", m_isLButtonDown);

    if (entry)
    {
        ScreenToClient(child, &pt);
        iHitTestResult = SendMessageW(child, TB_HITTEST, 0, (LPARAM) &pt);

        if (iHitTestResult >= 0)
        {
            TRACE("MouseUp send %d\n", iHitTestResult);
            entry->mb->_MenuBarMouseUp(child, iHitTestResult);
        }
    }

    return TRUE;
}

LRESULT CMenuFocusManager::MsgFilterHook(INT nCode, WPARAM hookWParam, LPARAM hookLParam)
{
    if (nCode < 0)
        return CallNextHookEx(m_hMsgFilterHook, nCode, hookWParam, hookLParam);

    if (nCode == MSGF_MENU)
    {
        BOOL callNext = TRUE;
        MSG* msg = reinterpret_cast<MSG*>(hookLParam);

        switch (msg->message)
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (m_menuBar && m_current->type == TrackedMenuEntry)
            {
                POINT pt = msg->pt;
                HWND child = WindowFromPoint(pt);
                BOOL hoveringMenuBar = m_menuBar->mb->IsWindowOwner(child) == S_OK;
                if (hoveringMenuBar)
                {
                    m_menuBar->mb->_BeforeCancelPopup();
                }
            }
            break;
        case WM_MOUSEMOVE:
            callNext = ProcessMouseMove(msg);
            break;
        case WM_INITMENUPOPUP:
            TRACE("WM_INITMENUPOPUP %p %p\n", msg->wParam, msg->lParam);
            m_selectedMenu = reinterpret_cast<HMENU>(msg->lParam);
            m_selectedItem = -1;
            m_selectedItemFlags = 0;
            break;
        case WM_MENUSELECT:
            TRACE("WM_MENUSELECT %p %p\n", msg->wParam, msg->lParam);
            m_selectedMenu = reinterpret_cast<HMENU>(msg->lParam);
            m_selectedItem = GET_X_LPARAM(msg->wParam);
            m_selectedItemFlags = HIWORD(msg->wParam);
            break;
        case WM_KEYDOWN:
            switch (msg->wParam)
            {
            case VK_LEFT:
                if (m_current->hmenu == m_selectedMenu)
                {
                    m_parent->mb->_MenuItemHotTrack(VK_LEFT);
                }
                break;
            case VK_RIGHT:
                if (m_selectedItem < 0 || !(m_selectedItemFlags & MF_POPUP))
                {
                    m_parent->mb->_MenuItemHotTrack(VK_RIGHT);
                }
                break;
            }
            break;
        }

        if (!callNext)
            return 1;
    }

    return CallNextHookEx(m_hMsgFilterHook, nCode, hookWParam, hookLParam);
}

LRESULT CMenuFocusManager::GetMsgHook(INT nCode, WPARAM hookWParam, LPARAM hookLParam)
{
    BOOL isLButton = FALSE;
    if (nCode < 0)
        return CallNextHookEx(m_hGetMsgHook, nCode, hookWParam, hookLParam);
    
    if (nCode == HC_ACTION)
    {
        BOOL callNext = TRUE;
        MSG* msg = reinterpret_cast<MSG*>(hookLParam);
        POINT pt = msg->pt;

        switch (msg->message)
        {
        case WM_NCLBUTTONDOWN:
        case WM_LBUTTONDOWN:
            isLButton = TRUE;

            // fallthrough;
        case WM_NCRBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (m_menuBar && m_current->type == MenuPopupEntry)
            {
                POINT pt = msg->pt;
                HWND child = WindowFromPoint(pt);
                BOOL hoveringMenuBar = m_menuBar->mb->IsWindowOwner(child) == S_OK;
                if (hoveringMenuBar)
                {
                    m_current->mb->_MenuItemHotTrack(MPOS_FULLCANCEL);
                    break;
                }
            }

            if (m_current->type == MenuPopupEntry)
            {
                HWND child = WindowFromPoint(pt);

                if (IsTrackedWindowOrParent(child) != S_OK)
                {
                    SetCapture(NULL);
                    m_current->mb->_MenuItemHotTrack(MPOS_FULLCANCEL);
                    break;
                }
            }
            
            if (isLButton)
            {
                ProcessMouseDown(msg);
            }
            break;
        case WM_NCLBUTTONUP:
        case WM_LBUTTONUP:
            ProcessMouseUp(msg);
            break;
        case WM_MOUSEMOVE:
            callNext = ProcessMouseMove(msg);
            break;
        case WM_MOUSELEAVE:
            callNext = ProcessMouseMove(msg);
            //callNext = ProcessMouseLeave(msg);
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            if (m_current->type == MenuPopupEntry)
            {
                DisableMouseTrack(m_current->hwnd, TRUE);
                switch (msg->wParam)
                {
                case VK_ESCAPE:
                case VK_MENU:
                case VK_LMENU:
                case VK_RMENU:
                    m_current->mb->_MenuItemHotTrack(MPOS_FULLCANCEL);
                    break;
                case VK_RETURN:
                    m_current->mb->_MenuItemHotTrack(MPOS_EXECUTE);
                    break;
                case VK_LEFT:
                    m_current->mb->_MenuItemHotTrack(VK_LEFT);
                    break;
                case VK_RIGHT:
                    m_current->mb->_MenuItemHotTrack(VK_RIGHT);
                    break;
                case VK_UP:
                    m_current->mb->_MenuItemHotTrack(VK_UP);
                    break;
                case VK_DOWN:
                    m_current->mb->_MenuItemHotTrack(VK_DOWN);
                    break;
                }
                msg->message = WM_NULL;
                msg->lParam = 0;
                msg->wParam = 0;
            }
            break;
        }

        if (!callNext)
            return 1;
    }

    return CallNextHookEx(m_hGetMsgHook, nCode, hookWParam, hookLParam);
}

HRESULT CMenuFocusManager::PlaceHooks()
{
    if (m_hMsgFilterHook)
    {
        WARN("GETMESSAGE hook already placed!\n");
        return S_OK;
    }
    if (m_hMsgFilterHook)
    {
        WARN("MSGFILTER hook already placed!\n");
        return S_OK;
    }
    if (m_current->type == TrackedMenuEntry)
    {
        TRACE("Entering MSGFILTER hook...\n");
        m_hMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, s_MsgFilterHook, NULL, m_threadId);
    }
    else
    {
        TRACE("Entering GETMESSAGE hook...\n");
        m_hGetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, s_GetMsgHook, NULL, m_threadId);
    }
    return S_OK;
}

HRESULT CMenuFocusManager::RemoveHooks()
{
    TRACE("Removing all hooks...\n");
    if (m_hMsgFilterHook)
        UnhookWindowsHookEx(m_hMsgFilterHook);
    if (m_hGetMsgHook)
        UnhookWindowsHookEx(m_hGetMsgHook);
    m_hMsgFilterHook = NULL;
    m_hGetMsgHook = NULL;
    return S_OK;
}

HRESULT CMenuFocusManager::UpdateFocus()
{
    HRESULT hr;
    StackEntry * old = m_current;

    TRACE("UpdateFocus\n");

    if (old)
        SetCapture(NULL);

    if (m_bandCount > 0)
        m_current = &(m_bandStack[m_bandCount - 1]);
    else
        m_current = NULL;

    if (m_current && m_current->type != TrackedMenuEntry)
    {
        hr = m_current->mb->_GetTopLevelWindow(&(m_current->hwnd));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_bandCount >= 2)
    {
        m_parent = &(m_bandStack[m_bandCount - 2]);
        _ASSERT(m_parent->type != TrackedMenuEntry);
    }
    else
    {
        m_parent = NULL;
    }

    if (m_bandCount >= 1 && m_bandStack[0].type == MenuBarEntry)
    {
        m_menuBar = &(m_bandStack[0]);
    }
    else
    {
        m_menuBar = NULL;
    }

    if (old && (!m_current || old->type != m_current->type))
    {
        if (m_current && m_current->type != TrackedMenuEntry)
        {
            DisableMouseTrack(m_current->hwnd, FALSE);
        }

        hr = RemoveHooks();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_current && (!old || old->type != m_current->type))
    {
        hr = PlaceHooks();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_parent)
    {
        DisableMouseTrack(m_parent->hwnd, TRUE);
    }

    if ((m_current && m_current->type == MenuPopupEntry) &&
        (!m_parent || m_parent->type == MenuBarEntry))
    {
        // When the mouse moves, it should set itself to the proper band
        SetCapture(m_current->hwnd);

        if (old && old->type == TrackedMenuEntry)
        {
            // FIXME: Debugging code, probably not right
            POINT pt2;
            RECT rc2;
            GetCursorPos(&pt2);
            ScreenToClient(m_current->hwnd, &pt2);
            GetClientRect(m_current->hwnd, &rc2);
            if (PtInRect(&rc2, pt2))
                SendMessage(m_current->hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt2.x, pt2.y));
            else
                SendMessage(m_current->hwnd, WM_MOUSELEAVE, 0, 0);
        }
    }

    _ASSERT(!m_parent || m_parent->type != TrackedMenuEntry);

    return S_OK;
}

HRESULT CMenuFocusManager::PushMenuBar(CMenuBand * mb)
{
    DbgPrint("PushMenuBar %p\n", mb);

    mb->AddRef();

    _ASSERT(m_bandCount == 0);

    HRESULT hr = PushToArray(MenuBarEntry, mb, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return UpdateFocus();
}

HRESULT CMenuFocusManager::PushMenuPopup(CMenuBand * mb)
{
    DbgPrint("PushTrackedPopup %p\n", mb);

    mb->AddRef();

    _ASSERT(!m_current || m_current->type != TrackedMenuEntry);

    HRESULT hr = PushToArray(MenuPopupEntry, mb, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = UpdateFocus();

    if (m_parent && m_parent->type != TrackedMenuEntry)
    {
        m_parent->mb->_SetChildBand(mb);
        mb->_SetParentBand(m_parent->mb);
    }

    return hr;
}

HRESULT CMenuFocusManager::PushTrackedPopup(HMENU popup)
{
    DbgPrint("PushTrackedPopup %p\n", popup);

    _ASSERT(m_bandCount > 0);
    _ASSERT(!m_current || m_current->type != TrackedMenuEntry);

    HRESULT hr = PushToArray(TrackedMenuEntry, NULL, popup);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    DbgPrint("PushTrackedPopup %p\n", popup);
    m_selectedMenu = popup;
    m_selectedItem = -1;
    m_selectedItemFlags = 0;

    return UpdateFocus();
}

HRESULT CMenuFocusManager::PopMenuBar(CMenuBand * mb)
{
    StackEntryType type;
    CMenuBand * mbc;
    HRESULT hr;

    DbgPrint("PopMenuBar %p\n", mb);

    if (m_current == m_entryUnderMouse)
    {
        m_entryUnderMouse = NULL;
    }

    hr = PopFromArray(&type, &mbc, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        UpdateFocus();
        return hr;
    }

    _ASSERT(type == MenuBarEntry);
    if (type != MenuBarEntry)
        return E_FAIL;

    if (!mbc)
        return E_FAIL;

    mbc->_SetParentBand(NULL);

    mbc->Release();

    hr = UpdateFocus();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_current)
    {
        _ASSERT(m_current->type != TrackedMenuEntry);
        m_current->mb->_SetChildBand(NULL);
    }

    return S_OK;
}

HRESULT CMenuFocusManager::PopMenuPopup(CMenuBand * mb)
{
    StackEntryType type;
    CMenuBand * mbc;
    HRESULT hr;

    DbgPrint("PopMenuPopup %p\n", mb);

    if (m_current == m_entryUnderMouse)
    {
        m_entryUnderMouse = NULL;
    }

    hr = PopFromArray(&type, &mbc, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        UpdateFocus();
        return hr;
    }

    _ASSERT(type == MenuPopupEntry);
    if (type != MenuPopupEntry)
        return E_FAIL;

    if (!mbc)
        return E_FAIL;

    mbc->_SetParentBand(NULL);

    mbc->Release();

    hr = UpdateFocus();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_current)
    {
        _ASSERT(m_current->type != TrackedMenuEntry);
        m_current->mb->_SetChildBand(NULL);
    }

    return S_OK;
}

HRESULT CMenuFocusManager::PopTrackedPopup(HMENU popup)
{
    StackEntryType type;
    HMENU hmenu;
    HRESULT hr;

    DbgPrint("PopTrackedPopup %p\n", popup);

    hr = PopFromArray(&type, NULL, &hmenu);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        UpdateFocus();
        return hr;
    }

    _ASSERT(type == TrackedMenuEntry);
    if (type != TrackedMenuEntry)
        return E_FAIL;

    if (hmenu != popup)
        return E_FAIL;

    hr = UpdateFocus();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}