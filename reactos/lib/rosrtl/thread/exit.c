/* $Id: exit.c,v 1.3.4.2 2004/10/25 02:25:01 ion Exp $
*/
/*
*/

#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/zwfuncs.h>
#include <ndk/pstypes.h>
#include "thread.h"

#define NDEBUG

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
