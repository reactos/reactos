//------------------------------------------------------------------------------
// ioleobj.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Author
//     bash
//
// History
//      6-27-97      created     (bash)
//		4-8-98       ported to dhtmled (vank)
//
// Implementation of IOleObject.
//
// We mostly just delegate to Trident's IOleObject, except
// for GetUserClassID and GetUserType.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "DHTMLEd.h"
#include "dhtmledit.h"

STDMETHODIMP CDHTMLEdit::SetClientSite(IOleClientSite *pClientSite)
{
	ATLTRACE(_T("CDHTMLEdit::SetClientSite\n"));

	_ASSERTE(m_pInnerIOleObj);
    
	return m_pInnerIOleObj->SetClientSite ( pClientSite );
}

STDMETHODIMP CDHTMLEdit::GetClientSite(IOleClientSite **ppClientSite)
{
	ATLTRACE(_T("CDHTMLEdit::GetClientSite\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->GetClientSite(ppClientSite);
}

STDMETHODIMP CDHTMLEdit::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
	ATLTRACE(_T("CDHTMLEdit::SetHostNames\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->SetHostNames(szContainerApp, szContainerObj);
}

STDMETHODIMP CDHTMLEdit::Close(DWORD dwSaveOption)
{
	ATLTRACE(_T("CDHTMLEdit::Close\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->Close(dwSaveOption);
}

STDMETHODIMP CDHTMLEdit::SetMoniker(DWORD dwWhichMoniker, IMoniker* pmk)
{
	ATLTRACE(_T("CDHTMLEdit::SetMoniker\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->SetMoniker(dwWhichMoniker, pmk);
}

STDMETHODIMP CDHTMLEdit::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk )
{
	ATLTRACE(_T("CDHTMLEdit::GetMoniker\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->GetMoniker(dwAssign, dwWhichMoniker, ppmk);
}

STDMETHODIMP CDHTMLEdit::InitFromData(IDataObject*  pDataObject, BOOL fCreation, DWORD dwReserved)
{
	ATLTRACE(_T("CDHTMLEdit::InitFromData\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->InitFromData(pDataObject,  fCreation,  dwReserved );
}

STDMETHODIMP CDHTMLEdit::GetClipboardData(DWORD dwReserved, IDataObject** ppDataObject)
{
	ATLTRACE(_T("CDHTMLEdit::GetClipboardData\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->GetClipboardData(dwReserved, ppDataObject);
}


STDMETHODIMP CDHTMLEdit::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite* pActiveSite , LONG lindex ,
								 HWND hwndParent, LPCRECT lprcPosRect)
{
	ATLTRACE(_T("CDHTMLEdit::DoVerb\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
}

STDMETHODIMP CDHTMLEdit::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
{
	ATLTRACE(_T("CDHTMLEdit::EnumVerbs\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->EnumVerbs(ppEnumOleVerb);
}

STDMETHODIMP CDHTMLEdit::Update(void)
{
	ATLTRACE(_T("CDHTMLEdit::Update\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->Update();
}

STDMETHODIMP CDHTMLEdit::IsUpToDate(void)
{
	ATLTRACE(_T("CDHTMLEdit::IsUpToDate\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->IsUpToDate();
}

STDMETHODIMP CDHTMLEdit::GetUserClassID(CLSID *pClsid)
{
	ATLTRACE(_T("CDHTMLEdit::GetUserClassID\n"));

	_ASSERTE(m_pInnerIOleObj);

	*pClsid = GetObjectCLSID();

	return S_OK;
}

STDMETHODIMP CDHTMLEdit::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
	ATLTRACE(_T("CDHTMLEdit::GetUserType\n"));

	_ASSERTE(m_pInnerIOleObj);

	return OleRegGetUserType(GetObjectCLSID(), dwFormOfType, pszUserType);
}

STDMETHODIMP CDHTMLEdit::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	ATLTRACE(_T("CDHTMLEdit::SetExtent\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->SetExtent(dwDrawAspect, psizel);
}

STDMETHODIMP CDHTMLEdit::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	ATLTRACE(_T("CDHTMLEdit::GetExtent\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->GetExtent(dwDrawAspect, psizel);
}

STDMETHODIMP CDHTMLEdit::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	ATLTRACE(_T("CDHTMLEdit::Advise\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->Advise(pAdvSink, pdwConnection);
}

STDMETHODIMP CDHTMLEdit::Unadvise(DWORD dwConnection)
{
	ATLTRACE(_T("CDHTMLEdit::Unadvise\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->Unadvise(dwConnection);
}

STDMETHODIMP CDHTMLEdit::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
{
	ATLTRACE(_T("CDHTMLEdit::EnumAdvise\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->EnumAdvise(ppenumAdvise);
}

STDMETHODIMP CDHTMLEdit::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
	ATLTRACE(_T("CDHTMLEdit::GetMiscStatus\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->GetMiscStatus(dwAspect, pdwStatus);
}

STDMETHODIMP CDHTMLEdit::SetColorScheme(LOGPALETTE* pLogpal)
{
	ATLTRACE(_T("CDHTMLEdit::SetColorScheme\n"));

	_ASSERTE(m_pInnerIOleObj);

	return m_pInnerIOleObj->SetColorScheme(pLogpal);
}


