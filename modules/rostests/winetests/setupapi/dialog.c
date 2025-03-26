/*
 * Unit tests for SetupPromptForDisk
 *
 * Copyright 2014 Michael Müller
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "guiddef.h"
#include "setupapi.h"

#include "wine/test.h"

static void test_SetupPromptForDiskA(void)
{
    char file[] = "kernel32.dll";
    char path[MAX_PATH];
    char buffer[MAX_PATH];
    UINT ret;
    DWORD length;

    GetSystemDirectoryA(path, MAX_PATH);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskA(0, "Test", "Testdisk", path, file, 0, IDF_CHECKFIRST, buffer, sizeof(buffer) - 1, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %u\n", ret);
    ok(length == strlen(path)+1, "Expect length %u, got %u\n", (DWORD)strlen(path) + 1, length);
    ok(strcmp(path, buffer) == 0, "Expected path %s, got %s\n", path, buffer);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskA(0, "Test", "Testdisk", path, file, 0, IDF_CHECKFIRST, 0, 0, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %d\n", ret);
    ok(length == strlen(path)+1, "Expect length %u, got %u\n", (DWORD)strlen(path) + 1, length);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskA(0, "Test", "Testdisk", path, file, 0, IDF_CHECKFIRST, buffer, 1, &length);
    ok(ret == DPROMPT_BUFFERTOOSMALL, "Expected DPROMPT_BUFFERTOOSMALL, got %u\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskA(0, "Test", "Testdisk", path, file, 0, IDF_CHECKFIRST, buffer, strlen(path), &length);
    ok(ret == DPROMPT_BUFFERTOOSMALL, "Expected DPROMPT_BUFFERTOOSMALL, got %u\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskA(0, "Test", "Testdisk", path, file, 0, IDF_CHECKFIRST, buffer, strlen(path)+1, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %u\n", ret);
    ok(length == strlen(path)+1, "Expect length %u, got %u\n", (DWORD)strlen(path) + 1, length);
    ok(strcmp(path, buffer) == 0, "Expected path %s, got %s\n", path, buffer);
}

static void test_SetupPromptForDiskW(void)
{
    WCHAR file[] = {'k','e','r','n','e','l','3','2','.','d','l','l','\0'};
    WCHAR title[] = {'T','e','s','t','\0'};
    WCHAR disk[] = {'T','e','s','t','d','i','s','k','\0'};
    WCHAR path[MAX_PATH];
    WCHAR buffer[MAX_PATH];
    UINT ret;
    DWORD length;

    GetSystemDirectoryW(path, MAX_PATH);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskW(0, title, disk, path, file, 0, IDF_CHECKFIRST, buffer, MAX_PATH-1, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %u\n", ret);
    ok(length == lstrlenW(path)+1, "Expect length %u, got %u\n", lstrlenW(path)+1, length);
    ok(lstrcmpW(path, buffer) == 0, "Expected path %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskW(0, title, disk, path, file, 0, IDF_CHECKFIRST, 0, 0, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %d\n", ret);
    ok(length == lstrlenW(path)+1, "Expect length %u, got %u\n", lstrlenW(path)+1, length);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskW(0, title, disk, path, file, 0, IDF_CHECKFIRST, buffer, 1, &length);
    ok(ret == DPROMPT_BUFFERTOOSMALL, "Expected DPROMPT_BUFFERTOOSMALL, got %u\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskW(0, title, disk, path, file, 0, IDF_CHECKFIRST, buffer, lstrlenW(path), &length);
    ok(ret == DPROMPT_BUFFERTOOSMALL, "Expected DPROMPT_BUFFERTOOSMALL, got %u\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = SetupPromptForDiskW(0, title, disk, path, file, 0, IDF_CHECKFIRST, buffer, lstrlenW(path)+1, &length);
    ok(ret == DPROMPT_SUCCESS, "Expected DPROMPT_SUCCESS, got %u\n", ret);
    ok(length == lstrlenW(path)+1, "Expect length %u, got %u\n", lstrlenW(path)+1, length);
    ok(lstrcmpW(path, buffer) == 0, "Expected path %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
}

START_TEST(dialog)
{
    test_SetupPromptForDiskA();
    test_SetupPromptForDiskW();
}
