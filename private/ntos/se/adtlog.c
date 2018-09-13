/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtlog.c

Abstract:

    Auditing - Audit Record Queuing and Logging Routines

    This file contains functions that construct Audit Records in self-
    relative form from supplied information, enqueue/dequeue them and
    write them to the log.

Author:

    Scott Birrell       (ScottBi)       November 8, 1991

Environment:

    Kernel Mode only

Revision History:

--*/

#include <nt.h>
#include <ntos.h>
#include <zwapi.h>
#include "sep.h"
#include "sertlp.h"
#include "adt.h"
#include "adtp.h"
#include "rmp.h"



#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,SepAdtCopyToLsaSharedMemory)
#pragma alloc_text(PAGE,SepAdtLogAuditRecord)
#pragma alloc_text(PAGE,SepAdtMarshallAuditRecord)
#pragma alloc_text(PAGE,SepAdtSetAuditLogInformation)
#pragma alloc_text(PAGE,SepDequeueWorkItem)
#pragma alloc_text(PAGE,SepQueueWorkItem)

#endif

VOID
SepAdtLogAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    This function manages the logging of Audit Records.  It provides the
    single interface to the Audit Logging component from the Audit/Alarm
    generation routines.  The function constructs an Audit Record in
    self-relative format from the information provided and appends it to
    the Audit Record Queue, a doubly-linked list of Audit Records awaiting
    output to the Audit Log.  A dedicated thread reads this queue, writing
    Audit Records to the Audit Log and removing them from the Audit Queue.

Arguments:

    AuditEventType - Specifies the type of the Audit Event described by
        the audit information provided.

    AuditInformation - Pointer to buffer containing captured auditing
        information related to an Audit Event of type AuditEventType.

Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL - Audit record was not queued
    STATUS_INSUFFICIENT_RESOURCES - unable to allocate heap

--*/

{
    NTSTATUS Status;
    PSEP_LSA_WORK_ITEM AuditWorkItem;

    PAGED_CODE();

    AuditWorkItem = ExAllocatePoolWithTag( PagedPool, sizeof( SEP_LSA_WORK_ITEM ), 'iAeS' );

    if ( AuditWorkItem == NULL ) {

        SepAuditFailed();
        return;
    }

    AuditWorkItem->Tag = SepAuditRecord;
    AuditWorkItem->CommandNumber = LsapWriteAuditMessageCommand;
    AuditWorkItem->ReplyBuffer = NULL;
    AuditWorkItem->ReplyBufferLength = 0;
    AuditWorkItem->CleanupFunction = NULL;

    //
    // Build an Audit record in self-relative format from the supplied
    // Audit Information.
    //

    Status = SepAdtMarshallAuditRecord(
                 AuditParameters,
                 (PSE_ADT_PARAMETER_ARRAY *) &AuditWorkItem->CommandParams.BaseAddress,
                 &AuditWorkItem->CommandParamsMemoryType
                 );

    if (NT_SUCCESS(Status)) {

        //
        // Extract the length of the Audit Record.  Store it as the length
        // of the Command Parameters buffer.
        //

        AuditWorkItem->CommandParamsLength =
            ((PSE_ADT_PARAMETER_ARRAY) AuditWorkItem->CommandParams.BaseAddress)->Length;

        //
        // If we're going to crash on a discarded audit, ignore the queue bounds
        // check and force the item onto the queue.
        //

        if (!SepQueueWorkItem( AuditWorkItem, (BOOLEAN)(SepCrashOnAuditFail ? TRUE : FALSE) )) {

            ExFreePool( AuditWorkItem->CommandParams.BaseAddress );
            ExFreePool( AuditWorkItem );

            //
            // We failed to put the record on the queue.  Take whatever action is
            // appropriate.
            //

            SepAuditFailed();
        }

    } else {

        ExFreePool( AuditWorkItem );
        SepAuditFailed();
    }
}



VOID
SepAuditFailed(
    VOID
    )

/*++

Routine Description:

    Bugchecks the system due to a missed audit (optional requirement
    for C2 compliance).

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UCHAR NewValue;

    ASSERT(sizeof(UCHAR) == sizeof(BOOLEAN));

    if (!SepCrashOnAuditFail) {
        return;
    }

    //
    // Turn off flag in the registry that controls crashing on audit failure
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

    InitializeObjectAttributes( &Obja,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE | 
                                    OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL
                                );
    do {

        Status = ZwOpenKey(
                     &KeyHandle,
                     KEY_SET_VALUE,
                     &Obja
                     );

    } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));

    //
    // If the LSA key isn't there, he's got big problems.  But don't crash.
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        SepCrashOnAuditFail = FALSE;
        return;
    }

    if (!NT_SUCCESS( Status )) {
        goto bugcheck;
    }

    RtlInitUnicodeString( &ValueName, CRASH_ON_AUDIT_FAIL_VALUE );

    NewValue = LSAP_ALLOW_ADIMIN_LOGONS_ONLY;

    do {

        Status = ZwSetValueKey( KeyHandle,
                                &ValueName,
                                0,
                                REG_NONE,
                                &NewValue,
                                sizeof(UCHAR)
                                );

    } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
    ASSERT(NT_SUCCESS(Status));

    if (!NT_SUCCESS( Status )) {
        goto bugcheck;
    }

    do {

        Status = ZwFlushKey( KeyHandle );

    } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
    ASSERT(NT_SUCCESS(Status));

    //
    // go boom.
    //

bugcheck:

    KeBugCheck(AUDIT_FAILURE);
}



NTSTATUS
SepAdtMarshallAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    OUT PSE_ADT_PARAMETER_ARRAY *MarshalledAuditParameters,
    OUT PSEP_RM_LSA_MEMORY_TYPE RecordMemoryType
    )

/*++

Routine Description:

    This routine will take an AuditParamters structure and create
    a new AuditParameters structure that is suitable for sending
    to LSA.  It will be in self-relative form and allocated as
    a single chunk of memory.

Arguments:


    AuditParameters - A filled in set of AuditParameters to be marshalled.

    MarshalledAuditParameters - Returns a pointer to a block of heap memory
        containing the passed AuditParameters in self-relative form suitable
        for passing to LSA.


Return Value:

    None.

--*/

{
    ULONG i;
    ULONG TotalSize = sizeof( SE_ADT_PARAMETER_ARRAY );
    PUNICODE_STRING TargetString;
    PCHAR Base;
    ULONG BaseIncr;

    PAGED_CODE();

    //
    // Calculate the total size required for the passed AuditParameters
    // block.  This calculation will probably be an overestimate of the
    // amount of space needed, because data smaller that 2 dwords will
    // be stored directly in the parameters structure, but their length
    // will be counted here anyway.  The overestimate can't be more than
    // 24 dwords, and will never even approach that amount, so it isn't
    // worth the time it would take to avoid it.
    //

    for (i=0; i<AuditParameters->ParameterCount; i++) {
        TotalSize += (ULONG)LongAlignSize(AuditParameters->Parameters[i].Length);
    }

    //
    // Allocate a big enough block of memory to hold everything.
    // If it fails, quietly abort, since there isn't much else we
    // can do.
    //

    *MarshalledAuditParameters = ExAllocatePoolWithTag( PagedPool, TotalSize, 'pAeS' );

    if (*MarshalledAuditParameters == NULL) {

        *RecordMemoryType = SepRmNoMemory;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    *RecordMemoryType = SepRmPagedPoolMemory;

    RtlCopyMemory (
       *MarshalledAuditParameters,
       AuditParameters,
       sizeof( SE_ADT_PARAMETER_ARRAY )
       );

   (*MarshalledAuditParameters)->Length = TotalSize;
   (*MarshalledAuditParameters)->Flags  = SE_ADT_PARAMETERS_SELF_RELATIVE;

    //
    // Start walking down the list of parameters and marshall them
    // into the target buffer.
    //

    Base = (PCHAR) ((PCHAR)(*MarshalledAuditParameters) + sizeof( SE_ADT_PARAMETER_ARRAY ));

    for (i=0; i<AuditParameters->ParameterCount; i++) {


        switch (AuditParameters->Parameters[i].Type) {
            case SeAdtParmTypeNone:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeNoLogonId:
            case SeAdtParmTypeAccessMask:
                {
                    //
                    // Nothing to do for this
                    //

                    break;

                }
            case SeAdtParmTypeString:
            case SeAdtParmTypeFileSpec:
                {
                    PUNICODE_STRING SourceString;
                    //
                    // We must copy the body of the unicode string
                    // and then copy the body of the string.  Pointers
                    // must be turned into offsets.

                    TargetString = (PUNICODE_STRING)Base;

                    SourceString = AuditParameters->Parameters[i].Address;

                    *TargetString = *SourceString;

                    //
                    // Reset the data pointer in the output parameters to
                    // 'point' to the new string structure.
                    //

                    (*MarshalledAuditParameters)->Parameters[i].Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base += sizeof( UNICODE_STRING );

                    RtlCopyMemory( Base, SourceString->Buffer, SourceString->Length );

                    //
                    // Make the string buffer in the target string point to where we
                    // just copied the data.
                    //

                    TargetString->Buffer = (PWSTR)(Base - (ULONG_PTR)(*MarshalledAuditParameters));

                    BaseIncr = (ULONG)LongAlignSize(SourceString->Length);

                    Base += BaseIncr;

                    break;
                }

            //
            // Handle types where we simply copy the buffer.
            //
            case SeAdtParmTypeSid:
            case SeAdtParmTypePrivs:
            case SeAdtParmTypeObjectTypes:
                {
                    //
                    // Copy the data into the output buffer
                    //

                    RtlCopyMemory( Base,
                                   AuditParameters->Parameters[i].Address,
                                   AuditParameters->Parameters[i].Length );

                    //
                    // Reset the 'address' of the data to be its offset in the
                    // buffer.
                    //

                    (*MarshalledAuditParameters)->Parameters[i].Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base +=  (ULONG)LongAlignSize( AuditParameters->Parameters[i].Length );


                    break;
                }
            default:
                {
                    //
                    // We got passed junk, complain.
                    //

                    ASSERT( FALSE );
                    break;
                }
        }
    }

    return( STATUS_SUCCESS );
}


VOID
SepAdtSetAuditLogInformation(
    IN PPOLICY_AUDIT_LOG_INFO AuditLogInformation
    )

/*++

Routine Description:

    This function stores Audit Log Information in the Reference Monitor's
    in-memory database.  This information contains parameters such as Audit
    Log size etc.  It is the caller's responsibility to ensure that the
    supplied information is valid.

    NOTE:  After initialization, this function is only called by the LSA
    via a Reference Monitor command.  This is a necessary restriction
    because the Audit Log Information stored in the LSA Database must
    remain in sync

Arguments:

    AuditLogInformation - Pointer to Audit Log Information structure.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Acquire Reference Monitor Database write lock.
    //

    SepRmAcquireDbWriteLock();

    //
    // Write the information
    //

    SepAdtLogInformation = *AuditLogInformation;

    //
    // Release Reference Monitor Database write lock
    //

    SepRmReleaseDbWriteLock();
}



NTSTATUS
SepAdtCopyToLsaSharedMemory(
    IN HANDLE LsaProcessHandle,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PVOID *LsaBufferAddress
    )

/*++

Routine Description:

    This function allocates memory shared with the LSA and optionally copies
    a given buffer to it.

Arguments:

    LsaProcessHandle - Specifies a handle to the Lsa Process.

    Buffer - Pointer to the buffer to be copied.

    BufferLength - Length of buffer.

    LsaBufferAddress - Receives the address of the buffer valid in the
        Lsa process context.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes returned by called routines.
--*/

{
    NTSTATUS Status, SecondaryStatus;
    PVOID OutputLsaBufferAddress = NULL;
    SIZE_T RegionSize = BufferLength;

    PAGED_CODE();

    Status = ZwAllocateVirtualMemory(
                 LsaProcessHandle,
                 &OutputLsaBufferAddress,
                 0,
                 &RegionSize,
                 MEM_COMMIT,
                 PAGE_READWRITE
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyToLsaSharedMemoryError;
    }

    Status = ZwWriteVirtualMemory(
                 LsaProcessHandle,
                 OutputLsaBufferAddress,
                 Buffer,
                 BufferLength,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyToLsaSharedMemoryError;
    }

    *LsaBufferAddress = OutputLsaBufferAddress;
    return(Status);

CopyToLsaSharedMemoryError:

    //
    // If we allocated memory, free it.
    //

    if (OutputLsaBufferAddress != NULL) {

        RegionSize = 0;

        SecondaryStatus = ZwFreeVirtualMemory(
                              LsaProcessHandle,
                              &OutputLsaBufferAddress,
                              &RegionSize,
                              MEM_RELEASE
                              );

        ASSERT(NT_SUCCESS(SecondaryStatus));

        OutputLsaBufferAddress = NULL;
    }

    return(Status);
}


BOOLEAN
SepQueueWorkItem(
    IN PSEP_LSA_WORK_ITEM LsaWorkItem,
    IN BOOLEAN ForceQueue
    )

/*++

Routine Description:

    Puts the passed work item on the queue to be passed to LSA,
    and returns the state of the queue upon arrival.

Arguments:

    LsaWorkItem - Pointer to the work item to be queued.

    ForceQueue - Indicate that this item is not to be discarded
        due because of a full queue.

Return Value:

    TRUE - The item was successfully queued.

    FALSE - The item was not queued and must be discarded.

--*/

{
    BOOLEAN rc = TRUE;
    BOOLEAN StartExThread = FALSE ;

    PAGED_CODE();

#if 0

  DbgPrint("Queueing an audit\n");

#endif

    SepLockLsaQueue();

    if (SepAdtDiscardingAudits && !ForceQueue) {

        if (SepAdtCurrentListLength < SepAdtMinListLength) {

            //
            // We need to generate an audit saying how many audits we've
            // discarded.
            //
            // Since we have the mutex protecting the Audit queue, we don't
            // have to worry about anyone coming along and logging an
            // audit.  But *we* can, since a mutex may be acquired recursively.
            //
            // Since we are so protected, turn off the SepAdtDiscardingAudits
            // flag here so that we don't come through this path again.
            //

            SepAdtDiscardingAudits = FALSE;

            SepAdtGenerateDiscardAudit();
#if 0
            DbgPrint("Auditing resumed\n");
#endif

            //
            // We must assume that that worked, so clear the discard count.
            //

            SepAdtCountEventsDiscarded = 0;

            //
            // Our 'audits discarded' audit is now on the queue,
            // continue logging the one we started with.
            //

        } else {

            //
            // We are not yet below our low water mark.  Toss
            // this audit and increment the discard count.
            //

            SepAdtCountEventsDiscarded++;
            rc = FALSE;
            goto Exit;
        }
    }

    if (SepAdtCurrentListLength < SepAdtMaxListLength || ForceQueue) {

        InsertTailList(&SepLsaQueue, &LsaWorkItem->List);

        if (++SepAdtCurrentListLength == 1) {

#if 0
            DbgPrint("Queueing a work item\n");
#endif

            StartExThread = TRUE ;
        }

    } else {

        //
        // There is no room for this audit on the queue,
        // so change our state to 'discarding' and tell
        // the caller to toss this audit.
        //

        SepAdtDiscardingAudits = TRUE;

#if 0
        DbgPrint("Starting to discard audits\n");
#endif

        rc = FALSE;
    }

Exit:

    SepUnlockLsaQueue();

    if ( StartExThread )
    {
        ExInitializeWorkItem( &SepExWorkItem.WorkItem,
                              (PWORKER_THREAD_ROUTINE) SepRmCallLsa,
                              &SepExWorkItem
                              );

        ExQueueWorkItem( &SepExWorkItem.WorkItem, DelayedWorkQueue );
    }

    return( rc );
}



PSEP_LSA_WORK_ITEM
SepDequeueWorkItem(
    VOID
    )

/*++

Routine Description:

    Removes the top element of the SepLsaQueue and returns the
    next element if there is one, NULL otherwise.

Arguments:

    None.

Return Value:

    A pointer to the next SEP_LSA_WORK_ITEM, or NULL.

--*/

{
    PSEP_LSA_WORK_ITEM OldWorkQueueItem;

    PAGED_CODE();

    SepLockLsaQueue();

    OldWorkQueueItem = (PSEP_LSA_WORK_ITEM)RemoveHeadList(&SepLsaQueue);
    OldWorkQueueItem->List.Flink = NULL;
    ExFreePool( OldWorkQueueItem );

    SepAdtCurrentListLength--;

#if 0
        DbgPrint("Removing item\n");
#endif

    if (IsListEmpty( &SepLsaQueue )) {

        SepUnlockLsaQueue();
        return( NULL );
    }

    //
    // We know there's something on the queue now, so we
    // can unlock it.
    //

    SepUnlockLsaQueue();
    return((PSEP_LSA_WORK_ITEM)(&SepLsaQueue)->Flink);
}
