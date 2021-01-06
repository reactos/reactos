/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    ide.h

Abstract:

    These are the structures and defines that are used in the
    PCI IDE mini drivers.

Revision History:

--*/


#if !defined (___ide_h___)
#define ___ide_h___

#include "ideuser.h"

#define MAX_IDE_DEVICE      2
#define MAX_IDE_LINE        2
#define MAX_IDE_CHANNEL     2

//
// Some miniports need this structure.
// IdentifyData is passed to the miniport in
// the XfermodeSelect structure
// 

//
// IDENTIFY data
//

#pragma pack (1)
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumCylinders;                    // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumHeads;                        // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT NumSectorsPerTrack;              // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    UCHAR  SerialNumber[20];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    UCHAR  FirmwareRevision[8];             // 2E  23-26
    UCHAR  ModelNumber[40];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:3;        // 6A  53
    USHORT Reserved3:13;
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
    USHORT Reserved5[11];                   //     69-79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81
    USHORT Reserved6;                       //     82
    USHORT CommandSetSupport;               //     83
    USHORT Reserved6a[2];                   //     84-85
    USHORT CommandSetActive;                //     86
    USHORT Reserved6b;                      //     87
    USHORT UltraDMASupport : 8;             //     88
    USHORT UltraDMAActive  : 8;             //
    USHORT Reserved7[11];                   //     89-99
    ULONG  Max48BitLBA[2];                  //     100-103
    USHORT Reserved7a[22];                  //     104-125
    USHORT LastLun:3;                       //     126
    USHORT Reserved8:13;
    USHORT MediaStatusNotification:2;       //     127
    USHORT Reserved9:6;
    USHORT DeviceWriteProtect:1;
    USHORT Reserved10:7;
    USHORT Reserved11[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// Identify data without the Reserved4.
//

//typedef struct _IDENTIFY_DATA2 {
//    USHORT GeneralConfiguration;            // 00 00
//    USHORT NumCylinders;                    // 02  1
//    USHORT Reserved1;                       // 04  2
//    USHORT NumHeads;                        // 06  3
//    USHORT UnformattedBytesPerTrack;        // 08  4
//    USHORT UnformattedBytesPerSector;       // 0A  5
//    USHORT NumSectorsPerTrack;              // 0C  6
//    USHORT VendorUnique1[3];                // 0E  7-9
//    UCHAR  SerialNumber[20];                // 14  10-19
//    USHORT BufferType;                      // 28  20
//    USHORT BufferSectorSize;                // 2A  21
//    USHORT NumberOfEccBytes;                // 2C  22
//    UCHAR  FirmwareRevision[8];             // 2E  23-26
//    UCHAR  ModelNumber[40];                 // 36  27-46
//    UCHAR  MaximumBlockTransfer;            // 5E  47
//    UCHAR  VendorUnique2;                   // 5F
//    USHORT DoubleWordIo;                    // 60  48
//    USHORT Capabilities;                    // 62  49
//    USHORT Reserved2;                       // 64  50
//    UCHAR  VendorUnique3;                   // 66  51
//    UCHAR  PioCycleTimingMode;              // 67
//    UCHAR  VendorUnique4;                   // 68  52
//    UCHAR  DmaCycleTimingMode;              // 69
//    USHORT TranslationFieldsValid:3;        // 6A  53
//    USHORT Reserved3:13;
//    USHORT NumberOfCurrentCylinders;        // 6C  54
//    USHORT NumberOfCurrentHeads;            // 6E  55
//    USHORT CurrentSectorsPerTrack;          // 70  56
//    ULONG  CurrentSectorCapacity;           // 72  57-58
//    USHORT CurrentMultiSectorSetting;       //     59
//    ULONG  UserAddressableSectors;          //     60-61
//    USHORT SingleWordDMASupport : 8;        //     62
//    USHORT SingleWordDMAActive : 8;
//    USHORT MultiWordDMASupport : 8;         //     63
//    USHORT MultiWordDMAActive : 8;
//    USHORT AdvancedPIOModes : 8;            //     64
//    USHORT Reserved4 : 8;
//    USHORT MinimumMWXferCycleTime;          //     65
//    USHORT RecommendedMWXferCycleTime;      //     66
//    USHORT MinimumPIOCycleTime;             //     67
//    USHORT MinimumPIOCycleTimeIORDY;        //     68
//    USHORT Reserved5[11];                   //     69-79
//    USHORT MajorRevision;                   //     80
//    USHORT MinorRevision;                   //     81
//    USHORT Reserved6[6];                    //     82-87
//    USHORT UltraDMASupport : 8;             //     88
//    USHORT UltraDMAActive  : 8;             //
//    USHORT Reserved7[37];                   //     89-125
//    USHORT LastLun:3;                       //     126
//    USHORT Reserved8:13;
//    USHORT MediaStatusNotification:2;       //     127
//    USHORT Reserved9:6;
//    USHORT DeviceWriteProtect:1;
//    USHORT Reserved10:7;
//} IDENTIFY_DATA2, *PIDENTIFY_DATA2;
#pragma pack ()

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)


//
// The structure is passed to pci ide mini driver
// TransferModeSelect callback for selecting
// proper transfer mode the the devices connected
// to the given IDE channel
//
typedef struct _PCIIDE_TRANSFER_MODE_SELECT {

    //
    // Input Parameters
    //          
          
    //
    // IDE Channel Number.  0 or 1
    //                                       
    ULONG   Channel;

    //
    // Indicate whether devices are present
    //                                  
    BOOLEAN DevicePresent[MAX_IDE_DEVICE * MAX_IDE_LINE];
    
    //
    // Indicate whether devices are ATA harddisk
    //
    BOOLEAN FixedDisk[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicate whether devices support IO Ready Line
    //                                                
    BOOLEAN IoReadySupported[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicate the data transfer modes devices support
    //               
    ULONG DeviceTransferModeSupported[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicate devices' best timings for PIO, single word DMA,
    // multiword DMA, and Ultra DMA modes
    //
    ULONG BestPioCycleTime[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG BestSwDmaCycleTime[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG BestMwDmaCycleTime[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG BestUDmaCycleTime[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicate devices' current data transfer modes
    //
    ULONG DeviceTransferModeCurrent[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // The user's choice. This will allow pciidex to
    // default to a transfer mode indicated by the mini driver
    //
    ULONG UserChoiceTransferMode[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // This enables UDMA66 on the intel chipsets
    //
    ULONG EnableUDMA66;

    //
    //Some miniports need this
    // The miniport will save this data in their deviceExtension
    //
    IDENTIFY_DATA IdentifyData[MAX_IDE_DEVICE];


    //
    // Output Parameters
    //          
                      
    //
    // Indicate devices' data transfer modes chosen by
    // the pcii ide mini drive
    //
    ULONG DeviceTransferModeSelected[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Transfermode timings
    //
    PULONG TransferModeTimingTable;
    ULONG   TransferModeTableLength;

} PCIIDE_TRANSFER_MODE_SELECT, *PPCIIDE_TRANSFER_MODE_SELECT;

//
// possible ide channel state
//         
        
typedef enum {
    ChannelDisabled = 0,
    ChannelEnabled,
    ChannelStateUnknown
} IDE_CHANNEL_STATE;
                         
   
//
// Prototype for different PCI IDE mini driver
// callbacks
//   
typedef IDE_CHANNEL_STATE
    (*PCIIDE_CHANNEL_ENABLED) (
        IN PVOID DeviceExtension,
        IN ULONG Channel
        );

typedef BOOLEAN 
    (*PCIIDE_SYNC_ACCESS_REQUIRED) (
        IN PVOID DeviceExtension
        );

typedef NTSTATUS
    (*PCIIDE_TRANSFER_MODE_SELECT_FUNC) (
        IN     PVOID                     DeviceExtension,
        IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect
        );

typedef    ULONG  
    (*PCIIDE_USEDMA_FUNC)(
        IN PVOID deviceExtension, 
        IN PVOID cdbCmd,
        IN UCHAR targetID
        ); 

typedef    NTSTATUS
    (*PCIIDE_UDMA_MODES_SUPPORTED) (
        IDENTIFY_DATA   IdentifyData,
        PULONG          BestXferMode,
        PULONG          CurrentMode
        );
// 
// This structure is for the PCI IDE mini driver to 
// return its properties
// 
typedef struct _IDE_CONTROLLER_PROPERTIES {

    //
    // sizeof (IDE_CONTROLLER_PROPERTIES)
    //
    ULONG Size;      
    
    //
    // Indicate the amount of memory PCI IDE mini driver
    // needs for its private data
    //
    ULONG ExtensionSize;

    //
    // Indicate all the data transfer modes the PCI IDE
    // controller supports
    //                                 
    ULONG SupportedTransferMode[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];

    //
    // callback to query whether a IDE channel is enabled
    //                          
    PCIIDE_CHANNEL_ENABLED      PciIdeChannelEnabled;
    
    //
    // callback to query whether both IDE channels requires
    // synchronized access.  (one channel at a time)
    //                                                              
    PCIIDE_SYNC_ACCESS_REQUIRED PciIdeSyncAccessRequired;
    
    //
    // callback to select proper transfer modes for the
    // given devices
    //
    PCIIDE_TRANSFER_MODE_SELECT_FUNC PciIdeTransferModeSelect;

    //
    // at the end of a ATA data transfer, ignores busmaster 
    // status active bit.  Normally, it should be FALSE
    //                    
    BOOLEAN IgnoreActiveBitForAtaDevice;

    //
    // always clear the busmaster interrupt on every interrupt
    // generated by the device.  Normally, it should be FALSE
    //
    BOOLEAN AlwaysClearBusMasterInterrupt;

    //
    // callback to determine whether DMA should be used or not
    // called for every IO
    //
    PCIIDE_USEDMA_FUNC PciIdeUseDma;


    //
    // if the miniport needs a different alignment
    //
    ULONG AlignmentRequirement;

    ULONG DefaultPIO;

    //
    // retrieves the supported udma modes from the Identify data
    //
    PCIIDE_UDMA_MODES_SUPPORTED PciIdeUdmaModesSupported;

} IDE_CONTROLLER_PROPERTIES, *PIDE_CONTROLLER_PROPERTIES;

//
// callback to query PCI IDE controller properties
//                            
typedef
NTSTATUS (*PCONTROLLER_PROPERTIES) (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    );

                     
//
// To initialize PCI IDE mini driver
//                     
NTSTATUS
PciIdeXInitialize(
    IN PDRIVER_OBJECT           DriverObject,
    IN PUNICODE_STRING          RegistryPath,
    IN PCONTROLLER_PROPERTIES   PciIdeGetControllerProperties,
    IN ULONG                    ExtensionSize
    );

//
// To query PCI IDE config space data
//                                    
NTSTATUS
PciIdeXGetBusData(
    IN PVOID DeviceExtension,
    IN PVOID Buffer,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );

//
// To save PCI IDE config space data
//                                    
NTSTATUS
PciIdeXSetBusData(
    IN PVOID DeviceExtension,
    IN PVOID Buffer,
    IN PVOID DataMask,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );

                     
#pragma pack(1)
typedef struct _PCIIDE_CONFIG_HEADER {

    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)

    //
    //  Command
    //
    union {

        struct {

            USHORT  IoAccessEnable:1;           // Device control
            USHORT  MemAccessEnable:1;
            USHORT  MasterEnable:1;
            USHORT  SpecialCycle:1;
            USHORT  MemWriteInvalidateEnable:1;
            USHORT  VgaPaletteSnoopEnable:1;
            USHORT  ParityErrorResponse:1;
            USHORT  WaitCycleEnable:1;
            USHORT  SystemErrorEnable:1;
            USHORT  FastBackToBackEnable:1;
            USHORT  CommandReserved:6;
        } b;

        USHORT w;

    } Command;


    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)

    //
    //  Program Interface
    //
    UCHAR   Chan0OpMode:1;
    UCHAR   Chan0Programmable:1;
    UCHAR   Chan1OpMode:1;
    UCHAR   Chan1Programmable:1;
    UCHAR   ProgIfReserved:3;
    UCHAR   MasterIde:1;

    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    struct _PCI_HEADER_TYPE_0 type0;

} PCIIDE_CONFIG_HEADER, *PPCIIDE_CONFIG_HEADER;
#pragma pack()
                     
//
// Debug Print
//                        
#if DBG

VOID
PciIdeXDebugPrint(
    _In_ ULONG DebugPrintLevel,
    _In_z_ _Printf_format_string_ PCCHAR DebugMessage,
    ...
    );

#define PciIdeXDebugPrint(x)    PciIdeXDebugPrint x
    
#else
    
#define PciIdeXDebugPrint(x)    

#endif // DBG
                     
#endif // ___ide_h___

