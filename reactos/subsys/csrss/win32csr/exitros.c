/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSRSS subsystem
 * FILE:            subsys/csrss/win32csr/exitros.c
 * PURPOSE:         Logout/shutdown
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <reactos/winlogon.h>
#include "api.h"
#include "win32csr.h"

#define NDEBUG
#include <debug.h>

static HWND LogonNotifyWindow = NULL;
static DWORD LogonProcess = 0;

CSR_API(CsrRegisterLogonProcess)
{
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  if (Request->Data.RegisterLogonProcessRequest.Register)
    {
      if (0 != LogonProcess)
        {
          Reply->Status = STATUS_LOGON_SESSION_EXISTS;
          return Reply->Status;
        }
      LogonProcess = Request->Data.RegisterLogonProcessRequest.ProcessId;
    }
  else
    {
      if ((DWORD) Request->Header.ClientId.UniqueProcess != LogonProcess)
        {
          DPRINT1("Current logon process 0x%x, can't deregister from process 0x%x\n",
                  LogonProcess, Request->Header.ClientId.UniqueProcess);
          Reply->Status = STATUS_NOT_LOGON_PROCESS;
          return Reply->Status;
        }
      LogonProcess = 0;
    }

  Reply->Status = STATUS_SUCCESS;

  return Reply->Status;
}

CSR_API(CsrSetLogonNotifyWindow)
{
  DWORD WindowCreator;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  if (0 == GetWindowThreadProcessId(Request->Data.SetLogonNotifyWindowRequest.LogonNotifyWindow,
                                    &WindowCreator))
    {
      DPRINT1("Can't get window creator\n");
      Reply->Status = STATUS_INVALID_HANDLE;
      return Reply->Status;
    }
  if (WindowCreator != LogonProcess)
    {
      DPRINT1("Trying to register window not created by winlogon as notify window\n");
      Reply->Status = STATUS_ACCESS_DENIED;
      return Reply->Status;
    }

  LogonNotifyWindow = Request->Data.SetLogonNotifyWindowRequest.LogonNotifyWindow;

  Reply->Status = STATUS_SUCCESS;

  return Reply->Status;
}

CSR_API(CsrExitReactos)
{
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  if (NULL == LogonNotifyWindow)
    {
      DPRINT1("No LogonNotifyWindow registered\n");
      Reply->Status = STATUS_NOT_FOUND;
      return Reply->Status;
    }

  /* FIXME Inside 2000 says we should impersonate the caller here */
  Reply->Status = SendMessageW(LogonNotifyWindow, PM_WINLOGON_EXITWINDOWS,
                               (WPARAM) Request->Header.ClientId.UniqueProcess,
                               (LPARAM) Request->Data.ExitReactosRequest.Flags);
  /* If the message isn't handled, the return value is 0, so 0 doesn't indicate success.
     Success is indicated by a 1 return value, if anything besides 0 or 1 it's a
     NTSTATUS value */
  if (1 == Reply->Status)
    {
      Reply->Status = STATUS_SUCCESS;
    }
  else if (0 == Reply->Status)
    {
      Reply->Status = STATUS_NOT_IMPLEMENTED;
    }

  return Reply->Status;
}

/* EOF */
