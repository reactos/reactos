/* $Id: exit.c,v 1.4 2004/04/09 20:03:14 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/exit.c
 * PURPOSE:         Shutdown related functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#include <windows.h>
#include <user32.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <ntdll/csr.h>

/*
 * @implemented
 */
BOOL STDCALL
ExitWindowsEx(UINT uFlags,
	      DWORD dwReserved)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_EXIT_REACTOS;
  Request.Data.ExitReactosRequest.Flags = uFlags;
  Request.Data.ExitReactosRequest.Reserved = dwReserved;

  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
RegisterServicesProcess(DWORD ServicesProcessId)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_REGISTER_SERVICES_PROCESS;
  Request.Data.RegisterServicesProcessRequest.ProcessId = ServicesProcessId;

  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}

/* EOF */
