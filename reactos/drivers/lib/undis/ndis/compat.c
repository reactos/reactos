struct _NDIS_BUFFER;
#define __NTDRIVER__
#include <ndissys.h>
#include <buffer.h>
#undef  __NTDRIVER__
#include <ndishack.h>

typedef VOID (*PLOOKASIDE_MINMAX_ROUTINE)(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth);

/* global list and lock of Miniports NDIS has registered */
LIST_ENTRY MiniportListHead;
KSPIN_LOCK MiniportListLock;

/* global list and lock of adapters NDIS has registered */
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;

/* global list and lock of orphan adapters waiting to be claimed by a miniport */
LIST_ENTRY OrphanAdapterListHead;
KSPIN_LOCK OrphanAdapterListLock;

static KSPIN_LOCK ExpGlobalListLock = { 0, };

PNDIS_BUFFER_POOL GlobalBufferPool;
PNDIS_PACKET_POOL GlobalPacketPool;

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;

LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;

PLOOKASIDE_MINMAX_ROUTINE ExpMinMaxRoutine;

//#define InitializeListHead(PL) { (PL)->Flink = (PL); (PL)->Blink = (PL); }

/*
 * @implemented
 */
PLIST_ENTRY FASTCALL
ExInterlockedInsertTailList(PLIST_ENTRY ListHead,
			    PLIST_ENTRY ListEntry,
			    PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts an entry at the tail of a doubly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
 * RETURNS: The previous head of the list
 */
{
  PLIST_ENTRY Old;
  KIRQL oldlvl;

  KeAcquireSpinLock(Lock,&oldlvl);
  if (IsListEmpty(ListHead))
    {
      Old = NULL;
    }
  else
    {
      Old = ListHead->Blink;
    }
  InsertTailList(ListHead,ListEntry);
  KeReleaseSpinLock(Lock,oldlvl);

  return(Old);
}

static
inline
PSINGLE_LIST_ENTRY
 PopEntrySList(
	PSLIST_HEADER	ListHead
	)
{
	PSINGLE_LIST_ENTRY ListEntry;

	ListEntry = ListHead->Next.Next;
	if (ListEntry!=NULL)
	{
		ListHead->Next.Next = ListEntry->Next;
    ListHead->Depth++;
    ListHead->Sequence++;
  }
	return ListEntry;
}


static
inline
VOID
PushEntrySList (
	PSLIST_HEADER	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	)
{
	Entry->Next = ListHead->Next.Next;
	ListHead->Next.Next = Entry;
  ListHead->Depth++;
  ListHead->Sequence++;
}

VOID ExpDefaultMinMax(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth)
/*
 * FUNCTION: Determines the minimum and maximum depth of a new lookaside list
 * ARGUMENTS:
 *   Type         = Type of executive pool
 *   Size         = Size in bytes of each element in the new lookaside list
 *   MinimumDepth = Buffer to store minimum depth of the new lookaside list in
 *   MaximumDepth = Buffer to store maximum depth of the new lookaside list in
 */
{
  /* FIXME: Could probably do some serious computing here */
  if ((PoolType == NonPagedPool) ||
    (PoolType == NonPagedPoolMustSucceed))
  {
    *MinimumDepth = 10;
    *MaximumDepth = 100;
  }
  else
  {
    *MinimumDepth = 20;
    *MaximumDepth = 200;
  }
}

PVOID STDCALL
ExpDefaultAllocate(POOL_TYPE PoolType,
		   ULONG NumberOfBytes,
		   ULONG Tag)
/*
 * FUNCTION: Default allocate function for lookaside lists
 * ARGUMENTS:
 *   Type          = Type of executive pool
 *   NumberOfBytes = Number of bytes to allocate
 *   Tag           = Tag to use
 * RETURNS:
 *   Pointer to allocated memory, or NULL if there is not enough free resources
 */
{
  return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}


VOID STDCALL
ExpDefaultFree(PVOID Buffer)
/*
 * FUNCTION: Default free function for lookaside lists
 * ARGUMENTS:
 *   Buffer = Pointer to memory to free
 */
{
  ExFreePool(Buffer);
}

#define INIT_FUNCTION

VOID INIT_FUNCTION
ExpInitLookasideLists()
{
  InitializeListHead(&ExpNonPagedLookasideListHead);
  KeInitializeSpinLock(&ExpNonPagedLookasideListLock);

  InitializeListHead(&ExpPagedLookasideListHead);
  KeInitializeSpinLock(&ExpPagedLookasideListLock);

  /* FIXME: Possibly configure the algorithm using the registry */
  ExpMinMaxRoutine = ExpDefaultMinMax;
}


PVOID
FASTCALL
ExiAllocateFromPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  PVOID Entry;

  /* Try to obtain an entry from the lookaside list. If that fails, try to
     allocate a new entry with the allocate method for the lookaside list */

  Lookaside->TotalAllocates++;

//  ExAcquireFastMutex(LookasideListLock(Lookaside));

  Entry = PopEntrySList(&Lookaside->ListHead);

//  ExReleaseFastMutex(LookasideListLock(Lookaside));

  if (Entry)
    return Entry;

  Lookaside->AllocateMisses++;

  Entry = (*Lookaside->Allocate)(Lookaside->Type,
    Lookaside->Size,
    Lookaside->Tag);

  return Entry;
}

#define LookasideListLock(l)(&(l->Obsoleted))

/*
 * @implemented
 */
VOID
STDCALL
ExDeleteNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  KIRQL OldIrql;
  PVOID Entry;

  /* Pop all entries off the stack and release the resources allocated
     for them */
  while ((Entry = ExInterlockedPopEntrySList(
    &Lookaside->ListHead,
    LookasideListLock(Lookaside))) != NULL)
  {
    (*Lookaside->Free)(Entry);
  }

  KeAcquireSpinLock(&ExpNonPagedLookasideListLock, &OldIrql);
  RemoveEntryList(&Lookaside->ListEntry);
  KeReleaseSpinLock(&ExpNonPagedLookasideListLock, OldIrql);
}


/*
 * @implemented
 */
VOID
STDCALL
ExDeletePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  KIRQL OldIrql;
  PVOID Entry;

  /* Pop all entries off the stack and release the resources allocated
     for them */
  for (;;)
  {

//  ExAcquireFastMutex(LookasideListLock(Lookaside));

    Entry = PopEntrySList(&Lookaside->ListHead);
    if (!Entry)
      break;

//  ExReleaseFastMutex(LookasideListLock(Lookaside));

    (*Lookaside->Free)(Entry);
  }

  KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);
  RemoveEntryList(&Lookaside->ListEntry);
  KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);
}


VOID
STDCALL
ExiFreeToPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	)
{
	Lookaside->TotalFrees++;

	if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth)
	{
		Lookaside->FreeMisses++;
		(*Lookaside->Free)(Entry);
	}
	else
	{
//  ExAcquireFastMutex(LookasideListLock(Lookaside));
    PushEntrySList(&Lookaside->ListHead, (PSINGLE_LIST_ENTRY)Entry);
//  ExReleaseFastMutex(LookasideListLock(Lookaside));
	}
}


/*
 * @implemented
 */
VOID
STDCALL
ExInitializeNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth)
{
    DbgPrint("Initializing nonpaged lookaside list at 0x%X\n", Lookaside);

    Lookaside->TotalAllocates = 0;
    Lookaside->AllocateMisses = 0;
    Lookaside->TotalFrees = 0;
    Lookaside->FreeMisses = 0;
    Lookaside->Type = NonPagedPool;
    Lookaside->Tag = Tag;

    /* We use a field of type SINGLE_LIST_ENTRY as a link to the next entry in
       the lookaside list so we must allocate at least sizeof(SINGLE_LIST_ENTRY) */
    if (Size < sizeof(SINGLE_LIST_ENTRY))
	Lookaside->Size = sizeof(SINGLE_LIST_ENTRY);
    else
	Lookaside->Size = Size;

    if (Allocate)
	Lookaside->Allocate = Allocate;
    else
	Lookaside->Allocate = ExpDefaultAllocate;

    if (Free)
	Lookaside->Free = Free;
    else
	Lookaside->Free = ExpDefaultFree;

    ExInitializeSListHead(&Lookaside->ListHead);
    KeInitializeSpinLock(LookasideListLock(Lookaside));

    /* Determine minimum and maximum number of entries on the lookaside list
       using the configured algorithm */
    (*ExpMinMaxRoutine)(
	NonPagedPool,
	Lookaside->Size,
	&Lookaside->Depth,
	&Lookaside->MaximumDepth);

    ExInterlockedInsertTailList(
	&ExpNonPagedLookasideListHead,
	&Lookaside->ListEntry,
	&ExpNonPagedLookasideListLock);
}

#define NTOSAPI
#define DDKFASTAPI FASTCALL

/*
 * @implemented
 */
PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPopEntrySList(IN PSLIST_HEADER ListHead,
			   IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes (pops) an entry from a sequenced list
 * ARGUMENTS:
 *          ListHead = Points to the head of the list
 *          Lock     = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
  PSINGLE_LIST_ENTRY ret;
  KIRQL oldlvl;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret = PopEntryList(&ListHead->Next);
  if (ret)
    {
      ListHead->Depth--;
      ListHead->Sequence++;
    }
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}

/*
 * @implemented
 */
PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
			    IN PSINGLE_LIST_ENTRY ListEntry,
			    IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts (pushes) an entry into a sequenced list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
 * RETURNS: The previous head of the list
 */
{
  KIRQL oldlvl;
  PSINGLE_LIST_ENTRY ret;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret=ListHead->Next.Next;
  PushEntryList(&ListHead->Next,ListEntry);
  ListHead->Depth++;
  ListHead->Sequence++;
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}

/*
 * @implemented
 */
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(IN PSLIST_HEADER ListHead)
{
  return (PSLIST_ENTRY) ExInterlockedPopEntrySList(ListHead,
    &ExpGlobalListLock);
}


/*
 * @implemented
 */
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
  IN PSLIST_ENTRY ListEntry)
{
  return (PSLIST_ENTRY) ExInterlockedPushEntrySList(ListHead,
    ListEntry,
    &ExpGlobalListLock);
}

LONG FASTCALL InterlockedIncrement( PLONG Addend ) { return ++(*Addend); }
LONG FASTCALL InterlockedDecrement( PLONG Addend ) { return --(*Addend); }
VOID STDCALL KeBugCheck(ULONG x) { assert(0); }
PVOID STDCALL ExAllocatePool( POOL_TYPE Type, ULONG Bytes ) {
    return ExAllocatePoolWithTag( 0, Type, Bytes );
}
PVOID STDCALL ExAllocatePoolWithTag( ULONG Tag, POOL_TYPE Type, ULONG Bytes ) {
    PUINT Data = malloc( Bytes + 4 );
    if( Data ) { Data[0] = Tag; return &Data[1]; }
    return Data;
}
VOID STDCALL KeInitializeSpinLock( PKSPIN_LOCK Lock ) { }
VOID STDCALL ExFreePool( PVOID Data ) { free( Data ); }
VOID STDCALL KeAcquireSpinLock( PKSPIN_LOCK Lock, PKIRQL Irql ) { }
VOID STDCALL KeReleaseSpinLock( PKSPIN_LOCK Lock, KIRQL Irql ) { }
VOID STDCALL KeAcquireSpinLockAtDpcLevel( PKSPIN_LOCK Lock ) { }
VOID STDCALL KeReleaseSpinLockFromDpcLevel( PKSPIN_LOCK Lock ) { }
VOID FASTCALL ExAcquireFastMutex( PFAST_MUTEX Mutex ) { }
VOID FASTCALL ExReleaseFastMutex( PFAST_MUTEX Mutex ) { }
VOID STDCALL KeInitializeEvent( PKEVENT Event,
				 EVENT_TYPE	Type,
				 BOOLEAN		State ) { }

#if 0
PLIST_ENTRY FASTCALL ExInterlockedInsertTailList
( PLIST_ENTRY Head,
  PLIST_ENTRY Item,
  PKSPIN_LOCK Lock ) {
    return InsertTailList( Head, Item );
}
#endif

UINT RecursiveMutexEnter( struct _RECURSIVE_MUTEX *RM, BOOLEAN Write ) {
    return 0;
}
VOID RecursiveMutexLeave( struct _RECURSIVE_MUTEX *RM ) { }
VOID RecursiveMutexInit( struct _RECURSIVE_MUTEX *RM ) { }

static LIST_ENTRY WorkQueue = { &WorkQueue, &WorkQueue };

VOID STDCALL ExQueueWorkItem( PWORK_QUEUE_ITEM WorkItem,
			       WORK_QUEUE_TYPE Type ) {
    InsertTailList( &WorkQueue, &WorkItem->List );
}

LIST_ENTRY Timers = { &Timers, &Timers };
LARGE_INTEGER CurTime = { };

VOID STDCALL KeInitializeTimer( PKTIMER Timer ) {
}

BOOLEAN STDCALL KeSetTimerEx
( PKTIMER Timer, LARGE_INTEGER DueTime, LONG Period, PKDPC Dpc ) {
    Timer->DueTime.QuadPart = CurTime.QuadPart;
    if( DueTime.QuadPart > 0 ) Timer->DueTime.QuadPart = DueTime.QuadPart;
    else Timer->DueTime.QuadPart -= DueTime.QuadPart;
    Timer->DueTime.QuadPart = DueTime.QuadPart;
    Timer->Dpc = Dpc;
    InsertTailList( &Timers, &Timer->TimerListEntry );
    return TRUE;
}

BOOLEAN STDCALL KeSetTimer
( PKTIMER Timer, LARGE_INTEGER DueTime, PKDPC Dpc ) {
    return KeSetTimer( Timer, DueTime, Dpc );
}

BOOLEAN STDCALL KeCancelTimer( PKTIMER Timer ) {
    PLIST_ENTRY ListEntry;

    for( ListEntry = Timers.Flink;
	 ListEntry != &Timers;
	 ListEntry = ListEntry->Flink ) {
	if( ListEntry == &Timer->TimerListEntry ) {
	    RemoveEntryList( &Timer->TimerListEntry );
	    return TRUE;
	}
    }

    return FALSE;
}

VOID TimerTick( LARGE_INTEGER Time ) {
    PLIST_ENTRY ListEntry;
    PKTIMER Timer;

    while( (ListEntry = RemoveHeadList( &Timers )) ) {
	Timer = CONTAINING_RECORD( ListEntry, KTIMER, TimerListEntry );
	if( Timer->DueTime.QuadPart < Time.QuadPart )
	    (Timer->Dpc->DeferredRoutine)( Timer->Dpc,
					   Timer->Dpc->DeferredContext,
					   Timer->Dpc->SystemArgument1,
					   Timer->Dpc->SystemArgument2 );
    }

    if( Timer )
	InsertHeadList( &Timers, &Timer->TimerListEntry );
}

LONG STDCALL KeSetEvent( PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait ) {
    return 0;
}

/* Host uses this */
PWORK_QUEUE_ITEM GetWorkQueueItem() {
    PLIST_ENTRY ListEntry = RemoveHeadList( &WorkQueue );
    if( ListEntry )
	return CONTAINING_RECORD(ListEntry, WORK_QUEUE_ITEM, List);
    else
	return NULL;
}

NTSTATUS STDCALL KeWaitForSingleObject
( PVOID		Object,
  KWAIT_REASON	WaitReason,
  KPROCESSOR_MODE	WaitMode,
  BOOLEAN		Alertable,
  PLARGE_INTEGER	Timeout
    ) {
    return STATUS_SUCCESS;
}

VOID STDCALL KeInitializeDpc (PKDPC			Dpc,
			      PKDEFERRED_ROUTINE	DeferredRoutine,
			       PVOID			DeferredContext) {
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
}



BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Determines wether a packet has the same destination address as an adapter
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter object
 *     Packet  = Pointer to NDIS packet
 * RETURNS:
 *     TRUE if the destination address is that of the adapter, FALSE if not
 */
{
  UINT Length;
  PUCHAR Start1;
  PUCHAR Start2;
  PNDIS_BUFFER NdisBuffer;
  UINT BufferLength;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

#ifdef DBG
  if(!Adapter)
    {
      NDIS_DbgPrint(MID_TRACE, ("Adapter object was null\n"));
      return FALSE;
    }

  if(!Packet)
    {
      NDIS_DbgPrint(MID_TRACE, ("Packet was null\n"));
      return FALSE;
    }
#endif

  NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

  if (!NdisBuffer)
    {
      NDIS_DbgPrint(MID_TRACE, ("Packet contains no buffers.\n"));
      return FALSE;
    }

  NdisQueryBuffer(NdisBuffer, (PVOID)&Start2, &BufferLength);

  /* FIXME: Should handle fragmented packets */

  switch (Adapter->NdisMiniportBlock.MediaType)
    {
      case NdisMedium802_3:
        Length = ETH_LENGTH_OF_ADDRESS;
        /* Destination address is the first field */
        break;

      default:
        NDIS_DbgPrint(MIN_TRACE, ("Adapter has unsupported media type (0x%X).\n", Adapter->NdisMiniportBlock.MediaType));
        return FALSE;
    }

  if (BufferLength < Length)
    {
        NDIS_DbgPrint(MID_TRACE, ("Buffer is too small.\n"));
        return FALSE;
    }

  Start1 = (PUCHAR)&Adapter->Address;
  NDIS_DbgPrint(MAX_TRACE, ("packet address: %x:%x:%x:%x:%x:%x adapter address: %x:%x:%x:%x:%x:%x\n",
      *((char *)Start1), *(((char *)Start1)+1), *(((char *)Start1)+2), *(((char *)Start1)+3), *(((char *)Start1)+4), *(((char *)Start1)+5),
      *((char *)Start2), *(((char *)Start2)+1), *(((char *)Start2)+2), *(((char *)Start2)+3), *(((char *)Start2)+4), *(((char *)Start2)+5))
  );

  return (RtlCompareMemory((PVOID)Start1, (PVOID)Start2, Length) == Length);
}

VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet)
{
//#ifdef DBG
#if 0
    ULONG i, Length;
    UCHAR Buffer[64];
    if ((DebugTraceLevel | DEBUG_PACKET) > 0) {
        Length = CopyPacketToBuffer(
            (PUCHAR)&Buffer,
            Packet,
            0,
            64);

        DbgPrint("*** PACKET START ***");

        for (i = 0; i < Length; i++) {
            if (i % 12 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", Buffer[i]);
        }

        DbgPrint("*** PACKET STOP ***\n");
    }
#endif /* DBG */
}

NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest)
/*
 * FUNCTION: Sends a request to a miniport
 * ARGUMENTS:
 *     Adapter     = Pointer to logical adapter object
 *     NdisRequest = Pointer to NDIS request structure describing request
 * RETURNS:
 *     Status of operation
 */
{
  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  Adapter->NdisMiniportBlock.MediaRequest = NdisRequest;

  switch (NdisRequest->RequestType)
    {
      case NdisRequestQueryInformation:
        return (*Adapter->Miniport->Chars.QueryInformationHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            NdisRequest->DATA.QUERY_INFORMATION.Oid,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);
        break;

      case NdisRequestSetInformation:
        return (*Adapter->Miniport->Chars.SetInformationHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            NdisRequest->DATA.SET_INFORMATION.Oid,
            NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
            NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded);
        break;

      default:
        return NDIS_STATUS_FAILURE;
    }
}


VOID
MiniDisplayPacket2(
    PVOID  HeaderBuffer,
    UINT   HeaderBufferSize,
    PVOID  LookaheadBuffer,
    UINT   LookaheadBufferSize)
{
#ifdef DBG
    if ((DebugTraceLevel | DEBUG_PACKET) > 0) {
        ULONG i, Length;
        PUCHAR p;

        DbgPrint("*** RECEIVE PACKET START ***\n");
        DbgPrint("HEADER:");
        p = HeaderBuffer;
        for (i = 0; i < HeaderBufferSize; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p);
            *(ULONG_PTR*)p += 1;
        }

        DbgPrint("\nFRAME:");

        p = LookaheadBuffer;
        Length = (LookaheadBufferSize < 64)? LookaheadBufferSize : 64;
        for (i = 0; i < Length; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p);
            *(ULONG_PTR*)p += 1;
        }

        DbgPrint("\n*** RECEIVE PACKET STOP ***\n");
    }
#endif /* DBG */
}

VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize)
/*
 * FUNCTION: Indicate received data to bound protocols
 * ARGUMENTS:
 *     Adapter             = Pointer to logical adapter
 *     MacReceiveContext   = MAC receive context handle
 *     HeaderBuffer        = Pointer to header buffer
 *     HeaderBufferSize    = Size of header buffer
 *     LookaheadBuffer     = Pointer to lookahead buffer
 *     LookaheadBufferSize = Size of lookahead buffer
 *     PacketSize          = Total size of received packet
 */
{
  /* KIRQL OldIrql; */
  PLIST_ENTRY CurrentEntry;
  PADAPTER_BINDING AdapterBinding;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called. Adapter (0x%X)  HeaderBuffer (0x%X)  "
      "HeaderBufferSize (0x%X)  LookaheadBuffer (0x%X)  LookaheadBufferSize (0x%X).\n",
      Adapter, HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize));

  MiniDisplayPacket2(HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize);

  /*
   * XXX Think about this.  This is probably broken.  Spinlocks are
   * taken out for now until i comprehend the Right Way to do this.
   *
   * This used to acquire the MiniportBlock spinlock and hold it until
   * just before the call to ReceiveHandler.  It would then release and
   * subsequently re-acquire the lock.
   *
   * I don't see how this does any good, as it would seem he's just
   * trying to protect the packet list.  If someobdy else dequeues
   * a packet, we are in fact in bad shape, but we don't want to
   * necessarily call the receive handler at elevated irql either.
   *
   * therefore: We *are* going to call the receive handler at high irql
   * (due to holding the lock) for now, and eventually we have to
   * figure out another way to protect this packet list.
   *
   * UPDATE: this is busted; this results in a recursive lock acquisition.
   */
  //NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  //KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    {
      CurrentEntry = Adapter->ProtocolListHead.Flink;
      NDIS_DbgPrint(DEBUG_MINIPORT, ("CurrentEntry = %x\n", CurrentEntry));

      if (CurrentEntry == &Adapter->ProtocolListHead)
        {
          NDIS_DbgPrint(DEBUG_MINIPORT, ("WARNING: No upper protocol layer.\n"));
        }

      while (CurrentEntry != &Adapter->ProtocolListHead)
        {
          AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);
	  NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterBinding = %x\n", AdapterBinding));

          /* see above */
          /* KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql); */

#ifdef DBG
          if(!AdapterBinding)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding was null\n"));
              break;
            }

          if(!AdapterBinding->ProtocolBinding)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding->ProtocolBinding was null\n"));
              break;
            }

          if(!AdapterBinding->ProtocolBinding->Chars.u4.ReceiveHandler)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding->ProtocolBinding->Chars.u4.ReceiveHandler was null\n"));
              break;
            }
#endif

	  NDIS_DbgPrint
	      (MID_TRACE,
	       ("XXX (%x) %x %x %x %x %x %x %x XXX\n",
		*AdapterBinding->ProtocolBinding->Chars.u4.ReceiveHandler,
		AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
		MacReceiveContext,
		HeaderBuffer,
		HeaderBufferSize,
		LookaheadBuffer,
		LookaheadBufferSize,
		PacketSize));

          /* call the receive handler */
          (*AdapterBinding->ProtocolBinding->Chars.u4.ReceiveHandler)(
              AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
              MacReceiveContext,
              HeaderBuffer,
              HeaderBufferSize,
              LookaheadBuffer,
              LookaheadBufferSize,
              PacketSize);

          /* see above */
          /* KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql); */

          CurrentEntry = CurrentEntry->Flink;
        }
    }
  //KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName)
/*
 * FUNCTION: Finds an adapter object by name
 * ARGUMENTS:
 *     AdapterName = Pointer to name of adapter
 * RETURNS:
 *     Pointer to logical adapter object, or NULL if none was found.
 *     If found, the adapter is referenced for the caller. The caller
 *     is responsible for dereferencing after use
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PLOGICAL_ADAPTER Adapter = 0;

  ASSERT(AdapterName);

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if(IsListEmpty(&AdapterListHead))
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("No registered miniports for protocol to bind to\n"));
      return NULL;
    }

  KeAcquireSpinLock(&AdapterListLock, &OldIrql);
    {
      do
        {
          CurrentEntry = AdapterListHead.Flink;

          while (CurrentEntry != &AdapterListHead)
            {
              Adapter = CONTAINING_RECORD(CurrentEntry, LOGICAL_ADAPTER, ListEntry);

              ASSERT(Adapter);

              NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterName = %wZ\n", &AdapterName));
              NDIS_DbgPrint(DEBUG_MINIPORT, ("DeviceName = %wZ\n", &Adapter->DeviceName));

              if (RtlCompareUnicodeString(AdapterName, &Adapter->DeviceName, TRUE) == 0)
                {
                  ReferenceObject(Adapter);
                  break;
                }

              CurrentEntry = CurrentEntry->Flink;
            }
        } while (0);
    }
  KeReleaseSpinLock(&AdapterListLock, OldIrql);

  if(Adapter)
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving. Adapter found at 0x%x\n", Adapter));
    }
  else
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving (adapter not found).\n"));
    }

  return Adapter;
}

/*
 * @implemented
 */
ULONG DDKAPI
NDIS_BUFFER_TO_SPAN_PAGES(
    IN  PNDIS_BUFFER    Buffer)
/*
 * FUNCTION: Determines how many physical pages a buffer is made of
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer descriptor
 */
{
    if (Buffer->ByteCount == 0)
        return 1;

    return ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            MmGetMdlVirtualAddress(Buffer),
            MmGetMdlByteCount(Buffer));
}

/*
 * @implemented
 */
VOID DDKAPI
NdisAllocateBuffer(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_BUFFER    * Buffer,
    IN  NDIS_HANDLE     PoolHandle,
    IN  PVOID           VirtualAddress,
    IN  UINT            Length)
/*
 * FUNCTION: Allocates an NDIS buffer descriptor
 * ARGUMENTS:
 *     Status         = Address of buffer for status
 *     Buffer         = Address of buffer for NDIS buffer descriptor
 *     PoolHandle     = Handle returned by NdisAllocateBufferPool
 *     VirtualAddress = Pointer to virtual address of data buffer
 *     Length         = Number of bytes in data buffer
 */
{
    KIRQL OldIrql;
    PNETWORK_HEADER Temp;
    PNDIS_BUFFER_POOL Pool = (PNDIS_BUFFER_POOL)PoolHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  Buffer (0x%X)  PoolHandle (0x%X)  "
        "VirtualAddress (0x%X)  Length (%d)\n",
        Status, Buffer, PoolHandle, VirtualAddress, Length));

#if 0
    Temp = Pool->FreeList;
    while( Temp ) {
	NDIS_DbgPrint(MID_TRACE,("Free buffer -> %x\n", Temp));
	Temp = Temp->Next;
    }

    NDIS_DbgPrint(MID_TRACE,("|:. <- End free buffers"));
#endif

    if(!VirtualAddress && !Length) return;

    KeAcquireSpinLock(&Pool->SpinLock, &OldIrql);

    if (Pool->FreeList) {
        Temp           = Pool->FreeList;
        Pool->FreeList = Temp->Next;

        KeReleaseSpinLock(&Pool->SpinLock, OldIrql);

        Temp->Next = NULL;

	Temp->Mdl.Next = (PMDL)NULL;
	Temp->Mdl.Size = (CSHORT)(sizeof(MDL) +
				  (ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, Length) * sizeof(ULONG)));
	Temp->Mdl.MdlFlags   = (MDL_SOURCE_IS_NONPAGED_POOL | MDL_ALLOCATED_FIXED_SIZE);
	;	    Temp->Mdl.StartVa    = (PVOID)PAGE_ROUND_DOWN(VirtualAddress);
	Temp->Mdl.ByteOffset = (ULONG_PTR)(VirtualAddress - PAGE_ROUND_DOWN(VirtualAddress));
	Temp->Mdl.ByteCount  = Length;
        Temp->Mdl.MappedSystemVa = VirtualAddress;

        Temp->BufferPool = Pool;

        *Buffer = (PNDIS_BUFFER)Temp;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        KeReleaseSpinLock(&Pool->SpinLock, OldIrql);
        *Status = NDIS_STATUS_FAILURE;
	NDIS_DbgPrint(MID_TRACE, ("Can't get another packet.\n"));
	KeBugCheck(0);
    }
}

/*
 * @implemented
 */
VOID DDKAPI
NdisAllocatePacket(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    * Packet,
    IN  NDIS_HANDLE     PoolHandle)
/*
 * FUNCTION: Allocates an NDIS packet descriptor
 * ARGUMENTS:
 *     Status     = Address of buffer for status
 *     Packet     = Address of buffer for packet descriptor
 *     PoolHandle = Handle returned by NdisAllocatePacketPool
 */
{
    KIRQL OldIrql;
    PNDIS_PACKET Temp;
    PNDIS_PACKET_POOL Pool = (PNDIS_PACKET_POOL)PoolHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  Packet (0x%X)  PoolHandle (0x%X).\n",
        Status, Packet, PoolHandle));

    KeAcquireSpinLock(&Pool->SpinLock.SpinLock, &OldIrql);

    if (Pool->FreeList) {
        Temp           = Pool->FreeList;
        Pool->FreeList = (PNDIS_PACKET)Temp->Private.Head;

        KeReleaseSpinLock(&Pool->SpinLock.SpinLock, OldIrql);

        RtlZeroMemory(&Temp->Private, sizeof(NDIS_PACKET_PRIVATE));
        Temp->Private.Pool = Pool;

        *Packet = Temp;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        *Status = NDIS_STATUS_RESOURCES;
        KeReleaseSpinLock(&Pool->SpinLock.SpinLock, OldIrql);
    }
}

/*
 * @implemented
 */
VOID DDKAPI
NdisFreeBuffer(
    IN   PNDIS_BUFFER   Buffer)
/*
 * FUNCTION: Puts an NDIS buffer descriptor back in it's pool
 * ARGUMENTS:
 *     Buffer = Pointer to buffer descriptor
 */
{
    KIRQL OldIrql;
    PNDIS_BUFFER_POOL Pool;
    PNETWORK_HEADER Temp = (PNETWORK_HEADER)Buffer;

    NDIS_DbgPrint(MAX_TRACE, ("Buffer (0x%X).\n", Buffer));

    Pool = Temp->BufferPool;

    KeAcquireSpinLock(&Pool->SpinLock, &OldIrql);
    Temp->Next     = (PNETWORK_HEADER)Pool->FreeList;
    Pool->FreeList = (PNETWORK_HEADER)Temp;
    KeReleaseSpinLock(&Pool->SpinLock, OldIrql);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisQueryBuffer(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length)
/*
 * FUNCTION:
 *     Queries an NDIS buffer for information
 * ARGUMENTS:
 *     Buffer         = Pointer to NDIS buffer to query
 *     VirtualAddress = Address of buffer to place virtual address
 *     Length         = Address of buffer to place length of buffer
 */
{
	if (VirtualAddress != NULL)
	    *(PVOID*)VirtualAddress = Buffer->MappedSystemVa;

	*Length = MmGetMdlByteCount(Buffer);
}

/*
 * @implemented
 */
VOID DDKAPI NdisFreePacket(IN   PNDIS_PACKET   Packet)
/*
 * FUNCTION: Puts an NDIS packet descriptor back in it's pool
 * ARGUMENTS:
 *     Packet = Pointer to packet descriptor
 */
{
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("Packet (0x%X).\n", Packet));

    KeAcquireSpinLock(&Packet->Private.Pool->SpinLock.SpinLock, &OldIrql);
    Packet->Private.Head           = (PNDIS_BUFFER)Packet->Private.Pool->FreeList;
    Packet->Private.Pool->FreeList = Packet;
    KeReleaseSpinLock(&Packet->Private.Pool->SpinLock.SpinLock, OldIrql);
}

PVOID DDKAPI MmMapLockedPages( PMDL Mdl, KPROCESSOR_MODE Mode ) {
    return Mdl->MappedSystemVa;
}

__inline INT SkipToOffset(
    PNDIS_BUFFER Buffer,
    UINT Offset,
    PCHAR *Data,
    PUINT Size)
/*
 * FUNCTION: Skip Offset bytes into a buffer chain
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer
 *     Offset = Number of bytes to skip
 *     Data   = Address of a pointer that on return will contain the
 *              address of the offset in the buffer
 *     Size   = Address of a pointer that on return will contain the
 *              size of the destination buffer
 * RETURNS:
 *     Offset into buffer, -1 if buffer chain was smaller than Offset bytes
 * NOTES:
 *     Buffer may be NULL
 */
{
    for (;;) {

        if (!Buffer)
            return -1;

        NdisQueryBuffer(Buffer, (PVOID)Data, Size);

        if (Offset < *Size) {
            *Data = (PCHAR)((ULONG_PTR) *Data + Offset);
            *Size              -= Offset;
            break;
        }

        Offset -= *Size;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    return Offset;
}

UINT CopyBufferToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PUCHAR SrcData,
    UINT Length)
/*
 * FUNCTION: Copies data from a buffer to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer
 *     DstOffset = Destination start offset
 *     SrcData   = Pointer to source buffer
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the destination
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, DstSize;
    PCHAR DstData;

    DbgPrint("DstBuffer (0x%X)  DstOffset (0x%X)  SrcData (0x%X)  "
	     "Length (%d)\n", DstBuffer, DstOffset, SrcData, Length);

    /* Skip DstOffset bytes in the destination buffer chain */
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(DstSize, Length);

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        SrcData      = (PCHAR)((ULONG_PTR)SrcData + BytesToCopy);

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            /* No more bytes in desination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }
    }

    return BytesCopied;
}


UINT CopyBufferChainToBuffer(
    PUCHAR DstData,
    PNDIS_BUFFER SrcBuffer,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS buffer chain to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcBuffer = Pointer to source NDIS buffer
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, SrcSize;
    PCHAR SrcData;

    DbgPrint("DstData 0x%X  SrcBuffer 0x%X  SrcOffset 0x%X  Length %d\n",
	     DstData,SrcBuffer, SrcOffset, Length);

    /* Skip SrcOffset bytes in the source buffer chain */
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(SrcSize, Length);

        DbgPrint("Copying (%d) bytes from 0x%X to 0x%X\n",
		 BytesToCopy, SrcData, DstData);

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        DstData      = (PCHAR)((ULONG_PTR)DstData + BytesToCopy);

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        SrcSize -= BytesToCopy;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    return BytesCopied;
}


UINT CopyPacketToBuffer(
    PUCHAR DstData,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    PNDIS_BUFFER FirstBuffer;
    UINT FirstLength;
    UINT TotalLength;

    DbgPrint("DstData (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  "
	     "Length (%d)\n", DstData, SrcPacket, SrcOffset, Length);

    NdisQueryPacket( SrcPacket, &FirstBuffer, 0, &FirstLength, &TotalLength );
#if 0
    NdisGetFirstBufferFromPacket(SrcPacket,
                                 &FirstBuffer,
                                 &Address,
                                 &FirstLength,
                                 &TotalLength);
#endif

    return CopyBufferChainToBuffer(DstData, FirstBuffer, SrcOffset, Length);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           Offset,
    OUT PUINT           Length)
{
    *((PUINT)Offset) = MmGetMdlByteOffset(Buffer);
    *((PUINT)Length) = MmGetMdlByteCount(Buffer);
}

UINT CopyPacketToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer
 *     DstOffset = Destination start offset
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source and
 *     destination buffer sizes
 */
{
    PNDIS_BUFFER SrcBuffer;
    PCHAR DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    DbgPrint("DstBuffer (0x%X)  DstOffset (0x%X)  SrcPacket (0x%X)  "
	     "SrcOffset (0x%X)  Length (%d)\n",
	     DstBuffer, DstOffset, SrcPacket, SrcOffset, Length);

    /* Skip DstOffset bytes in the destination buffer chain */
    NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Skip SrcOffset bytes in the source packet */
    NdisQueryPacket( SrcPacket, &SrcBuffer, 0, &SrcSize, &Total );
    NdisQueryBuffer( SrcBuffer, (PVOID *)&SrcData, &SrcSize );
#if 0
    NdisGetFirstBufferFromPacket(SrcPacket, &SrcBuffer, (PVOID *)&SrcData,
				 &SrcSize, &Total);
#endif
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Copy the data */
    for (Total = 0;;) {
        /* Find out how many bytes we can copy at one time */
        if (Length < SrcSize)
            Count = Length;
        else
            Count = SrcSize;
        if (DstSize < Count)
            Count = DstSize;

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, Count);

        Total  += Count;
        Length -= Count;
        if (Length == 0)
            break;

        DstSize -= Count;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    return Total;
}


PVOID AdjustPacket(
    PNDIS_PACKET Packet,
    UINT Available,
    UINT Needed)
/*
 * FUNCTION: Adjusts the amount of unused space at the beginning of the packet
 * ARGUMENTS:
 *     Packet    = Pointer to packet
 *     Available = Number of bytes available at start of first buffer
 *     Needed    = Number of bytes needed for the header
 * RETURNS:
 *     Pointer to start of packet
 */
{
    PNDIS_BUFFER NdisBuffer;
    INT Adjust;

    DbgPrint("Available = %d, Needed = %d.\n", Available, Needed);

    Adjust = Available - Needed;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    /* If Adjust is zero there is no need to adjust this packet as
       there is no additional space at start the of first buffer */
    if (Adjust != 0) {
        NdisBuffer->MappedSystemVa  = (PVOID) ((ULONG_PTR)(NdisBuffer->MappedSystemVa) + Adjust);
        NdisBuffer->ByteOffset     += Adjust;
        NdisBuffer->ByteCount      -= Adjust;
    }

    return NdisBuffer->MappedSystemVa;
}


UINT ResizePacket(
    PNDIS_PACKET Packet,
    UINT Size)
/*
 * FUNCTION: Resizes an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to packet
 *     Size   = Number of bytes in first buffer
 * RETURNS:
 *     Previous size of first buffer
 */
{
    PNDIS_BUFFER NdisBuffer;
    UINT OldSize;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    OldSize = NdisBuffer->ByteCount;

    if (Size != OldSize)
        NdisBuffer->ByteCount = Size;

    return OldSize;
}

NDIS_STATUS PrependPacket( PNDIS_PACKET Packet, PCHAR Data, UINT Length,
			   BOOLEAN Copy ) {
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;
    PCHAR NewBuf;

    if( Copy ) {
	NewBuf = ExAllocatePool( NonPagedPool, Length );
	if( !NewBuf ) return STATUS_NO_MEMORY;
	RtlCopyMemory( NewBuf, Data, Length );
    } else NewBuf = Data;

    NdisAllocateBuffer( &Status, &Buffer, GlobalBufferPool, Data, Length );
    if( Status != NDIS_STATUS_SUCCESS ) return Status;

    NdisChainBufferAtFront( Packet, Buffer );

    return STATUS_SUCCESS;
}

void GetDataPtr( PNDIS_PACKET Packet,
		 UINT Offset,
		 PCHAR *DataOut,
		 PUINT Size ) {
    PNDIS_BUFFER Buffer;

    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    if( !Buffer ) return;
    SkipToOffset( Buffer, Offset, DataOut, Size );
}

NDIS_STATUS AllocatePacketWithBufferX( PNDIS_PACKET *NdisPacket,
				       PCHAR Data, UINT Len,
				       PCHAR File, UINT Line ) {
#if 0
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;
    PCHAR NewData;

    NewData = ExAllocatePool( NonPagedPool, Len );
    if( !NewData ) return NDIS_STATUS_NOT_ACCEPTED; // XXX
    TrackWithTag(EXALLOC_TAG, NewData, File, Line);

    if( Data )
	RtlCopyMemory(NewData, Data, Len);

    NdisAllocatePacket( &Status, &Packet, GlobalPacketPool );
    if( Status != NDIS_STATUS_SUCCESS ) {
	ExFreePool( NewData );
	return Status;
    }
    TrackWithTag(NDIS_PACKET_TAG, Packet, File, Line);

    NdisAllocateBuffer( &Status, &Buffer, GlobalBufferPool, NewData, Len );
    if( Status != NDIS_STATUS_SUCCESS ) {
	ExFreePool( NewData );
	FreeNdisPacket( Packet );
    }
    TrackWithTag(NDIS_BUFFER_TAG, Buffer, File, Line);

    NdisChainBufferAtFront( Packet, Buffer );
    *NdisPacket = Packet;
#endif

    return NDIS_STATUS_SUCCESS;
}


VOID FreeNdisPacketX
( PNDIS_PACKET Packet,
  PCHAR File,
  UINT Line )
/*
 * FUNCTION: Frees an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet to be freed
 */
{
#if 0
    PNDIS_BUFFER Buffer, NextBuffer;

    TI_DbgPrint(DEBUG_PBUFFER, ("Packet (0x%X)\n", Packet));

    /* Free all the buffers in the packet first */
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        PVOID Data;
        UINT Length;

        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Data, &Length);
	UntrackFL(File,Line,Buffer);
        NdisFreeBuffer(Buffer);
	UntrackFL(File,Line,Data);
        ExFreePool(Data);
    }

    /* Finally free the NDIS packet discriptor */
    UntrackFL(File,Line,Packet);
    NdisFreePacket(Packet);
#endif
}
