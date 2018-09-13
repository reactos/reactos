/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    remlock.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Peter Wieland
    Kenneth Ray

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#define RTL_REMOVE_LOCK_SIG     'COLR'

#if DBG
typedef struct _RTL_REMOVE_LOCK_TRACKING_BLOCK {
    struct _RTL_REMOVE_LOCK_TRACKING_BLOCK * Link;
    PVOID           Tag;
    LARGE_INTEGER   TimeLocked;
    PCSTR           File;
    ULONG           Line;
} RTL_REMOVE_LOCK_TRACKING_BLOCK, *PRTL_REMOVE_LOCK_TRACKING_BLOCK;
#endif


typedef struct _RTL_REMOVE_LOCK {
    LONG        Signature;
    BOOLEAN     Removed;
    BOOLEAN     Reserved [3];
    LONG        IoCount;
    KEVENT      RemoveEvent;
#if DBG
    LONG        HighWatermark;
    LONG        MaxLockedMinutes;
    LONG        AllocateTag;
    LIST_ENTRY  LockList;
    KSPIN_LOCK  Spin;
    RTL_REMOVE_LOCK_TRACKING_BLOCK Blocks;
#endif
} RTL_REMOVE_LOCK, *PRTL_REMOVE_LOCK;
