/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS Boot Configuration Data
* FILE:            boot/environ/include/bcd.h
* PURPOSE:         BCD Main Header
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifndef _BCD_H
#define _BCD_H

/* ENUMERATIONS **************************************************************/

/* See https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bcd/bcdelementtype */

#define BCD_CLASS_LIBRARY       0x01
#define BCD_CLASS_APPLICATION   0x02
#define BCD_CLASS_DEVICE        0x03
#define BCD_CLASS_OEM           0x05

#define BCD_TYPE_DEVICE         0x01
#define BCD_TYPE_STRING         0x02
#define BCD_TYPE_OBJECT         0x03
#define BCD_TYPE_OBJECT_LIST    0x04
#define BCD_TYPE_INTEGER        0x05
#define BCD_TYPE_BOOLEAN        0x06
#define BCD_TYPE_INTEGER_LIST   0x07

#define BCD_IMAGE_TYPE_FIRMWARE     0x01
#define BCD_IMAGE_TYPE_BOOT_APP     0x02
#define BCD_IMAGE_TYPE_NTLDR        0x03
#define BCD_IMAGE_TYPE_REAL_MODE    0x04

#define BCD_APPLICATION_TYPE_FWBOOTMGR  0x01
#define BCD_APPLICATION_TYPE_BOOTMGR    0x02
#define BCD_APPLICATION_TYPE_OSLOADER   0x03
#define BCD_APPLICATION_TYPE_RESUME     0x04
#define BCD_APPLICATION_TYPE_MEMDIAG    0x05
#define BCD_APPLICATION_TYPE_NTLDR      0x06
#define BCD_APPLICATION_TYPE_SETUPLDR   0x07
#define BCD_APPLICATION_TYPE_BOOTSECTOR 0x08
#define BCD_APPLICATION_TYPE_STARTUPCOM 0x09

#define BCD_OBJECT_TYPE_APPLICATION     0x01
#define BCD_OBJECT_TYPE_INHERIT         0x02
#define BCD_OBJECT_TYPE_DEVICE          0x03

typedef enum BcdLibraryElementTypes
{
    BcdLibraryDevice_ApplicationDevice = 0x11000001,
    BcdLibraryString_ApplicationPath = 0x12000002,
    BcdLibraryString_Description = 0x12000004,
    BcdLibraryString_PreferredLocale = 0x12000005,
    BcdLibraryObjectList_InheritedObjects = 0x14000006,
    BcdLibraryInteger_TruncatePhysicalMemory = 0x15000007,
    BcdLibraryObjectList_RecoverySequence = 0x14000008,
    BcdLibraryBoolean_AutoRecoveryEnabled = 0x16000009,
    BcdLibraryIntegerList_BadMemoryList = 0x1700000a,
    BcdLibraryBoolean_AllowBadMemoryAccess = 0x1600000b,
    BcdLibraryInteger_FirstMegabytePolicy = 0x1500000c,
    BcdLibraryInteger_RelocatePhysicalMemory = 0x1500000D,
    BcdLibraryInteger_AvoidLowPhysicalMemory = 0x1500000E,
    BcdLibraryBoolean_DebuggerEnabled = 0x16000010,
    BcdLibraryInteger_DebuggerType = 0x15000011,
    BcdLibraryInteger_SerialDebuggerPortAddress = 0x15000012,
    BcdLibraryInteger_SerialDebuggerPort = 0x15000013,
    BcdLibraryInteger_SerialDebuggerBaudRate = 0x15000014,
    BcdLibraryInteger_1394DebuggerChannel = 0x15000015,
    BcdLibraryString_UsbDebuggerTargetName = 0x12000016,
    BcdLibraryBoolean_DebuggerIgnoreUsermodeExceptions = 0x16000017,
    BcdLibraryInteger_DebuggerStartPolicy = 0x15000018,
    BcdLibraryString_DebuggerBusParameters = 0x12000019,
    BcdLibraryInteger_DebuggerNetHostIP = 0x1500001A,
    BcdLibraryInteger_DebuggerNetPort = 0x1500001B,
    BcdLibraryBoolean_DebuggerNetDhcp = 0x1600001C,
    BcdLibraryString_DebuggerNetKey = 0x1200001D,
    BcdLibraryBoolean_EmsEnabled = 0x16000020,
    BcdLibraryInteger_EmsPort = 0x15000022,
    BcdLibraryInteger_EmsBaudRate = 0x15000023,
    BcdLibraryString_LoadOptionsString = 0x12000030,
    BcdLibraryBoolean_DisplayAdvancedOptions = 0x16000040,
    BcdLibraryBoolean_DisplayOptionsEdit = 0x16000041,
    BcdLibraryDevice_BsdLogDevice = 0x11000043,
    BcdLibraryString_BsdLogPath = 0x12000044,
    BcdLibraryBoolean_PreserveBsdLog = 0x16000045, /* Undocumented */
    BcdLibraryBoolean_GraphicsModeDisabled = 0x16000046,
    BcdLibraryInteger_ConfigAccessPolicy = 0x15000047,
    BcdLibraryBoolean_DisableIntegrityChecks = 0x16000048,
    BcdLibraryBoolean_AllowPrereleaseSignatures = 0x16000049,
    BcdLibraryString_FontPath = 0x1200004A,
    BcdLibraryInteger_SiPolicy = 0x1500004B,
    BcdLibraryInteger_FveBandId = 0x1500004C,
    BcdLibraryBoolean_ConsoleExtendedInput = 0x16000050,
    BcdLibraryInteger_GraphicsResolution = 0x15000052,
    BcdLibraryInteger_DisplayMessage = 0x15000065, /* Undocumented */
    BcdLibraryInteger_DisplayMessageOverride = 0x15000066, /* Undocumented */
    BcdLibraryInteger_UndocumentedMagic = 0x15000075, /* Undocumented magic */
    BcdLibraryBoolean_RestartOnFailure = 0x16000053,
    BcdLibraryBoolean_GraphicsForceHighestMode = 0x16000054,
    BcdLibraryBoolean_IsolatedExecutionContext = 0x16000060,
    BcdLibraryBoolean_BootUxDisable = 0x1600006C,
    BcdLibraryBoolean_BootShutdownDisabled = 0x16000074,
    BcdLibraryIntegerList_AllowedInMemorySettings = 0x17000077,
    BcdLibraryBoolean_ForceFipsCrypto = 0x16000079,
    BcdLibraryBoolean_MobileGraphics = 0x1600007A /* Undocumented */
} BcdLibraryElementTypes;

typedef enum BcdOSLoaderElementTypes
{
    BcdOSLoaderDevice_OSDevice = 0x21000001,
    BcdOSLoaderString_SystemRoot = 0x22000002,
    BcdOSLoaderObject_AssociatedResumeObject = 0x23000003,
    BcdOSLoaderBoolean_DetectKernelAndHal = 0x26000010,
    BcdOSLoaderString_KernelPath = 0x22000011,
    BcdOSLoaderString_HalPath = 0x22000012,
    BcdOSLoaderString_DbgTransportPath = 0x22000013,
    BcdOSLoaderInteger_NxPolicy = 0x25000020,
    BcdOSLoaderInteger_PAEPolicy = 0x25000021,
    BcdOSLoaderBoolean_WinPEMode = 0x26000022,
    BcdOSLoaderBoolean_DisableCrashAutoReboot = 0x26000024,
    BcdOSLoaderBoolean_UseLastGoodSettings = 0x26000025,
    BcdOSLoaderBoolean_AllowPrereleaseSignatures = 0x26000027,
    BcdOSLoaderBoolean_NoLowMemory = 0x26000030,
    BcdOSLoaderInteger_RemoveMemory = 0x25000031,
    BcdOSLoaderInteger_IncreaseUserVa = 0x25000032,
    BcdOSLoaderBoolean_UseVgaDriver = 0x26000040,
    BcdOSLoaderBoolean_DisableBootDisplay = 0x26000041,
    BcdOSLoaderBoolean_DisableVesaBios = 0x26000042,
    BcdOSLoaderBoolean_DisableVgaMode = 0x26000043,
    BcdOSLoaderInteger_ClusterModeAddressing = 0x25000050,
    BcdOSLoaderBoolean_UsePhysicalDestination = 0x26000051,
    BcdOSLoaderInteger_RestrictApicCluster = 0x25000052,
    BcdOSLoaderBoolean_UseLegacyApicMode = 0x26000054,
    BcdOSLoaderInteger_X2ApicPolicy = 0x25000055,
    BcdOSLoaderBoolean_UseBootProcessorOnly = 0x26000060,
    BcdOSLoaderInteger_NumberOfProcessors = 0x25000061,
    BcdOSLoaderBoolean_ForceMaximumProcessors = 0x26000062,
    BcdOSLoaderBoolean_ProcessorConfigurationFlags = 0x25000063,
    BcdOSLoaderBoolean_MaximizeGroupsCreated = 0x26000064,
    BcdOSLoaderBoolean_ForceGroupAwareness = 0x26000065,
    BcdOSLoaderInteger_GroupSize = 0x25000066,
    BcdOSLoaderInteger_UseFirmwarePciSettings = 0x26000070,
    BcdOSLoaderInteger_MsiPolicy = 0x25000071,
    BcdOSLoaderInteger_SafeBoot = 0x25000080,
    BcdOSLoaderBoolean_SafeBootAlternateShell = 0x26000081,
    BcdOSLoaderBoolean_BootLogInitialization = 0x26000090,
    BcdOSLoaderBoolean_VerboseObjectLoadMode = 0x26000091,
    BcdOSLoaderBoolean_KernelDebuggerEnabled = 0x260000a0,
    BcdOSLoaderBoolean_DebuggerHalBreakpoint = 0x260000a1,
    BcdOSLoaderBoolean_UsePlatformClock = 0x260000A2,
    BcdOSLoaderBoolean_ForceLegacyPlatform = 0x260000A3,
    BcdOSLoaderInteger_TscSyncPolicy = 0x250000A6,
    BcdOSLoaderBoolean_EmsEnabled = 0x260000B0,
    BcdOSLoaderInteger_ForceFailure = 0x250000C0,
    BcdOSLoaderInteger_DriverLoadFailurePolicy = 0x250000C1,
    BcdOSLoaderInteger_BootMenuPolicy = 0x250000C2,
    BcdOSLoaderBoolean_AdvancedOptionsOneTime = 0x260000C3,
    BcdOSLoaderBoolean_OptionsEditOneTime = 0x260000C4, /* Undocumented */
    BcdOSLoaderInteger_BootStatusPolicy = 0x250000E0,
    BcdOSLoaderBoolean_DisableElamDrivers = 0x260000E1,
    BcdOSLoaderInteger_HypervisorLaunchType = 0x250000F0,
    BcdOSLoaderBoolean_HypervisorDebuggerEnabled = 0x260000F2,
    BcdOSLoaderInteger_HypervisorDebuggerType = 0x250000F3,
    BcdOSLoaderInteger_HypervisorDebuggerPortNumber = 0x250000F4,
    BcdOSLoaderInteger_HypervisorDebuggerBaudrate = 0x250000F5,
    BcdOSLoaderInteger_HypervisorDebugger1394Channel = 0x250000F6,
    BcdOSLoaderInteger_BootUxPolicy = 0x250000F7,
    BcdOSLoaderString_HypervisorDebuggerBusParams = 0x220000F9,
    BcdOSLoaderInteger_HypervisorNumProc = 0x250000FA,
    BcdOSLoaderInteger_HypervisorRootProcPerNode = 0x250000FB,
    BcdOSLoaderBoolean_HypervisorUseLargeVTlb = 0x260000FC,
    BcdOSLoaderInteger_HypervisorDebuggerNetHostIp = 0x250000FD,
    BcdOSLoaderInteger_HypervisorDebuggerNetHostPort = 0x250000FE,
    BcdOSLoaderInteger_TpmBootEntropyPolicy = 0x25000100,
    BcdOSLoaderString_HypervisorDebuggerNetKey = 0x22000110,
    BcdOSLoaderBoolean_HypervisorDebuggerNetDhcp = 0x26000114,
    BcdOSLoaderInteger_HypervisorIommuPolicy = 0x25000115,
    BcdOSLoaderInteger_XSaveDisable = 0x2500012b
} BcdOSLoaderElementTypes;

typedef enum BcdBootMgrElementTypes
{
    BcdBootMgrObjectList_DisplayOrder = 0x24000001,
    BcdBootMgrObjectList_BootSequence = 0x24000002,
    BcdBootMgrObject_DefaultObject = 0x23000003,
    BcdBootMgrInteger_Timeout = 0x25000004,
    BcdBootMgrBoolean_AttemptResume = 0x26000005,
    BcdBootMgrObject_ResumeObject = 0x23000006,
    BcdBootMgrObjectList_ToolsDisplayOrder = 0x24000010,
    BcdBootMgrBoolean_DisplayBootMenu = 0x26000020,
    BcdBootMgrBoolean_NoErrorDisplay = 0x26000021,
    BcdBootMgrDevice_BcdDevice = 0x21000022,
    BcdBootMgrString_BcdFilePath = 0x22000023,
    BcdBootMgrBoolean_ProcessCustomActionsFirst = 0x26000028,
    BcdBootMgrIntegerList_CustomActionsList = 0x27000030,
    BcdBootMgrBoolean_PersistBootSequence = 0x26000031
} BcdBootMgrElementTypes;

typedef enum _BcdResumeElementTypes
{
    Reserved1 = 0x21000001,
    Reserved2 = 0x22000002,
    BcdResumeBoolean_UseCustomSettings = 0x26000003,
    BcdResumeDevice_AssociatedOsDevice = 0x21000005,
    BcdResumeBoolean_DebugOptionEnabled = 0x26000006,
    BcdResumeInteger_BootMenuPolicy = 0x25000008
} BcdResumeElementTypes;

typedef enum _BCDE_OSLOADER_TYPE_BOOT_STATUS_POLICY
{
    DisplayAllFailures,
    IgnoreAllFailures,
    IgnoreShutdownFailures,
    IgnoreBootFailures,
    IgnoreCheckpointFailures,
    DisplayShutdownFailures,
    DisplayBootFailures,
    DisplayCheckpointFailures
} BCDE_OSLOADER_TYPE_BOOT_STATUS_POLICY;

/* Undocumented */
typedef enum BcdStartupElementTypes
{
    BcdStartupBoolean_PxeSoftReboot = 0x26000001,
    BcdStartupString_PxeApplicationName = 0x22000002,
} BcdStartupElementTypes;

/* DATA STRUCTURES ***********************************************************/

typedef struct
{
    union
    {
        ULONG PackedValue;
        struct
        {
            ULONG SubType : 24;
            ULONG Format : 4;
            ULONG Class : 4;
        };
    };
} BcdElementType;

typedef struct
{
    union
    {
        ULONG PackedValue;
        union
        {
            struct
            {
                ULONG ApplicationCode : 20;
                ULONG ImageCode : 4;
                ULONG Reserved : 4;
                ULONG ObjectCode : 4;
            } Application;
            struct
            {
                ULONG Value : 20;
                ULONG ClassCode : 4;
                ULONG Reserved : 4;
                ULONG ObjectCode : 4;
            } Inherit;
            struct
            {
                ULONG Reserved:28;
                ULONG ObjectCode : 4;
            } Device;
        };
    };
} BcdObjectType;

typedef struct _BCD_ELEMENT_HEADER
{
    ULONG Version;
    ULONG Type;
    ULONG Size;
} BCD_ELEMENT_HEADER, *PBCD_ELEMENT_HEADER;

typedef struct _BCD_PACKED_ELEMENT
{
    struct _BCD_PACKED_ELEMENT* NextEntry;
    BcdElementType RootType;
    BCD_ELEMENT_HEADER;
    UCHAR Data[ANYSIZE_ARRAY];
} BCD_PACKED_ELEMENT, *PBCD_PACKED_ELEMENT;

typedef struct _BCD_ELEMENT
{
    PBCD_ELEMENT_HEADER Header;
    PUCHAR Body;
} BCD_ELEMENT, *PBCD_ELEMENT;

typedef struct _BCD_DEVICE_OPTION
{
    GUID AssociatedEntry;
    BL_DEVICE_DESCRIPTOR DeviceDescriptor;
} BCD_DEVICE_OPTION, *PBCD_DEVICE_OPTION;

typedef struct _BCD_OBJECT_DESCRIPTION
{
    ULONG Valid;
    ULONG Type;
} BCD_OBJECT_DESCRIPTION, *PBCD_OBJECT_DESCRIPTION;

/* FUNCTIONS ******************************************************************/

NTSTATUS
BcdOpenStoreFromFile (
    _In_ PUNICODE_STRING FileName,
    _In_ PHANDLE StoreHandle
    );

#define BCD_ENUMERATE_FLAG_DEEP         0x04
#define BCD_ENUMERATE_FLAG_DEVICES      0x08
#define BCD_ENUMERATE_FLAG_IN_ORDER     0x10

NTSTATUS
BiEnumerateElements (
    _In_ HANDLE BcdHandle,
    _In_ HANDLE ObjectHandle,
    _In_ ULONG RootElementType,
    _In_ ULONG Flags,
    _Out_opt_ PBCD_PACKED_ELEMENT Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCountNe
    );

NTSTATUS
BcdOpenObject (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _Out_ PHANDLE ObjectHandle
    );

NTSTATUS
BcdDeleteElement (
    _In_ HANDLE ObjectHandle,
    _In_ ULONG Type
    );

NTSTATUS
BcdEnumerateAndUnpackElements (
    _In_ HANDLE BcdHandle,
    _In_ HANDLE ObjectHandle,
    _Out_opt_ PBCD_ELEMENT Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCount
    );

NTSTATUS
BiGetObjectDescription (
    _In_ HANDLE ObjectHandle,
    _Out_ PBCD_OBJECT_DESCRIPTION Description
    );

#endif
