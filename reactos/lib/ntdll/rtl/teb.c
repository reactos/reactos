/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/teb.c
 * PURPOSE:         
 */

#include <ddk/ntddk.h>
#include <napi/teb.h>


PTEB STDCALL
_NtCurrentTeb() { return NtCurrentTeb(); }


/*
 * @implemented
 */
VOID STDCALL
RtlAcquirePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebLockRoutine (Peb->FastPebLock);
}


/*
 * @implemented
 */
VOID STDCALL
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebUnlockRoutine (Peb->FastPebLock);
}

/* EOF */
