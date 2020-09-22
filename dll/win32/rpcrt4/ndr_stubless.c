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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO:
 *  - Pipes
 *  - Some types of binding handles
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"
#include "rpc.h"
#include "rpcproxy.h"

#include "wine/exception.h"
#include "wine/asm.h"
#include "wine/debug.h"

#include "cpsf.h"
#include "ndr_misc.h"
#include "ndr_stubless.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

#define NDR_TABLE_MASK 127

static inline BOOL is_oicf_stubdesc(const PMIDL_STUB_DESC pStubDesc)
{
    return pStubDesc->Version >= 0x20000;
}

static inline void call_buffer_sizer(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
                                     const NDR_PARAM_OIF *param)
{
    PFORMAT_STRING pFormat;
    NDR_BUFFERSIZE m;

    if (param->attr.IsBasetype)
    {
        pFormat = &param->u.type_format_char;
        if (param->attr.IsSimpleRef) pMemory = *(unsigned char **)pMemory;
    }
    else
    {
        pFormat = &pStubMsg->StubDesc->pFormatTypes[param->u.type_offset];
        if (!param->attr.IsByValue) pMemory = *(unsigned char **)pMemory;
    }

    m = NdrBufferSizer[pFormat[0] & NDR_TABLE_MASK];
    if (m) m(pStubMsg, pMemory, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
}

static inline unsigned char *call_marshaller(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
                                             const NDR_PARAM_OIF *param)
{
    PFORMAT_STRING pFormat;
    NDR_MARSHALL m;

    if (param->attr.IsBasetype)
    {
        pFormat = &param->u.type_format_char;
        if (param->attr.IsSimpleRef) pMemory = *(unsigned char **)pMemory;
    }
    else
    {
        pFormat = &pStubMsg->StubDesc->pFormatTypes[param->u.type_offset];
        if (!param->attr.IsByValue) pMemory = *(unsigned char **)pMemory;
    }

    m = NdrMarshaller[pFormat[0] & NDR_TABLE_MASK];
    if (m) return m(pStubMsg, pMemory, pFormat);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
        return NULL;
    }
}

static inline unsigned char *call_unmarshaller(PMIDL_STUB_MESSAGE pStubMsg, unsigned char **ppMemory,
                                               const NDR_PARAM_OIF *param, unsigned char fMustAlloc)
{
    PFORMAT_STRING pFormat;
    NDR_UNMARSHALL m;

    if (param->attr.IsBasetype)
    {
        pFormat = &param->u.type_format_char;
        if (param->attr.IsSimpleRef) ppMemory = (unsigned char **)*ppMemory;
    }
    else
    {
        pFormat = &pStubMsg->StubDesc->pFormatTypes[param->u.type_offset];
        if (!param->attr.IsByValue) ppMemory = (unsigned char **)*ppMemory;
    }

    m = NdrUnmarshaller[pFormat[0] & NDR_TABLE_MASK];
    if (m) return m(pStubMsg, ppMemory, pFormat, fMustAlloc);
    else
    {
        FIXME("format type 0x%x not implemented\n", pFormat[0]);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
        return NULL;
    }
}

static inline void call_freer(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
                              const NDR_PARAM_OIF *param)
{
    PFORMAT_STRING pFormat;
    NDR_FREE m;

    if (param->attr.IsBasetype) return;  /* nothing to do */
    pFormat = &pStubMsg->StubDesc->pFormatTypes[param->u.type_offset];
    if (!param->attr.IsByValue) pMemory = *(unsigned char **)pMemory;

    m = NdrFreer[pFormat[0] & NDR_TABLE_MASK];
    if (m) m(pStubMsg, pMemory, pFormat);
}

static DWORD calc_arg_size(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat)
{
    DWORD size;
    switch(*pFormat)
    {
    case FC_RP:
        if (pFormat[1] & FC_SIMPLE_POINTER)
        {
            size = 0;
            break;
        }
        size = calc_arg_size(pStubMsg, &pFormat[2] + *(const SHORT*)&pFormat[2]);
        break;
    case FC_STRUCT:
    case FC_PSTRUCT:
        size = *(const WORD*)(pFormat + 2);
        break;
    case FC_BOGUS_STRUCT:
        size = *(const WORD*)(pFormat + 2);
        if(*(const WORD*)(pFormat + 4))
            FIXME("Unhandled conformant description\n");
        break;
    case FC_CARRAY:
    case FC_CVARRAY:
        size = *(const WORD*)(pFormat + 2);
        ComputeConformance(pStubMsg, NULL, pFormat + 4, 0);
        size *= pStubMsg->MaxCount;
        break;
    case FC_SMFARRAY:
    case FC_SMVARRAY:
        size = *(const WORD*)(pFormat + 2);
        break;
    case FC_LGFARRAY:
    case FC_LGVARRAY:
        size = *(const DWORD*)(pFormat + 2);
        break;
    case FC_BOGUS_ARRAY:
        pFormat = ComputeConformance(pStubMsg, NULL, pFormat + 4, *(const WORD*)&pFormat[2]);
        TRACE("conformance = %ld\n", pStubMsg->MaxCount);
        pFormat = ComputeVariance(pStubMsg, NULL, pFormat, pStubMsg->MaxCount);
        size = ComplexStructSize(pStubMsg, pFormat);
        size *= pStubMsg->MaxCount;
        break;
    case FC_USER_MARSHAL:
        size = *(const WORD*)(pFormat + 4);
        break;
    case FC_CSTRING:
        size = *(const WORD*)(pFormat + 2);
        break;
    case FC_WSTRING:
        size = *(const WORD*)(pFormat + 2) * sizeof(WCHAR);
        break;
    case FC_C_CSTRING:
    case FC_C_WSTRING:
        if (*pFormat == FC_C_CSTRING)
            size = sizeof(CHAR);
        else
            size = sizeof(WCHAR);
        if (pFormat[1] == FC_STRING_SIZED)
            ComputeConformance(pStubMsg, NULL, pFormat + 2, 0);
        else
            pStubMsg->MaxCount = 0;
        size *= pStubMsg->MaxCount;
        break;
    default:
        FIXME("Unhandled type %02x\n", *pFormat);
        /* fallthrough */
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_IP:
        size = sizeof(void *);
        break;
    }
    return size;
}

void WINAPI NdrRpcSmSetClientToOsf(PMIDL_STUB_MESSAGE pMessage)
{
#if 0 /* these functions are not defined yet */
    pMessage->pfnAllocate = NdrRpcSmClientAllocate;
    pMessage->pfnFree = NdrRpcSmClientFree;
#endif
}

static const char *debugstr_PROC_PF(PARAM_ATTRIBUTES param_attributes)
{
    char buffer[160];

    buffer[0] = 0;
    if (param_attributes.MustSize) strcat(buffer, " MustSize");
    if (param_attributes.MustFree) strcat(buffer, " MustFree");
    if (param_attributes.IsPipe) strcat(buffer, " IsPipe");
    if (param_attributes.IsIn) strcat(buffer, " IsIn");
    if (param_attributes.IsOut) strcat(buffer, " IsOut");
    if (param_attributes.IsReturn) strcat(buffer, " IsReturn");
    if (param_attributes.IsBasetype) strcat(buffer, " IsBasetype");
    if (param_attributes.IsByValue) strcat(buffer, " IsByValue");
    if (param_attributes.IsSimpleRef) strcat(buffer, " IsSimpleRef");
    if (param_attributes.IsDontCallFreeInst) strcat(buffer, " IsDontCallFreeInst");
    if (param_attributes.SaveForAsyncFinish) strcat(buffer, " SaveForAsyncFinish");
    if (param_attributes.ServerAllocSize)
        sprintf( buffer + strlen(buffer), " ServerAllocSize = %d", param_attributes.ServerAllocSize * 8);
    return buffer[0] ? wine_dbg_sprintf( "%s", buffer + 1 ) : "";
}

static const char *debugstr_INTERPRETER_OPT_FLAGS(INTERPRETER_OPT_FLAGS Oi2Flags)
{
    char buffer[160];

    buffer[0] = 0;
    if (Oi2Flags.ServerMustSize) strcat(buffer, " ServerMustSize");
    if (Oi2Flags.ClientMustSize) strcat(buffer, " ClientMustSize");
    if (Oi2Flags.HasReturn) strcat(buffer, " HasReturn");
    if (Oi2Flags.HasPipes) strcat(buffer, " HasPipes");
    if (Oi2Flags.Unused) strcat(buffer, " Unused");
    if (Oi2Flags.HasAsyncUuid) strcat(buffer, " HasAsyncUuid");
    if (Oi2Flags.HasExtensions) strcat(buffer, " HasExtensions");
    if (Oi2Flags.HasAsyncHandle) strcat(buffer, " HasAsyncHandle");
    return buffer[0] ? wine_dbg_sprintf( "%s", buffer + 1 ) : "";
}

#define ARG_FROM_OFFSET(args, offset) ((args) + (offset))

static size_t get_handle_desc_size(const NDR_PROC_HEADER *proc_header, PFORMAT_STRING format)
{
    if (!proc_header->handle_type)
    {
        if (*format == FC_BIND_PRIMITIVE)
            return sizeof(NDR_EHD_PRIMITIVE);
        else if (*format == FC_BIND_GENERIC)
            return sizeof(NDR_EHD_GENERIC);
        else if (*format == FC_BIND_CONTEXT)
            return sizeof(NDR_EHD_CONTEXT);
    }
    return 0;
}

static handle_t client_get_handle(const MIDL_STUB_MESSAGE *pStubMsg,
        const NDR_PROC_HEADER *pProcHeader, const PFORMAT_STRING pFormat)
{
    /* binding */
    switch (pProcHeader->handle_type)
    {
    /* explicit binding: parse additional section */
    case 0:
        switch (*pFormat) /* handle_type */
        {
        case FC_BIND_PRIMITIVE: /* explicit primitive */
            {
                const NDR_EHD_PRIMITIVE *pDesc = (const NDR_EHD_PRIMITIVE *)pFormat;

                TRACE("Explicit primitive handle @ %d\n", pDesc->offset);

                if (pDesc->flag) /* pointer to binding */
                    return **(handle_t **)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                else
                    return *(handle_t *)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
            }
        case FC_BIND_GENERIC: /* explicit generic */
            {
                const NDR_EHD_GENERIC *pDesc = (const NDR_EHD_GENERIC *)pFormat;
                void *pObject = NULL;
                void *pArg;
                const GENERIC_BINDING_ROUTINE_PAIR *pGenPair;

                TRACE("Explicit generic binding handle #%d\n", pDesc->binding_routine_pair_index);

                if (pDesc->flag_and_size & HANDLE_PARAM_IS_VIA_PTR)
                    pArg = *(void **)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                else
                    pArg = ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                memcpy(&pObject, pArg, pDesc->flag_and_size & 0xf);
                pGenPair = &pStubMsg->StubDesc->aGenericBindingRoutinePairs[pDesc->binding_routine_pair_index];
                return pGenPair->pfnBind(pObject);
            }
        case FC_BIND_CONTEXT: /* explicit context */
            {
                const NDR_EHD_CONTEXT *pDesc = (const NDR_EHD_CONTEXT *)pFormat;
                NDR_CCONTEXT context_handle;
                TRACE("Explicit bind context\n");
                if (pDesc->flags & HANDLE_PARAM_IS_VIA_PTR)
                {
                    TRACE("\tHANDLE_PARAM_IS_VIA_PTR\n");
                    context_handle = **(NDR_CCONTEXT **)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                }
                else
                    context_handle = *(NDR_CCONTEXT *)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);

                if (context_handle) return NDRCContextBinding(context_handle);
                else if (pDesc->flags & NDR_CONTEXT_HANDLE_CANNOT_BE_NULL)
                {
                    ERR("null context handle isn't allowed\n");
                    RpcRaiseException(RPC_X_SS_IN_NULL_CONTEXT);
                    return NULL;
                }
                /* FIXME: should we store this structure in stubMsg.pContext? */
            }
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
        break;
    case FC_BIND_GENERIC: /* implicit generic */
        FIXME("FC_BIND_GENERIC\n");
        RpcRaiseException(RPC_X_BAD_STUB_DATA); /* FIXME: remove when implemented */
        break;
    case FC_BIND_PRIMITIVE: /* implicit primitive */
        TRACE("Implicit primitive handle\n");
        return *pStubMsg->StubDesc->IMPLICIT_HANDLE_INFO.pPrimitiveHandle;
    case FC_CALLBACK_HANDLE: /* implicit callback */
        TRACE("FC_CALLBACK_HANDLE\n");
        /* server calls callback procedures only in response to remote call, and most recent
           binding handle is used. Calling back to a client can potentially result in another
           callback with different current handle. */
        return I_RpcGetCurrentCallHandle();
    case FC_AUTO_HANDLE: /* implicit auto handle */
        /* strictly speaking, it isn't necessary to set hBinding here
         * since it isn't actually used (hence the automatic in its name),
         * but then why does MIDL generate a valid entry in the
         * MIDL_STUB_DESC for it? */
        TRACE("Implicit auto handle\n");
        return *pStubMsg->StubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle;
    default:
        ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
    return NULL;
}

static void client_free_handle(
    PMIDL_STUB_MESSAGE pStubMsg, const NDR_PROC_HEADER *pProcHeader,
    PFORMAT_STRING pFormat, handle_t hBinding)
{
    /* binding */
    switch (pProcHeader->handle_type)
    {
    /* explicit binding: parse additional section */
    case 0:
        switch (*pFormat) /* handle_type */
        {
        case FC_BIND_GENERIC: /* explicit generic */
            {
                const NDR_EHD_GENERIC *pDesc = (const NDR_EHD_GENERIC *)pFormat;
                void *pObject = NULL;
                void *pArg;
                const GENERIC_BINDING_ROUTINE_PAIR *pGenPair;

                TRACE("Explicit generic binding handle #%d\n", pDesc->binding_routine_pair_index);

                if (pDesc->flag_and_size & HANDLE_PARAM_IS_VIA_PTR)
                    pArg = *(void **)ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                else
                    pArg = ARG_FROM_OFFSET(pStubMsg->StackTop, pDesc->offset);
                memcpy(&pObject, pArg, pDesc->flag_and_size & 0xf);
                pGenPair = &pStubMsg->StubDesc->aGenericBindingRoutinePairs[pDesc->binding_routine_pair_index];
#ifdef __REACTOS__
                if (hBinding) pGenPair->pfnUnbind(pObject, hBinding);
#else
                pGenPair->pfnUnbind(pObject, hBinding);
#endif
                break;
            }
        case FC_BIND_CONTEXT: /* explicit context */
        case FC_BIND_PRIMITIVE: /* explicit primitive */
            break;
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
        break;
    case FC_BIND_GENERIC: /* implicit generic */
        FIXME("FC_BIND_GENERIC\n");
        RpcRaiseException(RPC_X_BAD_STUB_DATA); /* FIXME: remove when implemented */
        break;
    case FC_CALLBACK_HANDLE: /* implicit callback */
    case FC_BIND_PRIMITIVE: /* implicit primitive */
    case FC_AUTO_HANDLE: /* implicit auto handle */
        break;
    default:
        ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
}

static inline BOOL param_needs_alloc( PARAM_ATTRIBUTES attr )
{
    return attr.IsOut && !attr.IsIn && !attr.IsBasetype && !attr.IsByValue;
}

static inline BOOL param_is_out_basetype( PARAM_ATTRIBUTES attr )
{
    return attr.IsOut && !attr.IsIn && attr.IsBasetype && attr.IsSimpleRef;
}

static size_t basetype_arg_size( unsigned char fc )
{
    switch (fc)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        return sizeof(char);
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        return sizeof(short);
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_ERROR_STATUS_T:
        return sizeof(int);
    case FC_FLOAT:
        return sizeof(float);
    case FC_HYPER:
        return sizeof(LONGLONG);
    case FC_DOUBLE:
        return sizeof(double);
    case FC_INT3264:
    case FC_UINT3264:
        return sizeof(INT_PTR);
    default:
        FIXME("Unhandled basetype %#x.\n", fc);
        return 0;
    }
}

void client_do_args( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, enum stubless_phase phase,
                     void **fpu_args, unsigned short number_of_params, unsigned char *pRetVal )
{
    const NDR_PARAM_OIF *params = (const NDR_PARAM_OIF *)pFormat;
    unsigned int i;

    for (i = 0; i < number_of_params; i++)
    {
        unsigned char *pArg = pStubMsg->StackTop + params[i].stack_offset;
        PFORMAT_STRING pTypeFormat = (PFORMAT_STRING)&pStubMsg->StubDesc->pFormatTypes[params[i].u.type_offset];

#ifdef __x86_64__  /* floats are passed as doubles through varargs functions */
        float f;

        if (params[i].attr.IsBasetype &&
            params[i].u.type_format_char == FC_FLOAT &&
            !params[i].attr.IsSimpleRef &&
            !fpu_args)
        {
            f = *(double *)pArg;
            pArg = (unsigned char *)&f;
        }
#endif

        TRACE("param[%d]: %p type %02x %s\n", i, pArg,
              params[i].attr.IsBasetype ? params[i].u.type_format_char : *pTypeFormat,
              debugstr_PROC_PF( params[i].attr ));

        switch (phase)
        {
        case STUBLESS_INITOUT:
            if (*(unsigned char **)pArg)
            {
                if (param_needs_alloc(params[i].attr))
                    memset( *(unsigned char **)pArg, 0, calc_arg_size( pStubMsg, pTypeFormat ));
                else if (param_is_out_basetype(params[i].attr))
                    memset( *(unsigned char **)pArg, 0, basetype_arg_size( params[i].u.type_format_char ));
            }
            break;
        case STUBLESS_CALCSIZE:
            if (params[i].attr.IsSimpleRef && !*(unsigned char **)pArg)
                RpcRaiseException(RPC_X_NULL_REF_POINTER);
            if (params[i].attr.IsIn) call_buffer_sizer(pStubMsg, pArg, &params[i]);
            break;
        case STUBLESS_MARSHAL:
            if (params[i].attr.IsIn) call_marshaller(pStubMsg, pArg, &params[i]);
            break;
        case STUBLESS_UNMARSHAL:
            if (params[i].attr.IsOut)
            {
                if (params[i].attr.IsReturn && pRetVal) pArg = pRetVal;
                call_unmarshaller(pStubMsg, &pArg, &params[i], 0);
            }
            break;
        case STUBLESS_FREE:
            if (!params[i].attr.IsBasetype && params[i].attr.IsOut && !params[i].attr.IsByValue)
                NdrClearOutParameters( pStubMsg, pTypeFormat, *(unsigned char **)pArg );
            break;
        default:
            RpcRaiseException(RPC_S_INTERNAL_ERROR);
        }
    }
}

static unsigned int type_stack_size(unsigned char fc)
{
    switch (fc)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
    case FC_LONG:
    case FC_ULONG:
    case FC_INT3264:
    case FC_UINT3264:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_FLOAT:
    case FC_ERROR_STATUS_T:
    case FC_IGNORE:
        return sizeof(void *);
    case FC_DOUBLE:
        return sizeof(double);
    case FC_HYPER:
        return sizeof(ULONGLONG);
    default:
        ERR("invalid base type 0x%x\n", fc);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
}

static BOOL is_by_value( PFORMAT_STRING format )
{
    switch (*format)
    {
    case FC_USER_MARSHAL:
    case FC_STRUCT:
    case FC_PSTRUCT:
    case FC_CSTRUCT:
    case FC_CPSTRUCT:
    case FC_CVSTRUCT:
    case FC_BOGUS_STRUCT:
        return TRUE;
    default:
        return FALSE;
    }
}

PFORMAT_STRING convert_old_args( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat,
                                 unsigned int stack_size, BOOL object_proc,
                                 void *buffer, unsigned int size, unsigned int *count )
{
    NDR_PARAM_OIF *args = buffer;
    unsigned int i, stack_offset = object_proc ? sizeof(void *) : 0;

    for (i = 0; stack_offset < stack_size; i++)
    {
        const NDR_PARAM_OI_BASETYPE *param = (const NDR_PARAM_OI_BASETYPE *)pFormat;
        const NDR_PARAM_OI_OTHER *other = (const NDR_PARAM_OI_OTHER *)pFormat;

        if (i + 1 > size / sizeof(*args))
        {
            FIXME( "%u args not supported\n", i );
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
        }

        args[i].stack_offset = stack_offset;
        memset( &args[i].attr, 0, sizeof(args[i].attr) );

        switch (param->param_direction)
        {
        case FC_IN_PARAM_BASETYPE:
            args[i].attr.IsIn = 1;
            args[i].attr.IsBasetype = 1;
            break;
        case FC_RETURN_PARAM_BASETYPE:
            args[i].attr.IsOut = 1;
            args[i].attr.IsReturn = 1;
            args[i].attr.IsBasetype = 1;
            break;
        case FC_IN_PARAM:
            args[i].attr.IsIn = 1;
            args[i].attr.MustFree = 1;
            break;
        case FC_IN_PARAM_NO_FREE_INST:
            args[i].attr.IsIn = 1;
            args[i].attr.IsDontCallFreeInst = 1;
            break;
        case FC_IN_OUT_PARAM:
            args[i].attr.IsIn = 1;
            args[i].attr.IsOut = 1;
            args[i].attr.MustFree = 1;
            break;
        case FC_OUT_PARAM:
            args[i].attr.IsOut = 1;
            break;
        case FC_RETURN_PARAM:
            args[i].attr.IsOut = 1;
            args[i].attr.IsReturn = 1;
            break;
        }
        if (args[i].attr.IsBasetype)
        {
            args[i].u.type_format_char = param->type_format_char;
            stack_offset += type_stack_size( param->type_format_char );
            pFormat += sizeof(NDR_PARAM_OI_BASETYPE);
        }
        else
        {
            args[i].u.type_offset = other->type_offset;
            args[i].attr.IsByValue = is_by_value( &pStubMsg->StubDesc->pFormatTypes[other->type_offset] );
            stack_offset += other->stack_size * sizeof(void *);
            pFormat += sizeof(NDR_PARAM_OI_OTHER);
        }
    }
    *count = i;
    return (PFORMAT_STRING)args;
}

struct ndr_client_call_ctx
{
    MIDL_STUB_MESSAGE *stub_msg;
    INTERPRETER_OPT_FLAGS Oif_flags;
    INTERPRETER_OPT_FLAGS2 ext_flags;
    const NDR_PROC_HEADER *proc_header;
    void *This;
    PFORMAT_STRING handle_format;
    handle_t hbinding;
};

static void CALLBACK ndr_client_call_finally(BOOL normal, void *arg)
{
    struct ndr_client_call_ctx *ctx = arg;

    if (ctx->ext_flags.HasNewCorrDesc)
    {
        /* free extra correlation package */
        NdrCorrelationFree(ctx->stub_msg);
    }

    if (ctx->Oif_flags.HasPipes)
    {
        /* NdrPipesDone(...) */
    }

    /* free the full pointer translation tables */
    if (ctx->proc_header->Oi_flags & Oi_FULL_PTR_USED)
        NdrFullPointerXlatFree(ctx->stub_msg->FullPtrXlatTables);

    /* free marshalling buffer */
    if (ctx->proc_header->Oi_flags & Oi_OBJECT_PROC)
        NdrProxyFreeBuffer(ctx->This, ctx->stub_msg);
    else
    {
        NdrFreeBuffer(ctx->stub_msg);
        client_free_handle(ctx->stub_msg, ctx->proc_header, ctx->handle_format, ctx->hbinding);
    }
}

/* Helper for ndr_client_call, to factor out the part that may or may not be
 * guarded by a try/except block. */
static LONG_PTR do_ndr_client_call( const MIDL_STUB_DESC *stub_desc, const PFORMAT_STRING format,
        const PFORMAT_STRING handle_format, void **stack_top, void **fpu_stack, MIDL_STUB_MESSAGE *stub_msg,
        unsigned short procedure_number, unsigned short stack_size, unsigned int number_of_params,
        INTERPRETER_OPT_FLAGS Oif_flags, INTERPRETER_OPT_FLAGS2 ext_flags, const NDR_PROC_HEADER *proc_header )
{
    struct ndr_client_call_ctx finally_ctx;
    RPC_MESSAGE rpc_msg;
    handle_t hbinding = NULL;
    /* the value to return to the client from the remote procedure */
    LONG_PTR retval = 0;
    /* the pointer to the object when in OLE mode */
    void *This = NULL;
    /* correlation cache */
    ULONG_PTR NdrCorrCache[256];

    /* create the full pointer translation tables, if requested */
    if (proc_header->Oi_flags & Oi_FULL_PTR_USED)
        stub_msg->FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);

    if (proc_header->Oi_flags & Oi_OBJECT_PROC)
    {
        /* object is always the first argument */
        This = stack_top[0];
        NdrProxyInitialize(This, &rpc_msg, stub_msg, stub_desc, procedure_number);
    }

    finally_ctx.stub_msg = stub_msg;
    finally_ctx.Oif_flags = Oif_flags;
    finally_ctx.ext_flags = ext_flags;
    finally_ctx.proc_header = proc_header;
    finally_ctx.This = This;
    finally_ctx.handle_format = handle_format;
    finally_ctx.hbinding = hbinding;

    __TRY
    {
        if (!(proc_header->Oi_flags & Oi_OBJECT_PROC))
            NdrClientInitializeNew(&rpc_msg, stub_msg, stub_desc, procedure_number);

        stub_msg->StackTop = (unsigned char *)stack_top;

        /* we only need a handle if this isn't an object method */
        if (!(proc_header->Oi_flags & Oi_OBJECT_PROC))
        {
            hbinding = client_get_handle(stub_msg, proc_header, handle_format);
            if (!hbinding) return 0;
        }

        stub_msg->BufferLength = 0;

        /* store the RPC flags away */
        if (proc_header->Oi_flags & Oi_HAS_RPCFLAGS)
            rpc_msg.RpcFlags = ((const NDR_PROC_HEADER_RPC *)proc_header)->rpc_flags;

        /* use alternate memory allocation routines */
        if (proc_header->Oi_flags & Oi_RPCSS_ALLOC_USED)
            NdrRpcSmSetClientToOsf(stub_msg);

        if (Oif_flags.HasPipes)
        {
            FIXME("pipes not supported yet\n");
            RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
            /* init pipes package */
            /* NdrPipesInitialize(...) */
        }
        if (ext_flags.HasNewCorrDesc)
        {
            /* initialize extra correlation package */
            NdrCorrelationInitialize(stub_msg, NdrCorrCache, sizeof(NdrCorrCache), 0);
            if (ext_flags.Unused & 0x2) /* has range on conformance */
                stub_msg->CorrDespIncrement = 12;
        }

        /* order of phases:
         * 1. INITOUT - zero [out] parameters (proxies only)
         * 2. CALCSIZE - calculate the buffer size
         * 3. GETBUFFER - allocate the buffer
         * 4. MARSHAL - marshal [in] params into the buffer
         * 5. SENDRECEIVE - send/receive buffer
         * 6. UNMARSHAL - unmarshal [out] params from buffer
         * 7. FREE - clear [out] parameters (for proxies, and only on error)
         */

        /* 1. INITOUT */
        if (proc_header->Oi_flags & Oi_OBJECT_PROC)
        {
            TRACE( "INITOUT\n" );
            client_do_args(stub_msg, format, STUBLESS_INITOUT, fpu_stack,
                           number_of_params, (unsigned char *)&retval);
        }

        /* 2. CALCSIZE */
        TRACE( "CALCSIZE\n" );
        client_do_args(stub_msg, format, STUBLESS_CALCSIZE, fpu_stack,
                       number_of_params, (unsigned char *)&retval);

        /* 3. GETBUFFER */
        TRACE( "GETBUFFER\n" );
        if (proc_header->Oi_flags & Oi_OBJECT_PROC)
            NdrProxyGetBuffer(This, stub_msg);
        else if (Oif_flags.HasPipes)
            FIXME("pipes not supported yet\n");
        else if (proc_header->handle_type == FC_AUTO_HANDLE)
#if 0
            NdrNsGetBuffer(stub_msg, stub_msg->BufferLength, hBinding);
#else
            FIXME("using auto handle - call NdrNsGetBuffer when it gets implemented\n");
#endif
        else
            NdrGetBuffer(stub_msg, stub_msg->BufferLength, hbinding);

        /* 4. MARSHAL */
        TRACE( "MARSHAL\n" );
        client_do_args(stub_msg, format, STUBLESS_MARSHAL, fpu_stack,
                       number_of_params, (unsigned char *)&retval);

        /* 5. SENDRECEIVE */
        TRACE( "SENDRECEIVE\n" );
        if (proc_header->Oi_flags & Oi_OBJECT_PROC)
            NdrProxySendReceive(This, stub_msg);
        else if (Oif_flags.HasPipes)
            /* NdrPipesSendReceive(...) */
            FIXME("pipes not supported yet\n");
        else if (proc_header->handle_type == FC_AUTO_HANDLE)
#if 0
            NdrNsSendReceive(stub_msg, stub_msg->Buffer, pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
#else
            FIXME("using auto handle - call NdrNsSendReceive when it gets implemented\n");
#endif
        else
            NdrSendReceive(stub_msg, stub_msg->Buffer);

        /* convert strings, floating point values and endianness into our
         * preferred format */
        if ((rpc_msg.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
            NdrConvert(stub_msg, format);

        /* 6. UNMARSHAL */
        TRACE( "UNMARSHAL\n" );
        client_do_args(stub_msg, format, STUBLESS_UNMARSHAL, fpu_stack,
                       number_of_params, (unsigned char *)&retval);
    }
    __FINALLY_CTX(ndr_client_call_finally, &finally_ctx)

    return retval;
}

LONG_PTR CDECL DECLSPEC_HIDDEN ndr_client_call( PMIDL_STUB_DESC pStubDesc, PFORMAT_STRING pFormat,
                                                void **stack_top, void **fpu_stack )
{
    /* pointer to start of stack where arguments start */
    MIDL_STUB_MESSAGE stubMsg;
    /* procedure number */
    unsigned short procedure_number;
    /* size of stack */
    unsigned short stack_size;
    /* number of parameters. optional for client to give it to us */
    unsigned int number_of_params;
    /* cache of Oif_flags from v2 procedure header */
    INTERPRETER_OPT_FLAGS Oif_flags = { 0 };
    /* cache of extension flags from NDR_PROC_HEADER_EXTS */
    INTERPRETER_OPT_FLAGS2 ext_flags = { 0 };
    /* header for procedure string */
    const NDR_PROC_HEADER * pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];
    /* the value to return to the client from the remote procedure */
    LONG_PTR RetVal = 0;
    PFORMAT_STRING pHandleFormat;
    NDR_PARAM_OIF old_args[256];

    TRACE("pStubDesc %p, pFormat %p, ...\n", pStubDesc, pFormat);

    TRACE("NDR Version: 0x%x\n", pStubDesc->Version);

    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
    {
        const NDR_PROC_HEADER_RPC *header_rpc = (const NDR_PROC_HEADER_RPC *)&pFormat[0];
        stack_size = header_rpc->stack_size;
        procedure_number = header_rpc->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER_RPC);
    }
    else
    {
        stack_size = pProcHeader->stack_size;
        procedure_number = pProcHeader->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER);
    }
    TRACE("stack size: 0x%x\n", stack_size);
    TRACE("proc num: %d\n", procedure_number);
    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);
    TRACE("MIDL stub version = 0x%x\n", pStubDesc->MIDLVersion);

    pHandleFormat = pFormat;

    /* we only need a handle if this isn't an object method */
    if (!(pProcHeader->Oi_flags & Oi_OBJECT_PROC))
        pFormat += get_handle_desc_size(pProcHeader, pFormat);

    if (is_oicf_stubdesc(pStubDesc))  /* -Oicf format */
    {
        const NDR_PROC_PARTIAL_OIF_HEADER *pOIFHeader =
            (const NDR_PROC_PARTIAL_OIF_HEADER *)pFormat;

        Oif_flags = pOIFHeader->Oi2Flags;
        number_of_params = pOIFHeader->number_of_params;

        pFormat += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);

        TRACE("Oif_flags = %s\n", debugstr_INTERPRETER_OPT_FLAGS(Oif_flags) );

        if (Oif_flags.HasExtensions)
        {
            const NDR_PROC_HEADER_EXTS *pExtensions = (const NDR_PROC_HEADER_EXTS *)pFormat;
            ext_flags = pExtensions->Flags2;
            pFormat += pExtensions->Size;
#ifdef __x86_64__
            if (pExtensions->Size > sizeof(*pExtensions) && fpu_stack)
            {
                int i;
                unsigned short fpu_mask = *(unsigned short *)(pExtensions + 1);
                for (i = 0; i < 4; i++, fpu_mask >>= 2)
                    switch (fpu_mask & 3)
                    {
                    case 1: *(float *)&stack_top[i] = *(float *)&fpu_stack[i]; break;
                    case 2: *(double *)&stack_top[i] = *(double *)&fpu_stack[i]; break;
                    }
            }
#endif
        }
    }
    else
    {
        pFormat = convert_old_args( &stubMsg, pFormat, stack_size,
                                    pProcHeader->Oi_flags & Oi_OBJECT_PROC,
                                    old_args, sizeof(old_args), &number_of_params );
    }

    if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
    {
        __TRY
        {
            RetVal = do_ndr_client_call(pStubDesc, pFormat, pHandleFormat,
                    stack_top, fpu_stack, &stubMsg, procedure_number, stack_size,
                    number_of_params, Oif_flags, ext_flags, pProcHeader);
        }
        __EXCEPT_ALL
        {
            /* 7. FREE */
            TRACE( "FREE\n" );
            client_do_args(&stubMsg, pFormat, STUBLESS_FREE, fpu_stack,
                           number_of_params, (unsigned char *)&RetVal);
            RetVal = NdrProxyErrorHandler(GetExceptionCode());
        }
        __ENDTRY
    }
    else if (pProcHeader->Oi_flags & Oi_HAS_COMM_OR_FAULT)
    {
        __TRY
        {
            RetVal = do_ndr_client_call(pStubDesc, pFormat, pHandleFormat,
                    stack_top, fpu_stack, &stubMsg, procedure_number, stack_size,
                    number_of_params, Oif_flags, ext_flags, pProcHeader);
        }
        __EXCEPT_ALL
        {
            const COMM_FAULT_OFFSETS *comm_fault_offsets = &pStubDesc->CommFaultOffsets[procedure_number];
            ULONG *comm_status;
            ULONG *fault_status;

            TRACE("comm_fault_offsets = {0x%hx, 0x%hx}\n", comm_fault_offsets->CommOffset, comm_fault_offsets->FaultOffset);

            if (comm_fault_offsets->CommOffset == -1)
                comm_status = (ULONG *)&RetVal;
            else if (comm_fault_offsets->CommOffset >= 0)
                comm_status = *(ULONG **)ARG_FROM_OFFSET(stubMsg.StackTop, comm_fault_offsets->CommOffset);
            else
                comm_status = NULL;

            if (comm_fault_offsets->FaultOffset == -1)
                fault_status = (ULONG *)&RetVal;
            else if (comm_fault_offsets->FaultOffset >= 0)
                fault_status = *(ULONG **)ARG_FROM_OFFSET(stubMsg.StackTop, comm_fault_offsets->FaultOffset);
            else
                fault_status = NULL;

            NdrMapCommAndFaultStatus(&stubMsg, comm_status, fault_status,
                                     GetExceptionCode());
        }
        __ENDTRY
    }
    else
    {
        RetVal = do_ndr_client_call(pStubDesc, pFormat, pHandleFormat,
                stack_top, fpu_stack, &stubMsg, procedure_number, stack_size,
                number_of_params, Oif_flags, ext_flags, pProcHeader);
    }

    TRACE("RetVal = 0x%lx\n", RetVal);
    return RetVal;
}

#ifdef __x86_64__

__ASM_GLOBAL_FUNC( NdrClientCall2,
                   "movq %r8,0x18(%rsp)\n\t"
                   "movq %r9,0x20(%rsp)\n\t"
                   "leaq 0x18(%rsp),%r8\n\t"
                   "xorq %r9,%r9\n\t"
                   "subq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   "call " __ASM_NAME("ndr_client_call") "\n\t"
                   "addq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -0x28\n\t")
                   "ret" );

#else  /* __x86_64__ */

/***********************************************************************
 *            NdrClientCall2 [RPCRT4.@]
 */
CLIENT_CALL_RETURN WINAPIV NdrClientCall2( PMIDL_STUB_DESC desc, PFORMAT_STRING format, ... )
{
    __ms_va_list args;
    LONG_PTR ret;

    __ms_va_start( args, format );
    ret = ndr_client_call( desc, format, va_arg( args, void ** ), NULL );
    __ms_va_end( args );
    return *(CLIENT_CALL_RETURN *)&ret;
}

#endif  /* __x86_64__ */

/* Calls a function with the specified arguments, restoring the stack
 * properly afterwards as we don't know the calling convention of the
 * function */
#if defined __i386__ && defined _MSC_VER
__declspec(naked) LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size)
{
    __asm
    {
        push ebp
        mov ebp, esp
        push edi            ; Save registers
        push esi
        mov eax, [ebp+16]   ; Get stack size
        sub esp, eax        ; Make room in stack for arguments
        and esp, 0xFFFFFFF0
        mov edi, esp
        mov ecx, eax
        mov esi, [ebp+12]
        shr ecx, 2
        cld
        rep movsd           ; Copy dword blocks
        call [ebp+8]        ; Call function
        lea esp, [ebp-8]    ; Restore stack
        pop esi             ; Restore registers
        pop edi
        pop ebp
        ret
    }
}
#elif defined __i386__ && defined __GNUC__
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size);
__ASM_GLOBAL_FUNC(call_server_func,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "pushl %edi\n\t"            /* Save registers */
    __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
    "pushl %esi\n\t"
    __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
    "movl 16(%ebp), %eax\n\t"   /* Get stack size */
    "subl %eax, %esp\n\t"       /* Make room in stack for arguments */
    "andl $~15, %esp\n\t"	/* Make sure stack has 16-byte alignment for Mac OS X */
    "movl %esp, %edi\n\t"
    "movl %eax, %ecx\n\t"
    "movl 12(%ebp), %esi\n\t"
    "shrl $2, %ecx\n\t"         /* divide by 4 */
    "cld\n\t"
    "rep; movsl\n\t"            /* Copy dword blocks */
    "call *8(%ebp)\n\t"         /* Call function */
    "leal -8(%ebp), %esp\n\t"   /* Restore stack */
    "popl %esi\n\t"             /* Restore registers */
    __ASM_CFI(".cfi_same_value %esi\n\t")
    "popl %edi\n\t"
    __ASM_CFI(".cfi_same_value %edi\n\t")
    "popl %ebp\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#elif defined __x86_64__
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size);
__ASM_GLOBAL_FUNC( call_server_func,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_SEH(".seh_pushreg %rsi\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_SEH(".seh_pushreg %rdi\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   "movq %rcx,%rax\n\t"   /* function to call */
                   "movq $32,%rcx\n\t"    /* allocate max(32,stack_size) bytes of stack space */
                   "cmpq %rcx,%r8\n\t"
                   "cmovgq %r8,%rcx\n\t"
                   "subq %rcx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %r8,%rcx\n\t"
                   "shrq $3,%rcx\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %rdx,%rsi\n\t"
                   "rep; movsq\n\t"       /* copy arguments */
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "movq 0(%rsp),%xmm0\n\t"
                   "movq 8(%rsp),%xmm1\n\t"
                   "movq 16(%rsp),%xmm2\n\t"
                   "movq 24(%rsp),%xmm3\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"  /* restore stack */
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")
#elif defined __arm__
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char *args, unsigned int stack_size);
__ASM_GLOBAL_FUNC( call_server_func,
                   ".arm\n\t"
                   "push {r4, r5, LR}\n\t"
                   "mov r4, r0\n\t"
                   "mov r5, SP\n\t"
                   "lsr r3, r2, #2\n\t"
                   "cmp r3, #0\n\t"
                   "beq 5f\n\t"
                   "sub SP, SP, r2\n\t"
                   "tst r3, #1\n\t"
                   "subeq SP, SP, #4\n\t"
                   "1:\tsub r2, r2, #4\n\t"
                   "ldr r0, [r1, r2]\n\t"
                   "str r0, [SP, r2]\n\t"
                   "cmp r2, #0\n\t"
                   "bgt 1b\n\t"
                   "cmp r3, #1\n\t"
                   "bgt 2f\n\t"
                   "pop {r0}\n\t"
                   "b 5f\n\t"
                   "2:\tcmp r3, #2\n\t"
                   "bgt 3f\n\t"
                   "pop {r0-r1}\n\t"
                   "b 5f\n\t"
                   "3:\tcmp r3, #3\n\t"
                   "bgt 4f\n\t"
                   "pop {r0-r2}\n\t"
                   "b 5f\n\t"
                   "4:\tpop {r0-r3}\n\t"
                   "5:\tblx r4\n\t"
                   "mov SP, r5\n\t"
                   "pop {r4, r5, PC}" )
#elif defined __aarch64__
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char *args, unsigned int stack_size);
__ASM_GLOBAL_FUNC( call_server_func,
                   "stp x29, x30, [sp, #-16]!\n\t"
                   "mov x29, sp\n\t"
                   "add x3, x2, #15\n\t"
                   "lsr x3, x3, #4\n\t"
                   "sub sp, sp, x3, lsl #4\n\t"
                   "cbz x2, 2f\n"
                   "1:\tsub x2, x2, #8\n\t"
                   "ldr x4, [x1, x2]\n\t"
                   "str x4, [sp, x2]\n\t"
                   "cbnz x2, 1b\n"
                   "2:\tmov x8, x0\n\t"
                   "cbz x3, 3f\n\t"
                   "ldp x0, x1, [sp], #16\n\t"
                   "cmp x3, #1\n\t"
                   "b.le 3f\n\t"
                   "ldp x2, x3, [sp], #16\n\t"
                   "cmp x3, #2\n\t"
                   "b.le 3f\n\t"
                   "ldp x4, x5, [sp], #16\n\t"
                   "cmp x3, #3\n\t"
                   "b.le 3f\n\t"
                   "ldp x6, x7, [sp], #16\n"
                   "3:\tblr x8\n\t"
                   "mov sp, x29\n\t"
                   "ldp x29, x30, [sp], #16\n\t"
                   "ret" )
#else
#warning call_server_func not implemented for your architecture
LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned short stack_size)
{
    FIXME("Not implemented for your architecture\n");
    return 0;
}
#endif

static LONG_PTR *stub_do_args(MIDL_STUB_MESSAGE *pStubMsg,
                              PFORMAT_STRING pFormat, enum stubless_phase phase,
                              unsigned short number_of_params)
{
    const NDR_PARAM_OIF *params = (const NDR_PARAM_OIF *)pFormat;
    unsigned int i;
    LONG_PTR *retval_ptr = NULL;

    for (i = 0; i < number_of_params; i++)
    {
        unsigned char *pArg = pStubMsg->StackTop + params[i].stack_offset;
        const unsigned char *pTypeFormat = &pStubMsg->StubDesc->pFormatTypes[params[i].u.type_offset];

        TRACE("param[%d]: %p -> %p type %02x %s\n", i,
              pArg, *(unsigned char **)pArg,
              params[i].attr.IsBasetype ? params[i].u.type_format_char : *pTypeFormat,
              debugstr_PROC_PF( params[i].attr ));

        switch (phase)
        {
        case STUBLESS_MARSHAL:
            if (params[i].attr.IsOut || params[i].attr.IsReturn)
                call_marshaller(pStubMsg, pArg, &params[i]);
            break;
        case STUBLESS_MUSTFREE:
            if (params[i].attr.MustFree)
            {
                call_freer(pStubMsg, pArg, &params[i]);
            }
            break;
        case STUBLESS_FREE:
            if (params[i].attr.ServerAllocSize)
            {
                HeapFree(GetProcessHeap(), 0, *(void **)pArg);
            }
            else if (param_needs_alloc(params[i].attr) &&
                     (!params[i].attr.MustFree || params[i].attr.IsSimpleRef))
            {
                if (*pTypeFormat != FC_BIND_CONTEXT) pStubMsg->pfnFree(*(void **)pArg);
            }
            break;
        case STUBLESS_INITOUT:
            if (param_needs_alloc(params[i].attr) && !params[i].attr.ServerAllocSize)
            {
                if (*pTypeFormat == FC_BIND_CONTEXT)
                {
                    NDR_SCONTEXT ctxt = NdrContextHandleInitialize(pStubMsg, pTypeFormat);
                    *(void **)pArg = NDRSContextValue(ctxt);
                    if (params[i].attr.IsReturn) retval_ptr = (LONG_PTR *)NDRSContextValue(ctxt);
                }
                else
                {
                    DWORD size = calc_arg_size(pStubMsg, pTypeFormat);
                    if (size)
                    {
                        *(void **)pArg = NdrAllocate(pStubMsg, size);
                        memset(*(void **)pArg, 0, size);
                    }
                }
            }
            if (!retval_ptr && params[i].attr.IsReturn) retval_ptr = (LONG_PTR *)pArg;
            break;
        case STUBLESS_UNMARSHAL:
            if (params[i].attr.ServerAllocSize)
                *(void **)pArg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           params[i].attr.ServerAllocSize * 8);

            if (params[i].attr.IsIn)
                call_unmarshaller(pStubMsg, &pArg, &params[i], 0);
            break;
        case STUBLESS_CALCSIZE:
            if (params[i].attr.IsOut || params[i].attr.IsReturn)
                call_buffer_sizer(pStubMsg, pArg, &params[i]);
            break;
        default:
            RpcRaiseException(RPC_S_INTERNAL_ERROR);
        }
        TRACE("\tmemory addr (after): %p -> %p\n", pArg, *(unsigned char **)pArg);
    }
    return retval_ptr;
}

/***********************************************************************
 *            NdrStubCall2 [RPCRT4.@]
 *
 * Unmarshals [in] parameters, calls either a method in an object or a server
 * function, marshals any [out] parameters and frees any allocated data.
 *
 * NOTES
 *  Used by stubless MIDL-generated code.
 */
LONG WINAPI NdrStubCall2(
    struct IRpcStubBuffer * pThis,
    struct IRpcChannelBuffer * pChannel,
    PRPC_MESSAGE pRpcMsg,
    DWORD * pdwStubPhase)
{
    const MIDL_SERVER_INFO *pServerInfo;
    const MIDL_STUB_DESC *pStubDesc;
    PFORMAT_STRING pFormat;
    MIDL_STUB_MESSAGE stubMsg;
    /* pointer to start of stack to pass into stub implementation */
    unsigned char * args;
    /* size of stack */
    unsigned short stack_size;
    /* number of parameters. optional for client to give it to us */
    unsigned int number_of_params;
    /* cache of Oif_flags from v2 procedure header */
    INTERPRETER_OPT_FLAGS Oif_flags = { 0 };
    /* cache of extension flags from NDR_PROC_HEADER_EXTS */
    INTERPRETER_OPT_FLAGS2 ext_flags = { 0 };
    /* the type of pass we are currently doing */
    enum stubless_phase phase;
    /* header for procedure string */
    const NDR_PROC_HEADER *pProcHeader;
    /* location to put retval into */
    LONG_PTR *retval_ptr = NULL;
    /* correlation cache */
    ULONG_PTR NdrCorrCache[256];

    TRACE("pThis %p, pChannel %p, pRpcMsg %p, pdwStubPhase %p\n", pThis, pChannel, pRpcMsg, pdwStubPhase);

    if (pThis)
        pServerInfo = CStdStubBuffer_GetServerInfo(pThis);
    else
        pServerInfo = ((RPC_SERVER_INTERFACE *)pRpcMsg->RpcInterfaceInformation)->InterpreterInfo;

    pStubDesc = pServerInfo->pStubDesc;
    pFormat = pServerInfo->ProcString + pServerInfo->FmtStringOffset[pRpcMsg->ProcNum];
    pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];

    TRACE("NDR Version: 0x%x\n", pStubDesc->Version);

    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
    {
        const NDR_PROC_HEADER_RPC *header_rpc = (const NDR_PROC_HEADER_RPC *)&pFormat[0];
        stack_size = header_rpc->stack_size;
        pFormat += sizeof(NDR_PROC_HEADER_RPC);

    }
    else
    {
        stack_size = pProcHeader->stack_size;
        pFormat += sizeof(NDR_PROC_HEADER);
    }

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);

    /* binding */
    switch (pProcHeader->handle_type)
    {
    /* explicit binding: parse additional section */
    case 0:
        switch (*pFormat) /* handle_type */
        {
        case FC_BIND_PRIMITIVE: /* explicit primitive */
            pFormat += sizeof(NDR_EHD_PRIMITIVE);
            break;
        case FC_BIND_GENERIC: /* explicit generic */
            pFormat += sizeof(NDR_EHD_GENERIC);
            break;
        case FC_BIND_CONTEXT: /* explicit context */
            pFormat += sizeof(NDR_EHD_CONTEXT);
            break;
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
        break;
    case FC_BIND_GENERIC: /* implicit generic */
    case FC_BIND_PRIMITIVE: /* implicit primitive */
    case FC_CALLBACK_HANDLE: /* implicit callback */
    case FC_AUTO_HANDLE: /* implicit auto handle */
        break;
    default:
        ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
        NdrStubInitialize(pRpcMsg, &stubMsg, pStubDesc, pChannel);
    else
        NdrServerInitializeNew(pRpcMsg, &stubMsg, pStubDesc);

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        stubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);

    /* store the RPC flags away */
    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
        pRpcMsg->RpcFlags = ((const NDR_PROC_HEADER_RPC *)pProcHeader)->rpc_flags;

    /* use alternate memory allocation routines */
    if (pProcHeader->Oi_flags & Oi_RPCSS_ALLOC_USED)
#if 0
          NdrRpcSsEnableAllocate(&stubMsg);
#else
          FIXME("Set RPCSS memory allocation routines\n");
#endif

    TRACE("allocating memory for stack of size %x\n", stack_size);

    args = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stack_size);
    stubMsg.StackTop = args; /* used by conformance of top-level objects */

    /* add the implicit This pointer as the first arg to the function if we
     * are calling an object method */
    if (pThis)
        *(void **)args = ((CStdStubBuffer *)pThis)->pvServerObject;

    if (is_oicf_stubdesc(pStubDesc))
    {
        const NDR_PROC_PARTIAL_OIF_HEADER *pOIFHeader = (const NDR_PROC_PARTIAL_OIF_HEADER *)pFormat;

        Oif_flags = pOIFHeader->Oi2Flags;
        number_of_params = pOIFHeader->number_of_params;

        pFormat += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);

        TRACE("Oif_flags = %s\n", debugstr_INTERPRETER_OPT_FLAGS(Oif_flags) );

        if (Oif_flags.HasExtensions)
        {
            const NDR_PROC_HEADER_EXTS *pExtensions = (const NDR_PROC_HEADER_EXTS *)pFormat;
            ext_flags = pExtensions->Flags2;
            pFormat += pExtensions->Size;
        }

        if (Oif_flags.HasPipes)
        {
            FIXME("pipes not supported yet\n");
            RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
            /* init pipes package */
            /* NdrPipesInitialize(...) */
        }
        if (ext_flags.HasNewCorrDesc)
        {
            /* initialize extra correlation package */
            NdrCorrelationInitialize(&stubMsg, NdrCorrCache, sizeof(NdrCorrCache), 0);
            if (ext_flags.Unused & 0x2) /* has range on conformance */
                stubMsg.CorrDespIncrement = 12;
        }
    }
    else
    {
        pFormat = convert_old_args( &stubMsg, pFormat, stack_size,
                                    pProcHeader->Oi_flags & Oi_OBJECT_PROC,
                                    /* reuse the correlation cache, it's not needed for v1 format */
                                    NdrCorrCache, sizeof(NdrCorrCache), &number_of_params );
    }

    /* convert strings, floating point values and endianness into our
     * preferred format */
    if ((pRpcMsg->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
        NdrConvert(&stubMsg, pFormat);

    for (phase = STUBLESS_UNMARSHAL; phase <= STUBLESS_FREE; phase++)
    {
        TRACE("phase = %d\n", phase);
        switch (phase)
        {
        case STUBLESS_CALLSERVER:
            /* call the server function */
            if (pServerInfo->ThunkTable && pServerInfo->ThunkTable[pRpcMsg->ProcNum])
                pServerInfo->ThunkTable[pRpcMsg->ProcNum](&stubMsg);
            else
            {
                SERVER_ROUTINE func;
                LONG_PTR retval;

                if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
                {
                    SERVER_ROUTINE *vtbl = *(SERVER_ROUTINE **)((CStdStubBuffer *)pThis)->pvServerObject;
                    func = vtbl[pRpcMsg->ProcNum];
                }
                else
                    func = pServerInfo->DispatchTable[pRpcMsg->ProcNum];

                /* FIXME: what happens with return values that don't fit into a single register on x86? */
                retval = call_server_func(func, args, stack_size);

                if (retval_ptr)
                {
                    TRACE("stub implementation returned 0x%lx\n", retval);
                    *retval_ptr = retval;
                }
                else
                    TRACE("void stub implementation\n");
            }

            stubMsg.Buffer = NULL;
            stubMsg.BufferLength = 0;

            break;
        case STUBLESS_GETBUFFER:
            if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
                NdrStubGetBuffer(pThis, pChannel, &stubMsg);
            else
            {
                RPC_STATUS Status;

                pRpcMsg->BufferLength = stubMsg.BufferLength;
                /* allocate buffer for [out] and [ret] params */
                Status = I_RpcGetBuffer(pRpcMsg); 
                if (Status)
                    RpcRaiseException(Status);
                stubMsg.Buffer = pRpcMsg->Buffer;
            }
            break;
        case STUBLESS_UNMARSHAL:
        case STUBLESS_INITOUT:
        case STUBLESS_CALCSIZE:
        case STUBLESS_MARSHAL:
        case STUBLESS_MUSTFREE:
        case STUBLESS_FREE:
            retval_ptr = stub_do_args(&stubMsg, pFormat, phase, number_of_params);
            break;
        default:
            ERR("shouldn't reach here. phase %d\n", phase);
            break;
        }
    }

    pRpcMsg->BufferLength = (unsigned int)(stubMsg.Buffer - (unsigned char *)pRpcMsg->Buffer);

    if (ext_flags.HasNewCorrDesc)
    {
        /* free extra correlation package */
        NdrCorrelationFree(&stubMsg);
    }

    if (Oif_flags.HasPipes)
    {
        /* NdrPipesDone(...) */
    }

    /* free the full pointer translation tables */
    if (pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        NdrFullPointerXlatFree(stubMsg.FullPtrXlatTables);

    /* free server function stack */
    HeapFree(GetProcessHeap(), 0, args);

    return S_OK;
}

/***********************************************************************
 *            NdrServerCall2 [RPCRT4.@]
 */
void WINAPI NdrServerCall2(PRPC_MESSAGE pRpcMsg)
{
    DWORD dwPhase;
    NdrStubCall2(NULL, NULL, pRpcMsg, &dwPhase);
}

/***********************************************************************
 *            NdrStubCall [RPCRT4.@]
 */
LONG WINAPI NdrStubCall( struct IRpcStubBuffer *This, struct IRpcChannelBuffer *channel,
                         PRPC_MESSAGE msg, DWORD *phase )
{
    return NdrStubCall2( This, channel, msg, phase );
}

/***********************************************************************
 *            NdrServerCall [RPCRT4.@]
 */
void WINAPI NdrServerCall( PRPC_MESSAGE msg )
{
    DWORD phase;
    NdrStubCall( NULL, NULL, msg, &phase );
}

/***********************************************************************
 *            NdrServerCallAll [RPCRT4.@]
 */
void WINAPI NdrServerCallAll( PRPC_MESSAGE msg )
{
    FIXME("%p stub\n", msg);
}

/* Helper for ndr_async_client_call, to factor out the part that may or may not be
 * guarded by a try/except block. */
static void do_ndr_async_client_call( const MIDL_STUB_DESC *pStubDesc, PFORMAT_STRING pFormat, void **stack_top )
{
    /* pointer to start of stack where arguments start */
    PRPC_MESSAGE pRpcMsg;
    PMIDL_STUB_MESSAGE pStubMsg;
    RPC_ASYNC_STATE *pAsync;
    struct async_call_data *async_call_data;
    /* procedure number */
    unsigned short procedure_number;
    /* cache of Oif_flags from v2 procedure header */
    INTERPRETER_OPT_FLAGS Oif_flags = { 0 };
    /* cache of extension flags from NDR_PROC_HEADER_EXTS */
    INTERPRETER_OPT_FLAGS2 ext_flags = { 0 };
    /* header for procedure string */
    const NDR_PROC_HEADER * pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];
    RPC_STATUS status;

    /* Later NDR language versions probably won't be backwards compatible */
    if (pStubDesc->Version > 0x50002)
    {
        FIXME("Incompatible stub description version: 0x%x\n", pStubDesc->Version);
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION);
    }

    async_call_data = I_RpcAllocate(sizeof(*async_call_data) + sizeof(MIDL_STUB_MESSAGE) + sizeof(RPC_MESSAGE));
    if (!async_call_data) RpcRaiseException(RPC_X_NO_MEMORY);
    async_call_data->pProcHeader = pProcHeader;

    async_call_data->pStubMsg = pStubMsg = (PMIDL_STUB_MESSAGE)(async_call_data + 1);
    pRpcMsg = (PRPC_MESSAGE)(pStubMsg + 1);

    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
    {
        const NDR_PROC_HEADER_RPC *header_rpc = (const NDR_PROC_HEADER_RPC *)&pFormat[0];
        async_call_data->stack_size = header_rpc->stack_size;
        procedure_number = header_rpc->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER_RPC);
    }
    else
    {
        async_call_data->stack_size = pProcHeader->stack_size;
        procedure_number = pProcHeader->proc_num;
        pFormat += sizeof(NDR_PROC_HEADER);
    }
    TRACE("stack size: 0x%x\n", async_call_data->stack_size);
    TRACE("proc num: %d\n", procedure_number);

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);

    if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
    {
        ERR("objects not supported\n");
        I_RpcFree(async_call_data);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    NdrClientInitializeNew(pRpcMsg, pStubMsg, pStubDesc, procedure_number);

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);
    TRACE("MIDL stub version = 0x%x\n", pStubDesc->MIDLVersion);

    /* needed for conformance of top-level objects */
    pStubMsg->StackTop = I_RpcAllocate(async_call_data->stack_size);
    memcpy(pStubMsg->StackTop, stack_top, async_call_data->stack_size);

    pAsync = *(RPC_ASYNC_STATE **)pStubMsg->StackTop;
    pAsync->StubInfo = async_call_data;
    async_call_data->pHandleFormat = pFormat;

    TRACE("pAsync %p, pAsync->StubInfo %p, NotificationType %d\n", pAsync, pAsync->StubInfo, pAsync->NotificationType);

    pFormat += get_handle_desc_size(pProcHeader, pFormat);
    async_call_data->hBinding = client_get_handle(pStubMsg, pProcHeader, async_call_data->pHandleFormat);
    if (!async_call_data->hBinding) return;

    if (is_oicf_stubdesc(pStubDesc))
    {
        const NDR_PROC_PARTIAL_OIF_HEADER *pOIFHeader =
            (const NDR_PROC_PARTIAL_OIF_HEADER *)pFormat;

        Oif_flags = pOIFHeader->Oi2Flags;
        async_call_data->number_of_params = pOIFHeader->number_of_params;

        pFormat += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);

        TRACE("Oif_flags = %s\n", debugstr_INTERPRETER_OPT_FLAGS(Oif_flags) );

        if (Oif_flags.HasExtensions)
        {
            const NDR_PROC_HEADER_EXTS *pExtensions =
                (const NDR_PROC_HEADER_EXTS *)pFormat;
            ext_flags = pExtensions->Flags2;
            pFormat += pExtensions->Size;
        }
    }
    else
    {
        pFormat = convert_old_args( pStubMsg, pFormat, async_call_data->stack_size,
                                    pProcHeader->Oi_flags & Oi_OBJECT_PROC,
                                    async_call_data->NdrCorrCache, sizeof(async_call_data->NdrCorrCache),
                                    &async_call_data->number_of_params );
    }

    async_call_data->pParamFormat = pFormat;

    pStubMsg->BufferLength = 0;

    /* store the RPC flags away */
    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
        pRpcMsg->RpcFlags = ((const NDR_PROC_HEADER_RPC *)pProcHeader)->rpc_flags;

    /* use alternate memory allocation routines */
    if (pProcHeader->Oi_flags & Oi_RPCSS_ALLOC_USED)
        NdrRpcSmSetClientToOsf(pStubMsg);

    if (Oif_flags.HasPipes)
    {
        FIXME("pipes not supported yet\n");
        RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
        /* init pipes package */
        /* NdrPipesInitialize(...) */
    }
    if (ext_flags.HasNewCorrDesc)
    {
        /* initialize extra correlation package */
        NdrCorrelationInitialize(pStubMsg, async_call_data->NdrCorrCache, sizeof(async_call_data->NdrCorrCache), 0);
        if (ext_flags.Unused & 0x2) /* has range on conformance */
            pStubMsg->CorrDespIncrement = 12;
    }

    /* order of phases:
     * 1. CALCSIZE - calculate the buffer size
     * 2. GETBUFFER - allocate the buffer
     * 3. MARSHAL - marshal [in] params into the buffer
     * 4. SENDRECEIVE - send buffer
     * Then in NdrpCompleteAsyncClientCall:
     * 1. SENDRECEIVE - receive buffer
     * 2. UNMARSHAL - unmarshal [out] params from buffer
     */

    /* 1. CALCSIZE */
    TRACE( "CALCSIZE\n" );
    client_do_args(pStubMsg, pFormat, STUBLESS_CALCSIZE, NULL, async_call_data->number_of_params, NULL);

    /* 2. GETBUFFER */
    TRACE( "GETBUFFER\n" );
    if (Oif_flags.HasPipes)
        /* NdrGetPipeBuffer(...) */
        FIXME("pipes not supported yet\n");
    else
    {
        if (pProcHeader->handle_type == FC_AUTO_HANDLE)
#if 0
            NdrNsGetBuffer(pStubMsg, pStubMsg->BufferLength, async_call_data->hBinding);
#else
        FIXME("using auto handle - call NdrNsGetBuffer when it gets implemented\n");
#endif
        else
            NdrGetBuffer(pStubMsg, pStubMsg->BufferLength, async_call_data->hBinding);
    }
    pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;
    status = I_RpcAsyncSetHandle(pRpcMsg, pAsync);
    if (status != RPC_S_OK)
        RpcRaiseException(status);

    /* 3. MARSHAL */
    TRACE( "MARSHAL\n" );
    client_do_args(pStubMsg, pFormat, STUBLESS_MARSHAL, NULL, async_call_data->number_of_params, NULL);

    /* 4. SENDRECEIVE */
    TRACE( "SEND\n" );
    pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;
    /* send the [in] params only */
    if (Oif_flags.HasPipes)
        /* NdrPipesSend(...) */
        FIXME("pipes not supported yet\n");
    else
    {
        if (pProcHeader->handle_type == FC_AUTO_HANDLE)
#if 0
            NdrNsSend(&stubMsg, stubMsg.Buffer, pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
#else
        FIXME("using auto handle - call NdrNsSend when it gets implemented\n");
#endif
        else
        {
            pStubMsg->RpcMsg->BufferLength = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
            status = I_RpcSend(pStubMsg->RpcMsg);
            if (status != RPC_S_OK)
                RpcRaiseException(status);
        }
    }
}

LONG_PTR CDECL DECLSPEC_HIDDEN ndr_async_client_call( PMIDL_STUB_DESC pStubDesc, PFORMAT_STRING pFormat,
                                                      void **stack_top )
{
    LONG_PTR ret = 0;
    const NDR_PROC_HEADER *pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];

    TRACE("pStubDesc %p, pFormat %p, ...\n", pStubDesc, pFormat);

    if (pProcHeader->Oi_flags & Oi_HAS_COMM_OR_FAULT)
    {
        __TRY
        {
            do_ndr_async_client_call( pStubDesc, pFormat, stack_top );
        }
        __EXCEPT_ALL
        {
            FIXME("exception %x during ndr_async_client_call()\n", GetExceptionCode());
            ret = GetExceptionCode();
        }
        __ENDTRY
    }
    else
        do_ndr_async_client_call( pStubDesc, pFormat, stack_top);

    TRACE("returning %ld\n", ret);
    return ret;
}

RPC_STATUS NdrpCompleteAsyncClientCall(RPC_ASYNC_STATE *pAsync, void *Reply)
{
    /* pointer to start of stack where arguments start */
    PMIDL_STUB_MESSAGE pStubMsg;
    struct async_call_data *async_call_data;
    /* header for procedure string */
    const NDR_PROC_HEADER * pProcHeader;
    RPC_STATUS status = RPC_S_OK;

    if (!pAsync->StubInfo)
        return RPC_S_INVALID_ASYNC_HANDLE;

    async_call_data = pAsync->StubInfo;
    pStubMsg = async_call_data->pStubMsg;
    pProcHeader = async_call_data->pProcHeader;

    /* order of phases:
     * 1. CALCSIZE - calculate the buffer size
     * 2. GETBUFFER - allocate the buffer
     * 3. MARSHAL - marshal [in] params into the buffer
     * 4. SENDRECEIVE - send buffer
     * Then in NdrpCompleteAsyncClientCall:
     * 1. SENDRECEIVE - receive buffer
     * 2. UNMARSHAL - unmarshal [out] params from buffer
     */

    /* 1. SENDRECEIVE */
    TRACE( "RECEIVE\n" );
    pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;
    /* receive the [out] params */
    if (pProcHeader->handle_type == FC_AUTO_HANDLE)
#if 0
        NdrNsReceive(&stubMsg, stubMsg.Buffer, pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
#else
    FIXME("using auto handle - call NdrNsReceive when it gets implemented\n");
#endif
    else
    {
        status = I_RpcReceive(pStubMsg->RpcMsg);
        if (status != RPC_S_OK)
            goto cleanup;
        pStubMsg->BufferLength = pStubMsg->RpcMsg->BufferLength;
        pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
        pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
        pStubMsg->Buffer = pStubMsg->BufferStart;
    }

    /* convert strings, floating point values and endianness into our
     * preferred format */
#if 0
    if ((pStubMsg->RpcMsg.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
        NdrConvert(pStubMsg, pFormat);
#endif

    /* 2. UNMARSHAL */
    TRACE( "UNMARSHAL\n" );
    client_do_args(pStubMsg, async_call_data->pParamFormat, STUBLESS_UNMARSHAL,
                   NULL, async_call_data->number_of_params, Reply);

cleanup:
    if (pStubMsg->fHasNewCorrDesc)
    {
        /* free extra correlation package */
        NdrCorrelationFree(pStubMsg);
    }

    /* free the full pointer translation tables */
    if (pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

    /* free marshalling buffer */
    NdrFreeBuffer(pStubMsg);
    client_free_handle(pStubMsg, pProcHeader, async_call_data->pHandleFormat, async_call_data->hBinding);

    I_RpcFree(pStubMsg->StackTop);
    I_RpcFree(async_call_data);

    TRACE("-- 0x%x\n", status);
    return status;
}

#ifdef __x86_64__

__ASM_GLOBAL_FUNC( NdrAsyncClientCall,
                   "subq $0x28,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x28\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   "movq %r8,0x40(%rsp)\n\t"
                   "movq %r9,0x48(%rsp)\n\t"
                   "leaq 0x40(%rsp),%r8\n\t"
                   "call " __ASM_NAME("ndr_async_client_call") "\n\t"
                   "addq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -0x28\n\t")
                   "ret" );

#else  /* __x86_64__ */

/***********************************************************************
 *            NdrAsyncClientCall [RPCRT4.@]
 */
CLIENT_CALL_RETURN WINAPIV NdrAsyncClientCall( PMIDL_STUB_DESC desc, PFORMAT_STRING format, ... )
{
    __ms_va_list args;
    LONG_PTR ret;

    __ms_va_start( args, format );
    ret = ndr_async_client_call( desc, format, va_arg( args, void ** ));
    __ms_va_end( args );
    return *(CLIENT_CALL_RETURN *)&ret;
}

#endif  /* __x86_64__ */

RPCRTAPI LONG RPC_ENTRY NdrAsyncStubCall(struct IRpcStubBuffer* pThis,
    struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg,
    DWORD * pdwStubPhase)
{
    FIXME("unimplemented, expect crash!\n");
    return 0;
}

void RPC_ENTRY NdrAsyncServerCall(PRPC_MESSAGE pRpcMsg)
{
    const MIDL_SERVER_INFO *pServerInfo;
    const MIDL_STUB_DESC *pStubDesc;
    PFORMAT_STRING pFormat;
    /* pointer to start of stack to pass into stub implementation */
    unsigned char *args;
    /* header for procedure string */
    const NDR_PROC_HEADER *pProcHeader;
    struct async_call_data *async_call_data;
    PRPC_ASYNC_STATE pAsync;
    RPC_STATUS status;

    TRACE("%p\n", pRpcMsg);

    pServerInfo = ((RPC_SERVER_INTERFACE *)pRpcMsg->RpcInterfaceInformation)->InterpreterInfo;

    pStubDesc = pServerInfo->pStubDesc;
    pFormat = pServerInfo->ProcString + pServerInfo->FmtStringOffset[pRpcMsg->ProcNum];
    pProcHeader = (const NDR_PROC_HEADER *)&pFormat[0];

    TRACE("NDR Version: 0x%x\n", pStubDesc->Version);

    async_call_data = I_RpcAllocate(sizeof(*async_call_data) + sizeof(MIDL_STUB_MESSAGE) + sizeof(RPC_MESSAGE));
    if (!async_call_data) RpcRaiseException(RPC_X_NO_MEMORY);
    async_call_data->pProcHeader = pProcHeader;

    async_call_data->pStubMsg = (PMIDL_STUB_MESSAGE)(async_call_data + 1);
    *(PRPC_MESSAGE)(async_call_data->pStubMsg + 1) = *pRpcMsg;

    if (pProcHeader->Oi_flags & Oi_HAS_RPCFLAGS)
    {
        const NDR_PROC_HEADER_RPC *header_rpc = (const NDR_PROC_HEADER_RPC *)&pFormat[0];
        async_call_data->stack_size = header_rpc->stack_size;
        pFormat += sizeof(NDR_PROC_HEADER_RPC);
    }
    else
    {
        async_call_data->stack_size = pProcHeader->stack_size;
        pFormat += sizeof(NDR_PROC_HEADER);
    }

    TRACE("Oi_flags = 0x%02x\n", pProcHeader->Oi_flags);

    /* binding */
    switch (pProcHeader->handle_type)
    {
    /* explicit binding: parse additional section */
    case 0:
        switch (*pFormat) /* handle_type */
        {
        case FC_BIND_PRIMITIVE: /* explicit primitive */
            pFormat += sizeof(NDR_EHD_PRIMITIVE);
            break;
        case FC_BIND_GENERIC: /* explicit generic */
            pFormat += sizeof(NDR_EHD_GENERIC);
            break;
        case FC_BIND_CONTEXT: /* explicit context */
            pFormat += sizeof(NDR_EHD_CONTEXT);
            break;
        default:
            ERR("bad explicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
            RpcRaiseException(RPC_X_BAD_STUB_DATA);
        }
        break;
    case FC_BIND_GENERIC: /* implicit generic */
    case FC_BIND_PRIMITIVE: /* implicit primitive */
    case FC_CALLBACK_HANDLE: /* implicit callback */
    case FC_AUTO_HANDLE: /* implicit auto handle */
        break;
    default:
        ERR("bad implicit binding handle type (0x%02x)\n", pProcHeader->handle_type);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    if (pProcHeader->Oi_flags & Oi_OBJECT_PROC)
    {
        ERR("objects not supported\n");
        I_RpcFree(async_call_data);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    NdrServerInitializeNew(pRpcMsg, async_call_data->pStubMsg, pStubDesc);

    /* create the full pointer translation tables, if requested */
    if (pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        async_call_data->pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit(0, XLAT_SERVER);

    /* use alternate memory allocation routines */
    if (pProcHeader->Oi_flags & Oi_RPCSS_ALLOC_USED)
#if 0
          NdrRpcSsEnableAllocate(&stubMsg);
#else
          FIXME("Set RPCSS memory allocation routines\n");
#endif

    TRACE("allocating memory for stack of size %x\n", async_call_data->stack_size);

    args = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, async_call_data->stack_size);
    async_call_data->pStubMsg->StackTop = args; /* used by conformance of top-level objects */

    pAsync = I_RpcAllocate(sizeof(*pAsync));
    if (!pAsync) RpcRaiseException(RPC_X_NO_MEMORY);

    status = RpcAsyncInitializeHandle(pAsync, sizeof(*pAsync));
    if (status != RPC_S_OK)
        RpcRaiseException(status);

    pAsync->StubInfo = async_call_data;
    TRACE("pAsync %p, pAsync->StubInfo %p, pFormat %p\n", pAsync, pAsync->StubInfo, async_call_data->pHandleFormat);

    /* add the implicit pAsync pointer as the first arg to the function */
    *(void **)args = pAsync;

    if (is_oicf_stubdesc(pStubDesc))
    {
        const NDR_PROC_PARTIAL_OIF_HEADER *pOIFHeader = (const NDR_PROC_PARTIAL_OIF_HEADER *)pFormat;
        /* cache of Oif_flags from v2 procedure header */
        INTERPRETER_OPT_FLAGS Oif_flags;
        /* cache of extension flags from NDR_PROC_HEADER_EXTS */
        INTERPRETER_OPT_FLAGS2 ext_flags = { 0 };

        Oif_flags = pOIFHeader->Oi2Flags;
        async_call_data->number_of_params = pOIFHeader->number_of_params;

        pFormat += sizeof(NDR_PROC_PARTIAL_OIF_HEADER);

        TRACE("Oif_flags = %s\n", debugstr_INTERPRETER_OPT_FLAGS(Oif_flags) );

        if (Oif_flags.HasExtensions)
        {
            const NDR_PROC_HEADER_EXTS *pExtensions = (const NDR_PROC_HEADER_EXTS *)pFormat;
            ext_flags = pExtensions->Flags2;
            pFormat += pExtensions->Size;
        }

        if (Oif_flags.HasPipes)
        {
            FIXME("pipes not supported yet\n");
            RpcRaiseException(RPC_X_WRONG_STUB_VERSION); /* FIXME: remove when implemented */
            /* init pipes package */
            /* NdrPipesInitialize(...) */
        }
        if (ext_flags.HasNewCorrDesc)
        {
            /* initialize extra correlation package */
            NdrCorrelationInitialize(async_call_data->pStubMsg, async_call_data->NdrCorrCache, sizeof(async_call_data->NdrCorrCache), 0);
            if (ext_flags.Unused & 0x2) /* has range on conformance */
                async_call_data->pStubMsg->CorrDespIncrement = 12;
        }
    }
    else
    {
        pFormat = convert_old_args( async_call_data->pStubMsg, pFormat, async_call_data->stack_size,
                                    pProcHeader->Oi_flags & Oi_OBJECT_PROC,
                                    /* reuse the correlation cache, it's not needed for v1 format */
                                    async_call_data->NdrCorrCache, sizeof(async_call_data->NdrCorrCache), &async_call_data->number_of_params );
    }

    /* convert strings, floating point values and endianness into our
     * preferred format */
    if ((pRpcMsg->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)
        NdrConvert(async_call_data->pStubMsg, pFormat);

    async_call_data->pHandleFormat = pFormat;

    /* 1. UNMARSHAL */
    TRACE("UNMARSHAL\n");
    stub_do_args(async_call_data->pStubMsg, pFormat, STUBLESS_UNMARSHAL, async_call_data->number_of_params);

    /* 2. INITOUT */
    TRACE("INITOUT\n");
    async_call_data->retval_ptr = stub_do_args(async_call_data->pStubMsg, pFormat, STUBLESS_INITOUT, async_call_data->number_of_params);

    /* 3. CALLSERVER */
    TRACE("CALLSERVER\n");
    if (pServerInfo->ThunkTable && pServerInfo->ThunkTable[pRpcMsg->ProcNum])
        pServerInfo->ThunkTable[pRpcMsg->ProcNum](async_call_data->pStubMsg);
    else
        call_server_func(pServerInfo->DispatchTable[pRpcMsg->ProcNum], args, async_call_data->stack_size);
}

RPC_STATUS NdrpCompleteAsyncServerCall(RPC_ASYNC_STATE *pAsync, void *Reply)
{
    /* pointer to start of stack where arguments start */
    PMIDL_STUB_MESSAGE pStubMsg;
    struct async_call_data *async_call_data;
    /* the type of pass we are currently doing */
    enum stubless_phase phase;
    RPC_STATUS status = RPC_S_OK;

    if (!pAsync->StubInfo)
        return RPC_S_INVALID_ASYNC_HANDLE;

    async_call_data = pAsync->StubInfo;
    pStubMsg = async_call_data->pStubMsg;

    TRACE("pAsync %p, pAsync->StubInfo %p, pFormat %p\n", pAsync, pAsync->StubInfo, async_call_data->pHandleFormat);

    if (async_call_data->retval_ptr)
    {
        TRACE("stub implementation returned 0x%lx\n", *(LONG_PTR *)Reply);
        *async_call_data->retval_ptr = *(LONG_PTR *)Reply;
    }
    else
        TRACE("void stub implementation\n");

    for (phase = STUBLESS_CALCSIZE; phase <= STUBLESS_FREE; phase++)
    {
        TRACE("phase = %d\n", phase);
        switch (phase)
        {
        case STUBLESS_GETBUFFER:
            if (async_call_data->pProcHeader->Oi_flags & Oi_OBJECT_PROC)
            {
                ERR("objects not supported\n");
                HeapFree(GetProcessHeap(), 0, async_call_data->pStubMsg->StackTop);
                I_RpcFree(async_call_data);
                I_RpcFree(pAsync);
                RpcRaiseException(RPC_X_BAD_STUB_DATA);
            }
            else
            {
                pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
                /* allocate buffer for [out] and [ret] params */
                status = I_RpcGetBuffer(pStubMsg->RpcMsg);
                if (status)
                    RpcRaiseException(status);
                pStubMsg->Buffer = pStubMsg->RpcMsg->Buffer;
            }
            break;

        case STUBLESS_CALCSIZE:
        case STUBLESS_MARSHAL:
        case STUBLESS_MUSTFREE:
        case STUBLESS_FREE:
            stub_do_args(pStubMsg, async_call_data->pHandleFormat, phase, async_call_data->number_of_params);
            break;
        default:
            ERR("shouldn't reach here. phase %d\n", phase);
            break;
        }
    }

#if 0 /* FIXME */
    if (ext_flags.HasNewCorrDesc)
    {
        /* free extra correlation package */
        NdrCorrelationFree(pStubMsg);
    }

    if (Oif_flags.HasPipes)
    {
        /* NdrPipesDone(...) */
    }

    /* free the full pointer translation tables */
    if (async_call_data->pProcHeader->Oi_flags & Oi_FULL_PTR_USED)
        NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);
#endif

    /* free server function stack */
    HeapFree(GetProcessHeap(), 0, async_call_data->pStubMsg->StackTop);
    I_RpcFree(async_call_data);
    I_RpcFree(pAsync);

    return S_OK;
}
