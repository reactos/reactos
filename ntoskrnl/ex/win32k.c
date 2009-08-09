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

PKWIN32_PARSEMETHOD_CALLOUT ExpWindowStationObjectParse = NULL;
PKWIN32_DELETEMETHOD_CALLOUT ExpWindowStationObjectDelete = NULL;
PKWIN32_DELETEMETHOD_CALLOUT ExpDesktopObjectDelete = NULL;

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
ExpWinStaObjectDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;

    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;

    /* Call the Registered Callback */
    ExpWindowStationObjectDelete(&Parameters);
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

    /* Call the Registered Callback */
    return ExpWindowStationObjectParse(&Parameters);
}
VOID
NTAPI
ExpDesktopDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;

    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;

    /* Call the Registered Callback */
    ExpDesktopObjectDelete(&Parameters);
}

VOID
INIT_FUNCTION
NTAPI
ExpWin32kInit(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    DPRINT("Creating Win32 Object Types\n");

    /* Create the window station Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"WindowStation");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &ExWindowStationObjectType);

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &ExDesktopObjectType);
}

/* EOF */
