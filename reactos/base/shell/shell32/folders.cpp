/*
 *    Copyright 1997    Marcus Meissner
 *    Copyright 1998    Juergen Schmied
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

WCHAR swShell32Name[MAX_PATH];

DWORD NumIconOverlayHandlers = 0;
IShellIconOverlayIdentifier ** Handlers = NULL;

static HRESULT getIconLocationForFolder(LPCITEMIDLIST pidl, UINT uFlags,
                                        LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    int icon_idx;
    bool cont=TRUE;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszCLSIDValue[CHARS_IN_GUID];
    static const WCHAR shellClassInfo[] = { '.', 'S', 'h', 'e', 'l', 'l', 'C', 'l', 'a', 's', 's', 'I', 'n', 'f', 'o', 0 };
    static const WCHAR iconFile[] = { 'I', 'c', 'o', 'n', 'F', 'i', 'l', 'e', 0 };
    static const WCHAR clsid[] = { 'C', 'L', 'S', 'I', 'D', 0 };
    static const WCHAR clsid2[] = { 'C', 'L', 'S', 'I', 'D', '2', 0 };
    static const WCHAR iconIndex[] = { 'I', 'c', 'o', 'n', 'I', 'n', 'd', 'e', 'x', 0 };

    /*
    Optimisation. GetCustomFolderAttribute has a critical lock on it, and isn't fast.
    Test the water (i.e., see if the attribute exists) before questioning it three times
    when most folders don't use it at all.
    */
    WCHAR wszBigToe[3];
    if (!(uFlags & GIL_DEFAULTICON) && SHELL32_GetCustomFolderAttributes(pidl, shellClassInfo,
                                         wszBigToe, 3))
    {
        if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, iconFile,
                                             wszPath, MAX_PATH))
        {
            WCHAR wszIconIndex[10];
            SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, iconIndex,
                                             wszIconIndex, 10);
            *piIndex = _wtoi(wszIconIndex);
            cont=FALSE;
        }
        else if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, clsid,
                 wszCLSIDValue, CHARS_IN_GUID) &&
                 HCR_GetIconW(wszCLSIDValue, szIconFile, NULL, cchMax, &icon_idx))
        {
            *piIndex = icon_idx;
            cont=FALSE;
        }
        else if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, clsid2,
                 wszCLSIDValue, CHARS_IN_GUID) &&
                 HCR_GetIconW(wszCLSIDValue, szIconFile, NULL, cchMax, &icon_idx))
        {
            *piIndex = icon_idx;
            cont=FALSE;
        }
    }
    if (cont)
    {
        static const WCHAR folder[] = { 'F', 'o', 'l', 'd', 'e', 'r', 0 };

        if (!HCR_GetIconW(folder, szIconFile, NULL, cchMax, &icon_idx))
        {
            lstrcpynW(szIconFile, swShell32Name, cchMax);
            icon_idx = -IDI_SHELL_FOLDER;
        }

        if (uFlags & GIL_OPENICON)
            *piIndex = icon_idx < 0 ? icon_idx - 1 : icon_idx + 1;
        else
            *piIndex = icon_idx;
    }

    return S_OK;
}

void InitIconOverlays(void)
{
    HKEY hKey;
    DWORD dwIndex, dwResult, dwSize;
    WCHAR szName[MAX_PATH];
    WCHAR szValue[100];
    CLSID clsid;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    if (RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwResult, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    Handlers = (IShellIconOverlayIdentifier **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwResult * sizeof(IShellIconOverlayIdentifier*));
    if (!Handlers)
    {
        RegCloseKey(hKey);
        return;
    }

    dwIndex = 0;

    CoInitialize(0);

    do
    {
        dwSize = sizeof(szName) / sizeof(WCHAR);
        dwResult = RegEnumKeyExW(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, NULL);

        if (dwResult == ERROR_NO_MORE_ITEMS)
            break;

        if (dwResult == ERROR_SUCCESS)
        {
            dwSize = sizeof(szValue) / sizeof(WCHAR);
            if (RegGetValueW(hKey, szName, NULL, RRF_RT_REG_SZ, NULL, szValue, &dwSize) == ERROR_SUCCESS)
            {
                CComPtr<IShellIconOverlayIdentifier> Overlay;

                CLSIDFromString(szValue, &clsid);
                dwResult = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellIconOverlayIdentifier, &Overlay));
                if (dwResult == S_OK)
                {
                    Handlers[NumIconOverlayHandlers] = Overlay.Detach();
                    NumIconOverlayHandlers++;
                }
            }
        }

        dwIndex++;

    } while(1);

    RegCloseKey(hKey);
}

BOOL
GetIconOverlay(LPCITEMIDLIST pidl, WCHAR * wTemp, int* pIndex)
{
    DWORD Index;
    HRESULT hResult;
    int Priority;
    int HighestPriority;
    ULONG IconIndex;
    ULONG Flags;
    WCHAR szPath[MAX_PATH];

    if(!SHGetPathFromIDListW(pidl, szPath))
        return FALSE;


    HighestPriority = 101;
    IconIndex = NumIconOverlayHandlers;
    for(Index = 0; Index < NumIconOverlayHandlers; Index++)
    {
        hResult = Handlers[Index]->IsMemberOf(szPath, SFGAO_FILESYSTEM);
        if (hResult == S_OK)
        {
            hResult = Handlers[Index]->GetPriority(&Priority);
            if (hResult == S_OK)
            {
                if (Priority < HighestPriority)
                {
                    HighestPriority = Priority;
                    IconIndex = Index;
                }
            }
        }
    }

    if (IconIndex == NumIconOverlayHandlers)
        return FALSE;

    hResult = Handlers[IconIndex]->GetOverlayInfo(wTemp, MAX_PATH, pIndex, &Flags);

    if (hResult == S_OK)
        return TRUE;
    else
        return FALSE;
}

/**************************************************************************
*  IExtractIconW_Constructor
*/
IExtractIconW* IExtractIconW_Constructor(LPCITEMIDLIST pidl)
{
    CComPtr<IDefaultExtractIconInit>    initIcon;
    CComPtr<IExtractIconW> extractIcon;
    GUID const * riid;
    int icon_idx;
    UINT flags;
    CHAR sTemp[MAX_PATH];
    WCHAR wTemp[MAX_PATH];
    LPITEMIDLIST pSimplePidl = ILFindLastID(pidl);
    HRESULT hr;

    hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit,&initIcon));
    if (FAILED(hr))
        return NULL;

    hr = initIcon->QueryInterface(IID_PPV_ARG(IExtractIconW,&extractIcon));
    if (FAILED(hr))
        return NULL;

    if (_ILIsDesktop(pSimplePidl))
    {
        initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_DESKTOP);
    }
    else if ((riid = _ILGetGUIDPointer(pSimplePidl)))
    {
        /* my computer and other shell extensions */
        static const WCHAR fmt[] = { 'C', 'L', 'S', 'I', 'D', '\\',
                                     '{', '%', '0', '8', 'l', 'x', '-', '%', '0', '4', 'x', '-', '%', '0', '4', 'x', '-',
                                     '%', '0', '2', 'x', '%', '0', '2', 'x', '-', '%', '0', '2', 'x', '%', '0', '2', 'x',
                                     '%', '0', '2', 'x', '%', '0', '2', 'x', '%', '0', '2', 'x', '%', '0', '2', 'x', '}', 0
                                   };
        WCHAR xriid[50];

        swprintf(xriid, fmt,
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]);

        const WCHAR* iconname = NULL;
        if (_ILIsBitBucket(pSimplePidl))
        {
            static const WCHAR szFull[] = {'F','u','l','l',0};
            static const WCHAR szEmpty[] = {'E','m','p','t','y',0};
            CComPtr<IEnumIDList> EnumIDList;
            CoInitialize(NULL);

            CComPtr<IShellFolder2> psfRecycleBin;
            CComPtr<IShellFolder> psfDesktop;
            hr = SHGetDesktopFolder(&psfDesktop);

            if (SUCCEEDED(hr))
                hr = psfDesktop->BindToObject(pSimplePidl, NULL, IID_PPV_ARG(IShellFolder2, &psfRecycleBin));
            if (SUCCEEDED(hr))
                hr = psfRecycleBin->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &EnumIDList);

            ULONG itemcount;
            LPITEMIDLIST pidl = NULL;
            if (SUCCEEDED(hr) && (hr = EnumIDList->Next(1, &pidl, &itemcount)) == S_OK)
            {
                CoTaskMemFree(pidl);
                iconname = szFull;
            } else {
                iconname = szEmpty;
            }
        }

        if (HCR_GetIconW(xriid, wTemp, iconname, MAX_PATH, &icon_idx))
        {
            initIcon->SetNormalIcon(wTemp, icon_idx);
        }
        else
        {
            if (IsEqualGUID(*riid, CLSID_MyComputer))
                initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_MY_COMPUTER);
            else if (IsEqualGUID(*riid, CLSID_MyDocuments))
                initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_MY_DOCUMENTS);
            else if (IsEqualGUID(*riid, CLSID_NetworkPlaces))
                initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_MY_NETWORK_PLACES);
            else
                initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_FOLDER);
        }
    }

    else if (_ILIsDrive (pSimplePidl))
    {
        static const WCHAR drive[] = { 'D', 'r', 'i', 'v', 'e', 0 };
        int icon_idx = -1;

        if (_ILGetDrive(pSimplePidl, sTemp, MAX_PATH))
        {
            switch(GetDriveTypeA(sTemp))
            {
                case DRIVE_REMOVABLE:
                    icon_idx = IDI_SHELL_FLOPPY;
                    break;
                case DRIVE_CDROM:
                    icon_idx = IDI_SHELL_CDROM;
                    break;
                case DRIVE_REMOTE:
                    icon_idx = IDI_SHELL_NETDRIVE;
                    break;
                case DRIVE_RAMDISK:
                    icon_idx = IDI_SHELL_RAMDISK;
                    break;
                case DRIVE_NO_ROOT_DIR:
                    icon_idx = IDI_SHELL_CDROM;
                    break;
            }
        }

        if (icon_idx != -1)
        {
            initIcon->SetNormalIcon(swShell32Name, -icon_idx);
        }
        else
        {
            if (HCR_GetIconW(drive, wTemp, NULL, MAX_PATH, &icon_idx))
                initIcon->SetNormalIcon(wTemp, icon_idx);
            else
                initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_DRIVE);
        }
    }

    else if (_ILIsFolder (pSimplePidl))
    {
        if (SUCCEEDED(getIconLocationForFolder(
                          pidl, 0, wTemp, MAX_PATH,
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetNormalIcon(wTemp, icon_idx);
            // FIXME: if/when getIconLocationForFolder does something for 
            //        GIL_FORSHORTCUT, code below should be uncommented. and
            //        the following line removed.
            initIcon->SetShortcutIcon(wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(
                          pidl, GIL_DEFAULTICON, wTemp, MAX_PATH,
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetDefaultIcon(wTemp, icon_idx);
        }
        // if (SUCCEEDED(getIconLocationForFolder(
        //                   pidl, GIL_FORSHORTCUT, wTemp, MAX_PATH,
        //                   &icon_idx,
        //                   &flags)))
        // {
        //     initIcon->SetShortcutIcon(wTemp, icon_idx);
        // }
        if (SUCCEEDED(getIconLocationForFolder(
                          pidl, GIL_OPENICON, wTemp, MAX_PATH,
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetOpenIcon(wTemp, icon_idx);
        }
    }
    else
    {
        BOOL found = FALSE;

        if (_ILIsCPanelStruct(pSimplePidl))
        {
            if (SUCCEEDED(CPanel_GetIconLocationW(pSimplePidl, wTemp, MAX_PATH, &icon_idx)))
                found = TRUE;
        }
        else if (_ILGetExtension(pSimplePidl, sTemp, MAX_PATH))
        {
            if (HCR_MapTypeToValueA(sTemp, sTemp, MAX_PATH, TRUE)
                    && HCR_GetIconA(sTemp, sTemp, NULL, MAX_PATH, &icon_idx))
            {
                if (!lstrcmpA("%1", sTemp)) /* icon is in the file */
                {
                    SHGetPathFromIDListW(pidl, wTemp);
                    icon_idx = 0;
                }
                else
                {
                    MultiByteToWideChar(CP_ACP, 0, sTemp, -1, wTemp, MAX_PATH);
                }

                found = TRUE;
            }
            else if (!lstrcmpiA(sTemp, "lnkfile"))
            {
                /* extract icon from shell shortcut */
                CComPtr<IShellFolder>        dsf;
                CComPtr<IShellLinkW>        psl;

                if (SUCCEEDED(SHGetDesktopFolder(&dsf)))
                {
                    HRESULT hr = dsf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*) &pidl, IID_NULL_PPV_ARG(IShellLinkW, &psl));

                    if (SUCCEEDED(hr))
                    {
                        hr = psl->GetIconLocation(wTemp, MAX_PATH, &icon_idx);

                        if (SUCCEEDED(hr) && *sTemp)
                            found = TRUE;

                    }
                }
            }
        }

        if (!found)
            /* default icon */
            initIcon->SetNormalIcon(swShell32Name, 0);
        else
            initIcon->SetNormalIcon(wTemp, icon_idx);
    }

    return extractIcon.Detach();
}

/**************************************************************************
*  IExtractIconA_Constructor
*/
IExtractIconA* IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
    CComPtr<IExtractIconW> extractIconW;
    CComPtr<IExtractIconA> extractIconA;
    HRESULT hr;

    extractIconW = IExtractIconW_Constructor(pidl);
    if (!extractIconW)
        return NULL;

    hr = extractIconW->QueryInterface(IID_PPV_ARG(IExtractIconA, &extractIconA));
    if (FAILED(hr))
        return NULL;
    return extractIconA.Detach();
}
