/*
 * PROJECT:         ReactOS Tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            tunneltest.c
 * PURPOSE:         Usermode tunnel cache testing
 * PROGRAMMERS:     Pierre Schweitzer (pierre@reactos.org)
 * NOTES:           Based on indications from: http://support.microsoft.com/?kbid=172190
 */

#include <stdio.h>
#include <windef.h>
#include <winternl.h>
#include <winbase.h>
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }

const UNICODE_STRING FatName = RTL_CONSTANT_STRING(L"FAT");
const UNICODE_STRING Fat32Name = RTL_CONSTANT_STRING(L"FAT32");
const UNICODE_STRING NtfsName = RTL_CONSTANT_STRING(L"NTFS");

int wmain(int argc, WCHAR * argv[])
{
    WCHAR TempPath[MAX_PATH + 1];
    WCHAR CopyPath[MAX_PATH + 1];
    WCHAR RootPath[] = {'A', ':', '\\', '\0'};
    WCHAR FileSystemName[sizeof("FAT32")]; /* Max we should hold - fail otherwise, we don't care */
    UNICODE_STRING FSName;
    WCHAR File1[] = {'\\', 'f', 'i', 'l', 'e', '1', '\0'};
    WCHAR File2[] = {'\\', 'f', 'i', 'l', 'e', '2', '\0'};
    ULONG FilePos;
    HANDLE hFile;
    FILETIME FileTime, File1Time;

    /* Get temp path in which will work */
    if (GetTempPathW(sizeof(TempPath) / sizeof(TempPath[0]), TempPath) == 0)
    {
        fprintf(stderr, "Failed to get temp path\n");
        return GetLastError();
    }

    /* Assume it's X:\ something */
    RootPath[0] = TempPath[0];

    /* Get information about the volume */
    if (GetVolumeInformationW(RootPath, NULL, 0, NULL, NULL, NULL, FileSystemName, sizeof(FileSystemName) / sizeof(FileSystemName[0])) == 0)
    {
        fprintf(stderr, "Failed to get volume info\n");
        return GetLastError();
    }

    /* Convert to string */
    RtlInitUnicodeString(&FSName, FileSystemName);

    /* Bail out if that's not FAT or NTFS */
    if (RtlCompareUnicodeString(&FSName, &FatName, FALSE) != 0 &&
        RtlCompareUnicodeString(&FSName, &Fat32Name, FALSE) != 0 &&
        RtlCompareUnicodeString(&FSName, &NtfsName, FALSE) != 0)
    {
        fprintf(stderr, "!(FAT, FAT32, NTFS): \'%S\'\n", FSName.Buffer);
        return 0;
    }

    /* Ensure we can store complete path - no overrun */
    FilePos = wcslen(TempPath);
    if (FilePos > MAX_PATH - sizeof(File2) / sizeof(WCHAR))
    {
        fprintf(stderr, "Files won't fit\n");
        return 0;
    }

    /* Create first file */
    wcscat(TempPath, File1);
    hFile = CreateFileW(TempPath, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to create file1\n");
        return GetLastError();
    }

    /* Get its creation timestamp. It will be our reference */
    /* Get it in FileTime because file1 will renamed to file */
    if (GetFileTime(hFile, &FileTime, NULL, NULL) == FALSE)
    {
        fprintf(stderr, "Failed to read creation time\n");
        CloseHandle(hFile);
        return GetLastError();
    }

    CloseHandle(hFile);

    /* Wait a least 10ms (resolution of FAT) */
    /* XXX: Increased to 1s for ReactOS... */
    Sleep(1000);

    /* Create second file */
    /* Remove old file from buffer */
    TempPath[FilePos - 1] = 0;
    wcscat(TempPath, File2);
    hFile = CreateFileW(TempPath, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to create file2\n");
        return GetLastError();
    }
    CloseHandle(hFile);

    /* Rename file1 to file */
    TempPath[FilePos] = 0;
    wcscat(TempPath, File1);
    wcscpy(CopyPath, TempPath);
    /* Remove number for dest */
    CopyPath[wcslen(TempPath) - 1] = 0;
    if (MoveFileW(TempPath, CopyPath) == 0)
    {
        fprintf(stderr, "Failed to rename file1\n");
        return GetLastError();
    }

    /* Rename file2 to file1 */
    wcscpy(CopyPath, TempPath);
    /* Change 1 to 2 */
    CopyPath[wcslen(TempPath) - 1] = L'2';
    if (MoveFileW(CopyPath, TempPath) == 0)
    {
        fprintf(stderr, "Failed to rename file2\n");
        return GetLastError();
    }

    /* Open file1 and get its creation time */
    hFile = CreateFileW(TempPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open file1\n");
        return GetLastError();
    }
    if (GetFileTime(hFile, &File1Time, NULL, NULL) == FALSE)
    {
        fprintf(stderr, "Failed to read creation time\n");
        CloseHandle(hFile);
        return GetLastError();
    }
    CloseHandle(hFile);

    /* Delete files */
    CopyPath[wcslen(TempPath) - 1] = 0;
    DeleteFileW(TempPath);
    DeleteFileW(CopyPath);

    /* Compare both, they have to be strictly identical */
    if (RtlCompareMemory(&FileTime, &File1Time, sizeof(FILETIME)) == sizeof(FILETIME))
    {
        fprintf(stdout, "Tunnel cache in action\n");
        return 0;
    }

    fprintf(stdout, "Tunnel cache NOT in action\n");
    return 0;
}
