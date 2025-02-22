/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtSetInformationFile
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

START_TEST(NtSetInformationFile)
{
    NTSTATUS Status;

    Status = NtSetInformationFile(NULL, NULL, NULL, 0, 0);
    ok(Status == STATUS_INVALID_INFO_CLASS, "Status = %lx\n", Status);

    Status = NtSetInformationFile(NULL, NULL, NULL, 0, 0x80000000);
    ok(Status == STATUS_INVALID_INFO_CLASS, "Status = %lx\n", Status);
}
