/*
 * RPCSS named pipe client implementation
 *
 * Copyright (C) 2002 Greg Turner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/rpcss_shared.h"
#include "wine/debug.h"

#include "rpc_binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HANDLE RPCRT4_RpcssNPConnect(void)
{
  HANDLE the_pipe = NULL;
  DWORD dwmode, wait_result;
  HANDLE master_mutex = RPCRT4_GetMasterMutex();

  TRACE("\n");

  while (TRUE) {

    wait_result = WaitForSingleObject(master_mutex, MASTER_MUTEX_TIMEOUT);
    switch (wait_result) {
      case WAIT_ABANDONED:
      case WAIT_OBJECT_0:
        break;
      case WAIT_FAILED:
      case WAIT_TIMEOUT:
      default:
        ERR("This should never happen: couldn't enter mutex.\n");
        return NULL;
    }

    /* try to open the client side of the named pipe. */
    the_pipe = CreateFileA(
      NAME_RPCSS_NAMED_PIPE,           /* pipe name */
      GENERIC_READ | GENERIC_WRITE,    /* r/w access */
      0,                               /* no sharing */
      NULL,                            /* no security attributes */
      OPEN_EXISTING,                   /* open an existing pipe */
      0,                               /* default attributes */
      NULL                             /* no template file */
    );

    if (the_pipe != INVALID_HANDLE_VALUE)
      break;

    if (GetLastError() != ERROR_PIPE_BUSY) {
      WARN("Unable to open named pipe %s (assuming unavailable).\n",
        debugstr_a(NAME_RPCSS_NAMED_PIPE));
      the_pipe = NULL;
      break;
    }

    WARN("Named pipe busy (will wait)\n");

    if (!ReleaseMutex(master_mutex))
      ERR("Failed to release master mutex.  Expect deadlock.\n");

    /* wait for the named pipe.  We are only
       willing to wait only 5 seconds.  It should be available /very/ soon. */
    if (! WaitNamedPipeA(NAME_RPCSS_NAMED_PIPE, MASTER_MUTEX_WAITNAMEDPIPE_TIMEOUT))
    {
      ERR("Named pipe unavailable after waiting.  Something is probably wrong.\n");
      the_pipe = NULL;
      break;
    }

  }

  if (the_pipe) {
    dwmode = PIPE_READMODE_MESSAGE;
    /* SetNamedPipeHandleState not implemented ATM, but still seems to work somehow. */
    if (! SetNamedPipeHandleState(the_pipe, &dwmode, NULL, NULL))
      WARN("Failed to set pipe handle state\n");
  }

  if (!ReleaseMutex(master_mutex))
    ERR("Uh oh, failed to leave the RPC Master Mutex!\n");

  return the_pipe;
}

BOOL RPCRT4_SendReceiveNPMsg(HANDLE np, PRPCSS_NP_MESSAGE msg, char *vardata, PRPCSS_NP_REPLY reply)
{
  DWORD count;
  UINT32 payload_offset;
  RPCSS_NP_MESSAGE vardata_payload_msg;

  TRACE("(np == %p, msg == %p, vardata == %p, reply == %p)\n",
    np, msg, vardata, reply);

  if (! WriteFile(np, msg, sizeof(RPCSS_NP_MESSAGE), &count, NULL)) {
    ERR("write failed.\n");
    return FALSE;
  }

  if (count != sizeof(RPCSS_NP_MESSAGE)) {
    ERR("write count mismatch.\n");
    return FALSE;
  }

  /* process the vardata payload if necessary */
  vardata_payload_msg.message_type = RPCSS_NP_MESSAGE_TYPEID_VARDATAPAYLOADMSG;
  vardata_payload_msg.vardata_payload_size = 0; /* meaningless */
  for ( payload_offset = 0; payload_offset < msg->vardata_payload_size;
        payload_offset += VARDATA_PAYLOAD_BYTES ) {
    TRACE("sending vardata payload.  vd=%p, po=%d, ps=%d\n", vardata,
      payload_offset, msg->vardata_payload_size);
    ZeroMemory(vardata_payload_msg.message.vardatapayloadmsg.payload, VARDATA_PAYLOAD_BYTES);
    CopyMemory(vardata_payload_msg.message.vardatapayloadmsg.payload,
               vardata,
	       min( VARDATA_PAYLOAD_BYTES, msg->vardata_payload_size - payload_offset ));
    vardata += VARDATA_PAYLOAD_BYTES;
    if (! WriteFile(np, &vardata_payload_msg, sizeof(RPCSS_NP_MESSAGE), &count, NULL)) {
      ERR("vardata write failed at %u bytes.\n", payload_offset);
      return FALSE;
    }
  }

  if (! ReadFile(np, reply, sizeof(RPCSS_NP_REPLY), &count, NULL)) {
    ERR("read failed.\n");
    return FALSE;
  }

  if (count != sizeof(RPCSS_NP_REPLY)) {
    ERR("read count mismatch. got %ld, expected %u.\n", count, sizeof(RPCSS_NP_REPLY));
    return FALSE;
  }

  /* message execution was successful */
  return TRUE;
}
