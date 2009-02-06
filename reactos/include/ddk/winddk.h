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

#ifdef __cplusplus
extern "C" {
#endif

#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

#ifdef __GNUC__
#include "intrin.h"
#endif

#ifdef _NTOSKRNL_
/* HACKHACKHACK!!! We shouldn't include this header from ntoskrnl! */
#define NTKERNELAPI
#else
#define NTKERNELAPI DECLSPEC_IMPORT
#endif

#if !defined(_NTHAL_)
#define NTHALAPI DECLSPEC_IMPORT
#else
#define NTHALAPI
#endif

/* Pseudo modifiers for parameters */
#define IN
#define OUT
#define OPTIONAL
#define UNALLIGNED

#define CONST const
#define VOLATILE volatile

#define RESTRICTED_POINTER
#define POINTER_ALIGNMENT
#define DECLSPEC_ADDRSAFE

#ifdef NONAMELESSUNION
# define _DDK_DUMMYUNION_MEMBER(name) DUMMYUNIONNAME.name
# define _DDK_DUMMYUNION_N_MEMBER(n, name) DUMMYUNIONNAME##n.name
#else
# define _DDK_DUMMYUNION_MEMBER(name) name
# define _DDK_DUMMYUNION_N_MEMBER(n, name) name
#endif

#if !defined(_NTSYSTEM_)
#define NTSYSAPI     DECLSPEC_IMPORT
#define NTSYSCALLAPI DECLSPEC_IMPORT
#else
#define NTSYSAPI
#if defined(_NTDLLBUILD_)
#define NTSYSCALLAPI
#else
#define NTSYSCALLAPI DECLSPEC_ADDRSAFE
#endif
#endif

/*
 * Alignment Macros
 */
#define ALIGN_DOWN(s, t) \
    ((ULONG)(s) & ~(sizeof(t) - 1))

#define ALIGN_UP(s, t) \
    (ALIGN_DOWN(((ULONG)(s) + sizeof(t) - 1), t))

#define ALIGN_DOWN_POINTER(p, t) \
    ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)sizeof(t) - 1)))

#define ALIGN_UP_POINTER(p, t) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(p) + sizeof(t) - 1), t))

/*
 * GUID Comparison
 */

#ifndef __IID_ALIGNED__
    #define __IID_ALIGNED__

    #define IsEqualGUIDAligned(guid1, guid2) \
        ( (*(PLONGLONG)(guid1) == *(PLONGLONG)(guid2)) && \
            (*((PLONGLONG)(guid1) + 1) == *((PLONGLONG)(guid2) + 1)) )
#endif

/*
** Forward declarations
*/

struct _IRP;
struct _MDL;
struct _KAPC;
struct _KDPC;
struct _KPCR;
struct _KPRCB;
struct _KTSS;
struct _FILE_OBJECT;
struct _DMA_ADAPTER;
struct _DEVICE_OBJECT;
struct _DRIVER_OBJECT;
struct _IO_STATUS_BLOCK;
struct _DEVICE_DESCRIPTION;
struct _SCATTER_GATHER_LIST;
struct _DRIVE_LAYOUT_INFORMATION;
struct _DRIVE_LAYOUT_INFORMATION_EX;
struct _LOADER_PARAMETER_BLOCK;

#ifndef _SECURITY_ATTRIBUTES_
#define _SECURITY_ATTRIBUTES_
typedef PVOID PSECURITY_DESCRIPTOR;
#endif
typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;
typedef PVOID PSID;

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

/*
** Routines specific to this DDK
*/
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )

/*
** Simple structures
*/

typedef LONG KPRIORITY;
typedef UCHAR KIRQL, *PKIRQL;
typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;
typedef UCHAR KPROCESSOR_MODE;

typedef enum _MODE {
  KernelMode,
  UserMode,
  MaximumMode
} MODE;


/* Structures not exposed to drivers */
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _HAL_DISPATCH_TABLE *PHAL_DISPATCH_TABLE;
typedef struct _HAL_PRIVATE_DISPATCH_TABLE *PHAL_PRIVATE_DISPATCH_TABLE;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;
typedef struct _BUS_HANDLER *PBUS_HANDLER;

typedef struct _ADAPTER_OBJECT *PADAPTER_OBJECT; 
typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _ETHREAD *PETHREAD;
typedef struct _EPROCESS *PEPROCESS;
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _KPROCESS *PKPROCESS;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;

//
// Forwarder
//
struct _COMPRESSED_DATA_INFO;

/* Constants */
#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )
#define ZwCurrentThread() NtCurrentThread()

#if (_M_IX86)
#define KIP0PCRADDRESS                      0xffdff000
#endif

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

#define MAXIMUM_PROCESSORS                32

#define MAXIMUM_WAIT_OBJECTS              64

#define EX_RUNDOWN_ACTIVE                 0x1
#define EX_RUNDOWN_COUNT_SHIFT            0x1
#define EX_RUNDOWN_COUNT_INC              (1 << EX_RUNDOWN_COUNT_SHIFT)

#define METHOD_BUFFERED                   0
#define METHOD_IN_DIRECT                  1
#define METHOD_OUT_DIRECT                 2
#define METHOD_NEITHER                    3

#define LOW_PRIORITY                      0
#define LOW_REALTIME_PRIORITY             16
#define HIGH_PRIORITY                     31
#define MAXIMUM_PRIORITY                  32

#define MAXIMUM_SUSPEND_COUNT             MAXCHAR

#define MAXIMUM_FILENAME_LENGTH           256

#define FILE_SUPERSEDED                   0x00000000
#define FILE_OPENED                       0x00000001
#define FILE_CREATED                      0x00000002
#define FILE_OVERWRITTEN                  0x00000003
#define FILE_EXISTS                       0x00000004
#define FILE_DOES_NOT_EXIST               0x00000005

#define FILE_USE_FILE_POINTER_POSITION    0xfffffffe
#define FILE_WRITE_TO_END_OF_FILE         0xffffffff

/* also in winnt.h */
#define FILE_LIST_DIRECTORY               0x00000001
#define FILE_READ_DATA                    0x00000001
#define FILE_ADD_FILE                     0x00000002
#define FILE_WRITE_DATA                   0x00000002
#define FILE_ADD_SUBDIRECTORY             0x00000004
#define FILE_APPEND_DATA                  0x00000004
#define FILE_CREATE_PIPE_INSTANCE         0x00000004
#define FILE_READ_EA                      0x00000008
#define FILE_WRITE_EA                     0x00000010
#define FILE_EXECUTE                      0x00000020
#define FILE_TRAVERSE                     0x00000020
#define FILE_DELETE_CHILD                 0x00000040
#define FILE_READ_ATTRIBUTES              0x00000080
#define FILE_WRITE_ATTRIBUTES             0x00000100

#define FILE_SHARE_READ                   0x00000001
#define FILE_SHARE_WRITE                  0x00000002
#define FILE_SHARE_DELETE                 0x00000004
#define FILE_SHARE_VALID_FLAGS            0x00000007

#define FILE_ATTRIBUTE_READONLY           0x00000001
#define FILE_ATTRIBUTE_HIDDEN             0x00000002
#define FILE_ATTRIBUTE_SYSTEM             0x00000004
#define FILE_ATTRIBUTE_DIRECTORY          0x00000010
#define FILE_ATTRIBUTE_ARCHIVE            0x00000020
#define FILE_ATTRIBUTE_DEVICE             0x00000040
#define FILE_ATTRIBUTE_NORMAL             0x00000080
#define FILE_ATTRIBUTE_TEMPORARY          0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE        0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT      0x00000400
#define FILE_ATTRIBUTE_COMPRESSED         0x00000800
#define FILE_ATTRIBUTE_OFFLINE            0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED          0x00004000

#define FILE_ATTRIBUTE_VALID_FLAGS        0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS    0x000031a7

#define FILE_COPY_STRUCTURED_STORAGE      0x00000041
#define FILE_STRUCTURED_STORAGE           0x00000441

#define FILE_VALID_OPTION_FLAGS           0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS      0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS  0x00000032
#define FILE_VALID_SET_FLAGS              0x00000036

#define FILE_SUPERSEDE                    0x00000000
#define FILE_OPEN                         0x00000001
#define FILE_CREATE                       0x00000002
#define FILE_OPEN_IF                      0x00000003
#define FILE_OVERWRITE                    0x00000004
#define FILE_OVERWRITE_IF                 0x00000005
#define FILE_MAXIMUM_DISPOSITION          0x00000005

#define FILE_DIRECTORY_FILE               0x00000001
#define FILE_WRITE_THROUGH                0x00000002
#define FILE_SEQUENTIAL_ONLY              0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT         0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT      0x00000020
#define FILE_NON_DIRECTORY_FILE           0x00000040
#define FILE_CREATE_TREE_CONNECTION       0x00000080
#define FILE_COMPLETE_IF_OPLOCKED         0x00000100
#define FILE_NO_EA_KNOWLEDGE              0x00000200
#define FILE_OPEN_FOR_RECOVERY            0x00000400
#define FILE_RANDOM_ACCESS                0x00000800
#define FILE_DELETE_ON_CLOSE              0x00001000
#define FILE_OPEN_BY_FILE_ID              0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT       0x00004000
#define FILE_NO_COMPRESSION               0x00008000
#define FILE_RESERVE_OPFILTER             0x00100000
#define FILE_OPEN_REPARSE_POINT           0x00200000
#define FILE_OPEN_NO_RECALL               0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY    0x00800000

#define FILE_ANY_ACCESS                   0x00000000
#define FILE_SPECIAL_ACCESS               FILE_ANY_ACCESS
#define FILE_READ_ACCESS                  0x00000001
#define FILE_WRITE_ACCESS                 0x00000002

#define FILE_ALL_ACCESS \
  (STANDARD_RIGHTS_REQUIRED | \
   SYNCHRONIZE | \
   0x1FF)

#define FILE_GENERIC_EXECUTE \
  (STANDARD_RIGHTS_EXECUTE | \
   FILE_READ_ATTRIBUTES | \
   FILE_EXECUTE | \
   SYNCHRONIZE)

#define FILE_GENERIC_READ \
  (STANDARD_RIGHTS_READ | \
   FILE_READ_DATA | \
   FILE_READ_ATTRIBUTES | \
   FILE_READ_EA | \
   SYNCHRONIZE)

#define FILE_GENERIC_WRITE \
  (STANDARD_RIGHTS_WRITE | \
   FILE_WRITE_DATA | \
   FILE_WRITE_ATTRIBUTES | \
   FILE_WRITE_EA | \
   FILE_APPEND_DATA | \
   SYNCHRONIZE)
/* end winnt.h */

#define OBJ_NAME_PATH_SEPARATOR     ((WCHAR)L'\\')

#define OBJECT_TYPE_CREATE (0x0001)
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

#define DIRECTORY_QUERY (0x0001)
#define DIRECTORY_TRAVERSE (0x0002)
#define DIRECTORY_CREATE_OBJECT (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define EVENT_QUERY_STATE (0x0001)
#define EVENT_MODIFY_STATE (0x0002)
#define EVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3)

#define SEMAPHORE_QUERY_STATE (0x0001)
#define SEMAPHORE_MODIFY_STATE (0x0002)
#define SEMAPHORE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3)

#define THREAD_ALERT (0x0004)

#define FM_LOCK_BIT             (0x1)
#define FM_LOCK_BIT_V           (0x0)
#define FM_LOCK_WAITER_WOKEN    (0x2)
#define FM_LOCK_WAITER_INC      (0x4)

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

#define PROCESSOR_FEATURE_MAX 64
#define MAX_WOW64_SHARED_ENTRIES 16

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign,
    NEC98x86,
    EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

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
    union {
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
    union
    {
        ULONG SharedDataFlags;
        struct
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

#define IRP_MJ_CREATE                     0x00
#define IRP_MJ_CREATE_NAMED_PIPE          0x01
#define IRP_MJ_CLOSE                      0x02
#define IRP_MJ_READ                       0x03
#define IRP_MJ_WRITE                      0x04
#define IRP_MJ_QUERY_INFORMATION          0x05
#define IRP_MJ_SET_INFORMATION            0x06
#define IRP_MJ_QUERY_EA                   0x07
#define IRP_MJ_SET_EA                     0x08
#define IRP_MJ_FLUSH_BUFFERS              0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION   0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION     0x0b
#define IRP_MJ_DIRECTORY_CONTROL          0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL        0x0d
#define IRP_MJ_DEVICE_CONTROL             0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL    0x0f
#define IRP_MJ_SCSI                       0x0f
#define IRP_MJ_SHUTDOWN                   0x10
#define IRP_MJ_LOCK_CONTROL               0x11
#define IRP_MJ_CLEANUP                    0x12
#define IRP_MJ_CREATE_MAILSLOT            0x13
#define IRP_MJ_QUERY_SECURITY             0x14
#define IRP_MJ_SET_SECURITY               0x15
#define IRP_MJ_POWER                      0x16
#define IRP_MJ_SYSTEM_CONTROL             0x17
#define IRP_MJ_DEVICE_CHANGE              0x18
#define IRP_MJ_QUERY_QUOTA                0x19
#define IRP_MJ_SET_QUOTA                  0x1a
#define IRP_MJ_PNP                        0x1b
#define IRP_MJ_PNP_POWER                  0x1b
#define IRP_MJ_MAXIMUM_FUNCTION           0x1b

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

#define IRP_MN_SCSI_CLASS                 0x01

#define IRP_MN_START_DEVICE               0x00
#define IRP_MN_QUERY_REMOVE_DEVICE        0x01
#define IRP_MN_REMOVE_DEVICE              0x02
#define IRP_MN_CANCEL_REMOVE_DEVICE       0x03
#define IRP_MN_STOP_DEVICE                0x04
#define IRP_MN_QUERY_STOP_DEVICE          0x05
#define IRP_MN_CANCEL_STOP_DEVICE         0x06

#define IRP_MN_QUERY_DEVICE_RELATIONS       0x07
#define IRP_MN_QUERY_INTERFACE              0x08
#define IRP_MN_QUERY_CAPABILITIES           0x09
#define IRP_MN_QUERY_RESOURCES              0x0A
#define IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
#define IRP_MN_QUERY_DEVICE_TEXT            0x0C
#define IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D

#define IRP_MN_READ_CONFIG                  0x0F
#define IRP_MN_WRITE_CONFIG                 0x10
#define IRP_MN_EJECT                        0x11
#define IRP_MN_SET_LOCK                     0x12
#define IRP_MN_QUERY_ID                     0x13
#define IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
#define IRP_MN_QUERY_BUS_INFORMATION        0x15
#define IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
#define IRP_MN_SURPRISE_REMOVAL             0x17
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

#define IRP_MN_WAIT_WAKE                  0x00
#define IRP_MN_POWER_SEQUENCE             0x01
#define IRP_MN_SET_POWER                  0x02
#define IRP_MN_QUERY_POWER                0x03

#define IRP_MN_QUERY_ALL_DATA             0x00
#define IRP_MN_QUERY_SINGLE_INSTANCE      0x01
#define IRP_MN_CHANGE_SINGLE_INSTANCE     0x02
#define IRP_MN_CHANGE_SINGLE_ITEM         0x03
#define IRP_MN_ENABLE_EVENTS              0x04
#define IRP_MN_DISABLE_EVENTS             0x05
#define IRP_MN_ENABLE_COLLECTION          0x06
#define IRP_MN_DISABLE_COLLECTION         0x07
#define IRP_MN_REGINFO                    0x08
#define IRP_MN_EXECUTE_METHOD             0x09

#define IRP_MN_REGINFO_EX                 0x0b

typedef enum _IO_PAGING_PRIORITY
{
  IoPagingPriorityInvalid,
  IoPagingPriorityNormal,
  IoPagingPriorityHigh,
  IoPagingPriorityReserved1,
  IoPagingPriorityReserved2
} IO_PAGING_PRIORITY;

typedef enum _IO_ALLOCATION_ACTION {
  KeepObject = 1,
  DeallocateObject,
  DeallocateObjectKeepRegisters
} IO_ALLOCATION_ACTION, *PIO_ALLOCATION_ACTION;

typedef IO_ALLOCATION_ACTION
(DDKAPI *PDRIVER_CONTROL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN PVOID  MapRegisterBase,
  IN PVOID  Context);


typedef EXCEPTION_DISPOSITION
(DDKAPI *PEXCEPTION_ROUTINE)(
  IN struct _EXCEPTION_RECORD *ExceptionRecord,
  IN PVOID EstablisherFrame,
  IN OUT struct _CONTEXT *ContextRecord,
  IN OUT PVOID DispatcherContext);

typedef VOID
(DDKAPI *PDRIVER_LIST_CONTROL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN struct _SCATTER_GATHER_LIST  *ScatterGather,
  IN PVOID  Context);

typedef NTSTATUS
(DDKAPI DRIVER_ADD_DEVICE)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN struct _DEVICE_OBJECT  *PhysicalDeviceObject);
typedef DRIVER_ADD_DEVICE *PDRIVER_ADD_DEVICE;

typedef NTSTATUS
(DDKAPI IO_COMPLETION_ROUTINE)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN PVOID  Context);
typedef IO_COMPLETION_ROUTINE *PIO_COMPLETION_ROUTINE;

typedef VOID
(DDKAPI DRIVER_CANCEL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);
typedef DRIVER_CANCEL *PDRIVER_CANCEL;

typedef VOID
(DDKAPI *PKDEFERRED_ROUTINE)(
  IN struct _KDPC  *Dpc,
  IN PVOID  DeferredContext,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

typedef NTSTATUS
(DDKAPI DRIVER_DISPATCH)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);
typedef DRIVER_DISPATCH *PDRIVER_DISPATCH;

typedef VOID
(DDKAPI *PIO_DPC_ROUTINE)(
  IN struct _KDPC  *Dpc,
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN PVOID  Context);

typedef NTSTATUS
(DDKAPI *PMM_DLL_INITIALIZE)(
  IN PUNICODE_STRING  RegistryPath);

typedef NTSTATUS
(DDKAPI *PMM_DLL_UNLOAD)(
  VOID);

typedef NTSTATUS
(DDKAPI *PDRIVER_ENTRY)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN PUNICODE_STRING  RegistryPath);

typedef NTSTATUS
(DDKAPI DRIVER_INITIALIZE)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN PUNICODE_STRING  RegistryPath);
typedef DRIVER_INITIALIZE *PDRIVER_INITIALIZE;

typedef BOOLEAN
(DDKAPI KSERVICE_ROUTINE)(
  IN struct _KINTERRUPT  *Interrupt,
  IN PVOID  ServiceContext);
typedef KSERVICE_ROUTINE *PKSERVICE_ROUTINE;

typedef VOID
(DDKAPI *PIO_TIMER_ROUTINE)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN PVOID  Context);

typedef VOID
(DDKAPI *PDRIVER_REINITIALIZE)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN PVOID  Context,
  IN ULONG  Count);

typedef VOID
(DDKAPI DRIVER_STARTIO)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);
typedef DRIVER_STARTIO *PDRIVER_STARTIO;

typedef BOOLEAN
(DDKAPI *PKSYNCHRONIZE_ROUTINE)(
  IN PVOID  SynchronizeContext);

typedef VOID
(DDKAPI DRIVER_UNLOAD)(
  IN struct _DRIVER_OBJECT  *DriverObject);
typedef DRIVER_UNLOAD *PDRIVER_UNLOAD;



/*
** Plug and Play structures
*/

typedef VOID
(DDKAPI *PINTERFACE_REFERENCE)(
  PVOID  Context);

typedef VOID
(DDKAPI *PINTERFACE_DEREFERENCE)(
  PVOID Context);

typedef BOOLEAN
(DDKAPI *PTRANSLATE_BUS_ADDRESS)(
  IN PVOID  Context,
  IN PHYSICAL_ADDRESS  BusAddress,
  IN ULONG  Length,
  IN OUT PULONG  AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);

typedef struct _DMA_ADAPTER*
(DDKAPI *PGET_DMA_ADAPTER)(
  IN PVOID  Context,
  IN struct _DEVICE_DESCRIPTION  *DeviceDescriptor,
  OUT PULONG  NumberOfMapRegisters);

typedef ULONG
(DDKAPI *PGET_SET_DEVICE_DATA)(
  IN PVOID  Context,
  IN ULONG  DataType,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

/* PCI_DEVICE_PRESENCE_PARAMETERS.Flags */
#define PCI_USE_SUBSYSTEM_IDS   0x00000001
#define PCI_USE_REVISION        0x00000002
#define PCI_USE_VENDEV_IDS      0x00000004
#define PCI_USE_CLASS_SUBCLASS  0x00000008
#define PCI_USE_PROGIF          0x00000010
#define PCI_USE_LOCAL_BUS       0x00000020
#define PCI_USE_LOCAL_DEVICE    0x00000040

typedef struct _PCI_DEVICE_PRESENCE_PARAMETERS {
  ULONG   Size;
  ULONG   Flags;
  USHORT  VendorID;
  USHORT  DeviceID;
  UCHAR   RevisionID;
  USHORT  SubVendorID;
  USHORT  SubSystemID;
  UCHAR   BaseClass;
  UCHAR   SubClass;
  UCHAR   ProgIf;
} PCI_DEVICE_PRESENCE_PARAMETERS, *PPCI_DEVICE_PRESENCE_PARAMETERS;

typedef BOOLEAN
(DDKAPI *PPCI_IS_DEVICE_PRESENT)(
  IN USHORT  VendorID,
  IN USHORT  DeviceID,
  IN UCHAR   RevisionID,
  IN USHORT  SubVendorID,
  IN USHORT  SubSystemID,
  IN ULONG   Flags);

typedef BOOLEAN
(DDKAPI *PPCI_IS_DEVICE_PRESENT_EX)(
  IN PVOID Context,
  IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters);

typedef union _POWER_STATE {
  SYSTEM_POWER_STATE  SystemState;
  DEVICE_POWER_STATE  DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
  SystemPowerState,
  DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

typedef struct _BUS_INTERFACE_STANDARD {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PTRANSLATE_BUS_ADDRESS  TranslateBusAddress;
  PGET_DMA_ADAPTER  GetDmaAdapter;
  PGET_SET_DEVICE_DATA  SetBusData;
  PGET_SET_DEVICE_DATA  GetBusData;
} BUS_INTERFACE_STANDARD, *PBUS_INTERFACE_STANDARD;

typedef struct _PCI_DEVICE_PRESENT_INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PPCI_IS_DEVICE_PRESENT  IsDevicePresent;
  PPCI_IS_DEVICE_PRESENT_EX  IsDevicePresentEx;
} PCI_DEVICE_PRESENT_INTERFACE, *PPCI_DEVICE_PRESENT_INTERFACE;

typedef struct _DEVICE_CAPABILITIES {
  USHORT  Size;
  USHORT  Version;
  ULONG  DeviceD1 : 1;
  ULONG  DeviceD2 : 1;
  ULONG  LockSupported : 1;
  ULONG  EjectSupported : 1;
  ULONG  Removable : 1;
  ULONG  DockDevice : 1;
  ULONG  UniqueID : 1;
  ULONG  SilentInstall : 1;
  ULONG  RawDeviceOK : 1;
  ULONG  SurpriseRemovalOK : 1;
  ULONG  WakeFromD0 : 1;
  ULONG  WakeFromD1 : 1;
  ULONG  WakeFromD2 : 1;
  ULONG  WakeFromD3 : 1;
  ULONG  HardwareDisabled : 1;
  ULONG  NonDynamic : 1;
  ULONG  WarmEjectSupported : 1;
  ULONG  NoDisplayInUI : 1;
  ULONG  Reserved : 14;
  ULONG  Address;
  ULONG  UINumber;
  DEVICE_POWER_STATE  DeviceState[PowerSystemMaximum];
  SYSTEM_POWER_STATE  SystemWake;
  DEVICE_POWER_STATE  DeviceWake;
  ULONG  D1Latency;
  ULONG  D2Latency;
  ULONG  D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _DEVICE_INTERFACE_CHANGE_NOTIFICATION {
  USHORT  Version;
  USHORT  Size;
  GUID  Event;
  GUID  InterfaceClassGuid;
  PUNICODE_STRING  SymbolicLinkName;
} DEVICE_INTERFACE_CHANGE_NOTIFICATION, *PDEVICE_INTERFACE_CHANGE_NOTIFICATION;

typedef struct _HWPROFILE_CHANGE_NOTIFICATION {
  USHORT  Version;
  USHORT  Size;
  GUID  Event;
} HWPROFILE_CHANGE_NOTIFICATION, *PHWPROFILE_CHANGE_NOTIFICATION;

#undef INTERFACE

typedef struct _INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
} INTERFACE, *PINTERFACE;

typedef struct _PLUGPLAY_NOTIFICATION_HEADER {
  USHORT  Version;
  USHORT  Size;
  GUID  Event;
} PLUGPLAY_NOTIFICATION_HEADER, *PPLUGPLAY_NOTIFICATION_HEADER;

typedef ULONG PNP_DEVICE_STATE, *PPNP_DEVICE_STATE;

/* PNP_DEVICE_STATE */

#define PNP_DEVICE_DISABLED                      0x00000001
#define PNP_DEVICE_DONT_DISPLAY_IN_UI            0x00000002
#define PNP_DEVICE_FAILED                        0x00000004
#define PNP_DEVICE_REMOVED                       0x00000008
#define PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED 0x00000010
#define PNP_DEVICE_NOT_DISABLEABLE               0x00000020

typedef struct _TARGET_DEVICE_CUSTOM_NOTIFICATION {
  USHORT  Version;
  USHORT  Size;
  GUID  Event;
  struct _FILE_OBJECT  *FileObject;
  LONG  NameBufferOffset;
  UCHAR  CustomDataBuffer[1];
} TARGET_DEVICE_CUSTOM_NOTIFICATION, *PTARGET_DEVICE_CUSTOM_NOTIFICATION;

typedef struct _TARGET_DEVICE_REMOVAL_NOTIFICATION {
  USHORT  Version;
  USHORT  Size;
  GUID  Event;
  struct _FILE_OBJECT  *FileObject;
} TARGET_DEVICE_REMOVAL_NOTIFICATION, *PTARGET_DEVICE_REMOVAL_NOTIFICATION;

typedef enum _BUS_QUERY_ID_TYPE {
  BusQueryDeviceID,
  BusQueryHardwareIDs,
  BusQueryCompatibleIDs,
  BusQueryInstanceID,
  BusQueryDeviceSerialNumber
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

typedef enum _DEVICE_TEXT_TYPE {
  DeviceTextDescription,
  DeviceTextLocationInformation
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE {
  DeviceUsageTypeUndefined,
  DeviceUsageTypePaging,
  DeviceUsageTypeHibernation,
  DeviceUsageTypeDumpFile
} DEVICE_USAGE_NOTIFICATION_TYPE;

typedef struct _POWER_SEQUENCE {
  ULONG  SequenceD1;
  ULONG  SequenceD2;
  ULONG  SequenceD3;
} POWER_SEQUENCE, *PPOWER_SEQUENCE;

typedef enum {
  DevicePropertyDeviceDescription,
  DevicePropertyHardwareID,
  DevicePropertyCompatibleIDs,
  DevicePropertyBootConfiguration,
  DevicePropertyBootConfigurationTranslated,
  DevicePropertyClassName,
  DevicePropertyClassGuid,
  DevicePropertyDriverKeyName,
  DevicePropertyManufacturer,
  DevicePropertyFriendlyName,
  DevicePropertyLocationInformation,
  DevicePropertyPhysicalDeviceObjectName,
  DevicePropertyBusTypeGuid,
  DevicePropertyLegacyBusType,
  DevicePropertyBusNumber,
  DevicePropertyEnumeratorName,
  DevicePropertyAddress,
  DevicePropertyUINumber,
  DevicePropertyInstallState,
  DevicePropertyRemovalPolicy
} DEVICE_REGISTRY_PROPERTY;

typedef enum _IO_NOTIFICATION_EVENT_CATEGORY {
  EventCategoryReserved,
  EventCategoryHardwareProfileChange,
  EventCategoryDeviceInterfaceChange,
  EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

#define PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES    0x00000001

typedef NTSTATUS
(DDKAPI *PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)(
  IN PVOID NotificationStructure,
  IN PVOID Context);

typedef VOID
(DDKAPI *PDEVICE_CHANGE_COMPLETE_CALLBACK)(
  IN PVOID Context);


/*
** System structures
*/

#define SYMBOLIC_LINK_QUERY               0x0001
#define SYMBOLIC_LINK_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED | 0x1)

/* also in winnt,h */
#define DUPLICATE_CLOSE_SOURCE            0x00000001
#define DUPLICATE_SAME_ACCESS             0x00000002
#define DUPLICATE_SAME_ATTRIBUTES         0x00000004
/* end winnt.h */

typedef struct _OBJECT_NAME_INFORMATION {
  UNICODE_STRING  Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
  _ANONYMOUS_UNION union {
    NTSTATUS  Status;
    PVOID  Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR  Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID
(DDKAPI *PIO_APC_ROUTINE)(
  IN PVOID ApcContext,
  IN PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG Reserved);

typedef VOID
(DDKAPI *PKNORMAL_ROUTINE)(
  IN PVOID  NormalContext,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

typedef VOID
(DDKAPI *PKKERNEL_ROUTINE)(
  IN struct _KAPC  *Apc,
  IN OUT PKNORMAL_ROUTINE  *NormalRoutine,
  IN OUT PVOID  *NormalContext,
  IN OUT PVOID  *SystemArgument1,
  IN OUT PVOID  *SystemArgument2);

typedef VOID
(DDKAPI *PKRUNDOWN_ROUTINE)(
  IN struct _KAPC  *Apc);

typedef BOOLEAN
(DDKAPI *PKTRANSFER_ROUTINE)(
  VOID);

typedef struct _KAPC
{
    UCHAR Type;
    UCHAR SpareByte0;
    UCHAR Size;
    UCHAR SpareByte1;
    ULONG SpareLong0;
    struct _KTHREAD *Thread;
    LIST_ENTRY ApcListEntry;
    PKKERNEL_ROUTINE KernelRoutine;
    PKRUNDOWN_ROUTINE RundownRoutine;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    CCHAR ApcStateIndex;
    KPROCESSOR_MODE ApcMode;
    BOOLEAN Inserted;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

typedef struct _KDEVICE_QUEUE {
  CSHORT  Type;
  CSHORT  Size;
  LIST_ENTRY  DeviceListHead;
  KSPIN_LOCK  Lock;
  BOOLEAN  Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE, *RESTRICTED_POINTER PRKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY {
  LIST_ENTRY  DeviceListEntry;
  ULONG  SortKey;
  BOOLEAN  Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY,
*RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

#define LOCK_QUEUE_WAIT                   1
#define LOCK_QUEUE_OWNER                  2
#define LOCK_QUEUE_TIMER_LOCK_SHIFT       4
#define LOCK_QUEUE_TIMER_TABLE_LOCKS (1 << (8 - LOCK_QUEUE_TIMER_LOCK_SHIFT))

typedef enum _KSPIN_LOCK_QUEUE_NUMBER
{
    LockQueueDispatcherLock,
    LockQueueExpansionLock,
    LockQueuePfnLock,
    LockQueueSystemSpaceLock,
    LockQueueVacbLock,
    LockQueueMasterLock,
    LockQueueNonPagedPoolLock,
    LockQueueIoCancelLock,
    LockQueueWorkQueueLock,
    LockQueueIoVpbLock,
    LockQueueIoDatabaseLock,
    LockQueueIoCompletionLock,
    LockQueueNtfsStructLock,
    LockQueueAfdWorkQueueLock,
    LockQueueBcbLock,
    LockQueueMmNonPagedPoolLock,
    LockQueueUnusedSpare16,
    LockQueueTimerTableLock,
    LockQueueMaximumLock = LockQueueTimerTableLock + LOCK_QUEUE_TIMER_TABLE_LOCKS
} KSPIN_LOCK_QUEUE_NUMBER, *PKSPIN_LOCK_QUEUE_NUMBER;

typedef struct _KSPIN_LOCK_QUEUE {
  struct _KSPIN_LOCK_QUEUE  *VOLATILE Next;
  PKSPIN_LOCK VOLATILE  Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
  KSPIN_LOCK_QUEUE  LockQueue;
  KIRQL  OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

#define DPC_NORMAL 0
#define DPC_THREADED 1

#define ASSERT_APC(Object) \
    ASSERT((Object)->Type == ApcObject)

#define ASSERT_DPC(Object) \
    ASSERT(((Object)->Type == 0) || \
           ((Object)->Type == DpcObject) || \
           ((Object)->Type == ThreadedDpcObject))

#define ASSERT_DEVICE_QUEUE(Object) \
    ASSERT((Object)->Type == DeviceQueueObject)

typedef struct _KDPC
{
    UCHAR Type;
    UCHAR Importance;
    USHORT Number;
    LIST_ENTRY DpcListEntry;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    volatile PVOID  DpcData;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;

typedef PVOID PKIPI_CONTEXT;

typedef
VOID
(NTAPI *PKIPI_WORKER)(
    IN PKIPI_CONTEXT PacketContext,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
);

typedef struct _WAIT_CONTEXT_BLOCK {
  KDEVICE_QUEUE_ENTRY  WaitQueueEntry;
  PDRIVER_CONTROL  DeviceRoutine;
  PVOID  DeviceContext;
  ULONG  NumberOfMapRegisters;
  PVOID  DeviceObject;
  PVOID  CurrentIrp;
  PKDPC  BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

typedef struct _DISPATCHER_HEADER
{
    union
    {
        struct
        {
            UCHAR Type;
            union
            {
                UCHAR Absolute;
                UCHAR NpxIrql;
            };
            union
            {
                UCHAR Size;
                UCHAR Hand;
            };
            union
            {
                UCHAR Inserted;
                BOOLEAN DebugActive;
            };
        };
        volatile LONG Lock;
    };
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef struct _KEVENT {
  DISPATCHER_HEADER  Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

typedef struct _FAST_MUTEX
{
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Gate;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef struct _EX_RUNDOWN_REF
{
    union
    {
        volatile ULONG_PTR Count;
        volatile PVOID Ptr;
    };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

#define ASSERT_GATE(object) \
    ASSERT((((object)->Header.Type & KOBJECT_TYPE_MASK) == GateObject) || \
          (((object)->Header.Type & KOBJECT_TYPE_MASK) == EventSynchronizationObject))

typedef struct _KGATE
{
    DISPATCHER_HEADER Header;
} KGATE, *PKGATE, *RESTRICTED_POINTER PRKGATE;

#define GM_LOCK_BIT          0x1
#define GM_LOCK_BIT_V        0x0
#define GM_LOCK_WAITER_WOKEN 0x2
#define GM_LOCK_WAITER_INC   0x4

typedef struct _KGUARDED_MUTEX
{
    volatile LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KGATE Gate;
    union
    {
        struct
        {
            SHORT KernelApcDisable;
            SHORT SpecialApcDisable;
        };
        ULONG CombinedApcDisable;
    };
} KGUARDED_MUTEX, *PKGUARDED_MUTEX;

#define TIMER_TABLE_SIZE 512
#define TIMER_TABLE_SHIFT 9

typedef struct _KTIMER {
  DISPATCHER_HEADER  Header;
  ULARGE_INTEGER  DueTime;
  LIST_ENTRY  TimerListEntry;
  struct _KDPC  *Dpc;
  LONG  Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;

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

typedef struct _KMUTANT {
  DISPATCHER_HEADER  Header;
  LIST_ENTRY  MutantListEntry;
  struct _KTHREAD  *RESTRICTED_POINTER OwnerThread;
  BOOLEAN  Abandoned;
  UCHAR  ApcDisable;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT, KMUTEX, *PKMUTEX, *RESTRICTED_POINTER PRKMUTEX;

typedef enum _TIMER_TYPE {
  NotificationTimer,
  SynchronizationTimer
} TIMER_TYPE;

#define EVENT_INCREMENT                   1
#define IO_NO_INCREMENT                   0
#define IO_CD_ROM_INCREMENT               1
#define IO_DISK_INCREMENT                 1
#define IO_KEYBOARD_INCREMENT             6
#define IO_MAILSLOT_INCREMENT             2
#define IO_MOUSE_INCREMENT                6
#define IO_NAMED_PIPE_INCREMENT           2
#define IO_NETWORK_INCREMENT              2
#define IO_PARALLEL_INCREMENT             1
#define IO_SERIAL_INCREMENT               2
#define IO_SOUND_INCREMENT                8
#define IO_VIDEO_INCREMENT                1
#define SEMAPHORE_INCREMENT               1

#define MM_MAXIMUM_DISK_IO_SIZE          (0x10000)

typedef struct _IRP {
  CSHORT  Type;
  USHORT  Size;
  struct _MDL  *MdlAddress;
  ULONG  Flags;
  union {
    struct _IRP  *MasterIrp;
    volatile LONG  IrpCount;
    PVOID  SystemBuffer;
  } AssociatedIrp;
  LIST_ENTRY  ThreadListEntry;
  IO_STATUS_BLOCK  IoStatus;
  KPROCESSOR_MODE  RequestorMode;
  BOOLEAN  PendingReturned;
  CHAR  StackCount;
  CHAR  CurrentLocation;
  BOOLEAN  Cancel;
  KIRQL  CancelIrql;
  CCHAR  ApcEnvironment;
  UCHAR  AllocationFlags;
  PIO_STATUS_BLOCK  UserIosb;
  PKEVENT  UserEvent;
  union {
    struct {
      PIO_APC_ROUTINE  UserApcRoutine;
      PVOID  UserApcContext;
    } AsynchronousParameters;
    LARGE_INTEGER  AllocationSize;
  } Overlay;
  volatile PDRIVER_CANCEL  CancelRoutine;
  PVOID  UserBuffer;
  union {
    struct {
      _ANONYMOUS_UNION union {
        KDEVICE_QUEUE_ENTRY  DeviceQueueEntry;
        _ANONYMOUS_STRUCT struct {
          PVOID  DriverContext[4];
        } DUMMYSTRUCTNAME;
      } DUMMYUNIONNAME;
      PETHREAD  Thread;
      PCHAR  AuxiliaryBuffer;
      _ANONYMOUS_STRUCT struct {
        LIST_ENTRY  ListEntry;
        _ANONYMOUS_UNION union {
          struct _IO_STACK_LOCATION  *CurrentStackLocation;
          ULONG  PacketType;
        } DUMMYUNIONNAME;
      } DUMMYSTRUCTNAME;
      struct _FILE_OBJECT  *OriginalFileObject;
    } Overlay;
    KAPC  Apc;
    PVOID  CompletionKey;
  } Tail;
} IRP;
typedef struct _IRP *PIRP;

/* IRP.Flags */

#define SL_FORCE_ACCESS_CHECK             0x01
#define SL_OPEN_PAGING_FILE               0x02
#define SL_OPEN_TARGET_DIRECTORY          0x04
#define SL_CASE_SENSITIVE                 0x80

#define SL_KEY_SPECIFIED                  0x01
#define SL_OVERRIDE_VERIFY_VOLUME         0x02
#define SL_WRITE_THROUGH                  0x04
#define SL_FT_SEQUENTIAL_WRITE            0x08

#define SL_FAIL_IMMEDIATELY               0x01
#define SL_EXCLUSIVE_LOCK                 0x02

#define SL_RESTART_SCAN                   0x01
#define SL_RETURN_SINGLE_ENTRY            0x02
#define SL_INDEX_SPECIFIED                0x04

#define SL_WATCH_TREE                     0x01

#define SL_ALLOW_RAW_MOUNT                0x01

#define CTL_CODE(DeviceType, Function, Method, Access)( \
  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define DEVICE_TYPE_FROM_CTL_CODE(ctl) (((ULONG) (ctl & 0xffff0000)) >> 16)

enum
{
   IRP_NOCACHE = 0x1,
   IRP_PAGING_IO = 0x2,
   IRP_MOUNT_COMPLETION = 0x2,
   IRP_SYNCHRONOUS_API = 0x4,
   IRP_ASSOCIATED_IRP = 0x8,
   IRP_BUFFERED_IO = 0x10,
   IRP_DEALLOCATE_BUFFER = 0x20,
   IRP_INPUT_OPERATION = 0x40,
   IRP_SYNCHRONOUS_PAGING_IO = 0x40,
   IRP_CREATE_OPERATION = 0x80,
   IRP_READ_OPERATION = 0x100,
   IRP_WRITE_OPERATION = 0x200,
   IRP_CLOSE_OPERATION = 0x400,
   IRP_DEFER_IO_COMPLETION = 0x800,
   IRP_OB_QUERY_NAME = 0x1000,
   IRP_HOLD_DEVICE_QUEUE = 0x2000,
   IRP_RETRY_IO_COMPLETION = 0x4000
};

#define IRP_QUOTA_CHARGED                 0x01
#define IRP_ALLOCATED_MUST_SUCCEED        0x02
#define IRP_ALLOCATED_FIXED_SIZE          0x04
#define IRP_LOOKASIDE_ALLOCATION          0x08

typedef struct _BOOTDISK_INFORMATION {
  LONGLONG  BootPartitionOffset;
  LONGLONG  SystemPartitionOffset;
  ULONG  BootDeviceSignature;
  ULONG  SystemDeviceSignature;
} BOOTDISK_INFORMATION, *PBOOTDISK_INFORMATION;

typedef struct _BOOTDISK_INFORMATION_EX {
  LONGLONG  BootPartitionOffset;
  LONGLONG  SystemPartitionOffset;
  ULONG  BootDeviceSignature;
  ULONG  SystemDeviceSignature;
  GUID  BootDeviceGuid;
  GUID  SystemDeviceGuid;
  BOOLEAN  BootDeviceIsGpt;
  BOOLEAN  SystemDeviceIsGpt;
} BOOTDISK_INFORMATION_EX, *PBOOTDISK_INFORMATION_EX;

typedef struct _EISA_MEMORY_TYPE {
  UCHAR  ReadWrite : 1;
  UCHAR  Cached : 1;
  UCHAR  Reserved0 : 1;
  UCHAR  Type : 2;
  UCHAR  Shared : 1;
  UCHAR  Reserved1 : 1;
  UCHAR  MoreEntries : 1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

#include <pshpack1.h>
typedef struct _EISA_MEMORY_CONFIGURATION {
  EISA_MEMORY_TYPE  ConfigurationByte;
  UCHAR  DataSize;
  USHORT  AddressLowWord;
  UCHAR  AddressHighByte;
  USHORT  MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;
#include <poppack.h>

typedef struct _EISA_IRQ_DESCRIPTOR {
  UCHAR  Interrupt : 4;
  UCHAR  Reserved : 1;
  UCHAR  LevelTriggered : 1;
  UCHAR  Shared : 1;
  UCHAR  MoreEntries : 1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION {
  EISA_IRQ_DESCRIPTOR  ConfigurationByte;
  UCHAR  Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;

typedef struct _DMA_CONFIGURATION_BYTE0 {
  UCHAR Channel : 3;
  UCHAR Reserved : 3;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1 {
  UCHAR  Reserved0 : 2;
  UCHAR  TransferSize : 2;
  UCHAR  Timing : 2;
  UCHAR  Reserved1 : 2;
} DMA_CONFIGURATION_BYTE1;

typedef struct _EISA_DMA_CONFIGURATION {
  DMA_CONFIGURATION_BYTE0  ConfigurationByte0;
  DMA_CONFIGURATION_BYTE1  ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;

#include <pshpack1.h>
typedef struct _EISA_PORT_DESCRIPTOR {
  UCHAR  NumberPorts : 5;
  UCHAR  Reserved : 1;
  UCHAR  Shared : 1;
  UCHAR  MoreEntries : 1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION {
  EISA_PORT_DESCRIPTOR  Configuration;
  USHORT  PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;
#include <poppack.h>

typedef struct _CM_EISA_FUNCTION_INFORMATION {
  ULONG  CompressedId;
  UCHAR  IdSlotFlags1;
  UCHAR  IdSlotFlags2;
  UCHAR  MinorRevision;
  UCHAR  MajorRevision;
  UCHAR  Selections[26];
  UCHAR  FunctionFlags;
  UCHAR  TypeString[80];
  EISA_MEMORY_CONFIGURATION  EisaMemory[9];
  EISA_IRQ_CONFIGURATION  EisaIrq[7];
  EISA_DMA_CONFIGURATION  EisaDma[4];
  EISA_PORT_CONFIGURATION  EisaPort[20];
  UCHAR  InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;

/* CM_EISA_FUNCTION_INFORMATION.FunctionFlags */

#define EISA_FUNCTION_ENABLED           0x80
#define EISA_FREE_FORM_DATA             0x40
#define EISA_HAS_PORT_INIT_ENTRY        0x20
#define EISA_HAS_PORT_RANGE             0x10
#define EISA_HAS_DMA_ENTRY              0x08
#define EISA_HAS_IRQ_ENTRY              0x04
#define EISA_HAS_MEMORY_ENTRY           0x02
#define EISA_HAS_TYPE_ENTRY             0x01
#define EISA_HAS_INFORMATION \
  (EISA_HAS_PORT_RANGE + EISA_HAS_DMA_ENTRY + EISA_HAS_IRQ_ENTRY \
  + EISA_HAS_MEMORY_ENTRY + EISA_HAS_TYPE_ENTRY)

typedef struct _CM_EISA_SLOT_INFORMATION {
  UCHAR  ReturnCode;
  UCHAR  ReturnFlags;
  UCHAR  MajorRevision;
  UCHAR  MinorRevision;
  USHORT  Checksum;
  UCHAR  NumberFunctions;
  UCHAR  FunctionInformation;
  ULONG  CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;

/* CM_EISA_SLOT_INFORMATION.ReturnCode */

#define EISA_INVALID_SLOT               0x80
#define EISA_INVALID_FUNCTION           0x81
#define EISA_INVALID_CONFIGURATION      0x82
#define EISA_EMPTY_SLOT                 0x83
#define EISA_INVALID_BIOS_CALL          0x86

typedef struct _CM_FLOPPY_DEVICE_DATA {
  USHORT  Version;
  USHORT  Revision;
  CHAR  Size[8];
  ULONG  MaxDensity;
  ULONG  MountDensity;
  UCHAR  StepRateHeadUnloadTime;
  UCHAR  HeadLoadTime;
  UCHAR  MotorOffTime;
  UCHAR  SectorLengthCode;
  UCHAR  SectorPerTrack;
  UCHAR  ReadWriteGapLength;
  UCHAR  DataTransferLength;
  UCHAR  FormatGapLength;
  UCHAR  FormatFillCharacter;
  UCHAR  HeadSettleTime;
  UCHAR  MotorSettleTime;
  UCHAR  MaximumTrackValue;
  UCHAR  DataTransferRate;
} CM_FLOPPY_DEVICE_DATA, *PCM_FLOPPY_DEVICE_DATA;

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
  PNPBus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef struct _PNP_BUS_INFORMATION {
  GUID  BusTypeGuid;
  INTERFACE_TYPE  LegacyBusType;
  ULONG  BusNumber;
} PNP_BUS_INFORMATION, *PPNP_BUS_INFORMATION;

#include <pshpack1.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
      ULONG Level;
      ULONG Vector;
      KAFFINITY Affinity;
    } Interrupt;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      union {
        struct {
          USHORT Reserved;
          USHORT MessageCount;
          ULONG Vector;
          KAFFINITY Affinity;
        } Raw;
        struct {
          ULONG Level;
          ULONG Vector;
          KAFFINITY Affinity;
        } Translated;
      };
    } MessageInterrupt;
#endif
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length40;
    } Memory40;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length48;
    } Memory48;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length64;
    } Memory64;
#endif
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Type */

#define CmResourceTypeNull                0
#define CmResourceTypePort                1
#define CmResourceTypeInterrupt           2
#define CmResourceTypeMemory              3
#define CmResourceTypeDma                 4
#define CmResourceTypeDeviceSpecific      5
#define CmResourceTypeBusNumber           6
#define CmResourceTypeMaximum             7
#define CmResourceTypeNonArbitrated     128
#define CmResourceTypeConfigData        128
#define CmResourceTypeDevicePrivate     129
#define CmResourceTypePcCardConfig      130
#define CmResourceTypeMfCardConfig      131

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.ShareDisposition */

typedef enum _CM_SHARE_DISPOSITION {
  CmResourceShareUndetermined,
  CmResourceShareDeviceExclusive,
  CmResourceShareDriverExclusive,
  CmResourceShareShared
} CM_SHARE_DISPOSITION;

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypePort */

#define CM_RESOURCE_PORT_MEMORY           0x0000
#define CM_RESOURCE_PORT_IO               0x0001
#define CM_RESOURCE_PORT_10_BIT_DECODE    0x0004
#define CM_RESOURCE_PORT_12_BIT_DECODE    0x0008
#define CM_RESOURCE_PORT_16_BIT_DECODE    0x0010
#define CM_RESOURCE_PORT_POSITIVE_DECODE  0x0020
#define CM_RESOURCE_PORT_PASSIVE_DECODE   0x0040
#define CM_RESOURCE_PORT_WINDOW_DECODE    0x0080

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeInterrupt */

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0x0000
#define CM_RESOURCE_INTERRUPT_LATCHED         0x0001

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeMemory */

#define CM_RESOURCE_MEMORY_READ_WRITE     0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY      0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY     0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE   0x0004
#define CM_RESOURCE_MEMORY_COMBINEDWRITE  0x0008
#define CM_RESOURCE_MEMORY_24             0x0010
#define CM_RESOURCE_MEMORY_CACHEABLE      0x0020

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeDma */

#define CM_RESOURCE_DMA_8                 0x0000
#define CM_RESOURCE_DMA_16                0x0001
#define CM_RESOURCE_DMA_32                0x0002
#define CM_RESOURCE_DMA_8_AND_16          0x0004
#define CM_RESOURCE_DMA_BUS_MASTER        0x0008
#define CM_RESOURCE_DMA_TYPE_A            0x0010
#define CM_RESOURCE_DMA_TYPE_B            0x0020
#define CM_RESOURCE_DMA_TYPE_F            0x0040

typedef struct _CM_PARTIAL_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  CM_PARTIAL_RESOURCE_LIST  PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
  ULONG  Count;
  CM_FULL_RESOURCE_DESCRIPTOR  List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

typedef struct _CM_INT13_DRIVE_PARAMETER {
  USHORT  DriveSelect;
  ULONG  MaxCylinders;
  USHORT  SectorsPerTrack;
  USHORT  MaxHeads;
  USHORT  NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

typedef struct _CM_PNP_BIOS_DEVICE_NODE
{
    USHORT Size;
    UCHAR Node;
    ULONG ProductId;
    UCHAR DeviceType[3];
    USHORT DeviceAttributes;
} CM_PNP_BIOS_DEVICE_NODE,*PCM_PNP_BIOS_DEVICE_NODE;

typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK
{
    UCHAR Signature[4];
    UCHAR Revision;
    UCHAR Length;
    USHORT ControlField;
    UCHAR Checksum;
    ULONG EventFlagAddress;
    USHORT RealModeEntryOffset;
    USHORT RealModeEntrySegment;
    USHORT ProtectedModeEntryOffset;
    ULONG ProtectedModeCodeBaseAddress;
    ULONG OemDeviceId;
    USHORT RealModeDataBaseAddress;
    ULONG ProtectedModeDataBaseAddress;
} CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;

#include <poppack.h>

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
    ULONG BytesPerSector;
    ULONG NumberOfCylinders;
    ULONG SectorsPerTrack;
    ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

typedef struct _CM_KEYBOARD_DEVICE_DATA {
  USHORT  Version;
  USHORT  Revision;
  UCHAR  Type;
  UCHAR  Subtype;
  USHORT  KeyboardFlags;
} CM_KEYBOARD_DEVICE_DATA, *PCM_KEYBOARD_DEVICE_DATA;

#define KEYBOARD_INSERT_ON                0x08
#define KEYBOARD_CAPS_LOCK_ON             0x04
#define KEYBOARD_NUM_LOCK_ON              0x02
#define KEYBOARD_SCROLL_LOCK_ON           0x01
#define KEYBOARD_ALT_KEY_DOWN             0x80
#define KEYBOARD_CTRL_KEY_DOWN            0x40
#define KEYBOARD_LEFT_SHIFT_DOWN          0x20
#define KEYBOARD_RIGHT_SHIFT_DOWN         0x10

typedef struct _CM_MCA_POS_DATA {
  USHORT  AdapterId;
  UCHAR  PosData1;
  UCHAR  PosData2;
  UCHAR  PosData3;
  UCHAR  PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

typedef struct CM_Power_Data_s {
  ULONG  PD_Size;
  DEVICE_POWER_STATE  PD_MostRecentPowerState;
  ULONG  PD_Capabilities;
  ULONG  PD_D1Latency;
  ULONG  PD_D2Latency;
  ULONG  PD_D3Latency;
  DEVICE_POWER_STATE  PD_PowerStateMapping[PowerSystemMaximum];
} CM_POWER_DATA, *PCM_POWER_DATA;

#define PDCAP_D0_SUPPORTED                0x00000001
#define PDCAP_D1_SUPPORTED                0x00000002
#define PDCAP_D2_SUPPORTED                0x00000004
#define PDCAP_D3_SUPPORTED                0x00000008
#define PDCAP_WAKE_FROM_D0_SUPPORTED      0x00000010
#define PDCAP_WAKE_FROM_D1_SUPPORTED      0x00000020
#define PDCAP_WAKE_FROM_D2_SUPPORTED      0x00000040
#define PDCAP_WAKE_FROM_D3_SUPPORTED      0x00000080
#define PDCAP_WARM_EJECT_SUPPORTED        0x00000100

typedef struct _CM_SCSI_DEVICE_DATA {
  USHORT  Version;
  USHORT  Revision;
  UCHAR  HostIdentifier;
} CM_SCSI_DEVICE_DATA, *PCM_SCSI_DEVICE_DATA;

typedef struct _CM_SERIAL_DEVICE_DATA {
  USHORT  Version;
  USHORT  Revision;
  ULONG  BaudClock;
} CM_SERIAL_DEVICE_DATA, *PCM_SERIAL_DEVICE_DATA;

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

typedef enum _KINTERRUPT_POLARITY
{
    InterruptPolarityUnknown,
    InterruptActiveHigh,
    InterruptActiveLow
} KINTERRUPT_POLARITY, *PKINTERRUPT_POLARITY;

/* IO_RESOURCE_DESCRIPTOR.Option */

#define IO_RESOURCE_PREFERRED             0x01
#define IO_RESOURCE_DEFAULT               0x02
#define IO_RESOURCE_ALTERNATIVE           0x08

typedef struct _IO_RESOURCE_DESCRIPTOR {
  UCHAR  Option;
  UCHAR  Type;
  UCHAR  ShareDisposition;
  UCHAR  Spare1;
  USHORT  Flags;
  USHORT  Spare2;
  union {
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Port;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Memory;
    struct {
      ULONG  MinimumVector;
      ULONG  MaximumVector;
    } Interrupt;
    struct {
      ULONG  MinimumChannel;
      ULONG  MaximumChannel;
    } Dma;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Generic;
    struct {
      ULONG  Data[3];
    } DevicePrivate;
    struct {
      ULONG  Length;
      ULONG  MinBusNumber;
      ULONG  MaxBusNumber;
      ULONG  Reserved;
    } BusNumber;
    struct {
      ULONG  Priority;
      ULONG  Reserved1;
      ULONG  Reserved2;
    } ConfigData;
  } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  IO_RESOURCE_DESCRIPTOR  Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
  ULONG  ListSize;
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  ULONG  SlotNumber;
  ULONG  Reserved[3];
  ULONG  AlternativeLists;
  IO_RESOURCE_LIST  List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

typedef struct _IO_ERROR_LOG_PACKET {
  UCHAR  MajorFunctionCode;
  UCHAR  RetryCount;
  USHORT  DumpDataSize;
  USHORT  NumberOfStrings;
  USHORT  StringOffset;
  USHORT  EventCategory;
  NTSTATUS  ErrorCode;
  ULONG  UniqueErrorValue;
  NTSTATUS  FinalStatus;
  ULONG  SequenceNumber;
  ULONG  IoControlCode;
  LARGE_INTEGER  DeviceOffset;
  ULONG  DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

typedef struct _IO_ERROR_LOG_MESSAGE {
  USHORT  Type;
  USHORT  Size;
  USHORT  DriverNameLength;
  LARGE_INTEGER  TimeStamp;
  ULONG  DriverNameOffset;
  IO_ERROR_LOG_PACKET  EntryData;
} IO_ERROR_LOG_MESSAGE, *PIO_ERROR_LOG_MESSAGE;

#define ERROR_LOG_LIMIT_SIZE               240
#define IO_ERROR_LOG_MESSAGE_HEADER_LENGTH (sizeof(IO_ERROR_LOG_MESSAGE) - \
                                            sizeof(IO_ERROR_LOG_PACKET) + \
                                            (sizeof(WCHAR) * 40))
#define ERROR_LOG_MESSAGE_LIMIT_SIZE                                          \
    (ERROR_LOG_LIMIT_SIZE + IO_ERROR_LOG_MESSAGE_HEADER_LENGTH)
#define IO_ERROR_LOG_MESSAGE_LENGTH                                           \
    ((PORT_MAXIMUM_MESSAGE_LENGTH > ERROR_LOG_MESSAGE_LIMIT_SIZE) ?           \
        ERROR_LOG_MESSAGE_LIMIT_SIZE :                                        \
        PORT_MAXIMUM_MESSAGE_LENGTH)
#define ERROR_LOG_MAXIMUM_SIZE (IO_ERROR_LOG_MESSAGE_LENGTH -                 \
                                IO_ERROR_LOG_MESSAGE_HEADER_LENGTH)

typedef struct _CONTROLLER_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PVOID  ControllerExtension;
  KDEVICE_QUEUE  DeviceWaitQueue;
  ULONG  Spare1;
  LARGE_INTEGER  Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

typedef enum _DMA_WIDTH {
  Width8Bits,
  Width16Bits,
  Width32Bits,
  MaximumDmaWidth
} DMA_WIDTH, *PDMA_WIDTH;

typedef enum _DMA_SPEED {
  Compatible,
  TypeA,
  TypeB,
  TypeC,
  TypeF,
  MaximumDmaSpeed
} DMA_SPEED, *PDMA_SPEED;

/* DEVICE_DESCRIPTION.Version */

#define DEVICE_DESCRIPTION_VERSION        0x0000
#define DEVICE_DESCRIPTION_VERSION1       0x0001
#define DEVICE_DESCRIPTION_VERSION2       0x0002

typedef struct _DEVICE_DESCRIPTION {
  ULONG  Version;
  BOOLEAN  Master;
  BOOLEAN  ScatterGather;
  BOOLEAN  DemandMode;
  BOOLEAN  AutoInitialize;
  BOOLEAN  Dma32BitAddresses;
  BOOLEAN  IgnoreCount;
  BOOLEAN  Reserved1;
  BOOLEAN  Dma64BitAddresses;
  ULONG  BusNumber;
  ULONG  DmaChannel;
  INTERFACE_TYPE  InterfaceType;
  DMA_WIDTH  DmaWidth;
  DMA_SPEED  DmaSpeed;
  ULONG  MaximumLength;
  ULONG  DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

/* VPB.Flags */
#define VPB_MOUNTED                       0x0001
#define VPB_LOCKED                        0x0002
#define VPB_PERSISTENT                    0x0004
#define VPB_REMOVE_PENDING                0x0008
#define VPB_RAW_MOUNT                     0x0010

#define MAXIMUM_VOLUME_LABEL_LENGTH       (32 * sizeof(WCHAR))

typedef struct _VPB {
  CSHORT  Type;
  CSHORT  Size;
  USHORT  Flags;
  USHORT  VolumeLabelLength;
  struct _DEVICE_OBJECT  *DeviceObject;
  struct _DEVICE_OBJECT  *RealDevice;
  ULONG  SerialNumber;
  ULONG  ReferenceCount;
  WCHAR  VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
} VPB, *PVPB;

/* DEVICE_OBJECT.Flags */

#define DO_VERIFY_VOLUME                  0x00000002
#define DO_BUFFERED_IO                    0x00000004
#define DO_EXCLUSIVE                      0x00000008
#define DO_DIRECT_IO                      0x00000010
#define DO_MAP_IO_BUFFER                  0x00000020
#define DO_DEVICE_HAS_NAME                0x00000040
#define DO_DEVICE_INITIALIZING            0x00000080
#define DO_SYSTEM_BOOT_PARTITION          0x00000100
#define DO_LONG_TERM_REQUESTS             0x00000200
#define DO_NEVER_LAST_DEVICE              0x00000400
#define DO_SHUTDOWN_REGISTERED            0x00000800
#define DO_BUS_ENUMERATED_DEVICE          0x00001000
#define DO_POWER_PAGABLE                  0x00002000
#define DO_POWER_INRUSH                   0x00004000
#define DO_LOW_PRIORITY_FILESYSTEM        0x00010000
#define DO_XIP                            0x00020000

/* DEVICE_OBJECT.Characteristics */

#define FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONCE_MEDIA           0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define FILE_DEVICE_SECURE_OPEN         0x00000100

/* DEVICE_OBJECT.AlignmentRequirement */

#define FILE_BYTE_ALIGNMENT             0x00000000
#define FILE_WORD_ALIGNMENT             0x00000001
#define FILE_LONG_ALIGNMENT             0x00000003
#define FILE_QUAD_ALIGNMENT             0x00000007
#define FILE_OCTA_ALIGNMENT             0x0000000f
#define FILE_32_BYTE_ALIGNMENT          0x0000001f
#define FILE_64_BYTE_ALIGNMENT          0x0000003f
#define FILE_128_BYTE_ALIGNMENT         0x0000007f
#define FILE_256_BYTE_ALIGNMENT         0x000000ff
#define FILE_512_BYTE_ALIGNMENT         0x000001ff

/* DEVICE_OBJECT.DeviceType */

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                  0x00000001
#define FILE_DEVICE_CD_ROM                0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM    0x00000003
#define FILE_DEVICE_CONTROLLER            0x00000004
#define FILE_DEVICE_DATALINK              0x00000005
#define FILE_DEVICE_DFS                   0x00000006
#define FILE_DEVICE_DISK                  0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM      0x00000008
#define FILE_DEVICE_FILE_SYSTEM           0x00000009
#define FILE_DEVICE_INPORT_PORT           0x0000000a
#define FILE_DEVICE_KEYBOARD              0x0000000b
#define FILE_DEVICE_MAILSLOT              0x0000000c
#define FILE_DEVICE_MIDI_IN               0x0000000d
#define FILE_DEVICE_MIDI_OUT              0x0000000e
#define FILE_DEVICE_MOUSE                 0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER    0x00000010
#define FILE_DEVICE_NAMED_PIPE            0x00000011
#define FILE_DEVICE_NETWORK               0x00000012
#define FILE_DEVICE_NETWORK_BROWSER       0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM   0x00000014
#define FILE_DEVICE_NULL                  0x00000015
#define FILE_DEVICE_PARALLEL_PORT         0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD      0x00000017
#define FILE_DEVICE_PRINTER               0x00000018
#define FILE_DEVICE_SCANNER               0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT     0x0000001a
#define FILE_DEVICE_SERIAL_PORT           0x0000001b
#define FILE_DEVICE_SCREEN                0x0000001c
#define FILE_DEVICE_SOUND                 0x0000001d
#define FILE_DEVICE_STREAMS               0x0000001e
#define FILE_DEVICE_TAPE                  0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM      0x00000020
#define FILE_DEVICE_TRANSPORT             0x00000021
#define FILE_DEVICE_UNKNOWN               0x00000022
#define FILE_DEVICE_VIDEO                 0x00000023
#define FILE_DEVICE_VIRTUAL_DISK          0x00000024
#define FILE_DEVICE_WAVE_IN               0x00000025
#define FILE_DEVICE_WAVE_OUT              0x00000026
#define FILE_DEVICE_8042_PORT             0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR    0x00000028
#define FILE_DEVICE_BATTERY               0x00000029
#define FILE_DEVICE_BUS_EXTENDER          0x0000002a
#define FILE_DEVICE_MODEM                 0x0000002b
#define FILE_DEVICE_VDM                   0x0000002c
#define FILE_DEVICE_MASS_STORAGE          0x0000002d
#define FILE_DEVICE_SMB                   0x0000002e
#define FILE_DEVICE_KS                    0x0000002f
#define FILE_DEVICE_CHANGER               0x00000030
#define FILE_DEVICE_SMARTCARD             0x00000031
#define FILE_DEVICE_ACPI                  0x00000032
#define FILE_DEVICE_DVD                   0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO      0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM       0x00000035
#define FILE_DEVICE_DFS_VOLUME            0x00000036
#define FILE_DEVICE_SERENUM               0x00000037
#define FILE_DEVICE_TERMSRV               0x00000038
#define FILE_DEVICE_KSEC                  0x00000039
#define FILE_DEVICE_FIPS                  0x0000003a

typedef struct _DEVICE_OBJECT {
  CSHORT  Type;
  USHORT  Size;
  LONG  ReferenceCount;
  struct _DRIVER_OBJECT  *DriverObject;
  struct _DEVICE_OBJECT  *NextDevice;
  struct _DEVICE_OBJECT  *AttachedDevice;
  struct _IRP  *CurrentIrp;
  PIO_TIMER  Timer;
  ULONG  Flags;
  ULONG  Characteristics;
  volatile PVPB  Vpb;
  PVOID  DeviceExtension;
  DEVICE_TYPE  DeviceType;
  CCHAR  StackSize;
  union {
    LIST_ENTRY  ListEntry;
    WAIT_CONTEXT_BLOCK  Wcb;
  } Queue;
  ULONG  AlignmentRequirement;
  KDEVICE_QUEUE  DeviceQueue;
  KDPC  Dpc;
  ULONG  ActiveThreadCount;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;
  KEVENT  DeviceLock;
  USHORT  SectorSize;
  USHORT  Spare1;
  struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;
  PVOID  Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

typedef enum _DEVICE_RELATION_TYPE {
  BusRelations,
  EjectionRelations,
  PowerRelations,
  RemovalRelations,
  TargetDeviceRelation,
  SingleBusRelations
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

typedef struct _DEVICE_RELATIONS {
  ULONG  Count;
  PDEVICE_OBJECT Objects[1];
} DEVICE_RELATIONS, *PDEVICE_RELATIONS;

typedef struct _SCATTER_GATHER_ELEMENT {
  PHYSICAL_ADDRESS  Address;
  ULONG  Length;
  ULONG_PTR  Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

typedef struct _SCATTER_GATHER_LIST {
  ULONG  NumberOfElements;
  ULONG_PTR  Reserved;
  SCATTER_GATHER_ELEMENT  Elements[1];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

typedef struct _MDL {
  struct _MDL  *Next;
  CSHORT  Size;
  CSHORT  MdlFlags;
  struct _EPROCESS  *Process;
  PVOID  MappedSystemVa;
  PVOID  StartVa;
  ULONG  ByteCount;
  ULONG  ByteOffset;
} MDL, *PMDL;

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_FREE_EXTRA_PTES         0x0200
#define MDL_DESCRIBES_AWE           0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000
#define MDL_INTERNAL                0x8000


#define MDL_MAPPING_FLAGS ( \
  MDL_MAPPED_TO_SYSTEM_VA     | \
  MDL_PAGES_LOCKED            | \
  MDL_SOURCE_IS_NONPAGED_POOL | \
  MDL_PARTIAL_HAS_BEEN_MAPPED | \
  MDL_PARENT_MAPPED_SYSTEM_VA | \
  MDL_SYSTEM_VA               | \
  MDL_IO_SPACE)

typedef struct _DRIVER_EXTENSION {
  struct _DRIVER_OBJECT  *DriverObject;
  PDRIVER_ADD_DEVICE  AddDevice;
  ULONG  Count;
  UNICODE_STRING  ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

#define DRVO_UNLOAD_INVOKED               0x00000001
#define DRVO_LEGACY_DRIVER                0x00000002
#define DRVO_BUILTIN_DRIVER               0x00000004
#define DRVO_REINIT_REGISTERED            0x00000008
#define DRVO_INITIALIZED                  0x00000010
#define DRVO_BOOTREINIT_REGISTERED        0x00000020
#define DRVO_LEGACY_RESOURCES             0x00000040

typedef struct _DRIVER_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PDEVICE_OBJECT  DeviceObject;
  ULONG  Flags;
  PVOID  DriverStart;
  ULONG  DriverSize;
  PVOID  DriverSection;
  PDRIVER_EXTENSION  DriverExtension;
  UNICODE_STRING  DriverName;
  PUNICODE_STRING  HardwareDatabase;
  struct _FAST_IO_DISPATCH *FastIoDispatch;
  PDRIVER_INITIALIZE  DriverInit;
  PDRIVER_STARTIO  DriverStartIo;
  PDRIVER_UNLOAD  DriverUnload;
  PDRIVER_DISPATCH  MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;

typedef struct _DMA_ADAPTER {
  USHORT  Version;
  USHORT  Size;
  struct _DMA_OPERATIONS*  DmaOperations;
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID
(DDKAPI *PPUT_DMA_ADAPTER)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef PVOID
(DDKAPI *PALLOCATE_COMMON_BUFFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN ULONG  Length,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

typedef VOID
(DDKAPI *PFREE_COMMON_BUFFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

typedef NTSTATUS
(DDKAPI *PALLOCATE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

typedef BOOLEAN
(DDKAPI *PFLUSH_ADAPTER_BUFFERS)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN BOOLEAN  WriteToDevice);

typedef VOID
(DDKAPI *PFREE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef VOID
(DDKAPI *PFREE_MAP_REGISTERS)(
  IN PDMA_ADAPTER  DmaAdapter,
  PVOID  MapRegisterBase,
  ULONG  NumberOfMapRegisters);

typedef PHYSICAL_ADDRESS
(DDKAPI *PMAP_TRANSFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN OUT PULONG  Length,
  IN BOOLEAN  WriteToDevice);

typedef ULONG
(DDKAPI *PGET_DMA_ALIGNMENT)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef ULONG
(DDKAPI *PREAD_DMA_COUNTER)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef NTSTATUS
(DDKAPI *PGET_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PMDL  Mdl,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN PDRIVER_LIST_CONTROL  ExecutionRoutine,
  IN PVOID  Context,
  IN BOOLEAN  WriteToDevice);

typedef VOID
(DDKAPI *PPUT_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PSCATTER_GATHER_LIST  ScatterGather,
  IN BOOLEAN  WriteToDevice);

typedef NTSTATUS
(DDKAPI *PCALCULATE_SCATTER_GATHER_LIST_SIZE)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl  OPTIONAL,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  OUT PULONG  ScatterGatherListSize,
  OUT PULONG  pNumberOfMapRegisters  OPTIONAL);

typedef NTSTATUS
(DDKAPI *PBUILD_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PMDL  Mdl,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN PDRIVER_LIST_CONTROL  ExecutionRoutine,
  IN PVOID  Context,
  IN BOOLEAN  WriteToDevice,
  IN PVOID  ScatterGatherBuffer,
  IN ULONG  ScatterGatherLength);

typedef NTSTATUS
(DDKAPI *PBUILD_MDL_FROM_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PSCATTER_GATHER_LIST  ScatterGather,
  IN PMDL  OriginalMdl,
  OUT PMDL  *TargetMdl);

typedef struct _DMA_OPERATIONS {
  ULONG  Size;
  PPUT_DMA_ADAPTER  PutDmaAdapter;
  PALLOCATE_COMMON_BUFFER  AllocateCommonBuffer;
  PFREE_COMMON_BUFFER  FreeCommonBuffer;
  PALLOCATE_ADAPTER_CHANNEL  AllocateAdapterChannel;
  PFLUSH_ADAPTER_BUFFERS  FlushAdapterBuffers;
  PFREE_ADAPTER_CHANNEL  FreeAdapterChannel;
  PFREE_MAP_REGISTERS  FreeMapRegisters;
  PMAP_TRANSFER  MapTransfer;
  PGET_DMA_ALIGNMENT  GetDmaAlignment;
  PREAD_DMA_COUNTER  ReadDmaCounter;
  PGET_SCATTER_GATHER_LIST  GetScatterGatherList;
  PPUT_SCATTER_GATHER_LIST  PutScatterGatherList;
  PCALCULATE_SCATTER_GATHER_LIST_SIZE  CalculateScatterGatherList;
  PBUILD_SCATTER_GATHER_LIST  BuildScatterGatherList;
  PBUILD_MDL_FROM_SCATTER_GATHER_LIST  BuildMdlFromScatterGatherList;
} DMA_OPERATIONS, *PDMA_OPERATIONS;

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

typedef enum _KD_OPTION {
    KD_OPTION_SET_BLOCK_ENABLE,
} KD_OPTION;

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
extern DECLSPEC_IMPORT PHAL_DISPATCH HalDispatchTable;
#define HALDISPATCH ((PHAL_DISPATCH)&HalDispatchTable)
#else
extern DECLSPEC_EXPORT HAL_DISPATCH HalDispatchTable;
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

#ifndef _NTOSKRNL_
#define HalDeviceControl                HALDISPATCH->HalDeviceControl
#define HalIoAssignDriveLetters         HALDISPATCH->HalIoAssignDriveLetters
#define HalIoReadPartitionTable         HALDISPATCH->HalIoReadPartitionTable
#define HalIoSetPartitionInformation    HALDISPATCH->HalIoSetPartitionInformation
#define HalIoWritePartitionTable        HALDISPATCH->HalIoWritePartitionTable
#endif

typedef enum _FILE_INFORMATION_CLASS {
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_POSITION_INFORMATION {
  LARGE_INTEGER  CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
  ULONG  AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
  ULONG  FileNameLength;
  WCHAR  FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

#include <pshpack8.h>
typedef struct _FILE_BASIC_INFORMATION {
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  ULONG  FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
#include <poppack.h>

typedef struct _FILE_STANDARD_INFORMATION {
  LARGE_INTEGER  AllocationSize;
  LARGE_INTEGER  EndOfFile;
  ULONG  NumberOfLinks;
  BOOLEAN  DeletePending;
  BOOLEAN  Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  LARGE_INTEGER  AllocationSize;
  LARGE_INTEGER  EndOfFile;
  ULONG  FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

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

typedef enum _FSINFOCLASS {
  FileFsVolumeInformation = 1,
  FileFsLabelInformation,
  FileFsSizeInformation,
  FileFsDeviceInformation,
  FileFsAttributeInformation,
  FileFsControlInformation,
  FileFsFullSizeInformation,
  FileFsObjectIdInformation,
  FileFsDriverPathInformation,
  FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_DEVICE_INFORMATION {
  DEVICE_TYPE  DeviceType;
  ULONG  Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
  ULONG  NextEntryOffset;
  UCHAR  Flags;
  UCHAR  EaNameLength;
  USHORT  EaValueLength;
  CHAR  EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
  ERESOURCE_THREAD  OwnerThread;
  _ANONYMOUS_UNION union {
      LONG  OwnerCount;
      ULONG  TableSize;
  } DUMMYUNIONNAME;
} OWNER_ENTRY, *POWNER_ENTRY;

/* ERESOURCE.Flag */

#define ResourceNeverExclusive            0x0010
#define ResourceReleaseByOtherThread      0x0020
#define ResourceOwnedExclusive            0x0080

#define RESOURCE_HASH_TABLE_SIZE          64

typedef struct _ERESOURCE
{
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    volatile PKSEMAPHORE SharedWaiters;
    volatile PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerEntry;
    ULONG ActiveEntries;
    ULONG ContentionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
    union
    {
        PVOID Address;
        ULONG_PTR CreatorBackTraceIndex;
    };
    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

typedef struct _DEVOBJ_EXTENSION
{
    CSHORT Type;
    USHORT Size;
    PDEVICE_OBJECT DeviceObject;
} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;

typedef BOOLEAN
(DDKAPI *PFAST_IO_CHECK_IF_POSSIBLE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  IN BOOLEAN  CheckForReadOperation,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_READ)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  OUT PVOID  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  IN PVOID  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_QUERY_BASIC_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT PFILE_BASIC_INFORMATION  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_QUERY_STANDARD_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT PFILE_STANDARD_INFORMATION  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_LOCK)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PLARGE_INTEGER  Length,
  PEPROCESS  ProcessId,
  ULONG  Key,
  BOOLEAN  FailImmediately,
  BOOLEAN  ExclusiveLock,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_UNLOCK_SINGLE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PLARGE_INTEGER  Length,
  PEPROCESS  ProcessId,
  ULONG  Key,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_UNLOCK_ALL)(
  IN struct _FILE_OBJECT  *FileObject,
  PEPROCESS  ProcessId,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_UNLOCK_ALL_BY_KEY)(
  IN struct _FILE_OBJECT  *FileObject,
  PVOID  ProcessId,
  ULONG  Key,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_DEVICE_CONTROL)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  IN PVOID  InputBuffer  OPTIONAL,
  IN ULONG  InputBufferLength,
  OUT PVOID  OutputBuffer  OPTIONAL,
  IN ULONG  OutputBufferLength,
  IN ULONG  IoControlCode,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef VOID
(DDKAPI *PFAST_IO_ACQUIRE_FILE)(
  IN struct _FILE_OBJECT  *FileObject);

typedef VOID
(DDKAPI *PFAST_IO_RELEASE_FILE)(
  IN struct _FILE_OBJECT  *FileObject);

typedef VOID
(DDKAPI *PFAST_IO_DETACH_DEVICE)(
  IN struct _DEVICE_OBJECT  *SourceDevice,
  IN struct _DEVICE_OBJECT  *TargetDevice);

typedef BOOLEAN
(DDKAPI *PFAST_IO_QUERY_NETWORK_OPEN_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT struct _FILE_NETWORK_OPEN_INFORMATION  *Buffer,
  OUT struct _IO_STATUS_BLOCK  *IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS
(DDKAPI *PFAST_IO_ACQUIRE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  EndingOffset,
  OUT struct _ERESOURCE  **ResourceToRelease,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_MDL_READ)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_MDL_READ_COMPLETE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_PREPARE_MDL_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_MDL_WRITE_COMPLETE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_READ_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  OUT PVOID  Buffer,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  OUT struct _COMPRESSED_DATA_INFO  *CompressedDataInfo,
  IN ULONG  CompressedDataInfoLength,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_WRITE_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  IN PVOID  Buffer,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _COMPRESSED_DATA_INFO  *CompressedDataInfo,
  IN ULONG  CompressedDataInfoLength,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_MDL_READ_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN
(DDKAPI *PFAST_IO_QUERY_OPEN)(
  IN struct _IRP  *Irp,
  OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS
(DDKAPI *PFAST_IO_RELEASE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN struct _ERESOURCE  *ResourceToRelease,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS
(DDKAPI *PFAST_IO_ACQUIRE_FOR_CCFLUSH)(
  IN struct _FILE_OBJECT  *FileObject,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS
(DDKAPI *PFAST_IO_RELEASE_FOR_CCFLUSH) (
  IN struct _FILE_OBJECT  *FileObject,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef struct _FAST_IO_DISPATCH {
  ULONG  SizeOfFastIoDispatch;
  PFAST_IO_CHECK_IF_POSSIBLE  FastIoCheckIfPossible;
  PFAST_IO_READ  FastIoRead;
  PFAST_IO_WRITE  FastIoWrite;
  PFAST_IO_QUERY_BASIC_INFO  FastIoQueryBasicInfo;
  PFAST_IO_QUERY_STANDARD_INFO  FastIoQueryStandardInfo;
  PFAST_IO_LOCK  FastIoLock;
  PFAST_IO_UNLOCK_SINGLE  FastIoUnlockSingle;
  PFAST_IO_UNLOCK_ALL  FastIoUnlockAll;
  PFAST_IO_UNLOCK_ALL_BY_KEY  FastIoUnlockAllByKey;
  PFAST_IO_DEVICE_CONTROL  FastIoDeviceControl;
  PFAST_IO_ACQUIRE_FILE  AcquireFileForNtCreateSection;
  PFAST_IO_RELEASE_FILE  ReleaseFileForNtCreateSection;
  PFAST_IO_DETACH_DEVICE  FastIoDetachDevice;
  PFAST_IO_QUERY_NETWORK_OPEN_INFO  FastIoQueryNetworkOpenInfo;
  PFAST_IO_ACQUIRE_FOR_MOD_WRITE  AcquireForModWrite;
  PFAST_IO_MDL_READ  MdlRead;
  PFAST_IO_MDL_READ_COMPLETE  MdlReadComplete;
  PFAST_IO_PREPARE_MDL_WRITE  PrepareMdlWrite;
  PFAST_IO_MDL_WRITE_COMPLETE  MdlWriteComplete;
  PFAST_IO_READ_COMPRESSED  FastIoReadCompressed;
  PFAST_IO_WRITE_COMPRESSED  FastIoWriteCompressed;
  PFAST_IO_MDL_READ_COMPLETE_COMPRESSED  MdlReadCompleteCompressed;
  PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED  MdlWriteCompleteCompressed;
  PFAST_IO_QUERY_OPEN  FastIoQueryOpen;
  PFAST_IO_RELEASE_FOR_MOD_WRITE  ReleaseForModWrite;
  PFAST_IO_ACQUIRE_FOR_CCFLUSH  AcquireForCcFlush;
  PFAST_IO_RELEASE_FOR_CCFLUSH  ReleaseForCcFlush;
} FAST_IO_DISPATCH, *PFAST_IO_DISPATCH;

typedef struct _SECTION_OBJECT_POINTERS {
  PVOID  DataSectionObject;
  PVOID  SharedCacheMap;
  PVOID  ImageSectionObject;
} SECTION_OBJECT_POINTERS, *PSECTION_OBJECT_POINTERS;

typedef struct _IO_COMPLETION_CONTEXT {
  PVOID  Port;
  PVOID  Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

/* FILE_OBJECT.Flags */

#define FO_FILE_OPEN                      0x00000001
#define FO_SYNCHRONOUS_IO                 0x00000002
#define FO_ALERTABLE_IO                   0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING      0x00000008
#define FO_WRITE_THROUGH                  0x00000010
#define FO_SEQUENTIAL_ONLY                0x00000020
#define FO_CACHE_SUPPORTED                0x00000040
#define FO_NAMED_PIPE                     0x00000080
#define FO_STREAM_FILE                    0x00000100
#define FO_MAILSLOT                       0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE        0x00000400
#define FO_DIRECT_DEVICE_OPEN             0x00000800
#define FO_FILE_MODIFIED                  0x00001000
#define FO_FILE_SIZE_CHANGED              0x00002000
#define FO_CLEANUP_COMPLETE               0x00004000
#define FO_TEMPORARY_FILE                 0x00008000
#define FO_DELETE_ON_CLOSE                0x00010000
#define FO_OPENED_CASE_SENSITIVE          0x00020000
#define FO_HANDLE_CREATED                 0x00040000
#define FO_FILE_FAST_IO_READ              0x00080000
#define FO_RANDOM_ACCESS                  0x00100000
#define FO_FILE_OPEN_CANCELLED            0x00200000
#define FO_VOLUME_OPEN                    0x00400000
#define FO_REMOTE_ORIGIN                  0x01000000

typedef struct _FILE_OBJECT
{
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
    PVPB Vpb;
    PVOID FsContext;
    PVOID FsContext2;
    PSECTION_OBJECT_POINTERS SectionObjectPointer;
    PVOID PrivateCacheMap;
    NTSTATUS FinalStatus;
    struct _FILE_OBJECT *RelatedFileObject;
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
    volatile ULONG Waiters;
    volatile ULONG Busy;
    PVOID LastLock;
    KEVENT Lock;
    KEVENT Event;
    volatile PIO_COMPLETION_CONTEXT CompletionContext;
    KSPIN_LOCK IrpListLock;
    LIST_ENTRY IrpList;
    volatile PVOID FileObjectExtension;
} FILE_OBJECT;
typedef struct _FILE_OBJECT *PFILE_OBJECT;

typedef enum _SECURITY_OPERATION_CODE {
  SetSecurityDescriptor,
  QuerySecurityDescriptor,
  DeleteSecurityDescriptor,
  AssignSecurityDescriptor
} SECURITY_OPERATION_CODE, *PSECURITY_OPERATION_CODE;

#define INITIAL_PRIVILEGE_COUNT           3

typedef struct _INITIAL_PRIVILEGE_SET {
  ULONG  PrivilegeCount;
  ULONG  Control;
  LUID_AND_ATTRIBUTES  Privilege[INITIAL_PRIVILEGE_COUNT];
} INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;

#define SE_MIN_WELL_KNOWN_PRIVILEGE       2
#define SE_CREATE_TOKEN_PRIVILEGE         2
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3
#define SE_LOCK_MEMORY_PRIVILEGE          4
#define SE_INCREASE_QUOTA_PRIVILEGE       5
#define SE_UNSOLICITED_INPUT_PRIVILEGE    6
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6
#define SE_TCB_PRIVILEGE                  7
#define SE_SECURITY_PRIVILEGE             8
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9
#define SE_LOAD_DRIVER_PRIVILEGE          10
#define SE_SYSTEM_PROFILE_PRIVILEGE       11
#define SE_SYSTEMTIME_PRIVILEGE           12
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  13
#define SE_INC_BASE_PRIORITY_PRIVILEGE    14
#define SE_CREATE_PAGEFILE_PRIVILEGE      15
#define SE_CREATE_PERMANENT_PRIVILEGE     16
#define SE_BACKUP_PRIVILEGE               17
#define SE_RESTORE_PRIVILEGE              18
#define SE_SHUTDOWN_PRIVILEGE             19
#define SE_DEBUG_PRIVILEGE                20
#define SE_AUDIT_PRIVILEGE                21
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE   22
#define SE_CHANGE_NOTIFY_PRIVILEGE        23
#define SE_REMOTE_SHUTDOWN_PRIVILEGE      24
#define SE_UNDOCK_PRIVILEGE               25
#define SE_SYNC_AGENT_PRIVILEGE           26
#define SE_ENABLE_DELEGATION_PRIVILEGE    27
#define SE_MANAGE_VOLUME_PRIVILEGE        28
#define SE_IMPERSONATE_PRIVILEGE          29
#define SE_CREATE_GLOBAL_PRIVILEGE        30
#define SE_MAX_WELL_KNOWN_PRIVILEGE       SE_CREATE_GLOBAL_PRIVILEGE

typedef struct _SECURITY_SUBJECT_CONTEXT {
  PACCESS_TOKEN  ClientToken;
  SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel;
  PACCESS_TOKEN  PrimaryToken;
  PVOID  ProcessAuditId;
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

#include <pshpack4.h>
typedef struct _ACCESS_STATE {
  LUID  OperationID;
  BOOLEAN  SecurityEvaluated;
  BOOLEAN  GenerateAudit;
  BOOLEAN  GenerateOnClose;
  BOOLEAN  PrivilegesAllocated;
  ULONG  Flags;
  ACCESS_MASK  RemainingDesiredAccess;
  ACCESS_MASK  PreviouslyGrantedAccess;
  ACCESS_MASK  OriginalDesiredAccess;
  SECURITY_SUBJECT_CONTEXT  SubjectSecurityContext;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;
  PVOID  AuxData;
  union {
    INITIAL_PRIVILEGE_SET  InitialPrivilegeSet;
    PRIVILEGE_SET  PrivilegeSet;
  } Privileges;

  BOOLEAN  AuditPrivileges;
  UNICODE_STRING  ObjectName;
  UNICODE_STRING  ObjectTypeName;
} ACCESS_STATE, *PACCESS_STATE;
#include <poppack.h>

typedef struct _IO_SECURITY_CONTEXT {
  PSECURITY_QUALITY_OF_SERVICE  SecurityQos;
  PACCESS_STATE  AccessState;
  ACCESS_MASK  DesiredAccess;
  ULONG  FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;

#define IO_TYPE_ADAPTER                 1
#define IO_TYPE_CONTROLLER              2
#define IO_TYPE_DEVICE                  3
#define IO_TYPE_DRIVER                  4
#define IO_TYPE_FILE                    5
#define IO_TYPE_IRP                     6
#define IO_TYPE_MASTER_ADAPTER          7
#define IO_TYPE_OPEN_PACKET             8
#define IO_TYPE_TIMER                   9
#define IO_TYPE_VPB                     10
#define IO_TYPE_ERROR_LOG               11
#define IO_TYPE_ERROR_MESSAGE           12
#define IO_TYPE_DEVICE_OBJECT_EXTENSION 13

#define IO_TYPE_CSQ_IRP_CONTEXT 1
#define IO_TYPE_CSQ 2

struct _IO_CSQ;

typedef struct _IO_CSQ_IRP_CONTEXT {
  ULONG  Type;
  struct _IRP  *Irp;
  struct _IO_CSQ  *Csq;
} IO_CSQ_IRP_CONTEXT, *PIO_CSQ_IRP_CONTEXT;

typedef VOID
(DDKAPI *PIO_CSQ_INSERT_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp);

typedef VOID
(DDKAPI *PIO_CSQ_REMOVE_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp);

typedef PIRP
(DDKAPI *PIO_CSQ_PEEK_NEXT_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp,
  IN PVOID  PeekContext);

typedef VOID
(DDKAPI *PIO_CSQ_ACQUIRE_LOCK)(
  IN  struct _IO_CSQ  *Csq,
  OUT PKIRQL  Irql);

typedef VOID
(DDKAPI *PIO_CSQ_RELEASE_LOCK)(
  IN struct _IO_CSQ  *Csq,
  IN KIRQL  Irql);

typedef VOID
(DDKAPI *PIO_CSQ_COMPLETE_CANCELED_IRP)(
  IN  struct _IO_CSQ  *Csq,
  IN  PIRP  Irp);

typedef struct _IO_CSQ {
  ULONG  Type;
  PIO_CSQ_INSERT_IRP  CsqInsertIrp;
  PIO_CSQ_REMOVE_IRP  CsqRemoveIrp;
  PIO_CSQ_PEEK_NEXT_IRP  CsqPeekNextIrp;
  PIO_CSQ_ACQUIRE_LOCK  CsqAcquireLock;
  PIO_CSQ_RELEASE_LOCK  CsqReleaseLock;
  PIO_CSQ_COMPLETE_CANCELED_IRP  CsqCompleteCanceledIrp;
  PVOID  ReservePointer;
} IO_CSQ, *PIO_CSQ;

#if !defined(_ALPHA_)
#include <pshpack4.h>
#endif
typedef struct _IO_STACK_LOCATION {
  UCHAR  MajorFunction;
  UCHAR  MinorFunction;
  UCHAR  Flags;
  UCHAR  Control;
  union {
    struct {
      PIO_SECURITY_CONTEXT  SecurityContext;
      ULONG  Options;
      USHORT POINTER_ALIGNMENT  FileAttributes;
      USHORT  ShareAccess;
      ULONG POINTER_ALIGNMENT  EaLength;
    } Create;
    struct {
      ULONG  Length;
      ULONG POINTER_ALIGNMENT  Key;
      LARGE_INTEGER  ByteOffset;
    } Read;
    struct {
      ULONG  Length;
      ULONG POINTER_ALIGNMENT  Key;
      LARGE_INTEGER  ByteOffset;
    } Write;
    struct {
      ULONG  Length;
      PUNICODE_STRING  FileName;
      FILE_INFORMATION_CLASS  FileInformationClass;
      ULONG  FileIndex;
    } QueryDirectory;
    struct {
      ULONG  Length;
      ULONG  CompletionFilter;
    } NotifyDirectory;
    struct {
      ULONG  Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT  FileInformationClass;
    } QueryFile;
    struct {
      ULONG  Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT  FileInformationClass;
      PFILE_OBJECT  FileObject;
      _ANONYMOUS_UNION union {
        _ANONYMOUS_STRUCT struct {
          BOOLEAN  ReplaceIfExists;
          BOOLEAN  AdvanceOnly;
        } DUMMYSTRUCTNAME;
        ULONG  ClusterCount;
        HANDLE  DeleteHandle;
      } DUMMYUNIONNAME;
    } SetFile;
    struct {
      ULONG  Length;
      PVOID  EaList;
      ULONG  EaListLength;
      ULONG  EaIndex;
    } QueryEa;
    struct {
      ULONG  Length;
    } SetEa;
    struct {
      ULONG  Length;
      FS_INFORMATION_CLASS POINTER_ALIGNMENT  FsInformationClass;
    } QueryVolume;
    struct {
      ULONG  Length;
      FS_INFORMATION_CLASS  FsInformationClass;
    } SetVolume;
    struct {
      ULONG  OutputBufferLength;
      ULONG  InputBufferLength;
      ULONG  FsControlCode;
      PVOID  Type3InputBuffer;
    } FileSystemControl;
    struct {
      PLARGE_INTEGER  Length;
      ULONG  Key;
      LARGE_INTEGER  ByteOffset;
    } LockControl;
    struct {
      ULONG  OutputBufferLength;
      ULONG POINTER_ALIGNMENT  InputBufferLength;
      ULONG POINTER_ALIGNMENT  IoControlCode;
      PVOID  Type3InputBuffer;
    } DeviceIoControl;
    struct {
      SECURITY_INFORMATION  SecurityInformation;
      ULONG POINTER_ALIGNMENT  Length;
    } QuerySecurity;
    struct {
      SECURITY_INFORMATION  SecurityInformation;
      PSECURITY_DESCRIPTOR  SecurityDescriptor;
    } SetSecurity;
    struct {
      PVPB  Vpb;
      PDEVICE_OBJECT  DeviceObject;
    } MountVolume;
    struct {
      PVPB  Vpb;
      PDEVICE_OBJECT  DeviceObject;
    } VerifyVolume;
    struct {
      struct _SCSI_REQUEST_BLOCK  *Srb;
    } Scsi;
    struct {
      ULONG  Length;
      PSID  StartSid;
      struct _FILE_GET_QUOTA_INFORMATION  *SidList;
      ULONG  SidListLength;
    } QueryQuota;
    struct {
      ULONG  Length;
    } SetQuota;
    struct {
      DEVICE_RELATION_TYPE  Type;
    } QueryDeviceRelations;
    struct {
      CONST GUID  *InterfaceType;
      USHORT  Size;
      USHORT  Version;
      PINTERFACE  Interface;
      PVOID  InterfaceSpecificData;
    } QueryInterface;
    struct {
      PDEVICE_CAPABILITIES  Capabilities;
    } DeviceCapabilities;
    struct {
      PIO_RESOURCE_REQUIREMENTS_LIST  IoResourceRequirementList;
    } FilterResourceRequirements;
    struct {
      ULONG  WhichSpace;
      PVOID  Buffer;
      ULONG  Offset;
      ULONG POINTER_ALIGNMENT  Length;
    } ReadWriteConfig;
    struct {
      BOOLEAN  Lock;
    } SetLock;
    struct {
      BUS_QUERY_ID_TYPE  IdType;
    } QueryId;
    struct {
      DEVICE_TEXT_TYPE  DeviceTextType;
      LCID POINTER_ALIGNMENT  LocaleId;
    } QueryDeviceText;
    struct {
      BOOLEAN  InPath;
      BOOLEAN  Reserved[3];
      DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT  Type;
    } UsageNotification;
    struct {
      SYSTEM_POWER_STATE  PowerState;
    } WaitWake;
    struct {
      PPOWER_SEQUENCE  PowerSequence;
    } PowerSequence;
    struct {
      ULONG  SystemContext;
      POWER_STATE_TYPE POINTER_ALIGNMENT  Type;
      POWER_STATE POINTER_ALIGNMENT  State;
      POWER_ACTION POINTER_ALIGNMENT  ShutdownType;
    } Power;
    struct {
      PCM_RESOURCE_LIST  AllocatedResources;
      PCM_RESOURCE_LIST  AllocatedResourcesTranslated;
    } StartDevice;
    struct {
      ULONG_PTR  ProviderId;
      PVOID  DataPath;
      ULONG  BufferSize;
      PVOID  Buffer;
    } WMI;
    struct {
      PVOID  Argument1;
      PVOID  Argument2;
      PVOID  Argument3;
      PVOID  Argument4;
    } Others;
  } Parameters;
  PDEVICE_OBJECT  DeviceObject;
  PFILE_OBJECT  FileObject;
  PIO_COMPLETION_ROUTINE  CompletionRoutine;
  PVOID  Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
#if !defined(_ALPHA_)
#include <poppack.h>
#endif

/* IO_STACK_LOCATION.Control */

#define SL_PENDING_RETURNED               0x01
#define SL_ERROR_RETURNED                 0x02
#define SL_INVOKE_ON_CANCEL               0x20
#define SL_INVOKE_ON_SUCCESS              0x40
#define SL_INVOKE_ON_ERROR                0x80

/* IO_STACK_LOCATION.Parameters.ReadWriteControl.WhichSpace */

#define PCI_WHICHSPACE_CONFIG             0x0
#define PCI_WHICHSPACE_ROM                0x52696350 /* 'PciR' */

typedef enum _KEY_INFORMATION_CLASS {
  KeyBasicInformation,
  KeyNodeInformation,
  KeyFullInformation,
  KeyNameInformation,
  KeyCachedInformation,
  KeyFlagsInformation
} KEY_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION {
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  ClassOffset;
  ULONG  ClassLength;
  ULONG  SubKeys;
  ULONG  MaxNameLen;
  ULONG  MaxClassLen;
  ULONG  Values;
  ULONG  MaxValueNameLen;
  ULONG  MaxValueDataLen;
  WCHAR  Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  ClassOffset;
  ULONG  ClassLength;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_VALUE_BASIC_INFORMATION {
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  DataOffset;
  ULONG  DataLength;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  DataLength;
  UCHAR  Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
  ULONG  Type;
  ULONG  DataLength;
  UCHAR  Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef struct _KEY_VALUE_ENTRY {
  PUNICODE_STRING  ValueName;
  ULONG  DataLength;
  ULONG  DataOffset;
  ULONG  Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
  KeyValueBasicInformation,
  KeyValueFullInformation,
  KeyValuePartialInformation,
  KeyValueFullInformationAlign64,
  KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_WRITE_TIME_INFORMATION {
  LARGE_INTEGER  LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef struct _KEY_USER_FLAGS_INFORMATION {
  ULONG  UserFlags;
} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
  KeyWriteTimeInformation,
  KeyUserFlagsInformation,
  MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

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

#define PCI_ADDRESS_IO_SPACE                0x01
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x06
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x08
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

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS,
  MaxPoolType,
  NonPagedPoolSession = 32,
  PagedPoolSession,
  NonPagedPoolMustSucceedSession,
  DontUseThisTypeSession,
  NonPagedPoolCacheAlignedSession,
  PagedPoolCacheAlignedSession,
  NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

#define POOL_COLD_ALLOCATION                256
#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE    8
#define POOL_RAISE_IF_ALLOCATION_FAILURE    16


typedef enum _EX_POOL_PRIORITY {
  LowPoolPriority,
  LowPoolPrioritySpecialPoolOverrun = 8,
  LowPoolPrioritySpecialPoolUnderrun = 9,
  NormalPoolPriority = 16,
  NormalPoolPrioritySpecialPoolOverrun = 24,
  NormalPoolPrioritySpecialPoolUnderrun = 25,
  HighPoolPriority = 32,
  HighPoolPrioritySpecialPoolOverrun = 40,
  HighPoolPrioritySpecialPoolUnderrun = 41
} EX_POOL_PRIORITY;

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

typedef struct _RTL_BITMAP {
  ULONG  SizeOfBitMap;
  PULONG  Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP) / 32]) >> ((BP) % 32)) & 0x1)

typedef struct _RTL_BITMAP_RUN {
    ULONG  StartingIndex;
    ULONG  NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

struct _RTL_RANGE;

typedef BOOLEAN
(NTAPI *PRTL_CONFLICT_RANGE_CALLBACK) (
    PVOID Context,
    struct _RTL_RANGE *Range
);

typedef NTSTATUS
(DDKAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
  IN PWSTR  ValueName,
  IN ULONG  ValueType,
  IN PVOID  ValueData,
  IN ULONG  ValueLength,
  IN PVOID  Context,
  IN PVOID  EntryContext);

#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5
#define RTL_REGISTRY_HANDLE               0x40000000
#define RTL_REGISTRY_OPTIONAL             0x80000000

/* RTL_QUERY_REGISTRY_TABLE.Flags */
#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040

typedef struct _RTL_QUERY_REGISTRY_TABLE {
  PRTL_QUERY_REGISTRY_ROUTINE  QueryRoutine;
  ULONG  Flags;
  PCWSTR Name;
  PVOID  EntryContext;
  ULONG  DefaultType;
  PVOID  DefaultData;
  ULONG  DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _TIME_FIELDS {
  CSHORT  Year;
  CSHORT  Month;
  CSHORT  Day;
  CSHORT  Hour;
  CSHORT  Minute;
  CSHORT  Second;
  CSHORT  Milliseconds;
  CSHORT  Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

typedef PVOID
(DDKAPI *PALLOCATE_FUNCTION)(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

typedef VOID
(DDKAPI *PFREE_FUNCTION)(
  IN PVOID  Buffer);

typedef struct _GENERAL_LOOKASIDE {
  SLIST_HEADER  ListHead;
  USHORT  Depth;
  USHORT  MaximumDepth;
  ULONG  TotalAllocates;
  union {
    ULONG  AllocateMisses;
    ULONG  AllocateHits;
  };
  ULONG  TotalFrees;
  union {
    ULONG  FreeMisses;
    ULONG  FreeHits;
  };
  POOL_TYPE  Type;
  ULONG  Tag;
  ULONG  Size;
  PALLOCATE_FUNCTION  Allocate;
  PFREE_FUNCTION  Free;
  LIST_ENTRY  ListEntry;
  ULONG  LastTotalAllocates;
  union {
    ULONG  LastAllocateMisses;
    ULONG  LastAllocateHits;
  };
  ULONG Future[2];
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _NPAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE  L;
  KSPIN_LOCK  Obsoleted;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct _PAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE  L;
  FAST_MUTEX  Obsoleted;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef VOID
(DDKAPI *PCALLBACK_FUNCTION)(
  IN PVOID  CallbackContext,
  IN PVOID  Argument1,
  IN PVOID  Argument2);

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
#ifdef DBG
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

typedef enum _KPROFILE_SOURCE {
  ProfileTime,
  ProfileAlignmentFixup,
  ProfileTotalIssues,
  ProfilePipelineDry,
  ProfileLoadInstructions,
  ProfilePipelineFrozen,
  ProfileBranchInstructions,
  ProfileTotalNonissues,
  ProfileDcacheMisses,
  ProfileIcacheMisses,
  ProfileCacheMisses,
  ProfileBranchMispredictions,
  ProfileStoreInstructions,
  ProfileFpInstructions,
  ProfileIntegerInstructions,
  Profile2Issue,
  Profile3Issue,
  Profile4Issue,
  ProfileSpecialInstructions,
  ProfileTotalCycles,
  ProfileIcacheIssues,
  ProfileDcacheAccesses,
  ProfileMemoryBarrierCycles,
  ProfileLoadLinkedIssues,
  ProfileMaximum
} KPROFILE_SOURCE;

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

typedef enum _WORK_QUEUE_TYPE {
  CriticalWorkQueue,
  DelayedWorkQueue,
  HyperCriticalWorkQueue,
  MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef VOID
(DDKAPI *PWORKER_THREAD_ROUTINE)(
  IN PVOID Parameter);

typedef struct _WORK_QUEUE_ITEM {
  LIST_ENTRY  List;
  PWORKER_THREAD_ROUTINE  WorkerRoutine;
  volatile PVOID  Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

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
    union
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
(*PDRIVER_VERIFIER_THUNK_ROUTINE)(
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

typedef enum _SUITE_TYPE {
  SmallBusiness,
  Enterprise,
  BackOffice,
  CommunicationServer,
  TerminalServer,
  SmallBusinessRestricted,
  EmbeddedNT,
  DataCenter,
  SingleUserTS,
  Personal,
  Blade,
  MaxSuiteType
} SUITE_TYPE;

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

static __inline
ULONG
DDKAPI
KeGetCurrentProcessorNumber(VOID)
{
#if defined(__GNUC__)
  ULONG ret;
  __asm__ __volatile__ (
    "movl %%fs:%c1, %0\n"
    : "=r" (ret)
    : "i" (FIELD_OFFSET(KPCR, Number))
  );
  return ret;
#elif defined(_MSC_VER)
#if _MSC_FULL_VER >= 13012035
  return (ULONG)__readfsbyte(FIELD_OFFSET(KPCR, Number));
#else
  __asm { movzx eax, _PCR KPCR.Number }
#endif
#else
#error Unknown compiler
#endif
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
    USHORT EFlags;

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
    union
    {
        NT_TIB NtTib;
        struct
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

#define PCI_DATA_TAG TAG('P', 'C', 'I', ' ')
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


/** INTERLOCKED FUNCTIONS *****************************************************/

#if !defined(__INTERLOCKED_DECLARED)
#define __INTERLOCKED_DECLARED

#if defined (_X86_)
#if defined(NO_INTERLOCKED_INTRINSICS)
NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedCompareExchange(
  IN OUT LONG volatile *Destination,
  IN LONG  Exchange,
  IN LONG  Comparand);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
  IN OUT LONG volatile *Destination,
  IN LONG Value);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchangeAdd(
  IN OUT LONG volatile *Addend,
  IN LONG  Value);

#else // !defined(NO_INTERLOCKED_INTRINSICS)

#define InterlockedExchange _InterlockedExchange
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedOr _InterlockedOr
#define InterlockedAnd _InterlockedAnd
#define InterlockedXor _InterlockedXor

#endif // !defined(NO_INTERLOCKED_INTRINSICS)

#endif // defined (_X86_)

#if !defined (_WIN64)
/*
 * PVOID
 * InterlockedExchangePointer(
 *   IN OUT PVOID VOLATILE  *Target,
 *   IN PVOID  Value)
 */
#define InterlockedExchangePointer(Target, Value) \
  ((PVOID) InterlockedExchange((PLONG) Target, (LONG) Value))

/*
 * PVOID
 * InterlockedCompareExchangePointer(
 *   IN OUT PVOID  *Destination,
 *   IN PVOID  Exchange,
 *   IN PVOID  Comparand)
 */
#define InterlockedCompareExchangePointer(Destination, Exchange, Comparand) \
  ((PVOID) InterlockedCompareExchange((PLONG) Destination, (LONG) Exchange, (LONG) Comparand))

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement((LONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement((LONG *)a)

#endif // !defined (_WIN64)

#if defined (_M_AMD64)

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONGLONG *)a, (LONGLONG)b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONGLONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONGLONG *)a)
#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedAdd _InterlockedAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedExchangePointer _InterlockedExchangePointer

#define ExInterlockedPopEntrySList(Head, Lock) ExpInterlockedPopEntrySList(Head)
#define ExInterlockedPushEntrySList(Head, Entry, Lock) ExpInterlockedPushEntrySList(Head, Entry)
#define ExInterlockedFlushSList(Head) ExpInterlockedFlushSList(Head)

#if !defined(_WINBASE_)
#define InterlockedPopEntrySList(Head) ExpInterlockedPopEntrySList(Head)
#define InterlockedPushEntrySList(Head, Entry) ExpInterlockedPushEntrySList(Head, Entry)
#define InterlockedFlushSList(Head) ExpInterlockedFlushSList(Head)
#define QueryDepthSList(Head) ExQueryDepthSList(Head)
#endif // !defined(_WINBASE_)

#endif // _M_AMD64

#endif /* !__INTERLOCKED_DECLARED */


/** SPINLOCK FUNCTIONS ********************************************************/

NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(
    IN OUT PKSPIN_LOCK SpinLock
);

#if defined (_X86_)

NTKERNELAPI
VOID
NTAPI
KeInitializeSpinLock(
  IN PKSPIN_LOCK  SpinLock);

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

#else // !defined (_X86_)

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

#define RtlCopyMemoryNonTemporal RtlCopyMemory

#define KeGetDcacheFillSize() 1L



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

NTSYSAPI
VOID
NTAPI
RtlAssert(
  IN PVOID  FailedAssertion,
  IN PVOID  FileName,
  IN ULONG  LineNumber,
  IN PCHAR  Message);

#ifdef DBG

#define ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( #exp, __FILE__, __LINE__, NULL ), FALSE : TRUE)

#define ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    RtlAssert( #exp, __FILE__, __LINE__, msg ), FALSE : TRUE)

#define RTL_SOFT_ASSERT(exp) \
  (VOID)((!(_exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #exp), FALSE : TRUE)

#define RTL_SOFT_ASSERTMSG(msg, exp) \
  (VOID)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #exp, (msg)), FALSE : TRUE)

#define RTL_VERIFY(exp) ASSERT(exp)
#define RTL_VERIFYMSG(msg, exp) ASSERT(msg, exp)

#define RTL_SOFT_VERIFY(exp) RTL_SOFT_ASSERT(exp)
#define RTL_SOFT_VERIFYMSG(msg, exp) RTL_SOFT_ASSERTMSG(msg, exp)

#else /* !DBG */

#define ASSERT(exp) ((VOID) 0)
#define ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_SOFT_ASSERT(exp) ((VOID) 0)
#define RTL_SOFT_ASSERTMSG(msg, exp) ((VOID) 0)

#define RTL_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(exp) ((exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, exp) ((exp) ? TRUE : FALSE)

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
  return (OldFlink == OldBlink);
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

#if !defined(_WINBASE_) || _WIN32_WINNT < 0x0501

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(
  IN PSLIST_HEADER  ListHead);

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
  IN PSLIST_HEADER  ListHead,
  IN PSLIST_ENTRY  ListEntry);

#endif

/*
 * USHORT
 * QueryDepthSList(
 *   IN PSLIST_HEADER  SListHead)
 */
#define QueryDepthSList(_SListHead) \
  ((USHORT) ((_SListHead)->Alignment & 0xffff))

#define InterlockedFlushSList(ListHead) ExInterlockedFlushSList(ListHead)

NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
  IN PCANSI_STRING  AnsiString);

#define RtlAnsiStringToUnicodeSize(STRING) (               \
  NLS_MB_CODE_PAGE_TAG ?                                   \
  RtlxAnsiStringToUnicodeSize(STRING) :                    \
  ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)   \
)

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PANSI_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
  IN OUT PUNICODE_STRING  Destination,
  IN PCUNICODE_STRING  Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
  IN OUT PUNICODE_STRING  Destination,
  IN PCWSTR  Source);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  Length);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  Length);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  IN PCSZ  String,
  IN ULONG  Base  OPTIONAL,
  IN OUT PULONG  Value);

#if 0
NTSYSAPI
ULONG
NTAPI
RtlCheckBit(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  BitPosition);
#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
  IN ULONG  RelativeTo,
  IN PWSTR  Path);

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
  IN PRTL_BITMAP  BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlClearBit(
  PRTL_BITMAP  BitMapHeader,
  ULONG  BitNumber);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  NumberToClear);

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
  IN CONST VOID  *Source1,
  IN CONST VOID  *Source2,
  IN SIZE_T  Length);

NTSYSAPI
LONG
NTAPI
RtlCompareString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  BOOLEAN  CaseInSensitive);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
  IN PCUNICODE_STRING  String1,
  IN PCUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(LONG SignedInteger)
{
    LARGE_INTEGER Result;

    Result.QuadPart = SignedInteger;
    return Result;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertUlongToLargeInteger(
  ULONG UnsignedInteger)
{
    LARGE_INTEGER ret;
    ret.QuadPart = UnsignedInteger;
    return ret;
}

NTSYSAPI
LUID
NTAPI
RtlConvertLongToLuid(
  IN LONG  Long);


NTSYSAPI
LUID
NTAPI
RtlConvertUlongToLuid(
  ULONG  Ulong);

#ifdef _M_AMD64

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedIntegerMultiply(
  LARGE_INTEGER Multiplicand,
  LONG Multiplier)
{
    LARGE_INTEGER ret;
    ret.QuadPart = Multiplicand.QuadPart * Multiplier;
    return ret;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlExtendedLargeIntegerDivide(
  LARGE_INTEGER Dividend,
  ULONG Divisor,
  PULONG Remainder)
{
    LARGE_INTEGER ret;
    ret.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
    if (Remainder)
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    return ret;
}

#endif

/*
 * VOID
 * RtlCopyMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN CONST VOID UNALIGNED  *Source,
 *   IN SIZE_T  Length)
 */
#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) \
  memcpy(Destination, Source, Length)
#endif

#ifndef RtlCopyBytes
#define RtlCopyBytes RtlCopyMemory
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
VOID
NTAPI
RtlCopyUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCUNICODE_STRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
  IN ULONG  RelativeTo,
  IN PWSTR  Path);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PCWSTR  ValueName);

/*
 * BOOLEAN
 * RtlEqualLuid(
 *   IN PLUID  Luid1,
 *   IN PLUID  Luid2)
 */
#define RtlEqualLuid(Luid1, \
                     Luid2) \
  (((Luid1)->LowPart == (Luid2)->LowPart) && ((Luid1)->HighPart == (Luid2)->HighPart))

/*
 * ULONG
 * RtlEqualMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN CONST VOID UNALIGNED  *Source,
 *   IN SIZE_T  Length)
 */
#define RtlEqualMemory(Destination, Source, Length) (!memcmp(Destination, Source, Length))

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
  IN CONST UNICODE_STRING  *String1,
  IN CONST UNICODE_STRING  *String2,
  IN BOOLEAN  CaseInSensitive);

/*
 * VOID
 * RtlFillMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN SIZE_T  Length,
 *   IN UCHAR  Fill)
 */
#ifndef RtlFillMemory
#define RtlFillMemory(Destination, Length, Fill) \
  memset(Destination, Fill, Length)
#endif

#ifndef RtlFillBytes
#define RtlFillBytes RtlFillMemory
#endif

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
  IN PRTL_BITMAP  BitMapHeader,
  OUT PRTL_BITMAP_RUN  RunArray,
  IN ULONG  SizeOfRunArray,
  IN BOOLEAN  LocateLongestRuns);

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  OUT PULONG  StartingIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  FromIndex,
  OUT PULONG  StartingRunIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
  IN ULONGLONG  Set);

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  OUT PULONG  StartingIndex);

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
  IN ULONGLONG  Set);

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  FromIndex,
  OUT PULONG  StartingRunIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex);

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex);

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
  IN PANSI_STRING  AnsiString);

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
  IN PUNICODE_STRING  UnicodeString);

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
NTSTATUS
NTAPI
RtlGUIDFromString(
  IN PUNICODE_STRING  GuidString,
  OUT GUID  *Guid);

NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
  IN CONST UNICODE_STRING  *String,
  IN BOOLEAN  CaseInSensitive,
  IN ULONG  HashAlgorithm,
  OUT PULONG  HashValue);

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
  IN OUT PANSI_STRING  DestinationString,
  IN PCSZ  SourceString);

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
  IN PRTL_BITMAP  BitMapHeader,
  IN PULONG  BitMapBuffer,
  IN ULONG  SizeOfBitMap);

NTSYSAPI
VOID
NTAPI
RtlInitString(
  IN OUT PSTRING  DestinationString,
  IN PCSZ  SourceString);

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCWSTR  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
  IN ULONGLONG  Value,
  IN ULONG  Base OPTIONAL,
  IN OUT PUNICODE_STRING  String);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
  IN ULONG  Value,
  IN ULONG  Base  OPTIONAL,
  IN OUT PUNICODE_STRING  String);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntPtrToUnicodeString(
  PLONG  Value,
  ULONG  Base  OPTIONAL,
  PUNICODE_STRING  String);

/*
 * BOOLEAN
 * RtlIsZeroLuid(
 *   IN PLUID  L1)
 */
#define RtlIsZeroLuid(_L1) \
  ((BOOLEAN) ((!(_L1)->LowPart) && (!(_L1)->HighPart)))

NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK  AccessMask,
  IN PGENERIC_MAPPING  GenericMapping);

/*
 * VOID
 * RtlMoveMemory(
 *  IN VOID UNALIGNED  *Destination,
 *  IN CONST VOID UNALIGNED  *Source,
 *  IN SIZE_T  Length)
 */
#define RtlMoveMemory memmove

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
  IN PRTL_BITMAP  BitMapHeader);

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
  IN PRTL_BITMAP  BitMapHeader);

NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
  IN PVOID  Source,
  IN SIZE_T  Length);

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
RtlQueryRegistryValues(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PRTL_QUERY_REGISTRY_TABLE  QueryTable,
  IN PVOID  Context,
  IN PVOID  Environment  OPTIONAL);


#define LONG_SIZE (sizeof(LONG))
#define LONG_MASK (LONG_SIZE - 1)

/*
 * VOID
 * RtlRetrieveUlong (
 *	PULONG	DestinationAddress,
 *	PULONG	SourceAddress
 *	);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
        ((PUCHAR)(DestAddress))[2]=((PUCHAR)(SrcAddress))[2]; \
        ((PUCHAR)(DestAddress))[3]=((PUCHAR)(SrcAddress))[3]; \
    } \
    else \
    { \
        *((PULONG)(DestAddress))=*((PULONG)(SrcAddress)); \
    }

NTSYSAPI
VOID
NTAPI
RtlRetrieveUshort(
  IN OUT PUSHORT  DestinationAddress,
  IN PUSHORT  SourceAddress);

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
  IN PRTL_BITMAP  BitMapHeader);

NTSYSAPI
VOID
NTAPI
RtlSetBit(
  PRTL_BITMAP  BitMapHeader,
  ULONG  BitNumber);

NTSYSAPI
VOID
NTAPI
RtlSetBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  NumberToSet);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN BOOLEAN  DaclPresent,
  IN PACL  Dacl  OPTIONAL,
  IN BOOLEAN  DaclDefaulted  OPTIONAL);

NTSYSAPI
VOID
NTAPI
RtlStoreUlong(
  IN PULONG  Address,
  IN ULONG  Value);

NTSYSAPI
VOID
NTAPI
RtlStoreUlonglong(
  IN OUT PULONGLONG  Address,
  ULONGLONG  Value);

NTSYSAPI
VOID
NTAPI
RtlStoreUlongPtr(
  IN OUT PULONG_PTR  Address,
  IN ULONG_PTR  Value);

NTSYSAPI
VOID
NTAPI
RtlStoreUshort(
  IN PUSHORT  Address,
  IN USHORT  Value);

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
  IN REFGUID  Guid,
  OUT PUNICODE_STRING  GuidString);

NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  BitNumber);

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
  IN PTIME_FIELDS  TimeFields,
  IN PLARGE_INTEGER  Time);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
  IN PLARGE_INTEGER  Time,
  IN PTIME_FIELDS  TimeFields);

ULONG
FASTCALL
RtlUlongByteSwap(
  IN ULONG  Source);

ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
  IN ULONGLONG  Source);

#define RtlUnicodeStringToAnsiSize(STRING) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(STRING) :                     \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

FORCEINLINE
VOID
RtlInitEmptyUnicodeString(OUT PUNICODE_STRING UnicodeString,
                          IN PWSTR Buffer,
                          IN USHORT BufferSize)
{
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferSize;
    UnicodeString->Buffer = Buffer;
}

FORCEINLINE
VOID
RtlInitEmptyAnsiString(OUT PANSI_STRING AnsiString,
                       IN PCHAR Buffer,
                       IN USHORT BufferSize)
{
    AnsiString->Length = 0;
    AnsiString->MaximumLength = BufferSize;
    AnsiString->Buffer = Buffer;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
  IN OUT PANSI_STRING  DestinationString,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
  IN PCUNICODE_STRING  String,
  IN ULONG  Base  OPTIONAL,
  OUT PULONG  Value);

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
  IN WCHAR  SourceCharacter);

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

USHORT
FASTCALL
RtlUshortByteSwap(
  IN USHORT  Source);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
  IN ULONG  SecurityDescriptorLength,
  IN SECURITY_INFORMATION  RequiredInformation);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

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

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PCWSTR  ValueName,
  IN ULONG  ValueType,
  IN PVOID  ValueData,
  IN ULONG  ValueLength);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
  IN PCUNICODE_STRING  UnicodeString);

/*
 * VOID
 * RtlZeroMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN SIZE_T  Length)
 */
#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) \
  memset(Destination, 0, Length)
#endif

#ifndef RtlZeroBytes
#define RtlZeroBytes RtlZeroMemory
#endif

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

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx(
     IN OUT PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx(
     IN OUT PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
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
FASTCALL
ExAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex);

#if defined(_NTHAL_) && defined(_X86_)
NTKERNELAPI
VOID
FASTCALL
ExiAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExiReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
ExiTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);

#define ExAcquireFastMutex(FastMutex)       ExiAcquireFastMutex(FastMutex)
#define ExReleaseFastMutex(FastMutex)       ExiReleaseFastMutex(FastMutex)
#define ExTryToAcquireFastMutex(FastMutex)  ExiTryToAcquireFastMutex(FastMutex)

#else

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);
#endif

/** Executive support routines **/

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceExclusiveLite(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

static __inline PVOID
ExAllocateFromNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses++;
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type, Lookaside->L.Size, Lookaside->L.Tag);
  }
  return Entry;
}

static __inline PVOID
ExAllocateFromPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses++;
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type, Lookaside->L.Size, Lookaside->L.Tag);
  }
  return Entry;
}

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

#ifdef POOL_TAGGING

#define ExAllocatePoolWithQuota(p,n) ExAllocatePoolWithQuotaTag(p,n,' kdD')
#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n,' kdD')

#else /* !POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePool(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuota(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes);

#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag,
  IN EX_POOL_PRIORITY  Priority);

NTKERNELAPI
VOID
NTAPI
ExConvertExclusiveToSharedLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
NTSTATUS
NTAPI
ExCreateCallback(
  OUT PCALLBACK_OBJECT  *CallbackObject,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  Create,
  IN BOOLEAN  AllowMultipleCallbacks);

NTKERNELAPI
VOID
NTAPI
ExDeleteNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside);

NTKERNELAPI
VOID
NTAPI
ExDeletePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside);

NTKERNELAPI
NTSTATUS
NTAPI
ExDeleteResourceLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
VOID
NTAPI
ExFreePool(
  IN PVOID  P);

#define PROTECTED_POOL                    0x80000000

#ifdef POOL_TAGGING
#define ExFreePool(P) ExFreePoolWithTag(P, 0)
#endif

NTKERNELAPI
VOID
NTAPI
ExFreePoolWithTag(
  IN PVOID  P,
  IN ULONG  Tag);

#define ExQueryDepthSList(ListHead) QueryDepthSList(ListHead)

static __inline VOID
ExFreeToNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->L.TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses++;
    (Lookaside->L.Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
  }
}

static __inline VOID
ExFreeToPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->L.TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses++;
    (Lookaside->L.Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
  }
}

/*
 * ERESOURCE_THREAD
 * ExGetCurrentResourceThread(
 *   VOID);
 */
#define ExGetCurrentResourceThread() ((ERESOURCE_THREAD) PsGetCurrentThread())

NTKERNELAPI
ULONG
NTAPI
ExGetExclusiveWaiterCount(
  IN PERESOURCE  Resource);

NTKERNELAPI
KPROCESSOR_MODE
NTAPI
ExGetPreviousMode(
  VOID);

NTKERNELAPI
ULONG
NTAPI
ExGetSharedWaiterCount(
  IN PERESOURCE  Resource);

NTKERNELAPI
VOID
NTAPI
KeInitializeEvent(
  IN PRKEVENT  Event,
  IN EVENT_TYPE  Type,
  IN BOOLEAN  State);

NTKERNELAPI
VOID
NTAPI
ExInitializeNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
  IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
  IN PFREE_FUNCTION  Free  OPTIONAL,
  IN ULONG  Flags,
  IN SIZE_T  Size,
  IN ULONG  Tag,
  IN USHORT  Depth);

NTKERNELAPI
VOID
NTAPI
ExInitializePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside,
  IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
  IN PFREE_FUNCTION  Free  OPTIONAL,
  IN ULONG  Flags,
  IN SIZE_T  Size,
  IN ULONG  Tag,
  IN USHORT  Depth);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeResourceLite(
  IN PERESOURCE  Resource);

/*
 * VOID
 * InitializeSListHead(
 *   IN PSLIST_HEADER  SListHead)
 */
#define InitializeSListHead(_SListHead) \
	(_SListHead)->Alignment = 0

#define ExInitializeSListHead InitializeSListHead

NTKERNELAPI
LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
  IN PLARGE_INTEGER  Addend,
  IN LARGE_INTEGER  Increment,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic(
  IN PLARGE_INTEGER  Addend,
  IN ULONG  Increment);

NTKERNELAPI
ULONG
NTAPI
ExInterlockedAddUlong(
  IN PULONG  Addend,
  IN ULONG  Increment,
  PKSPIN_LOCK  Lock);

NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64(
  IN OUT PLONGLONG  Destination,
  IN PLONGLONG  Exchange,
  IN PLONGLONG  Comparand,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
  IN OUT LONGLONG volatile  *Destination,
  IN PLONGLONG  Exchange,
  IN PLONGLONG  Comperand);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(
  IN PSLIST_HEADER  ListHead);

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedInsertHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedInsertTailList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPopEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock);

/*
 * PSINGLE_LIST_ENTRY
 * ExInterlockedPopEntrySList(
 *   IN PSLIST_HEADER  ListHead,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedPopEntrySList(_ListHead, \
                                   _Lock) \
  InterlockedPopEntrySList(_ListHead)

NTKERNELAPI
PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPushEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PSINGLE_LIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

/*
 * PSINGLE_LIST_ENTRY FASTCALL
 * ExInterlockedPushEntrySList(
 *   IN PSLIST_HEADER  ListHead,
 *   IN PSINGLE_LIST_ENTRY  ListEntry,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedPushEntrySList(_ListHead, \
                                    _ListEntry, \
                                    _Lock) \
  InterlockedPushEntrySList(_ListHead, _ListEntry)

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedRemoveHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsProcessorFeaturePresent(
  IN ULONG  ProcessorFeature);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsResourceAcquiredExclusiveLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
USHORT
NTAPI
ExIsResourceAcquiredLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
ULONG
NTAPI
ExIsResourceAcquiredSharedLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
VOID
NTAPI
ExLocalTimeToSystemTime(
  IN PLARGE_INTEGER  LocalTime,
  OUT PLARGE_INTEGER  SystemTime);

NTKERNELAPI
VOID
NTAPI
ExNotifyCallback(
  IN PCALLBACK_OBJECT  CallbackObject,
  IN PVOID  Argument1,
  IN PVOID  Argument2);

NTKERNELAPI
VOID
NTAPI
__declspec(noreturn)
ExRaiseAccessViolation(
  VOID);

NTKERNELAPI
VOID
NTAPI
__declspec(noreturn)
ExRaiseDatatypeMisalignment(
  VOID);

DECLSPEC_NORETURN
NTKERNELAPI
VOID
NTAPI
__declspec(noreturn)
ExRaiseStatus(
  IN NTSTATUS  Status);

NTKERNELAPI
PVOID
NTAPI
ExRegisterCallback(
  IN PCALLBACK_OBJECT  CallbackObject,
  IN PCALLBACK_FUNCTION  CallbackFunction,
  IN PVOID  CallbackContext);

NTKERNELAPI
NTSTATUS
NTAPI
ExReinitializeResourceLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
VOID
NTAPI
ExReleaseResourceForThreadLite(
  IN PERESOURCE  Resource,
  IN ERESOURCE_THREAD  ResourceThreadId);

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
  IN PERESOURCE  Resource);

NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointer(
  IN PERESOURCE  Resource,
  IN PVOID  OwnerPointer);

NTKERNELAPI
ULONG
NTAPI
ExSetTimerResolution(
  IN ULONG  DesiredTime,
  IN BOOLEAN  SetResolution);

NTKERNELAPI
VOID
NTAPI
ExSystemTimeToLocalTime(
  IN PLARGE_INTEGER  SystemTime,
  OUT PLARGE_INTEGER  LocalTime);

NTKERNELAPI
VOID
NTAPI
ExUnregisterCallback(
  IN PVOID  CbRegistration);

NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  OUT UUID  *Uuid);

NTKERNELAPI
BOOLEAN
NTAPI
ExVerifySuite(
  IN SUITE_TYPE  SuiteType);

#ifdef DBG

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

NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
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
#define IoSetCancelRoutine( Irp, NewCancelRoutine ) (  \
 (PDRIVER_CANCEL)InterlockedExchange( (PLONG)&(Irp)->CancelRoutine, (LONG)(NewCancelRoutine) ) )
    
    
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
VOID
NTAPI
__declspec(noreturn)
KeBugCheck(
  IN ULONG  BugCheckCode);

NTKERNELAPI
VOID
NTAPI
__declspec(noreturn)
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

/*
 * PVOID
 * MmGetSystemAddressForMdlSafe(
 *   IN PMDL  Mdl,
 *   IN MM_PAGE_PRIORITY  Priority)
 */
#define MmGetSystemAddressForMdlSafe(_Mdl, _Priority) \
  (((_Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA \
    | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
    (_Mdl)->MappedSystemVa : \
    (PVOID) MmMapLockedPagesSpecifyCache((_Mdl), \
      KernelMode, MmCached, NULL, FALSE, (_Priority)))

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
ULONG
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
  IN LPCSTR Format,
  IN va_list ap);

ULONG
NTAPI
vDbgPrintExWithPrefix(
  IN LPCSTR Prefix,
  IN ULONG ComponentId,
  IN ULONG Level,
  IN LPCSTR Format,
  IN va_list ap);

NTKERNELAPI
ULONG
DDKCDECLAPI
DbgPrintReturnControlC(
  IN PCH  Format,
  IN ...);

NTKERNELAPI
BOOLEAN
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

#ifdef DBG

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

#if defined(_NTDDK_) || defined(_NTHAL_) || defined(_WDMDDK_) || defined(_NTOSP_)

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

#ifdef __cplusplus
}
#endif

#endif /* __WINDDK_H */
