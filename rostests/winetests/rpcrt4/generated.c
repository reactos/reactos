/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "rpcndr.h"
#include "rpcproxy.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: Not sure if is possible to do without compiler extension
 *        (if type is not just a name that is, if so the normal)
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_I_RPC_HANDLE(void)
{
    /* I_RPC_HANDLE */
    TEST_TYPE(I_RPC_HANDLE, 4, 4);
}

static void test_pack_RPC_STATUS(void)
{
    /* RPC_STATUS */
    TEST_TYPE(RPC_STATUS, 4, 4);
    TEST_TYPE_SIGNED(RPC_STATUS);
}

static void test_pack_PRPC_POLICY(void)
{
    /* PRPC_POLICY */
    TEST_TYPE(PRPC_POLICY, 4, 4);
    TEST_TYPE_POINTER(PRPC_POLICY, 12, 4);
}

static void test_pack_RPC_AUTH_IDENTITY_HANDLE(void)
{
    /* RPC_AUTH_IDENTITY_HANDLE */
    TEST_TYPE(RPC_AUTH_IDENTITY_HANDLE, 4, 4);
}

static void test_pack_RPC_AUTH_KEY_RETRIEVAL_FN(void)
{
    /* RPC_AUTH_KEY_RETRIEVAL_FN */
}

static void test_pack_RPC_AUTHZ_HANDLE(void)
{
    /* RPC_AUTHZ_HANDLE */
    TEST_TYPE(RPC_AUTHZ_HANDLE, 4, 4);
}

static void test_pack_RPC_BINDING_HANDLE(void)
{
    /* RPC_BINDING_HANDLE */
    TEST_TYPE(RPC_BINDING_HANDLE, 4, 4);
}

static void test_pack_RPC_BINDING_VECTOR(void)
{
    /* RPC_BINDING_VECTOR (pack 4) */
    TEST_TYPE(RPC_BINDING_VECTOR, 8, 4);
    TEST_FIELD(RPC_BINDING_VECTOR, unsigned long, Count, 0, 4, 4);
    TEST_FIELD(RPC_BINDING_VECTOR, RPC_BINDING_HANDLE[1], BindingH, 4, 4, 4);
}

static void test_pack_RPC_IF_HANDLE(void)
{
    /* RPC_IF_HANDLE */
    TEST_TYPE(RPC_IF_HANDLE, 4, 4);
}

static void test_pack_RPC_IF_ID(void)
{
    /* RPC_IF_ID (pack 4) */
}

static void test_pack_RPC_POLICY(void)
{
    /* RPC_POLICY (pack 4) */
    TEST_TYPE(RPC_POLICY, 12, 4);
    TEST_FIELD(RPC_POLICY, unsigned int, Length, 0, 4, 4);
    TEST_FIELD(RPC_POLICY, unsigned long, EndpointFlags, 4, 4, 4);
    TEST_FIELD(RPC_POLICY, unsigned long, NICFlags, 8, 4, 4);
}

static void test_pack_UUID_VECTOR(void)
{
    /* UUID_VECTOR (pack 4) */
    TEST_TYPE(UUID_VECTOR, 8, 4);
    TEST_FIELD(UUID_VECTOR, unsigned long, Count, 0, 4, 4);
    TEST_FIELD(UUID_VECTOR, UUID *[1], Uuid, 4, 4, 4);
}

static void test_pack_PRPC_CLIENT_INTERFACE(void)
{
    /* PRPC_CLIENT_INTERFACE */
    TEST_TYPE(PRPC_CLIENT_INTERFACE, 4, 4);
}

static void test_pack_PRPC_DISPATCH_TABLE(void)
{
    /* PRPC_DISPATCH_TABLE */
    TEST_TYPE(PRPC_DISPATCH_TABLE, 4, 4);
    TEST_TYPE_POINTER(PRPC_DISPATCH_TABLE, 12, 4);
}

static void test_pack_PRPC_MESSAGE(void)
{
    /* PRPC_MESSAGE */
    TEST_TYPE(PRPC_MESSAGE, 4, 4);
    TEST_TYPE_POINTER(PRPC_MESSAGE, 44, 4);
}

static void test_pack_PRPC_PROTSEQ_ENDPOINT(void)
{
    /* PRPC_PROTSEQ_ENDPOINT */
    TEST_TYPE(PRPC_PROTSEQ_ENDPOINT, 4, 4);
    TEST_TYPE_POINTER(PRPC_PROTSEQ_ENDPOINT, 8, 4);
}

static void test_pack_PRPC_SERVER_INTERFACE(void)
{
    /* PRPC_SERVER_INTERFACE */
    TEST_TYPE(PRPC_SERVER_INTERFACE, 4, 4);
}

static void test_pack_PRPC_SYNTAX_IDENTIFIER(void)
{
    /* PRPC_SYNTAX_IDENTIFIER */
    TEST_TYPE(PRPC_SYNTAX_IDENTIFIER, 4, 4);
    TEST_TYPE_POINTER(PRPC_SYNTAX_IDENTIFIER, 20, 4);
}

static void test_pack_RPC_CLIENT_INTERFACE(void)
{
    /* RPC_CLIENT_INTERFACE (pack 4) */
    TEST_FIELD(RPC_CLIENT_INTERFACE, unsigned int, Length, 0, 4, 4);
}

static void test_pack_RPC_DISPATCH_FUNCTION(void)
{
    /* RPC_DISPATCH_FUNCTION */
}

static void test_pack_RPC_DISPATCH_TABLE(void)
{
    /* RPC_DISPATCH_TABLE (pack 4) */
    TEST_TYPE(RPC_DISPATCH_TABLE, 12, 4);
    TEST_FIELD(RPC_DISPATCH_TABLE, unsigned int, DispatchTableCount, 0, 4, 4);
    TEST_FIELD(RPC_DISPATCH_TABLE, RPC_DISPATCH_FUNCTION*, DispatchTable, 4, 4, 4);
    TEST_FIELD(RPC_DISPATCH_TABLE, LONG_PTR, Reserved, 8, 4, 4);
}

static void test_pack_RPC_MESSAGE(void)
{
    /* RPC_MESSAGE (pack 4) */
    TEST_TYPE(RPC_MESSAGE, 44, 4);
    TEST_FIELD(RPC_MESSAGE, RPC_BINDING_HANDLE, Handle, 0, 4, 4);
    TEST_FIELD(RPC_MESSAGE, unsigned long, DataRepresentation, 4, 4, 4);
    TEST_FIELD(RPC_MESSAGE, void*, Buffer, 8, 4, 4);
    TEST_FIELD(RPC_MESSAGE, unsigned int, BufferLength, 12, 4, 4);
    TEST_FIELD(RPC_MESSAGE, unsigned int, ProcNum, 16, 4, 4);
    TEST_FIELD(RPC_MESSAGE, PRPC_SYNTAX_IDENTIFIER, TransferSyntax, 20, 4, 4);
    TEST_FIELD(RPC_MESSAGE, void*, RpcInterfaceInformation, 24, 4, 4);
    TEST_FIELD(RPC_MESSAGE, void*, ReservedForRuntime, 28, 4, 4);
    TEST_FIELD(RPC_MESSAGE, RPC_MGR_EPV*, ManagerEpv, 32, 4, 4);
    TEST_FIELD(RPC_MESSAGE, void*, ImportContext, 36, 4, 4);
    TEST_FIELD(RPC_MESSAGE, unsigned long, RpcFlags, 40, 4, 4);
}

static void test_pack_RPC_PROTSEQ_ENDPOINT(void)
{
    /* RPC_PROTSEQ_ENDPOINT (pack 4) */
    TEST_TYPE(RPC_PROTSEQ_ENDPOINT, 8, 4);
    TEST_FIELD(RPC_PROTSEQ_ENDPOINT, unsigned char*, RpcProtocolSequence, 0, 4, 4);
    TEST_FIELD(RPC_PROTSEQ_ENDPOINT, unsigned char*, Endpoint, 4, 4, 4);
}

static void test_pack_RPC_SERVER_INTERFACE(void)
{
    /* RPC_SERVER_INTERFACE (pack 4) */
    TEST_FIELD(RPC_SERVER_INTERFACE, unsigned int, Length, 0, 4, 4);
}

static void test_pack_RPC_SYNTAX_IDENTIFIER(void)
{
    /* RPC_SYNTAX_IDENTIFIER (pack 4) */
    TEST_TYPE(RPC_SYNTAX_IDENTIFIER, 20, 4);
    TEST_FIELD(RPC_SYNTAX_IDENTIFIER, GUID, SyntaxGUID, 0, 16, 4);
    TEST_FIELD(RPC_SYNTAX_IDENTIFIER, RPC_VERSION, SyntaxVersion, 16, 4, 2);
}

static void test_pack_RPC_VERSION(void)
{
    /* RPC_VERSION (pack 4) */
    TEST_TYPE(RPC_VERSION, 4, 2);
    TEST_FIELD(RPC_VERSION, unsigned short, MajorVersion, 0, 2, 2);
    TEST_FIELD(RPC_VERSION, unsigned short, MinorVersion, 2, 2, 2);
}

static void test_pack_ARRAY_INFO(void)
{
    /* ARRAY_INFO (pack 4) */
    TEST_TYPE(ARRAY_INFO, 24, 4);
    TEST_FIELD(ARRAY_INFO, long, Dimension, 0, 4, 4);
    TEST_FIELD(ARRAY_INFO, unsigned long *, BufferConformanceMark, 4, 4, 4);
    TEST_FIELD(ARRAY_INFO, unsigned long *, BufferVarianceMark, 8, 4, 4);
    TEST_FIELD(ARRAY_INFO, unsigned long *, MaxCountArray, 12, 4, 4);
    TEST_FIELD(ARRAY_INFO, unsigned long *, OffsetArray, 16, 4, 4);
    TEST_FIELD(ARRAY_INFO, unsigned long *, ActualCountArray, 20, 4, 4);
}

static void test_pack_COMM_FAULT_OFFSETS(void)
{
    /* COMM_FAULT_OFFSETS (pack 4) */
    TEST_TYPE(COMM_FAULT_OFFSETS, 4, 2);
    TEST_FIELD(COMM_FAULT_OFFSETS, short, CommOffset, 0, 2, 2);
    TEST_FIELD(COMM_FAULT_OFFSETS, short, FaultOffset, 2, 2, 2);
}

static void test_pack_CS_STUB_INFO(void)
{
    /* CS_STUB_INFO (pack 4) */
    TEST_TYPE(CS_STUB_INFO, 12, 4);
    TEST_FIELD(CS_STUB_INFO, unsigned long, WireCodeset, 0, 4, 4);
    TEST_FIELD(CS_STUB_INFO, unsigned long, DesiredReceivingCodeset, 4, 4, 4);
    TEST_FIELD(CS_STUB_INFO, void *, CSArrayInfo, 8, 4, 4);
}

static void test_pack_EXPR_EVAL(void)
{
    /* EXPR_EVAL */
}

static void test_pack_FULL_PTR_TO_REFID_ELEMENT(void)
{
    /* FULL_PTR_TO_REFID_ELEMENT (pack 4) */
    TEST_TYPE(FULL_PTR_TO_REFID_ELEMENT, 16, 4);
    TEST_FIELD(FULL_PTR_TO_REFID_ELEMENT, struct _FULL_PTR_TO_REFID_ELEMENT *, Next, 0, 4, 4);
    TEST_FIELD(FULL_PTR_TO_REFID_ELEMENT, void *, Pointer, 4, 4, 4);
    TEST_FIELD(FULL_PTR_TO_REFID_ELEMENT, unsigned long, RefId, 8, 4, 4);
    TEST_FIELD(FULL_PTR_TO_REFID_ELEMENT, unsigned char, State, 12, 1, 1);
}

static void test_pack_FULL_PTR_XLAT_TABLES(void)
{
    /* FULL_PTR_XLAT_TABLES (pack 4) */
}

static void test_pack_GENERIC_BINDING_INFO(void)
{
    /* GENERIC_BINDING_INFO (pack 4) */
    TEST_FIELD(GENERIC_BINDING_INFO, void *, pObj, 0, 4, 4);
    TEST_FIELD(GENERIC_BINDING_INFO, unsigned int, Size, 4, 4, 4);
}

static void test_pack_GENERIC_BINDING_ROUTINE_PAIR(void)
{
    /* GENERIC_BINDING_ROUTINE_PAIR (pack 4) */
}

static void test_pack_MALLOC_FREE_STRUCT(void)
{
    /* MALLOC_FREE_STRUCT (pack 4) */
}

static void test_pack_MIDL_FORMAT_STRING(void)
{
    /* MIDL_FORMAT_STRING (pack 4) */
    TEST_FIELD(MIDL_FORMAT_STRING, short, Pad, 0, 2, 2);
}

static void test_pack_MIDL_SERVER_INFO(void)
{
    /* MIDL_SERVER_INFO (pack 4) */
    TEST_TYPE(MIDL_SERVER_INFO, 32, 4);
    TEST_FIELD(MIDL_SERVER_INFO, PMIDL_STUB_DESC, pStubDesc, 0, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, SERVER_ROUTINE *, DispatchTable, 4, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, PFORMAT_STRING, ProcString, 8, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, unsigned short *, FmtStringOffset, 12, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, STUB_THUNK *, ThunkTable, 16, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, PRPC_SYNTAX_IDENTIFIER, pTransferSyntax, 20, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, ULONG_PTR, nCount, 24, 4, 4);
    TEST_FIELD(MIDL_SERVER_INFO, PMIDL_SYNTAX_INFO, pSyntaxInfo, 28, 4, 4);
}

static void test_pack_MIDL_STUB_DESC(void)
{
    /* MIDL_STUB_DESC (pack 4) */
    TEST_FIELD(MIDL_STUB_DESC, void *, RpcInterfaceInformation, 0, 4, 4);
}

static void test_pack_MIDL_STUB_MESSAGE(void)
{
    /* MIDL_STUB_MESSAGE (pack 4) */
    TEST_FIELD(MIDL_STUB_MESSAGE, PRPC_MESSAGE, RpcMsg, 0, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, Buffer, 4, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, BufferStart, 8, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, BufferEnd, 12, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, BufferMark, 16, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned long, BufferLength, 20, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned long, MemorySize, 24, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, Memory, 28, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, int, IsClient, 32, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, int, ReuseBuffer, 36, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, struct NDR_ALLOC_ALL_NODES_CONTEXT *, pAllocAllNodesContext, 40, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, struct NDR_POINTER_QUEUE_STATE *, pPointerQueueState, 44, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, int, IgnoreEmbeddedPointers, 48, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char *, PointerBufferMark, 52, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char, fBufferValid, 56, 1, 1);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned char, uFlags, 57, 1, 1);
    TEST_FIELD(MIDL_STUB_MESSAGE, ULONG_PTR, MaxCount, 60, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned long, Offset, 64, 4, 4);
    TEST_FIELD(MIDL_STUB_MESSAGE, unsigned long, ActualCount, 68, 4, 4);
}

static void test_pack_MIDL_STUBLESS_PROXY_INFO(void)
{
    /* MIDL_STUBLESS_PROXY_INFO (pack 4) */
    TEST_TYPE(MIDL_STUBLESS_PROXY_INFO, 24, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, PMIDL_STUB_DESC, pStubDesc, 0, 4, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, PFORMAT_STRING, ProcFormatString, 4, 4, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, unsigned short *, FormatStringOffset, 8, 4, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, PRPC_SYNTAX_IDENTIFIER, pTransferSyntax, 12, 4, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, ULONG_PTR, nCount, 16, 4, 4);
    TEST_FIELD(MIDL_STUBLESS_PROXY_INFO, PMIDL_SYNTAX_INFO, pSyntaxInfo, 20, 4, 4);
}

static void test_pack_MIDL_SYNTAX_INFO(void)
{
    /* MIDL_SYNTAX_INFO (pack 4) */
}

static void test_pack_NDR_CCONTEXT(void)
{
    /* NDR_CCONTEXT */
    TEST_TYPE(NDR_CCONTEXT, 4, 4);
}

static void test_pack_NDR_NOTIFY_ROUTINE(void)
{
    /* NDR_NOTIFY_ROUTINE */
}

static void test_pack_NDR_NOTIFY2_ROUTINE(void)
{
    /* NDR_NOTIFY2_ROUTINE */
}

static void test_pack_NDR_RUNDOWN(void)
{
    /* NDR_RUNDOWN */
}

static void test_pack_NDR_SCONTEXT(void)
{
    /* NDR_SCONTEXT */
    TEST_TYPE(NDR_SCONTEXT, 4, 4);
}

static void test_pack_PARRAY_INFO(void)
{
    /* PARRAY_INFO */
    TEST_TYPE(PARRAY_INFO, 4, 4);
    TEST_TYPE_POINTER(PARRAY_INFO, 24, 4);
}

static void test_pack_PFORMAT_STRING(void)
{
    /* PFORMAT_STRING */
    TEST_TYPE(PFORMAT_STRING, 4, 4);
}

static void test_pack_PFULL_PTR_TO_REFID_ELEMENT(void)
{
    /* PFULL_PTR_TO_REFID_ELEMENT */
    TEST_TYPE(PFULL_PTR_TO_REFID_ELEMENT, 4, 4);
    TEST_TYPE_POINTER(PFULL_PTR_TO_REFID_ELEMENT, 16, 4);
}

static void test_pack_PFULL_PTR_XLAT_TABLES(void)
{
    /* PFULL_PTR_XLAT_TABLES */
    TEST_TYPE(PFULL_PTR_XLAT_TABLES, 4, 4);
}

static void test_pack_PGENERIC_BINDING_INFO(void)
{
    /* PGENERIC_BINDING_INFO */
    TEST_TYPE(PGENERIC_BINDING_INFO, 4, 4);
}

static void test_pack_PGENERIC_BINDING_ROUTINE_PAIR(void)
{
    /* PGENERIC_BINDING_ROUTINE_PAIR */
    TEST_TYPE(PGENERIC_BINDING_ROUTINE_PAIR, 4, 4);
}

static void test_pack_PMIDL_SERVER_INFO(void)
{
    /* PMIDL_SERVER_INFO */
    TEST_TYPE(PMIDL_SERVER_INFO, 4, 4);
    TEST_TYPE_POINTER(PMIDL_SERVER_INFO, 32, 4);
}

static void test_pack_PMIDL_STUB_DESC(void)
{
    /* PMIDL_STUB_DESC */
    TEST_TYPE(PMIDL_STUB_DESC, 4, 4);
}

static void test_pack_PMIDL_STUB_MESSAGE(void)
{
    /* PMIDL_STUB_MESSAGE */
    TEST_TYPE(PMIDL_STUB_MESSAGE, 4, 4);
}

static void test_pack_PMIDL_STUBLESS_PROXY_INFO(void)
{
    /* PMIDL_STUBLESS_PROXY_INFO */
    TEST_TYPE(PMIDL_STUBLESS_PROXY_INFO, 4, 4);
    TEST_TYPE_POINTER(PMIDL_STUBLESS_PROXY_INFO, 24, 4);
}

static void test_pack_PMIDL_SYNTAX_INFO(void)
{
    /* PMIDL_SYNTAX_INFO */
    TEST_TYPE(PMIDL_SYNTAX_INFO, 4, 4);
}

static void test_pack_PNDR_ASYNC_MESSAGE(void)
{
    /* PNDR_ASYNC_MESSAGE */
    TEST_TYPE(PNDR_ASYNC_MESSAGE, 4, 4);
}

static void test_pack_PNDR_CORRELATION_INFO(void)
{
    /* PNDR_CORRELATION_INFO */
    TEST_TYPE(PNDR_CORRELATION_INFO, 4, 4);
}

static void test_pack_PSCONTEXT_QUEUE(void)
{
    /* PSCONTEXT_QUEUE */
    TEST_TYPE(PSCONTEXT_QUEUE, 4, 4);
    TEST_TYPE_POINTER(PSCONTEXT_QUEUE, 8, 4);
}

static void test_pack_PXMIT_ROUTINE_QUINTUPLE(void)
{
    /* PXMIT_ROUTINE_QUINTUPLE */
    TEST_TYPE(PXMIT_ROUTINE_QUINTUPLE, 4, 4);
}

static void test_pack_SCONTEXT_QUEUE(void)
{
    /* SCONTEXT_QUEUE (pack 4) */
    TEST_TYPE(SCONTEXT_QUEUE, 8, 4);
    TEST_FIELD(SCONTEXT_QUEUE, unsigned long, NumberOfObjects, 0, 4, 4);
    TEST_FIELD(SCONTEXT_QUEUE, NDR_SCONTEXT *, ArrayOfObjects, 4, 4, 4);
}

static void test_pack_SERVER_ROUTINE(void)
{
    /* SERVER_ROUTINE */
}

static void test_pack_STUB_THUNK(void)
{
    /* STUB_THUNK */
}

static void test_pack_USER_MARSHAL_CB(void)
{
    /* USER_MARSHAL_CB (pack 4) */
    TEST_FIELD(USER_MARSHAL_CB, unsigned long, Flags, 0, 4, 4);
    TEST_FIELD(USER_MARSHAL_CB, PMIDL_STUB_MESSAGE, pStubMsg, 4, 4, 4);
    TEST_FIELD(USER_MARSHAL_CB, PFORMAT_STRING, pReserve, 8, 4, 4);
    TEST_FIELD(USER_MARSHAL_CB, unsigned long, Signature, 12, 4, 4);
}

static void test_pack_USER_MARSHAL_FREEING_ROUTINE(void)
{
    /* USER_MARSHAL_FREEING_ROUTINE */
}

static void test_pack_USER_MARSHAL_MARSHALLING_ROUTINE(void)
{
    /* USER_MARSHAL_MARSHALLING_ROUTINE */
}

static void test_pack_USER_MARSHAL_ROUTINE_QUADRUPLE(void)
{
    /* USER_MARSHAL_ROUTINE_QUADRUPLE (pack 4) */
}

static void test_pack_USER_MARSHAL_SIZING_ROUTINE(void)
{
    /* USER_MARSHAL_SIZING_ROUTINE */
}

static void test_pack_USER_MARSHAL_UNMARSHALLING_ROUTINE(void)
{
    /* USER_MARSHAL_UNMARSHALLING_ROUTINE */
}

static void test_pack_XMIT_HELPER_ROUTINE(void)
{
    /* XMIT_HELPER_ROUTINE */
}

static void test_pack_XMIT_ROUTINE_QUINTUPLE(void)
{
    /* XMIT_ROUTINE_QUINTUPLE (pack 4) */
}

static void test_pack_PRPC_STUB_FUNCTION(void)
{
    /* PRPC_STUB_FUNCTION */
}

static void test_pack(void)
{
    test_pack_ARRAY_INFO();
    test_pack_COMM_FAULT_OFFSETS();
    test_pack_CS_STUB_INFO();
    test_pack_EXPR_EVAL();
    test_pack_FULL_PTR_TO_REFID_ELEMENT();
    test_pack_FULL_PTR_XLAT_TABLES();
    test_pack_GENERIC_BINDING_INFO();
    test_pack_GENERIC_BINDING_ROUTINE_PAIR();
    test_pack_I_RPC_HANDLE();
    test_pack_MALLOC_FREE_STRUCT();
    test_pack_MIDL_FORMAT_STRING();
    test_pack_MIDL_SERVER_INFO();
    test_pack_MIDL_STUBLESS_PROXY_INFO();
    test_pack_MIDL_STUB_DESC();
    test_pack_MIDL_STUB_MESSAGE();
    test_pack_MIDL_SYNTAX_INFO();
    test_pack_NDR_CCONTEXT();
    test_pack_NDR_NOTIFY2_ROUTINE();
    test_pack_NDR_NOTIFY_ROUTINE();
    test_pack_NDR_RUNDOWN();
    test_pack_NDR_SCONTEXT();
    test_pack_PARRAY_INFO();
    test_pack_PFORMAT_STRING();
    test_pack_PFULL_PTR_TO_REFID_ELEMENT();
    test_pack_PFULL_PTR_XLAT_TABLES();
    test_pack_PGENERIC_BINDING_INFO();
    test_pack_PGENERIC_BINDING_ROUTINE_PAIR();
    test_pack_PMIDL_SERVER_INFO();
    test_pack_PMIDL_STUBLESS_PROXY_INFO();
    test_pack_PMIDL_STUB_DESC();
    test_pack_PMIDL_STUB_MESSAGE();
    test_pack_PMIDL_SYNTAX_INFO();
    test_pack_PNDR_ASYNC_MESSAGE();
    test_pack_PNDR_CORRELATION_INFO();
    test_pack_PRPC_CLIENT_INTERFACE();
    test_pack_PRPC_DISPATCH_TABLE();
    test_pack_PRPC_MESSAGE();
    test_pack_PRPC_POLICY();
    test_pack_PRPC_PROTSEQ_ENDPOINT();
    test_pack_PRPC_SERVER_INTERFACE();
    test_pack_PRPC_STUB_FUNCTION();
    test_pack_PRPC_SYNTAX_IDENTIFIER();
    test_pack_PSCONTEXT_QUEUE();
    test_pack_PXMIT_ROUTINE_QUINTUPLE();
    test_pack_RPC_AUTHZ_HANDLE();
    test_pack_RPC_AUTH_IDENTITY_HANDLE();
    test_pack_RPC_AUTH_KEY_RETRIEVAL_FN();
    test_pack_RPC_BINDING_HANDLE();
    test_pack_RPC_BINDING_VECTOR();
    test_pack_RPC_CLIENT_INTERFACE();
    test_pack_RPC_DISPATCH_FUNCTION();
    test_pack_RPC_DISPATCH_TABLE();
    test_pack_RPC_IF_HANDLE();
    test_pack_RPC_IF_ID();
    test_pack_RPC_MESSAGE();
    test_pack_RPC_POLICY();
    test_pack_RPC_PROTSEQ_ENDPOINT();
    test_pack_RPC_SERVER_INTERFACE();
    test_pack_RPC_STATUS();
    test_pack_RPC_SYNTAX_IDENTIFIER();
    test_pack_RPC_VERSION();
    test_pack_SCONTEXT_QUEUE();
    test_pack_SERVER_ROUTINE();
    test_pack_STUB_THUNK();
    test_pack_USER_MARSHAL_CB();
    test_pack_USER_MARSHAL_FREEING_ROUTINE();
    test_pack_USER_MARSHAL_MARSHALLING_ROUTINE();
    test_pack_USER_MARSHAL_ROUTINE_QUADRUPLE();
    test_pack_USER_MARSHAL_SIZING_ROUTINE();
    test_pack_USER_MARSHAL_UNMARSHALLING_ROUTINE();
    test_pack_UUID_VECTOR();
    test_pack_XMIT_HELPER_ROUTINE();
    test_pack_XMIT_ROUTINE_QUINTUPLE();
}

START_TEST(generated)
{
    test_pack();
}
