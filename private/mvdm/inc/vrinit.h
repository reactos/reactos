/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrinit.h

Abstract:

    Contains function prototypes for Vdm Redir init routines

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Revision History:

    13-Sep-1991 rfirth
        Created

--*/

BOOLEAN
VrInitialized(
    VOID
    );

BOOLEAN
VrInitialize(
    VOID
    );

VOID
VrUninitialize(
    VOID
    );

VOID
VrRaiseInterrupt(
    VOID
    );

VOID
VrDismissInterrupt(
    VOID
    );

VOID
VrQueueCompletionHandler(
    IN VOID (*AsyncDispositionRoutine)(VOID)
    );

VOID
VrHandleAsyncCompletion(
    VOID
    );

VOID
VrCheckPmNetbiosAnr(
    VOID
    );

VOID
VrEoiAndDismissInterrupt(
    VOID
    );
