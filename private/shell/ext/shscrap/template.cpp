#include "shole.h"
#include "ids.h"
#include "scguid.h"

#ifdef FEATURE_SHELLEXTENSION

extern "C" const TCHAR c_szCLSID[];

class CTemplateFolder : public IShellFolder, public IPersistFolder
{
public:
    CTemplateFolder();
    ~CTemplateFolder();
    inline BOOL ConstructedSuccessfully() { return _hdpaMap != NULL; }

    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    // IShellFolder
    virtual HRESULT __stdcall ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

    virtual HRESULT __stdcall EnumObjects( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList);

    virtual HRESULT __stdcall BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut);
    virtual HRESULT __stdcall BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj);
    virtual HRESULT __stdcall CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual HRESULT __stdcall CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    virtual HRESULT __stdcall GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut);
    virtual HRESULT __stdcall GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual HRESULT __stdcall GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    virtual HRESULT __stdcall SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut);

    // IPersistFolder
    virtual HRESULT __stdcall GetClassID(LPCLSID lpClassID);
    virtual HRESULT __stdcall Initialize(LPCITEMIDLIST pidl);

protected:
    // Defview callback
    friend HRESULT CALLBACK DefViewCallback(
                                LPSHELLVIEW psvOuter, LPSHELLFOLDER psf,
                                HWND hwndOwner, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);
    HRESULT GetDetailsOfDVM(UINT ici, DETAILSINFO *pdi);

    BOOL IsMyPidl(LPCITEMIDLIST pidl)
	{ return (pidl->mkid.abID[0] == 'S' && pidl->mkid.abID[1] == 'N'); }

    UINT _cRef;

    //
    //  To speed up name lookups, we cache the mapping between CLSIDs and
    //  display names.  We cannot persist this mapping because it won't
    //  survive localization or ANSI/UNICODE interop.
    //
    typedef struct {
        CLSID clsid;
        TCHAR achName[MAX_PATH];
    } CLSIDMAP, *PCLSIDMAP;

    HDPA _hdpaMap;

    HRESULT GetNameOf(LPCITEMIDLIST pidl, LPCTSTR *ppsz);
};

class CEnumTemplate : public IEnumIDList
{
public:
    CEnumTemplate(DWORD grfFlags);
    ~CEnumTemplate();

protected:
    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    virtual HRESULT __stdcall Next(ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched);
    virtual HRESULT __stdcall Skip(ULONG celt);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumIDList **ppenum);

    UINT	_cRef;
    const DWORD	_grfFlags;
    UINT	_iCur;
    HKEY	_hkeyCLSID;
};

//
//  For Win95/NT interop, our PIDLs are always UNICODE.
//  Use explicit packing for Win32/64 interop.
#include <pshpack1.h>
typedef struct _TIDL {
    USHORT          cb;             // This matches SHITEMID
    BYTE            abID[2];        // This matches SHITEMID
    CLSID	    clsid;
} TIDL;
typedef const UNALIGNED TIDL *PTIDL;

//
//  This is the TIDL constructor -- it has the cbZero at the end.
//
typedef struct _TIDLCONS {
    TIDL            tidl;
    USHORT          cbZero;
} TIDLCONS;

#include <poppack.h>

class CTemplateUIObj : public IExtractIcon, public IDataObject, public IContextMenu
{
public:
    static HRESULT Create(REFCLSID, REFIID, LPVOID*);
protected:
    CTemplateUIObj(REFCLSID rclsid)
                    : _clsid(rclsid), _cRef(1)
                                { DllAddRef(); }
    ~CTemplateUIObj()           { DllRelease(); }
    HRESULT _CreateInstance(IStorage* pstg);

    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    // *** IExtractIcon methods ***
    virtual HRESULT __stdcall GetIconLocation(
                         UINT   uFlags, LPTSTR  szIconFile,
                         UINT   cchMax, int   * piIndex,
                         UINT  * pwFlags);

    virtual HRESULT __stdcall Extract(
                           LPCTSTR pszFile, UINT          nIconIndex,
                           HICON   *phiconLarge, HICON   *phiconSmall,
                           UINT    nIconSize);

    // IDataObject
    virtual HRESULT __stdcall GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall QueryGetData(FORMATETC *pformatetc);
    virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
    virtual HRESULT __stdcall SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT __stdcall DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT __stdcall DUnadvise(DWORD dwConnection);
    virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

    // IContextMenu
    virtual HRESULT __stdcall QueryContextMenu(
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    virtual HRESULT __stdcall InvokeCommand(
                             LPCMINVOKECOMMANDINFO lpici);

    virtual HRESULT __stdcall GetCommandString(
                                UINT        idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);

    UINT	_cRef;
    CLSID	_clsid;
};


CTemplateFolder::CTemplateFolder() : _cRef(1)
{
    _hdpaMap = DPA_Create(10);
    OleInitialize(NULL);
    DllAddRef();
}

CTemplateFolder::~CTemplateFolder()
{
    if (_hdpaMap) {
        for (int i = DPA_GetPtrCount(_hdpaMap) - 1; i >= 0; i--) {
            PCLSIDMAP pmap = (PCLSIDMAP)DPA_FastGetPtr(_hdpaMap, i);
            LocalFree(pmap);
        }
        DPA_Destroy(_hdpaMap);
    }

    OleUninitialize();
    DllRelease();
}

HRESULT CTemplateFolder::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellFolder*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistFolder))
    {
        *ppvObj = (IPersistFolder*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CTemplateFolder::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTemplateFolder::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTemplateFolder_CreateInstance(LPUNKNOWN * ppunk)
{
    *ppunk = NULL;

    CTemplateFolder* ptfld = new CTemplateFolder();
    if (ptfld) {
        if (ptfld->ConstructedSuccessfully()) {
            *ppunk = (IShellFolder *)ptfld;
            return S_OK;
        }
        ptfld->Release();
    }
    return E_OUTOFMEMORY;
}

HRESULT CTemplateFolder::ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    return E_NOTIMPL;
}

HRESULT CTemplateFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList)
{
    *ppenumIDList = new CEnumTemplate(grfFlags);
    return *ppenumIDList ? S_OK : E_OUTOFMEMORY;
}

HRESULT CTemplateFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CTemplateFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj)
{
    return E_NOTIMPL;
}

//
//  If the name is in the cache, celebrate our good fortune and return it.
//  Else, go get the name from the registry and cache it for later.
//
HRESULT CTemplateFolder::GetNameOf(LPCITEMIDLIST pidl, LPCTSTR *ppsz)
{
    if (!IsMyPidl(pidl))
        return E_INVALIDARG;

    HRESULT hres;
    PTIDL ptidl = (PTIDL)pidl;
    CLSIDMAP map;
    map.clsid = ptidl->clsid;           // Align the CLSID
    PCLSIDMAP pmap;
    for (int i = DPA_GetPtrCount(_hdpaMap) - 1; i >= 0; i--) {
        pmap = (PCLSIDMAP)DPA_FastGetPtr(_hdpaMap, i);
        if (IsEqualGUID(pmap->clsid, map.clsid)) {
            *ppsz = pmap->achName;
            return S_OK;
        }
    }

    //
    //  Not in cache -- go find it in the registry
    //
    TCHAR szKey[GUIDSTR_MAX + 6];
    _KeyNameFromCLSID(map.clsid, szKey, ARRAYSIZE(szKey));
    LONG dwSize = ARRAYSIZE(map.achName);
    LONG lError = RegQueryValue(HKEY_CLASSES_ROOT, szKey, map.achName, &dwSize);
    if (lError == ERROR_SUCCESS)
    {
        UINT cb = FIELD_OFFSET(CLSIDMAP, achName[lstrlen(map.achName)+1]);
        pmap = (PCLSIDMAP)LocalAlloc(LMEM_FIXED, cb);
        if (pmap) {
            CopyMemory(pmap, &map, cb);
            if (DPA_AppendPtr(_hdpaMap, pmap) >= 0) {
                *ppsz = pmap->achName;
                hres = S_OK;
            } else {
                LocalFree(pmap);
                hres = E_OUTOFMEMORY;
            }
        } else {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = HRESULT_FROM_WIN32(lError);
    }

    return hres;

}

HRESULT CTemplateFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCTSTR psz1, psz2;
    HRESULT hres;

    hres = GetNameOf(pidl1, &psz1);
    if (SUCCEEDED(hres)) {
        hres = GetNameOf(pidl2, &psz2);
        if (SUCCEEDED(hres)) {
            hres = ResultFromShort(lstrcmpi(psz1, psz2));
        }
    }
    return hres;
}


HRESULT CTemplateFolder::GetDetailsOfDVM(UINT ici, DETAILSINFO *pdi)
{
    HRESULT hres;

    switch (ici) {
    case 0:
        if (pdi->pidl) {
            hres = GetDisplayNameOf(pdi->pidl, SHGDN_NORMAL, &pdi->str);
        } else {
            pdi->fmt = LVCFMT_LEFT;
            pdi->cxChar = 30;
            pdi->str.uType = STRRET_CSTR;
            lstrcpyA(pdi->str.cStr, "Name"); // BUGBUG: NLS
            hres = S_OK;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}

HRESULT CALLBACK DefViewCallback(LPSHELLVIEW psvOuter, LPSHELLFOLDER psf,
                                HWND hwndOwner, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // BUGBUG: DefView GPF if I don't pass the callback function!
    // BUGBUG: DefView GPF if it returns S_FALSE as the default!
    // DebugMsg(DM_TRACE, "sc TR - DefViewCallBack %d,%x,%x", uMsg, wParam, lParam);
    switch(uMsg)
    {
    case DVM_WINDOWDESTROY:
	DebugMsg(DM_TRACE, TEXT("sc TR - DefViewCallBack Calling OleFlushClipboard"));
	OleFlushClipboard();
	return S_OK;
    case DVM_GETDETAILSOF:
        return ((CTemplateFolder*)psf)->GetDetailsOfDVM((UINT)wParam, (DETAILSINFO*)lParam);
    }
    return E_FAIL; // S_FALSE;
}


HRESULT CTemplateFolder::CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    if (IsEqualIID(riid, IID_IShellView))
    {
        CSFV csfv = {
            SIZEOF(CSFV),       // cbSize
            this,               // pshf
            NULL,               // psvOuter
            NULL,               // pidl
            0,
            DefViewCallback,    // pfnCallback
            FVM_ICON,
        };
        return SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW *)ppvOut);
    }
    return E_NOINTERFACE;
}

HRESULT CTemplateFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut)
{
    if (cidl==1)
    {
	UINT rgfOut = SFGAO_CANCOPY /* | SFGAO_HASPROPSHEET */;
	*rgfInOut &= rgfOut;
    }
    else
    {
	*rgfInOut = 0;
    }
    return S_OK;
}

HRESULT CTemplateFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    HRESULT hres = E_INVALIDARG;
    if (cidl==1 && IsMyPidl(apidl[0]))
    {
	PTIDL ptidl = (PTIDL)apidl[0];
	hres = CTemplateUIObj::Create(ptidl->clsid, riid, ppvOut);
    }

    return hres;
}

HRESULT _KeyNameFromCLSID(REFCLSID rclsid, LPTSTR pszKey, UINT cchMax)
{
    ASSERT(cchMax - 6 >= GUIDSTR_MAX);
    lstrcpyn(pszKey, TEXT("CLSID\\"), cchMax);
    SHStringFromGUID(rclsid, pszKey + 6, cchMax - 6);
    return S_OK;
}

HRESULT CTemplateFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
    LPCTSTR psz;
    HRESULT hres;

    hres = GetNameOf(pidl, &psz);
    if (SUCCEEDED(hres)) {
#ifdef UNICODE
        lpName->uType = STRRET_WSTR;
        hres = SHStrDupW(psz, &lpName->pOleStr);
#else
        lstrcpyn(lpName->cStr, psz, ARRAYSIZE(lpName->cStr));
        hres = S_OK;
#endif
    }
    return hres;
}

HRESULT CTemplateFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut)
{
    return E_NOTIMPL;
}

HRESULT __stdcall CTemplateFolder::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_CTemplateFolder;
    return S_OK;
}

HRESULT __stdcall CTemplateFolder::Initialize(LPCITEMIDLIST pidl)
{
    return S_OK;
}

CEnumTemplate::CEnumTemplate(DWORD grfFlags)
    : _cRef(1), _grfFlags(grfFlags), _iCur(0), _hkeyCLSID(NULL)
{
    DllAddRef();
}

CEnumTemplate::~CEnumTemplate()
{
    if (_hkeyCLSID) {
        RegCloseKey(_hkeyCLSID);
    }
    DllRelease();
}

HRESULT CEnumTemplate::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IEnumIDList) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IEnumIDList*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CEnumTemplate::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CEnumTemplate::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CEnumTemplate::Next(ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched)
{
    // Assume error
    if (pceltFetched) {
        *pceltFetched = 0;
    }

    if (!(_grfFlags & SHCONTF_NONFOLDERS)) {
	return S_FALSE;
    }

    if (!_hkeyCLSID)
    {
        if (RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSID, &_hkeyCLSID) != ERROR_SUCCESS)
        {
            return E_FAIL;
        }
    }

    TCHAR szKeyBuf[128];    // enough for {CLSID} or "ProgId/XXX"

    //  Subtract 64 to allow room for the goo we append later on
    while (RegEnumKey(HKEY_CLASSES_ROOT, _iCur++, szKeyBuf, ARRAYSIZE(szKeyBuf) - 64) == ERROR_SUCCESS)
    {
	TCHAR szT[128];
	LONG dwRead;
	int cchKey = lstrlen(szKeyBuf);

	// Check for \NotInsertable.
	lstrcpy(szKeyBuf+cchKey, TEXT("\\NotInsertable"));
	dwRead = ARRAYSIZE(szT);
	if (RegQueryValue(HKEY_CLASSES_ROOT, szKeyBuf, szT, &dwRead) == ERROR_SUCCESS) {
	    continue;
	}

	BOOL fInsertable = FALSE;
//
// Let's stop supporting OLE1 servers anymore.
//
#if 0
	lstrcpy(szKeyBuf+cchKey, TEXT("\\protocol\\StdFileEditing\\server"));
	dwRead = ARRAYSIZE(szT);
	if (RegQueryValue(HKEY_CLASSES_ROOT, szKeyBuf, szT, &dwRead) == ERROR_SUCCESS)
	{
	    fInsertable = TRUE;
	}
	else
#endif
	{
	    lstrcpy(szKeyBuf+cchKey, TEXT("\\Insertable"));
	    dwRead = ARRAYSIZE(szT);
	    if (RegQueryValue(HKEY_CLASSES_ROOT, szKeyBuf, szT, &dwRead) == ERROR_SUCCESS)
	    {
		fInsertable = TRUE;
	    }
	}

	if (fInsertable)
	{
	    lstrcpy(szKeyBuf+cchKey, TEXT("\\CLSID"));
	    dwRead = ARRAYSIZE(szT);
	    if (RegQueryValue(HKEY_CLASSES_ROOT, szKeyBuf, szT, &dwRead) == ERROR_SUCCESS)
	    {
		TIDLCONS tidlCons;
                CLSID clsid;            // Aligned version
		tidlCons.tidl.cb = sizeof(TIDL);
		tidlCons.tidl.abID[0] = 'S';
		tidlCons.tidl.abID[1] = 'N';

		if (GUIDFromString(szT, &clsid))
		{
                    tidlCons.tidl.clsid = clsid;
                    tidlCons.cbZero = 0;
		    rgelt[0] = ILClone((LPITEMIDLIST)&tidlCons);
		    *pceltFetched = 1;
		    return S_OK;
		}
	    }
	}
    }

    return S_FALSE;     // no more element
}

HRESULT CEnumTemplate::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

HRESULT CEnumTemplate::Reset()
{
    return E_NOTIMPL;
}

HRESULT CEnumTemplate::Clone(IEnumIDList **ppenum)
{
    return E_NOTIMPL;
}


//==========================================================================
// CTemplateUIObj members (IUnknown override)
//==========================================================================

HRESULT CTemplateUIObj::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IExtractIcon) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IExtractIcon*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        *ppvObj = (IDataObject*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppvObj = (IContextMenu*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CTemplateUIObj::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTemplateUIObj::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

//
// NOTES: This logic MUST be identical to the one in the shell.
//
int _ParseIconLocation(LPTSTR pszIconFile)
{
    int iIndex = 0;
    LPTSTR pszComma = StrChr(pszIconFile, TEXT(','));

    if (pszComma) {
        *pszComma++ = 0;            // terminate the icon file name.
        iIndex = StrToInt(pszComma);
    }
    PathRemoveBlanks(pszIconFile);
    return iIndex;
}


//==========================================================================
// CTemplateUIObj members (IExtractIcon override)
//==========================================================================

//
// szClass -- Specifies either CLSID\{CLSID} or ProgID
//
HRESULT _GetDefaultIcon(LPCTSTR szClass, LPTSTR szIconFile, UINT cchMax, int *piIndex)
{
    TCHAR szKey[256];
    wsprintf(szKey, TEXT("%s\\DefaultIcon"), szClass);
    TCHAR szValue[MAX_PATH+40];
    LONG dwSize = ARRAYSIZE(szValue);
    if (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &dwSize) == ERROR_SUCCESS)
    {
	*piIndex = _ParseIconLocation(szValue);
	lstrcpyn(szIconFile, szValue, cchMax);
	return S_OK;
    }
    return E_FAIL;
}

HRESULT CTemplateUIObj::GetIconLocation(
                         UINT   uFlags, LPTSTR  szIconFile,
                         UINT   cchMax, int   * piIndex,
                         UINT  * pwFlags)
{
    *pwFlags = GIL_PERCLASS;	// Always per-class

    TCHAR szKey[128];
    HRESULT hres = _KeyNameFromCLSID(_clsid, szKey, ARRAYSIZE(szKey));
    if (SUCCEEDED(hres))
    {
	//
	// First, look at "CLSID\{CLSID}\DefautlIcon"
	//
	hres = _GetDefaultIcon(szKey, szIconFile, cchMax, piIndex);
	if (FAILED(hres))
	{
	    //
	    // Then, look at "ProgID\DefaultIcon" to work-around a bug
	    //  of "Wave Sound".
	    //
    	    lstrcat(szKey, TEXT("\\ProgID"));
	    TCHAR szValue[MAX_PATH+40];
	    LONG dwSize = ARRAYSIZE(szValue);
	    if (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &dwSize) == ERROR_SUCCESS)
	    {
		hres = _GetDefaultIcon(szValue, szIconFile, cchMax, piIndex);
	    }
	}
    }
    return hres;
}

HRESULT CTemplateUIObj::Extract(
                           LPCTSTR pszFile, UINT          nIconIndex,
                           HICON   *phiconLarge, HICON   *phiconSmall,
                           UINT    nIconSize)
{
    return S_FALSE;
}

HRESULT CTemplateUIObj::Create(REFCLSID rclsid, REFIID riid, LPVOID* ppvOut)
{
    CTemplateUIObj *pti = new CTemplateUIObj(rclsid);
    HRESULT hres;
    if (pti) {
        hres = pti->QueryInterface(riid, ppvOut);
        pti->Release();
        return hres;
    }

    *ppvOut=NULL;
    return E_OUTOFMEMORY;
}

//==========================================================================
// CTemplateUIObj members (IDataObject override)
//==========================================================================

HRESULT CTemplateUIObj::_CreateInstance(IStorage* pstg)
{
    HRESULT hres;
    IPersistStorage* pps = NULL;
    hres = OleCreate(_clsid, IID_IPersistStorage, OLERENDER_DRAW,
                     NULL, NULL, pstg, (LPVOID*)&pps);
    DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI OleCreate returned (%x)"), hres);

    if (SUCCEEDED(hres))
    {
        hres = OleSave(pps, pstg, TRUE);
        DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI OleSave returned (%x)"), hres);
        pps->HandsOffStorage();
        pps->Release();

        if (SUCCEEDED(hres))
        {
            hres = pstg->Commit(STGC_OVERWRITE);
            DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI pstg->Commit returned (%x)"), hres);
        }
    }

    return hres;
}

HRESULT CTemplateUIObj::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = DATA_E_FORMATETC;

    pmedium->pUnkForRelease = NULL;
    pmedium->pstg = NULL;

    //
    // NOTES: We should avoid calling _OpenStorage if we don't support
    //  the format.
    //

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE)
    {
        IStorage* pstg = NULL;
        hres = StgCreateDocfile(NULL, STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pstg);
        DebugMsg(DM_TRACE, TEXT("so TR - TUO:GD StgCreateDocfile returned (%x)"), hres);
        if (SUCCEEDED(hres))
        {
            hres = _CreateInstance(pstg);
            if (SUCCEEDED(hres)) {
                pmedium->tymed = TYMED_ISTORAGE;
                pmedium->pstg = pstg;
            } else {
                pstg->Release();
            }
        }
    }
    else if (pformatetcIn->cfFormat == CF_OBJECTDESCRIPTOR
	&& pformatetcIn->tymed == TYMED_HGLOBAL)
    {
	DebugMsg(DM_TRACE, TEXT("so TR - TUO:GD cfFormat==CF_OBJECTDESCRIPTOR"));

	static WCHAR szUserTypeName[] = L"Foo";	// BUGBUG
	static WCHAR szSrcOfCopy[] = L"Bar";	// BUGBUG
	UINT cbUserTypeName = sizeof(szUserTypeName);
	UINT cbSrcOfCopy = sizeof(szSrcOfCopy);
	pmedium->hGlobal = GlobalAlloc(GPTR, sizeof(OBJECTDESCRIPTOR)+cbUserTypeName+cbSrcOfCopy);
	if (pmedium->hGlobal)
	{
	    OBJECTDESCRIPTOR* podsc = (OBJECTDESCRIPTOR*)pmedium->hGlobal;
	    podsc->cbSize = sizeof(OBJECTDESCRIPTOR);
	    podsc->clsid = _clsid;
	    podsc->dwDrawAspect = 0; // The source does not draw the object
	    // podsc->sizel = { 0, 0 }; // The source does not draw the object
	    // podsc->pointl = { 0, 0 };
	    podsc->dwStatus = 0; // BUGBUG: read it from registry! CLSID/MiscStatus
	    podsc->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
	    podsc->dwSrcOfCopy = sizeof(OBJECTDESCRIPTOR)+cbUserTypeName;
	    LPBYTE pbT = (LPBYTE)(podsc+1);
            lstrcpyW((LPWSTR)pbT, szUserTypeName);
            lstrcpyW((LPWSTR)(pbT+cbUserTypeName), szSrcOfCopy);
	    Assert(pbT == ((LPBYTE)podsc)+podsc->dwFullUserTypeName);
	    Assert(pbT+cbUserTypeName == ((LPBYTE)podsc)+podsc->dwSrcOfCopy);

	    pmedium->tymed = TYMED_HGLOBAL;
	    hres = S_OK;
	}
	else
	{
	    hres = E_OUTOFMEMORY;
	}
    }
    return hres;
}

HRESULT CTemplateUIObj::GetDataHere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = DATA_E_FORMATETC;

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && pmedium->tymed == TYMED_ISTORAGE)
    {
        hres = _CreateInstance(pmedium->pstg);
    }

    return hres;
}

HRESULT CTemplateUIObj::QueryGetData(LPFORMATETC pformatetcIn)
{
    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE)
    {
        return S_OK;
    }
    else if (pformatetcIn->cfFormat == CF_OBJECTDESCRIPTOR
	&& pformatetcIn->tymed == TYMED_HGLOBAL)
    {
	return S_OK;
    }
    return DATA_E_FORMATETC;
}

HRESULT CTemplateUIObj::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    //
    //  This is the simplest implemtation. It means we always return
    // the data in the format requested.
    //
    return ResultFromScode(DATA_S_SAMEFORMATETC);
}

HRESULT CTemplateUIObj::SetData(LPFORMATETC pformatetc, STGMEDIUM  * pmedium, BOOL fRelease)
{
    return E_FAIL;
}

HRESULT CTemplateUIObj::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc)
{
    static FORMATETC s_afmt[] = { { (CLIPFORMAT)CF_EMBEDDEDOBJECT }, {(CLIPFORMAT)CF_OBJECTDESCRIPTOR} };
    return SHCreateStdEnumFmtEtc(ARRAYSIZE(s_afmt), s_afmt, ppenumFormatEtc);
}

HRESULT CTemplateUIObj::DAdvise(FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD * pdwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CTemplateUIObj::DUnadvise(DWORD dwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CTemplateUIObj::EnumDAdvise(LPENUMSTATDATA * ppenumAdvise)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

#define TIDC_INVALID    -1
#define TIDC_COPY	0
#define TIDC_MAX	1

HRESULT CTemplateUIObj::QueryContextMenu(
                    HMENU hmenu,
                    UINT indexMenu,
                    UINT idCmdFirst,
                    UINT idCmdLast,
                    UINT uFlags)
{
    DebugMsg(DM_TRACE, TEXT("sc TR - CTUI::QCM called (uFlags=%x)"), uFlags);

    //
    // REVIEW: Checking CMF_DVFILE is subtle, need to be documented clearly!
    //
    if (!(uFlags & (CMF_VERBSONLY|CMF_DVFILE)))
    {
	MENUITEMINFO mii = {
	    sizeof(MENUITEMINFO),
	    MIIM_STATE|MIIM_ID|MIIM_TYPE,
	    MFT_STRING,
	    MFS_DEFAULT,
	    idCmdFirst+TIDC_COPY,
	    NULL, NULL, NULL, 0,
	    TEXT("&Copy"),	// BUGBUG: NLS
	    5
	};
	InsertMenuItem(hmenu, indexMenu++, TRUE, &mii);
	// InsertMenu(hmenu, indexMenu++, MF_BYPOSITION|MF_DEFAULT, idCmdFirst+TIDC_COPY, "&Copy"); // BUGBUG: NLS
    }
    return ResultFromShort(TIDC_MAX);
}

HRESULT CTemplateUIObj::InvokeCommand(
                 LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hres;
    DebugMsg(DM_TRACE, TEXT("sc TR - CTUI::IC called (%x)"), lpici->lpVerb);
    int idCmd = TIDC_INVALID;

    if (HIWORD(lpici->lpVerb))
    {
	if (lstrcmpiA(lpici->lpVerb, "copy") == 0) {
	    idCmd = TIDC_COPY;
	}
    }
    else
    {
	idCmd = LOWORD(lpici->lpVerb);
    }

    switch(idCmd)
    {
    case TIDC_COPY:
	hres = OleSetClipboard(this);
	break;

    default:
	hres = E_INVALIDARG;
	break;
    }

    return hres;
}

HRESULT CTemplateUIObj::GetCommandString(
                                UINT        idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax)
{
    DebugMsg(DM_TRACE, TEXT("sc TR - CTUI::GCS called (%d, %x)"), idCmd, uType);
    return E_NOTIMPL;
}

#endif // FEATURE_SHELLEXTENSION
