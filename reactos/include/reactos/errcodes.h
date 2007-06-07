/* This file is generated with wmc version 0.3.0. Do not edit! */
/* Source : dll/win32/kernel32/kernel32.mc */
/* Cmdline: wmc -i -U -H include/reactos/errcodes.h -o dll/win32/kernel32/errcodes.rc dll/win32/kernel32/kernel32.mc */
/* Date   : Wed May 23 04:08:35 2007 */

#ifndef __WMCGENERATED_4653be43_H
#define __WMCGENERATED_4653be43_H

/* Severity codes */
#define STATUS_SEVERITY_ERROR	0x3
#define STATUS_SEVERITY_INFORMATIONAL	0x1
#define STATUS_SEVERITY_SUCCESS	0x0
#define STATUS_SEVERITY_WARNING	0x2

/* Facility codes */
#define FACILITY_ITF	0x4
#define FACILITY_SYSTEM	0x0
#define FACILITY_GENERAL	0x7

/* Message definitions */

/*  kernel32.mc MESSAGE resources for kernel32.dll */


/*  message definitions */

/*  Facility=System */

/* MessageId  : 0x00000000 */
/* Approx. msg: ERROR_SUCCESS - The operation completed successfully. */
#define ERROR_SUCCESS	((ULONG)0x00000000L)

/* MessageId  : 0x00000001 */
/* Approx. msg: ERROR_INVALID_FUNCTION - Incorrect function. */
#define ERROR_INVALID_FUNCTION	((ULONG)0x00000001L)

/* MessageId  : 0x00000002 */
/* Approx. msg: ERROR_FILE_NOT_FOUND - The system cannot find the file specified. */
#define ERROR_FILE_NOT_FOUND	((ULONG)0x00000002L)

/* MessageId  : 0x00000003 */
/* Approx. msg: ERROR_PATH_NOT_FOUND - The system cannot find the path specified. */
#define ERROR_PATH_NOT_FOUND	((ULONG)0x00000003L)

/* MessageId  : 0x00000004 */
/* Approx. msg: ERROR_TOO_MANY_OPEN_FILES - The system cannot open the file. */
#define ERROR_TOO_MANY_OPEN_FILES	((ULONG)0x00000004L)

/* MessageId  : 0x00000005 */
/* Approx. msg: ERROR_ACCESS_DENIED - Access is denied. */
#define ERROR_ACCESS_DENIED	((ULONG)0x00000005L)

/* MessageId  : 0x00000006 */
/* Approx. msg: ERROR_INVALID_HANDLE - The handle is invalid. */
#define ERROR_INVALID_HANDLE	((ULONG)0x00000006L)

/* MessageId  : 0x00000007 */
/* Approx. msg: ERROR_ARENA_TRASHED - The storage control blocks were destroyed. */
#define ERROR_ARENA_TRASHED	((ULONG)0x00000007L)

/* MessageId  : 0x00000008 */
/* Approx. msg: ERROR_NOT_ENOUGH_MEMORY - Not enough storage is available to process this command. */
#define ERROR_NOT_ENOUGH_MEMORY	((ULONG)0x00000008L)

/* MessageId  : 0x00000009 */
/* Approx. msg: ERROR_INVALID_BLOCK - The storage control block address is invalid. */
#define ERROR_INVALID_BLOCK	((ULONG)0x00000009L)

/* MessageId  : 0x0000000a */
/* Approx. msg: ERROR_BAD_ENVIRONMENT - The environment is incorrect. */
#define ERROR_BAD_ENVIRONMENT	((ULONG)0x0000000aL)

/* MessageId  : 0x0000000b */
/* Approx. msg: ERROR_BAD_FORMAT - An attempt was made to load a program with an incorrect format. */
#define ERROR_BAD_FORMAT	((ULONG)0x0000000bL)

/* MessageId  : 0x0000000c */
/* Approx. msg: ERROR_INVALID_ACCESS - The access code is invalid. */
#define ERROR_INVALID_ACCESS	((ULONG)0x0000000cL)

/* MessageId  : 0x0000000d */
/* Approx. msg: ERROR_INVALID_DATA - The data is invalid. */
#define ERROR_INVALID_DATA	((ULONG)0x0000000dL)

/* MessageId  : 0x0000000e */
/* Approx. msg: ERROR_OUTOFMEMORY - Not enough storage is available to complete this operation. */
#define ERROR_OUTOFMEMORY	((ULONG)0x0000000eL)

/* MessageId  : 0x0000000f */
/* Approx. msg: ERROR_INVALID_DRIVE - The system cannot find the drive specified. */
#define ERROR_INVALID_DRIVE	((ULONG)0x0000000fL)

/* MessageId  : 0x00000010 */
/* Approx. msg: ERROR_CURRENT_DIRECTORY - The directory cannot be removed. */
#define ERROR_CURRENT_DIRECTORY	((ULONG)0x00000010L)

/* MessageId  : 0x00000011 */
/* Approx. msg: ERROR_NOT_SAME_DEVICE - The system cannot move the file to a different disk drive. */
#define ERROR_NOT_SAME_DEVICE	((ULONG)0x00000011L)

/* MessageId  : 0x00000012 */
/* Approx. msg: ERROR_NO_MORE_FILES - There are no more files. */
#define ERROR_NO_MORE_FILES	((ULONG)0x00000012L)

/* MessageId  : 0x00000013 */
/* Approx. msg: ERROR_WRITE_PROTECT - The media is write protected. */
#define ERROR_WRITE_PROTECT	((ULONG)0x00000013L)

/* MessageId  : 0x00000014 */
/* Approx. msg: ERROR_BAD_UNIT - The system cannot find the device specified. */
#define ERROR_BAD_UNIT	((ULONG)0x00000014L)

/* MessageId  : 0x00000015 */
/* Approx. msg: ERROR_NOT_READY - The device is not ready. */
#define ERROR_NOT_READY	((ULONG)0x00000015L)

/* MessageId  : 0x00000016 */
/* Approx. msg: ERROR_BAD_COMMAND - The device does not recognize the command. */
#define ERROR_BAD_COMMAND	((ULONG)0x00000016L)

/* MessageId  : 0x00000017 */
/* Approx. msg: ERROR_CRC - Data error (cyclic redundancy check). */
#define ERROR_CRC	((ULONG)0x00000017L)

/* MessageId  : 0x00000018 */
/* Approx. msg: ERROR_BAD_LENGTH - The program issued a command but the command length is incorrect. */
#define ERROR_BAD_LENGTH	((ULONG)0x00000018L)

/* MessageId  : 0x00000019 */
/* Approx. msg: ERROR_SEEK - The drive cannot locate a specific area or track on the disk. */
#define ERROR_SEEK	((ULONG)0x00000019L)

/* MessageId  : 0x0000001a */
/* Approx. msg: ERROR_NOT_DOS_DISK - The specified disk or diskette cannot be accessed. */
#define ERROR_NOT_DOS_DISK	((ULONG)0x0000001aL)

/* MessageId  : 0x0000001b */
/* Approx. msg: ERROR_SECTOR_NOT_FOUND - The drive cannot find the sector requested. */
#define ERROR_SECTOR_NOT_FOUND	((ULONG)0x0000001bL)

/* MessageId  : 0x0000001c */
/* Approx. msg: ERROR_OUT_OF_PAPER - The printer is out of paper. */
#define ERROR_OUT_OF_PAPER	((ULONG)0x0000001cL)

/* MessageId  : 0x0000001d */
/* Approx. msg: ERROR_WRITE_FAULT - The system cannot write to the specified device. */
#define ERROR_WRITE_FAULT	((ULONG)0x0000001dL)

/* MessageId  : 0x0000001e */
/* Approx. msg: ERROR_READ_FAULT - The system cannot read from the specified device. */
#define ERROR_READ_FAULT	((ULONG)0x0000001eL)

/* MessageId  : 0x0000001f */
/* Approx. msg: ERROR_GEN_FAILURE - A device attached to the system is not functioning. */
#define ERROR_GEN_FAILURE	((ULONG)0x0000001fL)

/* MessageId  : 0x00000020 */
/* Approx. msg: ERROR_SHARING_VIOLATION - The process cannot access the file because it is being used by another process. */
#define ERROR_SHARING_VIOLATION	((ULONG)0x00000020L)

/* MessageId  : 0x00000021 */
/* Approx. msg: ERROR_LOCK_VIOLATION - The process cannot access the file because another process has locked a portion of the file. */
#define ERROR_LOCK_VIOLATION	((ULONG)0x00000021L)

/* MessageId  : 0x00000022 */
/* Approx. msg: ERROR_WRONG_DISK - The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1. */
#define ERROR_WRONG_DISK	((ULONG)0x00000022L)

/* MessageId  : 0x00000024 */
/* Approx. msg: ERROR_SHARING_BUFFER_EXCEEDED - Too many files opened for sharing. */
#define ERROR_SHARING_BUFFER_EXCEEDED	((ULONG)0x00000024L)

/* MessageId  : 0x00000026 */
/* Approx. msg: ERROR_HANDLE_EOF - Reached the end of the file. */
#define ERROR_HANDLE_EOF	((ULONG)0x00000026L)

/* MessageId  : 0x00000027 */
/* Approx. msg: ERROR_HANDLE_DISK_FULL - The disk is full. */
#define ERROR_HANDLE_DISK_FULL	((ULONG)0x00000027L)

/* MessageId  : 0x00000032 */
/* Approx. msg: ERROR_NOT_SUPPORTED - The request is not supported. */
#define ERROR_NOT_SUPPORTED	((ULONG)0x00000032L)

/* MessageId  : 0x00000033 */
/* Approx. msg: ERROR_REM_NOT_LIST - Windows cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If Windows still cannot find the network path, contact your network administrator. */
#define ERROR_REM_NOT_LIST	((ULONG)0x00000033L)

/* MessageId  : 0x00000034 */
/* Approx. msg: ERROR_DUP_NAME - You were not connected because a duplicate name exists on the network. Go to System in the Control Panel to change the computer name and try again. */
#define ERROR_DUP_NAME	((ULONG)0x00000034L)

/* MessageId  : 0x00000035 */
/* Approx. msg: ERROR_BAD_NETPATH - The network path was not found. */
#define ERROR_BAD_NETPATH	((ULONG)0x00000035L)

/* MessageId  : 0x00000036 */
/* Approx. msg: ERROR_NETWORK_BUSY - The network is busy. */
#define ERROR_NETWORK_BUSY	((ULONG)0x00000036L)

/* MessageId  : 0x00000037 */
/* Approx. msg: ERROR_DEV_NOT_EXIST - The specified network resource or device is no longer available. */
#define ERROR_DEV_NOT_EXIST	((ULONG)0x00000037L)

/* MessageId  : 0x00000038 */
/* Approx. msg: ERROR_TOO_MANY_CMDS - The network BIOS command limit has been reached. */
#define ERROR_TOO_MANY_CMDS	((ULONG)0x00000038L)

/* MessageId  : 0x00000039 */
/* Approx. msg: ERROR_ADAP_HDW_ERR - A network adapter hardware error occurred. */
#define ERROR_ADAP_HDW_ERR	((ULONG)0x00000039L)

/* MessageId  : 0x0000003a */
/* Approx. msg: ERROR_BAD_NET_RESP - The specified server cannot perform the requested operation. */
#define ERROR_BAD_NET_RESP	((ULONG)0x0000003aL)

/* MessageId  : 0x0000003b */
/* Approx. msg: ERROR_UNEXP_NET_ERR - An unexpected network error occurred. */
#define ERROR_UNEXP_NET_ERR	((ULONG)0x0000003bL)

/* MessageId  : 0x0000003c */
/* Approx. msg: ERROR_BAD_REM_ADAP - The remote adapter is not compatible. */
#define ERROR_BAD_REM_ADAP	((ULONG)0x0000003cL)

/* MessageId  : 0x0000003d */
/* Approx. msg: ERROR_PRINTQ_FULL - The printer queue is full. */
#define ERROR_PRINTQ_FULL	((ULONG)0x0000003dL)

/* MessageId  : 0x0000003e */
/* Approx. msg: ERROR_NO_SPOOL_SPACE - Space to store the file waiting to be printed is not available on the server. */
#define ERROR_NO_SPOOL_SPACE	((ULONG)0x0000003eL)

/* MessageId  : 0x0000003f */
/* Approx. msg: ERROR_PRINT_CANCELLED - Your file waiting to be printed was deleted. */
#define ERROR_PRINT_CANCELLED	((ULONG)0x0000003fL)

/* MessageId  : 0x00000040 */
/* Approx. msg: ERROR_NETNAME_DELETED - The specified network name is no longer available. */
#define ERROR_NETNAME_DELETED	((ULONG)0x00000040L)

/* MessageId  : 0x00000041 */
/* Approx. msg: ERROR_NETWORK_ACCESS_DENIED - Network access is denied. */
#define ERROR_NETWORK_ACCESS_DENIED	((ULONG)0x00000041L)

/* MessageId  : 0x00000042 */
/* Approx. msg: ERROR_BAD_DEV_TYPE - The network resource type is not correct. */
#define ERROR_BAD_DEV_TYPE	((ULONG)0x00000042L)

/* MessageId  : 0x00000043 */
/* Approx. msg: ERROR_BAD_NET_NAME - The network name cannot be found. */
#define ERROR_BAD_NET_NAME	((ULONG)0x00000043L)

/* MessageId  : 0x00000044 */
/* Approx. msg: ERROR_TOO_MANY_NAMES - The name limit for the local computer network adapter card was exceeded. */
#define ERROR_TOO_MANY_NAMES	((ULONG)0x00000044L)

/* MessageId  : 0x00000045 */
/* Approx. msg: ERROR_TOO_MANY_SESS - The network BIOS session limit was exceeded. */
#define ERROR_TOO_MANY_SESS	((ULONG)0x00000045L)

/* MessageId  : 0x00000046 */
/* Approx. msg: ERROR_SHARING_PAUSED - The remote server has been paused or is in the process of being started. */
#define ERROR_SHARING_PAUSED	((ULONG)0x00000046L)

/* MessageId  : 0x00000047 */
/* Approx. msg: ERROR_REQ_NOT_ACCEP - No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept. */
#define ERROR_REQ_NOT_ACCEP	((ULONG)0x00000047L)

/* MessageId  : 0x00000048 */
/* Approx. msg: ERROR_REDIR_PAUSED - The specified printer or disk device has been paused. */
#define ERROR_REDIR_PAUSED	((ULONG)0x00000048L)

/* MessageId  : 0x00000050 */
/* Approx. msg: ERROR_FILE_EXISTS - The file exists. */
#define ERROR_FILE_EXISTS	((ULONG)0x00000050L)

/* MessageId  : 0x00000052 */
/* Approx. msg: ERROR_CANNOT_MAKE - The directory or file cannot be created. */
#define ERROR_CANNOT_MAKE	((ULONG)0x00000052L)

/* MessageId  : 0x00000053 */
/* Approx. msg: ERROR_FAIL_I24 - Fail on INT 24. */
#define ERROR_FAIL_I24	((ULONG)0x00000053L)

/* MessageId  : 0x00000054 */
/* Approx. msg: ERROR_OUT_OF_STRUCTURES - Storage to process this request is not available. */
#define ERROR_OUT_OF_STRUCTURES	((ULONG)0x00000054L)

/* MessageId  : 0x00000055 */
/* Approx. msg: ERROR_ALREADY_ASSIGNED - The local device name is already in use. */
#define ERROR_ALREADY_ASSIGNED	((ULONG)0x00000055L)

/* MessageId  : 0x00000056 */
/* Approx. msg: ERROR_INVALID_PASSWORD - The specified network password is not correct. */
#define ERROR_INVALID_PASSWORD	((ULONG)0x00000056L)

/* MessageId  : 0x00000057 */
/* Approx. msg: ERROR_INVALID_PARAMETER - The parameter is incorrect. */
#define ERROR_INVALID_PARAMETER	((ULONG)0x00000057L)

/* MessageId  : 0x00000058 */
/* Approx. msg: ERROR_NET_WRITE_FAULT - A write fault occurred on the network. */
#define ERROR_NET_WRITE_FAULT	((ULONG)0x00000058L)

/* MessageId  : 0x00000059 */
/* Approx. msg: ERROR_NO_PROC_SLOTS - The system cannot start another process at this time. */
#define ERROR_NO_PROC_SLOTS	((ULONG)0x00000059L)

/* MessageId  : 0x00000064 */
/* Approx. msg: ERROR_TOO_MANY_SEMAPHORES - Cannot create another system semaphore. */
#define ERROR_TOO_MANY_SEMAPHORES	((ULONG)0x00000064L)

/* MessageId  : 0x00000065 */
/* Approx. msg: ERROR_EXCL_SEM_ALREADY_OWNED - The exclusive semaphore is owned by another process. */
#define ERROR_EXCL_SEM_ALREADY_OWNED	((ULONG)0x00000065L)

/* MessageId  : 0x00000066 */
/* Approx. msg: ERROR_SEM_IS_SET - The semaphore is set and cannot be closed. */
#define ERROR_SEM_IS_SET	((ULONG)0x00000066L)

/* MessageId  : 0x00000067 */
/* Approx. msg: ERROR_TOO_MANY_SEM_REQUESTS - The semaphore cannot be set again. */
#define ERROR_TOO_MANY_SEM_REQUESTS	((ULONG)0x00000067L)

/* MessageId  : 0x00000068 */
/* Approx. msg: ERROR_INVALID_AT_INTERRUPT_TIME - Cannot request exclusive semaphores at interrupt time. */
#define ERROR_INVALID_AT_INTERRUPT_TIME	((ULONG)0x00000068L)

/* MessageId  : 0x00000069 */
/* Approx. msg: ERROR_SEM_OWNER_DIED - The previous ownership of this semaphore has ended. */
#define ERROR_SEM_OWNER_DIED	((ULONG)0x00000069L)

/* MessageId  : 0x0000006a */
/* Approx. msg: ERROR_SEM_USER_LIMIT - Insert the diskette for drive %1. */
#define ERROR_SEM_USER_LIMIT	((ULONG)0x0000006aL)

/* MessageId  : 0x0000006b */
/* Approx. msg: ERROR_DISK_CHANGE - The program stopped because an alternate diskette was not inserted. */
#define ERROR_DISK_CHANGE	((ULONG)0x0000006bL)

/* MessageId  : 0x0000006c */
/* Approx. msg: ERROR_DRIVE_LOCKED - The disk is in use or locked by another process. */
#define ERROR_DRIVE_LOCKED	((ULONG)0x0000006cL)

/* MessageId  : 0x0000006d */
/* Approx. msg: ERROR_BROKEN_PIPE - The pipe has been ended. */
#define ERROR_BROKEN_PIPE	((ULONG)0x0000006dL)

/* MessageId  : 0x0000006e */
/* Approx. msg: ERROR_OPEN_FAILED - The system cannot open the device or file specified. */
#define ERROR_OPEN_FAILED	((ULONG)0x0000006eL)

/* MessageId  : 0x0000006f */
/* Approx. msg: ERROR_BUFFER_OVERFLOW - The file name is too long. */
#define ERROR_BUFFER_OVERFLOW	((ULONG)0x0000006fL)

/* MessageId  : 0x00000070 */
/* Approx. msg: ERROR_DISK_FULL - There is not enough space on the disk. */
#define ERROR_DISK_FULL	((ULONG)0x00000070L)

/* MessageId  : 0x00000071 */
/* Approx. msg: ERROR_NO_MORE_SEARCH_HANDLES - No more internal file identifiers available. */
#define ERROR_NO_MORE_SEARCH_HANDLES	((ULONG)0x00000071L)

/* MessageId  : 0x00000072 */
/* Approx. msg: ERROR_INVALID_TARGET_HANDLE - The target internal file identifier is incorrect. */
#define ERROR_INVALID_TARGET_HANDLE	((ULONG)0x00000072L)

/* MessageId  : 0x00000075 */
/* Approx. msg: ERROR_INVALID_CATEGORY - The IOCTL call made by the application program is not correct. */
#define ERROR_INVALID_CATEGORY	((ULONG)0x00000075L)

/* MessageId  : 0x00000076 */
/* Approx. msg: ERROR_INVALID_VERIFY_SWITCH - The verify-on-write switch parameter value is not correct. */
#define ERROR_INVALID_VERIFY_SWITCH	((ULONG)0x00000076L)

/* MessageId  : 0x00000077 */
/* Approx. msg: ERROR_BAD_DRIVER_LEVEL - The system does not support the command requested. */
#define ERROR_BAD_DRIVER_LEVEL	((ULONG)0x00000077L)

/* MessageId  : 0x00000078 */
/* Approx. msg: ERROR_CALL_NOT_IMPLEMENTED - This function is not supported on this system. */
#define ERROR_CALL_NOT_IMPLEMENTED	((ULONG)0x00000078L)

/* MessageId  : 0x00000079 */
/* Approx. msg: ERROR_SEM_TIMEOUT - The semaphore timeout period has expired. */
#define ERROR_SEM_TIMEOUT	((ULONG)0x00000079L)

/* MessageId  : 0x0000007a */
/* Approx. msg: ERROR_INSUFFICIENT_BUFFER - The data area passed to a system call is too small. */
#define ERROR_INSUFFICIENT_BUFFER	((ULONG)0x0000007aL)

/* MessageId  : 0x0000007b */
/* Approx. msg: ERROR_INVALID_NAME - The filename, directory name, or volume label syntax is incorrect. */
#define ERROR_INVALID_NAME	((ULONG)0x0000007bL)

/* MessageId  : 0x0000007c */
/* Approx. msg: ERROR_INVALID_LEVEL - The system call level is not correct. */
#define ERROR_INVALID_LEVEL	((ULONG)0x0000007cL)

/* MessageId  : 0x0000007d */
/* Approx. msg: ERROR_NO_VOLUME_LABEL - The disk has no volume label. */
#define ERROR_NO_VOLUME_LABEL	((ULONG)0x0000007dL)

/* MessageId  : 0x0000007e */
/* Approx. msg: ERROR_MOD_NOT_FOUND - The specified module could not be found. */
#define ERROR_MOD_NOT_FOUND	((ULONG)0x0000007eL)

/* MessageId  : 0x0000007f */
/* Approx. msg: ERROR_PROC_NOT_FOUND - The specified procedure could not be found. */
#define ERROR_PROC_NOT_FOUND	((ULONG)0x0000007fL)

/* MessageId  : 0x00000080 */
/* Approx. msg: ERROR_WAIT_NO_CHILDREN - There are no child processes to wait for. */
#define ERROR_WAIT_NO_CHILDREN	((ULONG)0x00000080L)

/* MessageId  : 0x00000081 */
/* Approx. msg: ERROR_CHILD_NOT_COMPLETE - The %1 application cannot be run in Win32 mode. */
#define ERROR_CHILD_NOT_COMPLETE	((ULONG)0x00000081L)

/* MessageId  : 0x00000082 */
/* Approx. msg: ERROR_DIRECT_ACCESS_HANDLE - Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O. */
#define ERROR_DIRECT_ACCESS_HANDLE	((ULONG)0x00000082L)

/* MessageId  : 0x00000083 */
/* Approx. msg: ERROR_NEGATIVE_SEEK - An attempt was made to move the file pointer before the beginning of the file. */
#define ERROR_NEGATIVE_SEEK	((ULONG)0x00000083L)

/* MessageId  : 0x00000084 */
/* Approx. msg: ERROR_SEEK_ON_DEVICE - The file pointer cannot be set on the specified device or file. */
#define ERROR_SEEK_ON_DEVICE	((ULONG)0x00000084L)

/* MessageId  : 0x00000085 */
/* Approx. msg: ERROR_IS_JOIN_TARGET - A JOIN or SUBST command cannot be used for a drive that contains previously joined drives. */
#define ERROR_IS_JOIN_TARGET	((ULONG)0x00000085L)

/* MessageId  : 0x00000086 */
/* Approx. msg: ERROR_IS_JOINED - An attempt was made to use a JOIN or SUBST command on a drive that has already been joined. */
#define ERROR_IS_JOINED	((ULONG)0x00000086L)

/* MessageId  : 0x00000087 */
/* Approx. msg: ERROR_IS_SUBSTED - An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted. */
#define ERROR_IS_SUBSTED	((ULONG)0x00000087L)

/* MessageId  : 0x00000088 */
/* Approx. msg: ERROR_NOT_JOINED - The system tried to delete the JOIN of a drive that is not joined. */
#define ERROR_NOT_JOINED	((ULONG)0x00000088L)

/* MessageId  : 0x00000089 */
/* Approx. msg: ERROR_NOT_SUBSTED - The system tried to delete the substitution of a drive that is not substituted. */
#define ERROR_NOT_SUBSTED	((ULONG)0x00000089L)

/* MessageId  : 0x0000008a */
/* Approx. msg: ERROR_JOIN_TO_JOIN - The system tried to join a drive to a directory on a joined drive. */
#define ERROR_JOIN_TO_JOIN	((ULONG)0x0000008aL)

/* MessageId  : 0x0000008b */
/* Approx. msg: ERROR_SUBST_TO_SUBST - The system tried to substitute a drive to a directory on a substituted drive. */
#define ERROR_SUBST_TO_SUBST	((ULONG)0x0000008bL)

/* MessageId  : 0x0000008c */
/* Approx. msg: ERROR_JOIN_TO_SUBST - The system tried to join a drive to a directory on a substituted drive. */
#define ERROR_JOIN_TO_SUBST	((ULONG)0x0000008cL)

/* MessageId  : 0x0000008d */
/* Approx. msg: ERROR_SUBST_TO_JOIN - The system tried to SUBST a drive to a directory on a joined drive. */
#define ERROR_SUBST_TO_JOIN	((ULONG)0x0000008dL)

/* MessageId  : 0x0000008e */
/* Approx. msg: ERROR_BUSY_DRIVE - The system cannot perform a JOIN or SUBST at this time. */
#define ERROR_BUSY_DRIVE	((ULONG)0x0000008eL)

/* MessageId  : 0x0000008f */
/* Approx. msg: ERROR_SAME_DRIVE - The system cannot join or substitute a drive to or for a directory on the same drive. */
#define ERROR_SAME_DRIVE	((ULONG)0x0000008fL)

/* MessageId  : 0x00000090 */
/* Approx. msg: ERROR_DIR_NOT_ROOT - The directory is not a subdirectory of the root directory. */
#define ERROR_DIR_NOT_ROOT	((ULONG)0x00000090L)

/* MessageId  : 0x00000091 */
/* Approx. msg: ERROR_DIR_NOT_EMPTY - The directory is not empty. */
#define ERROR_DIR_NOT_EMPTY	((ULONG)0x00000091L)

/* MessageId  : 0x00000092 */
/* Approx. msg: ERROR_IS_SUBST_PATH - The path specified is being used in a substitute. */
#define ERROR_IS_SUBST_PATH	((ULONG)0x00000092L)

/* MessageId  : 0x00000093 */
/* Approx. msg: ERROR_IS_JOIN_PATH - Not enough resources are available to process this command. */
#define ERROR_IS_JOIN_PATH	((ULONG)0x00000093L)

/* MessageId  : 0x00000094 */
/* Approx. msg: ERROR_PATH_BUSY - The path specified cannot be used at this time. */
#define ERROR_PATH_BUSY	((ULONG)0x00000094L)

/* MessageId  : 0x00000095 */
/* Approx. msg: ERROR_IS_SUBST_TARGET - An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute. */
#define ERROR_IS_SUBST_TARGET	((ULONG)0x00000095L)

/* MessageId  : 0x00000096 */
/* Approx. msg: ERROR_SYSTEM_TRACE - System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed. */
#define ERROR_SYSTEM_TRACE	((ULONG)0x00000096L)

/* MessageId  : 0x00000097 */
/* Approx. msg: ERROR_INVALID_EVENT_COUNT - The number of specified semaphore events for DosMuxSemWait is not correct. */
#define ERROR_INVALID_EVENT_COUNT	((ULONG)0x00000097L)

/* MessageId  : 0x00000098 */
/* Approx. msg: ERROR_TOO_MANY_MUXWAITERS - DosMuxSemWait did not execute; too many semaphores are already set. */
#define ERROR_TOO_MANY_MUXWAITERS	((ULONG)0x00000098L)

/* MessageId  : 0x00000099 */
/* Approx. msg: ERROR_INVALID_LIST_FORMAT - The DosMuxSemWait list is not correct. */
#define ERROR_INVALID_LIST_FORMAT	((ULONG)0x00000099L)

/* MessageId  : 0x0000009a */
/* Approx. msg: ERROR_LABEL_TOO_LONG - The volume label you entered exceeds the label character limit of the target file system. */
#define ERROR_LABEL_TOO_LONG	((ULONG)0x0000009aL)

/* MessageId  : 0x0000009b */
/* Approx. msg: ERROR_TOO_MANY_TCBS - Cannot create another thread. */
#define ERROR_TOO_MANY_TCBS	((ULONG)0x0000009bL)

/* MessageId  : 0x0000009c */
/* Approx. msg: ERROR_SIGNAL_REFUSED - The recipient process has refused the signal. */
#define ERROR_SIGNAL_REFUSED	((ULONG)0x0000009cL)

/* MessageId  : 0x0000009d */
/* Approx. msg: ERROR_DISCARDED - The segment is already discarded and cannot be locked. */
#define ERROR_DISCARDED	((ULONG)0x0000009dL)

/* MessageId  : 0x0000009e */
/* Approx. msg: ERROR_NOT_LOCKED - The segment is already unlocked. */
#define ERROR_NOT_LOCKED	((ULONG)0x0000009eL)

/* MessageId  : 0x0000009f */
/* Approx. msg: ERROR_BAD_THREADID_ADDR - The address for the thread ID is not correct. */
#define ERROR_BAD_THREADID_ADDR	((ULONG)0x0000009fL)

/* MessageId  : 0x000000a0 */
/* Approx. msg: ERROR_BAD_ARGUMENTS - The argument string passed to DosExecPgm is not correct. */
#define ERROR_BAD_ARGUMENTS	((ULONG)0x000000a0L)

/* MessageId  : 0x000000a1 */
/* Approx. msg: ERROR_BAD_PATHNAME - The specified path is invalid. */
#define ERROR_BAD_PATHNAME	((ULONG)0x000000a1L)

/* MessageId  : 0x000000a2 */
/* Approx. msg: ERROR_SIGNAL_PENDING - A signal is already pending. */
#define ERROR_SIGNAL_PENDING	((ULONG)0x000000a2L)

/* MessageId  : 0x000000a4 */
/* Approx. msg: ERROR_MAX_THRDS_REACHED - No more threads can be created in the system. */
#define ERROR_MAX_THRDS_REACHED	((ULONG)0x000000a4L)

/* MessageId  : 0x000000a7 */
/* Approx. msg: ERROR_LOCK_FAILED - Unable to lock a region of a file. */
#define ERROR_LOCK_FAILED	((ULONG)0x000000a7L)

/* MessageId  : 0x000000aa */
/* Approx. msg: ERROR_BUSY - The requested resource is in use. */
#define ERROR_BUSY	((ULONG)0x000000aaL)

/* MessageId  : 0x000000ad */
/* Approx. msg: ERROR_CANCEL_VIOLATION - A lock request was not outstanding for the supplied cancel region. */
#define ERROR_CANCEL_VIOLATION	((ULONG)0x000000adL)

/* MessageId  : 0x000000ae */
/* Approx. msg: ERROR_ATOMIC_LOCKS_NOT_SUPPORTED - The file system does not support atomic changes to the lock type. */
#define ERROR_ATOMIC_LOCKS_NOT_SUPPORTED	((ULONG)0x000000aeL)

/* MessageId  : 0x000000b4 */
/* Approx. msg: ERROR_INVALID_SEGMENT_NUMBER - The system detected a segment number that was not correct. */
#define ERROR_INVALID_SEGMENT_NUMBER	((ULONG)0x000000b4L)

/* MessageId  : 0x000000b6 */
/* Approx. msg: ERROR_INVALID_ORDINAL - The operating system cannot run %1. */
#define ERROR_INVALID_ORDINAL	((ULONG)0x000000b6L)

/* MessageId  : 0x000000b7 */
/* Approx. msg: ERROR_ALREADY_EXISTS - Cannot create a file when that file already exists. */
#define ERROR_ALREADY_EXISTS	((ULONG)0x000000b7L)

/* MessageId  : 0x000000ba */
/* Approx. msg: ERROR_INVALID_FLAG_NUMBER - The flag passed is not correct. */
#define ERROR_INVALID_FLAG_NUMBER	((ULONG)0x000000baL)

/* MessageId  : 0x000000bb */
/* Approx. msg: ERROR_SEM_NOT_FOUND - The specified system semaphore name was not found. */
#define ERROR_SEM_NOT_FOUND	((ULONG)0x000000bbL)

/* MessageId  : 0x000000bc */
/* Approx. msg: ERROR_INVALID_STARTING_CODESEG - The operating system cannot run %1. */
#define ERROR_INVALID_STARTING_CODESEG	((ULONG)0x000000bcL)

/* MessageId  : 0x000000bd */
/* Approx. msg: ERROR_INVALID_STACKSEG - The operating system cannot run %1. */
#define ERROR_INVALID_STACKSEG	((ULONG)0x000000bdL)

/* MessageId  : 0x000000be */
/* Approx. msg: ERROR_INVALID_MODULETYPE - The operating system cannot run %1. */
#define ERROR_INVALID_MODULETYPE	((ULONG)0x000000beL)

/* MessageId  : 0x000000bf */
/* Approx. msg: ERROR_INVALID_EXE_SIGNATURE - Cannot run %1 in Win32 mode. */
#define ERROR_INVALID_EXE_SIGNATURE	((ULONG)0x000000bfL)

/* MessageId  : 0x000000c0 */
/* Approx. msg: ERROR_EXE_MARKED_INVALID - The operating system cannot run %1. */
#define ERROR_EXE_MARKED_INVALID	((ULONG)0x000000c0L)

/* MessageId  : 0x000000c1 */
/* Approx. msg: ERROR_BAD_EXE_FORMAT - %1 is not a valid Win32 application. */
#define ERROR_BAD_EXE_FORMAT	((ULONG)0x000000c1L)

/* MessageId  : 0x000000c2 */
/* Approx. msg: ERROR_ITERATED_DATA_EXCEEDS_64k - The operating system cannot run %1. */
#define ERROR_ITERATED_DATA_EXCEEDS_64k	((ULONG)0x000000c2L)

/* MessageId  : 0x000000c3 */
/* Approx. msg: ERROR_INVALID_MINALLOCSIZE - The operating system cannot run %1. */
#define ERROR_INVALID_MINALLOCSIZE	((ULONG)0x000000c3L)

/* MessageId  : 0x000000c4 */
/* Approx. msg: ERROR_DYNLINK_FROM_INVALID_RING - The operating system cannot run this application program. */
#define ERROR_DYNLINK_FROM_INVALID_RING	((ULONG)0x000000c4L)

/* MessageId  : 0x000000c5 */
/* Approx. msg: ERROR_IOPL_NOT_ENABLED - The operating system is not presently configured to run this application. */
#define ERROR_IOPL_NOT_ENABLED	((ULONG)0x000000c5L)

/* MessageId  : 0x000000c6 */
/* Approx. msg: ERROR_INVALID_SEGDPL - The operating system cannot run %1. */
#define ERROR_INVALID_SEGDPL	((ULONG)0x000000c6L)

/* MessageId  : 0x000000c7 */
/* Approx. msg: ERROR_AUTODATASEG_EXCEEDS_64k - The operating system cannot run this application program. */
#define ERROR_AUTODATASEG_EXCEEDS_64k	((ULONG)0x000000c7L)

/* MessageId  : 0x000000c8 */
/* Approx. msg: ERROR_RING2SEG_MUST_BE_MOVABLE - The code segment cannot be greater than or equal to 64K. */
#define ERROR_RING2SEG_MUST_BE_MOVABLE	((ULONG)0x000000c8L)

/* MessageId  : 0x000000c9 */
/* Approx. msg: ERROR_RELOC_CHAIN_XEEDS_SEGLIM - The operating system cannot run %1. */
#define ERROR_RELOC_CHAIN_XEEDS_SEGLIM	((ULONG)0x000000c9L)

/* MessageId  : 0x000000ca */
/* Approx. msg: ERROR_INFLOOP_IN_RELOC_CHAIN - The operating system cannot run %1. */
#define ERROR_INFLOOP_IN_RELOC_CHAIN	((ULONG)0x000000caL)

/* MessageId  : 0x000000cb */
/* Approx. msg: ERROR_ENVVAR_NOT_FOUND - The system could not find the environment option that was entered. */
#define ERROR_ENVVAR_NOT_FOUND	((ULONG)0x000000cbL)

/* MessageId  : 0x000000cd */
/* Approx. msg: ERROR_NO_SIGNAL_SENT - No process in the command subtree has a signal handler. */
#define ERROR_NO_SIGNAL_SENT	((ULONG)0x000000cdL)

/* MessageId  : 0x000000ce */
/* Approx. msg: ERROR_FILENAME_EXCED_RANGE - The filename or extension is too long. */
#define ERROR_FILENAME_EXCED_RANGE	((ULONG)0x000000ceL)

/* MessageId  : 0x000000cf */
/* Approx. msg: ERROR_RING2_STACK_IN_USE - The ring 2 stack is in use. */
#define ERROR_RING2_STACK_IN_USE	((ULONG)0x000000cfL)

/* MessageId  : 0x000000d0 */
/* Approx. msg: ERROR_META_EXPANSION_TOO_LONG - The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified. */
#define ERROR_META_EXPANSION_TOO_LONG	((ULONG)0x000000d0L)

/* MessageId  : 0x000000d1 */
/* Approx. msg: ERROR_INVALID_SIGNAL_NUMBER - The signal being posted is not correct. */
#define ERROR_INVALID_SIGNAL_NUMBER	((ULONG)0x000000d1L)

/* MessageId  : 0x000000d2 */
/* Approx. msg: ERROR_THREAD_1_INACTIVE - The signal handler cannot be set. */
#define ERROR_THREAD_1_INACTIVE	((ULONG)0x000000d2L)

/* MessageId  : 0x000000d4 */
/* Approx. msg: ERROR_LOCKED - The segment is locked and cannot be reallocated. */
#define ERROR_LOCKED	((ULONG)0x000000d4L)

/* MessageId  : 0x000000d6 */
/* Approx. msg: ERROR_TOO_MANY_MODULES - Too many dynamic-link modules are attached to this program or dynamic-link module. */
#define ERROR_TOO_MANY_MODULES	((ULONG)0x000000d6L)

/* MessageId  : 0x000000d7 */
/* Approx. msg: ERROR_NESTING_NOT_ALLOWED - Cannot nest calls to LoadModule. */
#define ERROR_NESTING_NOT_ALLOWED	((ULONG)0x000000d7L)

/* MessageId  : 0x000000d8 */
/* Approx. msg: ERROR_EXE_MACHINE_TYPE_MISMATCH - The image file %1 is valid, but is for a machine type other than the current machine. */
#define ERROR_EXE_MACHINE_TYPE_MISMATCH	((ULONG)0x000000d8L)

/* MessageId  : 0x000000d9 */
/* Approx. msg: ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY - The image file %1 is signed, unable to modify. */
#define ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY	((ULONG)0x000000d9L)

/* MessageId  : 0x000000da */
/* Approx. msg: ERRO_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY - The image file %1 is strong signed, unable to modify. */
#define ERRO_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY	((ULONG)0x000000daL)

/* MessageId  : 0x000000e6 */
/* Approx. msg: ERROR_BAD_PIPE - The pipe state is invalid. */
#define ERROR_BAD_PIPE	((ULONG)0x000000e6L)

/* MessageId  : 0x000000e7 */
/* Approx. msg: ERROR_PIPE_BUSY - All pipe instances are busy. */
#define ERROR_PIPE_BUSY	((ULONG)0x000000e7L)

/* MessageId  : 0x000000e8 */
/* Approx. msg: ERROR_NO_DATA - The pipe is being closed. */
#define ERROR_NO_DATA	((ULONG)0x000000e8L)

/* MessageId  : 0x000000e9 */
/* Approx. msg: ERROR_PIPE_NOT_CONNECTED - No process is on the other end of the pipe. */
#define ERROR_PIPE_NOT_CONNECTED	((ULONG)0x000000e9L)

/* MessageId  : 0x000000ea */
/* Approx. msg: ERROR_MORE_DATA - More data is available. */
#define ERROR_MORE_DATA	((ULONG)0x000000eaL)

/* MessageId  : 0x000000f0 */
/* Approx. msg: ERROR_VC_DISCONNECTED - The session was canceled. */
#define ERROR_VC_DISCONNECTED	((ULONG)0x000000f0L)

/* MessageId  : 0x000000fe */
/* Approx. msg: ERROR_INVALID_EA_NAME - The specified extended attribute name was invalid. */
#define ERROR_INVALID_EA_NAME	((ULONG)0x000000feL)

/* MessageId  : 0x000000ff */
/* Approx. msg: ERROR_EA_LIST_INCONSISTENT - The extended attributes are inconsistent. */
#define ERROR_EA_LIST_INCONSISTENT	((ULONG)0x000000ffL)

/* MessageId  : 0x00000102 */
/* Approx. msg: WAIT_TIMEOUT - The wait operation timed out. */
#define WAIT_TIMEOUT	((ULONG)0x00000102L)

/* MessageId  : 0x00000103 */
/* Approx. msg: ERROR_NO_MORE_ITEMS - No more data is available. */
#define ERROR_NO_MORE_ITEMS	((ULONG)0x00000103L)

/* MessageId  : 0x0000010a */
/* Approx. msg: ERROR_CANNOT_COPY - The copy functions cannot be used. */
#define ERROR_CANNOT_COPY	((ULONG)0x0000010aL)

/* MessageId  : 0x0000010b */
/* Approx. msg: ERROR_DIRECTORY - The directory name is invalid. */
#define ERROR_DIRECTORY	((ULONG)0x0000010bL)

/* MessageId  : 0x00000113 */
/* Approx. msg: ERROR_EAS_DIDNT_FIT - The extended attributes did not fit in the buffer. */
#define ERROR_EAS_DIDNT_FIT	((ULONG)0x00000113L)

/* MessageId  : 0x00000114 */
/* Approx. msg: ERROR_EA_FILE_CORRUPT - The extended attribute file on the mounted file system is corrupt. */
#define ERROR_EA_FILE_CORRUPT	((ULONG)0x00000114L)

/* MessageId  : 0x00000115 */
/* Approx. msg: ERROR_EA_TABLE_FULL - The extended attribute table file is full. */
#define ERROR_EA_TABLE_FULL	((ULONG)0x00000115L)

/* MessageId  : 0x00000116 */
/* Approx. msg: ERROR_INVALID_EA_HANDLE - The specified extended attribute handle is invalid. */
#define ERROR_INVALID_EA_HANDLE	((ULONG)0x00000116L)

/* MessageId  : 0x0000011a */
/* Approx. msg: ERROR_EAS_NOT_SUPPORTED - The mounted file system does not support extended attributes. */
#define ERROR_EAS_NOT_SUPPORTED	((ULONG)0x0000011aL)

/* MessageId  : 0x00000120 */
/* Approx. msg: ERROR_NOT_OWNER - Attempt to release mutex not owned by caller. */
#define ERROR_NOT_OWNER	((ULONG)0x00000120L)

/* MessageId  : 0x0000012a */
/* Approx. msg: ERROR_TOO_MANY_POSTS - Too many posts were made to a semaphore. */
#define ERROR_TOO_MANY_POSTS	((ULONG)0x0000012aL)

/* MessageId  : 0x0000012b */
/* Approx. msg: ERROR_PARTIAL_COPY - Only part of a ReadProcessMemory or WriteProcessMemory request was completed. */
#define ERROR_PARTIAL_COPY	((ULONG)0x0000012bL)

/* MessageId  : 0x0000012c */
/* Approx. msg: ERROR_OPLOCK_NOT_GRANTED - The oplock request is denied. */
#define ERROR_OPLOCK_NOT_GRANTED	((ULONG)0x0000012cL)

/* MessageId  : 0x0000012d */
/* Approx. msg: ERROR_INVALID_OPLOCK_PROTOCOL - An invalid oplock acknowledgment was received by the system. */
#define ERROR_INVALID_OPLOCK_PROTOCOL	((ULONG)0x0000012dL)

/* MessageId  : 0x0000012e */
/* Approx. msg: ERROR_DISK_TOO_FRAGMENTED - The volume is too fragmented to complete this operation. */
#define ERROR_DISK_TOO_FRAGMENTED	((ULONG)0x0000012eL)

/* MessageId  : 0x0000012f */
/* Approx. msg: ERROR_DELETE_PENDING - The file cannot be opened because it is in the process of being deleted. */
#define ERROR_DELETE_PENDING	((ULONG)0x0000012fL)

/* MessageId  : 0x0000013d */
/* Approx. msg: ERROR_MR_MID_NOT_FOUND - The system cannot find message text for message number 0x%1 in the message file for %2. */
#define ERROR_MR_MID_NOT_FOUND	((ULONG)0x0000013dL)

/* MessageId  : 0x0000013e */
/* Approx. msg: ERROR_SCOPE_NOT_FOUND - The scope specified was not found. */
#define ERROR_SCOPE_NOT_FOUND	((ULONG)0x0000013eL)

/* MessageId  : 0x000001e7 */
/* Approx. msg: ERROR_INVALID_ADDRESS - Attempt to access invalid address. */
#define ERROR_INVALID_ADDRESS	((ULONG)0x000001e7L)

/* MessageId  : 0x00000216 */
/* Approx. msg: ERROR_ARITHMETIC_OVERFLOW - Arithmetic result exceeded 32 bits. */
#define ERROR_ARITHMETIC_OVERFLOW	((ULONG)0x00000216L)

/* MessageId  : 0x00000217 */
/* Approx. msg: ERROR_PIPE_CONNECTED - There is a process on other end of the pipe. */
#define ERROR_PIPE_CONNECTED	((ULONG)0x00000217L)

/* MessageId  : 0x00000218 */
/* Approx. msg: ERROR_PIPE_LISTENING - Waiting for a process to open the other end of the pipe. */
#define ERROR_PIPE_LISTENING	((ULONG)0x00000218L)

/* MessageId  : 0x00000219 */
/* Approx. msg: ERROR_ACPI_ERROR - An error occurred in the ACPI subsystem. */
#define ERROR_ACPI_ERROR	((ULONG)0x00000219L)

/* MessageId  : 0x0000021a */
/* Approx. msg: ERROR_ABIOS_ERROR - An error occurred in the ABIOS subsystem */
#define ERROR_ABIOS_ERROR	((ULONG)0x0000021aL)

/* MessageId  : 0x0000021b */
/* Approx. msg: ERROR_WX86_WARNING - A warning occurred in the WX86 subsystem. */
#define ERROR_WX86_WARNING	((ULONG)0x0000021bL)

/* MessageId  : 0x0000021c */
/* Approx. msg: ERROR_WX86_ERROR - An error occurred in the WX86 subsystem. */
#define ERROR_WX86_ERROR	((ULONG)0x0000021cL)

/* MessageId  : 0x0000021d */
/* Approx. msg: ERROR_TIMER_NOT_CANCELED - An attempt was made to cancel or set a timer that has an associated APC and the subject thread is not the thread that originally set the timer with an associated APC routine. */
#define ERROR_TIMER_NOT_CANCELED	((ULONG)0x0000021dL)

/* MessageId  : 0x0000021e */
/* Approx. msg: ERROR_UNWIND - Unwind exception code. */
#define ERROR_UNWIND	((ULONG)0x0000021eL)

/* MessageId  : 0x0000021f */
/* Approx. msg: ERROR_BAD_STACK - An invalid or unaligned stack was encountered during an unwind operation. */
#define ERROR_BAD_STACK	((ULONG)0x0000021fL)

/* MessageId  : 0x00000220 */
/* Approx. msg: ERROR_INVALID_UNWIND_TARGET - An invalid unwind target was encountered during an unwind operation. */
#define ERROR_INVALID_UNWIND_TARGET	((ULONG)0x00000220L)

/* MessageId  : 0x00000221 */
/* Approx. msg: ERROR_INVALID_PORT_ATTRIBUTES - Invalid Object Attributes specified to NtCreatePort or invalid Port Attributes specified to NtConnectPort */
#define ERROR_INVALID_PORT_ATTRIBUTES	((ULONG)0x00000221L)

/* MessageId  : 0x00000222 */
/* Approx. msg: ERROR_PORT_MESSAGE_TOO_LONG - Length of message passed to NtRequestPort or NtRequestWaitReplyPort was longer than the maximum message allowed by the port. */
#define ERROR_PORT_MESSAGE_TOO_LONG	((ULONG)0x00000222L)

/* MessageId  : 0x00000223 */
/* Approx. msg: ERROR_INVALID_QUOTA_LOWER - An attempt was made to lower a quota limit below the current usage. */
#define ERROR_INVALID_QUOTA_LOWER	((ULONG)0x00000223L)

/* MessageId  : 0x00000224 */
/* Approx. msg: ERROR_DEVICE_ALREADY_ATTACHED - An attempt was made to attach to a device that was already attached to another device. */
#define ERROR_DEVICE_ALREADY_ATTACHED	((ULONG)0x00000224L)

/* MessageId  : 0x00000225 */
/* Approx. msg: ERROR_INSTRUCTION_MISALIGNMENT - An attempt was made to execute an instruction at an unaligned address and the host system does not support unaligned instruction references. */
#define ERROR_INSTRUCTION_MISALIGNMENT	((ULONG)0x00000225L)

/* MessageId  : 0x00000226 */
/* Approx. msg: ERROR_PROFILING_NOT_STARTED - Profiling not started. */
#define ERROR_PROFILING_NOT_STARTED	((ULONG)0x00000226L)

/* MessageId  : 0x00000227 */
/* Approx. msg: ERROR_PROFILING_NOT_STOPPED - Profiling not stopped. */
#define ERROR_PROFILING_NOT_STOPPED	((ULONG)0x00000227L)

/* MessageId  : 0x00000228 */
/* Approx. msg: ERROR_COULD_NOT_INTERPRET - The passed ACL did not contain the minimum required information. */
#define ERROR_COULD_NOT_INTERPRET	((ULONG)0x00000228L)

/* MessageId  : 0x00000229 */
/* Approx. msg: ERROR_PROFILING_AT_LIMIT - The number of active profiling objects is at the maximum and no more may be started. */
#define ERROR_PROFILING_AT_LIMIT	((ULONG)0x00000229L)

/* MessageId  : 0x0000022a */
/* Approx. msg: ERROR_CANT_WAIT - Used to indicate that an operation cannot continue without blocking for I/O. */
#define ERROR_CANT_WAIT	((ULONG)0x0000022aL)

/* MessageId  : 0x0000022b */
/* Approx. msg: ERROR_CANT_TERMINATE_SELF - Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process. */
#define ERROR_CANT_TERMINATE_SELF	((ULONG)0x0000022bL)

/* MessageId  : 0x0000022c */
/* Approx. msg: ERROR_UNEXPECTED_MM_CREATE_ERR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception. */
#define ERROR_UNEXPECTED_MM_CREATE_ERR	((ULONG)0x0000022cL)

/* MessageId  : 0x0000022d */
/* Approx. msg: ERROR_UNEXPECTED_MM_MAP_ERROR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception. */
#define ERROR_UNEXPECTED_MM_MAP_ERROR	((ULONG)0x0000022dL)

/* MessageId  : 0x0000022e */
/* Approx. msg: ERROR_UNEXPECTED_MM_EXTEND_ERR - If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception. */
#define ERROR_UNEXPECTED_MM_EXTEND_ERR	((ULONG)0x0000022eL)

/* MessageId  : 0x0000022f */
/* Approx. msg: ERROR_BAD_FUNCTION_TABLE - A malformed function table was encountered during an unwind operation. */
#define ERROR_BAD_FUNCTION_TABLE	((ULONG)0x0000022fL)

/* MessageId  : 0x00000230 */
/* Approx. msg: ERROR_NO_GUID_TRANSLATION - Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail. */
#define ERROR_NO_GUID_TRANSLATION	((ULONG)0x00000230L)

/* MessageId  : 0x00000231 */
/* Approx. msg: ERROR_INVALID_LDT_SIZE - Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors. */
#define ERROR_INVALID_LDT_SIZE	((ULONG)0x00000231L)

/* MessageId  : 0x00000233 */
/* Approx. msg: ERROR_INVALID_LDT_OFFSET - Indicates that the starting value for the LDT information was not an integral multiple of the selector size. */
#define ERROR_INVALID_LDT_OFFSET	((ULONG)0x00000233L)

/* MessageId  : 0x00000234 */
/* Approx. msg: ERROR_INVALID_LDT_DESCRIPTOR - Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors. */
#define ERROR_INVALID_LDT_DESCRIPTOR	((ULONG)0x00000234L)

/* MessageId  : 0x00000235 */
/* Approx. msg: ERROR_TOO_MANY_THREADS - Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads. */
#define ERROR_TOO_MANY_THREADS	((ULONG)0x00000235L)

/* MessageId  : 0x00000236 */
/* Approx. msg: ERROR_THREAD_NOT_IN_PROCESS - An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified. */
#define ERROR_THREAD_NOT_IN_PROCESS	((ULONG)0x00000236L)

/* MessageId  : 0x00000237 */
/* Approx. msg: ERROR_PAGEFILE_QUOTA_EXCEEDED - Page file quota was exceeded. */
#define ERROR_PAGEFILE_QUOTA_EXCEEDED	((ULONG)0x00000237L)

/* MessageId  : 0x00000238 */
/* Approx. msg: ERROR_LOGON_SERVER_CONFLICT - The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role. */
#define ERROR_LOGON_SERVER_CONFLICT	((ULONG)0x00000238L)

/* MessageId  : 0x00000239 */
/* Approx. msg: ERROR_SYNCHRONIZATION_REQUIRED - The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required. */
#define ERROR_SYNCHRONIZATION_REQUIRED	((ULONG)0x00000239L)

/* MessageId  : 0x0000023a */
/* Approx. msg: ERROR_NET_OPEN_FAILED - The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines. */
#define ERROR_NET_OPEN_FAILED	((ULONG)0x0000023aL)

/* MessageId  : 0x0000023b */
/* Approx. msg: ERROR_IO_PRIVILEGE_FAILED - The I/O permissions for the process could not be changed. */
#define ERROR_IO_PRIVILEGE_FAILED	((ULONG)0x0000023bL)

/* MessageId  : 0x0000023c */
/* Approx. msg: ERROR_CONTROL_C_EXIT - The application terminated as a result of a CTRL+C. */
#define ERROR_CONTROL_C_EXIT	((ULONG)0x0000023cL)

/* MessageId  : 0x0000023d */
/* Approx. msg: ERROR_MISSING_SYSTEMFILE - The required system file %hs is bad or missing. */
#define ERROR_MISSING_SYSTEMFILE	((ULONG)0x0000023dL)

/* MessageId  : 0x0000023e */
/* Approx. msg: ERROR_UNHANDLED_EXCEPTION - The exception %s (0x%08lx) occurred in the application at location 0x%08lx. */
#define ERROR_UNHANDLED_EXCEPTION	((ULONG)0x0000023eL)

/* MessageId  : 0x0000023f */
/* Approx. msg: ERROR_APP_INIT_FAILURE - The application failed to initialize properly (0x%lx). Click on OK to terminate the application. */
#define ERROR_APP_INIT_FAILURE	((ULONG)0x0000023fL)

/* MessageId  : 0x00000240 */
/* Approx. msg: ERROR_PAGEFILE_CREATE_FAILED - The creation of the paging file %hs failed (%lx). The requested size was %ld. */
#define ERROR_PAGEFILE_CREATE_FAILED	((ULONG)0x00000240L)

/* MessageId  : 0x00000242 */
/* Approx. msg: ERROR_NO_PAGEFILE - No paging file was specified in the system configuration. */
#define ERROR_NO_PAGEFILE	((ULONG)0x00000242L)

/* MessageId  : 0x00000243 */
/* Approx. msg: ERROR_ILLEGAL_FLOAT_CONTEXT - A real-mode application issued a floating-point instruction and floating-point hardware is not present. */
#define ERROR_ILLEGAL_FLOAT_CONTEXT	((ULONG)0x00000243L)

/* MessageId  : 0x00000244 */
/* Approx. msg: ERROR_NO_EVENT_PAIR - An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread. */
#define ERROR_NO_EVENT_PAIR	((ULONG)0x00000244L)

/* MessageId  : 0x00000245 */
/* Approx. msg: ERROR_DOMAIN_CTRLR_CONFIG_ERROR - A Windows Server has an incorrect configuration. */
#define ERROR_DOMAIN_CTRLR_CONFIG_ERROR	((ULONG)0x00000245L)

/* MessageId  : 0x00000246 */
/* Approx. msg: ERROR_ILLEGAL_CHARACTER - An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE. */
#define ERROR_ILLEGAL_CHARACTER	((ULONG)0x00000246L)

/* MessageId  : 0x00000247 */
/* Approx. msg: ERROR_UNDEFINED_CHARACTER - The Unicode character is not defined in the Unicode character set installed on the system. */
#define ERROR_UNDEFINED_CHARACTER	((ULONG)0x00000247L)

/* MessageId  : 0x00000248 */
/* Approx. msg: ERROR_FLOPPY_VOLUME - The paging file cannot be created on a floppy diskette. */
#define ERROR_FLOPPY_VOLUME	((ULONG)0x00000248L)

/* MessageId  : 0x00000249 */
/* Approx. msg: ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT - The system bios failed to connect a system interrupt to the device or bus for which the device is connected. */
#define ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT	((ULONG)0x00000249L)

/* MessageId  : 0x0000024a */
/* Approx. msg: ERROR_BACKUP_CONTROLLER - This operation is only allowed for the Primary Domain Controller of the domain. */
#define ERROR_BACKUP_CONTROLLER	((ULONG)0x0000024aL)

/* MessageId  : 0x0000024b */
/* Approx. msg: ERROR_MUTANT_LIMIT_EXCEEDED - An attempt was made to acquire a mutant such that its maximum count would have been exceeded. */
#define ERROR_MUTANT_LIMIT_EXCEEDED	((ULONG)0x0000024bL)

/* MessageId  : 0x0000024c */
/* Approx. msg: ERROR_FS_DRIVER_REQUIRED - A volume has been accessed for which a file system driver is required that has not yet been loaded. */
#define ERROR_FS_DRIVER_REQUIRED	((ULONG)0x0000024cL)

/* MessageId  : 0x0000024d */
/* Approx. msg: ERROR_CANNOT_LOAD_REGISTRY_FILE - The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable. */
#define ERROR_CANNOT_LOAD_REGISTRY_FILE	((ULONG)0x0000024dL)

/* MessageId  : 0x0000024e */
/* Approx. msg: ERROR_DEBUG_ATTACH_FAILED - An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error. */
#define ERROR_DEBUG_ATTACH_FAILED	((ULONG)0x0000024eL)

/* MessageId  : 0x0000024f */
/* Approx. msg: ERROR_SYSTEM_PROCESS_TERMINATED - The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down. */
#define ERROR_SYSTEM_PROCESS_TERMINATED	((ULONG)0x0000024fL)

/* MessageId  : 0x00000250 */
/* Approx. msg: ERROR_DATA_NOT_ACCEPTED - The TDI client could not handle the data received during an indication. */
#define ERROR_DATA_NOT_ACCEPTED	((ULONG)0x00000250L)

/* MessageId  : 0x00000251 */
/* Approx. msg: ERROR_VDM_HARD_ERROR - NTVDM encountered a hard error. */
#define ERROR_VDM_HARD_ERROR	((ULONG)0x00000251L)

/* MessageId  : 0x00000252 */
/* Approx. msg: ERROR_DRIVER_CANCEL_TIMEOUT - The driver %hs failed to complete a cancelled I/O request in the allotted time. */
#define ERROR_DRIVER_CANCEL_TIMEOUT	((ULONG)0x00000252L)

/* MessageId  : 0x00000253 */
/* Approx. msg: ERROR_REPLY_MESSAGE_MISMATCH - An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message. */
#define ERROR_REPLY_MESSAGE_MISMATCH	((ULONG)0x00000253L)

/* MessageId  : 0x00000254 */
/* Approx. msg: ERROR_LOST_WRITEBEHIND_DATA - Windows was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere. */
#define ERROR_LOST_WRITEBEHIND_DATA	((ULONG)0x00000254L)

/* MessageId  : 0x00000255 */
/* Approx. msg: ERROR_CLIENT_SERVER_PARAMETERS_INVALID - The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window. */
#define ERROR_CLIENT_SERVER_PARAMETERS_INVALID	((ULONG)0x00000255L)

/* MessageId  : 0x00000256 */
/* Approx. msg: ERROR_NOT_TINY_STREAM - The stream is not a tiny stream. */
#define ERROR_NOT_TINY_STREAM	((ULONG)0x00000256L)

/* MessageId  : 0x00000257 */
/* Approx. msg: ERROR_STACK_OVERFLOW_READ - The request must be handled by the stack overflow code. */
#define ERROR_STACK_OVERFLOW_READ	((ULONG)0x00000257L)

/* MessageId  : 0x00000258 */
/* Approx. msg: ERROR_CONVERT_TO_LARGE - Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream. */
#define ERROR_CONVERT_TO_LARGE	((ULONG)0x00000258L)

/* MessageId  : 0x00000259 */
/* Approx. msg: ERROR_FOUND_OUT_OF_SCOPE - The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation. */
#define ERROR_FOUND_OUT_OF_SCOPE	((ULONG)0x00000259L)

/* MessageId  : 0x0000025a */
/* Approx. msg: ERROR_ALLOCATE_BUCKET - The bucket array must be grown. Retry transaction after doing so. */
#define ERROR_ALLOCATE_BUCKET	((ULONG)0x0000025aL)

/* MessageId  : 0x0000025b */
/* Approx. msg: ERROR_MARSHALL_OVERFLOW - The user/kernel marshalling buffer has overflowed. */
#define ERROR_MARSHALL_OVERFLOW	((ULONG)0x0000025bL)

/* MessageId  : 0x0000025c */
/* Approx. msg: ERROR_INVALID_VARIANT - The supplied variant structure contains invalid data. */
#define ERROR_INVALID_VARIANT	((ULONG)0x0000025cL)

/* MessageId  : 0x0000025d */
/* Approx. msg: ERROR_BAD_COMPRESSION_BUFFER - The specified buffer contains ill-formed data. */
#define ERROR_BAD_COMPRESSION_BUFFER	((ULONG)0x0000025dL)

/* MessageId  : 0x0000025e */
/* Approx. msg: ERROR_AUDIT_FAILED - An attempt to generate a security audit failed. */
#define ERROR_AUDIT_FAILED	((ULONG)0x0000025eL)

/* MessageId  : 0x0000025f */
/* Approx. msg: ERROR_TIMER_RESOLUTION_NOT_SET - The timer resolution was not previously set by the current process. */
#define ERROR_TIMER_RESOLUTION_NOT_SET	((ULONG)0x0000025fL)

/* MessageId  : 0x00000260 */
/* Approx. msg: ERROR_INSUFFICIENT_LOGON_INFO - There is insufficient account information to log you on. */
#define ERROR_INSUFFICIENT_LOGON_INFO	((ULONG)0x00000260L)

/* MessageId  : 0x00000261 */
/* Approx. msg: ERROR_BAD_DLL_ENTRYPOINT - The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly. */
#define ERROR_BAD_DLL_ENTRYPOINT	((ULONG)0x00000261L)

/* MessageId  : 0x00000262 */
/* Approx. msg: ERROR_BAD_SERVICE_ENTRYPOINT - The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly. */
#define ERROR_BAD_SERVICE_ENTRYPOINT	((ULONG)0x00000262L)

/* MessageId  : 0x00000263 */
/* Approx. msg: ERROR_IP_ADDRESS_CONFLICT1 - There is an IP address conflict with another system on the network */
#define ERROR_IP_ADDRESS_CONFLICT1	((ULONG)0x00000263L)

/* MessageId  : 0x00000264 */
/* Approx. msg: ERROR_IP_ADDRESS_CONFLICT2 - There is an IP address conflict with another system on the network */
#define ERROR_IP_ADDRESS_CONFLICT2	((ULONG)0x00000264L)

/* MessageId  : 0x00000265 */
/* Approx. msg: ERROR_REGISTRY_QUOTA_LIMIT - The system has reached the maximum size allowed for the system part of the registry. Additional storage requests will be ignored. */
#define ERROR_REGISTRY_QUOTA_LIMIT	((ULONG)0x00000265L)

/* MessageId  : 0x00000266 */
/* Approx. msg: ERROR_NO_CALLBACK_ACTIVE - A callback return system service cannot be executed when no callback is active. */
#define ERROR_NO_CALLBACK_ACTIVE	((ULONG)0x00000266L)

/* MessageId  : 0x00000267 */
/* Approx. msg: ERROR_PWD_TOO_SHORT - The password provided is too short to meet the policy of your user account. Please choose a longer password. */
#define ERROR_PWD_TOO_SHORT	((ULONG)0x00000267L)

/* MessageId  : 0x00000268 */
/* Approx. msg: ERROR_PWD_TOO_RECENT - The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned. */
#define ERROR_PWD_TOO_RECENT	((ULONG)0x00000268L)

/* MessageId  : 0x00000269 */
/* Approx. msg: ERROR_PWD_HISTORY_CONFLICT - You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used. */
#define ERROR_PWD_HISTORY_CONFLICT	((ULONG)0x00000269L)

/* MessageId  : 0x0000026a */
/* Approx. msg: ERROR_UNSUPPORTED_COMPRESSION - The specified compression format is unsupported. */
#define ERROR_UNSUPPORTED_COMPRESSION	((ULONG)0x0000026aL)

/* MessageId  : 0x0000026b */
/* Approx. msg: ERROR_INVALID_HW_PROFILE - The specified hardware profile configuration is invalid. */
#define ERROR_INVALID_HW_PROFILE	((ULONG)0x0000026bL)

/* MessageId  : 0x0000026c */
/* Approx. msg: ERROR_INVALID_PLUGPLAY_DEVICE_PATH - The specified Plug and Play registry device path is invalid. */
#define ERROR_INVALID_PLUGPLAY_DEVICE_PATH	((ULONG)0x0000026cL)

/* MessageId  : 0x0000026d */
/* Approx. msg: ERROR_QUOTA_LIST_INCONSISTENT - The specified quota list is internally inconsistent with its descriptor. */
#define ERROR_QUOTA_LIST_INCONSISTENT	((ULONG)0x0000026dL)

/* MessageId  : 0x0000026e */
/* Approx. msg: ERROR_EVALUATION_EXPIRATION - The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product. */
#define ERROR_EVALUATION_EXPIRATION	((ULONG)0x0000026eL)

/* MessageId  : 0x0000026f */
/* Approx. msg: ERROR_ILLEGAL_DLL_RELOCATION - The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for Windows system DLLs. The vendor supplying the DLL should be contacted for a new DLL. */
#define ERROR_ILLEGAL_DLL_RELOCATION	((ULONG)0x0000026fL)

/* MessageId  : 0x00000270 */
/* Approx. msg: ERROR_DLL_INIT_FAILED_LOGOFF - The application failed to initialize because the window station is shutting down. */
#define ERROR_DLL_INIT_FAILED_LOGOFF	((ULONG)0x00000270L)

/* MessageId  : 0x00000271 */
/* Approx. msg: ERROR_VALIDATE_CONTINUE - The validation process needs to continue on to the next step. */
#define ERROR_VALIDATE_CONTINUE	((ULONG)0x00000271L)

/* MessageId  : 0x00000272 */
/* Approx. msg: ERROR_NO_MORE_MATCHES - There are no more matches for the current index enumeration. */
#define ERROR_NO_MORE_MATCHES	((ULONG)0x00000272L)

/* MessageId  : 0x00000273 */
/* Approx. msg: ERROR_RANGE_LIST_CONFLICT - The range could not be added to the range list because of a conflict. */
#define ERROR_RANGE_LIST_CONFLICT	((ULONG)0x00000273L)

/* MessageId  : 0x00000274 */
/* Approx. msg: ERROR_SERVER_SID_MISMATCH - The server process is running under a SID different than that required by client. */
#define ERROR_SERVER_SID_MISMATCH	((ULONG)0x00000274L)

/* MessageId  : 0x00000275 */
/* Approx. msg: ERROR_CANT_ENABLE_DENY_ONLY - A group marked use for deny only cannot be enabled. */
#define ERROR_CANT_ENABLE_DENY_ONLY	((ULONG)0x00000275L)

/* MessageId  : 0x00000276 */
/* Approx. msg: ERROR_FLOAT_MULTIPLE_FAULTS - Multiple floating point faults. */
#define ERROR_FLOAT_MULTIPLE_FAULTS	((ULONG)0x00000276L)

/* MessageId  : 0x00000277 */
/* Approx. msg: ERROR_FLOAT_MULTIPLE_TRAPS - Multiple floating point traps. */
#define ERROR_FLOAT_MULTIPLE_TRAPS	((ULONG)0x00000277L)

/* MessageId  : 0x00000278 */
/* Approx. msg: ERROR_NOINTERFACE - The requested interface is not supported. */
#define ERROR_NOINTERFACE	((ULONG)0x00000278L)

/* MessageId  : 0x00000279 */
/* Approx. msg: ERROR_DRIVER_FAILED_SLEEP - The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode. */
#define ERROR_DRIVER_FAILED_SLEEP	((ULONG)0x00000279L)

/* MessageId  : 0x0000027a */
/* Approx. msg: ERROR_CORRUPT_SYSTEM_FILE - The system file %1 has become corrupt and has been replaced. */
#define ERROR_CORRUPT_SYSTEM_FILE	((ULONG)0x0000027aL)

/* MessageId  : 0x0000027b */
/* Approx. msg: ERROR_COMMITMENT_MINIMUM - Your system is low on virtual memory. Windows is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help. */
#define ERROR_COMMITMENT_MINIMUM	((ULONG)0x0000027bL)

/* MessageId  : 0x0000027c */
/* Approx. msg: ERROR_PNP_RESTART_ENUMERATION - A device was removed so enumeration must be restarted. */
#define ERROR_PNP_RESTART_ENUMERATION	((ULONG)0x0000027cL)

/* MessageId  : 0x0000027d */
/* Approx. msg: ERROR_SYSTEM_IMAGE_BAD_SIGNATURE - The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down. */
#define ERROR_SYSTEM_IMAGE_BAD_SIGNATURE	((ULONG)0x0000027dL)

/* MessageId  : 0x0000027e */
/* Approx. msg: ERROR_PNP_REBOOT_REQUIRED - Device will not start without a reboot. */
#define ERROR_PNP_REBOOT_REQUIRED	((ULONG)0x0000027eL)

/* MessageId  : 0x0000027f */
/* Approx. msg: ERROR_INSUFFICIENT_POWER - There is not enough power to complete the requested operation. */
#define ERROR_INSUFFICIENT_POWER	((ULONG)0x0000027fL)

/* MessageId  : 0x00000281 */
/* Approx. msg: ERROR_SYSTEM_SHUTDOWN - The system is in the process of shutting down. */
#define ERROR_SYSTEM_SHUTDOWN	((ULONG)0x00000281L)

/* MessageId  : 0x00000282 */
/* Approx. msg: ERROR_PORT_NOT_SET - An attempt to remove a processes DebugPort was made, but a port was not already associated with the process. */
#define ERROR_PORT_NOT_SET	((ULONG)0x00000282L)

/* MessageId  : 0x00000283 */
/* Approx. msg: ERROR_DS_VERSION_CHECK_FAILURE - This version of Windows is not compatible with the behavior version of directory forest, domain or domain controller. */
#define ERROR_DS_VERSION_CHECK_FAILURE	((ULONG)0x00000283L)

/* MessageId  : 0x00000284 */
/* Approx. msg: ERROR_RANGE_NOT_FOUND - The specified range could not be found in the range list. */
#define ERROR_RANGE_NOT_FOUND	((ULONG)0x00000284L)

/* MessageId  : 0x00000286 */
/* Approx. msg: ERROR_NOT_SAFE_MODE_DRIVER - The driver was not loaded because the system is booting into safe mode. */
#define ERROR_NOT_SAFE_MODE_DRIVER	((ULONG)0x00000286L)

/* MessageId  : 0x00000287 */
/* Approx. msg: ERROR_FAILED_DRIVER_ENTRY - The driver was not loaded because it failed it's initialization call. */
#define ERROR_FAILED_DRIVER_ENTRY	((ULONG)0x00000287L)

/* MessageId  : 0x00000288 */
/* Approx. msg: ERROR_DEVICE_ENUMERATION_ERROR - The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection. */
#define ERROR_DEVICE_ENUMERATION_ERROR	((ULONG)0x00000288L)

/* MessageId  : 0x00000289 */
/* Approx. msg: ERROR_MOUNT_POINT_NOT_RESOLVED - The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached. */
#define ERROR_MOUNT_POINT_NOT_RESOLVED	((ULONG)0x00000289L)

/* MessageId  : 0x0000028a */
/* Approx. msg: ERROR_INVALID_DEVICE_OBJECT_PARAMETER - The device object parameter is either not a valid device object or is not attached to the volume specified by the file name. */
#define ERROR_INVALID_DEVICE_OBJECT_PARAMETER	((ULONG)0x0000028aL)

/* MessageId  : 0x0000028b */
/* Approx. msg: ERROR_MCA_OCCURED - A Machine Check Error has occurred. Please check the system eventlog for additional information. */
#define ERROR_MCA_OCCURED	((ULONG)0x0000028bL)

/* MessageId  : 0x0000028c */
/* Approx. msg: ERROR_DRIVER_DATABASE_ERROR - There was error [%2] processing the driver database. */
#define ERROR_DRIVER_DATABASE_ERROR	((ULONG)0x0000028cL)

/* MessageId  : 0x0000028d */
/* Approx. msg: ERROR_SYSTEM_HIVE_TOO_LARGE - System hive size has exceeded its limit. */
#define ERROR_SYSTEM_HIVE_TOO_LARGE	((ULONG)0x0000028dL)

/* MessageId  : 0x0000028e */
/* Approx. msg: ERROR_DRIVER_FAILED_PRIOR_UNLOAD - The driver could not be loaded because a previous version of the driver is still in memory. */
#define ERROR_DRIVER_FAILED_PRIOR_UNLOAD	((ULONG)0x0000028eL)

/* MessageId  : 0x0000028f */
/* Approx. msg: ERROR_VOLSNAP_PREPARE_HIBERNATE - Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation. */
#define ERROR_VOLSNAP_PREPARE_HIBERNATE	((ULONG)0x0000028fL)

/* MessageId  : 0x00000290 */
/* Approx. msg: ERROR_HIBERNATION_FAILURE - The system has failed to hibernate (The error code is %hs). Hibernation will be disabled until the system is restarted. */
#define ERROR_HIBERNATION_FAILURE	((ULONG)0x00000290L)

/* MessageId  : 0x00000291 */
/* Approx. msg: ERROR_HUNG_DISPLAY_DRIVER_THREAD - The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft. */
#define ERROR_HUNG_DISPLAY_DRIVER_THREAD	((ULONG)0x00000291L)

/* MessageId  : 0x00000299 */
/* Approx. msg: ERROR_FILE_SYSTEM_LIMITATION - The requested operation could not be completed due to a file system limitation. */
#define ERROR_FILE_SYSTEM_LIMITATION	((ULONG)0x00000299L)

/* MessageId  : 0x0000029c */
/* Approx. msg: ERROR_ASSERTION_FAILURE - An assertion failure has occurred. */
#define ERROR_ASSERTION_FAILURE	((ULONG)0x0000029cL)

/* MessageId  : 0x0000029d */
/* Approx. msg: ERROR_VERIFIER_STOP - Application verifier has found an error in the current process. */
#define ERROR_VERIFIER_STOP	((ULONG)0x0000029dL)

/* MessageId  : 0x0000029e */
/* Approx. msg: ERROR_WOW_ASSERTION - WOW Assertion Error. */
#define ERROR_WOW_ASSERTION	((ULONG)0x0000029eL)

/* MessageId  : 0x0000029f */
/* Approx. msg: ERROR_PNP_BAD_MPS_TABLE - A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update. */
#define ERROR_PNP_BAD_MPS_TABLE	((ULONG)0x0000029fL)

/* MessageId  : 0x000002a0 */
/* Approx. msg: ERROR_PNP_TRANSLATION_FAILED - A translator failed to translate resources. */
#define ERROR_PNP_TRANSLATION_FAILED	((ULONG)0x000002a0L)

/* MessageId  : 0x000002a1 */
/* Approx. msg: ERROR_PNP_IRQ_TRANSLATION_FAILED - A IRQ translator failed to translate resources. */
#define ERROR_PNP_IRQ_TRANSLATION_FAILED	((ULONG)0x000002a1L)

/* MessageId  : 0x000002a2 */
/* Approx. msg: ERROR_PNP_INVALID_ID - Driver %2 returned invalid ID for a child device (%3). */
#define ERROR_PNP_INVALID_ID	((ULONG)0x000002a2L)

/* MessageId  : 0x000002a3 */
/* Approx. msg: ERROR_WAKE_SYSTEM_DEBUGGER - The system debugger was awakened by an interrupt. */
#define ERROR_WAKE_SYSTEM_DEBUGGER	((ULONG)0x000002a3L)

/* MessageId  : 0x000002a4 */
/* Approx. msg: ERROR_HANDLES_CLOSED - Handles to objects have been automatically closed as a result of the requested operation. */
#define ERROR_HANDLES_CLOSED	((ULONG)0x000002a4L)

/* MessageId  : 0x000002a5 */
/* Approx. msg: ERROR_EXTRANEOUS_INFORMATION - he specified access control list (ACL) contained more information than was expected. */
#define ERROR_EXTRANEOUS_INFORMATION	((ULONG)0x000002a5L)

/* MessageId  : 0x000002a6 */
/* Approx. msg: ERROR_RXACT_COMMIT_NECESSARY - This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired). */
#define ERROR_RXACT_COMMIT_NECESSARY	((ULONG)0x000002a6L)

/* MessageId  : 0x000002a7 */
/* Approx. msg: ERROR_MEDIA_CHECK - The media may have changed. */
#define ERROR_MEDIA_CHECK	((ULONG)0x000002a7L)

/* MessageId  : 0x000002a8 */
/* Approx. msg: ERROR_GUID_SUBSTITUTION_MADE - During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended. */
#define ERROR_GUID_SUBSTITUTION_MADE	((ULONG)0x000002a8L)

/* MessageId  : 0x000002a9 */
/* Approx. msg: ERROR_STOPPED_ON_SYMLINK - The create operation stopped after reaching a symbolic link. */
#define ERROR_STOPPED_ON_SYMLINK	((ULONG)0x000002a9L)

/* MessageId  : 0x000002aa */
/* Approx. msg: ERROR_LONGJUMP - A long jump has been executed. */
#define ERROR_LONGJUMP	((ULONG)0x000002aaL)

/* MessageId  : 0x000002ab */
/* Approx. msg: ERROR_PLUGPLAY_QUERY_VETOED - The Plug and Play query operation was not successful. */
#define ERROR_PLUGPLAY_QUERY_VETOED	((ULONG)0x000002abL)

/* MessageId  : 0x000002ac */
/* Approx. msg: ERROR_UNWIND_CONSOLIDATE - A frame consolidation has been executed. */
#define ERROR_UNWIND_CONSOLIDATE	((ULONG)0x000002acL)

/* MessageId  : 0x000002ad */
/* Approx. msg: ERROR_REGISTRY_HIVE_RECOVERED - Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost. */
#define ERROR_REGISTRY_HIVE_RECOVERED	((ULONG)0x000002adL)

/* MessageId  : 0x000002ae */
/* Approx. msg: ERROR_DLL_MIGHT_BE_INSECURE - The application is attempting to run executable code from the module %hs. This may be insecure. An alternative, %hs, is available. Should the application use the secure module %hs? */
#define ERROR_DLL_MIGHT_BE_INSECURE	((ULONG)0x000002aeL)

/* MessageId  : 0x000002af */
/* Approx. msg: ERROR_DLL_MIGHT_BE_INCOMPATIBLE - The application is loading executable code from the module %hs. This is secure, but may be incompatible with previous releases of the operating system. An alternative, %hs, is available. Should the application use the secure module %hs? */
#define ERROR_DLL_MIGHT_BE_INCOMPATIBLE	((ULONG)0x000002afL)

/* MessageId  : 0x000002b0 */
/* Approx. msg: ERROR_DBG_EXCEPTION_NOT_HANDLED - Debugger did not handle the exception. */
#define ERROR_DBG_EXCEPTION_NOT_HANDLED	((ULONG)0x000002b0L)

/* MessageId  : 0x000002b1 */
/* Approx. msg: ERROR_DBG_REPLY_LATER - Debugger will reply later. */
#define ERROR_DBG_REPLY_LATER	((ULONG)0x000002b1L)

/* MessageId  : 0x000002b2 */
/* Approx. msg: ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE - Debugger can not provide handle. */
#define ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE	((ULONG)0x000002b2L)

/* MessageId  : 0x000002b3 */
/* Approx. msg: ERROR_DBG_TERMINATE_THREAD - Debugger terminated thread. */
#define ERROR_DBG_TERMINATE_THREAD	((ULONG)0x000002b3L)

/* MessageId  : 0x000002b4 */
/* Approx. msg: ERROR_DBG_TERMINATE_PROCESS - Debugger terminated process. */
#define ERROR_DBG_TERMINATE_PROCESS	((ULONG)0x000002b4L)

/* MessageId  : 0x000002b5 */
/* Approx. msg: ERROR_DBG_CONTROL_C - Debugger got control C. */
#define ERROR_DBG_CONTROL_C	((ULONG)0x000002b5L)

/* MessageId  : 0x000002b6 */
/* Approx. msg: ERROR_DBG_PRINTEXCEPTION_C - Debugger printed exception on control C. */
#define ERROR_DBG_PRINTEXCEPTION_C	((ULONG)0x000002b6L)

/* MessageId  : 0x000002b7 */
/* Approx. msg: ERROR_DBG_RIPEXCEPTION - Debugger received RIP exception. */
#define ERROR_DBG_RIPEXCEPTION	((ULONG)0x000002b7L)

/* MessageId  : 0x000002b8 */
/* Approx. msg: ERROR_DBG_CONTROL_BREAK - Debugger received control break. */
#define ERROR_DBG_CONTROL_BREAK	((ULONG)0x000002b8L)

/* MessageId  : 0x000002b9 */
/* Approx. msg: ERROR_DBG_COMMAND_EXCEPTION - Debugger command communication exception. */
#define ERROR_DBG_COMMAND_EXCEPTION	((ULONG)0x000002b9L)

/* MessageId  : 0x000002ba */
/* Approx. msg: ERROR_OBJECT_NAME_EXISTS - An attempt was made to create an object and the object name already existed. */
#define ERROR_OBJECT_NAME_EXISTS	((ULONG)0x000002baL)

/* MessageId  : 0x000002bb */
/* Approx. msg: ERROR_THREAD_WAS_SUSPENDED - A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded. */
#define ERROR_THREAD_WAS_SUSPENDED	((ULONG)0x000002bbL)

/* MessageId  : 0x000002bc */
/* Approx. msg: ERROR_IMAGE_NOT_AT_BASE - An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image. */
#define ERROR_IMAGE_NOT_AT_BASE	((ULONG)0x000002bcL)

/* MessageId  : 0x000002bd */
/* Approx. msg: ERROR_RXACT_STATE_CREATED - This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created. */
#define ERROR_RXACT_STATE_CREATED	((ULONG)0x000002bdL)

/* MessageId  : 0x000002be */
/* Approx. msg: ERROR_SEGMENT_NOTIFICATION - A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments. */
#define ERROR_SEGMENT_NOTIFICATION	((ULONG)0x000002beL)

/* MessageId  : 0x000002bf */
/* Approx. msg: ERROR_BAD_CURRENT_DIRECTORY - The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit. */
#define ERROR_BAD_CURRENT_DIRECTORY	((ULONG)0x000002bfL)

/* MessageId  : 0x000002c0 */
/* Approx. msg: ERROR_FT_READ_RECOVERY_FROM_BACKUP - To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device. */
#define ERROR_FT_READ_RECOVERY_FROM_BACKUP	((ULONG)0x000002c0L)

/* MessageId  : 0x000002c1 */
/* Approx. msg: ERROR_FT_WRITE_RECOVERY - To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device. */
#define ERROR_FT_WRITE_RECOVERY	((ULONG)0x000002c1L)

/* MessageId  : 0x000002c2 */
/* Approx. msg: ERROR_IMAGE_MACHINE_TYPE_MISMATCH - The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load. */
#define ERROR_IMAGE_MACHINE_TYPE_MISMATCH	((ULONG)0x000002c2L)

/* MessageId  : 0x000002c3 */
/* Approx. msg: ERROR_RECEIVE_PARTIAL - The network transport returned partial data to its client. The remaining data will be sent later. */
#define ERROR_RECEIVE_PARTIAL	((ULONG)0x000002c3L)

/* MessageId  : 0x000002c4 */
/* Approx. msg: ERROR_RECEIVE_EXPEDITED - The network transport returned data to its client that was marked as expedited by the remote system. */
#define ERROR_RECEIVE_EXPEDITED	((ULONG)0x000002c4L)

/* MessageId  : 0x000002c5 */
/* Approx. msg: ERROR_RECEIVE_PARTIAL_EXPEDITED - The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later. */
#define ERROR_RECEIVE_PARTIAL_EXPEDITED	((ULONG)0x000002c5L)

/* MessageId  : 0x000002c6 */
/* Approx. msg: ERROR_EVENT_DONE - The TDI indication has completed successfully. */
#define ERROR_EVENT_DONE	((ULONG)0x000002c6L)

/* MessageId  : 0x000002c7 */
/* Approx. msg: ERROR_EVENT_PENDING - The TDI indication has entered the pending state. */
#define ERROR_EVENT_PENDING	((ULONG)0x000002c7L)

/* MessageId  : 0x000002c8 */
/* Approx. msg: ERROR_CHECKING_FILE_SYSTEM - Checking file system on %wZ. */
#define ERROR_CHECKING_FILE_SYSTEM	((ULONG)0x000002c8L)

/* MessageId  : 0x000002ca */
/* Approx. msg: ERROR_PREDEFINED_HANDLE - The specified registry key is referenced by a predefined handle. */
#define ERROR_PREDEFINED_HANDLE	((ULONG)0x000002caL)

/* MessageId  : 0x000002cb */
/* Approx. msg: ERROR_WAS_UNLOCKED - The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process. */
#define ERROR_WAS_UNLOCKED	((ULONG)0x000002cbL)

/* MessageId  : 0x000002cd */
/* Approx. msg: ERROR_WAS_LOCKED - One of the pages to lock was already locked. */
#define ERROR_WAS_LOCKED	((ULONG)0x000002cdL)

/* MessageId  : 0x000002d0 */
/* Approx. msg: ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE - The image file %hs is valid, but is for a machine type other than the current machine. */
#define ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE	((ULONG)0x000002d0L)

/* MessageId  : 0x000002d1 */
/* Approx. msg: ERROR_NO_YIELD_PERFORMED - A yield execution was performed and no thread was available to run. */
#define ERROR_NO_YIELD_PERFORMED	((ULONG)0x000002d1L)

/* MessageId  : 0x000002d2 */
/* Approx. msg: ERROR_TIMER_RESUME_IGNORED - The resumable flag to a timer API was ignored. */
#define ERROR_TIMER_RESUME_IGNORED	((ULONG)0x000002d2L)

/* MessageId  : 0x000002d3 */
/* Approx. msg: ERROR_ARBITRATION_UNHANDLED - The arbiter has deferred arbitration of these resources to its parent. */
#define ERROR_ARBITRATION_UNHANDLED	((ULONG)0x000002d3L)

/* MessageId  : 0x000002d4 */
/* Approx. msg: ERROR_CARDBUS_NOT_SUPPORTED - The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller. */
#define ERROR_CARDBUS_NOT_SUPPORTED	((ULONG)0x000002d4L)

/* MessageId  : 0x000002d5 */
/* Approx. msg: ERROR_MP_PROCESSOR_MISMATCH - The CPUs in this multiprocessor system are not all the same revision level. To use all processors the operating system restricts itself to the features of the least capable processor in the system. Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported. */
#define ERROR_MP_PROCESSOR_MISMATCH	((ULONG)0x000002d5L)

/* MessageId  : 0x000002d6 */
/* Approx. msg: ERROR_HIBERNATED - The system was put into hibernation. */
#define ERROR_HIBERNATED	((ULONG)0x000002d6L)

/* MessageId  : 0x000002d7 */
/* Approx. msg: ERROR_RESUME_HIBERNATION - The system was resumed from hibernation. */
#define ERROR_RESUME_HIBERNATION	((ULONG)0x000002d7L)

/* MessageId  : 0x000002d8 */
/* Approx. msg: ERROR_FIRMWARE_UPDATED - Windows has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3]. */
#define ERROR_FIRMWARE_UPDATED	((ULONG)0x000002d8L)

/* MessageId  : 0x000002d9 */
/* Approx. msg: ERROR_DRIVERS_LEAKING_LOCKED_PAGES - A device driver is leaking locked I/O pages causing system degradation. The system has automatically enabled tracking code in order to try and catch the culprit. */
#define ERROR_DRIVERS_LEAKING_LOCKED_PAGES	((ULONG)0x000002d9L)

/* MessageId  : 0x000002da */
/* Approx. msg: ERROR_WAKE_SYSTEM - The system has awoken */
#define ERROR_WAKE_SYSTEM	((ULONG)0x000002daL)

/* MessageId  : 0x000002e5 */
/* Approx. msg: ERROR_REPARSE - A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link. */
#define ERROR_REPARSE	((ULONG)0x000002e5L)

/* MessageId  : 0x000002e6 */
/* Approx. msg: ERROR_OPLOCK_BREAK_IN_PROGRESS - An open/create operation completed while an oplock break is underway. */
#define ERROR_OPLOCK_BREAK_IN_PROGRESS	((ULONG)0x000002e6L)

/* MessageId  : 0x000002e7 */
/* Approx. msg: ERROR_VOLUME_MOUNTED - A new volume has been mounted by a file system. */
#define ERROR_VOLUME_MOUNTED	((ULONG)0x000002e7L)

/* MessageId  : 0x000002e8 */
/* Approx. msg: ERROR_RXACT_COMMITTED - This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed. */
#define ERROR_RXACT_COMMITTED	((ULONG)0x000002e8L)

/* MessageId  : 0x000002e9 */
/* Approx. msg: ERROR_NOTIFY_CLEANUP - This indicates that a notify change request has been completed due to closing the handle which made the notify change request. */
#define ERROR_NOTIFY_CLEANUP	((ULONG)0x000002e9L)

/* MessageId  : 0x000002ea */
/* Approx. msg: ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED - An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport. */
#define ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED	((ULONG)0x000002eaL)

/* MessageId  : 0x000002eb */
/* Approx. msg: ERROR_PAGE_FAULT_TRANSITION - Page fault was a transition fault. */
#define ERROR_PAGE_FAULT_TRANSITION	((ULONG)0x000002ebL)

/* MessageId  : 0x000002ec */
/* Approx. msg: ERROR_PAGE_FAULT_DEMAND_ZERO - Page fault was a demand zero fault. */
#define ERROR_PAGE_FAULT_DEMAND_ZERO	((ULONG)0x000002ecL)

/* MessageId  : 0x000002ed */
/* Approx. msg: ERROR_PAGE_FAULT_COPY_ON_WRITE - Page fault was a demand zero fault. */
#define ERROR_PAGE_FAULT_COPY_ON_WRITE	((ULONG)0x000002edL)

/* MessageId  : 0x000002ee */
/* Approx. msg: ERROR_PAGE_FAULT_GUARD_PAGE - Page fault was a demand zero fault. */
#define ERROR_PAGE_FAULT_GUARD_PAGE	((ULONG)0x000002eeL)

/* MessageId  : 0x000002ef */
/* Approx. msg: ERROR_PAGE_FAULT_PAGING_FILE - Page fault was satisfied by reading from a secondary storage device. */
#define ERROR_PAGE_FAULT_PAGING_FILE	((ULONG)0x000002efL)

/* MessageId  : 0x000002f0 */
/* Approx. msg: ERROR_CACHE_PAGE_LOCKED - Cached page was locked during operation. */
#define ERROR_CACHE_PAGE_LOCKED	((ULONG)0x000002f0L)

/* MessageId  : 0x000002f1 */
/* Approx. msg: ERROR_CRASH_DUMP - Crash dump exists in paging file. */
#define ERROR_CRASH_DUMP	((ULONG)0x000002f1L)

/* MessageId  : 0x000002f2 */
/* Approx. msg: ERROR_BUFFER_ALL_ZEROS - Specified buffer contains all zeros. */
#define ERROR_BUFFER_ALL_ZEROS	((ULONG)0x000002f2L)

/* MessageId  : 0x000002f3 */
/* Approx. msg: ERROR_REPARSE_OBJECT - A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link. */
#define ERROR_REPARSE_OBJECT	((ULONG)0x000002f3L)

/* MessageId  : 0x000002f4 */
/* Approx. msg: ERROR_RESOURCE_REQUIREMENTS_CHANGED - The device has succeeded a query-stop and its resource requirements have changed. */
#define ERROR_RESOURCE_REQUIREMENTS_CHANGED	((ULONG)0x000002f4L)

/* MessageId  : 0x000002f5 */
/* Approx. msg: ERROR_TRANSLATION_COMPLETE - The translator has translated these resources into the global space and no further translations should be performed. */
#define ERROR_TRANSLATION_COMPLETE	((ULONG)0x000002f5L)

/* MessageId  : 0x000002f6 */
/* Approx. msg: ERROR_NOTHING_TO_TERMINATE - A process being terminated has no threads to terminate. */
#define ERROR_NOTHING_TO_TERMINATE	((ULONG)0x000002f6L)

/* MessageId  : 0x000002f7 */
/* Approx. msg: ERROR_PROCESS_NOT_IN_JOB - The specified process is not part of a job. */
#define ERROR_PROCESS_NOT_IN_JOB	((ULONG)0x000002f7L)

/* MessageId  : 0x000002f8 */
/* Approx. msg: ERROR_PROCESS_IN_JOB - The specified process is part of a job. */
#define ERROR_PROCESS_IN_JOB	((ULONG)0x000002f8L)

/* MessageId  : 0x000002f9 */
/* Approx. msg: ERROR_VOLSNAP_HIBERNATE_READY - The system is now ready for hibernation. */
#define ERROR_VOLSNAP_HIBERNATE_READY	((ULONG)0x000002f9L)

/* MessageId  : 0x000002fa */
/* Approx. msg: ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY - A file system or file system filter driver has successfully completed an FsFilter operation. */
#define ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY	((ULONG)0x000002faL)

/* MessageId  : 0x000002fb */
/* Approx. msg: ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED - The specified interrupt vector was already connected. */
#define ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED	((ULONG)0x000002fbL)

/* MessageId  : 0x000002fc */
/* Approx. msg: ERROR_INTERRUPT_STILL_CONNECTED - The specified interrupt vector is still connected. */
#define ERROR_INTERRUPT_STILL_CONNECTED	((ULONG)0x000002fcL)

/* MessageId  : 0x000002fd */
/* Approx. msg: ERROR_WAIT_FOR_OPLOCK - An operation is blocked waiting for an oplock. */
#define ERROR_WAIT_FOR_OPLOCK	((ULONG)0x000002fdL)

/* MessageId  : 0x000002fe */
/* Approx. msg: ERROR_DBG_EXCEPTION_HANDLED - Debugger handled exception. */
#define ERROR_DBG_EXCEPTION_HANDLED	((ULONG)0x000002feL)

/* MessageId  : 0x000002ff */
/* Approx. msg: ERROR_DBG_CONTINUE - Debugger continued */
#define ERROR_DBG_CONTINUE	((ULONG)0x000002ffL)

/* MessageId  : 0x00000300 */
/* Approx. msg: ERROR_CALLBACK_POP_STACK - An exception occurred in a user mode callback and the kernel callback frame should be removed. */
#define ERROR_CALLBACK_POP_STACK	((ULONG)0x00000300L)

/* MessageId  : 0x00000301 */
/* Approx. msg: ERROR_COMPRESSION_DISABLED - Compression is disabled for this volume. */
#define ERROR_COMPRESSION_DISABLED	((ULONG)0x00000301L)

/* MessageId  : 0x00000302 */
/* Approx. msg: ERROR_CANTFETCHBACKWARDS - The data provider cannot fetch backwards through a result set. */
#define ERROR_CANTFETCHBACKWARDS	((ULONG)0x00000302L)

/* MessageId  : 0x00000303 */
/* Approx. msg: ERROR_CANTSCROLLBACKWARDS - The data provider cannot scroll backwards through a result set. */
#define ERROR_CANTSCROLLBACKWARDS	((ULONG)0x00000303L)

/* MessageId  : 0x00000304 */
/* Approx. msg: ERROR_ROWSNOTRELEASED - The data provider requires that previously fetched data is released before asking for more data. */
#define ERROR_ROWSNOTRELEASED	((ULONG)0x00000304L)

/* MessageId  : 0x00000305 */
/* Approx. msg: ERROR_BAD_ACCESSOR_FLAGS - The data provider was not able to interpret the flags set for a column binding in an accessor. */
#define ERROR_BAD_ACCESSOR_FLAGS	((ULONG)0x00000305L)

/* MessageId  : 0x00000306 */
/* Approx. msg: ERROR_ERRORS_ENCOUNTERED - One or more errors occurred while processing the request. */
#define ERROR_ERRORS_ENCOUNTERED	((ULONG)0x00000306L)

/* MessageId  : 0x00000307 */
/* Approx. msg: ERROR_NOT_CAPABLE - The implementation is not capable of performing the request. */
#define ERROR_NOT_CAPABLE	((ULONG)0x00000307L)

/* MessageId  : 0x00000308 */
/* Approx. msg: ERROR_REQUEST_OUT_OF_SEQUENCE - The client of a component requested an operation which is not valid given the state of the component instance. */
#define ERROR_REQUEST_OUT_OF_SEQUENCE	((ULONG)0x00000308L)

/* MessageId  : 0x00000309 */
/* Approx. msg: ERROR_VERSION_PARSE_ERROR - A version number could not be parsed. */
#define ERROR_VERSION_PARSE_ERROR	((ULONG)0x00000309L)

/* MessageId  : 0x0000030a */
/* Approx. msg: ERROR_BADSTARTPOSITION - The iterator's start position is invalid. */
#define ERROR_BADSTARTPOSITION	((ULONG)0x0000030aL)

/* MessageId  : 0x000003e2 */
/* Approx. msg: ERROR_EA_ACCESS_DENIED - Access to the extended attribute was denied. */
#define ERROR_EA_ACCESS_DENIED	((ULONG)0x000003e2L)

/* MessageId  : 0x000003e3 */
/* Approx. msg: ERROR_OPERATION_ABORTED - The I/O operation has been aborted because of either a thread exit or an application request. */
#define ERROR_OPERATION_ABORTED	((ULONG)0x000003e3L)

/* MessageId  : 0x000003e4 */
/* Approx. msg: ERROR_IO_INCOMPLETE - Overlapped I/O event is not in a signaled state. */
#define ERROR_IO_INCOMPLETE	((ULONG)0x000003e4L)

/* MessageId  : 0x000003e5 */
/* Approx. msg: ERROR_IO_PENDING - Overlapped I/O operation is in progress. */
#define ERROR_IO_PENDING	((ULONG)0x000003e5L)

/* MessageId  : 0x000003e6 */
/* Approx. msg: ERROR_NOACCESS - Invalid access to memory location. */
#define ERROR_NOACCESS	((ULONG)0x000003e6L)

/* MessageId  : 0x000003e7 */
/* Approx. msg: ERROR_SWAPERROR - Error performing inpage operation. */
#define ERROR_SWAPERROR	((ULONG)0x000003e7L)

/* MessageId  : 0x000003e9 */
/* Approx. msg: ERROR_STACK_OVERFLOW - Recursion too deep; the stack overflowed. */
#define ERROR_STACK_OVERFLOW	((ULONG)0x000003e9L)

/* MessageId  : 0x000003ea */
/* Approx. msg: ERROR_INVALID_MESSAGE - The window cannot act on the sent message. */
#define ERROR_INVALID_MESSAGE	((ULONG)0x000003eaL)

/* MessageId  : 0x000003eb */
/* Approx. msg: ERROR_CAN_NOT_COMPLETE - Cannot complete this function. */
#define ERROR_CAN_NOT_COMPLETE	((ULONG)0x000003ebL)

/* MessageId  : 0x000003ec */
/* Approx. msg: ERROR_INVALID_FLAGS - Invalid flags. */
#define ERROR_INVALID_FLAGS	((ULONG)0x000003ecL)

/* MessageId  : 0x000003ed */
/* Approx. msg: ERROR_UNRECOGNIZED_VOLUME - The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted. */
#define ERROR_UNRECOGNIZED_VOLUME	((ULONG)0x000003edL)

/* MessageId  : 0x000003ee */
/* Approx. msg: ERROR_FILE_INVALID - The volume for a file has been externally altered so that the opened file is no longer valid. */
#define ERROR_FILE_INVALID	((ULONG)0x000003eeL)

/* MessageId  : 0x000003ef */
/* Approx. msg: ERROR_FULLSCREEN_MODE - The requested operation cannot be performed in full-screen mode. */
#define ERROR_FULLSCREEN_MODE	((ULONG)0x000003efL)

/* MessageId  : 0x000003f0 */
/* Approx. msg: ERROR_NO_TOKEN - An attempt was made to reference a token that does not exist. */
#define ERROR_NO_TOKEN	((ULONG)0x000003f0L)

/* MessageId  : 0x000003f1 */
/* Approx. msg: ERROR_BADDB - The configuration registry database is corrupt. */
#define ERROR_BADDB	((ULONG)0x000003f1L)

/* MessageId  : 0x000003f2 */
/* Approx. msg: ERROR_BADKEY - The configuration registry key is invalid. */
#define ERROR_BADKEY	((ULONG)0x000003f2L)

/* MessageId  : 0x000003f3 */
/* Approx. msg: ERROR_CANTOPEN - The configuration registry key could not be opened. */
#define ERROR_CANTOPEN	((ULONG)0x000003f3L)

/* MessageId  : 0x000003f4 */
/* Approx. msg: ERROR_CANTREAD - The configuration registry key could not be read. */
#define ERROR_CANTREAD	((ULONG)0x000003f4L)

/* MessageId  : 0x000003f5 */
/* Approx. msg: ERROR_CANTWRITE - The configuration registry key could not be written. */
#define ERROR_CANTWRITE	((ULONG)0x000003f5L)

/* MessageId  : 0x000003f6 */
/* Approx. msg: ERROR_REGISTRY_RECOVERED - One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful. */
#define ERROR_REGISTRY_RECOVERED	((ULONG)0x000003f6L)

/* MessageId  : 0x000003f7 */
/* Approx. msg: ERROR_REGISTRY_CORRUPT - The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted. */
#define ERROR_REGISTRY_CORRUPT	((ULONG)0x000003f7L)

/* MessageId  : 0x000003f8 */
/* Approx. msg: ERROR_REGISTRY_IO_FAILED - An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry. */
#define ERROR_REGISTRY_IO_FAILED	((ULONG)0x000003f8L)

/* MessageId  : 0x000003f9 */
/* Approx. msg: ERROR_NOT_REGISTRY_FILE - The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format. */
#define ERROR_NOT_REGISTRY_FILE	((ULONG)0x000003f9L)

/* MessageId  : 0x000003fa */
/* Approx. msg: ERROR_KEY_DELETED - Illegal operation attempted on a registry key that has been marked for deletion. */
#define ERROR_KEY_DELETED	((ULONG)0x000003faL)

/* MessageId  : 0x000003fb */
/* Approx. msg: ERROR_NO_LOG_SPACE - System could not allocate the required space in a registry log. */
#define ERROR_NO_LOG_SPACE	((ULONG)0x000003fbL)

/* MessageId  : 0x000003fc */
/* Approx. msg: ERROR_KEY_HAS_CHILDREN - Cannot create a symbolic link in a registry key that already has subkeys or values. */
#define ERROR_KEY_HAS_CHILDREN	((ULONG)0x000003fcL)

/* MessageId  : 0x000003fd */
/* Approx. msg: ERROR_CHILD_MUST_BE_VOLATILE - Cannot create a stable subkey under a volatile parent key. */
#define ERROR_CHILD_MUST_BE_VOLATILE	((ULONG)0x000003fdL)

/* MessageId  : 0x000003fe */
/* Approx. msg: ERROR_NOTIFY_ENUM_DIR - A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes. */
#define ERROR_NOTIFY_ENUM_DIR	((ULONG)0x000003feL)

/* MessageId  : 0x0000041b */
/* Approx. msg: ERROR_DEPENDENT_SERVICES_RUNNING - A stop control has been sent to a service that other running services are dependent on. */
#define ERROR_DEPENDENT_SERVICES_RUNNING	((ULONG)0x0000041bL)

/* MessageId  : 0x0000041c */
/* Approx. msg: ERROR_INVALID_SERVICE_CONTROL - The requested control is not valid for this service. */
#define ERROR_INVALID_SERVICE_CONTROL	((ULONG)0x0000041cL)

/* MessageId  : 0x0000041d */
/* Approx. msg: ERROR_SERVICE_REQUEST_TIMEOUT - The service did not respond to the start or control request in a timely fashion. */
#define ERROR_SERVICE_REQUEST_TIMEOUT	((ULONG)0x0000041dL)

/* MessageId  : 0x0000041e */
/* Approx. msg: ERROR_SERVICE_NO_THREAD - A thread could not be created for the service. */
#define ERROR_SERVICE_NO_THREAD	((ULONG)0x0000041eL)

/* MessageId  : 0x0000041f */
/* Approx. msg: ERROR_SERVICE_DATABASE_LOCKED - The service database is locked. */
#define ERROR_SERVICE_DATABASE_LOCKED	((ULONG)0x0000041fL)

/* MessageId  : 0x00000420 */
/* Approx. msg: ERROR_SERVICE_ALREADY_RUNNING - An instance of the service is already running. */
#define ERROR_SERVICE_ALREADY_RUNNING	((ULONG)0x00000420L)

/* MessageId  : 0x00000421 */
/* Approx. msg: ERROR_INVALID_SERVICE_ACCOUNT - The account name is invalid or does not exist, or the password is invalid for the account name specified. */
#define ERROR_INVALID_SERVICE_ACCOUNT	((ULONG)0x00000421L)

/* MessageId  : 0x00000422 */
/* Approx. msg: ERROR_SERVICE_DISABLED - The service cannot be started, either because it is disabled or because it has no enabled devices associated with it. */
#define ERROR_SERVICE_DISABLED	((ULONG)0x00000422L)

/* MessageId  : 0x00000423 */
/* Approx. msg: ERROR_CIRCULAR_DEPENDENCY - Circular service dependency was specified. */
#define ERROR_CIRCULAR_DEPENDENCY	((ULONG)0x00000423L)

/* MessageId  : 0x00000424 */
/* Approx. msg: ERROR_SERVICE_DOES_NOT_EXIST - The specified service does not exist as an installed service. */
#define ERROR_SERVICE_DOES_NOT_EXIST	((ULONG)0x00000424L)

/* MessageId  : 0x00000425 */
/* Approx. msg: ERROR_SERVICE_CANNOT_ACCEPT_CTRL - The service cannot accept control messages at this time. */
#define ERROR_SERVICE_CANNOT_ACCEPT_CTRL	((ULONG)0x00000425L)

/* MessageId  : 0x00000426 */
/* Approx. msg: ERROR_SERVICE_NOT_ACTIVE - The service has not been started. */
#define ERROR_SERVICE_NOT_ACTIVE	((ULONG)0x00000426L)

/* MessageId  : 0x00000427 */
/* Approx. msg: ERROR_FAILED_SERVICE_CONTROLLER_CONNECT - The service process could not connect to the service controller. */
#define ERROR_FAILED_SERVICE_CONTROLLER_CONNECT	((ULONG)0x00000427L)

/* MessageId  : 0x00000428 */
/* Approx. msg: ERROR_EXCEPTION_IN_SERVICE - An exception occurred in the service when handling the control request. */
#define ERROR_EXCEPTION_IN_SERVICE	((ULONG)0x00000428L)

/* MessageId  : 0x00000429 */
/* Approx. msg: ERROR_DATABASE_DOES_NOT_EXIST - The database specified does not exist. */
#define ERROR_DATABASE_DOES_NOT_EXIST	((ULONG)0x00000429L)

/* MessageId  : 0x0000042a */
/* Approx. msg: ERROR_SERVICE_SPECIFIC_ERROR - The service has returned a service-specific error code. */
#define ERROR_SERVICE_SPECIFIC_ERROR	((ULONG)0x0000042aL)

/* MessageId  : 0x0000042b */
/* Approx. msg: ERROR_PROCESS_ABORTED - The process terminated unexpectedly. */
#define ERROR_PROCESS_ABORTED	((ULONG)0x0000042bL)

/* MessageId  : 0x0000042c */
/* Approx. msg: ERROR_SERVICE_DEPENDENCY_FAIL - The dependency service or group failed to start. */
#define ERROR_SERVICE_DEPENDENCY_FAIL	((ULONG)0x0000042cL)

/* MessageId  : 0x0000042d */
/* Approx. msg: ERROR_SERVICE_LOGON_FAILED - The service did not start due to a logon failure. */
#define ERROR_SERVICE_LOGON_FAILED	((ULONG)0x0000042dL)

/* MessageId  : 0x0000042e */
/* Approx. msg: ERROR_SERVICE_START_HANG - After starting, the service hung in a start-pending state. */
#define ERROR_SERVICE_START_HANG	((ULONG)0x0000042eL)

/* MessageId  : 0x0000042f */
/* Approx. msg: ERROR_INVALID_SERVICE_LOCK - The specified service database lock is invalid. */
#define ERROR_INVALID_SERVICE_LOCK	((ULONG)0x0000042fL)

/* MessageId  : 0x00000430 */
/* Approx. msg: ERROR_SERVICE_MARKED_FOR_DELETE - The specified service has been marked for deletion. */
#define ERROR_SERVICE_MARKED_FOR_DELETE	((ULONG)0x00000430L)

/* MessageId  : 0x00000431 */
/* Approx. msg: ERROR_SERVICE_EXISTS - The specified service already exists. */
#define ERROR_SERVICE_EXISTS	((ULONG)0x00000431L)

/* MessageId  : 0x00000432 */
/* Approx. msg: ERROR_ALREADY_RUNNING_LKG - The system is currently running with the last-known-good configuration. */
#define ERROR_ALREADY_RUNNING_LKG	((ULONG)0x00000432L)

/* MessageId  : 0x00000433 */
/* Approx. msg: ERROR_SERVICE_DEPENDENCY_DELETED - The dependency service does not exist or has been marked for deletion. */
#define ERROR_SERVICE_DEPENDENCY_DELETED	((ULONG)0x00000433L)

/* MessageId  : 0x00000434 */
/* Approx. msg: ERROR_BOOT_ALREADY_ACCEPTED - The current boot has already been accepted for use as the last-known-good control set. */
#define ERROR_BOOT_ALREADY_ACCEPTED	((ULONG)0x00000434L)

/* MessageId  : 0x00000435 */
/* Approx. msg: ERROR_SERVICE_NEVER_STARTED - No attempts to start the service have been made since the last boot. */
#define ERROR_SERVICE_NEVER_STARTED	((ULONG)0x00000435L)

/* MessageId  : 0x00000436 */
/* Approx. msg: ERROR_DUPLICATE_SERVICE_NAME - The name is already in use as either a service name or a service display name. */
#define ERROR_DUPLICATE_SERVICE_NAME	((ULONG)0x00000436L)

/* MessageId  : 0x00000437 */
/* Approx. msg: ERROR_DIFFERENT_SERVICE_ACCOUNT - The account specified for this service is different from the account specified for other services running in the same process. */
#define ERROR_DIFFERENT_SERVICE_ACCOUNT	((ULONG)0x00000437L)

/* MessageId  : 0x00000438 */
/* Approx. msg: ERROR_CANNOT_DETECT_DRIVER_FAILURE - Failure actions can only be set for Win32 services, not for drivers. */
#define ERROR_CANNOT_DETECT_DRIVER_FAILURE	((ULONG)0x00000438L)

/* MessageId  : 0x00000439 */
/* Approx. msg: ERROR_CANNOT_DETECT_PROCESS_ABORT - This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly. */
#define ERROR_CANNOT_DETECT_PROCESS_ABORT	((ULONG)0x00000439L)

/* MessageId  : 0x0000043a */
/* Approx. msg: ERROR_NO_RECOVERY_PROGRAM - No recovery program has been configured for this service. */
#define ERROR_NO_RECOVERY_PROGRAM	((ULONG)0x0000043aL)

/* MessageId  : 0x0000043b */
/* Approx. msg: ERROR_SERVICE_NOT_IN_EXE - The executable program that this service is configured to run in does not implement the service. */
#define ERROR_SERVICE_NOT_IN_EXE	((ULONG)0x0000043bL)

/* MessageId  : 0x0000043c */
/* Approx. msg: ERROR_NOT_SAFEBOOT_SERVICE - This service cannot be started in Safe Mode. */
#define ERROR_NOT_SAFEBOOT_SERVICE	((ULONG)0x0000043cL)

/* MessageId  : 0x0000044c */
/* Approx. msg: ERROR_END_OF_MEDIA - The physical end of the tape has been reached. */
#define ERROR_END_OF_MEDIA	((ULONG)0x0000044cL)

/* MessageId  : 0x0000044d */
/* Approx. msg: ERROR_FILEMARK_DETECTED - A tape access reached a filemark. */
#define ERROR_FILEMARK_DETECTED	((ULONG)0x0000044dL)

/* MessageId  : 0x0000044e */
/* Approx. msg: ERROR_BEGINNING_OF_MEDIA - The beginning of the tape or a partition was encountered. */
#define ERROR_BEGINNING_OF_MEDIA	((ULONG)0x0000044eL)

/* MessageId  : 0x0000044f */
/* Approx. msg: ERROR_SETMARK_DETECTED - A tape access reached the end of a set of files. */
#define ERROR_SETMARK_DETECTED	((ULONG)0x0000044fL)

/* MessageId  : 0x00000450 */
/* Approx. msg: ERROR_NO_DATA_DETECTED - No more data is on the tape. */
#define ERROR_NO_DATA_DETECTED	((ULONG)0x00000450L)

/* MessageId  : 0x00000451 */
/* Approx. msg: ERROR_PARTITION_FAILURE - Tape could not be partitioned. */
#define ERROR_PARTITION_FAILURE	((ULONG)0x00000451L)

/* MessageId  : 0x00000452 */
/* Approx. msg: ERROR_INVALID_BLOCK_LENGTH - When accessing a new tape of a multivolume partition, the current block size is incorrect. */
#define ERROR_INVALID_BLOCK_LENGTH	((ULONG)0x00000452L)

/* MessageId  : 0x00000453 */
/* Approx. msg: ERROR_DEVICE_NOT_PARTITIONED - Tape partition information could not be found when loading a tape. */
#define ERROR_DEVICE_NOT_PARTITIONED	((ULONG)0x00000453L)

/* MessageId  : 0x00000454 */
/* Approx. msg: ERROR_UNABLE_TO_LOCK_MEDIA - Unable to lock the media eject mechanism. */
#define ERROR_UNABLE_TO_LOCK_MEDIA	((ULONG)0x00000454L)

/* MessageId  : 0x00000455 */
/* Approx. msg: ERROR_UNABLE_TO_UNLOAD_MEDIA - Unable to unload the media. */
#define ERROR_UNABLE_TO_UNLOAD_MEDIA	((ULONG)0x00000455L)

/* MessageId  : 0x00000456 */
/* Approx. msg: ERROR_MEDIA_CHANGED - The media in the drive may have changed. */
#define ERROR_MEDIA_CHANGED	((ULONG)0x00000456L)

/* MessageId  : 0x00000457 */
/* Approx. msg: ERROR_BUS_RESET - The I/O bus was reset. */
#define ERROR_BUS_RESET	((ULONG)0x00000457L)

/* MessageId  : 0x00000458 */
/* Approx. msg: ERROR_NO_MEDIA_IN_DRIVE - No media in drive. */
#define ERROR_NO_MEDIA_IN_DRIVE	((ULONG)0x00000458L)

/* MessageId  : 0x00000459 */
/* Approx. msg: ERROR_NO_UNICODE_TRANSLATION - No mapping for the Unicode character exists in the target multi-byte code page. */
#define ERROR_NO_UNICODE_TRANSLATION	((ULONG)0x00000459L)

/* MessageId  : 0x0000045a */
/* Approx. msg: ERROR_DLL_INIT_FAILED - A dynamic link library (DLL) initialization routine failed. */
#define ERROR_DLL_INIT_FAILED	((ULONG)0x0000045aL)

/* MessageId  : 0x0000045b */
/* Approx. msg: ERROR_SHUTDOWN_IN_PROGRESS - A system shutdown is in progress. */
#define ERROR_SHUTDOWN_IN_PROGRESS	((ULONG)0x0000045bL)

/* MessageId  : 0x0000045c */
/* Approx. msg: ERROR_NO_SHUTDOWN_IN_PROGRESS - Unable to abort the system shutdown because no shutdown was in progress. */
#define ERROR_NO_SHUTDOWN_IN_PROGRESS	((ULONG)0x0000045cL)

/* MessageId  : 0x0000045d */
/* Approx. msg: ERROR_IO_DEVICE - The request could not be performed because of an I/O device error. */
#define ERROR_IO_DEVICE	((ULONG)0x0000045dL)

/* MessageId  : 0x0000045e */
/* Approx. msg: ERROR_SERIAL_NO_DEVICE - No serial device was successfully initialized. The serial driver will unload. */
#define ERROR_SERIAL_NO_DEVICE	((ULONG)0x0000045eL)

/* MessageId  : 0x0000045f */
/* Approx. msg: ERROR_IRQ_BUSY - Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened. */
#define ERROR_IRQ_BUSY	((ULONG)0x0000045fL)

/* MessageId  : 0x00000460 */
/* Approx. msg: ERROR_MORE_WRITES - A serial I/O operation was completed by another write to the serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.) */
#define ERROR_MORE_WRITES	((ULONG)0x00000460L)

/* MessageId  : 0x00000461 */
/* Approx. msg: ERROR_COUNTER_TIMEOUT - A serial I/O operation completed because the timeout period expired. (The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.) */
#define ERROR_COUNTER_TIMEOUT	((ULONG)0x00000461L)

/* MessageId  : 0x00000462 */
/* Approx. msg: ERROR_FLOPPY_ID_MARK_NOT_FOUND - No ID address mark was found on the floppy disk. */
#define ERROR_FLOPPY_ID_MARK_NOT_FOUND	((ULONG)0x00000462L)

/* MessageId  : 0x00000463 */
/* Approx. msg: ERROR_FLOPPY_WRONG_CYLINDER - Mismatch between the floppy disk sector ID field and the floppy disk controller track address. */
#define ERROR_FLOPPY_WRONG_CYLINDER	((ULONG)0x00000463L)

/* MessageId  : 0x00000464 */
/* Approx. msg: ERROR_FLOPPY_UNKNOWN_ERROR - The floppy disk controller reported an error that is not recognized by the floppy disk driver. */
#define ERROR_FLOPPY_UNKNOWN_ERROR	((ULONG)0x00000464L)

/* MessageId  : 0x00000465 */
/* Approx. msg: ERROR_FLOPPY_BAD_REGISTERS - The floppy disk controller returned inconsistent results in its registers. */
#define ERROR_FLOPPY_BAD_REGISTERS	((ULONG)0x00000465L)

/* MessageId  : 0x00000466 */
/* Approx. msg: ERROR_DISK_RECALIBRATE_FAILED - While accessing the hard disk, a recalibrate operation failed, even after retries. */
#define ERROR_DISK_RECALIBRATE_FAILED	((ULONG)0x00000466L)

/* MessageId  : 0x00000467 */
/* Approx. msg: ERROR_DISK_OPERATION_FAILED - While accessing the hard disk, a disk operation failed even after retries. */
#define ERROR_DISK_OPERATION_FAILED	((ULONG)0x00000467L)

/* MessageId  : 0x00000468 */
/* Approx. msg: ERROR_DISK_RESET_FAILED - While accessing the hard disk, a disk controller reset was needed, but even that failed. */
#define ERROR_DISK_RESET_FAILED	((ULONG)0x00000468L)

/* MessageId  : 0x00000469 */
/* Approx. msg: ERROR_EOM_OVERFLOW - Physical end of tape encountered. */
#define ERROR_EOM_OVERFLOW	((ULONG)0x00000469L)

/* MessageId  : 0x0000046a */
/* Approx. msg: ERROR_NOT_ENOUGH_SERVER_MEMORY - Not enough server storage is available to process this command. */
#define ERROR_NOT_ENOUGH_SERVER_MEMORY	((ULONG)0x0000046aL)

/* MessageId  : 0x0000046b */
/* Approx. msg: ERROR_POSSIBLE_DEADLOCK - A potential deadlock condition has been detected. */
#define ERROR_POSSIBLE_DEADLOCK	((ULONG)0x0000046bL)

/* MessageId  : 0x0000046c */
/* Approx. msg: ERROR_MAPPED_ALIGNMENT - The base address or the file offset specified does not have the proper alignment. */
#define ERROR_MAPPED_ALIGNMENT	((ULONG)0x0000046cL)

/* MessageId  : 0x00000474 */
/* Approx. msg: ERROR_SET_POWER_STATE_VETOED - An attempt to change the system power state was vetoed by another application or driver. */
#define ERROR_SET_POWER_STATE_VETOED	((ULONG)0x00000474L)

/* MessageId  : 0x00000475 */
/* Approx. msg: ERROR_SET_POWER_STATE_FAILED - The system BIOS failed an attempt to change the system power state. */
#define ERROR_SET_POWER_STATE_FAILED	((ULONG)0x00000475L)

/* MessageId  : 0x00000476 */
/* Approx. msg: ERROR_TOO_MANY_LINKS - An attempt was made to create more links on a file than the file system supports. */
#define ERROR_TOO_MANY_LINKS	((ULONG)0x00000476L)

/* MessageId  : 0x0000047e */
/* Approx. msg: ERROR_OLD_WIN_VERSION - The specified program requires a newer version of Windows. */
#define ERROR_OLD_WIN_VERSION	((ULONG)0x0000047eL)

/* MessageId  : 0x0000047f */
/* Approx. msg: ERROR_APP_WRONG_OS - The specified program is not a Windows or MS-DOS program. */
#define ERROR_APP_WRONG_OS	((ULONG)0x0000047fL)

/* MessageId  : 0x00000480 */
/* Approx. msg: ERROR_SINGLE_INSTANCE_APP - Cannot start more than one instance of the specified program. */
#define ERROR_SINGLE_INSTANCE_APP	((ULONG)0x00000480L)

/* MessageId  : 0x00000481 */
/* Approx. msg: ERROR_RMODE_APP - The specified program was written for an earlier version of Windows. */
#define ERROR_RMODE_APP	((ULONG)0x00000481L)

/* MessageId  : 0x00000482 */
/* Approx. msg: ERROR_INVALID_DLL - One of the library files needed to run this application is damaged. */
#define ERROR_INVALID_DLL	((ULONG)0x00000482L)

/* MessageId  : 0x00000483 */
/* Approx. msg: ERROR_NO_ASSOCIATION - No application is associated with the specified file for this operation. */
#define ERROR_NO_ASSOCIATION	((ULONG)0x00000483L)

/* MessageId  : 0x00000484 */
/* Approx. msg: ERROR_DDE_FAIL - An error occurred in sending the command to the application. */
#define ERROR_DDE_FAIL	((ULONG)0x00000484L)

/* MessageId  : 0x00000485 */
/* Approx. msg: ERROR_DLL_NOT_FOUND - One of the library files needed to run this application cannot be found. */
#define ERROR_DLL_NOT_FOUND	((ULONG)0x00000485L)

/* MessageId  : 0x00000486 */
/* Approx. msg: ERROR_NO_MORE_USER_HANDLES - The current process has used all of its system allowance of handles for Window Manager objects. */
#define ERROR_NO_MORE_USER_HANDLES	((ULONG)0x00000486L)

/* MessageId  : 0x00000487 */
/* Approx. msg: ERROR_MESSAGE_SYNC_ONLY - The message can be used only with synchronous operations. */
#define ERROR_MESSAGE_SYNC_ONLY	((ULONG)0x00000487L)

/* MessageId  : 0x00000488 */
/* Approx. msg: ERROR_SOURCE_ELEMENT_EMPTY - The indicated source element has no media. */
#define ERROR_SOURCE_ELEMENT_EMPTY	((ULONG)0x00000488L)

/* MessageId  : 0x00000489 */
/* Approx. msg: ERROR_DESTINATION_ELEMENT_FULL - The indicated destination element already contains media. */
#define ERROR_DESTINATION_ELEMENT_FULL	((ULONG)0x00000489L)

/* MessageId  : 0x0000048a */
/* Approx. msg: ERROR_ILLEGAL_ELEMENT_ADDRESS - The indicated element does not exist. */
#define ERROR_ILLEGAL_ELEMENT_ADDRESS	((ULONG)0x0000048aL)

/* MessageId  : 0x0000048b */
/* Approx. msg: ERROR_MAGAZINE_NOT_PRESENT - The indicated element is part of a magazine that is not present. */
#define ERROR_MAGAZINE_NOT_PRESENT	((ULONG)0x0000048bL)

/* MessageId  : 0x0000048c */
/* Approx. msg: ERROR_DEVICE_REINITIALIZATION_NEEDED - The indicated device requires reinitialization due to hardware errors. */
#define ERROR_DEVICE_REINITIALIZATION_NEEDED	((ULONG)0x0000048cL)

/* MessageId  : 0x0000048d */
/* Approx. msg: ERROR_DEVICE_REQUIRES_CLEANING - The device has indicated that cleaning is required before further operations are attempted. */
#define ERROR_DEVICE_REQUIRES_CLEANING	((ULONG)0x0000048dL)

/* MessageId  : 0x0000048e */
/* Approx. msg: ERROR_DEVICE_DOOR_OPEN - The device has indicated that its door is open. */
#define ERROR_DEVICE_DOOR_OPEN	((ULONG)0x0000048eL)

/* MessageId  : 0x0000048f */
/* Approx. msg: ERROR_DEVICE_NOT_CONNECTED - The device is not connected. */
#define ERROR_DEVICE_NOT_CONNECTED	((ULONG)0x0000048fL)

/* MessageId  : 0x00000490 */
/* Approx. msg: ERROR_NOT_FOUND - Element not found. */
#define ERROR_NOT_FOUND	((ULONG)0x00000490L)

/* MessageId  : 0x00000491 */
/* Approx. msg: ERROR_NO_MATCH - There was no match for the specified key in the index. */
#define ERROR_NO_MATCH	((ULONG)0x00000491L)

/* MessageId  : 0x00000492 */
/* Approx. msg: ERROR_SET_NOT_FOUND - The property set specified does not exist on the object. */
#define ERROR_SET_NOT_FOUND	((ULONG)0x00000492L)

/* MessageId  : 0x00000493 */
/* Approx. msg: ERROR_POINT_NOT_FOUND - The point passed to GetMouseMovePointsEx is not in the buffer. */
#define ERROR_POINT_NOT_FOUND	((ULONG)0x00000493L)

/* MessageId  : 0x00000494 */
/* Approx. msg: ERROR_NO_TRACKING_SERVICE - The tracking (workstation) service is not running. */
#define ERROR_NO_TRACKING_SERVICE	((ULONG)0x00000494L)

/* MessageId  : 0x00000495 */
/* Approx. msg: ERROR_NO_VOLUME_ID - The Volume ID could not be found. */
#define ERROR_NO_VOLUME_ID	((ULONG)0x00000495L)

/* MessageId  : 0x00000497 */
/* Approx. msg: ERROR_UNABLE_TO_REMOVE_REPLACED - Unable to remove the file to be replaced. */
#define ERROR_UNABLE_TO_REMOVE_REPLACED	((ULONG)0x00000497L)

/* MessageId  : 0x00000498 */
/* Approx. msg: ERROR_UNABLE_TO_MOVE_REPLACEMENT - Unable to move the replacement file to the file to be replaced. The file to be replaced has retained its original name. */
#define ERROR_UNABLE_TO_MOVE_REPLACEMENT	((ULONG)0x00000498L)

/* MessageId  : 0x00000499 */
/* Approx. msg: ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 - Unable to move the replacement file to the file to be replaced. The file to be replaced has been renamed using the backup name. */
#define ERROR_UNABLE_TO_MOVE_REPLACEMENT_2	((ULONG)0x00000499L)

/* MessageId  : 0x0000049a */
/* Approx. msg: ERROR_JOURNAL_DELETE_IN_PROGRESS - The volume change journal is being deleted. */
#define ERROR_JOURNAL_DELETE_IN_PROGRESS	((ULONG)0x0000049aL)

/* MessageId  : 0x0000049b */
/* Approx. msg: ERROR_JOURNAL_NOT_ACTIVE - The volume change journal is not active. */
#define ERROR_JOURNAL_NOT_ACTIVE	((ULONG)0x0000049bL)

/* MessageId  : 0x0000049c */
/* Approx. msg: ERROR_POTENTIAL_FILE_FOUND - A file was found, but it may not be the correct file. */
#define ERROR_POTENTIAL_FILE_FOUND	((ULONG)0x0000049cL)

/* MessageId  : 0x0000049d */
/* Approx. msg: ERROR_JOURNAL_ENTRY_DELETED - The journal entry has been deleted from the journal. */
#define ERROR_JOURNAL_ENTRY_DELETED	((ULONG)0x0000049dL)

/* MessageId  : 0x000004b0 */
/* Approx. msg: ERROR_BAD_DEVICE - The specified device name is invalid. */
#define ERROR_BAD_DEVICE	((ULONG)0x000004b0L)

/* MessageId  : 0x000004b1 */
/* Approx. msg: ERROR_CONNECTION_UNAVAIL - The device is not currently connected but it is a remembered connection. */
#define ERROR_CONNECTION_UNAVAIL	((ULONG)0x000004b1L)

/* MessageId  : 0x000004b2 */
/* Approx. msg: ERROR_DEVICE_ALREADY_REMEMBERED - The local device name has a remembered connection to another network resource. */
#define ERROR_DEVICE_ALREADY_REMEMBERED	((ULONG)0x000004b2L)

/* MessageId  : 0x000004b3 */
/* Approx. msg: ERROR_NO_NET_OR_BAD_PATH - The network path was either typed incorrectly, does not exist, or the network provider is not currently available. Please try retyping the path or contact your network administrator. */
#define ERROR_NO_NET_OR_BAD_PATH	((ULONG)0x000004b3L)

/* MessageId  : 0x000004b4 */
/* Approx. msg: ERROR_BAD_PROVIDER - The specified network provider name is invalid. */
#define ERROR_BAD_PROVIDER	((ULONG)0x000004b4L)

/* MessageId  : 0x000004b5 */
/* Approx. msg: ERROR_CANNOT_OPEN_PROFILE - Unable to open the network connection profile. */
#define ERROR_CANNOT_OPEN_PROFILE	((ULONG)0x000004b5L)

/* MessageId  : 0x000004b6 */
/* Approx. msg: ERROR_BAD_PROFILE - The network connection profile is corrupted. */
#define ERROR_BAD_PROFILE	((ULONG)0x000004b6L)

/* MessageId  : 0x000004b7 */
/* Approx. msg: ERROR_NOT_CONTAINER - Cannot enumerate a noncontainer. */
#define ERROR_NOT_CONTAINER	((ULONG)0x000004b7L)

/* MessageId  : 0x000004b8 */
/* Approx. msg: ERROR_EXTENDED_ERROR - An extended error has occurred. */
#define ERROR_EXTENDED_ERROR	((ULONG)0x000004b8L)

/* MessageId  : 0x000004b9 */
/* Approx. msg: ERROR_INVALID_GROUPNAME - The format of the specified group name is invalid. */
#define ERROR_INVALID_GROUPNAME	((ULONG)0x000004b9L)

/* MessageId  : 0x000004ba */
/* Approx. msg: ERROR_INVALID_COMPUTERNAME - The format of the specified computer name is invalid. */
#define ERROR_INVALID_COMPUTERNAME	((ULONG)0x000004baL)

/* MessageId  : 0x000004bb */
/* Approx. msg: ERROR_INVALID_EVENTNAME - The format of the specified event name is invalid. */
#define ERROR_INVALID_EVENTNAME	((ULONG)0x000004bbL)

/* MessageId  : 0x000004bc */
/* Approx. msg: ERROR_INVALID_DOMAINNAME - The format of the specified domain name is invalid. */
#define ERROR_INVALID_DOMAINNAME	((ULONG)0x000004bcL)

/* MessageId  : 0x000004bd */
/* Approx. msg: ERROR_INVALID_SERVICENAME - The format of the specified service name is invalid. */
#define ERROR_INVALID_SERVICENAME	((ULONG)0x000004bdL)

/* MessageId  : 0x000004be */
/* Approx. msg: ERROR_INVALID_NETNAME - The format of the specified network name is invalid. */
#define ERROR_INVALID_NETNAME	((ULONG)0x000004beL)

/* MessageId  : 0x000004bf */
/* Approx. msg: ERROR_INVALID_SHARENAME - The format of the specified share name is invalid. */
#define ERROR_INVALID_SHARENAME	((ULONG)0x000004bfL)

/* MessageId  : 0x000004c0 */
/* Approx. msg: ERROR_INVALID_PASSWORDNAME - The format of the specified password is invalid. */
#define ERROR_INVALID_PASSWORDNAME	((ULONG)0x000004c0L)

/* MessageId  : 0x000004c1 */
/* Approx. msg: ERROR_INVALID_MESSAGENAME - The format of the specified message name is invalid. */
#define ERROR_INVALID_MESSAGENAME	((ULONG)0x000004c1L)

/* MessageId  : 0x000004c2 */
/* Approx. msg: ERROR_INVALID_MESSAGEDEST - The format of the specified message destination is invalid. */
#define ERROR_INVALID_MESSAGEDEST	((ULONG)0x000004c2L)

/* MessageId  : 0x000004c3 */
/* Approx. msg: ERROR_SESSION_CREDENTIAL_CONFLICT - Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again. */
#define ERROR_SESSION_CREDENTIAL_CONFLICT	((ULONG)0x000004c3L)

/* MessageId  : 0x000004c4 */
/* Approx. msg: ERROR_REMOTE_SESSION_LIMIT_EXCEEDED - An attempt was made to establish a session to a network server, but there are already too many sessions established to that server. */
#define ERROR_REMOTE_SESSION_LIMIT_EXCEEDED	((ULONG)0x000004c4L)

/* MessageId  : 0x000004c5 */
/* Approx. msg: ERROR_DUP_DOMAINNAME - The workgroup or domain name is already in use by another computer on the network. */
#define ERROR_DUP_DOMAINNAME	((ULONG)0x000004c5L)

/* MessageId  : 0x000004c6 */
/* Approx. msg: ERROR_NO_NETWORK - The network is not present or not started. */
#define ERROR_NO_NETWORK	((ULONG)0x000004c6L)

/* MessageId  : 0x000004c7 */
/* Approx. msg: ERROR_CANCELLED - The operation was canceled by the user. */
#define ERROR_CANCELLED	((ULONG)0x000004c7L)

/* MessageId  : 0x000004c8 */
/* Approx. msg: ERROR_USER_MAPPED_FILE - The requested operation cannot be performed on a file with a user-mapped section open. */
#define ERROR_USER_MAPPED_FILE	((ULONG)0x000004c8L)

/* MessageId  : 0x000004c9 */
/* Approx. msg: ERROR_CONNECTION_REFUSED - The remote system refused the network connection. */
#define ERROR_CONNECTION_REFUSED	((ULONG)0x000004c9L)

/* MessageId  : 0x000004ca */
/* Approx. msg: ERROR_GRACEFUL_DISCONNECT - The network connection was gracefully closed. */
#define ERROR_GRACEFUL_DISCONNECT	((ULONG)0x000004caL)

/* MessageId  : 0x000004cb */
/* Approx. msg: ERROR_ADDRESS_ALREADY_ASSOCIATED - The network transport endpoint already has an address associated with it. */
#define ERROR_ADDRESS_ALREADY_ASSOCIATED	((ULONG)0x000004cbL)

/* MessageId  : 0x000004cc */
/* Approx. msg: ERROR_ADDRESS_NOT_ASSOCIATED - An address has not yet been associated with the network endpoint. */
#define ERROR_ADDRESS_NOT_ASSOCIATED	((ULONG)0x000004ccL)

/* MessageId  : 0x000004cd */
/* Approx. msg: ERROR_CONNECTION_INVALID - An operation was attempted on a nonexistent network connection. */
#define ERROR_CONNECTION_INVALID	((ULONG)0x000004cdL)

/* MessageId  : 0x000004ce */
/* Approx. msg: ERROR_CONNECTION_ACTIVE - An invalid operation was attempted on an active network connection. */
#define ERROR_CONNECTION_ACTIVE	((ULONG)0x000004ceL)

/* MessageId  : 0x000004cf */
/* Approx. msg: ERROR_NETWORK_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help. */
#define ERROR_NETWORK_UNREACHABLE	((ULONG)0x000004cfL)

/* MessageId  : 0x000004d0 */
/* Approx. msg: ERROR_HOST_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help. */
#define ERROR_HOST_UNREACHABLE	((ULONG)0x000004d0L)

/* MessageId  : 0x000004d1 */
/* Approx. msg: ERROR_PROTOCOL_UNREACHABLE - The network location cannot be reached. For information about network troubleshooting, see Windows Help. */
#define ERROR_PROTOCOL_UNREACHABLE	((ULONG)0x000004d1L)

/* MessageId  : 0x000004d2 */
/* Approx. msg: ERROR_PORT_UNREACHABLE - No service is operating at the destination network endpoint on the remote system. */
#define ERROR_PORT_UNREACHABLE	((ULONG)0x000004d2L)

/* MessageId  : 0x000004d3 */
/* Approx. msg: ERROR_REQUEST_ABORTED - The request was aborted. */
#define ERROR_REQUEST_ABORTED	((ULONG)0x000004d3L)

/* MessageId  : 0x000004d4 */
/* Approx. msg: ERROR_CONNECTION_ABORTED - The network connection was aborted by the local system. */
#define ERROR_CONNECTION_ABORTED	((ULONG)0x000004d4L)

/* MessageId  : 0x000004d5 */
/* Approx. msg: ERROR_RETRY - The operation could not be completed. A retry should be performed. */
#define ERROR_RETRY	((ULONG)0x000004d5L)

/* MessageId  : 0x000004d6 */
/* Approx. msg: ERROR_CONNECTION_COUNT_LIMIT - A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached. */
#define ERROR_CONNECTION_COUNT_LIMIT	((ULONG)0x000004d6L)

/* MessageId  : 0x000004d7 */
/* Approx. msg: ERROR_LOGIN_TIME_RESTRICTION - Attempting to log in during an unauthorized time of day for this account. */
#define ERROR_LOGIN_TIME_RESTRICTION	((ULONG)0x000004d7L)

/* MessageId  : 0x000004d8 */
/* Approx. msg: ERROR_LOGIN_WKSTA_RESTRICTION - The account is not authorized to log in from this station. */
#define ERROR_LOGIN_WKSTA_RESTRICTION	((ULONG)0x000004d8L)

/* MessageId  : 0x000004d9 */
/* Approx. msg: ERROR_INCORRECT_ADDRESS - The network address could not be used for the operation requested. */
#define ERROR_INCORRECT_ADDRESS	((ULONG)0x000004d9L)

/* MessageId  : 0x000004da */
/* Approx. msg: ERROR_ALREADY_REGISTERED - The service is already registered. */
#define ERROR_ALREADY_REGISTERED	((ULONG)0x000004daL)

/* MessageId  : 0x000004db */
/* Approx. msg: ERROR_SERVICE_NOT_FOUND - The specified service does not exist. */
#define ERROR_SERVICE_NOT_FOUND	((ULONG)0x000004dbL)

/* MessageId  : 0x000004dc */
/* Approx. msg: ERROR_NOT_AUTHENTICATED - The operation being requested was not performed because the user has not been authenticated. */
#define ERROR_NOT_AUTHENTICATED	((ULONG)0x000004dcL)

/* MessageId  : 0x000004dd */
/* Approx. msg: ERROR_NOT_LOGGED_ON - The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist. */
#define ERROR_NOT_LOGGED_ON	((ULONG)0x000004ddL)

/* MessageId  : 0x000004de */
/* Approx. msg: ERROR_CONTINUE - Continue with work in progress. */
#define ERROR_CONTINUE	((ULONG)0x000004deL)

/* MessageId  : 0x000004df */
/* Approx. msg: ERROR_ALREADY_INITIALIZED - An attempt was made to perform an initialization operation when initialization has already been completed. */
#define ERROR_ALREADY_INITIALIZED	((ULONG)0x000004dfL)

/* MessageId  : 0x000004e0 */
/* Approx. msg: ERROR_NO_MORE_DEVICES - No more local devices. */
#define ERROR_NO_MORE_DEVICES	((ULONG)0x000004e0L)

/* MessageId  : 0x000004e1 */
/* Approx. msg: ERROR_NO_SUCH_SITE - The specified site does not exist. */
#define ERROR_NO_SUCH_SITE	((ULONG)0x000004e1L)

/* MessageId  : 0x000004e2 */
/* Approx. msg: ERROR_DOMAIN_CONTROLLER_EXISTS - A domain controller with the specified name already exists. */
#define ERROR_DOMAIN_CONTROLLER_EXISTS	((ULONG)0x000004e2L)

/* MessageId  : 0x000004e3 */
/* Approx. msg: ERROR_ONLY_IF_CONNECTED - This operation is supported only when you are connected to the server. */
#define ERROR_ONLY_IF_CONNECTED	((ULONG)0x000004e3L)

/* MessageId  : 0x000004e4 */
/* Approx. msg: ERROR_OVERRIDE_NOCHANGES - The group policy framework should call the extension even if there are no changes. */
#define ERROR_OVERRIDE_NOCHANGES	((ULONG)0x000004e4L)

/* MessageId  : 0x000004e5 */
/* Approx. msg: ERROR_BAD_USER_PROFILE - The specified user does not have a valid profile. */
#define ERROR_BAD_USER_PROFILE	((ULONG)0x000004e5L)

/* MessageId  : 0x000004e6 */
/* Approx. msg: ERROR_NOT_SUPPORTED_ON_SBS - This operation is not supported on a computer running Windows Server 2003 for Small Business Server. */
#define ERROR_NOT_SUPPORTED_ON_SBS	((ULONG)0x000004e6L)

/* MessageId  : 0x000004e7 */
/* Approx. msg: ERROR_SERVER_SHUTDOWN_IN_PROGRESS - The server machine is shutting down. */
#define ERROR_SERVER_SHUTDOWN_IN_PROGRESS	((ULONG)0x000004e7L)

/* MessageId  : 0x000004e8 */
/* Approx. msg: ERROR_HOST_DOWN - The remote system is not available. For information about network troubleshooting, see Windows Help. */
#define ERROR_HOST_DOWN	((ULONG)0x000004e8L)

/* MessageId  : 0x000004e9 */
/* Approx. msg: ERROR_NON_ACCOUNT_SID - The security identifier provided is not from an account domain. */
#define ERROR_NON_ACCOUNT_SID	((ULONG)0x000004e9L)

/* MessageId  : 0x000004ea */
/* Approx. msg: ERROR_NON_DOMAIN_SID - The security identifier provided does not have a domain component. */
#define ERROR_NON_DOMAIN_SID	((ULONG)0x000004eaL)

/* MessageId  : 0x000004eb */
/* Approx. msg: ERROR_APPHELP_BLOCK - AppHelp dialog canceled thus preventing the application from starting. */
#define ERROR_APPHELP_BLOCK	((ULONG)0x000004ebL)

/* MessageId  : 0x000004ec */
/* Approx. msg: ERROR_ACCESS_DISABLED_BY_POLICY - Windows cannot open this program because it has been prevented by a software restriction policy. For more information, open Event Viewer or contact your system administrator. */
#define ERROR_ACCESS_DISABLED_BY_POLICY	((ULONG)0x000004ecL)

/* MessageId  : 0x000004ed */
/* Approx. msg: ERROR_REG_NAT_CONSUMPTION - A program attempt to use an invalid register value. Normally caused by an uninitialized register. This error is Itanium specific. */
#define ERROR_REG_NAT_CONSUMPTION	((ULONG)0x000004edL)

/* MessageId  : 0x000004ee */
/* Approx. msg: ERROR_CSCSHARE_OFFLINE - The share is currently offline or does not exist. */
#define ERROR_CSCSHARE_OFFLINE	((ULONG)0x000004eeL)

/* MessageId  : 0x000004ef */
/* Approx. msg: ERROR_PKINIT_FAILURE - The kerberos protocol encountered an error while validating the KDC certificate during smartcard logon. */
#define ERROR_PKINIT_FAILURE	((ULONG)0x000004efL)

/* MessageId  : 0x000004f0 */
/* Approx. msg: ERROR_SMARTCARD_SUBSYSTEM_FAILURE - The kerberos protocol encountered an error while attempting to utilize the smartcard subsystem. */
#define ERROR_SMARTCARD_SUBSYSTEM_FAILURE	((ULONG)0x000004f0L)

/* MessageId  : 0x000004f1 */
/* Approx. msg: ERROR_DOWNGRADE_DETECTED - The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you. */
#define ERROR_DOWNGRADE_DETECTED	((ULONG)0x000004f1L)

/* MessageId  : 0x000004f2 */
/* Approx. msg: SEC_E_SMARTCARD_CERT_REVOKED - The smartcard certificate used for authentication has been revoked. Please contact your system administrator. There may be additional information in the event log. */
#define SEC_E_SMARTCARD_CERT_REVOKED	((ULONG)0x000004f2L)

/* MessageId  : 0x000004f3 */
/* Approx. msg: SEC_E_ISSUING_CA_UNTRUSTED - An untrusted certificate authority was detected while processing the smartcard certificate used for authentication. Please contact your system administrator. */
#define SEC_E_ISSUING_CA_UNTRUSTED	((ULONG)0x000004f3L)

/* MessageId  : 0x000004f4 */
/* Approx. msg: SEC_E_REVOCATION_OFFLINE_C - The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator. */
#define SEC_E_REVOCATION_OFFLINE_C	((ULONG)0x000004f4L)

/* MessageId  : 0x000004f5 */
/* Approx. msg: SEC_E_PKINIT_CLIENT_FAILUR - The smartcard certificate used for authentication was not trusted. Please contact your system administrator. */
#define SEC_E_PKINIT_CLIENT_FAILUR	((ULONG)0x000004f5L)

/* MessageId  : 0x000004f6 */
/* Approx. msg: SEC_E_SMARTCARD_CERT_EXPIRED - The smartcard certificate used for authentication has expired. Please contact your system administrator. */
#define SEC_E_SMARTCARD_CERT_EXPIRED	((ULONG)0x000004f6L)

/* MessageId  : 0x000004f7 */
/* Approx. msg: ERROR_MACHINE_LOCKED - The machine is locked and cannot be shut down without the force option. */
#define ERROR_MACHINE_LOCKED	((ULONG)0x000004f7L)

/* MessageId  : 0x000004f9 */
/* Approx. msg: ERROR_CALLBACK_SUPPLIED_INVALID_DATA - An application-defined callback gave invalid data when called. */
#define ERROR_CALLBACK_SUPPLIED_INVALID_DATA	((ULONG)0x000004f9L)

/* MessageId  : 0x000004fa */
/* Approx. msg: ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED - The group policy framework should call the extension in the synchronous foreground policy refresh. */
#define ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED	((ULONG)0x000004faL)

/* MessageId  : 0x000004fb */
/* Approx. msg: ERROR_DRIVER_BLOCKED - This driver has been blocked from loading. */
#define ERROR_DRIVER_BLOCKED	((ULONG)0x000004fbL)

/* MessageId  : 0x000004fc */
/* Approx. msg: ERROR_INVALID_IMPORT_OF_NON_DLL - A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image. */
#define ERROR_INVALID_IMPORT_OF_NON_DLL	((ULONG)0x000004fcL)

/* MessageId  : 0x000004fd */
/* Approx. msg: ERROR_ACCESS_DISABLED_WEBBLADE - Windows cannot open this program since it has been disabled. */
#define ERROR_ACCESS_DISABLED_WEBBLADE	((ULONG)0x000004fdL)

/* MessageId  : 0x000004fe */
/* Approx. msg: ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER - Windows cannot open this program because the license enforcement system has been tampered with or become corrupted. */
#define ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER	((ULONG)0x000004feL)

/* MessageId  : 0x000004ff */
/* Approx. msg: ERROR_RECOVERY_FAILURE - A transaction recovery failed. */
#define ERROR_RECOVERY_FAILURE	((ULONG)0x000004ffL)

/* MessageId  : 0x00000500 */
/* Approx. msg: ERROR_ALREADY_FIBER - The current thread has already been converted to a fiber. */
#define ERROR_ALREADY_FIBER	((ULONG)0x00000500L)

/* MessageId  : 0x00000501 */
/* Approx. msg: ERROR_ALREADY_THREAD - The current thread has already been converted from a fiber. */
#define ERROR_ALREADY_THREAD	((ULONG)0x00000501L)

/* MessageId  : 0x00000502 */
/* Approx. msg: ERROR_STACK_BUFFER_OVERRUN - The system detected an overrun of a stack-based buffer in this application. This overrun could potentially allow a malicious user to gain control of this application. */
#define ERROR_STACK_BUFFER_OVERRUN	((ULONG)0x00000502L)

/* MessageId  : 0x00000503 */
/* Approx. msg: ERROR_PARAMETER_QUOTA_EXCEEDED - Data present in one of the parameters is more than the function can operate on. */
#define ERROR_PARAMETER_QUOTA_EXCEEDED	((ULONG)0x00000503L)

/* MessageId  : 0x00000504 */
/* Approx. msg: ERROR_DEBUGGER_INACTIVE - An attempt to do an operation on a debug object failed because the object is in the process of being deleted. */
#define ERROR_DEBUGGER_INACTIVE	((ULONG)0x00000504L)

/* MessageId  : 0x00000505 */
/* Approx. msg: ERROR_DELAY_LOAD_FAILED - An attempt to delay-load a .dll or get a function address in a delay-loaded .dll failed. */
#define ERROR_DELAY_LOAD_FAILED	((ULONG)0x00000505L)

/* MessageId  : 0x00000506 */
/* Approx. msg: ERROR_VDM_DISALLOWED - %1 is a 16-bit application. You do not have permissions to execute 16-bit applications. Check your permissions with your system administrator. */
#define ERROR_VDM_DISALLOWED	((ULONG)0x00000506L)

/* MessageId  : 0x00000507 */
/* Approx. msg: ERROR_UNIDENTIFIED_ERROR - Insufficient information exists to identify the cause of failure. */
#define ERROR_UNIDENTIFIED_ERROR	((ULONG)0x00000507L)

/* MessageId  : 0x00000508 */
/* Approx. msg: ERROR_INVALID_BANDWIDTH_PARAMETERS - An invalid budget or period parameter was specified. */
#define ERROR_INVALID_BANDWIDTH_PARAMETERS	((ULONG)0x00000508L)

/* MessageId  : 0x00000509 */
/* Approx. msg: ERROR_AFFINITY_NOT_COMPATIBLE - An attempt was made to join a thread to a reserve whose affinity did not intersect the reserve affinity or an attempt was made to associate a process with a reserve whose affinity did not intersect the reserve affinity. */
#define ERROR_AFFINITY_NOT_COMPATIBLE	((ULONG)0x00000509L)

/* MessageId  : 0x0000050a */
/* Approx. msg: ERROR_THREAD_ALREADY_IN_RESERVE - An attempt was made to join a thread to a reserve which was already joined to another reserve. */
#define ERROR_THREAD_ALREADY_IN_RESERVE	((ULONG)0x0000050aL)

/* MessageId  : 0x0000050b */
/* Approx. msg: ERROR_THREAD_NOT_IN_RESERVE - An attempt was made to disjoin a thread from a reserve, but the thread was not joined to the reserve. */
#define ERROR_THREAD_NOT_IN_RESERVE	((ULONG)0x0000050bL)

/* MessageId  : 0x0000050c */
/* Approx. msg: ERROR_THREAD_PROCESS_IN_RESERVE - An attempt was made to disjoin a thread from a reserve whose process is associated with a reserve. */
#define ERROR_THREAD_PROCESS_IN_RESERVE	((ULONG)0x0000050cL)

/* MessageId  : 0x0000050d */
/* Approx. msg: ERROR_PROCESS_ALREADY_IN_RESERVE - An attempt was made to associate a process with a reserve that was already associated with a reserve. */
#define ERROR_PROCESS_ALREADY_IN_RESERVE	((ULONG)0x0000050dL)

/* MessageId  : 0x0000050e */
/* Approx. msg: ERROR_PROCESS_NOT_IN_RESERVE - An attempt was made to disassociate a process from a reserve, but the process did not have an associated reserve. */
#define ERROR_PROCESS_NOT_IN_RESERVE	((ULONG)0x0000050eL)

/* MessageId  : 0x0000050f */
/* Approx. msg: ERROR_PROCESS_THREADS_IN_RESERVE - An attempt was made to associate a process with a reserve, but the process contained thread joined to a reserve. */
#define ERROR_PROCESS_THREADS_IN_RESERVE	((ULONG)0x0000050fL)

/* MessageId  : 0x00000510 */
/* Approx. msg: ERROR_AFFINITY_NOT_SET_IN_RESERVE - An attempt was made to set the affinity of a thread or a process, but the thread or process was joined or associated with a reserve. */
#define ERROR_AFFINITY_NOT_SET_IN_RESERVE	((ULONG)0x00000510L)

/* MessageId  : 0x00000511 */
/* Approx. msg: ERROR_IMPLEMENTATION_LIMIT - An operation attempted to exceed an implementation-defined limit. */
#define ERROR_IMPLEMENTATION_LIMIT	((ULONG)0x00000511L)

/* MessageId  : 0x00000512 */
/* Approx. msg: ERROR_DS_CACHE_ONLY - The requested object is for internal DS operations only. */
#define ERROR_DS_CACHE_ONLY	((ULONG)0x00000512L)

/* MessageId  : 0x00000514 */
/* Approx. msg: ERROR_NOT_ALL_ASSIGNED - Not all privileges referenced are assigned to the caller. */
#define ERROR_NOT_ALL_ASSIGNED	((ULONG)0x00000514L)

/* MessageId  : 0x00000515 */
/* Approx. msg: ERROR_SOME_NOT_MAPPED - Some mapping between account names and security IDs was not done. */
#define ERROR_SOME_NOT_MAPPED	((ULONG)0x00000515L)

/* MessageId  : 0x00000516 */
/* Approx. msg: ERROR_NO_QUOTAS_FOR_ACCOUNT - No system quota limits are specifically set for this account. */
#define ERROR_NO_QUOTAS_FOR_ACCOUNT	((ULONG)0x00000516L)

/* MessageId  : 0x00000517 */
/* Approx. msg: ERROR_LOCAL_USER_SESSION_KEY - No encryption key is available. A well-known encryption key was returned. */
#define ERROR_LOCAL_USER_SESSION_KEY	((ULONG)0x00000517L)

/* MessageId  : 0x00000518 */
/* Approx. msg: ERROR_NULL_LM_PASSWORD - The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string. */
#define ERROR_NULL_LM_PASSWORD	((ULONG)0x00000518L)

/* MessageId  : 0x00000519 */
/* Approx. msg: ERROR_UNKNOWN_REVISION - The revision level is unknown. */
#define ERROR_UNKNOWN_REVISION	((ULONG)0x00000519L)

/* MessageId  : 0x0000051a */
/* Approx. msg: ERROR_REVISION_MISMATCH - Indicates two revision levels are incompatible. */
#define ERROR_REVISION_MISMATCH	((ULONG)0x0000051aL)

/* MessageId  : 0x0000051b */
/* Approx. msg: ERROR_INVALID_OWNER - This security ID may not be assigned as the owner of this object. */
#define ERROR_INVALID_OWNER	((ULONG)0x0000051bL)

/* MessageId  : 0x0000051c */
/* Approx. msg: ERROR_INVALID_PRIMARY_GROUP - This security ID may not be assigned as the primary group of an object. */
#define ERROR_INVALID_PRIMARY_GROUP	((ULONG)0x0000051cL)

/* MessageId  : 0x0000051d */
/* Approx. msg: ERROR_NO_IMPERSONATION_TOKEN - An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client. */
#define ERROR_NO_IMPERSONATION_TOKEN	((ULONG)0x0000051dL)

/* MessageId  : 0x0000051e */
/* Approx. msg: ERROR_CANT_DISABLE_MANDATORY - The group may not be disabled. */
#define ERROR_CANT_DISABLE_MANDATORY	((ULONG)0x0000051eL)

/* MessageId  : 0x0000051f */
/* Approx. msg: ERROR_NO_LOGON_SERVERS - There are currently no logon servers available to service the logon request. */
#define ERROR_NO_LOGON_SERVERS	((ULONG)0x0000051fL)

/* MessageId  : 0x00000520 */
/* Approx. msg: ERROR_NO_SUCH_LOGON_SESSION - A specified logon session does not exist. It may already have been terminated. */
#define ERROR_NO_SUCH_LOGON_SESSION	((ULONG)0x00000520L)

/* MessageId  : 0x00000521 */
/* Approx. msg: ERROR_NO_SUCH_PRIVILEGE - A specified privilege does not exist. */
#define ERROR_NO_SUCH_PRIVILEGE	((ULONG)0x00000521L)

/* MessageId  : 0x00000522 */
/* Approx. msg: ERROR_PRIVILEGE_NOT_HELD - A required privilege is not held by the client. */
#define ERROR_PRIVILEGE_NOT_HELD	((ULONG)0x00000522L)

/* MessageId  : 0x00000523 */
/* Approx. msg: ERROR_INVALID_ACCOUNT_NAME - The name provided is not a properly formed account name. */
#define ERROR_INVALID_ACCOUNT_NAME	((ULONG)0x00000523L)

/* MessageId  : 0x00000524 */
/* Approx. msg: ERROR_USER_EXISTS - The specified user already exists. */
#define ERROR_USER_EXISTS	((ULONG)0x00000524L)

/* MessageId  : 0x00000525 */
/* Approx. msg: ERROR_NO_SUCH_USER - The specified user does not exist. */
#define ERROR_NO_SUCH_USER	((ULONG)0x00000525L)

/* MessageId  : 0x00000526 */
/* Approx. msg: ERROR_GROUP_EXISTS - The specified group already exists. */
#define ERROR_GROUP_EXISTS	((ULONG)0x00000526L)

/* MessageId  : 0x00000527 */
/* Approx. msg: ERROR_NO_SUCH_GROUP - The specified group does not exist. */
#define ERROR_NO_SUCH_GROUP	((ULONG)0x00000527L)

/* MessageId  : 0x00000528 */
/* Approx. msg: ERROR_MEMBER_IN_GROUP - Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member. */
#define ERROR_MEMBER_IN_GROUP	((ULONG)0x00000528L)

/* MessageId  : 0x00000529 */
/* Approx. msg: ERROR_MEMBER_NOT_IN_GROUP - The specified user account is not a member of the specified group account. */
#define ERROR_MEMBER_NOT_IN_GROUP	((ULONG)0x00000529L)

/* MessageId  : 0x0000052a */
/* Approx. msg: ERROR_LAST_ADMIN - The last remaining administration account cannot be disabled or deleted. */
#define ERROR_LAST_ADMIN	((ULONG)0x0000052aL)

/* MessageId  : 0x0000052b */
/* Approx. msg: ERROR_WRONG_PASSWORD - Unable to update the password. The value provided as the current password is incorrect. */
#define ERROR_WRONG_PASSWORD	((ULONG)0x0000052bL)

/* MessageId  : 0x0000052c */
/* Approx. msg: ERROR_ILL_FORMED_PASSWORD - Unable to update the password. The value provided for the new password contains values that are not allowed in passwords. */
#define ERROR_ILL_FORMED_PASSWORD	((ULONG)0x0000052cL)

/* MessageId  : 0x0000052d */
/* Approx. msg: ERROR_PASSWORD_RESTRICTION - Unable to update the password. The value provided for the new password does not meet the length, complexity, or history requirement of the domain. */
#define ERROR_PASSWORD_RESTRICTION	((ULONG)0x0000052dL)

/* MessageId  : 0x0000052e */
/* Approx. msg: ERROR_LOGON_FAILURE - Logon failure: unknown user name or bad password. */
#define ERROR_LOGON_FAILURE	((ULONG)0x0000052eL)

/* MessageId  : 0x0000052f */
/* Approx. msg: ERROR_ACCOUNT_RESTRICTION - Logon failure: user account restriction. Possible reasons are blank passwords not allowed, logon hour restrictions, or a policy restriction has been enforced. */
#define ERROR_ACCOUNT_RESTRICTION	((ULONG)0x0000052fL)

/* MessageId  : 0x00000530 */
/* Approx. msg: ERROR_INVALID_LOGON_HOURS - Logon failure: account logon time restriction violation. */
#define ERROR_INVALID_LOGON_HOURS	((ULONG)0x00000530L)

/* MessageId  : 0x00000531 */
/* Approx. msg: ERROR_INVALID_WORKSTATION - Logon failure: user not allowed to log on to this computer. */
#define ERROR_INVALID_WORKSTATION	((ULONG)0x00000531L)

/* MessageId  : 0x00000532 */
/* Approx. msg: ERROR_PASSWORD_EXPIRED - Logon failure: the specified account password has expired. */
#define ERROR_PASSWORD_EXPIRED	((ULONG)0x00000532L)

/* MessageId  : 0x00000533 */
/* Approx. msg: ERROR_ACCOUNT_DISABLED - Logon failure: account currently disabled. */
#define ERROR_ACCOUNT_DISABLED	((ULONG)0x00000533L)

/* MessageId  : 0x00000534 */
/* Approx. msg: ERROR_NONE_MAPPED - No mapping between account names and security IDs was done. */
#define ERROR_NONE_MAPPED	((ULONG)0x00000534L)

/* MessageId  : 0x00000535 */
/* Approx. msg: ERROR_TOO_MANY_LUIDS_REQUESTED - Too many local user identifiers (LUIDs) were requested at one time. */
#define ERROR_TOO_MANY_LUIDS_REQUESTED	((ULONG)0x00000535L)

/* MessageId  : 0x00000536 */
/* Approx. msg: ERROR_LUIDS_EXHAUSTED - No more local user identifiers (LUIDs) are available. */
#define ERROR_LUIDS_EXHAUSTED	((ULONG)0x00000536L)

/* MessageId  : 0x00000537 */
/* Approx. msg: ERROR_INVALID_SUB_AUTHORITY - The subauthority part of a security ID is invalid for this particular use. */
#define ERROR_INVALID_SUB_AUTHORITY	((ULONG)0x00000537L)

/* MessageId  : 0x00000538 */
/* Approx. msg: ERROR_INVALID_ACL - The access control list (ACL) structure is invalid. */
#define ERROR_INVALID_ACL	((ULONG)0x00000538L)

/* MessageId  : 0x00000539 */
/* Approx. msg: ERROR_INVALID_SID - The security ID structure is invalid. */
#define ERROR_INVALID_SID	((ULONG)0x00000539L)

/* MessageId  : 0x0000053a */
/* Approx. msg: ERROR_INVALID_SECURITY_DESCR - The security descriptor structure is invalid. */
#define ERROR_INVALID_SECURITY_DESCR	((ULONG)0x0000053aL)

/* MessageId  : 0x0000053c */
/* Approx. msg: ERROR_BAD_INHERITANCE_ACL - The inherited access control list (ACL) or access control entry (ACE) could not be built. */
#define ERROR_BAD_INHERITANCE_ACL	((ULONG)0x0000053cL)

/* MessageId  : 0x0000053d */
/* Approx. msg: ERROR_SERVER_DISABLED - The server is currently disabled. */
#define ERROR_SERVER_DISABLED	((ULONG)0x0000053dL)

/* MessageId  : 0x0000053e */
/* Approx. msg: ERROR_SERVER_NOT_DISABLED - The server is currently enabled. */
#define ERROR_SERVER_NOT_DISABLED	((ULONG)0x0000053eL)

/* MessageId  : 0x0000053f */
/* Approx. msg: ERROR_INVALID_ID_AUTHORITY - The value provided was an invalid value for an identifier authority. */
#define ERROR_INVALID_ID_AUTHORITY	((ULONG)0x0000053fL)

/* MessageId  : 0x00000540 */
/* Approx. msg: ERROR_ALLOTTED_SPACE_EXCEEDED - No more memory is available for security information updates. */
#define ERROR_ALLOTTED_SPACE_EXCEEDED	((ULONG)0x00000540L)

/* MessageId  : 0x00000541 */
/* Approx. msg: ERROR_INVALID_GROUP_ATTRIBUTES - The specified attributes are invalid, or incompatible with the attributes for the group as a whole. */
#define ERROR_INVALID_GROUP_ATTRIBUTES	((ULONG)0x00000541L)

/* MessageId  : 0x00000542 */
/* Approx. msg: ERROR_BAD_IMPERSONATION_LEVEL - Either a required impersonation level was not provided, or the provided impersonation level is invalid. */
#define ERROR_BAD_IMPERSONATION_LEVEL	((ULONG)0x00000542L)

/* MessageId  : 0x00000543 */
/* Approx. msg: ERROR_CANT_OPEN_ANONYMOUS - Cannot open an anonymous level security token. */
#define ERROR_CANT_OPEN_ANONYMOUS	((ULONG)0x00000543L)

/* MessageId  : 0x00000544 */
/* Approx. msg: ERROR_BAD_VALIDATION_CLASS - The validation information class requested was invalid. */
#define ERROR_BAD_VALIDATION_CLASS	((ULONG)0x00000544L)

/* MessageId  : 0x00000545 */
/* Approx. msg: ERROR_BAD_TOKEN_TYPE - The type of the token is inappropriate for its attempted use. */
#define ERROR_BAD_TOKEN_TYPE	((ULONG)0x00000545L)

/* MessageId  : 0x00000546 */
/* Approx. msg: ERROR_NO_SECURITY_ON_OBJECT - Unable to perform a security operation on an object that has no associated security. */
#define ERROR_NO_SECURITY_ON_OBJECT	((ULONG)0x00000546L)

/* MessageId  : 0x00000547 */
/* Approx. msg: ERROR_CANT_ACCESS_DOMAIN_INFO - Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied. */
#define ERROR_CANT_ACCESS_DOMAIN_INFO	((ULONG)0x00000547L)

/* MessageId  : 0x00000548 */
/* Approx. msg: ERROR_INVALID_SERVER_STATE - The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation. */
#define ERROR_INVALID_SERVER_STATE	((ULONG)0x00000548L)

/* MessageId  : 0x00000549 */
/* Approx. msg: ERROR_INVALID_DOMAIN_STATE - The domain was in the wrong state to perform the security operation. */
#define ERROR_INVALID_DOMAIN_STATE	((ULONG)0x00000549L)

/* MessageId  : 0x0000054a */
/* Approx. msg: ERROR_INVALID_DOMAIN_ROLE - This operation is only allowed for the Primary Domain Controller of the domain. */
#define ERROR_INVALID_DOMAIN_ROLE	((ULONG)0x0000054aL)

/* MessageId  : 0x0000054b */
/* Approx. msg: ERROR_NO_SUCH_DOMAIN - The specified domain either does not exist or could not be contacted. */
#define ERROR_NO_SUCH_DOMAIN	((ULONG)0x0000054bL)

/* MessageId  : 0x0000054c */
/* Approx. msg: ERROR_DOMAIN_EXISTS - The specified domain already exists. */
#define ERROR_DOMAIN_EXISTS	((ULONG)0x0000054cL)

/* MessageId  : 0x0000054d */
/* Approx. msg: ERROR_DOMAIN_LIMIT_EXCEEDED - An attempt was made to exceed the limit on the number of domains per server. */
#define ERROR_DOMAIN_LIMIT_EXCEEDED	((ULONG)0x0000054dL)

/* MessageId  : 0x0000054e */
/* Approx. msg: ERROR_INTERNAL_DB_CORRUPTION - Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk. */
#define ERROR_INTERNAL_DB_CORRUPTION	((ULONG)0x0000054eL)

/* MessageId  : 0x0000054f */
/* Approx. msg: ERROR_INTERNAL_ERROR - An internal error occurred. */
#define ERROR_INTERNAL_ERROR	((ULONG)0x0000054fL)

/* MessageId  : 0x00000550 */
/* Approx. msg: ERROR_GENERIC_NOT_MAPPED - Generic access types were contained in an access mask which should already be mapped to nongeneric types. */
#define ERROR_GENERIC_NOT_MAPPED	((ULONG)0x00000550L)

/* MessageId  : 0x00000551 */
/* Approx. msg: ERROR_BAD_DESCRIPTOR_FORMAT - A security descriptor is not in the right format (absolute or self-relative). */
#define ERROR_BAD_DESCRIPTOR_FORMAT	((ULONG)0x00000551L)

/* MessageId  : 0x00000552 */
/* Approx. msg: ERROR_NOT_LOGON_PROCESS - The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process. */
#define ERROR_NOT_LOGON_PROCESS	((ULONG)0x00000552L)

/* MessageId  : 0x00000553 */
/* Approx. msg: ERROR_LOGON_SESSION_EXISTS - Cannot start a new logon session with an ID that is already in use. */
#define ERROR_LOGON_SESSION_EXISTS	((ULONG)0x00000553L)

/* MessageId  : 0x00000554 */
/* Approx. msg: ERROR_NO_SUCH_PACKAGE - A specified authentication package is unknown. */
#define ERROR_NO_SUCH_PACKAGE	((ULONG)0x00000554L)

/* MessageId  : 0x00000555 */
/* Approx. msg: ERROR_BAD_LOGON_SESSION_STATE - The logon session is not in a state that is consistent with the requested operation. */
#define ERROR_BAD_LOGON_SESSION_STATE	((ULONG)0x00000555L)

/* MessageId  : 0x00000556 */
/* Approx. msg: ERROR_LOGON_SESSION_COLLISION - The logon session ID is already in use. */
#define ERROR_LOGON_SESSION_COLLISION	((ULONG)0x00000556L)

/* MessageId  : 0x00000557 */
/* Approx. msg: ERROR_INVALID_LOGON_TYPE - A logon request contained an invalid logon type value. */
#define ERROR_INVALID_LOGON_TYPE	((ULONG)0x00000557L)

/* MessageId  : 0x00000558 */
/* Approx. msg: ERROR_CANNOT_IMPERSONATE - Unable to impersonate using a named pipe until data has been read from that pipe. */
#define ERROR_CANNOT_IMPERSONATE	((ULONG)0x00000558L)

/* MessageId  : 0x00000559 */
/* Approx. msg: ERROR_RXACT_INVALID_STATE - The transaction state of a registry subtree is incompatible with the requested operation. */
#define ERROR_RXACT_INVALID_STATE	((ULONG)0x00000559L)

/* MessageId  : 0x0000055a */
/* Approx. msg: ERROR_RXACT_COMMIT_FAILURE - An internal security database corruption has been encountered. */
#define ERROR_RXACT_COMMIT_FAILURE	((ULONG)0x0000055aL)

/* MessageId  : 0x0000055b */
/* Approx. msg: ERROR_SPECIAL_ACCOUNT - Cannot perform this operation on built-in accounts. */
#define ERROR_SPECIAL_ACCOUNT	((ULONG)0x0000055bL)

/* MessageId  : 0x0000055c */
/* Approx. msg: ERROR_SPECIAL_GROUP - Cannot perform this operation on this built-in special group. */
#define ERROR_SPECIAL_GROUP	((ULONG)0x0000055cL)

/* MessageId  : 0x0000055d */
/* Approx. msg: ERROR_SPECIAL_USER - Cannot perform this operation on this built-in special user. */
#define ERROR_SPECIAL_USER	((ULONG)0x0000055dL)

/* MessageId  : 0x0000055e */
/* Approx. msg: ERROR_MEMBERS_PRIMARY_GROUP - The user cannot be removed from a group because the group is currently the user's primary group. */
#define ERROR_MEMBERS_PRIMARY_GROUP	((ULONG)0x0000055eL)

/* MessageId  : 0x0000055f */
/* Approx. msg: ERROR_TOKEN_ALREADY_IN_USE - The token is already in use as a primary token. */
#define ERROR_TOKEN_ALREADY_IN_USE	((ULONG)0x0000055fL)

/* MessageId  : 0x00000560 */
/* Approx. msg: ERROR_NO_SUCH_ALIAS - The specified local group does not exist. */
#define ERROR_NO_SUCH_ALIAS	((ULONG)0x00000560L)

/* MessageId  : 0x00000561 */
/* Approx. msg: ERROR_MEMBER_NOT_IN_ALIAS - The specified account name is not a member of the local group. */
#define ERROR_MEMBER_NOT_IN_ALIAS	((ULONG)0x00000561L)

/* MessageId  : 0x00000562 */
/* Approx. msg: ERROR_MEMBER_IN_ALIAS - The specified account name is already a member of the local group. */
#define ERROR_MEMBER_IN_ALIAS	((ULONG)0x00000562L)

/* MessageId  : 0x00000563 */
/* Approx. msg: ERROR_ALIAS_EXISTS - The specified local group already exists. */
#define ERROR_ALIAS_EXISTS	((ULONG)0x00000563L)

/* MessageId  : 0x00000564 */
/* Approx. msg: ERROR_LOGON_NOT_GRANTED - Logon failure: the user has not been granted the requested logon type at this computer. */
#define ERROR_LOGON_NOT_GRANTED	((ULONG)0x00000564L)

/* MessageId  : 0x00000565 */
/* Approx. msg: ERROR_TOO_MANY_SECRETS - The maximum number of secrets that may be stored in a single system has been exceeded. */
#define ERROR_TOO_MANY_SECRETS	((ULONG)0x00000565L)

/* MessageId  : 0x00000566 */
/* Approx. msg: ERROR_SECRET_TOO_LONG - The length of a secret exceeds the maximum length allowed. */
#define ERROR_SECRET_TOO_LONG	((ULONG)0x00000566L)

/* MessageId  : 0x00000567 */
/* Approx. msg: ERROR_INTERNAL_DB_ERROR - The local security authority database contains an internal inconsistency. */
#define ERROR_INTERNAL_DB_ERROR	((ULONG)0x00000567L)

/* MessageId  : 0x00000568 */
/* Approx. msg: ERROR_TOO_MANY_CONTEXT_IDS - During a logon attempt, the user's security context accumulated too many security IDs. */
#define ERROR_TOO_MANY_CONTEXT_IDS	((ULONG)0x00000568L)

/* MessageId  : 0x00000569 */
/* Approx. msg: ERROR_LOGON_TYPE_NOT_GRANTED - Logon failure: the user has not been granted the requested logon type at this computer. */
#define ERROR_LOGON_TYPE_NOT_GRANTED	((ULONG)0x00000569L)

/* MessageId  : 0x0000056a */
/* Approx. msg: ERROR_NT_CROSS_ENCRYPTION_REQUIRED - A cross-encrypted password is necessary to change a user password. */
#define ERROR_NT_CROSS_ENCRYPTION_REQUIRED	((ULONG)0x0000056aL)

/* MessageId  : 0x0000056b */
/* Approx. msg: ERROR_NO_SUCH_MEMBER - A new member could not be added to or removed from the local group because the member does not exist. */
#define ERROR_NO_SUCH_MEMBER	((ULONG)0x0000056bL)

/* MessageId  : 0x0000056c */
/* Approx. msg: ERROR_INVALID_MEMBER - A new member could not be added to a local group because the member has the wrong account type. */
#define ERROR_INVALID_MEMBER	((ULONG)0x0000056cL)

/* MessageId  : 0x0000056d */
/* Approx. msg: ERROR_TOO_MANY_SIDS - Too many security IDs have been specified. */
#define ERROR_TOO_MANY_SIDS	((ULONG)0x0000056dL)

/* MessageId  : 0x0000056e */
/* Approx. msg: ERROR_LM_CROSS_ENCRYPTION_REQUIRED - A cross-encrypted password is necessary to change this user password. */
#define ERROR_LM_CROSS_ENCRYPTION_REQUIRED	((ULONG)0x0000056eL)

/* MessageId  : 0x0000056f */
/* Approx. msg: ERROR_NO_INHERITANCE - Indicates an ACL contains no inheritable components. */
#define ERROR_NO_INHERITANCE	((ULONG)0x0000056fL)

/* MessageId  : 0x00000570 */
/* Approx. msg: ERROR_FILE_CORRUPT - The file or directory is corrupted and unreadable. */
#define ERROR_FILE_CORRUPT	((ULONG)0x00000570L)

/* MessageId  : 0x00000571 */
/* Approx. msg: ERROR_DISK_CORRUPT - The disk structure is corrupted and unreadable. */
#define ERROR_DISK_CORRUPT	((ULONG)0x00000571L)

/* MessageId  : 0x00000572 */
/* Approx. msg: ERROR_NO_USER_SESSION_KEY - There is no user session key for the specified logon session. */
#define ERROR_NO_USER_SESSION_KEY	((ULONG)0x00000572L)

/* MessageId  : 0x00000573 */
/* Approx. msg: ERROR_LICENSE_QUOTA_EXCEEDED - The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept. */
#define ERROR_LICENSE_QUOTA_EXCEEDED	((ULONG)0x00000573L)

/* MessageId  : 0x00000574 */
/* Approx. msg: ERROR_WRONG_TARGET_NAME - Logon Failure: The target account name is incorrect. */
#define ERROR_WRONG_TARGET_NAME	((ULONG)0x00000574L)

/* MessageId  : 0x00000575 */
/* Approx. msg: ERROR_MUTUAL_AUTH_FAILED - Mutual Authentication failed. The server's password is out of date at the domain controller. */
#define ERROR_MUTUAL_AUTH_FAILED	((ULONG)0x00000575L)

/* MessageId  : 0x00000576 */
/* Approx. msg: ERROR_TIME_SKEW - There is a time and/or date difference between the client and server. */
#define ERROR_TIME_SKEW	((ULONG)0x00000576L)

/* MessageId  : 0x00000577 */
/* Approx. msg: ERROR_CURRENT_DOMAIN_NOT_ALLOWED - This operation cannot be performed on the current domain. */
#define ERROR_CURRENT_DOMAIN_NOT_ALLOWED	((ULONG)0x00000577L)

/* MessageId  : 0x00000578 */
/* Approx. msg: ERROR_INVALID_WINDOW_HANDLE - Invalid window handle. */
#define ERROR_INVALID_WINDOW_HANDLE	((ULONG)0x00000578L)

/* MessageId  : 0x00000579 */
/* Approx. msg: ERROR_INVALID_MENU_HANDLE - Invalid menu handle. */
#define ERROR_INVALID_MENU_HANDLE	((ULONG)0x00000579L)

/* MessageId  : 0x0000057a */
/* Approx. msg: ERROR_INVALID_CURSOR_HANDLE - Invalid cursor handle. */
#define ERROR_INVALID_CURSOR_HANDLE	((ULONG)0x0000057aL)

/* MessageId  : 0x0000057b */
/* Approx. msg: ERROR_INVALID_ACCEL_HANDLE - Invalid accelerator table handle. */
#define ERROR_INVALID_ACCEL_HANDLE	((ULONG)0x0000057bL)

/* MessageId  : 0x0000057c */
/* Approx. msg: ERROR_INVALID_HOOK_HANDLE - Invalid hook handle. */
#define ERROR_INVALID_HOOK_HANDLE	((ULONG)0x0000057cL)

/* MessageId  : 0x0000057d */
/* Approx. msg: ERROR_INVALID_DWP_HANDLE - Invalid handle to a multiple-window position structure. */
#define ERROR_INVALID_DWP_HANDLE	((ULONG)0x0000057dL)

/* MessageId  : 0x0000057e */
/* Approx. msg: ERROR_TLW_WITH_WSCHILD - Cannot create a top-level child window. */
#define ERROR_TLW_WITH_WSCHILD	((ULONG)0x0000057eL)

/* MessageId  : 0x0000057f */
/* Approx. msg: ERROR_CANNOT_FIND_WND_CLASS - Cannot find window class. */
#define ERROR_CANNOT_FIND_WND_CLASS	((ULONG)0x0000057fL)

/* MessageId  : 0x00000580 */
/* Approx. msg: ERROR_WINDOW_OF_OTHER_THREAD - Invalid window; it belongs to other thread. */
#define ERROR_WINDOW_OF_OTHER_THREAD	((ULONG)0x00000580L)

/* MessageId  : 0x00000581 */
/* Approx. msg: ERROR_HOTKEY_ALREADY_REGISTERED - Hot key is already registered. */
#define ERROR_HOTKEY_ALREADY_REGISTERED	((ULONG)0x00000581L)

/* MessageId  : 0x00000582 */
/* Approx. msg: ERROR_CLASS_ALREADY_EXISTS - Class already exists. */
#define ERROR_CLASS_ALREADY_EXISTS	((ULONG)0x00000582L)

/* MessageId  : 0x00000583 */
/* Approx. msg: ERROR_CLASS_DOES_NOT_EXIST - Class does not exist. */
#define ERROR_CLASS_DOES_NOT_EXIST	((ULONG)0x00000583L)

/* MessageId  : 0x00000584 */
/* Approx. msg: ERROR_CLASS_HAS_WINDOWS - Class still has open windows. */
#define ERROR_CLASS_HAS_WINDOWS	((ULONG)0x00000584L)

/* MessageId  : 0x00000585 */
/* Approx. msg: ERROR_INVALID_INDEX - Invalid index. */
#define ERROR_INVALID_INDEX	((ULONG)0x00000585L)

/* MessageId  : 0x00000586 */
/* Approx. msg: ERROR_INVALID_ICON_HANDLE - Invalid icon handle. */
#define ERROR_INVALID_ICON_HANDLE	((ULONG)0x00000586L)

/* MessageId  : 0x00000587 */
/* Approx. msg: ERROR_PRIVATE_DIALOG_INDEX - Using private DIALOG window words. */
#define ERROR_PRIVATE_DIALOG_INDEX	((ULONG)0x00000587L)

/* MessageId  : 0x00000588 */
/* Approx. msg: ERROR_LISTBOX_ID_NOT_FOUND - The list box identifier was not found. */
#define ERROR_LISTBOX_ID_NOT_FOUND	((ULONG)0x00000588L)

/* MessageId  : 0x00000589 */
/* Approx. msg: ERROR_NO_WILDCARD_CHARACTERS - No wildcards were found. */
#define ERROR_NO_WILDCARD_CHARACTERS	((ULONG)0x00000589L)

/* MessageId  : 0x0000058a */
/* Approx. msg: ERROR_CLIPBOARD_NOT_OPEN - Thread does not have a clipboard open. */
#define ERROR_CLIPBOARD_NOT_OPEN	((ULONG)0x0000058aL)

/* MessageId  : 0x0000058b */
/* Approx. msg: ERROR_HOTKEY_NOT_REGISTERED - Hot key is not registered. */
#define ERROR_HOTKEY_NOT_REGISTERED	((ULONG)0x0000058bL)

/* MessageId  : 0x0000058c */
/* Approx. msg: ERROR_WINDOW_NOT_DIALOG - The window is not a valid dialog window. */
#define ERROR_WINDOW_NOT_DIALOG	((ULONG)0x0000058cL)

/* MessageId  : 0x0000058d */
/* Approx. msg: ERROR_CONTROL_ID_NOT_FOUND - Control ID not found. */
#define ERROR_CONTROL_ID_NOT_FOUND	((ULONG)0x0000058dL)

/* MessageId  : 0x0000058e */
/* Approx. msg: ERROR_INVALID_COMBOBOX_MESSAGE - Invalid message for a combo box because it does not have an edit control. */
#define ERROR_INVALID_COMBOBOX_MESSAGE	((ULONG)0x0000058eL)

/* MessageId  : 0x0000058f */
/* Approx. msg: ERROR_WINDOW_NOT_COMBOBOX - The window is not a combo box. */
#define ERROR_WINDOW_NOT_COMBOBOX	((ULONG)0x0000058fL)

/* MessageId  : 0x00000590 */
/* Approx. msg: ERROR_INVALID_EDIT_HEIGHT - Height must be less than 256. */
#define ERROR_INVALID_EDIT_HEIGHT	((ULONG)0x00000590L)

/* MessageId  : 0x00000591 */
/* Approx. msg: ERROR_DC_NOT_FOUND - Invalid device context (DC) handle. */
#define ERROR_DC_NOT_FOUND	((ULONG)0x00000591L)

/* MessageId  : 0x00000592 */
/* Approx. msg: ERROR_INVALID_HOOK_FILTER - Invalid hook procedure type. */
#define ERROR_INVALID_HOOK_FILTER	((ULONG)0x00000592L)

/* MessageId  : 0x00000593 */
/* Approx. msg: ERROR_INVALID_FILTER_PROC - Invalid hook procedure. */
#define ERROR_INVALID_FILTER_PROC	((ULONG)0x00000593L)

/* MessageId  : 0x00000594 */
/* Approx. msg: ERROR_HOOK_NEEDS_HMOD - Cannot set nonlocal hook without a module handle. */
#define ERROR_HOOK_NEEDS_HMOD	((ULONG)0x00000594L)

/* MessageId  : 0x00000595 */
/* Approx. msg: ERROR_GLOBAL_ONLY_HOOK - This hook procedure can only be set globally. */
#define ERROR_GLOBAL_ONLY_HOOK	((ULONG)0x00000595L)

/* MessageId  : 0x00000596 */
/* Approx. msg: ERROR_JOURNAL_HOOK_SET - The journal hook procedure is already installed. */
#define ERROR_JOURNAL_HOOK_SET	((ULONG)0x00000596L)

/* MessageId  : 0x00000597 */
/* Approx. msg: ERROR_HOOK_NOT_INSTALLED - The hook procedure is not installed. */
#define ERROR_HOOK_NOT_INSTALLED	((ULONG)0x00000597L)

/* MessageId  : 0x00000598 */
/* Approx. msg: ERROR_INVALID_LB_MESSAGE - Invalid message for single-selection list box. */
#define ERROR_INVALID_LB_MESSAGE	((ULONG)0x00000598L)

/* MessageId  : 0x00000599 */
/* Approx. msg: ERROR_SETCOUNT_ON_BAD_LB - LB_SETCOUNT sent to non-lazy list box. */
#define ERROR_SETCOUNT_ON_BAD_LB	((ULONG)0x00000599L)

/* MessageId  : 0x0000059a */
/* Approx. msg: ERROR_LB_WITHOUT_TABSTOPS - This list box does not support tab stops. */
#define ERROR_LB_WITHOUT_TABSTOPS	((ULONG)0x0000059aL)

/* MessageId  : 0x0000059b */
/* Approx. msg: ERROR_DESTROY_OBJECT_OF_OTHER_THREAD - Cannot destroy object created by another thread. */
#define ERROR_DESTROY_OBJECT_OF_OTHER_THREAD	((ULONG)0x0000059bL)

/* MessageId  : 0x0000059c */
/* Approx. msg: ERROR_CHILD_WINDOW_MENU - Child windows cannot have menus. */
#define ERROR_CHILD_WINDOW_MENU	((ULONG)0x0000059cL)

/* MessageId  : 0x0000059d */
/* Approx. msg: ERROR_NO_SYSTEM_MENU - The window does not have a system menu. */
#define ERROR_NO_SYSTEM_MENU	((ULONG)0x0000059dL)

/* MessageId  : 0x0000059e */
/* Approx. msg: ERROR_INVALID_MSGBOX_STYLE - Invalid message box style. */
#define ERROR_INVALID_MSGBOX_STYLE	((ULONG)0x0000059eL)

/* MessageId  : 0x0000059f */
/* Approx. msg: ERROR_INVALID_SPI_VALUE - Invalid system-wide (SPI_*) parameter. */
#define ERROR_INVALID_SPI_VALUE	((ULONG)0x0000059fL)

/* MessageId  : 0x000005a0 */
/* Approx. msg: ERROR_SCREEN_ALREADY_LOCKED - Screen already locked. */
#define ERROR_SCREEN_ALREADY_LOCKED	((ULONG)0x000005a0L)

/* MessageId  : 0x000005a1 */
/* Approx. msg: ERROR_HWNDS_HAVE_DIFF_PARENT - All handles to windows in a multiple-window position structure must have the same parent. */
#define ERROR_HWNDS_HAVE_DIFF_PARENT	((ULONG)0x000005a1L)

/* MessageId  : 0x000005a2 */
/* Approx. msg: ERROR_NOT_CHILD_WINDOW - The window is not a child window. */
#define ERROR_NOT_CHILD_WINDOW	((ULONG)0x000005a2L)

/* MessageId  : 0x000005a3 */
/* Approx. msg: ERROR_INVALID_GW_COMMAND - Invalid GW_* command. */
#define ERROR_INVALID_GW_COMMAND	((ULONG)0x000005a3L)

/* MessageId  : 0x000005a4 */
/* Approx. msg: ERROR_INVALID_THREAD_ID - Invalid thread identifier. */
#define ERROR_INVALID_THREAD_ID	((ULONG)0x000005a4L)

/* MessageId  : 0x000005a5 */
/* Approx. msg: ERROR_NON_MDICHILD_WINDOW - Cannot process a message from a window that is not a multiple document interface (MDI) window. */
#define ERROR_NON_MDICHILD_WINDOW	((ULONG)0x000005a5L)

/* MessageId  : 0x000005a6 */
/* Approx. msg: ERROR_POPUP_ALREADY_ACTIVE - Popup menu already active. */
#define ERROR_POPUP_ALREADY_ACTIVE	((ULONG)0x000005a6L)

/* MessageId  : 0x000005a7 */
/* Approx. msg: ERROR_NO_SCROLLBARS - The window does not have scroll bars. */
#define ERROR_NO_SCROLLBARS	((ULONG)0x000005a7L)

/* MessageId  : 0x000005a8 */
/* Approx. msg: ERROR_INVALID_SCROLLBAR_RANGE - Scroll bar range cannot be greater than MAXLONG. */
#define ERROR_INVALID_SCROLLBAR_RANGE	((ULONG)0x000005a8L)

/* MessageId  : 0x000005a9 */
/* Approx. msg: ERROR_INVALID_SHOWWIN_COMMAND - Cannot show or remove the window in the way specified. */
#define ERROR_INVALID_SHOWWIN_COMMAND	((ULONG)0x000005a9L)

/* MessageId  : 0x000005aa */
/* Approx. msg: ERROR_NO_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service. */
#define ERROR_NO_SYSTEM_RESOURCES	((ULONG)0x000005aaL)

/* MessageId  : 0x000005ab */
/* Approx. msg: ERROR_NONPAGED_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service. */
#define ERROR_NONPAGED_SYSTEM_RESOURCES	((ULONG)0x000005abL)

/* MessageId  : 0x000005ac */
/* Approx. msg: ERROR_PAGED_SYSTEM_RESOURCES - Insufficient system resources exist to complete the requested service. */
#define ERROR_PAGED_SYSTEM_RESOURCES	((ULONG)0x000005acL)

/* MessageId  : 0x000005ad */
/* Approx. msg: ERROR_WORKING_SET_QUOTA - Insufficient quota to complete the requested service. */
#define ERROR_WORKING_SET_QUOTA	((ULONG)0x000005adL)

/* MessageId  : 0x000005ae */
/* Approx. msg: ERROR_PAGEFILE_QUOTA - Insufficient quota to complete the requested service. */
#define ERROR_PAGEFILE_QUOTA	((ULONG)0x000005aeL)

/* MessageId  : 0x000005af */
/* Approx. msg: ERROR_COMMITMENT_LIMIT - The paging file is too small for this operation to complete. */
#define ERROR_COMMITMENT_LIMIT	((ULONG)0x000005afL)

/* MessageId  : 0x000005b0 */
/* Approx. msg: ERROR_MENU_ITEM_NOT_FOUND - A menu item was not found. */
#define ERROR_MENU_ITEM_NOT_FOUND	((ULONG)0x000005b0L)

/* MessageId  : 0x000005b1 */
/* Approx. msg: ERROR_INVALID_KEYBOARD_HANDLE - Invalid keyboard layout handle. */
#define ERROR_INVALID_KEYBOARD_HANDLE	((ULONG)0x000005b1L)

/* MessageId  : 0x000005b2 */
/* Approx. msg: ERROR_HOOK_TYPE_NOT_ALLOWED - Hook type not allowed. */
#define ERROR_HOOK_TYPE_NOT_ALLOWED	((ULONG)0x000005b2L)

/* MessageId  : 0x000005b3 */
/* Approx. msg: ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION - This operation requires an interactive window station. */
#define ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION	((ULONG)0x000005b3L)

/* MessageId  : 0x000005b4 */
/* Approx. msg: ERROR_TIMEOUT - This operation returned because the timeout period expired. */
#define ERROR_TIMEOUT	((ULONG)0x000005b4L)

/* MessageId  : 0x000005b5 */
/* Approx. msg: ERROR_INVALID_MONITOR_HANDLE - Invalid monitor handle. */
#define ERROR_INVALID_MONITOR_HANDLE	((ULONG)0x000005b5L)

/* MessageId  : 0x000005dc */
/* Approx. msg: ERROR_EVENTLOG_FILE_CORRUPT - The event log file is corrupted. */
#define ERROR_EVENTLOG_FILE_CORRUPT	((ULONG)0x000005dcL)

/* MessageId  : 0x000005dd */
/* Approx. msg: ERROR_EVENTLOG_CANT_START - No event log file could be opened, so the event logging service did not start. */
#define ERROR_EVENTLOG_CANT_START	((ULONG)0x000005ddL)

/* MessageId  : 0x000005de */
/* Approx. msg: ERROR_LOG_FILE_FULL - The event log file is full. */
#define ERROR_LOG_FILE_FULL	((ULONG)0x000005deL)

/* MessageId  : 0x000005df */
/* Approx. msg: ERROR_EVENTLOG_FILE_CHANGED - The event log file has changed between read operations. */
#define ERROR_EVENTLOG_FILE_CHANGED	((ULONG)0x000005dfL)

/* MessageId  : 0x00000641 */
/* Approx. msg: ERROR_INSTALL_SERVICE_FAILURE - The Windows Installer service could not be accessed. This can occur if you are running Windows in safe mode, or if the Windows Installer is not correctly installed. Contact your support personnel for assistance. */
#define ERROR_INSTALL_SERVICE_FAILURE	((ULONG)0x00000641L)

/* MessageId  : 0x00000642 */
/* Approx. msg: ERROR_INSTALL_USEREXIT - User cancelled installation. */
#define ERROR_INSTALL_USEREXIT	((ULONG)0x00000642L)

/* MessageId  : 0x00000643 */
/* Approx. msg: ERROR_INSTALL_FAILURE - Fatal error during installation. */
#define ERROR_INSTALL_FAILURE	((ULONG)0x00000643L)

/* MessageId  : 0x00000644 */
/* Approx. msg: ERROR_INSTALL_SUSPEND - Installation suspended, incomplete. */
#define ERROR_INSTALL_SUSPEND	((ULONG)0x00000644L)

/* MessageId  : 0x00000645 */
/* Approx. msg: ERROR_UNKNOWN_PRODUCT - This action is only valid for products that are currently installed. */
#define ERROR_UNKNOWN_PRODUCT	((ULONG)0x00000645L)

/* MessageId  : 0x00000646 */
/* Approx. msg: ERROR_UNKNOWN_FEATURE - Feature ID not registered. */
#define ERROR_UNKNOWN_FEATURE	((ULONG)0x00000646L)

/* MessageId  : 0x00000647 */
/* Approx. msg: ERROR_UNKNOWN_COMPONENT - Component ID not registered. */
#define ERROR_UNKNOWN_COMPONENT	((ULONG)0x00000647L)

/* MessageId  : 0x00000648 */
/* Approx. msg: ERROR_UNKNOWN_PROPERTY - Unknown property. */
#define ERROR_UNKNOWN_PROPERTY	((ULONG)0x00000648L)

/* MessageId  : 0x00000649 */
/* Approx. msg: ERROR_INVALID_HANDLE_STATE - Handle is in an invalid state. */
#define ERROR_INVALID_HANDLE_STATE	((ULONG)0x00000649L)

/* MessageId  : 0x0000064a */
/* Approx. msg: ERROR_BAD_CONFIGURATION - The configuration data for this product is corrupt. Contact your support personnel. */
#define ERROR_BAD_CONFIGURATION	((ULONG)0x0000064aL)

/* MessageId  : 0x0000064b */
/* Approx. msg: ERROR_INDEX_ABSENT - Component qualifier not present. */
#define ERROR_INDEX_ABSENT	((ULONG)0x0000064bL)

/* MessageId  : 0x0000064c */
/* Approx. msg: ERROR_INSTALL_SOURCE_ABSENT - The installation source for this product is not available. Verify that the source exists and that you can access it. */
#define ERROR_INSTALL_SOURCE_ABSENT	((ULONG)0x0000064cL)

/* MessageId  : 0x0000064d */
/* Approx. msg: ERROR_INSTALL_PACKAGE_VERSION - This installation package cannot be installed by the Windows Installer service. You must install a Windows service pack that contains a newer version of the Windows Installer service. */
#define ERROR_INSTALL_PACKAGE_VERSION	((ULONG)0x0000064dL)

/* MessageId  : 0x0000064e */
/* Approx. msg: ERROR_PRODUCT_UNINSTALLED - Product is uninstalled. */
#define ERROR_PRODUCT_UNINSTALLED	((ULONG)0x0000064eL)

/* MessageId  : 0x0000064f */
/* Approx. msg: ERROR_BAD_QUERY_SYNTAX - SQL query syntax invalid or unsupported. */
#define ERROR_BAD_QUERY_SYNTAX	((ULONG)0x0000064fL)

/* MessageId  : 0x00000650 */
/* Approx. msg: ERROR_INVALID_FIELD - Record field does not exist. */
#define ERROR_INVALID_FIELD	((ULONG)0x00000650L)

/* MessageId  : 0x00000651 */
/* Approx. msg: ERROR_DEVICE_REMOVED - The device has been removed. */
#define ERROR_DEVICE_REMOVED	((ULONG)0x00000651L)

/* MessageId  : 0x00000652 */
/* Approx. msg: ERROR_INSTALL_ALREADY_RUNNING - Another installation is already in progress. Complete that installation before proceeding with this install. */
#define ERROR_INSTALL_ALREADY_RUNNING	((ULONG)0x00000652L)

/* MessageId  : 0x00000653 */
/* Approx. msg: ERROR_INSTALL_PACKAGE_OPEN_FAILED - This installation package could not be opened. Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package. */
#define ERROR_INSTALL_PACKAGE_OPEN_FAILED	((ULONG)0x00000653L)

/* MessageId  : 0x00000654 */
/* Approx. msg: ERROR_INSTALL_PACKAGE_INVALID - This installation package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer package. */
#define ERROR_INSTALL_PACKAGE_INVALID	((ULONG)0x00000654L)

/* MessageId  : 0x00000655 */
/* Approx. msg: ERROR_INSTALL_UI_FAILURE - There was an error starting the Windows Installer service user interface. Contact your support personnel. */
#define ERROR_INSTALL_UI_FAILURE	((ULONG)0x00000655L)

/* MessageId  : 0x00000656 */
/* Approx. msg: ERROR_INSTALL_LOG_FAILURE - Error opening installation log file. Verify that the specified log file location exists and that you can write to it. */
#define ERROR_INSTALL_LOG_FAILURE	((ULONG)0x00000656L)

/* MessageId  : 0x00000657 */
/* Approx. msg: ERROR_INSTALL_LANGUAGE_UNSUPPORTED - The language of this installation package is not supported by your system. */
#define ERROR_INSTALL_LANGUAGE_UNSUPPORTED	((ULONG)0x00000657L)

/* MessageId  : 0x00000658 */
/* Approx. msg: ERROR_INSTALL_TRANSFORM_FAILURE - Error applying transforms. Verify that the specified transform paths are valid. */
#define ERROR_INSTALL_TRANSFORM_FAILURE	((ULONG)0x00000658L)

/* MessageId  : 0x00000659 */
/* Approx. msg: ERROR_INSTALL_PACKAGE_REJECTED - This installation is forbidden by system policy. Contact your system administrator. */
#define ERROR_INSTALL_PACKAGE_REJECTED	((ULONG)0x00000659L)

/* MessageId  : 0x0000065a */
/* Approx. msg: ERROR_FUNCTION_NOT_CALLED - Function could not be executed. */
#define ERROR_FUNCTION_NOT_CALLED	((ULONG)0x0000065aL)

/* MessageId  : 0x0000065b */
/* Approx. msg: ERROR_FUNCTION_FAILED - Function failed during execution. */
#define ERROR_FUNCTION_FAILED	((ULONG)0x0000065bL)

/* MessageId  : 0x0000065c */
/* Approx. msg: ERROR_INVALID_TABLE - Invalid or unknown table specified. */
#define ERROR_INVALID_TABLE	((ULONG)0x0000065cL)

/* MessageId  : 0x0000065d */
/* Approx. msg: ERROR_DATATYPE_MISMATCH - Data supplied is of wrong type. */
#define ERROR_DATATYPE_MISMATCH	((ULONG)0x0000065dL)

/* MessageId  : 0x0000065e */
/* Approx. msg: ERROR_UNSUPPORTED_TYPE - Data of this type is not supported. */
#define ERROR_UNSUPPORTED_TYPE	((ULONG)0x0000065eL)

/* MessageId  : 0x0000065f */
/* Approx. msg: ERROR_CREATE_FAILED - The Windows Installer service failed to start. Contact your support personnel. */
#define ERROR_CREATE_FAILED	((ULONG)0x0000065fL)

/* MessageId  : 0x00000660 */
/* Approx. msg: ERROR_INSTALL_TEMP_UNWRITABLE - The Temp folder is on a drive that is full or inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder. */
#define ERROR_INSTALL_TEMP_UNWRITABLE	((ULONG)0x00000660L)

/* MessageId  : 0x00000661 */
/* Approx. msg: ERROR_INSTALL_PLATFORM_UNSUPPORTED - This installation package is not supported by this processor type. Contact your product vendor. */
#define ERROR_INSTALL_PLATFORM_UNSUPPORTED	((ULONG)0x00000661L)

/* MessageId  : 0x00000662 */
/* Approx. msg: ERROR_INSTALL_NOTUSED - Component not used on this computer. */
#define ERROR_INSTALL_NOTUSED	((ULONG)0x00000662L)

/* MessageId  : 0x00000663 */
/* Approx. msg: ERROR_PATCH_PACKAGE_OPEN_FAILED - This patch package could not be opened. Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package. */
#define ERROR_PATCH_PACKAGE_OPEN_FAILED	((ULONG)0x00000663L)

/* MessageId  : 0x00000664 */
/* Approx. msg: ERROR_PATCH_PACKAGE_INVALID - This patch package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer patch package. */
#define ERROR_PATCH_PACKAGE_INVALID	((ULONG)0x00000664L)

/* MessageId  : 0x00000665 */
/* Approx. msg: ERROR_PATCH_PACKAGE_UNSUPPORTED - This patch package cannot be processed by the Windows Installer service. You must install a Windows service pack that contains a newer version of the Windows Installer service. */
#define ERROR_PATCH_PACKAGE_UNSUPPORTED	((ULONG)0x00000665L)

/* MessageId  : 0x00000666 */
/* Approx. msg: ERROR_PRODUCT_VERSION - Another version of this product is already installed. Installation of this version cannot continue. To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel. */
#define ERROR_PRODUCT_VERSION	((ULONG)0x00000666L)

/* MessageId  : 0x00000667 */
/* Approx. msg: ERROR_INVALID_COMMAND_LINE - Invalid command line argument. Consult the Windows Installer SDK for detailed command line help. */
#define ERROR_INVALID_COMMAND_LINE	((ULONG)0x00000667L)

/* MessageId  : 0x00000668 */
/* Approx. msg: ERROR_INSTALL_REMOTE_DISALLOWED - Only administrators have permission to add, remove, or configure server software during a Terminal Services remote session. If you want to install or configure software on the server, contact your network administrator. */
#define ERROR_INSTALL_REMOTE_DISALLOWED	((ULONG)0x00000668L)

/* MessageId  : 0x00000669 */
/* Approx. msg: ERROR_SUCCESS_REBOOT_INITIATED - The requested operation completed successfully. The system will be restarted so the changes can take effect. */
#define ERROR_SUCCESS_REBOOT_INITIATED	((ULONG)0x00000669L)

/* MessageId  : 0x0000066a */
/* Approx. msg: ERROR_PATCH_TARGET_NOT_FOUND - The upgrade patch cannot be installed by the Windows Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch. */
#define ERROR_PATCH_TARGET_NOT_FOUND	((ULONG)0x0000066aL)

/* MessageId  : 0x0000066b */
/* Approx. msg: ERROR_PATCH_PACKAGE_REJECTED - The patch package is not permitted by software restriction policy. */
#define ERROR_PATCH_PACKAGE_REJECTED	((ULONG)0x0000066bL)

/* MessageId  : 0x0000066c */
/* Approx. msg: ERROR_INSTALL_TRANSFORM_REJECTED - One or more customizations are not permitted by software restriction policy. */
#define ERROR_INSTALL_TRANSFORM_REJECTED	((ULONG)0x0000066cL)

/* MessageId  : 0x0000066d */
/* Approx. msg: ERROR_INSTALL_REMOTE_PROHIBITED - The Windows Installer does not permit installation from a Remote Desktop Connection. */
#define ERROR_INSTALL_REMOTE_PROHIBITED	((ULONG)0x0000066dL)

/* MessageId  : 0x000006a4 */
/* Approx. msg: RPC_S_INVALID_STRING_BINDING - The string binding is invalid. */
#define RPC_S_INVALID_STRING_BINDING	((ULONG)0x000006a4L)

/* MessageId  : 0x000006a5 */
/* Approx. msg: RPC_S_WRONG_KIND_OF_BINDING - The binding handle is not the correct type. */
#define RPC_S_WRONG_KIND_OF_BINDING	((ULONG)0x000006a5L)

/* MessageId  : 0x000006a6 */
/* Approx. msg: RPC_S_INVALID_BINDING - The binding handle is invalid. */
#define RPC_S_INVALID_BINDING	((ULONG)0x000006a6L)

/* MessageId  : 0x000006a7 */
/* Approx. msg: RPC_S_PROTSEQ_NOT_SUPPORTED - The RPC protocol sequence is not supported. */
#define RPC_S_PROTSEQ_NOT_SUPPORTED	((ULONG)0x000006a7L)

/* MessageId  : 0x000006a8 */
/* Approx. msg: RPC_S_INVALID_RPC_PROTSEQ - The RPC protocol sequence is invalid. */
#define RPC_S_INVALID_RPC_PROTSEQ	((ULONG)0x000006a8L)

/* MessageId  : 0x000006a9 */
/* Approx. msg: RPC_S_INVALID_STRING_UUID - The string universal unique identifier (UUID) is invalid. */
#define RPC_S_INVALID_STRING_UUID	((ULONG)0x000006a9L)

/* MessageId  : 0x000006aa */
/* Approx. msg: RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint format is invalid. */
#define RPC_S_INVALID_ENDPOINT_FORMAT	((ULONG)0x000006aaL)

/* MessageId  : 0x000006ab */
/* Approx. msg: RPC_S_INVALID_NET_ADDR - The network address is invalid. */
#define RPC_S_INVALID_NET_ADDR	((ULONG)0x000006abL)

/* MessageId  : 0x000006ac */
/* Approx. msg: RPC_S_NO_ENDPOINT_FOUND - No endpoint was found. */
#define RPC_S_NO_ENDPOINT_FOUND	((ULONG)0x000006acL)

/* MessageId  : 0x000006ad */
/* Approx. msg: RPC_S_INVALID_TIMEOUT - The timeout value is invalid. */
#define RPC_S_INVALID_TIMEOUT	((ULONG)0x000006adL)

/* MessageId  : 0x000006ae */
/* Approx. msg: RPC_S_OBJECT_NOT_FOUND - The object universal unique identifier (UUID) was not found. */
#define RPC_S_OBJECT_NOT_FOUND	((ULONG)0x000006aeL)

/* MessageId  : 0x000006af */
/* Approx. msg: RPC_S_ALREADY_REGISTERED - The object universal unique identifier (UUID) has already been registered. */
#define RPC_S_ALREADY_REGISTERED	((ULONG)0x000006afL)

/* MessageId  : 0x000006b0 */
/* Approx. msg: RPC_S_TYPE_ALREADY_REGISTERED - The type universal unique identifier (UUID) has already been registered. */
#define RPC_S_TYPE_ALREADY_REGISTERED	((ULONG)0x000006b0L)

/* MessageId  : 0x000006b1 */
/* Approx. msg: RPC_S_ALREADY_LISTENING - The RPC server is already listening. */
#define RPC_S_ALREADY_LISTENING	((ULONG)0x000006b1L)

/* MessageId  : 0x000006b2 */
/* Approx. msg: RPC_S_NO_PROTSEQS_REGISTERED - No protocol sequences have been registered. */
#define RPC_S_NO_PROTSEQS_REGISTERED	((ULONG)0x000006b2L)

/* MessageId  : 0x000006b3 */
/* Approx. msg: RPC_S_NOT_LISTENING - The RPC server is not listening. */
#define RPC_S_NOT_LISTENING	((ULONG)0x000006b3L)

/* MessageId  : 0x000006b4 */
/* Approx. msg: RPC_S_UNKNOWN_MGR_TYPE - The manager type is unknown. */
#define RPC_S_UNKNOWN_MGR_TYPE	((ULONG)0x000006b4L)

/* MessageId  : 0x000006b5 */
/* Approx. msg: RPC_S_UNKNOWN_IF - The interface is unknown. */
#define RPC_S_UNKNOWN_IF	((ULONG)0x000006b5L)

/* MessageId  : 0x000006b6 */
/* Approx. msg: RPC_S_NO_BINDINGS - There are no bindings. */
#define RPC_S_NO_BINDINGS	((ULONG)0x000006b6L)

/* MessageId  : 0x000006b7 */
/* Approx. msg: RPC_S_NO_PROTSEQS - There are no protocol sequences. */
#define RPC_S_NO_PROTSEQS	((ULONG)0x000006b7L)

/* MessageId  : 0x000006b8 */
/* Approx. msg: RPC_S_CANT_CREATE_ENDPOINT - The endpoint cannot be created. */
#define RPC_S_CANT_CREATE_ENDPOINT	((ULONG)0x000006b8L)

/* MessageId  : 0x000006b9 */
/* Approx. msg: RPC_S_OUT_OF_RESOURCES - Not enough resources are available to complete this operation. */
#define RPC_S_OUT_OF_RESOURCES	((ULONG)0x000006b9L)

/* MessageId  : 0x000006ba */
/* Approx. msg: RPC_S_SERVER_UNAVAILABLE - The RPC server is unavailable. */
#define RPC_S_SERVER_UNAVAILABLE	((ULONG)0x000006baL)

/* MessageId  : 0x000006bb */
/* Approx. msg: RPC_S_SERVER_TOO_BUSY - The RPC server is too busy to complete this operation. */
#define RPC_S_SERVER_TOO_BUSY	((ULONG)0x000006bbL)

/* MessageId  : 0x000006bc */
/* Approx. msg: RPC_S_INVALID_NETWORK_OPTIONS - The network options are invalid. */
#define RPC_S_INVALID_NETWORK_OPTIONS	((ULONG)0x000006bcL)

/* MessageId  : 0x000006bd */
/* Approx. msg: RPC_S_NO_CALL_ACTIVE - There are no remote procedure calls active on this thread. */
#define RPC_S_NO_CALL_ACTIVE	((ULONG)0x000006bdL)

/* MessageId  : 0x000006be */
/* Approx. msg: RPC_S_CALL_FAILED - The remote procedure call failed. */
#define RPC_S_CALL_FAILED	((ULONG)0x000006beL)

/* MessageId  : 0x000006bf */
/* Approx. msg: RPC_S_CALL_FAILED_DNE - The remote procedure call failed and did not execute. */
#define RPC_S_CALL_FAILED_DNE	((ULONG)0x000006bfL)

/* MessageId  : 0x000006c0 */
/* Approx. msg: RPC_S_PROTOCOL_ERROR - A remote procedure call (RPC) protocol error occurred. */
#define RPC_S_PROTOCOL_ERROR	((ULONG)0x000006c0L)

/* MessageId  : 0x000006c2 */
/* Approx. msg: RPC_S_UNSUPPORTED_TRANS_SYN - The transfer syntax is not supported by the RPC server. */
#define RPC_S_UNSUPPORTED_TRANS_SYN	((ULONG)0x000006c2L)

/* MessageId  : 0x000006c4 */
/* Approx. msg: RPC_S_UNSUPPORTED_TYPE - The universal unique identifier (UUID) type is not supported. */
#define RPC_S_UNSUPPORTED_TYPE	((ULONG)0x000006c4L)

/* MessageId  : 0x000006c5 */
/* Approx. msg: RPC_S_INVALID_TAG - The tag is invalid. */
#define RPC_S_INVALID_TAG	((ULONG)0x000006c5L)

/* MessageId  : 0x000006c6 */
/* Approx. msg: RPC_S_INVALID_BOUND - The array bounds are invalid. */
#define RPC_S_INVALID_BOUND	((ULONG)0x000006c6L)

/* MessageId  : 0x000006c7 */
/* Approx. msg: RPC_S_NO_ENTRY_NAME - The binding does not contain an entry name. */
#define RPC_S_NO_ENTRY_NAME	((ULONG)0x000006c7L)

/* MessageId  : 0x000006c8 */
/* Approx. msg: RPC_S_INVALID_NAME_SYNTAX - The name syntax is invalid. */
#define RPC_S_INVALID_NAME_SYNTAX	((ULONG)0x000006c8L)

/* MessageId  : 0x000006c9 */
/* Approx. msg: RPC_S_UNSUPPORTED_NAME_SYNTAX - The name syntax is not supported. */
#define RPC_S_UNSUPPORTED_NAME_SYNTAX	((ULONG)0x000006c9L)

/* MessageId  : 0x000006cb */
/* Approx. msg: RPC_S_UUID_NO_ADDRESS - No network address is available to use to construct a universal unique identifier (UUID). */
#define RPC_S_UUID_NO_ADDRESS	((ULONG)0x000006cbL)

/* MessageId  : 0x000006cc */
/* Approx. msg: RPC_S_DUPLICATE_ENDPOINT - The endpoint is a duplicate. */
#define RPC_S_DUPLICATE_ENDPOINT	((ULONG)0x000006ccL)

/* MessageId  : 0x000006cd */
/* Approx. msg: RPC_S_UNKNOWN_AUTHN_TYPE - The authentication type is unknown. */
#define RPC_S_UNKNOWN_AUTHN_TYPE	((ULONG)0x000006cdL)

/* MessageId  : 0x000006ce */
/* Approx. msg: RPC_S_MAX_CALLS_TOO_SMALL - The maximum number of calls is too small. */
#define RPC_S_MAX_CALLS_TOO_SMALL	((ULONG)0x000006ceL)

/* MessageId  : 0x000006cf */
/* Approx. msg: RPC_S_STRING_TOO_LONG - The string is too long. */
#define RPC_S_STRING_TOO_LONG	((ULONG)0x000006cfL)

/* MessageId  : 0x000006d0 */
/* Approx. msg: RPC_S_PROTSEQ_NOT_FOUND - The RPC protocol sequence was not found. */
#define RPC_S_PROTSEQ_NOT_FOUND	((ULONG)0x000006d0L)

/* MessageId  : 0x000006d1 */
/* Approx. msg: RPC_S_PROCNUM_OUT_OF_RANGE - The procedure number is out of range. */
#define RPC_S_PROCNUM_OUT_OF_RANGE	((ULONG)0x000006d1L)

/* MessageId  : 0x000006d2 */
/* Approx. msg: RPC_S_BINDING_HAS_NO_AUTH - The binding does not contain any authentication information. */
#define RPC_S_BINDING_HAS_NO_AUTH	((ULONG)0x000006d2L)

/* MessageId  : 0x000006d3 */
/* Approx. msg: RPC_S_UNKNOWN_AUTHN_SERVICE - The authentication service is unknown. */
#define RPC_S_UNKNOWN_AUTHN_SERVICE	((ULONG)0x000006d3L)

/* MessageId  : 0x000006d4 */
/* Approx. msg: RPC_S_UNKNOWN_AUTHN_LEVEL - The authentication level is unknown. */
#define RPC_S_UNKNOWN_AUTHN_LEVEL	((ULONG)0x000006d4L)

/* MessageId  : 0x000006d5 */
/* Approx. msg: RPC_S_INVALID_AUTH_IDENTITY - The security context is invalid. */
#define RPC_S_INVALID_AUTH_IDENTITY	((ULONG)0x000006d5L)

/* MessageId  : 0x000006d6 */
/* Approx. msg: RPC_S_UNKNOWN_AUTHZ_SERVICE - The authorization service is unknown. */
#define RPC_S_UNKNOWN_AUTHZ_SERVICE	((ULONG)0x000006d6L)

/* MessageId  : 0x000006d7 */
/* Approx. msg: EPT_S_INVALID_ENTRY - The entry is invalid. */
#define EPT_S_INVALID_ENTRY	((ULONG)0x000006d7L)

/* MessageId  : 0x000006d8 */
/* Approx. msg: EPT_S_CANT_PERFORM_OP - The server endpoint cannot perform the operation. */
#define EPT_S_CANT_PERFORM_OP	((ULONG)0x000006d8L)

/* MessageId  : 0x000006d9 */
/* Approx. msg: EPT_S_NOT_REGISTERED - There are no more endpoints available from the endpoint mapper. */
#define EPT_S_NOT_REGISTERED	((ULONG)0x000006d9L)

/* MessageId  : 0x000006da */
/* Approx. msg: RPC_S_NOTHING_TO_EXPORT - No interfaces have been exported. */
#define RPC_S_NOTHING_TO_EXPORT	((ULONG)0x000006daL)

/* MessageId  : 0x000006db */
/* Approx. msg: RPC_S_INCOMPLETE_NAME - The entry name is incomplete. */
#define RPC_S_INCOMPLETE_NAME	((ULONG)0x000006dbL)

/* MessageId  : 0x000006dc */
/* Approx. msg: RPC_S_INVALID_VERS_OPTION - The version option is invalid. */
#define RPC_S_INVALID_VERS_OPTION	((ULONG)0x000006dcL)

/* MessageId  : 0x000006dd */
/* Approx. msg: RPC_S_NO_MORE_MEMBERS - There are no more members. */
#define RPC_S_NO_MORE_MEMBERS	((ULONG)0x000006ddL)

/* MessageId  : 0x000006de */
/* Approx. msg: RPC_S_NOT_ALL_OBJS_UNEXPORTED - There is nothing to unexport. */
#define RPC_S_NOT_ALL_OBJS_UNEXPORTED	((ULONG)0x000006deL)

/* MessageId  : 0x000006df */
/* Approx. msg: RPC_S_INTERFACE_NOT_FOUND - The interface was not found. */
#define RPC_S_INTERFACE_NOT_FOUND	((ULONG)0x000006dfL)

/* MessageId  : 0x000006e0 */
/* Approx. msg: RPC_S_ENTRY_ALREADY_EXISTS - The entry already exists. */
#define RPC_S_ENTRY_ALREADY_EXISTS	((ULONG)0x000006e0L)

/* MessageId  : 0x000006e1 */
/* Approx. msg: RPC_S_ENTRY_NOT_FOUND - The entry is not found. */
#define RPC_S_ENTRY_NOT_FOUND	((ULONG)0x000006e1L)

/* MessageId  : 0x000006e2 */
/* Approx. msg: RPC_S_NAME_SERVICE_UNAVAILABLE - The name service is unavailable. */
#define RPC_S_NAME_SERVICE_UNAVAILABLE	((ULONG)0x000006e2L)

/* MessageId  : 0x000006e3 */
/* Approx. msg: RPC_S_INVALID_NAF_ID - The network address family is invalid. */
#define RPC_S_INVALID_NAF_ID	((ULONG)0x000006e3L)

/* MessageId  : 0x000006e4 */
/* Approx. msg: RPC_S_CANNOT_SUPPORT - The requested operation is not supported. */
#define RPC_S_CANNOT_SUPPORT	((ULONG)0x000006e4L)

/* MessageId  : 0x000006e5 */
/* Approx. msg: RPC_S_NO_CONTEXT_AVAILABLE - No security context is available to allow impersonation. */
#define RPC_S_NO_CONTEXT_AVAILABLE	((ULONG)0x000006e5L)

/* MessageId  : 0x000006e6 */
/* Approx. msg: RPC_S_INTERNAL_ERROR - An internal error occurred in a remote procedure call (RPC). */
#define RPC_S_INTERNAL_ERROR	((ULONG)0x000006e6L)

/* MessageId  : 0x000006e7 */
/* Approx. msg: RPC_S_ZERO_DIVIDE - The RPC server attempted an integer division by zero. */
#define RPC_S_ZERO_DIVIDE	((ULONG)0x000006e7L)

/* MessageId  : 0x000006e8 */
/* Approx. msg: RPC_S_ADDRESS_ERROR - An addressing error occurred in the RPC server. */
#define RPC_S_ADDRESS_ERROR	((ULONG)0x000006e8L)

/* MessageId  : 0x000006e9 */
/* Approx. msg: RPC_S_FP_DIV_ZERO - A floating-point operation at the RPC server caused a division by zero. */
#define RPC_S_FP_DIV_ZERO	((ULONG)0x000006e9L)

/* MessageId  : 0x000006ea */
/* Approx. msg: RPC_S_FP_UNDERFLOW - A floating-point underflow occurred at the RPC server. */
#define RPC_S_FP_UNDERFLOW	((ULONG)0x000006eaL)

/* MessageId  : 0x000006eb */
/* Approx. msg: RPC_S_FP_OVERFLOW - A floating-point overflow occurred at the RPC server. */
#define RPC_S_FP_OVERFLOW	((ULONG)0x000006ebL)

/* MessageId  : 0x000006ec */
/* Approx. msg: RPC_X_NO_MORE_ENTRIES - The list of RPC servers available for the binding of auto handles has been exhausted. */
#define RPC_X_NO_MORE_ENTRIES	((ULONG)0x000006ecL)

/* MessageId  : 0x000006ed */
/* Approx. msg: RPC_X_SS_CHAR_TRANS_OPEN_FAIL - Unable to open the character translation table file. */
#define RPC_X_SS_CHAR_TRANS_OPEN_FAIL	((ULONG)0x000006edL)

/* MessageId  : 0x000006ee */
/* Approx. msg: RPC_X_SS_CHAR_TRANS_SHORT_FILE - The file containing the character translation table has fewer than 512 bytes. */
#define RPC_X_SS_CHAR_TRANS_SHORT_FILE	((ULONG)0x000006eeL)

/* MessageId  : 0x000006ef */
/* Approx. msg: RPC_X_SS_IN_NULL_CONTEXT - A null context handle was passed from the client to the host during a remote procedure call. */
#define RPC_X_SS_IN_NULL_CONTEXT	((ULONG)0x000006efL)

/* MessageId  : 0x000006f1 */
/* Approx. msg: RPC_X_SS_CONTEXT_DAMAGED - The context handle changed during a remote procedure call. */
#define RPC_X_SS_CONTEXT_DAMAGED	((ULONG)0x000006f1L)

/* MessageId  : 0x000006f2 */
/* Approx. msg: RPC_X_SS_HANDLES_MISMATCH - The binding handles passed to a remote procedure call do not match. */
#define RPC_X_SS_HANDLES_MISMATCH	((ULONG)0x000006f2L)

/* MessageId  : 0x000006f3 */
/* Approx. msg: RPC_X_SS_CANNOT_GET_CALL_HANDLE - The stub is unable to get the remote procedure call handle. */
#define RPC_X_SS_CANNOT_GET_CALL_HANDLE	((ULONG)0x000006f3L)

/* MessageId  : 0x000006f4 */
/* Approx. msg: RPC_X_NULL_REF_POINTER - A null reference pointer was passed to the stub. */
#define RPC_X_NULL_REF_POINTER	((ULONG)0x000006f4L)

/* MessageId  : 0x000006f5 */
/* Approx. msg: RPC_X_ENUM_VALUE_OUT_OF_RANGE - The enumeration value is out of range. */
#define RPC_X_ENUM_VALUE_OUT_OF_RANGE	((ULONG)0x000006f5L)

/* MessageId  : 0x000006f6 */
/* Approx. msg: RPC_X_BYTE_COUNT_TOO_SMALL - The byte count is too small. */
#define RPC_X_BYTE_COUNT_TOO_SMALL	((ULONG)0x000006f6L)

/* MessageId  : 0x000006f7 */
/* Approx. msg: RPC_X_BAD_STUB_DATA - The stub received bad data. */
#define RPC_X_BAD_STUB_DATA	((ULONG)0x000006f7L)

/* MessageId  : 0x000006f8 */
/* Approx. msg: ERROR_INVALID_USER_BUFFER - The supplied user buffer is not valid for the requested operation. */
#define ERROR_INVALID_USER_BUFFER	((ULONG)0x000006f8L)

/* MessageId  : 0x000006f9 */
/* Approx. msg: ERROR_UNRECOGNIZED_MEDIA - The disk media is not recognized. It may not be formatted. */
#define ERROR_UNRECOGNIZED_MEDIA	((ULONG)0x000006f9L)

/* MessageId  : 0x000006fa */
/* Approx. msg: ERROR_NO_TRUST_LSA_SECRET - The workstation does not have a trust secret. */
#define ERROR_NO_TRUST_LSA_SECRET	((ULONG)0x000006faL)

/* MessageId  : 0x000006fb */
/* Approx. msg: ERROR_NO_TRUST_SAM_ACCOUNT - The security database on the server does not have a computer account for this workstation trust relationship. */
#define ERROR_NO_TRUST_SAM_ACCOUNT	((ULONG)0x000006fbL)

/* MessageId  : 0x000006fc */
/* Approx. msg: ERROR_TRUSTED_DOMAIN_FAILURE - The trust relationship between the primary domain and the trusted domain failed. */
#define ERROR_TRUSTED_DOMAIN_FAILURE	((ULONG)0x000006fcL)

/* MessageId  : 0x000006fd */
/* Approx. msg: ERROR_TRUSTED_RELATIONSHIP_FAILURE - The trust relationship between this workstation and the primary domain failed. */
#define ERROR_TRUSTED_RELATIONSHIP_FAILURE	((ULONG)0x000006fdL)

/* MessageId  : 0x000006fe */
/* Approx. msg: ERROR_TRUST_FAILURE - The network logon failed. */
#define ERROR_TRUST_FAILURE	((ULONG)0x000006feL)

/* MessageId  : 0x000006ff */
/* Approx. msg: RPC_S_CALL_IN_PROGRESS - A remote procedure call is already in progress for this thread. */
#define RPC_S_CALL_IN_PROGRESS	((ULONG)0x000006ffL)

/* MessageId  : 0x00000700 */
/* Approx. msg: ERROR_NETLOGON_NOT_STARTED - An attempt was made to logon, but the network logon service was not started. */
#define ERROR_NETLOGON_NOT_STARTED	((ULONG)0x00000700L)

/* MessageId  : 0x00000701 */
/* Approx. msg: ERROR_ACCOUNT_EXPIRED - The user's account has expired. */
#define ERROR_ACCOUNT_EXPIRED	((ULONG)0x00000701L)

/* MessageId  : 0x00000702 */
/* Approx. msg: ERROR_REDIRECTOR_HAS_OPEN_HANDLES - The redirector is in use and cannot be unloaded. */
#define ERROR_REDIRECTOR_HAS_OPEN_HANDLES	((ULONG)0x00000702L)

/* MessageId  : 0x00000703 */
/* Approx. msg: ERROR_PRINTER_DRIVER_ALREADY_INSTALLED - The specified printer driver is already installed. */
#define ERROR_PRINTER_DRIVER_ALREADY_INSTALLED	((ULONG)0x00000703L)

/* MessageId  : 0x00000704 */
/* Approx. msg: ERROR_UNKNOWN_PORT - The specified port is unknown. */
#define ERROR_UNKNOWN_PORT	((ULONG)0x00000704L)

/* MessageId  : 0x00000705 */
/* Approx. msg: ERROR_UNKNOWN_PRINTER_DRIVER - The printer driver is unknown. */
#define ERROR_UNKNOWN_PRINTER_DRIVER	((ULONG)0x00000705L)

/* MessageId  : 0x00000706 */
/* Approx. msg: ERROR_UNKNOWN_PRINTPROCESSOR - The print processor is unknown. */
#define ERROR_UNKNOWN_PRINTPROCESSOR	((ULONG)0x00000706L)

/* MessageId  : 0x00000707 */
/* Approx. msg: ERROR_INVALID_SEPARATOR_FILE - The specified separator file is invalid. */
#define ERROR_INVALID_SEPARATOR_FILE	((ULONG)0x00000707L)

/* MessageId  : 0x00000708 */
/* Approx. msg: ERROR_INVALID_PRIORITY - The specified priority is invalid. */
#define ERROR_INVALID_PRIORITY	((ULONG)0x00000708L)

/* MessageId  : 0x00000709 */
/* Approx. msg: ERROR_INVALID_PRINTER_NAME - The printer name is invalid. */
#define ERROR_INVALID_PRINTER_NAME	((ULONG)0x00000709L)

/* MessageId  : 0x0000070a */
/* Approx. msg: ERROR_PRINTER_ALREADY_EXISTS - The printer already exists. */
#define ERROR_PRINTER_ALREADY_EXISTS	((ULONG)0x0000070aL)

/* MessageId  : 0x0000070b */
/* Approx. msg: ERROR_INVALID_PRINTER_COMMAND - The printer command is invalid. */
#define ERROR_INVALID_PRINTER_COMMAND	((ULONG)0x0000070bL)

/* MessageId  : 0x0000070c */
/* Approx. msg: ERROR_INVALID_DATATYPE - The specified datatype is invalid. */
#define ERROR_INVALID_DATATYPE	((ULONG)0x0000070cL)

/* MessageId  : 0x0000070d */
/* Approx. msg: ERROR_INVALID_ENVIRONMENT - The environment specified is invalid. */
#define ERROR_INVALID_ENVIRONMENT	((ULONG)0x0000070dL)

/* MessageId  : 0x0000070e */
/* Approx. msg: RPC_S_NO_MORE_BINDINGS - There are no more bindings. */
#define RPC_S_NO_MORE_BINDINGS	((ULONG)0x0000070eL)

/* MessageId  : 0x0000070f */
/* Approx. msg: ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT - The account used is an interdomain trust account. Use your global user account or local user account to access this server. */
#define ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT	((ULONG)0x0000070fL)

/* MessageId  : 0x00000710 */
/* Approx. msg: ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT - The account used is a computer account. Use your global user account or local user account to access this server. */
#define ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT	((ULONG)0x00000710L)

/* MessageId  : 0x00000711 */
/* Approx. msg: ERROR_NOLOGON_SERVER_TRUST_ACCOUNT - The account used is a server trust account. Use your global user account or local user account to access this server. */
#define ERROR_NOLOGON_SERVER_TRUST_ACCOUNT	((ULONG)0x00000711L)

/* MessageId  : 0x00000712 */
/* Approx. msg: ERROR_DOMAIN_TRUST_INCONSISTENT - The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain. */
#define ERROR_DOMAIN_TRUST_INCONSISTENT	((ULONG)0x00000712L)

/* MessageId  : 0x00000713 */
/* Approx. msg: ERROR_SERVER_HAS_OPEN_HANDLES - The server is in use and cannot be unloaded. */
#define ERROR_SERVER_HAS_OPEN_HANDLES	((ULONG)0x00000713L)

/* MessageId  : 0x00000714 */
/* Approx. msg: ERROR_RESOURCE_DATA_NOT_FOUND - The specified image file did not contain a resource section. */
#define ERROR_RESOURCE_DATA_NOT_FOUND	((ULONG)0x00000714L)

/* MessageId  : 0x00000715 */
/* Approx. msg: ERROR_RESOURCE_TYPE_NOT_FOUND - The specified resource type cannot be found in the image file. */
#define ERROR_RESOURCE_TYPE_NOT_FOUND	((ULONG)0x00000715L)

/* MessageId  : 0x00000716 */
/* Approx. msg: ERROR_RESOURCE_NAME_NOT_FOUND - The specified resource name cannot be found in the image file. */
#define ERROR_RESOURCE_NAME_NOT_FOUND	((ULONG)0x00000716L)

/* MessageId  : 0x00000717 */
/* Approx. msg: ERROR_RESOURCE_LANG_NOT_FOUND - The specified resource language ID cannot be found in the image file. */
#define ERROR_RESOURCE_LANG_NOT_FOUND	((ULONG)0x00000717L)

/* MessageId  : 0x00000718 */
/* Approx. msg: ERROR_NOT_ENOUGH_QUOTA - Not enough quota is available to process this command. */
#define ERROR_NOT_ENOUGH_QUOTA	((ULONG)0x00000718L)

/* MessageId  : 0x00000719 */
/* Approx. msg: RPC_S_NO_INTERFACES - No interfaces have been registered. */
#define RPC_S_NO_INTERFACES	((ULONG)0x00000719L)

/* MessageId  : 0x0000071a */
/* Approx. msg: RPC_S_CALL_CANCELLED - The remote procedure call was cancelled. */
#define RPC_S_CALL_CANCELLED	((ULONG)0x0000071aL)

/* MessageId  : 0x0000071b */
/* Approx. msg: RPC_S_BINDING_INCOMPLETE - The binding handle does not contain all required information. */
#define RPC_S_BINDING_INCOMPLETE	((ULONG)0x0000071bL)

/* MessageId  : 0x0000071c */
/* Approx. msg: RPC_S_COMM_FAILURE - A communications failure occurred during a remote procedure call. */
#define RPC_S_COMM_FAILURE	((ULONG)0x0000071cL)

/* MessageId  : 0x0000071d */
/* Approx. msg: RPC_S_UNSUPPORTED_AUTHN_LEVEL - The requested authentication level is not supported. */
#define RPC_S_UNSUPPORTED_AUTHN_LEVEL	((ULONG)0x0000071dL)

/* MessageId  : 0x0000071e */
/* Approx. msg: RPC_S_NO_PRINC_NAME - No principal name registered. */
#define RPC_S_NO_PRINC_NAME	((ULONG)0x0000071eL)

/* MessageId  : 0x0000071f */
/* Approx. msg: RPC_S_NOT_RPC_ERROR - The error specified is not a valid Windows RPC error code. */
#define RPC_S_NOT_RPC_ERROR	((ULONG)0x0000071fL)

/* MessageId  : 0x00000720 */
/* Approx. msg: RPC_S_UUID_LOCAL_ONLY - A UUID that is valid only on this computer has been allocated. */
#define RPC_S_UUID_LOCAL_ONLY	((ULONG)0x00000720L)

/* MessageId  : 0x00000721 */
/* Approx. msg: RPC_S_SEC_PKG_ERROR - A security package specific error occurred. */
#define RPC_S_SEC_PKG_ERROR	((ULONG)0x00000721L)

/* MessageId  : 0x00000722 */
/* Approx. msg: RPC_S_NOT_CANCELLED - Thread is not canceled. */
#define RPC_S_NOT_CANCELLED	((ULONG)0x00000722L)

/* MessageId  : 0x00000723 */
/* Approx. msg: RPC_X_INVALID_ES_ACTION - Invalid operation on the encoding/decoding handle. */
#define RPC_X_INVALID_ES_ACTION	((ULONG)0x00000723L)

/* MessageId  : 0x00000724 */
/* Approx. msg: RPC_X_WRONG_ES_VERSION - Incompatible version of the serializing package. */
#define RPC_X_WRONG_ES_VERSION	((ULONG)0x00000724L)

/* MessageId  : 0x00000725 */
/* Approx. msg: RPC_X_WRONG_STUB_VERSION - Incompatible version of the RPC stub. */
#define RPC_X_WRONG_STUB_VERSION	((ULONG)0x00000725L)

/* MessageId  : 0x00000726 */
/* Approx. msg: RPC_X_INVALID_PIPE_OBJECT - The RPC pipe object is invalid or corrupted. */
#define RPC_X_INVALID_PIPE_OBJECT	((ULONG)0x00000726L)

/* MessageId  : 0x00000727 */
/* Approx. msg: RPC_X_WRONG_PIPE_ORDER - An invalid operation was attempted on an RPC pipe object. */
#define RPC_X_WRONG_PIPE_ORDER	((ULONG)0x00000727L)

/* MessageId  : 0x00000728 */
/* Approx. msg: RPC_X_WRONG_PIPE_VERSION - Unsupported RPC pipe version. */
#define RPC_X_WRONG_PIPE_VERSION	((ULONG)0x00000728L)

/* MessageId  : 0x0000076a */
/* Approx. msg: RPC_S_GROUP_MEMBER_NOT_FOUND - The group member was not found. */
#define RPC_S_GROUP_MEMBER_NOT_FOUND	((ULONG)0x0000076aL)

/* MessageId  : 0x0000076b */
/* Approx. msg: EPT_S_CANT_CREATE - The endpoint mapper database entry could not be created. */
#define EPT_S_CANT_CREATE	((ULONG)0x0000076bL)

/* MessageId  : 0x0000076c */
/* Approx. msg: RPC_S_INVALID_OBJECT - The object universal unique identifier (UUID) is the nil UUID. */
#define RPC_S_INVALID_OBJECT	((ULONG)0x0000076cL)

/* MessageId  : 0x0000076d */
/* Approx. msg: ERROR_INVALID_TIME - The specified time is invalid. */
#define ERROR_INVALID_TIME	((ULONG)0x0000076dL)

/* MessageId  : 0x0000076e */
/* Approx. msg: ERROR_INVALID_FORM_NAME - The specified form name is invalid. */
#define ERROR_INVALID_FORM_NAME	((ULONG)0x0000076eL)

/* MessageId  : 0x0000076f */
/* Approx. msg: ERROR_INVALID_FORM_SIZE - The specified form size is invalid. */
#define ERROR_INVALID_FORM_SIZE	((ULONG)0x0000076fL)

/* MessageId  : 0x00000770 */
/* Approx. msg: ERROR_ALREADY_WAITING - The specified printer handle is already being waited on */
#define ERROR_ALREADY_WAITING	((ULONG)0x00000770L)

/* MessageId  : 0x00000771 */
/* Approx. msg: ERROR_PRINTER_DELETED - The specified printer has been deleted. */
#define ERROR_PRINTER_DELETED	((ULONG)0x00000771L)

/* MessageId  : 0x00000772 */
/* Approx. msg: ERROR_INVALID_PRINTER_STATE - The state of the printer is invalid. */
#define ERROR_INVALID_PRINTER_STATE	((ULONG)0x00000772L)

/* MessageId  : 0x00000773 */
/* Approx. msg: ERROR_PASSWORD_MUST_CHANGE - The user's password must be changed before logging on the first time. */
#define ERROR_PASSWORD_MUST_CHANGE	((ULONG)0x00000773L)

/* MessageId  : 0x00000774 */
/* Approx. msg: ERROR_DOMAIN_CONTROLLER_NOT_FOUND - Could not find the domain controller for this domain. */
#define ERROR_DOMAIN_CONTROLLER_NOT_FOUND	((ULONG)0x00000774L)

/* MessageId  : 0x00000775 */
/* Approx. msg: ERROR_ACCOUNT_LOCKED_OUT - The referenced account is currently locked out and may not be used to log on. */
#define ERROR_ACCOUNT_LOCKED_OUT	((ULONG)0x00000775L)

/* MessageId  : 0x00000776 */
/* Approx. msg: OR_INVALID_OXID - The object exporter specified was not found. */
#define OR_INVALID_OXID	((ULONG)0x00000776L)

/* MessageId  : 0x00000777 */
/* Approx. msg: OR_INVALID_OID - The object specified was not found. */
#define OR_INVALID_OID	((ULONG)0x00000777L)

/* MessageId  : 0x00000778 */
/* Approx. msg: OR_INVALID_SET - The object resolver set specified was not found. */
#define OR_INVALID_SET	((ULONG)0x00000778L)

/* MessageId  : 0x00000779 */
/* Approx. msg: RPC_S_SEND_INCOMPLETE - Some data remains to be sent in the request buffer. */
#define RPC_S_SEND_INCOMPLETE	((ULONG)0x00000779L)

/* MessageId  : 0x0000077a */
/* Approx. msg: RPC_S_INVALID_ASYNC_HANDLE - Invalid asynchronous remote procedure call handle. */
#define RPC_S_INVALID_ASYNC_HANDLE	((ULONG)0x0000077aL)

/* MessageId  : 0x0000077b */
/* Approx. msg: RPC_S_INVALID_ASYNC_CALL - Invalid asynchronous RPC call handle for this operation. */
#define RPC_S_INVALID_ASYNC_CALL	((ULONG)0x0000077bL)

/* MessageId  : 0x0000077c */
/* Approx. msg: RPC_X_PIPE_CLOSED - The RPC pipe object has already been closed. */
#define RPC_X_PIPE_CLOSED	((ULONG)0x0000077cL)

/* MessageId  : 0x0000077d */
/* Approx. msg: RPC_X_PIPE_DISCIPLINE_ERROR - The RPC call completed before all pipes were processed. */
#define RPC_X_PIPE_DISCIPLINE_ERROR	((ULONG)0x0000077dL)

/* MessageId  : 0x0000077e */
/* Approx. msg: RPC_X_PIPE_EMPTY - No more data is available from the RPC pipe. */
#define RPC_X_PIPE_EMPTY	((ULONG)0x0000077eL)

/* MessageId  : 0x0000077f */
/* Approx. msg: ERROR_NO_SITENAME - No site name is available for this machine. */
#define ERROR_NO_SITENAME	((ULONG)0x0000077fL)

/* MessageId  : 0x00000780 */
/* Approx. msg: ERROR_CANT_ACCESS_FILE - The file cannot be accessed by the system. */
#define ERROR_CANT_ACCESS_FILE	((ULONG)0x00000780L)

/* MessageId  : 0x00000781 */
/* Approx. msg: ERROR_CANT_RESOLVE_FILENAME - The name of the file cannot be resolved by the system. */
#define ERROR_CANT_RESOLVE_FILENAME	((ULONG)0x00000781L)

/* MessageId  : 0x00000782 */
/* Approx. msg: RPC_S_ENTRY_TYPE_MISMATCH - The entry is not of the expected type. */
#define RPC_S_ENTRY_TYPE_MISMATCH	((ULONG)0x00000782L)

/* MessageId  : 0x00000783 */
/* Approx. msg: RPC_S_NOT_ALL_OBJS_EXPORTED - Not all object UUIDs could be exported to the specified entry. */
#define RPC_S_NOT_ALL_OBJS_EXPORTED	((ULONG)0x00000783L)

/* MessageId  : 0x00000784 */
/* Approx. msg: RPC_S_INTERFACE_NOT_EXPORTED - Interface could not be exported to the specified entry. */
#define RPC_S_INTERFACE_NOT_EXPORTED	((ULONG)0x00000784L)

/* MessageId  : 0x00000785 */
/* Approx. msg: RPC_S_PROFILE_NOT_ADDED - The specified profile entry could not be added. */
#define RPC_S_PROFILE_NOT_ADDED	((ULONG)0x00000785L)

/* MessageId  : 0x00000786 */
/* Approx. msg: RPC_S_PRF_ELT_NOT_ADDED - The specified profile element could not be added. */
#define RPC_S_PRF_ELT_NOT_ADDED	((ULONG)0x00000786L)

/* MessageId  : 0x00000787 */
/* Approx. msg: RPC_S_PRF_ELT_NOT_REMOVED - The specified profile element could not be removed. */
#define RPC_S_PRF_ELT_NOT_REMOVED	((ULONG)0x00000787L)

/* MessageId  : 0x00000788 */
/* Approx. msg: RPC_S_GRP_ELT_NOT_ADDED - The group element could not be added. */
#define RPC_S_GRP_ELT_NOT_ADDED	((ULONG)0x00000788L)

/* MessageId  : 0x00000789 */
/* Approx. msg: RPC_S_GRP_ELT_NOT_REMOVED - The group element could not be removed. */
#define RPC_S_GRP_ELT_NOT_REMOVED	((ULONG)0x00000789L)

/* MessageId  : 0x0000078a */
/* Approx. msg: ERROR_KM_DRIVER_BLOCKED - The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers. */
#define ERROR_KM_DRIVER_BLOCKED	((ULONG)0x0000078aL)

/* MessageId  : 0x0000078b */
/* Approx. msg: ERROR_CONTEXT_EXPIRED - The context has expired and can no longer be used. */
#define ERROR_CONTEXT_EXPIRED	((ULONG)0x0000078bL)

/* MessageId  : 0x0000078c */
/* Approx. msg: ERROR_PER_USER_TRUST_QUOTA_EXCEEDED - The current user's delegated trust creation quota has been exceeded. */
#define ERROR_PER_USER_TRUST_QUOTA_EXCEEDED	((ULONG)0x0000078cL)

/* MessageId  : 0x0000078d */
/* Approx. msg: ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED - The total delegated trust creation quota has been exceeded. */
#define ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED	((ULONG)0x0000078dL)

/* MessageId  : 0x0000078e */
/* Approx. msg: ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED - The current user's delegated trust deletion quota has been exceeded. */
#define ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED	((ULONG)0x0000078eL)

/* MessageId  : 0x000007d0 */
/* Approx. msg: ERROR_INVALID_PIXEL_FORMAT - The pixel format is invalid. */
#define ERROR_INVALID_PIXEL_FORMAT	((ULONG)0x000007d0L)

/* MessageId  : 0x000007d1 */
/* Approx. msg: ERROR_BAD_DRIVER - The specified driver is invalid. */
#define ERROR_BAD_DRIVER	((ULONG)0x000007d1L)

/* MessageId  : 0x000007d2 */
/* Approx. msg: ERROR_INVALID_WINDOW_STYLE - The window style or class attribute is invalid for this operation. */
#define ERROR_INVALID_WINDOW_STYLE	((ULONG)0x000007d2L)

/* MessageId  : 0x000007d3 */
/* Approx. msg: ERROR_METAFILE_NOT_SUPPORTED - The requested metafile operation is not supported. */
#define ERROR_METAFILE_NOT_SUPPORTED	((ULONG)0x000007d3L)

/* MessageId  : 0x000007d4 */
/* Approx. msg: ERROR_TRANSFORM_NOT_SUPPORTED - The requested transformation operation is not supported. */
#define ERROR_TRANSFORM_NOT_SUPPORTED	((ULONG)0x000007d4L)

/* MessageId  : 0x000007d5 */
/* Approx. msg: ERROR_CLIPPING_NOT_SUPPORTED - The requested clipping operation is not supported. */
#define ERROR_CLIPPING_NOT_SUPPORTED	((ULONG)0x000007d5L)

/* MessageId  : 0x000007da */
/* Approx. msg: ERROR_INVALID_CMM - The specified color management module is invalid. */
#define ERROR_INVALID_CMM	((ULONG)0x000007daL)

/* MessageId  : 0x000007db */
/* Approx. msg: ERROR_INVALID_PROFILE - The specified color profile is invalid. */
#define ERROR_INVALID_PROFILE	((ULONG)0x000007dbL)

/* MessageId  : 0x000007dc */
/* Approx. msg: ERROR_TAG_NOT_FOUND - The specified tag was not found. */
#define ERROR_TAG_NOT_FOUND	((ULONG)0x000007dcL)

/* MessageId  : 0x000007dd */
/* Approx. msg: ERROR_TAG_NOT_PRESENT - A required tag is not present. */
#define ERROR_TAG_NOT_PRESENT	((ULONG)0x000007ddL)

/* MessageId  : 0x000007de */
/* Approx. msg: ERROR_DUPLICATE_TAG - The specified tag is already present. */
#define ERROR_DUPLICATE_TAG	((ULONG)0x000007deL)

/* MessageId  : 0x000007df */
/* Approx. msg: ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE - The specified color profile is not associated with any device. */
#define ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE	((ULONG)0x000007dfL)

/* MessageId  : 0x000007e0 */
/* Approx. msg: ERROR_PROFILE_NOT_FOUND - The specified color profile was not found. */
#define ERROR_PROFILE_NOT_FOUND	((ULONG)0x000007e0L)

/* MessageId  : 0x000007e1 */
/* Approx. msg: ERROR_INVALID_COLORSPACE - The specified color space is invalid. */
#define ERROR_INVALID_COLORSPACE	((ULONG)0x000007e1L)

/* MessageId  : 0x000007e2 */
/* Approx. msg: ERROR_ICM_NOT_ENABLED - Image Color Management is not enabled. */
#define ERROR_ICM_NOT_ENABLED	((ULONG)0x000007e2L)

/* MessageId  : 0x000007e3 */
/* Approx. msg: ERROR_DELETING_ICM_XFORM - There was an error while deleting the color transform. */
#define ERROR_DELETING_ICM_XFORM	((ULONG)0x000007e3L)

/* MessageId  : 0x000007e4 */
/* Approx. msg: ERROR_INVALID_TRANSFORM - The specified color transform is invalid. */
#define ERROR_INVALID_TRANSFORM	((ULONG)0x000007e4L)

/* MessageId  : 0x000007e5 */
/* Approx. msg: ERROR_COLORSPACE_MISMATCH - The specified transform does not match the bitmap's color space. */
#define ERROR_COLORSPACE_MISMATCH	((ULONG)0x000007e5L)

/* MessageId  : 0x000007e6 */
/* Approx. msg: ERROR_INVALID_COLORINDEX - The specified named color index is not present in the profile. */
#define ERROR_INVALID_COLORINDEX	((ULONG)0x000007e6L)

/* MessageId  : 0x0000083c */
/* Approx. msg: ERROR_CONNECTED_OTHER_PASSWORD - The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified. */
#define ERROR_CONNECTED_OTHER_PASSWORD	((ULONG)0x0000083cL)

/* MessageId  : 0x0000083d */
/* Approx. msg: ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT - The network connection was made successfully using default credentials. */
#define ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT	((ULONG)0x0000083dL)

/* MessageId  : 0x0000089a */
/* Approx. msg: ERROR_BAD_USERNAME - The specified username is invalid. */
#define ERROR_BAD_USERNAME	((ULONG)0x0000089aL)

/* MessageId  : 0x000008ca */
/* Approx. msg: ERROR_NOT_CONNECTED - This network connection does not exist. */
#define ERROR_NOT_CONNECTED	((ULONG)0x000008caL)

/* MessageId  : 0x00000961 */
/* Approx. msg: ERROR_OPEN_FILES - This network connection has files open or requests pending. */
#define ERROR_OPEN_FILES	((ULONG)0x00000961L)

/* MessageId  : 0x00000962 */
/* Approx. msg: ERROR_ACTIVE_CONNECTIONS - Active connections still exist. */
#define ERROR_ACTIVE_CONNECTIONS	((ULONG)0x00000962L)

/* MessageId  : 0x00000964 */
/* Approx. msg: ERROR_DEVICE_IN_USE - The device is in use by an active process and cannot be disconnected. */
#define ERROR_DEVICE_IN_USE	((ULONG)0x00000964L)

/* MessageId  : 0x00000bb8 */
/* Approx. msg: ERROR_UNKNOWN_PRINT_MONITOR - The specified print monitor is unknown. */
#define ERROR_UNKNOWN_PRINT_MONITOR	((ULONG)0x00000bb8L)

/* MessageId  : 0x00000bb9 */
/* Approx. msg: ERROR_PRINTER_DRIVER_IN_USE - The specified printer driver is currently in use. */
#define ERROR_PRINTER_DRIVER_IN_USE	((ULONG)0x00000bb9L)

/* MessageId  : 0x00000bba */
/* Approx. msg: ERROR_SPOOL_FILE_NOT_FOUND - The spool file was not found. */
#define ERROR_SPOOL_FILE_NOT_FOUND	((ULONG)0x00000bbaL)

/* MessageId  : 0x00000bbb */
/* Approx. msg: ERROR_SPL_NO_STARTDOC - A StartDocPrinter call was not issued. */
#define ERROR_SPL_NO_STARTDOC	((ULONG)0x00000bbbL)

/* MessageId  : 0x00000bbc */
/* Approx. msg: ERROR_SPL_NO_ADDJOB - An AddJob call was not issued. */
#define ERROR_SPL_NO_ADDJOB	((ULONG)0x00000bbcL)

/* MessageId  : 0x00000bbd */
/* Approx. msg: ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED - The specified print processor has already been installed. */
#define ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED	((ULONG)0x00000bbdL)

/* MessageId  : 0x00000bbe */
/* Approx. msg: ERROR_PRINT_MONITOR_ALREADY_INSTALLED - The specified print monitor has already been installed. */
#define ERROR_PRINT_MONITOR_ALREADY_INSTALLED	((ULONG)0x00000bbeL)

/* MessageId  : 0x00000bbf */
/* Approx. msg: ERROR_INVALID_PRINT_MONITOR - The specified print monitor does not have the required functions. */
#define ERROR_INVALID_PRINT_MONITOR	((ULONG)0x00000bbfL)

/* MessageId  : 0x00000bc0 */
/* Approx. msg: ERROR_PRINT_MONITOR_IN_USE - The specified print monitor is currently in use. */
#define ERROR_PRINT_MONITOR_IN_USE	((ULONG)0x00000bc0L)

/* MessageId  : 0x00000bc1 */
/* Approx. msg: ERROR_PRINTER_HAS_JOBS_QUEUED - The requested operation is not allowed when there are jobs queued to the printer. */
#define ERROR_PRINTER_HAS_JOBS_QUEUED	((ULONG)0x00000bc1L)

/* MessageId  : 0x00000bc2 */
/* Approx. msg: ERROR_SUCCESS_REBOOT_REQUIRED - The requested operation is successful. Changes will not be effective until the system is rebooted. */
#define ERROR_SUCCESS_REBOOT_REQUIRED	((ULONG)0x00000bc2L)

/* MessageId  : 0x00000bc3 */
/* Approx. msg: ERROR_SUCCESS_RESTART_REQUIRED - The requested operation is successful. Changes will not be effective until the service is restarted. */
#define ERROR_SUCCESS_RESTART_REQUIRED	((ULONG)0x00000bc3L)

/* MessageId  : 0x00000bc4 */
/* Approx. msg: ERROR_PRINTER_NOT_FOUND - No printers were found. */
#define ERROR_PRINTER_NOT_FOUND	((ULONG)0x00000bc4L)

/* MessageId  : 0x00000bc5 */
/* Approx. msg: ERROR_PRINTER_DRIVER_WARNED - The printer driver is known to be unreliable. */
#define ERROR_PRINTER_DRIVER_WARNED	((ULONG)0x00000bc5L)

/* MessageId  : 0x00000bc6 */
/* Approx. msg: ERROR_PRINTER_DRIVER_BLOCKED - The printer driver is known to harm the system. */
#define ERROR_PRINTER_DRIVER_BLOCKED	((ULONG)0x00000bc6L)

/* MessageId  : 0x00000c1c */
/* Approx. msg: ERROR_XML_UNDEFINED_ENTITY - The XML contains an entity reference to an undefined entity. */
#define ERROR_XML_UNDEFINED_ENTITY	((ULONG)0x00000c1cL)

/* MessageId  : 0x00000c1d */
/* Approx. msg: ERROR_XML_MALFORMED_ENTITY - The XML contains a malformed entity reference. */
#define ERROR_XML_MALFORMED_ENTITY	((ULONG)0x00000c1dL)

/* MessageId  : 0x00000c1e */
/* Approx. msg: ERROR_XML_CHAR_NOT_IN_RANGE - The XML contains a character which is not permitted in XML. */
#define ERROR_XML_CHAR_NOT_IN_RANGE	((ULONG)0x00000c1eL)

/* MessageId  : 0x00000c80 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_EXTERNAL_PROXY - The manifest contained a duplicate definition for external proxy stub %1 at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_EXTERNAL_PROXY	((ULONG)0x00000c80L)

/* MessageId  : 0x00000c81 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_ASSEMBLY_REFERENCE - The manifest already contains a reference to %4 - a second reference was found at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_ASSEMBLY_REFERENCE	((ULONG)0x00000c81L)

/* MessageId  : 0x00000c82 */
/* Approx. msg: ERROR_PCM_COMPILER_INVALID_ASSEMBLY_REFERENCE - The assembly reference at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_INVALID_ASSEMBLY_REFERENCE	((ULONG)0x00000c82L)

/* MessageId  : 0x00000c83 */
/* Approx. msg: ERROR_PCM_COMPILER_INVALID_ASSEMBLY_DEFINITION - The assembly definition at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_INVALID_ASSEMBLY_DEFINITION	((ULONG)0x00000c83L)

/* MessageId  : 0x00000c84 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_WINDOW_CLASS - The manifest already contained the window class %4, found a second declaration at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_WINDOW_CLASS	((ULONG)0x00000c84L)

/* MessageId  : 0x00000c85 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_PROGID - The manifest already declared the progId %4, found a second declaration at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_PROGID	((ULONG)0x00000c85L)

/* MessageId  : 0x00000c86 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_NOINHERIT - Only one noInherit tag may be present in a manifest, found a second tag at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_NOINHERIT	((ULONG)0x00000c86L)

/* MessageId  : 0x00000c87 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_NOINHERITABLE - Only one noInheritable tag may be present in a manifest, found a second tag at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_NOINHERITABLE	((ULONG)0x00000c87L)

/* MessageId  : 0x00000c88 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_COM_CLASS - The manifest contained a duplicate declaration of COM class %4 at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_COM_CLASS	((ULONG)0x00000c88L)

/* MessageId  : 0x00000c89 */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_FILE_NAME - The manifest already declared the file %4, a second definition was found at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_FILE_NAME	((ULONG)0x00000c89L)

/* MessageId  : 0x00000c8a */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_CLR_SURROGATE - CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_DUPLICATE_CLR_SURROGATE	((ULONG)0x00000c8aL)

/* MessageId  : 0x00000c8b */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_TYPE_LIBRARY - Type library %1 was already defined, second definition at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_DUPLICATE_TYPE_LIBRARY	((ULONG)0x00000c8bL)

/* MessageId  : 0x00000c8c */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_PROXY_STUB - Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_DUPLICATE_PROXY_STUB	((ULONG)0x00000c8cL)

/* MessageId  : 0x00000c8d */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_CATEGORY_NAME - Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid. */
#define ERROR_PCM_COMPILER_DUPLICATE_CATEGORY_NAME	((ULONG)0x00000c8dL)

/* MessageId  : 0x00000c8e */
/* Approx. msg: ERROR_PCM_COMPILER_DUPLICATE_TOP_LEVEL_IDENTITY_FOUND - Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3) */
#define ERROR_PCM_COMPILER_DUPLICATE_TOP_LEVEL_IDENTITY_FOUND	((ULONG)0x00000c8eL)

/* MessageId  : 0x00000c8f */
/* Approx. msg: ERROR_PCM_COMPILER_UNKNOWN_ROOT_ELEMENT - The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version. */
#define ERROR_PCM_COMPILER_UNKNOWN_ROOT_ELEMENT	((ULONG)0x00000c8fL)

/* MessageId  : 0x00000c90 */
/* Approx. msg: ERROR_PCM_COMPILER_INVALID_ELEMENT - The element found at (%1:%2,%3) was not expected according to the manifest schema. */
#define ERROR_PCM_COMPILER_INVALID_ELEMENT	((ULONG)0x00000c90L)

/* MessageId  : 0x00000c91 */
/* Approx. msg: ERROR_PCM_COMPILER_MISSING_REQUIRED_ATTRIBUTE - The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information */
#define ERROR_PCM_COMPILER_MISSING_REQUIRED_ATTRIBUTE	((ULONG)0x00000c91L)

/* MessageId  : 0x00000c92 */
/* Approx. msg: ERROR_PCM_COMPILER_INVALID_ATTRIBUTE_VALUE - The attribute value %4 at (%1:%2,%3) was invalid according to the schema. */
#define ERROR_PCM_COMPILER_INVALID_ATTRIBUTE_VALUE	((ULONG)0x00000c92L)

/* MessageId  : 0x00000c93 */
/* Approx. msg: ERROR_PCM_COMPILER_UNEXPECTED_PCDATA - PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4. */
#define ERROR_PCM_COMPILER_UNEXPECTED_PCDATA	((ULONG)0x00000c93L)

/* MessageId  : 0x00000c94 */
/* Approx. msg: ERROR_PCM_DUPLICATE_STRING_TABLE_ENT - The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry. */
#define ERROR_PCM_DUPLICATE_STRING_TABLE_ENT	((ULONG)0x00000c94L)

/* MessageId  : 0x00000fa0 */
/* Approx. msg: ERROR_WINS_INTERNAL - WINS encountered an error while processing the command. */
#define ERROR_WINS_INTERNAL	((ULONG)0x00000fa0L)

/* MessageId  : 0x00000fa1 */
/* Approx. msg: ERROR_CAN_NOT_DEL_LOCAL_WINS - The local WINS cannot be deleted. */
#define ERROR_CAN_NOT_DEL_LOCAL_WINS	((ULONG)0x00000fa1L)

/* MessageId  : 0x00000fa2 */
/* Approx. msg: ERROR_STATIC_INIT - The importation from the file failed. */
#define ERROR_STATIC_INIT	((ULONG)0x00000fa2L)

/* MessageId  : 0x00000fa3 */
/* Approx. msg: ERROR_INC_BACKUP - The backup failed. Was a full backup done before? */
#define ERROR_INC_BACKUP	((ULONG)0x00000fa3L)

/* MessageId  : 0x00000fa4 */
/* Approx. msg: ERROR_FULL_BACKUP - The backup failed. Check the directory to which you are backing the database. */
#define ERROR_FULL_BACKUP	((ULONG)0x00000fa4L)

/* MessageId  : 0x00000fa5 */
/* Approx. msg: ERROR_REC_NON_EXISTENT - The name does not exist in the WINS database. */
#define ERROR_REC_NON_EXISTENT	((ULONG)0x00000fa5L)

/* MessageId  : 0x00000fa6 */
/* Approx. msg: ERROR_RPL_NOT_ALLOWED - Replication with a nonconfigured partner is not allowed. */
#define ERROR_RPL_NOT_ALLOWED	((ULONG)0x00000fa6L)

/* MessageId  : 0x00001004 */
/* Approx. msg: ERROR_DHCP_ADDRESS_CONFLICT - The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address. */
#define ERROR_DHCP_ADDRESS_CONFLICT	((ULONG)0x00001004L)

/* MessageId  : 0x00001068 */
/* Approx. msg: ERROR_WMI_GUID_NOT_FOUND - The GUID passed was not recognized as valid by a WMI data provider. */
#define ERROR_WMI_GUID_NOT_FOUND	((ULONG)0x00001068L)

/* MessageId  : 0x00001069 */
/* Approx. msg: ERROR_WMI_INSTANCE_NOT_FOUND - The instance name passed was not recognized as valid by a WMI data provider. */
#define ERROR_WMI_INSTANCE_NOT_FOUND	((ULONG)0x00001069L)

/* MessageId  : 0x0000106a */
/* Approx. msg: ERROR_WMI_ITEMID_NOT_FOUND - The data item ID passed was not recognized as valid by a WMI data provider. */
#define ERROR_WMI_ITEMID_NOT_FOUND	((ULONG)0x0000106aL)

/* MessageId  : 0x0000106b */
/* Approx. msg: ERROR_WMI_TRY_AGAIN - The WMI request could not be completed and should be retried. */
#define ERROR_WMI_TRY_AGAIN	((ULONG)0x0000106bL)

/* MessageId  : 0x0000106c */
/* Approx. msg: ERROR_WMI_DP_NOT_FOUND - The WMI data provider could not be located. */
#define ERROR_WMI_DP_NOT_FOUND	((ULONG)0x0000106cL)

/* MessageId  : 0x0000106d */
/* Approx. msg: ERROR_WMI_UNRESOLVED_INSTANCE_REF - The WMI data provider references an instance set that has not been registered. */
#define ERROR_WMI_UNRESOLVED_INSTANCE_REF	((ULONG)0x0000106dL)

/* MessageId  : 0x0000106e */
/* Approx. msg: ERROR_WMI_ALREADY_ENABLED - The WMI data block or event notification has already been enabled. */
#define ERROR_WMI_ALREADY_ENABLED	((ULONG)0x0000106eL)

/* MessageId  : 0x0000106f */
/* Approx. msg: ERROR_WMI_GUID_DISCONNECTED - The WMI data block is no longer available. */
#define ERROR_WMI_GUID_DISCONNECTED	((ULONG)0x0000106fL)

/* MessageId  : 0x00001070 */
/* Approx. msg: ERROR_WMI_SERVER_UNAVAILABLE - The WMI data service is not available. */
#define ERROR_WMI_SERVER_UNAVAILABLE	((ULONG)0x00001070L)

/* MessageId  : 0x00001071 */
/* Approx. msg: ERROR_WMI_DP_FAILED - The WMI data provider failed to carry out the request. */
#define ERROR_WMI_DP_FAILED	((ULONG)0x00001071L)

/* MessageId  : 0x00001072 */
/* Approx. msg: ERROR_WMI_INVALID_MOF - The WMI MOF information is not valid. */
#define ERROR_WMI_INVALID_MOF	((ULONG)0x00001072L)

/* MessageId  : 0x00001073 */
/* Approx. msg: ERROR_WMI_INVALID_REGINFO - The WMI registration information is not valid. */
#define ERROR_WMI_INVALID_REGINFO	((ULONG)0x00001073L)

/* MessageId  : 0x00001074 */
/* Approx. msg: ERROR_WMI_ALREADY_DISABLED - The WMI data block or event notification has already been disabled. */
#define ERROR_WMI_ALREADY_DISABLED	((ULONG)0x00001074L)

/* MessageId  : 0x00001075 */
/* Approx. msg: ERROR_WMI_READ_ONLY - The WMI data item or data block is read only. */
#define ERROR_WMI_READ_ONLY	((ULONG)0x00001075L)

/* MessageId  : 0x00001076 */
/* Approx. msg: ERROR_WMI_SET_FAILURE - The WMI data item or data block could not be changed. */
#define ERROR_WMI_SET_FAILURE	((ULONG)0x00001076L)

/* MessageId  : 0x000010cc */
/* Approx. msg: ERROR_INVALID_MEDIA - The media identifier does not represent a valid medium. */
#define ERROR_INVALID_MEDIA	((ULONG)0x000010ccL)

/* MessageId  : 0x000010cd */
/* Approx. msg: ERROR_INVALID_LIBRARY - The library identifier does not represent a valid library. */
#define ERROR_INVALID_LIBRARY	((ULONG)0x000010cdL)

/* MessageId  : 0x000010ce */
/* Approx. msg: ERROR_INVALID_MEDIA_POOL - The media pool identifier does not represent a valid media pool. */
#define ERROR_INVALID_MEDIA_POOL	((ULONG)0x000010ceL)

/* MessageId  : 0x000010cf */
/* Approx. msg: ERROR_DRIVE_MEDIA_MISMATCH - The drive and medium are not compatible or exist in different libraries. */
#define ERROR_DRIVE_MEDIA_MISMATCH	((ULONG)0x000010cfL)

/* MessageId  : 0x000010d0 */
/* Approx. msg: ERROR_MEDIA_OFFLINE - The medium currently exists in an offline library and must be online to perform this operation. */
#define ERROR_MEDIA_OFFLINE	((ULONG)0x000010d0L)

/* MessageId  : 0x000010d1 */
/* Approx. msg: ERROR_LIBRARY_OFFLINE - The operation cannot be performed on an offline library. */
#define ERROR_LIBRARY_OFFLINE	((ULONG)0x000010d1L)

/* MessageId  : 0x000010d2 */
/* Approx. msg: ERROR_EMPTY - The library, drive, or media pool is empty. */
#define ERROR_EMPTY	((ULONG)0x000010d2L)

/* MessageId  : 0x000010d3 */
/* Approx. msg: ERROR_NOT_EMPTY - The library, drive, or media pool must be empty to perform this operation. */
#define ERROR_NOT_EMPTY	((ULONG)0x000010d3L)

/* MessageId  : 0x000010d4 */
/* Approx. msg: ERROR_MEDIA_UNAVAILABLE - No media is currently available in this media pool or library. */
#define ERROR_MEDIA_UNAVAILABLE	((ULONG)0x000010d4L)

/* MessageId  : 0x000010d5 */
/* Approx. msg: ERROR_RESOURCE_DISABLED - A resource required for this operation is disabled. */
#define ERROR_RESOURCE_DISABLED	((ULONG)0x000010d5L)

/* MessageId  : 0x000010d6 */
/* Approx. msg: ERROR_INVALID_CLEANER - The media identifier does not represent a valid cleaner. */
#define ERROR_INVALID_CLEANER	((ULONG)0x000010d6L)

/* MessageId  : 0x000010d7 */
/* Approx. msg: ERROR_UNABLE_TO_CLEAN - The drive cannot be cleaned or does not support cleaning. */
#define ERROR_UNABLE_TO_CLEAN	((ULONG)0x000010d7L)

/* MessageId  : 0x000010d8 */
/* Approx. msg: ERROR_OBJECT_NOT_FOUND - The object identifier does not represent a valid object. */
#define ERROR_OBJECT_NOT_FOUND	((ULONG)0x000010d8L)

/* MessageId  : 0x000010d9 */
/* Approx. msg: ERROR_DATABASE_FAILURE - Unable to read from or write to the database. */
#define ERROR_DATABASE_FAILURE	((ULONG)0x000010d9L)

/* MessageId  : 0x000010da */
/* Approx. msg: ERROR_DATABASE_FULL - The database is full. */
#define ERROR_DATABASE_FULL	((ULONG)0x000010daL)

/* MessageId  : 0x000010db */
/* Approx. msg: ERROR_MEDIA_INCOMPATIBLE - The medium is not compatible with the device or media pool. */
#define ERROR_MEDIA_INCOMPATIBLE	((ULONG)0x000010dbL)

/* MessageId  : 0x000010dc */
/* Approx. msg: ERROR_RESOURCE_NOT_PRESENT - The resource required for this operation does not exist. */
#define ERROR_RESOURCE_NOT_PRESENT	((ULONG)0x000010dcL)

/* MessageId  : 0x000010dd */
/* Approx. msg: ERROR_INVALID_OPERATION - The operation identifier is not valid. */
#define ERROR_INVALID_OPERATION	((ULONG)0x000010ddL)

/* MessageId  : 0x000010de */
/* Approx. msg: ERROR_MEDIA_NOT_AVAILABLE - The media is not mounted or ready for use. */
#define ERROR_MEDIA_NOT_AVAILABLE	((ULONG)0x000010deL)

/* MessageId  : 0x000010df */
/* Approx. msg: ERROR_DEVICE_NOT_AVAILABLE - The device is not ready for use. */
#define ERROR_DEVICE_NOT_AVAILABLE	((ULONG)0x000010dfL)

/* MessageId  : 0x000010e0 */
/* Approx. msg: ERROR_REQUEST_REFUSED - The operator or administrator has refused the request. */
#define ERROR_REQUEST_REFUSED	((ULONG)0x000010e0L)

/* MessageId  : 0x000010e1 */
/* Approx. msg: ERROR_INVALID_DRIVE_OBJECT - The drive identifier does not represent a valid drive. */
#define ERROR_INVALID_DRIVE_OBJECT	((ULONG)0x000010e1L)

/* MessageId  : 0x000010e2 */
/* Approx. msg: ERROR_LIBRARY_FULL - Library is full. No slot is available for use. */
#define ERROR_LIBRARY_FULL	((ULONG)0x000010e2L)

/* MessageId  : 0x000010e3 */
/* Approx. msg: ERROR_MEDIUM_NOT_ACCESSIBLE - The transport cannot access the medium. */
#define ERROR_MEDIUM_NOT_ACCESSIBLE	((ULONG)0x000010e3L)

/* MessageId  : 0x000010e4 */
/* Approx. msg: ERROR_UNABLE_TO_LOAD_MEDIUM - Unable to load the medium into the drive. */
#define ERROR_UNABLE_TO_LOAD_MEDIUM	((ULONG)0x000010e4L)

/* MessageId  : 0x000010e5 */
/* Approx. msg: ERROR_UNABLE_TO_INVENTORY_DRIVE - Unable to retrieve status about the drive. */
#define ERROR_UNABLE_TO_INVENTORY_DRIVE	((ULONG)0x000010e5L)

/* MessageId  : 0x000010e6 */
/* Approx. msg: ERROR_UNABLE_TO_INVENTORY_SLOT - Unable to retrieve status about the slot. */
#define ERROR_UNABLE_TO_INVENTORY_SLOT	((ULONG)0x000010e6L)

/* MessageId  : 0x000010e7 */
/* Approx. msg: ERROR_UNABLE_TO_INVENTORY_TRANSPORT - Unable to retrieve status about the transport. */
#define ERROR_UNABLE_TO_INVENTORY_TRANSPORT	((ULONG)0x000010e7L)

/* MessageId  : 0x000010e8 */
/* Approx. msg: ERROR_TRANSPORT_FULL - Cannot use the transport because it is already in use. */
#define ERROR_TRANSPORT_FULL	((ULONG)0x000010e8L)

/* MessageId  : 0x000010e9 */
/* Approx. msg: ERROR_CONTROLLING_IEPORT - Unable to open or close the inject/eject port. */
#define ERROR_CONTROLLING_IEPORT	((ULONG)0x000010e9L)

/* MessageId  : 0x000010ea */
/* Approx. msg: ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA - Unable to eject the media because it is in a drive. */
#define ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA	((ULONG)0x000010eaL)

/* MessageId  : 0x000010eb */
/* Approx. msg: ERROR_CLEANER_SLOT_SET - A cleaner slot is already reserved. */
#define ERROR_CLEANER_SLOT_SET	((ULONG)0x000010ebL)

/* MessageId  : 0x000010ec */
/* Approx. msg: ERROR_CLEANER_SLOT_NOT_SET - A cleaner slot is not reserved. */
#define ERROR_CLEANER_SLOT_NOT_SET	((ULONG)0x000010ecL)

/* MessageId  : 0x000010ed */
/* Approx. msg: ERROR_CLEANER_CARTRIDGE_SPENT - The cleaner cartridge has performed the maximum number of drive cleanings. */
#define ERROR_CLEANER_CARTRIDGE_SPENT	((ULONG)0x000010edL)

/* MessageId  : 0x000010ee */
/* Approx. msg: ERROR_UNEXPECTED_OMID - Unexpected on-medium identifier. */
#define ERROR_UNEXPECTED_OMID	((ULONG)0x000010eeL)

/* MessageId  : 0x000010ef */
/* Approx. msg: ERROR_CANT_DELETE_LAST_ITEM - The last remaining item in this group or resource cannot be deleted. */
#define ERROR_CANT_DELETE_LAST_ITEM	((ULONG)0x000010efL)

/* MessageId  : 0x000010f0 */
/* Approx. msg: ERROR_MESSAGE_EXCEEDS_MAX_SIZE - The message provided exceeds the maximum size allowed for this parameter. */
#define ERROR_MESSAGE_EXCEEDS_MAX_SIZE	((ULONG)0x000010f0L)

/* MessageId  : 0x000010f1 */
/* Approx. msg: ERROR_VOLUME_CONTAINS_SYS_FILES - The volume contains system or paging files. */
#define ERROR_VOLUME_CONTAINS_SYS_FILES	((ULONG)0x000010f1L)

/* MessageId  : 0x000010f2 */
/* Approx. msg: ERROR_INDIGENOUS_TYPE - The media type cannot be removed from this library since at least one drive in the library reports it can support this media type. */
#define ERROR_INDIGENOUS_TYPE	((ULONG)0x000010f2L)

/* MessageId  : 0x000010f3 */
/* Approx. msg: ERROR_NO_SUPPORTING_DRIVES - This offline media cannot be mounted on this system since no enabled drives are present which can be used. */
#define ERROR_NO_SUPPORTING_DRIVES	((ULONG)0x000010f3L)

/* MessageId  : 0x000010f4 */
/* Approx. msg: ERROR_CLEANER_CARTRIDGE_INSTALLED - A cleaner cartridge is present in the tape library. */
#define ERROR_CLEANER_CARTRIDGE_INSTALLED	((ULONG)0x000010f4L)

/* MessageId  : 0x000010fe */
/* Approx. msg: ERROR_FILE_OFFLINE - The remote storage service was not able to recall the file. */
#define ERROR_FILE_OFFLINE	((ULONG)0x000010feL)

/* MessageId  : 0x000010ff */
/* Approx. msg: ERROR_REMOTE_STORAGE_NOT_ACTIVE - The remote storage service is not operational at this time. */
#define ERROR_REMOTE_STORAGE_NOT_ACTIVE	((ULONG)0x000010ffL)

/* MessageId  : 0x00001100 */
/* Approx. msg: ERROR_REMOTE_STORAGE_MEDIA_ERROR - The remote storage service encountered a media error. */
#define ERROR_REMOTE_STORAGE_MEDIA_ERROR	((ULONG)0x00001100L)

/* MessageId  : 0x00001126 */
/* Approx. msg: ERROR_NOT_A_REPARSE_POINT - The file or directory is not a reparse point. */
#define ERROR_NOT_A_REPARSE_POINT	((ULONG)0x00001126L)

/* MessageId  : 0x00001127 */
/* Approx. msg: ERROR_REPARSE_ATTRIBUTE_CONFLICT - The reparse point attribute cannot be set because it conflicts with an existing attribute. */
#define ERROR_REPARSE_ATTRIBUTE_CONFLICT	((ULONG)0x00001127L)

/* MessageId  : 0x00001128 */
/* Approx. msg: ERROR_INVALID_REPARSE_DATA - The data present in the reparse point buffer is invalid. */
#define ERROR_INVALID_REPARSE_DATA	((ULONG)0x00001128L)

/* MessageId  : 0x00001129 */
/* Approx. msg: ERROR_REPARSE_TAG_INVALID - The tag present in the reparse point buffer is invalid. */
#define ERROR_REPARSE_TAG_INVALID	((ULONG)0x00001129L)

/* MessageId  : 0x0000112a */
/* Approx. msg: ERROR_REPARSE_TAG_MISMATCH - There is a mismatch between the tag specified in the request and the tag present in the reparse point. */
#define ERROR_REPARSE_TAG_MISMATCH	((ULONG)0x0000112aL)

/* MessageId  : 0x00001194 */
/* Approx. msg: ERROR_VOLUME_NOT_SIS_ENABLED - Single Instance Storage is not available on this volume. */
#define ERROR_VOLUME_NOT_SIS_ENABLED	((ULONG)0x00001194L)

/* MessageId  : 0x00001389 */
/* Approx. msg: ERROR_DEPENDENT_RESOURCE_EXISTS - The cluster resource cannot be moved to another group because other resources are dependent on it. */
#define ERROR_DEPENDENT_RESOURCE_EXISTS	((ULONG)0x00001389L)

/* MessageId  : 0x0000138a */
/* Approx. msg: ERROR_DEPENDENCY_NOT_FOUND - The cluster resource dependency cannot be found. */
#define ERROR_DEPENDENCY_NOT_FOUND	((ULONG)0x0000138aL)

/* MessageId  : 0x0000138b */
/* Approx. msg: ERROR_DEPENDENCY_ALREADY_EXISTS - The cluster resource cannot be made dependent on the specified resource because it is already dependent. */
#define ERROR_DEPENDENCY_ALREADY_EXISTS	((ULONG)0x0000138bL)

/* MessageId  : 0x0000138c */
/* Approx. msg: ERROR_RESOURCE_NOT_ONLINE - The cluster resource is not online. */
#define ERROR_RESOURCE_NOT_ONLINE	((ULONG)0x0000138cL)

/* MessageId  : 0x0000138d */
/* Approx. msg: ERROR_HOST_NODE_NOT_AVAILABLE - A cluster node is not available for this operation. */
#define ERROR_HOST_NODE_NOT_AVAILABLE	((ULONG)0x0000138dL)

/* MessageId  : 0x0000138e */
/* Approx. msg: ERROR_RESOURCE_NOT_AVAILABLE - The cluster resource is not available. */
#define ERROR_RESOURCE_NOT_AVAILABLE	((ULONG)0x0000138eL)

/* MessageId  : 0x0000138f */
/* Approx. msg: ERROR_RESOURCE_NOT_FOUND - The cluster resource could not be found. */
#define ERROR_RESOURCE_NOT_FOUND	((ULONG)0x0000138fL)

/* MessageId  : 0x00001390 */
/* Approx. msg: ERROR_SHUTDOWN_CLUSTER - The cluster is being shut down. */
#define ERROR_SHUTDOWN_CLUSTER	((ULONG)0x00001390L)

/* MessageId  : 0x00001391 */
/* Approx. msg: ERROR_CANT_EVICT_ACTIVE_NODE - A cluster node cannot be evicted from the cluster unless the node is down. */
#define ERROR_CANT_EVICT_ACTIVE_NODE	((ULONG)0x00001391L)

/* MessageId  : 0x00001392 */
/* Approx. msg: ERROR_OBJECT_ALREADY_EXISTS - The object already exists. */
#define ERROR_OBJECT_ALREADY_EXISTS	((ULONG)0x00001392L)

/* MessageId  : 0x00001393 */
/* Approx. msg: ERROR_OBJECT_IN_LIST - The object is already in the list. */
#define ERROR_OBJECT_IN_LIST	((ULONG)0x00001393L)

/* MessageId  : 0x00001394 */
/* Approx. msg: ERROR_GROUP_NOT_AVAILABLE - The cluster group is not available for any new requests. */
#define ERROR_GROUP_NOT_AVAILABLE	((ULONG)0x00001394L)

/* MessageId  : 0x00001395 */
/* Approx. msg: ERROR_GROUP_NOT_FOUND - The cluster group could not be found. */
#define ERROR_GROUP_NOT_FOUND	((ULONG)0x00001395L)

/* MessageId  : 0x00001396 */
/* Approx. msg: ERROR_GROUP_NOT_ONLINE - The operation could not be completed because the cluster group is not online. */
#define ERROR_GROUP_NOT_ONLINE	((ULONG)0x00001396L)

/* MessageId  : 0x00001397 */
/* Approx. msg: ERROR_HOST_NODE_NOT_RESOURCE_OWNER - The cluster node is not the owner of the resource. */
#define ERROR_HOST_NODE_NOT_RESOURCE_OWNER	((ULONG)0x00001397L)

/* MessageId  : 0x00001398 */
/* Approx. msg: ERROR_HOST_NODE_NOT_GROUP_OWNER - The cluster node is not the owner of the group. */
#define ERROR_HOST_NODE_NOT_GROUP_OWNER	((ULONG)0x00001398L)

/* MessageId  : 0x00001399 */
/* Approx. msg: ERROR_RESMON_CREATE_FAILED - The cluster resource could not be created in the specified resource monitor. */
#define ERROR_RESMON_CREATE_FAILED	((ULONG)0x00001399L)

/* MessageId  : 0x0000139a */
/* Approx. msg: ERROR_RESMON_ONLINE_FAILED - The cluster resource could not be brought online by the resource monitor. */
#define ERROR_RESMON_ONLINE_FAILED	((ULONG)0x0000139aL)

/* MessageId  : 0x0000139b */
/* Approx. msg: ERROR_RESOURCE_ONLINE - The operation could not be completed because the cluster resource is online. */
#define ERROR_RESOURCE_ONLINE	((ULONG)0x0000139bL)

/* MessageId  : 0x0000139c */
/* Approx. msg: ERROR_QUORUM_RESOURCE - The cluster resource could not be deleted or brought offline because it is the quorum resource. */
#define ERROR_QUORUM_RESOURCE	((ULONG)0x0000139cL)

/* MessageId  : 0x0000139d */
/* Approx. msg: ERROR_NOT_QUORUM_CAPABLE - The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource. */
#define ERROR_NOT_QUORUM_CAPABLE	((ULONG)0x0000139dL)

/* MessageId  : 0x0000139e */
/* Approx. msg: ERROR_CLUSTER_SHUTTING_DOWN - The cluster software is shutting down. */
#define ERROR_CLUSTER_SHUTTING_DOWN	((ULONG)0x0000139eL)

/* MessageId  : 0x0000139f */
/* Approx. msg: ERROR_INVALID_STATE - The group or resource is not in the correct state to perform the requested operation. */
#define ERROR_INVALID_STATE	((ULONG)0x0000139fL)

/* MessageId  : 0x000013a0 */
/* Approx. msg: ERROR_RESOURCE_PROPERTIES_STORED - The properties were stored but not all changes will take effect until the next time the resource is brought online. */
#define ERROR_RESOURCE_PROPERTIES_STORED	((ULONG)0x000013a0L)

/* MessageId  : 0x000013a1 */
/* Approx. msg: ERROR_NOT_QUORUM_CLASS - The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class. */
#define ERROR_NOT_QUORUM_CLASS	((ULONG)0x000013a1L)

/* MessageId  : 0x000013a2 */
/* Approx. msg: ERROR_CORE_RESOURCE - The cluster resource could not be deleted since it is a core resource. */
#define ERROR_CORE_RESOURCE	((ULONG)0x000013a2L)

/* MessageId  : 0x000013a3 */
/* Approx. msg: ERROR_QUORUM_RESOURCE_ONLINE_FAILED - The quorum resource failed to come online. */
#define ERROR_QUORUM_RESOURCE_ONLINE_FAILED	((ULONG)0x000013a3L)

/* MessageId  : 0x000013a4 */
/* Approx. msg: ERROR_QUORUMLOG_OPEN_FAILED - The quorum log could not be created or mounted successfully. */
#define ERROR_QUORUMLOG_OPEN_FAILED	((ULONG)0x000013a4L)

/* MessageId  : 0x000013a5 */
/* Approx. msg: ERROR_CLUSTERLOG_CORRUPT - The cluster log is corrupt. */
#define ERROR_CLUSTERLOG_CORRUPT	((ULONG)0x000013a5L)

/* MessageId  : 0x000013a6 */
/* Approx. msg: ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE - The record could not be written to the cluster log since it exceeds the maximum size. */
#define ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE	((ULONG)0x000013a6L)

/* MessageId  : 0x000013a7 */
/* Approx. msg: ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE - The cluster log exceeds its maximum size. */
#define ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE	((ULONG)0x000013a7L)

/* MessageId  : 0x000013a8 */
/* Approx. msg: ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND - No checkpoint record was found in the cluster log. */
#define ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND	((ULONG)0x000013a8L)

/* MessageId  : 0x000013a9 */
/* Approx. msg: ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE - The minimum required disk space needed for logging is not available. */
#define ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE	((ULONG)0x000013a9L)

/* MessageId  : 0x000013aa */
/* Approx. msg: ERROR_QUORUM_OWNER_ALIVE - The cluster node failed to take control of the quorum resource because the resource is owned by another active node. */
#define ERROR_QUORUM_OWNER_ALIVE	((ULONG)0x000013aaL)

/* MessageId  : 0x000013ab */
/* Approx. msg: ERROR_NETWORK_NOT_AVAILABLE - A cluster network is not available for this operation. */
#define ERROR_NETWORK_NOT_AVAILABLE	((ULONG)0x000013abL)

/* MessageId  : 0x000013ac */
/* Approx. msg: ERROR_NODE_NOT_AVAILABLE - A cluster node is not available for this operation. */
#define ERROR_NODE_NOT_AVAILABLE	((ULONG)0x000013acL)

/* MessageId  : 0x000013ad */
/* Approx. msg: ERROR_ALL_NODES_NOT_AVAILABLE - All cluster nodes must be running to perform this operation. */
#define ERROR_ALL_NODES_NOT_AVAILABLE	((ULONG)0x000013adL)

/* MessageId  : 0x000013ae */
/* Approx. msg: ERROR_RESOURCE_FAILED - A cluster resource failed. */
#define ERROR_RESOURCE_FAILED	((ULONG)0x000013aeL)

/* MessageId  : 0x000013af */
/* Approx. msg: ERROR_CLUSTER_INVALID_NODE - The cluster node is not valid. */
#define ERROR_CLUSTER_INVALID_NODE	((ULONG)0x000013afL)

/* MessageId  : 0x000013b0 */
/* Approx. msg: ERROR_CLUSTER_NODE_EXISTS - The cluster node already exists. */
#define ERROR_CLUSTER_NODE_EXISTS	((ULONG)0x000013b0L)

/* MessageId  : 0x000013b1 */
/* Approx. msg: ERROR_CLUSTER_JOIN_IN_PROGRESS - A node is in the process of joining the cluster. */
#define ERROR_CLUSTER_JOIN_IN_PROGRESS	((ULONG)0x000013b1L)

/* MessageId  : 0x000013b2 */
/* Approx. msg: ERROR_CLUSTER_NODE_NOT_FOUND - The cluster node was not found. */
#define ERROR_CLUSTER_NODE_NOT_FOUND	((ULONG)0x000013b2L)

/* MessageId  : 0x000013b3 */
/* Approx. msg: ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND - The cluster local node information was not found. */
#define ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND	((ULONG)0x000013b3L)

/* MessageId  : 0x000013b4 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_EXISTS - The cluster network already exists. */
#define ERROR_CLUSTER_NETWORK_EXISTS	((ULONG)0x000013b4L)

/* MessageId  : 0x000013b5 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_NOT_FOUND - The cluster network was not found. */
#define ERROR_CLUSTER_NETWORK_NOT_FOUND	((ULONG)0x000013b5L)

/* MessageId  : 0x000013b6 */
/* Approx. msg: ERROR_CLUSTER_NETINTERFACE_EXISTS - The cluster network interface already exists. */
#define ERROR_CLUSTER_NETINTERFACE_EXISTS	((ULONG)0x000013b6L)

/* MessageId  : 0x000013b7 */
/* Approx. msg: ERROR_CLUSTER_NETINTERFACE_NOT_FOUND - The cluster network interface was not found. */
#define ERROR_CLUSTER_NETINTERFACE_NOT_FOUND	((ULONG)0x000013b7L)

/* MessageId  : 0x000013b8 */
/* Approx. msg: ERROR_CLUSTER_INVALID_REQUEST - The cluster request is not valid for this object. */
#define ERROR_CLUSTER_INVALID_REQUEST	((ULONG)0x000013b8L)

/* MessageId  : 0x000013b9 */
/* Approx. msg: ERROR_CLUSTER_INVALID_NETWORK_PROVIDER - The cluster network provider is not valid. */
#define ERROR_CLUSTER_INVALID_NETWORK_PROVIDER	((ULONG)0x000013b9L)

/* MessageId  : 0x000013ba */
/* Approx. msg: ERROR_CLUSTER_NODE_DOWN - The cluster node is down. */
#define ERROR_CLUSTER_NODE_DOWN	((ULONG)0x000013baL)

/* MessageId  : 0x000013bb */
/* Approx. msg: ERROR_CLUSTER_NODE_UNREACHABLE - The cluster node is not reachable. */
#define ERROR_CLUSTER_NODE_UNREACHABLE	((ULONG)0x000013bbL)

/* MessageId  : 0x000013bc */
/* Approx. msg: ERROR_CLUSTER_NODE_NOT_MEMBER - The cluster node is not a member of the cluster. */
#define ERROR_CLUSTER_NODE_NOT_MEMBER	((ULONG)0x000013bcL)

/* MessageId  : 0x000013bd */
/* Approx. msg: ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS - A cluster join operation is not in progress. */
#define ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS	((ULONG)0x000013bdL)

/* MessageId  : 0x000013be */
/* Approx. msg: ERROR_CLUSTER_INVALID_NETWORK - The cluster network is not valid. */
#define ERROR_CLUSTER_INVALID_NETWORK	((ULONG)0x000013beL)

/* MessageId  : 0x000013c0 */
/* Approx. msg: ERROR_CLUSTER_NODE_UP - The cluster node is up. */
#define ERROR_CLUSTER_NODE_UP	((ULONG)0x000013c0L)

/* MessageId  : 0x000013c1 */
/* Approx. msg: ERROR_CLUSTER_IPADDR_IN_USE - The cluster IP address is already in use. */
#define ERROR_CLUSTER_IPADDR_IN_USE	((ULONG)0x000013c1L)

/* MessageId  : 0x000013c2 */
/* Approx. msg: ERROR_CLUSTER_NODE_NOT_PAUSED - The cluster node is not paused. */
#define ERROR_CLUSTER_NODE_NOT_PAUSED	((ULONG)0x000013c2L)

/* MessageId  : 0x000013c3 */
/* Approx. msg: ERROR_CLUSTER_NO_SECURITY_CONTEXT - No cluster security context is available. */
#define ERROR_CLUSTER_NO_SECURITY_CONTEXT	((ULONG)0x000013c3L)

/* MessageId  : 0x000013c4 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_NOT_INTERNAL - The cluster network is not configured for internal cluster communication. */
#define ERROR_CLUSTER_NETWORK_NOT_INTERNAL	((ULONG)0x000013c4L)

/* MessageId  : 0x000013c5 */
/* Approx. msg: ERROR_CLUSTER_NODE_ALREADY_UP - The cluster node is already up. */
#define ERROR_CLUSTER_NODE_ALREADY_UP	((ULONG)0x000013c5L)

/* MessageId  : 0x000013c6 */
/* Approx. msg: ERROR_CLUSTER_NODE_ALREADY_DOWN - The cluster node is already down. */
#define ERROR_CLUSTER_NODE_ALREADY_DOWN	((ULONG)0x000013c6L)

/* MessageId  : 0x000013c7 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_ALREADY_ONLINE - The cluster network is already online. */
#define ERROR_CLUSTER_NETWORK_ALREADY_ONLINE	((ULONG)0x000013c7L)

/* MessageId  : 0x000013c8 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE - The cluster network is already offline. */
#define ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE	((ULONG)0x000013c8L)

/* MessageId  : 0x000013c9 */
/* Approx. msg: ERROR_CLUSTER_NODE_ALREADY_MEMBER - The cluster node is already a member of the cluster. */
#define ERROR_CLUSTER_NODE_ALREADY_MEMBER	((ULONG)0x000013c9L)

/* MessageId  : 0x000013ca */
/* Approx. msg: ERROR_CLUSTER_LAST_INTERNAL_NETWORK - The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network. */
#define ERROR_CLUSTER_LAST_INTERNAL_NETWORK	((ULONG)0x000013caL)

/* MessageId  : 0x000013cb */
/* Approx. msg: ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS - One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network. */
#define ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS	((ULONG)0x000013cbL)

/* MessageId  : 0x000013cc */
/* Approx. msg: ERROR_INVALID_OPERATION_ON_QUORUM - This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list. */
#define ERROR_INVALID_OPERATION_ON_QUORUM	((ULONG)0x000013ccL)

/* MessageId  : 0x000013cd */
/* Approx. msg: ERROR_DEPENDENCY_NOT_ALLOWED - The cluster quorum resource is not allowed to have any dependencies. */
#define ERROR_DEPENDENCY_NOT_ALLOWED	((ULONG)0x000013cdL)

/* MessageId  : 0x000013ce */
/* Approx. msg: ERROR_CLUSTER_NODE_PAUSED - The cluster node is paused. */
#define ERROR_CLUSTER_NODE_PAUSED	((ULONG)0x000013ceL)

/* MessageId  : 0x000013cf */
/* Approx. msg: ERROR_NODE_CANT_HOST_RESOURCE - The cluster resource cannot be brought online. The owner node cannot run this resource. */
#define ERROR_NODE_CANT_HOST_RESOURCE	((ULONG)0x000013cfL)

/* MessageId  : 0x000013d0 */
/* Approx. msg: ERROR_CLUSTER_NODE_NOT_READY - The cluster node is not ready to perform the requested operation. */
#define ERROR_CLUSTER_NODE_NOT_READY	((ULONG)0x000013d0L)

/* MessageId  : 0x000013d1 */
/* Approx. msg: ERROR_CLUSTER_NODE_SHUTTING_DOWN - The cluster node is shutting down. */
#define ERROR_CLUSTER_NODE_SHUTTING_DOWN	((ULONG)0x000013d1L)

/* MessageId  : 0x000013d2 */
/* Approx. msg: ERROR_CLUSTER_JOIN_ABORTED - The cluster join operation was aborted. */
#define ERROR_CLUSTER_JOIN_ABORTED	((ULONG)0x000013d2L)

/* MessageId  : 0x000013d3 */
/* Approx. msg: ERROR_CLUSTER_INCOMPATIBLE_VERSIONS - The cluster join operation failed due to incompatible software versions between the joining node and its sponsor. */
#define ERROR_CLUSTER_INCOMPATIBLE_VERSIONS	((ULONG)0x000013d3L)

/* MessageId  : 0x000013d4 */
/* Approx. msg: ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED - This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor. */
#define ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED	((ULONG)0x000013d4L)

/* MessageId  : 0x000013d5 */
/* Approx. msg: ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED - The system configuration changed during the cluster join or form operation. The join or form operation was aborted. */
#define ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED	((ULONG)0x000013d5L)

/* MessageId  : 0x000013d6 */
/* Approx. msg: ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND - The specified resource type was not found. */
#define ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND	((ULONG)0x000013d6L)

/* MessageId  : 0x000013d7 */
/* Approx. msg: ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED - The specified node does not support a resource of this type. This may be due to version inconsistencies or due to the absence of the resource DLL on this node. */
#define ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED	((ULONG)0x000013d7L)

/* MessageId  : 0x000013d8 */
/* Approx. msg: ERROR_CLUSTER_RESNAME_NOT_FOUND - The specified resource name is supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL. */
#define ERROR_CLUSTER_RESNAME_NOT_FOUND	((ULONG)0x000013d8L)

/* MessageId  : 0x000013d9 */
/* Approx. msg: ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED - No authentication package could be registered with the RPC server. */
#define ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED	((ULONG)0x000013d9L)

/* MessageId  : 0x000013da */
/* Approx. msg: ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST - You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group. */
#define ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST	((ULONG)0x000013daL)

/* MessageId  : 0x000013db */
/* Approx. msg: ERROR_CLUSTER_DATABASE_SEQMISMATCH - The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join. */
#define ERROR_CLUSTER_DATABASE_SEQMISMATCH	((ULONG)0x000013dbL)

/* MessageId  : 0x000013dc */
/* Approx. msg: ERROR_RESMON_INVALID_STATE - The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state. */
#define ERROR_RESMON_INVALID_STATE	((ULONG)0x000013dcL)

/* MessageId  : 0x000013dd */
/* Approx. msg: ERROR_CLUSTER_GUM_NOT_LOCKER - A non locker code got a request to reserve the lock for making global updates. */
#define ERROR_CLUSTER_GUM_NOT_LOCKER	((ULONG)0x000013ddL)

/* MessageId  : 0x000013de */
/* Approx. msg: ERROR_QUORUM_DISK_NOT_FOUND - The quorum disk could not be located by the cluster service. */
#define ERROR_QUORUM_DISK_NOT_FOUND	((ULONG)0x000013deL)

/* MessageId  : 0x000013df */
/* Approx. msg: ERROR_DATABASE_BACKUP_CORRUPT - The backup up cluster database is possibly corrupt. */
#define ERROR_DATABASE_BACKUP_CORRUPT	((ULONG)0x000013dfL)

/* MessageId  : 0x000013e0 */
/* Approx. msg: ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT - A DFS root already exists in this cluster node. */
#define ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT	((ULONG)0x000013e0L)

/* MessageId  : 0x000013e1 */
/* Approx. msg: ERROR_RESOURCE_PROPERTY_UNCHANGEABLE - An attempt to modify a resource property failed because it conflicts with another existing property. */
#define ERROR_RESOURCE_PROPERTY_UNCHANGEABLE	((ULONG)0x000013e1L)

/* MessageId  : 0x00001702 */
/* Approx. msg: ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE - An operation was attempted that is incompatible with the current membership state of the node. */
#define ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE	((ULONG)0x00001702L)

/* MessageId  : 0x00001703 */
/* Approx. msg: ERROR_CLUSTER_QUORUMLOG_NOT_FOUND - The quorum resource does not contain the quorum log. */
#define ERROR_CLUSTER_QUORUMLOG_NOT_FOUND	((ULONG)0x00001703L)

/* MessageId  : 0x00001704 */
/* Approx. msg: ERROR_CLUSTER_MEMBERSHIP_HALT - The membership engine requested shutdown of the cluster service on this node. */
#define ERROR_CLUSTER_MEMBERSHIP_HALT	((ULONG)0x00001704L)

/* MessageId  : 0x00001705 */
/* Approx. msg: ERROR_CLUSTER_INSTANCE_ID_MISMATCH - The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node. */
#define ERROR_CLUSTER_INSTANCE_ID_MISMATCH	((ULONG)0x00001705L)

/* MessageId  : 0x00001706 */
/* Approx. msg: ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP - A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network. */
#define ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP	((ULONG)0x00001706L)

/* MessageId  : 0x00001707 */
/* Approx. msg: ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH - The actual data type of the property did not match the expected data type of the property. */
#define ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH	((ULONG)0x00001707L)

/* MessageId  : 0x00001708 */
/* Approx. msg: ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP - The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available. */
#define ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP	((ULONG)0x00001708L)

/* MessageId  : 0x00001709 */
/* Approx. msg: ERROR_CLUSTER_PARAMETER_MISMATCH - Two or more parameter values specified for a resource's properties are in conflict. */
#define ERROR_CLUSTER_PARAMETER_MISMATCH	((ULONG)0x00001709L)

/* MessageId  : 0x0000170a */
/* Approx. msg: ERROR_NODE_CANNOT_BE_CLUSTERED - This computer cannot be made a member of a cluster. */
#define ERROR_NODE_CANNOT_BE_CLUSTERED	((ULONG)0x0000170aL)

/* MessageId  : 0x0000170b */
/* Approx. msg: ERROR_CLUSTER_WRONG_OS_VERSION - This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed. */
#define ERROR_CLUSTER_WRONG_OS_VERSION	((ULONG)0x0000170bL)

/* MessageId  : 0x0000170c */
/* Approx. msg: ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME - A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster. */
#define ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME	((ULONG)0x0000170cL)

/* MessageId  : 0x0000170d */
/* Approx. msg: ERROR_CLUSCFG_ALREADY_COMMITTED - The cluster configuration action has already been committed. */
#define ERROR_CLUSCFG_ALREADY_COMMITTED	((ULONG)0x0000170dL)

/* MessageId  : 0x0000170e */
/* Approx. msg: ERROR_CLUSCFG_ROLLBACK_FAILED - The cluster configuration action could not be rolled back. */
#define ERROR_CLUSCFG_ROLLBACK_FAILED	((ULONG)0x0000170eL)

/* MessageId  : 0x0000170f */
/* Approx. msg: ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT - The drive letter assigned to a system disk on one node conflicted with the driver letter assigned to a disk on another node. */
#define ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT	((ULONG)0x0000170fL)

/* MessageId  : 0x00001710 */
/* Approx. msg: ERROR_CLUSTER_OLD_VERSION - One or more nodes in the cluster are running a version of Windows that does not support this operation. */
#define ERROR_CLUSTER_OLD_VERSION	((ULONG)0x00001710L)

/* MessageId  : 0x00001711 */
/* Approx. msg: ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME - The name of the corresponding computer account doesn't match the Network Name for this resource. */
#define ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME	((ULONG)0x00001711L)

/* MessageId  : 0x00001712 */
/* Approx. msg: ERROR_CLUSTER_NO_NET_ADAPTERS - No network adapters are available. */
#define ERROR_CLUSTER_NO_NET_ADAPTERS	((ULONG)0x00001712L)

/* MessageId  : 0x00001713 */
/* Approx. msg: ERROR_CLUSTER_POISONED - The cluster node has been poisoned. */
#define ERROR_CLUSTER_POISONED	((ULONG)0x00001713L)

/* MessageId  : 0x00001770 */
/* Approx. msg: ERROR_ENCRYPTION_FAILED - The specified file could not be encrypted. */
#define ERROR_ENCRYPTION_FAILED	((ULONG)0x00001770L)

/* MessageId  : 0x00001771 */
/* Approx. msg: ERROR_DECRYPTION_FAILED - The specified file could not be decrypted. */
#define ERROR_DECRYPTION_FAILED	((ULONG)0x00001771L)

/* MessageId  : 0x00001772 */
/* Approx. msg: ERROR_FILE_ENCRYPTED - The specified file is encrypted and the user does not have the ability to decrypt it. */
#define ERROR_FILE_ENCRYPTED	((ULONG)0x00001772L)

/* MessageId  : 0x00001773 */
/* Approx. msg: ERROR_NO_RECOVERY_POLICY - There is no valid encryption recovery policy configured for this system. */
#define ERROR_NO_RECOVERY_POLICY	((ULONG)0x00001773L)

/* MessageId  : 0x00001774 */
/* Approx. msg: ERROR_NO_EFS - The required encryption driver is not loaded for this system. */
#define ERROR_NO_EFS	((ULONG)0x00001774L)

/* MessageId  : 0x00001775 */
/* Approx. msg: ERROR_WRONG_EFS - The file was encrypted with a different encryption driver than is currently loaded. */
#define ERROR_WRONG_EFS	((ULONG)0x00001775L)

/* MessageId  : 0x00001776 */
/* Approx. msg: ERROR_NO_USER_KEYS - There are no EFS keys defined for the user. */
#define ERROR_NO_USER_KEYS	((ULONG)0x00001776L)

/* MessageId  : 0x00001777 */
/* Approx. msg: ERROR_FILE_NOT_ENCRYPTED - The specified file is not encrypted. */
#define ERROR_FILE_NOT_ENCRYPTED	((ULONG)0x00001777L)

/* MessageId  : 0x00001778 */
/* Approx. msg: ERROR_NOT_EXPORT_FORMAT - The specified file is not in the defined EFS export format. */
#define ERROR_NOT_EXPORT_FORMAT	((ULONG)0x00001778L)

/* MessageId  : 0x00001779 */
/* Approx. msg: ERROR_FILE_READ_ONLY - The specified file is read only. */
#define ERROR_FILE_READ_ONLY	((ULONG)0x00001779L)

/* MessageId  : 0x0000177a */
/* Approx. msg: ERROR_DIR_EFS_DISALLOWED - The directory has been disabled for encryption. */
#define ERROR_DIR_EFS_DISALLOWED	((ULONG)0x0000177aL)

/* MessageId  : 0x0000177b */
/* Approx. msg: ERROR_EFS_SERVER_NOT_TRUSTED - The server is not trusted for remote encryption operation. */
#define ERROR_EFS_SERVER_NOT_TRUSTED	((ULONG)0x0000177bL)

/* MessageId  : 0x0000177c */
/* Approx. msg: ERROR_BAD_RECOVERY_POLICY - Recovery policy configured for this system contains invalid recovery certificate. */
#define ERROR_BAD_RECOVERY_POLICY	((ULONG)0x0000177cL)

/* MessageId  : 0x0000177d */
/* Approx. msg: ERROR_EFS_ALG_BLOB_TOO_BIG - The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file. */
#define ERROR_EFS_ALG_BLOB_TOO_BIG	((ULONG)0x0000177dL)

/* MessageId  : 0x0000177e */
/* Approx. msg: ERROR_VOLUME_NOT_SUPPORT_EFS - The disk partition does not support file encryption. */
#define ERROR_VOLUME_NOT_SUPPORT_EFS	((ULONG)0x0000177eL)

/* MessageId  : 0x0000177f */
/* Approx. msg: ERROR_EFS_DISABLED - This machine is disabled for file encryption. */
#define ERROR_EFS_DISABLED	((ULONG)0x0000177fL)

/* MessageId  : 0x00001780 */
/* Approx. msg: ERROR_EFS_VERSION_NOT_SUPPORT - A newer system is required to decrypt this encrypted file. */
#define ERROR_EFS_VERSION_NOT_SUPPORT	((ULONG)0x00001780L)

/* MessageId  : 0x000017e6 */
/* Approx. msg: ERROR_NO_BROWSER_SERVERS_FOUND - The list of servers for this workgroup is not currently available. */
#define ERROR_NO_BROWSER_SERVERS_FOUND	((ULONG)0x000017e6L)

/* MessageId  : 0x00001838 */
/* Approx. msg: SCHED_E_SERVICE_NOT_LOCALSYSTEM - The Task Scheduler service must be configured to run in the System account to function properly. Individual tasks may be configured to run in other accounts. */
#define SCHED_E_SERVICE_NOT_LOCALSYSTEM	((ULONG)0x00001838L)

/* MessageId  : 0x00001b59 */
/* Approx. msg: ERROR_CTX_WINSTATION_NAME_INVALID - The specified session name is invalid. */
#define ERROR_CTX_WINSTATION_NAME_INVALID	((ULONG)0x00001b59L)

/* MessageId  : 0x00001b5a */
/* Approx. msg: ERROR_CTX_INVALID_PD - The specified protocol driver is invalid. */
#define ERROR_CTX_INVALID_PD	((ULONG)0x00001b5aL)

/* MessageId  : 0x00001b5b */
/* Approx. msg: ERROR_CTX_PD_NOT_FOUND - The specified protocol driver was not found in the system path. */
#define ERROR_CTX_PD_NOT_FOUND	((ULONG)0x00001b5bL)

/* MessageId  : 0x00001b5c */
/* Approx. msg: ERROR_CTX_WD_NOT_FOUND - The specified terminal connection driver was not found in the system path. */
#define ERROR_CTX_WD_NOT_FOUND	((ULONG)0x00001b5cL)

/* MessageId  : 0x00001b5d */
/* Approx. msg: ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY - A registry key for event logging could not be created for this session. */
#define ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY	((ULONG)0x00001b5dL)

/* MessageId  : 0x00001b5e */
/* Approx. msg: ERROR_CTX_SERVICE_NAME_COLLISION - A service with the same name already exists on the system. */
#define ERROR_CTX_SERVICE_NAME_COLLISION	((ULONG)0x00001b5eL)

/* MessageId  : 0x00001b5f */
/* Approx. msg: ERROR_CTX_CLOSE_PENDING - A close operation is pending on the session. */
#define ERROR_CTX_CLOSE_PENDING	((ULONG)0x00001b5fL)

/* MessageId  : 0x00001b60 */
/* Approx. msg: ERROR_CTX_NO_OUTBUF - There are no free output buffers available. */
#define ERROR_CTX_NO_OUTBUF	((ULONG)0x00001b60L)

/* MessageId  : 0x00001b61 */
/* Approx. msg: ERROR_CTX_MODEM_INF_NOT_FOUND - The MODEM.INF file was not found. */
#define ERROR_CTX_MODEM_INF_NOT_FOUND	((ULONG)0x00001b61L)

/* MessageId  : 0x00001b62 */
/* Approx. msg: ERROR_CTX_INVALID_MODEMNAME - The modem name was not found in MODEM.INF. */
#define ERROR_CTX_INVALID_MODEMNAME	((ULONG)0x00001b62L)

/* MessageId  : 0x00001b63 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_ERROR - The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem. */
#define ERROR_CTX_MODEM_RESPONSE_ERROR	((ULONG)0x00001b63L)

/* MessageId  : 0x00001b64 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_TIMEOUT - The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on. */
#define ERROR_CTX_MODEM_RESPONSE_TIMEOUT	((ULONG)0x00001b64L)

/* MessageId  : 0x00001b65 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_NO_CARRIER - Carrier detect has failed or carrier has been dropped due to disconnect. */
#define ERROR_CTX_MODEM_RESPONSE_NO_CARRIER	((ULONG)0x00001b65L)

/* MessageId  : 0x00001b66 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE - Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional. */
#define ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE	((ULONG)0x00001b66L)

/* MessageId  : 0x00001b67 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_BUSY - Busy signal detected at remote site on callback. */
#define ERROR_CTX_MODEM_RESPONSE_BUSY	((ULONG)0x00001b67L)

/* MessageId  : 0x00001b68 */
/* Approx. msg: ERROR_CTX_MODEM_RESPONSE_VOICE - Voice detected at remote site on callback. */
#define ERROR_CTX_MODEM_RESPONSE_VOICE	((ULONG)0x00001b68L)

/* MessageId  : 0x00001b69 */
/* Approx. msg: ERROR_CTX_TD_ERROR - Transport driver error */
#define ERROR_CTX_TD_ERROR	((ULONG)0x00001b69L)

/* MessageId  : 0x00001b6e */
/* Approx. msg: ERROR_CTX_WINSTATION_NOT_FOUND - The specified session cannot be found. */
#define ERROR_CTX_WINSTATION_NOT_FOUND	((ULONG)0x00001b6eL)

/* MessageId  : 0x00001b6f */
/* Approx. msg: ERROR_CTX_WINSTATION_ALREADY_EXISTS - The specified session name is already in use. */
#define ERROR_CTX_WINSTATION_ALREADY_EXISTS	((ULONG)0x00001b6fL)

/* MessageId  : 0x00001b70 */
/* Approx. msg: ERROR_CTX_WINSTATION_BUSY - The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation. */
#define ERROR_CTX_WINSTATION_BUSY	((ULONG)0x00001b70L)

/* MessageId  : 0x00001b71 */
/* Approx. msg: ERROR_CTX_BAD_VIDEO_MODE - An attempt has been made to connect to a session whose video mode is not supported by the current client. */
#define ERROR_CTX_BAD_VIDEO_MODE	((ULONG)0x00001b71L)

/* MessageId  : 0x00001b7b */
/* Approx. msg: ERROR_CTX_GRAPHICS_INVALID - The application attempted to enable DOS graphics mode. DOS graphics mode is not supported. */
#define ERROR_CTX_GRAPHICS_INVALID	((ULONG)0x00001b7bL)

/* MessageId  : 0x00001b7d */
/* Approx. msg: ERROR_CTX_LOGON_DISABLED - Your interactive logon privilege has been disabled. Please contact your administrator. */
#define ERROR_CTX_LOGON_DISABLED	((ULONG)0x00001b7dL)

/* MessageId  : 0x00001b7e */
/* Approx. msg: ERROR_CTX_NOT_CONSOLE - The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access. */
#define ERROR_CTX_NOT_CONSOLE	((ULONG)0x00001b7eL)

/* MessageId  : 0x00001b80 */
/* Approx. msg: ERROR_CTX_CLIENT_QUERY_TIMEOUT - The client failed to respond to the server connect message. */
#define ERROR_CTX_CLIENT_QUERY_TIMEOUT	((ULONG)0x00001b80L)

/* MessageId  : 0x00001b81 */
/* Approx. msg: ERROR_CTX_CONSOLE_DISCONNECT - Disconnecting the console session is not supported. */
#define ERROR_CTX_CONSOLE_DISCONNECT	((ULONG)0x00001b81L)

/* MessageId  : 0x00001b82 */
/* Approx. msg: ERROR_CTX_CONSOLE_CONNECT - Reconnecting a disconnected session to the console is not supported. */
#define ERROR_CTX_CONSOLE_CONNECT	((ULONG)0x00001b82L)

/* MessageId  : 0x00001b84 */
/* Approx. msg: ERROR_CTX_SHADOW_DENIED - The request to control another session remotely was denied. */
#define ERROR_CTX_SHADOW_DENIED	((ULONG)0x00001b84L)

/* MessageId  : 0x00001b85 */
/* Approx. msg: ERROR_CTX_WINSTATION_ACCESS_DENIED - The requested session access is denied. */
#define ERROR_CTX_WINSTATION_ACCESS_DENIED	((ULONG)0x00001b85L)

/* MessageId  : 0x00001b89 */
/* Approx. msg: ERROR_CTX_INVALID_WD - The specified terminal connection driver is invalid. */
#define ERROR_CTX_INVALID_WD	((ULONG)0x00001b89L)

/* MessageId  : 0x00001b8a */
/* Approx. msg: ERROR_CTX_SHADOW_INVALID - The requested session cannot be controlled remotely. This may be because the session is disconnected or does not currently have a user logged on. */
#define ERROR_CTX_SHADOW_INVALID	((ULONG)0x00001b8aL)

/* MessageId  : 0x00001b8b */
/* Approx. msg: ERROR_CTX_SHADOW_DISABLED - The requested session is not configured to allow remote control. */
#define ERROR_CTX_SHADOW_DISABLED	((ULONG)0x00001b8bL)

/* MessageId  : 0x00001b8c */
/* Approx. msg: ERROR_CTX_CLIENT_LICENSE_IN_USE - Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user. Please call your system administrator to obtain a unique license number. */
#define ERROR_CTX_CLIENT_LICENSE_IN_USE	((ULONG)0x00001b8cL)

/* MessageId  : 0x00001b8d */
/* Approx. msg: ERROR_CTX_CLIENT_LICENSE_NOT_SET - Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client. Please contact your system administrator. */
#define ERROR_CTX_CLIENT_LICENSE_NOT_SET	((ULONG)0x00001b8dL)

/* MessageId  : 0x00001b8e */
/* Approx. msg: ERROR_CTX_LICENSE_NOT_AVAILABLE - The system has reached its licensed logon limit. Please try again later. */
#define ERROR_CTX_LICENSE_NOT_AVAILABLE	((ULONG)0x00001b8eL)

/* MessageId  : 0x00001b8f */
/* Approx. msg: ERROR_CTX_LICENSE_CLIENT_INVALID - The client you are using is not licensed to use this system. Your logon request is denied. */
#define ERROR_CTX_LICENSE_CLIENT_INVALID	((ULONG)0x00001b8fL)

/* MessageId  : 0x00001b90 */
/* Approx. msg: ERROR_CTX_LICENSE_EXPIRED - The system license has expired. Your logon request is denied. */
#define ERROR_CTX_LICENSE_EXPIRED	((ULONG)0x00001b90L)

/* MessageId  : 0x00001b91 */
/* Approx. msg: ERROR_CTX_SHADOW_NOT_RUNNING - Remote control could not be terminated because the specified session is not currently being remotely controlled. */
#define ERROR_CTX_SHADOW_NOT_RUNNING	((ULONG)0x00001b91L)

/* MessageId  : 0x00001b92 */
/* Approx. msg: ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE - The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported. */
#define ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE	((ULONG)0x00001b92L)

/* MessageId  : 0x00001b93 */
/* Approx. msg: ERROR_ACTIVATION_COUNT_EXCEEDED - Activation has already been reset the maximum number of times for this installation. Your activation timer will not be cleared. */
#define ERROR_ACTIVATION_COUNT_EXCEEDED	((ULONG)0x00001b93L)

/* MessageId  : 0x00001b94 */
/* Approx. msg: ERROR_CTX_WINSTATIONS_DISABLED - Remote logins are currently disabled. */
#define ERROR_CTX_WINSTATIONS_DISABLED	((ULONG)0x00001b94L)

/* MessageId  : 0x00001b95 */
/* Approx. msg: ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED - You do not have the proper encryption level to access this Session. */
#define ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED	((ULONG)0x00001b95L)

/* MessageId  : 0x00001b96 */
/* Approx. msg: ERROR_CTX_SESSION_IN_USE - The user %s\\%s is currently logged on to this computer. Only the current user or an administrator can log on to this computer. */
#define ERROR_CTX_SESSION_IN_USE	((ULONG)0x00001b96L)

/* MessageId  : 0x00001b97 */
/* Approx. msg: ERROR_CTX_NO_FORCE_LOGOFF - The user %s\\%s is already logged on to the console of this computer. You do not have permission to log in at this time. To resolve this issue, contact %s\\%s and have them log off. */
#define ERROR_CTX_NO_FORCE_LOGOFF	((ULONG)0x00001b97L)

/* MessageId  : 0x00001b98 */
/* Approx. msg: ERROR_CTX_ACCOUNT_RESTRICTION - Unable to log you on because of an account restriction. */
#define ERROR_CTX_ACCOUNT_RESTRICTION	((ULONG)0x00001b98L)

/* MessageId  : 0x00001b99 */
/* Approx. msg: ERROR_RDP_PROTOCOL_ERROR - The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client. */
#define ERROR_RDP_PROTOCOL_ERROR	((ULONG)0x00001b99L)

/* MessageId  : 0x00001b9a */
/* Approx. msg: ERROR_CTX_CDM_CONNECT - The Client Drive Mapping Service Has Connected on Terminal Connection. */
#define ERROR_CTX_CDM_CONNECT	((ULONG)0x00001b9aL)

/* MessageId  : 0x00001b9b */
/* Approx. msg: ERROR_CTX_CDM_DISCONNECT - The Client Drive Mapping Service Has Disconnected on Terminal Connection. */
#define ERROR_CTX_CDM_DISCONNECT	((ULONG)0x00001b9bL)

/* MessageId  : 0x00001f41 */
/* Approx. msg: FRS_ERR_INVALID_API_SEQUENCE - The file replication service API was called incorrectly. */
#define FRS_ERR_INVALID_API_SEQUENCE	((ULONG)0x00001f41L)

/* MessageId  : 0x00001f42 */
/* Approx. msg: FRS_ERR_STARTING_SERVICE - The file replication service cannot be started. */
#define FRS_ERR_STARTING_SERVICE	((ULONG)0x00001f42L)

/* MessageId  : 0x00001f43 */
/* Approx. msg: FRS_ERR_STOPPING_SERVICE - The file replication service cannot be stopped. */
#define FRS_ERR_STOPPING_SERVICE	((ULONG)0x00001f43L)

/* MessageId  : 0x00001f44 */
/* Approx. msg: FRS_ERR_INTERNAL_API - The file replication service API terminated the request. The event log may have more information. */
#define FRS_ERR_INTERNAL_API	((ULONG)0x00001f44L)

/* MessageId  : 0x00001f45 */
/* Approx. msg: FRS_ERR_INTERNAL - The file replication service terminated the request. The event log may have more information. */
#define FRS_ERR_INTERNAL	((ULONG)0x00001f45L)

/* MessageId  : 0x00001f46 */
/* Approx. msg: FRS_ERR_SERVICE_COMM - The file replication service cannot be contacted. The event log may have more information. */
#define FRS_ERR_SERVICE_COMM	((ULONG)0x00001f46L)

/* MessageId  : 0x00001f47 */
/* Approx. msg: FRS_ERR_INSUFFICIENT_PRIV - The file replication service cannot satisfy the request because the user has insufficient privileges. The event log may have more information. */
#define FRS_ERR_INSUFFICIENT_PRIV	((ULONG)0x00001f47L)

/* MessageId  : 0x00001f48 */
/* Approx. msg: FRS_ERR_AUTHENTICATION - The file replication service cannot satisfy the request because authenticated RPC is not available. The event log may have more information. */
#define FRS_ERR_AUTHENTICATION	((ULONG)0x00001f48L)

/* MessageId  : 0x00001f49 */
/* Approx. msg: FRS_ERR_PARENT_INSUFFICIENT_PRIV - The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller. The event log may have more information. */
#define FRS_ERR_PARENT_INSUFFICIENT_PRIV	((ULONG)0x00001f49L)

/* MessageId  : 0x00001f4a */
/* Approx. msg: FRS_ERR_PARENT_AUTHENTICATION - The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller. The event log may have more information. */
#define FRS_ERR_PARENT_AUTHENTICATION	((ULONG)0x00001f4aL)

/* MessageId  : 0x00001f4b */
/* Approx. msg: FRS_ERR_CHILD_TO_PARENT_COMM - The file replication service cannot communicate with the file replication service on the domain controller. The event log may have more information. */
#define FRS_ERR_CHILD_TO_PARENT_COMM	((ULONG)0x00001f4bL)

/* MessageId  : 0x00001f4c */
/* Approx. msg: FRS_ERR_PARENT_TO_CHILD_COMM - The file replication service on the domain controller cannot communicate with the file replication service on this computer. The event log may have more information. */
#define FRS_ERR_PARENT_TO_CHILD_COMM	((ULONG)0x00001f4cL)

/* MessageId  : 0x00001f4d */
/* Approx. msg: FRS_ERR_SYSVOL_POPULATE - The file replication service cannot populate the system volume because of an internal error. The event log may have more information. */
#define FRS_ERR_SYSVOL_POPULATE	((ULONG)0x00001f4dL)

/* MessageId  : 0x00001f4e */
/* Approx. msg: FRS_ERR_SYSVOL_POPULATE_TIMEOUT - The file replication service cannot populate the system volume because of an internal timeout. The event log may have more information. */
#define FRS_ERR_SYSVOL_POPULATE_TIMEOUT	((ULONG)0x00001f4eL)

/* MessageId  : 0x00001f4f */
/* Approx. msg: FRS_ERR_SYSVOL_IS_BUSY - The file replication service cannot process the request. The system volume is busy with a previous request. */
#define FRS_ERR_SYSVOL_IS_BUSY	((ULONG)0x00001f4fL)

/* MessageId  : 0x00001f50 */
/* Approx. msg: FRS_ERR_SYSVOL_DEMOTE - The file replication service cannot stop replicating the system volume because of an internal error. The event log may have more information. */
#define FRS_ERR_SYSVOL_DEMOTE	((ULONG)0x00001f50L)

/* MessageId  : 0x00001f51 */
/* Approx. msg: FRS_ERR_INVALID_SERVICE_PARAMETER - The file replication service detected an invalid parameter. */
#define FRS_ERR_INVALID_SERVICE_PARAMETER	((ULONG)0x00001f51L)

/* MessageId  : 0x00002008 */
/* Approx. msg: ERROR_DS_NOT_INSTALLED - An error occurred while installing the directory service. For more information, see the event log. */
#define ERROR_DS_NOT_INSTALLED	((ULONG)0x00002008L)

/* MessageId  : 0x00002009 */
/* Approx. msg: ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY - The directory service evaluated group memberships locally. */
#define ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY	((ULONG)0x00002009L)

/* MessageId  : 0x0000200a */
/* Approx. msg: ERROR_DS_NO_ATTRIBUTE_OR_VALUE - The specified directory service attribute or value does not exist. */
#define ERROR_DS_NO_ATTRIBUTE_OR_VALUE	((ULONG)0x0000200aL)

/* MessageId  : 0x0000200b */
/* Approx. msg: ERROR_DS_INVALID_ATTRIBUTE_SYNTAX - The attribute syntax specified to the directory service is invalid. */
#define ERROR_DS_INVALID_ATTRIBUTE_SYNTAX	((ULONG)0x0000200bL)

/* MessageId  : 0x0000200c */
/* Approx. msg: ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED - The attribute type specified to the directory service is not defined. */
#define ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED	((ULONG)0x0000200cL)

/* MessageId  : 0x0000200d */
/* Approx. msg: ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS - The specified directory service attribute or value already exists. */
#define ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS	((ULONG)0x0000200dL)

/* MessageId  : 0x0000200e */
/* Approx. msg: ERROR_DS_BUSY - The directory service is busy. */
#define ERROR_DS_BUSY	((ULONG)0x0000200eL)

/* MessageId  : 0x0000200f */
/* Approx. msg: ERROR_DS_UNAVAILABLE - The directory service is unavailable. */
#define ERROR_DS_UNAVAILABLE	((ULONG)0x0000200fL)

/* MessageId  : 0x00002010 */
/* Approx. msg: ERROR_DS_NO_RIDS_ALLOCATED - The directory service was unable to allocate a relative identifier. */
#define ERROR_DS_NO_RIDS_ALLOCATED	((ULONG)0x00002010L)

/* MessageId  : 0x00002011 */
/* Approx. msg: ERROR_DS_NO_MORE_RIDS - The directory service has exhausted the pool of relative identifiers. */
#define ERROR_DS_NO_MORE_RIDS	((ULONG)0x00002011L)

/* MessageId  : 0x00002012 */
/* Approx. msg: ERROR_DS_INCORRECT_ROLE_OWNER - The requested operation could not be performed because the directory service is not the master for that type of operation. */
#define ERROR_DS_INCORRECT_ROLE_OWNER	((ULONG)0x00002012L)

/* MessageId  : 0x00002013 */
/* Approx. msg: ERROR_DS_RIDMGR_INIT_ERROR - The directory service was unable to initialize the subsystem that allocates relative identifiers. */
#define ERROR_DS_RIDMGR_INIT_ERROR	((ULONG)0x00002013L)

/* MessageId  : 0x00002014 */
/* Approx. msg: ERROR_DS_OBJ_CLASS_VIOLATION - The requested operation did not satisfy one or more constraints associated with the class of the object. */
#define ERROR_DS_OBJ_CLASS_VIOLATION	((ULONG)0x00002014L)

/* MessageId  : 0x00002015 */
/* Approx. msg: ERROR_DS_CANT_ON_NON_LEAF - The directory service can perform the requested operation only on a leaf object. */
#define ERROR_DS_CANT_ON_NON_LEAF	((ULONG)0x00002015L)

/* MessageId  : 0x00002016 */
/* Approx. msg: ERROR_DS_CANT_ON_RDN - The directory service cannot perform the requested operation on the RDN attribute of an object. */
#define ERROR_DS_CANT_ON_RDN	((ULONG)0x00002016L)

/* MessageId  : 0x00002017 */
/* Approx. msg: ERROR_DS_CANT_MOD_OBJ_CLASS - The directory service detected an attempt to modify the object class of an object. */
#define ERROR_DS_CANT_MOD_OBJ_CLASS	((ULONG)0x00002017L)

/* MessageId  : 0x00002018 */
/* Approx. msg: ERROR_DS_CROSS_DOM_MOVE_ERROR - The requested cross-domain move operation could not be performed. */
#define ERROR_DS_CROSS_DOM_MOVE_ERROR	((ULONG)0x00002018L)

/* MessageId  : 0x00002019 */
/* Approx. msg: ERROR_DS_GC_NOT_AVAILABLE - Unable to contact the global catalog server. */
#define ERROR_DS_GC_NOT_AVAILABLE	((ULONG)0x00002019L)

/* MessageId  : 0x0000201a */
/* Approx. msg: ERROR_SHARED_POLICY - The policy object is shared and can only be modified at the root. */
#define ERROR_SHARED_POLICY	((ULONG)0x0000201aL)

/* MessageId  : 0x0000201b */
/* Approx. msg: ERROR_POLICY_OBJECT_NOT_FOUND - The policy object does not exist. */
#define ERROR_POLICY_OBJECT_NOT_FOUND	((ULONG)0x0000201bL)

/* MessageId  : 0x0000201c */
/* Approx. msg: ERROR_POLICY_ONLY_IN_DS - The requested policy information is only in the directory service. */
#define ERROR_POLICY_ONLY_IN_DS	((ULONG)0x0000201cL)

/* MessageId  : 0x0000201d */
/* Approx. msg: ERROR_PROMOTION_ACTIVE - A domain controller promotion is currently active. */
#define ERROR_PROMOTION_ACTIVE	((ULONG)0x0000201dL)

/* MessageId  : 0x0000201e */
/* Approx. msg: ERROR_NO_PROMOTION_ACTIVE - A domain controller promotion is not currently active */
#define ERROR_NO_PROMOTION_ACTIVE	((ULONG)0x0000201eL)

/* MessageId  : 0x00002020 */
/* Approx. msg: ERROR_DS_OPERATIONS_ERROR - An operations error occurred. */
#define ERROR_DS_OPERATIONS_ERROR	((ULONG)0x00002020L)

/* MessageId  : 0x00002021 */
/* Approx. msg: ERROR_DS_PROTOCOL_ERROR - A protocol error occurred. */
#define ERROR_DS_PROTOCOL_ERROR	((ULONG)0x00002021L)

/* MessageId  : 0x00002022 */
/* Approx. msg: ERROR_DS_TIMELIMIT_EXCEEDED - The time limit for this request was exceeded. */
#define ERROR_DS_TIMELIMIT_EXCEEDED	((ULONG)0x00002022L)

/* MessageId  : 0x00002023 */
/* Approx. msg: ERROR_DS_SIZELIMIT_EXCEEDED - The size limit for this request was exceeded. */
#define ERROR_DS_SIZELIMIT_EXCEEDED	((ULONG)0x00002023L)

/* MessageId  : 0x00002024 */
/* Approx. msg: ERROR_DS_ADMIN_LIMIT_EXCEEDED - The administrative limit for this request was exceeded. */
#define ERROR_DS_ADMIN_LIMIT_EXCEEDED	((ULONG)0x00002024L)

/* MessageId  : 0x00002025 */
/* Approx. msg: ERROR_DS_COMPARE_FALSE - The compare response was false. */
#define ERROR_DS_COMPARE_FALSE	((ULONG)0x00002025L)

/* MessageId  : 0x00002026 */
/* Approx. msg: ERROR_DS_COMPARE_TRUE - The compare response was true. */
#define ERROR_DS_COMPARE_TRUE	((ULONG)0x00002026L)

/* MessageId  : 0x00002027 */
/* Approx. msg: ERROR_DS_AUTH_METHOD_NOT_SUPPORTED - The requested authentication method is not supported by the server. */
#define ERROR_DS_AUTH_METHOD_NOT_SUPPORTED	((ULONG)0x00002027L)

/* MessageId  : 0x00002028 */
/* Approx. msg: ERROR_DS_STRONG_AUTH_REQUIRED - A more secure authentication method is required for this server. */
#define ERROR_DS_STRONG_AUTH_REQUIRED	((ULONG)0x00002028L)

/* MessageId  : 0x00002029 */
/* Approx. msg: ERROR_DS_INAPPROPRIATE_AUTH - Inappropriate authentication. */
#define ERROR_DS_INAPPROPRIATE_AUTH	((ULONG)0x00002029L)

/* MessageId  : 0x0000202a */
/* Approx. msg: ERROR_DS_AUTH_UNKNOWN - The authentication mechanism is unknown. */
#define ERROR_DS_AUTH_UNKNOWN	((ULONG)0x0000202aL)

/* MessageId  : 0x0000202b */
/* Approx. msg: ERROR_DS_REFERRAL - A referral was returned from the server. */
#define ERROR_DS_REFERRAL	((ULONG)0x0000202bL)

/* MessageId  : 0x0000202c */
/* Approx. msg: ERROR_DS_UNAVAILABLE_CRIT_EXTENSION - The server does not support the requested critical extension. */
#define ERROR_DS_UNAVAILABLE_CRIT_EXTENSION	((ULONG)0x0000202cL)

/* MessageId  : 0x0000202d */
/* Approx. msg: ERROR_DS_CONFIDENTIALITY_REQUIRED - This request requires a secure connection. */
#define ERROR_DS_CONFIDENTIALITY_REQUIRED	((ULONG)0x0000202dL)

/* MessageId  : 0x0000202e */
/* Approx. msg: ERROR_DS_INAPPROPRIATE_MATCHING - Inappropriate matching. */
#define ERROR_DS_INAPPROPRIATE_MATCHING	((ULONG)0x0000202eL)

/* MessageId  : 0x0000202f */
/* Approx. msg: ERROR_DS_CONSTRAINT_VIOLATION - A constraint violation occurred. */
#define ERROR_DS_CONSTRAINT_VIOLATION	((ULONG)0x0000202fL)

/* MessageId  : 0x00002030 */
/* Approx. msg: ERROR_DS_NO_SUCH_OBJECT - There is no such object on the server. */
#define ERROR_DS_NO_SUCH_OBJECT	((ULONG)0x00002030L)

/* MessageId  : 0x00002031 */
/* Approx. msg: ERROR_DS_ALIAS_PROBLEM - There is an alias problem. */
#define ERROR_DS_ALIAS_PROBLEM	((ULONG)0x00002031L)

/* MessageId  : 0x00002032 */
/* Approx. msg: ERROR_DS_INVALID_DN_SYNTAX - An invalid dn syntax has been specified. */
#define ERROR_DS_INVALID_DN_SYNTAX	((ULONG)0x00002032L)

/* MessageId  : 0x00002033 */
/* Approx. msg: ERROR_DS_IS_LEAF - The object is a leaf object. */
#define ERROR_DS_IS_LEAF	((ULONG)0x00002033L)

/* MessageId  : 0x00002034 */
/* Approx. msg: ERROR_DS_ALIAS_DEREF_PROBLEM - There is an alias dereferencing problem. */
#define ERROR_DS_ALIAS_DEREF_PROBLEM	((ULONG)0x00002034L)

/* MessageId  : 0x00002035 */
/* Approx. msg: ERROR_DS_UNWILLING_TO_PERFORM - The server is unwilling to process the request. */
#define ERROR_DS_UNWILLING_TO_PERFORM	((ULONG)0x00002035L)

/* MessageId  : 0x00002036 */
/* Approx. msg: ERROR_DS_LOOP_DETECT - A loop has been detected. */
#define ERROR_DS_LOOP_DETECT	((ULONG)0x00002036L)

/* MessageId  : 0x00002037 */
/* Approx. msg: ERROR_DS_NAMING_VIOLATION - There is a naming violation. */
#define ERROR_DS_NAMING_VIOLATION	((ULONG)0x00002037L)

/* MessageId  : 0x00002038 */
/* Approx. msg: ERROR_DS_OBJECT_RESULTS_TOO_LARGE - The result set is too large. */
#define ERROR_DS_OBJECT_RESULTS_TOO_LARGE	((ULONG)0x00002038L)

/* MessageId  : 0x00002039 */
/* Approx. msg: ERROR_DS_AFFECTS_MULTIPLE_DSAS - The operation affects multiple DSAs */
#define ERROR_DS_AFFECTS_MULTIPLE_DSAS	((ULONG)0x00002039L)

/* MessageId  : 0x0000203a */
/* Approx. msg: ERROR_DS_SERVER_DOWN - The server is not operational. */
#define ERROR_DS_SERVER_DOWN	((ULONG)0x0000203aL)

/* MessageId  : 0x0000203b */
/* Approx. msg: ERROR_DS_LOCAL_ERROR - A local error has occurred. */
#define ERROR_DS_LOCAL_ERROR	((ULONG)0x0000203bL)

/* MessageId  : 0x0000203c */
/* Approx. msg: ERROR_DS_ENCODING_ERROR - An encoding error has occurred. */
#define ERROR_DS_ENCODING_ERROR	((ULONG)0x0000203cL)

/* MessageId  : 0x0000203d */
/* Approx. msg: ERROR_DS_DECODING_ERROR - A decoding error has occurred. */
#define ERROR_DS_DECODING_ERROR	((ULONG)0x0000203dL)

/* MessageId  : 0x0000203e */
/* Approx. msg: ERROR_DS_FILTER_UNKNOWN - The search filter cannot be recognized. */
#define ERROR_DS_FILTER_UNKNOWN	((ULONG)0x0000203eL)

/* MessageId  : 0x0000203f */
/* Approx. msg: ERROR_DS_PARAM_ERROR - One or more parameters are illegal. */
#define ERROR_DS_PARAM_ERROR	((ULONG)0x0000203fL)

/* MessageId  : 0x00002040 */
/* Approx. msg: ERROR_DS_NOT_SUPPORTED - The specified method is not supported. */
#define ERROR_DS_NOT_SUPPORTED	((ULONG)0x00002040L)

/* MessageId  : 0x00002041 */
/* Approx. msg: ERROR_DS_NO_RESULTS_RETURNED - No results were returned. */
#define ERROR_DS_NO_RESULTS_RETURNED	((ULONG)0x00002041L)

/* MessageId  : 0x00002042 */
/* Approx. msg: ERROR_DS_CONTROL_NOT_FOUND - The specified control is not supported by the server. */
#define ERROR_DS_CONTROL_NOT_FOUND	((ULONG)0x00002042L)

/* MessageId  : 0x00002043 */
/* Approx. msg: ERROR_DS_CLIENT_LOOP - A referral loop was detected by the client. */
#define ERROR_DS_CLIENT_LOOP	((ULONG)0x00002043L)

/* MessageId  : 0x00002044 */
/* Approx. msg: ERROR_DS_REFERRAL_LIMIT_EXCEEDED - The preset referral limit was exceeded. */
#define ERROR_DS_REFERRAL_LIMIT_EXCEEDED	((ULONG)0x00002044L)

/* MessageId  : 0x00002045 */
/* Approx. msg: ERROR_DS_SORT_CONTROL_MISSING - The search requires a SORT control. */
#define ERROR_DS_SORT_CONTROL_MISSING	((ULONG)0x00002045L)

/* MessageId  : 0x00002046 */
/* Approx. msg: ERROR_DS_OFFSET_RANGE_ERROR - The search results exceed the offset range specified. */
#define ERROR_DS_OFFSET_RANGE_ERROR	((ULONG)0x00002046L)

/* MessageId  : 0x0000206d */
/* Approx. msg: ERROR_DS_ROOT_MUST_BE_NC - The root object must be the head of a naming context. The root object cannot have an instantiated parent. */
#define ERROR_DS_ROOT_MUST_BE_NC	((ULONG)0x0000206dL)

/* MessageId  : 0x0000206e */
/* Approx. msg: ERROR_DS_ADD_REPLICA_INHIBITED - The add replica operation cannot be performed. The naming context must be writeable in order to create the replica. */
#define ERROR_DS_ADD_REPLICA_INHIBITED	((ULONG)0x0000206eL)

/* MessageId  : 0x0000206f */
/* Approx. msg: ERROR_DS_ATT_NOT_DEF_IN_SCHEMA - A reference to an attribute that is not defined in the schema occurred. */
#define ERROR_DS_ATT_NOT_DEF_IN_SCHEMA	((ULONG)0x0000206fL)

/* MessageId  : 0x00002070 */
/* Approx. msg: ERROR_DS_MAX_OBJ_SIZE_EXCEEDED - The maximum size of an object has been exceeded. */
#define ERROR_DS_MAX_OBJ_SIZE_EXCEEDED	((ULONG)0x00002070L)

/* MessageId  : 0x00002071 */
/* Approx. msg: ERROR_DS_OBJ_STRING_NAME_EXISTS - An attempt was made to add an object to the directory with a name that is already in use. */
#define ERROR_DS_OBJ_STRING_NAME_EXISTS	((ULONG)0x00002071L)

/* MessageId  : 0x00002072 */
/* Approx. msg: ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA - An attempt was made to add an object of a class that does not have an RDN defined in the schema. */
#define ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA	((ULONG)0x00002072L)

/* MessageId  : 0x00002073 */
/* Approx. msg: ERROR_DS_RDN_DOESNT_MATCH_SCHEMA - An attempt was made to add an object using an RDN that is not the RDN defined in the schema. */
#define ERROR_DS_RDN_DOESNT_MATCH_SCHEMA	((ULONG)0x00002073L)

/* MessageId  : 0x00002074 */
/* Approx. msg: ERROR_DS_NO_REQUESTED_ATTS_FOUND - None of the requested attributes were found on the objects. */
#define ERROR_DS_NO_REQUESTED_ATTS_FOUND	((ULONG)0x00002074L)

/* MessageId  : 0x00002075 */
/* Approx. msg: ERROR_DS_USER_BUFFER_TO_SMALL - The user buffer is too small. */
#define ERROR_DS_USER_BUFFER_TO_SMALL	((ULONG)0x00002075L)

/* MessageId  : 0x00002076 */
/* Approx. msg: ERROR_DS_ATT_IS_NOT_ON_OBJ - The attribute specified in the operation is not present on the object. */
#define ERROR_DS_ATT_IS_NOT_ON_OBJ	((ULONG)0x00002076L)

/* MessageId  : 0x00002077 */
/* Approx. msg: ERROR_DS_ILLEGAL_MOD_OPERATION - Illegal modify operation. Some aspect of the modification is not permitted. */
#define ERROR_DS_ILLEGAL_MOD_OPERATION	((ULONG)0x00002077L)

/* MessageId  : 0x00002078 */
/* Approx. msg: ERROR_DS_OBJ_TOO_LARGE - The specified object is too large. */
#define ERROR_DS_OBJ_TOO_LARGE	((ULONG)0x00002078L)

/* MessageId  : 0x00002079 */
/* Approx. msg: ERROR_DS_BAD_INSTANCE_TYPE - The specified instance type is not valid. */
#define ERROR_DS_BAD_INSTANCE_TYPE	((ULONG)0x00002079L)

/* MessageId  : 0x0000207a */
/* Approx. msg: ERROR_DS_MASTERDSA_REQUIRED - The operation must be performed at a master DSA. */
#define ERROR_DS_MASTERDSA_REQUIRED	((ULONG)0x0000207aL)

/* MessageId  : 0x0000207b */
/* Approx. msg: ERROR_DS_OBJECT_CLASS_REQUIRED - The object class attribute must be specified. */
#define ERROR_DS_OBJECT_CLASS_REQUIRED	((ULONG)0x0000207bL)

/* MessageId  : 0x0000207c */
/* Approx. msg: ERROR_DS_MISSING_REQUIRED_ATT - A required attribute is missing. */
#define ERROR_DS_MISSING_REQUIRED_ATT	((ULONG)0x0000207cL)

/* MessageId  : 0x0000207d */
/* Approx. msg: ERROR_DS_ATT_NOT_DEF_FOR_CLASS - An attempt was made to modify an object to include an attribute that is not legal for its class */
#define ERROR_DS_ATT_NOT_DEF_FOR_CLASS	((ULONG)0x0000207dL)

/* MessageId  : 0x0000207e */
/* Approx. msg: ERROR_DS_ATT_ALREADY_EXISTS - The specified attribute is already present on the object. */
#define ERROR_DS_ATT_ALREADY_EXISTS	((ULONG)0x0000207eL)

/* MessageId  : 0x00002080 */
/* Approx. msg: ERROR_DS_CANT_ADD_ATT_VALUES - The specified attribute is not present, or has no values. */
#define ERROR_DS_CANT_ADD_ATT_VALUES	((ULONG)0x00002080L)

/* MessageId  : 0x00002081 */
/* Approx. msg: ERROR_DS_SINGLE_VALUE_CONSTRAINT - Multiple values were specified for an attribute that can have only one value. */
#define ERROR_DS_SINGLE_VALUE_CONSTRAINT	((ULONG)0x00002081L)

/* MessageId  : 0x00002082 */
/* Approx. msg: ERROR_DS_RANGE_CONSTRAINT - A value for the attribute was not in the acceptable range of values. */
#define ERROR_DS_RANGE_CONSTRAINT	((ULONG)0x00002082L)

/* MessageId  : 0x00002083 */
/* Approx. msg: ERROR_DS_ATT_VAL_ALREADY_EXISTS - The specified value already exists. */
#define ERROR_DS_ATT_VAL_ALREADY_EXISTS	((ULONG)0x00002083L)

/* MessageId  : 0x00002084 */
/* Approx. msg: ERROR_DS_CANT_REM_MISSING_ATT - The attribute cannot be removed because it is not present on the object. */
#define ERROR_DS_CANT_REM_MISSING_ATT	((ULONG)0x00002084L)

/* MessageId  : 0x00002085 */
/* Approx. msg: ERROR_DS_CANT_REM_MISSING_ATT_VAL - The attribute value cannot be removed because it is not present on the object. */
#define ERROR_DS_CANT_REM_MISSING_ATT_VAL	((ULONG)0x00002085L)

/* MessageId  : 0x00002086 */
/* Approx. msg: ERROR_DS_ROOT_CANT_BE_SUBREF - The specified root object cannot be a subref. */
#define ERROR_DS_ROOT_CANT_BE_SUBREF	((ULONG)0x00002086L)

/* MessageId  : 0x00002087 */
/* Approx. msg: ERROR_DS_NO_CHAINING - Chaining is not permitted. */
#define ERROR_DS_NO_CHAINING	((ULONG)0x00002087L)

/* MessageId  : 0x00002088 */
/* Approx. msg: ERROR_DS_NO_CHAINED_EVAL - Chained evaluation is not permitted. */
#define ERROR_DS_NO_CHAINED_EVAL	((ULONG)0x00002088L)

/* MessageId  : 0x00002089 */
/* Approx. msg: ERROR_DS_NO_PARENT_OBJECT - The operation could not be performed because the object's parent is either uninstantiated or deleted. */
#define ERROR_DS_NO_PARENT_OBJECT	((ULONG)0x00002089L)

/* MessageId  : 0x0000208a */
/* Approx. msg: ERROR_DS_PARENT_IS_AN_ALIAS - Having a parent that is an alias is not permitted. Aliases are leaf objects. */
#define ERROR_DS_PARENT_IS_AN_ALIAS	((ULONG)0x0000208aL)

/* MessageId  : 0x0000208b */
/* Approx. msg: ERROR_DS_CANT_MIX_MASTER_AND_REPS - The object and parent must be of the same type, either both masters or both replicas. */
#define ERROR_DS_CANT_MIX_MASTER_AND_REPS	((ULONG)0x0000208bL)

/* MessageId  : 0x0000208c */
/* Approx. msg: ERROR_DS_CHILDREN_EXIST - The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object. */
#define ERROR_DS_CHILDREN_EXIST	((ULONG)0x0000208cL)

/* MessageId  : 0x0000208d */
/* Approx. msg: ERROR_DS_OBJ_NOT_FOUND - Directory object not found. */
#define ERROR_DS_OBJ_NOT_FOUND	((ULONG)0x0000208dL)

/* MessageId  : 0x0000208e */
/* Approx. msg: ERROR_DS_ALIASED_OBJ_MISSING - The aliased object is missing. */
#define ERROR_DS_ALIASED_OBJ_MISSING	((ULONG)0x0000208eL)

/* MessageId  : 0x0000208f */
/* Approx. msg: ERROR_DS_BAD_NAME_SYNTAX - The object name has bad syntax. */
#define ERROR_DS_BAD_NAME_SYNTAX	((ULONG)0x0000208fL)

/* MessageId  : 0x00002090 */
/* Approx. msg: ERROR_DS_ALIAS_POINTS_TO_ALIAS - It is not permitted for an alias to refer to another alias. */
#define ERROR_DS_ALIAS_POINTS_TO_ALIAS	((ULONG)0x00002090L)

/* MessageId  : 0x00002091 */
/* Approx. msg: ERROR_DS_CANT_DEREF_ALIAS - The alias cannot be dereferenced. */
#define ERROR_DS_CANT_DEREF_ALIAS	((ULONG)0x00002091L)

/* MessageId  : 0x00002092 */
/* Approx. msg: ERROR_DS_OUT_OF_SCOPE - The operation is out of scope. */
#define ERROR_DS_OUT_OF_SCOPE	((ULONG)0x00002092L)

/* MessageId  : 0x00002093 */
/* Approx. msg: ERROR_DS_OBJECT_BEING_REMOVED - The operation cannot continue because the object is in the process of being removed. */
#define ERROR_DS_OBJECT_BEING_REMOVED	((ULONG)0x00002093L)

/* MessageId  : 0x00002094 */
/* Approx. msg: ERROR_DS_CANT_DELETE_DSA_OBJ - The DSA object cannot be deleted. */
#define ERROR_DS_CANT_DELETE_DSA_OBJ	((ULONG)0x00002094L)

/* MessageId  : 0x00002095 */
/* Approx. msg: ERROR_DS_GENERIC_ERROR - A directory service error has occurred. */
#define ERROR_DS_GENERIC_ERROR	((ULONG)0x00002095L)

/* MessageId  : 0x00002096 */
/* Approx. msg: ERROR_DS_DSA_MUST_BE_INT_MASTER - The operation can only be performed on an internal master DSA object. */
#define ERROR_DS_DSA_MUST_BE_INT_MASTER	((ULONG)0x00002096L)

/* MessageId  : 0x00002097 */
/* Approx. msg: ERROR_DS_CLASS_NOT_DSA - The object must be of class DSA. */
#define ERROR_DS_CLASS_NOT_DSA	((ULONG)0x00002097L)

/* MessageId  : 0x00002098 */
/* Approx. msg: ERROR_DS_INSUFF_ACCESS_RIGHTS - Insufficient access rights to perform the operation. */
#define ERROR_DS_INSUFF_ACCESS_RIGHTS	((ULONG)0x00002098L)

/* MessageId  : 0x00002099 */
/* Approx. msg: ERROR_DS_ILLEGAL_SUPERIOR - The object cannot be added because the parent is not on the list of possible superiors. */
#define ERROR_DS_ILLEGAL_SUPERIOR	((ULONG)0x00002099L)

/* MessageId  : 0x0000209a */
/* Approx. msg: ERROR_DS_ATTRIBUTE_OWNED_BY_SAM - Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM). */
#define ERROR_DS_ATTRIBUTE_OWNED_BY_SAM	((ULONG)0x0000209aL)

/* MessageId  : 0x0000209b */
/* Approx. msg: ERROR_DS_NAME_TOO_MANY_PARTS - The name has too many parts. */
#define ERROR_DS_NAME_TOO_MANY_PARTS	((ULONG)0x0000209bL)

/* MessageId  : 0x0000209c */
/* Approx. msg: ERROR_DS_NAME_TOO_LONG - The name is too long. */
#define ERROR_DS_NAME_TOO_LONG	((ULONG)0x0000209cL)

/* MessageId  : 0x0000209d */
/* Approx. msg: ERROR_DS_NAME_VALUE_TOO_LONG - The name value is too long. */
#define ERROR_DS_NAME_VALUE_TOO_LONG	((ULONG)0x0000209dL)

/* MessageId  : 0x0000209e */
/* Approx. msg: ERROR_DS_NAME_UNPARSEABLE - The directory service encountered an error parsing a name. */
#define ERROR_DS_NAME_UNPARSEABLE	((ULONG)0x0000209eL)

/* MessageId  : 0x0000209f */
/* Approx. msg: ERROR_DS_NAME_TYPE_UNKNOWN - The directory service cannot get the attribute type for a name. */
#define ERROR_DS_NAME_TYPE_UNKNOWN	((ULONG)0x0000209fL)

/* MessageId  : 0x000020a0 */
/* Approx. msg: ERROR_DS_NOT_AN_OBJECT - The name does not identify an object; the name identifies a phantom. */
#define ERROR_DS_NOT_AN_OBJECT	((ULONG)0x000020a0L)

/* MessageId  : 0x000020a1 */
/* Approx. msg: ERROR_DS_SEC_DESC_TOO_SHORT - The security descriptor is too short. */
#define ERROR_DS_SEC_DESC_TOO_SHORT	((ULONG)0x000020a1L)

/* MessageId  : 0x000020a2 */
/* Approx. msg: ERROR_DS_SEC_DESC_INVALID - The security descriptor is invalid. */
#define ERROR_DS_SEC_DESC_INVALID	((ULONG)0x000020a2L)

/* MessageId  : 0x000020a3 */
/* Approx. msg: ERROR_DS_NO_DELETED_NAME - Failed to create name for deleted object. */
#define ERROR_DS_NO_DELETED_NAME	((ULONG)0x000020a3L)

/* MessageId  : 0x000020a4 */
/* Approx. msg: ERROR_DS_SUBREF_MUST_HAVE_PARENT - The parent of a new subref must exist. */
#define ERROR_DS_SUBREF_MUST_HAVE_PARENT	((ULONG)0x000020a4L)

/* MessageId  : 0x000020a5 */
/* Approx. msg: ERROR_DS_NCNAME_MUST_BE_NC - The object must be a naming context. */
#define ERROR_DS_NCNAME_MUST_BE_NC	((ULONG)0x000020a5L)

/* MessageId  : 0x000020a6 */
/* Approx. msg: ERROR_DS_CANT_ADD_SYSTEM_ONLY - It is not permitted to add an attribute which is owned by the system. */
#define ERROR_DS_CANT_ADD_SYSTEM_ONLY	((ULONG)0x000020a6L)

/* MessageId  : 0x000020a7 */
/* Approx. msg: ERROR_DS_CLASS_MUST_BE_CONCRETE - The class of the object must be structural; you cannot instantiate an abstract class. */
#define ERROR_DS_CLASS_MUST_BE_CONCRETE	((ULONG)0x000020a7L)

/* MessageId  : 0x000020a8 */
/* Approx. msg: ERROR_DS_INVALID_DMD - The schema object could not be found. */
#define ERROR_DS_INVALID_DMD	((ULONG)0x000020a8L)

/* MessageId  : 0x000020a9 */
/* Approx. msg: ERROR_DS_OBJ_GUID_EXISTS - A local object with this GUID (dead or alive) already exists. */
#define ERROR_DS_OBJ_GUID_EXISTS	((ULONG)0x000020a9L)

/* MessageId  : 0x000020aa */
/* Approx. msg: ERROR_DS_NOT_ON_BACKLINK - The operation cannot be performed on a back link. */
#define ERROR_DS_NOT_ON_BACKLINK	((ULONG)0x000020aaL)

/* MessageId  : 0x000020ab */
/* Approx. msg: ERROR_DS_NO_CROSSREF_FOR_NC - The cross reference for the specified naming context could not be found. */
#define ERROR_DS_NO_CROSSREF_FOR_NC	((ULONG)0x000020abL)

/* MessageId  : 0x000020ac */
/* Approx. msg: ERROR_DS_SHUTTING_DOWN - The operation could not be performed because the directory service is shutting down. */
#define ERROR_DS_SHUTTING_DOWN	((ULONG)0x000020acL)

/* MessageId  : 0x000020ad */
/* Approx. msg: ERROR_DS_UNKNOWN_OPERATION - The directory service request is invalid. */
#define ERROR_DS_UNKNOWN_OPERATION	((ULONG)0x000020adL)

/* MessageId  : 0x000020ae */
/* Approx. msg: ERROR_DS_INVALID_ROLE_OWNER - The role owner attribute could not be read. */
#define ERROR_DS_INVALID_ROLE_OWNER	((ULONG)0x000020aeL)

/* MessageId  : 0x000020af */
/* Approx. msg: ERROR_DS_COULDNT_CONTACT_FSMO - The requested FSMO operation failed. The current FSMO holder could not be reached. */
#define ERROR_DS_COULDNT_CONTACT_FSMO	((ULONG)0x000020afL)

/* MessageId  : 0x000020b0 */
/* Approx. msg: ERROR_DS_CROSS_NC_DN_RENAME - Modification of a DN across a naming context is not permitted. */
#define ERROR_DS_CROSS_NC_DN_RENAME	((ULONG)0x000020b0L)

/* MessageId  : 0x000020b1 */
/* Approx. msg: ERROR_DS_CANT_MOD_SYSTEM_ONLY - The attribute cannot be modified because it is owned by the system. */
#define ERROR_DS_CANT_MOD_SYSTEM_ONLY	((ULONG)0x000020b1L)

/* MessageId  : 0x000020b2 */
/* Approx. msg: ERROR_DS_REPLICATOR_ONLY - Only the replicator can perform this function. */
#define ERROR_DS_REPLICATOR_ONLY	((ULONG)0x000020b2L)

/* MessageId  : 0x000020b3 */
/* Approx. msg: ERROR_DS_OBJ_CLASS_NOT_DEFINED - The specified class is not defined. */
#define ERROR_DS_OBJ_CLASS_NOT_DEFINED	((ULONG)0x000020b3L)

/* MessageId  : 0x000020b4 */
/* Approx. msg: ERROR_DS_OBJ_CLASS_NOT_SUBCLASS - The specified class is not a subclass. */
#define ERROR_DS_OBJ_CLASS_NOT_SUBCLASS	((ULONG)0x000020b4L)

/* MessageId  : 0x000020b5 */
/* Approx. msg: ERROR_DS_NAME_REFERENCE_INVALID - The name reference is invalid. */
#define ERROR_DS_NAME_REFERENCE_INVALID	((ULONG)0x000020b5L)

/* MessageId  : 0x000020b6 */
/* Approx. msg: ERROR_DS_CROSS_REF_EXISTS - A cross reference already exists. */
#define ERROR_DS_CROSS_REF_EXISTS	((ULONG)0x000020b6L)

/* MessageId  : 0x000020b7 */
/* Approx. msg: ERROR_DS_CANT_DEL_MASTER_CROSSREF - It is not permitted to delete a master cross reference. */
#define ERROR_DS_CANT_DEL_MASTER_CROSSREF	((ULONG)0x000020b7L)

/* MessageId  : 0x000020b8 */
/* Approx. msg: ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD - Subtree notifications are only supported on NC heads. */
#define ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD	((ULONG)0x000020b8L)

/* MessageId  : 0x000020b9 */
/* Approx. msg: ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX - Notification filter is too complex. */
#define ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX	((ULONG)0x000020b9L)

/* MessageId  : 0x000020ba */
/* Approx. msg: ERROR_DS_DUP_RDN - Schema update failed: duplicate RDN. */
#define ERROR_DS_DUP_RDN	((ULONG)0x000020baL)

/* MessageId  : 0x000020bb */
/* Approx. msg: ERROR_DS_DUP_OID - Schema update failed: duplicate OID */
#define ERROR_DS_DUP_OID	((ULONG)0x000020bbL)

/* MessageId  : 0x000020bc */
/* Approx. msg: ERROR_DS_DUP_MAPI_ID - Schema update failed: duplicate MAPI identifier. */
#define ERROR_DS_DUP_MAPI_ID	((ULONG)0x000020bcL)

/* MessageId  : 0x000020bd */
/* Approx. msg: ERROR_DS_DUP_SCHEMA_ID_GUID - Schema update failed: duplicate schema-id GUID. */
#define ERROR_DS_DUP_SCHEMA_ID_GUID	((ULONG)0x000020bdL)

/* MessageId  : 0x000020be */
/* Approx. msg: ERROR_DS_DUP_LDAP_DISPLAY_NAME - Schema update failed: duplicate LDAP display name. */
#define ERROR_DS_DUP_LDAP_DISPLAY_NAME	((ULONG)0x000020beL)

/* MessageId  : 0x000020bf */
/* Approx. msg: ERROR_DS_SEMANTIC_ATT_TEST - Schema update failed: range-lower less than range upper */
#define ERROR_DS_SEMANTIC_ATT_TEST	((ULONG)0x000020bfL)

/* MessageId  : 0x000020c0 */
/* Approx. msg: ERROR_DS_SYNTAX_MISMATCH - Schema update failed: syntax mismatch */
#define ERROR_DS_SYNTAX_MISMATCH	((ULONG)0x000020c0L)

/* MessageId  : 0x000020c1 */
/* Approx. msg: ERROR_DS_EXISTS_IN_MUST_HAVE - Schema deletion failed: attribute is used in must-contain */
#define ERROR_DS_EXISTS_IN_MUST_HAVE	((ULONG)0x000020c1L)

/* MessageId  : 0x000020c2 */
/* Approx. msg: ERROR_DS_EXISTS_IN_MAY_HAVE - Schema deletion failed: attribute is used in may-contain */
#define ERROR_DS_EXISTS_IN_MAY_HAVE	((ULONG)0x000020c2L)

/* MessageId  : 0x000020c3 */
/* Approx. msg: ERROR_DS_NONEXISTENT_MAY_HAVE - Schema update failed: attribute in may-contain does not exist */
#define ERROR_DS_NONEXISTENT_MAY_HAVE	((ULONG)0x000020c3L)

/* MessageId  : 0x000020c4 */
/* Approx. msg: ERROR_DS_NONEXISTENT_MUST_HAVE - Schema update failed: attribute in must-contain does not exist */
#define ERROR_DS_NONEXISTENT_MUST_HAVE	((ULONG)0x000020c4L)

/* MessageId  : 0x000020c5 */
/* Approx. msg: ERROR_DS_AUX_CLS_TEST_FAIL - Schema update failed: class in aux-class list does not exist or is not an auxiliary class */
#define ERROR_DS_AUX_CLS_TEST_FAIL	((ULONG)0x000020c5L)

/* MessageId  : 0x000020c6 */
/* Approx. msg: ERROR_DS_NONEXISTENT_POSS_SUP - Schema update failed: class in poss-superiors does not exist */
#define ERROR_DS_NONEXISTENT_POSS_SUP	((ULONG)0x000020c6L)

/* MessageId  : 0x000020c7 */
/* Approx. msg: ERROR_DS_SUB_CLS_TEST_FAIL - Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules */
#define ERROR_DS_SUB_CLS_TEST_FAIL	((ULONG)0x000020c7L)

/* MessageId  : 0x000020c8 */
/* Approx. msg: ERROR_DS_BAD_RDN_ATT_ID_SYNTAX - Schema update failed: Rdn-Att-Id has wrong syntax */
#define ERROR_DS_BAD_RDN_ATT_ID_SYNTAX	((ULONG)0x000020c8L)

/* MessageId  : 0x000020c9 */
/* Approx. msg: ERROR_DS_EXISTS_IN_AUX_CLS - Schema deletion failed: class is used as auxiliary class */
#define ERROR_DS_EXISTS_IN_AUX_CLS	((ULONG)0x000020c9L)

/* MessageId  : 0x000020ca */
/* Approx. msg: ERROR_DS_EXISTS_IN_SUB_CLS - Schema deletion failed: class is used as sub class */
#define ERROR_DS_EXISTS_IN_SUB_CLS	((ULONG)0x000020caL)

/* MessageId  : 0x000020cb */
/* Approx. msg: ERROR_DS_EXISTS_IN_POSS_SUP - Schema deletion failed: class is used as poss superior */
#define ERROR_DS_EXISTS_IN_POSS_SUP	((ULONG)0x000020cbL)

/* MessageId  : 0x000020cc */
/* Approx. msg: ERROR_DS_RECALCSCHEMA_FAILED - Schema update failed in recalculating validation cache. */
#define ERROR_DS_RECALCSCHEMA_FAILED	((ULONG)0x000020ccL)

/* MessageId  : 0x000020cd */
/* Approx. msg: ERROR_DS_TREE_DELETE_NOT_FINISHED - The tree deletion is not finished. */
#define ERROR_DS_TREE_DELETE_NOT_FINISHED	((ULONG)0x000020cdL)

/* MessageId  : 0x000020ce */
/* Approx. msg: ERROR_DS_CANT_DELETE - The requested delete operation could not be performed. */
#define ERROR_DS_CANT_DELETE	((ULONG)0x000020ceL)

/* MessageId  : 0x000020cf */
/* Approx. msg: ERROR_DS_ATT_SCHEMA_REQ_ID - Cannot read the governs class identifier for the schema record. */
#define ERROR_DS_ATT_SCHEMA_REQ_ID	((ULONG)0x000020cfL)

/* MessageId  : 0x000020d0 */
/* Approx. msg: ERROR_DS_BAD_ATT_SCHEMA_SYNTAX - The attribute schema has bad syntax. */
#define ERROR_DS_BAD_ATT_SCHEMA_SYNTAX	((ULONG)0x000020d0L)

/* MessageId  : 0x000020d1 */
/* Approx. msg: ERROR_DS_CANT_CACHE_ATT - The attribute could not be cached. */
#define ERROR_DS_CANT_CACHE_ATT	((ULONG)0x000020d1L)

/* MessageId  : 0x000020d2 */
/* Approx. msg: ERROR_DS_CANT_CACHE_CLASS - The class could not be cached. */
#define ERROR_DS_CANT_CACHE_CLASS	((ULONG)0x000020d2L)

/* MessageId  : 0x000020d3 */
/* Approx. msg: ERROR_DS_CANT_REMOVE_ATT_CACHE - The attribute could not be removed from the cache. */
#define ERROR_DS_CANT_REMOVE_ATT_CACHE	((ULONG)0x000020d3L)

/* MessageId  : 0x000020d4 */
/* Approx. msg: ERROR_DS_CANT_REMOVE_CLASS_CACHE - The class could not be removed from the cache. */
#define ERROR_DS_CANT_REMOVE_CLASS_CACHE	((ULONG)0x000020d4L)

/* MessageId  : 0x000020d5 */
/* Approx. msg: ERROR_DS_CANT_RETRIEVE_DN - The distinguished name attribute could not be read. */
#define ERROR_DS_CANT_RETRIEVE_DN	((ULONG)0x000020d5L)

/* MessageId  : 0x000020d6 */
/* Approx. msg: ERROR_DS_MISSING_SUPREF - No superior reference has been configured for the directory service. The directory service is therefore unable to issue referrals to objects outside this forest. */
#define ERROR_DS_MISSING_SUPREF	((ULONG)0x000020d6L)

/* MessageId  : 0x000020d7 */
/* Approx. msg: ERROR_DS_CANT_RETRIEVE_INSTANCE - The instance type attribute could not be retrieved. */
#define ERROR_DS_CANT_RETRIEVE_INSTANCE	((ULONG)0x000020d7L)

/* MessageId  : 0x000020d8 */
/* Approx. msg: ERROR_DS_CODE_INCONSISTENCY - An internal error has occurred. */
#define ERROR_DS_CODE_INCONSISTENCY	((ULONG)0x000020d8L)

/* MessageId  : 0x000020d9 */
/* Approx. msg: ERROR_DS_DATABASE_ERROR - A database error has occurred. */
#define ERROR_DS_DATABASE_ERROR	((ULONG)0x000020d9L)

/* MessageId  : 0x000020da */
/* Approx. msg: ERROR_DS_GOVERNSID_MISSING - The attribute GOVERNSID is missing. */
#define ERROR_DS_GOVERNSID_MISSING	((ULONG)0x000020daL)

/* MessageId  : 0x000020db */
/* Approx. msg: ERROR_DS_MISSING_EXPECTED_ATT - An expected attribute is missing. */
#define ERROR_DS_MISSING_EXPECTED_ATT	((ULONG)0x000020dbL)

/* MessageId  : 0x000020dc */
/* Approx. msg: ERROR_DS_NCNAME_MISSING_CR_REF - The specified naming context is missing a cross reference. */
#define ERROR_DS_NCNAME_MISSING_CR_REF	((ULONG)0x000020dcL)

/* MessageId  : 0x000020dd */
/* Approx. msg: ERROR_DS_SECURITY_CHECKING_ERROR - A security checking error has occurred. */
#define ERROR_DS_SECURITY_CHECKING_ERROR	((ULONG)0x000020ddL)

/* MessageId  : 0x000020de */
/* Approx. msg: ERROR_DS_SCHEMA_NOT_LOADED - The schema is not loaded. */
#define ERROR_DS_SCHEMA_NOT_LOADED	((ULONG)0x000020deL)

/* MessageId  : 0x000020df */
/* Approx. msg: ERROR_DS_SCHEMA_ALLOC_FAILED - Schema allocation failed. Please check if the machine is running low on memory. */
#define ERROR_DS_SCHEMA_ALLOC_FAILED	((ULONG)0x000020dfL)

/* MessageId  : 0x000020e0 */
/* Approx. msg: ERROR_DS_ATT_SCHEMA_REQ_SYNTAX - Failed to obtain the required syntax for the attribute schema. */
#define ERROR_DS_ATT_SCHEMA_REQ_SYNTAX	((ULONG)0x000020e0L)

/* MessageId  : 0x000020e1 */
/* Approx. msg: ERROR_DS_GCVERIFY_ERROR - The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available. */
#define ERROR_DS_GCVERIFY_ERROR	((ULONG)0x000020e1L)

/* MessageId  : 0x000020e2 */
/* Approx. msg: ERROR_DS_DRA_SCHEMA_MISMATCH - The replication operation failed because of a schema mismatch between the servers involved. */
#define ERROR_DS_DRA_SCHEMA_MISMATCH	((ULONG)0x000020e2L)

/* MessageId  : 0x000020e3 */
/* Approx. msg: ERROR_DS_CANT_FIND_DSA_OBJ - The DSA object could not be found. */
#define ERROR_DS_CANT_FIND_DSA_OBJ	((ULONG)0x000020e3L)

/* MessageId  : 0x000020e4 */
/* Approx. msg: ERROR_DS_CANT_FIND_EXPECTED_NC - The naming context could not be found. */
#define ERROR_DS_CANT_FIND_EXPECTED_NC	((ULONG)0x000020e4L)

/* MessageId  : 0x000020e5 */
/* Approx. msg: ERROR_DS_CANT_FIND_NC_IN_CACHE - The naming context could not be found in the cache. */
#define ERROR_DS_CANT_FIND_NC_IN_CACHE	((ULONG)0x000020e5L)

/* MessageId  : 0x000020e6 */
/* Approx. msg: ERROR_DS_CANT_RETRIEVE_CHILD - The child object could not be retrieved. */
#define ERROR_DS_CANT_RETRIEVE_CHILD	((ULONG)0x000020e6L)

/* MessageId  : 0x000020e7 */
/* Approx. msg: ERROR_DS_SECURITY_ILLEGAL_MODIFY - The modification was not permitted for security reasons. */
#define ERROR_DS_SECURITY_ILLEGAL_MODIFY	((ULONG)0x000020e7L)

/* MessageId  : 0x000020e8 */
/* Approx. msg: ERROR_DS_CANT_REPLACE_HIDDEN_REC - The operation cannot replace the hidden record. */
#define ERROR_DS_CANT_REPLACE_HIDDEN_REC	((ULONG)0x000020e8L)

/* MessageId  : 0x000020e9 */
/* Approx. msg: ERROR_DS_BAD_HIERARCHY_FILE - The hierarchy file is invalid. */
#define ERROR_DS_BAD_HIERARCHY_FILE	((ULONG)0x000020e9L)

/* MessageId  : 0x000020ea */
/* Approx. msg: ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED - The attempt to build the hierarchy table failed. */
#define ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED	((ULONG)0x000020eaL)

/* MessageId  : 0x000020eb */
/* Approx. msg: ERROR_DS_CONFIG_PARAM_MISSING - The directory configuration parameter is missing from the registry. */
#define ERROR_DS_CONFIG_PARAM_MISSING	((ULONG)0x000020ebL)

/* MessageId  : 0x000020ec */
/* Approx. msg: ERROR_DS_COUNTING_AB_INDICES_FAILED - The attempt to count the address book indices failed. */
#define ERROR_DS_COUNTING_AB_INDICES_FAILED	((ULONG)0x000020ecL)

/* MessageId  : 0x000020ed */
/* Approx. msg: ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED - The allocation of the hierarchy table failed. */
#define ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED	((ULONG)0x000020edL)

/* MessageId  : 0x000020ee */
/* Approx. msg: ERROR_DS_INTERNAL_FAILURE - The directory service encountered an internal failure. */
#define ERROR_DS_INTERNAL_FAILURE	((ULONG)0x000020eeL)

/* MessageId  : 0x000020ef */
/* Approx. msg: ERROR_DS_UNKNOWN_ERROR - The directory service encountered an unknown failure. */
#define ERROR_DS_UNKNOWN_ERROR	((ULONG)0x000020efL)

/* MessageId  : 0x000020f0 */
/* Approx. msg: ERROR_DS_ROOT_REQUIRES_CLASS_TOP - A root object requires a class of 'top'. */
#define ERROR_DS_ROOT_REQUIRES_CLASS_TOP	((ULONG)0x000020f0L)

/* MessageId  : 0x000020f1 */
/* Approx. msg: ERROR_DS_REFUSING_FSMO_ROLES - This directory server is shutting down, and cannot take ownership of new floating single-master operation roles. */
#define ERROR_DS_REFUSING_FSMO_ROLES	((ULONG)0x000020f1L)

/* MessageId  : 0x000020f2 */
/* Approx. msg: ERROR_DS_MISSING_FSMO_SETTINGS - The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles. */
#define ERROR_DS_MISSING_FSMO_SETTINGS	((ULONG)0x000020f2L)

/* MessageId  : 0x000020f3 */
/* Approx. msg: ERROR_DS_UNABLE_TO_SURRENDER_ROLES - The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers. */
#define ERROR_DS_UNABLE_TO_SURRENDER_ROLES	((ULONG)0x000020f3L)

/* MessageId  : 0x000020f4 */
/* Approx. msg: ERROR_DS_DRA_GENERIC - The replication operation failed. */
#define ERROR_DS_DRA_GENERIC	((ULONG)0x000020f4L)

/* MessageId  : 0x000020f5 */
/* Approx. msg: ERROR_DS_DRA_INVALID_PARAMETER - An invalid parameter was specified for this replication operation. */
#define ERROR_DS_DRA_INVALID_PARAMETER	((ULONG)0x000020f5L)

/* MessageId  : 0x000020f6 */
/* Approx. msg: ERROR_DS_DRA_BUSY - The directory service is too busy to complete the replication operation at this time. */
#define ERROR_DS_DRA_BUSY	((ULONG)0x000020f6L)

/* MessageId  : 0x000020f7 */
/* Approx. msg: ERROR_DS_DRA_BAD_DN - The distinguished name specified for this replication operation is invalid. */
#define ERROR_DS_DRA_BAD_DN	((ULONG)0x000020f7L)

/* MessageId  : 0x000020f8 */
/* Approx. msg: ERROR_DS_DRA_BAD_NC - The naming context specified for this replication operation is invalid. */
#define ERROR_DS_DRA_BAD_NC	((ULONG)0x000020f8L)

/* MessageId  : 0x000020f9 */
/* Approx. msg: ERROR_DS_DRA_DN_EXISTS - The distinguished name specified for this replication operation already exists. */
#define ERROR_DS_DRA_DN_EXISTS	((ULONG)0x000020f9L)

/* MessageId  : 0x000020fa */
/* Approx. msg: ERROR_DS_DRA_INTERNAL_ERROR - The replication system encountered an internal error. */
#define ERROR_DS_DRA_INTERNAL_ERROR	((ULONG)0x000020faL)

/* MessageId  : 0x000020fb */
/* Approx. msg: ERROR_DS_DRA_INCONSISTENT_DIT - The replication operation encountered a database inconsistency. */
#define ERROR_DS_DRA_INCONSISTENT_DIT	((ULONG)0x000020fbL)

/* MessageId  : 0x000020fc */
/* Approx. msg: ERROR_DS_DRA_CONNECTION_FAILED - The server specified for this replication operation could not be contacted. */
#define ERROR_DS_DRA_CONNECTION_FAILED	((ULONG)0x000020fcL)

/* MessageId  : 0x000020fd */
/* Approx. msg: ERROR_DS_DRA_BAD_INSTANCE_TYPE - The replication operation encountered an object with an invalid instance type. */
#define ERROR_DS_DRA_BAD_INSTANCE_TYPE	((ULONG)0x000020fdL)

/* MessageId  : 0x000020fe */
/* Approx. msg: ERROR_DS_DRA_OUT_OF_MEM - The replication operation failed to allocate memory. */
#define ERROR_DS_DRA_OUT_OF_MEM	((ULONG)0x000020feL)

/* MessageId  : 0x000020ff */
/* Approx. msg: ERROR_DS_DRA_MAIL_PROBLEM - The replication operation encountered an error with the mail system. */
#define ERROR_DS_DRA_MAIL_PROBLEM	((ULONG)0x000020ffL)

/* MessageId  : 0x00002100 */
/* Approx. msg: ERROR_DS_DRA_REF_ALREADY_EXISTS - The replication reference information for the target server already exists. */
#define ERROR_DS_DRA_REF_ALREADY_EXISTS	((ULONG)0x00002100L)

/* MessageId  : 0x00002101 */
/* Approx. msg: ERROR_DS_DRA_REF_NOT_FOUND - The replication reference information for the target server does not exist. */
#define ERROR_DS_DRA_REF_NOT_FOUND	((ULONG)0x00002101L)

/* MessageId  : 0x00002102 */
/* Approx. msg: ERROR_DS_DRA_OBJ_IS_REP_SOURCE - The naming context cannot be removed because it is replicated to another server. */
#define ERROR_DS_DRA_OBJ_IS_REP_SOURCE	((ULONG)0x00002102L)

/* MessageId  : 0x00002103 */
/* Approx. msg: ERROR_DS_DRA_DB_ERROR - The replication operation encountered a database error. */
#define ERROR_DS_DRA_DB_ERROR	((ULONG)0x00002103L)

/* MessageId  : 0x00002104 */
/* Approx. msg: ERROR_DS_DRA_NO_REPLICA - The naming context is in the process of being removed or is not replicated from the specified server. */
#define ERROR_DS_DRA_NO_REPLICA	((ULONG)0x00002104L)

/* MessageId  : 0x00002105 */
/* Approx. msg: ERROR_DS_DRA_ACCESS_DENIED - Replication access was denied. */
#define ERROR_DS_DRA_ACCESS_DENIED	((ULONG)0x00002105L)

/* MessageId  : 0x00002106 */
/* Approx. msg: ERROR_DS_DRA_NOT_SUPPORTED - The requested operation is not supported by this version of the directory service. */
#define ERROR_DS_DRA_NOT_SUPPORTED	((ULONG)0x00002106L)

/* MessageId  : 0x00002107 */
/* Approx. msg: ERROR_DS_DRA_RPC_CANCELLED - The replication remote procedure call was cancelled. */
#define ERROR_DS_DRA_RPC_CANCELLED	((ULONG)0x00002107L)

/* MessageId  : 0x00002108 */
/* Approx. msg: ERROR_DS_DRA_SOURCE_DISABLED - The source server is currently rejecting replication requests. */
#define ERROR_DS_DRA_SOURCE_DISABLED	((ULONG)0x00002108L)

/* MessageId  : 0x00002109 */
/* Approx. msg: ERROR_DS_DRA_SINK_DISABLED - The destination server is currently rejecting replication requests. */
#define ERROR_DS_DRA_SINK_DISABLED	((ULONG)0x00002109L)

/* MessageId  : 0x0000210a */
/* Approx. msg: ERROR_DS_DRA_NAME_COLLISION - The replication operation failed due to a collision of object names. */
#define ERROR_DS_DRA_NAME_COLLISION	((ULONG)0x0000210aL)

/* MessageId  : 0x0000210b */
/* Approx. msg: ERROR_DS_DRA_SOURCE_REINSTALLED - The replication source has been reinstalled. */
#define ERROR_DS_DRA_SOURCE_REINSTALLED	((ULONG)0x0000210bL)

/* MessageId  : 0x0000210c */
/* Approx. msg: ERROR_DS_DRA_MISSING_PARENT - The replication operation failed because a required parent object is missing. */
#define ERROR_DS_DRA_MISSING_PARENT	((ULONG)0x0000210cL)

/* MessageId  : 0x0000210d */
/* Approx. msg: ERROR_DS_DRA_PREEMPTED - The replication operation was preempted. */
#define ERROR_DS_DRA_PREEMPTED	((ULONG)0x0000210dL)

/* MessageId  : 0x0000210e */
/* Approx. msg: ERROR_DS_DRA_ABANDON_SYNC - The replication synchronization attempt was abandoned because of a lack of updates. */
#define ERROR_DS_DRA_ABANDON_SYNC	((ULONG)0x0000210eL)

/* MessageId  : 0x0000210f */
/* Approx. msg: ERROR_DS_DRA_SHUTDOWN - The replication operation was terminated because the system is shutting down. */
#define ERROR_DS_DRA_SHUTDOWN	((ULONG)0x0000210fL)

/* MessageId  : 0x00002110 */
/* Approx. msg: ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET - Synchronization attempt failed because the destination DC is currently waiting to synchronize new partial attributes from source. This condition is normal if a recent schema change modified the partial attribute set. The destination partial attribute set is not a subset of the source partial attribute set. */
#define ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET	((ULONG)0x00002110L)

/* MessageId  : 0x00002111 */
/* Approx. msg: ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA - The replication synchronization attempt failed because a master replica attempted to sync from a partial replica. */
#define ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA	((ULONG)0x00002111L)

/* MessageId  : 0x00002112 */
/* Approx. msg: ERROR_DS_DRA_EXTN_CONNECTION_FAILED - The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation. */
#define ERROR_DS_DRA_EXTN_CONNECTION_FAILED	((ULONG)0x00002112L)

/* MessageId  : 0x00002113 */
/* Approx. msg: ERROR_DS_INSTALL_SCHEMA_MISMATCH - The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer. */
#define ERROR_DS_INSTALL_SCHEMA_MISMATCH	((ULONG)0x00002113L)

/* MessageId  : 0x00002114 */
/* Approx. msg: ERROR_DS_DUP_LINK_ID - Schema update failed: An attribute with the same link identifier already exists. */
#define ERROR_DS_DUP_LINK_ID	((ULONG)0x00002114L)

/* MessageId  : 0x00002115 */
/* Approx. msg: ERROR_DS_NAME_ERROR_RESOLVING - Name translation: Generic processing error. */
#define ERROR_DS_NAME_ERROR_RESOLVING	((ULONG)0x00002115L)

/* MessageId  : 0x00002116 */
/* Approx. msg: ERROR_DS_NAME_ERROR_NOT_FOUND - Name translation: Could not find the name or insufficient right to see name. */
#define ERROR_DS_NAME_ERROR_NOT_FOUND	((ULONG)0x00002116L)

/* MessageId  : 0x00002117 */
/* Approx. msg: ERROR_DS_NAME_ERROR_NOT_UNIQUE - Name translation: Input name mapped to more than one output name. */
#define ERROR_DS_NAME_ERROR_NOT_UNIQUE	((ULONG)0x00002117L)

/* MessageId  : 0x00002118 */
/* Approx. msg: ERROR_DS_NAME_ERROR_NO_MAPPING - Name translation: Input name found, but not the associated output format. */
#define ERROR_DS_NAME_ERROR_NO_MAPPING	((ULONG)0x00002118L)

/* MessageId  : 0x00002119 */
/* Approx. msg: ERROR_DS_NAME_ERROR_DOMAIN_ONLY - Name translation: Unable to resolve completely, only the domain was found. */
#define ERROR_DS_NAME_ERROR_DOMAIN_ONLY	((ULONG)0x00002119L)

/* MessageId  : 0x0000211a */
/* Approx. msg: ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING - Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire. */
#define ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING	((ULONG)0x0000211aL)

/* MessageId  : 0x0000211b */
/* Approx. msg: ERROR_DS_CONSTRUCTED_ATT_MOD - Modification of a constructed attribute is not allowed. */
#define ERROR_DS_CONSTRUCTED_ATT_MOD	((ULONG)0x0000211bL)

/* MessageId  : 0x0000211c */
/* Approx. msg: ERROR_DS_WRONG_OM_OBJ_CLASS - The OM-Object-Class specified is incorrect for an attribute with the specified syntax. */
#define ERROR_DS_WRONG_OM_OBJ_CLASS	((ULONG)0x0000211cL)

/* MessageId  : 0x0000211d */
/* Approx. msg: ERROR_DS_DRA_REPL_PENDING - The replication request has been posted; waiting for reply. */
#define ERROR_DS_DRA_REPL_PENDING	((ULONG)0x0000211dL)

/* MessageId  : 0x0000211e */
/* Approx. msg: ERROR_DS_DS_REQUIRED - The requested operation requires a directory service, and none was available. */
#define ERROR_DS_DS_REQUIRED	((ULONG)0x0000211eL)

/* MessageId  : 0x0000211f */
/* Approx. msg: ERROR_DS_INVALID_LDAP_DISPLAY_NAME - The LDAP display name of the class or attribute contains non-ASCII characters. */
#define ERROR_DS_INVALID_LDAP_DISPLAY_NAME	((ULONG)0x0000211fL)

/* MessageId  : 0x00002120 */
/* Approx. msg: ERROR_DS_NON_BASE_SEARCH - The requested search operation is only supported for base searches. */
#define ERROR_DS_NON_BASE_SEARCH	((ULONG)0x00002120L)

/* MessageId  : 0x00002121 */
/* Approx. msg: ERROR_DS_CANT_RETRIEVE_ATTS - The search failed to retrieve attributes from the database. */
#define ERROR_DS_CANT_RETRIEVE_ATTS	((ULONG)0x00002121L)

/* MessageId  : 0x00002122 */
/* Approx. msg: ERROR_DS_BACKLINK_WITHOUT_LINK - The schema update operation tried to add a backward link attribute that has no corresponding forward link. */
#define ERROR_DS_BACKLINK_WITHOUT_LINK	((ULONG)0x00002122L)

/* MessageId  : 0x00002123 */
/* Approx. msg: ERROR_DS_EPOCH_MISMATCH - Source and destination of a cross domain move do not agree on the object's epoch number. Either source or destination does not have the latest version of the object. */
#define ERROR_DS_EPOCH_MISMATCH	((ULONG)0x00002123L)

/* MessageId  : 0x00002124 */
/* Approx. msg: ERROR_DS_SRC_NAME_MISMATCH - Source and destination of a cross domain move do not agree on the object's current name. Either source or destination does not have the latest version of the object. */
#define ERROR_DS_SRC_NAME_MISMATCH	((ULONG)0x00002124L)

/* MessageId  : 0x00002125 */
/* Approx. msg: ERROR_DS_SRC_AND_DST_NC_IDENTICAL - Source and destination of a cross domain move operation are identical. Caller should use local move operation instead of cross domain move operation. */
#define ERROR_DS_SRC_AND_DST_NC_IDENTICAL	((ULONG)0x00002125L)

/* MessageId  : 0x00002126 */
/* Approx. msg: ERROR_DS_DST_NC_MISMATCH - Source and destination for a cross domain move are not in agreement on the naming contexts in the forest. Either source or destination does not have the latest version of the Partitions container. */
#define ERROR_DS_DST_NC_MISMATCH	((ULONG)0x00002126L)

/* MessageId  : 0x00002127 */
/* Approx. msg: ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC - Destination of a cross domain move is not authoritative for the destination naming context. */
#define ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC	((ULONG)0x00002127L)

/* MessageId  : 0x00002128 */
/* Approx. msg: ERROR_DS_SRC_GUID_MISMATCH - Source and destination of a cross domain move do not agree on the identity of the source object. Either source or destination does not have the latest version of the source object. */
#define ERROR_DS_SRC_GUID_MISMATCH	((ULONG)0x00002128L)

/* MessageId  : 0x00002129 */
/* Approx. msg: ERROR_DS_CANT_MOVE_DELETED_OBJECT - Object being moved across domains is already known to be deleted by the destination server. The source server does not have the latest version of the source object. */
#define ERROR_DS_CANT_MOVE_DELETED_OBJECT	((ULONG)0x00002129L)

/* MessageId  : 0x0000212a */
/* Approx. msg: ERROR_DS_PDC_OPERATION_IN_PROGRESS - Another operation, which requires exclusive access to the PDC PSMO, is already in progress. */
#define ERROR_DS_PDC_OPERATION_IN_PROGRESS	((ULONG)0x0000212aL)

/* MessageId  : 0x0000212b */
/* Approx. msg: ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD - A cross domain move operation failed such that the two versions of the moved object exist - one each in the source and destination domains. The destination object needs to be removed to restore the system to a consistent state. */
#define ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD	((ULONG)0x0000212bL)

/* MessageId  : 0x0000212c */
/* Approx. msg: ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION - This object may not be moved across domain boundaries either because cross domain moves for this class are disallowed, or the object has some special characteristics, e.g.: trust account or restricted RID, which prevent its move. */
#define ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION	((ULONG)0x0000212cL)

/* MessageId  : 0x0000212d */
/* Approx. msg: ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS - Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group. Remove the object from any account group memberships and retry. */
#define ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS	((ULONG)0x0000212dL)

/* MessageId  : 0x0000212e */
/* Approx. msg: ERROR_DS_NC_MUST_HAVE_NC_PARENT - A naming context head must be the immediate child of another naming context head, not of an interior node. */
#define ERROR_DS_NC_MUST_HAVE_NC_PARENT	((ULONG)0x0000212eL)

/* MessageId  : 0x0000212f */
/* Approx. msg: ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE - The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context. Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters) */
#define ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE	((ULONG)0x0000212fL)

/* MessageId  : 0x00002130 */
/* Approx. msg: ERROR_DS_DST_DOMAIN_NOT_NATIVE - Destination domain must be in native mode. */
#define ERROR_DS_DST_DOMAIN_NOT_NATIVE	((ULONG)0x00002130L)

/* MessageId  : 0x00002131 */
/* Approx. msg: ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER - The operation cannot be performed because the server does not have an infrastructure container in the domain of interest. */
#define ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER	((ULONG)0x00002131L)

/* MessageId  : 0x00002132 */
/* Approx. msg: ERROR_DS_CANT_MOVE_ACCOUNT_GROUP - Cross-domain move of non-empty account groups is not allowed. */
#define ERROR_DS_CANT_MOVE_ACCOUNT_GROUP	((ULONG)0x00002132L)

/* MessageId  : 0x00002133 */
/* Approx. msg: ERROR_DS_CANT_MOVE_RESOURCE_GROUP - Cross-domain move of non-empty resource groups is not allowed. */
#define ERROR_DS_CANT_MOVE_RESOURCE_GROUP	((ULONG)0x00002133L)

/* MessageId  : 0x00002134 */
/* Approx. msg: ERROR_DS_INVALID_SEARCH_FLAG - The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings. */
#define ERROR_DS_INVALID_SEARCH_FLAG	((ULONG)0x00002134L)

/* MessageId  : 0x00002135 */
/* Approx. msg: ERROR_DS_NO_TREE_DELETE_ABOVE_NC - Tree deletions starting at an object which has an NC head as a descendant are not allowed. */
#define ERROR_DS_NO_TREE_DELETE_ABOVE_NC	((ULONG)0x00002135L)

/* MessageId  : 0x00002136 */
/* Approx. msg: ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE - The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use. */
#define ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE	((ULONG)0x00002136L)

/* MessageId  : 0x00002137 */
/* Approx. msg: ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE - The directory service failed to identify the list of objects to delete while attempting a tree deletion. */
#define ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE	((ULONG)0x00002137L)

/* MessageId  : 0x00002138 */
/* Approx. msg: ERROR_DS_SAM_INIT_FAILURE - Security Accounts Manager initialization failed because of the following error: %1. */
#define ERROR_DS_SAM_INIT_FAILURE	((ULONG)0x00002138L)

/* MessageId  : 0x00002139 */
/* Approx. msg: ERROR_DS_SENSITIVE_GROUP_VIOLATION - Only an administrator can modify the membership list of an administrative group. */
#define ERROR_DS_SENSITIVE_GROUP_VIOLATION	((ULONG)0x00002139L)

/* MessageId  : 0x0000213a */
/* Approx. msg: ERROR_DS_CANT_MOD_PRIMARYGROUPID - Cannot change the primary group ID of a domain controller account. */
#define ERROR_DS_CANT_MOD_PRIMARYGROUPID	((ULONG)0x0000213aL)

/* MessageId  : 0x0000213b */
/* Approx. msg: ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD - An attempt is made to modify the base schema. */
#define ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD	((ULONG)0x0000213bL)

/* MessageId  : 0x0000213c */
/* Approx. msg: ERROR_DS_NONSAFE_SCHEMA_CHANGE - Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed. */
#define ERROR_DS_NONSAFE_SCHEMA_CHANGE	((ULONG)0x0000213cL)

/* MessageId  : 0x0000213d */
/* Approx. msg: ERROR_DS_SCHEMA_UPDATE_DISALLOWED - Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner. */
#define ERROR_DS_SCHEMA_UPDATE_DISALLOWED	((ULONG)0x0000213dL)

/* MessageId  : 0x0000213e */
/* Approx. msg: ERROR_DS_CANT_CREATE_UNDER_SCHEMA - An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container. */
#define ERROR_DS_CANT_CREATE_UNDER_SCHEMA	((ULONG)0x0000213eL)

/* MessageId  : 0x0000213f */
/* Approx. msg: ERROR_DS_INSTALL_NO_SRC_SCH_VERSION - The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it. */
#define ERROR_DS_INSTALL_NO_SRC_SCH_VERSION	((ULONG)0x0000213fL)

/* MessageId  : 0x00002140 */
/* Approx. msg: ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE - The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory. */
#define ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE	((ULONG)0x00002140L)

/* MessageId  : 0x00002141 */
/* Approx. msg: ERROR_DS_INVALID_GROUP_TYPE - The specified group type is invalid. */
#define ERROR_DS_INVALID_GROUP_TYPE	((ULONG)0x00002141L)

/* MessageId  : 0x00002142 */
/* Approx. msg: ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN - Cannot nest global groups in a mixed domain if the group is security-enabled. */
#define ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN	((ULONG)0x00002142L)

/* MessageId  : 0x00002143 */
/* Approx. msg: ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN - Cannot nest local groups in a mixed domain if the group is security-enabled. */
#define ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN	((ULONG)0x00002143L)

/* MessageId  : 0x00002144 */
/* Approx. msg: ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER - A global group cannot have a local group as a member. */
#define ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER	((ULONG)0x00002144L)

/* MessageId  : 0x00002145 */
/* Approx. msg: ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER - A global group cannot have a universal group as a member. */
#define ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER	((ULONG)0x00002145L)

/* MessageId  : 0x00002146 */
/* Approx. msg: ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER - A universal group cannot have a local group as a member. */
#define ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER	((ULONG)0x00002146L)

/* MessageId  : 0x00002147 */
/* Approx. msg: ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER - A global group cannot have a cross-domain member. */
#define ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER	((ULONG)0x00002147L)

/* MessageId  : 0x00002148 */
/* Approx. msg: ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER - A local group cannot have another cross-domain local group as a member. */
#define ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER	((ULONG)0x00002148L)

/* MessageId  : 0x00002149 */
/* Approx. msg: ERROR_DS_HAVE_PRIMARY_MEMBERS - A group with primary members cannot change to a security-disabled group. */
#define ERROR_DS_HAVE_PRIMARY_MEMBERS	((ULONG)0x00002149L)

/* MessageId  : 0x0000214a */
/* Approx. msg: ERROR_DS_STRING_SD_CONVERSION_FAILED - The schema cache load failed to convert the string default SD on a class-schema object. */
#define ERROR_DS_STRING_SD_CONVERSION_FAILED	((ULONG)0x0000214aL)

/* MessageId  : 0x0000214b */
/* Approx. msg: ERROR_DS_NAMING_MASTER_GC - Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers) */
#define ERROR_DS_NAMING_MASTER_GC	((ULONG)0x0000214bL)

/* MessageId  : 0x0000214c */
/* Approx. msg: ERROR_DS_LOOKUP_FAILURE - The DSA operation is unable to proceed because of a DNS lookup failure. */
#define ERROR_DS_LOOKUP_FAILURE	((ULONG)0x0000214cL)

/* MessageId  : 0x0000214d */
/* Approx. msg: ERROR_DS_COULDNT_UPDATE_SPNS - While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync. */
#define ERROR_DS_COULDNT_UPDATE_SPNS	((ULONG)0x0000214dL)

/* MessageId  : 0x0000214e */
/* Approx. msg: ERROR_DS_CANT_RETRIEVE_SD - The Security Descriptor attribute could not be read. */
#define ERROR_DS_CANT_RETRIEVE_SD	((ULONG)0x0000214eL)

/* MessageId  : 0x0000214f */
/* Approx. msg: ERROR_DS_KEY_NOT_UNIQUE - The object requested was not found, but an object with that key was found. */
#define ERROR_DS_KEY_NOT_UNIQUE	((ULONG)0x0000214fL)

/* MessageId  : 0x00002150 */
/* Approx. msg: ERROR_DS_WRONG_LINKED_ATT_SYNTAX - The syntax of the linked attributed being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1. */
#define ERROR_DS_WRONG_LINKED_ATT_SYNTAX	((ULONG)0x00002150L)

/* MessageId  : 0x00002151 */
/* Approx. msg: ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD - Security Account Manager needs to get the boot password. */
#define ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD	((ULONG)0x00002151L)

/* MessageId  : 0x00002152 */
/* Approx. msg: ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY - Security Account Manager needs to get the boot key from floppy disk. */
#define ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY	((ULONG)0x00002152L)

/* MessageId  : 0x00002153 */
/* Approx. msg: ERROR_DS_CANT_START - Directory Service cannot start. */
#define ERROR_DS_CANT_START	((ULONG)0x00002153L)

/* MessageId  : 0x00002154 */
/* Approx. msg: ERROR_DS_INIT_FAILURE - Directory Services could not start. */
#define ERROR_DS_INIT_FAILURE	((ULONG)0x00002154L)

/* MessageId  : 0x00002155 */
/* Approx. msg: ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION - The connection between client and server requires packet privacy or better. */
#define ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION	((ULONG)0x00002155L)

/* MessageId  : 0x00002156 */
/* Approx. msg: ERROR_DS_SOURCE_DOMAIN_IN_FOREST - The source domain may not be in the same forest as destination. */
#define ERROR_DS_SOURCE_DOMAIN_IN_FOREST	((ULONG)0x00002156L)

/* MessageId  : 0x00002157 */
/* Approx. msg: ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST - The destination domain must be in the forest. */
#define ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST	((ULONG)0x00002157L)

/* MessageId  : 0x00002158 */
/* Approx. msg: ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED - The operation requires that destination domain auditing be enabled. */
#define ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED	((ULONG)0x00002158L)

/* MessageId  : 0x00002159 */
/* Approx. msg: ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN - The operation couldn't locate a DC for the source domain. */
#define ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN	((ULONG)0x00002159L)

/* MessageId  : 0x0000215a */
/* Approx. msg: ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER - The source object must be a group or user. */
#define ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER	((ULONG)0x0000215aL)

/* MessageId  : 0x0000215b */
/* Approx. msg: ERROR_DS_SRC_SID_EXISTS_IN_FOREST - The source object's SID already exists in destination forest. */
#define ERROR_DS_SRC_SID_EXISTS_IN_FOREST	((ULONG)0x0000215bL)

/* MessageId  : 0x0000215c */
/* Approx. msg: ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH - The source and destination object must be of the same type. */
#define ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH	((ULONG)0x0000215cL)

/* MessageId  : 0x0000215d */
/* Approx. msg: ERROR_SAM_INIT_FAILURE - Security Accounts Manager initialization failed because of the following error: %1. */
#define ERROR_SAM_INIT_FAILURE	((ULONG)0x0000215dL)

/* MessageId  : 0x0000215e */
/* Approx. msg: ERROR_DS_DRA_SCHEMA_INFO_SHIP - Schema information could not be included in the replication request. */
#define ERROR_DS_DRA_SCHEMA_INFO_SHIP	((ULONG)0x0000215eL)

/* MessageId  : 0x0000215f */
/* Approx. msg: ERROR_DS_DRA_SCHEMA_CONFLICT - The replication operation could not be completed due to a schema incompatibility. */
#define ERROR_DS_DRA_SCHEMA_CONFLICT	((ULONG)0x0000215fL)

/* MessageId  : 0x00002160 */
/* Approx. msg: ERROR_DS_DRA_EARLIER_SCHEMA_CONLICT - The replication operation could not be completed due to a previous schema incompatibility. */
#define ERROR_DS_DRA_EARLIER_SCHEMA_CONLICT	((ULONG)0x00002160L)

/* MessageId  : 0x00002161 */
/* Approx. msg: ERROR_DS_DRA_OBJ_NC_MISMATCH - The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation. */
#define ERROR_DS_DRA_OBJ_NC_MISMATCH	((ULONG)0x00002161L)

/* MessageId  : 0x00002162 */
/* Approx. msg: ERROR_DS_NC_STILL_HAS_DSAS - The requested domain could not be deleted because there exist domain controllers that still host this domain. */
#define ERROR_DS_NC_STILL_HAS_DSAS	((ULONG)0x00002162L)

/* MessageId  : 0x00002163 */
/* Approx. msg: ERROR_DS_GC_REQUIRED - The requested operation can be performed only on a global catalog server. */
#define ERROR_DS_GC_REQUIRED	((ULONG)0x00002163L)

/* MessageId  : 0x00002164 */
/* Approx. msg: ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY - A local group can only be a member of other local groups in the same domain. */
#define ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY	((ULONG)0x00002164L)

/* MessageId  : 0x00002165 */
/* Approx. msg: ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS - Foreign security principals cannot be members of universal groups. */
#define ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS	((ULONG)0x00002165L)

/* MessageId  : 0x00002166 */
/* Approx. msg: ERROR_DS_CANT_ADD_TO_GC - The attribute is not allowed to be replicated to the GC because of security reasons. */
#define ERROR_DS_CANT_ADD_TO_GC	((ULONG)0x00002166L)

/* MessageId  : 0x00002167 */
/* Approx. msg: ERROR_DS_NO_CHECKPOINT_WITH_PDC - The checkpoint with the PDC could not be taken because there are too many modifications being processed currently. */
#define ERROR_DS_NO_CHECKPOINT_WITH_PDC	((ULONG)0x00002167L)

/* MessageId  : 0x00002168 */
/* Approx. msg: ERROR_DS_SOURCE_AUDITING_NOT_ENABLED - The operation requires that source domain auditing be enabled. */
#define ERROR_DS_SOURCE_AUDITING_NOT_ENABLED	((ULONG)0x00002168L)

/* MessageId  : 0x00002169 */
/* Approx. msg: ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC - Security principal objects can only be created inside domain naming contexts. */
#define ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC	((ULONG)0x00002169L)

/* MessageId  : 0x0000216a */
/* Approx. msg: ERROR_DS_INVALID_NAME_FOR_SPN - A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format. */
#define ERROR_DS_INVALID_NAME_FOR_SPN	((ULONG)0x0000216aL)

/* MessageId  : 0x0000216b */
/* Approx. msg: ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS - A Filter was passed that uses constructed attributes. */
#define ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS	((ULONG)0x0000216bL)

/* MessageId  : 0x0000216c */
/* Approx. msg: ERROR_DS_UNICODEPWD_NOT_IN_QUOTES - The unicodePwd attribute value must be enclosed in double quotes. */
#define ERROR_DS_UNICODEPWD_NOT_IN_QUOTES	((ULONG)0x0000216cL)

/* MessageId  : 0x0000216d */
/* Approx. msg: ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED - Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased. */
#define ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED	((ULONG)0x0000216dL)

/* MessageId  : 0x0000216e */
/* Approx. msg: ERROR_DS_MUST_BE_RUN_ON_DST_DC - For security reasons, the operation must be run on the destination DC. */
#define ERROR_DS_MUST_BE_RUN_ON_DST_DC	((ULONG)0x0000216eL)

/* MessageId  : 0x0000216f */
/* Approx. msg: ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER - For security reasons, the source DC must be NT4SP4 or greater. */
#define ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER	((ULONG)0x0000216fL)

/* MessageId  : 0x00002170 */
/* Approx. msg: ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ - Critical Directory Service System objects cannot be deleted during tree delete operations. The tree delete may have been partially performed. */
#define ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ	((ULONG)0x00002170L)

/* MessageId  : 0x00002171 */
/* Approx. msg: ERROR_DS_INIT_FAILURE_CONSOLE - Directory Services could not start because of the following error: %1. */
#define ERROR_DS_INIT_FAILURE_CONSOLE	((ULONG)0x00002171L)

/* MessageId  : 0x00002172 */
/* Approx. msg: ERROR_DS_SAM_INIT_FAILURE_CONSOLE - Security Accounts Manager initialization failed because of the following error: %1. */
#define ERROR_DS_SAM_INIT_FAILURE_CONSOLE	((ULONG)0x00002172L)

/* MessageId  : 0x00002173 */
/* Approx. msg: ERROR_DS_FOREST_VERSION_TOO_HIGH - The version of the operating system installed is incompatible with the current forest functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this forest. */
#define ERROR_DS_FOREST_VERSION_TOO_HIGH	((ULONG)0x00002173L)

/* MessageId  : 0x00002174 */
/* Approx. msg: ERROR_DS_DOMAIN_VERSION_TOO_HIGH - The version of the operating system installed is incompatible with the current domain functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this domain. */
#define ERROR_DS_DOMAIN_VERSION_TOO_HIGH	((ULONG)0x00002174L)

/* MessageId  : 0x00002175 */
/* Approx. msg: ERROR_DS_FOREST_VERSION_TOO_LOW - This version of the operating system installed on this server no longer supports the current forest functional level. You must raise the forest functional level before this server can become a domain controller in this forest. */
#define ERROR_DS_FOREST_VERSION_TOO_LOW	((ULONG)0x00002175L)

/* MessageId  : 0x00002176 */
/* Approx. msg: ERROR_DS_DOMAIN_VERSION_TOO_LOW - This version of the operating system installed on this server no longer supports the current domain functional level. You must raise the domain functional level before this server can become a domain controller in this domain. */
#define ERROR_DS_DOMAIN_VERSION_TOO_LOW	((ULONG)0x00002176L)

/* MessageId  : 0x00002177 */
/* Approx. msg: ERROR_DS_INCOMPATIBLE_VERSION - The version of the operating system installed on this server is incompatible with the functional level of the domain or forest. */
#define ERROR_DS_INCOMPATIBLE_VERSION	((ULONG)0x00002177L)

/* MessageId  : 0x00002178 */
/* Approx. msg: ERROR_DS_LOW_DSA_VERSION - The functional level of the domain (or forest) cannot be raised to the requested value, because there exist one or more domain controllers in the domain (or forest) that are at a lower incompatible functional level. */
#define ERROR_DS_LOW_DSA_VERSION	((ULONG)0x00002178L)

/* MessageId  : 0x00002179 */
/* Approx. msg: ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN - The forest functional level cannot be raised to the requested level since one or more domains are still in mixed domain mode. All domains in the forest must be in native mode before you can raise the forest functional level. */
#define ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN	((ULONG)0x00002179L)

/* MessageId  : 0x0000217a */
/* Approx. msg: ERROR_DS_NOT_SUPPORTED_SORT_ORDER - The sort order requested is not supported. */
#define ERROR_DS_NOT_SUPPORTED_SORT_ORDER	((ULONG)0x0000217aL)

/* MessageId  : 0x0000217b */
/* Approx. msg: ERROR_DS_NAME_NOT_UNIQUE - The requested name already exists as a unique identifier. */
#define ERROR_DS_NAME_NOT_UNIQUE	((ULONG)0x0000217bL)

/* MessageId  : 0x0000217c */
/* Approx. msg: ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4 - The machine account was created pre-NT4. The account needs to be recreated. */
#define ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4	((ULONG)0x0000217cL)

/* MessageId  : 0x0000217d */
/* Approx. msg: ERROR_DS_OUT_OF_VERSION_STORE - The database is out of version store. */
#define ERROR_DS_OUT_OF_VERSION_STORE	((ULONG)0x0000217dL)

/* MessageId  : 0x0000217e */
/* Approx. msg: ERROR_DS_INCOMPATIBLE_CONTROLS_USED - Unable to continue operation because multiple conflicting controls were used. */
#define ERROR_DS_INCOMPATIBLE_CONTROLS_USED	((ULONG)0x0000217eL)

/* MessageId  : 0x0000217f */
/* Approx. msg: ERROR_DS_NO_REF_DOMAIN - Unable to find a valid security descriptor reference domain for this partition. */
#define ERROR_DS_NO_REF_DOMAIN	((ULONG)0x0000217fL)

/* MessageId  : 0x00002180 */
/* Approx. msg: ERROR_DS_RESERVED_LINK_ID - Schema update failed: The link identifier is reserved. */
#define ERROR_DS_RESERVED_LINK_ID	((ULONG)0x00002180L)

/* MessageId  : 0x00002181 */
/* Approx. msg: ERROR_DS_LINK_ID_NOT_AVAILABLE - Schema update failed: There are no link identifiers available. */
#define ERROR_DS_LINK_ID_NOT_AVAILABLE	((ULONG)0x00002181L)

/* MessageId  : 0x00002182 */
/* Approx. msg: ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER - An account group cannot have a universal group as a member. */
#define ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER	((ULONG)0x00002182L)

/* MessageId  : 0x00002183 */
/* Approx. msg: ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE - Rename or move operations on naming context heads or read-only objects are not allowed. */
#define ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE	((ULONG)0x00002183L)

/* MessageId  : 0x00002184 */
/* Approx. msg: ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC - Move operations on objects in the schema naming context are not allowed. */
#define ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC	((ULONG)0x00002184L)

/* MessageId  : 0x00002185 */
/* Approx. msg: ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG - A system flag has been set on the object and does not allow the object to be moved or renamed. */
#define ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG	((ULONG)0x00002185L)

/* MessageId  : 0x00002186 */
/* Approx. msg: ERROR_DS_MODIFYDN_WRONG_GRANDPARENT - This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers. */
#define ERROR_DS_MODIFYDN_WRONG_GRANDPARENT	((ULONG)0x00002186L)

/* MessageId  : 0x00002187 */
/* Approx. msg: ERROR_DS_NAME_ERROR_TRUST_REFERRAL - Unable to resolve completely, a referral to another forest is generated. */
#define ERROR_DS_NAME_ERROR_TRUST_REFERRAL	((ULONG)0x00002187L)

/* MessageId  : 0x00002188 */
/* Approx. msg: ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER - The requested action is not supported on standard server. */
#define ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER	((ULONG)0x00002188L)

/* MessageId  : 0x00002189 */
/* Approx. msg: ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD - Could not access a partition of the Active Directory located on a remote server. Make sure at least one server is running for the partition in question. */
#define ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD	((ULONG)0x00002189L)

/* MessageId  : 0x0000218a */
/* Approx. msg: ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2 - The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context. Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master. */
#define ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2	((ULONG)0x0000218aL)

/* MessageId  : 0x0000218b */
/* Approx. msg: ERROR_DS_THREAD_LIMIT_EXCEEDED - The thread limit for this request was exceeded. */
#define ERROR_DS_THREAD_LIMIT_EXCEEDED	((ULONG)0x0000218bL)

/* MessageId  : 0x0000218c */
/* Approx. msg: ERROR_DS_NOT_CLOSEST - The Global catalog server is not in the closet site. */
#define ERROR_DS_NOT_CLOSEST	((ULONG)0x0000218cL)

/* MessageId  : 0x0000218d */
/* Approx. msg: ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF - The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute. */
#define ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF	((ULONG)0x0000218dL)

/* MessageId  : 0x0000218e */
/* Approx. msg: ERROR_DS_SINGLE_USER_MODE_FAILED - The Directory Service failed to enter single user mode. */
#define ERROR_DS_SINGLE_USER_MODE_FAILED	((ULONG)0x0000218eL)

/* MessageId  : 0x0000218f */
/* Approx. msg: ERROR_DS_NTDSCRIPT_SYNTAX_ERROR - The Directory Service cannot parse the script because of a syntax error. */
#define ERROR_DS_NTDSCRIPT_SYNTAX_ERROR	((ULONG)0x0000218fL)

/* MessageId  : 0x00002190 */
/* Approx. msg: ERROR_DS_NTDSCRIPT_PROCESS_ERROR - The Directory Service cannot process the script because of an error. */
#define ERROR_DS_NTDSCRIPT_PROCESS_ERROR	((ULONG)0x00002190L)

/* MessageId  : 0x00002191 */
/* Approx. msg: ERROR_DS_DIFFERENT_REPL_EPOCHS - The directory service cannot perform the requested operation because the servers involved are of different replication epochs (which is usually related to a domain rename that is in progress). */
#define ERROR_DS_DIFFERENT_REPL_EPOCHS	((ULONG)0x00002191L)

/* MessageId  : 0x00002192 */
/* Approx. msg: ERROR_DS_DRS_EXTENSIONS_CHANGED - The directory service binding must be renegotiated due to a change in the server extensions information. */
#define ERROR_DS_DRS_EXTENSIONS_CHANGED	((ULONG)0x00002192L)

/* MessageId  : 0x00002193 */
/* Approx. msg: ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR - Operation not allowed on a disabled cross ref. */
#define ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR	((ULONG)0x00002193L)

/* MessageId  : 0x00002194 */
/* Approx. msg: ERROR_DS_NO_MSDS_INTID - Schema update failed: No values for msDS-IntId are available. */
#define ERROR_DS_NO_MSDS_INTID	((ULONG)0x00002194L)

/* MessageId  : 0x00002195 */
/* Approx. msg: ERROR_DS_DUP_MSDS_INTID - Schema update failed: Duplicate msDS-IntId. Retry the operation. */
#define ERROR_DS_DUP_MSDS_INTID	((ULONG)0x00002195L)

/* MessageId  : 0x00002196 */
/* Approx. msg: ERROR_DS_EXISTS_IN_RDNATTID - Schema deletion failed: attribute is used in rDNAttID. */
#define ERROR_DS_EXISTS_IN_RDNATTID	((ULONG)0x00002196L)

/* MessageId  : 0x00002197 */
/* Approx. msg: ERROR_DS_AUTHORIZATION_FAILED - The directory service failed to authorize the request. */
#define ERROR_DS_AUTHORIZATION_FAILED	((ULONG)0x00002197L)

/* MessageId  : 0x00002198 */
/* Approx. msg: ERROR_DS_INVALID_SCRIPT - The Directory Service cannot process the script because it is invalid. */
#define ERROR_DS_INVALID_SCRIPT	((ULONG)0x00002198L)

/* MessageId  : 0x00002199 */
/* Approx. msg: ERROR_DS_REMOTE_CROSSREF_OP_FAILED - The remote create cross reference operation failed on the Domain Naming Master FSMO. The operation's error is in the extended data. */
#define ERROR_DS_REMOTE_CROSSREF_OP_FAILED	((ULONG)0x00002199L)

/* MessageId  : 0x0000219a */
/* Approx. msg: ERROR_DS_CROSS_REF_BUSY - A cross reference is in use locally with the same name. */
#define ERROR_DS_CROSS_REF_BUSY	((ULONG)0x0000219aL)

/* MessageId  : 0x0000219b */
/* Approx. msg: ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN - The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the server's domain has been deleted from the forest. */
#define ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN	((ULONG)0x0000219bL)

/* MessageId  : 0x0000219c */
/* Approx. msg: ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC - Writeable NCs prevent this DC from demoting. */
#define ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC	((ULONG)0x0000219cL)

/* MessageId  : 0x0000219d */
/* Approx. msg: ERROR_DS_DUPLICATE_ID_FOUND - The requested object has a non-unique identifier and cannot be retrieved. */
#define ERROR_DS_DUPLICATE_ID_FOUND	((ULONG)0x0000219dL)

/* MessageId  : 0x0000219e */
/* Approx. msg: ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT - Insufficient attributes were given to create an object. This object may not exist because it may have been deleted and already garbage collected. */
#define ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT	((ULONG)0x0000219eL)

/* MessageId  : 0x0000219f */
/* Approx. msg: ERROR_DS_GROUP_CONVERSION_ERROR - The group cannot be converted due to attribute restrictions on the requested group type. */
#define ERROR_DS_GROUP_CONVERSION_ERROR	((ULONG)0x0000219fL)

/* MessageId  : 0x000021a0 */
/* Approx. msg: ERROR_DS_CANT_MOVE_APP_BASIC_GROUP - Cross-domain move of non-empty basic application groups is not allowed. */
#define ERROR_DS_CANT_MOVE_APP_BASIC_GROUP	((ULONG)0x000021a0L)

/* MessageId  : 0x000021a1 */
/* Approx. msg: ERROR_DS_CANT_MOVE_APP_QUERY_GROUP - Cross-domain move on non-empty query based application groups is not allowed. */
#define ERROR_DS_CANT_MOVE_APP_QUERY_GROUP	((ULONG)0x000021a1L)

/* MessageId  : 0x000021a2 */
/* Approx. msg: ERROR_DS_ROLE_NOT_VERIFIED - The role owner could not be verified because replication of its partition has not occurred recently. */
#define ERROR_DS_ROLE_NOT_VERIFIED	((ULONG)0x000021a2L)

/* MessageId  : 0x000021a3 */
/* Approx. msg: ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL - The target container for a redirection of a well-known object container cannot already be a special container. */
#define ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL	((ULONG)0x000021a3L)

/* MessageId  : 0x000021a4 */
/* Approx. msg: ERROR_DS_DOMAIN_RENAME_IN_PROGRESS - The Directory Service cannot perform the requested operation because a domain rename operation is in progress. */
#define ERROR_DS_DOMAIN_RENAME_IN_PROGRESS	((ULONG)0x000021a4L)

/* MessageId  : 0x000021a5 */
/* Approx. msg: ERROR_DS_EXISTING_AD_CHILD_NC - The Active Directory detected an Active Directory child partition below the requested new partition name. The Active Directory's partition hierarchy must be created in a top-down method. */
#define ERROR_DS_EXISTING_AD_CHILD_NC	((ULONG)0x000021a5L)

/* MessageId  : 0x000021a6 */
/* Approx. msg: ERROR_DS_REPL_LIFETIME_EXCEEDED - The Active Directory cannot replicate with this server because the time since the last replication with this server has exceeded the tombstone lifetime. */
#define ERROR_DS_REPL_LIFETIME_EXCEEDED	((ULONG)0x000021a6L)

/* MessageId  : 0x000021a7 */
/* Approx. msg: ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER - The requested operation is not allowed on an object under the system container. */
#define ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER	((ULONG)0x000021a7L)

/* MessageId  : 0x000021a8 */
/* Approx. msg: ERROR_DS_LDAP_SEND_QUEUE_FULL - The LDAP servers network send queue has filled up because the client is not processing the results of it's requests fast enough. No more requests will be processed until the client catches up. If the client does not catch up then it will be disconnected. */
#define ERROR_DS_LDAP_SEND_QUEUE_FULL	((ULONG)0x000021a8L)

/* MessageId  : 0x000021a9 */
/* Approx. msg: ERROR_DS_DRA_OUT_SCHEDULE_WINDOW - The scheduled replication did not take place because the system was too busy to execute the request within the schedule window. The replication queue is overloaded. Consider reducing the number of partners or decreasing the scheduled replication frequency. */
#define ERROR_DS_DRA_OUT_SCHEDULE_WINDOW	((ULONG)0x000021a9L)

/* MessageId  : 0x00002329 */
/* Approx. msg: DNS_ERROR_RCODE_FORMAT_ERROR - DNS server unable to interpret format. */
#define DNS_ERROR_RCODE_FORMAT_ERROR	((ULONG)0x00002329L)

/* MessageId  : 0x0000232a */
/* Approx. msg: DNS_ERROR_RCODE_SERVER_FAILURE - DNS server failure. */
#define DNS_ERROR_RCODE_SERVER_FAILURE	((ULONG)0x0000232aL)

/* MessageId  : 0x0000232b */
/* Approx. msg: DNS_ERROR_RCODE_NAME_ERROR - DNS name does not exist. */
#define DNS_ERROR_RCODE_NAME_ERROR	((ULONG)0x0000232bL)

/* MessageId  : 0x0000232c */
/* Approx. msg: DNS_ERROR_RCODE_NOT_IMPLEMENTED - DNS request not supported by name server. */
#define DNS_ERROR_RCODE_NOT_IMPLEMENTED	((ULONG)0x0000232cL)

/* MessageId  : 0x0000232d */
/* Approx. msg: DNS_ERROR_RCODE_REFUSED - DNS operation refused. */
#define DNS_ERROR_RCODE_REFUSED	((ULONG)0x0000232dL)

/* MessageId  : 0x0000232e */
/* Approx. msg: DNS_ERROR_RCODE_YXDOMAIN - DNS name that ought not exist, does exist. */
#define DNS_ERROR_RCODE_YXDOMAIN	((ULONG)0x0000232eL)

/* MessageId  : 0x0000232f */
/* Approx. msg: DNS_ERROR_RCODE_YXRRSET - DNS RR set that ought not exist, does exist. */
#define DNS_ERROR_RCODE_YXRRSET	((ULONG)0x0000232fL)

/* MessageId  : 0x00002330 */
/* Approx. msg: DNS_ERROR_RCODE_NXRRSET - DNS RR set that ought to exist, does not exist. */
#define DNS_ERROR_RCODE_NXRRSET	((ULONG)0x00002330L)

/* MessageId  : 0x00002331 */
/* Approx. msg: DNS_ERROR_RCODE_NOTAUTH - DNS server not authoritative for zone. */
#define DNS_ERROR_RCODE_NOTAUTH	((ULONG)0x00002331L)

/* MessageId  : 0x00002332 */
/* Approx. msg: DNS_ERROR_RCODE_NOTZONE - DNS name in update or prereq is not in zone. */
#define DNS_ERROR_RCODE_NOTZONE	((ULONG)0x00002332L)

/* MessageId  : 0x00002338 */
/* Approx. msg: DNS_ERROR_RCODE_BADSIG - DNS signature failed to verify. */
#define DNS_ERROR_RCODE_BADSIG	((ULONG)0x00002338L)

/* MessageId  : 0x00002339 */
/* Approx. msg: DNS_ERROR_RCODE_BADKEY - DNS bad key. */
#define DNS_ERROR_RCODE_BADKEY	((ULONG)0x00002339L)

/* MessageId  : 0x0000233a */
/* Approx. msg: DNS_ERROR_RCODE_BADTIME - DNS signature validity expired. */
#define DNS_ERROR_RCODE_BADTIME	((ULONG)0x0000233aL)

/* MessageId  : 0x0000251d */
/* Approx. msg: DNS_INFO_NO_RECORDS - No records found for given DNS query. */
#define DNS_INFO_NO_RECORDS	((ULONG)0x0000251dL)

/* MessageId  : 0x0000251e */
/* Approx. msg: DNS_ERROR_BAD_PACKET - Bad DNS packet. */
#define DNS_ERROR_BAD_PACKET	((ULONG)0x0000251eL)

/* MessageId  : 0x0000251f */
/* Approx. msg: DNS_ERROR_NO_PACKET - No DNS packet. */
#define DNS_ERROR_NO_PACKET	((ULONG)0x0000251fL)

/* MessageId  : 0x00002520 */
/* Approx. msg: DNS_ERROR_RCODE - DNS error, check rcode. */
#define DNS_ERROR_RCODE	((ULONG)0x00002520L)

/* MessageId  : 0x00002521 */
/* Approx. msg: DNS_ERROR_UNSECURE_PACKET - Unsecured DNS packet. */
#define DNS_ERROR_UNSECURE_PACKET	((ULONG)0x00002521L)

/* MessageId  : 0x0000254f */
/* Approx. msg: DNS_ERROR_INVALID_TYPE - Invalid DNS type. */
#define DNS_ERROR_INVALID_TYPE	((ULONG)0x0000254fL)

/* MessageId  : 0x00002550 */
/* Approx. msg: DNS_ERROR_INVALID_IP_ADDRESS - Invalid IP address. */
#define DNS_ERROR_INVALID_IP_ADDRESS	((ULONG)0x00002550L)

/* MessageId  : 0x00002551 */
/* Approx. msg: DNS_ERROR_INVALID_PROPERTY - Invalid property. */
#define DNS_ERROR_INVALID_PROPERTY	((ULONG)0x00002551L)

/* MessageId  : 0x00002552 */
/* Approx. msg: DNS_ERROR_TRY_AGAIN_LATER - Try DNS operation again later. */
#define DNS_ERROR_TRY_AGAIN_LATER	((ULONG)0x00002552L)

/* MessageId  : 0x00002553 */
/* Approx. msg: DNS_ERROR_NOT_UNIQUE - Record for given name and type is not unique. */
#define DNS_ERROR_NOT_UNIQUE	((ULONG)0x00002553L)

/* MessageId  : 0x00002554 */
/* Approx. msg: DNS_ERROR_NON_RFC_NAME - DNS name does not comply with RFC specifications. */
#define DNS_ERROR_NON_RFC_NAME	((ULONG)0x00002554L)

/* MessageId  : 0x00002555 */
/* Approx. msg: DNS_STATUS_FQDN - DNS name is a fully-qualified DNS name. */
#define DNS_STATUS_FQDN	((ULONG)0x00002555L)

/* MessageId  : 0x00002556 */
/* Approx. msg: DNS_STATUS_DOTTED_NAME - DNS name is dotted (multi-label). */
#define DNS_STATUS_DOTTED_NAME	((ULONG)0x00002556L)

/* MessageId  : 0x00002557 */
/* Approx. msg: DNS_STATUS_SINGLE_PART_NAME - DNS name is a single-part name. */
#define DNS_STATUS_SINGLE_PART_NAME	((ULONG)0x00002557L)

/* MessageId  : 0x00002558 */
/* Approx. msg: DNS_ERROR_INVALID_NAME_CHAR - DSN name contains an invalid character. */
#define DNS_ERROR_INVALID_NAME_CHAR	((ULONG)0x00002558L)

/* MessageId  : 0x00002559 */
/* Approx. msg: DNS_ERROR_NUMERIC_NAME - DNS name is entirely numeric. */
#define DNS_ERROR_NUMERIC_NAME	((ULONG)0x00002559L)

/* MessageId  : 0x0000255a */
/* Approx. msg: DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER - The operation requested is not permitted on a DNS root server. */
#define DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER	((ULONG)0x0000255aL)

/* MessageId  : 0x0000255b */
/* Approx. msg: DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION - The record could not be created because this part of the DNS namespace has been delegated to another server. */
#define DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION	((ULONG)0x0000255bL)

/* MessageId  : 0x0000255c */
/* Approx. msg: DNS_ERROR_CANNOT_FIND_ROOT_HINTS - The DNS server could not find a set of root hints. */
#define DNS_ERROR_CANNOT_FIND_ROOT_HINTS	((ULONG)0x0000255cL)

/* MessageId  : 0x0000255d */
/* Approx. msg: DNS_ERROR_INCONSISTENT_ROOT_HINTS - The DNS server found root hints but they were not consistent across all adapters. */
#define DNS_ERROR_INCONSISTENT_ROOT_HINTS	((ULONG)0x0000255dL)

/* MessageId  : 0x00002581 */
/* Approx. msg: DNS_ERROR_ZONE_DOES_NOT_EXIST - DNS zone does not exist. */
#define DNS_ERROR_ZONE_DOES_NOT_EXIST	((ULONG)0x00002581L)

/* MessageId  : 0x00002582 */
/* Approx. msg: DNS_ERROR_NO_ZONE_INFO - DNS zone information not available. */
#define DNS_ERROR_NO_ZONE_INFO	((ULONG)0x00002582L)

/* MessageId  : 0x00002583 */
/* Approx. msg: DNS_ERROR_INVALID_ZONE_OPERATION - Invalid operation for DNS zone. */
#define DNS_ERROR_INVALID_ZONE_OPERATION	((ULONG)0x00002583L)

/* MessageId  : 0x00002584 */
/* Approx. msg: DNS_ERROR_ZONE_CONFIGURATION_ERROR - Invalid DNS zone configuration. */
#define DNS_ERROR_ZONE_CONFIGURATION_ERROR	((ULONG)0x00002584L)

/* MessageId  : 0x00002585 */
/* Approx. msg: DNS_ERROR_ZONE_HAS_NO_SOA_RECORD - DNS zone has no start of authority (SOA) record. */
#define DNS_ERROR_ZONE_HAS_NO_SOA_RECORD	((ULONG)0x00002585L)

/* MessageId  : 0x00002586 */
/* Approx. msg: DNS_ERROR_ZONE_HAS_NO_NS_RECORDS - DNS zone has no name server (NS) record. */
#define DNS_ERROR_ZONE_HAS_NO_NS_RECORDS	((ULONG)0x00002586L)

/* MessageId  : 0x00002587 */
/* Approx. msg: DNS_ERROR_ZONE_LOCKED - DNS zone is locked. */
#define DNS_ERROR_ZONE_LOCKED	((ULONG)0x00002587L)

/* MessageId  : 0x00002588 */
/* Approx. msg: DNS_ERROR_ZONE_CREATION_FAILED - DNS zone creation failed. */
#define DNS_ERROR_ZONE_CREATION_FAILED	((ULONG)0x00002588L)

/* MessageId  : 0x00002589 */
/* Approx. msg: DNS_ERROR_ZONE_ALREADY_EXISTS - DNS zone already exists. */
#define DNS_ERROR_ZONE_ALREADY_EXISTS	((ULONG)0x00002589L)

/* MessageId  : 0x0000258a */
/* Approx. msg: DNS_ERROR_AUTOZONE_ALREADY_EXISTS - DNS automatic zone already exists. */
#define DNS_ERROR_AUTOZONE_ALREADY_EXISTS	((ULONG)0x0000258aL)

/* MessageId  : 0x0000258b */
/* Approx. msg: DNS_ERROR_INVALID_ZONE_TYPE - Invalid DNS zone type. */
#define DNS_ERROR_INVALID_ZONE_TYPE	((ULONG)0x0000258bL)

/* MessageId  : 0x0000258c */
/* Approx. msg: DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP - Secondary DNS zone requires master IP address. */
#define DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP	((ULONG)0x0000258cL)

/* MessageId  : 0x0000258d */
/* Approx. msg: DNS_ERROR_ZONE_NOT_SECONDARY - DNS zone not secondary. */
#define DNS_ERROR_ZONE_NOT_SECONDARY	((ULONG)0x0000258dL)

/* MessageId  : 0x0000258e */
/* Approx. msg: DNS_ERROR_NEED_SECONDARY_ADDRESSES - Need secondary IP address. */
#define DNS_ERROR_NEED_SECONDARY_ADDRESSES	((ULONG)0x0000258eL)

/* MessageId  : 0x0000258f */
/* Approx. msg: DNS_ERROR_WINS_INIT_FAILED - WINS initialization failed. */
#define DNS_ERROR_WINS_INIT_FAILED	((ULONG)0x0000258fL)

/* MessageId  : 0x00002590 */
/* Approx. msg: DNS_ERROR_NEED_WINS_SERVERS - Need WINS servers. */
#define DNS_ERROR_NEED_WINS_SERVERS	((ULONG)0x00002590L)

/* MessageId  : 0x00002591 */
/* Approx. msg: DNS_ERROR_NBSTAT_INIT_FAILED - NBTSTAT initialization call failed. */
#define DNS_ERROR_NBSTAT_INIT_FAILED	((ULONG)0x00002591L)

/* MessageId  : 0x00002592 */
/* Approx. msg: DNS_ERROR_SOA_DELETE_INVALID - Invalid delete of start of authority (SOA) */
#define DNS_ERROR_SOA_DELETE_INVALID	((ULONG)0x00002592L)

/* MessageId  : 0x00002593 */
/* Approx. msg: DNS_ERROR_FORWARDER_ALREADY_EXISTS - A conditional forwarding zone already exists for that name. */
#define DNS_ERROR_FORWARDER_ALREADY_EXISTS	((ULONG)0x00002593L)

/* MessageId  : 0x00002594 */
/* Approx. msg: DNS_ERROR_ZONE_REQUIRES_MASTER_IP - This zone must be configured with one or more master DNS server IP addresses. */
#define DNS_ERROR_ZONE_REQUIRES_MASTER_IP	((ULONG)0x00002594L)

/* MessageId  : 0x00002595 */
/* Approx. msg: DNS_ERROR_ZONE_IS_SHUTDOWN - The operation cannot be performed because this zone is shutdown. */
#define DNS_ERROR_ZONE_IS_SHUTDOWN	((ULONG)0x00002595L)

/* MessageId  : 0x000025b3 */
/* Approx. msg: DNS_ERROR_PRIMARY_REQUIRES_DATAFILE - Primary DNS zone requires datafile. */
#define DNS_ERROR_PRIMARY_REQUIRES_DATAFILE	((ULONG)0x000025b3L)

/* MessageId  : 0x000025b4 */
/* Approx. msg: DNS_ERROR_INVALID_DATAFILE_NAME - Invalid datafile name for DNS zone. */
#define DNS_ERROR_INVALID_DATAFILE_NAME	((ULONG)0x000025b4L)

/* MessageId  : 0x000025b5 */
/* Approx. msg: DNS_ERROR_DATAFILE_OPEN_FAILURE - Failed to open datafile for DNS zone. */
#define DNS_ERROR_DATAFILE_OPEN_FAILURE	((ULONG)0x000025b5L)

/* MessageId  : 0x000025b6 */
/* Approx. msg: DNS_ERROR_FILE_WRITEBACK_FAILED - Failed to write datafile for DNS zone. */
#define DNS_ERROR_FILE_WRITEBACK_FAILED	((ULONG)0x000025b6L)

/* MessageId  : 0x000025b7 */
/* Approx. msg: DNS_ERROR_DATAFILE_PARSING - Failure while reading datafile for DNS zone. */
#define DNS_ERROR_DATAFILE_PARSING	((ULONG)0x000025b7L)

/* MessageId  : 0x000025e5 */
/* Approx. msg: DNS_ERROR_RECORD_DOES_NOT_EXIST - DNS record does not exist. */
#define DNS_ERROR_RECORD_DOES_NOT_EXIST	((ULONG)0x000025e5L)

/* MessageId  : 0x000025e6 */
/* Approx. msg: DNS_ERROR_RECORD_FORMAT - DNS record format error. */
#define DNS_ERROR_RECORD_FORMAT	((ULONG)0x000025e6L)

/* MessageId  : 0x000025e7 */
/* Approx. msg: DNS_ERROR_NODE_CREATION_FAILED - Node creation failure in DNS. */
#define DNS_ERROR_NODE_CREATION_FAILED	((ULONG)0x000025e7L)

/* MessageId  : 0x000025e8 */
/* Approx. msg: DNS_ERROR_UNKNOWN_RECORD_TYPE - Unknown DNS record type. */
#define DNS_ERROR_UNKNOWN_RECORD_TYPE	((ULONG)0x000025e8L)

/* MessageId  : 0x000025e9 */
/* Approx. msg: DNS_ERROR_RECORD_TIMED_OUT - DNS record timed out. */
#define DNS_ERROR_RECORD_TIMED_OUT	((ULONG)0x000025e9L)

/* MessageId  : 0x000025ea */
/* Approx. msg: DNS_ERROR_NAME_NOT_IN_ZONE - Name not in DNS zone. */
#define DNS_ERROR_NAME_NOT_IN_ZONE	((ULONG)0x000025eaL)

/* MessageId  : 0x000025eb */
/* Approx. msg: DNS_ERROR_CNAME_LOOP - CNAME loop detected. */
#define DNS_ERROR_CNAME_LOOP	((ULONG)0x000025ebL)

/* MessageId  : 0x000025ec */
/* Approx. msg: DNS_ERROR_NODE_IS_CNAME - Node is a CNAME DNS record. */
#define DNS_ERROR_NODE_IS_CNAME	((ULONG)0x000025ecL)

/* MessageId  : 0x000025ed */
/* Approx. msg: DNS_ERROR_CNAME_COLLISION - A CNAME record already exists for given name. */
#define DNS_ERROR_CNAME_COLLISION	((ULONG)0x000025edL)

/* MessageId  : 0x000025ee */
/* Approx. msg: DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT - Record only at DNS zone root. */
#define DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT	((ULONG)0x000025eeL)

/* MessageId  : 0x000025ef */
/* Approx. msg: DNS_ERROR_RECORD_ALREADY_EXISTS - DNS record already exists. */
#define DNS_ERROR_RECORD_ALREADY_EXISTS	((ULONG)0x000025efL)

/* MessageId  : 0x000025f0 */
/* Approx. msg: DNS_ERROR_SECONDARY_DATA - Secondary DNS zone data error. */
#define DNS_ERROR_SECONDARY_DATA	((ULONG)0x000025f0L)

/* MessageId  : 0x000025f1 */
/* Approx. msg: DNS_ERROR_NO_CREATE_CACHE_DATA - Could not create DNS cache data. */
#define DNS_ERROR_NO_CREATE_CACHE_DATA	((ULONG)0x000025f1L)

/* MessageId  : 0x000025f2 */
/* Approx. msg: DNS_ERROR_NAME_DOES_NOT_EXIST - DNS name does not exist. */
#define DNS_ERROR_NAME_DOES_NOT_EXIST	((ULONG)0x000025f2L)

/* MessageId  : 0x000025f3 */
/* Approx. msg: DNS_WARNING_PTR_CREATE_FAILED - Could not create pointer (PTR) record. */
#define DNS_WARNING_PTR_CREATE_FAILED	((ULONG)0x000025f3L)

/* MessageId  : 0x000025f4 */
/* Approx. msg: DNS_WARNING_DOMAIN_UNDELETED - DNS domain was undeleted. */
#define DNS_WARNING_DOMAIN_UNDELETED	((ULONG)0x000025f4L)

/* MessageId  : 0x000025f5 */
/* Approx. msg: DNS_ERROR_DS_UNAVAILABLE - The directory service is unavailable. */
#define DNS_ERROR_DS_UNAVAILABLE	((ULONG)0x000025f5L)

/* MessageId  : 0x000025f6 */
/* Approx. msg: DNS_ERROR_DS_ZONE_ALREADY_EXISTS - DNS zone already exists in the directory service. */
#define DNS_ERROR_DS_ZONE_ALREADY_EXISTS	((ULONG)0x000025f6L)

/* MessageId  : 0x000025f7 */
/* Approx. msg: DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE - DNS server not creating or reading the boot file for the directory service integrated DNS zone. */
#define DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE	((ULONG)0x000025f7L)

/* MessageId  : 0x00002617 */
/* Approx. msg: DNS_INFO_AXFR_COMPLETE - DNS AXFR (zone transfer) complete. */
#define DNS_INFO_AXFR_COMPLETE	((ULONG)0x00002617L)

/* MessageId  : 0x00002618 */
/* Approx. msg: DNS_ERROR_AXFR - DNS zone transfer failed. */
#define DNS_ERROR_AXFR	((ULONG)0x00002618L)

/* MessageId  : 0x00002619 */
/* Approx. msg: DNS_INFO_ADDED_LOCAL_WINS - Added local WINS server. */
#define DNS_INFO_ADDED_LOCAL_WINS	((ULONG)0x00002619L)

/* MessageId  : 0x00002649 */
/* Approx. msg: DNS_STATUS_CONTINUE_NEEDED - Secure update call needs to continue update request. */
#define DNS_STATUS_CONTINUE_NEEDED	((ULONG)0x00002649L)

/* MessageId  : 0x0000267b */
/* Approx. msg: DNS_ERROR_NO_TCPIP - TCP/IP network protocol not installed. */
#define DNS_ERROR_NO_TCPIP	((ULONG)0x0000267bL)

/* MessageId  : 0x0000267c */
/* Approx. msg: DNS_ERROR_NO_DNS_SERVERS - No DNS servers configured for local system. */
#define DNS_ERROR_NO_DNS_SERVERS	((ULONG)0x0000267cL)

/* MessageId  : 0x000026ad */
/* Approx. msg: DNS_ERROR_DP_DOES_NOT_EXIST - The specified directory partition does not exist. */
#define DNS_ERROR_DP_DOES_NOT_EXIST	((ULONG)0x000026adL)

/* MessageId  : 0x000026ae */
/* Approx. msg: DNS_ERROR_DP_ALREADY_EXISTS - The specified directory partition already exists. */
#define DNS_ERROR_DP_ALREADY_EXISTS	((ULONG)0x000026aeL)

/* MessageId  : 0x000026af */
/* Approx. msg: DNS_ERROR_DP_NOT_ENLISTED - The DNS server is not enlisted in the specified directory partition. */
#define DNS_ERROR_DP_NOT_ENLISTED	((ULONG)0x000026afL)

/* MessageId  : 0x000026b0 */
/* Approx. msg: DNS_ERROR_DP_ALREADY_ENLISTED - The DNS server is already enlisted in the specified directory partition. */
#define DNS_ERROR_DP_ALREADY_ENLISTED	((ULONG)0x000026b0L)

/* MessageId  : 0x000026b1 */
/* Approx. msg: DNS_ERROR_DP_NOT_AVAILABLE - The directory partition is not available at this time. Please wait a few minutes and try again. */
#define DNS_ERROR_DP_NOT_AVAILABLE	((ULONG)0x000026b1L)

/* MessageId  : 0x000026b2 */
/* Approx. msg: DNS_ERROR_DP_FSMO_ERROR - The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003. */
#define DNS_ERROR_DP_FSMO_ERROR	((ULONG)0x000026b2L)

/* MessageId  : 0x00002714 */
/* Approx. msg: WSAEINTR - A blocking operation was interrupted by a call to WSACancelBlockingCall. */
#define WSAEINTR	((ULONG)0x00002714L)

/* MessageId  : 0x00002719 */
/* Approx. msg: WSAEBADF - The file handle supplied is not valid. */
#define WSAEBADF	((ULONG)0x00002719L)

/* MessageId  : 0x0000271d */
/* Approx. msg: WSAEACCES - An attempt was made to access a socket in a way forbidden by its access permissions. */
#define WSAEACCES	((ULONG)0x0000271dL)

/* MessageId  : 0x0000271e */
/* Approx. msg: WSAEFAULT - The system detected an invalid pointer address in attempting to use a pointer argument in a call. */
#define WSAEFAULT	((ULONG)0x0000271eL)

/* MessageId  : 0x00002726 */
/* Approx. msg: WSAEINVAL - An invalid argument was supplied. */
#define WSAEINVAL	((ULONG)0x00002726L)

/* MessageId  : 0x00002728 */
/* Approx. msg: WSAEMFILE - Too many open sockets. */
#define WSAEMFILE	((ULONG)0x00002728L)

/* MessageId  : 0x00002733 */
/* Approx. msg: WSAEWOULDBLOCK - A non-blocking socket operation could not be completed immediately. */
#define WSAEWOULDBLOCK	((ULONG)0x00002733L)

/* MessageId  : 0x00002734 */
/* Approx. msg: WSAEINPROGRESS - A blocking operation is currently executing. */
#define WSAEINPROGRESS	((ULONG)0x00002734L)

/* MessageId  : 0x00002735 */
/* Approx. msg: WSAEALREADY - An operation was attempted on a non-blocking socket that already had an operation in progress. */
#define WSAEALREADY	((ULONG)0x00002735L)

/* MessageId  : 0x00002736 */
/* Approx. msg: WSAENOTSOCK - An operation was attempted on something that is not a socket. */
#define WSAENOTSOCK	((ULONG)0x00002736L)

/* MessageId  : 0x00002737 */
/* Approx. msg: WSAEDESTADDRREQ - A required address was omitted from an operation on a socket. */
#define WSAEDESTADDRREQ	((ULONG)0x00002737L)

/* MessageId  : 0x00002738 */
/* Approx. msg: WSAEMSGSIZE - A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself. */
#define WSAEMSGSIZE	((ULONG)0x00002738L)

/* MessageId  : 0x00002739 */
/* Approx. msg: WSAEPROTOTYPE - A protocol was specified in the socket function call that does not support the semantics of the socket type requested. */
#define WSAEPROTOTYPE	((ULONG)0x00002739L)

/* MessageId  : 0x0000273a */
/* Approx. msg: WSAENOPROTOOPT - An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call. */
#define WSAENOPROTOOPT	((ULONG)0x0000273aL)

/* MessageId  : 0x0000273b */
/* Approx. msg: WSAEPROTONOSUPPORT - The requested protocol has not been configured into the system, or no implementation for it exists. */
#define WSAEPROTONOSUPPORT	((ULONG)0x0000273bL)

/* MessageId  : 0x0000273c */
/* Approx. msg: WSAESOCKTNOSUPPORT - The support for the specified socket type does not exist in this address family. */
#define WSAESOCKTNOSUPPORT	((ULONG)0x0000273cL)

/* MessageId  : 0x0000273d */
/* Approx. msg: WSAEOPNOTSUPP - The attempted operation is not supported for the type of object referenced. */
#define WSAEOPNOTSUPP	((ULONG)0x0000273dL)

/* MessageId  : 0x0000273e */
/* Approx. msg: WSAEPFNOSUPPORT - The protocol family has not been configured into the system or no implementation for it exists. */
#define WSAEPFNOSUPPORT	((ULONG)0x0000273eL)

/* MessageId  : 0x0000273f */
/* Approx. msg: WSAEAFNOSUPPORT - An address incompatible with the requested protocol was used. */
#define WSAEAFNOSUPPORT	((ULONG)0x0000273fL)

/* MessageId  : 0x00002740 */
/* Approx. msg: WSAEADDRINUSE - Only one usage of each socket address (protocol/network address/port) is normally permitted. */
#define WSAEADDRINUSE	((ULONG)0x00002740L)

/* MessageId  : 0x00002741 */
/* Approx. msg: WSAEADDRNOTAVAIL - The requested address is not valid in its context. */
#define WSAEADDRNOTAVAIL	((ULONG)0x00002741L)

/* MessageId  : 0x00002742 */
/* Approx. msg: WSAENETDOWN - A socket operation encountered a dead network. */
#define WSAENETDOWN	((ULONG)0x00002742L)

/* MessageId  : 0x00002743 */
/* Approx. msg: WSAENETUNREACH - A socket operation was attempted to an unreachable network. */
#define WSAENETUNREACH	((ULONG)0x00002743L)

/* MessageId  : 0x00002744 */
/* Approx. msg: WSAENETRESET - The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. */
#define WSAENETRESET	((ULONG)0x00002744L)

/* MessageId  : 0x00002745 */
/* Approx. msg: WSAECONNABORTED - An established connection was aborted by the software in your host machine. */
#define WSAECONNABORTED	((ULONG)0x00002745L)

/* MessageId  : 0x00002746 */
/* Approx. msg: WSAECONNRESET - An existing connection was forcibly closed by the remote host. */
#define WSAECONNRESET	((ULONG)0x00002746L)

/* MessageId  : 0x00002747 */
/* Approx. msg: WSAENOBUFS - An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full. */
#define WSAENOBUFS	((ULONG)0x00002747L)

/* MessageId  : 0x00002748 */
/* Approx. msg: WSAEISCONN - A connect request was made on an already connected socket. */
#define WSAEISCONN	((ULONG)0x00002748L)

/* MessageId  : 0x00002749 */
/* Approx. msg: WSAENOTCONN - A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied. */
#define WSAENOTCONN	((ULONG)0x00002749L)

/* MessageId  : 0x0000274a */
/* Approx. msg: WSAESHUTDOWN - A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. */
#define WSAESHUTDOWN	((ULONG)0x0000274aL)

/* MessageId  : 0x0000274b */
/* Approx. msg: WSAETOOMANYREFS - Too many references to some kernel object. */
#define WSAETOOMANYREFS	((ULONG)0x0000274bL)

/* MessageId  : 0x0000274c */
/* Approx. msg: WSAETIMEDOUT - A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond. */
#define WSAETIMEDOUT	((ULONG)0x0000274cL)

/* MessageId  : 0x0000274d */
/* Approx. msg: WSAECONNREFUSED - No connection could be made because the target machine actively refused it. */
#define WSAECONNREFUSED	((ULONG)0x0000274dL)

/* MessageId  : 0x0000274e */
/* Approx. msg: WSAELOOP - Cannot translate name. */
#define WSAELOOP	((ULONG)0x0000274eL)

/* MessageId  : 0x0000274f */
/* Approx. msg: WSAENAMETOOLONG - Name component or name was too long. */
#define WSAENAMETOOLONG	((ULONG)0x0000274fL)

/* MessageId  : 0x00002750 */
/* Approx. msg: WSAEHOSTDOWN - A socket operation failed because the destination host was down. */
#define WSAEHOSTDOWN	((ULONG)0x00002750L)

/* MessageId  : 0x00002751 */
/* Approx. msg: WSAEHOSTUNREACH - A socket operation was attempted to an unreachable host. */
#define WSAEHOSTUNREACH	((ULONG)0x00002751L)

/* MessageId  : 0x00002752 */
/* Approx. msg: WSAENOTEMPTY - Cannot remove a directory that is not empty. */
#define WSAENOTEMPTY	((ULONG)0x00002752L)

/* MessageId  : 0x00002753 */
/* Approx. msg: WSAEPROCLIM - A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously. */
#define WSAEPROCLIM	((ULONG)0x00002753L)

/* MessageId  : 0x00002754 */
/* Approx. msg: WSAEUSERS - Ran out of quota. */
#define WSAEUSERS	((ULONG)0x00002754L)

/* MessageId  : 0x00002755 */
/* Approx. msg: WSAEDQUOT - Ran out of disk quota. */
#define WSAEDQUOT	((ULONG)0x00002755L)

/* MessageId  : 0x00002756 */
/* Approx. msg: WSAESTALE - File handle reference is no longer available. */
#define WSAESTALE	((ULONG)0x00002756L)

/* MessageId  : 0x00002757 */
/* Approx. msg: WSAEREMOTE - Item is not available locally. */
#define WSAEREMOTE	((ULONG)0x00002757L)

/* MessageId  : 0x0000276b */
/* Approx. msg: WSASYSNOTREADY - WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable. */
#define WSASYSNOTREADY	((ULONG)0x0000276bL)

/* MessageId  : 0x0000276c */
/* Approx. msg: WSAVERNOTSUPPORTED - The Windows Sockets version requested is not supported. */
#define WSAVERNOTSUPPORTED	((ULONG)0x0000276cL)

/* MessageId  : 0x0000276d */
/* Approx. msg: WSANOTINITIALISED - Either the application has not called WSAStartup, or WSAStartup failed. */
#define WSANOTINITIALISED	((ULONG)0x0000276dL)

/* MessageId  : 0x00002775 */
/* Approx. msg: WSAEDISCON - Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence. */
#define WSAEDISCON	((ULONG)0x00002775L)

/* MessageId  : 0x00002776 */
/* Approx. msg: WSAENOMORE - No more results can be returned by WSALookupServiceNext. */
#define WSAENOMORE	((ULONG)0x00002776L)

/* MessageId  : 0x00002777 */
/* Approx. msg: WSAECANCELLED - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled. */
#define WSAECANCELLED	((ULONG)0x00002777L)

/* MessageId  : 0x00002778 */
/* Approx. msg: WSAEINVALIDPROCTABLE - The procedure call table is invalid. */
#define WSAEINVALIDPROCTABLE	((ULONG)0x00002778L)

/* MessageId  : 0x00002779 */
/* Approx. msg: WSAEINVALIDPROVIDER - The requested service provider is invalid. */
#define WSAEINVALIDPROVIDER	((ULONG)0x00002779L)

/* MessageId  : 0x0000277a */
/* Approx. msg: WSAEPROVIDERFAILEDINIT - The requested service provider could not be loaded or initialized. */
#define WSAEPROVIDERFAILEDINIT	((ULONG)0x0000277aL)

/* MessageId  : 0x0000277b */
/* Approx. msg: WSASYSCALLFAILURE - A system call that should never fail has failed. */
#define WSASYSCALLFAILURE	((ULONG)0x0000277bL)

/* MessageId  : 0x0000277c */
/* Approx. msg: WSASERVICE_NOT_FOUND - No such service is known. The service cannot be found in the specified name space. */
#define WSASERVICE_NOT_FOUND	((ULONG)0x0000277cL)

/* MessageId  : 0x0000277d */
/* Approx. msg: WSATYPE_NOT_FOUND - The specified class was not found. */
#define WSATYPE_NOT_FOUND	((ULONG)0x0000277dL)

/* MessageId  : 0x0000277e */
/* Approx. msg: WSA_E_NO_MORE - No more results can be returned by WSALookupServiceNext. */
#define WSA_E_NO_MORE	((ULONG)0x0000277eL)

/* MessageId  : 0x0000277f */
/* Approx. msg: WSA_E_CANCELLED - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled. */
#define WSA_E_CANCELLED	((ULONG)0x0000277fL)

/* MessageId  : 0x00002780 */
/* Approx. msg: WSAEREFUSED - A database query failed because it was actively refused. */
#define WSAEREFUSED	((ULONG)0x00002780L)

/* MessageId  : 0x00002af9 */
/* Approx. msg: WSAHOST_NOT_FOUND - No such host is known. */
#define WSAHOST_NOT_FOUND	((ULONG)0x00002af9L)

/* MessageId  : 0x00002afa */
/* Approx. msg: WSATRY_AGAIN - This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server. */
#define WSATRY_AGAIN	((ULONG)0x00002afaL)

/* MessageId  : 0x00002afb */
/* Approx. msg: WSANO_RECOVERY - A non-recoverable error occurred during a database lookup. */
#define WSANO_RECOVERY	((ULONG)0x00002afbL)

/* MessageId  : 0x00002afc */
/* Approx. msg: WSANO_DATA - The requested name is valid, but no data of the requested type was found. */
#define WSANO_DATA	((ULONG)0x00002afcL)

/* MessageId  : 0x00002afd */
/* Approx. msg: WSA_QOS_RECEIVERS - At least one reserve has arrived. */
#define WSA_QOS_RECEIVERS	((ULONG)0x00002afdL)

/* MessageId  : 0x00002afe */
/* Approx. msg: WSA_QOS_SENDERS - At least one path has arrived. */
#define WSA_QOS_SENDERS	((ULONG)0x00002afeL)

/* MessageId  : 0x00002aff */
/* Approx. msg: WSA_QOS_NO_SENDERS - There are no senders. */
#define WSA_QOS_NO_SENDERS	((ULONG)0x00002affL)

/* MessageId  : 0x00002b00 */
/* Approx. msg: WSA_QOS_NO_RECEIVERS - There are no receivers. */
#define WSA_QOS_NO_RECEIVERS	((ULONG)0x00002b00L)

/* MessageId  : 0x00002b01 */
/* Approx. msg: WSA_QOS_REQUEST_CONFIRMED - Reserve has been confirmed. */
#define WSA_QOS_REQUEST_CONFIRMED	((ULONG)0x00002b01L)

/* MessageId  : 0x00002b02 */
/* Approx. msg: WSA_QOS_ADMISSION_FAILURE - Error due to lack of resources. */
#define WSA_QOS_ADMISSION_FAILURE	((ULONG)0x00002b02L)

/* MessageId  : 0x00002b03 */
/* Approx. msg: WSA_QOS_POLICY_FAILURE - Rejected for administrative reasons - bad credentials. */
#define WSA_QOS_POLICY_FAILURE	((ULONG)0x00002b03L)

/* MessageId  : 0x00002b04 */
/* Approx. msg: WSA_QOS_BAD_STYLE - Unknown or conflicting style. */
#define WSA_QOS_BAD_STYLE	((ULONG)0x00002b04L)

/* MessageId  : 0x00002b05 */
/* Approx. msg: WSA_QOS_BAD_OBJECT - Problem with some part of the filterspec or providerspecific buffer in general. */
#define WSA_QOS_BAD_OBJECT	((ULONG)0x00002b05L)

/* MessageId  : 0x00002b06 */
/* Approx. msg: WSA_QOS_TRAFFIC_CTRL_ERROR - Problem with some part of the flowspec. */
#define WSA_QOS_TRAFFIC_CTRL_ERROR	((ULONG)0x00002b06L)

/* MessageId  : 0x00002b07 */
/* Approx. msg: WSA_QOS_GENERIC_ERROR - General QOS error. */
#define WSA_QOS_GENERIC_ERROR	((ULONG)0x00002b07L)

/* MessageId  : 0x00002b08 */
/* Approx. msg: WSA_QOS_ESERVICETYPE - An invalid or unrecognized service type was found in the flowspec. */
#define WSA_QOS_ESERVICETYPE	((ULONG)0x00002b08L)

/* MessageId  : 0x00002b09 */
/* Approx. msg: WSA_QOS_EFLOWSPEC - An invalid or inconsistent flowspec was found in the QOS structure. */
#define WSA_QOS_EFLOWSPEC	((ULONG)0x00002b09L)

/* MessageId  : 0x00002b0a */
/* Approx. msg: WSA_QOS_EPROVSPECBUF - Invalid QOS provider-specific buffer. */
#define WSA_QOS_EPROVSPECBUF	((ULONG)0x00002b0aL)

/* MessageId  : 0x00002b0b */
/* Approx. msg: WSA_QOS_EFILTERSTYLE - An invalid QOS filter style was used. */
#define WSA_QOS_EFILTERSTYLE	((ULONG)0x00002b0bL)

/* MessageId  : 0x00002b0c */
/* Approx. msg: WSA_QOS_EFILTERTYPE - An invalid QOS filter type was used. */
#define WSA_QOS_EFILTERTYPE	((ULONG)0x00002b0cL)

/* MessageId  : 0x00002b0d */
/* Approx. msg: WSA_QOS_EFILTERCOUNT - An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR. */
#define WSA_QOS_EFILTERCOUNT	((ULONG)0x00002b0dL)

/* MessageId  : 0x00002b0e */
/* Approx. msg: WSA_QOS_EOBJLENGTH - An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer. */
#define WSA_QOS_EOBJLENGTH	((ULONG)0x00002b0eL)

/* MessageId  : 0x00002b0f */
/* Approx. msg: WSA_QOS_EFLOWCOUNT - An incorrect number of flow descriptors was specified in the QOS structure. */
#define WSA_QOS_EFLOWCOUNT	((ULONG)0x00002b0fL)

/* MessageId  : 0x00002b10 */
/* Approx. msg: WSA_QOS_EUNKNOWNPSOBJ - An unrecognized object was found in the QOS provider-specific buffer. */
#define WSA_QOS_EUNKNOWNPSOBJ	((ULONG)0x00002b10L)

/* MessageId  : 0x00002b11 */
/* Approx. msg: WSA_QOS_EPOLICYOBJ - An invalid policy object was found in the QOS provider-specific buffer. */
#define WSA_QOS_EPOLICYOBJ	((ULONG)0x00002b11L)

/* MessageId  : 0x00002b12 */
/* Approx. msg: WSA_QOS_EFLOWDESC - An invalid QOS flow descriptor was found in the flow descriptor list. */
#define WSA_QOS_EFLOWDESC	((ULONG)0x00002b12L)

/* MessageId  : 0x00002b13 */
/* Approx. msg: WSA_QOS_EPSFLOWSPEC - An invalid or inconsistent flowspec was found in the QOS provider-specific buffer. */
#define WSA_QOS_EPSFLOWSPEC	((ULONG)0x00002b13L)

/* MessageId  : 0x00002b14 */
/* Approx. msg: WSA_QOS_EPSFILTERSPEC - An invalid FILTERSPEC was found in the QOS provider-specific buffer. */
#define WSA_QOS_EPSFILTERSPEC	((ULONG)0x00002b14L)

/* MessageId  : 0x00002b15 */
/* Approx. msg: WSA_QOS_ESDMODEOBJ - An invalid shape discard mode object was found in the QOS provider-specific buffer. */
#define WSA_QOS_ESDMODEOBJ	((ULONG)0x00002b15L)

/* MessageId  : 0x00002b16 */
/* Approx. msg: WSA_QOS_ESHAPERATEOBJ - An invalid shaping rate object was found in the QOS provider-specific buffer. */
#define WSA_QOS_ESHAPERATEOBJ	((ULONG)0x00002b16L)

/* MessageId  : 0x00002b17 */
/* Approx. msg: WSA_QOS_RESERVED_PETYPE - A reserved policy element was found in the QOS provider-specific buffer. */
#define WSA_QOS_RESERVED_PETYPE	((ULONG)0x00002b17L)

/* MessageId  : 0x00002ee0 */
/* Approx. msg: ERROR_FLT_IO_COMPLETE - The IO was completed by a filter. */
#define ERROR_FLT_IO_COMPLETE	((ULONG)0x00002ee0L)

/* MessageId  : 0x00002ee1 */
/* Approx. msg: ERROR_FLT_BUFFER_TOO_SMALL - The buffer is too small to contain the entry. No information has been written to the buffer. */
#define ERROR_FLT_BUFFER_TOO_SMALL	((ULONG)0x00002ee1L)

/* MessageId  : 0x00002ee2 */
/* Approx. msg: ERROR_FLT_NO_HANDLER_DEFINED - A handler was not defined by the filter for this operation. */
#define ERROR_FLT_NO_HANDLER_DEFINED	((ULONG)0x00002ee2L)

/* MessageId  : 0x00002ee3 */
/* Approx. msg: ERROR_FLT_CONTEXT_ALREADY_DEFINED - A context is already defined for this object. */
#define ERROR_FLT_CONTEXT_ALREADY_DEFINED	((ULONG)0x00002ee3L)

/* MessageId  : 0x00002ee4 */
/* Approx. msg: ERROR_FLT_INVALID_ASYNCHRONOUS_REQUEST - Asynchronous requests are not valid for this operation. */
#define ERROR_FLT_INVALID_ASYNCHRONOUS_REQUEST	((ULONG)0x00002ee4L)

/* MessageId  : 0x00002ee5 */
/* Approx. msg: ERROR_FLT_DISALLOW_FAST_IO - Disallow the Fast IO path for this operation. */
#define ERROR_FLT_DISALLOW_FAST_IO	((ULONG)0x00002ee5L)

/* MessageId  : 0x00002ee6 */
/* Approx. msg: ERROR_FLT_INVALID_NAME_REQUEST - An invalid name request was made. The name requested cannot be retrieved at this time. */
#define ERROR_FLT_INVALID_NAME_REQUEST	((ULONG)0x00002ee6L)

/* MessageId  : 0x00002ee7 */
/* Approx. msg: ERROR_FLT_NOT_SAFE_TO_POST_OPERATION - Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock. */
#define ERROR_FLT_NOT_SAFE_TO_POST_OPERATION	((ULONG)0x00002ee7L)

/* MessageId  : 0x00002ee8 */
/* Approx. msg: ERROR_FLT_NOT_INITIALIZED - The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver. */
#define ERROR_FLT_NOT_INITIALIZED	((ULONG)0x00002ee8L)

/* MessageId  : 0x00002ee9 */
/* Approx. msg: ERROR_FLT_FILTER_NOT_READY - The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called). */
#define ERROR_FLT_FILTER_NOT_READY	((ULONG)0x00002ee9L)

/* MessageId  : 0x00002eea */
/* Approx. msg: ERROR_FLT_POST_OPERATION_CLEANUP - The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers. */
#define ERROR_FLT_POST_OPERATION_CLEANUP	((ULONG)0x00002eeaL)

/* MessageId  : 0x00002eeb */
/* Approx. msg: ERROR_FLT_INTERNAL_ERROR - The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback. */
#define ERROR_FLT_INTERNAL_ERROR	((ULONG)0x00002eebL)

/* MessageId  : 0x00002eec */
/* Approx. msg: ERROR_FLT_DELETING_OBJECT - The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time. */
#define ERROR_FLT_DELETING_OBJECT	((ULONG)0x00002eecL)

/* MessageId  : 0x00002eed */
/* Approx. msg: ERROR_FLT_MUST_BE_NONPAGED_POOL - Non-paged pool must be used for this type of context. */
#define ERROR_FLT_MUST_BE_NONPAGED_POOL	((ULONG)0x00002eedL)

/* MessageId  : 0x00002eee */
/* Approx. msg: ERROR_FLT_DUPLICATE_ENTRY - A duplicate handler definition has been provided for an operation. */
#define ERROR_FLT_DUPLICATE_ENTRY	((ULONG)0x00002eeeL)

/* MessageId  : 0x00002eef */
/* Approx. msg: ERROR_FLT_CBDQ_DISABLED - The callback data queue has been disabled. */
#define ERROR_FLT_CBDQ_DISABLED	((ULONG)0x00002eefL)

/* MessageId  : 0x00002ef0 */
/* Approx. msg: ERROR_FLT_DO_NOT_ATTACH - Do not attach the filter to the volume at this time. */
#define ERROR_FLT_DO_NOT_ATTACH	((ULONG)0x00002ef0L)

/* MessageId  : 0x00002ef1 */
/* Approx. msg: ERROR_FLT_DO_NOT_DETACH - Do not detach the filter from the volume at this time. */
#define ERROR_FLT_DO_NOT_DETACH	((ULONG)0x00002ef1L)

/* MessageId  : 0x00002ef2 */
/* Approx. msg: ERROR_FLT_INSTANCE_ALTITUDE_COLLISION - An instance already exists at this altitude on the volume specified. */
#define ERROR_FLT_INSTANCE_ALTITUDE_COLLISION	((ULONG)0x00002ef2L)

/* MessageId  : 0x00002ef3 */
/* Approx. msg: ERROR_FLT_INSTANCE_NAME_COLLISION - An instance already exists with this name on the volume specified. */
#define ERROR_FLT_INSTANCE_NAME_COLLISION	((ULONG)0x00002ef3L)

/* MessageId  : 0x00002ef4 */
/* Approx. msg: ERROR_FLT_FILTER_NOT_FOUND - The system could not find the filter specified. */
#define ERROR_FLT_FILTER_NOT_FOUND	((ULONG)0x00002ef4L)

/* MessageId  : 0x00002ef5 */
/* Approx. msg: ERROR_FLT_VOLUME_NOT_FOUND - The system could not find the volume specified. */
#define ERROR_FLT_VOLUME_NOT_FOUND	((ULONG)0x00002ef5L)

/* MessageId  : 0x00002ef6 */
/* Approx. msg: ERROR_FLT_INSTANCE_NOT_FOUND - The system could not find the instance specified. */
#define ERROR_FLT_INSTANCE_NOT_FOUND	((ULONG)0x00002ef6L)

/* MessageId  : 0x00002ef7 */
/* Approx. msg: ERROR_FLT_CONTEXT_ALLOCATION_NOT_FOUND - No registered context allocation definition was found for the given request. */
#define ERROR_FLT_CONTEXT_ALLOCATION_NOT_FOUND	((ULONG)0x00002ef7L)

/* MessageId  : 0x00002ef8 */
/* Approx. msg: ERROR_FLT_INVALID_CONTEXT_REGISTRATION - An invalid parameter was specified during context registration. */
#define ERROR_FLT_INVALID_CONTEXT_REGISTRATION	((ULONG)0x00002ef8L)

/* MessageId  : 0x00002ef9 */
/* Approx. msg: ERROR_FLT_NAME_CACHE_MISS - The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system. */
#define ERROR_FLT_NAME_CACHE_MISS	((ULONG)0x00002ef9L)

/* MessageId  : 0x00002efa */
/* Approx. msg: ERROR_FLT_NO_DEVICE_OBJECT - The requested device object does not exist for the given volume. */
#define ERROR_FLT_NO_DEVICE_OBJECT	((ULONG)0x00002efaL)

/* MessageId  : 0x00002efb */
/* Approx. msg: ERROR_FLT_VOLUME_ALREADY_MOUNTED - The specified volume is already mounted. */
#define ERROR_FLT_VOLUME_ALREADY_MOUNTED	((ULONG)0x00002efbL)

/* MessageId  : 0x00002efc */
/* Approx. msg: ERROR_FLT_NO_WAITER_FOR_REPLY - No waiter is present for the filter's reply to this message. */
#define ERROR_FLT_NO_WAITER_FOR_REPLY	((ULONG)0x00002efcL)

/* MessageId  : 0x000032c8 */
/* Approx. msg: ERROR_IPSEC_QM_POLICY_EXISTS - The specified quick mode policy already exists. */
#define ERROR_IPSEC_QM_POLICY_EXISTS	((ULONG)0x000032c8L)

/* MessageId  : 0x000032c9 */
/* Approx. msg: ERROR_IPSEC_QM_POLICY_NOT_FOUND - The specified quick mode policy was not found. */
#define ERROR_IPSEC_QM_POLICY_NOT_FOUND	((ULONG)0x000032c9L)

/* MessageId  : 0x000032ca */
/* Approx. msg: ERROR_IPSEC_QM_POLICY_IN_USE - The specified quick mode policy is being used. */
#define ERROR_IPSEC_QM_POLICY_IN_USE	((ULONG)0x000032caL)

/* MessageId  : 0x000032cb */
/* Approx. msg: ERROR_IPSEC_MM_POLICY_EXISTS - The specified main mode policy already exists. */
#define ERROR_IPSEC_MM_POLICY_EXISTS	((ULONG)0x000032cbL)

/* MessageId  : 0x000032cc */
/* Approx. msg: ERROR_IPSEC_MM_POLICY_NOT_FOUND - The specified main mode policy was not found. */
#define ERROR_IPSEC_MM_POLICY_NOT_FOUND	((ULONG)0x000032ccL)

/* MessageId  : 0x000032cd */
/* Approx. msg: ERROR_IPSEC_MM_POLICY_IN_USE - The specified main mode policy is being used. */
#define ERROR_IPSEC_MM_POLICY_IN_USE	((ULONG)0x000032cdL)

/* MessageId  : 0x000032ce */
/* Approx. msg: ERROR_IPSEC_MM_FILTER_EXISTS - The specified main mode filter already exists. */
#define ERROR_IPSEC_MM_FILTER_EXISTS	((ULONG)0x000032ceL)

/* MessageId  : 0x000032cf */
/* Approx. msg: ERROR_IPSEC_MM_FILTER_NOT_FOUND - The specified main mode filter was not found. */
#define ERROR_IPSEC_MM_FILTER_NOT_FOUND	((ULONG)0x000032cfL)

/* MessageId  : 0x000032d0 */
/* Approx. msg: ERROR_IPSEC_TRANSPORT_FILTER_EXISTS - The specified transport mode filter already exists. */
#define ERROR_IPSEC_TRANSPORT_FILTER_EXISTS	((ULONG)0x000032d0L)

/* MessageId  : 0x000032d1 */
/* Approx. msg: ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND - The specified transport mode filter does not exist. */
#define ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND	((ULONG)0x000032d1L)

/* MessageId  : 0x000032d2 */
/* Approx. msg: ERROR_IPSEC_MM_AUTH_EXISTS - The specified main mode authentication list exists. */
#define ERROR_IPSEC_MM_AUTH_EXISTS	((ULONG)0x000032d2L)

/* MessageId  : 0x000032d3 */
/* Approx. msg: ERROR_IPSEC_MM_AUTH_NOT_FOUND - The specified main mode authentication list was not found. */
#define ERROR_IPSEC_MM_AUTH_NOT_FOUND	((ULONG)0x000032d3L)

/* MessageId  : 0x000032d4 */
/* Approx. msg: ERROR_IPSEC_MM_AUTH_IN_USE - The specified quick mode policy is being used. */
#define ERROR_IPSEC_MM_AUTH_IN_USE	((ULONG)0x000032d4L)

/* MessageId  : 0x000032d5 */
/* Approx. msg: ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND - The specified main mode policy was not found. */
#define ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND	((ULONG)0x000032d5L)

/* MessageId  : 0x000032d6 */
/* Approx. msg: ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND - The specified quick mode policy was not found. */
#define ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND	((ULONG)0x000032d6L)

/* MessageId  : 0x000032d7 */
/* Approx. msg: ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND - The manifest file contains one or more syntax errors. */
#define ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND	((ULONG)0x000032d7L)

/* MessageId  : 0x000032d8 */
/* Approx. msg: ERROR_IPSEC_TUNNEL_FILTER_EXISTS - The application attempted to activate a disabled activation context. */
#define ERROR_IPSEC_TUNNEL_FILTER_EXISTS	((ULONG)0x000032d8L)

/* MessageId  : 0x000032d9 */
/* Approx. msg: ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND - The requested lookup key was not found in any active activation context. */
#define ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND	((ULONG)0x000032d9L)

/* MessageId  : 0x000032da */
/* Approx. msg: ERROR_IPSEC_MM_FILTER_PENDING_DELETION - The Main Mode filter is pending deletion. */
#define ERROR_IPSEC_MM_FILTER_PENDING_DELETION	((ULONG)0x000032daL)

/* MessageId  : 0x000032db */
/* Approx. msg: ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION - The transport filter is pending deletion. */
#define ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION	((ULONG)0x000032dbL)

/* MessageId  : 0x000032dc */
/* Approx. msg: ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION - The tunnel filter is pending deletion. */
#define ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION	((ULONG)0x000032dcL)

/* MessageId  : 0x000032dd */
/* Approx. msg: ERROR_IPSEC_MM_POLICY_PENDING_DELETION - The Main Mode policy is pending deletion. */
#define ERROR_IPSEC_MM_POLICY_PENDING_DELETION	((ULONG)0x000032ddL)

/* MessageId  : 0x000032de */
/* Approx. msg: ERROR_IPSEC_MM_AUTH_PENDING_DELETION - The Main Mode authentication bundle is pending deletion. */
#define ERROR_IPSEC_MM_AUTH_PENDING_DELETION	((ULONG)0x000032deL)

/* MessageId  : 0x000032df */
/* Approx. msg: ERROR_IPSEC_QM_POLICY_PENDING_DELETION - The Quick Mode policy is pending deletion. */
#define ERROR_IPSEC_QM_POLICY_PENDING_DELETION	((ULONG)0x000032dfL)

/* MessageId  : 0x000032e0 */
/* Approx. msg: WARNING_IPSEC_MM_POLICY_PRUNED - The Main Mode policy was successfully added, but some of the requested offers are not supported. */
#define WARNING_IPSEC_MM_POLICY_PRUNED	((ULONG)0x000032e0L)

/* MessageId  : 0x000032e1 */
/* Approx. msg: WARNING_IPSEC_QM_POLICY_PRUNED - The Quick Mode policy was successfully added, but some of the requested offers are not supported. */
#define WARNING_IPSEC_QM_POLICY_PRUNED	((ULONG)0x000032e1L)

/* MessageId  : 0x000035e9 */
/* Approx. msg: ERROR_IPSEC_IKE_AUTH_FAIL - IKE authentication credentials are unacceptable. */
#define ERROR_IPSEC_IKE_AUTH_FAIL	((ULONG)0x000035e9L)

/* MessageId  : 0x000035ea */
/* Approx. msg: ERROR_IPSEC_IKE_ATTRIB_FAIL - IKE security attributes are unacceptable. */
#define ERROR_IPSEC_IKE_ATTRIB_FAIL	((ULONG)0x000035eaL)

/* MessageId  : 0x000035eb */
/* Approx. msg: ERROR_IPSEC_IKE_NEGOTIATION_PENDING - IKE Negotiation in progress. */
#define ERROR_IPSEC_IKE_NEGOTIATION_PENDING	((ULONG)0x000035ebL)

/* MessageId  : 0x000035ec */
/* Approx. msg: ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR - General processing error. */
#define ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR	((ULONG)0x000035ecL)

/* MessageId  : 0x000035ed */
/* Approx. msg: ERROR_IPSEC_IKE_TIMED_OUT - Negotiation timed out. */
#define ERROR_IPSEC_IKE_TIMED_OUT	((ULONG)0x000035edL)

/* MessageId  : 0x000035ee */
/* Approx. msg: ERROR_IPSEC_IKE_NO_CERT - IKE failed to find valid machine certificate. */
#define ERROR_IPSEC_IKE_NO_CERT	((ULONG)0x000035eeL)

/* MessageId  : 0x000035ef */
/* Approx. msg: ERROR_IPSEC_IKE_SA_DELETED - IKE SA deleted by peer before establishment completed. */
#define ERROR_IPSEC_IKE_SA_DELETED	((ULONG)0x000035efL)

/* MessageId  : 0x000035f0 */
/* Approx. msg: ERROR_IPSEC_IKE_SA_REAPED - IKE SA deleted before establishment completed. */
#define ERROR_IPSEC_IKE_SA_REAPED	((ULONG)0x000035f0L)

/* MessageId  : 0x000035f1 */
/* Approx. msg: ERROR_IPSEC_IKE_MM_ACQUIRE_DROP - Negotiation request sat in Queue too long. */
#define ERROR_IPSEC_IKE_MM_ACQUIRE_DROP	((ULONG)0x000035f1L)

/* MessageId  : 0x000035f2 */
/* Approx. msg: ERROR_IPSEC_IKE_QM_ACQUIRE_DROP - Negotiation request sat in Queue too long. */
#define ERROR_IPSEC_IKE_QM_ACQUIRE_DROP	((ULONG)0x000035f2L)

/* MessageId  : 0x000035f3 */
/* Approx. msg: ERROR_IPSEC_IKE_QUEUE_DROP_MM - Negotiation request sat in Queue too long. */
#define ERROR_IPSEC_IKE_QUEUE_DROP_MM	((ULONG)0x000035f3L)

/* MessageId  : 0x000035f4 */
/* Approx. msg: ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM - Negotiation request sat in Queue too long. */
#define ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM	((ULONG)0x000035f4L)

/* MessageId  : 0x000035f5 */
/* Approx. msg: ERROR_IPSEC_IKE_DROP_NO_RESPONSE - No response from peer. */
#define ERROR_IPSEC_IKE_DROP_NO_RESPONSE	((ULONG)0x000035f5L)

/* MessageId  : 0x000035f6 */
/* Approx. msg: ERROR_IPSEC_IKE_MM_DELAY_DROP - Negotiation took too long. */
#define ERROR_IPSEC_IKE_MM_DELAY_DROP	((ULONG)0x000035f6L)

/* MessageId  : 0x000035f7 */
/* Approx. msg: ERROR_IPSEC_IKE_QM_DELAY_DROP - Negotiation took too long. */
#define ERROR_IPSEC_IKE_QM_DELAY_DROP	((ULONG)0x000035f7L)

/* MessageId  : 0x000035f8 */
/* Approx. msg: ERROR_IPSEC_IKE_ERROR - Unknown error occurred. */
#define ERROR_IPSEC_IKE_ERROR	((ULONG)0x000035f8L)

/* MessageId  : 0x000035f9 */
/* Approx. msg: ERROR_IPSEC_IKE_CRL_FAILED - Certificate Revocation Check failed. */
#define ERROR_IPSEC_IKE_CRL_FAILED	((ULONG)0x000035f9L)

/* MessageId  : 0x000035fa */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_KEY_USAGE - Invalid certificate key usage. */
#define ERROR_IPSEC_IKE_INVALID_KEY_USAGE	((ULONG)0x000035faL)

/* MessageId  : 0x000035fb */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_CERT_TYPE - Invalid certificate type. */
#define ERROR_IPSEC_IKE_INVALID_CERT_TYPE	((ULONG)0x000035fbL)

/* MessageId  : 0x000035fc */
/* Approx. msg: ERROR_IPSEC_IKE_NO_PRIVATE_KEY - No private key associated with machine certificate. */
#define ERROR_IPSEC_IKE_NO_PRIVATE_KEY	((ULONG)0x000035fcL)

/* MessageId  : 0x000035fe */
/* Approx. msg: ERROR_IPSEC_IKE_DH_FAIL - Failure in Diffie-Hellman computation. */
#define ERROR_IPSEC_IKE_DH_FAIL	((ULONG)0x000035feL)

/* MessageId  : 0x00003600 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_HEADER - Invalid header. */
#define ERROR_IPSEC_IKE_INVALID_HEADER	((ULONG)0x00003600L)

/* MessageId  : 0x00003601 */
/* Approx. msg: ERROR_IPSEC_IKE_NO_POLICY - No policy configured. */
#define ERROR_IPSEC_IKE_NO_POLICY	((ULONG)0x00003601L)

/* MessageId  : 0x00003602 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_SIGNATURE - Failed to verify signature. */
#define ERROR_IPSEC_IKE_INVALID_SIGNATURE	((ULONG)0x00003602L)

/* MessageId  : 0x00003603 */
/* Approx. msg: ERROR_IPSEC_IKE_KERBEROS_ERROR - Failed to authenticate using Kerberos. */
#define ERROR_IPSEC_IKE_KERBEROS_ERROR	((ULONG)0x00003603L)

/* MessageId  : 0x00003604 */
/* Approx. msg: ERROR_IPSEC_IKE_NO_PUBLIC_KEY - Peer's certificate did not have a public key. */
#define ERROR_IPSEC_IKE_NO_PUBLIC_KEY	((ULONG)0x00003604L)

/* MessageId  : 0x00003605 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR - Error processing error payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR	((ULONG)0x00003605L)

/* MessageId  : 0x00003606 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_SA - Error processing SA payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_SA	((ULONG)0x00003606L)

/* MessageId  : 0x00003607 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_PROP - Error processing Proposal payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_PROP	((ULONG)0x00003607L)

/* MessageId  : 0x00003608 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_TRANS - Error processing Transform payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_TRANS	((ULONG)0x00003608L)

/* MessageId  : 0x00003609 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_KE - Error processing KE payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_KE	((ULONG)0x00003609L)

/* MessageId  : 0x0000360a */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_ID - Error processing ID payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_ID	((ULONG)0x0000360aL)

/* MessageId  : 0x0000360b */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_CERT - Error processing Cert payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_CERT	((ULONG)0x0000360bL)

/* MessageId  : 0x0000360c */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ - Error processing Certificate Request payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ	((ULONG)0x0000360cL)

/* MessageId  : 0x0000360d */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_HASH - Error processing Hash payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_HASH	((ULONG)0x0000360dL)

/* MessageId  : 0x0000360e */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_SIG - Error processing Signature payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_SIG	((ULONG)0x0000360eL)

/* MessageId  : 0x0000360f */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_NONCE - Error processing Nonce payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_NONCE	((ULONG)0x0000360fL)

/* MessageId  : 0x00003610 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY - Error processing Notify payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY	((ULONG)0x00003610L)

/* MessageId  : 0x00003611 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_DELETE - Error processing Delete Payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_DELETE	((ULONG)0x00003611L)

/* MessageId  : 0x00003612 */
/* Approx. msg: ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR - Error processing VendorId payload. */
#define ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR	((ULONG)0x00003612L)

/* MessageId  : 0x00003613 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_PAYLOAD - Invalid payload received. */
#define ERROR_IPSEC_IKE_INVALID_PAYLOAD	((ULONG)0x00003613L)

/* MessageId  : 0x00003614 */
/* Approx. msg: ERROR_IPSEC_IKE_LOAD_SOFT_SA - Soft SA loaded. */
#define ERROR_IPSEC_IKE_LOAD_SOFT_SA	((ULONG)0x00003614L)

/* MessageId  : 0x00003615 */
/* Approx. msg: ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN - Soft SA torn down. */
#define ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN	((ULONG)0x00003615L)

/* MessageId  : 0x00003616 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_COOKIE - Invalid cookie received.. */
#define ERROR_IPSEC_IKE_INVALID_COOKIE	((ULONG)0x00003616L)

/* MessageId  : 0x00003617 */
/* Approx. msg: ERROR_IPSEC_IKE_NO_PEER_CERT - Peer failed to send valid machine certificate. */
#define ERROR_IPSEC_IKE_NO_PEER_CERT	((ULONG)0x00003617L)

/* MessageId  : 0x00003618 */
/* Approx. msg: ERROR_IPSEC_IKE_PEER_CRL_FAILED - Certification Revocation check of peer's certificate failed. */
#define ERROR_IPSEC_IKE_PEER_CRL_FAILED	((ULONG)0x00003618L)

/* MessageId  : 0x00003619 */
/* Approx. msg: ERROR_IPSEC_IKE_POLICY_CHANGE - New policy invalidated SAs formed with old policy. */
#define ERROR_IPSEC_IKE_POLICY_CHANGE	((ULONG)0x00003619L)

/* MessageId  : 0x0000361a */
/* Approx. msg: ERROR_IPSEC_IKE_NO_MM_POLICY - There is no available Main Mode IKE policy. */
#define ERROR_IPSEC_IKE_NO_MM_POLICY	((ULONG)0x0000361aL)

/* MessageId  : 0x0000361b */
/* Approx. msg: ERROR_IPSEC_IKE_NOTCBPRIV - Failed to enabled TCB privilege. */
#define ERROR_IPSEC_IKE_NOTCBPRIV	((ULONG)0x0000361bL)

/* MessageId  : 0x0000361c */
/* Approx. msg: ERROR_IPSEC_IKE_SECLOADFAIL - Failed to load SECURITY.DLL. */
#define ERROR_IPSEC_IKE_SECLOADFAIL	((ULONG)0x0000361cL)

/* MessageId  : 0x0000361d */
/* Approx. msg: ERROR_IPSEC_IKE_FAILSSPINIT - Failed to obtain security function table dispatch address from SSPI. */
#define ERROR_IPSEC_IKE_FAILSSPINIT	((ULONG)0x0000361dL)

/* MessageId  : 0x0000361e */
/* Approx. msg: ERROR_IPSEC_IKE_FAILQUERYSSP - Failed to query Kerberos package to obtain max token size. */
#define ERROR_IPSEC_IKE_FAILQUERYSSP	((ULONG)0x0000361eL)

/* MessageId  : 0x0000361f */
/* Approx. msg: ERROR_IPSEC_IKE_SRVACQFAIL - Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup. */
#define ERROR_IPSEC_IKE_SRVACQFAIL	((ULONG)0x0000361fL)

/* MessageId  : 0x00003620 */
/* Approx. msg: ERROR_IPSEC_IKE_SRVQUERYCRED - Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes). */
#define ERROR_IPSEC_IKE_SRVQUERYCRED	((ULONG)0x00003620L)

/* MessageId  : 0x00003621 */
/* Approx. msg: ERROR_IPSEC_IKE_GETSPIFAIL - Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters. */
#define ERROR_IPSEC_IKE_GETSPIFAIL	((ULONG)0x00003621L)

/* MessageId  : 0x00003622 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_FILTER - Given filter is invalid. */
#define ERROR_IPSEC_IKE_INVALID_FILTER	((ULONG)0x00003622L)

/* MessageId  : 0x00003623 */
/* Approx. msg: ERROR_IPSEC_IKE_OUT_OF_MEMORY - Memory allocation failed. */
#define ERROR_IPSEC_IKE_OUT_OF_MEMORY	((ULONG)0x00003623L)

/* MessageId  : 0x00003624 */
/* Approx. msg: ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED - Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine. */
#define ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED	((ULONG)0x00003624L)

/* MessageId  : 0x00003625 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_POLICY - Invalid policy. */
#define ERROR_IPSEC_IKE_INVALID_POLICY	((ULONG)0x00003625L)

/* MessageId  : 0x00003626 */
/* Approx. msg: ERROR_IPSEC_IKE_UNKNOWN_DOI - Invalid DOI. */
#define ERROR_IPSEC_IKE_UNKNOWN_DOI	((ULONG)0x00003626L)

/* MessageId  : 0x00003627 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_SITUATION - Invalid situation. */
#define ERROR_IPSEC_IKE_INVALID_SITUATION	((ULONG)0x00003627L)

/* MessageId  : 0x00003628 */
/* Approx. msg: ERROR_IPSEC_IKE_DH_FAILURE - Diffie-Hellman failure. */
#define ERROR_IPSEC_IKE_DH_FAILURE	((ULONG)0x00003628L)

/* MessageId  : 0x00003629 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_GROUP - Invalid Diffie-Hellman group. */
#define ERROR_IPSEC_IKE_INVALID_GROUP	((ULONG)0x00003629L)

/* MessageId  : 0x0000362a */
/* Approx. msg: ERROR_IPSEC_IKE_ENCRYPT - Error encrypting payload. */
#define ERROR_IPSEC_IKE_ENCRYPT	((ULONG)0x0000362aL)

/* MessageId  : 0x0000362b */
/* Approx. msg: ERROR_IPSEC_IKE_DECRYPT - Error decrypting payload. */
#define ERROR_IPSEC_IKE_DECRYPT	((ULONG)0x0000362bL)

/* MessageId  : 0x0000362c */
/* Approx. msg: ERROR_IPSEC_IKE_POLICY_MATCH - Policy match error. */
#define ERROR_IPSEC_IKE_POLICY_MATCH	((ULONG)0x0000362cL)

/* MessageId  : 0x0000362d */
/* Approx. msg: ERROR_IPSEC_IKE_UNSUPPORTED_ID - Unsupported ID. */
#define ERROR_IPSEC_IKE_UNSUPPORTED_ID	((ULONG)0x0000362dL)

/* MessageId  : 0x0000362e */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_HASH - Hash verification failed. */
#define ERROR_IPSEC_IKE_INVALID_HASH	((ULONG)0x0000362eL)

/* MessageId  : 0x0000362f */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_HASH_ALG - Invalid hash algorithm. */
#define ERROR_IPSEC_IKE_INVALID_HASH_ALG	((ULONG)0x0000362fL)

/* MessageId  : 0x00003630 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_HASH_SIZE - Invalid hash size. */
#define ERROR_IPSEC_IKE_INVALID_HASH_SIZE	((ULONG)0x00003630L)

/* MessageId  : 0x00003631 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG - Invalid encryption algorithm. */
#define ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG	((ULONG)0x00003631L)

/* MessageId  : 0x00003632 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_AUTH_ALG - Invalid authentication algorithm. */
#define ERROR_IPSEC_IKE_INVALID_AUTH_ALG	((ULONG)0x00003632L)

/* MessageId  : 0x00003633 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_SIG - Invalid certificate signature. */
#define ERROR_IPSEC_IKE_INVALID_SIG	((ULONG)0x00003633L)

/* MessageId  : 0x00003634 */
/* Approx. msg: ERROR_IPSEC_IKE_LOAD_FAILED - Load failed. */
#define ERROR_IPSEC_IKE_LOAD_FAILED	((ULONG)0x00003634L)

/* MessageId  : 0x00003635 */
/* Approx. msg: ERROR_IPSEC_IKE_RPC_DELETE - Deleted via RPC call. */
#define ERROR_IPSEC_IKE_RPC_DELETE	((ULONG)0x00003635L)

/* MessageId  : 0x00003636 */
/* Approx. msg: ERROR_IPSEC_IKE_BENIGN_REINIT - Temporary state created to perform reinit. This is not a real failure. */
#define ERROR_IPSEC_IKE_BENIGN_REINIT	((ULONG)0x00003636L)

/* MessageId  : 0x00003637 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY - The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine. */
#define ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY	((ULONG)0x00003637L)

/* MessageId  : 0x00003639 */
/* Approx. msg: ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN - Key length in certificate is too small for configured security requirements. */
#define ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN	((ULONG)0x00003639L)

/* MessageId  : 0x0000363a */
/* Approx. msg: ERROR_IPSEC_IKE_MM_LIMIT - Max number of established MM SAs to peer exceeded. */
#define ERROR_IPSEC_IKE_MM_LIMIT	((ULONG)0x0000363aL)

/* MessageId  : 0x0000363b */
/* Approx. msg: ERROR_IPSEC_IKE_NEGOTIATION_DISABLED - IKE received a policy that disables negotiation. */
#define ERROR_IPSEC_IKE_NEGOTIATION_DISABLED	((ULONG)0x0000363bL)

/* MessageId  : 0x0000363c */
/* Approx. msg: ERROR_IPSEC_IKE_NEG_STATUS_END - ERROR_IPSEC_IKE_NEG_STATUS_END */
#define ERROR_IPSEC_IKE_NEG_STATUS_END	((ULONG)0x0000363cL)

/* MessageId  : 0x000036b0 */
/* Approx. msg: ERROR_SXS_SECTION_NOT_FOUND - The requested section was not present in the activation context. */
#define ERROR_SXS_SECTION_NOT_FOUND	((ULONG)0x000036b0L)

/* MessageId  : 0x000036b1 */
/* Approx. msg: ERROR_SXS_CANT_GEN_ACTCTX - This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem. */
#define ERROR_SXS_CANT_GEN_ACTCTX	((ULONG)0x000036b1L)

/* MessageId  : 0x000036b2 */
/* Approx. msg: ERROR_SXS_INVALID_ACTCTXDATA_FORMAT - The application binding data format is invalid. */
#define ERROR_SXS_INVALID_ACTCTXDATA_FORMAT	((ULONG)0x000036b2L)

/* MessageId  : 0x000036b3 */
/* Approx. msg: ERROR_SXS_ASSEMBLY_NOT_FOUND - The referenced assembly is not installed on your system. */
#define ERROR_SXS_ASSEMBLY_NOT_FOUND	((ULONG)0x000036b3L)

/* MessageId  : 0x000036b4 */
/* Approx. msg: ERROR_SXS_MANIFEST_FORMAT_ERROR - The manifest file does not begin with the required tag and format information. */
#define ERROR_SXS_MANIFEST_FORMAT_ERROR	((ULONG)0x000036b4L)

/* MessageId  : 0x000036b5 */
/* Approx. msg: ERROR_SXS_MANIFEST_PARSE_ERROR - The manifest file contains one or more syntax errors. */
#define ERROR_SXS_MANIFEST_PARSE_ERROR	((ULONG)0x000036b5L)

/* MessageId  : 0x000036b6 */
/* Approx. msg: ERROR_SXS_ACTIVATION_CONTEXT_DISABLED - The application attempted to activate a disabled activation context. */
#define ERROR_SXS_ACTIVATION_CONTEXT_DISABLED	((ULONG)0x000036b6L)

/* MessageId  : 0x000036b7 */
/* Approx. msg: ERROR_SXS_KEY_NOT_FOUND - The requested lookup key was not found in any active activation context. */
#define ERROR_SXS_KEY_NOT_FOUND	((ULONG)0x000036b7L)

/* MessageId  : 0x000036b8 */
/* Approx. msg: ERROR_SXS_VERSION_CONFLICT - A component version required by the application conflicts with another component version already active. */
#define ERROR_SXS_VERSION_CONFLICT	((ULONG)0x000036b8L)

/* MessageId  : 0x000036b9 */
/* Approx. msg: ERROR_SXS_WRONG_SECTION_TYPE - The type requested activation context section does not match the query API used. */
#define ERROR_SXS_WRONG_SECTION_TYPE	((ULONG)0x000036b9L)

/* MessageId  : 0x000036ba */
/* Approx. msg: ERROR_SXS_THREAD_QUERIES_DISABLED - Lack of system resources has required isolated activation to be disabled for the current thread of execution. */
#define ERROR_SXS_THREAD_QUERIES_DISABLED	((ULONG)0x000036baL)

/* MessageId  : 0x000036bb */
/* Approx. msg: ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET - An attempt to set the process default activation context failed because the process default activation context was already set. */
#define ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET	((ULONG)0x000036bbL)

/* MessageId  : 0x000036bc */
/* Approx. msg: ERROR_SXS_UNKNOWN_ENCODING_GROUP - The encoding group identifier specified is not recognized. */
#define ERROR_SXS_UNKNOWN_ENCODING_GROUP	((ULONG)0x000036bcL)

/* MessageId  : 0x000036bd */
/* Approx. msg: ERROR_SXS_UNKNOWN_ENCODING - The encoding requested is not recognized. */
#define ERROR_SXS_UNKNOWN_ENCODING	((ULONG)0x000036bdL)

/* MessageId  : 0x000036be */
/* Approx. msg: ERROR_SXS_INVALID_XML_NAMESPACE_URI - The manifest contains a reference to an invalid URI. */
#define ERROR_SXS_INVALID_XML_NAMESPACE_URI	((ULONG)0x000036beL)

/* MessageId  : 0x000036bf */
/* Approx. msg: ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED - The application manifest contains a reference to a dependent assembly which is not installed. */
#define ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED	((ULONG)0x000036bfL)

/* MessageId  : 0x000036c0 */
/* Approx. msg: ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED - The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed. */
#define ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED	((ULONG)0x000036c0L)

/* MessageId  : 0x000036c1 */
/* Approx. msg: ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE - The manifest contains an attribute for the assembly identity which is not valid. */
#define ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE	((ULONG)0x000036c1L)

/* MessageId  : 0x000036c2 */
/* Approx. msg: ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE - The manifest is missing the required default namespace specification on the assembly element. */
#define ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE	((ULONG)0x000036c2L)

/* MessageId  : 0x000036c3 */
/* Approx. msg: ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE - The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\". */
#define ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE	((ULONG)0x000036c3L)

/* MessageId  : 0x000036c4 */
/* Approx. msg: ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT - The private manifest probe has crossed the reparse-point-associated path. */
#define ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT	((ULONG)0x000036c4L)

/* MessageId  : 0x000036c5 */
/* Approx. msg: ERROR_SXS_DUPLICATE_DLL_NAME - Two or more components referenced directly or indirectly by the application manifest have files by the same name. */
#define ERROR_SXS_DUPLICATE_DLL_NAME	((ULONG)0x000036c5L)

/* MessageId  : 0x000036c6 */
/* Approx. msg: ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME - Two or more components referenced directly or indirectly by the application manifest have window classes with the same name. */
#define ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME	((ULONG)0x000036c6L)

/* MessageId  : 0x000036c7 */
/* Approx. msg: ERROR_SXS_DUPLICATE_CLSID - Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs. */
#define ERROR_SXS_DUPLICATE_CLSID	((ULONG)0x000036c7L)

/* MessageId  : 0x000036c8 */
/* Approx. msg: ERROR_SXS_DUPLICATE_IID - Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs. */
#define ERROR_SXS_DUPLICATE_IID	((ULONG)0x000036c8L)

/* MessageId  : 0x000036c9 */
/* Approx. msg: ERROR_SXS_DUPLICATE_TLBID - Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs. */
#define ERROR_SXS_DUPLICATE_TLBID	((ULONG)0x000036c9L)

/* MessageId  : 0x000036ca */
/* Approx. msg: ERROR_SXS_DUPLICATE_PROGID - Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs. */
#define ERROR_SXS_DUPLICATE_PROGID	((ULONG)0x000036caL)

/* MessageId  : 0x000036cb */
/* Approx. msg: ERROR_SXS_DUPLICATE_ASSEMBLY_NAME - Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted. */
#define ERROR_SXS_DUPLICATE_ASSEMBLY_NAME	((ULONG)0x000036cbL)

/* MessageId  : 0x000036cc */
/* Approx. msg: ERROR_SXS_FILE_HASH_MISMATCH - A component's file does not match the verification information present in the component manifest. */
#define ERROR_SXS_FILE_HASH_MISMATCH	((ULONG)0x000036ccL)

/* MessageId  : 0x000036cd */
/* Approx. msg: ERROR_SXS_POLICY_PARSE_ERROR - The policy manifest contains one or more syntax errors. */
#define ERROR_SXS_POLICY_PARSE_ERROR	((ULONG)0x000036cdL)

/* MessageId  : 0x000036ce */
/* Approx. msg: ERROR_SXS_XML_E_MISSINGQUOTE - Manifest Parse Error : A string literal was expected, but no opening quote character was found. */
#define ERROR_SXS_XML_E_MISSINGQUOTE	((ULONG)0x000036ceL)

/* MessageId  : 0x000036cf */
/* Approx. msg: ERROR_SXS_XML_E_COMMENTSYNTAX - Manifest Parse Error : Incorrect syntax was used in a comment. */
#define ERROR_SXS_XML_E_COMMENTSYNTAX	((ULONG)0x000036cfL)

/* MessageId  : 0x000036d0 */
/* Approx. msg: ERROR_SXS_XML_E_BADSTARTNAMECHAR - Manifest Parse Error : A name was started with an invalid character. */
#define ERROR_SXS_XML_E_BADSTARTNAMECHAR	((ULONG)0x000036d0L)

/* MessageId  : 0x000036d1 */
/* Approx. msg: ERROR_SXS_XML_E_BADNAMECHAR - Manifest Parse Error : A name contained an invalid character. */
#define ERROR_SXS_XML_E_BADNAMECHAR	((ULONG)0x000036d1L)

/* MessageId  : 0x000036d2 */
/* Approx. msg: ERROR_SXS_XML_E_BADCHARINSTRING - Manifest Parse Error : A string literal contained an invalid character. */
#define ERROR_SXS_XML_E_BADCHARINSTRING	((ULONG)0x000036d2L)

/* MessageId  : 0x000036d3 */
/* Approx. msg: ERROR_SXS_XML_E_XMLDECLSYNTAX - Manifest Parse Error : Invalid syntax for an XML declaration. */
#define ERROR_SXS_XML_E_XMLDECLSYNTAX	((ULONG)0x000036d3L)

/* MessageId  : 0x000036d4 */
/* Approx. msg: ERROR_SXS_XML_E_BADCHARDATA - Manifest Parse Error : An invalid character was found in text content. */
#define ERROR_SXS_XML_E_BADCHARDATA	((ULONG)0x000036d4L)

/* MessageId  : 0x000036d5 */
/* Approx. msg: ERROR_SXS_XML_E_MISSINGWHITESPACE - Manifest Parse Error : Required white space was missing. */
#define ERROR_SXS_XML_E_MISSINGWHITESPACE	((ULONG)0x000036d5L)

/* MessageId  : 0x000036d6 */
/* Approx. msg: ERROR_SXS_XML_E_EXPECTINGTAGEND - Manifest Parse Error : The character '>' was expected. */
#define ERROR_SXS_XML_E_EXPECTINGTAGEND	((ULONG)0x000036d6L)

/* MessageId  : 0x000036d7 */
/* Approx. msg: ERROR_SXS_XML_E_MISSINGSEMICOLON - Manifest Parse Error : A semi colon character was expected. */
#define ERROR_SXS_XML_E_MISSINGSEMICOLON	((ULONG)0x000036d7L)

/* MessageId  : 0x000036d8 */
/* Approx. msg: ERROR_SXS_XML_E_UNBALANCEDPAREN - Manifest Parse Error : Unbalanced parentheses. */
#define ERROR_SXS_XML_E_UNBALANCEDPAREN	((ULONG)0x000036d8L)

/* MessageId  : 0x000036d9 */
/* Approx. msg: ERROR_SXS_XML_E_INTERNALERROR - Manifest Parse Error : Internal error. */
#define ERROR_SXS_XML_E_INTERNALERROR	((ULONG)0x000036d9L)

/* MessageId  : 0x000036da */
/* Approx. msg: ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE - Manifest Parse Error : White space is not allowed at this location. */
#define ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE	((ULONG)0x000036daL)

/* MessageId  : 0x000036db */
/* Approx. msg: ERROR_SXS_XML_E_INCOMPLETE_ENCODING - Manifest Parse Error : End of file reached in invalid state for current encoding. */
#define ERROR_SXS_XML_E_INCOMPLETE_ENCODING	((ULONG)0x000036dbL)

/* MessageId  : 0x000036dc */
/* Approx. msg: ERROR_SXS_XML_E_MISSING_PAREN - Manifest Parse Error : Missing parenthesis. */
#define ERROR_SXS_XML_E_MISSING_PAREN	((ULONG)0x000036dcL)

/* MessageId  : 0x000036dd */
/* Approx. msg: ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE - Manifest Parse Error : A single or double closing quote character (\' or \") is missing. */
#define ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE	((ULONG)0x000036ddL)

/* MessageId  : 0x000036de */
/* Approx. msg: ERROR_SXS_XML_E_MULTIPLE_COLONS - Manifest Parse Error : Multiple colons are not allowed in a name. */
#define ERROR_SXS_XML_E_MULTIPLE_COLONS	((ULONG)0x000036deL)

/* MessageId  : 0x000036df */
/* Approx. msg: ERROR_SXS_XML_E_INVALID_DECIMAL - Manifest Parse Error : Invalid character for decimal digit. */
#define ERROR_SXS_XML_E_INVALID_DECIMAL	((ULONG)0x000036dfL)

/* MessageId  : 0x000036e0 */
/* Approx. msg: ERROR_SXS_XML_E_INVALID_HEXIDECIMAL - Manifest Parse Error : Invalid character for hexadecimal digit. */
#define ERROR_SXS_XML_E_INVALID_HEXIDECIMAL	((ULONG)0x000036e0L)

/* MessageId  : 0x000036e1 */
/* Approx. msg: ERROR_SXS_XML_E_INVALID_UNICODE - Manifest Parse Error : Invalid Unicode character value for this platform. */
#define ERROR_SXS_XML_E_INVALID_UNICODE	((ULONG)0x000036e1L)

/* MessageId  : 0x000036e2 */
/* Approx. msg: ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK - Manifest Parse Error : Expecting white space or '?'. */
#define ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK	((ULONG)0x000036e2L)

/* MessageId  : 0x000036e3 */
/* Approx. msg: ERROR_SXS_XML_E_UNEXPECTEDENDTAG - Manifest Parse Error : End tag was not expected at this location. */
#define ERROR_SXS_XML_E_UNEXPECTEDENDTAG	((ULONG)0x000036e3L)

/* MessageId  : 0x000036e4 */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDTAG - Manifest Parse Error : The following tags were not closed: %1. */
#define ERROR_SXS_XML_E_UNCLOSEDTAG	((ULONG)0x000036e4L)

/* MessageId  : 0x000036e5 */
/* Approx. msg: ERROR_SXS_XML_E_DUPLICATEATTRIBUTE - Manifest Parse Error : Duplicate attribute. */
#define ERROR_SXS_XML_E_DUPLICATEATTRIBUTE	((ULONG)0x000036e5L)

/* MessageId  : 0x000036e6 */
/* Approx. msg: ERROR_SXS_XML_E_MULTIPLEROOTS - Manifest Parse Error : Only one top level element is allowed in an XML document. */
#define ERROR_SXS_XML_E_MULTIPLEROOTS	((ULONG)0x000036e6L)

/* MessageId  : 0x000036e7 */
/* Approx. msg: ERROR_SXS_XML_E_INVALIDATROOTLEVEL - Manifest Parse Error : Invalid at the top level of the document. */
#define ERROR_SXS_XML_E_INVALIDATROOTLEVEL	((ULONG)0x000036e7L)

/* MessageId  : 0x000036e8 */
/* Approx. msg: ERROR_SXS_XML_E_BADXMLDECL - Manifest Parse Error : Invalid XML declaration. */
#define ERROR_SXS_XML_E_BADXMLDECL	((ULONG)0x000036e8L)

/* MessageId  : 0x000036e9 */
/* Approx. msg: ERROR_SXS_XML_E_MISSINGROOT - Manifest Parse Error : XML document must have a top level element. */
#define ERROR_SXS_XML_E_MISSINGROOT	((ULONG)0x000036e9L)

/* MessageId  : 0x000036ea */
/* Approx. msg: ERROR_SXS_XML_E_UNEXPECTEDEOF - Manifest Parse Error : Unexpected end of file. */
#define ERROR_SXS_XML_E_UNEXPECTEDEOF	((ULONG)0x000036eaL)

/* MessageId  : 0x000036eb */
/* Approx. msg: ERROR_SXS_XML_E_BADPEREFINSUBSET - Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset. */
#define ERROR_SXS_XML_E_BADPEREFINSUBSET	((ULONG)0x000036ebL)

/* MessageId  : 0x000036ec */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDSTARTTAG - Manifest Parse Error : Element was not closed. */
#define ERROR_SXS_XML_E_UNCLOSEDSTARTTAG	((ULONG)0x000036ecL)

/* MessageId  : 0x000036ed */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDENDTAG - Manifest Parse Error : End element was missing the character '>'. */
#define ERROR_SXS_XML_E_UNCLOSEDENDTAG	((ULONG)0x000036edL)

/* MessageId  : 0x000036ee */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDSTRING - Manifest Parse Error : A string literal was not closed. */
#define ERROR_SXS_XML_E_UNCLOSEDSTRING	((ULONG)0x000036eeL)

/* MessageId  : 0x000036ef */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDCOMMENT - Manifest Parse Error : A comment was not closed. */
#define ERROR_SXS_XML_E_UNCLOSEDCOMMENT	((ULONG)0x000036efL)

/* MessageId  : 0x000036f0 */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDDECL - Manifest Parse Error : A declaration was not closed. */
#define ERROR_SXS_XML_E_UNCLOSEDDECL	((ULONG)0x000036f0L)

/* MessageId  : 0x000036f1 */
/* Approx. msg: ERROR_SXS_XML_E_UNCLOSEDCDATA - Manifest Parse Error : A CDATA section was not closed. */
#define ERROR_SXS_XML_E_UNCLOSEDCDATA	((ULONG)0x000036f1L)

/* MessageId  : 0x000036f2 */
/* Approx. msg: ERROR_SXS_XML_E_RESERVEDNAMESPACE - Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\". */
#define ERROR_SXS_XML_E_RESERVEDNAMESPACE	((ULONG)0x000036f2L)

/* MessageId  : 0x000036f3 */
/* Approx. msg: ERROR_SXS_XML_E_INVALIDENCODING - Manifest Parse Error : System does not support the specified encoding. */
#define ERROR_SXS_XML_E_INVALIDENCODING	((ULONG)0x000036f3L)

/* MessageId  : 0x000036f4 */
/* Approx. msg: ERROR_SXS_XML_E_INVALIDSWITCH - Manifest Parse Error : Switch from current encoding to specified encoding not supported. */
#define ERROR_SXS_XML_E_INVALIDSWITCH	((ULONG)0x000036f4L)

/* MessageId  : 0x000036f5 */
/* Approx. msg: ERROR_SXS_XML_E_BADXMLCASE - Manifest Parse Error : The name 'xml' is reserved and must be lower case. */
#define ERROR_SXS_XML_E_BADXMLCASE	((ULONG)0x000036f5L)

/* MessageId  : 0x000036f6 */
/* Approx. msg: ERROR_SXS_XML_E_INVALID_STANDALONE - Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'. */
#define ERROR_SXS_XML_E_INVALID_STANDALONE	((ULONG)0x000036f6L)

/* MessageId  : 0x000036f7 */
/* Approx. msg: ERROR_SXS_XML_E_UNEXPECTED_STANDALONE - Manifest Parse Error : The standalone attribute cannot be used in external entities. */
#define ERROR_SXS_XML_E_UNEXPECTED_STANDALONE	((ULONG)0x000036f7L)

/* MessageId  : 0x000036f8 */
/* Approx. msg: ERROR_SXS_XML_E_INVALID_VERSION - Manifest Parse Error : Invalid version number. */
#define ERROR_SXS_XML_E_INVALID_VERSION	((ULONG)0x000036f8L)

/* MessageId  : 0x000036f9 */
/* Approx. msg: ERROR_SXS_XML_E_MISSINGEQUALS - Manifest Parse Error : Missing equals sign between attribute and attribute value. */
#define ERROR_SXS_XML_E_MISSINGEQUALS	((ULONG)0x000036f9L)

/* MessageId  : 0x000036fa */
/* Approx. msg: ERROR_SXS_PROTECTION_RECOVERY_FAILED - Assembly Protection Error: Unable to recover the specified assembly. */
#define ERROR_SXS_PROTECTION_RECOVERY_FAILED	((ULONG)0x000036faL)

/* MessageId  : 0x000036fb */
/* Approx. msg: ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT - Assembly Protection Error: The public key for an assembly was too short to be allowed. */
#define ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT	((ULONG)0x000036fbL)

/* MessageId  : 0x000036fc */
/* Approx. msg: ERROR_SXS_PROTECTION_CATALOG_NOT_VALID - Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest. */
#define ERROR_SXS_PROTECTION_CATALOG_NOT_VALID	((ULONG)0x000036fcL)

/* MessageId  : 0x000036fd */
/* Approx. msg: ERROR_SXS_UNTRANSLATABLE_HRESULT - An HRESULT could not be translated to a corresponding Win32 error code. */
#define ERROR_SXS_UNTRANSLATABLE_HRESULT	((ULONG)0x000036fdL)

/* MessageId  : 0x000036fe */
/* Approx. msg: ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING - Assembly Protection Error: The catalog for an assembly is missing. */
#define ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING	((ULONG)0x000036feL)

/* MessageId  : 0x000036ff */
/* Approx. msg: ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE - The supplied assembly identity is missing one or more attributes which must be present in this context. */
#define ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE	((ULONG)0x000036ffL)

/* MessageId  : 0x00003700 */
/* Approx. msg: ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME - The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names. */
#define ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME	((ULONG)0x00003700L)

/* MessageId  : 0x00003701 */
/* Approx. msg: ERROR_SXS_ASSEMBLY_MISSING - The referenced assembly could not be found. */
#define ERROR_SXS_ASSEMBLY_MISSING	((ULONG)0x00003701L)

/* MessageId  : 0x00003702 */
/* Approx. msg: ERROR_SXS_CORRUPT_ACTIVATION_STACK - The activation context activation stack for the running thread of execution is corrupt. */
#define ERROR_SXS_CORRUPT_ACTIVATION_STACK	((ULONG)0x00003702L)

/* MessageId  : 0x00003703 */
/* Approx. msg: ERROR_SXS_CORRUPTION - The application isolation metadata for this process or thread has become corrupt. */
#define ERROR_SXS_CORRUPTION	((ULONG)0x00003703L)

/* MessageId  : 0x00003704 */
/* Approx. msg: ERROR_SXS_EARLY_DEACTIVATION - The activation context being deactivated is not the most recently activated one. */
#define ERROR_SXS_EARLY_DEACTIVATION	((ULONG)0x00003704L)

/* MessageId  : 0x00003705 */
/* Approx. msg: ERROR_SXS_INVALID_DEACTIVATION - The activation context being deactivated is not active for the current thread of execution. */
#define ERROR_SXS_INVALID_DEACTIVATION	((ULONG)0x00003705L)

/* MessageId  : 0x00003706 */
/* Approx. msg: ERROR_SXS_MULTIPLE_DEACTIVATION - The activation context being deactivated has already been deactivated. */
#define ERROR_SXS_MULTIPLE_DEACTIVATION	((ULONG)0x00003706L)

/* MessageId  : 0x00003707 */
/* Approx. msg: ERROR_SXS_PROCESS_TERMINATION_REQUESTED - A component used by the isolation facility has requested to terminate the process. */
#define ERROR_SXS_PROCESS_TERMINATION_REQUESTED	((ULONG)0x00003707L)

/* MessageId  : 0x00003708 */
/* Approx. msg: ERROR_SXS_RELEASE_ACTIVATION_CONTEXT - A kernel mode component is releasing a reference on an activation context. */
#define ERROR_SXS_RELEASE_ACTIVATION_CONTEXT	((ULONG)0x00003708L)

/* MessageId  : 0x00003709 */
/* Approx. msg: ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY - The activation context of system default assembly could not be generated. */
#define ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY	((ULONG)0x00003709L)

/* MessageId  : 0x0000370a */
/* Approx. msg: ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE - The value of an attribute in an identity is not within the legal range. */
#define ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE	((ULONG)0x0000370aL)

/* MessageId  : 0x0000370b */
/* Approx. msg: ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME - The name of an attribute in an identity is not within the legal range. */
#define ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME	((ULONG)0x0000370bL)

/* MessageId  : 0x0000370c */
/* Approx. msg: ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE - An identity contains two definitions for the same attribute. */
#define ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE	((ULONG)0x0000370cL)

/* MessageId  : 0x0000370d */
/* Approx. msg: ERROR_SXS_IDENTITY_PARSE_ERROR - The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value. */
#define ERROR_SXS_IDENTITY_PARSE_ERROR	((ULONG)0x0000370dL)

/* MessageId  : 0x00003a98 */
/* Approx. msg: ERROR_EVT_INVALID_CHANNEL_PATH - The specified channel path is invalid. See extended error info for more details. */
#define ERROR_EVT_INVALID_CHANNEL_PATH	((ULONG)0x00003a98L)

/* MessageId  : 0x00003a99 */
/* Approx. msg: ERROR_EVT_INVALID_QUERY - The specified query is invalid. See extended error info for more details. */
#define ERROR_EVT_INVALID_QUERY	((ULONG)0x00003a99L)

/* MessageId  : 0x00003a9a */
/* Approx. msg: ERROR_EVT_PUBLISHER_MANIFEST_NOT_FOUND - The publisher did indicate they have a manifest/resource but a manifest/resource could not be found. */
#define ERROR_EVT_PUBLISHER_MANIFEST_NOT_FOUND	((ULONG)0x00003a9aL)

/* MessageId  : 0x00003a9b */
/* Approx. msg: ERROR_EVT_PUBLISHER_MANIFEST_NOT_SPECIFIED - The publisher does not have a manifest and is performing an operation which requires they have a manifest. */
#define ERROR_EVT_PUBLISHER_MANIFEST_NOT_SPECIFIED	((ULONG)0x00003a9bL)

/* MessageId  : 0x00003a9c */
/* Approx. msg: ERROR_EVT_NO_REGISTERED_TEMPLATE - There is no registered template for specified event id. */
#define ERROR_EVT_NO_REGISTERED_TEMPLATE	((ULONG)0x00003a9cL)

/* MessageId  : 0x00003a9d */
/* Approx. msg: ERROR_EVT_EVENT_CHANNEL_MISMATCH - The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to. */
#define ERROR_EVT_EVENT_CHANNEL_MISMATCH	((ULONG)0x00003a9dL)

/* MessageId  : 0x00003a9e */
/* Approx. msg: ERROR_EVT_UNEXPECTED_VALUE_TYPE - The type of a specified substitution value does not match the type expected from the template definition. */
#define ERROR_EVT_UNEXPECTED_VALUE_TYPE	((ULONG)0x00003a9eL)

/* MessageId  : 0x00003a9f */
/* Approx. msg: ERROR_EVT_UNEXPECTED_NUM_VALUES - The number of specified substitution values does not match the number expected from the template definition. */
#define ERROR_EVT_UNEXPECTED_NUM_VALUES	((ULONG)0x00003a9fL)

/* MessageId  : 0x00003aa0 */
/* Approx. msg: ERROR_EVT_CHANNEL_NOT_FOUND - The specified channel could not be found. Check channel configuration. */
#define ERROR_EVT_CHANNEL_NOT_FOUND	((ULONG)0x00003aa0L)

/* MessageId  : 0x00003aa1 */
/* Approx. msg: ERROR_EVT_MALFORMED_XML_TEXT - The specified xml text was not well-formed. See Extended Error for more details. */
#define ERROR_EVT_MALFORMED_XML_TEXT	((ULONG)0x00003aa1L)

/* MessageId  : 0x00003aa2 */
/* Approx. msg: ERROR_EVT_CHANNEL_PATH_TOO_GENERAL - The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance. */
#define ERROR_EVT_CHANNEL_PATH_TOO_GENERAL	((ULONG)0x00003aa2L)

/*  Facility=WIN32 */
/* MessageId  : 0x8007000e */
/* Approx. msg: E_OUTOFMEMORY - Out of memory */
#define E_OUTOFMEMORY	((ULONG)0x8007000eL)

/* MessageId  : 0x80070057 */
/* Approx. msg: E_INVALIDARG - One or more arguments are invalid */
#define E_INVALIDARG	((ULONG)0x80070057L)

/* MessageId  : 0x80070006 */
/* Approx. msg: E_POINTER - Invalid handle */
#define E_HANDLE	((ULONG)0x80070006L)

/* MessageId  : 0x80070005 */
/* Approx. msg: E_ACCESSDENIED - WIN32 access denied error */
#define E_ACCESSDENIED	((ULONG)0x80070005L)

/*  Facility=ITF */
/* MessageId  : 0x80040000 */
/* Approx. msg: OLE_E_OLEVERB - Invalid OLEVERB structure */
#define OLE_E_OLEVERB	((ULONG)0x80040000L)

/* MessageId  : 0x80040001 */
/* Approx. msg: OLE_E_ADVF - Invalid advise flags */
#define OLE_E_ADVF	((ULONG)0x80040001L)

/* MessageId  : 0x80040002 */
/* Approx. msg: OLE_E_ENUM_NOMORE - Can't enumerate any more, because the associated data is missing */
#define OLE_E_ENUM_NOMORE	((ULONG)0x80040002L)

/* MessageId  : 0x80040003 */
/* Approx. msg: OLE_E_ADVISENOTSUPPORTED - This implementation doesn't take advises */
#define OLE_E_ADVISENOTSUPPORTED	((ULONG)0x80040003L)

/* MessageId  : 0x80040004 */
/* Approx. msg: OLE_E_NOCONNECTION - There is no connection for this connection ID */
#define OLE_E_NOCONNECTION	((ULONG)0x80040004L)

/* MessageId  : 0x80040005 */
/* Approx. msg: OLE_E_NOTRUNNING - Need to run the object to perform this operation */
#define OLE_E_NOTRUNNING	((ULONG)0x80040005L)

/* MessageId  : 0x80040006 */
/* Approx. msg: OLE_E_NOCACHE - There is no cache to operate on */
#define OLE_E_NOCACHE	((ULONG)0x80040006L)

/* MessageId  : 0x80040007 */
/* Approx. msg: OLE_E_BLANK - Uninitialized object */
#define OLE_E_BLANK	((ULONG)0x80040007L)

/* MessageId  : 0x80040008 */
/* Approx. msg: OLE_E_CLASSDIFF - Linked object's source class has changed */
#define OLE_E_CLASSDIFF	((ULONG)0x80040008L)

/* MessageId  : 0x80040009 */
/* Approx. msg: OLE_E_CANT_GETMONIKER - Not able to get the moniker of the object */
#define OLE_E_CANT_GETMONIKER	((ULONG)0x80040009L)

/* MessageId  : 0x8004000a */
/* Approx. msg: OLE_E_CANT_BINDTOSOURCE - Not able to bind to the source */
#define OLE_E_CANT_BINDTOSOURCE	((ULONG)0x8004000aL)

/* MessageId  : 0x8004000b */
/* Approx. msg: OLE_E_STATIC - Object is static; operation not allowed */
#define OLE_E_STATIC	((ULONG)0x8004000bL)

/* MessageId  : 0x8004000c */
/* Approx. msg: OLE_E_PROMPTSAVECANCELLED - User canceled out of save dialog */
#define OLE_E_PROMPTSAVECANCELLED	((ULONG)0x8004000cL)

/* MessageId  : 0x8004000d */
/* Approx. msg: OLE_E_INVALIDRECT - Invalid rectangle */
#define OLE_E_INVALIDRECT	((ULONG)0x8004000dL)

/* MessageId  : 0x8004000e */
/* Approx. msg: OLE_E_WRONGCOMPOBJ - compobj.dll is too old for the ole2.dll initialized */
#define OLE_E_WRONGCOMPOBJ	((ULONG)0x8004000eL)

/* MessageId  : 0x8004000f */
/* Approx. msg: OLE_E_INVALIDHWND - Invalid window handle */
#define OLE_E_INVALIDHWND	((ULONG)0x8004000fL)

/* MessageId  : 0x80040010 */
/* Approx. msg: OLE_E_NOT_INPLACEACTIVE - Object is not in any of the inplace active states */
#define OLE_E_NOT_INPLACEACTIVE	((ULONG)0x80040010L)

/* MessageId  : 0x80040011 */
/* Approx. msg: OLE_E_CANTCONVERT - Not able to convert object */
#define OLE_E_CANTCONVERT	((ULONG)0x80040011L)

/* MessageId  : 0x80040012 */
/* Approx. msg: OLE_E_NOSTORAGE - Not able to perform the operation because object is not given storage yet */
#define OLE_E_NOSTORAGE	((ULONG)0x80040012L)

/* MessageId  : 0x80040064 */
/* Approx. msg: DV_E_FORMATETC - Invalid FORMATETC structure */
#define DV_E_FORMATETC	((ULONG)0x80040064L)

/* MessageId  : 0x80040065 */
/* Approx. msg: DV_E_DVTARGETDEVICE - Invalid DVTARGETDEVICE structure */
#define DV_E_DVTARGETDEVICE	((ULONG)0x80040065L)

/* MessageId  : 0x80040066 */
/* Approx. msg: DV_E_STGMEDIUM - Invalid STDGMEDIUM structure */
#define DV_E_STGMEDIUM	((ULONG)0x80040066L)

/* MessageId  : 0x80040067 */
/* Approx. msg: DV_E_STATDATA - Invalid STATDATA structure */
#define DV_E_STATDATA	((ULONG)0x80040067L)

/* MessageId  : 0x80040068 */
/* Approx. msg: DV_E_LINDEX - Invalid lindex */
#define DV_E_LINDEX	((ULONG)0x80040068L)

/* MessageId  : 0x80040069 */
/* Approx. msg: DV_E_TYMED - Invalid tymed */
#define DV_E_TYMED	((ULONG)0x80040069L)

/* MessageId  : 0x8004006a */
/* Approx. msg: DV_E_CLIPFORMAT - Invalid clipboard format */
#define DV_E_CLIPFORMAT	((ULONG)0x8004006aL)

/* MessageId  : 0x8004006b */
/* Approx. msg: DV_E_DVASPECT - Invalid aspect(s) */
#define DV_E_DVASPECT	((ULONG)0x8004006bL)

/* MessageId  : 0x8004006c */
/* Approx. msg: DV_E_DVTARGETDEVICE_SIZE - tdSize parameter of the DVTARGETDEVICE structure is invalid */
#define DV_E_DVTARGETDEVICE_SIZE	((ULONG)0x8004006cL)

/* MessageId  : 0x8004006d */
/* Approx. msg: DV_E_NOIVIEWOBJECT - Object doesn't support IViewObject interface */
#define DV_E_NOIVIEWOBJECT	((ULONG)0x8004006dL)

/* MessageId  : 0x80040100 */
/* Approx. msg: DRAGDROP_E_NOTREGISTERED - Trying to revoke a drop target that has not been registered */
#define DRAGDROP_E_NOTREGISTERED	((ULONG)0x80040100L)

/* MessageId  : 0x80040101 */
/* Approx. msg: DRAGDROP_E_ALREADYREGISTERED - This window has already been registered as a drop target */
#define DRAGDROP_E_ALREADYREGISTERED	((ULONG)0x80040101L)

/* MessageId  : 0x80040102 */
/* Approx. msg: DRAGDROP_E_INVALIDHWND - Invalid window handle */
#define DRAGDROP_E_INVALIDHWND	((ULONG)0x80040102L)

/* MessageId  : 0x80040110 */
/* Approx. msg: CLASS_E_NOAGGREGATION - Class does not support aggregation (or class object is remote) */
#define CLASS_E_NOAGGREGATION	((ULONG)0x80040110L)

/* MessageId  : 0x80040111 */
/* Approx. msg: CLASS_E_CLASSNOTAVAILABLE - ClassFactory cannot supply requested class */
#define CLASS_E_CLASSNOTAVAILABLE	((ULONG)0x80040111L)

/* MessageId  : 0x80040112 */
/* Approx. msg: CLASS_E_NOTLICENSED - Class is not licensed for use */
#define CLASS_E_NOTLICENSED	((ULONG)0x80040112L)

/*  EOF */

#endif
