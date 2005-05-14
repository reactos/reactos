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

OBJECT_CREATE_ROUTINE ExpWindowStationObjectCreate = NULL;
OBJECT_PARSE_ROUTINE ExpWindowStationObjectParse = NULL;
OBJECT_DELETE_ROUTINE ExpWindowStationObjectDelete = NULL;
OBJECT_FIND_ROUTINE ExpWindowStationObjectFind = NULL;
OBJECT_CREATE_ROUTINE ExpDesktopObjectCreate = NULL;
OBJECT_DELETE_ROUTINE ExpDesktopObjectDelete = NULL;

/* FUNCTIONS ****************************************************************/

NTSTATUS
STDCALL
ExpWinStaObjectCreate(PVOID ObjectBody,
                      PVOID Parent,
                      PWSTR RemainingPath,
                      struct _OBJECT_ATTRIBUTES* ObjectAttributes)
{
    /* Call the Registered Callback */
    return ExpWindowStationObjectCreate(ObjectBody,
                                        Parent,
                                        RemainingPath,
                                        ObjectAttributes);
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
    /* Create window station object type */
    ExWindowStationObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
    ExWindowStationObjectType->Tag = TAG('W', 'I', 'N', 'S');
    ExWindowStationObjectType->TotalObjects = 0;
    ExWindowStationObjectType->TotalHandles = 0;
    ExWindowStationObjectType->PeakObjects = 0;
    ExWindowStationObjectType->PeakHandles = 0;
    ExWindowStationObjectType->PagedPoolCharge = 0;
    ExWindowStationObjectType->NonpagedPoolCharge = sizeof(WINSTATION_OBJECT);
    ExWindowStationObjectType->Mapping = &ExpWindowStationMapping;
    ExWindowStationObjectType->Dump = NULL;
    ExWindowStationObjectType->Open = NULL;
    ExWindowStationObjectType->Close = NULL;
    ExWindowStationObjectType->Delete = ExpWinStaObjectDelete;
    ExWindowStationObjectType->Parse = ExpWinStaObjectParse;
    ExWindowStationObjectType->Security = NULL;
    ExWindowStationObjectType->QueryName = NULL;
    ExWindowStationObjectType->OkayToClose = NULL;
    ExWindowStationObjectType->Create = ExpWinStaObjectCreate;
    ExWindowStationObjectType->DuplicationNotify = NULL;
    RtlInitUnicodeString(&ExWindowStationObjectType->TypeName, L"WindowStation");
    ObpCreateTypeObject(ExWindowStationObjectType);

    /* Create desktop object type */
    ExDesktopObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
    ExDesktopObjectType->Tag = TAG('D', 'E', 'S', 'K');
    ExDesktopObjectType->TotalObjects = 0;
    ExDesktopObjectType->TotalHandles = 0;
    ExDesktopObjectType->PeakObjects = 0;
    ExDesktopObjectType->PeakHandles = 0;
    ExDesktopObjectType->PagedPoolCharge = 0;
    ExDesktopObjectType->NonpagedPoolCharge = sizeof(DESKTOP_OBJECT);
    ExDesktopObjectType->Mapping = &ExpDesktopMapping;
    ExDesktopObjectType->Dump = NULL;
    ExDesktopObjectType->Open = NULL;
    ExDesktopObjectType->Close = NULL;
    ExDesktopObjectType->Delete = ExpDesktopDelete;
    ExDesktopObjectType->Parse = NULL;
    ExDesktopObjectType->Security = NULL;
    ExDesktopObjectType->QueryName = NULL;
    ExDesktopObjectType->OkayToClose = NULL;
    ExDesktopObjectType->Create = ExpDesktopCreate;
    ExDesktopObjectType->DuplicationNotify = NULL;
    RtlInitUnicodeString(&ExDesktopObjectType->TypeName, L"Desktop");
    ObpCreateTypeObject(ExDesktopObjectType);
}

/* EOF */
