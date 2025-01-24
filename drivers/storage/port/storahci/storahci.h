/*
 * PROJECT:        ReactOS Kernel
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        To Implement AHCI Miniport driver targeting storport NT 5.2
 * PROGRAMMERS:    Aman Priyadarshi (aman.eureka@gmail.com)
 */

#include <ntddk.h>
#include <ata.h>
#include <storport.h>

#define NDEBUG
#include <debug.h>

#define DEBUG 1
#if defined(_MSC_VER)
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union
#endif

#define MAXIMUM_AHCI_PORT_COUNT             32
#define MAXIMUM_AHCI_PRDT_ENTRIES           32
#define MAXIMUM_AHCI_PORT_NCS               30
#define MAXIMUM_QUEUE_BUFFER_SIZE           255
#define MAXIMUM_TRANSFER_LENGTH             (128*1024) // 128 KB

#define DEVICE_ATA_BLOCK_SIZE               512

// device type (DeviceParams)
#define AHCI_DEVICE_TYPE_ATA                1
#define AHCI_DEVICE_TYPE_ATAPI              2
#define AHCI_DEVICE_TYPE_NODEVICE           3

// section 3.1.2
#define AHCI_Global_HBA_CAP_S64A            (1 << 31)

// FIS Types : https://wiki.osdev.org/AHCI
#define FIS_TYPE_REG_H2D        0x27 // Register FIS - host to device
#define FIS_TYPE_REG_D2H        0x34 // Register FIS - device to host
#define FIS_TYPE_DMA_ACT        0x39 // DMA activate FIS - device to host
#define FIS_TYPE_DMA_SETUP      0x41 // DMA setup FIS - bidirectional
#define FIS_TYPE_BIST           0x58 // BIST activate FIS - bidirectional
#define FIS_TYPE_PIO_SETUP      0x5F // PIO setup FIS - device to host
#define FIS_TYPE_DEV_BITS       0xA1 // Set device bits FIS - device to host

#define AHCI_ATA_CFIS_FisType               0
#define AHCI_ATA_CFIS_PMPort_C              1
#define AHCI_ATA_CFIS_CommandReg            2
#define AHCI_ATA_CFIS_FeaturesLow           3
#define AHCI_ATA_CFIS_LBA0                  4
#define AHCI_ATA_CFIS_LBA1                  5
#define AHCI_ATA_CFIS_LBA2                  6
#define AHCI_ATA_CFIS_Device                7
#define AHCI_ATA_CFIS_LBA3                  8
#define AHCI_ATA_CFIS_LBA4                  9
#define AHCI_ATA_CFIS_LBA5                  10
#define AHCI_ATA_CFIS_FeaturesHigh          11
#define AHCI_ATA_CFIS_SectorCountLow        12
#define AHCI_ATA_CFIS_SectorCountHigh       13

// ATA Functions
#define ATA_FUNCTION_ATA_COMMAND            0x100
#define ATA_FUNCTION_ATA_IDENTIFY           0x101
#define ATA_FUNCTION_ATA_READ               0x102

// ATAPI Functions
#define ATA_FUNCTION_ATAPI_COMMAND          0x200

// ATA Flags
#define ATA_FLAGS_DATA_IN                   (1 << 1)
#define ATA_FLAGS_DATA_OUT                  (1 << 2)
#define ATA_FLAGS_48BIT_COMMAND             (1 << 3)
#define ATA_FLAGS_USE_DMA                   (1 << 4)

#define IsAtaCommand(AtaFunction)           (AtaFunction & ATA_FUNCTION_ATA_COMMAND)
#define IsAtapiCommand(AtaFunction)         (AtaFunction & ATA_FUNCTION_ATAPI_COMMAND)
#define IsDataTransferNeeded(SrbExtension)  (SrbExtension->Flags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT))
#define IsAdapterCAPS64(CAP)                (CAP & AHCI_Global_HBA_CAP_S64A)

// 3.1.1 NCS = CAP[12:08] -> Align
#define AHCI_Global_Port_CAP_NCS(x)         (((x) & 0xF00) >> 8)

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
//#define AhciDebugPrint(format, ...) StorPortDebugPrint(0, format, __VA_ARGS__)
#define AhciDebugPrint(format, ...) DbgPrint("(%s:%d) " format, __RELFILE__, __LINE__, ##__VA_ARGS__)

typedef
VOID
(*PAHCI_COMPLETION_ROUTINE) (
    __in PVOID PortExtension,
    __in PVOID Srb
    );

//////////////////////////////////////////////////////////////
//              ---- Support Structures ---                 //
//////////////////////////////////////////////////////////////

// section 3.3.5
typedef union _AHCI_INTERRUPT_STATUS
{
    struct
    {
        ULONG DHRS:1;       //Device to Host Register FIS Interrupt
        ULONG PSS :1;       //PIO Setup FIS Interrupt
        ULONG DSS :1;       //DMA Setup FIS Interrupt
        ULONG SDBS :1;      //Set Device Bits Interrupt
        ULONG UFS :1;       //Unknown FIS Interrupt
        ULONG DPS :1;       //Descriptor Processed
        ULONG PCS :1;       //Port Connect Change Status
        ULONG DMPS :1;      //Device Mechanical Presence Status (DMPS)
        ULONG Reserved :14;
        ULONG PRCS :1;      //PhyRdy Change Status
        ULONG IPMS :1;      //Incorrect Port Multiplier Status
        ULONG OFS :1;       //Overflow Status
        ULONG Reserved2 :1;
        ULONG INFS :1;      //Interface Non-fatal Error Status
        ULONG IFS :1;       //Interface Fatal Error Status
        ULONG HBDS :1;      //Host Bus Data Error Status
        ULONG HBFS :1;      //Host Bus Fatal Error Status
        ULONG TFES :1;      //Task File Error Status
        ULONG CPDS :1;      //Cold Port Detect Status
    };

    ULONG Status;
} AHCI_INTERRUPT_STATUS;

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
    ULONG Reserved2;        // More reserved
    ULONG DmaBufferOffset;  // Byte offset into buffer. First 2 bits must be 0
    ULONG TranferCount;     // Number of bytes to transfer. Bit 0 must be 0
    ULONG Reserved3;        // Reserved
} AHCI_FIS_DMA_SETUP;

typedef struct _AHCI_PIO_SETUP_FIS
{
    UCHAR FisType;
    UCHAR Reserved1 :5;
    UCHAR D :1;
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

typedef struct _AHCI_QUEUE
{
    PVOID Buffer[MAXIMUM_QUEUE_BUFFER_SIZE];  // because Storahci hold Srb queue of 255 size
    ULONG Head;
    ULONG Tail;
} AHCI_QUEUE, *PAHCI_QUEUE;

//////////////////////////////////////////////////////////////
//              ---------------------------                 //
//////////////////////////////////////////////////////////////

typedef union _AHCI_COMMAND_HEADER_DESCRIPTION
{
    struct
    {
        ULONG CFL : 5;       // Command FIS Length
        ULONG A : 1;         // IsATAPI
        ULONG W : 1;         // Write
        ULONG P : 1;         // Prefetchable

        ULONG R : 1;         // Reset
        ULONG B : 1;         // BIST
        ULONG C : 1;         //Clear Busy upon R_OK
        ULONG RSV : 1;
        ULONG PMP : 4;       //Port Multiplier Port

        ULONG PRDTL : 16;    //Physical Region Descriptor Table Length
    };

    ULONG Status;
} AHCI_COMMAND_HEADER_DESCRIPTION;

typedef union _AHCI_GHC
{
    struct
    {
        ULONG HR : 1;
        ULONG IE : 1;
        ULONG MRSM : 1;
        ULONG RSV0 : 28;
        ULONG AE : 1;
    };

    ULONG Status;
} AHCI_GHC;

// section 3.3.7
typedef union _AHCI_PORT_CMD
{
    struct
    {
        ULONG ST : 1;
        ULONG SUD : 1;
        ULONG POD : 1;
        ULONG CLO : 1;
        ULONG FRE : 1;
        ULONG RSV0 : 3;
        ULONG CCS : 5;
        ULONG MPSS : 1;
        ULONG FR : 1;
        ULONG CR : 1;
        ULONG CPS : 1;
        ULONG PMA : 1;
        ULONG HPCP : 1;
        ULONG MPSP : 1;
        ULONG CPD : 1;
        ULONG ESP : 1;
        ULONG FBSCP : 1;
        ULONG APSTE : 1;
        ULONG ATAPI : 1;
        ULONG DLAE : 1;
        ULONG ALPE : 1;
        ULONG ASP : 1;
        ULONG ICC : 4;
    };

    ULONG Status;
} AHCI_PORT_CMD;

typedef union _AHCI_SERIAL_ATA_CONTROL
{
    struct
    {
        ULONG DET :4;
        ULONG SPD :4;
        ULONG IPM :4;
        ULONG SPM :4;
        ULONG PMP :4;
        ULONG DW11_Reserved :12;
    };

    ULONG Status;
}  AHCI_SERIAL_ATA_CONTROL;

typedef union _AHCI_SERIAL_ATA_STATUS
{
    struct
    {
        ULONG DET :4;
        ULONG SPD :4;
        ULONG IPM :4;
        ULONG RSV0 :20;
    };

    ULONG Status;
}  AHCI_SERIAL_ATA_STATUS;

typedef union _AHCI_TASK_FILE_DATA
{
    struct
    {
        struct _STS
        {
            UCHAR ERR : 1;
            UCHAR CS1 : 2;
            UCHAR DRQ : 1;
            UCHAR CS2 : 3;
            UCHAR BSY : 1;
        } STS;
        UCHAR ERR;
        USHORT RSV;
    };

    ULONG Status;
} AHCI_TASK_FILE_DATA;

typedef struct _AHCI_PRDT
{
    ULONG DBA;
    ULONG DBAU;
    ULONG RSV0;

    ULONG DBC : 22;
    ULONG RSV1 : 9;
    ULONG I : 1;
} AHCI_PRDT, *PAHCI_PRDT;

// 4.2.3 Command Table
typedef struct _AHCI_COMMAND_TABLE
{
    // (16 * 32) + 64 + 16 + 48 = 648
    // 128 byte aligned :D
    UCHAR CFIS[64];
    UCHAR ACMD[16];
    UCHAR RSV0[48];
    AHCI_PRDT PRDT[MAXIMUM_AHCI_PRDT_ENTRIES];
} AHCI_COMMAND_TABLE, *PAHCI_COMMAND_TABLE;

// 4.2.2 Command Header
typedef struct _AHCI_COMMAND_HEADER
{
    AHCI_COMMAND_HEADER_DESCRIPTION DI;   // DW 0
    ULONG PRDBC;                          // DW 1
    ULONG CTBA;                           // DW 2
    ULONG CTBA_U;                         // DW 3
    ULONG Reserved[4];                    // DW 4-7
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

typedef union _AHCI_INTERRUPT_ENABLE
{
    struct
    {
        ULONG DHRE :1;
        ULONG PSE :1;
        ULONG DSE :1;
        ULONG SDBE :1;
        ULONG UFE :1;
        ULONG DPE :1;
        ULONG PCE :1;
        ULONG DMPE :1;
        ULONG DW5_Reserved :14;
        ULONG PRCE :1;
        ULONG IPME :1;
        ULONG OFE :1;
        ULONG DW5_Reserved2 :1;
        ULONG INFE :1;
        ULONG IFE :1;
        ULONG HBDE :1;
        ULONG HBFE :1;
        ULONG TFEE :1;
        ULONG CPDE :1;
    };

    ULONG Status;
} AHCI_INTERRUPT_ENABLE;

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
    ULONG Reserved[0x1d];                       // 0x2C - 0x9F, Reserved
    ULONG VendorSpecific[0x18];                 // 0xA0 - 0xFF, Vendor specific registers
    AHCI_PORT PortList[MAXIMUM_AHCI_PORT_COUNT];
} AHCI_MEMORY_REGISTERS, *PAHCI_MEMORY_REGISTERS;

// Holds information for each attached attached port to a given adapter.
typedef struct _AHCI_PORT_EXTENSION
{
    ULONG PortNumber;
    ULONG QueueSlots;                                   // slots which we have already assigned task (Slot)
    ULONG CommandIssuedSlots;                           // slots which has been programmed
    ULONG MaxPortQueueDepth;

    struct
    {
        UCHAR RemovableDevice;
        UCHAR Lba48BitMode;
        UCHAR AccessType;
        UCHAR DeviceType;
        UCHAR IsActive;
        LARGE_INTEGER MaxLba;
        ULONG BytesPerLogicalSector;
        ULONG BytesPerPhysicalSector;
        UCHAR VendorId[41];
        UCHAR RevisionID[9];
        UCHAR SerialNumber[21];
    } DeviceParams;

    STOR_DPC CommandCompletion;
    PAHCI_PORT Port;                                    // AHCI Port Infomation
    AHCI_QUEUE SrbQueue;                                // pending Srbs
    AHCI_QUEUE CompletionQueue;
    PSCSI_REQUEST_BLOCK Slot[MAXIMUM_AHCI_PORT_NCS];    // Srbs which has been alloted a port
    PAHCI_RECEIVED_FIS ReceivedFIS;
    PAHCI_COMMAND_HEADER CommandList;
    STOR_DEVICE_POWER_STATE DevicePowerState;           // Device Power State
    PIDENTIFY_DEVICE_DATA IdentifyDeviceData;
    STOR_PHYSICAL_ADDRESS IdentifyDeviceDataPhysicalAddress;
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
    ULONG   PortCount;

    USHORT  VendorID;
    USHORT  DeviceID;
    USHORT  RevisionID;

    ULONG   Version;
    ULONG   CAP;
    ULONG   CAP2;
    ULONG   LastInterruptPort;
    ULONG   CurrentCommandSlot;

    PVOID NonCachedExtension; // holds virtual address to noncached buffer allocated for Port Extension

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

typedef struct _LOCAL_SCATTER_GATHER_LIST
{
    ULONG                       NumberOfElements;
    ULONG_PTR                   Reserved;
    STOR_SCATTER_GATHER_ELEMENT List[MAXIMUM_AHCI_PRDT_ENTRIES];
} LOCAL_SCATTER_GATHER_LIST, *PLOCAL_SCATTER_GATHER_LIST;

typedef struct _AHCI_SRB_EXTENSION
{
    AHCI_COMMAND_TABLE CommandTable;
    ULONG AtaFunction;
    ULONG Flags;

    UCHAR CommandReg;
    UCHAR FeaturesLow;
    UCHAR LBA0;
    UCHAR LBA1;
    UCHAR LBA2;
    UCHAR Device;
    UCHAR LBA3;
    UCHAR LBA4;
    UCHAR LBA5;
    UCHAR FeaturesHigh;

    UCHAR SectorCountLow;
    UCHAR SectorCountHigh;

    ULONG SlotIndex;
    LOCAL_SCATTER_GATHER_LIST Sgl;
    PLOCAL_SCATTER_GATHER_LIST pSgl;
    PAHCI_COMPLETION_ROUTINE CompletionRoutine;

    // for alignment purpose -- 128 byte alignment
    // do not try to access (R/W) this field
    UCHAR Reserved[128];
} AHCI_SRB_EXTENSION, *PAHCI_SRB_EXTENSION;

//////////////////////////////////////////////////////////////
//                       Declarations                       //
//////////////////////////////////////////////////////////////

VOID
AhciProcessIO (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in UCHAR PathId,
    __in PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
AhciAdapterReset (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

FORCEINLINE
VOID
AhciZeroMemory (
    __out PCHAR Buffer,
    __in ULONG BufferSize
    );

FORCEINLINE
BOOLEAN
IsPortValid (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in ULONG pathId
    );

UCHAR DeviceRequestSense (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

UCHAR DeviceRequestReadWrite (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

UCHAR DeviceRequestCapacity (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

UCHAR
DeviceInquiryRequest (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

UCHAR DeviceRequestComplete (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

UCHAR DeviceReportLuns (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    );

FORCEINLINE
BOOLEAN
AddQueue (
    __inout PAHCI_QUEUE Queue,
    __in PVOID Srb
    );

FORCEINLINE
PVOID
RemoveQueue (
    __inout PAHCI_QUEUE Queue
    );

FORCEINLINE
PAHCI_SRB_EXTENSION
GetSrbExtension(
    __in PSCSI_REQUEST_BLOCK Srb
    );

FORCEINLINE
ULONG64
AhciGetLba (
    __in PCDB Cdb,
    __in ULONG CdbLength
    );

//////////////////////////////////////////////////////////////
//                       Assertions                         //
//////////////////////////////////////////////////////////////

// I assert every silly mistake I can do while coding
// because god never help me debugging the code
// but these asserts do :')

C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, CAP)            == 0x00);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, GHC)            == 0x04);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, IS)             == 0x08);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, PI)             == 0x0C);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, VS)             == 0x10);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, CCC_CTL)        == 0x14);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, CCC_PTS)        == 0x18);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, EM_LOC)         == 0x1C);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, EM_CTL)         == 0x20);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, CAP2)           == 0x24);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, BOHC)           == 0x28);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, Reserved)       == 0x2C);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, VendorSpecific) == 0xA0);
C_ASSERT(FIELD_OFFSET(AHCI_MEMORY_REGISTERS, PortList)       == 0x100);

C_ASSERT(FIELD_OFFSET(AHCI_PORT, CLB)    == 0x00);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, CLBU)   == 0x04);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, FB)     == 0x08);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, FBU)    == 0x0C);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, IS)     == 0x10);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, IE)     == 0x14);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, CMD)    == 0x18);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, RSV0)   == 0x1C);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, TFD)    == 0x20);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SIG)    == 0x24);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SSTS)   == 0x28);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SCTL)   == 0x2C);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SERR)   == 0x30);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SACT)   == 0x34);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, CI)     == 0x38);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, SNTF)   == 0x3C);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, FBS)    == 0x40);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, RSV1)   == 0x44);
C_ASSERT(FIELD_OFFSET(AHCI_PORT, Vendor) == 0x70);

C_ASSERT((sizeof(AHCI_COMMAND_TABLE) % 128) == 0);

C_ASSERT(sizeof(AHCI_GHC)                        == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_PORT_CMD)                   == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_TASK_FILE_DATA)             == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_INTERRUPT_ENABLE)           == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_SERIAL_ATA_STATUS)          == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_SERIAL_ATA_CONTROL)         == sizeof(ULONG));
C_ASSERT(sizeof(AHCI_COMMAND_HEADER_DESCRIPTION) == sizeof(ULONG));

C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, CFIS) == 0x00);
C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, ACMD) == 0x40);
C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, RSV0) == 0x50);
C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, PRDT) == 0x80);
