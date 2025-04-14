/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtUserCreateAcceleratorTable
 * COPYRIGHT:   Copyright 2025 Max Korostil (mrmks04@yandex.ru)
 */

#include "../win32nt.h"
#include <pseh/pseh2.h>


START_TEST(NtUserCreateAcceleratorTable)
{
    HACCEL hAccel;
    ACCEL Entries[5];
    ULONG EntriesCount = 0x80000005;
    BOOL bHung = FALSE;

    /* Try heap overflow */
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(Entries, EntriesCount);        
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok_hdl(hAccel, NULL);

    /* Try NULL Entries argument */
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(NULL, EntriesCount);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok_hdl(hAccel, NULL);

    /* Try EntriesCount = 0 */
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(Entries, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok_hdl(hAccel, NULL);

    /* Try wrong Entries argument pointer */
    EntriesCount = ARRAYSIZE(Entries);
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable((LPACCEL)(ULONG_PTR)0xC0FEC0FE, EntriesCount);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok_hdl(hAccel, NULL);

    /* Try correct parameters */
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(Entries, EntriesCount);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok(hAccel != NULL, "hAccel is NULL\n");
    if (!bHung && hAccel != NULL);
        DestroyAcceleratorTable(hAccel);
}
