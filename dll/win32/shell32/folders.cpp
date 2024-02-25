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
