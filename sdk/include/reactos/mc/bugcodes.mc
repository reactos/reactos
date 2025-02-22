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

MessageId=0x7E
Severity=Informational
Facility=System
SymbolicName=WINDOWS_NT_BANNER
Language=English
ReactOS Version %s
Build %s
Reporting NT %s (Build %u%s)
.

MessageId=0x87
Severity=Informational
Facility=System
SymbolicName=WINDOWS_NT_CSD_STRING
Language=English
Service Pack
.

MessageId=0x88
Severity=Informational
Facility=System
SymbolicName=WINDOWS_NT_INFO_STRING
Language=English
%u System Processor [%u MB Memory] %Z
.

MessageId=0x89
Severity=Informational
Facility=System
SymbolicName=WINDOWS_NT_MP_STRING
Language=English
MultiProcessor Kernel
.

MessageId=0x8A
Severity=Informational
Facility=System
SymbolicName=THREAD_TERMINATE_HELD_MUTEX
Language=English
A kernel thread terminated while holding a mutex
.

MessageId=0x9D
Severity=Informational
Facility=System
SymbolicName=WINDOWS_NT_INFO_STRING_PLURAL
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

MessageId=0x7F
Severity=Warning
Facility=System
SymbolicName=BUGCHECK_MESSAGE_INTRO
Language=English
A problem has been detected and ReactOS has been shut down to prevent damage
to your computer.
.

MessageId=0x80
Severity=Warning
Facility=System
SymbolicName=BUGCODE_ID_DRIVER
Language=English
The problem seems to be caused by the following file:

.

MessageId=0x81
Severity=Warning
Facility=System
SymbolicName=PSS_MESSAGE_INTRO
Language=English
If this is the first time you've seen this Stop error screen,
restart your computer. If this screen appears again, follow
these steps:

.

MessageId=0x82
Severity=Warning
Facility=System
SymbolicName=BUGCODE_PSS_MESSAGE
Language=English
Check to make sure any new hardware or software is properly installed.
If this is a new installation, ask your hardware or software manufacturer
for any ReactOS updates you might need.

If problems continue, disable or remove any newly installed hardware
or software. Disable BIOS memory options such as caching or shadowing.
If you need to use Safe Mode to remove or disable components, restart
your computer, press F8 to select Advanced Startup Options, and then
select Safe Mode.
.

MessageId=0x83
Severity=Warning
Facility=System
SymbolicName=BUGCHECK_TECH_INFO
Language=English
Technical information:
.

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
CREATE_DELETE_LOCK_NOT_LOCKED
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
Check to be sure you have adequate disk space. If a driver is
identified in the Stop message, disable the driver or check
with the manufacturer for driver updates. Try changing video
adapters.

Check with your hardware vendor for any BIOS updates. Disable
BIOS memory options such as caching or shadowing. If you need
to use Safe Mode to remove or disable components, restart your
computer, press F8 to select Advanced Startup Options, and then
select Safe Mode.
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
Disable or uninstall any anti-virus, disk defragmentation
or backup utilities. Check your hard drive configuration,
and check for any updated drivers. Run CHKDSK /F to check
for hard drive corruption, and then restart your computer.
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
Run system diagnostics supplied by your hardware manufacturer.
In particular, run a memory check, and check for faulty or
mismatched memory. Try changing video adapters.

Check with your hardware vendor for any BIOS updates. Disable
BIOS memory options such as caching or shadowing. If you need
to use Safe Mode to remove or disable components, restart your
computer, press F8 to select Advanced Startup Options, and then
select Safe Mode.
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
Remove any recently installed software including backup
utilities or disk-intensive applications.

If you need to use Safe Mode to remove or disable components,
restart your computer, press F8 to select Advanced Startup
Options, and then select Safe Mode.
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

MessageId=0x4C
Severity=Success
Facility=System
SymbolicName=FATAL_UNHANDLED_HARD_ERROR
Language=English
FATAL_UNHANDLED_HARD_ERROR
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

MessageId=0x53
Severity=Success
Facility=System
SymbolicName=NO_BOOT_DEVICE
Language=English
NO_BOOT_DEVICE
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

MessageId=0x5A
Severity=Success
Facility=System
SymbolicName=CRITICAL_SERVICE_FAILED
Language=English
CRITICAL_SERVICE_FAILED
.

MessageId=0x5B
Severity=Success
Facility=System
SymbolicName=SET_ENV_VAR_FAILED
Language=English
SET_ENV_VAR_FAILED
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
SymbolicName=UNSUPPORTED_PROCESSOR
Language=English
UNSUPPORTED_PROCESSOR
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
Check for viruses on your computer. Remove any newly installed
hard drives or hard drive controllers. Check your hard drive
to make sure it is properly configured and terminated.
Run CHKDSK /F to check for hard drive corruption, and then
restart your computer.
.

MessageId=0x7C
Severity=Success
Facility=System
SymbolicName=BUGCODE_NDIS_DRIVER
Language=English
BUGCODE_NDIS_DRIVER
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
SymbolicName=SYSTEM_THREAD_EXCEPTION_NOT_HANDLED
Language=English
SYSTEM_THREAD_EXCEPTION_NOT_HANDLED
.

MessageId=0x7F
Severity=Success
Facility=System
SymbolicName=UNEXPECTED_KERNEL_MODE_TRAP
Language=English
Run a system diagnostic utility supplied by your hardware manufacturer.
In particular, run a memory check, and check for faulty or mismatched
memory. Try changing video adapters.

Disable or remove any newly installed hardware and drivers. Disable or
remove any newly installed software. If you need to use Safe Mode to
remove or disable components, restart your computer, press F8 to select
Advanced Startup Options, and then select Safe Mode.
.

MessageId=0x80
Severity=Success
Facility=System
SymbolicName=NMI_HARDWARE_FAILURE
Language=English
Hardware malfunction
.

MessageId=0x81
Severity=Success
Facility=System
SymbolicName=SPIN_LOCK_INIT_FAILURE
Language=English
SPIN_LOCK_INIT_FAILURE
.

MessageId=0x8E
Severity=Success
Facility=System
SymbolicName=KERNEL_MODE_EXCEPTION_NOT_HANDLED
Language=English
KERNEL_MODE_EXCEPTION_NOT_HANDLED
.

MessageId=0x8F
Severity=Success
Facility=System
SymbolicName=PP0_INITIALIZATION_FAILED
Language=English
PP0_INITIALIZATION_FAILED
.

MessageId=0x90
Severity=Success
Facility=System
SymbolicName=PP1_INITIALIZATION_FAILED
Language=English
PP1_INITIALIZATION_FAILED
.

MessageId=0x91
Severity=Success
Facility=System
SymbolicName=WIN32K_INIT_OR_RIT_FAILURE
Language=English
WIN32K_INIT_OR_RIT_FAILURE
.

MessageId=0x93
Severity=Success
Facility=System
SymbolicName=INVALID_KERNEL_HANDLE
Language=English
INVALID_KERNEL_HANDLE
.

MessageId=0x94
Severity=Success
Facility=System
SymbolicName=KERNEL_STACK_LOCKED_AT_EXIT
Language=English
KERNEL_STACK_LOCKED_AT_EXIT
.

MessageId=0x96
Severity=Success
Facility=System
SymbolicName=INVALID_WORK_QUEUE_ITEM
Language=English
INVALID_WORK_QUEUE_ITEM
.

MessageId=0x9A
Severity=Success
Facility=System
SymbolicName=MORAL_EXCEPTION_ERROR
Language=English
An attempt was made to execute a proprietary machine code instruction.
The system has been shut down to prevent damage to your conscience.

If this is the first time you have seen this error screen, read
<http://www.gnu.org/philosophy/free-sw.html>.

If problems continue, remove all nonfree software from your computer.
.

MessageId=0xA0
Severity=Success
Facility=System
SymbolicName=INTERNAL_POWER_ERROR
Language=English
INTERNAL_POWER_ERROR
.

MessageId=0xA1
Severity=Success
Facility=System
SymbolicName=PCI_BUS_DRIVER_INTERNAL
Language=English
Inconsistency detected in the PCI Bus driver's internal structures.
.

MessageId=0xA5
Severity=Success
Facility=System
SymbolicName=ACPI_BIOS_ERROR
Language=English
The BIOS in this system is not fully ACPI compliant.  Please contact your
system vendor for an updated BIOS.
.

MessageId=0xA8
Severity=Informational
Facility=System
SymbolicName=BOOTING_IN_SAFEMODE_MINIMAL
Language=English
The system is booting in safemode - Minimal Services
.

MessageId=0xA9
Severity=Informational
Facility=System
SymbolicName=BOOTING_IN_SAFEMODE_NETWORK
Language=English
The system is booting in safemode - Minimal Services with Network
.

MessageId=0xAA
Severity=Informational
Facility=System
SymbolicName=BOOTING_IN_SAFEMODE_DSREPAIR
Language=English
The system is booting in safemode - Directory Services Repair
.

MessageId=0xAC
Severity=Success
Facility=System
SymbolicName=HAL_MEMORY_ALLOCATION
Language=English
Allocate from NonPaged Pool failed for a HAL critical allocation.
.

MessageId=0xB4
Severity=Success
Facility=System
SymbolicName=VIDEO_DRIVER_INIT_FAILURE
Language=English
The video driver failed to initialize.
.

MessageId=0xB7
Severity=Informational
Facility=System
SymbolicName=BOOTLOG_ENABLED
Language=English
Boot Logging Enabled
.

MessageId=0xB8
Severity=Success
Facility=System
SymbolicName=ATTEMPTED_SWITCH_FROM_DPC
Language=English
A wait operation, attach process, or yield was attempted from a DPC routine.
.

MessageId=0xBE
Severity=Success
Facility=System
SymbolicName=ATTEMPTED_WRITE_TO_READONLY_MEMORY
Language=English
An attempt was made to write to read-only memory.
.

MessageId=0xC1
Severity=Success
Facility=System
SymbolicName=SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION
Language=English
SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION
.

MessageId=0xC2
Severity=Success
Facility=System
SymbolicName=BAD_POOL_CALLER
Language=English
BAD_POOL_CALLER
.

MessageId=0xC3
Severity=Success
Facility=System
SymbolicName=BUGCODE_PSS_MESSAGE_SIGNATURE
Language=English
A system file that is owned by ReactOS was replaced by an application
running on your system.  The operating system detected this and tried to
verify the validity of the file's signature.  The operating system found that
the file signature is not valid and put the original, correct file back
so that your operating system will continue to function properly.
.

MessageId=0xC4
Severity=Success
Facility=System
SymbolicName=DRIVER_VERIFIER_DETECTED_VIOLATION
Language=English
Driver Verifier has detected a fatal error condition.
.

MessageId=0xC5
Severity=Success
Facility=System
SymbolicName=DRIVER_CORRUPTED_EXPOOL
Language=English
A device driver has pool.

Check to make sure any new hardware or software is properly installed.
If this is a new installation, ask your hardware or software manufacturer
for any ReactOS updates you might need.

Run the driver verifier against any new (or suspect) drivers.
If that doesn't reveal the corrupting driver, try enabling special pool.
Both of these features are intended to catch the corruption at an earlier
point where the offending driver can be identified.

If you need to use Safe Mode to remove or disable components,
restart your computer, press F8 to select Advanced Startup Options,
and then select Safe Mode.
.

MessageId=0xC6
Severity=Success
Facility=System
SymbolicName=DRIVER_CAUGHT_MODIFYING_FREED_POOL
Language=English
A device driver attempting to corrupt the system has been caught.
The faulty driver currently on the kernel stack must be replaced
with a working version.
.

MessageId=0xC8
Severity=Success
Facility=System
SymbolicName=IRQL_UNEXPECTED_VALUE
Language=English
The processor's IRQL is not valid for the currently executing context.
This is a software error condition and is usually caused by a device
driver changing IRQL and not restoring it to its previous value when
it has finished its task.
.

MessageId=0xCA
Severity=Success
Facility=System
SymbolicName=PNP_DETECTED_FATAL_ERROR
Language=English
Plug and Play detected an error most likely caused by a faulty driver.
.

MessageId=0xCB
Severity=Success
Facility=System
SymbolicName=DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS
Language=English
DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS
.

MessageId=0xCC
Severity=Success
Facility=System
SymbolicName=PAGE_FAULT_IN_FREED_SPECIAL_POOL
Language=English
PAGE_FAULT_IN_FREED_SPECIAL_POOL
.

MessageId=0xCD
Severity=Success
Facility=System
SymbolicName=PAGE_FAULT_BEYOND_END_OF_ALLOCATION
Language=English
PAGE_FAULT_BEYOND_END_OF_ALLOCATION
.

MessageId=0xCE
Severity=Success
Facility=System
SymbolicName=DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS
Language=English
DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS
.

MessageId=0xD0
Severity=Success
Facility=System
SymbolicName=DRIVER_CORRUPTED_MMPOOL
Language=English
DRIVER_CORRUPTED_MMPOOL
.

MessageId=0xD1
Severity=Success
Facility=System
SymbolicName=DRIVER_IRQL_NOT_LESS_OR_EQUAL
Language=English
DRIVER_IRQL_NOT_LESS_OR_EQUAL
.

MessageId=0xD3
Severity=Success
Facility=System
SymbolicName=DRIVER_PORTION_MUST_BE_NONPAGED
Language=English
The driver mistakenly marked a part of its image pageable instead of non-pageable.
.

MessageId=0xD4
Severity=Success
Facility=System
SymbolicName=SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD
Language=English
The driver unloaded without cancelling pending operations.
.

MessageId=0xD5
Severity=Success
Facility=System
SymbolicName=DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL
Language=English
DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL
.

MessageId=0xD6
Severity=Success
Facility=System
SymbolicName=DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION
Language=English
DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION
.

MessageId=0xD7
Severity=Success
Facility=System
SymbolicName=DRIVER_UNMAPPING_INVALID_VIEW
Language=English
The driver is attempting to unmap an invalid memory address.
.

MessageId=0xD8
Severity=Success
Facility=System
SymbolicName=DRIVER_USED_EXCESSIVE_PTES
Language=English
The driver has used an excessive number of system PTEs.
.

MessageId=0xD9
Severity=Success
Facility=System
SymbolicName=LOCKED_PAGES_TRACKER_CORRUPTION
Language=English
The system's structures tracking locked pages have been corrupted.
.

MessageId=0xDA
Severity=Success
Facility=System
SymbolicName=SYSTEM_PTE_MISUSE
Language=English
The driver has called a system PTE routine in an improper way.
.

MessageId=0xDB
Severity=Success
Facility=System
SymbolicName=DRIVER_CORRUPTED_SYSPTES
Language=English
The driver has corrupted system PTEs.
.

MessageId=0xDC
Severity=Success
Facility=System
SymbolicName=DRIVER_INVALID_STACK_ACCESS
Language=English
The driver has accessed an invalid stack address.
.

MessageId=0xDE
Severity=Success
Facility=System
SymbolicName=POOL_CORRUPTION_IN_FILE_AREA
Language=English
Kernel pool corruption has been detected in an area marked to be written to disk.
.

MessageId=0xDF
Severity=Success
Facility=System
SymbolicName=IMPERSONATING_WORKER_THREAD
Language=English
A worker thread is impersonating another process. The work item forgot to
disable impersonation before it returned.
.

MessageId=0xE0
Severity=Success
Facility=System
SymbolicName=ACPI_BIOS_FATAL_ERROR
Language=English

Your computer (BIOS) has reported that a component in your system is faulty and
has prevented ReactOS from operating.  You can determine which component is
faulty by running the diagnostic disk or tool that came with your computer.

If you do not have this tool, you must contact your system vendor and report
this error message to them.  They will be able to assist you in correcting this
hardware problem thereby allowing ReactOS to operate.
.

MessageId=0xE1
Severity=Success
Facility=System
SymbolicName=WORKER_THREAD_RETURNED_AT_BAD_IRQL
Language=English
WORKER_THREAD_RETURNED_AT_BAD_IRQL
.

MessageId=0xE2
Severity=Success
Facility=System
SymbolicName=MANUALLY_INITIATED_CRASH
Language=English
The user manually generated the crash dump.
.

MessageId=0xE3
Severity=Success
Facility=System
SymbolicName=RESOURCE_NOT_OWNED
Language=English
RESOURCE_NOT_OWNED
.

MessageId=0xE4
Severity=Success
Facility=System
SymbolicName=WORKER_INVALID
Language=English
If Parameter1 == 0, an executive worker item was found in memory which
must not contain such items.  Usually this is memory being freed.  This
is usually caused by a device driver that has not cleaned up properly
before freeing memory.

If Parameter1 == 1, an attempt was made to queue an executive worker item
with a usermode execution routine.
.

MessageId=0xE5
Severity=Success
Facility=System
SymbolicName=POWER_FAILURE_SIMULATE
Language=English
POWER_FAILURE_SIMULATE
.

MessageId=0xE7
Severity=Success
Facility=System
SymbolicName=INVALID_FLOATING_POINT_STATE
Language=English
INVALID_FLOATING_POINT_STATE
.

MessageId=0xE8
Severity=Success
Facility=System
SymbolicName=INVALID_CANCEL_OF_FILE_OPEN
Language=English
Invalid cancel of a open file. It already has handle.
.

MessageId=0xE9
Severity=Success
Facility=System
SymbolicName=ACTIVE_EX_WORKER_THREAD_TERMINATION
Language=English
An executive worker thread is being terminated without having gone through the worker thread rundown code.
Work items queued to the Ex worker queue must not terminate their threads.
A stack trace should indicate the culprit.
.

MessageId=0xEA
Severity=Success
Facility=System
SymbolicName=THREAD_STUCK_IN_DEVICE_DRIVER
Language=English

The device driver got stuck in an infinite loop. This usually indicates
problem with the device itself or with the device driver programming the
hardware incorrectly.

Please check with your hardware device vendor for any driver updates.
.

MessageId=0xEF
Severity=Success
Facility=System
SymbolicName=CRITICAL_PROCESS_DIED
Language=English
The kernel attempted to ready a thread that was in an incorrect state such as terminated.
.

MessageId=0xF4
Severity=Success
Facility=System
SymbolicName=CRITICAL_OBJECT_TERMINATION
Language=English
A process or thread crucial to system operation has unexpectedly exited or been terminated.
.

MessageId=0xF6
Severity=Success
Facility=System
SymbolicName=PCI_VERIFIER_DETECTED_VIOLATION
Language=English
The PCI driver has detected an error in a PCI device or BIOS being verified.
.

MessageId=0xF7
Severity=Success
Facility=System
SymbolicName=DRIVER_OVERRAN_STACK_BUFFER
Language=English
A driver has overrun a stack-based buffer.  This overrun could potentially
allow a malicious user to gain control of this machine.
.

MessageId=0xF8
Severity=Success
Facility=System
SymbolicName=RAMDISK_BOOT_INITIALIZATION_FAILED
Language=English
An initialization failure occurred while attempting to boot from the RAM disk.
.

MessageId=0xF9
Severity=Success
Facility=System
SymbolicName=DRIVER_RETURNED_STATUS_REPARSE_FOR_VOLUME_OPEN
Language=English
STATUS_REPARSE was returned from a FSD when trying to open a volume.
.

MessageId=0xFA
Severity=Success
Facility=System
SymbolicName=HTTP_DRIVER_CORRUPTED
Language=English
Corruption was detected in the HTTP kernel driver.
.

MessageId=0xFC
Severity=Success
Facility=System
SymbolicName=ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY
Language=English
An attempt was made to execute to non-executable memory.
.

MessageId=0xFD
Severity=Success
Facility=System
SymbolicName=DIRTY_NOWRITE_PAGES_CONGESTION
Language=English
DIRTY_NOWRITE_PAGES_CONGESTION
.

MessageId=0xFE
Severity=Success
Facility=System
SymbolicName=BUGCODE_USB_DRIVER
Language=English
A fatal error occurred in the USB driver stack.
.

MessageId=0x139
Severity=Success
Facility=System
SymbolicName=KERNEL_SECURITY_CHECK_FAILURE
Language=English
A critical kernel security check failed.
.
