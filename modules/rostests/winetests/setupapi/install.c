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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "winsvc.h"
#include "setupapi.h"
#include "shlobj.h"

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

static void create_inf_file(LPCSTR filename, const char *data)
{
    DWORD res;
    BOOL ret;
    HANDLE handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(handle != INVALID_HANDLE_VALUE);
    ret = WriteFile(handle, data, strlen(data), &res, NULL);
    assert(ret != 0);
    CloseHandle(handle);
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
       "Expected registry key Software\\Wine\\setupapitest to %s, RegDeleteKey returned %d\n",
       expectsuccess ? "exist" : "not exist",
       ret);
}

/* Test command line processing */
static void test_cmdline(void)
{
    static const char infwithspaces[] = "test file.inf";
    char path[MAX_PATH];
    BOOL ret;

    create_inf_file(inffile, cmdline_inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(TRUE);
    ret = DeleteFileA(inffile);
    ok(ret, "Expected source inf to exist, last error was %d\n", GetLastError());

    /* Test handling of spaces in path, unquoted and quoted */
    create_inf_file(infwithspaces, cmdline_inf);

    sprintf(path, "%s\\%s", CURR_DIR, infwithspaces);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(TRUE);

    sprintf(path, "\"%s\\%s\"", CURR_DIR, infwithspaces);
    run_cmdline("DefaultInstall", 128, path);
    ok_registry(FALSE);

    ret = DeleteFileA(infwithspaces);
    ok(ret, "Expected source inf to exist, last error was %d\n", GetLastError());
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
    char path[MAX_PATH];
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
    ok(ret, "Expected source inf to exist, last error was %d\n", GetLastError());
}

static void test_install_from(void)
{
    char path[MAX_PATH];
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
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

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
    char path[MAX_PATH];
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
        "Expected ERROR_SECTION_NOT_FOUND, got %08x\n", GetLastError());
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
        "Expected ERROR_SECTION_NOT_FOUND, got %08x\n", GetLastError());
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
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", GetLastError());
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
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", GetLastError());
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
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", GetLastError());
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
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", GetLastError());
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
        "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", GetLastError());
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
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceA(scm_handle, "Winetest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %d\n", GetLastError());

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
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08x\n", GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* TODO: Test the Flags */
}

static void test_driver_install(void)
{
    HANDLE handle;
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    char path[MAX_PATH], windir[MAX_PATH], driver[MAX_PATH];
    DWORD attrs;
    /* Minimal stuff needed */
    static const char *inf =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "[DestinationDirs]\n"
        "Winetest.DriverFiles=12\n"
        "[DefaultInstall]\n"
        "CopyFiles=Winetest.DriverFiles\n"
        "[DefaultInstall.Services]\n"
        "AddService=Winetest,,Winetest.Service\n"
        "[Winetest.Service]\n"
        "ServiceBinary=%12%\\winetest.sys\n"
        "ServiceType=1\n"
        "StartType=4\n"
        "ErrorControl=1\n"
        "[Winetest.DriverFiles]\n"
        "winetest.sys";

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

    /* Create a dummy driver file */
    handle = CreateFileA("winetest.sys", GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(handle);

    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    run_cmdline("DefaultInstall", 128, path);

    /* Driver should have been installed */
    attrs = GetFileAttributesA(driver);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected driver to exist\n");

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceA(scm_handle, "Winetest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %d\n", GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    /* File cleanup */
    DeleteFileA(inffile);
    DeleteFileA("winetest.sys");
    DeleteFileA(driver);
}

static void test_profile_items(void)
{
    char path[MAX_PATH], commonprogs[MAX_PATH];

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
        win_skip("GetTempFileNameA failed with error %d\n", GetLastError());
        return;
    }
    if (!CreateDirectoryA(dir, NULL ))
    {
        win_skip("CreateDirectoryA(%s) failed with error %d\n", dir, GetLastError());
        return;
    }
    if (!SetCurrentDirectoryA(dir))
    {
        win_skip("SetCurrentDirectoryA failed with error %d\n", GetLastError());
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
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
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
    ok(ret, "expected SetupGetInfFileListW to succeed! Error: %d\n", GetLastError());
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
        win_skip("GetTempFileNameA failed with error %d\n", GetLastError());
        return;
    }
    if (!CreateDirectoryA(dirA, NULL ))
    {
        win_skip("CreateDirectoryA(%s) failed with error %d\n", dirA, GetLastError());
        return;
    }
    if (!SetCurrentDirectoryA(dirA))
    {
        win_skip("SetCurrentDirectoryA failed with error %d\n", GetLastError());
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
    ok(outsize == 1, "expected required buffersize to be 1, got %d\n", outsize);
    ok(ERROR_PATH_NOT_FOUND == GetLastError(),
       "expected error ERROR_PATH_NOT_FOUND, got %d\n", GetLastError());
    
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
       "expected error ERROR_DIRECTORY, got %d\n", GetLastError());

    /* make the filename look like directory
     */
    dir[1 + lstrlenW(dir)] = 0;
    dir[lstrlenW(dir)] = '\\';
    SetLastError(0xdeadbeef);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, NULL, 0, &outsize);
    ok(!ret, "expected SetupGetInfFileListW to fail!\n");
    ok(ERROR_DIRECTORY == GetLastError(),
       "expected error ERROR_DIRECTORY, got %d\n", GetLastError());

    /* now check the buffer contents of a valid call
     */
    *ptr = 0;
    expected = 3 + strlen(inffile) + strlen(inffile2);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    /* upper case value
     */
    create_inf_file(inffile2, inf2);
    ret = SetupGetInfFileListW(dir, INF_STYLE_WIN4, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
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
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,inffile2W) || !lstrcmpW(p,inffileW),
            "unexpected filename %s\n",wine_dbgstr_w(p));

    /* old style
     */
    expected = 2 + strlen(invalid_inf);
    ret = SetupGetInfFileListW(dir, INF_STYLE_OLDNT, buffer, MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
         expected, outsize);
    for(p = buffer; lstrlenW(p) && (outsize > (p - buffer)); p+=lstrlenW(p) + 1)
        ok(!lstrcmpW(p,invalid_infW), "unexpected filename %s\n",wine_dbgstr_w(p));

    /* mixed style
     */
    expected = 4 + strlen(inffile) + strlen(inffile2) + strlen(invalid_inf);
    ret = SetupGetInfFileListW(dir, INF_STYLE_OLDNT | INF_STYLE_WIN4, buffer,
                               MAX_PATH, &outsize);
    ok(ret, "expected SetupGetInfFileListW to succeed!\n");
    ok(expected == outsize, "expected required buffersize to be %d, got %d\n",
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
    char path[MAX_PATH], actual[MAX_PATH];
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

    ok(ret == ERROR_SUCCESS, "Failed getting value for dirid %i, err=%d\n", dirid, ret);
    ok(!strcmp(actual, expected), "Expected path for dirid %i was \"%s\", got \"%s\"\n", dirid, expected, actual);

    ok_registry(TRUE);
    ret = DeleteFileA(inffile);
    ok(ret, "Expected source inf to exist, last error was %d\n", GetLastError());
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

START_TEST(install)
{
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    DWORD len;

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    strcpy(CURR_DIR, temp_path);
    len = strlen(CURR_DIR);
    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    /* Set CBT hook to disallow MessageBox creation in current thread */
    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    assert(hhook != 0);

    test_cmdline();
    test_registry();
    test_install_from();
    test_install_svc_from();
    test_driver_install();
    test_dirid();

    UnhookWindowsHookEx(hhook);

    /* We have to run this test after the CBT hook is disabled because
        ProfileItems needs to create a window on Windows XP. */
    test_profile_items();

    test_inffilelist();
    test_inffilelistA();

    SetCurrentDirectoryA(prev_path);
}
