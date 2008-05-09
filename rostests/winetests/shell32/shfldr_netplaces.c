/*
 * Tests for My Network Places shell folder
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "shellapi.h"
#include "shlobj.h"

#include "wine/test.h"

static void test_parse_for_entire_network(void)
{
    static const WCHAR entire_network_path[] = {
        ':',':','{','2','0','8','D','2','C','6','0','-','3','A','E','A','-',
                    '1','0','6','9','-','A','2','D','7','-','0','8','0','0','2','B','3','0','3','0','9','D',
                '}','\\','E','n','t','i','r','e','N','e','t','w','o','r','k',0 };
    IShellFolder *psfDesktop;
    HRESULT hr;
    DWORD eaten = 0xdeadbeef;
    LPITEMIDLIST pidl;
    DWORD attr = ~0;
    DWORD expected_attr;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%x\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, (LPWSTR)entire_network_path, &eaten, &pidl, &attr);
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%x\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %u\n", eaten);
    expected_attr = SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_STORAGEANCESTOR|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    todo_wine
    ok(attr == expected_attr, "attr should be 0x%x, not 0x%x\n", expected_attr, attr);

    ILFree(pidl);
}


START_TEST(shfldr_netplaces)
{
    test_parse_for_entire_network();
}
