/*
 * Write.exe - this program only calls wordpad.exe
 *
 * Copyright 2007 by Mikolaj Zalewski
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>

#include <windows.h>
#include "resources.h"

static const WCHAR SZ_BACKSLASH[] = {'\\',0};
static const WCHAR SZ_WORDPAD[]   = {'w','o','r','d','p','a','d','.','e','x','e',0};

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPWSTR szCmdParagraph, int res)
{
    WCHAR path[MAX_PATH];
    STARTUPINFOW stinf;
    PROCESS_INFORMATION info;

    if (!GetSystemDirectoryW(path, MAX_PATH - 1 - lstrlenW(SZ_WORDPAD)))
	goto failed;
    if (path[lstrlenW(path) - 1] != '\\')
	lstrcatW(path, SZ_BACKSLASH);
    lstrcatW(path, SZ_WORDPAD);

    stinf.cb = sizeof(STARTUPINFOW);
    GetStartupInfoW(&stinf);

    if (!CreateProcessW(path, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &stinf, &info))
	goto failed;
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
    return 0;

failed:
    LoadStringW(GetModuleHandleW(NULL), IDS_FAILED, path, MAX_PATH);
    MessageBoxW(NULL, path, NULL, MB_OK|MB_ICONERROR);
    return 1;
}
