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

#if !defined(_NTHAL_)
#define NTHALAPI DECLSPEC_IMPORT
#else
#define NTHALAPI
#endif

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

#if defined(_WIN64)
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

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


#if 1
/* FIXME: Unknown definitions */
struct _SET_PARTITION_INFORMATION_EX;
typedef ULONG WAIT_TYPE;
#define WaitAll 0
#define WaitAny 1
typedef HANDLE TRACEHANDLE;
typedef PVOID PWMILIB_CONTEXT;
typedef ULONG LOGICAL;
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

//
// Forwarder
//
struct _COMPRESSED_DATA_INFO;

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

#define THREAD_ALERT (0x0004)

/* Exported object types */
extern POBJECT_TYPE NTSYSAPI ExDesktopObjectType;
extern POBJECT_TYPE NTSYSAPI ExEventObjectType;
extern POBJECT_TYPE NTSYSAPI ExSemaphoreObjectType;
extern POBJECT_TYPE NTSYSAPI ExWindowStationObjectType;
extern ULONG NTSYSAPI IoDeviceHandlerObjectSize;
extern POBJECT_TYPE NTSYSAPI IoDeviceHandlerObjectType;
extern POBJECT_TYPE NTSYSAPI IoDeviceObjectType;
extern POBJECT_TYPE NTSYSAPI IoDriverObjectType;
extern POBJECT_TYPE NTSYSAPI IoFileObjectType;
extern POBJECT_TYPE NTSYSAPI PsThreadType;
extern POBJECT_TYPE NTSYSAPI LpcPortObjectType;
extern POBJECT_TYPE NTSYSAPI SeTokenObjectType;
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

extern volatile KSYSTEM_TIME KeTickCount;

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


typedef enum _TIMER_TYPE {
  NotificationTimer,
  SynchronizationTimer
} TIMER_TYPE;

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

/* KEY_VALUE_Xxx.Type */

#define REG_NONE                           0
#define REG_SZ                             1
#define REG_EXPAND_SZ                      2
#define REG_BINARY                         3
#define REG_DWORD                          4
#define REG_DWORD_LITTLE_ENDIAN            4
#define REG_DWORD_BIG_ENDIAN               5
#define REG_LINK                           6
#define REG_MULTI_SZ                       7
#define REG_RESOURCE_LIST                  8
#define REG_FULL_RESOURCE_DESCRIPTOR       9
#define REG_RESOURCE_REQUIREMENTS_LIST    10
#define REG_QWORD                         11
#define REG_QWORD_LITTLE_ENDIAN           11

#define PCI_TYPE0_ADDRESSES               6
#define PCI_TYPE1_ADDRESSES               2
#define PCI_TYPE2_ADDRESSES               5

typedef struct _PCI_COMMON_CONFIG {
  USHORT  VendorID;
  USHORT  DeviceID;
  USHORT  Command;
  USHORT  Status;
  UCHAR  RevisionID;
  UCHAR  ProgIf;
  UCHAR  SubClass;
  UCHAR  BaseClass;
  UCHAR  CacheLineSize;
  UCHAR  LatencyTimer;
  UCHAR  HeaderType;
  UCHAR  BIST;
  union {
    struct _PCI_HEADER_TYPE_0 {
      ULONG  BaseAddresses[PCI_TYPE0_ADDRESSES];
      ULONG  CIS;
      USHORT  SubVendorID;
      USHORT  SubSystemID;
      ULONG  ROMBaseAddress;
      UCHAR  CapabilitiesPtr;
      UCHAR  Reserved1[3];
      ULONG  Reserved2;
      UCHAR  InterruptLine;
      UCHAR  InterruptPin;
      UCHAR  MinimumGrant;
      UCHAR  MaximumLatency;
    } type0;
    struct _PCI_HEADER_TYPE_1 {
      ULONG  BaseAddresses[PCI_TYPE1_ADDRESSES];
      UCHAR  PrimaryBus;
      UCHAR  SecondaryBus;
      UCHAR  SubordinateBus;
      UCHAR  SecondaryLatency;
      UCHAR  IOBase;
      UCHAR  IOLimit;
      USHORT  SecondaryStatus;
      USHORT  MemoryBase;
      USHORT  MemoryLimit;
      USHORT  PrefetchBase;
      USHORT  PrefetchLimit;
      ULONG  PrefetchBaseUpper32;
      ULONG  PrefetchLimitUpper32;
      USHORT  IOBaseUpper16;
      USHORT  IOLimitUpper16;
      UCHAR  CapabilitiesPtr;
      UCHAR  Reserved1[3];
      ULONG  ROMBaseAddress;
      UCHAR  InterruptLine;
      UCHAR  InterruptPin;
      USHORT  BridgeControl;
    } type1;
    struct _PCI_HEADER_TYPE_2 {
      ULONG  SocketRegistersBaseAddress;
      UCHAR  CapabilitiesPtr;
      UCHAR  Reserved;
      USHORT  SecondaryStatus;
      UCHAR  PrimaryBus;
      UCHAR  SecondaryBus;
      UCHAR  SubordinateBus;
      UCHAR  SecondaryLatency;
      struct {
        ULONG  Base;
        ULONG  Limit;
      } Range[PCI_TYPE2_ADDRESSES - 1];
      UCHAR  InterruptLine;
      UCHAR  InterruptPin;
      USHORT  BridgeControl;
    } type2;
  } u;
  UCHAR  DeviceSpecific[192];
} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;

/* PCI_COMMON_CONFIG.Command */

#define PCI_ENABLE_IO_SPACE               0x0001
#define PCI_ENABLE_MEMORY_SPACE           0x0002
#define PCI_ENABLE_BUS_MASTER             0x0004
#define PCI_ENABLE_SPECIAL_CYCLES         0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE   0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE 0x0020
#define PCI_ENABLE_PARITY                 0x0040
#define PCI_ENABLE_WAIT_CYCLE             0x0080
#define PCI_ENABLE_SERR                   0x0100
#define PCI_ENABLE_FAST_BACK_TO_BACK      0x0200

/* PCI_COMMON_CONFIG.Status */

#define PCI_STATUS_CAPABILITIES_LIST      0x0010
#define PCI_STATUS_66MHZ_CAPABLE          0x0020
#define PCI_STATUS_UDF_SUPPORTED          0x0040
#define PCI_STATUS_FAST_BACK_TO_BACK      0x0080
#define PCI_STATUS_DATA_PARITY_DETECTED   0x0100
#define PCI_STATUS_DEVSEL                 0x0600
#define PCI_STATUS_SIGNALED_TARGET_ABORT  0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT  0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT  0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR  0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR  0x8000

/* PCI_COMMON_CONFIG.HeaderType */

#define PCI_MULTIFUNCTION                 0x80
#define PCI_DEVICE_TYPE                   0x00
#define PCI_BRIDGE_TYPE                   0x01
#define PCI_CARDBUS_BRIDGE_TYPE           0x02

#define PCI_CONFIGURATION_TYPE(PciData) \
  (((PPCI_COMMON_CONFIG) (PciData))->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MULTIFUNCTION_DEVICE(PciData) \
  ((((PPCI_COMMON_CONFIG) (PciData))->HeaderType & PCI_MULTIFUNCTION) != 0)

/* PCI device classes */

#define PCI_CLASS_PRE_20                    0x00
#define PCI_CLASS_MASS_STORAGE_CTLR         0x01
#define PCI_CLASS_NETWORK_CTLR              0x02
#define PCI_CLASS_DISPLAY_CTLR              0x03
#define PCI_CLASS_MULTIMEDIA_DEV            0x04
#define PCI_CLASS_MEMORY_CTLR               0x05
#define PCI_CLASS_BRIDGE_DEV                0x06
#define PCI_CLASS_SIMPLE_COMMS_CTLR         0x07
#define PCI_CLASS_BASE_SYSTEM_DEV           0x08
#define PCI_CLASS_INPUT_DEV                 0x09
#define PCI_CLASS_DOCKING_STATION           0x0a
#define PCI_CLASS_PROCESSOR                 0x0b
#define PCI_CLASS_SERIAL_BUS_CTLR           0x0c

/* PCI device subclasses for class 0 */

#define PCI_SUBCLASS_PRE_20_NON_VGA         0x00
#define PCI_SUBCLASS_PRE_20_VGA             0x01

/* PCI device subclasses for class 1 (mass storage controllers)*/

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_OTHER              0x80

/* PCI device subclasses for class 2 (network controllers)*/

#define PCI_SUBCLASS_NET_ETHERNET_CTLR      0x00
#define PCI_SUBCLASS_NET_TOKEN_RING_CTLR    0x01
#define PCI_SUBCLASS_NET_FDDI_CTLR          0x02
#define PCI_SUBCLASS_NET_ATM_CTLR           0x03
#define PCI_SUBCLASS_NET_OTHER              0x80

/* PCI device subclasses for class 3 (display controllers)*/

#define PCI_SUBCLASS_VID_VGA_CTLR           0x00
#define PCI_SUBCLASS_VID_XGA_CTLR           0x01
#define PCI_SUBCLASS_VID_3D_CTLR            0x02
#define PCI_SUBCLASS_VID_OTHER              0x80

/* PCI device subclasses for class 4 (multimedia device)*/

#define PCI_SUBCLASS_MM_VIDEO_DEV           0x00
#define PCI_SUBCLASS_MM_AUDIO_DEV           0x01
#define PCI_SUBCLASS_MM_TELEPHONY_DEV       0x02
#define PCI_SUBCLASS_MM_OTHER               0x80

/* PCI device subclasses for class 5 (memory controller)*/

#define PCI_SUBCLASS_MEM_RAM                0x00
#define PCI_SUBCLASS_MEM_FLASH              0x01
#define PCI_SUBCLASS_MEM_OTHER              0x80

/* PCI device subclasses for class 6 (bridge device)*/

#define PCI_SUBCLASS_BR_HOST                0x00
#define PCI_SUBCLASS_BR_ISA                 0x01
#define PCI_SUBCLASS_BR_EISA                0x02
#define PCI_SUBCLASS_BR_MCA                 0x03
#define PCI_SUBCLASS_BR_PCI_TO_PCI          0x04
#define PCI_SUBCLASS_BR_PCMCIA              0x05
#define PCI_SUBCLASS_BR_NUBUS               0x06
#define PCI_SUBCLASS_BR_CARDBUS             0x07
#define PCI_SUBCLASS_BR_OTHER               0x80

/* PCI device subclasses for class C (serial bus controller)*/

#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04
#define PCI_SUBCLASS_SB_SMBUS               0x05

#define PCI_MAX_DEVICES        32
#define PCI_MAX_FUNCTION       8
#define PCI_MAX_BRIDGE_NUMBER  0xFF
#define PCI_INVALID_VENDORID   0xFFFF
#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_ADDRESS_MEMORY_SPACE            0x00000000
#define PCI_ADDRESS_IO_SPACE                0x00000001
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008
#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT 0
#define PCI_TYPE_20BIT 2
#define PCI_TYPE_64BIT 4

typedef struct _PCI_SLOT_NUMBER {
  union {
    struct {
      ULONG  DeviceNumber : 5;
      ULONG  FunctionNumber : 3;
      ULONG  Reserved : 24;
    } bits;
    ULONG  AsULONG;
  } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

#define POOL_COLD_ALLOCATION                256
#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE    8
#define POOL_RAISE_IF_ALLOCATION_FAILURE    16

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

typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE;

typedef enum _KWAIT_REASON {
  Executive,
  FreePage,
  PageIn,
  PoolAllocation,
  DelayExecution,
  Suspended,
  UserRequest,
  WrExecutive,
  WrFreePage,
  WrPageIn,
  WrPoolAllocation,
  WrDelayExecution,
  WrSuspended,
  WrUserRequest,
  WrEventPair,
  WrQueue,
  WrLpcReceive,
  WrLpcReply,
  WrVirtualMemory,
  WrPageOut,
  WrRendezvous,
  Spare2,
  WrGuardedMutex,
  Spare4,
  Spare5,
  Spare6,
  WrKernel,
  WrResource,
  WrPushLock,
  WrMutex,
  WrQuantumEnd,
  WrDispatchInt,
  WrPreempted,
  WrYieldExecution,
  MaximumWaitReason
} KWAIT_REASON;

typedef struct _KWAIT_BLOCK {
  LIST_ENTRY  WaitListEntry;
  struct _KTHREAD * RESTRICTED_POINTER  Thread;
  PVOID  Object;
  struct _KWAIT_BLOCK * RESTRICTED_POINTER  NextWaitBlock;
  USHORT  WaitKey;
  UCHAR WaitType;
  UCHAR SpareByte;
} KWAIT_BLOCK, *PKWAIT_BLOCK, *RESTRICTED_POINTER PRKWAIT_BLOCK;

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK * PIO_REMOVE_LOCK_TRACKING_BLOCK;

typedef struct _IO_REMOVE_LOCK_COMMON_BLOCK {
  BOOLEAN  Removed;
  BOOLEAN  Reserved[3];
  volatile LONG  IoCount;
  KEVENT  RemoveEvent;
} IO_REMOVE_LOCK_COMMON_BLOCK;

typedef struct _IO_REMOVE_LOCK_DBG_BLOCK {
  LONG  Signature;
  LONG  HighWatermark;
  LONGLONG  MaxLockedTicks;
  LONG  AllocateTag;
  LIST_ENTRY  LockList;
  KSPIN_LOCK  Spin;
  volatile LONG  LowMemoryCount;
  ULONG  Reserved1[4];
  PVOID  Reserved2;
  PIO_REMOVE_LOCK_TRACKING_BLOCK  Blocks;
} IO_REMOVE_LOCK_DBG_BLOCK;

typedef struct _IO_REMOVE_LOCK {
  IO_REMOVE_LOCK_COMMON_BLOCK  Common;
#if DBG
  IO_REMOVE_LOCK_DBG_BLOCK  Dbg;
#endif
} IO_REMOVE_LOCK, *PIO_REMOVE_LOCK;

typedef struct _IO_WORKITEM *PIO_WORKITEM;

typedef VOID
(DDKAPI IO_WORKITEM_ROUTINE)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PVOID  Context);
typedef IO_WORKITEM_ROUTINE *PIO_WORKITEM_ROUTINE;

typedef struct _SHARE_ACCESS {
  ULONG  OpenCount;
  ULONG  Readers;
  ULONG  Writers;
  ULONG  Deleters;
  ULONG  SharedRead;
  ULONG  SharedWrite;
  ULONG  SharedDelete;
} SHARE_ACCESS, *PSHARE_ACCESS;

typedef enum _KINTERRUPT_MODE {
  LevelSensitive,
  Latched
} KINTERRUPT_MODE;

#define THREAD_WAIT_OBJECTS 3

typedef VOID
(DDKAPI *PKINTERRUPT_ROUTINE)(
  VOID);

typedef enum _CREATE_FILE_TYPE {
  CreateFileTypeNone,
  CreateFileTypeNamedPipe,
  CreateFileTypeMailslot
} CREATE_FILE_TYPE;

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

#define IO_FORCE_ACCESS_CHECK               0x001
#define IO_NO_PARAMETER_CHECKING            0x100

#define IO_REPARSE                      0x0
#define IO_REMOUNT                      0x1

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

typedef enum _KBUGCHECK_CALLBACK_REASON {
  KbCallbackInvalid,
  KbCallbackReserved1,
  KbCallbackSecondaryDumpData,
  KbCallbackDumpIo,
} KBUGCHECK_CALLBACK_REASON;

struct _KBUGCHECK_REASON_CALLBACK_RECORD;

typedef VOID
(DDKAPI *PKBUGCHECK_REASON_CALLBACK_ROUTINE)(
  IN KBUGCHECK_CALLBACK_REASON  Reason,
  IN struct _KBUGCHECK_REASON_CALLBACK_RECORD  *Record,
  IN OUT PVOID  ReasonSpecificData,
  IN ULONG  ReasonSpecificDataLength);

typedef struct _KBUGCHECK_REASON_CALLBACK_RECORD {
  LIST_ENTRY  Entry;
  PKBUGCHECK_REASON_CALLBACK_ROUTINE  CallbackRoutine;
  PUCHAR  Component;
  ULONG_PTR  Checksum;
  KBUGCHECK_CALLBACK_REASON  Reason;
  UCHAR  State;
} KBUGCHECK_REASON_CALLBACK_RECORD, *PKBUGCHECK_REASON_CALLBACK_RECORD;

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
  BufferEmpty,
  BufferInserted,
  BufferStarted,
  BufferFinished,
  BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;

typedef VOID
(DDKAPI *PKBUGCHECK_CALLBACK_ROUTINE)(
  IN PVOID  Buffer,
  IN ULONG  Length);

typedef struct _KBUGCHECK_CALLBACK_RECORD {
  LIST_ENTRY  Entry;
  PKBUGCHECK_CALLBACK_ROUTINE  CallbackRoutine;
  PVOID  Buffer;
  ULONG  Length;
  PUCHAR  Component;
  ULONG_PTR  Checksum;
  UCHAR  State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

typedef BOOLEAN
(DDKAPI *PNMI_CALLBACK)(
    IN PVOID Context,
    IN BOOLEAN Handled);

/*
 * VOID
 * KeInitializeCallbackRecord(
 *   IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord)
 */
#define KeInitializeCallbackRecord(CallbackRecord) \
  CallbackRecord->State = BufferEmpty;

typedef enum _KDPC_IMPORTANCE {
  LowImportance,
  MediumImportance,
  HighImportance
} KDPC_IMPORTANCE;

typedef enum _MEMORY_CACHING_TYPE_ORIG {
  MmFrameBufferCached = 2
} MEMORY_CACHING_TYPE_ORIG;

typedef enum _MEMORY_CACHING_TYPE {
  MmNonCached = FALSE,
  MmCached = TRUE,
  MmWriteCombined = MmFrameBufferCached,
  MmHardwareCoherentCached,
  MmNonCachedUnordered,
  MmUSWCCached,
  MmMaximumCacheType
} MEMORY_CACHING_TYPE;

typedef enum _MM_PAGE_PRIORITY {
  LowPagePriority,
  NormalPagePriority = 16,
  HighPagePriority = 32
} MM_PAGE_PRIORITY;

typedef enum _LOCK_OPERATION {
  IoReadAccess,
  IoWriteAccess,
  IoModifyAccess
} LOCK_OPERATION;

#define FLUSH_MULTIPLE_MAXIMUM 32

typedef enum _MM_SYSTEM_SIZE {
  MmSmallSystem,
  MmMediumSystem,
  MmLargeSystem
} MM_SYSTEMSIZE;

typedef struct _OBJECT_HANDLE_INFORMATION {
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

typedef struct _CLIENT_ID {
  HANDLE  UniqueProcess;
  HANDLE  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef VOID
(DDKAPI *PKSTART_ROUTINE)(
  IN PVOID  StartContext);

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

typedef VOID
(DDKAPI *PREQUEST_POWER_COMPLETE)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN UCHAR  MinorFunction,
  IN POWER_STATE  PowerState,
  IN PVOID  Context,
  IN PIO_STATUS_BLOCK  IoStatus);

typedef enum _TRACE_INFORMATION_CLASS {
  TraceIdClass,
  TraceHandleClass,
  TraceEnableFlagsClass,
  TraceEnableLevelClass,
  GlobalLoggerHandleClass,
  EventLoggerHandleClass,
  AllLoggerHandlesClass,
  TraceHandleByNameClass
} TRACE_INFORMATION_CLASS;

typedef enum _REG_NOTIFY_CLASS
{
  RegNtDeleteKey,
  RegNtPreDeleteKey = RegNtDeleteKey,
  RegNtSetValueKey,
  RegNtPreSetValueKey = RegNtSetValueKey,
  RegNtDeleteValueKey,
  RegNtPreDeleteValueKey = RegNtDeleteValueKey,
  RegNtSetInformationKey,
  RegNtPreSetInformationKey = RegNtSetInformationKey,
  RegNtRenameKey,
  RegNtPreRenameKey = RegNtRenameKey,
  RegNtEnumerateKey,
  RegNtPreEnumerateKey = RegNtEnumerateKey,
  RegNtEnumerateValueKey,
  RegNtPreEnumerateValueKey = RegNtEnumerateValueKey,
  RegNtQueryKey,
  RegNtPreQueryKey = RegNtQueryKey,
  RegNtQueryValueKey,
  RegNtPreQueryValueKey = RegNtQueryValueKey,
  RegNtQueryMultipleValueKey,
  RegNtPreQueryMultipleValueKey = RegNtQueryMultipleValueKey,
  RegNtPreCreateKey,
  RegNtPostCreateKey,
  RegNtPreOpenKey,
  RegNtPostOpenKey,
  RegNtKeyHandleClose,
  RegNtPreKeyHandleClose = RegNtKeyHandleClose,
  RegNtPostDeleteKey,
  RegNtPostSetValueKey,
  RegNtPostDeleteValueKey,
  RegNtPostSetInformationKey,
  RegNtPostRenameKey,
  RegNtPostEnumerateKey,
  RegNtPostEnumerateValueKey,
  RegNtPostQueryKey,
  RegNtPostQueryValueKey,
  RegNtPostQueryMultipleValueKey,
  RegNtPostKeyHandleClose,
  RegNtPreCreateKeyEx,
  RegNtPostCreateKeyEx,
  RegNtPreOpenKeyEx,
  RegNtPostOpenKeyEx
} REG_NOTIFY_CLASS, *PREG_NOTIFY_CLASS;

typedef NTSTATUS
(NTAPI *PEX_CALLBACK_FUNCTION)(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
);

typedef struct _REG_DELETE_KEY_INFORMATION
{
    PVOID Object;
} REG_DELETE_KEY_INFORMATION, *PREG_DELETE_KEY_INFORMATION;

typedef struct _REG_SET_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PUNICODE_STRING ValueName;
    ULONG TitleIndex;
    ULONG Type;
    PVOID Data;
    ULONG DataSize;
} REG_SET_VALUE_KEY_INFORMATION, *PREG_SET_VALUE_KEY_INFORMATION;

typedef struct _REG_DELETE_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PUNICODE_STRING ValueName;
} REG_DELETE_VALUE_KEY_INFORMATION, *PREG_DELETE_VALUE_KEY_INFORMATION;

typedef struct _REG_SET_INFORMATION_KEY_INFORMATION
{
    PVOID Object;
    KEY_SET_INFORMATION_CLASS KeySetInformationClass;
    PVOID KeySetInformation;
    ULONG KeySetInformationLength;
} REG_SET_INFORMATION_KEY_INFORMATION, *PREG_SET_INFORMATION_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_KEY_INFORMATION
{
    PVOID Object;
    ULONG Index;
    KEY_INFORMATION_CLASS KeyInformationClass;
    PVOID KeyInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_ENUMERATE_KEY_INFORMATION, *PREG_ENUMERATE_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_VALUE_KEY_INFORMATION
{
    PVOID Object;
    ULONG Index;
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
    PVOID KeyValueInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_ENUMERATE_VALUE_KEY_INFORMATION, *PREG_ENUMERATE_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_KEY_INFORMATION
{
    PVOID Object;
    KEY_INFORMATION_CLASS KeyInformationClass;
    PVOID KeyInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_QUERY_KEY_INFORMATION, *PREG_QUERY_KEY_INFORMATION;

typedef struct _REG_QUERY_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PUNICODE_STRING ValueName;
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
    PVOID KeyValueInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_QUERY_VALUE_KEY_INFORMATION, *PREG_QUERY_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PKEY_VALUE_ENTRY ValueEntries;
    ULONG EntryCount;
    PVOID ValueBuffer;
    PULONG BufferLength;
    PULONG RequiredBufferLength;
} REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION, *PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION;

typedef struct _REG_PRE_CREATE_KEY_INFORMATION
{
    PUNICODE_STRING CompleteName;
} REG_PRE_CREATE_KEY_INFORMATION, *PREG_PRE_CREATE_KEY_INFORMATION;

typedef struct _REG_POST_CREATE_KEY_INFORMATION
{
    PUNICODE_STRING CompleteName;
    PVOID Object;
    NTSTATUS Status;
} REG_POST_CREATE_KEY_INFORMATION, *PREG_POST_CREATE_KEY_INFORMATION;

typedef struct _REG_PRE_OPEN_KEY_INFORMATION
{
    PUNICODE_STRING  CompleteName;
} REG_PRE_OPEN_KEY_INFORMATION, *PREG_PRE_OPEN_KEY_INFORMATION;

typedef struct _REG_POST_OPEN_KEY_INFORMATION
{
    PUNICODE_STRING CompleteName;
    PVOID Object;
    NTSTATUS Status;
} REG_POST_OPEN_KEY_INFORMATION, *PREG_POST_OPEN_KEY_INFORMATION;

typedef struct _REG_POST_OPERATION_INFORMATION
{
    PVOID Object;
    NTSTATUS Status;
} REG_POST_OPERATION_INFORMATION,*PREG_POST_OPERATION_INFORMATION;

typedef struct _REG_KEY_HANDLE_CLOSE_INFORMATION
{
    PVOID Object;
} REG_KEY_HANDLE_CLOSE_INFORMATION, *PREG_KEY_HANDLE_CLOSE_INFORMATION;

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

#define DBG_STATUS_CONTROL_C              1
#define DBG_STATUS_SYSRQ                  2
#define DBG_STATUS_BUGCHECK_FIRST         3
#define DBG_STATUS_BUGCHECK_SECOND        4
#define DBG_STATUS_FATAL                  5
#define DBG_STATUS_DEBUG_CONTROL          6
#define DBG_STATUS_WORKER                 7

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

#define HASH_STRING_ALGORITHM_DEFAULT     0
#define HASH_STRING_ALGORITHM_X65599      1
#define HASH_STRING_ALGORITHM_INVALID     0xffffffff

typedef VOID
(DDKAPI *PTIMER_APC_ROUTINE)(
  IN PVOID  TimerContext,
  IN ULONG  TimerLowValue,
  IN LONG  TimerHighValue);



/*
** WMI structures
*/

typedef VOID
(DDKAPI *WMI_NOTIFICATION_CALLBACK)(
  PVOID  Wnode,
  PVOID  Context);


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
#define MAXIMUM_SUPPORTED_EXTENSION  512

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

typedef struct _KFLOATING_SAVE {
  ULONG  ControlWord;
  ULONG  StatusWord;
  ULONG  ErrorOffset;
  ULONG  ErrorSelector;
  ULONG  DataOffset;
  ULONG  DataSelector;
  ULONG  Cr0NpxState;
  ULONG  Spare1;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readfsbyte(FIELD_OFFSET(KPCR, Number));
}

NTHALAPI
KIRQL
DDKAPI
KeGetCurrentIrql(
    VOID);

NTKERNELAPI
PRKTHREAD
NTAPI
KeGetCurrentThread(
    VOID);

#define KI_USER_SHARED_DATA               0xffdf0000

#define PAGE_SIZE                         0x1000
#define PAGE_SHIFT                        12L

#define SharedUserData                    ((KUSER_SHARED_DATA * CONST) KI_USER_SHARED_DATA)

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

typedef struct DECLSPEC_ALIGN(16) _M128A {
    ULONGLONG Low;
    LONGLONG High;
} M128A, *PM128A;

typedef struct _XMM_SAVE_AREA32 {
    USHORT ControlWord;
    USHORT StatusWord;
    UCHAR TagWord;
    UCHAR Reserved1;
    USHORT ErrorOpcode;
    ULONG ErrorOffset;
    USHORT ErrorSelector;
    USHORT Reserved2;
    ULONG DataOffset;
    USHORT DataSelector;
    USHORT Reserved3;
    ULONG MxCsr;
    ULONG MxCsr_Mask;
    M128A FloatRegisters[8];
    M128A XmmRegisters[16];
    UCHAR Reserved4[96];
} XMM_SAVE_AREA32, *PXMM_SAVE_AREA32;

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

//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG64 PFN_NUMBER, *PPFN_NUMBER;
typedef LONG64 SPFN_NUMBER, *PSPFN_NUMBER;

#define PASSIVE_LEVEL                      0
#define LOW_LEVEL                          0
#define APC_LEVEL                          1
#define DISPATCH_LEVEL                     2
#define CLOCK_LEVEL                       13
#define IPI_LEVEL                         14
#define POWER_LEVEL                       14
#define PROFILE_LEVEL                     15
#define HIGH_LEVEL                        15

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

NTKERNELAPI
PRKTHREAD
NTAPI
KeGetCurrentThread(
    VOID);

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

#define MM_DONT_ZERO_ALLOCATION             0x00000001
#define MM_ALLOCATE_FROM_LOCAL_NODE_ONLY    0x00000002

    
#define EFLAG_SIGN                        0x8000
#define EFLAG_ZERO                        0x4000
#define EFLAG_SELECT                      (EFLAG_SIGN | EFLAG_ZERO)

#define RESULT_NEGATIVE                   ((EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_ZERO                       ((~EFLAG_SIGN & EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_POSITIVE                   ((~EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)

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

NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(
    IN OUT PKSPIN_LOCK SpinLock
);

NTKERNELAPI
BOOLEAN
FASTCALL
KeTestSpinLock(
    IN PKSPIN_LOCK SpinLock
);

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

/*
** Utillity functions
*/

#define ARGUMENT_PRESENT(ArgumentPointer) \
  ((CHAR*)((ULONG_PTR)(ArgumentPointer)) != (CHAR*)NULL)

/*
 * ULONG
 * BYTE_OFFSET(
 *   IN PVOID  Va)
 */
#define BYTE_OFFSET(Va) \
  ((ULONG) ((ULONG_PTR) (Va) & (PAGE_SIZE - 1)))

/*
 * ULONG
 * BYTES_TO_PAGES(
 *   IN ULONG  Size)
 */
#define BYTES_TO_PAGES(Size) \
  ((ULONG) ((ULONG_PTR) (Size) >> PAGE_SHIFT) + (((ULONG) (Size) & (PAGE_SIZE - 1)) != 0))

/*
 * PVOID
 * PAGE_ALIGN(
 *   IN PVOID  Va)
 */
#define PAGE_ALIGN(Va) \
  ((PVOID) ((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

/*
 * ULONG_PTR
 * ROUND_TO_PAGES(
 *   IN ULONG_PTR  Size)
 */
#define ROUND_TO_PAGES(Size) \
  ((ULONG_PTR) (((ULONG_PTR) Size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)))



#if defined(_X86_) || defined(_AMD64_)

//
// x86 and x64 performs a 0x2C interrupt
//
#define DbgRaiseAssertionFailure __int2c

#elif defined(_ARM_)

//
// TODO
//

#else
#error Unsupported Architecture
#endif

#if DBG

#define ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

#define ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, msg ), FALSE : TRUE)

#define RTL_SOFT_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #exp), FALSE : TRUE)

#define RTL_SOFT_ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #exp, (msg)), FALSE : TRUE)

#define RTL_VERIFY(exp) ASSERT(exp)
#define RTL_VERIFYMSG(msg, exp) ASSERT(msg, exp)

#define RTL_SOFT_VERIFY(exp) RTL_SOFT_ASSERT(exp)
#define RTL_SOFT_VERIFYMSG(msg, exp) RTL_SOFT_ASSERTMSG(msg, exp)

#if defined(_MSC_VER)

#define NT_ASSERT(exp) \
   ((!(exp)) ? \
      (__annotation(L"Debug", L"AssertFail", L#exp), \
       DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSG(msg, exp) \
   ((!(exp)) ? \
      (__annotation(L"Debug", L"AssertFail", L##msg), \
      DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSGW(msg, exp) \
    ((!(exp)) ? \
        (__annotation(L"Debug", L"AssertFail", msg), \
         DbgRaiseAssertionFailure(), FALSE) : TRUE)

#else

//
// GCC doesn't support __annotation (nor PDB)
//
#define NT_ASSERT(exp) \
   (VOID)((!(exp)) ? (DbgRaiseAssertionFailure(), FALSE) : TRUE)

#define NT_ASSERTMSG NT_ASSERT
#define NT_ASSERTMSGW NT_ASSERT

#endif

#else /* !DBG */

#define ASSERT(exp) ((VOID) 0)
#define ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_SOFT_ASSERT(exp) ((VOID) 0)
#define RTL_SOFT_ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define NT_ASSERT(exp)     ((VOID)0)
#define NT_ASSERTMSG(exp)  ((VOID)0)
#define NT_ASSERTMSGW(exp) ((VOID)0)

#endif /* DBG */

/* HACK HACK HACK - GCC (or perhaps LD) is messing this up */
#if defined(_NTSYSTEM_) || defined(__GNUC__)
#define NLS_MB_CODE_PAGE_TAG NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag
#else
#define NLS_MB_CODE_PAGE_TAG (*NlsMbCodePageTag)
#define NLS_MB_OEM_CODE_PAGE_TAG (*NlsMbOemCodePageTag)
#endif /* _NT_SYSTEM */

extern BOOLEAN NTSYSAPI NLS_MB_CODE_PAGE_TAG;
extern BOOLEAN NTSYSAPI NLS_MB_OEM_CODE_PAGE_TAG;

/*
** Driver support routines
*/

/** Runtime library routines **/

static __inline VOID
InitializeListHead(
  IN PLIST_ENTRY  ListHead)
{
  ListHead->Flink = ListHead->Blink = ListHead;
}

static __inline VOID
InsertHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  Entry)
{
  PLIST_ENTRY OldFlink;
  OldFlink = ListHead->Flink;
  Entry->Flink = OldFlink;
  Entry->Blink = ListHead;
  OldFlink->Blink = Entry;
  ListHead->Flink = Entry;
}

static __inline VOID
InsertTailList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  Entry)
{
  PLIST_ENTRY OldBlink;
  OldBlink = ListHead->Blink;
  Entry->Flink = ListHead;
  Entry->Blink = OldBlink;
  OldBlink->Flink = Entry;
  ListHead->Blink = Entry;
}

/*
 * BOOLEAN
 * IsListEmpty(
 *   IN PLIST_ENTRY  ListHead)
 */
#define IsListEmpty(_ListHead) \
  ((_ListHead)->Flink == (_ListHead))

/*
 * PSINGLE_LIST_ENTRY
 * PopEntryList(
 *   IN PSINGLE_LIST_ENTRY  ListHead)
 */
#define PopEntryList(ListHead) \
  (ListHead)->Next; \
  { \
    PSINGLE_LIST_ENTRY _FirstEntry; \
    _FirstEntry = (ListHead)->Next; \
    if (_FirstEntry != NULL) \
      (ListHead)->Next = _FirstEntry->Next; \
  }

/*
 * VOID
 * PushEntryList(
 *   IN PSINGLE_LIST_ENTRY  ListHead,
 *   IN PSINGLE_LIST_ENTRY  Entry)
 */
#define PushEntryList(_ListHead, _Entry) \
	(_Entry)->Next = (_ListHead)->Next; \
	(_ListHead)->Next = (_Entry); \

static __inline BOOLEAN
RemoveEntryList(
  IN PLIST_ENTRY  Entry)
{
  PLIST_ENTRY OldFlink;
  PLIST_ENTRY OldBlink;

  OldFlink = Entry->Flink;
  OldBlink = Entry->Blink;
  OldFlink->Blink = OldBlink;
  OldBlink->Flink = OldFlink;
  return (BOOLEAN)(OldFlink == OldBlink);
}

static __inline PLIST_ENTRY
RemoveHeadList(
  IN PLIST_ENTRY  ListHead)
{
  PLIST_ENTRY Flink;
  PLIST_ENTRY Entry;

  Entry = ListHead->Flink;
  Flink = Entry->Flink;
  ListHead->Flink = Flink;
  Flink->Blink = ListHead;
  return Entry;
}

static __inline PLIST_ENTRY
RemoveTailList(
  IN PLIST_ENTRY  ListHead)
{
  PLIST_ENTRY Blink;
  PLIST_ENTRY Entry;

  Entry = ListHead->Blink;
  Blink = Entry->Blink;
  ListHead->Blink = Blink;
  Blink->Flink = ListHead;
  return Entry;
}

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

NTKERNELAPI
BOOLEAN
NTAPI
KeAreAllApcsDisabled(
    VOID
);

/* Guarded Mutex routines */

NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutex(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

NTKERNELAPI
VOID
NTAPI
KeEnterGuardedRegion(
    VOID
);

NTKERNELAPI
VOID
NTAPI
KeLeaveGuardedRegion(
    VOID
);

NTKERNELAPI
VOID
FASTCALL
KeInitializeGuardedMutex(
    OUT PKGUARDED_MUTEX GuardedMutex
);

NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutex(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireGuardedMutex(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

/* Fast Mutex */
#define ExInitializeFastMutex(_FastMutex) \
{ \
    (_FastMutex)->Count = FM_LOCK_BIT; \
    (_FastMutex)->Owner = NULL; \
    (_FastMutex)->Contention = 0; \
    KeInitializeEvent(&(_FastMutex)->Gate, SynchronizationEvent, FALSE); \
}

NTKERNELAPI
VOID
NTAPI
KeInitializeEvent(
  IN PRKEVENT  Event,
  IN EVENT_TYPE  Type,
  IN BOOLEAN  State);

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
 * VOID
 * InitializeSListHead(
 *   IN PSLIST_HEADER  SListHead)
 */
#define InitializeSListHead(_SListHead) \
	(_SListHead)->Alignment = 0

#define ExInitializeSListHead InitializeSListHead

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


#if DBG

#define PAGED_CODE() { \
  if (KeGetCurrentIrql() > APC_LEVEL) { \
    KdPrint( ("NTDDK: Pageable code called at IRQL > APC_LEVEL (%d)\n", KeGetCurrentIrql() )); \
    ASSERT(FALSE); \
  } \
}

#else

#define PAGED_CODE()

#endif

NTKERNELAPI
VOID
NTAPI
ProbeForRead(
  IN CONST VOID  *Address,
  IN SIZE_T  Length,
  IN ULONG  Alignment);

NTKERNELAPI
VOID
NTAPI
ProbeForWrite(
  IN PVOID  Address,
  IN SIZE_T  Length,
  IN ULONG  Alignment);



/** Configuration manager routines **/

NTKERNELAPI
NTSTATUS
NTAPI
CmRegisterCallback(
  IN PEX_CALLBACK_FUNCTION  Function,
  IN PVOID  Context,
  IN OUT PLARGE_INTEGER  Cookie);

NTKERNELAPI
NTSTATUS
NTAPI
CmUnRegisterCallback(
  IN LARGE_INTEGER  Cookie);



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


/** Io access routines **/

#if !defined(_M_AMD64)
NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTHALAPI
UCHAR
NTAPI
READ_PORT_UCHAR(
  IN PUCHAR  Port);

NTHALAPI
ULONG
NTAPI
READ_PORT_ULONG(
  IN PULONG  Port);

NTHALAPI
USHORT
NTAPI
READ_PORT_USHORT(
  IN PUSHORT  Port);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTKERNELAPI
UCHAR
NTAPI
READ_REGISTER_UCHAR(
  IN PUCHAR  Register);

NTKERNELAPI
ULONG
NTAPI
READ_REGISTER_ULONG(
  IN PULONG  Register);

NTKERNELAPI
USHORT
NTAPI
READ_REGISTER_USHORT(
  IN PUSHORT  Register);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTHALAPI
VOID
NTAPI
WRITE_PORT_UCHAR(
  IN PUCHAR  Port,
  IN UCHAR  Value);

NTHALAPI
VOID
NTAPI
WRITE_PORT_ULONG(
  IN PULONG  Port,
  IN ULONG  Value);

NTHALAPI
VOID
NTAPI
WRITE_PORT_USHORT(
  IN PUSHORT  Port,
  IN USHORT  Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_UCHAR(
  IN PUCHAR  Register,
  IN UCHAR  Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_ULONG(
  IN PULONG  Register,
  IN ULONG  Value);

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_USHORT(
  IN PUSHORT  Register,
  IN USHORT  Value);

#else

FORCEINLINE
VOID
READ_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count)
{
    __inbytestring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
READ_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count)
{
    __indwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
READ_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count)
{
    __inwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
UCHAR
READ_PORT_UCHAR(
  IN PUCHAR  Port)
{
    return __inbyte((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
ULONG
READ_PORT_ULONG(
  IN PULONG  Port)
{
    return __indword((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
USHORT
READ_PORT_USHORT(
  IN PUSHORT  Port)
{
    return __inword((USHORT)(ULONG_PTR)Port);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count)
{
    __movsb(Register, Buffer, Count);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count)
{
    __movsd(Register, Buffer, Count);
}

FORCEINLINE
VOID
READ_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count)
{
    __movsw(Register, Buffer, Count);
}

FORCEINLINE
UCHAR
READ_REGISTER_UCHAR(
  IN PUCHAR Register)
{
    return *Register;
}

FORCEINLINE
ULONG
READ_REGISTER_ULONG(
  IN PULONG Register)
{
    return *Register;
}

FORCEINLINE
USHORT
READ_REGISTER_USHORT(
  IN PUSHORT Register)
{
    return *Register;
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count)
{
    __outbytestring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count)
{
    __outdwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count)
{
    __outwordstring((USHORT)(ULONG_PTR)Port, Buffer, Count);
}

FORCEINLINE
VOID
WRITE_PORT_UCHAR(
  IN PUCHAR  Port,
  IN UCHAR  Value)
{
    __outbyte((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_PORT_ULONG(
  IN PULONG  Port,
  IN ULONG  Value)
{
    __outdword((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_PORT_USHORT(
  IN PUSHORT  Port,
  IN USHORT  Value)
{
    __outword((USHORT)(ULONG_PTR)Port, Value);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count)
{
    LONG Synch;
    __movsb(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count)
{
    LONG Synch;
    __movsd(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count)
{
    LONG Synch;
    __movsw(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_UCHAR(
  IN PUCHAR  Register,
  IN UCHAR  Value)
{
    LONG Synch;
    *Register = Value;
    InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_ULONG(
  IN PULONG  Register,
  IN ULONG  Value)
{
    LONG Synch;
    *Register = Value;
    InterlockedOr(&Synch, 1);
}

FORCEINLINE
VOID
WRITE_REGISTER_USHORT(
  IN PUSHORT  Register,
  IN USHORT  Value)
{
	LONG Sync;
	*Register = Value;
	InterlockedOr(&Sync, 1);
}

#endif

/** I/O manager routines **/

NTKERNELAPI
VOID
NTAPI
IoAcquireCancelSpinLock(
  OUT PKIRQL  Irql);

NTKERNELAPI
NTSTATUS
NTAPI
IoAcquireRemoveLockEx(
  IN PIO_REMOVE_LOCK  RemoveLock,
  IN OPTIONAL PVOID  Tag  OPTIONAL,
  IN PCSTR  File,
  IN ULONG  Line,
  IN ULONG  RemlockSize);

/*
 * NTSTATUS
 * IoAcquireRemoveLock(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN OPTIONAL PVOID  Tag)
 */
#define IoAcquireRemoveLock(_RemoveLock, \
                            _Tag) \
  IoAcquireRemoveLockEx(_RemoveLock, _Tag, __FILE__, __LINE__, sizeof(IO_REMOVE_LOCK))

/*
 * VOID
 * IoAdjustPagingPathCount(
 *   IN PLONG  Count,
 *   IN BOOLEAN  Increment)
 */
#define IoAdjustPagingPathCount(_Count, \
                                _Increment) \
{ \
  if (_Increment) \
    { \
      InterlockedIncrement(_Count); \
    } \
  else \
    { \
      InterlockedDecrement(_Count); \
    } \
}

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

NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateDriverObjectExtension(
  IN PDRIVER_OBJECT  DriverObject,
  IN PVOID  ClientIdentificationAddress,
  IN ULONG  DriverObjectExtensionSize,
  OUT PVOID  *DriverObjectExtension);

NTKERNELAPI
PVOID
NTAPI
IoAllocateErrorLogEntry(
  IN PVOID  IoObject,
  IN UCHAR  EntrySize);

NTKERNELAPI
PIRP
NTAPI
IoAllocateIrp(
  IN CCHAR  StackSize,
  IN BOOLEAN  ChargeQuota);

NTKERNELAPI
PMDL
NTAPI
IoAllocateMdl(
  IN PVOID  VirtualAddress,
  IN ULONG  Length,
  IN BOOLEAN  SecondaryBuffer,
  IN BOOLEAN  ChargeQuota,
  IN OUT PIRP  Irp  OPTIONAL);

NTKERNELAPI
PIO_WORKITEM
NTAPI
IoAllocateWorkItem(
  IN PDEVICE_OBJECT  DeviceObject);

/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDevice(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PUNICODE_STRING  TargetDevice,
  OUT PDEVICE_OBJECT  *AttachedDevice);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoAttachDeviceToDeviceStack(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PDEVICE_OBJECT  TargetDevice);

NTKERNELAPI
PIRP
NTAPI
IoBuildAsynchronousFsdRequest(
  IN ULONG  MajorFunction,
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PVOID  Buffer  OPTIONAL,
  IN ULONG  Length  OPTIONAL,
  IN PLARGE_INTEGER  StartingOffset  OPTIONAL,
  IN PIO_STATUS_BLOCK  IoStatusBlock  OPTIONAL);

NTKERNELAPI
PIRP
NTAPI
IoBuildDeviceIoControlRequest(
  IN ULONG  IoControlCode,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PVOID  InputBuffer  OPTIONAL,
  IN ULONG  InputBufferLength,
  OUT PVOID  OutputBuffer  OPTIONAL,
  IN ULONG  OutputBufferLength,
  IN BOOLEAN  InternalDeviceIoControl,
  IN PKEVENT  Event,
  OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTKERNELAPI
VOID
NTAPI
IoBuildPartialMdl(
  IN PMDL  SourceMdl,
  IN OUT PMDL  TargetMdl,
  IN PVOID  VirtualAddress,
  IN ULONG  Length);

NTKERNELAPI
PIRP
NTAPI
IoBuildSynchronousFsdRequest(
  IN ULONG  MajorFunction,
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PVOID  Buffer  OPTIONAL,
  IN ULONG  Length  OPTIONAL,
  IN PLARGE_INTEGER  StartingOffset  OPTIONAL,
  IN PKEVENT  Event,
  OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTKERNELAPI
NTSTATUS
FASTCALL
IofCallDriver(
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PIRP  Irp);

/*
 * NTSTATUS
 * IoCallDriver(
 *   IN PDEVICE_OBJECT  DeviceObject,
 *   IN OUT PIRP  Irp)
 */
#define IoCallDriver IofCallDriver

NTKERNELAPI
VOID
NTAPI
IoCancelFileOpen(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PFILE_OBJECT  FileObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoCancelIrp(
  IN PIRP  Irp);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckShareAccess(
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  DesiredShareAccess,
  IN OUT PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess,
  IN BOOLEAN  Update);

NTKERNELAPI
VOID
FASTCALL
IofCompleteRequest(
  IN PIRP  Irp,
  IN CCHAR  PriorityBoost);

/*
 * VOID
 * IoCompleteRequest(
 *  IN PIRP  Irp,
 *  IN CCHAR  PriorityBoost)
 */
#define IoCompleteRequest IofCompleteRequest

NTKERNELAPI
NTSTATUS
NTAPI
IoConnectInterrupt(
  OUT PKINTERRUPT  *InterruptObject,
  IN PKSERVICE_ROUTINE  ServiceRoutine,
  IN PVOID  ServiceContext,
  IN PKSPIN_LOCK  SpinLock  OPTIONAL,
  IN ULONG  Vector,
  IN KIRQL  Irql,
  IN KIRQL  SynchronizeIrql,
  IN KINTERRUPT_MODE    InterruptMode,
  IN BOOLEAN  ShareVector,
  IN KAFFINITY  ProcessorEnableMask,
  IN BOOLEAN  FloatingSave);

/*
 * PIO_STACK_LOCATION
 * IoGetCurrentIrpStackLocation(
 *   IN PIRP  Irp)
 */
#define IoGetCurrentIrpStackLocation(_Irp) \
  ((_Irp)->Tail.Overlay.CurrentStackLocation)

/*
 * PIO_STACK_LOCATION
 * IoGetNextIrpStackLocation(
 *   IN PIRP  Irp)
 */
#define IoGetNextIrpStackLocation(_Irp) \
  ((_Irp)->Tail.Overlay.CurrentStackLocation - 1)

/*
 * VOID
 * IoCopyCurrentIrpStackLocationToNext(
 *   IN PIRP  Irp)
 */
#define IoCopyCurrentIrpStackLocationToNext(_Irp) \
{ \
  PIO_STACK_LOCATION _IrpSp; \
  PIO_STACK_LOCATION _NextIrpSp; \
  _IrpSp = IoGetCurrentIrpStackLocation(_Irp); \
  _NextIrpSp = IoGetNextIrpStackLocation(_Irp); \
  RtlCopyMemory(_NextIrpSp, _IrpSp, \
    FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
  _NextIrpSp->Control = 0; \
}

NTKERNELAPI
PCONTROLLER_OBJECT
NTAPI
IoCreateController(
  IN ULONG  Size);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDevice(
  IN PDRIVER_OBJECT  DriverObject,
  IN ULONG  DeviceExtensionSize,
  IN PUNICODE_STRING  DeviceName  OPTIONAL,
  IN DEVICE_TYPE  DeviceType,
  IN ULONG  DeviceCharacteristics,
  IN BOOLEAN  Exclusive,
  OUT PDEVICE_OBJECT  *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDisk(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PCREATE_DISK  Disk);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateFile(
  OUT PHANDLE FileHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PLARGE_INTEGER AllocationSize OPTIONAL,
  IN ULONG FileAttributes,
  IN ULONG ShareAccess,
  IN ULONG Disposition,
  IN ULONG CreateOptions,
  IN PVOID EaBuffer OPTIONAL,
  IN ULONG EaLength,
  IN CREATE_FILE_TYPE CreateFileType,
  IN PVOID ExtraCreateParameters OPTIONAL,
  IN ULONG Options);

NTKERNELAPI
PKEVENT
NTAPI
IoCreateNotificationEvent(
  IN PUNICODE_STRING  EventName,
  OUT PHANDLE  EventHandle);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN PUNICODE_STRING  DeviceName);

NTKERNELAPI
PKEVENT
NTAPI
IoCreateSynchronizationEvent(
  IN PUNICODE_STRING  EventName,
  OUT PHANDLE  EventHandle);

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateUnprotectedSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN PUNICODE_STRING  DeviceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoCsqInitialize(
  PIO_CSQ  Csq,
  IN PIO_CSQ_INSERT_IRP  CsqInsertIrp,
  IN PIO_CSQ_REMOVE_IRP  CsqRemoveIrp,
  IN PIO_CSQ_PEEK_NEXT_IRP  CsqPeekNextIrp,
  IN PIO_CSQ_ACQUIRE_LOCK  CsqAcquireLock,
  IN PIO_CSQ_RELEASE_LOCK  CsqReleaseLock,
  IN PIO_CSQ_COMPLETE_CANCELED_IRP  CsqCompleteCanceledIrp);

NTKERNELAPI
VOID
NTAPI
IoCsqInsertIrp(
  IN  PIO_CSQ  Csq,
  IN  PIRP  Irp,
  IN  PIO_CSQ_IRP_CONTEXT  Context);

NTKERNELAPI
PIRP
NTAPI
IoCsqRemoveIrp(
  IN  PIO_CSQ  Csq,
  IN  PIO_CSQ_IRP_CONTEXT  Context);

NTKERNELAPI
PIRP
NTAPI
IoCsqRemoveNextIrp(
  IN PIO_CSQ  Csq,
  IN PVOID  PeekContext);

NTKERNELAPI
VOID
NTAPI
IoDeleteController(
  IN PCONTROLLER_OBJECT  ControllerObject);

NTKERNELAPI
VOID
NTAPI
IoDeleteDevice(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoDeleteSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName);

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

NTKERNELAPI
VOID
NTAPI
IoDetachDevice(
  IN OUT PDEVICE_OBJECT  TargetDevice);

NTKERNELAPI
VOID
NTAPI
IoDisconnectInterrupt(
  IN PKINTERRUPT  InterruptObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoForwardIrpSynchronously(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp);

#define IoForwardAndCatchIrp IoForwardIrpSynchronously

NTKERNELAPI
VOID
NTAPI
IoFreeController(
  IN PCONTROLLER_OBJECT  ControllerObject);

NTKERNELAPI
VOID
NTAPI
IoFreeErrorLogEntry(
  PVOID  ElEntry);

NTKERNELAPI
VOID
NTAPI
IoFreeIrp(
  IN PIRP  Irp);

NTKERNELAPI
VOID
NTAPI
IoFreeMdl(
  IN PMDL  Mdl);

NTKERNELAPI
VOID
NTAPI
IoFreeWorkItem(
  IN PIO_WORKITEM  pIOWorkItem);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetAttachedDevice(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetAttachedDeviceReference(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetBootDiskInformation(
  IN OUT PBOOTDISK_INFORMATION  BootDiskInformation,
  IN ULONG  Size);

NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(
  VOID);

NTKERNELAPI
PEPROCESS
NTAPI
IoGetCurrentProcess(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaceAlias(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN CONST GUID  *AliasInterfaceClassGuid,
  OUT PUNICODE_STRING  AliasSymbolicLinkName);

#define DEVICE_INTERFACE_INCLUDE_NONACTIVE 0x00000001

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaces(
  IN CONST GUID  *InterfaceClassGuid,
  IN PDEVICE_OBJECT  PhysicalDeviceObject  OPTIONAL,
  IN ULONG  Flags,
  OUT PWSTR  *SymbolicLinkList);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceObjectPointer(
  IN PUNICODE_STRING  ObjectName,
  IN ACCESS_MASK  DesiredAccess,
  OUT PFILE_OBJECT  *FileObject,
  OUT PDEVICE_OBJECT  *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceProperty(
  IN PDEVICE_OBJECT  DeviceObject,
  IN DEVICE_REGISTRY_PROPERTY  DeviceProperty,
  IN ULONG  BufferLength,
  OUT PVOID  PropertyBuffer,
  OUT PULONG  ResultLength);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(
  IN PETHREAD  Thread);

NTKERNELAPI
PDMA_ADAPTER
NTAPI
IoGetDmaAdapter(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PDEVICE_DESCRIPTION  DeviceDescription,
  IN OUT PULONG  NumberOfMapRegisters);

NTKERNELAPI
PVOID
NTAPI
IoGetDriverObjectExtension(
  IN PDRIVER_OBJECT  DriverObject,
  IN PVOID  ClientIdentificationAddress);

NTKERNELAPI
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(
  VOID);

/*
 * ULONG
 * IoGetFunctionCodeFromCtlCode(
 *   IN ULONG  ControlCode)
 */
#define IoGetFunctionCodeFromCtlCode(_ControlCode) \
  (((_ControlCode) >> 2) & 0x00000FFF)

NTKERNELAPI
PVOID
NTAPI
IoGetInitialStack(
  VOID);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetRelatedDeviceObject(
  IN PFILE_OBJECT  FileObject);

NTKERNELAPI
VOID
NTAPI
IoGetStackLimits(
  OUT PULONG_PTR  LowLimit,
  OUT PULONG_PTR  HighLimit);

FORCEINLINE
ULONG_PTR
IoGetRemainingStackSize(
  VOID
)
{
    ULONG_PTR End, Begin;
    ULONG_PTR Result;

    IoGetStackLimits(&Begin, &End);
    Result = (ULONG_PTR)(&End) - Begin;
    return Result;
}

NTKERNELAPI
VOID
NTAPI
KeInitializeDpc(
  IN PRKDPC  Dpc,
  IN PKDEFERRED_ROUTINE  DeferredRoutine,
  IN PVOID  DeferredContext);

/*
 * VOID
 * IoInitializeDpcRequest(
 *   IN PDEVICE_OBJECT DeviceObject,
 *   IN PIO_DPC_ROUTINE DpcRoutine)
 */
#define IoInitializeDpcRequest(_DeviceObject, \
                               _DpcRoutine) \
  KeInitializeDpc(&(_DeviceObject)->Dpc, \
    (PKDEFERRED_ROUTINE) (_DpcRoutine), \
    _DeviceObject)

NTKERNELAPI
VOID
NTAPI
IoInitializeIrp(
  IN OUT PIRP  Irp,
  IN USHORT  PacketSize,
  IN CCHAR  StackSize);

NTKERNELAPI
VOID
NTAPI
IoInitializeRemoveLockEx(
  IN  PIO_REMOVE_LOCK Lock,
  IN  ULONG   AllocateTag,
  IN  ULONG   MaxLockedMinutes,
  IN  ULONG   HighWatermark,
  IN  ULONG   RemlockSize);

/* VOID
 * IoInitializeRemoveLock(
 *   IN PIO_REMOVE_LOCK  Lock,
 *   IN ULONG  AllocateTag,
 *   IN ULONG  MaxLockedMinutes,
 *   IN ULONG  HighWatermark)
 */
#define IoInitializeRemoveLock( \
  Lock, AllocateTag, MaxLockedMinutes, HighWatermark) \
  IoInitializeRemoveLockEx(Lock, AllocateTag, MaxLockedMinutes, \
    HighWatermark, sizeof(IO_REMOVE_LOCK))

NTKERNELAPI
NTSTATUS
NTAPI
IoInitializeTimer(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIO_TIMER_ROUTINE  TimerRoutine,
  IN PVOID  Context);

NTKERNELAPI
VOID
NTAPI
IoInvalidateDeviceRelations(
  IN PDEVICE_OBJECT  DeviceObject,
  IN DEVICE_RELATION_TYPE  Type);

NTKERNELAPI
VOID
NTAPI
IoInvalidateDeviceState(
  IN PDEVICE_OBJECT  PhysicalDeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoIs32bitProcess(
  IN PIRP  Irp  OPTIONAL);

/*
 * BOOLEAN
 * IoIsErrorUserInduced(
 *   IN NTSTATUS  Status);
 */
#define IoIsErrorUserInduced(Status) \
   ((BOOLEAN)(((Status) == STATUS_DEVICE_NOT_READY) || \
   ((Status) == STATUS_IO_TIMEOUT) || \
   ((Status) == STATUS_MEDIA_WRITE_PROTECTED) || \
   ((Status) == STATUS_NO_MEDIA_IN_DEVICE) || \
   ((Status) == STATUS_VERIFY_REQUIRED) || \
   ((Status) == STATUS_UNRECOGNIZED_MEDIA) || \
   ((Status) == STATUS_WRONG_VOLUME)))

NTKERNELAPI
BOOLEAN
NTAPI
IoIsWdmVersionAvailable(
  IN UCHAR  MajorVersion,
  IN UCHAR  MinorVersion);

NTKERNELAPI
PIRP
NTAPI
IoMakeAssociatedIrp(
  IN PIRP  Irp,
  IN CCHAR  StackSize);

/*
 * VOID
 * IoMarkIrpPending(
 *   IN OUT PIRP  Irp)
 */
#define IoMarkIrpPending(_Irp) \
  (IoGetCurrentIrpStackLocation(_Irp)->Control |= SL_PENDING_RETURNED)

NTKERNELAPI
NTSTATUS
NTAPI
IoOpenDeviceInterfaceRegistryKey(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  DeviceInterfaceKey);

#define PLUGPLAY_REGKEY_DEVICE                            1
#define PLUGPLAY_REGKEY_DRIVER                            2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE                 4

NTKERNELAPI
NTSTATUS
NTAPI
IoOpenDeviceRegistryKey(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  DevInstKeyType,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  DevInstRegKey);

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
IoQueueWorkItem(
  IN PIO_WORKITEM  pIOWorkItem,
  IN PIO_WORKITEM_ROUTINE  Routine,
  IN WORK_QUEUE_TYPE  QueueType,
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
NTSTATUS
NTAPI
IoRegisterDeviceInterface(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN CONST GUID  *InterfaceClassGuid,
  IN PUNICODE_STRING  ReferenceString  OPTIONAL,
  OUT PUNICODE_STRING  SymbolicLinkName);

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
IoRegisterPlugPlayNotification(
  IN IO_NOTIFICATION_EVENT_CATEGORY  EventCategory,
  IN ULONG  EventCategoryFlags,
  IN PVOID  EventCategoryData  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE  CallbackRoutine,
  IN PVOID  Context,
  OUT PVOID  *NotificationEntry);

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterShutdownNotification(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoReleaseCancelSpinLock(
  IN KIRQL  Irql);

NTKERNELAPI
VOID
NTAPI
IoReleaseRemoveLockAndWaitEx(
  IN PIO_REMOVE_LOCK  RemoveLock,
  IN PVOID  Tag,
  IN ULONG  RemlockSize);

NTKERNELAPI
VOID
NTAPI
IoReleaseRemoveLockEx(
  IN PIO_REMOVE_LOCK  RemoveLock,
  IN PVOID  Tag,
  IN ULONG  RemlockSize);

/*
 * VOID
 * IoReleaseRemoveLock(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN PVOID  Tag)
 */
#define IoReleaseRemoveLock(_RemoveLock, \
                            _Tag) \
  IoReleaseRemoveLockEx(_RemoveLock, _Tag, sizeof(IO_REMOVE_LOCK))

/*
 * VOID
 * IoReleaseRemoveLockAndWait(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN PVOID  Tag)
 */
#define IoReleaseRemoveLockAndWait(_RemoveLock, \
                                   _Tag) \
  IoReleaseRemoveLockAndWaitEx(_RemoveLock, _Tag, sizeof(IO_REMOVE_LOCK))

NTKERNELAPI
VOID
NTAPI
IoRemoveShareAccess(
  IN PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess);

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
NTSTATUS
NTAPI
IoReportTargetDeviceChange(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PVOID  NotificationStructure);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PVOID  NotificationStructure,
  IN PDEVICE_CHANGE_COMPLETE_CALLBACK  Callback  OPTIONAL,
  IN PVOID  Context  OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRequestDeviceEject(
  IN PDEVICE_OBJECT  PhysicalDeviceObject);

/*
 * VOID
 * IoRequestDpc(
 *   IN PDEVICE_OBJECT  DeviceObject,
 *   IN PIRP  Irp,
 *   IN PVOID  Context);
 */
#define IoRequestDpc(DeviceObject, Irp, Context)( \
  KeInsertQueueDpc(&(DeviceObject)->Dpc, (Irp), (Context)))

NTKERNELAPI
VOID
NTAPI
IoReuseIrp(
  IN OUT PIRP  Irp,
  IN NTSTATUS  Status);

/*
 * PDRIVER_CANCEL
 * IoSetCancelRoutine(
 *   IN PIRP  Irp,
 *   IN PDRIVER_CANCEL  CancelRoutine)
 */
#define IoSetCancelRoutine(_Irp, \
                           _CancelRoutine) \
  ((PDRIVER_CANCEL) InterlockedExchangePointer( \
    (PVOID *) &(_Irp)->CancelRoutine, (PVOID) (_CancelRoutine)))

/*
 * VOID
 * IoSetCompletionRoutine(
 *   IN PIRP  Irp,
 *   IN PIO_COMPLETION_ROUTINE  CompletionRoutine,
 *   IN PVOID  Context,
 *   IN BOOLEAN  InvokeOnSuccess,
 *   IN BOOLEAN  InvokeOnError,
 *   IN BOOLEAN  InvokeOnCancel)
 */
#define IoSetCompletionRoutine(_Irp, \
                               _CompletionRoutine, \
                               _Context, \
                               _InvokeOnSuccess, \
                               _InvokeOnError, \
                               _InvokeOnCancel) \
{ \
  PIO_STACK_LOCATION _IrpSp; \
  ASSERT((_InvokeOnSuccess) || (_InvokeOnError) || (_InvokeOnCancel) ? \
    (_CompletionRoutine) != NULL : TRUE); \
  _IrpSp = IoGetNextIrpStackLocation(_Irp); \
  _IrpSp->CompletionRoutine = (PIO_COMPLETION_ROUTINE)(_CompletionRoutine); \
  _IrpSp->Context = (_Context); \
  _IrpSp->Control = 0; \
  if (_InvokeOnSuccess) _IrpSp->Control = SL_INVOKE_ON_SUCCESS; \
  if (_InvokeOnError) _IrpSp->Control |= SL_INVOKE_ON_ERROR; \
  if (_InvokeOnCancel) _IrpSp->Control |= SL_INVOKE_ON_CANCEL; \
}

NTKERNELAPI
NTSTATUS
NTAPI
IoSetCompletionRoutineEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN PIO_COMPLETION_ROUTINE  CompletionRoutine,
  IN PVOID  Context,
  IN BOOLEAN  InvokeOnSuccess,
  IN BOOLEAN  InvokeOnError,
  IN BOOLEAN  InvokeOnCancel);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetDeviceInterfaceState(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN BOOLEAN  Enable);

NTKERNELAPI
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(
  IN PIRP  Irp,
  IN PDEVICE_OBJECT  DeviceObject);

/*
 * VOID
 * IoSetNextIrpStackLocation(
 *   IN OUT PIRP  Irp)
 */
#define IoSetNextIrpStackLocation(_Irp) \
{ \
  (_Irp)->CurrentLocation--; \
  (_Irp)->Tail.Overlay.CurrentStackLocation--; \
}

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
VOID
NTAPI
IoSetShareAccess(
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  DesiredShareAccess,
  IN OUT PFILE_OBJECT  FileObject,
  OUT PSHARE_ACCESS  ShareAccess);

NTKERNELAPI
VOID
NTAPI
IoSetStartIoAttributes(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  DeferredStartIo,
  IN BOOLEAN  NonCancelable);

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

/*
 * USHORT
 * IoSizeOfIrp(
 *   IN CCHAR  StackSize)
 */
#define IoSizeOfIrp(_StackSize) \
  ((USHORT) (sizeof(IRP) + ((_StackSize) * (sizeof(IO_STACK_LOCATION)))))

/*
 * VOID
 * IoSkipCurrentIrpStackLocation(
 *   IN PIRP  Irp)
 */
#define IoSkipCurrentIrpStackLocation(_Irp) \
{ \
  (_Irp)->CurrentLocation++; \
  (_Irp)->Tail.Overlay.CurrentStackLocation++; \
}

NTKERNELAPI
VOID
NTAPI
IoStartNextPacket(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  Cancelable);

NTKERNELAPI
VOID
NTAPI
IoStartNextPacketByKey(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  Cancelable,
  IN ULONG  Key);

NTKERNELAPI
VOID
NTAPI
IoStartPacket(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN PULONG  Key  OPTIONAL,
  IN PDRIVER_CANCEL  CancelFunction  OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoStartTimer(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoStopTimer(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotification(
  IN PVOID  NotificationEntry);

NTKERNELAPI
VOID
NTAPI
IoUnregisterShutdownNotification(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoUpdateShareAccess(
  IN PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess);

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
NTAPI
IoWMIAllocateInstanceIds(
  IN GUID  *Guid,
  IN ULONG  InstanceCount,
  OUT ULONG  *FirstInstanceId);

NTKERNELAPI
ULONG
NTAPI
IoWMIDeviceObjectToProviderId(
  IN PDEVICE_OBJECT  DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIDeviceObjectToInstanceName(
  IN PVOID  DataBlockObject,
  IN PDEVICE_OBJECT  DeviceObject,
  OUT PUNICODE_STRING  InstanceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIExecuteMethod(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  MethodId,
  IN ULONG  InBufferSize,
  IN OUT PULONG  OutBufferSize,
  IN OUT  PUCHAR  InOutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIHandleToInstanceName(
  IN PVOID  DataBlockObject,
  IN HANDLE  FileHandle,
  OUT PUNICODE_STRING  InstanceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIOpenBlock(
  IN GUID  *DataBlockGuid,
  IN ULONG  DesiredAccess,
  OUT PVOID  *DataBlockObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQueryAllData(
  IN PVOID  DataBlockObject,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQueryAllDataMultiple(
  IN PVOID  *DataBlockObjectList,
  IN ULONG  ObjectCount,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQuerySingleInstance(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIQuerySingleInstanceMultiple(
  IN PVOID  *DataBlockObjectList,
  IN PUNICODE_STRING  InstanceNames,
  IN ULONG  ObjectCount,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIRegistrationControl(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  Action);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetNotificationCallback(
  IN PVOID  Object,
  IN WMI_NOTIFICATION_CALLBACK  Callback,
  IN PVOID  Context);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetSingleInstance(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  Version,
  IN ULONG  ValueBufferSize,
  IN PVOID  ValueBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISetSingleItem(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  DataItemId,
  IN ULONG  Version,
  IN ULONG  ValueBufferSize,
  IN PVOID  ValueBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMISuggestInstanceName(
  IN PDEVICE_OBJECT  PhysicalDeviceObject OPTIONAL,
  IN PUNICODE_STRING  SymbolicLinkName OPTIONAL,
  IN BOOLEAN  CombineNames,
  OUT PUNICODE_STRING  SuggestedInstanceName);

NTKERNELAPI
NTSTATUS
NTAPI
IoWMIWriteEvent(
  IN PVOID  WnodeEventItem);

NTKERNELAPI
VOID
NTAPI
IoWriteErrorLogEntry(
  IN PVOID  ElEntry);

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



/** Kernel routines **/

#if defined (_M_AMD64)
NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
  IN PKSPIN_LOCK  SpinLock,
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
  IN PKLOCK_QUEUE_HANDLE  LockHandle);
#else
NTHALAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
  IN PKSPIN_LOCK  SpinLock,
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTHALAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
  IN PKLOCK_QUEUE_HANDLE  LockHandle);
#endif

NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
  IN PKSPIN_LOCK  SpinLock,
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTKERNELAPI
KIRQL
NTAPI
KeAcquireInterruptSpinLock(
  IN PKINTERRUPT  Interrupt);

NTKERNELAPI
BOOLEAN
NTAPI
KeAreApcsDisabled(
  VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  IN ULONG  BugCheckCode);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckEx(
  IN ULONG  BugCheckCode,
  IN ULONG_PTR  BugCheckParameter1,
  IN ULONG_PTR  BugCheckParameter2,
  IN ULONG_PTR  BugCheckParameter3,
  IN ULONG_PTR  BugCheckParameter4);

NTKERNELAPI
BOOLEAN
NTAPI
KeCancelTimer(
  IN PKTIMER  Timer);

NTKERNELAPI
VOID
NTAPI
KeClearEvent(
  IN PRKEVENT  Event);

NTKERNELAPI
NTSTATUS
NTAPI
KeDelayExecutionThread(
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Interval);

NTKERNELAPI
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(
  IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord);

NTKERNELAPI
VOID
NTAPI
KeEnterCriticalRegion(
  VOID);

/*
 * VOID
 * KeFlushIoBuffers(
 *   IN PMDL  Mdl,
 *   IN BOOLEAN  ReadOperation,
 *   IN BOOLEAN  DmaOperation)
 */
#define KeFlushIoBuffers(_Mdl, _ReadOperation, _DmaOperation)

#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)

NTKERNELAPI
VOID
NTAPI
KeFlushQueuedDpcs(
    VOID
);

NTHALAPI
VOID
NTAPI
KeFlushWriteBuffer(VOID);

NTKERNELAPI
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(
  VOID);

NTKERNELAPI
VOID
NTAPI
KeInitializeDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue);

NTKERNELAPI
VOID
NTAPI
KeInitializeMutex(
  IN PRKMUTEX  Mutex,
  IN ULONG  Level);

NTKERNELAPI
VOID
NTAPI
KeInitializeSemaphore(
  IN PRKSEMAPHORE  Semaphore,
  IN LONG  Count,
  IN LONG  Limit);

NTKERNELAPI
VOID
NTAPI
KeInitializeTimer(
  IN PKTIMER  Timer);

NTKERNELAPI
VOID
NTAPI
KeInitializeTimerEx(
  IN PKTIMER  Timer,
  IN TIMER_TYPE  Type);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertByKeyDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry,
  IN ULONG  SortKey);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueDpc(
  IN PRKDPC  Dpc,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

NTKERNELAPI
VOID
NTAPI
KeLeaveCriticalRegion(
  VOID);

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

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryActiveProcessors(
    VOID
);

NTHALAPI
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
  OUT PLARGE_INTEGER  PerformanceFrequency  OPTIONAL);

NTKERNELAPI
KPRIORITY
NTAPI
KeQueryPriorityThread(
  IN PRKTHREAD  Thread);

NTKERNELAPI
ULONG
NTAPI
KeQueryRuntimeThread(
  IN PKTHREAD Thread,
  OUT PULONG UserTime);

#if !defined(_M_AMD64)
NTKERNELAPI
ULONGLONG
NTAPI
KeQueryInterruptTime(
  VOID);

NTKERNELAPI
VOID
NTAPI
KeQuerySystemTime(
  OUT PLARGE_INTEGER  CurrentTime);

NTKERNELAPI
VOID
NTAPI
KeQueryTickCount(
  OUT PLARGE_INTEGER  TickCount);
#endif

NTKERNELAPI
ULONG
NTAPI
KeQueryTimeIncrement(
  VOID);

NTKERNELAPI
LONG
NTAPI
KeReadStateEvent(
  IN PRKEVENT  Event);

NTKERNELAPI
LONG
NTAPI
KeReadStateMutex(
  IN PRKMUTEX  Mutex);


NTKERNELAPI
LONG
NTAPI
KeReadStateSemaphore(
  IN PRKSEMAPHORE  Semaphore);

NTKERNELAPI
BOOLEAN
NTAPI
KeReadStateTimer(
  IN PKTIMER  Timer);

NTKERNELAPI
BOOLEAN
NTAPI
KeRegisterBugCheckCallback(
  IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord,
  IN PKBUGCHECK_CALLBACK_ROUTINE  CallbackRoutine,
  IN PVOID  Buffer,
  IN ULONG  Length,
  IN PUCHAR  Component);

NTKERNELAPI
PVOID
NTAPI
KeRegisterNmiCallback(
  IN PNMI_CALLBACK CallbackRoutine,
  IN PVOID Context
);

NTKERNELAPI
NTSTATUS
NTAPI
KeDeregisterNmiCallback(
  IN PVOID Handle
);

NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTKERNELAPI
VOID
NTAPI
KeReleaseInterruptSpinLock(
  IN PKINTERRUPT  Interrupt,
  IN KIRQL  OldIrql);

NTKERNELAPI
LONG
NTAPI
KeReleaseMutex(
  IN PRKMUTEX  Mutex,
  IN BOOLEAN  Wait);

NTKERNELAPI
LONG
NTAPI
KeReleaseSemaphore(
  IN PRKSEMAPHORE  Semaphore,
  IN KPRIORITY  Increment,
  IN LONG  Adjustment,
  IN BOOLEAN  Wait);

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN ULONG  SortKey);

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue);

NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveEntryDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry);

NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveQueueDpc(
  IN PRKDPC  Dpc);

NTKERNELAPI
LONG
NTAPI
KeResetEvent(
  IN PRKEVENT  Event);

NTKERNELAPI
NTSTATUS
NTAPI
KeRestoreFloatingPointState(
  IN PKFLOATING_SAVE  FloatSave);

NTKERNELAPI
VOID
NTAPI
KeRevertToUserAffinityThread(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KeSaveFloatingPointState(
  OUT PKFLOATING_SAVE  FloatSave);

NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  IN PRKTHREAD  Thread,
  IN LONG  Increment);

NTKERNELAPI
LONG
NTAPI
KeSetEvent(
  IN PRKEVENT  Event,
  IN KPRIORITY  Increment,
  IN BOOLEAN  Wait);

NTKERNELAPI
VOID
NTAPI
KeSetImportanceDpc(
  IN PRKDPC  Dpc,
  IN KDPC_IMPORTANCE  Importance);

NTKERNELAPI
KPRIORITY
NTAPI
KeSetPriorityThread(
  IN PKTHREAD  Thread,
  IN KPRIORITY  Priority);

NTKERNELAPI
VOID
NTAPI
KeSetSystemAffinityThread(
    IN KAFFINITY Affinity);

NTKERNELAPI
VOID
NTAPI
KeSetTargetProcessorDpc(
  IN PRKDPC  Dpc,
  IN CCHAR  Number);

NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimer(
  IN PKTIMER  Timer,
  IN LARGE_INTEGER  DueTime,
  IN PKDPC  Dpc  OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimerEx(
  IN PKTIMER  Timer,
  IN LARGE_INTEGER  DueTime,
  IN LONG  Period  OPTIONAL,
  IN PKDPC  Dpc  OPTIONAL);

NTKERNELAPI
VOID
FASTCALL
KeSetTimeUpdateNotifyRoutine(
  IN PTIME_UPDATE_NOTIFY_ROUTINE  NotifyRoutine);

NTHALAPI
VOID
NTAPI
KeStallExecutionProcessor(
  IN ULONG  MicroSeconds);

NTKERNELAPI
BOOLEAN
NTAPI
KeSynchronizeExecution(
  IN PKINTERRUPT    Interrupt,
  IN PKSYNCHRONIZE_ROUTINE  SynchronizeRoutine,
  IN PVOID  SynchronizeContext);

NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForMultipleObjects(
  IN ULONG  Count,
  IN PVOID  Object[],
  IN WAIT_TYPE  WaitType,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL,
  IN PKWAIT_BLOCK  WaitBlockArray  OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForMutexObject(
  IN PRKMUTEX  Mutex,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForSingleObject(
  IN PVOID  Object,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL);

typedef
ULONG_PTR
(NTAPI *PKIPI_BROADCAST_WORKER)(
    IN ULONG_PTR Argument
);

NTKERNELAPI
ULONG_PTR
NTAPI
KeIpiGenericCall(
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context
);

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
NTSTATUS
NTAPI
MmAdvanceMdl(
  IN PMDL  Mdl,
  IN ULONG  NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemory(
  IN ULONG  NumberOfBytes,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(
  IN SIZE_T  NumberOfBytes,
  IN PHYSICAL_ADDRESS  LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS  BoundaryAddressMultiple  OPTIONAL,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
PVOID
NTAPI
MmAllocateMappingAddress(
  IN SIZE_T  NumberOfBytes,
  IN ULONG  PoolTag);

NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  IN ULONG  NumberOfBytes);

NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdl(
  IN PHYSICAL_ADDRESS  LowAddress,
  IN PHYSICAL_ADDRESS  HighAddress,
  IN PHYSICAL_ADDRESS  SkipBytes,
  IN SIZE_T  TotalBytes);

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdlEx(
  IN PHYSICAL_ADDRESS LowAddress,
  IN PHYSICAL_ADDRESS HighAddress,
  IN PHYSICAL_ADDRESS SkipBytes,
  IN SIZE_T TotalBytes,
  IN MEMORY_CACHING_TYPE CacheType,
  IN ULONG Flags);
#endif

NTKERNELAPI
VOID
NTAPI
MmBuildMdlForNonPagedPool(
  IN OUT PMDL  MemoryDescriptorList);

typedef enum _MMFLUSH_TYPE {
  MmFlushForDelete,
  MmFlushForWrite
} MMFLUSH_TYPE;

NTKERNELAPI
BOOLEAN
NTAPI
MmFlushImageSection(
  IN PSECTION_OBJECT_POINTERS  SectionObjectPointer,
  IN MMFLUSH_TYPE  FlushType);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemory(
  IN PVOID  BaseAddress);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
VOID
NTAPI
MmFreeMappingAddress(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag);

NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmFreePagesFromMdl(
  IN PMDL  MemoryDescriptorList);

/*
 * ULONG
 * MmGetMdlByteCount(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlByteCount(_Mdl) \
  ((_Mdl)->ByteCount)

/*
 * ULONG
 * MmGetMdlByteOffset(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlByteOffset(_Mdl) \
  ((_Mdl)->ByteOffset)

/*
 * PPFN_NUMBER
 * MmGetMdlPfnArray(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlPfnArray(_Mdl) \
  ((PPFN_NUMBER) ((_Mdl) + 1))

/*
 * PVOID
 * MmGetMdlVirtualAddress(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlVirtualAddress(_Mdl) \
  ((PVOID) ((PCHAR) ((_Mdl)->StartVa) + (_Mdl)->ByteOffset))

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
PVOID
NTAPI
MmMapLockedPagesSpecifyCache(
  IN PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode,
  IN MEMORY_CACHING_TYPE  CacheType,
  IN PVOID  BaseAddress,
  IN ULONG  BugCheckOnFailure,
  IN MM_PAGE_PRIORITY  Priority);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(
  IN PVOID  MappingAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList,
  IN MEMORY_CACHING_TYPE  CacheType);

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

NTKERNELAPI
PVOID
NTAPI
MmGetSystemRoutineAddress(
  IN PUNICODE_STRING  SystemRoutineName);

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

/*
 * VOID
 * MmInitializeMdl(
 *   IN PMDL  MemoryDescriptorList,
 *   IN PVOID  BaseVa,
 *   IN SIZE_T  Length)
 */
#define MmInitializeMdl(_MemoryDescriptorList, \
                        _BaseVa, \
                        _Length) \
{ \
  (_MemoryDescriptorList)->Next = (PMDL) NULL; \
  (_MemoryDescriptorList)->Size = (CSHORT) (sizeof(MDL) + \
    (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(_BaseVa, _Length))); \
  (_MemoryDescriptorList)->MdlFlags = 0; \
  (_MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN(_BaseVa); \
  (_MemoryDescriptorList)->ByteOffset = BYTE_OFFSET(_BaseVa); \
  (_MemoryDescriptorList)->ByteCount = (ULONG) _Length; \
}

NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  IN PVOID  VirtualAddress);

NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifying(
  IN PDRIVER_OBJECT  DriverObject);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
MmIsVerifierEnabled(
  OUT PULONG  VerifierFlags);

NTKERNELAPI
PVOID
NTAPI
MmLockPagableDataSection(
  IN PVOID  AddressWithinSection);

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
MmMapIoSpace(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN ULONG  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheEnable);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPages(
  IN PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode);

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
PVOID
NTAPI
MmPageEntireDriver(
  IN PVOID  AddressWithinSection);

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockProcessPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN PEPROCESS  Process,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTKERNELAPI
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(
  IN PMDL  MemoryDescriptorList,
  IN ULONG  NewProtect);

NTKERNELAPI
VOID
NTAPI
MmUnmapLockedPages(
  IN PVOID  BaseAddress,
  IN PMDL  MemoryDescriptorList);

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

/*
 * VOID
 * MmPrepareMdlForReuse(
 *   IN PMDL  Mdl)
 */
#define MmPrepareMdlForReuse(_Mdl) \
{ \
  if (((_Mdl)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) { \
    ASSERT(((_Mdl)->MdlFlags & MDL_PARTIAL) != 0); \
    MmUnmapLockedPages((_Mdl)->MappedSystemVa, (_Mdl)); \
  } else if (((_Mdl)->MdlFlags & MDL_PARTIAL) == 0) { \
    ASSERT(((_Mdl)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0); \
  } \
}

#define MmGetProcedureAddress(Address) (Address)

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTKERNELAPI
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmResetDriverPaging(
  IN PVOID  AddressWithinSection);

NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  IN PVOID  Address,
  IN SIZE_T  Size,
  IN ULONG  ProbeMode);

NTKERNELAPI
SIZE_T
NTAPI
MmSizeOfMdl(
  IN PVOID  Base,
  IN SIZE_T  Length);

NTKERNELAPI
VOID
NTAPI
MmUnlockPagableImageSection(
  IN PVOID  ImageSectionHandle);

NTKERNELAPI
VOID
NTAPI
MmUnlockPages(
  IN PMDL  MemoryDescriptorList);

NTKERNELAPI
VOID
NTAPI
MmUnmapIoSpace(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmUnmapReservedMapping(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList);

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
LONG_PTR
FASTCALL
ObfDereferenceObject(
  IN PVOID  Object);

/*
 * VOID
 * ObDereferenceObject(
 *   IN PVOID  Object)
 */
#define ObDereferenceObject ObfDereferenceObject

NTKERNELAPI
NTSTATUS
NTAPI
ObGetObjectSecurity(
  IN PVOID  Object,
  OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
  OUT PBOOLEAN  MemoryAllocated);

NTKERNELAPI
NTSTATUS
NTAPI
ObInsertObject(
  IN PVOID  Object,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  AdditionalReferences,
  OUT PVOID*  ReferencedObject  OPTIONAL,
  OUT PHANDLE  Handle);

NTKERNELAPI
LONG_PTR
FASTCALL
ObfReferenceObject(
  IN PVOID  Object);

NTKERNELAPI
NTSTATUS
NTAPI
ObLogSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  InputSecurityDescriptor,
  OUT PSECURITY_DESCRIPTOR  *OutputSecurityDescriptor,
  IN ULONG RefBias);
/*
 * VOID
 * ObReferenceObject(
 *   IN PVOID  Object)
 */
#define ObReferenceObject ObfReferenceObject

NTKERNELAPI
VOID
NTAPI
ObMakeTemporaryObject(
  IN PVOID  Object);

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
ObOpenObjectByPointer(
  IN PVOID  Object,
  IN ULONG  HandleAttributes,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess  OPTIONAL,
  IN POBJECT_TYPE  ObjectType  OPTIONAL,
  IN KPROCESSOR_MODE  AccessMode,
  OUT PHANDLE  Handle);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle(
  IN HANDLE  Handle,
  OUT PBOOLEAN  GenerateOnClose);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByHandle(
  IN HANDLE  Handle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_TYPE  ObjectType  OPTIONAL,
  IN KPROCESSOR_MODE  AccessMode,
  OUT PVOID  *Object,
  OUT POBJECT_HANDLE_INFORMATION  HandleInformation  OPTIONAL);

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
NTSTATUS
NTAPI
ObReferenceObjectByPointer(
  IN PVOID  Object,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_TYPE  ObjectType,
  IN KPROCESSOR_MODE  AccessMode);

NTKERNELAPI
VOID
NTAPI
ObReferenceSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  Count);

NTKERNELAPI
VOID
NTAPI
ObReleaseObjectSecurity(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN BOOLEAN  MemoryAllocated);



/** Process manager routines **/

NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemProcess(
  IN PHANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemThread(
  OUT PHANDLE  ThreadHandle,
  IN ULONG  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN HANDLE  ProcessHandle  OPTIONAL,
  OUT PCLIENT_ID  ClientId  OPTIONAL,
  IN PKSTART_ROUTINE  StartRoutine,
  IN PVOID  StartContext);

/*
 * PEPROCESS
 * PsGetCurrentProcess(VOID)
 */
#define PsGetCurrentProcess IoGetCurrentProcess

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(
  VOID);

/*
 * PETHREAD
 * PsGetCurrentThread(VOID)
 */
#define PsGetCurrentThread() \
  ((PETHREAD) KeGetCurrentThread())

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

NTKERNELAPI
NTSTATUS
NTAPI
PsTerminateSystemThread(
  IN NTSTATUS  ExitStatus);

extern NTSYSAPI PEPROCESS PsInitialSystemProcess;


/** Security reference monitor routines **/

NTKERNELAPI
BOOLEAN
NTAPI
SeAccessCheck(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT  SubjectSecurityContext,
  IN BOOLEAN  SubjectContextLocked,
  IN ACCESS_MASK  DesiredAccess,
  IN ACCESS_MASK  PreviouslyGrantedAccess,
  OUT PPRIVILEGE_SET  *Privileges  OPTIONAL,
  IN PGENERIC_MAPPING  GenericMapping,
  IN KPROCESSOR_MODE  AccessMode,
  OUT PACCESS_MASK  GrantedAccess,
  OUT PNTSTATUS  AccessStatus);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurity(
  IN PSECURITY_DESCRIPTOR  ParentDescriptor  OPTIONAL,
  IN PSECURITY_DESCRIPTOR  ExplicitDescriptor  OPTIONAL,
  OUT PSECURITY_DESCRIPTOR  *NewDescriptor,
  IN BOOLEAN  IsDirectoryObject,
  IN PSECURITY_SUBJECT_CONTEXT  SubjectContext,
  IN PGENERIC_MAPPING  GenericMapping,
  IN POOL_TYPE  PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurityEx(
  IN PSECURITY_DESCRIPTOR  ParentDescriptor  OPTIONAL,
  IN PSECURITY_DESCRIPTOR  ExplicitDescriptor  OPTIONAL,
  OUT PSECURITY_DESCRIPTOR  *NewDescriptor,
  IN GUID  *ObjectType  OPTIONAL,
  IN BOOLEAN  IsDirectoryObject,
  IN ULONG  AutoInheritFlags,
  IN PSECURITY_SUBJECT_CONTEXT  SubjectContext,
  IN PGENERIC_MAPPING  GenericMapping,
  IN POOL_TYPE  PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeDeassignSecurity(
  IN OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  LUID  PrivilegeValue,
  KPROCESSOR_MODE  PreviousMode);

NTKERNELAPI
BOOLEAN
NTAPI
SeValidSecurityDescriptor(
  IN ULONG  Length,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);



/** NtXxx routines **/

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



/** NtXxx and ZwXxx routines **/

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

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
  IN HANDLE  Handle);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
  OUT PHANDLE  DirectoryHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

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
ZwCreateFile(
  OUT PHANDLE  FileHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN PLARGE_INTEGER  AllocationSize  OPTIONAL,
  IN ULONG  FileAttributes,
  IN ULONG  ShareAccess,
  IN ULONG  CreateDisposition,
  IN ULONG  CreateOptions,
  IN PVOID  EaBuffer  OPTIONAL,
  IN ULONG  EaLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
  OUT PHANDLE  KeyHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN ULONG  TitleIndex,
  IN PUNICODE_STRING  Class  OPTIONAL,
  IN ULONG  CreateOptions,
  OUT PULONG  Disposition  OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN TIMER_TYPE  TimerType);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
  IN HANDLE  KeyHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
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

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
  IN HANDLE  KeyHandle,
  IN ULONG  Index,
  IN KEY_INFORMATION_CLASS  KeyInformationClass,
  OUT PVOID  KeyInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
  IN HANDLE  KeyHandle,
  IN ULONG  Index,
  IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
  OUT PVOID  KeyValueInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
  IN HANDLE  KeyHandle);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
  IN HANDLE  Handle);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
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
ZwOpenKey(
  OUT PHANDLE  KeyHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
  OUT PHANDLE  SectionHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
  OUT PHANDLE  LinkHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
  IN HANDLE  FileHandle,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  OUT PVOID  FileInformation,
  IN ULONG  Length,
  IN FILE_INFORMATION_CLASS  FileInformationClass);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
  IN HANDLE  KeyHandle,
  IN KEY_INFORMATION_CLASS  KeyInformationClass,
  OUT PVOID  KeyInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
  IN HANDLE  LinkHandle,
  IN OUT PUNICODE_STRING  LinkTarget,
  OUT PULONG  ReturnedLength  OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName,
  IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
  OUT PVOID  KeyValueInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
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
ZwSetInformationFile(
  IN HANDLE  FileHandle,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN PVOID  FileInformation,
  IN ULONG  Length,
  IN FILE_INFORMATION_CLASS  FileInformationClass);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName,
  IN ULONG  TitleIndex  OPTIONAL,
  IN ULONG  Type,
  IN PVOID  Data,
  IN ULONG  DataSize);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
  IN HANDLE  ProcessHandle,
  IN PVOID  BaseAddress);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
  IN HANDLE  ObjectHandle,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  TimeOut  OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
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

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
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
PoCallDriver(
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PIRP  Irp);

NTKERNELAPI
PULONG
NTAPI
PoRegisterDeviceForIdleDetection(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  ConservationIdleTime,
  IN ULONG  PerformanceIdleTime,
  IN DEVICE_POWER_STATE  State);

NTKERNELAPI
PVOID
NTAPI
PoRegisterSystemState(
  IN PVOID  StateHandle,
  IN EXECUTION_STATE  Flags);

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestPowerIrp(
  IN PDEVICE_OBJECT  DeviceObject,
  IN UCHAR  MinorFunction,
  IN POWER_STATE  PowerState,
  IN PREQUEST_POWER_COMPLETE  CompletionFunction,
  IN PVOID  Context,
  OUT PIRP  *Irp OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestShutdownEvent(
  OUT PVOID  *Event);

NTKERNELAPI
VOID
NTAPI
PoSetDeviceBusy(
  PULONG  IdlePointer);

#define PoSetDeviceBusy(IdlePointer) \
 ((void)(*(IdlePointer) = 0))

NTKERNELAPI
POWER_STATE
NTAPI
PoSetPowerState(
  IN PDEVICE_OBJECT  DeviceObject,
  IN POWER_STATE_TYPE  Type,
  IN POWER_STATE  State);

NTKERNELAPI
VOID
NTAPI
PoSetSystemState(
  IN EXECUTION_STATE  Flags);

NTKERNELAPI
VOID
NTAPI
PoStartNextPowerIrp(
  IN PIRP  Irp);

NTKERNELAPI
VOID
NTAPI
PoUnregisterSystemState(
  IN PVOID  StateHandle);



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

NTKERNELAPI
NTSTATUS
NTAPI
WmiQueryTraceInformation(
  IN TRACE_INFORMATION_CLASS  TraceInformationClass,
  OUT PVOID  TraceInformation,
  IN ULONG  TraceInformationLength,
  OUT PULONG  RequiredLength OPTIONAL,
  IN PVOID  Buffer OPTIONAL);

NTSTATUS
NTAPI
WmiSystemControl(
  IN PWMILIB_CONTEXT  WmiLibInfo,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  OUT PSYSCTL_IRP_DISPOSITION  IrpDisposition);

NTKERNELAPI
NTSTATUS
DDKCDECLAPI
WmiTraceMessage(
  IN TRACEHANDLE  LoggerHandle,
  IN ULONG  MessageFlags,
  IN LPGUID  MessageGuid,
  IN USHORT  MessageNumber,
  IN ...);

#if 0
/* FIXME: Get va_list from where? */
NTKERNELAPI
NTSTATUS
DDKCDECLAPI
WmiTraceMessageVa(
  IN TRACEHANDLE  LoggerHandle,
  IN ULONG  MessageFlags,
  IN LPGUID  MessageGuid,
  IN USHORT  MessageNumber,
  IN va_list  MessageArgList);
#endif


/** Kernel debugger routines **/

NTKERNELAPI
NTSTATUS
NTAPI
KdDisableDebugger(
  VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KdEnableDebugger(
  VOID);

NTKERNELAPI
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(
    VOID
);

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
NTSTATUS
NTAPI
KdChangeOption(
  IN KD_OPTION Option,
  IN ULONG InBufferBytes OPTIONAL,
  IN PVOID InBuffer,
  IN ULONG OutBufferBytes OPTIONAL,
  OUT PVOID OutBuffer,
  OUT PULONG OutBufferNeeded OPTIONAL);
#endif

VOID
NTAPI
DbgBreakPoint(
  VOID);

NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
  IN ULONG  Status);

ULONG
DDKCDECLAPI
DbgPrint(
  IN PCCH  Format,
  IN ...);

NTSYSAPI
ULONG
DDKCDECLAPI
DbgPrintEx(
  IN ULONG  ComponentId,
  IN ULONG  Level,
  IN PCCH  Format,
  IN ...);

ULONG
NTAPI
vDbgPrintEx(
  IN ULONG ComponentId,
  IN ULONG Level,
  IN PCCH Format,
  IN va_list ap);

ULONG
NTAPI
vDbgPrintExWithPrefix(
  IN PCCH Prefix,
  IN ULONG ComponentId,
  IN ULONG Level,
  IN PCCH Format,
  IN va_list ap);

NTKERNELAPI
ULONG
DDKCDECLAPI
DbgPrintReturnControlC(
  IN PCCH  Format,
  IN ...);

ULONG
NTAPI
DbgPrompt(
    IN PCCH Prompt,
    OUT PCH Response,
    IN ULONG MaximumResponseLength
);

NTKERNELAPI
NTSTATUS
NTAPI
DbgQueryDebugFilterState(
  IN ULONG  ComponentId,
  IN ULONG  Level);

NTKERNELAPI
NTSTATUS
NTAPI
DbgSetDebugFilterState(
  IN ULONG  ComponentId,
  IN ULONG  Level,
  IN BOOLEAN  State);

#if DBG

#define KdPrint(_x_) DbgPrint _x_
#define KdPrintEx(_x_) DbgPrintEx _x_
#define KdBreakPoint() DbgBreakPoint()
#define KdBreakPointWithStatus(s) DbgBreakPointWithStatus(s)

#else /* !DBG */

#define KdPrint(_x_)
#define KdPrintEx(_x_)
#define KdBreakPoint()
#define KdBreakPointWithStatus(s)

#endif /* !DBG */

#if defined(__GNUC__)

extern NTKERNELAPI BOOLEAN KdDebuggerNotPresent;
extern NTKERNELAPI BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#elif defined(_NTDDK_) || defined(_NTHAL_) || defined(_WDMDDK_) || defined(_NTOSP_)

extern NTKERNELAPI PBOOLEAN KdDebuggerNotPresent;
extern NTKERNELAPI PBOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     *KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT *KdDebuggerNotPresent

#else

extern BOOLEAN KdDebuggerNotPresent;
extern BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#endif

/** Stuff from winnt4.h */

#ifndef DMA_MACROS_DEFINED

#if (NTDDI_VERSION >= NTDDI_WIN2K)

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
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject);

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
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice);


#endif // (NTDDI_VERSION >= NTDDI_WIN2K)
#endif // !defined(DMA_MACROS_DEFINED)

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
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

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
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

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
HalReadDmaCounter(
  IN PADAPTER_OBJECT  AdapterObject);

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
  IN PLONG  Addend);

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

#define ExInterlockedIncrementLong(Addend,Lock) Exfi386InterlockedIncrementLong(Addend)
#define ExInterlockedDecrementLong(Addend,Lock) Exfi386InterlockedDecrementLong(Addend)
#define ExInterlockedExchangeUlong(Target, Value, Lock) Exfi386InterlockedExchangeUlong(Target, Value)

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
