#include "priv.h"
#include "privcpp.h"

CPackage_IDataObject::CPackage_IDataObject(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0); 

}

CPackage_IDataObject::~CPackage_IDataObject()
{
    DebugMsg(DM_TRACE,"CPackage_IDataObject destroyed with ref count %d",_cRef);
}


//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IDataObject::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);
}

ULONG CPackage_IDataObject::AddRef(void) 
{
    _cRef++;    // interface ref count for debugging
    return _pPackage->AddRef();
}

ULONG CPackage_IDataObject::Release(void)
{
    _cRef--;    // interface ref count for debugging
    return _pPackage->Release();
}


//////////////////////////////////
//
// IDataObject Methods...
//
HRESULT CPackage_IDataObject::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    UINT cf = pFEIn->cfFormat;

    DebugMsg(DM_TRACE, "pack do - GetData() called.");
    
    // Check the aspects we support
    if (!(pFEIn->dwAspect & DVASPECT_CONTENT)) {
        DebugMsg(DM_TRACE,
	    "            Invalid Aspect! dwAspect=%d",pFEIn->dwAspect);
        return DATA_E_FORMATETC;
    }
    
    // we set this to NULL so we aren't responsible for freeing memory
    pSTM->pUnkForRelease = NULL;

    // Go render the appropriate data for the format.
    if (cf == CF_FILEDESCRIPTOR) 
	return _pPackage->GetFileDescriptor(pFEIn,pSTM);
    
    else if (cf ==  CF_FILECONTENTS) 
	return _pPackage->GetFileContents(pFEIn,pSTM);
    
    else if (cf == CF_METAFILEPICT) 
	return _pPackage->GetMetafilePict(pFEIn,pSTM);

    else if (cf == CF_OBJECTDESCRIPTOR)
	return _pPackage->GetObjectDescriptor(pFEIn,pSTM);
		

#ifdef DEBUG
    else {
	TCHAR szFormat[80];
	GetClipboardFormatName(cf, szFormat, ARRAYSIZE(szFormat));
	DebugMsg(DM_TRACE,"            unknown format: %s",szFormat);
	return DATA_E_FORMATETC;
    }
#endif

    return DATA_E_FORMATETC;
}

HRESULT CPackage_IDataObject::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DebugMsg(DM_TRACE, "pack do - GetDataHere() called.");
    
    HRESULT     hr;
    
    // The only reasonable time this is called is for CFSTR_EMEDSOURCE and
    // TYMED_ISTORAGE.  This means the same as IPersistStorage::Save
    
    // Aspect is unimportant to us here, as is lindex and ptd.
    if (pFE->cfFormat == CF_EMBEDSOURCE && (pFE->tymed & TYMED_ISTORAGE)) {
        // we have an IStorage we can write into.
        pSTM->tymed = TYMED_ISTORAGE;
        pSTM->pUnkForRelease = NULL;
        
        hr = _pPackage->_pIPersistStorage->Save(pSTM->pstg, FALSE);
        _pPackage->_pIPersistStorage->SaveCompleted(NULL);
        return hr;
    }
    
    return DATA_E_FORMATETC;
}
    
    
HRESULT CPackage_IDataObject::QueryGetData(LPFORMATETC pFE)
{
    UINT        cf = pFE->cfFormat;
    BOOL        fRet = FALSE;
    
    DebugMsg(DM_TRACE, "pack do - QueryGetData() called.");

    if (!(pFE->dwAspect & DVASPECT_CONTENT))
        return S_FALSE;

    if (cf == CF_FILEDESCRIPTOR) {
	DebugMsg(DM_TRACE,"            Getting File Descriptor");
	fRet = (BOOL)(pFE->tymed & TYMED_HGLOBAL);
    }
    else if (cf == CF_FILECONTENTS) {
	DebugMsg(DM_TRACE,"            Getting File Contents");
	fRet = (BOOL)(pFE->tymed & (TYMED_HGLOBAL|TYMED_ISTREAM)); 
    }
    else if (cf == CF_EMBEDSOURCE) {
 	DebugMsg(DM_TRACE,"            Getting Embed Source");
	fRet = (BOOL)(pFE->tymed & TYMED_ISTORAGE);
    }
    else if (cf == CF_OBJECTDESCRIPTOR) {
 	DebugMsg(DM_TRACE,"            Getting Object Descriptor");
	fRet = (BOOL)(pFE->tymed & TYMED_HGLOBAL);
    }
    else if (cf == CF_METAFILEPICT) {
	DebugMsg(DM_TRACE,"            Getting MetafilePict");
	fRet = (BOOL)(pFE->tymed & TYMED_MFPICT);
    }

#ifdef DEBUG
    else {
	TCHAR szFormat[255];
	GetClipboardFormatName(cf, szFormat, ARRAYSIZE(szFormat));
	DebugMsg(DM_TRACE,"            unknown format: %s",szFormat);
	fRet = FALSE;
    }
#endif
	    
    DebugMsg(DM_TRACE,
	"            fRet == %s",fRet ? TEXT("TRUE") : TEXT("FALSE"));
    return fRet ? S_OK : S_FALSE;
}

    
HRESULT CPackage_IDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn,
LPFORMATETC pFEOut)
{
    DebugMsg(DM_TRACE, "pack do - GetCanonicalFormatEtc() called.");
    
    if (!pFEOut)  
        return E_INVALIDARG;
    
    pFEOut->ptd = NULL;
    return DATA_S_SAMEFORMATETC;
}

    
HRESULT CPackage_IDataObject::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM,
BOOL fRelease) 
{
    HRESULT hr;
    
    DebugMsg(DM_TRACE, "pack do - SetData() called.");

    if ((pFE->cfFormat == CF_FILENAMEW) && (pFE->tymed & (TYMED_HGLOBAL|TYMED_FILE)))
    {
        LPWSTR pwsz = pSTM->tymed == TYMED_HGLOBAL ? (LPWSTR)pSTM->hGlobal : pSTM->lpszFileName;
        
#ifdef UNICODE
        
        hr = _pPackage->CmlInitFromFile(pwsz, TRUE);
        
#else
        CHAR szPath[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, szPath, ARRAYSIZE(szPath), NULL, NULL);
        hr = _pPackage->CmlInitFromFile(szPath, TRUE);
        
#endif // UNICODE

        _pPackage->_pCml->fCmdIsLink = TRUE;
    }

    return DATA_E_FORMATETC;
}

    
HRESULT CPackage_IDataObject::EnumFormatEtc(DWORD dwDirection,
LPENUMFORMATETC *ppEnum)
{
    DebugMsg(DM_TRACE, "pack do - EnumFormatEtc() called.");
    
    // NOTE: This means that we'll have to put the appropriate entries in 
    // the registry for this to work.
    //
    return OleRegEnumFormatEtc(CLSID_CPackage, dwDirection, ppEnum);
}

    
HRESULT CPackage_IDataObject::DAdvise(LPFORMATETC pFE, DWORD grfAdv,
LPADVISESINK pAdvSink, LPDWORD pdwConnection)
{
    HRESULT hr;

    DebugMsg(DM_TRACE, "pack do - DAdvise() called.");
    
    if (_pPackage->_pIDataAdviseHolder == NULL) {
        hr = CreateDataAdviseHolder(&_pPackage->_pIDataAdviseHolder);
        if (FAILED(hr))
            return E_OUTOFMEMORY;
    }
    
    return _pPackage->_pIDataAdviseHolder->Advise(this, pFE, grfAdv, pAdvSink,
	pdwConnection);
}

     
HRESULT CPackage_IDataObject::DUnadvise(DWORD dwConnection)
{
    DebugMsg(DM_TRACE, "pack do - DUnadvise() called.");
    
    if (_pPackage->_pIDataAdviseHolder == NULL) 
        return E_UNEXPECTED;
    
    return _pPackage->_pIDataAdviseHolder->Unadvise(dwConnection);
}

   
HRESULT CPackage_IDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    DebugMsg(DM_TRACE, "pack do - EnumAdvise() called.");
    
    if (_pPackage->_pIDataAdviseHolder == NULL)
        return E_UNEXPECTED;
    
    return _pPackage->_pIDataAdviseHolder->EnumAdvise(ppEnum);
}
