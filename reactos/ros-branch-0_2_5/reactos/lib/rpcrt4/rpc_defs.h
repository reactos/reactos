/*
 * RPC definitions
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Filip Navara
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

#ifndef __WINE_RPC_DEFS_H
#define __WINE_RPC_DEFS_H

/* info from http://www.microsoft.com/msj/0398/dcomtextfigs.htm */

typedef struct
{
  unsigned char rpc_ver;          /* RPC major version (5) */
  unsigned char rpc_ver_minor;    /* RPC minor version (0) */
  unsigned char ptype;            /* Packet type (PKT_*) */
  unsigned char flags;
  unsigned char drep[4];          /* Data representation */
  unsigned short frag_len;        /* Data size in bytes including header and tail. */
  unsigned short auth_len;        /* Authentication length  */
  unsigned long call_id;          /* Call identifier. */
} RpcPktCommonHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned long alloc_hint;       /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned short opnum;
} RpcPktRequestHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned long alloc_hint;       /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char cancel_count;
  unsigned char reserved;
} RpcPktResponseHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned long alloc_hint;       /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char alert_count;      /* Pending alert count */
  unsigned char padding[3];       /* Force alignment! */
  unsigned long status;           /* Runtime fault code (RPC_STATUS) */
  unsigned long reserved;
} RpcPktFaultHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned short max_tsize;       /* Maximum transmission fragment size */
  unsigned short max_rsize;       /* Maximum receive fragment size */
  unsigned long assoc_gid;        /* Associated group id */
  unsigned char num_elements;     /* Number of elements */
  unsigned char padding[3];       /* Force alignment! */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char num_syntaxes;     /* Number of syntaxes */
  RPC_SYNTAX_IDENTIFIER abstract;
  RPC_SYNTAX_IDENTIFIER transfer;
} RpcPktBindHdr;

#include "pshpack1.h"
typedef struct
{
  unsigned short length;  /* Length of the string including null terminator */
  char string[1];         /* String data in single byte, null terminated form */
} RpcAddressString;
#include "poppack.h"

typedef struct
{
  unsigned char padding1[2];       /* Force alignment! */
  unsigned char num_results;       /* Number of results */
  unsigned char padding2[3];       /* Force alignment! */
  struct {
    unsigned short result;
    unsigned short reason;
  } results[1];
} RpcResults;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned short max_tsize;       /* Maximum transmission fragment size */
  unsigned short max_rsize;       /* Maximum receive fragment size */
  unsigned long assoc_gid;        /* Associated group id */
  /* 
   * Following this header are these fields:
   *   RpcAddressString server_address;
   *   RpcResults results;
   *   RPC_SYNTAX_IDENTIFIER transfer;
   */
} RpcPktBindAckHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned short reject_reason;
  unsigned char protocols_count;
  struct {
    unsigned char rpc_ver;
    unsigned char rpc_ver_minor;
  } protocols[1];
} RpcPktBindNAckHdr;

/* Union representing all possible packet headers */
typedef union
{
  RpcPktCommonHdr common;
  RpcPktRequestHdr request;
  RpcPktResponseHdr response;
  RpcPktFaultHdr fault;
  RpcPktBindHdr bind;
  RpcPktBindAckHdr bind_ack;
  RpcPktBindNAckHdr bind_nack;
} RpcPktHdr;

#define RPC_VER_MAJOR             5
#define RPC_VER_MINOR             0

#define RPC_FLG_FIRST             1
#define RPC_FLG_LAST              2
#define RPC_FLG_OBJECT_UUID    0x80

#define RPC_MIN_PACKET_SIZE  0x1000
#define RPC_MAX_PACKET_SIZE  0x16D0

#define PKT_REQUEST             0
#define PKT_PING                1
#define PKT_RESPONSE            2
#define PKT_FAULT               3
#define PKT_WORKING             4
#define PKT_NOCALL              5
#define PKT_REJECT              6
#define PKT_ACK                 7
#define PKT_CL_CANCEL           8
#define PKT_FACK                9
#define PKT_CANCEL_ACK         10
#define PKT_BIND               11
#define PKT_BIND_ACK           12
#define PKT_BIND_NACK          13
#define PKT_ALTER_CONTEXT      14
#define PKT_ALTER_CONTEXT_RESP 15
#define PKT_SHUTDOWN           17
#define PKT_CO_CANCEL          18
#define PKT_ORPHANED           19

#define RESULT_ACCEPT           0

#define NO_REASON               0

#define NCADG_IP_UDP   0x08
#define NCACN_IP_TCP   0x07
#define NCADG_IPX      0x0E
#define NCACN_SPX      0x0C
#define NCACN_NB_NB    0x12
#define NCACN_NB_IPX   0x0D
#define NCACN_DNET_NSP 0x04
#define NCACN_HTTP     0x1F

/* FreeDCE: TWR_C_FLR_PROT_ID_IP */
#define TWR_IP 0x09

#endif  /* __WINE_RPC_DEFS_H */
