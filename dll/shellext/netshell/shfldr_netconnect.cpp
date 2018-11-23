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

HRESULT
ShowNetConnectionStatus(
    IOleCommandTarget *lpOleCmd,
    PCUITEMID_CHILD pidl,
    HWND hwnd);

CNetworkConnections::CNetworkConnections() :
    m_pidlRoot(NULL)
{
    HRESULT hr;
    hr = CoCreateInstance(CLSID_ConnectionTray, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IOleCommandTarget, &m_lpOleCmd));
    if (FAILED_UNEXPECTEDLY(hr))
        m_lpOleCmd = NULL;
}

CNetworkConnections::~CNetworkConnections()
{
    if (m_pidlRoot)
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
    HRESULT hr = E_NOINTERFACE;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        CSFV cvf = {sizeof(cvf), this};
        CComPtr<IShellView> pShellView;
        hr = SHCreateShellFolderViewEx(&cvf, &pShellView);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return pShellView->QueryInterface(riid, ppvOut);
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
            PNETCONIDSTRUCT pdata = ILGetConnData(*apidl);
            if (!pdata)
                continue;

            if (!(pdata->dwCharacter & NCCF_ALLOW_RENAME))
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
    if (!strRet)
        return E_INVALIDARG;

    if (!pidl)
        return SHSetStrRet(strRet, netshell_hInstance, IDS_NETWORKCONNECTION);

    PWCHAR pwchName = ILGetConnName(pidl);
    if (!pwchName)
    {
        ERR("Got invalid pidl!\n");
        return E_INVALIDARG;
    }

    return SHSetStrRet(strRet, pwchName);
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
    HRESULT hr;
    CComPtr<INetConnection> pCon;

    hr = ILGetConnection(pidl, &pCon);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pCon->Rename(lpName);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    *pPidlOut = ILCreateNetConnectItem(pCon);
    if (*pPidlOut == NULL)
        return E_FAIL;

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
    if (iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetConnectSFHeader[iColumn].fmt;
    psd->cxChar = NetConnectSFHeader[iColumn].cxChar;
    if (pidl == NULL)
        return SHSetStrRet(&psd->str, netshell_hInstance, NetConnectSFHeader[iColumn].colnameid);

    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
        return E_FAIL;

    switch (iColumn)
    {
        case COLUMN_NAME:
            return SHSetStrRet(&psd->str, ILGetConnName(pidl));
        case COLUMN_TYPE:
            if (pdata->MediaType  == NCM_LAN || pdata->MediaType == NCM_SHAREDACCESSHOST_RAS)
            {
                return SHSetStrRet(&psd->str, netshell_hInstance, IDS_TYPE_ETHERNET);
            }
            else
            {
                return SHSetStrRet(&psd->str, "");
            }
            break;
        case COLUMN_STATUS:
            switch(pdata->Status)
            {
                case NCS_HARDWARE_DISABLED: 
                    return SHSetStrRet(&psd->str, netshell_hInstance, IDS_STATUS_NON_OPERATIONAL);
                case NCS_DISCONNECTED: 
                    return SHSetStrRet(&psd->str, netshell_hInstance, IDS_STATUS_UNREACHABLE);
                case NCS_MEDIA_DISCONNECTED: 
                    return SHSetStrRet(&psd->str, netshell_hInstance, IDS_STATUS_DISCONNECTED);
                case NCS_CONNECTING: 
                    return SHSetStrRet(&psd->str, netshell_hInstance, IDS_STATUS_CONNECTING);
                case NCS_CONNECTED: 
                    return SHSetStrRet(&psd->str, netshell_hInstance, IDS_STATUS_CONNECTED);
                default: 
                    return SHSetStrRet(&psd->str, "");
            }
            break;
        case COLUMN_DEVNAME:
            return SHSetStrRet(&psd->str, ILGetDeviceName(pidl));
        case COLUMN_PHONE:
        case COLUMN_OWNER:
            return SHSetStrRet(&psd->str, "");
    }

    return E_FAIL;
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
    : m_pidl(NULL)
{
}

CNetConUiObject::~CNetConUiObject()
{
}

HRESULT WINAPI CNetConUiObject::Initialize(PCUITEMID_CHILD pidl, IOleCommandTarget *lpOleCmd)
{
    m_pidl = pidl;
    m_lpOleCmd = lpOleCmd;
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
    PNETCONIDSTRUCT pdata = ILGetConnData(m_pidl);
    if (!pdata)
    {
        ERR("Got invalid pidl!\n");
        return E_FAIL;
    }

    if (pdata->Status == NCS_HARDWARE_DISABLED || pdata->Status == NCS_MEDIA_DISCONNECTED || pdata->Status == NCS_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_ACTIVATE), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 1, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DEACTIVATE), MFS_ENABLED);

    if (pdata->Status == NCS_HARDWARE_DISABLED || pdata->Status == NCS_MEDIA_DISCONNECTED || pdata->Status == NCS_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_GRAYED);
    else if (pdata->Status == NCS_CONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_DEFAULT);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 2, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_STATUS), MFS_ENABLED);

    if (pdata->Status == NCS_HARDWARE_DISABLED || pdata->Status == NCS_MEDIA_DISCONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 3, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_GRAYED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 3, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_REPAIR), MFS_ENABLED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 4, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_CREATELINK), MFS_ENABLED);

    if (pdata->dwCharacter & NCCF_ALLOW_REMOVAL)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 5, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 5, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_DELETE), MFS_GRAYED);

    if (pdata->dwCharacter & NCCF_ALLOW_RENAME)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 6, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 6, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_RENAME), MFS_GRAYED);

    _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    if (pdata->Status == NCS_CONNECTED)
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 7, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_PROPERTIES), MFS_ENABLED);
    else
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 7, MFT_STRING, MAKEINTRESOURCEW(IDS_NET_PROPERTIES),  MFS_DEFAULT);

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
    PCUITEMID_CHILD pidl,
    HWND hwnd)
{
    if (!lpOleCmd)
        return E_FAIL;

    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
    {
        ERR("Got invalid pidl!\n");
        return E_FAIL;
    }

    return lpOleCmd->Exec(&pdata->guidId, OLECMDID_NEW, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
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
    CComPtr<INetConnectionPropertyUi> pNCP;
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
    NcFreeNetconProperties(pProperties);
    return hr;
}


/**************************************************************************
* ISF_NetConnect_IContextMenu_InvokeCommand()
*/
HRESULT WINAPI CNetConUiObject::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    UINT CmdId;

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
        case IDS_NET_RENAME:
        {
            HRESULT hr;
            CComPtr<IShellView> psv;
            hr = IUnknown_QueryService(m_pUnknown, SID_IFolderView, IID_PPV_ARG(IShellView, &psv));
            if (SUCCEEDED(hr))
            {
                SVSIF selFlags = SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT;
                psv->SelectItem(m_pidl, selFlags);
            }

            return S_OK;
        }
        case IDS_NET_STATUS:
        {
            return ShowNetConnectionStatus(m_lpOleCmd, m_pidl, lpcmi->hwnd);
        }
        case IDS_NET_REPAIR:
        case IDS_NET_CREATELINK:
        case IDS_NET_DELETE:
            FIXME("Command %u is not implemented\n", CmdId);
            return E_NOTIMPL;
    }

    HRESULT hr;
    CComPtr<INetConnection> pCon;

    hr = ILGetConnection(m_pidl, &pCon);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    switch(CmdId)
    {
        case IDS_NET_ACTIVATE:
            return pCon->Connect();
        case IDS_NET_DEACTIVATE:
            return pCon->Disconnect();
        case IDS_NET_PROPERTIES:
            return ShowNetConnectionProperties(pCon, lpcmi->hwnd);
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
    if (!m_pUnknown)
    {
        *ppvSite = NULL;
        return E_FAIL;
    }

    return m_pUnknown->QueryInterface(riid, ppvSite);
}

HRESULT WINAPI CNetConUiObject::SetSite(IUnknown *pUnkSite)
{
    m_pUnknown = pUnkSite;
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
    *pwFlags = 0;
    if (!GetModuleFileNameW(netshell_hInstance, szIconFile, cchMax))
    {
        ERR("GetModuleFileNameW failed\n");
        return E_FAIL;
    }

    PNETCONIDSTRUCT pdata = ILGetConnData(m_pidl);
    if (!pdata)
    {
        ERR("Got invalid pidl!\n");
        return E_FAIL;
    }

    if (pdata->Status == NCS_CONNECTED || pdata->Status == NCS_CONNECTING)
        *piIndex = -IDI_NET_IDLE;
    else
        *piIndex = -IDI_NET_OFF;

    return S_OK;
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
    return SHDefExtractIconW(pszFile, nIconIndex, 0, phiconLarge, phiconSmall, nIconSize);
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
    if (m_pidlRoot)
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
    PCUITEMID_CHILD pidl = ILFindLastID((ITEMIDLIST*)pei->lpIDList);
    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
    {
        ERR("Got invalid pidl!\n");
        return E_FAIL;
    }

    if (pdata->Status == NCS_CONNECTED)
    {
        return ShowNetConnectionStatus(m_lpOleCmd, pidl, pei->hwnd);
    }

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
