/*
 * SetupAPI device class-related functions tests
 *
 * Copyright 2015 Víctor Martínez (victor.martinez@reactos.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <apitest.h>
#include <stdio.h>
#include <assert.h>
#include <winuser.h>
#include <winreg.h>
#include <winsvc.h>
#include <setupapi.h>

static const char inffile[] = "test.inf";
static char CURR_DIR[MAX_PATH];

/* Helpers */

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

static void test_SetupInstallServicesFromInfSectionExA(void)
{
    char inf[2048];
    char path[MAX_PATH];
    HINF infhandle;
    BOOL ret;
    SC_HANDLE scm_handle, svc_handle;

    /* Bail out if we are on win98 */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("OpenSCManagerA is not implemented, we are most likely on win9x\n");
        return;
    }
    CloseServiceHandle(scm_handle);

    /* Basic inf file to satisfy SetupOpenInfFileA */
    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    /* Nothing but the Version section */
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "trolololo", 0, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
       "Expected ERROR_SECTION_NOT_FOUND, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);


    /* Add the section */
    strcat(inf, "[Winetest.Services]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
       "Expected ERROR_SECTION_NOT_FOUND, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add a reference */
    strcat(inf, "AddService=Winetest,,Winetest.Service\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the section */
    strcat(inf, "[Winetest.Service]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Just the ServiceBinary */
    strcat(inf, "ServiceBinary=%12%\\winetest.sys\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the ServiceType */
    strcat(inf, "ServiceType=1\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the StartType */
    strcat(inf, "StartType=4\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* This should be it, the minimal entries to install a service */
    strcat(inf, "ErrorControl=1");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "Winetest.Services", 0, NULL, NULL, NULL, NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to install the service\n");
        SetupCloseInfFile(infhandle);
        DeleteFileA(inffile);
        return;
    }
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceA(scm_handle, "Winetest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %d\n", (int)GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    strcat(inf, "[XSP.InstallPerVer]\n");
    strcat(inf, "AddReg=AspEventlogMsg.Reg,Perf.Reg,AspVersions.Reg,FreeADO.Reg,IndexServer.Reg\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExA(infhandle, "XSP.InstallPerVer", 0, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* TODO: Test the Flags */
}
static void test_SetupInstallServicesFromInfSectionExW(void)
{
    char inf[2048];
    char path[MAX_PATH];
    HINF infhandle;
    BOOL ret;
    SC_HANDLE scm_handle, svc_handle;

    /* Bail out if we are on win98 */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerW(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("OpenSCManagerW is not implemented, we are most likely on win9x\n");
        return;
    }
    CloseServiceHandle(scm_handle);

    /* Basic inf file to satisfy SetupOpenInfFileA */
    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    /* Nothing but the Version section */
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"trolololo", 0, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
       "Expected ERROR_SECTION_NOT_FOUND, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);


    /* Add the section */
    strcat(inf, "[Winetest.Services]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
       "Expected ERROR_SECTION_NOT_FOUND, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add a reference */
    strcat(inf, "AddService=Winetest,,Winetest.Service\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the section */
    strcat(inf, "[Winetest.Service]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Just the ServiceBinary */
    strcat(inf, "ServiceBinary=%12%\\winetest.sys\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the ServiceType */
    strcat(inf, "ServiceType=1\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the StartType */
    strcat(inf, "StartType=4\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* This should be it, the minimal entries to install a service */
    strcat(inf, "ErrorControl=1");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"Winetest.Services", 0, NULL, NULL, NULL, NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to install the service\n");
        SetupCloseInfFile(infhandle);
        DeleteFileA(inffile);
        return;
    }
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    scm_handle = OpenSCManagerW(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceW(scm_handle, L"Winetest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %d\n", (int)GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    strcat(inf, "[XSP.InstallPerVer]\n");
    strcat(inf, "AddReg=AspEventlogMsg.Reg,Perf.Reg,AspVersions.Reg,FreeADO.Reg,IndexServer.Reg\n");
    create_inf_file(inffile, inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupInstallServicesFromInfSectionExW(infhandle, L"XSP.InstallPerVer", 0, NULL, NULL, NULL, NULL);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %08d\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* TODO: Test the Flags */
}


START_TEST(SetupInstallServicesFromInfSectionEx)
{
    char temp_path[MAX_PATH], prev_path[MAX_PATH]; 

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    strcpy(CURR_DIR, temp_path);

    /* Set CBT hook to disallow MessageBox creation in current thread */
    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    assert(hhook != 0);

    test_SetupInstallServicesFromInfSectionExA();
    test_SetupInstallServicesFromInfSectionExW();

    UnhookWindowsHookEx(hhook);
    SetCurrentDirectoryA(prev_path);
}
