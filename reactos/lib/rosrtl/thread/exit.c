/* $Id: exit.c,v 1.3 2004/10/03 21:03:06 gvg Exp $
*/
/*
*/

#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

#include <rosrtl/thread.h>

static VOID NTAPI RtlRosExitUserThread_Stage2
(
 IN ULONG_PTR Status
)
{
 RtlRosFreeUserThreadStack(NtCurrentProcess(), NtCurrentThread());
 NtTerminateThread(NtCurrentThread(), Status);
}

__declspec(noreturn) VOID NTAPI RtlRosExitUserThread
(
 IN NTSTATUS Status
)
{
 RtlRosSwitchStackForExit
 (
  NtCurrentTeb()->StaticUnicodeBuffer,
  sizeof(NtCurrentTeb()->StaticUnicodeBuffer),
  RtlRosExitUserThread_Stage2,
  Status
 );

 for(;;);
}

/* EOF */
