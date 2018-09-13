/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1991  NCR Corporation

Module Name:

    mca.h

Abstract:

    This module contains the defines and structure definitions for
    Micro Channel machines.

Author:

    David Risner  (o-ncrdr) 21-Jul-1991

Revision History:


--*/

#ifndef _MCA_
#define _MCA_





//
// Define the DMA page register structure (for 8237 compatibility)
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
} DMA_PAGE, *PDMA_PAGE;

//
// Define DMA 1 address and count structure (for 8237 compatibility)
//

typedef struct _DMA1_ADDRESS_COUNT {
    UCHAR DmaBaseAddress;
    UCHAR DmaBaseCount;
} DMA1_ADDRESS_COUNT, *PDMA1_ADDRESS_COUNT;

//
// Define DMA 2 address and count structure (for 8237 compatibility)
//

typedef struct _DMA2_ADDRESS_COUNT {
    UCHAR DmaBaseAddress;
    UCHAR Reserved1;
    UCHAR DmaBaseCount;
    UCHAR Reserved2;
} DMA2_ADDRESS_COUNT, *PDMA2_ADDRESS_COUNT;

//
// Define DMA 1 control register structure (for 8237 compatibility)
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
} DMA1_CONTROL, *PDMA1_CONTROL;

//
// Define DMA 2 control register structure (for 8237 compatibility)
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
} DMA2_CONTROL, *PDMA2_CONTROL;

typedef struct _MCA_DMA_CONTROLLER {
    UCHAR DmaFunctionLsb;               // Offset 0x018
    UCHAR DmaFunctionMsb;               // Offset 0x019
    UCHAR DmaFunctionData;              // Offset 0x01a
    UCHAR Reserved01;
    UCHAR ScbAttentionPort;             // Offset 0x01c
    UCHAR ScbCommandPort;               // Offset 0x01d
    UCHAR Reserved02;
    UCHAR ScbStatusPort;                // Offset 0x01f
} MCA_DMA_CONTROLLER, *PMCA_DMA_CONTROLLER;

//
// Define Programmable Option Select register set
//

typedef struct _PROGRAMMABLE_OPTION_SELECT {
    UCHAR AdapterIdLsb;
    UCHAR AdapterIdMsb;
    UCHAR OptionSelectData1;
    UCHAR OptionSelectData2;
    UCHAR OptionSelectData3;
    UCHAR OptionSelectData4;
    UCHAR SubaddressExtensionLsb;
    UCHAR SubaddressExtensionMsb;
} PROGRAMMABLE_OPTION_SELECT, *PPROGRAMMABLE_OPTION_SELECT;

//
// Define Micro Channel i/o address map
//

typedef struct _MCA_CONTROL {
    DMA1_CONTROL Dma1BasePort;          // Offset 0x000
    UCHAR Reserved0[8];
    UCHAR ExtendedDmaBasePort[8];       // Offset 0x018 
    UCHAR Interrupt1ControlPort0;       // Offset 0x020
    UCHAR Interrupt1ControlPort1;       // Offset 0x021
    UCHAR Reserved1[64 - 1];
    UCHAR SystemControlPortB;           // Offset 0x061
    UCHAR Reserved2[32 - 2];
    DMA_PAGE DmaPageLowPort;            // Offset 0x080
    UCHAR Reserved3;
    UCHAR CardSelectedFeedback;         // Offset 0x091
    UCHAR SystemControlPortA;           // Offset 0x092
    UCHAR Reserved4;
    UCHAR SystemBoardSetup;             // Offset 0x094
    UCHAR Reserved5;
    UCHAR AdapterSetup;                 // Offset 0x096
    UCHAR AdapterSetup2;                // Offset 0x097
    UCHAR Reserved7[8];
    UCHAR Interrupt2ControlPort0;       // Offset 0x0a0
    UCHAR Interrupt2ControlPort1;       // Offset 0x0a1
    UCHAR Reserved8[32-2];
    DMA2_CONTROL Dma2BasePort;          // Offset 0x0c0
    UCHAR Reserved9[32];
    PROGRAMMABLE_OPTION_SELECT Pos;     // Offset 0x100
} MCA_CONTROL, *PMCA_CONTROL;

//
// Define POS adapter setup equates for use with AdapterSetup field above
//

#define MCA_ADAPTER_SETUP_ON  0x008
#define MCA_ADAPTER_SETUP_OFF 0x000

//
// Define DMA Extended Function register
//

typedef struct _DMA_EXTENDED_FUNCTION {
    UCHAR ChannelNumber : 3;
    UCHAR Reserved      : 1;
    UCHAR Command       : 4;
} DMA_EXTENDED_FUNCTION, *PDMA_EXTENDED_FUNCTION;

//
// Define Command values
//

#define WRITE_IO_ADDRESS         0x00   // write I/O address reg
#define WRITE_MEMORY_ADDRESS     0x20   // write memory address reg
#define READ_MEMORY_ADDRESS      0x30   // read memory address reg
#define WRITE_TRANSFER_COUNT     0x40   // write transfer count reg
#define READ_TRANSFER_COUNT      0x50   // read transfer count reg
#define READ_STATUS              0x60   // read status register
#define WRITE_MODE               0x70   // write mode register
#define WRITE_ARBUS              0x80   // write arbus register
#define SET_MASK_BIT             0x90   // set bit in mask reg
#define CLEAR_MASK_BIT           0xa0   // clear bit in mask reg
#define MASTER_CLEAR             0xd0   // master clear

//
// Define DMA Extended Mode register
//

typedef struct _DMA_EXTENDED_MODE {
    UCHAR ProgrammedIo      : 1;     // 0 = do not use programmed i/o address
    UCHAR AutoInitialize    : 1;
    UCHAR DmaOpcode         : 1;     // 0 = verify memory, 1 = data transfer
    UCHAR TransferDirection : 1;     // 0 = read memory, 1 = write memory
    UCHAR Reserved1         : 2;
    UCHAR DmaWidth          : 1;     // 0 = 8bit, 1 = 16bit
    UCHAR Reserved2         : 1;
} DMA_EXTENDED_MODE, *PDMA_EXTENDED_MODE;

//
// DMA Extended Mode equates for use with the _DMA_EXTENDED_MODE structure.
//

#define DMA_EXT_USE_PIO       0x01
#define DMA_EXT_NO_PIO        0x00
#define DMA_EXT_VERIFY        0x00
#define DMA_EXT_DATA_XFER     0x01
#define DMA_EXT_WIDTH_8_BIT   0x00
#define DMA_EXT_WIDTH_16_BIT  0x01

//
// DMA mode option definitions
//

#define DMA_MODE_READ          0x00   // read data into memory
#define DMA_MODE_WRITE         0x08   // write data from memory
#define DMA_MODE_VERIFY        0x00   // verify data
#define DMA_MODE_TRANSFER      0x04   // transfer data

// 
// DMA extended mode constants
//

#define MAX_MCA_DMA_CHANNEL_NUMBER  0x07 // maximum MCA DMA channel number
#endif 
