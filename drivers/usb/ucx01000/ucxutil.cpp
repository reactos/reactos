/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver wide state, object creation and verifier fault injection
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

UcxDriverState UcxDriver;

/* The parent and execution constraints always come from UCX */
_Must_inspect_result_
NTSTATUS
NTAPI
UcxCreateObjectWithTwoContexts(
    _In_ PWDF_OBJECT_ATTRIBUTES Primary,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Secondary,
    _Out_ WDFOBJECT* Object)
{
    WDF_OBJECT_ATTRIBUTES Extra;
    WDFOBJECT Created;
    NTSTATUS Status;

    *Object = NULL;

    Status = WdfObjectCreate(Primary, &Created);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Object create failed 0x%lx\n", Status);
        return Status;
    }

    if (Secondary != NULL && Secondary->ContextTypeInfo != NULL)
    {
        Extra = *Secondary;
        Extra.ParentObject = NULL;
        Extra.ExecutionLevel = WdfExecutionLevelInheritFromParent;
        Extra.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;

        Status = WdfObjectAllocateContext(Created, &Extra, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Client context on object %p failed 0x%lx\n", Created, Status);
            WdfObjectDelete(Created);
            return Status;
        }
    }

    *Object = Created;
    return STATUS_SUCCESS;
}

/* Verifier fault injection; races on the seed only make it more random */

ULONG
NTAPI
UcxRandom(VOID)
{
    UcxDriver.RandomSeed = UcxDriver.RandomSeed * 1103515245 + 12345;

    return (UcxDriver.RandomSeed / 65536) % 32768;
}

/** 0 never fails, 1 always fails, N fails about once in N calls. */
BOOLEAN
NTAPI
UcxVerifierWantsFailure(
    _In_ ULONG Setting)
{
    if (Setting == 0)
        return FALSE;

    if (Setting == 1)
        return TRUE;

    return (32768 / Setting) > UcxRandom();
}

ULONG
NTAPI
UcxRandomInRange(
    _In_ ULONG Max)
{
    return (UcxRandom() % Max) + 1;
}

NTSTATUS
NTAPI
UcxRandomErrorStatus(VOID)
{
    return (NTSTATUS)(0xC0000000 | UcxRandom());
}
