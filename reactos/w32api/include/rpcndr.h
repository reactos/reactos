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
typedef struct _SCONTEXT_QUEUE {
	unsigned long NumberOfObjects;
	NDR_SCONTEXT *ArrayOfObjects;
} SCONTEXT_QUEUE,*PSCONTEXT_QUEUE;
struct _MIDL_STUB_MESSAGE;
struct _MIDL_STUB_DESC;
struct _FULL_PTR_XLAT_TABLES;
typedef unsigned char *RPC_BUFPTR;
typedef unsigned long RPC_LENGTH;
typedef void(__RPC_USER *EXPR_EVAL)(struct _MIDL_STUB_MESSAGE*);
typedef const unsigned char *PFORMAT_STRING;
typedef struct {
	long Dimension;
	unsigned long *BufferConformanceMark;
	unsigned long *BufferVarianceMark;
	unsigned long *MaxCountArray;
	unsigned long *OffsetArray;
	unsigned long *ActualCountArray;
} ARRAY_INFO,*PARRAY_INFO;

RPC_BINDING_HANDLE RPC_ENTRY NDRCContextBinding(NDR_CCONTEXT);
void RPC_ENTRY NDRCContextMarshall(NDR_CCONTEXT,void*);
void RPC_ENTRY NDRCContextUnmarshall(NDR_CCONTEXT*,RPC_BINDING_HANDLE,void*,unsigned long);
void RPC_ENTRY NDRSContextMarshall(NDR_SCONTEXT,void*,NDR_RUNDOWN);
NDR_SCONTEXT RPC_ENTRY NDRSContextUnmarshall(void*pBuff,unsigned long);
void RPC_ENTRY RpcSsDestroyClientContext(void**);
void RPC_ENTRY NDRcopy(void*,void*,unsigned int);
unsigned int RPC_ENTRY MIDL_wchar_strlen(wchar_t*);
void RPC_ENTRY MIDL_wchar_strcpy(void*,wchar_t*);
void RPC_ENTRY char_from_ndr(PRPC_MESSAGE,unsigned char*);
void RPC_ENTRY char_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,unsigned char*);
void RPC_ENTRY short_from_ndr(PRPC_MESSAGE,unsigned short*);
void RPC_ENTRY short_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,unsigned short*);
void RPC_ENTRY short_from_ndr_temp(unsigned char**,unsigned short*,unsigned long);
void RPC_ENTRY long_from_ndr(PRPC_MESSAGE,unsigned long*);
void RPC_ENTRY long_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,unsigned long*);
void RPC_ENTRY long_from_ndr_temp(unsigned char**,unsigned long*,unsigned long);
void RPC_ENTRY enum_from_ndr(PRPC_MESSAGE,unsigned int*);
void RPC_ENTRY float_from_ndr(PRPC_MESSAGE,void*);
void RPC_ENTRY float_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,void*);
void RPC_ENTRY double_from_ndr(PRPC_MESSAGE,void*);
void RPC_ENTRY double_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,void*);
void RPC_ENTRY hyper_from_ndr(PRPC_MESSAGE,hyper*);
void RPC_ENTRY hyper_array_from_ndr(PRPC_MESSAGE,unsigned long,unsigned long,hyper*);
void RPC_ENTRY hyper_from_ndr_temp(unsigned char**,hyper*,unsigned long);
void RPC_ENTRY data_from_ndr(PRPC_MESSAGE,void*,char*,unsigned char);
void RPC_ENTRY data_into_ndr(void*,PRPC_MESSAGE,char*,unsigned char);
void RPC_ENTRY tree_into_ndr(void*,PRPC_MESSAGE,char*,unsigned char);
void RPC_ENTRY data_size_ndr(void*,PRPC_MESSAGE,char*,unsigned char);
void RPC_ENTRY tree_size_ndr(void*,PRPC_MESSAGE,char*,unsigned char);
void RPC_ENTRY tree_peek_ndr(PRPC_MESSAGE,unsigned char**,char*,unsigned char);
void *RPC_ENTRY midl_allocate(int);

#pragma pack(push,4)
typedef struct _MIDL_STUB_MESSAGE {
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
	unsigned char *AllocAllNodesMemory;
	unsigned char *AllocAllNodesMemoryEnd;
	int IgnoreEmbeddedPointers;
	unsigned char *PointerBufferMark;
	unsigned char fBufferValid;
	unsigned char Unused;
	unsigned long MaxCount;
	unsigned long Offset;
	unsigned long ActualCount;
	void*(__RPC_API *pfnAllocate)(unsigned int);
	void(__RPC_API *pfnFree)(void*);
	unsigned char *StackTop;
	unsigned char *pPresentedType;
	unsigned char *pTransmitType;
	handle_t SavedHandle;
	const struct _MIDL_STUB_DESC *StubDesc;
	struct _FULL_PTR_XLAT_TABLES *FullPtrXlatTables;
	unsigned long FullPtrRefId;
	int fCheckBounds;
	int fInDontFree :1;
	int fDontCallFreeInst :1;
	int fInOnlyParam :1;
	int fHasReturn :1;
	unsigned long dwDestContext;
	void*pvDestContext;
	NDR_SCONTEXT *SavedContextHandles;
	long ParamNumber;
	struct IRpcChannelBuffer *pRpcChannelBuffer;
	PARRAY_INFO pArrayInfo;
	unsigned long *SizePtrCountArray;
	unsigned long *SizePtrOffsetArray;
	unsigned long *SizePtrLengthArray;
	void*pArgQueue;
	unsigned long dwStubPhase;
	unsigned long w2kReserved[5];
} MIDL_STUB_MESSAGE,*PMIDL_STUB_MESSAGE;
#pragma pack(pop)
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
typedef struct _XMIT_ROUTINE_QUINTUPLE {
	XMIT_HELPER_ROUTINE pfnTranslateToXmit;
	XMIT_HELPER_ROUTINE pfnTranslateFromXmit;
	XMIT_HELPER_ROUTINE pfnFreeXmit;
	XMIT_HELPER_ROUTINE pfnFreeInst;
} XMIT_ROUTINE_QUINTUPLE,*PXMIT_ROUTINE_QUINTUPLE;
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
typedef struct _USER_MARSHAL_ROUTINE_QUADRUPLE {
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
typedef struct _MIDL_STUB_DESC {
	void*RpcInterfaceInformation;
	void*(__RPC_API *pfnAllocate)(unsigned int);
	void(__RPC_API *pfnFree)(void*);
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
typedef struct _MIDL_FORMAT_STRING {
	short Pad;
	unsigned char Format[1];
} MIDL_FORMAT_STRING;
typedef void(__RPC_API *STUB_THUNK)(PMIDL_STUB_MESSAGE);
typedef long(__RPC_API *SERVER_ROUTINE)(void);
typedef struct _MIDL_SERVER_INFO_ {
	PMIDL_STUB_DESC pStubDesc;
	const SERVER_ROUTINE *DispatchTable;
	PFORMAT_STRING ProcString;
	const unsigned short *FmtStringOffset;
	const STUB_THUNK *ThunkTable;
} MIDL_SERVER_INFO,*PMIDL_SERVER_INFO;
typedef struct _MIDL_STUBLESS_PROXY_INFO {
	PMIDL_STUB_DESC pStubDesc;
	PFORMAT_STRING ProcFormatString;
	const unsigned short *FormatStringOffset;
} MIDL_STUBLESS_PROXY_INFO;
typedef MIDL_STUBLESS_PROXY_INFO *PMIDL_STUBLESS_PROXY_INFO;
typedef union _CLIENT_CALL_RETURN {
	void *Pointer;
	long Simple;
} CLIENT_CALL_RETURN;
typedef enum { XLAT_SERVER = 1,XLAT_CLIENT } XLAT_SIDE;
typedef struct _FULL_PTR_TO_REFID_ELEMENT {
	struct _FULL_PTR_TO_REFID_ELEMENT *Next;
	void*Pointer;
	unsigned long RefId;
	unsigned char State;
} FULL_PTR_TO_REFID_ELEMENT,*PFULL_PTR_TO_REFID_ELEMENT;
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
} FULL_PTR_XLAT_TABLES,*PFULL_PTR_XLAT_TABLES;
void RPC_ENTRY NdrSimpleTypeMarshall(PMIDL_STUB_MESSAGE,unsigned char*,unsigned char);
unsigned char *RPC_ENTRY NdrPointerMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING pFormat);
unsigned char *RPC_ENTRY NdrSimpleStructMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrConformantStructMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrConformantVaryingStructMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrHardStructMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrComplexStructMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrFixedArrayMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrConformantArrayMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrConformantVaryingArrayMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrVaryingArrayMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrComplexArrayMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrNonConformantStringMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrConformantStringMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrEncapsulatedUnionMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrNonEncapsulatedUnionMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrByteCountPointerMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrXmitOrRepAsMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char *RPC_ENTRY NdrInterfacePointerMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrClientContextMarshall(PMIDL_STUB_MESSAGE,NDR_CCONTEXT,int);
void RPC_ENTRY NdrServerContextMarshall(PMIDL_STUB_MESSAGE,NDR_SCONTEXT,NDR_RUNDOWN);
void RPC_ENTRY NdrSimpleTypeUnmarshall(PMIDL_STUB_MESSAGE,unsigned char*,unsigned char);
unsigned char *RPC_ENTRY NdrPointerUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrSimpleStructUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrConformantStructUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrConformantVaryingStructUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrHardStructUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrComplexStructUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrFixedArrayUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrConformantArrayUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrConformantVaryingArrayUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrVaryingArrayUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrComplexArrayUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrNonConformantStringUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrConformantStringUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrEncapsulatedUnionUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrNonEncapsulatedUnionUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrByteCountPointerUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrXmitOrRepAsUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
unsigned char *RPC_ENTRY NdrInterfacePointerUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
void RPC_ENTRY NdrClientContextUnmarshall(PMIDL_STUB_MESSAGE,NDR_CCONTEXT*,RPC_BINDING_HANDLE);
NDR_SCONTEXT RPC_ENTRY NdrServerContextUnmarshall(PMIDL_STUB_MESSAGE);
void RPC_ENTRY NdrPointerBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrSimpleStructBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantStructBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantVaryingStructBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrHardStructBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrComplexStructBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrFixedArrayBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantArrayBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantVaryingArrayBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrVaryingArrayBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrComplexArrayBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantStringBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrNonConformantStringBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrEncapsulatedUnionBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrNonEncapsulatedUnionBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrByteCountPointerBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrXmitOrRepAsBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrInterfacePointerBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrContextHandleSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrPointerMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrSimpleStructMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrConformantStructMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrConformantVaryingStructMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrHardStructMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrComplexStructMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrFixedArrayMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrConformantArrayMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrConformantVaryingArrayMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrVaryingArrayMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrComplexArrayMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrConformantStringMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrNonConformantStringMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrEncapsulatedUnionMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrNonEncapsulatedUnionMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrXmitOrRepAsMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrInterfacePointerMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
void RPC_ENTRY NdrPointerFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrSimpleStructFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantStructFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantVaryingStructFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrHardStructFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrComplexStructFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrFixedArrayFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantArrayFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConformantVaryingArrayFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrVaryingArrayFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrComplexArrayFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrEncapsulatedUnionFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrNonEncapsulatedUnionFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrByteCountPointerFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrXmitOrRepAsFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrInterfacePointerFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
void RPC_ENTRY NdrConvert(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
void RPC_ENTRY NdrClientInitializeNew(PRPC_MESSAGE,PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC,unsigned int);
unsigned char *RPC_ENTRY NdrServerInitializeNew(PRPC_MESSAGE,PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC);
void RPC_ENTRY NdrClientInitialize(PRPC_MESSAGE,PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC,unsigned int);
unsigned char *RPC_ENTRY NdrServerInitialize(PRPC_MESSAGE,PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC);
unsigned char *RPC_ENTRY NdrServerInitializeUnmarshall(PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC,PRPC_MESSAGE);
void RPC_ENTRY NdrServerInitializeMarshall(PRPC_MESSAGE,PMIDL_STUB_MESSAGE);
unsigned char *RPC_ENTRY NdrGetBuffer(PMIDL_STUB_MESSAGE,unsigned long,RPC_BINDING_HANDLE);
unsigned char *RPC_ENTRY NdrNsGetBuffer(PMIDL_STUB_MESSAGE,unsigned long,RPC_BINDING_HANDLE);
unsigned char *RPC_ENTRY NdrSendReceive(PMIDL_STUB_MESSAGE,unsigned char*);
unsigned char *RPC_ENTRY NdrNsSendReceive(PMIDL_STUB_MESSAGE,unsigned char*,RPC_BINDING_HANDLE*);
void RPC_ENTRY NdrFreeBuffer(PMIDL_STUB_MESSAGE);
CLIENT_CALL_RETURN RPC_VAR_ENTRY NdrClientCall(PMIDL_STUB_DESC,PFORMAT_STRING,...);
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
long RPC_ENTRY NdrStubCall(struct IRpcStubBuffer*,struct IRpcChannelBuffer*,PRPC_MESSAGE,unsigned long*);
void RPC_ENTRY NdrServerCall(PRPC_MESSAGE);
int RPC_ENTRY NdrServerUnmarshall(struct IRpcChannelBuffer*,PRPC_MESSAGE,PMIDL_STUB_MESSAGE,PMIDL_STUB_DESC,PFORMAT_STRING,void*);
void RPC_ENTRY NdrServerMarshall(struct IRpcStubBuffer*,struct IRpcChannelBuffer*,PMIDL_STUB_MESSAGE,PFORMAT_STRING);
RPC_STATUS RPC_ENTRY NdrMapCommAndFaultStatus(PMIDL_STUB_MESSAGE,unsigned long*,unsigned long*,RPC_STATUS);
int RPC_ENTRY NdrSH_UPDecision(PMIDL_STUB_MESSAGE,unsigned char**,RPC_BUFPTR);
int RPC_ENTRY NdrSH_TLUPDecision(PMIDL_STUB_MESSAGE,unsigned char**);
int RPC_ENTRY NdrSH_TLUPDecisionBuffer(PMIDL_STUB_MESSAGE,unsigned char**);
int RPC_ENTRY NdrSH_IfAlloc(PMIDL_STUB_MESSAGE,unsigned char**,unsigned long);
int RPC_ENTRY NdrSH_IfAllocRef(PMIDL_STUB_MESSAGE,unsigned char**,unsigned long);
int RPC_ENTRY NdrSH_IfAllocSet(PMIDL_STUB_MESSAGE,unsigned char**,unsigned long);
RPC_BUFPTR RPC_ENTRY NdrSH_IfCopy(PMIDL_STUB_MESSAGE,unsigned char**,unsigned long);
RPC_BUFPTR RPC_ENTRY NdrSH_IfAllocCopy(PMIDL_STUB_MESSAGE,unsigned char**,unsigned long);
unsigned long RPC_ENTRY NdrSH_Copy(unsigned char*,unsigned char*,unsigned long);
void RPC_ENTRY NdrSH_IfFree(PMIDL_STUB_MESSAGE,unsigned char*);
RPC_BUFPTR RPC_ENTRY NdrSH_StringMarshall(PMIDL_STUB_MESSAGE,unsigned char*,unsigned long,int);
RPC_BUFPTR RPC_ENTRY NdrSH_StringUnMarshall(PMIDL_STUB_MESSAGE,unsigned char**,int);
typedef void *RPC_SS_THREAD_HANDLE;
typedef void* __RPC_API RPC_CLIENT_ALLOC(unsigned int);
typedef void __RPC_API RPC_CLIENT_FREE(void*);
void*RPC_ENTRY RpcSsAllocate(unsigned int);
void RPC_ENTRY RpcSsDisableAllocate(void);
void RPC_ENTRY RpcSsEnableAllocate(void);
void RPC_ENTRY RpcSsFree(void*);
RPC_SS_THREAD_HANDLE RPC_ENTRY RpcSsGetThreadHandle(void);
void RPC_ENTRY RpcSsSetClientAllocFree(RPC_CLIENT_ALLOC*,RPC_CLIENT_FREE*);
void RPC_ENTRY RpcSsSetThreadHandle(RPC_SS_THREAD_HANDLE);
void RPC_ENTRY RpcSsSwapClientAllocFree(RPC_CLIENT_ALLOC*,RPC_CLIENT_FREE*,RPC_CLIENT_ALLOC**,RPC_CLIENT_FREE**);
void*RPC_ENTRY RpcSmAllocate(unsigned int,RPC_STATUS*);
RPC_STATUS RPC_ENTRY RpcSmClientFree(void*);
RPC_STATUS RPC_ENTRY RpcSmDestroyClientContext(void**);
RPC_STATUS RPC_ENTRY RpcSmDisableAllocate(void);
RPC_STATUS RPC_ENTRY RpcSmEnableAllocate(void);
RPC_STATUS RPC_ENTRY RpcSmFree(void*);
RPC_SS_THREAD_HANDLE RPC_ENTRY RpcSmGetThreadHandle(RPC_STATUS*);
RPC_STATUS RPC_ENTRY RpcSmSetClientAllocFree(RPC_CLIENT_ALLOC*,RPC_CLIENT_FREE*);
RPC_STATUS RPC_ENTRY RpcSmSetThreadHandle(RPC_SS_THREAD_HANDLE);
RPC_STATUS RPC_ENTRY RpcSmSwapClientAllocFree(RPC_CLIENT_ALLOC*,RPC_CLIENT_FREE*,RPC_CLIENT_ALLOC**,RPC_CLIENT_FREE**);
void RPC_ENTRY NdrRpcSsEnableAllocate(PMIDL_STUB_MESSAGE);
void RPC_ENTRY NdrRpcSsDisableAllocate(PMIDL_STUB_MESSAGE);
void RPC_ENTRY NdrRpcSmSetClientToOsf(PMIDL_STUB_MESSAGE);
void*RPC_ENTRY NdrRpcSmClientAllocate(unsigned int);
void RPC_ENTRY NdrRpcSmClientFree(void*);
void*RPC_ENTRY NdrRpcSsDefaultAllocate(unsigned int);
void RPC_ENTRY NdrRpcSsDefaultFree(void*);
PFULL_PTR_XLAT_TABLES RPC_ENTRY NdrFullPointerXlatInit(unsigned long,XLAT_SIDE);
void RPC_ENTRY NdrFullPointerXlatFree(PFULL_PTR_XLAT_TABLES);
int RPC_ENTRY NdrFullPointerQueryPointer(PFULL_PTR_XLAT_TABLES,void*,unsigned char,unsigned long*);
int RPC_ENTRY NdrFullPointerQueryRefId(PFULL_PTR_XLAT_TABLES,unsigned long,unsigned char,void**);
void RPC_ENTRY NdrFullPointerInsertRefId(PFULL_PTR_XLAT_TABLES,unsigned long,void*);
int RPC_ENTRY NdrFullPointerFree(PFULL_PTR_XLAT_TABLES,void*);
void*RPC_ENTRY NdrAllocate(PMIDL_STUB_MESSAGE,unsigned int);
void RPC_ENTRY NdrClearOutParameters(PMIDL_STUB_MESSAGE,PFORMAT_STRING,void*);
void*RPC_ENTRY NdrOleAllocate(unsigned int);
void RPC_ENTRY NdrOleFree(void*);
unsigned char*RPC_ENTRY NdrUserMarshalMarshall(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned char*RPC_ENTRY NdrUserMarshalUnmarshall(PMIDL_STUB_MESSAGE,unsigned char**,PFORMAT_STRING,unsigned char);
void RPC_ENTRY NdrUserMarshalBufferSize(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
unsigned long RPC_ENTRY NdrUserMarshalMemorySize(PMIDL_STUB_MESSAGE,PFORMAT_STRING);
void RPC_ENTRY NdrUserMarshalFree(PMIDL_STUB_MESSAGE,unsigned char*,PFORMAT_STRING);
#ifdef __cplusplus
}
#endif
#endif
