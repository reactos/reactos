/*
 * 
 */

#ifndef __INCLUDE_DDK_IOTYPES_H
#define __INCLUDE_DDK_IOTYPES_H

/*
 * These are referenced before they can be fully defined
 */
struct _DRIVER_OBJECT;
struct _FILE_OBJECT;
struct _DEVICE_OBJECT;
struct _IRP;
struct _IO_STATUS_BLOCK;

/* SIMPLE TYPES *************************************************************/

enum
{
   DeallocateObject,
   KeepObject,
};

typedef ULONG INTERFACE_TYPE;
typedef INTERFACE_TYPE* PINTERFACE_TYPE;

/*
 * FIXME: Definition needed
 */
typedef struct _SHARE_ACCESS
{
} SHARE_ACCESS, *PSHARE_ACCESS;

/* FUNCTION TYPES ************************************************************/

typedef VOID (*PDRIVER_REINITIALIZE)(struct _DRIVER_OBJECT* DriverObject,
				     PVOID Context,
				     ULONG Count);

typedef NTSTATUS (*PIO_QUERY_DEVICE_ROUTINE)(PVOID Context,
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

typedef NTSTATUS (*PIO_COMPLETION_ROUTINE)(struct _DEVICE_OBJECT* DeviceObject,
					   struct _IRP* Irp,
					   PVOID Context);

typedef VOID (*PIO_APC_ROUTINE) (PVOID ApcContext,
				 struct _IO_STATUS_BLOCK* IoStatusBlock,
				 ULONG Reserved);


/* STRUCTURE TYPES ***********************************************************/

/*
 * PURPOSE: Special timer associated with each device
 */
typedef struct _IO_TIMER
{
   KTIMER timer;
   KDPC dpc;
} IO_TIMER, *PIO_TIMER;

/*
 * PURPOSE: IRP stack location
 */
typedef struct _IO_STACK_LOCATION
{
   /*
    * Type of request
    */
   UCHAR MajorFunction;
   
   /*
    * Further information about request type
    */
   UCHAR MinorFunction;
   
   /*
    *
    */
   UCHAR Flags;
   
   /*
    * FUNCTION: Various flags including for the io completion routine
    */
   UCHAR Control;
   
   /*
    * Parameters for request
    */
   union
     {
	struct
	  {
	     /*
	      * Number of bytes to be transferrred
	      */
	     ULONG Length;
	     
	     /*
	      * Possibly used to sort incoming request (to be documented)
	      */
	     ULONG Key;
	     
	     /*
	      * Optional starting offset for read
	      */
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
	
     } Parameters;
   
   /*
    * PURPOSE: Completion routine
    * NOTE: If this is the nth stack location (where the 1st is passed to the
    * highest level driver) then this is the completion routine set by
    * the (n-1)th driver
    */
   PIO_COMPLETION_ROUTINE CompletionRoutine;
   PVOID CompletionContext;
   
   /*
    * Driver created device object representing the target device
    */
   struct _DEVICE_OBJECT* DeviceObject;
   
   /*
    * File object (if any) associated with DeviceObject
    */
   struct _FILE_OBJECT* FileObject;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct _IO_STATUS_BLOCK
{
   /*
    * Is the completion status
    */
   NTSTATUS Status;
   
   /*
    * Is a request dependant value
    */
   ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/*
 * Driver entry point declaration
 */
typedef NTSTATUS (*PDRIVER_INITIALIZE)(struct _DRIVER_OBJECT* DriverObject,
				       PUNICODE_STRING RegistryPath);

/*
 * Driver cancel declaration
 */
typedef NTSTATUS (*PDRIVER_CANCEL)(struct _DRIVER_OBJECT* DriverObject,
				   PUNICODE_STRING RegistryPath);


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

typedef struct _FILE_OBJECT
{
   CSHORT Type;
   CSHORT Size;
   struct _DEVICE_OBJECT* DeviceObject;
   struct _VPB* Vpb;   
   PVOID FsContext;
   PVOID FsContext2;
   PSECTION_OBJECT_POINTERS SectionObjectPointers;
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
   BOOLEAN Cancel;
   KIRQL CancelIrql;
   PDRIVER_CANCEL CancelRoutine;
   PVOID UserBuffer;
   PVOID ApcEnvironment;
   ULONG AllocationFlags;
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
   union
     {
	struct
	  {
	     KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
	     PETHREAD Thread;
	     PCHAR AuxiliaryBuffer;
	     LIST_ENTRY ListEntry;
	     struct _IO_STACK_LOCATION* CurrentStackLocation;
	     PFILE_OBJECT OriginalFileObject;
	  } Overlay;	  
	KAPC Apc;
	ULONG CompletionKey;
     } Tail;
   ULONG CurrentLocation;
   IO_STACK_LOCATION Stack[1];
} IRP, *PIRP;


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
   WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
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
   PIO_TIMER Timer;
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
 * Dispatch routine type declaration
 */
typedef NTSTATUS (*PDRIVER_DISPATCH)(struct _DEVICE_OBJECT*, IRP*);

/*
 * Fast i/o routine type declaration
 */
typedef NTSTATUS (*PFAST_IO_DISPATCH)(struct _DEVICE_OBJECT*, IRP*);

/*
 * Dispatch routine type declaration
 */
typedef VOID (*PDRIVER_STARTIO)(struct _DEVICE_OBJECT*, IRP*);

/*
 * Dispatch routine type declaration
 */
typedef NTSTATUS (*PDRIVER_UNLOAD)(struct _DRIVER_OBJECT*);

typedef struct _DRIVER_EXTENSION
{
   struct _DRIVER_OBJECT* DriverObject;
   PDRIVER_ADD_DEVICE AddDevice;
   ULONG Count;
   UNICODE_STRING ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

typedef struct _DRIVER_OBJECT
{
   /*
    * PURPOSE: Magic values for debugging
    */
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
   PFAST_IO_DISPATCH FastIoDispatch;
   PDRIVER_INITIALIZE DriverInit;
   PDRIVER_STARTIO DriverStartIo;
   PDRIVER_UNLOAD DriverUnload;
   PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;


typedef struct _CONFIGURATION_INFORMATION
{
   ULONG DiskCount;
   ULONG FloppyCount;
   ULONG CDRomCount;
   ULONG TapeCount;
   ULONG ScsiPortCount;
   ULONG SerialCount;
   ULONG ParallelCount;
   BOOLEAN AtDiskPrimaryAddressClaimed;
   BOOLEAN AtDiskSecondaryAddressClaimed;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef VOID (*PIO_DPC_ROUTINE)(PKDPC Dpc,
				PDEVICE_OBJECT DeviceObject,
				PIRP Irp,
				PVOID Context);

typedef VOID (*PIO_TIMER_ROUTINE)(PDEVICE_OBJECT DeviceObject,
				  PVOID Context);

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

typedef struct _DRIVER_LAYOUT_INFORMATION
{
   ULONG PartitionCount;
   ULONG Signature;
   PARTITION_INFORMATION PartitionEntry[1];
} DRIVER_LAYOUT_INFORMATION, *PDRIVER_LAYOUT_INFORMATION;





typedef struct _IO_RESOURCE_DESCRIPTOR
{
   UCHAR Option;
   UCHAR Type;
   UCHAR SharedDisposition;
   
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

typedef struct _IO_RESOURCE_LIST
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCES_REQUIREMENTS_LIST
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
} IO_RESOURCES_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

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
     } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST;

typedef struct
{
   INTERFACE_TYPE InterfaceType;
   ULONG BusNumber;
   CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR;

typedef struct
{
   ULONG Count;
   CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;


#endif __INCLUDE_DDK_IOTYPES_H
