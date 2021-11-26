/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for Tunnel Cache
 * PROGRAMMER:      Pierre Schweitzer <pierre.schweitzer@reactos.org>
 */

#include "precomp.h"

static
void
Test_VeryLongTests(void)
{
    UCHAR i = 0;
    HANDLE hFile;
    CHAR TestDir[] = "XTestDirTunnelCache";
    CHAR OldDir[MAX_PATH];
    FILETIME FileTime, File1Time;

    win_skip("Too long, see: ROSTESTS-177\n");
    return;

    /* Create a blank test directory */
    if (GetCurrentDirectory(MAX_PATH, OldDir) == 0)
    {
        win_skip("No test directory available\n");
        return;
    }

    /* Create a blank test directory */
    for (; i < 10; ++i)
    {
        TestDir[0] = '0' + i;
        if (CreateDirectory(TestDir, NULL))
        {
            if (SetCurrentDirectory(TestDir) == 0)
            {
                RemoveDirectory(TestDir);
                win_skip("No test directory available\n");
                return;
            }

            break;
        }
    }

    if (i == 10)
    {
        win_skip("No test directory available\n");
        return;
    }

    hFile = CreateFile("file1",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    ok(GetFileTime(hFile, &FileTime, NULL, NULL) != FALSE, "GetFileTime() failed\n");
    CloseHandle(hFile);

    /* Wait a least 10ms (resolution of FAT) */
    /* XXX: Increased to 1s for ReactOS... */
    Sleep(1000);

    hFile = CreateFile("file2",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    CloseHandle(hFile);

    ok(MoveFile("file1", "file") != FALSE, "MoveFile() failed\n");
    /* Sleep over cache expiry */
    /* FIXME: Query correct value from registry if it exists:
     * \HKLM\System\CurrentControlSet\Control\FileSystem\MaximumTunnelEntryAgeInSeconds */
    Sleep(16000);
    ok(MoveFile("file2", "file1") != FALSE, "MoveFile() failed\n");

    hFile = CreateFile("file1",
                       GENERIC_READ,
                       0, NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    ok(GetFileTime(hFile, &File1Time, NULL, NULL) != FALSE, "GetFileTime() failed\n");
    CloseHandle(hFile);

    ok(RtlCompareMemory(&FileTime, &File1Time, sizeof(FILETIME)) != sizeof(FILETIME), "Tunnel cache still in action?\n");

    DeleteFile("file2");
    DeleteFile("file1");
    DeleteFile("file");

    SetCurrentDirectory(OldDir);
    RemoveDirectory(TestDir);
}

static
void
Test_LongTests(void)
{
    UCHAR i = 0;
    HANDLE hFile;
    CHAR TestDir[] = "XTestDirTunnelCache";
    CHAR OldDir[MAX_PATH];
    FILETIME FileTime, File1Time;

    /* Create a blank test directory */
    if (GetCurrentDirectory(MAX_PATH, OldDir) == 0)
    {
        win_skip("No test directory available\n");
        return;
    }

    /* Create a blank test directory */
    for (; i < 10; ++i)
    {
        TestDir[0] = '0' + i;
        if (CreateDirectory(TestDir, NULL))
        {
            if (SetCurrentDirectory(TestDir) == 0)
            {
                RemoveDirectory(TestDir);
                win_skip("No test directory available\n");
                return;
            }

            break;
        }
    }

    if (i == 10)
    {
        win_skip("No test directory available\n");
        return;
    }

    hFile = CreateFile("file1",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    ok(GetFileTime(hFile, &FileTime, NULL, NULL) != FALSE, "GetFileTime() failed\n");
    CloseHandle(hFile);

    /* Wait a least 10ms (resolution of FAT) */
    /* XXX: Increased to 1s for ReactOS... */
    Sleep(1000);

    hFile = CreateFile("file2",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    CloseHandle(hFile);

    ok(MoveFile("file1", "file") != FALSE, "MoveFile() failed\n");
    ok(MoveFile("file2", "file1") != FALSE, "MoveFile() failed\n");

    hFile = CreateFile("file1",
                       GENERIC_READ,
                       0, NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    ok(GetFileTime(hFile, &File1Time, NULL, NULL) != FALSE, "GetFileTime() failed\n");
    CloseHandle(hFile);

    ros_skip_flaky
    ok(RtlCompareMemory(&FileTime, &File1Time, sizeof(FILETIME)) == sizeof(FILETIME), "Tunnel cache failed\n");

    DeleteFile("file2");
    DeleteFile("file1");
    DeleteFile("file");

    SetCurrentDirectory(OldDir);
    RemoveDirectory(TestDir);
}

static
void
Test_ShortTests(void)
{
    UCHAR i = 0;
    CHAR ShortName[14];
    HANDLE hFile, hFind;
    WIN32_FIND_DATA FileInfo;
    CHAR TestDir[] = "XTestDirTunnelCache";
    CHAR OldDir[MAX_PATH];

    /* Create a blank test directory */
    if (GetCurrentDirectory(MAX_PATH, OldDir) == 0)
    {
        win_skip("No test directory available\n");
        return;
    }

    /* Create a blank test directory */
    for (; i < 10; ++i)
    {
        TestDir[0] = '0' + i;
        if (CreateDirectory(TestDir, NULL))
        {
            if (SetCurrentDirectory(TestDir) == 0)
            {
                RemoveDirectory(TestDir);
                win_skip("No test directory available\n");
                return;
            }

            break;
        }
    }

    if (i == 10)
    {
        win_skip("No test directory available\n");
        return;
    }

    hFile = CreateFile("verylongfilename",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    CloseHandle(hFile);

    hFind = FindFirstFile("verylongfilename", &FileInfo);
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstFile() failed\n");
    FindClose(hFind);
    RtlCopyMemory(ShortName, FileInfo.cAlternateFileName, sizeof(ShortName));

    ok(MoveFile("verylongfilename", "verylongfilename2") != FALSE, "MoveFile() failed\n");
    hFind = FindFirstFile("verylongfilename2", &FileInfo);
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstFile() failed\n");
    FindClose(hFind);
    ok(strcmp(FileInfo.cAlternateFileName, ShortName) == 0, "strcmp() failed\n");

    hFile = CreateFile("randomfilename",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile() failed\n");
    CloseHandle(hFile);

    ok(MoveFileEx("randomfilename", "verylongfilename2", MOVEFILE_REPLACE_EXISTING) != FALSE, "MoveFile() failed\n");
    hFind = FindFirstFile("verylongfilename2", &FileInfo);
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstFile() failed\n");
    FindClose(hFind);
    ok(strcmp(FileInfo.cAlternateFileName, ShortName) == 0, "strcmp() failed\n");

    DeleteFile("randomfilename");
    DeleteFile("verylongfilename");
    DeleteFile("verylongfilename2");

    SetCurrentDirectory(OldDir);
    RemoveDirectory(TestDir);
}

START_TEST(TunnelCache)
{
    Test_ShortTests();
    Test_LongTests();
    Test_VeryLongTests();
}
