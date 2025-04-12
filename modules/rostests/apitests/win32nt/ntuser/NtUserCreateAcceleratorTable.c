/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtUserCreateAcceleratorTable
 * COPYRIGHT:   Copyright 2025 Max Korostil (mrmks04@yandex.ru)
 */

#include "../win32nt.h"


START_TEST(NtUserCreateAcceleratorTable)
{
    HACCEL hAccel;
    ACCEL Entries[5];
    ULONG EntriesCount = 0x80000005;

    hAccel = NtUserCreateAcceleratorTable(Entries, EntriesCount);
    ok_hdl(hAccel, NULL);

    hAccel = NtUserCreateAcceleratorTable(NULL, EntriesCount);
    ok_hdl(hAccel, NULL);

    hAccel = NtUserCreateAcceleratorTable(Entries, 0);
    ok_hdl(hAccel, NULL);

    EntriesCount = ARRAYSIZE(Entries);
    hAccel = NtUserCreateAcceleratorTable((LPACCEL)(ULONG_PTR)0xC0FEC0FE, EntriesCount);
    ok_hdl(hAccel, NULL);

    hAccel = NtUserCreateAcceleratorTable(Entries, EntriesCount);
    ok(hAccel != NULL, "hAccel is NULL\n");
}
