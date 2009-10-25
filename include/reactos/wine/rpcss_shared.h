/*
 * RPCSS shared definitions
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

#ifndef __WINE_RPCSS_SHARED_H
#define __WINE_RPCSS_SHARED_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <rpc.h>
#include <rpcdcep.h>

#define RPCSS_NP_PROTOCOL_VERSION 0x0000

#define RPCSS_STRINGIFY_MACRO(x) RPCSS_STRINGIFY_MACRO2(x)
#define RPCSS_STRINGIFY_MACRO2(x) #x

#define STRINGIFIED_RPCSS_NP_PROTOCOL_VERSION \
  RPCSS_STRINGIFY_MACRO(RPCSS_NP_PROTOCOL_VERSION)

/* only local communications are supported so far on this pipe.
   until this changes, we can just use a constant pipe-name */
#define NAME_RPCSS_NAMED_PIPE \
  ("\\\\.\\pipe\\RpcssNP" STRINGIFIED_RPCSS_NP_PROTOCOL_VERSION)

/* mutex is local only... perhaps this ought to be part of the pipe
   protocol for remote wine<->wine connections? */
#define RPCSS_MASTER_MUTEX_NAME \
  ("RPCSSMasterMutex" STRINGIFIED_RPCSS_NP_PROTOCOL_VERSION)

/* payloads above 1K are fragmented into multiple messages */
#define VARDATA_PAYLOAD_BYTES 1024

/* ick -- maybe we should pass a handle to a mailslot or something? */
#define MAX_RPCSS_NP_REPLY_STRING_LEN 512

/* number of microseconds/10 to wait for master mutex before giving up */
#define MASTER_MUTEX_TIMEOUT 6000000

/* number of miliseconds to wait on the master mutex after it returns BUSY */
#define MASTER_MUTEX_WAITNAMEDPIPE_TIMEOUT 5000

/* a data payload; not a normal message */
#define RPCSS_NP_MESSAGE_TYPEID_VARDATAPAYLOADMSG 1
typedef struct _RPCSS_NP_MESSAGE_UNION_VARDATAPAYLOADMSG {
  char payload[VARDATA_PAYLOAD_BYTES];
} RPCSS_NP_MESSAGE_UNION_VARDATAPAYLOADMSG;

/* RANMSG:
 *   Simply tells the server that another rpcss instance ran.
 *   The server should respond by resetting its timeout to the
 *   full lazy timeout.
 */
#define RPCSS_NP_MESSAGE_TYPEID_RANMSG 2
typedef struct _RPCSS_NP_MESSAGE_UNION_RANMSG {
  long timeout;
} RPCSS_NP_MESSAGE_UNION_RANMSG;

/* REGISTEREPMSG:
 *   Registers endpoints with the endpoint server.
 *   object_count and binding_count contain the number
 *   of object uuids and endpoints in the vardata payload,
 *   respectively.
 */
#define RPCSS_NP_MESSAGE_TYPEID_REGISTEREPMSG 3
typedef struct _RPCSS_NP_MESSAGE_UNION_REGISTEREPMSG {
  RPC_SYNTAX_IDENTIFIER iface;
  int object_count;
  int binding_count;
  int no_replace;
} RPCSS_NP_MESSAGE_UNION_REGISTEREPMSG;

/* UNREGISTEREPMSG:
 *   Unregisters endpoints with the endpoint server.
 *   object_count and binding_count contain the number
 *   of object uuids and endpoints in the vardata payload,
 *   respectively.
 */
#define RPCSS_NP_MESSAGE_TYPEID_UNREGISTEREPMSG 4
typedef struct _RPCSS_NP_MESSAGE_UNION_UNREGISTEREPMSG {
  RPC_SYNTAX_IDENTIFIER iface;
  int object_count;
  int binding_count;
} RPCSS_NP_MESSAGE_UNION_UNREGISTEREPMSG;

/* RESOLVEEPMSG:
 *   Locates an endpoint registered with the endpoint server.
 *   Vardata contains a single protseq string.  This is a bit
 *   silly: the protseq string is probably shorter than the
 *   reply (an endpoint string), which is truncated at
 *   MAX_RPCSS_NP_REPLY_STRING_LEN, at least for the moment.
 *   returns the empty string if the requested endpoint isn't
 *   registered.
 */
#define RPCSS_NP_MESSAGE_TYPEID_RESOLVEEPMSG 5
typedef struct _RPCSS_NP_MESSAGE_UNION_RESOLVEEPMSG {
  RPC_SYNTAX_IDENTIFIER iface;
  UUID object;
} RPCSS_NP_MESSAGE_UNION_RESOLVEEPMSG;

typedef union {
  RPCSS_NP_MESSAGE_UNION_RANMSG ranmsg;
  RPCSS_NP_MESSAGE_UNION_VARDATAPAYLOADMSG vardatapayloadmsg;
  RPCSS_NP_MESSAGE_UNION_REGISTEREPMSG registerepmsg;
  RPCSS_NP_MESSAGE_UNION_UNREGISTEREPMSG unregisterepmsg;
  RPCSS_NP_MESSAGE_UNION_RESOLVEEPMSG resolveepmsg;
} RPCSS_NP_MESSAGE_UNION;

/* vardata_payload_size specifies the number of bytes
 * to be transferred over the pipe in VARDATAPAYLOAD
 * messages (divide by VARDATA_PAYLOAD_BYTES to
 * get the # of payloads)
 */
typedef struct _RPCSS_NP_MESSAGE {
  UINT32 message_type;
  RPCSS_NP_MESSAGE_UNION message;
  UINT32 vardata_payload_size;
} RPCSS_NP_MESSAGE, *PRPCSS_NP_MESSAGE;

typedef union {
  /* some of these aren't used, but I guess we don't care */
  UINT as_uint;
  INT  as_int;
  void   *as_pvoid;
  HANDLE as_handle;
  char as_string[MAX_RPCSS_NP_REPLY_STRING_LEN]; /* FIXME: yucky */
} RPCSS_NP_REPLY, *PRPCSS_NP_REPLY;

#endif /* __WINE_RPCSS_SHARED_H */
