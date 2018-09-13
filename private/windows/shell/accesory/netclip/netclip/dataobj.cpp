// GenericDataObject.cpp : implementation file
//
// This class provides a very generic implementation of IDataObject.
// Call SetData with any FORMATETC and STGMEDIUM and it will cache
// a *copy* and make it available to GetData.
//
// IStream and IStorage based STGMEDIUM's are *copied*. If IStream,
// an IStream on HGLOBAL is created and the source is copied into it.
// if an IStorage, a temp DocFile is created, and the source
// copied into it.
//
// This class is a radical derivation of COleDataSource.
//
#include "stdafx.h"
#include "NetClip.h"
#include "DataObj.h"
#include "guids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLSID_GenericDataObject = {92436B40-6327-11cf-B63C-0080C792B782}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject

// Each time SetData is called for a new format, a new _DATACACHE_ENTRY
// is allocated to the array
struct _DATACACHE_ENTRY
{
	FORMATETC m_formatEtc;
	STGMEDIUM m_stgMedium;
};

DVTARGETDEVICE* CopyTargetDevice(DVTARGETDEVICE* ptdSrc)
{
	if (ptdSrc == NULL)
		return NULL;

	DVTARGETDEVICE* ptdDest =
		(DVTARGETDEVICE*)CoTaskMemAlloc(ptdSrc->tdSize);
	if (ptdDest == NULL)
		return NULL;

	memcpy(ptdDest, ptdSrc, (size_t)ptdSrc->tdSize);
	return ptdDest;
}

void CopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc)
{
	ASSERT(petcDest != NULL);
	ASSERT(petcSrc != NULL);

	petcDest->cfFormat = petcSrc->cfFormat;
	petcDest->ptd = CopyTargetDevice(petcSrc->ptd);
	petcDest->dwAspect = petcSrc->dwAspect;
	petcDest->lindex = petcSrc->lindex;
	petcDest->tymed = petcSrc->tymed;
}

static HGLOBAL CopyGlobalMemory(HGLOBAL hDest, HGLOBAL hSource)
{
	ASSERT(hSource != NULL);

	// make sure we have suitable hDest
	DWORD nSize = (DWORD)::GlobalSize(hSource);
	if (hDest == NULL)
	{
		hDest = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, nSize);
		if (hDest == NULL)
			return NULL;
	}
	else if (nSize > ::GlobalSize(hDest))
	{
		// hDest is not large enough
		return NULL;
	}

	// copy the bits
	LPVOID lpSource = ::GlobalLock(hSource);
	LPVOID lpDest = ::GlobalLock(hDest);
	ASSERT(lpDest != NULL);
	ASSERT(lpSource != NULL);
	memcpy(lpDest, lpSource, nSize);
	::GlobalUnlock(hDest);
	::GlobalUnlock(hSource);

	// success -- return hDest
	return hDest;
}

BOOL CopyStgMedium(
	CLIPFORMAT cfFormat, LPSTGMEDIUM lpDest, LPSTGMEDIUM lpSource)
{
	if (lpDest->tymed == TYMED_NULL)
	{
		ASSERT(lpSource->tymed != TYMED_NULL);
		switch (lpSource->tymed)
		{
		case TYMED_ENHMF:
		case TYMED_HGLOBAL:
			ASSERT(sizeof(HGLOBAL) == sizeof(HENHMETAFILE));
			lpDest->tymed = lpSource->tymed;
			lpDest->hGlobal = NULL;
			break;  // fall through to CopyGlobalMemory case

		case TYMED_ISTREAM:
    		lpDest->tymed = TYMED_ISTREAM;
            lpDest->pstm = NULL;
            CreateStreamOnHGlobal(0, TRUE, &lpDest->pstm);
            ASSERT(lpDest->pstm);
			break ;  // fall through to CopyTo case

		case TYMED_ISTORAGE:
            {
			    lpDest->tymed = TYMED_ISTORAGE;
                lpDest->pstg = NULL;
                HRESULT hr = StgCreateDocfile(NULL,
                        STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_DELETEONRELEASE|STGM_CREATE,
                        0, &lpDest->pstg);
                ASSERT(lpDest->pstg);
                if (FAILED(hr))
                    return FALSE;
            }
            break;// fall through to CopyTo case

		case TYMED_MFPICT:
			{
				// copy LPMETAFILEPICT struct + embedded HMETAFILE
				HGLOBAL hDest = ::CopyGlobalMemory(NULL, lpSource->hGlobal);
				if (hDest == NULL)
					return FALSE;
				LPMETAFILEPICT lpPict = (LPMETAFILEPICT)::GlobalLock(hDest);
				ASSERT(lpPict != NULL);
				lpPict->hMF = ::CopyMetaFile(lpPict->hMF, NULL);
				if (lpPict->hMF == NULL)
				{
					::GlobalUnlock(hDest);
					::GlobalFree(hDest);
					return FALSE;
				}
				::GlobalUnlock(hDest);

				// fill STGMEDIUM struct
				lpDest->hGlobal = hDest;
				lpDest->tymed = TYMED_MFPICT;
			}
			return TRUE;

		case TYMED_GDI:
			lpDest->tymed = TYMED_GDI;
			lpDest->hGlobal = NULL;
			break;

		// unable to create + copy other TYMEDs
		default:
			return FALSE;
		}
	}
	ASSERT(lpDest->tymed == lpSource->tymed);

	switch (lpSource->tymed)
	{
	case TYMED_HGLOBAL:
		{
			HGLOBAL hDest = ::CopyGlobalMemory(lpDest->hGlobal,
				lpSource->hGlobal);
			if (hDest == NULL)
				return FALSE;

			lpDest->hGlobal = hDest;
		}
		return TRUE;

	case TYMED_ISTREAM:
		{
			ASSERT(lpDest->pstm != NULL);
			ASSERT(lpSource->pstm != NULL);

			// get the size of the source stream
			STATSTG stat;
			if (lpSource->pstm->Stat(&stat, STATFLAG_NONAME) != S_OK)
			{
				// unable to get size of source stream
				return FALSE;
			}
			ASSERT(stat.pwcsName == NULL);

			// always seek to zero before copy
			LARGE_INTEGER zero = { 0, 0 };
			lpDest->pstm->Seek(zero, STREAM_SEEK_SET, NULL);
			lpSource->pstm->Seek(zero, STREAM_SEEK_SET, NULL);

			// copy source to destination
			if (lpSource->pstm->CopyTo(lpDest->pstm, stat.cbSize,
				NULL, NULL) != NULL)
			{
				// copy from source to dest failed
				return FALSE;
			}

			// always seek to zero after copy
			lpDest->pstm->Seek(zero, STREAM_SEEK_SET, NULL);
			lpSource->pstm->Seek(zero, STREAM_SEEK_SET, NULL);
		}
		return TRUE;

	case TYMED_ISTORAGE:
		{
			ASSERT(lpDest->pstg != NULL);
			ASSERT(lpSource->pstg != NULL);

			// just copy source to destination
			if (lpSource->pstg->CopyTo(0, NULL, NULL, lpDest->pstg) != S_OK)
				return FALSE;
		}
		return TRUE;

	case TYMED_ENHMF:
	case TYMED_GDI:
		{
			ASSERT(sizeof(HGLOBAL) == sizeof(HENHMETAFILE));

			// with TYMED_GDI cannot copy into existing HANDLE
			if (lpDest->hGlobal != NULL)
				return FALSE;

			// otherwise, use OleDuplicateData for the copy
			lpDest->hGlobal = OleDuplicateData(lpSource->hGlobal, cfFormat, 0);
			if (lpDest->hGlobal == NULL)
				return FALSE;
		}
		return TRUE;

	// other TYMEDs cannot be copied
	default:
		return FALSE;
	}
}


CGenericDataObject::CGenericDataObject(IDataObject* pdoSource)
{
	m_pDataCache = NULL;
	m_nMaxSize = 0;
	m_nSize = 0;
	m_nGrowBy = 10;
    m_bModified = FALSE;
    m_bInitialized = FALSE;

    if (pdoSource == NULL)
        return;

    IDataObject* pdoDest = (IDataObject*)GetInterface(&IID_IDataObject);
    ASSERT(pdoDest);
    ASSERT(pdoSource);

    // For each format copy.
    FORMATETC fetc;
    STGMEDIUM stgm;
    IEnumFORMATETC* penum=NULL;
    HRESULT hr;
    if (SUCCEEDED(hr = pdoSource->EnumFormatEtc(DATADIR_GET, &penum)))
    {
        ULONG ulFetched;
        while (S_OK == penum->Next(1, &fetc, &ulFetched))
        {
            stgm.tymed = fetc.tymed ;
            stgm.hGlobal = NULL;
            stgm.pUnkForRelease = NULL;
            if (SUCCEEDED(hr = pdoSource->GetData(&fetc, &stgm)))
            {
                // Call IDataObject::SetData of our generic dataobject,
                // which will copy the data.  We pass FALSE for fRelease so
                // that the generic dataobject will not try to take
                // ownership, but will copy.
                //
                // We do not test for failure because we don't care. If
                // the copy failed, then there's nothing we can do anyway.
                //
                // fetc.tymed may specify multiple tymeds.  GetData will
                // pick one and return it to us (in stgm).  SetData requires fetc.tymed
                // and stgm.tymed to be equal, so we simply fix up fetc.
                fetc.tymed = stgm.tymed;
                hr = pdoDest->SetData(&fetc, &stgm, FALSE);
#ifdef _DEBUG
                if (FAILED(hr))
                    TRACE(_T("SetData failed. %s"), HRtoString(hr));
#endif
                ReleaseStgMedium(&stgm);
            }
#ifdef _DEBUG
            else
                TRACE(_T("GetData failed. %s"), HRtoString(hr));
#endif

        }
        penum->Release();
    }

    /*
    IInterfaceViewer* piv= NULL;
    hr = CoCreateInstance(CLSID_IDataObjectViewer, NULL, CLSCTX_SERVER, IID_IInterfaceViewer, (void**)&piv);
    if (SUCCEEDED(hr))
    {
        piv->View(AfxGetMainWnd()->GetSafeHwnd(), IID_IDataObject, pdoDest);
        piv->Release();
    }
    else
        ErrorMessage("Could not load viewer.", hr);
    */
}

CGenericDataObject::~CGenericDataObject()
{
    // Go through our internal list releasing everything
	Empty();
}

void CGenericDataObject::Empty()
{
	if (m_pDataCache != NULL)
	{
		ASSERT(m_nMaxSize != 0);
		ASSERT(m_nSize != 0);

		// release all of the STGMEDIUMs and FORMATETCs
		for (UINT nIndex = 0; nIndex < m_nSize; nIndex++)
		{
            if (m_pDataCache[nIndex].m_formatEtc.ptd)
			    CoTaskMemFree(m_pDataCache[nIndex].m_formatEtc.ptd);
			::ReleaseStgMedium(&m_pDataCache[nIndex].m_stgMedium);
		}

		// delete the cache
		delete m_pDataCache;
		m_pDataCache = NULL;
		m_nMaxSize = 0;
		m_nSize = 0;
        m_bModified=TRUE;
	}
	ASSERT(m_pDataCache == NULL);
	ASSERT(m_nMaxSize == 0);
	ASSERT(m_nSize == 0);
}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject cache allocation

_DATACACHE_ENTRY* CGenericDataObject::GetCacheEntry(
	LPFORMATETC lpFormatEtc)
{
	_DATACACHE_ENTRY* pEntry = Lookup(lpFormatEtc);
	if (pEntry != NULL)
	{
		// cleanup current entry and return it
        if (pEntry->m_formatEtc.ptd)
		    CoTaskMemFree(pEntry->m_formatEtc.ptd);
		::ReleaseStgMedium(&pEntry->m_stgMedium);
	}
	else
	{
		// allocate space for item at m_nSize (at least room for 1 item)
		if (m_pDataCache == NULL || m_nSize == m_nMaxSize)
		{
			ASSERT(m_nGrowBy != 0);
			_DATACACHE_ENTRY* pCache = new _DATACACHE_ENTRY[m_nMaxSize+m_nGrowBy];
			m_nMaxSize += m_nGrowBy;
			if (m_pDataCache != NULL)
			{
				memcpy(pCache, m_pDataCache, m_nSize * sizeof(_DATACACHE_ENTRY));
				delete[] m_pDataCache;
			}
			m_pDataCache = pCache;
		}
		ASSERT(m_pDataCache != NULL);
		ASSERT(m_nMaxSize != 0);

		pEntry = &m_pDataCache[m_nSize++];
	}

	// fill the cache entry with the format and return it
	pEntry->m_formatEtc = *lpFormatEtc;
    memset(&pEntry->m_stgMedium, 0, sizeof(STGMEDIUM));
	return pEntry;
}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject operations

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject cache implementation

_DATACACHE_ENTRY* CGenericDataObject::Lookup(
	LPFORMATETC lpFormatEtc) const
{
	// look for suitable match to lpFormatEtc in cache
	for (UINT nIndex = 0; nIndex < m_nSize; nIndex++)
	{
		// get entry from cache at nIndex
		_DATACACHE_ENTRY* pCache = &m_pDataCache[nIndex];
		FORMATETC *pCacheFormat = &pCache->m_formatEtc;

		// check for match
		if (pCacheFormat->cfFormat == lpFormatEtc->cfFormat &&
			(pCacheFormat->tymed & lpFormatEtc->tymed) != 0 &&
			pCacheFormat->lindex == lpFormatEtc->lindex &&
			pCacheFormat->dwAspect == lpFormatEtc->dwAspect)
		{
			// return that cache entry
			return pCache;
		}
	}

	return NULL;    // not found
}

// TODO: Determine whether it makes sense to implement this
// function. It's possible that we can provide more formats
// than are specfied by SetData's (e.g. additional stgmediums per format).
BOOL CGenericDataObject::OnRenderData(
	LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
    /*
	// attempt TYMED_HGLOBAL as prefered format
	if (lpFormatEtc->tymed & TYMED_HGLOBAL)
	{
		// attempt HGLOBAL delay render hook
		HGLOBAL hGlobal = lpStgMedium->hGlobal;
		if (OnRenderGlobalData(lpFormatEtc, &hGlobal))
		{
			ASSERT(lpStgMedium->tymed != TYMED_HGLOBAL ||
				(lpStgMedium->hGlobal == hGlobal));
			ASSERT(hGlobal != NULL);
			lpStgMedium->tymed = TYMED_HGLOBAL;
			lpStgMedium->hGlobal = hGlobal;
			return TRUE;
		}

		// attempt CFile* based delay render hook
		CSharedFile file;
		if (lpStgMedium->tymed == TYMED_HGLOBAL)
		{
			ASSERT(lpStgMedium->hGlobal != NULL);
			file.SetHandle(lpStgMedium->hGlobal, FALSE);
		}
		if (OnRenderFileData(lpFormatEtc, &file))
		{
			lpStgMedium->tymed = TYMED_HGLOBAL;
			lpStgMedium->hGlobal = file.Detach();
			ASSERT(lpStgMedium->hGlobal != NULL);
			return TRUE;
		}
		if (lpStgMedium->tymed == TYMED_HGLOBAL)
			file.Detach();
	}

	// attempt TYMED_ISTREAM format
	if (lpFormatEtc->tymed & TYMED_ISTREAM)
	{
		COleStreamFile file;
		if (lpStgMedium->tymed == TYMED_ISTREAM)
		{
			ASSERT(lpStgMedium->pstm != NULL);
			file.Attach(lpStgMedium->pstm);
		}
		else
		{
			if (!file.CreateMemoryStream())
				AfxThrowMemoryException();
		}
		// get data into the stream
		if (OnRenderFileData(lpFormatEtc, &file))
		{
			lpStgMedium->tymed = TYMED_ISTREAM;
			lpStgMedium->pstm = file.Detach();
			return TRUE;
		}
		if (lpStgMedium->tymed == TYMED_ISTREAM)
			file.Detach();
	}
*/
	return FALSE;   // default does nothing
}


/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject message handlers


/////////////////////////////////////////////////////////////////////////////
// CEnumFormatEtc - enumerator for array for FORMATETC structures
// Stolen from MFC (OLEPRIV)
class CEnumFormatEtc : public CEnumArray
{
// Constructors
public:
	CEnumFormatEtc();

// Operations
	void AddFormat(const FORMATETC* lpFormatEtc);

// Implementation
public:
	virtual ~CEnumFormatEtc();

protected:
	virtual BOOL OnNext(void* pv);

	UINT m_nMaxSize;    // number of items allocated (>= m_nSize)
	DECLARE_INTERFACE_MAP()
};

BEGIN_INTERFACE_MAP(CEnumFormatEtc, CEnumArray)
	INTERFACE_PART(CEnumFormatEtc, IID_IEnumFORMATETC, EnumVOID)
END_INTERFACE_MAP()

CEnumFormatEtc::CEnumFormatEtc()
	: CEnumArray(sizeof(FORMATETC), NULL, 0, TRUE)
{
	m_nMaxSize = 0;
}

CEnumFormatEtc::~CEnumFormatEtc()
{
	if (m_pClonedFrom == NULL)
	{
		// release all of the pointers to DVTARGETDEVICE
		LPFORMATETC lpFormatEtc = (LPFORMATETC)m_pvEnum;
		for (UINT nIndex = 0; nIndex < m_nSize; nIndex++)
			CoTaskMemFree(lpFormatEtc[nIndex].ptd);
	}
	// destructor will free the actual array (if it was not a clone)
}

BOOL CEnumFormatEtc::OnNext(void* pv)
{
	if (!CEnumArray::OnNext(pv))
		return FALSE;

	// any outgoing formatEtc may require the DVTARGETDEVICE to
	//  be copied (the caller has responsibility to free it)
	LPFORMATETC lpFormatEtc = (LPFORMATETC)pv;
	if (lpFormatEtc->ptd != NULL)
	{
		lpFormatEtc->ptd = CopyTargetDevice(lpFormatEtc->ptd);
		if (lpFormatEtc->ptd == NULL)
			AfxThrowMemoryException();
	}
	// otherwise, copying worked...
	return TRUE;
}

void CEnumFormatEtc::AddFormat(const FORMATETC* lpFormatEtc)
{
	ASSERT(m_nSize <= m_nMaxSize);

	if (m_nSize == m_nMaxSize)
	{
		// not enough space for new item -- allocate more
		FORMATETC* pListNew = new FORMATETC[m_nSize+10];
		m_nMaxSize += 10;
		memcpy(pListNew, m_pvEnum, m_nSize*sizeof(FORMATETC));
		delete m_pvEnum;
		m_pvEnum = (BYTE*)pListNew;
	}

	// add this item to the list
	ASSERT(m_nSize < m_nMaxSize);
	FORMATETC* pFormat = &((FORMATETC*)m_pvEnum)[m_nSize];
	pFormat->cfFormat = lpFormatEtc->cfFormat;
	pFormat->ptd = lpFormatEtc->ptd;
		// Note: ownership of lpFormatEtc->ptd is transfered with this call.
	pFormat->dwAspect = lpFormatEtc->dwAspect;
	pFormat->lindex = lpFormatEtc->lindex;
	pFormat->tymed = lpFormatEtc->tymed;
	++m_nSize;
}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject::XDataObject

BEGIN_INTERFACE_MAP(CGenericDataObject, CCmdTarget)
	INTERFACE_PART(CGenericDataObject, IID_IDataObject, DataObject)
	INTERFACE_PART(CGenericDataObject, IID_IPersistStorage, PersistStorage)
END_INTERFACE_MAP()

STDMETHODIMP_(ULONG) CGenericDataObject::XDataObject::AddRef()
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, DataObject)
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CGenericDataObject::XDataObject::Release()
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, DataObject)
	return pThis->ExternalRelease();
}

STDMETHODIMP CGenericDataObject::XDataObject::QueryInterface(
	REFIID iid, LPVOID* ppvObj)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, DataObject)
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}

STDMETHODIMP CGenericDataObject::XDataObject::GetData(
	LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
	METHOD_PROLOGUE_EX(CGenericDataObject, DataObject)
	ASSERT_VALID(pThis);

	// attempt to find match in the cache
	_DATACACHE_ENTRY* pCache = pThis->Lookup(lpFormatEtc);
	if (pCache == NULL)
		return DATA_E_FORMATETC;

	// use cache if entry is not delay render
	memset(lpStgMedium, 0, sizeof(STGMEDIUM));
	if (pCache->m_stgMedium.tymed != TYMED_NULL)
	{
		// Copy the cached medium into the lpStgMedium provided by caller.
		if (!CopyStgMedium(lpFormatEtc->cfFormat, lpStgMedium,
		  &pCache->m_stgMedium))
			return DATA_E_FORMATETC;

		// format was supported for copying
		return S_OK;
	}

	SCODE sc = DATA_E_FORMATETC;
	try
	{
		// attempt LPSTGMEDIUM based delay render
		if (pThis->OnRenderData(lpFormatEtc, lpStgMedium))
			sc = S_OK;
	}
	catch(CException*e)
	{
		sc = COleException::Process(e);
		e->Delete();
	}

	return sc;
}

STDMETHODIMP CGenericDataObject::XDataObject::GetDataHere(
	LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
	METHOD_PROLOGUE_EX(CGenericDataObject, DataObject)
	ASSERT_VALID(pThis);

	// these two must be the same
	ASSERT(lpFormatEtc->tymed == lpStgMedium->tymed);
	lpFormatEtc->tymed = lpStgMedium->tymed;    // but just in case...

	// attempt to find match in the cache
	_DATACACHE_ENTRY* pCache = pThis->Lookup(lpFormatEtc);
	if (pCache == NULL)
		return DATA_E_FORMATETC;

	// handle cached medium and copy
	if (pCache->m_stgMedium.tymed != TYMED_NULL)
	{
		// found a cached format -- copy it to dest medium
		ASSERT(pCache->m_stgMedium.tymed == lpStgMedium->tymed);
		if (!CopyStgMedium(lpFormatEtc->cfFormat, lpStgMedium,
		  &pCache->m_stgMedium))
			return DATA_E_FORMATETC;

		// format was supported for copying
		return S_OK;
	}

	SCODE sc = DATA_E_FORMATETC;
	try
	{
		// attempt LPSTGMEDIUM based delay render
		if (pThis->OnRenderData(lpFormatEtc, lpStgMedium))
			sc = S_OK;
	}
	catch(CException*e)
	{
		sc = COleException::Process(e);
		e->Delete();
	}

	return sc;
}

STDMETHODIMP CGenericDataObject::XDataObject::QueryGetData(LPFORMATETC lpFormatEtc)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, DataObject)

	// attempt to find match in the cache
	_DATACACHE_ENTRY* pCache = pThis->Lookup(lpFormatEtc);
	if (pCache == NULL)
		return DATA_E_FORMATETC;

	// it was found in the cache or can be rendered -- success
	return S_OK;
}

STDMETHODIMP CGenericDataObject::XDataObject::GetCanonicalFormatEtc(
	LPFORMATETC /*lpFormatEtcIn*/, LPFORMATETC /*lpFormatEtcOut*/)
{
	// because we support the target-device (ptd) for server metafile format,
	//  all members of the FORMATETC are significant.

	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CGenericDataObject::XDataObject::SetData(
	LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium, BOOL bRelease)
{
	METHOD_PROLOGUE_EX(CGenericDataObject, DataObject)
	ASSERT_VALID(pThis);

	ASSERT(lpFormatEtc->tymed == lpStgMedium->tymed);

    // BUGBUG: We don't know how to deal with this yet..
    if (bRelease == TRUE)
        return E_INVALIDARG;

	// attempt to find match in the cache
	_DATACACHE_ENTRY* pCache = pThis->Lookup(lpFormatEtc);
	if (pCache == NULL)
    {
        // Not found, so create one
        //
        pCache = pThis->GetCacheEntry(lpFormatEtc);
        pCache->m_stgMedium.tymed = NULL;
        pCache->m_stgMedium.hGlobal = NULL;
        ASSERT(pCache);
        if (pCache ==NULL)
    		return DV_E_FORMATETC;
    }

//	ASSERT(pCache->m_stgMedium.tymed == TYMED_NULL);

	SCODE sc = E_UNEXPECTED;
	try
	{
	    if (CopyStgMedium(lpFormatEtc->cfFormat, &pCache->m_stgMedium, lpStgMedium))
    	    sc = S_OK;
	}
	catch(CException*e)
	{
		sc = COleException::Process(e);
		e->Delete();
	}

    if (sc == S_OK)
        pThis->m_bModified = TRUE;
	return sc;
}

STDMETHODIMP CGenericDataObject::XDataObject::EnumFormatEtc(
	DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, DataObject)

	*ppenumFormatEtc = NULL;

	CEnumFormatEtc* pFormatList = NULL;
	SCODE sc = E_OUTOFMEMORY;
	try
	{
		// generate a format list from the cache
		pFormatList = new CEnumFormatEtc;
		for (UINT nIndex = 0; nIndex < pThis->m_nSize; nIndex++)
		{
			_DATACACHE_ENTRY* pCache = &pThis->m_pDataCache[nIndex];
			{
				// entry should be enumerated -- add it to the list
				FORMATETC formatEtc;
				CopyFormatEtc(&formatEtc, &pCache->m_formatEtc);
				pFormatList->AddFormat(&formatEtc);
			}
		}
		// give it away to OLE (ref count is already 1)
		*ppenumFormatEtc = (LPENUMFORMATETC)&pFormatList->m_xEnumVOID;
		sc = S_OK;
	}
    catch(...)
    {
    }

	return sc;
}

STDMETHODIMP CGenericDataObject::XDataObject::DAdvise(
	FORMATETC* /*pFormatetc*/, DWORD /*advf*/,
	LPADVISESINK /*pAdvSink*/, DWORD* pdwConnection)
{
	*pdwConnection = 0;
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CGenericDataObject::XDataObject::DUnadvise(DWORD /*dwConnection*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CGenericDataObject::XDataObject::EnumDAdvise(
	LPENUMSTATDATA* ppenumAdvise)
{
	*ppenumAdvise = NULL;
	return OLE_E_ADVISENOTSUPPORTED;
}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject::XPersistStorage

STDMETHODIMP_(ULONG) CGenericDataObject::XPersistStorage::AddRef()
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)
	return (ULONG)pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CGenericDataObject::XPersistStorage::Release()
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)
	return (ULONG)pThis->ExternalRelease();
}

STDMETHODIMP CGenericDataObject::XPersistStorage::QueryInterface(
	REFIID iid, LPVOID* ppvObj)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)
	return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

STDMETHODIMP CGenericDataObject::XPersistStorage::GetClassID(LPCLSID lpClassID)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)
    // BUGBUG: Can I do this?
	return E_NOTIMPL;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::IsDirty()
{
	// Return S_OK if modified, and S_FALSE otherwise.
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)

	return (pThis->m_bModified) ? S_OK : S_FALSE;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::InitNew(LPSTORAGE pStg)
{
	METHOD_PROLOGUE_EX(CGenericDataObject, PersistStorage)

	pThis->Empty();
	pThis->m_bInitialized = TRUE;
	pThis->m_bModified = FALSE;
    WriteClassStg(pStg, CLSID_GenericDataObject);

	return S_OK;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::Load(LPSTORAGE pStg)
{
	ASSERT(pStg != NULL);
	METHOD_PROLOGUE_EX(CGenericDataObject, PersistStorage)
	HRESULT hr;
    USES_CONVERSION;

    pThis->Empty();

    CLSID clsid;
    // Verify it's our storage
    ReadClassStg(pStg, &clsid);
    if (clsid != CLSID_GenericDataObject)
        return E_FAIL;

	// Open the "Contents" stream of the supplied storage object
	LPSTREAM pStm = NULL;
	hr = pStg->OpenStream(L"CONTENTS", NULL,
		STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStm);

	ASSERT(FAILED(hr) || pStm != NULL);

	if (pStm == NULL)
        return hr;

    // The CONTENTs stream is formated thus:
    //   dwFormatCount
    // followed by dwFormatCount instances of:
    //   FORMATETC
    //   DWORD cb + string (if fetc.cfFormat non-native)
    //   DVTARGETDEVICE (if fetc.ptd != NULL)
    // From this information, the location of the
    // actual data is gleaned. Sub Stream & Storages are
    // named numerically by their dwFormatCount index.
    try
    {
        HRESULT hr;
        DWORD dwFormatCount ;
        hr = pStm->Read(&dwFormatCount, sizeof(DWORD), NULL);
        if (hr != S_OK)
            AfxThrowOleException(hr);

        FORMATETC fetc;
        _DATACACHE_ENTRY* pEntry ;
        // Open sub stream or storage and read
	    IStream* pstmData = NULL;
        IStorage* pstgData = NULL;
        TCHAR szName[32];
	    LARGE_INTEGER zero = { 0, 0 };
        try
        {

    		for (UINT nIndex = 0; nIndex < dwFormatCount; nIndex++)
            {
                fetc.ptd = NULL;
                pEntry = NULL;
	            pstmData = NULL;
                pstgData = NULL;

                hr = pStm->Read(&fetc, sizeof(FORMATETC), NULL);
                if (hr != S_OK)
                    AfxThrowOleException(hr);

                if (fetc.cfFormat >= 0xC000)
                {
                    // non-native
                    DWORD cb;
                    hr = pStm->Read(&cb, sizeof(DWORD), NULL);
                    if (hr != S_OK)
                        AfxThrowOleException(hr);
                    WCHAR* pwsz = new WCHAR[cb];
                    hr = pStm->Read(pwsz, cb, NULL);
                    if (hr == S_OK)
                        fetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(OLE2T(pwsz));
                    delete []pwsz;
                    if (hr != S_OK || fetc.cfFormat == 0)
                        AfxThrowOleException(E_FAIL);

                }

                // If .ptd is non-NULL then we need to read the
                // DVTARGETDEVICE info and setup .ptd correctly
                if (fetc.ptd != NULL)
                {
                    DWORD dwSize;
                    hr = pStm->Read(&dwSize, sizeof(DWORD), NULL);
                    if (hr != S_OK)
                        AfxThrowOleException(hr);
                    ASSERT(dwSize >= sizeof(DVTARGETDEVICE));

                    fetc.ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(dwSize);
                    ASSERT(fetc.ptd);
                    fetc.ptd->tdSize = dwSize;

                    hr = pStm->Read(((BYTE*)fetc.ptd)+sizeof(DWORD), dwSize-sizeof(DWORD), NULL);
                    if (hr != S_OK)
                        AfxThrowOleException(hr);
                }

                wsprintf(szName, _T("%d"), nIndex);
                switch(fetc.tymed)
                {
		        case TYMED_ENHMF:   // stm contains enhmetafile
		        case TYMED_HGLOBAL: // stm contains data
		        case TYMED_ISTREAM: // stm
                case TYMED_GDI:     // stm contains DIB
                case TYMED_MFPICT:  // stm contains metafile
                case TYMED_FILE:    // stm contains filename
	                hr = pStg->OpenStream(T2OLE(szName), NULL,
		                STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pstmData);
	                ASSERT(FAILED(hr) || pstmData != NULL);
                    if (FAILED(hr))
                        AfxThrowOleException(hr);
                    break;

                case TYMED_ISTORAGE:
	                hr = pStg->OpenStorage(T2OLE(szName), NULL,
		                STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &pstgData);
	                ASSERT(FAILED(hr) || pstgData != NULL);
                    if (FAILED(hr))
                        AfxThrowOleException(hr);
                    break;

                default:
                    AfxThrowOleException(E_FAIL);
                }

                pEntry = pThis->GetCacheEntry(&fetc);
                if (pEntry==NULL)
                    AfxThrowOleException(E_OUTOFMEMORY);

                pEntry->m_stgMedium.tymed = fetc.tymed;
                // Now read from the stream or storage
                switch(fetc.tymed)
                {
		        case TYMED_ENHMF:   // stm contains enhmetafile
                case TYMED_GDI:     // stm contains DIB
                case TYMED_MFPICT:  // stm contains metafile
                case TYMED_FILE:    // stm contains filename
		        case TYMED_HGLOBAL: // stm contains data
		        case TYMED_ISTREAM: // stm
                    {
			            // get the size of the source stream
			            STATSTG stat;
			            if (pstmData->Stat(&stat, STATFLAG_NONAME) != S_OK)
				            // unable to get size of source stream
                            AfxThrowOleException(hr);
			            ASSERT(stat.pwcsName == NULL);

			            // always seek to zero before copy
			            pstmData->Seek(zero, STREAM_SEEK_SET, NULL);

                        if (fetc.tymed == TYMED_ISTREAM)
                        {
                            hr = CreateStreamOnHGlobal(NULL, TRUE, &pEntry->m_stgMedium.pstm);
                            if (FAILED(hr))
                                AfxThrowOleException(hr);

                            hr = pstmData->CopyTo(pEntry->m_stgMedium.pstm, stat.cbSize, NULL, NULL);
                            if (FAILED(hr))
                            {
                                pEntry->m_stgMedium.pstm->Release();
                                AfxThrowOleException(hr);
                            }

			                // always seek to zero after copy
			                pEntry->m_stgMedium.pstm->Seek(zero, STREAM_SEEK_SET, NULL);
                        }
                        else
                        {
                            pEntry->m_stgMedium.hGlobal = GlobalAlloc(GHND, stat.cbSize.LowPart);
                            ASSERT(pEntry->m_stgMedium.hGlobal );

                            BYTE* pb = (BYTE*)GlobalLock(pEntry->m_stgMedium.hGlobal);
                            hr = pstmData->Read(pb, stat.cbSize.LowPart, NULL);

                            GlobalUnlock(pEntry->m_stgMedium.hGlobal);
                            if (hr != S_OK)
                            {
                                GlobalFree(pEntry->m_stgMedium.hGlobal);
                                AfxThrowOleException(hr);
                            }

                            if (fetc.tymed == TYMED_FILE)
                            {
                                HGLOBAL h = pEntry->m_stgMedium.hGlobal;
                                pb = (BYTE*)GlobalLock(h);
                                pEntry->m_stgMedium.lpszFileName = (LPWSTR)CoTaskMemAlloc(stat.cbSize.LowPart);
                                ASSERT(pEntry->m_stgMedium.lpszFileName);
                                memcpy(pEntry->m_stgMedium.lpszFileName, pb, stat.cbSize.LowPart);
                                GlobalUnlock(h);
                                GlobalFree(h);
                            }
                        }

			            // always seek to zero after copy
			            pstmData->Seek(zero, STREAM_SEEK_SET, NULL);
                    }
                    break;


                case TYMED_ISTORAGE:
                    hr = StgCreateDocfile(NULL,
                        STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_DELETEONRELEASE|STGM_CREATE,
                        0, &pEntry->m_stgMedium.pstg);
                    if (FAILED(hr))
                        AfxThrowOleException(hr);

                    hr = pstgData->CopyTo(0, NULL, NULL, pEntry->m_stgMedium.pstg);
                    if (FAILED(hr))
                    {
                        pEntry->m_stgMedium.pstg->Release();
                        AfxThrowOleException(hr);
                    }
                    break;

                default:
                    AfxThrowOleException(E_FAIL);
                }
                if (pstmData)
                {
                    pstmData->Release();
                    pstmData = NULL;
                }
                if (pstgData)
                {
                    pstgData->Release();
                    pstgData = NULL;
                }
            } // for(nIndex...)
        }
        catch(...)
        {
            if (fetc.ptd != NULL)
                CoTaskMemFree(fetc.ptd);
            if (pstmData)
                pstmData->Release();
            if (pstgData)
                pstgData->Release();
            throw;
        }

        pThis->m_bModified = FALSE;
    }
    catch(COleException* e)
    {
        if (e->m_sc == S_FALSE)
            hr = E_FAIL;
        else
            hr = e->m_sc;
        e->Delete();
    }
    pStm->Release();
	return hr;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::Save(LPSTORAGE pStg,
	BOOL fSameAsLoad)
{
	METHOD_PROLOGUE_EX(CGenericDataObject, PersistStorage)
	ASSERT(pStg != NULL);
    USES_CONVERSION;

	// Create a "Contents" stream on the supplied storage object

	// Don't bother saving if destination is up-to-date.
	if (fSameAsLoad && (IsDirty() != S_OK))
		return S_OK;

    WriteClassStg(pStg, CLSID_GenericDataObject);

	LPSTREAM pStm = NULL;
	HRESULT hr = pStg->CreateStream(L"CONTENTS",
		STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStm);

	ASSERT(FAILED(hr) || pStm != NULL);

	if (pStm == NULL)
        return hr;

	IStream* pstmData = NULL;
    IStorage* pstgData = NULL;
    TCHAR szName[32];
	LARGE_INTEGER zero = { 0, 0 };
    try
	{
		// Save
        UINT nName = 0;
        // Write out the first DWORD as zero.  After we're done, we seek back
        // and write the actual amount.
        pStm->Write(&nName, sizeof(nName), NULL);
		for (UINT nIndex = 0; nIndex < pThis->m_nSize; nIndex++)
		{
            // skip things we don't know how to persist reliably
            if (pThis->m_pDataCache[nIndex].m_formatEtc.tymed == TYMED_GDI)
                continue;

            // write out formatetc
            pStm->Write(&pThis->m_pDataCache[nIndex].m_formatEtc, sizeof(FORMATETC), NULL);
            if (pThis->m_pDataCache[nIndex].m_formatEtc.cfFormat >= 0xC000)
            {
                TCHAR sz[256];
                if (!GetClipboardFormatName(pThis->m_pDataCache[nIndex].m_formatEtc.cfFormat, sz, 256))
                    AfxThrowOleException(E_FAIL);

                OLECHAR* osz = T2OLE(sz);
                DWORD cb = (wcslen(osz)+1) * sizeof(OLECHAR) ;
                pStm->Write(&cb, sizeof(DWORD), NULL);
                pStm->Write(osz, cb, NULL);
            }

            if (pThis->m_pDataCache[nIndex].m_formatEtc.ptd != NULL)
            {
                pStm->Write(pThis->m_pDataCache[nIndex].m_formatEtc.ptd,
                            pThis->m_pDataCache[nIndex].m_formatEtc.ptd->tdSize, NULL);
            }

            // write out m_pDataCache[nIndex].m_stgMedium
            wsprintf(szName, _T("%d"), nName++);
            switch(pThis->m_pDataCache[nIndex].m_stgMedium.tymed)
            {
            case TYMED_GDI:     // stm contains DIB
		    case TYMED_ENHMF:   // stm contains enhmetafile
		    case TYMED_HGLOBAL: // stm contains data
		    case TYMED_ISTREAM: // stm
            case TYMED_MFPICT:  // stm contains metafile
            case TYMED_FILE:    // stm contains filename
	            hr = pStg->CreateStream(T2OLE(szName),
		            STGM_CREATE|STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0,0, &pstmData);
	            ASSERT(FAILED(hr) || pstmData != NULL);
                if (FAILED(hr))
                    AfxThrowOleException(hr);

                if (pThis->m_pDataCache[nIndex].m_stgMedium.tymed == TYMED_ISTREAM)
                {
			        // get the size of the source stream
			        STATSTG stat;
			        if (pThis->m_pDataCache[nIndex].m_stgMedium.pstm->Stat(&stat, STATFLAG_NONAME) != S_OK)
			        {
				        // unable to get size of source stream
				        return FALSE;
			        }
			        ASSERT(stat.pwcsName == NULL);

			        // always seek to zero before copy
			        LARGE_INTEGER zero = { 0, 0 };
			        pThis->m_pDataCache[nIndex].m_stgMedium.pstm->Seek(zero, STREAM_SEEK_SET, NULL);
                    pThis->m_pDataCache[nIndex].m_stgMedium.pstm->CopyTo(pstmData, stat.cbSize, NULL, NULL);

                    pThis->m_pDataCache[nIndex].m_stgMedium.pstm->Seek(zero, STREAM_SEEK_SET, NULL);
                }
                else if (pThis->m_pDataCache[nIndex].m_stgMedium.tymed == TYMED_FILE)
                {
                    pstmData->Write(pThis->m_pDataCache[nIndex].m_stgMedium.lpszFileName,
                                     (wcslen(pThis->m_pDataCache[nIndex].m_stgMedium.lpszFileName)+1)*2, NULL);
                }
                else
                {
                    BYTE* pb = (BYTE*)GlobalLock(pThis->m_pDataCache[nIndex].m_stgMedium.hGlobal);
                    ASSERT(pb);
                    pstmData->Write(pb, (DWORD)GlobalSize(pThis->m_pDataCache[nIndex].m_stgMedium.hGlobal), NULL);
                    GlobalUnlock(pThis->m_pDataCache[nIndex].m_stgMedium.hGlobal);
                }
			    pstmData->Seek(zero, STREAM_SEEK_SET, NULL);
                pstmData->Release();
                break;

            case TYMED_ISTORAGE:
	            hr = pStg->CreateStorage(T2OLE(szName),
		            STGM_CREATE|STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, NULL, &pstgData);
	            ASSERT(FAILED(hr) || pstgData != NULL);
                if (FAILED(hr))
                    AfxThrowOleException(hr);
                pThis->m_pDataCache[nIndex].m_stgMedium.pstg->CopyTo(NULL, NULL, NULL, pstgData);
                pstgData->Release();
                break;

            default:
                AfxThrowOleException(E_FAIL);
            }
		}

        // Write out the actual # of entries written
        //
        pStm->Seek(zero, STREAM_SEEK_SET, NULL);
        pStm->Write(&nName, sizeof(nName), NULL);

		// Bookkeeping:  Clear the dirty flag, if storage is same.
		if (fSameAsLoad)
			pThis->m_bModified = FALSE;
	}
    catch(COleException* e)
    {
        if (e->m_sc == S_FALSE)
            hr = E_FAIL;
        else
            hr = e->m_sc;

        e->Delete();
    }
    pStm->Release();

	return hr;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::SaveCompleted(LPSTORAGE pStgSaved)
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)

	//if (pStgSaved != NULL)
	//	pThis->m_bModified = FALSE;

	return E_NOTIMPL;
}

STDMETHODIMP CGenericDataObject::XPersistStorage::HandsOffStorage()
{
	METHOD_PROLOGUE_EX_(CGenericDataObject, PersistStorage)
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject diagnostics

#ifdef _DEBUG
void CGenericDataObject::AssertValid() const
{
	CCmdTarget::AssertValid();
	ASSERT(m_nSize <= m_nMaxSize);
	ASSERT(m_nMaxSize != 0 || m_pDataCache == NULL);
}

void CGenericDataObject::Dump(CDumpContext& dc) const
{
	CCmdTarget::Dump(dc);

	dc << "m_nMaxSize = " << m_nMaxSize;
	dc << "\nm_nSize = " << m_nSize;
	dc << "\nm_pDataCache = " << m_pDataCache;

	for (UINT n = 0; n < m_nSize; n++)
	{
		dc << "\n\tentry [" << n << "] = {";
		_DATACACHE_ENTRY& rEntry = m_pDataCache[n];
		dc << "\n\t m_formatEtc.cfFormat = " << rEntry.m_formatEtc.cfFormat;
		dc << "\n\t m_formatEtc.pdt = " << rEntry.m_formatEtc.ptd;
		dc << "\n\t m_formatEtc.dwAspect = " << rEntry.m_formatEtc.dwAspect;
		dc << "\n\t m_formatEtc.lindex = " << rEntry.m_formatEtc.lindex;
		dc << "\n\t m_formatEtc.tymed = " << rEntry.m_formatEtc.tymed;
		dc << "\n\t m_stgMedium.tymed = " << rEntry.m_stgMedium.tymed;
		dc << "\n\t}";
	}

	dc << "\n";
}
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// CEnumArray (provides OLE enumerator for arbitrary items in an array)
//
// Stolen from MFC (OLEIMPL2.H)
//
CEnumArray::CEnumArray(size_t nSizeElem, const void* pvEnum, UINT nSize,
	BOOL bNeedFree)
{
	m_nSizeElem = nSizeElem;
	m_pClonedFrom = NULL;

	m_nCurPos = 0;
	m_nSize = nSize;
	m_pvEnum = (BYTE*)pvEnum;
	m_bNeedFree = bNeedFree;

	ASSERT_VALID(this);
}

CEnumArray::~CEnumArray()
{
	ASSERT_VALID(this);

	// release the clone pointer (only for clones)
	if (m_pClonedFrom != NULL)
	{
		m_pClonedFrom->InternalRelease();
		ASSERT(!m_bNeedFree);
	}

	// release the pointer (should only happen on non-clones)
	if (m_bNeedFree)
	{
		ASSERT(m_pClonedFrom == NULL);
		delete m_pvEnum;
	}
}

BOOL CEnumArray::OnNext(void* pv)
{
	ASSERT_VALID(this);

	if (m_nCurPos >= m_nSize)
		return FALSE;

	memcpy(pv, &m_pvEnum[m_nCurPos*m_nSizeElem], m_nSizeElem);
	++m_nCurPos;
	return TRUE;
}

BOOL CEnumArray::OnSkip()
{
	ASSERT_VALID(this);

	if (m_nCurPos >= m_nSize)
		return FALSE;

	return ++m_nCurPos < m_nSize;
}

void CEnumArray::OnReset()
{
	ASSERT_VALID(this);

	m_nCurPos = 0;
}

CEnumArray* CEnumArray::OnClone()
{
	ASSERT_VALID(this);

	// set up an exact copy of this object
	//  (derivatives may have to replace this code)
	CEnumArray* pClone;
	pClone = new CEnumArray(m_nSizeElem, m_pvEnum, m_nSize);
	ASSERT(pClone != NULL);
	ASSERT(!pClone->m_bNeedFree);   // clones should never free themselves
	pClone->m_nCurPos = m_nCurPos;

	// finally, return the clone to OLE
	ASSERT_VALID(pClone);
	return pClone;
}

/////////////////////////////////////////////////////////////////////////////
// CEnumArray::XEnumVOID implementation

STDMETHODIMP_(ULONG) CEnumArray::XEnumVOID::AddRef()
{
	METHOD_PROLOGUE_EX_(CEnumArray, EnumVOID)
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CEnumArray::XEnumVOID::Release()
{
	METHOD_PROLOGUE_EX_(CEnumArray, EnumVOID)
	return pThis->ExternalRelease();
}

STDMETHODIMP CEnumArray::XEnumVOID::QueryInterface(
	REFIID iid, LPVOID* ppvObj)
{
	METHOD_PROLOGUE_EX_(CEnumArray, EnumVOID)
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}

STDMETHODIMP CEnumArray::XEnumVOID::Next(
	ULONG celt, void* reelt, ULONG* pceltFetched)
{
	METHOD_PROLOGUE_EX(CEnumArray, EnumVOID);
	ASSERT_VALID(pThis);

	if (pceltFetched != NULL)
		*pceltFetched = 0;

	ASSERT(celt > 0);
	ASSERT(celt == 1 || pceltFetched != NULL);

	BYTE* pchCur = (BYTE*)reelt;
	ULONG nFetched = 0;

	ULONG celtT = celt;
	SCODE sc = E_UNEXPECTED;
	try
	{
		while (celtT != 0 && pThis->OnNext((void*)pchCur))
		{
			pchCur += pThis->m_nSizeElem;
			--celtT;
		}
		if (pceltFetched != NULL)
			*pceltFetched = celt - celtT;
		sc = celtT == 0 ? S_OK : S_FALSE;
	}
    catch(...)
    {
    }
	
	return sc;
}

STDMETHODIMP CEnumArray::XEnumVOID::Skip(ULONG celt)
{
	METHOD_PROLOGUE_EX(CEnumArray, EnumVOID);
	ASSERT_VALID(pThis);

	ULONG celtT = celt;
	SCODE sc = E_UNEXPECTED;
	try
	{
		while (celtT != 0 && pThis->OnSkip())
			--celtT;
		sc = celtT == 0 ? S_OK : S_FALSE;
	}
    catch(...)
    {
    }

	return celtT != 0 ? S_FALSE : S_OK;
}

STDMETHODIMP CEnumArray::XEnumVOID::Reset()
{
	METHOD_PROLOGUE_EX(CEnumArray, EnumVOID);
	ASSERT_VALID(pThis);

	pThis->OnReset();
	return S_OK;
}

STDMETHODIMP CEnumArray::XEnumVOID::Clone(IEnumVOID** ppenm)
{
	METHOD_PROLOGUE_EX(CEnumArray, EnumVOID);
	ASSERT_VALID(pThis);

	*ppenm = NULL;

	SCODE sc = E_UNEXPECTED;
	try
	{
		CEnumArray* pEnumHelper = pThis->OnClone();
		ASSERT_VALID(pEnumHelper);

		// we use an extra reference to keep the original object alive
		//  (the extra reference is removed in the clone's destructor)
		if (pThis->m_pClonedFrom != NULL)
			pEnumHelper->m_pClonedFrom = pThis->m_pClonedFrom;
		else
			pEnumHelper->m_pClonedFrom = pThis;
		pEnumHelper->m_pClonedFrom->InternalAddRef();
		*ppenm = &pEnumHelper->m_xEnumVOID;

		sc = S_OK;
	}
    catch(...)
    {
    }

	return sc;
}

/////////////////////////////////////////////////////////////////////////////
