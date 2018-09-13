#include "priv.h"
#include "privcpp.h"

// Constructor
CPackage_IPersistFile::CPackage_IPersistFile(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0); 
}

CPackage_IPersistFile::~CPackage_IPersistFile()
{
    DebugMsg(DM_TRACE,"CPackage_IPersistFile destroyed with ref count %d",_cRef);
}

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IPersistFile::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);  // delegate to CPackage
}

ULONG CPackage_IPersistFile::AddRef(void) 
{
    _cRef++;                                    // interface ref count for debug
    return _pPackage->AddRef();                 // delegate to CPackage
}

ULONG CPackage_IPersistFile::Release(void)
{
    _cRef--;                                    // interface ref count for debug
    return _pPackage->Release();                // delegate to CPackage
}

//////////////////////////////////
//
// IPersistFile Methods...
//
HRESULT CPackage_IPersistFile::GetClassID(LPCLSID pClassID)
{
    DebugMsg(DM_TRACE, "pack ps - GetClassID() called.");
    
    if (pClassID == NULL)
        return E_INVALIDARG;
    
    *pClassID = CLSID_CPackage;
    return S_OK;
}

HRESULT CPackage_IPersistFile::IsDirty(void)
{
    DebugMsg(DM_TRACE, "pack ps - IsDirty() called.");
    return E_NOTIMPL;
}

    
HRESULT CPackage_IPersistFile::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT     hr;

    DebugMsg(DM_TRACE, "pack pf - Load() called.");

    if (!pszFileName) {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        return E_POINTER;
    }

    //
    // BUGBUG: We blow off the mode flags
    //
    
#ifdef UNICODE
    
    hr = _pPackage->EmbedInitFromFile((LPTSTR) pszFileName, TRUE);
    
#else
    
    CHAR szPath[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, szPath, ARRAYSIZE(szPath), NULL, NULL);
    hr = _pPackage->EmbedInitFromFile(szPath, TRUE);
    
#endif // UNICODE

    DebugMsg(DM_TRACE, "            leaving Load()");
    
    return hr;
}

    
HRESULT CPackage_IPersistFile::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    DebugMsg(DM_TRACE, "pack pf - Save() called.");

    return E_NOTIMPL;    
}

    
HRESULT CPackage_IPersistFile::SaveCompleted(LPCOLESTR pszFileName)
{
    DebugMsg(DM_TRACE, "pack pf - SaveCompleted() called.");

    return E_NOTIMPL;
}

    
HRESULT CPackage_IPersistFile::GetCurFile(LPOLESTR *ppszFileName)
{
    DebugMsg(DM_TRACE, "pack pf - GetCurFile() called.");

    if (!ppszFileName)
        return E_POINTER;
    
    *ppszFileName = NULL;           // null the out param
    return E_NOTIMPL;
}
