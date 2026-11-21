/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lancesft.h

Abstract:

    The main header for a LANCE (Local Area Network Controller
    Am 7990) MAC driver.

Author:

    Anthony V. Ercolano (tonye) creation-date 19-Jun-1990

Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernal mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

--*/

#ifndef _LANCESFT_
#define _LANCESFT_

#define LANCE_NDIS_MAJOR_VERSION 5
#define LANCE_NDIS_MINOR_VERSION 1

//
// Size of the multicast address table in the adapter structure, and so
// the most addresses OID_802_3_MULTICAST_LIST can ever accept.  The
// MaxMulticastList registry value can lower this, not raise it.
//
#define LANCE_MAX_MULTICAST_LIST 32

#if DBG

#define LANCELOG 1
#define LANCE_TRACE 0

#else

#define LANCELOG 0
#define LANCE_TRACE 0

#endif

#if LANCELOG

#define LOG_SIZE 256

#define TIMER      '.'
#define IN_ISR     'i'
#define OUT_ISR    'I'
#define IN_DPC     'd'
#define OUT_DPC    'D'
#define RECEIVE    'R'
#define TRANSMIT   'x'
#define TRANSMIT_COMPLETE   'X'
#define PEND       'p'
#define UNPEND     'P'
#define INDICATE   'r'
#define IN_SEND    's'
#define OUT_SEND   'S'
#define START      'G'
#define RESET_STEP_1 '1'
#define RESET_STEP_2 '2'
#define RESET_SAVE_PACKET     'b'
#define RESET_RECOVER_PACKET  'B'
#define RESET_COMPLETE_PACKET 'c'
#define RESET_STEP_3 '3'
#define REMOVE 'V'
#define CLOSE  'C'
#define UNLOAD 'U'



extern UCHAR Log[LOG_SIZE];

extern UCHAR LogPlace;
extern UCHAR LogWrapped;



#define LOG(c)     { Log[LogPlace] = (c); Log[(LogPlace+3) % 255] ='\0'; \
                     LogPlace = (LogPlace + 1) % 255; }

#else

#define LOG(c)

#endif



//
// ZZZ These macros are peculiar to NT.
//

#define LANCE_ALLOC_PHYS(pp, s) NdisAllocateMemoryWithTag((PVOID *)(pp),(s),'LANC')
#define LANCE_FREE_PHYS(p, s) NdisFreeMemory((PVOID)(p),(s),0)
#define LANCE_MOVE_MEMORY(Destination,Source,Length) NdisMoveMemory(Destination,Source,Length)
#define LANCE_ZERO_MEMORY(Destination,Length) NdisZeroMemory(Destination,Length)



//
// This structure is passed as context from the receive interrupt
// processor.  Eventually it will be used as a parameter to
// LanceTransferData.  LanceTransferData can get two kinds of
// context.  It will receive either an ndis packet or it will
// receive a LANCE_RECEIVE_CONTEXT.  It will be able to tell
// the difference since the LANCE_RECEIVE_CONTEXT will have
// its low bit set.  No pointer to an ndis packet can have its low
// bit set.
//
typedef union _LANCE_RECEIVE_CONTEXT {

    ULONG_PTR WholeThing;
    struct _INFO {
        //
        // Used to mark that this is context rather than a pointer
        // to a packet.
        //
        ULONG_PTR IsContext:1;

        //
        // The first receive ring descriptor used to hold the packet.
        //
        ULONG_PTR FirstBuffer:7;

        //
        // The last receive ring descriptor used to hold the packet.
        //
        ULONG_PTR LastBuffer:7;
    } INFO;

} LANCE_RECEIVE_CONTEXT,*PLANCE_RECEIVE_CONTEXT;






//
// This record type is inserted into the MacReserved portion
// of the packet header when the packet is going through the
// staged allocation of buffer space prior to the actual send.
//
typedef struct _LANCE_RESERVED {

    //
    // Points to the next packet in the chain of queued packets
    // being allocated, loopbacked, or waiting for the finish
    // of transmission.
    //
    // The packet will either be on the stage list for allocation,
    // the loopback list for loopback processing, on an adapter
    // wide doubly linked list (see below) for post transmission
    // processing.
    //
    // We always keep the packet on a list so that in case the
    // the adapter is closing down or resetting, all the packets
    // can easily be located and "canceled".
    //
    PNDIS_PACKET Next;

    //
    // This gives the index into the array of adapter buffer
    // descriptors that contains the packet information.
    //
    USHORT LanceBuffersIndex;

    //
    // When the hardware send is done this will record whether
    // the send was successful or not.
    //
    BOOLEAN SuccessfulTransmit;

} LANCE_RESERVED,*PLANCE_RESERVED;

//
// This macro will return a pointer to the lance reserved portion
// of a packet given a pointer to a packet.
//
#define PLANCE_RESERVED_FROM_PACKET(Packet) \
    ((PLANCE_RESERVED)((Packet)->MacReserved))

//
// This structure is used to map entries in the ring descriptors
// back to the packets from which the data in the ring descriptor
// originated.
//

typedef struct _LANCE_RING_TO_PACKET {

    //
    // Points to the packet from which data is being transmitted
    // through this ring entry.
    //
    PNDIS_PACKET OwningPacket;

    //
    // Index of the ring entry that is being used by the packet.
    //
    UINT RingIndex;

    //
    // When a packet is submitted to the hardware we record
    // here whether it used adapter buffers and if so, the buffer
    // index.
    //
    UINT LanceBuffersIndex;

} LANCE_RING_TO_PACKET,*PLANCE_RING_TO_PACKET;

//
// If an ndis packet does not meet the hardware contraints then
// an adapter buffer will be allocated.  Enough data will be copied
// out of the ndis packet so that by using a combination of the
// adapter buffer and remaining ndis buffers the hardware
// constraints are satisfied.
//
// In the LANCE_ADAPTER structure three threaded lists are kept in
// one array.  One points to a list of LANCE_BUFFER_DESCRIPTORS
// that point to small adapter buffers.  Another is for medium sized
// buffers and the last for full sized (large) buffers.
//
// The allocation is controlled via a free list head and
// the free lists are "threaded" by a field in the adapter buffer
// descriptor.
//
typedef struct _LANCE_BUFFER_DESCRIPTOR {

    //
    // A virtual pointer to a small, medium, or large buffer.
    //
    PVOID VirtualLanceBuffer;

    //
    // Threads the elements of an array of these descriptors into
    // a free list. -1 implies no more entries in the list.
    //
    INT Next;

    //
    // Holds the amount of space (in bytes) available in the buffer
    //
    UINT BufferSize;

    //
    // Holds the length of data placed into the buffer.  This
    // can (and likely will) be less that the actual buffers
    // length.
    //
    UINT DataLength;

} LANCE_BUFFER_DESCRIPTOR,*PLANCE_BUFFER_DESCRIPTOR;


#define LANCE_LENGTH_OF_ADDRESS 6

//
// Define the size of the ethernet header.
//
#define LANCE_HEADER_SIZE 14

//
// Define Maximum number of bytes a protocol can read during a
// receive data indication.
//
#define LANCE_MAX_LOOKAHEAD ( 248 - LANCE_HEADER_SIZE )

//
// What a receive indication actually carries after the header: the rest
// of the first receive buffer (LanceProcessReceiveInterrupts indicates
// that buffer only).  This is the true maximum lookahead, whatever
// lookahead a protocol asks for.
//
#define LANCE_INDICATED_LOOKAHEAD(A) \
    ((ULONG)((A)->SizeOfReceiveBuffer - LANCE_HEADER_SIZE))





typedef struct _LANCE_ADAPTER {

    //
    // Holds the interrupt object for this adapter.
    //
    NDIS_MINIPORT_INTERRUPT Interrupt;

    NDIS_MINIPORT_TIMER DeferredTimer;

    USHORT PendingInterruptStatus;

    //
    // Non OS fields of the adapter.
    //

    //
    // Contains Address first byte of adapter memory is mapped to.
    //
    PVOID MmMappedBaseAddr;

    //
    // Amount of memory
    //
    ULONG AmountOfHardwareMemory;

    //
    // Points to the first available address in shared adapter memory.
    //
    PVOID CurrentMemoryFirstFree;

    //
    // Address of the first byte following the memory.
    //
    PVOID MemoryFirstUnavailable;

    //
    // Physical address of base io port address
    //
    ULONG IoBaseAddr;

    //
    // Pointer to the RAP register.
    //
    ULONG RAP;

    //
    // Pointer to the RDP register.
    //
    ULONG RDP;

    //
    // Pointer to the BDP register on PCI PCnet adapters.
    //
    ULONG BDP;

    //
    // Mapped base address of the PCI register range.
    //
    ULONG IoPortBase;

    //
    // PCI PCnet shared-memory pool and bus address.
    //
    BOOLEAN SharedMemoryAllocated;
    BOOLEAN IoPortsRegistered;
    NDIS_PHYSICAL_ADDRESS SharedMemoryPhysicalAddress;

    //
    // Physical base of the 16 MB window that holds the DMA block
    // (LANCE_DMA_WINDOW_SIZE).  Its top byte goes into CSR2.
    //
    ULONG DmaWindowBase;

    //
    // PCI device ID, and the chip ID read from CSR88/CSR89 after reset.
    //
    USHORT PciDeviceId;
    USHORT ChipPart;
    ULONG ChipId;
    const char *ChipName;

    //
    // Address in memory of the network address
    //
    ULONG NetworkHardwareAddress;

    //
    // The network address from the hardware.
    //
    CHAR NetworkAddress[LANCE_LENGTH_OF_ADDRESS];

    //
    // The network address from the hardware.
    //
    CHAR CurrentNetworkAddress[LANCE_LENGTH_OF_ADDRESS];

    //
    // Interrupt number
    //
    UINT InterruptNumber;

    //
    // IRQL
    //
    UINT InterruptRequestLevel;

    //
    // Holds the number of transmit ring entries.
    //
    // NOTE NOTE NOTE
    //
    // There is code that depends on the number of transmit entries
    // being a power of two.
    //
    UINT NumberOfTransmitRings;

    //
    // Holds the number of receive ring entries.
    //
    UINT NumberOfReceiveRings;

    //
    // Holds the size of receive buffers.
    //
    UINT SizeOfReceiveBuffer;

    //
    // Holds number of each buffer size.
    //
    UINT NumberOfSmallBuffers;
    UINT NumberOfMediumBuffers;
    UINT NumberOfLargeBuffers;

    //
    // The log base two of the number of transmit ring entries.
    //
    UINT LogNumberTransmitRings;

    //
    // The log base two of the number of receive ring entries.
    //
    UINT LogNumberReceiveRings;

    //
    // Handle given by NDIS when the MAC registered itself.
    //
    NDIS_HANDLE NdisDriverHandle;

    //
    // Handle given by NDIS when the adapter was registered.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    //
    // Pointer to the LANCE initialization block.
    //
    PLANCE_INITIALIZATION_BLOCK InitBlock;

    //
    // Counter that records the number of transmit rings currently
    // available for allocation.
    //
    UINT NumberOfAvailableRings;

    //
    // Pointer to transmit descriptor ring entry that is the
    // first ring entry available for allocation of transmit
    // buffers.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PLANCE_TRANSMIT_ENTRY AllocateableRing;

    //
    // Pointer to a transmit descriptor ring entry that is the
    // first ring entry that the MAC currently has made available
    // for transmission.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PLANCE_TRANSMIT_ENTRY TransmittingRing;

    //
    // Pointer to the first packet that has been allocated to
    // a transmit packet but has not yet been relinquished to
    // the hardware.  We need this pointer to keep the transmit
    // post processing from running into a packet that has not
    // been transmitted.
    //
    PLANCE_TRANSMIT_ENTRY FirstUncommittedRing;

    //
    // Pointer to an array of structs that map transmit ring entries
    // back to a packet.
    //
    PLANCE_RING_TO_PACKET RingToPacket;

    //
    // Pointer to the transmit ring.
    //
    PLANCE_TRANSMIT_ENTRY TransmitRing;

    //
    // Pointer to the last transmit ring entry.
    //
    PLANCE_TRANSMIT_ENTRY LastTransmitRingEntry;

    //
    // Pointer to the receive ring.
    //
    PLANCE_RECEIVE_ENTRY ReceiveRing;

    //
    // Listheads for the adapters buffers.  If the list
    // head is equal to -1 then there are no free elements
    // on the list.
    //
    // The list heads must only be accessed when the
    // adapter lock is held.
    //
    // Note that the listhead at index 0 will always be -1.
    //
    INT LanceBufferListHeads[4];

    //
    // Pointers to an array of adapter buffer descriptors.
    // The array will actually be threaded together by
    // three free lists.  The lists will be for small,
    // medium and full sized packets.
    //
    PLANCE_BUFFER_DESCRIPTOR LanceBuffers;

    //
    // Did we indicate a packet?  Used to tell if NdisIndicateReceiveComplete
    // should be called.
    //
    BOOLEAN IndicatedAPacket;

    //
    // Pointer to an array of virtual addresses that describe
    // the virtual address of each receive ring descriptor buffer.
    //
    PVOID *ReceiveVAs;

    //
    // Index of the receive ring descriptor that started the
    // last packet not completely received by the hardware.
    //
    UINT CurrentReceiveIndex;

    //
    // Counters to hold the various number of errors/statistics for both
    // reception and transmission.
    //
    // Can only be accessed when the adapter lock is held.
    //
    UINT OutOfReceiveBuffers;
    UINT CRCError;
    UINT FramingError;
    UINT LengthError;       // frames dropped for an impossible length
    UINT RetryFailure;
    UINT LostCarrier;
    UINT LateCollision;
    UINT UnderFlow;
    UINT Deferred;
    UINT OneRetry;
    UINT MoreThanOneRetry;

    //
    // Holds counts of more global errors for the driver.  If we
    // get a memory error then the device needs to be reset.
    //
    UINT MemoryError;
    UINT Babble;
    UINT MissedPacket;

    //
    // Holds other cool counts.
    //

    ULONG Transmit;
    ULONG Receive;

    //
    // Flag that when enabled lets routines know that a reset
    // is in progress.
    //
    BOOLEAN ResetInProgress;

    //
    // Flag that when enabled lets routines know that a reset
    // is in progress and the initialization needs doing.
    //
    BOOLEAN ResetInitStarted;

    //
    // What the reinitialization in progress has to complete when IDON
    // arrives.  These are separate flags rather than one "request type"
    // because a MAC-issued reset (memory error, transmitter stopped) can
    // start while a set-information request is still pending, and that
    // request must still be completed.
    //
    BOOLEAN SetInformationPending;  // NdisMSetInformationComplete owed
    BOOLEAN ResetPending;           // NdisMResetComplete owed

    //
    // A filter or multicast set arrived during a driver-started reset
    // whose INIT had already been issued with the old init block.  The
    // IDON handler runs one more reset to load the new values before it
    // completes the request.
    //
    BOOLEAN ReinitAfterReset;

    //
    // During an indication this is set to the current indications context
    //
    LANCE_RECEIVE_CONTEXT IndicatingMacReceiveContext;


    //
    // Look ahead information.
    //

    ULONG MaxLookAhead;

    //
    // Open information
    //
    UCHAR MaxMulticastList;

    //
    // Will be true the first time that the hardware is initialized
    // by the driver initialization.
    //
    BOOLEAN FirstInitialization;

    //
    // Is the adapter being removed
    //
    BOOLEAN BeingRemoved;

    //
    // Will be true if the hardware fails for some reason
    //
    BOOLEAN HardwareFailure;

    //
    // Set by the PnP event handler when the device has been surprise
    // removed; the hardware must not be touched after that.
    //
    BOOLEAN SurpriseRemoved;

    //
    // Set at the start of LanceHalt.  From then on no reset may start
    // and the deferred timer may not be armed again.
    //
    BOOLEAN Halting;

    //
    // Link (cable) state, polled from BCR4 in LanceCheckForHang.
    // MediaConnected is what has been reported to NDIS; it starts TRUE.
    // A disconnect is only reported once BCR4 has shown the link up at
    // least once (LinkStatusTrusted), so a chip or emulator that doesn't
    // implement the link bit can't leave the adapter stuck offline.
    //
    BOOLEAN MediaConnected;
    BOOLEAN LinkStatusTrusted;
    USHORT Bcr4;

    //
    // Lookahead buffer for loopback packets
    //
    UCHAR Lookahead[LANCE_MAX_LOOKAHEAD + LANCE_HEADER_SIZE];

    UINT PacketFilter;
    UINT NumberOfAddresses;
    UCHAR MulticastAddresses[LANCE_MAX_MULTICAST_LIST][LANCE_LENGTH_OF_ADDRESS];

} LANCE_ADAPTER,*PLANCE_ADAPTER;

//
// Given a MacContextHandle return the PLANCE_ADAPTER
// it represents.
//
#define PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(Handle) \
    ((PLANCE_ADAPTER)(Handle))

//
// Given a pointer to a LANCE_ADAPTER return the
// proper MacContextHandle.
//
#define CONTEXT_HANDLE_FROM_PLANCE_ADAPTER(Ptr) \
    ((NDIS_HANDLE)(Ptr))

//
// This macro will act a "epilogue" to every routine in the
// *interface*.  It will check whether any requests need
// to defer their processing.  It will also decrement the reference
// count on the adapter.  If the reference count is zero and there
// is deferred work to do it will insert the interrupt processing
// routine in the DPC queue.
//
// Note that we don't need to include checking for blocked receives
// since blocked receives imply that there will eventually be an
// interrupt.
//
// NOTE: This macro assumes that it is called with the lock acquired.
//
// ZZZ This routine is NT specific.
//
#define LANCE_DO_DEFERRED(Adapter) \
{ \
    PLANCE_ADAPTER _A = (Adapter); \
    if (_A->ResetInProgress && !_A->Halting) { \
        NdisMSetTimer(&_A->DeferredTimer, 0);\
    } \
}




//
// Procedures which log errors.
//

typedef enum _LANCE_PROC_ID {
    processInterrupt
} LANCE_PROC_ID;


#define LANCE_ERRMSG_NDIS_ALLOC_MEM      (ULONG)0x01














//
// This macro is used to "allocate" memory for the structures that
// must be shared with the hardware.  It assigns a pvoid that is
// at least quadword aligned.
//
#define LANCE_ALLOCATE_MEMORY_FOR_HARDWARE(A,S,P) \
{ \
    PLANCE_ADAPTER _Adapter = (A); \
    UINT _Size = (((S) + 7)/8)*8; \
    PVOID _HighWater; \
    if (!_Size) { \
        *(P) = NULL; \
    } else { \
        _HighWater = ((PCHAR)_Adapter->CurrentMemoryFirstFree) + _Size; \
        if (((PUCHAR)_HighWater) > \
            ((PUCHAR)_Adapter->MemoryFirstUnavailable)) { \
            *(P) = NULL; \
        } else { \
            *(P) = _Adapter->CurrentMemoryFirstFree; \
            _Adapter->CurrentMemoryFirstFree = _HighWater; \
        } \
    } \
}

//
// We now convert the virtual address to the actual physical address.
//
#define LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(A,P) \
      (ULONG)(NdisGetPhysicalAddressLow((A)->SharedMemoryPhysicalAddress) + \
              (ULONG_PTR)(P) - (ULONG_PTR)(A)->MmMappedBaseAddr)

//
// This macro is used to "deallocate the memory from the hardware.
// Since this is hardware memory that is only allocated and deallocated
// once this macro really doesn't do anything.
//
#define LANCE_DEALLOCATE_MEMORY_FOR_HARDWARE(A,P)\
{\
}





#define LANCE_WRITE_RAP(A,C)      { \
    LANCE_ISR_WRITE_RAP(A,C);\
}


#define LANCE_WRITE_RDP(A,C) { \
    LANCE_ISR_WRITE_RDP(A,C);\
}

#define LANCE_READ_RDP(A,C) { \
    LANCE_ISR_READ_RDP(A,C);\
}

BOOLEAN
LanceSyncStopChip(
    IN PVOID Context
    );


//
// We define the external interfaces to the lance driver.
// These routines are only external to permit separate
// compilation.  Given a truely fast compiler they could
// all reside in a single file and be static.
//


NDIS_STATUS
LanceTransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    );

NDIS_STATUS
LanceSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    );

extern
VOID
LanceStagedAllocation(
    IN PLANCE_ADAPTER Adapter
    );

extern
VOID
LanceCopyFromPacketToBuffer(
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    IN UINT BytesToCopy,
    OUT PCHAR Buffer,
    OUT PUINT BytesCopied
    );

extern
VOID
LanceCopyFromPacketToPacket(
    IN PNDIS_PACKET Destination,
    IN UINT DestinationOffset,
    IN UINT BytesToCopy,
    IN PNDIS_PACKET Source,
    IN UINT SourceOffset,
    OUT PUINT BytesCopied
    );

extern
NDIS_STATUS
LanceRegisterAdapter(
    IN PLANCE_ADAPTER Adapter
    );

VOID
SetupAllocate(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet
    );

VOID
LanceDisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
LanceEnableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
LanceHalt(
    IN PVOID MiniportAdapterContext
    );

BOOLEAN
LanceCheckForHang(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
LanceShutdown(
    IN PVOID ShutdownContext
    );

VOID
LancePnPEventNotify(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT PnPEvent,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength
    );

VOID
LanceHandleInterrupt(
    IN NDIS_HANDLE Context
    );

VOID
LanceIsr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context
    );

NDIS_STATUS
LanceInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    );

#endif // _LANCESFT_
