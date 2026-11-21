/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lance.c

Abstract:

    The main program for a LANCE (Local Area Network Controller
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

#ifndef _LANCEHARDWARE_
#define _LANCEHARDWARE_

//
// All registers on the LANCE are 16 bits.
//

#define LANCE_SELECT_CSR0 ((USHORT)0)
#define LANCE_SELECT_CSR1 ((USHORT)1)
#define LANCE_SELECT_CSR2 ((USHORT)2)
#define LANCE_SELECT_CSR4  ((USHORT)4)
#define LANCE_SELECT_CSR88 ((USHORT)88)
#define LANCE_SELECT_CSR89 ((USHORT)89)
#define LANCE_SELECT_BCR4  ((USHORT)4)
#define LANCE_SELECT_BCR18 ((USHORT)18)
#define LANCE_SELECT_BCR20 ((USHORT)20)

//
// BCR4 (LED0 control, link status by default: 0x00C0 after reset).
// LEDOUT shows the OR of the enabled LED sources.  With only LNKSTE
// enabled among bits 6:0 it is the link status.  LEDPOL inverts the
// pin, so the driver doesn't use BCR4 for link status when it's set.
//
#define LANCE_BCR4_LEDOUT       ((USHORT)0x8000)
#define LANCE_BCR4_LEDPOL       ((USHORT)0x4000)
#define LANCE_BCR4_LNKSTE       ((USHORT)0x0040)
#define LANCE_BCR4_SOURCES_6_0  ((USHORT)0x007f)

//
// CSR4 (test and features control).  The xxxM bits mask the matching
// interrupt sources; all four are set after reset (0x0115).
//
#define LANCE_CSR4_DMAPLUS  ((USHORT)0x4000)
#define LANCE_CSR4_MFCOM    ((USHORT)0x0100)
#define LANCE_CSR4_RCVCCOM  ((USHORT)0x0010)
#define LANCE_CSR4_TXSTRTM  ((USHORT)0x0004)
#define LANCE_CSR4_JABM     ((USHORT)0x0001)

//
// BCR18 (burst and bus control).
//
#define LANCE_BCR18_DWIO    ((USHORT)0x0080)
#define LANCE_BCR18_BREADE  ((USHORT)0x0040)
#define LANCE_BCR18_BWRITE  ((USHORT)0x0020)

//
// Every PCnet-PCI/FAST part enumerates as 1022:2000; the Am79C978
// HomePNA function enumerates as 1022:2001.  The individual chip is
// identified by the part number in CSR88/CSR89, not by the PCI ID.
//
#define PCNET_PCI_VENDOR_ID         ((USHORT)0x1022)
#define PCNET_PCI_DEVICE_ID         ((USHORT)0x2000)
#define PCNET_HOME_PCI_DEVICE_ID    ((USHORT)0x2001)

#define PCNET_CHIP_PART(ChipId)     ((USHORT)(((ChipId) >> 12) & 0xffff))

#define PCNET_PART_79C970           ((USHORT)0x2420)
#define PCNET_PART_79C970_ALT       ((USHORT)0x2430)
#define PCNET_PART_79C970A          ((USHORT)0x2621)
#define PCNET_PART_79C971           ((USHORT)0x2623)
#define PCNET_PART_79C972           ((USHORT)0x2624)
#define PCNET_PART_79C973           ((USHORT)0x2625)
#define PCNET_PART_79C978           ((USHORT)0x2626)
#define PCNET_PART_79C975           ((USHORT)0x2627)

#define LANCE_CSR0_INITIALIZE             ((USHORT)(0x0001))
#define LANCE_CSR0_START                  ((USHORT)(0x0002))
#define LANCE_CSR0_STOP                   ((USHORT)(0x0004))
#define LANCE_CSR0_TRANSMIT_DEMAND        ((USHORT)(0x0008))
#define LANCE_CSR0_TRANSMITTER_ON         ((USHORT)(0x0010))
#define LANCE_CSR0_RECEIVER_ON            ((USHORT)(0x0020))
#define LANCE_CSR0_INTERRUPT_ENABLE       ((USHORT)(0x0040))
#define LANCE_CSR0_INTERRUPT_FLAG         ((USHORT)(0x0080))
#define LANCE_CSR0_INITIALIZATION_DONE    ((USHORT)(0x0100))
#define LANCE_CSR0_TRANSMITTER_INTERRUPT  ((USHORT)(0x0200))
#define LANCE_CSR0_RECEIVER_INTERRUPT     ((USHORT)(0x0400))
#define LANCE_CSR0_MEMORY_ERROR           ((USHORT)(0x0800))
#define LANCE_CSR0_MISSED_PACKET          ((USHORT)(0x1000))
#define LANCE_CSR0_COLLISION_ERROR        ((USHORT)(0x2000))
#define LANCE_CSR0_BABBLE                 ((USHORT)(0x4000))
#define LANCE_CSR0_ERROR_SUMMARY          ((USHORT)(0x8000))
#define LANCE_CSR0_ERROR_BITS             ((USHORT)(0xf800))


#define LANCE_NUMBER_OF_TRANSMIT_RINGS ((UINT)64)
#define LANCE_LOG_TRANSMIT_RINGS ((UINT)6)

#define LANCE_64K_NUMBER_OF_RECEIVE_RINGS ((UINT)64)
#define LANCE_64K_LOG_RECEIVE_RINGS ((UINT)6)

#define LANCE_64K_SIZE_OF_RECEIVE_BUFFERS ((UINT)512)

#define LANCE_SIZE_OF_RECEIVE_BUFFERS ((UINT)256)
#define LANCE_LOOPBACK_SIZE_OF_RECEIVE_BUFFERS  LANCE_SIZE_OF_RECEIVE_BUFFERS


#define LANCE_SMALL_BUFFER_SIZE ((UINT)64)
#define LANCE_MEDIUM_BUFFER_SIZE ((UINT)256)
#define LANCE_LARGE_BUFFER_SIZE ((UINT)1514)

#define LANCE_64K_NUMBER_OF_SMALL_BUFFERS ((UINT)10)
#define LANCE_64K_NUMBER_OF_MEDIUM_BUFFERS ((UINT)10)
#define LANCE_64K_NUMBER_OF_LARGE_BUFFERS ((UINT)16)

#define LANCE_ISR_WRITE_RAP(A,C)  NdisRawWritePortUshort((ULONG)((A)->RAP),C)
#define LANCE_ISR_READ_RDP(A,C)   NdisRawReadPortUshort((ULONG)((A)->RDP),C)
#define LANCE_ISR_WRITE_RDP(A,C)   NdisRawWritePortUshort((ULONG)((A)->RDP),C)


#define LANCE_GET_LOW_PART_ADDRESS(Adr) \
    ((USHORT)((Adr) & 0xffff))

#define LANCE_GET_HIGH_PART_ADDRESS(Adr) \
    ((UCHAR)(((ULONG)(Adr) >> 16) & 0xff))

//
// In 16-bit software style (SWSTYLE 0, SSIZE32 0) the init block and the
// descriptors only hold 24-bit addresses.  The PCnet-PCI chips take bits
// 31:24 of every bus-master address (init block, rings and buffers) from
// CSR2[15:8], IADR[31:24].  So the DMA memory does not have to be below
// 16 MB; it has to sit in one 16 MB-aligned window below 4 GB, and CSR2
// has to carry that window's top byte.
//
#define LANCE_DMA_WINDOW_SIZE ((ULONG)0x01000000)
#define LANCE_DMA_WINDOW_MASK ((ULONG)0xff000000)

//
// The full value for CSR2: IADR[31:24] in bits 15:8 and IADR[23:16] in
// bits 7:0.
//
#define LANCE_GET_CSR2_ADDRESS(Adr) \
    ((USHORT)(((ULONG)(Adr) >> 16) & 0xffff))

typedef struct _LANCE_INITIALIZATION_BLOCK {

    USHORT ModeRegister;
    UCHAR PhysicalAddress[6];
    UCHAR LogicalAddressFilter[8];
    USHORT LowReceiveRingAddress;
    UCHAR HighReceiveRingAddress;
    UCHAR ReceiveLengthLow5BitsReserved;
    USHORT LowTransmitRingAddress;
    UCHAR HighTransmitRingAddress;
    UCHAR TransmitLengthLow5BitsReserved;

} LANCE_INITIALIZATION_BLOCK,*PLANCE_INITIALIZATION_BLOCK;

//
// Define masks to access bits in the mode register of the initialization
// block.
//
#define LANCE_MODE_PROMISCUOUS ((USHORT)(0x8000))

//
// Defines for moving to/from shared memory.
//


#define LANCE_ZERO_MEMORY_FOR_HARDWARE(Destination,Length) \
    NdisZeroMappedMemory(Destination,Length)

#define LANCE_MOVE_STRUCT_TO_HARDWARE(Destination,Source) \
    NdisMoveToMappedMemory(&(Destination), &(Source), sizeof(Source))

#define LANCE_MOVE_MEMORY_TO_HARDWARE(Destination,Source,Length) \
    NdisMoveToMappedMemory(Destination, Source, Length)

#define LANCE_MOVE_HARDWARE_TO_MEMORY(Destination,Source,Length) \
    NdisMoveFromMappedMemory(Destination, Source, Length)

#define LANCE_WRITE_HARDWARE_LOW_PART_ADDRESS(Destination, Source) \
    NdisWriteRegisterUshort((PUSHORT)(&Destination), (USHORT)(Source))

#define LANCE_WRITE_HARDWARE_HIGH_PART_ADDRESS(Destination, Source) \
    NdisWriteRegisterUchar((PUCHAR)(&Destination), (UCHAR)(Source))

#define LANCE_WRITE_HARDWARE_MEMORY_UCHAR(Destination, Source) \
    NdisWriteRegisterUchar((PUCHAR)(&Destination), (UCHAR)(Source))

#define LANCE_WRITE_HARDWARE_MEMORY_USHORT(Destination, Source) \
    NdisWriteRegisterUshort((PUSHORT)(&Destination), (USHORT)(Source))

#define LANCE_READ_HARDWARE_MEMORY_UCHAR(Source, Destination) \
    NdisReadRegisterUchar((PUCHAR)(&Source), Destination)

#define LANCE_READ_HARDWARE_MEMORY_USHORT(Source, Destination) \
    NdisReadRegisterUshort((PUSHORT)(&Source), Destination)

#if defined(_ALPHA_)

#define LANCE_SET_RING_BITS(Destination, Data) \
{ \
    UCHAR Tmp; \
    LANCE_READ_HARDWARE_MEMORY_UCHAR(Destination, &Tmp); \
    LANCE_WRITE_HARDWARE_MEMORY_UCHAR(Destination, Tmp | Data); \
}

#else

#define LANCE_SET_RING_BITS(Destination, Data)   (Destination) |= ((Data))

#endif // _ALPHA_

typedef struct _LANCE_RECEIVE_ENTRY {

    //
    // 24 bit pointer to the buffer for the receive
    // data.  This is written by the host and unchanged
    // by the LANCE.
    //
    USHORT LowReceiveBufferAddress;
    UCHAR HighReceiveBufferAddress;

    //
    // This char field contains numerous bits describing
    // the errors that can occur in the packet as well as
    // whether this is the first and/or last buffer in the packet.
    //
    UCHAR ReceiveSummaryBits;

    //
    // This is the twos compliment of the buffer length.
    //
    // NOTE: The high order 4 bits must be enabled.
    //
    USHORT BufferByteCount;

    //
    // This is the length of the data in the packet.
    //
    // Note that the high order 4 bits are undefined.
    //
    USHORT MessageLength;

} LANCE_RECEIVE_ENTRY,*PLANCE_RECEIVE_ENTRY;

//
// A number of macros that make accessing the various bits of the receive
// ring entry a little easier as well as providing some validity checks.
//

//
// Used to set the address of the receive buffer.
//
// Rd is a pointer to a receive descriptor.
//
// Adr is a *physical* address.
//
//
#define LANCE_SET_RECEIVE_BUFFER_ADDRESS(Adptr,Rd,Adr) \
{ \
    PVOID _Adr = (Adr); \
    PLANCE_ADAPTER _Adptr = (Adptr);\
    PLANCE_RECEIVE_ENTRY _Rd = (Rd); \
    ULONG _Offset;\
    _Offset = LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(_Adptr, _Adr);\
    LANCE_WRITE_HARDWARE_LOW_PART_ADDRESS(\
        _Rd->LowReceiveBufferAddress,\
        (USHORT)((ULONG)_Offset) & 0xffff); \
    LANCE_WRITE_HARDWARE_HIGH_PART_ADDRESS(\
        _Rd->HighReceiveBufferAddress,\
        (UCHAR)(((ULONG)_Offset) >> 16) & 0xff); \
}


//
// Used to set the length of the receive buffer.  The stored value
// is actually the twos compliment of the length.  Note that
// the twos complement of this value must have the high order 4 bits
// enabled.
//
// Rd is a pointer to a receive descriptor.
//
// Len is the unsigned short length of the buffer.
//
#define LANCE_SET_RECEIVE_BUFFER_LENGTH(Rd,Len) \
    LANCE_WRITE_HARDWARE_MEMORY_USHORT(\
        Rd->BufferByteCount,\
        (USHORT)((~Len)+1)\
        )

//
// Masks for the summary bits in the receive descriptor.
//
#define LANCE_RECEIVE_END_OF_PACKET   ((UCHAR)0x01)
#define LANCE_RECEIVE_START_OF_PACKET ((UCHAR)0x02)
#define LANCE_RECEIVE_BUFFER_ERROR    ((UCHAR)0x04)
#define LANCE_RECEIVE_CRC_ERROR       ((UCHAR)0x08)
#define LANCE_RECEIVE_OVERFLOW_ERROR  ((UCHAR)0x10)
#define LANCE_RECEIVE_FRAMING_ERROR   ((UCHAR)0x20)
#define LANCE_RECEIVE_ERROR_SUMMARY   ((UCHAR)0x40)
#define LANCE_RECEIVE_OWNED_BY_CHIP   ((UCHAR)0x80)

//
// This macro gets the packet message length from what is
// assumed to be the last buffer in a packet.  Note that
// on the lance the length of the data includes the four
// byte CRC so we must subtract four from length in the
// ring entry.
//
// Rd is a pointer to a receive descriptor.
// Value is the place to store the result.
//
#if defined(_ALPHA_)

#define LANCE_GET_MESSAGE_SIZE(Rd, Value) \
{ \
    NdisReadRegisterUshort(                                                   \
              ((ULONG)Rd + FIELD_OFFSET(LANCE_RECEIVE_ENTRY, MessageLength)), \
              &Value                                                          \
              );                                                              \
    Value = (Value & ((USHORT)0x0fff)) -4;                                    \
}


#else

#define LANCE_GET_MESSAGE_SIZE(Rd, Value) \
   Value = (((Rd->MessageLength) & ((USHORT)0x0fff))-4)

#endif // _ALPHA_

typedef struct _LANCE_TRANSMIT_ENTRY {

    //
    // 24 bit pointer to the transmit buffer.  This is
    // written by the host and unchanged by the LANCE.
    //
    USHORT LowTransmitBufferAddress;
    UCHAR HighTransmitBufferAddress;

    //
    // This field contains summary information about the packet.
    //
    UCHAR TransmitSummaryBits;

    //
    // This field contains the "twos complement" of the length
    // of the buffer.
    //
    // NOTE: The high order four bits must be enabled.
    //
    USHORT BufferByteCount;

    //
    // This short contains the error summary information for the
    // ring entry.
    //
    USHORT ErrorSummaryInfo;

} LANCE_TRANSMIT_ENTRY,*PLANCE_TRANSMIT_ENTRY;

//
// Masks for the normal summary bits in the transmit descriptor.
//
#define LANCE_TRANSMIT_END_OF_PACKET       ((UCHAR)(0x01))
#define LANCE_TRANSMIT_START_OF_PACKET     ((UCHAR)(0x02))
#define LANCE_TRANSMIT_DEFERRED            ((UCHAR)(0x04))
#define LANCE_TRANSMIT_ONE_RETRY           ((UCHAR)(0x08))
#define LANCE_TRANSMIT_MORE_THAN_ONE_RETRY ((UCHAR)(0x10))
#define LANCE_TRANSMIT_ANY_ERRORS          ((UCHAR)(0x40))
#define LANCE_TRANSMIT_OWNED_BY_CHIP       ((UCHAR)(0x80))

//
// Set of masks to recover particular errors that a transmit can encounter.
//
#define LANCE_TRANSMIT_TDR            ((USHORT)(0x03ff))
#define LANCE_TRANSMIT_RETRY          ((USHORT)(0x0400))
#define LANCE_TRANSMIT_LOST_CARRIER   ((USHORT)(0x0800))
#define LANCE_TRANSMIT_LATE_COLLISION ((USHORT)(0x1000))
#define LANCE_TRANSMIT_UNDERFLOW      ((USHORT)(0x4000))
#define LANCE_TRANSMIT_BUFFER         ((USHORT)(0x8000))

//
// Used to set the address of the transmit buffer.
//
// Rd is a pointer to a transmit descriptor.
//
// Adr is a *physical* address.
//
//

#define LANCE_SET_TRANSMIT_BUFFER_ADDRESS(Adptr,Td,Adr) \
{ \
    PVOID _Adr = (Adr); \
    PLANCE_ADAPTER _Adptr = (Adptr);\
    PLANCE_TRANSMIT_ENTRY _Td = (Td); \
    ULONG _Offset;\
    _Offset = LANCE_GET_HARDWARE_PHYSICAL_ADDRESS(_Adptr, _Adr);\
    LANCE_WRITE_HARDWARE_LOW_PART_ADDRESS(\
        _Td->LowTransmitBufferAddress,\
        (USHORT)(((ULONG)_Offset) & 0xffff)\
        );\
    LANCE_WRITE_HARDWARE_HIGH_PART_ADDRESS(\
        _Td->HighTransmitBufferAddress,\
        (UCHAR)((((ULONG)_Offset) >> 16) & 0xff)\
        );\
}


//
// Used to set the length of the transmit buffer.  The stored value
// is actually the twos compliment of the length.  Note that
// the twos complement of this value must have the high order 4 bits
// enabled.
//
// Td is a pointer to a transmit descriptor.
//
// Len is the unsigned short length of the buffer.
//
#define LANCE_SET_TRANSMIT_BUFFER_LENGTH(Td,Len) \
    LANCE_WRITE_HARDWARE_MEMORY_USHORT(\
        Td->BufferByteCount,\
        (USHORT)((~Len)+1)\
        )

#endif // _LANCEHARDWARE_
