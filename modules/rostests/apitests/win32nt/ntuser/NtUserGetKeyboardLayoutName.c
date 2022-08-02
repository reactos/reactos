/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for NtUserGetKeyboardLayoutName
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32nt.h>
#include <pseh/pseh2.h>

typedef BOOL (APIENTRY *FN_NtUserGetKeyboardLayoutName)(PVOID);

START_TEST(NtUserGetKeyboardLayoutName)
{
    FN_NtUserGetKeyboardLayoutName fn = (FN_NtUserGetKeyboardLayoutName)NtUserGetKeyboardLayoutName;
    UNICODE_STRING ustr;
    WCHAR szBuff[MAX_PATH];
    BOOL bHung, ret;

    /* Try NULL */
    ok_int(fn(NULL), FALSE);

    /* Try szBuff */
    ret = bHung = FALSE;
    RtlZeroMemory(szBuff, sizeof(szBuff));
    _SEH2_TRY
    {
        ret = fn(szBuff);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    //ok_int(ret, FALSE); // XP:TRUE, 2k3:FALSE
    ok(szBuff[0] == 0, "szBuff[0] was %d\n", szBuff[0]);

    /* Try ustr */
    RtlZeroMemory(szBuff, sizeof(szBuff));
    ustr.Buffer = szBuff;
    ustr.Length = 0;
    ustr.MaximumLength = RTL_NUMBER_OF(szBuff) * sizeof(WCHAR);
    ret = bHung = FALSE;
    _SEH2_TRY
    {
        ret = fn(&ustr);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bHung = TRUE;
    }
    _SEH2_END;

    ok_int(bHung, FALSE);
    ok_int(ret, TRUE);
    ok(szBuff[0] != 0, "szBuff[0] was %d\n", szBuff[0]);
    trace("szBuff: %S\n", szBuff);
}
