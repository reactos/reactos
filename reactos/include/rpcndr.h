#ifndef __RPCNDR_H__
#define __RPCNDR_H__
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__        ( 450 )
#endif /* __RPCNDR_H_VERSION__ */
#include <rpcnsip.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <objfwd.h>
#define TARGET_IS_NT50_OR_LATER 1
#define TARGET_IS_NT40_OR_LATER 1
#define TARGET_IS_NT351_OR_WIN95_OR_LATER 1
#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x) struct
#define NDR_CHAR_REP_MASK (unsigned long)0xFL
#define NDR_INT_REP_MASK (unsigned long)0xF0L
#define NDR_FLOAT_REP_MASK (unsigned long)0xFF00L
#define NDR_LITTLE_ENDIAN (unsigned long)0x10L
#define NDR_BIG_ENDIAN (unsigned long)0
#define NDR_IEEE_FLOAT (unsigned long)0
#define NDR_VAX_FLOAT (unsigned long)0x100L
#define NDR_ASCII_CHAR (unsigned long)0
#define NDR_EBCDIC_CHAR (unsigned long)1
#define NDR_LOCAL_DATA_REPRESENTATION (unsigned long)0x10L
#define NDR_LOCAL_ENDIAN NDR_LITTLE_ENDIAN
#define __RPC_CALLEE __stdcall
#ifndef __MIDL_USER_DEFINED
#define midl_user_allocate MIDL_user_allocate
#define midl_user_free MIDL_user_free
#define __MIDL_USER_DEFINED
#endif
#define RPC_VAR_ENTRY __cdecl
#ifdef _M_IX86
#define __MIDL_DECLSPEC_DLLIMPORT __declspec(dllimport)
#define __MIDL_DECLSPEC_DLLEXPORT __declspec(dllexport)
#else
#define __MIDL_DECLSPEC_DLLIMPORT
#define __MIDL_DECLSPEC_DLLEXPORT
#endif
#if defined(_HAVE_INT64) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64)
#define hyper __int64
#define MIDL_uhyper unsigned __int64
#else
#define hyper double
#define MIDL_uhyper double
#endif
#define small char
typedef unsigned char byte;
#define NDRSContextValue(hContext) (&(hContext)->userContext)
#define cbNDRContext 20
#define byte_from_ndr(source, target) { *(target) = *(*(char**)&(source)->Buffer)++; }
#define byte_array_from_ndr(Source, LowerIndex, UpperIndex, Target) { NDRcopy ((((char *)(Target))+(LowerIndex)), (Source)->Buffer, (unsigned int)((UpperIndex)-(LowerIndex))); *(unsigned long *)&(Source)->Buffer += ((UpperIndex)-(LowerIndex)); }
#define boolean_from_ndr(source, target) { *(target) = *(*(char**)&(source)->Buffer)++; }
#define boolean_array_from_ndr(Source, LowerIndex, UpperIndex, Target) { NDRcopy ((((char *)(Target))+(LowerIndex)), (Source)->Buffer, (unsigned int)((UpperIndex)-(LowerIndex))); *(unsigned long *)&(Source)->Buffer += ((UpperIndex)-(LowerIndex)); }
#define small_from_ndr(source, target) { *(target) = *(*(char**)&(source)->Buffer)++; }
#define small_from_ndr_temp(source, target, format) { *(target) = *(*(char**)(source))++; }
#define small_array_from_ndr(Source, LowerIndex, UpperIndex, Target) { NDRcopy ((((char *)(Target))+(LowerIndex)), (Source)->Buffer, (unsigned int)((UpperIndex)-(LowerIndex))); *(unsigned long *)&(Source)->Buffer += ((UpperIndex)-(LowerIndex)); }
#define MIDL_ascii_strlen(string) strlen(string)
#define MIDL_ascii_strcpy(target,source) strcpy(target,source)
#define MIDL_memset(s,c,n) memset(s,c,n)
#define _midl_ma1( p, cast ) *(*( cast **)&p)++
#define _midl_ma2( p, cast ) *(*( cast **)&p)++
#define _midl_ma4( p, cast ) *(*( cast **)&p)++
#define _midl_ma8( p, cast ) *(*( cast **)&p)++
#define _midl_unma1( p, cast ) *(( cast *)p)++
#define _midl_unma2( p, cast ) *(( cast *)p)++
#define _midl_unma3( p, cast ) *(( cast *)p)++
#define _midl_unma4( p, cast ) *(( cast *)p)++
#define _midl_fa2( p ) (p = (RPC_BUFPTR )((unsigned long)(p+1) & 0xfffffffe))
#define _midl_fa4( p ) (p = (RPC_BUFPTR )((unsigned long)(p+3) & 0xfffffffc))
#define _midl_fa8( p ) (p = (RPC_BUFPTR )((unsigned long)(p+7) & 0xfffffff8))
#define _midl_addp( p, n ) (p += n)
#define _midl_marsh_lhs( p, cast ) *(*( cast **)&p)++
#define _midl_marsh_up( mp, p ) *(*(unsigned long **)&mp)++ = (unsigned long)p
#define _midl_advmp( mp ) *(*(unsigned long **)&mp)++
#define _midl_unmarsh_up( p ) (*(*(unsigned long **)&p)++)
#define NdrMarshConfStringHdr( p, s, l ) (_midl_ma4( p, unsigned long) = s, _midl_ma4( p, unsigned long) = 0, _midl_ma4( p, unsigned long) = l)
#define NdrUnMarshConfStringHdr(p, s, l) ((s=_midl_unma4(p,unsigned long), (_midl_addp(p,4)), (l=_midl_unma4(p,unsigned long))
#define NdrMarshCCtxtHdl(pc,p) (NDRCContextMarshall( (NDR_CCONTEXT)pc, p ),p+20)
#define NdrUnMarshCCtxtHdl(pc,p,h,drep) (NDRCContextUnmarshall((NDR_CONTEXT)pc,h,p,drep), p+20)
#define NdrUnMarshSCtxtHdl(pc, p,drep) (pc = NdrSContextUnMarshall(p,drep ))
#define NdrMarshSCtxtHdl(pc,p,rd) (NdrSContextMarshall((NDR_SCONTEXT)pc,p, (NDR_RUNDOWN)rd)
#define NdrFieldOffset(s,f) (long)(& (((s *)0)->f))
#define NdrFieldPad(s,f,p,t) (NdrFieldOffset(s,f) - NdrFieldOffset(s,p) - sizeof(t))
#define NdrFcShort(s) (unsigned char)(s & 0xff), (unsigned char)(s >> 8)
#define NdrFcLong(s) (unsigned char)(s & 0xff), (unsigned char)((s & 0x0000ff00) >> 8), (unsigned char)((s & 0x00ff0000) >> 16), (unsigned char)(s >> 24)
#ifdef CONST_VTABLE
#define CONST_VTBL const
#else
#define CONST_VTBL
#endif
typedef void *NDR_CCONTEXT;
typedef struct {
	void *pad[2];
	void *userContext;
} *NDR_SCONTEXT;
typedef void (__RPC_USER *NDR_RUNDOWN)(void*);

struct _MIDL_STUB_MESSAGE;
struct _MIDL_STUB_DESC;
struct _FULL_PTR_XLAT_TABLES;
typedef unsigned char *RPC_BUFPTR;
typedef unsigned long RPC_LENGTH;
typedef void(__RPC_USER *EXPR_EVAL)(struct _MIDL_STUB_MESSAGE*);
typedef const unsigned char *PFORMAT_STRING;

typedef struct
{
	long Dimension;
	unsigned long *BufferConformanceMark;
	unsigned long *BufferVarianceMark;
	unsigned long *MaxCountArray;
	unsigned long *OffsetArray;
	unsigned long *ActualCountArray;
} ARRAY_INFO, *PARRAY_INFO;

typedef struct
{
  unsigned long WireCodeset;
  unsigned long DesiredReceivingCodeset;
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
  unsigned long BufferLength;
  unsigned long MemorySize;
  unsigned char *Memory;
  int IsClient;
  int ReuseBuffer;
  struct NDR_ALLOC_ALL_NODES_CONTEXT *pAllocAllNodesContext;
  struct NDR_POINTER_QUEUE_STATE *pPointerQueueState;
  int IgnoreEmbeddedPointers;
  unsigned char *PointerBufferMark;
  unsigned char fBufferValid;
  unsigned char uFlags;
  unsigned short UniquePtrCount;
  ULONG_PTR MaxCount;
  unsigned long Offset;
  unsigned long ActualCount;
  void * (__RPC_API *pfnAllocate)(size_t);
  void (__RPC_API *pfnFree)(void *);
  unsigned char *StackTop;
  unsigned char *pPresentedType;
  unsigned char *pTransmitType;
  handle_t SavedHandle;
  const struct _MIDL_STUB_DESC *StubDesc;
  struct _FULL_PTR_XLAT_TABLES *FullPtrXlatTables;
  unsigned long FullPtrRefId;
  unsigned long PointerLength;
  int fInDontFree:1;
  int fDontCallFreeInst:1;
  int fInOnlyParam:1;
  int fHasReturn:1;
  int fHasExtensions:1;
  int fHasNewCorrDesc:1;
  int fUnused:10;
  int fUnused2:16;
  unsigned long dwDestContext;
  void *pvDestContext;
  NDR_SCONTEXT *SavedContextHandles;
  long ParamNumber;
  struct IRpcChannelBuffer *pRpcChannelBuffer;
  PARRAY_INFO pArrayInfo;
  unsigned long *SizePtrCountArray;
  unsigned long *SizePtrOffsetArray;
  unsigned long *SizePtrLengthArray;
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
  INT_PTR Unused;
  struct _NDR_PROC_CONTEXT *pContext;
  INT_PTR Reserved51_1;
  INT_PTR Reserved51_2;
  INT_PTR Reserved51_3;
  INT_PTR Reserved51_4;
  INT_PTR Reserved51_5;
} MIDL_STUB_MESSAGE, *PMIDL_STUB_MESSAGE;
#include <poppack.h>

typedef void*(__RPC_API *GENERIC_BINDING_ROUTINE)(void*);
typedef void (__RPC_API *GENERIC_UNBIND_ROUTINE)(void*,unsigned char*);
typedef struct _GENERIC_BINDING_ROUTINE_PAIR {
	GENERIC_BINDING_ROUTINE pfnBind;
	GENERIC_UNBIND_ROUTINE pfnUnbind;
} GENERIC_BINDING_ROUTINE_PAIR,*PGENERIC_BINDING_ROUTINE_PAIR;
typedef struct __GENERIC_BINDING_INFO {
	void *pObj;
	unsigned int Size;
	GENERIC_BINDING_ROUTINE pfnBind;
	GENERIC_UNBIND_ROUTINE pfnUnbind;
} GENERIC_BINDING_INFO,*PGENERIC_BINDING_INFO;
typedef void(__RPC_USER *XMIT_HELPER_ROUTINE)(PMIDL_STUB_MESSAGE);
typedef struct _XMIT_ROUTINE_QUINTUPLE
{
	XMIT_HELPER_ROUTINE pfnTranslateToXmit;
	XMIT_HELPER_ROUTINE pfnTranslateFromXmit;
	XMIT_HELPER_ROUTINE pfnFreeXmit;
	XMIT_HELPER_ROUTINE pfnFreeInst;
} XMIT_ROUTINE_QUINTUPLE, *PXMIT_ROUTINE_QUINTUPLE;

typedef struct _MALLOC_FREE_STRUCT {
void*(__RPC_USER *pfnAllocate)(unsigned int);
void(__RPC_USER *pfnFree)(void*);
} MALLOC_FREE_STRUCT;
typedef struct _COMM_FAULT_OFFSETS {
	short CommOffset;
	short FaultOffset;
} COMM_FAULT_OFFSETS;
typedef unsigned long (__RPC_USER *USER_MARSHAL_SIZING_ROUTINE)(unsigned long *,unsigned long,void *);
typedef unsigned char *(__RPC_USER *USER_MARSHAL_MARSHALLING_ROUTINE)(unsigned long *,unsigned char *,void *);
typedef unsigned char *(__RPC_USER *USER_MARSHAL_UNMARSHALLING_ROUTINE)(unsigned long *,unsigned char *,void *);
typedef void (__RPC_USER *USER_MARSHAL_FREEING_ROUTINE)(unsigned long *,void *);

typedef struct _USER_MARSHAL_ROUTINE_QUADRUPLE
{
	USER_MARSHAL_SIZING_ROUTINE pfnBufferSize;
	USER_MARSHAL_MARSHALLING_ROUTINE pfnMarshall;
	USER_MARSHAL_UNMARSHALLING_ROUTINE pfnUnmarshall;
	USER_MARSHAL_FREEING_ROUTINE pfnFree;
} USER_MARSHAL_ROUTINE_QUADRUPLE;
typedef void (__RPC_USER *NDR_NOTIFY_ROUTINE)(void);
typedef enum _IDL_CS_CONVERT {
	IDL_CS_NO_CONVERT,
	IDL_CS_IN_PLACE_CONVERT,
	IDL_CS_NEW_BUFFER_CONVERT
} IDL_CS_CONVERT;
typedef void (__RPC_USER *CS_TYPE_NET_SIZE_ROUTINE)(RPC_BINDING_HANDLE,unsigned long,unsigned long,IDL_CS_CONVERT*,unsigned long*,error_status_t*);
typedef void (__RPC_USER *CS_TYPE_LOCAL_SIZE_ROUTINE)(RPC_BINDING_HANDLE,unsigned long,unsigned long,IDL_CS_CONVERT*,unsigned long*,error_status_t*);
typedef void (__RPC_USER *CS_TYPE_TO_NETCS_ROUTINE)(RPC_BINDING_HANDLE,unsigned long,void*,unsigned long,byte*,unsigned long*,error_status_t*);
typedef void (__RPC_USER *CS_TYPE_FROM_NETCS_ROUTINE)(RPC_BINDING_HANDLE,unsigned long,byte*,unsigned long,unsigned long,void*,unsigned long*,error_status_t*);
typedef void (__RPC_USER *CS_TAG_GETTING_ROUTINE)(RPC_BINDING_HANDLE,int,unsigned long*,unsigned long*,unsigned long*,error_status_t*);
typedef struct _NDR_CS_SIZE_CONVERT_ROUTINES {
	CS_TYPE_NET_SIZE_ROUTINE pfnNetSize;
	CS_TYPE_TO_NETCS_ROUTINE pfnToNetCs;
	CS_TYPE_LOCAL_SIZE_ROUTINE pfnLocalSize;
	CS_TYPE_FROM_NETCS_ROUTINE pfnFromNetCs;
} NDR_CS_SIZE_CONVERT_ROUTINES;
typedef struct _NDR_CS_ROUTINES {
	NDR_CS_SIZE_CONVERT_ROUTINES *pSizeConvertRoutines;
	CS_TAG_GETTING_ROUTINE *pTagGettingRoutines;
} NDR_CS_ROUTINES;
typedef struct _MIDL_STUB_DESC
{
	void *RpcInterfaceInformation;
	void * (__RPC_API *pfnAllocate)(size_t);
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
	unsigned long Version;
	MALLOC_FREE_STRUCT *pMallocFreeStruct;
	long MIDLVersion;
	const COMM_FAULT_OFFSETS *CommFaultOffsets;
	const USER_MARSHAL_ROUTINE_QUADRUPLE *aUserMarshalQuadruple;
	const NDR_NOTIFY_ROUTINE *NotifyRoutineTable;
	ULONG_PTR mFlags;
	const NDR_CS_ROUTINES *CsRoutineTables;
	void *Reserved4;
	ULONG_PTR Reserved5;
} MIDL_STUB_DESC;
typedef const MIDL_STUB_DESC *PMIDL_STUB_DESC;
typedef void*PMIDL_XMIT_TYPE;
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

typedef long (__RPC_API *SERVER_ROUTINE)();

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
	unsigned long RefId;
	unsigned char State;
} FULL_PTR_TO_REFID_ELEMENT, *PFULL_PTR_TO_REFID_ELEMENT;

/* Full pointer translation tables */
typedef struct _FULL_PTR_XLAT_TABLES {
	struct {
		void **XlatTable;
		unsigned char *StateTable;
		unsigned long NumberOfEntries;
	} RefIdToPointer;

	struct {
		PFULL_PTR_TO_REFID_ELEMENT *XlatTable;
		unsigned long NumberOfBuckets;
		unsigned long HashMask;
	} PointerToRefId;

	unsigned long NextRefId;
	XLAT_SIDE XlatSide;
} FULL_PTR_XLAT_TABLES, *PFULL_PTR_XLAT_TABLES;

struct IRpcStubBuffer;

typedef unsigned long error_status_t;
typedef void  * NDR_CCONTEXT;

typedef struct _SCONTEXT_QUEUE {
	unsigned long NumberOfObjects;
	NDR_SCONTEXT *ArrayOfObjects;
} SCONTEXT_QUEUE, *PSCONTEXT_QUEUE;

/* Context Handles */

RPCRTAPI RPC_BINDING_HANDLE RPC_ENTRY
  NDRCContextBinding( IN NDR_CCONTEXT CContext );

RPCRTAPI void RPC_ENTRY
  NDRCContextMarshall( IN NDR_CCONTEXT CContext, OUT void *pBuff );

RPCRTAPI void RPC_ENTRY
  NDRCContextUnmarshall( OUT NDR_CCONTEXT *pCContext, IN RPC_BINDING_HANDLE hBinding,
                         IN void *pBuff, IN unsigned long DataRepresentation );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshall( IN NDR_SCONTEXT CContext, OUT void *pBuff, IN NDR_RUNDOWN userRunDownIn );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshall( IN void *pBuff, IN unsigned long DataRepresentation );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshallEx( IN RPC_BINDING_HANDLE BindingHandle, IN NDR_SCONTEXT CContext, 
                         OUT void *pBuff, IN NDR_RUNDOWN userRunDownIn );

RPCRTAPI void RPC_ENTRY
  NDRSContextMarshall2( IN RPC_BINDING_HANDLE BindingHandle, IN NDR_SCONTEXT CContext,
                        OUT void *pBuff, IN NDR_RUNDOWN userRunDownIn, IN void * CtxGuard,
                        IN unsigned long Flags );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshallEx( IN RPC_BINDING_HANDLE BindingHandle, IN void *pBuff, 
                           IN unsigned long DataRepresentation );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NDRSContextUnmarshall2( IN RPC_BINDING_HANDLE BindingHandle, IN void *pBuff,
                          IN unsigned long DataRepresentation, IN void *CtxGuard,
                          IN unsigned long Flags );

RPCRTAPI void RPC_ENTRY
  NdrClientContextMarshall ( PMIDL_STUB_MESSAGE pStubMsg, NDR_CCONTEXT ContextHandle, int fCheck );

RPCRTAPI void RPC_ENTRY
  NdrClientContextUnmarshall( IN PMIDL_STUB_MESSAGE pStubMsg, OUT NDR_CCONTEXT* pContextHandle,
                              IN RPC_BINDING_HANDLE BindHandle );

RPCRTAPI void RPC_ENTRY
  NdrServerContextMarshall ( PMIDL_STUB_MESSAGE pStubMsg, NDR_SCONTEXT ContextHandle, NDR_RUNDOWN RundownRoutine );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrServerContextUnmarshall( IN PMIDL_STUB_MESSAGE pStubMsg );

RPCRTAPI void RPC_ENTRY
  NdrContextHandleSize( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrContextHandleInitialize( IN PMIDL_STUB_MESSAGE pStubMsg, IN PFORMAT_STRING pFormat );

RPCRTAPI void RPC_ENTRY
  NdrServerContextNewMarshall( PMIDL_STUB_MESSAGE pStubMsg, NDR_SCONTEXT ContextHandle,
                               NDR_RUNDOWN RundownRoutine, PFORMAT_STRING pFormat );

RPCRTAPI NDR_SCONTEXT RPC_ENTRY
  NdrServerContextNewUnmarshall( IN PMIDL_STUB_MESSAGE pStubMsg, IN PFORMAT_STRING pFormat );

RPCRTAPI void RPC_ENTRY
  RpcSsDestroyClientContext( IN void **ContextHandle );

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
RPCRTAPI unsigned long RPC_ENTRY \
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
  NdrConvert2( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, long NumberParams );
RPCRTAPI void RPC_ENTRY
  NdrConvert( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat );

/* Note: this should return a CLIENT_CALL_RETURN, but calling convention for
 * returning structures/unions is different between Windows and gcc on i386. */
LONG_PTR RPC_VAR_ENTRY
  NdrClientCall2( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );
LONG_PTR RPC_VAR_ENTRY
  NdrClientCall( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );
LONG_PTR RPC_VAR_ENTRY
  NdrAsyncClientCall( PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, ... );

RPCRTAPI void RPC_ENTRY
  NdrServerCall2( PRPC_MESSAGE pRpcMsg );
RPCRTAPI void RPC_ENTRY
  NdrServerCall( PRPC_MESSAGE pRpcMsg );

RPCRTAPI long RPC_ENTRY
  NdrStubCall2( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );
RPCRTAPI long RPC_ENTRY
  NdrStubCall( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );
RPCRTAPI long RPC_ENTRY
  NdrAsyncStubCall( struct IRpcStubBuffer* pThis, struct IRpcChannelBuffer* pChannel, PRPC_MESSAGE pRpcMsg, unsigned long * pdwStubPhase );

RPCRTAPI void* RPC_ENTRY
  NdrAllocate( PMIDL_STUB_MESSAGE pStubMsg, size_t Len );

RPCRTAPI void RPC_ENTRY
  NdrClearOutParameters( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, void *ArgAddr );

RPCRTAPI RPC_STATUS RPC_ENTRY
  NdrMapCommAndFaultStatus( PMIDL_STUB_MESSAGE pStubMsg, unsigned long *pCommStatus,
                            unsigned long *pFaultStatus, RPC_STATUS Status_ );

RPCRTAPI void* RPC_ENTRY
  NdrOleAllocate( size_t Size );
RPCRTAPI void RPC_ENTRY
  NdrOleFree( void* NodeToFree );

RPCRTAPI void RPC_ENTRY
  NdrClientInitializeNew( PRPC_MESSAGE pRpcMessage, PMIDL_STUB_MESSAGE pStubMsg, 
                          PMIDL_STUB_DESC pStubDesc, unsigned int ProcNum );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrServerInitializeNew( PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDesc );  
RPCRTAPI unsigned char* RPC_ENTRY
  NdrGetBuffer( MIDL_STUB_MESSAGE *stubmsg, unsigned long buflen, RPC_BINDING_HANDLE handle );
RPCRTAPI void RPC_ENTRY
  NdrFreeBuffer( MIDL_STUB_MESSAGE *pStubMsg );
RPCRTAPI unsigned char* RPC_ENTRY
  NdrSendReceive( MIDL_STUB_MESSAGE *stubmsg, unsigned char *buffer );

RPCRTAPI unsigned char * RPC_ENTRY
  NdrNsGetBuffer( PMIDL_STUB_MESSAGE pStubMsg, unsigned long BufferLength, RPC_BINDING_HANDLE Handle );
RPCRTAPI unsigned char * RPC_ENTRY
  NdrNsSendReceive( PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pBufferEnd, RPC_BINDING_HANDLE *pAutoHandle );

RPCRTAPI PFULL_PTR_XLAT_TABLES RPC_ENTRY
  NdrFullPointerXlatInit( unsigned long NumberOfPointers, XLAT_SIDE XlatSide );
RPCRTAPI void RPC_ENTRY
  NdrFullPointerXlatFree( PFULL_PTR_XLAT_TABLES pXlatTables );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerQueryPointer( PFULL_PTR_XLAT_TABLES pXlatTables, void *pPointer,
                              unsigned char QueryType, unsigned long *pRefId );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerQueryRefId( PFULL_PTR_XLAT_TABLES pXlatTables, unsigned long RefId,
                            unsigned char QueryType, void **ppPointer );
RPCRTAPI void RPC_ENTRY
  NdrFullPointerInsertRefId( PFULL_PTR_XLAT_TABLES pXlatTables, unsigned long RefId, void *pPointer );
RPCRTAPI int RPC_ENTRY
  NdrFullPointerFree( PFULL_PTR_XLAT_TABLES pXlatTables, void *Pointer );

RPCRTAPI void RPC_ENTRY
  NdrRpcSsEnableAllocate( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void RPC_ENTRY
  NdrRpcSsDisableAllocate( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void RPC_ENTRY
  NdrRpcSmSetClientToOsf( PMIDL_STUB_MESSAGE pMessage );
RPCRTAPI void * RPC_ENTRY
  NdrRpcSmClientAllocate( IN size_t Size );
RPCRTAPI void RPC_ENTRY
  NdrRpcSmClientFree( IN void *NodeToFree );
RPCRTAPI void * RPC_ENTRY
  NdrRpcSsDefaultAllocate( IN size_t Size );
RPCRTAPI void RPC_ENTRY
  NdrRpcSsDefaultFree( IN void *NodeToFree );

#ifdef __cplusplus
}
#endif
#endif
