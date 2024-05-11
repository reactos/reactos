/*
 * Unit test suite for virtual substituted drive functions.
 *
 * Copyright 2011 Sam Arun Raj
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

#define SUBST_DRIVE_LETTER 'M'
#define SUBST_DRIVE "M:"
#define SUBST_DRIVE_NON_EXIST_DIR "M:\\deadbeef"
#define SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR "M:\\"
#define SUBST_DRIVE_SEARCH "M:\\*"
#define SUBST_DRIVE_LOWERCASE "m:"
#define SUBST_DRIVE_LOWERCASE_SEARCH "m:\\*"
#define SUBST_DRIVE2_LETTER 'N'
#define SUBST_DRIVE2 "N:"
#define SUBST_DRIVE2_WITH_TRAILING_PATH_SEPERATOR "N:\\"
#define SUBST_DRIVE2_SEARCH "N:\\*"

static void test_DefineDosDeviceA(void)
{
    CHAR Buffer[MAX_PATH], Target[MAX_PATH];
    BOOL Result;
    UINT CharCount;
    HANDLE hnd;
    WIN32_FIND_DATAA Data;
    UINT SystemDriveType, DriveType1, DriveType2;
    DWORD dwMaskPrev, dwMaskCur;
    CHAR c;

    /* Choose the symbolic link target */
    CharCount = GetSystemWindowsDirectoryA(Target, MAX_PATH);
    ok(CharCount > 0, "Failed to get windows directory\n");
    c = Target[3];
    Target[3] = '\0';
    SystemDriveType = GetDriveTypeA(Target);
    Target[3] = c;

    /* Test with a subst drive pointing to another substed drive */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, Target);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    Result = DefineDosDeviceA(0, SUBST_DRIVE2, SUBST_DRIVE);
    ok(Result, "Failed to subst drive\n");
    DriveType2 = GetDriveTypeA(SUBST_DRIVE2_WITH_TRAILING_PATH_SEPERATOR);
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE2_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE2_SEARCH, &Data);
    ok(hnd != INVALID_HANDLE_VALUE, "Failed to open subst drive\n");
    if (hnd) FindClose(hnd);
    ok(DriveType1 == DriveType2, "subst drive types don't match\n");
    ok(DriveType1 == SystemDriveType, "subst drive types don't match\n");
    ok(DriveType2 == SystemDriveType, "subst drive types don't match\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, Target);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");
    hnd = FindFirstFileA(SUBST_DRIVE2_SEARCH, &Data);
    ok(hnd == INVALID_HANDLE_VALUE, "Opened subst drive when it should fail, we removed the target\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE2, SUBST_DRIVE);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    Result = QueryDosDeviceA(SUBST_DRIVE2, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test using lowercase drive letter */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE_LOWERCASE, Target);
    ok(Result, "Failed to subst drive using lowercase drive letter\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 == SystemDriveType, "subst drive types don't match\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd != INVALID_HANDLE_VALUE, "Failed to open subst drive\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE_LOWERCASE, Target);
    ok(Result, "Failed to remove subst drive using lowercase drive letter\n");
    Result = QueryDosDeviceA(SUBST_DRIVE_LOWERCASE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test remove without using DDD_EXACT_MATCH_ON_REMOVE */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, Target);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 == SystemDriveType, "subst drive types don't match\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd != INVALID_HANDLE_VALUE, "Failed to open subst drive\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION, SUBST_DRIVE, NULL);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test multiple adds and multiple removes in add order */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_FILE_NOT_FOUND), GetLastError());
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp3") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp3") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test multiple adds and multiple removes in reverse order */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_FILE_NOT_FOUND), GetLastError());
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp2") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp1") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test multiple adds and multiple removes out of order */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp4");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, SUBST_DRIVE, "C:\\temp5");
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_FILE_NOT_FOUND), GetLastError());
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp5") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp5");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result) ok((_stricmp(Buffer, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, "C:\\temp4");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test with trailing '\' appended to TargetPath */
    dwMaskPrev = GetLogicalDrives();
    snprintf(Buffer, sizeof(Buffer), "%s\\\\\\", Target);
    Result = DefineDosDeviceA(0, SUBST_DRIVE, Buffer);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 == SystemDriveType, "subst drive types don't match\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd != INVALID_HANDLE_VALUE, "Failed to open subst drive\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, Buffer);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test with trailing '\' appended to TargetPath and DDD_RAW_TARGET_PATH flag */
    dwMaskPrev = GetLogicalDrives();
    snprintf(Buffer, sizeof(Buffer), "\\??\\%s\\\\\\", Target);
    Result = DefineDosDeviceA(DDD_RAW_TARGET_PATH, SUBST_DRIVE, Buffer);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when they shouldn't\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd == INVALID_HANDLE_VALUE, "Opened subst drive when it should fail\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_INVALID_NAME), GetLastError());
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION, SUBST_DRIVE, NULL);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test using trailing \ against drive letter */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR, Target);
    ok(!Result, "Subst drive using trailing path seperator, this should not happen\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");
    ok(!(dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is set when it shouldn't\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd == INVALID_HANDLE_VALUE, "Opened subst drive when it should fail\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR, Target);
    ok(!Result, "Removing Subst drive using trailing path seperator passed when it should fail\n");
    Result = QueryDosDeviceA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present when it should not be created in the first place\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test using arbitary string, not necessarily a DOS drive letter */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, "!QHello:", Target);
    ok(Result, "Failed to subst drive using non-DOS drive name\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, "!QHello:", Target);
    ok(Result, "Failed to subst drive using non-DOS drive name\n");
    Result = QueryDosDeviceA("!QHello:", Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test by subst a drive to itself */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, SUBST_DRIVE);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_FILE_NOT_FOUND), GetLastError());
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd == INVALID_HANDLE_VALUE, "Opened subst drive when it should fail\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, SUBST_DRIVE);
    ok(Result, "Failed to remove subst drive using lowercase drive letter\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");

    /* Test by subst a drive to an non-existent folder under itself */
    dwMaskPrev = GetLogicalDrives();
    Result = DefineDosDeviceA(0, SUBST_DRIVE, SUBST_DRIVE_NON_EXIST_DIR);
    ok(Result, "Failed to subst drive\n");
    DriveType1 = GetDriveTypeA(SUBST_DRIVE_WITH_TRAILING_PATH_SEPERATOR);
    ok(DriveType1 != SystemDriveType, "subst drive types match when it shouldn't\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_FILE_NOT_FOUND), GetLastError());
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur != dwMaskPrev, "Drive masks match when it shouldn't\n");
    ok((dwMaskCur & (1 << (SUBST_DRIVE_LETTER - 'A'))), "Drive bit is not set\n");
    hnd = FindFirstFileA(SUBST_DRIVE_SEARCH, &Data);
    ok(hnd == INVALID_HANDLE_VALUE, "Opened subst drive when it should fail\n");
    if (hnd) FindClose(hnd);
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, SUBST_DRIVE_NON_EXIST_DIR);
    ok(Result, "Failed to remove subst drive using lowercase drive letter\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
    dwMaskCur = GetLogicalDrives();
    ok(dwMaskCur == dwMaskPrev, "Drive masks don't match\n");
}

static void test_QueryDosDeviceA(void)
{
    CHAR Buffer[MAX_PATH], Target[MAX_PATH];
    BOOL Result;
    UINT CharCount;

    /* Choose the symbolic link target */
    CharCount = GetSystemWindowsDirectoryA(Target, MAX_PATH);
    ok(CharCount > 0, "Failed to get windows directory\n");

    Result = DefineDosDeviceA(0, SUBST_DRIVE, Target);
    ok(Result, "Failed to subst drive\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, 0);
    ok(!Result, "Should fail as the buffer passed is supposed to be small\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_INSUFFICIENT_BUFFER), GetLastError());
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);
    ok(Result, "failed to get target path\n");
    ok(_strnicmp(Buffer, "\\??\\", 4) == 0, "The target returned does have correct prefix set\n");
    ok(_stricmp(&Buffer[4], Target) == 0, "The target returned does not match the one set\n");
    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, SUBST_DRIVE, Target);
    ok(Result, "Failed to remove subst drive using lowercase drive letter\n");
    Result = QueryDosDeviceA(SUBST_DRIVE, Buffer, MAX_PATH);

    /* This will try to retrieve all existing MS-DOS device names */
    Result = QueryDosDeviceA(NULL, Buffer, 0);
    ok(!Result, "Should fail as the buffer passed is supposed to be small\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(ERROR_INSUFFICIENT_BUFFER), GetLastError());
}

START_TEST(dosdev)
{
    test_DefineDosDeviceA();
    test_QueryDosDeviceA();
}
