/* GENERAL DEFINITIONS ****************************************************/

#include <internal/hal/irq.h>



/*
 * PURPOSE: Number of a thread priority levels
 */
#define NR_PRIORITY_LEVELS (32)

/*
 * PURPOSE: Type of queue to insert a work item in
 */
enum
{
  CriticalWorkQueue,
  DelayedWorkQueue,
  HyperCriticalWorkQueue,
};

/*
 * Types of memory to allocate
 */
enum
{
   NonPagedPool,
   NonPagedPoolMustSucceed,
   NonPagedPoolCacheAligned,
   NonPagedPoolCacheAlignedMustS,
   PagedPool,
   PagedPoolCacheAligned,
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
 * Possible status codes
 * FIXME: These may not be the actual values used by NT
 */
enum
{
   STATUS_SUCCESS,
   STATUS_INSUFFICIENT_RESOURCES,
   STATUS_OBJECT_NAME_EXISTS,
   STATUS_OBJECT_NAME_COLLISION,
//   STATUS_DATATYPE_MISALIGNMENT,
   STATUS_CTL_FILE_NOT_SUPPORTED,
//   STATUS_ACCESS_VIOLATION,
   STATUS_PORT_ALREADY_SET,
   STATUS_SECTION_NOT_IMAGE,
   STATUS_BAD_WORKING_SET_LIMIT,
   STATUS_INCOMPATIBLE_FILE_MAP,
   STATUS_HANDLE_NOT_WAITABLE,
   STATUS_PORT_DISCONNECTED,
   STATUS_NOT_LOCKED,
   STATUS_NOT_MAPPED_VIEW,
   STATUS_UNABLE_TO_FREE_VM,
   STATUS_UNABLE_TO_DELETE_SECTION,
   STATUS_MORE_PROCESSING_REQUIRED,
   STATUS_INVALID_CID,
   STATUS_BAD_INITIAL_STACK,
   STATUS_INVALID_VOLUME_LABEL,
   STATUS_SECTION_NOT_EXTENDED,
   STATUS_NOT_MAPPED_DATA,
   STATUS_INFO_LENGTH_MISMATCH,
   STATUS_INVALID_INFO_CLASS,
   STATUS_SUSPEND_COUNT_EXCEEDED,
   STATUS_NOTIFY_ENUM_DIR,
   STATUS_REGISTRY_RECOVERED,
   STATUS_REGISTRY_IO_FAILED,
   STATUS_KEY_DELETED,
   STATUS_NO_LOG_SPACE,
   STATUS_KEY_HAS_CHILDREN,
   STATUS_CHILD_MUST_BE_VOLATILE,
   STATUS_REGISTRY_CORRUPT,
   STATUS_DLL_NOT_FOUND,
   STATUS_DLL_INIT_FAILED,
   STATUS_ORDINAL_NOT_FOUND,
   STATUS_ENTRYPOINT_NOT_FOUND,
//   STATUS_PENDING,
   STATUS_MORE_ENTRIES,
//   STATUS_INTEGER_OVERFLOW,
   STATUS_BUFFER_OVERFLOW,
   STATUS_NO_MORE_FILES,
   STATUS_NO_INHERITANCE,
   STATUS_NO_MORE_EAS,
   STATUS_NO_MORE_ENTRIES,
   STATUS_GUIDS_EXHAUSTED,
   STATUS_AGENTS_EXHAUSTED,
   STATUS_UNSUCCESSFUL,
   STATUS_NOT_IMPLEMENTED,
   STATUS_ILLEGAL_FUNCTION,
//   STATUS_IN_PAGE_ERROR,
   STATUS_PAGEFILE_QUOTA,
   STATUS_COMMITMENT_LIMIT,
   STATUS_SECTION_TOO_BIG,
   RPC_NT_SS_IN_NULL_CONTEXT,
   RPC_NT_INVALID_BINDING,
//   STATUS_INVALID_HANDLE,
   STATUS_OBJECT_FILE_MISMATCH,
   STATUS_FILE_CLOSED,
   STATUS_INVALID_PORT_HANDLE,
   STATUS_NOT_COMMITTED,
   STATUS_INVALID_PARAMETER,
   STATUS_INVALID_PARAMETER_1,
   STATUS_INVALID_PARAMETER_2,
   STATUS_INVALID_PARAMETER_3,
   STATUS_INVALID_PARAMETER_4,
   STATUS_INVALID_PARAMETER_5,
   STATUS_INVALID_PARAMETER_6,
   STATUS_INVALID_PARAMETER_7,
   STATUS_INVALID_PARAMETER_8,
   STATUS_INVALID_PARAMETER_9,
   STATUS_INVALID_PARAMETER_10,
   STATUS_INVALID_PARAMETER_11,
   STATUS_INVALID_PARAMETER_12,
   STATUS_INVALID_PARAMETER_MAX,
   STATUS_INVALID_PAGE_PROTECTION,
   STATUS_RESOURCE_DATA_NOT_FOUND,
   STATUS_RESOURCE_TYPE_NOT_FOUND,
   STATUS_RESOURCE_NAME_NOT_FOUND,
   STATUS_RESOURCE_LANG_NOT_FOUND,
   STATUS_NO_SUCH_DEVICE,
   STATUS_NO_SUCH_FILE,
   STATUS_INVALID_DEVICE_REQUEST,
   STATUS_END_OF_FILE,
   STATUS_FILE_FORCED_CLOSED,
   STATUS_WRONG_VOLUME,
   STATUS_NO_MEDIA,
   STATUS_NO_MEDIA_IN_DEVICE,
   STATUS_NONEXISTENT_SECTOR,
   STATUS_WORKING_SET_QUOTA,
//   STATUS_NO_MEMORY,
   STATUS_CONFLICTING_ADDRESS,
   STATUS_INVALID_SYSTEM_SERVICE,
   STATUS_THREAD_IS_TERMINATING,
   STATUS_PROCESS_IS_TERMINATING,
   STATUS_INVALID_LOCK_SEQUENCE,
   STATUS_INVALID_VIEW_SIZE,
   STATUS_ALREADY_COMMITTED,
   STATUS_ACCESS_DENIED,
   STATUS_FILE_IS_A_DIRECTORY,
   STATUS_CANNOT_DELETE,
   STATUS_INVALID_COMPUTER_NAME,
   STATUS_FILE_DELETED,
   STATUS_DELETE_PENDING,
   STATUS_PORT_CONNECTION_REFUSED,
   STATUS_NO_SUCH_PRIVILEGE,
   STATUS_PRIVILEGE_NOT_HELD,
   STATUS_CANNOT_IMPERSONATE,
   STATUS_LOGON_FAILURE,
   STATUS_ACCOUNT_RESTRICTION,
   STATUS_INVALID_LOGON_HOURS,
   STATUS_INVALID_WORKSTATION,
   STATUS_BUFFER_TOO_SMALL,
   STATUS_UNABLE_TO_DECOMMIT_VM,
   STATUS_DISK_CORRUPT_ERROR,
   STATUS_OBJECT_NAME_INVALID,
   STATUS_OBJECT_NAME_NOT_FOUND,
//   STATUS_OBJECT_NAME_COLLISION,
   STATUS_OBJECT_PATH_INVALID,
   STATUS_OBJECT_PATH_NOT_FOUND,
   STATUS_DFS_EXIT_PATH_FOUND,
   STATUS_OBJECT_PATH_SYNTAX_BAD,
   STATUS_DATA_OVERRUN,
   STATUS_DATA_LATE_ERROR,
   STATUS_DATA_ERROR,
   STATUS_CRC_ERROR,
   STATUS_SHARING_VIOLATION,
   STATUS_QUOTA_EXCEEDED,
   STATUS_MUTANT_NOT_OWNED,
   STATUS_SEMAPHORE_LIMIT_EXCEEDED,
   STATUS_DISK_FULL,
   STATUS_LOCK_NOT_GRANTED,
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
 * This is a list of bug check types (not MS's)
 */
enum
{
   KBUG_NONE,
   KBUG_ORPHANED_IRP,
   KBUG_IO_STACK_OVERFLOW,
   KBUG_OUT_OF_MEMORY,
   KBUG_POOL_FREE_LIST_CORRUPT,
     
   /*
    * These are well known but the actual value is unknown
    */
   NO_PAGES_AVAILABLE,
     
   /*
    * These are well known (MS) bug types
    * (Reference: NT Insider 1997 - http://www.osr.com)
    */
   IRQL_NOT_LESS_OR_EQUAL = 0xa,
   KMODE_EXCEPTION_NOT_HANDLED = 0x1e,
   UNEXPECTED_KERNEL_MODE_TRAP = 0x7f,
   PAGE_FAULT_IN_NON_PAGED_AREA = 0x50,
};

/*
 * PURPOSE: Object attributes
 */
enum
{
   OBJ_INHERIT = 0x1,
   OBJ_PERMANENT = 0x2,
   OBJ_EXCLUSIVE = 0x4,
   OBJ_CASE_INSENSITIVE = 0x8,
   OBJ_OPENIF = 0x10,
};

/*
 * PURPOSE: DPC priorities
 */
enum
{
   High,
   Medium,
   Low,
};

/*
 * PURPOSE: Timer types
 */
enum
  {
      NotificationTimer,
      SynchronizationTimer,	  
  };
   
/*
 * PURPOSE: Some drivers use these
 */
#define IN
#define OUT
#define OPTIONAL

/*
 * PURPOSE: Power IRP minor function numbers
 */
enum
{
   IRP_MN_QUERY_POWER,
   IRP_MN_SET_POWER,
   IRP_MN_WAIT_WAKE,
   IRP_MN_QUERY_CAPABILITIES,
   IRP_MN_POWER_SEQUENCE,
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

/*
 * PURPOSE: Used all over
 */
enum
{
   KernelMode,
   UserMode,
};
   
/*
 * PURPOSE: Arguments to MmProbeAndLockPages
 */
enum
{
   IoReadAccess,
   IoWriteAccess,
   IoModifyAccess,
};

#define MAXIMUM_VOLUME_LABEL_LENGTH (32)

/*
 * IRQ levels
 */
enum
{
   PASSIVE_LEVEL,
     
   /*
    * Which order for these (only DISPATCH_LEVEL is important for now)
    */
   APC_LEVEL,
   DISPATCH_LEVEL,
     
   /*
    * Above here are device specific IRQ levels
    */
   FIRST_DEVICE_SPECIFIC_LEVEL,
   HIGH_LEVEL = FIRST_DEVICE_SPECIFIC_LEVEL + NR_DEVICE_SPECIFIC_LEVELS,
};
  
