/*
 * PROJECT:        ReactOS Kernel
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        To Implement AHCI Miniport driver targeting storport NT 5.2
 * PROGRAMMERS:    Aman Priyadarshi (aman.eureka@gmail.com)
 */

#include "miniport.h"
#include "storport.h"

#define AHCI_POOL_TAG 'ahci'
#define MAXIMUM_AHCI_PORT_COUNT 12

// section 3.1.2
#define AHCI_Global_HBA_CONTROL_HR          (0x1<<0)
#define AHCI_Global_HBA_CONTROL_IE          (0x1<<1)
#define AHCI_Global_HBA_CONTROL_MRSM        (0x1<<2)
#define AHCI_Global_HBA_CONTROL_AE          (0x1<<31)

//////////////////////////////////////////////////////////////
//              ---- Support Structures ---                 //
//////////////////////////////////////////////////////////////

typedef struct _AHCI_FIS_DMA_SETUP
{
    ULONG ULONG0_1;         // FIS_TYPE_DMA_SETUP
                            // Port multiplier
                            // Reserved
                            // Data transfer direction, 1 - device to host
                            // Interrupt bit
                            // Auto-activate. Specifies if DMA Activate FIS is needed
    UCHAR Reserved[2];      // Reserved
    ULONG DmaBufferLow;     // DMA Buffer Identifier. Used to Identify DMA buffer in host memory. SATA Spec says host specific and not in Spec. Trying AHCI spec might work.
    ULONG DmaBufferHigh;
    ULONG Reserved2;        //More reserved
    ULONG DmaBufferOffset;  //Byte offset into buffer. First 2 bits must be 0
    ULONG TranferCount;     //Number of bytes to transfer. Bit 0 must be 0
    ULONG Reserved3;        //Reserved
} AHCI_FIS_DMA_SETUP;

typedef struct _AHCI_PIO_SETUP_FIS
{
    UCHAR FisType;      //0x5F
    UCHAR Reserved1 :5;
    UCHAR D :1;         // 1 is write (device to host)
    UCHAR I :1;
    UCHAR Reserved2 :1;
    UCHAR Status;
    UCHAR Error;

    UCHAR SectorNumber;
    UCHAR CylLow;
    UCHAR CylHigh;
    UCHAR Dev_Head;

    UCHAR SectorNumb_Exp;
    UCHAR CylLow_Exp;
    UCHAR CylHigh_Exp;
    UCHAR Reserved3;

    UCHAR SectorCount;
    UCHAR SectorCount_Exp;
    UCHAR Reserved4;
    UCHAR E_Status;

    USHORT TransferCount;
    UCHAR Reserved5[2];
} AHCI_PIO_SETUP_FIS;

typedef struct _AHCI_D2H_REGISTER_FIS
{
    UCHAR FisType;
    UCHAR Reserved1 :6;
    UCHAR I:1;
    UCHAR Reserved2 :1;
    UCHAR Status;
    UCHAR Error;

    UCHAR SectorNumber;
    UCHAR CylLow;
    UCHAR CylHigh;
    UCHAR Dev_Head;

    UCHAR SectorNum_Exp;
    UCHAR CylLow_Exp;
    UCHAR CylHigh_Exp;
    UCHAR Reserved;

    UCHAR SectorCount;
    UCHAR SectorCount_Exp;
    UCHAR Reserved3[2];

    UCHAR Reserved4[4];
} AHCI_D2H_REGISTER_FIS;

typedef struct _AHCI_SET_DEVICE_BITS_FIS
{
    UCHAR FisType;

    UCHAR PMPort: 4;
    UCHAR Reserved1 :2;
    UCHAR I :1;
    UCHAR N :1;

    UCHAR Status_Lo :3;
    UCHAR Reserved2 :1;
    UCHAR Status_Hi :3;
    UCHAR Reserved3 :1;

    UCHAR Error;

    UCHAR Reserved5[4];
} AHCI_SET_DEVICE_BITS_FIS;

//////////////////////////////////////////////////////////////
//              ---------------------------                 //
//////////////////////////////////////////////////////////////

// 4.2.2 Command Header
typedef struct _AHCI_COMMAND_HEADER
{
    ULONG HEADER_DESCRIPTION;   // DW 0
    ULONG PRDBC;                // DW 1
    ULONG CTBA0;                // DW 2
    ULONG CTBA_U0;              // DW 3
    ULONG Reserved[4];          // DW 4-7
} AHCI_COMMAND_HEADER, *PAHCI_COMMAND_HEADER;

// Received FIS
typedef struct _AHCI_RECEIVED_FIS
{
    struct _AHCI_FIS_DMA_SETUP          DmaSetupFIS;      // 0x00 -- DMA Setup FIS
    ULONG                               pad0;             // 4 BYTE padding
    struct _AHCI_PIO_SETUP_FIS          PioSetupFIS;      // 0x20 -- PIO Setup FIS
    ULONG                               pad1[3];          // 12 BYTE padding
    struct _AHCI_D2H_REGISTER_FIS       RegisterFIS;      // 0x40 -- Register – Device to Host FIS
    ULONG                               pad2;             // 4 BYTE padding
    struct _AHCI_SET_DEVICE_BITS_FIS    SetDeviceFIS;     // 0x58 -- Set Device Bit FIS
    ULONG                               UnknowFIS[16];    // 0x60 -- Unknown FIS
    ULONG                               Reserved[24];     // 0xA0 -- Reserved
} AHCI_RECEIVED_FIS, *PAHCI_RECEIVED_FIS;

// Holds Port Information
typedef struct _AHCI_PORT
{
    ULONG   CLB;                                // 0x00, command list base address, 1K-byte aligned
    ULONG   CLBU;                               // 0x04, command list base address upper 32 bits
    ULONG   FB;                                 // 0x08, FIS base address, 256-byte aligned
    ULONG   FBU;                                // 0x0C, FIS base address upper 32 bits
    ULONG   IS;                                 // 0x10, interrupt status
    ULONG   IE;                                 // 0x14, interrupt enable
    ULONG   CMD;                                // 0x18, command and status
    ULONG   RSV0;                               // 0x1C, Reserved
    ULONG   TFD;                                // 0x20, task file data
    ULONG   SIG;                                // 0x24, signature
    ULONG   SSTS;                               // 0x28, SATA status (SCR0:SStatus)
    ULONG   SCTL;                               // 0x2C, SATA control (SCR2:SControl)
    ULONG   SERR;                               // 0x30, SATA error (SCR1:SError)
    ULONG   SACT;                               // 0x34, SATA active (SCR3:SActive)
    ULONG   CI;                                 // 0x38, command issue
    ULONG   SNTF;                               // 0x3C, SATA notification (SCR4:SNotification)
    ULONG   FBS;                                // 0x40, FIS-based switch control
    ULONG   RSV1[11];                           // 0x44 ~ 0x6F, Reserved
    ULONG   Vendor[4];                          // 0x70 ~ 0x7F, vendor specific
} AHCI_PORT, *PAHCI_PORT;

typedef struct _AHCI_MEMORY_REGISTERS
{
    // 0x00 - 0x2B, Generic Host Control
    ULONG CAP;                                  // 0x00, Host capability
    ULONG GHC;                                  // 0x04, Global host control
    ULONG IS;                                   // 0x08, Interrupt status
    ULONG PI;                                   // 0x0C, Port implemented
    ULONG VS;                                   // 0x10, Version
    ULONG CCC_CTL;                              // 0x14, Command completion coalescing control
    ULONG CCC_PTS;                              // 0x18, Command completion coalescing ports
    ULONG EM_LOC;                               // 0x1C, Enclosure management location
    ULONG EM_CTL;                               // 0x20, Enclosure management control
    ULONG CAP2;                                 // 0x24, Host capabilities extended
    ULONG BOHC;                                 // 0x28, BIOS/OS handoff control and status
    ULONG Reserved[0xA0-0x2C];                  // 0x2C - 0x9F, Reserved
    ULONG VendorSpecific[0x100-0xA0];           // 0xA0 - 0xFF, Vendor specific registers
    AHCI_PORT PortList[MAXIMUM_AHCI_PORT_COUNT];

} AHCI_MEMORY_REGISTERS, *PAHCI_MEMORY_REGISTERS;

// Holds information for each attached attached port to a given adapter.
typedef struct _AHCI_PORT_EXTENSION
{
    ULONG PortNumber;
    BOOLEAN IsActive;
    PAHCI_PORT Port;                                    // AHCI Port Infomation
    PAHCI_RECEIVED_FIS ReceivedFIS;
    PAHCI_COMMAND_HEADER CommandList;
    STOR_DEVICE_POWER_STATE DevicePowerState;           // Device Power State
    struct _AHCI_ADAPTER_EXTENSION* AdapterExtension;   // Port's Adapter Information
} AHCI_PORT_EXTENSION, *PAHCI_PORT_EXTENSION;

// Holds Adapter Information
typedef struct _AHCI_ADAPTER_EXTENSION
{
    ULONG   SystemIoBusNumber;
    ULONG   SlotNumber;
    ULONG   AhciBaseAddress;
    PULONG  IS;// Interrupt Status, In case of MSIM == `1`
    ULONG   PortImplemented;// bit-mapping of ports which are implemented

    USHORT  VendorID;
    USHORT  DeviceID;
    USHORT  RevisionID;

    ULONG   Version;
    ULONG   CAP;
    ULONG   CAP2;
    ULONG   LastInterruptPort;

    PVOID NonCachedExtension;// holds virtual address to noncached buffer allocated for Port Extension

    struct
    {
        // Message per port or shared port?
        ULONG MessagePerPort : 1;
        ULONG Removed : 1;
        ULONG Reserved : 30; // not in use -- maintain 4 byte alignment
    } StateFlags;

    PAHCI_MEMORY_REGISTERS ABAR_Address;
    AHCI_PORT_EXTENSION PortExtension[MAXIMUM_AHCI_PORT_COUNT];
} AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

typedef struct _AHCI_SRB_EXTENSION
{
    ULONG Reserved[4];
} AHCI_SRB_EXTENSION;

//////////////////////////////////////////////////////////////
//                       Declarations                       //
//////////////////////////////////////////////////////////////

BOOLEAN AhciAdapterReset(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension
);

__inline
VOID AhciZeroMemory(
  __in      PCHAR                               buffer,
  __in      ULONG                               bufferSize
);

__inline
BOOLEAN IsPortValid(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
  __in      UCHAR                               pathId
);

ULONG DeviceInquiryRequest(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
  __in      PSCSI_REQUEST_BLOCK                 Srb,
  __in      PCDB                                Cdb
);
