/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmboot.c

Abstract:

    This provides routines for determining driver load lists from the
    registry.  The relevant drivers are extracted from the registry,
    sorted by groups, and then dependencies are resolved.

    This module is used both by the OS Loader for determining the boot
    driver list (CmScanRegistry) and by IoInitSystem for determining
    the drivers to be loaded in Phase 1 Initialization
    (CmGetSystemDriverList)

Author:

    John Vert (jvert) 7-Apr-1992

Environment:

    OS Loader environment
        or
    kernel mode

Revision History:

--*/
#include "cmp.h"
#include <profiles.h>

#define LOAD_LAST 0xffffffff
#define LOAD_NEXT_TO_LAST (LOAD_LAST-1)

//
// Private function prototypes.
//
BOOLEAN
CmpAddDriverToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX DriverCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING RegistryPath,
    IN PLIST_ENTRY BootDriverListHead
    );

BOOLEAN
CmpDoSort(
    IN PLIST_ENTRY DriverListHead,
    IN PUNICODE_STRING OrderList
    );

ULONG
CmpFindTagIndex(
    IN PHHIVE Hive,
    IN HCELL_INDEX TagCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING GroupName
    );

BOOLEAN
CmpIsLoadType(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN SERVICE_LOAD_TYPE LoadType
    );

BOOLEAN
CmpOrderGroup(
    IN PBOOT_DRIVER_NODE GroupStart,
    IN PBOOT_DRIVER_NODE GroupEnd
    );

#define CmpValueToData(Hive,Value,realsize)              \
    (CmpIsHKeyValueSmall(realsize,Value->DataLength) ?          \
     ((struct _CELL_DATA *)(&Value->Data)) : HvGetCell(Hive,Value->Data))

VOID
BlPrint(
    PCHAR cp,
    ...
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpFindNLSData)
#pragma alloc_text(INIT,CmpFindDrivers)
#pragma alloc_text(INIT,CmpIsLoadType)
#pragma alloc_text(INIT,CmpAddDriverToList)
#pragma alloc_text(INIT,CmpSortDriverList)
#pragma alloc_text(INIT,CmpDoSort)
#pragma alloc_text(INIT,CmpResolveDriverDependencies)
#pragma alloc_text(INIT,CmpSetCurrentProfile)
#pragma alloc_text(INIT,CmpOrderGroup)
#pragma alloc_text(INIT,CmpFindControlSet)
#pragma alloc_text(INIT,CmpFindTagIndex)
#pragma alloc_text(INIT,CmpFindProfileOption)
#ifdef _WANT_MACHINE_IDENTIFICATION
#pragma alloc_text(INIT,CmpGetBiosDateFromRegistry)
#endif
#endif


BOOLEAN
CmpFindNLSData(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING AnsiFilename,
    OUT PUNICODE_STRING OemFilename,
    OUT PUNICODE_STRING CaseTableFilename,
    OUT PUNICODE_STRING OemHalFont
    )

/*++

Routine Description:

    Traverses a particular control set and determines the filenames for
    the NLS data files that need to be loaded.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    AnsiFileName - Returns the name of the Ansi codepage file (c_1252.nls)

    OemFileName -  Returns the name of the OEM codepage file  (c_437.nls)

    CaseTableFileName - Returns the name of the Unicode upper/lowercase
            table for the language (l_intl.nls)

    OemHalfont - Returns the name of the font file to be used by the HAL.

Return Value:

    TRUE - filenames successfully determined

    FALSE - hive is corrupt

--*/

{
    UNICODE_STRING Name;
    HCELL_INDEX Control;
    HCELL_INDEX Nls;
    HCELL_INDEX CodePage;
    HCELL_INDEX Language;
    HCELL_INDEX ValueCell;
    PHCELL_INDEX Index;
    PCM_KEY_VALUE Value;
    NTSTATUS Status;
    ULONG realsize;

    //
    // Find CONTROL node
    //
    RtlInitUnicodeString(&Name, L"Control");
    Control = CmpFindSubKeyByName(Hive,
                                 (PCM_KEY_NODE)HvGetCell(Hive,ControlSet),
                                 &Name);
    if (Control == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find NLS node
    //
    RtlInitUnicodeString(&Name, L"NLS");
    Nls = CmpFindSubKeyByName(Hive,
                             (PCM_KEY_NODE)HvGetCell(Hive,Control),
                             &Name);
    if (Nls == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find CodePage node
    //
    RtlInitUnicodeString(&Name, L"CodePage");
    CodePage = CmpFindSubKeyByName(Hive,
                                  (PCM_KEY_NODE)HvGetCell(Hive,Nls),
                                  &Name);
    if (CodePage == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find ACP value
    //
    RtlInitUnicodeString(&Name, L"ACP");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,CodePage),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    Name.Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
    Name.MaximumLength=(USHORT)realsize;
    Name.Length = 0;
    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length += sizeof(WCHAR);
    }

    //
    // Find ACP filename
    //
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,CodePage),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    AnsiFilename->Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
    AnsiFilename->Length = AnsiFilename->MaximumLength = (USHORT)realsize;

    //
    // Find OEMCP node
    //
    RtlInitUnicodeString(&Name, L"OEMCP");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,CodePage),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    Name.Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
    Name.MaximumLength = (USHORT)realsize;
    Name.Length = 0;
    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length += sizeof(WCHAR);
    }

    //
    // Find OEMCP filename
    //
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,CodePage),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    OemFilename->Buffer = (PWSTR)CmpValueToData(Hive, Value,realsize);
    OemFilename->Length = OemFilename->MaximumLength = (USHORT)realsize;

    //
    // Find Language node
    //
    RtlInitUnicodeString(&Name, L"Language");
    Language = CmpFindSubKeyByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,Nls),
                                   &Name);
    if (Language == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find Default value
    //
    RtlInitUnicodeString(&Name, L"Default");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,Language),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
            return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    Name.Buffer = (PWSTR)CmpValueToData(Hive, Value,realsize);
    Name.MaximumLength = (USHORT)realsize;
    Name.Length = 0;

    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length+=sizeof(WCHAR);
    }

    //
    // Find default filename
    //
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,Language),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    CaseTableFilename->Buffer = (PWSTR)CmpValueToData(Hive, Value,realsize);
    CaseTableFilename->Length = CaseTableFilename->MaximumLength = (USHORT)realsize;

    //
    // Find OEMHAL filename
    //
    RtlInitUnicodeString(&Name, L"OEMHAL");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,CodePage),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
#ifdef i386
        OemHalFont->Buffer = NULL;
        OemHalFont->Length = 0;
        OemHalFont->MaximumLength = 0;
        return TRUE;
#endif
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    OemHalFont->Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
    OemHalFont->Length = (USHORT)realsize;
    OemHalFont->MaximumLength = (USHORT)realsize;

    return(TRUE);
}


BOOLEAN
CmpFindDrivers(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN SERVICE_LOAD_TYPE LoadType,
    IN PWSTR BootFileSystem OPTIONAL,
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    Traverses a particular control set and creates a list of boot drivers
    to be loaded.  This list is unordered, but complete.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    LoadType - Supplies the type of drivers to be loaded (BootLoad,
            SystemLoad, AutoLoad, etc)

    BootFileSystem - If present, supplies the base name of the boot
        filesystem, which is explicitly added to the driver list.

    DriverListHead - Supplies a pointer to the head of the (empty) list
            of boot drivers to load.

Return Value:

    TRUE - List successfully created.

    FALSE - Hive is corrupt.

--*/

{
    HCELL_INDEX Services;
    HCELL_INDEX Control;
    HCELL_INDEX GroupOrder;
    HCELL_INDEX DriverCell;
    UNICODE_STRING Name;
    PHCELL_INDEX Index;
    int i;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING BasePath;
    WCHAR BaseBuffer[128];
    PBOOT_DRIVER_NODE BootFileSystemNode;
    PCM_KEY_NODE ControlNode;
    PCM_KEY_NODE ServicesNode;

    //
    // Find SERVICES node.
    //
    ControlNode = (PCM_KEY_NODE)HvGetCell(Hive,ControlSet);
    RtlInitUnicodeString(&Name, L"Services");
    Services = CmpFindSubKeyByName(Hive,
                                   ControlNode,
                                   &Name);
    if (Services == HCELL_NIL) {
        return(FALSE);
    }
    ServicesNode = (PCM_KEY_NODE)HvGetCell(Hive,Services);

    //
    // Find CONTROL node.
    //
    RtlInitUnicodeString(&Name, L"Control");
    Control = CmpFindSubKeyByName(Hive,
                                  ControlNode,
                                  &Name);
    if (Control == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find GroupOrderList node.
    //
    RtlInitUnicodeString(&Name, L"GroupOrderList");
    GroupOrder = CmpFindSubKeyByName(Hive,
                                     (PCM_KEY_NODE)HvGetCell(Hive,Control),
                                     &Name);
    if (GroupOrder == HCELL_NIL) {
        return(FALSE);
    }

    BasePath.Length = 0;
    BasePath.MaximumLength = sizeof(BaseBuffer);
    BasePath.Buffer = BaseBuffer;
    RtlAppendUnicodeToString(&BasePath, L"\\Registry\\Machine\\System\\");
    RtlAppendUnicodeToString(&BasePath, L"CurrentControlSet\\Services\\");

    i=0;
    do {
        DriverCell = CmpFindSubKeyByNumber(Hive,ServicesNode,i++);
        if (DriverCell != HCELL_NIL) {
            if (CmpIsLoadType(Hive, DriverCell, LoadType)) {
                CmpAddDriverToList(Hive,
                                   DriverCell,
                                   GroupOrder,
                                   &BasePath,
                                   DriverListHead);

            }
        }
    } while ( DriverCell != HCELL_NIL );

    if (ARGUMENT_PRESENT(BootFileSystem)) {
        //
        // Add boot filesystem to boot driver list
        //

        RtlInitUnicodeString(&UnicodeString, BootFileSystem);
        DriverCell = CmpFindSubKeyByName(Hive,
                                         ServicesNode,
                                         &UnicodeString);
        if (DriverCell != HCELL_NIL) {
            CmpAddDriverToList(Hive,
                               DriverCell,
                               GroupOrder,
                               &BasePath,
                               DriverListHead);

            //
            // mark the Boot Filesystem critical
            //
            BootFileSystemNode = CONTAINING_RECORD(DriverListHead->Flink,
                                                   BOOT_DRIVER_NODE,
                                                   ListEntry.Link);
            BootFileSystemNode->ErrorControl = SERVICE_ERROR_CRITICAL;
        }
    }
    return(TRUE);

}


BOOLEAN
CmpIsLoadType(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN SERVICE_LOAD_TYPE LoadType
    )

/*++

Routine Description:

    Determines if the driver is of a specified LoadType, based on its
    node values.

Arguments:

    Hive - Supplies a pointer to the hive control structure for the system
           hive.

    Cell - Supplies the cell index of the driver's node in the system hive.

    LoadType - Supplies the type of drivers to be loaded (BootLoad,
            SystemLoad, AutoLoad, etc)

Return Value:

    TRUE - Driver is the correct type and should be loaded.

    FALSE - Driver is not the correct type and should not be loaded.

--*/

{
    HCELL_INDEX ValueCell;
    PLONG Data;
    PHCELL_INDEX Index;
    UNICODE_STRING Name;
    PCM_KEY_VALUE Value;
    NTSTATUS Status;
    ULONG realsize;

    //
    // Must have a Start=BootLoad value in order to be a boot driver, so
    // look for that first.
    //
    RtlInitUnicodeString(&Name, L"Start");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,Cell),
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);

    Data = (PLONG)CmpValueToData(Hive,Value,realsize);

    if (*Data != LoadType) {
        return(FALSE);
    }

    return(TRUE);
}


BOOLEAN
CmpAddDriverToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX DriverCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING RegistryPath,
    IN PLIST_ENTRY BootDriverListHead
    )

/*++

Routine Description:

    This routine allocates a list entry node for a particular driver.
    It initializes it with the registry path, filename, group name, and
    dependency list.  Finally, it inserts the new node into the boot
    driver list.

    Note that this routine allocates memory by calling the Hive's
    memory allocation procedure.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    DriverCell - Supplies the HCELL_INDEX of the driver's node in the hive.

    GroupOrderCell - Supplies the HCELL_INDEX of the GroupOrderList key.
        ( \Registry\Machine\System\CurrentControlSet\Control\GroupOrderList )

    RegistryPath - Supplies the full registry path to the SERVICES node
            of the current control set.

    BootDriverListHead - Supplies the head of the boot driver list

Return Value:

    TRUE - Driver successfully added to boot driver list.

    FALSE - Could not add driver to boot driver list.

--*/

{
    PCM_KEY_NODE Driver;
    USHORT DriverNameLength;
    PCM_KEY_VALUE Value;
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    HCELL_INDEX ValueCell;
    HCELL_INDEX Tag;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING FileName;
    NTSTATUS Status;
    ULONG Length;
    PHCELL_INDEX Index;
    ULONG   realsize;

    Driver = (PCM_KEY_NODE)HvGetCell(Hive, DriverCell);
    DriverNode = (Hive->Allocate)(sizeof(BOOT_DRIVER_NODE),FALSE);
    if (DriverNode == NULL) {
        return(FALSE);
    }
    if (Driver->Flags & KEY_COMP_NAME) {
        DriverNode->Name.Length = CmpCompressedNameSize(Driver->Name,Driver->NameLength);
        DriverNode->Name.Buffer = (Hive->Allocate)(DriverNode->Name.Length, FALSE);
        if (DriverNode->Name.Buffer == NULL) {
            return(FALSE);
        }
        CmpCopyCompressedName(DriverNode->Name.Buffer,
                              DriverNode->Name.Length,
                              Driver->Name,
                              Driver->NameLength);

    } else {
        DriverNode->Name.Length = Driver->NameLength;
        DriverNode->Name.Buffer = Driver->Name;
    }
    DriverNode->Name.MaximumLength = DriverNode->Name.Length;
    DriverNameLength = DriverNode->Name.Length;

    DriverEntry = &DriverNode->ListEntry;

    //
    // Check for ImagePath value, which will override the default name
    // if it is present.
    //
    RtlInitUnicodeString(&UnicodeString, L"ImagePath");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,DriverCell),
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {

        //
        // No ImagePath, so generate default filename.
        // Build up Unicode filename  ("system32\drivers\<nodename>.sys");
        //

        Length = sizeof(L"System32\\Drivers\\") +
                 DriverNameLength  +
                 sizeof(L".sys");

        FileName = &DriverEntry->FilePath;
        FileName->Length = 0;
        FileName->MaximumLength = (USHORT)Length;
        FileName->Buffer = (PWSTR)(Hive->Allocate)(Length, FALSE);
        if (FileName->Buffer == NULL) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L"System32\\"))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L"Drivers\\"))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(
                RtlAppendUnicodeStringToString(FileName,
                                               &DriverNode->Name))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L".sys"))) {
            return(FALSE);
        }

    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive,ValueCell);
        FileName = &DriverEntry->FilePath;
        FileName->Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
        FileName->MaximumLength = FileName->Length = (USHORT)realsize;
    }

    FileName = &DriverEntry->RegistryPath;
    FileName->Length = 0;
    FileName->MaximumLength = RegistryPath->Length + DriverNameLength;
    FileName->Buffer = (Hive->Allocate)(FileName->MaximumLength,FALSE);
    if (FileName->Buffer == NULL) {
        return(FALSE);
    }
    RtlAppendUnicodeStringToString(FileName, RegistryPath);
    RtlAppendUnicodeStringToString(FileName, &DriverNode->Name);

    InsertHeadList(BootDriverListHead, &DriverEntry->Link);

    //
    // Find "ErrorControl" value
    //

    RtlInitUnicodeString(&UnicodeString, L"ErrorControl");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,DriverCell),
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {
        DriverNode->ErrorControl = NormalError;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        DriverNode->ErrorControl = *(PULONG)CmpValueToData(Hive,Value,realsize);
    }

    //
    // Find "Group" value
    //
    RtlInitUnicodeString(&UnicodeString, L"group");
    ValueCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,DriverCell),
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {
        DriverNode->Group.Length = 0;
        DriverNode->Group.MaximumLength = 0;
        DriverNode->Group.Buffer = NULL;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        DriverNode->Group.Buffer = (PWSTR)CmpValueToData(Hive,Value,realsize);
        DriverNode->Group.Length = (USHORT)realsize - sizeof(WCHAR);
        DriverNode->Group.MaximumLength = (USHORT)DriverNode->Group.Length;
    }

    //
    // Calculate the tag value for the driver.  If the driver has no tag,
    // this defaults to 0xffffffff, so the driver is loaded last in the
    // group.
    //
    RtlInitUnicodeString(&UnicodeString, L"Tag");
    Tag = CmpFindValueByName(Hive,
                             (PCM_KEY_NODE)HvGetCell(Hive,DriverCell),
                             &UnicodeString);
    if (Tag == HCELL_NIL) {
        DriverNode->Tag = LOAD_LAST;
    } else {
        //
        // Now we have to find this tag in the tag list for the group.
        // If the tag is not in the tag list, then it defaults to 0xfffffffe,
        // so it is loaded after all the drivers in the tag list, but before
        // all the drivers without tags at all.
        //

        DriverNode->Tag = CmpFindTagIndex(Hive,
                                          Tag,
                                          GroupOrderCell,
                                          &DriverNode->Group);
    }

    return(TRUE);

}


BOOLEAN
CmpSortDriverList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    Sorts the list of boot drivers by their groups based on the group
    ordering in <control_set>\CONTROL\SERVICE_GROUP_ORDER:list

    Does NOT do dependency ordering.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

Return Value:

    TRUE - List successfully sorted

    FALSE - List is inconsistent and could not be sorted.

--*/

{
    HCELL_INDEX Controls;
    HCELL_INDEX GroupOrder;
    HCELL_INDEX ListCell;
    UNICODE_STRING Name;
    UNICODE_STRING DependList;
    PHCELL_INDEX Index;
    NTSTATUS Status;
    PCM_KEY_VALUE ListNode;
    ULONG realsize;

    //
    // Find "CONTROL" node.
    //
    RtlInitUnicodeString(&Name, L"Control");
    Controls = CmpFindSubKeyByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,ControlSet),
                                   &Name);
    if (Controls == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find "SERVICE_GROUP_ORDER" subkey
    //
    RtlInitUnicodeString(&Name, L"ServiceGroupOrder");
    GroupOrder = CmpFindSubKeyByName(Hive,
                                     (PCM_KEY_NODE)HvGetCell(Hive,Controls),
                                     &Name);
    if (GroupOrder == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find "list" value
    //
    RtlInitUnicodeString(&Name, L"list");
    ListCell = CmpFindValueByName(Hive,
                                  (PCM_KEY_NODE)HvGetCell(Hive,GroupOrder),
                                  &Name);
    if (ListCell == HCELL_NIL) {
        return(FALSE);
    }
    ListNode = (PCM_KEY_VALUE)HvGetCell(Hive, ListCell);
    if (ListNode->Type != REG_MULTI_SZ) {
        return(FALSE);
    }

    DependList.Buffer = (PWSTR)CmpValueToData(Hive,ListNode,realsize);
    DependList.Length = DependList.MaximumLength = (USHORT)realsize - sizeof(WCHAR);

    //
    // Dependency list is now pointed to by DependList->Buffer.  We need
    // to sort the driver entry list.
    //

    return (CmpDoSort(DriverListHead, &DependList));

}

BOOLEAN
CmpDoSort(
    IN PLIST_ENTRY DriverListHead,
    IN PUNICODE_STRING OrderList
    )

/*++

Routine Description:

    Sorts the boot driver list based on the order list

    Start with the last entry in the group order list and work towards
    the beginning.  For each group entry, move all driver entries that
    are members of the group to the front of the list.  Driver entries
    with no groups, or with a group that does not match any in the
    group list will be shoved to the end of the list.

Arguments:

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

    OrderList - Supplies pointer to the order list

Return Value:

    TRUE - List successfully ordered

    FALSE - List is inconsistent and could not be ordered.

--*/

{
    PWSTR Current;
    PWSTR End;
    PLIST_ENTRY Next;
    PBOOT_DRIVER_NODE CurrentNode;
    UNICODE_STRING CurrentGroup;


    Current = (PWSTR) ((PUCHAR)(OrderList->Buffer)+OrderList->Length);

    while (Current > OrderList->Buffer) {
        do {
            if (*(Current) == UNICODE_NULL) {
                End = Current;
            }
            --Current;
        } while ((*(Current-1) != UNICODE_NULL) &&
                 ( Current != OrderList->Buffer));

        //
        // Current now points to the beginning of the NULL-terminated
        // Unicode string.
        // End now points to the end of the string
        //
        CurrentGroup.Length = (USHORT) ((PCHAR)End - (PCHAR)Current);
        CurrentGroup.MaximumLength = CurrentGroup.Length;
        CurrentGroup.Buffer = Current;
        Next = DriverListHead->Flink;
        while (Next != DriverListHead) {
            CurrentNode = CONTAINING_RECORD(Next,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);
            Next = CurrentNode->ListEntry.Link.Flink;
            if (CurrentNode->Group.Buffer != NULL) {
                if (RtlEqualUnicodeString(&CurrentGroup, &CurrentNode->Group,TRUE)) {
                    RemoveEntryList(&CurrentNode->ListEntry.Link);
                    InsertHeadList(DriverListHead,
                                   &CurrentNode->ListEntry.Link);
                }
            }
        }
        --Current;

    }

    return(TRUE);

}


BOOLEAN
CmpResolveDriverDependencies(
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    This routine orders driver nodes in a group based on their dependencies
    on one another.  It removes any drivers that have circular dependencies
    from the list.

Arguments:

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

Return Value:

    TRUE - Dependencies successfully resolved

    FALSE - Corrupt hive.

--*/

{
    PLIST_ENTRY CurrentEntry;
    PBOOT_DRIVER_NODE GroupStart;
    PBOOT_DRIVER_NODE GroupEnd;
    PBOOT_DRIVER_NODE CurrentNode;

    CurrentEntry = DriverListHead->Flink;

    while (CurrentEntry != DriverListHead) {
        //
        // The list is already ordered by groups.  Find the first and
        // last entry in each group, and order each of these sub-lists
        // based on their dependencies.
        //

        GroupStart = CONTAINING_RECORD(CurrentEntry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);
        do {
            GroupEnd = CONTAINING_RECORD(CurrentEntry,
                                         BOOT_DRIVER_NODE,
                                         ListEntry.Link);

            CurrentEntry = CurrentEntry->Flink;
            CurrentNode = CONTAINING_RECORD(CurrentEntry,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);

            if (CurrentEntry == DriverListHead) {
                break;
            }

            if (!RtlEqualUnicodeString(&GroupStart->Group,
                                       &CurrentNode->Group,
                                       TRUE)) {
                break;
            }

        } while ( CurrentEntry != DriverListHead );

        //
        // GroupStart now points to the first driver node in the group,
        // and GroupEnd points to the last driver node in the group.
        //
        CmpOrderGroup(GroupStart, GroupEnd);

    }
    return(TRUE);
}


BOOLEAN
CmpOrderGroup(
    IN PBOOT_DRIVER_NODE GroupStart,
    IN PBOOT_DRIVER_NODE GroupEnd
    )

/*++

Routine Description:

    Reorders the nodes in a driver group based on their tag values.

Arguments:

    GroupStart - Supplies the first node in the group.

    GroupEnd - Supplies the last node in the group.

Return Value:

    TRUE - Group successfully reordered

    FALSE - Circular dependencies detected.

--*/

{
    PBOOT_DRIVER_NODE Current;
    PBOOT_DRIVER_NODE Previous;
    PLIST_ENTRY ListEntry;
    BOOLEAN StartOver=FALSE;

    if (GroupStart == GroupEnd) {
        return(TRUE);
    }

    Current = GroupStart;

    do {
        //
        // If the driver before the current one has a lower tag, then
        // we do not need to move it.  If not, then remove the driver
        // from the list and scan backwards until we find a driver with
        // a tag that is <= the current tag, or we reach the beginning
        // of the list.
        //
        Previous = Current;
        ListEntry = Current->ListEntry.Link.Flink;
        Current = CONTAINING_RECORD(ListEntry,
                                    BOOT_DRIVER_NODE,
                                    ListEntry.Link);

        if (Previous->Tag > Current->Tag) {
            //
            // Remove the Current driver from the list, and search
            // backwards until we find a tag that is <= the current
            // driver's tag.  Reinsert the current driver there.
            //
            if (Current == GroupEnd) {
                ListEntry = Current->ListEntry.Link.Blink;
                GroupEnd = CONTAINING_RECORD(ListEntry,
                                             BOOT_DRIVER_NODE,
                                             ListEntry.Link);
            }
            RemoveEntryList(&Current->ListEntry.Link);
            while ( (Previous->Tag > Current->Tag) &&
                    (Previous != GroupStart) ) {
                ListEntry = Previous->ListEntry.Link.Blink;
                Previous = CONTAINING_RECORD(ListEntry,
                                             BOOT_DRIVER_NODE,
                                             ListEntry.Link);
            }
            InsertTailList(&Previous->ListEntry.Link,
                           &Current->ListEntry.Link);
            if (Previous == GroupStart) {
                GroupStart = Current;
            }
        }

    } while ( Current != GroupEnd );

    return(TRUE);
}


HCELL_INDEX
CmpFindControlSet(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX RootCell,
     IN PUNICODE_STRING SelectName,
     OUT PBOOLEAN AutoSelect
     )

/*++

Routine Description:

    This routines parses the SYSTEM hive and "Select" node
    to locate the control set to be used for booting.

    Note that this routines also updates the value of Current to reflect
    the control set that was just found.  This is what we want to do
    when this is called during boot.  During I/O initialization, this
    is irrelevant, since we're just changing it to what it already is.

Arguments:

    SystemHive - Supplies the hive control structure for the SYSTEM hive.

    RootCell - Supplies the HCELL_INDEX of the root cell of the hive.

    SelectName - Supplies the name of the Select value to be used in
            determining the control set.  This should be one of "Current"
            "Default" or "LastKnownGood"

    AutoSelect - Returns the value of the AutoSelect value under
            the Select node.

Return Value:

    != HCELL_NIL - Cell Index of the control set to be used for booting.
    == HCELL_NIL - Indicates the hive is corrupt or inconsistent

--*/

{
    HCELL_INDEX Select;
    HCELL_INDEX ValueCell;
    HCELL_INDEX ControlSet;
    HCELL_INDEX AutoSelectCell;
    NTSTATUS Status;
    UNICODE_STRING Name;
    ANSI_STRING AnsiString;
    PHCELL_INDEX Index;
    PCM_KEY_VALUE Value;
    PULONG ControlSetIndex;
    PULONG CurrentControl;
    CHAR AsciiBuffer[128];
    WCHAR UnicodeBuffer[128];
    ULONG realsize;

    //
    // Find \SYSTEM\SELECT node.
    //
    RtlInitUnicodeString(&Name, L"select");
    Select = CmpFindSubKeyByName(SystemHive,
                                (PCM_KEY_NODE)HvGetCell(SystemHive,RootCell),
                                &Name);
    if (Select == HCELL_NIL) {
        return(HCELL_NIL);
    }

    //
    // Find AutoSelect value
    //
    RtlInitUnicodeString(&Name, L"AutoSelect");
    AutoSelectCell = CmpFindValueByName(SystemHive,
                                        (PCM_KEY_NODE)HvGetCell(SystemHive,Select),
                                        &Name);
    if (AutoSelectCell == HCELL_NIL) {
        //
        // It's not there, we don't care.  Set autoselect to TRUE
        //
        *AutoSelect = TRUE;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, AutoSelectCell);
        *AutoSelect = *(PBOOLEAN)(CmpValueToData(SystemHive,Value,realsize));
    }

    ValueCell = CmpFindValueByName(SystemHive,
                                   (PCM_KEY_NODE)HvGetCell(SystemHive,Select),
                                   SelectName);
    if (ValueCell == HCELL_NIL) {
        return(HCELL_NIL);
    }
    Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
    if (Value->Type != REG_DWORD) {
        return(HCELL_NIL);
    }

    ControlSetIndex = (PULONG)CmpValueToData(SystemHive, Value,realsize);

    //
    // Find appropriate control set
    //

    sprintf(AsciiBuffer, "ControlSet%03d", *ControlSetIndex);
    AnsiString.Length = AnsiString.MaximumLength = strlen(&(AsciiBuffer[0]));
    AnsiString.Buffer = AsciiBuffer;
    Name.MaximumLength = 128*sizeof(WCHAR);
    Name.Buffer = UnicodeBuffer;
    Status = RtlAnsiStringToUnicodeString(&Name,
                                          &AnsiString,
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        return(HCELL_NIL);
    }
    ControlSet = CmpFindSubKeyByName(SystemHive,
                                     (PCM_KEY_NODE)HvGetCell(SystemHive,RootCell),
                                     &Name);
    if (ControlSet == HCELL_NIL) {
        return(HCELL_NIL);
    }

    //
    // Control set was successfully found, so update the value in "Current"
    // to reflect the control set we are going to use.
    //
    RtlInitUnicodeString(&Name, L"Current");
    ValueCell = CmpFindValueByName(SystemHive,
                                   (PCM_KEY_NODE)HvGetCell(SystemHive,Select),
                                   &Name);
    if (ValueCell != HCELL_NIL) {
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
        if (Value->Type == REG_DWORD) {
            CurrentControl = (PULONG)CmpValueToData(SystemHive, Value,realsize);
            *CurrentControl = *ControlSetIndex;
        }
    }
    return(ControlSet);

}


VOID
CmpSetCurrentProfile(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PCM_HARDWARE_PROFILE Profile
    )

/*++

Routine Description:

    Edits the in-memory copy of the registry to reflect the hardware
    profile that the system is booting from.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    ControlSet - Supplies the HCELL_INDEX of the current control set.

    Profile - Supplies a pointer to the selected hardware profile

Return Value:

    None.

--*/

{
    HCELL_INDEX IDConfigDB;
    PCM_KEY_NODE IDConfigNode;
    HCELL_INDEX CurrentConfigCell;
    PCM_KEY_VALUE CurrentConfigValue;
    UNICODE_STRING Name;
    PULONG CurrentConfig;
    ULONG realsize;

    IDConfigDB = CmpFindProfileOption(Hive,
                                      ControlSet,
                                      NULL,
                                      NULL,
                                      NULL);
    if (IDConfigDB != HCELL_NIL) {
        IDConfigNode = (PCM_KEY_NODE)HvGetCell(Hive, IDConfigDB);
        RtlInitUnicodeString(&Name, L"CurrentConfig");
        CurrentConfigCell = CmpFindValueByName(Hive,
                                               IDConfigNode,
                                               &Name);
        if (CurrentConfigCell != HCELL_NIL) {
            CurrentConfigValue = (PCM_KEY_VALUE)HvGetCell(Hive, CurrentConfigCell);
            if (CurrentConfigValue->Type == REG_DWORD) {
                CurrentConfig = (PULONG)CmpValueToData(Hive,
                                                       CurrentConfigValue,
                                                       realsize);
                *CurrentConfig = Profile->Id;
            }
        }
    }


}


HCELL_INDEX
CmpFindProfileOption(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX ControlSet,
     OUT OPTIONAL PCM_HARDWARE_PROFILE_LIST *ReturnedProfileList,
     OUT OPTIONAL PCM_HARDWARE_PROFILE_ALIAS_LIST *ReturnedAliasList,
     OUT OPTIONAL PULONG ProfileTimeout
     )

/*++

Routine Description:

    This routines parses the SYSTEM hive and locates the
    "CurrentControlSet\Control\IDConfigDB" node to determine the
    hardware profile configuration settings.

Arguments:

    SystemHive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root cell of the hive.

    ProfileList - Returns the list of available hardware profiles sorted
                  by preference. Will be allocated by this routine if
                  NULL is passed in, or a pointer to a CM_HARDWARE_PROFILE_LIST
                  structure that is too small is passed in.

    ProfileTimeout - Returns the timeout value for the config menu.

Return Value:

    != HCELL_NIL - Cell Index of the IDConfigDB node.
    == HCELL_NIL - Indicates IDConfigDB does not exist

--*/
{
    HCELL_INDEX ControlCell;
    HCELL_INDEX IDConfigDB;
    HCELL_INDEX DefaultCell;
    HCELL_INDEX TimeoutCell;
    HCELL_INDEX ProfileCell;
    HCELL_INDEX AliasCell;
    HCELL_INDEX HWCell;
    PCM_KEY_NODE HWNode;
    PCM_KEY_NODE ProfileNode;
    PCM_KEY_NODE AliasNode;
    PCM_KEY_NODE ConfigDBNode;
    PCM_KEY_NODE Control;
    PCM_KEY_VALUE TimeoutValue;
    UNICODE_STRING Name;
    ULONG realsize;
    PCM_HARDWARE_PROFILE_LIST ProfileList;
    PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList;
    ULONG ProfileCount;
    ULONG AliasCount;
    ULONG i,j;
    WCHAR NameBuf[20];

    //
    // Find Control node
    //
    RtlInitUnicodeString(&Name, L"Control");
    ControlCell = CmpFindSubKeyByName(SystemHive,
                                      (PCM_KEY_NODE)HvGetCell(SystemHive,ControlSet),
                                      &Name);
    if (ControlCell == HCELL_NIL) {
        return(HCELL_NIL);
    }
    Control = (PCM_KEY_NODE)HvGetCell(SystemHive, ControlCell);

    //
    // Find IDConfigDB node
    //
    RtlInitUnicodeString(&Name, L"IDConfigDB");
    IDConfigDB = CmpFindSubKeyByName(SystemHive,
                                     Control,
                                     &Name);
    if (IDConfigDB == HCELL_NIL) {
        return(HCELL_NIL);
    }
    ConfigDBNode = (PCM_KEY_NODE)HvGetCell(SystemHive, IDConfigDB);

    if (ARGUMENT_PRESENT(ProfileTimeout)) {
        //
        // Find UserWaitInterval value. This is the timeout
        //
        RtlInitUnicodeString(&Name, L"UserWaitInterval");
        TimeoutCell = CmpFindValueByName(SystemHive,
                                         ConfigDBNode,
                                         &Name);
        if (TimeoutCell == HCELL_NIL) {
            *ProfileTimeout = 0;
        } else {
            TimeoutValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, TimeoutCell);
            if (TimeoutValue->Type != REG_DWORD) {
                *ProfileTimeout = 0;
            } else {
                *ProfileTimeout = *(PULONG)CmpValueToData(SystemHive, TimeoutValue, realsize);
            }
        }
    }

    if (ARGUMENT_PRESENT(ReturnedProfileList)) {
        ProfileList = *ReturnedProfileList;
        //
        // Enumerate the keys under IDConfigDB\Hardware Profiles
        // and build the list of available hardware profiles.  The list
        // is built sorted by PreferenceOrder.  Therefore, when the
        // list is complete, the default hardware profile is at the
        // head of the list.
        //
        RtlInitUnicodeString(&Name, L"Hardware Profiles");
        ProfileCell = CmpFindSubKeyByName(SystemHive,
                                          ConfigDBNode,
                                          &Name);
        if (ProfileCell == HCELL_NIL) {
            ProfileCount = 0;
            if (ProfileList != NULL) {
                ProfileList->CurrentProfileCount = 0;
            }
        } else {
            ProfileNode = (PCM_KEY_NODE)HvGetCell(SystemHive, ProfileCell);
            ProfileCount = ProfileNode->SubKeyCounts[Stable];
            if ((ProfileList == NULL) || (ProfileList->MaxProfileCount < ProfileCount)) {
                //
                // Allocate a larger ProfileList
                //
                ProfileList = (SystemHive->Allocate)(sizeof(CM_HARDWARE_PROFILE_LIST)
                                                     + (ProfileCount-1) * sizeof(CM_HARDWARE_PROFILE),
                                                     FALSE);
                if (ProfileList == NULL) {
                    return(HCELL_NIL);
                }
                ProfileList->MaxProfileCount = ProfileCount;
            }
            ProfileList->CurrentProfileCount = 0;

            //
            // Enumerate the keys and fill in the profile list.
            //
            for (i=0; i<ProfileCount; i++) {
                CM_HARDWARE_PROFILE TempProfile;
                HCELL_INDEX ValueCell;
                PCM_KEY_VALUE ValueNode;
                UNICODE_STRING KeyName;
                ULONG realsize;

                HWCell = CmpFindSubKeyByNumber(SystemHive, ProfileNode, i);
                if (HWCell == HCELL_NIL) {
                    //
                    // This should never happen.
                    //
                    ProfileList->CurrentProfileCount = i;
                    break;
                }
                HWNode = (PCM_KEY_NODE)HvGetCell(SystemHive, HWCell);
                if (HWNode->Flags & KEY_COMP_NAME) {
                    KeyName.Length = CmpCompressedNameSize(HWNode->Name,
                                                           HWNode->NameLength);
                    KeyName.MaximumLength = sizeof(NameBuf);
                    if (KeyName.MaximumLength < KeyName.Length) {
                        KeyName.Length = KeyName.MaximumLength;
                    }
                    KeyName.Buffer = NameBuf;
                    CmpCopyCompressedName(KeyName.Buffer,
                                          KeyName.Length,
                                          HWNode->Name,
                                          HWNode->NameLength);
                } else {
                    KeyName.Length = KeyName.MaximumLength = HWNode->NameLength;
                    KeyName.Buffer = HWNode->Name;
                }

                //
                // Fill in the temporary profile structure with this
                // profile's data.
                //
                RtlUnicodeStringToInteger(&KeyName, 0, &TempProfile.Id);
                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.PreferenceOrder = (ULONG)-1;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempProfile.PreferenceOrder = *(PULONG)CmpValueToData(SystemHive,
                                                                          ValueNode,
                                                                          realsize);
                }
                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_FRIENDLY_NAME);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.FriendlyName = L"-------";
                    TempProfile.NameLength = wcslen(TempProfile.FriendlyName) * sizeof(WCHAR);
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempProfile.FriendlyName = (PWSTR)CmpValueToData(SystemHive,
                                                                     ValueNode,
                                                                     realsize);
                    TempProfile.NameLength = realsize - sizeof(WCHAR);
                }

                TempProfile.Flags = 0;

                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_ALIASABLE);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.Flags = CM_HP_FLAGS_ALIASABLE;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);

                    if (*(PULONG)CmpValueToData (SystemHive,
                                                 ValueNode,
                                                 realsize)) {
                        TempProfile.Flags = CM_HP_FLAGS_ALIASABLE;
                        // NO other flags set.
                    }
                }

                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_PRISTINE);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell != HCELL_NIL) {

                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);

                    if (*(PULONG)CmpValueToData (SystemHive,
                                                 ValueNode,
                                                 realsize)) {
                        TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
                        // NO other flags set.
                    }
                }

                //
                // If we see a profile with the ID of zero (AKA an illegal)
                // ID for a hardware profile to possess, then we know that this
                // must be a pristine profile.
                //
                if (0 == TempProfile.Id) {
                    TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
                    // NO other flags set.

                    TempProfile.PreferenceOrder = -1; // move to the end of the list.
                }


                //
                // Insert this new profile into the appropriate spot in the
                // profile array. Entries are sorted by preference order.
                //
                for (j=0; j<ProfileList->CurrentProfileCount; j++) {
                    if (ProfileList->Profile[j].PreferenceOrder >= TempProfile.PreferenceOrder) {
                        //
                        // Insert at position j.
                        //
                        RtlMoveMemory(&ProfileList->Profile[j+1],
                                      &ProfileList->Profile[j],
                                      sizeof(CM_HARDWARE_PROFILE)*(ProfileList->MaxProfileCount-j-1));
                        break;
                    }
                }
                ProfileList->Profile[j] = TempProfile;
                ++ProfileList->CurrentProfileCount;
            }
        }
        *ReturnedProfileList = ProfileList;
    }

    if (ARGUMENT_PRESENT(ReturnedAliasList)) {
        AliasList = *ReturnedAliasList;
        //
        // Enumerate the keys under IDConfigDB\Alias
        // and build the list of available hardware profiles aliases.
        // So that if we know our docking state we can find it in the alias
        // table.
        //
        RtlInitUnicodeString(&Name, L"Alias");
        AliasCell = CmpFindSubKeyByName(SystemHive,
                                        ConfigDBNode,
                                        &Name);
        if (AliasCell == HCELL_NIL) {
            AliasCount = 0;
            if (AliasList != NULL) {
                AliasList->CurrentAliasCount = 0;
            }
        } else {
            AliasNode = (PCM_KEY_NODE)HvGetCell(SystemHive, AliasCell);
            AliasCount = AliasNode->SubKeyCounts[Stable];
            if ((AliasList == NULL) || (AliasList->MaxAliasCount < AliasCount)) {
                //
                // Allocate a larger AliasList
                //
                AliasList = (SystemHive->Allocate)(sizeof(CM_HARDWARE_PROFILE_LIST)
                                                   + (AliasCount-1) * sizeof(CM_HARDWARE_PROFILE),
                                                   FALSE);
                if (AliasList == NULL) {
                    return(HCELL_NIL);
                }
                AliasList->MaxAliasCount = AliasCount;
            }
            AliasList->CurrentAliasCount = 0;

            //
            // Enumerate the keys and fill in the profile list.
            //
            for (i=0; i<AliasCount; i++) {
#define TempAlias AliasList->Alias[i]
                HCELL_INDEX ValueCell;
                PCM_KEY_VALUE ValueNode;
                UNICODE_STRING KeyName;
                ULONG realsize;

                HWCell = CmpFindSubKeyByNumber(SystemHive, AliasNode, i);
                if (HWCell == HCELL_NIL) {
                    //
                    // This should never happen.
                    //
                    AliasList->CurrentAliasCount = i;
                    break;
                }
                HWNode = (PCM_KEY_NODE)HvGetCell(SystemHive, HWCell);
                if (HWNode->Flags & KEY_COMP_NAME) {
                    KeyName.Length = CmpCompressedNameSize(HWNode->Name,
                                                           HWNode->NameLength);
                    KeyName.MaximumLength = sizeof(NameBuf);
                    if (KeyName.MaximumLength < KeyName.Length) {
                        KeyName.Length = KeyName.MaximumLength;
                    }
                    KeyName.Buffer = NameBuf;
                    CmpCopyCompressedName(KeyName.Buffer,
                                          KeyName.Length,
                                          HWNode->Name,
                                          HWNode->NameLength);
                } else {
                    KeyName.Length = KeyName.MaximumLength = HWNode->NameLength;
                    KeyName.Buffer = HWNode->Name;
                }

                //
                // Fill in the temporary profile structure with this
                // profile's data.
                //
                RtlInitUnicodeString(&Name, L"ProfileNumber");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.ProfileNumber = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempAlias.ProfileNumber = *(PULONG)CmpValueToData(SystemHive,
                                                                      ValueNode,
                                                                      realsize);
                }
                RtlInitUnicodeString(&Name, L"DockState");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.DockState = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempAlias.DockState = *(PULONG)CmpValueToData(SystemHive,
                                                                  ValueNode,
                                                                  realsize);
                }
                RtlInitUnicodeString(&Name, L"DockID");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.DockID = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempAlias.DockID = *(PULONG)CmpValueToData(SystemHive,
                                                               ValueNode,
                                                               realsize);
                }
                RtlInitUnicodeString(&Name, L"SerialNumber");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.SerialNumber = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    TempAlias.SerialNumber = *(PULONG)CmpValueToData(SystemHive,
                                                                     ValueNode,
                                                                     realsize);
                }

                ++AliasList->CurrentAliasCount;
            }
        }
        *ReturnedAliasList = AliasList;
    }

    return(IDConfigDB);
}


ULONG
CmpFindTagIndex(
    IN PHHIVE Hive,
    IN HCELL_INDEX TagCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING GroupName
    )

/*++

Routine Description:

    Calculates the tag index for a driver based on its tag value and
    the GroupOrderList entry for its group.

Arguments:

    Hive - Supplies the hive control structure for the driver.

    TagCell - Supplies the cell index of the driver's tag value cell.

    GroupOrderCell - Supplies the cell index for the control set's
            GroupOrderList:

            \Registry\Machine\System\CurrentControlSet\Control\GroupOrderList

    GroupName - Supplies the name of the group the driver belongs to.
            Note that if a driver's group does not have an entry under
            GroupOrderList, its tags will be ignored.  Also note that if
            a driver belongs to no group (GroupName is NULL) its tags will
            be ignored.

Return Value:

    The index that the driver should be sorted by.

--*/

{
    PCM_KEY_VALUE TagValue;
    PCM_KEY_VALUE DriverTagValue;
    HCELL_INDEX OrderCell;
    PULONG OrderVector;
    PULONG DriverTag;
    NTSTATUS Status;
    ULONG CurrentTag;
    ULONG realsize;


    DriverTagValue = (PCM_KEY_VALUE)HvGetCell(Hive, TagCell);
    DriverTag = (PULONG)CmpValueToData(Hive, DriverTagValue, realsize);

    OrderCell = CmpFindValueByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,GroupOrderCell),
                                   GroupName);
    if (OrderCell == HCELL_NIL) {
        return(LOAD_NEXT_TO_LAST);
    }

    TagValue = (PCM_KEY_VALUE)HvGetCell(Hive, OrderCell);
    OrderVector = (PULONG)CmpValueToData(Hive, TagValue,realsize);

    for (CurrentTag=1; CurrentTag <= OrderVector[0]; CurrentTag++) {
        if (OrderVector[CurrentTag] == *DriverTag) {
            //
            // We have found a matching tag in the OrderVector, so return
            // its index.
            //
            return(CurrentTag);
        }
    }

    //
    // There was no matching tag in the OrderVector.
    //
    return(LOAD_NEXT_TO_LAST);

}

#ifdef _WANT_MACHINE_IDENTIFICATION

BOOLEAN
CmpGetBiosDateFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING Date
    )

/*++

Routine Description:

    Reads and returns the BIOS date from the registry.

Arguments:

    Hive - Supplies the hive control structure for the driver.

    ControlSet - Supplies the HCELL_INDEX of the root cell of the hive.

    Date - Receives the date string in the format "mm/dd/yy".
    
Return Value:

 	TRUE iff successful, else FALSE.
 	
--*/

{
    UNICODE_STRING  name;
    HCELL_INDEX     control;
    HCELL_INDEX     biosInfo;
    HCELL_INDEX     valueCell;
    PCM_KEY_VALUE   value;
    ULONG           realSize;

    //
    // Find CONTROL node
    //
    
    RtlInitUnicodeString(&name, L"Control");
    control = CmpFindSubKeyByName(  Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, ControlSet),
                                    &name);
    if (control == HCELL_NIL) {
    
        return(FALSE);
    }

    //
    // Find BIOSINFO node
    //
    
    RtlInitUnicodeString(&name, L"BIOSINFO");
    biosInfo = CmpFindSubKeyByName( Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, control),
                                    &name);
    if (biosInfo == HCELL_NIL) {
    
        return(FALSE);
    }
    
    //
    // Find SystemBiosDate value
    //
    
    RtlInitUnicodeString(&name, L"SystemBiosDate");
    valueCell = CmpFindValueByName( Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, biosInfo),
                                    &name);
    if (valueCell == HCELL_NIL) {
    
        return(FALSE);
    }

    value = (PCM_KEY_VALUE)HvGetCell(Hive, valueCell);
    Date->Buffer = (PWSTR)CmpValueToData(Hive, value, realSize);
    Date->MaximumLength=(USHORT)realSize;
    Date->Length = 0;
    while ( (Date->Length < Date->MaximumLength) &&
            (Date->Buffer[Date->Length/sizeof(WCHAR)] != UNICODE_NULL)) {
            
        Date->Length += sizeof(WCHAR);
    }

    return (TRUE);
}

BOOLEAN
CmpGetBiosinfoFileNameFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING InfName
    )
{
    UNICODE_STRING  name;
    HCELL_INDEX     control;
    HCELL_INDEX     biosInfo;
    HCELL_INDEX     valueCell;
    PCM_KEY_VALUE   value;
    ULONG           realSize;

    //
    // Find CONTROL node
    //
    
    RtlInitUnicodeString(&name, L"Control");
    control = CmpFindSubKeyByName(  Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, ControlSet),
                                    &name);
    if (control == HCELL_NIL) {
    
        return(FALSE);
    }

    //
    // Find BIOSINFO node
    //
    
    RtlInitUnicodeString(&name, L"BIOSINFO");
    biosInfo = CmpFindSubKeyByName( Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, control),
                                    &name);
    if (biosInfo == HCELL_NIL) {
    
        return(FALSE);
    }
    
    //
    // Find InfName value
    //
    
    RtlInitUnicodeString(&name, L"InfName");
    valueCell = CmpFindValueByName( Hive,
                                    (PCM_KEY_NODE)HvGetCell(Hive, biosInfo),
                                    &name);
    if (valueCell == HCELL_NIL) {
    
        return(FALSE);
    }
    
    value = (PCM_KEY_VALUE)HvGetCell(Hive, valueCell);
    InfName->Buffer = (PWSTR)CmpValueToData(Hive, value, realSize);
    InfName->MaximumLength=(USHORT)realSize;
    InfName->Length = 0;
    while ( (InfName->Length < InfName->MaximumLength) &&
            (InfName->Buffer[InfName->Length/sizeof(WCHAR)] != UNICODE_NULL)) {
            
        InfName->Length += sizeof(WCHAR);
    }

    return (TRUE);
}

#endif
