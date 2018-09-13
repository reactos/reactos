#include "shellprv.h"

#include "intshcut.h"
#include "sfviewp.h"
#include "ids.h"
#include <ntquery.h>    // defines some values used for fmtid and pid
#include "prop.h"       // SCID_ stuff
#include "netview.h"    // SHWNetGetConnection

///////////////
//
// This file contains the implementation of the column handlers
// Called from fstreex's FS_HandleExtendedColumn
//

STDAPI CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
STDAPI CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
STDAPI CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);

// A PROPVARIANT can hold a few more types than a VARIANT can.  We convert the types that are
// only supported by a PROPVARIANT into equivalent VARIANT types.
void PropVariantToVariant(const PROPVARIANT *pPropVar, VARIANT *pVar)
{
    ASSERT(pPropVar && pVar);

    // if pVar isn't empty, this will properly free before overwriting it
    VariantClear(pVar);

    switch (pPropVar->vt)
    {
        case VT_LPSTR: 
            pVar->bstrVal = SysAllocStringA(pPropVar->pszVal);
            if (pVar->bstrVal)
                pVar->vt = VT_BSTR;
            break;

        case VT_LPWSTR:
            pVar->bstrVal = SysAllocString(pPropVar->pwszVal);
            if (pVar->bstrVal)
                pVar->vt = VT_BSTR;
            break;

        case VT_FILETIME:
        {
            SYSTEMTIME st;
            pVar->vt = VT_DATE;
            FileTimeToSystemTime(&pPropVar->filetime, &st);
            SystemTimeToVariantTime(&st, &pVar->date); // delay load...
            break;
        }

        case VT_UI2:
            if (pPropVar->uiVal < 0x8000)
            {
                pVar->vt = VT_I2;
                pVar->iVal = (signed short) pPropVar->uiVal;
            }
            break;

        case VT_UI4:
            if (pPropVar->ulVal < 0x80000000)
            {
                pVar->vt = VT_I4;
                pVar->lVal = (signed int) pPropVar->ulVal;
            }
            break;

        case VT_CLSID:
            if (pVar->bstrVal = SysAllocStringLen(NULL, GUIDSTR_MAX))
                if (SUCCEEDED(SHStringFromGUIDW(*pPropVar->puuid, pVar->bstrVal, GUIDSTR_MAX)))
                    pVar->vt = VT_BSTR;
            break;

        case VT_BLOB:
        case VT_STREAM:
        case VT_STORAGE:
        case VT_BLOB_OBJECT:
        case VT_STREAMED_OBJECT:
        case VT_STORED_OBJECT:
        case VT_CF:
            ASSERT(0); // leave the output cleared
            break;

        default:
            VariantCopy(pVar, (VARIANT *)pPropVar);
            break;
    }
}

HRESULT ReadProperty(IPropertySetStorage *pPropSetStg, const FMTID *pfmtid, DWORD dwPid, VARIANT *pVar)
{
    VariantInit(pVar);

    IPropertyStorage *pPropStg;
    UINT uCodePage;

    HRESULT hr = SHPropStgCreate( pPropSetStg, *pfmtid, NULL, PROPSETFLAG_DEFAULT, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                  OPEN_EXISTING, &pPropStg, &uCodePage );
    if (SUCCEEDED(hr))
    {
        PROPSPEC PropSpec;
        PROPVARIANT PropVar;

        PropSpec.ulKind = PRSPEC_PROPID;
        PropSpec.propid = dwPid;

        hr = SHPropStgReadMultiple( pPropStg, uCodePage, 1, &PropSpec, &PropVar );
        if (SUCCEEDED(hr))
        {
            PropVariantToVariant(&PropVar, pVar);
            PropVariantClear(&PropVar);
        }
        pPropStg->Release();
    }
    return hr;
}

typedef struct {
    const SHCOLUMNID *pscid;
    VARTYPE vt;             // Note that the type of a given FMTID/PID pair is a known, fixed value
    DWORD fmt;              // listview format (LVCFMT_LEFT, usually)
    UINT cChars;            // count of chars (default col width)
    DWORD csFlags;          // SHCOLSTATE flags
    UINT idTitle;           // string id for title
    // UINT idDescription;  // BUGBUG: add these, make defview display them too!
} COLUMNINFO_RES;



class CBaseColumnProvider : public IPersist, public IColumnProvider
{
    // IUnknown methods
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvOut)
    {
        static const QITAB qit[] = {
            QITABENT(CBaseColumnProvider, IColumnProvider),           // IID_IColumnProvider
            QITABENT(CBaseColumnProvider, IPersist),           // IID_IColumnProvider
            { 0 },
        };
        return QISearch(this, qit, riid, ppvOut);
    };
    
    virtual STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    };

    virtual STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&_cRef))
            return _cRef;

        delete this;
        return 0;
    };

    // IPersist
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) { *pClassID = *_pclsid; return S_OK; };

    // IColumnProvider
    virtual STDMETHODIMP Initialize( LPCSHCOLUMNINIT psci )    { return S_OK ; }
    virtual STDMETHODIMP GetColumnInfo(DWORD dwIndex, LPSHCOLUMNINFO psci);

protected:
    CBaseColumnProvider(const CLSID *pclsid, const COLUMNINFO_RES rgColMap[], int iCount, const LPCWSTR rgExts[]) : 
       _cRef(1), _pclsid(pclsid), _rgColumns(rgColMap), _iCount(iCount), _rgExts(rgExts)
    {
        DllAddRef() ;
    };
    virtual ~CBaseColumnProvider()
    {
        DllRelease() ;
    }

    // helper fns
    BOOL   _IsHandled(LPCWSTR pszExt);
    int _iCount;
    const COLUMNINFO_RES  *_rgColumns;

private:
    // variables
    long _cRef;
    const CLSID * _pclsid;
    const LPCWSTR *_rgExts;

};

// the index is an arbitrary zero based index used for enumeration

STDMETHODIMP CBaseColumnProvider::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
    ZeroMemory(psci, sizeof(*psci));

    if (dwIndex < (UINT) _iCount)
    {
        psci->scid = *_rgColumns[dwIndex].pscid;
        psci->cChars = _rgColumns[dwIndex].cChars;
        psci->vt = _rgColumns[dwIndex].vt;
        psci->fmt = _rgColumns[dwIndex].fmt;
        psci->csFlags = _rgColumns[dwIndex].csFlags;
        LoadStringW(HINST_THISDLL, _rgColumns[dwIndex].idTitle, psci->wszTitle, ARRAYSIZE(psci->wszTitle));
        return S_OK;
    }
    return S_FALSE;
}

// see if this file type is one we are interested in
BOOL CBaseColumnProvider::_IsHandled(LPCWSTR pszExt)
{
    if (_rgExts)
    {
        int i = 0;
        while (_rgExts[i])
        {
            if (0 == StrCmpIW(pszExt, _rgExts[i]))
                return TRUE;
            i++;
        }
        return FALSE;
    }
    return TRUE;
}

// OLE structured storage (DocFile) handler

const COLUMNINFO_RES c_rgDocObjColumns[] = {
    { &SCID_Author,     VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_AUTHOR,        },
    { &SCID_Title,      VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_TITLE,         },
    { &SCID_Subject,    VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_SUBJECT,       },
    { &SCID_Category,   VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_CATEGORY,      },
    { &SCID_PageCount,  VT_I4   , LVCFMT_LEFT, 10, SHCOLSTATE_TYPE_INT, IDS_EXCOL_PAGECOUNT,     },
    { &SCID_Comment,    VT_LPSTR, LVCFMT_LEFT, 30, SHCOLSTATE_TYPE_STR, IDS_EXCOL_COMMENT,       },
    { &SCID_Copyright,  VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_COPYRIGHT,     },
};

class CDocFileColumns : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    // help on initializing base classes: mk:@ivt:vclang/FB/DD/S44B5E.HTM
    CDocFileColumns() : 
       CBaseColumnProvider(&CLSID_DocFileColumnProvider, c_rgDocObjColumns, ARRAYSIZE(c_rgDocObjColumns), NULL)
    {
    };
    
    ~CDocFileColumns()
    {
        _FreeCache();
    }
    
    // for the cache
    VARIANT _rgvCache[ARRAYSIZE(c_rgDocObjColumns)]; // zero'ing allocator will fill with VT_EMPTY
    WCHAR _wszLastFile[MAX_PATH];
    HRESULT _hrCache;
#ifdef DEBUG
    int deb_dwTotal, deb_dwMiss;
#endif
    
    void _FreeCache();

    friend HRESULT CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

void CDocFileColumns::_FreeCache()
{
    for (int i = 0; i < ARRAYSIZE(_rgvCache); i++)
        VariantClear(&_rgvCache[i]);

    _hrCache = S_OK;
}

STDMETHODIMP CDocFileColumns::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    HRESULT hr;

#ifdef DEBUG
    deb_dwTotal++;
#endif

    // see if its in the cache
    // We only check the cache if the SHCDF_UPDATEITEM flag was not passed in.  This flag is a hint
    // that the file for which we are getting data has changed since the last call.  This flag
    // is only passed once per filename, not once per column per filename so update the entire
    // cache if this flag is set.
    if (((pscd->dwFlags & SHCDF_UPDATEITEM) == 0) && (StrCmpW(_wszLastFile, pscd->wszFile) == 0))
    {
        hr = S_FALSE;       // assume we don't have it

        if (S_OK == _hrCache)
        {
            // find the index
            for (int i = 0; i< _iCount; i++)
            {
                if (IsEqualSCID(*_rgColumns[i].pscid, *pscid))
                {
                    if (_rgvCache[i].vt != VT_EMPTY)
                        hr = VariantCopy(pvarData, &_rgvCache[i]);
                    break;
                }
            }
        }
    }
    else
    {
        // sanity check our caching.  If the shell thread pool is > 1, we will thrash like mad, and should change this
#ifdef DEBUG
        deb_dwMiss++;
        if ((deb_dwTotal > 3) && (deb_dwTotal / deb_dwMiss <= 3))
            TraceMsg(TF_WARNING, "Column data caching is ineffective (%d misses for %d access)", deb_dwMiss, deb_dwTotal);
#endif
        _FreeCache();

        StrCpyW(_wszLastFile, pscd->wszFile);

        IPropertySetStorage *pPropSetStg;
        hr = SHStgOpenStorageW(pscd->wszFile, STGM_READ | STGM_SHARE_DENY_WRITE,
                               0, 0,  IID_IPropertySetStorage, (void **)&pPropSetStg);
        if (SUCCEEDED(hr))
        {
            hr = E_INVALIDARG; // normally overwritten by hrT below
            for (int i = 0; i < _iCount; i++)
            {
                // it would be slightly more efficient, but more code, to set up the propid array to call ReadMultiple
                HRESULT hrT = ReadProperty(pPropSetStg, &_rgColumns[i].pscid->fmtid, _rgColumns[i].pscid->pid, &_rgvCache[i]);
                if (IsEqualSCID(*_rgColumns[i].pscid, *pscid))
                {
                    VariantCopy(pvarData, &_rgvCache[i]);
                    hr = hrT;
                }
            }
            pPropSetStg->Release();
        }
        _hrCache = hr;
    }
    return hr;
}

STDAPI CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CDocFileColumns *pdocp = new CDocFileColumns;
    if (pdocp)
    {
        hr = pdocp->QueryInterface(riid, ppv);
        pdocp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// Shortcut handler

// W because pidl is always converted to widechar filename
const LPCWSTR c_szURLExtensions[] = {
    L".URL", 
    L".LNK", 
    NULL
};

const COLUMNINFO_RES c_rgURLColumns[] = {
    { &SCID_Author,     VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_AUTHOR,   },
    { &SCID_Title,      VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, IDS_EXCOL_TITLE,    },
    { &SCID_Comment,    VT_LPSTR, LVCFMT_LEFT, 30, SHCOLSTATE_TYPE_STR, IDS_EXCOL_COMMENT,  }
};


class CLinkColumnProvider : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    // help on initializing base classes: mk:@ivt:vclang/FB/DD/S44B5E.HTM
    CLinkColumnProvider() : CBaseColumnProvider(&CLSID_LinkColumnProvider, c_rgURLColumns, ARRAYSIZE(c_rgURLColumns), c_szURLExtensions)
    {};

    // friends
    friend HRESULT CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

const struct 
{
    DWORD dwSummaryPid;
    DWORD dwURLPid;
} c_URLMap[] =  {
    { PIDSI_AUTHOR,   PID_INTSITE_AUTHOR },
    { PIDSI_TITLE,    PID_INTSITE_TITLE },
    { PIDSI_COMMENTS, PID_INTSITE_COMMENT },
};

DWORD _MapSummaryToSitePID(DWORD pid)
{
    for (int i = 0; i < ARRAYSIZE(c_URLMap); i++)
    {
        if (c_URLMap[i].dwSummaryPid == pid)
            return c_URLMap[i].dwURLPid;
    }
    return -1;
}

STDMETHODIMP CLinkColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    HRESULT hr;
    USES_CONVERSION;
    const CLSID *pclsidLink = &CLSID_ShellLink;

    // should we match against a list of known extensions, or always try to open?

    if (FILE_ATTRIBUTE_DIRECTORY & pscd->dwFileAttributes)
    {
        if (PathIsShortcut(W2CT(pscd->wszFile)))
        {
            pclsidLink = &CLSID_FolderShortcut;     // we are dealing with a folder shortcut now
        }
        else
        {
            return S_FALSE;
        }
    }
    else
    {
        if (!_IsHandled(pscd->pwszExt))
        {
            return S_FALSE;
        }
    }

    if (StrCmpIW(pscd->pwszExt, L".URL") == 0)
    {
        //
        // its a .URL so lets handle it by creating the Internet Shortcut object, loading
        // the file and then reading the properties from it.
        //
        IPropertySetStorage *pPropSetStg;
        hr = LoadFromFile(&CLSID_InternetShortcut, W2CT(pscd->wszFile), IID_IPropertySetStorage, (void **)&pPropSetStg);
        if (SUCCEEDED(hr))
        {
            UINT pid;
            GUID fmtid;

            if (IsEqualGUID(pscid->fmtid, FMTID_SummaryInformation))
            {
                fmtid = FMTID_InternetSite;
                pid = _MapSummaryToSitePID(pscid->pid);
            }
            else
            {
                fmtid = pscid->fmtid;
                pid = pscid->pid;
            }

            hr = ReadProperty(pPropSetStg, &fmtid, pid, pvarData);
            pPropSetStg->Release();
        }
    }
    else
    {
        //
        // open the .LNK file, load it and then read the description for it.  we then
        // return this a the comment for this object.
        //

        if (IsEqualSCID(*pscid, SCID_Comment))
        {
            IShellLink *psl;
            hr = LoadFromFile(pclsidLink, W2CT(pscd->wszFile), IID_IShellLink, (void**)&psl);
            if (SUCCEEDED(hr))
            {
                TCHAR szBuffer[MAX_PATH];

                hr = psl->GetDescription(szBuffer, ARRAYSIZE(szBuffer));            
                if ( SUCCEEDED(hr) && szBuffer[0] )
                {
                    hr = InitVariantFromStr(pvarData, szBuffer);
                }
                else
                {
                    IQueryInfo *pqi;
                    if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IQueryInfo, &pqi))))
                    {
                        WCHAR *pwszTip;

                        if (SUCCEEDED(pqi->GetInfoTip(0, &pwszTip)) && pwszTip)
                        {
                            hr = InitVariantFromStr(pvarData, W2CT(pwszTip));
                            SHFree(pwszTip);
                        }
                        pqi->Release();
                    }
                }

                psl->Release();
            }
        }
        else
            hr = S_FALSE;
    }

    return hr;
}

STDAPI CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CLinkColumnProvider *pdocp = new CLinkColumnProvider;
    if (pdocp)
    {
        hr = pdocp->QueryInterface(riid, ppv);
        pdocp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// Folder column handler

#ifdef WINNT
// Warning!  Before you enable SUPPORTS_OWNERSHIP on Win9x, make sure
// to write an apithk wrapper, because Win95 does not have GetNamedSecurityInfo.
#define SUPPORTS_OWNERSHIP
#endif

const COLUMNINFO_RES c_rgFileSysColumns[] = {
    { &SCID_Comment,       VT_LPSTR, LVCFMT_LEFT, 30, SHCOLSTATE_TYPE_STR,                              IDS_EXCOL_COMMENT,    },
    { &SCID_CREATETIME,    VT_DATE,  LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_DATE,                             IDS_EXCOL_CREATE,     },
    { &SCID_ACCESSTIME,    VT_DATE,  LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_DATE,                             IDS_EXCOL_ACCESSTIME, },
#ifdef SUPPORTS_OWNERSHIP
    { &SCID_OWNER,         VT_LPSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_SECONDARYUI,    IDS_EXCOL_OWNER, },
#endif
    { &SCID_HTMLINFOTIPFILE, VT_LPSTR, LVCFMT_LEFT, 30, SHCOLSTATE_TYPE_STR | SHCOLSTATE_HIDDEN,        0,    },
};

class CFileSysColumnProvider : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    CFileSysColumnProvider() : CBaseColumnProvider(&CLSID_FileSysColumnProvider, c_rgFileSysColumns, ARRAYSIZE(c_rgFileSysColumns), NULL)
    {
        ASSERT(_wszLastFile[0] == 0);
#ifdef SUPPORTS_OWNERSHIP
        ASSERT(_psid==NULL && _pwszName==NULL && _psd==NULL);
#endif
    };

#ifdef SUPPORTS_OWNERSHIP
    ~CFileSysColumnProvider() { _CacheSidName(NULL, NULL, NULL); }
#endif

    WCHAR _wszLastFile[MAX_PATH];
    WIN32_FIND_DATA _fd;

#ifdef SUPPORTS_OWNERSHIP
    //
    //  Since we typically get pinged for files all in the same folder,
    //  cache the "folder to server" mapping to avoid calling
    //  WNetGetConnection five million times.
    //
    //  Since files in the same directory tend to have the same owner,
    //  we cache the SID/Name mapping.
    //
    //  Column providers do not have to support multithreaded clients,
    //  so we won't take any critical sections.
    //

    HRESULT _LookupOwnerName(LPCTSTR pszFile, VARIANT *pvar);
    void _CacheSidName(PSECURITY_DESCRIPTOR psd, LPVOID psid, LPCWSTR pwszName);

    LPVOID               _psid;
    LPWSTR               _pwszName;
    PSECURITY_DESCRIPTOR _psd;          // _psid points into here

    int                  _iCachedDrive; // What drive letter is cached in _pszServer?
    LPTSTR               _pszServer;    // What server to use (NULL = local machine)

#endif

    friend HRESULT CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

#ifdef SUPPORTS_OWNERSHIP
//
//  _CacheSidName takes ownership of the psd.  (psid points into the psd)
//
void CFileSysColumnProvider::_CacheSidName(PSECURITY_DESCRIPTOR psd, LPVOID psid, LPCWSTR pwszName)
{
    if (_psd)
        LocalFree(_psd);
    _psd = psd;
    _psid = psid;

    Str_SetPtrW(&_pwszName, pwszName);
}

//
//  Given a string of the form \\server\share\blah\blah, stomps the
//  inner backslash (if necessary) and returns a pointer to "server".
//
STDAPI_(LPTSTR) PathExtractServer(LPTSTR pszUNC)
{
    if (PathIsUNC(pszUNC))
    {
        pszUNC += 2;            // Skip over the two leading backslashes
        LPTSTR pszEnd = StrChr(pszUNC, TEXT('\\'));
        if (pszEnd) *pszEnd = TEXT('\0'); // nuke the backslash
    }
    else
        pszUNC = NULL;
    return pszUNC;
}


HRESULT CFileSysColumnProvider::_LookupOwnerName(LPCTSTR pszFile, VARIANT *pvar)
{
    DWORD err;
    PSECURITY_DESCRIPTOR psd;
    LPVOID psid;
    USES_CONVERSION;

    pvar->vt = VT_BSTR;
    pvar->bstrVal = NULL;

    err = GetNamedSecurityInfo(const_cast<LPTSTR>(pszFile),
                               SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                               &psid, NULL, NULL, NULL, &psd);
    if (err == ERROR_SUCCESS)
    {
        if (_psid && EqualSid(psid, _psid) && _pwszName)
        {
            pvar->bstrVal = SysAllocString(_pwszName);
            LocalFree(psd);
            err = ERROR_SUCCESS;
        }
        else
        {
            LPTSTR pszServer;
            TCHAR szServer[MAX_PATH];

            //
            //  Now go figure out which server to resolve the SID against.
            //
            if (PathIsUNC(pszFile))
            {
                lstrcpyn(szServer, pszFile, ARRAYSIZE(szServer));
                pszServer = PathExtractServer(szServer);
            }
            else if (pszFile[0] == _iCachedDrive)
            {
                // Local drive letter already in cache -- use it
                pszServer = _pszServer;
            }
            else
            {
                // Local drive not cached -- cache it
                _iCachedDrive = pszFile[0];
                DWORD cch = ARRAYSIZE(szServer);
                if (SHWNetGetConnection(pszFile, szServer, &cch) == NO_ERROR)
                    pszServer = PathExtractServer(szServer);
                else
                    pszServer = NULL;
                Str_SetPtr(&_pszServer, pszServer);
            }

            TCHAR szName[MAX_PATH];
            DWORD cchName = ARRAYSIZE(szName);
            TCHAR szDomain[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD cchDomain = ARRAYSIZE(szDomain);
            SID_NAME_USE snu;
            LPTSTR pszName;
            BOOL fFreeName = FALSE; // Do we need to LocalFree(pszName)?

            if (LookupAccountSid(pszServer, psid, szName, &cchName,
                                 szDomain, &cchDomain, &snu))
            {
                //
                //  If the domain is the bogus "BUILTIN" or we don't have a domain
                //  at all, then just use the name.  Otherwise, use domain\userid.
                //
                if (!szDomain[0] || StrCmpC(szDomain, TEXT("BUILTIN")) == 0)
                {
                    pszName = szName;
                }
                else
                {
                    // Borrow szServer as a scratch buffer
                    wnsprintf(szServer, ARRAYSIZE(szServer), TEXT("%s\\%s"), szDomain, szName);
                    pszName = szServer;
                }
                err = ERROR_SUCCESS;
            }
            else
            {
                err = GetLastError();

                // Couldn't map the SID to a name.  Use the horrid raw version
                // if available.
                if (ConvertSidToStringSid(psid, &pszName))
                {
                    fFreeName = TRUE;
                    err = ERROR_SUCCESS;
                }
                else
                    pszName = NULL;
            }

            // Even on error, cache the result so we don't keep trying over and over
            // on the same SID.

            LPWSTR pwszName = T2W(pszName);
            _CacheSidName(psd, psid, pwszName);
            pvar->bstrVal = SysAllocString(pwszName);

            if (fFreeName)
                LocalFree(pszName);
        }
    }

    if (err == ERROR_SUCCESS && pvar->bstrVal == NULL)
        err = ERROR_OUTOFMEMORY;

    return HRESULT_FROM_WIN32(err);
}

#endif // SUPPORTS_OWNERSHIP


STDAPI GetFolderComment(LPCWSTR pwszFolder, LPTSTR pszComment, int cchComment)
{
    TCHAR szFolder[MAX_PATH];

    *pszComment = 0;

    SHUnicodeToTChar(pwszFolder, szFolder, ARRAYSIZE(szFolder));

    GetShellClassInfoInfoTip(szFolder, pszComment, cchComment);

    return S_OK;
}

STDAPI GetFolderHTMLInfoTipFile(LPCWSTR pwszFolder, LPTSTR pszHTMLInfoTipFile, int cchHTMLInfoTipFile)
{
    TCHAR szFolder[MAX_PATH];

    *pszHTMLInfoTipFile = 0;

    SHUnicodeToTChar(pwszFolder, szFolder, ARRAYSIZE(szFolder));

    GetShellClassInfoHTMLInfoTipFile(szFolder, pszHTMLInfoTipFile, cchHTMLInfoTipFile);

    return S_OK;
}

HRESULT FileTimeToVariant(const FILETIME *pft, VARIANT *pv)
{
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft, &ftLocal);

    //
    //  Watch out for the special filesystem "uninitialized" values.
    //
    if (FILETIMEtoInt64(*pft)    == FT_NTFS_UNKNOWNGMT ||
        FILETIMEtoInt64(ftLocal) == FT_FAT_UNKNOWNLOCAL)
        return E_FAIL;

    FileTimeToSystemTime(pft, &st);
    pv->vt = VT_DATE;
    return SUCCEEDED(SystemTimeToVariantTime(&st, &pv->date)) ? S_OK : E_FAIL; // delay load...
}

STDMETHODIMP CFileSysColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    TCHAR szStr[128];
    HRESULT hr = S_FALSE;
    USES_CONVERSION;

    // If we are asking for a different filename then we lookup the data using a FindFirstFile.  If
    // the filename is the same then we simply use our cached data.  The exception to this is if the
    // SHCDF_UPDATEITEM flag is passed.  This flag tell us we need to update our cache.
    if ((pscd->dwFlags & SHCDF_UPDATEITEM) || (StrCmpW(_wszLastFile, pscd->wszFile) != 0))
    {
        HANDLE hfind = FindFirstFile(W2CT(pscd->wszFile), &_fd);
        if (hfind != INVALID_HANDLE_VALUE)
            FindClose(hfind);

        StrCpyNW(_wszLastFile, pscd->wszFile, ARRAYSIZE(_wszLastFile));
    }

    if (IsEqualSCID(SCID_Comment, *pscid) ||
            IsEqualSCID(SCID_HTMLINFOTIPFILE, *pscid))
    {
        if (pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (pscd->dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
            {
                if (IsEqualSCID(SCID_Comment, *pscid))
                {
                    GetFolderComment(pscd->wszFile, szStr, ARRAYSIZE(szStr));
                }
                else
                {
                    GetFolderHTMLInfoTipFile(pscd->wszFile, szStr, ARRAYSIZE(szStr));
                }
                hr = InitVariantFromStr(pvarData, szStr);
            }
        }
    }
    else if (IsEqualSCID(SCID_CREATETIME, *pscid) || 
             IsEqualSCID(SCID_ACCESSTIME, *pscid))
    {
        hr = FileTimeToVariant(pscid->pid == PID_STG_ACCESSTIME ? 
            &_fd.ftLastAccessTime : &_fd.ftCreationTime, pvarData);
    }

#ifdef SUPPORTS_OWNERSHIP
    else if (IsEqualSCID(SCID_OWNER, *pscid))
    {
        hr = _LookupOwnerName(W2CT(pscd->wszFile), pvarData);
    }
#endif // SUPPORTS_OWNERSHIP

    return hr;
}

STDAPI CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CFileSysColumnProvider *pfcp = new CFileSysColumnProvider;
    if (pfcp)
    {
        hr = pfcp->QueryInterface(riid, ppv);
        pfcp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
