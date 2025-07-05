/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of longjmp for x86.
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

#define _JUMP_BUFFER_COOKIE 0x56433230

typedef void (__stdcall *PUNWIND_ROUTINE)(const _JUMP_BUFFER *);

void _local_unwind2(void* Registration);

__declspec(noreturn)
void __longjmp_nounwind(const _JUMP_BUFFER* _Buf, int _Value);

__declspec(noreturn)
void __cdecl longjmp(
    _In_reads_(_JBLEN) jmp_buf _Buf,
    _In_ int _Value)
{
    const _JUMP_BUFFER* jumpBuffer = (_JUMP_BUFFER*)_Buf;

    /* Check if the exception registration is from a previous function */
    if (jumpBuffer->Registration != __readfsdword(0))
    {
        /* Do a global unwind to the function that owns the target SEH frame */
        EXCEPTION_RECORD exceptionRecord = { 0 };
        exceptionRecord.ExceptionCode = STATUS_LONGJUMP;
        RtlUnwind((PVOID)jumpBuffer->Registration,
                  NULL,
                  &exceptionRecord,
                  (PVOID)(ULONG_PTR)_Value);
    }

    /* Check if this is a _setjmp3 buffer */
    if ((jumpBuffer->Cookie == _JUMP_BUFFER_COOKIE) &&
        (jumpBuffer->UnwindFunc != 0))
    {
        /* Call the unwind function if it exists */
        PUNWIND_ROUTINE unwindFunc = (PUNWIND_ROUTINE)jumpBuffer->UnwindFunc;
        unwindFunc(jumpBuffer);
    }
    else
    {
        _local_unwind2((void*)jumpBuffer->Registration);
    }

    if (_Value == 0)
    {
        /* setjmp returns 1 on longjmp with value 0 */
        _Value = 1;
    }

    __longjmp_nounwind(jumpBuffer, _Value);
}
