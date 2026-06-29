/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/compat/amd64/msys2.S
 * PURPOSE:         Hack for msys-2
 *
 * PROGRAMMER:      Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

EXTERN RtlGetCurrentDirectory_U:PROC
EXTERN FastPebLock:QWORD
EXTERN RtlEnterCriticalSection:PROC
EXTERN RtlpCurDirRef:QWORD

.code

// For the logic see https://github.com/git-for-windows/msys2-runtime/blob/bb4e8e05b4c68895887eba9c420e8a42e0e2801e/winsup/cygwin/path.cc#L4854C1-L4854C23

MsysDecoy_UseRtlpCurDirRef:
    // This path should never be executed!
    int HEX(2c)

    // msys-2 searches for this instruction squence to extract RtlpCurDirRef
    lea rcx, FastPebLock[rip]
    call RtlEnterCriticalSection
    mov rbx, qword ptr RtlpCurDirRef[rip]
    test rbx, rbx

MsysDecoy_continue:
    jmp RtlGetCurrentDirectory_U

PUBLIC RtlGetCurrentDirectory_U_RtlpMsysDecoy
RtlGetCurrentDirectory_U_RtlpMsysDecoy:

    // Short jmp that hopefully does not contain a 0xe8
    jmp short MsysDecoy_continue

    // This emulates the call that msys is looking for
    // and will never be executed
    call MsysDecoy_UseRtlpCurDirRef

END
