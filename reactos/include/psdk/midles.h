/*
 * NDR Serialization Services
 *
 * Copyright (c) 2007 Robert Shearman for CodeWeavers
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

#ifndef __WINE_MIDLES_H__
#define __WINE_MIDLES_H__

#include <rpcndr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MES_ENCODE,
    MES_DECODE,
    MES_ENCODE_NDR64
} MIDL_ES_CODE;

typedef enum
{
    MES_INCREMENTAL_HANDLE,
    MES_FIXED_BUFFER_HANDLE,
    MES_DYNAMIC_BUFFER_HANDLE
} MIDL_ES_HANDLE_STYLE;

typedef void (__RPC_USER * MIDL_ES_ALLOC)(void *,char **,unsigned int *);
typedef void (__RPC_USER * MIDL_ES_WRITE)(void *,char *,unsigned int);
typedef void (__RPC_USER * MIDL_ES_READ)(void *,char **,unsigned int *);

typedef struct _MIDL_ES_MESSAGE
{
    MIDL_STUB_MESSAGE       StubMsg;
    MIDL_ES_CODE            Operation;
    void                   *UserState;
    ULONG                   MesVersion : 8;
    ULONG                   HandleStyle : 8;
    ULONG                   HandleFlags : 8;
    ULONG                   Reserve : 8;
    MIDL_ES_ALLOC           Alloc;
    MIDL_ES_WRITE           Write;
    MIDL_ES_READ            Read;
    unsigned char          *Buffer;
    ULONG                   BufferSize;
    unsigned char         **pDynBuffer;
    ULONG                  *pEncodedSize;
    RPC_SYNTAX_IDENTIFIER   InterfaceId;
    ULONG                   ProcNumber;
    ULONG                   AlienDataRep;
    ULONG                   IncrDataSize;
    ULONG                   ByteCount;
} MIDL_ES_MESSAGE, *PMIDL_ES_MESSAGE;

typedef PMIDL_ES_MESSAGE MIDL_ES_HANDLE;

typedef struct _MIDL_TYPE_PICKLING_INFO
{
    ULONG       Version;
    ULONG       Flags;
    UINT_PTR    Reserved[3];
} MIDL_TYPE_PICKLING_INFO, *PMIDL_TYPE_PICKLING_INFO;

RPC_STATUS RPC_ENTRY
 MesEncodeIncrementalHandleCreate(void *,MIDL_ES_ALLOC,MIDL_ES_WRITE,handle_t *);
RPC_STATUS RPC_ENTRY
 MesDecodeIncrementalHandleCreate(void *,MIDL_ES_READ,handle_t *);
RPC_STATUS RPC_ENTRY
 MesIncrementalHandleReset(handle_t,void *,MIDL_ES_ALLOC,MIDL_ES_WRITE,MIDL_ES_READ,MIDL_ES_CODE);

RPC_STATUS RPC_ENTRY
 MesEncodeFixedBufferHandleCreate(char *,ULONG,ULONG *,handle_t *);
RPC_STATUS RPC_ENTRY
 MesEncodeDynBufferHandleCreate(char **,ULONG *,handle_t *);
RPC_STATUS RPC_ENTRY
 MesDecodeBufferHandleCreate(char *,ULONG,handle_t *);
RPC_STATUS RPC_ENTRY
 MesBufferHandleReset(handle_t,ULONG,MIDL_ES_CODE,char **,ULONG,ULONG *);

RPC_STATUS RPC_ENTRY
 MesHandleFree(handle_t);

RPC_STATUS RPC_ENTRY
 MesInqProcEncodingId(handle_t,PRPC_SYNTAX_IDENTIFIER,ULONG *);

SIZE_T RPC_ENTRY
 NdrMesSimpleTypeAlignSize(handle_t);
void RPC_ENTRY
 NdrMesSimpleTypeDecode(handle_t,void *,short);
void RPC_ENTRY
 NdrMesSimpleTypeEncode(handle_t,const MIDL_STUB_DESC *,const void *,short);

SIZE_T RPC_ENTRY
 NdrMesTypeAlignSize(handle_t,const MIDL_STUB_DESC *,PFORMAT_STRING,const void *);
void RPC_ENTRY
 NdrMesTypeEncode(handle_t,const MIDL_STUB_DESC *,PFORMAT_STRING,const void *);
void RPC_ENTRY
 NdrMesTypeDecode(handle_t,const MIDL_STUB_DESC *,PFORMAT_STRING,void *);

SIZE_T RPC_ENTRY
 NdrMesTypeAlignSize2(handle_t,const MIDL_TYPE_PICKLING_INFO *,const MIDL_STUB_DESC *,PFORMAT_STRING,const void *);
void RPC_ENTRY
 NdrMesTypeEncode2(handle_t,const MIDL_TYPE_PICKLING_INFO *,const MIDL_STUB_DESC *,PFORMAT_STRING,const void *);
void RPC_ENTRY
 NdrMesTypeDecode2(handle_t,const MIDL_TYPE_PICKLING_INFO *,const MIDL_STUB_DESC *,PFORMAT_STRING,void *);
void RPC_ENTRY
 NdrMesTypeFree2(handle_t,const MIDL_TYPE_PICKLING_INFO *,const MIDL_STUB_DESC *,PFORMAT_STRING,void *);

void RPC_VAR_ENTRY
 NdrMesProcEncodeDecode(handle_t,const MIDL_STUB_DESC *,PFORMAT_STRING,...);
CLIENT_CALL_RETURN RPC_VAR_ENTRY
 NdrMesProcEncodeDeocde2(handle_t,const MIDL_STUB_DESC *,PFORMAT_STRING,...);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MIDLES_H__ */
