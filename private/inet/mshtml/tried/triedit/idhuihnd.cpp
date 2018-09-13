//------------------------------------------------------------------------------
// idhuihnd.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Author
//     bash
//
// History
//      6-27-97     created     (bash)
//
// Implementation of IDocHostUIHandler.
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "triedit.h"
#include "document.h"

STDMETHODIMP CTriEditUIHandler::QueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;

    if ( IID_IDocHostUIHandler == riid || IID_IUnknown == riid )
    {
        *ppv = this;
    }

    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTriEditUIHandler::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CTriEditUIHandler::Release(void)
{
    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    return 0;
}

STDMETHODIMP CTriEditUIHandler::GetHostInfo(DOCHOSTUIINFO* pInfo)
{
    ATLTRACE(_T("IDocHostUIImpl::GetHostInfo\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->GetHostInfo(pInfo);

// REVIEW(MikhailA): remove this as soon as we start using IE5 headers VS-wide
#define DOCHOSTUIFLAG_TABSTOPONBODY 0x0800 // MikhailA: From IE5 headers

    pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_TABSTOPONBODY;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

STDMETHODIMP CTriEditUIHandler::ShowUI(DWORD dwID, IOleInPlaceActiveObject* pActiveObject,
                    IOleCommandTarget* /*pCommandTarget*/, IOleInPlaceFrame* pFrame,
                    IOleInPlaceUIWindow* pDoc)
{
    // ATLTRACE(_T("IDocHostUIImpl::ShowUI\n"));  Turn this off for now

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->ShowUI(dwID, pActiveObject, static_cast<IOleCommandTarget*>(m_pDoc), pFrame, pDoc);

    return S_FALSE;
}

STDMETHODIMP CTriEditUIHandler::HideUI()
{
    // ATLTRACE(_T("IDocHostUIImpl::HideUI\n"));  Turn this off for now

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->HideUI();

    return S_OK;
}

STDMETHODIMP CTriEditUIHandler::UpdateUI()
{
    // ATLTRACE(_T("IDocHostUIImpl::UpdateUI\n"));  Turn this off for now

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->UpdateUI();

    return S_OK;
}

STDMETHODIMP CTriEditUIHandler::EnableModeless(BOOL fEnable)
{
    ATLTRACE(_T("IDocHostUIImpl::EnableModeless\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->EnableModeless(fEnable);
    
    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::OnDocWindowActivate(BOOL fActivate)
{
    ATLTRACE(_T("IDocHostUIImpl::OnDocWindowActivate\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->OnDocWindowActivate(fActivate);

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
    ATLTRACE(_T("IDocHostUIImpl::OnFrameWindowActivate\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->OnFrameWindowActivate(fActivate);

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow)
{
    ATLTRACE(_T("IDocHostUIImpl::ResizeBorder\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->ResizeBorder(prcBorder, pUIWindow, fFrameWindow);

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::ShowContextMenu(DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget,
                             IDispatch* pDispatchObjectHit)
{
    ATLTRACE(_T("IDocHostUIImpl::ShowContextMenu\n"));

    if (m_pDoc->m_pUIHandlerHost)
    {
        HRESULT hr = S_OK;

        // Work around a Trident bug where they call ShowContextMenu recursively under some circumstances
        if (!m_pDoc->m_fInContextMenu)
        {
            m_pDoc->m_fInContextMenu = TRUE;
            hr = m_pDoc->m_pUIHandlerHost->ShowContextMenu(dwID, pptPosition, pCommandTarget, pDispatchObjectHit);
            m_pDoc->m_fInContextMenu = FALSE;
        }

        ATLTRACE(_T("Returning From IDocHostUIImpl::ShowContextMenu\n"));
        return hr;
    }

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::TranslateAccelerator(LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID)
{
    // ATLTRACE(_T("IDocHostUIImpl::TranslateAccelerator\n"));  Turn this off for now.

    // This is where we would add code if we wanted to handle any accelerators in TriEdit
    
    HRESULT hr  = S_FALSE;  // Defualt return value: not handled

    if (m_pDoc->m_pUIHandlerHost)
    {
        hr = m_pDoc->m_pUIHandlerHost->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
    }

    // Kill ctrl-g and ctrl-h before they reach Trident: erronious handling attempts to bring up
    // non-existant html dialogs for Go and Replace.
    if ( ( S_FALSE == hr ) && ( lpMsg->message == WM_KEYDOWN ) )
    {
        BOOL fControl = (0x8000 & GetKeyState(VK_CONTROL));
        BOOL fShift = (0x8000 & GetKeyState(VK_SHIFT));
        if ( fControl && !fShift )
        {
            switch ( lpMsg->wParam )
            {
                case 'G':
                case 'H':
                    hr = S_OK;  // Consider them handled.
                default:
                    break;
            }
        }
    }

    return hr;
}

STDMETHODIMP CTriEditUIHandler::GetOptionKeyPath(LPOLESTR* pbstrKey, DWORD dw)
{
    ATLTRACE(_T("IDocHostUIImpl::GetOptionKeyPath\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->GetOptionKeyPath(pbstrKey, dw);
    
    *pbstrKey = NULL;
    return S_FALSE;
}

STDMETHODIMP CTriEditUIHandler::GetDropTarget(IDropTarget __RPC_FAR *pDropTarget,
                           IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
    ATLTRACE(_T("IDocHostUIImpl::GetDropTarget\n"));

    SAFERELEASE(m_pDoc->m_pDropTgtTrident);

    m_pDoc->m_pDropTgtTrident = pDropTarget;
    m_pDoc->m_pDropTgtTrident->AddRef();

    if (NULL == m_pDoc->m_pUIHandlerHost ||
        S_OK != m_pDoc->m_pUIHandlerHost->GetDropTarget(static_cast<IDropTarget*>(m_pDoc), ppDropTarget))
    {
        *ppDropTarget = static_cast<IDropTarget*>(m_pDoc);
        (*ppDropTarget)->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CTriEditUIHandler::GetExternal(IDispatch **ppDispatch)
{
    ATLTRACE(_T("IDocHostUIImpl::GetExternal\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->GetExternal(ppDispatch);

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    ATLTRACE(_T("IDocHostUIImpl::TranslateUrl\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->TranslateUrl(dwTranslate, pchURLIn, ppchURLOut);

    return E_NOTIMPL;
}

STDMETHODIMP CTriEditUIHandler::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    ATLTRACE(_T("IDocHostUIImpl::FilterDataObject\n"));

    if (m_pDoc->m_pUIHandlerHost)
        return m_pDoc->m_pUIHandlerHost->FilterDataObject(pDO, ppDORet);

    return E_NOTIMPL;
}

