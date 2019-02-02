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
}
