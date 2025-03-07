/*
 * Copyright 2017 Giannis Adamopoulos
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

HANDLE _CreateActCtxFromFile(LPCWSTR FileName, int line);
VOID _ActivateCtx(HANDLE h, ULONG_PTR *cookie, int line);
VOID _DeactivateCtx(ULONG_PTR cookie, int line);

typedef DWORD (WINAPI *LPGETVERSION)();

VOID _TestVesion(HANDLE dll, DWORD ExpectedVersion, int line)
{
    LPGETVERSION proc = (LPGETVERSION)GetProcAddress(dll, "GetVersion");
    DWORD version = proc();
    ok_(__FILE__, line)(version == ExpectedVersion, "Got version %lu, expected %lu\n", version, ExpectedVersion);
}

VOID TestDllRedirection()
{
    HANDLE dll1, dll2, h;
    ULONG_PTR cookie;

    /* Try to load the libraries without sxs */
    dll1 = LoadLibraryExW(L"kernel32test_versioned.dll",0 , 0);
    ok (dll1 != NULL, "LoadLibraryExW failed\n");
    dll2 = LoadLibraryExW(L"testdata\\kernel32test_versioned.dll",0 , 0);
    ok (dll2 != NULL, "LoadLibraryExW failed\n");

    ok (dll1 != dll2, "\n");
    _TestVesion(dll1, 1, __LINE__);
    _TestVesion(dll2, 2, __LINE__);

    FreeLibrary(dll1);
    FreeLibrary(dll2);

    dll1 = LoadLibraryExW(L"kernel32test_versioned.dll",0 , 0);
    ok (dll1 != NULL, "LoadLibraryExW failed\n");

    /* redir2dep.manifest defines an assembly with nothing but a dependency on redirtest2 assembly */
    /* redirtest2.manifest defines an assembly that contains kernel32test_versioned.dll */
    /* In win10 it is enought to load and activate redirtest2 */
    /* In win2k3 however the only way to trigger the redirection is to load and activate redir2dep */
    h = _CreateActCtxFromFile(L"testdata\\redir2dep.manifest", __LINE__);
    _ActivateCtx(h, &cookie, __LINE__);
    dll2 = LoadLibraryExW(L"kernel32test_versioned.dll",0 , 0);
    _DeactivateCtx(cookie, __LINE__);
    ok (dll2 != NULL, "LoadLibraryExW failed\n");

    ok (dll1 != dll2, "\n");
    _TestVesion(dll1, 1, __LINE__);
    _TestVesion(dll2, 2, __LINE__);

    FreeLibrary(dll1);
    FreeLibrary(dll2);

    dll1 = LoadLibraryExW(L"comctl32.dll",0 ,0);
    ok (dll1 != NULL, "LoadLibraryExW failed\n");

    h = _CreateActCtxFromFile(L"comctl32dep.manifest", __LINE__);
    _ActivateCtx(h, &cookie, __LINE__);
    dll2 = LoadLibraryExW(L"comctl32.dll",0 , 0);
    _DeactivateCtx(cookie, __LINE__);
    ok (dll2 != NULL, "LoadLibraryExW failed\n");

    ok (dll1 != dll2, "\n");

}

START_TEST(LoadLibraryExW)
{
    TestDllRedirection();
}
