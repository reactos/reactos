/*
 *    Network Places (Neighbourhood) folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2003              Mike McCormack for Codeweavers
 *    Copyright 2009              Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL (shell);

#define HACKY_UNC_PATHS

#ifdef HACKY_UNC_PATHS
LPITEMIDLIST ILCreateFromNetworkPlaceW(LPCWSTR lpNetworkPlace)
{
    int cbData = sizeof(WORD) + sizeof(WCHAR) * (wcslen(lpNetworkPlace)+1);
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    pidl->mkid.cb = cbData;
    wcscpy((WCHAR*)&pidl->mkid.abID[0], lpNetworkPlace);
    *(WORD*)((char*)pidl + cbData) = 0;

    return pidl;
}
#endif

/***********************************************************************
*   IShellFolder implementation
*/

HRESULT CNetFolderExtractIcon_CreateInstance(LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit, &initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_NETWORK_FOLDER);

    return initIcon->QueryInterface(riid, ppvOut);
}

HRESULT CALLBACK NetFolderMenuCallback(IShellFolder *psf,
                                       HWND         hwnd,
                                       IDataObject  *pdtobj,
                                       UINT         uMsg,
                                       WPARAM       wParam,
                                       LPARAM       lParam)
{
    return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);
}

class CNetFolderEnum :
    public CEnumIDListBase
{
    public:
        CNetFolderEnum();
        ~CNetFolderEnum();
        HRESULT WINAPI Initialize(HWND hwndOwner, DWORD dwFlags);
        BOOL CreateMyCompEnumList(DWORD dwFlags);
        BOOL EnumerateRec(LPNETRESOURCE lpNet);

        BEGIN_COM_MAP(CNetFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

static shvheader NetworkPlacesSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_CATEGORY, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_WORKGROUP, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_NETLOCATION, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15}
};

#define COLUMN_NAME          0
#define COLUMN_CATEGORY      1
#define COLUMN_WORKGROUP     2
#define COLUMN_NETLOCATION   3

#define NETWORKPLACESSHELLVIEWCOLUMNS 4

CNetFolderEnum::CNetFolderEnum()
{
}

CNetFolderEnum::~CNetFolderEnum()
{
}

HRESULT WINAPI CNetFolderEnum::Initialize(HWND hwndOwner, DWORD dwFlags)
{
    if (CreateMyCompEnumList(dwFlags) == FALSE)
        return E_FAIL;

    return S_OK;
}

/**************************************************************************
 *  CDrivesFolderEnum::CreateMyCompEnumList()
 */

BOOL CNetFolderEnum::EnumerateRec(LPNETRESOURCE lpNet)
{
    BOOL bRet = TRUE;
    DWORD dRet;
    HANDLE hEnum;
    LPNETRESOURCE lpRes;
    DWORD dSize = 0x1000;
    DWORD dCount = -1;
    LPNETRESOURCE lpCur;

    dRet = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, lpNet, &hEnum);
    if (dRet != WN_SUCCESS)
    {
        ERR("WNetOpenEnum() failed: %x\n", dRet);
        return FALSE;
    }

    lpRes = (LPNETRESOURCE)CoTaskMemAlloc(dSize);
    if (!lpRes)
    {
        ERR("CoTaskMemAlloc() failed\n");
        WNetCloseEnum(hEnum);
        return FALSE;
    }

    do
    {
        dSize = 0x1000;
        dCount = -1;

        memset(lpRes, 0, dSize);
        dRet = WNetEnumResource(hEnum, &dCount, lpRes, &dSize);
        if (dRet == WN_SUCCESS || dRet == WN_MORE_DATA)
        {
            lpCur = lpRes;
            for (; dCount; dCount--)
            {
                TRACE("lpRemoteName: %S\n", lpCur->lpRemoteName);

                if ((lpCur->dwUsage & RESOURCEUSAGE_CONTAINER) == RESOURCEUSAGE_CONTAINER)
                {
                    TRACE("Found provider: %S\n", lpCur->lpProvider);
                    EnumerateRec(lpCur);
                }
                else
                {
                    LPITEMIDLIST pidl;

#ifdef HACKY_UNC_PATHS
                    pidl = ILCreateFromNetworkPlaceW(lpCur->lpRemoteName);
#endif
                    if (pidl != NULL)
                        bRet = AddToEnumList(pidl);
                    else
                    {
                        ERR("ILCreateFromPathW() failed\n");
                        bRet = FALSE;
                        break;
                    }
                }

                lpCur++;
            }
        }
    } while (dRet != WN_NO_MORE_ENTRIES);

    CoTaskMemFree(lpRes);
    WNetCloseEnum(hEnum);

    TRACE("Done: %u\n", bRet);

    return bRet;
}

BOOL CNetFolderEnum::CreateMyCompEnumList(DWORD dwFlags)
{
    BOOL bRet = TRUE;

    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

    /* enumerate the folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        bRet = EnumerateRec(NULL);
    }

    return bRet;
}

/**************************************************************************
 * CNetFolder background context menu
 */
static HRESULT CALLBACK CNetFolderBackgroundMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
                                                   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum { IDC_PROPERTIES };
    if (uMsg == DFM_INVOKECOMMAND && wParam == IDC_PROPERTIES)
    {
        return SHELL_ExecuteControlPanelCPL(hwnd, L"ncpa.cpl") ? S_OK : E_FAIL;
    }
    else if (uMsg == DFM_MERGECONTEXTMENU) // TODO: DFM_MERGECONTEXTMENU_BOTTOM
    {
        QCMINFO *pqcminfo = (QCMINFO*)lParam;
        HMENU hpopup = CreatePopupMenu();
        _InsertMenuItemW(hpopup, 0, TRUE, IDC_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
        pqcminfo->idCmdFirst = Shell_MergeMenus(pqcminfo->hmenu, hpopup, pqcminfo->indexMenu, pqcminfo->idCmdFirst, pqcminfo->idCmdLast, MM_ADDSEPARATOR);
        DestroyMenu(hpopup);
        return S_OK;
    }
    return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);
}

/**************************************************************************
 * CNetFolder
 */

CNetFolder::CNetFolder()
{
    pidlRoot = NULL;
}

CNetFolder::~CNetFolder()
{
    if (pidlRoot)
        SHFree(pidlRoot);
}

/**************************************************************************
*    CNetFolder::ParseDisplayName
*/
HRESULT WINAPI CNetFolder::ParseDisplayName(HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
        DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;
#ifdef HACKY_UNC_PATHS
    /* FIXME: the code below is an ugly hack */

    /* Can we use a CFSFolder on that path? */
    DWORD attrs = GetFileAttributes(lpszDisplayName);
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        if (pchEaten)
            *pchEaten = 0;        /* strange but like the original */

        /* YES WE CAN */

        /* Create our hacky pidl */
        LPITEMIDLIST pidl = ILCreateFromNetworkPlaceW(lpszDisplayName);

        *ppidl = pidl;
        if (pdwAttributes)
            *pdwAttributes = SFGAO_FILESYSTEM | SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR;
        return S_OK;
    }
#endif

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", this,
          hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    TRACE("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        CNetFolder::EnumObjects
*/
HRESULT WINAPI CNetFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CNetFolderEnum>(hwndOwner, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
*        CNetFolder::BindToObject
*/
HRESULT WINAPI CNetFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
#ifdef HACKY_UNC_PATHS
    /* Create the target folder info */
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    pfti.dwAttributes = -1;
    pfti.csidl = -1;
    StringCchCopyW(pfti.szTargetParsingName, MAX_PATH, (WCHAR*)pidl->mkid.abID);

    return SHELL32_BindToSF(pidlRoot, &pfti, pidl, &CLSID_ShellFSFolder, riid, ppvOut);
#else
    return E_NOTIMPL;
#endif
}

/**************************************************************************
*    CNetFolder::BindToStorage
*/
HRESULT WINAPI CNetFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this,
          pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     CNetFolder::CompareIDs
*/

HRESULT WINAPI CNetFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return E_NOTIMPL;
}

/**************************************************************************
*    CNetFolder::CreateViewObject
*/
HRESULT WINAPI CNetFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", this,
          hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        WARN("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, 0, NULL, this, CNetFolderBackgroundMenuCB,
                                    0, NULL, (IContextMenu**)ppvOut);
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
            SFV_CREATE sfvparams = {sizeof(SFV_CREATE), this};
            hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppvOut);
    }
    TRACE("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CNetFolder::GetAttributesOf
*/
HRESULT WINAPI CNetFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    static const DWORD dwNethoodAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", this,
          cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut = dwNethoodAttributes;
    else
    {
        /* FIXME: Implement when enumerating items is implemented */
#ifdef HACKY_UNC_PATHS
        *rgfInOut = SFGAO_FILESYSTEM | SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR;
#endif
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*    CNetFolder::GetUIObjectOf
*
* PARAMETERS
*  hwndOwner [in]  Parent window for any output
*  cidl      [in]  array size
*  apidl     [in]  simple pidl array
*  riid      [in]  Requested Interface
*  prgfInOut [   ] reserved
*  ppvObject [out] Resulting Interface
*
*/
HRESULT WINAPI CNetFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
        UINT * prgfInOut, LPVOID * ppvOut)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", this,
          hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IContextMenu) && (cidl >= 1))
    {
        IContextMenu * pCm = NULL;
        HKEY hkey;
        UINT cKeys = 0;
        AddClassKeyToArray(L"Folder", &hkey, &cKeys);
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, this, NetFolderMenuCallback, cKeys, &hkey, &pCm);
        pObj = pCm;
    }
    else if (IsEqualIID(riid, IID_IDataObject) && (cidl >= 1))
    {
        IDataObject * pDo = NULL;
        hr = IDataObject_Constructor (hwndOwner, pidlRoot, apidl, cidl, TRUE, &pDo);
        pObj = pDo;
    }
    else if ((IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) && (cidl == 1))
    {
        hr = CNetFolderExtractIcon_CreateInstance(apidl[0], riid, &pObj);
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && (cidl >= 1))
    {
        IDropTarget * pDt = NULL;
        hr = this->QueryInterface(IID_PPV_ARG(IDropTarget, &pDt));
        pObj = pDt;
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
*    CNetFolder::GetDisplayNameOf
*
*/
HRESULT WINAPI CNetFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!strRet || !pidl || !pidl->mkid.cb)
        return E_INVALIDARG;

#ifdef HACKY_UNC_PATHS
    return SHSetStrRet(strRet, (LPCWSTR)pidl->mkid.abID);
#endif
    return E_NOTIMPL;
}

/**************************************************************************
*  CNetFolder::SetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner [in]  Owner window for output
*  pidl      [in]  simple pidl of item to change
*  lpszName  [in]  the items new display name
*  dwFlags   [in]  SHGNO formatting flags
*  ppidlOut  [out] simple pidl returned
*/
HRESULT WINAPI CNetFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /*simple pidl */
                                     LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    FIXME("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this,
          hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

HRESULT WINAPI CNetFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::GetDefaultColumn (DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CNetFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    TRACE("(%p)\n", this);

    if (!pcsFlags || iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetworkPlacesSFHeader[iColumn].colstate;
    return S_OK;
}

HRESULT WINAPI CNetFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetworkPlacesSFHeader[iColumn].fmt;
    psd->cxChar = NetworkPlacesSFHeader[iColumn].cxChar;
    if (pidl == NULL)
        return SHSetStrRet(&psd->str, NetworkPlacesSFHeader[iColumn].colnameid);

    if (iColumn == COLUMN_NAME)
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &psd->str);

    FIXME("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME("(%p)\n", this);

    return E_NOTIMPL;
}

/************************************************************************
 *    CNetFolder::GetClassID
 */
HRESULT WINAPI CNetFolder::GetClassID(CLSID *lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkPlaces;

    return S_OK;
}

/************************************************************************
 *    CNetFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CNetFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    if (pidlRoot)
        SHFree((LPVOID)pidlRoot);

    pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    CNetFolder::GetCurFolder
 */
HRESULT WINAPI CNetFolder::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);

    return S_OK;
}
