/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/refderef.c
 * PURPOSE:         Manages the referencing and de-referencing of all Objects,
 *                  as well as the Object Fast Reference implementation.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
ObInitializeFastReference(IN PEX_FAST_REF FastRef,
                          PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;
}

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Reference */
    ObReferenceObject(FastRef->Object);

    /* Return the Object */
    return FastRef->Object;
}

VOID
FASTCALL
ObFastDereferenceObject(IN PEX_FAST_REF FastRef,
                        PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Dereference */
    ObDereferenceObject(FastRef->Object);
}

PVOID
FASTCALL
ObFastReplaceObject(IN PEX_FAST_REF FastRef,
                    PVOID Object)
{
    PVOID OldObject = FastRef->Object;

    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;

    /* Do a normal Dereference */
    ObDereferenceObject(OldObject);

    /* Return old Object*/
    return OldObject;
}

/* PUBLIC FUNCTIONS *********************************************************/

ULONG STDCALL
ObGetObjectPointerCount(PVOID Object)
{
    POBJECT_HEADER Header;

    PAGED_CODE();

    ASSERT(Object);
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    return Header->PointerCount;
}

VOID FASTCALL
ObfReferenceObject(IN PVOID Object)
{
    POBJECT_HEADER Header;

    ASSERT(Object);

    Header = OBJECT_TO_OBJECT_HEADER(Object);

    /* No one should be referencing an object once we are deleting it. */
    if (InterlockedIncrement(&Header->PointerCount) == 1 && !(Header->Flags & OB_FLAG_PERMANENT))
    {
        KEBUGCHECK(0);
    }

}

VOID
FASTCALL
ObfDereferenceObject(IN PVOID Object)
{
    POBJECT_HEADER Header;
    LONG NewPointerCount;
    BOOL Permanent;

    ASSERT(Object);

    /* Extract the object header. */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    Permanent = Header->Flags & OB_FLAG_PERMANENT;

    /*
    Drop our reference and get the new count so we can tell if this was the
    last reference.
    */
    NewPointerCount = InterlockedDecrement(&Header->PointerCount);
    DPRINT("ObfDereferenceObject(0x%x)==%d\n", Object, NewPointerCount);
    ASSERT(NewPointerCount >= 0);

    /* Check whether the object can now be deleted. */
    if (NewPointerCount == 0 &&
        !Permanent)
    {
        ObpDeleteObjectDpcLevel(Header, NewPointerCount);
    }
}

NTSTATUS STDCALL
ObReferenceObjectByPointer(IN PVOID Object,
                           IN ACCESS_MASK DesiredAccess,
                           IN POBJECT_TYPE ObjectType,
                           IN KPROCESSOR_MODE AccessMode)
{
    POBJECT_HEADER Header;

    /* NOTE: should be possible to reference an object above APC_LEVEL! */

    DPRINT("ObReferenceObjectByPointer(Object %x, ObjectType %x)\n",
        Object,ObjectType);

    Header = OBJECT_TO_OBJECT_HEADER(Object);

    if (ObjectType != NULL && Header->Type != ObjectType)
    {
        DPRINT("Failed %p (type was %x %wZ) should be %x %wZ\n",
            Header,
            Header->Type,
            &OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(Header->Type))->Name,
            ObjectType,
            &OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(ObjectType))->Name);
        return(STATUS_UNSUCCESSFUL);
    }
    if (Header->Type == PsProcessType)
    {
        DPRINT("Ref p 0x%x PointerCount %d type %x ",
            Object, Header->PointerCount, PsProcessType);
        DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
    }
    if (Header->Type == PsThreadType)
    {
        DPRINT("Deref t 0x%x with PointerCount %d type %x ",
            Object, Header->PointerCount, PsThreadType);
        DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
    }

    if (Header->PointerCount == 0 && !(Header->Flags & OB_FLAG_PERMANENT))
    {
        if (Header->Type == PsProcessType)
        {
            return STATUS_PROCESS_IS_TERMINATING;
        }
        if (Header->Type == PsThreadType)
        {
            return STATUS_THREAD_IS_TERMINATING;
        }
        return(STATUS_UNSUCCESSFUL);
    }

    if (1 == InterlockedIncrement(&Header->PointerCount) && !(Header->Flags & OB_FLAG_PERMANENT))
    {
        KEBUGCHECK(0);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
                        ULONG Attributes,
                        PACCESS_STATE PassedAccessState,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_TYPE ObjectType,
                        KPROCESSOR_MODE AccessMode,
                        PVOID ParseContext,
                        PVOID* ObjectPtr)
{
    PVOID Object = NULL;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT Context;
    AUX_DATA AuxData;
    ACCESS_STATE AccessState;

    /* Capture the name */
    Status = ObpCaptureObjectName(&ObjectName, ObjectPath, AccessMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we didn't get an access state */
    if (!PassedAccessState)
    {
        /* Use our built-in access state */
        PassedAccessState = &AccessState;
        Status = SeCreateAccessState(&AccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     &ObjectType->TypeInfo.GenericMapping);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Find the object */
    Status = ObFindObject(NULL,
                          &ObjectName,
                          Attributes,
                          AccessMode,
                          &Object,
                          ObjectType,
                          &Context,
                          PassedAccessState,
                          NULL,
                          ParseContext,
                          NULL);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* ROS Hack */
    if (Object == NULL)
    {
        *ObjectPtr = NULL;
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    /* Return the object */
    *ObjectPtr = Object;

    /* Free the access state */
    if (PassedAccessState == &AccessState)
    {
        SeDeleteAccessState(PassedAccessState);
    }

Quickie:
    /* Free the captured name if we had one, and return status */
    if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);
    return Status;
}

NTSTATUS STDCALL
ObReferenceObjectByHandle(HANDLE Handle,
                          ACCESS_MASK DesiredAccess,
                          POBJECT_TYPE ObjectType,
                          KPROCESSOR_MODE AccessMode,
                          PVOID* Object,
                          POBJECT_HANDLE_INFORMATION HandleInformation)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    PVOID ObjectBody;
    ACCESS_MASK GrantedAccess;
    ULONG Attributes;
    PEPROCESS CurrentProcess, Process;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    DPRINT("ObReferenceObjectByHandle(Handle %p, DesiredAccess %x, "
        "ObjectType %p, AccessMode %d, Object %p)\n",Handle,DesiredAccess,
        ObjectType,AccessMode,Object);

    if (Handle == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    CurrentProcess = PsGetCurrentProcess();

    /*
    * Handle special handle names
    */
    if (Handle == NtCurrentProcess() &&
        (ObjectType == PsProcessType || ObjectType == NULL))
    {
        ObReferenceObject(CurrentProcess);

        if (HandleInformation != NULL)
        {
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = PROCESS_ALL_ACCESS;
        }

        *Object = CurrentProcess;
        DPRINT("Referencing current process %p\n", CurrentProcess);
        return STATUS_SUCCESS;
    }
    else if (Handle == NtCurrentProcess())
    {
        CHECKPOINT;
        return(STATUS_OBJECT_TYPE_MISMATCH);
    }

    if (Handle == NtCurrentThread() &&
        (ObjectType == PsThreadType || ObjectType == NULL))
    {
        PETHREAD CurrentThread = PsGetCurrentThread();

        ObReferenceObject(CurrentThread);

        if (HandleInformation != NULL)
        {
            HandleInformation->HandleAttributes = 0;
            HandleInformation->GrantedAccess = THREAD_ALL_ACCESS;
        }

        *Object = CurrentThread;
        CHECKPOINT;
        return STATUS_SUCCESS;
    }
    else if (Handle == NtCurrentThread())
    {
        CHECKPOINT;
        return(STATUS_OBJECT_TYPE_MISMATCH);
    }

    /* desire as much access rights as possible */
    if (DesiredAccess & MAXIMUM_ALLOWED)
    {
        DesiredAccess &= ~MAXIMUM_ALLOWED;
        DesiredAccess |= GENERIC_ALL;
    }

    if(ObIsKernelHandle(Handle, AccessMode))
    {
        Process = PsInitialSystemProcess;
        Handle = ObKernelHandleToHandle(Handle);
    }
    else
    {
        Process = CurrentProcess;
    }

    KeEnterCriticalRegion();

    if (Process != CurrentProcess)
    {
        KeStackAttachProcess(&Process->Pcb,
            &ApcState);
        AttachedToProcess = TRUE;
    }

    HandleEntry = ExMapHandleToPointer(Process->ObjectTable,
        Handle);
    if (HandleEntry == NULL)
    {
        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }
        KeLeaveCriticalRegion();
        DPRINT("ExMapHandleToPointer() failed for handle 0x%p\n", Handle);
        return(STATUS_INVALID_HANDLE);
    }

    ObjectHeader = EX_HTE_TO_HDR(HandleEntry);
    ObjectBody = &ObjectHeader->Body;

    DPRINT("locked1: ObjectHeader: 0x%p [HT:0x%p]\n", ObjectHeader, Process->ObjectTable);

    if (ObjectType != NULL && ObjectType != ObjectHeader->Type)
    {
        DPRINT("ObjectType mismatch: %wZ vs %wZ (handle 0x%p)\n", &ObjectType->Name, ObjectHeader->Type ? &ObjectHeader->Type->Name : NULL, Handle);

        ExUnlockHandleTableEntry(Process->ObjectTable,
            HandleEntry);

        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        KeLeaveCriticalRegion();

        return(STATUS_OBJECT_TYPE_MISMATCH);
    }

    /* map the generic access masks if the caller asks for generic access */
    if (DesiredAccess & GENERIC_ACCESS)
    {
        RtlMapGenericMask(&DesiredAccess,
            &OBJECT_TO_OBJECT_HEADER(ObjectBody)->Type->TypeInfo.GenericMapping);
    }

    GrantedAccess = HandleEntry->GrantedAccess;

    /* Unless running as KernelMode, deny access if caller desires more access
    rights than the handle can grant */
    if(AccessMode != KernelMode && (~GrantedAccess & DesiredAccess))
    {
        ExUnlockHandleTableEntry(Process->ObjectTable,
            HandleEntry);

        if (AttachedToProcess)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        KeLeaveCriticalRegion();

        DPRINT1("GrantedAccess: 0x%x, ~GrantedAccess: 0x%x, DesiredAccess: 0x%x, denied: 0x%x\n", GrantedAccess, ~GrantedAccess, DesiredAccess, ~GrantedAccess & DesiredAccess);

        return(STATUS_ACCESS_DENIED);
    }

    ObReferenceObject(ObjectBody);

    Attributes = HandleEntry->ObAttributes & (EX_HANDLE_ENTRY_PROTECTFROMCLOSE |
        EX_HANDLE_ENTRY_INHERITABLE |
        EX_HANDLE_ENTRY_AUDITONCLOSE);

    ExUnlockHandleTableEntry(Process->ObjectTable,
        HandleEntry);

    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
    }

    KeLeaveCriticalRegion();

    if (HandleInformation != NULL)
    {
        HandleInformation->HandleAttributes = Attributes;
        HandleInformation->GrantedAccess = GrantedAccess;
    }

    *Object = ObjectBody;

    return(STATUS_SUCCESS);
}

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID STDCALL
ObDereferenceObject(IN PVOID Object)
{
    ObfDereferenceObject(Object);
}
/* EOF */
