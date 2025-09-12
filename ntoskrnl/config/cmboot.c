/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Configuration Manager - Boot Initialization
 * COPYRIGHT:   Copyright 2007 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2010 ReactOS Portable Systems Group
 *              Copyright 2022 Hermès Bélusca-Maïto
 *
 * NOTE: This module is shared by both the kernel and the bootloader.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#ifdef _BLDR_

#undef CODE_SEG
#define CODE_SEG(...)

#include <ntstrsafe.h>
#include <cmlib.h>
#include "internal/cmboot.h"

// HACK: This is part of non-NT-compatible SafeBoot support in kernel.
ULONG InitSafeBootMode = 0;

DBG_DEFAULT_CHANNEL(REGISTRY);
#define CMTRACE(x, fmt, ...) TRACE(fmt, ##__VA_ARGS__) // DPRINT

#endif /* _BLDR_ */


/* DEFINES ********************************************************************/

#define CM_BOOT_DEBUG   0x20

#define IS_NULL_TERMINATED(Buffer, Size) \
    (((Size) >= sizeof(WCHAR)) && ((Buffer)[(Size) / sizeof(WCHAR) - 1] == UNICODE_NULL))


/* FUNCTIONS ******************************************************************/

// HACK: This is part of non-NT-compatible SafeBoot support in kernel.
extern ULONG InitSafeBootMode;

CODE_SEG("INIT")
static
BOOLEAN
CmpIsSafe(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX SafeBootCell,
    _In_ HCELL_INDEX DriverCell);

/**
 * @brief
 * Finds the corresponding "HKLM\SYSTEM\ControlSetXXX" system control set
 * registry key, according to the "Current", "Default", or "LastKnownGood"
 * values in the "HKLM\SYSTEM\Select" registry key.
 *
 * @param[in]   SystemHive
 * The SYSTEM hive.
 *
 * @param[in]   RootCell
 * The root cell of the SYSTEM hive.
 *
 * @param[in]   SelectKeyName
 * The control set to check for: either "Current", "Default", or
 * "LastKnownGood", the value of which selects the corresponding
 * "HKLM\SYSTEM\ControlSetXXX" control set registry key.
 *
 * @param[out]  AutoSelect
 * Value of the "AutoSelect" registry value (unused).
 *
 * @return
 * The control set registry key's hive cell (if found), or HCELL_NIL.
 **/
CODE_SEG("INIT")
HCELL_INDEX
NTAPI
CmpFindControlSet(
    _In_ PHHIVE SystemHive,
    _In_ HCELL_INDEX RootCell,
    _In_ PCUNICODE_STRING SelectKeyName,
    _Out_ PBOOLEAN AutoSelect)
{
    UNICODE_STRING Name;
    PCM_KEY_NODE Node;
    HCELL_INDEX SelectCell, AutoSelectCell, SelectValueCell, ControlSetCell;
    HCELL_INDEX CurrentValueCell;
    PCM_KEY_VALUE Value;
    ULONG Length;
    NTSTATUS Status;
    PULONG CurrentData;
    PULONG ControlSetId;
    WCHAR Buffer[128];

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(SystemHive->ReleaseCellRoutine == NULL);

    /* Get the Select key */
    RtlInitUnicodeString(&Name, L"select");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    SelectCell = CmpFindSubKeyByName(SystemHive, Node, &Name);
    if (SelectCell == HCELL_NIL) return HCELL_NIL;

    /* Get AutoSelect value */
    RtlInitUnicodeString(&Name, L"AutoSelect");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    AutoSelectCell = CmpFindValueByName(SystemHive, Node, &Name);
    if (AutoSelectCell == HCELL_NIL)
    {
        /* Assume TRUE if the value is missing */
        *AutoSelect = TRUE;
    }
    else
    {
        /* Read the value */
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, AutoSelectCell);
        if (!Value) return HCELL_NIL;
        // if (Value->Type != REG_DWORD) return HCELL_NIL;

        /* Convert it to a boolean */
        CurrentData = (PULONG)CmpValueToData(SystemHive, Value, &Length);
        if (!CurrentData) return HCELL_NIL;
        // if (Length < sizeof(ULONG)) return HCELL_NIL;

        *AutoSelect = *(PBOOLEAN)CurrentData;
    }

    /* Now find the control set being looked up */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    SelectValueCell = CmpFindValueByName(SystemHive, Node, SelectKeyName);
    if (SelectValueCell == HCELL_NIL) return HCELL_NIL;

    /* Read the value (corresponding to the CCS ID) */
    Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, SelectValueCell);
    if (!Value) return HCELL_NIL;
    if (Value->Type != REG_DWORD) return HCELL_NIL;
    ControlSetId = (PULONG)CmpValueToData(SystemHive, Value, &Length);
    if (!ControlSetId) return HCELL_NIL;
    if (Length < sizeof(ULONG)) return HCELL_NIL;

    /* Now build the CCS's Name */
    Status = RtlStringCbPrintfW(Buffer, sizeof(Buffer),
                                L"ControlSet%03lu", *ControlSetId);
    if (!NT_SUCCESS(Status)) return HCELL_NIL;
    /* RtlStringCbPrintfW ensures the buffer to be NULL-terminated */
    RtlInitUnicodeString(&Name, Buffer);

    /* Now open it */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    ControlSetCell = CmpFindSubKeyByName(SystemHive, Node, &Name);
    if (ControlSetCell == HCELL_NIL) return HCELL_NIL;

    /* Get the value of the "Current" CCS */
    RtlInitUnicodeString(&Name, L"Current");
    Node =  (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    CurrentValueCell = CmpFindValueByName(SystemHive, Node, &Name);

    /* Make sure it exists */
    if (CurrentValueCell != HCELL_NIL)
    {
        /* Get the current value and make sure it's a ULONG */
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, CurrentValueCell);
        if (!Value) return HCELL_NIL;
        if (Value->Type == REG_DWORD)
        {
            /* Get the data and update it */
            CurrentData = (PULONG)CmpValueToData(SystemHive, Value, &Length);
            if (!CurrentData) return HCELL_NIL;
            if (Length < sizeof(ULONG)) return HCELL_NIL;

            *CurrentData = *ControlSetId;
        }
    }

    /* Return the CCS cell */
    return ControlSetCell;
}

/**
 * @brief
 * Finds the index of the driver's "Tag" value
 * in its corresponding group ordering list.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   TagCell
 * The driver's "Tag" registry value's hive cell.
 *
 * @param[in]   GroupOrderCell
 * The hive cell of the "Control\GroupOrderList" registry key
 * inside the currently selected control set.
 *
 * @param[in]   GroupName
 * The driver's group name.
 *
 * @return
 * The corresponding tag index, or -1 (last position),
 * or -2 (next-to-last position).
 **/
CODE_SEG("INIT")
static
ULONG
CmpFindTagIndex(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX TagCell,
    _In_ HCELL_INDEX GroupOrderCell,
    _In_ PCUNICODE_STRING GroupName)
{
    PCM_KEY_VALUE TagValue, Value;
    PCM_KEY_NODE Node;
    HCELL_INDEX OrderCell;
    PULONG DriverTag, TagOrder;
    ULONG CurrentTag, Length;
    BOOLEAN BufferAllocated;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Get the tag */
    Value = (PCM_KEY_VALUE)HvGetCell(Hive, TagCell);
    if (!Value) return -2;
    if (Value->Type != REG_DWORD) return -2;
    DriverTag = (PULONG)CmpValueToData(Hive, Value, &Length);
    if (!DriverTag) return -2;
    if (Length < sizeof(ULONG)) return -2;

    /* Get the order array */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, GroupOrderCell);
    if (!Node) return -2;
    OrderCell = CmpFindValueByName(Hive, Node, GroupName);
    if (OrderCell == HCELL_NIL) return -2;

    /* And read it */
    TagValue = (PCM_KEY_VALUE)HvGetCell(Hive, OrderCell);
    if (!TagValue) return -2;
    if (!CmpGetValueData(Hive,
                         TagValue,
                         &Length,
                         (PVOID*)&TagOrder,
                         &BufferAllocated,
                         &OrderCell)
        || !TagOrder)
    {
        return -2;
    }

    /* Parse each tag */
    for (CurrentTag = 1; CurrentTag <= TagOrder[0]; CurrentTag++)
    {
        /* Find a match */
        if (TagOrder[CurrentTag] == *DriverTag)
        {
            /* Found it -- return the tag */
            if (BufferAllocated) Hive->Free(TagOrder, Length);
            return CurrentTag;
        }
    }

    /* No matches, so assume next to last ordering */
    if (BufferAllocated) Hive->Free(TagOrder, Length);
    return -2;
}

#ifdef _BLDR_

/**
 * @brief
 * Checks whether the specified named driver is already in the driver list.
 * Optionally returns its corresponding driver node.
 *
 * @remarks Used in bootloader only.
 *
 * @param[in]   DriverListHead
 * The driver list.
 *
 * @param[in]   DriverName
 * The name of the driver to search for.
 *
 * @param[out]  FoundDriver
 * Optional pointer that receives in output the address of the
 * matching driver node, if any, or NULL if none has been found.
 *
 * @return
 * TRUE if the driver has been found, FALSE if not.
 **/
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpIsDriverInList(
    _In_ PLIST_ENTRY DriverListHead,
    _In_ PCUNICODE_STRING DriverName,
    _Out_opt_ PBOOT_DRIVER_NODE* FoundDriver)
{
    PLIST_ENTRY Entry;
    PBOOT_DRIVER_NODE DriverNode;

    for (Entry = DriverListHead->Flink;
         Entry != DriverListHead;
         Entry = Entry->Flink)
    {
        DriverNode = CONTAINING_RECORD(Entry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);

        if (RtlEqualUnicodeString(&DriverNode->Name,
                                  DriverName,
                                  TRUE))
        {
            /* The driver node has been found */
            if (FoundDriver)
                *FoundDriver = DriverNode;
            return TRUE;
        }
    }

    /* None has been found */
    if (FoundDriver)
        *FoundDriver = NULL;
    return FALSE;
}

#endif /* _BLDR_ */

/**
 * @brief
 * Inserts the specified driver entry into the driver list.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   DriverCell
 * The registry key's hive cell of the driver to be added, inside
 * the "Services" sub-key of the currently selected control set.
 *
 * @param[in]   GroupOrderCell
 * The hive cell of the "Control\GroupOrderList" registry key
 * inside the currently selected control set.
 *
 * @param[in]   RegistryPath
 * Constant UNICODE_STRING pointing to
 * "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\".
 *
 * @param[in,out]   DriverListHead
 * The driver list where to insert the driver entry.
 *
 * @return
 * TRUE if the driver has been inserted into the list, FALSE if not.
 **/
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpAddDriverToList(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX DriverCell,
    _In_ HCELL_INDEX GroupOrderCell,
    _In_ PCUNICODE_STRING RegistryPath,
    _Inout_ PLIST_ENTRY DriverListHead)
{
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    PCM_KEY_NODE Node;
    PCM_KEY_VALUE Value;
    ULONG Length;
    USHORT NameLength;
    HCELL_INDEX ValueCell, TagCell;
    PUNICODE_STRING FilePath, RegistryString;
    UNICODE_STRING Name;
    PULONG ErrorControl;
    PWCHAR Buffer;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Allocate a driver node and initialize it */
    DriverNode = Hive->Allocate(sizeof(BOOT_DRIVER_NODE), FALSE, TAG_CM);
    if (!DriverNode)
        return FALSE;

    RtlZeroMemory(DriverNode, sizeof(BOOT_DRIVER_NODE));
    DriverEntry = &DriverNode->ListEntry;

    /* Get the driver cell */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, DriverCell);
    if (!Node)
        goto Failure;

    /* Get the name from the cell */
    NameLength = (Node->Flags & KEY_COMP_NAME) ?
                 CmpCompressedNameSize(Node->Name, Node->NameLength) :
                 Node->NameLength;
    if (NameLength < sizeof(WCHAR))
        goto Failure;

    /* Now allocate the buffer for it and copy the name */
    RtlInitEmptyUnicodeString(&DriverNode->Name,
                              Hive->Allocate(NameLength, FALSE, TAG_CM),
                              NameLength);
    if (!DriverNode->Name.Buffer)
        goto Failure;

    DriverNode->Name.Length = NameLength;
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Compressed name */
        CmpCopyCompressedName(DriverNode->Name.Buffer,
                              DriverNode->Name.Length,
                              Node->Name,
                              Node->NameLength);
    }
    else
    {
        /* Normal name */
        RtlCopyMemory(DriverNode->Name.Buffer, Node->Name, Node->NameLength);
    }

    /* Now find the image path */
    RtlInitUnicodeString(&Name, L"ImagePath");
    ValueCell = CmpFindValueByName(Hive, Node, &Name);
    if (ValueCell == HCELL_NIL)
    {
        /* Could not find it, so assume the drivers path */
        Length = sizeof(L"System32\\Drivers\\") + NameLength + sizeof(L".sys");

        /* Allocate the path name */
        FilePath = &DriverEntry->FilePath;
        RtlInitEmptyUnicodeString(FilePath,
                                  Hive->Allocate(Length, FALSE, TAG_CM),
                                  (USHORT)Length);
        if (!FilePath->Buffer)
            goto Failure;

        /* Write the path name */
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FilePath, L"System32\\Drivers\\"))  ||
            !NT_SUCCESS(RtlAppendUnicodeStringToString(FilePath, &DriverNode->Name)) ||
            !NT_SUCCESS(RtlAppendUnicodeToString(FilePath, L".sys")))
        {
            goto Failure;
        }
    }
    else
    {
        /* Path name exists, so grab it */
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        if (!Value)
            goto Failure;
        if ((Value->Type != REG_SZ) && (Value->Type != REG_EXPAND_SZ))
            goto Failure;
        Buffer = (PWCHAR)CmpValueToData(Hive, Value, &Length);
        if (!Buffer)
            goto Failure;
        if (IS_NULL_TERMINATED(Buffer, Length))
            Length -= sizeof(UNICODE_NULL);
        if (Length < sizeof(WCHAR))
            goto Failure;

        /* Allocate and setup the path name */
        FilePath = &DriverEntry->FilePath;
        RtlInitEmptyUnicodeString(FilePath,
                                  Hive->Allocate(Length, FALSE, TAG_CM),
                                  (USHORT)Length);
        if (!FilePath->Buffer)
            goto Failure;

        /* Transfer the data */
        RtlCopyMemory(FilePath->Buffer, Buffer, Length);
        FilePath->Length = (USHORT)Length;
    }

    /* Now build the registry path */
    RegistryString = &DriverEntry->RegistryPath;
    Length = RegistryPath->Length + NameLength;
    RtlInitEmptyUnicodeString(RegistryString,
                              Hive->Allocate(Length, FALSE, TAG_CM),
                              (USHORT)Length);
    if (!RegistryString->Buffer)
        goto Failure;

    /* Add the driver name to it */
    if (!NT_SUCCESS(RtlAppendUnicodeStringToString(RegistryString, RegistryPath)) ||
        !NT_SUCCESS(RtlAppendUnicodeStringToString(RegistryString, &DriverNode->Name)))
    {
        goto Failure;
    }

    /* The entry is done, add it */
    InsertHeadList(DriverListHead, &DriverEntry->Link);

    /* Now find error control settings */
    RtlInitUnicodeString(&Name, L"ErrorControl");
    ValueCell = CmpFindValueByName(Hive, Node, &Name);
    if (ValueCell == HCELL_NIL)
    {
        /* Could not find it, so assume default */
        DriverNode->ErrorControl = NormalError;
    }
    else
    {
        /* Otherwise, read whatever the data says */
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        if (!Value)
            goto Failure;
        if (Value->Type != REG_DWORD)
            goto Failure;
        ErrorControl = (PULONG)CmpValueToData(Hive, Value, &Length);
        if (!ErrorControl)
            goto Failure;
        if (Length < sizeof(ULONG))
            goto Failure;

        DriverNode->ErrorControl = *ErrorControl;
    }

    /* Next, get the group cell */
    RtlInitUnicodeString(&Name, L"group");
    ValueCell = CmpFindValueByName(Hive, Node, &Name);
    if (ValueCell == HCELL_NIL)
    {
        /* Could not find it, so set an empty string */
        RtlInitEmptyUnicodeString(&DriverNode->Group, NULL, 0);
    }
    else
    {
        /* Found it, read the group value */
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        if (!Value)
            goto Failure;
        if (Value->Type != REG_SZ) // REG_EXPAND_SZ not really allowed there.
            goto Failure;

        /* Copy it into the node */
        Buffer = (PWCHAR)CmpValueToData(Hive, Value, &Length);
        if (!Buffer)
            goto Failure;
        if (IS_NULL_TERMINATED(Buffer, Length))
            Length -= sizeof(UNICODE_NULL);

        DriverNode->Group.Buffer = Buffer;
        DriverNode->Group.Length = (USHORT)Length;
        DriverNode->Group.MaximumLength = DriverNode->Group.Length;
    }

    /* Finally, find the tag */
    RtlInitUnicodeString(&Name, L"Tag");
    TagCell = CmpFindValueByName(Hive, Node, &Name);
    if (TagCell == HCELL_NIL)
    {
        /* No tag, so load last */
        DriverNode->Tag = -1;
    }
    else
    {
        /* Otherwise, decode it based on tag order */
        DriverNode->Tag = CmpFindTagIndex(Hive,
                                          TagCell,
                                          GroupOrderCell,
                                          &DriverNode->Group);
    }

    CMTRACE(CM_BOOT_DEBUG, "Adding boot driver: '%wZ', '%wZ'\n",
            &DriverNode->Name, &DriverEntry->FilePath);

    /* All done! */
    return TRUE;

Failure:
    if (DriverEntry->RegistryPath.Buffer)
    {
        Hive->Free(DriverEntry->RegistryPath.Buffer,
                   DriverEntry->RegistryPath.MaximumLength);
    }
    if (DriverEntry->FilePath.Buffer)
    {
        Hive->Free(DriverEntry->FilePath.Buffer,
                   DriverEntry->FilePath.MaximumLength);
    }
    if (DriverNode->Name.Buffer)
    {
        Hive->Free(DriverNode->Name.Buffer,
                   DriverNode->Name.MaximumLength);
    }
    Hive->Free(DriverNode, sizeof(BOOT_DRIVER_NODE));

    return FALSE;
}

/**
 * @brief
 * Checks whether the specified driver has the expected load type.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   DriverCell
 * The registry key's hive cell of the driver, inside the
 * "Services" sub-key of the currently selected control set.
 *
 * @param[in]   LoadType
 * The load type the driver should match.
 *
 * @return
 * TRUE if the driver's load type matches, FALSE if not.
 **/
CODE_SEG("INIT")
static
BOOLEAN
CmpIsLoadType(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX Cell,
    _In_ SERVICE_LOAD_TYPE LoadType)
{
    PCM_KEY_NODE Node;
    PCM_KEY_VALUE Value;
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"Start");
    HCELL_INDEX ValueCell;
    ULONG Length;
    PULONG Data;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Open the start cell */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Node) return FALSE;
    ValueCell = CmpFindValueByName(Hive, Node, &Name);
    if (ValueCell == HCELL_NIL) return FALSE;

    /* Read the start value */
    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if (!Value) return FALSE;
    if (Value->Type != REG_DWORD) return FALSE;
    Data = (PULONG)CmpValueToData(Hive, Value, &Length);
    if (!Data) return FALSE;
    if (Length < sizeof(ULONG)) return FALSE;

    /* Return if the type matches */
    return (*Data == LoadType);
}

/**
 * @brief
 * Enumerates all drivers within the given control set and load type,
 * present in the "Services" sub-key, and inserts them into the driver list.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   ControlSet
 * The control set registry key's hive cell.
 *
 * @param[in]   LoadType
 * The load type the driver should match.
 *
 * @param[in]   BootFileSystem
 * Optional name of the boot file system, for which to insert
 * its corresponding driver.
 *
 * @param[in,out]   DriverListHead
 * The driver list where to insert the enumerated drivers.
 *
 * @return
 * TRUE if the drivers have been successfully enumerated and inserted,
 * FALSE if not.
 **/
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpFindDrivers(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX ControlSet,
    _In_ SERVICE_LOAD_TYPE LoadType,
    _In_opt_ PCWSTR BootFileSystem,
    _Inout_ PLIST_ENTRY DriverListHead)
{
    HCELL_INDEX ServicesCell, ControlCell, GroupOrderCell, DriverCell;
    HCELL_INDEX SafeBootCell = HCELL_NIL;
    ULONG i;
    UNICODE_STRING Name;
    UNICODE_STRING KeyPath;
    PCM_KEY_NODE ControlNode, ServicesNode, Node;
    PBOOT_DRIVER_NODE FsNode;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Open the control set key */
    ControlNode = (PCM_KEY_NODE)HvGetCell(Hive, ControlSet);
    if (!ControlNode) return FALSE;

    /* Get services cell */
    RtlInitUnicodeString(&Name, L"Services");
    ServicesCell = CmpFindSubKeyByName(Hive, ControlNode, &Name);
    if (ServicesCell == HCELL_NIL) return FALSE;

    /* Open services key */
    ServicesNode = (PCM_KEY_NODE)HvGetCell(Hive, ServicesCell);
    if (!ServicesNode) return FALSE;

    /* Get control cell */
    RtlInitUnicodeString(&Name, L"Control");
    ControlCell = CmpFindSubKeyByName(Hive, ControlNode, &Name);
    if (ControlCell == HCELL_NIL) return FALSE;

    /* Get the group order cell and read it */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlCell);
    if (!Node) return FALSE;
    RtlInitUnicodeString(&Name, L"GroupOrderList");
    GroupOrderCell = CmpFindSubKeyByName(Hive, Node, &Name);
    if (GroupOrderCell == HCELL_NIL) return FALSE;

    /* Get Safe Boot cell */
    if (InitSafeBootMode)
    {
        /* Open the Safe Boot key */
        RtlInitUnicodeString(&Name, L"SafeBoot");
        Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlCell);
        if (!Node) return FALSE;
        SafeBootCell = CmpFindSubKeyByName(Hive, Node, &Name);
        if (SafeBootCell == HCELL_NIL) return FALSE;

        /* Open the correct start key (depending on the mode) */
        Node = (PCM_KEY_NODE)HvGetCell(Hive, SafeBootCell);
        if (!Node) return FALSE;
        switch (InitSafeBootMode)
        {
            /* NOTE: Assumes MINIMAL (1) and DSREPAIR (3) load same items */
            case 1:
            case 3: RtlInitUnicodeString(&Name, L"Minimal"); break;
            case 2: RtlInitUnicodeString(&Name, L"Network"); break;
            default: return FALSE;
        }
        SafeBootCell = CmpFindSubKeyByName(Hive, Node, &Name);
        if (SafeBootCell == HCELL_NIL) return FALSE;
    }

    /* Build the root registry path */
    RtlInitUnicodeString(&KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");

    /* Enumerate each sub-key */
    i = 0;
    DriverCell = CmpFindSubKeyByNumber(Hive, ServicesNode, i);
    while (DriverCell != HCELL_NIL)
    {
        /* Make sure it's a driver of this start type AND is "safe" to load */
        if (CmpIsLoadType(Hive, DriverCell, LoadType) &&
            CmpIsSafe(Hive, SafeBootCell, DriverCell))
        {
            /* Add it to the list */
            if (!CmpAddDriverToList(Hive,
                                    DriverCell,
                                    GroupOrderCell,
                                    &KeyPath,
                                    DriverListHead))
            {
                CMTRACE(CM_BOOT_DEBUG, "  Failed to add boot driver\n");
            }
        }

        /* Go to the next sub-key */
        DriverCell = CmpFindSubKeyByNumber(Hive, ServicesNode, ++i);
    }

    /* Check if we have a boot file system */
    if (BootFileSystem)
    {
        /* Find it */
        RtlInitUnicodeString(&Name, BootFileSystem);
        DriverCell = CmpFindSubKeyByName(Hive, ServicesNode, &Name);
        if (DriverCell != HCELL_NIL)
        {
            CMTRACE(CM_BOOT_DEBUG, "Adding Boot FileSystem '%S'\n",
                    BootFileSystem);

            /* Always add it to the list */
            if (!CmpAddDriverToList(Hive,
                                    DriverCell,
                                    GroupOrderCell,
                                    &KeyPath,
                                    DriverListHead))
            {
                CMTRACE(CM_BOOT_DEBUG, "  Failed to add boot driver\n");
            }
            else
            {
                /* Mark it as critical so it always loads */
                FsNode = CONTAINING_RECORD(DriverListHead->Flink,
                                           BOOT_DRIVER_NODE,
                                           ListEntry.Link);
                FsNode->ErrorControl = SERVICE_ERROR_CRITICAL;
            }
        }
    }

    /* We're done! */
    return TRUE;
}

/**
 * @brief
 * Performs the driver list sorting, according to the ordering list.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   ControlSet
 * The control set registry key's hive cell.
 *
 * @param[in,out]   DriverListHead
 * The driver list to sort.
 *
 * @return
 * TRUE if sorting has been successfully done, FALSE if not.
 **/
CODE_SEG("INIT")
static
BOOLEAN
CmpDoSort(
    _Inout_ PLIST_ENTRY DriverListHead,
    _In_ PCUNICODE_STRING OrderList)
{
    PWCHAR Current, End = NULL;
    UNICODE_STRING GroupName;
    PLIST_ENTRY NextEntry;
    PBOOT_DRIVER_NODE CurrentNode;

    /* We're going from end to start, so get to the last group and keep going */
    Current = &OrderList->Buffer[OrderList->Length / sizeof(WCHAR)];
    while (Current > OrderList->Buffer)
    {
        /* Scan the current string */
        do
        {
            if (*Current == UNICODE_NULL) End = Current;
        } while ((*(--Current - 1) != UNICODE_NULL) && (Current != OrderList->Buffer));

        /* This is our cleaned up string for this specific group */
        ASSERT(End != NULL);
        GroupName.Length = (USHORT)(End - Current) * sizeof(WCHAR);
        GroupName.MaximumLength = GroupName.Length;
        GroupName.Buffer = Current;

        /* Now loop the driver list */
        NextEntry = DriverListHead->Flink;
        while (NextEntry != DriverListHead)
        {
            /* Get this node */
            CurrentNode = CONTAINING_RECORD(NextEntry,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);

            /* Get the next entry now since we'll do a relink */
            NextEntry = NextEntry->Flink;

            /* Is there a group name and does it match the current group? */
            if (CurrentNode->Group.Buffer &&
                RtlEqualUnicodeString(&GroupName, &CurrentNode->Group, TRUE))
            {
                /* Remove from this location and re-link in the new one */
                RemoveEntryList(&CurrentNode->ListEntry.Link);
                InsertHeadList(DriverListHead, &CurrentNode->ListEntry.Link);
            }
        }

        /* Move on */
        --Current;
    }

    /* All done */
    return TRUE;
}

/**
 * @brief
 * Sorts the driver list, according to the drivers' group load ordering.
 *
 * @param[in]   Hive
 * The SYSTEM hive.
 *
 * @param[in]   ControlSet
 * The control set registry key's hive cell.
 *
 * @param[in,out]   DriverListHead
 * The driver list to sort.
 *
 * @return
 * TRUE if sorting has been successfully done, FALSE if not.
 **/
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpSortDriverList(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX ControlSet,
    _Inout_ PLIST_ENTRY DriverListHead)
{
    PCM_KEY_NODE Node;
    PCM_KEY_VALUE ListValue;
    HCELL_INDEX ControlCell, GroupOrder, ListCell;
    UNICODE_STRING Name, OrderList;
    ULONG Length;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Open the control key */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlSet);
    if (!Node) return FALSE;
    RtlInitUnicodeString(&Name, L"Control");
    ControlCell = CmpFindSubKeyByName(Hive, Node, &Name);
    if (ControlCell == HCELL_NIL) return FALSE;

    /* Open the service group order */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlCell);
    if (!Node) return FALSE;
    RtlInitUnicodeString(&Name, L"ServiceGroupOrder");
    GroupOrder = CmpFindSubKeyByName(Hive, Node, &Name);
    if (GroupOrder == HCELL_NIL) return FALSE;

    /* Open the list key */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, GroupOrder);
    if (!Node) return FALSE;
    RtlInitUnicodeString(&Name, L"list");
    ListCell = CmpFindValueByName(Hive, Node, &Name);
    if (ListCell == HCELL_NIL) return FALSE;

    /* Read the actual list */
    ListValue = (PCM_KEY_VALUE)HvGetCell(Hive, ListCell);
    if (!ListValue) return FALSE;
    if (ListValue->Type != REG_MULTI_SZ) return FALSE;

    /* Copy it into a buffer */
    OrderList.Buffer = (PWCHAR)CmpValueToData(Hive, ListValue, &Length);
    if (!OrderList.Buffer) return FALSE;
    if (!IS_NULL_TERMINATED(OrderList.Buffer, Length)) return FALSE;
    OrderList.Length = (USHORT)Length - sizeof(UNICODE_NULL);
    OrderList.MaximumLength = OrderList.Length;

    /* And start the sort algorithm */
    return CmpDoSort(DriverListHead, &OrderList);
}

CODE_SEG("INIT")
static
BOOLEAN
CmpOrderGroup(
    _In_ PBOOT_DRIVER_NODE StartNode,
    _In_ PBOOT_DRIVER_NODE EndNode)
{
    PBOOT_DRIVER_NODE CurrentNode, PreviousNode;
    PLIST_ENTRY ListEntry;

    /* Base case, nothing to do */
    if (StartNode == EndNode) return TRUE;

    /* Loop the nodes */
    CurrentNode = StartNode;
    do
    {
        /* Save this as the previous node */
        PreviousNode = CurrentNode;

        /* And move to the next one */
        ListEntry = CurrentNode->ListEntry.Link.Flink;
        CurrentNode = CONTAINING_RECORD(ListEntry,
                                        BOOT_DRIVER_NODE,
                                        ListEntry.Link);

        /* Check if the previous driver had a bigger tag */
        if (PreviousNode->Tag > CurrentNode->Tag)
        {
            /* Check if we need to update the tail */
            if (CurrentNode == EndNode)
            {
                /* Update the tail */
                ListEntry = CurrentNode->ListEntry.Link.Blink;
                EndNode = CONTAINING_RECORD(ListEntry,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);
            }

            /* Remove this driver since we need to move it */
            RemoveEntryList(&CurrentNode->ListEntry.Link);

            /* Keep looping until we find a driver with a lower tag than ours */
            while ((PreviousNode->Tag > CurrentNode->Tag) && (PreviousNode != StartNode))
            {
                /* We'll be re-inserted at this spot */
                ListEntry = PreviousNode->ListEntry.Link.Blink;
                PreviousNode = CONTAINING_RECORD(ListEntry,
                                                 BOOT_DRIVER_NODE,
                                                 ListEntry.Link);
            }

            /* Do the insert in the new location */
            InsertTailList(&PreviousNode->ListEntry.Link, &CurrentNode->ListEntry.Link);

            /* Update the head, if needed */
            if (PreviousNode == StartNode) StartNode = CurrentNode;
        }
    } while (CurrentNode != EndNode);

    /* All done */
    return TRUE;
}

/**
 * @brief
 * Removes potential circular dependencies (cycles) and sorts the driver list.
 *
 * @param[in,out]   DriverListHead
 * The driver list to sort.
 *
 * @return
 * Always TRUE.
 **/
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpResolveDriverDependencies(
    _Inout_ PLIST_ENTRY DriverListHead)
{
    PLIST_ENTRY NextEntry;
    PBOOT_DRIVER_NODE StartNode, EndNode, CurrentNode;

    /* Loop the list */
    NextEntry = DriverListHead->Flink;
    while (NextEntry != DriverListHead)
    {
        /* Find the first entry */
        StartNode = CONTAINING_RECORD(NextEntry,
                                      BOOT_DRIVER_NODE,
                                      ListEntry.Link);
        do
        {
            /* Find the last entry */
            EndNode = CONTAINING_RECORD(NextEntry,
                                        BOOT_DRIVER_NODE,
                                        ListEntry.Link);

            /* Get the next entry */
            NextEntry = NextEntry->Flink;
            CurrentNode = CONTAINING_RECORD(NextEntry,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);

            /* If the next entry is back to the top, break out */
            if (NextEntry == DriverListHead) break;

            /* Otherwise, check if this entry is equal */
            if (!RtlEqualUnicodeString(&StartNode->Group,
                                       &CurrentNode->Group,
                                       TRUE))
            {
                /* It is, so we've detected a cycle, break out */
                break;
            }
        } while (NextEntry != DriverListHead);

        /* Now we have the correct start and end pointers, so do the sort */
        CmpOrderGroup(StartNode, EndNode);
    }

    /* We're done */
    return TRUE;
}

CODE_SEG("INIT")
static
BOOLEAN
CmpIsSafe(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX SafeBootCell,
    _In_ HCELL_INDEX DriverCell)
{
    PCM_KEY_NODE SafeBootNode;
    PCM_KEY_NODE DriverNode;
    PCM_KEY_VALUE KeyValue;
    HCELL_INDEX CellIndex;
    ULONG Length;
    UNICODE_STRING Name;
    PWCHAR Buffer;

    /* Sanity check: We shouldn't need to release any acquired cells */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Driver key node (mandatory) */
    ASSERT(DriverCell != HCELL_NIL);
    DriverNode = (PCM_KEY_NODE)HvGetCell(Hive, DriverCell);
    if (!DriverNode) return FALSE;

    /* Safe boot key node (optional but return TRUE if not present) */
    if (SafeBootCell == HCELL_NIL) return TRUE;
    SafeBootNode = (PCM_KEY_NODE)HvGetCell(Hive, SafeBootCell);
    if (!SafeBootNode) return FALSE;

    /* Search by the name from the group */
    RtlInitUnicodeString(&Name, L"Group");
    CellIndex = CmpFindValueByName(Hive, DriverNode, &Name);
    if (CellIndex != HCELL_NIL)
    {
        KeyValue = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
        if (!KeyValue) return FALSE;

        if (KeyValue->Type == REG_SZ) // REG_EXPAND_SZ not really allowed there.
        {
            /* Compose the search 'key' */
            Buffer = (PWCHAR)CmpValueToData(Hive, KeyValue, &Length);
            if (!Buffer)
                return FALSE;
            if (IS_NULL_TERMINATED(Buffer, Length))
                Length -= sizeof(UNICODE_NULL);

            Name.Buffer = Buffer;
            Name.Length = (USHORT)Length;
            Name.MaximumLength = Name.Length;

            /* Search for corresponding key in the Safe Boot key */
            CellIndex = CmpFindSubKeyByName(Hive, SafeBootNode, &Name);
            if (CellIndex != HCELL_NIL) return TRUE;
        }
    }

    /* Group has not been found - find driver name */
    Length = (DriverNode->Flags & KEY_COMP_NAME) ?
             CmpCompressedNameSize(DriverNode->Name, DriverNode->NameLength) :
             DriverNode->NameLength;
    if (Length < sizeof(WCHAR))
        return FALSE;

    /* Now allocate the buffer for it and copy the name */
    RtlInitEmptyUnicodeString(&Name,
                              Hive->Allocate(Length, FALSE, TAG_CM),
                              (USHORT)Length);
    if (!Name.Buffer)
        return FALSE;

    Name.Length = (USHORT)Length;
    if (DriverNode->Flags & KEY_COMP_NAME)
    {
        /* Compressed name */
        CmpCopyCompressedName(Name.Buffer,
                              Name.Length,
                              DriverNode->Name,
                              DriverNode->NameLength);
    }
    else
    {
        /* Normal name */
        RtlCopyMemory(Name.Buffer, DriverNode->Name, DriverNode->NameLength);
    }

    CellIndex = CmpFindSubKeyByName(Hive, SafeBootNode, &Name);
    Hive->Free(Name.Buffer, Name.MaximumLength);
    if (CellIndex != HCELL_NIL) return TRUE;

    /* Not group or driver name - search by image name */
    RtlInitUnicodeString(&Name, L"ImagePath");
    CellIndex = CmpFindValueByName(Hive, DriverNode, &Name);
    if (CellIndex != HCELL_NIL)
    {
        KeyValue = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
        if (!KeyValue) return FALSE;

        if ((KeyValue->Type == REG_SZ) || (KeyValue->Type == REG_EXPAND_SZ))
        {
            /* Compose the search 'key' */
            Buffer = (PWCHAR)CmpValueToData(Hive, KeyValue, &Length);
            if (!Buffer) return FALSE;
            if (Length < sizeof(WCHAR)) return FALSE;

            /* Get the base image file name */
            // FIXME: wcsrchr() may fail if Buffer is *not* NULL-terminated!
            Name.Buffer = wcsrchr(Buffer, OBJ_NAME_PATH_SEPARATOR);
            if (!Name.Buffer) return FALSE;
            ++Name.Buffer;

            /* Length of the base name must be >=1 WCHAR */
            if (((ULONG_PTR)Name.Buffer - (ULONG_PTR)Buffer) >= Length)
                return FALSE;
            Length -= ((ULONG_PTR)Name.Buffer - (ULONG_PTR)Buffer);
            if (IS_NULL_TERMINATED(Name.Buffer, Length))
                Length -= sizeof(UNICODE_NULL);
            if (Length < sizeof(WCHAR)) return FALSE;

            Name.Length = (USHORT)Length;
            Name.MaximumLength = Name.Length;

            /* Search for corresponding key in the Safe Boot key */
            CellIndex = CmpFindSubKeyByName(Hive, SafeBootNode, &Name);
            if (CellIndex != HCELL_NIL) return TRUE;
        }
    }

    /* Nothing found - nothing else to search */
    return FALSE;
}

/**
 * @brief
 * Empties the driver list and frees all allocated driver nodes in it.
 *
 * @param[in]   Hive
 * The SYSTEM hive (used only for the Hive->Free() memory deallocator).
 *
 * @param[in,out]   DriverListHead
 * The driver list to free.
 *
 * @return  None
 **/
CODE_SEG("INIT")
VOID
NTAPI
CmpFreeDriverList(
    _In_ PHHIVE Hive,
    _Inout_ PLIST_ENTRY DriverListHead)
{
    PLIST_ENTRY Entry;
    PBOOT_DRIVER_NODE DriverNode;

    /* Loop through the list and remove each driver node */
    while (!IsListEmpty(DriverListHead))
    {
        /* Get the driver node */
        Entry = RemoveHeadList(DriverListHead);
        DriverNode = CONTAINING_RECORD(Entry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);

        /* Free any allocated string buffers, then the node */
        if (DriverNode->ListEntry.RegistryPath.Buffer)
        {
            Hive->Free(DriverNode->ListEntry.RegistryPath.Buffer,
                       DriverNode->ListEntry.RegistryPath.MaximumLength);
        }
        if (DriverNode->ListEntry.FilePath.Buffer)
        {
            Hive->Free(DriverNode->ListEntry.FilePath.Buffer,
                       DriverNode->ListEntry.FilePath.MaximumLength);
        }
        if (DriverNode->Name.Buffer)
        {
            Hive->Free(DriverNode->Name.Buffer,
                       DriverNode->Name.MaximumLength);
        }
        Hive->Free(DriverNode, sizeof(BOOT_DRIVER_NODE));
    }
}

/* EOF */
