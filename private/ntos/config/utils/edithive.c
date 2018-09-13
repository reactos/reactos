/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    edithive.c

Abstract:

    Provides functionality for dumping and editing registry hive files from
    user-mode.

Author:

    John Vert (jvert) 26-Mar-1992

Revision History:

--*/
#include "edithive.h"
#include "nturtl.h"

extern GENERIC_MAPPING CmpKeyMapping;
extern LIST_ENTRY CmpHiveListHead;            // List of CMHIVEs

#define SECURITY_CELL_LENGTH(pDescriptor) \
    FIELD_OFFSET(CM_KEY_SECURITY,Descriptor) + \
    RtlLengthSecurityDescriptor(pDescriptor)


NTSTATUS
EhpAttachSecurity(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    );

PVOID
EhpAllocate(
    ULONG   Size
    );

VOID
EhpFree(
    PVOID   MemoryBlock
    );

BOOLEAN
EhpFileRead (
    HANDLE      FileHandle,
    PULONG      FileOffset,
    PVOID       DataBuffer,
    ULONG       DataLength
    );

BOOLEAN
EhpFileWrite(
    HANDLE      FileHandle,
    PULONG      FileOffset,
    PVOID       DataBuffer,
    ULONG       DataLength
    );

BOOLEAN
EhpFileFlush (
    HANDLE      FileHandle
    );

VOID
CmpGetObjectSecurity(
    IN HCELL_INDEX Cell,
    IN PHHIVE Hive,
    OUT PCM_KEY_SECURITY *Security,
    OUT PHCELL_INDEX SecurityCell OPTIONAL
    )

/*++

Routine Description:

    This routine maps in the security cell of a registry object.

Arguments:

    Cell - Supplies the cell index of the object.

    Hive - Supplies the hive the object's cell is in.

    Security - Returns a pointer to the security cell of the object.

    SecurityCell - Returns the index of the security cell

Return Value:

    NONE.

--*/

{
    HCELL_INDEX CellIndex;
    PCM_KEY_NODE Node;

    //
    // Map the node we need to get the security descriptor for
    //
    Node = (PCM_KEY_NODE) HvGetCell(Hive, Cell);

    CellIndex = Node->u1.s1.Security;

    //
    // Map in the security descriptor cell
    //
    *Security = (PCM_KEY_SECURITY) HvGetCell(Hive, CellIndex);

    if (ARGUMENT_PRESENT(SecurityCell)) {
        *SecurityCell = CellIndex;
    }

    return;
}


VOID
EhCloseHive(
    IN HANDLE HiveHandle
    )

/*++

Routine Description:

    Closes a hive, including writing all the data out to disk and freeing
    the relevant structures.

Arguments:

    HiveHandle - Supplies a handle to the hive control structure

Return Value:

    None.

--*/

{
    HvSyncHive((PHHIVE)HiveHandle);
    NtClose(((PCMHIVE)HiveHandle)->FileHandles[HFILE_TYPE_PRIMARY]);
    NtClose(((PCMHIVE)HiveHandle)->FileHandles[HFILE_TYPE_ALTERNATE]);
    NtClose(((PCMHIVE)HiveHandle)->FileHandles[HFILE_TYPE_LOG]);
    CmpFree((PCMHIVE)HiveHandle, sizeof(CMHIVE));

}


HANDLE
EhOpenHive(
    IN PUNICODE_STRING FileName,
    OUT PHANDLE RootHandle,
    IN PUNICODE_STRING RootName,
    IN ULONG HiveType
    )

/*++

Routine Description:

    Opens an existing hive.  If the filename does not exist it will be
    created.

    WARNING:    Allocate FileName large enough to acomodate .log or
                .alt extension.

Arguments:

    FileName - Supplies the NULL-terminated filename to open as a hive.

    HiveType - TYPE_SIMPLE = no log or alternate
               TYPE_LOG = log
               TYPE_ALT = alternate

Return Value:

    != NULL - Handle to the opened hive.
    == NULL - Indicates file could not be opened.

--*/

{
    NTSTATUS Status;
    HANDLE File;
    BOOLEAN Allocate;
    PCMHIVE Hive;
    IO_STATUS_BLOCK IoStatus;
    ULONG CreateDisposition;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCM_KEY_NODE RootNode;
    HANDLE Handle;
    HANDLE  Primary;
    HANDLE  Log;
    HANDLE  Alt;
    ULONG   FileType;
    ULONG   Disposition;
    ULONG   SecondaryDisposition;
    ULONG   Operation;

    Alt = NULL;
    Log = NULL;
    InitializeListHead(&CmpHiveListHead);

    switch (HiveType) {
    case TYPE_SIMPLE:
        Status = CmpOpenHiveFiles(
                    FileName,
                    NULL,
                    &Primary,
                    NULL,
                    &Disposition,
                    &SecondaryDisposition,
                    TRUE,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }
        FileType = HFILE_TYPE_PRIMARY;
        break;

    case TYPE_LOG:
        Status = CmpOpenHiveFiles(
                    FileName,
                    L".log",
                    &Primary,
                    &Log,
                    &Disposition,
                    &SecondaryDisposition,
                    TRUE,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }
        if (Log == NULL) {
            return NULL;
        }
        FileType = HFILE_TYPE_LOG;
        break;

    case TYPE_ALT:
        Status = CmpOpenHiveFiles(
                    FileName,
                    L".alt",
                    &Primary,
                    &Alt,
                    &Disposition,
                    &SecondaryDisposition,
                    TRUE,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }
        if (Alt == NULL) {
            return NULL;
        }
        FileType = HFILE_TYPE_ALTERNATE;
        break;

    default:
        return NULL;
    }

    //
    // Initialize hive
    //
    if (Disposition == FILE_CREATED) {
        Operation = HINIT_CREATE;
        Allocate = TRUE;
    } else {
        Operation = HINIT_FILE;
        Allocate = FALSE;
    }

    if ( ! CmpInitializeHive(
                    &Hive,
                    Operation,
                    FALSE,
                    FileType,
                    NULL,
                    Primary,
                    Alt,
                    Log,
                    NULL,
                    NULL
                    ))
    {
        return NULL;
    }

    if (!Allocate) {
        *RootHandle = (HANDLE)(Hive->Hive.BaseBlock->RootCell);
        RootNode = (PCM_KEY_NODE)HvGetCell(
                                (PHHIVE)Hive, Hive->Hive.BaseBlock->RootCell);
        RootName->Length = RootName->MaximumLength = RootNode->NameLength;
        RootName->Buffer = RootNode->Name;
    } else {
        RtlInitUnicodeString(RootName, L"HiveRoot");
        *RootHandle = (HANDLE)HCELL_NIL;
        HvSyncHive((PHHIVE)Hive);
    }


    return((HANDLE)Hive);
}


NTSTATUS
EhEnumerateKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Enumerate sub keys, return data on Index'th entry.

    CmEnumerateKey returns the name of the Index'th sub key of the open
    key specified.  The value STATUS_NO_MORE_ENTRIES will be
    returned if value of Index is larger than the number of sub keys.

    Note that Index is simply a way to select among child keys.  Two calls
    to CmEnumerateKey with the same Index are NOT guaranteed to return
    the same results.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK    kcb;

    kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    kcb.Delete = FALSE;
    kcb.KeyHive = &((PCMHIVE)HiveHandle)->Hive;
    kcb.KeyCell = (HCELL_INDEX)CellHandle;
    kcb.RefCount = 1;

    return(CmEnumerateKey(&kcb,
                          Index,
                          KeyInformationClass,
                          KeyInformation,
                          Length,
                          ResultLength));
}


NTSTATUS
EhEnumerateValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The value entries of an open key may be enumerated.

    CmEnumerateValueKey returns the name of the Index'th value
    entry of the open key specified by KeyHandle.  The value
    STATUS_NO_MORE_ENTRIES will be returned if value of Index is
    larger than the number of sub keys.

    Note that Index is simply a way to select among value
    entries.  Two calls to NtEnumerateValueKey with the same Index
    are NOT guaranteed to return the same results.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a handle to the hive

    Cell - supplies handle to node whose sub keys are to be found

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyValueInformationClass - Specifies the type of information returned
    in Buffer. One of the following types:

        KeyValueBasicInformation - return time of last write,
            title index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write,
            title index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK    kcb;

    kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    kcb.Delete = FALSE;
    kcb.KeyHive = (PHHIVE)&(((PCMHIVE)HiveHandle)->Hive);
    kcb.KeyCell = (HCELL_INDEX)CellHandle;
    kcb.RefCount = 1;

    return(CmEnumerateValueKey(&kcb,
                               Index,
                               KeyValueInformationClass,
                               KeyValueInformation,
                               Length,
                               ResultLength));
}


NTSTATUS
EhOpenChildByNumber(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN ULONG  Index,
    IN NODE_TYPE   Type,
    OUT PHANDLE ChildCell
    )
/*++

Routine Description:

    Return the cell index of the Nth child cell.

Arguments:

    HiveHandle - handle of hive control structure for hive of interest

    CellHandle - handle for parent cell

    Index - number of desired child

    Type - type of the child object

    ChildCell - supplies a pointer to a variable to receive the
                    HCELL_INDEX of the Index'th child.

Return Value:

    status

--*/
{
    return(CmpFindChildByNumber(&((PCMHIVE)HiveHandle)->Hive,
                                (HCELL_INDEX)CellHandle,
                                Index,
                                Type,
                                (PHCELL_INDEX)ChildCell));

}


NTSTATUS
EhSetValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    A value entry may be created or replaced with EhSetValueKey.

    If a value entry with a Value ID (i.e. name) matching the
    one specified by ValueName exists, it is deleted and replaced
    with the one specified.  If no such value entry exists, a new
    one is created.  NULL is a legal Value ID.  While Value IDs must
    be unique within any given key, the same Value ID may appear
    in many different keys.

Arguments:

    HiveHandle - handle of hive control structure for hive of interest

    CellHandle - handle for parent cell

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    TitleIndex - Supplies the title index for ValueName.  The title
        index specifies the index of the localized alias for the ValueName.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK kcb;

    kcb.Delete = FALSE;
    kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    kcb.KeyHive = (PHHIVE)&(((PCMHIVE)HiveHandle)->Hive);
    kcb.KeyCell = (HCELL_INDEX)CellHandle;
    kcb.FullName.Length = 0;
    kcb.FullName.MaximumLength = 0;
    kcb.FullName.Buffer = NULL;

    return(CmSetValueKey(&kcb,
                         *ValueName,
                         TitleIndex,
                         Type,
                         Data,
                         DataSize));

}


NTSTATUS
EhOpenChildByName(
    HANDLE HiveHandle,
    HANDLE CellHandle,
    PUNICODE_STRING  Name,
    PHANDLE ChildCell
    )
/*++

Routine Description:

    Find the child subkey cell specified by Name.

Arguments:

    HiveHandle - handle of hive control structure for hive of interest

    CellHandle - handle for parent cell

    Name - name of child object to find

    ChildCell - pointer to variable to receive cell index of child

Return Value:

    status

--*/
{
    PHCELL_INDEX    Index;

    return(CmpFindChildByName(&((PCMHIVE)HiveHandle)->Hive,
                              (HCELL_INDEX)CellHandle,
                              *Name,
                              KeyBodyNode,
                              (PHCELL_INDEX)ChildCell,
                              &Index));
}


NTSTATUS
EhCreateChild(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN PUNICODE_STRING  Name,
    OUT PHANDLE ChildCell,
    OUT PULONG Disposition OPTIONAL
    )
/*++

Routine Description:

    Attempts to open the given child subkey specified by name.  If the
    child does not exist, it is created.

Arguments:

    HiveHandle - handle of hive control structure for hive of interest

    CellHandle - handle for parent cell


    Name - name of child object to create

    ChildCell - pointer to variable to receive cell index of child

    Disposition - This optional parameter is a pointer to a variable
        that will receive a value indicating whether a new Registry
        key was created or an existing one opened:

        REG_CREATED_NEW_KEY - A new Registry Key was created
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

    status

--*/
{
    PHCELL_INDEX Index;
    NTSTATUS Status;
    PHHIVE Hive;
    HCELL_INDEX NewCell;
    HCELL_INDEX NewListCell;
    HCELL_INDEX OldListCell;
    PHCELL_INDEX NewList;
    PHCELL_INDEX OldList;
    PCM_KEY_NODE Child;
    PCM_KEY_NODE Parent;
    PCM_KEY_SECURITY Security;
    HANDLE Handle;
    ULONG oldcount;

    Hive = &((PCMHIVE)HiveHandle)->Hive;

    if ((HCELL_INDEX)CellHandle == HCELL_NIL) {
        if (Hive->BaseBlock->RootCell != HCELL_NIL) {
            *ChildCell = (HANDLE)(Hive->BaseBlock->RootCell);
            if (ARGUMENT_PRESENT(Disposition)) {
                *Disposition = REG_OPENED_EXISTING_KEY;
            }
            return(STATUS_SUCCESS);
        } else {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    } else {
        Parent = (PCM_KEY_NODE)HvGetCell(Hive, (HCELL_INDEX)CellHandle);
        Status = CmpFindChildByName(Hive,
                                    (HCELL_INDEX)CellHandle,
                                    *Name,
                                    KeyBodyNode,
                                    (PHCELL_INDEX)ChildCell,
                                    &Index);
    }
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        NewCell = HvAllocateCell(Hive,
                                 CmpHKeyNodeSize(Hive,Name->Length),
                                 Stable);
        if (NewCell != HCELL_NIL) {

            *ChildCell = (HANDLE)NewCell;
            Child = (PCM_KEY_NODE)HvGetCell(Hive, NewCell);

            Child->Signature = CM_KEY_NODE_SIGNATURE;
            Child->Flags = 0;

            KeQuerySystemTime(&(Child->LastWriteTime));

            Child->Spare = 0;
            Child->Parent = (HCELL_INDEX)CellHandle;

            Child->ValueList.Count = 0;
            Child->ValueList.List = HCELL_NIL;
            Child->u1.s1.Security = HCELL_NIL;
            Child->u1.s1.Class = HCELL_NIL;
            Child->NameLength = Name->Length;
            Child->ClassLength = 0;

            Child->SubKeyCounts[Stable] = 0;
            Child->SubKeyCounts[Volatile] = 0;
            Child->SubKeyLists[Stable] = HCELL_NIL;
            Child->SubKeyLists[Volatile] = HCELL_NIL;

            Child->MaxValueDataLen = 0;
            Child->MaxNameLen = 0;
            Child->MaxValueNameLen = 0;
            Child->MaxClassLen = 0;

            if((HCELL_INDEX)CellHandle == HCELL_NIL) {
                Hive->BaseBlock->RootCell = NewCell;
                Status = EhpAttachSecurity(Hive, NewCell);
            } else {
                Child->u1.s1.Security = Parent->u1.s1.Security;
                Security = (PCM_KEY_SECURITY)HvGetCell(Hive, Child->u1.s1.Security);
                ++Security->ReferenceCount;
            }

            RtlMoveMemory(
                &(Child->Name[0]),
                Name->Buffer,
                Name->Length
                );

            Status = STATUS_SUCCESS;
        } else {
            Status == STATUS_INSUFFICIENT_RESOURCES;
            return(Status);
        }
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = REG_CREATED_NEW_KEY;
        }

        if ((HCELL_INDEX)CellHandle != HCELL_NIL) {

            //
            // put newly created child into parent's sub key list
            //
            if (! CmpAddSubKey(Hive, (HCELL_INDEX)CellHandle, NewCell)) {
                CmpFreeKeyByCell(Hive, NewCell, FALSE);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Parent = (PCM_KEY_NODE)HvGetCell(Hive, (HCELL_INDEX)CellHandle);

            if (Parent->MaxNameLen < Name->Length) {
                Parent->MaxNameLen = Name->Length;
            }
            Parent->MaxClassLen = 0;
        }
    } else {
        Status = STATUS_SUCCESS;
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = REG_OPENED_EXISTING_KEY;
        }
    }
    return(Status);
}


NTSTATUS
EhQueryKey(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Data about the class of a key, and the numbers and sizes of its
    children and value entries may be queried with CmQueryKey.

    NOTE: The returned lengths are guaranteed to be at least as
          long as the described values, but may be longer in
          some circumstances.

Arguments:

    Hive - supplies a handle to the hive control structure for the hive

    Cell - supplies handle of node to whose sub keys are to be found

    KeyInformationClass - Specifies the type of information
        returned in Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (See KEY_BASIC_INFORMATION)

        KeyNodeInformation - return last write time, title index, name, class.
            (See KEY_NODE_INFORMATION)

        KeyFullInformation - return all data except for name and security.
            (See KEY_FULL_INFORMATION)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK Kcb;

    Kcb.Delete = FALSE;
    Kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    Kcb.KeyHive = &((PCMHIVE)HiveHandle)->Hive;
    Kcb.KeyCell = (HCELL_INDEX)KeyHandle;
    Kcb.FullName.Length = 0;
    Kcb.FullName.MaximumLength = 0;
    Kcb.FullName.Buffer = NULL;

    return(CmQueryKey(&Kcb,
                      KeyInformationClass,
                      KeyInformation,
                      Length,
                      ResultLength));
}

PSECURITY_DESCRIPTOR
EhGetKeySecurity(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle
    )
{
    PCM_KEY_SECURITY Security;

    CmpGetObjectSecurity((HCELL_INDEX)KeyHandle,
                         &((PCMHIVE)HiveHandle)->Hive,
                         &Security,
                         NULL);

    return(&Security->Descriptor);
}


NTSTATUS
EhQueryValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The ValueName, TitleIndex, Type, and Data for any one of a key's
    value entries may be queried with CmQueryValueKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

    ValueName  - The name of the value entry to return data for.

    KeyValueInformationClass - Specifies the type of information
        returned in KeyValueInformation.  One of the following types:

        KeyValueBasicInformation - return time of last write, title
            index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write, title
            index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK kcb;

    kcb.Delete = FALSE;
    kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    kcb.KeyHive = &((PCMHIVE)HiveHandle)->Hive;
    kcb.KeyCell = (HCELL_INDEX)KeyHandle;
    kcb.FullName.Length = 0;
    kcb.FullName.MaximumLength = 0;
    kcb.FullName.Buffer = NULL;

    return(CmQueryValueKey(&kcb,
                           *ValueName,
                           KeyValueInformationClass,
                           KeyValueInformation,
                           Length,
                           ResultLength));

}


NTSTATUS
EhDeleteValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN PUNICODE_STRING ValueName         // RAW
    )
/*++

Routine Description:

    One of the value entries of a registry key may be removed with this call.

    The value entry with ValueName matching ValueName is removed from the key.
    If no such entry exists, an error is returned.

Arguments:

    Hive - Supplies a handle to the hive control structure

    Cell - Supplies a handle to the registry key to be operated on

    ValueName - The name of the value to be deleted.  NULL is a legal name.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    CM_KEY_CONTROL_BLOCK kcb;

    kcb.Delete = FALSE;
    kcb.Signature = CM_KEY_CONTROL_BLOCK_SIGNATURE;
    kcb.KeyHive = &((PCMHIVE)HiveHandle)->Hive;
    kcb.KeyCell = (HCELL_INDEX)CellHandle;
    kcb.FullName.Length = 0;
    kcb.FullName.MaximumLength = 0;
    kcb.FullName.Buffer = NULL;

    return(CmDeleteValueKey(&kcb, *ValueName));
}


NTSTATUS
EhpAttachSecurity(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )

/*++

Routine Description:

    Creates a security descriptor cell and attaches it to the given
    node.

Arguments:

    Hive - Supplies a pointer to the hive control structure.

    Cell - Supplies the cell index of the node to attach the security
           descriptor to.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    HANDLE Token;
    HCELL_INDEX SecurityCell;
    PCM_KEY_NODE Node;
    PCM_KEY_SECURITY Security;
    PSECURITY_DESCRIPTOR Descriptor;
    ULONG DescriptorLength;
    HANDLE Handle;

    Status = NtOpenProcessToken( NtCurrentProcess(),
                                 TOKEN_QUERY,
                                 &Token );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlNewSecurityObject( NULL,
                                   NULL,
                                   &Descriptor,
                                   FALSE,
                                   Token,
                                   &CmpKeyMapping );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    SecurityCell = HvAllocateCell(Hive,
                                  SECURITY_CELL_LENGTH(Descriptor),
                                  Stable);
    if (SecurityCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Node->u1.s1.Security = SecurityCell;
    Security = (PCM_KEY_SECURITY)HvGetCell(Hive, SecurityCell);
    DescriptorLength = RtlLengthSecurityDescriptor(Descriptor);
    Security->Signature = CM_KEY_SECURITY_SIGNATURE;
    Security->ReferenceCount = 1;
    Security->DescriptorLength = DescriptorLength;
    RtlMoveMemory( &Security->Descriptor,
                   Descriptor,
                   DescriptorLength );
    Security->Flink = Security->Blink = SecurityCell;
    return(STATUS_SUCCESS);

}
