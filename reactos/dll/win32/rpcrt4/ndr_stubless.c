/*
 * NDR -Oi,-Oif,-Oicf Interpreter
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2003-5 Robert Shearman (for CodeWeavers)
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
 *
 * TODO:
 *  - Pipes
 *  - Some types of binding handles
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "objbase.h"
#include "rpc.h"
#include "rpcproxy.h"
#include "ndrtypes.h"

#include "wine/debug.h"
#include "wine/rpcfc.h"

#include "ndr_misc.h"
#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

#define NDR_TABLE_MASK 127

static inline void call_buffer_sizer(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat)
{
    NDR_BUFFERSIZE m = NdrBufferSizer[pFormat[0] & NDR_TABLE_MASK];
    if (m) m(pStubMsg, pMemory, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
}

static inline unsigned char *call_marshaller(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat)
{
    NDR_MARSHALL m = NdrMarshaller[pFormat[0] & NDR_TABLE_MASK];
    if (m) return m(pStubMsg, pMemory, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
        return NULL;
    }
}

static inline unsigned char *call_unmarshaller(PMIDL_STUB_MESSAGE pStubMsg, unsigned char **ppMemory, PFORMAT_STRING pFormat, unsigned char fMustAlloc)
{
    NDR_UNMARSHALL m = NdrUnmarshaller[pFormat[0] & NDR_TABLE_MASK];
    if (m) return m(pStubMsg, ppMemory, pFormat, fMustAlloc);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
        return NULL;
    }
}

static inline void call_freer(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat)
{
    NDR_FREE m = NdrFreer[pFormat[0] & NDR_TABLE_MASK];
    if (m) m(pStubMsg, pMemory, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
}

static inline unsigned long call_memory_sizer(PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat)
{
    NDR_MEMORYSIZE m = NdrMemorySizer[pFormat[0] & NDR_TABLE_MASK];
    if (m) return m(pStubMsg, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
        return 0;
    }
}

/* there can't be any alignment with the structures in this file */
#include "pshpack1.h"

#define STUBLESS_UNMARSHAL  1
#define STUBLESS_CALLSERVER 2
#define STUBLESS_CALCSIZE   3
#define STUBLESS_GETBUFFER  4
#define STUBLESS_MARSHAL    5

/* From http://msdn.microsoft.com/library/default.asp?url=/library/en-us/rpc/rpc/parameter_descriptors.asp */
typedef struct _NDR_PROC_HEADER
{
    /* type of handle to use:
     * RPC_FC_BIND_EXPLICIT = 0 - Explicit handle.
     *   Handle is passed as a parameter to the function.
     *   Indicates that explicit handle information follows the header,
     *   which actually describes the handle.
     * RPC_FC_BIND_GENERIC = 31 - Implicit handle with custom binding routines
     *   (MIDL_STUB_DESC::IMPLICIT_HANDLE_INFO::pGenericBindingInfo)
     * RPC_FC_BIND_PRIMITIVE = 32 - Implicit handle using handle_t created by
     *   calling application
     * RPC_FC_AUTO_HANDLE = 33 - Automatic handle
     * RPC_FC_CALLBACK_HANDLE = 34 - undocmented
     */
    unsigned char handle_type;

    /* procedure flags:
     * Oi_FULL_PTR_USED = 0x01 - A full pointer can have the value NULL and can
     *   change during the call from NULL to non-NULL and supports aliasing
     *   and cycles. Indicates that the NdrFullPointerXlatInit function
     *   should be called.
     * Oi_RPCSS_ALLOC_USED = 0x02 - Use RpcSS allocate/free routines instead of
     *   normal allocate/free routines
     * Oi_OBJECT_PROC = 0x04 - Indicates a procedure that is part of an OLE
     *   interface, rather than a DCE RPC interface.
     * Oi_HAS_RPCFLAGS = 0x08 - Indicates that the rpc_flags element is 
     *   present in the header.
     * Oi_HAS_COMM_OR_FAULT = 0x20 - If Oi_OBJECT_PROC not present only then
     *   indicates that the procedure has the comm_status or fault_status
     *   MIDL attribute.
     * Oi_OBJ_USE_V2_INTERPRETER = 0x20 - If Oi_OBJECT_PROC present only
     *   then indicates that the format string is in -Oif or -Oicf format
     * Oi_USE_NEW_INIT_ROUTINES = 0x40 - Use NdrXInitializeNew instead of
     *   NdrXInitialize?
     */
    unsigned char Oi_flags;

    /* the zero-based index of the procedure */
    unsigned short proc_num;

    /* total size of all parameters on the stack, including any "this"
     * pointer and/or return value */
    unsigned short stack_size;
} NDR_PROC_HEADER;

/* same as above struct except additional element rpc_flags */
typedef struct _NDR_PROC_HEADER_RPC
{
    unsigned char handle_type;
    unsigned char Oi_flags;

    /*
     * RPCF_Idempotent = 0x0001 - [idempotent] MIDL attribute
     * RPCF_Broadcast = 0x0002 - [broadcast] MIDL attribute
     * RPCF_Maybe = 0x0004 - [maybe] MIDL attribute
     * Reserved = 0x0008 - 0x0080
     * RPCF_Message = 0x0100 - [message] MIDL attribute
     * Reserved = 0x0200 - 0x1000
     * RPCF_InputSynchronous = 0x2000 - unknown
     * RPCF_Asynchronous = 0x4000 - [async] MIDL attribute
     * Reserved = 0x8000
     */
    unsigned long rpc_flags;
    unsigned short proc_num;
    unsigned short stack_size;

} NDR_PROC_HEADER_RPC;

typedef struct _NDR_PROC_PARTIAL_OIF_HEADER
{
    /* the pre-computed client buffer size so that interpreter can skip all
     * or some (if the flag RPC_FC_PROC_OI2F_CLTMUSTSIZE is specified) of the
     * sizing pass */
    unsigned short constant_client_buffer_size;

    /* the pre-computed server buffer size so that interpreter can skip all
     * or some (if the flag RPC_FC_PROC_OI2F_SRVMUSTSIZE is specified) of the
     * sizing pass */
    unsigned short constant_server_buffer_size;

    /* -Oif flags:
     * RPC_FC_PROC_OI2F_SRVMUSTSIZE = 0x01 - the server must perform a
     *   sizing pass.
     * RPC_FC_PROC_OI2F_CLTMUSTSIZE = 0x02 - the client must perform a
     *   sizing pass.
     * RPC_FC_PROC_OI2F_HASRETURN = 0x04 - procedure has a return value.
     * RPC_FC_PROC_OI2F_HASPIPES = 0x08 - the pipe package should be used.
     * RPC_FC_PROC_OI2F_HASASYNCUUID = 0x20 - indicates an asynchronous DCOM
     *   procedure.
     * RPC_FC_PROC_OI2F_HASEXTS = 0x40 - indicates that Windows 2000
     *   extensions are in use.
     * RPC_FC_PROC_OI2F_HASASYNCHND = 0x80 - indicates an asynchronous RPC
     *   procedure.
     */
    unsigned char Oif_flags;

    /* number of params */
    unsigned char number_of_params;
} NDR_PROC_PARTIAL_OIF_HEADER;

/* Windows 2000 extensions */
typedef struct _NDR_PROC_EXTENSION
{
    /* size in bytes of all following extensions */
    unsigned char extension_version;

    /* extension flags:
     * HasNewCorrDesc = 0x01 - indicates new correlation descriptors in use
     * ClientCorrCheck = 0x02 - client needs correlation check
     * ServerCorrCheck = 0x04 - server needs correlation check
     * HasNotify = 0x08 - should call MIDL [notify] routine @ NotifyIndex
     * HasNotify2 = 0x10 - should call MIDL [notify_flag] routine @ 
     *   NotifyIndex
     */
    unsigned char ext_flags;

    /* client cache size hint */
    unsigned short ClientCorrHint;

    /* server cache size hint */
    unsigned short ServerCorrHint;

    /* index of routine in MIDL_STUB_DESC::NotifyRoutineTable to call if
     * HasNotify or HasNotify2 flag set */
    unsigned short NotifyIndex;
} NDR_PROC_EXTENSION;

/* usually generated only on IA64 */
typedef struct _NDR_PROC_EXTENSION_64
{
    NDR_PROC_EXTENSION ext;

    /* needed only on IA64 to cope with float/register loading */
    unsigned short FloatDoubleMask;
} NDR_PROC_EXTENSION_64;


typedef struct _NDR_PARAM_OI_BASETYPE
{
    /* parameter direction. One of:
     * FC_IN_PARAM_BASETYPE = 0x4e - an in param
     * FC_RETURN_PARAM_BASETYPE = 0x53 - a return param
     */
    unsigned char param_direction;

    /* One of: FC_BYTE,FC_CHAR,FC_SMALL,FC_USMALL,FC_WCHAR,FC_SHORT,FC_USHORT,
     * FC_LONG,FC_ULONG,FC_FLOAT,FC_HYPER,FC_DOUBLE,FC_ENUM16,FC_ENUM32,
     * FC_ERROR_STATUS_T,FC_INT3264,FC_UINT3264 */
    unsigned char type_format_char;
} NDR_PARAM_OI_BASETYPE;

typedef struct _NDR_PARAM_OI_OTHER
{
    /* One of:
     * FC_IN_PARAM = 0x4d - An in param
     * FC_IN_OUT_PARAM = 0x50 - An in/out param
     * FC_OUT_PARAM = 0x51 - An out param
     * FC_RETURN_PARAM = 0x52 - A return value
     * FC_IN_PARAM_NO_FREE_INST = 0x4f - A param for which no freeing is done
     */
    unsigned char param_direction;

    /* Size of param on stack in NUMBERS OF INTS */
    unsigned char stack_size;

    /* offset in the type format string table */
    unsigned short type_offset;
} NDR_PARAM_OI_OTHER;

typedef struct _NDR_PARAM_OIF_BASETYPE
{
    PARAM_ATTRIBUTES param_attributes;

    /* the offset on the calling stack where the parameter is located */
    unsigned short stack_offset;

    /* see NDR_PARAM_OI_BASETYPE::type_format_char */
    unsigned char type_format_char;

    /* always FC_PAD */
    unsigned char unused;
} NDR_PARAM_OIF_BASETYPE;

typedef struct _NDR_PARAM_OIF_OTHER
{
    PARAM_ATTRIBUTES param_attributes;

    /* see NDR_PARAM_OIF_BASETYPE::stack_offset */
    unsigned short stack_offset;

    /* offset into the provided type format string where the type for this
     * parameter starts */
    unsigned short type_offset;
} NDR_PARAM_OIF_OTHER;

/* explicit handle description for FC_BIND_PRIMITIVE type */
typedef struct _NDR_EHD_PRIMITIVE
{
    /* FC_BIND_PRIMITIVE */
    unsigned char handle_type;

    /* is the handle passed in via a pointer? */
    unsigned char flag;

    /* offset from the beginning of the stack to the handle in bytes */
    unsigned short offset;
} NDR_EHD_PRIMITIVE;

/* explicit handle description for FC_BIND_GENERIC type */
typedef struct _NDR_EHD_GENERIC
{
    /* FC_BIND_GENERIC */
    unsigned char handle_type;

    /* upper 4bits is a flag indicating whether the handle is passed in
     * via a pointer. lower 4bits is the size of the user defined generic
     * handle type. the size must be less than or equal to the machine
     * register size */
    unsigned char flag_and_size;

    /* offset from the beginning of the stack to the handle in bytes */
    unsigned short offset;

    /* the index into the aGenericBindingRoutinesPairs field of MIDL_STUB_DESC
     * giving the bind and unbind routines for the handle */
    unsigned char binding_routine_pair_index;

    /* FC_PAD */
    unsigned char unused;
} NDR_EHD_GENERIC;

/* explicit handle description for FC_BIND_CONTEXT type */
typedef struct _NDR_EHD_CONTEXT
{
    /* FC_BIND_CONTEXT */
    unsigned char handle_type;

    /* Any of the following flags:
     * NDR_CONTEXT_HANDLE_CANNOT_BE_NULL = 0x01
     * NDR_CONTEXT_HANDLE_SERIALIZE = 0x02
     * NDR_CONTEXT_HANDLE_NO_SERIALIZE = 0x04
     * NDR_STRICT_CONTEXT_HANDLE = 0x08
     * HANDLE_PARAM_IS_OUT = 0x20
     * HANDLE_PARAM_IS_RETURN = 0x21
     * HANDLE_PARAM_IS_IN = 0x40
     * HANDLE_PARAM_IS_VIA_PTR = 0x80
     */
    unsigned char flags;

    /* offset from the beginning of the stack to the handle in bytes */
    unsigned short offset;

    /* zero-based index on rundown routine in apfnNdrRundownRoutines field
     * of MIDL_STUB_DESC */
    unsigned char context_rundown_routine_index;

    /* varies depending on NDR version used.
     * V1: zero-based index into parameters 
     * V2: zero-based index into handles that are parameters */
    unsigned char param_num;
} NDR_EHD_CONTEXT;

#include "poppack.h"

void WINAPI NdrRpcSmSetClientToOsf(PMIDL_STUB_MESSAGE pMessage)
{
#if 0 /* these functions are not defined yet */
    pMessage->pfnAllocate = NdrRpcSmClientAllocate;
    pMessage->pfnFree = NdrRpcSmClientFree;
#endif
}

static void WINAPI dump_RPC_FC_PROC_PF(PARAM_ATTRIBUTES param_attributes)
{
    if (param_attributes.MustSize) TRACE(" MustSize");
    if (param_attributes.MustFree) TRACE(" MustFree");
    if (param_attributes.IsPipe) TRACE(" IsPipe");
    if (param_attributes.IsIn) TRACE(" IsIn");
    if (param_attributes.IsOut) TRACE(" IsOut");
    if (param_attributes.IsReturn) TRACE(" IsReturn");
    if (param_attributes.IsBasetype) TRACE(" IsBasetype");
    if (param_attributes.IsByValue) TRACE(" IsByValue");
    if (param_attributes.IsSimpleRef) TRACE(" IsSimpleRef");
    if (param_attributes.IsDontCallFreeInst) TRACE(" IsDontCallFreeInst");
    if (param_attributes.SaveForAsyncFinish) TRACE(" SaveForAsyncFinish");
    if (param_attributes.ServerAllocSize) TRACE(" ServerAllocSize = %d", param_attributes.ServerAllocSize * 8);
}

/* FIXME: this will be different on other plaftorms than i386 */
#define ARG_FROM_OFFSET(args, offset) (*(unsigned char **)args + offset)

/* the return type should be CLIENT_CALL_RETURN, but this is incompatible
 * with the way gcc returns structures. "void *" should be the largest type
 * that MIDL should allow you to return anyway */
LONG_PTR WINAPIV NdrClientCall2(PMIDL_STUB_DESC pStubDesc, PFORMAT_STRING pFormat, ...)
{
    /* pointer to start of stack where arguments start */
    /* FIXME: not portable */
    unsigned char *args = (unsigned char *)(&pFormat+1);
    RPC_MESSAGE rpcMsg;
    MIDL_STUB_MESSAGE stubMsg;
    handle_t hBinding = NULL;
    /* procedure number */
    unsigned short procedure_number;
    /* size of stack */
    unsigned short stack_size;
    /* current stack offset */
    unsigned short current_stack_offset;
    /* number of parameters. optional for client to give it to us */
    unsigned char number_of_params = ~0;
    /* counter */
    unsigned short i;
    /* cache of Oif_flags from v2 procedure header */
    unsigned char Oif_flags = 0;
    /* cache of extension flags from NDR_PROC_EXTENSION */
    unsigned char ext_flags = 0;
    /* the type of pass we are currently doing */
    int phase;
    /* header for procedure string */
    const NDR_PROC_HEADER * pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];
    /* offset in format string for start of params */
    int parameter_start_offset;
    /* current format string offset */
    int current_offset;
    /* -Oif or -Oicf generated format */
    BOOL bV2Format = FALSE;
    /* the value to return to the client from the remote procedure */
    LONG_PTR RetVal = 0;
    /* the pointer to the object when in OLE mode */
    void * This = NULL;

    TRACE("pStubDesc %p, pFormat %p, ...\n", pStubDesc, pFormat);
    TRACE("&first_argument = %p -> %p\n", args, ARG_FROM_OFFSET(args, 0));

    /* Later NDR language versions probably won't be backwards compatible */
    if (pStubDesc->Version > 0x50002)
    {
        FIXME("Incompatible stub description version: 0x%lx\n", pStubDesc->Version);
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION);
    }

    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCFLAGS)
    {
        NDR_PROC_HEADER_RPC * pProcHeader = (NDR_PROC_HEADER_RPC *)&pFormat[0];
        stack_size = pProcHeader->stack_size;
        procedure_number = pProcHeader->proc_num;
        current_offset = sizeof(NDR_PROC_HEADER_RPC);
    }
    else
    {
        stack_size = pProcHeader->stack_size;
        procedure_number = pProcHeader->proc_num;
        TRACE("proc num: %d\n", procedure_number);
        current_offset = sizeof(NDR_PROC_HEADER);
    }

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);
    TRACE("MIDL stub version = 0x%lx\n", pStubDesc->MIDLVersion);

    /* we only need a handle if this isn't an object method */
    if (!(pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT))
    {
        /* binding */
        switch (pProcHeader->handle_type)
        {
        /* explicit binding: parse additional section */
        case RPC_FC_BIND_EXPLICIT:
            switch (pFormat[current_offset]) /* handle_type */
            {
            case RPC_FC_BIND_PRIMITIVE: /* explicit primitive */
                {
                    NDR_EHD_PRIMITIVE * pDesc = (NDR_EHD_PRIMITIVE *)&pFormat[current_offset];

                    TRACE("Explicit primitive handle @ %d\n", pDesc->offset);

                    if (pDesc->flag) /* pointer to binding */
                        hBinding = **(handle_t **)ARG_FROM_OFFSET(args, pDesc->offset);
                    else
                        hBinding = *(handle_t *)ARG_FROM_OFFSET(args, pDesc->offset);
                    current_offset += sizeof(NDR_EHD_PRIMITIVE);
                    break;
                }
            case RPC_FC_BIND_GENERIC: /* explicit generic */
                FIXME("RPC_FC_BIND_GENERIC\n");
                RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
                current_offset += sizeof(NDR_EHD_GENERIC);
                break;
            case RPC_FC_BIND_CONTEXT: /* explicit context */
                {
                    NDR_EHD_CONTEXT * pDesc = (NDR_EHD_CONTEXT *)&pFormat[current_offset];
                    TRACE("Explicit bind context\n");
                    hBinding = NDRCContextBinding(*(NDR_CCONTEXT *)ARG_FROM_OFFSET(args, pDesc->offset));
                    /* FIXME: should we store this structure in stubMsg.pContext? */
                    current_offset += sizeof(NDR_EHD_CONTEXT);
                    break;
                }
            default:
                ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
                RpcRaiseException(RPC_X_BAD_STUB_DATA);
            }
            break;
        case RPC_FC_BIND_GENERIC: /* implicit generic */
            FIXME("RPC_FC_BIND_GENERIC\n");
            RpcRaiseException(RPC_X_BAD_STUB_DATA); /* FIXME: remove when implemented */
            break;
        case RPC_FC_BIND_PRIMITIVE: /* implicit primitive */
            TRACE("Implicit primitive handle\n");
            hBinding = *pStubDesc->IMPLICIT_HANDLE_INFO.pPrimitiveHandle;
            break;
        case RPC_FC_CALLBACK_HANDLE: /* implicit callback */
            FIXME("RPC_FC_CALLBACK_HANDLE\n");
            break;
        case RPC_FC_AUTO_HANDLE: /* implicit auto handle */
            /* strictly speaking, it isn't necessary to set hBinding here
            * since it isn't actually used (hence the automatic in its name),
            * but then why does MIDL generate a valid entry in the
            * MIDL_STUB_DESC for it? */
            TRACE("Implicit auto handle\n");
            hBinding = *pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle;
            break;
        default:
            ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
    }

    bV2Format = (pStubDesc->Version >= 0x20000);

    if (bV2Format)
    {
        NDR_PROC_PARTIAL_OIF_HEADER * pOIFHeader =
            (NDR_PROC_PARTIAL_OIF_HEADER*)&pFormat[current_offset];

        Oif_flags = pOIFHeader->Oif_flags;
        number_of_params = pOIFHeader->number_of_params;

        current_offset += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);
    }

    TRACE("Oif_flags = 0x%02x\n", Oif_flags);

    if (Oif_flags & RPC_FC_PROC_OI2F_HASEXTS)
    {
        NDR_PROC_EXTENSION * pExtensions =
            (NDR_PROC_EXTENSION *)&pFormat[current_offset];
        ext_flags = pExtensions->ext_flags;
        current_offset += pExtensions->extension_version;
    }

    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
    {
        /* object is always the first argument */
        This = *(void **)ARG_FROM_OFFSET(args, 0);
        NdrProxyInitialize(This, &rpcMsg, &stubMsg, pStubDesc, procedure_number);
    }
    else
        NdrClientInitializeNew(&rpcMsg, &stubMsg, pStubDesc, procedure_number);

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
#if 0
        stubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
#else
        FIXME("initialize full pointer translation tables\n");
#endif

    stubMsg.BufferLength = 0;

    /* store the RPC flags away */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCFLAGS)
        rpcMsg.RpcFlags = ((NDR_PROC_HEADER_RPC *)pProcHeader)->rpc_flags;

    /* use alternate memory allocation routines */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCSSALLOC)
        NdrRpcSmSetClientToOsf(&stubMsg);

    if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
    {
        FIXME("pipes not supported yet\n");
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
        /* init pipes package */
        /* NdrPipesInitialize(...) */
    }
    if (ext_flags & RPC_FC_PROC_EXT_NEWCORRDESC)
    {
        /* initialize extra correlation package */
        FIXME("new correlation description not implemented\n");
        stubMsg.fHasNewCorrDesc = TRUE;
    }

    parameter_start_offset = current_offset;

    /* order of phases:
     * 1. PROXY_CALCSIZE - calculate the buffer size
     * 2. PROXY_GETBUFFER - allocate the buffer
     * 3. PROXY_MARHSAL - marshal [in] params into the buffer
     * 4. PROXY_SENDRECEIVE - send/receive buffer
     * 5. PROXY_UNMARHSAL - unmarshal [out] params from buffer
     */
    for (phase = PROXY_CALCSIZE; phase <= PROXY_UNMARSHAL; phase++)
    {
        TRACE("phase = %d\n", phase);
        switch (phase)
        {
        case PROXY_GETBUFFER:
            /* allocate the buffer */
            if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
                NdrProxyGetBuffer(This, &stubMsg);
            else if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
                /* NdrGetPipeBuffer(...) */
                FIXME("pipes not supported yet\n");
            else
            {
                if (pProcHeader->handle_type == RPC_FC_AUTO_HANDLE)
#if 0
                    NdrNsGetBuffer(&stubMsg, stubMsg.BufferLength, hBinding);
#else
                    FIXME("using auto handle - call NdrNsGetBuffer when it gets implemented\n");
#endif
                else
                    NdrGetBuffer(&stubMsg, stubMsg.BufferLength, hBinding);
            }
            break;
        case PROXY_SENDRECEIVE:
            /* send the [in] params and receive the [out] and [retval]
             * params */
            if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
                NdrProxySendReceive(This, &stubMsg);
            else if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
                /* NdrPipesSendReceive(...) */
                FIXME("pipes not supported yet\n");
            else
            {
                if (pProcHeader->handle_type == RPC_FC_AUTO_HANDLE)
#if 0
                    NdrNsSendReceive(&stubMsg, stubMsg.Buffer, pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
#else
                    FIXME("using auto handle - call NdrNsSendReceive when it gets implemented\n");
#endif
                else
                    NdrSendReceive(&stubMsg, stubMsg.Buffer);
            }

            /* convert strings, floating point values and endianess into our
             * preferred format */
            if ((rpcMsg.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
                NdrConvert(&stubMsg, pFormat);

            break;
        case PROXY_CALCSIZE:
        case PROXY_MARSHAL:
        case PROXY_UNMARSHAL:
            current_offset = parameter_start_offset;
            current_stack_offset = 0;

            /* NOTE: V1 style format does't terminate on the number_of_params
             * condition as it doesn't have this attribute. Instead it
             * terminates when the stack size given in the header is exceeded.
             */
            for (i = 0; i < number_of_params; i++)
            {
                if (bV2Format) /* new parameter format */
                {
                    NDR_PARAM_OIF_BASETYPE * pParam =
                        (NDR_PARAM_OIF_BASETYPE *)&pFormat[current_offset];
                    unsigned char * pArg;

                    current_stack_offset = pParam->stack_offset;
                    pArg = ARG_FROM_OFFSET(args, current_stack_offset);

                    TRACE("param[%d]: new format\n", i);
                    TRACE("\tparam_attributes:"); dump_RPC_FC_PROC_PF(pParam->param_attributes); TRACE("\n");
                    TRACE("\tstack_offset: 0x%x\n", current_stack_offset);
                    TRACE("\tmemory addr (before): %p\n", pArg);

                    if (pParam->param_attributes.IsBasetype)
                    {
                        const unsigned char * pTypeFormat =
                            &pParam->type_format_char;

                        if (pParam->param_attributes.IsSimpleRef)
                            pArg = *(unsigned char **)pArg;

                        TRACE("\tbase type: 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case PROXY_CALCSIZE:
                            if (pParam->param_attributes.IsIn)
                                call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_MARSHAL:
                            if (pParam->param_attributes.IsIn)
                                call_marshaller(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_UNMARSHAL:
                            if (pParam->param_attributes.IsOut)
                            {
                                unsigned char *pRetVal = (unsigned char *)&RetVal;
                                if (pParam->param_attributes.IsReturn)
                                    call_unmarshaller(&stubMsg, &pRetVal, pTypeFormat, 0);
                                else
                                    call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                                TRACE("pRetVal = %p\n", pRetVal);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_offset += sizeof(NDR_PARAM_OIF_BASETYPE);
                    }
                    else
                    {
                        NDR_PARAM_OIF_OTHER * pParamOther =
                            (NDR_PARAM_OIF_OTHER *)&pFormat[current_offset];

                        const unsigned char * pTypeFormat =
                            &(pStubDesc->pFormatTypes[pParamOther->type_offset]);

                        /* if a simple ref pointer then we have to do the
                         * check for the pointer being non-NULL. */
                        if (pParam->param_attributes.IsSimpleRef)
                        {
                            if (!*(unsigned char **)pArg)
                                RpcRaiseException(RPC_X_NULL_REF_POINTER);
                        }

                        TRACE("\tcomplex type: 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case PROXY_CALCSIZE:
                            if (pParam->param_attributes.IsIn)
                            {
                                if (pParam->param_attributes.IsByValue)
                                    call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                                else
                                    call_buffer_sizer(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                            }
                            break;
                        case PROXY_MARSHAL:
                            if (pParam->param_attributes.IsIn)
                            {
                                if (pParam->param_attributes.IsByValue)
                                    call_marshaller(&stubMsg, pArg, pTypeFormat);
                                else
                                    call_marshaller(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                            }
                            break;
                        case PROXY_UNMARSHAL:
                            if (pParam->param_attributes.IsOut)
                            {
                                unsigned char *pRetVal = (unsigned char *)&RetVal;
                                if (pParam->param_attributes.IsReturn)
                                    call_unmarshaller(&stubMsg, &pRetVal, pTypeFormat, 0);
                                else if (pParam->param_attributes.IsByValue)
                                    call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                                else
                                    call_unmarshaller(&stubMsg, (unsigned char **)pArg, pTypeFormat, 0);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_offset += sizeof(NDR_PARAM_OIF_OTHER);
                    }
                    TRACE("\tmemory addr (after): %p\n", pArg);
                }
                else /* old parameter format */
                {
                    NDR_PARAM_OI_BASETYPE * pParam =
                        (NDR_PARAM_OI_BASETYPE *)&pFormat[current_offset];
                    unsigned char * pArg = ARG_FROM_OFFSET(args, current_stack_offset);

                    /* no more parameters; exit loop */
                    if (current_stack_offset > stack_size)
                        break;

                    TRACE("param[%d]: old format\n", i);
                    TRACE("\tparam_direction: %x\n", pParam->param_direction);
                    TRACE("\tstack_offset: 0x%x\n", current_stack_offset);
                    TRACE("\tmemory addr (before): %p\n", pArg);

                    if (pParam->param_direction == RPC_FC_IN_PARAM_BASETYPE ||
                        pParam->param_direction == RPC_FC_RETURN_PARAM_BASETYPE)
                    {
                        const unsigned char * pTypeFormat =
                            &pParam->type_format_char;

                        TRACE("\tbase type 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case PROXY_CALCSIZE:
                            if (pParam->param_direction == RPC_FC_IN_PARAM_BASETYPE)
                                call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_MARSHAL:
                            if (pParam->param_direction == RPC_FC_IN_PARAM_BASETYPE)
                                call_marshaller(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_UNMARSHAL:
                            if (pParam->param_direction == RPC_FC_RETURN_PARAM_BASETYPE)
                            {
                                if (pParam->param_direction & RPC_FC_RETURN_PARAM)
                                    call_unmarshaller(&stubMsg, (unsigned char **)&RetVal, pTypeFormat, 0);
                                else
                                    call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_stack_offset += call_memory_sizer(&stubMsg, pTypeFormat);
                        current_offset += sizeof(NDR_PARAM_OI_BASETYPE);
                    }
                    else
                    {
                        NDR_PARAM_OI_OTHER * pParamOther = 
                            (NDR_PARAM_OI_OTHER *)&pFormat[current_offset];

                        const unsigned char *pTypeFormat =
                            &pStubDesc->pFormatTypes[pParamOther->type_offset];

                        TRACE("\tcomplex type 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case PROXY_CALCSIZE:
                            if (pParam->param_direction == RPC_FC_IN_PARAM ||
                                pParam->param_direction & RPC_FC_IN_OUT_PARAM)
                                call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_MARSHAL:
                            if (pParam->param_direction == RPC_FC_IN_PARAM ||
                                pParam->param_direction & RPC_FC_IN_OUT_PARAM)
                                call_marshaller(&stubMsg, pArg, pTypeFormat);
                            break;
                        case PROXY_UNMARSHAL:
                            if (pParam->param_direction == RPC_FC_IN_OUT_PARAM ||
                                pParam->param_direction == RPC_FC_OUT_PARAM)
                                 call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                            else if (pParam->param_direction == RPC_FC_RETURN_PARAM)
                                call_unmarshaller(&stubMsg, (unsigned char **)&RetVal, pTypeFormat, 0);
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_stack_offset += pParamOther->stack_size * sizeof(INT);
                        current_offset += sizeof(NDR_PARAM_OI_OTHER);
                    }
                    TRACE("\tmemory addr (after): %p\n", pArg);
                }
            }

            break;
        default:
            ERR("shouldn't reach here. phase %d\n", phase);
            break;
        }
    }

    /* FIXME: unbind the binding handle */

    if (ext_flags & RPC_FC_PROC_EXT_NEWCORRDESC)
    {
        /* free extra correlation package */
        /* NdrCorrelationFree(&stubMsg); */
    }

    if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
    {
        /* NdrPipesDone(...) */
    }

#if 0
    /* free the full pointer translation tables */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
        NdrFullPointerXlatFree(stubMsg.FullPtrXlatTables);
#endif

    /* free marshalling buffer */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
        NdrProxyFreeBuffer(This, &stubMsg);
    else
        NdrFreeBuffer(&stubMsg);

    TRACE("RetVal = 0x%lx\n", RetVal);

    return RetVal;
}

/* calls a function with the specificed arguments, restoring the stack
 * properly afterwards as we don't know the calling convention of the
 * function */
#if defined __i386__ && defined _MSC_VER
__declspec(naked) LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size)
{
    __asm
    {
        push ebp
        push edi            ; Save registers
        push esi
        mov ebp, esp
        mov eax, [ebp+16]   ; Get stack size
        sub esp, eax        ; Make room in stack for arguments
        mov edi, esp
        mov ecx, eax
        mov esi, [ebp+12]
        shr ecx, 2
        cld
        rep movsd           ; Copy dword blocks
        call [ebp+8]        ; Call function
        lea esp, [ebp-8]    ; Restore stack
        pop ebp             ; Restore registers
        pop esi
        pop edi
        ret
    }
}
#elif defined __i386__ && defined __GNUC__
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size);
__ASM_GLOBAL_FUNC(call_server_func,
    "pushl %ebp\n\t"
    "movl %esp, %ebp\n\t"
    "pushl %edi\n\t"            /* Save registers */
    "pushl %esi\n\t"
    "movl 16(%ebp), %eax\n\t"   /* Get stack size */
    "subl %eax, %esp\n\t"       /* Make room in stack for arguments */
    "andl $~15, %esp\n\t"	/* Make sure stack has 16-byte alignment for MacOS X */
    "movl %esp, %edi\n\t"
    "movl %eax, %ecx\n\t"
    "movl 12(%ebp), %esi\n\t"
    "shrl $2, %ecx\n\t"         /* divide by 4 */
    "cld\n\t"
    "rep; movsl\n\t"            /* Copy dword blocks */
    "call *8(%ebp)\n\t"         /* Call function */
    "leal -8(%ebp), %esp\n\t"   /* Restore stack */
    "popl %esi\n\t"             /* Restore registers */
    "popl %edi\n\t"
    "popl %ebp\n\t"
    "ret\n" );
#else
#warning call_server_func not implemented for your architecture
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned short stack_size)
{
    FIXME("Not implemented for your architecture\n");
    return 0;
}
#endif

/* FIXME: need to free some stuff in here too */
long WINAPI NdrStubCall2(
    struct IRpcStubBuffer * pThis,
    struct IRpcChannelBuffer * pChannel,
    PRPC_MESSAGE pRpcMsg,
    unsigned long * pdwStubPhase)
{
    const MIDL_SERVER_INFO *pServerInfo;
    const MIDL_STUB_DESC *pStubDesc;
    PFORMAT_STRING pFormat;
    MIDL_STUB_MESSAGE stubMsg;
    /* pointer to start of stack to pass into stub implementation */
    unsigned char * args;
    /* size of stack */
    unsigned short stack_size;
    /* current stack offset */
    unsigned short current_stack_offset;
    /* number of parameters. optional for client to give it to us */
    unsigned char number_of_params = ~0;
    /* counter */
    unsigned short i;
    /* cache of Oif_flags from v2 procedure header */
    unsigned char Oif_flags = 0;
    /* cache of extension flags from NDR_PROC_EXTENSION */
    unsigned char ext_flags = 0;
    /* the type of pass we are currently doing */
    int phase;
    /* header for procedure string */
    const NDR_PROC_HEADER *pProcHeader;
    /* offset in format string for start of params */
    int parameter_start_offset;
    /* current format string offset */
    int current_offset;
    /* -Oif or -Oicf generated format */
    BOOL bV2Format = FALSE;
    /* the return value (not from this function, but to be put back onto
     * the wire */
    LONG_PTR RetVal = 0;

    TRACE("pThis %p, pChannel %p, pRpcMsg %p, pdwStubPhase %p\n", pThis, pChannel, pRpcMsg, pdwStubPhase);

    if (pThis)
        pServerInfo = CStdStubBuffer_GetServerInfo(pThis);
    else
        pServerInfo = ((RPC_SERVER_INTERFACE *)pRpcMsg->RpcInterfaceInformation)->InterpreterInfo;

    pStubDesc = pServerInfo->pStubDesc;
    pFormat = pServerInfo->ProcString + pServerInfo->FmtStringOffset[pRpcMsg->ProcNum];
    pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];

    /* Later NDR language versions probably won't be backwards compatible */
    if (pStubDesc->Version > 0x50002)
    {
        FIXME("Incompatible stub description version: 0x%lx\n", pStubDesc->Version);
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION);
    }

    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCFLAGS)
    {
        NDR_PROC_HEADER_RPC * pProcHeader = (NDR_PROC_HEADER_RPC *)&pFormat[0];
        stack_size = pProcHeader->stack_size;
        current_offset = sizeof(NDR_PROC_HEADER_RPC);

    }
    else
    {
        stack_size = pProcHeader->stack_size;
        current_offset = sizeof(NDR_PROC_HEADER);
    }

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);

    /* binding */
    switch (pProcHeader->handle_type)
    {
    /* explicit binding: parse additional section */
    case RPC_FC_BIND_EXPLICIT:
        switch (pFormat[current_offset]) /* handle_type */
        {
        case RPC_FC_BIND_PRIMITIVE: /* explicit primitive */
            current_offset += sizeof(NDR_EHD_PRIMITIVE);
            break;
        case RPC_FC_BIND_GENERIC: /* explicit generic */
            current_offset += sizeof(NDR_EHD_GENERIC);
            break;
        case RPC_FC_BIND_CONTEXT: /* explicit context */
            current_offset += sizeof(NDR_EHD_CONTEXT);
            break;
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
        break;
    case RPC_FC_BIND_GENERIC: /* implicit generic */
    case RPC_FC_BIND_PRIMITIVE: /* implicit primitive */
    case RPC_FC_CALLBACK_HANDLE: /* implicit callback */
    case RPC_FC_AUTO_HANDLE: /* implicit auto handle */
        break;
    default:
        ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    bV2Format = (pStubDesc->Version >= 0x20000);

    if (bV2Format)
    {
        NDR_PROC_PARTIAL_OIF_HEADER * pOIFHeader =
            (NDR_PROC_PARTIAL_OIF_HEADER*)&pFormat[current_offset];

        Oif_flags = pOIFHeader->Oif_flags;
        number_of_params = pOIFHeader->number_of_params;

        current_offset += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);
    }

    TRACE("Oif_flags = 0x%02x\n", Oif_flags);

    if (Oif_flags & RPC_FC_PROC_OI2F_HASEXTS)
    {
        NDR_PROC_EXTENSION * pExtensions =
            (NDR_PROC_EXTENSION *)&pFormat[current_offset];
        ext_flags = pExtensions->ext_flags;
        current_offset += pExtensions->extension_version;
    }

    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
        NdrStubInitialize(pRpcMsg, &stubMsg, pStubDesc, pChannel);
    else
        NdrServerInitializeNew(pRpcMsg, &stubMsg, pStubDesc);

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
#if 0
        stubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
#else
        FIXME("initialize full pointer translation tables\n");
#endif

    /* store the RPC flags away */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCFLAGS)
        pRpcMsg->RpcFlags = ((NDR_PROC_HEADER_RPC *)pProcHeader)->rpc_flags;

    /* use alternate memory allocation routines */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_RPCSSALLOC)
#if 0
          NdrRpcSsEnableAllocate(&stubMsg);
#else
          FIXME("Set RPCSS memory allocation routines\n");
#endif

    if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
    {
        FIXME("pipes not supported yet\n");
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
        /* init pipes package */
        /* NdrPipesInitialize(...) */
    }
    if (ext_flags & RPC_FC_PROC_EXT_NEWCORRDESC)
    {
        /* initialize extra correlation package */
        FIXME("new correlation description not implemented\n");
        stubMsg.fHasNewCorrDesc = TRUE;
    }


    /* convert strings, floating point values and endianess into our
     * preferred format */
    if ((pRpcMsg->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
        NdrConvert(&stubMsg, pFormat);

    parameter_start_offset = current_offset;

    TRACE("allocating memory for stack of size %x\n", stack_size);

    args = HeapAlloc(GetProcessHeap(), 0, stack_size);
    ZeroMemory(args, stack_size);

    /* add the implicit This pointer as the first arg to the function if we
     * are calling an object method */
    if (pThis)
        *(void **)args = ((CStdStubBuffer *)pThis)->pvServerObject;

    /* order of phases:
     * 1. STUBLESS_UNMARHSAL - unmarshal [in] params from buffer
     * 2. STUBLESS_CALLSERVER - send/receive buffer
     * 3. STUBLESS_CALCSIZE - get [out] buffer size
     * 4. STUBLESS_GETBUFFER - allocate [out] buffer
     * 5. STUBLESS_MARHSAL - marshal [out] params to buffer
     */
    for (phase = STUBLESS_UNMARSHAL; phase <= STUBLESS_MARSHAL; phase++)
    {
        TRACE("phase = %d\n", phase);
        switch (phase)
        {
        case STUBLESS_CALLSERVER:
            /* call the server function */
            if (pServerInfo->ThunkTable)
            {
                stubMsg.StackTop = args;
                pServerInfo->ThunkTable[pRpcMsg->ProcNum](&stubMsg);
                /* FIXME: RetVal is stored as the last argument - retrieve it */
            }
            else if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
            {
                SERVER_ROUTINE *vtbl = *(SERVER_ROUTINE **)((CStdStubBuffer *)pThis)->pvServerObject;
                RetVal = call_server_func(vtbl[pRpcMsg->ProcNum], args, stack_size);
            }
            else
                RetVal = call_server_func(pServerInfo->DispatchTable[pRpcMsg->ProcNum], args, stack_size);

            TRACE("stub implementation returned %p\n", (void *)RetVal);

            stubMsg.Buffer = NULL;
            stubMsg.BufferLength = 0;

            break;
        case STUBLESS_GETBUFFER:
            if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_OBJECT)
                NdrStubGetBuffer(pThis, pChannel, &stubMsg);
            else
            {
                RPC_STATUS Status;

                pRpcMsg->BufferLength = stubMsg.BufferLength;
                /* allocate buffer for [out] and [ret] params */
                Status = I_RpcGetBuffer(pRpcMsg); 
                if (Status)
                    RpcRaiseException(Status);
                stubMsg.BufferStart = pRpcMsg->Buffer;
                stubMsg.BufferEnd = stubMsg.BufferStart + stubMsg.BufferLength;
                stubMsg.Buffer = stubMsg.BufferStart;
            }
            break;
        case STUBLESS_MARSHAL:
        case STUBLESS_UNMARSHAL:
        case STUBLESS_CALCSIZE:
            current_offset = parameter_start_offset;
            current_stack_offset = 0;

            /* NOTE: V1 style format does't terminate on the number_of_params
             * condition as it doesn't have this attribute. Instead it
             * terminates when the stack size given in the header is exceeded.
             */
            for (i = 0; i < number_of_params; i++)
            {
                if (bV2Format) /* new parameter format */
                {
                    const NDR_PARAM_OIF_BASETYPE *pParam =
                        (NDR_PARAM_OIF_BASETYPE *)&pFormat[current_offset];
                    unsigned char *pArg;

                    current_stack_offset = pParam->stack_offset;
                    pArg = (unsigned char *)(args+current_stack_offset);

                    TRACE("param[%d]: new format\n", i);
                    TRACE("\tparam_attributes:"); dump_RPC_FC_PROC_PF(pParam->param_attributes); TRACE("\n");
                    TRACE("\tstack_offset: %x\n", current_stack_offset);
                    TRACE("\tmemory addr (before): %p -> %p\n", pArg, *(unsigned char **)pArg);

                    if (pParam->param_attributes.ServerAllocSize)
                        FIXME("ServerAllocSize of %d ignored for parameter %d\n",
                            pParam->param_attributes.ServerAllocSize * 8, i);

                    if (pParam->param_attributes.IsBasetype)
                    {
                        const unsigned char *pTypeFormat =
                            &pParam->type_format_char;

                        TRACE("\tbase type: 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case STUBLESS_MARSHAL:
                            if (pParam->param_attributes.IsReturn)
                                call_marshaller(&stubMsg, (unsigned char *)&RetVal, pTypeFormat);
                            else if (pParam->param_attributes.IsOut)
                            {
                                if (pParam->param_attributes.IsSimpleRef)
                                    call_marshaller(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                                else
                                    call_marshaller(&stubMsg, pArg, pTypeFormat);
                            }
                            /* FIXME: call call_freer here */
                            break;
                        case STUBLESS_UNMARSHAL:
                            if (pParam->param_attributes.IsIn)
                            {
                                if (pParam->param_attributes.IsSimpleRef)
                                    call_unmarshaller(&stubMsg, (unsigned char **)pArg, pTypeFormat, 0);
                                else
                                    call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                            }
                            break;
                        case STUBLESS_CALCSIZE:
                            if (pParam->param_attributes.IsReturn)
                                call_buffer_sizer(&stubMsg, (unsigned char *)&RetVal, pTypeFormat);
                            else if (pParam->param_attributes.IsOut)
                            {
                                if (pParam->param_attributes.IsSimpleRef)
                                    call_buffer_sizer(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                                else
                                    call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_offset += sizeof(NDR_PARAM_OIF_BASETYPE);
                    }
                    else
                    {
                        NDR_PARAM_OIF_OTHER * pParamOther =
                            (NDR_PARAM_OIF_OTHER *)&pFormat[current_offset];

                        const unsigned char * pTypeFormat =
                            &(pStubDesc->pFormatTypes[pParamOther->type_offset]);

                        TRACE("\tcomplex type 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case STUBLESS_MARSHAL:
                            if (pParam->param_attributes.IsReturn)
                                call_marshaller(&stubMsg, (unsigned char *)&RetVal, pTypeFormat);
                            else if (pParam->param_attributes.IsOut)
                            {
                                if (pParam->param_attributes.IsByValue)
                                    call_marshaller(&stubMsg, pArg, pTypeFormat);
                                else
                                {
                                    call_marshaller(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                                    stubMsg.pfnFree(*(void **)pArg);
                                }
                            }
                            /* FIXME: call call_freer here for IN types */
                            break;
                        case STUBLESS_UNMARSHAL:
                            if (pParam->param_attributes.IsIn)
                            {
                                if (pParam->param_attributes.IsByValue)
                                    call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                                else
                                    call_unmarshaller(&stubMsg, (unsigned char **)pArg, pTypeFormat, 0);
                            }
                            else if ((pParam->param_attributes.IsOut) && 
                                      !(pParam->param_attributes.IsByValue))
                            {
                                *(void **)pArg = NdrAllocate(&stubMsg, sizeof(void *));
                                **(void ***)pArg = 0;
                            }
                            break;
                        case STUBLESS_CALCSIZE:
                            if (pParam->param_attributes.IsReturn)
                                call_buffer_sizer(&stubMsg, (unsigned char *)&RetVal, pTypeFormat);
                            else if (pParam->param_attributes.IsOut)
                            {
                                if (pParam->param_attributes.IsByValue)
                                    call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                                else
                                    call_buffer_sizer(&stubMsg, *(unsigned char **)pArg, pTypeFormat);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_offset += sizeof(NDR_PARAM_OIF_OTHER);
                    }
                    TRACE("\tmemory addr (after): %p -> %p\n", pArg, *(unsigned char **)pArg);
                }
                else /* old parameter format */
                {
                    NDR_PARAM_OI_BASETYPE *pParam =
                        (NDR_PARAM_OI_BASETYPE *)&pFormat[current_offset];
                    unsigned char *pArg = (unsigned char *)(args+current_stack_offset);

                    /* no more parameters; exit loop */
                    if (current_stack_offset > stack_size)
                        break;

                    TRACE("param[%d]: old format\n\tparam_direction: 0x%x\n", i, pParam->param_direction);

                    if (pParam->param_direction == RPC_FC_IN_PARAM_BASETYPE ||
                        pParam->param_direction == RPC_FC_RETURN_PARAM_BASETYPE)
                    {
                        const unsigned char *pTypeFormat =
                            &pParam->type_format_char;

                        TRACE("\tbase type 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case STUBLESS_MARSHAL:
                            if (pParam->param_direction == RPC_FC_RETURN_PARAM_BASETYPE)
                            {
                                unsigned char *pRetVal = (unsigned char *)&RetVal;
                                call_marshaller(&stubMsg, (unsigned char *)&pRetVal, pTypeFormat);
                            }
                            break;
                        case STUBLESS_UNMARSHAL:
                            if (pParam->param_direction == RPC_FC_IN_PARAM_BASETYPE)
                                call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                            break;
                        case STUBLESS_CALCSIZE:
                            if (pParam->param_direction == RPC_FC_RETURN_PARAM_BASETYPE)
                            {
                                unsigned char * pRetVal = (unsigned char *)&RetVal;
                                call_buffer_sizer(&stubMsg, (unsigned char *)&pRetVal, pTypeFormat);
                            }
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_stack_offset += call_memory_sizer(&stubMsg, pTypeFormat);
                        current_offset += sizeof(NDR_PARAM_OI_BASETYPE);
                    }
                    else
                    {
                        NDR_PARAM_OI_OTHER * pParamOther = 
                            (NDR_PARAM_OI_OTHER *)&pFormat[current_offset];

                        const unsigned char * pTypeFormat =
                            &pStubDesc->pFormatTypes[pParamOther->type_offset];

                        TRACE("\tcomplex type 0x%02x\n", *pTypeFormat);

                        switch (phase)
                        {
                        case STUBLESS_MARSHAL:
                            if (pParam->param_direction == RPC_FC_RETURN_PARAM)
                            {
                                unsigned char *pRetVal = (unsigned char *)&RetVal;
                                call_marshaller(&stubMsg, (unsigned char *)&pRetVal, pTypeFormat);
                            }
                            else if (pParam->param_direction == RPC_FC_OUT_PARAM ||
                                pParam->param_direction == RPC_FC_IN_OUT_PARAM)
                                call_marshaller(&stubMsg, pArg, pTypeFormat);
                            break;
                        case STUBLESS_UNMARSHAL:
                            if (pParam->param_direction == RPC_FC_IN_OUT_PARAM ||
                                pParam->param_direction == RPC_FC_IN_PARAM)
                                call_unmarshaller(&stubMsg, &pArg, pTypeFormat, 0);
                            break;
                        case STUBLESS_CALCSIZE:
                            if (pParam->param_direction == RPC_FC_RETURN_PARAM)
                            {
                                unsigned char * pRetVal = (unsigned char *)&RetVal;
                                call_buffer_sizer(&stubMsg, (unsigned char *)&pRetVal, pTypeFormat);
                            }
                            else if (pParam->param_direction == RPC_FC_OUT_PARAM ||
                                pParam->param_direction == RPC_FC_IN_OUT_PARAM)
                                call_buffer_sizer(&stubMsg, pArg, pTypeFormat);
                            break;
                        default:
                            RpcRaiseException(RPC_S_INTERNAL_ERROR);
                        }

                        current_stack_offset += pParamOther->stack_size * sizeof(INT);
                        current_offset += sizeof(NDR_PARAM_OI_OTHER);
                    }
                }
            }

            break;
        default:
            ERR("shouldn't reach here. phase %d\n", phase);
            break;
        }
    }

    pRpcMsg->BufferLength = (unsigned int)(stubMsg.Buffer - (unsigned char *)pRpcMsg->Buffer);

    if (ext_flags & RPC_FC_PROC_EXT_NEWCORRDESC)
    {
        /* free extra correlation package */
        /* NdrCorrelationFree(&stubMsg); */
    }

    if (Oif_flags & RPC_FC_PROC_OI2F_HASPIPES)
    {
        /* NdrPipesDone(...) */
    }

#if 0
    /* free the full pointer translation tables */
    if (pProcHeader->Oi_flags & RPC_FC_PROC_OIF_FULLPTR)
        NdrFullPointerXlatFree(stubMsg.FullPtrXlatTables);
#endif

    /* free server function stack */
    HeapFree(GetProcessHeap(), 0, args);

    return S_OK;
}

void WINAPI NdrServerCall2(PRPC_MESSAGE pRpcMsg)
{
    DWORD dwPhase;
    NdrStubCall2(NULL, NULL, pRpcMsg, &dwPhase);
}
