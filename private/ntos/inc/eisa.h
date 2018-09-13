/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eisa.h

Abstract:

    The module defines the structures, and defines  for the EISA chip set.

Author:

    Jeff Havens  (jhavens) 19-Jun-1991

Revision History:


--*/

#ifndef _EISA_
#define _EISA_



//
// Define the DMA page register structure.
//

typedef struct _DMA_PAGE{
    UCHAR Reserved1;
    UCHAR Channel2;
    UCHAR Channel3;
    UCHAR Channel1;
    UCHAR Reserved2[3];
    UCHAR Channel0;
    UCHAR Reserved3;
    UCHAR Channel6;
    UCHAR Channel7;
    UCHAR Channel5;
    UCHAR Reserved4[3];
    UCHAR RefreshPage;
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
    UCHAR DmaBaseAddress;
    UCHAR DmaBaseCount;
}DMA1_ADDRESS_COUNT, *PDMA1_ADDRESS_COUNT;

//
// Define DMA 2 address and count structure.
//

typedef struct _DMA2_ADDRESS_COUNT {
    UCHAR DmaBaseAddress;
    UCHAR Reserved1;
    UCHAR DmaBaseCount;
    UCHAR Reserved2;
}DMA2_ADDRESS_COUNT, *PDMA2_ADDRESS_COUNT;

//
// Define DMA 1 control register structure.
//

typedef struct _DMA1_CONTROL {
    DMA1_ADDRESS_COUNT DmaAddressCount[4];
    UCHAR DmaStatus;
    UCHAR DmaRequest;
    UCHAR SingleMask;
    UCHAR Mode;
    UCHAR ClearBytePointer;
    UCHAR MasterClear;
    UCHAR ClearMask;
    UCHAR AllMask;
}DMA1_CONTROL, *PDMA1_CONTROL;

//
// Define DMA 2 control register structure.
//

typedef struct _DMA2_CONTROL {
    DMA2_ADDRESS_COUNT DmaAddressCount[4];
    UCHAR DmaStatus;
    UCHAR Reserved1;
    UCHAR DmaRequest;
    UCHAR Reserved2;
    UCHAR SingleMask;
    UCHAR Reserved3;
    UCHAR Mode;
    UCHAR Reserved4;
    UCHAR ClearBytePointer;
    UCHAR Reserved5;
    UCHAR MasterClear;
    UCHAR Reserved6;
    UCHAR ClearMask;
    UCHAR Reserved7;
    UCHAR AllMask;
    UCHAR Reserved8;
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
    DMA1_CONTROL Dma1BasePort;          // Offset 0x000
    UCHAR Reserved0[16];
    UCHAR Interrupt1ControlPort0;       // Offset 0x020
    UCHAR Interrupt1ControlPort1;       // Offset 0x021
    UCHAR Reserved1[32 - 2];
    UCHAR Timer1;                       // Offset 0x40
    UCHAR RefreshRequest;               // Offset 0x41
    UCHAR SpeakerTone;                  // Offset 0x42
    UCHAR CommandMode1;                 // Offset 0x43
    UCHAR Reserved17[4];
    UCHAR Timer2;                       // Offset 0x48
    UCHAR Reserved13;
    UCHAR CpuSpeedControl;              // Offset 0x4a
    UCHAR CommandMode2;                 // Offset 0x4b
    UCHAR Reserved14[21];
    UCHAR NmiStatus;                    // Offset 0x61
    UCHAR Reserved15[14];
    UCHAR NmiEnable;                    // Offset 0x70
    UCHAR Reserved16[15];
    DMA_PAGE DmaPageLowPort;            // Offset 0x080
    UCHAR Reserved2[16];
    UCHAR Interrupt2ControlPort0;       // Offset 0x0a0
    UCHAR Interrupt2ControlPort1;       // Offset 0x0a1
    UCHAR Reserved3[32-2];
    DMA2_CONTROL Dma2BasePort;          // Offset 0x0c0
    UCHAR Reserved4[0x320];
    UCHAR Dma1CountHigh[8];             // Offset 0x400
    UCHAR Reserved5[2];
    UCHAR Dma1ChainingInterrupt;        // Offset 0x40a
    UCHAR Dma1ExtendedModePort;         // Offset 0x40b
    UCHAR MasterControlPort;            // Offset 0x40c
    UCHAR SteppingLevelRegister;        // Offset 0x40d
    UCHAR IspTest1;                     // Offset 0x40e
    UCHAR IspTest2;                     // Offset 0x40f
    UCHAR Reserved6[81];
    UCHAR ExtendedNmiResetControl;      // Offset 0x461
    UCHAR NmiIoInterruptPort;           // Offset 0x462
    UCHAR Reserved7;
    UCHAR LastMaster;                   // Offset 0x464
    UCHAR Reserved8[27];
    DMA_PAGE DmaPageHighPort;           // Offset 0x480
    UCHAR Reserved12[48];
    UCHAR Dma2HighCount[16];            // Offset 0x4c0
    UCHAR Interrupt1EdgeLevel;          // Offset 0x4d0
    UCHAR Interrupt2EdgeLevel;          // Offset 0x4d1
    UCHAR Reserved9[2];
    UCHAR Dma2ChainingInterrupt;        // Offset 0x4d4
    UCHAR Reserved10;
    UCHAR Dma2ExtendedModePort;         // Offset 0x4d6
    UCHAR Reserved11[9];
    DMA_CHANNEL_STOP DmaChannelStop[8]; // Offset 0x4e0
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
// Define the IRQL which the slave intterrupts the master controller.
//

#define SLAVE_IRQL_LEVEL 2

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
#endif
