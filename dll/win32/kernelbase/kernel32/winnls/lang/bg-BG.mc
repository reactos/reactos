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
               ITF=0x4:FACILITY_ITF
               WIN32=0x7:FACILITY_GENERAL
              )

LanguageNames=(Bulgarian=0x419:MSG00419)


;
; message definitions
;

; Facility=System

MessageId=0
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS
Language=Bulgarian
ERROR_SUCCESS - ���������� � ��������� �������.
.

MessageId=1
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FUNCTION
Language=Bulgarian
ERROR_INVALID_FUNCTION - ������ �������.
.

MessageId=2
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_NOT_FOUND
Language=Bulgarian
ERROR_FILE_NOT_FOUND - ������ �� � ������.
.

MessageId=3
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_NOT_FOUND
Language=Bulgarian
ERROR_PATH_NOT_FOUND - ����� �� � ������.
.

MessageId=4
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_OPEN_FILES
Language=Bulgarian
ERROR_TOO_MANY_OPEN_FILES - ������ ����� �������� �������.
.

MessageId=5
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DENIED
Language=Bulgarian
ERROR_ACCESS_DENIED - �������� � �������.
.

MessageId=6
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HANDLE
Language=Bulgarian
ERROR_INVALID_HANDLE - ������ ��������.
.

MessageId=7
Severity=Success
Facility=System
SymbolicName=ERROR_ARENA_TRASHED
Language=Bulgarian
ERROR_ARENA_TRASHED - The storage control blocks were destroyed.
.

MessageId=8
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_MEMORY
Language=Bulgarian
ERROR_NOT_ENOUGH_MEMORY - ������������ �������� ��������� �� ���������� �� ���������.
.

MessageId=9
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK
Language=Bulgarian
ERROR_INVALID_BLOCK - The storage control block address is invalid.
.

MessageId=10
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ENVIRONMENT
Language=Bulgarian
ERROR_BAD_ENVIRONMENT - ������� � ����������.
.

MessageId=11
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_FORMAT
Language=Bulgarian
ERROR_BAD_FORMAT - �������� � ���� �� ��������� �� ���������� � ���������� ������.
.

MessageId=12
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCESS
Language=Bulgarian
ERROR_INVALID_ACCESS - ����� �� ������ � ������.
.

MessageId=13
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATA
Language=Bulgarian
ERROR_INVALID_DATA - ���������� �����.
.

MessageId=14
Severity=Success
Facility=System
SymbolicName=ERROR_OUTOFMEMORY
Language=Bulgarian
ERROR_OUTOFMEMORY - Not enough storage is available to complete this operation.
.

MessageId=15
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DRIVE
Language=Bulgarian
ERROR_INVALID_DRIVE - �������� �� ���� �� ������ ��������� ����������.
.

MessageId=16
Severity=Success
Facility=System
SymbolicName=ERROR_CURRENT_DIRECTORY
Language=Bulgarian
ERROR_CURRENT_DIRECTORY - ����������� �� ������� � ����������.
.

MessageId=17
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAME_DEVICE
Language=Bulgarian
ERROR_NOT_SAME_DEVICE - �������� �� ���� �� �������� ����� �� ����� ������� ����������.
.

MessageId=18
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_FILES
Language=Bulgarian
ERROR_NO_MORE_FILES - ���� ������ �������.
.

MessageId=19
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_PROTECT
Language=Bulgarian
ERROR_WRITE_PROTECT - ��������� � ������� ������ �����.
.

MessageId=20
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_UNIT
Language=Bulgarian
ERROR_BAD_UNIT - �������� �� ���� �� ������ ��������� ����������.
.

MessageId=21
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_READY
Language=Bulgarian
ERROR_NOT_READY - ������������ �� � ������.
.

MessageId=22
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_COMMAND
Language=Bulgarian
ERROR_BAD_COMMAND - ������������ �� ���������� ���������.
.

MessageId=23
Severity=Success
Facility=System
SymbolicName=ERROR_CRC
Language=Bulgarian
ERROR_CRC - ������ � ������� (�������� �� ��������� ������� (crc)).
.

MessageId=24
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LENGTH
Language=Bulgarian
ERROR_BAD_LENGTH - ���������� ������ �������, �� ��������� �� ��������� � ������.
.

MessageId=25
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK
Language=Bulgarian
ERROR_SEEK - ������������ �� ���� �� ������ ���������� ������ ��� ��������� �� �����.
.

MessageId=26
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_DOS_DISK
Language=Bulgarian
ERROR_NOT_DOS_DISK - ��������� ���� ��� ������� �� ���� �� ���� ��������.
.

MessageId=27
Severity=Success
Facility=System
SymbolicName=ERROR_SECTOR_NOT_FOUND
Language=Bulgarian
ERROR_SECTOR_NOT_FOUND - ������������ �� ���� �� ������ �������� ����� (������).
.

MessageId=28
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_PAPER
Language=Bulgarian
ERROR_OUT_OF_PAPER - �������� � �������� ������.
.

MessageId=29
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_FAULT
Language=Bulgarian
ERROR_WRITE_FAULT - �������� �� ���� �� ���� �� ��������� ����������.
.

MessageId=30
Severity=Success
Facility=System
SymbolicName=ERROR_READ_FAULT
Language=Bulgarian
ERROR_READ_FAULT - �������� �� ���� �� ���� �� ��������� ����������..

MessageId=31
Severity=Success
Facility=System
SymbolicName=ERROR_GEN_FAILURE
Language=Bulgarian
ERROR_GEN_FAILURE - ���������� ��� �������� ����������� �� ������.
.

MessageId=32
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_VIOLATION
Language=Bulgarian
ERROR_SHARING_VIOLATION - ���������� ���� ������ �� �����, ������ � ���� �� �� ����� ��������.
.

MessageId=33
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_VIOLATION
Language=Bulgarian
ERROR_LOCK_VIOLATION - The process cannot access the file because another process has locked a portion of the file.
.

MessageId=34
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_DISK
Language=Bulgarian
ERROR_WRONG_DISK - � ������������ ��� ���������� �������. ������� %2 (������ � �� ����: %3) � ���������� %1.
.

MessageId=36
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_BUFFER_EXCEEDED
Language=Bulgarian
ERROR_SHARING_BUFFER_EXCEEDED - Too many files opened for sharing.
.

MessageId=38
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_EOF
Language=Bulgarian
ERROR_HANDLE_EOF - ��������� � ����� �� �����.
.

MessageId=39
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_DISK_FULL
Language=Bulgarian
ERROR_HANDLE_DISK_FULL - ������ � �����.
.

MessageId=50
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED
Language=Bulgarian
ERROR_NOT_SUPPORTED - �������� �� �� ��������.
.

MessageId=51
Severity=Success
Facility=System
SymbolicName=ERROR_REM_NOT_LIST
Language=Bulgarian
ERROR_REM_NOT_LIST - Windows cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If Windows still cannot find the network path, contact your network administrator.
.

MessageId=52
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_NAME
Language=Bulgarian
ERROR_DUP_NAME - You were not connected because a duplicate name exists on the network. Go to System in the Control Panel to change the computer name and try again.
.

MessageId=53
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NETPATH
Language=Bulgarian
ERROR_BAD_NETPATH - ��������� ��� �� � ������.
.

MessageId=54
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_BUSY
Language=Bulgarian
ERROR_NETWORK_BUSY - ������� � �����.
.

MessageId=55
Severity=Success
Facility=System
SymbolicName=ERROR_DEV_NOT_EXIST
Language=Bulgarian
ERROR_DEV_NOT_EXIST - The specified network resource or device is no longer available.
.

MessageId=56
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CMDS
Language=Bulgarian
ERROR_TOO_MANY_CMDS - The network BIOS command limit has been reached.
.

MessageId=57
Severity=Success
Facility=System
SymbolicName=ERROR_ADAP_HDW_ERR
Language=Bulgarian
ERROR_ADAP_HDW_ERR - ���������� � ������� � ��������� �����.
.

MessageId=58
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_RESP
Language=Bulgarian
ERROR_BAD_NET_RESP - ��������� ������ �� ���� �� ������� ��������� ��������.
.

MessageId=59
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXP_NET_ERR
Language=Bulgarian
ERROR_UNEXP_NET_ERR - ����� �� ���������� ������� ������.
.

MessageId=60
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_REM_ADAP
Language=Bulgarian
ERROR_BAD_REM_ADAP - The remote adapter is not compatible.
.

MessageId=61
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTQ_FULL
Language=Bulgarian
ERROR_PRINTQ_FULL - �������� �� �������� � �����.
.

MessageId=62
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SPOOL_SPACE
Language=Bulgarian
ERROR_NO_SPOOL_SPACE - Space to store the file waiting to be printed is not available on the server.
.

MessageId=63
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_CANCELLED
Language=Bulgarian
ERROR_PRINT_CANCELLED - Your file waiting to be printed was deleted.
.

MessageId=64
Severity=Success
Facility=System
SymbolicName=ERROR_NETNAME_DELETED
Language=Bulgarian
ERROR_NETNAME_DELETED - The specified network name is no longer available.
.

MessageId=65
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_ACCESS_DENIED
Language=Bulgarian
ERROR_NETWORK_ACCESS_DENIED - Network access is denied.
.

MessageId=66
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEV_TYPE
Language=Bulgarian
ERROR_BAD_DEV_TYPE - The network resource type is not correct.
.

MessageId=67
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_NAME
Language=Bulgarian
ERROR_BAD_NET_NAME - ��������� ��� �� � ��������.
.

MessageId=68
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_NAMES
Language=Bulgarian
ERROR_TOO_MANY_NAMES - The name limit for the local computer network adapter card was exceeded.
.

MessageId=69
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SESS
Language=Bulgarian
ERROR_TOO_MANY_SESS - The network BIOS session limit was exceeded.
.

MessageId=70
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_PAUSED
Language=Bulgarian
ERROR_SHARING_PAUSED - The remote server has been paused or is in the process of being started.
.

MessageId=71
Severity=Success
Facility=System
SymbolicName=ERROR_REQ_NOT_ACCEP
Language=Bulgarian
ERROR_REQ_NOT_ACCEP - No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.
.

MessageId=72
Severity=Success
Facility=System
SymbolicName=ERROR_REDIR_PAUSED
Language=Bulgarian
ERROR_REDIR_PAUSED - The specified printer or disk device has been paused.
.

MessageId=80
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_EXISTS
Language=Bulgarian
ERROR_FILE_EXISTS - ������ ����������.
.

MessageId=82
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_MAKE
Language=Bulgarian
ERROR_CANNOT_MAKE - ��������� ��������� �� ���� ��� �����.
.

MessageId=83
Severity=Success
Facility=System
SymbolicName=ERROR_FAIL_I24
Language=Bulgarian
ERROR_FAIL_I24 - Fail on INT 24.
.

MessageId=84
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_STRUCTURES
Language=Bulgarian
ERROR_OUT_OF_STRUCTURES - Storage to process this request is not available.
.

MessageId=85
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_ASSIGNED
Language=Bulgarian
ERROR_ALREADY_ASSIGNED - ����� �� �������� ���������� ���� �� ��������.
.

MessageId=86
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORD
Language=Bulgarian
ERROR_INVALID_PASSWORD - The specified network password is not correct.
.

MessageId=87
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PARAMETER
Language=Bulgarian
ERROR_INVALID_PARAMETER - ���������� ����.
.

MessageId=88
Severity=Success
Facility=System
SymbolicName=ERROR_NET_WRITE_FAULT
Language=Bulgarian
ERROR_NET_WRITE_FAULT - A write fault occurred on the network.
.

MessageId=89
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PROC_SLOTS
Language=Bulgarian
ERROR_NO_PROC_SLOTS - The system cannot start another process at this time.
.

MessageId=100
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEMAPHORES
Language=Bulgarian
ERROR_TOO_MANY_SEMAPHORES - Cannot create another system semaphore.
.

MessageId=101
Severity=Success
Facility=System
SymbolicName=ERROR_EXCL_SEM_ALREADY_OWNED
Language=Bulgarian
ERROR_EXCL_SEM_ALREADY_OWNED - The exclusive semaphore is owned by another process.
.

MessageId=102
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_IS_SET
Language=Bulgarian
ERROR_SEM_IS_SET - The semaphore is set and cannot be closed.
.

MessageId=103
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEM_REQUESTS
Language=Bulgarian
ERROR_TOO_MANY_SEM_REQUESTS - The semaphore cannot be set again.
.

MessageId=104
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_AT_INTERRUPT_TIME
Language=Bulgarian
ERROR_INVALID_AT_INTERRUPT_TIME - Cannot request exclusive semaphores at interrupt time.
.

MessageId=105
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_OWNER_DIED
Language=Bulgarian
ERROR_SEM_OWNER_DIED - The previous ownership of this semaphore has ended.
.

MessageId=106
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_USER_LIMIT
Language=Bulgarian
ERROR_SEM_USER_LIMIT - ��������� ��������� � ���������� %1.
.

MessageId=107
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CHANGE
Language=Bulgarian
ERROR_DISK_CHANGE - The program stopped because an alternate diskette was not inserted.
.

MessageId=108
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVE_LOCKED
Language=Bulgarian
ERROR_DRIVE_LOCKED - The disk is in use or locked by another process.
.

MessageId=109
Severity=Success
Facility=System
SymbolicName=ERROR_BROKEN_PIPE
Language=Bulgarian
ERROR_BROKEN_PIPE - The pipe has been ended.
.

MessageId=110
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FAILED
Language=Bulgarian
ERROR_OPEN_FAILED - ������� �� ������� ������� ��������� ���� ��� ����������.
.

MessageId=111
Severity=Success
Facility=System
SymbolicName=ERROR_BUFFER_OVERFLOW
Language=Bulgarian
ERROR_BUFFER_OVERFLOW - ��� ����� � ������ �����.
.

MessageId=112
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_FULL
Language=Bulgarian
ERROR_DISK_FULL - �� ����� ���� ���������� �����.
.

MessageId=113
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_SEARCH_HANDLES
Language=Bulgarian
ERROR_NO_MORE_SEARCH_HANDLES - No more internal file identifiers available.
.

MessageId=114
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TARGET_HANDLE
Language=Bulgarian
ERROR_INVALID_TARGET_HANDLE - The target internal file identifier is incorrect.
.

MessageId=117
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CATEGORY
Language=Bulgarian
ERROR_INVALID_CATEGORY - The IOCTL call made by the application program is not correct.
.

MessageId=118
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_VERIFY_SWITCH
Language=Bulgarian
ERROR_INVALID_VERIFY_SWITCH - The verify-on-write switch parameter value is not correct.
.

MessageId=119
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER_LEVEL
Language=Bulgarian
ERROR_BAD_DRIVER_LEVEL - The system does not support the command requested.
.

MessageId=120
Severity=Success
Facility=System
SymbolicName=ERROR_CALL_NOT_IMPLEMENTED
Language=Bulgarian
ERROR_CALL_NOT_IMPLEMENTED - ��� ������� �� �������������� � ������ �������.
.

MessageId=121
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_TIMEOUT
Language=Bulgarian
ERROR_SEM_TIMEOUT - The semaphore timeout period has expired.
.

MessageId=122
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_BUFFER
Language=Bulgarian
ERROR_INSUFFICIENT_BUFFER - The data area passed to a system call is too small.
.

MessageId=123
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NAME
Language=Bulgarian
ERROR_INVALID_NAME - ��� �����, ���������� ��� ����� ����� ����� �������� ���������.
.

MessageId=124
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LEVEL
Language=Bulgarian
ERROR_INVALID_LEVEL - The system call level is not correct.
.

MessageId=125
Severity=Success
Facility=System
SymbolicName=ERROR_NO_VOLUME_LABEL
Language=Bulgarian
ERROR_NO_VOLUME_LABEL - ���� �� ����� ����� ����.
.

MessageId=126
Severity=Success
Facility=System
SymbolicName=ERROR_MOD_NOT_FOUND
Language=Bulgarian
ERROR_MOD_NOT_FOUND - ��������� ������ �� ������.
.

MessageId=127
Severity=Success
Facility=System
SymbolicName=ERROR_PROC_NOT_FOUND
Language=Bulgarian
ERROR_PROC_NOT_FOUND - ��������� ��������� �� �������.
.

MessageId=128
Severity=Success
Facility=System
SymbolicName=ERROR_WAIT_NO_CHILDREN
Language=Bulgarian
ERROR_WAIT_NO_CHILDREN - There are no child processes to wait for.
.

MessageId=129
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_NOT_COMPLETE
Language=Bulgarian
ERROR_CHILD_NOT_COMPLETE - ���������� %1 �� ����� ���� �������� � ������ Win32.
.

MessageId=130
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECT_ACCESS_HANDLE
Language=Bulgarian
ERROR_DIRECT_ACCESS_HANDLE - Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.
.

MessageId=131
Severity=Success
Facility=System
SymbolicName=ERROR_NEGATIVE_SEEK
Language=Bulgarian
ERROR_NEGATIVE_SEEK - An attempt was made to move the file pointer before the beginning of the file.
.

MessageId=132
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK_ON_DEVICE
Language=Bulgarian
ERROR_SEEK_ON_DEVICE - The file pointer cannot be set on the specified device or file.
.

MessageId=133
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_TARGET
Language=Bulgarian
ERROR_IS_JOIN_TARGET - A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.
.

MessageId=134
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOINED
Language=Bulgarian
ERROR_IS_JOINED - An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.
.

MessageId=135
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBSTED
Language=Bulgarian
ERROR_IS_SUBSTED - An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.
.

MessageId=136
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_JOINED
Language=Bulgarian
ERROR_NOT_JOINED - The system tried to delete the JOIN of a drive that is not joined.
.

MessageId=137
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUBSTED
Language=Bulgarian
ERROR_NOT_SUBSTED - The system tried to delete the substitution of a drive that is not substituted.
.

MessageId=138
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_JOIN
Language=Bulgarian
ERROR_JOIN_TO_JOIN - The system tried to join a drive to a directory on a joined drive.
.

MessageId=139
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_SUBST
Language=Bulgarian
ERROR_SUBST_TO_SUBST - The system tried to substitute a drive to a directory on a substituted drive.
.

MessageId=140
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_SUBST
Language=Bulgarian
ERROR_JOIN_TO_SUBST - The system tried to join a drive to a directory on a substituted drive.
.

MessageId=141
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_JOIN
Language=Bulgarian
ERROR_SUBST_TO_JOIN - The system tried to SUBST a drive to a directory on a joined drive.
.

MessageId=142
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY_DRIVE
Language=Bulgarian
ERROR_BUSY_DRIVE - The system cannot perform a JOIN or SUBST at this time.
.

MessageId=143
Severity=Success
Facility=System
SymbolicName=ERROR_SAME_DRIVE
Language=Bulgarian
ERROR_SAME_DRIVE - The system cannot join or substitute a drive to or for a directory on the same drive.
.

MessageId=144
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_ROOT
Language=Bulgarian
ERROR_DIR_NOT_ROOT - The directory is not a subdirectory of the root directory.
.

MessageId=145
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_EMPTY
Language=Bulgarian
ERROR_DIR_NOT_EMPTY - ���������� �� ������.
.

MessageId=146
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_PATH
Language=Bulgarian
ERROR_IS_SUBST_PATH - The path specified is being used in a substitute.
.

MessageId=147
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_PATH
Language=Bulgarian
ERROR_IS_JOIN_PATH - Not enough resources are available to process this command.
.

MessageId=148
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_BUSY
Language=Bulgarian
ERROR_PATH_BUSY - ��������� ���� �� ����� ���� ����������� � ������ �����.
.

MessageId=149
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_TARGET
Language=Bulgarian
ERROR_IS_SUBST_TARGET - An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.
.

MessageId=150
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_TRACE
Language=Bulgarian
ERROR_SYSTEM_TRACE - System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.
.

MessageId=151
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENT_COUNT
Language=Bulgarian
ERROR_INVALID_EVENT_COUNT - The number of specified semaphore events for DosMuxSemWait is not correct.
.

MessageId=152
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MUXWAITERS
Language=Bulgarian
ERROR_TOO_MANY_MUXWAITERS - DosMuxSemWait did not execute; too many semaphores are already set.
.

MessageId=153
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LIST_FORMAT
Language=Bulgarian
ERROR_INVALID_LIST_FORMAT - The DosMuxSemWait list is not correct.
.

MessageId=154
Severity=Success
Facility=System
SymbolicName=ERROR_LABEL_TOO_LONG
Language=Bulgarian
ERROR_LABEL_TOO_LONG - The volume label you entered exceeds the label character limit of the target file system.
.

MessageId=155
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_TCBS
Language=Bulgarian
ERROR_TOO_MANY_TCBS - ���������� ������� ������ �����.
.

MessageId=156
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_REFUSED
Language=Bulgarian
ERROR_SIGNAL_REFUSED - The recipient process has refused the signal.
.

MessageId=157
Severity=Success
Facility=System
SymbolicName=ERROR_DISCARDED
Language=Bulgarian
ERROR_DISCARDED - The segment is already discarded and cannot be locked.
.

MessageId=158
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOCKED
Language=Bulgarian
ERROR_NOT_LOCKED - The segment is already unlocked.
.

MessageId=159
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_THREADID_ADDR
Language=Bulgarian
ERROR_BAD_THREADID_ADDR - The address for the thread ID is not correct.
.

MessageId=160
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ARGUMENTS
Language=Bulgarian
ERROR_BAD_ARGUMENTS - The argument string passed to DosExecPgm is not correct.
.

MessageId=161
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PATHNAME
Language=Bulgarian
ERROR_BAD_PATHNAME - ��������� ���� ��������.
.

MessageId=162
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_PENDING
Language=Bulgarian
ERROR_SIGNAL_PENDING - A signal is already pending.
.

MessageId=164
Severity=Success
Facility=System
SymbolicName=ERROR_MAX_THRDS_REACHED
Language=Bulgarian
ERROR_MAX_THRDS_REACHED - No more threads can be created in the system.
.

MessageId=167
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_FAILED
Language=Bulgarian
ERROR_LOCK_FAILED - Unable to lock a region of a file.
.

MessageId=170
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY
Language=Bulgarian
ERROR_BUSY - ��������� ������ ��� ������������.
.

MessageId=173
Severity=Success
Facility=System
SymbolicName=ERROR_CANCEL_VIOLATION
Language=Bulgarian
ERROR_CANCEL_VIOLATION - A lock request was not outstanding for the supplied cancel region.
.

MessageId=174
Severity=Success
Facility=System
SymbolicName=ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
Language=Bulgarian
ERROR_ATOMIC_LOCKS_NOT_SUPPORTED - The file system does not support atomic changes to the lock type.
.

MessageId=180
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGMENT_NUMBER
Language=Bulgarian
ERROR_INVALID_SEGMENT_NUMBER - The system detected a segment number that was not correct.
.

MessageId=182
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ORDINAL
Language=Bulgarian
ERROR_INVALID_ORDINAL - ������������ ������� �� ����� ��������� %1.
.

MessageId=183
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_EXISTS
Language=Bulgarian
ERROR_ALREADY_EXISTS - Cannot create a file when that file already exists.
.

MessageId=186
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAG_NUMBER
Language=Bulgarian
ERROR_INVALID_FLAG_NUMBER - The flag passed is not correct.
.

MessageId=187
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_NOT_FOUND
Language=Bulgarian
ERROR_SEM_NOT_FOUND - The specified system semaphore name was not found.
.

MessageId=188
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STARTING_CODESEG
Language=Bulgarian
ERROR_INVALID_STARTING_CODESEG - ������������ ������� �� ����� ��������� %1.
.

MessageId=189
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STACKSEG
Language=Bulgarian
ERROR_INVALID_STACKSEG - ������������ ������� �� ����� ��������� %1.
.

MessageId=190
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MODULETYPE
Language=Bulgarian
ERROR_INVALID_MODULETYPE - ������������ ������� �� ����� ��������� %1.
.

MessageId=191
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EXE_SIGNATURE
Language=Bulgarian
ERROR_INVALID_EXE_SIGNATURE - ���������� ��������� %1 � ������ Win32.
.

MessageId=192
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_MARKED_INVALID
Language=Bulgarian
ERROR_EXE_MARKED_INVALID - ������������ ������� �� ����� ��������� %1.
.

MessageId=193
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_EXE_FORMAT
Language=Bulgarian
ERROR_BAD_EXE_FORMAT - %1 ����������� Win32 ����������.
.

MessageId=194
Severity=Success
Facility=System
SymbolicName=ERROR_ITERATED_DATA_EXCEEDS_64k
Language=Bulgarian
ERROR_ITERATED_DATA_EXCEEDS_64k - ������������ ������� �� ����� ��������� %1.
.

MessageId=195
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MINALLOCSIZE
Language=Bulgarian
ERROR_INVALID_MINALLOCSIZE - ������������ ������� �� ����� ��������� %1.
.

MessageId=196
Severity=Success
Facility=System
SymbolicName=ERROR_DYNLINK_FROM_INVALID_RING
Language=Bulgarian
ERROR_DYNLINK_FROM_INVALID_RING - The operating system cannot run this application program.
.

MessageId=197
Severity=Success
Facility=System
SymbolicName=ERROR_IOPL_NOT_ENABLED
Language=Bulgarian
ERROR_IOPL_NOT_ENABLED - The operating system is not presently configured to run this application.
.

MessageId=198
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGDPL
Language=Bulgarian
ERROR_INVALID_SEGDPL - ������������ ������� �� ����� ��������� %1.
.

MessageId=199
Severity=Success
Facility=System
SymbolicName=ERROR_AUTODATASEG_EXCEEDS_64k
Language=Bulgarian
ERROR_AUTODATASEG_EXCEEDS_64k - The operating system cannot run this application program.
.

MessageId=200
Severity=Success
Facility=System
SymbolicName=ERROR_RING2SEG_MUST_BE_MOVABLE
Language=Bulgarian
ERROR_RING2SEG_MUST_BE_MOVABLE - The code segment cannot be greater than or equal to 64K.
.

MessageId=201
Severity=Success
Facility=System
SymbolicName=ERROR_RELOC_CHAIN_XEEDS_SEGLIM
Language=Bulgarian
ERROR_RELOC_CHAIN_XEEDS_SEGLIM - ������������ ������� �� ����� ��������� %1.
.

MessageId=202
Severity=Success
Facility=System
SymbolicName=ERROR_INFLOOP_IN_RELOC_CHAIN
Language=Bulgarian
ERROR_INFLOOP_IN_RELOC_CHAIN - ������������ ������� �� ����� ��������� %1.
.

MessageId=203
Severity=Success
Facility=System
SymbolicName=ERROR_ENVVAR_NOT_FOUND
Language=Bulgarian
ERROR_ENVVAR_NOT_FOUND - The system could not find the environment option that was entered.
.

MessageId=205
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SIGNAL_SENT
Language=Bulgarian
ERROR_NO_SIGNAL_SENT - No process in the command subtree has a signal handler.
.

MessageId=206
Severity=Success
Facility=System
SymbolicName=ERROR_FILENAME_EXCED_RANGE
Language=Bulgarian
ERROR_FILENAME_EXCED_RANGE - ��� ����� ��� ���������� ������� �������.
.

MessageId=207
Severity=Success
Facility=System
SymbolicName=ERROR_RING2_STACK_IN_USE
Language=Bulgarian
ERROR_RING2_STACK_IN_USE - The ring 2 stack is in use.
.

MessageId=208
Severity=Success
Facility=System
SymbolicName=ERROR_META_EXPANSION_TOO_LONG
Language=Bulgarian
ERROR_META_EXPANSION_TOO_LONG - The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.
.

MessageId=209
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SIGNAL_NUMBER
Language=Bulgarian
ERROR_INVALID_SIGNAL_NUMBER - The signal being posted is not correct.
.

MessageId=210
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_1_INACTIVE
Language=Bulgarian
ERROR_THREAD_1_INACTIVE - The signal handler cannot be set.
.

MessageId=212
Severity=Success
Facility=System
SymbolicName=ERROR_LOCKED
Language=Bulgarian
ERROR_LOCKED - The segment is locked and cannot be reallocated.
.

MessageId=214
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MODULES
Language=Bulgarian
ERROR_TOO_MANY_MODULES - Too many dynamic-link modules are attached to this program or dynamic-link module.
.

MessageId=215
Severity=Success
Facility=System
SymbolicName=ERROR_NESTING_NOT_ALLOWED
Language=Bulgarian
ERROR_NESTING_NOT_ALLOWED - Cannot nest calls to LoadModule.
.

MessageId=216
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_MACHINE_TYPE_MISMATCH
Language=Bulgarian
ERROR_EXE_MACHINE_TYPE_MISMATCH - The image file %1 is valid, but is for a machine type other than the current machine.
.

MessageId=217
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY
Language=Bulgarian
ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY - The image file %1 is signed, unable to modify.
.

MessageId=218
Severity=Success
Facility=System
SymbolicName=ERRO_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY
Language=Bulgarian
ERRO_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY - The image file %1 is strong signed, unable to modify.
.

MessageId=230
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PIPE
Language=Bulgarian
ERROR_BAD_PIPE - The pipe state is invalid.
.

MessageId=231
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_BUSY
Language=Bulgarian
ERROR_PIPE_BUSY - All pipe instances are busy.
.

MessageId=232
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA
Language=Bulgarian
ERROR_NO_DATA - The pipe is being closed.
.

MessageId=233
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_NOT_CONNECTED
Language=Bulgarian
ERROR_PIPE_NOT_CONNECTED - No process is on the other end of the pipe.
.

MessageId=234
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_DATA
Language=Bulgarian
ERROR_MORE_DATA - ������ ������ ����������.
.

MessageId=240
Severity=Success
Facility=System
SymbolicName=ERROR_VC_DISCONNECTED
Language=Bulgarian
ERROR_VC_DISCONNECTED - ������ ���� ��������.
.

MessageId=254
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_NAME
Language=Bulgarian
ERROR_INVALID_EA_NAME - The specified extended attribute name was invalid.
.

MessageId=255
Severity=Success
Facility=System
SymbolicName=ERROR_EA_LIST_INCONSISTENT
Language=Bulgarian
ERROR_EA_LIST_INCONSISTENT - ����������� �������� �������������.
.

MessageId=258
Severity=Success
Facility=System
SymbolicName=WAIT_TIMEOUT
Language=Bulgarian
WAIT_TIMEOUT - The wait operation timed out.
.

MessageId=259
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_ITEMS
Language=Bulgarian
ERROR_NO_MORE_ITEMS - No more data is available.
.

MessageId=266
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_COPY
Language=Bulgarian
ERROR_CANNOT_COPY - ������� ����������� �� ����� ���� ������������.
.

MessageId=267
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECTORY
Language=Bulgarian
ERROR_DIRECTORY - �������� ��� ����������.
.

MessageId=275
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_DIDNT_FIT
Language=Bulgarian
ERROR_EAS_DIDNT_FIT - The extended attributes did not fit in the buffer.
.

MessageId=276
Severity=Success
Facility=System
SymbolicName=ERROR_EA_FILE_CORRUPT
Language=Bulgarian
ERROR_EA_FILE_CORRUPT - The extended attribute file on the mounted file system is corrupt.
.

MessageId=277
Severity=Success
Facility=System
SymbolicName=ERROR_EA_TABLE_FULL
Language=Bulgarian
ERROR_EA_TABLE_FULL - The extended attribute table file is full.
.

MessageId=278
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_HANDLE
Language=Bulgarian
ERROR_INVALID_EA_HANDLE - The specified extended attribute handle is invalid.
.

MessageId=282
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_NOT_SUPPORTED
Language=Bulgarian
ERROR_EAS_NOT_SUPPORTED - �������������� �������� ������� �� ������������ ����������� ��������.
.

MessageId=288
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_OWNER
Language=Bulgarian
ERROR_NOT_OWNER - Attempt to release mutex not owned by caller.
.

MessageId=298
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_POSTS
Language=Bulgarian
ERROR_TOO_MANY_POSTS - Too many posts were made to a semaphore.
.

MessageId=299
Severity=Success
Facility=System
SymbolicName=ERROR_PARTIAL_COPY
Language=Bulgarian
ERROR_PARTIAL_COPY - Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
.

MessageId=300
Severity=Success
Facility=System
SymbolicName=ERROR_OPLOCK_NOT_GRANTED
Language=Bulgarian
ERROR_OPLOCK_NOT_GRANTED - The oplock request is denied.
.

MessageId=301
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPLOCK_PROTOCOL
Language=Bulgarian
ERROR_INVALID_OPLOCK_PROTOCOL - An invalid oplock acknowledgment was received by the system.
.

MessageId=302
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_TOO_FRAGMENTED
Language=Bulgarian
ERROR_DISK_TOO_FRAGMENTED - The volume is too fragmented to complete this operation.
.

MessageId=303
Severity=Success
Facility=System
SymbolicName=ERROR_DELETE_PENDING
Language=Bulgarian
ERROR_DELETE_PENDING - The file cannot be opened because it is in the process of being deleted.
.

MessageId=317
Severity=Success
Facility=System
SymbolicName=ERROR_MR_MID_NOT_FOUND
Language=Bulgarian
ERROR_MR_MID_NOT_FOUND - The system cannot find message text for message number 0x%1 in the message file for %2.
.

MessageId=318
Severity=Success
Facility=System
SymbolicName=ERROR_SCOPE_NOT_FOUND
Language=Bulgarian
ERROR_SCOPE_NOT_FOUND - The scope specified was not found.
.

MessageId=487
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ADDRESS
Language=Bulgarian
ERROR_INVALID_ADDRESS - Attempt to access invalid address.
.

MessageId=534
Severity=Success
Facility=System
SymbolicName=ERROR_ARITHMETIC_OVERFLOW
Language=Bulgarian
ERROR_ARITHMETIC_OVERFLOW - Arithmetic result exceeded 32 bits.
.

MessageId=535
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_CONNECTED
Language=Bulgarian
ERROR_PIPE_CONNECTED - There is a process on other end of the pipe.
.

MessageId=536
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_LISTENING
Language=Bulgarian
ERROR_PIPE_LISTENING - Waiting for a process to open the other end of the pipe.
.

MessageId=537
Severity=Success
Facility=System
SymbolicName=ERROR_ACPI_ERROR
Language=Bulgarian
ERROR_ACPI_ERROR - ��������� ������ � ���������� ACPI.
.

MessageId=538
Severity=Success
Facility=System
SymbolicName=ERROR_ABIOS_ERROR
Language=Bulgarian
ERROR_ABIOS_ERROR - ��������� ������ � ���������� ABIOS.
.

MessageId=539
Severity=Success
Facility=System
SymbolicName=ERROR_WX86_WARNING
Language=Bulgarian
ERROR_WX86_WARNING - A warning occurred in the WX86 subsystem.
.

MessageId=540
Severity=Success
Facility=System
SymbolicName=ERROR_WX86_ERROR
Language=Bulgarian
ERROR_WX86_ERROR - ��������� ������ � ���������� WX86.
.

MessageId=541
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_NOT_CANCELED
Language=Bulgarian
ERROR_TIMER_NOT_CANCELED - An attempt was made to cancel or set a timer that has an associated APC and the subject thread is not the thread that originally set the timer with an associated APC routine.
.

MessageId=542
Severity=Success
Facility=System
SymbolicName=ERROR_UNWIND
Language=Bulgarian
ERROR_UNWIND - Unwind exception code.
.

MessageId=543
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_STACK
Language=Bulgarian
ERROR_BAD_STACK - An invalid or unaligned stack was encountered during an unwind operation.
.

MessageId=544
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_UNWIND_TARGET
Language=Bulgarian
ERROR_INVALID_UNWIND_TARGET - An invalid unwind target was encountered during an unwind operation.
.

MessageId=545
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PORT_ATTRIBUTES
Language=Bulgarian
ERROR_INVALID_PORT_ATTRIBUTES - Invalid Object Attributes specified to NtCreatePort or invalid Port Attributes specified to NtConnectPort
.

MessageId=546
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_MESSAGE_TOO_LONG
Language=Bulgarian
ERROR_PORT_MESSAGE_TOO_LONG - Length of message passed to NtRequestPort or NtRequestWaitReplyPort was longer than the maximum message allowed by the port.
.

MessageId=547
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_QUOTA_LOWER
Language=Bulgarian
ERROR_INVALID_QUOTA_LOWER - An attempt was made to lower a quota limit below the current usage.
.

MessageId=548
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ALREADY_ATTACHED
Language=Bulgarian
ERROR_DEVICE_ALREADY_ATTACHED - An attempt was made to attach to a device that was already attached to another device.
.

MessageId=549
Severity=Success
Facility=System
SymbolicName=ERROR_INSTRUCTION_MISALIGNMENT
Language=Bulgarian
ERROR_INSTRUCTION_MISALIGNMENT - An attempt was made to execute an instruction at an unaligned address and the host system does not support unaligned instruction references.
.

MessageId=550
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_NOT_STARTED
Language=Bulgarian
ERROR_PROFILING_NOT_STARTED - Profiling not started.
.

MessageId=551
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_NOT_STOPPED
Language=Bulgarian
ERROR_PROFILING_NOT_STOPPED - Profiling not stopped.
.

MessageId=552
Severity=Success
Facility=System
SymbolicName=ERROR_COULD_NOT_INTERPRET
Language=Bulgarian
ERROR_COULD_NOT_INTERPRET - The passed ACL did not contain the minimum required information.
.

MessageId=553
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_AT_LIMIT
Language=Bulgarian
ERROR_PROFILING_AT_LIMIT - The number of active profiling objects is at the maximum and no more may be started.
.

MessageId=554
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_WAIT
Language=Bulgarian
ERROR_CANT_WAIT - Used to indicate that an operation cannot continue without blocking for I/O.
.

MessageId=555
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_TERMINATE_SELF
Language=Bulgarian
ERROR_CANT_TERMINATE_SELF - Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process.
.

MessageId=556
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_CREATE_ERR
Language=Bulgarian
ERROR_UNEXPECTED_MM_CREATE_ERR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=557
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_MAP_ERROR
Language=Bulgarian
ERROR_UNEXPECTED_MM_MAP_ERROR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=558
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_EXTEND_ERR
Language=Bulgarian
ERROR_UNEXPECTED_MM_EXTEND_ERR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=559
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_FUNCTION_TABLE
Language=Bulgarian
ERROR_BAD_FUNCTION_TABLE - A malformed function table was encountered during an unwind operation.
.

MessageId=560
Severity=Success
Facility=System
SymbolicName=ERROR_NO_GUID_TRANSLATION
Language=Bulgarian
ERROR_NO_GUID_TRANSLATION - Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail.
.

MessageId=561
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_SIZE
Language=Bulgarian
ERROR_INVALID_LDT_SIZE - Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors.
.

MessageId=563
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_OFFSET
Language=Bulgarian
ERROR_INVALID_LDT_OFFSET - Indicates that the starting value for the LDT information was not an integral multiple of the selector size.
.

MessageId=564
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_DESCRIPTOR
Language=Bulgarian
ERROR_INVALID_LDT_DESCRIPTOR - Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors.
.

MessageId=565
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_THREADS
Language=Bulgarian
ERROR_TOO_MANY_THREADS - Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads.
.

MessageId=566
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_NOT_IN_PROCESS
Language=Bulgarian
ERROR_THREAD_NOT_IN_PROCESS - An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.
.

MessageId=567
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_PAGEFILE_QUOTA_EXCEEDED - Page file quota was exceeded.
.

MessageId=568
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SERVER_CONFLICT
Language=Bulgarian
ERROR_LOGON_SERVER_CONFLICT - The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role.
.

MessageId=569
Severity=Success
Facility=System
SymbolicName=ERROR_SYNCHRONIZATION_REQUIRED
Language=Bulgarian
ERROR_SYNCHRONIZATION_REQUIRED - The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required.
.

MessageId=570
Severity=Success
Facility=System
SymbolicName=ERROR_NET_OPEN_FAILED
Language=Bulgarian
ERROR_NET_OPEN_FAILED - The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines.
.

MessageId=571
Severity=Success
Facility=System
SymbolicName=ERROR_IO_PRIVILEGE_FAILED
Language=Bulgarian
ERROR_IO_PRIVILEGE_FAILED - The I/O permissions for the process could not be changed.
.

MessageId=572
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROL_C_EXIT
Language=Bulgarian
ERROR_CONTROL_C_EXIT - The application terminated as a result of a CTRL+C.
.

MessageId=573
Severity=Success
Facility=System
SymbolicName=ERROR_MISSING_SYSTEMFILE
Language=Bulgarian
ERROR_MISSING_SYSTEMFILE - The required system file %hs is bad or missing.
.

MessageId=574
Severity=Success
Facility=System
SymbolicName=ERROR_UNHANDLED_EXCEPTION
Language=Bulgarian
ERROR_UNHANDLED_EXCEPTION - The exception %s (0x%08lx) occurred in the application at location 0x%08lx.
.

MessageId=575
Severity=Success
Facility=System
SymbolicName=ERROR_APP_INIT_FAILURE
Language=Bulgarian
ERROR_APP_INIT_FAILURE - The application failed to initialize properly (0x%lx). Click on OK to terminate the application.
.

MessageId=576
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_CREATE_FAILED
Language=Bulgarian
ERROR_PAGEFILE_CREATE_FAILED - The creation of the paging file %hs failed (%lx). The requested size was %ld.
.

MessageId=578
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PAGEFILE
Language=Bulgarian
ERROR_NO_PAGEFILE - No paging file was specified in the system configuration.
.

MessageId=579
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_FLOAT_CONTEXT
Language=Bulgarian
ERROR_ILLEGAL_FLOAT_CONTEXT - A real-mode application issued a floating-point instruction and floating-point hardware is not present.
.

MessageId=580
Severity=Success
Facility=System
SymbolicName=ERROR_NO_EVENT_PAIR
Language=Bulgarian
ERROR_NO_EVENT_PAIR - An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread.
.

MessageId=581
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CTRLR_CONFIG_ERROR
Language=Bulgarian
ERROR_DOMAIN_CTRLR_CONFIG_ERROR - A Windows Server has an incorrect configuration.
.

MessageId=582
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_CHARACTER
Language=Bulgarian
ERROR_ILLEGAL_CHARACTER - An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE.
.

MessageId=583
Severity=Success
Facility=System
SymbolicName=ERROR_UNDEFINED_CHARACTER
Language=Bulgarian
ERROR_UNDEFINED_CHARACTER - The Unicode character is not defined in the Unicode character set installed on the system.
.

MessageId=584
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_VOLUME
Language=Bulgarian
ERROR_FLOPPY_VOLUME - The paging file cannot be created on a floppy diskette.
.

MessageId=585
Severity=Success
Facility=System
SymbolicName=ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT
Language=Bulgarian
ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT - The system bios failed to connect a system interrupt to the device or bus for which the device is connected.
.

MessageId=586
Severity=Success
Facility=System
SymbolicName=ERROR_BACKUP_CONTROLLER
Language=Bulgarian
ERROR_BACKUP_CONTROLLER - This operation is only allowed for the Primary Domain Controller of the domain.
.

MessageId=587
Severity=Success
Facility=System
SymbolicName=ERROR_MUTANT_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_MUTANT_LIMIT_EXCEEDED - An attempt was made to acquire a mutant such that its maximum count would have been exceeded.
.

MessageId=588
Severity=Success
Facility=System
SymbolicName=ERROR_FS_DRIVER_REQUIRED
Language=Bulgarian
ERROR_FS_DRIVER_REQUIRED - A volume has been accessed for which a file system driver is required that has not yet been loaded.
.

MessageId=589
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_LOAD_REGISTRY_FILE
Language=Bulgarian
ERROR_CANNOT_LOAD_REGISTRY_FILE - The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable.
.

MessageId=590
Severity=Success
Facility=System
SymbolicName=ERROR_DEBUG_ATTACH_FAILED
Language=Bulgarian
ERROR_DEBUG_ATTACH_FAILED - An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error.
.

MessageId=591
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_PROCESS_TERMINATED
Language=Bulgarian
ERROR_SYSTEM_PROCESS_TERMINATED - The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down.
.

MessageId=592
Severity=Success
Facility=System
SymbolicName=ERROR_DATA_NOT_ACCEPTED
Language=Bulgarian
ERROR_DATA_NOT_ACCEPTED - The TDI client could not handle the data received during an indication.
.

MessageId=593
Severity=Success
Facility=System
SymbolicName=ERROR_VDM_HARD_ERROR
Language=Bulgarian
ERROR_VDM_HARD_ERROR - NTVDM encountered a hard error.
.

MessageId=594
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_CANCEL_TIMEOUT
Language=Bulgarian
ERROR_DRIVER_CANCEL_TIMEOUT - The driver %hs failed to complete a cancelled I/O request in the allotted time.
.

MessageId=595
Severity=Success
Facility=System
SymbolicName=ERROR_REPLY_MESSAGE_MISMATCH
Language=Bulgarian
ERROR_REPLY_MESSAGE_MISMATCH - An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message.
.

MessageId=596
Severity=Success
Facility=System
SymbolicName=ERROR_LOST_WRITEBEHIND_DATA
Language=Bulgarian
ERROR_LOST_WRITEBEHIND_DATA - Windows was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.
.

MessageId=597
Severity=Success
Facility=System
SymbolicName=ERROR_CLIENT_SERVER_PARAMETERS_INVALID
Language=Bulgarian
ERROR_CLIENT_SERVER_PARAMETERS_INVALID - The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window.
.

MessageId=598
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_TINY_STREAM
Language=Bulgarian
ERROR_NOT_TINY_STREAM - The stream is not a tiny stream.
.

MessageId=599
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_OVERFLOW_READ
Language=Bulgarian
ERROR_STACK_OVERFLOW_READ - The request must be handled by the stack overflow code.
.

MessageId=600
Severity=Success
Facility=System
SymbolicName=ERROR_CONVERT_TO_LARGE
Language=Bulgarian
ERROR_CONVERT_TO_LARGE - Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream.
.

MessageId=601
Severity=Success
Facility=System
SymbolicName=ERROR_FOUND_OUT_OF_SCOPE
Language=Bulgarian
ERROR_FOUND_OUT_OF_SCOPE - The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation.
.

MessageId=602
Severity=Success
Facility=System
SymbolicName=ERROR_ALLOCATE_BUCKET
Language=Bulgarian
ERROR_ALLOCATE_BUCKET - The bucket array must be grown. Retry transaction after doing so.
.

MessageId=603
Severity=Success
Facility=System
SymbolicName=ERROR_MARSHALL_OVERFLOW
Language=Bulgarian
ERROR_MARSHALL_OVERFLOW - The user/kernel marshalling buffer has overflowed.
.

MessageId=604
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_VARIANT
Language=Bulgarian
ERROR_INVALID_VARIANT - The supplied variant structure contains invalid data.
.

MessageId=605
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_COMPRESSION_BUFFER
Language=Bulgarian
ERROR_BAD_COMPRESSION_BUFFER - The specified buffer contains ill-formed data.
.

MessageId=606
Severity=Success
Facility=System
SymbolicName=ERROR_AUDIT_FAILED
Language=Bulgarian
ERROR_AUDIT_FAILED - An attempt to generate a security audit failed.
.

MessageId=607
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_RESOLUTION_NOT_SET
Language=Bulgarian
ERROR_TIMER_RESOLUTION_NOT_SET - The timer resolution was not previously set by the current process.
.

MessageId=608
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_LOGON_INFO
Language=Bulgarian
ERROR_INSUFFICIENT_LOGON_INFO - There is insufficient account information to log you on.
.

MessageId=609
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DLL_ENTRYPOINT
Language=Bulgarian
ERROR_BAD_DLL_ENTRYPOINT - The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly.
.

MessageId=610
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_SERVICE_ENTRYPOINT
Language=Bulgarian
ERROR_BAD_SERVICE_ENTRYPOINT - The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly.
.

MessageId=611
Severity=Success
Facility=System
SymbolicName=ERROR_IP_ADDRESS_CONFLICT1
Language=Bulgarian
ERROR_IP_ADDRESS_CONFLICT1 - There is an IP address conflict with another system on the network
.

MessageId=612
Severity=Success
Facility=System
SymbolicName=ERROR_IP_ADDRESS_CONFLICT2
Language=Bulgarian
ERROR_IP_ADDRESS_CONFLICT2 - There is an IP address conflict with another system on the network
.

MessageId=613
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_QUOTA_LIMIT
Language=Bulgarian
ERROR_REGISTRY_QUOTA_LIMIT - The system has reached the maximum size allowed for the system part of the registry. Additional storage requests will be ignored.
.

MessageId=614
Severity=Success
Facility=System
SymbolicName=ERROR_NO_CALLBACK_ACTIVE
Language=Bulgarian
ERROR_NO_CALLBACK_ACTIVE - A callback return system service cannot be executed when no callback is active.
.

MessageId=615
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_TOO_SHORT
Language=Bulgarian
ERROR_PWD_TOO_SHORT - The password provided is too short to meet the policy of your user account. Please choose a longer password.
.

MessageId=616
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_TOO_RECENT
Language=Bulgarian
ERROR_PWD_TOO_RECENT - The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned.
.

MessageId=617
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_HISTORY_CONFLICT
Language=Bulgarian
ERROR_PWD_HISTORY_CONFLICT - You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used.
.

MessageId=618
Severity=Success
Facility=System
SymbolicName=ERROR_UNSUPPORTED_COMPRESSION
Language=Bulgarian
ERROR_UNSUPPORTED_COMPRESSION - The specified compression format is unsupported.
.

MessageId=619
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HW_PROFILE
Language=Bulgarian
ERROR_INVALID_HW_PROFILE - The specified hardware profile configuration is invalid.
.

MessageId=620
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PLUGPLAY_DEVICE_PATH
Language=Bulgarian
ERROR_INVALID_PLUGPLAY_DEVICE_PATH - The specified Plug and Play registry device path is invalid.
.

MessageId=621
Severity=Success
Facility=System
SymbolicName=ERROR_QUOTA_LIST_INCONSISTENT
Language=Bulgarian
ERROR_QUOTA_LIST_INCONSISTENT - The specified quota list is internally inconsistent with its descriptor.
.

MessageId=622
Severity=Success
Facility=System
SymbolicName=ERROR_EVALUATION_EXPIRATION
Language=Bulgarian
ERROR_EVALUATION_EXPIRATION - The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product.
.

MessageId=623
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_DLL_RELOCATION
Language=Bulgarian
ERROR_ILLEGAL_DLL_RELOCATION - The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for Windows system DLLs. The vendor supplying the DLL should be contacted for a new DLL.
.

MessageId=624
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_INIT_FAILED_LOGOFF
Language=Bulgarian
ERROR_DLL_INIT_FAILED_LOGOFF - The application failed to initialize because the window station is shutting down.
.

MessageId=625
Severity=Success
Facility=System
SymbolicName=ERROR_VALIDATE_CONTINUE
Language=Bulgarian
ERROR_VALIDATE_CONTINUE - The validation process needs to continue on to the next step.
.

MessageId=626
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_MATCHES
Language=Bulgarian
ERROR_NO_MORE_MATCHES - There are no more matches for the current index enumeration.
.

MessageId=627
Severity=Success
Facility=System
SymbolicName=ERROR_RANGE_LIST_CONFLICT
Language=Bulgarian
ERROR_RANGE_LIST_CONFLICT - The range could not be added to the range list because of a conflict.
.

MessageId=628
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_SID_MISMATCH
Language=Bulgarian
ERROR_SERVER_SID_MISMATCH - The server process is running under a SID different than that required by client.
.

MessageId=629
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ENABLE_DENY_ONLY
Language=Bulgarian
ERROR_CANT_ENABLE_DENY_ONLY - A group marked use for deny only cannot be enabled.
.

MessageId=630
Severity=Success
Facility=System
SymbolicName=ERROR_FLOAT_MULTIPLE_FAULTS
Language=Bulgarian
ERROR_FLOAT_MULTIPLE_FAULTS - Multiple floating point faults.
.

MessageId=631
Severity=Success
Facility=System
SymbolicName=ERROR_FLOAT_MULTIPLE_TRAPS
Language=Bulgarian
ERROR_FLOAT_MULTIPLE_TRAPS - Multiple floating point traps.
.

MessageId=632
Severity=Success
Facility=System
SymbolicName=ERROR_NOINTERFACE
Language=Bulgarian
ERROR_NOINTERFACE - The requested interface is not supported.
.

MessageId=633
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_FAILED_SLEEP
Language=Bulgarian
ERROR_DRIVER_FAILED_SLEEP - The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode.
.

MessageId=634
Severity=Success
Facility=System
SymbolicName=ERROR_CORRUPT_SYSTEM_FILE
Language=Bulgarian
ERROR_CORRUPT_SYSTEM_FILE - The system file %1 has become corrupt and has been replaced.
.

MessageId=635
Severity=Success
Facility=System
SymbolicName=ERROR_COMMITMENT_MINIMUM
Language=Bulgarian
ERROR_COMMITMENT_MINIMUM - Your system is low on virtual memory. Windows is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help.
.

MessageId=636
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_RESTART_ENUMERATION
Language=Bulgarian
ERROR_PNP_RESTART_ENUMERATION - A device was removed so enumeration must be restarted.
.

MessageId=637
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_IMAGE_BAD_SIGNATURE
Language=Bulgarian
ERROR_SYSTEM_IMAGE_BAD_SIGNATURE - The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down.
.

MessageId=638
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_REBOOT_REQUIRED
Language=Bulgarian
ERROR_PNP_REBOOT_REQUIRED - Device will not start without a reboot.
.

MessageId=639
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_POWER
Language=Bulgarian
ERROR_INSUFFICIENT_POWER - There is not enough power to complete the requested operation.
.

MessageId=641
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_SHUTDOWN
Language=Bulgarian
ERROR_SYSTEM_SHUTDOWN - The system is in the process of shutting down.
.

MessageId=642
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_NOT_SET
Language=Bulgarian
ERROR_PORT_NOT_SET - An attempt to remove a processes DebugPort was made, but a port was not already associated with the process.
.

MessageId=643
Severity=Success
Facility=System
SymbolicName=ERROR_DS_VERSION_CHECK_FAILURE
Language=Bulgarian
ERROR_DS_VERSION_CHECK_FAILURE - This version of Windows is not compatible with the behavior version of directory forest, domain or domain controller.
.

MessageId=644
Severity=Success
Facility=System
SymbolicName=ERROR_RANGE_NOT_FOUND
Language=Bulgarian
ERROR_RANGE_NOT_FOUND - The specified range could not be found in the range list.
.

MessageId=646
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAFE_MODE_DRIVER
Language=Bulgarian
ERROR_NOT_SAFE_MODE_DRIVER - The driver was not loaded because the system is booting into safe mode.
.

MessageId=647
Severity=Success
Facility=System
SymbolicName=ERROR_FAILED_DRIVER_ENTRY
Language=Bulgarian
ERROR_FAILED_DRIVER_ENTRY - The driver was not loaded because it failed its initialization call.
.

MessageId=648
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ENUMERATION_ERROR
Language=Bulgarian
ERROR_DEVICE_ENUMERATION_ERROR - The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection.
.

MessageId=649
Severity=Success
Facility=System
SymbolicName=ERROR_MOUNT_POINT_NOT_RESOLVED
Language=Bulgarian
ERROR_MOUNT_POINT_NOT_RESOLVED - The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached.
.

MessageId=650
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DEVICE_OBJECT_PARAMETER
Language=Bulgarian
ERROR_INVALID_DEVICE_OBJECT_PARAMETER - The device object parameter is either not a valid device object or is not attached to the volume specified by the file name.
.

MessageId=651
Severity=Success
Facility=System
SymbolicName=ERROR_MCA_OCCURED
Language=Bulgarian
ERROR_MCA_OCCURED - A Machine Check Error has occurred. Please check the system eventlog for additional information.
.

MessageId=652
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_DATABASE_ERROR
Language=Bulgarian
ERROR_DRIVER_DATABASE_ERROR - There was error [%2] processing the driver database.
.

MessageId=653
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_HIVE_TOO_LARGE
Language=Bulgarian
ERROR_SYSTEM_HIVE_TOO_LARGE - System hive size has exceeded its limit.
.

MessageId=654
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_FAILED_PRIOR_UNLOAD
Language=Bulgarian
ERROR_DRIVER_FAILED_PRIOR_UNLOAD - The driver could not be loaded because a previous version of the driver is still in memory.
.

MessageId=655
Severity=Success
Facility=System
SymbolicName=ERROR_VOLSNAP_PREPARE_HIBERNATE
Language=Bulgarian
ERROR_VOLSNAP_PREPARE_HIBERNATE - Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation.
.

MessageId=656
Severity=Success
Facility=System
SymbolicName=ERROR_HIBERNATION_FAILURE
Language=Bulgarian
ERROR_HIBERNATION_FAILURE - The system has failed to hibernate (The error code is %hs). Hibernation will be disabled until the system is restarted.
.

MessageId=657
Severity=Success
Facility=System
SymbolicName=ERROR_HUNG_DISPLAY_DRIVER_THREAD
Language=Bulgarian
ERROR_HUNG_DISPLAY_DRIVER_THREAD - The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft.
.

MessageId=665
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_SYSTEM_LIMITATION
Language=Bulgarian
ERROR_FILE_SYSTEM_LIMITATION - The requested operation could not be completed due to a file system limitation.
.

MessageId=668
Severity=Success
Facility=System
SymbolicName=ERROR_ASSERTION_FAILURE
Language=Bulgarian
ERROR_ASSERTION_FAILURE - An assertion failure has occurred.
.

MessageId=669
Severity=Success
Facility=System
SymbolicName=ERROR_VERIFIER_STOP
Language=Bulgarian
ERROR_VERIFIER_STOP - Application verifier has found an error in the current process.
.

MessageId=670
Severity=Success
Facility=System
SymbolicName=ERROR_WOW_ASSERTION
Language=Bulgarian
ERROR_WOW_ASSERTION - WOW Assertion Error.
.

MessageId=671
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_BAD_MPS_TABLE
Language=Bulgarian
ERROR_PNP_BAD_MPS_TABLE - A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update.
.

MessageId=672
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_TRANSLATION_FAILED
Language=Bulgarian
ERROR_PNP_TRANSLATION_FAILED - A translator failed to translate resources.
.

MessageId=673
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_IRQ_TRANSLATION_FAILED
Language=Bulgarian
ERROR_PNP_IRQ_TRANSLATION_FAILED - A IRQ translator failed to translate resources.
.

MessageId=674
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_INVALID_ID
Language=Bulgarian
ERROR_PNP_INVALID_ID - Driver %2 returned invalid ID for a child device (%3).
.

MessageId=675
Severity=Success
Facility=System
SymbolicName=ERROR_WAKE_SYSTEM_DEBUGGER
Language=Bulgarian
ERROR_WAKE_SYSTEM_DEBUGGER - The system debugger was awakened by an interrupt.
.

MessageId=676
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLES_CLOSED
Language=Bulgarian
ERROR_HANDLES_CLOSED - Handles to objects have been automatically closed as a result of the requested operation.
.

MessageId=677
Severity=Success
Facility=System
SymbolicName=ERROR_EXTRANEOUS_INFORMATION
Language=Bulgarian
ERROR_EXTRANEOUS_INFORMATION - he specified access control list (ACL) contained more information than was expected.
.

MessageId=678
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMIT_NECESSARY
Language=Bulgarian
ERROR_RXACT_COMMIT_NECESSARY - This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired).
.

MessageId=679
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_CHECK
Language=Bulgarian
ERROR_MEDIA_CHECK - The media may have changed.
.

MessageId=680
Severity=Success
Facility=System
SymbolicName=ERROR_GUID_SUBSTITUTION_MADE
Language=Bulgarian
ERROR_GUID_SUBSTITUTION_MADE - During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended.
.

MessageId=681
Severity=Success
Facility=System
SymbolicName=ERROR_STOPPED_ON_SYMLINK
Language=Bulgarian
ERROR_STOPPED_ON_SYMLINK - The create operation stopped after reaching a symbolic link.
.

MessageId=682
Severity=Success
Facility=System
SymbolicName=ERROR_LONGJUMP
Language=Bulgarian
ERROR_LONGJUMP - A long jump has been executed.
.

MessageId=683
Severity=Success
Facility=System
SymbolicName=ERROR_PLUGPLAY_QUERY_VETOED
Language=Bulgarian
ERROR_PLUGPLAY_QUERY_VETOED - The Plug and Play query operation was not successful.
.

MessageId=684
Severity=Success
Facility=System
SymbolicName=ERROR_UNWIND_CONSOLIDATE
Language=Bulgarian
ERROR_UNWIND_CONSOLIDATE - A frame consolidation has been executed.
.

MessageId=685
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_HIVE_RECOVERED
Language=Bulgarian
ERROR_REGISTRY_HIVE_RECOVERED - Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost.
.

MessageId=686
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_MIGHT_BE_INSECURE
Language=Bulgarian
ERROR_DLL_MIGHT_BE_INSECURE - The application is attempting to run executable code from the module %hs. This may be insecure. An alternative, %hs, is available. Should the application use the secure module %hs?
.

MessageId=687
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_MIGHT_BE_INCOMPATIBLE
Language=Bulgarian
ERROR_DLL_MIGHT_BE_INCOMPATIBLE - The application is loading executable code from the module %hs. This is secure, but may be incompatible with previous releases of the operating system. An alternative, %hs, is available. Should the application use the secure module %hs?
.

MessageId=688
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_EXCEPTION_NOT_HANDLED
Language=Bulgarian
ERROR_DBG_EXCEPTION_NOT_HANDLED - Debugger did not handle the exception.
.

MessageId=689
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_REPLY_LATER
Language=Bulgarian
ERROR_DBG_REPLY_LATER - Debugger will reply later.
.

MessageId=690
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE
Language=Bulgarian
ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE - Debugger can not provide handle.
.

MessageId=691
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_TERMINATE_THREAD
Language=Bulgarian
ERROR_DBG_TERMINATE_THREAD - Debugger terminated thread.
.

MessageId=692
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_TERMINATE_PROCESS
Language=Bulgarian
ERROR_DBG_TERMINATE_PROCESS - Debugger terminated process.
.

MessageId=693
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTROL_C
Language=Bulgarian
ERROR_DBG_CONTROL_C - Debugger got control C.
.

MessageId=694
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_PRINTEXCEPTION_C
Language=Bulgarian
ERROR_DBG_PRINTEXCEPTION_C - Debugger printed exception on control C.
.

MessageId=695
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_RIPEXCEPTION
Language=Bulgarian
ERROR_DBG_RIPEXCEPTION - Debugger received RIP exception.
.

MessageId=696
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTROL_BREAK
Language=Bulgarian
ERROR_DBG_CONTROL_BREAK - Debugger received control break.
.

MessageId=697
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_COMMAND_EXCEPTION
Language=Bulgarian
ERROR_DBG_COMMAND_EXCEPTION - Debugger command communication exception.
.

MessageId=698
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_NAME_EXISTS
Language=Bulgarian
ERROR_OBJECT_NAME_EXISTS - An attempt was made to create an object and the object name already existed.
.

MessageId=699
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_WAS_SUSPENDED
Language=Bulgarian
ERROR_THREAD_WAS_SUSPENDED - A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
.

MessageId=700
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_NOT_AT_BASE
Language=Bulgarian
ERROR_IMAGE_NOT_AT_BASE - An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image.
.

MessageId=701
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_STATE_CREATED
Language=Bulgarian
ERROR_RXACT_STATE_CREATED - This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created.
.

MessageId=702
Severity=Success
Facility=System
SymbolicName=ERROR_SEGMENT_NOTIFICATION
Language=Bulgarian
ERROR_SEGMENT_NOTIFICATION - A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments.
.

MessageId=703
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_CURRENT_DIRECTORY
Language=Bulgarian
ERROR_BAD_CURRENT_DIRECTORY - The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit.
.

MessageId=704
Severity=Success
Facility=System
SymbolicName=ERROR_FT_READ_RECOVERY_FROM_BACKUP
Language=Bulgarian
ERROR_FT_READ_RECOVERY_FROM_BACKUP - To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device.
.

MessageId=705
Severity=Success
Facility=System
SymbolicName=ERROR_FT_WRITE_RECOVERY
Language=Bulgarian
ERROR_FT_WRITE_RECOVERY - To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device.
.

MessageId=706
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_MACHINE_TYPE_MISMATCH
Language=Bulgarian
ERROR_IMAGE_MACHINE_TYPE_MISMATCH - The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.
.

MessageId=707
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_PARTIAL
Language=Bulgarian
ERROR_RECEIVE_PARTIAL - The network transport returned partial data to its client. The remaining data will be sent later.
.

MessageId=708
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_EXPEDITED
Language=Bulgarian
ERROR_RECEIVE_EXPEDITED - The network transport returned data to its client that was marked as expedited by the remote system.
.

MessageId=709
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_PARTIAL_EXPEDITED
Language=Bulgarian
ERROR_RECEIVE_PARTIAL_EXPEDITED - The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later.
.

MessageId=710
Severity=Success
Facility=System
SymbolicName=ERROR_EVENT_DONE
Language=Bulgarian
ERROR_EVENT_DONE - The TDI indication has completed successfully.
.

MessageId=711
Severity=Success
Facility=System
SymbolicName=ERROR_EVENT_PENDING
Language=Bulgarian
ERROR_EVENT_PENDING - The TDI indication has entered the pending state.
.

MessageId=712
Severity=Success
Facility=System
SymbolicName=ERROR_CHECKING_FILE_SYSTEM
Language=Bulgarian
ERROR_CHECKING_FILE_SYSTEM - Checking file system on %wZ.
.

MessageId=714
Severity=Success
Facility=System
SymbolicName=ERROR_PREDEFINED_HANDLE
Language=Bulgarian
ERROR_PREDEFINED_HANDLE - The specified registry key is referenced by a predefined handle.
.

MessageId=715
Severity=Success
Facility=System
SymbolicName=ERROR_WAS_UNLOCKED
Language=Bulgarian
ERROR_WAS_UNLOCKED - The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process.
.

MessageId=717
Severity=Success
Facility=System
SymbolicName=ERROR_WAS_LOCKED
Language=Bulgarian
ERROR_WAS_LOCKED - One of the pages to lock was already locked.
.

MessageId=720
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE
Language=Bulgarian
ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE - The image file %hs is valid, but is for a machine type other than the current machine.
.

MessageId=721
Severity=Success
Facility=System
SymbolicName=ERROR_NO_YIELD_PERFORMED
Language=Bulgarian
ERROR_NO_YIELD_PERFORMED - A yield execution was performed and no thread was available to run.
.

MessageId=722
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_RESUME_IGNORED
Language=Bulgarian
ERROR_TIMER_RESUME_IGNORED - The resumable flag to a timer API was ignored.
.

MessageId=723
Severity=Success
Facility=System
SymbolicName=ERROR_ARBITRATION_UNHANDLED
Language=Bulgarian
ERROR_ARBITRATION_UNHANDLED - The arbiter has deferred arbitration of these resources to its parent.
.

MessageId=724
Severity=Success
Facility=System
SymbolicName=ERROR_CARDBUS_NOT_SUPPORTED
Language=Bulgarian
ERROR_CARDBUS_NOT_SUPPORTED - The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller.
.

MessageId=725
Severity=Success
Facility=System
SymbolicName=ERROR_MP_PROCESSOR_MISMATCH
Language=Bulgarian
ERROR_MP_PROCESSOR_MISMATCH - The CPUs in this multiprocessor system are not all the same revision level. To use all processors the operating system restricts itself to the features of the least capable processor in the system. Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported.
.

MessageId=726
Severity=Success
Facility=System
SymbolicName=ERROR_HIBERNATED
Language=Bulgarian
ERROR_HIBERNATED - The system was put into hibernation.
.

MessageId=727
Severity=Success
Facility=System
SymbolicName=ERROR_RESUME_HIBERNATION
Language=Bulgarian
ERROR_RESUME_HIBERNATION - The system was resumed from hibernation.
.

MessageId=728
Severity=Success
Facility=System
SymbolicName=ERROR_FIRMWARE_UPDATED
Language=Bulgarian
ERROR_FIRMWARE_UPDATED - Windows has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3].
.

MessageId=729
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVERS_LEAKING_LOCKED_PAGES
Language=Bulgarian
ERROR_DRIVERS_LEAKING_LOCKED_PAGES - A device driver is leaking locked I/O pages causing system degradation. The system has automatically enabled tracking code in order to try and catch the culprit.
.

MessageId=730
Severity=Success
Facility=System
SymbolicName=ERROR_WAKE_SYSTEM
Language=Bulgarian
ERROR_WAKE_SYSTEM - The system has awoken
.

MessageId=741
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE
Language=Bulgarian
ERROR_REPARSE - A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.

MessageId=742
Severity=Success
Facility=System
SymbolicName=ERROR_OPLOCK_BREAK_IN_PROGRESS
Language=Bulgarian
ERROR_OPLOCK_BREAK_IN_PROGRESS - An open/create operation completed while an oplock break is underway.
.

MessageId=743
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_MOUNTED
Language=Bulgarian
ERROR_VOLUME_MOUNTED - A new volume has been mounted by a file system.
.

MessageId=744
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMITTED
Language=Bulgarian
ERROR_RXACT_COMMITTED - This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed.
.

MessageId=745
Severity=Success
Facility=System
SymbolicName=ERROR_NOTIFY_CLEANUP
Language=Bulgarian
ERROR_NOTIFY_CLEANUP - This indicates that a notify change request has been completed due to closing the handle which made the notify change request.
.

MessageId=746
Severity=Success
Facility=System
SymbolicName=ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED
Language=Bulgarian
ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED - An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport.
.

MessageId=747
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_TRANSITION
Language=Bulgarian
ERROR_PAGE_FAULT_TRANSITION - Page fault was a transition fault.
.

MessageId=748
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_DEMAND_ZERO
Language=Bulgarian
ERROR_PAGE_FAULT_DEMAND_ZERO - Page fault was a demand zero fault.
.

MessageId=749
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_COPY_ON_WRITE
Language=Bulgarian
ERROR_PAGE_FAULT_COPY_ON_WRITE - Page fault was a demand zero fault.
.

MessageId=750
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_GUARD_PAGE
Language=Bulgarian
ERROR_PAGE_FAULT_GUARD_PAGE - Page fault was a demand zero fault.
.

MessageId=751
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_PAGING_FILE
Language=Bulgarian
ERROR_PAGE_FAULT_PAGING_FILE - Page fault was satisfied by reading from a secondary storage device.
.

MessageId=752
Severity=Success
Facility=System
SymbolicName=ERROR_CACHE_PAGE_LOCKED
Language=Bulgarian
ERROR_CACHE_PAGE_LOCKED - Cached page was locked during operation.
.

MessageId=753
Severity=Success
Facility=System
SymbolicName=ERROR_CRASH_DUMP
Language=Bulgarian
ERROR_CRASH_DUMP - Crash dump exists in paging file.
.

MessageId=754
Severity=Success
Facility=System
SymbolicName=ERROR_BUFFER_ALL_ZEROS
Language=Bulgarian
ERROR_BUFFER_ALL_ZEROS - Specified buffer contains all zeros.
.

MessageId=755
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_OBJECT
Language=Bulgarian
ERROR_REPARSE_OBJECT - A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.

MessageId=756
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_REQUIREMENTS_CHANGED
Language=Bulgarian
ERROR_RESOURCE_REQUIREMENTS_CHANGED - The device has succeeded a query-stop and its resource requirements have changed.
.

MessageId=757
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSLATION_COMPLETE
Language=Bulgarian
ERROR_TRANSLATION_COMPLETE - The translator has translated these resources into the global space and no further translations should be performed.
.

MessageId=758
Severity=Success
Facility=System
SymbolicName=ERROR_NOTHING_TO_TERMINATE
Language=Bulgarian
ERROR_NOTHING_TO_TERMINATE - A process being terminated has no threads to terminate.
.

MessageId=759
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_NOT_IN_JOB
Language=Bulgarian
ERROR_PROCESS_NOT_IN_JOB - The specified process is not part of a job.
.

MessageId=760
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_IN_JOB
Language=Bulgarian
ERROR_PROCESS_IN_JOB - The specified process is part of a job.
.

MessageId=761
Severity=Success
Facility=System
SymbolicName=ERROR_VOLSNAP_HIBERNATE_READY
Language=Bulgarian
ERROR_VOLSNAP_HIBERNATE_READY - The system is now ready for hibernation.
.

MessageId=762
Severity=Success
Facility=System
SymbolicName=ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY
Language=Bulgarian
ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY - A file system or file system filter driver has successfully completed an FsFilter operation.
.

MessageId=763
Severity=Success
Facility=System
SymbolicName=ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED
Language=Bulgarian
ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED - The specified interrupt vector was already connected.
.

MessageId=764
Severity=Success
Facility=System
SymbolicName=ERROR_INTERRUPT_STILL_CONNECTED
Language=Bulgarian
ERROR_INTERRUPT_STILL_CONNECTED - The specified interrupt vector is still connected.
.

MessageId=765
Severity=Success
Facility=System
SymbolicName=ERROR_WAIT_FOR_OPLOCK
Language=Bulgarian
ERROR_WAIT_FOR_OPLOCK - An operation is blocked waiting for an oplock.
.

MessageId=766
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_EXCEPTION_HANDLED
Language=Bulgarian
ERROR_DBG_EXCEPTION_HANDLED - Debugger handled exception.
.

MessageId=767
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTINUE
Language=Bulgarian
ERROR_DBG_CONTINUE - Debugger continued
.

MessageId=768
Severity=Success
Facility=System
SymbolicName=ERROR_CALLBACK_POP_STACK
Language=Bulgarian
ERROR_CALLBACK_POP_STACK - An exception occurred in a user mode callback and the kernel callback frame should be removed.
.

MessageId=769
Severity=Success
Facility=System
SymbolicName=ERROR_COMPRESSION_DISABLED
Language=Bulgarian
ERROR_COMPRESSION_DISABLED - Compression is disabled for this volume.
.

MessageId=770
Severity=Success
Facility=System
SymbolicName=ERROR_CANTFETCHBACKWARDS
Language=Bulgarian
ERROR_CANTFETCHBACKWARDS - The data provider cannot fetch backwards through a result set.
.

MessageId=771
Severity=Success
Facility=System
SymbolicName=ERROR_CANTSCROLLBACKWARDS
Language=Bulgarian
ERROR_CANTSCROLLBACKWARDS - The data provider cannot scroll backwards through a result set.
.

MessageId=772
Severity=Success
Facility=System
SymbolicName=ERROR_ROWSNOTRELEASED
Language=Bulgarian
ERROR_ROWSNOTRELEASED - The data provider requires that previously fetched data is released before asking for more data.
.

MessageId=773
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ACCESSOR_FLAGS
Language=Bulgarian
ERROR_BAD_ACCESSOR_FLAGS - The data provider was not able to interpret the flags set for a column binding in an accessor.
.

MessageId=774
Severity=Success
Facility=System
SymbolicName=ERROR_ERRORS_ENCOUNTERED
Language=Bulgarian
ERROR_ERRORS_ENCOUNTERED - One or more errors occurred while processing the request.
.

MessageId=775
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CAPABLE
Language=Bulgarian
ERROR_NOT_CAPABLE - The implementation is not capable of performing the request.
.

MessageId=776
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_OUT_OF_SEQUENCE
Language=Bulgarian
ERROR_REQUEST_OUT_OF_SEQUENCE - The client of a component requested an operation which is not valid given the state of the component instance.
.

MessageId=777
Severity=Success
Facility=System
SymbolicName=ERROR_VERSION_PARSE_ERROR
Language=Bulgarian
ERROR_VERSION_PARSE_ERROR - A version number could not be parsed.
.

MessageId=778
Severity=Success
Facility=System
SymbolicName=ERROR_BADSTARTPOSITION
Language=Bulgarian
ERROR_BADSTARTPOSITION - The iterator's start position is invalid.
.

MessageId=994
Severity=Success
Facility=System
SymbolicName=ERROR_EA_ACCESS_DENIED
Language=Bulgarian
ERROR_EA_ACCESS_DENIED - Access to the extended attribute was denied.
.

MessageId=995
Severity=Success
Facility=System
SymbolicName=ERROR_OPERATION_ABORTED
Language=Bulgarian
ERROR_OPERATION_ABORTED - The I/O operation has been aborted because of either a thread exit or an application request.
.

MessageId=996
Severity=Success
Facility=System
SymbolicName=ERROR_IO_INCOMPLETE
Language=Bulgarian
ERROR_IO_INCOMPLETE - Overlapped I/O event is not in a signaled state.
.

MessageId=997
Severity=Success
Facility=System
SymbolicName=ERROR_IO_PENDING
Language=Bulgarian
ERROR_IO_PENDING - Overlapped I/O operation is in progress.
.

MessageId=998
Severity=Success
Facility=System
SymbolicName=ERROR_NOACCESS
Language=Bulgarian
ERROR_NOACCESS - Invalid access to memory location.
.

MessageId=999
Severity=Success
Facility=System
SymbolicName=ERROR_SWAPERROR
Language=Bulgarian
ERROR_SWAPERROR - Error performing inpage operation.
.

MessageId=1001
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_OVERFLOW
Language=Bulgarian
ERROR_STACK_OVERFLOW - Recursion too deep; the stack overflowed.
.

MessageId=1002
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGE
Language=Bulgarian
ERROR_INVALID_MESSAGE - The window cannot act on the sent message.
.

MessageId=1003
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_COMPLETE
Language=Bulgarian
ERROR_CAN_NOT_COMPLETE - Cannot complete this function.
.

MessageId=1004
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAGS
Language=Bulgarian
ERROR_INVALID_FLAGS - Invalid flags.
.

MessageId=1005
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_VOLUME
Language=Bulgarian
ERROR_UNRECOGNIZED_VOLUME - The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted.
.

MessageId=1006
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_INVALID
Language=Bulgarian
ERROR_FILE_INVALID - The volume for a file has been externally altered so that the opened file is no longer valid.
.

MessageId=1007
Severity=Success
Facility=System
SymbolicName=ERROR_FULLSCREEN_MODE
Language=Bulgarian
ERROR_FULLSCREEN_MODE - The requested operation cannot be performed in full-screen mode.
.

MessageId=1008
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TOKEN
Language=Bulgarian
ERROR_NO_TOKEN - An attempt was made to reference a token that does not exist.
.

MessageId=1009
Severity=Success
Facility=System
SymbolicName=ERROR_BADDB
Language=Bulgarian
ERROR_BADDB - The configuration registry database is corrupt.
.

MessageId=1010
Severity=Success
Facility=System
SymbolicName=ERROR_BADKEY
Language=Bulgarian
ERROR_BADKEY - The configuration registry key is invalid.
.

MessageId=1011
Severity=Success
Facility=System
SymbolicName=ERROR_CANTOPEN
Language=Bulgarian
ERROR_CANTOPEN - The configuration registry key could not be opened.
.

MessageId=1012
Severity=Success
Facility=System
SymbolicName=ERROR_CANTREAD
Language=Bulgarian
ERROR_CANTREAD - The configuration registry key could not be read.
.

MessageId=1013
Severity=Success
Facility=System
SymbolicName=ERROR_CANTWRITE
Language=Bulgarian
ERROR_CANTWRITE - The configuration registry key could not be written.
.

MessageId=1014
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_RECOVERED
Language=Bulgarian
ERROR_REGISTRY_RECOVERED - One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.
.

MessageId=1015
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_CORRUPT
Language=Bulgarian
ERROR_REGISTRY_CORRUPT - The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.
.

MessageId=1016
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_IO_FAILED
Language=Bulgarian
ERROR_REGISTRY_IO_FAILED - An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.
.

MessageId=1017
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_REGISTRY_FILE
Language=Bulgarian
ERROR_NOT_REGISTRY_FILE - The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.
.

MessageId=1018
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_DELETED
Language=Bulgarian
ERROR_KEY_DELETED - Illegal operation attempted on a registry key that has been marked for deletion.
.

MessageId=1019
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOG_SPACE
Language=Bulgarian
ERROR_NO_LOG_SPACE - System could not allocate the required space in a registry log.
.

MessageId=1020
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_HAS_CHILDREN
Language=Bulgarian
ERROR_KEY_HAS_CHILDREN - Cannot create a symbolic link in a registry key that already has subkeys or values.
.

MessageId=1021
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_MUST_BE_VOLATILE
Language=Bulgarian
ERROR_CHILD_MUST_BE_VOLATILE - Cannot create a stable subkey under a volatile parent key.
.

MessageId=1022
Severity=Success
Facility=System
SymbolicName=ERROR_NOTIFY_ENUM_DIR
Language=Bulgarian
ERROR_NOTIFY_ENUM_DIR - A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.
.

MessageId=1051
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENT_SERVICES_RUNNING
Language=Bulgarian
ERROR_DEPENDENT_SERVICES_RUNNING - A stop control has been sent to a service that other running services are dependent on.
.

MessageId=1052
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_CONTROL
Language=Bulgarian
ERROR_INVALID_SERVICE_CONTROL - The requested control is not valid for this service.
.

MessageId=1053
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_REQUEST_TIMEOUT
Language=Bulgarian
ERROR_SERVICE_REQUEST_TIMEOUT - The service did not respond to the start or control request in a timely fashion.
.

MessageId=1054
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NO_THREAD
Language=Bulgarian
ERROR_SERVICE_NO_THREAD - A thread could not be created for the service.
.

MessageId=1055
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DATABASE_LOCKED
Language=Bulgarian
ERROR_SERVICE_DATABASE_LOCKED - The service database is locked.
.

MessageId=1056
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_ALREADY_RUNNING
Language=Bulgarian
ERROR_SERVICE_ALREADY_RUNNING - An instance of the service is already running.
.

MessageId=1057
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_ACCOUNT
Language=Bulgarian
ERROR_INVALID_SERVICE_ACCOUNT - The account name is invalid or does not exist, or the password is invalid for the account name specified.
.

MessageId=1058
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DISABLED
Language=Bulgarian
ERROR_SERVICE_DISABLED - The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.
.

MessageId=1059
Severity=Success
Facility=System
SymbolicName=ERROR_CIRCULAR_DEPENDENCY
Language=Bulgarian
ERROR_CIRCULAR_DEPENDENCY - Circular service dependency was specified.
.

MessageId=1060
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DOES_NOT_EXIST
Language=Bulgarian
ERROR_SERVICE_DOES_NOT_EXIST - The specified service does not exist as an installed service.
.

MessageId=1061
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_CANNOT_ACCEPT_CTRL
Language=Bulgarian
ERROR_SERVICE_CANNOT_ACCEPT_CTRL - The service cannot accept control messages at this time.
.

MessageId=1062
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_ACTIVE
Language=Bulgarian
ERROR_SERVICE_NOT_ACTIVE - The service has not been started.
.

MessageId=1063
Severity=Success
Facility=System
SymbolicName=ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
Language=Bulgarian
ERROR_FAILED_SERVICE_CONTROLLER_CONNECT - The service process could not connect to the service controller.
.

MessageId=1064
Severity=Success
Facility=System
SymbolicName=ERROR_EXCEPTION_IN_SERVICE
Language=Bulgarian
ERROR_EXCEPTION_IN_SERVICE - An exception occurred in the service when handling the control request.
.

MessageId=1065
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_DOES_NOT_EXIST
Language=Bulgarian
ERROR_DATABASE_DOES_NOT_EXIST - The database specified does not exist.
.

MessageId=1066
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_SPECIFIC_ERROR
Language=Bulgarian
ERROR_SERVICE_SPECIFIC_ERROR - The service has returned a service-specific error code.
.

MessageId=1067
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_ABORTED
Language=Bulgarian
ERROR_PROCESS_ABORTED - The process terminated unexpectedly.
.

MessageId=1068
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_FAIL
Language=Bulgarian
ERROR_SERVICE_DEPENDENCY_FAIL - The dependency service or group failed to start.
.

MessageId=1069
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_LOGON_FAILED
Language=Bulgarian
ERROR_SERVICE_LOGON_FAILED - The service did not start due to a logon failure.
.

MessageId=1070
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_START_HANG
Language=Bulgarian
ERROR_SERVICE_START_HANG - After starting, the service hung in a start-pending state.
.

MessageId=1071
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_LOCK
Language=Bulgarian
ERROR_INVALID_SERVICE_LOCK - The specified service database lock is invalid.
.

MessageId=1072
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_MARKED_FOR_DELETE
Language=Bulgarian
ERROR_SERVICE_MARKED_FOR_DELETE - The specified service has been marked for deletion.
.

MessageId=1073
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_EXISTS
Language=Bulgarian
ERROR_SERVICE_EXISTS - The specified service already exists.
.

MessageId=1074
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_RUNNING_LKG
Language=Bulgarian
ERROR_ALREADY_RUNNING_LKG - The system is currently running with the last-known-good configuration.
.

MessageId=1075
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_DELETED
Language=Bulgarian
ERROR_SERVICE_DEPENDENCY_DELETED - The dependency service does not exist or has been marked for deletion.
.

MessageId=1076
Severity=Success
Facility=System
SymbolicName=ERROR_BOOT_ALREADY_ACCEPTED
Language=Bulgarian
ERROR_BOOT_ALREADY_ACCEPTED - The current boot has already been accepted for use as the last-known-good control set.
.

MessageId=1077
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NEVER_STARTED
Language=Bulgarian
ERROR_SERVICE_NEVER_STARTED - No attempts to start the service have been made since the last boot.
.

MessageId=1078
Severity=Success
Facility=System
SymbolicName=ERROR_DUPLICATE_SERVICE_NAME
Language=Bulgarian
ERROR_DUPLICATE_SERVICE_NAME - The name is already in use as either a service name or a service display name.
.

MessageId=1079
Severity=Success
Facility=System
SymbolicName=ERROR_DIFFERENT_SERVICE_ACCOUNT
Language=Bulgarian
ERROR_DIFFERENT_SERVICE_ACCOUNT - The account specified for this service is different from the account specified for other services running in the same process.
.

MessageId=1080
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_DETECT_DRIVER_FAILURE
Language=Bulgarian
ERROR_CANNOT_DETECT_DRIVER_FAILURE - Failure actions can only be set for Win32 services, not for drivers.
.

MessageId=1081
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_DETECT_PROCESS_ABORT
Language=Bulgarian
ERROR_CANNOT_DETECT_PROCESS_ABORT - This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.
.

MessageId=1082
Severity=Success
Facility=System
SymbolicName=ERROR_NO_RECOVERY_PROGRAM
Language=Bulgarian
ERROR_NO_RECOVERY_PROGRAM - No recovery program has been configured for this service.
.

MessageId=1083
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_IN_EXE
Language=Bulgarian
ERROR_SERVICE_NOT_IN_EXE - The executable program that this service is configured to run in does not implement the service.
.

MessageId=1084
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAFEBOOT_SERVICE
Language=Bulgarian
ERROR_NOT_SAFEBOOT_SERVICE - This service cannot be started in Safe Mode.
.

MessageId=1100
Severity=Success
Facility=System
SymbolicName=ERROR_END_OF_MEDIA
Language=Bulgarian
ERROR_END_OF_MEDIA - The physical end of the tape has been reached.
.

MessageId=1101
Severity=Success
Facility=System
SymbolicName=ERROR_FILEMARK_DETECTED
Language=Bulgarian
ERROR_FILEMARK_DETECTED - A tape access reached a filemark.
.

MessageId=1102
Severity=Success
Facility=System
SymbolicName=ERROR_BEGINNING_OF_MEDIA
Language=Bulgarian
ERROR_BEGINNING_OF_MEDIA - The beginning of the tape or a partition was encountered.
.

MessageId=1103
Severity=Success
Facility=System
SymbolicName=ERROR_SETMARK_DETECTED
Language=Bulgarian
ERROR_SETMARK_DETECTED - A tape access reached the end of a set of files.
.

MessageId=1104
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA_DETECTED
Language=Bulgarian
ERROR_NO_DATA_DETECTED - No more data is on the tape.
.

MessageId=1105
Severity=Success
Facility=System
SymbolicName=ERROR_PARTITION_FAILURE
Language=Bulgarian
ERROR_PARTITION_FAILURE - Tape could not be partitioned.
.

MessageId=1106
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK_LENGTH
Language=Bulgarian
ERROR_INVALID_BLOCK_LENGTH - When accessing a new tape of a multivolume partition, the current block size is incorrect.
.

MessageId=1107
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_PARTITIONED
Language=Bulgarian
ERROR_DEVICE_NOT_PARTITIONED - Tape partition information could not be found when loading a tape.
.

MessageId=1108
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_LOCK_MEDIA
Language=Bulgarian
ERROR_UNABLE_TO_LOCK_MEDIA - Unable to lock the media eject mechanism.
.

MessageId=1109
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_UNLOAD_MEDIA
Language=Bulgarian
ERROR_UNABLE_TO_UNLOAD_MEDIA - Unable to unload the media.
.

MessageId=1110
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_CHANGED
Language=Bulgarian
ERROR_MEDIA_CHANGED - The media in the drive may have changed.
.

MessageId=1111
Severity=Success
Facility=System
SymbolicName=ERROR_BUS_RESET
Language=Bulgarian
ERROR_BUS_RESET - The I/O bus was reset.
.

MessageId=1112
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MEDIA_IN_DRIVE
Language=Bulgarian
ERROR_NO_MEDIA_IN_DRIVE - No media in drive.
.

MessageId=1113
Severity=Success
Facility=System
SymbolicName=ERROR_NO_UNICODE_TRANSLATION
Language=Bulgarian
ERROR_NO_UNICODE_TRANSLATION - No mapping for the Unicode character exists in the target multi-byte code page.
.

MessageId=1114
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_INIT_FAILED
Language=Bulgarian
ERROR_DLL_INIT_FAILED - A dynamic link library (DLL) initialization routine failed.
.

MessageId=1115
Severity=Success
Facility=System
SymbolicName=ERROR_SHUTDOWN_IN_PROGRESS
Language=Bulgarian
ERROR_SHUTDOWN_IN_PROGRESS - A system shutdown is in progress.
.

MessageId=1116
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SHUTDOWN_IN_PROGRESS
Language=Bulgarian
ERROR_NO_SHUTDOWN_IN_PROGRESS - Unable to abort the system shutdown because no shutdown was in progress.
.

MessageId=1117
Severity=Success
Facility=System
SymbolicName=ERROR_IO_DEVICE
Language=Bulgarian
ERROR_IO_DEVICE - The request could not be performed because of an I/O device error.
.

MessageId=1118
Severity=Success
Facility=System
SymbolicName=ERROR_SERIAL_NO_DEVICE
Language=Bulgarian
ERROR_SERIAL_NO_DEVICE - No serial device was successfully initialized. The serial driver will unload.
.

MessageId=1119
Severity=Success
Facility=System
SymbolicName=ERROR_IRQ_BUSY
Language=Bulgarian
ERROR_IRQ_BUSY - Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.
.

MessageId=1120
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_WRITES
Language=Bulgarian
ERROR_MORE_WRITES - A serial I/O operation was completed by another write to the serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.)
.

MessageId=1121
Severity=Success
Facility=System
SymbolicName=ERROR_COUNTER_TIMEOUT
Language=Bulgarian
ERROR_COUNTER_TIMEOUT - A serial I/O operation completed because the timeout period expired. (The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)
.

MessageId=1122
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_ID_MARK_NOT_FOUND
Language=Bulgarian
ERROR_FLOPPY_ID_MARK_NOT_FOUND - No ID address mark was found on the floppy disk.
.

MessageId=1123
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_WRONG_CYLINDER
Language=Bulgarian
ERROR_FLOPPY_WRONG_CYLINDER - Mismatch between the floppy disk sector ID field and the floppy disk controller track address.
.

MessageId=1124
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_UNKNOWN_ERROR
Language=Bulgarian
ERROR_FLOPPY_UNKNOWN_ERROR - The floppy disk controller reported an error that is not recognized by the floppy disk driver.
.

MessageId=1125
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_BAD_REGISTERS
Language=Bulgarian
ERROR_FLOPPY_BAD_REGISTERS - The floppy disk controller returned inconsistent results in its registers.
.

MessageId=1126
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RECALIBRATE_FAILED
Language=Bulgarian
ERROR_DISK_RECALIBRATE_FAILED - While accessing the hard disk, a recalibrate operation failed, even after retries.
.

MessageId=1127
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_OPERATION_FAILED
Language=Bulgarian
ERROR_DISK_OPERATION_FAILED - While accessing the hard disk, a disk operation failed even after retries.
.

MessageId=1128
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RESET_FAILED
Language=Bulgarian
ERROR_DISK_RESET_FAILED - While accessing the hard disk, a disk controller reset was needed, but even that failed.
.

MessageId=1129
Severity=Success
Facility=System
SymbolicName=ERROR_EOM_OVERFLOW
Language=Bulgarian
ERROR_EOM_OVERFLOW - Physical end of tape encountered.
.

MessageId=1130
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_SERVER_MEMORY
Language=Bulgarian
ERROR_NOT_ENOUGH_SERVER_MEMORY - Not enough server storage is available to process this command.
.

MessageId=1131
Severity=Success
Facility=System
SymbolicName=ERROR_POSSIBLE_DEADLOCK
Language=Bulgarian
ERROR_POSSIBLE_DEADLOCK - A potential deadlock condition has been detected.
.

MessageId=1132
Severity=Success
Facility=System
SymbolicName=ERROR_MAPPED_ALIGNMENT
Language=Bulgarian
ERROR_MAPPED_ALIGNMENT - The base address or the file offset specified does not have the proper alignment.
.

MessageId=1140
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_VETOED
Language=Bulgarian
ERROR_SET_POWER_STATE_VETOED - An attempt to change the system power state was vetoed by another application or driver.
.

MessageId=1141
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_FAILED
Language=Bulgarian
ERROR_SET_POWER_STATE_FAILED - The system BIOS failed an attempt to change the system power state.
.

MessageId=1142
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LINKS
Language=Bulgarian
ERROR_TOO_MANY_LINKS - An attempt was made to create more links on a file than the file system supports.
.

MessageId=1150
Severity=Success
Facility=System
SymbolicName=ERROR_OLD_WIN_VERSION
Language=Bulgarian
ERROR_OLD_WIN_VERSION - The specified program requires a newer version of Windows.
.

MessageId=1151
Severity=Success
Facility=System
SymbolicName=ERROR_APP_WRONG_OS
Language=Bulgarian
ERROR_APP_WRONG_OS - The specified program is not a Windows or MS-DOS program.
.

MessageId=1152
Severity=Success
Facility=System
SymbolicName=ERROR_SINGLE_INSTANCE_APP
Language=Bulgarian
ERROR_SINGLE_INSTANCE_APP - Cannot start more than one instance of the specified program.
.

MessageId=1153
Severity=Success
Facility=System
SymbolicName=ERROR_RMODE_APP
Language=Bulgarian
ERROR_RMODE_APP - The specified program was written for an earlier version of Windows.
.

MessageId=1154
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DLL
Language=Bulgarian
ERROR_INVALID_DLL - One of the library files needed to run this application is damaged.
.

MessageId=1155
Severity=Success
Facility=System
SymbolicName=ERROR_NO_ASSOCIATION
Language=Bulgarian
ERROR_NO_ASSOCIATION - No application is associated with the specified file for this operation.
.

MessageId=1156
Severity=Success
Facility=System
SymbolicName=ERROR_DDE_FAIL
Language=Bulgarian
ERROR_DDE_FAIL - An error occurred in sending the command to the application.
.

MessageId=1157
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_NOT_FOUND
Language=Bulgarian
ERROR_DLL_NOT_FOUND - One of the library files needed to run this application cannot be found.
.

MessageId=1158
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_USER_HANDLES
Language=Bulgarian
ERROR_NO_MORE_USER_HANDLES - The current process has used all of its system allowance of handles for Window Manager objects.
.

MessageId=1159
Severity=Success
Facility=System
SymbolicName=ERROR_MESSAGE_SYNC_ONLY
Language=Bulgarian
ERROR_MESSAGE_SYNC_ONLY - The message can be used only with synchronous operations.
.

MessageId=1160
Severity=Success
Facility=System
SymbolicName=ERROR_SOURCE_ELEMENT_EMPTY
Language=Bulgarian
ERROR_SOURCE_ELEMENT_EMPTY - The indicated source element has no media.
.

MessageId=1161
Severity=Success
Facility=System
SymbolicName=ERROR_DESTINATION_ELEMENT_FULL
Language=Bulgarian
ERROR_DESTINATION_ELEMENT_FULL - The indicated destination element already contains media.
.

MessageId=1162
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_ELEMENT_ADDRESS
Language=Bulgarian
ERROR_ILLEGAL_ELEMENT_ADDRESS - The indicated element does not exist.
.

MessageId=1163
Severity=Success
Facility=System
SymbolicName=ERROR_MAGAZINE_NOT_PRESENT
Language=Bulgarian
ERROR_MAGAZINE_NOT_PRESENT - The indicated element is part of a magazine that is not present.
.

MessageId=1164
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REINITIALIZATION_NEEDED
Language=Bulgarian
ERROR_DEVICE_REINITIALIZATION_NEEDED - The indicated device requires reinitialization due to hardware errors.
.

MessageId=1165
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REQUIRES_CLEANING
Language=Bulgarian
ERROR_DEVICE_REQUIRES_CLEANING - The device has indicated that cleaning is required before further operations are attempted.
.

MessageId=1166
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_DOOR_OPEN
Language=Bulgarian
ERROR_DEVICE_DOOR_OPEN - The device has indicated that its door is open.
.

MessageId=1167
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_CONNECTED
Language=Bulgarian
ERROR_DEVICE_NOT_CONNECTED - The device is not connected.
.

MessageId=1168
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_FOUND
Language=Bulgarian
ERROR_NOT_FOUND - Element not found.
.

MessageId=1169
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MATCH
Language=Bulgarian
ERROR_NO_MATCH - There was no match for the specified key in the index.
.

MessageId=1170
Severity=Success
Facility=System
SymbolicName=ERROR_SET_NOT_FOUND
Language=Bulgarian
ERROR_SET_NOT_FOUND - The property set specified does not exist on the object.
.

MessageId=1171
Severity=Success
Facility=System
SymbolicName=ERROR_POINT_NOT_FOUND
Language=Bulgarian
ERROR_POINT_NOT_FOUND - The point passed to GetMouseMovePointsEx is not in the buffer.
.

MessageId=1172
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRACKING_SERVICE
Language=Bulgarian
ERROR_NO_TRACKING_SERVICE - The tracking (workstation) service is not running.
.

MessageId=1173
Severity=Success
Facility=System
SymbolicName=ERROR_NO_VOLUME_ID
Language=Bulgarian
ERROR_NO_VOLUME_ID - The Volume ID could not be found.
.

MessageId=1175
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_REMOVE_REPLACED
Language=Bulgarian
ERROR_UNABLE_TO_REMOVE_REPLACED - Unable to remove the file to be replaced.
.

MessageId=1176
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT
Language=Bulgarian
ERROR_UNABLE_TO_MOVE_REPLACEMENT - Unable to move the replacement file to the file to be replaced. The file to be replaced has retained its original name.
.

MessageId=1177
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT_2
Language=Bulgarian
ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 - Unable to move the replacement file to the file to be replaced. The file to be replaced has been renamed using the backup name.
.

MessageId=1178
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_DELETE_IN_PROGRESS
Language=Bulgarian
ERROR_JOURNAL_DELETE_IN_PROGRESS - The volume change journal is being deleted.
.

MessageId=1179
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_NOT_ACTIVE
Language=Bulgarian
ERROR_JOURNAL_NOT_ACTIVE - The volume change journal is not active.
.

MessageId=1180
Severity=Success
Facility=System
SymbolicName=ERROR_POTENTIAL_FILE_FOUND
Language=Bulgarian
ERROR_POTENTIAL_FILE_FOUND - A file was found, but it may not be the correct file.
.

MessageId=1181
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_ENTRY_DELETED
Language=Bulgarian
ERROR_JOURNAL_ENTRY_DELETED - The journal entry has been deleted from the journal.
.

MessageId=1200
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEVICE
Language=Bulgarian
ERROR_BAD_DEVICE - The specified device name is invalid.
.

MessageId=1201
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_UNAVAIL
Language=Bulgarian
ERROR_CONNECTION_UNAVAIL - The device is not currently connected but it is a remembered connection.
.

MessageId=1202
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ALREADY_REMEMBERED
Language=Bulgarian
ERROR_DEVICE_ALREADY_REMEMBERED - The local device name has a remembered connection to another network resource.
.

MessageId=1203
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NET_OR_BAD_PATH
Language=Bulgarian
ERROR_NO_NET_OR_BAD_PATH - The network path was either typed incorrectly, does not exist, or the network provider is not currently available. Please try retyping the path or contact your network administrator.
.

MessageId=1204
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROVIDER
Language=Bulgarian
ERROR_BAD_PROVIDER - The specified network provider name is invalid.
.

MessageId=1205
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_OPEN_PROFILE
Language=Bulgarian
ERROR_CANNOT_OPEN_PROFILE - Unable to open the network connection profile.
.

MessageId=1206
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROFILE
Language=Bulgarian
ERROR_BAD_PROFILE - The network connection profile is corrupted.
.

MessageId=1207
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONTAINER
Language=Bulgarian
ERROR_NOT_CONTAINER - Cannot enumerate a noncontainer.
.

MessageId=1208
Severity=Success
Facility=System
SymbolicName=ERROR_EXTENDED_ERROR
Language=Bulgarian
ERROR_EXTENDED_ERROR - An extended error has occurred.
.

MessageId=1209
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUPNAME
Language=Bulgarian
ERROR_INVALID_GROUPNAME - The format of the specified group name is invalid.
.

MessageId=1210
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMPUTERNAME
Language=Bulgarian
ERROR_INVALID_COMPUTERNAME - The format of the specified computer name is invalid.
.

MessageId=1211
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENTNAME
Language=Bulgarian
ERROR_INVALID_EVENTNAME - The format of the specified event name is invalid.
.

MessageId=1212
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAINNAME
Language=Bulgarian
ERROR_INVALID_DOMAINNAME - The format of the specified domain name is invalid.
.

MessageId=1213
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICENAME
Language=Bulgarian
ERROR_INVALID_SERVICENAME - The format of the specified service name is invalid.
.

MessageId=1214
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NETNAME
Language=Bulgarian
ERROR_INVALID_NETNAME - The format of the specified network name is invalid.
.

MessageId=1215
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHARENAME
Language=Bulgarian
ERROR_INVALID_SHARENAME - The format of the specified share name is invalid.
.

MessageId=1216
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORDNAME
Language=Bulgarian
ERROR_INVALID_PASSWORDNAME - The format of the specified password is invalid.
.

MessageId=1217
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGENAME
Language=Bulgarian
ERROR_INVALID_MESSAGENAME - The format of the specified message name is invalid.
.

MessageId=1218
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGEDEST
Language=Bulgarian
ERROR_INVALID_MESSAGEDEST - The format of the specified message destination is invalid.
.

MessageId=1219
Severity=Success
Facility=System
SymbolicName=ERROR_SESSION_CREDENTIAL_CONFLICT
Language=Bulgarian
ERROR_SESSION_CREDENTIAL_CONFLICT - Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again.
.

MessageId=1220
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_REMOTE_SESSION_LIMIT_EXCEEDED - An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.
.

MessageId=1221
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_DOMAINNAME
Language=Bulgarian
ERROR_DUP_DOMAINNAME - The workgroup or domain name is already in use by another computer on the network.
.

MessageId=1222
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NETWORK
Language=Bulgarian
ERROR_NO_NETWORK - The network is not present or not started.
.

MessageId=1223
Severity=Success
Facility=System
SymbolicName=ERROR_CANCELLED
Language=Bulgarian
ERROR_CANCELLED - The operation was canceled by the user.
.

MessageId=1224
Severity=Success
Facility=System
SymbolicName=ERROR_USER_MAPPED_FILE
Language=Bulgarian
ERROR_USER_MAPPED_FILE - The requested operation cannot be performed on a file with a user-mapped section open.
.

MessageId=1225
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_REFUSED
Language=Bulgarian
ERROR_CONNECTION_REFUSED - The remote system refused the network connection.
.

MessageId=1226
Severity=Success
Facility=System
SymbolicName=ERROR_GRACEFUL_DISCONNECT
Language=Bulgarian
ERROR_GRACEFUL_DISCONNECT - The network connection was gracefully closed.
.

MessageId=1227
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_ALREADY_ASSOCIATED
Language=Bulgarian
ERROR_ADDRESS_ALREADY_ASSOCIATED - The network transport endpoint already has an address associated with it.
.

MessageId=1228
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_NOT_ASSOCIATED
Language=Bulgarian
ERROR_ADDRESS_NOT_ASSOCIATED - An address has not yet been associated with the network endpoint.
.

MessageId=1229
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_INVALID
Language=Bulgarian
ERROR_CONNECTION_INVALID - An operation was attempted on a nonexistent network connection.
.

MessageId=1230
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ACTIVE
Language=Bulgarian
ERROR_CONNECTION_ACTIVE - An invalid operation was attempted on an active network connection.
.

MessageId=1231
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_UNREACHABLE
Language=Bulgarian
ERROR_NETWORK_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1232
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_UNREACHABLE
Language=Bulgarian
ERROR_HOST_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1233
Severity=Success
Facility=System
SymbolicName=ERROR_PROTOCOL_UNREACHABLE
Language=Bulgarian
ERROR_PROTOCOL_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1234
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_UNREACHABLE
Language=Bulgarian
ERROR_PORT_UNREACHABLE - No service is operating at the destination network endpoint on the remote system.
.

MessageId=1235
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_ABORTED
Language=Bulgarian
ERROR_REQUEST_ABORTED - The request was aborted.
.

MessageId=1236
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ABORTED
Language=Bulgarian
ERROR_CONNECTION_ABORTED - The network connection was aborted by the local system.
.

MessageId=1237
Severity=Success
Facility=System
SymbolicName=ERROR_RETRY
Language=Bulgarian
ERROR_RETRY - The operation could not be completed. A retry should be performed.
.

MessageId=1238
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_COUNT_LIMIT
Language=Bulgarian
ERROR_CONNECTION_COUNT_LIMIT - A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.
.

MessageId=1239
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_TIME_RESTRICTION
Language=Bulgarian
ERROR_LOGIN_TIME_RESTRICTION - Attempting to log in during an unauthorized time of day for this account.
.

MessageId=1240
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_WKSTA_RESTRICTION
Language=Bulgarian
ERROR_LOGIN_WKSTA_RESTRICTION - The account is not authorized to log in from this station.
.

MessageId=1241
Severity=Success
Facility=System
SymbolicName=ERROR_INCORRECT_ADDRESS
Language=Bulgarian
ERROR_INCORRECT_ADDRESS - The network address could not be used for the operation requested.
.

MessageId=1242
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_REGISTERED
Language=Bulgarian
ERROR_ALREADY_REGISTERED - The service is already registered.
.

MessageId=1243
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_FOUND
Language=Bulgarian
ERROR_SERVICE_NOT_FOUND - The specified service does not exist.
.

MessageId=1244
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_AUTHENTICATED
Language=Bulgarian
ERROR_NOT_AUTHENTICATED - The operation being requested was not performed because the user has not been authenticated.
.

MessageId=1245
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGGED_ON
Language=Bulgarian
ERROR_NOT_LOGGED_ON - The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist.
.

MessageId=1246
Severity=Success
Facility=System
SymbolicName=ERROR_CONTINUE
Language=Bulgarian
ERROR_CONTINUE - Continue with work in progress.
.

MessageId=1247
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_INITIALIZED
Language=Bulgarian
ERROR_ALREADY_INITIALIZED - An attempt was made to perform an initialization operation when initialization has already been completed.
.

MessageId=1248
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_DEVICES
Language=Bulgarian
ERROR_NO_MORE_DEVICES - No more local devices.
.

MessageId=1249
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_SITE
Language=Bulgarian
ERROR_NO_SUCH_SITE - The specified site does not exist.
.

MessageId=1250
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CONTROLLER_EXISTS
Language=Bulgarian
ERROR_DOMAIN_CONTROLLER_EXISTS - A domain controller with the specified name already exists.
.

MessageId=1251
Severity=Success
Facility=System
SymbolicName=ERROR_ONLY_IF_CONNECTED
Language=Bulgarian
ERROR_ONLY_IF_CONNECTED - This operation is supported only when you are connected to the server.
.

MessageId=1252
Severity=Success
Facility=System
SymbolicName=ERROR_OVERRIDE_NOCHANGES
Language=Bulgarian
ERROR_OVERRIDE_NOCHANGES - The group policy framework should call the extension even if there are no changes.
.

MessageId=1253
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_USER_PROFILE
Language=Bulgarian
ERROR_BAD_USER_PROFILE - The specified user does not have a valid profile.
.

MessageId=1254
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED_ON_SBS
Language=Bulgarian
ERROR_NOT_SUPPORTED_ON_SBS - This operation is not supported on a computer running Windows Server 2003 for Small Business Server.
.

MessageId=1255
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_SHUTDOWN_IN_PROGRESS
Language=Bulgarian
ERROR_SERVER_SHUTDOWN_IN_PROGRESS - The server machine is shutting down.
.

MessageId=1256
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_DOWN
Language=Bulgarian
ERROR_HOST_DOWN - The remote system is not available. For information about network troubleshooting, see Windows Help.
.

MessageId=1257
Severity=Success
Facility=System
SymbolicName=ERROR_NON_ACCOUNT_SID
Language=Bulgarian
ERROR_NON_ACCOUNT_SID - The security identifier provided is not from an account domain.
.

MessageId=1258
Severity=Success
Facility=System
SymbolicName=ERROR_NON_DOMAIN_SID
Language=Bulgarian
ERROR_NON_DOMAIN_SID - The security identifier provided does not have a domain component.
.

MessageId=1259
Severity=Success
Facility=System
SymbolicName=ERROR_APPHELP_BLOCK
Language=Bulgarian
ERROR_APPHELP_BLOCK - AppHelp dialog canceled thus preventing the application from starting.
.

MessageId=1260
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_BY_POLICY
Language=Bulgarian
ERROR_ACCESS_DISABLED_BY_POLICY - Windows cannot open this program because it has been prevented by a software restriction policy. For more information, open Event Viewer or contact your system administrator.
.

MessageId=1261
Severity=Success
Facility=System
SymbolicName=ERROR_REG_NAT_CONSUMPTION
Language=Bulgarian
ERROR_REG_NAT_CONSUMPTION - A program attempt to use an invalid register value. Normally caused by an uninitialized register. This error is Itanium specific.
.

MessageId=1262
Severity=Success
Facility=System
SymbolicName=ERROR_CSCSHARE_OFFLINE
Language=Bulgarian
ERROR_CSCSHARE_OFFLINE - The share is currently offline or does not exist.
.

MessageId=1263
Severity=Success
Facility=System
SymbolicName=ERROR_PKINIT_FAILURE
Language=Bulgarian
ERROR_PKINIT_FAILURE - The kerberos protocol encountered an error while validating the KDC certificate during smartcard logon.
.

MessageId=1264
Severity=Success
Facility=System
SymbolicName=ERROR_SMARTCARD_SUBSYSTEM_FAILURE
Language=Bulgarian
ERROR_SMARTCARD_SUBSYSTEM_FAILURE - The kerberos protocol encountered an error while attempting to utilize the smartcard subsystem.
.

MessageId=1265
Severity=Success
Facility=System
SymbolicName=ERROR_DOWNGRADE_DETECTED
Language=Bulgarian
ERROR_DOWNGRADE_DETECTED - The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you.
.

MessageId=1266
Severity=Success
Facility=System
SymbolicName=SEC_E_SMARTCARD_CERT_REVOKED
Language=Bulgarian
SEC_E_SMARTCARD_CERT_REVOKED - The smartcard certificate used for authentication has been revoked. Please contact your system administrator. There may be additional information in the event log.
.

MessageId=1267
Severity=Success
Facility=System
SymbolicName=SEC_E_ISSUING_CA_UNTRUSTED
Language=Bulgarian
SEC_E_ISSUING_CA_UNTRUSTED - An untrusted certificate authority was detected while processing the smartcard certificate used for authentication. Please contact your system administrator.
.

MessageId=1268
Severity=Success
Facility=System
SymbolicName=SEC_E_REVOCATION_OFFLINE_C
Language=Bulgarian
SEC_E_REVOCATION_OFFLINE_C - The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator.
.

MessageId=1269
Severity=Success
Facility=System
SymbolicName=SEC_E_PKINIT_CLIENT_FAILUR
Language=Bulgarian
SEC_E_PKINIT_CLIENT_FAILUR - The smartcard certificate used for authentication was not trusted. Please contact your system administrator.
.

MessageId=1270
Severity=Success
Facility=System
SymbolicName=SEC_E_SMARTCARD_CERT_EXPIRED
Language=Bulgarian
SEC_E_SMARTCARD_CERT_EXPIRED - The smartcard certificate used for authentication has expired. Please contact your system administrator.
.

MessageId=1271
Severity=Success
Facility=System
SymbolicName=ERROR_MACHINE_LOCKED
Language=Bulgarian
ERROR_MACHINE_LOCKED - The machine is locked and cannot be shut down without the force option.
.

MessageId=1273
Severity=Success
Facility=System
SymbolicName=ERROR_CALLBACK_SUPPLIED_INVALID_DATA
Language=Bulgarian
ERROR_CALLBACK_SUPPLIED_INVALID_DATA - An application-defined callback gave invalid data when called.
.

MessageId=1274
Severity=Success
Facility=System
SymbolicName=ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED
Language=Bulgarian
ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED - The group policy framework should call the extension in the synchronous foreground policy refresh.
.

MessageId=1275
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_BLOCKED
Language=Bulgarian
ERROR_DRIVER_BLOCKED - This driver has been blocked from loading.
.

MessageId=1276
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_IMPORT_OF_NON_DLL
Language=Bulgarian
ERROR_INVALID_IMPORT_OF_NON_DLL - A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image.
.

MessageId=1277
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_WEBBLADE
Language=Bulgarian
ERROR_ACCESS_DISABLED_WEBBLADE - Windows cannot open this program since it has been disabled.
.

MessageId=1278
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER
Language=Bulgarian
ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER - Windows cannot open this program because the license enforcement system has been tampered with or become corrupted.
.

MessageId=1279
Severity=Success
Facility=System
SymbolicName=ERROR_RECOVERY_FAILURE
Language=Bulgarian
ERROR_RECOVERY_FAILURE - A transaction recovery failed.
.

MessageId=1280
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_FIBER
Language=Bulgarian
ERROR_ALREADY_FIBER - The current thread has already been converted to a fiber.
.

MessageId=1281
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_THREAD
Language=Bulgarian
ERROR_ALREADY_THREAD - The current thread has already been converted from a fiber.
.

MessageId=1282
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_BUFFER_OVERRUN
Language=Bulgarian
ERROR_STACK_BUFFER_OVERRUN - The system detected an overrun of a stack-based buffer in this application. This overrun could potentially allow a malicious user to gain control of this application.
.

MessageId=1283
Severity=Success
Facility=System
SymbolicName=ERROR_PARAMETER_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_PARAMETER_QUOTA_EXCEEDED - Data present in one of the parameters is more than the function can operate on.
.

MessageId=1284
Severity=Success
Facility=System
SymbolicName=ERROR_DEBUGGER_INACTIVE
Language=Bulgarian
ERROR_DEBUGGER_INACTIVE - An attempt to do an operation on a debug object failed because the object is in the process of being deleted.
.

MessageId=1285
Severity=Success
Facility=System
SymbolicName=ERROR_DELAY_LOAD_FAILED
Language=Bulgarian
ERROR_DELAY_LOAD_FAILED - An attempt to delay-load a .dll or get a function address in a delay-loaded .dll failed.
.

MessageId=1286
Severity=Success
Facility=System
SymbolicName=ERROR_VDM_DISALLOWED
Language=Bulgarian
ERROR_VDM_DISALLOWED - %1 is a 16-bit application. You do not have permissions to execute 16-bit applications. Check your permissions with your system administrator.
.

MessageId=1287
Severity=Success
Facility=System
SymbolicName=ERROR_UNIDENTIFIED_ERROR
Language=Bulgarian
ERROR_UNIDENTIFIED_ERROR - Insufficient information exists to identify the cause of failure.
.

MessageId=1288
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BANDWIDTH_PARAMETERS
Language=Bulgarian
ERROR_INVALID_BANDWIDTH_PARAMETERS - An invalid budget or period parameter was specified.
.

MessageId=1289
Severity=Success
Facility=System
SymbolicName=ERROR_AFFINITY_NOT_COMPATIBLE
Language=Bulgarian
ERROR_AFFINITY_NOT_COMPATIBLE - An attempt was made to join a thread to a reserve whose affinity did not intersect the reserve affinity or an attempt was made to associate a process with a reserve whose affinity did not intersect the reserve affinity.
.

MessageId=1290
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_ALREADY_IN_RESERVE
Language=Bulgarian
ERROR_THREAD_ALREADY_IN_RESERVE - An attempt was made to join a thread to a reserve which was already joined to another reserve.
.

MessageId=1291
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_NOT_IN_RESERVE
Language=Bulgarian
ERROR_THREAD_NOT_IN_RESERVE - An attempt was made to disjoin a thread from a reserve, but the thread was not joined to the reserve.
.

MessageId=1292
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_PROCESS_IN_RESERVE
Language=Bulgarian
ERROR_THREAD_PROCESS_IN_RESERVE - An attempt was made to disjoin a thread from a reserve whose process is associated with a reserve.
.

MessageId=1293
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_ALREADY_IN_RESERVE
Language=Bulgarian
ERROR_PROCESS_ALREADY_IN_RESERVE - An attempt was made to associate a process with a reserve that was already associated with a reserve.
.

MessageId=1294
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_NOT_IN_RESERVE
Language=Bulgarian
ERROR_PROCESS_NOT_IN_RESERVE - An attempt was made to disassociate a process from a reserve, but the process did not have an associated reserve.
.

MessageId=1295
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_THREADS_IN_RESERVE
Language=Bulgarian
ERROR_PROCESS_THREADS_IN_RESERVE - An attempt was made to associate a process with a reserve, but the process contained thread joined to a reserve.
.

MessageId=1296
Severity=Success
Facility=System
SymbolicName=ERROR_AFFINITY_NOT_SET_IN_RESERVE
Language=Bulgarian
ERROR_AFFINITY_NOT_SET_IN_RESERVE - An attempt was made to set the affinity of a thread or a process, but the thread or process was joined or associated with a reserve.
.

MessageId=1297
Severity=Success
Facility=System
SymbolicName=ERROR_IMPLEMENTATION_LIMIT
Language=Bulgarian
ERROR_IMPLEMENTATION_LIMIT - An operation attempted to exceed an implementation-defined limit.
.

MessageId=1298
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CACHE_ONLY
Language=Bulgarian
ERROR_DS_CACHE_ONLY - The requested object is for internal DS operations only.
.

MessageId=1300
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ALL_ASSIGNED
Language=Bulgarian
ERROR_NOT_ALL_ASSIGNED - Not all privileges referenced are assigned to the caller.
.

MessageId=1301
Severity=Success
Facility=System
SymbolicName=ERROR_SOME_NOT_MAPPED
Language=Bulgarian
ERROR_SOME_NOT_MAPPED - Some mapping between account names and security IDs was not done.
.

MessageId=1302
Severity=Success
Facility=System
SymbolicName=ERROR_NO_QUOTAS_FOR_ACCOUNT
Language=Bulgarian
ERROR_NO_QUOTAS_FOR_ACCOUNT - No system quota limits are specifically set for this account.
.

MessageId=1303
Severity=Success
Facility=System
SymbolicName=ERROR_LOCAL_USER_SESSION_KEY
Language=Bulgarian
ERROR_LOCAL_USER_SESSION_KEY - No encryption key is available. A well-known encryption key was returned.
.

MessageId=1304
Severity=Success
Facility=System
SymbolicName=ERROR_NULL_LM_PASSWORD
Language=Bulgarian
ERROR_NULL_LM_PASSWORD - The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.
.

MessageId=1305
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_REVISION
Language=Bulgarian
ERROR_UNKNOWN_REVISION - The revision level is unknown.
.

MessageId=1306
Severity=Success
Facility=System
SymbolicName=ERROR_REVISION_MISMATCH
Language=Bulgarian
ERROR_REVISION_MISMATCH - Indicates two revision levels are incompatible.
.

MessageId=1307
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OWNER
Language=Bulgarian
ERROR_INVALID_OWNER - This security ID may not be assigned as the owner of this object.
.

MessageId=1308
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIMARY_GROUP
Language=Bulgarian
ERROR_INVALID_PRIMARY_GROUP - This security ID may not be assigned as the primary group of an object.
.

MessageId=1309
Severity=Success
Facility=System
SymbolicName=ERROR_NO_IMPERSONATION_TOKEN
Language=Bulgarian
ERROR_NO_IMPERSONATION_TOKEN - An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.
.

MessageId=1310
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_DISABLE_MANDATORY
Language=Bulgarian
ERROR_CANT_DISABLE_MANDATORY - The group may not be disabled.
.

MessageId=1311
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOGON_SERVERS
Language=Bulgarian
ERROR_NO_LOGON_SERVERS - There are currently no logon servers available to service the logon request.
.

MessageId=1312
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_LOGON_SESSION
Language=Bulgarian
ERROR_NO_SUCH_LOGON_SESSION - A specified logon session does not exist. It may already have been terminated.
.

MessageId=1313
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PRIVILEGE
Language=Bulgarian
ERROR_NO_SUCH_PRIVILEGE - A specified privilege does not exist.
.

MessageId=1314
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVILEGE_NOT_HELD
Language=Bulgarian
ERROR_PRIVILEGE_NOT_HELD - A required privilege is not held by the client.
.

MessageId=1315
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCOUNT_NAME
Language=Bulgarian
ERROR_INVALID_ACCOUNT_NAME - The name provided is not a properly formed account name.
.

MessageId=1316
Severity=Success
Facility=System
SymbolicName=ERROR_USER_EXISTS
Language=Bulgarian
ERROR_USER_EXISTS - The specified user already exists.
.

MessageId=1317
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_USER
Language=Bulgarian
ERROR_NO_SUCH_USER - The specified user does not exist.
.

MessageId=1318
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_EXISTS
Language=Bulgarian
ERROR_GROUP_EXISTS - The specified group already exists.
.

MessageId=1319
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_GROUP
Language=Bulgarian
ERROR_NO_SUCH_GROUP - The specified group does not exist.
.

MessageId=1320
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_GROUP
Language=Bulgarian
ERROR_MEMBER_IN_GROUP - Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member.
.

MessageId=1321
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_GROUP
Language=Bulgarian
ERROR_MEMBER_NOT_IN_GROUP - The specified user account is not a member of the specified group account.
.

MessageId=1322
Severity=Success
Facility=System
SymbolicName=ERROR_LAST_ADMIN
Language=Bulgarian
ERROR_LAST_ADMIN - The last remaining administration account cannot be disabled or deleted.
.

MessageId=1323
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_PASSWORD
Language=Bulgarian
ERROR_WRONG_PASSWORD - Unable to update the password. The value provided as the current password is incorrect.
.

MessageId=1324
Severity=Success
Facility=System
SymbolicName=ERROR_ILL_FORMED_PASSWORD
Language=Bulgarian
ERROR_ILL_FORMED_PASSWORD - Unable to update the password. The value provided for the new password contains values that are not allowed in passwords.
.

MessageId=1325
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_RESTRICTION
Language=Bulgarian
ERROR_PASSWORD_RESTRICTION - Unable to update the password. The value provided for the new password does not meet the length, complexity, or history requirement of the domain.
.

MessageId=1326
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_FAILURE
Language=Bulgarian
ERROR_LOGON_FAILURE - Logon failure: unknown user name or bad password.
.

MessageId=1327
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_RESTRICTION
Language=Bulgarian
ERROR_ACCOUNT_RESTRICTION - Logon failure: user account restriction. Possible reasons are blank passwords not allowed, logon hour restrictions, or a policy restriction has been enforced.
.

MessageId=1328
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_HOURS
Language=Bulgarian
ERROR_INVALID_LOGON_HOURS - Logon failure: account logon time restriction violation.
.

MessageId=1329
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WORKSTATION
Language=Bulgarian
ERROR_INVALID_WORKSTATION - Logon failure: user not allowed to log on to this computer.
.

MessageId=1330
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_EXPIRED
Language=Bulgarian
ERROR_PASSWORD_EXPIRED - Logon failure: the specified account password has expired.
.

MessageId=1331
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_DISABLED
Language=Bulgarian
ERROR_ACCOUNT_DISABLED - Logon failure: account currently disabled.
.

MessageId=1332
Severity=Success
Facility=System
SymbolicName=ERROR_NONE_MAPPED
Language=Bulgarian
ERROR_NONE_MAPPED - No mapping between account names and security IDs was done.
.

MessageId=1333
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LUIDS_REQUESTED
Language=Bulgarian
ERROR_TOO_MANY_LUIDS_REQUESTED - Too many local user identifiers (LUIDs) were requested at one time.
.

MessageId=1334
Severity=Success
Facility=System
SymbolicName=ERROR_LUIDS_EXHAUSTED
Language=Bulgarian
ERROR_LUIDS_EXHAUSTED - No more local user identifiers (LUIDs) are available.
.

MessageId=1335
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SUB_AUTHORITY
Language=Bulgarian
ERROR_INVALID_SUB_AUTHORITY - The subauthority part of a security ID is invalid for this particular use.
.

MessageId=1336
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACL
Language=Bulgarian
ERROR_INVALID_ACL - The access control list (ACL) structure is invalid.
.

MessageId=1337
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SID
Language=Bulgarian
ERROR_INVALID_SID - The security ID structure is invalid.
.

MessageId=1338
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SECURITY_DESCR
Language=Bulgarian
ERROR_INVALID_SECURITY_DESCR - The security descriptor structure is invalid.
.

MessageId=1340
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_INHERITANCE_ACL
Language=Bulgarian
ERROR_BAD_INHERITANCE_ACL - The inherited access control list (ACL) or access control entry (ACE) could not be built.
.

MessageId=1341
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_DISABLED
Language=Bulgarian
ERROR_SERVER_DISABLED - The server is currently disabled.
.

MessageId=1342
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_NOT_DISABLED
Language=Bulgarian
ERROR_SERVER_NOT_DISABLED - The server is currently enabled.
.

MessageId=1343
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ID_AUTHORITY
Language=Bulgarian
ERROR_INVALID_ID_AUTHORITY - The value provided was an invalid value for an identifier authority.
.

MessageId=1344
Severity=Success
Facility=System
SymbolicName=ERROR_ALLOTTED_SPACE_EXCEEDED
Language=Bulgarian
ERROR_ALLOTTED_SPACE_EXCEEDED - No more memory is available for security information updates.
.

MessageId=1345
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUP_ATTRIBUTES
Language=Bulgarian
ERROR_INVALID_GROUP_ATTRIBUTES - The specified attributes are invalid, or incompatible with the attributes for the group as a whole.
.

MessageId=1346
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_IMPERSONATION_LEVEL
Language=Bulgarian
ERROR_BAD_IMPERSONATION_LEVEL - Either a required impersonation level was not provided, or the provided impersonation level is invalid.
.

MessageId=1347
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_OPEN_ANONYMOUS
Language=Bulgarian
ERROR_CANT_OPEN_ANONYMOUS - Cannot open an anonymous level security token.
.

MessageId=1348
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_VALIDATION_CLASS
Language=Bulgarian
ERROR_BAD_VALIDATION_CLASS - The validation information class requested was invalid.
.

MessageId=1349
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_TOKEN_TYPE
Language=Bulgarian
ERROR_BAD_TOKEN_TYPE - The type of the token is inappropriate for its attempted use.
.

MessageId=1350
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SECURITY_ON_OBJECT
Language=Bulgarian
ERROR_NO_SECURITY_ON_OBJECT - Unable to perform a security operation on an object that has no associated security.
.

MessageId=1351
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ACCESS_DOMAIN_INFO
Language=Bulgarian
ERROR_CANT_ACCESS_DOMAIN_INFO - Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied.
.

MessageId=1352
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVER_STATE
Language=Bulgarian
ERROR_INVALID_SERVER_STATE - The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation.
.

MessageId=1353
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_STATE
Language=Bulgarian
ERROR_INVALID_DOMAIN_STATE - The domain was in the wrong state to perform the security operation.
.

MessageId=1354
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_ROLE
Language=Bulgarian
ERROR_INVALID_DOMAIN_ROLE - This operation is only allowed for the Primary Domain Controller of the domain.
.

MessageId=1355
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_DOMAIN
Language=Bulgarian
ERROR_NO_SUCH_DOMAIN - The specified domain either does not exist or could not be contacted.
.

MessageId=1356
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_EXISTS
Language=Bulgarian
ERROR_DOMAIN_EXISTS - The specified domain already exists.
.

MessageId=1357
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_DOMAIN_LIMIT_EXCEEDED - An attempt was made to exceed the limit on the number of domains per server.
.

MessageId=1358
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_CORRUPTION
Language=Bulgarian
ERROR_INTERNAL_DB_CORRUPTION - Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk.
.

MessageId=1359
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_ERROR
Language=Bulgarian
ERROR_INTERNAL_ERROR - An internal error occurred.
.

MessageId=1360
Severity=Success
Facility=System
SymbolicName=ERROR_GENERIC_NOT_MAPPED
Language=Bulgarian
ERROR_GENERIC_NOT_MAPPED - Generic access types were contained in an access mask which should already be mapped to nongeneric types.
.

MessageId=1361
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DESCRIPTOR_FORMAT
Language=Bulgarian
ERROR_BAD_DESCRIPTOR_FORMAT - A security descriptor is not in the right format (absolute or self-relative).
.

MessageId=1362
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGON_PROCESS
Language=Bulgarian
ERROR_NOT_LOGON_PROCESS - The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.
.

MessageId=1363
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_EXISTS
Language=Bulgarian
ERROR_LOGON_SESSION_EXISTS - Cannot start a new logon session with an ID that is already in use.
.

MessageId=1364
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PACKAGE
Language=Bulgarian
ERROR_NO_SUCH_PACKAGE - A specified authentication package is unknown.
.

MessageId=1365
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LOGON_SESSION_STATE
Language=Bulgarian
ERROR_BAD_LOGON_SESSION_STATE - The logon session is not in a state that is consistent with the requested operation.
.

MessageId=1366
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_COLLISION
Language=Bulgarian
ERROR_LOGON_SESSION_COLLISION - The logon session ID is already in use.
.

MessageId=1367
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_TYPE
Language=Bulgarian
ERROR_INVALID_LOGON_TYPE - A logon request contained an invalid logon type value.
.

MessageId=1368
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_IMPERSONATE
Language=Bulgarian
ERROR_CANNOT_IMPERSONATE - Unable to impersonate using a named pipe until data has been read from that pipe.
.

MessageId=1369
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_INVALID_STATE
Language=Bulgarian
ERROR_RXACT_INVALID_STATE - The transaction state of a registry subtree is incompatible with the requested operation.
.

MessageId=1370
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMIT_FAILURE
Language=Bulgarian
ERROR_RXACT_COMMIT_FAILURE - An internal security database corruption has been encountered.
.

MessageId=1371
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_ACCOUNT
Language=Bulgarian
ERROR_SPECIAL_ACCOUNT - Cannot perform this operation on built-in accounts.
.

MessageId=1372
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_GROUP
Language=Bulgarian
ERROR_SPECIAL_GROUP - Cannot perform this operation on this built-in special group.
.

MessageId=1373
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_USER
Language=Bulgarian
ERROR_SPECIAL_USER - Cannot perform this operation on this built-in special user.
.

MessageId=1374
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBERS_PRIMARY_GROUP
Language=Bulgarian
ERROR_MEMBERS_PRIMARY_GROUP - The user cannot be removed from a group because the group is currently the user's primary group.
.

MessageId=1375
Severity=Success
Facility=System
SymbolicName=ERROR_TOKEN_ALREADY_IN_USE
Language=Bulgarian
ERROR_TOKEN_ALREADY_IN_USE - The token is already in use as a primary token.
.

MessageId=1376
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_ALIAS
Language=Bulgarian
ERROR_NO_SUCH_ALIAS - The specified local group does not exist.
.

MessageId=1377
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_ALIAS
Language=Bulgarian
ERROR_MEMBER_NOT_IN_ALIAS - The specified account name is not a member of the local group.
.

MessageId=1378
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_ALIAS
Language=Bulgarian
ERROR_MEMBER_IN_ALIAS - The specified account name is already a member of the local group.
.

MessageId=1379
Severity=Success
Facility=System
SymbolicName=ERROR_ALIAS_EXISTS
Language=Bulgarian
ERROR_ALIAS_EXISTS - The specified local group already exists.
.

MessageId=1380
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_NOT_GRANTED
Language=Bulgarian
ERROR_LOGON_NOT_GRANTED - Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1381
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SECRETS
Language=Bulgarian
ERROR_TOO_MANY_SECRETS - The maximum number of secrets that may be stored in a single system has been exceeded.
.

MessageId=1382
Severity=Success
Facility=System
SymbolicName=ERROR_SECRET_TOO_LONG
Language=Bulgarian
ERROR_SECRET_TOO_LONG - The length of a secret exceeds the maximum length allowed.
.

MessageId=1383
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_ERROR
Language=Bulgarian
ERROR_INTERNAL_DB_ERROR - The local security authority database contains an internal inconsistency.
.

MessageId=1384
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CONTEXT_IDS
Language=Bulgarian
ERROR_TOO_MANY_CONTEXT_IDS - During a logon attempt, the user's security context accumulated too many security IDs.
.

MessageId=1385
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_TYPE_NOT_GRANTED
Language=Bulgarian
ERROR_LOGON_TYPE_NOT_GRANTED - Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1386
Severity=Success
Facility=System
SymbolicName=ERROR_NT_CROSS_ENCRYPTION_REQUIRED
Language=Bulgarian
ERROR_NT_CROSS_ENCRYPTION_REQUIRED - A cross-encrypted password is necessary to change a user password.
.

MessageId=1387
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_MEMBER
Language=Bulgarian
ERROR_NO_SUCH_MEMBER - A new member could not be added to or removed from the local group because the member does not exist.
.

MessageId=1388
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEMBER
Language=Bulgarian
ERROR_INVALID_MEMBER - A new member could not be added to a local group because the member has the wrong account type.
.

MessageId=1389
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SIDS
Language=Bulgarian
ERROR_TOO_MANY_SIDS - Too many security IDs have been specified.
.

MessageId=1390
Severity=Success
Facility=System
SymbolicName=ERROR_LM_CROSS_ENCRYPTION_REQUIRED
Language=Bulgarian
ERROR_LM_CROSS_ENCRYPTION_REQUIRED - A cross-encrypted password is necessary to change this user password.
.

MessageId=1391
Severity=Success
Facility=System
SymbolicName=ERROR_NO_INHERITANCE
Language=Bulgarian
ERROR_NO_INHERITANCE - Indicates an ACL contains no inheritable components.
.

MessageId=1392
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_CORRUPT
Language=Bulgarian
ERROR_FILE_CORRUPT - The file or directory is corrupted and unreadable.
.

MessageId=1393
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CORRUPT
Language=Bulgarian
ERROR_DISK_CORRUPT - The disk structure is corrupted and unreadable.
.

MessageId=1394
Severity=Success
Facility=System
SymbolicName=ERROR_NO_USER_SESSION_KEY
Language=Bulgarian
ERROR_NO_USER_SESSION_KEY - There is no user session key for the specified logon session.
.

MessageId=1395
Severity=Success
Facility=System
SymbolicName=ERROR_LICENSE_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_LICENSE_QUOTA_EXCEEDED - The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept.
.

MessageId=1396
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_TARGET_NAME
Language=Bulgarian
ERROR_WRONG_TARGET_NAME - Logon Failure: The target account name is incorrect.
.

MessageId=1397
Severity=Success
Facility=System
SymbolicName=ERROR_MUTUAL_AUTH_FAILED
Language=Bulgarian
ERROR_MUTUAL_AUTH_FAILED - Mutual Authentication failed. The server's password is out of date at the domain controller.
.

MessageId=1398
Severity=Success
Facility=System
SymbolicName=ERROR_TIME_SKEW
Language=Bulgarian
ERROR_TIME_SKEW - There is a time and/or date difference between the client and server.
.

MessageId=1399
Severity=Success
Facility=System
SymbolicName=ERROR_CURRENT_DOMAIN_NOT_ALLOWED
Language=Bulgarian
ERROR_CURRENT_DOMAIN_NOT_ALLOWED - This operation cannot be performed on the current domain.
.

MessageId=1400
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_HANDLE
Language=Bulgarian
ERROR_INVALID_WINDOW_HANDLE - Invalid window handle.
.

MessageId=1401
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MENU_HANDLE
Language=Bulgarian
ERROR_INVALID_MENU_HANDLE - Invalid menu handle.
.

MessageId=1402
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CURSOR_HANDLE
Language=Bulgarian
ERROR_INVALID_CURSOR_HANDLE - Invalid cursor handle.
.

MessageId=1403
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCEL_HANDLE
Language=Bulgarian
ERROR_INVALID_ACCEL_HANDLE - Invalid accelerator table handle.
.

MessageId=1404
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_HANDLE
Language=Bulgarian
ERROR_INVALID_HOOK_HANDLE - Invalid hook handle.
.

MessageId=1405
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DWP_HANDLE
Language=Bulgarian
ERROR_INVALID_DWP_HANDLE - Invalid handle to a multiple-window position structure.
.

MessageId=1406
Severity=Success
Facility=System
SymbolicName=ERROR_TLW_WITH_WSCHILD
Language=Bulgarian
ERROR_TLW_WITH_WSCHILD - Cannot create a top-level child window.
.

MessageId=1407
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_FIND_WND_CLASS
Language=Bulgarian
ERROR_CANNOT_FIND_WND_CLASS - Cannot find window class.
.

MessageId=1408
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_OF_OTHER_THREAD
Language=Bulgarian
ERROR_WINDOW_OF_OTHER_THREAD - Invalid window; it belongs to other thread.
.

MessageId=1409
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_ALREADY_REGISTERED
Language=Bulgarian
ERROR_HOTKEY_ALREADY_REGISTERED - Hot key is already registered.
.

MessageId=1410
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_ALREADY_EXISTS
Language=Bulgarian
ERROR_CLASS_ALREADY_EXISTS - Class already exists.
.

MessageId=1411
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_DOES_NOT_EXIST
Language=Bulgarian
ERROR_CLASS_DOES_NOT_EXIST - Class does not exist.
.

MessageId=1412
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_HAS_WINDOWS
Language=Bulgarian
ERROR_CLASS_HAS_WINDOWS - Class still has open windows.
.

MessageId=1413
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_INDEX
Language=Bulgarian
ERROR_INVALID_INDEX - Invalid index.
.

MessageId=1414
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ICON_HANDLE
Language=Bulgarian
ERROR_INVALID_ICON_HANDLE - Invalid icon handle.
.

MessageId=1415
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVATE_DIALOG_INDEX
Language=Bulgarian
ERROR_PRIVATE_DIALOG_INDEX - Using private DIALOG window words.
.

MessageId=1416
Severity=Success
Facility=System
SymbolicName=ERROR_LISTBOX_ID_NOT_FOUND
Language=Bulgarian
ERROR_LISTBOX_ID_NOT_FOUND - The list box identifier was not found.
.

MessageId=1417
Severity=Success
Facility=System
SymbolicName=ERROR_NO_WILDCARD_CHARACTERS
Language=Bulgarian
ERROR_NO_WILDCARD_CHARACTERS - No wildcards were found.
.

MessageId=1418
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPBOARD_NOT_OPEN
Language=Bulgarian
ERROR_CLIPBOARD_NOT_OPEN - Thread does not have a clipboard open.
.

MessageId=1419
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_NOT_REGISTERED
Language=Bulgarian
ERROR_HOTKEY_NOT_REGISTERED - Hot key is not registered.
.

MessageId=1420
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_DIALOG
Language=Bulgarian
ERROR_WINDOW_NOT_DIALOG - The window is not a valid dialog window.
.

MessageId=1421
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROL_ID_NOT_FOUND
Language=Bulgarian
ERROR_CONTROL_ID_NOT_FOUND - Control ID not found.
.

MessageId=1422
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMBOBOX_MESSAGE
Language=Bulgarian
ERROR_INVALID_COMBOBOX_MESSAGE - Invalid message for a combo box because it does not have an edit control.
.

MessageId=1423
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_COMBOBOX
Language=Bulgarian
ERROR_WINDOW_NOT_COMBOBOX - The window is not a combo box.
.

MessageId=1424
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EDIT_HEIGHT
Language=Bulgarian
ERROR_INVALID_EDIT_HEIGHT - Height must be less than 256.
.

MessageId=1425
Severity=Success
Facility=System
SymbolicName=ERROR_DC_NOT_FOUND
Language=Bulgarian
ERROR_DC_NOT_FOUND - Invalid device context (DC) handle.
.

MessageId=1426
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_FILTER
Language=Bulgarian
ERROR_INVALID_HOOK_FILTER - Invalid hook procedure type.
.

MessageId=1427
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FILTER_PROC
Language=Bulgarian
ERROR_INVALID_FILTER_PROC - Invalid hook procedure.
.

MessageId=1428
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NEEDS_HMOD
Language=Bulgarian
ERROR_HOOK_NEEDS_HMOD - Cannot set nonlocal hook without a module handle.
.

MessageId=1429
Severity=Success
Facility=System
SymbolicName=ERROR_GLOBAL_ONLY_HOOK
Language=Bulgarian
ERROR_GLOBAL_ONLY_HOOK - This hook procedure can only be set globally.
.

MessageId=1430
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_HOOK_SET
Language=Bulgarian
ERROR_JOURNAL_HOOK_SET - The journal hook procedure is already installed.
.

MessageId=1431
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NOT_INSTALLED
Language=Bulgarian
ERROR_HOOK_NOT_INSTALLED - The hook procedure is not installed.
.

MessageId=1432
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LB_MESSAGE
Language=Bulgarian
ERROR_INVALID_LB_MESSAGE - Invalid message for single-selection list box.
.

MessageId=1433
Severity=Success
Facility=System
SymbolicName=ERROR_SETCOUNT_ON_BAD_LB
Language=Bulgarian
ERROR_SETCOUNT_ON_BAD_LB - LB_SETCOUNT sent to non-lazy list box.
.

MessageId=1434
Severity=Success
Facility=System
SymbolicName=ERROR_LB_WITHOUT_TABSTOPS
Language=Bulgarian
ERROR_LB_WITHOUT_TABSTOPS - This list box does not support tab stops.
.

MessageId=1435
Severity=Success
Facility=System
SymbolicName=ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
Language=Bulgarian
ERROR_DESTROY_OBJECT_OF_OTHER_THREAD - Cannot destroy object created by another thread.
.

MessageId=1436
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_WINDOW_MENU
Language=Bulgarian
ERROR_CHILD_WINDOW_MENU - Child windows cannot have menus.
.

MessageId=1437
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_MENU
Language=Bulgarian
ERROR_NO_SYSTEM_MENU - The window does not have a system menu.
.

MessageId=1438
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MSGBOX_STYLE
Language=Bulgarian
ERROR_INVALID_MSGBOX_STYLE - Invalid message box style.
.

MessageId=1439
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SPI_VALUE
Language=Bulgarian
ERROR_INVALID_SPI_VALUE - Invalid system-wide (SPI_*) parameter.
.

MessageId=1440
Severity=Success
Facility=System
SymbolicName=ERROR_SCREEN_ALREADY_LOCKED
Language=Bulgarian
ERROR_SCREEN_ALREADY_LOCKED - Screen already locked.
.

MessageId=1441
Severity=Success
Facility=System
SymbolicName=ERROR_HWNDS_HAVE_DIFF_PARENT
Language=Bulgarian
ERROR_HWNDS_HAVE_DIFF_PARENT - All handles to windows in a multiple-window position structure must have the same parent.
.

MessageId=1442
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CHILD_WINDOW
Language=Bulgarian
ERROR_NOT_CHILD_WINDOW - The window is not a child window.
.

MessageId=1443
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GW_COMMAND
Language=Bulgarian
ERROR_INVALID_GW_COMMAND - Invalid GW_* command.
.

MessageId=1444
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_THREAD_ID
Language=Bulgarian
ERROR_INVALID_THREAD_ID - Invalid thread identifier.
.

MessageId=1445
Severity=Success
Facility=System
SymbolicName=ERROR_NON_MDICHILD_WINDOW
Language=Bulgarian
ERROR_NON_MDICHILD_WINDOW - Cannot process a message from a window that is not a multiple document interface (MDI) window.
.

MessageId=1446
Severity=Success
Facility=System
SymbolicName=ERROR_POPUP_ALREADY_ACTIVE
Language=Bulgarian
ERROR_POPUP_ALREADY_ACTIVE - Popup menu already active.
.

MessageId=1447
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SCROLLBARS
Language=Bulgarian
ERROR_NO_SCROLLBARS - The window does not have scroll bars.
.

MessageId=1448
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SCROLLBAR_RANGE
Language=Bulgarian
ERROR_INVALID_SCROLLBAR_RANGE - Scroll bar range cannot be greater than MAXLONG.
.

MessageId=1449
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHOWWIN_COMMAND
Language=Bulgarian
ERROR_INVALID_SHOWWIN_COMMAND - Cannot show or remove the window in the way specified.
.

MessageId=1450
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_RESOURCES
Language=Bulgarian
ERROR_NO_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service.
.

MessageId=1451
Severity=Success
Facility=System
SymbolicName=ERROR_NONPAGED_SYSTEM_RESOURCES
Language=Bulgarian
ERROR_NONPAGED_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service.
.

MessageId=1452
Severity=Success
Facility=System
SymbolicName=ERROR_PAGED_SYSTEM_RESOURCES
Language=Bulgarian
ERROR_PAGED_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service.
.

MessageId=1453
Severity=Success
Facility=System
SymbolicName=ERROR_WORKING_SET_QUOTA
Language=Bulgarian
ERROR_WORKING_SET_QUOTA - Insufficient quota to complete the requested service.
.

MessageId=1454
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_QUOTA
Language=Bulgarian
ERROR_PAGEFILE_QUOTA - Insufficient quota to complete the requested service.
.

MessageId=1455
Severity=Success
Facility=System
SymbolicName=ERROR_COMMITMENT_LIMIT
Language=Bulgarian
ERROR_COMMITMENT_LIMIT - The paging file is too small for this operation to complete.
.

MessageId=1456
Severity=Success
Facility=System
SymbolicName=ERROR_MENU_ITEM_NOT_FOUND
Language=Bulgarian
ERROR_MENU_ITEM_NOT_FOUND - A menu item was not found.
.

MessageId=1457
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_KEYBOARD_HANDLE
Language=Bulgarian
ERROR_INVALID_KEYBOARD_HANDLE - Invalid keyboard layout handle.
.

MessageId=1458
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_TYPE_NOT_ALLOWED
Language=Bulgarian
ERROR_HOOK_TYPE_NOT_ALLOWED - Hook type not allowed.
.

MessageId=1459
Severity=Success
Facility=System
SymbolicName=ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION
Language=Bulgarian
ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION - This operation requires an interactive window station.
.

MessageId=1460
Severity=Success
Facility=System
SymbolicName=ERROR_TIMEOUT
Language=Bulgarian
ERROR_TIMEOUT - This operation returned because the timeout period expired.
.

MessageId=1461
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MONITOR_HANDLE
Language=Bulgarian
ERROR_INVALID_MONITOR_HANDLE - Invalid monitor handle.
.

MessageId=1500
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CORRUPT
Language=Bulgarian
ERROR_EVENTLOG_FILE_CORRUPT - The event log file is corrupted.
.

MessageId=1501
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_CANT_START
Language=Bulgarian
ERROR_EVENTLOG_CANT_START - No event log file could be opened, so the event logging service did not start.
.

MessageId=1502
Severity=Success
Facility=System
SymbolicName=ERROR_LOG_FILE_FULL
Language=Bulgarian
ERROR_LOG_FILE_FULL - The event log file is full.
.

MessageId=1503
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CHANGED
Language=Bulgarian
ERROR_EVENTLOG_FILE_CHANGED - The event log file has changed between read operations.
.

MessageId=1601
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SERVICE_FAILURE
Language=Bulgarian
ERROR_INSTALL_SERVICE_FAILURE - The Windows Installer service could not be accessed. This can occur if you are running Windows in safe mode, or if the Windows Installer is not correctly installed. Contact your support personnel for assistance.
.

MessageId=1602
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_USEREXIT
Language=Bulgarian
ERROR_INSTALL_USEREXIT - User cancelled installation.
.

MessageId=1603
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_FAILURE
Language=Bulgarian
ERROR_INSTALL_FAILURE - Fatal error during installation.
.

MessageId=1604
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SUSPEND
Language=Bulgarian
ERROR_INSTALL_SUSPEND - Installation suspended, incomplete.
.

MessageId=1605
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRODUCT
Language=Bulgarian
ERROR_UNKNOWN_PRODUCT - This action is only valid for products that are currently installed.
.

MessageId=1606
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_FEATURE
Language=Bulgarian
ERROR_UNKNOWN_FEATURE - Feature ID not registered.
.

MessageId=1607
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_COMPONENT
Language=Bulgarian
ERROR_UNKNOWN_COMPONENT - Component ID not registered.
.

MessageId=1608
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PROPERTY
Language=Bulgarian
ERROR_UNKNOWN_PROPERTY - Unknown property.
.

MessageId=1609
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HANDLE_STATE
Language=Bulgarian
ERROR_INVALID_HANDLE_STATE - Handle is in an invalid state.
.

MessageId=1610
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_CONFIGURATION
Language=Bulgarian
ERROR_BAD_CONFIGURATION - The configuration data for this product is corrupt. Contact your support personnel.
.

MessageId=1611
Severity=Success
Facility=System
SymbolicName=ERROR_INDEX_ABSENT
Language=Bulgarian
ERROR_INDEX_ABSENT - Component qualifier not present.
.

MessageId=1612
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SOURCE_ABSENT
Language=Bulgarian
ERROR_INSTALL_SOURCE_ABSENT - The installation source for this product is not available. Verify that the source exists and that you can access it.
.

MessageId=1613
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_VERSION
Language=Bulgarian
ERROR_INSTALL_PACKAGE_VERSION - This installation package cannot be installed by the Windows Installer service. You must install a Windows service pack that contains a newer version of the Windows Installer service.
.

MessageId=1614
Severity=Success
Facility=System
SymbolicName=ERROR_PRODUCT_UNINSTALLED
Language=Bulgarian
ERROR_PRODUCT_UNINSTALLED - Product is uninstalled.
.

MessageId=1615
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_QUERY_SYNTAX
Language=Bulgarian
ERROR_BAD_QUERY_SYNTAX - SQL query syntax invalid or unsupported.
.

MessageId=1616
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FIELD
Language=Bulgarian
ERROR_INVALID_FIELD - Record field does not exist.
.

MessageId=1617
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REMOVED
Language=Bulgarian
ERROR_DEVICE_REMOVED - The device has been removed.
.

MessageId=1618
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_ALREADY_RUNNING
Language=Bulgarian
ERROR_INSTALL_ALREADY_RUNNING - Another installation is already in progress. Complete that installation before proceeding with this install.
.

MessageId=1619
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_OPEN_FAILED
Language=Bulgarian
ERROR_INSTALL_PACKAGE_OPEN_FAILED - This installation package could not be opened. Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1620
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_INVALID
Language=Bulgarian
ERROR_INSTALL_PACKAGE_INVALID - This installation package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1621
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_UI_FAILURE
Language=Bulgarian
ERROR_INSTALL_UI_FAILURE - There was an error starting the Windows Installer service user interface. Contact your support personnel.
.

MessageId=1622
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_LOG_FAILURE
Language=Bulgarian
ERROR_INSTALL_LOG_FAILURE - Error opening installation log file. Verify that the specified log file location exists and that you can write to it.
.

MessageId=1623
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_LANGUAGE_UNSUPPORTED
Language=Bulgarian
ERROR_INSTALL_LANGUAGE_UNSUPPORTED - The language of this installation package is not supported by your system.
.

MessageId=1624
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TRANSFORM_FAILURE
Language=Bulgarian
ERROR_INSTALL_TRANSFORM_FAILURE - Error applying transforms. Verify that the specified transform paths are valid.
.

MessageId=1625
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_REJECTED
Language=Bulgarian
ERROR_INSTALL_PACKAGE_REJECTED - This installation is forbidden by system policy. Contact your system administrator.
.

MessageId=1626
Severity=Success
Facility=System
SymbolicName=ERROR_FUNCTION_NOT_CALLED
Language=Bulgarian
ERROR_FUNCTION_NOT_CALLED - Function could not be executed.
.

MessageId=1627
Severity=Success
Facility=System
SymbolicName=ERROR_FUNCTION_FAILED
Language=Bulgarian
ERROR_FUNCTION_FAILED - Function failed during execution.
.

MessageId=1628
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TABLE
Language=Bulgarian
ERROR_INVALID_TABLE - Invalid or unknown table specified.
.

MessageId=1629
Severity=Success
Facility=System
SymbolicName=ERROR_DATATYPE_MISMATCH
Language=Bulgarian
ERROR_DATATYPE_MISMATCH - Data supplied is of wrong type.
.

MessageId=1630
Severity=Success
Facility=System
SymbolicName=ERROR_UNSUPPORTED_TYPE
Language=Bulgarian
ERROR_UNSUPPORTED_TYPE - Data of this type is not supported.
.

MessageId=1631
Severity=Success
Facility=System
SymbolicName=ERROR_CREATE_FAILED
Language=Bulgarian
ERROR_CREATE_FAILED - The Windows Installer service failed to start. Contact your support personnel.
.

MessageId=1632
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TEMP_UNWRITABLE
Language=Bulgarian
ERROR_INSTALL_TEMP_UNWRITABLE - The Temp folder is on a drive that is full or inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
.

MessageId=1633
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PLATFORM_UNSUPPORTED
Language=Bulgarian
ERROR_INSTALL_PLATFORM_UNSUPPORTED - This installation package is not supported by this processor type. Contact your product vendor.
.

MessageId=1634
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_NOTUSED
Language=Bulgarian
ERROR_INSTALL_NOTUSED - Component not used on this computer.
.

MessageId=1635
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_OPEN_FAILED
Language=Bulgarian
ERROR_PATCH_PACKAGE_OPEN_FAILED - This patch package could not be opened. Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1636
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_INVALID
Language=Bulgarian
ERROR_PATCH_PACKAGE_INVALID - This patch package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1637
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_UNSUPPORTED
Language=Bulgarian
ERROR_PATCH_PACKAGE_UNSUPPORTED - This patch package cannot be processed by the Windows Installer service. You must install a Windows service pack that contains a newer version of the Windows Installer service.
.

MessageId=1638
Severity=Success
Facility=System
SymbolicName=ERROR_PRODUCT_VERSION
Language=Bulgarian
ERROR_PRODUCT_VERSION - Another version of this product is already installed. Installation of this version cannot continue. To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
.

MessageId=1639
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMMAND_LINE
Language=Bulgarian
ERROR_INVALID_COMMAND_LINE - Invalid command line argument. Consult the Windows Installer SDK for detailed command line help.
.

MessageId=1640
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_REMOTE_DISALLOWED
Language=Bulgarian
ERROR_INSTALL_REMOTE_DISALLOWED - Only administrators have permission to add, remove, or configure server software during a Terminal Services remote session. If you want to install or configure software on the server, contact your network administrator.
.

MessageId=1641
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_REBOOT_INITIATED
Language=Bulgarian
ERROR_SUCCESS_REBOOT_INITIATED - The requested operation completed successfully. The system will be restarted so the changes can take effect.
.

MessageId=1642
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_TARGET_NOT_FOUND
Language=Bulgarian
ERROR_PATCH_TARGET_NOT_FOUND - The upgrade patch cannot be installed by the Windows Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch.
.

MessageId=1643
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_REJECTED
Language=Bulgarian
ERROR_PATCH_PACKAGE_REJECTED - The patch package is not permitted by software restriction policy.
.

MessageId=1644
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TRANSFORM_REJECTED
Language=Bulgarian
ERROR_INSTALL_TRANSFORM_REJECTED - One or more customizations are not permitted by software restriction policy.
.

MessageId=1645
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_REMOTE_PROHIBITED
Language=Bulgarian
ERROR_INSTALL_REMOTE_PROHIBITED - The Windows Installer does not permit installation from a Remote Desktop Connection.
.

MessageId=1700
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_BINDING
Language=Bulgarian
RPC_S_INVALID_STRING_BINDING - The string binding is invalid.
.

MessageId=1701
Severity=Success
Facility=System
SymbolicName=RPC_S_WRONG_KIND_OF_BINDING
Language=Bulgarian
RPC_S_WRONG_KIND_OF_BINDING - The binding handle is not the correct type.
.

MessageId=1702
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BINDING
Language=Bulgarian
RPC_S_INVALID_BINDING - The binding handle is invalid.
.

MessageId=1703
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_SUPPORTED
Language=Bulgarian
RPC_S_PROTSEQ_NOT_SUPPORTED - The RPC protocol sequence is not supported.
.

MessageId=1704
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_RPC_PROTSEQ
Language=Bulgarian
RPC_S_INVALID_RPC_PROTSEQ - The RPC protocol sequence is invalid.
.

MessageId=1705
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_UUID
Language=Bulgarian
RPC_S_INVALID_STRING_UUID - The string universal unique identifier (UUID) is invalid.
.

MessageId=1706
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ENDPOINT_FORMAT
Language=Bulgarian
RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint format is invalid.
.

MessageId=1707
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NET_ADDR
Language=Bulgarian
RPC_S_INVALID_NET_ADDR - The network address is invalid.
.

MessageId=1708
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENDPOINT_FOUND
Language=Bulgarian
RPC_S_NO_ENDPOINT_FOUND - No endpoint was found.
.

MessageId=1709
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TIMEOUT
Language=Bulgarian
RPC_S_INVALID_TIMEOUT - The timeout value is invalid.
.

MessageId=1710
Severity=Success
Facility=System
SymbolicName=RPC_S_OBJECT_NOT_FOUND
Language=Bulgarian
RPC_S_OBJECT_NOT_FOUND - The object universal unique identifier (UUID) was not found.
.

MessageId=1711
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_REGISTERED
Language=Bulgarian
RPC_S_ALREADY_REGISTERED - The object universal unique identifier (UUID) has already been registered.
.

MessageId=1712
Severity=Success
Facility=System
SymbolicName=RPC_S_TYPE_ALREADY_REGISTERED
Language=Bulgarian
RPC_S_TYPE_ALREADY_REGISTERED - The type universal unique identifier (UUID) has already been registered.
.

MessageId=1713
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_LISTENING
Language=Bulgarian
RPC_S_ALREADY_LISTENING - The RPC server is already listening.
.

MessageId=1714
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS_REGISTERED
Language=Bulgarian
RPC_S_NO_PROTSEQS_REGISTERED - No protocol sequences have been registered.
.

MessageId=1715
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_LISTENING
Language=Bulgarian
RPC_S_NOT_LISTENING - The RPC server is not listening.
.

MessageId=1716
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_MGR_TYPE
Language=Bulgarian
RPC_S_UNKNOWN_MGR_TYPE - The manager type is unknown.
.

MessageId=1717
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_IF
Language=Bulgarian
RPC_S_UNKNOWN_IF - The interface is unknown.
.

MessageId=1718
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_BINDINGS
Language=Bulgarian
RPC_S_NO_BINDINGS - There are no bindings.
.

MessageId=1719
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS
Language=Bulgarian
RPC_S_NO_PROTSEQS - There are no protocol sequences.
.

MessageId=1720
Severity=Success
Facility=System
SymbolicName=RPC_S_CANT_CREATE_ENDPOINT
Language=Bulgarian
RPC_S_CANT_CREATE_ENDPOINT - The endpoint cannot be created.
.

MessageId=1721
Severity=Success
Facility=System
SymbolicName=RPC_S_OUT_OF_RESOURCES
Language=Bulgarian
RPC_S_OUT_OF_RESOURCES - Not enough resources are available to complete this operation.
.

MessageId=1722
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_UNAVAILABLE
Language=Bulgarian
RPC_S_SERVER_UNAVAILABLE - The RPC server is unavailable.
.

MessageId=1723
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_TOO_BUSY
Language=Bulgarian
RPC_S_SERVER_TOO_BUSY - The RPC server is too busy to complete this operation.
.

MessageId=1724
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NETWORK_OPTIONS
Language=Bulgarian
RPC_S_INVALID_NETWORK_OPTIONS - The network options are invalid.
.

MessageId=1725
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CALL_ACTIVE
Language=Bulgarian
RPC_S_NO_CALL_ACTIVE - There are no remote procedure calls active on this thread.
.

MessageId=1726
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED
Language=Bulgarian
RPC_S_CALL_FAILED - The remote procedure call failed.
.

MessageId=1727
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED_DNE
Language=Bulgarian
RPC_S_CALL_FAILED_DNE - The remote procedure call failed and did not execute.
.

MessageId=1728
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTOCOL_ERROR
Language=Bulgarian
RPC_S_PROTOCOL_ERROR - A remote procedure call (RPC) protocol error occurred.
.

MessageId=1730
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TRANS_SYN
Language=Bulgarian
RPC_S_UNSUPPORTED_TRANS_SYN - The transfer syntax is not supported by the RPC server.
.

MessageId=1732
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TYPE
Language=Bulgarian
RPC_S_UNSUPPORTED_TYPE - The universal unique identifier (UUID) type is not supported.
.

MessageId=1733
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TAG
Language=Bulgarian
RPC_S_INVALID_TAG - The tag is invalid.
.

MessageId=1734
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BOUND
Language=Bulgarian
RPC_S_INVALID_BOUND - The array bounds are invalid.
.

MessageId=1735
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENTRY_NAME
Language=Bulgarian
RPC_S_NO_ENTRY_NAME - The binding does not contain an entry name.
.

MessageId=1736
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAME_SYNTAX
Language=Bulgarian
RPC_S_INVALID_NAME_SYNTAX - The name syntax is invalid.
.

MessageId=1737
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_NAME_SYNTAX
Language=Bulgarian
RPC_S_UNSUPPORTED_NAME_SYNTAX - The name syntax is not supported.
.

MessageId=1739
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_NO_ADDRESS
Language=Bulgarian
RPC_S_UUID_NO_ADDRESS - No network address is available to use to construct a universal unique identifier (UUID).
.

MessageId=1740
Severity=Success
Facility=System
SymbolicName=RPC_S_DUPLICATE_ENDPOINT
Language=Bulgarian
RPC_S_DUPLICATE_ENDPOINT - The endpoint is a duplicate.
.

MessageId=1741
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_TYPE
Language=Bulgarian
RPC_S_UNKNOWN_AUTHN_TYPE - The authentication type is unknown.
.

MessageId=1742
Severity=Success
Facility=System
SymbolicName=RPC_S_MAX_CALLS_TOO_SMALL
Language=Bulgarian
RPC_S_MAX_CALLS_TOO_SMALL - The maximum number of calls is too small.
.

MessageId=1743
Severity=Success
Facility=System
SymbolicName=RPC_S_STRING_TOO_LONG
Language=Bulgarian
RPC_S_STRING_TOO_LONG - The string is too long.
.

MessageId=1744
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_FOUND
Language=Bulgarian
RPC_S_PROTSEQ_NOT_FOUND - The RPC protocol sequence was not found.
.

MessageId=1745
Severity=Success
Facility=System
SymbolicName=RPC_S_PROCNUM_OUT_OF_RANGE
Language=Bulgarian
RPC_S_PROCNUM_OUT_OF_RANGE - The procedure number is out of range.
.

MessageId=1746
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_HAS_NO_AUTH
Language=Bulgarian
RPC_S_BINDING_HAS_NO_AUTH - The binding does not contain any authentication information.
.

MessageId=1747
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_SERVICE
Language=Bulgarian
RPC_S_UNKNOWN_AUTHN_SERVICE - The authentication service is unknown.
.

MessageId=1748
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_LEVEL
Language=Bulgarian
RPC_S_UNKNOWN_AUTHN_LEVEL - The authentication level is unknown.
.

MessageId=1749
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_AUTH_IDENTITY
Language=Bulgarian
RPC_S_INVALID_AUTH_IDENTITY - The security context is invalid.
.

MessageId=1750
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHZ_SERVICE
Language=Bulgarian
RPC_S_UNKNOWN_AUTHZ_SERVICE - The authorization service is unknown.
.

MessageId=1751
Severity=Success
Facility=System
SymbolicName=EPT_S_INVALID_ENTRY
Language=Bulgarian
EPT_S_INVALID_ENTRY - The entry is invalid.
.

MessageId=1752
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_PERFORM_OP
Language=Bulgarian
EPT_S_CANT_PERFORM_OP - The server endpoint cannot perform the operation.
.

MessageId=1753
Severity=Success
Facility=System
SymbolicName=EPT_S_NOT_REGISTERED
Language=Bulgarian
EPT_S_NOT_REGISTERED - There are no more endpoints available from the endpoint mapper.
.

MessageId=1754
Severity=Success
Facility=System
SymbolicName=RPC_S_NOTHING_TO_EXPORT
Language=Bulgarian
RPC_S_NOTHING_TO_EXPORT - No interfaces have been exported.
.

MessageId=1755
Severity=Success
Facility=System
SymbolicName=RPC_S_INCOMPLETE_NAME
Language=Bulgarian
RPC_S_INCOMPLETE_NAME - The entry name is incomplete.
.

MessageId=1756
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_VERS_OPTION
Language=Bulgarian
RPC_S_INVALID_VERS_OPTION - The version option is invalid.
.

MessageId=1757
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_MEMBERS
Language=Bulgarian
RPC_S_NO_MORE_MEMBERS - There are no more members.
.

MessageId=1758
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_ALL_OBJS_UNEXPORTED
Language=Bulgarian
RPC_S_NOT_ALL_OBJS_UNEXPORTED - There is nothing to unexport.
.

MessageId=1759
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERFACE_NOT_FOUND
Language=Bulgarian
RPC_S_INTERFACE_NOT_FOUND - The interface was not found.
.

MessageId=1760
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_ALREADY_EXISTS
Language=Bulgarian
RPC_S_ENTRY_ALREADY_EXISTS - The entry already exists.
.

MessageId=1761
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_NOT_FOUND
Language=Bulgarian
RPC_S_ENTRY_NOT_FOUND - The entry is not found.
.

MessageId=1762
Severity=Success
Facility=System
SymbolicName=RPC_S_NAME_SERVICE_UNAVAILABLE
Language=Bulgarian
RPC_S_NAME_SERVICE_UNAVAILABLE - The name service is unavailable.
.

MessageId=1763
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAF_ID
Language=Bulgarian
RPC_S_INVALID_NAF_ID - The network address family is invalid.
.

MessageId=1764
Severity=Success
Facility=System
SymbolicName=RPC_S_CANNOT_SUPPORT
Language=Bulgarian
RPC_S_CANNOT_SUPPORT - The requested operation is not supported.
.

MessageId=1765
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CONTEXT_AVAILABLE
Language=Bulgarian
RPC_S_NO_CONTEXT_AVAILABLE - No security context is available to allow impersonation.
.

MessageId=1766
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERNAL_ERROR
Language=Bulgarian
RPC_S_INTERNAL_ERROR - An internal error occurred in a remote procedure call (RPC).
.

MessageId=1767
Severity=Success
Facility=System
SymbolicName=RPC_S_ZERO_DIVIDE
Language=Bulgarian
RPC_S_ZERO_DIVIDE - The RPC server attempted an integer division by zero.
.

MessageId=1768
Severity=Success
Facility=System
SymbolicName=RPC_S_ADDRESS_ERROR
Language=Bulgarian
RPC_S_ADDRESS_ERROR - An addressing error occurred in the RPC server.
.

MessageId=1769
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_DIV_ZERO
Language=Bulgarian
RPC_S_FP_DIV_ZERO - A floating-point operation at the RPC server caused a division by zero.
.

MessageId=1770
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_UNDERFLOW
Language=Bulgarian
RPC_S_FP_UNDERFLOW - A floating-point underflow occurred at the RPC server.
.

MessageId=1771
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_OVERFLOW
Language=Bulgarian
RPC_S_FP_OVERFLOW - A floating-point overflow occurred at the RPC server.
.

MessageId=1772
Severity=Success
Facility=System
SymbolicName=RPC_X_NO_MORE_ENTRIES
Language=Bulgarian
RPC_X_NO_MORE_ENTRIES - The list of RPC servers available for the binding of auto handles has been exhausted.
.

MessageId=1773
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_OPEN_FAIL
Language=Bulgarian
RPC_X_SS_CHAR_TRANS_OPEN_FAIL - Unable to open the character translation table file.
.

MessageId=1774
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_SHORT_FILE
Language=Bulgarian
RPC_X_SS_CHAR_TRANS_SHORT_FILE - The file containing the character translation table has fewer than 512 bytes.
.

MessageId=1775
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_IN_NULL_CONTEXT
Language=Bulgarian
RPC_X_SS_IN_NULL_CONTEXT - A null context handle was passed from the client to the host during a remote procedure call.
.

MessageId=1777
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CONTEXT_DAMAGED
Language=Bulgarian
RPC_X_SS_CONTEXT_DAMAGED - The context handle changed during a remote procedure call.
.

MessageId=1778
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_HANDLES_MISMATCH
Language=Bulgarian
RPC_X_SS_HANDLES_MISMATCH - The binding handles passed to a remote procedure call do not match.
.

MessageId=1779
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CANNOT_GET_CALL_HANDLE
Language=Bulgarian
RPC_X_SS_CANNOT_GET_CALL_HANDLE - The stub is unable to get the remote procedure call handle.
.

MessageId=1780
Severity=Success
Facility=System
SymbolicName=RPC_X_NULL_REF_POINTER
Language=Bulgarian
RPC_X_NULL_REF_POINTER - A null reference pointer was passed to the stub.
.

MessageId=1781
Severity=Success
Facility=System
SymbolicName=RPC_X_ENUM_VALUE_OUT_OF_RANGE
Language=Bulgarian
RPC_X_ENUM_VALUE_OUT_OF_RANGE - The enumeration value is out of range.
.

MessageId=1782
Severity=Success
Facility=System
SymbolicName=RPC_X_BYTE_COUNT_TOO_SMALL
Language=Bulgarian
RPC_X_BYTE_COUNT_TOO_SMALL - The byte count is too small.
.

MessageId=1783
Severity=Success
Facility=System
SymbolicName=RPC_X_BAD_STUB_DATA
Language=Bulgarian
RPC_X_BAD_STUB_DATA - The stub received bad data.
.

MessageId=1784
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_USER_BUFFER
Language=Bulgarian
ERROR_INVALID_USER_BUFFER - The supplied user buffer is not valid for the requested operation.
.

MessageId=1785
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_MEDIA
Language=Bulgarian
ERROR_UNRECOGNIZED_MEDIA - The disk media is not recognized. It may not be formatted.
.

MessageId=1786
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_LSA_SECRET
Language=Bulgarian
ERROR_NO_TRUST_LSA_SECRET - The workstation does not have a trust secret.
.

MessageId=1787
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_SAM_ACCOUNT
Language=Bulgarian
ERROR_NO_TRUST_SAM_ACCOUNT - The security database on the server does not have a computer account for this workstation trust relationship.
.

MessageId=1788
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_DOMAIN_FAILURE
Language=Bulgarian
ERROR_TRUSTED_DOMAIN_FAILURE - The trust relationship between the primary domain and the trusted domain failed.
.

MessageId=1789
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_RELATIONSHIP_FAILURE
Language=Bulgarian
ERROR_TRUSTED_RELATIONSHIP_FAILURE - The trust relationship between this workstation and the primary domain failed.
.

MessageId=1790
Severity=Success
Facility=System
SymbolicName=ERROR_TRUST_FAILURE
Language=Bulgarian
ERROR_TRUST_FAILURE - The network logon failed.
.

MessageId=1791
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_IN_PROGRESS
Language=Bulgarian
RPC_S_CALL_IN_PROGRESS - A remote procedure call is already in progress for this thread.
.

MessageId=1792
Severity=Success
Facility=System
SymbolicName=ERROR_NETLOGON_NOT_STARTED
Language=Bulgarian
ERROR_NETLOGON_NOT_STARTED - An attempt was made to logon, but the network logon service was not started.
.

MessageId=1793
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_EXPIRED
Language=Bulgarian
ERROR_ACCOUNT_EXPIRED - The user's account has expired.
.

MessageId=1794
Severity=Success
Facility=System
SymbolicName=ERROR_REDIRECTOR_HAS_OPEN_HANDLES
Language=Bulgarian
ERROR_REDIRECTOR_HAS_OPEN_HANDLES - The redirector is in use and cannot be unloaded.
.

MessageId=1795
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
Language=Bulgarian
ERROR_PRINTER_DRIVER_ALREADY_INSTALLED - The specified printer driver is already installed.
.

MessageId=1796
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PORT
Language=Bulgarian
ERROR_UNKNOWN_PORT - The specified port is unknown.
.

MessageId=1797
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTER_DRIVER
Language=Bulgarian
ERROR_UNKNOWN_PRINTER_DRIVER - The printer driver is unknown.
.

MessageId=1798
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTPROCESSOR
Language=Bulgarian
ERROR_UNKNOWN_PRINTPROCESSOR - The print processor is unknown.
.

MessageId=1799
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEPARATOR_FILE
Language=Bulgarian
ERROR_INVALID_SEPARATOR_FILE - The specified separator file is invalid.
.

MessageId=1800
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIORITY
Language=Bulgarian
ERROR_INVALID_PRIORITY - The specified priority is invalid.
.

MessageId=1801
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_NAME
Language=Bulgarian
ERROR_INVALID_PRINTER_NAME - The printer name is invalid.
.

MessageId=1802
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_ALREADY_EXISTS
Language=Bulgarian
ERROR_PRINTER_ALREADY_EXISTS - The printer already exists.
.

MessageId=1803
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_COMMAND
Language=Bulgarian
ERROR_INVALID_PRINTER_COMMAND - The printer command is invalid.
.

MessageId=1804
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATATYPE
Language=Bulgarian
ERROR_INVALID_DATATYPE - The specified datatype is invalid.
.

MessageId=1805
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ENVIRONMENT
Language=Bulgarian
ERROR_INVALID_ENVIRONMENT - The environment specified is invalid.
.

MessageId=1806
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_BINDINGS
Language=Bulgarian
RPC_S_NO_MORE_BINDINGS - There are no more bindings.
.

MessageId=1807
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
Language=Bulgarian
ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT - The account used is an interdomain trust account. Use your global user account or local user account to access this server.
.

MessageId=1808
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
Language=Bulgarian
ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT - The account used is a computer account. Use your global user account or local user account to access this server.
.

MessageId=1809
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
Language=Bulgarian
ERROR_NOLOGON_SERVER_TRUST_ACCOUNT - The account used is a server trust account. Use your global user account or local user account to access this server.
.

MessageId=1810
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_TRUST_INCONSISTENT
Language=Bulgarian
ERROR_DOMAIN_TRUST_INCONSISTENT - The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.
.

MessageId=1811
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_HAS_OPEN_HANDLES
Language=Bulgarian
ERROR_SERVER_HAS_OPEN_HANDLES - The server is in use and cannot be unloaded.
.

MessageId=1812
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_DATA_NOT_FOUND
Language=Bulgarian
ERROR_RESOURCE_DATA_NOT_FOUND - The specified image file did not contain a resource section.
.

MessageId=1813
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_TYPE_NOT_FOUND
Language=Bulgarian
ERROR_RESOURCE_TYPE_NOT_FOUND - The specified resource type cannot be found in the image file.
.

MessageId=1814
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NAME_NOT_FOUND
Language=Bulgarian
ERROR_RESOURCE_NAME_NOT_FOUND - The specified resource name cannot be found in the image file.
.

MessageId=1815
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_LANG_NOT_FOUND
Language=Bulgarian
ERROR_RESOURCE_LANG_NOT_FOUND - The specified resource language ID cannot be found in the image file.
.

MessageId=1816
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_QUOTA
Language=Bulgarian
ERROR_NOT_ENOUGH_QUOTA - Not enough quota is available to process this command.
.

MessageId=1817
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_INTERFACES
Language=Bulgarian
RPC_S_NO_INTERFACES - No interfaces have been registered.
.

MessageId=1818
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_CANCELLED
Language=Bulgarian
RPC_S_CALL_CANCELLED - The remote procedure call was cancelled.
.

MessageId=1819
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_INCOMPLETE
Language=Bulgarian
RPC_S_BINDING_INCOMPLETE - The binding handle does not contain all required information.
.

MessageId=1820
Severity=Success
Facility=System
SymbolicName=RPC_S_COMM_FAILURE
Language=Bulgarian
RPC_S_COMM_FAILURE - A communications failure occurred during a remote procedure call.
.

MessageId=1821
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_AUTHN_LEVEL
Language=Bulgarian
RPC_S_UNSUPPORTED_AUTHN_LEVEL - The requested authentication level is not supported.
.

MessageId=1822
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PRINC_NAME
Language=Bulgarian
RPC_S_NO_PRINC_NAME - No principal name registered.
.

MessageId=1823
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_RPC_ERROR
Language=Bulgarian
RPC_S_NOT_RPC_ERROR - The error specified is not a valid Windows RPC error code.
.

MessageId=1824
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_LOCAL_ONLY
Language=Bulgarian
RPC_S_UUID_LOCAL_ONLY - A UUID that is valid only on this computer has been allocated.
.

MessageId=1825
Severity=Success
Facility=System
SymbolicName=RPC_S_SEC_PKG_ERROR
Language=Bulgarian
RPC_S_SEC_PKG_ERROR - A security package specific error occurred.
.

MessageId=1826
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_CANCELLED
Language=Bulgarian
RPC_S_NOT_CANCELLED - Thread is not canceled.
.

MessageId=1827
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_ES_ACTION
Language=Bulgarian
RPC_X_INVALID_ES_ACTION - Invalid operation on the encoding/decoding handle.
.

MessageId=1828
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_ES_VERSION
Language=Bulgarian
RPC_X_WRONG_ES_VERSION - Incompatible version of the serializing package.
.

MessageId=1829
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_STUB_VERSION
Language=Bulgarian
RPC_X_WRONG_STUB_VERSION - Incompatible version of the RPC stub.
.

MessageId=1830
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_PIPE_OBJECT
Language=Bulgarian
RPC_X_INVALID_PIPE_OBJECT - The RPC pipe object is invalid or corrupted.
.

MessageId=1831
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_ORDER
Language=Bulgarian
RPC_X_WRONG_PIPE_ORDER - An invalid operation was attempted on an RPC pipe object.
.

MessageId=1832
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_VERSION
Language=Bulgarian
RPC_X_WRONG_PIPE_VERSION - Unsupported RPC pipe version.
.

MessageId=1898
Severity=Success
Facility=System
SymbolicName=RPC_S_GROUP_MEMBER_NOT_FOUND
Language=Bulgarian
RPC_S_GROUP_MEMBER_NOT_FOUND - The group member was not found.
.

MessageId=1899
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_CREATE
Language=Bulgarian
EPT_S_CANT_CREATE - The endpoint mapper database entry could not be created.
.

MessageId=1900
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_OBJECT
Language=Bulgarian
RPC_S_INVALID_OBJECT - The object universal unique identifier (UUID) is the nil UUID.
.

MessageId=1901
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TIME
Language=Bulgarian
ERROR_INVALID_TIME - The specified time is invalid.
.

MessageId=1902
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_NAME
Language=Bulgarian
ERROR_INVALID_FORM_NAME - The specified form name is invalid.
.

MessageId=1903
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_SIZE
Language=Bulgarian
ERROR_INVALID_FORM_SIZE - The specified form size is invalid.
.

MessageId=1904
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_WAITING
Language=Bulgarian
ERROR_ALREADY_WAITING - The specified printer handle is already being waited on
.

MessageId=1905
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DELETED
Language=Bulgarian
ERROR_PRINTER_DELETED - The specified printer has been deleted.
.

MessageId=1906
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_STATE
Language=Bulgarian
ERROR_INVALID_PRINTER_STATE - The state of the printer is invalid.
.

MessageId=1907
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_MUST_CHANGE
Language=Bulgarian
ERROR_PASSWORD_MUST_CHANGE - The user's password must be changed before logging on the first time.
.

MessageId=1908
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CONTROLLER_NOT_FOUND
Language=Bulgarian
ERROR_DOMAIN_CONTROLLER_NOT_FOUND - Could not find the domain controller for this domain.
.

MessageId=1909
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_LOCKED_OUT
Language=Bulgarian
ERROR_ACCOUNT_LOCKED_OUT - The referenced account is currently locked out and may not be used to log on.
.

MessageId=1910
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OXID
Language=Bulgarian
OR_INVALID_OXID - The object exporter specified was not found.
.

MessageId=1911
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OID
Language=Bulgarian
OR_INVALID_OID - The object specified was not found.
.

MessageId=1912
Severity=Success
Facility=System
SymbolicName=OR_INVALID_SET
Language=Bulgarian
OR_INVALID_SET - The object resolver set specified was not found.
.

MessageId=1913
Severity=Success
Facility=System
SymbolicName=RPC_S_SEND_INCOMPLETE
Language=Bulgarian
RPC_S_SEND_INCOMPLETE - Some data remains to be sent in the request buffer.
.

MessageId=1914
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ASYNC_HANDLE
Language=Bulgarian
RPC_S_INVALID_ASYNC_HANDLE - Invalid asynchronous remote procedure call handle.
.

MessageId=1915
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ASYNC_CALL
Language=Bulgarian
RPC_S_INVALID_ASYNC_CALL - Invalid asynchronous RPC call handle for this operation.
.

MessageId=1916
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_CLOSED
Language=Bulgarian
RPC_X_PIPE_CLOSED - The RPC pipe object has already been closed.
.

MessageId=1917
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_DISCIPLINE_ERROR
Language=Bulgarian
RPC_X_PIPE_DISCIPLINE_ERROR - The RPC call completed before all pipes were processed.
.

MessageId=1918
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_EMPTY
Language=Bulgarian
RPC_X_PIPE_EMPTY - No more data is available from the RPC pipe.
.

MessageId=1919
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SITENAME
Language=Bulgarian
ERROR_NO_SITENAME - No site name is available for this machine.
.

MessageId=1920
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ACCESS_FILE
Language=Bulgarian
ERROR_CANT_ACCESS_FILE - The file cannot be accessed by the system.
.

MessageId=1921
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_RESOLVE_FILENAME
Language=Bulgarian
ERROR_CANT_RESOLVE_FILENAME - The name of the file cannot be resolved by the system.
.

MessageId=1922
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_TYPE_MISMATCH
Language=Bulgarian
RPC_S_ENTRY_TYPE_MISMATCH - The entry is not of the expected type.
.

MessageId=1923
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_ALL_OBJS_EXPORTED
Language=Bulgarian
RPC_S_NOT_ALL_OBJS_EXPORTED - Not all object UUIDs could be exported to the specified entry.
.

MessageId=1924
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERFACE_NOT_EXPORTED
Language=Bulgarian
RPC_S_INTERFACE_NOT_EXPORTED - Interface could not be exported to the specified entry.
.

MessageId=1925
Severity=Success
Facility=System
SymbolicName=RPC_S_PROFILE_NOT_ADDED
Language=Bulgarian
RPC_S_PROFILE_NOT_ADDED - The specified profile entry could not be added.
.

MessageId=1926
Severity=Success
Facility=System
SymbolicName=RPC_S_PRF_ELT_NOT_ADDED
Language=Bulgarian
RPC_S_PRF_ELT_NOT_ADDED - The specified profile element could not be added.
.

MessageId=1927
Severity=Success
Facility=System
SymbolicName=RPC_S_PRF_ELT_NOT_REMOVED
Language=Bulgarian
RPC_S_PRF_ELT_NOT_REMOVED - The specified profile element could not be removed.
.

MessageId=1928
Severity=Success
Facility=System
SymbolicName=RPC_S_GRP_ELT_NOT_ADDED
Language=Bulgarian
RPC_S_GRP_ELT_NOT_ADDED - The group element could not be added.
.

MessageId=1929
Severity=Success
Facility=System
SymbolicName=RPC_S_GRP_ELT_NOT_REMOVED
Language=Bulgarian
RPC_S_GRP_ELT_NOT_REMOVED - The group element could not be removed.
.

MessageId=1930
Severity=Success
Facility=System
SymbolicName=ERROR_KM_DRIVER_BLOCKED
Language=Bulgarian
ERROR_KM_DRIVER_BLOCKED - The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers.
.

MessageId=1931
Severity=Success
Facility=System
SymbolicName=ERROR_CONTEXT_EXPIRED
Language=Bulgarian
ERROR_CONTEXT_EXPIRED - The context has expired and can no longer be used.
.

MessageId=1932
Severity=Success
Facility=System
SymbolicName=ERROR_PER_USER_TRUST_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_PER_USER_TRUST_QUOTA_EXCEEDED - The current user's delegated trust creation quota has been exceeded.
.

MessageId=1933
Severity=Success
Facility=System
SymbolicName=ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED - The total delegated trust creation quota has been exceeded.
.

MessageId=1934
Severity=Success
Facility=System
SymbolicName=ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED - The current user's delegated trust deletion quota has been exceeded.
.

MessageId=2000
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PIXEL_FORMAT
Language=Bulgarian
ERROR_INVALID_PIXEL_FORMAT - The pixel format is invalid.
.

MessageId=2001
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER
Language=Bulgarian
ERROR_BAD_DRIVER - The specified driver is invalid.
.

MessageId=2002
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_STYLE
Language=Bulgarian
ERROR_INVALID_WINDOW_STYLE - The window style or class attribute is invalid for this operation.
.

MessageId=2003
Severity=Success
Facility=System
SymbolicName=ERROR_METAFILE_NOT_SUPPORTED
Language=Bulgarian
ERROR_METAFILE_NOT_SUPPORTED - The requested metafile operation is not supported.
.

MessageId=2004
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSFORM_NOT_SUPPORTED
Language=Bulgarian
ERROR_TRANSFORM_NOT_SUPPORTED - The requested transformation operation is not supported.
.

MessageId=2005
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPPING_NOT_SUPPORTED
Language=Bulgarian
ERROR_CLIPPING_NOT_SUPPORTED - The requested clipping operation is not supported.
.

MessageId=2010
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CMM
Language=Bulgarian
ERROR_INVALID_CMM - The specified color management module is invalid.
.

MessageId=2011
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PROFILE
Language=Bulgarian
ERROR_INVALID_PROFILE - The specified color profile is invalid.
.

MessageId=2012
Severity=Success
Facility=System
SymbolicName=ERROR_TAG_NOT_FOUND
Language=Bulgarian
ERROR_TAG_NOT_FOUND - The specified tag was not found.
.

MessageId=2013
Severity=Success
Facility=System
SymbolicName=ERROR_TAG_NOT_PRESENT
Language=Bulgarian
ERROR_TAG_NOT_PRESENT - A required tag is not present.
.

MessageId=2014
Severity=Success
Facility=System
SymbolicName=ERROR_DUPLICATE_TAG
Language=Bulgarian
ERROR_DUPLICATE_TAG - The specified tag is already present.
.

MessageId=2015
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE
Language=Bulgarian
ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE - The specified color profile is not associated with any device.
.

MessageId=2016
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILE_NOT_FOUND
Language=Bulgarian
ERROR_PROFILE_NOT_FOUND - The specified color profile was not found.
.

MessageId=2017
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COLORSPACE
Language=Bulgarian
ERROR_INVALID_COLORSPACE - The specified color space is invalid.
.

MessageId=2018
Severity=Success
Facility=System
SymbolicName=ERROR_ICM_NOT_ENABLED
Language=Bulgarian
ERROR_ICM_NOT_ENABLED - Image Color Management is not enabled.
.

MessageId=2019
Severity=Success
Facility=System
SymbolicName=ERROR_DELETING_ICM_XFORM
Language=Bulgarian
ERROR_DELETING_ICM_XFORM - There was an error while deleting the color transform.
.

MessageId=2020
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TRANSFORM
Language=Bulgarian
ERROR_INVALID_TRANSFORM - The specified color transform is invalid.
.

MessageId=2021
Severity=Success
Facility=System
SymbolicName=ERROR_COLORSPACE_MISMATCH
Language=Bulgarian
ERROR_COLORSPACE_MISMATCH - The specified transform does not match the bitmap's color space.
.

MessageId=2022
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COLORINDEX
Language=Bulgarian
ERROR_INVALID_COLORINDEX - The specified named color index is not present in the profile.
.

MessageId=2108
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD
Language=Bulgarian
ERROR_CONNECTED_OTHER_PASSWORD - The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.
.

MessageId=2109
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT
Language=Bulgarian
ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT - The network connection was made successfully using default credentials.
.

MessageId=2202
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_USERNAME
Language=Bulgarian
ERROR_BAD_USERNAME - The specified username is invalid.
.

MessageId=2250
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONNECTED
Language=Bulgarian
ERROR_NOT_CONNECTED - This network connection does not exist.
.

MessageId=2401
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FILES
Language=Bulgarian
ERROR_OPEN_FILES - This network connection has files open or requests pending.
.

MessageId=2402
Severity=Success
Facility=System
SymbolicName=ERROR_ACTIVE_CONNECTIONS
Language=Bulgarian
ERROR_ACTIVE_CONNECTIONS - Active connections still exist.
.

MessageId=2404
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_IN_USE
Language=Bulgarian
ERROR_DEVICE_IN_USE - The device is in use by an active process and cannot be disconnected.
.

MessageId=3000
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINT_MONITOR
Language=Bulgarian
ERROR_UNKNOWN_PRINT_MONITOR - The specified print monitor is unknown.
.

MessageId=3001
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_IN_USE
Language=Bulgarian
ERROR_PRINTER_DRIVER_IN_USE - The specified printer driver is currently in use.
.

MessageId=3002
Severity=Success
Facility=System
SymbolicName=ERROR_SPOOL_FILE_NOT_FOUND
Language=Bulgarian
ERROR_SPOOL_FILE_NOT_FOUND - The spool file was not found.
.

MessageId=3003
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_STARTDOC
Language=Bulgarian
ERROR_SPL_NO_STARTDOC - A StartDocPrinter call was not issued.
.

MessageId=3004
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_ADDJOB
Language=Bulgarian
ERROR_SPL_NO_ADDJOB - An AddJob call was not issued.
.

MessageId=3005
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
Language=Bulgarian
ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED - The specified print processor has already been installed.
.

MessageId=3006
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_MONITOR_ALREADY_INSTALLED
Language=Bulgarian
ERROR_PRINT_MONITOR_ALREADY_INSTALLED - The specified print monitor has already been installed.
.

MessageId=3007
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINT_MONITOR
Language=Bulgarian
ERROR_INVALID_PRINT_MONITOR - The specified print monitor does not have the required functions.
.

MessageId=3008
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_MONITOR_IN_USE
Language=Bulgarian
ERROR_PRINT_MONITOR_IN_USE - The specified print monitor is currently in use.
.

MessageId=3009
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_HAS_JOBS_QUEUED
Language=Bulgarian
ERROR_PRINTER_HAS_JOBS_QUEUED - The requested operation is not allowed when there are jobs queued to the printer.
.

MessageId=3010
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_REBOOT_REQUIRED
Language=Bulgarian
ERROR_SUCCESS_REBOOT_REQUIRED - The requested operation is successful. Changes will not be effective until the system is rebooted.
.

MessageId=3011
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_RESTART_REQUIRED
Language=Bulgarian
ERROR_SUCCESS_RESTART_REQUIRED - The requested operation is successful. Changes will not be effective until the service is restarted.
.

MessageId=3012
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_NOT_FOUND
Language=Bulgarian
ERROR_PRINTER_NOT_FOUND - No printers were found.
.

MessageId=3013
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_WARNED
Language=Bulgarian
ERROR_PRINTER_DRIVER_WARNED - The printer driver is known to be unreliable.
.

MessageId=3014
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_BLOCKED
Language=Bulgarian
ERROR_PRINTER_DRIVER_BLOCKED - The printer driver is known to harm the system.
.

MessageId=3100
Severity=Success
Facility=System
SymbolicName=ERROR_XML_UNDEFINED_ENTITY
Language=Bulgarian
ERROR_XML_UNDEFINED_ENTITY - The XML contains an entity reference to an undefined entity.
.

MessageId=3101
Severity=Success
Facility=System
SymbolicName=ERROR_XML_MALFORMED_ENTITY
Language=Bulgarian
ERROR_XML_MALFORMED_ENTITY - The XML contains a malformed entity reference.
.

MessageId=3102
Severity=Success
Facility=System
SymbolicName=ERROR_XML_CHAR_NOT_IN_RANGE
Language=Bulgarian
ERROR_XML_CHAR_NOT_IN_RANGE - The XML contains a character which is not permitted in XML.
.

MessageId=3200
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_EXTERNAL_PROXY
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_EXTERNAL_PROXY - The manifest contained a duplicate definition for external proxy stub %1 at (%1:%2,%3)
.

MessageId=3201
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_ASSEMBLY_REFERENCE
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_ASSEMBLY_REFERENCE - The manifest already contains a reference to %4 - a second reference was found at (%1:%2,%3)
.

MessageId=3202
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ASSEMBLY_REFERENCE
Language=Bulgarian
ERROR_PCM_COMPILER_INVALID_ASSEMBLY_REFERENCE - The assembly reference at (%1:%2,%3) is invalid.
.

MessageId=3203
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ASSEMBLY_DEFINITION
Language=Bulgarian
ERROR_PCM_COMPILER_INVALID_ASSEMBLY_DEFINITION - The assembly definition at (%1:%2,%3) is invalid.
.

MessageId=3204
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_WINDOW_CLASS
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_WINDOW_CLASS - The manifest already contained the window class %4, found a second declaration at (%1:%2,%3)
.

MessageId=3205
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_PROGID
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_PROGID - The manifest already declared the progId %4, found a second declaration at (%1:%2,%3)
.

MessageId=3206
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_NOINHERIT
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_NOINHERIT - Only one noInherit tag may be present in a manifest, found a second tag at (%1:%2,%3)
.

MessageId=3207
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_NOINHERITABLE
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_NOINHERITABLE - Only one noInheritable tag may be present in a manifest, found a second tag at (%1:%2,%3)
.

MessageId=3208
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_COM_CLASS
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_COM_CLASS - The manifest contained a duplicate declaration of COM class %4 at (%1:%2,%3)
.

MessageId=3209
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_FILE_NAME
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_FILE_NAME - The manifest already declared the file %4, a second definition was found at (%1:%2,%3)
.

MessageId=3210
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_CLR_SURROGATE
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_CLR_SURROGATE - CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3211
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_TYPE_LIBRARY
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_TYPE_LIBRARY - Type library %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3212
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_PROXY_STUB
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_PROXY_STUB - Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3213
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_CATEGORY_NAME
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_CATEGORY_NAME - Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid.
.

MessageId=3214
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_TOP_LEVEL_IDENTITY_FOUND
Language=Bulgarian
ERROR_PCM_COMPILER_DUPLICATE_TOP_LEVEL_IDENTITY_FOUND - Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3)
.

MessageId=3215
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_UNKNOWN_ROOT_ELEMENT
Language=Bulgarian
ERROR_PCM_COMPILER_UNKNOWN_ROOT_ELEMENT - The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version.
.

MessageId=3216
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ELEMENT
Language=Bulgarian
ERROR_PCM_COMPILER_INVALID_ELEMENT - The element found at (%1:%2,%3) was not expected according to the manifest schema.
.

MessageId=3217
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_MISSING_REQUIRED_ATTRIBUTE
Language=Bulgarian
ERROR_PCM_COMPILER_MISSING_REQUIRED_ATTRIBUTE - The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information
.

MessageId=3218
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ATTRIBUTE_VALUE
Language=Bulgarian
ERROR_PCM_COMPILER_INVALID_ATTRIBUTE_VALUE - The attribute value %4 at (%1:%2,%3) was invalid according to the schema.
.

MessageId=3219
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_UNEXPECTED_PCDATA
Language=Bulgarian
ERROR_PCM_COMPILER_UNEXPECTED_PCDATA - PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4.
.

MessageId=3220
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_DUPLICATE_STRING_TABLE_ENT
Language=Bulgarian
ERROR_PCM_DUPLICATE_STRING_TABLE_ENT - The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry.
.

MessageId=4000
Severity=Success
Facility=System
SymbolicName=ERROR_WINS_INTERNAL
Language=Bulgarian
ERROR_WINS_INTERNAL - WINS encountered an error while processing the command.
.

MessageId=4001
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_DEL_LOCAL_WINS
Language=Bulgarian
ERROR_CAN_NOT_DEL_LOCAL_WINS - The local WINS cannot be deleted.
.

MessageId=4002
Severity=Success
Facility=System
SymbolicName=ERROR_STATIC_INIT
Language=Bulgarian
ERROR_STATIC_INIT - The importation from the file failed.
.

MessageId=4003
Severity=Success
Facility=System
SymbolicName=ERROR_INC_BACKUP
Language=Bulgarian
ERROR_INC_BACKUP - The backup failed. Was a full backup done before?
.

MessageId=4004
Severity=Success
Facility=System
SymbolicName=ERROR_FULL_BACKUP
Language=Bulgarian
ERROR_FULL_BACKUP - The backup failed. Check the directory to which you are backing the database.
.

MessageId=4005
Severity=Success
Facility=System
SymbolicName=ERROR_REC_NON_EXISTENT
Language=Bulgarian
ERROR_REC_NON_EXISTENT - The name does not exist in the WINS database.
.

MessageId=4006
Severity=Success
Facility=System
SymbolicName=ERROR_RPL_NOT_ALLOWED
Language=Bulgarian
ERROR_RPL_NOT_ALLOWED - Replication with a nonconfigured partner is not allowed.
.

MessageId=4100
Severity=Success
Facility=System
SymbolicName=ERROR_DHCP_ADDRESS_CONFLICT
Language=Bulgarian
ERROR_DHCP_ADDRESS_CONFLICT - The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.
.

MessageId=4200
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_GUID_NOT_FOUND
Language=Bulgarian
ERROR_WMI_GUID_NOT_FOUND - The GUID passed was not recognized as valid by a WMI data provider.
.

MessageId=4201
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INSTANCE_NOT_FOUND
Language=Bulgarian
ERROR_WMI_INSTANCE_NOT_FOUND - The instance name passed was not recognized as valid by a WMI data provider.
.

MessageId=4202
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ITEMID_NOT_FOUND
Language=Bulgarian
ERROR_WMI_ITEMID_NOT_FOUND - The data item ID passed was not recognized as valid by a WMI data provider.
.

MessageId=4203
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_TRY_AGAIN
Language=Bulgarian
ERROR_WMI_TRY_AGAIN - The WMI request could not be completed and should be retried.
.

MessageId=4204
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_DP_NOT_FOUND
Language=Bulgarian
ERROR_WMI_DP_NOT_FOUND - The WMI data provider could not be located.
.

MessageId=4205
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_UNRESOLVED_INSTANCE_REF
Language=Bulgarian
ERROR_WMI_UNRESOLVED_INSTANCE_REF - The WMI data provider references an instance set that has not been registered.
.

MessageId=4206
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ALREADY_ENABLED
Language=Bulgarian
ERROR_WMI_ALREADY_ENABLED - The WMI data block or event notification has already been enabled.
.

MessageId=4207
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_GUID_DISCONNECTED
Language=Bulgarian
ERROR_WMI_GUID_DISCONNECTED - The WMI data block is no longer available.
.

MessageId=4208
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_SERVER_UNAVAILABLE
Language=Bulgarian
ERROR_WMI_SERVER_UNAVAILABLE - The WMI data service is not available.
.

MessageId=4209
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_DP_FAILED
Language=Bulgarian
ERROR_WMI_DP_FAILED - The WMI data provider failed to carry out the request.
.

MessageId=4210
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INVALID_MOF
Language=Bulgarian
ERROR_WMI_INVALID_MOF - The WMI MOF information is not valid.
.

MessageId=4211
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INVALID_REGINFO
Language=Bulgarian
ERROR_WMI_INVALID_REGINFO - The WMI registration information is not valid.
.

MessageId=4212
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ALREADY_DISABLED
Language=Bulgarian
ERROR_WMI_ALREADY_DISABLED - The WMI data block or event notification has already been disabled.
.

MessageId=4213
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_READ_ONLY
Language=Bulgarian
ERROR_WMI_READ_ONLY - The WMI data item or data block is read only.
.

MessageId=4214
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_SET_FAILURE
Language=Bulgarian
ERROR_WMI_SET_FAILURE - The WMI data item or data block could not be changed.
.

MessageId=4300
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEDIA
Language=Bulgarian
ERROR_INVALID_MEDIA - The media identifier does not represent a valid medium.
.

MessageId=4301
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LIBRARY
Language=Bulgarian
ERROR_INVALID_LIBRARY - The library identifier does not represent a valid library.
.

MessageId=4302
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEDIA_POOL
Language=Bulgarian
ERROR_INVALID_MEDIA_POOL - The media pool identifier does not represent a valid media pool.
.

MessageId=4303
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVE_MEDIA_MISMATCH
Language=Bulgarian
ERROR_DRIVE_MEDIA_MISMATCH - The drive and medium are not compatible or exist in different libraries.
.

MessageId=4304
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_OFFLINE
Language=Bulgarian
ERROR_MEDIA_OFFLINE - The medium currently exists in an offline library and must be online to perform this operation.
.

MessageId=4305
Severity=Success
Facility=System
SymbolicName=ERROR_LIBRARY_OFFLINE
Language=Bulgarian
ERROR_LIBRARY_OFFLINE - The operation cannot be performed on an offline library.
.

MessageId=4306
Severity=Success
Facility=System
SymbolicName=ERROR_EMPTY
Language=Bulgarian
ERROR_EMPTY - The library, drive, or media pool is empty.
.

MessageId=4307
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_EMPTY
Language=Bulgarian
ERROR_NOT_EMPTY - The library, drive, or media pool must be empty to perform this operation.
.

MessageId=4308
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_UNAVAILABLE
Language=Bulgarian
ERROR_MEDIA_UNAVAILABLE - No media is currently available in this media pool or library.
.

MessageId=4309
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_DISABLED
Language=Bulgarian
ERROR_RESOURCE_DISABLED - A resource required for this operation is disabled.
.

MessageId=4310
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CLEANER
Language=Bulgarian
ERROR_INVALID_CLEANER - The media identifier does not represent a valid cleaner.
.

MessageId=4311
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_CLEAN
Language=Bulgarian
ERROR_UNABLE_TO_CLEAN - The drive cannot be cleaned or does not support cleaning.
.

MessageId=4312
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_NOT_FOUND
Language=Bulgarian
ERROR_OBJECT_NOT_FOUND - The object identifier does not represent a valid object.
.

MessageId=4313
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_FAILURE
Language=Bulgarian
ERROR_DATABASE_FAILURE - Unable to read from or write to the database.
.

MessageId=4314
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_FULL
Language=Bulgarian
ERROR_DATABASE_FULL - The database is full.
.

MessageId=4315
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_INCOMPATIBLE
Language=Bulgarian
ERROR_MEDIA_INCOMPATIBLE - The medium is not compatible with the device or media pool.
.

MessageId=4316
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_PRESENT
Language=Bulgarian
ERROR_RESOURCE_NOT_PRESENT - The resource required for this operation does not exist.
.

MessageId=4317
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPERATION
Language=Bulgarian
ERROR_INVALID_OPERATION - The operation identifier is not valid.
.

MessageId=4318
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_NOT_AVAILABLE
Language=Bulgarian
ERROR_MEDIA_NOT_AVAILABLE - The media is not mounted or ready for use.
.

MessageId=4319
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_AVAILABLE
Language=Bulgarian
ERROR_DEVICE_NOT_AVAILABLE - The device is not ready for use.
.

MessageId=4320
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_REFUSED
Language=Bulgarian
ERROR_REQUEST_REFUSED - The operator or administrator has refused the request.
.

MessageId=4321
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DRIVE_OBJECT
Language=Bulgarian
ERROR_INVALID_DRIVE_OBJECT - The drive identifier does not represent a valid drive.
.

MessageId=4322
Severity=Success
Facility=System
SymbolicName=ERROR_LIBRARY_FULL
Language=Bulgarian
ERROR_LIBRARY_FULL - Library is full. No slot is available for use.
.

MessageId=4323
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIUM_NOT_ACCESSIBLE
Language=Bulgarian
ERROR_MEDIUM_NOT_ACCESSIBLE - The transport cannot access the medium.
.

MessageId=4324
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_LOAD_MEDIUM
Language=Bulgarian
ERROR_UNABLE_TO_LOAD_MEDIUM - Unable to load the medium into the drive.
.

MessageId=4325
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_DRIVE
Language=Bulgarian
ERROR_UNABLE_TO_INVENTORY_DRIVE - Unable to retrieve status about the drive.
.

MessageId=4326
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_SLOT
Language=Bulgarian
ERROR_UNABLE_TO_INVENTORY_SLOT - Unable to retrieve status about the slot.
.

MessageId=4327
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_TRANSPORT
Language=Bulgarian
ERROR_UNABLE_TO_INVENTORY_TRANSPORT - Unable to retrieve status about the transport.
.

MessageId=4328
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSPORT_FULL
Language=Bulgarian
ERROR_TRANSPORT_FULL - Cannot use the transport because it is already in use.
.

MessageId=4329
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROLLING_IEPORT
Language=Bulgarian
ERROR_CONTROLLING_IEPORT - Unable to open or close the inject/eject port.
.

MessageId=4330
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA
Language=Bulgarian
ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA - Unable to eject the media because it is in a drive.
.

MessageId=4331
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_SLOT_SET
Language=Bulgarian
ERROR_CLEANER_SLOT_SET - A cleaner slot is already reserved.
.

MessageId=4332
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_SLOT_NOT_SET
Language=Bulgarian
ERROR_CLEANER_SLOT_NOT_SET - A cleaner slot is not reserved.
.

MessageId=4333
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_CARTRIDGE_SPENT
Language=Bulgarian
ERROR_CLEANER_CARTRIDGE_SPENT - The cleaner cartridge has performed the maximum number of drive cleanings.
.

MessageId=4334
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_OMID
Language=Bulgarian
ERROR_UNEXPECTED_OMID - Unexpected on-medium identifier.
.

MessageId=4335
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_DELETE_LAST_ITEM
Language=Bulgarian
ERROR_CANT_DELETE_LAST_ITEM - The last remaining item in this group or resource cannot be deleted.
.

MessageId=4336
Severity=Success
Facility=System
SymbolicName=ERROR_MESSAGE_EXCEEDS_MAX_SIZE
Language=Bulgarian
ERROR_MESSAGE_EXCEEDS_MAX_SIZE - The message provided exceeds the maximum size allowed for this parameter.
.

MessageId=4337
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_CONTAINS_SYS_FILES
Language=Bulgarian
ERROR_VOLUME_CONTAINS_SYS_FILES - The volume contains system or paging files.
.

MessageId=4338
Severity=Success
Facility=System
SymbolicName=ERROR_INDIGENOUS_TYPE
Language=Bulgarian
ERROR_INDIGENOUS_TYPE - The media type cannot be removed from this library since at least one drive in the library reports it can support this media type.
.

MessageId=4339
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUPPORTING_DRIVES
Language=Bulgarian
ERROR_NO_SUPPORTING_DRIVES - This offline media cannot be mounted on this system since no enabled drives are present which can be used.
.

MessageId=4340
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_CARTRIDGE_INSTALLED
Language=Bulgarian
ERROR_CLEANER_CARTRIDGE_INSTALLED - A cleaner cartridge is present in the tape library.
.

MessageId=4350
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_OFFLINE
Language=Bulgarian
ERROR_FILE_OFFLINE - The remote storage service was not able to recall the file.
.

MessageId=4351
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_STORAGE_NOT_ACTIVE
Language=Bulgarian
ERROR_REMOTE_STORAGE_NOT_ACTIVE - The remote storage service is not operational at this time.
.

MessageId=4352
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_STORAGE_MEDIA_ERROR
Language=Bulgarian
ERROR_REMOTE_STORAGE_MEDIA_ERROR - The remote storage service encountered a media error.
.

MessageId=4390
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_A_REPARSE_POINT
Language=Bulgarian
ERROR_NOT_A_REPARSE_POINT - The file or directory is not a reparse point.
.

MessageId=4391
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_ATTRIBUTE_CONFLICT
Language=Bulgarian
ERROR_REPARSE_ATTRIBUTE_CONFLICT - The reparse point attribute cannot be set because it conflicts with an existing attribute.
.

MessageId=4392
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_REPARSE_DATA
Language=Bulgarian
ERROR_INVALID_REPARSE_DATA - The data present in the reparse point buffer is invalid.
.

MessageId=4393
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_TAG_INVALID
Language=Bulgarian
ERROR_REPARSE_TAG_INVALID - The tag present in the reparse point buffer is invalid.
.

MessageId=4394
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_TAG_MISMATCH
Language=Bulgarian
ERROR_REPARSE_TAG_MISMATCH - There is a mismatch between the tag specified in the request and the tag present in the reparse point.
.

MessageId=4500
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_NOT_SIS_ENABLED
Language=Bulgarian
ERROR_VOLUME_NOT_SIS_ENABLED - Single Instance Storage is not available on this volume.
.

MessageId=5001
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENT_RESOURCE_EXISTS
Language=Bulgarian
ERROR_DEPENDENT_RESOURCE_EXISTS - The cluster resource cannot be moved to another group because other resources are dependent on it.
.

MessageId=5002
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_NOT_FOUND
Language=Bulgarian
ERROR_DEPENDENCY_NOT_FOUND - The cluster resource dependency cannot be found.
.

MessageId=5003
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_ALREADY_EXISTS
Language=Bulgarian
ERROR_DEPENDENCY_ALREADY_EXISTS - The cluster resource cannot be made dependent on the specified resource because it is already dependent.
.

MessageId=5004
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_ONLINE
Language=Bulgarian
ERROR_RESOURCE_NOT_ONLINE - The cluster resource is not online.
.

MessageId=5005
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_AVAILABLE
Language=Bulgarian
ERROR_HOST_NODE_NOT_AVAILABLE - A cluster node is not available for this operation.
.

MessageId=5006
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_AVAILABLE
Language=Bulgarian
ERROR_RESOURCE_NOT_AVAILABLE - The cluster resource is not available.
.

MessageId=5007
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_FOUND
Language=Bulgarian
ERROR_RESOURCE_NOT_FOUND - The cluster resource could not be found.
.

MessageId=5008
Severity=Success
Facility=System
SymbolicName=ERROR_SHUTDOWN_CLUSTER
Language=Bulgarian
ERROR_SHUTDOWN_CLUSTER - The cluster is being shut down.
.

MessageId=5009
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_EVICT_ACTIVE_NODE
Language=Bulgarian
ERROR_CANT_EVICT_ACTIVE_NODE - A cluster node cannot be evicted from the cluster unless the node is down.
.

MessageId=5010
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_ALREADY_EXISTS
Language=Bulgarian
ERROR_OBJECT_ALREADY_EXISTS - The object already exists.
.

MessageId=5011
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_IN_LIST
Language=Bulgarian
ERROR_OBJECT_IN_LIST - The object is already in the list.
.

MessageId=5012
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_AVAILABLE
Language=Bulgarian
ERROR_GROUP_NOT_AVAILABLE - The cluster group is not available for any new requests.
.

MessageId=5013
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_FOUND
Language=Bulgarian
ERROR_GROUP_NOT_FOUND - The cluster group could not be found.
.

MessageId=5014
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_ONLINE
Language=Bulgarian
ERROR_GROUP_NOT_ONLINE - The operation could not be completed because the cluster group is not online.
.

MessageId=5015
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_RESOURCE_OWNER
Language=Bulgarian
ERROR_HOST_NODE_NOT_RESOURCE_OWNER - The cluster node is not the owner of the resource.
.

MessageId=5016
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_GROUP_OWNER
Language=Bulgarian
ERROR_HOST_NODE_NOT_GROUP_OWNER - The cluster node is not the owner of the group.
.

MessageId=5017
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_CREATE_FAILED
Language=Bulgarian
ERROR_RESMON_CREATE_FAILED - The cluster resource could not be created in the specified resource monitor.
.

MessageId=5018
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_ONLINE_FAILED
Language=Bulgarian
ERROR_RESMON_ONLINE_FAILED - The cluster resource could not be brought online by the resource monitor.
.

MessageId=5019
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_ONLINE
Language=Bulgarian
ERROR_RESOURCE_ONLINE - The operation could not be completed because the cluster resource is online.
.

MessageId=5020
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_RESOURCE
Language=Bulgarian
ERROR_QUORUM_RESOURCE - The cluster resource could not be deleted or brought offline because it is the quorum resource.
.

MessageId=5021
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_QUORUM_CAPABLE
Language=Bulgarian
ERROR_NOT_QUORUM_CAPABLE - The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.
.

MessageId=5022
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_SHUTTING_DOWN
Language=Bulgarian
ERROR_CLUSTER_SHUTTING_DOWN - The cluster software is shutting down.
.

MessageId=5023
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STATE
Language=Bulgarian
ERROR_INVALID_STATE - The group or resource is not in the correct state to perform the requested operation.
.

MessageId=5024
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_PROPERTIES_STORED
Language=Bulgarian
ERROR_RESOURCE_PROPERTIES_STORED - The properties were stored but not all changes will take effect until the next time the resource is brought online.
.

MessageId=5025
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_QUORUM_CLASS
Language=Bulgarian
ERROR_NOT_QUORUM_CLASS - The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.
.

MessageId=5026
Severity=Success
Facility=System
SymbolicName=ERROR_CORE_RESOURCE
Language=Bulgarian
ERROR_CORE_RESOURCE - The cluster resource could not be deleted since it is a core resource.
.

MessageId=5027
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_RESOURCE_ONLINE_FAILED
Language=Bulgarian
ERROR_QUORUM_RESOURCE_ONLINE_FAILED - The quorum resource failed to come online.
.

MessageId=5028
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUMLOG_OPEN_FAILED
Language=Bulgarian
ERROR_QUORUMLOG_OPEN_FAILED - The quorum log could not be created or mounted successfully.
.

MessageId=5029
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_CORRUPT
Language=Bulgarian
ERROR_CLUSTERLOG_CORRUPT - The cluster log is corrupt.
.

MessageId=5030
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE
Language=Bulgarian
ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE - The record could not be written to the cluster log since it exceeds the maximum size.
.

MessageId=5031
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE
Language=Bulgarian
ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE - The cluster log exceeds its maximum size.
.

MessageId=5032
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND - No checkpoint record was found in the cluster log.
.

MessageId=5033
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE
Language=Bulgarian
ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE - The minimum required disk space needed for logging is not available.
.

MessageId=5034
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_OWNER_ALIVE
Language=Bulgarian
ERROR_QUORUM_OWNER_ALIVE - The cluster node failed to take control of the quorum resource because the resource is owned by another active node.
.

MessageId=5035
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_NOT_AVAILABLE
Language=Bulgarian
ERROR_NETWORK_NOT_AVAILABLE - A cluster network is not available for this operation.
.

MessageId=5036
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_NOT_AVAILABLE
Language=Bulgarian
ERROR_NODE_NOT_AVAILABLE - A cluster node is not available for this operation.
.

MessageId=5037
Severity=Success
Facility=System
SymbolicName=ERROR_ALL_NODES_NOT_AVAILABLE
Language=Bulgarian
ERROR_ALL_NODES_NOT_AVAILABLE - All cluster nodes must be running to perform this operation.
.

MessageId=5038
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_FAILED
Language=Bulgarian
ERROR_RESOURCE_FAILED - A cluster resource failed.
.

MessageId=5039
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NODE
Language=Bulgarian
ERROR_CLUSTER_INVALID_NODE - The cluster node is not valid.
.

MessageId=5040
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_EXISTS
Language=Bulgarian
ERROR_CLUSTER_NODE_EXISTS - The cluster node already exists.
.

MessageId=5041
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_IN_PROGRESS
Language=Bulgarian
ERROR_CLUSTER_JOIN_IN_PROGRESS - A node is in the process of joining the cluster.
.

MessageId=5042
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_NODE_NOT_FOUND - The cluster node was not found.
.

MessageId=5043
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND - The cluster local node information was not found.
.

MessageId=5044
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_EXISTS
Language=Bulgarian
ERROR_CLUSTER_NETWORK_EXISTS - The cluster network already exists.
.

MessageId=5045
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_NETWORK_NOT_FOUND - The cluster network was not found.
.

MessageId=5046
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETINTERFACE_EXISTS
Language=Bulgarian
ERROR_CLUSTER_NETINTERFACE_EXISTS - The cluster network interface already exists.
.

MessageId=5047
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_NETINTERFACE_NOT_FOUND - The cluster network interface was not found.
.

MessageId=5048
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_REQUEST
Language=Bulgarian
ERROR_CLUSTER_INVALID_REQUEST - The cluster request is not valid for this object.
.

MessageId=5049
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NETWORK_PROVIDER
Language=Bulgarian
ERROR_CLUSTER_INVALID_NETWORK_PROVIDER - The cluster network provider is not valid.
.

MessageId=5050
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_DOWN
Language=Bulgarian
ERROR_CLUSTER_NODE_DOWN - The cluster node is down.
.

MessageId=5051
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_UNREACHABLE
Language=Bulgarian
ERROR_CLUSTER_NODE_UNREACHABLE - The cluster node is not reachable.
.

MessageId=5052
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_MEMBER
Language=Bulgarian
ERROR_CLUSTER_NODE_NOT_MEMBER - The cluster node is not a member of the cluster.
.

MessageId=5053
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS
Language=Bulgarian
ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS - A cluster join operation is not in progress.
.

MessageId=5054
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NETWORK
Language=Bulgarian
ERROR_CLUSTER_INVALID_NETWORK - The cluster network is not valid.
.

MessageId=5056
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_UP
Language=Bulgarian
ERROR_CLUSTER_NODE_UP - The cluster node is up.
.

MessageId=5057
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_IPADDR_IN_USE
Language=Bulgarian
ERROR_CLUSTER_IPADDR_IN_USE - The cluster IP address is already in use.
.

MessageId=5058
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_PAUSED
Language=Bulgarian
ERROR_CLUSTER_NODE_NOT_PAUSED - The cluster node is not paused.
.

MessageId=5059
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_SECURITY_CONTEXT
Language=Bulgarian
ERROR_CLUSTER_NO_SECURITY_CONTEXT - No cluster security context is available.
.

MessageId=5060
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_INTERNAL
Language=Bulgarian
ERROR_CLUSTER_NETWORK_NOT_INTERNAL - The cluster network is not configured for internal cluster communication.
.

MessageId=5061
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_UP
Language=Bulgarian
ERROR_CLUSTER_NODE_ALREADY_UP - The cluster node is already up.
.

MessageId=5062
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_DOWN
Language=Bulgarian
ERROR_CLUSTER_NODE_ALREADY_DOWN - The cluster node is already down.
.

MessageId=5063
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_ONLINE
Language=Bulgarian
ERROR_CLUSTER_NETWORK_ALREADY_ONLINE - The cluster network is already online.
.

MessageId=5064
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE
Language=Bulgarian
ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE - The cluster network is already offline.
.

MessageId=5065
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_MEMBER
Language=Bulgarian
ERROR_CLUSTER_NODE_ALREADY_MEMBER - The cluster node is already a member of the cluster.
.

MessageId=5066
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_LAST_INTERNAL_NETWORK
Language=Bulgarian
ERROR_CLUSTER_LAST_INTERNAL_NETWORK - The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network.
.

MessageId=5067
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS
Language=Bulgarian
ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS - One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network.
.

MessageId=5068
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPERATION_ON_QUORUM
Language=Bulgarian
ERROR_INVALID_OPERATION_ON_QUORUM - This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list.
.

MessageId=5069
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_NOT_ALLOWED
Language=Bulgarian
ERROR_DEPENDENCY_NOT_ALLOWED - The cluster quorum resource is not allowed to have any dependencies.
.

MessageId=5070
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_PAUSED
Language=Bulgarian
ERROR_CLUSTER_NODE_PAUSED - The cluster node is paused.
.

MessageId=5071
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_CANT_HOST_RESOURCE
Language=Bulgarian
ERROR_NODE_CANT_HOST_RESOURCE - The cluster resource cannot be brought online. The owner node cannot run this resource.
.

MessageId=5072
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_READY
Language=Bulgarian
ERROR_CLUSTER_NODE_NOT_READY - The cluster node is not ready to perform the requested operation.
.

MessageId=5073
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_SHUTTING_DOWN
Language=Bulgarian
ERROR_CLUSTER_NODE_SHUTTING_DOWN - The cluster node is shutting down.
.

MessageId=5074
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_ABORTED
Language=Bulgarian
ERROR_CLUSTER_JOIN_ABORTED - The cluster join operation was aborted.
.

MessageId=5075
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INCOMPATIBLE_VERSIONS
Language=Bulgarian
ERROR_CLUSTER_INCOMPATIBLE_VERSIONS - The cluster join operation failed due to incompatible software versions between the joining node and its sponsor.
.

MessageId=5076
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED
Language=Bulgarian
ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED - This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor.
.

MessageId=5077
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED
Language=Bulgarian
ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED - The system configuration changed during the cluster join or form operation. The join or form operation was aborted.
.

MessageId=5078
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND - The specified resource type was not found.
.

MessageId=5079
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED
Language=Bulgarian
ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED - The specified node does not support a resource of this type. This may be due to version inconsistencies or due to the absence of the resource DLL on this node.
.

MessageId=5080
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESNAME_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_RESNAME_NOT_FOUND - The specified resource name is supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL.
.

MessageId=5081
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED
Language=Bulgarian
ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED - No authentication package could be registered with the RPC server.
.

MessageId=5082
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST
Language=Bulgarian
ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST - You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group.
.

MessageId=5083
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_DATABASE_SEQMISMATCH
Language=Bulgarian
ERROR_CLUSTER_DATABASE_SEQMISMATCH - The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join.
.

MessageId=5084
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_INVALID_STATE
Language=Bulgarian
ERROR_RESMON_INVALID_STATE - The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state.
.

MessageId=5085
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_GUM_NOT_LOCKER
Language=Bulgarian
ERROR_CLUSTER_GUM_NOT_LOCKER - A non locker code got a request to reserve the lock for making global updates.
.

MessageId=5086
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_DISK_NOT_FOUND
Language=Bulgarian
ERROR_QUORUM_DISK_NOT_FOUND - The quorum disk could not be located by the cluster service.
.

MessageId=5087
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_BACKUP_CORRUPT
Language=Bulgarian
ERROR_DATABASE_BACKUP_CORRUPT - The backup up cluster database is possibly corrupt.
.

MessageId=5088
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT
Language=Bulgarian
ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT - A DFS root already exists in this cluster node.
.

MessageId=5089
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_PROPERTY_UNCHANGEABLE
Language=Bulgarian
ERROR_RESOURCE_PROPERTY_UNCHANGEABLE - An attempt to modify a resource property failed because it conflicts with another existing property.
.

MessageId=5890
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE
Language=Bulgarian
ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE - An operation was attempted that is incompatible with the current membership state of the node.
.

MessageId=5891
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_QUORUMLOG_NOT_FOUND
Language=Bulgarian
ERROR_CLUSTER_QUORUMLOG_NOT_FOUND - The quorum resource does not contain the quorum log.
.

MessageId=5892
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MEMBERSHIP_HALT
Language=Bulgarian
ERROR_CLUSTER_MEMBERSHIP_HALT - The membership engine requested shutdown of the cluster service on this node.
.

MessageId=5893
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INSTANCE_ID_MISMATCH
Language=Bulgarian
ERROR_CLUSTER_INSTANCE_ID_MISMATCH - The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node.
.

MessageId=5894
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP
Language=Bulgarian
ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP - A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.

MessageId=5895
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH
Language=Bulgarian
ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH - The actual data type of the property did not match the expected data type of the property.
.

MessageId=5896
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP
Language=Bulgarian
ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP - The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available.
.

MessageId=5897
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_PARAMETER_MISMATCH
Language=Bulgarian
ERROR_CLUSTER_PARAMETER_MISMATCH - Two or more parameter values specified for a resource's properties are in conflict.
.

MessageId=5898
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_CANNOT_BE_CLUSTERED
Language=Bulgarian
ERROR_NODE_CANNOT_BE_CLUSTERED - This computer cannot be made a member of a cluster.
.

MessageId=5899
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_WRONG_OS_VERSION
Language=Bulgarian
ERROR_CLUSTER_WRONG_OS_VERSION - This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed.
.

MessageId=5900
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME
Language=Bulgarian
ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME - A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster.
.

MessageId=5901
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_ALREADY_COMMITTED
Language=Bulgarian
ERROR_CLUSCFG_ALREADY_COMMITTED - The cluster configuration action has already been committed.
.

MessageId=5902
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_ROLLBACK_FAILED
Language=Bulgarian
ERROR_CLUSCFG_ROLLBACK_FAILED - The cluster configuration action could not be rolled back.
.

MessageId=5903
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT
Language=Bulgarian
ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT - The drive letter assigned to a system disk on one node conflicted with the driver letter assigned to a disk on another node.
.

MessageId=5904
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_OLD_VERSION
Language=Bulgarian
ERROR_CLUSTER_OLD_VERSION - One or more nodes in the cluster are running a version of Windows that does not support this operation.
.

MessageId=5905
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME
Language=Bulgarian
ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME - The name of the corresponding computer account doesn't match the Network Name for this resource.
.

MessageId=5906
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_NET_ADAPTERS
Language=Bulgarian
ERROR_CLUSTER_NO_NET_ADAPTERS - No network adapters are available.
.

MessageId=5907
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_POISONED
Language=Bulgarian
ERROR_CLUSTER_POISONED - The cluster node has been poisoned.
.

MessageId=6000
Severity=Success
Facility=System
SymbolicName=ERROR_ENCRYPTION_FAILED
Language=Bulgarian
ERROR_ENCRYPTION_FAILED - The specified file could not be encrypted.
.

MessageId=6001
Severity=Success
Facility=System
SymbolicName=ERROR_DECRYPTION_FAILED
Language=Bulgarian
ERROR_DECRYPTION_FAILED - The specified file could not be decrypted.
.

MessageId=6002
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_ENCRYPTED
Language=Bulgarian
ERROR_FILE_ENCRYPTED - The specified file is encrypted and the user does not have the ability to decrypt it.
.

MessageId=6003
Severity=Success
Facility=System
SymbolicName=ERROR_NO_RECOVERY_POLICY
Language=Bulgarian
ERROR_NO_RECOVERY_POLICY - There is no valid encryption recovery policy configured for this system.
.

MessageId=6004
Severity=Success
Facility=System
SymbolicName=ERROR_NO_EFS
Language=Bulgarian
ERROR_NO_EFS - The required encryption driver is not loaded for this system.
.

MessageId=6005
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_EFS
Language=Bulgarian
ERROR_WRONG_EFS - The file was encrypted with a different encryption driver than is currently loaded.
.

MessageId=6006
Severity=Success
Facility=System
SymbolicName=ERROR_NO_USER_KEYS
Language=Bulgarian
ERROR_NO_USER_KEYS - There are no EFS keys defined for the user.
.

MessageId=6007
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_NOT_ENCRYPTED
Language=Bulgarian
ERROR_FILE_NOT_ENCRYPTED - The specified file is not encrypted.
.

MessageId=6008
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_EXPORT_FORMAT
Language=Bulgarian
ERROR_NOT_EXPORT_FORMAT - The specified file is not in the defined EFS export format.
.

MessageId=6009
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_READ_ONLY
Language=Bulgarian
ERROR_FILE_READ_ONLY - The specified file is read only.
.

MessageId=6010
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_EFS_DISALLOWED
Language=Bulgarian
ERROR_DIR_EFS_DISALLOWED - The directory has been disabled for encryption.
.

MessageId=6011
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_SERVER_NOT_TRUSTED
Language=Bulgarian
ERROR_EFS_SERVER_NOT_TRUSTED - The server is not trusted for remote encryption operation.
.

MessageId=6012
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_RECOVERY_POLICY
Language=Bulgarian
ERROR_BAD_RECOVERY_POLICY - Recovery policy configured for this system contains invalid recovery certificate.
.

MessageId=6013
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_ALG_BLOB_TOO_BIG
Language=Bulgarian
ERROR_EFS_ALG_BLOB_TOO_BIG - The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file.
.

MessageId=6014
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_NOT_SUPPORT_EFS
Language=Bulgarian
ERROR_VOLUME_NOT_SUPPORT_EFS - The disk partition does not support file encryption.
.

MessageId=6015
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_DISABLED
Language=Bulgarian
ERROR_EFS_DISABLED - This machine is disabled for file encryption.
.

MessageId=6016
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_VERSION_NOT_SUPPORT
Language=Bulgarian
ERROR_EFS_VERSION_NOT_SUPPORT - A newer system is required to decrypt this encrypted file.
.

MessageId=6118
Severity=Success
Facility=System
SymbolicName=ERROR_NO_BROWSER_SERVERS_FOUND
Language=Bulgarian
ERROR_NO_BROWSER_SERVERS_FOUND - The list of servers for this workgroup is not currently available.
.

MessageId=6200
Severity=Success
Facility=System
SymbolicName=SCHED_E_SERVICE_NOT_LOCALSYSTEM
Language=Bulgarian
SCHED_E_SERVICE_NOT_LOCALSYSTEM - The Task Scheduler service must be configured to run in the System account to function properly. Individual tasks may be configured to run in other accounts.
.

MessageId=7001
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_NAME_INVALID
Language=Bulgarian
ERROR_CTX_WINSTATION_NAME_INVALID - The specified session name is invalid.
.

MessageId=7002
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_PD
Language=Bulgarian
ERROR_CTX_INVALID_PD - The specified protocol driver is invalid.
.

MessageId=7003
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_PD_NOT_FOUND
Language=Bulgarian
ERROR_CTX_PD_NOT_FOUND - The specified protocol driver was not found in the system path.
.

MessageId=7004
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WD_NOT_FOUND
Language=Bulgarian
ERROR_CTX_WD_NOT_FOUND - The specified terminal connection driver was not found in the system path.
.

MessageId=7005
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY
Language=Bulgarian
ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY - A registry key for event logging could not be created for this session.
.

MessageId=7006
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SERVICE_NAME_COLLISION
Language=Bulgarian
ERROR_CTX_SERVICE_NAME_COLLISION - A service with the same name already exists on the system.
.

MessageId=7007
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLOSE_PENDING
Language=Bulgarian
ERROR_CTX_CLOSE_PENDING - A close operation is pending on the session.
.

MessageId=7008
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NO_OUTBUF
Language=Bulgarian
ERROR_CTX_NO_OUTBUF - There are no free output buffers available.
.

MessageId=7009
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_INF_NOT_FOUND
Language=Bulgarian
ERROR_CTX_MODEM_INF_NOT_FOUND - The MODEM.INF file was not found.
.

MessageId=7010
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_MODEMNAME
Language=Bulgarian
ERROR_CTX_INVALID_MODEMNAME - The modem name was not found in MODEM.INF.
.

MessageId=7011
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_ERROR
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_ERROR - The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem.
.

MessageId=7012
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_TIMEOUT
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_TIMEOUT - The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on.
.

MessageId=7013
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_CARRIER
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_NO_CARRIER - Carrier detect has failed or carrier has been dropped due to disconnect.
.

MessageId=7014
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE - Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional.
.

MessageId=7015
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_BUSY
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_BUSY - Busy signal detected at remote site on callback.
.

MessageId=7016
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_VOICE
Language=Bulgarian
ERROR_CTX_MODEM_RESPONSE_VOICE - Voice detected at remote site on callback.
.

MessageId=7017
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_TD_ERROR
Language=Bulgarian
ERROR_CTX_TD_ERROR - Transport driver error
.

MessageId=7022
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_NOT_FOUND
Language=Bulgarian
ERROR_CTX_WINSTATION_NOT_FOUND - The specified session cannot be found.
.

MessageId=7023
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_ALREADY_EXISTS
Language=Bulgarian
ERROR_CTX_WINSTATION_ALREADY_EXISTS - The specified session name is already in use.
.

MessageId=7024
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_BUSY
Language=Bulgarian
ERROR_CTX_WINSTATION_BUSY - The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation.
.

MessageId=7025
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_BAD_VIDEO_MODE
Language=Bulgarian
ERROR_CTX_BAD_VIDEO_MODE - An attempt has been made to connect to a session whose video mode is not supported by the current client.
.

MessageId=7035
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_GRAPHICS_INVALID
Language=Bulgarian
ERROR_CTX_GRAPHICS_INVALID - The application attempted to enable DOS graphics mode. DOS graphics mode is not supported.
.

MessageId=7037
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LOGON_DISABLED
Language=Bulgarian
ERROR_CTX_LOGON_DISABLED - Your interactive logon privilege has been disabled. Please contact your administrator.
.

MessageId=7038
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NOT_CONSOLE
Language=Bulgarian
ERROR_CTX_NOT_CONSOLE - The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access.
.

MessageId=7040
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_QUERY_TIMEOUT
Language=Bulgarian
ERROR_CTX_CLIENT_QUERY_TIMEOUT - The client failed to respond to the server connect message.
.

MessageId=7041
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CONSOLE_DISCONNECT
Language=Bulgarian
ERROR_CTX_CONSOLE_DISCONNECT - Disconnecting the console session is not supported.
.

MessageId=7042
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CONSOLE_CONNECT
Language=Bulgarian
ERROR_CTX_CONSOLE_CONNECT - Reconnecting a disconnected session to the console is not supported.
.

MessageId=7044
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_DENIED
Language=Bulgarian
ERROR_CTX_SHADOW_DENIED - The request to control another session remotely was denied.
.

MessageId=7045
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_ACCESS_DENIED
Language=Bulgarian
ERROR_CTX_WINSTATION_ACCESS_DENIED - The requested session access is denied.
.

MessageId=7049
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_WD
Language=Bulgarian
ERROR_CTX_INVALID_WD - The specified terminal connection driver is invalid.
.

MessageId=7050
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_INVALID
Language=Bulgarian
ERROR_CTX_SHADOW_INVALID - The requested session cannot be controlled remotely. This may be because the session is disconnected or does not currently have a user logged on.
.

MessageId=7051
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_DISABLED
Language=Bulgarian
ERROR_CTX_SHADOW_DISABLED - The requested session is not configured to allow remote control.
.

MessageId=7052
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_LICENSE_IN_USE
Language=Bulgarian
ERROR_CTX_CLIENT_LICENSE_IN_USE - Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user. Please call your system administrator to obtain a unique license number.
.

MessageId=7053
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_LICENSE_NOT_SET
Language=Bulgarian
ERROR_CTX_CLIENT_LICENSE_NOT_SET - Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client. Please contact your system administrator.
.

MessageId=7054
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_NOT_AVAILABLE
Language=Bulgarian
ERROR_CTX_LICENSE_NOT_AVAILABLE - The system has reached its licensed logon limit. Please try again later.
.

MessageId=7055
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_CLIENT_INVALID
Language=Bulgarian
ERROR_CTX_LICENSE_CLIENT_INVALID - The client you are using is not licensed to use this system. Your logon request is denied.
.

MessageId=7056
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_EXPIRED
Language=Bulgarian
ERROR_CTX_LICENSE_EXPIRED - The system license has expired. Your logon request is denied.
.

MessageId=7057
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_NOT_RUNNING
Language=Bulgarian
ERROR_CTX_SHADOW_NOT_RUNNING - Remote control could not be terminated because the specified session is not currently being remotely controlled.
.

MessageId=7058
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE
Language=Bulgarian
ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE - The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.
.

MessageId=7059
Severity=Success
Facility=System
SymbolicName=ERROR_ACTIVATION_COUNT_EXCEEDED
Language=Bulgarian
ERROR_ACTIVATION_COUNT_EXCEEDED - Activation has already been reset the maximum number of times for this installation. Your activation timer will not be cleared.
.

MessageId=7060
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATIONS_DISABLED
Language=Bulgarian
ERROR_CTX_WINSTATIONS_DISABLED - Remote logins are currently disabled.
.

MessageId=7061
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED
Language=Bulgarian
ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED - You do not have the proper encryption level to access this Session.
.

MessageId=7062
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SESSION_IN_USE
Language=Bulgarian
ERROR_CTX_SESSION_IN_USE - The user %s\\%s is currently logged on to this computer. Only the current user or an administrator can log on to this computer.
.

MessageId=7063
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NO_FORCE_LOGOFF
Language=Bulgarian
ERROR_CTX_NO_FORCE_LOGOFF - The user %s\\%s is already logged on to the console of this computer. You do not have permission to log in at this time. To resolve this issue, contact %s\\%s and have them log off.
.

MessageId=7064
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_ACCOUNT_RESTRICTION
Language=Bulgarian
ERROR_CTX_ACCOUNT_RESTRICTION - Unable to log you on because of an account restriction.
.

MessageId=7065
Severity=Success
Facility=System
SymbolicName=ERROR_RDP_PROTOCOL_ERROR
Language=Bulgarian
ERROR_RDP_PROTOCOL_ERROR - The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client.
.

MessageId=7066
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CDM_CONNECT
Language=Bulgarian
ERROR_CTX_CDM_CONNECT - The Client Drive Mapping Service Has Connected on Terminal Connection.
.

MessageId=7067
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CDM_DISCONNECT
Language=Bulgarian
ERROR_CTX_CDM_DISCONNECT - The Client Drive Mapping Service Has Disconnected on Terminal Connection.
.

MessageId=8001
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INVALID_API_SEQUENCE
Language=Bulgarian
FRS_ERR_INVALID_API_SEQUENCE - The file replication service API was called incorrectly.
.

MessageId=8002
Severity=Success
Facility=System
SymbolicName=FRS_ERR_STARTING_SERVICE
Language=Bulgarian
FRS_ERR_STARTING_SERVICE - The file replication service cannot be started.
.

MessageId=8003
Severity=Success
Facility=System
SymbolicName=FRS_ERR_STOPPING_SERVICE
Language=Bulgarian
FRS_ERR_STOPPING_SERVICE - The file replication service cannot be stopped.
.

MessageId=8004
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INTERNAL_API
Language=Bulgarian
FRS_ERR_INTERNAL_API - The file replication service API terminated the request. The event log may have more information.
.

MessageId=8005
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INTERNAL
Language=Bulgarian
FRS_ERR_INTERNAL - The file replication service terminated the request. The event log may have more information.
.

MessageId=8006
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SERVICE_COMM
Language=Bulgarian
FRS_ERR_SERVICE_COMM - The file replication service cannot be contacted. The event log may have more information.
.

MessageId=8007
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INSUFFICIENT_PRIV
Language=Bulgarian
FRS_ERR_INSUFFICIENT_PRIV - The file replication service cannot satisfy the request because the user has insufficient privileges. The event log may have more information.
.

MessageId=8008
Severity=Success
Facility=System
SymbolicName=FRS_ERR_AUTHENTICATION
Language=Bulgarian
FRS_ERR_AUTHENTICATION - The file replication service cannot satisfy the request because authenticated RPC is not available. The event log may have more information.
.

MessageId=8009
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_INSUFFICIENT_PRIV
Language=Bulgarian
FRS_ERR_PARENT_INSUFFICIENT_PRIV - The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller. The event log may have more information.
.

MessageId=8010
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_AUTHENTICATION
Language=Bulgarian
FRS_ERR_PARENT_AUTHENTICATION - The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller. The event log may have more information.
.

MessageId=8011
Severity=Success
Facility=System
SymbolicName=FRS_ERR_CHILD_TO_PARENT_COMM
Language=Bulgarian
FRS_ERR_CHILD_TO_PARENT_COMM - The file replication service cannot communicate with the file replication service on the domain controller. The event log may have more information.
.

MessageId=8012
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_TO_CHILD_COMM
Language=Bulgarian
FRS_ERR_PARENT_TO_CHILD_COMM - The file replication service on the domain controller cannot communicate with the file replication service on this computer. The event log may have more information.
.

MessageId=8013
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_POPULATE
Language=Bulgarian
FRS_ERR_SYSVOL_POPULATE - The file replication service cannot populate the system volume because of an internal error. The event log may have more information.
.

MessageId=8014
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_POPULATE_TIMEOUT
Language=Bulgarian
FRS_ERR_SYSVOL_POPULATE_TIMEOUT - The file replication service cannot populate the system volume because of an internal timeout. The event log may have more information.
.

MessageId=8015
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_IS_BUSY
Language=Bulgarian
FRS_ERR_SYSVOL_IS_BUSY - The file replication service cannot process the request. The system volume is busy with a previous request.
.

MessageId=8016
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_DEMOTE
Language=Bulgarian
FRS_ERR_SYSVOL_DEMOTE - The file replication service cannot stop replicating the system volume because of an internal error. The event log may have more information.
.

MessageId=8017
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INVALID_SERVICE_PARAMETER
Language=Bulgarian
FRS_ERR_INVALID_SERVICE_PARAMETER - The file replication service detected an invalid parameter.
.

MessageId=8200
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_INSTALLED
Language=Bulgarian
ERROR_DS_NOT_INSTALLED - An error occurred while installing the directory service. For more information, see the event log.
.

MessageId=8201
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY
Language=Bulgarian
ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY - The directory service evaluated group memberships locally.
.

MessageId=8202
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_ATTRIBUTE_OR_VALUE
Language=Bulgarian
ERROR_DS_NO_ATTRIBUTE_OR_VALUE - The specified directory service attribute or value does not exist.
.

MessageId=8203
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_ATTRIBUTE_SYNTAX
Language=Bulgarian
ERROR_DS_INVALID_ATTRIBUTE_SYNTAX - The attribute syntax specified to the directory service is invalid.
.

MessageId=8204
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED
Language=Bulgarian
ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED - The attribute type specified to the directory service is not defined.
.

MessageId=8205
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS
Language=Bulgarian
ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS - The specified directory service attribute or value already exists.
.

MessageId=8206
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BUSY
Language=Bulgarian
ERROR_DS_BUSY - The directory service is busy.
.

MessageId=8207
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNAVAILABLE
Language=Bulgarian
ERROR_DS_UNAVAILABLE - The directory service is unavailable.
.

MessageId=8208
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RIDS_ALLOCATED
Language=Bulgarian
ERROR_DS_NO_RIDS_ALLOCATED - The directory service was unable to allocate a relative identifier.
.

MessageId=8209
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_MORE_RIDS
Language=Bulgarian
ERROR_DS_NO_MORE_RIDS - The directory service has exhausted the pool of relative identifiers.
.

MessageId=8210
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCORRECT_ROLE_OWNER
Language=Bulgarian
ERROR_DS_INCORRECT_ROLE_OWNER - The requested operation could not be performed because the directory service is not the master for that type of operation.
.

MessageId=8211
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RIDMGR_INIT_ERROR
Language=Bulgarian
ERROR_DS_RIDMGR_INIT_ERROR - The directory service was unable to initialize the subsystem that allocates relative identifiers.
.

MessageId=8212
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_VIOLATION
Language=Bulgarian
ERROR_DS_OBJ_CLASS_VIOLATION - The requested operation did not satisfy one or more constraints associated with the class of the object.
.

MessageId=8213
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ON_NON_LEAF
Language=Bulgarian
ERROR_DS_CANT_ON_NON_LEAF - The directory service can perform the requested operation only on a leaf object.
.

MessageId=8214
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ON_RDN
Language=Bulgarian
ERROR_DS_CANT_ON_RDN - The directory service cannot perform the requested operation on the RDN attribute of an object.
.

MessageId=8215
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_OBJ_CLASS
Language=Bulgarian
ERROR_DS_CANT_MOD_OBJ_CLASS - The directory service detected an attempt to modify the object class of an object.
.

MessageId=8216
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_DOM_MOVE_ERROR
Language=Bulgarian
ERROR_DS_CROSS_DOM_MOVE_ERROR - The requested cross-domain move operation could not be performed.
.

MessageId=8217
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GC_NOT_AVAILABLE
Language=Bulgarian
ERROR_DS_GC_NOT_AVAILABLE - Unable to contact the global catalog server.
.

MessageId=8218
Severity=Success
Facility=System
SymbolicName=ERROR_SHARED_POLICY
Language=Bulgarian
ERROR_SHARED_POLICY - The policy object is shared and can only be modified at the root.
.

MessageId=8219
Severity=Success
Facility=System
SymbolicName=ERROR_POLICY_OBJECT_NOT_FOUND
Language=Bulgarian
ERROR_POLICY_OBJECT_NOT_FOUND - The policy object does not exist.
.

MessageId=8220
Severity=Success
Facility=System
SymbolicName=ERROR_POLICY_ONLY_IN_DS
Language=Bulgarian
ERROR_POLICY_ONLY_IN_DS - The requested policy information is only in the directory service.
.

MessageId=8221
Severity=Success
Facility=System
SymbolicName=ERROR_PROMOTION_ACTIVE
Language=Bulgarian
ERROR_PROMOTION_ACTIVE - A domain controller promotion is currently active.
.

MessageId=8222
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PROMOTION_ACTIVE
Language=Bulgarian
ERROR_NO_PROMOTION_ACTIVE - A domain controller promotion is not currently active
.

MessageId=8224
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OPERATIONS_ERROR
Language=Bulgarian
ERROR_DS_OPERATIONS_ERROR - An operations error occurred.
.

MessageId=8225
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PROTOCOL_ERROR
Language=Bulgarian
ERROR_DS_PROTOCOL_ERROR - A protocol error occurred.
.

MessageId=8226
Severity=Success
Facility=System
SymbolicName=ERROR_DS_TIMELIMIT_EXCEEDED
Language=Bulgarian
ERROR_DS_TIMELIMIT_EXCEEDED - The time limit for this request was exceeded.
.

MessageId=8227
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SIZELIMIT_EXCEEDED
Language=Bulgarian
ERROR_DS_SIZELIMIT_EXCEEDED - The size limit for this request was exceeded.
.

MessageId=8228
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ADMIN_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_DS_ADMIN_LIMIT_EXCEEDED - The administrative limit for this request was exceeded.
.

MessageId=8229
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COMPARE_FALSE
Language=Bulgarian
ERROR_DS_COMPARE_FALSE - The compare response was false.
.

MessageId=8230
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COMPARE_TRUE
Language=Bulgarian
ERROR_DS_COMPARE_TRUE - The compare response was true.
.

MessageId=8231
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTH_METHOD_NOT_SUPPORTED
Language=Bulgarian
ERROR_DS_AUTH_METHOD_NOT_SUPPORTED - The requested authentication method is not supported by the server.
.

MessageId=8232
Severity=Success
Facility=System
SymbolicName=ERROR_DS_STRONG_AUTH_REQUIRED
Language=Bulgarian
ERROR_DS_STRONG_AUTH_REQUIRED - A more secure authentication method is required for this server.
.

MessageId=8233
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INAPPROPRIATE_AUTH
Language=Bulgarian
ERROR_DS_INAPPROPRIATE_AUTH - Inappropriate authentication.
.

MessageId=8234
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTH_UNKNOWN
Language=Bulgarian
ERROR_DS_AUTH_UNKNOWN - The authentication mechanism is unknown.
.

MessageId=8235
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFERRAL
Language=Bulgarian
ERROR_DS_REFERRAL - A referral was returned from the server.
.

MessageId=8236
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNAVAILABLE_CRIT_EXTENSION
Language=Bulgarian
ERROR_DS_UNAVAILABLE_CRIT_EXTENSION - The server does not support the requested critical extension.
.

MessageId=8237
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONFIDENTIALITY_REQUIRED
Language=Bulgarian
ERROR_DS_CONFIDENTIALITY_REQUIRED - This request requires a secure connection.
.

MessageId=8238
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INAPPROPRIATE_MATCHING
Language=Bulgarian
ERROR_DS_INAPPROPRIATE_MATCHING - Inappropriate matching.
.

MessageId=8239
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONSTRAINT_VIOLATION
Language=Bulgarian
ERROR_DS_CONSTRAINT_VIOLATION - A constraint violation occurred.
.

MessageId=8240
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_SUCH_OBJECT
Language=Bulgarian
ERROR_DS_NO_SUCH_OBJECT - There is no such object on the server.
.

MessageId=8241
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_PROBLEM
Language=Bulgarian
ERROR_DS_ALIAS_PROBLEM - There is an alias problem.
.

MessageId=8242
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_DN_SYNTAX
Language=Bulgarian
ERROR_DS_INVALID_DN_SYNTAX - An invalid dn syntax has been specified.
.

MessageId=8243
Severity=Success
Facility=System
SymbolicName=ERROR_DS_IS_LEAF
Language=Bulgarian
ERROR_DS_IS_LEAF - The object is a leaf object.
.

MessageId=8244
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_DEREF_PROBLEM
Language=Bulgarian
ERROR_DS_ALIAS_DEREF_PROBLEM - There is an alias dereferencing problem.
.

MessageId=8245
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNWILLING_TO_PERFORM
Language=Bulgarian
ERROR_DS_UNWILLING_TO_PERFORM - The server is unwilling to process the request.
.

MessageId=8246
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOOP_DETECT
Language=Bulgarian
ERROR_DS_LOOP_DETECT - A loop has been detected.
.

MessageId=8247
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAMING_VIOLATION
Language=Bulgarian
ERROR_DS_NAMING_VIOLATION - There is a naming violation.
.

MessageId=8248
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_RESULTS_TOO_LARGE
Language=Bulgarian
ERROR_DS_OBJECT_RESULTS_TOO_LARGE - The result set is too large.
.

MessageId=8249
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AFFECTS_MULTIPLE_DSAS
Language=Bulgarian
ERROR_DS_AFFECTS_MULTIPLE_DSAS - The operation affects multiple DSAs
.

MessageId=8250
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SERVER_DOWN
Language=Bulgarian
ERROR_DS_SERVER_DOWN - The server is not operational.
.

MessageId=8251
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_ERROR
Language=Bulgarian
ERROR_DS_LOCAL_ERROR - A local error has occurred.
.

MessageId=8252
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ENCODING_ERROR
Language=Bulgarian
ERROR_DS_ENCODING_ERROR - An encoding error has occurred.
.

MessageId=8253
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DECODING_ERROR
Language=Bulgarian
ERROR_DS_DECODING_ERROR - A decoding error has occurred.
.

MessageId=8254
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FILTER_UNKNOWN
Language=Bulgarian
ERROR_DS_FILTER_UNKNOWN - The search filter cannot be recognized.
.

MessageId=8255
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PARAM_ERROR
Language=Bulgarian
ERROR_DS_PARAM_ERROR - One or more parameters are illegal.
.

MessageId=8256
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_SUPPORTED
Language=Bulgarian
ERROR_DS_NOT_SUPPORTED - The specified method is not supported.
.

MessageId=8257
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RESULTS_RETURNED
Language=Bulgarian
ERROR_DS_NO_RESULTS_RETURNED - No results were returned.
.

MessageId=8258
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONTROL_NOT_FOUND
Language=Bulgarian
ERROR_DS_CONTROL_NOT_FOUND - The specified control is not supported by the server.
.

MessageId=8259
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLIENT_LOOP
Language=Bulgarian
ERROR_DS_CLIENT_LOOP - A referral loop was detected by the client.
.

MessageId=8260
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFERRAL_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_DS_REFERRAL_LIMIT_EXCEEDED - The preset referral limit was exceeded.
.

MessageId=8261
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SORT_CONTROL_MISSING
Language=Bulgarian
ERROR_DS_SORT_CONTROL_MISSING - The search requires a SORT control.
.

MessageId=8262
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OFFSET_RANGE_ERROR
Language=Bulgarian
ERROR_DS_OFFSET_RANGE_ERROR - The search results exceed the offset range specified.
.

MessageId=8301
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_MUST_BE_NC
Language=Bulgarian
ERROR_DS_ROOT_MUST_BE_NC - The root object must be the head of a naming context. The root object cannot have an instantiated parent.
.

MessageId=8302
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ADD_REPLICA_INHIBITED
Language=Bulgarian
ERROR_DS_ADD_REPLICA_INHIBITED - The add replica operation cannot be performed. The naming context must be writeable in order to create the replica.
.

MessageId=8303
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_NOT_DEF_IN_SCHEMA
Language=Bulgarian
ERROR_DS_ATT_NOT_DEF_IN_SCHEMA - A reference to an attribute that is not defined in the schema occurred.
.

MessageId=8304
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MAX_OBJ_SIZE_EXCEEDED
Language=Bulgarian
ERROR_DS_MAX_OBJ_SIZE_EXCEEDED - The maximum size of an object has been exceeded.
.

MessageId=8305
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_STRING_NAME_EXISTS
Language=Bulgarian
ERROR_DS_OBJ_STRING_NAME_EXISTS - An attempt was made to add an object to the directory with a name that is already in use.
.

MessageId=8306
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA
Language=Bulgarian
ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA - An attempt was made to add an object of a class that does not have an RDN defined in the schema.
.

MessageId=8307
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RDN_DOESNT_MATCH_SCHEMA
Language=Bulgarian
ERROR_DS_RDN_DOESNT_MATCH_SCHEMA - An attempt was made to add an object using an RDN that is not the RDN defined in the schema.
.

MessageId=8308
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_REQUESTED_ATTS_FOUND
Language=Bulgarian
ERROR_DS_NO_REQUESTED_ATTS_FOUND - None of the requested attributes were found on the objects.
.

MessageId=8309
Severity=Success
Facility=System
SymbolicName=ERROR_DS_USER_BUFFER_TO_SMALL
Language=Bulgarian
ERROR_DS_USER_BUFFER_TO_SMALL - The user buffer is too small.
.

MessageId=8310
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_IS_NOT_ON_OBJ
Language=Bulgarian
ERROR_DS_ATT_IS_NOT_ON_OBJ - The attribute specified in the operation is not present on the object.
.

MessageId=8311
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_MOD_OPERATION
Language=Bulgarian
ERROR_DS_ILLEGAL_MOD_OPERATION - Illegal modify operation. Some aspect of the modification is not permitted.
.

MessageId=8312
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_TOO_LARGE
Language=Bulgarian
ERROR_DS_OBJ_TOO_LARGE - The specified object is too large.
.

MessageId=8313
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_INSTANCE_TYPE
Language=Bulgarian
ERROR_DS_BAD_INSTANCE_TYPE - The specified instance type is not valid.
.

MessageId=8314
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MASTERDSA_REQUIRED
Language=Bulgarian
ERROR_DS_MASTERDSA_REQUIRED - The operation must be performed at a master DSA.
.

MessageId=8315
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_CLASS_REQUIRED
Language=Bulgarian
ERROR_DS_OBJECT_CLASS_REQUIRED - The object class attribute must be specified.
.

MessageId=8316
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_REQUIRED_ATT
Language=Bulgarian
ERROR_DS_MISSING_REQUIRED_ATT - A required attribute is missing.
.

MessageId=8317
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_NOT_DEF_FOR_CLASS
Language=Bulgarian
ERROR_DS_ATT_NOT_DEF_FOR_CLASS - An attempt was made to modify an object to include an attribute that is not legal for its class
.

MessageId=8318
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_ALREADY_EXISTS
Language=Bulgarian
ERROR_DS_ATT_ALREADY_EXISTS - The specified attribute is already present on the object.
.

MessageId=8320
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_ATT_VALUES
Language=Bulgarian
ERROR_DS_CANT_ADD_ATT_VALUES - The specified attribute is not present, or has no values.
.

MessageId=8321
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SINGLE_VALUE_CONSTRAINT
Language=Bulgarian
ERROR_DS_SINGLE_VALUE_CONSTRAINT - Multiple values were specified for an attribute that can have only one value.
.

MessageId=8322
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RANGE_CONSTRAINT
Language=Bulgarian
ERROR_DS_RANGE_CONSTRAINT - A value for the attribute was not in the acceptable range of values.
.

MessageId=8323
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_VAL_ALREADY_EXISTS
Language=Bulgarian
ERROR_DS_ATT_VAL_ALREADY_EXISTS - The specified value already exists.
.

MessageId=8324
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT
Language=Bulgarian
ERROR_DS_CANT_REM_MISSING_ATT - The attribute cannot be removed because it is not present on the object.
.

MessageId=8325
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT_VAL
Language=Bulgarian
ERROR_DS_CANT_REM_MISSING_ATT_VAL - The attribute value cannot be removed because it is not present on the object.
.

MessageId=8326
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_CANT_BE_SUBREF
Language=Bulgarian
ERROR_DS_ROOT_CANT_BE_SUBREF - The specified root object cannot be a subref.
.

MessageId=8327
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHAINING
Language=Bulgarian
ERROR_DS_NO_CHAINING - Chaining is not permitted.
.

MessageId=8328
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHAINED_EVAL
Language=Bulgarian
ERROR_DS_NO_CHAINED_EVAL - Chained evaluation is not permitted.
.

MessageId=8329
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_PARENT_OBJECT
Language=Bulgarian
ERROR_DS_NO_PARENT_OBJECT - The operation could not be performed because the object's parent is either uninstantiated or deleted.
.

MessageId=8330
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PARENT_IS_AN_ALIAS
Language=Bulgarian
ERROR_DS_PARENT_IS_AN_ALIAS - Having a parent that is an alias is not permitted. Aliases are leaf objects.
.

MessageId=8331
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MIX_MASTER_AND_REPS
Language=Bulgarian
ERROR_DS_CANT_MIX_MASTER_AND_REPS - The object and parent must be of the same type, either both masters or both replicas.
.

MessageId=8332
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CHILDREN_EXIST
Language=Bulgarian
ERROR_DS_CHILDREN_EXIST - The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object.
.

MessageId=8333
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_NOT_FOUND
Language=Bulgarian
ERROR_DS_OBJ_NOT_FOUND - Directory object not found.
.

MessageId=8334
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIASED_OBJ_MISSING
Language=Bulgarian
ERROR_DS_ALIASED_OBJ_MISSING - The aliased object is missing.
.

MessageId=8335
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_NAME_SYNTAX
Language=Bulgarian
ERROR_DS_BAD_NAME_SYNTAX - The object name has bad syntax.
.

MessageId=8336
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_POINTS_TO_ALIAS
Language=Bulgarian
ERROR_DS_ALIAS_POINTS_TO_ALIAS - It is not permitted for an alias to refer to another alias.
.

MessageId=8337
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEREF_ALIAS
Language=Bulgarian
ERROR_DS_CANT_DEREF_ALIAS - The alias cannot be dereferenced.
.

MessageId=8338
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OUT_OF_SCOPE
Language=Bulgarian
ERROR_DS_OUT_OF_SCOPE - The operation is out of scope.
.

MessageId=8339
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_BEING_REMOVED
Language=Bulgarian
ERROR_DS_OBJECT_BEING_REMOVED - The operation cannot continue because the object is in the process of being removed.
.

MessageId=8340
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DELETE_DSA_OBJ
Language=Bulgarian
ERROR_DS_CANT_DELETE_DSA_OBJ - The DSA object cannot be deleted.
.

MessageId=8341
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GENERIC_ERROR
Language=Bulgarian
ERROR_DS_GENERIC_ERROR - A directory service error has occurred.
.

MessageId=8342
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DSA_MUST_BE_INT_MASTER
Language=Bulgarian
ERROR_DS_DSA_MUST_BE_INT_MASTER - The operation can only be performed on an internal master DSA object.
.

MessageId=8343
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLASS_NOT_DSA
Language=Bulgarian
ERROR_DS_CLASS_NOT_DSA - The object must be of class DSA.
.

MessageId=8344
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSUFF_ACCESS_RIGHTS
Language=Bulgarian
ERROR_DS_INSUFF_ACCESS_RIGHTS - Insufficient access rights to perform the operation.
.

MessageId=8345
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_SUPERIOR
Language=Bulgarian
ERROR_DS_ILLEGAL_SUPERIOR - The object cannot be added because the parent is not on the list of possible superiors.
.

MessageId=8346
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_OWNED_BY_SAM
Language=Bulgarian
ERROR_DS_ATTRIBUTE_OWNED_BY_SAM - Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM).
.

MessageId=8347
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TOO_MANY_PARTS
Language=Bulgarian
ERROR_DS_NAME_TOO_MANY_PARTS - The name has too many parts.
.

MessageId=8348
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TOO_LONG
Language=Bulgarian
ERROR_DS_NAME_TOO_LONG - The name is too long.
.

MessageId=8349
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_VALUE_TOO_LONG
Language=Bulgarian
ERROR_DS_NAME_VALUE_TOO_LONG - The name value is too long.
.

MessageId=8350
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_UNPARSEABLE
Language=Bulgarian
ERROR_DS_NAME_UNPARSEABLE - The directory service encountered an error parsing a name.
.

MessageId=8351
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TYPE_UNKNOWN
Language=Bulgarian
ERROR_DS_NAME_TYPE_UNKNOWN - The directory service cannot get the attribute type for a name.
.

MessageId=8352
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_AN_OBJECT
Language=Bulgarian
ERROR_DS_NOT_AN_OBJECT - The name does not identify an object; the name identifies a phantom.
.

MessageId=8353
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEC_DESC_TOO_SHORT
Language=Bulgarian
ERROR_DS_SEC_DESC_TOO_SHORT - The security descriptor is too short.
.

MessageId=8354
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEC_DESC_INVALID
Language=Bulgarian
ERROR_DS_SEC_DESC_INVALID - The security descriptor is invalid.
.

MessageId=8355
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_DELETED_NAME
Language=Bulgarian
ERROR_DS_NO_DELETED_NAME - Failed to create name for deleted object.
.

MessageId=8356
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUBREF_MUST_HAVE_PARENT
Language=Bulgarian
ERROR_DS_SUBREF_MUST_HAVE_PARENT - The parent of a new subref must exist.
.

MessageId=8357
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NCNAME_MUST_BE_NC
Language=Bulgarian
ERROR_DS_NCNAME_MUST_BE_NC - The object must be a naming context.
.

MessageId=8358
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_SYSTEM_ONLY
Language=Bulgarian
ERROR_DS_CANT_ADD_SYSTEM_ONLY - It is not permitted to add an attribute which is owned by the system.
.

MessageId=8359
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLASS_MUST_BE_CONCRETE
Language=Bulgarian
ERROR_DS_CLASS_MUST_BE_CONCRETE - The class of the object must be structural; you cannot instantiate an abstract class.
.

MessageId=8360
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_DMD
Language=Bulgarian
ERROR_DS_INVALID_DMD - The schema object could not be found.
.

MessageId=8361
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_GUID_EXISTS
Language=Bulgarian
ERROR_DS_OBJ_GUID_EXISTS - A local object with this GUID (dead or alive) already exists.
.

MessageId=8362
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_ON_BACKLINK
Language=Bulgarian
ERROR_DS_NOT_ON_BACKLINK - The operation cannot be performed on a back link.
.

MessageId=8363
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CROSSREF_FOR_NC
Language=Bulgarian
ERROR_DS_NO_CROSSREF_FOR_NC - The cross reference for the specified naming context could not be found.
.

MessageId=8364
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SHUTTING_DOWN
Language=Bulgarian
ERROR_DS_SHUTTING_DOWN - The operation could not be performed because the directory service is shutting down.
.

MessageId=8365
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNKNOWN_OPERATION
Language=Bulgarian
ERROR_DS_UNKNOWN_OPERATION - The directory service request is invalid.
.

MessageId=8366
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_ROLE_OWNER
Language=Bulgarian
ERROR_DS_INVALID_ROLE_OWNER - The role owner attribute could not be read.
.

MessageId=8367
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_CONTACT_FSMO
Language=Bulgarian
ERROR_DS_COULDNT_CONTACT_FSMO - The requested FSMO operation failed. The current FSMO holder could not be reached.
.

MessageId=8368
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_NC_DN_RENAME
Language=Bulgarian
ERROR_DS_CROSS_NC_DN_RENAME - Modification of a DN across a naming context is not permitted.
.

MessageId=8369
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_SYSTEM_ONLY
Language=Bulgarian
ERROR_DS_CANT_MOD_SYSTEM_ONLY - The attribute cannot be modified because it is owned by the system.
.

MessageId=8370
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPLICATOR_ONLY
Language=Bulgarian
ERROR_DS_REPLICATOR_ONLY - Only the replicator can perform this function.
.

MessageId=8371
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_NOT_DEFINED
Language=Bulgarian
ERROR_DS_OBJ_CLASS_NOT_DEFINED - The specified class is not defined.
.

MessageId=8372
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_NOT_SUBCLASS
Language=Bulgarian
ERROR_DS_OBJ_CLASS_NOT_SUBCLASS - The specified class is not a subclass.
.

MessageId=8373
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_REFERENCE_INVALID
Language=Bulgarian
ERROR_DS_NAME_REFERENCE_INVALID - The name reference is invalid.
.

MessageId=8374
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_REF_EXISTS
Language=Bulgarian
ERROR_DS_CROSS_REF_EXISTS - A cross reference already exists.
.

MessageId=8375
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEL_MASTER_CROSSREF
Language=Bulgarian
ERROR_DS_CANT_DEL_MASTER_CROSSREF - It is not permitted to delete a master cross reference.
.

MessageId=8376
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD
Language=Bulgarian
ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD - Subtree notifications are only supported on NC heads.
.

MessageId=8377
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX
Language=Bulgarian
ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX - Notification filter is too complex.
.

MessageId=8378
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_RDN
Language=Bulgarian
ERROR_DS_DUP_RDN - Schema update failed: duplicate RDN.
.

MessageId=8379
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_OID
Language=Bulgarian
ERROR_DS_DUP_OID - Schema update failed: duplicate OID
.

MessageId=8380
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_MAPI_ID
Language=Bulgarian
ERROR_DS_DUP_MAPI_ID - Schema update failed: duplicate MAPI identifier.
.

MessageId=8381
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_SCHEMA_ID_GUID
Language=Bulgarian
ERROR_DS_DUP_SCHEMA_ID_GUID - Schema update failed: duplicate schema-id GUID.
.

MessageId=8382
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_LDAP_DISPLAY_NAME
Language=Bulgarian
ERROR_DS_DUP_LDAP_DISPLAY_NAME - Schema update failed: duplicate LDAP display name.
.

MessageId=8383
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEMANTIC_ATT_TEST
Language=Bulgarian
ERROR_DS_SEMANTIC_ATT_TEST - Schema update failed: range-lower less than range upper
.

MessageId=8384
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SYNTAX_MISMATCH
Language=Bulgarian
ERROR_DS_SYNTAX_MISMATCH - Schema update failed: syntax mismatch
.

MessageId=8385
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_MUST_HAVE
Language=Bulgarian
ERROR_DS_EXISTS_IN_MUST_HAVE - Schema deletion failed: attribute is used in must-contain
.

MessageId=8386
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_MAY_HAVE
Language=Bulgarian
ERROR_DS_EXISTS_IN_MAY_HAVE - Schema deletion failed: attribute is used in may-contain
.

MessageId=8387
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_MAY_HAVE
Language=Bulgarian
ERROR_DS_NONEXISTENT_MAY_HAVE - Schema update failed: attribute in may-contain does not exist
.

MessageId=8388
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_MUST_HAVE
Language=Bulgarian
ERROR_DS_NONEXISTENT_MUST_HAVE - Schema update failed: attribute in must-contain does not exist
.

MessageId=8389
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUX_CLS_TEST_FAIL
Language=Bulgarian
ERROR_DS_AUX_CLS_TEST_FAIL - Schema update failed: class in aux-class list does not exist or is not an auxiliary class
.

MessageId=8390
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_POSS_SUP
Language=Bulgarian
ERROR_DS_NONEXISTENT_POSS_SUP - Schema update failed: class in poss-superiors does not exist
.

MessageId=8391
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUB_CLS_TEST_FAIL
Language=Bulgarian
ERROR_DS_SUB_CLS_TEST_FAIL - Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules
.

MessageId=8392
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_RDN_ATT_ID_SYNTAX
Language=Bulgarian
ERROR_DS_BAD_RDN_ATT_ID_SYNTAX - Schema update failed: Rdn-Att-Id has wrong syntax
.

MessageId=8393
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_AUX_CLS
Language=Bulgarian
ERROR_DS_EXISTS_IN_AUX_CLS - Schema deletion failed: class is used as auxiliary class
.

MessageId=8394
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_SUB_CLS
Language=Bulgarian
ERROR_DS_EXISTS_IN_SUB_CLS - Schema deletion failed: class is used as sub class
.

MessageId=8395
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_POSS_SUP
Language=Bulgarian
ERROR_DS_EXISTS_IN_POSS_SUP - Schema deletion failed: class is used as poss superior
.

MessageId=8396
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RECALCSCHEMA_FAILED
Language=Bulgarian
ERROR_DS_RECALCSCHEMA_FAILED - Schema update failed in recalculating validation cache.
.

MessageId=8397
Severity=Success
Facility=System
SymbolicName=ERROR_DS_TREE_DELETE_NOT_FINISHED
Language=Bulgarian
ERROR_DS_TREE_DELETE_NOT_FINISHED - The tree deletion is not finished.
.

MessageId=8398
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DELETE
Language=Bulgarian
ERROR_DS_CANT_DELETE - The requested delete operation could not be performed.
.

MessageId=8399
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_ID
Language=Bulgarian
ERROR_DS_ATT_SCHEMA_REQ_ID - Cannot read the governs class identifier for the schema record.
.

MessageId=8400
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_ATT_SCHEMA_SYNTAX
Language=Bulgarian
ERROR_DS_BAD_ATT_SCHEMA_SYNTAX - The attribute schema has bad syntax.
.

MessageId=8401
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CACHE_ATT
Language=Bulgarian
ERROR_DS_CANT_CACHE_ATT - The attribute could not be cached.
.

MessageId=8402
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CACHE_CLASS
Language=Bulgarian
ERROR_DS_CANT_CACHE_CLASS - The class could not be cached.
.

MessageId=8403
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REMOVE_ATT_CACHE
Language=Bulgarian
ERROR_DS_CANT_REMOVE_ATT_CACHE - The attribute could not be removed from the cache.
.

MessageId=8404
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REMOVE_CLASS_CACHE
Language=Bulgarian
ERROR_DS_CANT_REMOVE_CLASS_CACHE - The class could not be removed from the cache.
.

MessageId=8405
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_DN
Language=Bulgarian
ERROR_DS_CANT_RETRIEVE_DN - The distinguished name attribute could not be read.
.

MessageId=8406
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_SUPREF
Language=Bulgarian
ERROR_DS_MISSING_SUPREF - No superior reference has been configured for the directory service. The directory service is therefore unable to issue referrals to objects outside this forest.
.

MessageId=8407
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_INSTANCE
Language=Bulgarian
ERROR_DS_CANT_RETRIEVE_INSTANCE - The instance type attribute could not be retrieved.
.

MessageId=8408
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CODE_INCONSISTENCY
Language=Bulgarian
ERROR_DS_CODE_INCONSISTENCY - An internal error has occurred.
.

MessageId=8409
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DATABASE_ERROR
Language=Bulgarian
ERROR_DS_DATABASE_ERROR - A database error has occurred.
.

MessageId=8410
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GOVERNSID_MISSING
Language=Bulgarian
ERROR_DS_GOVERNSID_MISSING - The attribute GOVERNSID is missing.
.

MessageId=8411
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_EXPECTED_ATT
Language=Bulgarian
ERROR_DS_MISSING_EXPECTED_ATT - An expected attribute is missing.
.

MessageId=8412
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NCNAME_MISSING_CR_REF
Language=Bulgarian
ERROR_DS_NCNAME_MISSING_CR_REF - The specified naming context is missing a cross reference.
.

MessageId=8413
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SECURITY_CHECKING_ERROR
Language=Bulgarian
ERROR_DS_SECURITY_CHECKING_ERROR - A security checking error has occurred.
.

MessageId=8414
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_NOT_LOADED
Language=Bulgarian
ERROR_DS_SCHEMA_NOT_LOADED - The schema is not loaded.
.

MessageId=8415
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_ALLOC_FAILED
Language=Bulgarian
ERROR_DS_SCHEMA_ALLOC_FAILED - Schema allocation failed. Please check if the machine is running low on memory.
.

MessageId=8416
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_SYNTAX
Language=Bulgarian
ERROR_DS_ATT_SCHEMA_REQ_SYNTAX - Failed to obtain the required syntax for the attribute schema.
.

MessageId=8417
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GCVERIFY_ERROR
Language=Bulgarian
ERROR_DS_GCVERIFY_ERROR - The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available.
.

MessageId=8418
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_MISMATCH
Language=Bulgarian
ERROR_DS_DRA_SCHEMA_MISMATCH - The replication operation failed because of a schema mismatch between the servers involved.
.

MessageId=8419
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_DSA_OBJ
Language=Bulgarian
ERROR_DS_CANT_FIND_DSA_OBJ - The DSA object could not be found.
.

MessageId=8420
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_EXPECTED_NC
Language=Bulgarian
ERROR_DS_CANT_FIND_EXPECTED_NC - The naming context could not be found.
.

MessageId=8421
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_NC_IN_CACHE
Language=Bulgarian
ERROR_DS_CANT_FIND_NC_IN_CACHE - The naming context could not be found in the cache.
.

MessageId=8422
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_CHILD
Language=Bulgarian
ERROR_DS_CANT_RETRIEVE_CHILD - The child object could not be retrieved.
.

MessageId=8423
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SECURITY_ILLEGAL_MODIFY
Language=Bulgarian
ERROR_DS_SECURITY_ILLEGAL_MODIFY - The modification was not permitted for security reasons.
.

MessageId=8424
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REPLACE_HIDDEN_REC
Language=Bulgarian
ERROR_DS_CANT_REPLACE_HIDDEN_REC - The operation cannot replace the hidden record.
.

MessageId=8425
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_HIERARCHY_FILE
Language=Bulgarian
ERROR_DS_BAD_HIERARCHY_FILE - The hierarchy file is invalid.
.

MessageId=8426
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED
Language=Bulgarian
ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED - The attempt to build the hierarchy table failed.
.

MessageId=8427
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONFIG_PARAM_MISSING
Language=Bulgarian
ERROR_DS_CONFIG_PARAM_MISSING - The directory configuration parameter is missing from the registry.
.

MessageId=8428
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COUNTING_AB_INDICES_FAILED
Language=Bulgarian
ERROR_DS_COUNTING_AB_INDICES_FAILED - The attempt to count the address book indices failed.
.

MessageId=8429
Severity=Success
Facility=System
SymbolicName=ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED
Language=Bulgarian
ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED - The allocation of the hierarchy table failed.
.

MessageId=8430
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INTERNAL_FAILURE
Language=Bulgarian
ERROR_DS_INTERNAL_FAILURE - The directory service encountered an internal failure.
.

MessageId=8431
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNKNOWN_ERROR
Language=Bulgarian
ERROR_DS_UNKNOWN_ERROR - The directory service encountered an unknown failure.
.

MessageId=8432
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_REQUIRES_CLASS_TOP
Language=Bulgarian
ERROR_DS_ROOT_REQUIRES_CLASS_TOP - A root object requires a class of 'top'.
.

MessageId=8433
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFUSING_FSMO_ROLES
Language=Bulgarian
ERROR_DS_REFUSING_FSMO_ROLES - This directory server is shutting down, and cannot take ownership of new floating single-master operation roles.
.

MessageId=8434
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_FSMO_SETTINGS
Language=Bulgarian
ERROR_DS_MISSING_FSMO_SETTINGS - The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles.
.

MessageId=8435
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNABLE_TO_SURRENDER_ROLES
Language=Bulgarian
ERROR_DS_UNABLE_TO_SURRENDER_ROLES - The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers.
.

MessageId=8436
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_GENERIC
Language=Bulgarian
ERROR_DS_DRA_GENERIC - The replication operation failed.
.

MessageId=8437
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INVALID_PARAMETER
Language=Bulgarian
ERROR_DS_DRA_INVALID_PARAMETER - An invalid parameter was specified for this replication operation.
.

MessageId=8438
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BUSY
Language=Bulgarian
ERROR_DS_DRA_BUSY - The directory service is too busy to complete the replication operation at this time.
.

MessageId=8439
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_DN
Language=Bulgarian
ERROR_DS_DRA_BAD_DN - The distinguished name specified for this replication operation is invalid.
.

MessageId=8440
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_NC
Language=Bulgarian
ERROR_DS_DRA_BAD_NC - The naming context specified for this replication operation is invalid.
.

MessageId=8441
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_DN_EXISTS
Language=Bulgarian
ERROR_DS_DRA_DN_EXISTS - The distinguished name specified for this replication operation already exists.
.

MessageId=8442
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INTERNAL_ERROR
Language=Bulgarian
ERROR_DS_DRA_INTERNAL_ERROR - The replication system encountered an internal error.
.

MessageId=8443
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INCONSISTENT_DIT
Language=Bulgarian
ERROR_DS_DRA_INCONSISTENT_DIT - The replication operation encountered a database inconsistency.
.

MessageId=8444
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_CONNECTION_FAILED
Language=Bulgarian
ERROR_DS_DRA_CONNECTION_FAILED - The server specified for this replication operation could not be contacted.
.

MessageId=8445
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_INSTANCE_TYPE
Language=Bulgarian
ERROR_DS_DRA_BAD_INSTANCE_TYPE - The replication operation encountered an object with an invalid instance type.
.

MessageId=8446
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OUT_OF_MEM
Language=Bulgarian
ERROR_DS_DRA_OUT_OF_MEM - The replication operation failed to allocate memory.
.

MessageId=8447
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_MAIL_PROBLEM
Language=Bulgarian
ERROR_DS_DRA_MAIL_PROBLEM - The replication operation encountered an error with the mail system.
.

MessageId=8448
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REF_ALREADY_EXISTS
Language=Bulgarian
ERROR_DS_DRA_REF_ALREADY_EXISTS - The replication reference information for the target server already exists.
.

MessageId=8449
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REF_NOT_FOUND
Language=Bulgarian
ERROR_DS_DRA_REF_NOT_FOUND - The replication reference information for the target server does not exist.
.

MessageId=8450
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OBJ_IS_REP_SOURCE
Language=Bulgarian
ERROR_DS_DRA_OBJ_IS_REP_SOURCE - The naming context cannot be removed because it is replicated to another server.
.

MessageId=8451
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_DB_ERROR
Language=Bulgarian
ERROR_DS_DRA_DB_ERROR - The replication operation encountered a database error.
.

MessageId=8452
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NO_REPLICA
Language=Bulgarian
ERROR_DS_DRA_NO_REPLICA - The naming context is in the process of being removed or is not replicated from the specified server.
.

MessageId=8453
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_ACCESS_DENIED
Language=Bulgarian
ERROR_DS_DRA_ACCESS_DENIED - Replication access was denied.
.

MessageId=8454
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NOT_SUPPORTED
Language=Bulgarian
ERROR_DS_DRA_NOT_SUPPORTED - The requested operation is not supported by this version of the directory service.
.

MessageId=8455
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_RPC_CANCELLED
Language=Bulgarian
ERROR_DS_DRA_RPC_CANCELLED - The replication remote procedure call was cancelled.
.

MessageId=8456
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_DISABLED
Language=Bulgarian
ERROR_DS_DRA_SOURCE_DISABLED - The source server is currently rejecting replication requests.
.

MessageId=8457
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SINK_DISABLED
Language=Bulgarian
ERROR_DS_DRA_SINK_DISABLED - The destination server is currently rejecting replication requests.
.

MessageId=8458
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NAME_COLLISION
Language=Bulgarian
ERROR_DS_DRA_NAME_COLLISION - The replication operation failed due to a collision of object names.
.

MessageId=8459
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_REINSTALLED
Language=Bulgarian
ERROR_DS_DRA_SOURCE_REINSTALLED - The replication source has been reinstalled.
.

MessageId=8460
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_MISSING_PARENT
Language=Bulgarian
ERROR_DS_DRA_MISSING_PARENT - The replication operation failed because a required parent object is missing.
.

MessageId=8461
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_PREEMPTED
Language=Bulgarian
ERROR_DS_DRA_PREEMPTED - The replication operation was preempted.
.

MessageId=8462
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_ABANDON_SYNC
Language=Bulgarian
ERROR_DS_DRA_ABANDON_SYNC - The replication synchronization attempt was abandoned because of a lack of updates.
.

MessageId=8463
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SHUTDOWN
Language=Bulgarian
ERROR_DS_DRA_SHUTDOWN - The replication operation was terminated because the system is shutting down.
.

MessageId=8464
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET
Language=Bulgarian
ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET - Synchronization attempt failed because the destination DC is currently waiting to synchronize new partial attributes from source. This condition is normal if a recent schema change modified the partial attribute set. The destination partial attribute set is not a subset of the source partial attribute set.
.

MessageId=8465
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA
Language=Bulgarian
ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA - The replication synchronization attempt failed because a master replica attempted to sync from a partial replica.
.

MessageId=8466
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_EXTN_CONNECTION_FAILED
Language=Bulgarian
ERROR_DS_DRA_EXTN_CONNECTION_FAILED - The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation.
.

MessageId=8467
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_SCHEMA_MISMATCH
Language=Bulgarian
ERROR_DS_INSTALL_SCHEMA_MISMATCH - The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer.
.

MessageId=8468
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_LINK_ID
Language=Bulgarian
ERROR_DS_DUP_LINK_ID - Schema update failed: An attribute with the same link identifier already exists.
.

MessageId=8469
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_RESOLVING
Language=Bulgarian
ERROR_DS_NAME_ERROR_RESOLVING - Name translation: Generic processing error.
.

MessageId=8470
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NOT_FOUND
Language=Bulgarian
ERROR_DS_NAME_ERROR_NOT_FOUND - Name translation: Could not find the name or insufficient right to see name.
.

MessageId=8471
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NOT_UNIQUE
Language=Bulgarian
ERROR_DS_NAME_ERROR_NOT_UNIQUE - Name translation: Input name mapped to more than one output name.
.

MessageId=8472
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NO_MAPPING
Language=Bulgarian
ERROR_DS_NAME_ERROR_NO_MAPPING - Name translation: Input name found, but not the associated output format.
.

MessageId=8473
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_DOMAIN_ONLY
Language=Bulgarian
ERROR_DS_NAME_ERROR_DOMAIN_ONLY - Name translation: Unable to resolve completely, only the domain was found.
.

MessageId=8474
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
Language=Bulgarian
ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING - Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire.
.

MessageId=8475
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONSTRUCTED_ATT_MOD
Language=Bulgarian
ERROR_DS_CONSTRUCTED_ATT_MOD - Modification of a constructed attribute is not allowed.
.

MessageId=8476
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WRONG_OM_OBJ_CLASS
Language=Bulgarian
ERROR_DS_WRONG_OM_OBJ_CLASS - The OM-Object-Class specified is incorrect for an attribute with the specified syntax.
.

MessageId=8477
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REPL_PENDING
Language=Bulgarian
ERROR_DS_DRA_REPL_PENDING - The replication request has been posted; waiting for reply.
.

MessageId=8478
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DS_REQUIRED
Language=Bulgarian
ERROR_DS_DS_REQUIRED - The requested operation requires a directory service, and none was available.
.

MessageId=8479
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_LDAP_DISPLAY_NAME
Language=Bulgarian
ERROR_DS_INVALID_LDAP_DISPLAY_NAME - The LDAP display name of the class or attribute contains non-ASCII characters.
.

MessageId=8480
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NON_BASE_SEARCH
Language=Bulgarian
ERROR_DS_NON_BASE_SEARCH - The requested search operation is only supported for base searches.
.

MessageId=8481
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_ATTS
Language=Bulgarian
ERROR_DS_CANT_RETRIEVE_ATTS - The search failed to retrieve attributes from the database.
.

MessageId=8482
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BACKLINK_WITHOUT_LINK
Language=Bulgarian
ERROR_DS_BACKLINK_WITHOUT_LINK - The schema update operation tried to add a backward link attribute that has no corresponding forward link.
.

MessageId=8483
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EPOCH_MISMATCH
Language=Bulgarian
ERROR_DS_EPOCH_MISMATCH - Source and destination of a cross domain move do not agree on the object's epoch number. Either source or destination does not have the latest version of the object.
.

MessageId=8484
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_NAME_MISMATCH
Language=Bulgarian
ERROR_DS_SRC_NAME_MISMATCH - Source and destination of a cross domain move do not agree on the object's current name. Either source or destination does not have the latest version of the object.
.

MessageId=8485
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_AND_DST_NC_IDENTICAL
Language=Bulgarian
ERROR_DS_SRC_AND_DST_NC_IDENTICAL - Source and destination of a cross domain move operation are identical. Caller should use local move operation instead of cross domain move operation.
.

MessageId=8486
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DST_NC_MISMATCH
Language=Bulgarian
ERROR_DS_DST_NC_MISMATCH - Source and destination for a cross domain move are not in agreement on the naming contexts in the forest. Either source or destination does not have the latest version of the Partitions container.
.

MessageId=8487
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC
Language=Bulgarian
ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC - Destination of a cross domain move is not authoritative for the destination naming context.
.

MessageId=8488
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_GUID_MISMATCH
Language=Bulgarian
ERROR_DS_SRC_GUID_MISMATCH - Source and destination of a cross domain move do not agree on the identity of the source object. Either source or destination does not have the latest version of the source object.
.

MessageId=8489
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_DELETED_OBJECT
Language=Bulgarian
ERROR_DS_CANT_MOVE_DELETED_OBJECT - Object being moved across domains is already known to be deleted by the destination server. The source server does not have the latest version of the source object.
.

MessageId=8490
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PDC_OPERATION_IN_PROGRESS
Language=Bulgarian
ERROR_DS_PDC_OPERATION_IN_PROGRESS - Another operation, which requires exclusive access to the PDC PSMO, is already in progress.
.

MessageId=8491
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD
Language=Bulgarian
ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD - A cross domain move operation failed such that the two versions of the moved object exist - one each in the source and destination domains. The destination object needs to be removed to restore the system to a consistent state.
.

MessageId=8492
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION
Language=Bulgarian
ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION - This object may not be moved across domain boundaries either because cross domain moves for this class are disallowed, or the object has some special characteristics, e.g.: trust account or restricted RID, which prevent its move.
.

MessageId=8493
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS
Language=Bulgarian
ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS - Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group. Remove the object from any account group memberships and retry.
.

MessageId=8494
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NC_MUST_HAVE_NC_PARENT
Language=Bulgarian
ERROR_DS_NC_MUST_HAVE_NC_PARENT - A naming context head must be the immediate child of another naming context head, not of an interior node.
.

MessageId=8495
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE
Language=Bulgarian
ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE - The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context. Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters)
.

MessageId=8496
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DST_DOMAIN_NOT_NATIVE
Language=Bulgarian
ERROR_DS_DST_DOMAIN_NOT_NATIVE - Destination domain must be in native mode.
.

MessageId=8497
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER
Language=Bulgarian
ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER - The operation cannot be performed because the server does not have an infrastructure container in the domain of interest.
.

MessageId=8498
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_ACCOUNT_GROUP
Language=Bulgarian
ERROR_DS_CANT_MOVE_ACCOUNT_GROUP - Cross-domain move of non-empty account groups is not allowed.
.

MessageId=8499
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_RESOURCE_GROUP
Language=Bulgarian
ERROR_DS_CANT_MOVE_RESOURCE_GROUP - Cross-domain move of non-empty resource groups is not allowed.
.

MessageId=8500
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_SEARCH_FLAG
Language=Bulgarian
ERROR_DS_INVALID_SEARCH_FLAG - The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings.
.

MessageId=8501
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_TREE_DELETE_ABOVE_NC
Language=Bulgarian
ERROR_DS_NO_TREE_DELETE_ABOVE_NC - Tree deletions starting at an object which has an NC head as a descendant are not allowed.
.

MessageId=8502
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE
Language=Bulgarian
ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE - The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use.
.

MessageId=8503
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE
Language=Bulgarian
ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE - The directory service failed to identify the list of objects to delete while attempting a tree deletion.
.

MessageId=8504
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_INIT_FAILURE
Language=Bulgarian
ERROR_DS_SAM_INIT_FAILURE - Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Directory Services Restore Mode. Check the event log for detailed information.
.

MessageId=8505
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SENSITIVE_GROUP_VIOLATION
Language=Bulgarian
ERROR_DS_SENSITIVE_GROUP_VIOLATION - Only an administrator can modify the membership list of an administrative group.
.

MessageId=8506
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_PRIMARYGROUPID
Language=Bulgarian
ERROR_DS_CANT_MOD_PRIMARYGROUPID - Cannot change the primary group ID of a domain controller account.
.

MessageId=8507
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD
Language=Bulgarian
ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD - An attempt is made to modify the base schema.
.

MessageId=8508
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONSAFE_SCHEMA_CHANGE
Language=Bulgarian
ERROR_DS_NONSAFE_SCHEMA_CHANGE - Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed.
.

MessageId=8509
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_UPDATE_DISALLOWED
Language=Bulgarian
ERROR_DS_SCHEMA_UPDATE_DISALLOWED - Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner.
.

MessageId=8510
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CREATE_UNDER_SCHEMA
Language=Bulgarian
ERROR_DS_CANT_CREATE_UNDER_SCHEMA - An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container.
.

MessageId=8511
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_NO_SRC_SCH_VERSION
Language=Bulgarian
ERROR_DS_INSTALL_NO_SRC_SCH_VERSION - The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it.
.

MessageId=8512
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE
Language=Bulgarian
ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE - The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory.
.

MessageId=8513
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_GROUP_TYPE
Language=Bulgarian
ERROR_DS_INVALID_GROUP_TYPE - The specified group type is invalid.
.

MessageId=8514
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN
Language=Bulgarian
ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN - Cannot nest global groups in a mixed domain if the group is security-enabled.
.

MessageId=8515
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN
Language=Bulgarian
ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN - Cannot nest local groups in a mixed domain if the group is security-enabled.
.

MessageId=8516
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER
Language=Bulgarian
ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER - A global group cannot have a local group as a member.
.

MessageId=8517
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER
Language=Bulgarian
ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER - A global group cannot have a universal group as a member.
.

MessageId=8518
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER
Language=Bulgarian
ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER - A universal group cannot have a local group as a member.
.

MessageId=8519
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER
Language=Bulgarian
ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER - A global group cannot have a cross-domain member.
.

MessageId=8520
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER
Language=Bulgarian
ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER - A local group cannot have another cross-domain local group as a member.
.

MessageId=8521
Severity=Success
Facility=System
SymbolicName=ERROR_DS_HAVE_PRIMARY_MEMBERS
Language=Bulgarian
ERROR_DS_HAVE_PRIMARY_MEMBERS - A group with primary members cannot change to a security-disabled group.
.

MessageId=8522
Severity=Success
Facility=System
SymbolicName=ERROR_DS_STRING_SD_CONVERSION_FAILED
Language=Bulgarian
ERROR_DS_STRING_SD_CONVERSION_FAILED - The schema cache load failed to convert the string default SD on a class-schema object.
.

MessageId=8523
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAMING_MASTER_GC
Language=Bulgarian
ERROR_DS_NAMING_MASTER_GC - Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers)
.

MessageId=8524
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOOKUP_FAILURE
Language=Bulgarian
ERROR_DS_LOOKUP_FAILURE - The DSA operation is unable to proceed because of a DNS lookup failure.
.

MessageId=8525
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_UPDATE_SPNS
Language=Bulgarian
ERROR_DS_COULDNT_UPDATE_SPNS - While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync.
.

MessageId=8526
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_SD
Language=Bulgarian
ERROR_DS_CANT_RETRIEVE_SD - The Security Descriptor attribute could not be read.
.

MessageId=8527
Severity=Success
Facility=System
SymbolicName=ERROR_DS_KEY_NOT_UNIQUE
Language=Bulgarian
ERROR_DS_KEY_NOT_UNIQUE - The object requested was not found, but an object with that key was found.
.

MessageId=8528
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WRONG_LINKED_ATT_SYNTAX
Language=Bulgarian
ERROR_DS_WRONG_LINKED_ATT_SYNTAX - The syntax of the linked attributed being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1.
.

MessageId=8529
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD
Language=Bulgarian
ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD - Security Account Manager needs to get the boot password.
.

MessageId=8530
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY
Language=Bulgarian
ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY - Security Account Manager needs to get the boot key from floppy disk.
.

MessageId=8531
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_START
Language=Bulgarian
ERROR_DS_CANT_START - Directory Service cannot start.
.

MessageId=8532
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INIT_FAILURE
Language=Bulgarian
ERROR_DS_INIT_FAILURE - Directory Services could not start.
.

MessageId=8533
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION
Language=Bulgarian
ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION - The connection between client and server requires packet privacy or better.
.

MessageId=8534
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SOURCE_DOMAIN_IN_FOREST
Language=Bulgarian
ERROR_DS_SOURCE_DOMAIN_IN_FOREST - The source domain may not be in the same forest as destination.
.

MessageId=8535
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST
Language=Bulgarian
ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST - The destination domain must be in the forest.
.

MessageId=8536
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED
Language=Bulgarian
ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED - The operation requires that destination domain auditing be enabled.
.

MessageId=8537
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN
Language=Bulgarian
ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN - The operation couldn't locate a DC for the source domain.
.

MessageId=8538
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER
Language=Bulgarian
ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER - The source object must be a group or user.
.

MessageId=8539
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_SID_EXISTS_IN_FOREST
Language=Bulgarian
ERROR_DS_SRC_SID_EXISTS_IN_FOREST - The source object's SID already exists in destination forest.
.

MessageId=8540
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH
Language=Bulgarian
ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH - The source and destination object must be of the same type.
.

MessageId=8541
Severity=Success
Facility=System
SymbolicName=ERROR_SAM_INIT_FAILURE
Language=Bulgarian
ERROR_SAM_INIT_FAILURE - Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Safe Mode. Check the event log for detailed information.
.

MessageId=8542
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_INFO_SHIP
Language=Bulgarian
ERROR_DS_DRA_SCHEMA_INFO_SHIP - Schema information could not be included in the replication request.
.

MessageId=8543
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_CONFLICT
Language=Bulgarian
ERROR_DS_DRA_SCHEMA_CONFLICT - The replication operation could not be completed due to a schema incompatibility.
.

MessageId=8544
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_EARLIER_SCHEMA_CONLICT
Language=Bulgarian
ERROR_DS_DRA_EARLIER_SCHEMA_CONLICT - The replication operation could not be completed due to a previous schema incompatibility.
.

MessageId=8545
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OBJ_NC_MISMATCH
Language=Bulgarian
ERROR_DS_DRA_OBJ_NC_MISMATCH - The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation.
.

MessageId=8546
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NC_STILL_HAS_DSAS
Language=Bulgarian
ERROR_DS_NC_STILL_HAS_DSAS - The requested domain could not be deleted because there exist domain controllers that still host this domain.
.

MessageId=8547
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GC_REQUIRED
Language=Bulgarian
ERROR_DS_GC_REQUIRED - The requested operation can be performed only on a global catalog server.
.

MessageId=8548
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY
Language=Bulgarian
ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY - A local group can only be a member of other local groups in the same domain.
.

MessageId=8549
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS
Language=Bulgarian
ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS - Foreign security principals cannot be members of universal groups.
.

MessageId=8550
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_TO_GC
Language=Bulgarian
ERROR_DS_CANT_ADD_TO_GC - The attribute is not allowed to be replicated to the GC because of security reasons.
.

MessageId=8551
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHECKPOINT_WITH_PDC
Language=Bulgarian
ERROR_DS_NO_CHECKPOINT_WITH_PDC - The checkpoint with the PDC could not be taken because there are too many modifications being processed currently.
.

MessageId=8552
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SOURCE_AUDITING_NOT_ENABLED
Language=Bulgarian
ERROR_DS_SOURCE_AUDITING_NOT_ENABLED - The operation requires that source domain auditing be enabled.
.

MessageId=8553
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC
Language=Bulgarian
ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC - Security principal objects can only be created inside domain naming contexts.
.

MessageId=8554
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_NAME_FOR_SPN
Language=Bulgarian
ERROR_DS_INVALID_NAME_FOR_SPN - A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format.
.

MessageId=8555
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS
Language=Bulgarian
ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS - A Filter was passed that uses constructed attributes.
.

MessageId=8556
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNICODEPWD_NOT_IN_QUOTES
Language=Bulgarian
ERROR_DS_UNICODEPWD_NOT_IN_QUOTES - The unicodePwd attribute value must be enclosed in double quotes.
.

MessageId=8557
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED
Language=Bulgarian
ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED - Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.
.

MessageId=8558
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MUST_BE_RUN_ON_DST_DC
Language=Bulgarian
ERROR_DS_MUST_BE_RUN_ON_DST_DC - For security reasons, the operation must be run on the destination DC.
.

MessageId=8559
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER
Language=Bulgarian
ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER - For security reasons, the source DC must be NT4SP4 or greater.
.

MessageId=8560
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ
Language=Bulgarian
ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ - Critical Directory Service System objects cannot be deleted during tree delete operations. The tree delete may have been partially performed.
.

MessageId=8561
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INIT_FAILURE_CONSOLE
Language=Bulgarian
ERROR_DS_INIT_FAILURE_CONSOLE - Directory Services could not start because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8562
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_INIT_FAILURE_CONSOLE
Language=Bulgarian
ERROR_DS_SAM_INIT_FAILURE_CONSOLE - Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8563
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FOREST_VERSION_TOO_HIGH
Language=Bulgarian
ERROR_DS_FOREST_VERSION_TOO_HIGH - The version of the operating system installed is incompatible with the current forest functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this forest.
.

MessageId=8564
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_HIGH
Language=Bulgarian
ERROR_DS_DOMAIN_VERSION_TOO_HIGH - The version of the operating system installed is incompatible with the current domain functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this domain.
.

MessageId=8565
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FOREST_VERSION_TOO_LOW
Language=Bulgarian
ERROR_DS_FOREST_VERSION_TOO_LOW - This version of the operating system installed on this server no longer supports the current forest functional level. You must raise the forest functional level before this server can become a domain controller in this forest.
.

MessageId=8566
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_LOW
Language=Bulgarian
ERROR_DS_DOMAIN_VERSION_TOO_LOW - This version of the operating system installed on this server no longer supports the current domain functional level. You must raise the domain functional level before this server can become a domain controller in this domain.
.

MessageId=8567
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCOMPATIBLE_VERSION
Language=Bulgarian
ERROR_DS_INCOMPATIBLE_VERSION - The version of the operating system installed on this server is incompatible with the functional level of the domain or forest.
.

MessageId=8568
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOW_DSA_VERSION
Language=Bulgarian
ERROR_DS_LOW_DSA_VERSION - The functional level of the domain (or forest) cannot be raised to the requested value, because there exist one or more domain controllers in the domain (or forest) that are at a lower incompatible functional level.
.

MessageId=8569
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN
Language=Bulgarian
ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN - The forest functional level cannot be raised to the requested level since one or more domains are still in mixed domain mode. All domains in the forest must be in native mode before you can raise the forest functional level.
.

MessageId=8570
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_SUPPORTED_SORT_ORDER
Language=Bulgarian
ERROR_DS_NOT_SUPPORTED_SORT_ORDER - The sort order requested is not supported.
.

MessageId=8571
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_NOT_UNIQUE
Language=Bulgarian
ERROR_DS_NAME_NOT_UNIQUE - The requested name already exists as a unique identifier.
.

MessageId=8572
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4
Language=Bulgarian
ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4 - The machine account was created pre-NT4. The account needs to be recreated.
.

MessageId=8573
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OUT_OF_VERSION_STORE
Language=Bulgarian
ERROR_DS_OUT_OF_VERSION_STORE - The database is out of version store.
.

MessageId=8574
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCOMPATIBLE_CONTROLS_USED
Language=Bulgarian
ERROR_DS_INCOMPATIBLE_CONTROLS_USED - Unable to continue operation because multiple conflicting controls were used.
.

MessageId=8575
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_REF_DOMAIN
Language=Bulgarian
ERROR_DS_NO_REF_DOMAIN - Unable to find a valid security descriptor reference domain for this partition.
.

MessageId=8576
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RESERVED_LINK_ID
Language=Bulgarian
ERROR_DS_RESERVED_LINK_ID - Schema update failed: The link identifier is reserved.
.

MessageId=8577
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LINK_ID_NOT_AVAILABLE
Language=Bulgarian
ERROR_DS_LINK_ID_NOT_AVAILABLE - Schema update failed: There are no link identifiers available.
.

MessageId=8578
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER
Language=Bulgarian
ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER - An account group cannot have a universal group as a member.
.

MessageId=8579
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE
Language=Bulgarian
ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE - Rename or move operations on naming context heads or read-only objects are not allowed.
.

MessageId=8580
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC
Language=Bulgarian
ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC - Move operations on objects in the schema naming context are not allowed.
.

MessageId=8581
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG
Language=Bulgarian
ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG - A system flag has been set on the object and does not allow the object to be moved or renamed.
.

MessageId=8582
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_WRONG_GRANDPARENT
Language=Bulgarian
ERROR_DS_MODIFYDN_WRONG_GRANDPARENT - This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers.
.

MessageId=8583
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_TRUST_REFERRAL
Language=Bulgarian
ERROR_DS_NAME_ERROR_TRUST_REFERRAL - Unable to resolve completely, a referral to another forest is generated.
.

MessageId=8584
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER
Language=Bulgarian
ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER - The requested action is not supported on standard server.
.

MessageId=8585
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD
Language=Bulgarian
ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD - Could not access a partition of the Active Directory located on a remote server. Make sure at least one server is running for the partition in question.
.

MessageId=8586
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2
Language=Bulgarian
ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2 - The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context. Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master.
.

MessageId=8587
Severity=Success
Facility=System
SymbolicName=ERROR_DS_THREAD_LIMIT_EXCEEDED
Language=Bulgarian
ERROR_DS_THREAD_LIMIT_EXCEEDED - The thread limit for this request was exceeded.
.

MessageId=8588
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_CLOSEST
Language=Bulgarian
ERROR_DS_NOT_CLOSEST - The Global catalog server is not in the closet site.
.

MessageId=8589
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF
Language=Bulgarian
ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF - The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute.
.

MessageId=8590
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SINGLE_USER_MODE_FAILED
Language=Bulgarian
ERROR_DS_SINGLE_USER_MODE_FAILED - The Directory Service failed to enter single user mode.
.

MessageId=8591
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NTDSCRIPT_SYNTAX_ERROR
Language=Bulgarian
ERROR_DS_NTDSCRIPT_SYNTAX_ERROR - The Directory Service cannot parse the script because of a syntax error.
.

MessageId=8592
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NTDSCRIPT_PROCESS_ERROR
Language=Bulgarian
ERROR_DS_NTDSCRIPT_PROCESS_ERROR - The Directory Service cannot process the script because of an error.
.

MessageId=8593
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DIFFERENT_REPL_EPOCHS
Language=Bulgarian
ERROR_DS_DIFFERENT_REPL_EPOCHS - The directory service cannot perform the requested operation because the servers involved are of different replication epochs (which is usually related to a domain rename that is in progress).
.

MessageId=8594
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRS_EXTENSIONS_CHANGED
Language=Bulgarian
ERROR_DS_DRS_EXTENSIONS_CHANGED - The directory service binding must be renegotiated due to a change in the server extensions information.
.

MessageId=8595
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR
Language=Bulgarian
ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR - Operation not allowed on a disabled cross ref.
.

MessageId=8596
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_MSDS_INTID
Language=Bulgarian
ERROR_DS_NO_MSDS_INTID - Schema update failed: No values for msDS-IntId are available.
.

MessageId=8597
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_MSDS_INTID
Language=Bulgarian
ERROR_DS_DUP_MSDS_INTID - Schema update failed: Duplicate msDS-IntId. Retry the operation.
.

MessageId=8598
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_RDNATTID
Language=Bulgarian
ERROR_DS_EXISTS_IN_RDNATTID - Schema deletion failed: attribute is used in rDNAttID.
.

MessageId=8599
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTHORIZATION_FAILED
Language=Bulgarian
ERROR_DS_AUTHORIZATION_FAILED - The directory service failed to authorize the request.
.

MessageId=8600
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_SCRIPT
Language=Bulgarian
ERROR_DS_INVALID_SCRIPT - The Directory Service cannot process the script because it is invalid.
.

MessageId=8601
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REMOTE_CROSSREF_OP_FAILED
Language=Bulgarian
ERROR_DS_REMOTE_CROSSREF_OP_FAILED - The remote create cross reference operation failed on the Domain Naming Master FSMO. The operation's error is in the extended data.
.

MessageId=8602
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_REF_BUSY
Language=Bulgarian
ERROR_DS_CROSS_REF_BUSY - A cross reference is in use locally with the same name.
.

MessageId=8603
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN
Language=Bulgarian
ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN - The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the server's domain has been deleted from the forest.
.

MessageId=8604
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC
Language=Bulgarian
ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC - Writeable NCs prevent this DC from demoting.
.

MessageId=8605
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUPLICATE_ID_FOUND
Language=Bulgarian
ERROR_DS_DUPLICATE_ID_FOUND - The requested object has a non-unique identifier and cannot be retrieved.
.

MessageId=8606
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT
Language=Bulgarian
ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT - Insufficient attributes were given to create an object. This object may not exist because it may have been deleted and already garbage collected.
.

MessageId=8607
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GROUP_CONVERSION_ERROR
Language=Bulgarian
ERROR_DS_GROUP_CONVERSION_ERROR - The group cannot be converted due to attribute restrictions on the requested group type.
.

MessageId=8608
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_APP_BASIC_GROUP
Language=Bulgarian
ERROR_DS_CANT_MOVE_APP_BASIC_GROUP - Cross-domain move of non-empty basic application groups is not allowed.
.

MessageId=8609
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_APP_QUERY_GROUP
Language=Bulgarian
ERROR_DS_CANT_MOVE_APP_QUERY_GROUP - Cross-domain move on non-empty query based application groups is not allowed.
.

MessageId=8610
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROLE_NOT_VERIFIED
Language=Bulgarian
ERROR_DS_ROLE_NOT_VERIFIED - The role owner could not be verified because replication of its partition has not occurred recently.
.

MessageId=8611
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL
Language=Bulgarian
ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL - The target container for a redirection of a well-known object container cannot already be a special container.
.

MessageId=8612
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_RENAME_IN_PROGRESS
Language=Bulgarian
ERROR_DS_DOMAIN_RENAME_IN_PROGRESS - The Directory Service cannot perform the requested operation because a domain rename operation is in progress.
.

MessageId=8613
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTING_AD_CHILD_NC
Language=Bulgarian
ERROR_DS_EXISTING_AD_CHILD_NC - The Active Directory detected an Active Directory child partition below the requested new partition name. The Active Directory's partition hierarchy must be created in a top-down method.
.

MessageId=8614
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPL_LIFETIME_EXCEEDED
Language=Bulgarian
ERROR_DS_REPL_LIFETIME_EXCEEDED - The Active Directory cannot replicate with this server because the time since the last replication with this server has exceeded the tombstone lifetime.
.

MessageId=8615
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER
Language=Bulgarian
ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER - The requested operation is not allowed on an object under the system container.
.

MessageId=8616
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LDAP_SEND_QUEUE_FULL
Language=Bulgarian
ERROR_DS_LDAP_SEND_QUEUE_FULL - The LDAP servers network send queue has filled up because the client is not processing the results of it's requests fast enough. No more requests will be processed until the client catches up. If the client does not catch up then it will be disconnected.
.

MessageId=8617
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OUT_SCHEDULE_WINDOW
Language=Bulgarian
ERROR_DS_DRA_OUT_SCHEDULE_WINDOW - The scheduled replication did not take place because the system was too busy to execute the request within the schedule window. The replication queue is overloaded. Consider reducing the number of partners or decreasing the scheduled replication frequency.
.

MessageId=9001
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_FORMAT_ERROR
Language=Bulgarian
DNS_ERROR_RCODE_FORMAT_ERROR - DNS server unable to interpret format.
.

MessageId=9002
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_SERVER_FAILURE
Language=Bulgarian
DNS_ERROR_RCODE_SERVER_FAILURE - DNS server failure.
.

MessageId=9003
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NAME_ERROR
Language=Bulgarian
DNS_ERROR_RCODE_NAME_ERROR - DNS name does not exist.
.

MessageId=9004
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOT_IMPLEMENTED
Language=Bulgarian
DNS_ERROR_RCODE_NOT_IMPLEMENTED - DNS request not supported by name server.
.

MessageId=9005
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_REFUSED
Language=Bulgarian
DNS_ERROR_RCODE_REFUSED - DNS operation refused.
.

MessageId=9006
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_YXDOMAIN
Language=Bulgarian
DNS_ERROR_RCODE_YXDOMAIN - DNS name that ought not exist, does exist.
.

MessageId=9007
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_YXRRSET
Language=Bulgarian
DNS_ERROR_RCODE_YXRRSET - DNS RR set that ought not exist, does exist.
.

MessageId=9008
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NXRRSET
Language=Bulgarian
DNS_ERROR_RCODE_NXRRSET - DNS RR set that ought to exist, does not exist.
.

MessageId=9009
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOTAUTH
Language=Bulgarian
DNS_ERROR_RCODE_NOTAUTH - DNS server not authoritative for zone.
.

MessageId=9010
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOTZONE
Language=Bulgarian
DNS_ERROR_RCODE_NOTZONE - DNS name in update or prereq is not in zone.
.

MessageId=9016
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADSIG
Language=Bulgarian
DNS_ERROR_RCODE_BADSIG - DNS signature failed to verify.
.

MessageId=9017
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADKEY
Language=Bulgarian
DNS_ERROR_RCODE_BADKEY - DNS bad key.
.

MessageId=9018
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADTIME
Language=Bulgarian
DNS_ERROR_RCODE_BADTIME - DNS signature validity expired.
.

MessageId=9501
Severity=Success
Facility=System
SymbolicName=DNS_INFO_NO_RECORDS
Language=Bulgarian
DNS_INFO_NO_RECORDS - No records found for given DNS query.
.

MessageId=9502
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_BAD_PACKET
Language=Bulgarian
DNS_ERROR_BAD_PACKET - Bad DNS packet.
.

MessageId=9503
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_PACKET
Language=Bulgarian
DNS_ERROR_NO_PACKET - No DNS packet.
.

MessageId=9504
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE
Language=Bulgarian
DNS_ERROR_RCODE - DNS error, check rcode.
.

MessageId=9505
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_UNSECURE_PACKET
Language=Bulgarian
DNS_ERROR_UNSECURE_PACKET - Unsecured DNS packet.
.

MessageId=9551
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_TYPE
Language=Bulgarian
DNS_ERROR_INVALID_TYPE - Invalid DNS type.
.

MessageId=9552
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_IP_ADDRESS
Language=Bulgarian
DNS_ERROR_INVALID_IP_ADDRESS - Invalid IP address.
.

MessageId=9553
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_PROPERTY
Language=Bulgarian
DNS_ERROR_INVALID_PROPERTY - Invalid property.
.

MessageId=9554
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_TRY_AGAIN_LATER
Language=Bulgarian
DNS_ERROR_TRY_AGAIN_LATER - Try DNS operation again later.
.

MessageId=9555
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_UNIQUE
Language=Bulgarian
DNS_ERROR_NOT_UNIQUE - Record for given name and type is not unique.
.

MessageId=9556
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NON_RFC_NAME
Language=Bulgarian
DNS_ERROR_NON_RFC_NAME - DNS name does not comply with RFC specifications.
.

MessageId=9557
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_FQDN
Language=Bulgarian
DNS_STATUS_FQDN - DNS name is a fully-qualified DNS name.
.

MessageId=9558
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_DOTTED_NAME
Language=Bulgarian
DNS_STATUS_DOTTED_NAME - DNS name is dotted (multi-label).
.

MessageId=9559
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_SINGLE_PART_NAME
Language=Bulgarian
DNS_STATUS_SINGLE_PART_NAME - DNS name is a single-part name.
.

MessageId=9560
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_NAME_CHAR
Language=Bulgarian
DNS_ERROR_INVALID_NAME_CHAR - DSN name contains an invalid character.
.

MessageId=9561
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NUMERIC_NAME
Language=Bulgarian
DNS_ERROR_NUMERIC_NAME - DNS name is entirely numeric.
.

MessageId=9562
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER
Language=Bulgarian
DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER - The operation requested is not permitted on a DNS root server.
.

MessageId=9563
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION
Language=Bulgarian
DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION - The record could not be created because this part of the DNS namespace has been delegated to another server.
.

MessageId=9564
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CANNOT_FIND_ROOT_HINTS
Language=Bulgarian
DNS_ERROR_CANNOT_FIND_ROOT_HINTS - The DNS server could not find a set of root hints.
.

MessageId=9565
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INCONSISTENT_ROOT_HINTS
Language=Bulgarian
DNS_ERROR_INCONSISTENT_ROOT_HINTS - The DNS server found root hints but they were not consistent across all adapters.
.

MessageId=9601
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_DOES_NOT_EXIST
Language=Bulgarian
DNS_ERROR_ZONE_DOES_NOT_EXIST - DNS zone does not exist.
.

MessageId=9602
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_ZONE_INFO
Language=Bulgarian
DNS_ERROR_NO_ZONE_INFO - DNS zone information not available.
.

MessageId=9603
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_ZONE_OPERATION
Language=Bulgarian
DNS_ERROR_INVALID_ZONE_OPERATION - Invalid operation for DNS zone.
.

MessageId=9604
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_CONFIGURATION_ERROR
Language=Bulgarian
DNS_ERROR_ZONE_CONFIGURATION_ERROR - Invalid DNS zone configuration.
.

MessageId=9605
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_HAS_NO_SOA_RECORD
Language=Bulgarian
DNS_ERROR_ZONE_HAS_NO_SOA_RECORD - DNS zone has no start of authority (SOA) record.
.

MessageId=9606
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_HAS_NO_NS_RECORDS
Language=Bulgarian
DNS_ERROR_ZONE_HAS_NO_NS_RECORDS - DNS zone has no name server (NS) record.
.

MessageId=9607
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_LOCKED
Language=Bulgarian
DNS_ERROR_ZONE_LOCKED - DNS zone is locked.
.

MessageId=9608
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_CREATION_FAILED
Language=Bulgarian
DNS_ERROR_ZONE_CREATION_FAILED - DNS zone creation failed.
.

MessageId=9609
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_ZONE_ALREADY_EXISTS - DNS zone already exists.
.

MessageId=9610
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_AUTOZONE_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_AUTOZONE_ALREADY_EXISTS - DNS automatic zone already exists.
.

MessageId=9611
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_ZONE_TYPE
Language=Bulgarian
DNS_ERROR_INVALID_ZONE_TYPE - Invalid DNS zone type.
.

MessageId=9612
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP
Language=Bulgarian
DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP - Secondary DNS zone requires master IP address.
.

MessageId=9613
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_NOT_SECONDARY
Language=Bulgarian
DNS_ERROR_ZONE_NOT_SECONDARY - DNS zone not secondary.
.

MessageId=9614
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NEED_SECONDARY_ADDRESSES
Language=Bulgarian
DNS_ERROR_NEED_SECONDARY_ADDRESSES - Need secondary IP address.
.

MessageId=9615
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_WINS_INIT_FAILED
Language=Bulgarian
DNS_ERROR_WINS_INIT_FAILED - WINS initialization failed.
.

MessageId=9616
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NEED_WINS_SERVERS
Language=Bulgarian
DNS_ERROR_NEED_WINS_SERVERS - Need WINS servers.
.

MessageId=9617
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NBSTAT_INIT_FAILED
Language=Bulgarian
DNS_ERROR_NBSTAT_INIT_FAILED - NBTSTAT initialization call failed.
.

MessageId=9618
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SOA_DELETE_INVALID
Language=Bulgarian
DNS_ERROR_SOA_DELETE_INVALID - Invalid delete of start of authority (SOA)
.

MessageId=9619
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_FORWARDER_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_FORWARDER_ALREADY_EXISTS - A conditional forwarding zone already exists for that name.
.

MessageId=9620
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_REQUIRES_MASTER_IP
Language=Bulgarian
DNS_ERROR_ZONE_REQUIRES_MASTER_IP - This zone must be configured with one or more master DNS server IP addresses.
.

MessageId=9621
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_IS_SHUTDOWN
Language=Bulgarian
DNS_ERROR_ZONE_IS_SHUTDOWN - The operation cannot be performed because this zone is shutdown.
.

MessageId=9651
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_PRIMARY_REQUIRES_DATAFILE
Language=Bulgarian
DNS_ERROR_PRIMARY_REQUIRES_DATAFILE - Primary DNS zone requires datafile.
.

MessageId=9652
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_DATAFILE_NAME
Language=Bulgarian
DNS_ERROR_INVALID_DATAFILE_NAME - Invalid datafile name for DNS zone.
.

MessageId=9653
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DATAFILE_OPEN_FAILURE
Language=Bulgarian
DNS_ERROR_DATAFILE_OPEN_FAILURE - Failed to open datafile for DNS zone.
.

MessageId=9654
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_FILE_WRITEBACK_FAILED
Language=Bulgarian
DNS_ERROR_FILE_WRITEBACK_FAILED - Failed to write datafile for DNS zone.
.

MessageId=9655
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DATAFILE_PARSING
Language=Bulgarian
DNS_ERROR_DATAFILE_PARSING - Failure while reading datafile for DNS zone.
.

MessageId=9701
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_DOES_NOT_EXIST
Language=Bulgarian
DNS_ERROR_RECORD_DOES_NOT_EXIST - DNS record does not exist.
.

MessageId=9702
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_FORMAT
Language=Bulgarian
DNS_ERROR_RECORD_FORMAT - DNS record format error.
.

MessageId=9703
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NODE_CREATION_FAILED
Language=Bulgarian
DNS_ERROR_NODE_CREATION_FAILED - Node creation failure in DNS.
.

MessageId=9704
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_UNKNOWN_RECORD_TYPE
Language=Bulgarian
DNS_ERROR_UNKNOWN_RECORD_TYPE - Unknown DNS record type.
.

MessageId=9705
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_TIMED_OUT
Language=Bulgarian
DNS_ERROR_RECORD_TIMED_OUT - DNS record timed out.
.

MessageId=9706
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NAME_NOT_IN_ZONE
Language=Bulgarian
DNS_ERROR_NAME_NOT_IN_ZONE - Name not in DNS zone.
.

MessageId=9707
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CNAME_LOOP
Language=Bulgarian
DNS_ERROR_CNAME_LOOP - CNAME loop detected.
.

MessageId=9708
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NODE_IS_CNAME
Language=Bulgarian
DNS_ERROR_NODE_IS_CNAME - Node is a CNAME DNS record.
.

MessageId=9709
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CNAME_COLLISION
Language=Bulgarian
DNS_ERROR_CNAME_COLLISION - A CNAME record already exists for given name.
.

MessageId=9710
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT
Language=Bulgarian
DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT - Record only at DNS zone root.
.

MessageId=9711
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_RECORD_ALREADY_EXISTS - DNS record already exists.
.

MessageId=9712
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SECONDARY_DATA
Language=Bulgarian
DNS_ERROR_SECONDARY_DATA - Secondary DNS zone data error.
.

MessageId=9713
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_CREATE_CACHE_DATA
Language=Bulgarian
DNS_ERROR_NO_CREATE_CACHE_DATA - Could not create DNS cache data.
.

MessageId=9714
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NAME_DOES_NOT_EXIST
Language=Bulgarian
DNS_ERROR_NAME_DOES_NOT_EXIST - DNS name does not exist.
.

MessageId=9715
Severity=Success
Facility=System
SymbolicName=DNS_WARNING_PTR_CREATE_FAILED
Language=Bulgarian
DNS_WARNING_PTR_CREATE_FAILED - Could not create pointer (PTR) record.
.

MessageId=9716
Severity=Success
Facility=System
SymbolicName=DNS_WARNING_DOMAIN_UNDELETED
Language=Bulgarian
DNS_WARNING_DOMAIN_UNDELETED - DNS domain was undeleted.
.

MessageId=9717
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DS_UNAVAILABLE
Language=Bulgarian
DNS_ERROR_DS_UNAVAILABLE - The directory service is unavailable.
.

MessageId=9718
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DS_ZONE_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_DS_ZONE_ALREADY_EXISTS - DNS zone already exists in the directory service.
.

MessageId=9719
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE
Language=Bulgarian
DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE - DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.

MessageId=9751
Severity=Success
Facility=System
SymbolicName=DNS_INFO_AXFR_COMPLETE
Language=Bulgarian
DNS_INFO_AXFR_COMPLETE - DNS AXFR (zone transfer) complete.
.

MessageId=9752
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_AXFR
Language=Bulgarian
DNS_ERROR_AXFR - DNS zone transfer failed.
.

MessageId=9753
Severity=Success
Facility=System
SymbolicName=DNS_INFO_ADDED_LOCAL_WINS
Language=Bulgarian
DNS_INFO_ADDED_LOCAL_WINS - Added local WINS server.
.

MessageId=9801
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_CONTINUE_NEEDED
Language=Bulgarian
DNS_STATUS_CONTINUE_NEEDED - Secure update call needs to continue update request.
.

MessageId=9851
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_TCPIP
Language=Bulgarian
DNS_ERROR_NO_TCPIP - TCP/IP network protocol not installed.
.

MessageId=9852
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_DNS_SERVERS
Language=Bulgarian
DNS_ERROR_NO_DNS_SERVERS - No DNS servers configured for local system.
.

MessageId=9901
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_DOES_NOT_EXIST
Language=Bulgarian
DNS_ERROR_DP_DOES_NOT_EXIST - The specified directory partition does not exist.
.

MessageId=9902
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_ALREADY_EXISTS
Language=Bulgarian
DNS_ERROR_DP_ALREADY_EXISTS - The specified directory partition already exists.
.

MessageId=9903
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_NOT_ENLISTED
Language=Bulgarian
DNS_ERROR_DP_NOT_ENLISTED - The DNS server is not enlisted in the specified directory partition.
.

MessageId=9904
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_ALREADY_ENLISTED
Language=Bulgarian
DNS_ERROR_DP_ALREADY_ENLISTED - The DNS server is already enlisted in the specified directory partition.
.

MessageId=9905
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_NOT_AVAILABLE
Language=Bulgarian
DNS_ERROR_DP_NOT_AVAILABLE - The directory partition is not available at this time. Please wait a few minutes and try again.
.

MessageId=9906
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_FSMO_ERROR
Language=Bulgarian
DNS_ERROR_DP_FSMO_ERROR - The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003.
.

MessageId=10004
Severity=Success
Facility=System
SymbolicName=WSAEINTR
Language=Bulgarian
WSAEINTR - A blocking operation was interrupted by a call to WSACancelBlockingCall.
.

MessageId=10009
Severity=Success
Facility=System
SymbolicName=WSAEBADF
Language=Bulgarian
WSAEBADF - The file handle supplied is not valid.
.

MessageId=10013
Severity=Success
Facility=System
SymbolicName=WSAEACCES
Language=Bulgarian
WSAEACCES - An attempt was made to access a socket in a way forbidden by its access permissions.
.

MessageId=10014
Severity=Success
Facility=System
SymbolicName=WSAEFAULT
Language=Bulgarian
WSAEFAULT - The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.

MessageId=10022
Severity=Success
Facility=System
SymbolicName=WSAEINVAL
Language=Bulgarian
WSAEINVAL - An invalid argument was supplied.
.

MessageId=10024
Severity=Success
Facility=System
SymbolicName=WSAEMFILE
Language=Bulgarian
WSAEMFILE - Too many open sockets.
.

MessageId=10035
Severity=Success
Facility=System
SymbolicName=WSAEWOULDBLOCK
Language=Bulgarian
WSAEWOULDBLOCK - A non-blocking socket operation could not be completed immediately.
.

MessageId=10036
Severity=Success
Facility=System
SymbolicName=WSAEINPROGRESS
Language=Bulgarian
WSAEINPROGRESS - A blocking operation is currently executing.
.

MessageId=10037
Severity=Success
Facility=System
SymbolicName=WSAEALREADY
Language=Bulgarian
WSAEALREADY - An operation was attempted on a non-blocking socket that already had an operation in progress.
.

MessageId=10038
Severity=Success
Facility=System
SymbolicName=WSAENOTSOCK
Language=Bulgarian
WSAENOTSOCK - An operation was attempted on something that is not a socket.
.

MessageId=10039
Severity=Success
Facility=System
SymbolicName=WSAEDESTADDRREQ
Language=Bulgarian
WSAEDESTADDRREQ - A required address was omitted from an operation on a socket.
.

MessageId=10040
Severity=Success
Facility=System
SymbolicName=WSAEMSGSIZE
Language=Bulgarian
WSAEMSGSIZE - A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.

MessageId=10041
Severity=Success
Facility=System
SymbolicName=WSAEPROTOTYPE
Language=Bulgarian
WSAEPROTOTYPE - A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.

MessageId=10042
Severity=Success
Facility=System
SymbolicName=WSAENOPROTOOPT
Language=Bulgarian
WSAENOPROTOOPT - An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.

MessageId=10043
Severity=Success
Facility=System
SymbolicName=WSAEPROTONOSUPPORT
Language=Bulgarian
WSAEPROTONOSUPPORT - The requested protocol has not been configured into the system, or no implementation for it exists.
.

MessageId=10044
Severity=Success
Facility=System
SymbolicName=WSAESOCKTNOSUPPORT
Language=Bulgarian
WSAESOCKTNOSUPPORT - The support for the specified socket type does not exist in this address family.
.

MessageId=10045
Severity=Success
Facility=System
SymbolicName=WSAEOPNOTSUPP
Language=Bulgarian
WSAEOPNOTSUPP - The attempted operation is not supported for the type of object referenced.
.

MessageId=10046
Severity=Success
Facility=System
SymbolicName=WSAEPFNOSUPPORT
Language=Bulgarian
WSAEPFNOSUPPORT - The protocol family has not been configured into the system or no implementation for it exists.
.

MessageId=10047
Severity=Success
Facility=System
SymbolicName=WSAEAFNOSUPPORT
Language=Bulgarian
WSAEAFNOSUPPORT - An address incompatible with the requested protocol was used.
.

MessageId=10048
Severity=Success
Facility=System
SymbolicName=WSAEADDRINUSE
Language=Bulgarian
WSAEADDRINUSE - Only one usage of each socket address (protocol/network address/port) is normally permitted.
.

MessageId=10049
Severity=Success
Facility=System
SymbolicName=WSAEADDRNOTAVAIL
Language=Bulgarian
WSAEADDRNOTAVAIL - The requested address is not valid in its context.
.

MessageId=10050
Severity=Success
Facility=System
SymbolicName=WSAENETDOWN
Language=Bulgarian
WSAENETDOWN - A socket operation encountered a dead network.
.

MessageId=10051
Severity=Success
Facility=System
SymbolicName=WSAENETUNREACH
Language=Bulgarian
WSAENETUNREACH - A socket operation was attempted to an unreachable network.
.

MessageId=10052
Severity=Success
Facility=System
SymbolicName=WSAENETRESET
Language=Bulgarian
WSAENETRESET - The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.

MessageId=10053
Severity=Success
Facility=System
SymbolicName=WSAECONNABORTED
Language=Bulgarian
WSAECONNABORTED - An established connection was aborted by the software in your host machine.
.

MessageId=10054
Severity=Success
Facility=System
SymbolicName=WSAECONNRESET
Language=Bulgarian
WSAECONNRESET - An existing connection was forcibly closed by the remote host.
.

MessageId=10055
Severity=Success
Facility=System
SymbolicName=WSAENOBUFS
Language=Bulgarian
WSAENOBUFS - An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.

MessageId=10056
Severity=Success
Facility=System
SymbolicName=WSAEISCONN
Language=Bulgarian
WSAEISCONN - A connect request was made on an already connected socket.
.

MessageId=10057
Severity=Success
Facility=System
SymbolicName=WSAENOTCONN
Language=Bulgarian
WSAENOTCONN - A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.

MessageId=10058
Severity=Success
Facility=System
SymbolicName=WSAESHUTDOWN
Language=Bulgarian
WSAESHUTDOWN - A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.

MessageId=10059
Severity=Success
Facility=System
SymbolicName=WSAETOOMANYREFS
Language=Bulgarian
WSAETOOMANYREFS - Too many references to some kernel object.
.

MessageId=10060
Severity=Success
Facility=System
SymbolicName=WSAETIMEDOUT
Language=Bulgarian
WSAETIMEDOUT - A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.

MessageId=10061
Severity=Success
Facility=System
SymbolicName=WSAECONNREFUSED
Language=Bulgarian
WSAECONNREFUSED - No connection could be made because the target machine actively refused it.
.

MessageId=10062
Severity=Success
Facility=System
SymbolicName=WSAELOOP
Language=Bulgarian
WSAELOOP - Cannot translate name.
.

MessageId=10063
Severity=Success
Facility=System
SymbolicName=WSAENAMETOOLONG
Language=Bulgarian
WSAENAMETOOLONG - Name component or name was too long.
.

MessageId=10064
Severity=Success
Facility=System
SymbolicName=WSAEHOSTDOWN
Language=Bulgarian
WSAEHOSTDOWN - A socket operation failed because the destination host was down.
.

MessageId=10065
Severity=Success
Facility=System
SymbolicName=WSAEHOSTUNREACH
Language=Bulgarian
WSAEHOSTUNREACH - A socket operation was attempted to an unreachable host.
.

MessageId=10066
Severity=Success
Facility=System
SymbolicName=WSAENOTEMPTY
Language=Bulgarian
WSAENOTEMPTY - Cannot remove a directory that is not empty.
.

MessageId=10067
Severity=Success
Facility=System
SymbolicName=WSAEPROCLIM
Language=Bulgarian
WSAEPROCLIM - A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.

MessageId=10068
Severity=Success
Facility=System
SymbolicName=WSAEUSERS
Language=Bulgarian
WSAEUSERS - Ran out of quota.
.

MessageId=10069
Severity=Success
Facility=System
SymbolicName=WSAEDQUOT
Language=Bulgarian
WSAEDQUOT - Ran out of disk quota.
.

MessageId=10070
Severity=Success
Facility=System
SymbolicName=WSAESTALE
Language=Bulgarian
WSAESTALE - File handle reference is no longer available.
.

MessageId=10071
Severity=Success
Facility=System
SymbolicName=WSAEREMOTE
Language=Bulgarian
WSAEREMOTE - Item is not available locally.
.

MessageId=10091
Severity=Success
Facility=System
SymbolicName=WSASYSNOTREADY
Language=Bulgarian
WSASYSNOTREADY - WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.

MessageId=10092
Severity=Success
Facility=System
SymbolicName=WSAVERNOTSUPPORTED
Language=Bulgarian
WSAVERNOTSUPPORTED - The Windows Sockets version requested is not supported.
.

MessageId=10093
Severity=Success
Facility=System
SymbolicName=WSANOTINITIALISED
Language=Bulgarian
WSANOTINITIALISED - Either the application has not called WSAStartup, or WSAStartup failed.
.

MessageId=10101
Severity=Success
Facility=System
SymbolicName=WSAEDISCON
Language=Bulgarian
WSAEDISCON - Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.

MessageId=10102
Severity=Success
Facility=System
SymbolicName=WSAENOMORE
Language=Bulgarian
WSAENOMORE - No more results can be returned by WSALookupServiceNext.
.

MessageId=10103
Severity=Success
Facility=System
SymbolicName=WSAECANCELLED
Language=Bulgarian
WSAECANCELLED - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10104
Severity=Success
Facility=System
SymbolicName=WSAEINVALIDPROCTABLE
Language=Bulgarian
WSAEINVALIDPROCTABLE - The procedure call table is invalid.
.

MessageId=10105
Severity=Success
Facility=System
SymbolicName=WSAEINVALIDPROVIDER
Language=Bulgarian
WSAEINVALIDPROVIDER - The requested service provider is invalid.
.

MessageId=10106
Severity=Success
Facility=System
SymbolicName=WSAEPROVIDERFAILEDINIT
Language=Bulgarian
WSAEPROVIDERFAILEDINIT - The requested service provider could not be loaded or initialized.
.

MessageId=10107
Severity=Success
Facility=System
SymbolicName=WSASYSCALLFAILURE
Language=Bulgarian
WSASYSCALLFAILURE - A system call that should never fail has failed.
.

MessageId=10108
Severity=Success
Facility=System
SymbolicName=WSASERVICE_NOT_FOUND
Language=Bulgarian
WSASERVICE_NOT_FOUND - No such service is known. The service cannot be found in the specified name space.
.

MessageId=10109
Severity=Success
Facility=System
SymbolicName=WSATYPE_NOT_FOUND
Language=Bulgarian
WSATYPE_NOT_FOUND - The specified class was not found.
.

MessageId=10110
Severity=Success
Facility=System
SymbolicName=WSA_E_NO_MORE
Language=Bulgarian
WSA_E_NO_MORE - No more results can be returned by WSALookupServiceNext.
.

MessageId=10111
Severity=Success
Facility=System
SymbolicName=WSA_E_CANCELLED
Language=Bulgarian
WSA_E_CANCELLED - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10112
Severity=Success
Facility=System
SymbolicName=WSAEREFUSED
Language=Bulgarian
WSAEREFUSED - A database query failed because it was actively refused.
.

MessageId=11001
Severity=Success
Facility=System
SymbolicName=WSAHOST_NOT_FOUND
Language=Bulgarian
WSAHOST_NOT_FOUND - No such host is known.
.

MessageId=11002
Severity=Success
Facility=System
SymbolicName=WSATRY_AGAIN
Language=Bulgarian
WSATRY_AGAIN - This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.

MessageId=11003
Severity=Success
Facility=System
SymbolicName=WSANO_RECOVERY
Language=Bulgarian
WSANO_RECOVERY - A non-recoverable error occurred during a database lookup.
.

MessageId=11004
Severity=Success
Facility=System
SymbolicName=WSANO_DATA
Language=Bulgarian
WSANO_DATA - The requested name is valid, but no data of the requested type was found.
.

MessageId=11005
Severity=Success
Facility=System
SymbolicName=WSA_QOS_RECEIVERS
Language=Bulgarian
WSA_QOS_RECEIVERS - At least one reserve has arrived.
.

MessageId=11006
Severity=Success
Facility=System
SymbolicName=WSA_QOS_SENDERS
Language=Bulgarian
WSA_QOS_SENDERS - At least one path has arrived.
.

MessageId=11007
Severity=Success
Facility=System
SymbolicName=WSA_QOS_NO_SENDERS
Language=Bulgarian
WSA_QOS_NO_SENDERS - There are no senders.
.

MessageId=11008
Severity=Success
Facility=System
SymbolicName=WSA_QOS_NO_RECEIVERS
Language=Bulgarian
WSA_QOS_NO_RECEIVERS - There are no receivers.
.

MessageId=11009
Severity=Success
Facility=System
SymbolicName=WSA_QOS_REQUEST_CONFIRMED
Language=Bulgarian
WSA_QOS_REQUEST_CONFIRMED - Reserve has been confirmed.
.

MessageId=11010
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ADMISSION_FAILURE
Language=Bulgarian
WSA_QOS_ADMISSION_FAILURE - Error due to lack of resources.
.

MessageId=11011
Severity=Success
Facility=System
SymbolicName=WSA_QOS_POLICY_FAILURE
Language=Bulgarian
WSA_QOS_POLICY_FAILURE - Rejected for administrative reasons - bad credentials.
.

MessageId=11012
Severity=Success
Facility=System
SymbolicName=WSA_QOS_BAD_STYLE
Language=Bulgarian
WSA_QOS_BAD_STYLE - Unknown or conflicting style.
.

MessageId=11013
Severity=Success
Facility=System
SymbolicName=WSA_QOS_BAD_OBJECT
Language=Bulgarian
WSA_QOS_BAD_OBJECT - Problem with some part of the filterspec or providerspecific buffer in general.
.

MessageId=11014
Severity=Success
Facility=System
SymbolicName=WSA_QOS_TRAFFIC_CTRL_ERROR
Language=Bulgarian
WSA_QOS_TRAFFIC_CTRL_ERROR - Problem with some part of the flowspec.
.

MessageId=11015
Severity=Success
Facility=System
SymbolicName=WSA_QOS_GENERIC_ERROR
Language=Bulgarian
WSA_QOS_GENERIC_ERROR - General QOS error.
.

MessageId=11016
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESERVICETYPE
Language=Bulgarian
WSA_QOS_ESERVICETYPE - An invalid or unrecognized service type was found in the flowspec.
.

MessageId=11017
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWSPEC
Language=Bulgarian
WSA_QOS_EFLOWSPEC - An invalid or inconsistent flowspec was found in the QOS structure.
.

MessageId=11018
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPROVSPECBUF
Language=Bulgarian
WSA_QOS_EPROVSPECBUF - Invalid QOS provider-specific buffer.
.

MessageId=11019
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERSTYLE
Language=Bulgarian
WSA_QOS_EFILTERSTYLE - An invalid QOS filter style was used.
.

MessageId=11020
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERTYPE
Language=Bulgarian
WSA_QOS_EFILTERTYPE - An invalid QOS filter type was used.
.

MessageId=11021
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERCOUNT
Language=Bulgarian
WSA_QOS_EFILTERCOUNT - An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.

MessageId=11022
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EOBJLENGTH
Language=Bulgarian
WSA_QOS_EOBJLENGTH - An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.

MessageId=11023
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWCOUNT
Language=Bulgarian
WSA_QOS_EFLOWCOUNT - An incorrect number of flow descriptors was specified in the QOS structure.
.

MessageId=11024
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EUNKNOWNPSOBJ
Language=Bulgarian
WSA_QOS_EUNKNOWNPSOBJ - An unrecognized object was found in the QOS provider-specific buffer.
.

MessageId=11025
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPOLICYOBJ
Language=Bulgarian
WSA_QOS_EPOLICYOBJ - An invalid policy object was found in the QOS provider-specific buffer.
.

MessageId=11026
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWDESC
Language=Bulgarian
WSA_QOS_EFLOWDESC - An invalid QOS flow descriptor was found in the flow descriptor list.
.

MessageId=11027
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPSFLOWSPEC
Language=Bulgarian
WSA_QOS_EPSFLOWSPEC - An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.
.

MessageId=11028
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPSFILTERSPEC
Language=Bulgarian
WSA_QOS_EPSFILTERSPEC - An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.

MessageId=11029
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESDMODEOBJ
Language=Bulgarian
WSA_QOS_ESDMODEOBJ - An invalid shape discard mode object was found in the QOS provider-specific buffer.
.

MessageId=11030
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESHAPERATEOBJ
Language=Bulgarian
WSA_QOS_ESHAPERATEOBJ - An invalid shaping rate object was found in the QOS provider-specific buffer.
.

MessageId=11031
Severity=Success
Facility=System
SymbolicName=WSA_QOS_RESERVED_PETYPE
Language=Bulgarian
WSA_QOS_RESERVED_PETYPE - A reserved policy element was found in the QOS provider-specific buffer.
.

MessageId=12000
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_IO_COMPLETE
Language=Bulgarian
ERROR_FLT_IO_COMPLETE - The IO was completed by a filter.
.

MessageId=12001
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_BUFFER_TOO_SMALL
Language=Bulgarian
ERROR_FLT_BUFFER_TOO_SMALL - The buffer is too small to contain the entry. No information has been written to the buffer.
.

MessageId=12002
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_HANDLER_DEFINED
Language=Bulgarian
ERROR_FLT_NO_HANDLER_DEFINED - A handler was not defined by the filter for this operation.
.

MessageId=12003
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CONTEXT_ALREADY_DEFINED
Language=Bulgarian
ERROR_FLT_CONTEXT_ALREADY_DEFINED - A context is already defined for this object.
.

MessageId=12004
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_ASYNCHRONOUS_REQUEST
Language=Bulgarian
ERROR_FLT_INVALID_ASYNCHRONOUS_REQUEST - Asynchronous requests are not valid for this operation.
.

MessageId=12005
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DISALLOW_FAST_IO
Language=Bulgarian
ERROR_FLT_DISALLOW_FAST_IO - Disallow the Fast IO path for this operation.
.

MessageId=12006
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_NAME_REQUEST
Language=Bulgarian
ERROR_FLT_INVALID_NAME_REQUEST - An invalid name request was made. The name requested cannot be retrieved at this time.
.

MessageId=12007
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NOT_SAFE_TO_POST_OPERATION
Language=Bulgarian
ERROR_FLT_NOT_SAFE_TO_POST_OPERATION - Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.
.

MessageId=12008
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NOT_INITIALIZED
Language=Bulgarian
ERROR_FLT_NOT_INITIALIZED - The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver.
.

MessageId=12009
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_FILTER_NOT_READY
Language=Bulgarian
ERROR_FLT_FILTER_NOT_READY - The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).
.

MessageId=12010
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_POST_OPERATION_CLEANUP
Language=Bulgarian
ERROR_FLT_POST_OPERATION_CLEANUP - The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.
.

MessageId=12011
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INTERNAL_ERROR
Language=Bulgarian
ERROR_FLT_INTERNAL_ERROR - The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback.
.

MessageId=12012
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DELETING_OBJECT
Language=Bulgarian
ERROR_FLT_DELETING_OBJECT - The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.
.

MessageId=12013
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_MUST_BE_NONPAGED_POOL
Language=Bulgarian
ERROR_FLT_MUST_BE_NONPAGED_POOL - Non-paged pool must be used for this type of context.
.

MessageId=12014
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DUPLICATE_ENTRY
Language=Bulgarian
ERROR_FLT_DUPLICATE_ENTRY - A duplicate handler definition has been provided for an operation.
.

MessageId=12015
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CBDQ_DISABLED
Language=Bulgarian
ERROR_FLT_CBDQ_DISABLED - The callback data queue has been disabled.
.

MessageId=12016
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DO_NOT_ATTACH
Language=Bulgarian
ERROR_FLT_DO_NOT_ATTACH - Do not attach the filter to the volume at this time.
.

MessageId=12017
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DO_NOT_DETACH
Language=Bulgarian
ERROR_FLT_DO_NOT_DETACH - Do not detach the filter from the volume at this time.
.

MessageId=12018
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_ALTITUDE_COLLISION
Language=Bulgarian
ERROR_FLT_INSTANCE_ALTITUDE_COLLISION - An instance already exists at this altitude on the volume specified.
.

MessageId=12019
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_NAME_COLLISION
Language=Bulgarian
ERROR_FLT_INSTANCE_NAME_COLLISION - An instance already exists with this name on the volume specified.
.

MessageId=12020
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_FILTER_NOT_FOUND
Language=Bulgarian
ERROR_FLT_FILTER_NOT_FOUND - The system could not find the filter specified.
.

MessageId=12021
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_VOLUME_NOT_FOUND
Language=Bulgarian
ERROR_FLT_VOLUME_NOT_FOUND - The system could not find the volume specified.
.

MessageId=12022
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_NOT_FOUND
Language=Bulgarian
ERROR_FLT_INSTANCE_NOT_FOUND - The system could not find the instance specified.
.

MessageId=12023
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CONTEXT_ALLOCATION_NOT_FOUND
Language=Bulgarian
ERROR_FLT_CONTEXT_ALLOCATION_NOT_FOUND - No registered context allocation definition was found for the given request.
.

MessageId=12024
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_CONTEXT_REGISTRATION
Language=Bulgarian
ERROR_FLT_INVALID_CONTEXT_REGISTRATION - An invalid parameter was specified during context registration.
.

MessageId=12025
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NAME_CACHE_MISS
Language=Bulgarian
ERROR_FLT_NAME_CACHE_MISS - The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.
.

MessageId=12026
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_DEVICE_OBJECT
Language=Bulgarian
ERROR_FLT_NO_DEVICE_OBJECT - The requested device object does not exist for the given volume.
.

MessageId=12027
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_VOLUME_ALREADY_MOUNTED
Language=Bulgarian
ERROR_FLT_VOLUME_ALREADY_MOUNTED - The specified volume is already mounted.
.

MessageId=12028
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_WAITER_FOR_REPLY
Language=Bulgarian
ERROR_FLT_NO_WAITER_FOR_REPLY - No waiter is present for the filter's reply to this message.
.

MessageId=13000
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_EXISTS
Language=Bulgarian
ERROR_IPSEC_QM_POLICY_EXISTS - The specified quick mode policy already exists.
.

MessageId=13001
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_QM_POLICY_NOT_FOUND - The specified quick mode policy was not found.
.

MessageId=13002
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_IN_USE
Language=Bulgarian
ERROR_IPSEC_QM_POLICY_IN_USE - The specified quick mode policy is being used.
.

MessageId=13003
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_EXISTS
Language=Bulgarian
ERROR_IPSEC_MM_POLICY_EXISTS - The specified main mode policy already exists.
.

MessageId=13004
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_MM_POLICY_NOT_FOUND - The specified main mode policy was not found.
.

MessageId=13005
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_IN_USE
Language=Bulgarian
ERROR_IPSEC_MM_POLICY_IN_USE - The specified main mode policy is being used.
.

MessageId=13006
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_EXISTS
Language=Bulgarian
ERROR_IPSEC_MM_FILTER_EXISTS - The specified main mode filter already exists.
.

MessageId=13007
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_MM_FILTER_NOT_FOUND - The specified main mode filter was not found.
.

MessageId=13008
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_EXISTS
Language=Bulgarian
ERROR_IPSEC_TRANSPORT_FILTER_EXISTS - The specified transport mode filter already exists.
.

MessageId=13009
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND - The specified transport mode filter does not exist.
.

MessageId=13010
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_EXISTS
Language=Bulgarian
ERROR_IPSEC_MM_AUTH_EXISTS - The specified main mode authentication list exists.
.

MessageId=13011
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_MM_AUTH_NOT_FOUND - The specified main mode authentication list was not found.
.

MessageId=13012
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_IN_USE
Language=Bulgarian
ERROR_IPSEC_MM_AUTH_IN_USE - The specified quick mode policy is being used.
.

MessageId=13013
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND - The specified main mode policy was not found.
.

MessageId=13014
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND - The specified quick mode policy was not found.
.

MessageId=13015
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND - The manifest file contains one or more syntax errors.
.

MessageId=13016
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_EXISTS
Language=Bulgarian
ERROR_IPSEC_TUNNEL_FILTER_EXISTS - The application attempted to activate a disabled activation context.
.

MessageId=13017
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND
Language=Bulgarian
ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND - The requested lookup key was not found in any active activation context.
.

MessageId=13018
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_MM_FILTER_PENDING_DELETION - The Main Mode filter is pending deletion.
.

MessageId=13019
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION - The transport filter is pending deletion.
.

MessageId=13020
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION - The tunnel filter is pending deletion.
.

MessageId=13021
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_MM_POLICY_PENDING_DELETION - The Main Mode policy is pending deletion.
.

MessageId=13022
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_MM_AUTH_PENDING_DELETION - The Main Mode authentication bundle is pending deletion.
.

MessageId=13023
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_PENDING_DELETION
Language=Bulgarian
ERROR_IPSEC_QM_POLICY_PENDING_DELETION - The Quick Mode policy is pending deletion.
.

MessageId=13024
Severity=Success
Facility=System
SymbolicName=WARNING_IPSEC_MM_POLICY_PRUNED
Language=Bulgarian
WARNING_IPSEC_MM_POLICY_PRUNED - The Main Mode policy was successfully added, but some of the requested offers are not supported.
.

MessageId=13025
Severity=Success
Facility=System
SymbolicName=WARNING_IPSEC_QM_POLICY_PRUNED
Language=Bulgarian
WARNING_IPSEC_QM_POLICY_PRUNED - The Quick Mode policy was successfully added, but some of the requested offers are not supported.
.

MessageId=13801
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_AUTH_FAIL
Language=Bulgarian
ERROR_IPSEC_IKE_AUTH_FAIL - IKE authentication credentials are unacceptable.
.

MessageId=13802
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ATTRIB_FAIL
Language=Bulgarian
ERROR_IPSEC_IKE_ATTRIB_FAIL - IKE security attributes are unacceptable.
.

MessageId=13803
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_PENDING
Language=Bulgarian
ERROR_IPSEC_IKE_NEGOTIATION_PENDING - IKE Negotiation in progress.
.

MessageId=13804
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR
Language=Bulgarian
ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR - General processing error.
.

MessageId=13805
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_TIMED_OUT
Language=Bulgarian
ERROR_IPSEC_IKE_TIMED_OUT - Negotiation timed out.
.

MessageId=13806
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_CERT
Language=Bulgarian
ERROR_IPSEC_IKE_NO_CERT - IKE failed to find valid machine certificate.
.

MessageId=13807
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SA_DELETED
Language=Bulgarian
ERROR_IPSEC_IKE_SA_DELETED - IKE SA deleted by peer before establishment completed.
.

MessageId=13808
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SA_REAPED
Language=Bulgarian
ERROR_IPSEC_IKE_SA_REAPED - IKE SA deleted before establishment completed.
.

MessageId=13809
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_ACQUIRE_DROP
Language=Bulgarian
ERROR_IPSEC_IKE_MM_ACQUIRE_DROP - Negotiation request sat in Queue too long.
.

MessageId=13810
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QM_ACQUIRE_DROP
Language=Bulgarian
ERROR_IPSEC_IKE_QM_ACQUIRE_DROP - Negotiation request sat in Queue too long.
.

MessageId=13811
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_MM
Language=Bulgarian
ERROR_IPSEC_IKE_QUEUE_DROP_MM - Negotiation request sat in Queue too long.
.

MessageId=13812
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM
Language=Bulgarian
ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM - Negotiation request sat in Queue too long.
.

MessageId=13813
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DROP_NO_RESPONSE
Language=Bulgarian
ERROR_IPSEC_IKE_DROP_NO_RESPONSE - No response from peer.
.

MessageId=13814
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_DELAY_DROP
Language=Bulgarian
ERROR_IPSEC_IKE_MM_DELAY_DROP - Negotiation took too long.
.

MessageId=13815
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QM_DELAY_DROP
Language=Bulgarian
ERROR_IPSEC_IKE_QM_DELAY_DROP - Negotiation took too long.
.

MessageId=13816
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ERROR
Language=Bulgarian
ERROR_IPSEC_IKE_ERROR - Unknown error occurred.
.

MessageId=13817
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_CRL_FAILED
Language=Bulgarian
ERROR_IPSEC_IKE_CRL_FAILED - Certificate Revocation Check failed.
.

MessageId=13818
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_KEY_USAGE
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_KEY_USAGE - Invalid certificate key usage.
.

MessageId=13819
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_TYPE
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_CERT_TYPE - Invalid certificate type.
.

MessageId=13820
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PRIVATE_KEY
Language=Bulgarian
ERROR_IPSEC_IKE_NO_PRIVATE_KEY - No private key associated with machine certificate.
.

MessageId=13822
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DH_FAIL
Language=Bulgarian
ERROR_IPSEC_IKE_DH_FAIL - Failure in Diffie-Hellman computation.
.

MessageId=13824
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HEADER
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_HEADER - Invalid header.
.

MessageId=13825
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_POLICY
Language=Bulgarian
ERROR_IPSEC_IKE_NO_POLICY - No policy configured.
.

MessageId=13826
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SIGNATURE
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_SIGNATURE - Failed to verify signature.
.

MessageId=13827
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_KERBEROS_ERROR
Language=Bulgarian
ERROR_IPSEC_IKE_KERBEROS_ERROR - Failed to authenticate using Kerberos.
.

MessageId=13828
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PUBLIC_KEY
Language=Bulgarian
ERROR_IPSEC_IKE_NO_PUBLIC_KEY - Peer's certificate did not have a public key.
.

MessageId=13829
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR - Error processing error payload.
.

MessageId=13830
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SA
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_SA - Error processing SA payload.
.

MessageId=13831
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_PROP
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_PROP - Error processing Proposal payload.
.

MessageId=13832
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_TRANS
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_TRANS - Error processing Transform payload.
.

MessageId=13833
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_KE
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_KE - Error processing KE payload.
.

MessageId=13834
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_ID
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_ID - Error processing ID payload.
.

MessageId=13835
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_CERT - Error processing Cert payload.
.

MessageId=13836
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ - Error processing Certificate Request payload.
.

MessageId=13837
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_HASH
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_HASH - Error processing Hash payload.
.

MessageId=13838
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SIG
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_SIG - Error processing Signature payload.
.

MessageId=13839
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NONCE
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_NONCE - Error processing Nonce payload.
.

MessageId=13840
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY - Error processing Notify payload.
.

MessageId=13841
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_DELETE
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_DELETE - Error processing Delete Payload.
.

MessageId=13842
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR
Language=Bulgarian
ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR - Error processing VendorId payload.
.

MessageId=13843
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_PAYLOAD
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_PAYLOAD - Invalid payload received.
.

MessageId=13844
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_LOAD_SOFT_SA
Language=Bulgarian
ERROR_IPSEC_IKE_LOAD_SOFT_SA - Soft SA loaded.
.

MessageId=13845
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN
Language=Bulgarian
ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN - Soft SA torn down.
.

MessageId=13846
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_COOKIE
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_COOKIE - Invalid cookie received..
.

MessageId=13847
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PEER_CERT
Language=Bulgarian
ERROR_IPSEC_IKE_NO_PEER_CERT - Peer failed to send valid machine certificate.
.

MessageId=13848
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PEER_CRL_FAILED
Language=Bulgarian
ERROR_IPSEC_IKE_PEER_CRL_FAILED - Certification Revocation check of peer's certificate failed.
.

MessageId=13849
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_POLICY_CHANGE
Language=Bulgarian
ERROR_IPSEC_IKE_POLICY_CHANGE - New policy invalidated SAs formed with old policy.
.

MessageId=13850
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_MM_POLICY
Language=Bulgarian
ERROR_IPSEC_IKE_NO_MM_POLICY - There is no available Main Mode IKE policy.
.

MessageId=13851
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NOTCBPRIV
Language=Bulgarian
ERROR_IPSEC_IKE_NOTCBPRIV - Failed to enabled TCB privilege.
.

MessageId=13852
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SECLOADFAIL
Language=Bulgarian
ERROR_IPSEC_IKE_SECLOADFAIL - Failed to load SECURITY.DLL.
.

MessageId=13853
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_FAILSSPINIT
Language=Bulgarian
ERROR_IPSEC_IKE_FAILSSPINIT - Failed to obtain security function table dispatch address from SSPI.
.

MessageId=13854
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_FAILQUERYSSP
Language=Bulgarian
ERROR_IPSEC_IKE_FAILQUERYSSP - Failed to query Kerberos package to obtain max token size.
.

MessageId=13855
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SRVACQFAIL
Language=Bulgarian
ERROR_IPSEC_IKE_SRVACQFAIL - Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup.
.

MessageId=13856
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SRVQUERYCRED
Language=Bulgarian
ERROR_IPSEC_IKE_SRVQUERYCRED - Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.

MessageId=13857
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_GETSPIFAIL
Language=Bulgarian
ERROR_IPSEC_IKE_GETSPIFAIL - Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters.
.

MessageId=13858
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_FILTER
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_FILTER - Given filter is invalid.
.

MessageId=13859
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_OUT_OF_MEMORY
Language=Bulgarian
ERROR_IPSEC_IKE_OUT_OF_MEMORY - Memory allocation failed.
.

MessageId=13860
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED
Language=Bulgarian
ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED - Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine.
.

MessageId=13861
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_POLICY
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_POLICY - Invalid policy.
.

MessageId=13862
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_UNKNOWN_DOI
Language=Bulgarian
ERROR_IPSEC_IKE_UNKNOWN_DOI - Invalid DOI.
.

MessageId=13863
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SITUATION
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_SITUATION - Invalid situation.
.

MessageId=13864
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DH_FAILURE
Language=Bulgarian
ERROR_IPSEC_IKE_DH_FAILURE - Diffie-Hellman failure.
.

MessageId=13865
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_GROUP
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_GROUP - Invalid Diffie-Hellman group.
.

MessageId=13866
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ENCRYPT
Language=Bulgarian
ERROR_IPSEC_IKE_ENCRYPT - Error encrypting payload.
.

MessageId=13867
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DECRYPT
Language=Bulgarian
ERROR_IPSEC_IKE_DECRYPT - Error decrypting payload.
.

MessageId=13868
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_POLICY_MATCH
Language=Bulgarian
ERROR_IPSEC_IKE_POLICY_MATCH - Policy match error.
.

MessageId=13869
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_UNSUPPORTED_ID
Language=Bulgarian
ERROR_IPSEC_IKE_UNSUPPORTED_ID - Unsupported ID.
.

MessageId=13870
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_HASH - Hash verification failed.
.

MessageId=13871
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_ALG
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_HASH_ALG - Invalid hash algorithm.
.

MessageId=13872
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_SIZE
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_HASH_SIZE - Invalid hash size.
.

MessageId=13873
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG - Invalid encryption algorithm.
.

MessageId=13874
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_AUTH_ALG
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_AUTH_ALG - Invalid authentication algorithm.
.

MessageId=13875
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SIG
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_SIG - Invalid certificate signature.
.

MessageId=13876
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_LOAD_FAILED
Language=Bulgarian
ERROR_IPSEC_IKE_LOAD_FAILED - Load failed.
.

MessageId=13877
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_RPC_DELETE
Language=Bulgarian
ERROR_IPSEC_IKE_RPC_DELETE - Deleted via RPC call.
.

MessageId=13878
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_BENIGN_REINIT
Language=Bulgarian
ERROR_IPSEC_IKE_BENIGN_REINIT - Temporary state created to perform reinit. This is not a real failure.
.

MessageId=13879
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY - The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine.
.

MessageId=13881
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN
Language=Bulgarian
ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN - Key length in certificate is too small for configured security requirements.
.

MessageId=13882
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_LIMIT
Language=Bulgarian
ERROR_IPSEC_IKE_MM_LIMIT - Max number of established MM SAs to peer exceeded.
.

MessageId=13883
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_DISABLED
Language=Bulgarian
ERROR_IPSEC_IKE_NEGOTIATION_DISABLED - IKE received a policy that disables negotiation.
.

MessageId=13884
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEG_STATUS_END
Language=Bulgarian
ERROR_IPSEC_IKE_NEG_STATUS_END - ERROR_IPSEC_IKE_NEG_STATUS_END
.

MessageId=14000
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_SECTION_NOT_FOUND
Language=Bulgarian
ERROR_SXS_SECTION_NOT_FOUND - The requested section was not present in the activation context.
.

MessageId=14001
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CANT_GEN_ACTCTX
Language=Bulgarian
ERROR_SXS_CANT_GEN_ACTCTX - This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.

MessageId=14002
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ACTCTXDATA_FORMAT
Language=Bulgarian
ERROR_SXS_INVALID_ACTCTXDATA_FORMAT - The application binding data format is invalid.
.

MessageId=14003
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ASSEMBLY_NOT_FOUND
Language=Bulgarian
ERROR_SXS_ASSEMBLY_NOT_FOUND - The referenced assembly is not installed on your system.
.

MessageId=14004
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_FORMAT_ERROR
Language=Bulgarian
ERROR_SXS_MANIFEST_FORMAT_ERROR - The manifest file does not begin with the required tag and format information.
.

MessageId=14005
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_PARSE_ERROR
Language=Bulgarian
ERROR_SXS_MANIFEST_PARSE_ERROR - The manifest file contains one or more syntax errors.
.

MessageId=14006
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ACTIVATION_CONTEXT_DISABLED
Language=Bulgarian
ERROR_SXS_ACTIVATION_CONTEXT_DISABLED - The application attempted to activate a disabled activation context.
.

MessageId=14007
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_KEY_NOT_FOUND
Language=Bulgarian
ERROR_SXS_KEY_NOT_FOUND - The requested lookup key was not found in any active activation context.
.

MessageId=14008
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_VERSION_CONFLICT
Language=Bulgarian
ERROR_SXS_VERSION_CONFLICT - A component version required by the application conflicts with another component version already active.
.

MessageId=14009
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_WRONG_SECTION_TYPE
Language=Bulgarian
ERROR_SXS_WRONG_SECTION_TYPE - The type requested activation context section does not match the query API used.
.

MessageId=14010
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_THREAD_QUERIES_DISABLED
Language=Bulgarian
ERROR_SXS_THREAD_QUERIES_DISABLED - Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.

MessageId=14011
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET
Language=Bulgarian
ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET - An attempt to set the process default activation context failed because the process default activation context was already set.
.

MessageId=14012
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNKNOWN_ENCODING_GROUP
Language=Bulgarian
ERROR_SXS_UNKNOWN_ENCODING_GROUP - The encoding group identifier specified is not recognized.
.

MessageId=14013
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNKNOWN_ENCODING
Language=Bulgarian
ERROR_SXS_UNKNOWN_ENCODING - The encoding requested is not recognized.
.

MessageId=14014
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_XML_NAMESPACE_URI
Language=Bulgarian
ERROR_SXS_INVALID_XML_NAMESPACE_URI - The manifest contains a reference to an invalid URI.
.

MessageId=14015
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=Bulgarian
ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED - The application manifest contains a reference to a dependent assembly which is not installed.
.

MessageId=14016
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=Bulgarian
ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED - The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed.
.

MessageId=14017
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=Bulgarian
ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE - The manifest contains an attribute for the assembly identity which is not valid.
.

MessageId=14018
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE
Language=Bulgarian
ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE - The manifest is missing the required default namespace specification on the assembly element.
.

MessageId=14019
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE
Language=Bulgarian
ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE - The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\".
.

MessageId=14020
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT
Language=Bulgarian
ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT - The private manifest probe has crossed the reparse-point-associated path.
.

MessageId=14021
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_DLL_NAME
Language=Bulgarian
ERROR_SXS_DUPLICATE_DLL_NAME - Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.

MessageId=14022
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME
Language=Bulgarian
ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME - Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.

MessageId=14023
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_CLSID
Language=Bulgarian
ERROR_SXS_DUPLICATE_CLSID - Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.

MessageId=14024
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_IID
Language=Bulgarian
ERROR_SXS_DUPLICATE_IID - Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.

MessageId=14025
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_TLBID
Language=Bulgarian
ERROR_SXS_DUPLICATE_TLBID - Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.

MessageId=14026
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_PROGID
Language=Bulgarian
ERROR_SXS_DUPLICATE_PROGID - Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.

MessageId=14027
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_ASSEMBLY_NAME
Language=Bulgarian
ERROR_SXS_DUPLICATE_ASSEMBLY_NAME - Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.

MessageId=14028
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_FILE_HASH_MISMATCH
Language=Bulgarian
ERROR_SXS_FILE_HASH_MISMATCH - A component's file does not match the verification information present in the component manifest.
.

MessageId=14029
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_POLICY_PARSE_ERROR
Language=Bulgarian
ERROR_SXS_POLICY_PARSE_ERROR - The policy manifest contains one or more syntax errors.
.

MessageId=14030
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGQUOTE
Language=Bulgarian
ERROR_SXS_XML_E_MISSINGQUOTE - Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.

MessageId=14031
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_COMMENTSYNTAX
Language=Bulgarian
ERROR_SXS_XML_E_COMMENTSYNTAX - Manifest Parse Error : Incorrect syntax was used in a comment.
.

MessageId=14032
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADSTARTNAMECHAR
Language=Bulgarian
ERROR_SXS_XML_E_BADSTARTNAMECHAR - Manifest Parse Error : A name was started with an invalid character.
.

MessageId=14033
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADNAMECHAR
Language=Bulgarian
ERROR_SXS_XML_E_BADNAMECHAR - Manifest Parse Error : A name contained an invalid character.
.

MessageId=14034
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADCHARINSTRING
Language=Bulgarian
ERROR_SXS_XML_E_BADCHARINSTRING - Manifest Parse Error : A string literal contained an invalid character.
.

MessageId=14035
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_XMLDECLSYNTAX
Language=Bulgarian
ERROR_SXS_XML_E_XMLDECLSYNTAX - Manifest Parse Error : Invalid syntax for an XML declaration.
.

MessageId=14036
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADCHARDATA
Language=Bulgarian
ERROR_SXS_XML_E_BADCHARDATA - Manifest Parse Error : An invalid character was found in text content.
.

MessageId=14037
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGWHITESPACE
Language=Bulgarian
ERROR_SXS_XML_E_MISSINGWHITESPACE - Manifest Parse Error : Required white space was missing.
.

MessageId=14038
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_EXPECTINGTAGEND
Language=Bulgarian
ERROR_SXS_XML_E_EXPECTINGTAGEND - Manifest Parse Error : The character '>' was expected.
.

MessageId=14039
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGSEMICOLON
Language=Bulgarian
ERROR_SXS_XML_E_MISSINGSEMICOLON - Manifest Parse Error : A semi colon character was expected.
.

MessageId=14040
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNBALANCEDPAREN
Language=Bulgarian
ERROR_SXS_XML_E_UNBALANCEDPAREN - Manifest Parse Error : Unbalanced parentheses.
.

MessageId=14041
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INTERNALERROR
Language=Bulgarian
ERROR_SXS_XML_E_INTERNALERROR - Manifest Parse Error : Internal error.
.

MessageId=14042
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE
Language=Bulgarian
ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE - Manifest Parse Error : White space is not allowed at this location.
.

MessageId=14043
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INCOMPLETE_ENCODING
Language=Bulgarian
ERROR_SXS_XML_E_INCOMPLETE_ENCODING - Manifest Parse Error : End of file reached in invalid state for current encoding.
.

MessageId=14044
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSING_PAREN
Language=Bulgarian
ERROR_SXS_XML_E_MISSING_PAREN - Manifest Parse Error : Missing parenthesis.
.

MessageId=14045
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE
Language=Bulgarian
ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE - Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.

MessageId=14046
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MULTIPLE_COLONS
Language=Bulgarian
ERROR_SXS_XML_E_MULTIPLE_COLONS - Manifest Parse Error : Multiple colons are not allowed in a name.
.

MessageId=14047
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_DECIMAL
Language=Bulgarian
ERROR_SXS_XML_E_INVALID_DECIMAL - Manifest Parse Error : Invalid character for decimal digit.
.

MessageId=14048
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_HEXIDECIMAL
Language=Bulgarian
ERROR_SXS_XML_E_INVALID_HEXIDECIMAL - Manifest Parse Error : Invalid character for hexadecimal digit.
.

MessageId=14049
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_UNICODE
Language=Bulgarian
ERROR_SXS_XML_E_INVALID_UNICODE - Manifest Parse Error : Invalid Unicode character value for this platform.
.

MessageId=14050
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK
Language=Bulgarian
ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK - Manifest Parse Error : Expecting white space or '?'.
.

MessageId=14051
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDENDTAG
Language=Bulgarian
ERROR_SXS_XML_E_UNEXPECTEDENDTAG - Manifest Parse Error : End tag was not expected at this location.
.

MessageId=14052
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDTAG
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDTAG - Manifest Parse Error : The following tags were not closed: %1.
.

MessageId=14053
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_DUPLICATEATTRIBUTE
Language=Bulgarian
ERROR_SXS_XML_E_DUPLICATEATTRIBUTE - Manifest Parse Error : Duplicate attribute.
.

MessageId=14054
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MULTIPLEROOTS
Language=Bulgarian
ERROR_SXS_XML_E_MULTIPLEROOTS - Manifest Parse Error : Only one top level element is allowed in an XML document.
.

MessageId=14055
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDATROOTLEVEL
Language=Bulgarian
ERROR_SXS_XML_E_INVALIDATROOTLEVEL - Manifest Parse Error : Invalid at the top level of the document.
.

MessageId=14056
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADXMLDECL
Language=Bulgarian
ERROR_SXS_XML_E_BADXMLDECL - Manifest Parse Error : Invalid XML declaration.
.

MessageId=14057
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGROOT
Language=Bulgarian
ERROR_SXS_XML_E_MISSINGROOT - Manifest Parse Error : XML document must have a top level element.
.

MessageId=14058
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDEOF
Language=Bulgarian
ERROR_SXS_XML_E_UNEXPECTEDEOF - Manifest Parse Error : Unexpected end of file.
.

MessageId=14059
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADPEREFINSUBSET
Language=Bulgarian
ERROR_SXS_XML_E_BADPEREFINSUBSET - Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.

MessageId=14060
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTARTTAG
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDSTARTTAG - Manifest Parse Error : Element was not closed.
.

MessageId=14061
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDENDTAG
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDENDTAG - Manifest Parse Error : End element was missing the character '>'.
.

MessageId=14062
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTRING
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDSTRING - Manifest Parse Error : A string literal was not closed.
.

MessageId=14063
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCOMMENT
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDCOMMENT - Manifest Parse Error : A comment was not closed.
.

MessageId=14064
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDDECL
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDDECL - Manifest Parse Error : A declaration was not closed.
.

MessageId=14065
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCDATA
Language=Bulgarian
ERROR_SXS_XML_E_UNCLOSEDCDATA - Manifest Parse Error : A CDATA section was not closed.
.

MessageId=14066
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_RESERVEDNAMESPACE
Language=Bulgarian
ERROR_SXS_XML_E_RESERVEDNAMESPACE - Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\".
.

MessageId=14067
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDENCODING
Language=Bulgarian
ERROR_SXS_XML_E_INVALIDENCODING - Manifest Parse Error : System does not support the specified encoding.
.

MessageId=14068
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDSWITCH
Language=Bulgarian
ERROR_SXS_XML_E_INVALIDSWITCH - Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.

MessageId=14069
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADXMLCASE
Language=Bulgarian
ERROR_SXS_XML_E_BADXMLCASE - Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.

MessageId=14070
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_STANDALONE
Language=Bulgarian
ERROR_SXS_XML_E_INVALID_STANDALONE - Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.

MessageId=14071
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_STANDALONE
Language=Bulgarian
ERROR_SXS_XML_E_UNEXPECTED_STANDALONE - Manifest Parse Error : The standalone attribute cannot be used in external entities.
.

MessageId=14072
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_VERSION
Language=Bulgarian
ERROR_SXS_XML_E_INVALID_VERSION - Manifest Parse Error : Invalid version number.
.

MessageId=14073
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGEQUALS
Language=Bulgarian
ERROR_SXS_XML_E_MISSINGEQUALS - Manifest Parse Error : Missing equals sign between attribute and attribute value.
.

MessageId=14074
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_RECOVERY_FAILED
Language=Bulgarian
ERROR_SXS_PROTECTION_RECOVERY_FAILED - Assembly Protection Error: Unable to recover the specified assembly.
.

MessageId=14075
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT
Language=Bulgarian
ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT - Assembly Protection Error: The public key for an assembly was too short to be allowed.
.

MessageId=14076
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_CATALOG_NOT_VALID
Language=Bulgarian
ERROR_SXS_PROTECTION_CATALOG_NOT_VALID - Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest.
.

MessageId=14077
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNTRANSLATABLE_HRESULT
Language=Bulgarian
ERROR_SXS_UNTRANSLATABLE_HRESULT - An HRESULT could not be translated to a corresponding Win32 error code.
.

MessageId=14078
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING
Language=Bulgarian
ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING - Assembly Protection Error: The catalog for an assembly is missing.
.

MessageId=14079
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=Bulgarian
ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE - The supplied assembly identity is missing one or more attributes which must be present in this context.
.

MessageId=14080
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME
Language=Bulgarian
ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME - The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.

MessageId=14081
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ASSEMBLY_MISSING
Language=Bulgarian
ERROR_SXS_ASSEMBLY_MISSING - The referenced assembly could not be found.
.

MessageId=14082
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CORRUPT_ACTIVATION_STACK
Language=Bulgarian
ERROR_SXS_CORRUPT_ACTIVATION_STACK - The activation context activation stack for the running thread of execution is corrupt.
.

MessageId=14083
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CORRUPTION
Language=Bulgarian
ERROR_SXS_CORRUPTION - The application isolation metadata for this process or thread has become corrupt.
.

MessageId=14084
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_EARLY_DEACTIVATION
Language=Bulgarian
ERROR_SXS_EARLY_DEACTIVATION - The activation context being deactivated is not the most recently activated one.
.

MessageId=14085
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_DEACTIVATION
Language=Bulgarian
ERROR_SXS_INVALID_DEACTIVATION - The activation context being deactivated is not active for the current thread of execution.
.

MessageId=14086
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MULTIPLE_DEACTIVATION
Language=Bulgarian
ERROR_SXS_MULTIPLE_DEACTIVATION - The activation context being deactivated has already been deactivated.
.

MessageId=14087
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROCESS_TERMINATION_REQUESTED
Language=Bulgarian
ERROR_SXS_PROCESS_TERMINATION_REQUESTED - A component used by the isolation facility has requested to terminate the process.
.

MessageId=14088
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_RELEASE_ACTIVATION_CONTEXT
Language=Bulgarian
ERROR_SXS_RELEASE_ACTIVATION_CONTEXT - A kernel mode component is releasing a reference on an activation context.
.

MessageId=14089
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY
Language=Bulgarian
ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY - The activation context of system default assembly could not be generated.
.

MessageId=14090
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE
Language=Bulgarian
ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE - The value of an attribute in an identity is not within the legal range.
.

MessageId=14091
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME
Language=Bulgarian
ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME - The name of an attribute in an identity is not within the legal range.
.

MessageId=14092
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE
Language=Bulgarian
ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE - An identity contains two definitions for the same attribute.
.

MessageId=14093
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_IDENTITY_PARSE_ERROR
Language=Bulgarian
ERROR_SXS_IDENTITY_PARSE_ERROR - The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.
.

MessageId=15000
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_INVALID_CHANNEL_PATH
Language=Bulgarian
ERROR_EVT_INVALID_CHANNEL_PATH - The specified channel path is invalid. See extended error info for more details.
.

MessageId=15001
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_INVALID_QUERY
Language=Bulgarian
ERROR_EVT_INVALID_QUERY - The specified query is invalid. See extended error info for more details.
.

MessageId=15002
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_PUBLISHER_MANIFEST_NOT_FOUND
Language=Bulgarian
ERROR_EVT_PUBLISHER_MANIFEST_NOT_FOUND - The publisher did indicate they have a manifest/resource but a manifest/resource could not be found.
.

MessageId=15003
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_PUBLISHER_MANIFEST_NOT_SPECIFIED
Language=Bulgarian
ERROR_EVT_PUBLISHER_MANIFEST_NOT_SPECIFIED - The publisher does not have a manifest and is performing an operation which requires they have a manifest.
.

MessageId=15004
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_NO_REGISTERED_TEMPLATE
Language=Bulgarian
ERROR_EVT_NO_REGISTERED_TEMPLATE - There is no registered template for specified event id.
.

MessageId=15005
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_EVENT_CHANNEL_MISMATCH
Language=Bulgarian
ERROR_EVT_EVENT_CHANNEL_MISMATCH - The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to.
.

MessageId=15006
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_UNEXPECTED_VALUE_TYPE
Language=Bulgarian
ERROR_EVT_UNEXPECTED_VALUE_TYPE - The type of a specified substitution value does not match the type expected from the template definition.
.

MessageId=15007
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_UNEXPECTED_NUM_VALUES
Language=Bulgarian
ERROR_EVT_UNEXPECTED_NUM_VALUES - The number of specified substitution values does not match the number expected from the template definition.
.

MessageId=15008
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_CHANNEL_NOT_FOUND
Language=Bulgarian
ERROR_EVT_CHANNEL_NOT_FOUND - The specified channel could not be found. Check channel configuration.
.

MessageId=15009
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_MALFORMED_XML_TEXT
Language=Bulgarian
ERROR_EVT_MALFORMED_XML_TEXT - The specified xml text was not well-formed. See Extended Error for more details.
.

MessageId=15010
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_CHANNEL_PATH_TOO_GENERAL
Language=Bulgarian
ERROR_EVT_CHANNEL_PATH_TOO_GENERAL - The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance.
.


; Facility=WIN32

MessageId=0x000E
Severity=Warning
Facility=WIN32
SymbolicName=E_OUTOFMEMORY
Language=Bulgarian
E_OUTOFMEMORY - Out of memory
.

MessageId=0x0057
Severity=Warning
Facility=WIN32
SymbolicName=E_INVALIDARG
Language=Bulgarian
E_INVALIDARG - One or more arguments are invalid
.

MessageId=0x0006
Severity=Warning
Facility=WIN32
SymbolicName=E_HANDLE
Language=Bulgarian
E_POINTER - Invalid handle
.

MessageId=0x0005
Severity=Warning
Facility=WIN32
SymbolicName=E_ACCESSDENIED
Language=Bulgarian
E_ACCESSDENIED - WIN32 access denied error
.


; Facility=ITF

MessageId=0x0000
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_OLEVERB
Language=Bulgarian
OLE_E_OLEVERB - Invalid OLEVERB structure
.

MessageId=0x0001
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ADVF
Language=Bulgarian
OLE_E_ADVF - Invalid advise flags
.

MessageId=0x0002
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ENUM_NOMORE
Language=Bulgarian
OLE_E_ENUM_NOMORE - Can't enumerate any more, because the associated data is missing
.

MessageId=0x0003
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ADVISENOTSUPPORTED
Language=Bulgarian
OLE_E_ADVISENOTSUPPORTED - This implementation doesn't take advises
.

MessageId=0x0004
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOCONNECTION
Language=Bulgarian
OLE_E_NOCONNECTION - There is no connection for this connection ID
.

MessageId=0x0005
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOTRUNNING
Language=Bulgarian
OLE_E_NOTRUNNING - Need to run the object to perform this operation
.

MessageId=0x0006
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOCACHE
Language=Bulgarian
OLE_E_NOCACHE - There is no cache to operate on
.

MessageId=0x0007
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_BLANK
Language=Bulgarian
OLE_E_BLANK - Uninitialized object
.

MessageId=0x0008
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CLASSDIFF
Language=Bulgarian
OLE_E_CLASSDIFF - Linked object's source class has changed
.

MessageId=0x0009
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANT_GETMONIKER
Language=Bulgarian
OLE_E_CANT_GETMONIKER - Not able to get the moniker of the object
.

MessageId=0x000A
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANT_BINDTOSOURCE
Language=Bulgarian
OLE_E_CANT_BINDTOSOURCE - Not able to bind to the source
.

MessageId=0x000B
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_STATIC
Language=Bulgarian
OLE_E_STATIC - Object is static; operation not allowed
.

MessageId=0x000C
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_PROMPTSAVECANCELLED
Language=Bulgarian
OLE_E_PROMPTSAVECANCELLED - User canceled out of save dialog
.

MessageId=0x000D
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_INVALIDRECT
Language=Bulgarian
OLE_E_INVALIDRECT - Invalid rectangle
.

MessageId=0x000E
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_WRONGCOMPOBJ
Language=Bulgarian
OLE_E_WRONGCOMPOBJ - compobj.dll is too old for the ole2.dll initialized
.

MessageId=0x000F
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_INVALIDHWND
Language=Bulgarian
OLE_E_INVALIDHWND - Invalid window handle
.

MessageId=0x0010
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOT_INPLACEACTIVE
Language=Bulgarian
OLE_E_NOT_INPLACEACTIVE - Object is not in any of the inplace active states
.

MessageId=0x0011
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANTCONVERT
Language=Bulgarian
OLE_E_CANTCONVERT - Not able to convert object
.

MessageId=0x0012
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOSTORAGE
Language=Bulgarian
OLE_E_NOSTORAGE - Not able to perform the operation because object is not given storage yet
.

MessageId=0x0064
Severity=Warning
Facility=ITF
SymbolicName=DV_E_FORMATETC
Language=Bulgarian
DV_E_FORMATETC - Invalid FORMATETC structure
.

MessageId=0x0065
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVTARGETDEVICE
Language=Bulgarian
DV_E_DVTARGETDEVICE - Invalid DVTARGETDEVICE structure
.

MessageId=0x0066
Severity=Warning
Facility=ITF
SymbolicName=DV_E_STGMEDIUM
Language=Bulgarian
DV_E_STGMEDIUM - Invalid STDGMEDIUM structure
.

MessageId=0x0067
Severity=Warning
Facility=ITF
SymbolicName=DV_E_STATDATA
Language=Bulgarian
DV_E_STATDATA - Invalid STATDATA structure
.

MessageId=0x0068
Severity=Warning
Facility=ITF
SymbolicName=DV_E_LINDEX
Language=Bulgarian
DV_E_LINDEX - Invalid lindex
.

MessageId=0x0069
Severity=Warning
Facility=ITF
SymbolicName=DV_E_TYMED
Language=Bulgarian
DV_E_TYMED - Invalid tymed
.

MessageId=0x006A
Severity=Warning
Facility=ITF
SymbolicName=DV_E_CLIPFORMAT
Language=Bulgarian
DV_E_CLIPFORMAT - Invalid clipboard format
.

MessageId=0x006B
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVASPECT
Language=Bulgarian
DV_E_DVASPECT - Invalid aspect(s)
.

MessageId=0x006C
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVTARGETDEVICE_SIZE
Language=Bulgarian
DV_E_DVTARGETDEVICE_SIZE - tdSize parameter of the DVTARGETDEVICE structure is invalid
.

MessageId=0x006D
Severity=Warning
Facility=ITF
SymbolicName=DV_E_NOIVIEWOBJECT
Language=Bulgarian
DV_E_NOIVIEWOBJECT - Object doesn't support IViewObject interface
.

MessageId=0x0100
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_NOTREGISTERED
Language=Bulgarian
DRAGDROP_E_NOTREGISTERED - Trying to revoke a drop target that has not been registered
.

MessageId=0x0101
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_ALREADYREGISTERED
Language=Bulgarian
DRAGDROP_E_ALREADYREGISTERED - This window has already been registered as a drop target
.

MessageId=0x0102
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_INVALIDHWND
Language=Bulgarian
DRAGDROP_E_INVALIDHWND - Invalid window handle
.

MessageId=0x0110
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_NOAGGREGATION
Language=Bulgarian
CLASS_E_NOAGGREGATION - Class does not support aggregation (or class object is remote)
.

MessageId=0x0111
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_CLASSNOTAVAILABLE
Language=Bulgarian
CLASS_E_CLASSNOTAVAILABLE - ClassFactory cannot supply requested class
.

MessageId=0x0112
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_NOTLICENSED
Language=Bulgarian
CLASS_E_NOTLICENSED - Class is not licensed for use
.

; EOF
