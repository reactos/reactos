#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id: fstypes.h,v 1.8 2002/11/07 02:44:49 robd Exp $ */

#define FSRTL_TAG 	TAG('F','S','r','t')

typedef struct _FILE_LOCK_INFO {
    LARGE_INTEGER   StartingByte;
    LARGE_INTEGER   Length;
    BOOLEAN         ExclusiveLock;
    ULONG           Key;
    PFILE_OBJECT    FileObject;
    PEPROCESS       Process;
    LARGE_INTEGER   EndingByte;
} FILE_LOCK_INFO, *PFILE_LOCK_INFO;

typedef struct _FILE_LOCK_TOC {
	KSPIN_LOCK			SpinLock;
	LIST_ENTRY			GrantedListHead;
	LIST_ENTRY			PendingListHead;
} FILE_LOCK_TOC, *PFILE_LOCK_TOC;

typedef struct _FILE_LOCK_GRANTED {
	LIST_ENTRY			ListEntry;
	FILE_LOCK_INFO		Lock;
} FILE_LOCK_GRANTED, *PFILE_LOCK_GRANTED;

typedef struct _FILE_LOCK_PENDING {
	LIST_ENTRY			ListEntry;
	PIRP				Irp;
	PVOID				Context;
} FILE_LOCK_PENDING, *PFILE_LOCK_PENDING;

// raw internal file lock struct returned from FsRtlGetNextFileLock
typedef struct _FILE_SHARED_LOCK_ENTRY {
    PVOID           Unknown1;
    PVOID           Unknown2;
    FILE_LOCK_INFO  FileLock;
} FILE_SHARED_LOCK_ENTRY, *PFILE_SHARED_LOCK_ENTRY;

// raw internal file lock struct returned from FsRtlGetNextFileLock
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


typedef struct _LARGE_MCB
{
  PFAST_MUTEX FastMutex;
  ULONG MaximumPairCount;
  ULONG PairCount;
  POOL_TYPE PoolType;
  PVOID Mapping;
} LARGE_MCB, *PLARGE_MCB;


typedef VOID
(*POPLOCK_WAIT_COMPLETE_ROUTINE)(PVOID Context,
				 PIRP Irp);

typedef VOID
(*POPLOCK_FS_PREPOST_IRP)(PVOID Context,
			  PIRP Irp);

typedef PVOID OPLOCK, *POPLOCK;

#endif /* __INCLUDE_DDK_FSFUNCS_H */
