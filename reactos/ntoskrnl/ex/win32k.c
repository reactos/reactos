/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/win32k.c
 * PURPOSE:         Executive Win32 Object Support (Desktop/WinStation)
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpWin32kInit)
#endif

/* DATA **********************************************************************/

POBJECT_TYPE ExWindowStationObjectType = NULL;
POBJECT_TYPE ExDesktopObjectType = NULL;

static GENERIC_MAPPING ExpWindowStationMapping = 
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

static GENERIC_MAPPING ExpDesktopMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

OB_OPEN_METHOD ExpWindowStationObjectOpen = NULL;
OB_PARSE_METHOD ExpWindowStationObjectParse = NULL;
OB_DELETE_METHOD ExpWindowStationObjectDelete = NULL;
OB_PARSE_METHOD ExpDesktopObjectParse = NULL;
OB_DELETE_METHOD ExpDesktopObjectDelete = NULL;

/* FUNCTIONS ****************************************************************/

NTSTATUS
STDCALL
ExpWinStaObjectOpen(OB_OPEN_REASON Reason,
                    PEPROCESS Process,
                    PVOID ObjectBody,
                    ACCESS_MASK GrantedAccess,
                    ULONG HandleCount)
{
    /* Call the Registered Callback */
    return ExpWindowStationObjectOpen(Reason,
                                      Process,
                                      ObjectBody,
                                      GrantedAccess,
                                      HandleCount);
}

VOID
STDCALL
ExpWinStaObjectDelete(PVOID DeletedObject)
{
    /* Call the Registered Callback */
    ExpWindowStationObjectDelete(DeletedObject);
}

NTSTATUS
STDCALL
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
    /* Call the Registered Callback */
    return ExpWindowStationObjectParse(ParseObject,
                                       ObjectType,
                                       AccessState,
                                       AccessMode,
                                       Attributes,
                                       CompleteName,
                                       RemainingName,
                                       Context,
                                       SecurityQos,
                                       Object);
}
VOID
STDCALL
ExpDesktopDelete(PVOID DeletedObject)
{
    /* Call the Registered Callback */
    ExpDesktopObjectDelete(DeletedObject);
}

VOID
INIT_FUNCTION
STDCALL
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
    ObjectTypeInitializer.OpenProcedure = ExpWinStaObjectOpen;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObpCreateTypeObject(&ObjectTypeInitializer,
                        &Name,
                        &ExWindowStationObjectType);

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.OpenProcedure = NULL;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExDesktopObjectType);
}

/* EOF */
