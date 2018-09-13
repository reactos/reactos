/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rtnotify.c

Abstract:

    NT level registry test program, basic non-error paths.

    Wait for notification.

    This program tests waiting for notification on a change to
    a registry node.  It can wait synchronously, for an event,
    for for an Apc.  It can use any filter.

    Only the first letter of option control are significant.

    rtnotify <keyname> {key|tree|event|Apc|hold|name|write|security|prop|*}

        key = key only [default]  (last of these two wins)
        tree = subtree

        event = wait on an event (overrides hold)
        Apc = use an Apc         (overrides hold)
        hold = be synchronous [default]  (overrides event and Apc)

        name = watch for create/delete of children
        write = last set change
        security = acl change
        prop = any attr == security change
        * = all



    Example:

        rtflush \REGISTRY\MACHINE\TEST\bigkey

Author:

    Bryan Willman (bryanwi)  10-Jan-92

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void main();
void processargs();

ULONG CallCount = 0L;

VOID
ApcTest(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock
    );

UNICODE_STRING  KeyName;
WCHAR           workbuffer[WORK_SIZE];
BOOLEAN         WatchTree;
BOOLEAN         UseEvent;
BOOLEAN         UseApc;
BOOLEAN         ApcSeen;
BOOLEAN         Hold;
BOOLEAN         Filter;
IO_STATUS_BLOCK RtIoStatusBlock;


void
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE          BaseHandle;
    HANDLE          EventHandle;
    PIO_APC_ROUTINE ApcRoutine;

    //
    // Process args
    //

    KeyName.MaximumLength = WORK_SIZE;
    KeyName.Length = 0L;
    KeyName.Buffer = &(workbuffer[0]);

    processargs(argc, argv);

    //
    // Set up and open KeyPath
    //

    printf("rtnotify: starting\n");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtOpenKey(
                &BaseHandle,
                KEY_NOTIFY,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtnotify: t0: %08lx\n", status);
        exit(1);
    }

    EventHandle = (HANDLE)NULL;
    if (UseEvent == TRUE) {
        status = NtCreateEvent(
                    &EventHandle,
                    GENERIC_READ | GENERIC_WRITE  | SYNCHRONIZE,
                    NULL,
                    NotificationEvent,
                    FALSE
                    );
        if (!NT_SUCCESS(status)) {
            printf("rtnotify: t1: %08lx\n", status);
            exit(1);
        }
    }

    ApcRoutine = NULL;
    if (UseApc) {
        ApcRoutine = ApcTest;
    }

    printf("rtnotify:\n");
    printf("\tUseEvent = %08lx\n", UseEvent);
    printf("\tApcRoutine = %08lx\n", ApcRoutine);
    printf("\tHold = %08lx\n", Hold);
    printf("\tFilter = %08lx\n", Filter);
    printf("\tWatchTree = %08lx\n", WatchTree);

    while (TRUE) {
        ApcSeen = FALSE;
        printf("\nCallCount = %dt\n", CallCount);
        CallCount++;
        status = NtNotifyChangeKey(
                    BaseHandle,
                    EventHandle,
                    ApcRoutine,
                    (PVOID)1992,           // arbitrary context value
                    &RtIoStatusBlock,
                    Filter,
                    WatchTree,
                    NULL,
                    0,
                    ! Hold
                    );

        exit(0);

        if ( ! NT_SUCCESS(status)) {
            printf("rtnotify: t2: %08lx\n", status);
            exit(1);
        }

        if (Hold) {
            printf("rtnotify: Synchronous Status = %08lx\n", RtIoStatusBlock.Status);
        }

        if (UseEvent) {
            status = NtWaitForSingleObject(
                        EventHandle,
                        TRUE,
                        NULL
                        );
            if (!NT_SUCCESS(status)) {
                printf("rtnotify: t3: status = %08lx\n", status);
                exit(1);
            }
            printf("rtnotify: Event Status = %08lx\n", RtIoStatusBlock.Status);
        }

        if (UseApc) {
            while ((volatile)ApcSeen == FALSE) {
                NtTestAlert();
            }
        }
    }

    NtClose(BaseHandle);
    exit(0);
}


VOID
ApcTest(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock
    )
{
    ApcSeen = TRUE;

    if (ApcContext != (PVOID)1992) {
        printf("rtnotify: Apc: Apccontext is wrong %08lx\n", ApcContext);
        exit(1);
    }
    if (IoStatusBlock != &RtIoStatusBlock) {
        printf("rtnotify: Apc: IoStatusBlock is wrong %08ln", IoStatusBlock);
        exit(1);
    }


    printf("rtnotify: Apc status = %08lx\n", IoStatusBlock->Status);
    return;
}


void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;
    ULONG   i;

    if (argc < 2) {
        goto Usage;
    }

    //
    // name
    //
    RtlInitAnsiString(
        &temp,
        argv[1]
        );

    RtlAnsiStringToUnicodeString(
        &KeyName,
        &temp,
        FALSE
        );

    WatchTree = FALSE;
    UseEvent = FALSE;
    UseApc = FALSE;
    Hold = TRUE;
    Filter = 0;

    //
    // switches
    //
    for (i = 2; i < (ULONG)argc; i++) {
        switch (*argv[i]) {

        case 'a':   // Apc
        case 'A':
            Hold = FALSE;
            UseApc = TRUE;
            break;

        case 'e':   // event
        case 'E':
            Hold = FALSE;
            UseEvent = TRUE;
            break;

        case 'h':   // hold
        case 'H':
            UseApc = FALSE;
            UseEvent = FALSE;
            Hold = TRUE;
            break;

        case 'k':   // key only
        case 'K':
            WatchTree = FALSE;
            break;

        case 'n':
        case 'N':
            Filter |= REG_NOTIFY_CHANGE_NAME;
            break;

        case 'p':
        case 'P':
            Filter |= REG_NOTIFY_CHANGE_ATTRIBUTES;
            break;

        case 's':
        case 'S':
            Filter |= REG_NOTIFY_CHANGE_SECURITY;
            break;

        case 't':   // subtree
        case 'T':
            WatchTree = TRUE;
            break;

        case 'w':
        case 'W':
            Filter |= REG_NOTIFY_CHANGE_LAST_SET;
            break;

        case '*':
            Filter = REG_LEGAL_CHANGE_FILTER;
            break;

        default:
            goto Usage;
            break;
        }
    }
    if (Filter == 0) {
        Filter = REG_LEGAL_CHANGE_FILTER;
    }
    return;

Usage:
    printf("Usage: %s <KeyPath> {key|tree|event|Apc|sync|name|write|security|attribute|*}\n",
            argv[0]);
    exit(1);
}
