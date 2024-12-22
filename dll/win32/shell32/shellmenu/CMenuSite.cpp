/*
 * Shell Menu Site
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
#include "shellmenu.h"
#include <atlwin.h>
#include <shlwapi_undoc.h>

#include "CMenuSite.h"

CMenuSite::CMenuSite() :
    m_DeskBarSite(NULL),
    m_BandObject(NULL),
    m_DeskBand(NULL),
    m_WinEventHandler(NULL),
    m_hWndBand(NULL)
{
}

// Child Band management (simplified due to only supporting one single child)
HRESULT STDMETHODCALLTYPE CMenuSite::AddBand(IUnknown * punk)
{
    HRESULT hr;

    // Little helper, for readability
#define TO_HRESULT(x) ((HRESULT)(S_OK+(x)))

    CComPtr<IUnknown> pUnknown;

    punk->QueryInterface(IID_PPV_ARG(IUnknown, &pUnknown));

    if (pUnknown == m_BandObject)
        return TO_HRESULT(0);

    if (m_BandObject)
    {
        hr = IUnknown_SetSite(m_BandObject, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    m_BandObject = NULL;
    m_DeskBand = NULL;
    m_WinEventHandler = NULL;
    m_hWndBand = NULL;

    if (!pUnknown)
        return TO_HRESULT(0);

    hr = pUnknown->QueryInterface(IID_PPV_ARG(IDeskBand, &m_DeskBand));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pUnknown->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_WinEventHandler));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = IUnknown_SetSite(pUnknown, this->ToIUnknown());
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = IUnknown_GetWindow(pUnknown, &m_hWndBand);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_BandObject = pUnknown;

    return TO_HRESULT(0);
}

HRESULT STDMETHODCALLTYPE CMenuSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    if (uBand == -1ul)
        return GetBandCount();

    if (uBand != 0)
        return E_FAIL;

    *pdwBandID = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    if (dwBandID != 0 || m_BandObject == NULL)
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return m_BandObject->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    if (dwBandID != 0)
        return E_FAIL;

    if (!m_BandObject)
    {
        *ppstb = NULL;
        return E_NOINTERFACE;
    }

    HRESULT hr = m_BandObject->QueryInterface(IID_PPV_ARG(IDeskBand, ppstb));

    *pdwState = 1;

    if (cchName > 0)
        pszName[0] = 0;

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    memset(prc, 0, sizeof(*prc));

    if (dwWhich != 0)
        return S_OK;

    if (m_DeskBand == NULL)
        return S_OK;

    DESKBANDINFO info = { 0 };
    info.dwMask = DBIM_MAXSIZE;

    m_DeskBand->GetBandInfo(0, 0, &info);

    prc->right = info.ptMaxSize.x;
    prc->bottom = info.ptMaxSize.y;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetWindow(HWND *phwnd)
{
    if (!IsWindow())
        return E_FAIL;

    *phwnd = m_hWnd;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::IsWindowOwner(HWND hWnd)
{
    if (hWnd == m_hWnd)
        return S_OK;

    if (!m_WinEventHandler)
        return S_FALSE;

    return m_WinEventHandler->IsWindowOwner(hWnd);
}

HRESULT STDMETHODCALLTYPE CMenuSite::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    if (!m_WinEventHandler)
        return S_OK;

    return m_WinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(guidService, SID_SMenuBandBottom) ||
        IsEqualGUID(guidService, SID_SMenuBandBottomSelected) ||
        IsEqualGUID(guidService, SID_SMenuBandChild))
    {
        if (m_BandObject == NULL)
            return E_NOINTERFACE;

        return IUnknown_QueryService(m_BandObject, guidService, riid, ppvObject);
    }

    if (!m_DeskBarSite)
        return E_NOINTERFACE;

    return IUnknown_QueryService(m_DeskBarSite, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuSite::Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    // Forward Exec calls directly to the parent deskbar
    return IUnknown_Exec(m_DeskBarSite, *pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    // Forward QueryStatus calls directly to the parent deskbar
    return IUnknown_QueryStatus(m_DeskBarSite, *pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetDeskBarSite(IUnknown *punkSite)
{
    HRESULT hr;

    CComPtr<IUnknown> protectThis(this->ToIUnknown());

    // Only initialize if a parent site is being assigned
    if (punkSite)
    {
        HWND hWndSite;

        m_DeskBarSite = NULL;

        hr = IUnknown_GetWindow(punkSite, &hWndSite);

        if (FAILED(hr) || !hWndSite)
            return E_FAIL;

        if (!m_hWnd)
        {
            Create(hWndSite, NULL, L"MenuSite");
        }

        m_DeskBarSite = punkSite;
    }
    else
    {
        // Otherwise, deinitialize.
        if (m_DeskBand)
        {
            m_DeskBand->CloseDW(0);
        }

        hr = IUnknown_SetSite(m_BandObject, NULL);

        m_BandObject = NULL;
        m_DeskBand = NULL;
        m_WinEventHandler = NULL;
        m_hWndBand = NULL;
        if (m_hWnd)
            DestroyWindow();
        m_DeskBarSite = NULL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::UIActivateDBC(DWORD dwState)
{
    if (!m_DeskBand)
        return S_OK;

    return m_DeskBand->ShowDW(dwState != 0);
}

HRESULT STDMETHODCALLTYPE CMenuSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (lpMsg)
        return E_FAIL;

    return IUnknown_UIActivateIO(m_BandObject, fActivate, lpMsg);
}

BOOL CMenuSite::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD mapId)
{
    HWND hWndTarget = NULL;
    CComPtr<IUnknown> protectThis(this->ToIUnknown());

    switch (uMsg)
    {
    case WM_SIZE:
        if (m_BandObject)
        {
            CComPtr<IMenuPopup> pMenuPopup;
            if (SUCCEEDED(m_BandObject->QueryInterface(IID_PPV_ARG(IMenuPopup, &pMenuPopup))))
            {
                RECT Rect = { 0 };
                GetClientRect(&Rect);
                pMenuPopup->OnPosRectChangeDB(&Rect);
            }
        }
        hWndTarget = hWnd;
        lResult = 1;
        break;
    case WM_NOTIFY:
        hWndTarget = reinterpret_cast<LPNMHDR>(lParam)->hwndFrom;
        break;
    case WM_COMMAND:
        hWndTarget = (HWND) lParam;
        break;
    default:
        return FALSE;
    }

    if (hWndTarget && m_WinEventHandler &&
        m_WinEventHandler->IsWindowOwner(hWndTarget) == S_OK)
    {
        if (SUCCEEDED(m_WinEventHandler->OnWinEvent(hWndTarget, uMsg, wParam, lParam, &lResult)))
            return TRUE;
    }

    return FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::RemoveBand(DWORD dwBandID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetModeDBC(DWORD dwMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuSite::HasFocusIO()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuSite::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return S_OK;
}

extern "C"
HRESULT WINAPI RSHELL_CMenuSite_CreateInstance(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CMenuSite>(riid, ppv);
}
