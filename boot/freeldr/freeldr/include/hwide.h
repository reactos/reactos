/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA/ATAPI programmed I/O driver header file.
 * COPYRIGHT:   Copyright 2019-2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* GLOBALS ********************************************************************/

/* Some definitions were taken from UniATA driver by Alter */

/*
 * IDE registers offsets
 */
#if defined(SARCH_PC98)
#define IDX_IO1_i_Data          0x00
#define IDX_IO1_i_Error         0x02
#define IDX_IO1_i_BlockCount    0x04
#define IDX_IO1_i_BlockNumber   0x06
#define IDX_IO1_i_CylinderLow   0x08
#define IDX_IO1_i_CylinderHigh  0x0A
#define IDX_IO1_i_DriveSelect   0x0C
#define IDX_IO1_i_Status        0x0E

#define IDX_IO2_i_AltStatus     0x10C
#define IDX_IO2_i_DriveAddress  0x10E
#define IDE_IO_i_Bank           0x432

#define IDX_IO1_o_Data          0x00
#define IDX_IO1_o_Feature       0x02
#define IDX_IO1_o_BlockCount    0x04
#define IDX_IO1_o_BlockNumber   0x06
#define IDX_IO1_o_CylinderLow   0x08
#define IDX_IO1_o_CylinderHigh  0x0A
#define IDX_IO1_o_DriveSelect   0x0C
#define IDX_IO1_o_Command       0x0E

#define IDX_IO2_o_Control       0x10C
#define IDE_IO_o_BankSelect     0x432
#else /* SARCH_PC98 */
#define IDX_IO1_i_Data          0x00
#define IDX_IO1_i_Error         0x01
#define IDX_IO1_i_BlockCount    0x02
#define IDX_IO1_i_BlockNumber   0x03
#define IDX_IO1_i_CylinderLow   0x04
#define IDX_IO1_i_CylinderHigh  0x05
#define IDX_IO1_i_DriveSelect   0x06
#define IDX_IO1_i_Status        0x07

#define IDX_IO2_i_AltStatus     0x206
#define IDX_IO2_i_DriveAddress  0x207

#define IDX_IO1_o_Data          0x00
#define IDX_IO1_o_Feature       0x01
#define IDX_IO1_o_BlockCount    0x02
#define IDX_IO1_o_BlockNumber   0x03
#define IDX_IO1_o_CylinderLow   0x04
#define IDX_IO1_o_CylinderHigh  0x05
#define IDX_IO1_o_DriveSelect   0x06
#define IDX_IO1_o_Command       0x07

#define IDX_IO2_o_Control       0x206
#endif

/*
 * ATAPI registers offsets
 */
#if defined(SARCH_PC98)
#define IDX_ATAPI_IO1_i_Data              0x00
#define IDX_ATAPI_IO1_i_Error             0x02
#define IDX_ATAPI_IO1_i_InterruptReason   0x04
#define IDX_ATAPI_IO1_i_Unused1           0x06
#define IDX_ATAPI_IO1_i_ByteCountLow      0x08
#define IDX_ATAPI_IO1_i_ByteCountHigh     0x0A
#define IDX_ATAPI_IO1_i_DriveSelect       0x0C
#define IDX_ATAPI_IO1_i_Status            0x0E

#define IDX_ATAPI_IO1_o_Data              0x00
#define IDX_ATAPI_IO1_o_Feature           0x02
#define IDX_ATAPI_IO1_o_Unused0           0x04
#define IDX_ATAPI_IO1_o_Unused1           0x06
#define IDX_ATAPI_IO1_o_ByteCountLow      0x08
#define IDX_ATAPI_IO1_o_ByteCountHigh     0x0A
#define IDX_ATAPI_IO1_o_DriveSelect       0x0C
#define IDX_ATAPI_IO1_o_Command           0x0E
#else /* SARCH_PC98 */
#define IDX_ATAPI_IO1_i_Data              0x00
#define IDX_ATAPI_IO1_i_Error             0x01
#define IDX_ATAPI_IO1_i_InterruptReason   0x02
#define IDX_ATAPI_IO1_i_Unused1           0x03
#define IDX_ATAPI_IO1_i_ByteCountLow      0x04
#define IDX_ATAPI_IO1_i_ByteCountHigh     0x05
#define IDX_ATAPI_IO1_i_DriveSelect       0x06
#define IDX_ATAPI_IO1_i_Status            0x07

#define IDX_ATAPI_IO1_o_Data              0x00
#define IDX_ATAPI_IO1_o_Feature           0x01
#define IDX_ATAPI_IO1_o_Unused0           0x02
#define IDX_ATAPI_IO1_o_Unused1           0x03
#define IDX_ATAPI_IO1_o_ByteCountLow      0x04
#define IDX_ATAPI_IO1_o_ByteCountHigh     0x05
#define IDX_ATAPI_IO1_o_DriveSelect       0x06
#define IDX_ATAPI_IO1_o_Command           0x07
#endif

/*
 * IDE status definitions
 */
#define IDE_STATUS_SUCCESS           0x00
#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DMA               0x20      /* DMA ready */
#define IDE_STATUS_DWF               0x20      /* drive write fault */
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

#define IDE_STATUS_WRONG             0xff
#define IDE_STATUS_MASK              0xff

/*
 * IDE drive select/head definitions
 */
#define IDE_DRIVE_SELECT             0xA0
#define IDE_DRIVE_1                  0x00
#define IDE_DRIVE_2                  0x10
#define IDE_DRIVE_SELECT_1           (IDE_DRIVE_SELECT | IDE_DRIVE_1)
#define IDE_DRIVE_SELECT_2           (IDE_DRIVE_SELECT | IDE_DRIVE_2)
#define IDE_DRIVE_MASK               (IDE_DRIVE_SELECT_1 | IDE_DRIVE_SELECT_2)
#define IDE_USE_LBA                  0x40

/*
 * IDE drive control definitions
 */
#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_A_4BIT                0x80
#define IDE_DC_USE_HOB               0x80 // use high-order byte(s)
#define IDE_DC_REENABLE_CONTROLLER   0x00

/*
 * IDE error definitions
 */
#define IDE_ERROR_ICRC               0x80
#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_DATA_ERROR         0x40
#define IDE_ERROR_MEDIA_CHANGE       0x20
#define IDE_ERROR_ID_NOT_FOUND       0x10
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_NO_MEDIA           0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01

/*
 * Values for TransferMode
 */
#define ATA_PIO                      0x00

/*
 * IDENTIFY data
 */
#include <pshpack1.h>
typedef struct _IDENTIFY_DATA
{
    UCHAR  AtapiCmdSize:2;                  // 00 00 General configuration
    UCHAR  Unused1:3;
    UCHAR  DrqType:2;
    UCHAR  Removable:1;
    UCHAR  DeviceType:5;
    UCHAR  Unused2:1;
    UCHAR  CmdProtocol:2;
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
    UCHAR  ReadWriteMultipleSupport;        // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Reserved62_0:8;                  // 62  49 Capabilities
    USHORT SupportDma:1;
    USHORT SupportLba:1;
    USHORT DisableIordy:1;
    USHORT SupportIordy:1;
    USHORT SoftReset:1;
    USHORT StandbyOverlap:1;
    USHORT SupportQTag:1;
    USHORT SupportIDma:1;
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
    USHORT SingleWordDMASupport:8;          //     62
    USHORT SingleWordDMAActive:8;
    USHORT MultiWordDMASupport:8;           //     63
    USHORT MultiWordDMAActive:8;
    USHORT AdvancedPIOModes:8;              //     64
    USHORT Reserved4:8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT Reserved73_74[2];                //     73-74
    USHORT QueueLength:5;                   //     75
    USHORT Reserved75_6:11;
    USHORT SataCapabilities;                //     76
    USHORT Reserved77;                      //     77
    USHORT SataSupport;                     //     78
    USHORT SataEnable;                      //     79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81
    struct {
        USHORT Smart:1;                     //     82
        USHORT Security:1;
        USHORT Removable:1;
        USHORT PowerMngt:1;
        USHORT Packet:1;
        USHORT WriteCache:1;
        USHORT LookAhead:1;
        USHORT ReleaseDRQ:1;
        USHORT ServiceDRQ:1;
        USHORT Reset:1;
        USHORT Protected:1;
        USHORT Reserved_82_11:1;
        USHORT WriteBuffer:1;
        USHORT ReadBuffer:1;
        USHORT Nop:1;
        USHORT Reserved_82_15:1;
        USHORT Microcode:1;                 //     83/86
        USHORT Queued:1;
        USHORT CFA:1;
        USHORT APM:1;
        USHORT Notify:1;
        USHORT Standby:1;
        USHORT Spinup:1;
        USHORT Reserver_83_7:1;
        USHORT MaxSecurity:1;
        USHORT AutoAcoustic:1;
        USHORT Address48:1;
        USHORT ConfigOverlay:1;
        USHORT FlushCache:1;
        USHORT FlushCache48:1;
        USHORT SupportOne:1;
        USHORT SupportZero:1;
        USHORT SmartErrorLog:1;             //     84/87
        USHORT SmartSelfTest:1;
        USHORT MediaSerialNo:1;
        USHORT MediaCardPass:1;
        USHORT Streaming:1;
        USHORT Logging:1;
        USHORT Reserver_84_6:8;
        USHORT ExtendedOne:1;
        USHORT ExtendedZero:1;
    } FeaturesSupport, FeaturesEnabled;
    USHORT Reserved6[13];                   //     88-99
    ULONGLONG UserAddressableSectors48;     //     100-103
    USHORT Reserved7[151];                  //     104-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;
#include <poppack.h>
#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)

#define ATAPI_MAGIC_LSB         0x14
#define ATAPI_MAGIC_MSB         0xEB
#define MAXIMUM_CDROM_SIZE      804

typedef struct _DEVICE_UNIT
{
    UCHAR         Channel;
    UCHAR         DeviceNumber;
    ULONG         Cylinders;
    ULONG         Heads;
    ULONG         Sectors;
    ULONG         SectorSize;
    ULONGLONG     TotalSectors; /* This number starts from 0 */
    USHORT        Flags;
    IDENTIFY_DATA IdentifyData;
} DEVICE_UNIT, *PDEVICE_UNIT;

#define ATA_DEVICE_ATAPI        (1 << 0)
#define ATA_DEVICE_NO_MEDIA     (1 << 1)
#define ATA_DEVICE_NOT_READY    (1 << 2)
#define ATA_DEVICE_LBA48        (1 << 3)
#define ATA_DEVICE_LBA          (1 << 4)
#define ATA_DEVICE_CHS          (1 << 5)

/* PROTOTYPES ****************************************************************/

BOOLEAN
AtaInit(
    OUT PUCHAR DetectedCount
);

VOID
AtaFree();

PDEVICE_UNIT
AtaGetDevice(
    IN UCHAR UnitNumber
);

BOOLEAN
AtaAtapiReadLogicalSectorsLBA(
    IN OUT PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer
);
