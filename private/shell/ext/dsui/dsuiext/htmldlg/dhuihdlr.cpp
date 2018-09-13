//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      dhuihdlr.cpp
//
//  Contents:  CDocHostUiHandler implimentation
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

CDocHostUiHandler::CDocHostUiHandler(LPSITE pSite) :
    m_cRef(0)
{
#ifdef _DEBUG
    ASSERT(pSite != NULL);
    strcpy(szClass, "CDocHostUiHandler");
#endif
    m_pSite = pSite;
}

CDocHostUiHandler::~CDocHostUiHandler(void)
{
    ASSERT(m_cRef == 0);
}

STDMETHODIMP CDocHostUiHandler::QueryInterface(REFIID riid, void **ppv)
{
    return m_pSite->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) CDocHostUiHandler::AddRef(void)
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG) CDocHostUiHandler::Release(void)
{
    ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
}

// * CDocHostUiHandler::GetHostInfo
// *
// * Purpose: Called at initialisation
// *
STDMETHODIMP CDocHostUiHandler::GetHostInfo(DOCHOSTUIINFO * pInfo)
{
    pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    return S_OK;
}

// * CDocHostUiHandler::ShowUI
// *
// * Purpose: Called when trident shows its UI
// *
STDMETHODIMP
CDocHostUiHandler::ShowUI(DWORD dwID,
                          IOleInPlaceActiveObject * /*pActiveObject*/,
                          IOleCommandTarget * pCommandTarget,
                          IOleInPlaceFrame * /*pFrame*/,
                          IOleInPlaceUIWindow * /*pDoc*/)
{
#ifdef DONTNEED
    // Set the Browse/Edit mode flag using the ID
    if (dwID == DOCHOSTUITYPE_AUTHOR)
    {
        m_pSite->GetFrame()->m_bBrowse = FALSE;
    }
    else
    {
        m_pSite->GetFrame()->m_bBrowse = TRUE;
    }
#endif // DONTNEED

    // If we havent already got the CommandTraget interface then store it
    // away now.
    //
    if (m_pSite->GetCommandTarget() == NULL)
    {
        if (pCommandTarget != NULL)
        {
            // Don't forget to AddRef or Trident will assume we aren't using
            // it, and it'll go away.
            //
            pCommandTarget->AddRef();
            m_pSite->SetCommandTarget(pCommandTarget);
        }
    }

    // We've already got our own UI in place so just return S_OK
    return S_OK;
}

// * CDocHostUiHandler::HideUI
// *
// * Purpose: Called when Trident hides its UI
// *
STDMETHODIMP CDocHostUiHandler::HideUI(void)
{
    return S_OK;
}

// * CDocHostUiHandler::UpdateUI
// *
// * Purpose: Called when Trident updates its UI
// *
STDMETHODIMP CDocHostUiHandler::UpdateUI(void)
{
    // MFC is pretty good about updating it's UI in it's Idle loop so I don't do anything here
    return S_OK;
}

// * CDocHostUiHandler::EnableModeless
// *
// * Purpose: Called from Trident's IOleInPlaceActiveObject::EnableModeless
// *
STDMETHODIMP CDocHostUiHandler::EnableModeless(BOOL /*fEnable*/)
{
    return E_NOTIMPL;
}

// * CDocHostUiHandler::OnDocWindowActivate
// *
// * Purpose: Called from Trident's IOleInPlaceActiveObject::OnDocWindowActivate
// *
STDMETHODIMP CDocHostUiHandler::OnDocWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CDocHostUiHandler::OnFrameWindowActivate
// *
// * Purpose: Called from Trident's IOleInPlaceActiveObject::OnFrameWindowActivate
// *
STDMETHODIMP CDocHostUiHandler::OnFrameWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CDocHostUiHandler::ResizeBorder
// *
// * Purpose: Called from Trident's IOleInPlaceActiveObject::ResizeBorder
// *
STDMETHODIMP CDocHostUiHandler::ResizeBorder(
                LPCRECT /*prcBorder*/, 
                IOleInPlaceUIWindow* /*pUIWindow*/,
                BOOL /*fRameWindow*/)
{
    return E_NOTIMPL;
}

// * CDocHostUiHandler::ShowContextMenu
// *
// * Purpose: Called when Trident would normally display its context menu
// *
STDMETHODIMP CDocHostUiHandler::ShowContextMenu(
                DWORD /*dwID*/, 
                POINT* /*pptPosition*/,
                IUnknown* /*pCommandTarget*/,
                IDispatch* /*pDispatchObjectHit*/)
{
    return E_NOTIMPL;
}

// * CDocHostUiHandler::TranslateAccelerator
// *
// * Purpose: Called from Trident's TranslateAccelerator routines
// *
STDMETHODIMP CDocHostUiHandler::TranslateAccelerator(LPMSG lpMsg,
            /* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID)
{
    return S_FALSE;
}

// * CDocHostUiHandler::GetOptionKeyPath
// *
// * Purpose: Called by Trident to find where the host wishes to store 
// *    its options in the registry
// *
STDMETHODIMP CDocHostUiHandler::GetOptionKeyPath(BSTR* pbstrKey, ULONG cchpstrKey)
{
#if 0 
    // BUGBUG: more MFC to remove
    CProptestApp * pApp = (CProptestApp *)AfxGetApp();
    CString strPath = pApp->m_pszRegistryKey;
    BSTR bstr = strPath.AllocSysString();
    pbstrKey = &bstr;
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

STDMETHODIMP CDocHostUiHandler::GetDropTarget(
            /* [in] */ IDropTarget __RPC_FAR *pDropTarget,
            /* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  CDocHostUiHandler::GetExternal
//
//  BUGBUG: GetExternal not implemented.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDocHostUiHandler::GetExternal(IDispatch **pDispath)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  CDocHostUiHandler::TranslateUrl
//
//  BUGBUG: TranslateUrl not implemented.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDocHostUiHandler::TranslateUrl(DWORD dwTranslate,
                                OLECHAR *pStr,
                                OLECHAR **ppStr)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  CDocHostUiHandler::FilterDataObject
//
//  BUGBUG: FilterDataObject not implemented.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDocHostUiHandler::FilterDataObject(IDataObject *pObj,
                                    IDataObject **ppObj)
{
    return E_NOTIMPL;
}
