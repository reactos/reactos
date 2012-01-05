/*
 * provides new shell item service
 *
 * Copyright 2007 Johannes Anderwald (janderwald@reactos.org)
 * Copyright 2009 Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static WCHAR szNew[MAX_PATH];

CNewMenu::CNewMenu()
{
    szPath = NULL;
    s_SnHead = NULL;
    fSite = NULL;
}


CNewMenu::~CNewMenu()
{
    UnloadShellItems();
}

void CNewMenu::UnloadItem(SHELLNEW_ITEM *item)
{
    // bail if the item is clearly invalid
    if (NULL == item)
        return;

    if (NULL != item->szTarget)
        free(item->szTarget);

    free(item->szDesc);
    free(item->szIcon);
    free(item->szExt);

    HeapFree(GetProcessHeap(), 0, item);
}

void CNewMenu::UnloadShellItems()
{
    SHELLNEW_ITEM *pCurItem;

    while (s_SnHead)
    {
        pCurItem = s_SnHead;
        s_SnHead = s_SnHead->Next;

        UnloadItem(pCurItem);
    }

    s_SnHead = NULL;
}

static
BOOL
GetKeyDescription(LPWSTR szKeyName, LPWSTR szResult)
{
    HKEY hKey;
    DWORD dwDesc, dwError;
    WCHAR szDesc[100];

    TRACE("GetKeyDescription: keyname %s\n", debugstr_w(szKeyName));

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szKeyName, 0, KEY_READ | KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegLoadMUIStringW(hKey, L"\\FriendlyTypeName", szResult, MAX_PATH, &dwDesc, 0, NULL) == ERROR_SUCCESS)
    {
        TRACE("result %s\n", debugstr_w(szResult));
        RegCloseKey(hKey);
        return TRUE;
    }
    /* fetch default value */
    dwDesc = sizeof(szDesc);
    dwError = RegGetValueW(hKey, NULL, NULL, RRF_RT_REG_SZ, NULL, szDesc, &dwDesc);
    if(dwError == ERROR_SUCCESS)
    {
        if (wcsncmp(szKeyName, szDesc, dwDesc / sizeof(WCHAR)))
        {
            /* recurse for to a linked key */
            if (!GetKeyDescription(szDesc, szResult))
            {
                /* use description */
                wcscpy(szResult, szDesc);
            }
        }
        else
        {
            /* use default value as description */
            wcscpy(szResult, szDesc);
        }
    }
    else
    {
        /* registry key w/o default key?? */
        TRACE("RegGetValue failed with %x\n", dwError);
        wcscpy(szResult, szKeyName);
    }

    RegCloseKey(hKey);
    return TRUE;
}

CNewMenu::SHELLNEW_ITEM *CNewMenu::LoadItem(LPWSTR szKeyName)
{
    HKEY hKey;
    DWORD dwIndex = 0;
    WCHAR szName[MAX_PATH];
    WCHAR szCommand[MAX_PATH];
    WCHAR szDesc[MAX_PATH] = L"";
    WCHAR szIcon[MAX_PATH] = L"";
    DWORD dwName, dwCommand;
    LONG result;
    SHELLNEW_ITEM *pNewItem = NULL;

    wcscpy(szName, szKeyName);
    GetKeyDescription(szKeyName, szDesc);
    wcscat(szName, L"\\ShellNew");
    result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szName, 0, KEY_READ, &hKey);

    TRACE("LoadItem dwName %d keyname %s szName %s szDesc %s szIcon %s\n", dwName, debugstr_w(szKeyName), debugstr_w(szName), debugstr_w(szDesc), debugstr_w(szIcon));

    if (result != ERROR_SUCCESS)
    {
        TRACE("Failed to open key\n");
        return NULL;
    }

    do
    {
        dwName = MAX_PATH;
        dwCommand = MAX_PATH;
        result = RegEnumValueW(hKey, dwIndex, szName, &dwName, NULL, NULL, (LPBYTE)szCommand, &dwCommand);
        if (result == ERROR_SUCCESS)
        {
            SHELLNEW_TYPE type = SHELLNEW_TYPE_INVALID;
            LPWSTR szTarget = szCommand;

            TRACE("szName %s szCommand %s\n", debugstr_w(szName), debugstr_w(szCommand));

            if (!wcsicmp(szName, L"Command"))
                type = SHELLNEW_TYPE_COMMAND;
            else if (!wcsicmp(szName, L"Data"))
                type = SHELLNEW_TYPE_DATA;
            else if (!wcsicmp(szName, L"FileName"))
                type = SHELLNEW_TYPE_FILENAME;
            else if (!wcsicmp(szName, L"NullFile"))
            {
                type = SHELLNEW_TYPE_NULLFILE;
                szTarget = NULL;
            }

            if (type != SHELLNEW_TYPE_INVALID)
            {
                pNewItem = (SHELLNEW_ITEM *)HeapAlloc(GetProcessHeap(), 0, sizeof(SHELLNEW_ITEM));
                if (!pNewItem)
                    break;

                pNewItem->Type = type;
                if (szTarget)
                    pNewItem->szTarget = _wcsdup(szTarget);
                else
                    pNewItem->szTarget = NULL;

                pNewItem->szDesc = _wcsdup(szDesc);
                pNewItem->szIcon = _wcsdup(szIcon);
                pNewItem->szExt = _wcsdup(szKeyName);
                pNewItem->Next = NULL;
                break;
            }
        }
        dwIndex++;
    } while(result != ERROR_NO_MORE_ITEMS);
    RegCloseKey(hKey);
    return pNewItem;
}

BOOL
CNewMenu::LoadShellNewItems()
{
    DWORD dwIndex;
    WCHAR szName[MAX_PATH];
    LONG result;
    SHELLNEW_ITEM *pNewItem;
    SHELLNEW_ITEM *pCurItem = NULL;

    /* insert do new folder action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, szNew, sizeof(szNew) / sizeof(WCHAR)))
        szNew[0] = 0;

    UnloadShellItems();

    dwIndex = 0;
    do
    {
        result = RegEnumKeyW(HKEY_CLASSES_ROOT, dwIndex, szName, MAX_PATH);
        if (result == ERROR_SUCCESS)
        {
            pNewItem = LoadItem(szName);
            if (pNewItem)
            {
                if (!wcsicmp(pNewItem->szExt, L".lnk"))
                {
                    if (s_SnHead)
                    {
                        pNewItem->Next = s_SnHead;
                        s_SnHead = pNewItem;
                    }
                    else
                    {
                        s_SnHead = pCurItem = pNewItem;
                    }
                }
                else
                {
                    if (pCurItem)
                    {
                        pCurItem->Next = pNewItem;
                        pCurItem = pNewItem;
                    }
                    else
                    {
                        pCurItem = s_SnHead = pNewItem;
                    }
                }
            }
        }
        dwIndex++;
    } while(result != ERROR_NO_MORE_ITEMS);

    if (s_SnHead == NULL)
        return FALSE;
    else
        return TRUE;
}

UINT
CNewMenu::InsertShellNewItems(HMENU hMenu, UINT idFirst, UINT idMenu)
{
    MENUITEMINFOW mii;
    SHELLNEW_ITEM *pCurItem;
    UINT i;
    WCHAR szBuffer[MAX_PATH];

    if (s_SnHead == NULL)
    {
        if (!LoadShellNewItems())
            return 0;
    }

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);

    /* insert do new shortcut action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWFOLDER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
        szBuffer[0] = 0;
    szBuffer[MAX_PATH-1] = 0;
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.dwTypeData = szBuffer;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idFirst++;
    InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);

    /* insert do new shortcut action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWLINK, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
        szBuffer[0] = 0;
    szBuffer[MAX_PATH-1] = 0;
    mii.dwTypeData = szBuffer;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idFirst++;
    InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);

    /* insert seperator for custom new action */
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    /*
     * FIXME
     * implement loading of icons
     * and using MFT_OWNERDRAWN
     */
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;

    pCurItem = s_SnHead;
    i = 0;

    while(pCurItem)
    {
        if (i >= 1)
        {
            TRACE("szDesc %s\n", debugstr_w(pCurItem->szDesc));
            mii.dwTypeData = pCurItem->szDesc;
            mii.cch = wcslen(mii.dwTypeData);
            mii.wID = idFirst++;
            InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);
        }
        pCurItem = pCurItem->Next;
        i++;
    }
    return (i + 2);
}

HRESULT
CNewMenu::DoShellNewCmd(LPCMINVOKECOMMANDINFO lpcmi, IShellView *psv)
{
    SHELLNEW_ITEM *pCurItem = s_SnHead;
    IPersistFolder3 * psf;
    LPITEMIDLIST pidl;
    STRRET strTemp;
    WCHAR szTemp[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    STARTUPINFOW sInfo;
    PROCESS_INFORMATION pi;
    UINT i, target;
    HANDLE hFile;
    DWORD dwWritten, dwError;
    CComPtr<IFolderView> folderView;
    CComPtr<IShellFolder> parentFolder;
    HRESULT hResult;

    i = 1;
    target = LOWORD(lpcmi->lpVerb);

    while(pCurItem)
    {
        if (i == target)
            break;

        pCurItem = pCurItem->Next;
        i++;
    }

    if (!pCurItem)
        return E_UNEXPECTED;

    //if (fSite == NULL)
    //    return E_FAIL;
    hResult = IUnknown_QueryService(psv, SID_IFolderView, IID_IFolderView, (void **)&folderView);
    if (FAILED(hResult))
        return hResult;
    hResult = folderView->GetFolder(IID_IShellFolder, (void **)&parentFolder);
    if (FAILED(hResult))
        return hResult;

    if (parentFolder->QueryInterface(IID_IPersistFolder2, (LPVOID*)&psf) != S_OK)
    {
        ERR("Failed to get interface IID_IPersistFolder2\n");
        return E_FAIL;
    }
    if (psf->GetCurFolder(&pidl) != S_OK)
    {
        ERR("IPersistFolder2_GetCurFolder failed\n");
        return E_FAIL;
    }

    if (parentFolder == NULL || parentFolder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strTemp) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed\n");
        return E_FAIL;
    }
    StrRetToBufW(&strTemp, pidl, szPath, MAX_PATH);

    switch(pCurItem->Type)
    {
        case SHELLNEW_TYPE_COMMAND:
        {
            LPWSTR ptr;
            LPWSTR szCmd;

            if (!ExpandEnvironmentStringsW(pCurItem->szTarget, szBuffer, MAX_PATH))
            {
                TRACE("ExpandEnvironmentStrings failed\n");
                break;
            }

            ptr = wcsstr(szBuffer, L"%1");
            if (ptr)
            {
                ptr[1] = 's';
                swprintf(szTemp, szBuffer, szPath);
                ptr = szTemp;
            }
            else
            {
                ptr = szBuffer;
            }

            ZeroMemory(&sInfo, sizeof(sInfo));
            sInfo.cb = sizeof(sInfo);
            szCmd = _wcsdup(ptr);
            if (!szCmd)
                break;
            if (CreateProcessW(NULL, szCmd, NULL, NULL, FALSE, 0, NULL, NULL, &sInfo, &pi))
            {
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
            free(szCmd);
            break;
        }
        case SHELLNEW_TYPE_DATA:
        case SHELLNEW_TYPE_FILENAME:
        case SHELLNEW_TYPE_NULLFILE:
        {
            i = 2;

            PathAddBackslashW(szPath);
            wcscat(szPath, szNew);
            wcscat(szPath, L" ");
            wcscat(szPath, pCurItem->szDesc);
            wcscpy(szBuffer, szPath);
            wcscat(szBuffer, pCurItem->szExt);
            do
            {
                hFile = CreateFileW(szBuffer, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                    break;
                dwError = GetLastError();

                TRACE("FileName %s szBuffer %s i %u error %x\n", debugstr_w(szBuffer), debugstr_w(szPath), i, dwError);
                swprintf(szBuffer, L"%s (%d)%s", szPath, i, pCurItem->szExt);
                i++;
            } while(hFile == INVALID_HANDLE_VALUE && dwError == ERROR_FILE_EXISTS);

            if (hFile == INVALID_HANDLE_VALUE)
                return E_FAIL;

            if (pCurItem->Type == SHELLNEW_TYPE_DATA)
            {
                i = WideCharToMultiByte(CP_ACP, 0, pCurItem->szTarget, -1, (LPSTR)szTemp, MAX_PATH * 2, NULL, NULL);
                if (i)
                {
                    WriteFile(hFile, (LPCVOID)szTemp, i, &dwWritten, NULL);
                }
            }
            CloseHandle(hFile);
            if (pCurItem->Type == SHELLNEW_TYPE_FILENAME)
            {
                if (!CopyFileW(pCurItem->szTarget, szBuffer, FALSE))
                    break;
            }
            TRACE("Notifying fs %s\n", debugstr_w(szBuffer));
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, (LPCVOID)szBuffer, NULL);
            break;
            case SHELLNEW_TYPE_INVALID:
                break;
            }
    }
    return S_OK;
}
/**************************************************************************
* DoMeasureItem
*/
HRESULT
CNewMenu::DoMeasureItem(HWND hWnd, MEASUREITEMSTRUCT * lpmis)
{
    SHELLNEW_ITEM *pCurItem;
    SHELLNEW_ITEM *pItem;
    UINT i;
    HDC hDC;
    SIZE size;

    TRACE("DoMeasureItem entered with id %x\n", lpmis->itemID);

    pCurItem = s_SnHead;

    i = 1;
    pItem = NULL;
    while(pCurItem)
    {
        if (i == lpmis->itemID)
        {
            pItem = pCurItem;
            break;
        }
        pCurItem = pCurItem->Next;
        i++;
    }

    if (!pItem)
    {
        TRACE("DoMeasureItem no item found\n");
        return E_FAIL;
    }
    hDC = GetDC(hWnd);
    GetTextExtentPoint32W(hDC, pCurItem->szDesc, wcslen(pCurItem->szDesc), &size);
    lpmis->itemWidth = size.cx + 32;
    lpmis->itemHeight = max(size.cy, 20);
    ReleaseDC (hWnd, hDC);
    return S_OK;
}
/**************************************************************************
* DoDrawItem
*/
HRESULT
CNewMenu::DoDrawItem(HWND hWnd, DRAWITEMSTRUCT * drawItem)
{
    SHELLNEW_ITEM *pCurItem;
    SHELLNEW_ITEM *pItem;
    UINT i;
    pCurItem = s_SnHead;

    TRACE("DoDrawItem entered with id %x\n", drawItem->itemID);

    i = 1;
    pItem = NULL;
    while(pCurItem)
    {
        if (i == drawItem->itemID)
        {
            pItem = pCurItem;
            break;
        }
        pCurItem = pCurItem->Next;
        i++;
    }

    if (!pItem)
        return E_FAIL;

    drawItem->rcItem.left += 20;

    DrawTextW(drawItem->hDC, pCurItem->szDesc, wcslen(pCurItem->szDesc), &drawItem->rcItem, 0);
    return S_OK;
}

/**************************************************************************
* DoNewFolder
*/
void CNewMenu::DoNewFolder(
    IShellView *psv)
{
    ISFHelper *psfhlp;
    WCHAR wszName[MAX_PATH];
    CComPtr<IFolderView> folderView;
    CComPtr<IShellFolder> parentFolder;
    HRESULT hResult;

    //if (fSite == NULL)
    //    return;
    hResult = IUnknown_QueryService(psv, SID_IFolderView, IID_IFolderView, (void **)&folderView);
    if (FAILED(hResult))
        return;
    hResult = folderView->GetFolder(IID_IShellFolder, (void **)&parentFolder);
    if (FAILED(hResult))
        return;

    parentFolder->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlp);
    if (psfhlp)
    {
        LPITEMIDLIST pidl;

        if (psfhlp->GetUniqueName(wszName, MAX_PATH) != S_OK)
            return;
        if (psfhlp->AddFolder(0, wszName, &pidl) != S_OK)
            return;

        if(psv)
        {
            psv->Refresh();
            /* if we are in a shellview do labeledit */
            psv->SelectItem(
                pidl, (SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                       | SVSI_FOCUSED | SVSI_SELECT));
            psv->Refresh();
        }
        SHFree(pidl);

        psfhlp->Release();
    }
}

HRESULT STDMETHODCALLTYPE CNewMenu::SetSite(IUnknown *pUnkSite)
{
    fSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNewMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    *ppvSite = fSite;
    if (fSite != NULL)
        fSite->AddRef();
    return S_OK;
}

HRESULT
WINAPI
CNewMenu::QueryContextMenu(HMENU hmenu,
                           UINT indexMenu,
                           UINT idCmdFirst,
                           UINT idCmdLast,
                           UINT uFlags)
{
    WCHAR szBuffer[200];
    MENUITEMINFOW mii;
    HMENU hSubMenu;
    int id = 1;

    TRACE("%p %p %u %u %u %u\n", this,
          hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, szBuffer, _countof(szBuffer)))
        szBuffer[0] = 0;

    hSubMenu = CreateMenu();
    memset( &mii, 0, sizeof(mii) );
    mii.cbSize = sizeof (mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.wID = idCmdFirst + id++;
    mii.dwTypeData = szBuffer;
    mii.cch = wcslen(mii.dwTypeData);
    mii.fState = MFS_ENABLED;

    if (hSubMenu)
    {
        id += InsertShellNewItems(hSubMenu, idCmdFirst, 0);
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = hSubMenu;
    }

    if (!InsertMenuItemW(hmenu, indexMenu, TRUE, &mii))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, id);
}

HRESULT
WINAPI
CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV = NULL;
    HRESULT hr;

    if((lpSB = (LPSHELLBROWSER)SendMessageA(lpici->hwnd, CWM_GETISHELLBROWSER, 0, 0)))
    {
        lpSB->QueryActiveShellView(&lpSV);
    }

    if (LOWORD(lpici->lpVerb) == 0)
    {
        DoNewFolder(lpSV);
        return S_OK;
    }

    hr = DoShellNewCmd(lpici, lpSV);
    if (SUCCEEDED(hr) && lpSV)
    {
        lpSV->Refresh();
    }

    TRACE("INewItem_IContextMenu_fnInvokeCommand %x\n", hr);
    return hr;
}

HRESULT
WINAPI
CNewMenu::GetCommandString(UINT_PTR idCmd,
                           UINT uType,
                           UINT* pwReserved,
                           LPSTR pszName,
                           UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

HRESULT
WINAPI
CNewMenu::HandleMenuMsg(UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    DRAWITEMSTRUCT *lpids = (DRAWITEMSTRUCT*) lParam;
    MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*) lParam;

    TRACE("INewItem_IContextMenu_fnHandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n", this, uMsg, wParam, lParam);

    switch(uMsg)
    {
        case WM_MEASUREITEM:
            return DoMeasureItem((HWND)wParam, lpmis);
            break;
        case WM_DRAWITEM:
            return DoDrawItem((HWND)wParam, lpids);
            break;
    }
    return S_OK;

    return E_UNEXPECTED;
}

HRESULT WINAPI
CNewMenu::Initialize(LPCITEMIDLIST pidlFolder,
                     IDataObject *pdtobj, HKEY hkeyProgID )
{

    return S_OK;
}
