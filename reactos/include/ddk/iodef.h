#ifndef __INCLUDE_DDK_IODEF_H
#define __INCLUDE_DDK_IODEF_H

typedef enum _IO_QUERY_DEVICE_DESCRIPTION
{
   IoQueryDeviceIdentifier = 0,
   IoQueryDeviceConfigurationData,
   IoQueryDeviceComponentInformation,
   IoQueryDeviceDataFormatMaximum,
} IO_QUERY_DEVICE_DESCRIPTION, *PIO_QUERY_DEVICE_DESCRIPTION;

typedef enum _CONFIGURATION_TYPE
{
   DiskController,
   ParallelController,
   MaximumType,
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

typedef enum _CM_RESOURCE_TYPE
{
   CmResourceTypePort = 1,
   CmResourceTypeInterrupt,
   CmResourceTypeMemory,
   CmResourceTypeDma,
   CmResourceTypeDeviceSpecific,
   CmResourceTypeMaximum,
} CM_RESOURCE_TYPE;

typedef enum _CM_SHARE_DISPOSITION
{
   CmResourceShareDeviceExclusive = 1,
   CmResourceShareDriverExclusive,
   CmResourceShareShared,
   CmResourceShareMaximum,
} CM_SHARE_DISPOSITION;

enum
{
   CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE,
   CM_RESOURCE_INTERRUPT_LATCHED,
};

enum
{
   CM_RESOURCE_PORT_MEMORY,
   CM_RESOURCE_PORT_IO,
};

/*
 * PURPOSE: Irp flags
 */
enum
{
   /*
    * Read any data from the actual backing media
    */
   IRP_NOCACHE,
     
   /*
    * The I/O operation is performing paging
    */
   IRP_PAGING_IO,
     
   /*
    * The IRP is for a mount operation
    */
   IRP_MOUNT_COMPLETION,
     
   /*
    * The API expects synchronous behaviour
    */
   IRP_SYNCHRONOUS_API,
     
   /*
    * The IRP is associated with a larger operation
    */
   IRP_ASSOCIATED_IRP,
     
   /*
    * The AssociatedIrp.SystemBuffer field is valid
    */
   IRP_BUFFERED_IO,
     
   /*
    * The system buffer was allocated from pool and should be deallocated 
    * by the I/O manager
    */
   IRP_DEALLOCATE_BUFFER,
     
   /*
    * The IRP is for an input operation
    */
   IRP_INPUT_OPERATION,
     
   /*
    * The paging operation should complete synchronously 
    */
   IRP_SYNCHRONOUS_PAGING_IO,
     
   /*
    * The IRP represents a filesystem create operation
    */
   IRP_CREATE_OPERATION,
     
   /*
    * The IRP represents a filesystem read operation
    */
   IRP_READ_OPERATION,
     
   /*
    * The IRP represents a filesystem write operation
    */
   IRP_WRITE_OPERATION,
          
   /*
    * The IRP represents a filesystem close operation
    */
   IRP_CLOSE_OPERATION,
     
   /*
    * Asynchronous behavior is advised but not required
    */
   IRP_DEFER_IO_COMPLETION,
};

/*
 * I/O operation flags
 */
enum
{
   /*
    * Force an access check even if opened in kernel mode
    */
   SL_FORCE_ACCESS_CHECK,
     
   /*
    * The file being opened is a paging file
    */
   SL_OPEN_PAGING_FILE,
     
   SL_OPEN_TARGET_DIRECTORY,
     
   SL_CASE_SENSITIVE,
     
   SL_KEY_SPECIFIED,
     
   SL_OVERRIDE_VERIFY_VOLUME,
     
   SL_WRITE_THROUGH,
     
   SL_FT_SEQUENTIAL_WRITE,
     
   SL_FAIL_IMMEDIATELY,
     
   SL_EXCLUSIVE_LOCK,
     
   SL_RESTART_SCAN,
     
   SL_RETURN_SINGLE_ENTRY,
     
   SL_INDEX_SPECIFIED,
     
   SL_WATCH_TREE,
     
   SL_ALLOW_RAW_MOUNT,
     
   SL_PENDING_RETURNED,
     
};

enum
{
   SL_INVOKE_ON_SUCCESS = 1,
   SL_INVOKE_ON_ERROR = 2,
   SL_INVOKE_ON_CANCEL = 4,
};

/*
 * Possible flags for the device object flags
 */
enum
{
   DO_BUFFERED_IO = 0x1,
   DO_DIRECT_IO   = 0x2,
};

/*
 * Possible device types
 */
enum
{
   /*
    * Standard define types
    */
   FILE_DEVICE_BEEP,
   FILE_DEVICE_CDROM,
   FILE_DEVICE_CONTROLLER,
   FILE_DEVICE_DISK,
   FILE_DEVICE_INPORT_PORT,
   FILE_DEVICE_KEYBOARD,
   FILE_DEVICE_MIDI_IN,
   FILE_DEVICE_MIDI_OUT,
   FILE_DEVICE_MOUSE,
   FILE_DEVICE_NULL,
   FILE_DEVICE_PARALLEL_PORT,
   FILE_DEVICE_PRINTER,
   FILE_DEVICE_SCANNER,
   FILE_DEVICE_SERIAL_MOUSE_PORT,
   FILE_DEVICE_SERIAL_PORT,
   FILE_DEVICE_SCREEN,
   FILE_DEVICE_TAPE,
   FILE_DEVICE_UNKNOWN,
   FILE_DEVICE_VIDEO,
   FILE_DEVICE_VIRTUAL_DISK,
   FILE_DEVICE_WAVE_IN,
   FILE_DEVICE_WAVE_OUT,
   FILE_DEVICE_8042_PORT,
     
   /*
    * Values beyond this are reserved for ISVs
    */
   FILE_DEVICE_FIRST_FREE = 32768
};



/*
 * Possible device characteristics
 */
enum
{
   FILE_REMOVABLE_MEDIA  = 0x1,
   FILE_READ_ONLY_DEVICE = 0x2,
   FILE_FLOPPY_DISKETTE  = 0x4,
   FILE_WRITE_ONCE_MEDIA = 0x8,
   FILE_REMOTE_DEVICE    = 0x10,
};

/*
 * PURPOSE: Bus types
 */
enum
{
   Internal,
   Isa,
   MicroChannel,
   TurboChannel,
   PCIBus,
   MaximumInterfaceType,
};

/*
 * FIXME: These are not in the correct order
 */
enum
{  
     IRP_MJ_CREATE,
     IRP_MJ_CREATE_NAMED_PIPE,
     IRP_MJ_CLOSE,
     IRP_MJ_READ,
     IRP_MJ_WRITE,
     IRP_MJ_QUERY_INFORMATION,
     IRP_MJ_SET_INFORMATION,
     IRP_MJ_QUERY_EA,
     IRP_MJ_SET_EA,
     IRP_MJ_FLUSH_BUFFERS,
     IRP_MJ_QUERY_VOLUME_INFORMATION,
     IRP_MJ_SET_VOLUME_INFORMATION,
     IRP_MJ_DIRECTORY_CONTROL,
     IRP_MJ_FILE_SYSTEM_CONTROL,     
     IRP_MJ_DEVICE_CONTROL,
     IRP_MJ_INTERNAL_DEVICE_CONTROL,
     IRP_MJ_SHUTDOWN,
     IRP_MJ_LOCK_CONTROL,
     IRP_MJ_CLEANUP,
     IRP_MJ_CREATE_MAILSLOT,
     IRP_MJ_QUERY_SECURITY,
     IRP_MJ_SET_SECURITY,
     IRP_MJ_QUERY_POWER,
     IRP_MJ_SET_POWER,
     IRP_MJ_DEVICE_CHANGE,
     IRP_MJ_QUERY_QUOTA,
     IRP_MJ_SET_QUOTA,
     IRP_MJ_PNP_POWER,
     IRP_MJ_MAXIMUM_FUNCTION,
};

enum
/*
 * PURPOSE: Details about the result of a file open or create
 */
{
     FILE_CREATED,
//     FILE_OPENED,
     FILE_OVERWRITTEN,
     FILE_SUPERSEDED,
     FILE_EXISTS,
     FILE_DOES_NOT_EXIST,
};

#endif
