#include <precomp.h>

/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.
   Author: Leon Finker	11/2000
   Modifications: replaced ATL by STL, Martin Fuchs 7/2003
**************************************************************************/

// dragdropimp.cpp: implementation of the IDataObjectImpl class.
//////////////////////////////////////////////////////////////////////

//#include <shlobj.h>
//#include <assert.h>

//#include "dragdropimpl.h"

//////////////////////////////////////////////////////////////////////
// IDataObjectImpl Class
//////////////////////////////////////////////////////////////////////

IDataObjectImpl::IDataObjectImpl(IDropSourceImpl* pDropSource)
 :	super(IID_IDataObject),
	m_pDropSource(pDropSource),
	m_cRefCount(0)
{
}

IDataObjectImpl::~IDataObjectImpl()
{
	for(StorageArray::iterator it=_storage.begin(); it!=_storage.end(); ++it)
		ReleaseStgMedium(it->_medium);
}

STDMETHODIMP IDataObjectImpl::GetData(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
	/* [out] */ STGMEDIUM __RPC_FAR *pmedium)
{
	if (pformatetcIn == NULL || pmedium == NULL)
		return E_INVALIDARG;

	pmedium->hGlobal = NULL;

	for(StorageArray::iterator it=_storage.begin(); it!=_storage.end(); ++it)
	{
		if (pformatetcIn->tymed & it->_format->tymed &&
		   pformatetcIn->dwAspect == it->_format->dwAspect &&
		   pformatetcIn->cfFormat == it->_format->cfFormat)
		{
			CopyMedium(pmedium, it->_medium, it->_format);
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

STDMETHODIMP IDataObjectImpl::GetDataHere(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
	/* [out][in] */ STGMEDIUM __RPC_FAR *pmedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP IDataObjectImpl::QueryGetData(
   /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc)
{
	if (pformatetc == NULL)
		return E_INVALIDARG;

	//support others if needed DVASPECT_THUMBNAIL  //DVASPECT_ICON	 //DVASPECT_DOCPRINT
	if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
		return (DV_E_DVASPECT);

	HRESULT  hr = DV_E_TYMED;

	for(StorageArray::iterator it=_storage.begin(); it!=_storage.end(); ++it)
	{
	   if (pformatetc->tymed & it->_format->tymed)
	   {
		  if (pformatetc->cfFormat == it->_format->cfFormat)
			 return S_OK;
		  else
			 hr = DV_E_CLIPFORMAT;
	   }
	   else
		  hr = DV_E_TYMED;
	}

	return hr;
}

STDMETHODIMP IDataObjectImpl::GetCanonicalFormatEtc(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
	/* [out] */ FORMATETC __RPC_FAR *pformatetcOut)
{
	if (pformatetcOut == NULL)
		return E_INVALIDARG;

	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP IDataObjectImpl::SetData(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
	/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
	/* [in] */ BOOL fRelease)
{
	if (pformatetc == NULL || pmedium == NULL)
	  return E_INVALIDARG;

	assert(pformatetc->tymed == pmedium->tymed);
	FORMATETC* fetc=new FORMATETC;
	STGMEDIUM* pStgMed = new STGMEDIUM;

	if (fetc == NULL || pStgMed == NULL)
		return E_OUTOFMEMORY;

	ZeroMemory(fetc, sizeof(FORMATETC));
	ZeroMemory(pStgMed, sizeof(STGMEDIUM));

	*fetc = *pformatetc;

	if (fRelease)
	  *pStgMed = *pmedium;
	else
		CopyMedium(pStgMed, pmedium, pformatetc);

	DataStorage storage;

	storage._format = fetc;
	storage._medium = pStgMed;

	_storage.push_back(storage);

	return S_OK;
}

void IDataObjectImpl::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
	switch(pMedSrc->tymed)
	{
	case TYMED_HGLOBAL:
		pMedDest->hGlobal = (HGLOBAL)OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, 0);
		break;
	case TYMED_GDI:
		pMedDest->hBitmap = (HBITMAP)OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, 0);
		break;
	case TYMED_MFPICT:
		pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, 0);
		break;
	case TYMED_ENHMF:
		pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, 0);
		break;
	case TYMED_FILE:
		pMedDest->lpszFileName = (LPOLESTR)OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, 0);
		break;
	case TYMED_ISTREAM:
		pMedDest->pstm = pMedSrc->pstm;
		pMedSrc->pstm->AddRef();
		break;
	case TYMED_ISTORAGE:
		pMedDest->pstg = pMedSrc->pstg;
		pMedSrc->pstg->AddRef();
		break;
	case TYMED_NULL:
	default:
		break;
	}
	pMedDest->tymed = pMedSrc->tymed;
	pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
}

STDMETHODIMP IDataObjectImpl::EnumFormatEtc(
   /* [in] */ DWORD dwDirection,
   /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc)
{
	if (ppenumFormatEtc == NULL)
	  return E_POINTER;

	*ppenumFormatEtc=NULL;
	switch (dwDirection)
	{
	  case DATADIR_GET:
		 *ppenumFormatEtc = new EnumFormatEtcImpl(_storage);

		 if (!*ppenumFormatEtc)
			 return E_OUTOFMEMORY;

		 (*ppenumFormatEtc)->AddRef();
		 break;

	  case DATADIR_SET:
	  default:
		 return E_NOTIMPL;
		 break;
	}

   return S_OK;
}

STDMETHODIMP IDataObjectImpl::DAdvise(
   /* [in] */ FORMATETC __RPC_FAR *pformatetc,
   /* [in] */ DWORD advf,
   /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
   /* [out] */ DWORD __RPC_FAR *pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP IDataObjectImpl::DUnadvise(
   /* [in] */ DWORD dwConnection)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDataObjectImpl::EnumDAdvise(
   /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////
// IDropSourceImpl Class
//////////////////////////////////////////////////////////////////////

STDMETHODIMP IDropSourceImpl::QueryContinueDrag(
	/* [in] */ BOOL fEscapePressed,
	/* [in] */ DWORD grfKeyState)
{
   if (fEscapePressed)
	  return DRAGDROP_S_CANCEL;
   if (!(grfKeyState & (MK_LBUTTON|MK_RBUTTON)))
   {
	  m_bDropped = true;
	  return DRAGDROP_S_DROP;
   }

   return S_OK;

}

STDMETHODIMP IDropSourceImpl::GiveFeedback(
	/* [in] */ DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//////////////////////////////////////////////////////////////////////
// EnumFormatEtcImpl Class
//////////////////////////////////////////////////////////////////////

EnumFormatEtcImpl::EnumFormatEtcImpl(const FormatArray& ArrFE)
 :	super(IID_IEnumFORMATETC),
	m_cRefCount(0),
	m_iCur(0)
{
   for(FormatArray::const_iterator it=ArrFE.begin(); it!=ArrFE.end(); ++it)
		m_pFmtEtc.push_back(*it);
}

EnumFormatEtcImpl::EnumFormatEtcImpl(const StorageArray& ArrFE)
 :	super(IID_IEnumFORMATETC),
	m_cRefCount(0),
	m_iCur(0)
{
   for(StorageArray::const_iterator it=ArrFE.begin(); it!=ArrFE.end(); ++it)
		m_pFmtEtc.push_back(*it->_format);
}

STDMETHODIMP EnumFormatEtcImpl::Next(ULONG celt,LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
{
   if (pceltFetched != NULL)
	   *pceltFetched=0;

   ULONG cReturn = celt;

   if (celt <= 0 || lpFormatEtc == NULL || m_iCur >= m_pFmtEtc.size())
	  return S_FALSE;

   if (pceltFetched == NULL && celt != 1) // pceltFetched can be NULL only for 1 item request
	  return S_FALSE;

	while (m_iCur < m_pFmtEtc.size() && cReturn > 0)
	{
		*lpFormatEtc++ = m_pFmtEtc[m_iCur++];
		--cReturn;
	}
	if (pceltFetched != NULL)
		*pceltFetched = celt - cReturn;

	return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP EnumFormatEtcImpl::Skip(ULONG celt)
{
	if ((m_iCur + int(celt)) >= m_pFmtEtc.size())
		return S_FALSE;

	m_iCur += celt;
	return S_OK;
}

STDMETHODIMP EnumFormatEtcImpl::Reset(void)
{
   m_iCur = 0;
   return S_OK;
}

STDMETHODIMP EnumFormatEtcImpl::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
  if (ppCloneEnumFormatEtc == NULL)
	return E_POINTER;

  EnumFormatEtcImpl* newEnum = new EnumFormatEtcImpl(m_pFmtEtc);

  if (!newEnum)
	return E_OUTOFMEMORY;

  newEnum->AddRef();
  newEnum->m_iCur = m_iCur;
  *ppCloneEnumFormatEtc = newEnum;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IDropTargetImpl Class
//////////////////////////////////////////////////////////////////////
IDropTargetImpl::IDropTargetImpl(HWND hTargetWnd)
 :	m_cRefCount(0),
	m_bAllowDrop(false),
	m_pDropTargetHelper(NULL),
	m_pSupportedFrmt(NULL),
	m_hTargetWnd(hTargetWnd)
{
	assert(m_hTargetWnd != NULL);

	if (FAILED(CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
					 IID_IDropTargetHelper,(LPVOID*)&m_pDropTargetHelper)))
		m_pDropTargetHelper = NULL;
}

IDropTargetImpl::~IDropTargetImpl()
{
	if (m_pDropTargetHelper != NULL)
	{
		m_pDropTargetHelper->Release();
		m_pDropTargetHelper = NULL;
	}
}

HRESULT STDMETHODCALLTYPE IDropTargetImpl::QueryInterface( /* [in] */ REFIID riid,
						/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDropTarget==riid)
		*ppvObject=this;

	if (*ppvObject != NULL)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE IDropTargetImpl::Release()
{
   long nTemp = --m_cRefCount;

   assert(nTemp >= 0);

   if (nTemp == 0)
	  delete this;

   return nTemp;
}

bool IDropTargetImpl::QueryDrop(DWORD grfKeyState, LPDWORD pdwEffect)
{
	DWORD dwOKEffects = *pdwEffect;

	if (!m_bAllowDrop)
	{
	   *pdwEffect = DROPEFFECT_NONE;
	   return false;
	}

	//CTRL+SHIFT  -- DROPEFFECT_LINK
	//CTRL		  -- DROPEFFECT_COPY
	//SHIFT 	  -- DROPEFFECT_MOVE
	//no modifier -- DROPEFFECT_MOVE or whatever is allowed by src
	*pdwEffect = (grfKeyState & MK_CONTROL) ?
				 ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_LINK : DROPEFFECT_COPY ):
				 ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : DROPEFFECT_NONE );
	if (*pdwEffect == 0)
	{
	   // No modifier keys used by user while dragging.
	   if (DROPEFFECT_COPY & dwOKEffects)
		  *pdwEffect = DROPEFFECT_COPY;
	   else if (DROPEFFECT_MOVE & dwOKEffects)
		  *pdwEffect = DROPEFFECT_MOVE;
	   else if (DROPEFFECT_LINK & dwOKEffects)
		  *pdwEffect = DROPEFFECT_LINK;
	   else
	   {
		  *pdwEffect = DROPEFFECT_NONE;
	   }
	}
	else
	{
	   // Check if the drag source application allows the drop effect desired by user.
	   // The drag source specifies this in DoDragDrop
	   if (!(*pdwEffect & dwOKEffects))
		  *pdwEffect = DROPEFFECT_NONE;
	}

	return (DROPEFFECT_NONE == *pdwEffect)?false:true;
}

HRESULT STDMETHODCALLTYPE IDropTargetImpl::DragEnter(
	/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
	/* [in] */ DWORD grfKeyState,
	/* [in] */ POINTL pt,
	/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if (pDataObj == NULL)
		return E_INVALIDARG;

	if (m_pDropTargetHelper)
		m_pDropTargetHelper->DragEnter(m_hTargetWnd, pDataObj, (LPPOINT)&pt, *pdwEffect);

	//IEnumFORMATETC* pEnum;
	//pDataObj->EnumFormatEtc(DATADIR_GET,&pEnum);
	//FORMATETC ftm;
	//for()
	//pEnum->Next(1,&ftm,0);
	//pEnum->Release();
	m_pSupportedFrmt = NULL;

	for(FormatArray::iterator it=m_formatetc.begin(); it!=m_formatetc.end(); ++it)
	{
		m_bAllowDrop = (pDataObj->QueryGetData(&*it) == S_OK)? true: false;

		if (m_bAllowDrop)
		{
			m_pSupportedFrmt = &*it;
			break;
		}
	}

	QueryDrop(grfKeyState, pdwEffect);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTargetImpl::DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if (m_pDropTargetHelper)
		m_pDropTargetHelper->DragOver((LPPOINT)&pt, *pdwEffect);

	QueryDrop(grfKeyState, pdwEffect);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTargetImpl::DragLeave()
{
	if (m_pDropTargetHelper)
		m_pDropTargetHelper->DragLeave();

	m_bAllowDrop = false;
	m_pSupportedFrmt = NULL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTargetImpl::Drop(
	/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
	/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt,
	/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if (pDataObj == NULL)
		return E_INVALIDARG;

	if (m_pDropTargetHelper)
		m_pDropTargetHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);

	if (QueryDrop(grfKeyState, pdwEffect))
	{
		if (m_bAllowDrop && m_pSupportedFrmt != NULL)
		{
			STGMEDIUM medium;

			if (pDataObj->GetData(m_pSupportedFrmt, &medium) == S_OK)
			{
				if (OnDrop(m_pSupportedFrmt, medium, pdwEffect)) //does derive class wants us to free medium?
					ReleaseStgMedium(&medium);
			}
		}
	}

	m_bAllowDrop = false;
	*pdwEffect = DROPEFFECT_NONE;
	m_pSupportedFrmt = NULL;

	return S_OK;
}
