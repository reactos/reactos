/*
 * Unit test for setupapi.dll install functions
 *
 * Copyright 2007 Misha Koshelev
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
#include <string.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "winsvc.h"
#include "setupapi.h"
#include "shlobj.h"
#include "fci.h"

#include "wine/test.h"

static const char inffile[] = "test.inf";
static const WCHAR inffileW[] = {'t','e','s','t','.','i','n','f',0};
static char CURR_DIR[MAX_PATH];

/* Notes on InstallHinfSectionA/W:
 * - InstallHinfSectionA on WinXP seems to be a stub - it does not do anything
 *   and simply returns without displaying any error message or setting last
 *   error.
 * - These functions do not return a value and do not always set last error to
 *   ERROR_SUCCESS when installation still occurs (e.g., unquoted inf file with
 *   spaces, registry keys are written but last error is 6).
 * - On installation problems, a MessageBox() is displayed and a Beep() is
 *   issued. The MessageBox() is disabled with a CBT hook.
 */

/*
 * Helpers
 */

static void load_resource(const char *name, const char *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", filename, GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static void create_inf_file(LPCSTR filename, const char *data)
{
    DWORD res;
    BOOL ret;
    HANDLE handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", filename, GetLastError());
    ret = WriteFile(handle, data, strlen(data), &res, NULL);
    ok(ret, "Failed to write file, error %lu.\n", GetLastError());
    CloseHandle(handle);
}

static void create_file(const char *filename)
{
    create_inf_file(filename, "dummy");
}

static BOOL delete_file(const char *filename)
{
    if (GetFileAttributesA(filename) & FILE_ATTRIBUTE_DIRECTORY)
        return RemoveDirectoryA(filename);
    else
        return DeleteFileA(filename);
}

static BOOL file_exists(const char *path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static void * CDECL mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL CDECL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    sprintf(pccab->szCab, pv, pccab->iCab);
    return TRUE;
}

static LONG CDECL progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int CDECL file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                             BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    dwAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;

    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;

    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL CDECL get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    LPSTR tempname;

    tempname = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        HeapFree(GetProcessHeap(), 0, tempname);
        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, tempname);

    return FALSE;
}

static INT_PTR CDECL get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                                   USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFileA(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");

    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributesA(pszName);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");

    return (INT_PTR)handle;
}

static BOOL add_file(HFCI hfci, const char *file, TCOMP compress)
{
    char path[MAX_PATH];
    char filename[MAX_PATH];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    lstrcpyA(filename, file);

    return FCIAddFile(hfci, path, filename, FALSE, get_next_cabinet,
                      progress, get_open_info, compress);
}

static void create_cab_file(const CHAR *name, const CHAR *files)
{
    CCAB cabParams = {0};
    LPCSTR ptr;
    HFCI hfci;
    ERF erf;
    BOOL res;

    cabParams.cb = INT_MAX;
    cabParams.cbFolderThresh = 900000;
    cabParams.setID = 0xbeef;
    cabParams.iCab = 1;
    lstrcpyA(cabParams.szCabPath, CURR_DIR);
    lstrcatA(cabParams.szCabPath, "\\");
    lstrcpyA(cabParams.szCab, name);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    ptr = files;
    while (*ptr)
    {
        create_file(ptr);
        res = add_file(hfci, ptr, tcompTYPE_MSZIP);
        ok(res, "Failed to add file: %s\n", ptr);
        res = DeleteFileA(ptr);
        ok(res, "Failed to delete file %s, error %lu\n", ptr, GetLastError());
        ptr += lstrlenA(ptr) + 1;
    }

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

/* CBT hook to ensure a window (e.g., MessageBox) cannot be created */
static HHOOK hhook;
static LRESULT CALLBACK cbt_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return nCode == HCBT_CREATEWND ? 1: CallNextHookEx(hhook, nCode, wParam, lParam);
}

/*
 * Tests
 */

static const char *cmdline_inf = "[Version]\n"
    "Signature=\"$Chicago$\"\n"
    "[DefaultInstall]\n"
    "AddReg=Add.Settings\n"
    "[Add.Settings]\n"
    "HKCU,Software\\Wine\\setupapitest,,\n";

static void run_cmdline(LPCSTR section, int mode, LPCSTR path)
{
    CHAR cmdline[MAX_PATH * 2];
    WCHAR cmdlinew[MAX_PATH * 2];

    sprintf(cmdline, "%s %d %s", section, mode, path);
    MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlinew, MAX_PATH*2);
    InstallHinfSectionW(NULL, NULL, cmdlinew, 0);
}

static void ok_registry(BOOL expectsuccess)
{
    LONG ret;

    /* Functional tests for success of install and clean up */
    ret = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest");
    ok((expectsuccess && ret == ERROR_SUCCESS) ||
       (!expectsuccess && ret == ERROR_FILE_NOT_FOUND),
       "Expected registry key Software\\Wine\\setupapitest to %s, RegDeleteKey returned %ld\n",
       expectsuccess ? "exist" : "not exist",
       ret);
}

/* Test command line processing */
static void test_cmdline(void)
{
    static const char infwithspaces[] = "test file.inf";
    char path[MAX_PATH + 16];
    BOOL ret;

    create_inf_file(inffile, cmdline_inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(TRUE);
    ret = DeleteFileA(inffile);
    ok(ret, "Expected source inf to exist, last error was %ld\n", GetLastError());

    /* Test handling of spaces in path, unquoted and quoted */
    create_inf_file(infwithspaces, cmdline_inf);

    sprintf(path, "%s\\%s", CURR_DIR, infwithspaces);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(TRUE);

    sprintf(path, "\"%s\\%s\"", CURR_DIR, infwithspaces);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(FALSE);

    ret = DeleteFileA(infwithspaces);
    ok(ret, "Expected source inf to exist, last error was %ld\n", GetLastError());
}

static const char *cmdline_inf_reg = "[Version]\n"
    "Signature=\"$Chicago$\"\n"
    "[DefaultInstall]\n"
    "DelReg=Del.Settings\n"
    "[Del.Settings]\n"
    "HKCU,Software\\Wine\\setupapitest\n";

static void test_registry(void)
{
    HKEY key;
    LONG res;
    char path[MAX_PATH + 9];
    BOOL ret;

    /* First create a registry structure we would like to be deleted */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest", &key),
        "Expected RegCreateKeyA to succeed\n");

    /* Doublecheck if the registry key is present */
    ok(!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest", &key),
        "Expected registry key to exist\n");

    create_inf_file(inffile, cmdline_inf_reg);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);

    /* Check if the registry key is recursively deleted */
    res = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest", &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Didn't expect the registry key to exist\n");
    /* Just in case */
    if (res == ERROR_SUCCESS)
    {
        RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest");
        RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest");
    }
    ret = DeleteFileA(inffile);
    ok(ret, "Expected source inf to exist, last error was %ld\n", GetLastError());
}

static void test_install_from(void)
{
    char path[MAX_PATH + 9];
    HINF infhandle;
    HKEY key;
    LONG res;
    BOOL ret;

    /* First create a registry structure we would like to be deleted */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest", &key),
        "Expected RegCreateKeyA to succeed\n");

    /* Doublecheck if the registry key is present */
    ok(!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest", &key),
        "Expected registry key to exist\n");

    create_inf_file(inffile, cmdline_inf_reg);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallFromInfSectionA(NULL, infhandle, "DefaultInstall", SPINST_REGISTRY, key,
        "A:\\", 0, NULL, NULL, NULL, NULL);
    ok(ret, "Unexpected failure\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());

    /* Check if the registry key is recursively deleted */
    res = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest", &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Didn't expect the registry key to exist\n");
    /* Just in case */
    if (res == ERROR_SUCCESS)
    {
        RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest\\setupapitest");
        RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest");
    }

    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);
}

static void test_install_svc_from(void)
{
    char inf[2048];
    char path[MAX_PATH + 9];
    HINF infhandle;
    BOOL ret;
    SC_HANDLE scm_handle, svc_handle;

    /* Basic inf file to satisfy SetupOpenInfFileA */
    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    /* Nothing but the Version section */
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
        "Expected ERROR_SECTION_NOT_FOUND, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the section */
    strcat(inf, "[Winetest.Services]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
        "Expected ERROR_SECTION_NOT_FOUND, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add a reference */
    strcat(inf, "AddService=Winetest,,Winetest.Service\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the section */
    strcat(inf, "[Winetest.Service]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Just the ServiceBinary */
    strcat(inf, "ServiceBinary=%12%\\winetest.sys\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the ServiceType */
    strcat(inf, "ServiceType=1\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the StartType */
    strcat(inf, "StartType=4\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* This should be it, the minimal entries to install a service */
    strcat(inf, "ErrorControl=1");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to install the service\n");
        SetupCloseInfFile(infhandle);
        DeleteFileA(inffile);
        return;
    }
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceA(scm_handle, "Winetest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %ld\n", GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    strcat(inf, "[XSP.InstallPerVer]\n");
    strcat(inf, "AddReg=AspEventlogMsg.Reg,Perf.Reg,AspVersions.Reg,FreeADO.Reg,IndexServer.Reg\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "XSP.InstallPerVer", 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    strcat(inf, "[Winetest.Services]\n");
    strcat(inf, "AddService=,2\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionA(infhandle, "Winetest.Services", 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* TODO: Test the Flags */
}

static void test_service_install(const char *executable, const char *argument)
{
    struct
    {
        const char *add_service_flags;
        const char *service_type;
        const char *start_type;
        DWORD expect_start_error;
    } tests[] =
    {
        {.add_service_flags = "", .service_type = "1", .start_type = "4", .expect_start_error = ERROR_SERVICE_DISABLED},
        {.add_service_flags = "", .service_type = "0x10", .start_type = "2", .expect_start_error = 0},
        {.add_service_flags = "0x800", .service_type = "0x10", .start_type = "2", .expect_start_error = ERROR_SERVICE_ALREADY_RUNNING},
    };

    SC_HANDLE scm_handle, svc_handle;
    SERVICE_STATUS status;
    BOOL ret;
    char path[MAX_PATH + 9], windir[MAX_PATH], driver[MAX_PATH];
    DWORD i, attrs;
    /* Minimal stuff needed */
    static const char *inf =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "[DestinationDirs]\n"
        "Winetest.DriverFiles=12\n"
        "[DefaultInstall]\n"
        "CopyFiles=Winetest.DriverFiles\n"
        "[DefaultInstall.Services]\n"
        "AddService=Winetest,%s,Winetest.Service\n"
        "[Winetest.Service]\n"
        "ServiceBinary=%%12%%\\winetest.sys %s service\n"
        "ServiceType=%s\n"
        "StartType=%s\n"
        "ErrorControl=1\n"
        "[Winetest.DriverFiles]\n"
        "winetest.sys";
    char buffer[1024];

    /* Bail out if we don't have enough rights */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to install the service\n");
        return;
    }
    CloseServiceHandle(scm_handle);

    /* Place where we expect the driver to be installed */
    GetWindowsDirectoryA(windir, MAX_PATH);
    lstrcpyA(driver, windir);
    lstrcatA(driver, "\\system32\\drivers\\winetest.sys");

    ret = CopyFileA(executable, "winetest.sys", TRUE);
    ok(ret, "CopyFileA failed, error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%lu", i);

        sprintf(buffer, inf, tests[i].add_service_flags, argument, tests[i].service_type, tests[i].start_type);
        create_inf_file(inffile, buffer);
        sprintf(path, "%s\\%s", CURR_DIR, inffile);
        run_cmdline("DefaultInstall", 128, path);

        /* Driver should have been installed */
        attrs = GetFileAttributesA(driver);
        ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected driver to exist\n");

        scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

        /* Open the service to see if it's really there */
        svc_handle = OpenServiceA(scm_handle, "Winetest", SERVICE_START|SERVICE_STOP|SERVICE_QUERY_STATUS|DELETE);
        ok(svc_handle != NULL, "Service was not created\n");

        ret = StartServiceA(svc_handle, 0, NULL);
        if (!tests[i].expect_start_error)
            ok(ret, "StartServiceA failed, error %lu\n", GetLastError());
        else
        {
            ok(!ret, "StartServiceA succeeded\n");
            ok(GetLastError() == tests[i].expect_start_error, "got error %lu\n", GetLastError());
        }

        ret = QueryServiceStatus(svc_handle, &status);
        ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
        while (status.dwCurrentState == SERVICE_START_PENDING)
        {
            Sleep(100);
            ret = QueryServiceStatus(svc_handle, &status);
            ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
        }

        ret = ControlService(svc_handle, SERVICE_CONTROL_STOP, &status);
        while (status.dwCurrentState == SERVICE_STOP_PENDING)
        {
            Sleep(100);
            ret = QueryServiceStatus(svc_handle, &status);
            ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
        }
        ok(status.dwCurrentState == SERVICE_STOPPED, "expected SERVICE_STOPPED, got %ld\n", status.dwCurrentState);

        SetLastError(0xdeadbeef);
        ret = DeleteService(svc_handle);
        ok(ret, "Service could not be deleted : %ld\n", GetLastError());

        CloseServiceHandle(svc_handle);
        CloseServiceHandle(scm_handle);

        /* File cleanup */
        DeleteFileA(inffile);
        DeleteFileA(driver);

        winetest_pop_context();
    }

    DeleteFileA("winetest.sys");
}

static void test_profile_items(void)
{
    char path[MAX_PATH + 22], commonprogs[MAX_PATH];

    static const char *inf =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "[DefaultInstall]\n"
        "ProfileItems=TestItem,TestItem2,TestGroup\n"
        "[TestItem]\n"
        "Name=TestItem\n"
        "CmdLine=11,,notepad.exe\n"
        "[TestItem2]\n"
        "Name=TestItem2\n"
        "CmdLine=11,,notepad.exe\n"
        "SubDir=TestDir\n"
        "[TestGroup]\n"
        "Name=TestGroup,4\n"
        ;

    if (S_OK != SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, commonprogs))
    {
        skip("No common program files directory exists\n");
        goto cleanup;
    }

    sprintf(path, "%s\\TestDir", commonprogs);
    if (!CreateDirectoryA(path, NULL) && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("need admin rights\n");
        return;
    }
    RemoveDirectoryA(path);

    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);

    sprintf(path, "%s\\TestItem.lnk", commonprogs);
    sprintf(path, "%s\\TestDir", commonprogs);
    ok(INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path), "directory not created\n");
    sprintf(path, "%s\\TestDir\\TestItem2.lnk", commonprogs);
    ok(INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path), "link not created\n");
    sprintf(path, "%s\\TestGroup", commonprogs);
    ok(INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path), "group not created\n");

    sprintf(path, "%s\\TestItem.lnk", commonprogs);
    DeleteFileA(path);
    sprintf(path, "%s\\TestDir\\TestItem2.lnk", commonprogs);
    DeleteFileA(path);
    sprintf(path, "%s\\TestItem2.lnk", commonprogs);
    DeleteFileA(path);
    sprintf(path, "%s\\TestDir", commonprogs);
    RemoveDirectoryA(path);
    sprintf(path, "%s\\TestGroup", commonprogs);
    RemoveDirectoryA(path);

cleanup:
    DeleteFileA(inffile);
}

static void test_inffilelistA(void)
{
    static const char inffile2[] = "test2.inf";
    static const char *inf =
        "[Version]\n"
        "Signature=\"$Chicago$\"";

    char buffer[MAX_PATH] = { 0 };
    char dir[MAX_PATH], *p;
    DWORD expected, outsize;
    BOOL ret;

    /* create a private directory, the temp directory may contain some
     * inf files left over from old installations
     */
    if (!GetTempFileNameA(CURR_DIR, "inftest", 1, dir))
    {
        win_skip("GetTempFileNameA failed with error %ld\n", GetLastError());
        return;
    }
    if (!CreateDirectoryA(dir, NULL ))
    {
        win_skip("CreateDirectoryA(%s) failed with error %ld\n", dir, GetLastError());
        return;
    }
    if (!SetCurrentDirectoryA(dir))
    {
        win_skip("SetCurrentDirectoryA failed with error %ld\n", GetLastError());
        RemoveDirectoryA(dir);
        return;
    }

    create_inf_file(inffile, inf);
    create_inf_file(inffile2, inf);

    /* mixed style
     */
    expected = 3 + strlen(inffile) + strlen(inffile2);
    ret = SetupGetInfFileListA(dir, INF_STYLE_OLDNT | INF_STYLE_WIN4, buffer,
                               MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListA to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenA(p) && (outsize > (p - buffer)); p+=lstrlenA(p) + 1)
        ok(!lstrcmpA(p,inffile2) || !lstrcmpA(p,inffile),
            "unexpected filename %s\n",p);

    DeleteFileA(inffile);
    DeleteFileA(inffile2);
    SetCurrentDirectoryA(CURR_DIR);
    RemoveDirectoryA(dir);
}

static void test_inffilelist(void)
{
    static const char inffile2[] = "test2.inf";
    static const WCHAR inffile2W[] = {'t','e','s','t','2','.','i','n','f',0};
    static const char invalid_inf[] = "invalid.inf";
    static const WCHAR invalid_infW[] = {'i','n','v','a','l','i','d','.','i','n','f',0};
    static const char *inf =
        "[Version]\n"
        "Signature=\"$Chicago$\"";
    static const char *inf2 =
        "[Version]\n"
        "Signature=\"$CHICAGO$\"";
    static const char *infNT =
        "[Version]\n"
        "Signature=\"$WINDOWS NT$\"";

    WCHAR *p, *ptr;
    char dirA[MAX_PATH];
    WCHAR dir[MAX_PATH] = { 0 };
    WCHAR buffer[MAX_PATH] = { 0 };
    DWORD expected, outsize;
    BOOL ret;

    /* NULL means %windir%\\inf
     * get the value as reference
     */
    expected = 0;
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(NULL, INF_STYLE_WIN4, NULL, 0, &expected);
    ok(ret, "expected SetupGetInfFileListW to succeed! Error: %ld\n", GetLastError());
    ok(expected > 0, "expected required buffersize to be at least 1\n");

    /* check if an empty string doesn't behaves like NULL */
    outsize = 0;
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, NULL, 0, &outsize);
    ok(!ret, "expected SetupGetInfFileListW to fail!\n");

    /* create a private directory, the temp directory may contain some
     * inf files left over from old installations
     */
    if (!GetTempFileNameA(CURR_DIR, "inftest", 1, dirA))
    {
        win_skip("GetTempFileNameA failed with error %ld\n", GetLastError());
        return;
    }
    if (!CreateDirectoryA(dirA, NULL ))
    {
        win_skip("CreateDirectoryA(%s) failed with error %ld\n", dirA, GetLastError());
        return;
    }
    if (!SetCurrentDirectoryA(dirA))
    {
        win_skip("SetCurrentDirectoryA failed with error %ld\n", GetLastError());
        RemoveDirectoryA(dirA);
        return;
    }

    MultiByteToWideChar(CP_ACP, 0, dirA, -1, dir, MAX_PATH);
    /* check a not existing directory
     */
    ptr = dir + lstrlenW(dir);
    MultiByteToWideChar(CP_ACP, 0, "\\not_existent", -1, ptr, MAX_PATH - lstrlenW(dir));
    outsize = 0xffffffff;
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, NULL, 0, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(outsize == 1, "expected required buffersize to be 1, got %ld\n", outsize);
    ok(ERROR_PATH_NOT_FOUND == GetLastError(),
       "expected error ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    
    create_inf_file(inffile, inf);
    create_inf_file(inffile2, inf);
    create_inf_file(invalid_inf, "This content does not match the inf file format");

    /* pass a filename instead of a directory
     */
    *ptr = '\\';
    MultiByteToWideChar(CP_ACP, 0, invalid_inf, -1, ptr+1, MAX_PATH - lstrlenW(dir));
    outsize = 0xffffffff;
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, NULL, 0, &outsize);
    ok(!ret, "expected SetupGetInfFileListW to fail!\n");
    ok(ERROR_DIRECTORY == GetLastError(),
       "expected error ERROR_DIRECTORY, got %ld\n", GetLastError());

    /* make the filename look like directory
     */
    dir[1 + lstrlenW(dir)] = 0;
    dir[lstrlenW(dir)] = '\\';
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, NULL, 0, &outsize);
    ok(!ret, "expected SetupGetInfFileListW to fail!\n");
    ok(ERROR_DIRECTORY == GetLastError(),
       "expected error ERROR_DIRECTORY, got %ld\n", GetLastError());

    /* now check the buffer contents of a valid call
     */
    *ptr = 0;
    expected = 3 + strlen(inffile) + strlen(inffile2);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    /* upper case value
     */
    create_inf_file(inffile2, inf2);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    /* signature Windows NT is also inf style win4
     */
    create_inf_file(inffile2, infNT);
    expected = 3 + strlen(inffile) + strlen(inffile2);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    /* old style
     */
    expected = 2 + strlen(invalid_inf);
    ret = SetupGetInfFileListW(dir, INF_STYLE_OLDNT, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,invalid_infW), "unexpected filename %s\n",wine_dbgstr_w(p));

    /* mixed style
     */
    expected = 4 + strlen(inffile) + strlen(inffile2) + strlen(invalid_inf);
    ret = SetupGetInfFileListW(dir, INF_STYLE_OLDNT | INF_STYLE_WIN4, buffer,
                               MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %ld, got %ld\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW) || !lstrcmpW(p,invalid_infW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    DeleteFileA(inffile);
    DeleteFileA(inffile2);
    DeleteFileA(invalid_inf);
    SetCurrentDirectoryA(CURR_DIR);
    RemoveDirectoryA(dirA);
}

static const char dirid_inf[] = "[Version]\n"
    "Signature=\"$Chicago$\"\n"
    "[DefaultInstall]\n"
    "AddReg=Add.Settings\n"
    "[Add.Settings]\n"
    "HKCU,Software\\Wine\\setupapitest,dirid,,%%%i%%\n";

static void check_dirid(int dirid, LPCSTR expected)
{
    char buffer[sizeof(dirid_inf)+11];
    char path[MAX_PATH + 9], actual[MAX_PATH];
    LONG ret;
    DWORD size, type;
    HKEY key;

    sprintf(buffer, dirid_inf, dirid);

    create_inf_file(inffile, buffer);

    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);

    size = sizeof(actual);
    actual[0] = '\0';
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest", &key);
    if (ret == ERROR_SUCCESS)
    {
        ret = RegQueryValueExA(key, "dirid", NULL, &type, (BYTE*)&actual, &size);
        RegCloseKey(key);
        if (type != REG_SZ)
            ret = ERROR_FILE_NOT_FOUND;
    }

    ok(ret == ERROR_SUCCESS, "Failed getting value for dirid %i, err=%ld\n", dirid, ret);
    ok(!strcmp(actual, expected), "Expected path for dirid %i was \"%s\", got \"%s\"\n", dirid, expected, actual);

    ok_registry(TRUE);
    ret = DeleteFileA(inffile);
    ok(ret, "Expected source inf to exist, last error was %ld\n", GetLastError());
}

/* Test dirid values */
static void test_dirid(void)
{
    char expected[MAX_PATH];

    check_dirid(DIRID_NULL, "");

    GetWindowsDirectoryA(expected, MAX_PATH);
    check_dirid(DIRID_WINDOWS, expected);

    GetSystemDirectoryA(expected, MAX_PATH);
    check_dirid(DIRID_SYSTEM, expected);

    strcat(expected, "\\unknown");
    check_dirid(40, expected);
}

static void test_install_files_queue(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"

            "[DefaultInstall]\n"
            "CopyFiles=copy_section\n"
            "DelFiles=delete_section\n"
            "RenFiles=rename_section\n"

            "[copy_section]\n"
            "one.txt\n"
            "two.txt\n"
            "three.txt\n"
            "four.txt\n"
            "five.txt\n"
            "six.txt\n"
            "seven.txt\n"
            "eight.txt\n"

            "[delete_section]\n"
            "nine.txt\n"

            "[rename_section]\n"
            "eleven.txt,ten.txt\n"

            "[SourceDisksNames]\n"
            "1=heis\n"
            "2=duo,,,alpha\n"
            "3=treis,treis.cab\n"
            "4=tessares,tessares.cab,,alpha\n"

            "[SourceDisksFiles]\n"
            "one.txt=1\n"
            "two.txt=1,beta\n"
            "three.txt=2\n"
            "four.txt=2,beta\n"
            "five.txt=3\n"
            "six.txt=3,beta\n"
            "seven.txt=4\n"
            "eight.txt=4,beta\n"

            "[DestinationDirs]\n"
            "copy_section=40000,dst\n"
            "delete_section=40000,dst\n"
            "rename_section=40000,dst\n";

    char path[MAX_PATH + 9];
    HSPFILEQ queue;
    void *context;
    HINF hinf;
    BOOL ret;

    create_inf_file(inffile, inf_data);

    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    hinf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    ret = CreateDirectoryA("src", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/alpha", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/alpha/beta", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/beta", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = SetupSetDirectoryIdA(hinf, 40000, CURR_DIR);
    ok(ret, "Failed to set directory ID, error %lu.\n", GetLastError());

    ret = CreateDirectoryA("dst", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());

    create_file("src/one.txt");
    create_file("src/beta/two.txt");
    create_file("src/alpha/three.txt");
    create_file("src/alpha/beta/four.txt");
    create_cab_file("src/treis.cab", "src\\beta\\five.txt\0six.txt\0");
    create_cab_file("src/alpha/tessares.cab", "seven.txt\0eight.txt\0");
    create_file("dst/nine.txt");
    create_file("dst/ten.txt");

    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());

    context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, 0);
    ok(!!context, "Failed to create callback context, error %#lx.\n", GetLastError());

    ret = SetupInstallFilesFromInfSectionA(hinf, NULL, queue, "DefaultInstall", "src", 0);
    ok(ret, "Failed to install files, error %#lx.\n", GetLastError());

    ok(file_exists("src/one.txt"), "Source file should exist.\n");
    ok(!file_exists("dst/one.txt"), "Destination file should not exist.\n");

    ret = SetupCommitFileQueueA(NULL, queue, SetupDefaultQueueCallbackA, context);
    ok(ret, "Failed to commit queue, error %#lx.\n", GetLastError());

    ok(file_exists("src/one.txt"), "Source file should exist.\n");
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/four.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/five.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/six.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/seven.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/eight.txt"), "Destination file should exist.\n");
    ok(!delete_file("dst/nine.txt"), "Destination file should not exist.\n");
    ok(!delete_file("dst/ten.txt"), "Destination file should not exist.\n");
    ok(delete_file("dst/eleven.txt"), "Destination file should exist.\n");

    SetupTermDefaultQueueCallback(context);
    ret = SetupCloseFileQueue(queue);
    ok(ret, "Failed to close queue, error %#lx.\n", GetLastError());

    SetupCloseInfFile(hinf);
    delete_file("src/one.txt");
    delete_file("src/beta/two.txt");
    delete_file("src/alpha/three.txt");
    delete_file("src/alpha/beta/four.txt");
    delete_file("src/treis.cab");
    delete_file("src/alpha/tessares.cab");
    delete_file("src/alpha/beta/");
    delete_file("src/alpha/");
    delete_file("src/beta/");
    delete_file("src/");
    delete_file("dst/");
    ret = DeleteFileA(inffile);
    ok(ret, "Failed to delete INF file, error %lu.\n", GetLastError());
}

static unsigned int got_need_media, got_copy_error, got_start_copy;
static unsigned int testmode;

static UINT WINAPI need_media_cb(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    if (winetest_debug > 1) trace("%p, %#x, %#Ix, %#Ix\n", context, message, param1, param2);

    if (message == SPFILENOTIFY_NEEDMEDIA)
    {
        const SOURCE_MEDIA_A *media = (const SOURCE_MEDIA_A *)param1;
        char *path = (char *)param2;
        UINT ret;

        /* The default callback will fill out SourcePath, but as long as DOIT
         * is returned, it's ignored. */
        ok(!path[0], "Test %u: got path '%s'.\n", testmode, path);
        ret = SetupDefaultQueueCallbackA(context, message, param1, param2);
        ok(!strcmp(path, media->SourcePath), "Test %u: got path '%s'.\n", testmode, path);
        ok(ret == FILEOP_DOIT, "Got unexpected return value %u.\n", ret);
        path[0] = 0;

        if (testmode == 0)
            ok(media->Flags == SP_COPY_WARNIFSKIP, "Got Flags %#lx.\n", media->Flags);
        else
            ok(!media->Flags, "Got Flags %#lx for test %u.\n", media->Flags, testmode);

        switch (testmode)
        {
        case 0:
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->Description, "File One"), "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 1:
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!media->Description, "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src\\beta"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "two.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 2:
            ok(!strcmp(media->Tagfile, "faketag"), "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->Description, "desc"), "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src\\beta"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "two.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 3:
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->Description, "heis"), "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 4:
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->Description, "heis"), "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src\\beta"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "two.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 5:
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->Description, "duo"), "Got Description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src\\alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "three.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 6:
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            testmode = 7;
            break;
        case 7:
            ok(!strcmp(media->SourcePath, "src/alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "three.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 8:
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            testmode = 9;
            break;
        case 9:
            ok(!strcmp(media->SourcePath, "src\\alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "three.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 10:
            ok(!strcmp(media->Description, "desc1"), "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            testmode = 11;
            break;
        case 11:
            ok(!media->Description, "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src\\beta"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "two.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 12:
            ok(!strcmp(media->Description, "desc1"), "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->Tagfile, "faketag"), "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            testmode = 13;
            break;
        case 13:
            ok(!strcmp(media->Description, "desc1"), "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->Tagfile, "faketag2"), "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->SourcePath, "src\\beta"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "two.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 14:
            ok(!strcmp(media->Description, "desc"), "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->Tagfile, "treis.cab"), "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->SourcePath, "src"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "four.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 15:
            ok(!strcmp(media->Description, "desc"), "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->Tagfile, "tessares.cab"), "Got Tagfile '%s'.\n", media->Tagfile);
            ok(!strcmp(media->SourcePath, "src\\alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "seven.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        case 16:
            ok(!media->Description, "Got description '%s'.\n", media->Description);
            ok(!strcmp(media->SourcePath, "src/alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
            ok(!strcmp(media->SourceFile, "six.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            break;
        }

        ++got_need_media;

        return ret;
    }
    else if (message == SPFILENOTIFY_COPYERROR)
    {
        const FILEPATHS_A *paths = (const FILEPATHS_A *)param1;
        ok(0, "Got unexpected copy error %s -> %s.\n", paths->Source, paths->Target);
    }

    return SetupDefaultQueueCallbackA(context, message, param1, param2);
}

static UINT WINAPI need_media_newpath_cb(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    if (winetest_debug > 1) trace("%p, %#x, %#Ix, %#Ix\n", context, message, param1, param2);

    if (message == SPFILENOTIFY_NEEDMEDIA)
    {
        const SOURCE_MEDIA_A *media = (const SOURCE_MEDIA_A *)param1;
        char *path = (char *)param2;

        ++got_need_media;

        if (testmode == 1)
            strcpy(path, "src\\alpha");
        else if (testmode == 2)
        {
            if (got_need_media == 1)
                strcpy(path, "src\\alpha");
            else
            {
                ok(!strcmp(media->SourcePath, "src\\alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
                strcpy(path, "src");
            }
        }
        else if (testmode == 5)
        {
            if (got_need_media == 1)
            {
                ok(!strcmp(media->SourcePath, "fake"), "Got SourcePath '%s'.\n", media->SourcePath);
                ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
                return FILEOP_SKIP;
            }
            else
            {
                ok(!strcmp(media->SourcePath, "fake\\alpha"), "Got SourcePath '%s'.\n", media->SourcePath);
                ok(!strcmp(media->SourceFile, "three.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
                strcpy(path, "src\\alpha");
            }
        }
        else if (testmode == 6)
        {
            /* SourcePath is not really consistent here, but it's not supplied
             * from the INF file. Usually it's a drive root, but not always. */
            ok(!strcmp(media->SourceFile, "one.txt"), "Got SourceFile '%s'.\n", media->SourceFile);
            ok(!media->Description, "Got Description '%s'.\n", media->Description);
            ok(!media->Tagfile, "Got Tagfile '%s'.\n", media->Tagfile);
            strcpy(path, "src");
        }
        else
            strcpy(path, "src");

        return FILEOP_NEWPATH;
    }
    else if (message == SPFILENOTIFY_COPYERROR)
    {
        char *path = (char *)param2;

        ++got_copy_error;

        if (testmode == 3)
        {
            strcpy(path, "src\\alpha");
            return FILEOP_NEWPATH;
        }
        else if (testmode == 4)
        {
            if (got_copy_error == 1)
                strcpy(path, "fake2");
            else
                strcpy(path, "src\\alpha");
            return FILEOP_NEWPATH;
        }
        else
            return FILEOP_SKIP;
    }
    else if (message == SPFILENOTIFY_STARTCOPY) got_start_copy++;

    return SetupDefaultQueueCallbackA(context, message, param1, param2);
}

#define run_queue(a,b) run_queue_(__LINE__,a,b)
static void run_queue_(unsigned int line, HSPFILEQ queue, PSP_FILE_CALLBACK_A cb)
{
    void *context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, 0);
    BOOL ret;
    ok_(__FILE__,line)(!!context, "Failed to create callback context, error %#lx.\n", GetLastError());
    ret = SetupCommitFileQueueA(NULL, queue, cb, context);
    ok_(__FILE__,line)(ret, "Failed to commit queue, error %#lx.\n", GetLastError());
    SetupTermDefaultQueueCallback(context);
    ret = SetupCloseFileQueue(queue);
    ok_(__FILE__,line)(ret, "Failed to close queue, error %#lx.\n", GetLastError());
}

static void test_install_file(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[section1]\n"
            "one.txt\n"
            "two.txt\n"
            "three.txt\n"
            "[SourceDisksNames]\n"
            "1=heis\n"
            "2=duo,,,alpha\n"
            "[SourceDisksFiles]\n"
            "one.txt=1\n"
            "two.txt=1,beta\n"
            "three.txt=2\n"
            "[DestinationDirs]\n"
            "DefaultDestDir=40000,dst\n";

    char path[MAX_PATH + 9];
    INFCONTEXT infctx;
    HINF hinf;
    BOOL ret;

    create_inf_file(inffile, inf_data);

    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    hinf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    ret = CreateDirectoryA("src", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/alpha", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/beta", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("dst", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    create_file("src/one.txt");
    create_file("src/beta/two.txt");
    create_file("src/alpha/three.txt");

    ret = SetupFindFirstLineA(hinf, "section1", "one.txt", &infctx);
    ok(ret, "Failed to find line.\n");
    SetLastError(0xdeadbeef);
    ret = SetupInstallFileA(hinf, &infctx, "one.txt", "src", "one.txt", 0, NULL, NULL);
    ok(ret, "Expected success.\n");
    ok(GetLastError() == ERROR_SUCCESS, "Got unexpected error %#lx.\n", GetLastError());
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    SetLastError(0xdeadbeef);
    ret = SetupInstallFileA(hinf, &infctx, "one.txt", "src", "one.txt", SP_COPY_REPLACEONLY, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_SUCCESS, "Got unexpected error %#lx.\n", GetLastError());
    ok(!file_exists("dst/one.txt"), "Destination file should not exist.\n");

    ret = SetupFindFirstLineA(hinf, "section1", "two.txt", &infctx);
    ok(ret, "Failed to find line.\n");
    SetLastError(0xdeadbeef);
    ret = SetupInstallFileA(hinf, &infctx, "two.txt", "src", "two.txt", 0, NULL, NULL);
    todo_wine ok(ret, "Expected success.\n");
    todo_wine ok(GetLastError() == ERROR_SUCCESS, "Got unexpected error %#lx.\n", GetLastError());
    todo_wine ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    ret = SetupFindFirstLineA(hinf, "section1", "three.txt", &infctx);
    ok(ret, "Failed to find line.\n");
    SetLastError(0xdeadbeef);
    ret = SetupInstallFileA(hinf, &infctx, "three.txt", "src", "three.txt", 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got unexpected error %#lx.\n", GetLastError());
    ok(!file_exists("dst/three.txt"), "Destination file should not exist.\n");

    ret = SetupFindFirstLineA(hinf, "section1", "three.txt", &infctx);
    ok(ret, "Failed to find line.\n");
    SetLastError(0xdeadbeef);
    ret = SetupInstallFileA(hinf, &infctx, "three.txt", "src/alpha", "three.txt", 0, NULL, NULL);
    ok(ret, "Expected success.\n");
    ok(GetLastError() == ERROR_SUCCESS, "Got unexpected error %#lx.\n", GetLastError());
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    SetupCloseInfFile(hinf);
    delete_file("src/one.txt");
    delete_file("src/beta/two.txt");
    delete_file("src/beta/");
    delete_file("src/alpha/three.txt");
    delete_file("src/alpha/");
    delete_file("src/");
    delete_file("dst/");
    ok(delete_file(inffile), "Failed to delete INF file.\n");
}

static void test_need_media(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[section1]\n"
            "one.txt\n"
            "[section2]\n"
            "two.txt\n"
            "[section3]\n"
            "three.txt\n"
            "[section4]\n"
            "one.txt\n"
            "two.txt\n"
            "three.txt\n"
            "[install_section]\n"
            "CopyFiles=section1\n"
            "[SourceDisksNames]\n"
            "1=heis\n"
            "2=duo,,,alpha\n"
            "[SourceDisksFiles]\n"
            "one.txt=1\n"
            "two.txt=1,beta\n"
            "three.txt=2\n"
            "[DestinationDirs]\n"
            "DefaultDestDir=40000,dst\n";

    SP_FILE_COPY_PARAMS_A copy_params = {sizeof(copy_params)};
    char path[MAX_PATH + 9];
    HSPFILEQ queue;
    HINF hinf;
    BOOL ret;

    create_inf_file(inffile, inf_data);

    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    hinf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    ret = CreateDirectoryA("src", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/alpha", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("src/beta", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("dst", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    create_file("src/one.txt");
    create_file("src/beta/two.txt");
    create_file("src/alpha/three.txt");
    create_file("src/alpha/six.txt");
    create_cab_file("src/treis.cab", "four.txt\0five.txt\0");
    create_cab_file("src/alpha/tessares.cab", "seven.txt\0eight.txt\0");

    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", "File One", NULL, "dst", NULL, SP_COPY_WARNIFSKIP);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", "File One", NULL,
            "dst", NULL, SP_COPY_WARNIFSKIP | SP_COPY_REPLACEONLY);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!file_exists("dst/one.txt"), "Destination file should exist.\n");

    /* Test with a subdirectory. */

    got_need_media = 0;
    testmode = 1;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    /* Test with a tag file. */

    got_need_media = 0;
    testmode = 2;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", "desc", "faketag", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    /* Test from INF file. */

    got_need_media = 0;
    testmode = 3;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section1", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupInstallFilesFromInfSectionA(hinf, NULL, queue, "install_section", "src", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());

    ret = SetupQueueDefaultCopyA(queue, hinf, "src", NULL, "one.txt", 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got error %#lx.\n", GetLastError());

    ret = SetupQueueDefaultCopyA(queue, hinf, "src", "one.txt", "one.txt", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 4;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section2", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueDefaultCopyA(queue, hinf, "src", "two.txt", "two.txt", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 5;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section3", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueDefaultCopyA(queue, hinf, "src", "three.txt", "three.txt", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* One callback is sent per source directory. */

    got_need_media = 0;
    testmode = 6;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src/alpha", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* The same rules apply to INF files. Here the subdir specified in the
     * SourceDisksNames counts as part of the root directory, but the subdir in
     * SourceDisksFiles does not. */

    got_need_media = 0;
    testmode = 8;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section4", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());\
    run_queue(queue, need_media_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* Descriptions and tag files also distinguish source paths. */

    got_need_media = 0;
    testmode = 10;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", "desc1", NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 12;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", "desc1", "faketag", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", "desc1", "faketag2", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    /* Test from cabinets. Subdir is only relevant for the first argument. */

    got_need_media = 0;
    testmode = 14;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "four.txt", "desc", "treis.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "alpha", "five.txt", "desc", "treis.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/four.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/five.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 15;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "alpha", "seven.txt", "desc", "tessares.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "eight.txt", "desc", "tessares.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/seven.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/eight.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 16;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueDefaultCopyA(queue, hinf, "src/alpha", "six.txt", "six.txt", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/six.txt"), "Destination file should exist.\n");

    /* Test absolute paths. */

    got_need_media = 0;
    testmode = 1;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", NULL, NULL, "dst", NULL, SP_COPY_SOURCE_ABSOLUTE);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 1;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", "beta", "two.txt", NULL, NULL, "dst", NULL, SP_COPY_SOURCEPATH_ABSOLUTE);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 5;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section3", SP_COPY_SOURCE_ABSOLUTE);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    got_need_media = 0;
    testmode = 5;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopySectionA(queue, "src", hinf, NULL, "section3", SP_COPY_SOURCEPATH_ABSOLUTE);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* Test returning a new path from the NEEDMEDIA callback. */

    testmode = 0;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha", "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* setupapi expects the callback to return the path including the subdir
     * for the first file. It then strips off the final element. If the final
     * element doesn't match the given subdir exactly, then it's not stripped.
     * To make matters even stranger, the first file copied effectively has its
     * subdir removed. */

    testmode = 1;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha", "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "beta", "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");

    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha\\", "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "six.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/six.txt"), "Destination file should exist.\n");

    /* If the source file does not exist (even if the path is valid),
     * SPFILENOTIFY_NEEDMEDIA is resent until it does. */

    testmode = 2;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha", "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    /* If a following file doesn't exist, it results in a copy error instead. */

    testmode = 0;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "fake.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(got_copy_error == 1, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");

    /* Test providing a new path from SPFILENOTIFY_COPYERROR. */

    testmode = 3;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "six.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(got_copy_error == 1, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/six.txt"), "Destination file should exist.\n");

    /* SPFILENOTIFY_COPYERROR will also be resent until the copy is successful. */

    testmode = 4;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "six.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(got_copy_error == 2, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/six.txt"), "Destination file should exist.\n");

    /* Test with cabinet. As above, subdir only matters for the first file. */

    testmode = 0;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "four.txt", "desc", "treis.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha", "five.txt", "desc", "treis.cab", "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(delete_file("dst/four.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/five.txt"), "Destination file should exist.\n");

    /* Test returning FILEOP_SKIP from the NEEDMEDIA handler. */

    testmode = 5;
    got_need_media = got_copy_error = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "fake", "alpha", "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 2, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    ok(!file_exists("dst/one.txt"), "Destination file should not exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    testmode = 6;
    got_need_media = got_copy_error = got_start_copy = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    copy_params.QueueHandle = queue;
    copy_params.SourceFilename = "one.txt";
    /* Leaving TargetDirectory NULL causes it to be filled with garbage on
     * Windows, so the copy may succeed or fail. In any case it's not supplied
     * from LayoutInf. */
    copy_params.TargetDirectory = "dst";
    ret = SetupQueueCopyIndirectA(&copy_params);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, need_media_newpath_cb);
    ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
    ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
    if (got_start_copy) ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    else ok(!file_exists("dst/one.txt"), "Destination file should not exist.\n");

    got_need_media = got_copy_error = got_start_copy = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    copy_params.LayoutInf = hinf;
    copy_params.QueueHandle = queue;
    /* In fact this fails with ERROR_INVALID_PARAMETER on 8+. */
    if (SetupQueueCopyIndirectA(&copy_params))
    {
        run_queue(queue, need_media_newpath_cb);
        ok(got_need_media == 1, "Got %u callbacks.\n", got_need_media);
        ok(!got_copy_error, "Got %u copy errors.\n", got_copy_error);
        if (got_start_copy) ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
        else ok(!file_exists("dst/one.txt"), "Destination file should not exist.\n");
    }
    else
        SetupCloseFileQueue(queue);

    SetupCloseInfFile(hinf);
    delete_file("src/one.txt");
    delete_file("src/beta/two.txt");
    delete_file("src/beta/");
    delete_file("src/alpha/six.txt");
    delete_file("src/alpha/three.txt");
    delete_file("src/alpha/tessares.cab");
    delete_file("src/alpha/");
    delete_file("src/treis.cab");
    delete_file("src/");
    delete_file("dst/");
    ret = DeleteFileA(inffile);
    ok(ret, "Failed to delete INF file, error %lu.\n", GetLastError());
}

static void test_close_queue(void)
{
    void *context;
    BOOL ret;

    context = SetupInitDefaultQueueCallback(NULL);
    ok(!!context, "Failed to create callback context, error %#lx.\n", GetLastError());

    ret = SetupCloseFileQueue(context);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %lu.\n", GetLastError());

    SetupTermDefaultQueueCallback(context);
}

static unsigned int start_copy_ret;

static UINT WINAPI start_copy_cb(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    if (winetest_debug > 1) trace("%p, %#x, %#Ix, %#Ix\n", context, message, param1, param2);

    if (message == SPFILENOTIFY_STARTCOPY)
    {
        const FILEPATHS_A *paths = (const FILEPATHS_A *)param1;

        ++got_start_copy;

        if (strstr(paths->Source, "two.txt"))
        {
            SetLastError(0xdeadf00d);
            return start_copy_ret;
        }
    }

    return SetupDefaultQueueCallbackA(context, message, param1, param2);
}

static void test_start_copy(void)
{
    HSPFILEQ queue;
    void *context;
    BOOL ret;

    ret = CreateDirectoryA("src", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    create_file("src/one.txt");
    create_file("src/two.txt");
    create_file("src/three.txt");

    start_copy_ret = FILEOP_DOIT;
    got_start_copy = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, start_copy_cb);
    todo_wine ok(got_start_copy == 3, "Got %u callbacks.\n", got_start_copy);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/two.txt"), "Destination file should exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    start_copy_ret = FILEOP_SKIP;
    got_start_copy = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    run_queue(queue, start_copy_cb);
    ok(got_start_copy == 3, "Got %u callbacks.\n", got_start_copy);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(!file_exists("dst/two.txt"), "Destination file should not exist.\n");
    ok(delete_file("dst/three.txt"), "Destination file should exist.\n");

    start_copy_ret = FILEOP_ABORT;
    got_start_copy = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "one.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "two.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "src", NULL, "three.txt", NULL, NULL, "dst", NULL, 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, 0);
    SetLastError(0xdeadbeef);
    ret = SetupCommitFileQueueA(NULL, queue, start_copy_cb, context);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadf00d, "Got unexpected error %lu.\n", GetLastError());
    SetupTermDefaultQueueCallback(context);
    SetupCloseFileQueue(queue);
    ok(got_start_copy == 2, "Got %u callbacks.\n", got_start_copy);
    ok(delete_file("dst/one.txt"), "Destination file should exist.\n");
    ok(!file_exists("dst/two.txt"), "Destination file should not exist.\n");
    ok(!file_exists("dst/three.txt"), "Destination file should not exist.\n");

    delete_file("src/one.txt");
    delete_file("src/two.txt");
    delete_file("src/three.txt");
    delete_file("src/");
    delete_file("dst/");
}

static void test_register_dlls(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[DefaultInstall]\n"
            "RegisterDlls=register_section\n"
            "UnregisterDlls=register_section\n"
            "[register_section]\n"
            "40000,,winetest_selfreg.dll,1\n";

    void *context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, 0);
    char path[MAX_PATH];
    HRESULT hr;
    HINF hinf;
    BOOL ret;
    HKEY key;
    LONG l;

    create_inf_file("test.inf", inf_data);
    sprintf(path, "%s\\test.inf", CURR_DIR);
    hinf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    load_resource("selfreg.dll", "winetest_selfreg.dll");
    ret = SetupSetDirectoryIdA(hinf, 40000, CURR_DIR);
    ok(ret, "Failed to set directory ID, error %lu.\n", GetLastError());

    RegDeleteKeyA(HKEY_CURRENT_USER, "winetest_setupapi_selfreg");

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGSVR,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    l = RegOpenKeyA(HKEY_CURRENT_USER, "winetest_setupapi_selfreg", &key);
    ok(!l, "Got error %lu.\n", l);
    RegCloseKey(key);

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_UNREGSVR,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    l = RegOpenKeyA(HKEY_CURRENT_USER, "winetest_setupapi_selfreg", &key);
    ok(l == ERROR_FILE_NOT_FOUND, "Got error %lu.\n", l);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGSVR,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    l = RegOpenKeyA(HKEY_CURRENT_USER, "winetest_setupapi_selfreg", &key);
    ok(!l, "Got error %lu.\n", l);
    RegCloseKey(key);

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_UNREGSVR,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    l = RegOpenKeyA(HKEY_CURRENT_USER, "winetest_setupapi_selfreg", &key);
    ok(l == ERROR_FILE_NOT_FOUND, "Got error %lu.\n", l);

    CoUninitialize();

    SetupCloseInfFile(hinf);
    ret = DeleteFileA("test.inf");
    ok(ret, "Failed to delete INF file, error %lu.\n", GetLastError());
    ret = DeleteFileA("winetest_selfreg.dll");
    ok(ret, "Failed to delete test DLL, error %lu.\n", GetLastError());
}

static unsigned int start_rename_ret, got_start_rename;

static UINT WINAPI start_rename_cb(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    if (winetest_debug > 1) trace("%p, %#x, %#Ix, %#Ix\n", context, message, param1, param2);

    if (message == SPFILENOTIFY_STARTRENAME)
    {
        const FILEPATHS_A *paths = (const FILEPATHS_A *)param1;

        ++got_start_rename;

        if (strstr(paths->Source, "three.txt"))
        {
            SetLastError(0xdeadf00d);
            return start_rename_ret;
        }
    }

    return SetupDefaultQueueCallbackA(context, message, param1, param2);
}

static void test_rename(void)
{
    HSPFILEQ queue;
    BOOL ret;

    ret = CreateDirectoryA("a", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());
    ret = CreateDirectoryA("b", NULL);
    ok(ret, "Failed to create test directory, error %lu.\n", GetLastError());

    create_file("a/one.txt");
    create_file("b/three.txt");
    create_file("a/five.txt");
    create_file("b/six.txt");
    start_rename_ret = FILEOP_DOIT;
    got_start_rename = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueCopyA(queue, "b", NULL, "one.txt", NULL, NULL, "b", "two.txt", 0);
    ok(ret, "Failed to queue copy, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "a", "one.txt", "b", "one.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "b", "three.txt", NULL, "four.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "b", "six.txt", "b", "seven.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "a", "five.txt", "b", "six.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    run_queue(queue, start_rename_cb);
    ok(got_start_rename == 4, "Got %u callbacks.\n", got_start_rename);
    ok(!delete_file("a/one.txt"), "File should not exist.\n");
    ok(!delete_file("a/five.txt"), "File should not exist.\n");
    ok(delete_file("b/one.txt"), "File should exist.\n");
    ok(delete_file("b/two.txt"), "File should exist.\n");
    ok(!delete_file("b/three.txt"), "File should not exist.\n");
    ok(delete_file("b/four.txt"), "File should exist.\n");
    ok(!delete_file("b/five.txt"), "File should not exist.\n");
    ok(delete_file("b/six.txt"), "File should exist.\n");
    ok(delete_file("b/seven.txt"), "File should exist.\n");
    SetupCloseFileQueue(queue);

    create_file("a/one.txt");
    create_file("a/three.txt");
    create_file("a/five.txt");
    start_rename_ret = FILEOP_SKIP;
    got_start_rename = 0;
    queue = SetupOpenFileQueue();
    ok(queue != INVALID_HANDLE_VALUE, "Failed to open queue, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "a", "one.txt", "a", "two.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "a", "three.txt", "a", "four.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    ret = SetupQueueRenameA(queue, "a", "five.txt", "a", "six.txt");
    ok(ret, "Failed to queue rename, error %#lx.\n", GetLastError());
    run_queue(queue, start_rename_cb);
    ok(got_start_rename == 3, "Got %u callbacks.\n", got_start_rename);
    ok(!delete_file("a/one.txt"), "File should not exist.\n");
    ok(delete_file("a/two.txt"), "File should exist.\n");
    ok(delete_file("a/three.txt"), "File should exist.\n");
    ok(!delete_file("a/four.txt"), "File should not exist.\n");
    ok(!delete_file("a/five.txt"), "File should not exist.\n");
    ok(delete_file("a/six.txt"), "File should exist.\n");
    SetupCloseFileQueue(queue);

    ret = delete_file("a/");
    ok(ret, "Failed to delete directory, error %lu.\n", GetLastError());
    ret = delete_file("b/");
    ok(ret, "Failed to delete directory, error %lu.\n", GetLastError());
}

static void test_append_reg(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[DefaultInstall]\n"
            "AddReg=reg_section\n"
            "[reg_section]\n"
            "HKCU,Software\\winetest_setupapi,value,0x10008,\"data\"\n";

    void *context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, 0);
    char path[MAX_PATH];
    DWORD type, size;
    char value[20];
    HINF hinf;
    BOOL ret;
    HKEY key;
    LONG l;

    create_inf_file("test.inf", inf_data);
    sprintf(path, "%s\\test.inf", CURR_DIR);
    hinf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    /* Key doesn't exist yet. */

    RegDeleteKeyA(HKEY_CURRENT_USER, "winetest_setupapi");

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGISTRY,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    l = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\winetest_setupapi", &key);
    ok(!l, "Got error %lu.\n", l);
    size = sizeof(value);
    l = RegQueryValueExA(key, "value", NULL, &type, (BYTE *)value, &size);
    ok(!l, "Got error %lu.\n", l);
    ok(type == REG_MULTI_SZ, "Got type %#lx.\n", type);
    ok(size == sizeof("data\0"), "Got size %lu.\n", size);
    ok(!memcmp(value, "data\0", size), "Got data %s.\n", debugstr_an(value, size));

    /* Key exists and already has a value. */

    l = RegSetValueExA(key, "value", 0, REG_MULTI_SZ, (const BYTE *)"foo\0bar\0", sizeof("foo\0bar\0"));
    ok(!l, "Got error %lu.\n", l);

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGISTRY,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    size = sizeof(value);
    l = RegQueryValueExA(key, "value", NULL, &type, (BYTE *)value, &size);
    ok(!l, "Got error %lu.\n", l);
    ok(type == REG_MULTI_SZ, "Got type %#lx.\n", type);
    ok(size == sizeof("foo\0bar\0data\0"), "Got size %lu.\n", size);
    ok(!memcmp(value, "foo\0bar\0data\0", size), "Got data %s.\n", debugstr_an(value, size));

    /* Key exists and already has the value to be added. */

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGISTRY,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(ret, "Failed to install, error %#lx.\n", GetLastError());

    size = sizeof(value);
    l = RegQueryValueExA(key, "value", NULL, &type, (BYTE *)value, &size);
    ok(!l, "Got error %lu.\n", l);
    ok(type == REG_MULTI_SZ, "Got type %#lx.\n", type);
    ok(size == sizeof("foo\0bar\0data\0"), "Got size %lu.\n", size);
    ok(!memcmp(value, "foo\0bar\0data\0", size), "Got data %s.\n", debugstr_an(value, size));

    /* Key exists and already has a value of the wrong type. */

    l = RegSetValueExA(key, "value", 0, REG_SZ, (const BYTE *)"string", sizeof("string"));
    ok(!l, "Got error %lu.\n", l);

    ret = SetupInstallFromInfSectionA(NULL, hinf, "DefaultInstall", SPINST_REGISTRY,
            NULL, "C:\\", 0, SetupDefaultQueueCallbackA, context, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got error %#lx.\n", GetLastError());

    size = sizeof(value);
    l = RegQueryValueExA(key, "value", NULL, &type, (BYTE *)value, &size);
    ok(!l, "Got error %lu.\n", l);
    ok(type == REG_SZ, "Got type %#lx.\n", type);
    ok(size == sizeof("string"), "Got size %lu.\n", size);
    ok(!memcmp(value, "string", size), "Got data %s.\n", debugstr_an(value, size));

    RegCloseKey(key);

    l = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\winetest_setupapi");
    ok(!l, "Got error %lu.\n", l);

    SetupCloseInfFile(hinf);
    ret = DeleteFileA("test.inf");
    ok(ret, "Failed to delete INF file, error %lu.\n", GetLastError());
}

static WCHAR service_name[] = L"Wine Test Service";
static SERVICE_STATUS_HANDLE service_handle;
static HANDLE stop_event;

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        trace( "shutting down\n" );
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( stop_event );
        return NO_ERROR;
    default:
        trace( "got service ctrl %lx\n", ctrl );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    SERVICE_STATUS status;

    trace( "starting service\n" );

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( L"Wine Test Service", service_handler, NULL );
    if (!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    SetServiceStatus( service_handle, &status );

    WaitForSingleObject( stop_event, INFINITE );

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    trace( "service stopped\n" );
}

START_TEST(install)
{
    char temp_path[MAX_PATH], prev_path[MAX_PATH], path[MAX_PATH];
    char **argv;
    DWORD len;
    int argc;

    argc = winetest_get_mainargs(&argv);
    if (argc > 2 && !strcmp( argv[2], "service" ))
    {
        static const SERVICE_TABLE_ENTRYW service_table[] =
        {
            { service_name, ServiceMain },
            { NULL, NULL }
        };

        StartServiceCtrlDispatcherW( service_table );
        return;
    }

    GetFullPathNameA(argv[0], ARRAY_SIZE(path), path, NULL);
    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    strcpy(CURR_DIR, temp_path);
    len = strlen(CURR_DIR);
    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    /* Set CBT hook to disallow MessageBox creation in current thread */
    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    ok(!!hhook, "Failed to set hook, error %lu.\n", GetLastError());

    test_cmdline();
    test_registry();
    test_install_from();
    test_install_svc_from();
    test_service_install(path, argv[1]);
    test_dirid();
    test_install_files_queue();
    test_need_media();
    test_close_queue();
    test_install_file();
    test_start_copy();
    test_register_dlls();
    test_rename();
    test_append_reg();

    UnhookWindowsHookEx(hhook);

    /* We have to run this test after the CBT hook is disabled because
        ProfileItems needs to create a window on Windows XP. */
    test_profile_items();

    test_inffilelist();
    test_inffilelistA();

    SetCurrentDirectoryA(prev_path);
}
