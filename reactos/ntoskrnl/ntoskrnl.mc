;
; ntoskrnl.exe bug codes 
;

MessageIdTypedef=ULONG

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

LanguageNames=(English=0x409:MSG00409)

;
; message definitions
;

MessageId=0x0
Severity=Success
Facility=System
SymbolicName=UNDEFINED_BUG_CODE
Language=English
The bug code is undefined. Please use an existing code instead.
.

MessageId=0x01
Severity=Success
Facility=System
SymbolicName=APC_INDEX_MISMATCH
Language=English
APC_INDEX_MISMATCH
.

MessageId=0x02
Severity=Success
Facility=System
SymbolicName=DEVICE_QUEUE_NOT_BUSY
Language=English
DEVICE_QUEUE_NOT_BUSY
.

MessageId=0x3
Severity=Success
Facility=System
SymbolicName=INVALID_AFFINITY_SET
Language=English
INVALID_AFFINITY_SET
.

MessageId=0x04
Severity=Success
Facility=System
SymbolicName=INVALID_DATA_ACCESS_TRAP
Language=English
INVALID_DATA_ACCESS_TRAP
.

MessageId=0x05
Severity=Success
Facility=System
SymbolicName=INVALID_PROCESS_ATTACH_ATTEMPT
Language=English
INVALID_PROCESS_ATTACH_ATTEMPT
.

MessageId=0x06
Severity=Success
Facility=System
SymbolicName=INVALID_PROCESS_DETACH_ATTEMPT
Language=English
INVALID_PROCESS_DETACH_ATTEMPT
.

MessageId=0x7
Severity=Success
Facility=System
SymbolicName=INVALID_SOFTWARE_INTERRUPT
Language=English
INVALID_SOFTWARE_INTERRUPT
.

MessageId=0x08
Severity=Success
Facility=System
SymbolicName=IRQL_NOT_DISPATCH_LEVEL
Language=English
IRQL_NOT_DISPATCH_LEVEL
.

MessageId=0x09
Severity=Success
Facility=System
SymbolicName=IRQL_NOT_GREATER_OR_EQUAL
Language=English
IRQL_NOT_GREATER_OR_EQUAL
.

MessageId=0x0A
Severity=Success
Facility=System
SymbolicName=IRQL_NOT_LESS_OR_EQUAL
Language=English
IRQL_NOT_LESS_OR_EQUAL
.

MessageId=0x0B
Severity=Success
Facility=System
SymbolicName=NO_EXCEPTION_HANDLING_SUPPORT
Language=English
NO_EXCEPTION_HANDLING_SUPPORT
.

MessageId=0x0C
Severity=Success
Facility=System
SymbolicName=MAXIMUM_WAIT_OBJECTS_EXCEEDED
Language=English
MAXIMUM_WAIT_OBJECTS_EXCEEDED
.

MessageId=0x0D
Severity=Success
Facility=System
SymbolicName=MUTEX_LEVEL_NUMBER_VIOLATION
Language=English
MUTEX_LEVEL_NUMBER_VIOLATION
.

MessageId=0x0E
Severity=Success
Facility=System
SymbolicName=NO_USER_MODE_CONTEXT
Language=English
NO_USER_MODE_CONTEXT
.

MessageId=0x0F
Severity=Success
Facility=System
SymbolicName=SPIN_LOCK_ALREADY_OWNED
Language=English
SPIN_LOCK_ALREADY_OWNED
.

MessageId=0x10
Severity=Success
Facility=System
SymbolicName=SPIN_LOCK_NOT_OWNED
Language=English
SPIN_LOCK_NOT_OWNED
.

MessageId=0x11
Severity=Success
Facility=System
SymbolicName=THREAD_NOT_MUTEX_OWNER
Language=English
THREAD_NOT_MUTEX_OWNER
.

MessageId=0x12
Severity=Success
Facility=System
SymbolicName=TRAP_CAUSE_UNKNOWN
Language=English
TRAP_CAUSE_UNKNOWN
.

MessageId=0x13
Severity=Success
Facility=System
SymbolicName=EMPTY_THREAD_REAPER_LIST
Language=English
EMPTY_THREAD_REAPER_LIST
.

MessageId=0x14
Severity=Success
Facility=System
SymbolicName=CREATE_DELETE_LOCK_NOT_LOCKED
Language=English
The thread reaper was handed a thread to reap, but the thread's process'
.

MessageId=0x15
Severity=Success
Facility=System
SymbolicName=LAST_CHANCE_CALLED_FROM_KMODE
Language=English
LAST_CHANCE_CALLED_FROM_KMODE
.

MessageId=0x16
Severity=Success
Facility=System
SymbolicName=CID_HANDLE_CREATION
Language=English
CID_HANDLE_CREATION
.

MessageId=0x17
Severity=Success
Facility=System
SymbolicName=CID_HANDLE_DELETION
Language=English
CID_HANDLE_DELETION
.

MessageId=0x18
Severity=Success
Facility=System
SymbolicName=REFERENCE_BY_POINTER
Language=English
REFERENCE_BY_POINTER
.

MessageId=0x19
Severity=Success
Facility=System
SymbolicName=BAD_POOL_HEADER
Language=English
BAD_POOL_HEADER
.

MessageId=0x1A
Severity=Success
Facility=System
SymbolicName=MEMORY_MANAGEMENT
Language=English
MEMORY_MANAGEMENT
.

MessageId=0x1B
Severity=Success
Facility=System
SymbolicName=PFN_SHARE_COUNT
Language=English
PFN_SHARE_COUNT
.

MessageId=0x1C
Severity=Success
Facility=System
SymbolicName=PFN_REFERENCE_COUNT
Language=English
PFN_REFERENCE_COUNT
.

MessageId=0x1D
Severity=Success
Facility=System
SymbolicName=NO_SPINLOCK_AVAILABLE
Language=English
NO_SPINLOCK_AVAILABLE
.

MessageId=0x1E
Severity=Success
Facility=System
SymbolicName=KMODE_EXCEPTION_NOT_HANDLED
Language=English
KMODE_EXCEPTION_NOT_HANDLED
.

MessageId=0x1F
Severity=Success
Facility=System
SymbolicName=SHARED_RESOURCE_CONV_ERROR
Language=English
SHARED_RESOURCE_CONV_ERROR
.

MessageId=0x20
Severity=Success
Facility=System
SymbolicName=KERNEL_APC_PENDING_DURING_EXIT
Language=English
KERNEL_APC_PENDING_DURING_EXIT
.

MessageId=0x21
Severity=Success
Facility=System
SymbolicName=QUOTA_UNDERFLOW
Language=English
QUOTA_UNDERFLOW
.

MessageId=0x22
Severity=Success
Facility=System
SymbolicName=FILE_SYSTEM
Language=English
FILE_SYSTEM
.

MessageId=0x23
Severity=Success
Facility=System
SymbolicName=FAT_FILE_SYSTEM
Language=English
FAT_FILE_SYSTEM
.

MessageId=0x24
Severity=Success
Facility=System
SymbolicName=NTFS_FILE_SYSTEM
Language=English
NTFS_FILE_SYSTEM
.

MessageId=0x25
Severity=Success
Facility=System
SymbolicName=NPFS_FILE_SYSTEM
Language=English
NPFS_FILE_SYSTEM
.

MessageId=0x26
Severity=Success
Facility=System
SymbolicName=CDFS_FILE_SYSTEM
Language=English
CDFS_FILE_SYSTEM
.

MessageId=0x27
Severity=Success
Facility=System
SymbolicName=RDR_FILE_SYSTEM
Language=English
RDR_FILE_SYSTEM
.

MessageId=0x28
Severity=Success
Facility=System
SymbolicName=CORRUPT_ACCESS_TOKEN
Language=English
CORRUPT_ACCESS_TOKEN
.

MessageId=0x29
Severity=Success
Facility=System
SymbolicName=SECURITY_SYSTEM
Language=English
SECURITY_SYSTEM
.

MessageId=0x2A
Severity=Success
Facility=System
SymbolicName=INCONSISTENT_IRP
Language=English
INCONSISTENT_IRP
.

MessageId=0x2B
Severity=Success
Facility=System
SymbolicName=PANIC_STACK_SWITCH
Language=English
PANIC_STACK_SWITCH
.

MessageId=0x2C
Severity=Success
Facility=System
SymbolicName=PORT_DRIVER_INTERNAL
Language=English
PORT_DRIVER_INTERNAL
.

MessageId=0x2D
Severity=Success
Facility=System
SymbolicName=SCSI_DISK_DRIVER_INTERNAL
Language=English
SCSI_DISK_DRIVER_INTERNAL
.

MessageId=0x2E
Severity=Success
Facility=System
SymbolicName=DATA_BUS_ERROR
Language=English
DATA_BUS_ERROR
.

MessageId=0x2F
Severity=Success
Facility=System
SymbolicName=INSTRUCTION_BUS_ERROR
Language=English
INSTRUCTION_BUS_ERROR
.

MessageId=0x30
Severity=Success
Facility=System
SymbolicName=SET_OF_INVALID_CONTEXT
Language=English
SET_OF_INVALID_CONTEXT
.

MessageId=0x31
Severity=Success
Facility=System
SymbolicName=PHASE0_INITIALIZATION_FAILED
Language=English
PHASE0_INITIALIZATION_FAILED
.

MessageId=0x32
Severity=Success
Facility=System
SymbolicName=PHASE1_INITIALIZATION_FAILED
Language=English
PHASE1_INITIALIZATION_FAILED
.

MessageId=0x33
Severity=Success
Facility=System
SymbolicName=UNEXPECTED_INITIALIZATION_CALL
Language=English
UNEXPECTED_INITIALIZATION_CALL
.

MessageId=0x34
Severity=Success
Facility=System
SymbolicName=CACHE_MANAGER
Language=English
CACHE_MANAGER
.

MessageId=0x35
Severity=Success
Facility=System
SymbolicName=NO_MORE_IRP_STACK_LOCATIONS
Language=English
NO_MORE_IRP_STACK_LOCATIONS
.

MessageId=0x36
Severity=Success
Facility=System
SymbolicName=DEVICE_REFERENCE_COUNT_NOT_ZERO
Language=English
DEVICE_REFERENCE_COUNT_NOT_ZERO
.

MessageId=0x37
Severity=Success
Facility=System
SymbolicName=FLOPPY_INTERNAL_ERROR
Language=English
FLOPPY_INTERNAL_ERROR
.

MessageId=0x38
Severity=Success
Facility=System
SymbolicName=SERIAL_DRIVER_INTERNAL
Language=English
SERIAL_DRIVER_INTERNAL
.

MessageId=0x39
Severity=Success
Facility=System
SymbolicName=SYSTEM_EXIT_OWNED_MUTEX
Language=English
SYSTEM_EXIT_OWNED_MUTEX
.




MessageId=0x3E
Severity=Success
Facility=System
SymbolicName=MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED
Language=English
MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED
.

MessageId=0x3F
Severity=Success
Facility=System
SymbolicName=NO_MORE_SYSTEM_PTES
Language=English
NO_MORE_SYSTEM_PTES
.

MessageId=0x40
Severity=Success
Facility=System
SymbolicName=TARGET_MDL_TOO_SMALL
Language=English
TARGET_MDL_TOO_SMALL
.

MessageId=0x41
Severity=Success
Facility=System
SymbolicName=MUST_SUCCEED_POOL_EMPTY
Language=English
MUST_SUCCEED_POOL_EMPTY
.

MessageId=0x42
Severity=Success
Facility=System
SymbolicName=ATDISK_DRIVER_INTERNAL
Language=English
ATDISK_DRIVER_INTERNAL
.



MessageId=0x44
Severity=Success
Facility=System
SymbolicName=MULTIPLE_IRP_COMPLETE_REQUESTS
Language=English
MULTIPLE_IRP_COMPLETE_REQUESTS
.

MessageId=0x45
Severity=Success
Facility=System
SymbolicName=INSUFFICIENT_SYSTEM_MAP_REGS
Language=English
INSUFFICIENT_SYSTEM_MAP_REGS
.



MessageId=0x48
Severity=Success
Facility=System
SymbolicName=CANCEL_STATE_IN_COMPLETED_IRP
Language=English
CANCEL_STATE_IN_COMPLETED_IRP
.

MessageId=0x49
Severity=Success
Facility=System
SymbolicName=PAGE_FAULT_WITH_INTERRUPTS_OFF
Language=English
PAGE_FAULT_WITH_INTERRUPTS_OFF
.

MessageId=0x4A
Severity=Success
Facility=System
SymbolicName=IRQL_GT_ZERO_AT_SYSTEM_SERVICE
Language=English
IRQL_GT_ZERO_AT_SYSTEM_SERVICE
.

MessageId=0x4B
Severity=Success
Facility=System
SymbolicName=STREAMS_INTERNAL_ERROR
Language=English
STREAMS_INTERNAL_ERROR
.



MessageId=0x4D
Severity=Success
Facility=System
SymbolicName=NO_PAGES_AVAILABLE
Language=English
NO_PAGES_AVAILABLE
.

MessageId=0x4E
Severity=Success
Facility=System
SymbolicName=PFN_LIST_CORRUPT
Language=English
PFN_LIST_CORRUPT
.

MessageId=0x4F
Severity=Success
Facility=System
SymbolicName=NDIS_INTERNAL_ERROR
Language=English
NDIS_INTERNAL_ERROR
.

MessageId=0x50
Severity=Success
Facility=System
SymbolicName=PAGE_FAULT_IN_NONPAGED_AREA
Language=English
PAGE_FAULT_IN_NONPAGED_AREA
.

MessageId=0x51
Severity=Success
Facility=System
SymbolicName=REGISTRY_ERROR
Language=English
REGISTRY_ERROR
.

MessageId=0x52
Severity=Success
Facility=System
SymbolicName=MAILSLOT_FILE_SYSTEM
Language=English
MAILSLOT_FILE_SYSTEM
.



MessageId=0x54
Severity=Success
Facility=System
SymbolicName=LM_SERVER_INTERNAL_ERROR
Language=English
LM_SERVER_INTERNAL_ERROR
.

MessageId=0x55
Severity=Success
Facility=System
SymbolicName=DATA_COHERENCY_EXCEPTION
Language=English
DATA_COHERENCY_EXCEPTION
.

MessageId=0x56
Severity=Success
Facility=System
SymbolicName=INSTRUCTION_COHERENCY_EXCEPTION
Language=English
INSTRUCTION_COHERENCY_EXCEPTION
.

MessageId=0x57
Severity=Success
Facility=System
SymbolicName=XNS_INTERNAL_ERROR
Language=English
XNS_INTERNAL_ERROR
.

MessageId=0x58
Severity=Success
Facility=System
SymbolicName=FTDISK_INTERNAL_ERROR
Language=English
FTDISK_INTERNAL_ERROR
.

MessageId=0x59
Severity=Success
Facility=System
SymbolicName=PINBALL_FILE_SYSTEM
Language=English
PINBALL_FILE_SYSTEM
.



MessageId=0x5C
Severity=Success
Facility=System
SymbolicName=HAL_INITIALIZATION_FAILED
Language=English
HAL_INITIALIZATION_FAILED
.

MessageId=0x5D
Severity=Success
Facility=System
SymbolicName=HEAP_INITIALIZATION_FAILED
Language=English
HEAP_INITIALIZATION_FAILED
.

MessageId=0x5E
Severity=Success
Facility=System
SymbolicName=OBJECT_INITIALIZATION_FAILED
Language=English
OBJECT_INITIALIZATION_FAILED
.

MessageId=0x5F
Severity=Success
Facility=System
SymbolicName=SECURITY_INITIALIZATION_FAILED
Language=English
SECURITY_INITIALIZATION_FAILED
.

MessageId=0x60
Severity=Success
Facility=System
SymbolicName=PROCESS_INITIALIZATION_FAILED
Language=English
PROCESS_INITIALIZATION_FAILED
.

MessageId=0x61
Severity=Success
Facility=System
SymbolicName=HAL1_INITIALIZATION_FAILED
Language=English
HAL1_INITIALIZATION_FAILED
.

MessageId=0x62
Severity=Success
Facility=System
SymbolicName=OBJECT1_INITIALIZATION_FAILED
Language=English
OBJECT1_INITIALIZATION_FAILED
.

MessageId=0x63
Severity=Success
Facility=System
SymbolicName=SECURITY1_INITIALIZATION_FAILED
Language=English
SECURITY1_INITIALIZATION_FAILED
.

MessageId=0x64
Severity=Success
Facility=System
SymbolicName=SYMBOLIC_INITIALIZATION_FAILED
Language=English
SYMBOLIC_INITIALIZATION_FAILED
.

MessageId=0x65
Severity=Success
Facility=System
SymbolicName=MEMORY1_INITIALIZATION_FAILED
Language=English
MEMORY1_INITIALIZATION_FAILED
.

MessageId=0x66
Severity=Success
Facility=System
SymbolicName=CACHE_INITIALIZATION_FAILED
Language=English
CACHE_INITIALIZATION_FAILED
.

MessageId=0x67
Severity=Success
Facility=System
SymbolicName=CONFIG_INITIALIZATION_FAILED
Language=English
CONFIG_INITIALIZATION_FAILED
.

MessageId=0x68
Severity=Success
Facility=System
SymbolicName=FILE_INITIALIZATION_FAILED
Language=English
FILE_INITIALIZATION_FAILED
.

MessageId=0x69
Severity=Success
Facility=System
SymbolicName=IO1_INITIALIZATION_FAILED
Language=English
IO1_INITIALIZATION_FAILED
.

MessageId=0x6A
Severity=Success
Facility=System
SymbolicName=LPC_INITIALIZATION_FAILED
Language=English
LPC_INITIALIZATION_FAILED
.

MessageId=0x6B
Severity=Success
Facility=System
SymbolicName=PROCESS1_INITIALIZATION_FAILED
Language=English
PROCESS1_INITIALIZATION_FAILED
.

MessageId=0x6C
Severity=Success
Facility=System
SymbolicName=REFMON_INITIALIZATION_FAILED
Language=English
REFMON_INITIALIZATION_FAILED
.

MessageId=0x6D
Severity=Success
Facility=System
SymbolicName=SESSION1_INITIALIZATION_FAILED
Language=English
SESSION1_INITIALIZATION_FAILED
.

MessageId=0x6E
Severity=Success
Facility=System
SymbolicName=SESSION2_INITIALIZATION_FAILED
Language=English
SESSION2_INITIALIZATION_FAILED
.

MessageId=0x6F
Severity=Success
Facility=System
SymbolicName=SESSION3_INITIALIZATION_FAILED
Language=English
SESSION3_INITIALIZATION_FAILED
.

MessageId=0x70
Severity=Success
Facility=System
SymbolicName=SESSION4_INITIALIZATION_FAILED
Language=English
SESSION4_INITIALIZATION_FAILED
.

MessageId=0x71
Severity=Success
Facility=System
SymbolicName=SESSION5_INITIALIZATION_FAILED
Language=English
SESSION5_INITIALIZATION_FAILED
.

MessageId=0x72
Severity=Success
Facility=System
SymbolicName=ASSIGN_DRIVE_LETTERS_FAILED
Language=English
ASSIGN_DRIVE_LETTERS_FAILED
.

MessageId=0x73
Severity=Success
Facility=System
SymbolicName=CONFIG_LIST_FAILED
Language=English
CONFIG_LIST_FAILED
.

MessageId=0x74
Severity=Success
Facility=System
SymbolicName=BAD_SYSTEM_CONFIG_INFO
Language=English
BAD_SYSTEM_CONFIG_INFO
.

MessageId=0x75
Severity=Success
Facility=System
SymbolicName=CANNOT_WRITE_CONFIGURATION
Language=English
CANNOT_WRITE_CONFIGURATION
.

MessageId=0x76
Severity=Success
Facility=System
SymbolicName=PROCESS_HAS_LOCKED_PAGES
Language=English
PROCESS_HAS_LOCKED_PAGES
.

MessageId=0x77
Severity=Success
Facility=System
SymbolicName=KERNEL_STACK_INPAGE_ERROR
Language=English
KERNEL_STACK_INPAGE_ERROR
.

MessageId=0x78
Severity=Success
Facility=System
SymbolicName=PHASE0_EXCEPTION
Language=English
PHASE0_EXCEPTION
.

MessageId=0x79
Severity=Success
Facility=System
SymbolicName=MISMATCHED_HAL
Language=English
Mismatched Kernel and HAL image
.

MessageId=0x7A
Severity=Success
Facility=System
SymbolicName=KERNEL_DATA_INPAGE_ERROR
Language=English
KERNEL_DATA_INPAGE_ERROR
.

MessageId=0x7B
Severity=Success
Facility=System
SymbolicName=INACCESSIBLE_BOOT_DEVICE
Language=English
INACCESSIBLE_BOOT_DEVICE
.



MessageId=0x7D
Severity=Success
Facility=System
SymbolicName=INSTALL_MORE_MEMORY
Language=English
INSTALL_MORE_MEMORY
.

MessageId=0x7E
Severity=Success
Facility=System
SymbolicName=UNEXPECTED_KERNEL_MODE_TRAP
Language=English
UNEXPECTED_KERNEL_MODE_TRAP
.

MessageId=0x7F
Severity=Success
Facility=System
SymbolicName=NMI_HARDWARE_FAILURE
Language=English
Hardware malfunction
.

MessageId=0x80
Severity=Success
Facility=System
SymbolicName=SPIN_LOCK_INIT_FAILURE
Language=English
SPIN_LOCK_INIT_FAILURE
.




MessageId=0x9A
Severity=Informational
Facility=System
SymbolicName=REACTOS_BANNER
Language=English
ReactOS Version %s (Build %s)
.

MessageId=0x9B
Severity=Informational
Facility=System
SymbolicName=REACTOS_SERVICE_PACK
Language=English
Service Pack
.

MessageId=0x9C
Severity=Informational
Facility=System
SymbolicName=REACTOS_INFO_STRING_UNI_PROCESSOR
Language=English
%u System Processor [%u MB Memory] %Z
.

MessageId=0x9D
Severity=Informational
Facility=System
SymbolicName=REACTOS_MP_KERNEL
Language=English
MulitProcessor Kernel
.

MessageId=0x9E
Severity=Informational
Facility=System
SymbolicName=REACTOS_INFO_STRING_MULTI_PROCESSOR
Language=English
%u System Processors [%u MB Memory] %Z
.

MessageId=0x9F
Severity=Informational
Facility=System
SymbolicName=REACTOS_COPYRIGHT_NOTICE
Language=English
\n\nReactOS is free software, covered by the GNU General Public License,
 and you\n are welcome to change it and/or distribute copies of it under
 certain\n conditions. There is absolutely no warranty for ReactOS.\n
.

; EOF