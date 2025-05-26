/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of longjmp for ARM64
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <setjmp.h>
#define RtlUnwind RtlUnwind_imported
#include <windef.h>

#undef RtlUnwind
VOID
NTAPI
RtlUnwind(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue
    );

__declspec(noreturn)
void __longjmp_noframe(const _JUMP_BUFFER* _Buf, int _Value);

__declspec(noreturn)
void __cdecl longjmp(
    _In_reads_(_JBLEN) jmp_buf _Buf,
    _In_ int _Value)
{
    const _JUMP_BUFFER* jumpBuffer = (_JUMP_BUFFER*)_Buf;

    /* Ensure _Value is non-zero */
    _Value = (_Value == 0) ? 1 : _Value;

    EXCEPTION_RECORD exceptionRecord = { 0 };
    exceptionRecord.ExceptionCode = STATUS_LONGJUMP;
    exceptionRecord.NumberParameters = 1;
    exceptionRecord.ExceptionInformation[0] = (ULONG_PTR)jumpBuffer;

    RtlUnwind((PVOID)jumpBuffer->Frame,
                (PVOID)jumpBuffer->Lr,
                &exceptionRecord,
                (PVOID)(ULONG_PTR)_Value);
}
