/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiFlushUserBatch
 * PROGRAMMERS:
 */

#include <win32nt.h>

NTSTATUS
(NTAPI
*pNtGdiFlushUserBatch)(VOID);

START_TEST(NtGdiFlushUserBatch)
{
    PVOID pRet;
    PTEB pTeb;

    pNtGdiFlushUserBatch = (PVOID)GetProcAddress(g_hModule, "NtGdiFlushUserBatch");
    if (pNtGdiFlushUserBatch == NULL)
        return APISTATUS_NOT_FOUND;

    pTeb = NtCurrentTeb();
    ok(pTeb != NULL, "pTeb was NULL.\n");

    pRet = (PVOID)pNtGdiFlushUserBatch();

    ok(pRet != NULL, "pRet was NULL.\n");
    ok_ptr(pRet, &pTeb->RealClientId);

    ok_long(pTeb->GdiBatchCount, 0);
    ok_long(pTeb->GdiTebBatch.Offset, 0);
    ok_ptr(pTeb->GdiTebBatch.HDC, NULL);

    /* Set up some bullshit */
    pTeb->InDbgPrint = 1;
    pTeb->GdiBatchCount = 12;
    pTeb->GdiTebBatch.Offset = 21;
    pTeb->GdiTebBatch.HDC = (HDC)123;

    pRet = (PVOID)pNtGdiFlushUserBatch();
    ok_ptr(pRet, &pTeb->RealClientId);

    ok_int(pTeb->InDbgPrint, 0);
    ok_long(pTeb->GdiBatchCount, 12);
    ok_long(pTeb->GdiTebBatch.Offset, 0);
    ok_ptr(pTeb->GdiTebBatch.HDC, NULL);
}
