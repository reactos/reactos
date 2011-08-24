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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_RPC_DEFS_H
#define __WINE_RPC_DEFS_H

#include "pshpack1.h"
typedef struct
{
  unsigned char rpc_ver;          /* RPC major version (5) */
  unsigned char rpc_ver_minor;    /* RPC minor version (0) */
  unsigned char ptype;            /* Packet type (PKT_*) */
  unsigned char flags;
  unsigned char drep[4];          /* Data representation */
  unsigned short frag_len;        /* Data size in bytes including header and tail. */
  unsigned short auth_len;        /* Authentication length  */
  unsigned int  call_id;          /* Call identifier. */
} RpcPktCommonHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned int   alloc_hint;      /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned short opnum;
} RpcPktRequestHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned int   alloc_hint;      /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char cancel_count;
  unsigned char reserved;
} RpcPktResponseHdr;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned int   alloc_hint;      /* Data size in bytes excluding header and tail. */
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char cancel_count;     /* Received cancel count */
  unsigned char reserved;         /* Force alignment! */
  unsigned int  status;           /* Runtime fault code (RPC_STATUS) */
  unsigned int  reserved2;
} RpcPktFaultHdr;

typedef struct
{
  unsigned short context_id;      /* Presentation context identifier */
  unsigned char num_syntaxes;     /* Number of syntaxes */
  unsigned char reserved;         /* For alignment */
  RPC_SYNTAX_IDENTIFIER abstract_syntax;
  RPC_SYNTAX_IDENTIFIER transfer_syntaxes[ANYSIZE_ARRAY]; /* size_is(num_syntaxes) */
} RpcContextElement;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned short max_tsize;       /* Maximum transmission fragment size */
  unsigned short max_rsize;       /* Maximum receive fragment size */
  unsigned int  assoc_gid;        /* Associated group id */
  unsigned char num_elements;     /* Number of elements */
  unsigned char padding[3];       /* Force alignment! */
  /*
   * Following this header are these fields:
   *  RpcContextElement context_elements[num_elements]
   */
} RpcPktBindHdr;

typedef struct
{
  unsigned short length;  /* Length of the string including null terminator */
  char string[ANYSIZE_ARRAY]; /* String data in single byte, null terminated form */
} RpcAddressString;

typedef struct
{
  unsigned short result;
  unsigned short reason;
  RPC_SYNTAX_IDENTIFIER transfer_syntax;
} RpcResult;

typedef struct
{
  unsigned char num_results;       /* Number of results */
  unsigned char reserved[3];       /* Force alignment! */
  RpcResult results[ANYSIZE_ARRAY]; /* size_is(num_results) */
} RpcResultList;

typedef struct
{
  RpcPktCommonHdr common;
  unsigned short max_tsize;       /* Maximum transmission fragment size */
  unsigned short max_rsize;       /* Maximum receive fragment size */
  unsigned int assoc_gid;         /* Associated group id */
  /* 
   * Following this header are these fields:
   *   RpcAddressString server_address;
   *   [0 - 3 bytes of padding so that results is 4-byte aligned]
   *   RpcResultList results;
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
  } protocols[ANYSIZE_ARRAY];
} RpcPktBindNAckHdr;

/* undocumented packet sent during RPC over HTTP */
typedef struct
{
  RpcPktCommonHdr common;
  unsigned short flags;
  unsigned short num_data_items;
} RpcPktHttpHdr;

/* AUTH3 packet */
typedef struct
{
  RpcPktCommonHdr common;
  unsigned int pad; /* ignored */
} RpcPktAuth3Hdr;

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
  RpcPktHttpHdr http;
  RpcPktAuth3Hdr auth3;
} RpcPktHdr;

typedef struct
{
  unsigned char auth_type;       /* authentication scheme in use */
  unsigned char auth_level;      /* RPC_C_AUTHN_LEVEL* */
  unsigned char auth_pad_length; /* length of padding to restore n % 4 alignment */
  unsigned char auth_reserved;   /* reserved, must be zero */
  unsigned int  auth_context_id; /* unique value for the authenticated connection */
} RpcAuthVerifier;
#include "poppack.h"

#define RPC_AUTH_VERIFIER_LEN(common_hdr) \
    ((common_hdr)->auth_len ? (common_hdr)->auth_len + sizeof(RpcAuthVerifier) : 0)

#define RPC_VER_MAJOR             5
#define RPC_VER_MINOR             0

#define RPC_FLG_FIRST             1
#define RPC_FLG_LAST              2
#define RPC_FLG_OBJECT_UUID    0x80

#define RPC_MIN_PACKET_SIZE  0x1000
#define RPC_MAX_PACKET_SIZE  0x16D0

enum rpc_packet_type
{
    PKT_REQUEST = 0,
    PKT_PING = 1,
    PKT_RESPONSE = 2,
    PKT_FAULT = 3,
    PKT_WORKING = 4,
    PKT_NOCALL = 5,
    PKT_REJECT = 6,
    PKT_ACK = 7,
    PKT_CL_CANCEL = 8,
    PKT_FACK = 9,
    PKT_CANCEL_ACK = 10,
    PKT_BIND = 11,
    PKT_BIND_ACK = 12,
    PKT_BIND_NACK = 13,
    PKT_ALTER_CONTEXT = 14,
    PKT_ALTER_CONTEXT_RESP = 15,
    PKT_AUTH3 = 16,
    PKT_SHUTDOWN = 17,
    PKT_CO_CANCEL = 18,
    PKT_ORPHANED = 19,
    PKT_HTTP = 20,
};

#define RESULT_ACCEPT               0
#define RESULT_USER_REJECTION       1
#define RESULT_PROVIDER_REJECTION   2

#define REASON_NONE                             0
#define REASON_ABSTRACT_SYNTAX_NOT_SUPPORTED    1
#define REASON_TRANSFER_SYNTAXES_NOT_SUPPORTED  2
#define REASON_LOCAL_LIMIT_EXCEEDED             3

#define REJECT_REASON_NOT_SPECIFIED            0
#define REJECT_TEMPORARY_CONGESTION            1
#define REJECT_LOCAL_LIMIT_EXCEEDED            2
#define REJECT_CALLED_PADDR_UNKNOWN            3 /* not used */
#define REJECT_PROTOCOL_VERSION_NOT_SUPPORTED  4
#define REJECT_DEFAULT_CONTEXT_NOT_SUPPORTED   5 /* not used */
#define REJECT_USER_DATA_NOT_READABLE          6 /* not used */
#define REJECT_NO_PSAP_AVAILABLE               7 /* not used */
#define REJECT_UNKNOWN_AUTHN_SERVICE           8
#define REJECT_INVALID_CHECKSUM                9

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
