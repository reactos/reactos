#include "mslocusr.h"
#include "msluglob.h"

#include <ole2.h>

CLUEnum::CLUEnum(CLUDatabase *pDB)
	: m_cRef(1),
	  m_hkeyDB(NULL),
	  m_papszNames(NULL),
	  m_cNames(0),
	  m_cAlloc(0),
	  m_iCurrent(0),
	  m_pDB(pDB)
{
	m_pDB->AddRef();
    RefThisDLL(TRUE);
}


CLUEnum::~CLUEnum(void)
{
	Cleanup();
	if (m_pDB != NULL)
		m_pDB->Release();
    RefThisDLL(FALSE);
}


#define cpPerAlloc 16		/* allocate 16 pointers at a go */
HRESULT CLUEnum::Init(void)
{
	UINT err = (UINT)RegOpenKey(HKEY_LOCAL_MACHINE, ::szProfileList, &m_hkeyDB);
	if (err != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(err);

	m_papszNames = (LPSTR *)::MemAlloc(cpPerAlloc * sizeof(LPCSTR));	/* not "new", so we can realloc */
	if (m_papszNames == NULL)
		return ResultFromScode(E_OUTOFMEMORY);
	m_cAlloc = cpPerAlloc;

	NLS_STR nlsTempName(cchMaxUsername+1);

	err = nlsTempName.QueryError();
	if (err)
		return HRESULT_FROM_WIN32(err);

	for (DWORD iSubkey=0; err == ERROR_SUCCESS; iSubkey++) {
		DWORD cbBuffer = nlsTempName.QueryAllocSize();
		err = (UINT)RegEnumKey(m_hkeyDB, iSubkey, nlsTempName.Party(), cbBuffer);
		nlsTempName.DonePartying();		
		if (err == ERROR_SUCCESS) {
			if (m_cNames == m_cAlloc) {
				LPSTR *pNew = (LPSTR *)::MemReAlloc(m_papszNames, (m_cAlloc + cpPerAlloc) * sizeof(LPCSTR));
				if (pNew == NULL) {
					err = ERROR_NOT_ENOUGH_MEMORY;
					break;
				}
				m_cAlloc += cpPerAlloc;
				m_papszNames = pNew;
			}
			LPSTR pszNewName = new char[nlsTempName.strlen()+1];
			if (pszNewName == NULL) {
				err = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
			::strcpyf(pszNewName, nlsTempName.QueryPch());
			m_papszNames[m_cNames++] = pszNewName;
		}
	}

	return NOERROR;
}


void CLUEnum::Cleanup(void)
{
	if (m_hkeyDB != NULL) {
		RegCloseKey(m_hkeyDB);
		m_hkeyDB = NULL;
	}

	if (m_papszNames != NULL) {
		for (UINT i=0; i<m_cNames; i++) {
			delete m_papszNames[i];
		}
		::MemFree(m_papszNames);
		m_papszNames = NULL;
		m_cNames = 0;
	}
}


STDMETHODIMP CLUEnum::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	if (!IsEqualIID(riid, IID_IUnknown) &&
		!IsEqualIID(riid, IID_IEnumUnknown)) {
        *ppvObj = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}

	*ppvObj = this;
	AddRef();
	return NOERROR;
}


STDMETHODIMP_(ULONG) CLUEnum::AddRef(void)
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) CLUEnum::Release(void)
{
	ULONG cRef;

	cRef = --m_cRef;

	if (0L == m_cRef) {
		delete this;
	}

	return cRef;
}


STDMETHODIMP CLUEnum::Next(ULONG celt, IUnknown __RPC_FAR *__RPC_FAR *rgelt,
						   ULONG __RPC_FAR *pceltFetched)
{
	ULONG celtFetched = 0;
	HRESULT hres = ResultFromScode(S_OK);

	while (celt) {
		if (m_iCurrent == m_cNames) {
			hres = ResultFromScode(S_FALSE);
			break;
		}

		CLUUser *pUser = new CLUUser(m_pDB);
		if (pUser == NULL) {
			hres = ResultFromScode(E_OUTOFMEMORY);
		}
		else {
			hres = pUser->Init(m_papszNames[m_iCurrent]);
		}
		if (FAILED(hres)) {
			for (ULONG i=0; i<celtFetched; i++) {
				rgelt[i]->Release();
			}
			celtFetched = 0;
			if (pUser != NULL)
				pUser->Release();
			break;
		}

		m_iCurrent++;

        /* If this is a name without a real profile attached, don't return
         * it, just go on to the next one.  Advancing m_iCurrent (the index
         * into the name array) above ensures that we won't infinite-loop.
         */
        if (!pUser->Exists()) {
            pUser->Release();
            continue;
        }

		rgelt[celtFetched++] = pUser;
		celt--;
	}

	if (pceltFetched != NULL)
		*pceltFetched = celtFetched;

	return hres;
}


STDMETHODIMP CLUEnum::Skip(ULONG celt)
{
	SCODE sc;

	if (m_iCurrent + celt > m_cNames) {
		m_iCurrent = m_cNames;
		sc = S_FALSE;
	}
	else {
		m_iCurrent += celt;
		sc = S_OK;
	}

	return ResultFromScode(sc);
}


STDMETHODIMP CLUEnum::Reset(void)
{
	Cleanup();
	m_iCurrent = 0;
	return Init();
}


STDMETHODIMP CLUEnum::Clone(IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum)
{
	CLUEnum *pNewEnum = new CLUEnum(m_pDB);

	if (pNewEnum == NULL)
		return ResultFromScode(E_OUTOFMEMORY);

	HRESULT hres = pNewEnum->Init();
	if (FAILED(hres)) {
		pNewEnum->Release();
		return hres;
	}

	if (m_iCurrent == m_cNames) {			/* if at end, new one is at end */
		pNewEnum->m_iCurrent = pNewEnum->m_cNames;
	}
	else {
		LPCSTR pszCurrentName = m_papszNames[m_iCurrent];
		for (UINT i=0; i<pNewEnum->m_cNames; i++) {
			if (!::strcmpf(pszCurrentName, pNewEnum->m_papszNames[i])) {
				pNewEnum->m_iCurrent = i;	/* names match, start here */
				break;
			}
		}
		if (i == pNewEnum->m_cNames)
			pNewEnum->m_iCurrent = 0;		/* current name not found, start at beginning */
	}

	return NOERROR;
}

