/*
 * Network Connections Shell Folder
 *
 * Copyright 2008       Johannes Anderwald <johannes.anderwald@reactos.org>
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

#include "precomp.h"

#define MAX_PROPERTY_SHEET_PAGE (10)

/***********************************************************************
*   IShellFolder implementation
*/

class CNetworkConnections final :
    public IShellFolder2,
    public IPersistFolder2,
    public IShellExecuteHookW
{
    public:
        CNetworkConnections();

        /* IUnknown */
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();

        // IShellFolder
        virtual HRESULT WINAPI ParseDisplayName (HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, LPITEMIDLIST *ppidl, DWORD *pdwAttributes);
        virtual HRESULT WINAPI EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList);
        virtual HRESULT WINAPI BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
        virtual HRESULT WINAPI CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI GetAttributesOf (UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut);
        virtual HRESULT WINAPI GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
        virtual HRESULT WINAPI GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet);
        virtual HRESULT WINAPI SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST *pPidlOut);

        /* ShellFolder2 */
        virtual HRESULT WINAPI GetDefaultSearchGUID(GUID *pguid);
        virtual HRESULT WINAPI EnumSearches(IEnumExtraSearch **ppenum);
        virtual HRESULT WINAPI GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
        virtual HRESULT WINAPI GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags);
        virtual HRESULT WINAPI GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
        virtual HRESULT WINAPI GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd);
        virtual HRESULT WINAPI MapColumnToSCID(UINT column, SHCOLUMNID *pscid);

        // IPersistFolder2
        virtual HRESULT WINAPI GetClassID(CLSID *lpClassId);
        virtual HRESULT WINAPI Initialize(LPCITEMIDLIST pidl);
        virtual HRESULT WINAPI GetCurFolder(LPITEMIDLIST *pidl);

        // IShellExecuteHookW
        virtual HRESULT WINAPI Execute(LPSHELLEXECUTEINFOW pei);

    private:
        LONG ref;
        /* both paths are parsible from the desktop */
        LPITEMIDLIST pidlRoot;	/* absolute pidl */
        LPITEMIDLIST pidl; /* enumerated pidl */
        IOleCommandTarget * lpOleCmd;
};

class CNetConUiObject final :
    public IContextMenu3,
    public IObjectWithSite,
    public IQueryInfo,
    public IExtractIconW
{
    public:
        CNetConUiObject(LPCITEMIDLIST apidl, IOleCommandTarget *lpOleCmd);

        // IUnknown
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();

        // IContextMenu3
        virtual HRESULT WINAPI QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);
        virtual HRESULT WINAPI HandleMenuMsg( UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual HRESULT WINAPI HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        // IObjectWithSite
        virtual HRESULT WINAPI SetSite(IUnknown *punk);
        virtual HRESULT WINAPI GetSite(REFIID iid, void **ppvSite);

        // IQueryInfo
        virtual HRESULT WINAPI GetInfoFlags(DWORD *pdwFlags);
        virtual HRESULT WINAPI GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);

        // IExtractIconW
        virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
        virtual HRESULT STDMETHODCALLTYPE Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    private:
        LONG ref;
        LPCITEMIDLIST apidl;
        IUnknown *pUnknown;
        IOleCommandTarget * lpOleCmd;
};

static const shvheader NetConnectSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 20},
    {IDS_SHV_COLUMN_TYPE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 8},
    {IDS_SHV_COLUMN_STATE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_DEVNAME, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN_PHONE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_OWNER, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define NETCONNECTSHELLVIEWCOLUMNS 6

#define COLUMN_NAME     0
#define COLUMN_TYPE     1
#define COLUMN_STATUS   2
#define COLUMN_DEVNAME  3
#define COLUMN_PHONE    4
#define COLUMN_OWNER    5

HRESULT ShowNetConnectionStatus(IOleCommandTarget * lpOleCmd, INetConnection * pNetConnect, HWND hwnd);

CNetworkConnections::CNetworkConnections()
{
    pidlRoot = _ILCreateNetConnect();	/* my qualified pidl */
}

/**************************************************************************
 *	ISF_NetConnect_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
HRESULT WINAPI CNetworkConnections::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellFolder) ||
        IsEqualIID(riid, IID_IShellFolder2))
    {
        *ppvObj = (IShellFolder2*)this;
    }
    else if (IsEqualIID (riid, IID_IPersistFolder) ||
             IsEqualIID (riid, IID_IPersistFolder2))
    {
        *ppvObj = (IPersistFolder2*)this;
    }
    else if (IsEqualIID(riid, IID_IShellExecuteHookW))
    {
        *ppvObj = (IShellExecuteHookW*)this;
    }
#if 0
    else if (IsEqualIID(riid, IID_IPersistIDList))
    {
        //*ppvObj = (IPersistIDList*)this;
    }
#endif
    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    /* TODO:
     * IID_IPersistFreeThreadedObject
     * IID_IBrowserFrameOptions
     * IID_IShellIconOverlay
     * IID_IPersistIDList
     * IID_IPersist
     */

    return E_NOINTERFACE;
}

ULONG WINAPI CNetworkConnections::AddRef()
{
    ULONG refCount = InterlockedIncrement(&ref);

    return refCount;
}

ULONG WINAPI CNetworkConnections::Release()
{
    ULONG refCount = InterlockedDecrement(&ref);

    if (!refCount)
    {
        SHFree(pidlRoot);
        delete this;
    }
    return refCount;
}

/**************************************************************************
*	ISF_NetConnect_fnParseDisplayName
*/
HRESULT WINAPI CNetworkConnections::ParseDisplayName (
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    return hr;
}

/**************************************************************************
 *  CreateNetConnectEnumListss()
 */
static BOOL CreateNetConnectEnumList(CEnumIDList *list, DWORD dwFlags)
{
    HRESULT hr;
    INetConnectionManager *pNetConMan;
    IEnumNetConnection *pEnumCon;
    INetConnection *INetCon;
    ULONG Count;
    LPITEMIDLIST pidl;

    /* get an instance to of IConnectionManager */
    hr = INetConnectionManager_Constructor(NULL, IID_INetConnectionManager, (LPVOID*)&pNetConMan);
    if (FAILED(hr))
        return FALSE;

    hr = pNetConMan->EnumConnections(NCME_DEFAULT, &pEnumCon);
    if (FAILED(hr))
    {
        pNetConMan->Release();
        return FALSE;
    }

    do
    {
        hr = pEnumCon->Next(1, &INetCon, &Count);
        if (hr == S_OK)
        {
            pidl = ILCreateNetConnectItem(INetCon);
            if (pidl)
            {
                list->AddToEnumList(pidl);
            }
        }
        else
        {
            break;
        }
    }while(TRUE);

    pEnumCon->Release();
    pNetConMan->Release();

    return TRUE;
}

/**************************************************************************
*		ISF_NetConnect_fnEnumObjects
*/
HRESULT WINAPI CNetworkConnections::EnumObjects(
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CEnumIDList *pList = new CEnumIDList;
    *ppEnumIDList = (LPENUMIDLIST)pList;
    if (!pList)
        return E_OUTOFMEMORY;

    pList->AddRef();
    CreateNetConnectEnumList(pList, dwFlags);
    return S_OK;
}

/**************************************************************************
*		ISF_NetConnect_fnBindToObject
*/
HRESULT WINAPI CNetworkConnections::BindToObject (
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnBindToStorage
*/
HRESULT WINAPI CNetworkConnections::BindToStorage(
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetConnect_fnCompareIDs
*/

HRESULT WINAPI CNetworkConnections::CompareIDs(
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;



    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnCreateViewObject
*/
HRESULT WINAPI CNetworkConnections::CreateViewObject(
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    CSFV cvf;
    HRESULT hr = E_NOINTERFACE;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        ZeroMemory(&cvf, sizeof(cvf));
        cvf.cbSize = sizeof(cvf);
        cvf.pshf = (IShellFolder*)this;

        IShellView* pShellView;
        hr = SHCreateShellFolderViewEx(&cvf, &pShellView);
        if (SUCCEEDED(hr))
        {
            hr = pShellView->QueryInterface(riid, ppvOut);
            pShellView->Release();
        }
    }

    return hr;
}

/**************************************************************************
*  ISF_NetConnect_fnGetAttributesOf
*/
HRESULT WINAPI CNetworkConnections::GetAttributesOf(
               UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD * rgfInOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    static const DWORD dwNetConnectAttributes = SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;

    static const DWORD dwNetConnectItemAttributes = SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_CANRENAME;

    if (!rgfInOut)
        return E_INVALIDARG;

    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut = dwNetConnectAttributes;
    else
        *rgfInOut = dwNetConnectItemAttributes;

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    return hr;
}

/**************************************************************************
*	ISF_NetConnect_fnGetUIObjectOf
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

HRESULT IContextMenuImpl_Constructor(REFIID riid, LPCITEMIDLIST apidl, LPVOID * ppvOut, IOleCommandTarget * lpOleCmd)
{
    CNetConUiObject *pMenu = new CNetConUiObject(apidl, lpOleCmd);
    if (!pMenu)
        return E_OUTOFMEMORY;

    pMenu->AddRef();
    HRESULT hr = pMenu->QueryInterface(riid, ppvOut);
    pMenu->Release();
    return hr;
}

HRESULT WINAPI CNetworkConnections::GetUIObjectOf(
               HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID (riid, IID_IContextMenu) || IsEqualIID (riid, IID_IContextMenu2) || IsEqualIID(riid, IID_IContextMenu3) ||
         IsEqualIID(riid, IID_IQueryInfo) || IsEqualIID(riid, IID_IExtractIconW)) && cidl >= 1)
    {
        return IContextMenuImpl_Constructor(riid, apidl[0], ppvOut, lpOleCmd);
    }
    else
        hr = E_NOINTERFACE;

    *ppvOut = pObj;
    return hr;
}

/**************************************************************************
*	ISF_NetConnect_fnGetDisplayNameOf
*
*/
HRESULT WINAPI CNetworkConnections::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszName;
    HRESULT hr = E_FAIL;
    NETCON_PROPERTIES * pProperties;
    VALUEStruct * val;

    if (!strRet)
        return E_INVALIDARG;

    pszName = (WCHAR*)CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR));
    if (!pszName)
        return E_OUTOFMEMORY;

    if (_ILIsNetConnect(pidl))
    {
        if (LoadStringW(netshell_hInstance, IDS_NETWORKCONNECTION, pszName, MAX_PATH))
        {
            pszName[MAX_PATH-1] = L'\0';
            hr = S_OK;
        }
    }
    else
    {
        val = _ILGetValueStruct(pidl);
        if (val)
        {
            if (val->pItem->GetProperties(&pProperties) == S_OK)
            {
                if (pProperties->pszwName)
                {
                    wcscpy(pszName, pProperties->pszwName);
                    hr = S_OK;
                }
                NcFreeNetconProperties(pProperties);
            }
        }

    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszName;
    }
    else
    {
        CoTaskMemFree(pszName);
    }

    return hr;
}

/**************************************************************************
*  ISF_NetConnect_fnSetNameOf
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
HRESULT WINAPI CNetworkConnections::SetNameOf (
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    VALUEStruct * val;

    val = _ILGetValueStruct(pidl);
    if (!val)
        return E_FAIL;

   if (!val->pItem)
       return E_FAIL;

    return val->pItem->Rename(lpName);
}

HRESULT WINAPI CNetworkConnections::GetDefaultSearchGUID(GUID * pguid)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::GetDefaultColumn(DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CNetworkConnections::GetDefaultColumnState(UINT iColumn, DWORD * pcsFlags)
{
    if (!pcsFlags || iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetConnectSFHeader[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CNetworkConnections::GetDetailsEx(
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::GetDetailsOf(
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;

    if (iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetConnectSFHeader[iColumn].fmt;
    psd->cxChar = NetConnectSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(netshell_hInstance, NetConnectSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &psd->str);
    }

    val = _ILGetValueStruct(pidl);
    if (!val)
        return E_FAIL;

   if (!val->pItem)
       return E_FAIL;

    if (val->pItem->GetProperties(&pProperties) != S_OK)
        return E_FAIL;


    switch(iColumn)
    {
        case COLUMN_TYPE:
            if (pProperties->MediaType  == NCM_LAN || pProperties->MediaType == NCM_SHAREDACCESSHOST_RAS)
            {
                if (LoadStringW(netshell_hInstance, IDS_TYPE_ETHERNET, buffer, MAX_PATH))
                {
                    psd->str.uType = STRRET_WSTR;
                    hr = SHStrDupW(buffer, &psd->str.pOleStr);
                }
            }
            break;
        case COLUMN_STATUS:
            buffer[0] = L'\0';
            if (pProperties->Status == NCS_HARDWARE_DISABLED)
                LoadStringW(netshell_hInstance, IDS_STATUS_NON_OPERATIONAL, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_DISCONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_UNREACHABLE, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_MEDIA_DISCONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_DISCONNECTED, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_CONNECTING)
                LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTING, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_CONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTED, buffer, MAX_PATH);

            if (buffer[0])
            {
                buffer[MAX_PATH-1] = L'\0';
                psd->str.uType = STRRET_WSTR;
                hr = SHStrDupW(buffer, &psd->str.pOleStr);
            }
            break;
        case COLUMN_DEVNAME:
            if (pProperties->pszwDeviceName)
            {
                wcscpy(buffer, pProperties->pszwDeviceName);
                buffer[MAX_PATH-1] = L'\0';
                psd->str.uType = STRRET_WSTR;
                hr = SHStrDupW(buffer, &psd->str.pOleStr);
            }
            else
            {
                psd->str.cStr[0] = '\0';
                psd->str.uType = STRRET_CSTR;
            }
            break;
        case COLUMN_PHONE:
        case COLUMN_OWNER:
            psd->str.cStr[0] = '\0';
            psd->str.uType = STRRET_CSTR;
            break;
    }

    NcFreeNetconProperties(pProperties);
    return hr;
}

HRESULT WINAPI CNetworkConnections::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return E_NOTIMPL;
}

//IObjectWithSite
//IInternetSecurityManager

/**************************************************************************
* IContextMenu2 Implementation
*/

CNetConUiObject::CNetConUiObject(LPCITEMIDLIST apidl, IOleCommandTarget *lpOleCmd)
{
    this->apidl = apidl;
    pUnknown = NULL;
    this->lpOleCmd = lpOleCmd;
    ref = 0;
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_QueryInterface
 */
HRESULT WINAPI CNetConUiObject::QueryInterface(REFIID iid, LPVOID *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(iid, IID_IContextMenu) || IsEqualIID(iid, IID_IContextMenu2) || IsEqualIID(iid, IID_IContextMenu3))
        *ppvObject = (IContextMenu3*)this;
    else if (IsEqualIID(iid, IID_IObjectWithSite))
        *ppvObject = (IObjectWithSite*)this;
    else if (IsEqualIID(iid, IID_IQueryInfo))
        *ppvObject = (IQueryInfo*)this;
    else if(IsEqualIID(iid, IID_IExtractIconW))
        *ppvObject = (IExtractIconW*)this;

    if (*ppvObject)
    {
        InterlockedIncrement(&ref);
        return S_OK;
    }

    //LPOLESTR pStr;
    //StringFromCLSID(iid, &pStr);
    //MessageBoxW(NULL, L"ISF_NetConnect_IContextMenu2_QueryInterface unhandled", pStr, MB_OK);
    return E_NOINTERFACE;
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_AddRef
 */
ULONG WINAPI CNetConUiObject::AddRef()
{
    ULONG refCount;

    refCount = InterlockedIncrement(&ref);

    return refCount;
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_Release
 */
ULONG WINAPI CNetConUiObject::Release()
{
    ULONG refCount;

    refCount = InterlockedDecrement(&ref);
    if (!refCount)
        delete this;

    return refCount;
}

void WINAPI _InsertMenuItemW (
    HMENU hmenu,
    UINT indexMenu,
    BOOL fByPosition,
    UINT wID,
    UINT fType,
    LPCWSTR dwTypeData,
    UINT fState)
{
    MENUITEMINFOW mii;
    WCHAR szText[100];

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    if (fType == MFT_SEPARATOR)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE;
    }
    else if (fType == MFT_STRING)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        if ((ULONG_PTR)HIWORD((ULONG_PTR)dwTypeData) == 0)
        {
            if (LoadStringW(netshell_hInstance, LOWORD((ULONG_PTR)dwTypeData), szText, sizeof(szText)/sizeof(WCHAR)))
            {
                szText[(sizeof(szText)/sizeof(WCHAR))-1] = 0;
                mii.dwTypeData = szText;
            }
            else
            {
                return;
            }
        }
        else
        {
            mii.dwTypeData = (LPWSTR) dwTypeData;
        }
        mii.fState = fState;
    }

    mii.wID = wID;
    mii.fType = fType;
    InsertMenuItemW( hmenu, indexMenu, fByPosition, &mii);
}

/**************************************************************************
* ISF_NetConnect_IContextMenu_QueryContextMenu()
*/
HRESULT WINAPI CNetConUiObject::QueryContextMenu(
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;

    val = _ILGetValueStruct(apidl);
    if (!val)
        return E_FAIL;

    if (val->pItem->GetProperties(&pProperties) != S_OK)
        return E_FAIL;

    if (pProperties->Status == NCS_HARDWARE_DISABLED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_ACTIVATE, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_ACTIVATE), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_DEACTIVATE, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DEACTIVATE), MFS_ENABLED);

    if (pProperties->Status == NCS_HARDWARE_DISABLED || pProperties->Status == NCS_MEDIA_DISCONNECTED || pProperties->Status == NCS_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_STATUS, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_GRAYED);
    else if (pProperties->Status == NCS_CONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_STATUS, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_STATUS, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_ENABLED);

    if (pProperties->Status == NCS_HARDWARE_DISABLED || pProperties->Status == NCS_MEDIA_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_REPAIR, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_GRAYED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_REPAIR, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_ENABLED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_CREATELINK, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_CREATELINK), MFS_ENABLED);

    if (pProperties->dwCharacter & NCCF_ALLOW_REMOVAL)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_DELETE, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_DELETE, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_GRAYED);

    if (pProperties->dwCharacter & NCCF_ALLOW_RENAME)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_RENAME, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_RENAME, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_GRAYED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_NET_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_PROPERTIES), MFS_ENABLED);
    NcFreeNetconProperties(pProperties);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 9);
}

BOOL
CALLBACK
PropSheetExCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pinfo = (PROPSHEETHEADERW *)lParam;

    if (pinfo->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pinfo->phpage[pinfo->nPages++] = hPage;
        return TRUE;
    }
    return FALSE;
}

HRESULT
ShowNetConnectionStatus(
    IOleCommandTarget *lpOleCmd,
    INetConnection *pNetConnect,
    HWND hwnd)
{
    NETCON_PROPERTIES *pProperties;
    HRESULT hr;

    if (!lpOleCmd)
        return E_FAIL;

    if (pNetConnect->GetProperties(&pProperties) != S_OK)
        return E_FAIL;

    hr = lpOleCmd->Exec(&pProperties->guidId, 2, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

    NcFreeNetconProperties(pProperties);
    return hr;
}

HRESULT
ShowNetConnectionProperties(
    INetConnection *pNetConnect,
    HWND hwnd)
{
    HRESULT hr;
    CLSID ClassID;
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    INetConnectionPropertyUi * pNCP;
    NETCON_PROPERTIES * pProperties;

    if (pNetConnect->GetProperties(&pProperties) != S_OK)
        return E_FAIL;

    hr = pNetConnect->GetUiObjectClassId(&ClassID);
    if (FAILED(hr))
    {
        NcFreeNetconProperties(pProperties);
        return hr;
    }

    hr = CoCreateInstance(ClassID, NULL, CLSCTX_INPROC_SERVER, IID_INetConnectionPropertyUi, (LPVOID*)&pNCP);
    if (FAILED(hr))
    {
        NcFreeNetconProperties(pProperties);
        return hr;
    }

    hr = pNCP->SetConnection(pNetConnect);
    if (SUCCEEDED(hr))
    {
        ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
        ZeroMemory(hppages, sizeof(hppages));
        pinfo.dwSize = sizeof(PROPSHEETHEADERW);
        pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW;
        pinfo.phpage = hppages;
        pinfo.hwndParent = hwnd;

        pinfo.pszCaption = pProperties->pszwName;
        hr = pNCP->AddPages(hwnd, PropSheetExCallback, (LPARAM)&pinfo);
        if (SUCCEEDED(hr))
        {
            if(PropertySheetW(&pinfo) < 0)
                hr = E_FAIL;
        }
    }
    pNCP->Release();
    NcFreeNetconProperties(pProperties);
    return hr;
}


/**************************************************************************
* ISF_NetConnect_IContextMenu_InvokeCommand()
*/
HRESULT WINAPI CNetConUiObject::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    VALUEStruct * val;

    val = _ILGetValueStruct(apidl);
    if (!val)
        return E_FAIL;

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_NET_STATUS) ||
        lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_NET_STATUS-1)) //HACK for Windows XP
    {
        return ShowNetConnectionStatus(lpOleCmd, val->pItem, lpcmi->hwnd);
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_NET_PROPERTIES) ||
             lpcmi->lpVerb == MAKEINTRESOURCEA(10099)) //HACK for Windows XP
    {
        /* FIXME perform version checks */
        return ShowNetConnectionProperties(val->pItem, lpcmi->hwnd);
    }

    return S_OK;
}

/**************************************************************************
 *  ISF_NetConnect_IContextMenu_GetCommandString()
 *
 */
HRESULT WINAPI CNetConUiObject::GetCommandString(
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	return E_FAIL;
}

/**************************************************************************
* ISF_NetConnect_IContextMenu_HandleMenuMsg()
*/
HRESULT WINAPI CNetConUiObject::HandleMenuMsg(
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetConUiObject::HandleMenuMsg2(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetConUiObject::GetSite(REFIID riid, PVOID *ppvSite)
{
    HRESULT hr;
    IUnknown *pUnknown;

    if (!this->pUnknown)
    {
        *ppvSite = NULL;
        return E_FAIL;
    }

    hr = this->pUnknown->QueryInterface(riid, (LPVOID*)&pUnknown);
    if (SUCCEEDED(hr))
    {
        pUnknown->AddRef();
        *ppvSite = pUnknown;
        return S_OK;
    }

    *ppvSite = NULL;
    return hr;
}

HRESULT WINAPI CNetConUiObject::SetSite(IUnknown *pUnkSite)
{
    if(!pUnkSite)
    {
        if (this->pUnknown)
        {
            this->pUnknown->Release();
            this->pUnknown = NULL;
        }
    }
    else
    {
        pUnkSite->AddRef();
        if (this->pUnknown)
            this->pUnknown->Release();
        this->pUnknown = pUnkSite;
    }

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_GetIconLocation
 */
HRESULT WINAPI CNetConUiObject::GetIconLocation(
    UINT uFlags,
    LPWSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    VALUEStruct *val;
    NETCON_PROPERTIES *pProperties;

    *pwFlags = 0;
    if (!GetModuleFileNameW(netshell_hInstance, szIconFile, cchMax))
    {
        ERR("GetModuleFileNameW failed\n");
        return E_FAIL;
    }

    val = _ILGetValueStruct(apidl);
    if (!val)
    {
        ERR("_ILGetValueStruct failed\n");
        return E_FAIL;
    }

    if (val->pItem->GetProperties(&pProperties) != NOERROR)
    {
        ERR("INetConnection_GetProperties failed\n");
        return E_FAIL;
    }

    if (pProperties->Status == NCS_CONNECTED || pProperties->Status == NCS_CONNECTING)
        *piIndex = -IDI_NET_IDLE;
    else
        *piIndex = -IDI_NET_OFF;

    NcFreeNetconProperties(pProperties);

    return NOERROR;
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_Extract
 */
HRESULT WINAPI CNetConUiObject::Extract(
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    //IContextMenuImpl * This = impl_from_IExtractIcon(iface);
    if (nIconIndex == IDI_NET_IDLE)
    {
        *phiconLarge = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        *phiconSmall = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        return NOERROR;
    }
    else if (nIconIndex == IDI_NET_OFF)
    {
        *phiconLarge = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        *phiconSmall = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        return NOERROR;
    }

    return S_FALSE;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetClassID
 */
HRESULT WINAPI CNetworkConnections::GetClassID(CLSID *lpClassId)
{
    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkConnections;

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CNetworkConnections::Initialize(LPCITEMIDLIST pidl)
{
    SHFree(pidlRoot);
    pidlRoot = ILClone(pidl);

    return S_OK;
}

/**************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetCurFolder
 */
HRESULT WINAPI CNetworkConnections::GetCurFolder(LPITEMIDLIST *pidl)
{
    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_ShellExecuteHookW_Execute
 */
HRESULT WINAPI CNetworkConnections::Execute(LPSHELLEXECUTEINFOW pei)
{
    VALUEStruct *val;
    NETCON_PROPERTIES * pProperties;

    val = _ILGetValueStruct(ILFindLastID((ITEMIDLIST*)pei->lpIDList));
    if (!val)
        return E_FAIL;

    if (val->pItem->GetProperties(&pProperties) != NOERROR)
        return E_FAIL;

    if (pProperties->Status == NCS_CONNECTED)
    {
        NcFreeNetconProperties(pProperties);
        return ShowNetConnectionStatus(lpOleCmd, val->pItem, pei->hwnd);
    }

    NcFreeNetconProperties(pProperties);

    return S_OK;
}

HRESULT WINAPI CNetConUiObject::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CNetConUiObject::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    *ppwszTip = NULL;
    return S_OK;
}

/**************************************************************************
*	ISF_NetConnect_Constructor
*/
HRESULT WINAPI ISF_NetConnect_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    TRACE("ISF_NetConnect_Constructor\n");

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CNetworkConnections *pnc = new CNetworkConnections;
    if (!pnc)
        return E_OUTOFMEMORY;

    pnc->AddRef();
    HRESULT hr = pnc->QueryInterface(riid, ppv);
    pnc->Release();

    return hr;
}
