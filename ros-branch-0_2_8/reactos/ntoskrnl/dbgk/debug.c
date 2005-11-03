/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/dbgk/debug.c
 * PURPOSE:         User-Mode Debugging Support, Debug Object Management.
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE DbgkDebugObjectType;
/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreateDebugObject(OUT PHANDLE DebugHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                    IN BOOLEAN KillProcessOnExit)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDBGK_DEBUG_OBJECT DebugObject;
    HANDLE hDebug;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtCreateDebugObject(0x%p, 0x%x, 0x%p)\n", DebugHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(DebugHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            DbgkDebugObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(PDBGK_DEBUG_OBJECT),
                            0,
                            0,
                            (PVOID*)&DebugObject);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

        /* Initialize the Debug Object's Fast Mutex */
        ExInitializeFastMutex(&DebugObject->Mutex);

        /* Initialize the State Event List */
        InitializeListHead(&DebugObject->StateEventListEntry);

        /* Initialize the Debug Object's Wait Event */
        KeInitializeEvent(&DebugObject->Event, NotificationEvent, 0);

        /* Set the Flags */
        DebugObject->KillProcessOnExit = KillProcessOnExit;

        /* Insert it */
        Status = ObInsertObject((PVOID)DebugObject,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hDebug);
        ObDereferenceObject(DebugObject);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status)) {

            _SEH_TRY {

                *DebugHandle = hDebug;

            } _SEH_HANDLE {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS
STDCALL
NtWaitForDebugEvent(IN HANDLE DebugObject, // Debug object handle must grant DEBUG_OBJECT_WAIT_STATE_CHANGE access.
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL,
                    OUT PDBGUI_WAIT_STATE_CHANGE StateChange)
{

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtDebugContinue(IN HANDLE DebugObject,    // Debug object handle must grant DEBUG_OBJECT_WAIT_STATE_CHANGE access.
                IN PCLIENT_ID AppClientId,
                IN NTSTATUS ContinueStatus)
{

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtDebugActiveProcess(IN HANDLE Process,     // Process handle must grant PROCESS_SUSPEND_RESUME access.
                     IN HANDLE DebugObject)  // Debug object handle must grant DEBUG_OBJECT_ADD_REMOVE_PROCESS access.
{

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtRemoveProcessDebug(IN HANDLE Process,     // Process handle must grant PROCESS_SUSPEND_RESUME access.
                     IN HANDLE DebugObject)  // Debug object handle must grant DEBUG_OBJECT_ADD_REMOVE_PROCESS access.
{

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetInformationDebugObject(IN HANDLE DebugObject, // Debug object handle need not grant any particular access right.
                            IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
                            IN PVOID DebugInformation,
                            IN ULONG DebugInformationLength,
                            OUT PULONG ReturnLength OPTIONAL)
{
    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
