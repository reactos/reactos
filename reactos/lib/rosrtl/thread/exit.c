/* $Id: exit.c,v 1.1 2003/12/30 05:10:32 hyperion Exp $
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
