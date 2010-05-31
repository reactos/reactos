/******************************************************************************
 *                         I/O Manager Types                                  *
 ******************************************************************************/

$if (_WDMDDK_)
#define WDM_MAJORVERSION        0x06
#define WDM_MINORVERSION        0x00

#if defined(_WIN64)

#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif /* defined(_WIN64) */

#define STATUS_CONTINUE_COMPLETION      STATUS_SUCCESS

#define CONNECT_FULLY_SPECIFIED         0x1
#define CONNECT_LINE_BASED              0x2
#define CONNECT_MESSAGE_BASED           0x3
#define CONNECT_FULLY_SPECIFIED_GROUP   0x4
#define CONNECT_CURRENT_VERSION         0x4

#define POOL_COLD_ALLOCATION                256
#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE    8
#define POOL_RAISE_IF_ALLOCATION_FAILURE    16

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
#define IO_TYPE_CSQ_EX 3

/* IO_RESOURCE_DESCRIPTOR.Option */
#define IO_RESOURCE_PREFERRED             0x01
#define IO_RESOURCE_DEFAULT               0x02
#define IO_RESOURCE_ALTERNATIVE           0x08

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
#define FILE_DEVICE_FIPS                  0x0000003A
#define FILE_DEVICE_INFINIBAND            0x0000003B
#define FILE_DEVICE_VMBUS                 0x0000003E
#define FILE_DEVICE_CRYPT_PROVIDER        0x0000003F
#define FILE_DEVICE_WPD                   0x00000040
#define FILE_DEVICE_BLUETOOTH             0x00000041
#define FILE_DEVICE_MT_COMPOSITE          0x00000042
#define FILE_DEVICE_MT_TRANSPORT          0x00000043
#define FILE_DEVICE_BIOMETRIC             0x00000044
#define FILE_DEVICE_PMI                   0x00000045

#if defined(NT_PROCESSOR_GROUPS)

typedef USHORT IRQ_DEVICE_POLICY, *PIRQ_DEVICE_POLICY;

typedef enum _IRQ_DEVICE_POLICY_USHORT {
  IrqPolicyMachineDefault = 0,
  IrqPolicyAllCloseProcessors = 1,
  IrqPolicyOneCloseProcessor = 2,
  IrqPolicyAllProcessorsInMachine = 3,
  IrqPolicyAllProcessorsInGroup = 3,
  IrqPolicySpecifiedProcessors = 4,
  IrqPolicySpreadMessagesAcrossAllProcessors = 5};

#else /* defined(NT_PROCESSOR_GROUPS) */

typedef enum _IRQ_DEVICE_POLICY {
  IrqPolicyMachineDefault = 0,
  IrqPolicyAllCloseProcessors,
  IrqPolicyOneCloseProcessor,
  IrqPolicyAllProcessorsInMachine,
  IrqPolicySpecifiedProcessors,
  IrqPolicySpreadMessagesAcrossAllProcessors
} IRQ_DEVICE_POLICY, *PIRQ_DEVICE_POLICY;

#endif

typedef enum _IRQ_PRIORITY {
  IrqPriorityUndefined = 0,
  IrqPriorityLow,
  IrqPriorityNormal,
  IrqPriorityHigh
} IRQ_PRIORITY, *PIRQ_PRIORITY;

typedef enum _IRQ_GROUP_POLICY {
  GroupAffinityAllGroupZero = 0,
  GroupAffinityDontCare
} IRQ_GROUP_POLICY, *PIRQ_GROUP_POLICY;

#define MAXIMUM_VOLUME_LABEL_LENGTH       (32 * sizeof(WCHAR))

typedef struct _OBJECT_HANDLE_INFORMATION {
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

typedef struct _CLIENT_ID {
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _VPB {
  CSHORT Type;
  CSHORT Size;
  USHORT Flags;
  USHORT VolumeLabelLength;
  struct _DEVICE_OBJECT *DeviceObject;
  struct _DEVICE_OBJECT *RealDevice;
  ULONG SerialNumber;
  ULONG ReferenceCount;
  WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
} VPB, *PVPB;

typedef enum _IO_ALLOCATION_ACTION {
  KeepObject = 1,
  DeallocateObject,
  DeallocateObjectKeepRegisters
} IO_ALLOCATION_ACTION, *PIO_ALLOCATION_ACTION;

typedef IO_ALLOCATION_ACTION
(NTAPI DRIVER_CONTROL)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp,
  IN PVOID MapRegisterBase,
  IN PVOID Context);
typedef DRIVER_CONTROL *PDRIVER_CONTROL;

typedef struct _WAIT_CONTEXT_BLOCK {
  KDEVICE_QUEUE_ENTRY WaitQueueEntry;
  PDRIVER_CONTROL DeviceRoutine;
  PVOID DeviceContext;
  ULONG NumberOfMapRegisters;
  PVOID DeviceObject;
  PVOID CurrentIrp;
  PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

$endif
/* DEVICE_OBJECT.Flags */
$if (_NTDDK_)
#define DO_DEVICE_HAS_NAME                0x00000040
#define DO_SYSTEM_BOOT_PARTITION          0x00000100
#define DO_LONG_TERM_REQUESTS             0x00000200
#define DO_NEVER_LAST_DEVICE              0x00000400
#define DO_LOW_PRIORITY_FILESYSTEM        0x00010000
#define DO_SUPPORTS_TRANSACTIONS          0x00040000
#define DO_FORCE_NEITHER_IO               0x00080000
#define DO_VOLUME_DEVICE_OBJECT           0x00100000
#define DO_SYSTEM_SYSTEM_PARTITION        0x00200000
#define DO_SYSTEM_CRITICAL_PARTITION      0x00400000
#define DO_DISALLOW_EXECUTE               0x00800000
$endif
$if (_WDMDDK_)
#define DO_VERIFY_VOLUME                  0x00000002
#define DO_BUFFERED_IO                    0x00000004
#define DO_EXCLUSIVE                      0x00000008
#define DO_DIRECT_IO                      0x00000010
#define DO_MAP_IO_BUFFER                  0x00000020
#define DO_DEVICE_INITIALIZING            0x00000080
#define DO_SHUTDOWN_REGISTERED            0x00000800
#define DO_BUS_ENUMERATED_DEVICE          0x00001000
#define DO_POWER_PAGABLE                  0x00002000
#define DO_POWER_INRUSH                   0x00004000

/* DEVICE_OBJECT.Characteristics */
#define FILE_REMOVABLE_MEDIA              0x00000001
#define FILE_READ_ONLY_DEVICE             0x00000002
#define FILE_FLOPPY_DISKETTE              0x00000004
#define FILE_WRITE_ONCE_MEDIA             0x00000008
#define FILE_REMOTE_DEVICE                0x00000010
#define FILE_DEVICE_IS_MOUNTED            0x00000020
#define FILE_VIRTUAL_VOLUME               0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME    0x00000080
#define FILE_DEVICE_SECURE_OPEN           0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE    0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE     0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE 0x00002000

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

typedef struct _DEVICE_OBJECT {
  CSHORT Type;
  USHORT Size;
  LONG ReferenceCount;
  struct _DRIVER_OBJECT *DriverObject;
  struct _DEVICE_OBJECT *NextDevice;
  struct _DEVICE_OBJECT *AttachedDevice;
  struct _IRP *CurrentIrp;
  PIO_TIMER Timer;
  ULONG Flags;
  ULONG Characteristics;
  volatile PVPB Vpb;
  PVOID DeviceExtension;
  DEVICE_TYPE DeviceType;
  CCHAR StackSize;
  union {
    LIST_ENTRY ListEntry;
    WAIT_CONTEXT_BLOCK Wcb;
  } Queue;
  ULONG AlignmentRequirement;
  KDEVICE_QUEUE DeviceQueue;
  KDPC Dpc;
  ULONG ActiveThreadCount;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
  KEVENT DeviceLock;
  USHORT SectorSize;
  USHORT Spare1;
  struct _DEVOBJ_EXTENSION *DeviceObjectExtension;
  PVOID Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

typedef enum _IO_SESSION_STATE {
  IoSessionStateCreated = 1,
  IoSessionStateInitialized,
  IoSessionStateConnected,
  IoSessionStateDisconnected,
  IoSessionStateDisconnectedLoggedOn,
  IoSessionStateLoggedOn,
  IoSessionStateLoggedOff,
  IoSessionStateTerminated,
  IoSessionStateMax
} IO_SESSION_STATE, *PIO_SESSION_STATE;

typedef enum _IO_COMPLETION_ROUTINE_RESULT {
  ContinueCompletion = STATUS_CONTINUE_COMPLETION,
  StopCompletion = STATUS_MORE_PROCESSING_REQUIRED
} IO_COMPLETION_ROUTINE_RESULT, *PIO_COMPLETION_ROUTINE_RESULT;

typedef struct _IO_INTERRUPT_MESSAGE_INFO_ENTRY {
  PHYSICAL_ADDRESS MessageAddress;
  KAFFINITY TargetProcessorSet;
  PKINTERRUPT InterruptObject;
  ULONG MessageData;
  ULONG Vector;
  KIRQL Irql;
  KINTERRUPT_MODE Mode;
  KINTERRUPT_POLARITY Polarity;
} IO_INTERRUPT_MESSAGE_INFO_ENTRY, *PIO_INTERRUPT_MESSAGE_INFO_ENTRY;

typedef struct _IO_INTERRUPT_MESSAGE_INFO {
  KIRQL UnifiedIrql;
  ULONG MessageCount;
  IO_INTERRUPT_MESSAGE_INFO_ENTRY MessageInfo[1];
} IO_INTERRUPT_MESSAGE_INFO, *PIO_INTERRUPT_MESSAGE_INFO;

typedef struct _IO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS {
  IN PDEVICE_OBJECT PhysicalDeviceObject;
  OUT PKINTERRUPT *InterruptObject;
  IN PKSERVICE_ROUTINE ServiceRoutine;
  IN PVOID ServiceContext;
  IN PKSPIN_LOCK SpinLock OPTIONAL;
  IN KIRQL SynchronizeIrql;
  IN BOOLEAN FloatingSave;
  IN BOOLEAN ShareVector;
  IN ULONG Vector;
  IN KIRQL Irql;
  IN KINTERRUPT_MODE InterruptMode;
  IN KAFFINITY ProcessorEnableMask;
  IN USHORT Group;
} IO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS, *PIO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS;

typedef struct _IO_CONNECT_INTERRUPT_LINE_BASED_PARAMETERS {
  IN PDEVICE_OBJECT PhysicalDeviceObject;
  OUT PKINTERRUPT *InterruptObject;
  IN PKSERVICE_ROUTINE ServiceRoutine;
  IN PVOID ServiceContext;
  IN PKSPIN_LOCK SpinLock OPTIONAL;
  IN KIRQL SynchronizeIrql OPTIONAL;
  IN BOOLEAN FloatingSave;
} IO_CONNECT_INTERRUPT_LINE_BASED_PARAMETERS, *PIO_CONNECT_INTERRUPT_LINE_BASED_PARAMETERS;

typedef struct _IO_CONNECT_INTERRUPT_MESSAGE_BASED_PARAMETERS {
  IN PDEVICE_OBJECT PhysicalDeviceObject;
  union {
    OUT PVOID *Generic;
    OUT PIO_INTERRUPT_MESSAGE_INFO *InterruptMessageTable;
    OUT PKINTERRUPT *InterruptObject;
  } ConnectionContext;
  IN PKMESSAGE_SERVICE_ROUTINE MessageServiceRoutine;
  IN PVOID ServiceContext;
  IN PKSPIN_LOCK SpinLock OPTIONAL;
  IN KIRQL SynchronizeIrql OPTIONAL;
  IN BOOLEAN FloatingSave;
  IN PKSERVICE_ROUTINE FallBackServiceRoutine OPTIONAL;
} IO_CONNECT_INTERRUPT_MESSAGE_BASED_PARAMETERS, *PIO_CONNECT_INTERRUPT_MESSAGE_BASED_PARAMETERS;

typedef struct _IO_CONNECT_INTERRUPT_PARAMETERS {
  IN OUT ULONG Version;
  union {
    IO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS FullySpecified;
    IO_CONNECT_INTERRUPT_LINE_BASED_PARAMETERS LineBased;
    IO_CONNECT_INTERRUPT_MESSAGE_BASED_PARAMETERS MessageBased;
  };
} IO_CONNECT_INTERRUPT_PARAMETERS, *PIO_CONNECT_INTERRUPT_PARAMETERS;

typedef struct _IO_DISCONNECT_INTERRUPT_PARAMETERS {
  IN ULONG Version;
  union {
    IN PVOID Generic;
    IN PKINTERRUPT InterruptObject;
    IN PIO_INTERRUPT_MESSAGE_INFO InterruptMessageTable;
  } ConnectionContext;
} IO_DISCONNECT_INTERRUPT_PARAMETERS, *PIO_DISCONNECT_INTERRUPT_PARAMETERS;

typedef enum _IO_ACCESS_TYPE {
  ReadAccess,
  WriteAccess,
  ModifyAccess
} IO_ACCESS_TYPE;

typedef enum _IO_ACCESS_MODE {
  SequentialAccess,
  RandomAccess
} IO_ACCESS_MODE;

typedef enum _IO_CONTAINER_NOTIFICATION_CLASS {
  IoSessionStateNotification,
  IoMaxContainerNotificationClass
} IO_CONTAINER_NOTIFICATION_CLASS;

typedef struct _IO_SESSION_STATE_NOTIFICATION {
  ULONG Size;
  ULONG Flags;
  PVOID IoObject;
  ULONG EventMask;
  PVOID Context;
} IO_SESSION_STATE_NOTIFICATION, *PIO_SESSION_STATE_NOTIFICATION;

typedef enum _IO_CONTAINER_INFORMATION_CLASS {
  IoSessionStateInformation,
  IoMaxContainerInformationClass
} IO_CONTAINER_INFORMATION_CLASS;

typedef struct _IO_SESSION_STATE_INFORMATION {
  ULONG SessionId;
  IO_SESSION_STATE SessionState;
  BOOLEAN LocalSession;
} IO_SESSION_STATE_INFORMATION, *PIO_SESSION_STATE_INFORMATION;

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef NTSTATUS
(NTAPI *PIO_CONTAINER_NOTIFICATION_FUNCTION)(
  VOID);

typedef NTSTATUS
(NTAPI IO_SESSION_NOTIFICATION_FUNCTION)(
  IN PVOID SessionObject,
  IN PVOID IoObject,
  IN ULONG Event,
  IN PVOID Context,
  IN PVOID NotificationPayload,
  IN ULONG PayloadLength);

typedef IO_SESSION_NOTIFICATION_FUNCTION *PIO_SESSION_NOTIFICATION_FUNCTION;

#endif

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK * PIO_REMOVE_LOCK_TRACKING_BLOCK;

typedef struct _IO_REMOVE_LOCK_COMMON_BLOCK {
  BOOLEAN Removed;
  BOOLEAN Reserved[3];
  volatile LONG IoCount;
  KEVENT RemoveEvent;
} IO_REMOVE_LOCK_COMMON_BLOCK;

typedef struct _IO_REMOVE_LOCK_DBG_BLOCK {
  LONG Signature;
  LONG HighWatermark;
  LONGLONG MaxLockedTicks;
  LONG AllocateTag;
  LIST_ENTRY LockList;
  KSPIN_LOCK Spin;
  volatile LONG LowMemoryCount;
  ULONG Reserved1[4];
  PVOID Reserved2;
  PIO_REMOVE_LOCK_TRACKING_BLOCK Blocks;
} IO_REMOVE_LOCK_DBG_BLOCK;

typedef struct _IO_REMOVE_LOCK {
  IO_REMOVE_LOCK_COMMON_BLOCK Common;
#if DBG
  IO_REMOVE_LOCK_DBG_BLOCK Dbg;
#endif
} IO_REMOVE_LOCK, *PIO_REMOVE_LOCK;

typedef struct _IO_WORKITEM *PIO_WORKITEM;

typedef VOID
(NTAPI IO_WORKITEM_ROUTINE)(
  IN PDEVICE_OBJECT DeviceObject,
  IN PVOID Context);
typedef IO_WORKITEM_ROUTINE *PIO_WORKITEM_ROUTINE;

typedef VOID
(NTAPI IO_WORKITEM_ROUTINE_EX)(
  IN PVOID IoObject,
  IN PVOID Context OPTIONAL,
  IN PIO_WORKITEM IoWorkItem);
typedef IO_WORKITEM_ROUTINE_EX *PIO_WORKITEM_ROUTINE_EX;

typedef struct _SHARE_ACCESS {
  ULONG OpenCount;
  ULONG Readers;
  ULONG Writers;
  ULONG Deleters;
  ULONG SharedRead;
  ULONG SharedWrite;
  ULONG SharedDelete;
} SHARE_ACCESS, *PSHARE_ACCESS;

typedef enum _CREATE_FILE_TYPE {
  CreateFileTypeNone,
  CreateFileTypeNamedPipe,
  CreateFileTypeMailslot
} CREATE_FILE_TYPE;

#define IO_FORCE_ACCESS_CHECK               0x001
#define IO_NO_PARAMETER_CHECKING            0x100

#define IO_REPARSE                      0x0
#define IO_REMOUNT                      0x1

typedef struct _IO_STATUS_BLOCK {
  _ANONYMOUS_UNION union {
    NTSTATUS Status;
    PVOID Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#if defined(_WIN64)
typedef struct _IO_STATUS_BLOCK32 {
  NTSTATUS Status;
  ULONG Information;
} IO_STATUS_BLOCK32, *PIO_STATUS_BLOCK32;
#endif

typedef VOID
(NTAPI *PIO_APC_ROUTINE)(
  IN PVOID ApcContext,
  IN PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG Reserved);

#define PIO_APC_ROUTINE_DEFINED

typedef enum _IO_SESSION_EVENT {
  IoSessionEventIgnore = 0,
  IoSessionEventCreated,
  IoSessionEventTerminated,
  IoSessionEventConnected,
  IoSessionEventDisconnected,
  IoSessionEventLogon,
  IoSessionEventLogoff,
  IoSessionEventMax
} IO_SESSION_EVENT, *PIO_SESSION_EVENT;

#define IO_SESSION_STATE_ALL_EVENTS        0xffffffff
#define IO_SESSION_STATE_CREATION_EVENT    0x00000001
#define IO_SESSION_STATE_TERMINATION_EVENT 0x00000002
#define IO_SESSION_STATE_CONNECT_EVENT     0x00000004
#define IO_SESSION_STATE_DISCONNECT_EVENT  0x00000008
#define IO_SESSION_STATE_LOGON_EVENT       0x00000010
#define IO_SESSION_STATE_LOGOFF_EVENT      0x00000020

#define IO_SESSION_STATE_VALID_EVENT_MASK  0x0000003f

#define IO_SESSION_MAX_PAYLOAD_SIZE        256L

typedef struct _IO_SESSION_CONNECT_INFO {
  ULONG SessionId;
  BOOLEAN LocalSession;
} IO_SESSION_CONNECT_INFO, *PIO_SESSION_CONNECT_INFO;

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

typedef struct _BOOTDISK_INFORMATION {
  LONGLONG BootPartitionOffset;
  LONGLONG SystemPartitionOffset;
  ULONG BootDeviceSignature;
  ULONG SystemDeviceSignature;
} BOOTDISK_INFORMATION, *PBOOTDISK_INFORMATION;

typedef struct _BOOTDISK_INFORMATION_EX {
  LONGLONG BootPartitionOffset;
  LONGLONG SystemPartitionOffset;
  ULONG BootDeviceSignature;
  ULONG SystemDeviceSignature;
  GUID BootDeviceGuid;
  GUID SystemDeviceGuid;
  BOOLEAN BootDeviceIsGpt;
  BOOLEAN SystemDeviceIsGpt;
} BOOTDISK_INFORMATION_EX, *PBOOTDISK_INFORMATION_EX;

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct _LOADER_PARTITION_INFORMATION_EX {
  ULONG PartitionStyle;
  ULONG PartitionNumber;
  union {
    ULONG Signature;
    GUID DeviceId;
  };
  ULONG Flags;
} LOADER_PARTITION_INFORMATION_EX, *PLOADER_PARTITION_INFORMATION_EX;

typedef struct _BOOTDISK_INFORMATION_LITE {
  ULONG NumberEntries;
  LOADER_PARTITION_INFORMATION_EX Entries[1];
} BOOTDISK_INFORMATION_LITE, *PBOOTDISK_INFORMATION_LITE;

#else

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef struct _BOOTDISK_INFORMATION_LITE {
  ULONG BootDeviceSignature;
  ULONG SystemDeviceSignature;
  GUID BootDeviceGuid;
  GUID SystemDeviceGuid;
  BOOLEAN BootDeviceIsGpt;
  BOOLEAN SystemDeviceIsGpt;
} BOOTDISK_INFORMATION_LITE, *PBOOTDISK_INFORMATION_LITE;
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#include <pshpack1.h>

typedef struct _EISA_MEMORY_TYPE {
  UCHAR ReadWrite:1;
  UCHAR Cached:1;
  UCHAR Reserved0:1;
  UCHAR Type:2;
  UCHAR Shared:1;
  UCHAR Reserved1:1;
  UCHAR MoreEntries:1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION {
  EISA_MEMORY_TYPE ConfigurationByte;
  UCHAR DataSize;
  USHORT AddressLowWord;
  UCHAR AddressHighByte;
  USHORT MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;

typedef struct _EISA_IRQ_DESCRIPTOR {
  UCHAR Interrupt:4;
  UCHAR Reserved:1;
  UCHAR LevelTriggered:1;
  UCHAR Shared:1;
  UCHAR MoreEntries:1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION {
  EISA_IRQ_DESCRIPTOR ConfigurationByte;
  UCHAR Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;

typedef struct _DMA_CONFIGURATION_BYTE0 {
  UCHAR Channel:3;
  UCHAR Reserved:3;
  UCHAR Shared:1;
  UCHAR MoreEntries:1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1 {
  UCHAR Reserved0:2;
  UCHAR TransferSize:2;
  UCHAR Timing:2;
  UCHAR Reserved1:2;
} DMA_CONFIGURATION_BYTE1;

typedef struct _EISA_DMA_CONFIGURATION {
  DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
  DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;

typedef struct _EISA_PORT_DESCRIPTOR {
  UCHAR NumberPorts:5;
  UCHAR Reserved:1;
  UCHAR Shared:1;
  UCHAR MoreEntries:1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION {
  EISA_PORT_DESCRIPTOR Configuration;
  USHORT PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;

typedef struct _CM_EISA_SLOT_INFORMATION {
  UCHAR ReturnCode;
  UCHAR ReturnFlags;
  UCHAR MajorRevision;
  UCHAR MinorRevision;
  USHORT Checksum;
  UCHAR NumberFunctions;
  UCHAR FunctionInformation;
  ULONG CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;

typedef struct _CM_EISA_FUNCTION_INFORMATION {
  ULONG CompressedId;
  UCHAR IdSlotFlags1;
  UCHAR IdSlotFlags2;
  UCHAR MinorRevision;
  UCHAR MajorRevision;
  UCHAR Selections[26];
  UCHAR FunctionFlags;
  UCHAR TypeString[80];
  EISA_MEMORY_CONFIGURATION EisaMemory[9];
  EISA_IRQ_CONFIGURATION EisaIrq[7];
  EISA_DMA_CONFIGURATION EisaDma[4];
  EISA_PORT_CONFIGURATION EisaPort[20];
  UCHAR InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;

#include <poppack.h>

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

#define EISA_MORE_ENTRIES               0x80
#define EISA_SYSTEM_MEMORY              0x00
#define EISA_MEMORY_TYPE_RAM            0x01

/* CM_EISA_SLOT_INFORMATION.ReturnCode */

#define EISA_INVALID_SLOT               0x80
#define EISA_INVALID_FUNCTION           0x81
#define EISA_INVALID_CONFIGURATION      0x82
#define EISA_EMPTY_SLOT                 0x83
#define EISA_INVALID_BIOS_CALL          0x86

/*
** Plug and Play structures
*/

typedef VOID
(NTAPI *PINTERFACE_REFERENCE)(
  PVOID Context);

typedef VOID
(NTAPI *PINTERFACE_DEREFERENCE)(
  PVOID Context);

typedef BOOLEAN
(NTAPI TRANSLATE_BUS_ADDRESS)(
  IN PVOID Context,
  IN PHYSICAL_ADDRESS BusAddress,
  IN ULONG Length,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);
typedef TRANSLATE_BUS_ADDRESS *PTRANSLATE_BUS_ADDRESS;

typedef struct _DMA_ADAPTER*
(NTAPI GET_DMA_ADAPTER)(
  IN PVOID Context,
  IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
  OUT PULONG NumberOfMapRegisters);
typedef GET_DMA_ADAPTER *PGET_DMA_ADAPTER;

typedef ULONG
(NTAPI GET_SET_DEVICE_DATA)(
  IN PVOID Context,
  IN ULONG DataType,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);
typedef GET_SET_DEVICE_DATA *PGET_SET_DEVICE_DATA;

typedef enum _DEVICE_INSTALL_STATE {
  InstallStateInstalled,
  InstallStateNeedsReinstall,
  InstallStateFailedInstall,
  InstallStateFinishInstall
} DEVICE_INSTALL_STATE, *PDEVICE_INSTALL_STATE;

typedef struct _LEGACY_BUS_INFORMATION {
  GUID BusTypeGuid;
  INTERFACE_TYPE LegacyBusType;
  ULONG BusNumber;
} LEGACY_BUS_INFORMATION, *PLEGACY_BUS_INFORMATION;

typedef enum _DEVICE_REMOVAL_POLICY {
  RemovalPolicyExpectNoRemoval = 1,
  RemovalPolicyExpectOrderlyRemoval = 2,
  RemovalPolicyExpectSurpriseRemoval = 3
} DEVICE_REMOVAL_POLICY, *PDEVICE_REMOVAL_POLICY;

typedef VOID
(NTAPI*PREENUMERATE_SELF)(
  IN PVOID Context);

typedef struct _REENUMERATE_SELF_INTERFACE_STANDARD {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PREENUMERATE_SELF SurpriseRemoveAndReenumerateSelf;
} REENUMERATE_SELF_INTERFACE_STANDARD, *PREENUMERATE_SELF_INTERFACE_STANDARD;

typedef VOID
(NTAPI *PIO_DEVICE_EJECT_CALLBACK)(
  IN NTSTATUS Status,
  IN OUT PVOID Context OPTIONAL);

#define PCI_DEVICE_PRESENT_INTERFACE_VERSION     1

/* PCI_DEVICE_PRESENCE_PARAMETERS.Flags */
#define PCI_USE_SUBSYSTEM_IDS   0x00000001
#define PCI_USE_REVISION        0x00000002
#define PCI_USE_VENDEV_IDS      0x00000004
#define PCI_USE_CLASS_SUBCLASS  0x00000008
#define PCI_USE_PROGIF          0x00000010
#define PCI_USE_LOCAL_BUS       0x00000020
#define PCI_USE_LOCAL_DEVICE    0x00000040

typedef struct _PCI_DEVICE_PRESENCE_PARAMETERS {
  ULONG Size;
  ULONG Flags;
  USHORT VendorID;
  USHORT DeviceID;
  UCHAR RevisionID;
  USHORT SubVendorID;
  USHORT SubSystemID;
  UCHAR BaseClass;
  UCHAR SubClass;
  UCHAR ProgIf;
} PCI_DEVICE_PRESENCE_PARAMETERS, *PPCI_DEVICE_PRESENCE_PARAMETERS;

typedef BOOLEAN
(NTAPI PCI_IS_DEVICE_PRESENT)(
  IN USHORT VendorID,
  IN USHORT DeviceID,
  IN UCHAR RevisionID,
  IN USHORT SubVendorID,
  IN USHORT SubSystemID,
  IN ULONG Flags);
typedef PCI_IS_DEVICE_PRESENT *PPCI_IS_DEVICE_PRESENT;

typedef BOOLEAN
(NTAPI PCI_IS_DEVICE_PRESENT_EX)(
  IN PVOID Context,
  IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters);
typedef PCI_IS_DEVICE_PRESENT_EX *PPCI_IS_DEVICE_PRESENT_EX;

typedef struct _BUS_INTERFACE_STANDARD {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSLATE_BUS_ADDRESS TranslateBusAddress;
  PGET_DMA_ADAPTER GetDmaAdapter;
  PGET_SET_DEVICE_DATA SetBusData;
  PGET_SET_DEVICE_DATA GetBusData;
} BUS_INTERFACE_STANDARD, *PBUS_INTERFACE_STANDARD;

typedef struct _PCI_DEVICE_PRESENT_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PPCI_IS_DEVICE_PRESENT IsDevicePresent;
  PPCI_IS_DEVICE_PRESENT_EX IsDevicePresentEx;
} PCI_DEVICE_PRESENT_INTERFACE, *PPCI_DEVICE_PRESENT_INTERFACE;

typedef struct _DEVICE_CAPABILITIES {
  USHORT Size;
  USHORT Version;
  ULONG DeviceD1:1;
  ULONG DeviceD2:1;
  ULONG LockSupported:1;
  ULONG EjectSupported:1;
  ULONG Removable:1;
  ULONG DockDevice:1;
  ULONG UniqueID:1;
  ULONG SilentInstall:1;
  ULONG RawDeviceOK:1;
  ULONG SurpriseRemovalOK:1;
  ULONG WakeFromD0:1;
  ULONG WakeFromD1:1;
  ULONG WakeFromD2:1;
  ULONG WakeFromD3:1;
  ULONG HardwareDisabled:1;
  ULONG NonDynamic:1;
  ULONG WarmEjectSupported:1;
  ULONG NoDisplayInUI:1;
  ULONG Reserved:14;
  ULONG Address;
  ULONG UINumber;
  DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];
  SYSTEM_POWER_STATE SystemWake;
  DEVICE_POWER_STATE DeviceWake;
  ULONG D1Latency;
  ULONG D2Latency;
  ULONG D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _DEVICE_INTERFACE_CHANGE_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  GUID InterfaceClassGuid;
  PUNICODE_STRING SymbolicLinkName;
} DEVICE_INTERFACE_CHANGE_NOTIFICATION, *PDEVICE_INTERFACE_CHANGE_NOTIFICATION;

typedef struct _HWPROFILE_CHANGE_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
} HWPROFILE_CHANGE_NOTIFICATION, *PHWPROFILE_CHANGE_NOTIFICATION;

#undef INTERFACE

typedef struct _INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
} INTERFACE, *PINTERFACE;

typedef struct _PLUGPLAY_NOTIFICATION_HEADER {
  USHORT Version;
  USHORT Size;
  GUID Event;
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
  USHORT Version;
  USHORT Size;
  GUID Event;
  struct _FILE_OBJECT *FileObject;
  LONG NameBufferOffset;
  UCHAR CustomDataBuffer[1];
} TARGET_DEVICE_CUSTOM_NOTIFICATION, *PTARGET_DEVICE_CUSTOM_NOTIFICATION;

typedef struct _TARGET_DEVICE_REMOVAL_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  struct _FILE_OBJECT *FileObject;
} TARGET_DEVICE_REMOVAL_NOTIFICATION, *PTARGET_DEVICE_REMOVAL_NOTIFICATION;

#if (NTDDI_VERSION >= NTDDI_VISTA)
#include <devpropdef.h>
#define PLUGPLAY_PROPERTY_PERSISTENT   0x00000001
#endif

#define PNP_REPLACE_NO_MAP             MAXLONGLONG

typedef NTSTATUS
(NTAPI *PREPLACE_MAP_MEMORY)(
  IN PHYSICAL_ADDRESS TargetPhysicalAddress,
  IN PHYSICAL_ADDRESS SparePhysicalAddress,
  IN OUT PLARGE_INTEGER NumberOfBytes,
  OUT PVOID *TargetAddress,
  OUT PVOID *SpareAddress);

typedef struct _PNP_REPLACE_MEMORY_LIST {
  ULONG AllocatedCount;
  ULONG Count;
  ULONGLONG TotalLength;
  struct {
    PHYSICAL_ADDRESS Address;
    ULONGLONG Length;
  } Ranges[ANYSIZE_ARRAY];
} PNP_REPLACE_MEMORY_LIST, *PPNP_REPLACE_MEMORY_LIST;

typedef struct _PNP_REPLACE_PROCESSOR_LIST {
  PKAFFINITY Affinity;
  ULONG GroupCount;
  ULONG AllocatedCount;
  ULONG Count;
  ULONG ApicIds[ANYSIZE_ARRAY];
} PNP_REPLACE_PROCESSOR_LIST, *PPNP_REPLACE_PROCESSOR_LIST;

typedef struct _PNP_REPLACE_PROCESSOR_LIST_V1 {
  KAFFINITY AffinityMask;
  ULONG AllocatedCount;
  ULONG Count;
  ULONG ApicIds[ANYSIZE_ARRAY];
} PNP_REPLACE_PROCESSOR_LIST_V1, *PPNP_REPLACE_PROCESSOR_LIST_V1;

#define PNP_REPLACE_PARAMETERS_VERSION           2

typedef struct _PNP_REPLACE_PARAMETERS {
  ULONG Size;
  ULONG Version;
  ULONG64 Target;
  ULONG64 Spare;
  PPNP_REPLACE_PROCESSOR_LIST TargetProcessors;
  PPNP_REPLACE_PROCESSOR_LIST SpareProcessors;
  PPNP_REPLACE_MEMORY_LIST TargetMemory;
  PPNP_REPLACE_MEMORY_LIST SpareMemory;
  PREPLACE_MAP_MEMORY MapMemory;
} PNP_REPLACE_PARAMETERS, *PPNP_REPLACE_PARAMETERS;

typedef VOID
(NTAPI *PREPLACE_UNLOAD)(
  VOID);

typedef NTSTATUS
(NTAPI *PREPLACE_BEGIN)(
  IN PPNP_REPLACE_PARAMETERS Parameters,
  OUT PVOID *Context);

typedef NTSTATUS
(NTAPI *PREPLACE_END)(
  IN PVOID Context);

typedef NTSTATUS
(NTAPI *PREPLACE_MIRROR_PHYSICAL_MEMORY)(
  IN PVOID Context,
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN LARGE_INTEGER ByteCount);

typedef NTSTATUS
(NTAPI *PREPLACE_SET_PROCESSOR_ID)(
  IN PVOID Context,
  IN ULONG ApicId,
  IN BOOLEAN Target);

typedef NTSTATUS
(NTAPI *PREPLACE_SWAP)(
  IN PVOID Context);

typedef NTSTATUS
(NTAPI *PREPLACE_INITIATE_HARDWARE_MIRROR)(
  IN PVOID Context);

typedef NTSTATUS
(NTAPI *PREPLACE_MIRROR_PLATFORM_MEMORY)(
  IN PVOID Context);

typedef NTSTATUS
(NTAPI *PREPLACE_GET_MEMORY_DESTINATION)(
  IN PVOID Context,
  IN PHYSICAL_ADDRESS SourceAddress,
  OUT PPHYSICAL_ADDRESS DestinationAddress);

typedef NTSTATUS
(NTAPI *PREPLACE_ENABLE_DISABLE_HARDWARE_QUIESCE)(
  IN PVOID Context,
  IN BOOLEAN Enable);

#define PNP_REPLACE_DRIVER_INTERFACE_VERSION      1
#define PNP_REPLACE_DRIVER_INTERFACE_MINIMUM_SIZE \
             FIELD_OFFSET(PNP_REPLACE_DRIVER_INTERFACE, InitiateHardwareMirror)

#define PNP_REPLACE_MEMORY_SUPPORTED             0x0001
#define PNP_REPLACE_PROCESSOR_SUPPORTED          0x0002
#define PNP_REPLACE_HARDWARE_MEMORY_MIRRORING    0x0004
#define PNP_REPLACE_HARDWARE_PAGE_COPY           0x0008
#define PNP_REPLACE_HARDWARE_QUIESCE             0x0010

typedef struct _PNP_REPLACE_DRIVER_INTERFACE {
  ULONG Size;
  ULONG Version;
  ULONG Flags;
  PREPLACE_UNLOAD Unload;
  PREPLACE_BEGIN BeginReplace;
  PREPLACE_END EndReplace;
  PREPLACE_MIRROR_PHYSICAL_MEMORY MirrorPhysicalMemory;
  PREPLACE_SET_PROCESSOR_ID SetProcessorId;
  PREPLACE_SWAP Swap;
  PREPLACE_INITIATE_HARDWARE_MIRROR InitiateHardwareMirror;
  PREPLACE_MIRROR_PLATFORM_MEMORY MirrorPlatformMemory;
  PREPLACE_GET_MEMORY_DESTINATION GetMemoryDestination;
  PREPLACE_ENABLE_DISABLE_HARDWARE_QUIESCE EnableDisableHardwareQuiesce;
} PNP_REPLACE_DRIVER_INTERFACE, *PPNP_REPLACE_DRIVER_INTERFACE;

typedef NTSTATUS
(NTAPI *PREPLACE_DRIVER_INIT)(
  IN OUT PPNP_REPLACE_DRIVER_INTERFACE Interface,
  IN PVOID Unused);

typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE {
  DeviceUsageTypeUndefined,
  DeviceUsageTypePaging,
  DeviceUsageTypeHibernation,
  DeviceUsageTypeDumpFile
} DEVICE_USAGE_NOTIFICATION_TYPE;

typedef struct _POWER_SEQUENCE {
  ULONG SequenceD1;
  ULONG SequenceD2;
  ULONG SequenceD3;
} POWER_SEQUENCE, *PPOWER_SEQUENCE;

typedef enum {
  DevicePropertyDeviceDescription = 0x0,
  DevicePropertyHardwareID = 0x1,
  DevicePropertyCompatibleIDs = 0x2,
  DevicePropertyBootConfiguration = 0x3,
  DevicePropertyBootConfigurationTranslated = 0x4,
  DevicePropertyClassName = 0x5,
  DevicePropertyClassGuid = 0x6,
  DevicePropertyDriverKeyName = 0x7,
  DevicePropertyManufacturer = 0x8,
  DevicePropertyFriendlyName = 0x9,
  DevicePropertyLocationInformation = 0xa,
  DevicePropertyPhysicalDeviceObjectName = 0xb,
  DevicePropertyBusTypeGuid = 0xc,
  DevicePropertyLegacyBusType = 0xd,
  DevicePropertyBusNumber = 0xe,
  DevicePropertyEnumeratorName = 0xf,
  DevicePropertyAddress = 0x10,
  DevicePropertyUINumber = 0x11,
  DevicePropertyInstallState = 0x12,
  DevicePropertyRemovalPolicy = 0x13,
  DevicePropertyResourceRequirements = 0x14,
  DevicePropertyAllocatedResources = 0x15,
  DevicePropertyContainerID = 0x16
} DEVICE_REGISTRY_PROPERTY;

typedef enum _IO_NOTIFICATION_EVENT_CATEGORY {
  EventCategoryReserved,
  EventCategoryHardwareProfileChange,
  EventCategoryDeviceInterfaceChange,
  EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

typedef enum _IO_PRIORITY_HINT {
  IoPriorityVeryLow = 0,
  IoPriorityLow,
  IoPriorityNormal,
  IoPriorityHigh,
  IoPriorityCritical,
  MaxIoPriorityTypes
} IO_PRIORITY_HINT;

#define PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES    0x00000001

typedef NTSTATUS
(NTAPI DRIVER_NOTIFICATION_CALLBACK_ROUTINE)(
  IN PVOID NotificationStructure,
  IN PVOID Context);
typedef DRIVER_NOTIFICATION_CALLBACK_ROUTINE *PDRIVER_NOTIFICATION_CALLBACK_ROUTINE;

typedef VOID
(NTAPI DEVICE_CHANGE_COMPLETE_CALLBACK)(
  IN PVOID Context);
typedef DEVICE_CHANGE_COMPLETE_CALLBACK *PDEVICE_CHANGE_COMPLETE_CALLBACK;

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
  FileIoCompletionNotificationInformation,
  FileIoStatusBlockRangeInformation,
  FileIoPriorityHintInformation,
  FileSfioReserveInformation,
  FileSfioVolumeInformation,
  FileHardLinkInformation,
  FileProcessIdsUsingFileInformation,
  FileNormalizedNameInformation,
  FileNetworkPhysicalNameInformation,
  FileIdGlobalTxDirectoryInformation,
  FileIsRemoteDeviceInformation,
  FileAttributeCacheInformation,
  FileNumaNodeInformation,
  FileStandardLinkInformation,
  FileRemoteProtocolInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_POSITION_INFORMATION {
  LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION {
  IO_PRIORITY_HINT PriorityHint;
} FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;

typedef struct _FILE_IO_COMPLETION_NOTIFICATION_INFORMATION {
  ULONG Flags;
} FILE_IO_COMPLETION_NOTIFICATION_INFORMATION, *PFILE_IO_COMPLETION_NOTIFICATION_INFORMATION;

typedef struct _FILE_IOSTATUSBLOCK_RANGE_INFORMATION {
  PUCHAR IoStatusBlockRange;
  ULONG Length;
} FILE_IOSTATUSBLOCK_RANGE_INFORMATION, *PFILE_IOSTATUSBLOCK_RANGE_INFORMATION;

typedef struct _FILE_IS_REMOTE_DEVICE_INFORMATION {
  BOOLEAN IsRemote;
} FILE_IS_REMOTE_DEVICE_INFORMATION, *PFILE_IS_REMOTE_DEVICE_INFORMATION;

typedef struct _FILE_NUMA_NODE_INFORMATION {
  USHORT NodeNumber;
} FILE_NUMA_NODE_INFORMATION, *PFILE_NUMA_NODE_INFORMATION;

typedef struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION {
  ULONG NumberOfProcessIdsInList;
  ULONG_PTR ProcessIdList[1];
} FILE_PROCESS_IDS_USING_FILE_INFORMATION, *PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER EndOfFile;
  ULONG NumberOfLinks;
  BOOLEAN DeletePending;
  BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER EndOfFile;
  ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

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
  FileFsVolumeFlagsInformation,
  FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_DEVICE_INFORMATION {
  DEVICE_TYPE DeviceType;
  ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
  ULONG NextEntryOffset;
  UCHAR Flags;
  UCHAR EaNameLength;
  USHORT EaValueLength;
  CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_SFIO_RESERVE_INFORMATION {
  ULONG RequestsPerPeriod;
  ULONG Period;
  BOOLEAN RetryFailures;
  BOOLEAN Discardable;
  ULONG RequestSize;
  ULONG NumOutstandingRequests;
} FILE_SFIO_RESERVE_INFORMATION, *PFILE_SFIO_RESERVE_INFORMATION;

typedef struct _FILE_SFIO_VOLUME_INFORMATION {
  ULONG MaximumRequestsPerPeriod;
  ULONG MinimumPeriod;
  ULONG MinimumTransferSize;
} FILE_SFIO_VOLUME_INFORMATION, *PFILE_SFIO_VOLUME_INFORMATION;

#define FILE_SKIP_COMPLETION_PORT_ON_SUCCESS     0x1
#define FILE_SKIP_SET_EVENT_ON_HANDLE            0x2
#define FILE_SKIP_SET_USER_EVENT_ON_FAST_IO      0x4

#define FM_LOCK_BIT             (0x1)
#define FM_LOCK_BIT_V           (0x0)
#define FM_LOCK_WAITER_WOKEN    (0x2)
#define FM_LOCK_WAITER_INC      (0x4)

typedef BOOLEAN
(NTAPI FAST_IO_CHECK_IF_POSSIBLE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN ULONG LockKey,
  IN BOOLEAN CheckForReadOperation,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_CHECK_IF_POSSIBLE *PFAST_IO_CHECK_IF_POSSIBLE;

typedef BOOLEAN
(NTAPI FAST_IO_READ)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN ULONG LockKey,
  OUT PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_READ *PFAST_IO_READ;

typedef BOOLEAN
(NTAPI FAST_IO_WRITE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN BOOLEAN Wait,
  IN ULONG LockKey,
  IN PVOID Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_WRITE *PFAST_IO_WRITE;

typedef BOOLEAN
(NTAPI FAST_IO_QUERY_BASIC_INFO)(
  IN struct _FILE_OBJECT *FileObject,
  IN BOOLEAN Wait,
  OUT PFILE_BASIC_INFORMATION Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_QUERY_BASIC_INFO *PFAST_IO_QUERY_BASIC_INFO;

typedef BOOLEAN
(NTAPI FAST_IO_QUERY_STANDARD_INFO)(
  IN struct _FILE_OBJECT *FileObject,
  IN BOOLEAN Wait,
  OUT PFILE_STANDARD_INFORMATION Buffer,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_QUERY_STANDARD_INFO *PFAST_IO_QUERY_STANDARD_INFO;

typedef BOOLEAN
(NTAPI FAST_IO_LOCK)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  PEPROCESS ProcessId,
  ULONG Key,
  BOOLEAN FailImmediately,
  BOOLEAN ExclusiveLock,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_LOCK *PFAST_IO_LOCK;

typedef BOOLEAN
(NTAPI FAST_IO_UNLOCK_SINGLE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PLARGE_INTEGER Length,
  PEPROCESS ProcessId,
  ULONG Key,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_UNLOCK_SINGLE *PFAST_IO_UNLOCK_SINGLE;

typedef BOOLEAN
(NTAPI FAST_IO_UNLOCK_ALL)(
  IN struct _FILE_OBJECT *FileObject,
  PEPROCESS ProcessId,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_UNLOCK_ALL *PFAST_IO_UNLOCK_ALL;

typedef BOOLEAN
(NTAPI FAST_IO_UNLOCK_ALL_BY_KEY)(
  IN struct _FILE_OBJECT *FileObject,
  PVOID ProcessId,
  ULONG Key,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_UNLOCK_ALL_BY_KEY *PFAST_IO_UNLOCK_ALL_BY_KEY;

typedef BOOLEAN
(NTAPI FAST_IO_DEVICE_CONTROL)(
  IN struct _FILE_OBJECT *FileObject,
  IN BOOLEAN Wait,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength,
  IN ULONG IoControlCode,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_DEVICE_CONTROL *PFAST_IO_DEVICE_CONTROL;

typedef VOID
(NTAPI FAST_IO_ACQUIRE_FILE)(
  IN struct _FILE_OBJECT *FileObject);
typedef FAST_IO_ACQUIRE_FILE *PFAST_IO_ACQUIRE_FILE;

typedef VOID
(NTAPI FAST_IO_RELEASE_FILE)(
  IN struct _FILE_OBJECT *FileObject);
typedef FAST_IO_RELEASE_FILE *PFAST_IO_RELEASE_FILE;

typedef VOID
(NTAPI FAST_IO_DETACH_DEVICE)(
  IN struct _DEVICE_OBJECT *SourceDevice,
  IN struct _DEVICE_OBJECT *TargetDevice);
typedef FAST_IO_DETACH_DEVICE *PFAST_IO_DETACH_DEVICE;

typedef BOOLEAN
(NTAPI FAST_IO_QUERY_NETWORK_OPEN_INFO)(
  IN struct _FILE_OBJECT *FileObject,
  IN BOOLEAN Wait,
  OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
  OUT struct _IO_STATUS_BLOCK *IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_QUERY_NETWORK_OPEN_INFO *PFAST_IO_QUERY_NETWORK_OPEN_INFO;

typedef NTSTATUS
(NTAPI FAST_IO_ACQUIRE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER EndingOffset,
  OUT struct _ERESOURCE **ResourceToRelease,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_ACQUIRE_FOR_MOD_WRITE *PFAST_IO_ACQUIRE_FOR_MOD_WRITE;

typedef BOOLEAN
(NTAPI FAST_IO_MDL_READ)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_MDL_READ *PFAST_IO_MDL_READ;

typedef BOOLEAN
(NTAPI FAST_IO_MDL_READ_COMPLETE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_MDL_READ_COMPLETE *PFAST_IO_MDL_READ_COMPLETE;

typedef BOOLEAN
(NTAPI FAST_IO_PREPARE_MDL_WRITE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_PREPARE_MDL_WRITE *PFAST_IO_PREPARE_MDL_WRITE;

typedef BOOLEAN
(NTAPI FAST_IO_MDL_WRITE_COMPLETE)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_MDL_WRITE_COMPLETE *PFAST_IO_MDL_WRITE_COMPLETE;

typedef BOOLEAN
(NTAPI FAST_IO_READ_COMPRESSED)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  OUT PVOID Buffer,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
  IN ULONG CompressedDataInfoLength,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_READ_COMPRESSED *PFAST_IO_READ_COMPRESSED;

typedef BOOLEAN
(NTAPI FAST_IO_WRITE_COMPRESSED)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN ULONG Length,
  IN ULONG LockKey,
  IN PVOID Buffer,
  OUT PMDL *MdlChain,
  OUT PIO_STATUS_BLOCK IoStatus,
  IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
  IN ULONG CompressedDataInfoLength,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_WRITE_COMPRESSED *PFAST_IO_WRITE_COMPRESSED;

typedef BOOLEAN
(NTAPI FAST_IO_MDL_READ_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT *FileObject,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_MDL_READ_COMPLETE_COMPRESSED *PFAST_IO_MDL_READ_COMPLETE_COMPRESSED;

typedef BOOLEAN
(NTAPI FAST_IO_MDL_WRITE_COMPLETE_COMPRESSED)(
  IN struct _FILE_OBJECT *FileObject,
  IN PLARGE_INTEGER FileOffset,
  IN PMDL MdlChain,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_MDL_WRITE_COMPLETE_COMPRESSED *PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED;

typedef BOOLEAN
(NTAPI FAST_IO_QUERY_OPEN)(
  IN struct _IRP *Irp,
  OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_QUERY_OPEN *PFAST_IO_QUERY_OPEN;

typedef NTSTATUS
(NTAPI FAST_IO_RELEASE_FOR_MOD_WRITE)(
  IN struct _FILE_OBJECT *FileObject,
  IN struct _ERESOURCE *ResourceToRelease,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_RELEASE_FOR_MOD_WRITE *PFAST_IO_RELEASE_FOR_MOD_WRITE;

typedef NTSTATUS
(NTAPI FAST_IO_ACQUIRE_FOR_CCFLUSH)(
  IN struct _FILE_OBJECT *FileObject,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_ACQUIRE_FOR_CCFLUSH *PFAST_IO_ACQUIRE_FOR_CCFLUSH;

typedef NTSTATUS
(NTAPI FAST_IO_RELEASE_FOR_CCFLUSH)(
  IN struct _FILE_OBJECT *FileObject,
  IN struct _DEVICE_OBJECT *DeviceObject);
typedef FAST_IO_RELEASE_FOR_CCFLUSH *PFAST_IO_RELEASE_FOR_CCFLUSH;

typedef struct _FAST_IO_DISPATCH {
  ULONG SizeOfFastIoDispatch;
  PFAST_IO_CHECK_IF_POSSIBLE FastIoCheckIfPossible;
  PFAST_IO_READ FastIoRead;
  PFAST_IO_WRITE FastIoWrite;
  PFAST_IO_QUERY_BASIC_INFO FastIoQueryBasicInfo;
  PFAST_IO_QUERY_STANDARD_INFO FastIoQueryStandardInfo;
  PFAST_IO_LOCK FastIoLock;
  PFAST_IO_UNLOCK_SINGLE FastIoUnlockSingle;
  PFAST_IO_UNLOCK_ALL FastIoUnlockAll;
  PFAST_IO_UNLOCK_ALL_BY_KEY FastIoUnlockAllByKey;
  PFAST_IO_DEVICE_CONTROL FastIoDeviceControl;
  PFAST_IO_ACQUIRE_FILE AcquireFileForNtCreateSection;
  PFAST_IO_RELEASE_FILE ReleaseFileForNtCreateSection;
  PFAST_IO_DETACH_DEVICE FastIoDetachDevice;
  PFAST_IO_QUERY_NETWORK_OPEN_INFO FastIoQueryNetworkOpenInfo;
  PFAST_IO_ACQUIRE_FOR_MOD_WRITE AcquireForModWrite;
  PFAST_IO_MDL_READ MdlRead;
  PFAST_IO_MDL_READ_COMPLETE MdlReadComplete;
  PFAST_IO_PREPARE_MDL_WRITE PrepareMdlWrite;
  PFAST_IO_MDL_WRITE_COMPLETE MdlWriteComplete;
  PFAST_IO_READ_COMPRESSED FastIoReadCompressed;
  PFAST_IO_WRITE_COMPRESSED FastIoWriteCompressed;
  PFAST_IO_MDL_READ_COMPLETE_COMPRESSED MdlReadCompleteCompressed;
  PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED MdlWriteCompleteCompressed;
  PFAST_IO_QUERY_OPEN FastIoQueryOpen;
  PFAST_IO_RELEASE_FOR_MOD_WRITE ReleaseForModWrite;
  PFAST_IO_ACQUIRE_FOR_CCFLUSH AcquireForCcFlush;
  PFAST_IO_RELEASE_FOR_CCFLUSH ReleaseForCcFlush;
} FAST_IO_DISPATCH, *PFAST_IO_DISPATCH;

typedef struct _SECTION_OBJECT_POINTERS {
  PVOID DataSectionObject;
  PVOID SharedCacheMap;
  PVOID ImageSectionObject;
} SECTION_OBJECT_POINTERS, *PSECTION_OBJECT_POINTERS;

typedef struct _IO_COMPLETION_CONTEXT {
  PVOID Port;
  PVOID Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

/* FILE_OBJECT.Flags */
#define FO_FILE_OPEN                 0x00000001
#define FO_SYNCHRONOUS_IO            0x00000002
#define FO_ALERTABLE_IO              0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING 0x00000008
#define FO_WRITE_THROUGH             0x00000010
#define FO_SEQUENTIAL_ONLY           0x00000020
#define FO_CACHE_SUPPORTED           0x00000040
#define FO_NAMED_PIPE                0x00000080
#define FO_STREAM_FILE               0x00000100
#define FO_MAILSLOT                  0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE   0x00000400
#define FO_QUEUE_IRP_TO_THREAD       0x00000400
#define FO_DIRECT_DEVICE_OPEN        0x00000800
#define FO_FILE_MODIFIED             0x00001000
#define FO_FILE_SIZE_CHANGED         0x00002000
#define FO_CLEANUP_COMPLETE          0x00004000
#define FO_TEMPORARY_FILE            0x00008000
#define FO_DELETE_ON_CLOSE           0x00010000
#define FO_OPENED_CASE_SENSITIVE     0x00020000
#define FO_HANDLE_CREATED            0x00040000
#define FO_FILE_FAST_IO_READ         0x00080000
#define FO_RANDOM_ACCESS             0x00100000
#define FO_FILE_OPEN_CANCELLED       0x00200000
#define FO_VOLUME_OPEN               0x00400000
#define FO_REMOTE_ORIGIN             0x01000000
#define FO_DISALLOW_EXCLUSIVE        0x02000000
#define FO_SKIP_COMPLETION_PORT      0x02000000
#define FO_SKIP_SET_EVENT            0x04000000
#define FO_SKIP_SET_FAST_IO          0x08000000
#define FO_FLAGS_VALID_ONLY_DURING_CREATE FO_DISALLOW_EXCLUSIVE

/* VPB.Flags */
#define VPB_MOUNTED                       0x0001
#define VPB_LOCKED                        0x0002
#define VPB_PERSISTENT                    0x0004
#define VPB_REMOVE_PENDING                0x0008
#define VPB_RAW_MOUNT                     0x0010
#define VPB_DIRECT_WRITES_ALLOWED         0x0020

/* IRP.Flags */

#define SL_FORCE_ACCESS_CHECK             0x01
#define SL_OPEN_PAGING_FILE               0x02
#define SL_OPEN_TARGET_DIRECTORY          0x04
#define SL_STOP_ON_SYMLINK                0x08
#define SL_CASE_SENSITIVE                 0x80

#define SL_KEY_SPECIFIED                  0x01
#define SL_OVERRIDE_VERIFY_VOLUME         0x02
#define SL_WRITE_THROUGH                  0x04
#define SL_FT_SEQUENTIAL_WRITE            0x08
#define SL_FORCE_DIRECT_WRITE             0x10
#define SL_REALTIME_STREAM                0x20

#define SL_READ_ACCESS_GRANTED            0x01
#define SL_WRITE_ACCESS_GRANTED           0x04

#define SL_FAIL_IMMEDIATELY               0x01
#define SL_EXCLUSIVE_LOCK                 0x02

#define SL_RESTART_SCAN                   0x01
#define SL_RETURN_SINGLE_ENTRY            0x02
#define SL_INDEX_SPECIFIED                0x04

#define SL_WATCH_TREE                     0x01

#define SL_ALLOW_RAW_MOUNT                0x01

#define CTL_CODE(DeviceType, Function, Method, Access) \
  (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define DEVICE_TYPE_FROM_CTL_CODE(ctl) (((ULONG) (ctl & 0xffff0000)) >> 16)

#define METHOD_FROM_CTL_CODE(ctrlCode)          ((ULONG)(ctrlCode & 3))

#define IRP_NOCACHE                     0x00000001
#define IRP_PAGING_IO                   0x00000002
#define IRP_MOUNT_COMPLETION            0x00000002
#define IRP_SYNCHRONOUS_API             0x00000004
#define IRP_ASSOCIATED_IRP              0x00000008
#define IRP_BUFFERED_IO                 0x00000010
#define IRP_DEALLOCATE_BUFFER           0x00000020
#define IRP_INPUT_OPERATION             0x00000040
#define IRP_SYNCHRONOUS_PAGING_IO       0x00000040
#define IRP_CREATE_OPERATION            0x00000080
#define IRP_READ_OPERATION              0x00000100
#define IRP_WRITE_OPERATION             0x00000200
#define IRP_CLOSE_OPERATION             0x00000400
#define IRP_DEFER_IO_COMPLETION         0x00000800
#define IRP_OB_QUERY_NAME               0x00001000
#define IRP_HOLD_DEVICE_QUEUE           0x00002000

#define IRP_QUOTA_CHARGED                 0x01
#define IRP_ALLOCATED_MUST_SUCCEED        0x02
#define IRP_ALLOCATED_FIXED_SIZE          0x04
#define IRP_LOOKASIDE_ALLOCATION          0x08

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
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define IRP_MN_DEVICE_ENUMERATED            0x19
#endif

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

typedef struct _FILE_OBJECT {
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
} FILE_OBJECT, *PFILE_OBJECT;

typedef struct _IO_ERROR_LOG_PACKET {
  UCHAR MajorFunctionCode;
  UCHAR RetryCount;
  USHORT DumpDataSize;
  USHORT NumberOfStrings;
  USHORT StringOffset;
  USHORT EventCategory;
  NTSTATUS ErrorCode;
  ULONG UniqueErrorValue;
  NTSTATUS FinalStatus;
  ULONG SequenceNumber;
  ULONG IoControlCode;
  LARGE_INTEGER DeviceOffset;
  ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

typedef struct _IO_ERROR_LOG_MESSAGE {
  USHORT Type;
  USHORT Size;
  USHORT DriverNameLength;
  LARGE_INTEGER TimeStamp;
  ULONG DriverNameOffset;
  IO_ERROR_LOG_PACKET EntryData;
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

#ifdef _WIN64
#define PORT_MAXIMUM_MESSAGE_LENGTH    512
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH    256
#endif

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
  ULONG Version;
  BOOLEAN Master;
  BOOLEAN ScatterGather;
  BOOLEAN DemandMode;
  BOOLEAN AutoInitialize;
  BOOLEAN Dma32BitAddresses;
  BOOLEAN IgnoreCount;
  BOOLEAN Reserved1;
  BOOLEAN Dma64BitAddresses;
  ULONG BusNumber;
  ULONG DmaChannel;
  INTERFACE_TYPE InterfaceType;
  DMA_WIDTH DmaWidth;
  DMA_SPEED DmaSpeed;
  ULONG MaximumLength;
  ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

typedef enum _DEVICE_RELATION_TYPE {
  BusRelations,
  EjectionRelations,
  PowerRelations,
  RemovalRelations,
  TargetDeviceRelation,
  SingleBusRelations,
  TransportRelations
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

typedef struct _DEVICE_RELATIONS {
  ULONG Count;
  PDEVICE_OBJECT Objects[1];
} DEVICE_RELATIONS, *PDEVICE_RELATIONS;

typedef struct _DEVOBJ_EXTENSION {
  CSHORT Type;
  USHORT Size;
  PDEVICE_OBJECT DeviceObject;
} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;

typedef struct _SCATTER_GATHER_ELEMENT {
  PHYSICAL_ADDRESS Address;
  ULONG Length;
  ULONG_PTR Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

#if defined(_MSC_EXTENSIONS)

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4200)
typedef struct _SCATTER_GATHER_LIST {
  ULONG NumberOfElements;
  ULONG_PTR Reserved;
  SCATTER_GATHER_ELEMENT Elements[1];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4200)
#endif

#else

struct _SCATTER_GATHER_LIST;
typedef struct _SCATTER_GATHER_LIST SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

#endif

typedef NTSTATUS
(NTAPI DRIVER_ADD_DEVICE)(
  IN struct _DRIVER_OBJECT *DriverObject,
  IN struct _DEVICE_OBJECT *PhysicalDeviceObject);
typedef DRIVER_ADD_DEVICE *PDRIVER_ADD_DEVICE;

typedef struct _DRIVER_EXTENSION {
  struct _DRIVER_OBJECT *DriverObject;
  PDRIVER_ADD_DEVICE AddDevice;
  ULONG Count;
  UNICODE_STRING ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

#define DRVO_UNLOAD_INVOKED               0x00000001
#define DRVO_LEGACY_DRIVER                0x00000002
#define DRVO_BUILTIN_DRIVER               0x00000004

typedef NTSTATUS
(NTAPI DRIVER_INITIALIZE)(
  IN struct _DRIVER_OBJECT *DriverObject,
  IN PUNICODE_STRING RegistryPath);
typedef DRIVER_INITIALIZE *PDRIVER_INITIALIZE;

typedef VOID
(NTAPI DRIVER_STARTIO)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp);
typedef DRIVER_STARTIO *PDRIVER_STARTIO;

typedef VOID
(NTAPI DRIVER_UNLOAD)(
  IN struct _DRIVER_OBJECT *DriverObject);
typedef DRIVER_UNLOAD *PDRIVER_UNLOAD;

typedef NTSTATUS
(NTAPI DRIVER_DISPATCH)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp);
typedef DRIVER_DISPATCH *PDRIVER_DISPATCH;

typedef struct _DRIVER_OBJECT {
  CSHORT Type;
  CSHORT Size;
  PDEVICE_OBJECT DeviceObject;
  ULONG Flags;
  PVOID DriverStart;
  ULONG DriverSize;
  PVOID DriverSection;
  PDRIVER_EXTENSION DriverExtension;
  UNICODE_STRING DriverName;
  PUNICODE_STRING HardwareDatabase;
  struct _FAST_IO_DISPATCH *FastIoDispatch;
  PDRIVER_INITIALIZE DriverInit;
  PDRIVER_STARTIO DriverStartIo;
  PDRIVER_UNLOAD DriverUnload;
  PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef struct _DMA_ADAPTER {
  USHORT Version;
  USHORT Size;
  struct _DMA_OPERATIONS* DmaOperations;
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID
(NTAPI *PPUT_DMA_ADAPTER)(
  IN PDMA_ADAPTER DmaAdapter);

typedef PVOID
(NTAPI *PALLOCATE_COMMON_BUFFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled);

typedef VOID
(NTAPI *PFREE_COMMON_BUFFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled);

typedef NTSTATUS
(NTAPI *PALLOCATE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG NumberOfMapRegisters,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context);

typedef BOOLEAN
(NTAPI *PFLUSH_ADAPTER_BUFFERS)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice);

typedef VOID
(NTAPI *PFREE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER DmaAdapter);

typedef VOID
(NTAPI *PFREE_MAP_REGISTERS)(
  IN PDMA_ADAPTER DmaAdapter,
  PVOID MapRegisterBase,
  ULONG NumberOfMapRegisters);

typedef PHYSICAL_ADDRESS
(NTAPI *PMAP_TRANSFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice);

typedef ULONG
(NTAPI *PGET_DMA_ALIGNMENT)(
  IN PDMA_ADAPTER DmaAdapter);

typedef ULONG
(NTAPI *PREAD_DMA_COUNTER)(
  IN PDMA_ADAPTER DmaAdapter);

typedef VOID
(NTAPI DRIVER_LIST_CONTROL)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp,
  IN struct _SCATTER_GATHER_LIST *ScatterGather,
  IN PVOID Context);
typedef DRIVER_LIST_CONTROL *PDRIVER_LIST_CONTROL;

typedef NTSTATUS
(NTAPI *PGET_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN PMDL Mdl,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PDRIVER_LIST_CONTROL ExecutionRoutine,
  IN PVOID Context,
  IN BOOLEAN WriteToDevice);

typedef VOID
(NTAPI *PPUT_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN BOOLEAN WriteToDevice);

typedef NTSTATUS
(NTAPI *PCALCULATE_SCATTER_GATHER_LIST_SIZE)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl OPTIONAL,
  IN PVOID CurrentVa,
  IN ULONG Length,
  OUT PULONG ScatterGatherListSize,
  OUT PULONG pNumberOfMapRegisters OPTIONAL);

typedef NTSTATUS
(NTAPI *PBUILD_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN PMDL Mdl,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PDRIVER_LIST_CONTROL ExecutionRoutine,
  IN PVOID Context,
  IN BOOLEAN WriteToDevice,
  IN PVOID ScatterGatherBuffer,
  IN ULONG ScatterGatherLength);

typedef NTSTATUS
(NTAPI *PBUILD_MDL_FROM_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN PMDL OriginalMdl,
  OUT PMDL *TargetMdl);

typedef struct _DMA_OPERATIONS {
  ULONG Size;
  PPUT_DMA_ADAPTER PutDmaAdapter;
  PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;
  PFREE_COMMON_BUFFER FreeCommonBuffer;
  PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
  PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
  PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
  PFREE_MAP_REGISTERS FreeMapRegisters;
  PMAP_TRANSFER MapTransfer;
  PGET_DMA_ALIGNMENT GetDmaAlignment;
  PREAD_DMA_COUNTER ReadDmaCounter;
  PGET_SCATTER_GATHER_LIST GetScatterGatherList;
  PPUT_SCATTER_GATHER_LIST PutScatterGatherList;
  PCALCULATE_SCATTER_GATHER_LIST_SIZE CalculateScatterGatherList;
  PBUILD_SCATTER_GATHER_LIST BuildScatterGatherList;
  PBUILD_MDL_FROM_SCATTER_GATHER_LIST BuildMdlFromScatterGatherList;
} DMA_OPERATIONS, *PDMA_OPERATIONS;

typedef struct _IO_RESOURCE_DESCRIPTOR {
  UCHAR Option;
  UCHAR Type;
  UCHAR ShareDisposition;
  UCHAR Spare1;
  USHORT Flags;
  USHORT Spare2;
  union {
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Port;
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Memory;
    struct {
      ULONG MinimumVector;
      ULONG MaximumVector;
    } Interrupt;
    struct {
      ULONG MinimumChannel;
      ULONG MaximumChannel;
    } Dma;
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Generic;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Length;
      ULONG MinBusNumber;
      ULONG MaxBusNumber;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG Priority;
      ULONG Reserved1;
      ULONG Reserved2;
    } ConfigData;
  } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
  USHORT Version;
  USHORT Revision;
  ULONG Count;
  IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
  ULONG ListSize;
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  ULONG SlotNumber;
  ULONG Reserved[3];
  ULONG AlternativeLists;
  IO_RESOURCE_LIST List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

typedef VOID
(NTAPI DRIVER_CANCEL)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp);
typedef DRIVER_CANCEL *PDRIVER_CANCEL;

typedef struct _IRP {
  CSHORT Type;
  USHORT Size;
  struct _MDL *MdlAddress;
  ULONG Flags;
  union {
    struct _IRP *MasterIrp;
    volatile LONG IrpCount;
    PVOID SystemBuffer;
  } AssociatedIrp;
  LIST_ENTRY ThreadListEntry;
  IO_STATUS_BLOCK IoStatus;
  KPROCESSOR_MODE RequestorMode;
  BOOLEAN PendingReturned;
  CHAR StackCount;
  CHAR CurrentLocation;
  BOOLEAN Cancel;
  KIRQL CancelIrql;
  CCHAR ApcEnvironment;
  UCHAR AllocationFlags;
  PIO_STATUS_BLOCK UserIosb;
  PKEVENT UserEvent;
  union {
    struct {
      _ANONYMOUS_UNION union {
        PIO_APC_ROUTINE UserApcRoutine;
        PVOID IssuingProcess;
      } DUMMYUNIONNAME;
      PVOID UserApcContext;
    } AsynchronousParameters;
    LARGE_INTEGER AllocationSize;
  } Overlay;
  volatile PDRIVER_CANCEL CancelRoutine;
  PVOID UserBuffer;
  union {
    struct {
      _ANONYMOUS_UNION union {
        KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
        _ANONYMOUS_STRUCT struct {
          PVOID DriverContext[4];
        } DUMMYSTRUCTNAME;
      } DUMMYUNIONNAME;
      PETHREAD Thread;
      PCHAR AuxiliaryBuffer;
      _ANONYMOUS_STRUCT struct {
        LIST_ENTRY ListEntry;
        _ANONYMOUS_UNION union {
          struct _IO_STACK_LOCATION *CurrentStackLocation;
          ULONG PacketType;
        } DUMMYUNIONNAME;
      } DUMMYSTRUCTNAME;
      struct _FILE_OBJECT *OriginalFileObject;
    } Overlay;
    KAPC Apc;
    PVOID CompletionKey;
  } Tail;
} IRP, *PIRP;

typedef enum _IO_PAGING_PRIORITY {
  IoPagingPriorityInvalid,
  IoPagingPriorityNormal,
  IoPagingPriorityHigh,
  IoPagingPriorityReserved1,
  IoPagingPriorityReserved2
} IO_PAGING_PRIORITY;

typedef NTSTATUS
(NTAPI IO_COMPLETION_ROUTINE)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp,
  IN PVOID Context);
typedef IO_COMPLETION_ROUTINE *PIO_COMPLETION_ROUTINE;

typedef VOID
(NTAPI IO_DPC_ROUTINE)(
  IN struct _KDPC *Dpc,
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp,
  IN PVOID Context);
typedef IO_DPC_ROUTINE *PIO_DPC_ROUTINE;

typedef NTSTATUS
(NTAPI *PMM_DLL_INITIALIZE)(
  IN PUNICODE_STRING RegistryPath);

typedef NTSTATUS
(NTAPI *PMM_DLL_UNLOAD)(
  VOID);

typedef VOID
(NTAPI IO_TIMER_ROUTINE)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN PVOID Context);
typedef IO_TIMER_ROUTINE *PIO_TIMER_ROUTINE;

typedef struct _IO_SECURITY_CONTEXT {
  PSECURITY_QUALITY_OF_SERVICE SecurityQos;
  PACCESS_STATE AccessState;
  ACCESS_MASK DesiredAccess;
  ULONG FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;

struct _IO_CSQ;

typedef struct _IO_CSQ_IRP_CONTEXT {
  ULONG Type;
  struct _IRP *Irp;
  struct _IO_CSQ *Csq;
} IO_CSQ_IRP_CONTEXT, *PIO_CSQ_IRP_CONTEXT;

typedef VOID
(NTAPI *PIO_CSQ_INSERT_IRP)(
  IN struct _IO_CSQ *Csq,
  IN PIRP Irp);

typedef NTSTATUS
(NTAPI IO_CSQ_INSERT_IRP_EX)(
  IN struct _IO_CSQ *Csq,
  IN PIRP Irp,
  IN PVOID InsertContext);
typedef IO_CSQ_INSERT_IRP_EX *PIO_CSQ_INSERT_IRP_EX;

typedef VOID
(NTAPI *PIO_CSQ_REMOVE_IRP)(
  IN struct _IO_CSQ *Csq,
  IN PIRP Irp);

typedef PIRP
(NTAPI *PIO_CSQ_PEEK_NEXT_IRP)(
  IN struct _IO_CSQ *Csq,
  IN PIRP Irp,
  IN PVOID PeekContext);

typedef VOID
(NTAPI *PIO_CSQ_ACQUIRE_LOCK)(
  IN struct _IO_CSQ *Csq,
  OUT PKIRQL Irql);

typedef VOID
(NTAPI *PIO_CSQ_RELEASE_LOCK)(
  IN struct _IO_CSQ *Csq,
  IN KIRQL Irql);

typedef VOID
(NTAPI *PIO_CSQ_COMPLETE_CANCELED_IRP)(
  IN struct _IO_CSQ *Csq,
  IN PIRP Irp);

typedef struct _IO_CSQ {
  ULONG Type;
  PIO_CSQ_INSERT_IRP CsqInsertIrp;
  PIO_CSQ_REMOVE_IRP CsqRemoveIrp;
  PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp;
  PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock;
  PIO_CSQ_RELEASE_LOCK CsqReleaseLock;
  PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp;
  PVOID ReservePointer;
} IO_CSQ, *PIO_CSQ;

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

typedef BOOLEAN
(NTAPI *PGPE_SERVICE_ROUTINE)(
  PVOID,
  PVOID);

typedef NTSTATUS
(NTAPI *PGPE_CONNECT_VECTOR)(
  PDEVICE_OBJECT,
  ULONG,
  KINTERRUPT_MODE,
  BOOLEAN,
  PGPE_SERVICE_ROUTINE,
  PVOID,
  PVOID);

typedef NTSTATUS
(NTAPI *PGPE_DISCONNECT_VECTOR)(
  PVOID);

typedef NTSTATUS
(NTAPI *PGPE_ENABLE_EVENT)(
  PDEVICE_OBJECT,
  PVOID);

typedef NTSTATUS
(NTAPI *PGPE_DISABLE_EVENT)(
  PDEVICE_OBJECT,
  PVOID);

typedef NTSTATUS
(NTAPI *PGPE_CLEAR_STATUS)(
  PDEVICE_OBJECT,
  PVOID);

typedef VOID
(NTAPI *PDEVICE_NOTIFY_CALLBACK)(
  PVOID,
  ULONG);

typedef NTSTATUS
(NTAPI *PREGISTER_FOR_DEVICE_NOTIFICATIONS)(
  PDEVICE_OBJECT,
  PDEVICE_NOTIFY_CALLBACK,
  PVOID);

typedef VOID
(NTAPI *PUNREGISTER_FOR_DEVICE_NOTIFICATIONS)(
  PDEVICE_OBJECT,
  PDEVICE_NOTIFY_CALLBACK);

typedef struct _ACPI_INTERFACE_STANDARD {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PGPE_CONNECT_VECTOR GpeConnectVector;
  PGPE_DISCONNECT_VECTOR GpeDisconnectVector;
  PGPE_ENABLE_EVENT GpeEnableEvent;
  PGPE_DISABLE_EVENT GpeDisableEvent;
  PGPE_CLEAR_STATUS GpeClearStatus;
  PREGISTER_FOR_DEVICE_NOTIFICATIONS RegisterForDeviceNotifications;
  PUNREGISTER_FOR_DEVICE_NOTIFICATIONS UnregisterForDeviceNotifications;
} ACPI_INTERFACE_STANDARD, *PACPI_INTERFACE_STANDARD;

typedef BOOLEAN
(NTAPI *PGPE_SERVICE_ROUTINE2)(
  PVOID ObjectContext,
  PVOID ServiceContext);

typedef NTSTATUS
(NTAPI *PGPE_CONNECT_VECTOR2)(
  PVOID Context,
  ULONG GpeNumber,
  KINTERRUPT_MODE Mode,
  BOOLEAN Shareable,
  PGPE_SERVICE_ROUTINE ServiceRoutine,
  PVOID ServiceContext,
  PVOID *ObjectContext);

typedef NTSTATUS
(NTAPI *PGPE_DISCONNECT_VECTOR2)(
  PVOID Context,
  PVOID ObjectContext);

typedef NTSTATUS
(NTAPI *PGPE_ENABLE_EVENT2)(
  PVOID Context,
  PVOID ObjectContext);

typedef NTSTATUS
(NTAPI *PGPE_DISABLE_EVENT2)(
  PVOID Context,
  PVOID ObjectContext);

typedef NTSTATUS
(NTAPI *PGPE_CLEAR_STATUS2)(
  PVOID Context,
  PVOID ObjectContext);

typedef VOID
(NTAPI *PDEVICE_NOTIFY_CALLBACK2)(
  PVOID NotificationContext,
  ULONG NotifyCode);

typedef NTSTATUS
(NTAPI *PREGISTER_FOR_DEVICE_NOTIFICATIONS2)(
  PVOID Context,
  PDEVICE_NOTIFY_CALLBACK2 NotificationHandler,
  PVOID NotificationContext);

typedef VOID
(NTAPI *PUNREGISTER_FOR_DEVICE_NOTIFICATIONS2)(
  PVOID Context);

typedef struct _ACPI_INTERFACE_STANDARD2 {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PGPE_CONNECT_VECTOR2 GpeConnectVector;
  PGPE_DISCONNECT_VECTOR2 GpeDisconnectVector;
  PGPE_ENABLE_EVENT2 GpeEnableEvent;
  PGPE_DISABLE_EVENT2 GpeDisableEvent;
  PGPE_CLEAR_STATUS2 GpeClearStatus;
  PREGISTER_FOR_DEVICE_NOTIFICATIONS2 RegisterForDeviceNotifications;
  PUNREGISTER_FOR_DEVICE_NOTIFICATIONS2 UnregisterForDeviceNotifications;
} ACPI_INTERFACE_STANDARD2, *PACPI_INTERFACE_STANDARD2;

#if !defined(_AMD64_) && !defined(_IA64_)
#include <pshpack4.h>
#endif
typedef struct _IO_STACK_LOCATION {
  UCHAR MajorFunction;
  UCHAR MinorFunction;
  UCHAR Flags;
  UCHAR Control;
  union {
    struct {
      PIO_SECURITY_CONTEXT SecurityContext;
      ULONG Options;
      USHORT POINTER_ALIGNMENT FileAttributes;
      USHORT ShareAccess;
      ULONG POINTER_ALIGNMENT EaLength;
    } Create;
    struct {
      ULONG Length;
      ULONG POINTER_ALIGNMENT Key;
      LARGE_INTEGER ByteOffset;
    } Read;
    struct {
      ULONG Length;
      ULONG POINTER_ALIGNMENT Key;
      LARGE_INTEGER ByteOffset;
    } Write;
    struct {
      ULONG Length;
      PUNICODE_STRING FileName;
      FILE_INFORMATION_CLASS FileInformationClass;
      ULONG FileIndex;
    } QueryDirectory;
    struct {
      ULONG Length;
      ULONG CompletionFilter;
    } NotifyDirectory;
    struct {
      ULONG Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
    } QueryFile;
    struct {
      ULONG Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
      PFILE_OBJECT FileObject;
      _ANONYMOUS_UNION union {
        _ANONYMOUS_STRUCT struct {
          BOOLEAN ReplaceIfExists;
          BOOLEAN AdvanceOnly;
        } DUMMYSTRUCTNAME;
        ULONG ClusterCount;
        HANDLE DeleteHandle;
      } DUMMYUNIONNAME;
    } SetFile;
    struct {
      ULONG Length;
      PVOID EaList;
      ULONG EaListLength;
      ULONG EaIndex;
    } QueryEa;
    struct {
      ULONG Length;
    } SetEa;
    struct {
      ULONG Length;
      FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
    } QueryVolume;
    struct {
      ULONG Length;
      FS_INFORMATION_CLASS FsInformationClass;
    } SetVolume;
    struct {
      ULONG OutputBufferLength;
      ULONG InputBufferLength;
      ULONG FsControlCode;
      PVOID Type3InputBuffer;
    } FileSystemControl;
    struct {
      PLARGE_INTEGER Length;
      ULONG Key;
      LARGE_INTEGER ByteOffset;
    } LockControl;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID Type3InputBuffer;
    } DeviceIoControl;
    struct {
      SECURITY_INFORMATION SecurityInformation;
      ULONG POINTER_ALIGNMENT Length;
    } QuerySecurity;
    struct {
      SECURITY_INFORMATION SecurityInformation;
      PSECURITY_DESCRIPTOR SecurityDescriptor;
    } SetSecurity;
    struct {
      PVPB Vpb;
      PDEVICE_OBJECT DeviceObject;
    } MountVolume;
    struct {
      PVPB Vpb;
      PDEVICE_OBJECT DeviceObject;
    } VerifyVolume;
    struct {
      struct _SCSI_REQUEST_BLOCK *Srb;
    } Scsi;
    struct {
      ULONG Length;
      PSID StartSid;
      struct _FILE_GET_QUOTA_INFORMATION *SidList;
      ULONG SidListLength;
    } QueryQuota;
    struct {
      ULONG Length;
    } SetQuota;
    struct {
      DEVICE_RELATION_TYPE Type;
    } QueryDeviceRelations;
    struct {
      CONST GUID *InterfaceType;
      USHORT Size;
      USHORT Version;
      PINTERFACE Interface;
      PVOID InterfaceSpecificData;
    } QueryInterface;
    struct {
      PDEVICE_CAPABILITIES Capabilities;
    } DeviceCapabilities;
    struct {
      PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
    } FilterResourceRequirements;
    struct {
      ULONG WhichSpace;
      PVOID Buffer;
      ULONG Offset;
      ULONG POINTER_ALIGNMENT Length;
    } ReadWriteConfig;
    struct {
      BOOLEAN Lock;
    } SetLock;
    struct {
      BUS_QUERY_ID_TYPE IdType;
    } QueryId;
    struct {
      DEVICE_TEXT_TYPE DeviceTextType;
      LCID POINTER_ALIGNMENT LocaleId;
    } QueryDeviceText;
    struct {
      BOOLEAN InPath;
      BOOLEAN Reserved[3];
      DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT Type;
    } UsageNotification;
    struct {
      SYSTEM_POWER_STATE PowerState;
    } WaitWake;
    struct {
      PPOWER_SEQUENCE PowerSequence;
    } PowerSequence;
    struct {
      ULONG SystemContext;
      POWER_STATE_TYPE POINTER_ALIGNMENT Type;
      POWER_STATE POINTER_ALIGNMENT State;
      POWER_ACTION POINTER_ALIGNMENT ShutdownType;
    } Power;
    struct {
      PCM_RESOURCE_LIST AllocatedResources;
      PCM_RESOURCE_LIST AllocatedResourcesTranslated;
    } StartDevice;
    struct {
      ULONG_PTR ProviderId;
      PVOID DataPath;
      ULONG BufferSize;
      PVOID Buffer;
    } WMI;
    struct {
      PVOID Argument1;
      PVOID Argument2;
      PVOID Argument3;
      PVOID Argument4;
    } Others;
  } Parameters;
  PDEVICE_OBJECT DeviceObject;
  PFILE_OBJECT FileObject;
  PIO_COMPLETION_ROUTINE CompletionRoutine;
  PVOID Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
#if !defined(_AMD64_) && !defined(_IA64_)
#include <poppack.h>
#endif

/* IO_STACK_LOCATION.Control */

#define SL_PENDING_RETURNED               0x01
#define SL_ERROR_RETURNED                 0x02
#define SL_INVOKE_ON_CANCEL               0x20
#define SL_INVOKE_ON_SUCCESS              0x40
#define SL_INVOKE_ON_ERROR                0x80

#define METHOD_BUFFERED                   0
#define METHOD_IN_DIRECT                  1
#define METHOD_OUT_DIRECT                 2
#define METHOD_NEITHER                    3

#define METHOD_DIRECT_TO_HARDWARE       METHOD_IN_DIRECT
#define METHOD_DIRECT_FROM_HARDWARE     METHOD_OUT_DIRECT

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
#define FILE_ATTRIBUTE_VIRTUAL            0x00010000

#define FILE_ATTRIBUTE_VALID_FLAGS        0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS    0x000031a7

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
#define FILE_OPEN_REMOTE_INSTANCE         0x00000400
#define FILE_RANDOM_ACCESS                0x00000800
#define FILE_DELETE_ON_CLOSE              0x00001000
#define FILE_OPEN_BY_FILE_ID              0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT       0x00004000
#define FILE_NO_COMPRESSION               0x00008000
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK        0x00010000
#define FILE_DISALLOW_EXCLUSIVE           0x00020000
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
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

#define WMIREG_ACTION_REGISTER      1
#define WMIREG_ACTION_DEREGISTER    2
#define WMIREG_ACTION_REREGISTER    3
#define WMIREG_ACTION_UPDATE_GUIDS  4
#define WMIREG_ACTION_BLOCK_IRPS    5

#define WMIREGISTER                 0
#define WMIUPDATE                   1

typedef VOID
(NTAPI FWMI_NOTIFICATION_CALLBACK)(
  PVOID Wnode,
  PVOID Context);
typedef FWMI_NOTIFICATION_CALLBACK *WMI_NOTIFICATION_CALLBACK;

#ifndef _PCI_X_
#define _PCI_X_

typedef struct _PCI_SLOT_NUMBER {
  union {
    struct {
      ULONG DeviceNumber:5;
      ULONG FunctionNumber:3;
      ULONG Reserved:24;
    } bits;
    ULONG AsULONG;
  } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

#define PCI_TYPE0_ADDRESSES               6
#define PCI_TYPE1_ADDRESSES               2
#define PCI_TYPE2_ADDRESSES               5

/* While MS WDK uses inheritance in C++, we cannot do this with gcc, as
   inheritance, even from a struct renders the type non-POD. So we use
   this hack */

    struct _PCI_HEADER_TYPE_0 {
      ULONG BaseAddresses[PCI_TYPE0_ADDRESSES];
      ULONG CIS;
      USHORT SubVendorID;
      USHORT SubSystemID;
      ULONG ROMBaseAddress;
      UCHAR CapabilitiesPtr;
      UCHAR Reserved1[3];
      ULONG Reserved2;
      UCHAR InterruptLine;
      UCHAR InterruptPin;
      UCHAR MinimumGrant;
      UCHAR MaximumLatency;
    };

    struct _PCI_HEADER_TYPE_1 {
      ULONG BaseAddresses[PCI_TYPE1_ADDRESSES];
      UCHAR PrimaryBus;
      UCHAR SecondaryBus;
      UCHAR SubordinateBus;
      UCHAR SecondaryLatency;
      UCHAR IOBase;
      UCHAR IOLimit;
      USHORT SecondaryStatus;
      USHORT MemoryBase;
      USHORT MemoryLimit;
      USHORT PrefetchBase;
      USHORT PrefetchLimit;
      ULONG PrefetchBaseUpper32;
      ULONG PrefetchLimitUpper32;
      USHORT IOBaseUpper16;
      USHORT IOLimitUpper16;
      UCHAR CapabilitiesPtr;
      UCHAR Reserved1[3];
      ULONG ROMBaseAddress;
      UCHAR InterruptLine;
      UCHAR InterruptPin;
      USHORT BridgeControl;
    };

    struct _PCI_HEADER_TYPE_2 {
      ULONG SocketRegistersBaseAddress;
      UCHAR CapabilitiesPtr;
      UCHAR Reserved;
      USHORT SecondaryStatus;
      UCHAR PrimaryBus;
      UCHAR SecondaryBus;
      UCHAR SubordinateBus;
      UCHAR SecondaryLatency;
      struct {
        ULONG Base;
        ULONG Limit;
      } Range[PCI_TYPE2_ADDRESSES-1];
      UCHAR InterruptLine;
      UCHAR InterruptPin;
      USHORT BridgeControl;
    };

#define PCI_COMMON_HEADER_LAYOUT \
  USHORT VendorID; \
  USHORT DeviceID; \
  USHORT Command; \
  USHORT Status; \
  UCHAR RevisionID; \
  UCHAR ProgIf; \
  UCHAR SubClass; \
  UCHAR BaseClass; \
  UCHAR CacheLineSize; \
  UCHAR LatencyTimer; \
  UCHAR HeaderType; \
  UCHAR BIST; \
  union { \
    struct _PCI_HEADER_TYPE_0 type0; \
    struct _PCI_HEADER_TYPE_1 type1; \
    struct _PCI_HEADER_TYPE_2 type2; \
  } u;

typedef struct _PCI_COMMON_HEADER {
  PCI_COMMON_HEADER_LAYOUT
} PCI_COMMON_HEADER, *PPCI_COMMON_HEADER;

typedef struct _PCI_COMMON_CONFIG {
  PCI_COMMON_HEADER_LAYOUT
  UCHAR DeviceSpecific[192];
} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;

#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_EXTENDED_CONFIG_LENGTH               0x1000

#define PCI_MAX_DEVICES        32
#define PCI_MAX_FUNCTION       8
#define PCI_MAX_BRIDGE_NUMBER  0xFF
#define PCI_INVALID_VENDORID   0xFFFF

/* PCI_COMMON_CONFIG.HeaderType */
#define PCI_MULTIFUNCTION                 0x80
#define PCI_DEVICE_TYPE                   0x00
#define PCI_BRIDGE_TYPE                   0x01
#define PCI_CARDBUS_BRIDGE_TYPE           0x02

#define PCI_CONFIGURATION_TYPE(PciData) \
  (((PPCI_COMMON_CONFIG) (PciData))->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MULTIFUNCTION_DEVICE(PciData) \
  ((((PPCI_COMMON_CONFIG) (PciData))->HeaderType & PCI_MULTIFUNCTION) != 0)

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
#define PCI_DISABLE_LEVEL_INTERRUPT       0x0400

/* PCI_COMMON_CONFIG.Status */
#define PCI_STATUS_INTERRUPT_PENDING      0x0008
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

/* IO_STACK_LOCATION.Parameters.ReadWriteControl.WhichSpace */

#define PCI_WHICHSPACE_CONFIG             0x0
#define PCI_WHICHSPACE_ROM                0x52696350 /* 'PciR' */

#define PCI_CAPABILITY_ID_POWER_MANAGEMENT  0x01
#define PCI_CAPABILITY_ID_AGP               0x02
#define PCI_CAPABILITY_ID_VPD               0x03
#define PCI_CAPABILITY_ID_SLOT_ID           0x04
#define PCI_CAPABILITY_ID_MSI               0x05
#define PCI_CAPABILITY_ID_CPCI_HOTSWAP      0x06
#define PCI_CAPABILITY_ID_PCIX              0x07
#define PCI_CAPABILITY_ID_HYPERTRANSPORT    0x08
#define PCI_CAPABILITY_ID_VENDOR_SPECIFIC   0x09
#define PCI_CAPABILITY_ID_DEBUG_PORT        0x0A
#define PCI_CAPABILITY_ID_CPCI_RES_CTRL     0x0B
#define PCI_CAPABILITY_ID_SHPC              0x0C
#define PCI_CAPABILITY_ID_P2P_SSID          0x0D
#define PCI_CAPABILITY_ID_AGP_TARGET        0x0E
#define PCI_CAPABILITY_ID_SECURE            0x0F
#define PCI_CAPABILITY_ID_PCI_EXPRESS       0x10
#define PCI_CAPABILITY_ID_MSIX              0x11

typedef struct _PCI_CAPABILITIES_HEADER {
  UCHAR CapabilityID;
  UCHAR Next;
} PCI_CAPABILITIES_HEADER, *PPCI_CAPABILITIES_HEADER;

typedef struct _PCI_PMC {
  UCHAR Version:3;
  UCHAR PMEClock:1;
  UCHAR Rsvd1:1;
  UCHAR DeviceSpecificInitialization:1;
  UCHAR Rsvd2:2;
  struct _PM_SUPPORT {
    UCHAR Rsvd2:1;
    UCHAR D1:1;
    UCHAR D2:1;
    UCHAR PMED0:1;
    UCHAR PMED1:1;
    UCHAR PMED2:1;
    UCHAR PMED3Hot:1;
    UCHAR PMED3Cold:1;
  } Support;
} PCI_PMC, *PPCI_PMC;

typedef struct _PCI_PMCSR {
  USHORT PowerState:2;
  USHORT Rsvd1:6;
  USHORT PMEEnable:1;
  USHORT DataSelect:4;
  USHORT DataScale:2;
  USHORT PMEStatus:1;
} PCI_PMCSR, *PPCI_PMCSR;

typedef struct _PCI_PMCSR_BSE {
  UCHAR Rsvd1:6;
  UCHAR D3HotSupportsStopClock:1;
  UCHAR BusPowerClockControlEnabled:1;
} PCI_PMCSR_BSE, *PPCI_PMCSR_BSE;

typedef struct _PCI_PM_CAPABILITY {
  PCI_CAPABILITIES_HEADER Header;
  union {
    PCI_PMC Capabilities;
    USHORT AsUSHORT;
  } PMC;
    union {
      PCI_PMCSR ControlStatus;
      USHORT AsUSHORT;
    } PMCSR;
    union {
      PCI_PMCSR_BSE BridgeSupport;
      UCHAR AsUCHAR;
    } PMCSR_BSE;
  UCHAR Data;
} PCI_PM_CAPABILITY, *PPCI_PM_CAPABILITY;

typedef struct {
  PCI_CAPABILITIES_HEADER Header;
  union {
    struct {
      USHORT DataParityErrorRecoveryEnable:1;
      USHORT EnableRelaxedOrdering:1;
      USHORT MaxMemoryReadByteCount:2;
      USHORT MaxOutstandingSplitTransactions:3;
      USHORT Reserved:9;
    } bits;
    USHORT AsUSHORT;
  } Command;
  union {
    struct {
      ULONG FunctionNumber:3;
      ULONG DeviceNumber:5;
      ULONG BusNumber:8;
      ULONG Device64Bit:1;
      ULONG Capable133MHz:1;
      ULONG SplitCompletionDiscarded:1;
      ULONG UnexpectedSplitCompletion:1;
      ULONG DeviceComplexity:1;
      ULONG DesignedMaxMemoryReadByteCount:2;
      ULONG DesignedMaxOutstandingSplitTransactions:3;
      ULONG DesignedMaxCumulativeReadSize:3;
      ULONG ReceivedSplitCompletionErrorMessage:1;
      ULONG CapablePCIX266:1;
      ULONG CapablePCIX533:1;
      } bits;
    ULONG AsULONG;
  } Status;
} PCI_X_CAPABILITY, *PPCI_X_CAPABILITY;

#define PCI_EXPRESS_ADVANCED_ERROR_REPORTING_CAP_ID                     0x0001
#define PCI_EXPRESS_VIRTUAL_CHANNEL_CAP_ID                              0x0002
#define PCI_EXPRESS_DEVICE_SERIAL_NUMBER_CAP_ID                         0x0003
#define PCI_EXPRESS_POWER_BUDGETING_CAP_ID                              0x0004
#define PCI_EXPRESS_RC_LINK_DECLARATION_CAP_ID                          0x0005
#define PCI_EXPRESS_RC_INTERNAL_LINK_CONTROL_CAP_ID                     0x0006
#define PCI_EXPRESS_RC_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION_CAP_ID      0x0007
#define PCI_EXPRESS_MFVC_CAP_ID                                         0x0008
#define PCI_EXPRESS_VC_AND_MFVC_CAP_ID                                  0x0009
#define PCI_EXPRESS_RCRB_HEADER_CAP_ID                                  0x000A
#define PCI_EXPRESS_SINGLE_ROOT_IO_VIRTUALIZATION_CAP_ID                0x0010

typedef struct _PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER {
  USHORT CapabilityID;
  USHORT Version:4;
  USHORT Next:12;
} PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER, *PPCI_EXPRESS_ENHANCED_CAPABILITY_HEADER;

typedef struct _PCI_EXPRESS_SERIAL_NUMBER_CAPABILITY {
  PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
  ULONG LowSerialNumber;
  ULONG HighSerialNumber;
} PCI_EXPRESS_SERIAL_NUMBER_CAPABILITY, *PPCI_EXPRESS_SERIAL_NUMBER_CAPABILITY;

typedef union _PCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS {
  struct {
    ULONG Undefined:1;
    ULONG Reserved1:3;
    ULONG DataLinkProtocolError:1;
    ULONG SurpriseDownError:1;
    ULONG Reserved2:6;
    ULONG PoisonedTLP:1;
    ULONG FlowControlProtocolError:1;
    ULONG CompletionTimeout:1;
    ULONG CompleterAbort:1;
    ULONG UnexpectedCompletion:1;
    ULONG ReceiverOverflow:1;
    ULONG MalformedTLP:1;
    ULONG ECRCError:1;
    ULONG UnsupportedRequestError:1;
    ULONG Reserved3:11;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS, *PPCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS;

typedef union _PCI_EXPRESS_UNCORRECTABLE_ERROR_MASK {
  struct {
    ULONG Undefined:1;
    ULONG Reserved1:3;
    ULONG DataLinkProtocolError:1;
    ULONG SurpriseDownError:1;
    ULONG Reserved2:6;
    ULONG PoisonedTLP:1;
    ULONG FlowControlProtocolError:1;
    ULONG CompletionTimeout:1;
    ULONG CompleterAbort:1;
    ULONG UnexpectedCompletion:1;
    ULONG ReceiverOverflow:1;
    ULONG MalformedTLP:1;
    ULONG ECRCError:1;
    ULONG UnsupportedRequestError:1;
    ULONG Reserved3:11;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_UNCORRECTABLE_ERROR_MASK, *PPCI_EXPRESS_UNCORRECTABLE_ERROR_MASK;

typedef union _PCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY {
  struct {
    ULONG Undefined:1;
    ULONG Reserved1:3;
    ULONG DataLinkProtocolError:1;
    ULONG SurpriseDownError:1;
    ULONG Reserved2:6;
    ULONG PoisonedTLP:1;
    ULONG FlowControlProtocolError:1;
    ULONG CompletionTimeout:1;
    ULONG CompleterAbort:1;
    ULONG UnexpectedCompletion:1;
    ULONG ReceiverOverflow:1;
    ULONG MalformedTLP:1;
    ULONG ECRCError:1;
    ULONG UnsupportedRequestError:1;
    ULONG Reserved3:11;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY, *PPCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY;

typedef union _PCI_EXPRESS_CORRECTABLE_ERROR_STATUS {
  struct {
    ULONG ReceiverError:1;
    ULONG Reserved1:5;
    ULONG BadTLP:1;
    ULONG BadDLLP:1;
    ULONG ReplayNumRollover:1;
    ULONG Reserved2:3;
    ULONG ReplayTimerTimeout:1;
    ULONG AdvisoryNonFatalError:1;
    ULONG Reserved3:18;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_CORRECTABLE_ERROR_STATUS, *PPCI_CORRECTABLE_ERROR_STATUS;

typedef union _PCI_EXPRESS_CORRECTABLE_ERROR_MASK {
  struct {
    ULONG ReceiverError:1;
    ULONG Reserved1:5;
    ULONG BadTLP:1;
    ULONG BadDLLP:1;
    ULONG ReplayNumRollover:1;
    ULONG Reserved2:3;
    ULONG ReplayTimerTimeout:1;
    ULONG AdvisoryNonFatalError:1;
    ULONG Reserved3:18;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_CORRECTABLE_ERROR_MASK, *PPCI_CORRECTABLE_ERROR_MASK;

typedef union _PCI_EXPRESS_AER_CAPABILITIES {
  struct {
    ULONG FirstErrorPointer:5;
    ULONG ECRCGenerationCapable:1;
    ULONG ECRCGenerationEnable:1;
    ULONG ECRCCheckCapable:1;
    ULONG ECRCCheckEnable:1;
    ULONG Reserved:23;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_AER_CAPABILITIES, *PPCI_EXPRESS_AER_CAPABILITIES;

typedef union _PCI_EXPRESS_ROOT_ERROR_COMMAND {
  struct {
    ULONG CorrectableErrorReportingEnable:1;
    ULONG NonFatalErrorReportingEnable:1;
    ULONG FatalErrorReportingEnable:1;
    ULONG Reserved:29;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_ROOT_ERROR_COMMAND, *PPCI_EXPRESS_ROOT_ERROR_COMMAND;

typedef union _PCI_EXPRESS_ROOT_ERROR_STATUS {
  struct {
    ULONG CorrectableErrorReceived:1;
    ULONG MultipleCorrectableErrorsReceived:1;
    ULONG UncorrectableErrorReceived:1;
    ULONG MultipleUncorrectableErrorsReceived:1;
    ULONG FirstUncorrectableFatal:1;
    ULONG NonFatalErrorMessagesReceived:1;
    ULONG FatalErrorMessagesReceived:1;
    ULONG Reserved:20;
    ULONG AdvancedErrorInterruptMessageNumber:5;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_ROOT_ERROR_STATUS, *PPCI_EXPRESS_ROOT_ERROR_STATUS;

typedef union _PCI_EXPRESS_ERROR_SOURCE_ID {
  struct {
    USHORT CorrectableSourceIdFun:3;
    USHORT CorrectableSourceIdDev:5;
    USHORT CorrectableSourceIdBus:8;
    USHORT UncorrectableSourceIdFun:3;
    USHORT UncorrectableSourceIdDev:5;
    USHORT UncorrectableSourceIdBus:8;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_ERROR_SOURCE_ID, *PPCI_EXPRESS_ERROR_SOURCE_ID;

typedef union _PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_STATUS {
  struct {
    ULONG TargetAbortOnSplitCompletion:1;
    ULONG MasterAbortOnSplitCompletion:1;
    ULONG ReceivedTargetAbort:1;
    ULONG ReceivedMasterAbort:1;
    ULONG RsvdZ:1;
    ULONG UnexpectedSplitCompletionError:1;
    ULONG UncorrectableSplitCompletion:1;
    ULONG UncorrectableDataError:1;
    ULONG UncorrectableAttributeError:1;
    ULONG UncorrectableAddressError:1;
    ULONG DelayedTransactionDiscardTimerExpired:1;
    ULONG PERRAsserted:1;
    ULONG SERRAsserted:1;
    ULONG InternalBridgeError:1;
    ULONG Reserved:18;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_STATUS, *PPCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_STATUS;

typedef union _PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_MASK {
  struct {
    ULONG TargetAbortOnSplitCompletion:1;
    ULONG MasterAbortOnSplitCompletion:1;
    ULONG ReceivedTargetAbort:1;
    ULONG ReceivedMasterAbort:1;
    ULONG RsvdZ:1;
    ULONG UnexpectedSplitCompletionError:1;
    ULONG UncorrectableSplitCompletion:1;
    ULONG UncorrectableDataError:1;
    ULONG UncorrectableAttributeError:1;
    ULONG UncorrectableAddressError:1;
    ULONG DelayedTransactionDiscardTimerExpired:1;
    ULONG PERRAsserted:1;
    ULONG SERRAsserted:1;
    ULONG InternalBridgeError:1;
    ULONG Reserved:18;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_MASK, *PPCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_MASK;

typedef union _PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_SEVERITY {
  struct {
    ULONG TargetAbortOnSplitCompletion:1;
    ULONG MasterAbortOnSplitCompletion:1;
    ULONG ReceivedTargetAbort:1;
    ULONG ReceivedMasterAbort:1;
    ULONG RsvdZ:1;
    ULONG UnexpectedSplitCompletionError:1;
    ULONG UncorrectableSplitCompletion:1;
    ULONG UncorrectableDataError:1;
    ULONG UncorrectableAttributeError:1;
    ULONG UncorrectableAddressError:1;
    ULONG DelayedTransactionDiscardTimerExpired:1;
    ULONG PERRAsserted:1;
    ULONG SERRAsserted:1;
    ULONG InternalBridgeError:1;
    ULONG Reserved:18;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_SEVERITY, *PPCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_SEVERITY;

typedef union _PCI_EXPRESS_SEC_AER_CAPABILITIES {
  struct {
    ULONG SecondaryUncorrectableFirstErrorPtr:5;
    ULONG Reserved:27;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SEC_AER_CAPABILITIES, *PPCI_EXPRESS_SEC_AER_CAPABILITIES;

#define ROOT_CMD_ENABLE_CORRECTABLE_ERROR_REPORTING  0x00000001
#define ROOT_CMD_ENABLE_NONFATAL_ERROR_REPORTING     0x00000002
#define ROOT_CMD_ENABLE_FATAL_ERROR_REPORTING        0x00000004

#define ROOT_CMD_ERROR_REPORTING_ENABLE_MASK \
    (ROOT_CMD_ENABLE_FATAL_ERROR_REPORTING | \
     ROOT_CMD_ENABLE_NONFATAL_ERROR_REPORTING | \
     ROOT_CMD_ENABLE_CORRECTABLE_ERROR_REPORTING)

typedef struct _PCI_EXPRESS_AER_CAPABILITY {
  PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS UncorrectableErrorStatus;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_MASK UncorrectableErrorMask;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY UncorrectableErrorSeverity;
  PCI_EXPRESS_CORRECTABLE_ERROR_STATUS CorrectableErrorStatus;
  PCI_EXPRESS_CORRECTABLE_ERROR_MASK CorrectableErrorMask;
  PCI_EXPRESS_AER_CAPABILITIES CapabilitiesAndControl;
  ULONG HeaderLog[4];
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_STATUS SecUncorrectableErrorStatus;
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_MASK SecUncorrectableErrorMask;
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_SEVERITY SecUncorrectableErrorSeverity;
  PCI_EXPRESS_SEC_AER_CAPABILITIES SecCapabilitiesAndControl;
  ULONG SecHeaderLog[4];
} PCI_EXPRESS_AER_CAPABILITY, *PPCI_EXPRESS_AER_CAPABILITY;

typedef struct _PCI_EXPRESS_ROOTPORT_AER_CAPABILITY {
  PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS UncorrectableErrorStatus;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_MASK UncorrectableErrorMask;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY UncorrectableErrorSeverity;
  PCI_EXPRESS_CORRECTABLE_ERROR_STATUS CorrectableErrorStatus;
  PCI_EXPRESS_CORRECTABLE_ERROR_MASK CorrectableErrorMask;
  PCI_EXPRESS_AER_CAPABILITIES CapabilitiesAndControl;
  ULONG HeaderLog[4];
  PCI_EXPRESS_ROOT_ERROR_COMMAND RootErrorCommand;
  PCI_EXPRESS_ROOT_ERROR_STATUS RootErrorStatus;
  PCI_EXPRESS_ERROR_SOURCE_ID ErrorSourceId;
} PCI_EXPRESS_ROOTPORT_AER_CAPABILITY, *PPCI_EXPRESS_ROOTPORT_AER_CAPABILITY;

typedef struct _PCI_EXPRESS_BRIDGE_AER_CAPABILITY {
  PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_STATUS UncorrectableErrorStatus;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_MASK UncorrectableErrorMask;
  PCI_EXPRESS_UNCORRECTABLE_ERROR_SEVERITY UncorrectableErrorSeverity;
  PCI_EXPRESS_CORRECTABLE_ERROR_STATUS CorrectableErrorStatus;
  PCI_EXPRESS_CORRECTABLE_ERROR_MASK CorrectableErrorMask;
  PCI_EXPRESS_AER_CAPABILITIES CapabilitiesAndControl;
  ULONG HeaderLog[4];
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_STATUS SecUncorrectableErrorStatus;
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_MASK SecUncorrectableErrorMask;
  PCI_EXPRESS_SEC_UNCORRECTABLE_ERROR_SEVERITY SecUncorrectableErrorSeverity;
  PCI_EXPRESS_SEC_AER_CAPABILITIES SecCapabilitiesAndControl;
  ULONG SecHeaderLog[4];
} PCI_EXPRESS_BRIDGE_AER_CAPABILITY, *PPCI_EXPRESS_BRIDGE_AER_CAPABILITY;

typedef union _PCI_EXPRESS_SRIOV_CAPS {
  struct {
    ULONG VFMigrationCapable:1;
    ULONG Reserved1:20;
    ULONG VFMigrationInterruptNumber:11;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SRIOV_CAPS, *PPCI_EXPRESS_SRIOV_CAPS;

typedef union _PCI_EXPRESS_SRIOV_CONTROL {
  struct {
    USHORT VFEnable:1;
    USHORT VFMigrationEnable:1;
    USHORT VFMigrationInterruptEnable:1;
    USHORT VFMemorySpaceEnable:1;
    USHORT ARICapableHierarchy:1;
    USHORT Reserved1:11;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_SRIOV_CONTROL, *PPCI_EXPRESS_SRIOV_CONTROL;

typedef union _PCI_EXPRESS_SRIOV_STATUS {
  struct {
    USHORT VFMigrationStatus:1;
    USHORT Reserved1:15;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_SRIOV_STATUS, *PPCI_EXPRESS_SRIOV_STATUS;

typedef union _PCI_EXPRESS_SRIOV_MIGRATION_STATE_ARRAY {
  struct {
    ULONG VFMigrationStateBIR:3;
    ULONG VFMigrationStateOffset:29;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SRIOV_MIGRATION_STATE_ARRAY, *PPCI_EXPRESS_SRIOV_MIGRATION_STATE_ARRAY;

typedef struct _PCI_EXPRESS_SRIOV_CAPABILITY {
  PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
  PCI_EXPRESS_SRIOV_CAPS SRIOVCapabilities;
  PCI_EXPRESS_SRIOV_CONTROL SRIOVControl;
  PCI_EXPRESS_SRIOV_STATUS SRIOVStatus;
  USHORT InitialVFs;
  USHORT TotalVFs;
  USHORT NumVFs;
  UCHAR FunctionDependencyLink;
  UCHAR RsvdP1;
  USHORT FirstVFOffset;
  USHORT VFStride;
  USHORT RsvdP2;
  USHORT VFDeviceId;
  ULONG SupportedPageSizes;
  ULONG SystemPageSize;
  ULONG BaseAddresses[PCI_TYPE0_ADDRESSES];
  PCI_EXPRESS_SRIOV_MIGRATION_STATE_ARRAY VFMigrationStateArrayOffset;
} PCI_EXPRESS_SRIOV_CAPABILITY, *PPCI_EXPRESS_SRIOV_CAPABILITY;

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
#define PCI_CLASS_WIRELESS_CTLR             0x0d
#define PCI_CLASS_INTELLIGENT_IO_CTLR       0x0e
#define PCI_CLASS_SATELLITE_COMMS_CTLR      0x0f
#define PCI_CLASS_ENCRYPTION_DECRYPTION     0x10
#define PCI_CLASS_DATA_ACQ_SIGNAL_PROC      0x11
#define PCI_CLASS_NOT_DEFINED               0xff

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
#define PCI_SUBCLASS_NET_ISDN_CTLR          0x04
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
#define PCI_SUBCLASS_BR_RACEWAY             0x08
#define PCI_SUBCLASS_BR_OTHER               0x80

#define PCI_SUBCLASS_COM_SERIAL             0x00
#define PCI_SUBCLASS_COM_PARALLEL           0x01
#define PCI_SUBCLASS_COM_MULTIPORT          0x02
#define PCI_SUBCLASS_COM_MODEM              0x03
#define PCI_SUBCLASS_COM_OTHER              0x80

#define PCI_SUBCLASS_SYS_INTERRUPT_CTLR     0x00
#define PCI_SUBCLASS_SYS_DMA_CTLR           0x01
#define PCI_SUBCLASS_SYS_SYSTEM_TIMER       0x02
#define PCI_SUBCLASS_SYS_REAL_TIME_CLOCK    0x03
#define PCI_SUBCLASS_SYS_GEN_HOTPLUG_CTLR   0x04
#define PCI_SUBCLASS_SYS_SDIO_CTRL          0x05
#define PCI_SUBCLASS_SYS_OTHER              0x80

#define PCI_SUBCLASS_INP_KEYBOARD           0x00
#define PCI_SUBCLASS_INP_DIGITIZER          0x01
#define PCI_SUBCLASS_INP_MOUSE              0x02
#define PCI_SUBCLASS_INP_SCANNER            0x03
#define PCI_SUBCLASS_INP_GAMEPORT           0x04
#define PCI_SUBCLASS_INP_OTHER              0x80

#define PCI_SUBCLASS_DOC_GENERIC            0x00
#define PCI_SUBCLASS_DOC_OTHER              0x80

#define PCI_SUBCLASS_PROC_386               0x00
#define PCI_SUBCLASS_PROC_486               0x01
#define PCI_SUBCLASS_PROC_PENTIUM           0x02
#define PCI_SUBCLASS_PROC_ALPHA             0x10
#define PCI_SUBCLASS_PROC_POWERPC           0x20
#define PCI_SUBCLASS_PROC_COPROCESSOR       0x40

/* PCI device subclasses for class C (serial bus controller)*/
#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04
#define PCI_SUBCLASS_SB_SMBUS               0x05

#define PCI_SUBCLASS_WIRELESS_IRDA          0x00
#define PCI_SUBCLASS_WIRELESS_CON_IR        0x01
#define PCI_SUBCLASS_WIRELESS_RF            0x10
#define PCI_SUBCLASS_WIRELESS_OTHER         0x80

#define PCI_SUBCLASS_INTIO_I2O              0x00

#define PCI_SUBCLASS_SAT_TV                 0x01
#define PCI_SUBCLASS_SAT_AUDIO              0x02
#define PCI_SUBCLASS_SAT_VOICE              0x03
#define PCI_SUBCLASS_SAT_DATA               0x04

#define PCI_SUBCLASS_CRYPTO_NET_COMP        0x00
#define PCI_SUBCLASS_CRYPTO_ENTERTAINMENT   0x10
#define PCI_SUBCLASS_CRYPTO_OTHER           0x80

#define PCI_SUBCLASS_DASP_DPIO              0x00
#define PCI_SUBCLASS_DASP_OTHER             0x80

#define PCI_ADDRESS_IO_SPACE                0x00000001
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008
#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT                      0
#define PCI_TYPE_20BIT                      2
#define PCI_TYPE_64BIT                      4

#define PCI_ROMADDRESS_ENABLED              0x00000001

#endif /* _PCI_X_ */

#define PCI_EXPRESS_LINK_QUIESCENT_INTERFACE_VERSION       1

typedef NTSTATUS
(NTAPI PCI_EXPRESS_ENTER_LINK_QUIESCENT_MODE)(
  IN OUT PVOID Context);
typedef PCI_EXPRESS_ENTER_LINK_QUIESCENT_MODE *PPCI_EXPRESS_ENTER_LINK_QUIESCENT_MODE;

typedef NTSTATUS
(NTAPI PCI_EXPRESS_EXIT_LINK_QUIESCENT_MODE)(
  IN OUT PVOID Context);
typedef PCI_EXPRESS_EXIT_LINK_QUIESCENT_MODE *PPCI_EXPRESS_EXIT_LINK_QUIESCENT_MODE;

typedef struct _PCI_EXPRESS_LINK_QUIESCENT_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PPCI_EXPRESS_ENTER_LINK_QUIESCENT_MODE PciExpressEnterLinkQuiescentMode;
  PPCI_EXPRESS_EXIT_LINK_QUIESCENT_MODE PciExpressExitLinkQuiescentMode;
} PCI_EXPRESS_LINK_QUIESCENT_INTERFACE, *PPCI_EXPRESS_LINK_QUIESCENT_INTERFACE;

#define PCI_EXPRESS_ROOT_PORT_INTERFACE_VERSION            1

typedef ULONG
(NTAPI *PPCI_EXPRESS_ROOT_PORT_READ_CONFIG_SPACE)(
  IN PVOID Context,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef ULONG
(NTAPI *PPCI_EXPRESS_ROOT_PORT_WRITE_CONFIG_SPACE)(
  IN PVOID Context,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef struct _PCI_EXPRESS_ROOT_PORT_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PPCI_EXPRESS_ROOT_PORT_READ_CONFIG_SPACE ReadConfigSpace;
  PPCI_EXPRESS_ROOT_PORT_WRITE_CONFIG_SPACE WriteConfigSpace;
} PCI_EXPRESS_ROOT_PORT_INTERFACE, *PPCI_EXPRESS_ROOT_PORT_INTERFACE;

#define PCI_MSIX_TABLE_CONFIG_INTERFACE_VERSION            1

typedef NTSTATUS
(NTAPI PCI_MSIX_SET_ENTRY)(
  IN PVOID Context,
  IN ULONG TableEntry,
  IN ULONG MessageNumber);
typedef PCI_MSIX_SET_ENTRY *PPCI_MSIX_SET_ENTRY;

typedef NTSTATUS
(NTAPI PCI_MSIX_MASKUNMASK_ENTRY)(
  IN PVOID Context,
  IN ULONG TableEntry);
typedef PCI_MSIX_MASKUNMASK_ENTRY *PPCI_MSIX_MASKUNMASK_ENTRY;

typedef NTSTATUS
(NTAPI PCI_MSIX_GET_ENTRY)(
  IN PVOID Context,
  IN ULONG TableEntry,
  OUT PULONG MessageNumber,
  OUT PBOOLEAN Masked);
typedef PCI_MSIX_GET_ENTRY *PPCI_MSIX_GET_ENTRY;

typedef NTSTATUS
(NTAPI PCI_MSIX_GET_TABLE_SIZE)(
  IN PVOID Context,
  OUT PULONG TableSize);
typedef PCI_MSIX_GET_TABLE_SIZE *PPCI_MSIX_GET_TABLE_SIZE;

typedef struct _PCI_MSIX_TABLE_CONFIG_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PPCI_MSIX_SET_ENTRY SetTableEntry;
  PPCI_MSIX_MASKUNMASK_ENTRY MaskTableEntry;
  PPCI_MSIX_MASKUNMASK_ENTRY UnmaskTableEntry;
  PPCI_MSIX_GET_ENTRY GetTableEntry;
  PPCI_MSIX_GET_TABLE_SIZE GetTableSize;
} PCI_MSIX_TABLE_CONFIG_INTERFACE, *PPCI_MSIX_TABLE_CONFIG_INTERFACE;

#define PCI_MSIX_TABLE_CONFIG_MINIMUM_SIZE \
        RTL_SIZEOF_THROUGH_FIELD(PCI_MSIX_TABLE_CONFIG_INTERFACE, UnmaskTableEntry)
$endif
$if (_NTDDK_)

#ifndef _ARC_DDK_
#define _ARC_DDK_
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
#endif /* !_ARC_DDK_ */

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

#define IRP_MN_FLUSH_AND_PURGE          0x01

#define IRP_MN_NORMAL                     0x00
#define IRP_MN_DPC                        0x01
#define IRP_MN_MDL                        0x02
#define IRP_MN_COMPLETE                   0x04
#define IRP_MN_COMPRESSED                 0x08

#define IRP_MN_MDL_DPC                    (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL               (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC           (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)

#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

#define IO_CHECK_CREATE_PARAMETERS      0x0200
#define IO_ATTACH_DEVICE                0x0400
#define IO_IGNORE_SHARE_ACCESS_CHECK    0x0800

typedef
NTSTATUS
(NTAPI *PIO_QUERY_DEVICE_ROUTINE)(
  IN PVOID Context,
  IN PUNICODE_STRING PathName,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
  IN CONFIGURATION_TYPE ControllerType,
  IN ULONG ControllerNumber,
  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
  IN CONFIGURATION_TYPE PeripheralType,
  IN ULONG PeripheralNumber,
  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation);

typedef enum _IO_QUERY_DEVICE_DATA_FORMAT {
  IoQueryDeviceIdentifier = 0,
  IoQueryDeviceConfigurationData,
  IoQueryDeviceComponentInformation,
  IoQueryDeviceMaxData
} IO_QUERY_DEVICE_DATA_FORMAT, *PIO_QUERY_DEVICE_DATA_FORMAT;

typedef VOID
(NTAPI *PDRIVER_REINITIALIZE)(
  IN struct _DRIVER_OBJECT *DriverObject,
  IN PVOID Context OPTIONAL,
  IN ULONG Count);

typedef struct _CONTROLLER_OBJECT {
  CSHORT Type;
  CSHORT Size;
  PVOID ControllerExtension;
  KDEVICE_QUEUE DeviceWaitQueue;
  ULONG Spare1;
  LARGE_INTEGER Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

#define DRVO_REINIT_REGISTERED          0x00000008
#define DRVO_INITIALIZED                0x00000010
#define DRVO_BOOTREINIT_REGISTERED      0x00000020
#define DRVO_LEGACY_RESOURCES           0x00000040

typedef struct _CONFIGURATION_INFORMATION {
  ULONG DiskCount;
  ULONG FloppyCount;
  ULONG CdRomCount;
  ULONG TapeCount;
  ULONG ScsiPortCount;
  ULONG SerialCount;
  ULONG ParallelCount;
  BOOLEAN AtDiskPrimaryAddressClaimed;
  BOOLEAN AtDiskSecondaryAddressClaimed;
  ULONG Version;
  ULONG MediumChangerCount;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef struct _DISK_SIGNATURE {
  ULONG PartitionStyle;
  _ANONYMOUS_UNION union {
    struct {
      ULONG Signature;
      ULONG CheckSum;
    } Mbr;
    struct {
      GUID DiskId;
    } Gpt;
  } DUMMYUNIONNAME;
} DISK_SIGNATURE, *PDISK_SIGNATURE;

typedef struct _TXN_PARAMETER_BLOCK {
  USHORT Length;
  USHORT TxFsContext;
  PVOID TransactionObject;
} TXN_PARAMETER_BLOCK, *PTXN_PARAMETER_BLOCK;

#define TXF_MINIVERSION_DEFAULT_VIEW        (0xFFFE)

typedef struct _IO_DRIVER_CREATE_CONTEXT {
  CSHORT Size;
  struct _ECP_LIST *ExtraCreateParameter;
  PVOID DeviceObjectHint;
  PTXN_PARAMETER_BLOCK TxnParameters;
} IO_DRIVER_CREATE_CONTEXT, *PIO_DRIVER_CREATE_CONTEXT;

typedef struct _AGP_TARGET_BUS_INTERFACE_STANDARD {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PGET_SET_DEVICE_DATA SetBusData;
  PGET_SET_DEVICE_DATA GetBusData;
  UCHAR CapabilityID;
} AGP_TARGET_BUS_INTERFACE_STANDARD, *PAGP_TARGET_BUS_INTERFACE_STANDARD;

typedef NTSTATUS
(NTAPI *PGET_LOCATION_STRING)(
  IN OUT PVOID Context OPTIONAL,
  OUT PWCHAR *LocationStrings);

typedef struct _PNP_LOCATION_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PGET_LOCATION_STRING GetLocationString;
} PNP_LOCATION_INTERFACE, *PPNP_LOCATION_INTERFACE;

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
  PDEVICE_OBJECT OwningObject;
  ULONGLONG Start;
  ULONGLONG End;
} ARBITER_CONFLICT_INFO, *PARBITER_CONFLICT_INFO;

typedef struct _ARBITER_TEST_ALLOCATION_PARAMETERS {
  IN OUT PLIST_ENTRY ArbitrationList;
  IN ULONG AllocateFromCount;
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;
} ARBITER_TEST_ALLOCATION_PARAMETERS, *PARBITER_TEST_ALLOCATION_PARAMETERS;

typedef struct _ARBITER_RETEST_ALLOCATION_PARAMETERS {
  IN OUT PLIST_ENTRY ArbitrationList;
  IN ULONG AllocateFromCount;
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;
} ARBITER_RETEST_ALLOCATION_PARAMETERS, *PARBITER_RETEST_ALLOCATION_PARAMETERS;

typedef struct _ARBITER_BOOT_ALLOCATION_PARAMETERS {
  IN OUT PLIST_ENTRY ArbitrationList;
} ARBITER_BOOT_ALLOCATION_PARAMETERS, *PARBITER_BOOT_ALLOCATION_PARAMETERS;

typedef struct _ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS {
  OUT PCM_PARTIAL_RESOURCE_LIST *AllocatedResources;
} ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS, *PARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS;

typedef struct _ARBITER_QUERY_CONFLICT_PARAMETERS {
  IN PDEVICE_OBJECT PhysicalDeviceObject;
  IN PIO_RESOURCE_DESCRIPTOR ConflictingResource;
  OUT PULONG ConflictCount;
  OUT PARBITER_CONFLICT_INFO *Conflicts;
} ARBITER_QUERY_CONFLICT_PARAMETERS, *PARBITER_QUERY_CONFLICT_PARAMETERS;

typedef struct _ARBITER_QUERY_ARBITRATE_PARAMETERS {
  IN PLIST_ENTRY ArbitrationList;
} ARBITER_QUERY_ARBITRATE_PARAMETERS, *PARBITER_QUERY_ARBITRATE_PARAMETERS;

typedef struct _ARBITER_ADD_RESERVED_PARAMETERS {
  IN PDEVICE_OBJECT ReserveDevice;
} ARBITER_ADD_RESERVED_PARAMETERS, *PARBITER_ADD_RESERVED_PARAMETERS;

typedef struct _ARBITER_PARAMETERS {
  union {
    ARBITER_TEST_ALLOCATION_PARAMETERS TestAllocation;
    ARBITER_RETEST_ALLOCATION_PARAMETERS RetestAllocation;
    ARBITER_BOOT_ALLOCATION_PARAMETERS BootAllocation;
    ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS QueryAllocatedResources;
    ARBITER_QUERY_CONFLICT_PARAMETERS QueryConflict;
    ARBITER_QUERY_ARBITRATE_PARAMETERS QueryArbitrate;
    ARBITER_ADD_RESERVED_PARAMETERS AddReserved;
  } Parameters;
} ARBITER_PARAMETERS, *PARBITER_PARAMETERS;

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

#define ARBITER_FLAG_BOOT_CONFIG 0x00000001

typedef struct _ARBITER_LIST_ENTRY {
  LIST_ENTRY ListEntry;
  ULONG AlternativeCount;
  PIO_RESOURCE_DESCRIPTOR Alternatives;
  PDEVICE_OBJECT PhysicalDeviceObject;
  ARBITER_REQUEST_SOURCE RequestSource;
  ULONG Flags;
  LONG_PTR WorkSpace;
  INTERFACE_TYPE InterfaceType;
  ULONG SlotNumber;
  ULONG BusNumber;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR Assignment;
  PIO_RESOURCE_DESCRIPTOR SelectedAlternative;
  ARBITER_RESULT Result;
} ARBITER_LIST_ENTRY, *PARBITER_LIST_ENTRY;

typedef NTSTATUS
(NTAPI *PARBITER_HANDLER)(
  IN OUT PVOID Context,
  IN ARBITER_ACTION Action,
  IN OUT PARBITER_PARAMETERS Parameters);

#define ARBITER_PARTIAL 0x00000001

typedef struct _ARBITER_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PARBITER_HANDLER ArbiterHandler;
  ULONG Flags;
} ARBITER_INTERFACE, *PARBITER_INTERFACE;

typedef enum _RESOURCE_TRANSLATION_DIRECTION {
  TranslateChildToParent,
  TranslateParentToChild
} RESOURCE_TRANSLATION_DIRECTION;

typedef NTSTATUS
(NTAPI *PTRANSLATE_RESOURCE_HANDLER)(
  IN OUT PVOID Context OPTIONAL,
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
  IN RESOURCE_TRANSLATION_DIRECTION Direction,
  IN ULONG AlternativesCount OPTIONAL,
  IN IO_RESOURCE_DESCRIPTOR Alternatives[],
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target);

typedef NTSTATUS
(NTAPI *PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(
  IN OUT PVOID Context OPTIONAL,
  IN PIO_RESOURCE_DESCRIPTOR Source,
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  OUT PULONG TargetCount,
  OUT PIO_RESOURCE_DESCRIPTOR *Target);

typedef struct _TRANSLATOR_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSLATE_RESOURCE_HANDLER TranslateResources;
  PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;

typedef struct _PCI_AGP_CAPABILITY {
  PCI_CAPABILITIES_HEADER Header;
  USHORT Minor:4;
  USHORT Major:4;
  USHORT Rsvd1:8;
  struct _PCI_AGP_STATUS {
    ULONG Rate:3;
    ULONG Agp3Mode:1;
    ULONG FastWrite:1;
    ULONG FourGB:1;
    ULONG HostTransDisable:1;
    ULONG Gart64:1;
    ULONG ITA_Coherent:1;
    ULONG SideBandAddressing:1;
    ULONG CalibrationCycle:3;
    ULONG AsyncRequestSize:3;
    ULONG Rsvd1:1;
    ULONG Isoch:1;
    ULONG Rsvd2:6;
    ULONG RequestQueueDepthMaximum:8;
  } AGPStatus;
  struct _PCI_AGP_COMMAND {
    ULONG Rate:3;
    ULONG Rsvd1:1;
    ULONG FastWriteEnable:1;
    ULONG FourGBEnable:1;
    ULONG Rsvd2:1;
    ULONG Gart64:1;
    ULONG AGPEnable:1;
    ULONG SBAEnable:1;
    ULONG CalibrationCycle:3;
    ULONG AsyncReqSize:3;
    ULONG Rsvd3:8;
    ULONG RequestQueueDepth:8;
  } AGPCommand;
} PCI_AGP_CAPABILITY, *PPCI_AGP_CAPABILITY;

typedef enum _EXTENDED_AGP_REGISTER {
  IsochStatus,
  AgpControl,
  ApertureSize,
  AperturePageSize,
  GartLow,
  GartHigh,
  IsochCommand
} EXTENDED_AGP_REGISTER, *PEXTENDED_AGP_REGISTER;

typedef struct _PCI_AGP_ISOCH_STATUS {
  ULONG ErrorCode:2;
  ULONG Rsvd1:1;
  ULONG Isoch_L:3;
  ULONG Isoch_Y:2;
  ULONG Isoch_N:8;
  ULONG Rsvd2:16;
} PCI_AGP_ISOCH_STATUS, *PPCI_AGP_ISOCH_STATUS;

typedef struct _PCI_AGP_CONTROL {
  ULONG Rsvd1:7;
  ULONG GTLB_Enable:1;
  ULONG AP_Enable:1;
  ULONG CAL_Disable:1;
  ULONG Rsvd2:22;
} PCI_AGP_CONTROL, *PPCI_AGP_CONTROL;

typedef struct _PCI_AGP_APERTURE_PAGE_SIZE {
  USHORT PageSizeMask:11;
  USHORT Rsvd1:1;
  USHORT PageSizeSelect:4;
} PCI_AGP_APERTURE_PAGE_SIZE, *PPCI_AGP_APERTURE_PAGE_SIZE;

typedef struct _PCI_AGP_ISOCH_COMMAND {
  USHORT Rsvd1:6;
  USHORT Isoch_Y:2;
  USHORT Isoch_N:8;
} PCI_AGP_ISOCH_COMMAND, *PPCI_AGP_ISOCH_COMMAND;

typedef struct PCI_AGP_EXTENDED_CAPABILITY {
  PCI_AGP_ISOCH_STATUS IsochStatus;
  PCI_AGP_CONTROL AgpControl;
  USHORT ApertureSize;
  PCI_AGP_APERTURE_PAGE_SIZE AperturePageSize;
  ULONG GartLow;
  ULONG GartHigh;
  PCI_AGP_ISOCH_COMMAND IsochCommand;
} PCI_AGP_EXTENDED_CAPABILITY, *PPCI_AGP_EXTENDED_CAPABILITY;

#define PCI_AGP_RATE_1X     0x1
#define PCI_AGP_RATE_2X     0x2
#define PCI_AGP_RATE_4X     0x4

#define PCIX_MODE_CONVENTIONAL_PCI  0x0
#define PCIX_MODE1_66MHZ            0x1
#define PCIX_MODE1_100MHZ           0x2
#define PCIX_MODE1_133MHZ           0x3
#define PCIX_MODE2_266_66MHZ        0x9
#define PCIX_MODE2_266_100MHZ       0xA
#define PCIX_MODE2_266_133MHZ       0xB
#define PCIX_MODE2_533_66MHZ        0xD
#define PCIX_MODE2_533_100MHZ       0xE
#define PCIX_MODE2_533_133MHZ       0xF

#define PCIX_VERSION_MODE1_ONLY     0x0
#define PCIX_VERSION_MODE2_ECC      0x1
#define PCIX_VERSION_DUAL_MODE_ECC  0x2

typedef struct _PCIX_BRIDGE_CAPABILITY {
  PCI_CAPABILITIES_HEADER Header;
  union {
    struct {
      USHORT Bus64Bit:1;
      USHORT Bus133MHzCapable:1;
      USHORT SplitCompletionDiscarded:1;
      USHORT UnexpectedSplitCompletion:1;
      USHORT SplitCompletionOverrun:1;
      USHORT SplitRequestDelayed:1;
      USHORT BusModeFrequency:4;
      USHORT Rsvd:2;
      USHORT Version:2;
      USHORT Bus266MHzCapable:1;
      USHORT Bus533MHzCapable:1;
    } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
  } SecondaryStatus;
  union {
    struct {
      ULONG FunctionNumber:3;
      ULONG DeviceNumber:5;
      ULONG BusNumber:8;
      ULONG Device64Bit:1;
      ULONG Device133MHzCapable:1;
      ULONG SplitCompletionDiscarded:1;
      ULONG UnexpectedSplitCompletion:1;
      ULONG SplitCompletionOverrun:1;
      ULONG SplitRequestDelayed:1;
      ULONG Rsvd:7;
      ULONG DIMCapable:1;
      ULONG Device266MHzCapable:1;
      ULONG Device533MHzCapable:1;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
  } BridgeStatus;
  USHORT UpstreamSplitTransactionCapacity;
  USHORT UpstreamSplitTransactionLimit;
  USHORT DownstreamSplitTransactionCapacity;
  USHORT DownstreamSplitTransactionLimit;
  union {
    struct {
      ULONG SelectSecondaryRegisters:1;
      ULONG ErrorPresentInOtherBank:1;
      ULONG AdditionalCorrectableError:1;
      ULONG AdditionalUncorrectableError:1;
      ULONG ErrorPhase:3;
      ULONG ErrorCorrected:1;
      ULONG Syndrome:8;
      ULONG ErrorFirstCommand:4;
      ULONG ErrorSecondCommand:4;
      ULONG ErrorUpperAttributes:4;
      ULONG ControlUpdateEnable:1;
      ULONG Rsvd:1;
      ULONG DisableSingleBitCorrection:1;
      ULONG EccMode:1;
    } DUMMYSTRUCTNAME;
  ULONG AsULONG;
  } EccControlStatus;
  ULONG EccFirstAddress;
  ULONG EccSecondAddress;
  ULONG EccAttribute;
} PCIX_BRIDGE_CAPABILITY, *PPCIX_BRIDGE_CAPABILITY;

typedef struct _PCI_SUBSYSTEM_IDS_CAPABILITY {
  PCI_CAPABILITIES_HEADER Header;
  USHORT Reserved;
  USHORT SubVendorID;
  USHORT SubSystemID;
} PCI_SUBSYSTEM_IDS_CAPABILITY, *PPCI_SUBSYSTEM_IDS_CAPABILITY;

#define OSC_FIRMWARE_FAILURE                            0x02
#define OSC_UNRECOGNIZED_UUID                           0x04
#define OSC_UNRECOGNIZED_REVISION                       0x08
#define OSC_CAPABILITIES_MASKED                         0x10

#define PCI_ROOT_BUS_OSC_METHOD_CAPABILITY_REVISION     0x01

typedef struct _PCI_ROOT_BUS_OSC_SUPPORT_FIELD {
  union {
    struct {
      ULONG ExtendedConfigOpRegions:1;
      ULONG ActiveStatePowerManagement:1;
      ULONG ClockPowerManagement:1;
      ULONG SegmentGroups:1;
      ULONG MessageSignaledInterrupts:1;
      ULONG WindowsHardwareErrorArchitecture:1;
      ULONG Reserved:26;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
  } u;
} PCI_ROOT_BUS_OSC_SUPPORT_FIELD, *PPCI_ROOT_BUS_OSC_SUPPORT_FIELD;

typedef struct _PCI_ROOT_BUS_OSC_CONTROL_FIELD {
  union {
    struct {
      ULONG ExpressNativeHotPlug:1;
      ULONG ShpcNativeHotPlug:1;
      ULONG ExpressNativePME:1;
      ULONG ExpressAdvancedErrorReporting:1;
      ULONG ExpressCapabilityStructure:1;
      ULONG Reserved:27;
    } DUMMYSTRUCTNAME;
  ULONG AsULONG;
  } u;
} PCI_ROOT_BUS_OSC_CONTROL_FIELD, *PPCI_ROOT_BUS_OSC_CONTROL_FIELD;

typedef enum _PCI_HARDWARE_INTERFACE {
  PciConventional,
  PciXMode1,
  PciXMode2,
  PciExpress
} PCI_HARDWARE_INTERFACE, *PPCI_HARDWARE_INTERFACE;

typedef enum {
  BusWidth32Bits,
  BusWidth64Bits
} PCI_BUS_WIDTH;

typedef struct _PCI_ROOT_BUS_HARDWARE_CAPABILITY {
  PCI_HARDWARE_INTERFACE SecondaryInterface;
  struct {
    BOOLEAN BusCapabilitiesFound;
    ULONG CurrentSpeedAndMode;
    ULONG SupportedSpeedsAndModes;
    BOOLEAN DeviceIDMessagingCapable;
    PCI_BUS_WIDTH SecondaryBusWidth;
  } DUMMYSTRUCTNAME;
  PCI_ROOT_BUS_OSC_SUPPORT_FIELD OscFeatureSupport;
  PCI_ROOT_BUS_OSC_CONTROL_FIELD OscControlRequest;
  PCI_ROOT_BUS_OSC_CONTROL_FIELD OscControlGranted;
} PCI_ROOT_BUS_HARDWARE_CAPABILITY, *PPCI_ROOT_BUS_HARDWARE_CAPABILITY;

typedef union _PCI_EXPRESS_CAPABILITIES_REGISTER {
  struct {
    USHORT CapabilityVersion:4;
    USHORT DeviceType:4;
    USHORT SlotImplemented:1;
    USHORT InterruptMessageNumber:5;
    USHORT Rsvd:2;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_CAPABILITIES_REGISTER, *PPCI_EXPRESS_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER {
  struct {
    ULONG MaxPayloadSizeSupported:3;
    ULONG PhantomFunctionsSupported:2;
    ULONG ExtendedTagSupported:1;
    ULONG L0sAcceptableLatency:3;
    ULONG L1AcceptableLatency:3;
    ULONG Undefined:3;
    ULONG RoleBasedErrorReporting:1;
    ULONG Rsvd1:2;
    ULONG CapturedSlotPowerLimit:8;
    ULONG CapturedSlotPowerLimitScale:2;
    ULONG Rsvd2:4;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER, *PPCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER;

#define PCI_EXPRESS_AER_DEVICE_CONTROL_MASK 0x07;

typedef union _PCI_EXPRESS_DEVICE_CONTROL_REGISTER {
  struct {
    USHORT CorrectableErrorEnable:1;
    USHORT NonFatalErrorEnable:1;
    USHORT FatalErrorEnable:1;
    USHORT UnsupportedRequestErrorEnable:1;
    USHORT EnableRelaxedOrder:1;
    USHORT MaxPayloadSize:3;
    USHORT ExtendedTagEnable:1;
    USHORT PhantomFunctionsEnable:1;
    USHORT AuxPowerEnable:1;
    USHORT NoSnoopEnable:1;
    USHORT MaxReadRequestSize:3;
    USHORT BridgeConfigRetryEnable:1;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_DEVICE_CONTROL_REGISTER, *PPCI_EXPRESS_DEVICE_CONTROL_REGISTER;

#define PCI_EXPRESS_AER_DEVICE_STATUS_MASK 0x0F;

typedef union _PCI_EXPRESS_DEVICE_STATUS_REGISTER {
  struct {
    USHORT CorrectableErrorDetected:1;
    USHORT NonFatalErrorDetected:1;
    USHORT FatalErrorDetected:1;
    USHORT UnsupportedRequestDetected:1;
    USHORT AuxPowerDetected:1;
    USHORT TransactionsPending:1;
    USHORT Rsvd:10;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_DEVICE_STATUS_REGISTER, *PPCI_EXPRESS_DEVICE_STATUS_REGISTER;

typedef union _PCI_EXPRESS_LINK_CAPABILITIES_REGISTER {
  struct {
    ULONG MaximumLinkSpeed:4;
    ULONG MaximumLinkWidth:6;
    ULONG ActiveStatePMSupport:2;
    ULONG L0sExitLatency:3;
    ULONG L1ExitLatency:3;
    ULONG ClockPowerManagement:1;
    ULONG SurpriseDownErrorReportingCapable:1;
    ULONG DataLinkLayerActiveReportingCapable:1;
    ULONG Rsvd:3;
    ULONG PortNumber:8;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_LINK_CAPABILITIES_REGISTER, *PPCI_EXPRESS_LINK_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_LINK_CONTROL_REGISTER {
  struct {
    USHORT ActiveStatePMControl:2;
    USHORT Rsvd1:1;
    USHORT ReadCompletionBoundary:1;
    USHORT LinkDisable:1;
    USHORT RetrainLink:1;
    USHORT CommonClockConfig:1;
    USHORT ExtendedSynch:1;
    USHORT EnableClockPowerManagement:1;
    USHORT Rsvd2:7;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_LINK_CONTROL_REGISTER, *PPCI_EXPRESS_LINK_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_LINK_STATUS_REGISTER {
  struct {
    USHORT LinkSpeed:4;
    USHORT LinkWidth:6;
    USHORT Undefined:1;
    USHORT LinkTraining:1;
    USHORT SlotClockConfig:1;
    USHORT DataLinkLayerActive:1;
    USHORT Rsvd:2;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_LINK_STATUS_REGISTER, *PPCI_EXPRESS_LINK_STATUS_REGISTER;

typedef union _PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER {
  struct {
    ULONG AttentionButtonPresent:1;
    ULONG PowerControllerPresent:1;
    ULONG MRLSensorPresent:1;
    ULONG AttentionIndicatorPresent:1;
    ULONG PowerIndicatorPresent:1;
    ULONG HotPlugSurprise:1;
    ULONG HotPlugCapable:1;
    ULONG SlotPowerLimit:8;
    ULONG SlotPowerLimitScale:2;
    ULONG ElectromechanicalLockPresent:1;
    ULONG NoCommandCompletedSupport:1;
    ULONG PhysicalSlotNumber:13;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_SLOT_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_SLOT_CONTROL_REGISTER {
  struct {
    USHORT AttentionButtonEnable:1;
    USHORT PowerFaultDetectEnable:1;
    USHORT MRLSensorEnable:1;
    USHORT PresenceDetectEnable:1;
    USHORT CommandCompletedEnable:1;
    USHORT HotPlugInterruptEnable:1;
    USHORT AttentionIndicatorControl:2;
    USHORT PowerIndicatorControl:2;
    USHORT PowerControllerControl:1;
    USHORT ElectromechanicalLockControl:1;
    USHORT DataLinkStateChangeEnable:1;
    USHORT Rsvd:3;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_SLOT_CONTROL_REGISTER, *PPCI_EXPRESS_SLOT_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_SLOT_STATUS_REGISTER {
  struct {
    USHORT AttentionButtonPressed:1;
    USHORT PowerFaultDetected:1;
    USHORT MRLSensorChanged:1;
    USHORT PresenceDetectChanged:1;
    USHORT CommandCompleted:1;
    USHORT MRLSensorState:1;
    USHORT PresenceDetectState:1;
    USHORT ElectromechanicalLockEngaged:1;
    USHORT DataLinkStateChanged:1;
    USHORT Rsvd:7;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_SLOT_STATUS_REGISTER, *PPCI_EXPRESS_SLOT_STATUS_REGISTER;

typedef union _PCI_EXPRESS_ROOT_CONTROL_REGISTER {
  struct {
    USHORT CorrectableSerrEnable:1;
    USHORT NonFatalSerrEnable:1;
    USHORT FatalSerrEnable:1;
    USHORT PMEInterruptEnable:1;
    USHORT CRSSoftwareVisibilityEnable:1;
    USHORT Rsvd:11;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_ROOT_CONTROL_REGISTER, *PPCI_EXPRESS_ROOT_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER {
  struct {
    USHORT CRSSoftwareVisibility:1;
    USHORT Rsvd:15;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_ROOT_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_ROOT_STATUS_REGISTER {
  struct {
    ULONG PMERequestorId:16;
    ULONG PMEStatus:1;
    ULONG PMEPending:1;
    ULONG Rsvd:14;
  } DUMMYSTRUCTNAME;
  ULONG AsULONG;
} PCI_EXPRESS_ROOT_STATUS_REGISTER, *PPCI_EXPRESS_ROOT_STATUS_REGISTER;

typedef struct _PCI_EXPRESS_CAPABILITY {
  PCI_CAPABILITIES_HEADER Header;
  PCI_EXPRESS_CAPABILITIES_REGISTER ExpressCapabilities;
  PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER DeviceCapabilities;
  PCI_EXPRESS_DEVICE_CONTROL_REGISTER DeviceControl;
  PCI_EXPRESS_DEVICE_STATUS_REGISTER DeviceStatus;
  PCI_EXPRESS_LINK_CAPABILITIES_REGISTER LinkCapabilities;
  PCI_EXPRESS_LINK_CONTROL_REGISTER LinkControl;
  PCI_EXPRESS_LINK_STATUS_REGISTER LinkStatus;
  PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER SlotCapabilities;
  PCI_EXPRESS_SLOT_CONTROL_REGISTER SlotControl;
  PCI_EXPRESS_SLOT_STATUS_REGISTER SlotStatus;
  PCI_EXPRESS_ROOT_CONTROL_REGISTER RootControl;
  PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER RootCapabilities;
  PCI_EXPRESS_ROOT_STATUS_REGISTER RootStatus;
} PCI_EXPRESS_CAPABILITY, *PPCI_EXPRESS_CAPABILITY;

typedef enum {
  MRLClosed = 0,
  MRLOpen
} PCI_EXPRESS_MRL_STATE;

typedef enum {
  SlotEmpty = 0,
  CardPresent
} PCI_EXPRESS_CARD_PRESENCE;

typedef enum {
  IndicatorOn = 1,
  IndicatorBlink,
  IndicatorOff
} PCI_EXPRESS_INDICATOR_STATE;

typedef enum {
  PowerOn = 0,
  PowerOff
} PCI_EXPRESS_POWER_STATE;

typedef enum {
  L0sEntrySupport = 1,
  L0sAndL1EntrySupport = 3
} PCI_EXPRESS_ASPM_SUPPORT;

typedef enum {
  L0sAndL1EntryDisabled,
  L0sEntryEnabled,
  L1EntryEnabled,
  L0sAndL1EntryEnabled
} PCI_EXPRESS_ASPM_CONTROL;

typedef enum {
  L0s_Below64ns = 0,
  L0s_64ns_128ns,
  L0s_128ns_256ns,
  L0s_256ns_512ns,
  L0s_512ns_1us,
  L0s_1us_2us,
  L0s_2us_4us,
  L0s_Above4us
} PCI_EXPRESS_L0s_EXIT_LATENCY;

typedef enum {
  L1_Below1us = 0,
  L1_1us_2us,
  L1_2us_4us,
  L1_4us_8us,
  L1_8us_16us,
  L1_16us_32us,
  L1_32us_64us,
  L1_Above64us
} PCI_EXPRESS_L1_EXIT_LATENCY;

typedef enum {
  PciExpressEndpoint = 0,
  PciExpressLegacyEndpoint,
  PciExpressRootPort = 4,
  PciExpressUpstreamSwitchPort,
  PciExpressDownstreamSwitchPort,
  PciExpressToPciXBridge,
  PciXToExpressBridge,
  PciExpressRootComplexIntegratedEndpoint,
  PciExpressRootComplexEventCollector
} PCI_EXPRESS_DEVICE_TYPE;

typedef enum {
  MaxPayload128Bytes = 0,
  MaxPayload256Bytes,
  MaxPayload512Bytes,
  MaxPayload1024Bytes,
  MaxPayload2048Bytes,
  MaxPayload4096Bytes
} PCI_EXPRESS_MAX_PAYLOAD_SIZE;

typedef union _PCI_EXPRESS_PME_REQUESTOR_ID {
  struct {
    USHORT FunctionNumber:3;
    USHORT DeviceNumber:5;
    USHORT BusNumber:8;
  } DUMMYSTRUCTNAME;
  USHORT AsUSHORT;
} PCI_EXPRESS_PME_REQUESTOR_ID, *PPCI_EXPRESS_PME_REQUESTOR_ID;

#if defined(_WIN64)

#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif /* defined(_WIN64) */

typedef enum _PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE {
  ResourceTypeSingle = 0,
  ResourceTypeRange,
  ResourceTypeExtendedCounterConfiguration,
  ResourceTypeOverflow,
  ResourceTypeMax
} PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE;

typedef struct _PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR {
  PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE Type;
  ULONG Flags;
  union {
    ULONG CounterIndex;
    ULONG ExtendedRegisterAddress;
    struct {
      ULONG Begin;
      ULONG End;
    } Range;
  } u;
} PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR, *PPHYSICAL_COUNTER_RESOURCE_DESCRIPTOR;

typedef struct _PHYSICAL_COUNTER_RESOURCE_LIST {
  ULONG Count;
  PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR Descriptors[ANYSIZE_ARRAY];
} PHYSICAL_COUNTER_RESOURCE_LIST, *PPHYSICAL_COUNTER_RESOURCE_LIST;

typedef VOID
(NTAPI *PciPin2Line)(
  IN struct _BUS_HANDLER *BusHandler,
  IN struct _BUS_HANDLER *RootHandler,
  IN PCI_SLOT_NUMBER SlotNumber,
  IN PPCI_COMMON_CONFIG PciData);

typedef VOID
(NTAPI *PciLine2Pin)(
  IN struct _BUS_HANDLER *BusHandler,
  IN struct _BUS_HANDLER *RootHandler,
  IN PCI_SLOT_NUMBER SlotNumber,
  IN PPCI_COMMON_CONFIG PciNewData,
  IN PPCI_COMMON_CONFIG PciOldData);

typedef VOID
(NTAPI *PciReadWriteConfig)(
  IN struct _BUS_HANDLER *BusHandler,
  IN PCI_SLOT_NUMBER Slot,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

#define PCI_DATA_TAG ' ICP'
#define PCI_DATA_VERSION 1

typedef struct _PCIBUSDATA {
  ULONG Tag;
  ULONG Version;
  PciReadWriteConfig ReadConfig;
  PciReadWriteConfig WriteConfig;
  PciPin2Line Pin2Line;
  PciLine2Pin Line2Pin;
  PCI_SLOT_NUMBER ParentSlot;
  PVOID Reserved[4];
} PCIBUSDATA, *PPCIBUSDATA;

#ifndef _PCIINTRF_X_
#define _PCIINTRF_X_

typedef ULONG
(NTAPI *PCI_READ_WRITE_CONFIG)(
  IN PVOID Context,
  IN ULONG BusOffset,
  IN ULONG Slot,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef VOID
(NTAPI *PCI_PIN_TO_LINE)(
  IN PVOID Context,
  IN PPCI_COMMON_CONFIG PciData);

typedef VOID
(NTAPI *PCI_LINE_TO_PIN)(
  IN PVOID Context,
  IN PPCI_COMMON_CONFIG PciNewData,
  IN PPCI_COMMON_CONFIG PciOldData);

typedef VOID
(NTAPI *PCI_ROOT_BUS_CAPABILITY)(
  IN PVOID Context,
  OUT PPCI_ROOT_BUS_HARDWARE_CAPABILITY HardwareCapability);

typedef VOID
(NTAPI *PCI_EXPRESS_WAKE_CONTROL)(
  IN PVOID Context,
  IN BOOLEAN EnableWake);

typedef struct _PCI_BUS_INTERFACE_STANDARD {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PCI_READ_WRITE_CONFIG ReadConfig;
  PCI_READ_WRITE_CONFIG WriteConfig;
  PCI_PIN_TO_LINE PinToLine;
  PCI_LINE_TO_PIN LineToPin;
  PCI_ROOT_BUS_CAPABILITY RootBusCapability;
  PCI_EXPRESS_WAKE_CONTROL ExpressWakeControl;
} PCI_BUS_INTERFACE_STANDARD, *PPCI_BUS_INTERFACE_STANDARD;

#define PCI_BUS_INTERFACE_STANDARD_VERSION 1

#endif /* _PCIINTRF_X_ */

#if (NTDDI_VERSION >= NTDDI_WIN7)

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX     0x00004000
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX    0x00008000
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX \
    (FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX | \
     FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX)

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_DEPRECATED 0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_DEPRECATED 0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_DEPRECATED 0x00000300

#else

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL     0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL    0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK        0x00000300

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#define FILE_CHARACTERISTICS_PROPAGATED (   FILE_REMOVABLE_MEDIA   | \
                                            FILE_READ_ONLY_DEVICE  | \
                                            FILE_FLOPPY_DISKETTE   | \
                                            FILE_WRITE_ONCE_MEDIA  | \
                                            FILE_DEVICE_SECURE_OPEN  )

typedef struct _FILE_ALIGNMENT_INFORMATION {
  ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;


typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
  ULONG FileAttributes;
  ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
  BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
  LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
  LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef struct _FILE_FS_LABEL_INFORMATION {
  ULONG VolumeLabelLength;
  WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
  LARGE_INTEGER VolumeCreationTime;
  ULONG VolumeSerialNumber;
  ULONG VolumeLabelLength;
  BOOLEAN SupportsObjects;
  WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
  LARGE_INTEGER TotalAllocationUnits;
  LARGE_INTEGER AvailableAllocationUnits;
  ULONG SectorsPerAllocationUnit;
  ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
  LARGE_INTEGER TotalAllocationUnits;
  LARGE_INTEGER CallerAvailableAllocationUnits;
  LARGE_INTEGER ActualAvailableAllocationUnits;
  ULONG SectorsPerAllocationUnit;
  ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECTID_INFORMATION {
  UCHAR ObjectId[16];
  UCHAR ExtendedInfo[48];
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

typedef union _FILE_SEGMENT_ELEMENT {
  PVOID64 Buffer;
  ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

#define IOCTL_AVIO_ALLOCATE_STREAM      CTL_CODE(FILE_DEVICE_AVIO, 1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_FREE_STREAM          CTL_CODE(FILE_DEVICE_AVIO, 2, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_MODIFY_STREAM        CTL_CODE(FILE_DEVICE_AVIO, 3, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

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
$endif

