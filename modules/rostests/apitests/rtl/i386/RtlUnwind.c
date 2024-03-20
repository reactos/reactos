/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for x86 RtlUnwind
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtltests.h>

CONTEXT g_InContext;
CONTEXT g_OutContext;

VOID
WINAPI
RtlUnwindWrapper(
    _In_ PVOID TargetFrame,
    _In_ PVOID TargetIp,
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue);

START_TEST(RtlUnwind)
{
    BOOL IsWow64;

    RtlZeroMemory(&g_InContext, sizeof(g_InContext));
    RtlZeroMemory(&g_OutContext, sizeof(g_OutContext));

    PEXCEPTION_REGISTRATION_RECORD ExcptReg = (PEXCEPTION_REGISTRATION_RECORD)__readfsdword(0);
    ok(ExcptReg != NULL, "ExcpReg is NULL\n");

    g_InContext.Eax = 0xabcd0001;
    g_InContext.Ebx = 0xabcd0002;
    g_InContext.Ecx = 0xabcd0003;
    g_InContext.Edx = 0xabcd0004;
    g_InContext.Esi = 0xabcd0005;
    g_InContext.Edi = 0xabcd0006;
    RtlUnwindWrapper(ExcptReg, NULL, NULL, (PVOID)0x12345678);
    ok_eq_hex(g_OutContext.Eax, 0x12345678ul);
    ok_eq_hex(g_OutContext.Ebx, 0ul);
    ok_eq_hex(g_OutContext.Ecx, 0ul);
    ok_eq_hex(g_OutContext.Edx, 0ul);
    ok_eq_hex(g_OutContext.Esi, 0ul);
    ok_eq_hex(g_OutContext.Edi, 0ul);
    if (IsWow64Process(NtCurrentProcess(), &IsWow64) && IsWow64)
    {
        ok_eq_hex(g_OutContext.SegCs, 0x23ul);
        ok_eq_hex(g_OutContext.SegDs, 0x2bul);
        ok_eq_hex(g_OutContext.SegEs, 0x2bul);
        ok_eq_hex(g_OutContext.SegFs, 0x53ul);
    }
    else
    {
        ok_eq_hex(g_OutContext.SegCs, 0x1bul);
        ok_eq_hex(g_OutContext.SegDs, 0x23ul);
        ok_eq_hex(g_OutContext.SegEs, 0x23ul);
        ok_eq_hex(g_OutContext.SegFs, 0x3bul);
    }
}
