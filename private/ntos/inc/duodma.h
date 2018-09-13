/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    duodma.h

Abstract:

    This module is the header file that describes the DMA control register
    structure for the Duo system.

Author:

    David N. Cutler (davec) 13-Nov-1990

Revision History:

--*/

#ifndef _DUODMA_
#define _DUODMA_

//
// Define DMA register structures.
//

typedef struct _DMA_REGISTER {
    ULONG Long;
    ULONG Fill;
} DMA_REGISTER, *PDMA_REGISTER;

typedef struct _DMA_LARGE_REGISTER {
    union {
        LARGE_INTEGER LargeInteger;
        double Double;
    } u;
} DMA_LARGE_REGISTER, *PDMA_LARGE_REGISTER;

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
    DMA_REGISTER RemoteFailedAddress;
    DMA_REGISTER MemoryFailedAddress;
    DMA_REGISTER InvalidAddress;
    DMA_REGISTER TranslationBase;
    DMA_REGISTER TranslationLimit;
    DMA_REGISTER TranslationInvalidate;
    DMA_REGISTER ChannelInterruptAcknowledge;
    DMA_REGISTER LocalInterruptAcknowledge;
    DMA_REGISTER EisaInterruptAcknowledge;
    DMA_REGISTER TimerInterruptAcknowledge;
    DMA_REGISTER IpInterruptAcknowledge;
    DMA_REGISTER Reserved1;
    DMA_REGISTER WhoAmI;
    DMA_REGISTER NmiSource;
    DMA_REGISTER RemoteSpeed[15];
    DMA_REGISTER InterruptEnable;
    DMA_CHANNEL Channel[4];
    DMA_REGISTER ArbitrationControl;
    DMA_REGISTER Errortype;
    DMA_REGISTER RefreshRate;
    DMA_REGISTER RefreshCounter;
    DMA_REGISTER SystemSecurity;
    DMA_REGISTER InterruptInterval;
    DMA_REGISTER IntervalTimer;
    DMA_REGISTER IpInterruptRequest;
    DMA_REGISTER InterruptDiagnostic;
    DMA_LARGE_REGISTER EccDiagnostic;
    DMA_REGISTER MemoryConfig[4];
    DMA_REGISTER Reserved2;
    DMA_REGISTER Reserved3;
    DMA_LARGE_REGISTER IoCacheBuffer[64];
    DMA_REGISTER IoCachePhysicalTag[8];
    DMA_REGISTER IoCacheLogicalTag[8];
    DMA_REGISTER IoCacheLowByteMask[8];
    DMA_REGISTER IoCacheHighByteMask[8];
} DMA_REGISTERS, *PDMA_REGISTERS;

//
// Configuration Register values.
//

#define LOAD_CLEAN_EXCLUSIVE 0x20
#define DISABLE_EISA_MEMORY 0x10
#define ENABLE_PROCESSOR_B 0x08
#define MAP_PROM 0x04

//
// Interrupt Enable bits.
//
#define ENABLE_CHANNEL_INTERRUPTS (1 << 0)
#define ENABLE_DEVICE_INTERRUPTS (1 << 1)
#define ENABLE_EISA_INTERRUPTS (1 << 2)
#define ENABLE_TIMER_INTERRUPTS (1 << 3)
#define ENABLE_IP_INTERRUPTS (1 << 4)

//
// Eisa Interupt Acknowledge Register values.
//

#define EISA_NMI_VECTOR 0x8000

//
// DMA_NMI_SRC register bit definitions.
//

#define NMI_SRC_MEMORY_ERROR        1
#define NMI_SRC_R4000_ADDRESS_ERROR 2
#define NMI_SRC_IO_CACHE_ERROR      4
#define NMI_SRC_ADR_NMI             8

//
// Define DMA channel mode register structure.
//

typedef struct _DMA_CHANNEL_MODE {
    ULONG AccessTime : 3;
    ULONG TransferWidth : 2;
    ULONG InterruptEnable : 1;
    ULONG BurstMode : 1;
    ULONG Reserved1 : 25;
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
// Define translation table entry structure.
//

typedef volatile struct _TRANSLATION_ENTRY {
    ULONG PageFrame;
    ULONG Fill;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
// Error Type Register values
//

#define SONIC_ADDRESS_ERROR 4
#define SONIC_MEMORY_ERROR 0x40
#define EISA_ADDRESS_ERROR 1
#define EISA_MEMORY_ERROR 2

//
// Address Mask definitions.
//

#define LFAR_ADDRESS_MASK 0xfffff000
#define RFAR_ADDRESS_MASK 0x00ffffc0
#define MFAR_ADDRESS_MASK 0x1ffffff0

//
// ECC Register Definitions.
//

#define ECC_SINGLE_BIT_DP0 0x02000000
#define ECC_SINGLE_BIT_DP1 0x20000000
#define ECC_SINGLE_BIT ( ECC_SINGLE_BIT_DP0 | ECC_SINGLE_BIT_DP1 )
#define ECC_DOUBLE_BIT_DP0 0x04000000
#define ECC_DOUBLE_BIT_DP1 0x40000000
#define ECC_DOUBLE_BIT ( ECC_DOUBLE_BIT_DP0 | ECC_DOUBLE_BIT_DP1 )
#define ECC_MULTIPLE_BIT_DP0 0x08000000
#define ECC_MULTIPLE_BIT_DP1 0x80000000

#define ECC_FORCE_DP0 0x010000
#define ECC_FORCE_DP1 0x100000
#define ECC_DISABLE_SINGLE_DP0 0x020000
#define ECC_DISABLE_SINGLE_DP1 0x200000
#define ECC_ENABLE_DP0 0x040000
#define ECC_ENABLE_DP1 0x400000

//
// LED/DIAG Register Definitions.
//

#define DIAG_NMI_SWITCH 2

//
// Common error bit definitions
//

#define SINGLE_ERROR   1
#define MULTIPLE_ERROR 2
#define RFAR_CACHE_FLUSH 4

#endif // _DUODMA_
