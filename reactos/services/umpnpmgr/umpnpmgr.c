/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/umpnpmgr/umpnpmgr.c
 * PURPOSE:          User-mode Plug and Play manager
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <ntos/ntpnp.h>
#include <ddk/wdmguid.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <rpc.h>
#include <rpcdce.h>

#include "pnp.h"

#define DBG
#define NDEBUG
#include <debug.h>



/* GLOBALS ******************************************************************/

static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv);

static SERVICE_TABLE_ENTRY ServiceTable[2] =
{
  {_T("PlugPlay"), ServiceMain},
  {NULL, NULL}
};


/* FUNCTIONS *****************************************************************/

static DWORD WINAPI
RpcServerThread(LPVOID lpParameter)
{
  RPC_STATUS Status;

  DPRINT("RpcServerThread() called\n");

  Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                  20,
                                  L"\\pipe\\umpnpmgr",
                                  NULL);  // Security descriptor
  if (Status != RPC_S_OK)
  {
    DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
    return 0;
  }

  Status = RpcServerRegisterIf(pnp_v1_0_s_ifspec,
                               NULL,
                               NULL);
  if (Status != RPC_S_OK)
  {
    DPRINT1("RpcServerRegisterIf() failed (Status %lx)\n", Status);
    return 0;
  }

  Status = RpcServerListen(1,
                           20,
                           FALSE);
  if (Status != RPC_S_OK)
  {
    DPRINT1("RpcServerListen() failed (Status %lx)\n", Status);
    return 0;
  }

  DPRINT("RpcServerThread() done\n");

  return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
  return GlobalAlloc(GPTR, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
  GlobalFree(ptr);
}


//WORD PNP_GetVersion(RPC_BINDING_HANDLE BindingHandle)
WORD PNP_GetVersion(handle_t BindingHandle)
{
  return 0x0400;
}


static DWORD WINAPI
PnpEventThread(LPVOID lpParameter)
{
  PPLUGPLAY_EVENT_BLOCK PnpEvent;
  ULONG PnpEventSize;
  NTSTATUS Status;
  RPC_STATUS RpcStatus;

  PnpEventSize = 0x1000;
  PnpEvent = HeapAlloc(GetProcessHeap(), 0, PnpEventSize);
  if (PnpEvent == NULL)
    return ERROR_OUTOFMEMORY;

  for (;;)
  {
    DPRINT("Calling NtGetPlugPlayEvent()\n");

    /* Wait for the next pnp event */
    Status = NtGetPlugPlayEvent(0, 0, PnpEvent, PnpEventSize);
    /* Resize the buffer for the PnP event if it's too small. */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
      PnpEventSize += 0x400;
      PnpEvent = HeapReAlloc(GetProcessHeap(), 0, PnpEvent, PnpEventSize);
      if (PnpEvent == NULL)
        return ERROR_OUTOFMEMORY;
      continue;
    }
    if (!NT_SUCCESS(Status))
    {
      DPRINT("NtPlugPlayEvent() failed (Status %lx)\n", Status);
      break;
    }

    DPRINT("Received PnP Event\n");
    if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ARRIVAL, &RpcStatus))
    {
      DPRINT1("Device arrival event: %S\n", PnpEvent->TargetDevice.DeviceIds);
    }
    else
    {
      DPRINT1("Unknown event\n");
    }

    /* FIXME: Process the pnp event */

    /* Dequeue the current pnp event and signal the next one */
    NtPlugPlayControl(PLUGPLAY_USER_RESPONSE, NULL, 0);
  }

  HeapFree(GetProcessHeap(), 0, PnpEvent);

  return ERROR_SUCCESS;
}


static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{
  HANDLE hThread;
  DWORD dwThreadId;

  DPRINT("ServiceMain() called\n");

  hThread = CreateThread(NULL,
			 0,
			 PnpEventThread,
			 NULL,
			 0,
			 &dwThreadId);
  if (hThread != NULL)
    CloseHandle(hThread);

  hThread = CreateThread(NULL,
			 0,
			 RpcServerThread,
			 NULL,
			 0,
			 &dwThreadId);
  if (hThread != NULL)
    CloseHandle(hThread);

  DPRINT("ServiceMain() done\n");
}


int
main(int argc, char *argv[])
{
  DPRINT("Umpnpmgr: main() started\n");

  StartServiceCtrlDispatcher(ServiceTable);

  DPRINT("Umpnpmgr: main() done\n");

  ExitThread(0);

  return 0;
}

/* EOF */
