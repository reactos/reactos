/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CNetworkConnections Shell Folder
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

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

CNetworkConnections::CNetworkConnections() :
    m_pidlRoot(_ILCreateNetConnect())
{
    HRESULT hr;
    hr = CoCreateInstance(CLSID_ConnectionTray, NULL, CLSCTX_INPROC_SERVER, IID_IOleCommandTarget, reinterpret_cast<PVOID*>(&m_lpOleCmd));
    if (FAILED(hr))
    {
        ERR("CoCreateInstance failed with %lx\n", hr);
        m_lpOleCmd = NULL;
    }
}

CNetworkConnections::~CNetworkConnections()
{
    if (m_lpOleCmd)
        m_lpOleCmd->Release();
    SHFree(m_pidlRoot);
}

/**************************************************************************
*	ISF_NetConnect_fnParseDisplayName
*/
HRESULT WINAPI CNetworkConnections::ParseDisplayName (
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, PIDLIST_RELATIVE * ppidl, DWORD * pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    return hr;
}

/**************************************************************************
*		ISF_NetConnect_fnEnumObjects
*/
HRESULT WINAPI CNetworkConnections::EnumObjects(
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return CEnumIDList_CreateInstance(hwndOwner, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
*		ISF_NetConnect_fnBindToObject
*/
HRESULT WINAPI CNetworkConnections::BindToObject (
               PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnBindToStorage
*/
HRESULT WINAPI CNetworkConnections::BindToStorage(
               PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetConnect_fnCompareIDs
*/

HRESULT WINAPI CNetworkConnections::CompareIDs(
               LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
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
        cvf.pshf = static_cast<IShellFolder*>(this);

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

    if (cidl == 0)
    {
        *rgfInOut = dwNetConnectAttributes;
    }
    else
    {
        *rgfInOut = dwNetConnectItemAttributes;

        while (cidl > 0 && *apidl)
        {
            const VALUEStruct * val;
            NETCON_PROPERTIES * pProperties;

            val = _ILGetValueStruct(*apidl);
            if (!val)
                continue;

            if (val->pItem->GetProperties(&pProperties) != S_OK)
                continue;

            if (!(pProperties->dwCharacter & NCCF_ALLOW_RENAME))
                *rgfInOut &= ~SFGAO_CANRENAME;

            apidl++;
            cidl--;
        }
    }

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

HRESULT WINAPI CNetworkConnections::GetUIObjectOf(
               HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID(riid, IID_IContextMenu) || IsEqualIID (riid, IID_IContextMenu2) || IsEqualIID(riid, IID_IContextMenu3) ||
         IsEqualIID(riid, IID_IQueryInfo) || IsEqualIID(riid, IID_IExtractIconW)) && cidl >= 1)
    {
        return ShellObjectCreatorInit<CNetConUiObject>(apidl[0], m_lpOleCmd, riid, ppvOut);
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
HRESULT WINAPI CNetworkConnections::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszName;
    HRESULT hr = E_FAIL;
    NETCON_PROPERTIES * pProperties;
    const VALUEStruct * val;

    if (!strRet)
        return E_INVALIDARG;

    pszName = static_cast<LPWSTR>(CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR)));
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
               HWND hwndOwner, PCUITEMID_CHILD pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD * pPidlOut)
{
    const VALUEStruct * val;
    HRESULT hr;

    val = _ILGetValueStruct(pidl);
    if (!val)
        return E_FAIL;

   if (!val->pItem)
       return E_FAIL;

    hr = val->pItem->Rename(lpName);
    if (FAILED(hr))
        return hr;

    /* The pidl hasn't changed */
    *pPidlOut = ILClone(pidl);

    return S_OK;
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
               PCUITEMID_CHILD pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::GetDetailsOf(
               PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS * psd)
{
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;
    const VALUEStruct * val;
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


    switch (iColumn)
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

CNetConUiObject::CNetConUiObject()
    : m_pidl(NULL),
      m_pUnknown(NULL),
      m_lpOleCmd(NULL)
{
}

CNetConUiObject::~CNetConUiObject()
{
    if (m_lpOleCmd)
        m_lpOleCmd->Release();
}

HRESULT WINAPI CNetConUiObject::Initialize(PCUITEMID_CHILD pidl, IOleCommandTarget *lpOleCmd)
{
    m_pidl = pidl;
    m_lpOleCmd = lpOleCmd;
    if (m_lpOleCmd)
        m_lpOleCmd->AddRef();
    return S_OK;
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
    const VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;

    val = _ILGetValueStruct(m_pidl);
    if (!val)
        return E_FAIL;

    if (val->pItem->GetProperties(&pProperties) != S_OK)
        return E_FAIL;

    if (pProperties->Status == NCS_HARDWARE_DISABLED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_ACTIVATE), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 1, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DEACTIVATE), MFS_ENABLED);

    if (pProperties->Status == NCS_HARDWARE_DISABLED || pProperties->Status == NCS_MEDIA_DISCONNECTED || pProperties->Status == NCS_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_GRAYED);
    else if (pProperties->Status == NCS_CONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_ENABLED);

    if (pProperties->Status == NCS_HARDWARE_DISABLED || pProperties->Status == NCS_MEDIA_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 3, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_GRAYED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 3, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_ENABLED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 4, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_CREATELINK), MFS_ENABLED);

    if (pProperties->dwCharacter & NCCF_ALLOW_REMOVAL)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 5, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 5, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_GRAYED);

    if (pProperties->dwCharacter & NCCF_ALLOW_RENAME)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 6, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 6, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_GRAYED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    if (pProperties->Status == NCS_CONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 7, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_PROPERTIES), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 7, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_PROPERTIES),  MFS_DEFAULT);
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

    hr = lpOleCmd->Exec(&pProperties->guidId, OLECMDID_NEW, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

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
            if (PropertySheetW(&pinfo) < 0)
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
    const VALUEStruct * val;
    UINT CmdId;

    val = _ILGetValueStruct(m_pidl);
    if (!val)
        return E_FAIL;

    /* We should get this when F2 is pressed in explorer */
    if (HIWORD(lpcmi->lpVerb) && !strcmp(lpcmi->lpVerb, "rename"))
        lpcmi->lpVerb = MAKEINTRESOURCEA(IDS_NET_RENAME);

    if (HIWORD(lpcmi->lpVerb) || LOWORD(lpcmi->lpVerb) > 7)
    {
        FIXME("Got invalid command\n");
        return E_NOTIMPL;
    }

    CmdId = LOWORD(lpcmi->lpVerb) + IDS_NET_ACTIVATE;

    switch(CmdId)
    {
        case IDS_NET_ACTIVATE:
        case IDS_NET_DEACTIVATE:
        case IDS_NET_REPAIR:
        case IDS_NET_CREATELINK:
        case IDS_NET_DELETE:
            FIXME("Command %u is not implemented\n", CmdId);
            return E_NOTIMPL;
        case IDS_NET_RENAME:
        {
            HRESULT hr;
            IShellView *psv;
            hr = IUnknown_QueryService(m_pUnknown, SID_IFolderView, IID_IShellView, (PVOID*)&psv);
            if (SUCCEEDED(hr))
            {
                SVSIF selFlags = SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT;
                psv->SelectItem(m_pidl, selFlags);
            }
            psv->Release();

            return S_OK;
        }
        case IDS_NET_STATUS:
            return ShowNetConnectionStatus(m_lpOleCmd, val->pItem, lpcmi->hwnd);
        case IDS_NET_PROPERTIES:
            return ShowNetConnectionProperties(val->pItem, lpcmi->hwnd);
    }

    return E_NOTIMPL;
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

    if (!m_pUnknown)
    {
        *ppvSite = NULL;
        return E_FAIL;
    }

    hr = m_pUnknown->QueryInterface(riid, reinterpret_cast<PVOID*>(&pUnknown));
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
    if (!pUnkSite)
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }
    else
    {
        pUnkSite->AddRef();
        if (m_pUnknown)
            m_pUnknown->Release();
        m_pUnknown = pUnkSite;
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
    const VALUEStruct *val;
    NETCON_PROPERTIES *pProperties;

    *pwFlags = 0;
    if (!GetModuleFileNameW(netshell_hInstance, szIconFile, cchMax))
    {
        ERR("GetModuleFileNameW failed\n");
        return E_FAIL;
    }

    val = _ILGetValueStruct(m_pidl);
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

    *lpClassId = CLSID_ConnectionFolder;

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CNetworkConnections::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    SHFree(m_pidlRoot);
    m_pidlRoot = ILClone(pidl);

    return S_OK;
}

/**************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetCurFolder
 */
HRESULT WINAPI CNetworkConnections::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(m_pidlRoot);

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_ShellExecuteHookW_Execute
 */
HRESULT WINAPI CNetworkConnections::Execute(LPSHELLEXECUTEINFOW pei)
{
    const VALUEStruct *val;
    NETCON_PROPERTIES * pProperties;

    val = _ILGetValueStruct(ILFindLastID((ITEMIDLIST*)pei->lpIDList));
    if (!val)
        return E_FAIL;

    if (val->pItem->GetProperties(&pProperties) != NOERROR)
        return E_FAIL;

    if (pProperties->Status == NCS_CONNECTED)
    {
        NcFreeNetconProperties(pProperties);
        return ShowNetConnectionStatus(m_lpOleCmd, val->pItem, pei->hwnd);
    }

    NcFreeNetconProperties(pProperties);

    return S_OK;
}

HRESULT WINAPI CNetworkConnections::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    FIXME("CNetworkConnections::Initialize()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("CNetworkConnections::Exec()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    FIXME("CNetworkConnections::QueryStatus()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CNetworkConnections::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("CNetworkConnections::MessageSFVCB()\n");
    return E_NOTIMPL;
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
