#ifndef _NDISHACK_H
#define _NDISHACK_H

#ifndef __NTDRIVER__

struct _RECURSIVE_MUTEX;

#undef KeAcquireSpinLockAtDpcLevel
#undef KeReleaseSpinLockFromDpcLevel
#undef KeRaiseIrql
#undef KeLowerIrql

#define NdisAllocateBuffer XNdisAllocateBuffer
#define NdisAllocatePacket XNdisAllocatePacket
#define NdisFreeBuffer XNdisFreeBuffer
#define NdisFreePacket XNdisFreePacket
#define NDIS_BUFFER_TO_SPAN_PAGES XNDIS_BUFFER_TO_SPAN_PAGES
#define ExAllocatePool XExAllocatePool
#define ExFreePool XExFreePool
#define KeAcquireSpinLock XKeAcquireSpinLock
#define KeAcquireSpinLockAtDpcLevel XKeAcquireSpinLockAtDpcLevel
#define KeReleaseSpinLock XKeReleaseSpinLock
#define KeReleaseSpinLockFromDpcLevel XKeReleaseSpinLockFromDpcLevel
#define KeInitializeSpinLock XKeInitializeSpinLock
#define InterlockedIncrement XInterlockedIncrement
#define InterlockedDecrement XInterlockedDecrement
#define ExQueueWorkItem XExQueueWorkItem
#define ExAcquireFastMutex XExAcquireFastMutex
#define ExReleaseFastMutex XExReleaseFastMutex
#define KeSetEvent XKeSetEvent
#define KeBugCheck XKeBugCheck
#define ExInterlockedInsertTailList XExInterlockedInsertTailList
#define KeInitializeEvent XKeInitializeEvent
#define KeRaiseIrql XKeRaiseIrql
#define KeLowerIrql XKeLowerIrql
#define ExFreeToNPagedLookasideList XExFreeToNPagedLookasideList
#define ExAllocateFromNPagedLookasideList XExAllocateFromNPagedLookasideList
#define ExInitializeNPagedLookasideList XExInitializeNPagedLookasideList
#define KeInitializeTimer XKeInitializeTimer
#define KeSetTimer XKeSetTimer
#define KeSetTimerEx XKeSetTimerEx
#define KeCancelTimer XKeCancelTimer
#define KeWaitForSingleObject XKeWaitForSingleObject
#define KeInitializeDpc XKeInitializeDpc

extern VOID XNdisAllocateBuffer( PNDIS_STATUS Status, PNDIS_BUFFER *Buffer, NDIS_HANDLE BufferPool, PVOID Data, UINT Size );
extern VOID XNdisFreeBuffer( PNDIS_BUFFER Buffer );
extern VOID XNdisAllocatePacket( PNDIS_STATUS Status, PNDIS_PACKET *Packet, NDIS_HANDLE PacketPool );
extern VOID XNdisFreePacket( PNDIS_PACKET Packet );
extern ULONG XNDIS_BUFFER_TO_SPAN_PAGES(IN  PNDIS_BUFFER    Buffer);
extern PVOID STDCALL XExAllocatePool( POOL_TYPE Type, ULONG Size );
extern VOID STDCALL XExFreePool( PVOID Buffer );
extern VOID STDCALL XKeRaiseIrql( KIRQL NewIrql, PKIRQL OldIrql );
extern VOID STDCALL XKeLowerIrql( KIRQL NewIrql );
extern VOID STDCALL XKeAcquireSpinLock( PKSPIN_LOCK Lock, PKIRQL Irql );
extern VOID STDCALL XKeReleaseSpinLock( PKSPIN_LOCK Lock, KIRQL Irql );
extern VOID STDCALL XKeAcquireSpinLockAtDpcLevel( PKSPIN_LOCK Lock );
extern VOID STDCALL XKeReleaseSpinLockFromDpcLevel( PKSPIN_LOCK Lock );
extern VOID STDCALL XKeInitializeSpinLock( PKSPIN_LOCK Lock );
extern LONG FASTCALL XInterlockedIncrement( PLONG Addend );
extern LONG FASTCALL XInterlockedDecrement( PLONG Addend );
extern VOID STDCALL XExQueueWorkItem( PWORK_QUEUE_ITEM WorkItem, 
				      WORK_QUEUE_TYPE Type );
extern VOID STDCALL XExAcquireFastMutex( PFAST_MUTEX Mutex );
extern VOID STDCALL XExReleaseFastMutex( PFAST_MUTEX Mutex );
extern LONG STDCALL XKeSetEvent( PKEVENT Event, KPRIORITY Increment, 
				 BOOLEAN Wiat );
extern VOID STDCALL XKeBugCheck( ULONG Code );
extern VOID STDCALL XExInterlockedInsertTailList( PLIST_ENTRY Head,
						  PLIST_ENTRY Item,
						  PKSPIN_LOCK Lock );
extern VOID STDCALL XKeInitializeTimer( PKTIMER Timer );
extern VOID STDCALL XKeInitializeEvent( PKEVENT Event,
					EVENT_TYPE	Type,
					BOOLEAN		State);
extern VOID STDCALL XKeSetTimer( PKTIMER Timer,
				 LARGE_INTEGER DueTime,
				 PKDPC Dpc );
extern VOID STDCALL XKeSetTimerEx( PKTIMER Timer,
				   LARGE_INTEGER DueTime,
				   LONG Period,
				   PKDPC Dpc );
extern VOID STDCALL XKeCancelTimer( PKTIMER Timer );
extern NTSTATUS STDCALL XKeWaitForSingleObject
( PVOID		Object,
  KWAIT_REASON	WaitReason,
  KPROCESSOR_MODE	WaitMode,
  BOOLEAN		Alertable,
  PLARGE_INTEGER	Timeout
    );
extern VOID STDCALL KeInitializeDpc (PKDPC			Dpc,
				     PKDEFERRED_ROUTINE	DeferredRoutine,
				     PVOID			DeferredContext);


extern UINT RecursiveMutexEnter( struct _RECURSIVE_MUTEX *RM, BOOLEAN Write );
extern VOID RecursiveMutexLeave( struct _RECURSIVE_MUTEX *RM );
extern VOID RecursiveMutexInit( struct _RECURSIVE_MUTEX *RM );
extern VOID STDCALL ExDeleteNPagedLookasideList
(PNPAGED_LOOKASIDE_LIST	Lookaside);
extern VOID STDCALL ExInitializeNPagedLookasideList
( PNPAGED_LOOKASIDE_LIST	Lookaside,
  PALLOCATE_FUNCTION	Allocate,
  PFREE_FUNCTION		Free,
  ULONG			Flags,
  ULONG			Size,
  ULONG			Tag,
  USHORT			Depth );



#undef NdisGetFirstBufferFromPacket
#undef NdisQueryBuffer
#undef NdisQueryPacket

/*
 * VOID
 * ExFreeToNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	Lookaside,
 *	PVOID			Entry
 *	);
 *
 * FUNCTION:
 *	Inserts (pushes) the specified entry into the specified
 *	nonpaged lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to the nonpaged lookaside list
 *	Entry = Pointer to the entry that is inserted in the lookaside list
 */
static
inline
VOID
ExFreeToNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside,
	IN	PVOID			Entry
	)
{
    Lookaside->Free( Entry );
}

/*
 * PVOID
 * ExAllocateFromNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	LookSide
 *	);
 *
 * FUNCTION:
 *	Removes (pops) the first entry from the specified nonpaged
 *	lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to a nonpaged lookaside list
 *
 * RETURNS:
 *	Address of the allocated list entry
 */
static
inline
PVOID
ExAllocateFromNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
	PVOID Entry;

	Entry = (Lookaside->Allocate)(Lookaside->Type,
				      Lookaside->Size,
				      Lookaside->Tag);

	return Entry;
}

/*
 * VOID
 * NdisGetFirstBufferFromPacket(
 *   IN PNDIS_PACKET  _Packet,
 *   OUT PNDIS_BUFFER  * _FirstBuffer,
 *   OUT PVOID  * _FirstBufferVA,
 *   OUT PUINT  _FirstBufferLength,
 *   OUT PUINT  _TotalBufferLength)
 */
#define	NdisGetFirstBufferFromPacket(_Packet,             \
                                     _FirstBuffer,        \
                                     _FirstBufferVA,      \
                                     _FirstBufferLength,  \
                                     _TotalBufferLength)  \
{                                                         \
  PNDIS_BUFFER _Buffer;                                   \
                                                          \
  _Buffer         = (_Packet)->Private.Head;              \
  *(_FirstBuffer) = _Buffer;                              \
  if (_Buffer != NULL)                                    \
    {                                                     \
	    *(_FirstBufferVA)     = (_Buffer)->MappedSystemVa;                \
	    *(_FirstBufferLength) = (_Buffer)->Size;                          \
	    _Buffer = _Buffer->Next;                                          \
		  *(_TotalBufferLength) = *(_FirstBufferLength);              \
		  while (_Buffer != NULL) {                                   \
		    *(_TotalBufferLength) += (_Buffer)->Size;                 \
		    _Buffer = _Buffer->Next;                                  \
		  }                                                           \
    }                             \
  else                            \
    {                             \
      *(_FirstBufferVA) = 0;      \
      *(_FirstBufferLength) = 0;  \
      *(_TotalBufferLength) = 0;  \
    } \
}

/*
 * VOID
 * NdisQueryBuffer(
 *   IN PNDIS_BUFFER  Buffer,
 *   OUT PVOID  *VirtualAddress OPTIONAL,
 *   OUT PUINT  Length)
 */
#define NdisQueryBuffer(Buffer,         \
                        VirtualAddress, \
                        Length)         \
{                                       \
	if (VirtualAddress)                   \
		*((PVOID*)VirtualAddress) = Buffer->MappedSystemVa; \
                                        \
	*((PUINT)Length) = (Buffer)->Size; \
}

/*
 * VOID
 * NdisQueryPacket(
 *     IN  PNDIS_PACKET    Packet,
 *     OUT PUINT           PhysicalBufferCount OPTIONAL,
 *     OUT PUINT           BufferCount         OPTIONAL,
 *     OUT PNDIS_BUFFER    *FirstBuffer        OPTIONAL,
 *     OUT PUINT           TotalPacketLength   OPTIONAL);
 */
#define NdisQueryPacket(Packet,                                                 \
                        PhysicalBufferCount,                                    \
                        BufferCount,                                            \
                        FirstBuffer,                                            \
                        TotalPacketLength)                                      \
{                                                                               \
    if (FirstBuffer)                                                            \
        *((PNDIS_BUFFER*)FirstBuffer) = (Packet)->Private.Head;                 \
    if ((TotalPacketLength) || (BufferCount) || (PhysicalBufferCount))          \
    {                                                                           \
        if (!(Packet)->Private.ValidCounts) {                                   \
            UINT _Offset;                                                       \
            UINT _PacketLength;                                                 \
            PNDIS_BUFFER _NdisBuffer;                                           \
            UINT _PhysicalBufferCount = 0;                                      \
            UINT _TotalPacketLength   = 0;                                      \
            UINT _Count               = 0;                                      \
                                                                                \
            for (_NdisBuffer = (Packet)->Private.Head;                          \
                _NdisBuffer != (PNDIS_BUFFER)NULL;                              \
                _NdisBuffer = _NdisBuffer->Next)                                \
            {                                                                   \
                _PhysicalBufferCount += XNDIS_BUFFER_TO_SPAN_PAGES(_NdisBuffer); \
                NdisQueryBufferOffset(_NdisBuffer, &_Offset, &_PacketLength);   \
                _TotalPacketLength += _PacketLength;                            \
                _Count++;                                                       \
            }                                                                   \
            (Packet)->Private.PhysicalCount = _PhysicalBufferCount;             \
            (Packet)->Private.TotalLength   = _TotalPacketLength;               \
            (Packet)->Private.Count         = _Count;                           \
            (Packet)->Private.ValidCounts   = TRUE;                             \
		}                                                                       \
                                                                                \
        if (PhysicalBufferCount)                                                \
            *((PUINT)PhysicalBufferCount) = (Packet)->Private.PhysicalCount;    \
                                                                                \
        if (BufferCount)                                                        \
            *((PUINT)BufferCount) = (Packet)->Private.Count;                    \
                                                                                \
        if (TotalPacketLength)                                                  \
            *((PUINT)TotalPacketLength) = (Packet)->Private.TotalLength;        \
    }                                                                           \
}

#endif/*__NTDRIVER__*/

#endif/*_NDISHACK_H*/
