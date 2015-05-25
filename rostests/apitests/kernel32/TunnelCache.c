/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for Tunnel Cache
 * PROGRAMMER:      Pierre Schweitzer <pierre.schweitzer@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/rtlfuncs.h>

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
}
