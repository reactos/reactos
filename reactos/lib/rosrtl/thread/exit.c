/* $Id$
*/
/*
*/

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define NDEBUG
#include <debug.h>

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
