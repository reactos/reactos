/* $Id: iotypes.h,v 1.63 2004/06/23 21:43:19 ion Exp $
 *
 */

#ifndef __INCLUDE_DDK_IOTYPES_H
#define __INCLUDE_DDK_IOTYPES_H

#include <ntos/obtypes.h>
#include <ntos/disk.h>
#include <ntos/file.h>

#include <pshpack4.h>

/* from winddk.h */
#define POINTER_ALIGNMENT


/*
 * These are referenced before they can be fully defined
 */
struct _DRIVER_OBJECT;
struct _FILE_OBJECT;
struct _DEVICE_OBJECT;
struct _IRP;
struct _IO_STATUS_BLOCK;
struct _SCSI_REQUEST_BLOCK;
struct _IO_TIMER;

/* SIMPLE TYPES *************************************************************/

enum
{
   DeallocateObject,
   KeepObject,
   DeallocateObjectKeepRegisters
};


typedef enum _CREATE_FILE_TYPE
{
   CreateFileTypeNone,
   CreateFileTypeNamedPipe,
   CreateFileTypeMailslot
} CREATE_FILE_TYPE;


typedef struct _SHARE_ACCESS
{
   ULONG OpenCount;
   ULONG Readers;
   ULONG Writers;
   ULONG Deleters;
   ULONG SharedRead;
   ULONG SharedWrite;
   ULONG SharedDelete;
} SHARE_ACCESS, *PSHARE_ACCESS;

/* FUNCTION TYPES ************************************************************/

typedef VOID STDCALL_FUNC
(*PDRIVER_REINITIALIZE)(struct _DRIVER_OBJECT* DriverObject,
			PVOID Context,
			ULONG Count);

typedef NTSTATUS STDCALL_FUNC
(*PIO_QUERY_DEVICE_ROUTINE)(PVOID Context,
			    PUNICODE_STRING Pathname,
			    INTERFACE_TYPE BusType,
			    ULONG BusNumber,
			    PKEY_VALUE_FULL_INFORMATION* BI,
			    CONFIGURATION_TYPE ControllerType,
			    ULONG ControllerNumber,
			    PKEY_VALUE_FULL_INFORMATION* CI,
			    CONFIGURATION_TYPE PeripheralType,
			    ULONG PeripheralNumber,
			    PKEY_VALUE_FULL_INFORMATION* PI);

typedef NTSTATUS STDCALL_FUNC
(*PIO_COMPLETION_ROUTINE)(struct _DEVICE_OBJECT* DeviceObject,
			  struct _IRP* Irp,
			  PVOID Context);

typedef VOID STDCALL_FUNC
(*PIO_APC_ROUTINE)(PVOID ApcContext,
		   struct _IO_STATUS_BLOCK* IoStatusBlock,
		   ULONG Reserved);


/* STRUCTURE TYPES ***********************************************************/

typedef struct _ADAPTER_OBJECT ADAPTER_OBJECT, *PADAPTER_OBJECT;

typedef struct _IO_SECURITY_CONTEXT
{
   PSECURITY_QUALITY_OF_SERVICE SecurityQos;
   PACCESS_STATE AccessState;
   ACCESS_MASK DesiredAccess;
   ULONG FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;


typedef struct _IO_RESOURCE_DESCRIPTOR
{
   UCHAR Option;
   UCHAR Type;
   UCHAR ShareDisposition;
   
   /*
    * Reserved for system use
    */
   UCHAR Spare1;
   
   USHORT Flags;
   
   /*
    * Reserved for system use
    */
   UCHAR Spare2;
   
   union
     {
	struct
	  {
	     ULONG Length;
	     ULONG Alignment;
	     PHYSICAL_ADDRESS MinimumAddress;
	     PHYSICAL_ADDRESS MaximumAddress;
	  } Port;
	struct
	  {
	     ULONG Length;
	     ULONG Alignment;
	     PHYSICAL_ADDRESS MinimumAddress;
	     PHYSICAL_ADDRESS MaximumAddress;
	  } Memory;
	struct
	  { 
	     ULONG MinimumVector;
	     ULONG MaximumVector;
	  } Interrupt;
	struct
	  {
	     ULONG MinimumChannel;
	     ULONG MaximumChannel;
	  } Dma;
     } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

// IO_RESOURCE_DESCRIPTOR Options
#define IO_RESOURCE_REQUIRED    0x00
#define IO_RESOURCE_PREFERRED   0x01
#define IO_RESOURCE_DEFAULT     0x02
#define IO_RESOURCE_ALTERNATIVE 0x08

typedef struct _IO_RESOURCE_LIST
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST
{
   /*
    * List size in bytes
    */
   ULONG ListSize;
   
   /*
    * System defined enum for the bus
    */
   INTERFACE_TYPE InterfaceType;
   
   ULONG BusNumber;
   ULONG SlotNumber;
   ULONG Reserved[3];
   ULONG AlternativeLists;
   IO_RESOURCE_LIST List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;


/* MicroChannel bus data */

typedef struct _CM_MCA_POS_DATA
{
  USHORT AdapterId;
  UCHAR PosData1;
  UCHAR PosData2;
  UCHAR PosData3;
  UCHAR PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;


/* Int13 drive geometry data */

typedef struct _CM_INT13_DRIVE_PARAMETER
{
  USHORT DriveSelect;
  ULONG MaxCylinders;
  USHORT SectorsPerTrack;
  USHORT MaxHeads;
  USHORT NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;


/* Extended drive geometry data */

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
  ULONG BytesPerSector;
  ULONG NumberOfCylinders;
  ULONG SectorsPerTrack;
  ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;


#include <pshpack1.h>

typedef struct
{
   UCHAR Type;
   UCHAR ShareDisposition;
   USHORT Flags;
   union
     {
	struct
	  {
	     PHYSICAL_ADDRESS Start;
	     ULONG Length;
	  } Port;
	struct
	  {
	     ULONG Level;
	     ULONG Vector;
	     ULONG Affinity;
	  } Interrupt;
	struct
	  {
	     PHYSICAL_ADDRESS Start;
	     ULONG Length;
	  } Memory;
	struct
	  {
	     ULONG Channel;
	     ULONG Port;
	     ULONG Reserved1;
	  } Dma;
	struct
	  {
	     ULONG DataSize;
	     ULONG Reserved1;
	     ULONG Reserved2;
	  } DeviceSpecificData;
     }  u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct
{
   INTERFACE_TYPE InterfaceType;
   ULONG BusNumber;
   CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct
{
   ULONG Count;
   CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

#include <poppack.h>


/*
 * PURPOSE: IRP stack location
 */

/*
typedef struct _IO_STACK_LOCATION
{
  UCHAR MajorFunction;
  UCHAR MinorFunction;
  UCHAR Flags;
  UCHAR Control;
  
  union
    {
      struct
	{
	  PIO_SECURITY_CONTEXT SecurityContext;
	  ULONG Options;
	  USHORT FileAttributes;
	  USHORT ShareAccess;
	  ULONG EaLength;
	} Create;
      struct
	{
	  ULONG Length;
	  ULONG Key;
	  LARGE_INTEGER ByteOffset;
	} Read;
      struct
	{
	  ULONG Length;
	  ULONG Key;
	  LARGE_INTEGER ByteOffset;
	} Write;
      struct
	{
	  ULONG OutputBufferLength;
	  ULONG InputBufferLength;
	  ULONG IoControlCode;
	  PVOID Type3InputBuffer;
	} DeviceIoControl;
      struct
	{
	  ULONG OutputBufferLength;
	  ULONG InputBufferLength;
	  ULONG IoControlCode;
	  PVOID Type3InputBuffer;
	} FileSystemControl;
      struct
	{
	  struct _VPB* Vpb;
	  struct _DEVICE_OBJECT* DeviceObject;
	} MountVolume;
      struct
	{
	  struct _VPB* Vpb;
	  struct _DEVICE_OBJECT* DeviceObject;
	} VerifyVolume;
      struct
	{
	  ULONG Length;
	  FILE_INFORMATION_CLASS FileInformationClass;
	} QueryFile;
      struct
	{
	  ULONG Length;
	  FS_INFORMATION_CLASS FsInformationClass;
	} QueryVolume;
      struct
	{
	  ULONG Length;
	  FS_INFORMATION_CLASS FsInformationClass;
	} SetVolume;
      struct
	{
	  ULONG Length;
	  FILE_INFORMATION_CLASS FileInformationClass;
	  struct _FILE_OBJECT* FileObject;
	  union
	    {
	      struct
		{
		  BOOLEAN ReplaceIfExists;
		  BOOLEAN AdvanceOnly;
		} d;
	      ULONG ClusterCount;
	      HANDLE DeleteHandle;
	    } u;
	} SetFile;
      struct
	{
	  ULONG Length;
	  PUNICODE_STRING FileName;
	  FILE_INFORMATION_CLASS FileInformationClass;
	  ULONG FileIndex;
	} QueryDirectory;

      // Parameters for IRP_MN_QUERY_DEVICE_RELATIONS
      struct
	{
	  DEVICE_RELATION_TYPE Type;
	} QueryDeviceRelations;

      // Parameters for IRP_MN_QUERY_INTERFACE
      struct
	{
	  CONST GUID *InterfaceType;
	  USHORT Size;
	  USHORT Version;
	  PINTERFACE Interface;
	  PVOID InterfaceSpecificData;
	} QueryInterface;

      // Parameters for IRP_MN_QUERY_CAPABILITIES
      struct
	{
	  PDEVICE_CAPABILITIES Capabilities;
	} DeviceCapabilities;

      // Parameters for IRP_MN_FILTER_RESOURCE_REQUIREMENTS
      struct
	{
      PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
    } FilterResourceRequirements;

      // Parameters for IRP_MN_QUERY_ID
      struct
	{
	  BUS_QUERY_ID_TYPE IdType;
	} QueryId;

      // Parameters for IRP_MN_QUERY_DEVICE_TEXT
      struct
	{
	  DEVICE_TEXT_TYPE DeviceTextType;
	  LCID LocaleId;
	} QueryDeviceText;

      // Parameters for IRP_MN_DEVICE_USAGE_NOTIFICATION
      struct
	{
	  BOOLEAN InPath;
	  BOOLEAN Reserved[3];
	  DEVICE_USAGE_NOTIFICATION_TYPE Type;
	} UsageNotification;

      // Parameters for IRP_MN_WAIT_WAKE
      struct
	{
	  SYSTEM_POWER_STATE PowerState;
	} WaitWake;

      // Parameter for IRP_MN_POWER_SEQUENCE
      struct
	{
	  PPOWER_SEQUENCE PowerSequence;
	} PowerSequence;

      // Parameters for IRP_MN_SET_POWER and IRP_MN_QUERY_POWER
      struct
	{
	  ULONG SystemContext;
	  POWER_STATE_TYPE Type;
	  POWER_STATE State;
	  POWER_ACTION ShutdownType;
	} Power;

      // Parameters for IRP_MN_START_DEVICE
      struct
	{
	  PCM_RESOURCE_LIST AllocatedResources;
	  PCM_RESOURCE_LIST AllocatedResourcesTranslated;
	} StartDevice;
*/
      /* Parameters for IRP_MN_SCSI_CLASS */
/*      struct
	{
	  struct _SCSI_REQUEST_BLOCK *Srb;
	} Scsi;

	  //byte range file locking
	  struct 
	{
      PLARGE_INTEGER Length;
      ULONG Key;
      LARGE_INTEGER ByteOffset;
    } LockControl;
*/
      /* Paramters for other calls */
/*      struct
	{
	  PVOID Argument1;
	  PVOID Argument2;
	  PVOID Argument3;
	  PVOID Argument4;
	} Others;
    } Parameters;
  
  struct _DEVICE_OBJECT* DeviceObject;
  struct _FILE_OBJECT* FileObject;

  PIO_COMPLETION_ROUTINE CompletionRoutine;
  PVOID CompletionContext;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;*/

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
      struct _FILE_OBJECT*  FileObject;
      union {
        struct {
          BOOLEAN  ReplaceIfExists;
          BOOLEAN  AdvanceOnly;
        };
        ULONG  ClusterCount;
        HANDLE  DeleteHandle;
      };
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
      struct _VPB*  Vpb;
      struct _DEVICE_OBJECT*  DeviceObject;
    } MountVolume;
    struct {
      struct _VPB*  Vpb;
      struct _DEVICE_OBJECT*  DeviceObject;
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
  struct _DEVICE_OBJECT*  DeviceObject;
  struct _FILE_OBJECT*  FileObject;
  PIO_COMPLETION_ROUTINE  CompletionRoutine;
  PVOID  Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

// AG: The commented lines in this structure are due to PMAILSLOT_CREATE_PARAMETERS
// and PNAMED_PIPE_CREATE_PARAMETERS and PFILE_GET_QUOTA_INFORMATION not being
// defined.

typedef struct _EXTENDED_IO_STACK_LOCATION {

    /* Included for padding */
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    union {

//       struct {
//          PIO_SECURITY_CONTEXT              SecurityContext;
//          ULONG                             Options;
//          USHORT                            Reserved;
//          USHORT                            ShareAccess;
//          PMAILSLOT_CREATE_PARAMETERS       Parameters;
//       } CreateMailslot;

//        struct {
//            PIO_SECURITY_CONTEXT            SecurityContext;
//            ULONG                           Options;
//            USHORT                          Reserved;
//            USHORT                          ShareAccess;
//            PNAMED_PIPE_CREATE_PARAMETERS   Parameters;
//        } CreatePipe;

        struct {
            ULONG                           OutputBufferLength;
            ULONG                           InputBufferLength;
            ULONG                           FsControlCode;
            PVOID                           Type3InputBuffer;
        } FileSystemControl;

        struct {
            PLARGE_INTEGER                  Length;
            ULONG                           Key;
            LARGE_INTEGER                   ByteOffset;
        } LockControl;

        struct {
            ULONG                           Length;
            ULONG                           CompletionFilter;
        } NotifyDirectory;

        struct {
            ULONG                           Length;
            PUNICODE_STRING                 FileName;
            FILE_INFORMATION_CLASS          FileInformationClass;
            ULONG                           FileIndex;
        } QueryDirectory;

        struct {
            ULONG                           Length;
            PVOID                           EaList;
            ULONG                           EaListLength;
            ULONG                           EaIndex;
        } QueryEa;

//        struct {
//            ULONG                           Length;
//            PSID                            StartSid;
//            PFILE_GET_QUOTA_INFORMATION     SidList;
//            ULONG                           SidListLength;
//        } QueryQuota;

        struct {
            ULONG                           Length;
        } SetEa;

        struct {
            ULONG                           Length;
        } SetQuota;

        struct {
            ULONG                           Length;
            FS_INFORMATION_CLASS            FsInformationClass;
        } SetVolume;

    } Parameters;
    struct _DEVICE_OBJECT*  DeviceObject;
    struct _FILE_OBJECT*  FileObject;
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID  Context;

} EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;

typedef struct _IO_STATUS_BLOCK
{
  NTSTATUS Status;
  ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/*
 * Driver entry point declaration
 */
typedef NTSTATUS STDCALL_FUNC
(*PDRIVER_INITIALIZE)(struct _DRIVER_OBJECT* DriverObject,
		      PUNICODE_STRING RegistryPath);

/*
 * Driver cancel declaration
 */
typedef NTSTATUS STDCALL_FUNC
(*PDRIVER_CANCEL)(struct _DEVICE_OBJECT* DeviceObject,
		  struct _IRP* RegistryPath);


typedef struct _SECTION_OBJECT_POINTERS
{
   PVOID DataSectionObject;
   PVOID SharedCacheMap;
   PVOID ImageSectionObject;
} SECTION_OBJECT_POINTERS, *PSECTION_OBJECT_POINTERS;

typedef struct _IO_COMPLETION_CONTEXT
{
   PVOID Port;
   ULONG Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000

/*
 * ReactOS specific flags
 */
#define FO_DIRECT_CACHE_READ            0x72000001
#define FO_DIRECT_CACHE_WRITE           0x72000002
#define FO_DIRECT_CACHE_PAGING_READ     0x72000004
#define FO_DIRECT_CACHE_PAGING_WRITE    0x72000008
#define FO_FCB_IS_VALID                 0x72000010

typedef struct _FILE_OBJECT
{
   CSHORT Type;
   CSHORT Size;
   struct _DEVICE_OBJECT* DeviceObject;
   struct _VPB* Vpb;
   PVOID FsContext;
   PVOID FsContext2;
   PSECTION_OBJECT_POINTERS SectionObjectPointer;
   PVOID PrivateCacheMap;
   NTSTATUS FinalStatus;
   struct _FILE_OBJECT* RelatedFileObject;
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
   ULONG Waiters;
   ULONG Busy;
   PVOID LastLock;
   KEVENT Lock;
   KEVENT Event;
   PIO_COMPLETION_CONTEXT CompletionContext;
} FILE_OBJECT, *PFILE_OBJECT;


typedef struct _IRP
{
   CSHORT Type;
   USHORT Size;
   PMDL MdlAddress;
   ULONG Flags;
   union
     {
	struct _IRP* MasterIrp;
	LONG IrpCount;
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
   CCHAR ApcEnvironment;// CCHAR or PVOID?
   UCHAR AllocationFlags;//UCHAR or ULONG?
   PIO_STATUS_BLOCK UserIosb;
   PKEVENT UserEvent;
   union
     {
	struct
	  {
	     PIO_APC_ROUTINE UserApcRoutine;
	     PVOID UserApcContext;
	  } AsynchronousParameters;
	LARGE_INTEGER AllocationSize;
     } Overlay;
   PDRIVER_CANCEL CancelRoutine;
   PVOID UserBuffer;
   union
     {
	struct
	  {
	     union {
	       KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
	       PVOID DriverContext[4];
	     };
	     struct _ETHREAD* Thread;
	     PCHAR AuxiliaryBuffer;
	     LIST_ENTRY ListEntry;
	     struct _IO_STACK_LOCATION* CurrentStackLocation;
	     PFILE_OBJECT OriginalFileObject;
	  } Overlay;
	KAPC Apc;
	ULONG CompletionKey;
     } Tail;
   IO_STACK_LOCATION Stack[1];
} IRP, *PIRP;

#define VPB_MOUNTED                     0x00000001
#define VPB_LOCKED                      0x00000002
#define VPB_PERSISTENT                  0x00000004
#define VPB_REMOVE_PENDING              0x00000008

typedef struct _VPB
{
   CSHORT Type;
   CSHORT Size;
   USHORT Flags;
   USHORT VolumeLabelLength;
   struct _DEVICE_OBJECT* DeviceObject;
   struct _DEVICE_OBJECT* RealDevice;
   ULONG SerialNumber;
   ULONG ReferenceCount;
   WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH];
} VPB, *PVPB;


typedef struct _DEVICE_OBJECT
{
   CSHORT Type;
   CSHORT Size;
   LONG ReferenceCount;
   struct _DRIVER_OBJECT* DriverObject;
   struct _DEVICE_OBJECT* NextDevice;
   struct _DEVICE_OBJECT* AttachedDevice;
   struct _IRP* CurrentIrp;
   struct _IO_TIMER *Timer;
   ULONG Flags;
   ULONG Characteristics;
   PVPB Vpb;
   PVOID DeviceExtension;
   DEVICE_TYPE DeviceType;
   CCHAR StackSize;
   union
     {
	LIST_ENTRY ListHead;
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
   struct _DEVOBJ_EXTENSION* DeviceObjectExtension;
   PVOID Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;


/*
 * Fast i/o routine type declaration
 */
//typedef NTSTATUS (*PFAST_IO_DISPATCH)(struct _DEVICE_OBJECT*, IRP*);
//FIXME : this type is ok for read and write, but not for all routines
typedef BOOLEAN STDCALL_FUNC
(*PFAST_IO_ROUTINE)(IN struct _FILE_OBJECT *FileObject,
		    IN PLARGE_INTEGER FileOffset,
		    IN ULONG Length,
		    IN BOOLEAN Wait,
		    IN ULONG LockKey,
		    OUT PVOID Buffer,
		    OUT PIO_STATUS_BLOCK IoStatus,
		    IN struct _DEVICE_OBJECT *DeviceObject);

typedef struct _FAST_IO_DISPATCH {
   ULONG SizeOfFastIoDispatch;
   PFAST_IO_ROUTINE FastIoCheckIfPossible;
   PFAST_IO_ROUTINE FastIoRead;
   PFAST_IO_ROUTINE FastIoWrite;
   PFAST_IO_ROUTINE FastIoQueryBasicInfo;
   PFAST_IO_ROUTINE FastIoQueryStandardInfo;
   PFAST_IO_ROUTINE FastIoLock;
   PFAST_IO_ROUTINE FastIoUnlockSingle;
   PFAST_IO_ROUTINE FastIoUnlockAll;
   PFAST_IO_ROUTINE FastIoUnlockAllByKey;
   PFAST_IO_ROUTINE FastIoDeviceControl;
   PFAST_IO_ROUTINE AcquireFileForNtCreateSection;
   PFAST_IO_ROUTINE ReleaseFileForNtCreateSection;
   PFAST_IO_ROUTINE FastIoDetachDevice;
   PFAST_IO_ROUTINE FastIoQueryNetworkOpenInfo;
   PFAST_IO_ROUTINE AcquireForModWrite;
   PFAST_IO_ROUTINE MdlRead;
   PFAST_IO_ROUTINE MdlReadComplete;
   PFAST_IO_ROUTINE PrepareMdlWrite;
   PFAST_IO_ROUTINE MdlWriteComplete;
   PFAST_IO_ROUTINE FastIoReadCompressed;
   PFAST_IO_ROUTINE FastIoWriteCompressed;
   PFAST_IO_ROUTINE MdlReadCompleteCompressed;
   PFAST_IO_ROUTINE MdlWriteCompleteCompressed;
   PFAST_IO_ROUTINE FastIoQueryOpen;
   PFAST_IO_ROUTINE ReleaseForModWrite;
   PFAST_IO_ROUTINE AcquireForCcFlush;
   PFAST_IO_ROUTINE ReleaseForCcFlush;
} FAST_IO_DISPATCH, *PFAST_IO_DISPATCH;

/*
 * Dispatch routine type declaration
 */
typedef NTSTATUS STDCALL_FUNC
(*PDRIVER_DISPATCH)(IN struct _DEVICE_OBJECT *DeviceObject,
		   IN struct _IRP *Irp);

/*
 * StartIo routine type declaration
 */
typedef VOID STDCALL_FUNC
(*PDRIVER_STARTIO)(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

/*
 * Unload routine type declaration
 */
typedef VOID STDCALL_FUNC
(*PDRIVER_UNLOAD)(IN struct _DRIVER_OBJECT *DriverObject);

/*
 * AddDevice routine type declaration
 */
typedef NTSTATUS STDCALL_FUNC
(*PDRIVER_ADD_DEVICE)(IN struct _DRIVER_OBJECT *DriverObject,
		      IN struct _DEVICE_OBJECT *PhysicalDeviceObject);


typedef struct _DRIVER_EXTENSION
{
   struct _DRIVER_OBJECT* DriverObject;
   PDRIVER_ADD_DEVICE AddDevice;
   ULONG Count;
   UNICODE_STRING ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

#if 0
typedef
struct _FAST_IO_DISPATCH_TABLE
{
	ULONG			Count;
	PFAST_IO_DISPATCH	Dispatch;

} FAST_IO_DISPATCH_TABLE, * PFAST_IO_DISPATCH_TABLE;
#endif

typedef struct _DRIVER_OBJECT
{
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
#if 0
   PFAST_IO_DISPATCH_TABLE FastIoDispatch;
#else
   PFAST_IO_DISPATCH FastIoDispatch;
#endif
   PDRIVER_INITIALIZE DriverInit;
   PDRIVER_STARTIO DriverStartIo;
   PDRIVER_UNLOAD DriverUnload;
   PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;


typedef struct _CONFIGURATION_INFORMATION
{
   ULONG DiskCount;
   ULONG FloppyCount;
   ULONG CdRomCount;
   ULONG TapeCount;
   ULONG ScsiPortCount;
   ULONG SerialCount;
   ULONG ParallelCount;
   BOOLEAN AtDiskPrimaryAddressClaimed;
   BOOLEAN AtDiskSecondaryAddressClaimed;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef VOID STDCALL_FUNC
(*PIO_DPC_ROUTINE)(PKDPC Dpc,
		   PDEVICE_OBJECT DeviceObject,
		   PIRP Irp,
		   PVOID Context);

typedef VOID STDCALL_FUNC
(*PIO_TIMER_ROUTINE)(PDEVICE_OBJECT DeviceObject,
		     PVOID Context);

/*
 * PURPOSE: Special timer associated with each device
 * NOTES: This is a guess
 */
typedef struct _IO_TIMER
{
   KTIMER timer;
   KDPC dpc;
   PDEVICE_OBJECT DeviceObject;
   PIO_TIMER_ROUTINE TimerRoutine;
   PVOID Context;
} IO_TIMER, *PIO_TIMER;

typedef struct _IO_WORKITEM *PIO_WORKITEM;
typedef VOID (*PIO_WORKITEM_ROUTINE)(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context);

#if WINDOWS_STRUCTS_DOESNT_ALREADY_DEFINE_THIS
typedef struct _PARTITION_INFORMATION
{
   LARGE_INTEGER StartingOffset;
   LARGE_INTEGER PartitionLength;
   ULONG HiddenSectors;
   ULONG PartitionNumber;
   UCHAR PartitionType;
   BOOLEAN BootIndicator;
   BOOLEAN RecognizedPartition;
   BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;
#endif

typedef enum _PARTITION_STYLE {
    PARTITION_STYLE_MBR,
    PARTITION_STYLE_GPT,
    PARTITION_STYLE_RAW
} PARTITION_STYLE;

typedef struct _PARTITION_INFORMATION_GPT {
    GUID PartitionType;             
    GUID PartitionId;                 
    ULONG64 Attributes;                
    WCHAR Name [36];                 
} PARTITION_INFORMATION_GPT, *PPARTITION_INFORMATION_GPT;

typedef struct _PARTITION_INFORMATION_MBR {
    UCHAR PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    ULONG HiddenSectors;
} PARTITION_INFORMATION_MBR, *PPARTITION_INFORMATION_MBR;

typedef SET_PARTITION_INFORMATION SET_PARTITION_INFORMATION_MBR;
typedef PARTITION_INFORMATION_GPT SET_PARTITION_INFORMATION_GPT;

typedef struct _SET_PARTITION_INFORMATION_EX {
    PARTITION_STYLE PartitionStyle;
    union {
        SET_PARTITION_INFORMATION_MBR Mbr;
        SET_PARTITION_INFORMATION_GPT Gpt;
    };
} SET_PARTITION_INFORMATION_EX, *PSET_PARTITION_INFORMATION_EX;

typedef struct _CREATE_DISK_GPT {
    GUID DiskId;                    
    ULONG MaxPartitionCount;       
} CREATE_DISK_GPT, *PCREATE_DISK_GPT;

typedef struct _CREATE_DISK_MBR {
    ULONG Signature;
} CREATE_DISK_MBR, *PCREATE_DISK_MBR;


typedef struct _CREATE_DISK {
    PARTITION_STYLE PartitionStyle;
    union {
        CREATE_DISK_MBR Mbr;
        CREATE_DISK_GPT Gpt;
    };
} CREATE_DISK, *PCREATE_DISK;

typedef struct _DRIVE_LAYOUT_INFORMATION_GPT {
    GUID DiskId;
    LARGE_INTEGER StartingUsableOffset;
    LARGE_INTEGER UsableLength;
    ULONG MaxPartitionCount;
} DRIVE_LAYOUT_INFORMATION_GPT, *PDRIVE_LAYOUT_INFORMATION_GPT;

typedef struct _DRIVE_LAYOUT_INFORMATION_MBR {
    ULONG Signature;
} DRIVE_LAYOUT_INFORMATION_MBR, *PDRIVE_LAYOUT_INFORMATION_MBR;

typedef struct _PARTITION_INFORMATION_EX {
    PARTITION_STYLE PartitionStyle;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    ULONG PartitionNumber;
    BOOLEAN RewritePartition;
    union {
        PARTITION_INFORMATION_MBR Mbr;
        PARTITION_INFORMATION_GPT Gpt;
    };
} PARTITION_INFORMATION_EX, *PPARTITION_INFORMATION_EX;

typedef struct _DRIVE_LAYOUT_INFORMATION_EX {
    ULONG PartitionStyle;
    ULONG PartitionCount;
    union {
        DRIVE_LAYOUT_INFORMATION_MBR Mbr;
        DRIVE_LAYOUT_INFORMATION_GPT Gpt;
    };
    PARTITION_INFORMATION_EX PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION_EX, *PDRIVE_LAYOUT_INFORMATION_EX;

typedef struct _BOOTDISK_INFORMATION {
    LONGLONG BootPartitionOffset;
    LONGLONG SystemPartitionOffset;
    ULONG BootDeviceSignature;
    ULONG SystemDeviceSignature;
} BOOTDISK_INFORMATION, *PBOOTDISK_INFORMATION;


typedef struct _DISK_SIGNATURE {
    ULONG PartitionStyle;
    union {
        struct {
            ULONG Signature;
            ULONG CheckSum;
        } Mbr;

        struct {
            GUID DiskId;
        } Gpt;
    };
} DISK_SIGNATURE, *PDISK_SIGNATURE;

typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

typedef struct _DRIVER_LAYOUT_INFORMATION
{
   ULONG PartitionCount;
   ULONG Signature;
   PARTITION_INFORMATION PartitionEntry[1];
} DRIVER_LAYOUT_INFORMATION, *PDRIVER_LAYOUT_INFORMATION;


typedef IO_ALLOCATION_ACTION STDCALL_FUNC
(*PDRIVER_CONTROL)(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp,
		   PVOID MapRegisterBase,
		   PVOID Context);
#if (_WIN32_WINNT >= 0x0400)
typedef VOID STDCALL_FUNC
(*PFSDNOTIFICATIONPROC)(IN PDEVICE_OBJECT PtrTargetFileSystemDeviceObject,
			IN BOOLEAN DriverActive);
#endif // (_WIN32_WINNT >= 0x0400)


typedef struct _IO_PIPE_CREATE_BUFFER
{
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   ULONG MaxInstances;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   LARGE_INTEGER TimeOut;
} IO_PIPE_CREATE_BUFFER, *PIO_PIPE_CREATE_BUFFER;

typedef struct _IO_MAILSLOT_CREATE_BUFFER
{
   ULONG Param; /* ?? */
   ULONG MaxMessageSize;
   LARGE_INTEGER TimeOut;
} IO_MAILSLOT_CREATE_BUFFER, *PIO_MAILSLOT_CREATE_BUFFER;

/* error logging */

typedef struct _IO_ERROR_LOG_PACKET
{
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

typedef struct _IO_ERROR_LOG_MESSAGE
{
  USHORT Type;
  USHORT Size;
  USHORT DriverNameLength;
  LARGE_INTEGER TimeStamp;
  ULONG DriverNameOffset;
  IO_ERROR_LOG_PACKET EntryData;
} IO_ERROR_LOG_MESSAGE, *PIO_ERROR_LOG_MESSAGE;


/* DMA types */

typedef struct _SCATTER_GATHER_ELEMENT {
  PHYSICAL_ADDRESS Address;
  ULONG Length;
  ULONG_PTR Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

typedef struct _SCATTER_GATHER_LIST {
  ULONG NumberOfElements;
  ULONG_PTR Reserved;
  SCATTER_GATHER_ELEMENT Elements[];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

typedef struct _DMA_OPERATIONS *PDMA_OPERATIONS;

typedef struct _DMA_ADAPTER {
  USHORT Version;
  USHORT Size;
  PDMA_OPERATIONS DmaOperations;
  PADAPTER_OBJECT HalAdapter;
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID (*PPUT_DMA_ADAPTER)(
  PDMA_ADAPTER DmaAdapter
  );

typedef PVOID (*PALLOCATE_COMMON_BUFFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled
  );

typedef VOID (*PFREE_COMMON_BUFFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled
  );

typedef NTSTATUS (*PALLOCATE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG NumberOfMapRegisters,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context
  );

typedef BOOLEAN (*PFLUSH_ADAPTER_BUFFERS)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice
  );

typedef VOID (*PFREE_ADAPTER_CHANNEL)(
  IN PDMA_ADAPTER DmaAdapter
  );

typedef VOID (*PFREE_MAP_REGISTERS)(
  IN PDMA_ADAPTER DmaAdapter,
  PVOID MapRegisterBase,
  ULONG NumberOfMapRegisters
  );

typedef PHYSICAL_ADDRESS (*PMAP_TRANSFER)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice
  );

typedef ULONG (*PGET_DMA_ALIGNMENT)(
  IN PDMA_ADAPTER DmaAdapter
  );

typedef ULONG (*PREAD_DMA_COUNTER)(
  IN PDMA_ADAPTER DmaAdapter
  );

typedef VOID (*PDRIVER_LIST_CONTROL)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN struct _IRP *Irp,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN PVOID Context
  );

typedef NTSTATUS (*PGET_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN PMDL Mdl,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PDRIVER_LIST_CONTROL ExecutionRoutine,
  IN PVOID Context,
  IN BOOLEAN WriteToDevice
  );

typedef VOID (*PPUT_SCATTER_GATHER_LIST)(
  IN PDMA_ADAPTER DmaAdapter,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN BOOLEAN WriteToDevice
  );

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
} DMA_OPERATIONS;


/* Standard bus interface */

struct _DEVICE_DESCRIPTION;

typedef BOOLEAN STDCALL_FUNC (*PTRANSLATE_BUS_ADDRESS)(
  IN PVOID Context,
  IN PHYSICAL_ADDRESS BusAddress,
  IN ULONG Length,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress);

typedef PDMA_ADAPTER STDCALL_FUNC (*PGET_DMA_ADAPTER)(
  IN PVOID Context,
  IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
  OUT PULONG NumberOfMapRegisters);

typedef ULONG STDCALL_FUNC (*PGET_SET_DEVICE_DATA)(
  IN PVOID Context,
  IN ULONG DataType,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

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

#include <poppack.h>

#endif /* __INCLUDE_DDK_IOTYPES_H */
