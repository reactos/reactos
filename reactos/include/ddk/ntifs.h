/*
    This is a free version of the file ntifs.h, release 8.
    The purpose of this include file is to build Windows NT file system and
    file system filter drivers.
    Copyright (C) 1999  Bo Branten
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    DISCLAIMER: I do not encourage anyone to use this include file to build
    Windows NT file system or file system filter drivers used in production.
    The information in this include file is incomplete and intended only as
    an studying companion. The information has been found in books, magazines,
    on the Internet and received from contributors.

    Please send comments, corrections and contributions to bosse@acc.umu.se

    The most recent version of this file is available from:
    http://www.acc.umu.se/~bosse/ntifs.h

    Thanks to Andrey Shedel, Bartjan Wattel and Darja Isaksson.

    Revision history:

    8. Corrected:
         ZwOpenProcessToken
         ZwOpenThreadToken
       Added:
         Defines:
           FILE_OPLOCK_BROKEN_TO_LEVEL_2
           FILE_OPLOCK_BROKEN_TO_NONE
           FILE_CASE_SENSITIVE_SEARCH
           FILE_CASE_PRESERVED_NAMES
           FILE_UNICODE_ON_DISK
           FILE_PERSISTENT_ACLS
           FILE_FILE_COMPRESSION
           FILE_VOLUME_IS_COMPRESSED
           FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX
           FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH
           IOCTL_REDIR_QUERY_PATH
         Structures:
           FILE_FS_LABEL_INFORMATION
           PATHNAME_BUFFER
         In IO_STACK_LOCATION:
           FileSystemControl
           LockControl
           SetVolume
         Function prototypes:
           FsRtlCopyRead
           FsRtlCopyWrite
           IoVerifyVolume

    7. Added:
         defines for TOKEN_XXX
         SID_NAME_USE
         TOKEN_INFORMATION_CLASS
         TOKEN_TYPE
         FILE_FS_ATTRIBUTE_INFORMATION
         FILE_FS_SIZE_INFORMATION
         SID_IDENTIFIER_AUTHORITY
         SID
         SID_AND_ATTRIBUTES
         TOKEN_CONTROL
         TOKEN_DEFAULT_DACL
         TOKEN_GROUPS
         TOKEN_OWNER
         TOKEN_PRIMARY_GROUP
         TOKEN_PRIVILEGES
         TOKEN_SOURCE
         TOKEN_STATISTICS
         TOKEN_USER
         IoCreateFile
         IoGetAttachedDevice
         IoGetBaseFileSystemDeviceObject
         PsReferenceImpersonationToken
         PsReferencePrimaryToken
         RtlConvertSidToUnicodeString
         SeCaptureSubjectContext
         SeMarkLogonSessionForTerminationNotification
         SeRegisterLogonSessionTerminatedRoutine
         SeUnregisterLogonSessionTerminatedRoutine
         ZwOpenProcessToken
         ZwOpenThreadToken
         ZwQueryInformationToken

    6. Corrected declarations of Zw functions.
       Added:
         ZwCancelIoFile
         ZwDeleteFile
         ZwFlushBuffersFile
         ZwFsControlFile
         ZwLockFile
         ZwNotifyChangeDirectoryFile
         ZwOpenFile
         ZwQueryEaFile
         ZwSetEaFile
         ZwSetVolumeInformationFile
         ZwUnlockFile

    5. Added:
         defines for FILE_ACTION_XXX and FILE_NOTIFY_XXX
         FILE_FS_VOLUME_INFORMATION
         RETRIEVAL_POINTERS_BUFFER
         STARTING_VCN_INPUT_BUFFER
         FsRtlNotifyFullReportChange

    4. Corrected:
         ZwCreateThread
       Added:
         define _GNU_NTIFS_

    3. Added:
         defines for MAP_XXX, MEM_XXX and SEC_XXX
         FILE_BOTH_DIR_INFORMATION
         FILE_DIRECTORY_INFORMATION
         FILE_FULL_DIR_INFORMATION
         FILE_NAMES_INFORMATION
         FILE_NOTIFY_INFORMATION
         FsRtlNotifyCleanup
         KeAttachProcess
         KeDetachProcess
         MmCreateSection
         ZwCreateProcess
         ZwCreateThread
         ZwDeviceIoControlFile
         ZwGetContextThread
         ZwLoadDriver
         ZwOpenDirectoryObject
         ZwOpenProcess
         ZwOpenSymbolicLinkObject
         ZwQueryDirectoryFile
         ZwUnloadDriver

    2. Added:
         FILE_COMPRESSION_INFORMATION
         FILE_STREAM_INFORMATION
         FILE_LINK_INFORMATION
         FILE_RENAME_INFORMATION
         EXTENDED_IO_STACK_LOCATION
         IoQueryFileInformation
         IoQueryFileVolumeInformation
         ZwQueryVolumeInformationFile
       Moved include of ntddk.h to inside extern "C" block.

    1. Initial release.
*/

#ifndef _NTIFS_
#define _NTIFS_
#define _GNU_NTIFS_

#ifdef __cplusplus
extern "C" {
#endif

#include <ntddk.h>

#define FILE_ACTION_ADDED               0x00000001
#define FILE_ACTION_REMOVED             0x00000002
#define FILE_ACTION_MODIFIED            0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME    0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME    0x00000005
#define FILE_ACTION_ADDED_STREAM        0x00000006
#define FILE_ACTION_REMOVED_STREAM      0x00000007
#define FILE_ACTION_MODIFIED_STREAM     0x00000008

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

#define FILE_OPLOCK_BROKEN_TO_LEVEL_2   0x00000007
#define FILE_OPLOCK_BROKEN_TO_NONE      0x00000008

#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
#define FILE_CASE_PRESERVED_NAMES       0x00000002
#define FILE_UNICODE_ON_DISK            0x00000004
#define FILE_PERSISTENT_ACLS            0x00000008
#define FILE_FILE_COMPRESSION           0x00000010
#define FILE_VOLUME_IS_COMPRESSED       0x00008000

#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX (0x08)
#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH (0x10)

#define FSRTL_FSP_TOP_LEVEL_IRP         (0x01)
#define FSRTL_CACHE_TOP_LEVEL_IRP       (0x02)
#define FSRTL_MOD_WRITE_TOP_LEVEL_IRP   (0x03)
#define FSRTL_FAST_IO_TOP_LEVEL_IRP     (0x04)
#define FSRTL_MAX_TOP_LEVEL_IRP_FLAG    (0x04)

#define IOCTL_REDIR_QUERY_PATH CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
                                        99, METHOD_NEITHER, FILE_ANY_ACCESS)

#define MAP_PROCESS                     1L
#define MAP_SYSTEM                      2L

#define MEM_DOS_LIM                     0x40000000
#define MEM_IMAGE                       SEC_IMAGE

#define SEC_BASED                       0x00200000
#define SEC_NO_CHANGE                   0x00400000
#define SEC_FILE                        0x00800000
#define SEC_IMAGE                       0x01000000
#define SEC_COMMIT                      0x08000000
#define SEC_NOCACHE                     0x10000000

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

typedef PVOID PNOTIFY_SYNC;

typedef enum _FAST_IO_POSSIBLE {
    FastIoIsPossible,
    FastIoIsNotPossible,
    FastIoIsQuestionable
} FAST_IO_POSSIBLE;

typedef enum _MMFLUSH_TYPE {
    MmFlushForDelete,
    MmFlushForWrite
} MMFLUSH_TYPE;

typedef enum _SID_NAME_USE {
    SidTypeUser = 1,
    SidTypeGroup,
    SidTypeDomain,
    SidTypeAlias,
    SidTypeWellKnownGroup,
    SidTypeDeletedAccount,
    SidTypeInvalid,
    SidTypeUnknown
} SID_NAME_USE, *PSID_NAME_USE;

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1,
    TokenGroups,
    TokenPrivileges,
    TokenOwner,
    TokenPrimaryGroup,
    TokenDefaultDacl,
    TokenSource,
    TokenType,
    TokenImpersonationLevel,
    TokenStatistics,
    TokenRestrictedSids
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

typedef enum _TOKEN_TYPE {
    TokenPrimary = 1,
    TokenImpersonation
} TOKEN_TYPE, *PTOKEN_TYPE;

typedef struct _CACHE_UNINITIALIZE_EVENT {
    struct _CACHE_UNINITIALIZE_EVENT    *Next;
    KEVENT                              Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

typedef struct _CC_FILE_SIZES {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;

/*
 * When needing these cast your PIO_STACK_LOCATION to
 * PEXTENDED_IO_STACK_LOCATION
 */
typedef struct _EXTENDED_IO_STACK_LOCATION {

    union {

        struct {
            ULONG                   OutputBufferLength;
            ULONG                   InputBufferLength;
            ULONG                   FsControlCode;
            PVOID                   Type3InputBuffer;
        } FileSystemControl;

        struct {
            PLARGE_INTEGER          Length;
            ULONG                   Key;
            LARGE_INTEGER           ByteOffset;
        } LockControl;

        struct {
            ULONG                   Length;
            ULONG                   CompletionFilter;
        } NotifyDirectory;

        struct {
            ULONG                   Length;
            PSTRING                 FileName;
            FILE_INFORMATION_CLASS  FileInformationClass;
            ULONG                   FileIndex;
        } QueryDirectory;

        struct {
            ULONG                   Length;
            PVOID                   EaList;
            ULONG                   EaListLength;
            ULONG                   EaaIndex;
        } QueryEa;

        struct {
            ULONG                   Length;
            FS_INFORMATION_CLASS    FsInformationClass;
        } SetVolume;

    } Parameters;

} EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;

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

typedef struct _FILE_COMPRESSION_INFORMATION {
    LARGE_INTEGER CompressedFileSize;
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

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

typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG   FileSystemAttributes;
    LONG    MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

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

typedef struct _FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_NOTIFY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG Action;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NOTIFY_INFORMATION, *PFILE_NOTIFY_INFORMATION;

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

typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION      BasicInformation;
    FILE_STANDARD_INFORMATION   StandardInformation;
    FILE_INTERNAL_INFORMATION   InternalInformation;
    FILE_EA_INFORMATION         EaInformation;
    //FILE_ACCESS_INFORMATION   AccessInformation;      // Yet unknown
    FILE_POSITION_INFORMATION   PositionInformation;
    //FILE_MODE_INFORMATION     ModeInformation;        // Yet unknown
    FILE_ALIGNMENT_INFORMATION  AlignmentInformation;
    FILE_NAME_INFORMATION       NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FSRTL_COMMON_FCB_HEADER {
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    UCHAR           Flags;
    UCHAR           IsFastIoPossible;
#if (_WIN32_WINNT >= 0x0400)
    UCHAR           Flags2;
    UCHAR           Reserved;
#endif // (_WIN32_WINNT >= 0x0400)
    PERESOURCE      Resource;
    PERESOURCE      PagingIoResource;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

typedef struct _PATHNAME_BUFFER {
    ULONG PathNameLength;
    WCHAR Name[1];
} PATHNAME_BUFFER, *PPATHNAME_BUFFER;

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

typedef struct _SID_IDENTIFIER_AUTHORITY {
    UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
    UCHAR                       Revision;
    UCHAR                       SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY    IdentifierAuthority;
    ULONG                       SubAuthority[1];
} SID;

typedef struct _SID_AND_ATTRIBUTES {
    PSID    Sid;
    ULONG   Attributes;
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;

typedef struct _STARTING_VCN_INPUT_BUFFER {
    LARGE_INTEGER StartingVcn;
} STARTING_VCN_INPUT_BUFFER, *PSTARTING_VCN_INPUT_BUFFER;

typedef struct _TOKEN_SOURCE {
    CCHAR   SourceName[TOKEN_SOURCE_LENGTH];
    LUID    SourceIdentifier;
} TOKEN_SOURCE, *PTOKEN_SOURCE;

typedef struct _TOKEN_CONTROL {
    LUID            TokenId;
    LUID            AuthenticationId;
    LUID            ModifiedId;
    TOKEN_SOURCE    TokenSource;
} TOKEN_CONTROL, *PTOKEN_CONTROL;

typedef struct _TOKEN_DEFAULT_DACL {
    PACL DefaultDacl;
} TOKEN_DEFAULT_DACL, *PTOKEN_DEFAULT_DACL;

typedef struct _TOKEN_GROUPS {
    ULONG               GroupCount;
    SID_AND_ATTRIBUTES  Groups[1];
} TOKEN_GROUPS, *PTOKEN_GROUPS;

typedef struct _TOKEN_OWNER {
    PSID Owner;
} TOKEN_OWNER, *PTOKEN_OWNER;

typedef struct _TOKEN_PRIMARY_GROUP {
    PSID PrimaryGroup;
} TOKEN_PRIMARY_GROUP, *PTOKEN_PRIMARY_GROUP;

typedef struct _TOKEN_PRIVILEGES {
    ULONG               PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[1];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

typedef struct _TOKEN_STATISTICS {
    LUID                            TokenId;
    LUID                            AuthenticationId;
    LARGE_INTEGER                   ExpirationTime;
    TOKEN_TYPE                      TokenType;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
    ULONG                           DynamicCharged;
    ULONG                           DynamicAvailable;
    ULONG                           GroupCount;
    ULONG                           PrivilegeCount;
    LUID                            ModifiedId;
} TOKEN_STATISTICS, *PTOKEN_STATISTICS;

typedef struct _TOKEN_USER {
    SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

NTKERNELAPI
BOOLEAN
CcCanIWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG        BytesToWrite,
    IN BOOLEAN      Wait,
    IN BOOLEAN      Retrying
);

NTKERNELAPI
BOOLEAN
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
CcCopyWrite (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Wait,
    IN PVOID            Buffer
);

typedef VOID (*PCC_POST_DEFERRED_WRITE) (
    IN PVOID Context1,
    IN PVOID Context2
);

NTKERNELAPI
VOID
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
CcFastCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG        FileOffset,
    IN ULONG        Length,
    IN PVOID        Buffer
);

NTKERNELAPI
VOID
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
CcGetDirtyPages (
    IN PVOID                LogHandle,
    IN PDIRTY_PAGE_ROUTINE  DirtyPageRoutine,
    IN PVOID                Context1,
    IN PVOID                Context2
);

NTKERNELAPI
PFILE_OBJECT
CcGetFileObjectFromBcb (
    IN PVOID Bcb
);

NTKERNELAPI
PFILE_OBJECT
CcGetFileObjectFromSectionPtrs (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
);

NTKERNELAPI
LARGE_INTEGER
CcGetLsnForFileObject(
    IN PFILE_OBJECT     FileObject,
    OUT PLARGE_INTEGER  OldestLsn OPTIONAL
);

typedef BOOLEAN (*PACQUIRE_FOR_LAZY_WRITE) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (*PRELEASE_FROM_LAZY_WRITE) (
    IN PVOID Context
);

typedef BOOLEAN (*PACQUIRE_FOR_READ_AHEAD) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (*PRELEASE_FROM_READ_AHEAD) (
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
CcInitializeCacheMap (
    IN PFILE_OBJECT             FileObject,
    IN PCC_FILE_SIZES           FileSizes,
    IN BOOLEAN                  PinAccess,
    IN PCACHE_MANAGER_CALLBACKS Callbacks,
    IN PVOID                    LazyWriteContext
);

NTKERNELAPI
BOOLEAN
CcIsThereDirtyData (
    IN PVPB Vpb
);

NTKERNELAPI
BOOLEAN
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
CcMdlRead (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    OUT PMDL                *MdlChain,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
VOID
CcMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL         MdlChain
);

NTKERNELAPI
VOID
CcMdlWriteComplete (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
CcPinMappedData (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Wait,
    IN OUT PVOID        *Bcb
);

NTKERNELAPI
BOOLEAN
CcPinRead (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Wait,
    OUT PVOID           *Bcb,
    OUT PVOID           *Buffer
);

NTKERNELAPI
VOID
CcPrepareMdlWrite (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    OUT PMDL                *MdlChain,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
BOOLEAN
CcPreparePinWrite (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN BOOLEAN          Zero,
    IN BOOLEAN          Wait,
    OUT PVOID           *Bcb,
    OUT PVOID           *Buffer
);

NTKERNELAPI
BOOLEAN
CcPurgeCacheSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER           FileOffset OPTIONAL,
    IN ULONG                    Length,
    IN BOOLEAN                  UninitializeCacheMaps
);

#define CcReadAhead(FO, FOFF, LEN) ( \
    if ((LEN) >= 256) { \
        CcScheduleReadAhead((FO), (FOFF), (LEN)); \
    } \
)

NTKERNELAPI
VOID
CcRepinBcb (
    IN PVOID Bcb
);

NTKERNELAPI
VOID
CcScheduleReadAhead (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length
);

NTKERNELAPI
VOID
CcSetAdditionalCacheAttributes (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN      DisableReadAhead,
    IN BOOLEAN      DisableWriteBehind
);

NTKERNELAPI
VOID
CcSetBcbOwnerPointer (
    IN PVOID Bcb,
    IN PVOID OwnerPointer
);

NTKERNELAPI
VOID
CcSetDirtyPageThreshold (
    IN PFILE_OBJECT FileObject,
    IN ULONG        DirtyPageThreshold
);

NTKERNELAPI
VOID
CcSetDirtyPinnedData (
    IN PVOID            Bcb,
    IN PLARGE_INTEGER   Lsn OPTIONAL
);

NTKERNELAPI
VOID
CcSetFileSizes (
    IN PFILE_OBJECT     FileObject,
    IN PCC_FILE_SIZES   FileSizes
);

typedef VOID (*PFLUSH_TO_LSN) (
    IN PVOID            LogHandle,
    IN PLARGE_INTEGER   Lsn
);

NTKERNELAPI
VOID
CcSetLogHandleForFile (
    IN PFILE_OBJECT     FileObject,
    IN PVOID            LogHandle,
    IN PFLUSH_TO_LSN    FlushToLsnRoutine
);

NTKERNELAPI
VOID
CcSetReadAheadGranularity (
    IN PFILE_OBJECT FileObject,
    IN ULONG        Granularity     // default: PAGE_SIZE, allowed: 2^n * PAGE_SIZE
);

NTKERNELAPI
BOOLEAN
CcUninitializeCacheMap (
    IN PFILE_OBJECT                 FileObject,
    IN PLARGE_INTEGER               TruncateSize OPTIONAL,
    IN PCACHE_UNINITIALIZE_EVENT    UninitializeCompleteEvent OPTIONAL
);

NTKERNELAPI
VOID
CcUnpinData (
    IN PVOID Bcb
);

NTKERNELAPI
VOID
CcUnpinDataForThread (
    IN PVOID            Bcb,
    IN ERESOURCE_THREAD ResourceThreadId
);

NTKERNELAPI
VOID
CcUnpinRepinnedBcb (
    IN PVOID                Bcb,
    IN BOOLEAN              WriteThrough,
    OUT PIO_STATUS_BLOCK    IoStatus
);

NTKERNELAPI
BOOLEAN
CcZeroData (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   StartOffset,
    IN PLARGE_INTEGER   EndOffset,
    IN BOOLEAN          Wait
);

NTKERNELAPI
VOID
CcZeroEndOfLastPage (
    IN PFILE_OBJECT FileObject
);

NTKERNELAPI
BOOLEAN
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

#define FsRtlEnterFileSystem    KeEnterCriticalRegion

#define FsRtlExitFileSystem     KeLeaveCriticalRegion

NTKERNELAPI
BOOLEAN
FsRtlIsNtstatusExpected (
    IN NTSTATUS Ntstatus
);

NTKERNELAPI
BOOLEAN
FsRtlMdlReadCompleteDev (
    IN PFILE_OBJECT     FileObject,
    IN PMDL             MdlChain,
    IN PDEVICE_OBJECT   DeviceObject
);

NTKERNELAPI
BOOLEAN
FsRtlMdlWriteCompleteDev (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain,
    IN PDEVICE_OBJECT   DeviceObject
);

NTKERNELAPI
VOID
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
IoAcquireVpbSpinLock (
    OUT PKIRQL Irql
);

NTKERNELAPI
NTSTATUS
IoCreateFile (
    OUT PHANDLE             FileHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN PLARGE_INTEGER       AllocationSize OPTIONAL,
    IN ULONG                FileAttributes,
    IN ULONG                ShareAccess,
    IN ULONG                CreateDisposition,
    IN ULONG                CreateOptions,
    IN PVOID                EaBuffer OPTIONAL,
    IN ULONG                EaLength,
    IN CREATE_FILE_TYPE     CreateFileType,
    IN ULONG                ExtraCreateParameters,
    IN ULONG                Options
);

NTKERNELAPI
PFILE_OBJECT
IoCreateStreamFileObject (
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject
);

NTKERNELAPI
PDEVICE_OBJECT
IoGetAttachedDevice (
    IN PDEVICE_OBJECT DeviceObject
);

NTKERNELAPI
PDEVICE_OBJECT
IoGetBaseFileSystemDeviceObject (
    IN PFILE_OBJECT FileObject
);

NTKERNELAPI
PIRP
IoGetTopLevelIrp (
    VOID
);

NTKERNELAPI
BOOLEAN
IoIsOperationSynchronous (
    IN PIRP Irp
);

NTKERNELAPI
NTSTATUS
IoQueryFileInformation (
    IN PFILE_OBJECT             FileObject,
    IN FILE_INFORMATION_CLASS   FileInformationClass,
    IN ULONG                    Length,
    OUT PVOID                   FileInformation,
    OUT PULONG                  ReturnedLength
);

NTKERNELAPI
NTSTATUS
IoQueryFileVolumeInformation (
    IN PFILE_OBJECT         FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG                Length,
    OUT PVOID               FsInformation,
    OUT PULONG              ReturnedLength
);

NTKERNELAPI
VOID
IoRegisterFileSystem (
    IN PDEVICE_OBJECT DeviceObject
);

#if (_WIN32_WINNT >= 0x0400)
typedef VOID (*PFSDNOTIFICATIONPROC) (
    IN PDEVICE_OBJECT PtrTargetFileSystemDeviceObject,
    IN BOOLEAN        DriverActive
);

NTKERNELAPI
NTSTATUS
IoRegisterFsRegistrationChange (
    IN PDRIVER_OBJECT       DriverObject,
    IN PFSDNOTIFICATIONPROC FSDNotificationProc
);
#endif // (_WIN32_WINNT >= 0x0400)

NTKERNELAPI
VOID
IoReleaseVpbSpinLock (
    IN KIRQL Irql
);

NTKERNELAPI
VOID
IoSetTopLevelIrp (
    IN PIRP Irp
);

NTKERNELAPI
VOID
IoUnregisterFileSystem (
    IN PDEVICE_OBJECT DeviceObject
);

NTKERNELAPI
NTSTATUS
IoVerifyVolume (
    IN PDEVICE_OBJECT   DeviceObject,
    IN BOOLEAN          AllowRawMount
);

NTKERNELAPI
VOID
KeAttachProcess (
    IN PEPROCESS Process
);

NTKERNELAPI
VOID
KeDetachProcess (
    VOID
);

NTKERNELAPI
BOOLEAN
MmCanFileBeTruncated (
    IN PSECTION_OBJECT_POINTERS     SectionObjectPointer,
    IN PLARGE_INTEGER               NewFileSize
);

NTKERNELAPI
NTSTATUS
MmCreateSection (
    OUT PVOID               *SectionObject,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER       MaximumSize,
    IN ULONG                SectionPageProtection,
    IN ULONG                AllocationAttributes,
    IN HANDLE               FileHandle OPTIONAL,
    IN PFILE_OBJECT         File OPTIONAL
);

NTKERNELAPI
BOOLEAN
MmFlushImageSection (
    IN PSECTION_OBJECT_POINTERS     SectionObjectPointer,
    IN MMFLUSH_TYPE                 FlushType
);

#define MmIsRecursiveIoFault() ( \
    (PsGetCurrentThread()->DisablePageFaultClustering) | \
    (PsGetCurrentThread()->ForwardClusterOnly) \
)

NTKERNELAPI
NTSTATUS
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
HANDLE
PsReferenceImpersonationToken (
    IN PETHREAD Thread
);

NTKERNELAPI
HANDLE
PsReferencePrimaryToken (
    IN PEPROCESS Process
);

NTKERNELAPI
NTSTATUS
RtlConvertSidToUnicodeString (
    OUT PUNICODE_STRING DestinationString,
    IN PVOID            Sid,
    IN BOOLEAN          AllocateDestinationString
);

NTKERNELAPI
VOID
SeCaptureSubjectContext (
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTKERNELAPI
NTSTATUS
SeMarkLogonSessionForTerminationNotification (
    IN PLUID LogonId
);

typedef NTSTATUS (*PSE_LOGON_SESSION_TERMINATED_ROUTINE) (
    IN PLUID LogonId
);

NTKERNELAPI
NTSTATUS
SeRegisterLogonSessionTerminatedRoutine (
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
);

NTKERNELAPI
NTSTATUS
SeUnregisterLogonSessionTerminatedRoutine (
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
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
ZwCancelIoFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess (
    OUT PHANDLE             ProcessHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN HANDLE               ParentProcessHandle,
    IN BOOLEAN              InheritObjectTable,
    IN HANDLE               SectionHandle,
    IN HANDLE               DebugPort,
    IN HANDLE               ExceptionPort
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
ZwCreateThread (
    OUT PHANDLE             ThreadHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN HANDLE               ProcessHandle OPTIONAL,
    OUT PCLIENT_ID          ClientId OPTIONAL,
    IN PCONTEXT             ThreadContext,
    IN HANDLE               ThreadStack,
    IN BOOLEAN              CreateSuspended
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
ZwFlushBuffersFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwGetContextThread (
    IN HANDLE       ThreadHandle,
    IN OUT PCONTEXT ThreadContext
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver (
    // "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\<DriverName>"
    IN PUNICODE_STRING RegistryPath
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN PLARGE_INTEGER       ByteOffset,
    IN PLARGE_INTEGER       Length,
    IN PULONG               Key,
    IN BOOLEAN              FailImmediately,
    IN BOOLEAN              ExclusiveLock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeDirectoryFile (
    IN HANDLE               FileHandle,
    IN HANDLE               Event OPTIONAL,
    IN PIO_APC_ROUTINE      ApcRoutine OPTIONAL,
    IN PVOID                ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               Buffer,
    IN ULONG                Length,
    IN ULONG                CompletionFilter,
    IN BOOLEAN              WatchTree
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
ZwOpenFile (
    OUT PHANDLE             FileHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN ULONG                ShareAccess,
    IN ULONG                OpenOptions
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
    IN PEPROCESS    Process,
    IN ACCESS_MASK  DesiredAccess,
    OUT PHANDLE     TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject (
    OUT PHANDLE             SymbolicLinkHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken (
    IN PETHREAD     Thread,
    IN ACCESS_MASK  DesiredAccess,
    IN BOOLEAN      OpenAsSelf,
    OUT PHANDLE     TokenHandle
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
ZwSetEaFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    OUT PVOID               Buffer,
    IN ULONG                Length
);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver (
    // "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\<DriverName>"
    IN PUNICODE_STRING RegistryPath
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN PLARGE_INTEGER       ByteOffset,
    IN PLARGE_INTEGER       Length,
    IN PULONG               Key
);

#ifdef __cplusplus
}
#endif

/* --- BEGIN - REACTOS ONLY --- */

typedef struct _BCB
{
	LIST_ENTRY	CacheSegmentListHead;
	PFILE_OBJECT	FileObject;
	KSPIN_LOCK	BcbLock;
	
} BCB, *PBCB;

#define CACHE_SEGMENT_SIZE (0x1000)

struct _MEMORY_AREA;

typedef struct _CACHE_SEGMENT
{
	PVOID			BaseAddress;
	struct _MEMORY_AREA	* MemoryArea;
	BOOLEAN			Valid;
	LIST_ENTRY		ListEntry;
	ULONG			FileOffset;
	KEVENT			Lock;
	ULONG			ReferenceCount;
	PBCB			Bcb;
	
} CACHE_SEGMENT, *PCACHE_SEGMENT;


NTSTATUS
CcFlushCachePage (
	PCACHE_SEGMENT	CacheSeg
	);

NTSTATUS
CcReleaseCachePage (
	PBCB		Bcb,
	PCACHE_SEGMENT	CacheSeg,
	BOOLEAN		Valid
	);

NTSTATUS
CcRequestCachePage (
	PBCB		Bcb,
	ULONG		FileOffset,
	PVOID		* BaseAddress,
	PBOOLEAN	UptoDate,
	PCACHE_SEGMENT	* CacheSeg
	);

NTSTATUS
CcInitializeFileCache (
	PFILE_OBJECT	FileObject,
	PBCB		* Bcb
	);

NTSTATUS
CcReleaseFileCache (
	PFILE_OBJECT	FileObject,
	PBCB		Bcb
	);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

/* --- END - REACTOS ONLY --- */

#endif // _NTIFS_
