#include "sendmail.h"       // pch file
#include "resource.h"
#include "debug.h"

extern CLIPFORMAT g_cfHIDA;

LPIDA DataObj_GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pmedium)
    {
        pmedium->pUnkForRelease = NULL;
        pmedium->hGlobal = NULL;
    }

    if (!pmedium)
    {
        if (SUCCEEDED(pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte)))
            return (LPIDA)TRUE;
        else
            return (LPIDA)FALSE;
    }
    else if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, pmedium)))
    {
        return (LPIDA)GlobalLock(pmedium->hGlobal);
    }

    return NULL;
}

#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])
LPCITEMIDLIST IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    if (NULL == pida)
    {
        return NULL;
    }

    if (i == (UINT)-1 || i < pida->cidl)
    {
        return HIDA_GetPIDLItem(pida, i);
    }

    return NULL;
}

void _ReleaseStgMedium(void *pv, STGMEDIUM *pmedium)
{
    if (pmedium->hGlobal && (pmedium->tymed == TYMED_HGLOBAL))
    {
        GlobalUnlock(pmedium->hGlobal);
    }
    ReleaseStgMedium(pmedium);
}

STDAPI InvokeVerbOnItems(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl, LPCTSTR pszDirectory)
{
    IContextMenu *pcm;
    CHAR szVerbA[128];
    WCHAR szVerbW[128];
    CHAR szDirA[MAX_PATH];
    WCHAR szDirW[MAX_PATH];
    HRESULT hr;
    hr = psf->lpVtbl->GetUIObjectOf(psf, hwnd, cidl, apidl, &IID_IContextMenu, NULL, (void **)&pcm);
    if (SUCCEEDED(hr))
    {
        CMINVOKECOMMANDINFOEX ici =
        {
            SIZEOF(CMINVOKECOMMANDINFOEX),
            uFlags | CMIC_MASK_UNICODE | CMIC_MASK_FLAG_NO_UI,
            hwnd,
            NULL,
            NULL,
            NULL,
            SW_NORMAL,
        };
        SHTCharToAnsi(pszVerb, szVerbA, ARRAYSIZE(szVerbA));
        SHTCharToUnicode(pszVerb, szVerbW, ARRAYSIZE(szVerbW));

        if (pszDirectory)
        {
            SHTCharToAnsi(pszDirectory, szDirA, ARRAYSIZE(szDirA));
            SHTCharToUnicode(pszDirectory, szDirW, ARRAYSIZE(szDirW));
            ici.lpDirectory = szDirA;
            ici.lpDirectoryW = szDirW;
        }

        ici.lpVerb = szVerbA;
        ici.lpVerbW = szVerbW;

        hr = pcm->lpVtbl->InvokeCommand(pcm, (CMINVOKECOMMANDINFO*)&ici);
        pcm->lpVtbl->Release(pcm);
    }
    return hr;
}


STDAPI InvokeVerbOnDataObj(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IDataObject *pdtobj, LPCTSTR pszDirectory)
{
    HRESULT hr;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    IShellFolder *psf;

    hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr) && pida)
    {
        LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
        if (pidlParent &&
            SUCCEEDED(psf->lpVtbl->BindToObject(psf, pidlParent, NULL, &IID_IShellFolder, (void **)&psf)))
        {
            LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * sizeof(LPCITEMIDLIST));
            if (ppidl)
            {
                UINT i;
                for (i = 0; i < pida->cidl; i++) 
                {
                    ppidl[i] = IDA_GetIDListPtr(pida, i);
                }
                hr = InvokeVerbOnItems(hwnd, pszVerb, uFlags, psf, pida->cidl, ppidl, pszDirectory);
                LocalFree((LPVOID)ppidl);
            }
            psf->lpVtbl->Release(psf);
        }
        _ReleaseStgMedium(pida, &medium);
    }
    return hr;
}

STDAPI DesktopShortcutDropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect)
{
    TCHAR szDesktop[MAX_PATH];

    if (_SHGetSpecialFolderPath(NULL, szDesktop, CSIDL_DESKTOP, FALSE))
    {
        if (g_cfHIDA == 0)
        {
            g_cfHIDA = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        }
        return InvokeVerbOnDataObj (NULL, TEXT("link"), 0, pdtobj, szDesktop);
    }
    return E_OUTOFMEMORY;
}


STDAPI DesktopShortcut_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return DropHandler_CreateInstance(DesktopShortcutDropHandler, punkOuter, riid, ppv);
}

#define DESKLINK_EXTENSION  TEXT("DeskLink")

void CommonRegister(HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszExtension, int idFileName);

STDAPI DesktopShortcut_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule)
{
    TCHAR szFile[MAX_PATH];
    if (bReg)
    {
        HKEY hk;

        // get rid of old name "Desktop as Shortcut" link from IE4

        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_DESKTOPLINK_FILENAME, DESKLINK_EXTENSION)))
            DeleteFile(szFile);

        if (RegCreateKey(hkCLSID, DEFAULTICON, &hk) == ERROR_SUCCESS) 
        {
            TCHAR szExplorer[MAX_PATH];
            TCHAR szIcon[MAX_PATH+10];
            GetWindowsDirectory(szExplorer, ARRAYSIZE(szExplorer));
            wnsprintf(szIcon, ARRAYSIZE(szIcon), TEXT("%s\\explorer.exe,-103"), szExplorer);    // ICO_DESKTOP res ID
            RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szIcon, (lstrlen(szIcon) + 1) * SIZEOF(TCHAR));
            RegCloseKey(hk);
        }
        
        CommonRegister(hkCLSID, pszCLSID, DESKLINK_EXTENSION, IDS_DESKTOPLINK_FILENAME_NEW);
    }
    else
    {
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_DESKTOPLINK_FILENAME, DESKLINK_EXTENSION)))
            DeleteFile(szFile);
    }
    return S_OK;
}
