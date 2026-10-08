/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtSetLdtEntries
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include "precomp.h"

START_TEST(NtSetLdtEntries)
{
#ifdef _M_AMD64
    NTSTATUS Status;
    LDT_ENTRY Zero, Data;

    RtlZeroMemory(&Zero, sizeof(Zero));

    /* Base 0, 4 KB read/write data segment, DPL 3, present */
    RtlZeroMemory(&Data, sizeof(Data));
    Data.LimitLow = 0xFFF;
    Data.HighWord.Bits.Type = 0x12;
    Data.HighWord.Bits.Dpl = 3;
    Data.HighWord.Bits.Pres = 1;
    Data.HighWord.Bits.Default_Big = 1;

    Status = NtSetLdtEntries(0, Zero, 0, Zero);
    ok(Status == STATUS_NOT_IMPLEMENTED, "Status = 0x%lx\n", Status);

    Status = NtSetLdtEntries(0x0F, Data, 0, Zero);
    ok(Status == STATUS_NOT_IMPLEMENTED, "Status = 0x%lx\n", Status);

    Status = NtSetLdtEntries(0x0F, Data, 0x17, Data);
    ok(Status == STATUS_NOT_IMPLEMENTED, "Status = 0x%lx\n", Status);

    Status = NtSetLdtEntries(0x0F, Zero, 0, Zero);
    ok(Status == STATUS_NOT_IMPLEMENTED, "Status = 0x%lx\n", Status);

    Status = NtSetLdtEntries(0xFFFFFFFF, Data, 0xFFFFFFFF, Data);
    ok(Status == STATUS_NOT_IMPLEMENTED, "Status = 0x%lx\n", Status);
#else
    skip("NtSetLdtEntries is only tested in amd64 builds\n");
#endif
}
