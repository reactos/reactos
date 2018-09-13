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



Revision History:

--*/

#define IO_REMOVE_LOCK_SIG     'COLR'

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK {
    struct _IO_REMOVE_LOCK_TRACKING_BLOCK * Link;
    PVOID           Tag;
    LARGE_INTEGER   TimeLocked;
    PCSTR           File;
    ULONG           Line;
} IO_REMOVE_LOCK_TRACKING_BLOCK, *PIO_REMOVE_LOCK_TRACKING_BLOCK;


