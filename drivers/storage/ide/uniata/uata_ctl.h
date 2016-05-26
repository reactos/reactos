/*++

Copyright (c) 2004-2005 Alexandr A. Telyatnikov (Alter)

Module Name:
    uata_ctl.h

Abstract:
    This header contains definitions for private UniATA SRB_IOCTL.

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

Licence:
    GPLv2

--*/

#ifndef __UNIATA_IO_CONTROL_CODES__H__
#define __UNIATA_IO_CONTROL_CODES__H__

//#include "scsi.h"

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define AHCI_MAX_PORT                   32
#define IDE_MAX_CHAN          16
// Thanks to SATA Port Multipliers:
#define IDE_MAX_LUN_PER_CHAN  2
#define IDE_MAX_LUN           (AHCI_MAX_PORT*IDE_MAX_LUN_PER_CHAN)

#define MAX_QUEUE_STAT        8

#define UNIATA_COMM_PORT_VENDOR_STR "UNIATA  " "Management Port " UNIATA_VER_STR

#ifndef UNIATA_CORE

#define IOCTL_SCSI_MINIPORT_UNIATA_FIND_DEVICES    ((FILE_DEVICE_SCSI << 16) + 0x09a0)
#define IOCTL_SCSI_MINIPORT_UNIATA_DELETE_DEVICE   ((FILE_DEVICE_SCSI << 16) + 0x09a1)
#define IOCTL_SCSI_MINIPORT_UNIATA_SET_MAX_MODE    ((FILE_DEVICE_SCSI << 16) + 0x09a2)
#define IOCTL_SCSI_MINIPORT_UNIATA_GET_MODE        ((FILE_DEVICE_SCSI << 16) + 0x09a3)
#define IOCTL_SCSI_MINIPORT_UNIATA_ADAPTER_INFO    ((FILE_DEVICE_SCSI << 16) + 0x09a4)
//#define IOCTL_SCSI_MINIPORT_UNIATA_LUN_IDENT       ((FILE_DEVICE_SCSI << 16) + 0x09a5) -> IOCTL_SCSI_MINIPORT_IDENTIFY
#define IOCTL_SCSI_MINIPORT_UNIATA_RESETBB         ((FILE_DEVICE_SCSI << 16) + 0x09a5)
#define IOCTL_SCSI_MINIPORT_UNIATA_RESET_DEVICE    ((FILE_DEVICE_SCSI << 16) + 0x09a6)
#define IOCTL_SCSI_MINIPORT_UNIATA_REG_IO          ((FILE_DEVICE_SCSI << 16) + 0x09a7)
#define IOCTL_SCSI_MINIPORT_UNIATA_GET_VERSION     ((FILE_DEVICE_SCSI << 16) + 0x09a8)

typedef struct _ADDREMOVEDEV {
    ULONG WaitForPhysicalLink; // us
    ULONG Flags;

#define UNIATA_REMOVE_FLAGS_HIDE   0x01
#define UNIATA_ADD_FLAGS_UNHIDE    0x01

} ADDREMOVEDEV, *PADDREMOVEDEV;

typedef struct _SETTRANSFERMODE {
    ULONG MaxMode;
    ULONG OrigMode;
    BOOLEAN ApplyImmediately;
    UCHAR Reserved[3];
} SETTRANSFERMODE, *PSETTRANSFERMODE;

typedef struct _GETTRANSFERMODE {
    ULONG MaxMode;
    ULONG OrigMode;
    ULONG CurrentMode;
    ULONG PhyMode; // since v0.42i6
} GETTRANSFERMODE, *PGETTRANSFERMODE;

typedef struct _GETDRVVERSION {
    ULONG Length;
    USHORT VersionMj;
    USHORT VersionMn;
    USHORT SubVerMj;
    USHORT SubVerMn;
    ULONG Reserved;
} GETDRVVERSION, *PGETDRVVERSION;

typedef struct _CHANINFO {
    ULONG               MaxTransferMode; // may differ from Controller's value due to 40-pin cable
    ULONG               ChannelCtrlFlags;
    LONGLONG QueueStat[MAX_QUEUE_STAT];
    LONGLONG ReorderCount;
    LONGLONG IntersectCount;
    LONGLONG TryReorderCount;
    LONGLONG TryReorderHeadCount;
    LONGLONG TryReorderTailCount; /* in-order requests */
//    ULONG               opt_MaxTransferMode; // user-specified
} CHANINFO, *PCHANINFO;

typedef struct _ADAPTERINFO {
    // Device identification
    ULONG HeaderLength;
    ULONG DevID;
    ULONG RevID;
    ULONG slotNumber;
    ULONG SystemIoBusNumber;
    ULONG DevIndex;

    ULONG Channel;

    ULONG   HbaCtrlFlags;
    BOOLEAN simplexOnly;
    BOOLEAN MemIo;
    BOOLEAN UnknownDev;
    BOOLEAN MasterDev;

    ULONG   MaxTransferMode;
    ULONG   HwFlags;
    ULONG   OrigAdapterInterfaceType;

    CHAR    DeviceName[64];

    ULONG BusInterruptLevel;    // Interrupt level
    ULONG InterruptMode;        // Interrupt Mode (Level or Edge)
    ULONG BusInterruptVector;
    // Number of channels being supported by one instantiation
    // of the device extension. Normally (and correctly) one, but
    // with so many broken PCI IDE controllers being sold, we have
    // to support them.
    ULONG NumberChannels;
    BOOLEAN ChanInfoValid;

    UCHAR   NumberLuns;  // per channel
    BOOLEAN LunInfoValid;
    BOOLEAN ChanHeaderLengthValid; // since v0.42i8

    ULONG   AdapterInterfaceType;
    ULONG   ChanHeaderLength;
    ULONG   LunHeaderLength;

    //CHANINFO Chan[0];

} ADAPTERINFO, *PADAPTERINFO;

#ifdef USER_MODE

typedef enum _INTERFACE_TYPE {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG   DeviceNumber:5;
            ULONG   FunctionNumber:3;
            ULONG   Reserved:24;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

#endif

#ifndef ATA_FLAGS_DRDY_REQUIRED

//The ATA_PASS_THROUGH_DIRECT structure is used in conjunction with an IOCTL_ATA_PASS_THROUGH_DIRECT request to instruct the port driver to send an embedded ATA command to the target device.

typedef struct _ATA_PASS_THROUGH_DIRECT {
  USHORT  Length;
  USHORT  AtaFlags;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  ReservedAsUchar;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  ULONG  ReservedAsUlong;
  PVOID  DataBuffer;
  union {
      UCHAR  PreviousTaskFile[8];
      IDEREGS Regs;
  };
  union {
      UCHAR  CurrentTaskFile[8];
      IDEREGS RegsH;
  };
} ATA_PASS_THROUGH_DIRECT, *PATA_PASS_THROUGH_DIRECT;

#define    ATA_FLAGS_DRDY_REQUIRED 0x01 // Wait for DRDY status from the device before sending the command to the device.
#define    ATA_FLAGS_DATA_OUT 	   0x02 // Write data to the device.
#define    ATA_FLAGS_DATA_IN 	   0x04 // Read data from the device.
#define    ATA_FLAGS_48BIT_COMMAND 0x08 // The ATA command to be send uses the 48 bit LBA feature set.
                                        //   When this flag is set, the contents of the PreviousTaskFile member in the
                                        //   ATA_PASS_THROUGH_DIRECT structure should be valid.
#define    ATA_FLAGS_USE_DMA       0x10 // Set the transfer mode to DMA.
#define    ATA_FLAGS_NO_MULTIPLE   0x20 // Read single sector only.

#endif //ATA_FLAGS_DRDY_REQUIRED

#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _IDEREGS_EX {
    union {
        UCHAR    bFeaturesReg;           // Used for specifying SMART "commands" on input.
        UCHAR    bErrorReg;              // Error on output.
    };
        UCHAR    bSectorCountReg;        // IDE sector count register
        UCHAR    bSectorNumberReg;       // IDE sector number register
        UCHAR    bCylLowReg;             // IDE low order cylinder value
        UCHAR    bCylHighReg;            // IDE high order cylinder value
        UCHAR    bDriveHeadReg;          // IDE drive/head register
    union {
        UCHAR    bCommandReg;            // Actual IDE command.
        UCHAR    bStatusReg;             // Status register.
    };
        UCHAR    bOpFlags;               // 00 - send
                                         // 01 - read regs
                                         // 08 - lba48
                                         // 10 - treat timeout as msec

#define UNIATA_SPTI_EX_SND               0x00
#define UNIATA_SPTI_EX_RCV               0x01
#define UNIATA_SPTI_EX_LBA48             0x08
//#define UNIATA_SPTI_EX_SPEC_TO           0x10
//#define UNIATA_SPTI_EX_FREEZE_TO         0x20 // do not reset device on timeout and keep interrupts disabled
#define UNIATA_SPTI_EX_USE_DMA 	         0x10 // Force DMA transfer mode

// use 'invalid' combination to specify special TO options
#define UNIATA_SPTI_EX_SPEC_TO           (ATA_FLAGS_DATA_OUT | ATA_FLAGS_DATA_IN)

        UCHAR    bFeaturesRegH;          // feature (high part for LBA48 mode)
        UCHAR    bSectorCountRegH;       // IDE sector count register (high part for LBA48 mode)
        UCHAR    bSectorNumberRegH;      // IDE sector number register (high part for LBA48 mode)
        UCHAR    bCylLowRegH;            // IDE low order cylinder value (high part for LBA48 mode)
        UCHAR    bCylHighRegH;           // IDE high order cylinder value (high part for LBA48 mode)
        UCHAR    bReserved2;             // 0
} IDEREGS_EX, *PIDEREGS_EX, *LPIDEREGS_EX;

typedef struct _UNIATA_REG_IO {
    USHORT RegIDX;
    UCHAR  RegSz:3;   // 0=1, 1=2, 2=4, 3=1+1 (for lba48) 4=2+2 (for lba48)
    UCHAR  InOut:1;   // 0=in, 1=out
    UCHAR  Reserved:4;
    UCHAR  Reserved1;
    union {
        ULONG  Data;
        ULONG  d32;
        USHORT d16[2];
        USHORT d8[2];
    };
} UNIATA_REG_IO, *PUNIATA_REG_IO;

typedef struct _UNIATA_REG_IO_HDR {
    ULONG          ItemCount;
    UNIATA_REG_IO  r[1];
} UNIATA_REG_IO_HDR, *PUNIATA_REG_IO_HDR;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct _UNIATA_CTL {
    SRB_IO_CONTROL hdr;
    SCSI_ADDRESS   addr;
    ULONG Reserved;
    union {
        UCHAR                   RawData[1];
        ADDREMOVEDEV            FindDelDev;
        SETTRANSFERMODE         SetMode;
        GETTRANSFERMODE         GetMode;
        ADAPTERINFO             AdapterInfo;
//        IDENTIFY_DATA2        LunIdent;
//        ATA_PASS_THROUGH_DIRECT AtaDirect;
        GETDRVVERSION           Version;
        UNIATA_REG_IO_HDR       RegIo;
    };
} UNIATA_CTL, *PUNIATA_CTL;

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[32];
    UCHAR             ucDataBuf[512]; // recommended minimum
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

#endif //UNIATA_CORE

#ifdef __cplusplus
};
#endif //__cplusplus

#pragma pack(pop)

#endif //__UNIATA_IO_CONTROL_CODES__H__
