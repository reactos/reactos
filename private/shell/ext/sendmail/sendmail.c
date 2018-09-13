#include "sendmail.h"       // pch file
#include "resource.h"
#include "shlguidp.h"
#include "debug.h"

// these bits are set by the user (holding down the keys) durring drag drop,
// but more importantly, they are set in the SimulateDragDrop() call that the
// browser implements to get the "Send Page..." vs "Send Link..." feature

#define IS_FORCE_LINK(grfKeyState)   ((grfKeyState == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) || \
                                      (grfKeyState == (MK_LBUTTON | MK_ALT)))
#define IS_FORCE_COPY(grfKeyState)   (grfKeyState == (MK_LBUTTON | MK_CONTROL))


STDAPI DesktopShortcutDropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect);

UINT g_cfShellURL = 0;
UINT g_cfFileContents = 0;
UINT g_cfFileDescA = 0;
UINT g_cfFileDescW = 0;
UINT g_cfHIDA = 0;

BOOL RunningOnNT()
{
    static int s_bOnNT = -1;  // -1 means uninited, 0 means no, 1 means yes
    if (s_bOnNT == -1) 
    {
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
            s_bOnNT = 1;
        else 
            s_bOnNT = 0;
    }
    return (BOOL)s_bOnNT;
}

// thunks // {

// from browseui/runonnt.c
#define g_fRunningOnNT  RunningOnNT()
int _AorW_PathCleanupSpec(/*IN OPTIONAL*/ LPCTSTR pszDir, /*IN OUT*/ LPTSTR pszSpec)
{
    //THUNKMSG(TEXT("PathCleanupSpec"));

    if (g_fRunningOnNT)
    {
        WCHAR wzDir[MAX_PATH];
        WCHAR wzSpec[MAX_PATH];
        LPWSTR pwszDir = wzDir;
        int iRet;

        if (pszDir)
            SHTCharToUnicode(pszDir, wzDir, ARRAYSIZE(wzDir));
        else
            pwszDir = NULL;

        SHTCharToUnicode(pszSpec, wzSpec, ARRAYSIZE(wzSpec));
        iRet = PathCleanupSpec((LPTSTR)pwszDir, (LPTSTR)wzSpec);

        SHUnicodeToTChar(wzSpec, pszSpec, MAX_PATH);
        return iRet;
    }
    else
    {
        CHAR szDir[MAX_PATH];
        CHAR szSpec[MAX_PATH];
        LPSTR pszDir2 = szDir;
        int iRet;

        if (pszDir)
            SHTCharToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        else
            pszDir2 = NULL;

        SHTCharToAnsi(pszSpec, szSpec, ARRAYSIZE(szSpec));
        iRet = PathCleanupSpec((LPTSTR)pszDir2, (LPTSTR)szSpec);

        SHAnsiToTChar(szSpec, pszSpec, MAX_PATH);
        return iRet;
    }
}

#define PathCleanupSpec(s1, s2) _AorW_PathCleanupSpec(s1, s2)

// from shell32/copyfgd.cpp // {

// thunk A/W funciton to access A/W FILEGROUPDESCRIPTOR
// this relies on the fact that the first part of the A/W structures are
// identical. only the string buffer part is different. so all accesses to the
// cFileName field need to go through this function.
//

FILEDESCRIPTOR *GetFileDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, int nIndex, LPTSTR pszName)
{
    if (fUnicode)
    {
        // Yes, so grab the data because it matches.
        FILEGROUPDESCRIPTORW * pfgdW = (FILEGROUPDESCRIPTORW *)pfgd;    // cast to what this really is
        if (pszName)
            SHUnicodeToTChar(pfgdW->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdW->fgd[nIndex];   // cast assume the non string parts are the same!
    }
    else
    {
        FILEGROUPDESCRIPTORA *pfgdA = (FILEGROUPDESCRIPTORA *)pfgd;     // cast to what this really is

        if (pszName)
            SHAnsiToTChar(pfgdA->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdA->fgd[nIndex];   // cast assume the non string parts are the same!
    }
}

// }

// }

//
// our own impl since URLMON IStream::CopyTo is busted, danpoz will be fixing this
//
HRESULT IStream_CopyTo(IStream *pstmFrom, IStream *pstmTo, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    BYTE buf[512];
    ULONG cbRead;
    HRESULT hres = NOERROR;

    if (pcbRead)
    {
        pcbRead->LowPart = 0;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten)
    {
        pcbWritten->LowPart = 0;
        pcbWritten->HighPart = 0;
    }

    ASSERT(cb.HighPart == 0);

    while (cb.LowPart)
    {
        hres = pstmFrom->lpVtbl->Read(pstmFrom, buf, min(cb.LowPart, SIZEOF(buf)), &cbRead);

        if (pcbRead)
            pcbRead->LowPart += cbRead;

        if (FAILED(hres) || (cbRead == 0))
            break;

        cb.LowPart -= cbRead;

        hres = pstmTo->lpVtbl->Write(pstmTo, buf, cbRead, &cbRead);

        if (pcbWritten)
            pcbWritten->LowPart += cbRead;

        if (FAILED(hres) || (cbRead == 0))
            break;
    }

    return hres;
}

STDMETHODIMP DropHandler_QueryInterface(IDropTarget *pdropt, REFIID riid, void **ppv)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
        *ppv = &this->dt;
    }
    else if (IsEqualIID(riid, &IID_IPersistFile))
    {
        *ppv = &this->pf;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppv = &this->sxi;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    // pdropt->lpVtbl->AddRef(pdropt);
    return S_OK;
}

STDMETHODIMP_(ULONG) DropHandler_AddRef(IDropTarget *pdropt)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);
    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) DropHandler_Release(IDropTarget *pdropt)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);

    this->cRef--;
    if (this->cRef > 0)
            return this->cRef;

    LocalFree((HLOCAL)this);

    DllRelease();

    return 0;
}

STDMETHODIMP DropHandler_DragEnter(IDropTarget *pdropt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);

    TraceMsg(DM_TRACE, "DropHandler_DragEnter");
    this->grfKeyStateLast = grfKeyState;
    this->dwEffectLast = *pdwEffect;

    return S_OK;
}

STDMETHODIMP DropHandler_DragOver(IDropTarget *pdropt, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);

    *pdwEffect &= ~DROPEFFECT_MOVE;

    if (IS_FORCE_COPY(grfKeyState))
        *pdwEffect &= DROPEFFECT_COPY;
    else if (IS_FORCE_LINK(grfKeyState))
        *pdwEffect &= DROPEFFECT_LINK;

    this->grfKeyStateLast = grfKeyState;
    this->dwEffectLast = *pdwEffect;

    return S_OK;
}

STDMETHODIMP DropHandler_DragLeave(IDropTarget *pdropt)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);
    return S_OK;
}

STDMETHODIMP DropHandler_Drop(IDropTarget *pdropt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CDropHandler *this = IToClass(CDropHandler, dt, pdropt);
    HRESULT hres = this->pfnDrop(pdtobj, this->grfKeyStateLast, this->dwEffectLast);
    
    *pdwEffect = DROPEFFECT_COPY;   // don't let source delete data
    return hres;
}

const IDropTargetVtbl c_DropHandler_DTVtbl =
{
    DropHandler_QueryInterface, DropHandler_AddRef, DropHandler_Release,
    DropHandler_DragEnter,
    DropHandler_DragOver,
    DropHandler_DragLeave,
    DropHandler_Drop,
};

STDMETHODIMP DropHandler_PF_QueryInterface(IPersistFile *ppf, REFIID riid, void **ppv)
{
    CDropHandler *this = IToClass(CDropHandler, pf, ppf);
    return DropHandler_QueryInterface(&this->dt, riid, ppv);
}

STDMETHODIMP_(ULONG) DropHandler_PF_AddRef(IPersistFile *ppf)
{
    CDropHandler *this = IToClass(CDropHandler, pf, ppf);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) DropHandler_PF_Release(IPersistFile *ppf)
{
    CDropHandler *this = IToClass(CDropHandler, pf, ppf);
    return DropHandler_Release(&this->dt);
}

STDMETHODIMP DropHandler_GetClassID(IPersistFile *ppf, CLSID *pClassID)
{
    *pClassID = CLSID_MailRecipient;
    return S_OK;
}

STDMETHODIMP DropHandler_IsDirty(IPersistFile *psf)
{
    return S_FALSE;
}

STDMETHODIMP DropHandler_Load(IPersistFile *psf, LPCOLESTR pwszFile, DWORD grfMode)
{
    TraceMsg(DM_TRACE, "DropHandler_Load");

    return S_OK;
}

STDMETHODIMP DropHandler_Save(IPersistFile *psf, LPCOLESTR pwszFile, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP DropHandler_SaveCompleted(IPersistFile *psf, LPCOLESTR pwszFile)
{
    return S_OK;
}

STDMETHODIMP DropHandler_GetCurFile(IPersistFile *psf, LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;
    return S_OK;
}

const IPersistFileVtbl c_DropHandler_PFVtbl = {
    DropHandler_PF_QueryInterface, DropHandler_PF_AddRef, DropHandler_PF_Release,
    DropHandler_GetClassID,
    DropHandler_IsDirty,
    DropHandler_Load,
    DropHandler_Save,
    DropHandler_SaveCompleted,
    DropHandler_GetCurFile
};


STDMETHODIMP DropHandler_SXI_QueryInterface(IShellExtInit *psxi, REFIID riid, void **ppv)
{
    CDropHandler *this = IToClass(CDropHandler, sxi, psxi);
    return DropHandler_QueryInterface(&this->dt, riid, ppv);
}

STDMETHODIMP_(ULONG) DropHandler_SXI_AddRef(IShellExtInit *psxi)
{
    CDropHandler *this = IToClass(CDropHandler, sxi, psxi);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) DropHandler_SXI_Release(IShellExtInit *psxi)
{
    CDropHandler *this = IToClass(CDropHandler, sxi, psxi);
    return DropHandler_Release(&this->dt);
}

STDMETHODIMP DropHandler_SXI_Initialize(IShellExtInit *psxi, LPCITEMIDLIST pidl, IDataObject *pdtobj, HKEY hkeyProgID)
{
    CDropHandler *this = IToClass(CDropHandler, sxi, psxi);

    TraceMsg(DM_TRACE, "DropHandler_SXI_Initialize");

    return S_OK;
}

IShellExtInitVtbl c_DropHandler_SXIVtbl = {
    DropHandler_SXI_QueryInterface, DropHandler_SXI_AddRef, DropHandler_SXI_Release,
    DropHandler_SXI_Initialize
};


STDAPI DropHandler_CreateInstance(LPDROPPROC pfnDrop, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hres;
    CDropHandler *this;

    *ppv = NULL;                // assume error

    TraceMsg(DM_TRACE, "DropHandler_CreateInstance");

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    this = (CDropHandler *)LocalAlloc(LPTR, sizeof(CDropHandler));
    if (this)
    {
        this->dt.lpVtbl = &c_DropHandler_DTVtbl;
        this->sxi.lpVtbl = &c_DropHandler_SXIVtbl;
        this->pf.lpVtbl = &c_DropHandler_PFVtbl;
        this->pfnDrop = pfnDrop;

        this->cRef = 1;

        DllAddRef();

        hres = DropHandler_QueryInterface(&this->dt, riid, ppv);
        DropHandler_Release(&this->dt);
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkSetPath(IUnknown *punk, LPCTSTR pszPath)
{
    HRESULT hres;
#ifdef UNICODE
    IShellLinkW *pslW;
    hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellLinkW, (void **)&pslW);
    if (SUCCEEDED(hres))
    {
        hres = pslW->lpVtbl->SetPath(pslW, pszPath);
        pslW->lpVtbl->Release(pslW);
    }
    else
#endif
    {
        IShellLinkA *pslA;
        hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellLinkA, (void **)&pslA);
        if (SUCCEEDED(hres))
        {
            CHAR szPath[MAX_PATH];
            SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
            hres = pslA->lpVtbl->SetPath(pslA, szPath);
            pslA->lpVtbl->Release(pslA);
        }
    }
    return hres;
}

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkGetPath(IUnknown *punk, LPTSTR pszPath, UINT cch)
{
    HRESULT hres;
#ifdef UNICODE
    IShellLinkW *pslW;
    hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellLinkW, (void **)&pslW);
    if (SUCCEEDED(hres))
    {
        hres = pslW->lpVtbl->GetPath(pslW, pszPath, cch, NULL, SLGP_UNCPRIORITY);
        pslW->lpVtbl->Release(pslW);
    }
    else
#endif
    {
        IShellLinkA *pslA;
        hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellLinkA, (void **)&pslA);
        if (SUCCEEDED(hres))
        {
            CHAR szPath[MAX_PATH];
            hres = pslA->lpVtbl->GetPath(pslA, szPath, ARRAYSIZE(szPath), NULL, SLGP_UNCPRIORITY);
            if (SUCCEEDED(hres))
                SHAnsiToUnicode(szPath, pszPath, cch);
            pslA->lpVtbl->Release(pslA);
        }
    }
    return hres;
}

HRESULT _CreateShortcutToPath(LPCTSTR pszPath, LPCTSTR pszTarget)
{
    IUnknown *punk;
    HRESULT hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, &punk);
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;

        ShellLinkSetPath(punk, pszTarget);

        hres = punk->lpVtbl->QueryInterface(punk, &IID_IPersistFile, &ppf);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            hres = ppf->lpVtbl->Save(ppf, wszPath, TRUE);
            ppf->lpVtbl->Release(ppf);
        }
        punk->lpVtbl->Release(punk);
    }
    return hres;
}

BOOL _IsShortcut(LPCTSTR pszFile)
{
    SHFILEINFO sfi;

    return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi),
            SHGFI_ATTRIBUTES) && (sfi.dwAttributes & SFGAO_LINK);
}

// create a temporary shortcut to a file
// BUGBUG: Colision is not handled here
BOOL _CreateTempFileShortcut(LPCTSTR pszTarget, LPTSTR pszShortcut)
{
    TCHAR szShortcutPath[MAX_PATH + 1];
    BOOL bSuccess = FALSE;
    
    if (GetTempPath(ARRAYSIZE(szShortcutPath), szShortcutPath))
    {
        PathAppend(szShortcutPath, PathFindFileName(pszTarget));

        if (_IsShortcut(pszTarget))
        {
            TCHAR szTarget[MAX_PATH + 1];
            SHFILEOPSTRUCT shop = {0};
            shop.wFunc = FO_COPY;
            shop.pFrom = szTarget;
            shop.pTo = szShortcutPath;
            shop.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            StrCpyN(szTarget, pszTarget, ARRAYSIZE(szTarget));
            szTarget[lstrlen(szTarget) + 1] = TEXT('\0');

            szShortcutPath[lstrlen(szShortcutPath) + 1] = TEXT('\0');

            bSuccess = (0 ==  SHFileOperation(&shop));
        }
        else
        {
            PathRenameExtension(szShortcutPath, TEXT(".lnk"));
            bSuccess = SUCCEEDED(_CreateShortcutToPath(szShortcutPath, pszTarget));
        }

        if (bSuccess)
            lstrcpyn(pszShortcut, szShortcutPath, MAX_PATH);
    }
    return bSuccess;
} 

BOOL AllocatePMP(MRPARAM *pmp, DWORD cchTitle, DWORD cchFiles)
{
    pmp->pszTitle = GlobalAlloc(GPTR, cchTitle * SIZEOF(TCHAR));
    if (!pmp->pszTitle) 
        return FALSE;
    
    pmp->pszFiles = GlobalAlloc(GPTR, cchFiles * SIZEOF(TCHAR));
    if (!pmp->pszFiles)
        return FALSE;
    
    Assert(pmp->pszTitle[cchTitle-1] == 0);
    Assert(pmp->pszFiles[cchFiles-1] == 0);

    return TRUE;
}

void DeleteMultipleFiles(LPCTSTR pszFiles)
{
    SHFILEOPSTRUCT shop = {0};
    shop.wFunc = FO_DELETE;
    shop.pFrom = pszFiles;  // This is already double null terminated.
    shop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

    SHFileOperation(&shop);
}

BOOL CleanupPMP(MRPARAM *pmp)
{
    if (pmp->dwFlags & MRPARAM_DELETEFILE)
        DeleteMultipleFiles(pmp->pszFiles);

    if (pmp->pszFiles)
    {
        GlobalFree((LPVOID)pmp->pszFiles);
        pmp->pszFiles = NULL;
    }

    if (pmp->pszTitle)
    {
        GlobalFree((LPVOID)pmp->pszTitle);
        pmp->pszTitle = NULL;
    }

    GlobalFree(pmp);
    return TRUE;
}

HRESULT _GetFileNameFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, LPTSTR pszDescription)
{
    STGMEDIUM medium;
    HRESULT hres = pdtobj->lpVtbl->GetData(pdtobj, pfmtetc, &medium);
    if (SUCCEEDED(hres))
    {
        // NOTE: this is a TCHAR format, we depend on how we are compiled, we really
        // should test both the A and W formats
        FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(medium.hGlobal);
        if (pfgd)
        {
            TCHAR szFdName[MAX_PATH];       // pfd->cFileName
            FILEDESCRIPTOR *pfd;

            // &pfgd->fgd[0], w/ thunk
            ASSERT(pfmtetc->cfFormat == g_cfFileDescW
              || pfmtetc->cfFormat == g_cfFileDescA);
            // for now, all callers are ANSI (other untested)
            //ASSERT(pfmtetc->cfFormat == g_cfFileDescA);
            pfd = GetFileDescriptor(pfgd, pfmtetc->cfFormat == g_cfFileDescW, 0, szFdName);

            lstrcpy(pszDescription, szFdName);      // pfd->cFileName

            GlobalUnlock(medium.hGlobal);
            hres = S_OK;
        }
        ReleaseStgMedium(&medium);
    }
    return hres;
}

// construct a nice title "<File Name> (<File Type>)"

void _GetFileAndTypeDescFromPath(LPCTSTR pszPath, LPTSTR pszDesc)
{
    SHFILEINFO sfi;

    if (!SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_DISPLAYNAME))
    {
        lstrcpyn(sfi.szDisplayName, PathFindFileName(pszPath), ARRAYSIZE(sfi.szDisplayName));
        sfi.szTypeName[0] = 0;
    }

    lstrcpyn(pszDesc, sfi.szDisplayName, MAX_PATH);
}

/*
 * pcszURL -> "ftp://ftp.microsoft.com"
 * pcszPath -> "c:\windows\desktop\internet\Microsoft FTP.url"
 */
HRESULT CreateNewURLShortcut(LPCTSTR pcszURL, LPCTSTR pcszURLFile)
{
    IUniformResourceLocator *purl;
    HRESULT hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUniformResourceLocator, (void **)&purl);
    if (SUCCEEDED(hr))
    {
        hr = purl->lpVtbl->SetURL(purl, pcszURL, 0);
        if (SUCCEEDED(hr))
        {
            IPersistFile *ppf;
            hr = purl->lpVtbl->QueryInterface(purl, &IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hr))
            {
                WCHAR wszFile[INTERNET_MAX_URL_LENGTH];
                SHTCharToUnicode(pcszURLFile, wszFile, ARRAYSIZE(wszFile));

                hr = ppf->lpVtbl->Save(ppf, wszFile, TRUE);
                ppf->lpVtbl->Release(ppf);
            }
        }
        purl->lpVtbl->Release(purl);
    }
    return hr;
}

HRESULT _CreateURLFileToSend(IDataObject *pdtobj, MRPARAM *pmp)
{
    HRESULT hr = CreateNewURLShortcut(pmp->pszTitle, pmp->pszFiles);
    if (SUCCEEDED(hr))
    {
        _GetFileAndTypeDescFromPath(pmp->pszFiles, pmp->pszTitle);

        pmp->dwFlags |= MRPARAM_DELETEFILE;
    }    
    return hr;
}

// First undefine everything that we are intercepting as to not forward back to us...
#undef SHGetSpecialFolderPath

// Explicit prototype because only the A/W prototypes exist in the headers
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPTSTR lpszPath, int nFolder, BOOL fCreate);

BOOL _SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    BOOL fRet;

    if (RunningOnNT())
    {
#ifdef UNICODE
        fRet = SHGetSpecialFolderPath(hwnd, pszPath, nFolder, fCreate);
#else
        WCHAR wszPath[MAX_PATH];
        fRet = SHGetSpecialFolderPath(hwnd, (LPTSTR)wszPath, nFolder, fCreate);
        if (fRet)
            SHUnicodeToTChar(wszPath, pszPath, MAX_PATH);
#endif
    }
    else
    {
#ifdef UNICODE
        CHAR szPath[MAX_PATH];
        fRet = SHGetSpecialFolderPath(hwnd, (LPTSTR)szPath, nFolder, fCreate);
        if (fRet)
            SHAnsiToTChar(szPath, pszPath, MAX_PATH);
#else
        fRet = SHGetSpecialFolderPath(hwnd, pszPath, nFolder, fCreate);
#endif
    }
    return fRet;
}

BOOL PathYetAnotherMakeUniqueNameT(LPTSTR  pszUniqueName,
                                   LPCTSTR pszPath,
                                   LPCTSTR pszShort,
                                   LPCTSTR pszFileSpec)
{
    if (RunningOnNT())
    {
        WCHAR wszUniqueName[MAX_PATH];
        WCHAR wszPath[MAX_PATH];
        WCHAR wszShort[32];
        WCHAR wszFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
        pszPath = (LPCTSTR)wszPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToUnicode(pszShort, wszShort, ARRAYSIZE(wszShort));
            pszShort = (LPCTSTR)wszShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToUnicode(pszFileSpec, wszFileSpec, ARRAYSIZE(wszFileSpec));
            pszFileSpec = (LPCTSTR)wszFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)wszUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHUnicodeToTChar(wszUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
    else {
        // win9x thunk code from runonnt.c
        CHAR szUniqueName[MAX_PATH];
        CHAR szPath[MAX_PATH];
        CHAR szShort[32];
        CHAR szFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToAnsi(pszShort, szShort, ARRAYSIZE(szShort));
            pszShort = (LPCTSTR)szShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToAnsi(pszFileSpec, szFileSpec, ARRAYSIZE(szFileSpec));
            pszFileSpec = (LPCTSTR)szFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)szUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHAnsiToTChar(szUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
}

HRESULT _GetHDROPFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hres = E_FAIL;
    TCHAR szPath[MAX_PATH], szDesc[MAX_PATH];

    pmp->nFiles = DragQueryFile(pmedium->hGlobal, -1, NULL, 0);

    if (pmp->nFiles && AllocatePMP(pmp, MAX_PATH * pmp->nFiles, MAX_PATH * pmp->nFiles))
    {
        int i;
        BOOL bAllTempLinks = TRUE;
        LPTSTR pszFile, pszTitle;
        for (i = 0, pszFile = pmp->pszFiles, pszTitle = pmp->pszTitle; DragQueryFile(pmedium->hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
        {
            if (IS_FORCE_LINK(grfKeyState) || PathIsDirectory(szPath))
            {
                // Want to send a link even for the real file, we will create links to the real files
                // and send it.
                _CreateTempFileShortcut(szPath, pszFile);
            }
            else
            {
                bAllTempLinks = FALSE;
                lstrcpy(pszFile, szPath);
            }

            _GetFileAndTypeDescFromPath(pszFile, szDesc);

            // This is used to separate the names
            if (pszTitle != pmp->pszTitle)
                *pszTitle++ = TEXT(';');

            lstrcpy(pszTitle, szDesc);
            pszTitle += lstrlen(pszTitle);      // ";" seperated string

            pszFile[lstrlen(pszFile) + 1] = TEXT('\0'); // Double Null terminate
            pszFile += lstrlen(pszFile) + 1;    // dbl null string
        }

        if (bAllTempLinks)
            pmp->dwFlags |= MRPARAM_DELETEFILE;

        hres = S_OK;
    }
    return hres;
}

// "Uniform Resource Locator" format

HRESULT _GetURLFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hres = E_FAIL;

    // This DataObj is from the internet
    // NOTE: We only allow to send one file here.
    pmp->nFiles = 1;
    if (AllocatePMP(pmp, INTERNET_MAX_URL_LENGTH, MAX_PATH))
    {
        // n.b. STR not TSTR!  since URLs only support ansi
        //lstrcpyn(pmp->pszTitle, (LPSTR)GlobalLock(pmedium->hGlobal), INTERNET_MAX_URL_LENGTH);
        SHAnsiToTChar((LPSTR)GlobalLock(pmedium->hGlobal), pmp->pszTitle, INTERNET_MAX_URL_LENGTH);
        GlobalUnlock(pmedium->hGlobal);
        
        if (pmp->pszTitle[0])
        {
            // Note some of these functions depend on which OS we
            // are running on to know if we should pass ansi or unicode strings
            // to it
            // Windows 95
            if (GetTempPath(MAX_PATH, pmp->pszFiles))
            {
                TCHAR szFileName[MAX_PATH];
                // it's an URL, which is always ANSI, but the filename
                // can still be wide (?)
                FORMATETC fmteW = {(CLIPFORMAT)g_cfFileDescW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                FORMATETC fmteA = {(CLIPFORMAT)g_cfFileDescA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                
// #ifdef UNICODE
                if (FAILED(_GetFileNameFromData(pdtobj, &fmteW, szFileName)))
// #endif
                    if (FAILED(_GetFileNameFromData(pdtobj, &fmteA, szFileName)))
                        LoadString(g_hinst, IDS_SENDMAIL_URL_FILENAME, szFileName, ARRAYSIZE(szFileName));

                PathCleanupSpec(pmp->pszFiles, szFileName);

                hres = _CreateURLFileToSend(pdtobj, pmp);
            }
        }
    }
    return hres;
}



// transfer FILECONTENTS/FILEGROUPDESCRIPTOR data to a temp file
// then send that in mail


HRESULT _GetFileContentsFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hres = E_FAIL;

    // NOTE: We only allow to send one file here.
    pmp->nFiles = 1;
    if (AllocatePMP(pmp, INTERNET_MAX_URL_LENGTH, MAX_PATH))
    {
        FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(pmedium->hGlobal);
        if (pfgd)
        {
            TCHAR szFdName[MAX_PATH];       // pfd->cFileName
            FILEDESCRIPTOR *pfd;

            // &pfgd->fgd[0], w/ thunk
            ASSERT(pfmtetc->cfFormat == g_cfFileDescW
              || pfmtetc->cfFormat == g_cfFileDescA);
            pfd = GetFileDescriptor(pfgd, pfmtetc->cfFormat == g_cfFileDescW, 0, szFdName);

            if (GetTempPath(MAX_PATH, pmp->pszFiles))
            {
                STGMEDIUM medium;
                FORMATETC fmte = {(CLIPFORMAT)g_cfFileContents, NULL, pfmtetc->dwAspect, 0, TYMED_ISTREAM | TYMED_HGLOBAL};
                hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium);
                if (SUCCEEDED(hres))
                {
                    IStream *pstmFile;

                    PathAppend(pmp->pszFiles, szFdName);    // pfd->cFileName
                    PathCleanupSpec(pmp->pszFiles, PathFindFileName(pmp->pszFiles));
                    PathYetAnotherMakeUniqueNameT(pmp->pszFiles, pmp->pszFiles, NULL, NULL);

                    hres = SHCreateStreamOnFile(pmp->pszFiles, STGM_WRITE | STGM_CREATE, &pstmFile);
                    if (SUCCEEDED(hres))
                    {
                        const ULARGE_INTEGER li = {-1, 0};   // the whole thing

                        switch (medium.tymed) 
                        {
                        case TYMED_ISTREAM:
                            hres = IStream_CopyTo(medium.pstm, pstmFile, li, NULL, NULL);
                            break;

                        case TYMED_HGLOBAL:
                            hres = pstmFile->lpVtbl->Write(pstmFile, GlobalLock(medium.hGlobal), 
                                pfd->dwFlags & FD_FILESIZE ? pfd->nFileSizeLow : (DWORD)GlobalSize(medium.hGlobal),
                                NULL);
                            GlobalUnlock(medium.hGlobal);
                            break;

                        default:
                            hres = E_FAIL;
                        }
                        pstmFile->lpVtbl->Release(pstmFile);
                        if (FAILED(hres))
                            DeleteFile(pmp->pszFiles);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
            GlobalUnlock(pmedium->hGlobal);
        }
    }

    if (SUCCEEDED(hres))
    {
        _GetFileAndTypeDescFromPath(pmp->pszFiles, pmp->pszTitle);

        pmp->dwFlags |= MRPARAM_DELETEFILE;

        if (pfmtetc->dwAspect == DVASPECT_COPY)
        {
            IQueryCodePage *pqcp;

            pmp->dwFlags |= MRPARAM_DOC;    // we are sending the document

            // get the code page if there is one
            if (SUCCEEDED(pdtobj->lpVtbl->QueryInterface(pdtobj, &IID_IQueryCodePage, (void **)&pqcp)))
            {
                if (SUCCEEDED(pqcp->lpVtbl->GetCodePage(pqcp, &pmp->uiCodePage)))
                    pmp->dwFlags |= MRPARAM_USECODEPAGE;
                pqcp->lpVtbl->Release(pqcp);
            }
        }
    }
    else if (pfmtetc->dwAspect == DVASPECT_COPY)
    {
        TCHAR szFailureMsg[MAX_PATH], szFailureMsgTitle[40];
        int iRet;
        LoadString(g_hinst, IDS_SENDMAIL_FAILUREMSG, szFailureMsg, ARRAYSIZE(szFailureMsg));
        LoadString(g_hinst, IDS_SENDMAIL_FAILUREMSGTITLE, szFailureMsgTitle, ARRAYSIZE(szFailureMsgTitle));
                    
        iRet = MessageBox(NULL, szFailureMsg, szFailureMsgTitle, MB_YESNO);
        if (iRet == IDNO)
            hres = S_FALSE;     // convert to success to we don't try DVASPECT_LINK
    }

    return hres;
}

typedef struct {
    HRESULT (*pfnCreateFromData)(IDataObject *, FORMATETC *, STGMEDIUM *, DWORD, MRPARAM *);
    FORMATETC fmte;
} DATA_HANDLER;


HRESULT _CreateSendToFilesFromDataObj(IDataObject *pdtobj, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hres;
    DWORD dwAspectPrefered;
    IEnumFORMATETC *penum;

    if (g_cfShellURL == 0)
    {
        // URL is always ANSI
        g_cfShellURL = RegisterClipboardFormat(CFSTR_SHELLURL);
        g_cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
        g_cfFileDescA = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
// #ifdef UNICODE
        g_cfFileDescW = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
// #endif
    }

    if (IS_FORCE_COPY(grfKeyState))
        dwAspectPrefered = DVASPECT_COPY;
    else if (IS_FORCE_LINK(grfKeyState))
        dwAspectPrefered = DVASPECT_LINK;
    else
        dwAspectPrefered = DVASPECT_CONTENT;

    hres = pdtobj->lpVtbl->EnumFormatEtc(pdtobj, DATADIR_GET, &penum);
    if (SUCCEEDED(hres))
    {
        DATA_HANDLER rg_data_handlers[] = {
// #ifdef UNICODE
            _GetFileContentsFromData, {(CLIPFORMAT)g_cfFileDescW, NULL, dwAspectPrefered, -1, TYMED_HGLOBAL},
            _GetFileContentsFromData, {(CLIPFORMAT)g_cfFileDescW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
// #endif
            _GetFileContentsFromData, {(CLIPFORMAT)g_cfFileDescA, NULL, dwAspectPrefered, -1, TYMED_HGLOBAL},
            _GetFileContentsFromData, {(CLIPFORMAT)g_cfFileDescA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
            _GetHDROPFromData, {(CLIPFORMAT)CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
            _GetURLFromData, {(CLIPFORMAT)g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        };
        FORMATETC fmte;
        while (penum->lpVtbl->Next(penum, 1, &fmte, NULL) == S_OK)
        {
            int i;
            for (i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                if (rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat &&
                    rg_data_handlers[i].fmte.dwAspect == fmte.dwAspect)
                {
                    STGMEDIUM medium;
                    if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &rg_data_handlers[i].fmte, &medium)))
                    {
                        hres = rg_data_handlers[i].pfnCreateFromData(pdtobj, &fmte, &medium, grfKeyState, pmp);
                        ReleaseStgMedium(&medium);

                        if (SUCCEEDED(hres))
                            goto Done;
                    }
                }
            }
        }
Done:
        penum->lpVtbl->Release(penum);
    }
    return hres;
}

// send as mail support

// {9E56BE60-C50F-11CF-9A2C-00A0C90A90CE}
const GUID CLSID_MailRecipient = { 0x9E56BE60L, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE };
const GUID CLSID_DesktopShortcut = { 0x9E56BE61L, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE };

#include <mapi.h>

#define SIZEOF(x)   sizeof(x)       // has been checked for UNICODE correctness

//
// like OLE GetClassFile(), but it only works on ProgID\CLSID type registration
// not real doc files or pattern matched files
//

HRESULT _CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid)
{
    TCHAR szProgID[80];
    ULONG cb = SIZEOF(szProgID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szProgID, &cb) == ERROR_SUCCESS)
    {
        TCHAR szCLSID[80];

        lstrcat(szProgID, TEXT("\\CLSID"));
        cb = SIZEOF(szCLSID);

        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szCLSID, &cb) == ERROR_SUCCESS)
        {
            WCHAR wszCLSID[80];

            SHTCharToUnicode(szCLSID, wszCLSID, ARRAYSIZE(wszCLSID));

            return CLSIDFromString(wszCLSID, pclsid);
        }
    }
    return E_FAIL;
}

// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally
//

BOOL _GetShortcutTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch)
{
    IUnknown *punk;
    HRESULT hres;
    CLSID clsid;

    *pszTarget = 0;     // assume none

    if (!_IsShortcut(pszPath))
        return FALSE;

    if (FAILED(_CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, &punk);
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(punk->lpVtbl->QueryInterface(punk, &IID_IPersistFile, &ppf)))
        {
            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
            ppf->lpVtbl->Load(ppf, wszPath, 0);
            ppf->lpVtbl->Release(ppf);
        }
        hres = ShellLinkGetPath(punk, pszTarget, cch);
        punk->lpVtbl->Release(punk);
    }

    return FALSE;
}

#define MAIL_HANDLER    TEXT("Software\\Clients\\Mail")
#define MAIL_ATHENA_V1  TEXT("Internet Mail and News")
#define MAIL_ATHENA_V2  TEXT("Outlook Express")

BOOL GetDefaultMailHandler(LPTSTR pszMAPIDLL, DWORD cbMAPIDLL, BOOL *pbWantsCodePageInfo)
{
    TCHAR szDefaultProg[80];
    DWORD cb = SIZEOF(szDefaultProg);

    *pbWantsCodePageInfo = FALSE;

    *pszMAPIDLL = 0;
    if (ERROR_SUCCESS == SHRegGetUSValue(MAIL_HANDLER, TEXT(""), NULL, szDefaultProg, &cb, FALSE, NULL, 0))
    {
        HKEY hkey;
        TCHAR szProgKey[128];

        lstrcpy(szProgKey, MAIL_HANDLER TEXT("\\"));
        lstrcat(szProgKey, szDefaultProg);

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szProgKey, 
            0,  KEY_QUERY_VALUE,  &hkey))
        {
            // ugly, hard code this for OE
            *pbWantsCodePageInfo = (lstrcmpi(szDefaultProg, MAIL_ATHENA_V2) == 0);

            cb = cbMAPIDLL;
            if (ERROR_SUCCESS != SHQueryValueEx(hkey, TEXT("DLLPath"), 0, NULL, (LPBYTE)pszMAPIDLL, &cb))
            {
                if (lstrcmpi(szDefaultProg, MAIL_ATHENA_V1) == 0)
                {
                    lstrcpyn(pszMAPIDLL, TEXT("mailnews.dll"), cbMAPIDLL);
                }
            }
            RegCloseKey(hkey);
        }
    }
    return *pszMAPIDLL;
}

HMODULE LoadMailProvider(BOOL *pbWantsCodePageInfo)
{
    TCHAR szMAPIDLL[MAX_PATH];

    if (!GetDefaultMailHandler(szMAPIDLL, sizeof(szMAPIDLL), pbWantsCodePageInfo))
    {
        // read win.ini (bogus hu!) for mapi dll provider
        if (GetProfileString(TEXT("Mail"), TEXT("CMCDLLName32"), TEXT(""), szMAPIDLL, ARRAYSIZE(szMAPIDLL)) <= 0)
            lstrcpy(szMAPIDLL, TEXT("mapi32.dll"));
    }
    return LoadLibrary(szMAPIDLL);
}

typedef struct {
    TCHAR szTempShortcut[MAX_PATH];
    MapiMessage mm;
    MapiFileDesc mfd[0];
} MAPI_FILES;

int lstrzlen(int nStrs, LPCTSTR pszStrs)
{
    LPCTSTR psz;
    int i;

    for (i = 0, psz = pszStrs; ((i < nStrs) && (*psz)); psz += lstrlen(psz) + 1, i++)
        ;
    return (int)(psz - pszStrs + 1);
}


// SHPathToAnsi creates an ANSI version of a pathname.  If there is going to be a
// loss when converting from Unicode, the short pathname is obtained and stored in the 
// destination.  
//
// pszSrc  : Source buffer containing filename (of existing file) to be converted
// pszDest : Destination buffer to receive converted ANSI string.
// cbDest  : Size of the destination buffer, in bytes.
// 
// returns:
//      TRUE, the filename was converted without change
//      FALSE, we had to convert to short name
//

BOOL SHPathToAnsi(LPCTSTR pszSrc, LPSTR pszDest, int cbDest)
{
#ifdef UNICODE
    BOOL bUsedDefaultChar;
   
    WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, pszDest, cbDest, NULL, &bUsedDefaultChar);

    if (bUsedDefaultChar) 
    {  
        TCHAR szTemp[MAX_PATH];
        if (GetShortPathName(pszSrc, szTemp, ARRAYSIZE(szTemp)))
            SHTCharToAnsi(szTemp, pszDest, cbDest);
    }

    return !bUsedDefaultChar;
#else
    SHTCharToAnsi(pszSrc, pszDest, cbDest);
    return TRUE;
#endif

}


MAPI_FILES *_AllocMapiFiles(int nFiles, LPCTSTR pszFiles)
{
    MAPI_FILES *pmf;
    int n;

    n = SIZEOF(*pmf) + (nFiles * SIZEOF(pmf->mfd[0]));
    // buffer space *2 for DBCS
    pmf = GlobalAlloc(GPTR, n + (lstrzlen(nFiles, pszFiles) * 2)); 
    if (pmf)
    {
        pmf->mm.nFileCount = nFiles;
        if (nFiles)
        {
            int i;
            LPCTSTR psz;
            LPSTR pszA = (CHAR *)pmf + n;   // thunk buffer

            pmf->mm.lpFiles = pmf->mfd;

            for (i = 0, psz = pszFiles; ((i < nFiles) && (*psz));
              pszA += lstrlen(psz) + 1, psz += lstrlen(psz) + 1, i++)
            {
                // if the first item is a folder, we will create a shortcut to
                // that instead of trying to mail the folder (that MAPI does
                // not support)

                SHPathToAnsi(psz, pszA, (lstrlen(psz) * 2) + 1); // buffer space *2 for DBCS

                pmf->mfd[i].lpszPathName = pszA;
                pmf->mfd[i].lpszFileName = PathFindFileNameA(pszA);
                pmf->mfd[i].nPosition = (UINT)-1;
            }
        }
    }
    return pmf;
}

void _FreeMapiFiles(MAPI_FILES *pmf)
{
    if (pmf->szTempShortcut[0])
        DeleteFile(pmf->szTempShortcut);
    GlobalFree(pmf);
}

//
// pv is pointer to double null terminated file list
//

const TCHAR c_szPad[] = TEXT(" \r\n ");

STDAPI_(DWORD) MailRecipientThreadProc(void *pv)
{
    MRPARAM *pmp = (MRPARAM *)pv;
    MAPI_FILES *pmf;

    CoInitialize(NULL);     // we are going to do some COM stuff

    pmf = _AllocMapiFiles(pmp->nFiles, pmp->pszFiles);
    if (pmf)
    {
        TCHAR szText[2148];     // hold a URL/FilePath + some formatting text
        CHAR szTextA[2148];     // ...
        CHAR szTitleA[80];      // because the title is supposed to be non-const (and ansi)
        HMODULE hmodMail;
        BOOL bWantsCodePageInfo = FALSE;
        if (pmf->mm.nFileCount)
        {
            lstrcpy(szText, c_szPad);    // init the buffer with some stuff
            if (_IsShortcut(pmp->pszFiles))
                _GetShortcutTarget(pmp->pszFiles, szText + ARRAYSIZE(c_szPad) - 1, ARRAYSIZE(szText) - ARRAYSIZE(c_szPad));
            else
                lstrcpyn(szText + ARRAYSIZE(c_szPad) - 1, pmp->pszTitle, ARRAYSIZE(szText) - ARRAYSIZE(c_szPad));

            // Don't fill in lpszNoteText if we know we are sending 
            // documents because OE will puke on it 

            SHTCharToAnsi(szText, szTextA, ARRAYSIZE(szTextA));
            if (!(pmp->dwFlags & MRPARAM_DOC)) {
                pmf->mm.lpszNoteText = szTextA;
            }
            else
                Assert(pmf->mm.lpszNoteText == NULL);  

            if (pmp->pszTitle) 
            {
                //lstrcpyn(szTitle, pmp->pszTitle, ARRAYSIZE(szTitle));
                SHTCharToAnsi(pmp->pszTitle, szTitleA, ARRAYSIZE(szTitleA));
                pmf->mm.lpszSubject = szTitleA;
            } 
            else
                pmf->mm.lpszSubject = szTextA + ARRAYSIZE(c_szPad) - 1;
        }

        hmodMail = LoadMailProvider(&bWantsCodePageInfo);
        if (bWantsCodePageInfo && (pmp->dwFlags & MRPARAM_USECODEPAGE))
        {
            // When this flag is set, we know that we have just one file to send and we have a code page
            // Athena will then look at ulReserved for the code page
            // BUGBUG: Will the other MAPI handlers puke on this?  -- dli
            ASSERT(pmf->mm.nFileCount == 1);
            pmf->mfd[0].ulReserved = ((MRPARAM *)pmp)->uiCodePage;
        }
        
        if (hmodMail)
        {
            LPMAPISENDMAIL pfnSendMail = (LPMAPISENDMAIL)GetProcAddress(hmodMail, "MAPISendMail");
            if (pfnSendMail)
                pfnSendMail(0, 0, &pmf->mm, MAPI_LOGON_UI | MAPI_DIALOG, 0);

            FreeLibrary(hmodMail);
        }
        _FreeMapiFiles(pmf);
    }
    
    CleanupPMP(pmp);

    DllRelease();

    CoUninitialize();

    return 0;
}


STDAPI MailRecipientDropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect)
{
    MRPARAM *pmp = GlobalAlloc(GPTR, SIZEOF(*pmp));
    if (pmp)
    {        
        if (!pdtobj || SUCCEEDED(_CreateSendToFilesFromDataObj(pdtobj, grfKeyState, pmp)))
        {
            DllAddRef();

            if (SHCreateThread(MailRecipientThreadProc, pmp, CTF_PROCESS_REF, NULL))
            {
                return S_OK;
            }
        }
        CleanupPMP(pmp);
        DllRelease();
    }
    
    return E_OUTOFMEMORY;
}

STDAPI MailRecipient_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return DropHandler_CreateInstance(MailRecipientDropHandler, punkOuter, riid, ppv);
}

void GetIconString(LPTSTR pszBuf, LPCTSTR pszModule, int id)
{
    wnsprintf(pszBuf, MAX_PATH, TEXT("%s,-%d"), pszModule, id);
}

// get the pathname to a sendto folder item

HRESULT GetDropTargetPath(LPTSTR pszPath, int id, LPCTSTR pszExt)
{
    LPITEMIDLIST pidl;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, &pidl)))
    {
        TCHAR szFileName[128], szBase[64];

        SHGetPathFromIDList(pidl, pszPath);

        SHFree(pidl);

        LoadString(g_hinst, id, szBase, ARRAYSIZE(szBase));

        wnsprintf(szFileName, ARRAYSIZE(szFileName), TEXT("\\%s.%s"), szBase, pszExt);

        lstrcat(pszPath, szFileName);
        return S_OK;
    }
    return E_FAIL;
}

#define SENDMAIL_EXTENSION  TEXT("MAPIMail")
#define EXCHANGE_EXTENSION  TEXT("lnk")

void CommonRegister(HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszExtension, int idFileName)
{
    TCHAR szFile[MAX_PATH];
    HKEY hk;
    TCHAR szKey[80];

    // BUGBUG 981007 review BYTE/TEXT
    RegSetValueEx(hkCLSID, NEVERSHOWEXT, 0, REG_SZ, (BYTE *)TEXT(""), SIZEOF(TCHAR));

    if (RegCreateKey(hkCLSID, SHELLEXT_DROPHANDLER, &hk) == ERROR_SUCCESS) 
    {
        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)pszCLSID, (lstrlen(pszCLSID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT(".%s"), pszExtension);
    if (RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hk) == ERROR_SUCCESS) 
    {
        TCHAR szProgID[80];

        wnsprintf(szProgID, ARRAYSIZE(szProgID), TEXT("CLSID\\%s"), pszCLSID);

        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szProgID, (lstrlen(szProgID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    if (SUCCEEDED(GetDropTargetPath(szFile, idFileName, pszExtension)))
    {
        HANDLE hfile = CreateFile(szFile, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile != INVALID_HANDLE_VALUE)
            CloseHandle(hfile);
    }
}

STDAPI MailRecipient_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule)
{
    TCHAR szFile[MAX_PATH];
    if (bReg)
    {
        HKEY hk;
        CommonRegister(hkCLSID, pszCLSID, SENDMAIL_EXTENSION, IDS_MAIL_FILENAME);

        if (RegCreateKey(hkCLSID, DEFAULTICON, &hk) == ERROR_SUCCESS) 
        {
            TCHAR szIcon[MAX_PATH + 10];
            GetIconString(szIcon, pszModule, IDI_MAIL);
            RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szIcon, (lstrlen(szIcon) + 1) * SIZEOF(TCHAR));
            RegCloseKey(hk);
        }

        // hide the exchange shortcut
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, EXCHANGE_EXTENSION)))
            SetFileAttributes(szFile, FILE_ATTRIBUTE_HIDDEN);
    }
    else
    {
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, SENDMAIL_EXTENSION)))
            DeleteFile(szFile);

        // unhide the exchange shortcut
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, EXCHANGE_EXTENSION)))
            SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
    }
    return S_OK;
}
