#ifndef _SETUPBLK_
#define _SETUPBLK_

//
// Type of file described in DETECTED_DEVICE_FILE
//
typedef enum
{
    HwFileDriver,
    HwFilePort,
    HwFileClass,
    HwFileInf,
    HwFileDll,
    HwFileDetect,
    HwFileHal,
    HwFileCatalog,
    HwFileMax,
    HwFileDynUpdt = 31,
} HwFileType;

//
// Hardware ID for the detected device
//
typedef struct _PNP_HARDWARE_ID
{
    struct _PNP_HARDWARE_ID *Next;
    PCHAR Id;
    PCHAR DriverName;
    PCHAR ClassGuid;
} PNP_HARDWARE_ID, *PPNP_HARDWARE_ID;

//
// Structures for detected device data (file/registry)
//
typedef struct _DETECTED_DEVICE_REGISTRY
{
    struct _DETECTED_DEVICE_REGISTRY *Next;
    PCHAR KeyName;
    PCHAR ValueName;
    ULONG ValueType;
    PVOID Buffer;
    ULONG BufferSize;
} DETECTED_DEVICE_REGISTRY, *PDETECTED_DEVICE_REGISTRY;

typedef struct _DETECTED_DEVICE_FILE
{
    struct _DETECTED_DEVICE_FILE *Next;
    PCHAR FileName;
    HwFileType FileType;
    PCHAR ConfigName;
    PDETECTED_DEVICE_REGISTRY RegistryValueList;
    PCHAR DiskDescription;
    PCHAR DiskTagfile;
    PCHAR Directory;
    PCHAR ArcDeviceName;
} DETECTED_DEVICE_FILE, *PDETECTED_DEVICE_FILE;

//
// Structure for any detected device
//
typedef struct _DETECTED_DEVICE
{
    struct _DETECTED_DEVICE *Next;
    PCHAR IdString;
    ULONG Ordinal;
    PCHAR Description;
    BOOLEAN ThirdPartyOptionSelected;
    ULONG FileTypeBits;
    PDETECTED_DEVICE_FILE Files;
    PCHAR BasedllName;
    BOOLEAN MigratedDriver;
    PPNP_HARDWARE_ID HardwareIds;
} DETECTED_DEVICE, *PDETECTED_DEVICE;

typedef struct _DETECTED_OEM_SOURCE_DEVICE
{
    struct _DETECTED_OEM_SOURCE_DEVICE *Next;
    PCHAR ArcDeviceName;
    ULONG ImageBase;
    ULONGLONG ImageSize;
} DETECTED_OEM_SOURCE_DEVICE, *PDETECTED_OEM_SOURCE_DEVICE;

//
// Setup Loader Parameter Block
//
typedef struct _SETUP_LOADER_BLOCK_SCALARS
{
    ULONG SetupOperation;
    union
    {
        struct
        {
            UCHAR SetupFromCdRom:1;
            UCHAR LoadedScsi:1;
            UCHAR LoadedFloppyDrivers:1;
            UCHAR LoadedDiskDrivers:1;
            UCHAR LoadedCdRomDrivers:1;
            UCHAR LoadedFileSystems:1;
        };
        ULONG AsULong;
    };
} SETUP_LOADER_BLOCK_SCALARS, *PSETUP_LOADER_BLOCK_SCALARS;

typedef struct _SETUP_LOADER_BLOCK
{
    PCHAR ArcSetupDeviceName;
    DETECTED_DEVICE VideoDevice;
    PDETECTED_DEVICE KeyboardDevices;
    DETECTED_DEVICE ComputerDevice;
    PDETECTED_DEVICE ScsiDevices;
    PDETECTED_OEM_SOURCE_DEVICE OemSourceDevices;
    SETUP_LOADER_BLOCK_SCALARS ScalarValues;
    PCHAR IniFile;
    ULONG IniFileLength;
    PCHAR WinntSifFile;
    ULONG WinntSifFileLength;
    PCHAR MigrateInfFile;
    ULONG MigrateInfFileLength;
    PCHAR UnsupDriversInfFile;
    ULONG UnsupDriversInfFileLength;
    PVOID BootFontFile;
    ULONG BootFontFileLength;
    MONITOR_CONFIGURATION_DATA Monitor;
    PCHAR MonitorId;
    PDETECTED_DEVICE BootBusExtenders;
    PDETECTED_DEVICE BusExtenders;
    PDETECTED_DEVICE InputDevicesSupport;
    PPNP_HARDWARE_ID HardwareIdDatabase;
    WCHAR ComputerName[64];
    ULONG IpAddress;
    ULONG SubnetMask;
    ULONG ServerIpAddress;
    ULONG DefaultRouter;
    ULONG DnsNameServer;
    WCHAR NetbootCardHardwareId[64];
    WCHAR NetbootCardDriverName[24];
    WCHAR NetbootCardServiceName[24];
    PCHAR NetbootCardRegistry;
    ULONG NetbootCardRegistryLength;
    PCHAR NetbootCardInfo;
    ULONG NetbootCardInfoLength;
    ULONG Flags;
    PCHAR MachineDirectoryPath;
    PCHAR NetBootSifPath;
    PVOID NetBootSecret;
    CHAR NetBootIMirrorFilePath[26];
    PCHAR ASRPnPSifFile;
    ULONG ASRPnPSifFileLength;
    CHAR NetBootAdministratorPassword[64];
} SETUP_LOADER_BLOCK, *PSETUP_LOADER_BLOCK;

#endif
