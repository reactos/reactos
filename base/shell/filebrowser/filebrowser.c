/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>

typedef HRESULT (WINAPI *SH_OPEN_NEW_FRAME)(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14);

int _tmain(int argc, _TCHAR* argv[])
{
    WCHAR root[MAX_PATH];
    HRESULT hr;
    LPSHELLFOLDER pDesktopFolder = NULL;
    LPITEMIDLIST pidlRoot = NULL;
    typedef HRESULT(WINAPI *SH_OPEN_NEW_FRAME)(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14);
    SH_OPEN_NEW_FRAME SHOpenNewFrame;

    HMODULE hBrowseui = LoadLibraryW(L"browseui.dll");

    if (!hBrowseui)
        return 1;


    if (argc < 2)
    {
        SH_OPEN_NEW_FRAME SHOpenNewFrame = (SH_OPEN_NEW_FRAME) GetProcAddress(hBrowseui, (LPCSTR) 103);
        LPITEMIDLIST pidlDrives;
        SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlDrives);
        SHOpenNewFrame((LPITEMIDLIST) pidlDrives, NULL, 0, 0);
    }
    else
    {
        /* A shell is already loaded. Parse the command line arguments
        and unless we need to do something specific simply display
        the desktop in a separate explorer window */
        /* FIXME */

        /* Commandline switches:
        *
        * /n               Open a new window, even if an existing one still exists.
        * /e               Start with the explorer sidebar shown.
        * /root,<object>   Open a window for the given object path.
        * /select,<object> Open a window with the given object selected.
        */

        /* FIXME: Do it right */
        WCHAR* tmp = wcsstr(argv[1], L"/root,");
        if (tmp)
        {
            WCHAR* tmp2;

            tmp += 6; // skip to beginning of path
            tmp2 = wcschr(tmp, L',');

            if (tmp2)
            {
                wcsncpy(root, tmp, tmp2 - tmp);
            }
            else
            {
                wcscpy(root, tmp);
            }
        }
        else
        {
            wcscpy(root, argv[1]);
        }

        if (root[0] == L'"')
        {
            int len = wcslen(root) - 2;
            wcsncpy(root, root + 1, len);
            root[len] = 0;
        }

        if (wcslen(root) > 0)
        {
            LPITEMIDLIST  pidl;
            ULONG         chEaten;
            ULONG         dwAttributes;

            if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
            {
                hr = pDesktopFolder->lpVtbl->ParseDisplayName(pDesktopFolder,
                    NULL,
                    NULL,
                    root,
                    &chEaten,
                    &pidl,
                    &dwAttributes);
                if (SUCCEEDED(hr))
                {
                    pidlRoot = pidl;
                }
            }
        }

        if (!pidlRoot)
        {
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlRoot);
            if (FAILED(hr))
                return 0;
        }
        
        SHOpenNewFrame = (SH_OPEN_NEW_FRAME) GetProcAddress(hBrowseui, (LPCSTR) 103);

        hr = SHOpenNewFrame(pidlRoot, (IUnknown*) pDesktopFolder, 0, 0);
        if (FAILED(hr))
            return 0;
    }

    /* FIXME: we should wait a bit here and see if a window was created. If not we should exit this process. */
    Sleep(1000);

    ExitThread(0);
}

