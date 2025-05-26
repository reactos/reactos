/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of longjmp for x86.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <setjmp.h>
#include <windef.h>

#define _JUMP_BUFFER_COOKIE 0x56433230

void __cdecl _global_unwind2(void* Registration);

typedef void (__stdcall *PUNWIND_ROUTINE)(const _JUMP_BUFFER *);

void _stdcall _seh_longjmp_unwind(const _JUMP_BUFFER* _Buf);

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
        _global_unwind2((void*)jumpBuffer->Registration);
    }

    /* Check if we have a registration */
    if (jumpBuffer->Registration != 0)
    {
        /* Check if this is a _setjmp3 buffer */
        if ((jumpBuffer->Cookie == _JUMP_BUFFER_COOKIE) &&
            (jumpBuffer->UnwindFunc != 0))
        {
            /* Call the unwind function if it exists */
            PUNWIND_ROUTINE unwindFunc = (PUNWIND_ROUTINE)jumpBuffer->UnwindFunc;
            unwindFunc(jumpBuffer);
        }
        else
        {//__debugbreak();
            _seh_longjmp_unwind(jumpBuffer);
        }
    }

    /* Nake sure longjmp never returns 0 */
    if (_Value == 0)
    {
        _Value = 1;
    }

    __longjmp_nounwind(jumpBuffer, _Value);
}
