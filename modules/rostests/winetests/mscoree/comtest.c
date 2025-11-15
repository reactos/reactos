/*
 * Copyright 2018 Fabian Maurer
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
#include <stdio.h>

#include "windows.h"
#include "ole2.h"
#include "mscoree.h"
#include "corerror.h"
#include "shlwapi.h"
#include "shlobj.h"

#include "wine/test.h"

#include "initguid.h"
#include "interfaces.h"

HMODULE hmscoree;

DEFINE_GUID(IID_ITest2, 0x50adb433, 0xf6c5, 0x3b30, 0x92,0x0a, 0x55,0x57,0x11,0x86,0x75,0x09);

typedef enum _run_type
{
    run_type_current_working_directory = 0,
    run_type_exe_directory,
    run_type_system32,
} run_type;

static BOOL write_resource_file(const char *path_tmp, const char *name_res, const char *name_file, char *path_file)
{
    HRSRC rsrc;
    void *rsrc_data;
    DWORD rsrc_size;
    BOOL ret;
    HANDLE hfile;

    rsrc = FindResourceA(GetModuleHandleA(NULL), name_res, (LPCSTR)RT_RCDATA);
    if (!rsrc) return FALSE;

    rsrc_data = LockResource(LoadResource(GetModuleHandleA(NULL), rsrc));
    if (!rsrc_data) return FALSE;

    rsrc_size = SizeofResource(GetModuleHandleA(NULL), rsrc);
    if (!rsrc_size) return FALSE;

    strcpy(path_file, path_tmp);
    PathAppendA(path_file, name_file);
    hfile = CreateFileA(path_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    ret = WriteFile(hfile, rsrc_data, rsrc_size, &rsrc_size, NULL);

    CloseHandle(hfile);
    return ret;
}

static BOOL compile_cs_to_dll(char *source_path, char *dest_path)
{
    const char *path_csc = "C:\\windows\\Microsoft.NET\\Framework\\v2.0.50727\\csc.exe";
    char cmdline[2 * MAX_PATH + 74];
    char path_temp[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    BOOL ret;

    if (!PathFileExistsA(path_csc))
    {
        skip("Can't find csc.exe\n");
        return FALSE;
    }

    GetTempPathA(MAX_PATH, path_temp);
    PathAppendA(path_temp, "comtest.dll");

    sprintf(cmdline, "%s /t:library /out:\"%s\" \"%s\"", path_csc, path_temp, source_path);

    si.cb = sizeof(si);
    ret = CreateProcessA(path_csc, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %lu\n", GetLastError());

    wait_child_process(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    ret = PathFileExistsA(path_temp);
    ok(ret, "Compilation failed\n");

    ret = MoveFileA(path_temp, dest_path);
    ok(ret, "Could not move %s to %s: %lu\n", path_temp, dest_path, GetLastError());
    return ret;
}

static void run_test(BOOL expect_success)
{
    typedef HRESULT (WINAPI *_DllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
    ITest *test = NULL;
    HRESULT hr;
    _DllGetClassObject getClassObject;
    IClassFactory *classFactory = NULL;
    HRESULT result_expected = expect_success ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    hr = CoCreateInstance(&CLSID_Test, NULL, CLSCTX_INPROC_SERVER, &IID_ITest, (void**)&test);
    todo_wine_if(!expect_success)
    ok(hr == result_expected, "Expected %lx, got %lx\n", result_expected, hr);

    if (hr == S_OK)
    {
        int i = 0;
        hr = ITest_Func(test, &i);
        ok(hr == S_OK, "Got %lx\n", hr);
        ok(i == 42, "Expected 42, got %d\n", i);
        ITest_Release(test);
    }

    getClassObject = (_DllGetClassObject)GetProcAddress(hmscoree, "DllGetClassObject");
    hr = getClassObject(&CLSID_Test, &IID_IClassFactory, (void **)&classFactory);
    todo_wine_if(!expect_success)
    ok(hr == result_expected, "Expected %lx, got %lx\n", result_expected, hr);

    if (hr == S_OK)
    {
        ITest *test2 = NULL;
        hr = IClassFactory_CreateInstance(classFactory, NULL, &IID_ITest, (void **)&test2);
        todo_wine_if(!expect_success)
        ok(hr == S_OK, "Got %lx\n", hr);

        if (hr == S_OK)
        {
            int i = 0;
            hr = ITest_Func(test2, &i);
            ok(hr == S_OK, "Got %lx\n", hr);
            ok(i == 42, "Expected 42, got %d\n", i);
            ITest_Release(test2);
        }
        IClassFactory_Release(classFactory);
    }

}

static void run_registry_test(run_type run)
{
    char buffer[256];
    ITest *test = NULL;
    HRESULT hr, result_expected;
    IUnknown *unk = NULL;
    HKEY hkey;
    DWORD ret;
    int i = 0;

    if (run == run_type_exe_directory) result_expected = S_OK;
    else result_expected = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    sprintf(buffer, "CLSID\\%s", wine_dbgstr_guid(&CLSID_Test));
    ret = RegCreateKeyA( HKEY_CLASSES_ROOT, buffer, &hkey );
    if (ret == ERROR_ACCESS_DENIED && !IsUserAnAdmin())
    {
        win_skip("cannot run the registry tests due to user not being admin\n");
        RegCloseKey(hkey);
        return;
    }
    ok(ret == ERROR_SUCCESS, "RegCreateKeyA returned %lx\n", ret);

    ret = RegSetKeyValueA(hkey, "InprocServer32", NULL, REG_SZ, "mscoree.dll", 11);
    ok(ret == ERROR_SUCCESS, "RegSetKeyValueA returned %lx\n", ret);
    ret = RegSetKeyValueA(hkey, "InprocServer32", "Assembly", REG_SZ, "comtest, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null", 74);
    ok(ret == ERROR_SUCCESS, "RegSetKeyValueA returned %lx\n", ret);
    ret = RegSetKeyValueA(hkey, "InprocServer32", "Class", REG_SZ, "DLL.Test", 8);
    ok(ret == ERROR_SUCCESS, "RegSetKeyValueA returned %lx\n", ret);
    ret = RegSetKeyValueA(hkey, "InprocServer32", "CodeBase", REG_SZ, "file:///U:/invalid/path/to/comtest.dll", 41);
    ok(ret == ERROR_SUCCESS, "RegSetKeyValueA returned %lx\n", ret);

    hr = CoCreateInstance(&CLSID_Test, NULL, CLSCTX_INPROC_SERVER, &IID_ITest, (void**)&test);
    todo_wine_if(result_expected != S_OK)
    ok(hr == result_expected, "Expected %lx, got %lx\n", result_expected, hr);

    if (hr == S_OK)
    {
        hr = ITest_Func(test, &i);
        ok(hr == S_OK, "Got %lx\n", hr);
        ok(i == 42, "Expected 42, got %d\n", i);
        hr = ITest_QueryInterface(test, &IID_ITest2, (void**)&unk);
        ok(hr == S_OK, "ITest_QueryInterface returned %lx\n", hr);
        if (hr == S_OK) IUnknown_Release(unk);
        ITest_Release(test);
    }

    RegDeleteKeyValueA(hkey, "InprocServer32", "CodeBase");
    RegDeleteKeyValueA(hkey, "InprocServer32", "Class");
    RegDeleteKeyValueA(hkey, "InprocServer32", "Assembly");
    RegDeleteKeyValueA(hkey, "InprocServer32", NULL);
    RegDeleteKeyA(hkey, "InprocServer32");
    RegCloseKey(hkey);
}

static void get_dll_path_for_run(char *path_dll, UINT path_dll_size, run_type run)
{
    char path_tmp[MAX_PATH];

    GetTempPathA(MAX_PATH, path_tmp);

    switch (run)
    {
    case run_type_current_working_directory:
       strcpy(path_dll, path_tmp);
       PathAppendA(path_dll, "comtest.dll");
       break;
    case run_type_exe_directory:
       GetModuleFileNameA(NULL, path_dll, path_dll_size);
       PathRemoveFileSpecA(path_dll);
       PathAppendA(path_dll, "comtest.dll");
       break;
    case run_type_system32:
       GetSystemDirectoryA(path_dll, path_dll_size);
       PathAppendA(path_dll, "comtest.dll");
       break;
    }
}
static void prepare_and_run_test(const char *dll_source, run_type run)
{
    char path_tmp[MAX_PATH];
    char path_tmp_manifest[MAX_PATH];
    char path_dll[MAX_PATH];
    char path_dll_source[MAX_PATH];
    char path_manifest_dll[MAX_PATH];
    char path_manifest_exe[MAX_PATH];
    BOOL success;
    ACTCTXA context = {0};
    ULONG_PTR cookie;
    HANDLE handle_context = 0;

    path_manifest_exe[0] = path_manifest_dll[0] = path_dll_source[0] = 0;

    GetTempPathA(MAX_PATH, path_tmp);
    GetTempPathA(MAX_PATH, path_tmp_manifest);
    PathAppendA(path_tmp_manifest, "manifests");

    CreateDirectoryA(path_tmp_manifest, NULL);

    if (run == run_type_system32)
    {
        if (!IsUserAnAdmin())
        {
            skip("Can't test dll in system32 due to user not being admin.\n");
            return;
        }
    }

    if (!write_resource_file(path_tmp, dll_source, "comtest.cs", path_dll_source))
    {
        ok(0, "run: %d, Failed to create file for testing\n", run);
        goto cleanup;
    }

    get_dll_path_for_run(path_dll, sizeof(path_dll), run);

    if (!compile_cs_to_dll(path_dll_source, path_dll))
        goto cleanup;

    if (!write_resource_file(path_tmp_manifest, "comtest_exe.manifest", "exe.manifest", path_manifest_exe))
    {
        ok(0, "run: %d, Failed to create file for testing\n", run);
        goto cleanup;
    }

    if (!write_resource_file(path_tmp_manifest, "comtest_dll.manifest", "comtest.manifest", path_manifest_dll))
    {
        ok(0, "run: %d, Failed to create file for testing\n", run);
        goto cleanup;
    }

    context.cbSize = sizeof(ACTCTXA);
    context.lpSource = path_manifest_exe;
    context.lpAssemblyDirectory = path_tmp_manifest;
    context.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;

    handle_context = CreateActCtxA(&context);
    ok(handle_context != NULL && handle_context != INVALID_HANDLE_VALUE, "run: %d, CreateActCtxA failed: %ld\n", run, GetLastError());

    if (handle_context == NULL || handle_context == INVALID_HANDLE_VALUE)
    {
        ok(0, "run: %d, Failed to create activation context\n", run);
        goto cleanup;
    }

    success = ActivateActCtx(handle_context, &cookie);
    ok(success, "run: %d, ActivateActCtx failed: %ld\n", run,  GetLastError());

    if (run == run_type_current_working_directory)
        SetCurrentDirectoryA(path_tmp);

    run_test(run == run_type_exe_directory);
    run_registry_test(run);

cleanup:
    if (handle_context != NULL && handle_context != INVALID_HANDLE_VALUE)
    {
        success = DeactivateActCtx(0, cookie);
        ok(success, "run: %d, DeactivateActCtx failed: %ld\n", run, GetLastError());
        ReleaseActCtx(handle_context);
    }
    if (*path_manifest_exe)
    {
        success = DeleteFileA(path_manifest_exe);
        ok(success, "run: %d, DeleteFileA failed: %ld\n", run, GetLastError());
    }
    if(*path_manifest_dll)
    {
        success = DeleteFileA(path_manifest_dll);
        ok(success, "run: %d, DeleteFileA failed: %ld\n", run, GetLastError());
    }
    if(*path_dll_source)
    {
        success = DeleteFileA(path_dll_source);
        ok(success, "run: %d, DeleteFileA failed: %ld\n", run, GetLastError());
    }
    RemoveDirectoryA(path_tmp_manifest);
    /* dll cleanup is handled by the parent, because it might still be used by the child */
}


static void cleanup_test(run_type run)
{
    char path_dll[MAX_PATH];
    BOOL success;

    get_dll_path_for_run(path_dll, sizeof(path_dll), run);

    if (!PathFileExistsA(path_dll))
        return;

    success = DeleteFileA(path_dll);
    if (!success)
    {
        Sleep(500);
        success = DeleteFileA(path_dll);
    }
    ok(success, "DeleteFileA failed: %ld\n", GetLastError());
}

static void run_child_process(const char *dll_source, run_type run)
{
    char cmdline[MAX_PATH];
    char exe[MAX_PATH];
    char **argv;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    BOOL ret;

    winetest_get_mainargs(&argv);

    if (strstr(argv[0], ".exe"))
        sprintf(exe, "%s", argv[0]);
    else
        sprintf(exe, "%s.exe", argv[0]);
    sprintf(cmdline, "\"%s\" %s %s %d", argv[0], argv[1], dll_source, run);

    si.cb = sizeof(si);
    ret = CreateProcessA(exe, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %lu\n", GetLastError());

    wait_child_process(pi.hProcess);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* Cleanup dll, because it might still have been used by the child */
    cleanup_test(run);
}

START_TEST(comtest)
{
    int argc;
    char **argv;

    CoInitialize(NULL);

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
    {
        skip(".NET or mono not available\n");
        return;
    }

    argc = winetest_get_mainargs(&argv);
    if (argc > 2)
    {
        const char *dll_source = argv[2];
        run_type run = atoi(argv[3]);
        prepare_and_run_test(dll_source, run);

        goto cleanup;
    }

    run_child_process("comtest.cs", run_type_current_working_directory);
    run_child_process("comtest.cs", run_type_exe_directory);
    run_child_process("comtest.cs", run_type_system32);

cleanup:
    FreeLibrary(hmscoree);
    CoUninitialize();
}
