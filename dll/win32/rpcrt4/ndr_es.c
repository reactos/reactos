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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "rpc.h"
#include "midles.h"
#include "ndrtypes.h"

#include "ndr_misc.h"
#include "ndr_stubless.h"

#include "wine/debug.h"
#include "wine/rpcfc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static inline void init_MIDL_ES_MESSAGE(MIDL_ES_MESSAGE *pEsMsg)
{
    memset(pEsMsg, 0, sizeof(*pEsMsg));
    /* even if we are unmarshalling, as we don't want pointers to be pointed
     * to buffer memory */
    pEsMsg->StubMsg.IsClient = TRUE;
}

/***********************************************************************
 *            MesEncodeIncrementalHandleCreate [RPCRT4.@]
 */
RPC_STATUS WINAPI MesEncodeIncrementalHandleCreate(
    void *UserState, MIDL_ES_ALLOC AllocFn, MIDL_ES_WRITE WriteFn,
    handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %p, %p, %p)\n", UserState, AllocFn, WriteFn, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_ENCODE;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Alloc = AllocFn;
    pEsMsg->Write = WriteFn;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesDecodeIncrementalHandleCreate [RPCRT4.@]
 */
RPC_STATUS WINAPI MesDecodeIncrementalHandleCreate(
    void *UserState, MIDL_ES_READ ReadFn, handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %p, %p)\n", UserState, ReadFn, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_DECODE;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Read = ReadFn;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesIncrementalHandleReset [RPCRT4.@]
 */
RPC_STATUS WINAPI MesIncrementalHandleReset(
    handle_t Handle, void *UserState, MIDL_ES_ALLOC AllocFn,
    MIDL_ES_WRITE WriteFn, MIDL_ES_READ ReadFn, MIDL_ES_CODE Operation)
{
    MIDL_ES_MESSAGE *pEsMsg = (MIDL_ES_MESSAGE *)Handle;

    TRACE("(%p, %p, %p, %p, %p, %d)\n", Handle, UserState, AllocFn,
        WriteFn, ReadFn, Operation);

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = Operation;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Alloc = AllocFn;
    pEsMsg->Write = WriteFn;
    pEsMsg->Read = ReadFn;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesHandleFree [RPCRT4.@]
 */
RPC_STATUS WINAPI MesHandleFree(handle_t Handle)
{
    TRACE("(%p)\n", Handle);
    HeapFree(GetProcessHeap(), 0, Handle);
    return RPC_S_OK;
}

/***********************************************************************
 *            MesEncodeFixedBufferHandleCreate [RPCRT4.@]
 */
RPC_STATUS RPC_ENTRY MesEncodeFixedBufferHandleCreate(
    char *Buffer, ULONG BufferSize, ULONG *pEncodedSize, handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %d, %p, %p)\n", Buffer, BufferSize, pEncodedSize, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_ENCODE;
    pEsMsg->HandleStyle = MES_FIXED_BUFFER_HANDLE;
    pEsMsg->Buffer = (unsigned char *)Buffer;
    pEsMsg->BufferSize = BufferSize;
    pEsMsg->pEncodedSize = pEncodedSize;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;}

/***********************************************************************
 *            MesDecodeBufferHandleCreate [RPCRT4.@]
 */
RPC_STATUS RPC_ENTRY MesDecodeBufferHandleCreate(
    char *Buffer, ULONG BufferSize, handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %d, %p)\n", Buffer, BufferSize, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_DECODE;
    pEsMsg->HandleStyle = MES_FIXED_BUFFER_HANDLE;
    pEsMsg->Buffer = (unsigned char *)Buffer;
    pEsMsg->BufferSize = BufferSize;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;
}

static void es_data_alloc(MIDL_ES_MESSAGE *pEsMsg, ULONG size)
{
    if (pEsMsg->HandleStyle == MES_INCREMENTAL_HANDLE)
    {
        unsigned int tmpsize = size;
        TRACE("%d with incremental handle\n", size);
        pEsMsg->Alloc(pEsMsg->UserState, (char **)&pEsMsg->StubMsg.Buffer, &tmpsize);
        if (tmpsize < size)
        {
            ERR("not enough bytes allocated - requested %d, got %d\n", size, tmpsize);
            RpcRaiseException(ERROR_OUTOFMEMORY);
        }
    }
    else if (pEsMsg->HandleStyle == MES_FIXED_BUFFER_HANDLE)
    {
        TRACE("%d with fixed buffer handle\n", size);
        pEsMsg->StubMsg.Buffer = pEsMsg->Buffer;
    }
    pEsMsg->StubMsg.RpcMsg->Buffer = pEsMsg->StubMsg.BufferStart = pEsMsg->StubMsg.Buffer;
}

static void es_data_read(MIDL_ES_MESSAGE *pEsMsg, ULONG size)
{
    if (pEsMsg->HandleStyle == MES_INCREMENTAL_HANDLE)
    {
        unsigned int tmpsize = size;
        TRACE("%d from incremental handle\n", size);
        pEsMsg->Read(pEsMsg->UserState, (char **)&pEsMsg->StubMsg.Buffer, &tmpsize);
        if (tmpsize < size)
        {
            ERR("not enough bytes read - requested %d, got %d\n", size, tmpsize);
            RpcRaiseException(ERROR_OUTOFMEMORY);
        }
    }
    else
    {
        TRACE("%d from fixed or dynamic buffer handle\n", size);
        /* FIXME: validate BufferSize? */
        pEsMsg->StubMsg.Buffer = pEsMsg->Buffer;
        pEsMsg->Buffer += size;
        pEsMsg->BufferSize -= size;
    }
    pEsMsg->StubMsg.BufferLength = size;
    pEsMsg->StubMsg.RpcMsg->Buffer = pEsMsg->StubMsg.BufferStart = pEsMsg->StubMsg.Buffer;
    pEsMsg->StubMsg.BufferEnd = pEsMsg->StubMsg.Buffer + size;
}

static void es_data_write(MIDL_ES_MESSAGE *pEsMsg, ULONG size)
{
    if (pEsMsg->HandleStyle == MES_INCREMENTAL_HANDLE)
    {
        TRACE("%d to incremental handle\n", size);
        pEsMsg->Write(pEsMsg->UserState, (char *)pEsMsg->StubMsg.BufferStart, size);
    }
    else
    {
        TRACE("%d to dynamic or fixed buffer handle\n", size);
        *pEsMsg->pEncodedSize += size;
    }
}

static inline ULONG mes_proc_header_buffer_size(void)
{
    return 4 + 2*sizeof(RPC_SYNTAX_IDENTIFIER) + 12;
}

static void mes_proc_header_marshal(MIDL_ES_MESSAGE *pEsMsg)
{
    const RPC_CLIENT_INTERFACE *client_interface = pEsMsg->StubMsg.StubDesc->RpcInterfaceInformation;
    *(WORD *)pEsMsg->StubMsg.Buffer = 0x0101;
    pEsMsg->StubMsg.Buffer += 2;
    *(WORD *)pEsMsg->StubMsg.Buffer = 0xcccc;
    pEsMsg->StubMsg.Buffer += 2;
    memcpy(pEsMsg->StubMsg.Buffer, &client_interface->TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER));
    pEsMsg->StubMsg.Buffer += sizeof(RPC_SYNTAX_IDENTIFIER);
    memcpy(pEsMsg->StubMsg.Buffer, &pEsMsg->InterfaceId, sizeof(RPC_SYNTAX_IDENTIFIER));
    pEsMsg->StubMsg.Buffer += sizeof(RPC_SYNTAX_IDENTIFIER);
    *(DWORD *)pEsMsg->StubMsg.Buffer = pEsMsg->ProcNumber;
    pEsMsg->StubMsg.Buffer += 4;
    *(DWORD *)pEsMsg->StubMsg.Buffer = 0x00000001;
    pEsMsg->StubMsg.Buffer += 4;
    *(DWORD *)pEsMsg->StubMsg.Buffer = pEsMsg->ByteCount;
    pEsMsg->StubMsg.Buffer += 4;
}

static void mes_proc_header_unmarshal(MIDL_ES_MESSAGE *pEsMsg)
{
    const RPC_CLIENT_INTERFACE *client_interface = pEsMsg->StubMsg.StubDesc->RpcInterfaceInformation;

    es_data_read(pEsMsg, mes_proc_header_buffer_size());

    if (*(WORD *)pEsMsg->StubMsg.Buffer != 0x0101)
    {
        FIXME("unknown value at Buffer[0] 0x%04x\n", *(WORD *)pEsMsg->StubMsg.Buffer);
        RpcRaiseException(RPC_X_WRONG_ES_VERSION);
    }
    pEsMsg->StubMsg.Buffer += 2;
    if (*(WORD *)pEsMsg->StubMsg.Buffer != 0xcccc)
        FIXME("unknown value at Buffer[2] 0x%04x\n", *(WORD *)pEsMsg->StubMsg.Buffer);
    pEsMsg->StubMsg.Buffer += 2;
    if (memcmp(pEsMsg->StubMsg.Buffer, &client_interface->TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER)))
    {
        const RPC_SYNTAX_IDENTIFIER *AlienTransferSyntax = (const RPC_SYNTAX_IDENTIFIER *)pEsMsg->StubMsg.Buffer;
        ERR("bad transfer syntax %s {%d.%d}\n", debugstr_guid(&AlienTransferSyntax->SyntaxGUID),
            AlienTransferSyntax->SyntaxVersion.MajorVersion,
            AlienTransferSyntax->SyntaxVersion.MinorVersion);
        RpcRaiseException(RPC_S_UNSUPPORTED_TRANS_SYN);
    }
    pEsMsg->StubMsg.Buffer += sizeof(RPC_SYNTAX_IDENTIFIER);
    memcpy(&pEsMsg->InterfaceId, pEsMsg->StubMsg.Buffer, sizeof(RPC_SYNTAX_IDENTIFIER));
    pEsMsg->StubMsg.Buffer += sizeof(RPC_SYNTAX_IDENTIFIER);
    pEsMsg->ProcNumber = *(DWORD *)pEsMsg->StubMsg.Buffer;
    pEsMsg->StubMsg.Buffer += 4;
    if (*(DWORD *)pEsMsg->StubMsg.Buffer != 0x00000001)
        FIXME("unknown value 0x%08x, expected 0x00000001\n", *(DWORD *)pEsMsg->StubMsg.Buffer);
    pEsMsg->StubMsg.Buffer += 4;
    pEsMsg->ByteCount = *(DWORD *)pEsMsg->StubMsg.Buffer;
    pEsMsg->StubMsg.Buffer += 4;
    if (pEsMsg->ByteCount + mes_proc_header_buffer_size() < pEsMsg->ByteCount)
        RpcRaiseException(RPC_S_INVALID_BOUND);
}

/***********************************************************************
 *            NdrMesProcEncodeDecode [RPCRT4.@]
 */
void WINAPIV NdrMesProcEncodeDecode(handle_t Handle, const MIDL_STUB_DESC * pStubDesc, PFORMAT_STRING pFormat, ...)
{
    /* pointer to start of stack where arguments start */
    RPC_MESSAGE rpcMsg;
    MIDL_ES_MESSAGE *pEsMsg = (MIDL_ES_MESSAGE *)Handle;
    /* size of stack */
    unsigned short stack_size;
    /* header for procedure string */
    const NDR_PROC_HEADER *pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];
    /* the value to return to the client from the remote procedure */
    LONG_PTR RetVal = 0;
    const RPC_CLIENT_INTERFACE *client_interface;

    TRACE("Handle %p, pStubDesc %p, pFormat %p, ...\n", Handle, pStubDesc, pFormat);

    /* Later NDR language versions probably won't be backwards compatible */
    if (pStubDesc->Version > 0x50002)
    {
        FIXME("Incompatible stub description version: 0x%x\n", pStubDesc->Version);
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION);
    }

    client_interface = pStubDesc->RpcInterfaceInformation;
    pEsMsg->InterfaceId = client_interface->InterfaceId;

    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCFLAGS)
    {
        const NDR_PROC_HEADER_RPC *pProcHeader = (const NDR_PROC_HEADER_RPC *)&pFormat[0];
        stack_size = pProcHeader->stack_size;
        pEsMsg->ProcNumber = pProcHeader->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER_RPC);
    }
    else
    {
        stack_size = pProcHeader->stack_size;
        pEsMsg->ProcNumber = pProcHeader->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER);
    }

    if (pProcHeader->handle_type == RPC_FC_BIND_EXPLICIT)
    {
        switch (*pFormat) /* handle_type */
        {
        case RPC_FC_BIND_PRIMITIVE: /* explicit primitive */
            pFormat += sizeof(NDR_EHD_PRIMITIVE);
            break;
        case RPC_FC_BIND_GENERIC: /* explicit generic */
            pFormat += sizeof(NDR_EHD_GENERIC);
            break;
        case RPC_FC_BIND_CONTEXT: /* explicit context */
            pFormat += sizeof(NDR_EHD_CONTEXT);
            break;
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
    }

    TRACE("stack size: 0x%x\n", stack_size);
    TRACE("proc num: %d\n", pEsMsg->ProcNumber);

    memset(&rpcMsg, 0, sizeof(rpcMsg));
    pEsMsg->StubMsg.RpcMsg = &rpcMsg;
    pEsMsg->StubMsg.StubDesc = pStubDesc;
    pEsMsg->StubMsg.pfnAllocate = pStubDesc->pfnAllocate;
    pEsMsg->StubMsg.pfnFree = pStubDesc->pfnFree;

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
        pEsMsg->StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);
    TRACE("stubdesc version = 0x%x\n", pStubDesc->Version);
    TRACE("MIDL stub version = 0x%x\n", pStubDesc->MIDLVersion);

    /* needed for conformance of top-level objects */
#ifdef __i386__
    pEsMsg->StubMsg.StackTop = *(unsigned char **)(&pFormat+1);
#else
# warning Stack not retrieved for your CPU architecture
#endif

    switch (pEsMsg->Operation)
    {
    case MES_ENCODE:
        pEsMsg->StubMsg.BufferLength = mes_proc_header_buffer_size();

        client_do_args_old_format(&pEsMsg->StubMsg, pFormat, PROXY_CALCSIZE,
            pEsMsg->StubMsg.StackTop, stack_size, (unsigned char *)&RetVal,
            FALSE /* object_proc */, TRUE /* ignore_retval */);

        pEsMsg->ByteCount = pEsMsg->StubMsg.BufferLength - mes_proc_header_buffer_size();
        es_data_alloc(pEsMsg, pEsMsg->StubMsg.BufferLength);

        mes_proc_header_marshal(pEsMsg);

        client_do_args_old_format(&pEsMsg->StubMsg, pFormat, PROXY_MARSHAL,
            pEsMsg->StubMsg.StackTop, stack_size, (unsigned char *)&RetVal,
            FALSE /* object_proc */, TRUE /* ignore_retval */);

        es_data_write(pEsMsg, pEsMsg->ByteCount);
        break;
    case MES_DECODE:
        mes_proc_header_unmarshal(pEsMsg);

        es_data_read(pEsMsg, pEsMsg->ByteCount);

        client_do_args_old_format(&pEsMsg->StubMsg, pFormat, PROXY_UNMARSHAL,
            pEsMsg->StubMsg.StackTop, stack_size, (unsigned char *)&RetVal,
            FALSE /* object_proc */, TRUE /* ignore_retval */);
        break;
    default:
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }
    /* free the full pointer translation tables */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
        NdrFullPointerXlatFree(pEsMsg->StubMsg.FullPtrXlatTables);
}
