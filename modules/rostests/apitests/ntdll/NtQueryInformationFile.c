/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQueryInformationFile
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

#define ntv6(x) (LOBYTE(LOWORD(GetVersion())) >= 6 ? (x) : 0)

START_TEST(NtQueryInformationFile)
{
    NTSTATUS Status;

    Status = NtQueryInformationFile(NULL, NULL, NULL, 0, 0);
    ok(Status == STATUS_INVALID_INFO_CLASS ||
       ntv6(Status == STATUS_NOT_IMPLEMENTED), "Status = %lx\n", Status);

    Status = NtQueryInformationFile(NULL, NULL, NULL, 0, 0x80000000);
    ok(Status == STATUS_INVALID_INFO_CLASS ||
       ntv6(Status == STATUS_NOT_IMPLEMENTED), "Status = %lx\n", Status);

    /* Get the full path of the current executable */
    CHAR Path[MAX_PATH];
    DWORD Length = GetModuleFileNameA(NULL, Path, _countof(Path));
    ok(Length != 0, "GetModuleFileNameA failed\n");
    if (Length == 0)
        return;

    /* Open the file */
    HANDLE hFile = CreateFileA(Path,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed\n");
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    /* Query FileEndOfFileInformation */
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
    EndOfFileInformation.EndOfFile.QuadPart = 0xdeaddead;
    Status = NtQueryInformationFile(hFile,
                                    NULL,
                                    &EndOfFileInformation,
                                    sizeof(EndOfFileInformation),
                                    FileEndOfFileInformation);
    ok_hex(Status, STATUS_INVALID_INFO_CLASS);
    ok(EndOfFileInformation.EndOfFile.QuadPart == 0xdeaddead, "EndOfFile is modified\n");

    CloseHandle(hFile);
}
