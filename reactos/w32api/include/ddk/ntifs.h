/*
 * ntifs.h
 *
 * Windows NT Filesystem Driver Developer Kit
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Bo Brantén <bosse@acc.umu.se>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTIFS_
#define _NTIFS_
#define _GNU_NTIFS_

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "ntapi.h"

#pragma pack(push,4)

#define VER_PRODUCTBUILD 10000

#ifndef NTSYSAPI
#define NTSYSAPI
#endif

#ifndef NTKERNELAPI
#define NTKERNELAPI STDCALL
#endif

typedef struct _SE_EXPORTS                  *PSE_EXPORTS;

extern PUCHAR                       *FsRtlLegalAnsiCharacterArray;
extern PSE_EXPORTS                  SeExports;
extern PACL                         SePublicDefaultDacl;
extern PACL                         SeSystemDefaultDacl;

#define ANSI_DOS_STAR                   ('<')
#define ANSI_DOS_QM                     ('>')
#define ANSI_DOS_DOT                    ('"')

#define DOS_STAR                        (L'<')
#define DOS_QM                          (L'>')
#define DOS_DOT                         (L'"')

/* also in winnt.h */
#define ACCESS_ALLOWED_ACE_TYPE         (0x0)
#define ACCESS_DENIED_ACE_TYPE          (0x1)
#define SYSTEM_AUDIT_ACE_TYPE           (0x2)
#define SYSTEM_ALARM_ACE_TYPE           (0x3)
 
#define COMPRESSION_FORMAT_NONE         (0x0000)
#define COMPRESSION_FORMAT_DEFAULT      (0x0001)
#define COMPRESSION_FORMAT_LZNT1        (0x0002)
#define COMPRESSION_ENGINE_STANDARD     (0x0000)
#define COMPRESSION_ENGINE_MAXIMUM      (0x0100)
#define COMPRESSION_ENGINE_HIBER        (0x0200)

#define FILE_ACTION_ADDED                   0x00000001
#define FILE_ACTION_REMOVED                 0x00000002
#define FILE_ACTION_MODIFIED                0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005
#define FILE_ACTION_ADDED_STREAM            0x00000006
#define FILE_ACTION_REMOVED_STREAM          0x00000007
#define FILE_ACTION_MODIFIED_STREAM         0x00000008
#define FILE_ACTION_REMOVED_BY_DELETE       0x00000009
#define FILE_ACTION_ID_NOT_TUNNELLED        0x0000000A
#define FILE_ACTION_TUNNELLED_ID_COLLISION  0x0000000B
/* end  winnt.h */

#define FILE_EA_TYPE_BINARY             0xfffe
#define FILE_EA_TYPE_ASCII              0xfffd
#define FILE_EA_TYPE_BITMAP             0xfffb
#define FILE_EA_TYPE_METAFILE           0xfffa
#define FILE_EA_TYPE_ICON               0xfff9
#define FILE_EA_TYPE_EA                 0xffee
#define FILE_EA_TYPE_MVMT               0xffdf
#define FILE_EA_TYPE_MVST               0xffde
#define FILE_EA_TYPE_ASN1               0xffdd
#define FILE_EA_TYPE_FAMILY_IDS         0xff01

#define FILE_NEED_EA                    0x00000080

/* also in winnt.h */
#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001
#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002
#define FILE_NOTIFY_CHANGE_NAME         0x00000003
#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004
#define FILE_NOTIFY_CHANGE_SIZE         0x00000008
#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010
#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020
#define FILE_NOTIFY_CHANGE_CREATION     0x00000040
#define FILE_NOTIFY_CHANGE_EA           0x00000080
#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100
#define FILE_NOTIFY_CHANGE_STREAM_NAME  0x00000200
#define FILE_NOTIFY_CHANGE_STREAM_SIZE  0x00000400
#define FILE_NOTIFY_CHANGE_STREAM_WRITE 0x00000800
#define FILE_NOTIFY_VALID_MASK          0x00000fff
/* end winnt.h */

#define FILE_OPLOCK_BROKEN_TO_LEVEL_2   0x00000007
#define FILE_OPLOCK_BROKEN_TO_NONE      0x00000008

#define FILE_OPBATCH_BREAK_UNDERWAY     0x00000009

#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
#define FILE_CASE_PRESERVED_NAMES       0x00000002
#define FILE_UNICODE_ON_DISK            0x00000004
#define FILE_PERSISTENT_ACLS            0x00000008
#define FILE_FILE_COMPRESSION           0x00000010
#define FILE_VOLUME_QUOTAS              0x00000020
#define FILE_SUPPORTS_SPARSE_FILES      0x00000040
#define FILE_SUPPORTS_REPARSE_POINTS    0x00000080
#define FILE_SUPPORTS_REMOTE_STORAGE    0x00000100
#define FS_LFN_APIS                     0x00004000
#define FILE_VOLUME_IS_COMPRESSED       0x00008000
#define FILE_SUPPORTS_OBJECT_IDS        0x00010000
#define FILE_SUPPORTS_ENCRYPTION        0x00020000
#define FILE_NAMED_STREAMS              0x00040000

#define FILE_PIPE_BYTE_STREAM_TYPE      0x00000000
#define FILE_PIPE_MESSAGE_TYPE          0x00000001

#define FILE_PIPE_BYTE_STREAM_MODE      0x00000000
#define FILE_PIPE_MESSAGE_MODE          0x00000001

#define FILE_PIPE_QUEUE_OPERATION       0x00000000
#define FILE_PIPE_COMPLETE_OPERATION    0x00000001

#define FILE_PIPE_INBOUND               0x00000000
#define FILE_PIPE_OUTBOUND              0x00000001
#define FILE_PIPE_FULL_DUPLEX           0x00000002

#define FILE_PIPE_DISCONNECTED_STATE    0x00000001
#define FILE_PIPE_LISTENING_STATE       0x00000002
#define FILE_PIPE_CONNECTED_STATE       0x00000003
#define FILE_PIPE_CLOSING_STATE         0x00000004

#define FILE_PIPE_CLIENT_END            0x00000000
#define FILE_PIPE_SERVER_END            0x00000001

#define FILE_PIPE_READ_DATA             0x00000000
#define FILE_PIPE_WRITE_SPACE           0x00000001

#define FILE_STORAGE_TYPE_SPECIFIED             0x00000041  /* FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE */
#define FILE_STORAGE_TYPE_DEFAULT               (StorageTypeDefault << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_DIRECTORY             (StorageTypeDirectory << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_FILE                  (StorageTypeFile << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_DOCFILE               (StorageTypeDocfile << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_JUNCTION_POINT        (StorageTypeJunctionPoint << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_CATALOG               (StorageTypeCatalog << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_STRUCTURED_STORAGE    (StorageTypeStructuredStorage << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_EMBEDDING             (StorageTypeEmbedding << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_STREAM                (StorageTypeStream << FILE_STORAGE_TYPE_SHIFT)
#define FILE_MINIMUM_STORAGE_TYPE               FILE_STORAGE_TYPE_DEFAULT
#define FILE_MAXIMUM_STORAGE_TYPE               FILE_STORAGE_TYPE_STREAM
#define FILE_STORAGE_TYPE_MASK                  0x000f0000
#define FILE_STORAGE_TYPE_SHIFT                 16

#define FILE_VC_QUOTA_NONE              0x00000000
#define FILE_VC_QUOTA_TRACK             0x00000001
#define FILE_VC_QUOTA_ENFORCE           0x00000002
#define FILE_VC_QUOTA_MASK              0x00000003

#define FILE_VC_QUOTAS_LOG_VIOLATIONS   0x00000004
#define FILE_VC_CONTENT_INDEX_DISABLED  0x00000008

#define FILE_VC_LOG_QUOTA_THRESHOLD     0x00000010
#define FILE_VC_LOG_QUOTA_LIMIT         0x00000020
#define FILE_VC_LOG_VOLUME_THRESHOLD    0x00000040
#define FILE_VC_LOG_VOLUME_LIMIT        0x00000080

#define FILE_VC_QUOTAS_INCOMPLETE       0x00000100
#define FILE_VC_QUOTAS_REBUILDING       0x00000200

#define FILE_VC_VALID_MASK              0x000003ff

#define FSRTL_FLAG_FILE_MODIFIED        (0x01)
#define FSRTL_FLAG_FILE_LENGTH_CHANGED  (0x02)
#define FSRTL_FLAG_LIMIT_MODIFIED_PAGES (0x04)
#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX (0x08)
#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH (0x10)
#define FSRTL_FLAG_USER_MAPPED_FILE     (0x20)
#define FSRTL_FLAG_EOF_ADVANCE_ACTIVE   (0x80)

#define FSRTL_FLAG2_DO_MODIFIED_WRITE   (0x01)

#define FSRTL_FSP_TOP_LEVEL_IRP         (0x01)
#define FSRTL_CACHE_TOP_LEVEL_IRP       (0x02)
#define FSRTL_MOD_WRITE_TOP_LEVEL_IRP   (0x03)
#define FSRTL_FAST_IO_TOP_LEVEL_IRP     (0x04)
#define FSRTL_MAX_TOP_LEVEL_IRP_FLAG    (0x04)

#define FSRTL_VOLUME_DISMOUNT           1
#define FSRTL_VOLUME_DISMOUNT_FAILED    2
#define FSRTL_VOLUME_LOCK               3
#define FSRTL_VOLUME_LOCK_FAILED        4
#define FSRTL_VOLUME_UNLOCK             5
#define FSRTL_VOLUME_MOUNT              6

#define FSRTL_WILD_CHARACTER            0x08

#ifdef _X86_
#define HARDWARE_PTE    HARDWARE_PTE_X86
#define PHARDWARE_PTE   PHARDWARE_PTE_X86
#else
#define HARDWARE_PTE    ULONG
#define PHARDWARE_PTE   PULONG
#endif

#define IO_CHECK_CREATE_PARAMETERS      0x0200
#define IO_ATTACH_DEVICE                0x0400

#define IO_ATTACH_DEVICE_API            0x80000000
/* also in winnt.h */
#define IO_COMPLETION_QUERY_STATE       0x0001
#define IO_COMPLETION_MODIFY_STATE      0x0002
#define IO_COMPLETION_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)
/* end winnt.h */
#define IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE    64
#define IO_FILE_OBJECT_PAGED_POOL_CHARGE        1024

#define IO_TYPE_APC                     18
#define IO_TYPE_DPC                     19
#define IO_TYPE_DEVICE_QUEUE            20
#define IO_TYPE_EVENT_PAIR              21
#define IO_TYPE_INTERRUPT               22
#define IO_TYPE_PROFILE                 23

#define IRP_BEING_VERIFIED              0x10

#define MAILSLOT_CLASS_FIRSTCLASS       1
#define MAILSLOT_CLASS_SECONDCLASS      2

#define MAILSLOT_SIZE_AUTO              0

#define MAP_PROCESS                     1L
#define MAP_SYSTEM                      2L
#define MEM_DOS_LIM                     0x40000000
/* also in winnt.h */
#define MEM_IMAGE                       SEC_IMAGE
/* end winnt.h */ 
#define OB_TYPE_TYPE                    1
#define OB_TYPE_DIRECTORY               2
#define OB_TYPE_SYMBOLIC_LINK           3
#define OB_TYPE_TOKEN                   4
#define OB_TYPE_PROCESS                 5
#define OB_TYPE_THREAD                  6
#define OB_TYPE_EVENT                   7
#define OB_TYPE_EVENT_PAIR              8
#define OB_TYPE_MUTANT                  9
#define OB_TYPE_SEMAPHORE               10
#define OB_TYPE_TIMER                   11
#define OB_TYPE_PROFILE                 12
#define OB_TYPE_WINDOW_STATION          13
#define OB_TYPE_DESKTOP                 14
#define OB_TYPE_SECTION                 15
#define OB_TYPE_KEY                     16
#define OB_TYPE_PORT                    17
#define OB_TYPE_ADAPTER                 18
#define OB_TYPE_CONTROLLER              19
#define OB_TYPE_DEVICE                  20
#define OB_TYPE_DRIVER                  21
#define OB_TYPE_IO_COMPLETION           22
#define OB_TYPE_FILE                    23

#define PIN_WAIT                        (1)
#define PIN_EXCLUSIVE                   (2)
#define PIN_NO_READ                     (4)
#define PIN_IF_BCB                      (8)

#define PORT_CONNECT                    0x0001
#define PORT_ALL_ACCESS                 (STANDARD_RIGHTS_ALL |\
                                         PORT_CONNECT)
/* also in winnt.h */
#define SEC_BASED	0x00200000
#define SEC_NO_CHANGE	0x00400000
#define SEC_FILE	0x00800000
#define SEC_IMAGE	0x01000000
#define SEC_VLM		0x02000000
#define SEC_RESERVE	0x04000000
#define SEC_COMMIT	0x08000000
#define SEC_NOCACHE	0x10000000

#define SECURITY_WORLD_SID_AUTHORITY    {0,0,0,0,0,1}
#define SECURITY_WORLD_RID              (0x00000000L)

#define SID_REVISION                    1

#define TOKEN_ASSIGN_PRIMARY            (0x0001)
#define TOKEN_DUPLICATE                 (0x0002)
#define TOKEN_IMPERSONATE               (0x0004)
#define TOKEN_QUERY                     (0x0008)
#define TOKEN_QUERY_SOURCE              (0x0010)
#define TOKEN_ADJUST_PRIVILEGES         (0x0020)
#define TOKEN_ADJUST_GROUPS             (0x0040)
#define TOKEN_ADJUST_DEFAULT            (0x0080)

#define TOKEN_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |\
                          TOKEN_ASSIGN_PRIMARY     |\
                          TOKEN_DUPLICATE          |\
                          TOKEN_IMPERSONATE        |\
                          TOKEN_QUERY              |\
                          TOKEN_QUERY_SOURCE       |\
                          TOKEN_ADJUST_PRIVILEGES  |\
                          TOKEN_ADJUST_GROUPS      |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_READ       (STANDARD_RIGHTS_READ     |\
                          TOKEN_QUERY)

#define TOKEN_WRITE      (STANDARD_RIGHTS_WRITE    |\
                          TOKEN_ADJUST_PRIVILEGES  |\
                          TOKEN_ADJUST_GROUPS      |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_EXECUTE    (STANDARD_RIGHTS_EXECUTE)

#define TOKEN_SOURCE_LENGTH 8
/* end winnt.h */

#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x01
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x02
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x04
#define TOKEN_HAS_ADMIN_GROUP           0x08
#define TOKEN_IS_RESTRICTED             0x10

#define VACB_MAPPING_GRANULARITY        (0x40000)
#define VACB_OFFSET_SHIFT               (18)

#define FSCTL_REQUEST_OPLOCK_LEVEL_1    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_OPLOCK_LEVEL_2    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_BATCH_OPLOCK      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_ACKNOWLEDGE  CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPBATCH_ACK_CLOSE_PENDING CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_NOTIFY       CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_IS_VOLUME_MOUNTED         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_PATHNAME_VALID         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MARK_VOLUME_DIRTY         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_QUERY_RETRIEVAL_POINTERS  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 14,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_GET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 16, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)


#define FSCTL_MARK_AS_SYSTEM_HIVE       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 19,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_ACK_NO_2     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_INVALIDATE_VOLUMES        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_FAT_BPB             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_FILTER_OPLOCK     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FILESYSTEM_GET_STATISTICS CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 24, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if (VER_PRODUCTBUILD >= 1381)

#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_VOLUME_BITMAP         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 27,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_MOVE_FILE                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 29, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_VOLUME_DIRTY           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_HFS_INFORMATION       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 31, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ALLOW_EXTENDED_DASD_IO    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 32, METHOD_NEITHER,  FILE_ANY_ACCESS)

#endif /* (VER_PRODUCTBUILD >= 1381) */

#if (VER_PRODUCTBUILD >= 2195)

#define FSCTL_READ_PROPERTY_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 33, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_WRITE_PROPERTY_DATA       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 34, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_FIND_FILES_BY_SID         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 35, METHOD_NEITHER, FILE_ANY_ACCESS)

#define FSCTL_DUMP_PROPERTY_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 37,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_SET_OBJECT_ID             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 38, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_GET_OBJECT_ID             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 39, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_OBJECT_ID          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 40, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_GET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_REPARSE_POINT      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_ENUM_USN_DATA             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 44,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_SECURITY_ID_CHECK         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 45,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_READ_USN_JOURNAL          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 46,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_SET_OBJECT_ID_EXTENDED    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 47, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_CREATE_OR_GET_OBJECT_ID   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 48, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_SPARSE                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SET_ZERO_DATA             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_QUERY_ALLOCATED_RANGES    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 51,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_ENABLE_UPGRADE            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 52, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SET_ENCRYPTION            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 53, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ENCRYPTION_FSCTL_IO       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 54,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_WRITE_RAW_ENCRYPTED       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 55,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_READ_RAW_ENCRYPTED        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 56,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_CREATE_USN_JOURNAL        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 57,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_READ_FILE_USN_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 58,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_WRITE_USN_CLOSE_RECORD    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 59,  METHOD_NEITHER, FILE_READ_DATA)
#define FSCTL_EXTEND_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 60, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_USN_JOURNAL         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 61, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_USN_JOURNAL        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 62, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MARK_HANDLE               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 63, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SIS_COPYFILE              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 64, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SIS_LINK_FILES            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 65, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_HSM_MSG                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 66, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_NSS_CONTROL               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 67, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_HSM_DATA                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 68, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_RECALL_FILE               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 69, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NSS_RCONTROL              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 70, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_READ_FROM_PLEX            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 71, METHOD_OUT_DIRECT, FILE_READ_DATA)
#define FSCTL_FILE_PREFETCH             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 72, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#endif /* (VER_PRODUCTBUILD >= 2195) */

#define FSCTL_MAILSLOT_PEEK             CTL_CODE(FILE_DEVICE_MAILSLOT, 0, METHOD_NEITHER, FILE_READ_DATA)

#define FSCTL_NETWORK_SET_CONFIGURATION_INFO    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 102, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_CONFIGURATION_INFO    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 103, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_CONNECTION_INFO       CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 104, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_ENUMERATE_CONNECTIONS     CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 105, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_DELETE_CONNECTION         CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 107, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_STATISTICS            CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 116, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_SET_DOMAIN_NAME           CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 120, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_REMOTE_BOOT_INIT_SCRT     CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 250, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_ASSIGN_EVENT         CTL_CODE(FILE_DEVICE_NAMED_PIPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_DISCONNECT           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_LISTEN               CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_PEEK                 CTL_CODE(FILE_DEVICE_NAMED_PIPE, 3, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_QUERY_EVENT          CTL_CODE(FILE_DEVICE_NAMED_PIPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_TRANSCEIVE           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 5, METHOD_NEITHER,  FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_WAIT                 CTL_CODE(FILE_DEVICE_NAMED_PIPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_IMPERSONATE          CTL_CODE(FILE_DEVICE_NAMED_PIPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_CLIENT_PROCESS   CTL_CODE(FILE_DEVICE_NAMED_PIPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_QUERY_CLIENT_PROCESS CTL_CODE(FILE_DEVICE_NAMED_PIPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_INTERNAL_READ        CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2045, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_INTERNAL_WRITE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2046, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_TRANSCEIVE  CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2047, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_READ_OVFLOW CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2048, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_REDIR_QUERY_PATH          CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 99, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef PVOID PEJOB;
typedef PVOID OPLOCK, *POPLOCK;
typedef PVOID PWOW64_PROCESS;

typedef struct _CACHE_MANAGER_CALLBACKS         *PCACHE_MANAGER_CALLBACKS;
typedef struct _EPROCESS_QUOTA_BLOCK            *PEPROCESS_QUOTA_BLOCK;
typedef struct _FILE_GET_QUOTA_INFORMATION      *PFILE_GET_QUOTA_INFORMATION;
typedef struct _HANDLE_TABLE                    *PHANDLE_TABLE;
typedef struct _KEVENT_PAIR                     *PKEVENT_PAIR;
typedef struct _KPROCESS                        *PKPROCESS;
typedef struct _KQUEUE                          *PKQUEUE;
typedef struct _KTRAP_FRAME                     *PKTRAP_FRAME;
typedef struct _MAILSLOT_CREATE_PARAMETERS      *PMAILSLOT_CREATE_PARAMETERS;
typedef struct _MMWSL                           *PMMWSL;
typedef struct _NAMED_PIPE_CREATE_PARAMETERS    *PNAMED_PIPE_CREATE_PARAMETERS;
typedef struct _OBJECT_DIRECTORY                *POBJECT_DIRECTORY;
typedef struct _PAGEFAULT_HISTORY               *PPAGEFAULT_HISTORY;
typedef struct _PS_IMPERSONATION_INFORMATION    *PPS_IMPERSONATION_INFORMATION;
typedef struct _SECTION_OBJECT                  *PSECTION_OBJECT;
typedef struct _SHARED_CACHE_MAP                *PSHARED_CACHE_MAP;
typedef struct _TERMINATION_PORT                *PTERMINATION_PORT;
typedef struct _VACB                            *PVACB;
typedef struct _VAD_HEADER                      *PVAD_HEADER;

typedef struct _NOTIFY_SYNC
{
    ULONG Unknown0;
    ULONG Unknown1;
    ULONG Unknown2;
    USHORT Unknown3;
    USHORT Unknown4;
    ULONG Unknown5;
    ULONG Unknown6;
    ULONG Unknown7;
    ULONG Unknown8;
    ULONG Unknown9;
    ULONG Unknown10;
} NOTIFY_SYNC, * PNOTIFY_SYNC;

typedef enum _FAST_IO_POSSIBLE {
    FastIoIsNotPossible,
    FastIoIsPossible,
    FastIoIsQuestionable
} FAST_IO_POSSIBLE;

typedef enum _FILE_STORAGE_TYPE {
    StorageTypeDefault = 1,
    StorageTypeDirectory,
    StorageTypeFile,
    StorageTypeJunctionPoint,
    StorageTypeCatalog,
    StorageTypeStructuredStorage,
    StorageTypeEmbedding,
    StorageTypeStream
} FILE_STORAGE_TYPE;

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
    IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

typedef enum _OBJECT_INFO_CLASS {
    ObjectBasicInfo,
    ObjectNameInfo,
    ObjectTypeInfo,
    ObjectAllTypesInfo,
    ObjectProtectionInfo
} OBJECT_INFO_CLASS;

typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _KAPC_STATE {
    LIST_ENTRY  ApcListHead[2];
    PKPROCESS   Process;
    BOOLEAN     KernelApcInProgress;
    BOOLEAN     KernelApcPending;
    BOOLEAN     UserApcPending;
} KAPC_STATE, *PKAPC_STATE;

typedef struct _KGDTENTRY {
    USHORT LimitLow;
    USHORT BaseLow;
    union {
        struct {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct {
            ULONG BaseMid       : 8;
            ULONG Type          : 5;
            ULONG Dpl           : 2;
            ULONG Pres          : 1;
            ULONG LimitHi       : 4;
            ULONG Sys           : 1;
            ULONG Reserved_0    : 1;
            ULONG Default_Big   : 1;
            ULONG Granularity   : 1;
            ULONG BaseHi        : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

typedef struct _KIDTENTRY {
    USHORT Offset;
    USHORT Selector;
    USHORT Access;
    USHORT ExtendedOffset;
} KIDTENTRY, *PKIDTENTRY;

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _MMSUPPORT_FLAGS {
    ULONG SessionSpace              : 1;
    ULONG BeingTrimmed              : 1;
    ULONG SessionLeader             : 1;
    ULONG TrimHard                  : 1;
    ULONG WorkingSetHard            : 1;
    ULONG AddressSpaceBeingDeleted  : 1;
    ULONG Available                 : 10;
    ULONG AllowWorkingSetAdjustment : 8;
    ULONG MemoryPriority            : 8;
} MMSUPPORT_FLAGS, *PMMSUPPORT_FLAGS;

#else

typedef struct _MMSUPPORT_FLAGS {
    ULONG SessionSpace      : 1;
    ULONG BeingTrimmed      : 1;
    ULONG ProcessInSession  : 1;
    ULONG SessionLeader     : 1;
    ULONG TrimHard          : 1;
    ULONG WorkingSetHard    : 1;
    ULONG WriteWatch        : 1;
    ULONG Filler            : 25;
} MMSUPPORT_FLAGS, *PMMSUPPORT_FLAGS;

#endif

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _MMSUPPORT {
    LARGE_INTEGER   LastTrimTime;
    MMSUPPORT_FLAGS Flags;
    ULONG           PageFaultCount;
    ULONG           PeakWorkingSetSize;
    ULONG           WorkingSetSize;
    ULONG           MinimumWorkingSetSize;
    ULONG           MaximumWorkingSetSize;
    PMMWSL          VmWorkingSetList;
    LIST_ENTRY      WorkingSetExpansionLinks;
    ULONG           Claim;
    ULONG           NextEstimationSlot;
    ULONG           NextAgingSlot;
    ULONG           EstimatedAvailable;
    ULONG           GrowthSinceLastEstimate;
} MMSUPPORT, *PMMSUPPORT;

#else

typedef struct _MMSUPPORT {
    LARGE_INTEGER   LastTrimTime;
    ULONG           LastTrimFaultCount;
    ULONG           PageFaultCount;
    ULONG           PeakWorkingSetSize;
    ULONG           WorkingSetSize;
    ULONG           MinimumWorkingSetSize;
    ULONG           MaximumWorkingSetSize;
    PMMWSL          VmWorkingSetList;
    LIST_ENTRY      WorkingSetExpansionLinks;
    BOOLEAN         AllowWorkingSetAdjustment;
    BOOLEAN         AddressSpaceBeingDeleted;
    UCHAR           ForegroundSwitchCount;
    UCHAR           MemoryPriority;
#if (VER_PRODUCTBUILD >= 2195)
    union {
        ULONG           LongFlags;
        MMSUPPORT_FLAGS Flags;
    } u;
    ULONG           Claim;
    ULONG           NextEstimationSlot;
    ULONG           NextAgingSlot;
    ULONG           EstimatedAvailable;
    ULONG           GrowthSinceLastEstimate;
#endif /* (VER_PRODUCTBUILD >= 2195) */
} MMSUPPORT, *PMMSUPPORT;

#endif

typedef struct _SE_AUDIT_PROCESS_CREATION_INFO {
    POBJECT_NAME_INFORMATION ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;

typedef struct _BITMAP_RANGE {
    LIST_ENTRY      Links;
    LARGE_INTEGER   BasePage;
    ULONG           FirstDirtyPage;
    ULONG           LastDirtyPage;
    ULONG           DirtyPages;
    PULONG          Bitmap;
} BITMAP_RANGE, *PBITMAP_RANGE;

typedef struct _CACHE_UNINITIALIZE_EVENT {
    struct _CACHE_UNINITIALIZE_EVENT    *Next;
    KEVENT                              Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

typedef struct _CC_FILE_SIZES {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;

typedef struct _COMPRESSED_DATA_INFO {
    USHORT  CompressionFormatAndEngine;
    UCHAR   CompressionUnitShift;
    UCHAR   ChunkShift;
    UCHAR   ClusterShift;
    UCHAR   Reserved;
    USHORT  NumberOfChunks;
    ULONG   CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

typedef struct _DEVICE_MAP {
    POBJECT_DIRECTORY   DosDevicesDirectory;
    POBJECT_DIRECTORY   GlobalDosDevicesDirectory;
    ULONG               ReferenceCount;
    ULONG               DriveMap;
    UCHAR               DriveType[32];
} DEVICE_MAP, *PDEVICE_MAP; 

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _EX_FAST_REF {
    _ANONYMOUS_UNION union {
        PVOID Object;
        ULONG RefCnt : 3;
        ULONG Value;
    } DUMMYUNIONNAME;
} EX_FAST_REF, *PEX_FAST_REF;

typedef struct _EX_PUSH_LOCK {
    _ANONYMOUS_UNION union {
        _ANONYMOUS_STRUCT struct {
            ULONG   Waiting     : 1;
            ULONG   Exclusive   : 1;
            ULONG   Shared      : 30;
        } DUMMYSTRUCTNAME;
        ULONG   Value;
        PVOID   Ptr;
    } DUMMYUNIONNAME;
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef struct _EX_RUNDOWN_REF {
    _ANONYMOUS_UNION union {
        ULONG Count;
        PVOID Ptr;
    } DUMMYUNIONNAME;
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

#endif

typedef struct _EPROCESS_QUOTA_ENTRY {
    ULONG Usage;
    ULONG Limit;
    ULONG Peak;
    ULONG Return;
} EPROCESS_QUOTA_ENTRY, *PEPROCESS_QUOTA_ENTRY;

typedef struct _EPROCESS_QUOTA_BLOCK {
    EPROCESS_QUOTA_ENTRY    QuotaEntry[3];
    LIST_ENTRY              QuotaList;
    ULONG                   ReferenceCount;
    ULONG                   ProcessCount;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

/*
 * When needing these parameters cast your PIO_STACK_LOCATION to
 * PEXTENDED_IO_STACK_LOCATION
 */
#if !defined(_ALPHA_)
#include <pshpack4.h>
#endif
typedef struct _EXTENDED_IO_STACK_LOCATION {

    /* Included for padding */
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    union {

       struct {
          PIO_SECURITY_CONTEXT              SecurityContext;
          ULONG                             Options;
          USHORT                            Reserved;
          USHORT                            ShareAccess;
          PMAILSLOT_CREATE_PARAMETERS       Parameters;
       } CreateMailslot;

        struct {
            PIO_SECURITY_CONTEXT            SecurityContext;
            ULONG                           Options;
            USHORT                          Reserved;
            USHORT                          ShareAccess;
            PNAMED_PIPE_CREATE_PARAMETERS   Parameters;
        } CreatePipe;

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

        struct {
            ULONG                           Length;
            PSID                            StartSid;
            PFILE_GET_QUOTA_INFORMATION     SidList;
            ULONG                           SidListLength;
        } QueryQuota;

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
    PDEVICE_OBJECT  DeviceObject;
    PFILE_OBJECT  FileObject;
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID  Context;

} EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;
#if !defined(_ALPHA_)
#include <poppack.h>
#endif

typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    CCHAR           ShortNameLength;
    WCHAR           ShortName[12];
    WCHAR           FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION {
    HANDLE  Port;
    ULONG   Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
    LARGE_INTEGER   CompressedFileSize;
    USHORT          CompressionFormat;
    UCHAR           CompressionUnitShift;
    UCHAR           ChunkShift;
    UCHAR           ClusterShift;
    UCHAR           Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_COPY_ON_WRITE_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_COPY_ON_WRITE_INFORMATION, *PFILE_COPY_ON_WRITE_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    WCHAR           FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
		ULONG	          NextEntryOffset;
		ULONG	          FileIndex;
		LARGE_INTEGER   CreationTime;
		LARGE_INTEGER   LastAccessTime;
		LARGE_INTEGER   LastWriteTime;
		LARGE_INTEGER   ChangeTime;
		LARGE_INTEGER   EndOfFile;
		LARGE_INTEGER   AllocationSize;
		ULONG           FileAttributes;
		ULONG           FileNameLength;
		ULONG           EaSize;
		WCHAR           FileName[0];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION;

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
		ULONG         NextEntryOffset;
		ULONG	        FileIndex;
		LARGE_INTEGER CreationTime;
		LARGE_INTEGER LastAccessTime;
		LARGE_INTEGER LastWriteTime;
		LARGE_INTEGER ChangeTime;
		LARGE_INTEGER EndOfFile;
		LARGE_INTEGER AllocationSize;
		ULONG         FileAttributes;
		ULONG         FileNameLength;
		ULONG         EaSize;
		CHAR          ShortNameLength;
		WCHAR         ShortName[12];
		WCHAR         FileName[0];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG   FileSystemAttributes;
    ULONG   MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _FILE_FS_CONTROL_INFORMATION {
    LARGE_INTEGER   FreeSpaceStartFiltering;
    LARGE_INTEGER   FreeSpaceThreshold;
    LARGE_INTEGER   FreeSpaceStopFiltering;
    LARGE_INTEGER   DefaultQuotaThreshold;
    LARGE_INTEGER   DefaultQuotaLimit;
    ULONG           FileSystemControlFlags;
} FILE_FS_CONTROL_INFORMATION, *PFILE_FS_CONTROL_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   CallerAvailableAllocationUnits;
    LARGE_INTEGER   ActualAvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

#if (VER_PRODUCTBUILD >= 2195)

typedef struct _FILE_FS_OBJECT_ID_INFORMATION {
    UCHAR ObjectId[16];
    UCHAR ExtendedInfo[48];
} FILE_FS_OBJECT_ID_INFORMATION, *PFILE_FS_OBJECT_ID_INFORMATION;

#endif /* (VER_PRODUCTBUILD >= 2195) */

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   AvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER   VolumeCreationTime;
    ULONG           VolumeSerialNumber;
    ULONG           VolumeLabelLength;
    BOOLEAN         SupportsObjects;
    WCHAR           VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    WCHAR           FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION {
    ULONG   NextEntryOffset;
    UCHAR   EaNameLength;
    CHAR    EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

typedef struct _FILE_GET_QUOTA_INFORMATION {
    ULONG   NextEntryOffset;
    ULONG   SidLength;
    SID     Sid;
} FILE_GET_QUOTA_INFORMATION, *PFILE_GET_QUOTA_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

typedef struct _FILE_LOCK_INFO {
    LARGE_INTEGER   StartingByte;
    LARGE_INTEGER   Length;
    BOOLEAN         ExclusiveLock;
    ULONG           Key;
    PFILE_OBJECT    FileObject;
    PEPROCESS       Process;
    LARGE_INTEGER   EndingByte;
} FILE_LOCK_INFO, *PFILE_LOCK_INFO;

/* raw internal file lock struct returned from FsRtlGetNextFileLock */
typedef struct _FILE_SHARED_LOCK_ENTRY {
    PVOID           Unknown1;
    PVOID           Unknown2;
    FILE_LOCK_INFO  FileLock;
} FILE_SHARED_LOCK_ENTRY, *PFILE_SHARED_LOCK_ENTRY;

/* raw internal file lock struct returned from FsRtlGetNextFileLock */
typedef struct _FILE_EXCLUSIVE_LOCK_ENTRY {
    LIST_ENTRY      ListEntry;
    PVOID           Unknown1;
    PVOID           Unknown2;
    FILE_LOCK_INFO  FileLock;
} FILE_EXCLUSIVE_LOCK_ENTRY, *PFILE_EXCLUSIVE_LOCK_ENTRY;

typedef NTSTATUS (*PCOMPLETE_LOCK_IRP_ROUTINE) (
    IN PVOID    Context,
    IN PIRP     Irp
);

typedef VOID (NTAPI *PUNLOCK_ROUTINE) (
    IN PVOID            Context,
    IN PFILE_LOCK_INFO  FileLockInfo
);

typedef struct _FILE_LOCK {
    PCOMPLETE_LOCK_IRP_ROUTINE  CompleteLockIrpRoutine;
    PUNLOCK_ROUTINE             UnlockRoutine;
    BOOLEAN                     FastIoIsQuestionable;
    BOOLEAN                     Pad[3];
    PVOID                       LockInformation;
    FILE_LOCK_INFO              LastReturnedLockInfo;
    PVOID                       LastReturnedLock;
} FILE_LOCK, *PFILE_LOCK;

typedef struct _FILE_MAILSLOT_PEEK_BUFFER {
    ULONG ReadDataAvailable;
    ULONG NumberOfMessages;
    ULONG MessageLength;
} FILE_MAILSLOT_PEEK_BUFFER, *PFILE_MAILSLOT_PEEK_BUFFER;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
    ULONG           MaximumMessageSize;
    ULONG           MailslotQuota;
    ULONG           NextMessageSize;
    ULONG           MessagesAvailable;
    LARGE_INTEGER   ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION      BasicInformation;
    FILE_STANDARD_INFORMATION   StandardInformation;
    FILE_INTERNAL_INFORMATION   InternalInformation;
    FILE_EA_INFORMATION         EaInformation;
    FILE_ACCESS_INFORMATION     AccessInformation;
    FILE_POSITION_INFORMATION   PositionInformation;
    FILE_MODE_INFORMATION       ModeInformation;
    FILE_ALIGNMENT_INFORMATION  AlignmentInformation;
    FILE_NAME_INFORMATION       NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_OBJECTID_INFORMATION {
    LONGLONG        FileReference;
    UCHAR           ObjectId[16];
    _ANONYMOUS_UNION union {
        struct {
            UCHAR   BirthVolumeId[16];
            UCHAR   BirthObjectId[16];
            UCHAR   DomainId[16];
        } ;
        UCHAR       ExtendedInfo[48];
    } DUMMYUNIONNAME;
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

typedef struct _FILE_OLE_CLASSID_INFORMATION {
    GUID ClassId;
} FILE_OLE_CLASSID_INFORMATION, *PFILE_OLE_CLASSID_INFORMATION;

typedef struct _FILE_OLE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION          BasicInformation;
    FILE_STANDARD_INFORMATION       StandardInformation;
    FILE_INTERNAL_INFORMATION       InternalInformation;
    FILE_EA_INFORMATION             EaInformation;
    FILE_ACCESS_INFORMATION         AccessInformation;
    FILE_POSITION_INFORMATION       PositionInformation;
    FILE_MODE_INFORMATION           ModeInformation;
    FILE_ALIGNMENT_INFORMATION      AlignmentInformation;
    USN                             LastChangeUsn;
    USN                             ReplicationUsn;
    LARGE_INTEGER                   SecurityChangeTime;
    FILE_OLE_CLASSID_INFORMATION    OleClassIdInformation;
    FILE_OBJECTID_INFORMATION       ObjectIdInformation;
    FILE_STORAGE_TYPE               StorageType;
    ULONG                           OleStateBits;
    ULONG                           OleId;
    ULONG                           NumberOfStreamReferences;
    ULONG                           StreamIndex;
    ULONG                           SecurityId;
    BOOLEAN                         ContentIndexDisable;
    BOOLEAN                         InheritContentIndexDisable;
    FILE_NAME_INFORMATION           NameInformation;
} FILE_OLE_ALL_INFORMATION, *PFILE_OLE_ALL_INFORMATION;

typedef struct _FILE_OLE_DIR_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    FILE_STORAGE_TYPE   StorageType;
    GUID                OleClassId;
    ULONG               OleStateBits;
    BOOLEAN             ContentIndexDisable;
    BOOLEAN             InheritContentIndexDisable;
    WCHAR               FileName[1];
} FILE_OLE_DIR_INFORMATION, *PFILE_OLE_DIR_INFORMATION;

typedef struct _FILE_OLE_INFORMATION {
    LARGE_INTEGER                   SecurityChangeTime;
    FILE_OLE_CLASSID_INFORMATION    OleClassIdInformation;
    FILE_OBJECTID_INFORMATION       ObjectIdInformation;
    FILE_STORAGE_TYPE               StorageType;
    ULONG                           OleStateBits;
    BOOLEAN                         ContentIndexDisable;
    BOOLEAN                         InheritContentIndexDisable;
} FILE_OLE_INFORMATION, *PFILE_OLE_INFORMATION;

typedef struct _FILE_OLE_STATE_BITS_INFORMATION {
    ULONG StateBits;
    ULONG StateBitsMask;
} FILE_OLE_STATE_BITS_INFORMATION, *PFILE_OLE_STATE_BITS_INFORMATION;

typedef struct _FILE_PIPE_ASSIGN_EVENT_BUFFER {
    HANDLE  EventHandle;
    ULONG   KeyValue;
} FILE_PIPE_ASSIGN_EVENT_BUFFER, *PFILE_PIPE_ASSIGN_EVENT_BUFFER;

typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER {
    PVOID ClientSession;
    PVOID ClientProcess;
} FILE_PIPE_CLIENT_PROCESS_BUFFER, *PFILE_PIPE_CLIENT_PROCESS_BUFFER;

typedef struct _FILE_PIPE_EVENT_BUFFER {
    ULONG NamedPipeState;
    ULONG EntryType;
    ULONG ByteCount;
    ULONG KeyValue;
    ULONG NumberRequests;
} FILE_PIPE_EVENT_BUFFER, *PFILE_PIPE_EVENT_BUFFER;

typedef struct _FILE_PIPE_INFORMATION {
    ULONG ReadMode;
    ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
    ULONG NamedPipeType;
    ULONG NamedPipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG ReadDataAvailable;
    ULONG OutboundQuota;
    ULONG WriteQuotaAvailable;
    ULONG NamedPipeState;
    ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION {
    LARGE_INTEGER   CollectDataTime;
    ULONG           MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_PIPE_WAIT_FOR_BUFFER {
    LARGE_INTEGER   Timeout;
    ULONG           NameLength;
    BOOLEAN         TimeoutSpecified;
    WCHAR           Name[1];
} FILE_PIPE_WAIT_FOR_BUFFER, *PFILE_PIPE_WAIT_FOR_BUFFER;

typedef struct _FILE_QUOTA_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           SidLength;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   QuotaUsed;
    LARGE_INTEGER   QuotaThreshold;
    LARGE_INTEGER   QuotaLimit;
    SID             Sid;
} FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           StreamNameLength;
    LARGE_INTEGER   StreamSize;
    LARGE_INTEGER   StreamAllocationSize;
    WCHAR           StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_TRACKING_INFORMATION {
    HANDLE  DestinationFile;
    ULONG   ObjectInformationLength;
    CHAR    ObjectInformation[1];
} FILE_TRACKING_INFORMATION, *PFILE_TRACKING_INFORMATION;

typedef struct _FSRTL_COMMON_FCB_HEADER {
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    UCHAR           Flags;
    UCHAR           IsFastIoPossible;
#if (VER_PRODUCTBUILD >= 1381)
    UCHAR           Flags2;
    UCHAR           Reserved;
#endif /* (VER_PRODUCTBUILD >= 1381) */
    PERESOURCE      Resource;
    PERESOURCE      PagingIoResource;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

typedef struct _GENERATE_NAME_CONTEXT {
    USHORT  Checksum;
    BOOLEAN CheckSumInserted;
    UCHAR   NameLength;
    WCHAR   NameBuffer[8];
    ULONG   ExtensionLength;
    WCHAR   ExtensionBuffer[4];
    ULONG   LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

typedef struct _HANDLE_TABLE_ENTRY {
    PVOID   Object;
    ULONG   ObjectAttributes;
    ULONG   GrantedAccess;
    USHORT  GrantedAccessIndex;
    USHORT  CreatorBackTraceIndex;
    ULONG   NextFreeTableEntry;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _MAPPING_PAIR {
    ULONGLONG Vcn;
    ULONGLONG Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

typedef struct _GET_RETRIEVAL_DESCRIPTOR {
    ULONG           NumberOfPairs;
    ULONGLONG       StartVcn;
    MAPPING_PAIR    Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct _IO_CLIENT_EXTENSION {
    struct _IO_CLIENT_EXTENSION *NextExtension;
    PVOID                       ClientIdentificationAddress;
} IO_CLIENT_EXTENSION, *PIO_CLIENT_EXTENSION;

typedef struct _IO_COMPLETION_BASIC_INFORMATION {
    LONG Depth;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

typedef struct _KEVENT_PAIR {
    USHORT Type;
    USHORT Size;
    KEVENT Event1;
    KEVENT Event2;
} KEVENT_PAIR, *PKEVENT_PAIR;

typedef struct _KQUEUE {
    DISPATCHER_HEADER   Header;
    LIST_ENTRY          EntryListHead;
    ULONG               CurrentCount;
    ULONG               MaximumCount;
    LIST_ENTRY          ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

typedef struct _MAILSLOT_CREATE_PARAMETERS {
    ULONG           MailslotQuota;
    ULONG           MaximumMessageSize;
    LARGE_INTEGER   ReadTimeout;
    BOOLEAN         TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _MBCB {
    CSHORT          NodeTypeCode;
    CSHORT          NodeIsInZone;
    ULONG           PagesToWrite;
    ULONG           DirtyPages;
    ULONG           Reserved;
    LIST_ENTRY      BitmapRanges;
    LONGLONG        ResumeWritePage;
    BITMAP_RANGE    BitmapRange1;
    BITMAP_RANGE    BitmapRange2;
    BITMAP_RANGE    BitmapRange3;
} MBCB, *PMBCB;

typedef struct _MOVEFILE_DESCRIPTOR {
     HANDLE         FileHandle; 
     ULONG          Reserved;   
     LARGE_INTEGER  StartVcn; 
     LARGE_INTEGER  TargetLcn;
     ULONG          NumVcns; 
     ULONG          Reserved1;  
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS {
    ULONG           NamedPipeType;
    ULONG           ReadMode;
    ULONG           CompletionMode;
    ULONG           MaximumInstances;
    ULONG           InboundQuota;
    ULONG           OutboundQuota;
    LARGE_INTEGER   DefaultTimeout;
    BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

typedef struct _OBJECT_BASIC_INFO {
    ULONG           Attributes;
    ACCESS_MASK     GrantedAccess;
    ULONG           HandleCount;
    ULONG           ReferenceCount;
    ULONG           PagedPoolUsage;
    ULONG           NonPagedPoolUsage;
    ULONG           Reserved[3];
    ULONG           NameInformationLength;
    ULONG           TypeInformationLength;
    ULONG           SecurityDescriptorLength;
    LARGE_INTEGER   CreateTime;
} OBJECT_BASIC_INFO, *POBJECT_BASIC_INFO;

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFO {
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFO, *POBJECT_HANDLE_ATTRIBUTE_INFO;

typedef struct _OBJECT_NAME_INFO {
    UNICODE_STRING  ObjectName;
    WCHAR           ObjectNameBuffer[1];
} OBJECT_NAME_INFO, *POBJECT_NAME_INFO;

typedef struct _OBJECT_PROTECTION_INFO {
    BOOLEAN Inherit;
    BOOLEAN ProtectHandle;
} OBJECT_PROTECTION_INFO, *POBJECT_PROTECTION_INFO;

typedef struct _OBJECT_TYPE_INFO {
    UNICODE_STRING  ObjectTypeName;
    UCHAR           Unknown[0x58];
    WCHAR           ObjectTypeNameBuffer[1];
} OBJECT_TYPE_INFO, *POBJECT_TYPE_INFO;

typedef struct _OBJECT_ALL_TYPES_INFO {
    ULONG               NumberOfObjectTypes;
    OBJECT_TYPE_INFO    ObjectsTypeInfo[1];
} OBJECT_ALL_TYPES_INFO, *POBJECT_ALL_TYPES_INFO;

typedef struct _PAGEFAULT_HISTORY {
    ULONG                           CurrentIndex;
    ULONG                           MaxIndex;
    KSPIN_LOCK                      SpinLock;
    PVOID                           Reserved;
    PROCESS_WS_WATCH_INFORMATION    WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;

typedef struct _PATHNAME_BUFFER {
    ULONG PathNameLength;
    WCHAR Name[1];
} PATHNAME_BUFFER, *PPATHNAME_BUFFER;

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _PRIVATE_CACHE_MAP_FLAGS {
    ULONG DontUse           : 16;
    ULONG ReadAheadActive   : 1;
    ULONG ReadAheadEnabled  : 1;
    ULONG Available         : 14;
} PRIVATE_CACHE_MAP_FLAGS, *PPRIVATE_CACHE_MAP_FLAGS;

typedef struct _PRIVATE_CACHE_MAP {
    _ANONYMOUS_UNION union {
        CSHORT                  NodeTypeCode;
        PRIVATE_CACHE_MAP_FLAGS Flags;
        ULONG                   UlongFlags;
    } DUMMYUNIONNAME;
    ULONG                       ReadAheadMask;
    PFILE_OBJECT                FileObject;
    LARGE_INTEGER               FileOffset1;
    LARGE_INTEGER               BeyondLastByte1;
    LARGE_INTEGER               FileOffset2;
    LARGE_INTEGER               BeyondLastByte2;
    LARGE_INTEGER               ReadAheadOffset[2];
    ULONG                       ReadAheadLength[2];
    KSPIN_LOCK                  ReadAheadSpinLock;
    LIST_ENTRY                  PrivateLinks;
} PRIVATE_CACHE_MAP, *PPRIVATE_CACHE_MAP;

#endif

typedef struct _PS_IMPERSONATION_INFORMATION {
    PACCESS_TOKEN                   Token;
    BOOLEAN                         CopyOnOpen;
    BOOLEAN                         EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;

typedef struct _PUBLIC_BCB {
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    ULONG           MappedLength;
    LARGE_INTEGER   MappedFileOffset;
} PUBLIC_BCB, *PPUBLIC_BCB;

typedef struct _QUERY_PATH_REQUEST {
    ULONG                   PathNameLength;
    PIO_SECURITY_CONTEXT    SecurityContext;
    WCHAR                   FilePathName[1];
} QUERY_PATH_REQUEST, *PQUERY_PATH_REQUEST;

typedef struct _QUERY_PATH_RESPONSE {
    ULONG LengthAccepted;
} QUERY_PATH_RESPONSE, *PQUERY_PATH_RESPONSE;

typedef struct _RETRIEVAL_POINTERS_BUFFER {
    ULONG               ExtentCount;
    LARGE_INTEGER       StartingVcn;
    struct {
        LARGE_INTEGER   NextVcn;
        LARGE_INTEGER   Lcn;
    } Extents[1];
} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

typedef struct _SE_EXPORTS {

    LUID    SeCreateTokenPrivilege;
    LUID    SeAssignPrimaryTokenPrivilege;
    LUID    SeLockMemoryPrivilege;
    LUID    SeIncreaseQuotaPrivilege;
    LUID    SeUnsolicitedInputPrivilege;
    LUID    SeTcbPrivilege;
    LUID    SeSecurityPrivilege;
    LUID    SeTakeOwnershipPrivilege;
    LUID    SeLoadDriverPrivilege;
    LUID    SeCreatePagefilePrivilege;
    LUID    SeIncreaseBasePriorityPrivilege;
    LUID    SeSystemProfilePrivilege;
    LUID    SeSystemtimePrivilege;
    LUID    SeProfileSingleProcessPrivilege;
    LUID    SeCreatePermanentPrivilege;
    LUID    SeBackupPrivilege;
    LUID    SeRestorePrivilege;
    LUID    SeShutdownPrivilege;
    LUID    SeDebugPrivilege;
    LUID    SeAuditPrivilege;
    LUID    SeSystemEnvironmentPrivilege;
    LUID    SeChangeNotifyPrivilege;
    LUID    SeRemoteShutdownPrivilege;

    PSID    SeNullSid;
    PSID    SeWorldSid;
    PSID    SeLocalSid;
    PSID    SeCreatorOwnerSid;
    PSID    SeCreatorGroupSid;

    PSID    SeNtAuthoritySid;
    PSID    SeDialupSid;
    PSID    SeNetworkSid;
    PSID    SeBatchSid;
    PSID    SeInteractiveSid;
    PSID    SeLocalSystemSid;
    PSID    SeAliasAdminsSid;
    PSID    SeAliasUsersSid;
    PSID    SeAliasGuestsSid;
    PSID    SeAliasPowerUsersSid;
    PSID    SeAliasAccountOpsSid;
    PSID    SeAliasSystemOpsSid;
    PSID    SeAliasPrintOpsSid;
    PSID    SeAliasBackupOpsSid;

    PSID    SeAuthenticatedUsersSid;

    PSID    SeRestrictedSid;
    PSID    SeAnonymousLogonSid;

    LUID    SeUndockPrivilege;
    LUID    SeSyncAgentPrivilege;
    LUID    SeEnableDelegationPrivilege;

} SE_EXPORTS, *PSE_EXPORTS;

typedef struct _SECTION_BASIC_INFORMATION {
    PVOID           BaseAddress;
    ULONG           Attributes;
    LARGE_INTEGER   Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID   EntryPoint;
    ULONG   Unknown1;
    ULONG   StackReserve;
    ULONG   StackCommit;
    ULONG   Subsystem;
    USHORT  MinorSubsystemVersion;
    USHORT  MajorSubsystemVersion;
    ULONG   Unknown2;
    ULONG   Characteristics;
    USHORT  ImageNumber;
    BOOLEAN Executable;
    UCHAR   Unknown3;
    ULONG   Unknown4[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _SHARED_CACHE_MAP {
    CSHORT                      NodeTypeCode;
    CSHORT                      NodeByteSize;
    ULONG                       OpenCount;
    LARGE_INTEGER               FileSize;
    LIST_ENTRY                  BcbList;
    LARGE_INTEGER               SectionSize;
    LARGE_INTEGER               ValidDataLength;
    LARGE_INTEGER               ValidDataGoal;
    PVACB                       InitialVacbs[4];
    PVACB                       *Vacbs;
    PFILE_OBJECT                FileObject;
    PVACB                       ActiveVacb;
    PVOID                       NeedToZero;
    ULONG                       ActivePage;
    ULONG                       NeedToZeroPage;
    KSPIN_LOCK                  ActiveVacbSpinLock;
    ULONG                       VacbActiveCount;
    ULONG                       DirtyPages;
    LIST_ENTRY                  SharedCacheMapLinks;
    ULONG                       Flags;
    NTSTATUS                    Status;
    PMBCB                       Mbcb;
    PVOID                       Section;
    PKEVENT                     CreateEvent;
    PKEVENT                     WaitOnActiveCount;
    ULONG                       PagesToWrite;
    LONGLONG                    BeyondLastFlush;
    PCACHE_MANAGER_CALLBACKS    Callbacks;
    PVOID                       LazyWriteContext;
    LIST_ENTRY                  PrivateList;
    PVOID                       LogHandle;
    PVOID                       FlushToLsnRoutine;
    ULONG                       DirtyPageThreshold;
    ULONG                       LazyWritePassCount;
    PCACHE_UNINITIALIZE_EVENT   UninitializeEvent;
    PVACB                       NeedToZeroVacb;
    KSPIN_LOCK                  BcbSpinLock;
    PVOID                       Reserved;
    KEVENT                      Event;
    EX_PUSH_LOCK                VacbPushLock;
    PRIVATE_CACHE_MAP           PrivateCacheMap;
} SHARED_CACHE_MAP, *PSHARED_CACHE_MAP;

#endif

typedef struct _STARTING_VCN_INPUT_BUFFER {
    LARGE_INTEGER StartingVcn;
} STARTING_VCN_INPUT_BUFFER, *PSTARTING_VCN_INPUT_BUFFER;

typedef struct _SYSTEM_CACHE_INFORMATION {
    ULONG CurrentSize;
    ULONG PeakSize;
    ULONG PageFaultCount;
    ULONG MinimumWorkingSet;
    ULONG MaximumWorkingSet;
    ULONG Unused[4];
} SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;

typedef struct _TERMINATION_PORT {
    struct _TERMINATION_PORT*   Next;
    PVOID                       Port;
} TERMINATION_PORT, *PTERMINATION_PORT;

typedef struct _SECURITY_CLIENT_CONTEXT {
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PACCESS_TOKEN               ClientToken;
    BOOLEAN                     DirectlyAccessClientToken;
    BOOLEAN                     DirectAccessEffectiveOnly;
    BOOLEAN                     ServerIsRemote;
    TOKEN_CONTROL               ClientTokenControl;
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;

typedef struct _TUNNEL {
    FAST_MUTEX          Mutex;
    PRTL_SPLAY_LINKS    Cache;
    LIST_ENTRY          TimerQueue;
    USHORT              NumEntries;
} TUNNEL, *PTUNNEL;

typedef struct _VACB {
    PVOID               BaseAddress;
    PSHARED_CACHE_MAP   SharedCacheMap;
    union {
        LARGE_INTEGER   FileOffset;
        USHORT          ActiveCount;
    } Overlay;
    LIST_ENTRY          LruList;
} VACB, *PVACB;

typedef struct _VAD_HEADER {
    PVOID       StartVPN;
    PVOID       EndVPN;
    PVAD_HEADER ParentLink;
    PVAD_HEADER LeftLink;
    PVAD_HEADER RightLink;
    ULONG       Flags;          /* LSB = CommitCharge */
    PVOID       ControlArea;
    PVOID       FirstProtoPte;
    PVOID       LastPTE;
    ULONG       Unknown;
    LIST_ENTRY  Secured;
} VAD_HEADER, *PVAD_HEADER;

NTKERNELAPI
BOOLEAN
NTAPI
CcCanIWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG        BytesToWrite,
    IN BOOLEAN      Wait,
    IN BOOLEAN      Retrying
);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyRead (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWrite (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Wait,
    IN PVOID            Buffer
);

#define CcCopyWriteWontFlush(FO, FOFF, LEN) ((LEN) <= 0x10000)

typedef VOID (NTAPI *PCC_POST_DEFERRED_WRITE) (
    IN PVOID Context1,
    IN PVOID Context2
);

NTKERNELAPI
VOID
NTAPI
CcDeferWrite (
    IN PFILE_OBJECT             FileObject,
    IN PCC_POST_DEFERRED_WRITE  PostRoutine,
    IN PVOID                    Context1,
    IN PVOID                    Context2,
    IN ULONG                    BytesToWrite,
    IN BOOLEAN                  Retrying
);

NTKERNELAPI
VOID
NTAPI
CcFastCopyRead (
    IN PFILE_OBJECT         FileObject,
    IN ULONG                FileOffset,
    IN ULONG                Length,
    IN ULONG                PageCount,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
VOID
NTAPI
CcFastCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG        FileOffset,
    IN ULONG        Length,
    IN PVOID        Buffer
);

NTKERNELAPI
VOID
NTAPI
CcFlushCache (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER           FileOffset OPTIONAL,
    IN ULONG                    Length,
    OUT PIO_STATUS_BLOCK        IoStatus OPTIONAL
);

typedef VOID (*PDIRTY_PAGE_ROUTINE) (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN PLARGE_INTEGER   OldestLsn,
    IN PLARGE_INTEGER   NewestLsn,
    IN PVOID            Context1,
    IN PVOID            Context2
);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetDirtyPages (
    IN PVOID                LogHandle,
    IN PDIRTY_PAGE_ROUTINE  DirtyPageRoutine,
    IN PVOID                Context1,
    IN PVOID                Context2
);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb (
    IN PVOID Bcb
);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
);

#define CcGetFileSizePointer(FO) (                                     \
    ((PLARGE_INTEGER)((FO)->SectionObjectPointer->SharedCacheMap) + 1) \
)

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetFlushedValidData (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN                  BcbListHeld
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
LARGE_INTEGER
CcGetLsnForFileObject (
    IN PFILE_OBJECT     FileObject,
    OUT PLARGE_INTEGER  OldestLsn OPTIONAL
);

typedef BOOLEAN (NTAPI *PACQUIRE_FOR_LAZY_WRITE) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (NTAPI *PRELEASE_FROM_LAZY_WRITE) (
    IN PVOID Context
);

typedef BOOLEAN (NTAPI *PACQUIRE_FOR_READ_AHEAD) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (NTAPI *PRELEASE_FROM_READ_AHEAD) (
    IN PVOID Context
);

typedef struct _CACHE_MANAGER_CALLBACKS {
    PACQUIRE_FOR_LAZY_WRITE     AcquireForLazyWrite;
    PRELEASE_FROM_LAZY_WRITE    ReleaseFromLazyWrite;
    PACQUIRE_FOR_READ_AHEAD     AcquireForReadAhead;
    PRELEASE_FROM_READ_AHEAD    ReleaseFromReadAhead;
} CACHE_MANAGER_CALLBACKS, *PCACHE_MANAGER_CALLBACKS;

NTKERNELAPI
VOID
NTAPI
CcInitializeCacheMap (
    IN PFILE_OBJECT             FileObject,
    IN PCC_FILE_SIZES           FileSizes,
    IN BOOLEAN                  PinAccess,
    IN PCACHE_MANAGER_CALLBACKS Callbacks,
    IN PVOID                    LazyWriteContext
);

#define CcIsFileCached(FO) (                                                         \
    ((FO)->SectionObjectPointer != NULL) &&                                          \
    (((PSECTION_OBJECT_POINTERS)(FO)->SectionObjectPointer)->SharedCacheMap != NULL) \
)

NTKERNELAPI
BOOLEAN
NTAPI
CcIsThereDirtyData (
    IN PVPB Vpb
);

NTKERNELAPI
BOOLEAN
NTAPI
CcMapData (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Wait,
    OUT PVOID           *Bcb,
    OUT PVOID           *Buffer
);

NTKERNELAPI
VOID
NTAPI
CcMdlRead (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    OUT PMDL                *MdlChain,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
VOID
NTAPI
CcMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL         MdlChain
);

NTKERNELAPI
VOID
NTAPI
CcMdlWriteComplete (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinMappedData (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
#if (VER_PRODUCTBUILD >= 2195)
    IN ULONG            Flags,
#else
    IN BOOLEAN          Wait,
#endif
    IN OUT PVOID        *Bcb
);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinRead (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
#if (VER_PRODUCTBUILD >= 2195)
    IN ULONG            Flags,
#else
    IN BOOLEAN          Wait,
#endif
    OUT PVOID           *Bcb,
    OUT PVOID           *Buffer
);

NTKERNELAPI
VOID
NTAPI
CcPrepareMdlWrite (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    OUT PMDL                *MdlChain,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
BOOLEAN
NTAPI
CcPreparePinWrite (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Zero,
#if (VER_PRODUCTBUILD >= 2195)
    IN ULONG            Flags,
#else
    IN BOOLEAN          Wait,
#endif
    OUT PVOID           *Bcb,
    OUT PVOID           *Buffer
);

NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER           FileOffset OPTIONAL,
    IN ULONG                    Length,
    IN BOOLEAN                  UninitializeCacheMaps
);

#define CcReadAhead(FO, FOFF, LEN) (                \
    if ((LEN) >= 256) {                             \
        CcScheduleReadAhead((FO), (FOFF), (LEN));   \
    }                                               \
)

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
PVOID
NTAPI
CcRemapBcb (
    IN PVOID Bcb
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
VOID
NTAPI
CcRepinBcb (
    IN PVOID Bcb
);

NTKERNELAPI
VOID
NTAPI
CcScheduleReadAhead (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length
);

NTKERNELAPI
VOID
NTAPI
CcSetAdditionalCacheAttributes (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN      DisableReadAhead,
    IN BOOLEAN      DisableWriteBehind
);

NTKERNELAPI
VOID
NTAPI
CcSetBcbOwnerPointer (
    IN PVOID Bcb,
    IN PVOID OwnerPointer
);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPageThreshold (
    IN PFILE_OBJECT FileObject,
    IN ULONG        DirtyPageThreshold
);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPinnedData (
    IN PVOID            BcbVoid,
    IN PLARGE_INTEGER   Lsn OPTIONAL
);

NTKERNELAPI
VOID
NTAPI
CcSetFileSizes (
    IN PFILE_OBJECT     FileObject,
    IN PCC_FILE_SIZES   FileSizes
);

typedef VOID (NTAPI *PFLUSH_TO_LSN) (
    IN PVOID            LogHandle,
    IN PLARGE_INTEGER   Lsn
);

NTKERNELAPI
VOID
NTAPI
CcSetLogHandleForFile (
    IN PFILE_OBJECT     FileObject,
    IN PVOID            LogHandle,
    IN PFLUSH_TO_LSN    FlushToLsnRoutine
);

NTKERNELAPI
VOID
NTAPI
CcSetReadAheadGranularity (
    IN PFILE_OBJECT FileObject,
    IN ULONG        Granularity     /* default: PAGE_SIZE */
                                    /* allowed: 2^n * PAGE_SIZE */
);

NTKERNELAPI
BOOLEAN
NTAPI
CcUninitializeCacheMap (
    IN PFILE_OBJECT                 FileObject,
    IN PLARGE_INTEGER               TruncateSize OPTIONAL,
    IN PCACHE_UNINITIALIZE_EVENT    UninitializeCompleteEvent OPTIONAL
);

NTKERNELAPI
VOID
NTAPI
CcUnpinData (
    IN PVOID Bcb
);

NTKERNELAPI
VOID
NTAPI
CcUnpinDataForThread (
    IN PVOID            Bcb,
    IN ERESOURCE_THREAD ResourceThreadId
);

NTKERNELAPI
VOID
NTAPI
CcUnpinRepinnedBcb (
    IN PVOID                Bcb,
    IN BOOLEAN              WriteThrough,
    OUT PIO_STATUS_BLOCK    IoStatus
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity (
    VOID
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
BOOLEAN
NTAPI
CcZeroData (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   StartOffset,
    IN PLARGE_INTEGER   EndOffset,
    IN BOOLEAN          Wait
);

NTKERNELAPI
VOID
NTAPI
ExDisableResourceBoostLite (
    IN PERESOURCE Resource
);

NTKERNELAPI
ULONG
NTAPI
ExQueryPoolBlockSize (
    IN PVOID        PoolBlock,
    OUT PBOOLEAN    QuotaCharged
);

#define FlagOn(x, f) ((x) & (f))

NTKERNELAPI
VOID
NTAPI
FsRtlAddToTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  ShortName,
    IN PUNICODE_STRING  LongName,
    IN BOOLEAN          KeyByShortName,
    IN ULONG            DataLength,
    IN PVOID            Data
);

#if (VER_PRODUCTBUILD >= 2195)

PFILE_LOCK
NTAPI
FsRtlAllocateFileLock (
    IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePool (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuota (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuotaTag (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN ULONG        Tag
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithTag (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN ULONG        Tag
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreNamesEqual (
    IN PUNICODE_STRING  Name1,
    IN PUNICODE_STRING  Name2,
    IN BOOLEAN          IgnoreCase,
    IN PWCHAR           UpcaseTable OPTIONAL
);

#define FsRtlAreThereCurrentFileLocks(FL) ( \
    ((FL)->FastIoIsQuestionable)            \
)

/*
  FsRtlCheckLockForReadAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForRead.
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp
);

/*
  FsRtlCheckLockForWriteAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForWrite.
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp
);

typedef
VOID NTAPI
(*POPLOCK_WAIT_COMPLETE_ROUTINE) (
    IN PVOID    Context,
    IN PIRP     Irp
);

typedef
VOID NTAPI
(*POPLOCK_FS_PREPOST_IRP) (
    IN PVOID    Context,
    IN PIRP     Irp
);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplock (
    IN POPLOCK                          Oplock,
    IN PIRP                             Irp,
    IN PVOID                            Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE    CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP           PostIrpRoutine OPTIONAL
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyRead (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyWrite (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    IN PVOID                Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentBatchOplock (
    IN POPLOCK Oplock
);

NTKERNELAPI
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache (
    IN PTUNNEL      Cache,
    IN ULONGLONG    DirectoryKey
);

NTKERNELAPI
VOID
NTAPI
FsRtlDeleteTunnelCache (
    IN PTUNNEL Cache
);

NTKERNELAPI
VOID
NTAPI
FsRtlDeregisterUncProvider (
    IN HANDLE Handle
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesNameContainWildCards (
    IN PUNICODE_STRING Name
);

#define FsRtlEnterFileSystem    KeEnterCriticalRegion

#define FsRtlExitFileSystem     KeLeaveCriticalRegion

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForRead (
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForWrite (
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process
);

#define FsRtlFastLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) (       \
     FsRtlPrivateLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, NULL, A10, A11)   \
)

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAll (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN PVOID                Context OPTIONAL
);
/* ret: STATUS_RANGE_NOT_LOCKED */

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAllByKey (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL
);  
/* ret: STATUS_RANGE_NOT_LOCKED */

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockSingle (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL,
    IN BOOLEAN              AlreadySynchronized
);                      
/* ret:  STATUS_RANGE_NOT_LOCKED */

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFindInTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  Name,
    OUT PUNICODE_STRING ShortName,
    OUT PUNICODE_STRING LongName,
    IN OUT PULONG       DataLength,
    OUT PVOID           Data
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
VOID
NTAPI
FsRtlFreeFileLock (
    IN PFILE_LOCK FileLock
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetFileSize (
    IN PFILE_OBJECT         FileObject,
    IN OUT PLARGE_INTEGER   FileSize
);

/*
  FsRtlGetNextFileLock:

  ret: NULL if no more locks

  Internals:
    FsRtlGetNextFileLock uses FileLock->LastReturnedLockInfo and
    FileLock->LastReturnedLock as storage.
    LastReturnedLock is a pointer to the 'raw' lock inkl. double linked
    list, and FsRtlGetNextFileLock needs this to get next lock on subsequent
    calls with Restart = FALSE.
*/
NTKERNELAPI
PFILE_LOCK_INFO
NTAPI
FsRtlGetNextFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN      Restart
);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeFileLock (
    IN PFILE_LOCK                   FileLock,
    IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeOplock (
    IN OUT POPLOCK Oplock
);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeTunnelCache (
    IN PTUNNEL Cache
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNameInExpression (
    IN PUNICODE_STRING  Expression,
    IN PUNICODE_STRING  Name,
    IN BOOLEAN          IgnoreCase,
    IN PWCHAR           UpcaseTable OPTIONAL
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNtstatusExpected (
    IN NTSTATUS Ntstatus
);

#define FsRtlIsUnicodeCharacterWild(C) (                                    \
    (((C) >= 0x40) ?                                                        \
    FALSE :                                                                 \
    FlagOn((*FsRtlLegalAnsiCharacterArray)[(C)], FSRTL_WILD_CHARACTER ))    \
)

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadComplete (
    IN PFILE_OBJECT     FileObject,
    IN PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev (
    IN PFILE_OBJECT     FileObject,
    IN PMDL             MdlChain,
    IN PDEVICE_OBJECT   DeviceObject
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteComplete (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain,
    IN PDEVICE_OBJECT   DeviceObject
);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNormalizeNtstatus (
    IN NTSTATUS Exception,
    IN NTSTATUS GenericException
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PVOID        FsContext,
    IN PSTRING      FullDirectoryName,
    IN PLIST_ENTRY  NotifyList,
    IN BOOLEAN      WatchTree,
    IN ULONG        CompletionFilter,
    IN PIRP         NotifyIrp
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanup (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY  NotifyList,
    IN PVOID        FsContext
);

typedef BOOLEAN (*PCHECK_FOR_TRAVERSE_ACCESS) (
    IN PVOID                        NotifyContext,
    IN PVOID                        TargetContext,
    IN PSECURITY_SUBJECT_CONTEXT    SubjectContext
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullChangeDirectory (
    IN PNOTIFY_SYNC                 NotifySync,
    IN PLIST_ENTRY                  NotifyList,
    IN PVOID                        FsContext,
    IN PSTRING                      FullDirectoryName,
    IN BOOLEAN                      WatchTree,
    IN BOOLEAN                      IgnoreBuffer,
    IN ULONG                        CompletionFilter,
    IN PIRP                         NotifyIrp,
    IN PCHECK_FOR_TRAVERSE_ACCESS   TraverseCallback OPTIONAL,
    IN PSECURITY_SUBJECT_CONTEXT    SubjectContext OPTIONAL
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY  NotifyList,
    IN PSTRING      FullTargetName,
    IN USHORT       TargetNameOffset,
    IN PSTRING      StreamName OPTIONAL,
    IN PSTRING      NormalizedParentName OPTIONAL,
    IN ULONG        FilterMatch,
    IN ULONG        Action,
    IN PVOID        TargetContext
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyInitializeSync (
    IN PNOTIFY_SYNC NotifySync
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY  NotifyList,
    IN PSTRING      FullTargetName,
    IN PUSHORT      FileNamePartLength,
    IN ULONG        FilterMatch
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyUninitializeSync (
    IN PNOTIFY_SYNC NotifySync
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent (
    IN PFILE_OBJECT FileObject,
    IN ULONG        EventCode
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrl (
    IN POPLOCK  Oplock,
    IN PIRP     Irp,
    IN ULONG    OpenCount
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsFastIoPossible (
    IN POPLOCK Oplock
);

/*
  FsRtlPrivateLock:

  ret: IoStatus->Status: STATUS_PENDING, STATUS_LOCK_NOT_GRANTED

  Internals: 
    -Calls IoCompleteRequest if Irp
    -Uses exception handling / ExRaiseStatus with STATUS_INSUFFICIENT_RESOURCES
*/
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrivateLock (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately, 
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus, 
    IN PIRP                 Irp OPTIONAL,
    IN PVOID                Context,
    IN BOOLEAN              AlreadySynchronized
);

/*
  FsRtlProcessFileLock:

  ret:
    -STATUS_INVALID_DEVICE_REQUEST
    -STATUS_RANGE_NOT_LOCKED from unlock routines.
    -STATUS_PENDING, STATUS_LOCK_NOT_GRANTED from FsRtlPrivateLock
    (redirected IoStatus->Status).

  Internals: 
    -switch ( Irp->CurrentStackLocation->MinorFunction )
        lock: return FsRtlPrivateLock;
        unlocksingle: return FsRtlFastUnlockSingle;
        unlockall: return FsRtlFastUnlockAll;
        unlockallbykey: return FsRtlFastUnlockAllByKey;
        default: IofCompleteRequest with STATUS_INVALID_DEVICE_REQUEST;
                 return STATUS_INVALID_DEVICE_REQUEST;

    -'AllwaysZero' is passed thru as 'AllwaysZero' to lock / unlock routines.
    -'Irp' is passet thru as 'Irp' to FsRtlPrivateLock.
*/
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlProcessFileLock (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp,
    IN PVOID        Context OPTIONAL
);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterUncProvider (
    IN OUT PHANDLE      MupHandle,
    IN PUNICODE_STRING  RedirectorDeviceName,
    IN BOOLEAN          MailslotsSupported
);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeFileLock (
    IN PFILE_LOCK FileLock
);

NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeOplock (
    IN OUT POPLOCK Oplock
);

NTSYSAPI
VOID
NTAPI
HalDisplayString (
    IN PCHAR String
);

NTSYSAPI
VOID
NTAPI
HalQueryRealTimeClock (
    IN OUT PTIME_FIELDS TimeFields
);

NTSYSAPI
VOID
NTAPI
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
);

#define InitializeMessageHeader(m, l, t) {                  \
    (m)->Length = (USHORT)(l);                              \
    (m)->DataLength = (USHORT)(l - sizeof( LPC_MESSAGE ));  \
    (m)->MessageType = (USHORT)(t);                         \
    (m)->DataInfoOffset = 0;                                \
}

NTKERNELAPI
VOID
NTAPI
IoAcquireVpbSpinLock (
    OUT PKIRQL Irql
);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckDesiredAccess (
    IN OUT PACCESS_MASK DesiredAccess,
    IN ACCESS_MASK      GrantedAccess
);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckEaBufferValidity (
    IN PFILE_FULL_EA_INFORMATION    EaBuffer,
    IN ULONG                        EaLength,
    OUT PULONG                      ErrorOffset
);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckFunctionAccess (
    IN ACCESS_MASK              GrantedAccess,
    IN UCHAR                    MajorFunction,
    IN UCHAR                    MinorFunction,
    IN ULONG                    IoControlCode,
    IN PFILE_INFORMATION_CLASS  FileInformationClass OPTIONAL,
    IN PFS_INFORMATION_CLASS    FsInformationClass OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuotaBufferValidity (
    IN PFILE_QUOTA_INFORMATION  QuotaBuffer,
    IN ULONG                    QuotaLength,
    OUT PULONG                  ErrorOffset
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObject (
    IN PFILE_OBJECT     FileObject OPTIONAL,
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectLite (
    IN PFILE_OBJECT     FileObject OPTIONAL,
    IN PDEVICE_OBJECT   DeviceObject OPTIONAL
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
BOOLEAN
NTAPI
IoFastQueryNetworkAttributes (
    IN POBJECT_ATTRIBUTES               ObjectAttributes,
    IN ACCESS_MASK                      DesiredAccess,
    IN ULONG                            OpenOptions,
    OUT PIO_STATUS_BLOCK                IoStatus,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer
);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetAttachedDevice (
    IN PDEVICE_OBJECT DeviceObject
);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetBaseFileSystemDeviceObject (
    IN PFILE_OBJECT FileObject
);

NTKERNELAPI
PEPROCESS
NTAPI
IoGetRequestorProcess (
    IN PIRP Irp
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
ULONG
NTAPI
IoGetRequestorProcessId (
    IN PIRP Irp
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
PIRP
NTAPI
IoGetTopLevelIrp (
    VOID
);

#define IoIsFileOpenedExclusively(FileObject) ( \
    (BOOLEAN) !(                                \
    (FileObject)->SharedRead ||                 \
    (FileObject)->SharedWrite ||                \
    (FileObject)->SharedDelete                  \
    )                                           \
)

NTKERNELAPI
BOOLEAN
NTAPI
IoIsOperationSynchronous (
    IN PIRP Irp
);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsSystemThread (
    IN PETHREAD Thread
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
BOOLEAN
NTAPI
IoIsValidNameGraftingBuffer (
    IN PIRP                 Irp,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
NTSTATUS
NTAPI
IoPageRead (
    IN PFILE_OBJECT         FileObject,
    IN PMDL                 Mdl,
    IN PLARGE_INTEGER       Offset,
    IN PKEVENT              Event,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryFileInformation (
    IN PFILE_OBJECT             FileObject,
    IN FILE_INFORMATION_CLASS   FileInformationClass,
    IN ULONG                    Length,
    OUT PVOID                   FileInformation,
    OUT PULONG                  ReturnedLength
);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryVolumeInformation (
    IN PFILE_OBJECT         FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG                Length,
    OUT PVOID               FsInformation,
    OUT PULONG              ReturnedLength
);

NTKERNELAPI
VOID
NTAPI
IoRegisterFileSystem (
    IN OUT PDEVICE_OBJECT DeviceObject
);

#if (VER_PRODUCTBUILD >= 1381)

typedef VOID (NTAPI *PDRIVER_FS_NOTIFICATION) (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN        DriverActive
);

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChange (
    IN PDRIVER_OBJECT           DriverObject,
    IN PDRIVER_FS_NOTIFICATION  DriverNotificationRoutine
);

#endif /* (VER_PRODUCTBUILD >= 1381) */

NTKERNELAPI
VOID
NTAPI
IoReleaseVpbSpinLock (
    IN KIRQL Irql
);

NTKERNELAPI
VOID
NTAPI
IoSetDeviceToVerify (
    IN PETHREAD         Thread,
    IN PDEVICE_OBJECT   DeviceObject
);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetInformation (
    IN PFILE_OBJECT             FileObject,
    IN FILE_INFORMATION_CLASS   FileInformationClass,
    IN ULONG                    Length,
    IN PVOID                    FileInformation
);

NTKERNELAPI
VOID
NTAPI
IoSetTopLevelIrp (
    IN PIRP Irp
);

NTKERNELAPI
NTSTATUS
NTAPI
IoSynchronousPageWrite (
    IN PFILE_OBJECT         FileObject,
    IN PMDL                 Mdl,
    IN PLARGE_INTEGER       FileOffset,
    IN PKEVENT              Event,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

NTKERNELAPI
PEPROCESS
NTAPI
IoThreadToProcess (
    IN PETHREAD Thread
);

NTKERNELAPI
VOID
NTAPI
IoUnregisterFileSystem (
    IN OUT PDEVICE_OBJECT DeviceObject
);

#if (VER_PRODUCTBUILD >= 1381)

NTKERNELAPI
NTSTATUS
NTAPI
IoUnregisterFsRegistrationChange (
    IN PDRIVER_OBJECT           DriverObject,
    IN PDRIVER_FS_NOTIFICATION  DriverNotificationRoutine
);

#endif /* (VER_PRODUCTBUILD >= 1381) */

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyVolume (
    IN PDEVICE_OBJECT   DeviceObject,
    IN BOOLEAN          AllowRawMount
);

NTKERNELAPI
VOID
NTAPI
KeAttachProcess (
    IN PEPROCESS Process
);

NTKERNELAPI
VOID
NTAPI
KeDetachProcess (
    VOID
);

NTKERNELAPI
VOID
NTAPI
KeInitializeQueue (
    IN PRKQUEUE Queue,
    IN ULONG    Count OPTIONAL
);

NTKERNELAPI
LONG
NTAPI
KeInsertHeadQueue (
    IN PRKQUEUE     Queue,
    IN PLIST_ENTRY  Entry
);

NTKERNELAPI
LONG
NTAPI
KeInsertQueue (
    IN PRKQUEUE     Queue,
    IN PLIST_ENTRY  Entry
);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueApc (
    IN PKAPC      Apc,
    IN PVOID      SystemArgument1,
    IN PVOID      SystemArgument2,
    IN KPRIORITY  PriorityBoost
);

NTKERNELAPI
LONG
NTAPI
KeReadStateQueue (
    IN PRKQUEUE Queue
);

NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRemoveQueue (
    IN PRKQUEUE         Queue,
    IN KPROCESSOR_MODE  WaitMode,
    IN PLARGE_INTEGER   Timeout OPTIONAL
);

NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRundownQueue (
    IN PRKQUEUE Queue
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
VOID
NTAPI
KeStackAttachProcess (
    IN PKPROCESS    Process,
    OUT PKAPC_STATE ApcState
);

NTKERNELAPI
VOID
NTAPI
KeUnstackDetachProcess (
    IN PKAPC_STATE ApcState
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
BOOLEAN
NTAPI
MmCanFileBeTruncated (
    IN PSECTION_OBJECT_POINTERS     SectionObjectPointer,
    IN PLARGE_INTEGER               NewFileSize
);

NTKERNELAPI
BOOLEAN
NTAPI
MmFlushImageSection (
    IN PSECTION_OBJECT_POINTERS     SectionObjectPointer,
    IN MMFLUSH_TYPE                 FlushType
);

NTKERNELAPI
BOOLEAN
NTAPI
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN                  DelayClose
);

#if (VER_PRODUCTBUILD >= 1381)

NTKERNELAPI
BOOLEAN
NTAPI
MmIsRecursiveIoFault (
    VOID
);

#else

#define MmIsRecursiveIoFault() (                            \
    (PsGetCurrentThread()->DisablePageFaultClustering) |    \
    (PsGetCurrentThread()->ForwardClusterOnly)              \
)

#endif

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewOfSection (
    IN PVOID                SectionObject,
    IN PEPROCESS            Process,
    IN OUT PVOID            *BaseAddress,
    IN ULONG                ZeroBits,
    IN ULONG                CommitSize,
    IN OUT PLARGE_INTEGER   SectionOffset OPTIONAL,
    IN OUT PULONG           ViewSize,
    IN SECTION_INHERIT      InheritDisposition,
    IN ULONG                AllocationType,
    IN ULONG                Protect
);

NTKERNELAPI
BOOLEAN
NTAPI
MmSetAddressRangeModified (
    IN PVOID    Address,
    IN ULONG    Length
);

NTKERNELAPI
NTSTATUS
NTAPI
ObCreateObject (
    IN KPROCESSOR_MODE      ObjectAttributesAccessMode OPTIONAL,
    IN POBJECT_TYPE         ObjectType,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE      AccessMode,
    IN OUT PVOID            ParseContext OPTIONAL,
    IN ULONG                ObjectSize,
    IN ULONG                PagedPoolCharge OPTIONAL,
    IN ULONG                NonPagedPoolCharge OPTIONAL,
    OUT PVOID               *Object
);

NTKERNELAPI
ULONG
NTAPI
ObGetObjectPointerCount (
    IN PVOID Object
);

NTKERNELAPI
NTSTATUS
NTAPI
ObInsertObject (
    IN PVOID            Object,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess,
    IN ULONG            AdditionalReferences,
    OUT PVOID           *ReferencedObject OPTIONAL,
    OUT PHANDLE         Handle
);

NTKERNELAPI
VOID
NTAPI
ObMakeTemporaryObject (
    IN PVOID Object
);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointer (
    IN PVOID            Object,
    IN ULONG            HandleAttributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType OPTIONAL,
    IN KPROCESSOR_MODE  AccessMode,
    OUT PHANDLE         Handle
);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryNameString (
    IN PVOID                        Object,
    OUT POBJECT_NAME_INFORMATION    ObjectNameInfo,
    IN ULONG                        Length,
    OUT PULONG                      ReturnLength
);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle (
    IN HANDLE       Handle,
    OUT PBOOLEAN    GenerateOnClose
);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByName (
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            Attributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType,
    IN KPROCESSOR_MODE  AccessMode,
    IN OUT PVOID        ParseContext OPTIONAL,
    OUT PVOID           *Object
);

NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota (
    IN PEPROCESS    Process,
    IN POOL_TYPE    PoolType,
    IN ULONG        Amount
);

#define PsDereferenceImpersonationToken(T)  \
            {if (ARGUMENT_PRESENT(T)) {     \
                (ObDereferenceObject((T))); \
            } else {                        \
                ;                           \
            }                               \
}

#define PsDereferencePrimaryToken(T) (ObDereferenceObject((T)))

NTKERNELAPI
ULONGLONG
NTAPI
PsGetProcessExitTime (
    VOID
);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsThreadTerminating (
    IN PETHREAD Thread
);

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId (
    IN PVOID        ProcessId,
    OUT PEPROCESS   *Process
);

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid (
    IN PCLIENT_ID   Cid,
    OUT PEPROCESS   *Process OPTIONAL,
    OUT PETHREAD    *Thread
);

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupThreadByThreadId (
    IN PVOID        UniqueThreadId,
    OUT PETHREAD    *Thread
);

NTKERNELAPI
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken (
    IN PETHREAD                         Thread,
    OUT PBOOLEAN                        CopyOnUse,
    OUT PBOOLEAN                        EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL   Level
);

NTKERNELAPI
HANDLE
NTAPI
PsReferencePrimaryToken (
    IN PEPROCESS Process
);

NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota (
    IN PEPROCESS    Process,
    IN POOL_TYPE    PoolType,
    IN ULONG        Amount
);

NTKERNELAPI
VOID
NTAPI
PsRevertToSelf (
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD (
    IN PSECURITY_DESCRIPTOR     AbsoluteSecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    IN PULONG                   BufferLength
);

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap (
    IN HANDLE  HeapHandle,
    IN ULONG   Flags,
    IN ULONG   Size
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressBuffer (
    IN USHORT   CompressionFormatAndEngine,
    IN PUCHAR   UncompressedBuffer,
    IN ULONG    UncompressedBufferSize,
    OUT PUCHAR  CompressedBuffer,
    IN ULONG    CompressedBufferSize,
    IN ULONG    UncompressedChunkSize,
    OUT PULONG  FinalCompressedSize,
    IN PVOID    WorkSpace
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressChunks (
    IN PUCHAR                       UncompressedBuffer,
    IN ULONG                        UncompressedBufferSize,
    OUT PUCHAR                      CompressedBuffer,
    IN ULONG                        CompressedBufferSize,
    IN OUT PCOMPRESSED_DATA_INFO    CompressedDataInfo,
    IN ULONG                        CompressedDataInfoLength,
    IN PVOID                        WorkSpace
);

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString (
    OUT PUNICODE_STRING DestinationString,
    IN PSID             Sid,
    IN BOOLEAN          AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid (
    IN ULONG   Length,
    IN PSID    Destination,
    IN PSID    Source
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer (
    IN USHORT   CompressionFormat,
    OUT PUCHAR  UncompressedBuffer,
    IN ULONG    UncompressedBufferSize,
    IN PUCHAR   CompressedBuffer,
    IN ULONG    CompressedBufferSize,
    OUT PULONG  FinalUncompressedSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressChunks (
    OUT PUCHAR                  UncompressedBuffer,
    IN ULONG                    UncompressedBufferSize,
    IN PUCHAR                   CompressedBuffer,
    IN ULONG                    CompressedBufferSize,
    IN PUCHAR                   CompressedTail,
    IN ULONG                    CompressedTailSize,
    IN PCOMPRESSED_DATA_INFO    CompressedDataInfo
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragment (
    IN USHORT   CompressionFormat,
    OUT PUCHAR  UncompressedFragment,
    IN ULONG    UncompressedFragmentSize,
    IN PUCHAR   CompressedBuffer,
    IN ULONG    CompressedBufferSize,
    IN ULONG    FragmentOffset,
    OUT PULONG  FinalUncompressedSize,
    IN PVOID    WorkSpace
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk (
    IN USHORT       CompressionFormat,
    IN OUT PUCHAR   *CompressedBuffer,
    IN PUCHAR       EndOfCompressedBufferPlus1,
    OUT PUCHAR      *ChunkBuffer,
    OUT PULONG      ChunkSize
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid (
    IN PSID Sid1,
    IN PSID Sid2
);

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong (
    IN PVOID    Destination,
    IN ULONG    Length,
    IN ULONG    Fill
);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap (
    IN HANDLE  HeapHandle,
    IN ULONG   Flags,
    IN PVOID   P
);

NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name (
    IN PUNICODE_STRING              Name,
    IN BOOLEAN                      AllowExtendedCharacters,
    IN OUT PGENERATE_NAME_CONTEXT   Context,
    OUT PUNICODE_STRING             Name8dot3
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize (
    IN USHORT   CompressionFormatAndEngine,
    OUT PULONG  CompressBufferWorkSpaceSize,
    OUT PULONG  CompressFragmentWorkSpaceSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN            DaclPresent,
    OUT PACL                *Dacl,
    OUT PBOOLEAN            DaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID                *Group,
    OUT PBOOLEAN            GroupDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID                *Owner,
    OUT PBOOLEAN            OwnerDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid (
    IN OUT PSID                     Sid,
    IN PSID_IDENTIFIER_AUTHORITY    IdentifierAuthority,
    IN UCHAR                        SubAuthorityCount
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3 (
    IN PUNICODE_STRING UnicodeName,
    IN PANSI_STRING    AnsiName,
    PBOOLEAN           Unknown
);

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid (
    IN UCHAR SubAuthorityCount
);

NTSYSAPI
ULONG
NTAPI
RtlLengthSid (
    IN PSID Sid
);

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError (
    IN NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk (
    IN USHORT       CompressionFormat,
    IN OUT PUCHAR   *CompressedBuffer,
    IN PUCHAR       EndOfCompressedBufferPlus1,
    OUT PUCHAR      *ChunkBuffer,
    IN ULONG        ChunkSize
);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime (
    IN ULONG            SecondsSince1970,
    OUT PLARGE_INTEGER  Time
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD (
    IN PSECURITY_DESCRIPTOR     SelfRelativeSD,
    OUT PSECURITY_DESCRIPTOR    AbsoluteSD,
    IN PULONG                   AbsoluteSDSize,
    IN PACL                     Dacl,
    IN PULONG                   DaclSize,
    IN PACL                     Sacl,
    IN PULONG                   SaclSize,
    IN PSID                     Owner,
    IN PULONG                   OwnerSize,
    IN PSID                     PrimaryGroup,
    IN PULONG                   PrimaryGroupSize
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID                     Group,
    IN BOOLEAN                  GroupDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID                     Owner,
    IN BOOLEAN                  OwnerDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN                  SaclPresent,
    IN PACL                     Sacl,
    IN BOOLEAN                  SaclDefaulted
);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid (
    IN PSID Sid
);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid (
    IN PSID    Sid,
    IN ULONG   SubAuthority
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid (
    IN PSID Sid
);

NTKERNELAPI
NTSTATUS
NTAPI
SeAppendPrivileges (
    PACCESS_STATE   AccessState,
    PPRIVILEGE_SET  Privileges
);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEvents (
    IN BOOLEAN              AccessGranted,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents (
    IN BOOLEAN                      AccessGranted,
    IN PSECURITY_DESCRIPTOR         SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT    SubjectContext
);

NTKERNELAPI
VOID
NTAPI
SeCaptureSubjectContext (
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateAccessState (
    OUT PACCESS_STATE   AccessState,
    IN PVOID            AuxData,
    IN ACCESS_MASK      AccessMask,
    IN PGENERIC_MAPPING Mapping
);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurity (
    IN PETHREAD                     Thread,
    IN PSECURITY_QUALITY_OF_SERVICE QualityOfService,
    IN BOOLEAN                      RemoteClient,
    OUT PSECURITY_CLIENT_CONTEXT    ClientContext
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT    SubjectContext,
    IN PSECURITY_QUALITY_OF_SERVICE QualityOfService,
    IN BOOLEAN                      ServerIsRemote,
    OUT PSECURITY_CLIENT_CONTEXT    ClientContext
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

#define SeDeleteClientSecurity(C)  {                                           \
            if (SeTokenType((C)->ClientToken) == TokenPrimary) {               \
                PsDereferencePrimaryToken( (C)->ClientToken );                 \
            } else {                                                           \
                PsDereferenceImpersonationToken( (C)->ClientToken );           \
            }                                                                  \
}

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarm (
    IN PVOID    Object,
    IN HANDLE   Handle
);

#define SeEnableAccessToExports() SeExports = *(PSE_EXPORTS *)SeExports;

NTKERNELAPI
VOID
NTAPI
SeFreePrivileges (
    IN PPRIVILEGE_SET Privileges
);

NTKERNELAPI
VOID
NTAPI
SeImpersonateClient (
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD                 ServerThread OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
SeImpersonateClientEx (
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD                 ServerThread OPTIONAL
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
VOID
NTAPI
SeLockSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTKERNELAPI
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification (
    IN PLUID LogonId
);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarm (
    IN PUNICODE_STRING      ObjectTypeName,
    IN PVOID                Object OPTIONAL,
    IN PUNICODE_STRING      AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE        AccessState,
    IN BOOLEAN              ObjectCreated,
    IN BOOLEAN              AccessGranted,
    IN KPROCESSOR_MODE      AccessMode,
    OUT PBOOLEAN            GenerateOnClose
);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarm (
    IN PUNICODE_STRING      ObjectTypeName,
    IN PVOID                Object OPTIONAL,
    IN PUNICODE_STRING      AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE        AccessState,
    IN BOOLEAN              ObjectCreated,
    IN BOOLEAN              AccessGranted,
    IN KPROCESSOR_MODE      AccessMode,
    OUT PBOOLEAN            GenerateOnClose
);

NTKERNELAPI
BOOLEAN
NTAPI
SePrivilegeCheck (
    IN OUT PPRIVILEGE_SET           RequiredPrivileges,
    IN PSECURITY_SUBJECT_CONTEXT    SubjectContext,
    IN KPROCESSOR_MODE              AccessMode
);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken (
    IN PACCESS_TOKEN    Token,
    OUT PLUID           LogonId
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryInformationToken (
    IN PACCESS_TOKEN           Token,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID                  *TokenInformation
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySecurityDescriptorInfo (
    IN PSECURITY_INFORMATION    SecurityInformation,
    OUT PSECURITY_DESCRIPTOR    SecurityDescriptor,
    IN OUT PULONG               Length,
    IN PSECURITY_DESCRIPTOR     *ObjectsSecurityDescriptor
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySessionIdToken (
    IN PACCESS_TOKEN    Token,
    IN PULONG           SessionId
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

#define SeQuerySubjectContextToken( SubjectContext )                \
    ( ARGUMENT_PRESENT(                                             \
        ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken   \
        ) ?                                                         \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken :     \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->PrimaryToken )

typedef NTSTATUS (*PSE_LOGON_SESSION_TERMINATED_ROUTINE) (
    IN PLUID LogonId
);

NTKERNELAPI
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine (
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
);

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTKERNELAPI
VOID
NTAPI
SeSetAccessStateGenericMapping (
    PACCESS_STATE       AccessState,
    PGENERIC_MAPPING    GenericMapping
);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfo (
    IN PVOID                    Object OPTIONAL,
    IN PSECURITY_INFORMATION    SecurityInformation,
    IN PSECURITY_DESCRIPTOR     SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE                PoolType,
    IN PGENERIC_MAPPING         GenericMapping
);

#if (VER_PRODUCTBUILD >= 2195)

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfoEx (
    IN PVOID                    Object OPTIONAL,
    IN PSECURITY_INFORMATION    SecurityInformation,
    IN PSECURITY_DESCRIPTOR     ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG                    AutoInheritFlags,
    IN POOL_TYPE                PoolType,
    IN PGENERIC_MAPPING         GenericMapping
);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsAdmin (
    IN PACCESS_TOKEN Token
);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsRestricted (
    IN PACCESS_TOKEN Token
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTKERNELAPI
TOKEN_TYPE
NTAPI
SeTokenType (
    IN PACCESS_TOKEN Token
);

NTKERNELAPI
VOID
NTAPI
SeUnlockSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTKERNELAPI
NTSTATUS
SeUnregisterLogonSessionTerminatedRoutine (
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken (
    IN HANDLE               TokenHandle,
    IN BOOLEAN              DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES    NewState,
    IN ULONG                BufferLength,
    OUT PTOKEN_PRIVILEGES   PreviousState OPTIONAL,
    OUT PULONG              ReturnLength
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread (
    IN HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory (
    IN HANDLE       ProcessHandle,
    IN OUT PVOID    *BaseAddress,
    IN ULONG        ZeroBits,
    IN OUT PULONG   RegionSize,
    IN ULONG        AllocationType,
    IN ULONG        Protect
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm (
    IN PUNICODE_STRING      SubsystemName,
    IN PVOID                HandleId,
    IN PUNICODE_STRING      ObjectTypeName,
    IN PUNICODE_STRING      ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK          DesiredAccess,
    IN PGENERIC_MAPPING     GenericMapping,
    IN BOOLEAN              ObjectCreation,
    OUT PACCESS_MASK        GrantedAccess,
    OUT PBOOLEAN            AccessStatus,
    OUT PBOOLEAN            GenerateOnClose
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent (
    IN HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm (
    IN PUNICODE_STRING  SubsystemName,
    IN PVOID            HandleId,
    IN BOOLEAN          GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection (
    OUT PHANDLE             SectionHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER       MaximumSize OPTIONAL,
    IN ULONG                SectionPageProtection,
    IN ULONG                AllocationAttributes,
    IN HANDLE               FileHandle OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject (
    OUT PHANDLE             SymbolicLinkHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PUNICODE_STRING      TargetName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile (
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey (
    IN HANDLE           Handle,
    IN PUNICODE_STRING  Name
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN ULONG                IoControlCode,
    IN PVOID                InputBuffer OPTIONAL,
    IN ULONG                InputBufferLength,
    OUT PVOID               OutputBuffer OPTIONAL,
    IN ULONG                OutputBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString (
    IN PUNICODE_STRING String
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject (
    IN HANDLE       SourceProcessHandle,
    IN HANDLE       SourceHandle,
    IN HANDLE       TargetProcessHandle OPTIONAL,
    OUT PHANDLE     TargetHandle OPTIONAL,
    IN ACCESS_MASK  DesiredAccess,
    IN ULONG        HandleAttributes,
    IN ULONG        Options
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken (
    IN HANDLE               ExistingTokenHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN BOOLEAN              EffectiveOnly,
    IN TOKEN_TYPE           TokenType,
    OUT PHANDLE             NewTokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache (
    IN HANDLE   ProcessHandle,
    IN PVOID    BaseAddress OPTIONAL,
    IN ULONG    FlushSize
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory (
    IN HANDLE               ProcessHandle,
    IN OUT PVOID            *BaseAddress,
    IN OUT PULONG           FlushSize,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory (
    IN HANDLE       ProcessHandle,
    IN OUT PVOID    *BaseAddress,
    IN OUT PULONG   RegionSize,
    IN ULONG        FreeType
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN ULONG                FsControlCode,
    IN PVOID                InputBuffer OPTIONAL,
    IN ULONG                InputBufferLength,
    OUT PVOID               OutputBuffer OPTIONAL,
    IN ULONG                OutputBufferLength
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction (
    IN POWER_ACTION         SystemAction,
    IN SYSTEM_POWER_STATE   MinSystemState,
    IN ULONG                Flags,
    IN BOOLEAN              Asynchronous
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver (
    /* "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\<DriverName>" */
    IN PUNICODE_STRING RegistryPath
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey (
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey (
    IN HANDLE               KeyHandle,
    IN HANDLE               EventHandle OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN ULONG                NotifyFilter,
    IN BOOLEAN              WatchSubtree,
    IN PVOID                Buffer,
    IN ULONG                BufferLength,
    IN BOOLEAN              Asynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject (
    OUT PHANDLE             DirectoryHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent (
    OUT PHANDLE             EventHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess (
    OUT PHANDLE             ProcessHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PCLIENT_ID           ClientId OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken (
    IN HANDLE       ProcessHandle,
    IN ACCESS_MASK  DesiredAccess,
    OUT PHANDLE     TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread (
    OUT PHANDLE             ThreadHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PCLIENT_ID           ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken (
    IN HANDLE       ThreadHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN BOOLEAN      OpenAsSelf,
    OUT PHANDLE     TokenHandle
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation (
    IN POWER_INFORMATION_LEVEL  PowerInformationLevel,
    IN PVOID                    InputBuffer OPTIONAL,
    IN ULONG                    InputBufferLength,
    OUT PVOID                   OutputBuffer OPTIONAL,
    IN ULONG                    OutputBufferLength
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent (
    IN HANDLE   EventHandle,
    OUT PULONG  PreviousState OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale (
    IN BOOLEAN  ThreadOrSystem,
    OUT PLCID   Locale
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile (
    IN HANDLE                   FileHandle,
    IN HANDLE                   Event OPTIONAL,
    IN PIO_APC_ROUTINE          ApcRoutine OPTIONAL,
    IN PVOID                    ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK        IoStatusBlock,
    OUT PVOID                   FileInformation,
    IN ULONG                    Length,
    IN FILE_INFORMATION_CLASS   FileInformationClass,
    IN BOOLEAN                  ReturnSingleEntry,
    IN PUNICODE_STRING          FileName OPTIONAL,
    IN BOOLEAN                  RestartScan
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject (
    IN HANDLE       DirectoryHandle,
    OUT PVOID       Buffer,
    IN ULONG        Length,
    IN BOOLEAN      ReturnSingleEntry,
    IN BOOLEAN      RestartScan,
    IN OUT PULONG   Context,
    OUT PULONG      ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               Buffer,
    IN ULONG                Length,
    IN BOOLEAN              ReturnSingleEntry,
    IN PVOID                EaList OPTIONAL,
    IN ULONG                EaListLength,
    IN PULONG               EaIndex OPTIONAL,
    IN BOOLEAN              RestartScan
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess (
    IN HANDLE           ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID           ProcessInformation,
    IN ULONG            ProcessInformationLength,
    OUT PULONG          ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken (
    IN HANDLE                   TokenHandle,
    IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
    OUT PVOID                   TokenInformation,
    IN ULONG                    Length,
    OUT PULONG                  ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject (
    IN HANDLE                      ObjectHandle,
    IN OBJECT_INFORMATION_CLASS    ObjectInformationClass,
    OUT PVOID                      ObjectInformation,
    IN ULONG                       Length,
    OUT PULONG                     ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySection (
    IN HANDLE                       SectionHandle,
    IN SECTION_INFORMATION_CLASS    SectionInformationClass,
    OUT PVOID                       SectionInformation,
    IN ULONG                        SectionInformationLength,
    OUT PULONG                      ResultLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject (
    IN HANDLE                   FileHandle,
    IN SECURITY_INFORMATION     SecurityInformation,
    OUT PSECURITY_DESCRIPTOR    SecurityDescriptor,
    IN ULONG                    Length,
    OUT PULONG                  ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID                   SystemInformation,
    IN ULONG                    Length,
    OUT PULONG                  ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               FsInformation,
    IN ULONG                Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey (
    IN POBJECT_ATTRIBUTES   NewFileObjectAttributes,
    IN HANDLE               KeyHandle,
    IN POBJECT_ATTRIBUTES   OldFileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent (
    IN HANDLE   EventHandle,
    OUT PULONG  PreviousState OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey (
    IN HANDLE   KeyHandle,
    IN HANDLE   FileHandle,
    IN ULONG    Flags
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey (
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale (
    IN BOOLEAN  ThreadOrSystem,
    IN LCID     Locale
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage (
    IN LANGID LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               Buffer,
    IN ULONG                Length
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent (
    IN HANDLE   EventHandle,
    OUT PULONG  PreviousState OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationObject (
    IN HANDLE                       ObjectHandle,
    IN OBJECT_INFORMATION_CLASS    ObjectInformationClass,
    IN PVOID                        ObjectInformation,
    IN ULONG                        ObjectInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess (
    IN HANDLE           ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID            ProcessInformation,
    IN ULONG            ProcessInformationLength
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject (
    IN HANDLE               Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID                    SystemInformation,
    IN ULONG                    Length
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime (
    IN PLARGE_INTEGER   NewTime,
    OUT PLARGE_INTEGER  OldTime OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN PVOID                FsInformation,
    IN ULONG                Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess (
    IN HANDLE   ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver (
    /* "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\<DriverName>" */
    IN PUNICODE_STRING RegistryPath
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey (
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject (
    IN HANDLE           Handle,
    IN BOOLEAN          Alertable,
    IN PLARGE_INTEGER   Timeout OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    IN ULONG            HandleCount,
    IN PHANDLE          Handles,
    IN WAIT_TYPE        WaitType,
    IN BOOLEAN          Alertable,
    IN PLARGE_INTEGER   Timeout OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* _NTIFS_ */
