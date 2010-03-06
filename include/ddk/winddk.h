/*
 * winddk.h
 *
 * Windows Device Driver Kit
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WINDDK_H
#define __WINDDK_H

/* Helper macro to enable gcc's extension.  */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

#include "intrin.h"

/* Pseudo modifiers for parameters */
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
#ifndef UNALLIGNED
#define UNALLIGNED
#endif

#define CONST const

#define RESTRICTED_POINTER

#define DECLSPEC_ADDRSAFE

#ifdef NONAMELESSUNION
# define _DDK_DUMMYUNION_MEMBER(name) DUMMYUNIONNAME.name
# define _DDK_DUMMYUNION_N_MEMBER(n, name) DUMMYUNIONNAME##n.name
#else
# define _DDK_DUMMYUNION_MEMBER(name) name
# define _DDK_DUMMYUNION_N_MEMBER(n, name) name
#endif

/*
** Forward declarations
*/

struct _KPCR;
struct _KPRCB;
struct _KTSS;
struct _DRIVE_LAYOUT_INFORMATION_EX;
struct _LOADER_PARAMETER_BLOCK;
struct _BUS_HANDLER;

typedef struct _BUS_HANDLER *PBUS_HANDLER;

#if 1
/* FIXME: Unknown definitions */
struct _SET_PARTITION_INFORMATION_EX;
#define WaitAll 0
#define WaitAny 1
typedef HANDLE TRACEHANDLE;
typedef PVOID PWMILIB_CONTEXT;
#endif

/*
** WmiLib specific structure
*/
typedef enum
{
    IrpProcessed,    // Irp was processed and possibly completed
    IrpNotCompleted, // Irp was process and NOT completed
    IrpNotWmi,       // Irp is not a WMI irp
    IrpForward       // Irp is wmi irp, but targeted at another device object
} SYSCTL_IRP_DISPOSITION, *PSYSCTL_IRP_DISPOSITION;

#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT           12288

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

#define DPFLTR_ERROR_LEVEL                  0
#define DPFLTR_WARNING_LEVEL                1
#define DPFLTR_TRACE_LEVEL                  2
#define DPFLTR_INFO_LEVEL                   3
#define DPFLTR_MASK                         0x80000000

typedef enum _DPFLTR_TYPE
{
    DPFLTR_SYSTEM_ID = 0,
    DPFLTR_SMSS_ID = 1,
    DPFLTR_SETUP_ID = 2,
    DPFLTR_NTFS_ID = 3,
    DPFLTR_FSTUB_ID = 4,
    DPFLTR_CRASHDUMP_ID = 5,
    DPFLTR_CDAUDIO_ID = 6,
    DPFLTR_CDROM_ID = 7,
    DPFLTR_CLASSPNP_ID = 8,
    DPFLTR_DISK_ID = 9,
    DPFLTR_REDBOOK_ID = 10,
    DPFLTR_STORPROP_ID = 11,
    DPFLTR_SCSIPORT_ID = 12,
    DPFLTR_SCSIMINIPORT_ID = 13,
    DPFLTR_CONFIG_ID = 14,
    DPFLTR_I8042PRT_ID = 15,
    DPFLTR_SERMOUSE_ID = 16,
    DPFLTR_LSERMOUS_ID = 17,
    DPFLTR_KBDHID_ID = 18,
    DPFLTR_MOUHID_ID = 19,
    DPFLTR_KBDCLASS_ID = 20,
    DPFLTR_MOUCLASS_ID = 21,
    DPFLTR_TWOTRACK_ID = 22,
    DPFLTR_WMILIB_ID = 23,
    DPFLTR_ACPI_ID = 24,
    DPFLTR_AMLI_ID = 25,
    DPFLTR_HALIA64_ID = 26,
    DPFLTR_VIDEO_ID = 27,
    DPFLTR_SVCHOST_ID = 28,
    DPFLTR_VIDEOPRT_ID = 29,
    DPFLTR_TCPIP_ID = 30,
    DPFLTR_DMSYNTH_ID = 31,
    DPFLTR_NTOSPNP_ID = 32,
    DPFLTR_FASTFAT_ID = 33,
    DPFLTR_SAMSS_ID = 34,
    DPFLTR_PNPMGR_ID = 35,
    DPFLTR_NETAPI_ID = 36,
    DPFLTR_SCSERVER_ID = 37,
    DPFLTR_SCCLIENT_ID = 38,
    DPFLTR_SERIAL_ID = 39,
    DPFLTR_SERENUM_ID = 40,
    DPFLTR_UHCD_ID = 41,
    DPFLTR_BOOTOK_ID = 42,
    DPFLTR_BOOTVRFY_ID = 43,
    DPFLTR_RPCPROXY_ID = 44,
    DPFLTR_AUTOCHK_ID = 45,
    DPFLTR_DCOMSS_ID = 46,
    DPFLTR_UNIMODEM_ID = 47,
    DPFLTR_SIS_ID = 48,
    DPFLTR_FLTMGR_ID = 49,
    DPFLTR_WMICORE_ID = 50,
    DPFLTR_BURNENG_ID = 51,
    DPFLTR_IMAPI_ID = 52,
    DPFLTR_SXS_ID = 53,
    DPFLTR_FUSION_ID = 54,
    DPFLTR_IDLETASK_ID = 55,
    DPFLTR_SOFTPCI_ID = 56,
    DPFLTR_TAPE_ID = 57,
    DPFLTR_MCHGR_ID = 58,
    DPFLTR_IDEP_ID = 59,
    DPFLTR_PCIIDE_ID = 60,
    DPFLTR_FLOPPY_ID = 61,
    DPFLTR_FDC_ID = 62,
    DPFLTR_TERMSRV_ID = 63,
    DPFLTR_W32TIME_ID = 64,
    DPFLTR_PREFETCHER_ID = 65,
    DPFLTR_RSFILTER_ID = 66,
    DPFLTR_FCPORT_ID = 67,
    DPFLTR_PCI_ID = 68,
    DPFLTR_DMIO_ID = 69,
    DPFLTR_DMCONFIG_ID = 70,
    DPFLTR_DMADMIN_ID = 71,
    DPFLTR_WSOCKTRANSPORT_ID = 72,
    DPFLTR_VSS_ID = 73,
    DPFLTR_PNPMEM_ID = 74,
    DPFLTR_PROCESSOR_ID = 75,
    DPFLTR_DMSERVER_ID = 76,
    DPFLTR_SR_ID = 77,
    DPFLTR_INFINIBAND_ID = 78,
    DPFLTR_IHVDRIVER_ID = 79,
    DPFLTR_IHVVIDEO_ID = 80,
    DPFLTR_IHVAUDIO_ID = 81,
    DPFLTR_IHVNETWORK_ID = 82,
    DPFLTR_IHVSTREAMING_ID = 83,
    DPFLTR_IHVBUS_ID = 84,
    DPFLTR_HPS_ID = 85,
    DPFLTR_RTLTHREADPOOL_ID = 86,
    DPFLTR_LDR_ID = 87,
    DPFLTR_TCPIP6_ID = 88,
    DPFLTR_ISAPNP_ID = 89,
    DPFLTR_SHPC_ID = 90,
    DPFLTR_STORPORT_ID = 91,
    DPFLTR_STORMINIPORT_ID = 92,
    DPFLTR_PRINTSPOOLER_ID = 93,
    DPFLTR_VDS_ID = 94,
    DPFLTR_VDSBAS_ID = 95,
    DPFLTR_VDSDYNDR_ID = 96,
    DPFLTR_VDSUTIL_ID = 97,
    DPFLTR_DFRGIFC_ID = 98,
    DPFLTR_DEFAULT_ID = 99,
    DPFLTR_MM_ID = 100,
    DPFLTR_DFSC_ID = 101,
    DPFLTR_WOW64_ID = 102,
    DPFLTR_ENDOFTABLE_ID
} DPFLTR_TYPE;

/* also in winnt.h */

#define FILE_COPY_STRUCTURED_STORAGE      0x00000041
#define FILE_STRUCTURED_STORAGE           0x00000441

/* end winnt.h */

/* Exported object types */
extern POBJECT_TYPE NTSYSAPI ExDesktopObjectType;
extern POBJECT_TYPE NTSYSAPI ExWindowStationObjectType;
extern ULONG NTSYSAPI IoDeviceHandlerObjectSize;
extern POBJECT_TYPE NTSYSAPI IoDeviceHandlerObjectType;
extern POBJECT_TYPE NTSYSAPI IoDeviceObjectType;
extern POBJECT_TYPE NTSYSAPI IoDriverObjectType;
extern POBJECT_TYPE NTSYSAPI LpcPortObjectType;
extern POBJECT_TYPE NTSYSAPI PsProcessType;

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
extern volatile CCHAR NTSYSAPI KeNumberProcessors;
#else
#if (NTDDI_VERSION >= NTDDI_WINXP)
extern CCHAR NTSYSAPI KeNumberProcessors;
#else
//extern PCCHAR KeNumberProcessors;
extern NTSYSAPI CCHAR KeNumberProcessors; //FIXME: Note to Alex: I won't fix this atm, since I prefer to discuss this with you first.
#endif
#endif

#define MAX_WOW64_SHARED_ENTRIES 16

#define NX_SUPPORT_POLICY_ALWAYSOFF 0
#define NX_SUPPORT_POLICY_ALWAYSON 1
#define NX_SUPPORT_POLICY_OPTIN 2
#define NX_SUPPORT_POLICY_OPTOUT 3

typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
    ULONG Reserved1;
    ULONG Reserved3;
    volatile ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    BOOLEAN KdDebuggerEnabled;
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
    UCHAR NXSupportPolicy;
#endif
    volatile ULONG ActiveConsoleId;
    volatile ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
    ULONG TraceLogging;
    ULONG Fill0;
    ULONGLONG TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    ULONGLONG SystemCallPad[3];
    __GNU_EXTENSION union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
    };
    ULONG Cookie;
#if (NTDDI_VERSION >= NTDDI_WS03)
    LONGLONG ConsoleSessionForegroundProcessId;
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    USHORT UserModeGlobalLogger[8];
    ULONG HeapTracingPid[2];
    ULONG CritSecTracingPid[2];
    __GNU_EXTENSION union
    {
        ULONG SharedDataFlags;
        __GNU_EXTENSION struct
        {
            ULONG DbgErrorPortPresent:1;
            ULONG DbgElevationEnabled:1;
            ULONG DbgVirtEnabled:1;
            ULONG DbgInstallerDetectEnabled:1;
            ULONG SpareBits:28;
        };
    };
    ULONG ImageFileExecutionOptions;
    KAFFINITY ActiveProcessorAffinity;
#endif
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

/*
** IRP function codes
*/

#define IRP_MN_QUERY_DIRECTORY            0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY    0x02

#define IRP_MN_USER_FS_REQUEST            0x00
#define IRP_MN_MOUNT_VOLUME               0x01
#define IRP_MN_VERIFY_VOLUME              0x02
#define IRP_MN_LOAD_FILE_SYSTEM           0x03
#define IRP_MN_TRACK_LINK                 0x04
#define IRP_MN_KERNEL_CALL                0x04

#define IRP_MN_LOCK                       0x01
#define IRP_MN_UNLOCK_SINGLE              0x02
#define IRP_MN_UNLOCK_ALL                 0x03
#define IRP_MN_UNLOCK_ALL_BY_KEY          0x04

#define IRP_MN_NORMAL                     0x00
#define IRP_MN_DPC                        0x01
#define IRP_MN_MDL                        0x02
#define IRP_MN_COMPLETE                   0x04
#define IRP_MN_COMPRESSED                 0x08

#define IRP_MN_MDL_DPC                    (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL               (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC           (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)

#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

typedef EXCEPTION_DISPOSITION
(DDKAPI *PEXCEPTION_ROUTINE)(
  IN struct _EXCEPTION_RECORD *ExceptionRecord,
  IN PVOID EstablisherFrame,
  IN OUT struct _CONTEXT *ContextRecord,
  IN OUT PVOID DispatcherContext);

typedef NTSTATUS
(DDKAPI *PDRIVER_ENTRY)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN PUNICODE_STRING  RegistryPath);

typedef VOID
(DDKAPI *PDRIVER_REINITIALIZE)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN PVOID  Context,
  IN ULONG  Count);

typedef BOOLEAN
(DDKAPI *PKTRANSFER_ROUTINE)(
  VOID);

#define ASSERT_GATE(object) \
    ASSERT((((object)->Header.Type & KOBJECT_TYPE_MASK) == GateObject) || \
          (((object)->Header.Type & KOBJECT_TYPE_MASK) == EventSynchronizationObject))

#define TIMER_TABLE_SIZE 512
#define TIMER_TABLE_SHIFT 9

#define ASSERT_TIMER(E) \
    ASSERT(((E)->Header.Type == TimerNotificationObject) || \
           ((E)->Header.Type == TimerSynchronizationObject))

#define ASSERT_MUTANT(E) \
    ASSERT((E)->Header.Type == MutantObject)

#define ASSERT_SEMAPHORE(E) \
    ASSERT((E)->Header.Type == SemaphoreObject)

#define ASSERT_EVENT(E) \
    ASSERT(((E)->Header.Type == NotificationEvent) || \
           ((E)->Header.Type == SynchronizationEvent))

#define KEYBOARD_INSERT_ON                0x08
#define KEYBOARD_CAPS_LOCK_ON             0x04
#define KEYBOARD_NUM_LOCK_ON              0x02
#define KEYBOARD_SCROLL_LOCK_ON           0x01
#define KEYBOARD_ALT_KEY_DOWN             0x80
#define KEYBOARD_CTRL_KEY_DOWN            0x40
#define KEYBOARD_LEFT_SHIFT_DOWN          0x20
#define KEYBOARD_RIGHT_SHIFT_DOWN         0x10

typedef struct _IO_COUNTERS {
    ULONGLONG  ReadOperationCount;
    ULONGLONG  WriteOperationCount;
    ULONGLONG  OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;

typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

typedef struct _POOLED_USAGE_AND_LIMITS
{
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

typedef struct _CONTROLLER_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PVOID  ControllerExtension;
  KDEVICE_QUEUE  DeviceWaitQueue;
  ULONG  Spare1;
  LARGE_INTEGER  Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

/* DEVICE_OBJECT.Flags */

#define DO_DEVICE_HAS_NAME                0x00000040
#define DO_SYSTEM_BOOT_PARTITION          0x00000100
#define DO_LONG_TERM_REQUESTS             0x00000200
#define DO_NEVER_LAST_DEVICE              0x00000400
#define DO_LOW_PRIORITY_FILESYSTEM        0x00010000
#define DO_XIP                            0x00020000

#define DRVO_REINIT_REGISTERED            0x00000008
#define DRVO_INITIALIZED                  0x00000010
#define DRVO_BOOTREINIT_REGISTERED        0x00000020
#define DRVO_LEGACY_RESOURCES             0x00000040

typedef enum _ARBITER_REQUEST_SOURCE {
  ArbiterRequestUndefined = -1,
  ArbiterRequestLegacyReported,
  ArbiterRequestHalReported,
  ArbiterRequestLegacyAssigned,
  ArbiterRequestPnpDetected,
  ArbiterRequestPnpEnumerated
} ARBITER_REQUEST_SOURCE;

typedef enum _ARBITER_RESULT {
  ArbiterResultUndefined = -1,
  ArbiterResultSuccess,
  ArbiterResultExternalConflict,
  ArbiterResultNullRequest
} ARBITER_RESULT;

typedef enum _ARBITER_ACTION {
  ArbiterActionTestAllocation,
  ArbiterActionRetestAllocation,
  ArbiterActionCommitAllocation,
  ArbiterActionRollbackAllocation,
  ArbiterActionQueryAllocatedResources,
  ArbiterActionWriteReservedResources,
  ArbiterActionQueryConflict,
  ArbiterActionQueryArbitrate,
  ArbiterActionAddReserved,
  ArbiterActionBootAllocation
} ARBITER_ACTION, *PARBITER_ACTION;

typedef struct _ARBITER_CONFLICT_INFO {
  PDEVICE_OBJECT  OwningObject;
  ULONGLONG  Start;
  ULONGLONG  End;
} ARBITER_CONFLICT_INFO, *PARBITER_CONFLICT_INFO;

typedef struct _ARBITER_PARAMETERS {
  union {
    struct {
      IN OUT PLIST_ENTRY  ArbitrationList;
      IN ULONG  AllocateFromCount;
      IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  AllocateFrom;
    } TestAllocation;

    struct {
      IN OUT PLIST_ENTRY  ArbitrationList;
      IN ULONG  AllocateFromCount;
      IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  AllocateFrom;
    } RetestAllocation;

    struct {
      IN OUT PLIST_ENTRY  ArbitrationList;
    } BootAllocation;

    struct {
      OUT PCM_PARTIAL_RESOURCE_LIST  *AllocatedResources;
    } QueryAllocatedResources;

    struct {
      IN PDEVICE_OBJECT  PhysicalDeviceObject;
      IN PIO_RESOURCE_DESCRIPTOR  ConflictingResource;
      OUT PULONG  ConflictCount;
      OUT PARBITER_CONFLICT_INFO  *Conflicts;
    } QueryConflict;

    struct {
      IN PLIST_ENTRY  ArbitrationList;
    } QueryArbitrate;

    struct {
      IN PDEVICE_OBJECT  ReserveDevice;
    } AddReserved;
  } Parameters;
} ARBITER_PARAMETERS, *PARBITER_PARAMETERS;

#define ARBITER_FLAG_BOOT_CONFIG 0x00000001

typedef struct _ARBITER_LIST_ENTRY {
  LIST_ENTRY  ListEntry;
  ULONG  AlternativeCount;
  PIO_RESOURCE_DESCRIPTOR  Alternatives;
  PDEVICE_OBJECT  PhysicalDeviceObject;
  ARBITER_REQUEST_SOURCE  RequestSource;
  ULONG  Flags;
  LONG_PTR  WorkSpace;
  INTERFACE_TYPE  InterfaceType;
  ULONG  SlotNumber;
  ULONG  BusNumber;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR  Assignment;
  PIO_RESOURCE_DESCRIPTOR  SelectedAlternative;
  ARBITER_RESULT  Result;
} ARBITER_LIST_ENTRY, *PARBITER_LIST_ENTRY;

typedef NTSTATUS
(DDKAPI *PARBITER_HANDLER)(
  IN PVOID  Context,
  IN ARBITER_ACTION  Action,
  IN OUT PARBITER_PARAMETERS  Parameters);

#define ARBITER_PARTIAL 0x00000001

typedef struct _ARBITER_INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PARBITER_HANDLER  ArbiterHandler;
  ULONG  Flags;
} ARBITER_INTERFACE, *PARBITER_INTERFACE;

typedef enum _HAL_QUERY_INFORMATION_CLASS {
  HalInstalledBusInformation,
  HalProfileSourceInformation,
  HalInformationClassUnused1,
  HalPowerInformation,
  HalProcessorSpeedInformation,
  HalCallbackInformation,
  HalMapRegisterInformation,
  HalMcaLogInformation,
  HalFrameBufferCachingInformation,
  HalDisplayBiosInformation,
  HalProcessorFeatureInformation,
  HalNumaTopologyInterface,
  HalErrorInformation,
  HalCmcLogInformation,
  HalCpeLogInformation,
  HalQueryMcaInterface,
  HalQueryAMLIIllegalIOPortAddresses,
  HalQueryMaxHotPlugMemoryAddress,
  HalPartitionIpiInterface,
  HalPlatformInformation,
  HalQueryProfileSourceList
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;

typedef enum _HAL_SET_INFORMATION_CLASS {
  HalProfileSourceInterval,
  HalProfileSourceInterruptHandler,
  HalMcaRegisterDriver,
  HalKernelErrorHandler,
  HalCmcRegisterDriver,
  HalCpeRegisterDriver,
  HalMcaLog,
  HalCmcLog,
  HalCpeLog,
  HalGenerateCmcInterrupt
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;

typedef struct _HAL_PROFILE_SOURCE_INTERVAL
{
    KPROFILE_SOURCE Source;
    ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION
{
    KPROFILE_SOURCE Source;
    BOOLEAN Supported;
    ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

typedef struct _MAP_REGISTER_ENTRY
{
    PVOID MapRegister;
    BOOLEAN WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;

typedef struct
{
    UCHAR Type;
    BOOLEAN Valid;
    UCHAR Reserved[2];
    PUCHAR TranslatedAddress;
    ULONG Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct
{
    PHYSICAL_ADDRESS Start;
    PHYSICAL_ADDRESS MaxEnd;
    PVOID VirtualAddress;
    ULONG Length;
    BOOLEAN Cached;
    BOOLEAN Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef struct
{
    ULONG Bus;
    ULONG Slot;
    USHORT VendorID;
    USHORT DeviceID;
    UCHAR BaseClass;
    UCHAR SubClass;
    UCHAR ProgIf;
    BOOLEAN Initialized;
    DEBUG_DEVICE_ADDRESS BaseAddress[6];
    DEBUG_MEMORY_REQUIREMENTS Memory;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

/* Function Type Defintions for Dispatch Functions */
struct _DEVICE_CONTROL_CONTEXT;

typedef VOID
(DDKAPI *PDEVICE_CONTROL_COMPLETION)(
  IN struct _DEVICE_CONTROL_CONTEXT  *ControlContext);

typedef struct _DEVICE_CONTROL_CONTEXT {
  NTSTATUS  Status;
  PDEVICE_HANDLER_OBJECT  DeviceHandler;
  PDEVICE_OBJECT  DeviceObject;
  ULONG  ControlCode;
  PVOID  Buffer;
  PULONG  BufferLength;
  PVOID  Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

typedef struct _PM_DISPATCH_TABLE {
  ULONG  Signature;
  ULONG  Version;
  PVOID  Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef enum _RESOURCE_TRANSLATION_DIRECTION {
  TranslateChildToParent,
  TranslateParentToChild
} RESOURCE_TRANSLATION_DIRECTION;

typedef NTSTATUS
(DDKAPI *PTRANSLATE_RESOURCE_HANDLER)(
  IN PVOID  Context,
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  Source,
  IN RESOURCE_TRANSLATION_DIRECTION  Direction,
  IN ULONG  AlternativesCount,
  IN IO_RESOURCE_DESCRIPTOR  Alternatives[],
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR  Target);

typedef NTSTATUS
(DDKAPI *PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(
  IN PVOID  Context,
  IN PIO_RESOURCE_DESCRIPTOR  Source,
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  OUT PULONG  TargetCount,
  OUT PIO_RESOURCE_DESCRIPTOR  *Target);

typedef struct _TRANSLATOR_INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PTRANSLATE_RESOURCE_HANDLER  TranslateResources;
  PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER  TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;

typedef NTSTATUS
(DDKAPI *pHalDeviceControl)(
  IN PDEVICE_HANDLER_OBJECT  DeviceHandler,
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  ControlCode,
  IN OUT PVOID  Buffer OPTIONAL,
  IN OUT PULONG  BufferLength OPTIONAL,
  IN PVOID  Context,
  IN PDEVICE_CONTROL_COMPLETION  CompletionRoutine);

typedef VOID
(FASTCALL *pHalExamineMBR)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  MBRTypeIdentifier,
  OUT PVOID  *Buffer);

typedef VOID
(FASTCALL *pHalIoAssignDriveLetters)(
  IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  IN PSTRING NtDeviceName,
  OUT PUCHAR NtSystemPath,
  OUT PSTRING NtSystemPathString);

typedef NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN BOOLEAN  ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

typedef NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  PartitionNumber,
  IN ULONG  PartitionType);

typedef NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  SectorsPerTrack,
  IN ULONG  NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef PBUS_HANDLER
(FASTCALL *pHalHandlerForBus)(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber);

typedef VOID
(FASTCALL *pHalReferenceBusHandler)(
  IN PBUS_HANDLER  BusHandler);

typedef NTSTATUS
(DDKAPI *pHalQuerySystemInformation)(
  IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
  IN ULONG  BufferSize,
  IN OUT PVOID  Buffer,
  OUT PULONG  ReturnedLength);

typedef NTSTATUS
(DDKAPI *pHalSetSystemInformation)(
  IN HAL_SET_INFORMATION_CLASS  InformationClass,
  IN ULONG  BufferSize,
  IN PVOID  Buffer);

typedef NTSTATUS
(DDKAPI *pHalQueryBusSlots)(
  IN PBUS_HANDLER  BusHandler,
  IN ULONG  BufferSize,
  OUT PULONG  SlotNumbers,
  OUT PULONG  ReturnedLength);

typedef NTSTATUS
(DDKAPI *pHalInitPnpDriver)(
  VOID);

typedef NTSTATUS
(DDKAPI *pHalInitPowerManagement)(
  IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
  OUT PPM_DISPATCH_TABLE  *PmHalDispatchTable);

typedef struct _DMA_ADAPTER*
(DDKAPI *pHalGetDmaAdapter)(
  IN PVOID  Context,
  IN struct _DEVICE_DESCRIPTION  *DeviceDescriptor,
  OUT PULONG  NumberOfMapRegisters);

typedef NTSTATUS
(DDKAPI *pHalGetInterruptTranslator)(
  IN INTERFACE_TYPE  ParentInterfaceType,
  IN ULONG  ParentBusNumber,
  IN INTERFACE_TYPE  BridgeInterfaceType,
  IN USHORT  Size,
  IN USHORT  Version,
  OUT PTRANSLATOR_INTERFACE  Translator,
  OUT PULONG  BridgeBusNumber);

typedef NTSTATUS
(DDKAPI *pHalStartMirroring)(
  VOID);

typedef NTSTATUS
(DDKAPI *pHalEndMirroring)(
  IN ULONG  PassNumber);

typedef NTSTATUS
(DDKAPI *pHalMirrorPhysicalMemory)(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN LARGE_INTEGER  NumberOfBytes);

typedef NTSTATUS
(DDKAPI *pHalMirrorVerify)(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN LARGE_INTEGER  NumberOfBytes);

typedef VOID
(DDKAPI *pHalEndOfBoot)(
  VOID);

typedef
BOOLEAN
(DDKAPI *pHalTranslateBusAddress)(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
);

typedef
NTSTATUS
(DDKAPI *pHalAssignSlotResources)(
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

typedef
VOID
(DDKAPI *pHalHaltSystem)(
    VOID
);

typedef
BOOLEAN
(DDKAPI *pHalResetDisplay)(
    VOID
);

typedef
UCHAR
(DDKAPI *pHalVectorToIDTEntry)(
    ULONG Vector
);

typedef
BOOLEAN
(DDKAPI *pHalFindBusAddressTranslation)(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
);

typedef
NTSTATUS
(DDKAPI *pKdSetupPciDeviceForDebugging)(
    IN PVOID LoaderBlock OPTIONAL,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

typedef
NTSTATUS
(DDKAPI *pKdReleasePciDeviceForDebugging)(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

typedef
PVOID
(DDKAPI *pKdGetAcpiTablePhase0)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN ULONG Signature
);

typedef
VOID
(DDKAPI *pKdCheckPowerButton)(
    VOID
);

typedef
ULONG
(DDKAPI *pHalGetInterruptVector)(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
);

typedef
NTSTATUS
(DDKAPI *pHalGetVectorInput)(
    IN ULONG Vector,
    IN KAFFINITY Affinity,
    OUT PULONG Input,
    OUT PKINTERRUPT_POLARITY Polarity
);

typedef
PVOID
(DDKAPI *pKdMapPhysicalMemory64)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
);

typedef
VOID
(DDKAPI *pKdUnmapVirtualAddress)(
    IN PVOID VirtualAddress,
    IN ULONG NumberPages
);

typedef
ULONG
(DDKAPI *pKdGetPciDataByOffset)(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

typedef
ULONG
(DDKAPI *pKdSetPciDataByOffset)(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

typedef BOOLEAN
(DDKAPI *PHAL_RESET_DISPLAY_PARAMETERS)(
  ULONG Columns, ULONG Rows);

typedef struct {
  ULONG  Version;
  pHalQuerySystemInformation  HalQuerySystemInformation;
  pHalSetSystemInformation  HalSetSystemInformation;
  pHalQueryBusSlots  HalQueryBusSlots;
  ULONG  Spare1;
  pHalExamineMBR  HalExamineMBR;
  pHalIoAssignDriveLetters  HalIoAssignDriveLetters;
  pHalIoReadPartitionTable  HalIoReadPartitionTable;
  pHalIoSetPartitionInformation  HalIoSetPartitionInformation;
  pHalIoWritePartitionTable  HalIoWritePartitionTable;
  pHalHandlerForBus  HalReferenceHandlerForBus;
  pHalReferenceBusHandler  HalReferenceBusHandler;
  pHalReferenceBusHandler  HalDereferenceBusHandler;
  pHalInitPnpDriver  HalInitPnpDriver;
  pHalInitPowerManagement  HalInitPowerManagement;
  pHalGetDmaAdapter  HalGetDmaAdapter;
  pHalGetInterruptTranslator  HalGetInterruptTranslator;
  pHalStartMirroring  HalStartMirroring;
  pHalEndMirroring  HalEndMirroring;
  pHalMirrorPhysicalMemory  HalMirrorPhysicalMemory;
  pHalEndOfBoot  HalEndOfBoot;
  pHalMirrorVerify  HalMirrorVerify;
} HAL_DISPATCH, *PHAL_DISPATCH;

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTHAL_)
extern NTSYSAPI PHAL_DISPATCH HalDispatchTable;
#define HALDISPATCH ((PHAL_DISPATCH)&HalDispatchTable)
#else
extern __declspec(dllexport) HAL_DISPATCH HalDispatchTable;
#define HALDISPATCH (&HalDispatchTable)
#endif

#define HAL_DISPATCH_VERSION            3
#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots
#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler
#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement
#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator
#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify

typedef struct _FILE_ALIGNMENT_INFORMATION {
  ULONG  AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
  ULONG  FileNameLength;
  WCHAR  FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;


typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
  ULONG  FileAttributes;
  ULONG  ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
  BOOLEAN  DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
  LARGE_INTEGER  EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
  LARGE_INTEGER  ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef union _FILE_SEGMENT_ELEMENT {
    PVOID64 Buffer;
    ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

#define SE_UNSOLICITED_INPUT_PRIVILEGE    6

typedef struct _KEY_USER_FLAGS_INFORMATION {
  ULONG  UserFlags;
} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION;

#define PCI_ADDRESS_MEMORY_SPACE            0x00000000

typedef struct _OSVERSIONINFOA {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    CHAR   szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OSVERSIONINFOW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR  szCSDVersion[128];
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

#ifdef UNICODE
typedef OSVERSIONINFOW OSVERSIONINFO;
typedef POSVERSIONINFOW POSVERSIONINFO;
typedef LPOSVERSIONINFOW LPOSVERSIONINFO;
#else
typedef OSVERSIONINFOA OSVERSIONINFO;
typedef POSVERSIONINFOA POSVERSIONINFO;
typedef LPOSVERSIONINFOA LPOSVERSIONINFO;
#endif // UNICODE

typedef struct _OSVERSIONINFOEXA {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    CHAR   szCSDVersion[128];
    USHORT wServicePackMajor;
    USHORT wServicePackMinor;
    USHORT wSuiteMask;
    UCHAR wProductType;
    UCHAR wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;

typedef struct _OSVERSIONINFOEXW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR  szCSDVersion[128];
    USHORT wServicePackMajor;
    USHORT wServicePackMinor;
    USHORT wSuiteMask;
    UCHAR wProductType;
    UCHAR wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

#ifdef UNICODE
typedef OSVERSIONINFOEXW OSVERSIONINFOEX;
typedef POSVERSIONINFOEXW POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXW LPOSVERSIONINFOEX;
#else
typedef OSVERSIONINFOEXA OSVERSIONINFOEX;
typedef POSVERSIONINFOEXA POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXA LPOSVERSIONINFOEX;
#endif // UNICODE

NTSYSAPI
ULONGLONG
DDKAPI
VerSetConditionMask(
  IN ULONGLONG  ConditionMask,
  IN ULONG  TypeMask,
  IN UCHAR  Condition);

#define VER_SET_CONDITION(ConditionMask, TypeBitMask, ComparisonType)  \
        ((ConditionMask) = VerSetConditionMask((ConditionMask), \
        (TypeBitMask), (ComparisonType)))

/* RtlVerifyVersionInfo() TypeMask */

#define VER_MINORVERSION                  0x0000001
#define VER_MAJORVERSION                  0x0000002
#define VER_BUILDNUMBER                   0x0000004
#define VER_PLATFORMID                    0x0000008
#define VER_SERVICEPACKMINOR              0x0000010
#define VER_SERVICEPACKMAJOR              0x0000020
#define VER_SUITENAME                     0x0000040
#define VER_PRODUCT_TYPE                  0x0000080

/* RtlVerifyVersionInfo() ComparisonType */

#define VER_EQUAL                       1
#define VER_GREATER                     2
#define VER_GREATER_EQUAL               3
#define VER_LESS                        4
#define VER_LESS_EQUAL                  5
#define VER_AND                         6
#define VER_OR                          7

#define VER_CONDITION_MASK              7
#define VER_NUM_BITS_PER_CONDITION_MASK 3

struct _RTL_RANGE;

typedef BOOLEAN
(NTAPI *PRTL_CONFLICT_RANGE_CALLBACK) (
    PVOID Context,
    struct _RTL_RANGE *Range
);

typedef struct _CONFIGURATION_INFORMATION {
  ULONG  DiskCount;
  ULONG  FloppyCount;
  ULONG  CdRomCount;
  ULONG  TapeCount;
  ULONG  ScsiPortCount;
  ULONG  SerialCount;
  ULONG  ParallelCount;
  BOOLEAN  AtDiskPrimaryAddressClaimed;
  BOOLEAN  AtDiskSecondaryAddressClaimed;
  ULONG  Version;
  ULONG  MediumChangerCount;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef enum _CONFIGURATION_TYPE {
  ArcSystem,
  CentralProcessor,
  FloatingPointProcessor,
  PrimaryIcache,
  PrimaryDcache,
  SecondaryIcache,
  SecondaryDcache,
  SecondaryCache,
  EisaAdapter,
  TcAdapter,
  ScsiAdapter,
  DtiAdapter,
  MultiFunctionAdapter,
  DiskController,
  TapeController,
  CdromController,
  WormController,
  SerialController,
  NetworkController,
  DisplayController,
  ParallelController,
  PointerController,
  KeyboardController,
  AudioController,
  OtherController,
  DiskPeripheral,
  FloppyDiskPeripheral,
  TapePeripheral,
  ModemPeripheral,
  MonitorPeripheral,
  PrinterPeripheral,
  PointerPeripheral,
  KeyboardPeripheral,
  TerminalPeripheral,
  OtherPeripheral,
  LinePeripheral,
  NetworkPeripheral,
  SystemMemory,
  DockingInformation,
  RealModeIrqRoutingTable,
  RealModePCIEnumeration,
  MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

typedef NTSTATUS
(DDKAPI *PIO_QUERY_DEVICE_ROUTINE)(
  IN PVOID  Context,
  IN PUNICODE_STRING  PathName,
  IN INTERFACE_TYPE  BusType,
  IN ULONG  BusNumber,
  IN PKEY_VALUE_FULL_INFORMATION  *BusInformation,
  IN CONFIGURATION_TYPE  ControllerType,
  IN ULONG  ControllerNumber,
  IN PKEY_VALUE_FULL_INFORMATION  *ControllerInformation,
  IN CONFIGURATION_TYPE  PeripheralType,
  IN ULONG  PeripheralNumber,
  IN PKEY_VALUE_FULL_INFORMATION  *PeripheralInformation);

typedef enum _IO_QUERY_DEVICE_DATA_FORMAT {
  IoQueryDeviceIdentifier = 0,
  IoQueryDeviceConfigurationData,
  IoQueryDeviceComponentInformation,
  IoQueryDeviceMaxData
} IO_QUERY_DEVICE_DATA_FORMAT, *PIO_QUERY_DEVICE_DATA_FORMAT;

typedef VOID
(DDKAPI *PCREATE_PROCESS_NOTIFY_ROUTINE)(
  IN HANDLE  ParentId,
  IN HANDLE  ProcessId,
  IN BOOLEAN  Create);

typedef VOID
(DDKAPI *PCREATE_THREAD_NOTIFY_ROUTINE)(
  IN HANDLE  ProcessId,
  IN HANDLE  ThreadId,
  IN BOOLEAN  Create);

typedef struct _IMAGE_INFO {
  _ANONYMOUS_UNION union {
    ULONG  Properties;
    _ANONYMOUS_STRUCT struct {
      ULONG  ImageAddressingMode  : 8;
      ULONG  SystemModeImage      : 1;
      ULONG  ImageMappedToAllPids : 1;
      ULONG  Reserved             : 22;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  PVOID  ImageBase;
  ULONG  ImageSelector;
  SIZE_T  ImageSize;
  ULONG  ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

#define IMAGE_ADDRESSING_MODE_32BIT       3

typedef VOID
(DDKAPI *PLOAD_IMAGE_NOTIFY_ROUTINE)(
  IN PUNICODE_STRING  FullImageName,
  IN HANDLE  ProcessId,
  IN PIMAGE_INFO  ImageInfo);

#pragma pack(push,4)
typedef enum _BUS_DATA_TYPE {
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    SgiInternalConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;
#pragma pack(pop)

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
	_ANONYMOUS_UNION union {
		PVOID FiberData;
		ULONG Version;
	} DUMMYUNIONNAME;
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _NT_TIB32 {
	ULONG ExceptionList;
	ULONG StackBase;
	ULONG StackLimit;
	ULONG SubSystemTib;
	__GNU_EXTENSION union {
		ULONG FiberData;
		ULONG Version;
	};
	ULONG ArbitraryUserPointer;
	ULONG Self;
} NT_TIB32,*PNT_TIB32;

typedef struct _NT_TIB64 {
	ULONG64 ExceptionList;
	ULONG64 StackBase;
	ULONG64 StackLimit;
	ULONG64 SubSystemTib;
	__GNU_EXTENSION union {
		ULONG64 FiberData;
		ULONG Version;
	};
	ULONG64 ArbitraryUserPointer;
	ULONG64 Self;
} NT_TIB64,*PNT_TIB64;

typedef enum _PROCESSINFOCLASS {
  ProcessBasicInformation,
  ProcessQuotaLimits,
  ProcessIoCounters,
  ProcessVmCounters,
  ProcessTimes,
  ProcessBasePriority,
  ProcessRaisePriority,
  ProcessDebugPort,
  ProcessExceptionPort,
  ProcessAccessToken,
  ProcessLdtInformation,
  ProcessLdtSize,
  ProcessDefaultHardErrorMode,
  ProcessIoPortHandlers,
  ProcessPooledUsageAndLimits,
  ProcessWorkingSetWatch,
  ProcessUserModeIOPL,
  ProcessEnableAlignmentFaultFixup,
  ProcessPriorityClass,
  ProcessWx86Information,
  ProcessHandleCount,
  ProcessAffinityMask,
  ProcessPriorityBoost,
  ProcessDeviceMap,
  ProcessSessionInformation,
  ProcessForegroundInformation,
  ProcessWow64Information,
  ProcessImageFileName,
  ProcessLUIDDeviceMapsEnabled,
  ProcessBreakOnTermination,
  ProcessDebugObjectHandle,
  ProcessDebugFlags,
  ProcessHandleTracing,
  ProcessIoPriority,
  ProcessExecuteFlags,
  ProcessTlsInformation,
  ProcessCookie,
  ProcessImageInformation,
  ProcessCycleTime,
  ProcessPagePriority,
  ProcessInstrumentationCallback,
  MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
  ThreadBasicInformation,
  ThreadTimes,
  ThreadPriority,
  ThreadBasePriority,
  ThreadAffinityMask,
  ThreadImpersonationToken,
  ThreadDescriptorTableEntry,
  ThreadEnableAlignmentFaultFixup,
  ThreadEventPair_Reusable,
  ThreadQuerySetWin32StartAddress,
  ThreadZeroTlsCell,
  ThreadPerformanceCount,
  ThreadAmILastThread,
  ThreadIdealProcessor,
  ThreadPriorityBoost,
  ThreadSetTlsArrayAddress,
  ThreadIsIoPending,
  ThreadHideFromDebugger,
  ThreadBreakOnTermination,
  ThreadSwitchLegacyState,
  ThreadIsTerminated,
  ThreadLastSystemCall,
  ThreadIoPriority,
  ThreadCycleTime,
  ThreadPagePriority,
  ThreadActualBasePriority,
  MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    struct _PEB *PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    __GNU_EXTENSION union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

/*
** Storage structures
*/
typedef enum _PARTITION_STYLE {
  PARTITION_STYLE_MBR,
  PARTITION_STYLE_GPT,
  PARTITION_STYLE_RAW
} PARTITION_STYLE;

typedef struct _CREATE_DISK_MBR {
  ULONG  Signature;
} CREATE_DISK_MBR, *PCREATE_DISK_MBR;

typedef struct _CREATE_DISK_GPT {
  GUID  DiskId;
  ULONG  MaxPartitionCount;
} CREATE_DISK_GPT, *PCREATE_DISK_GPT;

typedef struct _CREATE_DISK {
  PARTITION_STYLE  PartitionStyle;
  _ANONYMOUS_UNION union {
    CREATE_DISK_MBR  Mbr;
    CREATE_DISK_GPT  Gpt;
  } DUMMYUNIONNAME;
} CREATE_DISK, *PCREATE_DISK;

typedef struct _DISK_SIGNATURE {
  ULONG  PartitionStyle;
  _ANONYMOUS_UNION union {
    struct {
      ULONG  Signature;
      ULONG  CheckSum;
    } Mbr;
    struct {
      GUID  DiskId;
    } Gpt;
  } DUMMYUNIONNAME;
} DISK_SIGNATURE, *PDISK_SIGNATURE;

typedef VOID
(FASTCALL*PTIME_UPDATE_NOTIFY_ROUTINE)(
  IN HANDLE  ThreadId,
  IN KPROCESSOR_MODE  Mode);

typedef struct _PHYSICAL_MEMORY_RANGE {
  PHYSICAL_ADDRESS  BaseAddress;
  LARGE_INTEGER  NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

typedef ULONG_PTR
(NTAPI *PDRIVER_VERIFIER_THUNK_ROUTINE)(
  IN PVOID  Context);

typedef struct _DRIVER_VERIFIER_THUNK_PAIRS {
  PDRIVER_VERIFIER_THUNK_ROUTINE  PristineRoutine;
  PDRIVER_VERIFIER_THUNK_ROUTINE  NewRoutine;
} DRIVER_VERIFIER_THUNK_PAIRS, *PDRIVER_VERIFIER_THUNK_PAIRS;

#define DRIVER_VERIFIER_SPECIAL_POOLING             0x0001
#define DRIVER_VERIFIER_FORCE_IRQL_CHECKING         0x0002
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES  0x0004
#define DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS      0x0008
#define DRIVER_VERIFIER_IO_CHECKING                 0x0010

typedef VOID
(DDKAPI *PTIMER_APC_ROUTINE)(
  IN PVOID  TimerContext,
  IN ULONG  TimerLowValue,
  IN LONG  TimerHighValue);

/*
** Architecture specific structures
*/
#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

#ifdef _X86_

#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)

typedef struct _FLOATING_SAVE_AREA {
    ULONG ControlWord;
    ULONG StatusWord;
    ULONG TagWord;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

typedef struct _CONTEXT {
    ULONG ContextFlags;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    FLOATING_SAVE_AREA FloatSave;
    ULONG SegGs;
    ULONG SegFs;
    ULONG SegEs;
    ULONG SegDs;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG Ebp;
    ULONG Eip;
    ULONG SegCs;
    ULONG EFlags;
    ULONG Esp;
    ULONG SegSs;
    UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;

typedef struct _KPCR_TIB {
  PVOID  ExceptionList;         /* 00 */
  PVOID  StackBase;             /* 04 */
  PVOID  StackLimit;            /* 08 */
  PVOID  SubSystemTib;          /* 0C */
  _ANONYMOUS_UNION union {
    PVOID  FiberData;           /* 10 */
    ULONG  Version;             /* 10 */
  } DUMMYUNIONNAME;
  PVOID  ArbitraryUserPointer;  /* 14 */
  struct _KPCR_TIB *Self;       /* 18 */
} KPCR_TIB, *PKPCR_TIB;         /* 1C */

typedef struct _KPCR {
  KPCR_TIB  Tib;                /* 00 */
  struct _KPCR  *Self;          /* 1C */
  struct _KPRCB  *Prcb;         /* 20 */
  KIRQL  Irql;                  /* 24 */
  ULONG  IRR;                   /* 28 */
  ULONG  IrrActive;             /* 2C */
  ULONG  IDR;                   /* 30 */
  PVOID  KdVersionBlock;        /* 34 */
  PUSHORT  IDT;                 /* 38 */
  PUSHORT  GDT;                 /* 3C */
  struct _KTSS  *TSS;           /* 40 */
  USHORT  MajorVersion;         /* 44 */
  USHORT  MinorVersion;         /* 46 */
  KAFFINITY  SetMember;         /* 48 */
  ULONG  StallScaleFactor;      /* 4C */
  UCHAR  SpareUnused;           /* 50 */
  UCHAR  Number;                /* 51 */
  UCHAR Spare0;
  UCHAR SecondLevelCacheAssociativity;
  ULONG VdmAlert;
  ULONG KernelReserved[14];         // For use by the kernel
  ULONG SecondLevelCacheSize;
  ULONG HalReserved[16];            // For use by Hal
} KPCR, *PKPCR;                 /* 54 */

#define KeGetPcr()                      PCR

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readfsbyte(FIELD_OFFSET(KPCR, Number));
}

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG_PTR MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS            (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS          (PVOID)0xC0C00000

#define MM_KSEG0_BASE       MM_SYSTEM_RANGE_START
#define MM_SYSTEM_SPACE_END 0xFFFFFFFF
    
#elif defined(__x86_64__)

#define CONTEXT_AMD64 0x100000
#if !defined(RC_INVOKED)
#define CONTEXT_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT (CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000
#define CONTEXT_SERVICE_ACTIVE 0x10000000
#define CONTEXT_EXCEPTION_REQUEST 0x40000000
#define CONTEXT_EXCEPTION_REPORTING 0x80000000
#endif

typedef struct DECLSPEC_ALIGN(16) _CONTEXT {
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;

    /* Control flags */
    ULONG ContextFlags;
    ULONG MxCsr;

    /* Segment */
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;

    /* Debug */
    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    /* Integer */
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

    /* Counter */
    ULONG64 Rip;

   /* Floating point */
   union {
       XMM_SAVE_AREA32 FltSave;
       struct {
           M128A Header[2];
           M128A Legacy[8];
           M128A Xmm0;
           M128A Xmm1;
           M128A Xmm2;
           M128A Xmm3;
           M128A Xmm4;
           M128A Xmm5;
           M128A Xmm6;
           M128A Xmm7;
           M128A Xmm8;
           M128A Xmm9;
           M128A Xmm10;
           M128A Xmm11;
           M128A Xmm12;
           M128A Xmm13;
           M128A Xmm14;
           M128A Xmm15;
      } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

     /* Vector */
    M128A VectorRegister[26];
    ULONG64 VectorControl;

    /* Debug control */
    ULONG64 DebugControl;
    ULONG64 LastBranchToRip;
    ULONG64 LastBranchFromRip;
    ULONG64 LastExceptionToRip;
    ULONG64 LastExceptionFromRip;
} CONTEXT;

#define PAGE_SIZE   0x1000
#define PAGE_SHIFT 12L
#define PTI_SHIFT  12L
#define PDI_SHIFT  21L
#define PPI_SHIFT  30L
#define PXI_SHIFT  39L
#define PTE_PER_PAGE 512
#define PDE_PER_PAGE 512
#define PPE_PER_PAGE 512
#define PXE_PER_PAGE 512
#define PTI_MASK_AMD64 (PTE_PER_PAGE - 1)
#define PDI_MASK_AMD64 (PDE_PER_PAGE - 1)
#define PPI_MASK (PPE_PER_PAGE - 1)
#define PXI_MASK (PXE_PER_PAGE - 1)

#define PXE_BASE    0xFFFFF6FB7DBED000ULL
#define PXE_SELFMAP 0xFFFFF6FB7DBEDF68ULL
#define PPE_BASE    0xFFFFF6FB7DA00000ULL
#define PDE_BASE    0xFFFFF6FB40000000ULL
#define PTE_BASE    0xFFFFF68000000000ULL
#define PXE_TOP     0xFFFFF6FB7DBEDFFFULL
#define PPE_TOP     0xFFFFF6FB7DBFFFFFULL
#define PDE_TOP     0xFFFFF6FB7FFFFFFFULL
#define PTE_TOP     0xFFFFF6FFFFFFFFFFULL

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG_PTR MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS   (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xFFFF080000000000ULL
#define KI_USER_SHARED_DATA       0xFFFFF78000000000ULL

#define SharedUserData ((PKUSER_SHARED_DATA const)KI_USER_SHARED_DATA)
#define SharedInterruptTime (&SharedUserData->InterruptTime)
#define SharedSystemTime (&SharedUserData->SystemTime)
#define SharedTickCount (&SharedUserData->TickCount)

#define KeQueryInterruptTime() \
    (*(volatile ULONG64*)SharedInterruptTime)
#define KeQuerySystemTime(CurrentCount) \
    *(ULONG64*)(CurrentCount) = *(volatile ULONG64*)SharedSystemTime
#define KeQueryTickCount(CurrentCount) \
    *(ULONG64*)(CurrentCount) = *(volatile ULONG64*)SharedTickCount

typedef struct _KPCR
{
    __GNU_EXTENSION union
    {
        NT_TIB NtTib;
        __GNU_EXTENSION struct
        {
            union _KGDTENTRY64 *GdtBase;
            struct _KTSS64 *TssBase;
            ULONG64 UserRsp;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    union _KIDTENTRY64 *IdtBase;
    ULONG64 Unused[2];
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR ObsoleteNumber;
    UCHAR Fill0;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
    ULONG Unused2;
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[24];
} KPCR, *PKPCR;

typedef struct _KFLOATING_SAVE {
  ULONG Dummy;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

FORCEINLINE
PKPCR
KeGetPcr(VOID)
{
    return (PKPCR)__readgsqword(FIELD_OFFSET(KPCR, Self));
}

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readgsword(0x184);
}

#elif defined(__PowerPC__)

//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;

#define PASSIVE_LEVEL                      0
#define LOW_LEVEL                          0
#define APC_LEVEL                          1
#define DISPATCH_LEVEL                     2
#define PROFILE_LEVEL                     27
#define CLOCK1_LEVEL                      28
#define CLOCK2_LEVEL                      28
#define IPI_LEVEL                         29
#define POWER_LEVEL                       30
#define HIGH_LEVEL                        31

typedef struct _KFLOATING_SAVE {
  ULONG Dummy;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

typedef struct _KPCR_TIB {
  PVOID  ExceptionList;         /* 00 */
  PVOID  StackBase;             /* 04 */
  PVOID  StackLimit;            /* 08 */
  PVOID  SubSystemTib;          /* 0C */
  _ANONYMOUS_UNION union {
    PVOID  FiberData;           /* 10 */
    ULONG  Version;             /* 10 */
  } DUMMYUNIONNAME;
  PVOID  ArbitraryUserPointer;  /* 14 */
  struct _KPCR_TIB *Self;       /* 18 */
} KPCR_TIB, *PKPCR_TIB;         /* 1C */

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {
  KPCR_TIB  Tib;                /* 00 */
  struct _KPCR  *Self;          /* 1C */
  struct _KPRCB  *Prcb;         /* 20 */
  KIRQL  Irql;                  /* 24 */
  ULONG  IRR;                   /* 28 */
  ULONG  IrrActive;             /* 2C */
  ULONG  IDR;                   /* 30 */
  PVOID  KdVersionBlock;        /* 34 */
  PUSHORT  IDT;                 /* 38 */
  PUSHORT  GDT;                 /* 3C */
  struct _KTSS  *TSS;           /* 40 */
  USHORT  MajorVersion;         /* 44 */
  USHORT  MinorVersion;         /* 46 */
  KAFFINITY  SetMember;         /* 48 */
  ULONG  StallScaleFactor;      /* 4C */
  UCHAR  SpareUnused;           /* 50 */
  UCHAR  Number;                /* 51 */
} KPCR, *PKPCR;                 /* 54 */

#define KeGetPcr()                      PCR

static __inline
ULONG
DDKAPI
KeGetCurrentProcessorNumber(VOID)
{
    ULONG Number;
  __asm__ __volatile__ (
    "lwz %0, %c1(12)\n"
    : "=r" (Number)
    : "i" (FIELD_OFFSET(KPCR, Number))
  );
  return Number;
}

#elif defined(_MIPS_)

#error MIPS Headers are totally incorrect

//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;

#define PASSIVE_LEVEL                      0
#define APC_LEVEL                          1
#define DISPATCH_LEVEL                     2
#define PROFILE_LEVEL                     27
#define IPI_LEVEL                         29
#define HIGH_LEVEL                        31

typedef struct _KPCR {
    struct _KPRCB  *Prcb;         /* 20 */
    KIRQL  Irql;                  /* 24 */
    ULONG  IRR;                   /* 28 */
    ULONG  IDR;                   /* 30 */
} KPCR, *PKPCR;

#define KeGetPcr()                      PCR

typedef struct _KFLOATING_SAVE {
} KFLOATING_SAVE, *PKFLOATING_SAVE;

static __inline
ULONG
DDKAPI
KeGetCurrentProcessorNumber(VOID)
{
    return 0;
}

#elif defined(_M_ARM)

//
// NT-ARM is not documented, need DDK-ARM
//
#include <armddk.h>

#else
#error Unknown architecture
#endif

typedef enum _INTERLOCKED_RESULT {
  ResultNegative = RESULT_NEGATIVE,
  ResultZero = RESULT_ZERO,
  ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

typedef VOID
(NTAPI *PciPin2Line)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PPCI_COMMON_CONFIG PciData
);

typedef VOID
(NTAPI *PciLine2Pin)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PPCI_COMMON_CONFIG PciNewData,
    IN PPCI_COMMON_CONFIG PciOldData
);

typedef VOID
(NTAPI *PciReadWriteConfig)(
    IN struct _BUS_HANDLER *BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

#define PCI_DATA_TAG ' ICP'
#define PCI_DATA_VERSION 1

typedef struct _PCIBUSDATA
{
    ULONG Tag;
    ULONG Version;
    PciReadWriteConfig ReadConfig;
    PciReadWriteConfig WriteConfig;
    PciPin2Line Pin2Line;
    PciLine2Pin Line2Pin;
    PCI_SLOT_NUMBER ParentSlot;
    PVOID Reserved[4];
} PCIBUSDATA, *PPCIBUSDATA;


/** SPINLOCK FUNCTIONS ********************************************************/

#if defined (_X86_)

#if defined(WIN9X_COMPAT_SPINLOCK)

NTKERNELAPI
VOID
NTAPI
KeInitializeSpinLock(
    IN PKSPIN_LOCK SpinLock
);

#else

FORCEINLINE
VOID
KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Clear the lock */
    *SpinLock = 0;
}

#endif

NTHALAPI
KIRQL
FASTCALL
KfAcquireSpinLock(
  IN PKSPIN_LOCK SpinLock);

NTHALAPI
VOID
FASTCALL
KfReleaseSpinLock(
  IN PKSPIN_LOCK SpinLock,
  IN KIRQL NewIrql);

NTKERNELAPI
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(
  IN PKSPIN_LOCK  SpinLock);

NTKERNELAPI
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(
  IN PKSPIN_LOCK  SpinLock);

#define KeAcquireSpinLockAtDpcLevel(SpinLock) KefAcquireSpinLockAtDpcLevel(SpinLock)
#define KeReleaseSpinLockFromDpcLevel(SpinLock) KefReleaseSpinLockFromDpcLevel(SpinLock)
#define KeAcquireSpinLock(a,b)  *(b) = KfAcquireSpinLock(a)
#define KeReleaseSpinLock(a,b)  KfReleaseSpinLock(a,b)

#define KeGetDcacheFillSize() 1L
    
#elif defined(_M_ARM) // !defined (_X86_)
    
    FORCEINLINE
    VOID
    KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
    {
        /* Clear the lock */
        *SpinLock = 0;
    }
    
    NTHALAPI
    KIRQL
    FASTCALL
    KfAcquireSpinLock(
                      IN PKSPIN_LOCK SpinLock);
    
    NTHALAPI
    VOID
    FASTCALL
    KfReleaseSpinLock(
                      IN PKSPIN_LOCK SpinLock,
                      IN KIRQL NewIrql);
    
    
    NTKERNELAPI
    VOID
    FASTCALL
    KefAcquireSpinLockAtDpcLevel(
                                 IN PKSPIN_LOCK  SpinLock);
    
    NTKERNELAPI
    VOID
    FASTCALL
    KefReleaseSpinLockFromDpcLevel(
                                   IN PKSPIN_LOCK  SpinLock);
    
    
#define KeAcquireSpinLockAtDpcLevel(SpinLock) KefAcquireSpinLockAtDpcLevel(SpinLock)
#define KeReleaseSpinLockFromDpcLevel(SpinLock) KefReleaseSpinLockFromDpcLevel(SpinLock)
#define KeAcquireSpinLock(a,b)  *(b) = KfAcquireSpinLock(a)
#define KeReleaseSpinLock(a,b)  KfReleaseSpinLock(a,b)
    
    NTKERNELAPI
    VOID
    NTAPI
    KeInitializeSpinLock(
                         IN PKSPIN_LOCK  SpinLock);
    
#else
    
FORCEINLINE
VOID
NTAPI
KeInitializeSpinLock(
  PKSPIN_LOCK SpinLock)
{
    *SpinLock = 0;
}

NTKERNELAPI
VOID
KeReleaseSpinLock(
  IN PKSPIN_LOCK SpinLock,
  IN KIRQL NewIrql);

NTKERNELAPI
VOID
KeAcquireSpinLockAtDpcLevel(
  IN PKSPIN_LOCK SpinLock);

NTKERNELAPI
VOID
KeReleaseSpinLockFromDpcLevel(
  IN PKSPIN_LOCK SpinLock);

NTKERNELAPI
KIRQL
KeAcquireSpinLockRaiseToDpc(
  IN PKSPIN_LOCK SpinLock);

#define KeAcquireSpinLock(SpinLock, OldIrql) \
  *(OldIrql) = KeAcquireSpinLockRaiseToDpc(SpinLock)
    
#endif // !defined (_X86_)

#define ARGUMENT_PRESENT(ArgumentPointer) \
  ((CHAR*)((ULONG_PTR)(ArgumentPointer)) != (CHAR*)NULL)

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  IN PCSZ  String,
  IN ULONG  Base  OPTIONAL,
  IN OUT PULONG  Value);

NTSYSAPI
LONG
NTAPI
RtlCompareString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  BOOLEAN  CaseInSensitive);

#if !defined(MIDL_PASS)

FORCEINLINE
LUID
NTAPI
RtlConvertLongToLuid(
    IN LONG Val)
{
    LUID Luid;
    LARGE_INTEGER Temp;

    Temp.QuadPart = Val;
    Luid.LowPart = Temp.u.LowPart;
    Luid.HighPart = Temp.u.HighPart;

    return Luid;
}

FORCEINLINE
LUID
NTAPI
RtlConvertUlongToLuid(
    IN ULONG Val)
{
    LUID Luid;

    Luid.LowPart = Val;
    Luid.HighPart = 0;

    return Luid;
}
#endif


NTSYSAPI
VOID
NTAPI
RtlCopyMemory32(
  IN VOID UNALIGNED  *Destination,
  IN CONST VOID UNALIGNED  *Source,
  IN ULONG  Length);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
  IN OUT PSTRING  DestinationString,
  IN PSTRING  SourceString  OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  IN BOOLEAN  CaseInSensitive);

#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)
#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;
#else
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
  OUT PVOID  *CallersAddress,
  OUT PVOID  *CallersCaller);
#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
  IN OUT PRTL_OSVERSIONINFOW  lpVersionInformation);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK  AccessMask,
  IN PGENERIC_MAPPING  GenericMapping);

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
  IN PCUNICODE_STRING  String1,
  IN PCUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString  OPTIONAL,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
  IN CHAR Character);

NTSYSAPI
VOID
NTAPI
RtlUpperString(
  IN OUT PSTRING  DestinationString,
  IN PSTRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
  IN PRTL_OSVERSIONINFOEXW  VersionInfo,
  IN ULONG  TypeMask,
  IN ULONGLONG  ConditionMask);

NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
  IN PVOID  VolumeDeviceObject,
  OUT PUNICODE_STRING  DosName);

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
  OUT PVOID  *Callers,
  IN ULONG  Count,
  IN ULONG  Flags);

/******************************************************************************
 *                            Executive Types                                 *
 ******************************************************************************/

typedef struct _ZONE_SEGMENT_HEADER {
  SINGLE_LIST_ENTRY  SegmentList;
  PVOID  Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
  SINGLE_LIST_ENTRY  FreeList;
  SINGLE_LIST_ENTRY  SegmentList;
  ULONG  BlockSize;
  ULONG  TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

#define PROTECTED_POOL                    0x80000000

/******************************************************************************
 *                          Executive Functions                               *
 ******************************************************************************/

NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Segment,
  IN ULONG  SegmentSize);

static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER  Zone)
{
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return (PVOID) Zone->FreeList.Next;
}

static __inline PVOID
ExFreeToZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Block)
{
  ((PSINGLE_LIST_ENTRY) Block)->Next = Zone->FreeList.Next;
  Zone->FreeList.Next = ((PSINGLE_LIST_ENTRY) Block);
  return ((PSINGLE_LIST_ENTRY) Block)->Next;
}

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  IN PZONE_HEADER  Zone,
  IN ULONG  BlockSize,
  IN PVOID  InitialSegment,
  IN ULONG  InitialSegmentSize);

/*
 * PVOID
 * ExInterlockedAllocateFromZone(
 *   IN PZONE_HEADER  Zone,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedAllocateFromZone(Zone, Lock) \
    ((PVOID) ExInterlockedPopEntryList(&Zone->FreeList, Lock))

NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Segment,
  IN ULONG  SegmentSize,
  IN PKSPIN_LOCK  Lock);

/* PVOID
 * ExInterlockedFreeToZone(
 *  IN PZONE_HEADER  Zone,
 *  IN PVOID  Block,
 *  IN PKSPIN_LOCK  Lock);
 */
#define ExInterlockedFreeToZone(Zone, Block, Lock) \
    ExInterlockedPushEntryList(&(Zone)->FreeList, (PSINGLE_LIST_ENTRY)(Block), Lock)

/*
 * BOOLEAN
 * ExIsFullZone(
 *  IN PZONE_HEADER  Zone)
 */
#define ExIsFullZone(Zone) \
  ((Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY) NULL)

/* BOOLEAN
 * ExIsObjectInFirstZoneSegment(
 *     IN PZONE_HEADER Zone,
 *     IN PVOID Object);
 */
#define ExIsObjectInFirstZoneSegment(Zone,Object) \
    ((BOOLEAN)( ((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) && \
                ((PUCHAR)(Object) <  (PUCHAR)(Zone)->SegmentList.Next + \
                         (Zone)->TotalSegmentSize)) )

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseAccessViolation(
  VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseDatatypeMisalignment(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  OUT UUID  *Uuid);

#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExInitializeResource ExInitializeResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite

/** Filesystem runtime library routines **/

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  IN NTSTATUS  Status);

/** Hardware abstraction layer routines **/

NTHALAPI
BOOLEAN
NTAPI
HalMakeBeep(
  IN ULONG Frequency);

NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  MBRTypeIdentifier,
  OUT PVOID  *Buffer);

VOID
NTAPI
HalPutDmaAdapter(
    PADAPTER_OBJECT AdapterObject
);

/** I/O manager routines **/

#ifndef DMA_MACROS_DEFINED
NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context);
#endif

NTKERNELAPI
VOID
NTAPI
IoAllocateController(
  IN PCONTROLLER_OBJECT  ControllerObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

NTKERNELAPI
VOID
NTAPI
IoCancelFileOpen(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PFILE_OBJECT  FileObject);

NTKERNELAPI
PCONTROLLER_OBJECT
NTAPI
IoCreateController(
  IN ULONG  Size);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDisk(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PCREATE_DISK  Disk);

NTKERNELAPI
VOID
NTAPI
IoDeleteController(
  IN PCONTROLLER_OBJECT  ControllerObject);

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

NTKERNELAPI
VOID
NTAPI
IoFreeController(
  IN PCONTROLLER_OBJECT  ControllerObject);

NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(
  VOID);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(
  IN PETHREAD  Thread);

NTKERNELAPI
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(
  VOID);

NTKERNELAPI
PIRP
NTAPI
IoMakeAssociatedIrp(
  IN PIRP  Irp,
  IN CCHAR  StackSize);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryDeviceDescription(
  IN PINTERFACE_TYPE  BusType  OPTIONAL,
  IN PULONG  BusNumber  OPTIONAL,
  IN PCONFIGURATION_TYPE  ControllerType  OPTIONAL,
  IN PULONG  ControllerNumber  OPTIONAL,
  IN PCONFIGURATION_TYPE  PeripheralType  OPTIONAL,
  IN PULONG  PeripheralNumber  OPTIONAL,
  IN PIO_QUERY_DEVICE_ROUTINE  CalloutRoutine,
  IN PVOID  Context);

NTKERNELAPI
VOID
NTAPI
IoRaiseHardError(
  IN PIRP  Irp,
  IN PVPB  Vpb  OPTIONAL,
  IN PDEVICE_OBJECT  RealDeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoRaiseInformationalHardError(
  IN NTSTATUS  ErrorStatus,
  IN PUNICODE_STRING  String  OPTIONAL,
  IN PKTHREAD  Thread  OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadDiskSignature(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  BytesPerSector,
  OUT PDISK_SIGNATURE  Signature);

NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN BOOLEAN  ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION  **PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadPartitionTableEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX  **PartitionBuffer);

NTKERNELAPI
VOID
NTAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTKERNELAPI
VOID
NTAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTKERNELAPI
VOID
NTAPI
IoRegisterDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportDetectedDevice(
  IN PDRIVER_OBJECT  DriverObject,
  IN INTERFACE_TYPE  LegacyBusType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PCM_RESOURCE_LIST  ResourceList,
  IN PIO_RESOURCE_REQUIREMENTS_LIST  ResourceRequirements  OPTIONAL,
  IN BOOLEAN  ResourceAssigned,
  IN OUT PDEVICE_OBJECT  *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceForDetection(
  IN PDRIVER_OBJECT  DriverObject,
  IN PCM_RESOURCE_LIST  DriverList  OPTIONAL,
  IN ULONG  DriverListSize  OPTIONAL,
  IN PDEVICE_OBJECT  DeviceObject  OPTIONAL,
  IN PCM_RESOURCE_LIST  DeviceList  OPTIONAL,
  IN ULONG  DeviceListSize  OPTIONAL,
  OUT PBOOLEAN  ConflictDetected);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceUsage(
  IN PUNICODE_STRING  DriverClassName  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PCM_RESOURCE_LIST  DriverList  OPTIONAL,
  IN ULONG  DriverListSize  OPTIONAL,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PCM_RESOURCE_LIST  DeviceList  OPTIONAL,
  IN ULONG  DeviceListSize  OPTIONAL,
  IN BOOLEAN  OverrideConflict,
  OUT PBOOLEAN  ConflictDetected);

NTKERNELAPI
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(
  IN PIRP  Irp,
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  PartitionNumber,
  IN ULONG  PartitionType);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetPartitionInformationEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  PartitionNumber,
  IN struct _SET_PARTITION_INFORMATION_EX  *PartitionInfo);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetSystemPartition(
  IN PUNICODE_STRING  VolumeNameString);

NTKERNELAPI
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(
  IN BOOLEAN  EnableHardErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyPartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  FixErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(
  IN  PVOID  VolumeDeviceObject,
  OUT PUNICODE_STRING  DosName);

NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  SectorsPerTrack,
  IN ULONG  NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION  *PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWritePartitionTableEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX  *PartitionBuffer);

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT AdapterObject,
  IN PVOID MapRegisterBase,
  IN ULONG NumberOfMapRegisters);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  IN PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT AdapterObject);

#endif

/** Kernel routines **/

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  IN ULONG  BugCheckCode);

#ifdef _X86_

static __inline
VOID
KeMemoryBarrier(
  VOID)
{
  volatile LONG Barrier;
#if defined(__GNUC__)
  __asm__ __volatile__ ("xchg %%eax, %0" : : "m" (Barrier) : "%eax");
#elif defined(_MSC_VER)
  __asm xchg [Barrier], eax
#endif
}

#endif

NTKERNELAPI
LONG
NTAPI
KePulseEvent(
  IN PRKEVENT  Event,
  IN KPRIORITY  Increment,
  IN BOOLEAN  Wait);

#if !defined(_M_AMD64)

NTKERNELAPI
VOID
NTAPI
KeQueryTickCount(
  OUT PLARGE_INTEGER  TickCount);
#endif

NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  IN PRKTHREAD  Thread,
  IN LONG  Increment);

NTKERNELAPI
VOID
FASTCALL
KeSetTimeUpdateNotifyRoutine(
  IN PTIME_UPDATE_NOTIFY_ROUTINE  NotifyRoutine);

#if defined(_X86_)

NTHALAPI
VOID
FASTCALL
KfLowerIrql(
  IN KIRQL  NewIrql);

NTHALAPI
KIRQL
FASTCALL
KfRaiseIrql(
  IN KIRQL  NewIrql);

NTHALAPI
KIRQL
DDKAPI
KeRaiseIrqlToDpcLevel(
  VOID);

NTHALAPI
KIRQL
DDKAPI
KeRaiseIrqlToSynchLevel(
    VOID);

#define KeLowerIrql(a) KfLowerIrql(a)
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

#elif defined(_M_AMD64)

FORCEINLINE
KIRQL
KeGetCurrentIrql(VOID)
{
    return (KIRQL)__readcr8();
}

FORCEINLINE
VOID
KeLowerIrql(IN KIRQL NewIrql)
{
    ASSERT(KeGetCurrentIrql() >= NewIrql);
    __writecr8(NewIrql);
}

FORCEINLINE
KIRQL
KfRaiseIrql(IN KIRQL NewIrql)
{
    KIRQL OldIrql;

    OldIrql = __readcr8();
    ASSERT(OldIrql <= NewIrql);
    __writecr8(NewIrql);
    return OldIrql;
}
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

FORCEINLINE
KIRQL
KeRaiseIrqlToDpcLevel(VOID)
{
    return KfRaiseIrql(DISPATCH_LEVEL);
}

FORCEINLINE
KIRQL
KeRaiseIrqlToSynchLevel(VOID)
{
    return KfRaiseIrql(12); // SYNCH_LEVEL = IPI_LEVEL - 2
}

#elif defined(__PowerPC__)

NTHALAPI
VOID
FASTCALL
KfLowerIrql(
  IN KIRQL  NewIrql);

NTHALAPI
KIRQL
FASTCALL
KfRaiseIrql(
  IN KIRQL  NewIrql);

NTHALAPI
KIRQL
DDKAPI
KeRaiseIrqlToDpcLevel(
  VOID);

NTHALAPI
KIRQL
DDKAPI
KeRaiseIrqlToSynchLevel(
    VOID);

#define KeLowerIrql(a) KfLowerIrql(a)
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

#elif defined(_M_MIPS)

#define KeLowerIrql(a) KfLowerIrql(a)
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

NTKERNELAPI
VOID
NTAPI
KfLowerIrql(
  IN KIRQL  NewIrql);

NTKERNELAPI
KIRQL
NTAPI
KfRaiseIrql(
  IN KIRQL  NewIrql);

NTKERNELAPI
KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(
  VOID);

NTKERNELAPI
KIRQL
DDKAPI
KeRaiseIrqlToSynchLevel(
    VOID);

#elif defined(_M_ARM)

#include <armddk.h>

#else

NTKERNELAPI
VOID
NTAPI
KeLowerIrql(
  IN KIRQL  NewIrql);

NTKERNELAPI
VOID
NTAPI
KeRaiseIrql(
  IN KIRQL  NewIrql,
  OUT PKIRQL  OldIrql);

NTKERNELAPI
KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(
  VOID);

NTKERNELAPI
KIRQL
DDKAPI
KeRaiseIrqlToSynchLevel(
    VOID);

#endif

/** Memory manager routines **/

NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  IN ULONG  NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTKERNELAPI
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(
  IN PVOID  BaseAddress);

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(
  VOID);

NTKERNELAPI
PVOID
NTAPI
MmGetVirtualForPhysical(
  IN PHYSICAL_ADDRESS  PhysicalAddress);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapUserAddressesToPage(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes,
  IN PVOID  PageAddress);

NTKERNELAPI
PVOID
NTAPI
MmMapVideoDisplay(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSessionSpace(
  IN PVOID  Section,
  OUT PVOID  *MappedBase,
  IN OUT PSIZE_T  ViewSize);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSystemSpace(
  IN PVOID  Section,
  OUT PVOID  *MappedBase,
  IN PSIZE_T  ViewSize);

NTKERNELAPI
NTSTATUS
NTAPI
MmMarkPhysicalMemoryAsBad(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

NTKERNELAPI
NTSTATUS
NTAPI
MmMarkPhysicalMemoryAsGood(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

/*
 * ULONG
 * ADDRESS_AND_SIZE_TO_SPAN_PAGES(
 *   IN PVOID  Va,
 *   IN ULONG  Size)
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(_Va, \
                                       _Size) \
  ((ULONG) ((((ULONG_PTR) (_Va) & (PAGE_SIZE - 1)) \
    + (_Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  IN PVOID  VirtualAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(
  VOID);

NTKERNELAPI
PVOID
NTAPI
MmLockPagableImageSection(
  IN PVOID  AddressWithinSection);

/*
 * PVOID
 * MmLockPagableCodeSection(
 *   IN PVOID  AddressWithinSection)
 */
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

NTKERNELAPI
VOID
NTAPI
MmLockPagableSectionByHandle(
  IN PVOID  ImageSectionHandle);

NTKERNELAPI
PVOID
NTAPI
MmLockPageableDataSection (
    IN PVOID AddressWithinSection
);

NTKERNELAPI
VOID
NTAPI
MmUnlockPageableImageSection(
    IN PVOID ImageSectionHandle
);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(
  IN PVOID  MappedBase);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(
  IN PVOID MappedBase);

NTKERNELAPI
VOID
NTAPI
MmUnsecureVirtualMemory(
  IN HANDLE  SecureHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  IN PVOID  Address,
  IN SIZE_T  Size,
  IN ULONG  ProbeMode);

NTKERNELAPI
VOID
NTAPI
MmUnmapVideoDisplay(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

/** Object manager routines **/

NTKERNELAPI
NTSTATUS
NTAPI
ObAssignSecurity(
  IN PACCESS_STATE  AccessState,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN PVOID  Object,
  IN POBJECT_TYPE  Type);

NTKERNELAPI
VOID
NTAPI
ObDereferenceSecurityDescriptor(
  PSECURITY_DESCRIPTOR  SecurityDescriptor,
  ULONG  Count);

NTKERNELAPI
NTSTATUS
NTAPI
ObLogSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  InputSecurityDescriptor,
  OUT PSECURITY_DESCRIPTOR  *OutputSecurityDescriptor,
  IN ULONG RefBias);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByName(
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN POBJECT_TYPE  ObjectType,
  IN KPROCESSOR_MODE  AccessMode,
  IN PACCESS_STATE  PassedAccessState,
  IN ACCESS_MASK  DesiredAccess,
  IN OUT PVOID  ParseContext  OPTIONAL,
  OUT PHANDLE  Handle);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByName(
  IN PUNICODE_STRING  ObjectPath,
  IN ULONG  Attributes,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess  OPTIONAL,
  IN POBJECT_TYPE  ObjectType,
  IN KPROCESSOR_MODE  AccessMode,
  IN OUT PVOID  ParseContext  OPTIONAL,
  OUT PVOID  *Object);

NTKERNELAPI
VOID
NTAPI
ObReferenceSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  Count);

/** Process manager routines **/

NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemProcess(
  IN PHANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(
  VOID);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentThreadId(
  VOID);

NTKERNELAPI
HANDLE
NTAPI
PsGetProcessId(PEPROCESS Process);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetVersion(
  PULONG  MajorVersion  OPTIONAL,
  PULONG  MinorVersion  OPTIONAL,
  PULONG  BuildNumber  OPTIONAL,
  PUNICODE_STRING  CSDVersion  OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE  NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE  NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE  NotifyRoutine,
  IN BOOLEAN  Remove);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE  NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE  NotifyRoutine);

extern NTSYSAPI PEPROCESS PsInitialSystemProcess;

/** Security reference monitor routines **/

NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  LUID  PrivilegeValue,
  KPROCESSOR_MODE  PreviousMode);

/** NtXxx and ZwXxx routines **/

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
  OUT PHANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PCLIENT_ID  ClientId  OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
  IN HANDLE  ProcessHandle,
  IN PROCESSINFOCLASS  ProcessInformationClass,
  OUT PVOID  ProcessInformation,
  IN ULONG  ProcessInformationLength,
  OUT PULONG  ReturnLength OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelTimer(
  IN HANDLE  TimerHandle,
  OUT PBOOLEAN  CurrentState  OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
  IN HANDLE  Handle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
  OUT PHANDLE  EventHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN EVENT_TYPE  EventType,
  IN BOOLEAN  InitialState);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
  OUT PHANDLE  EventHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN EVENT_TYPE  EventType,
  IN BOOLEAN  InitialState);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN TIMER_TYPE  TimerType);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
  IN HANDLE  DeviceHandle,
  IN HANDLE  Event  OPTIONAL,
  IN PIO_APC_ROUTINE  UserApcRoutine  OPTIONAL,
  IN PVOID  UserApcContext  OPTIONAL,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  IoControlCode,
  IN PVOID  InputBuffer,
  IN ULONG  InputBufferSize,
  OUT PVOID  OutputBuffer,
  IN ULONG  OutputBufferSize);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
  IN HANDLE  SectionHandle,
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN ULONG_PTR  ZeroBits,
  IN SIZE_T  CommitSize,
  IN OUT PLARGE_INTEGER  SectionOffset  OPTIONAL,
  IN OUT PSIZE_T  ViewSize,
  IN SECTION_INHERIT  InheritDisposition,
  IN ULONG  AllocationType,
  IN ULONG  Protect);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
  OUT PHANDLE  FileHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  ShareAccess,
  IN ULONG  OpenOptions);



NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
  OUT PHANDLE  FileHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  ShareAccess,
  IN ULONG  OpenOptions);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
  IN HANDLE  FileHandle,
  IN HANDLE  Event  OPTIONAL,
  IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
  IN PVOID  ApcContext  OPTIONAL,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  OUT PVOID  Buffer,
  IN ULONG  Length,
  IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
  IN PULONG  Key  OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
  IN HANDLE  EventHandle,
  OUT PLONG  PreviousState  OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
  IN HANDLE  EventHandle,
  OUT PLONG  PreviousState  OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
  IN HANDLE  ThreadHandle,
  IN THREADINFOCLASS  ThreadInformationClass,
  IN PVOID  ThreadInformation,
  IN ULONG  ThreadInformationLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimer(
  IN HANDLE  TimerHandle,
  IN PLARGE_INTEGER  DueTime,
  IN PTIMER_APC_ROUTINE  TimerApcRoutine  OPTIONAL,
  IN PVOID  TimerContext  OPTIONAL,
  IN BOOLEAN  WakeTimer,
  IN LONG  Period  OPTIONAL,
  OUT PBOOLEAN  PreviousState  OPTIONAL);

/* [Nt|Zw]MapViewOfSection.InheritDisposition constants */
#define AT_EXTENDABLE_FILE                0x00002000
#define AT_RESERVED                       0x20000000
#define AT_ROUND_TO_PAGE                  0x40000000

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
  IN HANDLE  ProcessHandle,
  IN PVOID  BaseAddress);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
  IN HANDLE  ObjectHandle,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  TimeOut  OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
  IN HANDLE  FileHandle,
  IN HANDLE  Event  OPTIONAL,
  IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
  IN PVOID  ApcContext  OPTIONAL,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN PVOID  Buffer,
  IN ULONG  Length,
  IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
  IN PULONG  Key  OPTIONAL);

/** Power management support routines **/

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestShutdownEvent(
  OUT PVOID  *Event);

/** WMI library support routines **/

NTSTATUS
NTAPI
WmiCompleteRequest(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN NTSTATUS  Status,
  IN ULONG  BufferUsed,
  IN CCHAR  PriorityBoost);

NTSTATUS
NTAPI
WmiFireEvent(
  IN PDEVICE_OBJECT  DeviceObject,
  IN LPGUID  Guid,
  IN ULONG  InstanceIndex,
  IN ULONG  EventDataSize,
  IN PVOID  EventData);

NTSTATUS
NTAPI
WmiSystemControl(
  IN PWMILIB_CONTEXT  WmiLibInfo,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  OUT PSYSCTL_IRP_DISPOSITION  IrpDisposition);

/** Kernel debugger routines **/

ULONG
NTAPI
DbgPrompt(
    IN PCCH Prompt,
    OUT PCH Response,
    IN ULONG MaximumResponseLength
);

/** Stuff from winnt4.h */

NTKERNELAPI
NTSTATUS
NTAPI
IoAssignResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject  OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST  RequestedResources,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceByPointer(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PDEVICE_OBJECT  TargetDevice);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID  VirtualAddress);

#if defined(_AMD64_) || defined(_IA64_)
//DECLSPEC_DEPRECATED_DDK_WINXP
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerDivide(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER Divisor,
    IN OUT PLARGE_INTEGER Remainder)
{
    LARGE_INTEGER ret;
    ret.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
    if (Remainder)
        Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
    return ret;
}
#else
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER  Dividend,
  IN LARGE_INTEGER  Divisor,
  IN OUT PLARGE_INTEGER  Remainder);
#endif

#ifndef _X86_
NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedDecrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
ULONG
NTAPI
ExInterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedIncrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);
#endif

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PWAIT_CONTEXT_BLOCK  Wcb,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine);

NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN INTERFACE_TYPE  BusType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION  DeviceDescription,
  IN OUT PULONG  NumberOfMapRegisters);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalGetDmaAlignmentRequirement(
  VOID);

NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN ULONG  BusInterruptLevel,
  IN ULONG  BusInterruptVector,
  OUT PKIRQL  Irql,
  OUT PKAFFINITY  Affinity);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN PHYSICAL_ADDRESS  BusAddress,
  IN OUT PULONG  AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThanZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThanZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerNegate(
  IN LARGE_INTEGER  Subtrahend);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerNotEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerNotEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftLeft(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftRight(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerSubtract(
  IN LARGE_INTEGER  Minuend,
  IN LARGE_INTEGER  Subtrahend);


/*
 * ULONG
 * COMPUTE_PAGES_SPANNED(
 *   IN PVOID  Va,
 *   IN ULONG  Size)
 */
#define COMPUTE_PAGES_SPANNED(Va, \
                              Size) \
  (ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Size))


/*
** Architecture specific structures
*/

#ifdef _X86_

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  IN OUT PLONG  volatile Addend);

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend);

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value);

#endif /* _X86_ */

#ifdef _M_ARM
//
// NT-ARM is not documented
//
#include <armddk.h>   
#endif

#ifdef __cplusplus
}
#endif

#endif /* __WINDDK_H */
