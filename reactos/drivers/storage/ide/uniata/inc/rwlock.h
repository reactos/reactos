#ifndef __CROSS_NT_RWLOCK__H__
#define __CROSS_NT_RWLOCK__H__

#ifndef	MAXIMUM_PROCESSORS
#define	MAXIMUM_PROCESSORS	32
#endif

typedef union _NDIS_RW_LOCK_REFCOUNT {
    unsigned int                 RefCount;
    UCHAR                        cacheLine[16];    // One refCount per cache line
} NDIS_RW_LOCK_REFCOUNT;

typedef struct _NDIS_RW_LOCK {
    union  {
        struct {
            KSPIN_LOCK           SpinLock;
            PVOID                Context;
        };
        UCHAR                    Reserved[16];
    };

    NDIS_RW_LOCK_REFCOUNT        RefCount[MAXIMUM_PROCESSORS];
} NDIS_RW_LOCK, *PNDIS_RW_LOCK;

typedef struct _LOCK_STATE {
    USHORT     LockState;
    KIRQL      OldIrql;
} LOCK_STATE, *PLOCK_STATE;

#define RWLOCK_STATE_FREE             0
#define RWLOCK_STATE_READ_ACQUIRED    1
#define RWLOCK_STATE_WRITE_ACQUIRED   2
#define RWLOCK_STATE_RECURSIVE        3
#define RWLOCK_STATE_RELEASED         0xffff

#define RWLOCK_FOR_WRITE              TRUE
#define RWLOCK_FOR_READ               FALSE

#endif __CROSS_NT_RWLOCK__H__
