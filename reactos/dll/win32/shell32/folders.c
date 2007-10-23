/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "undocshell.h"
#include "shlguid.h"

#include "wine/debug.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shfldr.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

WCHAR swShell32Name[MAX_PATH];

static HRESULT getIconLocationForFolder(LPCITEMIDLIST pidl, UINT uFlags,
 LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    int icon_idx;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszCLSIDValue[CHARS_IN_GUID];
    static const WCHAR shellClassInfo[] = { '.','S','h','e','l','l','C','l','a','s','s','I','n','f','o',0 };
    static const WCHAR iconFile[] = { 'I','c','o','n','F','i','l','e',0 };
    static const WCHAR clsid[] = { 'C','L','S','I','D',0 };
    static const WCHAR clsid2[] = { 'C','L','S','I','D','2',0 };
    static const WCHAR iconIndex[] = { 'I','c','o','n','I','n','d','e','x',0 };

    if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, iconFile,
        wszPath, MAX_PATH))
    {
        WCHAR wszIconIndex[10];
        SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, iconIndex,
            wszIconIndex, 10);
        *piIndex = atoiW(wszIconIndex);
    }
    else if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, clsid,
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &icon_idx))
    {
       *piIndex = icon_idx;
    }
    else if (SHELL32_GetCustomFolderAttribute(pidl, shellClassInfo, clsid2,
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &icon_idx))
    {
       *piIndex = icon_idx;
    }
    else
    {
        static const WCHAR folder[] = { 'F','o','l','d','e','r',0 };

        if (!HCR_GetDefaultIconW(folder, szIconFile, cchMax, &icon_idx))
        {
            lstrcpynW(szIconFile, swShell32Name, cchMax);
            icon_idx = -IDI_SHELL_FOLDER;
        }

        if (uFlags & GIL_OPENICON)
            *piIndex = icon_idx<0? icon_idx-1: icon_idx+1;
        else
            *piIndex = icon_idx;
    }

    return S_OK;
}

/**************************************************************************
*  IExtractIconW_Constructor
*/
IExtractIconW* IExtractIconW_Constructor(LPCITEMIDLIST pidl)
{
    IDefaultExtractIconInit *initIcon;
    IExtractIconW *extractIcon;
    GUID const * riid;
    int icon_idx;
    UINT flags;
    CHAR sTemp[MAX_PATH];
    WCHAR wTemp[MAX_PATH];
    LPITEMIDLIST pSimplePidl = ILFindLastID(pidl);
    HRESULT hr;

    hr = SHCreateDefaultExtractIcon(&IID_IDefaultExtractIconInit, (void **)&initIcon);
    if (!SUCCEEDED(hr))
        return NULL;

    hr = IDefaultExtractIconInit_QueryInterface(initIcon, &IID_IExtractIconW, (void **)&extractIcon);
    IDefaultExtractIconInit_Release(initIcon);
    if (!SUCCEEDED(hr))
        return NULL;

    if (_ILIsDesktop(pSimplePidl))
    {
        IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_DESKTOP);
    }
    else if ((riid = _ILGetGUIDPointer(pSimplePidl)))
    {
        /* my computer and other shell extensions */
        static const WCHAR fmt[] = { 'C','L','S','I','D','\\',
            '{','%','0','8','l','x','-','%','0','4','x','-','%','0','4','x','-',
            '%','0','2','x','%','0','2','x','-','%','0','2','x', '%','0','2','x',
            '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0 };
            WCHAR xriid[50];

            sprintfW(xriid, fmt,
                riid->Data1, riid->Data2, riid->Data3,
                riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]);

            if (HCR_GetDefaultIconW(xriid, wTemp, MAX_PATH, &icon_idx))
            {
                IDefaultExtractIconInit_SetNormalIcon(initIcon, wTemp, icon_idx);
            }
            else
            {
                if (IsEqualGUID(riid, &CLSID_MyComputer))
                    IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_MY_COMPUTER);
                else if (IsEqualGUID(riid, &CLSID_MyDocuments))
                    IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_MY_DOCUMENTS);
                else if (IsEqualGUID(riid, &CLSID_NetworkPlaces))
                    IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_MY_NETWORK_PLACES);
                else if (IsEqualGUID(riid, &CLSID_UnixFolder) ||
                         IsEqualGUID(riid, &CLSID_UnixDosFolder))
                    IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_DRIVE);
                else
                    IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_FOLDER);
            }
    }

    else if (_ILIsDrive (pSimplePidl))
    {
        static const WCHAR drive[] = { 'D','r','i','v','e',0 };
        int icon_idx = -1;

        if (_ILGetDrive(pSimplePidl, sTemp, MAX_PATH))
        {
            switch(GetDriveTypeA(sTemp))
            {
                case DRIVE_REMOVABLE:   icon_idx = IDI_SHELL_FLOPPY;        break;
                case DRIVE_CDROM:       icon_idx = IDI_SHELL_CDROM;         break;
                case DRIVE_REMOTE:      icon_idx = IDI_SHELL_NETDRIVE;      break;
                case DRIVE_RAMDISK:     icon_idx = IDI_SHELL_RAMDISK;       break;
            }
        }

        if (icon_idx != -1)
        {
            IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -icon_idx);
        }
        else
        {
            if (HCR_GetDefaultIconW(drive, wTemp, MAX_PATH, &icon_idx))
                IDefaultExtractIconInit_SetNormalIcon(initIcon, wTemp, icon_idx);
            else
                IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, -IDI_SHELL_DRIVE);
        }
    }

    else if (_ILIsFolder (pSimplePidl))
    {
        if (SUCCEEDED(getIconLocationForFolder(
                       pidl, 0, wTemp, MAX_PATH,
                       &icon_idx,
                       &flags)))
        {
            IDefaultExtractIconInit_SetNormalIcon(initIcon, wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(
                       pidl, GIL_DEFAULTICON, wTemp, MAX_PATH,
                       &icon_idx,
                       &flags)))
        {
            IDefaultExtractIconInit_SetDefaultIcon(initIcon, wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(
                       pidl, GIL_FORSHORTCUT, wTemp, MAX_PATH,
                       &icon_idx,
                       &flags)))
        {
            IDefaultExtractIconInit_SetShortcutIcon(initIcon, wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(
                       pidl, GIL_OPENICON, wTemp, MAX_PATH,
                       &icon_idx,
                       &flags)))
        {
            IDefaultExtractIconInit_SetOpenIcon(initIcon, wTemp, icon_idx);
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
                && HCR_GetDefaultIconA(sTemp, sTemp, MAX_PATH, &icon_idx))
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
                IShellFolder* dsf;
                IShellLinkW* psl;

                if (SUCCEEDED(SHGetDesktopFolder(&dsf)))
                {
                    HRESULT hr = IShellFolder_GetUIObjectOf(dsf, NULL, 1, (LPCITEMIDLIST*)&pidl, &IID_IShellLinkW, NULL, (LPVOID*)&psl);

                    if (SUCCEEDED(hr))
                    {
                        hr = IShellLinkW_GetIconLocation(psl, wTemp, MAX_PATH, &icon_idx);

                        if (SUCCEEDED(hr) && *sTemp)
                            found = TRUE;

                        IShellLinkW_Release(psl);
                    }

                    IShellFolder_Release(dsf);
                }
            }
        }

        if (!found)
            /* default icon */
            IDefaultExtractIconInit_SetNormalIcon(initIcon, swShell32Name, 0);
        else
            IDefaultExtractIconInit_SetNormalIcon(initIcon, wTemp, icon_idx);
    }

    return extractIcon;
}

/**************************************************************************
*  IExtractIconA_Constructor
*/
IExtractIconA* IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
    IExtractIconW *extractIconW;
    IExtractIconA *extractIconA;
    HRESULT hr;

    extractIconW = IExtractIconW_Constructor(pidl);
    if (!extractIconW)
        return NULL;
    hr = IExtractIconW_QueryInterface(extractIconW, &IID_IExtractIconA, (void **)&extractIconA);
    IExtractIconW_Release(extractIconW);
    if (!SUCCEEDED(hr))
        return NULL;
    return extractIconA;
}
