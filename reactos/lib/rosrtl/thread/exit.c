/* $Id: exit.c,v 1.2 2004/10/03 18:53:05 gvg Exp $
*/
/*
*/

#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

#include <rosrtl/thread.h>

__declspec(noreturn) VOID NTAPI RtlRosExitUserThread
(
 IN NTSTATUS Status
)
{
 NtTerminateThread(NtCurrentThread(), Status);

 for(;;);
}

/* EOF */
