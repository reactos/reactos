/* $Id: api.c,v 1.1 1999/12/26 15:50:46 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/api.c
 * PURPOSE:         CSRSS API
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

static HANDLE WindowsApiPort;

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrConnectToServer(VOID)
{
   NTSTATUS Status;
   UNICODE_STRING PortName;
   
   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   
   Status = NtConnectPort(&WindowsApiPort,
			  &PortName,
			  NULL,
			  NULL,
			  NULL,
			  NULL,
			  
}
