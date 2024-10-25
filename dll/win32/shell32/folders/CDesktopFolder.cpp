/*
 *    Virtual Desktop Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2009                Andrew Hill
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>
#include "CFSFolder.h" // Only for CFSFolder::*FSColumn* helpers!

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern BOOL SHELL32_IsShellFolderNamespaceItemHidden(LPCWSTR SubKey, REFCLSID Clsid);

static const REQUIREDREGITEM g_RequiredItems[] =
{
    { CLSID_MyComputer, "sysdm.cpl", 0x50 },
    { CLSID_NetworkPlaces, "ncpa.cpl", 0x58 },
    { CLSID_Internet, "inetcpl.cpl", 0x68 },
};
static const REGFOLDERINFO g_RegFolderInfo =
{
    PT_DESKTOP_REGITEM,
    _countof(g_RequiredItems), g_RequiredItems,
    CLSID_ShellDesktop,
    L"",
    L"Desktop",
};

static BOOL IsSelf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
{
    return cidl == 0 || (cidl == 1 && apidl && _ILIsEmpty(apidl[0]));
}

static const CLSID* IsRegItem(PCUITEMID_CHILD pidl)
{
    if (pidl && pidl->mkid.cb == 2 + 2 + sizeof(CLSID) && pidl->mkid.abID[0] == PT_GUID)
        return (const CLSID*)(&pidl->mkid.abID[2]);
    return NULL;
}

STDMETHODIMP
CDesktopFolder::ShellUrlParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    LPWSTR pch;
    INT cch, csidl;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    PARSEDURLW ParsedURL = { sizeof(ParsedURL) };

    ::ParseURLW(lpszDisplayName, &ParsedURL);

    DWORD attrs = (pdwAttributes ? *pdwAttributes : 0) | SFGAO_STREAM;
    if (ParsedURL.pszSuffix[0] == L':' && ParsedURL.pszSuffix[1] == L':') // It begins from "::"
    {
        CComPtr<IShellFolder> psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            CComPtr<IBindCtx> pBindCtx;
            hr = ::CreateBindCtx(0, &pBindCtx);
            if (SUCCEEDED(hr))
            {
                BIND_OPTS BindOps = { sizeof(BindOps) };
                BindOps.grfMode = STGM_CREATE;
                pBindCtx->SetBindOptions(&BindOps);
                hr = psfDesktop->ParseDisplayName(hwndOwner, pBindCtx,
                                                  (LPWSTR)ParsedURL.pszSuffix,
                                                  pchEaten, ppidl, &attrs);
            }
        }
    }
    else
    {
        csidl = Shell_ParseSpecialFolder(ParsedURL.pszSuffix, &pch, &cch);
        if (csidl == -1)
        {
            ERR("\n");
            return hr;
        }

        CComHeapPtr<ITEMIDLIST> pidlLocation;
        hr = SHGetFolderLocation(hwndOwner, (csidl | CSIDL_FLAG_CREATE), NULL, 0, &pidlLocation);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (pch && *pch)
        {
            CComPtr<IShellFolder> psfFolder;
            hr = SHBindToObject(NULL, pidlLocation, IID_PPV_ARG(IShellFolder, &psfFolder));
            if (SUCCEEDED(hr))
            {
                CComHeapPtr<ITEMIDLIST> pidlNew;
                hr = psfFolder->ParseDisplayName(hwndOwner, pbc, pch, pchEaten, &pidlNew, &attrs);
                if (SUCCEEDED(hr))
                {
                    hr = SHILCombine(pidlLocation, pidlNew, ppidl);
                    if (pchEaten)
                        *pchEaten += cch;
                }
            }
        }
        else
        {
            if (attrs)
                hr = SHGetNameAndFlagsW(pidlLocation, 0, NULL, 0, &attrs);

            if (SUCCEEDED(hr))
            {
                if (pchEaten)
                    *pchEaten = cch;
                *ppidl = pidlLocation.Detach();
            }
        }
    }

    // FIXME: SHWindowsPolicy
    if (SUCCEEDED(hr) && (attrs & SFGAO_STREAM) &&
        !BindCtx_ContainsObject(pbc, STR_PARSE_SHELL_PROTOCOL_TO_FILE_OBJECTS))
    {
        ILFree(*ppidl);
        *ppidl = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (pdwAttributes)
        *pdwAttributes = attrs;

    // FIXME: SHWindowsPolicy
    if (FAILED(hr) && !Shell_FailForceReturn(hr))
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    return hr;
}

STDMETHODIMP
CDesktopFolder::HttpUrlParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    FIXME("\n");
    return E_NOTIMPL; // FIXME
}

/*
CDesktopFolder should create two file system folders internally, one representing the
user's desktop folder, and the other representing the common desktop folder. It should
also create a CRegFolder to represent the virtual items that exist only in the registry.
The CRegFolder is aggregated by the CDesktopFolder, and queries for the CLSID_IShellFolder,
CLSID_IShellFolder2, or CLSID_IShellIconOverlay interfaces prefer the CRegFolder
implementation.
The CDesktopFolderEnum class should create two enumerators, one for each of the file
system folders, and enumerate the contents of each folder. Since the CRegFolder
implementation of IShellFolder::EnumObjects enumerates the virtual items, the
CDesktopFolderEnum is only responsible for returning the physical items.
CDesktopFolderViewCB is responsible for filtering hidden regitems.
The enumerator always shows My Computer.
*/

/* Undocumented functions from shdocvw */
extern "C" HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl);

class CDesktopFolderEnum :
    public CEnumIDListBase
{
    private:
//    CComPtr                                fDesktopEnumerator;
//    CComPtr                                fCommonDesktopEnumerator;
    public:

        HRESULT WINAPI Initialize(IShellFolder *pRegFolder, SHCONTF dwFlags, IEnumIDList *pRegEnumerator,
                                  IEnumIDList *pDesktopEnumerator, IEnumIDList *pCommonDesktopEnumerator)
        {
            TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

            AppendItemsFromEnumerator(pRegEnumerator);

            /* Enumerate the items in the two fs folders */
            AppendItemsFromEnumerator(pDesktopEnumerator);
            AppendItemsFromEnumerator(pCommonDesktopEnumerator);

            return S_OK;
        }

        BEGIN_COM_MAP(CDesktopFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll);

static const DWORD dwDesktopAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
    SFGAO_STORAGEANCESTOR | SFGAO_HASPROPSHEET | SFGAO_STORAGE;
static const DWORD dwMyComputerAttributes =
    SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
    SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;
static DWORD dwMyNetPlacesAttributes =
    SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
    SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;

CDesktopFolder::CDesktopFolder() :
    sPathTarget(NULL),
    pidlRoot(NULL)
{
}

CDesktopFolder::~CDesktopFolder()
{
}

HRESULT WINAPI CDesktopFolder::FinalConstruct()
{
    WCHAR szMyPath[MAX_PATH];
    HRESULT hr;

    /* Create the root pidl */
    pidlRoot = _ILCreateDesktop();
    if (!pidlRoot)
        return E_OUTOFMEMORY;

    /* Create the inner fs folder */
    hr = SHELL32_CoCreateInitSF(pidlRoot,
                                &CLSID_ShellFSFolder,
                                CSIDL_DESKTOPDIRECTORY,
                                IID_PPV_ARG(IShellFolder2, &m_DesktopFSFolder));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Create the inner shared fs folder. Dont fail on failure. */
    hr = SHELL32_CoCreateInitSF(pidlRoot,
                                &CLSID_ShellFSFolder,
                                CSIDL_COMMON_DESKTOPDIRECTORY,
                                IID_PPV_ARG(IShellFolder2, &m_SharedDesktopFSFolder));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Create the inner reg folder */
    REGFOLDERINITDATA RegInit = { static_cast<IShellFolder*>(this), &g_RegFolderInfo };
    hr = CRegFolder_CreateInstance(&RegInit,
                                   pidlRoot,
                                   IID_PPV_ARG(IShellFolder2, &m_regFolder));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Cache the path to the user desktop directory */
    if (!SHGetSpecialFolderPathW( 0, szMyPath, CSIDL_DESKTOPDIRECTORY, TRUE ))
        return E_UNEXPECTED;

    sPathTarget = (LPWSTR)SHAlloc((wcslen(szMyPath) + 1) * sizeof(WCHAR));
    if (!sPathTarget)
        return E_OUTOFMEMORY;

    wcscpy(sPathTarget, szMyPath);
    return S_OK;
}

HRESULT CDesktopFolder::_GetSFFromPidl(LPCITEMIDLIST pidl, IShellFolder2** psf)
{
    WCHAR szFileName[MAX_PATH];

    if (_ILIsSpecialFolder(pidl))
        return m_regFolder->QueryInterface(IID_PPV_ARG(IShellFolder2, psf));

    lstrcpynW(szFileName, sPathTarget, MAX_PATH - 1);
    PathAddBackslashW(szFileName);
    int cLen = wcslen(szFileName);

    if (!_ILSimpleGetTextW(pidl, szFileName + cLen, MAX_PATH - cLen))
        return E_FAIL;

    if (GetFileAttributes(szFileName) == INVALID_FILE_ATTRIBUTES)
        return m_SharedDesktopFSFolder->QueryInterface(IID_PPV_ARG(IShellFolder2, psf));
    else
        return m_DesktopFSFolder->QueryInterface(IID_PPV_ARG(IShellFolder2, psf));
}

HRESULT CDesktopFolder::_ParseDisplayNameByParent(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    if (pchEaten)
        *pchEaten = 0;

    CComHeapPtr<ITEMIDLIST> pidlParent;
    BOOL bPath = FALSE;
    WCHAR wch = *lpszDisplayName;
    if (((L'A' <= wch && wch <= L'Z') || (L'a' <= wch && wch <= L'z')) &&
        (lpszDisplayName[1] == L':'))
    {
        // "C:..."
        bPath = TRUE;
        pidlParent.Attach(_ILCreateMyComputer());
    }
    else if (PathIsUNCW(lpszDisplayName)) // "\\\\..."
    {
        bPath = TRUE;
        pidlParent.Attach(_ILCreateNetwork());
    }

    if (bPath)
    {
        if (!pidlParent)
            return E_OUTOFMEMORY;

        CComPtr<IShellFolder> pParentFolder;
        SHBindToObject(NULL, pidlParent, IID_PPV_ARG(IShellFolder, &pParentFolder));

        CComHeapPtr<ITEMIDLIST> pidlChild;
        HRESULT hr = pParentFolder->ParseDisplayName(hwndOwner, pbc, lpszDisplayName,
                                                     pchEaten, &pidlChild, pdwAttributes);
        if (FAILED(hr))
            return hr;

        *ppidl = ILCombine(pidlParent, pidlChild);
        return (*ppidl ? S_OK : E_OUTOFMEMORY);
    }

    if (!UrlIsW(lpszDisplayName, URLIS_URL) || SHSkipJunctionBinding(pbc, NULL))
        return E_INVALIDARG;

    // Now lpszDisplayName is a URL
    PARSEDURLW ParsedURL = { sizeof(ParsedURL) };
    ::ParseURLW(lpszDisplayName, &ParsedURL);

    switch (ParsedURL.nScheme)
    {
        case URL_SCHEME_FILE: // "file:..."
        {
            // Convert "file://..." to a normal path
            WCHAR szPath[MAX_PATH];
            DWORD cchPath = _countof(szPath);
            HRESULT hr = PathCreateFromUrlW(lpszDisplayName, szPath, &cchPath, 0);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            CComPtr<IShellFolder> psfDesktop;
            hr = SHGetDesktopFolder(&psfDesktop);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            // Parse by desktop folder
            return psfDesktop->ParseDisplayName(hwndOwner, pbc, szPath, pchEaten, ppidl,
                                                pdwAttributes);
        }
        case URL_SCHEME_HTTP:  // "http:..."
        case URL_SCHEME_HTTPS: // "https:..."
        {
            if (!BindCtx_ContainsObject(pbc, STR_PARSE_PREFER_FOLDER_BROWSING))
                return E_INVALIDARG;

            return HttpUrlParseDisplayName(hwndOwner,
                                           pbc,
                                           lpszDisplayName,
                                           pchEaten,
                                           ppidl,
                                           pdwAttributes);
        }
        case URL_SCHEME_SHELL: // "shell:..."
        {
            return ShellUrlParseDisplayName(hwndOwner,
                                            pbc,
                                            lpszDisplayName,
                                            pchEaten,
                                            ppidl,
                                            pdwAttributes);
        }
        case URL_SCHEME_MSSHELLROOTED:
        case URL_SCHEME_MSSHELLIDLIST:
        {
            WARN("We don't support 'ms-shell-rooted:' and 'ms-shell-idlist:' schemes\n");
            break;
        }
        default:
        {
            TRACE("Scheme: %u\n", ParsedURL.nScheme);
            break;
        }
    }

    return E_INVALIDARG;
}

/**************************************************************************
 *    CDesktopFolder::ParseDisplayName
 *
 * NOTES
 *    "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
 *    to MyComputer
 */
HRESULT WINAPI CDesktopFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    if (!lpszDisplayName)
        return E_INVALIDARG;

    if (!*lpszDisplayName)
    {
        *ppidl = _ILCreateMyComputer();
        return (*ppidl ? S_OK : E_OUTOFMEMORY);
    }

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        return m_regFolder->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl,
                                             pdwAttributes);
    }

    HRESULT hr = _ParseDisplayNameByParent(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl,
                                           pdwAttributes);
    if (SUCCEEDED(hr))
    {
        if (BindCtx_ContainsObject(pbc, STR_PARSE_TRANSLATE_ALIASES))
        {
            CComHeapPtr<ITEMIDLIST> pidlAlias;
            if (SUCCEEDED(Shell_TranslateIDListAlias(*ppidl, NULL, &pidlAlias, 0xFFFF)))
            {
                ILFree(*ppidl);
                *ppidl = pidlAlias.Detach();
            }
        }

        TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);
        return hr;
    }

    if (Shell_FailForceReturn(hr))
        return hr;

    if (BindCtx_ContainsObject(pbc, STR_DONT_PARSE_RELATIVE))
        return E_INVALIDARG;

    if (SHIsFileSysBindCtx(pbc, NULL) == S_OK)
        return hr;

    BIND_OPTS BindOps = { sizeof(BindOps) };
    BOOL bCreate = FALSE;
    if (pbc && SUCCEEDED(pbc->GetBindOptions(&BindOps)) && (BindOps.grfMode & STGM_CREATE))
    {
        BindOps.grfMode &= ~STGM_CREATE;
        bCreate = TRUE;
        pbc->SetBindOptions(&BindOps);
    }

    if (m_DesktopFSFolder)
    {
        hr = m_DesktopFSFolder->ParseDisplayName(hwndOwner,
                                                 pbc,
                                                 lpszDisplayName,
                                                 pchEaten,
                                                 ppidl,
                                                 pdwAttributes);
    }

    if (FAILED(hr) && m_SharedDesktopFSFolder)
    {
        hr = m_SharedDesktopFSFolder->ParseDisplayName(hwndOwner,
                                                       pbc,
                                                       lpszDisplayName,
                                                       pchEaten,
                                                       ppidl,
                                                       pdwAttributes);
    }

    if (FAILED(hr) && bCreate && m_DesktopFSFolder)
    {
        BindOps.grfMode |= STGM_CREATE;
        pbc->SetBindOptions(&BindOps);
        hr = m_DesktopFSFolder->ParseDisplayName(hwndOwner,
                                                 pbc,
                                                 lpszDisplayName,
                                                 pchEaten,
                                                 ppidl,
                                                 pdwAttributes);
    }

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
 *        CDesktopFolder::EnumObjects
 */
HRESULT WINAPI CDesktopFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CComPtr<IEnumIDList> pRegEnumerator;
    CComPtr<IEnumIDList> pDesktopEnumerator;
    CComPtr<IEnumIDList> pCommonDesktopEnumerator;
    HRESULT hr;

    hr = m_regFolder->EnumObjects(hwndOwner, dwFlags, &pRegEnumerator);
    if (FAILED(hr))
        ERR("EnumObjects for reg folder failed\n");

    hr = m_DesktopFSFolder->EnumObjects(hwndOwner, dwFlags, &pDesktopEnumerator);
    if (FAILED(hr))
        ERR("EnumObjects for desktop fs folder failed\n");

    hr = m_SharedDesktopFSFolder->EnumObjects(hwndOwner, dwFlags, &pCommonDesktopEnumerator);
    if (FAILED(hr))
        ERR("EnumObjects for shared desktop fs folder failed\n");

    return ShellObjectCreatorInit<CDesktopFolderEnum>(m_regFolder, dwFlags, pRegEnumerator, pDesktopEnumerator,
                                                      pCommonDesktopEnumerator, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
 *        CDesktopFolder::BindToObject
 */
HRESULT WINAPI CDesktopFolder::BindToObject(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    if (!pidl)
        return E_INVALIDARG;

    CComPtr<IShellFolder2> psf;
    HRESULT hr = _GetSFFromPidl(pidl, &psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return psf->BindToObject(pidl, pbcReserved, riid, ppvOut);
}

/**************************************************************************
 *    CDesktopFolder::BindToStorage
 */
HRESULT WINAPI CDesktopFolder::BindToStorage(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     CDesktopFolder::CompareIDs
 */
HRESULT WINAPI CDesktopFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    bool bIsDesktopFolder1, bIsDesktopFolder2;

    if (!pidl1 || !pidl2)
    {
        ERR("Got null pidl pointer (%Ix %p %p)!\n", lParam, pidl1, pidl2);
        return E_INVALIDARG;
    }

    bIsDesktopFolder1 = _ILIsDesktop(pidl1);
    bIsDesktopFolder2 = _ILIsDesktop(pidl2);
    if (bIsDesktopFolder1 || bIsDesktopFolder2)
        return MAKE_COMPARE_HRESULT(bIsDesktopFolder1 - bIsDesktopFolder2);

    if (_ILIsSpecialFolder(pidl1) || _ILIsSpecialFolder(pidl2))
        return m_regFolder->CompareIDs(lParam, pidl1, pidl2);

    return m_DesktopFSFolder->CompareIDs(lParam, pidl1, pidl2);
}

/**************************************************************************
 *    CDesktopFolder::CreateViewObject
 */
HRESULT WINAPI CDesktopFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    LPVOID *ppvOut)
{
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           this, hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        hr = m_DesktopFSFolder->CreateViewObject(hwndOwner, riid, ppvOut);
    }
    else if (IsEqualIID (riid, IID_IContextMenu))
    {
            HKEY hKeys[16];
            UINT cKeys = 0;
            AddClassKeyToArray(L"Directory\\Background", hKeys, &cKeys);

            DEFCONTEXTMENU dcm;
            dcm.hwnd = hwndOwner;
            dcm.pcmcb = this;
            dcm.pidlFolder = pidlRoot;
            dcm.psf = this;
            dcm.cidl = 0;
            dcm.apidl = NULL;
            dcm.cKeys = cKeys;
            dcm.aKeys = hKeys;
            dcm.punkAssociationInfo = NULL;
            hr = SHCreateDefaultContextMenu (&dcm, riid, ppvOut);
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        CComPtr<CDesktopFolderViewCB> sfviewcb;
        if (SUCCEEDED(hr = ShellObjectCreator(sfviewcb)))
        {
            SFV_CREATE create = { sizeof(create), this, NULL, sfviewcb };
            hr = SHCreateShellFolderView(&create, (IShellView**)ppvOut);
            if (SUCCEEDED(hr))
                sfviewcb->Initialize((IShellView*)*ppvOut);
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
 *  CDesktopFolder::GetAttributesOf
 */
HRESULT WINAPI CDesktopFolder::GetAttributesOf(
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    DWORD *rgfInOut)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
          this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut &= dwDesktopAttributes;
    else
    {
        /* TODO: always add SFGAO_CANLINK */
        for (UINT i = 0; i < cidl; ++i)
        {
            pdump(*apidl);
            if (_ILIsDesktop(*apidl))
                *rgfInOut &= dwDesktopAttributes;
            else if (_ILIsMyComputer(apidl[i]))
                *rgfInOut &= dwMyComputerAttributes;
            else if (_ILIsNetHood(apidl[i]))
                *rgfInOut &= dwMyNetPlacesAttributes;
            else if (_ILIsFolderOrFile(apidl[i]) || _ILIsSpecialFolder(apidl[i]))
            {
                CComPtr<IShellFolder2> psf;
                HRESULT hr = _GetSFFromPidl(apidl[i], &psf);
                if (FAILED_UNEXPECTEDLY(hr))
                    continue;

                psf->GetAttributesOf(1, &apidl[i], rgfInOut);
            }
            else
                ERR("Got an unknown pidl type!!!\n");
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);

    return hr;
}

/**************************************************************************
 *    CDesktopFolder::GetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
HRESULT WINAPI CDesktopFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    REFIID riid,
    UINT *prgfInOut,
    LPVOID *ppvOut)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;
    *ppvOut = NULL;

    BOOL self = IsSelf(cidl, apidl);
    if (cidl == 1 && !_ILIsSpecialFolder(apidl[0]) && !self)
    {
        CComPtr<IShellFolder2> psf;
        HRESULT hr = _GetSFFromPidl(apidl[0], &psf);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psf->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    if (IsEqualIID (riid, IID_IContextMenu))
    {
        // FIXME: m_regFolder vs AddFSClassKeysToArray is incorrect when the selection includes both regitems and FS items
        if (!self && cidl > 0 && _ILIsSpecialFolder(apidl[0]))
        {
            hr = m_regFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, &pObj);
        }
        else
        {
            /* Do not use the context menu of the CFSFolder here. */
            /* We need to pass a pointer of the CDesktopFolder so as the data object that the context menu gets is rooted to the desktop */
            /* Otherwise operations like that involve items from both user and shared desktop will not work */
            HKEY hKeys[16];
            UINT cKeys = 0;
            if (self)
            {
                AddClsidKeyToArray(CLSID_ShellDesktop, hKeys, &cKeys);
                AddClassKeyToArray(L"Folder", hKeys, &cKeys);
            }
            else if (cidl > 0)
            {
                AddFSClassKeysToArray(cidl, apidl, hKeys, &cKeys);
            }

            DEFCONTEXTMENU dcm;
            dcm.hwnd = hwndOwner;
            dcm.pcmcb = this;
            dcm.pidlFolder = pidlRoot;
            dcm.psf = this;
            dcm.cidl = cidl;
            dcm.apidl = apidl;
            dcm.cKeys = cKeys;
            dcm.aKeys = hKeys;
            dcm.punkAssociationInfo = NULL;
            hr = SHCreateDefaultContextMenu (&dcm, riid, &pObj);
        }
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor( hwndOwner, pidlRoot, apidl, cidl, TRUE, (IDataObject **)&pObj);
    }
    else if ((IsEqualIID (riid, IID_IExtractIconA) || IsEqualIID (riid, IID_IExtractIconW)) && (cidl == 1))
    {
        hr = m_regFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, &pObj);
    }
    else if (IsEqualIID (riid, IID_IDropTarget) && (cidl == 1))
    {
        CComPtr<IShellFolder> psfChild;
        hr = this->BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psfChild));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfChild->CreateViewObject(NULL, riid, ppvOut);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
 *    CDesktopFolder::GetDisplayNameOf
 *
 * NOTES
 *    special case: pidl = null gives desktop-name back
 */
HRESULT WINAPI CDesktopFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    if (!_ILIsPidlSimple (pidl))
    {
        return SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags, strRet);
    }
    else if (_ILIsDesktop(pidl))
    {
        if (IS_SHGDN_DESKTOPABSOLUTEPARSING(dwFlags))
            return SHSetStrRet(strRet, sPathTarget);
        else
            return m_regFolder->GetDisplayNameOf(pidl, dwFlags, strRet);
    }

    /* file system folder or file rooted at the desktop */
    CComPtr<IShellFolder2> psf;
    HRESULT hr = _GetSFFromPidl(pidl, &psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return psf->GetDisplayNameOf(pidl, dwFlags, strRet);
}

/**************************************************************************
 *  CDesktopFolder::SetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
HRESULT WINAPI CDesktopFolder::SetNameOf(
    HWND hwndOwner,
    PCUITEMID_CHILD pidl,    /* simple pidl */
    LPCOLESTR lpName,
    DWORD dwFlags,
    PITEMID_CHILD *pPidlOut)
{
    CComPtr<IShellFolder2> psf;
    HRESULT hr = _GetSFFromPidl(pidl, &psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return psf->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CDesktopFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CDesktopFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    HRESULT hr;
    TRACE ("(%p)\n", this);

    if (!pcsFlags)
        return E_INVALIDARG;

    hr = CFSFolder::GetDefaultFSColumnState(iColumn, *pcsFlags);
    /*
    // CDesktopFolder may override the flags if desired (future)
    switch(iColumn)
    {
    case SHFSF_COL_FATTS:
        *pcsFlags &= ~SHCOLSTATE_ONBYDEFAULT;
        break;
    }
    */
    return hr;
}

HRESULT WINAPI CDesktopFolder::GetDetailsEx(
    PCUITEMID_CHILD pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

/*************************************************************************
 * Column info functions.
 * CFSFolder.h provides defaults for us.
 */
HRESULT CDesktopFolder::GetColumnDetails(UINT iColumn, SHELLDETAILS &sd)
{
    /* CDesktopFolder may override the flags and/or name if desired */
    return CFSFolder::GetFSColumnDetails(iColumn, sd);
}

HRESULT WINAPI CDesktopFolder::GetDetailsOf(
    PCUITEMID_CHILD pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    if (!psd)
        return E_INVALIDARG;

    if (!pidl)
    {
        return GetColumnDetails(iColumn, *psd);
    }

    CComPtr<IShellFolder2> psf;
    HRESULT hr = _GetSFFromPidl(pidl, &psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr =  psf->GetDetailsOf(pidl, iColumn, psd);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

HRESULT WINAPI CDesktopFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_ShellDesktop;

    return S_OK;
}

HRESULT WINAPI CDesktopFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return S_OK;

    return E_INVALIDARG;
}

HRESULT WINAPI CDesktopFolder::GetCurFolder(PIDLIST_ABSOLUTE * pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_INVALIDARG; /* xp doesn't have this check and crashes on NULL */
    *pidl = ILClone (pidlRoot);
    return S_OK;
}

HRESULT WINAPI CDesktopFolder::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum { IDC_PROPERTIES };
    if (uMsg == DFM_INVOKECOMMAND && wParam == (pdtobj ? DFM_CMD_PROPERTIES : IDC_PROPERTIES))
    {
        return SHELL_ExecuteControlPanelCPL(hwndOwner, L"desk.cpl") ? S_OK : E_FAIL;
    }
    else if (uMsg == DFM_MERGECONTEXTMENU && !pdtobj) // Add Properties item when called for directory background
    {
        QCMINFO *pqcminfo = (QCMINFO *)lParam;
        HMENU hpopup = CreatePopupMenu();
        _InsertMenuItemW(hpopup, 0, TRUE, IDC_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
        pqcminfo->idCmdFirst = Shell_MergeMenus(pqcminfo->hmenu, hpopup, pqcminfo->indexMenu, pqcminfo->idCmdFirst, pqcminfo->idCmdLast, MM_ADDSEPARATOR);
        DestroyMenu(hpopup);
        return S_OK;
    }
    return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);
}

/*************************************************************************
 * CDesktopFolderViewCB
 */

bool CDesktopFolderViewCB::IsProgmanHostedBrowser(IShellView *psv)
{
    FOLDERSETTINGS settings;
    return SUCCEEDED(psv->GetCurrentInfo(&settings)) && (settings.fFlags & FWF_DESKTOP);
}

bool CDesktopFolderViewCB::IsProgmanHostedBrowser()
{
    enum { Uninitialized = 0, NotHosted, IsHosted };
    C_ASSERT(Uninitialized == 0);
    if (m_IsProgmanHosted == Uninitialized)
        m_IsProgmanHosted = m_pShellView && IsProgmanHostedBrowser(m_pShellView) ? IsHosted : NotHosted;
    return m_IsProgmanHosted == IsHosted;
}

HRESULT WINAPI CDesktopFolderViewCB::ShouldShow(IShellFolder *psf, PCIDLIST_ABSOLUTE pidlFolder, PCUITEMID_CHILD pidlItem)
{
    const CLSID* pClsid;
    if (IsProgmanHostedBrowser() && (pClsid = IsRegItem(pidlItem)) != NULL)
    {
        const BOOL NewStart = SHELL_GetSetting(SSF_STARTPANELON, fStartPanelOn);
        LPCWSTR SubKey = NewStart ? L"HideDesktopIcons\\NewStartPanel" : L"HideDesktopIcons\\ClassicStartMenu";
        return SHELL32_IsShellFolderNamespaceItemHidden(SubKey, *pClsid) ? S_FALSE : S_OK;
    }
    return S_OK;
}

HRESULT WINAPI CDesktopFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_VIEWRELEASE:
            m_pShellView = NULL;
            return S_OK;
        case SFVM_GETCOMMANDDIR:
        {
            WCHAR buf[MAX_PATH];
            if (SHGetSpecialFolderPathW(NULL, buf, CSIDL_DESKTOPDIRECTORY, TRUE))
                return StringCchCopyW((PWSTR)lParam, wParam, buf);
            break;
        }
    }
    return E_NOTIMPL;
}

/*************************************************************************
 * SHGetDesktopFolder            [SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
    HRESULT    hres = S_OK;
    TRACE("\n");

    if(!psf) return E_INVALIDARG;
    *psf = NULL;
    hres = CDesktopFolder::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellFolder, psf));

    TRACE("-- %p->(%p)\n",psf, *psf);
    return hres;
}
