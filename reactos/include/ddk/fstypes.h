#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id: fstypes.h,v 1.15 2004/03/18 16:19:25 weiden Exp $ */

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


typedef VOID
(*POPLOCK_WAIT_COMPLETE_ROUTINE)(PVOID Context,
				 PIRP Irp);

typedef VOID
(*POPLOCK_FS_PREPOST_IRP)(PVOID Context,
			  PIRP Irp);

typedef PVOID OPLOCK, *POPLOCK;

#endif /* __USE_W32API */

#endif /* __INCLUDE_DDK_FSFUNCS_H */
