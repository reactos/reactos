/*
 * Unit tests for setupapi.dll query functions
 *
 * Copyright (C) 2006 James Hawkins
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
#include <windows.h>
#include <setupapi.h>
#include "wine/test.h"

static CHAR CURR_DIR[MAX_PATH];
static CHAR WIN_DIR[MAX_PATH];

static void get_directories(void)
{
    int len;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len-1] == '\\'))
        CURR_DIR[len-1] = 0;

    GetWindowsDirectoryA(WIN_DIR, MAX_PATH);
    len = lstrlenA(WIN_DIR);

    if (len && (WIN_DIR[len-1] == '\\'))
        WIN_DIR[len-1] = 0;
}

static const char inf_data1[] =
    "[Version]\n"
    "Signature=\"$Chicago$\"\n"
    "AdvancedINF=2.5\n"
    "[SourceDisksNames]\n"
    "2 = %SrcDiskName%, LANCOM\\LANtools\\lanconf.cab\n"
    "[SourceDisksFiles]\n"
    "lanconf.exe = 2\n"
    "[DestinationDirs]\n"
    "DefaultDestDir = 24, %DefaultDest%\n"
    "[Strings]\n"
    "LangDir = english\n"
    "DefaultDest = LANCOM\n"
    "SrcDiskName = \"LANCOM Software CD\"\n";

static const char inf_data2[] =
    "[SourceFileInfo]\n"
    "sp1qfe\\bitsinst.exe=250B3702C7CCD7C2F9E4DAA1555C933E,000600060A28062C,27136,SP1QFE\n"
    "sp1qfe\\bitsprx2.dll=4EBEA67F4BB4EB402E725CA7CA2857AE,000600060A280621,7680,SP1QFE\n"
    "sp1qfe\\bitsprx3.dll=C788A1D9330DA011EF25E95D3BC7BDE5,000600060A280621,7168,SP1QFE\n"
    "sp1qfe\\qmgr.dll=696AC82FB290A03F205901442E0E9589,000600060A280621,361984,SP1QFE\n"
    "sp1qfe\\qmgrprxy.dll=8B5848144829E1BC985EA4C3D8CA7E3F,000600060A280621,17408,SP1QFE\n"
    "sp1qfe\\winhttp.dll=3EC6F518114606CA59D4160322077437,000500010A280615,331776,SP1QFE\n"
    "sp1qfe\\xpob2res.dll=DB83156B9F496F20D1EA70E4ABEC0166,000500010A280622,158720,SP1QFE\n";

static const char inf_data3[] =
    "[Version]\n"
    "Signature = \"$Windows NT$\"\n"
    "[DestinationDirs]\n"
    "CopyAlways.Windir.files = 10\n"
    "[CopyAlways.Windir.Files]\n"
    "WindowsCodecs.dll\n";

static const char inf_data4[] =
    "[Version]\n"
    "Signature = \"$Windows NT$\"\n"
    "[CopyAlways.System32.Files]\n"
    "WindowsCodecs.dll\n";

static const char inf_data5[] =
    "[Version]\n"
    "Signature = \"$Windows NT$\"\n"
    "[DestinationDirs]\n"
    "DefaultDestDir = 11\n"
    "CopyAlways.Windir.files = 10\n"
    "[CopyAlways.Windir.Files]\n"
    "WindowsCodecs.dll\n";

static const char inf_data6[] =
    "[Version]\n"
    "Signature = \"$Windows NT$\"\n"
    "[DestinationDirs]\n"
    "DefaultDestDir = 10\n"
    "[CopyAlways.Windir.Files]\n"
    "WindowsCodecs.dll\n";

static BOOL create_inf_file(LPSTR filename, const char *data, DWORD size)
{
    BOOL ret;
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return FALSE;
    ret = WriteFile(hf, data, size, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
    return ret;
}

static BOOL check_info_filename(PSP_INF_INFORMATION info, LPSTR test)
{
    LPSTR filename;
    DWORD size;
    BOOL ret = FALSE;

    if (!SetupQueryInfFileInformationA(info, 0, NULL, 0, &size))
        return FALSE;

    filename = HeapAlloc(GetProcessHeap(), 0, size);
    if (!filename)
        return FALSE;

    SetupQueryInfFileInformationA(info, 0, filename, size, &size);

    if (!lstrcmpiA(test, filename))
        ret = TRUE;

    HeapFree(GetProcessHeap(), 0, filename);
    return ret;
}

static PSP_INF_INFORMATION alloc_inf_info(LPCSTR filename, DWORD search, PDWORD size)
{
    PSP_INF_INFORMATION info;
    BOOL ret;

    ret = SetupGetInfInformationA(filename, search, NULL, 0, size);
    if (!ret)
        return NULL;

    info = HeapAlloc(GetProcessHeap(), 0, *size);
    return info;
}

static void test_SetupGetInfInformation(void)
{
    PSP_INF_INFORMATION info;
    CHAR inf_filename[MAX_PATH];
    CHAR inf_one[MAX_PATH], inf_two[MAX_PATH];
    LPSTR revfile;
    DWORD size;
    HINF hinf;
    BOOL ret;

    lstrcpyA(inf_filename, CURR_DIR);
    lstrcatA(inf_filename, "\\");
    lstrcatA(inf_filename, "test.inf");

    /* try an invalid inf handle */
    size = 0xdeadbeef;
    SetLastError(0xbeefcafe);
    ret = SetupGetInfInformationA(NULL, INFINFO_INF_SPEC_IS_HINF, NULL, 0, &size);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE ||
       broken(GetLastError() == ERROR_BAD_PATHNAME) || /* win95 */
       broken(GetLastError() == ERROR_FILE_NOT_FOUND) || /* win98 */
       broken(GetLastError() == ERROR_PATH_NOT_FOUND) || /* NT4 */
       broken(GetLastError() == ERROR_INVALID_NAME) || /* win2k */
       broken(GetLastError() == ERROR_GENERAL_SYNTAX), /* another win2k / winMe */
       "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(size == 0xdeadbeef, "Expected size to remain unchanged\n");

    /* try an invalid inf filename */
    /* do not use NULL as absolute inf filename on win9x/winMe (crash) */
    if ((GetLastError() != ERROR_BAD_PATHNAME) &&   /* win95 */
        (GetLastError() != ERROR_FILE_NOT_FOUND) &&  /* win98 */
        (GetLastError() != ERROR_GENERAL_SYNTAX))  /* winMe */
    {
        size = 0xdeadbeef;
        SetLastError(0xbeefcafe);
        ret = SetupGetInfInformationA(NULL, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, 0, &size);
        ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
        ok(size == 0xdeadbeef, "Expected size to remain unchanged\n");
    }

    create_inf_file(inf_filename, inf_data1, sizeof(inf_data1) - 1);

    /* try an invalid search flag */
    size = 0xdeadbeef;
    SetLastError(0xbeefcafe);
    ret = SetupGetInfInformationA(inf_filename, -1, NULL, 0, &size);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(size == 0xdeadbeef, "Expected size to remain unchanged\n");

    /* try a nonexistent inf file */
    size = 0xdeadbeef;
    SetLastError(0xbeefcafe);
    ret = SetupGetInfInformationA("idontexist", INFINFO_INF_NAME_IS_ABSOLUTE, NULL, 0, &size);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    ok(size == 0xdeadbeef, "Expected size to remain unchanged\n");

    /* successfully open the inf file */
    size = 0xdeadbeef;
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, 0, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ok(size != 0xdeadbeef, "Expected a valid size on return\n");

    /* set ReturnBuffer to NULL and ReturnBufferSize to non-zero value 'size' */
    SetLastError(0xbeefcafe);
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, size, &size);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* set ReturnBuffer to NULL and ReturnBufferSize to non-zero value 'size-1' */
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, size-1, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");

    /* some tests for behaviour with a NULL RequiredSize pointer */
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, 0, NULL);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, size - 1, NULL);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, size, NULL);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");

    info = HeapAlloc(GetProcessHeap(), 0, size);

    /* try valid ReturnBuffer but too small size */
    SetLastError(0xbeefcafe);
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, info, size - 1, &size);
    ok(ret == FALSE, "Expected SetupGetInfInformation to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    /* successfully get the inf information */
    ret = SetupGetInfInformationA(inf_filename, INFINFO_INF_NAME_IS_ABSOLUTE, info, size, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ok(check_info_filename(info, inf_filename), "Expected returned filename to be equal\n");

    HeapFree(GetProcessHeap(), 0, info);

    /* try the INFINFO_INF_SPEC_IS_HINF search flag */
    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    info = alloc_inf_info(hinf, INFINFO_INF_SPEC_IS_HINF, &size);
    ret = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, info, size, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ok(check_info_filename(info, inf_filename), "Expected returned filename to be equal\n");
    SetupCloseInfFile(hinf);

    lstrcpyA(inf_two, WIN_DIR);
    lstrcatA(inf_two, "\\system32\\");
    lstrcatA(inf_two, "test.inf");
    ret = create_inf_file(inf_two, inf_data1, sizeof(inf_data1) - 1);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("need admin rights\n");
        goto done;
    }
    ok(ret, "can't create inf file %lu\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, info);
    info = alloc_inf_info("test.inf", INFINFO_DEFAULT_SEARCH, &size);

    /* check if system32 is searched for inf */
    ret = SetupGetInfInformationA("test.inf", INFINFO_DEFAULT_SEARCH, info, size, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
        revfile = inf_one; /* Vista */
    else
        revfile = inf_two;

    lstrcpyA(inf_one, WIN_DIR);
    lstrcatA(inf_one, "\\inf\\");
    lstrcatA(inf_one, "test.inf");
    create_inf_file(inf_one, inf_data1, sizeof(inf_data1) - 1);

    HeapFree(GetProcessHeap(), 0, info);
    info = alloc_inf_info("test.inf", INFINFO_DEFAULT_SEARCH, &size);

    /* test the INFINFO_DEFAULT_SEARCH search flag */
    ret = SetupGetInfInformationA("test.inf", INFINFO_DEFAULT_SEARCH, info, size, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed: %ld\n", GetLastError());
    ok(check_info_filename(info, inf_one), "Expected returned filename to be equal\n");

    HeapFree(GetProcessHeap(), 0, info);
    info = alloc_inf_info("test.inf", INFINFO_REVERSE_DEFAULT_SEARCH, &size);

    /* test the INFINFO_REVERSE_DEFAULT_SEARCH search flag */
    ret = SetupGetInfInformationA("test.inf", INFINFO_REVERSE_DEFAULT_SEARCH, info, size, &size);
    ok(ret == TRUE, "Expected SetupGetInfInformation to succeed\n");
    ok(check_info_filename(info, revfile), "Expected returned filename to be equal\n");

done:
    HeapFree(GetProcessHeap(), 0, info);
    DeleteFileA(inf_filename);
    DeleteFileA(inf_one);
    DeleteFileA(inf_two);
}

static void test_SetupGetSourceFileLocation(void)
{
    char buffer[MAX_PATH] = "not empty", inf_filename[MAX_PATH];
    UINT source_id;
    DWORD required, error;
    HINF hinf;
    BOOL ret;

    lstrcpyA(inf_filename, CURR_DIR);
    lstrcatA(inf_filename, "\\");
    lstrcatA(inf_filename, "test.inf");

    create_inf_file(inf_filename, inf_data1, sizeof(inf_data1) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;
    source_id = 0;

    ret = SetupGetSourceFileLocationA(hinf, NULL, "lanconf.exe", &source_id, buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetSourceFileLocation failed\n");

    ok(required == 1, "unexpected required size: %ld\n", required);
    ok(source_id == 2, "unexpected source id: %d\n", source_id);
    ok(!lstrcmpA("", buffer), "unexpected result string: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);

    create_inf_file(inf_filename, inf_data2, sizeof(inf_data2) - 1);

    SetLastError(0xdeadbeef);
    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    error = GetLastError();
    ok(hinf == INVALID_HANDLE_VALUE, "could open inf file\n");
    ok(error == ERROR_WRONG_INF_STYLE, "got wrong error: %ld\n", error);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_OLDNT, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    ret = SetupGetSourceFileLocationA(hinf, NULL, "", &source_id, buffer, sizeof(buffer), &required);
    ok(!ret, "SetupGetSourceFileLocation succeeded\n");

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);
}

static void test_SetupGetSourceInfo(void)
{
    char buffer[MAX_PATH], inf_filename[MAX_PATH];
    DWORD required;
    HINF hinf;
    BOOL ret;

    lstrcpyA(inf_filename, CURR_DIR);
    lstrcatA(inf_filename, "\\");
    lstrcatA(inf_filename, "test.inf");

    create_inf_file(inf_filename, inf_data1, sizeof(inf_data1) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;

    ret = SetupGetSourceInfoA(hinf, 2, SRCINFO_PATH, buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetSourceInfoA failed\n");

    ok(required == 1, "unexpected required size: %ld\n", required);
    ok(!lstrcmpA("", buffer), "unexpected result string: %s\n", buffer);

    required = 0;
    buffer[0] = 0;

    ret = SetupGetSourceInfoA(hinf, 2, SRCINFO_TAGFILE, buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetSourceInfoA failed\n");

    ok(required == 28, "unexpected required size: %ld\n", required);
    ok(!lstrcmpA("LANCOM\\LANtools\\lanconf.cab", buffer), "unexpected result string: %s\n", buffer);

    required = 0;
    buffer[0] = 0;

    ret = SetupGetSourceInfoA(hinf, 2, SRCINFO_DESCRIPTION, buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetSourceInfoA failed\n");

    ok(required == 19, "unexpected required size: %ld\n", required);
    ok(!lstrcmpA("LANCOM Software CD", buffer), "unexpected result string: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);
}

static void test_SetupGetTargetPath(void)
{
    char buffer[MAX_PATH], inf_filename[MAX_PATH];
    char destfile[MAX_PATH];
    DWORD required;
    HINF hinf;
    INFCONTEXT ctx;
    BOOL ret;

    lstrcpyA(inf_filename, CURR_DIR);
    lstrcatA(inf_filename, "\\");
    lstrcatA(inf_filename, "test.inf");

    create_inf_file(inf_filename, inf_data1, sizeof(inf_data1) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    ctx.Inf = hinf;
    ctx.CurrentInf = hinf;
    ctx.Section = 7;
    ctx.Line = 0;

    required = 0;

    ret = SetupGetTargetPathA(hinf, &ctx, NULL, buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetTargetPathA failed\n");
    ok(required == 10, "unexpected required size: %ld\n", required);
    /* Retrieve the system drive from the windows directory.
     * (%SystemDrive% is not available on Win9x)
     */
    lstrcpyA(destfile, WIN_DIR);
    destfile[3] = '\0';
    lstrcatA(destfile, "LANCOM");
    ok(!lstrcmpiA(destfile, buffer), "unexpected result string: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);

    create_inf_file(inf_filename, inf_data3, sizeof(inf_data3) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;

    ret = SetupGetTargetPathA(hinf, NULL, "CopyAlways.Windir.Files", buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetTargetPathA failed\n");

    lstrcpyA(destfile, WIN_DIR);

    ok(required == lstrlenA(destfile) + 1, "unexpected required size: %ld\n", required);
    ok(!lstrcmpiA(buffer, destfile), "unexpected path: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);

    create_inf_file(inf_filename, inf_data4, sizeof(inf_data4) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;

    ret = SetupGetTargetPathA(hinf, NULL, "CopyAlways.System32.Files", buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetTargetPathA failed\n");

    GetSystemDirectoryA(destfile, MAX_PATH);

    ok(required == lstrlenA(destfile) + 1, "unexpected required size: %ld\n", required);
    ok(!lstrcmpiA(buffer, destfile), "unexpected path: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);

    create_inf_file(inf_filename, inf_data5, sizeof(inf_data5) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;

    ret = SetupGetTargetPathA(hinf, NULL, "CopyAlways.Windir.Files", buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetTargetPathA failed\n");

    lstrcpyA(destfile, WIN_DIR);

    ok(required == lstrlenA(destfile) + 1, "unexpected required size: %ld\n", required);
    ok(!lstrcmpiA(buffer, destfile), "unexpected path: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);

    create_inf_file(inf_filename, inf_data6, sizeof(inf_data6) - 1);

    hinf = SetupOpenInfFileA(inf_filename, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "could not open inf file\n");

    required = 0;

    ret = SetupGetTargetPathA(hinf, NULL, "CopyAlways.Windir.Files", buffer, sizeof(buffer), &required);
    ok(ret, "SetupGetTargetPathA failed\n");

    lstrcpyA(destfile, WIN_DIR);

    ok(required == lstrlenA(destfile) + 1, "unexpected required size: %ld\n", required);
    ok(!lstrcmpiA(buffer, destfile), "unexpected path: %s\n", buffer);

    SetupCloseInfFile(hinf);
    DeleteFileA(inf_filename);
}

START_TEST(query)
{
    get_directories();

    test_SetupGetInfInformation();
    test_SetupGetSourceFileLocation();
    test_SetupGetSourceInfo();
    test_SetupGetTargetPath();
}
