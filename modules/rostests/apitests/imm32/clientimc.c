/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for imm32 ImmLockClientImc/ImmUnlockClientImc
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#if 0
static void DumpBinary(LPCVOID pv, size_t cb)
{
    const BYTE *pb = pv;
    while (cb--)
    {
        printf("%02X ", (BYTE)*pb++);
    }
    printf("\n");
}
#endif

START_TEST(clientimc)
{
    DWORD dwCode;
    CLIENTIMC *pClientImc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));

    pClientImc->hInputContext = (HANDLE)ImmCreateIMCC(4);
    pClientImc->cLockObj = 2;
    pClientImc->dwFlags = 0x40;
    RtlInitializeCriticalSection(&pClientImc->cs);
    ok_long(ImmGetIMCCSize((HIMCC)pClientImc->hInputContext), 4);

    ImmUnlockClientImc(pClientImc);
    ok_long(pClientImc->cLockObj, 1);
    ok_long(ImmGetIMCCSize((HIMCC)pClientImc->hInputContext), 4);

    dwCode = 0;
    _SEH2_TRY
    {
        ImmUnlockClientImc(pClientImc);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwCode = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_long(dwCode, STATUS_ACCESS_VIOLATION);

    ok_long(pClientImc->cLockObj, 0);
    ok_long(ImmGetIMCCSize((HIMCC)pClientImc->hInputContext), 0);

    HeapFree(GetProcessHeap(), 0, pClientImc);
}
