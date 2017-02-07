/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User Atom helper routines
 * FILE:             subsys/win32k/ntuser/useratom.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

RTL_ATOM FASTCALL
IntAddAtom(LPWSTR AtomName)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PTHREADINFO pti;
   RTL_ATOM Atom;

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->rpdesk == NULL)
   {
      SetLastNtError(Status);
      return (RTL_ATOM)0;
   }

   Status = RtlAddAtomToAtomTable(gAtomTable, AtomName, &Atom);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return (RTL_ATOM)0;
   }
   return Atom;
}

ULONG FASTCALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PTHREADINFO pti;
   ULONG Size = nSize;

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->rpdesk == NULL)
   {
      SetLastNtError(Status);
      return 0;
   }

   Status = RtlQueryAtomInAtomTable(gAtomTable, nAtom, NULL, NULL, lpBuffer, &Size);

   if (Size < nSize)
      *(lpBuffer + Size/sizeof(WCHAR)) = 0;
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }
   return Size;
}

RTL_ATOM FASTCALL
IntAddGlobalAtom(LPWSTR lpBuffer, BOOL PinAtom)
{
   RTL_ATOM Atom;
   NTSTATUS Status = STATUS_SUCCESS;

   Status = RtlAddAtomToAtomTable(gAtomTable, lpBuffer, &Atom);

   if (!NT_SUCCESS(Status))
   {
      ERR("Error init Global Atom.\n");
      return 0;
   }

   if ( Atom && PinAtom ) RtlPinAtomInAtomTable(gAtomTable, Atom);

   return Atom;
}

DWORD
APIENTRY
NtUserGetAtomName(
    ATOM nAtom,
    PUNICODE_STRING pBuffer)
{
   DWORD Ret;
   WCHAR Buffer[256];
   UNICODE_STRING CapturedName = {0};
   UserEnterShared();
   CapturedName.Buffer = (LPWSTR)&Buffer;
   CapturedName.MaximumLength = sizeof(Buffer);
   Ret = IntGetAtomName((RTL_ATOM)nAtom, CapturedName.Buffer, (ULONG)CapturedName.Length);
   _SEH2_TRY
   {
      RtlCopyMemory(pBuffer->Buffer, &Buffer, pBuffer->MaximumLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Ret = 0;
   }
   _SEH2_END
   UserLeave();
   return Ret;
}

/* EOF */
