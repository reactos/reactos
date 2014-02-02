/*
 * PROJECT:         ReactOS Storage Stack
 * LICENSE:         DDK - see license.txt in the root dir
 * FILE:            drivers/storage/atapi/atapi.h
 * PURPOSE:         ATAPI IDE miniport driver
 * PROGRAMMERS:     Based on a source code sample from Microsoft NT4 DDK
 */

#include <srb.h>
#include <scsi.h>

//
// IDE register definition
//

typedef struct _IDE_REGISTERS_1 {
    USHORT Data;
    UCHAR BlockCount;
    UCHAR BlockNumber;
    UCHAR CylinderLow;
    UCHAR CylinderHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

typedef struct _IDE_REGISTERS_2 {
    UCHAR AlternateStatus;
    UCHAR DriveAddress;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;

typedef struct _IDE_REGISTERS_3 {
    ULONG Data;
    UCHAR Others[4];
} IDE_REGISTERS_3, *PIDE_REGISTERS_3;

//
// Device Extension Device Flags
//

#define DFLAGS_DEVICE_PRESENT        0x0001    // Indicates that some device is present.
#define DFLAGS_ATAPI_DEVICE          0x0002    // Indicates whether Atapi commands can be used.
#define DFLAGS_TAPE_DEVICE           0x0004    // Indicates whether this is a tape device.
#define DFLAGS_INT_DRQ               0x0008    // Indicates whether device interrupts as DRQ is set after
                                               // receiving Atapi Packet Command
#define DFLAGS_REMOVABLE_DRIVE       0x0010    // Indicates that the drive has the 'removable' bit set in
                                               // identify data (offset 128)
#define DFLAGS_MEDIA_STATUS_ENABLED  0x0020    // Media status notification enabled
#define DFLAGS_ATAPI_CHANGER         0x0040    // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER   0x0080    // Indicates multi-platter device, not conforming to the 2.5 spec.
#define DFLAGS_CHANGER_INITED        0x0100    // Indicates that the init path for changers has already been done.
//
// Used to disable 'advanced' features.
//

#define MAX_ERRORS                     4

//
// ATAPI command definitions
//

#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_FORMAT_UNIT  0x24

//
// ATAPI Command Descriptor Block
//

typedef struct _MODE_SENSE_10 {
        UCHAR OperationCode;
        UCHAR Reserved1;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved3[3];
} MODE_SENSE_10, *PMODE_SENSE_10;

typedef struct _MODE_SELECT_10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR PFBit : 1;
        UCHAR Reserved2 : 3;
        UCHAR Reserved3[5];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved4[3];
} MODE_SELECT_10, *PMODE_SELECT_10;

typedef struct _MODE_PARAMETER_HEADER_10 {
    UCHAR ModeDataLengthMsb;
    UCHAR ModeDataLengthLsb;
    UCHAR MediumType;
    UCHAR Reserved[5];
}MODE_PARAMETER_HEADER_10, *PMODE_PARAMETER_HEADER_10;

//
// IDE command definitions
//

#define IDE_COMMAND_ATAPI_RESET      0x08
#define IDE_COMMAND_RECALIBRATE      0x10
#define IDE_COMMAND_READ             0x20
#define IDE_COMMAND_WRITE            0x30
#define IDE_COMMAND_VERIFY           0x40
#define IDE_COMMAND_SEEK             0x70
#define IDE_COMMAND_SET_DRIVE_PARAMETERS 0x91
#define IDE_COMMAND_ATAPI_PACKET     0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY   0xA1
#define IDE_COMMAND_READ_MULTIPLE    0xC4
#define IDE_COMMAND_WRITE_MULTIPLE   0xC5
#define IDE_COMMAND_SET_MULTIPLE     0xC6
#define IDE_COMMAND_READ_DMA         0xC8
#define IDE_COMMAND_WRITE_DMA             0xCA
#define IDE_COMMAND_GET_MEDIA_STATUS      0xDA
#define IDE_COMMAND_ENABLE_MEDIA_STATUS   0xEF
#define IDE_COMMAND_IDENTIFY              0xEC
#define IDE_COMMAND_MEDIA_EJECT           0xED

//
// IDE status definitions
//

#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

//
// IDE drive select/head definitions
//

#define IDE_DRIVE_SELECT_1           0xA0
#define IDE_DRIVE_SELECT_2           0x10

//
// IDE drive control definitions
//

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00

//
// IDE error definitions
//

#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_DATA_ERROR         0x40
#define IDE_ERROR_MEDIA_CHANGE       0x20
#define IDE_ERROR_ID_NOT_FOUND       0x10
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01

//
// ATAPI register definition
//

typedef struct _ATAPI_REGISTERS_1 {
    USHORT Data;
    UCHAR InterruptReason;
    UCHAR Unused1;
    UCHAR ByteCountLow;
    UCHAR ByteCountHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

typedef struct _ATAPI_REGISTERS_2 {
    UCHAR AlternateStatus;
    UCHAR DriveAddress;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;

//
// ATAPI interrupt reasons
//

#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

//
// IDENTIFY data
//

typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// Identify data without the Reserved4.
//

typedef struct _IDENTIFY_DATA2 {
    USHORT GeneralConfiguration;            // 00
    USHORT NumberOfCylinders;               // 02
    USHORT Reserved1;                       // 04
    USHORT NumberOfHeads;                   // 06
    USHORT UnformattedBytesPerTrack;        // 08
    USHORT UnformattedBytesPerSector;       // 0A
    USHORT SectorsPerTrack;                 // 0C
    USHORT VendorUnique1[3];                // 0E
    USHORT SerialNumber[10];                // 14
    USHORT BufferType;                      // 28
    USHORT BufferSectorSize;                // 2A
    USHORT NumberOfEccBytes;                // 2C
    USHORT FirmwareRevision[4];             // 2E
    USHORT ModelNumber[20];                 // 36
    UCHAR  MaximumBlockTransfer;            // 5E
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60
    USHORT Capabilities;                    // 62
    USHORT Reserved2;                       // 64
    UCHAR  VendorUnique3;                   // 66
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C
    USHORT NumberOfCurrentHeads;            // 6E
    USHORT CurrentSectorsPerTrack;          // 70
    ULONG  CurrentSectorCapacity;           // 72
} IDENTIFY_DATA2, *PIDENTIFY_DATA2;

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)

//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED 0x0100
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED 0x0200

//
// IDENTIFY DMA timing cycle modes.
//

#define IDENTIFY_DMA_CYCLES_MODE_0 0x00
#define IDENTIFY_DMA_CYCLES_MODE_1 0x01
#define IDENTIFY_DMA_CYCLES_MODE_2 0x02


typedef struct _BROKEN_CONTROLLER_INFORMATION {
    PCHAR   VendorId;
    ULONG   VendorIdLength;
    PCHAR   DeviceId;
    ULONG   DeviceIdLength;
}BROKEN_CONTROLLER_INFORMATION, *PBROKEN_CONTROLLER_INFORMATION;

BROKEN_CONTROLLER_INFORMATION const BrokenAdapters[] = {
    { "1095", 4, "0640", 4},
    { "1039", 4, "0601", 4}
};

#define BROKEN_ADAPTERS (sizeof(BrokenAdapters) / sizeof(BROKEN_CONTROLLER_INFORMATION))

typedef struct _NATIVE_MODE_CONTROLLER_INFORMATION {
    PCHAR   VendorId;
    ULONG   VendorIdLength;
    PCHAR   DeviceId;
    ULONG   DeviceIdLength;
}NATIVE_MODE_CONTROLLER_INFORMATION, *PNATIVE_MODE_CONTROLLER_INFORMATION;

NATIVE_MODE_CONTROLLER_INFORMATION const NativeModeAdapters[] = {
    { "10ad", 4, "0105", 4}
};
#define NUM_NATIVE_MODE_ADAPTERS (sizeof(NativeModeAdapters) / sizeof(NATIVE_MODE_CONTROLLER_INFORMATION))

//
// Beautification macros
//

#define GetStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->AlternateStatus);

#define GetBaseStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->Command);

#define WriteCommand(BaseIoAddress, Command) \
    ScsiPortWritePortUchar(&BaseIoAddress->Command, Command);



#define ReadBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUshort(&BaseIoAddress->Data, \
                                 Buffer, \
                                 Count);

#define WriteBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUshort(&BaseIoAddress->Data, \
                                  Buffer, \
                                  Count);

#define ReadBuffer2(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUlong(&BaseIoAddress->Data, \
                             Buffer, \
                             Count);

#define WriteBuffer2(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUlong(&BaseIoAddress->Data, \
                              Buffer, \
                              Count);

#define WaitOnBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(150); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WaitOnBaseBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetBaseStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(150); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WaitForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<1000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(200); \
        } \
    } \
}


#define WaitShortForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<2; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(100); \
        } \
    } \
}

#define AtapiSoftReset(BaseIoAddress,DeviceNumber) \
{\
    UCHAR statusByte; \
    ULONG i = 1000*1000;\
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)(((DeviceNumber & 0x1) << 4) | 0xA0)); \
    ScsiPortStallExecution(500);\
    ScsiPortWritePortUchar(&BaseIoAddress->Command, IDE_COMMAND_ATAPI_RESET); \
    while ((ScsiPortReadPortUchar(&BaseIoAddress->Command) & IDE_STATUS_BUSY) && i--)\
        ScsiPortStallExecution(30);\
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)((DeviceNumber << 4) | 0xA0)); \
    WaitOnBusy( ((PIDE_REGISTERS_2)((PUCHAR)BaseIoAddress + 0x206)), statusByte); \
    ScsiPortStallExecution(500);\
}

#define IdeHardReset(BaseIoAddress,result) \
{\
    UCHAR statusByte;\
    ULONG i;\
    ScsiPortWritePortUchar(&BaseIoAddress->AlternateStatus,IDE_DC_RESET_CONTROLLER );\
    ScsiPortStallExecution(50 * 1000);\
    ScsiPortWritePortUchar(&BaseIoAddress->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);\
    for (i = 0; i < 1000 * 1000; i++) {\
        statusByte = ScsiPortReadPortUchar(&BaseIoAddress->AlternateStatus);\
        if (statusByte != IDE_STATUS_IDLE && statusByte != 0x0) {\
            ScsiPortStallExecution(5);\
        } else {\
            break;\
        }\
    }\
    if (i == 1000*1000) {\
        result = FALSE;\
    }\
    result = TRUE;\
}

#define IS_RDP(OperationCode)\
    ((OperationCode == SCSIOP_ERASE)||\
    (OperationCode == SCSIOP_LOAD_UNLOAD)||\
    (OperationCode == SCSIOP_LOCATE)||\
    (OperationCode == SCSIOP_REWIND) ||\
    (OperationCode == SCSIOP_SPACE)||\
    (OperationCode == SCSIOP_SEEK)||\
    (OperationCode == SCSIOP_WRITE_FILEMARKS))

