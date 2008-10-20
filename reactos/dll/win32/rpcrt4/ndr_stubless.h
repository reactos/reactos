/*
 * NDR -Oi,-Oif,-Oicf Interpreter
 *
 * Copyright 2007 Robert Shearman (for CodeWeavers)
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

#include "ndrtypes.h"

/* there can't be any alignment with the structures in this file */
#include "pshpack1.h"

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
     * RPC_FC_CALLBACK_HANDLE = 34 - undocumented
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

    INTERPRETER_OPT_FLAGS Oi2Flags;

    /* number of params */
    unsigned char number_of_params;
} NDR_PROC_PARTIAL_OIF_HEADER;

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

void client_do_args_old_format(PMIDL_STUB_MESSAGE pStubMsg,
    PFORMAT_STRING pFormat, int phase, unsigned char *args,
    unsigned short stack_size, unsigned char *pRetVal, BOOL object_proc,
    BOOL ignore_retval);
