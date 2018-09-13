/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    logger.c

Abstract:

    This file contains the code for the debug logging facility.

Author:

    Steve Wood (stevewo) 20-Jun-1992

Environment:

    kernel mode callable only.

Revision History:

    20-Jun-1992 Steve Wood (stevewo) Created.

--*/

#include "exp.h"

PEX_DEBUG_LOG
ExCreateDebugLog(
    IN UCHAR MaximumNumberOfTags,
    IN ULONG MaximumNumberOfEvents
    )
{
    PEX_DEBUG_LOG Log;
    ULONG Size;

    Size = sizeof( EX_DEBUG_LOG ) +
            (MaximumNumberOfTags *
             sizeof( EX_DEBUG_LOG_TAG )
            ) +
            (MaximumNumberOfEvents *
             sizeof( EX_DEBUG_LOG_EVENT )
            );


    Log = ExAllocatePoolWithTag( NonPagedPool, Size, 'oLbD' );
    if (Log != NULL) {
        RtlZeroMemory( Log, Size );
        KeInitializeSpinLock( &Log->Lock );
        Log->MaximumNumberOfTags = MaximumNumberOfTags;
        Log->Tags = (PEX_DEBUG_LOG_TAG)(Log + 1);
        Log->First = (PEX_DEBUG_LOG_EVENT)(Log->Tags + MaximumNumberOfTags);
        Log->Last = Log->First + MaximumNumberOfEvents;
        Log->Next = Log->First;
        }

    return Log;
}

UCHAR
ExCreateDebugLogTag(
    IN PEX_DEBUG_LOG Log,
    IN PCHAR Name,
    IN UCHAR Format1,
    IN UCHAR Format2,
    IN UCHAR Format3,
    IN UCHAR Format4
    )
{
    KIRQL OldIrql;
    ULONG Size;
    PEX_DEBUG_LOG_TAG Tag;
    UCHAR TagIndex;
    PCHAR CapturedName;

    Size = strlen( Name );
    CapturedName = ExAllocatePoolWithTag( NonPagedPool, Size, 'oLbD' );
    RtlMoveMemory( CapturedName, Name, Size + 1 );

    ExAcquireSpinLock( &Log->Lock, &OldIrql );

    if (Log->NumberOfTags < Log->MaximumNumberOfTags) {
        TagIndex = (UCHAR)(Log->NumberOfTags++);
        Tag = &Log->Tags[ TagIndex ];
        Tag->Name = CapturedName;
        Tag->Format[ 0 ] = Format1;
        Tag->Format[ 1 ] = Format2;
        Tag->Format[ 2 ] = Format3;
        Tag->Format[ 3 ] = Format4;
        CapturedName = NULL;
        }
    else {
        TagIndex = (UCHAR)0xFF;
        }

    ExReleaseSpinLock( &Log->Lock, OldIrql );

    if (CapturedName != NULL) {
        ExFreePool( CapturedName );
        }
    return TagIndex;
}

VOID
ExDebugLogEvent(
    IN PEX_DEBUG_LOG Log,
    IN UCHAR Tag,
    IN ULONG Data1,
    IN ULONG Data2,
    IN ULONG Data3,
    IN ULONG Data4
    )
{
    KIRQL OldIrql;
    PEX_DEBUG_LOG_EVENT p;
    PETHREAD Thread = PsGetCurrentThread();
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime( &CurrentTime );

    ExAcquireSpinLock( &Log->Lock, &OldIrql );

    p = Log->Next;
    if (p == Log->Last) {
        p = Log->First;
        }
    Log->Next = p + 1;

    p->ThreadId = Thread->Cid.UniqueThread;
    p->ProcessId = Thread->Cid.UniqueProcess;
    p->Time = CurrentTime.LowPart;
    p->Tag = Tag;
    p->Data[ 0 ] = Data1;
    p->Data[ 1 ] = Data2;
    p->Data[ 2 ] = Data3;
    p->Data[ 3 ] = Data4;

    ExReleaseSpinLock( &Log->Lock, OldIrql );

    return;
}
