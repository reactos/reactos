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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

WCHAR swShell32Name[MAX_PATH];

DWORD NumIconOverlayHandlers = 0;
IShellIconOverlayIdentifier ** Handlers = NULL;

static HRESULT getIconLocationForFolder(IShellFolder * psf, LPCITEMIDLIST pidl, UINT uFlags,
                                        LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    static const WCHAR shellClassInfo[] = { '.', 'S', 'h', 'e', 'l', 'l', 'C', 'l', 'a', 's', 's', 'I', 'n', 'f', 'o', 0 };
    static const WCHAR iconFile[] = { 'I', 'c', 'o', 'n', 'F', 'i', 'l', 'e', 0 };
    static const WCHAR clsid[] = { 'C', 'L', 'S', 'I', 'D', 0 };
    static const WCHAR clsid2[] = { 'C', 'L', 'S', 'I', 'D', '2', 0 };
    static const WCHAR iconIndex[] = { 'I', 'c', 'o', 'n', 'I', 'n', 'd', 'e', 'x', 0 };
    static const WCHAR wszDesktopIni[] = { 'd','e','s','k','t','o','p','.','i','n','i',0 };
    int icon_idx;

    if (!(uFlags & GIL_DEFAULTICON) && (_ILGetFileAttributes(ILFindLastID(pidl), NULL, 0) & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) != 0 )
    {
        WCHAR wszFolderPath[MAX_PATH];

        if (!ILGetDisplayNameExW(psf, pidl, wszFolderPath, 0))
            return FALSE;

        PathAppendW(wszFolderPath, wszDesktopIni);

        if (PathFileExistsW(wszFolderPath))
        {
            WCHAR wszPath[MAX_PATH];
            WCHAR wszCLSIDValue[CHARS_IN_GUID];

            if (GetPrivateProfileStringW(shellClassInfo, iconFile, NULL, wszPath, MAX_PATH, wszFolderPath))
            {
                ExpandEnvironmentStringsW(wszPath, szIconFile, cchMax);

                *piIndex = GetPrivateProfileIntW(shellClassInfo, iconIndex, 0, wszFolderPath);
                return S_OK;
            }
            else if (GetPrivateProfileStringW(shellClassInfo, clsid, NULL, wszCLSIDValue, CHARS_IN_GUID, wszFolderPath) &&
                HCR_GetIconW(wszCLSIDValue, szIconFile, NULL, cchMax, &icon_idx))
            {
                *piIndex = icon_idx;
                return S_OK;
            }
            else if (GetPrivateProfileStringW(shellClassInfo, clsid2, NULL, wszCLSIDValue, CHARS_IN_GUID, wszFolderPath) &&
                HCR_GetIconW(wszCLSIDValue, szIconFile, NULL, cchMax, &icon_idx))
            {
                *piIndex = icon_idx;
                return S_OK;
            }
        }
    }

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

    return S_OK;
}

static void InitIconOverlays(void)
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

    if (!Handlers)
        InitIconOverlays();

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

HRESULT CFSExtractIcon_CreateInstance(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID iid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr;
    int icon_idx = 0;
    UINT flags = 0; // FIXME: Use it!
    CHAR sTemp[MAX_PATH] = "";
    WCHAR wTemp[MAX_PATH] = L"";

    hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit,&initIcon));
    if (FAILED(hr))
        return hr;

    if (_ILIsFolder (pidl))
    {
        if (SUCCEEDED(getIconLocationForFolder(psf, 
                          pidl, 0, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetNormalIcon(wTemp, icon_idx);
            // FIXME: if/when getIconLocationForFolder does something for 
            //        GIL_FORSHORTCUT, code below should be uncommented. and
            //        the following line removed.
            initIcon->SetShortcutIcon(wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(psf, 
                          pidl, GIL_DEFAULTICON, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetDefaultIcon(wTemp, icon_idx);
        }
        // if (SUCCEEDED(getIconLocationForFolder(psf, 
        //                   pidl, GIL_FORSHORTCUT, wTemp, _countof(wTemp),
        //                   &icon_idx,
        //                   &flags)))
        // {
        //     initIcon->SetShortcutIcon(wTemp, icon_idx);
        // }
        if (SUCCEEDED(getIconLocationForFolder(psf, 
                          pidl, GIL_OPENICON, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetOpenIcon(wTemp, icon_idx);
        }
    }
    else
    {
        BOOL found = FALSE;

        if (_ILGetExtension(pidl, sTemp, _countof(sTemp)))
        {
            if (HCR_MapTypeToValueA(sTemp, sTemp, _countof(sTemp), TRUE)
                    && HCR_GetIconA(sTemp, sTemp, NULL, _countof(sTemp), &icon_idx))
            {
                if (!lstrcmpA("%1", sTemp)) /* icon is in the file */
                {
                    ILGetDisplayNameExW(psf, pidl, wTemp, 0);
                    icon_idx = 0;
                }
                else
                {
                    MultiByteToWideChar(CP_ACP, 0, sTemp, -1, wTemp, _countof(wTemp));
                }

                found = TRUE;
            }
            else if (!lstrcmpiA(sTemp, "lnkfile"))
            {
                /* extract icon from shell shortcut */
                CComPtr<IShellLinkW> psl;
                CComPtr<IExtractIconW> pei;

                HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_NULL_PPV_ARG(IShellLinkW, &psl));
                if (SUCCEEDED(hr))
                {
                    hr = psl->GetIconLocation(wTemp, _countof(wTemp), &icon_idx);
                    if (FAILED(hr) || !*wTemp)
                    {
                        /* The icon was not found directly, try to retrieve it from the shell link target */
                        hr = psl->QueryInterface(IID_PPV_ARG(IExtractIconW, &pei));
                        if (FAILED(hr) || !pei)
                            TRACE("No IExtractIconW interface!\n");
                        else
                            hr = pei->GetIconLocation(GIL_FORSHELL, wTemp, _countof(wTemp), &icon_idx, &flags);
                    }

                    if (SUCCEEDED(hr) && *wTemp)
                        found = TRUE;
                }
            }
        }

        /* FIXME: We should normally use the correct icon format according to 'flags' */
        if (!found)
            /* default icon */
            initIcon->SetNormalIcon(swShell32Name, 0);
        else
            initIcon->SetNormalIcon(wTemp, icon_idx);
    }

    return initIcon->QueryInterface(iid, ppvOut);
}
