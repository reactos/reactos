/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtUserCreateAcceleratorTable
 * COPYRIGHT:   Copyright 2025 Max Korostil (mrmks04@yandex.ru)
 */

#include "../win32nt.h"
#include <pseh/pseh2.h>

#define MIN_VALID_NUMBER 1
#define MAX_VALID_NUMBER 32767

START_TEST(NtUserCreateAcceleratorTable)
{
    HACCEL hAccel = NULL;
    ACCEL Entries[5] = {0};
    ULONG EntriesCount = 0x80000005;
    BOOL bHung = FALSE;
    LPACCEL pEntries = NULL;

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

    ok_int(GetLastError(), ERROR_INVALID_PARAMETER);
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

    ok_int(GetLastError(), ERROR_INVALID_PARAMETER);
    ok_int(bHung, FALSE);
    ok_hdl(hAccel, NULL);

    /* Try EntriesCount = 0 */
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(Entries, MIN_VALID_NUMBER - 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(GetLastError(), ERROR_INVALID_PARAMETER);
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

    ok_int(GetLastError(), ERROR_NOACCESS);
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

    ok_int(GetLastError(), ERROR_SUCCESS);
    ok_int(bHung, FALSE);
    ok(hAccel != NULL, "hAccel is NULL\n");

    if (!bHung && hAccel != NULL)
        DestroyAcceleratorTable(hAccel);

    /* Try minimum */
    bHung = FALSE;
    _SEH2_TRY
    {
        hAccel = NtUserCreateAcceleratorTable(Entries, MIN_VALID_NUMBER);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(GetLastError(), ERROR_SUCCESS);
    ok_int(bHung, FALSE);
    ok(hAccel != NULL, "hAccel is NULL\n");

    if (!bHung && hAccel != NULL)
        DestroyAcceleratorTable(hAccel);

    /* Try maximum */
    bHung = FALSE;
    pEntries = HeapAlloc(GetProcessHeap(), 0, MAX_VALID_NUMBER * sizeof(ACCEL));
    ok(pEntries != NULL, "pEntries is NULL\n");
    if (pEntries != NULL)
    {
        _SEH2_TRY
        {
            hAccel = NtUserCreateAcceleratorTable(Entries, MAX_VALID_NUMBER);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            bHung = TRUE;
        }
        _SEH2_END;

        ok_int(GetLastError(), ERROR_SUCCESS);
        ok_int(bHung, FALSE);
        ok(hAccel != NULL, "hAccel is NULL\n");

        HeapFree(GetProcessHeap(), 0, pEntries);
        pEntries = NULL;

        if (!bHung && hAccel != NULL)
            DestroyAcceleratorTable(hAccel);
    }
}
