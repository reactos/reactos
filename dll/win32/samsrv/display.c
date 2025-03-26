/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/display.c
 * PURPOSE:         Display cache
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "samsrv.h"

#include <winuser.h>

/* GLOBALS *****************************************************************/

typedef struct _USER_ENTRY
{
    LIST_ENTRY ListEntry;
    SAMPR_DOMAIN_DISPLAY_USER User;
} USER_ENTRY, *PUSER_ENTRY;


static LIST_ENTRY UserListHead;
static BOOLEAN UserListFilled = FALSE;
static ULONG UserListCount = 0;

//static LIST_HEAD MachineListHead;
//static LIST_HEAD GroupListHead;



/* FUNCTIONS ***************************************************************/

static
NTSTATUS
SampFillUserDisplayCache(
    _In_ PSAM_DB_OBJECT DomainObject)
{
    HANDLE UsersKeyHandle = NULL;
    WCHAR UserKeyName[64];
    ULONG EnumIndex;
    ULONG NameLength;
    ULONG Rid;
    PSAM_DB_OBJECT UserObject;
    SAM_USER_FIXED_DATA FixedUserData;
    ULONG Size;
    ULONG DataType;
    PUSER_ENTRY UserEntry;
    NTSTATUS Status;

    FIXME("SampFillUserDisplayCache(%p)\n", DomainObject);

    if (UserListFilled)
    {
        FIXME("Already filled!\n");
        return STATUS_SUCCESS;
    }

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Users",
                            KEY_ALL_ACCESS, /* FIXME */
                            &UsersKeyHandle);
    if (!NT_SUCCESS(Status))
        goto done;

    for (EnumIndex = 0; ; EnumIndex++)
    {
        FIXME("EnumIndex: %lu\n", EnumIndex);
        NameLength = 64 * sizeof(WCHAR);
        Status = SampRegEnumerateSubKey(UsersKeyHandle,
                                        EnumIndex,
                                        NameLength,
                                        UserKeyName);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        FIXME("User name: %S\n", UserKeyName);
        FIXME("Name length: %lu\n", NameLength);

        Rid = wcstoul(UserKeyName, NULL, 16);
        if (Rid == 0 || Rid == ULONG_MAX)
            continue;

        FIXME("Rid: 0x%lx\n", Rid);
        Status = SampOpenDbObject(DomainObject,
                                  L"Users",
                                  UserKeyName,
                                  Rid,
                                  SamDbUserObject,
                                  0,
                                  &UserObject);
        if (NT_SUCCESS(Status))
        {
            Size = sizeof(SAM_USER_FIXED_DATA);
            Status = SampGetObjectAttribute(UserObject,
                                            L"F",
                                            &DataType,
                                            (LPVOID)&FixedUserData,
                                            &Size);
            if (NT_SUCCESS(Status))
            {
                FIXME("Account control: 0x%lx\n", FixedUserData.UserAccountControl);

                if (FixedUserData.UserAccountControl & USER_NORMAL_ACCOUNT)
                {
                    UserEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                sizeof(USER_ENTRY));
                    if (UserEntry != NULL)
                    {

                        UserEntry->User.Rid = Rid;
                        UserEntry->User.AccountControl = FixedUserData.UserAccountControl;

                        /* FIXME: Add remaining attributes */

                        InsertTailList(&UserListHead, &UserEntry->ListEntry);
                        UserListCount++;
                    }
                }
            }

            SampCloseDbObject(UserObject);
        }
    }

done:
    if (Status == STATUS_SUCCESS)
        UserListFilled = TRUE;

    FIXME("SampFillUserDisplayCache() done (Status 0x%08lx)\n", Status);

    return Status;
}



NTSTATUS
SampInitializeDisplayCache(VOID)
{
    TRACE("SampInitializeDisplayCache()\n");

    InitializeListHead(&UserListHead);
    UserListFilled = FALSE;
    UserListCount = 0;

//    InitializeListHead(&MachineListHead);
//    MachineListFilled = FALSE;
//    MachineListCount = 0;

//    InitializeListHead(&GroupListHead);
//    GroupListFilled = FALSE;
//    GroupListCount = 0;

    return STATUS_SUCCESS;
}


NTSTATUS
SampShutdownDisplayCache(VOID)
{
    TRACE("SampShutdownDisplayCache()\n");
    return STATUS_SUCCESS;
}


NTSTATUS
SampFillDisplayCache(
    _In_ PSAM_DB_OBJECT DomainObject,
    _In_ DOMAIN_DISPLAY_INFORMATION DisplayInformationClass)
{
    NTSTATUS Status;

    TRACE("SampFillDisplayCache()\n");

    switch (DisplayInformationClass)
    {
        case DomainDisplayUser:
            Status = SampFillUserDisplayCache(DomainObject);
            break;
/*
        case DomainDisplayMachine:
            Status = SampFillMachineDisplayCache(DomainObject);
            break;

        case DomainDisplayGroup:
            Status = SampFillGroupDisplayCache(DomainObject);
            break;
*/
        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}

/* EOF */
