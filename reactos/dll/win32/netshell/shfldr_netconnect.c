/*
 * Network Connections Shell Folder
 *
 * Copyright 2008       Johannes Anderwald <janderwald@reactos.org>
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

#define MAX_PROPERTY_SHEET_PAGE (10)

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl  *lpVtbl;
    LONG                       ref;

    const IPersistFolder2Vtbl *lpVtblPersistFolder2;
    const IShellExecuteHookWVtbl *lpVtblShellExecuteHookW;
    //const IPersistIDListVtbl *lpVtblPersistIDList;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
    LPITEMIDLIST pidl; /* enumerated pidl */
    IOleCommandTarget * lpOleCmd;
} IGenericSFImpl, *LPIGenericSFImpl;

typedef struct
{
    const IContextMenu3Vtbl *lpVtblContextMenu;
    const IObjectWithSiteVtbl *lpVtblObjectWithSite;
    const IQueryInfoVtbl *lpVtblQueryInfo;
    const IExtractIconWVtbl *lpVtblExtractIconW;
    LONG                       ref;
    LPCITEMIDLIST apidl;
    IUnknown *pUnknown;
    IOleCommandTarget * lpOleCmd;
}IContextMenuImpl, *LPIContextMenuImpl;

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

static const IContextMenu3Vtbl vt_ContextMenu3;
static const IObjectWithSiteVtbl vt_ObjectWithSite;
static const IQueryInfoVtbl vt_QueryInfo;
static const IExtractIconWVtbl vt_ExtractIconW;

static LPIContextMenuImpl __inline impl_from_IExtractIcon(IExtractIconW *iface)
{
    return (LPIContextMenuImpl)((char *)iface - FIELD_OFFSET(IContextMenuImpl, lpVtblExtractIconW));
}


static LPIContextMenuImpl __inline impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (LPIContextMenuImpl)((char *)iface - FIELD_OFFSET(IContextMenuImpl, lpVtblObjectWithSite));
}

static LPIGenericSFImpl __inline impl_from_IPersistFolder2(IPersistFolder2 *iface)
{
    return (LPIGenericSFImpl)((char *)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblPersistFolder2));
}

static LPIGenericSFImpl __inline impl_from_IShellExecuteHookW(IShellExecuteHookW *iface)
{
    return (LPIGenericSFImpl)((char *)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblShellExecuteHookW));
}

static LPIContextMenuImpl __inline impl_from_IQueryInfo(IQueryInfo *iface)
{
    return (LPIContextMenuImpl)((char *)iface - FIELD_OFFSET(IContextMenuImpl, lpVtblQueryInfo));
}


/**************************************************************************
 *	ISF_NetConnect_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_NetConnect_fnQueryInterface (IShellFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = (LPVOID *)&This->lpVtblPersistFolder2;
    }
    else if (IsEqualIID(riid, &IID_IShellExecuteHookW))
    {
        *ppvObj = (LPVOID *)&This->lpVtblShellExecuteHookW;
    }
#if 0
    else if (IsEqualIID(riid, &IID_IPersistIDList))
    {
        //*ppvObj = (LPVOID *)&This->lpVtblPersistIDList;
    }
#endif
    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
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

static ULONG WINAPI ISF_NetConnect_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static ULONG WINAPI ISF_NetConnect_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);


    if (!refCount) 
    {
        SHFree (This->pidlRoot);
        CoTaskMemFree(This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_NetConnect_fnParseDisplayName
*/
static HRESULT WINAPI ISF_NetConnect_fnParseDisplayName (IShellFolder2 * iface,
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    HRESULT hr = E_UNEXPECTED;

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    return hr;
}

/**************************************************************************
 *  CreateNetConnectEnumListss()
 */
static BOOL CreateNetConnectEnumList(IEnumIDList *list, DWORD dwFlags)
{
    HRESULT hr;
    INetConnectionManager * INetConMan;
    IEnumNetConnection * IEnumCon;
    INetConnection * INetCon;
    ULONG Count;
    LPITEMIDLIST pidl;

    /* get an instance to of IConnectionManager */
    hr = INetConnectionManager_Constructor(NULL, &IID_INetConnectionManager, (LPVOID*)&INetConMan);
    if (FAILED(hr))
        return FALSE;

    hr = INetConnectionManager_EnumConnections(INetConMan, NCME_DEFAULT, &IEnumCon);
    if (FAILED(hr))
    {
        INetConnectionManager_Release(INetConMan);
        return FALSE;
    }

    do
    {
        hr = IEnumNetConnection_Next(IEnumCon, 1, &INetCon, &Count);
        if (hr == S_OK)
        {
            pidl = ILCreateNetConnectItem(INetCon);
            if (pidl)
            {
                AddToEnumList(list, pidl);
            }
        }
        else
        {
            break;
        }
    }while(TRUE);

    IEnumNetConnection_Release(IEnumCon);
    INetConnectionManager_Release(INetConMan);

    return TRUE;
}

/**************************************************************************
*		ISF_NetConnect_fnEnumObjects
*/
static HRESULT WINAPI ISF_NetConnect_fnEnumObjects (IShellFolder2 * iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    *ppEnumIDList = IEnumIDList_Constructor();
    if(*ppEnumIDList)
        CreateNetConnectEnumList(*ppEnumIDList, dwFlags);


    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		ISF_NetConnect_fnBindToObject
*/
static HRESULT WINAPI ISF_NetConnect_fnBindToObject (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnBindToStorage
*/
static HRESULT WINAPI ISF_NetConnect_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;


    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetConnect_fnCompareIDs
*/

static HRESULT WINAPI ISF_NetConnect_fnCompareIDs (IShellFolder2 * iface,
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;



    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnCreateViewObject
*/
static HRESULT WINAPI ISF_NetConnect_fnCreateViewObject (IShellFolder2 * iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
    IShellView* pShellView;
    CSFV cvf;
    HRESULT hr = E_NOINTERFACE;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IShellView))
    {
        ZeroMemory(&cvf, sizeof(cvf));
        cvf.cbSize = sizeof(cvf);
        cvf.pshf = (IShellFolder*)iface;

        hr = SHCreateShellFolderViewEx(&cvf, &pShellView);
        if (SUCCEEDED(hr))
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }

    return hr;
}

/**************************************************************************
*  ISF_NetConnect_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_NetConnect_fnGetAttributesOf (IShellFolder2 * iface,
               UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
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
static HRESULT IContextMenuImpl_Constructor(REFIID riid, LPCITEMIDLIST apidl, LPVOID * ppvOut, IOleCommandTarget * lpOleCmd)
{
    IUnknown *pObj = NULL;
    IContextMenuImpl * pMenu = CoTaskMemAlloc(sizeof(IContextMenuImpl));
    if (!pMenu)
        return E_OUTOFMEMORY;

    ZeroMemory(pMenu, sizeof(IContextMenuImpl));

    pMenu->apidl = apidl;
    pMenu->lpVtblContextMenu = &vt_ContextMenu3;
    pMenu->lpVtblObjectWithSite = &vt_ObjectWithSite;
    pMenu->lpVtblExtractIconW = &vt_ExtractIconW;
    pMenu->lpVtblQueryInfo = &vt_QueryInfo;
    pMenu->pUnknown = NULL;
    pMenu->lpOleCmd = lpOleCmd;
    pMenu->ref = 1;

    if (IsEqualIID(riid, &IID_IContextMenu) || IsEqualIID(riid, &IID_IContextMenu2)|| IsEqualIID(riid, &IID_IContextMenu3))
        pObj = (IUnknown*)(&pMenu->lpVtblContextMenu);
    else if(IsEqualIID(riid, &IID_IQueryInfo))
        pObj = (IUnknown*)(&pMenu->lpVtblQueryInfo);
    else if(IsEqualIID(riid, &IID_IExtractIconW))
        pObj = (IUnknown*)(&pMenu->lpVtblExtractIconW);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef(pObj);


    *ppvOut = pObj;
    return S_OK;
}

static HRESULT WINAPI ISF_NetConnect_fnGetUIObjectOf (IShellFolder2 * iface,
               HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID (riid, &IID_IContextMenu) || IsEqualIID (riid, &IID_IContextMenu2) || IsEqualIID(riid, &IID_IContextMenu3) ||
         IsEqualIID(riid, &IID_IQueryInfo) || IsEqualIID(riid, &IID_IExtractIconW)) && (cidl >= 1))
    {
        return IContextMenuImpl_Constructor(riid, apidl[0], ppvOut, This->lpOleCmd);
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
static HRESULT WINAPI ISF_NetConnect_fnGetDisplayNameOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszName;
    HRESULT hr = E_FAIL;
    NETCON_PROPERTIES * pProperties;
    VALUEStruct * val;
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (!strRet)
        return E_INVALIDARG;

    pszName = CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR));
    if (!pszName)
        return E_OUTOFMEMORY;

    if (_ILIsNetConnect (pidl))
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
            if (INetConnection_GetProperties(val->pItem, &pProperties) == NOERROR)
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
        strRet->u.pOleStr = pszName;
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
static HRESULT WINAPI ISF_NetConnect_fnSetNameOf (IShellFolder2 * iface,
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultSearchGUID (
               IShellFolder2 * iface, GUID * pguid)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnEnumSearches (IShellFolder2 * iface,
               IEnumExtraSearch ** ppenum)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultColumn (IShellFolder2 * iface,
               DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (!pcsFlags || iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetConnectSFHeader[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
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
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &psd->str);
    }

    val = _ILGetValueStruct(pidl);
    if (!val)
        return E_FAIL;

   if (!val->pItem)
       return E_FAIL;

    if (INetConnection_GetProperties((INetConnection*)val->pItem, &pProperties) != NOERROR)
        return E_FAIL;


    switch(iColumn)
    {
        case COLUMN_TYPE:
            if (pProperties->MediaType  == NCM_LAN || pProperties->MediaType == NCM_SHAREDACCESSHOST_RAS)
            {
                if (LoadStringW(netshell_hInstance, IDS_TYPE_ETHERNET, buffer, MAX_PATH))
                {
                    psd->str.uType = STRRET_WSTR;
                    hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
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
                hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
            }
            break;
        case COLUMN_DEVNAME:
            if (pProperties->pszwDeviceName)
            {
                wcscpy(buffer, pProperties->pszwDeviceName);
                buffer[MAX_PATH-1] = L'\0';
                psd->str.uType = STRRET_WSTR;
                hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
            }
            break;
        case COLUMN_PHONE:
        case COLUMN_OWNER:
            psd->str.u.cStr[0] = '\0';
            psd->str.uType = STRRET_CSTR;
            break;
    }
#if 0
    NcFreeNetconProperties(pProperties);
#endif
    return hr;
}

static HRESULT WINAPI ISF_NetConnect_fnMapColumnToSCID (IShellFolder2 * iface,
               UINT column, SHCOLUMNID * pscid)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 = {
    ISF_NetConnect_fnQueryInterface,
    ISF_NetConnect_fnAddRef,
    ISF_NetConnect_fnRelease,
    ISF_NetConnect_fnParseDisplayName,
    ISF_NetConnect_fnEnumObjects,
    ISF_NetConnect_fnBindToObject,
    ISF_NetConnect_fnBindToStorage,
    ISF_NetConnect_fnCompareIDs,
    ISF_NetConnect_fnCreateViewObject,
    ISF_NetConnect_fnGetAttributesOf,
    ISF_NetConnect_fnGetUIObjectOf,
    ISF_NetConnect_fnGetDisplayNameOf,
    ISF_NetConnect_fnSetNameOf,
    /* ShellFolder2 */
    ISF_NetConnect_fnGetDefaultSearchGUID,
    ISF_NetConnect_fnEnumSearches,
    ISF_NetConnect_fnGetDefaultColumn,
    ISF_NetConnect_fnGetDefaultColumnState,
    ISF_NetConnect_fnGetDetailsEx,
    ISF_NetConnect_fnGetDetailsOf,
    ISF_NetConnect_fnMapColumnToSCID
};
//IObjectWithSite
//IInternetSecurityManager

/**************************************************************************
* IContextMenu2 Implementation
*/

/************************************************************************
 * ISF_NetConnect_IContextMenu_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_IContextMenu3_QueryInterface(IContextMenu3 * iface, REFIID iid, LPVOID * ppvObject)
{
    //LPOLESTR pStr;
    IContextMenuImpl * This = (IContextMenuImpl*)iface;

    if (IsEqualIID(iid, &IID_IContextMenu) || IsEqualIID(iid, &IID_IContextMenu2) || IsEqualIID(iid, &IID_IContextMenu3))
    {
        *ppvObject = (IUnknown*) &This->lpVtblContextMenu;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_IObjectWithSite))
    {
        *ppvObject = (IUnknown*) &This->lpVtblObjectWithSite;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_IQueryInfo))
    {
        *ppvObject = (IUnknown*) &This->lpVtblQueryInfo;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    //StringFromCLSID(iid, &pStr);
    //MessageBoxW(NULL, L"ISF_NetConnect_IContextMenu2_QueryInterface unhandled", pStr, MB_OK);
    return E_NOINTERFACE;
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_AddRef
 */
static ULONG WINAPI ISF_NetConnect_IContextMenu3_AddRef(IContextMenu3 * iface)
{
    ULONG refCount;
    IContextMenuImpl * This = (IContextMenuImpl*)iface;

    refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_Release
 */
static ULONG WINAPI ISF_NetConnect_IContextMenu3_Release(IContextMenu3  * iface)
{
    ULONG refCount;
    IContextMenuImpl * This = (IContextMenuImpl*)iface;

    refCount = InterlockedDecrement(&This->ref);
    if (!refCount) 
    {
        CoTaskMemFree(This);
    }
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
static HRESULT WINAPI ISF_NetConnect_IContextMenu3_QueryContextMenu(
	IContextMenu3 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    IContextMenuImpl * This = (IContextMenuImpl*)iface;
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;

    val = _ILGetValueStruct(This->apidl);
    if (!val)
        return E_FAIL;

    if (INetConnection_GetProperties((INetConnection*)val->pItem, &pProperties) != NOERROR)
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
        pinfo->u3.phpage[pinfo->nPages++] = hPage;
        return TRUE;
    }
    return FALSE;
}

HRESULT
ShowNetConnectionStatus(
    IOleCommandTarget * lpOleCmd,
    INetConnection * pNetConnect,
    HWND hwnd)
{
    NETCON_PROPERTIES * pProperties;
    HRESULT hr;

    if (!lpOleCmd)
        return E_FAIL;

    if (INetConnection_GetProperties(pNetConnect, &pProperties) != NOERROR)
        return E_FAIL;

    hr = IOleCommandTarget_Exec(lpOleCmd, &pProperties->guidId, 2, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

    NcFreeNetconProperties(pProperties);
    return hr;
}

HRESULT
ShowNetConnectionProperties(
    INetConnection * pNetConnect,
    HWND hwnd)
{
    HRESULT hr;
    CLSID ClassID;
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    INetConnectionPropertyUi * pNCP;
    NETCON_PROPERTIES * pProperties;

    if (INetConnection_GetProperties(pNetConnect, &pProperties) != NOERROR)
        return E_FAIL;

    hr = INetConnection_GetUiObjectClassId(pNetConnect, &ClassID);
    if (FAILED(hr))
    {
        NcFreeNetconProperties(pProperties);
        return hr;
    }

    hr = CoCreateInstance(&ClassID, NULL, CLSCTX_INPROC_SERVER, &IID_INetConnectionPropertyUi, (LPVOID)&pNCP);
    if (FAILED(hr))
    {
        NcFreeNetconProperties(pProperties);
        return hr;
    }

    hr = INetConnectionPropertyUi_SetConnection(pNCP, pNetConnect);
    if (SUCCEEDED(hr))
    {
        ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
        ZeroMemory(hppages, sizeof(hppages));
        pinfo.dwSize = sizeof(PROPSHEETHEADERW);
        pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW;
        pinfo.u3.phpage = hppages;
        pinfo.hwndParent = hwnd;

        pinfo.pszCaption = pProperties->pszwName;
        hr = INetConnectionPropertyUi_AddPages(pNCP, hwnd, PropSheetExCallback, (LPARAM)&pinfo);
        if (SUCCEEDED(hr))
        {
            if(PropertySheetW(&pinfo) < 0)
                hr = E_FAIL;
        }
    }
    INetConnectionPropertyUi_Release(pNCP);
    NcFreeNetconProperties(pProperties);
    return hr;
}


/**************************************************************************
* ISF_NetConnect_IContextMenu_InvokeCommand()
*/
static HRESULT WINAPI ISF_NetConnect_IContextMenu3_InvokeCommand(
    IContextMenu3 *iface,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    IContextMenuImpl * This = (IContextMenuImpl*)iface;
    VALUEStruct * val;

    val = _ILGetValueStruct(This->apidl);
    if (!val)
        return E_FAIL;

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_NET_STATUS) ||
        lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_NET_STATUS-1)) //HACK for Windows XP
    {
        return ShowNetConnectionStatus(This->lpOleCmd, val->pItem, lpcmi->hwnd);
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
static HRESULT WINAPI ISF_NetConnect_IContextMenu3_GetCommandString(
	IContextMenu3 *iface,
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
static HRESULT WINAPI ISF_NetConnect_IContextMenu3_HandleMenuMsg(
	IContextMenu3 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_IContextMenu3_HandleMenuMsg2(
    IContextMenu3 *iface,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult)
{
    return E_NOTIMPL;
}


static const IContextMenu3Vtbl vt_ContextMenu3 =
{
	ISF_NetConnect_IContextMenu3_QueryInterface,
	ISF_NetConnect_IContextMenu3_AddRef,
	ISF_NetConnect_IContextMenu3_Release,
	ISF_NetConnect_IContextMenu3_QueryContextMenu,
	ISF_NetConnect_IContextMenu3_InvokeCommand,
	ISF_NetConnect_IContextMenu3_GetCommandString,
	ISF_NetConnect_IContextMenu3_HandleMenuMsg,
	ISF_NetConnect_IContextMenu3_HandleMenuMsg2
};

static HRESULT WINAPI ISF_NetConnect_IObjectWithSite_QueryInterface (IObjectWithSite * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IContextMenuImpl * This = impl_from_IObjectWithSite(iface);

    if (IsEqualIID(iid, &IID_IObjectWithSite))
    {
        *ppvObj = (IUnknown*)&This->lpVtblObjectWithSite;
        return S_OK;
    }
    return E_NOINTERFACE;
}

/************************************************************************
 *	ISF_NetConnect_IQueryInfo_AddRef
 */
static ULONG WINAPI ISF_NetConnect_IObjectWithSite_AddRef (IObjectWithSite * iface)
{
    IContextMenuImpl * This = impl_from_IObjectWithSite(iface);

    return IContextMenu2_AddRef((IContextMenu2*)This);
}

/************************************************************************
 *	ISF_NetConnect_IQueryInfo_Release
 */
static ULONG WINAPI ISF_NetConnect_IObjectWithSite_Release (IObjectWithSite * iface)
{
    IContextMenuImpl * This = impl_from_IObjectWithSite(iface);

    return IContextMenu_Release((IContextMenu*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Release
 */
static HRESULT WINAPI ISF_NetConnect_IObjectWithSite_GetSite (IObjectWithSite * iface, REFIID riid, PVOID *ppvSite)
{
    HRESULT hr;
    IUnknown *pUnknown;
    IContextMenuImpl * This = impl_from_IObjectWithSite(iface);

    if (!This->pUnknown)
    {
        *ppvSite = NULL;
        return E_FAIL;
    }

    hr = IUnknown_QueryInterface(This->pUnknown, riid, (LPVOID*)&pUnknown);
    if (SUCCEEDED(hr))
    {
        IUnknown_AddRef(pUnknown);
        *ppvSite = pUnknown;
        return S_OK;
    }

    *ppvSite = NULL;
    return hr;
}

static HRESULT WINAPI ISF_NetConnect_IObjectWithSite_SetSite (IObjectWithSite * iface, IUnknown *pUnkSite)
{
    IContextMenuImpl * This = impl_from_IObjectWithSite(iface);

    if(!pUnkSite)
    {
        if (This->pUnknown)
        {
            IUnknown_Release(This->pUnknown);
            This->pUnknown = NULL;
        }
    }
    else
    {
        IUnknown_AddRef(pUnkSite);
        if (This->pUnknown)
            IUnknown_Release(This->pUnknown);
        This->pUnknown = pUnkSite;
    }

    return S_OK;
}


static const IObjectWithSiteVtbl vt_ObjectWithSite =
{
	ISF_NetConnect_IObjectWithSite_QueryInterface,
	ISF_NetConnect_IObjectWithSite_AddRef,
	ISF_NetConnect_IObjectWithSite_Release,
	ISF_NetConnect_IObjectWithSite_SetSite,
	ISF_NetConnect_IObjectWithSite_GetSite
};

static HRESULT WINAPI ISF_NetConnect_IExtractIconW_QueryInterface (IExtractIconW * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IContextMenuImpl * This = impl_from_IExtractIcon(iface);

    if (IsEqualIID(iid, &IID_IExtractIconW))
    {
        *ppvObj = (IUnknown*)&This->lpVtblExtractIconW;
        return S_OK;
    }
    return E_NOINTERFACE;
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_AddRef
 */
static ULONG WINAPI ISF_NetConnect_IExtractIconW_AddRef (IExtractIconW * iface)
{
    IContextMenuImpl * This = impl_from_IExtractIcon(iface);

    return IContextMenu2_AddRef((IContextMenu2*)This);
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_Release
 */
static ULONG WINAPI ISF_NetConnect_IExtractIconW_Release (IExtractIconW * iface)
{
    IContextMenuImpl * This = impl_from_IExtractIcon(iface);

    return IContextMenu_Release((IContextMenu*)This);
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_GetIconLocation
 */
static HRESULT WINAPI ISF_NetConnect_IExtractIconW_GetIconLocation(
    IExtractIconW * iface,
    UINT uFlags,
    LPWSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;
    IContextMenuImpl * This = impl_from_IExtractIcon(iface);

    *pwFlags = 0;
    if (!GetModuleFileNameW(netshell_hInstance, szIconFile, cchMax))
        return E_FAIL;

    val = _ILGetValueStruct(This->apidl);
    if (!val)
        return E_FAIL;

    if (INetConnection_GetProperties(val->pItem, &pProperties) != NOERROR)
        return E_FAIL;

    if (pProperties->Status == NCS_CONNECTED || pProperties->Status == NCS_CONNECTING)
        *piIndex = IDI_NET_IDLE;
    else
        *piIndex = IDI_NET_OFF;

    NcFreeNetconProperties(pProperties);

    return NOERROR;
}

/************************************************************************
 *	ISF_NetConnect_IExtractIcon_Extract
 */
static HRESULT WINAPI ISF_NetConnect_IExtractIconW_Extract(
    IExtractIconW * iface,
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    //IContextMenuImpl * This = impl_from_IExtractIcon(iface);
    if (nIconIndex == IDI_NET_IDLE)
    {
        *phiconLarge = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        *phiconSmall = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        return NOERROR;
    }
    else if (nIconIndex == IDI_NET_OFF)
    {
        *phiconLarge = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        *phiconSmall = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        return NOERROR;
    }

    return S_FALSE;
}

static const IExtractIconWVtbl vt_ExtractIconW =
{
	ISF_NetConnect_IExtractIconW_QueryInterface,
	ISF_NetConnect_IExtractIconW_AddRef,
	ISF_NetConnect_IExtractIconW_Release,
	ISF_NetConnect_IExtractIconW_GetIconLocation,
	ISF_NetConnect_IExtractIconW_Extract
};







/************************************************************************
 *	ISF_NetConnect_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_QueryInterface (IPersistFolder2 * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_QueryInterface ((IShellFolder2*)This, iid, ppvObj);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_AddRef
 */
static ULONG WINAPI ISF_NetConnect_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_AddRef((IShellFolder2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Release
 */
static ULONG WINAPI ISF_NetConnect_PersistFolder2_Release (IPersistFolder2 * iface)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_Release((IShellFolder2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetClassID
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    //IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

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
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    SHFree(This->pidlRoot);
    This->pidlRoot = ILClone(pidl);

    return S_OK;
}

/**************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetCurFolder
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);


    if (!pidl)
        return E_POINTER;

    *pidl = ILClone (This->pidlRoot);

    return S_OK;
}

static const IPersistFolder2Vtbl vt_PersistFolder2 =
{
    ISF_NetConnect_PersistFolder2_QueryInterface,
    ISF_NetConnect_PersistFolder2_AddRef,
    ISF_NetConnect_PersistFolder2_Release,
    ISF_NetConnect_PersistFolder2_GetClassID,
    ISF_NetConnect_PersistFolder2_Initialize,
    ISF_NetConnect_PersistFolder2_GetCurFolder
};

/************************************************************************
 *	ISF_NetConnect_ShellExecuteHookW_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_ShellExecuteHookW_QueryInterface (IShellExecuteHookW * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl * This = impl_from_IShellExecuteHookW(iface);

    return IShellFolder2_QueryInterface ((IShellFolder2*)This, iid, ppvObj);
}

/************************************************************************
 *	ISF_NetConnect_ShellExecuteHookW_AddRef
 */
static ULONG WINAPI ISF_NetConnect_ShellExecuteHookW_AddRef (IShellExecuteHookW * iface)
{
    IGenericSFImpl * This = impl_from_IShellExecuteHookW(iface);

    return IShellFolder2_AddRef((IShellFolder2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Release
 */
static ULONG WINAPI ISF_NetConnect_ShellExecuteHookW_Release (IShellExecuteHookW * iface)
{
    IGenericSFImpl * This = impl_from_IShellExecuteHookW(iface);

    return IShellFolder2_Release((IShellFolder2*)This);
}


/************************************************************************
 *	ISF_NetConnect_ShellExecuteHookW_Execute
 */
static HRESULT WINAPI ISF_NetConnect_ShellExecuteHookW_Execute (IShellExecuteHookW * iface,
               LPSHELLEXECUTEINFOW pei)
{
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;
    IGenericSFImpl * This = impl_from_IShellExecuteHookW(iface);

    val = _ILGetValueStruct(ILFindLastID(pei->lpIDList));
    if (!val)
        return E_FAIL;

    if (INetConnection_GetProperties((INetConnection*)val->pItem, &pProperties) != NOERROR)
        return E_FAIL;

    if (pProperties->Status == NCS_CONNECTED)
    {
        NcFreeNetconProperties(pProperties);
        return ShowNetConnectionStatus(This->lpOleCmd, val->pItem, pei->hwnd);
    }

    NcFreeNetconProperties(pProperties);

    return S_OK;
}


static const IShellExecuteHookWVtbl vt_ShellExecuteHookW =
{
    ISF_NetConnect_ShellExecuteHookW_QueryInterface,
    ISF_NetConnect_ShellExecuteHookW_AddRef,
    ISF_NetConnect_ShellExecuteHookW_Release,
    ISF_NetConnect_ShellExecuteHookW_Execute
};

#if 0
static const IPersistIDListVtbl vt_PersistIDList =
{
    ISF_NetConnect_PersistIDList_QueryInterface,
    ISF_NetConnect_PersistIDList_AddRef,
    ISF_NetConnect_PersistIDList_Release,
    ISF_NetConnect_PersistIDList_GetClassID,
    ISF_NetConnect_PersistIDList_SetIDList,
    ISF_NetConnect_PersistIDList_GetIDList,
};
#endif

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_IQueryInfo_QueryInterface (IQueryInfo * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IContextMenuImpl * This = impl_from_IQueryInfo(iface);

    if (IsEqualIID(iid, &IID_IQueryInfo))
    {
        *ppvObj = (IUnknown*)&This->lpVtblQueryInfo;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_AddRef
 */
static ULONG WINAPI ISF_NetConnect_IQueryInfo_AddRef (IQueryInfo * iface)
{
    IContextMenuImpl * This = impl_from_IQueryInfo(iface);

    return IContextMenu2_AddRef((IContextMenu2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Release
 */
static ULONG WINAPI ISF_NetConnect_IQueryInfo_Release (IQueryInfo * iface)
{
    IContextMenuImpl * This = impl_from_IQueryInfo(iface);

    return IContextMenu_Release((IContextMenu*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetClassID
 */
static HRESULT WINAPI ISF_NetConnect_IQueryInfo_GetInfoFlags (
               IQueryInfo * iface, DWORD *pdwFlags)
{
    *pdwFlags = 0;

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI ISF_NetConnect_IQueryInfo_GetInfoTip (
               IQueryInfo * iface, DWORD dwFlags, WCHAR **ppwszTip)
{
//    IGenericSFImpl * This = impl_from_IQueryInfo(iface);

    *ppwszTip = NULL;
    return S_OK;
}

static const IQueryInfoVtbl vt_QueryInfo =
{
    ISF_NetConnect_IQueryInfo_QueryInterface,
    ISF_NetConnect_IQueryInfo_AddRef,
    ISF_NetConnect_IQueryInfo_Release,
    ISF_NetConnect_IQueryInfo_GetInfoTip,
    ISF_NetConnect_IQueryInfo_GetInfoFlags
};

/**************************************************************************
*	ISF_NetConnect_Constructor
*/
HRESULT WINAPI ISF_NetConnect_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;
    HRESULT hr;

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = (IGenericSFImpl *) CoTaskMemAlloc(sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 1;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;
    sf->lpVtblShellExecuteHookW = &vt_ShellExecuteHookW;

    hr = CoCreateInstance(&CLSID_LanConnectStatusUI, NULL, CLSCTX_INPROC_SERVER, &IID_IOleCommandTarget, (LPVOID*)&sf->lpOleCmd);
    if (FAILED(hr))
       sf->lpOleCmd = NULL;
    else
    {
       IOleCommandTarget_Exec(sf->lpOleCmd, &CGID_ShellServiceObject, 2, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    }

    sf->pidlRoot = _ILCreateNetConnect();	/* my qualified pidl */

    if (!SUCCEEDED (IShellFolder2_QueryInterface ((IShellFolder2*)sf, riid, ppv)))
    {
        IShellFolder2_Release((IShellFolder2*)sf);
        return E_NOINTERFACE;
    }
    return S_OK;
}
