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

#include <stdio.h>

#define COBJMACROS

#include <windows.h>
#include <winsxs.h>
#include <corerror.h>
#include "shlwapi.h"

#include "wine/test.h"

#include "initguid.h"
#include "interfaces.h"

#define SXS_LOOKUP_CLR_GUID_USE_ACTCTX     0x00000001
#define SXS_LOOKUP_CLR_GUID_FIND_SURROGATE 0x00010000
#define SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS 0x00020000
#define SXS_LOOKUP_CLR_GUID_FIND_ANY       (SXS_LOOKUP_CLR_GUID_FIND_SURROGATE | SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS)

#define SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE  0x00000001
#define SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS      0x00000002

typedef struct _SXS_GUID_INFORMATION_CLR
{
    DWORD cbSize;
    DWORD dwFlags;
    PCWSTR pcwszRuntimeVersion;
    PCWSTR pcwszTypeName;
    PCWSTR pcwszAssemblyIdentity;
} SXS_GUID_INFORMATION_CLR;

/* Defined in sxs.dll, but not found in any header */
BOOL WINAPI SxsLookupClrGuid(DWORD flags, GUID *clsid, HANDLE actctx, void *buffer, SIZE_T buffer_len, SIZE_T *buffer_len_required);

static BOOL write_resource_file(const char *path_tmp, const char *name_res, const char *name_file, char *path_file)
{
    HRSRC rsrc;
    void *rsrc_data;
    DWORD rsrc_size;
    BOOL ret;
    HANDLE hfile;

    path_file[0] = 0;
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

static void run_test(void)
{
    SIZE_T buffer_size;
    BOOL ret;
    SXS_GUID_INFORMATION_CLR *info;

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_ANY | SXS_LOOKUP_CLR_GUID_USE_ACTCTX, (GUID *)&CLSID_Test,
            NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_NOT_FOUND, "Unexpected error %ld.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_ANY | SXS_LOOKUP_CLR_GUID_USE_ACTCTX, (GUID *)&CLSID_Test,
            NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_NOT_FOUND, "Unexpected error %ld.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_ANY, (GUID*)&CLSID_Test, NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS, (GUID*)&CLSID_Test, NULL, NULL, 0, &buffer_size);
    ok(ret == FALSE, "Got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got %ld\n", GetLastError());

    info = malloc(buffer_size);
    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS, (GUID*)&CLSID_Test, NULL, info, buffer_size, &buffer_size);
    ok(ret == TRUE, "Got %d\n", ret);
    ok(GetLastError() == 0, "Got %ld\n", GetLastError());

    ok(info->dwFlags == SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS, "Got %ld\n", info->dwFlags);
    ok(!lstrcmpW(info->pcwszTypeName, L"DLL.Test"), "Unexpected typename %s.\n", wine_dbgstr_w(info->pcwszTypeName));
    ok(!lstrcmpW(info->pcwszAssemblyIdentity, L"comtest,type=\"win32\",version=\"1.0.0.0\""),
           "Unexpected assembly identity %s.\n", wine_dbgstr_w(info->pcwszAssemblyIdentity));
    ok(!lstrcmpW(info->pcwszRuntimeVersion, L"v4.0.0.0"), "Unexpected runtime version %s.\n",
           wine_dbgstr_w(info->pcwszRuntimeVersion));

    free(info);

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_SURROGATE, (GUID *)&CLSID_SurrogateTest, NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got %ld\n", GetLastError());

    info = malloc(buffer_size);
    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_SURROGATE, (GUID *)&CLSID_SurrogateTest, NULL, info,
            buffer_size, &buffer_size);
    ok(ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == 0, "Got %ld\n", GetLastError());

    ok(info->dwFlags == SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE, "Unexpected flags %#lx.\n", info->dwFlags);
    ok(!lstrcmpW(info->pcwszTypeName, L"Surrogate.Test"), "Unexpected typename %s.\n", wine_dbgstr_w(info->pcwszTypeName));
    ok(!lstrcmpW(info->pcwszAssemblyIdentity, L"comtest,type=\"win32\",version=\"1.0.0.0\""),
           "Unexpected assembly identity %s.\n", wine_dbgstr_w(info->pcwszAssemblyIdentity));
    ok(!lstrcmpW(info->pcwszRuntimeVersion, L"v4.0.0.1"), "Unexpected runtime version %s.\n",
           wine_dbgstr_w(info->pcwszRuntimeVersion));

    free(info);

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_ANY, (GUID *)&CLSID_SurrogateTest, NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got %ld\n", GetLastError());
}

static void prepare_and_run_test(void)
{
    char path_tmp[MAX_PATH];
    char path_manifest_dll[MAX_PATH];
    char path_manifest_exe[MAX_PATH];
    BOOL success;
    ACTCTXA context = {0};
    ULONG_PTR cookie;
    HANDLE handle_context = INVALID_HANDLE_VALUE;

    GetTempPathA(MAX_PATH, path_tmp);

    if (!write_resource_file(path_tmp, "comtest_exe.manifest", "exe.manifest", path_manifest_exe))
    {
        ok(0, "Failed to create file for testing\n");
        goto cleanup;
    }

    if (!write_resource_file(path_tmp, "comtest_dll.manifest", "comtest.manifest", path_manifest_dll))
    {
        ok(0, "Failed to create file for testing\n");
        goto cleanup;
    }

    context.cbSize = sizeof(ACTCTXA);
    context.lpSource = path_manifest_exe;
    context.lpAssemblyDirectory = path_tmp;
    context.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;

    handle_context = CreateActCtxA(&context);
    ok(handle_context != INVALID_HANDLE_VALUE, "CreateActCtxA failed: %ld\n", GetLastError());

    if (handle_context == INVALID_HANDLE_VALUE)
    {
        ok(0, "Failed to create activation context\n");
        goto cleanup;
    }

    success = ActivateActCtx(handle_context, &cookie);
    ok(success, "ActivateActCtx failed: %ld\n", GetLastError());

    run_test();

cleanup:
    if (handle_context != INVALID_HANDLE_VALUE)
    {
        success = DeactivateActCtx(0, cookie);
        ok(success, "DeactivateActCtx failed: %ld\n", GetLastError());
        ReleaseActCtx(handle_context);
    }
    if (*path_manifest_exe)
    {
        success = DeleteFileA(path_manifest_exe);
        ok(success, "DeleteFileA failed: %ld\n", GetLastError());
    }
    if(*path_manifest_dll)
    {
        success = DeleteFileA(path_manifest_dll);
        ok(success, "DeleteFileA failed: %ld\n", GetLastError());
    }
}

static void run_child_process(void)
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
    sprintf(cmdline, "\"%s\" %s %s", argv[0], argv[1], "subtest");

    si.cb = sizeof(si);
    ret = CreateProcessA(exe, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %lu\n", GetLastError());

    wait_child_process(pi.hProcess);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

static void test_SxsLookupClrGuid(void)
{
    SIZE_T buffer_size;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS, (GUID*)&CLSID_Test, NULL, NULL, 0, &buffer_size);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_FOUND, "Expected ERROR_NOT_FOUND, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS | SXS_LOOKUP_CLR_GUID_USE_ACTCTX, (GUID *)&CLSID_Test,
            NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_NOT_FOUND, "Expected ERROR_NOT_FOUND, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SxsLookupClrGuid(SXS_LOOKUP_CLR_GUID_FIND_SURROGATE, (GUID *)&CLSID_Test, NULL, NULL, 0, &buffer_size);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_NOT_FOUND, "Expected ERROR_NOT_FOUND, got %ld\n", GetLastError());

    run_child_process();
}

START_TEST(sxs)
{
    char **argv;
    int argc = winetest_get_mainargs(&argv);
    if (argc > 2)
    {
        prepare_and_run_test();
        return;
    }

    test_SxsLookupClrGuid();
}
