//
//
//  assocapi.cpp
//
//     Association APIs
//
//
//


#include "priv.h"
#include "apithk.h"
#include <shstr.h>
#include <w95wraps.h>
#include <msi.h>

#define ISEXTENSION(psz)   (TEXT('.') == *(psz))
#define ISGUID(psz)        (TEXT('{') == *(psz))

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut);

inline BOOL IsEmptyStr(SHSTR &str)
{
    return (!*(LPTSTR)str);
}

void _MakeAppPathKey(LPCTSTR pszApp, LPTSTR pszKey, DWORD cchKey)
{
    if (_PathAppend(REGSTR_PATH_APPPATHS, pszApp, pszKey, cchKey))
    {
        // Currently we will only look up .EXE if an extension is not
        // specified
        if (*PathFindExtension(pszApp) == 0)
        {
            StrCatBuff(pszKey, TEXT(".exe"), cchKey);
        }
    }
}

void _MakeApplicationsKey(LPCTSTR pszApp, LPTSTR pszKey, DWORD cchKey)
{
    if (_PathAppend(TEXT("Applications"), pszApp, pszKey, cchKey))
    {
        // Currently we will only look up .EXE if an extension is not
        // specified
        if (*PathFindExtension(pszApp) == 0)
        {
            StrCatBuff(pszKey, TEXT(".exe"), cchKey);
        }
    }
}

HRESULT _AssocOpenRegKey(HKEY hk, LPCTSTR pszSub, HKEY *phkOut, BOOL fCreate = FALSE)
{
    ASSERT(phkOut);
    *phkOut = NULL;
    if (!hk)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        
    DWORD err;
    if (!fCreate)
        err = RegOpenKeyEx(hk, pszSub, 0, MAXIMUM_ALLOWED, phkOut);
    else
        err = RegCreateKeyEx(hk, pszSub, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, phkOut, NULL);

    if (ERROR_SUCCESS != err)
    {
        ASSERT(!*phkOut);
        return HRESULT_FROM_WIN32(err);
    }
    return S_OK;
}
        
HRESULT _AssocGetRegString(HKEY hk, LPCTSTR pszSub, LPCTSTR pszVal, SHSTR &strOut)
{
    if (!hk)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    DWORD cbOut = CbFromCch(strOut.GetSize());
    DWORD err = SHGetValue(hk, pszSub, pszVal, NULL, strOut.GetStr(), &cbOut);

    if (err == ERROR_SUCCESS)
        return S_OK;

    // else try to resize the buffer
    if (cbOut > CbFromCch(strOut.GetSize()))
    {
        strOut.SetSize(cbOut / SIZEOF(TCHAR));
        err = SHGetValue(hk, pszSub, pszVal, NULL, strOut.GetStr(), &cbOut);
    }

    return HRESULT_FROM_WIN32(err);
}

HRESULT _AssocGetRegData(HKEY hk, LPCTSTR pszSubKey, LPCTSTR pszValue, LPDWORD pdwType, LPBYTE pbOut, LPDWORD pcbOut)
{
    
    DWORD err;

    if (pszSubKey || pbOut || pcbOut || pdwType)
        err = SHGetValue(hk, pszSubKey, pszValue, pdwType, pbOut, pcbOut);
    else
        err = RegQueryValueEx(hk, pszValue, NULL, NULL, NULL, NULL);
        
    return HRESULT_FROM_WIN32(err);
}


BOOL _GetAppPath(LPCTSTR pszApp, SHSTR& strPath)
{
    TCHAR sz[MAX_PATH];
    _MakeAppPathKey(pszApp, sz, SIZECHARS(sz));

    return SUCCEEDED(_AssocGetRegString(HKEY_LOCAL_MACHINE, sz, NULL, strPath));
}

//
//  THE NEW WAY!
//

//  to differentiate in the debugger!
#ifdef UNICODE
#define CAssoc     CAssocW
#define ASSOCID    ASSOCIDW
#else
#define CAssoc     CAssocA
#define ASSOCID    ASSOCIDA
#endif


#define ASSOCF_INIT_ALL      (ASSOCF_INIT_BYEXENAME | ASSOCF_INIT_DEFAULTTOFOLDER | ASSOCF_INIT_DEFAULTTOSTAR | ASSOCF_INIT_NOREMAPCLSID)


class CAssoc : public IQueryAssociations
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // IQueryAssociations methods
    STDMETHODIMP Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd);
    STDMETHODIMP GetString(ASSOCF flags, ASSOCSTR str, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut);
    STDMETHODIMP GetKey(ASSOCF flags, ASSOCKEY, LPCWSTR pszExtra, HKEY *phkeyOut);
    STDMETHODIMP GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut);
    STDMETHODIMP GetEnum(ASSOCF flags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut);

    CAssoc();



protected:
    virtual ~CAssoc();

    //  static methods
    static HRESULT _CopyOut(BOOL fNoTruncate, SHSTR str, LPTSTR psz, DWORD *pcch);
    static void _DefaultShellVerb(HKEY hk, LPTSTR pszVerb, DWORD cchVerb, HKEY *phkOut);

    typedef enum {
        KEYCACHE_INVALID = 0,
        KEYCACHE_HKCU    = 1,
        KEYCACHE_HKLM,
        KEYCACHE_APP,
        KEYCACHE_FIXED,
        PSZCACHE_BASE,
        PSZCACHE_HKCU    = PSZCACHE_BASE + KEYCACHE_HKCU,
        PSZCACHE_HKLM,
        PSZCACHE_APP,
        PSZCACHE_FIXED,
    } KEYCACHETYPE;

    typedef struct {
        LPTSTR pszCache;
        HKEY hkCache;
        LPTSTR psz;
        KEYCACHETYPE type;
    } KEYCACHE;
    
    static BOOL _CanUseCache(KEYCACHE &kc, LPCTSTR psz, KEYCACHETYPE type);
    static void _CacheFree(KEYCACHE &kc);
    static void _CacheKey(KEYCACHE &kc, HKEY hkCache, LPCTSTR pszName, KEYCACHETYPE type);
    static void _CacheString(KEYCACHE &kc, LPCTSTR pszCache, LPCTSTR pszName, KEYCACHETYPE type);

    void _Reset(void);

    BOOL _UseBaseClass(void);
    //
    //  retrieve the appropriate cached keys
    //
    HKEY _RootKey(BOOL fForceLM);
    HKEY _AppKey(LPCTSTR pszApp, BOOL fCreate = FALSE);
    HKEY _ExtensionKey(BOOL fForceLM);
    HKEY _OpenProgidKey(LPCTSTR pszProgid);
    HKEY _ProgidKey(HKEY hkIn, BOOL fIsClsid, BOOL fForceLM);
    HKEY _UserProgidKey(void);
    HKEY _ProgidKey(BOOL fForceLM);
    HKEY _ShellVerbKey(HKEY hkProgid, KEYCACHETYPE type, LPCTSTR pszVerb);
    HKEY _ShellVerbKey(BOOL fForceLM, LPCTSTR pszVerb);
    HKEY _ShellNewKey(HKEY hkExt);
    HKEY _ShellNewKey(BOOL fForceLM);
    HKEY _DDEKey(BOOL fForceLM, LPCTSTR pszVerb);

    //
    //  actual worker routines
    //
    HRESULT _GetCommandString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _ParseCommand(ASSOCF flags, LPTSTR pszCommand, SHSTR& strExe, PSHSTR pstrArgs);
    HRESULT _GetExeString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetFriendlyAppByVerb(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetFriendlyAppByApp(LPCTSTR pszApp, ASSOCF flags, SHSTR& strOut);
    HRESULT _GetFriendlyAppString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetInfoTipString(BOOL fForceLM, SHSTR& strOut);
    HRESULT _GetShellNewValueString(BOOL fForceLM, BOOL fQueryOnly, LPCTSTR pszValue, SHSTR& strOut);
    HRESULT _GetDDEApplication(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetDDETopic(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut);
    HRESULT _GetMSIDescriptor(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, LPBYTE pbOut, LPDWORD pcbOut);

    HRESULT _GetShellExecKey(ASSOCF flags, BOOL fForceLM, LPCWSTR pszVerb, HKEY *phkey);
    HRESULT _CloneKey(HKEY hk, HKEY *phkey);


    //
    //  Members
    //
    LONG _cRef;
    TCHAR _szInit[MAX_PATH];
    ASSOCF _assocfBaseClass;
    HWND _hwndInit;

    BITBOOL _fInited:1;
    BITBOOL _fAppOnly:1;
    BITBOOL _fAppPath:1;
    BITBOOL _fInitedBaseClass:1;
    BITBOOL _fIsClsid:1;
    BITBOOL _fNoRemapClsid:1;
    BITBOOL _fBaseClassOnly:1;

    HKEY _hkFileExtsCU;
    HKEY _hkExtensionCU;
    HKEY _hkExtensionLM;

    KEYCACHE _kcProgid;
    KEYCACHE _kcShellVerb;
    KEYCACHE _kcApp;
    KEYCACHE _kcCommand;
    KEYCACHE _kcExecutable;
    KEYCACHE _kcShellNew;
    KEYCACHE _kcDDE;

    IQueryAssociations *_pqaBaseClass;
};

CAssoc::CAssoc() : _cRef(1)
{
};

HRESULT CAssoc::Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd)
{
    //
    //  pszIn can be:
    //  .Ext        //  Detectable
    //  {Clsid}     //  Detectable
    //  Progid      //  Ambiguous 
    //                    Default!
    //  ExeName     //  Ambiguous
    //                    Requires ASSOCF_OPEN_BYEXENAME
    //  MimeType    //  Ambiguous
    //                    NOT IMPLEMENTED...
    //  

    if (!pszAssoc && !hkProgid) 
        return E_INVALIDARG;
    
    HKEY hk = NULL;

    if (_fInited)
        _Reset();
        
    if (pszAssoc)
    {
        _fAppOnly = BOOLIFY(flags & ASSOCF_OPEN_BYEXENAME);

        if (StrChr(pszAssoc, TEXT('\\')))
        {
            // this is a path
            if (_fAppOnly)
                _fAppPath = TRUE;
            else 
            {
                //  we need the extension
                pszAssoc = PathFindExtension(pszAssoc);

                if (!*pszAssoc)
                    pszAssoc = NULL;
            }
            
        }

        if (pszAssoc && *pszAssoc)
        {
            if (ISGUID(pszAssoc))
            {
                _PathAppend(TEXT("CLSID"), pszAssoc, _szInit, SIZECHARS(_szInit));
                _fIsClsid = TRUE;

                // for legacy reasons we dont always 
                //  want to remap the clsid.
                if (flags & ASSOCF_INIT_NOREMAPCLSID)
                    _fNoRemapClsid = TRUE;
            }
            else
            {
                StrCpyN(_szInit , pszAssoc, SIZECHARS(_szInit));

                //  if we initializing to folder dont default to folder.
                if (0 == StrCmpI(_szInit, TEXT("Folder")))
                    flags &= ~ASSOCF_INIT_DEFAULTTOFOLDER;
            }
            
            hk = _ProgidKey(FALSE);

                
        }
    }
    else
    {
        ASSERT(hkProgid);
        hk = SHRegDuplicateHKey(hkProgid);
        if (hk)
            _CacheKey(_kcProgid, hk, NULL, KEYCACHE_FIXED);
    }

    _assocfBaseClass = (flags & (ASSOCF_INIT_DEFAULTTOFOLDER | ASSOCF_INIT_DEFAULTTOSTAR));

    //  
    //  NOTE - we can actually do some work even if 
    //  we were unable to create the applications
    //  key.  so we want to succeed in the case
    //  of an app only association.
    //
    if (hk || _fAppOnly)
    {
        _fInited = TRUE;

        return S_OK;
    }
    else if (_UseBaseClass())
    {
        _fBaseClassOnly = TRUE;
        _fInited = TRUE;
        
        return S_OK;
    }
    
    return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}

CAssoc::~CAssoc()
{
    _Reset();
}

#define REGFREE(hk)    if (hk) { RegCloseKey(hk); (hk) = NULL; } else { }

void CAssoc::_Reset(void)
{
    _CacheFree(_kcProgid);
    _CacheFree(_kcApp);
    _CacheFree(_kcShellVerb);
    _CacheFree(_kcCommand);
    _CacheFree(_kcExecutable);
    _CacheFree(_kcShellNew);
    _CacheFree(_kcDDE);

    REGFREE(_hkFileExtsCU);
    REGFREE(_hkExtensionCU);
    REGFREE(_hkExtensionLM);

    *_szInit = 0;
    _assocfBaseClass = 0;
    _hwndInit = NULL;

    _fInited = FALSE;
    _fAppOnly = FALSE;
    _fAppPath = FALSE;
    _fInitedBaseClass = FALSE;
    _fIsClsid = FALSE;
    _fBaseClassOnly = FALSE;
    
    ATOMICRELEASE(_pqaBaseClass);
}

STDMETHODIMP CAssoc::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAssoc, IQueryAssociations),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CAssoc::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAssoc::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CAssoc::_UseBaseClass(void)
{
    if (!_pqaBaseClass && !_fInitedBaseClass)
    {
        //  try to init the base class
        IQueryAssociations *pqa;
        AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&pqa);
        if (pqa)
        {
            SHSTR strBase;
            if (_fInited && SUCCEEDED(_AssocGetRegString(_ProgidKey(TRUE), NULL, TEXT("BaseClass"), strBase)))
            {
                if (SUCCEEDED(pqa->Init(_assocfBaseClass, strBase, NULL, _hwndInit)))
                    _pqaBaseClass = pqa;
            }

            if (!_pqaBaseClass)
            {
                if ((_assocfBaseClass & ASSOCF_INIT_DEFAULTTOFOLDER)
                && (SUCCEEDED(pqa->Init(0, L"Folder", NULL, _hwndInit))))
                    _pqaBaseClass = pqa;
                else if ((_assocfBaseClass & ASSOCF_INIT_DEFAULTTOSTAR)
                && (SUCCEEDED(pqa->Init(0, L"*", NULL, _hwndInit))))
                    _pqaBaseClass = pqa;
            }

            //  if we couldnt init the BaseClass, then kill the pqa
            if (!_pqaBaseClass)
                pqa->Release();
        }

        _fInitedBaseClass = TRUE;
    }

    return (_pqaBaseClass != NULL);
}
        
HRESULT CAssoc::_CopyOut(BOOL fNoTruncate, SHSTR str, LPTSTR psz, DWORD *pcch)
{
    //  if caller doesnt want any return size, 
    //  the incoming pointer is actually the size of the buffer
    
    ASSERT(pcch);
    ASSERT(psz || !IS_INTRESOURCE(pcch));
    
    HRESULT hr;
    DWORD cch = IS_INTRESOURCE(pcch) ? PtrToUlong(pcch) : *pcch;
    DWORD cchStr = str.GetLen();

    if (psz)
    {
        if (!fNoTruncate || cch > cchStr)
        {
            StrCpyN(psz, str, cch);
            hr = S_OK;
        }
        else
            hr = E_POINTER;
    }
    else
        hr = S_FALSE;
    
    //  return the number of chars written/required
    if (!IS_INTRESOURCE(pcch))
        *pcch = (hr == S_OK) ? lstrlen(psz) + 1 : cchStr + 1;

    return hr;
}


BOOL CAssoc::_CanUseCache(KEYCACHE &kc, LPCTSTR psz, KEYCACHETYPE type)
{
    if (KEYCACHE_FIXED == kc.type)
        return TRUE;
        
    if (KEYCACHE_INVALID != kc.type && type == kc.type)
    {
        return ((!psz && !kc.psz)
            || (psz && kc.psz && 0 == StrCmpC(psz, kc.psz)));
    }
    
    return FALSE;
}

void CAssoc::_CacheFree(KEYCACHE &kc)
{
    if (kc.pszCache)
        LocalFree(kc.pszCache);
    if (kc.hkCache)
        RegCloseKey(kc.hkCache);
    if (kc.psz)
        LocalFree(kc.psz);

    ZeroMemory(&kc, SIZEOF(kc));
}

void CAssoc::_CacheKey(KEYCACHE &kc, HKEY hkCache, LPCTSTR pszName, KEYCACHETYPE type)
{
    _CacheFree(kc);
    ASSERT(hkCache);

    kc.hkCache = hkCache;

    if (pszName)
        kc.psz = StrDup(pszName);
        
    if (!pszName || kc.psz)
        kc.type = type;
}

void CAssoc::_CacheString(KEYCACHE &kc, LPCTSTR pszCache, LPCTSTR pszName, KEYCACHETYPE type)
{
    _CacheFree(kc);
    ASSERT(pszCache && *pszCache);

    kc.pszCache = StrDup(pszCache);
    if (kc.pszCache)
    {
        if (pszName)
            kc.psz = StrDup(pszName);

        if (!pszName || kc.psz)
            kc.type = type;
    }
}

void CAssoc::_DefaultShellVerb(HKEY hk, LPTSTR pszVerb, DWORD cchVerb, HKEY *phkOut)
{
    //  default to "open"
    BOOL fDefaultSpecified = FALSE;
    TCHAR sz[MAX_PATH];
    DWORD cb = SIZEOF(sz);
    *phkOut = NULL;

    //  see if something is specified...
    if (ERROR_SUCCESS == SHGetValue(hk, TEXT("shell"), NULL, NULL, (LPVOID)sz, &cb) && *sz)
        fDefaultSpecified = TRUE;
    else
        StrCpy(sz, TEXT("open"));
        
    HKEY hkShell;
    if (SUCCEEDED(_AssocOpenRegKey(hk, TEXT("shell"), &hkShell)))
    {
        HKEY hkVerb;
        if (FAILED(_AssocOpenRegKey(hkShell, sz, &hkVerb)))
        {
            if (fDefaultSpecified)
            {
                // try to find one of the ordered verbs
                int c = StrCSpn(sz, TEXT(" ,"));
                sz[c] = 0;
                _AssocOpenRegKey(hkShell, sz, &hkVerb);
            }
            else
            {
                // otherwise just use the first key we find....
                cb = SIZECHARS(sz);
                if (ERROR_SUCCESS == RegEnumKeyEx(hkShell, 0, sz, &cb, NULL, NULL, NULL, NULL))
                    _AssocOpenRegKey(hkShell, sz, &hkVerb);
            }
        }

        if (hkVerb)
        {
            if (phkOut)
                *phkOut = hkVerb;
            else
                RegCloseKey(hkVerb);
        }
        
        RegCloseKey(hkShell);
    }

    if (pszVerb)
        StrCpyN(pszVerb, sz, cchVerb);

}

HKEY CAssoc::_RootKey(BOOL fForceLM)
{
    //
    //  this is one of the few places where there is no fallback to LM
    //  if there is no CU, then we return NULL
    //  we need to use a local for CU, but we can use a global for LM
    //

    if (!fForceLM)
    {
        if (!_hkFileExtsCU)
        {
            _AssocOpenRegKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts"), &_hkFileExtsCU);
        }
        
        return _hkFileExtsCU;
    }

    return HKEY_CLASSES_ROOT;

}

HKEY CAssoc::_AppKey(LPCTSTR pszApp, BOOL fCreate)
{
    //  right now we should only get NULL for the pszApp 
    //  when we are initialized with an EXE.
    ASSERT(_fAppOnly || pszApp);
    // else if (!_fAppOnly) TODO: handle getting it from default verb...or not
    
    if (!pszApp)
        pszApp = _fAppPath ? PathFindFileName(_szInit) : _szInit;

    if (_CanUseCache(_kcApp, pszApp, KEYCACHE_APP))
        return _kcApp.hkCache;
    else 
    {
        HKEY hk;
        TCHAR sz[MAX_PATH];
    
        _MakeApplicationsKey(pszApp, sz, SIZECHARS(sz));

        _AssocOpenRegKey(HKEY_CLASSES_ROOT, sz, &hk, fCreate);

        if (hk)
        {
            _CacheKey(_kcApp, hk, pszApp, KEYCACHE_APP);
        }

        return hk;
    }
}

HKEY CAssoc::_ExtensionKey(BOOL fForceLM)
{
    ASSERT(*_szInit);
    if (_fAppOnly)
        return _AppKey(NULL);
    if (!ISEXTENSION(_szInit) && !_fIsClsid)
        return NULL;

    if (!fForceLM)
    {
        if (!_hkExtensionCU)
            _AssocOpenRegKey(_RootKey(FALSE), _szInit, &_hkExtensionCU);

        //  NOTE there is no fallback here
        return _hkExtensionCU;
    }

    if (!_hkExtensionLM)
        _AssocOpenRegKey(_RootKey(TRUE), _szInit, &_hkExtensionLM);

    return _hkExtensionLM;
}

HKEY CAssoc::_OpenProgidKey(LPCTSTR pszProgid)
{
    HKEY hkOut;
    if (SUCCEEDED(_AssocOpenRegKey(_RootKey(TRUE), pszProgid, &hkOut)))
    {
        // Check for a newer version of the ProgID
        TCHAR sz[MAX_PATH];
        DWORD cb = SIZEOF(sz);

        if (ERROR_SUCCESS == SHGetValue(hkOut, TEXT("CurVer"), NULL, NULL, sz, &cb) 
            && (cb > SIZEOF(TCHAR)))
        {
            //  cache this bubby
            HKEY hkTemp = hkOut;            
            if (SUCCEEDED(_AssocOpenRegKey(_RootKey(TRUE), sz, &hkOut)))
            {
                //
                //  BUGBUG LEGACY - order of preference - ZekeL - 22-JUL-99
                //  this is to support associations that installed empty curver
                //  keys, like microsoft project.
                //
                //  1.  curver with shell subkey
                //  2.  progid with shell subkey
                //  3.  curver without shell subkey
                //  4.  progid without shell subkey
                //
                HKEY hkShell;

                if (SUCCEEDED(_AssocOpenRegKey(hkOut, TEXT("shell"), &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkTemp);    // close old ProgID key
                }
                else if (SUCCEEDED(_AssocOpenRegKey(hkTemp, TEXT("shell"), &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkOut);
                    hkOut = hkTemp;
                }
                else
                    RegCloseKey(hkTemp);
                
            }
            else  // reset!
                hkOut = hkTemp;
        }
    }

    return hkOut;
}

//  we only need to build this once, so build it for 
//  the lowest common denominator...
HKEY CAssoc::_ProgidKey(HKEY hkIn, BOOL fRemapToProgid, BOOL fDefaultToIn)
{
    TCHAR sz[MAX_PATH];
    ULONG cb = SIZEOF(sz);
    LPCTSTR psz;
    HKEY hkRet = NULL;

    if (!hkIn && !ISEXTENSION(_szInit))
    {
        psz = _szInit;
    }
    else if (!_fNoRemapClsid && (ERROR_SUCCESS == SHGetValue(hkIn, fRemapToProgid ? TEXT("ProgID") : NULL, NULL, NULL, sz, &cb))
        && (cb > SIZEOF(TCHAR)))
    {
        psz = sz;
    }
    else
        psz = NULL;

    if (psz)
    {
        hkRet = _OpenProgidKey(psz);

    }

    if (!hkRet && fDefaultToIn && hkIn)
        hkRet = SHRegDuplicateHKey(hkIn);

    return hkRet;
}

HKEY CAssoc::_UserProgidKey(void)
{
    SHSTR strApp;
    if (SUCCEEDED(_AssocGetRegString(_ExtensionKey(FALSE), NULL, TEXT("Application"), strApp)))
    {
        HKEY hkRet = _AppKey(strApp);

        if (hkRet)
            return SHRegDuplicateHKey(hkRet);
    }

    return NULL;
}

HKEY CAssoc::_ProgidKey(BOOL fForceLM)
{
    //  BUGBUG - we are not supporting clsids correctly here.
    HKEY hkRet = NULL;
    if (_fAppOnly)
        return _AppKey(NULL);
    else
    {
        KEYCACHETYPE type;
        if (!fForceLM)
            type = KEYCACHE_HKCU;
        else
            type = KEYCACHE_HKLM;

        if (_CanUseCache(_kcProgid, NULL, type))
            hkRet = _kcProgid.hkCache;
        else
        {
            if (!fForceLM)
                hkRet = _UserProgidKey();

            if (!hkRet)
                hkRet = _ProgidKey(_ExtensionKey(TRUE), _fIsClsid, TRUE);

            //  cache the value off
            if (hkRet)
                _CacheKey(_kcProgid, hkRet, NULL, type);
            
        }
    }
    return hkRet;
}

HKEY CAssoc::_ShellVerbKey(HKEY hkProgid, KEYCACHETYPE type, LPCTSTR pszVerb)
{
    HKEY hkRet = NULL;
    //  check our cache
    if (_CanUseCache(_kcShellVerb, pszVerb, type))
        hkRet = _kcShellVerb.hkCache;
    else if (hkProgid)
    {
        //  NO cache hit
        if (!pszVerb)
            _DefaultShellVerb(hkProgid, NULL, 0, &hkRet);
        else
        {
            TCHAR szKey[MAX_PATH];

            _PathAppend(TEXT("shell"), pszVerb, szKey, SIZECHARS(szKey));
            _AssocOpenRegKey(hkProgid, szKey, &hkRet);
        }
        
        // only replace the cache if we got something
        if (hkRet)
            _CacheKey(_kcShellVerb, hkRet, pszVerb, type);
    }

    return hkRet;
}

HKEY CAssoc::_ShellVerbKey(BOOL fForceLM, LPCTSTR pszVerb)
{
    HKEY hk = NULL;

    if (!fForceLM)
        hk = _ShellVerbKey(_ProgidKey(FALSE), KEYCACHE_HKCU, pszVerb);

    if (!hk) 
    {
        KEYCACHETYPE type = (_fAppOnly) ? KEYCACHE_APP : KEYCACHE_HKLM;
        hk = _ShellVerbKey(_ProgidKey(TRUE), type, pszVerb);
    }
    
    return hk;
}

HKEY CAssoc::_ShellNewKey(HKEY hkExt)
{
    //  
    //  shellnew keys look like this
    //  \.ext
    //      @ = "progid"
    //      \progid
    //          \shellnew
    //  -- OR --
    //  \.ext 
    //      \shellnew
    //
    HKEY hk = NULL;
    SHSTR strProgid;
    if (SUCCEEDED(_AssocGetRegString(hkExt, NULL, NULL, strProgid)))
    {
        strProgid.Append(TEXT("\\shellnew"));
        
        _AssocOpenRegKey(hkExt, TEXT("shellnew"), &hk);
    }
    
    if (!hk)
        _AssocOpenRegKey(hkExt, TEXT("shellnew"), &hk);

    return hk;
}

HKEY CAssoc::_ShellNewKey(BOOL fForceLM)
{
    ASSERT(!_fAppOnly);

    if (_CanUseCache(_kcShellNew, NULL, KEYCACHE_HKLM))
        return _kcShellNew.hkCache;

    HKEY hk = _ShellNewKey(_ExtensionKey(TRUE));
        

    if (hk)
        _CacheKey(_kcShellNew, hk, NULL, KEYCACHE_HKLM);

    return hk;
}

HRESULT CAssoc::_GetCommandString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_INVALIDARG;
    KEYCACHETYPE type;

    if (!fForceLM)
        type = PSZCACHE_HKCU;
    else if (_fAppOnly) 
        type = PSZCACHE_APP;
    else
        type = PSZCACHE_HKLM;

    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;

    if (_CanUseCache(_kcCommand, pszVerb, type))
    {
        hr = strOut.SetStr(_kcCommand.pszCache);
    }
    
    if (FAILED(hr))    
    {
        hr = _AssocGetRegString(_ShellVerbKey(fForceLM, pszVerb), TEXT("command"), NULL, strOut);

        if (SUCCEEDED(hr))
        {
            _CacheString(_kcCommand, strOut, pszVerb, type);
        }
    }

    return hr;
}

BOOL _PathIsFile(LPCTSTR pszPath)
{
    DWORD attrs = GetFileAttributes(pszPath);

    return ((DWORD)-1 != attrs && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}
    
HRESULT CAssoc::_ParseCommand(ASSOCF flags, LPTSTR pszCommand, SHSTR& strExe, PSHSTR pstrArgs)
{
    //  we just need to find where the params begin, and the exe ends...

    LPTSTR pch = PathGetArgs(pszCommand);

    if (*pch)
        *(--pch) = TEXT('\0');
    else
        pch = NULL;

    HRESULT hr = strExe.SetStr(pszCommand);

    //  to prevent brace proliferation
    if (S_OK != hr)
        goto quit;

    strExe.Trim();
    PathUnquoteSpaces(strExe);

    //
    //  WARNING:  Expensive disk hits all over!
    //
    // We check for %1 since it is what appears under (for example) HKCR\exefile\shell\open\command
    // This will save us a chain of 35 calls to _PathIsFile("%1") when launching or getting a 
    // context menu on a shortcut to an .exe or .bat file.
    if ((ASSOCF_VERIFY & flags) 
        && (0 != StrCmp(strExe, TEXT("%1")))
        && (!_PathIsFile(strExe)) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        if (PathIsFileSpec(strExe))
        {
            if (_GetAppPath(strExe, strExe))
            {
                if (_PathIsFile(strExe))
                    hr = S_OK;
            }
            else if (PathFindOnPathEx(strExe, NULL, PFOPEX_DEFAULT | PFOPEX_OPTIONAL))
            {
               //  the find does a disk check for us...
               hr = S_OK;
            }
        }
        else             
        {
            //
            //  sometimes the path is not properly quoted.
            //  these keys will still work because of the
            //  way CreateProcess works, but we need to do
            //  some fiddling to figure that out.
            //

            //  if we found args, put them back...
            //  and try some different args
            while (pch)
            {
                *pch++ = TEXT(' ');

                if (pch = StrChr(pch, TEXT(' ')))
                    *pch = TEXT('\0');

                if (S_OK == strExe.SetStr(pszCommand))
                {
                    strExe.Trim();

                    if (_PathIsFile(strExe))
                    {
                        hr = S_FALSE;

                        //  this means that we found something
                        //  but the command line was kinda screwed
                        break;

                    }
                    //  this is where we loop again
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }//  while (pch)
        }
    }

    if (SUCCEEDED(hr) && pch)
    {
        //  currently right before the args, on a NULL terminator
        ASSERT(!*pch);
        pch++;
        
        if ((ASSOCF_REMAPRUNDLL & flags) 
        && 0 == StrCmpNIW(PathFindFileName(strExe), TEXT("rundll"), SIZECHARS(TEXT("rundll")) -1))
        {
            LPTSTR pchComma = StrChr(pch, TEXT(','));
            //  make the comma the beginning of the args
            if (pchComma)
                *pchComma = TEXT('\0');

            if (!*(PathFindExtension(pch)) 
            && lstrlen(++pchComma) > SIZECHARS(TEXT(".dll")))
                StrCat(pch, TEXT(".dll"));

            //  recurse :P
            hr = _ParseCommand(flags, pch, strExe, pstrArgs);
        }
        //  set the args if we got'em
        else if (pstrArgs)
            pstrArgs->SetStr(pch);
    }
    
quit:
    return hr;
}

HRESULT CAssoc::_GetExeString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    KEYCACHETYPE type;

    if (!fForceLM)
        type = PSZCACHE_HKCU;
    else if (_fAppOnly) 
        type = PSZCACHE_APP;
    else
        type = PSZCACHE_HKLM;

    if (_CanUseCache(_kcExecutable, pszVerb, type))
        hr = strOut.SetStr(_kcExecutable.pszCache);

    if (FAILED(hr))
    {
        SHSTR strCommand;

        hr = _GetCommandString(flags, fForceLM, pszVerb, strCommand);
        
        if (S_OK == hr)
        {
            SHSTR strArgs;

            strCommand.Trim();
            hr = _ParseCommand(flags | ASSOCF_REMAPRUNDLL, strCommand, strOut, &strArgs);
            
            if (S_FALSE == hr)
            {
                hr = S_OK;

//                if (!ASSOCF_NOFIXUPS & flags)
//                    AssocSetCommandByKey(ASSOCF_SET_SUBSTENV, hkey, pszVerb, strExe.GetStr(), strArgs.GetStr());
            }
        }

        if (SUCCEEDED(hr))
            _CacheString(_kcExecutable, strOut, pszVerb, type);
    }

    return hr;
}    

HRESULT _AssocGetDarwinProductString(LPCTSTR pszDarwinCommand, SHSTR& strOut)
{
    DWORD cch = strOut.GetSize();
    UINT err = MsiGetProductInfo(pszDarwinCommand, INSTALLPROPERTY_PRODUCTNAME, strOut, &cch);

    if (err == ERROR_MORE_DATA  && cch > strOut.GetSize())
    {
        if (SUCCEEDED(strOut.SetSize(cch)))
            err = MsiGetProductInfo(pszDarwinCommand, INSTALLPROPERTY_PRODUCTNAME, strOut, &cch);
        else 
            return E_OUTOFMEMORY;
    }

    if (err)
        return HRESULT_FROM_WIN32(err);
    return ERROR_SUCCESS;
}

HRESULT CAssoc::_GetFriendlyAppByVerb(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;

    HKEY hk = _ShellVerbKey(fForceLM, pszVerb);

    if (hk)
    {
        HRESULT hr = _AssocGetRegString(hk, NULL, TEXT("FriendlyAppName"), strOut);

        if (FAILED(hr))
        {
            SHSTR str;
            //  check the appkey, for this executeables friendly
            //  name.  this should be the most common case
            hr = _GetExeString(flags, fForceLM, pszVerb, str);

            if (SUCCEEDED(hr))
            {
                hr = _GetFriendlyAppByApp(str, flags, strOut);
            }

            //  if the EXE isnt on the System, then Darwin might
            //  be able to tell us something about it...
            if (FAILED(hr))
            {
                hr = _AssocGetRegString(hk, TEXT("command"), TEXT("command"), str);
                if (SUCCEEDED(hr))
                {
                    hr = _AssocGetDarwinProductString(str, strOut);
                }
            }
        }


        return hr;
    }

    return E_FAIL;
}

HRESULT _GetFriendlyAppByCache(HKEY hkApp, LPCTSTR pszApp, BOOL fVerifyCache, BOOL fNoFixUps, SHSTR& strOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    if (pszApp)
    {
        FILETIME ftCurr;
        if (MyGetLastWriteTime(pszApp, &ftCurr))
        {
            if (fVerifyCache)
            {
                FILETIME ftCache = {0};
                DWORD cbCache = SIZEOF(ftCache);
                SHGetValue(hkApp, TEXT("shell"), TEXT("FriendlyCacheCTime"), NULL, &ftCache, &cbCache);

                if (0 == CompareFileTime(&ftCurr, &ftCache))
                    hr = S_OK;
            }

            if (FAILED(hr))
            {
                //  need to get this from the file itself
                UINT cch = strOut.GetSize();
                if (SHGetFileDescription(pszApp, NULL, NULL, strOut, &cch))
                    hr = S_OK;

                if (SUCCEEDED(hr) && !(fNoFixUps))
                {
                    SHSetValue(hkApp, TEXT("shell"), TEXT("FriendlyCache"), REG_SZ, strOut, CbFromCch(strOut.GetLen() +1));
                    SHSetValue(hkApp, TEXT("shell"), TEXT("FriendlyCacheCTime"), REG_BINARY, &ftCurr, SIZEOF(ftCurr));
                }
            }
        }
    }
    return hr;
}

HRESULT CAssoc::_GetFriendlyAppByApp(LPCTSTR pszApp, ASSOCF flags, SHSTR& strOut)
{
    HKEY hk = _AppKey(pszApp ? PathFindFileName(pszApp) : NULL, TRUE);
    HRESULT hr = _AssocGetRegString(hk, NULL, TEXT("FriendlyAppName"), strOut);

    ASSERT(hk == _kcApp.hkCache);
    
    if (FAILED(hr))
    {
        //  we have now tried the default
        //  we need to try our private cache
        hr = _AssocGetRegString(hk, TEXT("shell"), TEXT("FriendlyCache"), strOut);

        if (flags & ASSOCF_VERIFY)
        {
            SHSTR strExe;
  
            if (!pszApp)
            {
                ASSERT(_fAppOnly);
                if (_fAppPath)
                    pszApp = _szInit;
                else if (SUCCEEDED(_GetExeString(flags, FALSE, NULL, strExe)))
                    pszApp = strExe;
            }

            hr = _GetFriendlyAppByCache(hk, pszApp, SUCCEEDED(hr), (flags & ASSOCF_NOFIXUPS), strOut);
        }
    }
    return hr;
}
        
HRESULT CAssoc::_GetFriendlyAppString(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    // Algorithm:
    //     if there is a named value "friendly", return its value;
    //     if it is a darwin app, return darwin product name;
    //     if there is app key, return its named value "friendly"
    //     o/w, get friendly name from the exe, and cache it in its app key
    
    //  check the verb first.  that overrides the
    //  general exe case
    HRESULT hr; 

    if (_fAppOnly)
        hr = _GetFriendlyAppByApp(NULL, flags, strOut);
    else
    {
        hr = _GetFriendlyAppByVerb(flags, fForceLM, pszVerb, strOut);

    }

    return hr;
}

HRESULT CAssoc::_GetInfoTipString(BOOL fForceLM, SHSTR& strOut)
{
    HRESULT hr = _AssocGetRegString(_ProgidKey(fForceLM), NULL, L"InfoTip", strOut);
    if (FAILED(hr))
        hr = _AssocGetRegString(_ExtensionKey(fForceLM), NULL, L"InfoTip", strOut);
    return hr;
}


HRESULT CAssoc::_GetShellNewValueString(BOOL fForceLM, BOOL fQueryOnly, LPCTSTR pszValue, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _ShellNewKey(fForceLM);

    if (hk)
    {
        TCHAR sz[MAX_PATH];
        if (!pszValue)
        {
            //  get the default value....
            DWORD cch = SIZECHARS(sz);
            //  we want a pszValue....
            if (ERROR_SUCCESS == RegEnumValue(hk, 0, sz, &cch, NULL, NULL, NULL, NULL))
                pszValue = sz;
        }

        hr = _AssocGetRegString(hk, NULL, pszValue, strOut);
    }
    
    return hr;
}

HKEY CAssoc::_DDEKey(BOOL fForceLM, LPCTSTR pszVerb)
{
    HKEY hkRet = NULL;
    KEYCACHETYPE type;
    if (!fForceLM)
        type = KEYCACHE_HKCU;
    else
        type = KEYCACHE_HKLM;

    if (_CanUseCache(_kcDDE, pszVerb, type))
        hkRet = _kcDDE.hkCache;
    else
    {
        if (SUCCEEDED(_AssocOpenRegKey(_ShellVerbKey(fForceLM, pszVerb), TEXT("ddeexec"), &hkRet)))
            _CacheKey(_kcDDE, hkRet, pszVerb, type);
    }

    return hkRet;
}

HRESULT CAssoc::_GetDDEApplication(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _DDEKey(fForceLM, pszVerb);

    if (hk)
    {
        hr = _AssocGetRegString(hk, TEXT("Application"), NULL, strOut);

        if (FAILED(hr) || IsEmptyStr(strOut))
        {
            hr = E_FAIL;
            //  this means we should figure it out
            if (SUCCEEDED(_GetExeString(flags, fForceLM, pszVerb, strOut)))
            {
                PathRemoveExtension(strOut);
                PathStripPath(strOut);

                if (!IsEmptyStr(strOut))
                {
                    //  we have a useful app name
                    hr = S_OK;
                    
                    if (!(flags & ASSOCF_NOFIXUPS))
                    {
                        //  lets put it back!
                        SHSetValue(_DDEKey(fForceLM, pszVerb), TEXT("Application"), NULL, REG_SZ, strOut.GetStr(), CbFromCch(strOut.GetLen() +1));
                    }
                }
            }
        }
    }
    
    return hr;
}

HRESULT CAssoc::_GetDDETopic(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, SHSTR& strOut)
{
    HRESULT hr = E_FAIL;
    HKEY hk = _DDEKey(fForceLM, pszVerb);

    if (hk)
    {
        hr = _AssocGetRegString(hk, TEXT("Topic"), NULL, strOut);

        if (FAILED(hr) || IsEmptyStr(strOut))
            hr = strOut.SetStr(TEXT("System"));
    }
    
    return hr;
}

HRESULT CAssoc::GetString(ASSOCF flags, ASSOCSTR str, LPCTSTR pszExtra, LPTSTR pszOut, DWORD *pcchOut)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;
        
    HRESULT hr = E_INVALIDARG;
    SHSTR strOut;

    if (str && str < ASSOCSTR_MAX && pcchOut && (pszOut || !IS_INTRESOURCE(pcchOut)))
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);

        if (!_fBaseClassOnly)
        {
            switch(str)
            {
            case ASSOCSTR_COMMAND:
                hr = _GetCommandString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_EXECUTABLE:
                hr = _GetExeString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_FRIENDLYAPPNAME:
                hr = _GetFriendlyAppString(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_SHELLNEWVALUE:
                if (!_fAppOnly)
                    hr = _GetShellNewValueString(fForceLM, (pszOut == NULL), pszExtra, strOut);
                break;
                
            case ASSOCSTR_NOOPEN:
                if (!_fAppOnly)
                    hr = _AssocGetRegString(_ProgidKey(fForceLM), NULL, TEXT("NoOpen"), strOut);
                break;

            case ASSOCSTR_FRIENDLYDOCNAME:
                if (!_fAppOnly)
                    hr = _AssocGetRegString(_ProgidKey(fForceLM), NULL, NULL, strOut);
                break;

            case ASSOCSTR_DDECOMMAND:
                hr = _AssocGetRegString(_DDEKey(fForceLM, pszExtra), NULL, NULL, strOut);
                break;
                
            case ASSOCSTR_DDEIFEXEC:
                hr = _AssocGetRegString(_DDEKey(fForceLM, pszExtra), TEXT("IfExec"), NULL, strOut);
                break;

            case ASSOCSTR_DDEAPPLICATION:
                hr = _GetDDEApplication(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_DDETOPIC:
                hr = _GetDDETopic(flags, fForceLM, pszExtra, strOut);
                break;

            case ASSOCSTR_INFOTIP:
                hr = _GetInfoTipString(fForceLM, strOut);
                break;

            default:
                AssertMsg(FALSE, TEXT("CAssoc::GetString() mismatched headers - ZekeL"));
                hr = E_INVALIDARG;
                break;
            }
        }
        
        if (SUCCEEDED(hr))
            hr = _CopyOut(flags & ASSOCF_NOTRUNCATE, strOut, pszOut, pcchOut);
        else if (!(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            HRESULT hrT = _pqaBaseClass->GetString(flags, str, pszExtra, pszOut, pcchOut);
            if (SUCCEEDED(hrT))
                hr = hrT;
        }
    }
    
    return hr;
}

HRESULT CAssoc::_GetMSIDescriptor(ASSOCF flags, BOOL fForceLM, LPCTSTR pszVerb, LPBYTE pbOut, LPDWORD pcbOut)
{
    //BUGBUG what do we do with A/W thunks of REG_MULTI_SZ

    // the darwin ID is always a value name that is the same as the name of the parent key,
    // so instead of reading the default value we read the value with the name of the
    // parent key.
    //
    //  shell
    //    |
    //    -- Open
    //         |
    //         -- Command
    //              (Default)   =   "%SystemRoot%\system32\normal_app.exe"      <-- this is the normal app value
    //              Command     =   "[DarwinID] /c"                             <-- this is the darwin ID value
    //
    //  HACK!  Access 95 (shipping product) creates a "Command" value under
    //  the Command key but it is >>not<< a Darwin ID.  I don't know what
    //  they were smoking.  So we also check the key type and it must be
    //  REG_MULTI_SZ or we will ignore it.
    //
    //
    DWORD dwType;
    HRESULT hr = _AssocGetRegData(_ShellVerbKey(fForceLM, pszVerb), TEXT("command"), TEXT("command"), &dwType, pbOut, pcbOut);

    if (SUCCEEDED(hr) && dwType != REG_MULTI_SZ)
        hr = E_UNEXPECTED;

    return hr;
}

HRESULT CAssoc::GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;
        
    HRESULT hr = E_INVALIDARG;

    if (data && data < ASSOCSTR_MAX)
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);
        DWORD cbReal;
        if (pcbOut && IS_INTRESOURCE(pcbOut))
        {
            cbReal = PtrToUlong(pcbOut);
            pcbOut = &cbReal;
        }

        if (!_fBaseClassOnly)
        {
            switch(data)
            {
            case ASSOCDATA_MSIDESCRIPTOR:
                hr = _GetMSIDescriptor(flags, fForceLM, pszExtra, (LPBYTE)pvOut, pcbOut);
                break;

            case ASSOCDATA_NOACTIVATEHANDLER:
                hr = _AssocGetRegData(_DDEKey(fForceLM, pszExtra), NULL, TEXT("NoActivateHandler"), NULL, (LPBYTE) pvOut, pcbOut);
                break;

            case ASSOCDATA_QUERYCLASSSTORE:
                hr = _AssocGetRegData(_ProgidKey(fForceLM), NULL, TEXT("QueryClassStore"), NULL, (LPBYTE) pvOut, pcbOut);                
                break;

            case ASSOCDATA_HASPERUSERASSOC:
                {
                    HKEY hk = _UserProgidKey();
                    if (hk && _ShellVerbKey(hk, KEYCACHE_HKCU, pszExtra))
                        hr = S_OK;
                    else
                        hr = S_FALSE;

                    REGFREE(hk);
                }
                break;
                
            default:
                AssertMsg(FALSE, TEXT("CAssoc::GetString() mismatched headers - ZekeL"));
                hr = E_INVALIDARG;
                break;
            }
        }

        if (FAILED(hr) && !(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            HRESULT hrT = _pqaBaseClass->GetData(flags, data, pszExtra, pvOut, pcbOut);
            if (SUCCEEDED(hrT))
                hr = hrT;
        }
    }
    
    return hr;
}

HRESULT CAssoc::GetEnum(ASSOCF flags, ASSOCENUM assocenum, LPCTSTR pszExtra, REFIID riid, LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CAssoc::_GetShellExecKey(ASSOCF flags, BOOL fForceLM, LPCWSTR pszVerb, HKEY *phkey)
{
    HKEY hkProgid = NULL;

    if (pszVerb && !*pszVerb) 
        pszVerb = NULL;


    if (!fForceLM)
    {
        hkProgid = _ProgidKey(FALSE);

        if (!(flags & ASSOCF_VERIFY) || _ShellVerbKey(hkProgid, KEYCACHE_HKCU, pszVerb))
            *phkey = SHRegDuplicateHKey(hkProgid);

    }

    if (!*phkey) 
    {
        KEYCACHETYPE type = (_fAppOnly) ? KEYCACHE_APP : KEYCACHE_HKLM;
        hkProgid = _ProgidKey(TRUE);
        if (!(flags & ASSOCF_VERIFY) || _ShellVerbKey(hkProgid, type, pszVerb))
            *phkey = SHRegDuplicateHKey(hkProgid);
    }

    return *phkey ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}
    
HRESULT CAssoc::_CloneKey(HKEY hk, HKEY *phkey)
{
    if (hk)
        *phkey = SHRegDuplicateHKey(hk);

    return *phkey ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
}

HRESULT CAssoc::GetKey(ASSOCF flags, ASSOCKEY key, LPCTSTR pszExtra, HKEY *phkey)
{
    RIP(_fInited);
    if (!_fInited)
        return E_UNEXPECTED;

    HRESULT hr = E_INVALIDARG;
    
    if (key && key < ASSOCKEY_MAX && phkey)
    {
        BOOL fForceLM = (_fAppOnly) || (flags & ASSOCF_NOUSERSETTINGS);
        *phkey = NULL;

        if (!_fBaseClassOnly)
        {
            switch (key)
            {
            case ASSOCKEY_SHELLEXECCLASS:
                hr = _GetShellExecKey(flags, fForceLM, pszExtra, phkey);
                break;

            case ASSOCKEY_APP:
                hr = _fAppOnly ? _CloneKey(_AppKey(NULL), phkey) : E_INVALIDARG;
                break;

            case ASSOCKEY_CLASS:
                hr = _CloneKey(_ProgidKey(fForceLM), phkey);
                break;

            case ASSOCKEY_BASECLASS:
                //  fall through and it is handled by the BaseClass handling
                break;
                
            default:
                AssertMsg(FALSE, TEXT("CAssoc::GetKey() mismatched headers - ZekeL"));
                hr = E_INVALIDARG;
                break;
            }
        }
        
        if (FAILED(hr) && !(flags & ASSOCF_IGNOREBASECLASS) && _UseBaseClass())
        {
            //  it is possible to indicate the depth of the 
            //  base class by pszExtra being an INT
            if (key == ASSOCKEY_BASECLASS)
            {
                int depth = IS_INTRESOURCE(pszExtra) ? LOWORD(pszExtra) : 0;
                if (depth)
                {
                    //  go deeper than this
                    depth--;
                    hr = _pqaBaseClass->GetKey(flags, key, MAKEINTRESOURCE(depth), phkey);
                }
                else
                {
                    //  just return this baseclass
                    hr = _pqaBaseClass->GetKey(flags, ASSOCKEY_CLASS, pszExtra, phkey);
                }
            }
            else
            {
                //  forward to the base class
                hr = _pqaBaseClass->GetKey(flags, key, pszExtra, phkey);
            }
        }
        
    }

    return hr;
}
            
LWSTDAPI AssocCreate(CLSID clsid, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvOut)
    {
        *ppvOut = NULL;

        if (IsEqualGUID(clsid, CLSID_QueryAssociations))
        {
            CAssoc *passoc = new CAssoc();

            if (passoc)
            {
                hr = passoc->QueryInterface(riid, ppvOut);
                passoc->Release();
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_NOTIMPL;
    }

    return hr;
}


LWSTDAPI AssocQueryStringW(ASSOCF flags, ASSOCSTR str, LPCWSTR pszAssoc, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
    HRESULT hr;
    CAssoc*passoc = new CAssoc();

    if (passoc)
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, pszAssoc, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetString(flags, str, pszExtra, pszOut, pcchOut);

        passoc->Release();
    }
    return hr;
}

LWSTDAPI AssocQueryStringA(ASSOCF flags, ASSOCSTR str, LPCSTR pszAssoc, LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
    if (!pcchOut)
        return E_INVALIDARG;
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strAssoc, strExtra;

    if (SUCCEEDED(strAssoc.SetStr(pszAssoc))
    &&  SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        SHSTRW strOut;
        DWORD cchIn = IS_INTRESOURCE(pcchOut) ? PtrToUlong(pcchOut) : *pcchOut;
        DWORD cch;

        if (pszOut)
        {
            strOut.SetSize(cchIn);
            cch = strOut.GetSize();
        }

        hr = AssocQueryStringW(flags, str, strAssoc, strExtra, pszOut ? strOut.GetStr() : NULL, &cch);

        if (SUCCEEDED(hr) && pszOut)
        {
            cch = SHUnicodeToAnsi(strOut, pszOut, cchIn);
        }

        if (!IS_INTRESOURCE(pcchOut))
            *pcchOut = cch;
    }

    return hr;
}

LWSTDAPI AssocQueryStringByKeyW(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut)
{
    HRESULT hr;
    CAssoc*passoc = new CAssoc();

    if (passoc)
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, NULL, hkAssoc, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetString(flags, str, pszExtra, pszOut, pcchOut);

        passoc->Release();
    }
    return hr;
}

LWSTDAPI AssocQueryStringByKeyA(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc, LPCSTR pszExtra, LPSTR pszOut, DWORD *pcchOut)
{
    if (!pcchOut)
        return E_INVALIDARG;
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strExtra;

    if (SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        SHSTRW strOut;
        DWORD cchIn = IS_INTRESOURCE(pcchOut) ? PtrToUlong(pcchOut) : *pcchOut;
        DWORD cch;

        if (pszOut)
        {
            strOut.SetSize(cchIn);
            cch = strOut.GetSize();
        }

        hr = AssocQueryStringByKeyW(flags, str, hkAssoc, strExtra, pszOut ? strOut.GetStr() : NULL, &cch);

        if (SUCCEEDED(hr) && pszOut)
        {
            cch = IS_INTRESOURCE(pcchOut) ? PtrToUlong(pcchOut) : *pcchOut;
            cch = SHUnicodeToAnsi(strOut, pszOut, cch);
        }

        if (!IS_INTRESOURCE(pcchOut))
            *pcchOut = cch;
    }

    return hr;
}

LWSTDAPI AssocQueryKeyW(ASSOCF flags, ASSOCKEY key, LPCWSTR pszAssoc, LPCWSTR pszExtra, HKEY *phkey)
{
    HRESULT hr;
    CAssoc*passoc = new CAssoc();

    if (passoc)
    {
        hr = passoc->Init(flags & ASSOCF_INIT_ALL, pszAssoc, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = passoc->GetKey(flags, key, pszExtra, phkey);

        passoc->Release();
    }

    return hr;
}

LWSTDAPI AssocQueryKeyA(ASSOCF flags, ASSOCKEY key, LPCSTR pszAssoc, LPCSTR pszExtra, HKEY *phkey)
{
        
    HRESULT hr = E_OUTOFMEMORY;
    SHSTRW strAssoc, strExtra;

    if (SUCCEEDED(strAssoc.SetStr(pszAssoc))
    &&  SUCCEEDED(strExtra.SetStr(pszExtra)))
    {
        hr = AssocQueryKeyW(flags, key, strAssoc, strExtra, phkey);

    }

    return hr;
}

#define ISQUOTED(s)   (TEXT('"') == *(s) && TEXT('"') == *((s) + lstrlen(s) - 1))

BOOL _TrySubst(SHSTR& str, LPCTSTR psz)
{
    BOOL fRet = FALSE;
    TCHAR szVar[MAX_PATH];
    DWORD cch = GetEnvironmentVariable(psz, szVar, SIZECHARS(szVar));

    if (cch && cch <= SIZECHARS(szVar))
    {
        if (0 == StrCmpNI(str, szVar, cch))
        {
            //  we got a match. 
            //  size the buffer for the env var... +3 = (% + % + \0)
            SHSTR strT;
            if (S_OK == strT.SetStr(str.GetStr() + cch)                
                && S_OK == str.SetSize(str.GetLen() - cch + lstrlen(psz) + 3)

               )
            {
                wnsprintf(str.GetStr(), str.GetSize(), TEXT("%%%s%%%s"), psz, strT.GetStr());
                fRet = TRUE;
            }
        }
    }
    return fRet;
}
    
BOOL _TryEnvSubst(SHSTR& str)
{
    static LPCTSTR rgszEnv[] = {
        TEXT("USERPROFILE"),
        TEXT("ProgramFiles"),
        TEXT("SystemRoot"),
        TEXT("SystemDrive"),
        TEXT("windir"),
        NULL
    };

    LPCTSTR *ppsz = rgszEnv;
    BOOL fRet = FALSE;

    while (*ppsz && !fRet)
    {
        fRet = _TrySubst(str, *ppsz++);
    }

    return fRet;
}

HRESULT _MakeCommandString(ASSOCF *pflags, LPCTSTR pszExe, LPCTSTR pszArgs, SHSTR& str)
{
    SHSTR strArgs;
    HRESULT hr;
    
    //  BUGBUG the args maybe in the command string
    // hr = _ParseCommand(*pflags, pszExe, str, &strArgs);

    if (!pszArgs || !*pszArgs)
    {
        //  default to just passing the 
        //  file name right in.
        //  NOTE 16bit apps might have a problem with
        //  this, but i request that the caller
        //  specify that this is the case....
        pszArgs = TEXT("\"%1\"");
    }
    //  else NO _ParseCommand()

    hr = str.SetStr(pszExe);

    if (S_OK == hr)
    {
        //  check for quotes before doing env subst
        BOOL fNeedQuotes = (!ISQUOTED(str.GetStr()) && PathIsLFNFileSpec(str));
        
        //  this will put environment vars into the string...
        if ((*pflags & ASSOCMAKEF_SUBSTENV) && _TryEnvSubst(str))
        {
            *pflags |= ASSOCMAKEF_USEEXPAND;
        }

        str.Trim();

        if (fNeedQuotes)
        {
            //  3 = " + " + \0
            if (S_OK == str.SetSize(str.GetLen() + 3))
                PathQuoteSpaces(str.GetStr());
        }

        hr = str.Append(TEXT(' '));

        if (S_OK == hr)
        {
            hr = str.Append(pszArgs);
        }
    }

    return hr;
}
        
HRESULT _AssocMakeCommand(ASSOCMAKEF flags, HKEY hkVerb, LPCWSTR pszExe, LPCWSTR pszArgs)
{                    
    ASSERT(hkVerb && pszExe);
    SHSTR str;
    HRESULT hr = _MakeCommandString(&flags, pszExe, pszArgs, str);

    if (S_OK == hr)
    {
        DWORD dw = (flags & ASSOCMAKEF_USEEXPAND) ? REG_EXPAND_SZ : REG_SZ;

        DWORD err = SHSetValue(hkVerb, TEXT("command"), NULL, dw, (LPVOID) str.GetStr(), CbFromCch(str.GetLen() +1));

        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}

LWSTDAPI AssocMakeShell(ASSOCMAKEF flags, HKEY hkProgid, LPCWSTR pszApplication, ASSOCSHELL *pShell)
{
    HRESULT hr = E_INVALIDARG;

    if (hkProgid && pszApplication && pShell)
    {
        for (DWORD c = 0; c < pShell->cVerbs; c++)
        {
            ASSOCVERB *pverb = &pShell->rgVerbs[c];

            if (pverb->pszVerb)
            {
                TCHAR szVerbKey[MAX_PATH];
                HKEY hkVerb;
                
                _PathAppend(TEXT("shell"), pverb->pszVerb, szVerbKey, SIZECHARS(szVerbKey));
                
                if (c == pShell->iDefaultVerb) 
                    SHSetValue(hkProgid, TEXT("shell"), NULL, REG_SZ, pverb->pszVerb, CbFromCch(lstrlen(pverb->pszVerb) +1));

                //  ASSOCMAKEF_FAILIFEXIST check if its ok to overwrite
                if (SUCCEEDED(_AssocOpenRegKey(hkProgid, szVerbKey, &hkVerb, FALSE)))
                {
                    RegCloseKey(hkVerb);
                    SHDeleteKey(hkProgid, szVerbKey);
                }

                if (SUCCEEDED(_AssocOpenRegKey(hkProgid, szVerbKey, &hkVerb, TRUE)))
                {
                    if (pverb->pszTitle)
                        SHSetValue(hkVerb, NULL, NULL, REG_SZ, pverb->pszTitle, CbFromCch(lstrlen(pverb->pszTitle) +1));

                    hr = _AssocMakeCommand(flags, hkVerb, pverb->pszApplication ? pverb->pszApplication : pszApplication , pverb->pszParams);

                    // if (SUCCEEDED(hr) && pverb->pDDEExec)
                    //    hr = _AssocMakeDDEExec(flags, hkVerb, pverb->pDDEExec);

                    RegCloseKey(hkVerb);
                }
            }
        }
    }
    return hr;
}

HRESULT _OpenClasses(HKEY *phkOut)
{
    *phkOut = NULL;

    DWORD err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\classes"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, phkOut, NULL);
    if (err)
        err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\classes"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, phkOut, NULL);

    return HRESULT_FROM_WIN32(err);
}

LWSTDAPI AssocMakeProgid(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCPROGID *pProgid, HKEY *phkProgid)
{
    HRESULT hr = E_INVALIDARG;

    if (pszApplication 
    && pProgid 
    && pProgid->cbSize >= SIZEOF(ASSOCPROGID) 
    && pProgid->pszProgid 
    && *pProgid->pszProgid)
    {
        HKEY hkRoot;
        if ((!(flags & ASSOCMAKEF_VERIFY)  || PathFileExists(pszApplication))
        && SUCCEEDED(_OpenClasses(&hkRoot)))
        {
            HKEY hkProgid;
            //  need to add support for ASSOCMAKEF_VOLATILE...
            hr = _AssocOpenRegKey(hkRoot, pProgid->pszProgid, &hkProgid, TRUE);

            if (SUCCEEDED(hr))
            {
                if (pProgid->pszFriendlyDocName)
                    SHSetValue(hkProgid, NULL, NULL, REG_SZ, pProgid->pszFriendlyDocName, CbFromCch(lstrlen(pProgid->pszFriendlyDocName) +1));

                if (pProgid->pszDefaultIcon)
                    SHSetValue(hkProgid, TEXT("DefaultIcon"), NULL, REG_SZ, pProgid->pszDefaultIcon, CbFromCch(lstrlen(pProgid->pszDefaultIcon) +1));

                if (pProgid->pShellKey)
                    hr = AssocMakeShell(flags, hkProgid, pszApplication, pProgid->pShellKey);

                if (SUCCEEDED(hr) && pProgid->pszExtensions)
                {
                    LPCTSTR psz = pProgid->pszExtensions;
                    DWORD err = NOERROR;
                    while (*psz && NOERROR == err)
                    {
                        err = SHSetValue(hkRoot, psz, NULL, REG_SZ, pProgid->pszProgid, CbFromCch(lstrlen(pProgid->pszProgid) + 1));
                        psz += lstrlen(psz) + 1;
                    }

                    if (NOERROR != err)
                        HRESULT_FROM_WIN32(err);
                }

                if (SUCCEEDED(hr) && phkProgid)
                    *phkProgid = hkProgid;
                else
                    RegCloseKey(hkProgid);
            }

            RegCloseKey(hkRoot);
        }
    }

    return hr;
}

HRESULT _AssocCopyVerb(HKEY hkSrc, HKEY hkDst, LPCTSTR pszVerb)
{
    HRESULT hr = S_OK;
    TCHAR szKey[MAX_PATH];
    HKEY hkVerb;
    DWORD dwDisp;

    //  only copy the verb component
    wnsprintf(szKey, SIZECHARS(szKey), TEXT("shell\\%s"), pszVerb);

    RegCreateKeyEx(hkDst, szKey, 0, NULL, 0,
        MAXIMUM_ALLOWED, NULL, &hkVerb, &dwDisp);

    //  create a failure state here...
    if (hkVerb)
    {
        //  we avoid overwriting old keys by checking the dwDisp
        if ((dwDisp == REG_CREATED_NEW_KEY) && SHCopyKey(hkSrc, pszVerb, hkVerb, 0L))
            hr = E_UNEXPECTED;

        RegCloseKey(hkVerb);
    }

    return hr;
}

typedef BOOL (*PFNALLOWVERB)(LPCWSTR psz, LPARAM param);

LWSTDAPI _AssocCopyVerbs(HKEY hkSrc, HKEY hkDst, PFNALLOWVERB pfnAllow, LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;
    HKEY hkEnum;
    
    if (SUCCEEDED(_AssocOpenRegKey(hkSrc, TEXT("shell"), &hkEnum, FALSE)))
    {
        TCHAR szVerb[MAX_PATH];
        DWORD cchVerb = SIZECHARS(szVerb);

        for (DWORD i = 0
            ; (NOERROR == RegEnumKeyEx(hkEnum, i, szVerb, &cchVerb, NULL, NULL, NULL, NULL))
            ; (cchVerb = SIZECHARS(szVerb)), i++)
        {
            if (!pfnAllow || pfnAllow(szVerb, lParam))
                hr = _AssocCopyVerb(hkEnum, hkDst, szVerb);
        }

        //  switch to cbVerb here
        cchVerb = SIZEOF(szVerb);
        if (NOERROR == SHGetValue(hkEnum, NULL, NULL, NULL, szVerb, &cchVerb))
        {
            SHSetValue(hkDst, TEXT("shell"), NULL, REG_SZ, szVerb, cchVerb);
        }
        
        RegCloseKey(hkEnum);
    }

    return hr;
}

LWSTDAPI AssocCopyVerbs(HKEY hkSrc, HKEY hkDst)
{
    return _AssocCopyVerbs(hkSrc, hkDst, NULL, NULL);
}

BOOL _IsMSIPerUserInstall(IQueryAssociations *pqa, ASSOCF flags, LPCWSTR pszVerb)
{
    WCHAR sz[MAX_PATH];
    DWORD cb = SIZEOF(sz);
    
    if (SUCCEEDED(pqa->GetData(flags, ASSOCDATA_MSIDESCRIPTOR, pszVerb, sz, &cb)))
    {
        WCHAR szOut[3];  // bit enough for "1" or "0"
        cb = SIZECHARS(szOut);
        
        if (NOERROR == MsiGetProductInfoW(sz, INSTALLPROPERTY_ASSIGNMENTTYPE, szOut, &cb))
        {
            //  The string "1" for the value represents machine installations, 
            //  while "0" represents user installations.

            if (0 == StrCmpW(szOut, L"0"))
                return TRUE;
        }

    }

    return FALSE;
}
    
typedef struct {
    IQueryAssociations *pqa;
    ASSOCF Qflags;
    LPCWSTR pszExe;
    BOOL fAllowPerUser;
} QUERYEXECB;

BOOL _AllowExeVerb(LPCWSTR pszVerb, QUERYEXECB *pqcb)
{
    BOOL fRet = FALSE;
    WCHAR sz[MAX_PATH];
    if (SUCCEEDED(pqcb->pqa->GetString(pqcb->Qflags, ASSOCSTR_EXECUTABLE, pszVerb,
        sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
    {
        if (0 == StrCmpIW(PathFindFileNameW(sz), pqcb->pszExe))
        {
            // 
            //  EXEs match so we should copy this verb.
            //  but we need to block per-user installs by darwin being added to the 
            //  applications key, since other users wont be able to use them
            //
            if (_IsMSIPerUserInstall(pqcb->pqa, pqcb->Qflags, pszVerb))
                fRet = pqcb->fAllowPerUser;
            else
                fRet = TRUE;
        }
    }
    //  todo  mask off DARWIN per-user installs

    return fRet;
}

HRESULT _AssocCreateAppKey(LPCWSTR pszExe, BOOL fPerUser, HKEY *phk)
{   
    WCHAR szKey[MAX_PATH];
    wnsprintf(szKey, SIZECHARS(szKey), L"software\\classes\\applications\\%s", pszExe);

    if (*PathFindExtension(pszExe) == 0)
    {
        StrCatBuff(szKey, TEXT(".exe"), SIZECHARS(szKey));
    }

    return _AssocOpenRegKey(fPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szKey, phk, TRUE);
}

LWSTDAPI AssocMakeApplicationByKeyW(ASSOCMAKEF flags, HKEY hkSrc, LPCWSTR pszVerb)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    ASSOCF Qflags = (flags & ASSOCMAKEF_VERIFY) ? ASSOCF_VERIFY : 0;
    IQueryAssociations *pqa = (IQueryAssociations *) new CAssocW();

    if (!pqa)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(pqa->Init(0, NULL, hkSrc, NULL)))
    {
        
        if (SUCCEEDED(pqa->GetString(Qflags, ASSOCSTR_EXECUTABLE, pszVerb, 
            szPath, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szPath))))
            && (0 != StrCmpW(szPath, TEXT("%1"))))
        {
            LPCWSTR pszExe = PathFindFileNameW(szPath);
            BOOL fPerUser = _IsMSIPerUserInstall(pqa, Qflags, pszVerb);
            HKEY hkDst;
            
            ASSERT(pszExe && *pszExe);
            //  we have an exe to use

            //  check to see if this Application already has
            //  this verb installed
            DWORD cch;
            hr = AssocQueryString(Qflags | ASSOCF_OPEN_BYEXENAME, ASSOCSTR_COMMAND, pszExe,
                pszVerb, NULL, &cch);

            if (FAILED(hr)
            && SUCCEEDED(_AssocCreateAppKey(pszExe, fPerUser, &hkDst)))
            {
                QUERYEXECB qcb = {pqa, Qflags, pszExe, fPerUser};
                
                if (pszVerb)
                {
                    if (_AllowExeVerb(pszVerb, &qcb))
                        hr = _AssocCopyVerb(hkSrc, hkDst, pszVerb);
                }
                else
                    hr = _AssocCopyVerbs(hkSrc, hkDst, (PFNALLOWVERB)_AllowExeVerb, (LPARAM)&qcb);

                RegCloseKey(hkDst);
            }
            
            //  init the friendly name for later
            if ((flags & ASSOCMAKEF_VERIFY) && SUCCEEDED(hr))
                AssocQueryString(ASSOCF_OPEN_BYEXENAME | Qflags, ASSOCSTR_FRIENDLYAPPNAME, 
                    pszExe, NULL, NULL, &cch);
        }

        pqa->Release();
    }
    
    return hr;
}

LWSTDAPI AssocMakeApplicationByKeyA(ASSOCMAKEF flags, HKEY hkAssoc, LPCSTR pszVerb)
{
    SHSTRW strVerb;
    HRESULT hr = strVerb.SetStr(pszVerb);

    if (SUCCEEDED(hr))
        hr = AssocMakeApplicationByKeyW(flags, hkAssoc, strVerb);

    return hr;
}

