/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/win32k.c
 * PURPOSE:         Executive Win32 subsystem support
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Moved callbacks to win32k and cleanup.
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

POBJECT_TYPE EXPORTED ExWindowStationObjectType = NULL;
POBJECT_TYPE EXPORTED ExDesktopObjectType = NULL;

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
OB_FIND_METHOD ExpWindowStationObjectFind = NULL;
OB_CREATE_METHOD ExpDesktopObjectCreate = NULL;
OB_DELETE_METHOD ExpDesktopObjectDelete = NULL;

/* FUNCTIONS ****************************************************************/

NTSTATUS
STDCALL
ExpWinStaObjectOpen(OB_OPEN_REASON Reason,
                    PVOID ObjectBody,
                    PEPROCESS Process,
                    ULONG HandleCount,
                    ACCESS_MASK GrantedAccess)
{
    /* Call the Registered Callback */
    return ExpWindowStationObjectOpen(Reason,
                                      ObjectBody,
                                      Process,
                                      HandleCount,
                                      GrantedAccess);
}

VOID
STDCALL
ExpWinStaObjectDelete(PVOID DeletedObject)
{
    /* Call the Registered Callback */
    ExpWindowStationObjectDelete(DeletedObject);
}

PVOID
STDCALL
ExpWinStaObjectFind(PVOID WinStaObject,
                    PWSTR Name,
                    ULONG Attributes)
{
    /* Call the Registered Callback */
    return ExpWindowStationObjectFind(WinStaObject,
                                      Name,
                                      Attributes);
}

NTSTATUS
STDCALL
ExpWinStaObjectParse(PVOID Object,
                     PVOID *NextObject,
                     PUNICODE_STRING FullPath,
                     PWSTR *Path,
                     ULONG Attributes)
{
    /* Call the Registered Callback */
    return ExpWindowStationObjectParse(Object,
                                       NextObject,
                                       FullPath,
                                       Path,
                                       Attributes);
}

NTSTATUS
STDCALL
ExpDesktopCreate(PVOID ObjectBody,
                 PVOID Parent,
                 PWSTR RemainingPath,
                 struct _OBJECT_ATTRIBUTES* ObjectAttributes) 
{
    /* Call the Registered Callback */
    return ExpDesktopObjectCreate(ObjectBody,
                                  Parent,
                                  RemainingPath,
                                  ObjectAttributes);
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
ExpWin32kInit(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT("Creating window station  Object Type\n");
  
    /* Create the window station Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"WindowStation");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.OpenProcedure = ExpWinStaObjectOpen;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExWindowStationObjectType);

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.OpenProcedure = NULL;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
   
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExDesktopObjectType);
}

/* EOF */
