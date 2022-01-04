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
#include <strsafe.h>

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
    ret = WriteFile(handle, data, lstrlenA(data), &res, NULL);
    assert(ret != 0);
    CloseHandle(handle);
}

static void test_SetupDiInstallClassExA(void)
{
    char inf[2048];
    char path[MAX_PATH];
    BOOL ret;
    ULONG del;
    HKEY RegHandle;
    HINF infhandle;
    SC_HANDLE scm_handle, svc_handle;

    /* [Version]:Signature */
    strcpy(inf, "[Version]\nSignature=\"$Chicago$\"\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_CLASS,
       "Expected ERROR_INVALID_CLASS, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);

    /* [Version]:Signature+Class */
    strcat(inf, "Class=MySampleClass\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_CLASS,
       "Expected ERROR_INVALID_CLASS, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);

    /* [Version]:Signature+Class+ClassGUID */
    strcat(inf, "ClassGuid={3b409830-5f9d-432a-abf5-7d2e4e102467}\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
       "Expected ERROR_SECTION_NOT_FOUND, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);

    /* [Version]Signature+Class+ClassGUID;[ClassInstall32.NT]Empty */
    strcat(inf, "[ClassInstall32.NT]\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(!GetLastError(),
       "Expected no error, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);
    if (ret == TRUE)
    {
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &RegHandle);
        del = RegDeleteKeyW(RegHandle, L"CurrentControlSet\\Control\\Class\\{3B409830-5F9D-432A-ABF5-7D2E4E102467}");
        ok(del == ERROR_SUCCESS, "Expected success\n");
    }

    /* [Version]Signature+Class+ClassGUID;[ClassInstall32.NT]AddReg */
    strcat(inf, "AddReg=SampleClassAddReg\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(!GetLastError(),
       "Expected no error, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);
    if (ret == TRUE)
    {
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &RegHandle);
        del = RegDeleteKeyW(RegHandle, L"CurrentControlSet\\Control\\Class\\{3B409830-5F9D-432A-ABF5-7D2E4E102467}");
        ok(del == ERROR_SUCCESS, "Expected success\n");
    }

    /* [Version]Signature+Class+ClassGUID;[ClassInstall32.NT]AddReg; [SampleClassAddReg];*/
    strcat(inf, "[SampleClassAddReg]\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(!GetLastError(),
        "Expected no error, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);
    if (ret == TRUE)
    {
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &RegHandle);
        del = RegDeleteKeyW(RegHandle, L"CurrentControlSet\\Control\\Class\\{3B409830-5F9D-432A-ABF5-7D2E4E102467}");
        ok(del == ERROR_SUCCESS, "Expected success\n");
    }

    /* [Version]Signature+Class+ClassGUID;[ClassInstall32.NT]AddReg; [SampleClassAddReg]HKR;*/
    strcat(inf, "HKR,,,,\"ReactOS Test SetupDiInstallClassExA\"\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(!GetLastError(),
       "Expected no error, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);
    if (ret == TRUE)
    {
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &RegHandle);
        del = RegDeleteKeyW(RegHandle, L"CurrentControlSet\\Control\\Class\\{3B409830-5F9D-432A-ABF5-7D2E4E102467}");
        ok(del == ERROR_SUCCESS, "Expected success\n");
    }

    /*[Version]Signature+Class+ClassGUID;[ClassInstall32.NT]AddReg;[SampleClassAddReg]HKR;[ClassInstall32.NT.Services]*/
    strcat(inf, "[ClassInstall32.NT.Services]\n");
    create_inf_file(inffile, inf);
    StringCbPrintfA(path, sizeof(path), "%s\\%s", CURR_DIR, inffile);

    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(ret, "Expected success\n");
    ok(!GetLastError(),
        "Expected no error, got %08x\n", (int)GetLastError());
    DeleteFileA(inffile);
    if (ret == TRUE)
    {
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &RegHandle);
        del = RegDeleteKeyW(RegHandle, L"CurrentControlSet\\Control\\Class\\{3B409830-5F9D-432A-ABF5-7D2E4E102467}");
        ok(del == ERROR_SUCCESS, "Expected success\n");
    }

    /* Add a reference */
    strcat(inf, "AddService=Reactostest,,Reactostest.Service\n");
    create_inf_file(inffile, inf);
    infhandle =  SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the section */
    strcat(inf, "[Reactostest.Service]\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Just the ServiceBinary */
    strcat(inf, "ServiceBinary=%12%\\reactostest.sys\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the ServiceType */
    strcat(inf, "ServiceType=1\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* Add the StartType */
    strcat(inf, "StartType=4\n");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_BAD_SERVICE_INSTALLSECT,
       "Expected ERROR_BAD_SERVICE_INSTALLSECT, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    /* This should be it, the minimal entries to install a service */
    strcat(inf, "ErrorControl=1");
    create_inf_file(inffile, inf);
    infhandle = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    SetLastError(0xdeadbeef);
    ret = SetupDiInstallClassExA(NULL, path, DI_QUIETINSTALL, NULL, NULL, NULL,NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to install the service\n");
        SetupCloseInfFile(infhandle);
        DeleteFileA(inffile);
        return;
    }
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", (int)GetLastError());
    SetupCloseInfFile(infhandle);
    DeleteFileA(inffile);

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    /* Open the service to see if it's really there */
    svc_handle = OpenServiceA(scm_handle, "Reactostest", DELETE);
    ok(svc_handle != NULL, "Service was not created\n");

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Service could not be deleted : %d\n", (int)GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);
}


START_TEST(SetupDiInstallClassExA)
{
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    SIZE_T len;

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    strcpy(CURR_DIR, temp_path);
    len = strlen(CURR_DIR);
    if (len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    test_SetupDiInstallClassExA();
    SetCurrentDirectoryA(prev_path);

}
