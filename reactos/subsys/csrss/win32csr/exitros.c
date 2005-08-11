/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSRSS subsystem
 * FILE:            subsys/csrss/win32csr/exitros.c
 * PURPOSE:         Logout/shutdown
 */

/* INCLUDES ******************************************************************/

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

static HWND LogonNotifyWindow = NULL;
static HANDLE LogonProcess = NULL;

CSR_API(CsrRegisterLogonProcess)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - LPC_MESSAGE_BASE_SIZE;

  if (Request->Data.RegisterLogonProcessRequest.Register)
    {
      if (0 != LogonProcess)
        {
          Request->Status = STATUS_LOGON_SESSION_EXISTS;
          return Request->Status;
        }
      LogonProcess = Request->Data.RegisterLogonProcessRequest.ProcessId;
    }
  else
    {
      if (Request->Header.ClientId.UniqueProcess != LogonProcess)
        {
          DPRINT1("Current logon process 0x%x, can't deregister from process 0x%x\n",
                  LogonProcess, Request->Header.ClientId.UniqueProcess);
          Request->Status = STATUS_NOT_LOGON_PROCESS;
          return Request->Status;
        }
      LogonProcess = 0;
    }

  Request->Status = STATUS_SUCCESS;

  return Request->Status;
}

CSR_API(CsrSetLogonNotifyWindow)
{
  DWORD WindowCreator;

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - LPC_MESSAGE_BASE_SIZE;

  if (0 == GetWindowThreadProcessId(Request->Data.SetLogonNotifyWindowRequest.LogonNotifyWindow,
                                    &WindowCreator))
    {
      DPRINT1("Can't get window creator\n");
      Request->Status = STATUS_INVALID_HANDLE;
      return Request->Status;
    }
  if (WindowCreator != (DWORD)LogonProcess)
    {
      DPRINT1("Trying to register window not created by winlogon as notify window\n");
      Request->Status = STATUS_ACCESS_DENIED;
      return Request->Status;
    }

  LogonNotifyWindow = Request->Data.SetLogonNotifyWindowRequest.LogonNotifyWindow;

  Request->Status = STATUS_SUCCESS;

  return Request->Status;
}

CSR_API(CsrExitReactos)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - LPC_MESSAGE_BASE_SIZE;

  if (NULL == LogonNotifyWindow)
    {
      DPRINT1("No LogonNotifyWindow registered\n");
      Request->Status = STATUS_NOT_FOUND;
      return Request->Status;
    }

  /* FIXME Inside 2000 says we should impersonate the caller here */
  Request->Status = SendMessageW(LogonNotifyWindow, PM_WINLOGON_EXITWINDOWS,
                               (WPARAM) Request->Header.ClientId.UniqueProcess,
                               (LPARAM) Request->Data.ExitReactosRequest.Flags);
  /* If the message isn't handled, the return value is 0, so 0 doesn't indicate success.
     Success is indicated by a 1 return value, if anything besides 0 or 1 it's a
     NTSTATUS value */
  if (1 == Request->Status)
    {
      Request->Status = STATUS_SUCCESS;
    }
  else if (0 == Request->Status)
    {
      Request->Status = STATUS_NOT_IMPLEMENTED;
    }

  return Request->Status;
}

/* EOF */
