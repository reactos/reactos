/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/config/cmboot.c
 * PURPOSE:         Configuration Manager - Boot Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

extern ULONG InitSafeBootMode;

/* FUNCTIONS ******************************************************************/

HCELL_INDEX
NTAPI
INIT_FUNCTION
CmpFindControlSet(IN PHHIVE SystemHive,
                  IN HCELL_INDEX RootCell,
                  IN PUNICODE_STRING SelectKeyName,
                  OUT PBOOLEAN AutoSelect)
{
    UNICODE_STRING KeyName;
    PCM_KEY_NODE Node;
    HCELL_INDEX SelectCell, AutoSelectCell, SelectValueCell, ControlSetCell;
    HCELL_INDEX CurrentValueCell;
    PCM_KEY_VALUE KeyValue;
    ULONG Length;
    PULONG ControlSetId;
    ANSI_STRING ControlSetAnsiName;
    CHAR Buffer[128];
    WCHAR WideBuffer[128];
    NTSTATUS Status;
    PULONG CurrentData;

    /* Sanity check */
    ASSERT(SystemHive->ReleaseCellRoutine == NULL);

    /* Get the Select subkey */
    RtlInitUnicodeString(&KeyName, L"select");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    SelectCell = CmpFindSubKeyByName(SystemHive, Node, &KeyName);
    if (SelectCell == HCELL_NIL) return SelectCell;

    /* Get AutoSelect value */
    RtlInitUnicodeString(&KeyName, L"AutoSelect");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    AutoSelectCell = CmpFindValueByName(SystemHive, Node, &KeyName);
    if (AutoSelectCell == HCELL_NIL)
    {
        /* Assume TRUE if the value is missing. */
        *AutoSelect = TRUE;
    }
    else
    {
        /* Read the value */
        KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, AutoSelectCell);
        if (KeyValue == NULL) return HCELL_NIL;

        /* Convert it to a boolean */
        *AutoSelect = *(PBOOLEAN)CmpValueToData(SystemHive, KeyValue, &Length);
    }

    /* Now find the control set being looked up */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    SelectValueCell = CmpFindValueByName(SystemHive, Node, SelectKeyName);
    if (SelectValueCell == HCELL_NIL) return SelectValueCell;

    /* Read the value (corresponding to the CCS ID) */
    KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, SelectValueCell);
    if (!KeyValue) return HCELL_NIL;
    if (KeyValue->Type != REG_DWORD) return HCELL_NIL;
    ControlSetId = (PULONG)CmpValueToData(SystemHive, KeyValue, &Length);

    /* Now build an Ansi String for the CCS's Name */
    sprintf(Buffer, "ControlSet%03lu", *ControlSetId);
    ControlSetAnsiName.Length = (USHORT)strlen(Buffer);
    ControlSetAnsiName.MaximumLength = (USHORT)strlen(Buffer);
    ControlSetAnsiName.Buffer = Buffer;

    /* And convert it to Unicode... */
    KeyName.MaximumLength = 256;
    KeyName.Buffer = WideBuffer;
    Status = RtlAnsiStringToUnicodeString(&KeyName,
                                          &ControlSetAnsiName,
                                          FALSE);
    if (!NT_SUCCESS(Status)) return HCELL_NIL;

    /* Now open it */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    ControlSetCell = CmpFindSubKeyByName(SystemHive, Node, &KeyName);
    if (ControlSetCell == HCELL_NIL) return ControlSetCell;

    /* Get the value of the "Current" CCS */
    RtlInitUnicodeString(&KeyName, L"Current");
    Node =  (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    CurrentValueCell = CmpFindValueByName(SystemHive, Node, &KeyName);

    /* Make sure it exists */
    if (CurrentValueCell != HCELL_NIL)
    {
        /* Get the current value and make sure its a ULONG */
        KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, CurrentValueCell);
        if (!KeyValue) return HCELL_NIL;
        if (KeyValue->Type == REG_DWORD)
        {
            /* Get the data and update it */
            CurrentData = (PULONG)CmpValueToData(SystemHive,
                                                 KeyValue,
                                                 &Length);
            if (!CurrentData) return HCELL_NIL;
            *CurrentData = *ControlSetId;
        }
    }

    /* Return the CCS Cell */
    return ControlSetCell;
}

ULONG
NTAPI
INIT_FUNCTION
CmpFindTagIndex(IN PHHIVE Hive,
                IN HCELL_INDEX TagCell,
                IN HCELL_INDEX GroupOrderCell,
                IN PUNICODE_STRING GroupName)
{
    PCM_KEY_VALUE TagValue, Value;
    HCELL_INDEX OrderCell;
    PULONG TagOrder, DriverTag;
    ULONG CurrentTag, Length;
    PCM_KEY_NODE Node;
    BOOLEAN BufferAllocated;
    ASSERT(Hive->ReleaseCellRoutine == NULL);
    
    /* Get the tag */
    Value = HvGetCell(Hive, TagCell);
    ASSERT(Value);
    DriverTag = (PULONG)CmpValueToData(Hive, Value, &Length);
    ASSERT(DriverTag);

    /* Get the order array */
    Node = HvGetCell(Hive, GroupOrderCell);
    ASSERT(Node);
    OrderCell = CmpFindValueByName(Hive, Node, GroupName);
    if (OrderCell == HCELL_NIL) return -2;
    
    /* And read it */
    TagValue = HvGetCell(Hive, OrderCell);
    CmpGetValueData(Hive, TagValue, &Length, (PVOID*)&TagOrder, &BufferAllocated, &OrderCell);
    ASSERT(TagOrder);
    
    /* Parse each tag */
    for (CurrentTag = 1; CurrentTag <= TagOrder[0]; CurrentTag++)
    {
        /* Find a match */
        if (TagOrder[CurrentTag] == *DriverTag)
        {
            /* Found it -- return the tag */
            if (BufferAllocated) ExFreePool(TagOrder);
            return CurrentTag;
        }
    }
    
    /* No matches, so assume next to last ordering */
    if (BufferAllocated) ExFreePool(TagOrder);
    return -2;
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpAddDriverToList(IN PHHIVE Hive,
                   IN HCELL_INDEX DriverCell,
                   IN HCELL_INDEX GroupOrderCell,
                   IN PUNICODE_STRING RegistryPath,
                   IN PLIST_ENTRY BootDriverListHead)
{
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    PCM_KEY_NODE Node;
    ULONG Length;
    USHORT NameLength;
    HCELL_INDEX ValueCell, TagCell;    PCM_KEY_VALUE Value;
    PUNICODE_STRING FileName, RegistryString;
    UNICODE_STRING UnicodeString;
    PULONG ErrorControl;
    PWCHAR Buffer;
    ASSERT(Hive->ReleaseCellRoutine == NULL);
    
    /* Allocate a driver node and initialize it */
    DriverNode = CmpAllocate(sizeof(BOOT_DRIVER_NODE), FALSE, TAG_CM);
    if (!DriverNode) return FALSE;
    DriverEntry = &DriverNode->ListEntry;
    DriverEntry->RegistryPath.Buffer = NULL;
    DriverEntry->FilePath.Buffer = NULL;
    
    /* Get the driver cell */
    Node = HvGetCell(Hive, DriverCell);
    ASSERT(Node);
    
    /* Get the name from the cell */
    DriverNode->Name.Length = Node->Flags & KEY_COMP_NAME ?
                              CmpCompressedNameSize(Node->Name, Node->NameLength) :
                              Node->NameLength;
    DriverNode->Name.MaximumLength = DriverNode->Name.Length;
    NameLength = DriverNode->Name.Length;
    
    /* Now allocate the buffer for it and copy the name */
    DriverNode->Name.Buffer = CmpAllocate(NameLength, FALSE, TAG_CM);
    if (!DriverNode->Name.Buffer) return FALSE;
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
    RtlInitUnicodeString(&UnicodeString, L"ImagePath");
    ValueCell = CmpFindValueByName(Hive, Node, &UnicodeString);
    if (ValueCell == HCELL_NIL)
    {
        /* Couldn't find it, so assume the drivers path */
        Length = sizeof(L"System32\\Drivers\\") + NameLength + sizeof(L".sys");
        
        /* Allocate the path name */
        FileName = &DriverEntry->FilePath;
        FileName->Length = 0;
        FileName->MaximumLength = (USHORT)Length;
        FileName->Buffer = CmpAllocate(Length, FALSE,TAG_CM);
        if (!FileName->Buffer) return FALSE;

        /* Write the path name */
        RtlAppendUnicodeToString(FileName, L"System32\\Drivers\\");
        RtlAppendUnicodeStringToString(FileName, &DriverNode->Name);
        RtlAppendUnicodeToString(FileName, L".sys");
    }
    else
    {
        /* Path name exists, so grab it */
        Value = HvGetCell(Hive, ValueCell);
        ASSERT(Value);

        /* Allocate and setup the path name */
        FileName = &DriverEntry->FilePath;
        Buffer = (PWCHAR)CmpValueToData(Hive, Value, &Length);
        FileName->MaximumLength = FileName->Length = (USHORT)Length;
        FileName->Buffer = CmpAllocate(Length, FALSE, TAG_CM);
        
        /* Transfer the data */
        if (!(FileName->Buffer) || !(Buffer)) return FALSE;
        RtlCopyMemory(FileName->Buffer, Buffer, Length);
    }
    
    /* Now build the registry path */
    RegistryString = &DriverEntry->RegistryPath;
    RegistryString->Length = 0;
    RegistryString->MaximumLength = RegistryPath->Length + NameLength;
    RegistryString->Buffer = CmpAllocate(RegistryString->MaximumLength, FALSE, TAG_CM);
    if (!RegistryString->Buffer) return FALSE;
    
    /* Add the driver name to it */
    RtlAppendUnicodeStringToString(RegistryString, RegistryPath);
    RtlAppendUnicodeStringToString(RegistryString, &DriverNode->Name);
    
    /* The entry is done, add it */
    InsertHeadList(BootDriverListHead, &DriverEntry->Link);
    
    /* Now find error control settings */
    RtlInitUnicodeString(&UnicodeString, L"ErrorControl");
    ValueCell = CmpFindValueByName(Hive, Node, &UnicodeString);
    if (ValueCell == HCELL_NIL)
    {
        /* Couldn't find it, so assume default */
        DriverNode->ErrorControl = NormalError;
    }
    else
    {
        /* Otherwise, read whatever the data says */
        Value = HvGetCell(Hive, ValueCell);
        ASSERT(Value);
        ErrorControl = (PULONG)CmpValueToData(Hive, Value, &Length);
        ASSERT(ErrorControl);
        DriverNode->ErrorControl = *ErrorControl;
    }
    
    /* Next, get the group cell */
    RtlInitUnicodeString(&UnicodeString, L"group");
    ValueCell = CmpFindValueByName(Hive, Node, &UnicodeString);
    if (ValueCell == HCELL_NIL)
    {
        /* Couldn't find, so set an empty string */
        RtlInitEmptyUnicodeString(&DriverNode->Group, NULL, 0);
    }
    else
    {
        /* Found it, read the group value */
        Value = HvGetCell(Hive, ValueCell);
        ASSERT(Value);
        
        /* Copy it into the node */
        DriverNode->Group.Buffer = (PWCHAR)CmpValueToData(Hive, Value, &Length);
        if (!DriverNode->Group.Buffer) return FALSE;
        DriverNode->Group.Length = (USHORT)Length - sizeof(UNICODE_NULL);
        DriverNode->Group.MaximumLength = DriverNode->Group.Length;
    }
    
    /* Finally, find the tag */
    RtlInitUnicodeString(&UnicodeString, L"Tag");
    TagCell = CmpFindValueByName(Hive, Node, &UnicodeString);
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
    
    /* All done! */
    return TRUE;
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpIsLoadType(IN PHHIVE Hive,
              IN HCELL_INDEX Cell,
              IN SERVICE_LOAD_TYPE LoadType)
{
    PCM_KEY_NODE Node;
    HCELL_INDEX ValueCell;
    UNICODE_STRING ValueString = RTL_CONSTANT_STRING(L"Start");
    PCM_KEY_VALUE Value;
    ULONG Length;
    PLONG Data;
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Open the start cell */
    Node = HvGetCell(Hive, Cell);
    ASSERT(Node);
    ValueCell = CmpFindValueByName(Hive, Node, &ValueString);
    if (ValueCell == HCELL_NIL) return FALSE;
    
    /* Read the start value */
    Value = HvGetCell(Hive, ValueCell);
    ASSERT(Value);
    Data = (PLONG)CmpValueToData(Hive, Value, &Length);
    ASSERT(Data);
    
    /* Return if the type matches */
    return (*Data == LoadType);
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpFindDrivers(IN PHHIVE Hive,
               IN HCELL_INDEX ControlSet,
               IN SERVICE_LOAD_TYPE LoadType,
               IN PWCHAR BootFileSystem OPTIONAL,
               IN PLIST_ENTRY DriverListHead)
{
    HCELL_INDEX ServicesCell, ControlCell, GroupOrderCell, DriverCell;
    HCELL_INDEX SafeBootCell = HCELL_NIL;
    UNICODE_STRING Name;
    ULONG i;
    WCHAR Buffer[128];
    UNICODE_STRING UnicodeString, KeyPath;
    PBOOT_DRIVER_NODE FsNode;
    PCM_KEY_NODE ControlNode, ServicesNode, Node;
    ASSERT(Hive->ReleaseCellRoutine == NULL);
    
    /* Open the control set key */
    ControlNode = HvGetCell(Hive, ControlSet);
    ASSERT(ControlNode);
    
    /* Get services cell */
    RtlInitUnicodeString(&Name, L"Services");
    ServicesCell = CmpFindSubKeyByName(Hive, ControlNode, &Name);
    if (ServicesCell == HCELL_NIL) return FALSE;

    /* Open services key */
    ServicesNode = HvGetCell(Hive, ServicesCell);
    ASSERT(ServicesNode);
    
    /* Get control cell */
    RtlInitUnicodeString(&Name, L"Control");
    ControlCell = CmpFindSubKeyByName(Hive, ControlNode, &Name);
    if (ControlCell == HCELL_NIL) return FALSE;
    
    /* Get the group order cell and read it */
    RtlInitUnicodeString(&Name, L"GroupOrderList");
    Node = HvGetCell(Hive, ControlCell);
    ASSERT(Node);
    GroupOrderCell = CmpFindSubKeyByName(Hive, Node, &Name);
    if (GroupOrderCell == HCELL_NIL) return FALSE;

    /* Get Safe Boot cell */
    if(InitSafeBootMode)
    {
        /* Open the Safe Boot key */
        RtlInitUnicodeString(&Name, L"SafeBoot");
        Node = HvGetCell(Hive, ControlCell);
        ASSERT(Node);
        SafeBootCell = CmpFindSubKeyByName(Hive, Node, &Name);
        if (SafeBootCell == HCELL_NIL) return FALSE;

        /* Open the correct start key (depending on the mode) */
        Node = HvGetCell(Hive, SafeBootCell);
        ASSERT(Node);
        switch(InitSafeBootMode)
        {
            /* NOTE: Assumes MINIMAL (1) and DSREPAIR (3) load same items */
            case 1:
            case 3: RtlInitUnicodeString(&Name, L"Minimal"); break;
            case 2: RtlInitUnicodeString(&Name, L"Network"); break;
            default: return FALSE;
        }
        SafeBootCell = CmpFindSubKeyByName(Hive, Node, &Name);
        if(SafeBootCell == HCELL_NIL) return FALSE;
    }

    /* Build the root registry path */
    RtlInitEmptyUnicodeString(&KeyPath, Buffer, sizeof(Buffer));
    RtlAppendUnicodeToString(&KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    
    /* Find the first subkey (ie: the first driver or service) */
    i = 0;
    DriverCell = CmpFindSubKeyByNumber(Hive, ServicesNode, i);
    while (DriverCell != HCELL_NIL)
    {
        /* Make sure it's a driver of this start type AND is "safe" to load */
        if (CmpIsLoadType(Hive, DriverCell, LoadType) &&
            CmpIsSafe(Hive, SafeBootCell, DriverCell))
        {
            /* Add it to the list */
            CmpAddDriverToList(Hive,
                               DriverCell,
                               GroupOrderCell,
                               &KeyPath,
                               DriverListHead);
            
        }
        
        /* Try the next subkey */
        DriverCell = CmpFindSubKeyByNumber(Hive, ServicesNode, ++i);
    }
    
    /* Check if we have a boot file system */
    if (BootFileSystem)
    {
        /* Find it */
        RtlInitUnicodeString(&UnicodeString, BootFileSystem);
        DriverCell = CmpFindSubKeyByName(Hive, ServicesNode, &UnicodeString);
        if (DriverCell != HCELL_NIL)
        {
            /* Always add it to the list */
            CmpAddDriverToList(Hive,
                               DriverCell,
                               GroupOrderCell,
                               &KeyPath,
                               DriverListHead);
            
            /* Mark it as critical so it always loads */
            FsNode = CONTAINING_RECORD(DriverListHead->Flink,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);
            FsNode->ErrorControl = SERVICE_ERROR_CRITICAL;
        }
    }
    
    /* We're done! */
    return TRUE;
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpDoSort(IN PLIST_ENTRY DriverListHead,
          IN PUNICODE_STRING OrderList)
{
    PWCHAR Current, End = NULL;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING GroupName;
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
            NextEntry = CurrentNode->ListEntry.Link.Flink;
            
            /* Is there a group name and does it match the current group? */
            if ((CurrentNode->Group.Buffer) &&
                (RtlEqualUnicodeString(&GroupName, &CurrentNode->Group, TRUE)))
            {
                /* Remove from this location and re-link in the new one */
                RemoveEntryList(&CurrentNode->ListEntry.Link);
                InsertHeadList(DriverListHead, &CurrentNode->ListEntry.Link);
            }
        }
        
        /* Move on */
        Current--;
    }
    
    /* All done */
    return TRUE;
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpSortDriverList(IN PHHIVE Hive,
                  IN HCELL_INDEX ControlSet,
                  IN PLIST_ENTRY DriverListHead)
{
    HCELL_INDEX Controls, GroupOrder, ListCell;
    UNICODE_STRING Name, DependList;
    PCM_KEY_VALUE ListNode;
    ULONG Length;
    PCM_KEY_NODE Node;
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Open the control key */
    Node = HvGetCell(Hive, ControlSet);
    ASSERT(Node);
    RtlInitUnicodeString(&Name, L"Control");
    Controls = CmpFindSubKeyByName(Hive, Node, &Name);
    if (Controls == HCELL_NIL) return FALSE;

    /* Open the service group order */
    Node = HvGetCell(Hive, Controls);
    ASSERT(Node);
    RtlInitUnicodeString(&Name, L"ServiceGroupOrder");
    GroupOrder = CmpFindSubKeyByName(Hive, Node, &Name);
    if (GroupOrder == HCELL_NIL) return FALSE;

    /* Open the list key */
    Node = HvGetCell(Hive, GroupOrder);
    ASSERT(Node);
    RtlInitUnicodeString(&Name, L"list");
    ListCell = CmpFindValueByName(Hive, Node, &Name);
    if (ListCell == HCELL_NIL) return FALSE;
    
    /* Now read the actual list */
    ListNode = HvGetCell(Hive, ListCell);
    ASSERT(ListNode);
    if (ListNode->Type != REG_MULTI_SZ) return FALSE;
    
    /* Copy it into a buffer */
    DependList.Buffer = (PWCHAR)CmpValueToData(Hive, ListNode, &Length);
    if (!DependList.Buffer) return FALSE;
    DependList.Length = DependList.MaximumLength = (USHORT)Length - sizeof(UNICODE_NULL);
    
    /* And start the recurive sort algorithm */
    return CmpDoSort(DriverListHead, &DependList);
}

BOOLEAN
NTAPI
INIT_FUNCTION
CmpOrderGroup(IN PBOOT_DRIVER_NODE StartNode,
              IN PBOOT_DRIVER_NODE EndNode)
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

BOOLEAN
NTAPI
INIT_FUNCTION
CmpResolveDriverDependencies(IN PLIST_ENTRY DriverListHead)
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

BOOLEAN
NTAPI
INIT_FUNCTION
CmpIsSafe(IN PHHIVE Hive,
          IN HCELL_INDEX SafeBootCell,
          IN HCELL_INDEX DriverCell)
{
    PCM_KEY_NODE SafeBootNode;
    PCM_KEY_NODE DriverNode;
    PCM_KEY_VALUE KeyValue;
    HCELL_INDEX CellIndex;
    ULONG Length = 0;
    UNICODE_STRING Name;
    PWCHAR OriginalName;
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Driver key node (mandatory) */
    ASSERT(DriverCell != HCELL_NIL);
    DriverNode = HvGetCell(Hive, DriverCell);
    ASSERT(DriverNode);

    /* Safe boot key node (optional but return TRUE if not present) */
    if(SafeBootCell == HCELL_NIL) return TRUE;
    SafeBootNode = HvGetCell(Hive, SafeBootCell);
    if(!SafeBootNode) return FALSE;

    /* Search by the name from the group */
    RtlInitUnicodeString(&Name, L"Group");
    CellIndex = CmpFindValueByName(Hive, DriverNode, &Name);
    if(CellIndex != HCELL_NIL)
    {
        KeyValue = HvGetCell(Hive, CellIndex);
        ASSERT(KeyValue);
        if (KeyValue->Type == REG_SZ || KeyValue->Type == REG_EXPAND_SZ)
        {
            /* Compose the search 'key' */
            Name.Buffer = (PWCHAR)CmpValueToData(Hive, KeyValue, &Length);
            if (!Name.Buffer) return FALSE;
            Name.Length = (USHORT)Length - sizeof(UNICODE_NULL);
            Name.MaximumLength = Name.Length;
            /* Search for corresponding key in the Safe Boot key */
            CellIndex = CmpFindSubKeyByName(Hive, SafeBootNode, &Name);
            if(CellIndex != HCELL_NIL) return TRUE;
        }
    }

    /* Group has not been found - find driver name */
    Name.Length = DriverNode->Flags & KEY_COMP_NAME ?
                              CmpCompressedNameSize(DriverNode->Name,
                                                    DriverNode->NameLength) :
                              DriverNode->NameLength;
    Name.MaximumLength = Name.Length;
    /* Now allocate the buffer for it and copy the name */
    Name.Buffer = CmpAllocate(Name.Length, FALSE, TAG_CM);
    if (!Name.Buffer) return FALSE;
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
    RtlFreeUnicodeString(&Name);
    if(CellIndex != HCELL_NIL) return TRUE;

    /* Not group or driver name - search by image name */
    RtlInitUnicodeString(&Name, L"ImagePath");
    CellIndex = CmpFindValueByName(Hive, DriverNode, &Name);
    if(CellIndex != HCELL_NIL)
    {
        KeyValue = HvGetCell(Hive, CellIndex);
        ASSERT(KeyValue);
        if (KeyValue->Type == REG_SZ || KeyValue->Type == REG_EXPAND_SZ)
        {
            /* Compose the search 'key' */
            OriginalName = (PWCHAR)CmpValueToData(Hive, KeyValue, &Length);
            if (!OriginalName) return FALSE;
            /* Get the base image file name */
            Name.Buffer = wcsrchr(OriginalName, L'\\');
            if (!Name.Buffer) return FALSE;
            ++Name.Buffer;
            /* Length of the base name must be >=1 */
            Name.Length = (USHORT)Length - (USHORT)((PUCHAR)Name.Buffer - (PUCHAR)OriginalName)
                                 - sizeof(UNICODE_NULL);
            if(Name.Length < 1) return FALSE;
            Name.MaximumLength = Name.Length;
            /* Search for corresponding key in the Safe Boot key */
            CellIndex = CmpFindSubKeyByName(Hive, SafeBootNode, &Name);
            if(CellIndex != HCELL_NIL) return TRUE;
        }
    }
    /* Nothing found - nothing else to search */
    return FALSE;
}

/* EOF */
