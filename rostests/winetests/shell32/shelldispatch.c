/*
 * Unit tests for IShellDispatch
 *
 * Copyright 2010 Alexander Morozov for Etersoft
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "shldisp.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static HRESULT (WINAPI *pSHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);
static HRESULT (WINAPI *pSHGetNameFromIDList)(PCIDLIST_ABSOLUTE,SIGDN,PWSTR*);
static HRESULT (WINAPI *pSHGetSpecialFolderLocation)(HWND, int, LPITEMIDLIST *);
static DWORD (WINAPI *pGetLongPathNameW)(LPCWSTR, LPWSTR, DWORD);

static void init_function_pointers(void)
{
    HMODULE hshell32, hkernel32;

    hshell32 = GetModuleHandleA("shell32.dll");
    hkernel32 = GetModuleHandleA("kernel32.dll");
    pSHGetFolderPathW = (void*)GetProcAddress(hshell32, "SHGetFolderPathW");
    pSHGetNameFromIDList = (void*)GetProcAddress(hshell32, "SHGetNameFromIDList");
    pSHGetSpecialFolderLocation = (void*)GetProcAddress(hshell32,
     "SHGetSpecialFolderLocation");
    pGetLongPathNameW = (void*)GetProcAddress(hkernel32, "GetLongPathNameW");
}

static void test_namespace(void)
{
    static const WCHAR winetestW[] = {'w','i','n','e','t','e','s','t',0};
    static const WCHAR backslashW[] = {'\\',0};
    static const WCHAR clsidW[] = {
        ':',':','{','6','4','5','F','F','0','4','0','-','5','0','8','1','-',
                    '1','0','1','B','-','9','F','0','8','-',
                    '0','0','A','A','0','0','2','F','9','5','4','E','}',0};

    static WCHAR tempW[MAX_PATH], curW[MAX_PATH];
    WCHAR *long_pathW = NULL;
    HRESULT r;
    IShellDispatch *sd;
    Folder *folder;
    Folder2 *folder2;
    FolderItem *item;
    VARIANT var;
    BSTR title, item_path;
    int len;

    r = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
     &IID_IShellDispatch, (LPVOID*)&sd);
    if (r == REGDB_E_CLASSNOTREG) /* NT4 */
    {
        win_skip("skipping IShellDispatch tests\n");
        return;
    }
    ok(SUCCEEDED(r), "CoCreateInstance failed: %08x\n", r);
    if (FAILED(r))
        return;

    VariantInit(&var);
    folder = (void*)0xdeadbeef;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);
    ok(folder == NULL, "expected NULL, got %p\n", folder);

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    folder = (void*)0xdeadbeef;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine {
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);
    ok(folder == NULL, "got %p\n", folder);
}
    V_VT(&var) = VT_I4;
    V_I4(&var) = ssfPROGRAMFILES;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK ||
     broken(r == S_FALSE), /* NT4 */
     "IShellDispatch::NameSpace failed: %08x\n", r);
    if (r == S_OK)
    {
        static WCHAR path[MAX_PATH];

        if (pSHGetFolderPathW)
        {
            r = pSHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL,
             SHGFP_TYPE_CURRENT, path);
            ok(r == S_OK, "SHGetFolderPath failed: %08x\n", r);
        }
        r = Folder_get_Title(folder, &title);
        todo_wine
        ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
        if (r == S_OK)
        {
            /* On Win2000-2003 title is equal to program files directory name in
               HKLM\Software\Microsoft\Windows\CurrentVersion\ProgramFilesDir.
               On newer Windows it seems constant and is not changed
               if the program files directory name is changed */
            if (pSHGetSpecialFolderLocation && pSHGetNameFromIDList)
            {
                LPITEMIDLIST pidl;
                PWSTR name;

                r = pSHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl);
                ok(r == S_OK, "SHGetSpecialFolderLocation failed: %08x\n", r);
                r = pSHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &name);
                ok(r == S_OK, "SHGetNameFromIDList failed: %08x\n", r);
                todo_wine
                ok(!lstrcmpW(title, name), "expected %s, got %s\n",
                 wine_dbgstr_w(name), wine_dbgstr_w(title));
                CoTaskMemFree(name);
                CoTaskMemFree(pidl);
            }
            else if (pSHGetFolderPathW)
            {
                WCHAR *p;

                p = path + lstrlenW(path);
                while (path < p && *(p - 1) != '\\')
                    p--;
                ok(!lstrcmpiW(title, p), "expected %s, got %s\n",
                 wine_dbgstr_w(p), wine_dbgstr_w(title));
            }
            else skip("skipping Folder::get_Title test\n");
            SysFreeString(title);
        }
        r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
        ok(r == S_OK, "Folder::QueryInterface failed: %08x\n", r);
        if (r == S_OK)
        {
            r = Folder2_get_Self(folder2, &item);
            ok(r == S_OK, "Folder::get_Self failed: %08x\n", r);
            if (r == S_OK)
            {
                r = FolderItem_get_Path(item, &item_path);
                ok(r == S_OK, "FolderItem::get_Path failed: %08x\n", r);
                if (pSHGetFolderPathW)
                    ok(!lstrcmpiW(item_path, path), "expected %s, got %s\n",
                     wine_dbgstr_w(path), wine_dbgstr_w(item_path));
                SysFreeString(item_path);
                FolderItem_Release(item);
            }
            Folder2_Release(folder2);
        }
        Folder_Release(folder);
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = ssfBITBUCKET;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK ||
     broken(r == S_FALSE), /* NT4 */
     "IShellDispatch::NameSpace failed: %08x\n", r);
    if (r == S_OK)
    {
        r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
        ok(r == S_OK ||
         broken(r == E_NOINTERFACE), /* NT4 */
         "Folder::QueryInterface failed: %08x\n", r);
        if (r == S_OK)
        {
            r = Folder2_get_Self(folder2, &item);
            ok(r == S_OK, "Folder::get_Self failed: %08x\n", r);
            if (r == S_OK)
            {
                r = FolderItem_get_Path(item, &item_path);
                todo_wine
                ok(r == S_OK, "FolderItem::get_Path failed: %08x\n", r);
                todo_wine
                ok(!lstrcmpW(item_path, clsidW), "expected %s, got %s\n",
                 wine_dbgstr_w(clsidW), wine_dbgstr_w(item_path));
                SysFreeString(item_path);
                FolderItem_Release(item);
            }
            Folder2_Release(folder2);
        }
        Folder_Release(folder);
    }

    GetTempPathW(MAX_PATH, tempW);
    GetCurrentDirectoryW(MAX_PATH, curW);
    SetCurrentDirectoryW(tempW);
    CreateDirectoryW(winetestW, NULL);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(winetestW);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);
    SysFreeString(V_BSTR(&var));

    GetFullPathNameW(winetestW, MAX_PATH, tempW, NULL);
    if (pGetLongPathNameW)
    {
        len = pGetLongPathNameW(tempW, NULL, 0);
        long_pathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (long_pathW)
            pGetLongPathNameW(tempW, long_pathW, len);
    }
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(tempW);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08x\n", r);
    if (r == S_OK)
    {
        r = Folder_get_Title(folder, &title);
        ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
        if (r == S_OK)
        {
            ok(!lstrcmpW(title, winetestW), "bad title: %s\n",
             wine_dbgstr_w(title));
            SysFreeString(title);
        }
        r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
        ok(r == S_OK ||
         broken(r == E_NOINTERFACE), /* NT4 */
         "Folder::QueryInterface failed: %08x\n", r);
        if (r == S_OK)
        {
            r = Folder2_get_Self(folder2, &item);
            ok(r == S_OK, "Folder::get_Self failed: %08x\n", r);
            if (r == S_OK)
            {
                r = FolderItem_get_Path(item, &item_path);
                ok(r == S_OK, "FolderItem::get_Path failed: %08x\n", r);
                if (long_pathW)
                    ok(!lstrcmpW(item_path, long_pathW),
                     "expected %s, got %s\n", wine_dbgstr_w(long_pathW),
                     wine_dbgstr_w(item_path));
                SysFreeString(item_path);
                FolderItem_Release(item);
            }
            Folder2_Release(folder2);
        }
        Folder_Release(folder);
    }
    SysFreeString(V_BSTR(&var));

    len = lstrlenW(tempW);
    if (len < MAX_PATH - 1)
    {
        lstrcatW(tempW, backslashW);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(tempW);
        r = IShellDispatch_NameSpace(sd, var, &folder);
        ok(r == S_OK, "IShellDispatch::NameSpace failed: %08x\n", r);
        if (r == S_OK)
        {
            r = Folder_get_Title(folder, &title);
            ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
            if (r == S_OK)
            {
                ok(!lstrcmpW(title, winetestW), "bad title: %s\n",
                 wine_dbgstr_w(title));
                SysFreeString(title);
            }
            r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
            ok(r == S_OK ||
             broken(r == E_NOINTERFACE), /* NT4 */
             "Folder::QueryInterface failed: %08x\n", r);
            if (r == S_OK)
            {
                r = Folder2_get_Self(folder2, &item);
                ok(r == S_OK, "Folder::get_Self failed: %08x\n", r);
                if (r == S_OK)
                {
                    r = FolderItem_get_Path(item, &item_path);
                    ok(r == S_OK, "FolderItem::get_Path failed: %08x\n", r);
                    if (long_pathW)
                        ok(!lstrcmpW(item_path, long_pathW),
                         "expected %s, got %s\n", wine_dbgstr_w(long_pathW),
                         wine_dbgstr_w(item_path));
                    SysFreeString(item_path);
                    FolderItem_Release(item);
                }
                Folder2_Release(folder2);
            }
            Folder_Release(folder);
        }
        SysFreeString(V_BSTR(&var));
    }

    HeapFree(GetProcessHeap(), 0, long_pathW);
    RemoveDirectoryW(winetestW);
    SetCurrentDirectoryW(curW);
    IShellDispatch_Release(sd);
}

static void test_service(void)
{
    static const WCHAR spooler[] = {'S','p','o','o','l','e','r',0};
    static const WCHAR dummyW[] = {'d','u','m','m','y',0};
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    IShellDispatch2 *sd;
    DWORD dummy;
    HRESULT hr;
    BSTR name;
    VARIANT v;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch2, (void**)&sd);
    if (hr != S_OK)
    {
        win_skip("IShellDispatch2 not supported\n");
        return;
    }

    V_VT(&v) = VT_I2;
    V_I2(&v) = 10;
    hr = IShellDispatch2_IsServiceRunning(sd, NULL, &v);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    EXPECT_HR(hr, S_OK);

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    service = OpenServiceW(scm, spooler, SERVICE_QUERY_STATUS);
    QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(SERVICE_STATUS_PROCESS), &dummy);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    /* service should exist */
    name = SysAllocString(spooler);
    V_VT(&v) = VT_I2;
    hr = IShellDispatch2_IsServiceRunning(sd, name, &v);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    if (status.dwCurrentState == SERVICE_RUNNING)
        ok(V_BOOL(&v) == VARIANT_TRUE, "got %d\n", V_BOOL(&v));
    else
        ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    SysFreeString(name);

    /* service doesn't exist */
    name = SysAllocString(dummyW);
    V_VT(&v) = VT_I2;
    hr = IShellDispatch2_IsServiceRunning(sd, name, &v);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    SysFreeString(name);

    IShellDispatch2_Release(sd);
}

START_TEST(shelldispatch)
{
    HRESULT r;

    r = CoInitialize(NULL);
    ok(SUCCEEDED(r), "CoInitialize failed: %08x\n", r);
    if (FAILED(r))
        return;

    init_function_pointers();
    test_namespace();
    test_service();

    CoUninitialize();
}
