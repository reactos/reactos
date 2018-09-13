/*++

Copyright (c) 1994  NEC Corporation
Copyright (c) 1994  NEC Software, Ltd.

Module Name:

    nec98.h (cf. eisa.h)

Abstract:

    The module defines the structures, and defines  for the NEC PC98 chip set.

Author:

    Michio Nakamura        20-Sep-1994

Revision History:

    Takaaki Tochizawa      13-Mar-1998 Add 2nd DMA for FIR.

--*/

#ifndef _EISA_
#define _EISA_

//
// Define the DMA page register structure.
//

#define DMA_BANK_A31_A24_DR0   0xe05
#define DMA_BANK_A31_A24_DR1   0xe07
#define DMA_BANK_A31_A24_DR2   0xe09
#define DMA_BANK_A31_A24_DR3   0xe0b
#define DMA_INC_ENABLE_A31_A24 0xe0f

//
// Define the DMA 2 page register structure.
//

#define DMA2_BANK_A31_A24_DR5   0xf07
#define DMA2_BANK_A31_A24_DR6   0xf09
#define DMA2_BANK_A31_A24_DR7   0xf0b
#define DMA2_INC_ENABLE_A31_A24 0xf0f

//
// Define the DMA 2 mode change register
//
#define DMA2_MODE_CHANGE      0xf4
#define DMA2_MODE_8237_COMP   0x0
#define DMA2_MODE_71037_A     0x1
#define DMA2_MODE_71037_B     0x2
#define DMA2_MODE_71037_C     0x3

#define DMA_STATUS 0xc8
#define DMA_COMMAND 0xc8
#define SINGLE_MASK 0xca
#define MODE 0xcb
#define CLEAR_BYTE_POINTER 0xcc
#define CLEAR_MASK 0xce

typedef struct _DMA_PAGE{
    UCHAR Reserved1;            // offset 0x20
    UCHAR Channel1;             // offset 0x21
    UCHAR Reserved2;
    UCHAR Channel2;             // offset 0x23
    UCHAR Reserved3;
    UCHAR Channel3;             // offset 0x25
    UCHAR Reserved4;
    UCHAR Channel0;             // offset 0x27
    UCHAR Reserved5[0x120-0x27];// offset 0x120
    UCHAR Channel5;             // offset 0x121
    UCHAR Reserved6;
    UCHAR Channel6;             // offset 0x123
    UCHAR Reserved7;
    UCHAR Channel7;             // offset 0x125
    UCHAR Reserved8[4];
}DMA_PAGE, *PDMA_PAGE;

//
// Define the DMA stop register structure.
//

typedef struct _DMA_CHANNEL_STOP {
    UCHAR ChannelLsb;
    UCHAR ChannelMsb;
    UCHAR ChannelHsb;
    UCHAR Reserved;
}DMA_CHANNEL_STOP, *PDMA_CHANNEL_STOP;

//
// Define DMA 1 address and count structure.
//

typedef struct _DMA1_ADDRESS_COUNT {
    UCHAR Reserved1;
    UCHAR DmaBaseAddress;
    UCHAR Reserved2;
    UCHAR DmaBaseCount;
}DMA1_ADDRESS_COUNT, *PDMA1_ADDRESS_COUNT;


//
// Define DMA 2 address and count structure.
//

typedef struct _DMA2_ADDRESS_COUNT {
    UCHAR Reserved1;
    UCHAR DmaBaseAddress;
    UCHAR Reserved2;
    UCHAR DmaBaseCount;
}DMA2_ADDRESS_COUNT, *PDMA2_ADDRESS_COUNT;

//
// Define DMA 1 control register structure.
//

typedef struct _DMA1_CONTROL {
    DMA1_ADDRESS_COUNT DmaAddressCount[4];
    UCHAR Reserved1;
    UCHAR DmaStatus;            //offset 0x11
    UCHAR Reserved2;
    UCHAR DmaRequest;           //offset 0x13
    UCHAR Reserved3;
    UCHAR SingleMask;           //offset 0x15
    UCHAR Reserved4;
    UCHAR Mode;                 //offset 0x17
    UCHAR Reserved5;
    UCHAR ClearBytePointer;     //offset 0x19
    UCHAR Reserved6;
    UCHAR MasterClear;          //offset 0x1b
    UCHAR Reserved7;
    UCHAR ClearMask;            //offset 0x1d
    UCHAR Reserved;
    UCHAR AllMask;              //offset 0x1f
}DMA1_CONTROL, *PDMA1_CONTROL;

//
// Define DMA 2 control register structure.
//

typedef struct _DMA2_CONTROL {
    UCHAR Reserved8[0x100-0x20];//offset 0x20
    DMA2_ADDRESS_COUNT DmaAddressCount[4]; //offset 0x100
    UCHAR Reserved1;
    UCHAR DmaStatus;            //offset 0x111
    UCHAR Reserved2;
    UCHAR DmaRequest;           //offset 0x113
    UCHAR Reserved3;
    UCHAR SingleMask;           //offset 0x115
    UCHAR Reserved4;
    UCHAR Mode;                 //offset 0x117
    UCHAR Reserved5;
    UCHAR ClearBytePointer;     //offset 0x119
    UCHAR Reserved6;
    UCHAR MasterClear;          //offset 0x11b
    UCHAR Reserved7;
    UCHAR ClearMask;            //offset 0x11d
    UCHAR Reserved;
    UCHAR AllMask;              //offset 0x11f
    UCHAR Reserved9[10];        //offset 0x120
}DMA2_CONTROL, *PDMA2_CONTROL;

//
// Define Timer control register structure.
//

typedef struct _TIMER_CONTROL {
    UCHAR BcdMode : 1;
    UCHAR Mode : 3;
    UCHAR SelectByte : 2;
    UCHAR SelectCounter : 2;
}TIMER_CONTROL, *PTIMER_CONTROL;

//
// Define Timer status register structure.
//

typedef struct _TIMER_STATUS {
    UCHAR BcdMode : 1;
    UCHAR Mode : 3;
    UCHAR SelectByte : 2;
    UCHAR CrContentsMoved : 1;
    UCHAR OutPin : 1;
}TIMER_STATUS, *PTIMER_STATUS;

//
// Define Mode values.
//

#define TM_SIGNAL_END_OF_COUNT  0
#define TM_ONE_SHOT             1
#define TM_RATE_GENERATOR       2
#define TM_SQUARE_WAVE          3
#define TM_SOFTWARE_STROBE      4
#define TM_HARDWARE_STROBE      5

//
// Define SelectByte values
//

#define SB_COUNTER_LATCH        0
#define SB_LSB_BYTE             1
#define SB_MSB_BYTE             2
#define SB_LSB_THEN_MSB         3

//
// Define SelectCounter values.
//

#define SELECT_COUNTER_0        0
#define SELECT_COUNTER_1        1
#define SELECT_COUNTER_2        2
#define SELECT_READ_BACK        3

//
// Define Timer clock for speaker.
//

#define TIMER_CLOCK_IN  1193167     // 1.193Mhz

//
// Define NMI Status/Control register structure.
//

typedef struct _NMI_STATUS {
    UCHAR SpeakerGate : 1;
    UCHAR SpeakerData : 1;
    UCHAR DisableEisaParity : 1;
    UCHAR DisableNmi : 1;
    UCHAR RefreshToggle : 1;
    UCHAR SpeakerTimer : 1;
    UCHAR IochkNmi : 1;
    UCHAR ParityNmi : 1;
}NMI_STATUS, *PNMI_STATUS;

//
// Define NMI Enable register structure.
//

typedef struct _NMI_ENABLE {
   UCHAR RtClockAddress : 7;
   UCHAR NmiDisable : 1;
}NMI_ENABLE, *PNMI_ENABLE;
//
// Define the NMI extended status and control register structure.
//

typedef struct _NMI_EXTENDED_CONTROL {
    UCHAR BusReset : 1;
    UCHAR EnableNmiPort : 1;
    UCHAR EnableFailSafeNmi : 1;
    UCHAR EnableBusMasterTimeout : 1;
    UCHAR Reserved1 : 1;
    UCHAR PendingPortNmi : 1;
    UCHAR PendingBusMasterTimeout : 1;
    UCHAR PendingFailSafeNmi : 1;
}NMI_EXTENDED_CONTROL, *PNMI_EXTENDED_CONTROL;

//
// Define 82357 register structure.
//

typedef struct _EISA_CONTROL {
    union   {
        DMA1_CONTROL Dma1BasePort;          // Offset 0x00
        struct  {
            UCHAR Interrupt1ControlPort0;   // Offset 0x00
            UCHAR Reserved1;
            UCHAR Interrupt1ControlPort1;   // Offset 0x02
            UCHAR Reserved2[5];
            UCHAR Interrupt2ControlPort0;   // Offset 0x08
            UCHAR Reserved3;
            UCHAR Interrupt2ControlPort1;   // Offset 0x0A
            UCHAR Reserved4[sizeof(DMA1_CONTROL)-11];

        };
    };
    union {
        DMA_PAGE DmaPageLowPort;                    // Offset 0x20
        DMA2_CONTROL Dma2BasePort;                  // Offset 0x20
        struct {
            UCHAR Reserved20[9];                    // Offset 0x20
            UCHAR PageIncrementMode;                // Offset 0x29
            UCHAR Reserved21;
            UCHAR InDirectAddress;                  // Offset 0x2b
            UCHAR Reserved22;
            UCHAR InDirectData;                     // Offset 0x2d
            UCHAR Reserved23[0x7f - 0x2e];
            UCHAR PageIncrementMode2;               // Offset 0x7f
            UCHAR Reserved24[0x129 - 0x80];
            UCHAR DMA2PageIncrementMode;            // Offset 0x129
        };
    };
    UCHAR Reserved25[0xfffc - 0x130];               // Offset 0x130
    //
    // No NEC PC98 have 2nd DMA controller. But PC/AT has one. Therefore there are some valuable
    // refer to 2nd DMA in ixisasup.c.
    // I add it following valuable so that HAL builds. HAL of NEC PC98 doesn't use it.
    //
    UCHAR Dma1ExtendedModePort;
    UCHAR Dma2ExtendedModePort;
    UCHAR DmaPageHighPort;
    UCHAR Interrupt1EdgeLevel;
    UCHAR Interrupt2EdgeLevel;

} EISA_CONTROL, *PEISA_CONTROL;

//
// Define initialization command word 1 structure.
//

typedef struct _INITIALIZATION_COMMAND_1 {
    UCHAR Icw4Needed : 1;
    UCHAR CascadeMode : 1;
    UCHAR Unused1 : 2;
    UCHAR InitializationFlag : 1;
    UCHAR Unused2 : 3;
}INITIALIZATION_COMMAND_1, *PINITIALIZATION_COMMAND_1;

//
// Define initialization command word 4 structure.
//

typedef struct _INITIALIZATION_COMMAND_4 {
    UCHAR I80x86Mode : 1;
    UCHAR AutoEndOfInterruptMode : 1;
    UCHAR Unused1 : 2;
    UCHAR SpecialFullyNested : 1;
    UCHAR Unused2 : 3;
}INITIALIZATION_COMMAND_4, *PINITIALIZATION_COMMAND_4;

//
// Define EISA interrupt controller operational command values.
// Define operation control word 2 commands.
//

#define NONSPECIFIC_END_OF_INTERRUPT 0x20
#define SPECIFIC_END_OF_INTERRUPT    0x60

//
// Define external EISA interupts
//

#define EISA_EXTERNAL_INTERRUPTS_1  0xf8
#define EISA_EXTERNAL_INTERRUPTS_2  0xbe

//
// Define the DMA mode register structure.
//

typedef struct _DMA_EISA_MODE {
    UCHAR Channel : 2;
    UCHAR TransferType : 2;
    UCHAR AutoInitialize : 1;
    UCHAR AddressDecrement : 1;
    UCHAR RequestMode : 2;
}DMA_EISA_MODE, *PDMA_EISA_MODE;

//
// Define TransferType values.
//

#define VERIFY_TRANSFER     0x00
#define READ_TRANSFER       0x01        // Read from the device.
#define WRITE_TRANSFER      0x02        // Write to the device.

//
// Define RequestMode values.
//

#define DEMAND_REQUEST_MODE         0x00
#define SINGLE_REQUEST_MODE         0x01
#define BLOCK_REQUEST_MODE          0x02
#define CASCADE_REQUEST_MODE        0x03

//
// Define the DMA extended mode register structure.
//

typedef struct _DMA_EXTENDED_MODE {
    UCHAR ChannelNumber : 2;
    UCHAR TransferSize : 2;
    UCHAR TimingMode : 2;
    UCHAR EndOfPacketInput : 1;
    UCHAR StopRegisterEnabled : 1;
}DMA_EXTENDED_MODE, *PDMA_EXTENDED_MODE;

//
// Define the DMA extended mode register transfer size values.
//

#define BY_BYTE_8_BITS      0
#define BY_WORD_16_BITS     1
#define BY_BYTE_32_BITS     2
#define BY_BYTE_16_BITS     3

//
// Define the DMA extended mode timing mode values.
//

#define COMPATIBLITY_TIMING 0
#define TYPE_A_TIMING       1
#define TYPE_B_TIMING       2
#define BURST_TIMING        3

#ifndef DMA1_COMMAND_STATUS


//
// Define constants used by Intel 8237A DMA chip
//

#define DMA_SETMASK     4
#define DMA_CLEARMASK       0
#define DMA_READ            4  // These two appear backwards, but I think
#define DMA_WRITE           8  // the DMA docs have them mixed up
#define DMA_SINGLE_TRANSFER 0x40
#define DMA_AUTO_INIT       0x10 // Auto initialization mode
#endif


//
// This structure is drive layout and partition information
// for NEC PC-98xx series.
//

typedef struct _PARTITION_INFORMATION_NEC {
    UCHAR PartitionType;
    BOOLEAN RecognizedPartition;
    BOOLEAN RewritePartition;
    ULONG PartitionNumber;
    LARGE_INTEGER IplStartOffset;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    UCHAR BootableFlag;
    UCHAR PartitionName[16];
} PARTITION_INFORMATION_NEC, *PPARTITION_INFORMATION_NEC;

typedef struct _DRIVE_LAYOUT_INFORMATION_NEC {
    ULONG PartitionCount;
    ULONG Signature;
    UCHAR BootRecordNec[8];
    PARTITION_INFORMATION_NEC PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION_NEC, *PDRIVE_LAYOUT_INFORMATION_NEC;

//
// The system has memory over 16MB ?
//
extern UCHAR Over16MBMemoryFlag;

//
// We can't use DMA between 15MB and 16MB.
//
#define NOTDMA_MINIMUM_PHYSICAL_ADDRESS 0x0f00000

//
//
//
VOID
FASTCALL
xHalExamineMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    );

VOID
FASTCALL
xHalIoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

NTSTATUS
FASTCALL
xHalIoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

NTSTATUS
FASTCALL
xHalIoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTSTATUS
FASTCALL
xHalIoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

#endif //_EISA_
