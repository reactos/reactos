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

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

/*
** Definitions specific to this Device Driver Kit
*/
#define DDKAPI __attribute__((stdcall))
#define DDKFASTAPI __attribute__((fastcall))
#define DDKCDECLAPI __attribute__((cdecl))

#if defined(_NTOSKRNL_)
#ifndef NTOSAPI
#define NTOSAPI DECL_EXPORT
#endif
#define DECLARE_INTERNAL_OBJECT(x) typedef struct _##x; typedef struct _##x *P##x;
#define DECLARE_INTERNAL_OBJECT2(x,y) typedef struct _##x; typedef struct _##x *P##y;
#else
#ifndef NTOSAPI
#define NTOSAPI DECL_IMPORT
#endif
#define DECLARE_INTERNAL_OBJECT(x) struct _##x; typedef struct _##x *P##x;
#define DECLARE_INTERNAL_OBJECT2(x,y) struct _##x; typedef struct _##x *P##y;
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
struct _SECTION_OBJECT;
struct _IO_STATUS_BLOCK;
struct _DEVICE_DESCRIPTION;
struct _SCATTER_GATHER_LIST;

DECLARE_INTERNAL_OBJECT(ADAPTER_OBJECT)
DECLARE_INTERNAL_OBJECT(DMA_ADAPTER)
DECLARE_INTERNAL_OBJECT(IO_STATUS_BLOCK)
DECLARE_INTERNAL_OBJECT(SECTION_OBJECT)

#if 1
/* FIXME: Unknown definitions */
struct _SET_PARTITION_INFORMATION_EX;
typedef ULONG WAIT_TYPE;
typedef HANDLE TRACEHANDLE;
typedef PVOID PWMILIB_CONTEXT;
typedef PVOID PSYSCTL_IRP_DISPOSITION;
typedef ULONG LOGICAL;
#endif

/*
** Routines specific to this DDK
*/

#define TAG(_a, _b, _c, _d) (ULONG) \
	(((_a) << 0) + ((_b) << 8) + ((_c) << 16) + ((_d) << 24))

static __inline struct _KPCR * KeGetCurrentKPCR(
  VOID)
{
  ULONG Value;

  __asm__ __volatile__ ("movl %%fs:0x18, %0\n\t"
	  : "=r" (Value)
    : /* no inputs */
  );
  return (struct _KPCR *) Value;
}

/*
** Simple structures
*/

typedef LONG KPRIORITY;
typedef UCHAR KIRQL, *PKIRQL;
typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;
typedef ULONG KAFFINITY, *PKAFFINITY;
typedef CCHAR KPROCESSOR_MODE;

typedef enum _MODE {
  KernelMode,
  UserMode,
  MaximumMode
} MODE;


/* Structures not exposed to drivers */
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _EPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _COMPRESSED_DATA_INFO *PCOMPRESSED_DATA_INFO;
typedef struct _HAL_DISPATCH_TABLE *PHAL_DISPATCH_TABLE;
typedef struct _HAL_PRIVATE_DISPATCH_TABLE *PHAL_PRIVATE_DISPATCH_TABLE;
typedef struct _DRIVE_LAYOUT_INFORMATION *PDRIVE_LAYOUT_INFORMATION;
typedef struct _DRIVE_LAYOUT_INFORMATION_EX *PDRIVE_LAYOUT_INFORMATION_EX;

/* Constants */
#define	MAXIMUM_PROCESSORS                32

#define MAXIMUM_WAIT_OBJECTS              64

#define METHOD_BUFFERED                   0
#define METHOD_IN_DIRECT                  1
#define METHOD_OUT_DIRECT                 2
#define METHOD_NEITHER                    3

#define LOW_PRIORITY                      0
#define LOW_REALTIME_PRIORITY             16
#define HIGH_PRIORITY                     31
#define MAXIMUM_PRIORITY                  32

#define FILE_SUPERSEDED                   0x00000000
#define FILE_OPENED                       0x00000001
#define FILE_CREATED                      0x00000002
#define FILE_OVERWRITTEN                  0x00000003
#define FILE_EXISTS                       0x00000004
#define FILE_DOES_NOT_EXIST               0x00000005

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

#define DIRECTORY_QUERY (0x0001)
#define DIRECTORY_TRAVERSE (0x0002)
#define DIRECTORY_CREATE_OBJECT (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)
  
/* Exported object types */
extern NTOSAPI POBJECT_TYPE ExDesktopObjectType;
extern NTOSAPI POBJECT_TYPE ExEventObjectType;
extern NTOSAPI POBJECT_TYPE ExSemaphoreObjectType;
extern NTOSAPI POBJECT_TYPE ExWindowStationObjectType;
extern NTOSAPI POBJECT_TYPE IoAdapterObjectType;
extern NTOSAPI ULONG IoDeviceHandlerObjectSize;
extern NTOSAPI POBJECT_TYPE IoDeviceHandlerObjectType;
extern NTOSAPI POBJECT_TYPE IoDeviceObjectType;
extern NTOSAPI POBJECT_TYPE IoDriverObjectType;
extern NTOSAPI POBJECT_TYPE IoFileObjectType;
extern NTOSAPI POBJECT_TYPE LpcPortObjectType;
extern NTOSAPI POBJECT_TYPE MmSectionObjectType;
extern NTOSAPI POBJECT_TYPE SeTokenObjectType;

extern NTOSAPI CCHAR KeNumberProcessors;
extern NTOSAPI PHAL_DISPATCH_TABLE HalDispatchTable;
extern NTOSAPI PHAL_PRIVATE_DISPATCH_TABLE HalPrivateDispatchTable;


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

typedef VOID
(DDKAPI *PDRIVER_LIST_CONTROL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN struct _SCATTER_GATHER_LIST  *ScatterGather,
  IN PVOID  Context);

typedef NTSTATUS
(DDKAPI *PDRIVER_ADD_DEVICE)(
  IN struct _DRIVER_OBJECT  *DriverObject,
  IN struct _DEVICE_OBJECT  *PhysicalDeviceObject);

typedef NTSTATUS
(DDKAPI *PIO_COMPLETION_ROUTINE)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN PVOID  Context);

typedef VOID
(DDKAPI *PDRIVER_CANCEL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);

typedef VOID
(DDKAPI *PKDEFERRED_ROUTINE)(
  IN struct _KDPC  *Dpc,
  IN PVOID  DeferredContext,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

typedef NTSTATUS
(DDKAPI *PDRIVER_DISPATCH)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);

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
(DDKAPI *PDRIVER_INITIALIZE)(
  IN struct _DRIVER_OBJECT  *DriverObject, 
  IN PUNICODE_STRING  RegistryPath);

typedef BOOLEAN
(DDKAPI *PKSERVICE_ROUTINE)(
  IN struct _KINTERRUPT  *Interrupt,
  IN PVOID  ServiceContext);

typedef VOID
(DDKAPI *PIO_TIMER_ROUTINE)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN PVOID  Context);

typedef VOID
(DDKAPI *PDRIVER_REINITIALIZE)( 
  IN struct _DRIVER_OBJECT  *DriverObject, 
  IN PVOID  Context, 
  IN ULONG  Count); 

typedef NTSTATUS
(DDKAPI *PDRIVER_STARTIO)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);

typedef BOOLEAN
(DDKAPI *PKSYNCHRONIZE_ROUTINE)(
  IN PVOID  SynchronizeContext);

typedef VOID
(DDKAPI *PDRIVER_UNLOAD)( 
  IN struct _DRIVER_OBJECT  *DriverObject); 



/*
** Plug and Play structures
*/

typedef VOID DDKAPI
(*PINTERFACE_REFERENCE)(
  PVOID  Context);

typedef VOID DDKAPI
(*PINTERFACE_DEREFERENCE)(
  PVOID Context);

typedef BOOLEAN DDKAPI
(*PTRANSLATE_BUS_ADDRESS)(
  IN PVOID  Context,
  IN PHYSICAL_ADDRESS  BusAddress,
  IN ULONG  Length,
  IN OUT PULONG  AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);

typedef struct _DMA_ADAPTER* DDKAPI
(*PGET_DMA_ADAPTER)(
  IN PVOID  Context,
  IN struct _DEVICE_DESCRIPTION  *DeviceDescriptor,
  OUT PULONG  NumberOfMapRegisters);

typedef ULONG DDKAPI
(*PGET_SET_DEVICE_DATA)(
  IN PVOID  Context,
  IN ULONG  DataType,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

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

typedef NTSTATUS DDKAPI
(*PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)(
  IN PVOID NotificationStructure,
  IN PVOID Context);

typedef VOID DDKAPI
(*PDEVICE_CHANGE_COMPLETE_CALLBACK)(
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

typedef VOID DDKAPI
(*PIO_APC_ROUTINE)(
  IN PVOID ApcContext,
  IN PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG Reserved);

typedef struct _IO_STATUS_BLOCK {
  _ANONYMOUS_UNION union {
    NTSTATUS  Status;
    PVOID  Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR  Information;
} IO_STATUS_BLOCK;

typedef VOID DDKAPI
(*PKNORMAL_ROUTINE)(
  IN PVOID  NormalContext,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

typedef VOID DDKAPI
(*PKKERNEL_ROUTINE)(
  IN struct _KAPC  *Apc,
  IN OUT PKNORMAL_ROUTINE  *NormalRoutine,
  IN OUT PVOID  *NormalContext,
  IN OUT PVOID  *SystemArgument1,
  IN OUT PVOID  *SystemArgument2);

typedef VOID DDKAPI
(*PKRUNDOWN_ROUTINE)(
  IN struct _KAPC  *Apc);

typedef BOOLEAN DDKAPI
(*PKTRANSFER_ROUTINE)(
  VOID);

typedef struct _KAPC {
  CSHORT  Type;
  CSHORT  Size;
  ULONG  Spare0;
  struct _KTHREAD  *Thread;
  LIST_ENTRY  ApcListEntry;
  PKKERNEL_ROUTINE  KernelRoutine;
  PKRUNDOWN_ROUTINE  RundownRoutine;
  PKNORMAL_ROUTINE  NormalRoutine;
  PVOID  NormalContext;
  PVOID  SystemArgument1;
  PVOID  SystemArgument2;
  CCHAR  ApcStateIndex;
  KPROCESSOR_MODE  ApcMode;
  BOOLEAN  Inserted;
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

typedef enum _KSPIN_LOCK_QUEUE_NUMBER {
  LockQueueDispatcherLock,
  LockQueueContextSwapLock,
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
  LockQueueMaximumLock
} KSPIN_LOCK_QUEUE_NUMBER, *PKSPIN_LOCK_QUEUE_NUMBER;

typedef struct _KSPIN_LOCK_QUEUE {
  struct _KSPIN_LOCK_QUEUE  *VOLATILE Next;
  PKSPIN_LOCK VOLATILE  Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
  KSPIN_LOCK_QUEUE  LockQueue;
  KIRQL  OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

typedef struct _KDPC {
  CSHORT  Type;
  UCHAR  Number;
  UCHAR  Importance;
  LIST_ENTRY  DpcListEntry;
  PKDEFERRED_ROUTINE  DeferredRoutine;
  PVOID  DeferredContext;
  PVOID  SystemArgument1;
  PVOID  SystemArgument2;
  PULONG_PTR  Lock;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;

typedef struct _WAIT_CONTEXT_BLOCK {
  KDEVICE_QUEUE_ENTRY  WaitQueueEntry;
  struct _DRIVER_CONTROL  *DeviceRoutine;
  PVOID  DeviceContext;
  ULONG  NumberOfMapRegisters;
  PVOID  DeviceObject;
  PVOID  CurrentIrp;
  PKDPC  BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

typedef struct _DISPATCHER_HEADER {
  UCHAR  Type;
  UCHAR  Absolute;
  UCHAR  Size;
  UCHAR  Inserted;
  LONG  SignalState;
  LIST_ENTRY  WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef struct _KEVENT {
  DISPATCHER_HEADER  Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

typedef struct _FAST_MUTEX {
  LONG  Count;
  struct _KTHREAD  *Owner;
  ULONG  Contention;
  KEVENT  Event;
  ULONG  OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef struct _KTIMER {
  DISPATCHER_HEADER  Header;
  ULARGE_INTEGER  DueTime;
  LIST_ENTRY  TimerListEntry;
  struct _KDPC  *Dpc;
  LONG  Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;

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

typedef struct _IRP {
  CSHORT  Type;
  USHORT  Size;
  struct _MDL  *MdlAddress;
  ULONG  Flags;
  union {
    struct _IRP  *MasterIrp;
    LONG  IrpCount;
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
  PDRIVER_CANCEL  CancelRoutine;
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


typedef struct _DRIVE_LAYOUT_INFORMATION_MBR {
  ULONG  Signature;
} DRIVE_LAYOUT_INFORMATION_MBR, *PDRIVE_LAYOUT_INFORMATION_MBR;

typedef struct _DRIVE_LAYOUT_INFORMATION_GPT {
  GUID  DiskId;
  LARGE_INTEGER  StartingUsableOffset;
  LARGE_INTEGER  UsableLength;
  ULONG  MaxPartitionCount;
} DRIVE_LAYOUT_INFORMATION_GPT, *PDRIVE_LAYOUT_INFORMATION_GPT;

typedef struct _PARTITION_INFORMATION_MBR {
  UCHAR  PartitionType;
  BOOLEAN  BootIndicator;
  BOOLEAN  RecognizedPartition;
  ULONG  HiddenSectors;
} PARTITION_INFORMATION_MBR, *PPARTITION_INFORMATION_MBR;


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

typedef struct _EISA_MEMORY_CONFIGURATION {
  EISA_MEMORY_TYPE  ConfigurationByte;
  UCHAR  DataSize;
  USHORT  AddressLowWord;
  UCHAR  AddressHighByte;
  USHORT  MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;

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
      ULONG Affinity;
    } Interrupt;
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

typedef struct _CM_KEYBOARD_DEVICE_DATA {
  USHORT  Version;
  USHORT  Revision;
  UCHAR  Type;
  UCHAR  Subtype;
  USHORT  KeyboardFlags;
} CM_KEYBOARD_DEVICE_DATA, *PCM_KEYBOARD_DEVICE_DATA;

#define KEYBOARD_INSERT_ON                0x80
#define KEYBOARD_CAPS_LOCK_ON             0x40
#define KEYBOARD_NUM_LOCK_ON              0x20
#define KEYBOARD_SCROLL_LOCK_ON           0x10
#define KEYBOARD_ALT_KEY_DOWN             0x08
#define KEYBOARD_CTRL_KEY_DOWN            0x04
#define KEYBOARD_LEFT_SHIFT_DOWN          0x02
#define KEYBOARD_RIGHT_SHIFT_DOWN         0x01

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
#define FILE_DEVICE_FIPS		              0x0000003a

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
  PVPB  Vpb;
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
} DEVICE_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;

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
  SCATTER_GATHER_ELEMENT  Elements[0];
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

#define MDL_MAPPED_TO_SYSTEM_VA           0x0001
#define MDL_PAGES_LOCKED                  0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL       0x0004
#define MDL_ALLOCATED_FIXED_SIZE          0x0008
#define MDL_PARTIAL                       0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED       0x0020
#define MDL_IO_PAGE_READ                  0x0040
#define MDL_WRITE_OPERATION               0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA       0x0100
#define MDL_FREE_EXTRA_PTES               0x0200
#define MDL_IO_SPACE                      0x0800
#define MDL_NETWORK_HEADER                0x1000
#define MDL_MAPPING_CAN_FAIL              0x2000
#define MDL_ALLOCATED_MUST_SUCCEED        0x4000

#define MDL_MAPPING_FLAGS ( \
  MDL_MAPPED_TO_SYSTEM_VA     | \
  MDL_PAGES_LOCKED            | \
  MDL_SOURCE_IS_NONPAGED_POOL | \
  MDL_PARTIAL_HAS_BEEN_MAPPED | \
  MDL_PARENT_MAPPED_SYSTEM_VA | \
  MDL_SYSTEM_VA               | \
  MDL_IO_SPACE)

typedef VOID DDKAPI
(*PPUT_DMA_ADAPTER)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef PVOID DDKAPI
(*PALLOCATE_COMMON_BUFFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN ULONG  Length,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

typedef VOID DDKAPI
(*PFREE_COMMON_BUFFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

typedef NTSTATUS DDKAPI
(*PALLOCATE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

typedef BOOLEAN DDKAPI
(*PFLUSH_ADAPTER_BUFFERS)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN BOOLEAN  WriteToDevice);

typedef VOID DDKAPI
(*PFREE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef VOID DDKAPI
(*PFREE_MAP_REGISTERS)(
  IN PDMA_ADAPTER  DmaAdapter,
  PVOID  MapRegisterBase,
  ULONG  NumberOfMapRegisters);

typedef PHYSICAL_ADDRESS DDKAPI
(*PMAP_TRANSFER)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN OUT PULONG  Length,
  IN BOOLEAN  WriteToDevice);

typedef ULONG DDKAPI
(*PGET_DMA_ALIGNMENT)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef ULONG DDKAPI
(*PREAD_DMA_COUNTER)(
  IN PDMA_ADAPTER  DmaAdapter);

typedef NTSTATUS DDKAPI
(*PGET_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PMDL  Mdl,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN PDRIVER_LIST_CONTROL  ExecutionRoutine,
  IN PVOID  Context,
  IN BOOLEAN  WriteToDevice);

typedef VOID DDKAPI
(*PPUT_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PSCATTER_GATHER_LIST  ScatterGather,
  IN BOOLEAN  WriteToDevice);

typedef NTSTATUS DDKAPI
(*PCALCULATE_SCATTER_GATHER_LIST_SIZE)(
  IN PDMA_ADAPTER  DmaAdapter,
  IN PMDL  Mdl  OPTIONAL,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  OUT PULONG  ScatterGatherListSize,
  OUT PULONG  pNumberOfMapRegisters  OPTIONAL);

typedef NTSTATUS DDKAPI
(*PBUILD_SCATTER_GATHER_LIST)(
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

typedef NTSTATUS DDKAPI
(*PBUILD_MDL_FROM_SCATTER_GATHER_LIST)(
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

typedef struct _DMA_ADAPTER {
  USHORT  Version;
  USHORT  Size;
  PDMA_OPERATIONS  DmaOperations;
} DMA_ADAPTER;

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
} FILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {                     
  ULONG  FileNameLength;                                   
  WCHAR  FileName[1];                                      
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;           

typedef struct FILE_BASIC_INFORMATION {
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  ULONG  FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

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
  BOOLEAN  DoDeleteFile;                                         
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; 
                                                                
typedef struct _FILE_END_OF_FILE_INFORMATION {                  
  LARGE_INTEGER  EndOfFile;                                    
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION; 
                                                                
typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {                                    
  LARGE_INTEGER  ValidDataLength;                                                      
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;             

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

typedef struct _ERESOURCE {
  LIST_ENTRY  SystemResourcesList;
  POWNER_ENTRY  OwnerTable;
  SHORT  ActiveCount;
  USHORT  Flag;
  PKSEMAPHORE  SharedWaiters;
  PKEVENT  ExclusiveWaiters;
  OWNER_ENTRY  OwnerThreads[2];
  ULONG  ContentionCount;
  USHORT  NumberOfSharedWaiters;
  USHORT  NumberOfExclusiveWaiters;
  _ANONYMOUS_UNION union {
    PVOID  Address;
    ULONG_PTR  CreatorBackTraceIndex;
  } DUMMYUNIONNAME;
  KSPIN_LOCK  SpinLock;
} ERESOURCE, *PERESOURCE;

/* NOTE: PVOID for methods to avoid 'assignment from incompatible pointer type' warning */
typedef struct _DRIVER_EXTENSION {
  struct _DRIVER_OBJECT  *DriverObject;
  PVOID  AddDevice;
  ULONG  Count;
  UNICODE_STRING  ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

typedef BOOLEAN DDKAPI
(*PFAST_IO_CHECK_IF_POSSIBLE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  IN BOOLEAN  CheckForReadOperation,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_READ)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  OUT PVOID  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN BOOLEAN  Wait,
  IN ULONG  LockKey,
  IN PVOID  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_QUERY_BASIC_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT PFILE_BASIC_INFORMATION  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_QUERY_STANDARD_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT PFILE_STANDARD_INFORMATION  Buffer,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_LOCK)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PLARGE_INTEGER  Length,
  PEPROCESS  ProcessId,
  ULONG  Key,
  BOOLEAN  FailImmediately,
  BOOLEAN  ExclusiveLock,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_UNLOCK_SINGLE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PLARGE_INTEGER  Length,
  PEPROCESS  ProcessId,
  ULONG  Key,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_UNLOCK_ALL)(
  IN struct _FILE_OBJECT  *FileObject,
  PEPROCESS  ProcessId,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_UNLOCK_ALL_BY_KEY)(
  IN struct _FILE_OBJECT  *FileObject,
  PVOID  ProcessId,
  ULONG  Key,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_DEVICE_CONTROL)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  IN PVOID  InputBuffer  OPTIONAL,
  IN ULONG  InputBufferLength,
  OUT PVOID  OutputBuffer  OPTIONAL,
  IN ULONG  OutputBufferLength,
  IN ULONG  IoControlCode,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef VOID DDKAPI
(*PFAST_IO_ACQUIRE_FILE)(
  IN struct _FILE_OBJECT  *FileObject);

typedef VOID DDKAPI
(*PFAST_IO_RELEASE_FILE)(
  IN struct _FILE_OBJECT  *FileObject);

typedef VOID DDKAPI
(*PFAST_IO_DETACH_DEVICE)(
  IN struct _DEVICE_OBJECT  *SourceDevice,
  IN struct _DEVICE_OBJECT  *TargetDevice);

typedef BOOLEAN DDKAPI
(*PFAST_IO_QUERY_NETWORK_OPEN_INFO)(
  IN struct _FILE_OBJECT  *FileObject,
  IN BOOLEAN  Wait,
  OUT struct _FILE_NETWORK_OPEN_INFORMATION  *Buffer,
  OUT struct _IO_STATUS_BLOCK  *IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS DDKAPI
(*PFAST_IO_ACQUIRE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  EndingOffset,
  OUT struct _ERESOURCE  **ResourceToRelease,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_MDL_READ)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_MDL_READ_COMPLETE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_PREPARE_MDL_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN ULONG  Length,
  IN ULONG  LockKey,
  OUT PMDL  *MdlChain,
  OUT PIO_STATUS_BLOCK  IoStatus,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_MDL_WRITE_COMPLETE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_READ_COMPRESSED)(
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

typedef BOOLEAN DDKAPI
(*PFAST_IO_WRITE_COMPRESSED)(
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

typedef BOOLEAN DDKAPI
(*PFAST_IO_MDL_READ_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT  *FileObject,
  IN PLARGE_INTEGER  FileOffset,
  IN PMDL  MdlChain,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef BOOLEAN DDKAPI
(*PFAST_IO_QUERY_OPEN)(
  IN struct _IRP  *Irp,
  OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS DDKAPI
(*PFAST_IO_RELEASE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT  *FileObject,
  IN struct _ERESOURCE  *ResourceToRelease,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS DDKAPI
(*PFAST_IO_ACQUIRE_FOR_CCFLUSH)(
  IN struct _FILE_OBJECT  *FileObject,
  IN struct _DEVICE_OBJECT  *DeviceObject);

typedef NTSTATUS DDKAPI
(*PFAST_IO_RELEASE_FOR_CCFLUSH) (
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
  PFAST_IO_DISPATCH  FastIoDispatch;
  PDRIVER_INITIALIZE  DriverInit;
  PDRIVER_STARTIO  DriverStartIo;
  PDRIVER_UNLOAD  DriverUnload;
  PDRIVER_DISPATCH  MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;

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
#define FO_FILE_OBJECT_HAS_EXTENSION      0x00800000
#define FO_REMOTE_ORIGIN                  0x01000000

typedef struct _FILE_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PDEVICE_OBJECT  DeviceObject;
  PVPB  Vpb;
  PVOID  FsContext;
  PVOID  FsContext2;
  PSECTION_OBJECT_POINTERS  SectionObjectPointer;
  PVOID  PrivateCacheMap;
  NTSTATUS  FinalStatus;
  struct _FILE_OBJECT  *RelatedFileObject;
  BOOLEAN  LockOperation;
  BOOLEAN  DeletePending;
  BOOLEAN  ReadAccess;
  BOOLEAN  WriteAccess;
  BOOLEAN  DeleteAccess;
  BOOLEAN  SharedRead;
  BOOLEAN  SharedWrite;
  BOOLEAN  SharedDelete;
  ULONG  Flags;
  UNICODE_STRING  FileName;
  LARGE_INTEGER  CurrentByteOffset;
  ULONG  Waiters;
  ULONG  Busy;
  PVOID  LastLock;
  KEVENT  Lock;
  KEVENT  Event;
  PIO_COMPLETION_CONTEXT  CompletionContext;
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

typedef struct _SECURITY_SUBJECT_CONTEXT {
  PACCESS_TOKEN  ClientToken;
  SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel;
  PACCESS_TOKEN  PrimaryToken;
  PVOID  ProcessAuditId;
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

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

typedef struct _IO_SECURITY_CONTEXT {
  PSECURITY_QUALITY_OF_SERVICE  SecurityQos;
  PACCESS_STATE  AccessState;
  ACCESS_MASK  DesiredAccess;
  ULONG  FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;

struct _IO_CSQ;

typedef struct _IO_CSQ_IRP_CONTEXT {
  ULONG  Type;
  struct _IRP  *Irp;
  struct _IO_CSQ  *Csq;
} IO_CSQ_IRP_CONTEXT, *PIO_CSQ_IRP_CONTEXT;

typedef VOID DDKAPI
(*PIO_CSQ_INSERT_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp);

typedef VOID DDKAPI
(*PIO_CSQ_REMOVE_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp);

typedef PIRP DDKAPI
(*PIO_CSQ_PEEK_NEXT_IRP)(
  IN struct _IO_CSQ  *Csq,
  IN PIRP  Irp,
  IN PVOID  PeekContext);

typedef VOID DDKAPI
(*PIO_CSQ_ACQUIRE_LOCK)(
  IN  struct _IO_CSQ  *Csq,
  OUT PKIRQL  Irql);

typedef VOID DDKAPI
(*PIO_CSQ_RELEASE_LOCK)(
  IN struct _IO_CSQ  *Csq,
  IN KIRQL  Irql);

typedef VOID DDKAPI
(*PIO_CSQ_COMPLETE_CANCELED_IRP)(
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
      FS_INFORMATION_CLASS POINTER_ALIGNMENT  FsInformationClass;
    } QueryVolume;
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

/* IO_STACK_LOCATION.Control */

#define SL_PENDING_RETURNED               0x01
#define SL_INVOKE_ON_CANCEL               0x20
#define SL_INVOKE_ON_SUCCESS              0x40
#define SL_INVOKE_ON_ERROR                0x80

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

/* PRIVILEGE_SET.Control */

#define PRIVILEGE_SET_ALL_NECESSARY       1

typedef struct _RTL_OSVERSIONINFOW {
  ULONG  dwOSVersionInfoSize;
  ULONG  dwMajorVersion;
  ULONG  dwMinorVersion;
  ULONG  dwBuildNumber;
  ULONG  dwPlatformId;
  WCHAR  szCSDVersion[128];
} RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

typedef struct _RTL_OSVERSIONINFOEXW {
  ULONG  dwOSVersionInfoSize;
  ULONG  dwMajorVersion;
  ULONG  dwMinorVersion;
  ULONG  dwBuildNumber;
  ULONG  dwPlatformId;
  WCHAR  szCSDVersion[128];
  USHORT  wServicePackMajor;
  USHORT  wServicePackMinor;
  USHORT  wSuiteMask;
  UCHAR  wProductType;
  UCHAR  wReserved;
} RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

NTOSAPI
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

typedef struct _RTL_BITMAP_RUN {
    ULONG  StartingIndex;
    ULONG  NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef NTSTATUS DDKAPI
(*PRTL_QUERY_REGISTRY_ROUTINE)(
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
  PWSTR  Name;
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

typedef PVOID DDKAPI
(*PALLOCATE_FUNCTION)(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

typedef VOID DDKAPI
(*PFREE_FUNCTION)(
  IN PVOID  Buffer);

#define GENERAL_LOOKASIDE_S \
  SLIST_HEADER  ListHead; \
  USHORT  Depth; \
  USHORT  MaximumDepth; \
  ULONG  TotalAllocates; \
  _ANONYMOUS_UNION union { \
    ULONG  AllocateMisses; \
    ULONG  AllocateHits; \
  } DUMMYUNIONNAME; \
  ULONG  TotalFrees; \
  _ANONYMOUS_UNION union { \
    ULONG  FreeMisses; \
    ULONG  FreeHits; \
  } DUMMYUNIONNAME2; \
  POOL_TYPE  Type; \
  ULONG  Tag; \
  ULONG  Size; \
  PALLOCATE_FUNCTION  Allocate; \
  PFREE_FUNCTION  Free; \
  LIST_ENTRY  ListEntry; \
  ULONG  LastTotalAllocates; \
  _ANONYMOUS_UNION union { \
    ULONG  LastAllocateMisses; \
    ULONG  LastAllocateHits; \
  } DUMMYUNIONNAME3; \
  ULONG Future[2];

typedef struct _GENERAL_LOOKASIDE {
  GENERAL_LOOKASIDE_S
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _NPAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE_S
  KSPIN_LOCK  Obsoleted;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct _PAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE_S
  FAST_MUTEX  Obsoleted;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID DDKAPI (*PCALLBACK_FUNCTION)(
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
  Spare3,
  Spare4,
  Spare5,
  Spare6,
  WrKernel,
  MaximumWaitReason
} KWAIT_REASON;

typedef struct _KWAIT_BLOCK {
  LIST_ENTRY  WaitListEntry;
  struct _KTHREAD * RESTRICTED_POINTER  Thread;
  PVOID  Object;
  struct _KWAIT_BLOCK * RESTRICTED_POINTER  NextWaitBlock;
  USHORT  WaitKey;
  USHORT  WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK, *RESTRICTED_POINTER PRKWAIT_BLOCK;

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK * PIO_REMOVE_LOCK_TRACKING_BLOCK;

typedef struct _IO_REMOVE_LOCK_COMMON_BLOCK {
  BOOLEAN  Removed;
  BOOLEAN  Reserved[3];
  LONG  IoCount;
  KEVENT  RemoveEvent;
} IO_REMOVE_LOCK_COMMON_BLOCK;

typedef struct _IO_REMOVE_LOCK_DBG_BLOCK {
  LONG  Signature;
  LONG  HighWatermark;
  LONGLONG  MaxLockedTicks;
  LONG  AllocateTag;
  LIST_ENTRY  LockList;
  KSPIN_LOCK  Spin;
  LONG  LowMemoryCount;
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

typedef VOID DDKAPI
(*PIO_WORKITEM_ROUTINE)(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PVOID  Context);

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

typedef VOID DDKAPI
(*PKINTERRUPT_ROUTINE)(
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
  MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

typedef NTSTATUS (*PIO_QUERY_DEVICE_ROUTINE)(
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

typedef enum _WORK_QUEUE_TYPE {
  CriticalWorkQueue,
  DelayedWorkQueue,
  HyperCriticalWorkQueue,
  MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef VOID DDKAPI
(*PWORKER_THREAD_ROUTINE)(
  IN PVOID Parameter);

typedef struct _WORK_QUEUE_ITEM {
  LIST_ENTRY  List;
  PWORKER_THREAD_ROUTINE  WorkerRoutine;
  PVOID  Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
    BufferEmpty,
    BufferInserted,
    BufferStarted,
    BufferFinished,
    BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;

typedef VOID DDKAPI
(*PKBUGCHECK_CALLBACK_ROUTINE)(
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

typedef enum _MM_SYSTEM_SIZE {
  MmSmallSystem,
  MmMediumSystem,
  MmLargeSystem
} MM_SYSTEM_SIZE;

typedef struct _OBJECT_HANDLE_INFORMATION {
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

typedef struct _CLIENT_ID {
  HANDLE  UniqueProcess;
  HANDLE  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef VOID DDKAPI
(*PKSTART_ROUTINE)(
  IN PVOID  StartContext);

typedef VOID DDKAPI
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(
  IN HANDLE  ParentId,
  IN HANDLE  ProcessId,
  IN BOOLEAN  Create);

typedef VOID DDKAPI
(*PCREATE_THREAD_NOTIFY_ROUTINE)(
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

typedef VOID DDKAPI
(*PLOAD_IMAGE_NOTIFY_ROUTINE)(
  IN PUNICODE_STRING  FullImageName,
  IN HANDLE  ProcessId,
  IN PIMAGE_INFO  ImageInfo);

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
  MaxThreadInfoClass
} THREADINFOCLASS;

#define ES_SYSTEM_REQUIRED                0x00000001
#define ES_DISPLAY_REQUIRED               0x00000002
#define ES_USER_PRESENT                   0x00000004
#define ES_CONTINUOUS                     0x80000000

typedef ULONG EXECUTION_STATE;

typedef VOID DDKAPI
(*PREQUEST_POWER_COMPLETE)(
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

typedef NTSTATUS DDKAPI
(*PEX_CALLBACK_FUNCTION)(
  IN PVOID  CallbackContext,
  IN PVOID  Argument1,
  IN PVOID  Argument2);



/*
** Storage structures
*/
typedef enum _PARTITION_STYLE {
  PARTITION_STYLE_MBR,
  PARTITION_STYLE_GPT
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

typedef VOID DDKFASTAPI
(*PTIME_UPDATE_NOTIFY_ROUTINE)(
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

#define RTL_RANGE_LIST_ADD_IF_CONFLICT    0x00000001
#define RTL_RANGE_LIST_ADD_SHARED         0x00000002

#define RTL_RANGE_LIST_SHARED_OK          0x00000001
#define RTL_RANGE_LIST_NULL_CONFLICT_OK   0x00000002

#define RTL_RANGE_LIST_SHARED_OK          0x00000001
#define RTL_RANGE_LIST_NULL_CONFLICT_OK   0x00000002

#define RTL_RANGE_LIST_MERGE_IF_CONFLICT  RTL_RANGE_LIST_ADD_IF_CONFLICT

typedef struct _RTL_RANGE {
  ULONGLONG  Start;
  ULONGLONG  End;
  PVOID  UserData;
  PVOID  Owner;
  UCHAR  Attributes;
  UCHAR  Flags;
} RTL_RANGE, *PRTL_RANGE;

#define RTL_RANGE_SHARED                  0x01
#define RTL_RANGE_CONFLICT                0x02

typedef struct _RTL_RANGE_LIST {
  LIST_ENTRY  ListHead;
  ULONG  Flags;
  ULONG  Count;
  ULONG  Stamp;
} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

typedef struct _RANGE_LIST_ITERATOR {
  PLIST_ENTRY  RangeListHead;
  PLIST_ENTRY  MergedHead;
  PVOID  Current;
  ULONG  Stamp;
} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;

typedef BOOLEAN
(*PRTL_CONFLICT_RANGE_CALLBACK)(
  IN PVOID  Context,
  IN PRTL_RANGE  Range);

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

typedef VOID DDKAPI
(*PTIMER_APC_ROUTINE)(
  IN PVOID  TimerContext,
  IN ULONG  TimerLowValue,
  IN LONG  TimerHighValue);



/*
** WMI structures
*/

typedef VOID DDKAPI
(*WMI_NOTIFICATION_CALLBACK)(
  PVOID  Wnode,
  PVOID  Context);


/*
** Architecture specific structures
*/

#ifdef _X86_

typedef ULONG PFN_NUMBER, *PPFN_NUMBER;

#define PASSIVE_LEVEL                      0
#define LOW_LEVEL                          0
#define APC_LEVEL                          1
#define DISPATCH_LEVEL                     2
#define SYNCH_LEVEL                       27
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
    DWORD  Version;             /* 10 */
  } DUMMYUNIONNAME;
  PVOID  ArbitraryUserPointer;  /* 14 */
} KPCR_TIB, *PKPCR_TIB;         /* 18 */

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {
  KPCR_TIB  Tib;                /* 00 */
  struct _KPCR  *Self;          /* 18 */
  struct _KPRCB  *PCRCB;        /* 1C */
  KIRQL  Irql;                  /* 20 */
  ULONG  IRR;                   /* 24 */
  ULONG  IrrActive;             /* 28 */
  ULONG  IDR;                   /* 2C */
  PVOID  KdVersionBlock;        /* 30 */
  PUSHORT  IDT;                 /* 34 */
  PUSHORT  GDT;                 /* 38 */
  struct _KTSS  *TSS;           /* 3C */
  USHORT  MajorVersion;         /* 40 */
  USHORT  MinorVersion;         /* 42 */
  KAFFINITY  SetMember;         /* 44 */
  ULONG  StallScaleFactor;      /* 48 */
  UCHAR  DebugActive;           /* 4C */
  UCHAR  ProcessorNumber;       /* 4D */
  UCHAR  Reserved[2];           /* 4E */
} KPCR, *PKPCR;                 /* 50 */

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

#define PAGE_SIZE                         0x1000
#define PAGE_SHIFT                        12L

extern NTOSAPI PVOID *MmHighestUserAddress;
extern NTOSAPI PVOID *MmSystemRangeStart;
extern NTOSAPI ULONG *MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS           *MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             *MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             *MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS            (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS          (PVOID)0xC0C00000

#define KI_USER_SHARED_DATA               0xffdf0000
#define SharedUserData                    ((KUSER_SHARED_DATA * CONST) KI_USER_SHARED_DATA)

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

NTOSAPI
KIRQL
DDKAPI
KeGetCurrentIrql(
  VOID);

/*
 * ULONG
 * KeGetCurrentProcessorNumber(
 *   VOID)
 */
#define KeGetCurrentProcessorNumber() \
  ((ULONG)KeGetCurrentKPCR()->ProcessorNumber)

#if !defined(__INTERLOCKED_DECLARED)
#define __INTERLOCKED_DECLARED

NTOSAPI
LONG
DDKFASTAPI
InterlockedIncrement(
  IN PLONG  VOLATILE  Addend);

NTOSAPI
LONG
DDKFASTAPI
InterlockedDecrement(
  IN PLONG  VOLATILE  Addend);

NTOSAPI
LONG
DDKFASTAPI
InterlockedCompareExchange(
  IN OUT PLONG  VOLATILE  Destination,
  IN LONG  Exchange,
  IN LONG  Comparand);

NTOSAPI
LONG
DDKFASTAPI
InterlockedExchange(
  IN OUT PLONG  VOLATILE  Target,
  IN LONG Value);

NTOSAPI
LONG
DDKFASTAPI
InterlockedExchangeAdd(
  IN OUT PLONG VOLATILE  Addend,
  IN LONG  Value);

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

#endif /* !__INTERLOCKED_DECLARED */

NTOSAPI
VOID
DDKFASTAPI
KefAcquireSpinLockAtDpcLevel(
  IN PKSPIN_LOCK  SpinLock);

NTOSAPI
VOID
DDKFASTAPI
KefReleaseSpinLockFromDpcLevel(
  IN PKSPIN_LOCK  SpinLock);

#define KeAcquireSpinLockAtDpcLevel(SpinLock) KefAcquireSpinLockAtDpcLevel(SpinLock)
#define KeReleaseSpinLockFromDpcLevel(SpinLock) KefReleaseSpinLockFromDpcLevel(SpinLock)

#define RtlCopyMemoryNonTemporal RtlCopyMemory

#define KeGetDcacheFillSize() 1L

#endif /* _X86_ */



/*
** Utillity functions
*/

#define ARGUMENT_PRESENT(ArgumentPointer) \
  ((BOOLEAN) ((PVOID)ArgumentPointer != (PVOID)NULL))

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
 * PCHAR
 * CONTAINING_RECORD(
 *   IN PCHAR  Address,
 *   IN TYPE  Type,
 *   IN PCHAR  Field);
 */
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(Address, Type, Field) \
  ((Type *) (((ULONG_PTR) Address) - FIELD_OFFSET(Type, Field)))
#endif

/* LONG
 * FIELD_OFFSET(
 *   IN TYPE  Type,
 *   IN PCHAR  Field);
 */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(Type, Field) \
  ((LONG) (&(((Type *) 0)->Field)))
#endif

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

NTOSAPI
VOID
DDKAPI
RtlAssert(
  IN PVOID  FailedAssertion,
  IN PVOID  FileName,
  IN ULONG  LineNumber,
  IN PCHAR  Message);

#ifdef DBG

#define ASSERT(exp) \
  ((!(exp)) ? \
    (RtlAssert( #exp, __FILE__, __LINE__, NULL ), FALSE) : TRUE)

#define ASSERTMSG(msg, exp) \
  ((!(exp)) ? \
    (RtlAssert( #exp, __FILE__, __LINE__, msg ), FALSE) : TRUE)

#define RTL_SOFT_ASSERT(exp) \
  ((!(_exp)) ? \
    (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #exp), FALSE) : TRUE)

#define RTL_SOFT_ASSERTMSG(msg, exp) \
  ((!(exp)) ? \
    (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #exp, (msg)), FALSE) : TRUE)

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


/*
** Driver support routines
*/

/** Runtime library routines **/

/*
 * VOID
 * InitializeListHead(
 *   IN PLIST_ENTRY  ListHead)
 */
#define InitializeListHead(_ListHead) \
{ \
  (_ListHead)->Flink = (_ListHead); \
  (_ListHead)->Blink = (_ListHead); \
}

/*
 * VOID
 * InsertHeadList(
 *   IN PLIST_ENTRY  ListHead,
 *   IN PLIST_ENTRY  Entry)
 */
#define InsertHeadList(_ListHead, \
                       _Entry) \
{ \
  PLIST_ENTRY _OldFlink; \
  _OldFlink = (_ListHead)->Flink; \
  (_Entry)->Flink = _OldFlink; \
  (_Entry)->Blink = (_ListHead); \
  _OldFlink->Blink = (_Entry); \
  (_ListHead)->Flink = (_Entry); \
}

/*
 * VOID
 * InsertTailList(
 *   IN PLIST_ENTRY  ListHead,
 *   IN PLIST_ENTRY  Entry)
 */
#define InsertTailList(_ListHead, \
                       _Entry) \
{ \
	PLIST_ENTRY _OldBlink; \
	_OldBlink = (_ListHead)->Blink; \
	(_Entry)->Flink = (_ListHead); \
	(_Entry)->Blink = _OldBlink; \
	_OldBlink->Flink = (_Entry); \
	(_ListHead)->Blink = (_Entry); \
}

/*
 * BOOLEAN
 * IsListEmpty(
 *   IN PLIST_ENTRY  ListHead)
 */
#define IsListEmpty(_ListHead) \
  ((_ListHead)->Flink == (_ListHead))

static __inline PSINGLE_LIST_ENTRY 
PopEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead)
{
	PSINGLE_LIST_ENTRY Entry;

	Entry = ListHead->Next;
	if (Entry != NULL)
	{
		ListHead->Next = Entry->Next;
	}
  return Entry;
}

/*
 * VOID
 * PushEntryList(
 *   IN PSINGLE_LIST_ENTRY  ListHead,
 *   IN PSINGLE_LIST_ENTRY  Entry)
 */
#define PushEntryList(_ListHead, \
                      _Entry) \
{ \
	(_Entry)->Next = (_ListHead)->Next; \
	(_ListHead)->Next = (_Entry); \
}

/*
 * VOID
 * RemoveEntryList(
 *   IN PLIST_ENTRY  Entry)
 */
#define RemoveEntryList(_Entry) \
{ \
	PLIST_ENTRY _OldFlink; \
	PLIST_ENTRY _OldBlink; \
	_OldFlink = (_Entry)->Flink; \
	_OldBlink = (_Entry)->Blink; \
	_OldFlink->Blink = _OldBlink; \
	_OldBlink->Flink = _OldFlink; \
  (_Entry)->Flink = NULL; \
  (_Entry)->Blink = NULL; \
}

static __inline PLIST_ENTRY 
RemoveHeadList(
  IN PLIST_ENTRY  ListHead)
{
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;
	PLIST_ENTRY Entry;

	Entry = ListHead->Flink;
	OldFlink = ListHead->Flink->Flink;
	OldBlink = ListHead->Flink->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;

  if (Entry != ListHead)
  {
    Entry->Flink = NULL;
    Entry->Blink = NULL;
  }

	return Entry;
}

static __inline PLIST_ENTRY
RemoveTailList(
  IN PLIST_ENTRY  ListHead)
{
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;
	PLIST_ENTRY Entry;

	Entry = ListHead->Blink;
	OldFlink = ListHead->Blink->Flink;
	OldBlink = ListHead->Blink->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;

  if (Entry != ListHead)
  {
    Entry->Flink = NULL;
    Entry->Blink = NULL;
  }
   
  return Entry;
}

#if !defined(_WINBASE_H) || _WIN32_WINNT < 0x0501

NTOSAPI
PSLIST_ENTRY
DDKFASTAPI
InterlockedPopEntrySList(
  IN PSLIST_HEADER  ListHead);

NTOSAPI
PSLIST_ENTRY
DDKFASTAPI
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

NTOSAPI
ULONG
DDKAPI
RtlAnsiStringToUnicodeSize(
  IN PANSI_STRING  AnsiString);

NTOSAPI
NTSTATUS
DDKAPI
RtlAddRange(
  IN OUT PRTL_RANGE_LIST  RangeList,
  IN ULONGLONG  Start,
  IN ULONGLONG  End,
  IN UCHAR  Attributes,
  IN ULONG  Flags,
  IN PVOID  UserData  OPTIONAL,
  IN PVOID  Owner  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
RtlAnsiStringToUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PANSI_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTOSAPI
NTSTATUS
DDKAPI
RtlAppendUnicodeStringToString(
  IN OUT PUNICODE_STRING  Destination,
  IN PUNICODE_STRING  Source);

NTOSAPI
NTSTATUS
DDKAPI
RtlAppendUnicodeToString(
  IN OUT PUNICODE_STRING  Destination,
  IN PCWSTR  Source);

NTOSAPI
BOOLEAN
DDKAPI
RtlAreBitsClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  Length); 

NTOSAPI
BOOLEAN
DDKAPI
RtlAreBitsSet(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  Length); 

NTOSAPI
NTSTATUS
DDKAPI
RtlCharToInteger(
  IN PCSZ  String,
  IN ULONG  Base  OPTIONAL,
  IN OUT PULONG  Value);

NTOSAPI
ULONG
DDKAPI
RtlCheckBit(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  BitPosition); 

NTOSAPI
NTSTATUS
DDKAPI
RtlCheckRegistryKey(
  IN ULONG  RelativeTo,
  IN PWSTR  Path);

NTOSAPI
VOID
DDKAPI
RtlClearAllBits(
  IN PRTL_BITMAP  BitMapHeader); 

NTOSAPI
VOID
DDKAPI
RtlClearBit(
  PRTL_BITMAP  BitMapHeader,
  ULONG  BitNumber);

NTOSAPI
VOID
DDKAPI
RtlClearBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  NumberToClear); 

NTOSAPI
SIZE_T
DDKAPI
RtlCompareMemory(
  IN CONST VOID  *Source1,
  IN CONST VOID  *Source2,
  IN SIZE_T  Length);

NTOSAPI
LONG
DDKAPI
RtlCompareString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  BOOLEAN  CaseInSensitive);

NTOSAPI
LONG
DDKAPI
RtlCompareUnicodeString(
  IN PUNICODE_STRING  String1,
  IN PUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlConvertLongToLargeInteger(
  IN LONG  SignedInteger);

NTOSAPI
LUID
DDKAPI
RtlConvertLongToLuid(
  IN LONG  Long);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlConvertUlongToLargeInteger(
  IN ULONG  UnsignedInteger);

NTOSAPI
LUID
DDKAPI
RtlConvertUlongToLuid(
  ULONG  Ulong);

/*
 * VOID
 * RtlCopyMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN CONST VOID UNALIGNED  *Source,
 *   IN SIZE_T  Length)
 */
#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) \
  memcpy(Destination, Source, Length);
#endif

#ifndef RtlCopyBytes
#define RtlCopyBytes RtlCopyMemory
#endif

NTOSAPI
VOID
DDKAPI
RtlCopyMemory32(
  IN VOID UNALIGNED  *Destination,
  IN CONST VOID UNALIGNED  *Source,
  IN ULONG  Length);

NTOSAPI
NTSTATUS
DDKAPI
RtlCopyRangeList(
  OUT PRTL_RANGE_LIST  CopyRangeList,
  IN PRTL_RANGE_LIST  RangeList);

NTOSAPI
VOID
DDKAPI
RtlCopyString(
  IN OUT PSTRING  DestinationString,
  IN PSTRING  SourceString  OPTIONAL);

NTOSAPI
VOID
DDKAPI
RtlCopyUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PUNICODE_STRING  SourceString);

NTOSAPI
NTSTATUS
DDKAPI
RtlCreateRegistryKey(
  IN ULONG  RelativeTo,
  IN PWSTR  Path);

NTOSAPI
NTSTATUS
DDKAPI
RtlCreateSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  Revision);

NTOSAPI
NTSTATUS
DDKAPI
RtlDeleteOwnersRanges(
  IN OUT PRTL_RANGE_LIST  RangeList,
  IN PVOID  Owner);

NTOSAPI
NTSTATUS
DDKAPI
RtlDeleteRange(
  IN OUT PRTL_RANGE_LIST  RangeList,
  IN ULONGLONG  Start,
  IN ULONGLONG  End,
  IN PVOID  Owner);

NTOSAPI
NTSTATUS
DDKAPI
RtlDeleteRegistryValue(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PCWSTR  ValueName);

/*
 * BOOLEAN
 * RtlEqualLuid( 
 *   IN LUID  Luid1,
 *   IN LUID  Luid2)
 */
#define RtlEqualLuid(_Luid1, \
                     _Luid2) \
  ((Luid1.LowPart == Luid2.LowPart) && (Luid1.HighPart == Luid2.HighPart))

/*
 * ULONG
 * RtlEqualMemory(
 *   IN VOID UNALIGNED  *Destination,
 *   IN CONST VOID UNALIGNED  *Source,
 *   IN SIZE_T  Length)
 */
#define RtlEqualMemory(Destination, Source, Length) (!memcmp(Destination, Source, Length))

NTOSAPI
BOOLEAN
DDKAPI
RtlEqualString(
  IN PSTRING  String1,
  IN PSTRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTOSAPI
BOOLEAN
DDKAPI
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

NTOSAPI
ULONG
DDKAPI
RtlFindClearBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex); 

NTOSAPI
ULONG
DDKAPI
RtlFindClearBitsAndSet(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex); 

NTOSAPI
ULONG
DDKAPI
RtlFindClearRuns( 
  IN PRTL_BITMAP  BitMapHeader, 
  OUT PRTL_BITMAP_RUN  RunArray, 
  IN ULONG  SizeOfRunArray, 
  IN BOOLEAN  LocateLongestRuns);

NTOSAPI
ULONG
DDKAPI
RtlFindFirstRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  OUT PULONG  StartingIndex);

NTOSAPI
ULONG
DDKAPI
RtlFindLastBackwardRunClear(
  IN PRTL_BITMAP  BitMapHeader, 
  IN ULONG  FromIndex, 
  OUT PULONG  StartingRunIndex); 

NTOSAPI
CCHAR
DDKAPI
RtlFindLeastSignificantBit(
  IN ULONGLONG  Set);

NTOSAPI
ULONG
DDKAPI
RtlFindLongestRunClear(
  IN PRTL_BITMAP  BitMapHeader,
  OUT PULONG  StartingIndex); 

NTOSAPI
CCHAR
DDKAPI
RtlFindMostSignificantBit(
  IN ULONGLONG  Set);

NTOSAPI
ULONG
DDKAPI
RtlFindNextForwardRunClear(
  IN PRTL_BITMAP  BitMapHeader, 
  IN ULONG  FromIndex, 
  OUT PULONG  StartingRunIndex);

NTOSAPI
NTSTATUS
DDKAPI
RtlFindRange(
  IN PRTL_RANGE_LIST  RangeList,
  IN ULONGLONG  Minimum,
  IN ULONGLONG  Maximum,
  IN ULONG  Length,
  IN ULONG  Alignment,
  IN ULONG  Flags,
  IN UCHAR  AttributeAvailableMask,
  IN PVOID  Context  OPTIONAL,
  IN PRTL_CONFLICT_RANGE_CALLBACK  Callback  OPTIONAL,
  OUT PULONGLONG  Start);

NTOSAPI
ULONG
DDKAPI
RtlFindSetBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex); 

NTOSAPI
ULONG
DDKAPI
RtlFindSetBitsAndClear(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  NumberToFind,
  IN ULONG  HintIndex); 

NTOSAPI
VOID
DDKAPI
RtlFreeAnsiString(
  IN PANSI_STRING  AnsiString);

NTOSAPI
VOID
DDKAPI
RtlFreeRangeList(
  IN PRTL_RANGE_LIST  RangeList);

NTOSAPI
VOID
DDKAPI
RtlFreeUnicodeString(
  IN PUNICODE_STRING  UnicodeString);

NTOSAPI
VOID
DDKAPI
RtlGetCallersAddress(
  OUT PVOID  *CallersAddress,
  OUT PVOID  *CallersCaller);

NTOSAPI
NTSTATUS
DDKAPI
RtlGetVersion(
  IN OUT PRTL_OSVERSIONINFOW  lpVersionInformation);

NTOSAPI
NTSTATUS
DDKAPI
RtlGetFirstRange(
  IN PRTL_RANGE_LIST  RangeList,
  OUT PRTL_RANGE_LIST_ITERATOR  Iterator,
  OUT PRTL_RANGE  *Range);

NTOSAPI
NTSTATUS
DDKAPI
RtlGetNextRange(
  IN OUT  PRTL_RANGE_LIST_ITERATOR  Iterator,
  OUT PRTL_RANGE  *Range,
  IN BOOLEAN  MoveForwards);

#define FOR_ALL_RANGES(RangeList, Iterator, Current)          \
  for (RtlGetFirstRange((RangeList), (Iterator), &(Current)); \
    (Current) != NULL;                                        \
    RtlGetNextRange((Iterator), &(Current), TRUE))

#define FOR_ALL_RANGES_BACKWARDS(RangeList, Iterator, Current) \
  for (RtlGetLastRange((RangeList), (Iterator), &(Current));   \
    (Current) != NULL;                                         \
    RtlGetNextRange((Iterator), &(Current), FALSE))

NTOSAPI
NTSTATUS
DDKAPI
RtlGUIDFromString( 
  IN PUNICODE_STRING  GuidString, 
  OUT GUID  *Guid);

NTOSAPI
NTSTATUS
DDKAPI
RtlHashUnicodeString(
  IN CONST UNICODE_STRING  *String,
  IN BOOLEAN  CaseInSensitive,
  IN ULONG  HashAlgorithm,
  OUT PULONG  HashValue);

NTOSAPI
VOID
DDKAPI
RtlInitAnsiString(
  IN OUT PANSI_STRING  DestinationString,
  IN PCSZ  SourceString);

NTOSAPI
VOID
DDKAPI
RtlInitializeBitMap(
  IN PRTL_BITMAP  BitMapHeader,
  IN PULONG  BitMapBuffer,
  IN ULONG  SizeOfBitMap); 

NTOSAPI
VOID
DDKAPI
RtlInitializeRangeList(
  IN OUT PRTL_RANGE_LIST  RangeList);

NTOSAPI
VOID
DDKAPI
RtlInitString(
  IN OUT PSTRING  DestinationString,
  IN PCSZ  SourceString);

NTOSAPI
VOID
DDKAPI
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCWSTR  SourceString);

NTOSAPI
NTSTATUS
DDKAPI
RtlInt64ToUnicodeString(
  IN ULONGLONG  Value,
  IN ULONG  Base OPTIONAL,
  IN OUT PUNICODE_STRING  String);

NTOSAPI
NTSTATUS
DDKAPI
RtlIntegerToUnicodeString(
  IN ULONG  Value,
  IN ULONG  Base  OPTIONAL,
  IN OUT PUNICODE_STRING  String);

NTOSAPI
NTSTATUS
DDKAPI
RtlIntPtrToUnicodeString(
  PLONG  Value,
  ULONG  Base  OPTIONAL,
  PUNICODE_STRING  String);

NTOSAPI
NTSTATUS
DDKAPI
RtlInvertRangeList(
  OUT PRTL_RANGE_LIST  InvertedRangeList,
  IN PRTL_RANGE_LIST  RangeList);

NTOSAPI
NTSTATUS
DDKAPI
RtlIsRangeAvailable(
  IN PRTL_RANGE_LIST  RangeList,
  IN ULONGLONG  Start,
  IN ULONGLONG  End,
  IN ULONG  Flags,
  IN UCHAR  AttributeAvailableMask,
  IN PVOID  Context  OPTIONAL,
  IN PRTL_CONFLICT_RANGE_CALLBACK  Callback  OPTIONAL,
  OUT PBOOLEAN  Available);

/*
 * BOOLEAN
 * RtlIsZeroLuid(
 *   IN PLUID  L1)
 */
#define RtlIsZeroLuid(_L1) \
  ((BOOLEAN) ((!(_L1)->LowPart) && (!(_L1)->HighPart)))

NTOSAPI
ULONG
DDKAPI
RtlLengthSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

NTOSAPI
VOID
DDKAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK  AccessMask,
  IN PGENERIC_MAPPING  GenericMapping);

NTOSAPI
NTSTATUS
DDKAPI
RtlMergeRangeLists(
  OUT PRTL_RANGE_LIST  MergedRangeList,
  IN PRTL_RANGE_LIST  RangeList1,
  IN PRTL_RANGE_LIST  RangeList2,
  IN ULONG  Flags);

/*
 * VOID
 * RtlMoveMemory(
 *  IN VOID UNALIGNED  *Destination,
 *  IN CONST VOID UNALIGNED  *Source,
 *  IN SIZE_T  Length)
 */
#define RtlMoveMemory memmove

NTOSAPI
ULONG
DDKAPI
RtlNumberOfClearBits(
  IN PRTL_BITMAP  BitMapHeader);

NTOSAPI
ULONG
DDKAPI
RtlNumberOfSetBits(
  IN PRTL_BITMAP  BitMapHeader); 

NTOSAPI
VOID
DDKFASTAPI
RtlPrefetchMemoryNonTemporal(
  IN PVOID  Source,
  IN SIZE_T  Length);

NTOSAPI
BOOLEAN
DDKAPI
RtlPrefixUnicodeString( 
  IN PUNICODE_STRING  String1, 
  IN PUNICODE_STRING  String2, 
  IN BOOLEAN  CaseInSensitive);

NTOSAPI
NTSTATUS
DDKAPI
RtlQueryRegistryValues(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PRTL_QUERY_REGISTRY_TABLE  QueryTable,
  IN PVOID  Context,
  IN PVOID  Environment  OPTIONAL);

NTOSAPI
VOID
DDKAPI
RtlRetrieveUlong(
  IN OUT PULONG  DestinationAddress,
  IN PULONG  SourceAddress);

NTOSAPI
VOID
DDKAPI
RtlRetrieveUshort(
  IN OUT PUSHORT  DestinationAddress,
  IN PUSHORT  SourceAddress);

NTOSAPI
VOID
DDKAPI
RtlSetAllBits(
  IN PRTL_BITMAP  BitMapHeader); 

NTOSAPI
VOID
DDKAPI
RtlSetBit(
  PRTL_BITMAP  BitMapHeader,
  ULONG  BitNumber);

NTOSAPI
VOID
DDKAPI
RtlSetBits(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  StartingIndex,
  IN ULONG  NumberToSet); 

NTOSAPI
NTSTATUS
DDKAPI
RtlSetDaclSecurityDescriptor(
  IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN BOOLEAN  DaclPresent,
  IN PACL  Dacl  OPTIONAL,
  IN BOOLEAN  DaclDefaulted  OPTIONAL);

NTOSAPI
VOID
DDKAPI
RtlStoreUlong(
  IN PULONG  Address,
  IN ULONG  Value);

NTOSAPI
VOID
DDKAPI
RtlStoreUlonglong(
  IN OUT PULONGLONG  Address,
  ULONGLONG  Value);

NTOSAPI
VOID
DDKAPI
RtlStoreUlongPtr(
  IN OUT PULONG_PTR  Address,
  IN ULONG_PTR  Value);

NTOSAPI
VOID
DDKAPI
RtlStoreUshort(
  IN PUSHORT  Address,
  IN USHORT  Value);

NTOSAPI
NTSTATUS
DDKAPI
RtlStringFromGUID( 
  IN REFGUID  Guid, 
  OUT PUNICODE_STRING  GuidString);

NTOSAPI
BOOLEAN
DDKAPI
RtlTestBit(
  IN PRTL_BITMAP  BitMapHeader,
  IN ULONG  BitNumber);

NTOSAPI
BOOLEAN
DDKAPI
RtlTimeFieldsToTime(
  IN PTIME_FIELDS  TimeFields,
  IN PLARGE_INTEGER  Time);

NTOSAPI
VOID
DDKAPI
RtlTimeToTimeFields(
  IN PLARGE_INTEGER  Time,
  IN PTIME_FIELDS  TimeFields);

NTOSAPI
ULONG
DDKFASTAPI
RtlUlongByteSwap(
  IN ULONG  Source);

NTOSAPI
ULONGLONG
DDKFASTAPI
RtlUlonglongByteSwap(
  IN ULONGLONG  Source);

NTOSAPI
ULONG
DDKAPI
RtlUnicodeStringToAnsiSize(
  IN PUNICODE_STRING  UnicodeString);

NTOSAPI
NTSTATUS
DDKAPI
RtlUnicodeStringToAnsiString(
  IN OUT PANSI_STRING  DestinationString,
  IN PUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTOSAPI
NTSTATUS
DDKAPI
RtlUnicodeStringToInteger(
  IN PUNICODE_STRING  String,
  IN ULONG  Base  OPTIONAL,
  OUT PULONG  Value);

NTOSAPI
WCHAR
DDKAPI
RtlUpcaseUnicodeChar( 
  IN WCHAR  SourceCharacter);

NTOSAPI
NTSTATUS
DDKAPI
RtlUpcaseUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString  OPTIONAL,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTOSAPI
CHAR
DDKAPI
RtlUpperChar( 
  IN CHAR Character);

NTOSAPI
VOID
DDKAPI
RtlUpperString(
  IN OUT PSTRING  DestinationString,
  IN PSTRING  SourceString);

NTOSAPI
USHORT
DDKFASTAPI
RtlUshortByteSwap(
  IN USHORT  Source);

NTOSAPI
BOOLEAN
DDKAPI
RtlValidRelativeSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptorInput,
  IN ULONG  SecurityDescriptorLength,
  IN SECURITY_INFORMATION  RequiredInformation);

NTOSAPI
BOOLEAN
DDKAPI
RtlValidSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

NTOSAPI
NTSTATUS
DDKAPI
RtlVerifyVersionInfo(
  IN PRTL_OSVERSIONINFOEXW  VersionInfo,
  IN ULONG  TypeMask,
  IN ULONGLONG  ConditionMask);

NTOSAPI
NTSTATUS
DDKAPI
RtlVolumeDeviceToDosName(
  IN PVOID  VolumeDeviceObject,
  OUT PUNICODE_STRING  DosName);

NTOSAPI
ULONG
DDKAPI
RtlWalkFrameChain(
  OUT PVOID  *Callers,
  IN ULONG  Count,
  IN ULONG  Flags);

NTOSAPI
NTSTATUS
DDKAPI
RtlWriteRegistryValue(
  IN ULONG  RelativeTo,
  IN PCWSTR  Path,
  IN PCWSTR  ValueName,
  IN ULONG  ValueType,
  IN PVOID  ValueData,
  IN ULONG  ValueLength);

NTOSAPI
ULONG
DDKAPI
RtlxUnicodeStringToAnsiSize(
  IN PUNICODE_STRING  UnicodeString);

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


/** Executive support routines **/

NTOSAPI
VOID
DDKFASTAPI
ExAcquireFastMutex(
  IN PFAST_MUTEX  FastMutex);

NTOSAPI
VOID
DDKFASTAPI
ExAcquireFastMutexUnsafe(
  IN PFAST_MUTEX  FastMutex);

NTOSAPI
BOOLEAN
DDKAPI
ExAcquireResourceExclusiveLite(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTOSAPI
BOOLEAN
DDKAPI
ExAcquireResourceSharedLite(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTOSAPI
BOOLEAN
DDKAPI
ExAcquireSharedStarveExclusive(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

NTOSAPI
BOOLEAN
DDKAPI
ExAcquireSharedWaitForExclusive(
  IN PERESOURCE  Resource,
  IN BOOLEAN  Wait);

static __inline PVOID
ExAllocateFromNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside)
{
	PVOID Entry;

	Lookaside->TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
	if (Entry == NULL) {
		Lookaside->_DDK_DUMMYUNION_MEMBER(AllocateMisses)++;
		Entry = (Lookaside->Allocate)(Lookaside->Type, Lookaside->Size, Lookaside->Tag);
	}
  return Entry;
}

static __inline PVOID
ExAllocateFromPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside)
{
  PVOID Entry;

  Lookaside->TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
  if (Entry == NULL) {
    Lookaside->_DDK_DUMMYUNION_MEMBER(AllocateMisses)++;
    Entry = (Lookaside->Allocate)(Lookaside->Type,
      Lookaside->Size, Lookaside->Tag);
  }
  return Entry;
}

NTOSAPI
PVOID
DDKAPI
ExAllocatePoolWithQuotaTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

NTOSAPI
PVOID
DDKAPI
ExAllocatePoolWithTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

#ifdef POOL_TAGGING

#define ExAllocatePoolWithQuota(p,n) ExAllocatePoolWithQuotaTag(p,n,' kdD')
#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n,' kdD')

#else /* !POOL_TAGGING */

NTOSAPI
PVOID
DDKAPI
ExAllocatePool(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes);

NTOSAPI
PVOID
DDKAPI
ExAllocatePoolWithQuota(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes);

#endif /* POOL_TAGGING */

NTOSAPI
PVOID
DDKAPI
ExAllocatePoolWithTagPriority(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag,
  IN EX_POOL_PRIORITY  Priority);

NTOSAPI
VOID
DDKAPI
ExConvertExclusiveToSharedLite(
  IN PERESOURCE  Resource);

NTOSAPI
NTSTATUS
DDKAPI
ExCreateCallback(
  OUT PCALLBACK_OBJECT  *CallbackObject,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  Create,
  IN BOOLEAN  AllowMultipleCallbacks);

NTOSAPI
VOID
DDKAPI
ExDeleteNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside);

NTOSAPI
VOID
DDKAPI
ExDeletePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside);

NTOSAPI
NTSTATUS
DDKAPI
ExDeleteResourceLite(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKAPI
ExFreePool(
  IN PVOID  P);

#define PROTECTED_POOL                    0x80000000

#ifdef POOL_TAGGING
#define ExFreePool(P) ExFreePoolWithTag(P, 0)
#endif

NTOSAPI
VOID
DDKAPI
ExFreePoolWithTag(
  IN PVOID  P,
  IN ULONG  Tag);

#define ExQueryDepthSList(ListHead) QueryDepthSList(ListHead)

static __inline VOID
ExFreeToNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->TotalFrees++;
	if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
		Lookaside->_DDK_DUMMYUNION_N_MEMBER(2,FreeMisses)++;
		(Lookaside->Free)(Entry);
  } else {
		InterlockedPushEntrySList(&Lookaside->ListHead,
      (PSLIST_ENTRY)Entry);
	}
}

static __inline VOID
ExFreeToPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
    Lookaside->_DDK_DUMMYUNION_N_MEMBER(2,FreeMisses)++;
    (Lookaside->Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->ListHead, (PSLIST_ENTRY)Entry);
  }
}

/*
 * ERESOURCE_THREAD
 * ExGetCurrentResourceThread(
 *   VOID);
 */
#define ExGetCurrentResourceThread() ((ERESOURCE_THREAD) PsGetCurrentThread())

NTOSAPI
ULONG
DDKAPI
ExGetExclusiveWaiterCount(
  IN PERESOURCE  Resource);

NTOSAPI
KPROCESSOR_MODE
DDKAPI
ExGetPreviousMode( 
  VOID);

NTOSAPI
ULONG
DDKAPI
ExGetSharedWaiterCount(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKAPI
KeInitializeEvent(
  IN PRKEVENT  Event,
  IN EVENT_TYPE  Type,
  IN BOOLEAN  State);

/*
 * VOID DDKAPI
 * ExInitializeFastMutex(
 *   IN PFAST_MUTEX  FastMutex)
 */
#define ExInitializeFastMutex(_FastMutex) \
{ \
  (_FastMutex)->Count = 1; \
  (_FastMutex)->Owner = NULL; \
  (_FastMutex)->Contention = 0; \
  KeInitializeEvent(&(_FastMutex)->Event, SynchronizationEvent, FALSE); \
}

NTOSAPI
VOID
DDKAPI
ExInitializeNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
  IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
  IN PFREE_FUNCTION  Free  OPTIONAL,
  IN ULONG  Flags,
  IN SIZE_T  Size,
  IN ULONG  Tag,
  IN USHORT  Depth);

NTOSAPI
VOID
DDKAPI
ExInitializePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside,
  IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
  IN PFREE_FUNCTION  Free  OPTIONAL,
  IN ULONG  Flags,
  IN SIZE_T  Size,
  IN ULONG  Tag,
  IN USHORT  Depth);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
LARGE_INTEGER
DDKAPI
ExInterlockedAddLargeInteger(
  IN PLARGE_INTEGER  Addend,
  IN LARGE_INTEGER  Increment,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
VOID
DDKFASTAPI
ExInterlockedAddLargeStatistic(
  IN PLARGE_INTEGER  Addend,
  IN ULONG  Increment);

NTOSAPI
ULONG
DDKFASTAPI
ExInterlockedAddUlong(
  IN PULONG  Addend,
  IN ULONG  Increment,
  PKSPIN_LOCK  Lock);

NTOSAPI
LONGLONG
DDKFASTAPI
ExInterlockedCompareExchange64(
  IN OUT PLONGLONG  Destination,
  IN PLONGLONG  Exchange,
  IN PLONGLONG  Comparand,
  IN PKSPIN_LOCK  Lock); 

NTOSAPI
PSINGLE_LIST_ENTRY
DDKFASTAPI
ExInterlockedFlushSList(
  IN PSLIST_HEADER  ListHead);

NTOSAPI
PLIST_ENTRY
DDKFASTAPI
ExInterlockedInsertHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
PLIST_ENTRY
DDKFASTAPI
ExInterlockedInsertTailList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
PSINGLE_LIST_ENTRY
DDKFASTAPI
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

NTOSAPI
PSINGLE_LIST_ENTRY
DDKFASTAPI
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

NTOSAPI
PLIST_ENTRY
DDKFASTAPI
ExInterlockedRemoveHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
BOOLEAN
DDKAPI
ExIsProcessorFeaturePresent(
  IN ULONG  ProcessorFeature);

NTOSAPI
BOOLEAN
DDKAPI
ExIsResourceAcquiredExclusiveLite(
  IN PERESOURCE  Resource);

NTOSAPI
USHORT
DDKAPI
ExIsResourceAcquiredLite(
  IN PERESOURCE  Resource);

NTOSAPI
USHORT
DDKAPI
ExIsResourceAcquiredSharedLite(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKAPI
ExLocalTimeToSystemTime(
  IN PLARGE_INTEGER  LocalTime,
  OUT PLARGE_INTEGER  SystemTime);

NTOSAPI
VOID
DDKAPI
ExNotifyCallback(
  IN PCALLBACK_OBJECT  CallbackObject,
  IN PVOID  Argument1,
  IN PVOID  Argument2);

NTOSAPI
VOID
DDKAPI
ExRaiseAccessViolation(
  VOID);

NTOSAPI
VOID
DDKAPI
ExRaiseDatatypeMisalignment(
  VOID);

NTOSAPI
VOID
DDKAPI
ExRaiseStatus(
  IN NTSTATUS  Status);

NTOSAPI
PVOID
DDKAPI
ExRegisterCallback(
  IN PCALLBACK_OBJECT  CallbackObject,
  IN PCALLBACK_FUNCTION  CallbackFunction,
  IN PVOID  CallbackContext);

NTOSAPI
VOID
DDKAPI
ExReinitializeResourceLite(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKFASTAPI
ExReleaseFastMutex(
  IN PFAST_MUTEX  FastMutex);

NTOSAPI
VOID
DDKFASTAPI
ExReleaseFastMutexUnsafe(
  IN PFAST_MUTEX  FastMutex);

NTOSAPI
VOID
DDKAPI
ExReleaseResourceForThreadLite(
  IN PERESOURCE  Resource,
  IN ERESOURCE_THREAD  ResourceThreadId);

NTOSAPI
VOID
DDKFASTAPI
ExReleaseResourceLite(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKAPI
ExSetResourceOwnerPointer( 
  IN PERESOURCE  Resource,
  IN PVOID  OwnerPointer);

NTOSAPI
ULONG
DDKAPI
ExSetTimerResolution(
  IN ULONG  DesiredTime,
  IN BOOLEAN  SetResolution);

NTOSAPI
VOID
DDKAPI
ExSystemTimeToLocalTime(
  IN PLARGE_INTEGER  SystemTime,
  OUT PLARGE_INTEGER  LocalTime);

NTOSAPI
BOOLEAN
DDKFASTAPI
ExTryToAcquireFastMutex(
  IN PFAST_MUTEX  FastMutex);

NTOSAPI
BOOLEAN
DDKAPI
ExTryToAcquireResourceExclusiveLite(
  IN PERESOURCE  Resource);

NTOSAPI
VOID
DDKAPI
ExUnregisterCallback(
  IN PVOID  CbRegistration);

NTOSAPI
NTSTATUS
DDKAPI
ExUuidCreate(
  OUT UUID  *Uuid);

NTOSAPI
BOOLEAN
DDKAPI
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

NTOSAPI
VOID
DDKAPI
ProbeForRead(
  IN CONST VOID  *Address,
  IN ULONG  Length,
  IN ULONG  Alignment);

NTOSAPI
VOID
DDKAPI
ProbeForWrite(
  IN CONST VOID  *Address,
  IN ULONG  Length,
  IN ULONG  Alignment);



/** Configuration manager routines **/

NTOSAPI
NTSTATUS
DDKAPI
CmRegisterCallback(
  IN PEX_CALLBACK_FUNCTION  Function,
  IN PVOID  Context,
  IN OUT PLARGE_INTEGER  Cookie);

NTOSAPI
NTSTATUS
DDKAPI
CmUnRegisterCallback(
  IN LARGE_INTEGER  Cookie);



/** Filesystem runtime library routines **/

NTOSAPI
BOOLEAN
DDKAPI
FsRtlIsTotalDeviceFailure(
  IN NTSTATUS  Status);



/** Hardware abstraction layer routines **/

NTOSAPI
VOID
DDKFASTAPI
HalExamineMBR(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  MBRTypeIdentifier,
  OUT PVOID  Buffer);

NTOSAPI
VOID
DDKAPI
READ_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
READ_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
READ_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTOSAPI
UCHAR
DDKAPI
READ_PORT_UCHAR(
  IN PUCHAR  Port);

NTOSAPI
ULONG
DDKAPI
READ_PORT_ULONG(
  IN PULONG  Port);

NTOSAPI
USHORT
DDKAPI
READ_PORT_USHORT(
  IN PUSHORT  Port);

NTOSAPI
VOID
DDKAPI
READ_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
READ_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
READ_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTOSAPI
UCHAR
DDKAPI
READ_REGISTER_UCHAR(
  IN PUCHAR  Register);

NTOSAPI
ULONG
DDKAPI
READ_REGISTER_ULONG(
  IN PULONG  Register);

NTOSAPI
USHORT
DDKAPI
READ_REGISTER_USHORT(
  IN PUSHORT  Register);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_UCHAR(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_ULONG(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_USHORT(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_UCHAR(
  IN PUCHAR  Port,
  IN UCHAR  Value);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_ULONG(
  IN PULONG  Port,
  IN ULONG  Value);

NTOSAPI
VOID
DDKAPI
WRITE_PORT_USHORT(
  IN PUSHORT  Port,
  IN USHORT  Value);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_BUFFER_UCHAR(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_BUFFER_ULONG(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_BUFFER_USHORT(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_UCHAR(
  IN PUCHAR  Register,
  IN UCHAR  Value);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_ULONG(
  IN PULONG  Register,
  IN ULONG  Value);

NTOSAPI
VOID
DDKAPI
WRITE_REGISTER_USHORT(
  IN PUSHORT  Register,
  IN USHORT  Value);

/** I/O manager routines **/

NTOSAPI
VOID
DDKAPI
IoAcquireCancelSpinLock(
  OUT PKIRQL  Irql);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
VOID
DDKAPI
IoAllocateController(
  IN PCONTROLLER_OBJECT  ControllerObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

NTOSAPI
NTSTATUS
DDKAPI
IoAllocateDriverObjectExtension(
  IN PDRIVER_OBJECT  DriverObject,
  IN PVOID  ClientIdentificationAddress,
  IN ULONG  DriverObjectExtensionSize,
  OUT PVOID  *DriverObjectExtension);

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

NTOSAPI
PVOID
DDKAPI
IoAllocateErrorLogEntry(
  IN PVOID  IoObject,
  IN UCHAR  EntrySize);

NTOSAPI
PIRP
DDKAPI
IoAllocateIrp(
  IN CCHAR  StackSize,
  IN BOOLEAN  ChargeQuota);

NTOSAPI
PMDL
DDKAPI
IoAllocateMdl(
  IN PVOID  VirtualAddress,
  IN ULONG  Length,
  IN BOOLEAN  SecondaryBuffer,
  IN BOOLEAN  ChargeQuota,
  IN OUT PIRP  Irp  OPTIONAL);

NTOSAPI
PIO_WORKITEM
DDKAPI
IoAllocateWorkItem(
  IN PDEVICE_OBJECT  DeviceObject);

/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

NTOSAPI
NTSTATUS
DDKAPI
IoAttachDevice(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PUNICODE_STRING  TargetDevice,
  OUT PDEVICE_OBJECT  *AttachedDevice);

NTOSAPI
PDEVICE_OBJECT
DDKAPI
IoAttachDeviceToDeviceStack(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PDEVICE_OBJECT  TargetDevice);

NTOSAPI
PIRP
DDKAPI
IoBuildAsynchronousFsdRequest(
  IN ULONG  MajorFunction,
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PVOID  Buffer  OPTIONAL,
  IN ULONG  Length  OPTIONAL,
  IN PLARGE_INTEGER  StartingOffset  OPTIONAL,
  IN PIO_STATUS_BLOCK  IoStatusBlock  OPTIONAL);

NTOSAPI
PIRP
DDKAPI
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

NTOSAPI
VOID
DDKAPI
IoBuildPartialMdl(
  IN PMDL  SourceMdl,
  IN OUT PMDL  TargetMdl,
  IN PVOID  VirtualAddress,
  IN ULONG  Length);

NTOSAPI
PIRP
DDKAPI
IoBuildSynchronousFsdRequest(
  IN ULONG  MajorFunction,
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PVOID  Buffer  OPTIONAL,
  IN ULONG  Length  OPTIONAL,
  IN PLARGE_INTEGER  StartingOffset  OPTIONAL,
  IN PKEVENT  Event,
  OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTOSAPI
NTSTATUS
DDKFASTAPI
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

NTOSAPI
VOID
DDKAPI
IoCancelFileOpen(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PFILE_OBJECT  FileObject);

NTOSAPI
BOOLEAN
DDKAPI
IoCancelIrp(
  IN PIRP  Irp);

NTOSAPI
NTSTATUS
DDKAPI
IoCheckShareAccess(
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  DesiredShareAccess,
  IN OUT PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess,
  IN BOOLEAN  Update);

NTOSAPI
VOID
DDKFASTAPI
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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
PCONTROLLER_OBJECT
DDKAPI
IoCreateController(
  IN ULONG  Size);

NTOSAPI
NTSTATUS
DDKAPI
IoCreateDevice(
  IN PDRIVER_OBJECT  DriverObject,
  IN ULONG  DeviceExtensionSize,
  IN PUNICODE_STRING  DeviceName  OPTIONAL,
  IN DEVICE_TYPE  DeviceType,
  IN ULONG  DeviceCharacteristics,
  IN BOOLEAN  Exclusive,
  OUT PDEVICE_OBJECT  *DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoCreateDisk(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PCREATE_DISK  Disk);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
PKEVENT
DDKAPI
IoCreateNotificationEvent(
  IN PUNICODE_STRING  EventName,
  OUT PHANDLE  EventHandle);

NTOSAPI
NTSTATUS
DDKAPI
IoCreateSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN PUNICODE_STRING  DeviceName);

NTOSAPI
PKEVENT
DDKAPI
IoCreateSynchronizationEvent(
  IN PUNICODE_STRING  EventName,
  OUT PHANDLE  EventHandle);

NTOSAPI
NTSTATUS
DDKAPI
IoCreateUnprotectedSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN PUNICODE_STRING  DeviceName);

NTOSAPI
VOID
DDKAPI
IoCsqInitialize(
  PIO_CSQ  Csq,
  IN PIO_CSQ_INSERT_IRP  CsqInsertIrp,
  IN PIO_CSQ_REMOVE_IRP  CsqRemoveIrp,
  IN PIO_CSQ_PEEK_NEXT_IRP  CsqPeekNextIrp,
  IN PIO_CSQ_ACQUIRE_LOCK  CsqAcquireLock,
  IN PIO_CSQ_RELEASE_LOCK  CsqReleaseLock,
  IN PIO_CSQ_COMPLETE_CANCELED_IRP  CsqCompleteCanceledIrp);

NTOSAPI
VOID
DDKAPI
IoCsqInsertIrp(
  IN  PIO_CSQ  Csq,
  IN  PIRP  Irp,
  IN  PIO_CSQ_IRP_CONTEXT  Context);

NTOSAPI
PIRP
DDKAPI
IoCsqRemoveIrp(
  IN  PIO_CSQ  Csq,
  IN  PIO_CSQ_IRP_CONTEXT  Context);

NTOSAPI
PIRP
DDKAPI
IoCsqRemoveNextIrp(
  IN PIO_CSQ  Csq,
  IN PVOID  PeekContext);

NTOSAPI
VOID
DDKAPI
IoDeleteController(
  IN PCONTROLLER_OBJECT  ControllerObject);

NTOSAPI
VOID
DDKAPI
IoDeleteDevice(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoDeleteSymbolicLink(
  IN PUNICODE_STRING  SymbolicLinkName);

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

NTOSAPI
VOID
DDKAPI
IoDetachDevice(
  IN OUT PDEVICE_OBJECT  TargetDevice);

NTOSAPI
VOID
DDKAPI
IoDisconnectInterrupt(
  IN PKINTERRUPT  InterruptObject);

NTOSAPI
BOOLEAN
DDKAPI
IoForwardIrpSynchronously(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp);

#define IoForwardAndCatchIrp IoForwardIrpSynchronously

NTOSAPI
VOID
DDKAPI
IoFreeController(
  IN PCONTROLLER_OBJECT  ControllerObject);

NTOSAPI
VOID
DDKAPI
IoFreeErrorLogEntry(
  PVOID  ElEntry);

NTOSAPI
VOID
DDKAPI
IoFreeIrp(
  IN PIRP  Irp);

NTOSAPI
VOID
DDKAPI
IoFreeMdl(
  IN PMDL  Mdl);

NTOSAPI
VOID
DDKAPI
IoFreeWorkItem(
  IN PIO_WORKITEM  pIOWorkItem);

NTOSAPI
PDEVICE_OBJECT
DDKAPI
IoGetAttachedDevice(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
PDEVICE_OBJECT
DDKAPI
IoGetAttachedDeviceReference(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoGetBootDiskInformation(
  IN OUT PBOOTDISK_INFORMATION  BootDiskInformation,
  IN ULONG  Size);

NTOSAPI
PCONFIGURATION_INFORMATION
DDKAPI
IoGetConfigurationInformation( 
  VOID);

NTOSAPI
PEPROCESS
DDKAPI
IoGetCurrentProcess(
  VOID);

NTOSAPI
NTSTATUS
DDKAPI
IoGetDeviceInterfaceAlias(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN CONST GUID  *AliasInterfaceClassGuid,
  OUT PUNICODE_STRING  AliasSymbolicLinkName);

NTOSAPI
NTSTATUS
DDKAPI
IoGetDeviceInterfaces(
  IN CONST GUID  *InterfaceClassGuid,
  IN PDEVICE_OBJECT  PhysicalDeviceObject  OPTIONAL,
  IN ULONG  Flags,
  OUT PWSTR  *SymbolicLinkList);

NTOSAPI
NTSTATUS
DDKAPI
IoGetDeviceObjectPointer(
  IN PUNICODE_STRING  ObjectName,
  IN ACCESS_MASK  DesiredAccess,
  OUT PFILE_OBJECT  *FileObject,
  OUT PDEVICE_OBJECT  *DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoGetDeviceProperty(
  IN PDEVICE_OBJECT  DeviceObject,
  IN DEVICE_REGISTRY_PROPERTY  DeviceProperty,
  IN ULONG  BufferLength,
  OUT PVOID  PropertyBuffer,
  OUT PULONG  ResultLength);

NTOSAPI
PDEVICE_OBJECT
DDKAPI
IoGetDeviceToVerify(
  IN PETHREAD  Thread);

NTOSAPI
PDMA_ADAPTER
DDKAPI
IoGetDmaAdapter(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PDEVICE_DESCRIPTION  DeviceDescription,
  IN OUT PULONG  NumberOfMapRegisters);

NTOSAPI
PVOID
DDKAPI
IoGetDriverObjectExtension(
  IN PDRIVER_OBJECT  DriverObject,
  IN PVOID  ClientIdentificationAddress);

NTOSAPI
PGENERIC_MAPPING
DDKAPI
IoGetFileObjectGenericMapping(
  VOID);

/*
 * ULONG
 * IoGetFunctionCodeFromCtlCode(
 *   IN ULONG  ControlCode)
 */
#define IoGetFunctionCodeFromCtlCode(_ControlCode) \
  (((_ControlCode) >> 2) & 0x00000FFF)

NTOSAPI
PVOID
DDKAPI
IoGetInitialStack(
  VOID);

NTOSAPI
PDEVICE_OBJECT
DDKAPI
IoGetRelatedDeviceObject(
  IN PFILE_OBJECT  FileObject);

NTOSAPI
ULONG
DDKAPI
IoGetRemainingStackSize(
  VOID);

NTOSAPI
VOID
DDKAPI
IoGetStackLimits(
  OUT PULONG_PTR  LowLimit,
  OUT PULONG_PTR  HighLimit);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
VOID
DDKAPI
IoInitializeIrp(
  IN OUT PIRP  Irp,
  IN USHORT  PacketSize,
  IN CCHAR  StackSize);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
IoInitializeTimer(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIO_TIMER_ROUTINE  TimerRoutine,
  IN PVOID  Context);

NTOSAPI
VOID
DDKAPI
IoInvalidateDeviceRelations(
  IN PDEVICE_OBJECT  DeviceObject,
  IN DEVICE_RELATION_TYPE  Type);

NTOSAPI
VOID
DDKAPI
IoInvalidateDeviceState(
  IN PDEVICE_OBJECT  PhysicalDeviceObject);

NTOSAPI
BOOLEAN
DDKAPI
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

NTOSAPI
BOOLEAN
DDKAPI
IoIsWdmVersionAvailable(
  IN UCHAR  MajorVersion,
  IN UCHAR  MinorVersion);

NTOSAPI
PIRP
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
IoOpenDeviceInterfaceRegistryKey(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  DeviceInterfaceKey);

NTOSAPI
NTSTATUS
DDKAPI
IoOpenDeviceRegistryKey(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  DevInstKeyType,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  DevInstRegKey);

NTOSAPI
NTSTATUS
DDKAPI
IoQueryDeviceDescription(
  IN PINTERFACE_TYPE  BusType  OPTIONAL,
  IN PULONG  BusNumber  OPTIONAL,
  IN PCONFIGURATION_TYPE  ControllerType  OPTIONAL,
  IN PULONG  ControllerNumber  OPTIONAL,
  IN PCONFIGURATION_TYPE  PeripheralType  OPTIONAL,
  IN PULONG  PeripheralNumber  OPTIONAL,
  IN PIO_QUERY_DEVICE_ROUTINE  CalloutRoutine,
  IN PVOID  Context);

NTOSAPI
VOID
DDKAPI
IoQueueWorkItem(
  IN PIO_WORKITEM  pIOWorkItem,
  IN PIO_WORKITEM_ROUTINE  Routine,
  IN WORK_QUEUE_TYPE  QueueType,
  IN PVOID  Context);

NTOSAPI
VOID
DDKAPI
IoRaiseHardError(
  IN PIRP  Irp,
  IN PVPB  Vpb  OPTIONAL,
  IN PDEVICE_OBJECT  RealDeviceObject);

NTOSAPI
BOOLEAN
DDKAPI
IoRaiseInformationalHardError(
  IN NTSTATUS  ErrorStatus,
  IN PUNICODE_STRING  String  OPTIONAL,
  IN PKTHREAD  Thread  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
IoReadDiskSignature(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  BytesPerSector,
  OUT PDISK_SIGNATURE  Signature);

NTOSAPI
NTSTATUS
DDKAPI
IoReadPartitionTableEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX  **PartitionBuffer);

NTOSAPI
VOID
DDKAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTOSAPI
VOID
DDKAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTOSAPI
NTSTATUS
DDKAPI
IoRegisterDeviceInterface(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN CONST GUID  *InterfaceClassGuid,
  IN PUNICODE_STRING  ReferenceString  OPTIONAL,
  OUT PUNICODE_STRING  SymbolicLinkName);

NTOSAPI
VOID
DDKAPI
IoRegisterDriverReinitialization(
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_REINITIALIZE  DriverReinitializationRoutine,
  IN PVOID  Context);

NTOSAPI
NTSTATUS
DDKAPI
IoRegisterPlugPlayNotification(
  IN IO_NOTIFICATION_EVENT_CATEGORY  EventCategory,
  IN ULONG  EventCategoryFlags,
  IN PVOID  EventCategoryData  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE  CallbackRoutine,
  IN PVOID  Context,
  OUT PVOID  *NotificationEntry);

NTOSAPI
NTSTATUS
DDKAPI
IoRegisterShutdownNotification(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
VOID
DDKAPI
IoReleaseCancelSpinLock(
  IN KIRQL  Irql);

NTOSAPI
VOID
DDKAPI
IoReleaseRemoveLockAndWaitEx(
  IN PIO_REMOVE_LOCK  RemoveLock,
  IN PVOID  Tag,
  IN ULONG  RemlockSize);

NTOSAPI
VOID
DDKAPI
IoReleaseRemoveLockEx(
  IN PIO_REMOVE_LOCK  RemoveLock,
  IN PVOID  Tag,
  IN ULONG  RemlockSize);

/*
 * VOID
 * IoReleaseRemoveLockAndWait(
 *   IN PIO_REMOVE_LOCK  RemoveLock,
 *   IN PVOID  Tag)
 */
#define IoReleaseRemoveLockAndWait(_RemoveLock, \
                                   _Tag) \
  IoReleaseRemoveLockEx(_RemoveLock, _Tag, sizeof(IO_REMOVE_LOCK))

NTOSAPI
VOID
DDKAPI
IoRemoveShareAccess(
  IN PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess);

NTOSAPI
NTSTATUS
DDKAPI
IoReportDetectedDevice(
  IN PDRIVER_OBJECT  DriverObject,
  IN INTERFACE_TYPE  LegacyBusType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PCM_RESOURCE_LIST  ResourceList,
  IN PIO_RESOURCE_REQUIREMENTS_LIST  ResourceRequirements  OPTIONAL,
  IN BOOLEAN  ResourceAssigned,
  IN OUT PDEVICE_OBJECT  *DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoReportResourceForDetection(
  IN PDRIVER_OBJECT  DriverObject,
  IN PCM_RESOURCE_LIST  DriverList  OPTIONAL,
  IN ULONG  DriverListSize  OPTIONAL,
  IN PDEVICE_OBJECT  DeviceObject  OPTIONAL,
  IN PCM_RESOURCE_LIST  DeviceList  OPTIONAL,
  IN ULONG  DeviceListSize  OPTIONAL,
  OUT PBOOLEAN  ConflictDetected);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
IoReportTargetDeviceChange(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PVOID  NotificationStructure);

NTOSAPI
NTSTATUS
DDKAPI
IoReportTargetDeviceChangeAsynchronous(
  IN PDEVICE_OBJECT  PhysicalDeviceObject,
  IN PVOID  NotificationStructure,
  IN PDEVICE_CHANGE_COMPLETE_CALLBACK  Callback  OPTIONAL,
  IN PVOID  Context  OPTIONAL);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
VOID
DDKAPI
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
  ASSERT(_InvokeOnSuccess || _InvokeOnError || _InvokeOnCancel ? \
    _CompletionRoutine != NULL : TRUE); \
  _IrpSp = IoGetNextIrpStackLocation(_Irp); \
  _IrpSp->CompletionRoutine = (PIO_COMPLETION_ROUTINE)(_CompletionRoutine); \
	_IrpSp->Context = (_Context); \
  _IrpSp->Control = 0; \
  if (_InvokeOnSuccess) _IrpSp->Control = SL_INVOKE_ON_SUCCESS; \
  if (_InvokeOnError) _IrpSp->Control |= SL_INVOKE_ON_ERROR; \
  if (_InvokeOnCancel) _IrpSp->Control |= SL_INVOKE_ON_CANCEL; \
}

NTOSAPI
VOID
DDKAPI
IoSetCompletionRoutineEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN PIO_COMPLETION_ROUTINE  CompletionRoutine,
  IN PVOID  Context,
  IN BOOLEAN    InvokeOnSuccess,
  IN BOOLEAN  InvokeOnError,
  IN BOOLEAN  InvokeOnCancel);

NTOSAPI
NTSTATUS
DDKAPI
IoSetDeviceInterfaceState(
  IN PUNICODE_STRING  SymbolicLinkName,
  IN BOOLEAN  Enable);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
IoSetPartitionInformationEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  PartitionNumber,
  IN struct _SET_PARTITION_INFORMATION_EX  *PartitionInfo);

NTOSAPI
VOID
DDKAPI
IoSetShareAccess(
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  DesiredShareAccess,
  IN OUT PFILE_OBJECT  FileObject,
  OUT PSHARE_ACCESS  ShareAccess);

NTOSAPI
VOID
DDKAPI
IoSetStartIoAttributes(
  IN PDEVICE_OBJECT  DeviceObject, 
  IN BOOLEAN  DeferredStartIo, 
  IN BOOLEAN  NonCancelable); 

NTOSAPI
NTSTATUS
DDKAPI
IoSetSystemPartition(
  IN PUNICODE_STRING  VolumeNameString);

NTOSAPI
BOOLEAN
DDKAPI
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

NTOSAPI
VOID
DDKAPI
IoStartNextPacket(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  Cancelable);

NTOSAPI
VOID
DDKAPI
IoStartNextPacketByKey(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  Cancelable,
  IN ULONG  Key);

NTOSAPI
VOID
DDKAPI
IoStartPacket(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN PULONG  Key  OPTIONAL,
  IN PDRIVER_CANCEL  CancelFunction  OPTIONAL);

NTOSAPI
VOID
DDKAPI
IoStartTimer(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
VOID
DDKAPI
IoStopTimer(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoUnregisterPlugPlayNotification(
  IN PVOID  NotificationEntry);

NTOSAPI
VOID
DDKAPI
IoUnregisterShutdownNotification(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
VOID
DDKAPI
IoUpdateShareAccess(
  IN PFILE_OBJECT  FileObject,
  IN OUT PSHARE_ACCESS  ShareAccess);

NTOSAPI
NTSTATUS
DDKAPI
IoVerifyPartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN BOOLEAN  FixErrors);

NTOSAPI
NTSTATUS
DDKAPI
IoVolumeDeviceToDosName(
  IN  PVOID  VolumeDeviceObject,
  OUT PUNICODE_STRING  DosName);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIAllocateInstanceIds(
  IN GUID  *Guid,
  IN ULONG  InstanceCount,
  OUT ULONG  *FirstInstanceId);

NTOSAPI
ULONG
DDKAPI
IoWMIDeviceObjectToProviderId(
  IN PDEVICE_OBJECT  DeviceObject);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIDeviceObjectToInstanceName(
  IN PVOID  DataBlockObject,
  IN PDEVICE_OBJECT  DeviceObject,
  OUT PUNICODE_STRING  InstanceName);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIExecuteMethod(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  MethodId,
  IN ULONG  InBufferSize,
  IN OUT PULONG  OutBufferSize,
  IN OUT  PUCHAR  InOutBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIHandleToInstanceName(
  IN PVOID  DataBlockObject,
  IN HANDLE  FileHandle,
  OUT PUNICODE_STRING  InstanceName);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIOpenBlock(
  IN GUID  *DataBlockGuid,
  IN ULONG  DesiredAccess,
  OUT PVOID  *DataBlockObject);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIQueryAllData(
  IN PVOID  DataBlockObject,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIQueryAllDataMultiple(
  IN PVOID  *DataBlockObjectList,
  IN ULONG  ObjectCount,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIQuerySingleInstance(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID OutBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIQuerySingleInstanceMultiple(
  IN PVOID  *DataBlockObjectList,
  IN PUNICODE_STRING  InstanceNames,
  IN ULONG  ObjectCount,
  IN OUT ULONG  *InOutBufferSize,
  OUT PVOID  OutBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIRegistrationControl(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  Action);

NTOSAPI
NTSTATUS
DDKAPI
IoWMISetNotificationCallback(
  IN PVOID  Object,
  IN WMI_NOTIFICATION_CALLBACK  Callback,
  IN PVOID  Context);

NTOSAPI
NTSTATUS
DDKAPI
IoWMISetSingleInstance(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  Version,
  IN ULONG  ValueBufferSize,
  IN PVOID  ValueBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMISetSingleItem(
  IN PVOID  DataBlockObject,
  IN PUNICODE_STRING  InstanceName,
  IN ULONG  DataItemId,
  IN ULONG  Version,
  IN ULONG  ValueBufferSize,
  IN PVOID  ValueBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoWMISuggestInstanceName(
  IN PDEVICE_OBJECT  PhysicalDeviceObject OPTIONAL,
  IN PUNICODE_STRING  SymbolicLinkName OPTIONAL,
  IN BOOLEAN  CombineNames,
  OUT PUNICODE_STRING  SuggestedInstanceName);

NTOSAPI
NTSTATUS
DDKAPI
IoWMIWriteEvent(
  IN PVOID  WnodeEventItem);

NTOSAPI
VOID
DDKAPI
IoWriteErrorLogEntry(
  IN PVOID  ElEntry);

NTOSAPI
NTSTATUS
DDKAPI
IoWritePartitionTableEx(
  IN PDEVICE_OBJECT  DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX  *PartitionBuffer);



/** Kernel routines **/

NTOSAPI
VOID
DDKFASTAPI
KeAcquireInStackQueuedSpinLock(
  IN PKSPIN_LOCK  SpinLock,
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTOSAPI
VOID
DDKFASTAPI
KeAcquireInStackQueuedSpinLockAtDpcLevel(
  IN PKSPIN_LOCK  SpinLock,
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTOSAPI
KIRQL
DDKAPI
KeAcquireInterruptSpinLock(
  IN PKINTERRUPT  Interrupt);

NTOSAPI
VOID
DDKAPI
KeAcquireSpinLock(
  IN PKSPIN_LOCK  SpinLock,
  OUT PKIRQL  OldIrql);

/* System Service Dispatch Table */
typedef PVOID (NTAPI * SSDT)(VOID);
typedef SSDT * PSSDT;

/* System Service Parameters Table */
typedef UCHAR SSPT, * PSSPT;

typedef struct _SSDT_ENTRY {
	PSSDT  SSDT;
	PULONG  ServiceCounterTable;
	ULONG  NumberOfServices;
	PSSPT  SSPT;
} SSDT_ENTRY, *PSSDT_ENTRY;

NTOSAPI
BOOLEAN
DDKAPI
KeAddSystemServiceTable(
  IN PSSDT  SSDT,
  IN PULONG  ServiceCounterTable,
  IN ULONG  NumberOfServices,
  IN PSSPT  SSPT,
  IN ULONG  TableIndex);

NTOSAPI
BOOLEAN
DDKAPI
KeAreApcsDisabled(
  VOID);

NTOSAPI
VOID
DDKAPI
KeAttachProcess(
  IN PEPROCESS  Process);

NTOSAPI
VOID
DDKAPI
KeBugCheck(
  IN ULONG  BugCheckCode);

NTOSAPI
VOID
DDKAPI
KeBugCheckEx(
  IN ULONG  BugCheckCode,
  IN ULONG_PTR  BugCheckParameter1,
  IN ULONG_PTR  BugCheckParameter2,
  IN ULONG_PTR  BugCheckParameter3,
  IN ULONG_PTR  BugCheckParameter4);

NTOSAPI
BOOLEAN
DDKAPI
KeCancelTimer(
  IN PKTIMER  Timer);

NTOSAPI
VOID
DDKAPI
KeClearEvent(
  IN PRKEVENT  Event);

NTOSAPI
NTSTATUS
DDKAPI
KeDelayExecutionThread(
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Interval);

NTOSAPI
BOOLEAN
DDKAPI
KeDeregisterBugCheckCallback(
  IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord);

NTOSAPI
VOID
DDKAPI
KeDetachProcess(
  VOID);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
PRKTHREAD
DDKAPI
KeGetCurrentThread(
  VOID);

NTOSAPI
KPROCESSOR_MODE
DDKAPI
KeGetPreviousMode(
  VOID);

NTOSAPI
ULONG
DDKAPI
KeGetRecommendedSharedDataAlignment(
  VOID);

NTOSAPI
VOID
DDKAPI
KeInitializeApc(
  IN PKAPC  Apc,
	IN PKTHREAD  Thread,
	IN UCHAR  StateIndex,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine,
	IN PKNORMAL_ROUTINE  NormalRoutine,
	IN UCHAR  Mode,
	IN PVOID  Context);

NTOSAPI
VOID
DDKAPI
KeInitializeDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue);

NTOSAPI
VOID
DDKAPI
KeInitializeMutex(
  IN PRKMUTEX  Mutex,
  IN ULONG  Level);

NTOSAPI
VOID
DDKAPI
KeInitializeSemaphore(
  IN PRKSEMAPHORE  Semaphore,
  IN LONG  Count,
  IN LONG  Limit);

NTOSAPI
VOID
DDKAPI
KeInitializeSpinLock(
  IN PKSPIN_LOCK  SpinLock);

NTOSAPI
VOID
DDKAPI
KeInitializeTimer(
  IN PKTIMER  Timer);

NTOSAPI
VOID
DDKAPI
KeInitializeTimerEx(
  IN PKTIMER  Timer,
  IN TIMER_TYPE  Type);

NTOSAPI
BOOLEAN
DDKAPI
KeInsertByKeyDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry,
  IN ULONG  SortKey);

NTOSAPI
BOOLEAN
DDKAPI
KeInsertDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry);

NTOSAPI
BOOLEAN
DDKAPI
KeInsertQueueDpc(
  IN PRKDPC  Dpc,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

NTOSAPI
VOID
DDKAPI
KeLeaveCriticalRegion(
  VOID);

NTOSAPI
NTSTATUS
DDKAPI
KePulseEvent(
  IN PRKEVENT  Event,
  IN KPRIORITY  Increment,
  IN BOOLEAN  Wait);

NTOSAPI
ULONGLONG
DDKAPI
KeQueryInterruptTime(
  VOID);

NTOSAPI
LARGE_INTEGER
DDKAPI
KeQueryPerformanceCounter(
  OUT PLARGE_INTEGER  PerformanceFrequency  OPTIONAL);

NTOSAPI
KPRIORITY
DDKAPI
KeQueryPriorityThread(
  IN PRKTHREAD  Thread);

NTOSAPI
VOID
DDKAPI
KeQuerySystemTime(
  OUT PLARGE_INTEGER  CurrentTime);

NTOSAPI
VOID
DDKAPI
KeQueryTickCount(
  OUT PLARGE_INTEGER  TickCount);

NTOSAPI
ULONG
DDKAPI
KeQueryTimeIncrement(
  VOID);

NTOSAPI
LONG
DDKAPI
KeReadStateEvent(
  IN PRKEVENT  Event);

NTOSAPI
LONG
DDKAPI
KeReadStateMutex(
  IN PRKMUTEX  Mutex);

NTOSAPI
LONG
DDKAPI
KeReadStateSemaphore(
  IN PRKSEMAPHORE  Semaphore);

NTOSAPI
BOOLEAN
DDKAPI
KeReadStateTimer(
  IN PKTIMER  Timer);

NTOSAPI
BOOLEAN
DDKAPI
KeRegisterBugCheckCallback(
  IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord,
  IN PKBUGCHECK_CALLBACK_ROUTINE  CallbackRoutine,
  IN PVOID  Buffer,
  IN ULONG  Length,
  IN PUCHAR  Component);

NTOSAPI
VOID
DDKFASTAPI
KeReleaseInStackQueuedSpinLock(
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTOSAPI
VOID
DDKFASTAPI
KeReleaseInStackQueuedSpinLockFromDpcLevel(
  IN PKLOCK_QUEUE_HANDLE  LockHandle);

NTOSAPI
VOID
DDKAPI
KeReleaseInterruptSpinLock(
  IN PKINTERRUPT  Interrupt,
  IN KIRQL  OldIrql);

NTOSAPI
LONG
DDKAPI
KeReleaseMutex(
  IN PRKMUTEX  Mutex,
  IN BOOLEAN  Wait);

NTOSAPI
LONG
DDKAPI
KeReleaseSemaphore(
  IN PRKSEMAPHORE  Semaphore,
  IN KPRIORITY  Increment,
  IN LONG  Adjustment,
  IN BOOLEAN  Wait);

NTOSAPI
VOID
DDKAPI
KeReleaseSpinLock(
  IN PKSPIN_LOCK  SpinLock,
  IN KIRQL  NewIrql);

NTOSAPI
PKDEVICE_QUEUE_ENTRY
DDKAPI 
KeRemoveByKeyDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN ULONG  SortKey);

NTOSAPI
PKDEVICE_QUEUE_ENTRY
DDKAPI
KeRemoveDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue);

NTOSAPI
BOOLEAN
DDKAPI
KeRemoveEntryDeviceQueue(
  IN PKDEVICE_QUEUE  DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY  DeviceQueueEntry);

NTOSAPI
BOOLEAN
DDKAPI
KeRemoveQueueDpc(
  IN PRKDPC  Dpc);

NTOSAPI
LONG
DDKAPI
KeResetEvent(
  IN PRKEVENT  Event);

NTOSAPI
NTSTATUS
DDKAPI
KeRestoreFloatingPointState(
  IN PKFLOATING_SAVE  FloatSave);

NTOSAPI
NTSTATUS
DDKAPI
KeSaveFloatingPointState(
  OUT PKFLOATING_SAVE  FloatSave);

NTOSAPI
LONG
DDKAPI
KeSetBasePriorityThread(
  IN PRKTHREAD  Thread,
  IN LONG  Increment);

NTOSAPI
LONG
DDKAPI
KeSetEvent(
  IN PRKEVENT  Event,
  IN KPRIORITY  Increment,
  IN BOOLEAN  Wait);

NTOSAPI
VOID
DDKAPI
KeSetImportanceDpc(
  IN PRKDPC  Dpc,
  IN KDPC_IMPORTANCE  Importance);

NTOSAPI
KPRIORITY
DDKAPI
KeSetPriorityThread(
  IN PKTHREAD  Thread,
  IN KPRIORITY  Priority);

NTOSAPI
VOID
DDKAPI
KeSetTargetProcessorDpc(
  IN PRKDPC  Dpc,
  IN CCHAR  Number);

NTOSAPI
BOOLEAN
DDKAPI
KeSetTimer(
  IN PKTIMER  Timer,
  IN LARGE_INTEGER  DueTime,
  IN PKDPC  Dpc  OPTIONAL);

NTOSAPI
BOOLEAN
DDKAPI
KeSetTimerEx(
  IN PKTIMER  Timer,
  IN LARGE_INTEGER  DueTime,
  IN LONG  Period  OPTIONAL,
  IN PKDPC  Dpc  OPTIONAL);

NTOSAPI
VOID
DDKFASTAPI
KeSetTimeUpdateNotifyRoutine(
  IN PTIME_UPDATE_NOTIFY_ROUTINE  NotifyRoutine);

NTOSAPI
VOID
DDKAPI
KeStallExecutionProcessor(
  IN ULONG  MicroSeconds);

NTOSAPI
BOOLEAN
DDKAPI
KeSynchronizeExecution(
  IN PKINTERRUPT    Interrupt,
  IN PKSYNCHRONIZE_ROUTINE  SynchronizeRoutine,
  IN PVOID  SynchronizeContext);

NTOSAPI
NTSTATUS
DDKAPI
KeWaitForMultipleObjects(
  IN ULONG  Count,
  IN PVOID  Object[],
  IN WAIT_TYPE  WaitType,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL,
  IN PKWAIT_BLOCK  WaitBlockArray  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
KeWaitForMutexObject(
  IN PRKMUTEX  Mutex,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
KeWaitForSingleObject(
  IN PVOID  Object,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL);

#if defined(_X86_)

NTOSAPI
VOID
FASTCALL
KfLowerIrql(
  IN KIRQL  NewIrql);

NTOSAPI
KIRQL
FASTCALL
KfRaiseIrql(
  IN KIRQL  NewIrql);

#define KeLowerIrql(a) KfLowerIrql(a)
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

#else

NTOSAPI
VOID
DDKAPI
KeLowerIrql(
  IN KIRQL  NewIrql);

NTOSAPI
KIRQL
DDKAPI
KeRaiseIrql(
  IN KIRQL  NewIrql);

#endif

NTOSAPI
KIRQL
DDKAPI
KeRaiseIrqlToDpcLevel(
  VOID);

/** Memory manager routines **/

NTOSAPI
NTSTATUS
DDKAPI
MmAdvanceMdl(
  IN PMDL  Mdl,
  IN ULONG  NumberOfBytes);

NTOSAPI
PVOID
DDKAPI
MmAllocateContiguousMemory(
  IN ULONG  NumberOfBytes,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress);

NTOSAPI
PVOID
DDKAPI
MmAllocateContiguousMemorySpecifyCache(
  IN SIZE_T  NumberOfBytes,
  IN PHYSICAL_ADDRESS  LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS  BoundaryAddressMultiple  OPTIONAL,
  IN MEMORY_CACHING_TYPE  CacheType);

NTOSAPI
PVOID
DDKAPI
MmAllocateMappingAddress(
  IN SIZE_T  NumberOfBytes,
  IN ULONG  PoolTag);

NTOSAPI
PVOID
DDKAPI
MmAllocateNonCachedMemory(
  IN ULONG  NumberOfBytes);

NTOSAPI
PMDL
DDKAPI
MmAllocatePagesForMdl(
  IN PHYSICAL_ADDRESS  LowAddress,
  IN PHYSICAL_ADDRESS  HighAddress,
  IN PHYSICAL_ADDRESS  SkipBytes,
  IN SIZE_T  TotalBytes);

NTOSAPI
VOID
DDKAPI
MmBuildMdlForNonPagedPool(
  IN OUT PMDL  MemoryDescriptorList);

NTOSAPI
NTSTATUS
DDKAPI
MmCreateSection(
  OUT PSECTION_OBJECT  *SectionObject,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN PLARGE_INTEGER  MaximumSize,
  IN ULONG  SectionPageProtection,
  IN ULONG  AllocationAttributes,
  IN HANDLE  FileHandle  OPTIONAL,
  IN PFILE_OBJECT  File  OPTIONAL);

typedef enum _MMFLUSH_TYPE {
  MmFlushForDelete,
  MmFlushForWrite
} MMFLUSH_TYPE;

NTOSAPI
BOOLEAN
DDKAPI
MmFlushImageSection(
  IN PSECTION_OBJECT_POINTERS  SectionObjectPointer,
  IN MMFLUSH_TYPE  FlushType);

NTOSAPI
VOID
DDKAPI
MmFreeContiguousMemory(
  IN PVOID  BaseAddress);

NTOSAPI
VOID
DDKAPI
MmFreeContiguousMemorySpecifyCache(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheType);

NTOSAPI
VOID
DDKAPI
MmFreeMappingAddress(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag);

NTOSAPI
VOID
DDKAPI
MmFreeNonCachedMemory(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
PHYSICAL_ADDRESS
DDKAPI
MmGetPhysicalAddress(
  IN PVOID  BaseAddress);

NTOSAPI
PPHYSICAL_MEMORY_RANGE
DDKAPI
MmGetPhysicalMemoryRanges(
  VOID);

NTOSAPI
PVOID
DDKAPI
MmGetVirtualForPhysical(
  IN PHYSICAL_ADDRESS  PhysicalAddress);

NTOSAPI
PVOID
DDKAPI
MmMapLockedPagesSpecifyCache(
  IN PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode,
  IN MEMORY_CACHING_TYPE  CacheType,
  IN PVOID  BaseAddress,
  IN ULONG  BugCheckOnFailure,
  IN MM_PAGE_PRIORITY  Priority);

NTOSAPI
PVOID
DDKAPI
MmMapLockedPagesWithReservedMapping(
  IN PVOID  MappingAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList,
  IN MEMORY_CACHING_TYPE  CacheType);

NTOSAPI
NTSTATUS
DDKAPI
MmMapUserAddressesToPage(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes,
  IN PVOID  PageAddress);

NTOSAPI
PVOID
DDKAPI
MmMapVideoDisplay(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheType);

NTOSAPI
NTSTATUS
DDKAPI
MmMapViewInSessionSpace(
  IN PVOID  Section,
  OUT PVOID  *MappedBase,
  IN OUT PSIZE_T  ViewSize);

NTOSAPI
NTSTATUS
DDKAPI
MmMapViewInSystemSpace(
  IN PVOID  Section,
  OUT PVOID  *MappedBase,
  IN PSIZE_T  ViewSize);

NTOSAPI
NTSTATUS
DDKAPI
MmMarkPhysicalMemoryAsBad(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

NTOSAPI
NTSTATUS
DDKAPI
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
  ((_Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA \
    | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
    (_Mdl)->MappedSystemVa : \
    (PVOID) MmMapLockedPagesSpecifyCache((_Mdl), \
      KernelMode, MmCached, NULL, FALSE, _Priority)

NTOSAPI
PVOID
DDKAPI
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

NTOSAPI
BOOLEAN
DDKAPI
MmIsAddressValid(
  IN PVOID  VirtualAddress);

NTOSAPI
LOGICAL
DDKAPI
MmIsDriverVerifying(
  IN PDRIVER_OBJECT  DriverObject);

NTOSAPI
BOOLEAN
DDKAPI
MmIsThisAnNtAsSystem(
  VOID);

NTOSAPI
NTSTATUS
DDKAPI
MmIsVerifierEnabled(
  OUT PULONG  VerifierFlags);

NTOSAPI
PVOID
DDKAPI
MmLockPagableDataSection(
  IN PVOID  AddressWithinSection);

NTOSAPI
PVOID
DDKAPI
MmLockPagableImageSection(
  IN PVOID  AddressWithinSection);

/*
 * PVOID
 * MmLockPagableCodeSection(
 *   IN PVOID  AddressWithinSection)
 */
#define MmLockPagableCodeSection MmLockPagableDataSection

NTOSAPI
VOID
DDKAPI
MmLockPagableSectionByHandle(
  IN PVOID  ImageSectionHandle);

NTOSAPI
PVOID
DDKAPI
MmMapIoSpace(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN ULONG  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheEnable);

NTOSAPI
PVOID
DDKAPI
MmMapLockedPages(
  IN PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode);

NTOSAPI
VOID
DDKAPI
MmPageEntireDriver(
  IN PVOID  AddressWithinSection);

NTOSAPI
VOID
DDKAPI
MmProbeAndLockProcessPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN PEPROCESS  Process,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTOSAPI
NTSTATUS
DDKAPI
MmProtectMdlSystemAddress(
  IN PMDL  MemoryDescriptorList,
  IN ULONG  NewProtect);

NTOSAPI
VOID
DDKAPI
MmUnmapLockedPages(
  IN PVOID  BaseAddress,
  IN PMDL  MemoryDescriptorList);

NTOSAPI
NTSTATUS
DDKAPI
MmUnmapViewInSessionSpace(
  IN PVOID  MappedBase);

NTOSAPI
NTSTATUS
DDKAPI
MmUnmapViewInSystemSpace(
  IN PVOID MappedBase);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
VOID
DDKAPI
MmProbeAndLockPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTOSAPI
MM_SYSTEM_SIZE
DDKAPI
MmQuerySystemSize(
  VOID);

NTOSAPI
NTSTATUS
DDKAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS  StartAddress,
  IN OUT PLARGE_INTEGER  NumberOfBytes);

NTOSAPI
VOID
DDKAPI
MmResetDriverPaging(
  IN PVOID  AddressWithinSection);

NTOSAPI
HANDLE
DDKAPI
MmSecureVirtualMemory(
  IN PVOID  Address,
  IN SIZE_T  Size,
  IN ULONG  ProbeMode);

NTOSAPI
ULONG
DDKAPI
MmSizeOfMdl(
  IN PVOID  Base,
  IN SIZE_T  Length);

NTOSAPI
VOID
DDKAPI
MmUnlockPagableImageSection(
  IN PVOID  ImageSectionHandle);

NTOSAPI
VOID
DDKAPI
MmUnlockPages(
  IN PMDL  MemoryDescriptorList);

NTOSAPI
VOID
DDKAPI
MmUnmapIoSpace(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTOSAPI
VOID
DDKAPI
MmUnmapReservedMapping(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList);

NTOSAPI
VOID
DDKAPI
MmUnmapVideoDisplay(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);



/** Object manager routines **/

NTOSAPI
NTSTATUS
DDKAPI
ObAssignSecurity(
  IN PACCESS_STATE  AccessState,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN PVOID  Object,
  IN POBJECT_TYPE  Type);

NTOSAPI
VOID
DDKAPI
ObDereferenceSecurityDescriptor(
  PSECURITY_DESCRIPTOR  SecurityDescriptor,
  ULONG  Count);

NTOSAPI
VOID
DDKFASTAPI
ObfDereferenceObject(
  IN PVOID  Object);

/*
 * VOID
 * ObDereferenceObject(
 *   IN PVOID  Object)
 */
#define ObDereferenceObject ObfDereferenceObject

NTOSAPI
NTSTATUS
DDKAPI
ObGetObjectSecurity(
  IN PVOID  Object,
  OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
  OUT PBOOLEAN  MemoryAllocated); 

NTOSAPI
NTSTATUS
DDKAPI
ObInsertObject(
  IN PVOID  Object,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  AdditionalReferences,
  OUT PVOID*  ReferencedObject  OPTIONAL,
  OUT PHANDLE  Handle);

NTOSAPI
VOID
DDKFASTAPI
ObfReferenceObject(
  IN PVOID  Object);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
VOID
DDKAPI
ObMakeTemporaryObject(
  IN PVOID  Object);

NTOSAPI
NTSTATUS
DDKAPI
ObOpenObjectByName(
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN POBJECT_TYPE  ObjectType,
  IN OUT PVOID  ParseContext  OPTIONAL,
  IN KPROCESSOR_MODE  AccessMode,
  IN ACCESS_MASK  DesiredAccess,
  IN PACCESS_STATE  PassedAccessState,
  OUT PHANDLE  Handle);

NTOSAPI
NTSTATUS
DDKAPI
ObOpenObjectByPointer(
  IN PVOID  Object,
  IN ULONG  HandleAttributes,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess  OPTIONAL,
  IN POBJECT_TYPE  ObjectType  OPTIONAL,
  IN KPROCESSOR_MODE  AccessMode,
  OUT PHANDLE  Handle);

NTOSAPI
NTSTATUS
DDKAPI
ObQueryObjectAuditingByHandle(
  IN HANDLE  Handle,
  OUT PBOOLEAN  GenerateOnClose);

NTOSAPI
NTSTATUS
DDKAPI
ObReferenceObjectByHandle(
  IN HANDLE  Handle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_TYPE  ObjectType  OPTIONAL,
  IN KPROCESSOR_MODE  AccessMode,
  OUT PVOID  *Object,
  OUT POBJECT_HANDLE_INFORMATION  HandleInformation  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
ObReferenceObjectByName(
  IN PUNICODE_STRING  ObjectPath,
  IN ULONG  Attributes,
  IN PACCESS_STATE  PassedAccessState  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess  OPTIONAL,
  IN POBJECT_TYPE  ObjectType,
  IN KPROCESSOR_MODE  AccessMode,
  IN OUT PVOID  ParseContext  OPTIONAL,
  OUT PVOID  *Object);

NTOSAPI
NTSTATUS
DDKAPI
ObReferenceObjectByPointer(
  IN PVOID  Object,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_TYPE  ObjectType,
  IN KPROCESSOR_MODE  AccessMode);

NTOSAPI
VOID
DDKAPI
ObReferenceSecurityDescriptor(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  Count);

NTOSAPI
VOID
DDKAPI
ObReleaseObjectSecurity(
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN BOOLEAN  MemoryAllocated);



/** Process manager routines **/

NTOSAPI
NTSTATUS
DDKAPI
PsCreateSystemProcess(
  IN PHANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
HANDLE
DDKAPI
PsGetCurrentProcessId(
  VOID);

/*
 * PETHREAD
 * PsGetCurrentThread(VOID)
 */
#define PsGetCurrentThread() \
  ((PETHREAD) KeGetCurrentThread())

NTOSAPI
HANDLE
DDKAPI
PsGetCurrentThreadId(
  VOID);

NTOSAPI
BOOLEAN
DDKAPI
PsGetVersion(
  PULONG  MajorVersion  OPTIONAL,
  PULONG  MinorVersion  OPTIONAL,
  PULONG  BuildNumber  OPTIONAL,
  PUNICODE_STRING  CSDVersion  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
PsRemoveCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE  NotifyRoutine);

NTOSAPI
NTSTATUS
DDKAPI
PsRemoveLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE  NotifyRoutine);

NTOSAPI
NTSTATUS
DDKAPI
PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE  NotifyRoutine,
  IN BOOLEAN  Remove);

NTOSAPI
NTSTATUS
DDKAPI
PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE  NotifyRoutine);

NTOSAPI
NTSTATUS
DDKAPI
PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE  NotifyRoutine);

NTOSAPI
NTSTATUS
DDKAPI
PsTerminateSystemThread(
  IN NTSTATUS  ExitStatus);



/** Security reference monitor routines **/

NTOSAPI
BOOLEAN
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
SeAssignSecurity(
  IN PSECURITY_DESCRIPTOR  ParentDescriptor  OPTIONAL,
  IN PSECURITY_DESCRIPTOR  ExplicitDescriptor  OPTIONAL,
  OUT PSECURITY_DESCRIPTOR  *NewDescriptor,
  IN BOOLEAN  IsDirectoryObject,
  IN PSECURITY_SUBJECT_CONTEXT  SubjectContext,
  IN PGENERIC_MAPPING  GenericMapping,
  IN POOL_TYPE  PoolType);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
SeDeassignSecurity(
  IN OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor);

NTOSAPI
BOOLEAN
DDKAPI
SeSinglePrivilegeCheck(
  LUID  PrivilegeValue,
  KPROCESSOR_MODE  PreviousMode);

NTOSAPI
BOOLEAN
DDKAPI
SeValidSecurityDescriptor(
  IN ULONG  Length,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);



/** NtXxx routines **/

NTOSAPI
NTSTATUS
DDKAPI
NtOpenProcess(
  OUT PHANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PCLIENT_ID  ClientId  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
NtQueryInformationProcess(
  IN HANDLE  ProcessHandle,
  IN PROCESSINFOCLASS  ProcessInformationClass,
  OUT PVOID  ProcessInformation,
  IN ULONG  ProcessInformationLength,
  OUT PULONG  ReturnLength OPTIONAL);



/** NtXxx and ZwXxx routines **/

NTOSAPI
NTSTATUS
DDKAPI
ZwCancelTimer(
  IN HANDLE  TimerHandle,
  OUT PBOOLEAN  CurrentState  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
NtClose(
  IN HANDLE  Handle);

NTOSAPI
NTSTATUS
DDKAPI
ZwClose(
  IN HANDLE  Handle);

NTOSAPI
NTSTATUS
DDKAPI
ZwCreateDirectoryObject(
  OUT PHANDLE  DirectoryHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
NtCreateEvent(
  OUT PHANDLE  EventHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  ManualReset,
  IN BOOLEAN  InitialState);

NTOSAPI
NTSTATUS
DDKAPI
ZwCreateEvent(
  OUT PHANDLE  EventHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  ManualReset,
  IN BOOLEAN  InitialState);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
ZwCreateKey(
  OUT PHANDLE  KeyHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN ULONG  TitleIndex,
  IN PUNICODE_STRING  Class  OPTIONAL,
  IN ULONG  CreateOptions,
  OUT PULONG  Disposition  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
ZwCreateTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN TIMER_TYPE  TimerType);

NTOSAPI
NTSTATUS
DDKAPI
ZwDeleteKey(
  IN HANDLE  KeyHandle);

NTOSAPI
NTSTATUS
DDKAPI
ZwDeleteValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
ZwEnumerateKey(
  IN HANDLE  KeyHandle,
  IN ULONG  Index,
  IN KEY_INFORMATION_CLASS  KeyInformationClass,
  OUT PVOID  KeyInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTOSAPI
NTSTATUS
DDKAPI
ZwEnumerateValueKey(
  IN HANDLE  KeyHandle,
  IN ULONG  Index,
  IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
  OUT PVOID  KeyValueInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTOSAPI
NTSTATUS
DDKAPI
ZwFlushKey(
  IN HANDLE  KeyHandle);

NTOSAPI
NTSTATUS
DDKAPI
ZwMakeTemporaryObject(
  IN HANDLE  Handle);

NTOSAPI
NTSTATUS
DDKAPI
NtMapViewOfSection(
  IN HANDLE  SectionHandle,
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN ULONG  ZeroBits,
  IN ULONG  CommitSize,
  IN OUT PLARGE_INTEGER  SectionOffset  OPTIONAL,
  IN OUT PSIZE_T  ViewSize,
  IN SECTION_INHERIT  InheritDisposition,
  IN ULONG  AllocationType,
  IN ULONG  Protect);

NTOSAPI
NTSTATUS
DDKAPI
ZwMapViewOfSection(
  IN HANDLE  SectionHandle,
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN ULONG  ZeroBits,
  IN ULONG  CommitSize,
  IN OUT PLARGE_INTEGER  SectionOffset  OPTIONAL,
  IN OUT PSIZE_T  ViewSize,
  IN SECTION_INHERIT  InheritDisposition,
  IN ULONG  AllocationType,
  IN ULONG  Protect);

NTOSAPI
NTSTATUS
DDKAPI
NtOpenFile(
  OUT PHANDLE  FileHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  ShareAccess,
  IN ULONG  OpenOptions);

NTOSAPI
NTSTATUS
DDKAPI
ZwOpenFile(
  OUT PHANDLE  FileHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  ShareAccess,
  IN ULONG  OpenOptions);

NTOSAPI
NTSTATUS
DDKAPI
ZwOpenKey(
  OUT PHANDLE  KeyHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
ZwOpenSection(
  OUT PHANDLE  SectionHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
ZwOpenSymbolicLinkObject(
  OUT PHANDLE  LinkHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
ZwOpenTimer(
  OUT PHANDLE  TimerHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
DDKAPI
ZwQueryInformationFile(
  IN HANDLE  FileHandle,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  OUT PVOID  FileInformation,
  IN ULONG  Length,
  IN FILE_INFORMATION_CLASS  FileInformationClass);

NTOSAPI
NTSTATUS
DDKAPI
ZwQueryKey(
  IN HANDLE  KeyHandle,
  IN KEY_INFORMATION_CLASS  KeyInformationClass,
  OUT PVOID  KeyInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTOSAPI
NTSTATUS
DDKAPI
ZwQuerySymbolicLinkObject(
  IN HANDLE  LinkHandle,
  IN OUT PUNICODE_STRING  LinkTarget,
  OUT PULONG  ReturnedLength  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
ZwQueryValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName,
  IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
  OUT PVOID  KeyValueInformation,
  IN ULONG  Length,
  OUT PULONG  ResultLength);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
NtSetEvent(
  IN HANDLE  EventHandle,
  IN PULONG  NumberOfThreadsReleased);

NTOSAPI
NTSTATUS
DDKAPI
ZwSetEvent(
  IN HANDLE  EventHandle,
  IN PULONG  NumberOfThreadsReleased);

NTOSAPI
NTSTATUS
DDKAPI
ZwSetInformationFile(
  IN HANDLE  FileHandle,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN PVOID  FileInformation,
  IN ULONG  Length,
  IN FILE_INFORMATION_CLASS  FileInformationClass);

NTOSAPI
NTSTATUS
DDKAPI
ZwSetInformationThread(
  IN HANDLE  ThreadHandle,
  IN THREADINFOCLASS  ThreadInformationClass,
  IN PVOID  ThreadInformation,
  IN ULONG  ThreadInformationLength);

NTOSAPI
NTSTATUS
DDKAPI
ZwSetTimer(
  IN HANDLE  TimerHandle,
  IN PLARGE_INTEGER  DueTime,
  IN PTIMER_APC_ROUTINE  TimerApcRoutine  OPTIONAL,
  IN PVOID  TimerContext  OPTIONAL,
  IN BOOLEAN  WakeTimer,
  IN LONG  Period  OPTIONAL,
  OUT PBOOLEAN  PreviousState  OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
ZwSetValueKey(
  IN HANDLE  KeyHandle,
  IN PUNICODE_STRING  ValueName,
  IN ULONG  TitleIndex  OPTIONAL,
  IN ULONG  Type,
  IN PVOID  Data,
  IN ULONG  DataSize);

/* [Nt|Zw]MapViewOfSection.InheritDisposition constants */
#define AT_EXTENDABLE_FILE                0x00002000
#define SEC_NO_CHANGE                     0x00400000
#define AT_RESERVED                       0x20000000
#define AT_ROUND_TO_PAGE                  0x40000000

NTOSAPI
NTSTATUS
DDKAPI
NtUnmapViewOfSection(
  IN HANDLE  ProcessHandle,
  IN PVOID  BaseAddress);

NTOSAPI
NTSTATUS
DDKAPI
ZwUnmapViewOfSection(
  IN HANDLE  ProcessHandle,
  IN PVOID  BaseAddress);

NTOSAPI
NTSTATUS
DDKAPI
NtWaitForSingleObject(
  IN HANDLE  Object,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Time);

NTOSAPI
NTSTATUS
DDKAPI
ZwWaitForSingleObject(
  IN HANDLE  Object,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Time);

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
PoCallDriver(
  IN PDEVICE_OBJECT  DeviceObject,
  IN OUT PIRP  Irp);

NTOSAPI
PULONG
DDKAPI
PoRegisterDeviceForIdleDetection(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  ConservationIdleTime,
  IN ULONG  PerformanceIdleTime,
  IN DEVICE_POWER_STATE  State);

NTOSAPI
PVOID
DDKAPI
PoRegisterSystemState(
  IN PVOID  StateHandle,
  IN EXECUTION_STATE  Flags);

NTOSAPI
NTSTATUS
DDKAPI
PoRequestPowerIrp(
  IN PDEVICE_OBJECT  DeviceObject,
  IN UCHAR  MinorFunction,  
  IN POWER_STATE  PowerState,
  IN PREQUEST_POWER_COMPLETE  CompletionFunction,
  IN PVOID  Context,
  OUT PIRP  *Irp OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
PoRequestShutdownEvent(
  OUT PVOID  *Event);

NTOSAPI
VOID
DDKAPI
PoSetDeviceBusy(
  PULONG  IdlePointer); 

NTOSAPI
POWER_STATE
DDKAPI
PoSetPowerState(
  IN PDEVICE_OBJECT  DeviceObject,
  IN POWER_STATE_TYPE  Type,
  IN POWER_STATE  State);

NTOSAPI
VOID
DDKAPI
PoSetSystemState(
  IN EXECUTION_STATE  Flags);

NTOSAPI
VOID
DDKAPI
PoStartNextPowerIrp(
  IN PIRP  Irp);

NTOSAPI
VOID
DDKAPI
PoUnregisterSystemState(
  IN PVOID  StateHandle);



/** WMI library support routines **/

NTOSAPI
NTSTATUS
DDKAPI
WmiCompleteRequest(
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN NTSTATUS  Status,
  IN ULONG  BufferUsed,
  IN CCHAR  PriorityBoost);

NTOSAPI
NTSTATUS
DDKAPI
WmiFireEvent(
  IN PDEVICE_OBJECT  DeviceObject,
  IN LPGUID  Guid, 
  IN ULONG  InstanceIndex,
  IN ULONG  EventDataSize,
  IN PVOID  EventData); 

NTOSAPI
NTSTATUS
DDKAPI
WmiQueryTraceInformation(
  IN TRACE_INFORMATION_CLASS  TraceInformationClass,
  OUT PVOID  TraceInformation,
  IN ULONG  TraceInformationLength,
  OUT PULONG  RequiredLength OPTIONAL,
  IN PVOID  Buffer OPTIONAL);

NTOSAPI
NTSTATUS
DDKAPI
WmiSystemControl(
  IN PWMILIB_CONTEXT  WmiLibInfo,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  OUT PSYSCTL_IRP_DISPOSITION  IrpDisposition);

NTOSAPI
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
NTOSAPI
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

NTOSAPI
VOID
DDKAPI
KdDisableDebugger(
  VOID);

NTOSAPI
VOID
DDKAPI
KdEnableDebugger(
  VOID);

NTOSAPI
VOID
DDKAPI
DbgBreakPoint(
  VOID);

NTOSAPI
VOID
DDKAPI
DbgBreakPointWithStatus(
  IN ULONG  Status);

NTOSAPI
ULONG
DDKCDECLAPI
DbgPrint(
  IN PCH  Format,
  IN ...);

NTOSAPI
ULONG
DDKCDECLAPI
DbgPrintEx(
  IN ULONG  ComponentId,
  IN ULONG  Level,
  IN PCH  Format,
  IN ...);

NTOSAPI
ULONG
DDKCDECLAPI
DbgPrintReturnControlC(
  IN PCH  Format,
  IN ...);

NTOSAPI
NTSTATUS
DDKAPI
DbgQueryDebugFilterState(
  IN ULONG  ComponentId,
  IN ULONG  Level);

NTOSAPI
NTSTATUS
DDKAPI
DbgSetDebugFilterState(
  IN ULONG  ComponentId,
  IN ULONG  Level,
  IN BOOLEAN  State);

NTOSAPI
BOOLEAN
DDKAPI
KeRosPrintAddress ( PVOID address );

NTOSAPI
VOID
DDKAPI
KeRosDumpStackFrames ( PULONG Frame, ULONG FrameCount );


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

extern NTOSAPI PBOOLEAN KdDebuggerNotPresent;
extern NTOSAPI PBOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     *KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT *KdDebuggerNotPresent

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __WINDDK_H */
