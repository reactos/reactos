/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for imm32 ImmLockClientImc/ImmUnlockClientImc
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <imm.h>
#include <ddk/imm.h>
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include "../../../win32ss/include/ntuser.h"
#include <imm32_undoc.h>
#include <ndk/rtlfuncs.h>
#include <wine/test.h>
#include <stdio.h>

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
    CLIENTIMC *pClientImc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));

    pClientImc->hImc = (HIMC)ImmCreateIMCC(4);
    pClientImc->cLockObj = 2;
    pClientImc->dwFlags = 0x40;
    RtlInitializeCriticalSection(&pClientImc->cs);
    ok_long(ImmGetIMCCSize(pClientImc->hImc), 4);

    ImmUnlockClientImc(pClientImc);
    ok_long(pClientImc->cLockObj, 1);
    ok_long(ImmGetIMCCSize(pClientImc->hImc), 4);

    _SEH2_TRY
    {
        ImmUnlockClientImc(pClientImc);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ;
    }
    _SEH2_END;

    ok_long(pClientImc->cLockObj, 0);
    ok_long(ImmGetIMCCSize(pClientImc->hImc), 0);
}
