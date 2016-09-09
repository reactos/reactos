/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/win32k.c
 * PURPOSE:         Executive Win32 Object Support (Desktop/WinStation)
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpWin32kInit)
#endif

typedef struct _WIN32_KERNEL_OBJECT_HEADER
{
    ULONG SessionId;
} WIN32_KERNEL_OBJECT_HEADER, *PWIN32_KERNEL_OBJECT_HEADER;


/* DATA **********************************************************************/

POBJECT_TYPE ExWindowStationObjectType = NULL;
POBJECT_TYPE ExDesktopObjectType = NULL;

GENERIC_MAPPING ExpWindowStationMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

GENERIC_MAPPING ExpDesktopMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

PKWIN32_SESSION_CALLOUT ExpWindowStationObjectParse = NULL;
PKWIN32_SESSION_CALLOUT ExpWindowStationObjectDelete = NULL;
PKWIN32_SESSION_CALLOUT ExpWindowStationObjectOkToClose = NULL;
PKWIN32_SESSION_CALLOUT ExpDesktopObjectOkToClose = NULL;
PKWIN32_SESSION_CALLOUT ExpDesktopObjectDelete = NULL;
PKWIN32_SESSION_CALLOUT ExpDesktopObjectOpen = NULL;
PKWIN32_SESSION_CALLOUT ExpDesktopObjectClose = NULL;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
ExpWin32SessionCallout(
    _In_ PVOID Object,
    _In_ PKWIN32_SESSION_CALLOUT CalloutProcedure,
    _Inout_opt_ PVOID Parameter)
{
    PWIN32_KERNEL_OBJECT_HEADER Win32ObjectHeader;
    PVOID SessionEntry = NULL;
    KAPC_STATE ApcState;
    NTSTATUS Status;

    /* The objects have a common header. And the kernel accesses it!
       Thanks MS for this kind of retarded "design"! */
    Win32ObjectHeader = Object;

    /* Check if we are not already in the correct session */
    if (!PsGetCurrentProcess()->ProcessInSession ||
        (PsGetCurrentProcessSessionId() != Win32ObjectHeader->SessionId))
    {
        /* Get the session from the objects session Id */
        DPRINT("SessionId == %d\n", Win32ObjectHeader->SessionId);
        SessionEntry = MmGetSessionById(Win32ObjectHeader->SessionId);
        if (SessionEntry == NULL)
        {
            /* The requested session does not even exist! */
            ASSERT(FALSE);
            return STATUS_NOT_FOUND;
        }

        /* Attach to the session */
        Status = MmAttachSession(SessionEntry, &ApcState);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not attach to 0x%p, object %p, callout 0x%p\n",
                    SessionEntry,
                    Win32ObjectHeader,
                    CalloutProcedure);

            /* Cleanup and return */
            MmQuitNextSession(SessionEntry);
            ASSERT(FALSE);
            return Status;
        }
    }

    /* Call the callout routine */
    Status = CalloutProcedure(Parameter);

    /* Check if we have a session */
    if (SessionEntry != NULL)
    {
        /* Detach from the session and quit using it */
        MmDetachSession(SessionEntry, &ApcState);
        MmQuitNextSession(SessionEntry);
    }

    /* Return the callback status */
    return Status;
}

BOOLEAN
NTAPI
ExpDesktopOkToClose( IN PEPROCESS Process OPTIONAL,
                     IN PVOID Object,
                     IN HANDLE Handle,
                     IN KPROCESSOR_MODE AccessMode)
{
    WIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters;
    NTSTATUS Status;

    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.Handle = Handle;
    Parameters.PreviousMode = AccessMode;

    Status = ExpWin32SessionCallout(Object,
                                    ExpDesktopObjectOkToClose,
                                    &Parameters);

    return NT_SUCCESS(Status);
}

BOOLEAN
NTAPI
ExpWindowStationOkToClose( IN PEPROCESS Process OPTIONAL,
                     IN PVOID Object,
                     IN HANDLE Handle,
                     IN KPROCESSOR_MODE AccessMode)
{
    WIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters;
    NTSTATUS Status;

    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.Handle = Handle;
    Parameters.PreviousMode = AccessMode;

    Status = ExpWin32SessionCallout(Object,
                                    ExpWindowStationObjectOkToClose,
                                    &Parameters);

    return NT_SUCCESS(Status);
}

VOID
NTAPI
ExpWinStaObjectDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;

    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;

    ExpWin32SessionCallout(DeletedObject,
                           ExpWindowStationObjectDelete,
                           &Parameters);
}

NTSTATUS
NTAPI
ExpWinStaObjectParse(IN PVOID ParseObject,
                     IN PVOID ObjectType,
                     IN OUT PACCESS_STATE AccessState,
                     IN KPROCESSOR_MODE AccessMode,
                     IN ULONG Attributes,
                     IN OUT PUNICODE_STRING CompleteName,
                     IN OUT PUNICODE_STRING RemainingName,
                     IN OUT PVOID Context OPTIONAL,
                     IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                     OUT PVOID *Object)
{
    WIN32_PARSEMETHOD_PARAMETERS Parameters;

    /* Fill out the callback structure */
    Parameters.ParseObject = ParseObject;
    Parameters.ObjectType = ObjectType;
    Parameters.AccessState = AccessState;
    Parameters.AccessMode = AccessMode;
    Parameters.Attributes = Attributes;
    Parameters.CompleteName = CompleteName;
    Parameters.RemainingName = RemainingName;
    Parameters.Context = Context;
    Parameters.SecurityQos = SecurityQos;
    Parameters.Object = Object;

    return ExpWin32SessionCallout(ParseObject,
                                  ExpWindowStationObjectParse,
                                  &Parameters);
}
VOID
NTAPI
ExpDesktopDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;

    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;

    ExpWin32SessionCallout(DeletedObject,
                           ExpDesktopObjectDelete,
                           &Parameters);
}

NTSTATUS
NTAPI
ExpDesktopOpen(IN OB_OPEN_REASON Reason,
               IN PEPROCESS Process OPTIONAL,
               IN PVOID ObjectBody,
               IN ACCESS_MASK GrantedAccess,
               IN ULONG HandleCount)
{
    WIN32_OPENMETHOD_PARAMETERS Parameters;

    Parameters.OpenReason = Reason;
    Parameters.Process = Process;
    Parameters.Object = ObjectBody;
    Parameters.GrantedAccess = GrantedAccess;
    Parameters.HandleCount = HandleCount;

    return ExpWin32SessionCallout(ObjectBody,
                                  ExpDesktopObjectOpen,
                                  &Parameters);
}

VOID
NTAPI
ExpDesktopClose(IN PEPROCESS Process OPTIONAL,
                IN PVOID Object,
                IN ACCESS_MASK GrantedAccess,
                IN ULONG ProcessHandleCount,
                IN ULONG SystemHandleCount)
{
    WIN32_CLOSEMETHOD_PARAMETERS Parameters;

    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.AccessMask = GrantedAccess;
    Parameters.ProcessHandleCount = ProcessHandleCount;
    Parameters.SystemHandleCount = SystemHandleCount;

    ExpWin32SessionCallout(Object,
                           ExpDesktopObjectClose,
                           &Parameters);
}

BOOLEAN
INIT_FUNCTION
NTAPI
ExpWin32kInit(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;
    DPRINT("Creating Win32 Object Types\n");

    /* Create the window station Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"WindowStation");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObjectTypeInitializer.OkayToCloseProcedure = ExpWindowStationOkToClose;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK |
                                              OBJ_PERMANENT |
                                              OBJ_EXCLUSIVE;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_REQUIRED;
    Status = ObCreateObjectType(&Name,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExWindowStationObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObjectTypeInitializer.OkayToCloseProcedure = ExpDesktopOkToClose;
    ObjectTypeInitializer.OpenProcedure = ExpDesktopOpen;
    ObjectTypeInitializer.CloseProcedure = ExpDesktopClose;
    Status = ObCreateObjectType(&Name,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExDesktopObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;

    return TRUE;
}

/* EOF */
