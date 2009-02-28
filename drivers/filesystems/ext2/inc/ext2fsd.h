/*************************************************************************
*
* File: ext2fsd.h
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	The main include file for the Ext2 file system driver.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/


#ifndef	_EXT2_FSD_H_
#define	_EXT2_FSD_H_

#define EXT2_POOL_WITH_TAG


// some constant definitions
#define	EXT2_PANIC_IDENTIFIER		(0x86427531)

// any directory information EXT2 obtains from the local file system
//	will use a buffer of the following size ... (in KB)
#define	EXT2_READ_DIR_BUFFER_LENGTH	(512)
#define	EXT2_MAXCLOSABLE_FCBS_UL	20
#define	EXT2_MAXCLOSABLE_FCBS_LL	10

//	Some type definitions...
//	These are used later...

typedef unsigned int    UINT;
typedef unsigned char   BYTE;

// Common include files - should be in the include dir of the MS supplied IFS Kit
#include	<ntifs.h>
#include <ntdddisk.h>


/* REACTOS FIXME */
#undef DeleteFile
/* This is deprecated and should be changed in the EXT2FS driver. */
#define RtlLargeIntegerLessThan(a, b) (a).QuadPart < (b).QuadPart
#define RtlLargeIntegerGreaterThan(a, b) (a).QuadPart > (b).QuadPart


// the following include files should be in the inc sub-dir associated with this driver
#include	"ext2metadata.h"
#include	"struct.h"
#include	"protos.h"
#include	"errmsg.h"


// global variables - minimize these
extern Ext2Data				Ext2GlobalData;

// try-finally simulation
#define try_return(S)	{ S; goto try_exit; }
#define try_return1(S)	{ S; goto try_exit1; }
#define try_return2(S)	{ S; goto try_exit2; }

// some global (helpful) macros
#define Ext2IsFlagOn(Flags,SingleFlag) ((BOOLEAN)((((Flags) & (SingleFlag)) != 0)))
#define	Ext2SetFlag(Flag, Value)	((Flag) |= (Value))
#define	Ext2ClearFlag(Flag, Value)	((Flag) &= ~(Value))

#define	Ext2QuadAlign(Value)			((((uint32)(Value)) + 7) & 0xfffffff8)

// to perform a bug-check (panic), the following macro is used
#define	Ext2Panic(arg1, arg2, arg3)					\
	(KeBugCheckEx(EXT2_PANIC_IDENTIFIER, EXT2_BUG_CHECK_ID | __LINE__, (uint32)(arg1), (uint32)(arg2), (uint32)(arg3)))

// a convenient macro (must be invoked in the context of the thread that acquired the resource)
#define	Ext2ReleaseResource(Resource)	\
	(ExReleaseResourceForThreadLite((Resource), ExGetCurrentResourceThread()))

// each file has a unique bug-check identifier associated with it.
//	Here is a list of constant definitions for these identifiers
#define	EXT2_FILE_INIT									(0x00000001)
#define	EXT2_FILE_REGISTRY								(0x00000002)
#define	EXT2_FILE_CREATE								(0x00000003)
#define	EXT2_FILE_CLEANUP								(0x00000004)
#define	EXT2_FILE_CLOSE									(0x00000005)
#define	EXT2_FILE_READ									(0x00000006)
#define	EXT2_FILE_WRITE									(0x00000007)
#define	EXT2_FILE_INFORMATION							(0x00000008)
#define	EXT2_FILE_FLUSH									(0x00000009)
#define	EXT2_FILE_VOL_INFORMATION						(0x0000000A)
#define	EXT2_FILE_DIR_CONTROL							(0x0000000B)
#define	EXT2_FILE_FILE_CONTROL							(0x0000000C)
#define	EXT2_FILE_DEVICE_CONTROL						(0x0000000D)
#define	EXT2_FILE_SHUTDOWN								(0x0000000E)
#define	EXT2_FILE_LOCK_CONTROL							(0x0000000F)
#define	EXT2_FILE_SECURITY								(0x00000010)
#define	EXT2_FILE_EXT_ATTR								(0x00000011)
#define	EXT2_FILE_MISC									(0x00000012)
#define	EXT2_FILE_FAST_IO								(0x00000013)
#define	EXT2_FILE_IO									(0x00000014)
#define	EXT2_FILE_METADATA_IO							(0x00000015)



#if DBG
#define	Ext2BreakPoint()	DbgBreakPoint()
#else
#define	Ext2BreakPoint()
#endif

#define Ext2RaiseStatus(IRPCONTEXT,STATUS)		\
{												\
    (IRPCONTEXT)->ExceptionStatus = (STATUS);	\
    ExRaiseStatus( (STATUS) );					\
}

#ifdef EXT2_POOL_WITH_TAG
	#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
	#define Ext2AllocatePool(PoolType,NumberOfBytes)	\
		ExAllocatePoolWithTag( PoolType, NumberOfBytes, TAG ( 'E','x','t','2' ) ) 
#else
	#define Ext2AllocatePool(PoolType,NumberOfBytes)	\
		ExAllocatePool( PoolType, NumberOfBytes ) 
#endif


#if DBG

//
//	Trace types...
//	Any number of these may be enabled...
//
#define DEBUG_TRACE_IRQL	             (0x00000001)
#define DEBUG_TRACE_IRP_ENTRY			 (0x00000002)
#define DEBUG_TRACE_RESOURCE_ACQUIRE     (0x00000004)
#define DEBUG_TRACE_RESOURCE_RELEASE     (0x00000008)
#define DEBUG_TRACE_RESOURCE_RETRY		 (0x00000010)
#define DEBUG_TRACE_ASYNC                (0x00000020)
#define DEBUG_TRACE_MOUNT                (0x00000040)
#define DEBUG_TRACE_RESOURCE_STATE       (0x00000080)
#define DEBUG_TRACE_MISC                 (0x00000100)
#define DEBUG_TRACE_FILE_OBJ             (0x00000200)
#define DEBUG_TRACE_FILE_NAME            (0x00000400)
#define DEBUG_TRACE_SPECIAL              (0x00000800)
#define DEBUG_TRACE_ERROR				 (0x00001000)
#define DEBUG_TRACE_READ_DETAILS		 (0x00002000)
#define DEBUG_TRACE_WRITE_DETAILS		 (0x00004000)
#define DEBUG_TRACE_FILEINFO			 (0x00008000)
#define DEBUG_TRACE_DIRINFO				 (0x00010000)
#define DEBUG_TRACE_REFERENCE			 (0x00020000)
#define DEBUG_TRACE_FSCTRL				 (0x00040000)
#define DEBUG_TRACE_FREE				 (0x00080000)
#define DEBUG_TRACE_LINENO	             (0x00100000)
#define DEBUG_TRACE_TRIPLE	             (0x00200000)

#define DEBUG_TRACE_ALL					 (0xffffffff)
#define DEBUG_TRACE_NONE	 				  0
//
//	The permitted DebugTrace types...
//
#define PERMITTED_DEBUG_TRACE_TYPES		DEBUG_TRACE_NONE
/*
#define PERMITTED_DEBUG_TRACE_TYPES		DEBUG_TRACE_ERROR | DEBUG_TRACE_IRP_ENTRY |		\
										DEBUG_TRACE_FILE_NAME |	DEBUG_TRACE_SPECIAL	|	\
										DEBUG_TRACE_ASYNC

*/


#define DebugTrace( TYPE, X, Y )											\
{																			\
	if( ( TYPE ) & ( PERMITTED_DEBUG_TRACE_TYPES ) )						\
	{																		\
		if( ( DEBUG_TRACE_LINENO ) & ( PERMITTED_DEBUG_TRACE_TYPES ) )		\
		{																	\
			DbgPrint("(%s:%ld) ", __FILE__, __LINE__ );						\
		}																	\
		DbgPrint(X,Y);														\
		if( ( DEBUG_TRACE_IRQL ) & ( PERMITTED_DEBUG_TRACE_TYPES ) )		\
		{																	\
			DbgPrint( ",IRQL = %d ", KeGetCurrentIrql( ) );					\
		}																	\
		DbgPrint("\n");														\
	}																		\
}


#define DebugTraceState( STR, X1, X2, X3)														\
{																								\
	if( ( DEBUG_TRACE_RESOURCE_STATE ) & ( PERMITTED_DEBUG_TRACE_TYPES ) )						\
	{																							\
		DbgPrint("\nState: ");																	\
		DbgPrint( STR, X1, X2, X3 );															\
		if( ( DEBUG_TRACE_IRQL ) & ( PERMITTED_DEBUG_TRACE_TYPES ) )							\
		{																						\
			DbgPrint( "  IRQL = %d ", KeGetCurrentIrql( ) );									\
		}																						\
	}																							\
}

#define AssertFCB( PtrFCB )														\
{																				\
	if( !(PtrFCB) || (PtrFCB)->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB )	\
	{																			\
		Ext2BreakPoint();														\
	}																			\
}

#define AssertVCB( PtrVCB )														\
{																				\
	if( !(PtrVCB) || (PtrVCB)->NodeIdentifier.NodeType != EXT2_NODE_TYPE_VCB )	\
	{																			\
		Ext2BreakPoint();														\
	}																			\
}

#define AssertFCBorVCB( PtrVCBorFCB )											\
{																				\
	if( !(PtrVCBorFCB) ||														\
		( (PtrVCBorFCB)->NodeIdentifier.NodeType != EXT2_NODE_TYPE_VCB &&		\
		  (PtrVCBorFCB)->NodeIdentifier.NodeType != EXT2_NODE_TYPE_FCB ) )		\
	{																			\
		Ext2BreakPoint();														\
	}																			\
}

#else
	#define DebugTrace( TYPE, X, Y ) 
	#define DebugTraceState( STR, X1, X2, X3 )
	#define AssertFCB( PtrFCB )
	#define AssertVCB( PtrVCB )
	#define AssertFCBorVCB( PtrVCBorFCB )

#endif

#endif	// _EXT2_FSD_H_
