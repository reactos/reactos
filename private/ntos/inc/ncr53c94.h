/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ncr53C94.h

Abstract:

    The module defines the structures, defines and functions for the NCR 53C94
    host bus adapter chip.

Author:

    Jeff Havens  (jhavens) 28-Feb-1991

Revision History:

    R.D. Lanser  (DEC)     05-Oct-1991
        Copied SCSI_REGISTER structure from d3scsidd.c and added check for
        DECSTATION.  Changed the UCHAR's in the read/write register structures
        with SCSI_REGISTER, and added the dot Byte member reference to
        SCSI_WRITE and SCSI_READ macros.



--*/

#ifndef _NCR53C94_
#define _NCR53C94_


//
// Define SCSI Protocol Chip register format.
//

#if defined(DECSTATION)

typedef struct _SCSI_REGISTER {
    UCHAR Byte;
    UCHAR Fill[3];
} SCSI_REGISTER, *PSCSI_REGISTER;

#else

#define SCSI_REGISTER UCHAR

#endif // DECSTATION

//
// SCSI Protocol Chip Definitions.
//
// Define SCSI Protocol Chip Read registers structure.
//

typedef struct _SCSI_READ_REGISTERS {
    SCSI_REGISTER TransferCountLow;
    SCSI_REGISTER TransferCountHigh;
    SCSI_REGISTER Fifo;
    SCSI_REGISTER Command;
    SCSI_REGISTER ScsiStatus;
    SCSI_REGISTER ScsiInterrupt;
    SCSI_REGISTER SequenceStep;
    SCSI_REGISTER FifoFlags;
    SCSI_REGISTER Configuration1;
    SCSI_REGISTER Reserved1;
    SCSI_REGISTER Reserved2;
    SCSI_REGISTER Configuration2;
    SCSI_REGISTER Configuration3;
    SCSI_REGISTER Reserved;
    SCSI_REGISTER TransferCountPage;
    SCSI_REGISTER FifoBottem;
} SCSI_READ_REGISTERS, *PSCSI_READ_REGISTERS;

//
// Define SCSI Protocol Chip Write registers structure.
//

typedef struct _SCSI_WRITE_REGISTERS {
    SCSI_REGISTER TransferCountLow;
    SCSI_REGISTER TransferCountHigh;
    SCSI_REGISTER Fifo;
    SCSI_REGISTER Command;
    SCSI_REGISTER DestinationId;
    SCSI_REGISTER SelectTimeOut;
    SCSI_REGISTER SynchronousPeriod;
    SCSI_REGISTER SynchronousOffset;
    SCSI_REGISTER Configuration1;
    SCSI_REGISTER ClockConversionFactor;
    SCSI_REGISTER TestMode;
    SCSI_REGISTER Configuration2;
    SCSI_REGISTER Configuration3;
    SCSI_REGISTER Reserved;
    SCSI_REGISTER TransferCountPage;
    SCSI_REGISTER FifoBottem;
} SCSI_WRITE_REGISTERS, *PSCSI_WRITE_REGISTERS;

typedef union _SCSI_REGISTERS {
    SCSI_READ_REGISTERS  ReadRegisters;
    SCSI_WRITE_REGISTERS WriteRegisters;
} SCSI_REGISTERS, *PSCSI_REGISTERS;

//
// Define SCSI Command Codes.
//

#define NO_OPERATION_DMA 0x80
#define FLUSH_FIFO 0x1
#define RESET_SCSI_CHIP 0x2
#define RESET_SCSI_BUS 0x3
#define TRANSFER_INFORMATION 0x10
#define TRANSFER_INFORMATION_DMA 0x90
#define COMMAND_COMPLETE 0x11
#define MESSAGE_ACCEPTED 0x12
#define TRANSFER_PAD 0x18
#define SET_ATTENTION 0x1a
#define RESET_ATTENTION 0x1b
#define RESELECT 0x40
#define SELECT_WITHOUT_ATTENTION 0x41
#define SELECT_WITH_ATTENTION 0x42
#define SELECT_WITH_ATTENTION_STOP 0x43
#define ENABLE_SELECTION_RESELECTION 0x44
#define DISABLE_SELECTION_RESELECTION 0x45
#define SELECT_WITH_ATTENTION3 0x46

//
// Define SCSI Status Register structure.
//
typedef struct _SCSI_STATUS {
    UCHAR Phase : 3;
    UCHAR ValidGroup : 1;
    UCHAR TerminalCount : 1;
    UCHAR ParityError : 1;
    UCHAR GrossError : 1;
    UCHAR Interrupt : 1;
} SCSI_STATUS, *PSCSI_STATUS;

//
// Define SCSI Phase Codes.
//

#define DATA_OUT 0x0
#define DATA_IN 0x1
#define COMMAND_OUT 0x2
#define STATUS_IN 0x3
#define MESSAGE_OUT 0x6
#define MESSAGE_IN 0x7

//
// Define SCSI Interrupt Register structure.
//

typedef struct _SCSI_INTERRUPT {
    UCHAR Selected : 1;
    UCHAR SelectedWithAttention : 1;
    UCHAR Reselected : 1;
    UCHAR FunctionComplete : 1;
    UCHAR BusService : 1;
    UCHAR Disconnect : 1;
    UCHAR IllegalCommand : 1;
    UCHAR ScsiReset : 1;
} SCSI_INTERRUPT, *PSCSI_INTERRUPT;

//
// Define SCSI Sequence Step Register structure.
//

typedef struct _SCSI_SEQUENCE_STEP {
    UCHAR Step : 3;
    UCHAR MaximumOffset : 1;
    UCHAR Reserved : 4;
} SCSI_SEQUENCE_STEP, *PSCSI_SEQUENCE_STEP;

//
// Define SCSI Fifo Flags Register structure.
//

typedef struct _SCSI_FIFO_FLAGS {
    UCHAR ByteCount : 5;
    UCHAR FifoStep : 3;
} SCSI_FIFO_FLAGS, *PSCSI_FIFO_FLAGS;

//
// Define SCSI Configuration 1 Register structure.
//

typedef struct _SCSI_CONFIGURATION1 {
    UCHAR HostBusId : 3;
    UCHAR ChipTestEnable : 1;
    UCHAR ParityEnable : 1;
    UCHAR ParityTestMode : 1;
    UCHAR ResetInterruptDisable : 1;
    UCHAR SlowCableMode : 1;
} SCSI_CONFIGURATION1, *PSCSI_CONFIGURATION1;

//
// Define SCSI Configuration 2 Register structure.
//

typedef struct _SCSI_CONFIGURATION2 {
    UCHAR DmaParityEnable : 1;
    UCHAR RegisterParityEnable : 1;
    UCHAR TargetBadParityAbort : 1;
    UCHAR Scsi2 : 1;
    UCHAR HighImpedance : 1;
    UCHAR EnableByteControl : 1;
    UCHAR EnablePhaseLatch : 1;
    UCHAR ReserveFifoByte : 1;
} SCSI_CONFIGURATION2, *PSCSI_CONFIGURATION2;

//
// Define SCSI Configuration 3 Register structure.
//

typedef struct _SCSI_CONFIGURATION3 {
    UCHAR Threshold8 : 1;
    UCHAR AlternateDmaMode : 1;
    UCHAR SaveResidualByte : 1;
    UCHAR FastClock : 1;
    UCHAR FastScsi : 1;
    UCHAR EnableCdb10 : 1;
    UCHAR EnableQueue : 1;
    UCHAR CheckIdMessage : 1;
} SCSI_CONFIGURATION3, *PSCSI_CONFIGURATION3;

//
// Define Emulex FAS 218 unique part Id code.
//

typedef struct _NCR_PART_CODE {
    UCHAR RevisionLevel : 3;
    UCHAR ChipFamily : 5;
}NCR_PART_CODE, *PNCR_PART_CODE;

#define EMULEX_FAS_216 2

//
// SCSI Protocol Chip Control read and write macros.
//

#if defined(DECSTATION)

#define SCSI_READ(ChipAddr, Register) \
    (READ_REGISTER_UCHAR (&((ChipAddr)->ReadRegisters.Register.Byte)))

#define SCSI_WRITE(ChipAddr, Register, Value) \
    WRITE_REGISTER_UCHAR(&((ChipAddr)->WriteRegisters.Register.Byte), (Value))

#else

#define SCSI_READ(ChipAddr, Register) \
    (READ_REGISTER_UCHAR (&((ChipAddr)->ReadRegisters.Register)))

#define SCSI_WRITE(ChipAddr, Register, Value) \
    WRITE_REGISTER_UCHAR(&((ChipAddr)->WriteRegisters.Register), (Value))

#endif


#endif
