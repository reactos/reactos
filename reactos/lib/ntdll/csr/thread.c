/* $Id: thread.c,v 1.1 2001/06/17 20:05:09 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/propvar.c
 * PURPOSE:         CSRSS threads API
 */
#define NTOS_MODE_USER
#include <ntos.h>

#include <ntdll/csr.h>
#include <string.h>

#include <csrss/csrss.h>

#define NDEBUG
#include <ntdll/ntdll.h>

NTSTATUS STDCALL
CsrNewThread(VOID)
{
   return (NtRegisterThreadTerminatePort(WindowsApiPort));
}

NTSTATUS STDCALL
CsrSetPriorityClass(HANDLE Process,
		    PULONG PriorityClass)
{
   /* FIXME: call csrss to get hProcess' priority */
   *PriorityClass = CSR_PRIORITY_CLASS_NORMAL;

   return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
STDCALL
CsrIdentifyAlertableThread (VOID)
{
  /* FIXME: notify csrss that current thread is alertable */
#if 0
   CSRSS_IDENTIFY_ALERTABLE_THREAD_REPLY	Reply;
   CSRSS_IDENTIFY_ALERTABLE_THREAD_REQUEST	Request = (PCSRSS_IDENTIFY_ALERTABLE_THREAD_REQUEST) & Reply;
   PNT_TEB					Teb;

   Request->UniqueThread = NtCurrentTeb()->Cid.UniqueThread;
   /* FIXME: this is written the NT way, NOT the ROS way! */
   return CsrClientCallServer (
		Request,
		NULL, /* use Request storage for reply */
		CSRSS_IDENTIFY_ALERTABLE_THREAD,
		sizeof (CSRSS_IDENTIFY_ALERTABLE_THREAD_REPLY)
		);
#endif
   return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
