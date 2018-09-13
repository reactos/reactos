//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       scavenge.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-30-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SCAVENGE_HXX__
#define __SCAVENGE_HXX__


//
//  Structure used to control scavenger items
//

typedef struct _WL_SCAVENGER_ITEM {
    LIST_ENTRY              List ;
    ULONG                   HandleIndex;    // Index into handle table
    ULONG                   Flags;          // Flags, like spawn a new thread
    ULONG                   Type;           // Type, interval or handle
    ULONG                   RefCount ;      // Reference Count
    LPTHREAD_START_ROUTINE  Function;       // function to call
    PVOID                   Parameter ;     // parameter to pass
    ULONG                   Interval;       // Interval to call, in seconds
    ULONG                   NextTrigger;    // Next Trigger time, in ms
    ULONG                   ScavCheck;      // Quick check to make sure its valid
} WL_SCAVENGER_ITEM, * PWL_SCAVENGER_ITEM ;


//
// Magic values to protect ourselves from mean spirited packages
//

#define SCAVMAGIC_ACTIVE    0x76616353
#define SCAVMAGIC_FREE      0x65657266

//
// The codes:
//

#define NOTIFIER_FLAG_NEW_THREAD    0x00000001
#define NOTIFIER_FLAG_ONE_SHOT      0x00000002
#define NOTIFIER_FLAG_HANDLE_FREE   0x00000004
#define NOTIFIER_FLAG_SECONDS       0x80000000

#define NOTIFIER_TYPE_INTERVAL      1
#define NOTIFIER_TYPE_HANDLE_WAIT   2
#define NOTIFIER_TYPE_STATE_CHANGE  3
#define NOTIFIER_TYPE_NOTIFY_EVENT  4
#define NOTIFIER_TYPE_IMMEDIATE 16



//
// Prototypes:
//

BOOL
WlpInitializeScavenger(
    VOID
    );

PVOID
NTAPI
WlRegisterNotification(
    IN LPTHREAD_START_ROUTINE pFunction,
    IN PVOID pvParameter,
    IN ULONG Type,
    IN ULONG fItem,
    IN ULONG Interval,
    IN HANDLE hEvent
    );

NTSTATUS
NTAPI
WlCancelNotification(
    PVOID       pvScavHandle
    );

#endif
