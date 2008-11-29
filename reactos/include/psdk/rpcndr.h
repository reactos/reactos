/*
 * Copyright (C) 2000 Francois Gouget
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

#ifndef __RPCNDR_H_VERSION__
/* FIXME: What version?   Perhaps something is better than nothing, however incorrect */
#define __RPCNDR_H_VERSION__ ( 399 )
#endif

#ifndef __WINE_RPCNDR_H
#define __WINE_RPCNDR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <basetsd.h>

#undef CONST_VTBL
#ifdef CONST_VTABLE
# define CONST_VTBL const
#else
# define CONST_VTBL
#endif

/* stupid #if can't handle casts... this __stupidity
   is just a workaround for that limitation */

#define __NDR_CHAR_REP_MASK  0x000fL
#define __NDR_INT_REP_MASK   0x00f0L
#define __NDR_FLOAT_REP_MASK 0xff00L

#define __NDR_IEEE_FLOAT     0x0000L
#define __NDR_VAX_FLOAT      0x0100L
#define __NDR_IBM_FLOAT      0x0300L

#define __NDR_ASCII_CHAR     0x0000L
#define __NDR_EBCDIC_CHAR    0x0001L

#define __NDR_LITTLE_ENDIAN  0x0010L
#define __NDR_BIG_ENDIAN     0x0000L

/* Mac's are special */
#if defined(__RPC_MAC__)
# define __NDR_LOCAL_DATA_REPRESENTATION \
    (__NDR_IEEE_FLOAT | __NDR_ASCII_CHAR | __NDR_BIG_ENDIAN)
#else
# define __NDR_LOCAL_DATA_REPRESENTATION \
    (__NDR_IEEE_FLOAT | __NDR_ASCII_CHAR | __NDR_LITTLE_ENDIAN)
#endif

#define __NDR_LOCAL_ENDIAN \
  (__NDR_LOCAL_DATA_REPRESENTATION & __NDR_INT_REP_MASK)

/* for convenience, define NDR_LOCAL_IS_BIG_ENDIAN iff it is */
#if __NDR_LOCAL_ENDIAN == __NDR_BIG_ENDIAN
# define NDR_LOCAL_IS_BIG_ENDIAN
#elif __NDR_LOCAL_ENDIAN == __NDR_LITTLE_ENDIAN
# undef NDR_LOCAL_IS_BIG_ENDIAN
#else
# error alien NDR_LOCAL_ENDIAN - Greg botched the defines again, please report
#endif

/* finally, do the casts like Microsoft */

#define NDR_CHAR_REP_MASK             ((ULONG) __NDR_CHAR_REP_MASK)
#define NDR_INT_REP_MASK              ((ULONG) __NDR_INT_REP_MASK)
#define NDR_FLOAT_REP_MASK            ((ULONG) __NDR_FLOAT_REP_MASK)
#define NDR_IEEE_FLOAT                ((ULONG) __NDR_IEEE_FLOAT)
#define NDR_VAX_FLOAT                 ((ULONG) __NDR_VAX_FLOAT)
#define NDR_IBM_FLOAT                 ((ULONG) __NDR_IBM_FLOAT)
#define NDR_ASCII_CHAR                ((ULONG) __NDR_ASCII_CHAR)
#define NDR_EBCDIC_CHAR               ((ULONG) __NDR_EBCDIC_CHAR)
#define NDR_LITTLE_ENDIAN             ((ULONG) __NDR_LITTLE_ENDIAN)
#define NDR_BIG_ENDIAN                ((ULONG) __NDR_BIG_ENDIAN)
#define NDR_LOCAL_DATA_REPRESENTATION ((ULONG) __NDR_LOCAL_DATA_REPRESENTATION)
#define NDR_LOCAL_ENDIAN              ((ULONG) __NDR_LOCAL_ENDIAN)


#define TARGET_IS_NT50_OR_LATER 1
#define TARGET_IS_NT40_OR_LATER 1
#define TARGET_IS_NT351_OR_WIN95_OR_LATER 1

#define small char
typedef unsigned char byte;
#define hyper __int64
#define MIDL_uhyper unsigned __int64
typedef unsigned char boolean;

#define __RPC_CALLEE WINAPI
#define RPC_VAR_ENTRY __cdecl
#define NDR_SHAREABLE static

#define MIDL_ascii_strlen(s) strlen(s)
#define MIDL_ascii_strcpy(d,s) strcpy(d,s)
#define MIDL_memset(d,v,n) memset(d,v,n)
#define midl_user_free MIDL_user_free
#define midl_user_allocate MIDL_user_allocate

#define NdrFcShort(s) (unsigned char)(s & 0xff), (unsigned char)((s & 0xff00) >> 8)
#define NdrFcLong(s)  (unsigned char)(s & 0xff), (unsigned char)((s & 0x0000ff00) >> 8), \
  (unsigned char)((s & 0x00ff0000) >> 16), (unsigned char)(s >> 24)

#define RPC_BAD_STUB_DATA_EXCEPTION_FILTER  \
  ((RpcExceptionCode() == STATUS_ACCESS_VIOLATION) || \
   (RpcExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) || \
   (RpcExceptionCode() == RPC_X_BAD_STUB_DATA) || \
   (RpcExceptionCode() == RPC_S_INVALID_BOUND))

typedef struct
{
  void *pad[2];
  void *userContext;
} *NDR_SCONTEXT;

#define NDRSContextValue(hContext) (&(hContext)->userContext)
#define cbNDRContext 20

typedef void (__RPC_USER *NDR_RUNDOWN)(void *context);
typedef void (__RPC_USER *NDR_NOTIFY_ROUTINE)(void);
typedef void (__RPC_USER *NDR_NOTIFY2_ROUTINE)(boolean flag);

#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x)   struct

struct _MIDL_STUB_MESSAGE;
struct _MIDL_STUB_DESC;
struct _FULL_PTR_XLAT_TABLES;
struct NDR_ALLOC_ALL_NODES_CONTEXT;
struct NDR_POINTER_QUEUE_STATE;

typedef unsigned char *RPC_BUFPTR;
typedef unsigned long RPC_LENGTH;
typedef void (__RPC_USER *EXPR_EVAL)(struct _MIDL_STUB_MESSAGE *);
typedef const unsigned char *PFORMAT_STRING;

typedef struct
{
  LONG Dimension;
  ULONG *BufferConformanceMark;
  ULONG *BufferVarianceMark;
  ULONG *MaxCountArray;
  ULONG *OffsetArray;
  ULONG *ActualCountArray;
} ARRAY_INFO, *PARRAY_INFO;

typedef struct
{
  ULONG WireCodeset;
  ULONG DesiredReceivingCodeset;
  void *CSArrayInfo;
} CS_STUB_INFO;

typedef struct _NDR_PIPE_DESC *PNDR_PIPE_DESC;
typedef struct _NDR_PIPE_MESSAGE *PNDR_PIPE_MESSAGE;
typedef struct _NDR_ASYNC_MESSAGE *PNDR_ASYNC_MESSAGE;
typedef struct _NDR_CORRELATION_INFO *PNDR_CORRELATION_INFO;

#include <pshpack4.h>
typedef struct _MIDL_STUB_MESSAGE
{
  PRPC_MESSAGE RpcMsg;
  unsigned char *Buffer;
  unsigned char *BufferStart;
  unsigned char *BufferEnd;
  unsigned char *BufferMark;
  ULONG BufferLength;
  ULONG MemorySize;
  unsigned char *Memory;
  unsigned char IsClient;
  unsigned char Pad;
  unsigned short uFlags2;
  int ReuseBuffer;
  struct NDR_ALLOC_ALL_NODES_CONTEXT *pAllocAllNodesContext;
  struct NDR_POINTER_QUEUE_STATE *pPointerQueueState;
  int IgnoreEmbeddedPointers;
  unsigned char *PointerBufferMark;
  unsigned char CorrDespIncrement;
  unsigned char uFlags;
  unsigned short UniquePtrCount;
  ULONG_PTR MaxCount;
  ULONG Offset;
  ULONG ActualCount;
  void * (__WINE_ALLOC_SIZE(1) __RPC_API *pfnAllocate)(size_t);
  void (__RPC_API *pfnFree)(void *);
  unsigned char *StackTop;
  unsigned char *pPresentedType;
  unsigned char *pTransmitType;
  handle_t SavedHandle;
  const struct _MIDL_STUB_DESC *StubDesc;
  struct _FULL_PTR_XLAT_TABLES *FullPtrXlatTables;
  ULONG FullPtrRefId;
  ULONG PointerLength;
  int fInDontFree:1;
  int fDontCallFreeInst:1;
  int fInOnlyParam:1;
  int fHasReturn:1;
  int fHasExtensions:1;
  int fHasNewCorrDesc:1;
  int fIsIn:1;
  int fIsOut:1;
  int fIsOicf:1;
  int fBufferValid:1;
  int fHasMemoryValidateCallback:1;
  int fInFree:1;
  int fNeedMCCP:1;
  int fUnused:3;
  int fUnused2:16;
  unsigned long dwDestContext;
  void *pvDestContext;
  NDR_SCONTEXT *SavedContextHandles;
  LONG ParamNumber;
  struct IRpcChannelBuffer *pRpcChannelBuffer;
  PARRAY_INFO pArrayInfo;
  ULONG *SizePtrCountArray;
  ULONG *SizePtrOffsetArray;
  ULONG *SizePtrLengthArray;
  void *pArgQueue;
  unsigned long dwStubPhase;
  void *LowStackMark;
  PNDR_ASYNC_MESSAGE pAsyncMsg;
  PNDR_CORRELATION_INFO pCorrInfo;
  unsigned char *pCorrMemory;
  void *pMemoryList;
  CS_STUB_INFO *pCSInfo;
  unsigned char *ConformanceMark;
  unsigned char *VarianceMark;
  INT_PTR Unused; /* BackingStoreLowMark on IA64 */
  struct _NDR_PROC_CONTEXT *pContext;
  void* ContextHandleHash;
  void* pUserMarshalList;
  INT_PTR Reserved51_3;
  INT_PTR Reserved51_4;
  INT_PTR Reserved51_5;
} MIDL_STUB_MESSAGE, *PMIDL_STUB_MESSAGE;
#include <poppack.h>

typedef void * (__RPC_API * GENERIC_BINDING_ROUTINE)(void *);
typedef void (__RPC_API * GENERIC_UNBIND_ROUTINE)(void *, unsigned char *);

typedef struct _GENERIC_BINDING_ROUTINE_PAIR
{
  GENERIC_BINDING_ROUTINE pfnBind;
  GENERIC_UNBIND_ROUTINE pfnUnbind;
} GENERIC_BINDING_ROUTINE_PAIR, *PGENERIC_BINDING_ROUTINE_PAIR;

typedef struct __GENERIC_BINDING_INFO
{
  void *pObj;
  unsigned int Size;
  GENERIC_BINDING_ROUTINE pfnBind;
  GENERIC_UNBIND_ROUTINE pfnUnbind;
} GENERIC_BINDING_INFO, *PGENERIC_BINDING_INFO;

typedef void (__RPC_USER *XMIT_HELPER_ROUTINE)(PMIDL_STUB_MESSAGE);

typedef struct _XMIT_ROUTINE_QUINTUPLE
{
  XMIT_HELPER_ROUTINE pfnTranslateToXmit;
  XMIT_HELPER_ROUTINE pfnTranslateFromXmit;
  XMIT_HELPER_ROUTINE pfnFreeXmit;
  XMIT_HELPER_ROUTINE pfnFreeInst;
} XMIT_ROUTINE_QUINTUPLE, *PXMIT_ROUTINE_QUINTUPLE;

typedef ULONG (__RPC_USER *USER_MARSHAL_SIZING_ROUTINE)(ULONG *, ULONG, void *);
typedef unsigned char * (__RPC_USER *USER_MARSHAL_MARSHALLING_ROUTINE)(ULONG *, unsigned char *, void *);
typedef unsigned char * (__RPC_USER *USER_MARSHAL_UNMARSHALLING_ROUTINE)(ULONG *, unsigned char *, void *);
typedef void (__RPC_USER *USER_MARSHAL_FREEING_ROUTINE)(ULONG *, void *);

typedef struct _USER_MARSHAL_ROUTINE_QUADRUPLE
{
  USER_MARSHAL_SIZING_ROUTINE pfnBufferSize;
  USER_MARSHAL_MARSHALLING_ROUTINE pfnMarshall;
  USER_MARSHAL_UNMARSHALLING_ROUTINE pfnUnmarshall;
  USER_MARSHAL_FREEING_ROUTINE pfnFree;
} USER_MARSHAL_ROUTINE_QUADRUPLE;

/* 'USRC' */
#define USER_MARSHAL_CB_SIGNATURE \
	( ( (unsigned long)'U' << 24 ) | ( (unsigned long)'S' << 16 ) | \
	  ( (unsigned long)'R' << 8  ) | ( (unsigned long)'C'       ) )

typedef enum
{
    USER_MARSHAL_CB_BUFFER_SIZE,
    USER_MARSHAL_CB_MARSHALL,
    USER_MARSHAL_CB_UNMARSHALL,
    USER_MARSHAL_CB_FREE
} USER_MARSHAL_CB_TYPE;

typedef struct _USER_MARSHAL_CB
{
    ULONG Flags;
    PMIDL_STUB_MESSAGE pStubMsg;
    PFORMAT_STRING pReserve;
    ULONG Signature;
    USER_MARSHAL_CB_TYPE CBType;
    PFORMAT_STRING pFormat;
    PFORMAT_STRING pTypeFormat;
} USER_MARSHAL_CB;

#define USER_CALL_CTXT_MASK(f) ((f) & 0x00ff)
#define USER_CALL_AUX_MASK(f) ((f) & 0xff00)
#define GET_USER_DATA_REP(f) HIWORD(f)

#define USER_CALL_IS_ASYNC 0x0100
#define USER_CALL_NEW_CORRELATION_DESC 0x0200

typedef struct _MALLOC_FREE_STRUCT
{
  void * (__WINE_ALLOC_SIZE(1) __RPC_USER *pfnAllocate)(size_t);
  void   (__RPC_USER *pfnFree)(void *);
} MALLOC_FREE_STRUCT;

typedef struct _COMM_FAULT_OFFSETS
{
  short CommOffset;
  short FaultOffset;
} COMM_FAULT_OFFSETS;

typedef struct _MIDL_STUB_DESC
{
  void *RpcInterfaceInformation;
  void * (__WINE_ALLOC_SIZE(1) __RPC_API *pfnAllocate)(size_t);
  void (__RPC_API *pfnFree)(void *);
  union {
    handle_t *pAutoHandle;
    handle_t *pPrimitiveHandle;
    PGENERIC_BINDING_INFO pGenericBindingInfo;
  } IMPLICIT_HANDLE_INFO;
  const NDR_RUNDOWN *apfnNdrRundownRoutines;
  const GENERIC_BINDING_ROUTINE_PAIR *aGenericBindingRoutinePairs;
  const EXPR_EVAL *apfnExprEval;
  const XMIT_ROUTINE_QUINTUPLE *aXmitQuintuple;
  const unsigned char *pFormatTypes;
  int fCheckBounds;
  ULONG Version;
  MALLOC_FREE_STRUCT *pMallocFreeStruct;
  LONG MIDLVersion;
  const COMM_FAULT_OFFSETS *CommFaultOffsets;
  const USER_MARSHAL_ROUTINE_QUADRUPLE *aUserMarshalQuadruple;
  const NDR_NOTIFY_ROUTINE *NotifyRoutineTable;
  ULONG_PTR mFlags;
  ULONG_PTR Reserved3;
  ULONG_PTR Reserved4;
  ULONG_PTR Reserved5;
} MIDL_STUB_DESC;
typedef const MIDL_STUB_DESC *PMIDL_STUB_DESC;

typedef struct _MIDL_FORMAT_STRING
{
  short Pad;
#if defined(__GNUC__)
  unsigned char Format[0];
#else
  unsigned char Format[1];
#endif
} MIDL_FORMAT_STRING;

typedef struct _MIDL_SYNTAX_INFO
{
  RPC_SYNTAX_IDENTIFIER TransferSyntax;
  RPC_DISPATCH_TABLE* DispatchTable;
  PFORMAT_STRING ProcString;
  const unsigned short* FmtStringOffset;
  PFORMAT_STRING TypeString;
  const void* aUserMarshalQuadruple;
  ULONG_PTR pReserved1;
  ULONG_PTR pReserved2;
} MIDL_SYNTAX_INFO, *PMIDL_SYNTAX_INFO;

typedef void (__RPC_API *STUB_THUNK)( PMIDL_STUB_MESSAGE );

typedef LONG (__RPC_API *SERVER_ROUTINE)();

typedef struct _MIDL_SERVER_INFO_
{
  PMIDL_STUB_DESC pStubDesc;
  const SERVER_ROUTINE *DispatchTable;
  PFORMAT_STRING ProcString;
  const unsigned short *FmtStringOffset;
  const STUB_THUNK *ThunkTable;
  PRPC_SYNTAX_IDENTIFIER pTransferSyntax;
  ULONG_PTR nCount;
  PMIDL_SYNTAX_INFO pSyntaxInfo;
} MIDL_SERVER_INFO, *PMIDL_SERVER_INFO;

typedef struct _MIDL_STUBLESS_PROXY_INFO
{
  PMIDL_STUB_DESC pStubDesc;
  PFORMAT_STRING ProcFormatString;
  const unsigned short *FormatStringOffset;
  PRPC_SYNTAX_IDENTIFIER pTransferSyntax;
  ULONG_PTR nCount;
  PMIDL_SYNTAX_INFO pSyntaxInfo;
} MIDL_STUBLESS_PROXY_INFO, *PMIDL_STUBLESS_PROXY_INFO;

typedef union _CLIENT_CALL_RETURN
{
  void *Pointer;
  LONG_PTR Simple;
} CLIENT_CALL_RETURN;

typedef enum {
  STUB_UNMARSHAL,
  STUB_CALL_SERVER,
  STUB_MARSHAL,
  STUB_CALL_SERVER_NO_HRESULT
} STUB_PHASE;

typedef enum {
  PROXY_CALCSIZE,
  PROXY_GETBUFFER,
  PROXY_MARSHAL,
  PROXY_SENDRECEIVE,
  PROXY_UNMARSHAL
} PROXY_PHASE;

typedef enum {
  XLAT_SERVER = 1,
  XLAT_CLIENT
} XLAT_SIDE;

typedef struct _FULL_PTR_TO_REFID_ELEMENT {
  struct _FULL_PTR_TO_REFID_ELEMENT *Next;
  void *Pointer;
  ULONG RefId;
  unsigned char State;
} FULL_PTR_TO_REFID_ELEMENT, *PFULL_PTR_TO_REFID_ELEMENT;

/* Full pointer translation tables */
typedef struct _FULL_PTR_XLAT_TABLES {
  struct {
    void **XlatTable;
    unsigned char *StateTable;
    ULONG NumberOfEntries;
  } RefIdToPointer;

  struct {
    PFULL_PTR_TO_REFID_ELEMENT *XlatTable;
    ULONG NumberOfBuckets;
    ULONG HashMask;
  } PointerToRefId;

  ULONG                   NextRefId;
  XLAT_SIDE               XlatSide;
} FULL_PTR_XLAT_TABLES,  *PFULL_PTR_XLAT_TABLES;

struct IRpcStubBuffer;

typedef unsigned long error_status_t;

typedef void  * NDR_CCONTEXT;

typedef struct _SCONTEXT_QUEUE {
  ULONG NumberOfObjects;
  NDR_SCONTEXT *ArrayOfObjects;
} SCONTEXT_QUEUE, *PSCONTEXT_QUEUE;

typedef struct _NDR_USER_MARSHAL_INFO_LEVEL1
{
    void *Buffer;
    ULONG BufferSize;
    void * (__WINE_ALLOC_SIZE(1) __RPC_API *pfnAllocate)(size_t);
    void (__RPC_API *pfnFree)(void *);
    struct IRpcChannelBuffer *pRpcChannelBuffer;
    ULONG_PTR Reserved[5];
} NDR_USER_MARSHAL_INFO_LEVEL1;

typedef struct _NDR_USER_MARSHAL_INFO
{
    ULONG InformationLevel;
    union
    {
        NDR_USER_MARSHAL_INFO_LEVEL1 Level1;
    } DUMMYUNIONNAME1;
} NDR_USER_MARSHAL_INFO;

/* Context Handles */

RPCRTAPI RPC_BINDING_HANDLE RPC_ENTRY
  NDRCContextBinding( NDR_CCONTEXT CContext );

RPCRTAPI void RPC_ENTRY
  NDRCContextMarshall( NDR_CCONTEXT CContext, void *pBuff );

RPCRTAPI void RPC_ENTRY
  NDRCContextUnmarshall( NDR_CCONTEXT *pCContext, RPC_BINDING_HANDLE hBinding,
                         void *pBuff, ULONG DataRepresentation );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshall( NDR_SCONTEXT CContext, void *pBuff, NDR_RUNDOWN userRunDownIn );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshall( void *pBuff, ULONG DataRepresentation );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshallEx( RPC_BINDING_HANDLE BindingHandle, NDR_SCONTEXT CContext,
                         void *pBuff, NDR_RUNDOWN userRunDownIn );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshall2( RPC_BINDING_HANDLE BindingHandle, NDR_SCONTEXT CContext,
                        void *pBuff, NDR_RUNDOWN userRunDownIn, void * CtxGuard,
                        ULONG Flags );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshallEx( RPC_BINDING_HANDLE BindingHandle, void *pBuff,
                           ULONG DataRepresentation );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshall2( RPC_BINDING_HANDLE BindingHandle, void *pBuff,
                          ULONG DataRepresentation, void *CtxGuard,
                          ULONG Flags );

RPCRTAPI void RPC_ENTRY
  NdrClientContextMarshall ( PMIDL_STUB_MESSAGE pStubMsg, NDR_CCONTEXT ContextHandle, int fCheck );

RPCRTAPI void RPC_ENTRY
  NdrClientContextUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, NDR_CCONTEXT* pContextHandle,
                              RPC_BINDING_HANDLE BindHandle );

RPCRTAPI void RPC_ENTRY
  NdrServerContextMarshall ( PMIDL_STUB_MESSAGE pStubMsg, NDR_SCONTEXT ContextHandle, NDR_RUNDOWN RundownRoutine );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrServerContextUnmarshall( PMIDL_STUB_MESSAGE pStubMsg );

RPCRTAPI void RPC_ENTRY
  NdrContextHandleSize( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrContextHandleInitialize( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );

RPCRTAPI void RPC_ENTRY
  NdrServerContextNewMarshall( PMIDL_STUB_MESSAGE pStubMsg, NDR_SCONTEXT ContextHandle,
                               NDR_RUNDOWN RundownRoutine, PFORMAT_STRING pFormat );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrServerContextNewUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcSmDestroyClientContext( void **ContextHandle );

RPCRTAPI void RPC_ENTRY
  RpcSsDestroyClientContext( void **ContextHandle );

RPCRTAPI void RPC_ENTRY
  NdrSimpleTypeMarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, unsigned char FormatChar );
RPCRTAPI void RPC_ENTRY
  NdrSimpleTypeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, unsigned char FormatChar );

/* while MS declares each prototype separately, I prefer to use macros for this kind of thing instead */
#define SIMPLE_TYPE_MARSHAL(type) \
RPCRTAPI unsigned char* RPC_ENTRY \
  Ndr##type##Marshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat ); \
RPCRTAPI unsigned char* RPC_ENTRY \
  Ndr##type##Unmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char** ppMemory, PFORMAT_STRING pFormat, unsigned char fMustAlloc ); \
RPCRTAPI void RPC_ENTRY \
  Ndr##type##BufferSize( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat ); \
RPCRTAPI ULONG RPC_ENTRY \
  Ndr##type##MemorySize( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );

#define TYPE_MARSHAL(type) \
  SIMPLE_TYPE_MARSHAL(type) \
RPCRTAPI void RPC_ENTRY \
  Ndr##type##Free( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat );

TYPE_MARSHAL(Pointer)
TYPE_MARSHAL(SimpleStruct)
TYPE_MARSHAL(ConformantStruct)
TYPE_MARSHAL(ConformantVaryingStruct)
TYPE_MARSHAL(ComplexStruct)
TYPE_MARSHAL(FixedArray)
TYPE_MARSHAL(ConformantArray)
TYPE_MARSHAL(ConformantVaryingArray)
TYPE_MARSHAL(VaryingArray)
TYPE_MARSHAL(ComplexArray)
TYPE_MARSHAL(EncapsulatedUnion)
TYPE_MARSHAL(NonEncapsulatedUnion)
TYPE_MARSHAL(ByteCountPointer)
TYPE_MARSHAL(XmitOrRepAs)
TYPE_MARSHAL(UserMarshal)
TYPE_MARSHAL(InterfacePointer)
TYPE_MARSHAL(Range)

SIMPLE_TYPE_MARSHAL(ConformantString)
SIMPLE_TYPE_MARSHAL(NonConformantString)

#undef TYPE_MARSHAL
#undef SIMPLE_TYPE_MARSHAL

RPCRTAPI void RPC_ENTRY
  NdrCorrelationInitialize( PMIDL_STUB_MESSAGE pStubMsg, void *pMemory, ULONG CacheSize, ULONG flags );
RPCRTAPI void RPC_ENTRY
  NdrCorrelationPass( PMIDL_STUB_MESSAGE pStubMsg );
RPCRTAPI void RPC_ENTRY
  NdrCorrelationFree( PMIDL_STUB_MESSAGE pStubMsg );

RPCRTAPI void RPC_ENTRY
  NdrConvert2( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, LONG NumberParams );
RPCRTAPI void RPC_ENTRY
  NdrConvert( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );

#define USER_MARSHAL_FC_BYTE    1
#define USER_MARSHAL_FC_CHAR    2
#define USER_MARSHAL_FC_SMALL   3
#define USER_MARSHAL_FC_USMALL  4
#define USER_MARSHAL_FC_WCHAR   5
#define USER_MARSHAL_FC_SHORT   6
#define USER_MARSHAL_FC_USHORT  7
#define USER_MARSHAL_FC_LONG    8
#define USER_MARSHAL_FC_ULONG   9
#define USER_MARSHAL_FC_FLOAT   10
#define USER_MARSHAL_FC_HYPER   11
#define USER_MARSHAL_FC_DOUBLE  12

RPCRTAPI unsigned char* RPC_ENTRY
  NdrUserMarshalSimpleTypeConvert( ULONG *pFlags, unsigned char *pBuffer, unsigned char FormatChar );

/* Note: this should return a CLIENT_CALL_RETURN, but calling convention for
 * returning structures/unions is different between Windows and gcc on i386. */
LONG_PTR RPC_VAR_ENTRY
  NdrClientCall2( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );
LONG_PTR RPC_VAR_ENTRY
  NdrClientCall( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );
LONG_PTR RPC_VAR_ENTRY
  NdrAsyncClientCall( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );
LONG_PTR RPC_VAR_ENTRY
  NdrDcomAsyncClientCall( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );

RPCRTAPI void RPC_ENTRY
  NdrServerCall2( PRPC_MESSAGE pRpcMsg );
RPCRTAPI void RPC_ENTRY
  NdrServerCall( PRPC_MESSAGE pRpcMsg );
RPCRTAPI void RPC_ENTRY
  NdrAsyncServerCall( PRPC_MESSAGE pRpcMsg );

RPCRTAPI LONG RPC_ENTRY
  NdrStubCall2( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );
RPCRTAPI LONG RPC_ENTRY
  NdrStubCall( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );
RPCRTAPI LONG RPC_ENTRY
  NdrAsyncStubCall( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );
RPCRTAPI LONG RPC_ENTRY
  NdrDcomAsyncStubCall( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );

RPCRTAPI void* RPC_ENTRY
  NdrAllocate( PMIDL_STUB_MESSAGE pStubMsg, SIZE_T Len ) __WINE_ALLOC_SIZE(2);

RPCRTAPI void RPC_ENTRY
  NdrClearOutParameters( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, void *ArgAddr );

RPCRTAPI RPC_STATUS RPC_ENTRY
  NdrMapCommAndFaultStatus( PMIDL_STUB_MESSAGE pStubMsg, ULONG *pCommStatus,
                            ULONG *pFaultStatus, RPC_STATUS Status_ );

RPCRTAPI void* RPC_ENTRY
  NdrOleAllocate( size_t Size ) __WINE_ALLOC_SIZE(1);
RPCRTAPI void RPC_ENTRY
  NdrOleFree( void* NodeToFree );

RPCRTAPI void RPC_ENTRY
  NdrClientInitialize( PRPC_MESSAGE pRpcMessage, PMIDL_STUB_MESSAGE pStubMsg,
                       PMIDL_STUB_DESC pStubDesc, unsigned int ProcNum );
RPCRTAPI void RPC_ENTRY
  NdrClientInitializeNew( PRPC_MESSAGE pRpcMessage, PMIDL_STUB_MESSAGE pStubMsg,
                          PMIDL_STUB_DESC pStubDesc, unsigned int ProcNum );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrServerInitialize( PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDesc );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrServerInitializeNew( PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDesc );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrServerInitializeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDesc, PRPC_MESSAGE pRpcMsg );
RPCRTAPI void RPC_ENTRY
  NdrServerInitializeMarshall( PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg  );
RPCRTAPI void RPC_ENTRY
  NdrServerMarshall( struct IRpcStubBuffer *pThis, struct IRpcChannelBuffer *pChannel, PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );
RPCRTAPI void RPC_ENTRY
  NdrServerUnmarshall( struct IRpcChannelBuffer *pChannel, PRPC_MESSAGE pRpcMsg,
                       PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDesc,
                       PFORMAT_STRING pFormat, void *pParamList );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrGetBuffer( PMIDL_STUB_MESSAGE stubmsg, ULONG buflen, RPC_BINDING_HANDLE handle );
RPCRTAPI void RPC_ENTRY
  NdrFreeBuffer( PMIDL_STUB_MESSAGE pStubMsg );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrSendReceive( PMIDL_STUB_MESSAGE stubmsg, unsigned char *buffer );

RPCRTAPI unsigned char * RPC_ENTRY
  NdrNsGetBuffer( PMIDL_STUB_MESSAGE pStubMsg, ULONG BufferLength, RPC_BINDING_HANDLE Handle );
RPCRTAPI unsigned char * RPC_ENTRY
  NdrNsSendReceive( PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pBufferEnd, RPC_BINDING_HANDLE *pAutoHandle );

RPCRTAPI RPC_STATUS RPC_ENTRY
  NdrGetDcomProtocolVersion( PMIDL_STUB_MESSAGE pStubMsg, RPC_VERSION *pVersion );

RPCRTAPI PFULL_PTR_XLAT_TABLES RPC_ENTRY
  NdrFullPointerXlatInit( ULONG NumberOfPointers, XLAT_SIDE XlatSide );
RPCRTAPI void RPC_ENTRY
  NdrFullPointerXlatFree( PFULL_PTR_XLAT_TABLES pXlatTables );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerQueryPointer( PFULL_PTR_XLAT_TABLES pXlatTables, void *pPointer,
                              unsigned char QueryType, ULONG *pRefId );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerQueryRefId( PFULL_PTR_XLAT_TABLES pXlatTables, ULONG RefId,
                            unsigned char QueryType, void **ppPointer );
RPCRTAPI void RPC_ENTRY
  NdrFullPointerInsertRefId( PFULL_PTR_XLAT_TABLES pXlatTables, ULONG RefId, void *pPointer );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerFree( PFULL_PTR_XLAT_TABLES pXlatTables, void *Pointer );

RPCRTAPI void RPC_ENTRY
  NdrRpcSsEnableAllocate( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void RPC_ENTRY
  NdrRpcSsDisableAllocate( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void RPC_ENTRY
  NdrRpcSmSetClientToOsf( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void * RPC_ENTRY
  NdrRpcSmClientAllocate( size_t Size ) __WINE_ALLOC_SIZE(1);
RPCRTAPI void RPC_ENTRY
  NdrRpcSmClientFree( void *NodeToFree );
RPCRTAPI void * RPC_ENTRY
  NdrRpcSsDefaultAllocate( size_t Size ) __WINE_ALLOC_SIZE(1);
RPCRTAPI void RPC_ENTRY
  NdrRpcSsDefaultFree( void *NodeToFree );

RPCRTAPI RPC_STATUS RPC_ENTRY
  NdrGetUserMarshalInfo( ULONG *pFlags, ULONG InformationLevel, NDR_USER_MARSHAL_INFO *pMarshalInfo );

#ifdef __cplusplus
}
#endif
#endif /*__WINE_RPCNDR_H */
