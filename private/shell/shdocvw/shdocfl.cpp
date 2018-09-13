#include "priv.h"
#include "sccls.h"
#include "dochost.h"
#include "resource.h"
#include "stdenum.h"
#include <shlguidp.h>
#include <idhidden.h>
#include "shdocfl.h"
#include <vdate.h>

#include <mluisupp.h>

#ifdef UNIX
#include "unixstuff.h"
#endif

HRESULT CDocObjectView_Create(IShellView** ppv, IShellFolder* psf, LPCITEMIDLIST pidl);
#ifdef LIGHT_FRAMES
STDAPI _URLMONMonikerFromPidl(LPCITEMIDLIST pidl, IMoniker** ppmk, BOOL* pfFileProtocol);
#endif


#define DM_STARTUP          0
#define DM_CDOFPDN          0       // CDocObjectFolder::ParseDisplayName

class CDocObjectFolder :    public IShellFolder2, 
                            public IPersistFolder,
                            public IBrowserFrameOptions
{
public:
    CDocObjectFolder(LPCITEMIDLIST pidlRoot = NULL);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);

    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                           DWORD uFlags, LPITEMIDLIST * ppidlOut);
    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID pGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails){ return E_NOTIMPL; };
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid) { return E_NOTIMPL; };

    // IPersistFolder
    STDMETHODIMP GetClassID(LPCLSID pClassID);
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IBrowserFrameOptions
    STDMETHODIMP GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions);

protected:

    ~CDocObjectFolder();

    LONG            _cRef;
    LPITEMIDLIST    _pidlRoot;
};

//========================================================================
// CDocObjectFolder members
//========================================================================

CDocObjectFolder::CDocObjectFolder(LPCITEMIDLIST pidlRoot)
        : _cRef(1), _pidlRoot(NULL)
{
    TraceMsg(TF_SHDLIFE, "ctor CDocObjectFolder %x", this);

    DllAddRef();

    if (pidlRoot)
        _pidlRoot = ILClone(pidlRoot);
}

CDocObjectFolder::~CDocObjectFolder()
{
    TraceMsg(TF_SHDLIFE, "dtor CDocObjectFolder %x", this);

    if (_pidlRoot)
        ILFree(_pidlRoot);

    DllRelease();
}

HRESULT CDocObjectFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CDocObjectFolder, IShellFolder, IShellFolder2),
        QITABENT(CDocObjectFolder, IShellFolder2),
        QITABENT(CDocObjectFolder, IPersistFolder), 
        QITABENT(CDocObjectFolder, IBrowserFrameOptions), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDocObjectFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDocObjectFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDocObjectFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pwszDisplayName,
        ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    *ppidl = NULL;
    return E_UNEXPECTED;
}
HRESULT CDocObjectFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    *ppenumIDList = NULL;
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    *ppvOut = NULL;
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    *ppvObj = NULL;
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppvOut)
{
    HRESULT hres = E_FAIL;

    if (IsEqualIID(riid, IID_IShellView))
    {
#ifdef LIGHT_FRAMES
        hres = CoCreateInstance(CLSID_HTMLDocument,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            riid,
                            (void**)ppvOut);
#else
        hres = CDocObjectView_Create((IShellView**)ppvOut, this, _pidlRoot);
#endif

#ifdef LIGHT_FRAMES
        if (SUCCEEDED(hres))
        {
            IMoniker * pmk = NULL;
            BOOL fFileProtocol;
    
            if (SUCCEEDED(_URLMONMonikerFromPidl(_pidlRoot, &pmk, &fFileProtocol)))
            {
                IPersistMoniker * pPersistMoniker;
                if (SUCCEEDED( ((IUnknown*)(*ppvOut))->QueryInterface(IID_IPersistMoniker, (void**)&pPersistMoniker)))
                {
                    pPersistMoniker->Load(FALSE, pmk, NULL, 0);
                    pPersistMoniker->Release();
                }
                pmk->Release();
            }
        }
#endif
    }
    else
    {
        hres = E_NOINTERFACE;
        *ppvOut = NULL;
    }
    return hres;
}

HRESULT CDocObjectFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                 REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    *ppvOut = NULL;
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    //  we should never have any children.
    ASSERT(cidl == 0);
    if (cidl != 0)
        return E_UNEXPECTED;
        
    if (*rgfInOut)
    {
        //  they want to know about the document itself
        ASSERT(_pidlRoot);
        return SHGetAttributesOf(_pidlRoot, rgfInOut);
    }

    return S_OK;
}

HRESULT CDocObjectFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName)
{
    AssertMsg(FALSE, TEXT("CDocObjFolder - Called Improperly - ZekeL"));
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                           DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    return E_UNEXPECTED;
}

HRESULT CDocObjectFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    *pGuid = SRCID_SWebSearch;
    return S_OK;
}

HRESULT CDocObjectFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

HRESULT CDocObjectFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_CDocObjectFolder;
    return S_OK;
}

HRESULT CDocObjectFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (_pidlRoot) 
    {
        ILFree(_pidlRoot);
        _pidlRoot = NULL;
    }

    if (pidl)
        _pidlRoot = ILClone(pidl);

    return S_OK;
}


// IBrowserFrameOptions
#define BASE_OPTIONS \
                            (BFO_BROWSER_PERSIST_SETTINGS | BFO_RENAME_FOLDER_OPTIONS_TOINTERNET | \
                            BFO_PREFER_IEPROCESS | BFO_ENABLE_HYPERLINK_TRACKING | \
                            BFO_USE_IE_LOGOBANDING | BFO_ADD_IE_TOCAPTIONBAR | BFO_GO_HOME_PAGE | \
                            BFO_USE_IE_TOOLBAR | BFO_NO_PARENT_FOLDER_SUPPORT | BFO_NO_REOPEN_NEXT_RESTART)


// IBrowserFrameOptions
HRESULT CDocObjectFolder::GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions)
{
    // We are hosting a DocObj?
    BOOL fIsFileURL = FALSE;

    // Is this under the Internet Name Space? Yes for
    // HTTP and FTP owned by the IE name space.  MSIEFTP
    // pidls are passed straight to that folder.
    // This function will return FALSE for non-IE stuff
    // but we will then want to check if it's a file system
    // thing that wants to act like a web page because it's
    // MIME TYPE or other association is associated with the web.
    if (!IsURLChild(_pidlRoot, TRUE))
    {               
        // Since IsURLChild() returned FALSE, this must be in the file system.
        // This case will happen with:
        // C:\foo.htm
        // http://www.yahoo.com/
        // http://bryanst/resume.doc
        // http://bryanst/notes.txt
        // <Start Page>  [I couldn't find a case that hit CInternetFolder]
        // C:\foo.doc (use the addressbar to repro)
        fIsFileURL = TRUE;
    }

    *pdwOptions = dwMask & BASE_OPTIONS;
    if (!fIsFileURL)
    {
        // Add the Offline Support when we aren't in the file system.
        *pdwOptions |= dwMask & (BFO_USE_IE_OFFLINE_SUPPORT | BFO_USE_DIALUP_REF);
    }
        
    return S_OK;
}


STDAPI CDocObjectFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CDocObjectFolder *psf = new CDocObjectFolder;
    if (psf)
    {
        *ppunk = SAFECAST(psf, IShellFolder *);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}


class CInternetFolder : CDocObjectFolder
{
public:
    CInternetFolder(LPCITEMIDLIST pidlRoot = NULL) ;

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                 REFIID riid, UINT *prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                           DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IPersistFolder
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IBrowserFrameOptions
    STDMETHODIMP GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions);

protected:
    ~CInternetFolder();

    HRESULT _CreateProtocolHandler(LPCSTR pszProtocol, IBindCtx * pbc, IShellFolder **ppsf);
    HRESULT _CreateProtocolHandlerFromPidl(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsf);
    HRESULT _GetAttributesOfProtocol(LPCSTR pszProtocol, LPCITEMIDLIST *apidl, UINT cpidl, ULONG *rgfInOut);
    HRESULT _FaultInUrlHandler(LPCSTR pszProtocol, LPCTSTR pszUrl, IUnknown * punkSite);
    HRESULT _ConditionallyFaultInUrlHandler(LPCSTR pszProtocol, LPCTSTR pszUrl, IBindCtx * pbc);
    HRESULT _AssocCreate(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv);
    HRESULT _GetScheme(LPCITEMIDLIST pidl, LPWSTR pszOut, DWORD cchOut);
    HRESULT _GetUIObjectFromShortcut(LPCITEMIDLIST pidl, REFIID riid, void **ppvOut);
    HRESULT _GetTitle(LPCWSTR pszUrl, STRRET *pstr);
    HRESULT _InitHistoryStg(IUrlHistoryStg **pphist);

    IUrlHistoryStg *_phist;
};


CInternetFolder::CInternetFolder(LPCITEMIDLIST pidlRoot)
    : CDocObjectFolder(pidlRoot)
{
    TraceMsg(TF_URLNAMESPACE, "[%X] ctor CInternetFolder", this);
}

CInternetFolder::~CInternetFolder()
{
    ATOMICRELEASE(_phist);
    TraceMsg(TF_URLNAMESPACE, "[%X] dtor CInternetFolder", this);
}

HRESULT CInternetFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CInternetFolder, IShellFolder, IShellFolder2),
        QITABENT(CInternetFolder, IShellFolder2),
        QITABENT(CInternetFolder, IPersistFolder), 
        QITABENT(CInternetFolder, IBrowserFrameOptions), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CInternetFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CInternetFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


typedef struct tagURLID 
{
    ITEMIDLIST idl;     //  cb and SHID
    BYTE bType;         //  URLID
    UINT uiCP;          //  Code Page
    WCHAR achUrl[1];       //  variable size string
} URLID;

#define SHID_INTERNET           0x60
#define SHID_INTERNET_SITE      0x61    // IE name space item

#define URLID_URLBASEA          0x00
/////// URLID_LOCATION          0x01  //  LEGACY IE3/4 used for Frag IDs
/////// URLID_FTPFOLDER         0x02  //  LEGACY used by a pre-release FTP Folder dll
#define URLID_PROTOCOL          0x03  //  this is actually a delegated protocol
#define URLID_URLBASEW          0x80  //  
//      URLIDF_UNICODE          0x80  //  URLID_ is actually of UNICODE type

#ifdef UNICODE 
#define URLID_URLBASE           URLID_URLBASEW
#else
#define URLID_URLBASE           URLID_URLBASEA
#endif

typedef const UNALIGNED URLID *PCURLID;
typedef UNALIGNED URLID *PURLID;

#define PDID_SIG MAKEWORD(SHID_INTERNET_SITE, URLID_PROTOCOL)

inline PCDELEGATEITEMID _IsValidDelegateID(LPCITEMIDLIST pidl)
{
    PCDELEGATEITEMID pdi = (PCDELEGATEITEMID)pidl;
    ASSERT(pdi);

    if ((pdi->cbSize >= (SIZEOF(PDELEGATEITEMID)-1))
    && (pdi->wOuter == PDID_SIG))
        return pdi;

    return NULL;
}
    
LPCSTR _PidlToDelegateProtocol(LPCITEMIDLIST pidl)
{
    PCDELEGATEITEMID pdi = _IsValidDelegateID(pidl);
    if (pdi)
        return (LPCSTR)&(pdi->rgb[pdi->cbInner]);

    return NULL;
}

inline PCURLID _IsValidUrlID(LPCITEMIDLIST pidl)
{
    PCURLID purlid = (PCURLID)pidl;
    ASSERT(purlid);

//  98/12/22 #263932 vtan: ANSI and Unicode URLs are both valid. Use function
//  _ExtractURL to extract the URL from the PIDL as a Unicode string.

    if (purlid->idl.mkid.cb >= SIZEOF(URLID)
    && (purlid->idl.mkid.abID[0] == SHID_INTERNET_SITE)
    && (purlid->bType == URLID_URLBASEA || purlid->bType == URLID_URLBASEW || _IsValidDelegateID(pidl)))
        return purlid;

    return NULL;
}

//  98/12/22 #263932 vtan: IE4 stores the PIDL in a stream as an ANSI
//  string. IE5 stores the PIDL in a stream as a Unicode string. This
//  functions reads the string (ANSI or Unicode) and converts it to
//  an internal Unicode string which is what will be written to the stream.

void _ExtractURL (PCURLID pURLID, LPWSTR wszURL, int iCharCount)
{
    if (pURLID->bType == URLID_URLBASEA)
    {
        char aszURL[MAX_URL_STRING];

#ifdef UNIX
        ualstrcpynA(aszURL, (const char*)(pURLID->achUrl), sizeof(aszURL));
#else
        ualstrcpynA(aszURL, reinterpret_cast<const char*>(pURLID->achUrl), sizeof(aszURL));
#endif
        SHAnsiToUnicode(aszURL, wszURL, iCharCount);
    }
    else if (pURLID->bType == URLID_URLBASEW)
    {
        ualstrcpynW(wszURL, pURLID->achUrl, iCharCount);
    }
}

//  99/01/04 vtan: Added the following to help compare URLIDs which
//  can be AA/UU/AU/UA and perform the correct comparison.

int _CompareURL (PCURLID pURLID1, PCURLID pURLID2)
{
    int iResult;
    
    if ((pURLID1->bType == URLID_URLBASEA) && (pURLID2->bType == URLID_URLBASEA))
    {
#ifdef UNIX
        iResult = ualstrcmpA((const char*)(pURLID1->achUrl), (const char*)(pURLID2->achUrl));
#else
        iResult = ualstrcmpA(reinterpret_cast<const char*>(pURLID1->achUrl), reinterpret_cast<const char*>(pURLID2->achUrl));
#endif
    }
    else if ((pURLID1->bType == URLID_URLBASEW) && (pURLID2->bType == URLID_URLBASEW))
    {
        iResult = ualstrcmpW(pURLID1->achUrl, pURLID2->achUrl);
    }
    else
    {
        PCURLID pCompareURLID;
        WCHAR wszURL[MAX_URL_STRING];
        
        //  AU/UA comparison. To be efficient only convert the ANSI URLID
        //  to Unicode and perform the comparison in Unicode.
        
        if (pURLID1->bType == URLID_URLBASEA)
        {
            pCompareURLID = pURLID2;
            _ExtractURL(pURLID1, wszURL, SIZECHARS(wszURL));
        }
        else
        {
            pCompareURLID = pURLID1;
            _ExtractURL(pURLID2, wszURL, SIZECHARS(wszURL));
        }
        iResult = ualstrcmpW(pCompareURLID->achUrl, wszURL);
    }
    return iResult;
}

IShellFolder* g_psfInternet = NULL;

STDAPI CDelegateMalloc_Create(void *pv, UINT cbSize, WORD wOuter, IMalloc **ppmalloc);
//
// this might modify pszName if it's not a fully qualified url!
BOOL _ValidateURL(LPTSTR pszName, DWORD dwFlags)
{
    //
    // WARNING: In order to allow URL extensions, we assume all strings
    //  which contains ":" in it is a valid string.
    // Assumptions are:
    //
    // (1) CDesktop::ParseDisplayName parse file system strings first.
    // (2) URL moniker will return an error correctly if URL is not valid.
    //
    return SUCCEEDED(IURLQualify(pszName, dwFlags, pszName, NULL, NULL)) && (-1 != GetUrlScheme(pszName));
}

LPITEMIDLIST IEILAppendFragment(LPITEMIDLIST pidl, LPCWSTR pszFragment)
{
    // BUGBUG: See IE5 bug #'s 86951 and 36497 for more details.
    //         In a nutshell, we're rolling back the change for 36497 because
    //         the change caused many more problems with customers than
    //         the behavior we had in IE4.
    // 
    //         Because we're not ensuring that
    //         the fragment isn't prefixed with a '#', there may be
    //         cases where the URL in the address bar looks wrong,
    //         as well as cases where a hyperlink to a different doc
    //         or HTML page may fail if it contains a fragment.
    return ILAppendHiddenStringW(pidl, IDLHID_URLFRAGMENT, pszFragment);
}

// browser only uglyness... we need to construct a desktop relative "regitem" pidl for
// the internet since browser only shell does not support psf->ParseDisplayName("::{guid}", &pidl)
// this uses the same layout as REGITEMs so we have PIDL compatibility with integrated mode
// this ensures that a shortcut to the IE icon made in browser only mode works in integrated

#ifndef NOPRAGMAS
#pragma pack(1)
#endif
typedef struct
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bReserved;  // This is to get DWORD alignment
    CLSID   clsid;
} IDITEM;               // IDREGITEM

typedef struct
{
    IDITEM idri;
    USHORT cbNext;
} IDLITEM;              // IDLREGITEM
#ifndef NOPRAGMAS
#pragma pack()
#endif

// stolen from shell32\shitemid.h

#define SHID_ROOT_REGITEM       0x1f    // MyDocuments, Internet, etc

const IDLITEM c_idlInetRoot = 
{ 
    {SIZEOF(IDITEM), SHID_ROOT_REGITEM, 0, 
    { 0x871C5380, 0x42A0, 0x1069, 0xA2,0xEA,0x08,0x00,0x2B,0x30,0x30,0x9D },/* CLSID_Internet */ }, 0,
};

LPCITEMIDLIST c_pidlURLRoot = (LPCITEMIDLIST)&c_idlInetRoot;

// it must be an absolute pidl with a root regitem id at the front
// if we're a rooted explorer, this is always false
// this means we're definitely in nashville, so we shouldn't have a split
// world

PCURLID _FindUrlChild(LPCITEMIDLIST pidl, BOOL fIncludeHome = FALSE)
{
    if ((pidl == NULL) ||
        (pidl->mkid.cb != sizeof(IDITEM)) ||
        (pidl->mkid.abID[0] != SHID_ROOT_REGITEM))
    {
        return NULL;
    }

    //
    // the clsid in the pidl must be our internet folder's
    //
    if (!IsEqualGUID(((IDITEM*)pidl)->clsid, CLSID_Internet))
    {
        ASSERT(!IsEqualGUID(((IDITEM*)pidl)->clsid, CLSID_CURLFolder));
        return NULL;
    }

    //  go to the child...
    pidl = _ILNext(pidl);
    
    //
    // if it is a pidl to the internet root then it is the IE3 Home Page
    //
    
    if (fIncludeHome && ILIsEmpty(pidl))
        return (PCURLID)pidl;

    //
    // otherwise it is our child if it is a site object
    //
    return _IsValidUrlID(pidl);
}

STDAPI_(BOOL) IsURLChild(LPCITEMIDLIST pidl, BOOL fIncludeHome)
{
    return (NULL != _FindUrlChild(pidl, fIncludeHome));
}


BOOL IEILGetFragment(LPCITEMIDLIST pidl, LPWSTR pszFragment, DWORD cchFragment)
{
    return ILGetHiddenStringW(pidl, IDLHID_URLFRAGMENT, pszFragment, cchFragment);
}

UINT IEILGetCP(LPCITEMIDLIST pidl)
{
    PCURLID purlid = _FindUrlChild((pidl));
    if (purlid)
    {
        if (!_IsValidDelegateID((LPCITEMIDLIST)purlid))
            return purlid->uiCP;
    }
    return CP_ACP;
}

LPITEMIDLIST _UrlIdCreate(UINT uiCP, LPCTSTR pszUrl)
{
    //
    //  the URLID has a variable sized string
    //  member.  but we put the arbitrary limit
    //  of MAX_URL_STRING because that is what
    //  we use everywhere else.  we could just remove the
    //  limit however.
    //
    USHORT cb = (USHORT)SIZEOF(URLID) - (USHORT)CbFromCch(1);
    USHORT cchUrl = lstrlen(pszUrl) + 1;
    cchUrl = min(cchUrl, MAX_URL_STRING);
    cb += CbFromCch(cchUrl);

    PURLID purlid = (PURLID)IEILCreate(cb + SIZEOF(USHORT));

    if (purlid)
    {
        //  everything is actually aligned right now...
        purlid->idl.mkid.cb = cb;
        purlid->idl.mkid.abID[0] = SHID_INTERNET_SITE;
        purlid->bType = URLID_URLBASE;
        purlid->uiCP = uiCP;
#ifndef UNIX
        StrCpyN(purlid->achUrl, pszUrl, cchUrl);
#else
        TCHAR szTempUrlBuf[MAX_URL_STRING];
        StrCpyN(szTempUrlBuf, pszUrl, cchUrl);
        CopyMemory(purlid->achUrl, szTempUrlBuf, (lstrlen(szTempUrlBuf) + 1) * sizeof(TCHAR));
#endif
    }

    return (LPITEMIDLIST) purlid;
}
        
LPITEMIDLIST UrlToPidl(UINT uiCP, LPCTSTR pszUrl, LPCWSTR pszFragment)
{
    LPITEMIDLIST pidlRet;
    LPCTSTR pszAttachedFrag = UrlGetLocation(pszUrl);
    TCHAR szURLBuf[MAX_URL_STRING];

    //  deal with URL's that still include the location (as in ParseDisplayName)
    if (pszAttachedFrag) 
    {
        StrCpyN(szURLBuf, pszUrl, (int)(pszAttachedFrag-pszUrl+1));
        pszUrl = szURLBuf;

        //  prefer the passed in fragment to the attached one
        if (!pszFragment)
            pszFragment = pszAttachedFrag;
            
    }

    ASSERT(pszUrl);
    
    pidlRet = _UrlIdCreate(uiCP, pszUrl);

    if (pidlRet && pszFragment && *pszFragment)
        pidlRet = IEILAppendFragment(pidlRet, pszFragment);

    return pidlRet;
}

typedef struct
{
    LPCSTR pszProtocol;
    const CLSID * pCLSID;
} FAULTIN_URLHANDERS;

// TODO: If there are other URL Handlers, add them here.
const FAULTIN_URLHANDERS c_FaultInUrlHandlers[] =
{
    {"ftp", &CLSID_FTPShellExtension}
};

HRESULT CInternetFolder::_FaultInUrlHandler(LPCSTR pszProtocol, LPCTSTR pszUrl, IUnknown * punkSite)
{
    HRESULT hr = S_OK;
    if (pszProtocol)
    {
        for (int nIndex = 0; nIndex < ARRAYSIZE(c_FaultInUrlHandlers); nIndex++)
        {
            if (!StrCmpIA(pszProtocol, c_FaultInUrlHandlers[nIndex].pszProtocol))
            {
                // Only fault in the feature if we are navigating to an FTP directory.
                if ((0 == nIndex) && !UrlIs(pszUrl, URLIS_DIRECTORY))
                {
                    // It's not an ftp directory, so skip it.
                    continue;
                }

                // FTP has a URL Shell Extension handler that is optionally
                // installed.  Fault it in now if it's needed.
                uCLSSPEC ucs;
                QUERYCONTEXT qc = { 0 };
                HWND hwnd = NULL;

                ucs.tyspec = TYSPEC_CLSID;
                ucs.tagged_union.clsid = *c_FaultInUrlHandlers[nIndex].pCLSID;

                IUnknown_GetWindow(punkSite, &hwnd);
                if (EVAL(hwnd))
                {
                    // Make it modal while the dialog is being displayed.
                    IUnknown_EnableModless(punkSite, FALSE);
                    FaultInIEFeature(hwnd, &ucs, &qc, 0);
                    IUnknown_EnableModless(punkSite, TRUE);
                }
                break;    // pidl can only have 1 procotol, so we don't need to check the other protocol.
            }
        }
    }

    return hr;    // We don't care if it didn't make it.
}


HRESULT CInternetFolder::_ConditionallyFaultInUrlHandler(LPCSTR pszProtocol, LPCTSTR pszUrl, IBindCtx * pbc)
{
    HRESULT hr = S_OK;

    // Faulting in the feature will probably require UI, so we need to assure that the caller
    // will allow this.
    if (pbc)
    {
        IUnknown * punkSite = NULL;

        pbc->GetObjectParam(STR_DISPLAY_UI_DURING_BINDING, &punkSite);
        if (punkSite)
        {
            hr = _FaultInUrlHandler(pszProtocol, pszUrl, punkSite);
            punkSite->Release();
        }
    }

    ASSERT(SUCCEEDED(hr));
    return S_OK;    // We don't care if it didn't make it.
}


// returns:
//      success S_OK
//      failure FAILED(hres)

HRESULT CInternetFolder::_CreateProtocolHandler(LPCSTR pszProtocol, IBindCtx * pbc, IShellFolder **ppsf)
{
    HRESULT hres;
    CHAR szCLSID[GUIDSTR_MAX];
    DWORD cbSize = SIZEOF(szCLSID);

    *ppsf = NULL;

    if (pszProtocol && 
        SHGetValueA(HKEY_CLASSES_ROOT, pszProtocol, "ShellFolder", NULL, &szCLSID, &cbSize) == ERROR_SUCCESS)
    {
        CLSID clsid;
        IShellFolder *psf;

        GUIDFromStringA(szCLSID, &clsid);
        if (!SHSkipJunction(pbc, &clsid))
        {
            hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psf);
            if (SUCCEEDED(hres))
            {
                // IPersistFolder is optional
                IPersistFolder *ppf;
                if (SUCCEEDED(psf->QueryInterface(IID_IPersistFolder, (void **)&ppf)))
                {
                    ppf->Initialize(_pidlRoot);
                    ppf->Release();
                }

                IDelegateFolder *pdf;
                hres = psf->QueryInterface(IID_IDelegateFolder, (void **)&pdf);
                if (SUCCEEDED(hres))
                {
                    // REVIEW: we could cache the malloc on a per protocol basis
                    // to avoid creating these over and over
                    IMalloc *pmalloc;
                    hres = CDelegateMalloc_Create((void*)pszProtocol, (lstrlenA(pszProtocol) + 1), PDID_SIG, &pmalloc);
                    if (SUCCEEDED(hres))
                    {
                        hres = pdf->SetItemAlloc(pmalloc);
                        pmalloc->Release();
                    }
                    pdf->Release();
                }

                if (SUCCEEDED(hres))
                {
                    hres = S_OK;    // force all success codes to S_OK 
                    *ppsf = psf;
                }
                else
                    psf->Release();
            }
        }
        else
            hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    else
        hres = E_FAIL;

    return hres;
}

// returns:
//      S_FALSE if it is not a delegate protocol PIDL
//      hres of the bind opteration to the delegate protocol handler

HRESULT CInternetFolder::_CreateProtocolHandlerFromPidl(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsf)
{
    LPCSTR pszProtocol = _PidlToDelegateProtocol(pidl);
    if (pszProtocol)
    {
        HRESULT hres = _CreateProtocolHandler(pszProtocol, pbc, ppsf);
        ASSERT(hres != S_FALSE);    // enforce the return value comment
        return hres;
    }

    *ppsf = NULL;
    return S_FALSE;     // not a protocal PIDL
}

BOOL _GetUrlProtocol(LPCTSTR pszUrl, LPSTR pszProtocol, DWORD cchProtocol)
{
    TCHAR sz[MAX_PATH];
    DWORD cch = SIZECHARS(sz);
    if (SUCCEEDED(UrlGetPart(pszUrl, sz, &cch, URL_PART_SCHEME, 0)))
    {
        SHTCharToAnsi(sz, pszProtocol, cchProtocol);
        return TRUE;
    }

    return FALSE;
}

UINT CodePageFromBindCtx(LPBC pbc)
{
    UINT uiCP = CP_ACP;
    IDwnCodePage *pDwnCP;
    if (pbc && SUCCEEDED(pbc->QueryInterface(IID_IDwnCodePage, (void **)&pDwnCP)))
    {
        uiCP = pDwnCP->GetCodePage();
        pDwnCP->Release();
    }
    return uiCP;
}


BOOL _IsKeyInBindCtx(IBindCtx * pbc, LPCWSTR pwzKey)
{
    BOOL fResult = FALSE;
    IUnknown * punk;
    HRESULT hr = pbc->GetObjectParam((LPWSTR)pwzKey, &punk);
    if (SUCCEEDED(hr))    // Some browsers include the code page in the BindContext.
    {
        fResult = TRUE;
        punk->Release();        // We don't use it now.
    }
    return fResult;
}

HRESULT CInternetFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pwszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    HRESULT hres = E_FAIL;

    TCHAR szName[MAX_URL_STRING];
    UINT uiCP = CodePageFromBindCtx(pbc);

    StrCpyN(szName, pwszDisplayName, ARRAYSIZE(szName));
    if (!PathIsFilePath(szName))
    {
        if (_ValidateURL(szName, 0) || ShouldShellExecURL(szName))
        {
            CHAR szProtocol[MAX_PATH];
            DWORD cchName = ARRAYSIZE(szName);
            IShellFolder *psfHandler;
            BOOL fProtocolExists;

            // if we're down here, then the szName was really a url so try to encode it.
            // turn spaces to %20
            UrlEscape(szName, szName, &cchName, URL_ESCAPE_SPACES_ONLY);

            fProtocolExists = _GetUrlProtocol(szName, szProtocol, ARRAYSIZE(szProtocol));
            _ConditionallyFaultInUrlHandler(szProtocol, szName, pbc);

            if (fProtocolExists &&
                _CreateProtocolHandler(szProtocol, pbc, &psfHandler) == S_OK)
            {
                TraceMsg(TF_PIDLWRAP, "Asking \"%s\" handler to parse %s (%08X) into a pidl", szProtocol, szName, szName);
                hres = psfHandler->ParseDisplayName(hwnd, pbc,
                                                    pwszDisplayName, pchEaten,
                                                    ppidl, pdwAttributes);
                TraceMsg(TF_PIDLWRAP, "the result is %08X, the pidl is %08X", hres, *ppidl);
                psfHandler->Release();
                TraceMsg(TF_URLNAMESPACE, "CODF::PDN(%s) called psfHandler and returning %x",
                         szName, hres);
            }
            else
            {
                *ppidl = UrlToPidl(uiCP, szName, NULL);
                if (*ppidl)
                {
                    if (pdwAttributes)
                        hres = _GetAttributesOfProtocol(NULL, (LPCITEMIDLIST *)ppidl, 1, pdwAttributes);
                    else
                        hres = S_OK;
                }
                else 
                    hres = E_OUTOFMEMORY;

                TraceMsg(TF_URLNAMESPACE, "CODF::PDN(%s) called UrlToPidl and returning %x", szName, hres);
            }
        } 
        else 
        {
            TraceMsg(DM_CDOFPDN, "CDOF::PDN(%s) returning E_FAIL because of (%s) is FALSE", szName, TEXT("(_ValidateURL(szName) || ShouldShellExecURL( szName ))"));
        }
    } 

    return hres;
}

class CInternetFolderDummyEnum : public IEnumIDList
{
public:
    CInternetFolderDummyEnum();
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) {return E_NOTIMPL;}
    STDMETHODIMP Reset(void){return E_NOTIMPL;}
    STDMETHODIMP Clone(LPENUMIDLIST *ppenum){return E_NOTIMPL;}

protected:
    ~CInternetFolderDummyEnum() {;}
    
    long _cRef;
};

CInternetFolderDummyEnum::CInternetFolderDummyEnum() : _cRef(1)
{
}

HRESULT CInternetFolderDummyEnum::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CInternetFolderDummyEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CInternetFolderDummyEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CInternetFolderDummyEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CInternetFolderDummyEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    pceltFetched = 0;
    return S_FALSE;
}

HRESULT CInternetFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    CInternetFolderDummyEnum *pdummy = new CInternetFolderDummyEnum();

    if (pdummy)
    {
        *ppenumIDList = (IEnumIDList *)pdummy;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT CInternetFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    IShellFolder *psfHandler = NULL;
    BOOL fUseDefault = TRUE;
    *ppvOut = NULL;

    if (!_IsValidUrlID(pidl))
        return E_INVALIDARG;
        
    HRESULT hres = _CreateProtocolHandlerFromPidl(pidl, pbc, &psfHandler);
    if (hres == S_OK)
    {
        // NOTE: We allow Shell Extensions to take over URL handling on a per
        //     URL basis.  We entered the _CreateProtocolHandlerFromPidl()
        //     block of code above because
        //     a shell extension is registered to take over handling  this
        //     URL.  The above call to IShellFolder::BindToObject() just failed,
        //     so we need to fall back and handle it in the traditional way.
        //     This can be used by Shell Extensions, like the Ftp ShellExt, to
        //     let the browser (us) handle URLs that are either inaccessible because of
        //     the proxy or allow the browser to handle it so the traditional code
        //     will: 1) download the item(s), 2) sniff the data for the type, 3)
        //     use the suggested MIME type from the server or in the web page, 4)
        //     check the file for type extension mappings, 5)
        //     check any downloaded file for security certificates, and 6) display
        //     Open/Save dialogs.
            
        hres = psfHandler->BindToObject(pidl, pbc, riid, ppvOut);

        //  the handler will return ERROR_CANCELLED if it wants default behavior
        if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hres)
            fUseDefault = FALSE;
    }

    if (fUseDefault)
    {
        STRRET strRet;

        if (psfHandler)
        {
            //  we had a delegated folder that failed, need a normal pidl
            hres = psfHandler->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet);
        }
        else
            hres = GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet);

        TCHAR szUrl[MAX_URL_STRING];
        if (SUCCEEDED(hres) &&
            SUCCEEDED(hres = StrRetToBuf(&strRet, pidl, szUrl, ARRAYSIZE(szUrl))))
        {
            if (IsEqualIID(IID_IMoniker, riid))
            {
                hres = MonikerFromURL(szUrl, (IMoniker **)ppvOut);
            }
            else // DEFAULT
            {
                //  create a ShellFolder for the caller
                hres = E_OUTOFMEMORY;
                LPITEMIDLIST pidlT = NULL;

                //  if we are using a handler but it returned cancelled,
                //  then we need to recreate the pidl for ourselves to use
                //  otherwise we just use the one that was passed in, 
                //  which we assume was the one we created.
                if (psfHandler)
                {
                    pidlT = UrlToPidl(CP_ACP, szUrl, NULL);
                    pidl = pidlT;
                }

                if (pidl)
                {
                    LPITEMIDLIST pidlFull = ILCombine(_pidlRoot, pidl);

                    if (pidlFull)
                    {
                        CDocObjectFolder *psf = new CDocObjectFolder(pidlFull);
                        if (psf)
                        {
                            hres = psf->QueryInterface(riid, ppvOut);
                            psf->Release();
                        }
                        
                        ILFree(pidlFull);
                    }

                    ILFree(pidlT);
                }
            }
        }
    }

    
    if (psfHandler)
        psfHandler->Release();
        
    return hres;
}

HRESULT CInternetFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj)
{
    IShellFolder *psfHandler;

    *ppvObj = NULL;

    if (!_IsValidUrlID(pidl))
        return E_INVALIDARG;

    HRESULT hres = _CreateProtocolHandlerFromPidl(pidl, pbc, &psfHandler);
    if (hres != S_FALSE)
    {
        if (SUCCEEDED(hres))
        {
            hres = psfHandler->BindToStorage(pidl, pbc, riid, ppvObj);
            psfHandler->Release();
        }
        return hres;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

int CALLBACK CompareDelegateProtocols(void *pv1, void *pv2, LPARAM lParam)
{
    LPCSTR psz1 = _PidlToDelegateProtocol((LPCITEMIDLIST)pv1);
    LPCSTR psz2 = _PidlToDelegateProtocol((LPCITEMIDLIST)pv2);

    if (psz1 && psz2)
    {
        int iRet = StrCmpA(psz1, psz2);
        if (0 == iRet && lParam)
            *((LPCSTR *)lParam) = psz1;
    }
    else if (psz1)
    {
        return 1;
    }
    else if (psz2)
    {
        return -1;
    }
    return 0;
}


HRESULT CInternetFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet;

    ASSERT(!ILIsEmpty(pidl1) && !ILIsEmpty(pidl2));

    // Check for protocol pidls.
    LPCSTR psz = NULL;
    iRet = CompareDelegateProtocols((void *)pidl1, (void *)pidl2, (LPARAM)&psz);
    if (iRet)
        return ResultFromShort(iRet);

    if (psz)
    {
        IShellFolder *psfHandler;
        if (_CreateProtocolHandler(psz, NULL, &psfHandler) == S_OK)
        {
            iRet = psfHandler->CompareIDs(lParam, pidl1, pidl2);
            psfHandler->Release();
            return ResultFromShort(iRet);
        }
        
    }

    //  we only have one layer of children
    ASSERT(ILIsEmpty(_ILNext(pidl1)));
    ASSERT(ILIsEmpty(_ILNext(pidl2)));

    PCURLID purlid1 = _IsValidUrlID(pidl1);
    PCURLID purlid2 = _IsValidUrlID(pidl2);
    ASSERT(purlid1 && purlid2);
    
    iRet = _CompareURL(purlid1, purlid2);

    return ResultFromShort(iRet);
}


HRESULT CInternetFolder::_GetAttributesOfProtocol(LPCSTR pszProtocol,
                                                   LPCITEMIDLIST *apidl,
                                                   UINT cpidl, ULONG *rgfInOut)
{
    HRESULT hres;

    ASSERT(cpidl);
    
    if (pszProtocol)
    {
        //
        // We have a protocol.  Find the protocol handler
        // and pass it the bundle of pidls.
        //
        IShellFolder *psfHandler;
        hres = _CreateProtocolHandler(pszProtocol, NULL, &psfHandler);
        if (hres == S_OK)
        {
            hres = psfHandler->GetAttributesOf(cpidl, apidl, rgfInOut);
            psfHandler->Release();
        }
    }
    else if (_IsValidUrlID(apidl[0]))
    {
        ULONG uOut = SFGAO_CANLINK | SFGAO_BROWSABLE | SFGAO_CANMONIKER;
        *rgfInOut &= uOut;
        hres = S_OK;
        
    }
    else
        hres = E_INVALIDARG;

    return hres;
}


HRESULT CInternetFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    if (*rgfInOut)
    {
        //
        // Internet folder case.
        //
        LPCSTR pszProtocol;

        if (cidl == 0)
        {
            //
            // They are asking about the Internet Folder itself.
            //
            *rgfInOut &= SFGAO_FOLDER | SFGAO_CANLINK | SFGAO_CANMONIKER;
        }
        else if (cidl == 1)
        {
            //
            // Often we are asked about only one child,
            // so we optimize that case.
            //
            pszProtocol = _PidlToDelegateProtocol(apidl[0]);

            _GetAttributesOfProtocol(pszProtocol, apidl, cidl, rgfInOut);
        }
        else
        {
            //
            // They are asking about multiple internet children.
            // These children may have different protocols,
            // so we have to find the GetAttributesOf handler for
            // each group of protocols in the list.
            //
            LPCITEMIDLIST pidlBase;
            UINT i, cpidlGroup;

            // Create a list of pidls sorted by protocol.
            HDPA hdpa = DPA_Create(100);
            if (!hdpa)
                return E_OUTOFMEMORY;

            for (i = 0; i < cidl; i++)
            {
                DPA_AppendPtr(hdpa, (void *)apidl[i]);
            }
            DPA_Sort(hdpa, CompareDelegateProtocols, NULL);

            //
            // Call GetAttributesOf on each protocol group.
            // A group
            //   starts at pidlBase
            //   contains cpidlGroup pidls
            //   has a protocol of pszProtocol
            //
            pidlBase = (LPCITEMIDLIST)DPA_FastGetPtr(hdpa, 0);
            pszProtocol = NULL;
            cpidlGroup = 0;
            for (i = 0; *rgfInOut && (i < cidl); i++)
            {
                LPCITEMIDLIST pidlNew = (LPCITEMIDLIST)DPA_FastGetPtr(hdpa, i);
                LPCSTR pszProtocolNew = _PidlToDelegateProtocol(pidlNew);
                if (pszProtocolNew)
                {
                    // See if we have a new protocol.
                    if (!pszProtocol || StrCmpA(pszProtocol, pszProtocolNew))
                    {
                        // We have a new protocol, time to process
                        // the last batch pidls.
                        _GetAttributesOfProtocol(pszProtocol, &pidlBase, cpidlGroup, rgfInOut);

                        pidlBase = pidlNew;
                        pszProtocol = pszProtocolNew;
                        cpidlGroup = 0;
                    }
                }
                cpidlGroup++;
            }
            if (*rgfInOut)
            {
                ASSERT(cpidlGroup);
                _GetAttributesOfProtocol(pszProtocol, &pidlBase, cpidlGroup, rgfInOut);
            }

            DPA_Destroy(hdpa);
        }
    }

    return S_OK;
}

BOOL GetCommonProtocol(LPCITEMIDLIST *apidl, UINT cpidl, LPCSTR *ppszProtocol)
{
    UINT ipidl;
    LPCSTR pszProtocol;
    LPCSTR pszProtocolNext;

    *ppszProtocol = NULL;

    if (cpidl == 0)
    {
        return TRUE;    // No pidls - no protocols, but they do all match!
    }

    //
    // Grab the protocol of the first pidl, and use it to compare
    // against the rest of the pidls.
    //
    pszProtocol = _PidlToDelegateProtocol(apidl[0]);

    for (ipidl=1; ipidl<cpidl; ipidl++)
    {

        pszProtocolNext = _PidlToDelegateProtocol(apidl[ipidl]);

        //
        // Check if the protocols are different.
        //
        if ((pszProtocol != pszProtocolNext) &&
            ((pszProtocol == NULL) ||
             (pszProtocolNext == NULL) ||
             (StrCmpA(pszProtocol, pszProtocolNext) != 0)))
        {
            return FALSE;
        }
    }

    *ppszProtocol = pszProtocol;
    return TRUE;
}

HRESULT CInternetFolder::_GetUIObjectFromShortcut(LPCITEMIDLIST pidl, REFIID riid, void **ppvOut)
{
    HRESULT hres = E_NOINTERFACE;
    STRRET str;
    TCHAR sz[MAX_URL_STRING];

    if (SUCCEEDED(GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)) &&
        SUCCEEDED(StrRetToBuf(&str, pidl, sz, ARRAYSIZE(sz))))
    {
        IUniformResourceLocator *purl;
        hres = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                IID_IUniformResourceLocator, (void **)&purl);
        if (SUCCEEDED(hres))
        {
            hres = purl->SetURL(sz, 0);
            
            if (SUCCEEDED(hres))
            {
                IShellLink * psl;
                if (SUCCEEDED(purl->QueryInterface(IID_IShellLink, (void **)&psl)))
                {
                    if (SUCCEEDED(GetDisplayNameOf(pidl, SHGDN_INFOLDER, &str)) &&
                        SUCCEEDED(StrRetToBuf(&str, pidl, sz, ARRAYSIZE(sz))))
                    {
                        PathRenameExtension(sz, TEXT(".url"));
                        psl->SetDescription(sz);
                    }
                    psl->Release();
                }
                
                hres = purl->QueryInterface(riid, ppvOut);
            }
            purl->Release();
        }
    }       

    return hres;
}

HRESULT CInternetFolder::_GetScheme(LPCITEMIDLIST pidl, LPWSTR pszOut, DWORD cchOut)
{
    STRRET str;
    LPCSTR pszProtocol = _PidlToDelegateProtocol(pidl);

    if (pszProtocol)
    {
        SHAnsiToUnicode(pszProtocol, pszOut, cchOut);
        return S_OK;
    }
    else if (SUCCEEDED(GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
    {
        WCHAR sz[MAX_URL_STRING];
        if (SUCCEEDED(StrRetToBufW(&str, pidl, sz, ARRAYSIZE(sz))))
        {
            return UrlGetPartW(sz, pszOut, &cchOut, URL_PART_SCHEME, 0);
        }
    }
    return E_FAIL;
}
    
HRESULT CInternetFolder::_AssocCreate(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID*)&pqa);
    if (SUCCEEDED(hr))
    {
        WCHAR szScheme[MAX_PATH];
        _GetScheme(pidl, szScheme, SIZECHARS(szScheme));

        hr = pqa->Init(0, szScheme, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pqa->QueryInterface(riid, ppv);

        pqa->Release();
    }

    return hr;
}

HRESULT CInternetFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                 REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    HRESULT hres = E_NOINTERFACE;
    LPCSTR pszProtocol;
    
    *ppvOut = NULL;

    if (apidl[0] && GetCommonProtocol(apidl, cidl, &pszProtocol) && pszProtocol)
    {
        IShellFolder *psfHandler;
        hres = _CreateProtocolHandlerFromPidl(apidl[0], NULL, &psfHandler);
        if (hres != S_FALSE)
        {
            if (SUCCEEDED(hres))
            {
                hres = psfHandler->GetUIObjectOf(hwnd, 1, apidl, riid, prgfInOut, ppvOut);
                psfHandler->Release();
            }
            return hres;
        }
    }
    else if (IsEqualIID(riid, IID_IExtractIconA) 
         || IsEqualIID(riid, IID_IExtractIconW) 
         || IsEqualIID(riid, IID_IContextMenu)
         || IsEqualIID(riid, IID_IQueryInfo)
         || IsEqualIID(riid, IID_IDataObject))
    {
        //  BUGBUG - we only support this for one at a time.
        if (cidl == 1)
        {
            hres = _GetUIObjectFromShortcut(apidl[0], riid, ppvOut);
        }
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations))
    {
        //  BUGBUG - we only support this for one at a time.
        if (cidl == 1)
        {
            hres = _AssocCreate(apidl[0], riid, ppvOut);
        }
    }
        


    return hres;
}

HRESULT CInternetFolder::_InitHistoryStg(IUrlHistoryStg **pphist)
{
    HRESULT hr;
    if (!_phist)
    {
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER,
                IID_IUrlHistoryStg, (void **)&_phist);
    }
    
    if (_phist)
    {
        *pphist = _phist;
        _phist->AddRef();
        return S_OK;          
    }

    return hr;
}

HRESULT CInternetFolder::_GetTitle(LPCWSTR pszUrl, STRRET *pstr)
{
    ASSERT(pszUrl);

    IUrlHistoryStg *phist;

    HRESULT hr = _InitHistoryStg(&phist);

    if (SUCCEEDED(hr))
    {
        ASSERT(phist);
        STATURL stat = {0};
        hr = phist->QueryUrl(pszUrl, STATURL_QUERYFLAG_NOURL, &stat);

        if (SUCCEEDED(hr) && stat.pwcsTitle)
        {
            hr = StringToStrRet(stat.pwcsTitle, pstr); 
            CoTaskMemFree(stat.pwcsTitle);
        }
        else
            hr = E_FAIL;

        phist->Release();
    }

    return hr;
}

HRESULT CInternetFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pstr)
{
    IShellFolder *psfHandler;
    HRESULT hr = _CreateProtocolHandlerFromPidl(pidl, NULL, &psfHandler);
    if (hr != S_FALSE)
    {
        if (SUCCEEDED(hr))
        {
            hr = psfHandler->GetDisplayNameOf(pidl, uFlags, pstr);
            psfHandler->Release();
        }
        return hr;
    }

    // BUGBUGZEKEL - should i handle more SHGDN flags here?? - Zekel - 24-NOV-98
    PCURLID purlid = _IsValidUrlID(pidl);
    if (purlid)
    {
        WCHAR sz[MAX_URL_STRING];

        _ExtractURL(purlid, sz, SIZECHARS(sz));

        if (SHGDN_NORMAL != uFlags)
            hr = StringToStrRet(sz, pstr); 
        else
        {
            hr = _GetTitle(sz, pstr);

            //  fallback to the URL if necessary
            if (FAILED(hr))
                hr = StringToStrRet(sz, pstr); 
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

HRESULT CInternetFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
                                 LPCOLESTR pszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut)
{
    IShellFolder *psfHandler;
    HRESULT hres = _CreateProtocolHandlerFromPidl(pidl, NULL, &psfHandler);
    if (hres != S_FALSE)
    {
        if (SUCCEEDED(hres))
        {
            hres = psfHandler->SetNameOf(hwnd, pidl, pszName, uFlags, ppidlOut);
            psfHandler->Release();
        }
        return hres;
    }

    return E_FAIL;
}


HRESULT CInternetFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_Internet;
    return S_OK;
}


// IBrowserFrameOptions
HRESULT CInternetFolder::GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions)
{
    // The only case I know of that we hit this code is when you select "Internet Explorer" in the
    // Folder Browser Band.
    HRESULT hr = E_INVALIDARG;

    if (pdwOptions)
    {
        // CInternetFolder should only be used for the "Internet Explorer" pidl that
        // points to the Start Page, so find the start page and substitute it during
        // navigation.
        *pdwOptions |= dwMask & (BFO_SUBSTITUE_INTERNET_START_PAGE | BASE_OPTIONS);
        hr = S_OK;
    }

    return hr;
}



#ifdef DEBUG
extern void remove_from_memlist(void *pv);
#endif

STDAPI CInternetFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CInternetFolder *psf = new CInternetFolder;
    if (psf)
    {
#ifdef DEBUG
        //
        // HACK:
        //
        //   SHELL32 caches c_sfInetRoot in a static DATA section
        //  and never release it. It caches an instance of CInternetFolder
        //  and never release it. Therefore, we are removing this object
        //  from the to-be-memleak-detected list to avoid a false alarm
        //  assuming that we don't realy leak this object.
        //   Please don't copy it to another place unless you are really
        //  sure that it's OK not to detect leaks in that scenario.
        //  (SatoNa)
        //
        remove_from_memlist(psf);
#endif
        HRESULT hr = psf->QueryInterface(IID_IUnknown, (void **)ppunk);
        psf->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}
    

STDAPI MonikerFromURL(LPCWSTR wszPath, IMoniker** ppmk)
{
    HRESULT hres = CreateURLMoniker(NULL, wszPath, ppmk);
#ifndef UNIX
    if (FAILED(hres)) 
#else
    // BUG BUG : 
    // IEUNIX  : We use to crash on UNIX if we give a very long invalid url
    // in address bar  or  as home page in inetcpl. We used to crash inside 
    // MkParseDisplayName.
    if (FAILED(hres) && lstrlenW(wszPath) < MAX_PATH) 
#endif
    {
        IBindCtx* pbc;
        hres = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hres)) 
        {
            // Fall back to a system (file) moniker
            ULONG cchEaten = 0;
            hres = MkParseDisplayName(pbc, wszPath, &cchEaten, ppmk);
            pbc->Release();
        }
    }

    return hres;
}

STDAPI MonikerFromString(LPCTSTR szPath, IMoniker** ppmk)
{
    return MonikerFromURL(szPath, ppmk);
}

#if 0
// alternate way to do the below in a more "pure" way on integrated mode

HRESULT InitPSFInternet()
{
    if (g_psfInternet)
        return S_OK;

    IShellFolder *psfTemp;
    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        ULONG cchEaten;
        LPITEMIDLIST pidl;
        WCHAR wszInternet[] = L"::{871C5380-42A0-1069-A2EA-08002B30309D}"; // CLSID_Internet

        hres = psfDesktop->ParseDisplayName(NULL, NULL, wszInternet, &cchEaten, &pidl, NULL);
        if (SUCCEEDED(hres))
        {
            hres = psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (void **)&psfTemp);
            SHFree(pidl);
        }
        psfDesktop->Release();
    }

    if (SHInterlockedCompareExchange((void **)&g_psfInternet, psfTemp, 0))
        psfTemp->Release();     // race to the exchange, free dup copy

    return hres;
}
#endif


HRESULT InitPSFInternet()
{
    if (g_psfInternet)
        return S_OK;

    IShellFolder *psfTemp;
    HRESULT hres = CoCreateInstance(CLSID_CURLFolder, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psfTemp);
    if (SUCCEEDED(hres)) 
    {
        IPersistFolder* ppsf;
        hres = psfTemp->QueryInterface(IID_IPersistFolder, (void **)&ppsf);
        if (SUCCEEDED(hres)) 
        {
            hres = ppsf->Initialize(c_pidlURLRoot);
            if (SUCCEEDED(hres))
            {
                if (SHInterlockedCompareExchange((void **)&g_psfInternet, psfTemp, 0) == 0)
                    psfTemp->AddRef();  // global now holds ref
            }
            ppsf->Release();
        }
        psfTemp->Release();
    }
    return hres;
}

HRESULT _GetInternetRoot(IShellFolder **ppsfRoot)
{
    HRESULT hr = InitPSFInternet();
    *ppsfRoot = NULL;

    if (SUCCEEDED(hr))
    {
        g_psfInternet->AddRef();
        *ppsfRoot = g_psfInternet;
    }
    return hr;
}

HRESULT _GetRoot(LPCITEMIDLIST pidl, BOOL fIsUrl, IShellFolder **ppsfRoot)
{
    HRESULT hr;
    *ppsfRoot = NULL;
    
    if (fIsUrl)
    {
        ASSERT(IsURLChild(pidl, TRUE));
        TraceMsg(TF_URLNAMESPACE, "IEBTO(%x) using the Internet", pidl);
        hr = _GetInternetRoot(ppsfRoot);
    }
    else
    {
        ASSERT(ILIsRooted(pidl));
        TraceMsg(TF_URLNAMESPACE, "IEBTO(%x) using Rooted", pidl);

        CLSID clsid;

        ILRootedGetClsid(pidl, &clsid);

        if (IsEqualGUID(clsid, CLSID_ShellDesktop))
        {
            hr = SHBindToObject(NULL, IID_IShellFolder, ILRootedFindIDList(pidl), (void **)ppsfRoot);
        }
        else
        {
            IShellFolder *psf;
            hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_IShellFolder, (void **)&psf);
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlRoot = ILRootedFindIDList(pidl);
                if (!pidlRoot)
                    pidlRoot = &s_idlNULL;

                IPersistFolder* ppf;
                hr = psf->QueryInterface(IID_IPersistFolder, (void **)&ppf);
                if (SUCCEEDED(hr))
                {
                    hr = ppf->Initialize(pidlRoot);
                    ppf->Release();
                }

                //  hand over the reference
                *ppsfRoot = psf;
            }
        }
    }

    return hr;
}


STDAPI_(BOOL) IEILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fIgnoreHidden)
{
    UINT cb = ILGetSize(pidl1);

    if (cb != ILGetSize(pidl2) || 0 != memcmp(pidl1, pidl2, cb))
    {
        //  THEY are binarily different
        BOOL fRet = FALSE;
        BOOL fWebOnly = FALSE;

        if (IsURLChild(pidl1, TRUE) || IsURLChild(pidl2, TRUE))
            fWebOnly = TRUE;
       
        if ((IsURLChild(pidl1, FALSE) && IsURLChild(pidl2, FALSE))
        || (ILIsRooted(pidl1) && ILIsEqualRoot(pidl1, pidl2)))
        {
            IShellFolder *psf;
            if (SUCCEEDED(_GetRoot(pidl1, fWebOnly, &psf)))
            {
                if (0 == psf->CompareIDs(0, _ILNext(pidl1), _ILNext(pidl2)))
                    fRet = TRUE;

                psf->Release();
            }
        }
        
        if (!fRet && !fWebOnly)
        {
#undef ILIsEqual
            fRet = ILIsEqual(pidl1, pidl2);
        }
        
        if (fRet && !fIgnoreHidden)
        {
            fRet = (0 == ILCompareHiddenString(pidl1, pidl2, IDLHID_URLFRAGMENT));

            if (fRet)
                fRet = (0 == ILCompareHiddenString(pidl1, pidl2, IDLHID_URLQUERY));
        }

        return fRet;
    }
    
    return TRUE;
}

// pszName must be MAX_URL_STRING
STDAPI IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pszName, UINT uFlags)
{
    return IEGetNameAndFlags(pidl, uFlags, pszName, MAX_URL_STRING, NULL);
}

#ifdef UNIX
//IEUNIX : No support for internal and external names in hp and solaris linker.
extern "C" STDAPI IEGetDisplayNameW(LPCITEMIDLIST pidl, LPWSTR pszName, UINT uFlags)
{
    return IEGetDisplayName(pidl, pszName, uFlags);
}
#endif

HRESULT _GetInternetFolderName(LPWSTR pszName, DWORD cchName)
{
    LPCTSTR pszKey;
    DWORD cbSize = CbFromCch(cchName);

    if (4 > GetUIVersion())
        pszKey = TEXT("CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}");
    else
        pszKey = TEXT("CLSID\\{871C5380-42A0-1069-A2EA-08002B30309D}"); 

    if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, pszKey, NULL, NULL, pszName, &cbSize)
    && *pszName)
        return S_OK;

    if (MLLoadString(IDS_REG_THEINTERNET, pszName, cchName)
    && *pszName)
        return S_OK;

    return E_UNEXPECTED;
}
    
STDAPI IEGetNameAndFlags(LPCITEMIDLIST pidl, UINT uFlags, LPWSTR pszName, DWORD cchName, DWORD *prgfInOutAttrs)
{
    HRESULT hres = E_FAIL;

    if (pszName)
    {
        VDATEINPUTBUF(pszName, TCHAR, cchName);
        *pszName = 0;
    }

    //  for support of NON-integrated builds, and 
    //  to expedite handling of URLs while browsing
    if (IsURLChild(pidl, FALSE)) 
    {
        hres = InitPSFInternet();
        if (SUCCEEDED(hres))
        {
            if (pszName)
            {
                STRRET str;
                hres = g_psfInternet->GetDisplayNameOf(_ILNext(pidl), uFlags, &str);
                if (SUCCEEDED(hres))
                {
                    StrRetToBufW(&str, pidl, pszName, cchName);
                }
            }

            if (prgfInOutAttrs)
                hres = IEGetAttributesOf(pidl, prgfInOutAttrs);
            
        }
    } 
    else if (GetUIVersion() <= 4 && IsURLChild(pidl, TRUE))
    {
        //
        //  we need to support requests for the Internet SFs
        //  Friendly name.  on NT5 we will always have something
        //  even when the SF is hidden.  but on older versions
        //  of the shell, it was possible to delete the folder
        //  by just removing the icon from the desktop
        //

        hres = _GetInternetFolderName(pszName, cchName);
        if (prgfInOutAttrs)
            hres = IEGetAttributesOf(pidl, prgfInOutAttrs);
    }
    else if (ILIsRooted(pidl))
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlChild;
        
        hres = IEBindToParentFolder(pidl, &psf, &pidlChild);
        if (SUCCEEDED(hres))
        {
            if (pszName)
            {
                STRRET str;
                hres = IShellFolder_GetDisplayNameOf(psf, pidlChild, uFlags, &str, 0);
                if (SUCCEEDED(hres))
                {
                    hres = StrRetToBufW(&str, pidlChild, pszName, cchName);
                }
            }

            if (prgfInOutAttrs)
                hres = psf->GetAttributesOf(ILIsEmpty(pidlChild) ? 0 : 1, &pidlChild, prgfInOutAttrs);

            psf->Release();
        }
    } 
    else
        hres = SHGetNameAndFlags(pidl, uFlags, pszName, cchName, prgfInOutAttrs);

    if (SUCCEEDED(hres) && pszName && (uFlags & SHGDN_FORPARSING))
    {
        // 
        //  need to correctly append the fragment and query to the base
        //  if pszName is a DOSPATH, it will be converted to a
        //  file: URL so that it can accomadate the location
        //
        WCHAR sz[MAX_URL_STRING];
        DWORD cch = cchName;

        if (ILGetHiddenStringW(pidl, IDLHID_URLQUERY, sz, SIZECHARS(sz)))
            hres = UrlCombineW(pszName, sz, pszName, &cch, 0);
        
        if (IEILGetFragment(pidl, sz, SIZECHARS(sz)))
            hres = UrlCombineW(pszName, sz, pszName, &cchName, 0);

        //  else 
        //      BUBBUG - should we return just the fragment in some case?
    }

    TraceMsg(TF_URLNAMESPACE, "IEGDN(%s) returning %x", pszName, hres);
    return hres;
}

BOOL _ClassIsBrowsable(LPCTSTR pszClass)
{
    BOOL fRet = FALSE;
    HKEY hk;
    
    if (SUCCEEDED(AssocQueryKey(0, ASSOCKEY_CLASS, pszClass, NULL, &hk)))
    {
        fRet = (NOERROR == RegQueryValueEx(hk, TEXT("DocObject"), NULL, NULL, NULL, NULL)
             || NOERROR == RegQueryValueEx(hk, TEXT("BrowseInPlace"), NULL, NULL, NULL, NULL));

        RegCloseKey(hk);
    }

    return fRet;
}

BOOL _MimeIsBrowsable(LPCTSTR pszExt)
{
    BOOL fRet = FALSE;
    TCHAR sz[MAX_PATH];
    DWORD dwSize = sizeof(sz);
    if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, pszExt, TEXT("Content Type"), NULL, (LPVOID) sz, &dwSize))
    {
        TCHAR szKey[MAX_PATH];
        dwSize = SIZEOF(sz);

        // Get the CLSID for the handler of this content type.
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("MIME\\Database\\Content Type\\%s"), sz);

        // reuse sz for the clsid
        if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, szKey, TEXT("CLSID"), NULL, (LPVOID) sz, &dwSize))
        {
            fRet = _ClassIsBrowsable(sz);
        }
    }
    return fRet;
}
                    
BOOL _StorageIsBrowsable(LPCTSTR pszPath)
{
    BOOL fRet = FALSE;
    //
    // If the file is STILL not browsable, try to open it as a structured storage
    // and check its CLSID.
    //
    IStorage *pStg = NULL;

    if (StgOpenStorage(pszPath, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &pStg ) == S_OK && pStg)
    {
        STATSTG  statstg;
        if (pStg->Stat( &statstg, STATFLAG_NONAME ) == S_OK)
        {
            TCHAR szClsid[GUIDSTR_MAX];
            SHStringFromGUIDW(statstg.clsid, szClsid, SIZECHARS(szClsid));
            
            fRet = _ClassIsBrowsable(szClsid);
        }
        
        pStg->Release();
    }

    return fRet;
}

BOOL _IEIsBrowsable(LPCITEMIDLIST pidl)
{
    TCHAR szPath[MAX_PATH];
    BOOL fRet = FALSE;
    
    if (SUCCEEDED(SHGetPathFromIDList(pidl, szPath)))
    {
        // Don't change the order of the following OR'd conditions because
        // we want the HTML test to go first.  Also, the NT5 shell will
        // do the ClassIsBrowsable check for us, so we should avoid repeating
        // that check.

        if (PathIsHTMLFile(szPath)
        || _ClassIsBrowsable(szPath)
        || _MimeIsBrowsable(PathFindExtension(szPath))
        || _StorageIsBrowsable(szPath))
            fRet = TRUE;
    }
    
    return fRet;
}


HRESULT _IEGetAttributesOf(LPCITEMIDLIST pidl, DWORD* pdwAttribs, BOOL fAllowExtraChecks)
{
    HRESULT hres = E_FAIL;
    DWORD dwAttribs = *pdwAttribs;
    BOOL fExtraCheckForBrowsable = FALSE;

    //
    // BUGBUG - Check if we need to execute an additional logic - ZekeL - 7-JAN-99
    // to see if it's browsable or not. this is necessary on shell32s from NT4/win95/IE4 
    // both NT4/win95 have no notion of SFGAO_BROWSABLE, and even though
    // IE4 does, it doesnt handle it correctly for UNICODE file names.
    // We are just as thorough (more) in our private check, so just defer to it.  
    //
    // 78777: Even if we are on NT5, IE can browse things that Shell thinks is not 
    // browsable, for example, .htm files when Netscape is the default browser.  
    // So we should do the extra check on every platform.

    if (fAllowExtraChecks && (dwAttribs & SFGAO_BROWSABLE)) 
    {
        dwAttribs |= SFGAO_FILESYSTEM | SFGAO_FOLDER;
        fExtraCheckForBrowsable = TRUE;
    }

    IShellFolder* psfParent;
    LPCITEMIDLIST pidlChild;
    
    if (ILIsEmpty(pidl))
    {
        hres = SHGetDesktopFolder(&psfParent);
        pidlChild = pidl;
    }
    else if (ILIsRooted(pidl) && ILIsEmpty(_ILNext(pidl)))
    {
        //  
        //  when getting the attributes of the root itself, we
        //  decide its better to just limit the attribs to 
        //  some easily supported subset.  we used to always
        //  fail, but that is a little extreme.
        //
        //  we could also try to get the attributes from HKCR\CLSID\{clsid}\shellfolder\attributes
        //
        *pdwAttribs &= (SFGAO_FOLDER);
        return S_OK;
    }
    else 
    {
        if (GetUIVersion() < 4 && IsURLChild(pidl, TRUE))
        {
            IShellFolder *psfRoot;
            //
            //  if we are Browser Only, and this is the
            //  internet folder itself that we are interested
            //  in, then we need to bind to it by hand
            //  and query it with cidl = 0
            //
            hres = _GetInternetRoot(&psfRoot);

            if (SUCCEEDED(hres))
            {
                hres = SHBindToFolderIDListParent(psfRoot, _ILNext(pidl), IID_IShellFolder, (void **)&psfParent, &pidlChild);
                psfRoot->Release();
            }
        }
        else if (ILIsRooted(pidl))
            hres = IEBindToParentFolder(pidl, &psfParent, &pidlChild);
        else
            hres = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psfParent, &pidlChild);
    }

    if (SUCCEEDED(hres))
    {
        ASSERT(psfParent);
        hres = psfParent->GetAttributesOf(ILIsEmpty(pidlChild) ? 0 : 1, &pidlChild, &dwAttribs);

        if (FAILED(hres))
            TraceMsg(TF_WARNING, "IEGetAttribs psfParent->GetAttr failed %x", hres);

        psfParent->Release();
    }   
    else
        TraceMsg(TF_WARNING, "IEGetAttribs BindTOParent failed %x", hres);

    //
    //  This is the extra logic we need to execute if this is a browser
    // only mode to get the right "browsable" attribute flag to DocObjects.
    //
    if (fExtraCheckForBrowsable && !(dwAttribs & SFGAO_BROWSABLE))
    {
        if ((dwAttribs & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM) 
        {
            if (_IEIsBrowsable(pidl))
                dwAttribs |= SFGAO_BROWSABLE;
        }
    }

    *pdwAttribs &= dwAttribs;
    return hres;
}

HRESULT IEGetAttributesOf(LPCITEMIDLIST pidl, DWORD* pdwAttribs)
{
    return _IEGetAttributesOf(pidl, pdwAttribs, TRUE);
}

// BRYANST: 7/22/97  -  NT Bug #188099
// shell32.dll in IE4 SI and only in that version had a bug if pbc was passed
// to IShellFolder::BindToObject() (fstreex.c!FSBindToFSFolder), it would fail
// to bind to Shell Extensions that extended file system folders, such as:
// the history folder, the occache, etc.  We work around this by passing a NULL pbc
// if the destination is an IE4 shell32.dll and it will go thru FSBindToFSFolder().
BOOL ShouldWorkAroundBCBug(LPCITEMIDLIST pidl)
{
    BOOL fWillBCCauseBug = FALSE;

    if (4 == GetUIVersion())
    {
        LPITEMIDLIST pidlCopy = ILClone(pidl);
        LPITEMIDLIST pidlIterate = pidlCopy;

        // Skip the first two ItemIDs. (#1 could be My Computer)
        if (!ILIsEmpty(pidlIterate))
        {
            IShellFolder * psf;

            // (#2 could be CFSFolder::BindToObject())
            pidlIterate = _ILNext(pidlIterate);
            if (!ILIsEmpty(pidlIterate))
            {
                pidlIterate = _ILNext(pidlIterate);
                // Remove everything else so we bind directly to CFSFolder::BindToObject()
                pidlIterate->mkid.cb = 0;

                if (SUCCEEDED(IEBindToObject(pidlCopy, &psf)))
                {
                    IPersist * pp;

                    if (SUCCEEDED(psf->QueryInterface(IID_IPersist, (void **)&pp)))
                    {
                        CLSID clsid;

                        if (SUCCEEDED(pp->GetClassID(&clsid)) && 
                            IsEqualCLSID(clsid, CLSID_ShellFSFolder))
                        {
                            fWillBCCauseBug = TRUE;
                        }

                        pp->Release();
                    }
                    psf->Release();
                }
            }

        }

        ILFree(pidlCopy);
    }

    return fWillBCCauseBug;
}

typedef enum
{
    SHOULDBIND_DOCOBJ,
    SHOULDBIND_DESKTOP,
    SHOULDBIND_NONE,
} SHOULDBIND;

//
//  _ShouldDocObjBind() 
//  returns 
//      SHOULDBIND_DOCOBJ   -  Should just use DocObjectFolder directly
//      SHOULDBIND_DESKTOP  -  bind through the desktop
//      SHOULDBIND_NONE     -  FAIL the bind...
//
SHOULDBIND _ShouldDocObjBind(DWORD dwAttribs, BOOL fStrictBind)
{
    if (fStrictBind)
    {
        if ((dwAttribs & (SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_FILESYSTEM)) == (SFGAO_BROWSABLE | SFGAO_FILESYSTEM))
            return SHOULDBIND_DOCOBJ;
        else
            return SHOULDBIND_DESKTOP;
    }
    else
    {
        if (dwAttribs & (SFGAO_FOLDER | SFGAO_BROWSABLE))
            return SHOULDBIND_DESKTOP;

        // manually bind using our CDocObjectFolder for
        // files which are not DocObject. Without this code, file:
        // to non-Docobject files (such as multi-media files)
        // won't do anything.
        //
        // is is needed for non integraded browser mode 
        //
        if (dwAttribs & SFGAO_FILESYSTEM) 
            return SHOULDBIND_DOCOBJ;
        else
            return SHOULDBIND_NONE;
    }
}

STDAPI _IEBindToObjectInternal(BOOL fStrictBind, LPCITEMIDLIST pidl, IBindCtx * pbc, REFIID riid, void **ppvOut)
{
    IShellFolder *psfTemp;
    HRESULT hr;

    *ppvOut = NULL;

    // Special case:  If we have the pidl for the "Desktop" then just use the Desktop folder itself
    if (ILIsEmpty(pidl))
    {
        hr = SHGetDesktopFolder(&psfTemp);
        if (SUCCEEDED(hr))
        {
            hr = psfTemp->QueryInterface(riid, ppvOut);
            psfTemp->Release();
        }
    } 
    else 
    {
        BOOL fIsUrlChild = IsURLChild(pidl, TRUE);

        if (fIsUrlChild || ILIsRooted(pidl))
        {
            hr = _GetRoot(pidl, fIsUrlChild, &psfTemp);
            if (SUCCEEDED(hr))
            {
                pidl = _ILNext(pidl);
                
                if (!ILIsEmpty(pidl))
                    hr = psfTemp->BindToObject(pidl, pbc, riid, ppvOut);
                else
                    hr = psfTemp->QueryInterface(riid, ppvOut);

                psfTemp->Release();
            }
        }
        else
        {
            // non integrated browser mode will succeed on 
            // BindToObject(IID_IShellFolder) even for things that should 
            // fail (files). to avoid the down stream problems caused by this we
            // filter out things that are not "browseable" up front, 
            //
            // NOTE: this does not work on simple PIDLs

            DWORD dwAttribs = SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_FILESYSTEM;

            hr = _IEGetAttributesOf(pidl, &dwAttribs, fStrictBind);
            
            if (SUCCEEDED(hr)) 
            {
                switch (_ShouldDocObjBind(dwAttribs, fStrictBind))
                {
                case SHOULDBIND_DOCOBJ:
                    {
                        //
                        // shortcircuit and bind using our CDocObjectFolder for
                        // files which are BROWSABLE. Without this code, file:
                        // to non-Docobject files (such as multi-media files)
                        // won't do anything.
                        //
                        // is is needed for non integraded browser mode 
                        //
                        CDocObjectFolder *pdof = new CDocObjectFolder();

                        TraceMsg(TF_URLNAMESPACE, "IEBTO(%x) using DocObjectFolder", pidl);
                        if (pdof)
                        {
                            hr = pdof->Initialize(pidl);
                            if (SUCCEEDED(hr)) 
                                hr = pdof->QueryInterface(riid, ppvOut);
                            pdof->Release();
                        }
                        else
                            hr = E_OUTOFMEMORY;    
                    }
                    break;

                case SHOULDBIND_DESKTOP:
                    {
                        //
                        // This is the normal case. We just bind down through the desktop...
                        //
                        TraceMsg(TF_URLNAMESPACE, "IEBTO(%x) using Desktop", pidl);

                        hr = SHGetDesktopFolder(&psfTemp);
                        if (SUCCEEDED(hr))
                        {
                            // BRYANST: 7/22/97  -  NT Bug #188099
                            // shell32.dll in IE4 SI and only in that version had a bug if pbc was passed
                            // to IShellFolder::BindToObject() (fstreex.c!FSBindToFSFolder), it would fail
                            // to bind to Shell Extensions that extended file system folders, such as:
                            // the history folder, the occache, etc.  We work around this by passing a NULL pbc
                            // if the destination is an IE4 shell32.dll and it will go thru FSBindToFSFolder().
                            if (pbc && ShouldWorkAroundBCBug(pidl))
                            {
                                pbc = NULL;
                            }

                            hr = psfTemp->BindToObject(pidl, pbc, riid, ppvOut);
                            psfTemp->Release();
                        }
                    } 
                    break;

                default:
                    hr = E_FAIL;
                }
            } 
        }
    }
    
    TraceMsg(TF_URLNAMESPACE, "IEBTO(%x) returning %x", pidl, hr);

    return hr;
}

STDAPI IEBindToObjectEx(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppvOut)
{
    //bug# 87017: Some apps for eg:ws_ftp explorer returns S_OK but sets the interface 
    //pointer to NULL. Check for null pointer and set hr to E_FAIL if its NULL.
    HRESULT hr = S_OK;
    hr = _IEBindToObjectInternal(TRUE, pidl, pbc, riid, ppvOut);
    if(SUCCEEDED(hr) && *ppvOut == NULL)
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT IEBindToObject(LPCITEMIDLIST pidl, IShellFolder **ppsfOut)
{
    return _IEBindToObjectInternal(TRUE, pidl, NULL, IID_IShellFolder, (void **)ppsfOut);
}

HRESULT IEBindToObjectWithBC(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsfOut)
{
    return _IEBindToObjectInternal(TRUE, pidl, pbc, IID_IShellFolder, (void **)ppsfOut);
}

//  CLASSIC BIND here
HRESULT IEBindToObjectForNavigate(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsfOut)
{
    return _IEBindToObjectInternal(FALSE, pidl, pbc, IID_IShellFolder, (void **)ppsfOut);
}

//
// CDwnCodePage: Dummy supports IBindCtx interface object only for casting
//               It holds codepage info to pass via LPBC parameter
//
class CDwnCodePage : public IBindCtx
                   , public IDwnCodePage
{
public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IBindCtx methods
    virtual STDMETHODIMP RegisterObjectBound(IUnknown *punk) { return (_pbc ? _pbc->RegisterObjectBound(punk) : E_NOTIMPL); };
    virtual STDMETHODIMP RevokeObjectBound(IUnknown *punk) { return (_pbc ? _pbc->RevokeObjectBound(punk) : E_NOTIMPL); };
    virtual STDMETHODIMP ReleaseBoundObjects(void) { return (_pbc ? _pbc->ReleaseBoundObjects() : E_NOTIMPL); };
    virtual STDMETHODIMP SetBindOptions(BIND_OPTS *pbindopts) { return (_pbc ? _pbc->SetBindOptions(pbindopts) : E_NOTIMPL); };
    virtual STDMETHODIMP GetBindOptions(BIND_OPTS *pbindopts) { return (_pbc ? _pbc->GetBindOptions(pbindopts) : E_NOTIMPL); };
    virtual STDMETHODIMP GetRunningObjectTable(IRunningObjectTable **pprot) { *pprot = NULL; return (_pbc ? _pbc->GetRunningObjectTable(pprot) : E_NOTIMPL); };
    virtual STDMETHODIMP RegisterObjectParam(LPOLESTR pszKey, IUnknown *punk) { return (_pbc ? _pbc->RegisterObjectParam(pszKey, punk) : E_NOTIMPL); };
    virtual STDMETHODIMP GetObjectParam(LPOLESTR pszKey, IUnknown **ppunk) { *ppunk = NULL; return (_pbc ? _pbc->GetObjectParam(pszKey, ppunk) : E_NOTIMPL); };
    virtual STDMETHODIMP EnumObjectParam(IEnumString **ppenum) { *ppenum = NULL; return (_pbc ? _pbc->EnumObjectParam(ppenum) : E_NOTIMPL); };
    virtual STDMETHODIMP RevokeObjectParam(LPOLESTR pszKey) { return (_pbc ? _pbc->RevokeObjectParam(pszKey) : E_NOTIMPL); };

    virtual STDMETHODIMP RemoteSetBindOptions(BIND_OPTS2 *pbindopts) { return E_NOTIMPL; };
    virtual STDMETHODIMP RemoteGetBindOptions(BIND_OPTS2 *pbindopts) { return E_NOTIMPL; };

    // IDwnCodePage methods
    virtual STDMETHODIMP_(UINT) GetCodePage(void) { return _uiCodePage; };
    virtual STDMETHODIMP SetCodePage(UINT uiCodePage) { _uiCodePage = uiCodePage; return S_OK; };

    // Constructor
    CDwnCodePage(IBindCtx * pbc) : _cRef(1) { _uiCodePage = CP_ACP; _pbc = NULL; IUnknown_Set((IUnknown **)&_pbc, (IUnknown *)pbc); };
    ~CDwnCodePage() { ATOMICRELEASE(_pbc); };

private:
    int     _cRef;
    UINT    _uiCodePage;
    IBindCtx * _pbc;
};

STDAPI CDwnCodePage::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IBindCtx))
    {
        *ppvObj = SAFECAST(this, IBindCtx*);
    }
    else if (IsEqualIID(riid, IID_IDwnCodePage))
    {
        *ppvObj = SAFECAST(this, IDwnCodePage*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

STDAPI_(ULONG) CDwnCodePage::AddRef()
{
    _cRef++;
    return _cRef;
}

STDAPI_(ULONG) CDwnCodePage::Release()
{
    _cRef--;
    if (0 < _cRef)
        return _cRef;

    delete this;
    return 0;
}

// IEParseDisplayName() will do all of the below functionality in IECreateFromPathCPWithBC()
// plus the following two things:
// 1.  It will call ParseURLFromOutsideSource(), so this is more friendly to
//     strings from outside sources.
// 2.  If the URL has a fragment, this function will pass out a PIDL with the last
//     ID being the location.
HRESULT IECreateFromPathCPWithBCW(UINT uiCP, LPCWSTR pszPath, IBindCtx * pbc, LPITEMIDLIST *ppidlOut)
{
    TraceMsg(TF_URLNAMESPACE, "IECFP(%s) called", pszPath);

    HRESULT hr = S_OK;
    WCHAR szPath[MAX_URL_STRING];
    WCHAR szBuf[MAX_PATH];
    DWORD cchBuf = ARRAYSIZE(szBuf);
    CDwnCodePage DwnCodePage(pbc);
    DWORD len;

    // Initialize for failure case
    *ppidlOut = NULL;

    // if we are passed a NULL path, then there is no way we can convert it to a pidl.
    // in some cases the reason we are passed a NULL path is because the IShellFolder
    // provider was unable to generate a parseable display name (MSN Classic 1.3 is
    // a very good example, they return E_NOTIMPL).
    if ( ((len = lstrlen( pszPath )) == 0)  || len >= MAX_URL_STRING )
    {
        return E_FAIL;
    }

    // Is this a "file:" URL?
    if (IsFileUrlW(pszPath) && SUCCEEDED(hr = PathCreateFromUrl(pszPath, szBuf, &cchBuf, 0)))
        pszPath = szBuf;

    BOOL fIsFilePath = PathIsFilePath(pszPath);

#ifdef FEATURE_IE_USE_DESKTOP_PARSING
    //
    //  in order to take advantage of whatever enhancements the desktop
    //  makes to parsing (eg, WebFolders and shell: URLs), then we allow
    //  the desktop first go at it.  it will loop back into the internet
    //  shell folder if all the special cases fail.
    //      maybe use a reg setting to control???
    //
    //
    if (fIsFilePath || GetUIVersion() >= 5)
#else // !FEATURE_IE_USE_DESKTOP_PARSING
    //
    //  right now we just use the desktop if its a file path or
    //  it is a shell: URL on NT5
    //
    if (fIsFilePath || (GetUIVersion() >= 5 && URL_SCHEME_SHELL == GetUrlSchemeW(pszPath)))
#endif // FEATURE_IE_USE_DESKTOP_PARSING
    {
        ASSERT(SUCCEEDED(hr));
        
        // Resolve any dot-dot path reference and remove trailing backslash
        if (fIsFilePath)
        {
            PathCanonicalize(szPath, pszPath);
            pszPath = szPath;

            // This call will cause a network hit: one connection attempt to \\server\IPC$
            // and then a series of FindFirst's - one for each directory.
            if (StrChr(pszPath, L'*') || StrChr(pszPath, L'?'))
            {
                hr = E_FAIL;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            hr = SHILCreateFromPath(pszPath, ppidlOut, NULL);               
            TraceMsg(DM_CDOFPDN, "IECreateFromPath psDesktop->PDN(%s) returned %x", pszPath, hr);
        }
    }
    else
    {
        //
        // Need to put in buffer since ParseDisplayName doesn't take a 'const' string.
        StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
        pszPath = szPath;

        // Avoid the network and disk hits above for non-file urls.
        // This code path is taken on http: folks so a nice optimization. We will then drop
        // down below where we check the internet namespace.
        IShellFolder *psfRoot;
        hr = _GetInternetRoot(&psfRoot);
        if (SUCCEEDED(hr))
        {
            TraceMsg(TF_URLNAMESPACE, "IECFP(%s) calling g_psfInternet->PDN %x", pszPath, hr);
            LPITEMIDLIST pidlRel;

            hr = psfRoot->ParseDisplayName(NULL, (IBindCtx*)&DwnCodePage, szPath, NULL, &pidlRel, NULL);
            TraceMsg(DM_CDOFPDN, "IECreateFromPath called psfInternet->PDN(%s) %x", pszPath, hr);
            if (SUCCEEDED(hr))
            {
                *ppidlOut = ILCombine(c_pidlURLRoot, pidlRel);
                if (!*ppidlOut)
                    hr = E_OUTOFMEMORY;
                ILFree(pidlRel);
            }
            
            psfRoot->Release();
        }

    }

    // NOTE: NT5 beta 3 and before had a call to SHSimpleIDListFromPath().
    //       This is very bad because it will parse any garbage and prevent
    //       the caller from finding invalid strings.  I(BryanSt) needed
    //       this fixed for IEParseDisplayNameWithBCW() would fail on invalid
    //       address bar strings ("Search Get Rich Quick").
    TraceMsg(TF_URLNAMESPACE, "IECFP(%s) returning %x (hr=%x)",
             pszPath, *ppidlOut, hr);

    return hr;
}

HRESULT IECreateFromPathCPWithBCA(UINT uiCP, LPCSTR pszPath, IBindCtx * pbc, LPITEMIDLIST *ppidlOut)
{
    WCHAR szPath[MAX_URL_STRING];

    ASSERT(lstrlenA(pszPath) < ARRAYSIZE(szPath));
    SHAnsiToUnicodeCP(uiCP, pszPath, szPath, ARRAYSIZE(szPath));

    return IECreateFromPathCPWithBCW(uiCP, szPath, pbc, ppidlOut);
}


HRESULT IEParseDisplayName(UINT uiCP, LPCTSTR pszPath, LPITEMIDLIST * ppidlOut)
{
    return IEParseDisplayNameWithBCW(uiCP, pszPath, NULL, ppidlOut);
}


// This function will do two things that IECreateFromPathCPWithBC() will not do:
// 1.  It will add the "Query" section of the URL into the pidl.
// 2.  If the URL has a fragment, this function will pass out a PIDL with the last
//     ID being the location.
// NOTE: If the caller needs the string to be "cleaned up" because the user manually
//       entered the URL, the caller needs to call ParseURLFromOutsideSource() before
//       calling this function.  That function should only be called on strings entered
//       by the user because of the perf hit and it could incorrectly format valid
//       parsible display names.  For example, ParseURLFromOutsideSource() will
//       convert the string "My Computer" into a search URL for yahoo.com
//       (http://www.yahoo.com/search.asp?p=My+p=Computer) when some callers
//       want that string parsed by an IShellFolder in the desktop.
HRESULT IEParseDisplayNameWithBCW(UINT uiCP, LPCWSTR pwszPath, IBindCtx * pbc, LPITEMIDLIST * ppidlOut)
{
    TCHAR szPath[MAX_URL_STRING];
    LPCWSTR pwszFileLocation = NULL;
    WCHAR szQuery[MAX_URL_STRING];
    HRESULT hres;

    szQuery[0] = TEXT('\0');
#ifdef DEBUG
    if (IsFlagSet(g_dwDumpFlags, DF_URL)) 
    {
        TraceMsg(DM_TRACE, "IEParseDisplayName got %s", szPath);
    }
#endif

    //  We want to remove QUERY and FRAGMENT sections of
    //  FILE URLs because they need to be added in "Hidden" pidls.
    //  Also, URLs need to be escaped all the time except for paths
    //  to facility parsing and because we already removed all other
    //  parts of the URL (Query and Fragment).
    if (IsFileUrlW(pwszPath)) 
    {
        DWORD cchQuery = SIZECHARS(szQuery) - 1;
        
        pwszFileLocation = UrlGetLocationW(pwszPath);        

        if (SUCCEEDED(UrlGetPart(pwszPath, szQuery+1, &cchQuery, URL_PART_QUERY, 0)) && cchQuery)
            szQuery[0] = TEXT('?');

        DWORD cchPath = ARRAYSIZE(szPath);
        if (FAILED(PathCreateFromUrl(pwszPath, szPath, &cchPath, 0))) 
        {
            // Failed to parse it back. Use the original.
            StrCpyN(szPath, pwszPath, ARRAYSIZE(szPath));
        }
    }        
    else 
    {
        // If we failed, just try to use the original
        StrCpyN(szPath, pwszPath, ARRAYSIZE(szPath));
    }

#ifdef DEBUG
    if (IsFlagSet(g_dwDumpFlags, DF_URL)) 
        TraceMsg(DM_TRACE, "IEParseDisplayName calling IECreateFromPath %s", szPath);
#endif

    hres = IECreateFromPathCPWithBC(uiCP, szPath, pbc, ppidlOut);
    if (SUCCEEDED(hres) && pwszFileLocation)
    {
        ASSERT(*ppidlOut);
        *ppidlOut = IEILAppendFragment(*ppidlOut, pwszFileLocation);
        hres = *ppidlOut ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hres) && szQuery[0] == TEXT('?'))
    {
        *ppidlOut = ILAppendHiddenString(*ppidlOut, IDLHID_URLQUERY, szQuery);
        hres = *ppidlOut ? S_OK : E_OUTOFMEMORY;
    }

    return hres;
}
