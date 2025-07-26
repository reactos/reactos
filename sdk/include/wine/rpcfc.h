/*
 * RPC format chars, as found by studying MIDL output.
 * Some, but not all, usage of these are explained on MSDN.
 */

#ifndef __WINE_RPCFC_H
#define __WINE_RPCFC_H

/* base types */
#define RPC_FC_BYTE			0x01
#define RPC_FC_CHAR			0x02
#define RPC_FC_SMALL			0x03
#define RPC_FC_USMALL			0x04
#define RPC_FC_WCHAR			0x05
#define RPC_FC_SHORT			0x06
#define RPC_FC_USHORT			0x07
#define RPC_FC_LONG			0x08
#define RPC_FC_ULONG			0x09
#define RPC_FC_FLOAT			0x0a
#define RPC_FC_HYPER			0x0b
#define RPC_FC_DOUBLE			0x0c
#define RPC_FC_ENUM16			0x0d
#define RPC_FC_ENUM32			0x0e
#define RPC_FC_IGNORE			0x0f
#define RPC_FC_ERROR_STATUS_T		0x10

/* other stuff */
#define RPC_FC_RP			0x11 /* reference pointer */
#define RPC_FC_UP			0x12 /* unique pointer */
#define RPC_FC_OP			0x13 /* unique pointer in object ? */
#define RPC_FC_FP			0x14 /* full pointer */

#define RPC_FC_STRUCT			0x15 /* simple structure */
/* FC_STRUCT: align-1, NdrFcShort(size), fields */

#define RPC_FC_PSTRUCT			0x16 /* simple structure w/ pointers */
/* FC_PTRUCT: align-1, NdrFcShort(size), ptrs, fields */

#define RPC_FC_CSTRUCT			0x17 /* conformant structure */

#define RPC_FC_CPSTRUCT                 0x18 /* conformant structure w/ pointers */

#define RPC_FC_CVSTRUCT                 0x19 /* conformant varying struct */

#define RPC_FC_BOGUS_STRUCT		0x1a /* complex structure */

#define RPC_FC_CARRAY			0x1b /* conformant array */
/* FC_CARRAY: align-1, NdrFcShort(size), conformance, ptrs, fields */
#define RPC_FC_CVARRAY			0x1c /* conformant varying array */
/* FC_CARRAY: align-1, NdrFcShort(size), conformance, variance, ptrs, fields */
#define RPC_FC_SMFARRAY			0x1d /* small (<64K) fixed array */
/* FC_SMFARRAY: align-1, NdrFcShort(size), ptrs, fields */

#define RPC_FC_LGFARRAY                 0x1e /* large (>= 64k) fixed array */

#define RPC_FC_SMVARRAY                 0x1f /* small (<64k) varying array */

#define RPC_FC_LGVARRAY                 0x20 /* large (>= 64k) varying array */

#define RPC_FC_BOGUS_ARRAY		0x21 /* complex array */

#define RPC_FC_C_CSTRING		0x22
#define RPC_FC_C_SSTRING		0x24
#define RPC_FC_C_WSTRING		0x25
#define RPC_FC_CSTRING                  0x26
#define RPC_FC_SSTRING			0x28
#define RPC_FC_WSTRING                  0x29

#define RPC_FC_ENCAPSULATED_UNION	0x2a
#define RPC_FC_NON_ENCAPSULATED_UNION	0x2b

#define RPC_FC_BYTE_COUNT_POINTER       0x2c /* [byte_count] ACF attribute */

#define RPC_FC_TRANSMIT_AS              0x2d
#define RPC_FC_REPRESENT_AS             0x2e

#define RPC_FC_IP			0x2f /* interface pointer */
/* FC_IP: FC_CONSTANT_IID iid */
/* FC_IP: FC_PAD correlation */

#define RPC_FC_BIND_CONTEXT		0x30

#define RPC_FC_BIND_GENERIC		0x31
#define RPC_FC_BIND_PRIMITIVE		0x32
#define RPC_FC_AUTO_HANDLE		0x33
#define RPC_FC_CALLBACK_HANDLE		0x34

#define RPC_FC_POINTER			0x36

#define RPC_FC_ALIGNM2			0x37
#define RPC_FC_ALIGNM4			0x38
#define RPC_FC_ALIGNM8			0x39

#define RPC_FC_STRUCTPAD1		0x3d
#define RPC_FC_STRUCTPAD2		0x3e
#define RPC_FC_STRUCTPAD3		0x3f
#define RPC_FC_STRUCTPAD4       0x40
#define RPC_FC_STRUCTPAD5       0x41
#define RPC_FC_STRUCTPAD6       0x42
#define RPC_FC_STRUCTPAD7       0x43

#define RPC_FC_STRING_SIZED		0x44

#define RPC_FC_NO_REPEAT		0x46
#define RPC_FC_FIXED_REPEAT		0x47
#define RPC_FC_VARIABLE_REPEAT		0x48
#define RPC_FC_FIXED_OFFSET		0x49
#define RPC_FC_VARIABLE_OFFSET		0x4a

#define RPC_FC_PP			0x4b /* pointer layout */
/* FC_PP: FC_PAD layouts */
/* layouts: FC_NO_REPEAT FC_PAD instance */
/* instance: NdrFcShort(memofs), NdrFcShort(bufofs), desc */

#define RPC_FC_EMBEDDED_COMPLEX		0x4c
/* FC_EMBEDDED_COMPLEX: padding, NdrFcShort(typeofs) */

#define RPC_FC_IN_PARAM			0x4d
/* FC_IN_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_IN_PARAM_BASETYPE	0x4e
/* FC_IN_PARAM_BASETYPE: basetype */
#define RPC_FC_IN_PARAM_NO_FREE_INST	0x4f
#define RPC_FC_IN_OUT_PARAM		0x50
/* FC_IN_OUT_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_OUT_PARAM		0x51
/* FC_OUT_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_RETURN_PARAM		0x52
/* FC_RETURN_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_RETURN_PARAM_BASETYPE	0x53
/* FC_RETURN_PARAM_BASETYPE: basetype */

#define RPC_FC_DEREFERENCE		0x54
#define RPC_FC_DIV_2			0x55
#define RPC_FC_MULT_2			0x56
#define RPC_FC_ADD_1			0x57
#define RPC_FC_SUB_1			0x58

#define RPC_FC_CALLBACK			0x59

#define RPC_FC_CONSTANT_IID		0x5a
/* FC_CONSTANT_IID: NdrFcLong(), NdrFcShort(), NdrFcShort(), 8x () */

#define RPC_FC_END			0x5b
#define RPC_FC_PAD			0x5c

#define RPC_FC_USER_MARSHAL		0xb4

#define RPC_FC_RANGE			0xb7

#define RPC_FC_INT3264			0xb8
#define RPC_FC_UINT3264			0xb9

/* FC_RP/UP/OP/FP: flags, NdrFcShort(typeofs)/basetype */
#define RPC_FC_P_ALLOCALLNODES		0x01
#define RPC_FC_P_DONTFREE		0x02
#define RPC_FC_P_ONSTACK		0x04 /* [alloced_on_stack] */
#define RPC_FC_P_SIMPLEPOINTER		0x08 /* [simple_pointer] */
#define RPC_FC_P_DEREF			0x10

#define RPC_FC_BIND_EXPLICIT		0x00

/* proc header: oiflags, NdrFcLong(rpcflags), NdrFcShort(procnum), NdrFcShort(stacksiz),
 *  oi2 header: NdrFcShort(clientbuf), NdrFcShort(servbuf), oi2flags, parmcount
 * oi2 parameters: NdrFcShort(flags), NdrFcShort(stackofs), NdrFcShort(typeofs)/basetype */
#define RPC_FC_PROC_OIF_FULLPTR         0x01
#define RPC_FC_PROC_OIF_RPCSSALLOC      0x02
#define RPC_FC_PROC_OIF_OBJECT          0x04
#define RPC_FC_PROC_OIF_RPCFLAGS        0x08
#define RPC_FC_PROC_OIF_OBJ_V2          0x20
#define RPC_FC_PROC_OIF_HAS_COMM_OR_FAULT 0x20
#define RPC_FC_PROC_OIF_NEWINIT         0x40

#define RPC_FC_PROC_PF_MUSTSIZE         0x0001
#define RPC_FC_PROC_PF_MUSTFREE         0x0002
#define RPC_FC_PROC_PF_PIPE             0x0004
#define RPC_FC_PROC_PF_IN               0x0008
#define RPC_FC_PROC_PF_OUT              0x0010
#define RPC_FC_PROC_PF_RETURN           0x0020
#define RPC_FC_PROC_PF_BASETYPE         0x0040
#define RPC_FC_PROC_PF_BYVAL            0x0080
#define RPC_FC_PROC_PF_SIMPLEREF        0x0100
#define RPC_FC_PROC_PF_DONTFREEINST     0x0200
#define RPC_FC_PROC_PF_SAVEASYNC        0x0400
#define RPC_FC_PROC_PF_SRVALLOCSIZE     0xe000 /* in 8 byte units */

/* correlation types */
#define RPC_FC_NORMAL_CONFORMANCE		0x00
#define RPC_FC_POINTER_CONFORMANCE		0x10
#define RPC_FC_TOP_LEVEL_CONFORMANCE		0x20
#define RPC_FC_CONSTANT_CONFORMANCE		0x40
#define RPC_FC_TOP_LEVEL_MULTID_CONFORMANCE	0x80

/* user marshal flags */
#define USER_MARSHAL_UNIQUE	0x80
#define USER_MARSHAL_REF	0x40
#define USER_MARSHAL_POINTER	0xc0
#define USER_MARSHAL_IID	0x20

/* context handle flags */
#define NDR_CONTEXT_HANDLE_CANNOT_BE_NULL   0x01
#define NDR_CONTEXT_HANDLE_SERIALIZE        0x02
#define NDR_CONTEXT_HANDLE_NO_SERIALIZE     0x04
#define NDR_STRICT_CONTEXT_HANDLE           0x08

#endif /* __WINE_RPCFC_H */
