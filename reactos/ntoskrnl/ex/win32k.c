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

static GENERIC_MAPPING ExpWindowStationMapping = {

    STANDARD_RIGHTS_READ     | WINSTA_ENUMDESKTOPS      | WINSTA_ENUMERATE         | WINSTA_READATTRIBUTES | WINSTA_READSCREEN,
    STANDARD_RIGHTS_WRITE    | WINSTA_ACCESSCLIPBOARD   | WINSTA_CREATEDESKTOP     | WINSTA_WRITEATTRIBUTES,
    STANDARD_RIGHTS_EXECUTE  | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS,
    STANDARD_RIGHTS_REQUIRED | WINSTA_ACCESSCLIPBOARD   | WINSTA_ACCESSGLOBALATOMS | WINSTA_CREATEDESKTOP  |
                               WINSTA_ENUMDESKTOPS      | WINSTA_ENUMERATE         | WINSTA_EXITWINDOWS    |
                               WINSTA_READATTRIBUTES    | WINSTA_READSCREEN        | WINSTA_WRITEATTRIBUTES
};

static GENERIC_MAPPING ExpDesktopMapping = {

    STANDARD_RIGHTS_READ     | DESKTOP_ENUMERATE       | DESKTOP_READOBJECTS,
    STANDARD_RIGHTS_WRITE    | DESKTOP_CREATEMENU      | DESKTOP_CREATEWINDOW    | DESKTOP_HOOKCONTROL   |
                               DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD   | DESKTOP_WRITEOBJECTS,
    STANDARD_RIGHTS_EXECUTE  | DESKTOP_SWITCHDESKTOP,
    STANDARD_RIGHTS_REQUIRED | DESKTOP_CREATEMENU      | DESKTOP_CREATEWINDOW    | DESKTOP_ENUMERATE     |
                               DESKTOP_HOOKCONTROL     | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD |
                               DESKTOP_READOBJECTS     | DESKTOP_SWITCHDESKTOP   | DESKTOP_WRITEOBJECTS
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
ExpWinStaObjectFind(PWINSTATION_OBJECT WinStaObject,
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

    DPRINT1("Creating window station  Object Type\n");
  
    /* Create the window station Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"WindowStation");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(WINSTATION_OBJECT);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.OpenProcedure = ExpWinStaObjectOpen;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExWindowStationObjectType);

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DESKTOP_OBJECT);
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.OpenProcedure = NULL;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
   
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExDesktopObjectType);
}

/* EOF */
