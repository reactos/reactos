#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id: fstypes.h,v 1.16 2004/06/23 00:41:55 ion Exp $ */

#ifndef __USE_W32API

typedef struct _FILE_LOCK_INFO {
    LARGE_INTEGER   StartingByte;
    LARGE_INTEGER   Length;
    BOOLEAN         ExclusiveLock;
    ULONG           Key;
    PFILE_OBJECT    FileObject;
    PEPROCESS       Process;
    LARGE_INTEGER   EndingByte;
} FILE_LOCK_INFO, *PFILE_LOCK_INFO;

typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
    LARGE_INTEGER  ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef NTSTATUS (*PCOMPLETE_LOCK_IRP_ROUTINE) (
    IN PVOID    Context,
    IN PIRP     Irp
);

typedef VOID (*PUNLOCK_ROUTINE) (
    IN PVOID            Context,
    IN PFILE_LOCK_INFO  FileLockInfo
);

typedef
BOOLEAN (*PFILTER_REPORT_CHANGE) (
            IN PVOID NotifyContext,
            IN PVOID FilterContext
            );

typedef enum _FS_FILTER_SECTION_SYNC_TYPE {
    SyncTypeOther = 0,
    SyncTypeCreateSection
} FS_FILTER_SECTION_SYNC_TYPE, *PFS_FILTER_SECTION_SYNC_TYPE;

typedef union _FS_FILTER_PARAMETERS {

    /*  AcquireForModifiedPageWriter */
    struct {
        PLARGE_INTEGER EndingOffset;
    } AcquireForModifiedPageWriter;

    /*  ReleaseForModifiedPageWriter */
    struct {
        PERESOURCE ResourceToRelease;
    } ReleaseForModifiedPageWriter;

    /*  AcquireForSectionSynchronization */
    struct {
        FS_FILTER_SECTION_SYNC_TYPE SyncType;
        ULONG PageProtection;
    } AcquireForSectionSynchronization;

    /*  Other */
    struct {
        PVOID Argument1;
        PVOID Argument2;
        PVOID Argument3;
        PVOID Argument4;
        PVOID Argument5;
    } Others;

} FS_FILTER_PARAMETERS, *PFS_FILTER_PARAMETERS;

typedef struct _FS_FILTER_CALLBACK_DATA {

    ULONG SizeOfFsFilterCallbackData;
    UCHAR Operation;
    UCHAR Reserved;
    struct _DEVICE_OBJECT *DeviceObject;
    struct _FILE_OBJECT *FileObject;
    FS_FILTER_PARAMETERS Parameters;

} FS_FILTER_CALLBACK_DATA, *PFS_FILTER_CALLBACK_DATA;

typedef
NTSTATUS
(*PFS_FILTER_CALLBACK) (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
    );

typedef
VOID
(*PFS_FILTER_COMPLETION_CALLBACK) (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    );

typedef struct _FS_FILTER_CALLBACKS {

    ULONG SizeOfFsFilterCallbacks;
    ULONG Reserved;
    PFS_FILTER_CALLBACK PreAcquireForSectionSynchronization;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForSectionSynchronization;
    PFS_FILTER_CALLBACK PreReleaseForSectionSynchronization;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForSectionSynchronization;
    PFS_FILTER_CALLBACK PreAcquireForCcFlush;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForCcFlush;
    PFS_FILTER_CALLBACK PreReleaseForCcFlush;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForCcFlush;
    PFS_FILTER_CALLBACK PreAcquireForModifiedPageWriter;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForModifiedPageWriter;
    PFS_FILTER_CALLBACK PreReleaseForModifiedPageWriter;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForModifiedPageWriter;

} FS_FILTER_CALLBACKS, *PFS_FILTER_CALLBACKS;

typedef struct _FILE_LOCK {
    PCOMPLETE_LOCK_IRP_ROUTINE  CompleteLockIrpRoutine;
    PUNLOCK_ROUTINE             UnlockRoutine;
    BOOLEAN                     FastIoIsQuestionable;
    BOOLEAN                     Pad[3];
    PVOID                       LockInformation;
    FILE_LOCK_INFO              LastReturnedLockInfo;
    PVOID                       LastReturnedLock;
} FILE_LOCK, *PFILE_LOCK;


typedef struct _TUNNEL {
    FAST_MUTEX          Mutex;
    PRTL_SPLAY_LINKS    Cache;
    LIST_ENTRY          TimerQueue;
    USHORT              NumEntries;
} TUNNEL, *PTUNNEL;

typedef struct _NOTIFY_SYNC
{
	DWORD	Unknown0;	/* 0x00 */
	DWORD	Unknown1;	/* 0x04 */
	DWORD	Unknown2;	/* 0x08 */
	WORD	Unknown3;	/* 0x0c */
	WORD	Unknown4;	/* 0x0e */
	DWORD	Unknown5;	/* 0x10 */
	DWORD	Unknown6;	/* 0x14 */
	DWORD	Unknown7;	/* 0x18 */
	DWORD	Unknown8;	/* 0x1c */
	DWORD	Unknown9;	/* 0x20 */
	DWORD	Unknown10;	/* 0x24 */
	
} NOTIFY_SYNC, * PNOTIFY_SYNC;


typedef struct _FSRTL_ADVANCED_FCB_HEADER {
    FSRTL_COMMON_FCB_HEADER Header;
    PFAST_MUTEX FastMutex;
    LIST_ENTRY FilterContexts;
} FSRTL_ADVANCED_FCB_HEADER;

typedef FSRTL_ADVANCED_FCB_HEADER *PFSRTL_ADVANCED_FCB_HEADER;

typedef struct _FSRTL_PER_STREAM_CONTEXT {
    LIST_ENTRY Links;
    PVOID OwnerId;
    PVOID InstanceId;
    PFREE_FUNCTION FreeCallback;
} FSRTL_PER_STREAM_CONTEXT, *PFSRTL_PER_STREAM_CONTEXT;

typedef VOID
(*POPLOCK_WAIT_COMPLETE_ROUTINE)(PVOID Context,
				 PIRP Irp);

typedef VOID
(*POPLOCK_FS_PREPOST_IRP)(PVOID Context,
			  PIRP Irp);

typedef PVOID OPLOCK, *POPLOCK;

#endif /* __USE_W32API */

#endif /* __INCLUDE_DDK_FSFUNCS_H */
