#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id: fstypes.h,v 1.4 2002/01/13 22:02:30 ea Exp $ */

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

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

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


#endif /* __INCLUDE_DDK_FSFUNCS_H */
