/*
 *    Start menu object
 *
 *    Copyright 2009 Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL(shell32start);

CStartMenu::CStartMenu() :
    m_pBandSite(NULL),
    m_pUnkSite(NULL)
{
}

CStartMenu::~CStartMenu()
{
}

HRESULT STDMETHODCALLTYPE CStartMenu::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::GetWindow(HWND *phwnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::GetClient(IUnknown **ppunkClient)
{
    TRACE("(%p, %p)\n", this, ppunkClient);

    *ppunkClient = (IUnknown*)m_pBandSite;
    (*ppunkClient)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CStartMenu::OnPosRectChangeDB(LPRECT prc)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::SetClient(IUnknown *punkClient)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::OnSelect(DWORD dwSelectType)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::SetSite(IUnknown *pUnkSite)
{
    TRACE("(%p, %p)\n", this, pUnkSite);

    if (m_pUnkSite)
        m_pUnkSite->Release();
    m_pUnkSite = pUnkSite;
    if (m_pUnkSite)
        m_pUnkSite->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CStartMenu::GetSite(REFIID riid, void **ppvSite)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvSite);

    if (!m_pUnkSite)
        return E_FAIL;

    return m_pUnkSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CStartMenu::Initialize()
{
    HRESULT hr;
    CComObject<CMenuBandSite> *pBandSiteObj;

    TRACE("(%p)\n", this);

    //pBandSiteObj = new CComObject<CMenuBandSite>();
    ATLTRY (pBandSiteObj = new CComObject<CMenuBandSite>);
    if (pBandSiteObj == NULL)
        return E_OUTOFMEMORY;

    hr = pBandSiteObj->QueryInterface(IID_PPV_ARG(IBandSite, &m_pBandSite));
    if (FAILED(hr))
        return NULL;

    return m_pBandSite->AddBand((IMenuBand*)this);
}

HRESULT STDMETHODCALLTYPE CStartMenu::IsMenuMessage(MSG *pmsg)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenu::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

CMenuBandSite::CMenuBandSite() :
    m_pObjects(NULL),
    m_cObjects(0)
{
}

CMenuBandSite::~CMenuBandSite()
{
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::AddBand(IUnknown *punk)
{
    IUnknown **pObjects;

    TRACE("punk %p\n", punk);

    if (!punk)
        return E_FAIL;

    pObjects = (IUnknown**)CoTaskMemAlloc(sizeof(IUnknown*) * (m_cObjects + 1));
    if (!pObjects)
        return E_FAIL;

    RtlMoveMemory(pObjects, m_pObjects, sizeof(IUnknown*) * m_cObjects);

    CoTaskMemFree(m_pObjects);

    m_pObjects = pObjects;

    m_pObjects[m_cObjects] = punk;
    punk->AddRef();

    m_cObjects++;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::EnumBands(UINT uBand, DWORD *pdwBandID)
{
    ULONG Index, ObjectCount;

    TRACE("uBand %uu pdwBandID %p\n", uBand, pdwBandID);

    if (uBand == (UINT)-1)
        return m_cObjects;

    ObjectCount = 0;

    for (Index = 0; Index < m_cObjects; Index++)
    {
        if (m_pObjects[Index] != NULL)
        {
            if (uBand == ObjectCount)
            {
                *pdwBandID = Index;
                return S_OK;
            }
            ObjectCount++;
        }
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::RemoveBand(DWORD dwBandID)
{
    TRACE("dwBandID %u\n", dwBandID);

    if (m_cObjects <= dwBandID)
        return E_FAIL;

    if (m_pObjects[dwBandID])
    {
        m_pObjects[dwBandID]->Release();
        m_pObjects[dwBandID] = NULL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    TRACE("dwBandID %u riid %p ppv %p\n", dwBandID, riid, ppv);

    if (m_cObjects <= dwBandID)
        return E_FAIL;

    if (m_pObjects[dwBandID])
    {
        return m_pObjects[dwBandID]->QueryInterface(riid, ppv);
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetWindow(HWND *phwnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetDeskBarSite(IUnknown *punkSite)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::SetModeDBC(DWORD dwMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::UIActivateDBC(DWORD dwState)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::HasFocusIO()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::OnWinEvent(HWND paramC, UINT param10, WPARAM param14, LPARAM param18, LRESULT *param1C)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::IsWindowOwner(HWND paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBandSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
