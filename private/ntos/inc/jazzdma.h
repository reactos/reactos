/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    jazzdma.h

Abstract:

    This module is the header file that describes the DMA control register
    structure for the Jazz system.

Author:

    David N. Cutler (davec) 13-Nov-1990

Revision History:

--*/

#ifndef _JAZZDMA_
#define _JAZZDMA_

//
// Define DMA register structure.
//

typedef struct _DMA_REGISTER {
    ULONG Long;
    ULONG Fill;
} DMA_REGISTER, *PDMA_REGISTER;

//
// Define DMA channel register structure.
//

typedef struct _DMA_CHANNEL {
    DMA_REGISTER Mode;
    DMA_REGISTER Enable;
    DMA_REGISTER ByteCount;
    DMA_REGISTER Address;
} DMA_CHANNEL, *PDMA_CHANNEL;

//
// Define DMA control register structure.
//

typedef volatile struct _DMA_REGISTERS {
    DMA_REGISTER Configuration;
    DMA_REGISTER RevisionLevel;
    DMA_REGISTER InvalidAddress;
    DMA_REGISTER TranslationBase;
    DMA_REGISTER TranslationLimit;
    DMA_REGISTER TranslationInvalidate;
    DMA_REGISTER CacheMaintenance;
    DMA_REGISTER RemoteFailedAddress;
    DMA_REGISTER MemoryFailedAddress;
    DMA_REGISTER PhysicalTag;
    DMA_REGISTER LogicalTag;
    DMA_REGISTER ByteMask;
    DMA_REGISTER BufferWindowLow;
    DMA_REGISTER BufferWindowHigh;
    DMA_REGISTER RemoteSpeed[16];
    DMA_REGISTER ParityDiagnosticLow;
    DMA_REGISTER ParityDiagnosticHigh;
    DMA_CHANNEL Channel[8];
    DMA_REGISTER InterruptSource;
    DMA_REGISTER Errortype;
    DMA_REGISTER RefreshRate;
    DMA_REGISTER RefreshCounter;
    DMA_REGISTER SystemSecurity;
    DMA_REGISTER InterruptInterval;
    DMA_REGISTER IntervalTimer;
    DMA_REGISTER InterruptAcknowledge;
} DMA_REGISTERS, *PDMA_REGISTERS;

//
// Define DMA channel mode register structure.
//

typedef struct _DMA_CHANNEL_MODE {
    ULONG AccessTime : 3;
    ULONG TransferWidth : 2;
    ULONG InterruptEnable : 1;
    ULONG BurstMode : 1;
    ULONG FastDmaCycle : 1;
    ULONG Reserved1 : 24;
} DMA_CHANNEL_MODE, *PDMA_CHANNEL_MODE;

//
// Define access time values.
//

#define ACCESS_40NS 0x0                 // 40ns access time
#define ACCESS_80NS 0x1                 // 80ns access time
#define ACCESS_120NS 0x2                // 120ns access time
#define ACCESS_160NS 0x3                // 160ns access time
#define ACCESS_200NS 0x4                // 200ns access time
#define ACCESS_240NS 0x5                // 240ns access time
#define ACCESS_280NS 0x6                // 280ns access time
#define ACCESS_320NS 0x7                // 320ns access time

//
// Define transfer width values.
//

#define WIDTH_8BITS 0x1                 // 8-bit transfer width
#define WIDTH_16BITS 0x2                // 16-bit transfer width
#define WIDTH_32BITS 0x3                // 32-bit transfer width

//
// Define DMA channel enable register structure.
//

typedef struct _DMA_CHANNEL_ENABLE {
    ULONG ChannelEnable : 1;
    ULONG TransferDirection : 1;
    ULONG Reserved1 : 6;
    ULONG TerminalCount : 1;
    ULONG MemoryError : 1;
    ULONG TranslationError : 1;
    ULONG Reserved2 : 21;
} DMA_CHANNEL_ENABLE, *PDMA_CHANNEL_ENABLE;

//
// Define transfer direction values.
//

#define DMA_READ_OP 0x0                 // read from device
#define DMA_WRITE_OP 0x1                // write to device

//
// Define interrupt source register structure.
//

typedef struct _DMA_INTERRUPT_SOURCE {
    ULONG InterruptPending : 8;
    ULONG MemoryParityError : 1;
    ULONG R4000AddressError : 1;
    ULONG IoCacheFlushError : 1;
    ULONG reserved1 : 21;
} DMA_INTERRUPT_SOURCE, *PDMA_INTERRUPT_SOURCE;

//
// Define translation table entry structure.
//

typedef volatile struct _TRANSLATION_ENTRY {
    ULONG PageFrame;
    ULONG Fill;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

#endif // _JAZZDMA_
