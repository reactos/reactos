/*
 * Copyright 2001, Ove Kåven, TransGaming Technologies Inc.
 * Copyright 2002 Greg Turner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winsvc.h"
#include "irot_s.h"
#include "epm_s.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static WCHAR rpcssW[] = {'R','p','c','S','s',0};
static HANDLE exit_event;
static SERVICE_STATUS_HANDLE service_handle;

static BOOL RPCSS_Initialize(void)
{
  static unsigned short irot_protseq[] = IROT_PROTSEQ;
  static unsigned short irot_endpoint[] = IROT_ENDPOINT;
  static unsigned short epm_protseq[] = {'n','c','a','c','n','_','n','p',0};
  static unsigned short epm_endpoint[] = {'\\','p','i','p','e','\\','e','p','m','a','p','p','e','r',0};
  static unsigned short epm_protseq_lrpc[] = {'n','c','a','l','r','p','c',0};
  static unsigned short epm_endpoint_lrpc[] = {'e','p','m','a','p','p','e','r',0};
  RPC_STATUS status;

  WINE_TRACE("\n");

  status = RpcServerRegisterIf(epm_v3_0_s_ifspec, NULL, NULL);
  if (status != RPC_S_OK)
    return status;
  status = RpcServerRegisterIf(Irot_v0_2_s_ifspec, NULL, NULL);
  if (status != RPC_S_OK)
  {
    RpcServerUnregisterIf(epm_v3_0_s_ifspec, NULL, FALSE);
    return FALSE;
  }

  status = RpcServerUseProtseqEpW(epm_protseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                  epm_endpoint, NULL);
  if (status != RPC_S_OK)
    goto fail;

  status = RpcServerUseProtseqEpW(epm_protseq_lrpc, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                  epm_endpoint_lrpc, NULL);
  if (status != RPC_S_OK)
      goto fail;

  status = RpcServerUseProtseqEpW(irot_protseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                  irot_endpoint, NULL);
  if (status != RPC_S_OK)
    goto fail;

  status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
  if (status != RPC_S_OK)
    goto fail;

  return TRUE;

fail:
  RpcServerUnregisterIf(epm_v3_0_s_ifspec, NULL, FALSE);
  RpcServerUnregisterIf(Irot_v0_2_s_ifspec, NULL, FALSE);
  return FALSE;
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
#ifdef __REACTOS__
    status.dwControlsAccepted        = 0;
#else
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
#endif
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch (ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        TRACE( "shutting down\n" );
        RpcMgmtStopServerListening( NULL );
        RpcServerUnregisterIf( epm_v3_0_s_ifspec, NULL, TRUE );
        RpcServerUnregisterIf( Irot_v0_2_s_ifspec, NULL, TRUE );
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( exit_event );
        return NO_ERROR;
    default:
        FIXME( "got service ctrl %x\n", ctrl );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

#ifdef __REACTOS__
extern VOID DoRpcSsSetupConfiguration(VOID);
#endif

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    SERVICE_STATUS status;

    TRACE( "starting service\n" );

    if (!RPCSS_Initialize()) return;

    exit_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( rpcssW, service_handler, NULL );
    if (!service_handle) return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
#ifdef __REACTOS__
    status.dwControlsAccepted        = 0;
#else
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
#endif
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
#ifdef __REACTOS__
    status.dwWaitHint                = 0;
#else
    status.dwWaitHint                = 10000;
#endif
    SetServiceStatus( service_handle, &status );

#ifdef __REACTOS__
    DoRpcSsSetupConfiguration();
#endif

    WaitForSingleObject( exit_event, INFINITE );

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    TRACE( "service stopped\n" );
}

int wmain( int argc, WCHAR *argv[] )
{
    static const SERVICE_TABLE_ENTRYW service_table[] =
    {
        { rpcssW, ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcherW( service_table );
    return 0;
}
