/*
 * NDR Types
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
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

#ifndef __NDRTYPES_H__
#define __NDRTYPES_H__

#include <limits.h>

typedef struct
{
    unsigned short MustSize : 1; /* 0x0001 - client interpreter MUST size this
     *  parameter, other parameters may be skipped, using the value in
     *  NDR_PROC_PARTIAL_OIF_HEADER::constant_client_buffer_size instead. */
    unsigned short MustFree : 1; /* 0x0002 - server interpreter MUST size this
     *  parameter, other parameters may be skipped, using the value in
     *  NDR_PROC_PARTIAL_OIF_HEADER::constant_server_buffer_size instead. */
    unsigned short IsPipe : 1; /* 0x0004 - The parameter is a pipe handle */
    unsigned short IsIn : 1; /* 0x0008 - The parameter is an input */
    unsigned short IsOut : 1; /* 0x0010 - The parameter is an output */
    unsigned short IsReturn : 1; /* 0x0020 - The parameter is to be returned */
    unsigned short IsBasetype : 1; /* 0x0040 - The parameter is simple and has the
     *  format defined by NDR_PARAM_OIF_BASETYPE rather than by
     *  NDR_PARAM_OIF_OTHER. */
    unsigned short IsByValue : 1; /* 0x0080 - Set for compound types being sent by
     *  value. Can be of type: structure, union, transmit_as, represent_as,
     *  wire_marshal and SAFEARRAY. */
    unsigned short IsSimpleRef : 1; /* 0x0100 - parameter that is a reference
     *  pointer to anything other than another pointer, and which has no
     *  allocate attributes. */
    unsigned short IsDontCallFreeInst : 1; /*  0x0200 - Used for some represent_as types
     *  for when the free instance routine should not be called. */
    unsigned short SaveForAsyncFinish : 1; /* 0x0400 - Unknown */
    unsigned short Unused : 2;
    unsigned short ServerAllocSize : 3; /* 0xe000 - If non-zero
     *  specifies the size of the object in numbers of 8byte blocks needed.
     *  It will be stored on the server's stack rather than using an allocate
     *  call. */
} PARAM_ATTRIBUTES;

typedef struct
{
    unsigned char ServerMustSize : 1; /* 0x01 - the server must perform a
     *  sizing pass. */
    unsigned char ClientMustSize : 1; /* 0x02 - the client must perform a
     *  sizing pass. */
    unsigned char HasReturn : 1; /* 0x04 - procedure has a return value. */
    unsigned char HasPipes : 1; /* 0x08 - the pipe package should be used. */
    unsigned char Unused : 1; /* 0x10 - not used */
    unsigned char HasAsyncUuid : 1; /* 0x20 - indicates an asynchronous DCOM
     *  procedure. */
    unsigned char HasExtensions : 1; /* 0x40 - indicates that Win2000
     *  extensions are in use. */
    unsigned char HasAsyncHandle : 1; /* 0x80 - indicates an asynchronous RPC
     *  procedure. */
} INTERPRETER_OPT_FLAGS, *PINTERPRETER_OPT_FLAGS;

typedef struct
{
    unsigned char HasNewCorrDesc : 1; /* 0x01 - indicates new correlation
     *  descriptors in use. */
    unsigned char ClientCorrCheck : 1; /* 0x02 - client needs correlation
     *  check. */
    unsigned char ServerCorrCheck : 1; /* 0x04 - server needs correlation
     *  check. */
    unsigned char HasNotify : 1; /* 0x08 - should call MIDL [notify]
     *  routine @ NotifyIndex. */
    unsigned char HasNotify2 : 1; /* 0x10 - should call MIDL [notify_flag] routine @ 
     *  NotifyIndex. */

    /* The following bits are in fact used by midl but haven't yet been
       named in the SDK. */
    unsigned char Unused : 3; /* 0x20 - has complex return */
                              /* 0x40 - has range on conformance */
} INTERPRETER_OPT_FLAGS2, *PINTERPRETER_OPT_FLAGS2;

/* Win2000 extensions */
typedef struct
{
    /* size in bytes of all following extensions */
    unsigned char Size;

    INTERPRETER_OPT_FLAGS2 Flags2;

    /* client cache size hint */
    unsigned short ClientCorrHint;

    /* server cache size hint */
    unsigned short ServerCorrHint;

    /* index of routine in MIDL_STUB_DESC::NotifyRoutineTable to call if
     * HasNotify or HasNotify2 flag set */
    unsigned short NotifyIndex;
} NDR_PROC_HEADER_EXTS;

typedef struct
{
    /* size in bytes of all following extensions */
    unsigned char Size;

    INTERPRETER_OPT_FLAGS2 Flags2;

    /* client cache size hint */
    unsigned short ClientCorrHint;

    /* server cache size hint */
    unsigned short ServerCorrHint;

    /* index of routine in MIDL_STUB_DESC::NotifyRoutineTable to call if
     * HasNotify or HasNotify2 flag set */
    unsigned short NotifyIndex;

    /* needed only on IA64 to cope with float/register loading */
    unsigned short FloatArgMask;
} NDR_PROC_HEADER_EXTS64;

typedef enum
{
    FC_BYTE = 0x01, /* 0x01 */
    FC_CHAR, /* 0x02 */
    FC_SMALL, /* 0x03 */
    FC_USMALL, /* 0x04 */
    FC_WCHAR, /* 0x05 */
    FC_SHORT, /* 0x06 */
    FC_USHORT, /* 0x07 */
    FC_LONG, /* 0x08 */
    FC_ULONG, /* 0x09 */
    FC_FLOAT, /* 0x0a */
    FC_HYPER, /* 0x0b */
    FC_DOUBLE, /* 0x0c */
    FC_ENUM16, /* 0x0d */
    FC_ENUM32, /* 0x0e */
    FC_IGNORE, /* 0x0f */
    FC_ERROR_STATUS_T, /* 0x10 */

    FC_RP, /* 0x11 */ /* reference pointer */
    FC_UP, /* 0x12 */ /* unique pointer */
    FC_OP, /* 0x13 */ /* object pointer */
    FC_FP, /* 0x14 */ /* full pointer */

    FC_STRUCT, /* 0x15 */ /* simple structure */
    FC_PSTRUCT, /* 0x16 */ /* simple structure w/ pointers */
    FC_CSTRUCT, /* 0x17 */ /* conformant structure */
    FC_CPSTRUCT, /* 0x18 */ /* conformant structure w/ pointers */
    FC_CVSTRUCT, /* 0x19 */ /* conformant varying struct */
    FC_BOGUS_STRUCT, /* 0x1a */ /* complex structure */

    FC_CARRAY, /* 0x1b */ /* conformant array */
    FC_CVARRAY, /* 0x1c */ /* conformant varying array */
    FC_SMFARRAY, /* 0x1d */ /* small (<64K) fixed array */
    FC_LGFARRAY, /* 0x1e */ /* large (>= 64k) fixed array */
    FC_SMVARRAY, /* 0x1f */ /* small (<64k) varying array */
    FC_LGVARRAY, /* 0x20 */ /* large (>= 64k) varying array */
    FC_BOGUS_ARRAY, /* 0x21 */ /* complex array */

    FC_C_CSTRING, /* 0x22 */ /* conformant char string */
    FC_C_BSTRING, /* 0x23 */
    FC_C_SSTRING, /* 0x24 */
    FC_C_WSTRING, /* 0x25 */ /* conformant wchar string */
    FC_CSTRING, /* 0x26 */ /* non-conformant char string */
    FC_BSTRING, /* 0x27 */
    FC_SSTRING, /* 0x28 */
    FC_WSTRING, /* 0x29 */ /* non-conformant wchar string */

    FC_ENCAPSULATED_UNION, /* 0x2a */
    FC_NON_ENCAPSULATED_UNION, /* 0x2b */

    FC_BYTE_COUNT_POINTER, /* 0x2c */ /* [byte_count] ACF attribute */

    FC_TRANSMIT_AS, /* 0x2d */
    FC_REPRESENT_AS, /* 0x2e */

    FC_IP, /* 0x2f */ /* interface pointer */

    FC_BIND_CONTEXT, /* 0x30 */

    FC_BIND_GENERIC, /* 0x31 */
    FC_BIND_PRIMITIVE, /* 0x32 */
    FC_AUTO_HANDLE, /* 0x33 */
    FC_CALLBACK_HANDLE, /* 0x34 */

    FC_UNUSED1, /* 0x35 */

    FC_POINTER, /* 0x36 */

    FC_ALIGNM2, /* 0x37 */
    FC_ALIGNM4, /* 0x38 */
    FC_ALIGNM8, /* 0x39 */

    FC_UNUSED2, /* 0x3a */
    FC_UNUSED3, /* 0x3b */
    FC_UNUSED4, /* 0x3c */

    FC_STRUCTPAD1, /* 0x3d */
    FC_STRUCTPAD2, /* 0x3e */
    FC_STRUCTPAD3, /* 0x3f */
    FC_STRUCTPAD4, /* 0x40 */
    FC_STRUCTPAD5, /* 0x41 */
    FC_STRUCTPAD6, /* 0x42 */
    FC_STRUCTPAD7, /* 0x43 */

    FC_STRING_SIZED, /* 0x44 */

    FC_UNUSED5, /* 0x45 */

    FC_NO_REPEAT, /* 0x46 */
    FC_FIXED_REPEAT, /* 0x47 */
    FC_VARIABLE_REPEAT, /* 0x48 */
    FC_FIXED_OFFSET, /* 0x49 */
    FC_VARIABLE_OFFSET, /* 0x4a */

    FC_PP, /* 0x4b */ /* pointer layout */
/* FC_PP: FC_PAD layouts */
/* layouts: FC_NO_REPEAT FC_PAD instance */
/* instance: NdrFcShort(memofs), NdrFcShort(bufofs), desc */

    FC_EMBEDDED_COMPLEX, /* 0x4c */

    FC_IN_PARAM, /* 0x4d */
    FC_IN_PARAM_BASETYPE, /* 0x4e */
    FC_IN_PARAM_NO_FREE_INST, /* 0x4f */
    FC_IN_OUT_PARAM, /* 0x50 */
    FC_OUT_PARAM, /* 0x51 */
    FC_RETURN_PARAM, /* 0x52 */
    FC_RETURN_PARAM_BASETYPE, /* 0x53 */

    FC_DEREFERENCE, /* 0x54 */
    FC_DIV_2, /* 0x55 */
    FC_MULT_2, /* 0x56 */
    FC_ADD_1, /* 0x57 */
    FC_SUB_1, /* 0x58 */

    FC_CALLBACK, /* 0x59 */

    FC_CONSTANT_IID, /* 0x5a */

    FC_END, /* 0x5b */
    FC_PAD, /* 0x5c */

    FC_HARD_STRUCT = 0xb1,

    FC_TRANSMIT_AS_PTR, /* 0xb2 */
    FC_REPRESENT_AS_PTR, /* 0xb3 */

    FC_USER_MARSHAL, /* 0xb4 */

    FC_PIPE, /* 0xb5 */
    FC_BLKHOLE, /* 0xb6 */

    FC_RANGE, /* 0xb7 */

    FC_INT3264, /* 0xb8 */
    FC_UINT3264, /* 0xb9 */
} FORMAT_CHARACTER;

#define FC_ALLOCATE_ALL_NODES   0x01
#define FC_DONT_FREE            0x02
#define FC_ALLOCED_ON_STACK     0x04
#define FC_SIMPLE_POINTER       0x08
#define FC_POINTER_DEREF        0x10

/* flags for all handle types */
#define HANDLE_PARAM_IS_VIA_PTR 0x80
#define HANDLE_PARAM_IS_IN      0x40
#define HANDLE_PARAM_IS_OUT     0x20
#define HANDLE_PARAM_IS_RETURN  0x10

/* flags for context handles */
#define NDR_STRICT_CONTEXT_HANDLE           0x08
#define NDR_CONTEXT_HANDLE_NOSERIALIZE      0x04
#define NDR_CONTEXT_HANDLE_SERIALIZE        0x02
#define NDR_CONTEXT_HANDLE_CANNOT_BE_NULL   0x01

#define Oi_FULL_PTR_USED            0x01
#define Oi_RPCSS_ALLOC_USED         0x02
#define Oi_OBJECT_PROC              0x04
#define Oi_HAS_RPCFLAGS             0x08
#define Oi_OBJ_USE_V2_INTERPRETER   0x20
#define Oi_HAS_COMM_OR_FAULT        0x20
#define Oi_USE_NEW_INIT_ROUTINES    0x40

#define FC_NORMAL_CONFORMANCE           (unsigned char) 0x00
#define FC_POINTER_CONFORMANCE          (unsigned char) 0x10
#define FC_TOP_LEVEL_CONFORMANCE        (unsigned char) 0x20
#define FC_CONSTANT_CONFORMANCE         (unsigned char) 0x40
#define FC_TOP_LEVEL_MULTID_CONFORMANCE (unsigned char) 0x80

#define USER_MARSHAL_UNIQUE     0x80
#define USER_MARSHAL_REF        0x40
#define USER_MARSHAL_POINTER    0xc0
#define USER_MARSHAL_IID        0x20

#endif
