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
 *
 * ---- rpcss_main.c:
 *   Initialize and start serving requests.  Bail if rpcss already is
 *   running.
 *
 * ---- RPCSS.EXE:
 *   
 *   Wine needs a server whose role is somewhat like that
 *   of rpcss.exe in windows.  This is not a clone of
 *   windows rpcss at all.  It has been given the same name, however,
 *   to provide for the possibility that at some point in the future, 
 *   it may become interface compatible with the "real" rpcss.exe on
 *   Windows.
 *
 * ---- KNOWN BUGS / TODO:
 *
 *   o Service hooks are unimplemented (if you bother to implement
 *     these, also implement net.exe, at least for "net start" and
 *     "net stop" (should be pretty easy I guess, assuming the rest
 *     of the services API infrastructure works.
 *
 *   o There is a looming problem regarding listening on privileged
 *     ports.  We will need to be able to coexist with SAMBA, and be able
 *     to function without running winelib code as root.  This may
 *     take some doing, including significant reconceptualization of the
 *     role of rpcss.exe in wine.
 */

#include <stdio.h>
#include <limits.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "rpcss.h"
#include "winnt.h"
#include "irot.h"
#include "epm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HANDLE exit_event;

//extern HANDLE __wine_make_process_system(void);

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

  //exit_event = __wine_make_process_system();
  exit_event = CreateEventW(NULL, FALSE, FALSE, NULL); // never fires

  return TRUE;

fail:
  RpcServerUnregisterIf(epm_v3_0_s_ifspec, NULL, FALSE);
  RpcServerUnregisterIf(Irot_v0_2_s_ifspec, NULL, FALSE);
  return FALSE;
}

/* returns false if we discover at the last moment that we
   aren't ready to terminate */
static BOOL RPCSS_Shutdown(void)
{
  RpcMgmtStopServerListening(NULL);
  RpcServerUnregisterIf(epm_v3_0_s_ifspec, NULL, TRUE);
  RpcServerUnregisterIf(Irot_v0_2_s_ifspec, NULL, TRUE);

  CloseHandle(exit_event);

  return TRUE;
}

int main( int argc, char **argv )
{
  /* 
   * We are invoked as a standard executable; we act in a
   * "lazy" manner.  We register our interfaces and endpoints, and hang around
   * until we all user processes exit, and then silently terminate.
   */

  if (RPCSS_Initialize()) {
    WaitForSingleObject(exit_event, INFINITE);
    RPCSS_Shutdown();
  }

  return 0;
}
