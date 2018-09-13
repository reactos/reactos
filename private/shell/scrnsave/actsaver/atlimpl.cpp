// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#include "precomp.h"

const IID IID_IRegister = {0xCC118C81,0xB379,0x11CF,{0x84,0xE3,0x00,0xAA,0x00,0x21,0xF3,0x37}};
const CLSID CLSID_Register = {0xCC118C85,0xB379,0x11CF,{0x84,0xE3,0x00,0xAA,0x00,0x21,0xF3,0x37}};

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

CComBSTR& CComBSTR::operator=(const CComBSTR& src)
{
    if (m_str != src.m_str)
    {
        if (m_str)
            ::SysFreeString(m_str);
        m_str = src.Copy();
    }
    return *this;
}

CComBSTR& CComBSTR::operator=(LPCOLESTR pSrc)
{
    if (m_str)
        ::SysFreeString(m_str);

    m_str = ::SysAllocString(pSrc);
    return *this;
}

#ifndef OLE2ANSI
CComBSTR::CComBSTR(LPCSTR pSrc)
{
    A2COLE(pSrc, pwSrc);
    m_str = ::SysAllocString(pwSrc);
}

CComBSTR::CComBSTR(int nSize, LPCSTR sz)
{
    A2COLE(sz, pwSrc);
    m_str = ::SysAllocStringLen(pwSrc, nSize);
}

CComBSTR& CComBSTR::operator=(LPCSTR pSrc)
{
    if (m_str)
        ::SysFreeString(m_str);

    A2COLE(pSrc, pwStr);
    m_str = ::SysAllocString(pwStr);
    return *this;
}
#endif
/////////////////////////////////////////////////////////////////////////////
// CComVariant

#ifndef OLE2ANSI
CComVariant::CComVariant(LPCSTR lpsz)
{
    VariantInit(this);
    vt = VT_BSTR;
    A2COLE(lpsz, postr);
    bstrVal = ::SysAllocString(postr);
}

CComVariant& CComVariant::operator=(LPCSTR lpsz)
{
    VariantClear(this);
    vt = VT_BSTR;
    A2COLE(lpsz, postr);
    bstrVal = ::SysAllocString(postr);
    return *this;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

IUnknown* WINAPI _AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
    if (lp != NULL)
        lp->AddRef();
    if (*pp)
        (*pp)->Release();
    *pp = lp;
    return lp;
}

IUnknown* WINAPI _AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
    IUnknown* pTemp = *pp;
    lp->QueryInterface(riid, (void**)pp);
    if (pTemp)
        pTemp->Release();
    return *pp;
}

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

void WINAPI AtlFreeMarshalStream(IStream* pStream)
{
    if (pStream != NULL)
    {
        CoReleaseMarshalData(pStream);
        pStream->Release();
    }
}

HRESULT WINAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream)
{
    HRESULT hRes = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
    if (SUCCEEDED(hRes))
    {
        hRes = CoMarshalInterface(*ppStream, iid,
            pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
        if (FAILED(hRes))
        {
            (*ppStream)->Release();
            *ppStream = NULL;
        }
    }
    return hRes;
}

HRESULT WINAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk)
{
    *ppUnk = NULL;
    HRESULT hRes = E_INVALIDARG;
    if (pStream != NULL)
    {
        LARGE_INTEGER l;
        l.QuadPart = 0;
        pStream->Seek(l, STREAM_SEEK_SET, NULL);
        hRes = CoUnmarshalInterface(pStream, iid, (void**)ppUnk);
    }
    return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

HRESULT AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
    CComPtr<IConnectionPointContainer> pCPC;
    CComPtr<IConnectionPoint> pCP;
    HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
    if (SUCCEEDED(hRes))
        hRes = pCPC->FindConnectionPoint(iid, &pCP);
    if (SUCCEEDED(hRes))
        hRes = pCP->Advise(pUnk, pdw);
    return hRes;
}

HRESULT AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
    CComPtr<IConnectionPointContainer> pCPC;
    CComPtr<IConnectionPoint> pCP;
    HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
    if (SUCCEEDED(hRes))
        hRes = pCPC->FindConnectionPoint(iid, &pCP);
    if (SUCCEEDED(hRes))
        hRes = pCP->Unadvise(dw);
    return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// CComTypeInfoHolder

void CComTypeInfoHolder::AddRef()
{
    _pModule->m_csTypeInfoHolder.Lock();
    m_dwRef++;
    _pModule->m_csTypeInfoHolder.Unlock();
}

void CComTypeInfoHolder::Release()
{
    _pModule->m_csTypeInfoHolder.Lock();
    if (--m_dwRef == 0)
    {
        if (m_pInfo != NULL)
            m_pInfo->Release();
        m_pInfo = NULL;
    }
    _pModule->m_csTypeInfoHolder.Unlock();
}

HRESULT CComTypeInfoHolder::GetTI(LCID lcid, ITypeInfo** ppInfo)
{
    //If this assert occurs then most likely didn't initialize properly
    ASSERT(m_plibid != NULL && m_pguid != NULL);
    ASSERT(ppInfo != NULL);
    *ppInfo = NULL;

    HRESULT hRes = E_FAIL;
    _pModule->m_csTypeInfoHolder.Lock();
    if (m_pInfo == NULL)
    {
        ITypeLib* pTypeLib;
        hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
        if (SUCCEEDED(hRes))
        {
            ITypeInfo* pTypeInfo;
            hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &pTypeInfo);
            if (SUCCEEDED(hRes))
                m_pInfo = pTypeInfo;
            pTypeLib->Release();
        }
    }
    *ppInfo = m_pInfo;
    if (m_pInfo != NULL)
    {
        m_pInfo->AddRef();
        hRes = S_OK;
    }
    _pModule->m_csTypeInfoHolder.Unlock();
    return hRes;
}

HRESULT CComTypeInfoHolder::GetTypeInfo(UINT /*itinfo*/, LCID lcid,
    ITypeInfo** pptinfo)
{
    HRESULT hRes = E_POINTER;
    if (pptinfo != NULL)
        hRes = GetTI(lcid, pptinfo);
    return hRes;
}

HRESULT CComTypeInfoHolder::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgdispid)
{
    ITypeInfo* pInfo;
    HRESULT hRes = GetTI(lcid, &pInfo);
    if (pInfo != NULL)
    {
        hRes = pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
        pInfo->Release();
    }
    return hRes;
}

HRESULT CComTypeInfoHolder::Invoke(IDispatch* p, DISPID dispidMember, REFIID /*riid*/,
    LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
    EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    SetErrorInfo(0, NULL);
    ITypeInfo* pInfo;
    HRESULT hRes = GetTI(lcid, &pInfo);
    if (pInfo != NULL)
    {
        hRes = pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        pInfo->Release();
    }
    return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, const IID& iid,
    HRESULT hRes)
{
    TCHAR szDesc[1024];
    szDesc[0] = NULL;
    // For a valid HRESULT the id should be in the range [0x0200, 0xffff]
    ASSERT((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
    if (LoadString(_pModule->GetResourceInstance(), nID, szDesc, 1024) == 0)
    {
        ASSERT(FALSE);
        lstrcpy(szDesc, _T("Unknown Error"));
    }
    AtlReportError(clsid, szDesc, iid, hRes);
    if (hRes == 0)
        hRes = MAKE_HRESULT(3, FACILITY_ITF, nID);
    return hRes;
}

#ifndef OLE2ANSI
HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
    const IID& iid, HRESULT hRes)
{
    A2CW(lpszDesc, lpwDesc);
    return AtlReportError(clsid, lpwDesc, iid, hRes);
}
#endif

HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
    const IID& iid, HRESULT hRes)
{
    CComPtr<ICreateErrorInfo> pICEI;
    if (SUCCEEDED(CreateErrorInfo(&pICEI)))
    {
        CComPtr<IErrorInfo> pErrorInfo;
        pICEI->SetGUID(iid);
        LPOLESTR lpsz;
        ProgIDFromCLSID(clsid, &lpsz);
        if (lpsz != NULL)
            pICEI->SetSource(lpsz);
        CoTaskMemFree(lpsz);
        pICEI->SetDescription((LPOLESTR)lpszDesc);
        if (SUCCEEDED(pICEI->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo)))
            SetErrorInfo(0, pErrorInfo);
    }
    return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// QI implementation

#ifdef _ATL_DEBUG_QI
#define _DUMPIID(iid, name, hr) DumpIID(iid, name, hr)
#else
#define _DUMPIID(iid, name, hr) hr
#endif

HRESULT WINAPI CComObjectRoot::InternalQueryInterface(void* pThis,
    const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
    ASSERT(pThis != NULL);
    // First entry should be an offset (pFunc == 1)
    ASSERT(pEntries->pFunc == (_ATL_CREATORARGFUNC*)1);
#ifdef _ATL_DEBUG_QI
    LPCTSTR pszClassName = (LPCTSTR) pEntries[-1].dw;
#endif // _ATL_DEBUG_QI
    if (ppvObject == NULL)
        return _DUMPIID(iid, pszClassName, E_POINTER);
    *ppvObject = NULL;
    if (InlineIsEqualUnknown(iid)) // use first interface
    {
            IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
            pUnk->AddRef();
            *ppvObject = pUnk;
            return _DUMPIID(iid, pszClassName, S_OK);
    }
    while (pEntries->pFunc != NULL)
    {
        BOOL bBlind = (pEntries->piid == NULL);
        if (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))
        {
            if (pEntries->pFunc == (_ATL_CREATORARGFUNC*)1) //offset
            {
                ASSERT(!bBlind);
                IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
                pUnk->AddRef();
                *ppvObject = pUnk;
                return _DUMPIID(iid, pszClassName, S_OK);
            }
            else //actual function call
            {
                HRESULT hRes = pEntries->pFunc(pThis,
                    iid, ppvObject, pEntries->dw);
                if (hRes == S_OK || (!bBlind && FAILED(hRes)))
                    return _DUMPIID(iid, pszClassName, hRes);
            }
        }
        pEntries++;
    }
    return _DUMPIID(iid, pszClassName, E_NOINTERFACE);
}

#ifdef _ATL_DEBUG_QI

HRESULT CComObjectRoot::DumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr)
{
    CRegKey key;
    TCHAR szName[100];
    DWORD dwType,dw = sizeof(szName);

    LPOLESTR pszGUID = NULL;
    StringFromCLSID(iid, &pszGUID);
    OutputDebugString(pszClassName);
    OutputDebugString(_T(" - "));

    // Attempt to find it in the interfaces section
    key.Open(HKEY_CLASSES_ROOT, _T("Interface"));
    if (key.Open(key, OLE2T(pszGUID)) == S_OK)
    {
        *szName = 0;
        RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw);
        OutputDebugString(szName);
        goto cleanup;
    }
    // Attempt to find it in the clsid section
    key.Open(HKEY_CLASSES_ROOT, _T("CLSID"));
    if (key.Open(key, OLE2T(pszGUID)) == S_OK)
    {
        *szName = 0;
        RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw);
        OutputDebugString(_T("(CLSID\?\?\?) "));
        OutputDebugString(szName);
        goto cleanup;
    }
    OutputDebugString(OLE2T(pszGUID));
cleanup:
    if (hr != S_OK)
        OutputDebugString(_T(" - failed"));
    OutputDebugString(_T("\n"));
    CoTaskMemFree(pszGUID);
    return hr;
}
#endif

HRESULT WINAPI CComObjectRoot::_Cache(void* pv, REFIID iid, void** ppvObject, DWORD dw)
{
    HRESULT hRes = E_NOINTERFACE;
    _ATL_CACHEDATA* pcd = (_ATL_CACHEDATA*)dw;
    IUnknown** pp = (IUnknown**)((DWORD_PTR)pv + pcd->dwOffsetVar);
    if (*pp == NULL)
    {
        _ThreadModel::CriticalSection* pcs =
            (_ThreadModel::CriticalSection*)((INT_PTR)pv + pcd->dwOffsetCS);
        pcs->Lock();
        if (*pp == NULL)
            hRes = pcd->pFunc(pv, IID_IUnknown, (void**)pp);
        pcs->Unlock();
    }
    if (*pp != NULL)
        hRes = (*pp)->QueryInterface(iid, ppvObject);
    return hRes;
}

HRESULT WINAPI CComObjectRoot::_Creator(void* pv, REFIID iid, void** ppvObject, DWORD dw)
{
    _ATL_CREATORDATA* pcd = (_ATL_CREATORDATA*)dw;
    return pcd->pFunc(pv, iid, ppvObject);
}

HRESULT WINAPI CComObjectRoot::_Delegate(void* pv, REFIID iid, void** ppvObject, DWORD dw)
{
    HRESULT hRes = E_NOINTERFACE;
    IUnknown* p = *(IUnknown**)((DWORD_PTR)pv + dw);
    if (p != NULL)
        hRes = p->QueryInterface(iid, ppvObject);
    return hRes;
}

HRESULT WINAPI CComObjectRoot::_Chain(void* pv, REFIID iid, void** ppvObject, DWORD dw)
{
    _ATL_CHAINDATA* pcd = (_ATL_CHAINDATA*)dw;
    void* p = (void*)((DWORD_PTR)pv + pcd->dwOffset);
    return InternalQueryInterface(p, pcd->pFunc(), iid, ppvObject);
}

/////////////////////////////////////////////////////////////////////////////
// CComClassFactory

STDMETHODIMP CComClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
    REFIID riid, void** ppvObj)
{
    ASSERT(m_pfnCreateInstance != NULL);
    HRESULT hRes = E_POINTER;
    if (ppvObj != NULL)
    {
        *ppvObj = NULL;
        // can't ask for anything other than IUnknown when aggregating
        ASSERT((pUnkOuter == NULL) || InlineIsEqualUnknown(riid));
        if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
            hRes = CLASS_E_NOAGGREGATION;
        else
            hRes = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
    }
    return hRes;
}

STDMETHODIMP CComClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        _pModule->Lock();
    else
        _pModule->Unlock();
    return S_OK;
}

STDMETHODIMP CComClassFactory2Base::LockServer(BOOL fLock)
{
    if (fLock)
        _pModule->Lock();
    else
        _pModule->Unlock();
    return S_OK;
}

#ifndef _ATL_NO_CONNECTION_POINTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

CComConnectionPointBase* CComConnectionPointContainerImpl::
    FindConnPoint(REFIID riid)
{
    const _ATL_CONNMAP_ENTRY* pEntry = GetConnMap();
    while (pEntry->dwOffset != (DWORD)-1)
    {
        CComConnectionPointBase* pCP =
            (CComConnectionPointBase*)((INT_PTR)this+pEntry->dwOffset);
        if (InlineIsEqualGUID(riid, *pCP->GetIID()))
            return pCP;

#ifdef _FULLDEBUG
        char szIIDName[MAX_PATH];
        DebugIIDName(*pCP->GetIID(), szIIDName);

        TRACE("FindConnPoint(%s)\r\n", szIIDName);
#endif  // _FULLDEBUG

        pEntry++;
    }
    return NULL;
}


void CComConnectionPointContainerImpl::InitCloneVector(
    CComConnectionPointBase** ppCP)
{
    const _ATL_CONNMAP_ENTRY* pEntry = GetConnMap();
    while (pEntry->dwOffset != (DWORD)-1)
    {
        *ppCP = (CComConnectionPointBase*)((INT_PTR)this+pEntry->dwOffset);
        ppCP++;
        pEntry++;
    }
}


STDMETHODIMP CComConnectionPointContainerImpl::EnumConnectionPoints(
    IEnumConnectionPoints** ppEnum)
{
    
    HRESULT hRes = S_OK;
    CComConnectionPointBase** ppCP = NULL;
    _ATL_CONNMAP_ENTRY* pEntry = NULL;
    int nCPCount=0;

    if (ppEnum == NULL)
        return E_POINTER;

    *ppEnum = NULL;

    CComEnumConnectionPoints* pEnum = NULL;
    ATLTRY(pEnum = new CComObject<CComEnumConnectionPoints>)

    if (pEnum == NULL)
    {
        hRes = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // count the entries in the map
    pEntry = (_ATL_CONNMAP_ENTRY*)GetConnMap();
    ASSERT(pEntry != NULL);
    while (pEntry->dwOffset != (DWORD)-1)
    {
        nCPCount++;
        pEntry++;
    }
    ASSERT(nCPCount > 0);

    // allocate an initialize a vector of connection point object pointers
    ppCP = (CComConnectionPointBase**)malloc(sizeof(CComConnectionPointBase*)*nCPCount);

    if (NULL == ppCP)
    {
        hRes = E_OUTOFMEMORY;
        goto Cleanup;
    }

    InitCloneVector(ppCP);

    // copy the pointers: they will AddRef this object
    hRes = pEnum->Init((IConnectionPoint**)&ppCP[0],
        (IConnectionPoint**)&ppCP[nCPCount], this, AtlFlagCopy);

    if (FAILED(hRes))
        goto Cleanup;

    hRes = pEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum);

Cleanup:

    if (pEnum)
        delete pEnum;

    if (ppCP != NULL)
        free(ppCP);

    return hRes;
}


STDMETHODIMP CComConnectionPointContainerImpl::FindConnectionPoint(
    REFIID riid, IConnectionPoint** ppCP)
{
    if (ppCP == NULL)
        return E_POINTER;
    *ppCP = NULL;
    HRESULT hRes = CONNECT_E_NOCONNECTION;

#ifdef _FULLDEBUG
        char szIIDName[MAX_PATH];
        DebugIIDName(riid, szIIDName);

        TRACE("FindConnectionPoint(%s)\r\n", szIIDName);
#endif  // _FULLDEBUG

    CComConnectionPointBase* pCP = FindConnPoint(riid);
    if (pCP != NULL)
    {
        pCP->AddRef();
        *ppCP = pCP;
        hRes = S_OK;
    }
    return hRes;
}


BOOL CComDynamicArrayCONNECTDATA::Add(IUnknown* pUnk)
{
    if (m_nSize == 0) // no connections
    {
        m_cd.pUnk = pUnk;
        m_cd.dwCookie = 1;
        m_nSize = 1;
        return TRUE;
    }
    else if (m_nSize == 1)
    {
        //create array
        m_pCD = (CONNECTDATA*)malloc(sizeof(CONNECTDATA)*_DEFAULT_VECTORLENGTH);

        if (NULL == m_pCD)
            return FALSE;

        ZeroMemory(m_pCD, sizeof(CONNECTDATA)*_DEFAULT_VECTORLENGTH);
        m_pCD[0] = m_cd;
        m_nSize = _DEFAULT_VECTORLENGTH;
    }
    for (CONNECTDATA* p = begin();p<end();p++)
    {
        if (p->pUnk == NULL)
        {
            p->pUnk = pUnk;
            p->dwCookie = 1 + (DWORD)(p - begin());
            return TRUE;
        }
    }

    int nAlloc = m_nSize*2;
    m_pCD = (CONNECTDATA*)realloc(m_pCD, sizeof(CONNECTDATA)*nAlloc);

    if (NULL == m_pCD)
        return FALSE;

    ZeroMemory(&m_pCD[m_nSize], sizeof(CONNECTDATA)*m_nSize);
    m_pCD[m_nSize].pUnk = pUnk;
    m_pCD[m_nSize].dwCookie = m_nSize + 1;
    m_nSize = nAlloc;
    return TRUE;
}

BOOL CComDynamicArrayCONNECTDATA::Remove(DWORD dwCookie)
{
    CONNECTDATA* p;
    if (dwCookie == NULL)
        return FALSE;
    if (m_nSize == 0)
        return FALSE;
    if (m_nSize == 1)
    {
        if (m_cd.dwCookie == dwCookie)
        {
            m_nSize = 0;
            return TRUE;
        }
        return FALSE;
    }
    for (p=begin();p<end();p++)
    {
        if (p->dwCookie == dwCookie)
        {
            p->pUnk = NULL;
            p->dwCookie = 0;
            return TRUE;
        }
    }
    return FALSE;
}

STDMETHODIMP CComConnectionPointBase::GetConnectionInterface(IID* piid)
{
    if (piid == NULL)
        return E_POINTER;
    *piid = *(IID*)GetIID();
    return S_OK;
}

STDMETHODIMP CComConnectionPointBase::GetConnectionPointContainer(IConnectionPointContainer** ppCPC)
{
    if (ppCPC == NULL)
        return E_POINTER;
    ASSERT(m_pContainer != NULL);
    *ppCPC = m_pContainer;
    m_pContainer->AddRef();
    return S_OK;
}

#endif //!_ATL_NO_CONNECTION_POINTS

/////////////////////////////////////////////////////////////////////////////
// statics

static UINT WINAPI AtlGetDirLen(LPCOLESTR lpszPathName)
{
    ASSERT(lpszPathName != NULL);

    // always capture the complete file name including extension (if present)
    LPCOLESTR lpszTemp = lpszPathName;
    for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL; )
    {
        LPCOLESTR lp = CharNextO(lpsz);
        // remember last directory/drive separator
        if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
            lpszTemp = lp;
        lpsz = lp;
    }

    return (UINT)(lpszTemp-lpszPathName);
}

/////////////////////////////////////////////////////////////////////////////
// Object Registry Support

static HRESULT WINAPI AtlRegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc)
{
    CRegKey keyProgID;
    LONG lRes = keyProgID.Create(HKEY_CLASSES_ROOT, lpszProgID);
    if (lRes == ERROR_SUCCESS)
    {
        keyProgID.SetValue(lpszUserDesc);
        keyProgID.SetKeyValue(_T("CLSID"), lpszCLSID);
        return S_OK;
    }
    return HRESULT_FROM_WIN32(lRes);
}

HRESULT WINAPI CComModule::UpdateRegistryFromResource(UINT nResID, BOOL bRegister,
    struct _ATL_REGMAP_ENTRY* pMapEntries)
{
    CComPtr<IRegister> p;
    HRESULT hRes = CoCreateInstance(CLSID_Register, NULL,
        CLSCTX_INPROC_SERVER, IID_IRegister, (void**)&p);
    if (SUCCEEDED(hRes))
    {
        TCHAR szModule[MAX_PATH];
        GetModuleFileName(_pModule->GetModuleInstance(), szModule, MAX_PATH);
        p->AddReplacement(CComBSTR(OLESTR("Module")), CComBSTR(szModule));
        if (NULL != pMapEntries)
        {
            while (NULL != pMapEntries->szKey)
            {
                ASSERT(NULL != pMapEntries->szData);

                CComBSTR bstrKey(pMapEntries->szKey);
                CComBSTR bstrValue(pMapEntries->szData);
                p->AddReplacement(bstrKey, bstrValue);
                pMapEntries++;
            }
        }

        CComVariant varRes;
        varRes.vt = VT_I2;
        varRes.iVal = (short)nResID;
        CComVariant varReg(OLESTR("REGISTRY"));
        GetModuleFileName(_pModule->GetRegistryResourceInstance(), szModule, MAX_PATH);
        CComBSTR bstrModule = szModule;
        if (bRegister)
        {
            hRes = p->ResourceRegister(bstrModule, varRes, varReg);
        }
        else
        {
            hRes = p->ResourceUnregister(bstrModule, varRes, varReg);
        }
    }
    return hRes;
}

HRESULT WINAPI CComModule::UpdateRegistryFromResource(LPCTSTR lpszRes, BOOL bRegister,
    struct _ATL_REGMAP_ENTRY* pMapEntries)
{
    CComPtr<IRegister> p;
    HRESULT hRes = CoCreateInstance(CLSID_Register, NULL,
        CLSCTX_INPROC_SERVER, IID_IRegister, (void**)&p);
    if (SUCCEEDED(hRes))
    {
        TCHAR szModule[MAX_PATH];
        GetModuleFileName(_pModule->GetModuleInstance(), szModule, MAX_PATH);
        p->AddReplacement(CComBSTR(OLESTR("Module")), CComBSTR(szModule));
        if (NULL != pMapEntries)
        {
            while (NULL != pMapEntries->szKey)
            {
                ASSERT(NULL != pMapEntries->szData);

                CComBSTR bstrKey(pMapEntries->szKey);
                CComBSTR bstrValue(pMapEntries->szData);

                p->AddReplacement(bstrKey, bstrValue);
                pMapEntries++;
            }
        }
        CComVariant varRes(lpszRes);
        CComVariant varReg(OLESTR("REGISTRY"));
        GetModuleFileName(_pModule->GetRegistryResourceInstance(), szModule, MAX_PATH);
        CComBSTR bstrModule = szModule;
        if (bRegister)
        {
            hRes = p->ResourceRegister(bstrModule, varRes, varReg);
        }
        else
        {
            hRes = p->ResourceUnregister(bstrModule, varRes, varReg);
        }
    }
    return hRes;
}

#ifdef _ATL_STATIC_REGISTRY
// Statically linking to Registry Ponent
HRESULT WINAPI CComModule::UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
    struct _ATL_REGMAP_ENTRY* pMapEntries)
{
    CRegObject      ro;
    CRegException   re;
    TCHAR szModule[MAX_PATH];
    GetModuleFileName(_pModule->GetModuleInstance(), szModule, MAX_PATH);
    ro.AddReplacement(OLESTR("Module"), CComBSTR(szModule));
    if (NULL != pMapEntries)
    {
        while (NULL != pMapEntries->szKey)
        {
            ASSERT(NULL != pMapEntries->szData);
            ro.AddReplacement(CComBSTR(pMapEntries->szKey),
                CComBSTR(pMapEntries->szData));
            pMapEntries++;
        }
    }

    CComVariant varRes;
    varRes.vt = VT_I2;
    varRes.iVal = (short)nResID;
    CComVariant varReg(OLESTR("REGISTRY"));
    GetModuleFileName(_pModule->GetRegistryResourceInstance(), szModule, MAX_PATH);
    CComBSTR bstrModule = szModule;
    return (bRegister) ? ro.ResourceRegister(bstrModule, varRes, varReg, re) :
        ro.ResourceUnregister(bstrModule, varRes, varReg, re);
}

HRESULT WINAPI CComModule::UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
    struct _ATL_REGMAP_ENTRY* pMapEntries)
{
    CRegObject      ro;
    CRegException   re;
    TCHAR szModule[MAX_PATH];
    GetModuleFileName(_pModule->GetModuleInstance(), szModule, MAX_PATH);
    ro.AddReplacement(OLESTR("Module"), CComBSTR(szModule));
    if (NULL != pMapEntries)
    {
        while (NULL != pMapEntries->szKey)
        {
            ASSERT(NULL != pMapEntries->szData);
            ro.AddReplacement(CComBSTR(pMapEntries->szKey),
                CComBSTR(pMapEntries->szData));
            pMapEntries++;
        }
    }

    CComVariant varRes(lpszRes);
    CComVariant varReg(OLESTR("REGISTRY"));
    GetModuleFileName(_pModule->GetRegistryResourceInstance(), szModule, MAX_PATH);
    CComBSTR bstrModule = szModule;
    return (bRegister) ? ro.ResourceRegister(bstrModule, varRes, varReg, re) :
        ro.ResourceUnregister(bstrModule, varRes, varReg, re);
}
#endif // _ATL_STATIC_REGISTRY

HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
    LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister)
{
    if (bRegister)
    {
        return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, nDescID,
            dwFlags);
    }
    else
        return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

HRESULT WINAPI CComModule::RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
    LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags)
{
    static const TCHAR szProgID[] = _T("ProgID");
    static const TCHAR szVIProgID[] = _T("VersionIndependentProgID");
    static const TCHAR szLS32[] = _T("LocalServer32");
    static const TCHAR szIPS32[] = _T("InprocServer32");
    static const TCHAR szThreadingModel[] = _T("ThreadingModel");
    static const TCHAR szAUTPRX32[] = _T("AUTPRX32.DLL");
    static const TCHAR szApartment[] = _T("Apartment");
    static const TCHAR szBoth[] = _T("both");

    HRESULT hRes = S_OK;
    TCHAR szDesc[256];
    LoadString(m_hInst, nDescID, szDesc, 256);
    TCHAR szModule[MAX_PATH];
    GetModuleFileName(m_hInst, szModule, MAX_PATH);

    LPOLESTR lpOleStr;
    StringFromCLSID(clsid, &lpOleStr);
    OLE2T(lpOleStr, lpsz);

    hRes = AtlRegisterProgID(lpsz, lpszProgID, szDesc);
    if (hRes == S_OK)
        hRes = AtlRegisterProgID(lpsz, lpszVerIndProgID, szDesc);
    LONG lRes = ERROR_SUCCESS;
    if (hRes == S_OK)
    {
        CRegKey key;
        LONG lRes = key.Open(HKEY_CLASSES_ROOT, _T("CLSID"));
        if (lRes == ERROR_SUCCESS)
        {
            lRes = key.Create(key, lpsz);
            if (lRes == ERROR_SUCCESS)
            {
                key.SetValue(szDesc);
                key.SetKeyValue(szProgID, lpszProgID);
                key.SetKeyValue(szVIProgID, lpszVerIndProgID);

                if ((m_hInst == NULL) || (m_hInst == GetModuleHandle(NULL))) // register as EXE
                    key.SetKeyValue(szLS32, szModule);
                else
                {
                    key.SetKeyValue(szIPS32, (dwFlags & AUTPRXFLAG) ? szAUTPRX32 : szModule);
                    LPCTSTR lpszModel = (dwFlags & THREADFLAGS_BOTH) ? szBoth :
                        (dwFlags & THREADFLAGS_APARTMENT) ? szApartment : NULL;
                    if (lpszModel != NULL)
                        key.SetKeyValue(szIPS32, lpszModel, szThreadingModel);
                }
            }
        }
    }
    CoTaskMemFree(lpOleStr);
    if (lRes != ERROR_SUCCESS)
        hRes = HRESULT_FROM_WIN32(lRes);
    return hRes;
}

HRESULT WINAPI CComModule::UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
    LPCTSTR lpszVerIndProgID)
{
    CRegKey key;
    key.Attach(HKEY_CLASSES_ROOT);
    key.RecurseDeleteKey(lpszProgID);
    key.RecurseDeleteKey(lpszVerIndProgID);
    LPOLESTR lpOleStr;
    StringFromCLSID(clsid, &lpOleStr);
    OLE2T(lpOleStr, lpsz);
    if (key.Open(key, _T("CLSID")) == ERROR_SUCCESS)
        key.RecurseDeleteKey(lpsz);
    CoTaskMemFree(lpOleStr);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib Support

HRESULT CComModule::RegisterTypeLib(LPCTSTR lpszIndex)
{
    ASSERT(m_hInst != NULL);
    TCHAR szModule[MAX_PATH+10];
    OLECHAR szDir[MAX_PATH];
    GetModuleFileName(GetTypeLibInstance(), szModule, MAX_PATH);
    if (lpszIndex != NULL)
        lstrcat(szModule, lpszIndex);
    ITypeLib* pTypeLib;
    T2OLE(szModule, lpszModule);
    HRESULT hr = LoadTypeLib(lpszModule, &pTypeLib);
    if (!SUCCEEDED(hr))
    {
        // typelib not in module, try <module>.tlb instead
        LPTSTR lpszExt = NULL;
        LPTSTR lpsz;
        for (lpsz = szModule; *lpsz != NULL; lpsz = CharNext(lpsz))
        {
            if (*lpsz == _T('.'))
                lpszExt = lpsz;
        }
        if (lpszExt == NULL)
            lpszExt = lpsz;
        lstrcpy(lpszExt, _T(".tlb"));
        T2OLE(szModule, lpszModule);
        hr = LoadTypeLib(lpszModule, &pTypeLib);
    }
    if (SUCCEEDED(hr))
    {
        ocscpy(szDir, lpszModule);
        szDir[AtlGetDirLen(szDir)] = 0;
        hr = ::RegisterTypeLib(pTypeLib, lpszModule, szDir);
    }
    if (pTypeLib != NULL)
        pTypeLib->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CRegKey

LONG CRegKey::Close()
{
    LONG lRes = ERROR_SUCCESS;
    if (m_hKey != NULL)
    {
        lRes = RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    return lRes;
}

LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
    LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
    ASSERT(hKeyParent != NULL);
    DWORD dw;
    HKEY hKey = NULL;
    LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
        lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
    if (lpdwDisposition != NULL)
        *lpdwDisposition = dw;
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        m_hKey = hKey;
    }
    return lRes;
}

LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
    ASSERT(hKeyParent != NULL);
    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        ASSERT(lRes == ERROR_SUCCESS);
        m_hKey = hKey;
    }
    return lRes;
}

LONG CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
    DWORD dwType = NULL;
    DWORD dwCount = sizeof(DWORD);
    LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
        (LPBYTE)&dwValue, &dwCount);
    ASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
    ASSERT((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
    return lRes;
}

LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    ASSERT(lpszValue != NULL);
    CRegKey key;
    LONG lRes = key.Create(hKeyParent, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}

LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    ASSERT(lpszValue != NULL);
    CRegKey key;
    LONG lRes = key.Create(m_hKey, lpszKeyName);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}

//RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the
//specified key has subkeys
LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
    CRegKey key;
    LONG lRes = key.Open(m_hKey, lpszKey);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    FILETIME time;
    TCHAR szBuffer[256];
    DWORD dwSize = 256;
    while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
        &time)==ERROR_SUCCESS)
    {
        lRes = key.RecurseDeleteKey(szBuffer);
        if (lRes != ERROR_SUCCESS)
            return lRes;
        dwSize = 256;
    }
    key.Close();
    return DeleteSubKey(lpszKey);
}

/////////////////////////////////////////////////////////////////////////////
// Minimize CRT
// Specify DllMain as EntryPoint
// Turn off exception handling
// Define _ATL_MIN_CRT

#ifdef _ATL_MIN_CRT
/////////////////////////////////////////////////////////////////////////////
// Heap Allocation

int __cdecl _purecall()
{
#ifdef _DEBUG
    DebugBreak();
#endif  // _DEBUG

    return 0;
}

extern "C" const int _fltused = 0;

void * __cdecl malloc(size_t n)
{
    if (g_hHeap == NULL)
    {
        g_hHeap = HeapCreate(0, 0, 0);
#ifdef _DEBUG
        g_cHeapAllocsOutstanding = 0;
#endif  // _DEBUG
        if (g_hHeap == NULL)
            return NULL;
    }

    ASSERT(g_hHeap != NULL);

#ifdef _MALLOC_ZEROINIT
    void * p = HeapAlloc(g_hHeap, 0, n);

#ifdef _DEBUG
        g_cHeapAllocsOutstanding++;
#endif  // _DEBUG

    if (p != NULL)
        ZeroMemory(p, n);
    return p;
#else
#ifdef _DEBUG
        g_cHeapAllocsOutstanding++;
#endif  // _DEBUG

    return HeapAlloc(g_hHeap, 0, n);
#endif  // _MALLOC_ZEROINIT
}

void * __cdecl calloc(size_t n, size_t s)
{
    return malloc(n * s);
}

void * __cdecl realloc(void* p, size_t n)
{
    ASSERT(g_hHeap != NULL);
    return (p == NULL) ? malloc(n) : HeapReAlloc(g_hHeap, 0, p, n);
}

void __cdecl free(void* p)
{
    ASSERT(g_hHeap != NULL);

    if (p != NULL)
    {
        HeapFree(g_hHeap, 0, p);

#ifdef _DEBUG
        g_cHeapAllocsOutstanding--;
#endif  // _DEBUG
    }
}

inline void * __cdecl operator new(size_t n)
{
    return malloc(n);
}

inline void __cdecl operator delete(void* p)
{
    free(p);
}

#endif //_ATL_MIN_CRT
