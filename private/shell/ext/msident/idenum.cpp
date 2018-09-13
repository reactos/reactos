//
// ident.cpp - implementation of CIdentity class
//
#include "private.h"
#include "strconst.h"
#include "multiusr.h"
//
// Constructor / destructor
//
CEnumUserIdentity::CEnumUserIdentity()
{
    m_cRef = 1;
    m_dwCurrentUser = 0;
    m_cCountUsers = 0;
    m_rguidUsers = NULL;
    m_fInited = FALSE;

    DllAddRef();
}


CEnumUserIdentity::~CEnumUserIdentity()
{   
    _Cleanup();

    DllRelease();
}


//
// IUnknown members
//
STDMETHODIMP CEnumUserIdentity::QueryInterface(
    REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }
    
    *ppv=NULL;

    // Validate requested interface
    if(IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
    }
    else if(IID_IEnumUserIdentity == riid)
    {
        *ppv = (IEnumUserIdentity *)this;
    }

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumUserIdentity::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEnumUserIdentity::Release()
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}


// 
// IEnumUserIdentity members
//
STDMETHODIMP CEnumUserIdentity::Next(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
{
	ULONG celtFetched = 0;
	HRESULT hr = ResultFromScode(S_OK);
    CUserIdentity   *pIdentity;

    if (!m_fInited)
        hr = _Init();

    if (FAILED(hr))
        return hr;

    while (celt) 
    {
        if (m_dwCurrentUser == m_cCountUsers) {
            hr = ResultFromScode(S_FALSE);
            break;
        }

        pIdentity = new CUserIdentity;
        if (pIdentity)
        {
            if (SUCCEEDED(pIdentity->InitFromCookie(&m_rguidUsers[m_dwCurrentUser])))
            {
                rgelt[celtFetched++] = pIdentity;
                m_dwCurrentUser++;
            }
            else
            {
                pIdentity->Release();
                hr = ResultFromScode(E_OUTOFMEMORY);
                break;
            }
        }
        else
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            break;
        }
        celt--;
    }

    if (FAILED(hr))
    {
        for (ULONG i = 0; i < celtFetched; i++)
            rgelt[i]->Release();
        celtFetched = 0;
    }

	if (pceltFetched != NULL)
		*pceltFetched = celtFetched;

	return hr;
}

STDMETHODIMP CEnumUserIdentity::Skip(ULONG celt)
{
    SCODE sc;
    HRESULT hr = S_OK;

    if (!m_fInited)
        hr = _Init();

    if (FAILED(hr))
        return hr;

	if (m_dwCurrentUser + celt > m_cCountUsers) {
		m_dwCurrentUser = m_cCountUsers;
		sc = S_FALSE;
	}
	else {
		m_dwCurrentUser += celt;
		sc = S_OK;
	}

	return ResultFromScode(sc);
}

STDMETHODIMP CEnumUserIdentity::Reset(void)
{
    m_dwCurrentUser = 0;
    _Cleanup();
    return S_OK;
}

STDMETHODIMP CEnumUserIdentity::Clone(IEnumUserIdentity **ppenum)
{
    CEnumUserIdentity   *pEnum;
    HRESULT             hr = S_OK;

    if (!m_fInited)
        hr = _Init();

    if (FAILED(hr))
        return hr;

    pEnum = new CEnumUserIdentity;

    if (pEnum)
    {
        hr = pEnum->_Init(m_dwCurrentUser, m_cCountUsers, m_rguidUsers);

        if (SUCCEEDED(hr))
            *ppenum = pEnum;
    }

    return hr;
}

STDMETHODIMP CEnumUserIdentity::GetCount(ULONG *pnCount)
{
    HRESULT hr = S_OK;

    if (!m_fInited)
        hr = _Init();

    if (FAILED(hr))
        return hr;

    *pnCount = m_cCountUsers;

    return S_OK;
}

STDMETHODIMP CEnumUserIdentity::_Cleanup()
{
    SafeMemFree(m_rguidUsers);
    m_cCountUsers = 0;
    m_fInited = FALSE;

    return S_OK;
}

STDMETHODIMP CEnumUserIdentity::_Init()
{
    HRESULT hr=S_OK;
    HKEY    hReg = NULL;
    DWORD   cUsers = 0;
    DWORD   cbMaxSubKeyLen;
    DWORD   cb;
    DWORD   dwEnumIndex = 0;
    BOOL    fDisabled = MU_IdentitiesDisabled();

    m_cCountUsers = 0;

    // Open or Create root server key
    if (RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS, NULL, &hReg, NULL) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Enumerate keys
    if (RegQueryInfoKey(hReg, NULL, NULL, 0, &cUsers, &cbMaxSubKeyLen, NULL, NULL, NULL, NULL,
                        NULL, NULL) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto exit;
    }


    // No users ?
    if (cUsers == 0)
        goto done;

    if (fDisabled)
        cUsers = 1;

    // Allocate the users array
    MemAlloc((LPVOID *)&m_rguidUsers, sizeof(GUID) * cUsers);
    
    if (!m_rguidUsers)
    {
        cUsers = 0;
        goto done;
    }

    // Zero init
    ZeroMemory(m_rguidUsers, sizeof(GUID) * cUsers);

    if (fDisabled)
    {
        MU_GetDefaultUserID(&m_rguidUsers[0]);
        goto done;
    }

    while (TRUE) 
    {
        HKEY    hkUserKey;
        DWORD   dwStatus, dwSize, dwType;
        TCHAR   szKeyNameBuffer[MAX_PATH];
        TCHAR   szUid[255];

        if (RegEnumKey(hReg, dwEnumIndex++, szKeyNameBuffer,MAX_PATH)
            !=  ERROR_SUCCESS)
            break;
        
        if (RegOpenKey(hReg, szKeyNameBuffer, &hkUserKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szUid);
            dwStatus = RegQueryValueEx(hkUserKey, c_szUserID, NULL, &dwType, (LPBYTE)szUid, &dwSize);
            Assert(ERROR_SUCCESS == dwStatus);

            if (ERROR_SUCCESS == dwStatus)
                GUIDFromAString(szUid, &m_rguidUsers[dwEnumIndex - 1]);

            RegCloseKey(hkUserKey); 
        }
        else
            AssertSz(FALSE, "Couldn't open user's key");
    }

done:
    m_cCountUsers = cUsers;
    m_fInited = TRUE;

exit:
    if (hReg)
        RegCloseKey(hReg);

    return hr;
}


STDMETHODIMP CEnumUserIdentity::_Init(DWORD dwCurrentUser, DWORD dwCountUsers, GUID *prgUserCookies)
{
    m_dwCurrentUser = dwCurrentUser;
    m_cCountUsers = dwCountUsers;

    // Allocate the users array
    MemAlloc((LPVOID *)&m_rguidUsers, sizeof(GUID) * dwCountUsers);
    
    if (!m_rguidUsers)
        return E_OUTOFMEMORY;

    CopyMemory(m_rguidUsers, prgUserCookies, sizeof(GUID) * dwCountUsers);

    m_fInited = TRUE;
    
    return S_OK;
}

