;
; kernel32.mc MESSAGE resources for kernel32.dll
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

MessageId=0x00
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS
Language=English
ERROR_SUCCESS - The operation completed successfully.
.

MessageId=0x01
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FUNCTION
Language=English
ERROR_INVALID_FUNCTION - Incorrect function.
.

MessageId=0x02
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_NOT_FOUND
Language=English
ERROR_FILE_NOT_FOUND - The system cannot find the file specified.
.

MessageId=0x03
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_NOT_FOUND
Language=English
ERROR_PATH_NOT_FOUND - The system cannot find the path specified.
.

MessageId=0x04
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_OPEN_FILES
Language=English
ERROR_TOO_MANY_OPEN_FILES - The system cannot open the file.
.

MessageId=0x05
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DENIED
Language=English
ERROR_ACCESS_DENIED - Access is denied.
.

MessageId=0x06
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HANDLE
Language=English
ERROR_INVALID_HANDLE - The handle is invalid.
.

MessageId=0x07
Severity=Success
Facility=System
SymbolicName=ERROR_ARENA_TRASHED
Language=English
ERROR_ARENA_TRASHED - The storage control blocks were destroyed.
.

MessageId=0x08
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_MEMORY
Language=English
ERROR_NOT_ENOUGH_MEMORY - Not enough storage is available to process this command.
.

MessageId=0x09
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK
Language=English
ERROR_INVALID_BLOCK - The storage control block address is invalid.
.

MessageId=0x0A
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ENVIRONMENT
Language=English
ERROR_BAD_ENVIRONMENT - The environment is incorrect.
.

MessageId=0x0B
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_FORMAT
Language=English
ERROR_BAD_FORMAT - An attempt was made to load a program with an incorrect format.
.

MessageId=0x0C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCESS
Language=English
ERROR_INVALID_ACCESS - The access code is invalid.
.

MessageId=0x0D
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATA
Language=English
ERROR_INVALID_DATA - The data is invalid.
.

MessageId=0x0E
Severity=Success
Facility=System
SymbolicName=ERROR_OUTOFMEMORY
Language=English
ERROR_OUTOFMEMORY - Not enough storage is available to complete this operation.
.

MessageId=0x0F
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DRIVE
Language=English
ERROR_INVALID_DRIVE - The system cannot find the drive specified.
.

MessageId=0x10
Severity=Success
Facility=System
SymbolicName=ERROR_CURRENT_DIRECTORY
Language=English
ERROR_CURRENT_DIRECTORY - The directory cannot be removed.
.

MessageId=0x11
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAME_DEVICE
Language=English
ERROR_NOT_SAME_DEVICE - The system cannot move the file to a different disk drive.
.

MessageId=0x12
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_FILES
Language=English
ERROR_NO_MORE_FILES - There are no more files.
.

MessageId=0x13
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_PROTECT
Language=English
ERROR_WRITE_PROTECT - The media is write protected.
.

MessageId=0x14
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_UNIT
Language=English
ERROR_BAD_UNIT - The system cannot find the device specified.
.

MessageId=0x15
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_READY
Language=English
ERROR_NOT_READY - The device is not ready.
.

MessageId=0x16
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_COMMAND
Language=English
ERROR_BAD_COMMAND - The device does not recognize the command.
.

MessageId=0x17
Severity=Success
Facility=System
SymbolicName=ERROR_CRC
Language=English
ERROR_CRC - Data error (cyclic redundancy check).
.

MessageId=0x18
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LENGTH
Language=English
ERROR_BAD_LENGTH - The program issued a command but the command length is incorrect.
.

MessageId=0x19
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK
Language=English
ERROR_SEEK - The drive cannot locate a specific area or track on the disk.
.

MessageId=0x1A
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_DOS_DISK
Language=English
ERROR_NOT_DOS_DISK - The specified disk or diskette cannot be accessed.
.

MessageId=0x1B
Severity=Success
Facility=System
SymbolicName=ERROR_SECTOR_NOT_FOUND
Language=English
ERROR_SECTOR_NOT_FOUND - The drive cannot find the sector requested.
.

MessageId=0x1C
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_PAPER
Language=English
ERROR_OUT_OF_PAPER - The printer is out of paper.
.

MessageId=0x1D
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_FAULT
Language=English
ERROR_WRITE_FAULT - The system cannot write to the specified device.
.

MessageId=0x1E
Severity=Success
Facility=System
SymbolicName=ERROR_READ_FAULT
Language=English
ERROR_READ_FAULT - The system cannot read from the specified device.
.

MessageId=0x1F
Severity=Success
Facility=System
SymbolicName=ERROR_GEN_FAILURE
Language=English
ERROR_GEN_FAILURE - A device attached to the system is not functioning.
.

MessageId=0x20
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_VIOLATION
Language=English
ERROR_SHARING_VIOLATION - The process cannot access the file because it is being used by another process.
.

MessageId=0x21
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_VIOLATION
Language=English
ERROR_LOCK_VIOLATION - The process cannot access the file because another process has locked a portion of the file.
.

MessageId=0x22
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_DISK
Language=English
ERROR_WRONG_DISK
.

MessageId=0x24
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_BUFFER_EXCEEDED
Language=English
ERROR_SHARING_BUFFER_EXCEEDED
.

MessageId=0x26
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_EOF
Language=English
ERROR_HANDLE_EOF - Too many files opened for sharing.
.

MessageId=0x27
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_DISK_FULL
Language=English
ERROR_HANDLE_DISK_FULL
.

MessageId=0x32
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED
Language=English
ERROR_NOT_SUPPORTED - Reached the end of the file.
.

MessageId=0x33
Severity=Success
Facility=System
SymbolicName=ERROR_REM_NOT_LIST
Language=English
ERROR_REM_NOT_LIST - The disk is full.
.

MessageId=0x34
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_NAME
Language=English
ERROR_DUP_NAME
.

MessageId=0x35
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NETPATH
Language=English
ERROR_BAD_NETPATH
.

MessageId=0x36
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_BUSY
Language=English
ERROR_NETWORK_BUSY
.

MessageId=0x37
Severity=Success
Facility=System
SymbolicName=ERROR_DEV_NOT_EXIST
Language=English
ERROR_DEV_NOT_EXIST
.

MessageId=0x38
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CMDS
Language=English
ERROR_TOO_MANY_CMDS
.

MessageId=0x39
Severity=Success
Facility=System
SymbolicName=ERROR_ADAP_HDW_ERR
Language=English
ERROR_ADAP_HDW_ERR
.

MessageId=0x3A
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_RESP
Language=English
ERROR_BAD_NET_RESP
.

MessageId=0x3B
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXP_NET_ERR
Language=English
ERROR_UNEXP_NET_ERR
.

MessageId=0x3C
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_REM_ADAP
Language=English
ERROR_BAD_REM_ADAP
.

MessageId=0x3D
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTQ_FULL
Language=English
ERROR_PRINTQ_FULL
.

MessageId=0x3E
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SPOOL_SPACE
Language=English
ERROR_NO_SPOOL_SPACE - The network request is not supported.
.

MessageId=0x3F
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_CANCELLED
Language=English
ERROR_PRINT_CANCELLED - The remote computer is not available.
.

MessageId=0x40
Severity=Success
Facility=System
SymbolicName=ERROR_NETNAME_DELETED
Language=English
ERROR_NETNAME_DELETED - A duplicate name exists on the network.
.

MessageId=0x41
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_ACCESS_DENIED
Language=English
ERROR_NETWORK_ACCESS_DENIED - The network path was not found.
.

MessageId=0x42
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEV_TYPE
Language=English
ERROR_BAD_DEV_TYPE - The network is busy.
.

MessageId=0x43
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_NAME
Language=English
ERROR_BAD_NET_NAME - The specified network resource or device is no longer available.
.

MessageId=0x44
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_NAMES
Language=English
ERROR_TOO_MANY_NAMES - The network BIOS command limit has been reached.
.

MessageId=0x45
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SESS
Language=English
ERROR_TOO_MANY_SESS - A network adapter hardware error occurred.
.

MessageId=0x46
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_PAUSED
Language=English
ERROR_SHARING_PAUSED - The specified server cannot perform the requested operation.
.

MessageId=0x47
Severity=Success
Facility=System
SymbolicName=ERROR_REQ_NOT_ACCEP
Language=English
ERROR_REQ_NOT_ACCEP - An unexpected network error occurred.
.

MessageId=0x48
Severity=Success
Facility=System
SymbolicName=ERROR_REDIR_PAUSED
Language=English
ERROR_REDIR_PAUSED - The remote adapter is not compatible.
.

MessageId=0x50
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_EXISTS
Language=English
ERROR_FILE_EXISTS - The printer queue is full.
.

MessageId=0x52
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_MAKE
Language=English
ERROR_CANNOT_MAKE - Space to store the file waiting to be printed is not available on the server.
.

MessageId=0x53
Severity=Success
Facility=System
SymbolicName=ERROR_FAIL_I24
Language=English
ERROR_FAIL_I24 - Your file waiting to be printed was deleted.
.

MessageId=0x54
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_STRUCTURES
Language=English
ERROR_OUT_OF_STRUCTURES - The specified network name is no longer available.
.

MessageId=0x55
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_ASSIGNED
Language=English
ERROR_ALREADY_ASSIGNED - Network access is denied.
.

MessageId=0x56
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORD
Language=English
ERROR_INVALID_PASSWORD - The network resource type is not correct.
.

MessageId=0x57
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PARAMETER
Language=English
ERROR_INVALID_PARAMETER - The network name cannot be found.
.

MessageId=0x58
Severity=Success
Facility=System
SymbolicName=ERROR_NET_WRITE_FAULT
Language=English
ERROR_NET_WRITE_FAULT - The name limit for the local computer network adapter card was exceeded.
.

MessageId=0x59
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PROC_SLOTS
Language=English
ERROR_NO_PROC_SLOTS - The network BIOS session limit was exceeded.
.

MessageId=0x64
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEMAPHORES
Language=English
ERROR_TOO_MANY_SEMAPHORES - The remote server has been paused or is in the process of being started.
.

MessageId=0x65
Severity=Success
Facility=System
SymbolicName=ERROR_EXCL_SEM_ALREADY_OWNED
Language=English
ERROR_EXCL_SEM_ALREADY_OWNED - No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.
.

MessageId=0x66
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_IS_SET
Language=English
ERROR_SEM_IS_SET - The specified printer or disk device has been paused.
.

MessageId=0x67
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEM_REQUESTS
Language=English
ERROR_TOO_MANY_SEM_REQUESTS
.

MessageId=0x68
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_AT_INTERRUPT_TIME
Language=English
ERROR_INVALID_AT_INTERRUPT_TIME
.

MessageId=0x69
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_OWNER_DIED
Language=English
ERROR_SEM_OWNER_DIED
.

MessageId=0x6A
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_USER_LIMIT
Language=English
ERROR_SEM_USER_LIMIT
.

MessageId=0x6B
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CHANGE
Language=English
ERROR_DISK_CHANGE
.

MessageId=0x6C
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVE_LOCKED
Language=English
ERROR_DRIVE_LOCKED
.

MessageId=0x6D
Severity=Success
Facility=System
SymbolicName=ERROR_BROKEN_PIPE
Language=English
ERROR_BROKEN_PIPE
.

MessageId=0x6E
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FAILED
Language=English
ERROR_OPEN_FAILED - The file exists.
.

MessageId=0x6F
Severity=Success
Facility=System
SymbolicName=ERROR_BUFFER_OVERFLOW
Language=English
ERROR_BUFFER_OVERFLOW
.

MessageId=0x70
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_FULL
Language=English
ERROR_DISK_FULL - The directory or file cannot be created.
.

MessageId=0x71
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_SEARCH_HANDLES
Language=English
ERROR_NO_MORE_SEARCH_HANDLES - Fail on INT 24.
.

MessageId=0x72
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TARGET_HANDLE
Language=English
ERROR_INVALID_TARGET_HANDLE - Storage to process this request is not available.
.

MessageId=0x75
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CATEGORY
Language=English
ERROR_INVALID_CATEGORY - The local device name is already in use.
.

MessageId=0x76
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_VERIFY_SWITCH
Language=English
ERROR_INVALID_VERIFY_SWITCH - The specified network password is not correct.
.

MessageId=0x77
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER_LEVEL
Language=English
ERROR_BAD_DRIVER_LEVEL - The parameter is incorrect.
.

MessageId=0x78
Severity=Success
Facility=System
SymbolicName=ERROR_CALL_NOT_IMPLEMENTED
Language=English
ERROR_CALL_NOT_IMPLEMENTED - A write fault occurred on the network.
.

MessageId=0x79
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_TIMEOUT
Language=English
ERROR_SEM_TIMEOUT - The system cannot start another process at this time.
.

MessageId=0x7A
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_BUFFER
Language=English
ERROR_INSUFFICIENT_BUFFER
.

MessageId=0x7B
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NAME
Language=English
ERROR_INVALID_NAME
.

MessageId=0x7C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LEVEL
Language=English
ERROR_INVALID_LEVEL
.

MessageId=0x7D
Severity=Success
Facility=System
SymbolicName=ERROR_NO_VOLUME_LABEL
Language=English
ERROR_NO_VOLUME_LABEL
.

MessageId=0x7E
Severity=Success
Facility=System
SymbolicName=ERROR_MOD_NOT_FOUND
Language=English
ERROR_MOD_NOT_FOUND
.

MessageId=0x7F
Severity=Success
Facility=System
SymbolicName=ERROR_PROC_NOT_FOUND
Language=English
ERROR_PROC_NOT_FOUND
.

MessageId=0x80
Severity=Success
Facility=System
SymbolicName=ERROR_WAIT_NO_CHILDREN
Language=English
ERROR_WAIT_NO_CHILDREN
.

MessageId=0x81
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_NOT_COMPLETE
Language=English
ERROR_CHILD_NOT_COMPLETE
.

MessageId=0x82
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECT_ACCESS_HANDLE
Language=English
ERROR_DIRECT_ACCESS_HANDLE
.

MessageId=0x83
Severity=Success
Facility=System
SymbolicName=ERROR_NEGATIVE_SEEK
Language=English
ERROR_NEGATIVE_SEEK
.

MessageId=0x84
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK_ON_DEVICE
Language=English
ERROR_SEEK_ON_DEVICE - Cannot create another system semaphore.
.

MessageId=0x85
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_TARGET
Language=English
ERROR_IS_JOIN_TARGET - The exclusive semaphore is owned by another process.
.

MessageId=0x86
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOINED
Language=English
ERROR_IS_JOINED - The semaphore is set and cannot be closed.
.

MessageId=0x87
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBSTED
Language=English
ERROR_IS_SUBSTED - The semaphore cannot be set again.
.

MessageId=0x88
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_JOINED
Language=English
ERROR_NOT_JOINED - Cannot request exclusive semaphores at interrupt time.
.

MessageId=0x89
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUBSTED
Language=English
ERROR_NOT_SUBSTED - The previous ownership of this semaphore has ended.
.

MessageId=0x8A
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_JOIN
Language=English
ERROR_JOIN_TO_JOIN
.

MessageId=0x8B
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_SUBST
Language=English
ERROR_SUBST_TO_SUBST - The program stopped because an alternate diskette was not inserted.
.

MessageId=0x8C
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_SUBST
Language=English
ERROR_JOIN_TO_SUBST - The disk is in use or locked by
another process.
.

MessageId=0x8D
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_JOIN
Language=English
ERROR_SUBST_TO_JOIN - The pipe has been ended.
.

MessageId=0x8E
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY_DRIVE
Language=English
ERROR_BUSY_DRIVE - The system cannot open the
device or file specified.
.

MessageId=0x8F
Severity=Success
Facility=System
SymbolicName=ERROR_SAME_DRIVE
Language=English
ERROR_SAME_DRIVE - The file name is too long.
.

MessageId=0x90
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_ROOT
Language=English
ERROR_DIR_NOT_ROOT - There is not enough space on the disk.
.

MessageId=0x91
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_EMPTY
Language=English
ERROR_DIR_NOT_EMPTY - No more internal file identifiers available.
.

MessageId=0x92
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_PATH
Language=English
ERROR_IS_SUBST_PATH - The target internal file identifier is incorrect.
.

MessageId=0x93
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_PATH
Language=English
ERROR_IS_JOIN_PATH
.

MessageId=0x94
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_BUSY
Language=English
ERROR_PATH_BUSY
.

MessageId=0x95
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_TARGET
Language=English
ERROR_IS_SUBST_TARGET - The IOCTL call made by the application program is not correct.
.

MessageId=0x96
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_TRACE
Language=English
ERROR_SYSTEM_TRACE - The verify-on-write switch parameter value is not correct.
.

MessageId=0x97
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENT_COUNT
Language=English
ERROR_INVALID_EVENT_COUNT - The system does not support the command requested.
.

MessageId=0x98
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MUXWAITERS
Language=English
ERROR_TOO_MANY_MUXWAITERS - This function is not supported on this system.
.

MessageId=0x99
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LIST_FORMAT
Language=English
ERROR_INVALID_LIST_FORMAT - The semaphore timeout period has expired.
.

MessageId=0x9A
Severity=Success
Facility=System
SymbolicName=ERROR_LABEL_TOO_LONG
Language=English
ERROR_LABEL_TOO_LONG - The data area passed to a system call is too small.
.

MessageId=0x9B
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_TCBS
Language=English
ERROR_TOO_MANY_TCBS - The filename, directory name, or volume label syntax is incorrect.
.

MessageId=0x9C
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_REFUSED
Language=English
ERROR_SIGNAL_REFUSED - The system call level is not correct.
.

MessageId=0x9D
Severity=Success
Facility=System
SymbolicName=ERROR_DISCARDED
Language=English
ERROR_DISCARDED - The disk has no volume label.
.

MessageId=0x9E
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOCKED
Language=English
ERROR_NOT_LOCKED - The specified module could not be found.
.

MessageId=0x9F
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_THREADID_ADDR
Language=English
ERROR_BAD_THREADID_ADDR - The specified procedure could not be found.
.

MessageId=0xA0
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ARGUMENTS
Language=English
ERROR_BAD_ARGUMENTS - There are no child processes to wait for.
.

MessageId=0xA1
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PATHNAME
Language=English
ERROR_BAD_PATHNAME
.

MessageId=0xA2
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_PENDING
Language=English
ERROR_SIGNAL_PENDING - Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.
.

MessageId=0xA4
Severity=Success
Facility=System
SymbolicName=ERROR_MAX_THRDS_REACHED
Language=English
ERROR_MAX_THRDS_REACHED - An attempt was made to move the file pointer before the beginning of the file.
.

MessageId=0xA7
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_FAILED
Language=English
ERROR_LOCK_FAILED - The file pointer cannot be set on the specified device or file.
.

MessageId=0xAA
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY
Language=English
ERROR_BUSY - A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.
.

MessageId=0xAD
Severity=Success
Facility=System
SymbolicName=ERROR_CANCEL_VIOLATION
Language=English
ERROR_CANCEL_VIOLATION - An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.
.

MessageId=0xAE
Severity=Success
Facility=System
SymbolicName=ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
Language=English
ERROR_ATOMIC_LOCKS_NOT_SUPPORTED - An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.
.

MessageId=0xB4
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGMENT_NUMBER
Language=English
ERROR_INVALID_SEGMENT_NUMBER - The system tried to delete the JOIN of a drive that is not joined.
.

MessageId=0xB6
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ORDINAL
Language=English
ERROR_INVALID_ORDINAL - The system tried to delete the substitution of a drive that is not substituted.
.

MessageId=0xB7
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_EXISTS
Language=English
ERROR_ALREADY_EXISTS - The system tried to join a drive to a directory on a joined drive.
.

MessageId=0xBA
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAG_NUMBER
Language=English
ERROR_INVALID_FLAG_NUMBER - The system tried to substitute a drive to a directory on a substituted drive.
.

MessageId=0xBB
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_NOT_FOUND
Language=English
ERROR_SEM_NOT_FOUND - The system tried to join a drive to a directory on a substituted drive.
.

MessageId=0xBC
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STARTING_CODESEG
Language=English
ERROR_INVALID_STARTING_CODESEG - The system tried to SUBST a drive to a directory on a joined drive.
.

MessageId=0xBD
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STACKSEG
Language=English
ERROR_INVALID_STACKSEG - The system cannot perform a JOIN or SUBST at this time.
.

MessageId=0xBE
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MODULETYPE
Language=English
ERROR_INVALID_MODULETYPE - The system cannot join or substitute a drive to or for a directory on the same drive.
.

MessageId=0xBF
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EXE_SIGNATURE
Language=English
ERROR_INVALID_EXE_SIGNATURE - The directory is not a subdirectory of the root directory.
.

MessageId=0xC0
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_MARKED_INVALID
Language=English
ERROR_EXE_MARKED_INVALID - The directory is not empty.
.

MessageId=0xC1
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_EXE_FORMAT
Language=English
ERROR_BAD_EXE_FORMAT - The path specified is being used in a substitute.
.

MessageId=0xC2
Severity=Success
Facility=System
SymbolicName=ERROR_ITERATED_DATA_EXCEEDS_64k
Language=English
ERROR_ITERATED_DATA_EXCEEDS_64k - Not enough resources are available to process this command.
.

MessageId=0xC3
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MINALLOCSIZE
Language=English
ERROR_INVALID_MINALLOCSIZE - The path specified cannot be used at this time.
.

MessageId=0xC4
Severity=Success
Facility=System
SymbolicName=ERROR_DYNLINK_FROM_INVALID_RING
Language=English
ERROR_DYNLINK_FROM_INVALID_RING - An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.
.

MessageId=0xC5
Severity=Success
Facility=System
SymbolicName=ERROR_IOPL_NOT_ENABLED
Language=English
ERROR_IOPL_NOT_ENABLED - System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.
.

MessageId=0xC6
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGDPL
Language=English
ERROR_INVALID_SEGDPL - The number of specified semaphore events for DosMuxSemWait is not correct.
.

MessageId=0xC7
Severity=Success
Facility=System
SymbolicName=ERROR_AUTODATASEG_EXCEEDS_64k
Language=English
ERROR_AUTODATASEG_EXCEEDS_64k - DosMuxSemWait did not execute; too many semaphores are already set.
.

MessageId=0xC8
Severity=Success
Facility=System
SymbolicName=ERROR_RING2SEG_MUST_BE_MOVABLE
Language=English
ERROR_RING2SEG_MUST_BE_MOVABLE - The DosMuxSemWait list is not correct.
.

MessageId=0xC9
Severity=Success
Facility=System
SymbolicName=ERROR_RELOC_CHAIN_XEEDS_SEGLIM
Language=English
ERROR_RELOC_CHAIN_XEEDS_SEGLIM - The volume label you entered exceeds the label character
limit of the target file system.
.

MessageId=0xCA
Severity=Success
Facility=System
SymbolicName=ERROR_INFLOOP_IN_RELOC_CHAIN
Language=English
ERROR_INFLOOP_IN_RELOC_CHAIN - Cannot create another thread.
.

MessageId=0xCB
Severity=Success
Facility=System
SymbolicName=ERROR_ENVVAR_NOT_FOUND
Language=English
ERROR_ENVVAR_NOT_FOUND - The recipient process has refused the signal.
.

MessageId=0xCD
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SIGNAL_SENT
Language=English
ERROR_NO_SIGNAL_SENT - The segment is already discarded and cannot be locked.
.

MessageId=0xCE
Severity=Success
Facility=System
SymbolicName=ERROR_FILENAME_EXCED_RANGE
Language=English
ERROR_FILENAME_EXCED_RANGE - The segment is already unlocked.
.

MessageId=0xCF
Severity=Success
Facility=System
SymbolicName=ERROR_RING2_STACK_IN_USE
Language=English
ERROR_RING2_STACK_IN_USE - The address for the thread ID is not correct.
.

MessageId=0xD0
Severity=Success
Facility=System
SymbolicName=ERROR_META_EXPANSION_TOO_LONG
Language=English
ERROR_META_EXPANSION_TOO_LONG - The argument string passed to DosExecPgm is not correct.
.

MessageId=0xD1
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SIGNAL_NUMBER
Language=English
ERROR_INVALID_SIGNAL_NUMBER - The specified path is invalid.
.

MessageId=0xD2
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_1_INACTIVE
Language=English
ERROR_THREAD_1_INACTIVE - A signal is already pending.
.

MessageId=0xD4
Severity=Success
Facility=System
SymbolicName=ERROR_LOCKED
Language=English
ERROR_LOCKED
.

MessageId=0xD6
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MODULES
Language=English
ERROR_TOO_MANY_MODULES - No more threads can be created in the system.
.

MessageId=0xD7
Severity=Success
Facility=System
SymbolicName=ERROR_NESTING_NOT_ALLOWED
Language=English
ERROR_NESTING_NOT_ALLOWED
.

MessageId=0xE6
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PIPE
Language=English
ERROR_BAD_PIPE
.

MessageId=0xE7
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_BUSY
Language=English
ERROR_PIPE_BUSY - Unable to lock a region of a file.
.

MessageId=0xE8
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA
Language=English
ERROR_NO_DATA
.

MessageId=0xE9
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_NOT_CONNECTED
Language=English
ERROR_PIPE_NOT_CONNECTED
.

MessageId=0xEA
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_DATA
Language=English
ERROR_MORE_DATA - The requested resource is in use.
.

MessageId=0xF0
Severity=Success
Facility=System
SymbolicName=ERROR_VC_DISCONNECTED
Language=English
ERROR_VC_DISCONNECTED
.

MessageId=0xFE
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_NAME
Language=English
ERROR_INVALID_EA_NAME
.

MessageId=0xFF
Severity=Success
Facility=System
SymbolicName=ERROR_EA_LIST_INCONSISTENT
Language=English
ERROR_EA_LIST_INCONSISTENT - A lock request was not outstanding for the supplied cancel region.
.

MessageId=0x103
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_ITEMS
Language=English
ERROR_NO_MORE_ITEMS - The file system does not support atomic changes to the lock type.
.

MessageId=0x10A
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_COPY
Language=English
ERROR_CANNOT_COPY
.

MessageId=0x10B
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECTORY
Language=English
ERROR_DIRECTORY
.

MessageId=0x113
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_DIDNT_FIT
Language=English
ERROR_EAS_DIDNT_FIT
.

MessageId=0x114
Severity=Success
Facility=System
SymbolicName=ERROR_EA_FILE_CORRUPT
Language=English
ERROR_EA_FILE_CORRUPT
.

MessageId=0x115
Severity=Success
Facility=System
SymbolicName=ERROR_EA_TABLE_FULL
Language=English
ERROR_EA_TABLE_FULL
.

MessageId=0x116
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_HANDLE
Language=English
ERROR_INVALID_EA_HANDLE - The system detected a segment number that was not correct.
.

MessageId=0x11A
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_NOT_SUPPORTED
Language=English
ERROR_EAS_NOT_SUPPORTED
.

MessageId=0x120
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_OWNER
Language=English
ERROR_NOT_OWNER
.

MessageId=0x12A
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_POSTS
Language=English
ERROR_TOO_MANY_POSTS - Cannot create a file when that file already exists.
.

MessageId=0x12B
Severity=Success
Facility=System
SymbolicName=ERROR_PARTIAL_COPY
Language=English
ERROR_PARTIAL_COPY
.

MessageId=0x13D
Severity=Success
Facility=System
SymbolicName=ERROR_MR_MID_NOT_FOUND
Language=English
ERROR_MR_MID_NOT_FOUND
.

MessageId=0x1E7
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ADDRESS
Language=English
ERROR_INVALID_ADDRESS - The flag passed is not correct.
.

MessageId=0x216
Severity=Success
Facility=System
SymbolicName=ERROR_ARITHMETIC_OVERFLOW
Language=English
ERROR_ARITHMETIC_OVERFLOW - The specified system semaphore name was not found.
.

MessageId=0x217
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_CONNECTED
Language=English
ERROR_PIPE_CONNECTED
.

MessageId=0x218
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_LISTENING
Language=English
ERROR_PIPE_LISTENING
.

MessageId=0x3E2
Severity=Success
Facility=System
SymbolicName=ERROR_EA_ACCESS_DENIED
Language=English
ERROR_EA_ACCESS_DENIED
.

MessageId=0x3E3
Severity=Success
Facility=System
SymbolicName=ERROR_OPERATION_ABORTED
Language=English
ERROR_OPERATION_ABORTED
.

MessageId=0x3E4
Severity=Success
Facility=System
SymbolicName=ERROR_IO_INCOMPLETE
Language=English
ERROR_IO_INCOMPLETE
.

MessageId=0x3E5
Severity=Success
Facility=System
SymbolicName=ERROR_IO_PENDING
Language=English
ERROR_IO_PENDING
.

MessageId=0x3E6
Severity=Success
Facility=System
SymbolicName=ERROR_NOACCESS
Language=English
ERROR_NOACCESS
.

MessageId=0x3E7
Severity=Success
Facility=System
SymbolicName=ERROR_SWAPERROR
Language=English
ERROR_SWAPERROR
.

MessageId=0x3E9
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_OVERFLOW
Language=English
ERROR_STACK_OVERFLOW - The operating system cannot run this application program.
.

MessageId=0x3EA
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGE
Language=English
ERROR_INVALID_MESSAGE - The operating system is not presently configured to run this application.
.

MessageId=0x3EB
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_COMPLETE
Language=English
ERROR_CAN_NOT_COMPLETE
.

MessageId=0x3EC
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAGS
Language=English
ERROR_INVALID_FLAGS - The operating system cannot run this application program.
.

MessageId=0x3ED
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_VOLUME
Language=English
ERROR_UNRECOGNIZED_VOLUME - The code segment cannot be greater than or equal to 64K.
.

MessageId=0x3EE
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_INVALID
Language=English
ERROR_FILE_INVALID
.

MessageId=0x3EF
Severity=Success
Facility=System
SymbolicName=ERROR_FULLSCREEN_MODE
Language=English
ERROR_FULLSCREEN_MODE
.

MessageId=0x3F0
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TOKEN
Language=English
ERROR_NO_TOKEN - The system could not find the environment
option that was entered.
.

MessageId=0x3F1
Severity=Success
Facility=System
SymbolicName=ERROR_BADDB
Language=English
ERROR_BADDB
.

MessageId=0x3F2
Severity=Success
Facility=System
SymbolicName=ERROR_BADKEY
Language=English
ERROR_BADKEY - No process in the command subtree has a
signal handler.
.

MessageId=0x3F3
Severity=Success
Facility=System
SymbolicName=ERROR_CANTOPEN
Language=English
ERROR_CANTOPEN - The filename or extension is too long.
.

MessageId=0x3F4
Severity=Success
Facility=System
SymbolicName=ERROR_CANTREAD
Language=English
ERROR_CANTREAD - The ring 2 stack is in use.
.

MessageId=0x3F5
Severity=Success
Facility=System
SymbolicName=ERROR_CANTWRITE
Language=English
ERROR_CANTWRITE - The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.
.

MessageId=0x3F6
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_RECOVERED
Language=English
ERROR_REGISTRY_RECOVERED - The signal being posted is not correct.
.

MessageId=0x3F7
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_CORRUPT
Language=English
ERROR_REGISTRY_CORRUPT - The signal handler cannot be set.
.

MessageId=0x3F8
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_IO_FAILED
Language=English
ERROR_REGISTRY_IO_FAILED
.

MessageId=0x3F9
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_REGISTRY_FILE
Language=English
ERROR_NOT_REGISTRY_FILE - The segment is locked and cannot be reallocated.
.

MessageId=0x3FA
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_DELETED
Language=English
ERROR_KEY_DELETED
.

MessageId=0x3FB
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOG_SPACE
Language=English
ERROR_NO_LOG_SPACE - Too many dynamic-link modules are attached to this program or dynamic-link module.
.

MessageId=0x3FC
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_HAS_CHILDREN
Language=English
ERROR_KEY_HAS_CHILDREN - Cannot nest calls to LoadModule.
.

MessageId=0x3FD
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_MUST_BE_VOLATILE
Language=English
ERROR_CHILD_MUST_BE_VOLATILE
.

MessageId=0x3FE
Severity=Success
Facility=System
SymbolicName=ERROR_NOTIFY_ENUM_DIR
Language=English
ERROR_NOTIFY_ENUM_DIR
.

MessageId=0x41B
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENT_SERVICES_RUNNING
Language=English
ERROR_DEPENDENT_SERVICES_RUNNING
.

MessageId=0x41C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_CONTROL
Language=English
ERROR_INVALID_SERVICE_CONTROL
.

MessageId=0x41D
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_REQUEST_TIMEOUT
Language=English
ERROR_SERVICE_REQUEST_TIMEOUT
.

MessageId=0x41E
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NO_THREAD
Language=English
ERROR_SERVICE_NO_THREAD
.

MessageId=0x41F
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DATABASE_LOCKED
Language=English
ERROR_SERVICE_DATABASE_LOCKED
.

MessageId=0x420
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_ALREADY_RUNNING
Language=English
ERROR_SERVICE_ALREADY_RUNNING
.

MessageId=0x421
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_ACCOUNT
Language=English
ERROR_INVALID_SERVICE_ACCOUNT
.

MessageId=0x422
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DISABLED
Language=English
ERROR_SERVICE_DISABLED
.

MessageId=0x423
Severity=Success
Facility=System
SymbolicName=ERROR_CIRCULAR_DEPENDENCY
Language=English
ERROR_CIRCULAR_DEPENDENCY
.

MessageId=0x424
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DOES_NOT_EXIST
Language=English
ERROR_SERVICE_DOES_NOT_EXIST
.

MessageId=0x425
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_CANNOT_ACCEPT_CTRL
Language=English
ERROR_SERVICE_CANNOT_ACCEPT_CTRL
.

MessageId=0x426
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_ACTIVE
Language=English
ERROR_SERVICE_NOT_ACTIVE
.

MessageId=0x427
Severity=Success
Facility=System
SymbolicName=ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
Language=English
ERROR_FAILED_SERVICE_CONTROLLER_CONNECT - The pipe state is invalid.
.

MessageId=0x428
Severity=Success
Facility=System
SymbolicName=ERROR_EXCEPTION_IN_SERVICE
Language=English
ERROR_EXCEPTION_IN_SERVICE - All pipe instances are busy.
.

MessageId=0x429
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_DOES_NOT_EXIST
Language=English
ERROR_DATABASE_DOES_NOT_EXIST - The pipe is being closed.
.

MessageId=0x42A
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_SPECIFIC_ERROR
Language=English
ERROR_SERVICE_SPECIFIC_ERROR - No process is on the other end of the pipe.
.

MessageId=0x42B
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_ABORTED
Language=English
ERROR_PROCESS_ABORTED - More data is available.
.

MessageId=0x42C
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_FAIL
Language=English
ERROR_SERVICE_DEPENDENCY_FAIL
.

MessageId=0x42D
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_LOGON_FAILED
Language=English
ERROR_SERVICE_LOGON_FAILED
.

MessageId=0x42E
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_START_HANG
Language=English
ERROR_SERVICE_START_HANG
.

MessageId=0x42F
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_LOCK
Language=English
ERROR_INVALID_SERVICE_LOCK
.

MessageId=0x430
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_MARKED_FOR_DELETE
Language=English
ERROR_SERVICE_MARKED_FOR_DELETE
.

MessageId=0x431
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_EXISTS
Language=English
ERROR_SERVICE_EXISTS - The session was canceled.
.

MessageId=0x432
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_RUNNING_LKG
Language=English
ERROR_ALREADY_RUNNING_LKG
.

MessageId=0x433
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_DELETED
Language=English
ERROR_SERVICE_DEPENDENCY_DELETED
.

MessageId=0x434
Severity=Success
Facility=System
SymbolicName=ERROR_BOOT_ALREADY_ACCEPTED
Language=English
ERROR_BOOT_ALREADY_ACCEPTED
.

MessageId=0x435
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NEVER_STARTED
Language=English
ERROR_SERVICE_NEVER_STARTED
.

MessageId=0x436
Severity=Success
Facility=System
SymbolicName=ERROR_DUPLICATE_SERVICE_NAME
Language=English
ERROR_DUPLICATE_SERVICE_NAME
.

MessageId=0x44C
Severity=Success
Facility=System
SymbolicName=ERROR_END_OF_MEDIA
Language=English
ERROR_END_OF_MEDIA
.

MessageId=0x44D
Severity=Success
Facility=System
SymbolicName=ERROR_FILEMARK_DETECTED
Language=English
ERROR_FILEMARK_DETECTED
.

MessageId=0x44E
Severity=Success
Facility=System
SymbolicName=ERROR_BEGINNING_OF_MEDIA
Language=English
ERROR_BEGINNING_OF_MEDIA
.

MessageId=0x44F
Severity=Success
Facility=System
SymbolicName=ERROR_SETMARK_DETECTED
Language=English
ERROR_SETMARK_DETECTED
.

MessageId=0x450
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA_DETECTED
Language=English
ERROR_NO_DATA_DETECTED
.

MessageId=0x451
Severity=Success
Facility=System
SymbolicName=ERROR_PARTITION_FAILURE
Language=English
ERROR_PARTITION_FAILURE
.

MessageId=0x452
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK_LENGTH
Language=English
ERROR_INVALID_BLOCK_LENGTH
.

MessageId=0x453
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_PARTITIONED
Language=English
ERROR_DEVICE_NOT_PARTITIONED
.

MessageId=0x454
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_LOCK_MEDIA
Language=English
ERROR_UNABLE_TO_LOCK_MEDIA - The specified extended attribute name was invalid.
.

MessageId=0x455
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_UNLOAD_MEDIA
Language=English
ERROR_UNABLE_TO_UNLOAD_MEDIA - The extended attributes are inconsistent.
.

MessageId=0x456
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_CHANGED
Language=English
ERROR_MEDIA_CHANGED
.

MessageId=0x457
Severity=Success
Facility=System
SymbolicName=ERROR_BUS_RESET
Language=English
ERROR_BUS_RESET
.

MessageId=0x458
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MEDIA_IN_DRIVE
Language=English
ERROR_NO_MEDIA_IN_DRIVE - The wait operation timed out.
.

MessageId=0x459
Severity=Success
Facility=System
SymbolicName=ERROR_NO_UNICODE_TRANSLATION
Language=English
ERROR_NO_UNICODE_TRANSLATION - No more data is available.
.

MessageId=0x45A
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_INIT_FAILED
Language=English
ERROR_DLL_INIT_FAILED
.

MessageId=0x45B
Severity=Success
Facility=System
SymbolicName=ERROR_SHUTDOWN_IN_PROGRESS
Language=English
ERROR_SHUTDOWN_IN_PROGRESS
.

MessageId=0x45C
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SHUTDOWN_IN_PROGRESS
Language=English
ERROR_NO_SHUTDOWN_IN_PROGRESS
.

MessageId=0x45D
Severity=Success
Facility=System
SymbolicName=ERROR_IO_DEVICE
Language=English
ERROR_IO_DEVICE
.

MessageId=0x45E
Severity=Success
Facility=System
SymbolicName=ERROR_SERIAL_NO_DEVICE
Language=English
ERROR_SERIAL_NO_DEVICE
.

MessageId=0x45F
Severity=Success
Facility=System
SymbolicName=ERROR_IRQ_BUSY
Language=English
ERROR_IRQ_BUSY
.

MessageId=0x460
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_WRITES
Language=English
ERROR_MORE_WRITES - The copy functions cannot be used.
.

MessageId=0x461
Severity=Success
Facility=System
SymbolicName=ERROR_COUNTER_TIMEOUT
Language=English
ERROR_COUNTER_TIMEOUT - The directory name is invalid.
.

MessageId=0x462
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_ID_MARK_NOT_FOUND
Language=English
ERROR_FLOPPY_ID_MARK_NOT_FOUND
.

MessageId=0x463
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_WRONG_CYLINDER
Language=English
ERROR_FLOPPY_WRONG_CYLINDER
.

MessageId=0x464
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_UNKNOWN_ERROR
Language=English
ERROR_FLOPPY_UNKNOWN_ERROR
.

MessageId=0x465
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_BAD_REGISTERS
Language=English
ERROR_FLOPPY_BAD_REGISTERS
.

MessageId=0x466
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RECALIBRATE_FAILED
Language=English
ERROR_DISK_RECALIBRATE_FAILED
.

MessageId=0x467
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_OPERATION_FAILED
Language=English
ERROR_DISK_OPERATION_FAILED
.

MessageId=0x468
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RESET_FAILED
Language=English
ERROR_DISK_RESET_FAILED
.

MessageId=0x469
Severity=Success
Facility=System
SymbolicName=ERROR_EOM_OVERFLOW
Language=English
ERROR_EOM_OVERFLOW - The extended attributes did not fit in the buffer.
.

MessageId=0x46A
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_SERVER_MEMORY
Language=English
ERROR_NOT_ENOUGH_SERVER_MEMORY - The extended attribute file on the mounted file system is corrupt.
.

MessageId=0x46B
Severity=Success
Facility=System
SymbolicName=ERROR_POSSIBLE_DEADLOCK
Language=English
ERROR_POSSIBLE_DEADLOCK - The extended attribute table file is full.
.

MessageId=0x46C
Severity=Success
Facility=System
SymbolicName=ERROR_MAPPED_ALIGNMENT
Language=English
ERROR_MAPPED_ALIGNMENT - The specified extended attribute handle is invalid.
.

MessageId=0x474
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_VETOED
Language=English
ERROR_SET_POWER_STATE_VETOED
.

MessageId=0x475
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_FAILED
Language=English
ERROR_SET_POWER_STATE_FAILED
.

MessageId=0x476
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LINKS
Language=English
ERROR_TOO_MANY_LINKS
.

MessageId=0x47E
Severity=Success
Facility=System
SymbolicName=ERROR_OLD_WIN_VERSION
Language=English
ERROR_OLD_WIN_VERSION - The mounted file system does not support extended attributes.
.

MessageId=0x47F
Severity=Success
Facility=System
SymbolicName=ERROR_APP_WRONG_OS
Language=English
ERROR_APP_WRONG_OS
.

MessageId=0x480
Severity=Success
Facility=System
SymbolicName=ERROR_SINGLE_INSTANCE_APP
Language=English
ERROR_SINGLE_INSTANCE_APP
.

MessageId=0x481
Severity=Success
Facility=System
SymbolicName=ERROR_RMODE_APP
Language=English
ERROR_RMODE_APP
.

MessageId=0x482
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DLL
Language=English
ERROR_INVALID_DLL
.

MessageId=0x483
Severity=Success
Facility=System
SymbolicName=ERROR_NO_ASSOCIATION
Language=English
ERROR_NO_ASSOCIATION
.

MessageId=0x484
Severity=Success
Facility=System
SymbolicName=ERROR_DDE_FAIL
Language=English
ERROR_DDE_FAIL - Attempt to release mutex not owned by caller.
.

MessageId=0x485
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_NOT_FOUND
Language=English
ERROR_DLL_NOT_FOUND
.

MessageId=0x4B0
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEVICE
Language=English
ERROR_BAD_DEVICE
.

MessageId=0x4B1
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_UNAVAIL
Language=English
ERROR_CONNECTION_UNAVAIL
.

MessageId=0x4B2
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ALREADY_REMEMBERED
Language=English
ERROR_DEVICE_ALREADY_REMEMBERED
.

MessageId=0x4B3
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NET_OR_BAD_PATH
Language=English
ERROR_NO_NET_OR_BAD_PATH
.

MessageId=0x4B4
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROVIDER
Language=English
ERROR_BAD_PROVIDER
.

MessageId=0x4B5
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_OPEN_PROFILE
Language=English
ERROR_CANNOT_OPEN_PROFILE
.

MessageId=0x4B6
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROFILE
Language=English
ERROR_BAD_PROFILE
.

MessageId=0x4B7
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONTAINER
Language=English
ERROR_NOT_CONTAINER
.

MessageId=0x4B8
Severity=Success
Facility=System
SymbolicName=ERROR_EXTENDED_ERROR
Language=English
ERROR_EXTENDED_ERROR - Too many posts were made to a semaphore.
.

MessageId=0x4B9
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUPNAME
Language=English
ERROR_INVALID_GROUPNAME - Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
.

MessageId=0x4BA
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMPUTERNAME
Language=English
ERROR_INVALID_COMPUTERNAME - The oplock request is denied.
.

MessageId=0x4BB
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENTNAME
Language=English
ERROR_INVALID_EVENTNAME - An invalid oplock acknowledgment was received by the system.
.

MessageId=0x4BC
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAINNAME
Language=English
ERROR_INVALID_DOMAINNAME
.

MessageId=0x4BD
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICENAME
Language=English
ERROR_INVALID_SERVICENAME
.

MessageId=0x4BE
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NETNAME
Language=English
ERROR_INVALID_NETNAME
.

MessageId=0x4BF
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHARENAME
Language=English
ERROR_INVALID_SHARENAME
.

MessageId=0x4C0
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORDNAME
Language=English
ERROR_INVALID_PASSWORDNAME
.

MessageId=0x4C1
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGENAME
Language=English
ERROR_INVALID_MESSAGENAME
.

MessageId=0x4C2
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGEDEST
Language=English
ERROR_INVALID_MESSAGEDEST
.

MessageId=0x4C3
Severity=Success
Facility=System
SymbolicName=ERROR_SESSION_CREDENTIAL_CONFLICT
Language=English
ERROR_SESSION_CREDENTIAL_CONFLICT
.

MessageId=0x4C4
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
Language=English
ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
.

MessageId=0x4C5
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_DOMAINNAME
Language=English
ERROR_DUP_DOMAINNAME
.

MessageId=0x4C6
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NETWORK
Language=English
ERROR_NO_NETWORK
.

MessageId=0x4C7
Severity=Success
Facility=System
SymbolicName=ERROR_CANCELLED
Language=English
ERROR_CANCELLED
.

MessageId=0x4C8
Severity=Success
Facility=System
SymbolicName=ERROR_USER_MAPPED_FILE
Language=English
ERROR_USER_MAPPED_FILE
.

MessageId=0x4C9
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_REFUSED
Language=English
ERROR_CONNECTION_REFUSED
.

MessageId=0x4CA
Severity=Success
Facility=System
SymbolicName=ERROR_GRACEFUL_DISCONNECT
Language=English
ERROR_GRACEFUL_DISCONNECT
.

MessageId=0x4CB
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_ALREADY_ASSOCIATED
Language=English
ERROR_ADDRESS_ALREADY_ASSOCIATED
.

MessageId=0x4CC
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_NOT_ASSOCIATED
Language=English
ERROR_ADDRESS_NOT_ASSOCIATED
.

MessageId=0x4CD
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_INVALID
Language=English
ERROR_CONNECTION_INVALID
.

MessageId=0x4CE
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ACTIVE
Language=English
ERROR_CONNECTION_ACTIVE
.

MessageId=0x4CF
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_UNREACHABLE
Language=English
ERROR_NETWORK_UNREACHABLE
.

MessageId=0x4D0
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_UNREACHABLE
Language=English
ERROR_HOST_UNREACHABLE
.

MessageId=0x4D1
Severity=Success
Facility=System
SymbolicName=ERROR_PROTOCOL_UNREACHABLE
Language=English
ERROR_PROTOCOL_UNREACHABLE
.

MessageId=0x4D2
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_UNREACHABLE
Language=English
ERROR_PORT_UNREACHABLE
.

MessageId=0x4D3
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_ABORTED
Language=English
ERROR_REQUEST_ABORTED
.

MessageId=0x4D4
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ABORTED
Language=English
ERROR_CONNECTION_ABORTED
.

MessageId=0x4D5
Severity=Success
Facility=System
SymbolicName=ERROR_RETRY
Language=English
ERROR_RETRY
.

MessageId=0x4D6
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_COUNT_LIMIT
Language=English
ERROR_CONNECTION_COUNT_LIMIT
.

MessageId=0x4D7
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_TIME_RESTRICTION
Language=English
ERROR_LOGIN_TIME_RESTRICTION
.

MessageId=0x4D8
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_WKSTA_RESTRICTION
Language=English
ERROR_LOGIN_WKSTA_RESTRICTION
.

MessageId=0x4D9
Severity=Success
Facility=System
SymbolicName=ERROR_INCORRECT_ADDRESS
Language=English
ERROR_INCORRECT_ADDRESS
.

MessageId=0x4DA
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_REGISTERED
Language=English
ERROR_ALREADY_REGISTERED
.

MessageId=0x4DB
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_FOUND
Language=English
ERROR_SERVICE_NOT_FOUND
.

MessageId=0x4DC
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_AUTHENTICATED
Language=English
ERROR_NOT_AUTHENTICATED
.

MessageId=0x4DD
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGGED_ON
Language=English
ERROR_NOT_LOGGED_ON
.

MessageId=0x4DE
Severity=Success
Facility=System
SymbolicName=ERROR_CONTINUE
Language=English
ERROR_CONTINUE
.

MessageId=0x4DF
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_INITIALIZED
Language=English
ERROR_ALREADY_INITIALIZED
.

MessageId=0x4E0
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_DEVICES
Language=English
ERROR_NO_MORE_DEVICES
.

MessageId=0x514
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ALL_ASSIGNED
Language=English
ERROR_NOT_ALL_ASSIGNED
.

MessageId=0x515
Severity=Success
Facility=System
SymbolicName=ERROR_SOME_NOT_MAPPED
Language=English
ERROR_SOME_NOT_MAPPED
.

MessageId=0x516
Severity=Success
Facility=System
SymbolicName=ERROR_NO_QUOTAS_FOR_ACCOUNT
Language=English
ERROR_NO_QUOTAS_FOR_ACCOUNT
.

MessageId=0x517
Severity=Success
Facility=System
SymbolicName=ERROR_LOCAL_USER_SESSION_KEY
Language=English
ERROR_LOCAL_USER_SESSION_KEY
.

MessageId=0x518
Severity=Success
Facility=System
SymbolicName=ERROR_NULL_LM_PASSWORD
Language=English
ERROR_NULL_LM_PASSWORD
.

MessageId=0x519
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_REVISION
Language=English
ERROR_UNKNOWN_REVISION
.

MessageId=0x51A
Severity=Success
Facility=System
SymbolicName=ERROR_REVISION_MISMATCH
Language=English
ERROR_REVISION_MISMATCH
.

MessageId=0x51B
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OWNER
Language=English
ERROR_INVALID_OWNER
.

MessageId=0x51C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIMARY_GROUP
Language=English
ERROR_INVALID_PRIMARY_GROUP
.

MessageId=0x51D
Severity=Success
Facility=System
SymbolicName=ERROR_NO_IMPERSONATION_TOKEN
Language=English
ERROR_NO_IMPERSONATION_TOKEN
.

MessageId=0x51E
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_DISABLE_MANDATORY
Language=English
ERROR_CANT_DISABLE_MANDATORY
.

MessageId=0x51F
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOGON_SERVERS
Language=English
ERROR_NO_LOGON_SERVERS
.

MessageId=0x520
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_LOGON_SESSION
Language=English
ERROR_NO_SUCH_LOGON_SESSION
.

MessageId=0x521
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PRIVILEGE
Language=English
ERROR_NO_SUCH_PRIVILEGE
.

MessageId=0x522
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVILEGE_NOT_HELD
Language=English
ERROR_PRIVILEGE_NOT_HELD
.

MessageId=0x523
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCOUNT_NAME
Language=English
ERROR_INVALID_ACCOUNT_NAME
.

MessageId=0x524
Severity=Success
Facility=System
SymbolicName=ERROR_USER_EXISTS
Language=English
ERROR_USER_EXISTS
.

MessageId=0x525
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_USER
Language=English
ERROR_NO_SUCH_USER
.

MessageId=0x526
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_EXISTS
Language=English
ERROR_GROUP_EXISTS
.

MessageId=0x527
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_GROUP
Language=English
ERROR_NO_SUCH_GROUP
.

MessageId=0x528
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_GROUP
Language=English
ERROR_MEMBER_IN_GROUP
.

MessageId=0x529
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_GROUP
Language=English
ERROR_MEMBER_NOT_IN_GROUP
.

MessageId=0x52A
Severity=Success
Facility=System
SymbolicName=ERROR_LAST_ADMIN
Language=English
ERROR_LAST_ADMIN
.

MessageId=0x52B
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_PASSWORD
Language=English
ERROR_WRONG_PASSWORD
.

MessageId=0x52C
Severity=Success
Facility=System
SymbolicName=ERROR_ILL_FORMED_PASSWORD
Language=English
ERROR_ILL_FORMED_PASSWORD
.

MessageId=0x52D
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_RESTRICTION
Language=English
ERROR_PASSWORD_RESTRICTION
.

MessageId=0x52E
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_FAILURE
Language=English
ERROR_LOGON_FAILURE
.

MessageId=0x52F
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_RESTRICTION
Language=English
ERROR_ACCOUNT_RESTRICTION
.

MessageId=0x530
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_HOURS
Language=English
ERROR_INVALID_LOGON_HOURS
.

MessageId=0x531
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WORKSTATION
Language=English
ERROR_INVALID_WORKSTATION
.

MessageId=0x532
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_EXPIRED
Language=English
ERROR_PASSWORD_EXPIRED
.

MessageId=0x533
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_DISABLED
Language=English
ERROR_ACCOUNT_DISABLED
.

MessageId=0x534
Severity=Success
Facility=System
SymbolicName=ERROR_NONE_MAPPED
Language=English
ERROR_NONE_MAPPED
.

MessageId=0x535
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LUIDS_REQUESTED
Language=English
ERROR_TOO_MANY_LUIDS_REQUESTED
.

MessageId=0x536
Severity=Success
Facility=System
SymbolicName=ERROR_LUIDS_EXHAUSTED
Language=English
ERROR_LUIDS_EXHAUSTED
.

MessageId=0x537
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SUB_AUTHORITY
Language=English
ERROR_INVALID_SUB_AUTHORITY
.

MessageId=0x538
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACL
Language=English
ERROR_INVALID_ACL
.

MessageId=0x539
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SID
Language=English
ERROR_INVALID_SID
.

MessageId=0x53A
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SECURITY_DESCR
Language=English
ERROR_INVALID_SECURITY_DESCR
.

MessageId=0x53C
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_INHERITANCE_ACL
Language=English
ERROR_BAD_INHERITANCE_ACL
.

MessageId=0x53D
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_DISABLED
Language=English
ERROR_SERVER_DISABLED
.

MessageId=0x53E
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_NOT_DISABLED
Language=English
ERROR_SERVER_NOT_DISABLED
.

MessageId=0x53F
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ID_AUTHORITY
Language=English
ERROR_INVALID_ID_AUTHORITY
.

MessageId=0x540
Severity=Success
Facility=System
SymbolicName=ERROR_ALLOTTED_SPACE_EXCEEDED
Language=English
ERROR_ALLOTTED_SPACE_EXCEEDED
.

MessageId=0x541
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUP_ATTRIBUTES
Language=English
ERROR_INVALID_GROUP_ATTRIBUTES
.

MessageId=0x542
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_IMPERSONATION_LEVEL
Language=English
ERROR_BAD_IMPERSONATION_LEVEL
.

MessageId=0x543
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_OPEN_ANONYMOUS
Language=English
ERROR_CANT_OPEN_ANONYMOUS
.

MessageId=0x544
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_VALIDATION_CLASS
Language=English
ERROR_BAD_VALIDATION_CLASS
.

MessageId=0x545
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_TOKEN_TYPE
Language=English
ERROR_BAD_TOKEN_TYPE
.

MessageId=0x546
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SECURITY_ON_OBJECT
Language=English
ERROR_NO_SECURITY_ON_OBJECT
.

MessageId=0x547
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ACCESS_DOMAIN_INFO
Language=English
ERROR_CANT_ACCESS_DOMAIN_INFO
.

MessageId=0x548
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVER_STATE
Language=English
ERROR_INVALID_SERVER_STATE
.

MessageId=0x549
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_STATE
Language=English
ERROR_INVALID_DOMAIN_STATE
.

MessageId=0x54A
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_ROLE
Language=English
ERROR_INVALID_DOMAIN_ROLE
.

MessageId=0x54B
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_DOMAIN
Language=English
ERROR_NO_SUCH_DOMAIN
.

MessageId=0x54C
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_EXISTS
Language=English
ERROR_DOMAIN_EXISTS
.

MessageId=0x54D
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_LIMIT_EXCEEDED
Language=English
ERROR_DOMAIN_LIMIT_EXCEEDED
.

MessageId=0x54E
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_CORRUPTION
Language=English
ERROR_INTERNAL_DB_CORRUPTION
.

MessageId=0x54F
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_ERROR
Language=English
ERROR_INTERNAL_ERROR
.

MessageId=0x550
Severity=Success
Facility=System
SymbolicName=ERROR_GENERIC_NOT_MAPPED
Language=English
ERROR_GENERIC_NOT_MAPPED
.

MessageId=0x551
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DESCRIPTOR_FORMAT
Language=English
ERROR_BAD_DESCRIPTOR_FORMAT
.

MessageId=0x552
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGON_PROCESS
Language=English
ERROR_NOT_LOGON_PROCESS
.

MessageId=0x553
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_EXISTS
Language=English
ERROR_LOGON_SESSION_EXISTS
.

MessageId=0x554
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PACKAGE
Language=English
ERROR_NO_SUCH_PACKAGE
.

MessageId=0x555
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LOGON_SESSION_STATE
Language=English
ERROR_BAD_LOGON_SESSION_STATE
.

MessageId=0x556
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_COLLISION
Language=English
ERROR_LOGON_SESSION_COLLISION
.

MessageId=0x557
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_TYPE
Language=English
ERROR_INVALID_LOGON_TYPE
.

MessageId=0x558
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_IMPERSONATE
Language=English
ERROR_CANNOT_IMPERSONATE
.

MessageId=0x559
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_INVALID_STATE
Language=English
ERROR_RXACT_INVALID_STATE
.

MessageId=0x55A
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMIT_FAILURE
Language=English
ERROR_RXACT_COMMIT_FAILURE
.

MessageId=0x55B
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_ACCOUNT
Language=English
ERROR_SPECIAL_ACCOUNT
.

MessageId=0x55C
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_GROUP
Language=English
ERROR_SPECIAL_GROUP
.

MessageId=0x55D
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_USER
Language=English
ERROR_SPECIAL_USER
.

MessageId=0x55E
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBERS_PRIMARY_GROUP
Language=English
ERROR_MEMBERS_PRIMARY_GROUP
.

MessageId=0x55F
Severity=Success
Facility=System
SymbolicName=ERROR_TOKEN_ALREADY_IN_USE
Language=English
ERROR_TOKEN_ALREADY_IN_USE
.

MessageId=0x560
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_ALIAS
Language=English
ERROR_NO_SUCH_ALIAS
.

MessageId=0x561
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_ALIAS
Language=English
ERROR_MEMBER_NOT_IN_ALIAS
.

MessageId=0x562
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_ALIAS
Language=English
ERROR_MEMBER_IN_ALIAS
.

MessageId=0x563
Severity=Success
Facility=System
SymbolicName=ERROR_ALIAS_EXISTS
Language=English
ERROR_ALIAS_EXISTS
.

MessageId=0x564
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_NOT_GRANTED
Language=English
ERROR_LOGON_NOT_GRANTED
.

MessageId=0x565
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SECRETS
Language=English
ERROR_TOO_MANY_SECRETS
.

MessageId=0x566
Severity=Success
Facility=System
SymbolicName=ERROR_SECRET_TOO_LONG
Language=English
ERROR_SECRET_TOO_LONG
.

MessageId=0x567
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_ERROR
Language=English
ERROR_INTERNAL_DB_ERROR
.

MessageId=0x568
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CONTEXT_IDS
Language=English
ERROR_TOO_MANY_CONTEXT_IDS
.

MessageId=0x569
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_TYPE_NOT_GRANTED
Language=English
ERROR_LOGON_TYPE_NOT_GRANTED
.

MessageId=0x56A
Severity=Success
Facility=System
SymbolicName=ERROR_NT_CROSS_ENCRYPTION_REQUIRED
Language=English
ERROR_NT_CROSS_ENCRYPTION_REQUIRED
.

MessageId=0x56B
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_MEMBER
Language=English
ERROR_NO_SUCH_MEMBER
.

MessageId=0x56C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEMBER
Language=English
ERROR_INVALID_MEMBER
.

MessageId=0x56D
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SIDS
Language=English
ERROR_TOO_MANY_SIDS
.

MessageId=0x56E
Severity=Success
Facility=System
SymbolicName=ERROR_LM_CROSS_ENCRYPTION_REQUIRED
Language=English
ERROR_LM_CROSS_ENCRYPTION_REQUIRED
.

MessageId=0x56F
Severity=Success
Facility=System
SymbolicName=ERROR_NO_INHERITANCE
Language=English
ERROR_NO_INHERITANCE
.

MessageId=0x570
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_CORRUPT
Language=English
ERROR_FILE_CORRUPT
.

MessageId=0x571
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CORRUPT
Language=English
ERROR_DISK_CORRUPT
.

MessageId=0x572
Severity=Success
Facility=System
SymbolicName=ERROR_NO_USER_SESSION_KEY
Language=English
ERROR_NO_USER_SESSION_KEY
.

MessageId=0x573
Severity=Success
Facility=System
SymbolicName=ERROR_LICENSE_QUOTA_EXCEEDED
Language=English
ERROR_LICENSE_QUOTA_EXCEEDED
.

MessageId=0x578
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_HANDLE
Language=English
ERROR_INVALID_WINDOW_HANDLE
.

MessageId=0x579
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MENU_HANDLE
Language=English
ERROR_INVALID_MENU_HANDLE
.

MessageId=0x57A
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CURSOR_HANDLE
Language=English
ERROR_INVALID_CURSOR_HANDLE
.

MessageId=0x57B
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCEL_HANDLE
Language=English
ERROR_INVALID_ACCEL_HANDLE
.

MessageId=0x57C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_HANDLE
Language=English
ERROR_INVALID_HOOK_HANDLE
.

MessageId=0x57D
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DWP_HANDLE
Language=English
ERROR_INVALID_DWP_HANDLE
.

MessageId=0x57E
Severity=Success
Facility=System
SymbolicName=ERROR_TLW_WITH_WSCHILD
Language=English
ERROR_TLW_WITH_WSCHILD
.

MessageId=0x57F
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_FIND_WND_CLASS
Language=English
ERROR_CANNOT_FIND_WND_CLASS
.

MessageId=0x580
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_OF_OTHER_THREAD
Language=English
ERROR_WINDOW_OF_OTHER_THREAD
.

MessageId=0x581
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_ALREADY_REGISTERED
Language=English
ERROR_HOTKEY_ALREADY_REGISTERED
.

MessageId=0x582
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_ALREADY_EXISTS
Language=English
ERROR_CLASS_ALREADY_EXISTS
.

MessageId=0x583
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_DOES_NOT_EXIST
Language=English
ERROR_CLASS_DOES_NOT_EXIST
.

MessageId=0x584
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_HAS_WINDOWS
Language=English
ERROR_CLASS_HAS_WINDOWS
.

MessageId=0x585
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_INDEX
Language=English
ERROR_INVALID_INDEX
.

MessageId=0x586
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ICON_HANDLE
Language=English
ERROR_INVALID_ICON_HANDLE
.

MessageId=0x587
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVATE_DIALOG_INDEX
Language=English
ERROR_PRIVATE_DIALOG_INDEX
.

MessageId=0x588
Severity=Success
Facility=System
SymbolicName=ERROR_LISTBOX_ID_NOT_FOUND
Language=English
ERROR_LISTBOX_ID_NOT_FOUND
.

MessageId=0x589
Severity=Success
Facility=System
SymbolicName=ERROR_NO_WILDCARD_CHARACTERS
Language=English
ERROR_NO_WILDCARD_CHARACTERS
.

MessageId=0x58A
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPBOARD_NOT_OPEN
Language=English
ERROR_CLIPBOARD_NOT_OPEN
.

MessageId=0x58B
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_NOT_REGISTERED
Language=English
ERROR_HOTKEY_NOT_REGISTERED
.

MessageId=0x58C
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_DIALOG
Language=English
ERROR_WINDOW_NOT_DIALOG
.

MessageId=0x58D
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROL_ID_NOT_FOUND
Language=English
ERROR_CONTROL_ID_NOT_FOUND
.

MessageId=0x58E
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMBOBOX_MESSAGE
Language=English
ERROR_INVALID_COMBOBOX_MESSAGE
.

MessageId=0x58F
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_COMBOBOX
Language=English
ERROR_WINDOW_NOT_COMBOBOX
.

MessageId=0x590
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EDIT_HEIGHT
Language=English
ERROR_INVALID_EDIT_HEIGHT
.

MessageId=0x591
Severity=Success
Facility=System
SymbolicName=ERROR_DC_NOT_FOUND
Language=English
ERROR_DC_NOT_FOUND
.

MessageId=0x592
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_FILTER
Language=English
ERROR_INVALID_HOOK_FILTER
.

MessageId=0x593
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FILTER_PROC
Language=English
ERROR_INVALID_FILTER_PROC
.

MessageId=0x594
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NEEDS_HMOD
Language=English
ERROR_HOOK_NEEDS_HMOD
.

MessageId=0x595
Severity=Success
Facility=System
SymbolicName=ERROR_GLOBAL_ONLY_HOOK
Language=English
ERROR_GLOBAL_ONLY_HOOK
.

MessageId=0x596
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_HOOK_SET
Language=English
ERROR_JOURNAL_HOOK_SET
.

MessageId=0x597
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NOT_INSTALLED
Language=English
ERROR_HOOK_NOT_INSTALLED
.

MessageId=0x598
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LB_MESSAGE
Language=English
ERROR_INVALID_LB_MESSAGE
.

MessageId=0x599
Severity=Success
Facility=System
SymbolicName=ERROR_SETCOUNT_ON_BAD_LB
Language=English
ERROR_SETCOUNT_ON_BAD_LB
.

MessageId=0x59A
Severity=Success
Facility=System
SymbolicName=ERROR_LB_WITHOUT_TABSTOPS
Language=English
ERROR_LB_WITHOUT_TABSTOPS
.

MessageId=0x59B
Severity=Success
Facility=System
SymbolicName=ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
Language=English
ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
.

MessageId=0x59C
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_WINDOW_MENU
Language=English
ERROR_CHILD_WINDOW_MENU
.

MessageId=0x59D
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_MENU
Language=English
ERROR_NO_SYSTEM_MENU
.

MessageId=0x59E
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MSGBOX_STYLE
Language=English
ERROR_INVALID_MSGBOX_STYLE
.

MessageId=0x59F
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SPI_VALUE
Language=English
ERROR_INVALID_SPI_VALUE
.

MessageId=0x5A0
Severity=Success
Facility=System
SymbolicName=ERROR_SCREEN_ALREADY_LOCKED
Language=English
ERROR_SCREEN_ALREADY_LOCKED
.

MessageId=0x5A1
Severity=Success
Facility=System
SymbolicName=ERROR_HWNDS_HAVE_DIFF_PARENT
Language=English
ERROR_HWNDS_HAVE_DIFF_PARENT
.

MessageId=0x5A2
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CHILD_WINDOW
Language=English
ERROR_NOT_CHILD_WINDOW
.

MessageId=0x5A3
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GW_COMMAND
Language=English
ERROR_INVALID_GW_COMMAND
.

MessageId=0x5A4
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_THREAD_ID
Language=English
ERROR_INVALID_THREAD_ID
.

MessageId=0x5A5
Severity=Success
Facility=System
SymbolicName=ERROR_NON_MDICHILD_WINDOW
Language=English
ERROR_NON_MDICHILD_WINDOW
.

MessageId=0x5A6
Severity=Success
Facility=System
SymbolicName=ERROR_POPUP_ALREADY_ACTIVE
Language=English
ERROR_POPUP_ALREADY_ACTIVE
.

MessageId=0x5A7
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SCROLLBARS
Language=English
ERROR_NO_SCROLLBARS
.

MessageId=0x5A8
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SCROLLBAR_RANGE
Language=English
ERROR_INVALID_SCROLLBAR_RANGE
.

MessageId=0x5A9
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHOWWIN_COMMAND
Language=English
ERROR_INVALID_SHOWWIN_COMMAND
.

MessageId=0x5AA
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_RESOURCES
Language=English
ERROR_NO_SYSTEM_RESOURCES
.

MessageId=0x5AB
Severity=Success
Facility=System
SymbolicName=ERROR_NONPAGED_SYSTEM_RESOURCES
Language=English
ERROR_NONPAGED_SYSTEM_RESOURCES
.

MessageId=0x5AC
Severity=Success
Facility=System
SymbolicName=ERROR_PAGED_SYSTEM_RESOURCES
Language=English
ERROR_PAGED_SYSTEM_RESOURCES
.

MessageId=0x5AD
Severity=Success
Facility=System
SymbolicName=ERROR_WORKING_SET_QUOTA
Language=English
ERROR_WORKING_SET_QUOTA - Attempt to access invalid address.
.

MessageId=0x5AE
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_QUOTA
Language=English
ERROR_PAGEFILE_QUOTA
.

MessageId=0x5AF
Severity=Success
Facility=System
SymbolicName=ERROR_COMMITMENT_LIMIT
Language=English
ERROR_COMMITMENT_LIMIT
.

MessageId=0x5B0
Severity=Success
Facility=System
SymbolicName=ERROR_MENU_ITEM_NOT_FOUND
Language=English
ERROR_MENU_ITEM_NOT_FOUND
.

MessageId=0x5DC
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CORRUPT
Language=English
ERROR_EVENTLOG_FILE_CORRUPT
.

MessageId=0x5DD
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_CANT_START
Language=English
ERROR_EVENTLOG_CANT_START
.

MessageId=0x5DE
Severity=Success
Facility=System
SymbolicName=ERROR_LOG_FILE_FULL
Language=English
ERROR_LOG_FILE_FULL
.

MessageId=0x5DF
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CHANGED
Language=English
ERROR_EVENTLOG_FILE_CHANGED
.

MessageId=0x6A4
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_BINDING
Language=English
RPC_S_INVALID_STRING_BINDING
.

MessageId=0x6A5
Severity=Success
Facility=System
SymbolicName=RPC_S_WRONG_KIND_OF_BINDING
Language=English
RPC_S_WRONG_KIND_OF_BINDING
.

MessageId=0x6A6
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BINDING
Language=English
RPC_S_INVALID_BINDING
.

MessageId=0x6A7
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_SUPPORTED
Language=English
RPC_S_PROTSEQ_NOT_SUPPORTED
.

MessageId=0x6A8
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_RPC_PROTSEQ
Language=English
RPC_S_INVALID_RPC_PROTSEQ
.

MessageId=0x6A9
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_UUID
Language=English
RPC_S_INVALID_STRING_UUID
.

MessageId=0x6AA
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ENDPOINT_FORMAT
Language=English
RPC_S_INVALID_ENDPOINT_FORMAT
.

MessageId=0x6AB
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NET_ADDR
Language=English
RPC_S_INVALID_NET_ADDR
.

MessageId=0x6AC
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENDPOINT_FOUND
Language=English
RPC_S_NO_ENDPOINT_FOUND
.

MessageId=0x6AD
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TIMEOUT
Language=English
RPC_S_INVALID_TIMEOUT
.

MessageId=0x6AE
Severity=Success
Facility=System
SymbolicName=RPC_S_OBJECT_NOT_FOUND
Language=English
RPC_S_OBJECT_NOT_FOUND
.

MessageId=0x6AF
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_REGISTERED
Language=English
RPC_S_ALREADY_REGISTERED
.

MessageId=0x6B0
Severity=Success
Facility=System
SymbolicName=RPC_S_TYPE_ALREADY_REGISTERED
Language=English
RPC_S_TYPE_ALREADY_REGISTERED
.

MessageId=0x6B1
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_LISTENING
Language=English
RPC_S_ALREADY_LISTENING
.

MessageId=0x6B2
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS_REGISTERED
Language=English
RPC_S_NO_PROTSEQS_REGISTERED
.

MessageId=0x6B3
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_LISTENING
Language=English
RPC_S_NOT_LISTENING
.

MessageId=0x6B4
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_MGR_TYPE
Language=English
RPC_S_UNKNOWN_MGR_TYPE
.

MessageId=0x6B5
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_IF
Language=English
RPC_S_UNKNOWN_IF
.

MessageId=0x6B6
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_BINDINGS
Language=English
RPC_S_NO_BINDINGS
.

MessageId=0x6B7
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS
Language=English
RPC_S_NO_PROTSEQS
.

MessageId=0x6B8
Severity=Success
Facility=System
SymbolicName=RPC_S_CANT_CREATE_ENDPOINT
Language=English
RPC_S_CANT_CREATE_ENDPOINT
.

MessageId=0x6B9
Severity=Success
Facility=System
SymbolicName=RPC_S_OUT_OF_RESOURCES
Language=English
RPC_S_OUT_OF_RESOURCES
.

MessageId=0x6BA
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_UNAVAILABLE
Language=English
RPC_S_SERVER_UNAVAILABLE
.

MessageId=0x6BB
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_TOO_BUSY
Language=English
RPC_S_SERVER_TOO_BUSY
.

MessageId=0x6BC
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NETWORK_OPTIONS
Language=English
RPC_S_INVALID_NETWORK_OPTIONS
.

MessageId=0x6BD
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CALL_ACTIVE
Language=English
RPC_S_NO_CALL_ACTIVE
.

MessageId=0x6BE
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED
Language=English
RPC_S_CALL_FAILED
.

MessageId=0x6BF
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED_DNE
Language=English
RPC_S_CALL_FAILED_DNE
.

MessageId=0x6C0
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTOCOL_ERROR
Language=English
RPC_S_PROTOCOL_ERROR
.

MessageId=0x6C2
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TRANS_SYN
Language=English
RPC_S_UNSUPPORTED_TRANS_SYN
.

MessageId=0x6C4
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TYPE
Language=English
RPC_S_UNSUPPORTED_TYPE
.

MessageId=0x6C5
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TAG
Language=English
RPC_S_INVALID_TAG
.

MessageId=0x6C6
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BOUND
Language=English
RPC_S_INVALID_BOUND
.

MessageId=0x6C7
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENTRY_NAME
Language=English
RPC_S_NO_ENTRY_NAME
.

MessageId=0x6C8
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAME_SYNTAX
Language=English
RPC_S_INVALID_NAME_SYNTAX
.

MessageId=0x6C9
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_NAME_SYNTAX
Language=English
RPC_S_UNSUPPORTED_NAME_SYNTAX
.

MessageId=0x6CB
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_NO_ADDRESS
Language=English
RPC_S_UUID_NO_ADDRESS
.

MessageId=0x6CC
Severity=Success
Facility=System
SymbolicName=RPC_S_DUPLICATE_ENDPOINT
Language=English
RPC_S_DUPLICATE_ENDPOINT
.

MessageId=0x6CD
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_TYPE
Language=English
RPC_S_UNKNOWN_AUTHN_TYPE
.

MessageId=0x6CE
Severity=Success
Facility=System
SymbolicName=RPC_S_MAX_CALLS_TOO_SMALL
Language=English
RPC_S_MAX_CALLS_TOO_SMALL - Arithmetic result exceeded 32 bits.
.

MessageId=0x6CF
Severity=Success
Facility=System
SymbolicName=RPC_S_STRING_TOO_LONG
Language=English
RPC_S_STRING_TOO_LONG - There is a process on other end of the pipe.
.

MessageId=0x6D0
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_FOUND
Language=English
RPC_S_PROTSEQ_NOT_FOUND - Waiting for a process to open the other end of the pipe.
.

MessageId=0x6D1
Severity=Success
Facility=System
SymbolicName=RPC_S_PROCNUM_OUT_OF_RANGE
Language=English
RPC_S_PROCNUM_OUT_OF_RANGE
.

MessageId=0x6D2
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_HAS_NO_AUTH
Language=English
RPC_S_BINDING_HAS_NO_AUTH
.

MessageId=0x6D3
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_SERVICE
Language=English
RPC_S_UNKNOWN_AUTHN_SERVICE
.

MessageId=0x6D4
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_LEVEL
Language=English
RPC_S_UNKNOWN_AUTHN_LEVEL
.

MessageId=0x6D5
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_AUTH_IDENTITY
Language=English
RPC_S_INVALID_AUTH_IDENTITY
.

MessageId=0x6D6
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHZ_SERVICE
Language=English
RPC_S_UNKNOWN_AUTHZ_SERVICE
.

MessageId=0x6D7
Severity=Success
Facility=System
SymbolicName=EPT_S_INVALID_ENTRY
Language=English
EPT_S_INVALID_ENTRY
.

MessageId=0x6D8
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_PERFORM_OP
Language=English
EPT_S_CANT_PERFORM_OP
.

MessageId=0x6D9
Severity=Success
Facility=System
SymbolicName=EPT_S_NOT_REGISTERED
Language=English
EPT_S_NOT_REGISTERED
.

MessageId=0x6DA
Severity=Success
Facility=System
SymbolicName=RPC_S_NOTHING_TO_EXPORT
Language=English
RPC_S_NOTHING_TO_EXPORT
.

MessageId=0x6DB
Severity=Success
Facility=System
SymbolicName=RPC_S_INCOMPLETE_NAME
Language=English
RPC_S_INCOMPLETE_NAME
.

MessageId=0x6DC
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_VERS_OPTION
Language=English
RPC_S_INVALID_VERS_OPTION
.

MessageId=0x6DD
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_MEMBERS
Language=English
RPC_S_NO_MORE_MEMBERS
.

MessageId=0x6DE
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_ALL_OBJS_UNEXPORTED
Language=English
RPC_S_NOT_ALL_OBJS_UNEXPORTED
.

MessageId=0x6DF
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERFACE_NOT_FOUND
Language=English
RPC_S_INTERFACE_NOT_FOUND
.

MessageId=0x6E0
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_ALREADY_EXISTS
Language=English
RPC_S_ENTRY_ALREADY_EXISTS
.

MessageId=0x6E1
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_NOT_FOUND
Language=English
RPC_S_ENTRY_NOT_FOUND
.

MessageId=0x6E2
Severity=Success
Facility=System
SymbolicName=RPC_S_NAME_SERVICE_UNAVAILABLE
Language=English
RPC_S_NAME_SERVICE_UNAVAILABLE
.

MessageId=0x6E3
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAF_ID
Language=English
RPC_S_INVALID_NAF_ID
.

MessageId=0x6E4
Severity=Success
Facility=System
SymbolicName=RPC_S_CANNOT_SUPPORT
Language=English
RPC_S_CANNOT_SUPPORT
.

MessageId=0x6E5
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CONTEXT_AVAILABLE
Language=English
RPC_S_NO_CONTEXT_AVAILABLE
.

MessageId=0x6E6
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERNAL_ERROR
Language=English
RPC_S_INTERNAL_ERROR
.

MessageId=0x6E7
Severity=Success
Facility=System
SymbolicName=RPC_S_ZERO_DIVIDE
Language=English
RPC_S_ZERO_DIVIDE
.

MessageId=0x6E8
Severity=Success
Facility=System
SymbolicName=RPC_S_ADDRESS_ERROR
Language=English
RPC_S_ADDRESS_ERROR
.

MessageId=0x6E9
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_DIV_ZERO
Language=English
RPC_S_FP_DIV_ZERO
.

MessageId=0x6EA
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_UNDERFLOW
Language=English
RPC_S_FP_UNDERFLOW
.

MessageId=0x6EB
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_OVERFLOW
Language=English
RPC_S_FP_OVERFLOW
.

MessageId=0x6EC
Severity=Success
Facility=System
SymbolicName=RPC_X_NO_MORE_ENTRIES
Language=English
RPC_X_NO_MORE_ENTRIES
.

MessageId=0x6ED
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_OPEN_FAIL
Language=English
RPC_X_SS_CHAR_TRANS_OPEN_FAIL
.

MessageId=0x6EE
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_SHORT_FILE
Language=English
RPC_X_SS_CHAR_TRANS_SHORT_FILE
.

MessageId=0x6EF
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_IN_NULL_CONTEXT
Language=English
RPC_X_SS_IN_NULL_CONTEXT
.

MessageId=0x6F1
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CONTEXT_DAMAGED
Language=English
RPC_X_SS_CONTEXT_DAMAGED
.

MessageId=0x6F2
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_HANDLES_MISMATCH
Language=English
RPC_X_SS_HANDLES_MISMATCH
.

MessageId=0x6F3
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CANNOT_GET_CALL_HANDLE
Language=English
RPC_X_SS_CANNOT_GET_CALL_HANDLE
.

MessageId=0x6F4
Severity=Success
Facility=System
SymbolicName=RPC_X_NULL_REF_POINTER
Language=English
RPC_X_NULL_REF_POINTER
.

MessageId=0x6F5
Severity=Success
Facility=System
SymbolicName=RPC_X_ENUM_VALUE_OUT_OF_RANGE
Language=English
RPC_X_ENUM_VALUE_OUT_OF_RANGE
.

MessageId=0x6F6
Severity=Success
Facility=System
SymbolicName=RPC_X_BYTE_COUNT_TOO_SMALL
Language=English
RPC_X_BYTE_COUNT_TOO_SMALL
.

MessageId=0x6F7
Severity=Success
Facility=System
SymbolicName=RPC_X_BAD_STUB_DATA
Language=English
RPC_X_BAD_STUB_DATA
.

MessageId=0x6F8
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_USER_BUFFER
Language=English
ERROR_INVALID_USER_BUFFER
.

MessageId=0x6F9
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_MEDIA
Language=English
ERROR_UNRECOGNIZED_MEDIA
.

MessageId=0x6FA
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_LSA_SECRET
Language=English
ERROR_NO_TRUST_LSA_SECRET
.

MessageId=0x6FB
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_SAM_ACCOUNT
Language=English
ERROR_NO_TRUST_SAM_ACCOUNT
.

MessageId=0x6FC
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_DOMAIN_FAILURE
Language=English
ERROR_TRUSTED_DOMAIN_FAILURE
.

MessageId=0x6FD
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_RELATIONSHIP_FAILURE
Language=English
ERROR_TRUSTED_RELATIONSHIP_FAILURE
.

MessageId=0x6FE
Severity=Success
Facility=System
SymbolicName=ERROR_TRUST_FAILURE
Language=English
ERROR_TRUST_FAILURE
.

MessageId=0x6FF
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_IN_PROGRESS
Language=English
RPC_S_CALL_IN_PROGRESS
.

MessageId=0x700
Severity=Success
Facility=System
SymbolicName=ERROR_NETLOGON_NOT_STARTED
Language=English
ERROR_NETLOGON_NOT_STARTED
.

MessageId=0x701
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_EXPIRED
Language=English
ERROR_ACCOUNT_EXPIRED
.

MessageId=0x702
Severity=Success
Facility=System
SymbolicName=ERROR_REDIRECTOR_HAS_OPEN_HANDLES
Language=English
ERROR_REDIRECTOR_HAS_OPEN_HANDLES
.

MessageId=0x703
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
Language=English
ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
.

MessageId=0x704
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PORT
Language=English
ERROR_UNKNOWN_PORT
.

MessageId=0x705
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTER_DRIVER
Language=English
ERROR_UNKNOWN_PRINTER_DRIVER
.

MessageId=0x706
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTPROCESSOR
Language=English
ERROR_UNKNOWN_PRINTPROCESSOR
.

MessageId=0x707
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEPARATOR_FILE
Language=English
ERROR_INVALID_SEPARATOR_FILE
.

MessageId=0x708
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIORITY
Language=English
ERROR_INVALID_PRIORITY
.

MessageId=0x709
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_NAME
Language=English
ERROR_INVALID_PRINTER_NAME
.

MessageId=0x70A
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_ALREADY_EXISTS
Language=English
ERROR_PRINTER_ALREADY_EXISTS
.

MessageId=0x70B
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_COMMAND
Language=English
ERROR_INVALID_PRINTER_COMMAND
.

MessageId=0x70C
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATATYPE
Language=English
ERROR_INVALID_DATATYPE
.

MessageId=0x70D
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ENVIRONMENT
Language=English
ERROR_INVALID_ENVIRONMENT
.

MessageId=0x70E
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_BINDINGS
Language=English
RPC_S_NO_MORE_BINDINGS
.

MessageId=0x70F
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
Language=English
ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
.

MessageId=0x710
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
Language=English
ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
.

MessageId=0x711
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
Language=English
ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
.

MessageId=0x712
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_TRUST_INCONSISTENT
Language=English
ERROR_DOMAIN_TRUST_INCONSISTENT
.

MessageId=0x713
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_HAS_OPEN_HANDLES
Language=English
ERROR_SERVER_HAS_OPEN_HANDLES
.

MessageId=0x714
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_DATA_NOT_FOUND
Language=English
ERROR_RESOURCE_DATA_NOT_FOUND
.

MessageId=0x715
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_TYPE_NOT_FOUND
Language=English
ERROR_RESOURCE_TYPE_NOT_FOUND
.

MessageId=0x716
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NAME_NOT_FOUND
Language=English
ERROR_RESOURCE_NAME_NOT_FOUND
.

MessageId=0x717
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_LANG_NOT_FOUND
Language=English
ERROR_RESOURCE_LANG_NOT_FOUND
.

MessageId=0x718
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_QUOTA
Language=English
ERROR_NOT_ENOUGH_QUOTA
.

MessageId=0x719
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_INTERFACES
Language=English
RPC_S_NO_INTERFACES
.

MessageId=0x71A
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_CANCELLED
Language=English
RPC_S_CALL_CANCELLED
.

MessageId=0x71B
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_INCOMPLETE
Language=English
RPC_S_BINDING_INCOMPLETE
.

MessageId=0x71C
Severity=Success
Facility=System
SymbolicName=RPC_S_COMM_FAILURE
Language=English
RPC_S_COMM_FAILURE
.

MessageId=0x71D
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_AUTHN_LEVEL
Language=English
RPC_S_UNSUPPORTED_AUTHN_LEVEL
.

MessageId=0x71E
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PRINC_NAME
Language=English
RPC_S_NO_PRINC_NAME
.

MessageId=0x71F
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_RPC_ERROR
Language=English
RPC_S_NOT_RPC_ERROR
.

MessageId=0x720
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_LOCAL_ONLY
Language=English
RPC_S_UUID_LOCAL_ONLY
.

MessageId=0x721
Severity=Success
Facility=System
SymbolicName=RPC_S_SEC_PKG_ERROR
Language=English
RPC_S_SEC_PKG_ERROR
.

MessageId=0x722
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_CANCELLED
Language=English
RPC_S_NOT_CANCELLED
.

MessageId=0x723
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_ES_ACTION
Language=English
RPC_X_INVALID_ES_ACTION
.

MessageId=0x724
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_ES_VERSION
Language=English
RPC_X_WRONG_ES_VERSION
.

MessageId=0x725
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_STUB_VERSION
Language=English
RPC_X_WRONG_STUB_VERSION
.

MessageId=0x726
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_PIPE_OBJECT
Language=English
RPC_X_INVALID_PIPE_OBJECT
.

MessageId=0x727
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_ORDER
Language=English
RPC_X_WRONG_PIPE_ORDER
.

MessageId=0x728
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_VERSION
Language=English
RPC_X_WRONG_PIPE_VERSION
.

MessageId=0x76A
Severity=Success
Facility=System
SymbolicName=RPC_S_GROUP_MEMBER_NOT_FOUND
Language=English
RPC_S_GROUP_MEMBER_NOT_FOUND
.

MessageId=0x76B
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_CREATE
Language=English
EPT_S_CANT_CREATE
.

MessageId=0x76C
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_OBJECT
Language=English
RPC_S_INVALID_OBJECT
.

MessageId=0x76D
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TIME
Language=English
ERROR_INVALID_TIME
.

MessageId=0x76E
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_NAME
Language=English
ERROR_INVALID_FORM_NAME
.

MessageId=0x76F
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_SIZE
Language=English
ERROR_INVALID_FORM_SIZE
.

MessageId=0x770
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_WAITING
Language=English
ERROR_ALREADY_WAITING
.

MessageId=0x771
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DELETED
Language=English
ERROR_PRINTER_DELETED
.

MessageId=0x772
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_STATE
Language=English
ERROR_INVALID_PRINTER_STATE
.

MessageId=0x773
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_MUST_CHANGE
Language=English
ERROR_PASSWORD_MUST_CHANGE
.

MessageId=0x774
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CONTROLLER_NOT_FOUND
Language=English
ERROR_DOMAIN_CONTROLLER_NOT_FOUND
.

MessageId=0x775
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_LOCKED_OUT
Language=English
ERROR_ACCOUNT_LOCKED_OUT
.

MessageId=0x776
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OXID
Language=English
OR_INVALID_OXID
.

MessageId=0x777
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OID
Language=English
OR_INVALID_OID
.

MessageId=0x778
Severity=Success
Facility=System
SymbolicName=OR_INVALID_SET
Language=English
OR_INVALID_SET
.

MessageId=0x779
Severity=Success
Facility=System
SymbolicName=RPC_S_SEND_INCOMPLETE
Language=English
RPC_S_SEND_INCOMPLETE
.

MessageId=0x7D0
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PIXEL_FORMAT
Language=English
ERROR_INVALID_PIXEL_FORMAT
.

MessageId=0x7D1
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER
Language=English
ERROR_BAD_DRIVER
.

MessageId=0x7D2
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_STYLE
Language=English
ERROR_INVALID_WINDOW_STYLE
.

MessageId=0x7D3
Severity=Success
Facility=System
SymbolicName=ERROR_METAFILE_NOT_SUPPORTED
Language=English
ERROR_METAFILE_NOT_SUPPORTED
.

MessageId=0x7D4
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSFORM_NOT_SUPPORTED
Language=English
ERROR_TRANSFORM_NOT_SUPPORTED
.

MessageId=0x7D5
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPPING_NOT_SUPPORTED
Language=English
ERROR_CLIPPING_NOT_SUPPORTED
.

MessageId=0x89A
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_USERNAME
Language=English
ERROR_BAD_USERNAME
.

MessageId=0x8CA
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONNECTED
Language=English
ERROR_NOT_CONNECTED
.

MessageId=0x961
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FILES
Language=English
ERROR_OPEN_FILES
.

MessageId=0x962
Severity=Success
Facility=System
SymbolicName=ERROR_ACTIVE_CONNECTIONS
Language=English
ERROR_ACTIVE_CONNECTIONS
.

MessageId=0x964
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_IN_USE
Language=English
ERROR_DEVICE_IN_USE
.

MessageId=0xBB8
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINT_MONITOR
Language=English
ERROR_UNKNOWN_PRINT_MONITOR
.

MessageId=0xBB9
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_IN_USE
Language=English
ERROR_PRINTER_DRIVER_IN_USE
.

MessageId=0xBBA
Severity=Success
Facility=System
SymbolicName=ERROR_SPOOL_FILE_NOT_FOUND
Language=English
ERROR_SPOOL_FILE_NOT_FOUND
.

MessageId=0xBBB
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_STARTDOC
Language=English
ERROR_SPL_NO_STARTDOC
.

MessageId=0xBBC
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_ADDJOB
Language=English
ERROR_SPL_NO_ADDJOB
.

MessageId=0xBBD
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
Language=English
ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
.

MessageId=0xBBE
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_MONITOR_ALREADY_INSTALLED
Language=English
ERROR_PRINT_MONITOR_ALREADY_INSTALLED
.

MessageId=0xFA0
Severity=Success
Facility=System
SymbolicName=ERROR_WINS_INTERNAL
Language=English
ERROR_WINS_INTERNAL
.

MessageId=0xFA1
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_DEL_LOCAL_WINS
Language=English
ERROR_CAN_NOT_DEL_LOCAL_WINS
.

MessageId=0xFA2
Severity=Success
Facility=System
SymbolicName=ERROR_STATIC_INIT
Language=English
ERROR_STATIC_INIT
.

MessageId=0xFA3
Severity=Success
Facility=System
SymbolicName=ERROR_INC_BACKUP
Language=English
ERROR_INC_BACKUP
.

MessageId=0xFA4
Severity=Success
Facility=System
SymbolicName=ERROR_FULL_BACKUP
Language=English
ERROR_FULL_BACKUP
.

MessageId=0xFA5
Severity=Success
Facility=System
SymbolicName=ERROR_REC_NON_EXISTENT
Language=English
ERROR_REC_NON_EXISTENT
.

MessageId=0xFA6
Severity=Success
Facility=System
SymbolicName=ERROR_RPL_NOT_ALLOWED
Language=English
ERROR_RPL_NOT_ALLOWED
.

MessageId=0x17E6
Severity=Success
Facility=System
SymbolicName=ERROR_NO_BROWSER_SERVERS_FOUND
Language=English
ERROR_NO_BROWSER_SERVERS_FOUND
.

; EOF
