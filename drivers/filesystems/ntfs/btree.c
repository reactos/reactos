/*
*  ReactOS kernel
*  Copyright (C) 2002, 2017 ReactOS Team
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*
* COPYRIGHT:        See COPYING in the top level directory
* PROJECT:          ReactOS kernel
* FILE:             drivers/filesystem/ntfs/btree.c
* PURPOSE:          NTFS filesystem driver
* PROGRAMMERS:      Trevor Thompson
*/

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/**
* @name CompareTreeKeys
* @implemented
*
* Compare two B_TREE_KEY's to determine their order in the tree.
*
* @param Key1
* Pointer to a B_TREE_KEY that will be compared.
*
* @param Key2
* Pointer to the other B_TREE_KEY that will be compared.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application created the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @returns
* 0 if the two keys are equal.
* < 0 if key1 is less thank key2
* > 0 if key1 is greater than key2
*
* @remarks
* Any other key is always less than the final (dummy) key in a node. Key1 must not be the dummy node.
*/
LONG
CompareTreeKeys(PB_TREE_KEY Key1, PB_TREE_KEY Key2, BOOLEAN CaseSensitive)
{
    UNICODE_STRING Key1Name, Key2Name;
    LONG Comparison;

    ASSERT(!(Key1->IndexEntry->Flags & NTFS_INDEX_ENTRY_END));

    // If Key2 is the "dummy key", key 1 will always come first
    if (Key2->NextKey == NULL)
        return -1;

    Key1Name.Buffer = Key1->IndexEntry->FileName.Name;
    Key1Name.Length = Key1Name.MaximumLength
        = Key1->IndexEntry->FileName.NameLength * sizeof(WCHAR);

    Key2Name.Buffer = Key2->IndexEntry->FileName.Name;
    Key2Name.Length = Key2Name.MaximumLength
        = Key2->IndexEntry->FileName.NameLength * sizeof(WCHAR);

    // Are the two keys the same length?
    if (Key1Name.Length == Key2Name.Length)
        return RtlCompareUnicodeString(&Key1Name, &Key2Name, !CaseSensitive);

    // Is Key1 shorter?
    if (Key1Name.Length < Key2Name.Length)
    {
        // Truncate KeyName2 to be the same length as KeyName1
        Key2Name.Length = Key1Name.Length;
        
        // Compare the names of the same length
        Comparison = RtlCompareUnicodeString(&Key1Name, &Key2Name, !CaseSensitive);

        // If the truncated files are the same length, the shorter one comes first
        if (Comparison == 0)
            return -1;
    }
    else
    {
        // Key2 is shorter
        // Truncate KeyName1 to be the same length as KeyName2
        Key1Name.Length = Key2Name.Length;

        // Compare the names of the same length
        Comparison = RtlCompareUnicodeString(&Key1Name, &Key2Name, !CaseSensitive);

        // If the truncated files are the same length, the shorter one comes first
        if (Comparison == 0)
            return 1;
    }

    return Comparison;
}

/**
* @name CreateBTreeFromIndex
* @implemented
*
* Parse an index and create a B-Tree in memory from it.
*
* @param IndexRootContext
* Pointer to an NTFS_ATTR_CONTEXT that describes the location of the index root attribute.
*
* @param NewTree
* Pointer to a PB_TREE that will receive the pointer to a newly-created B-Tree.
*
* @returns
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* 
* @remarks
* Allocates memory for the entire tree. Caller is responsible for destroying the tree with DestroyBTree().
*/
NTSTATUS
CreateBTreeFromIndex(PNTFS_ATTR_CONTEXT IndexRootContext,
                     PINDEX_ROOT_ATTRIBUTE IndexRoot,
                     PB_TREE *NewTree)
{
    PINDEX_ENTRY_ATTRIBUTE CurrentNodeEntry;
    PB_TREE Tree = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE), TAG_NTFS);
    PB_TREE_FILENAME_NODE RootNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_FILENAME_NODE), TAG_NTFS);
    PB_TREE_KEY CurrentKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);

    DPRINT1("CreateBTreeFromIndex(%p, %p, %p)\n", IndexRootContext, IndexRoot, NewTree);

    if (!Tree || !RootNode || !CurrentKey)
    {
        DPRINT1("Couldn't allocate enough memory for B-Tree!\n");
        if (Tree)
            ExFreePoolWithTag(Tree, TAG_NTFS);
        if (CurrentKey)
            ExFreePoolWithTag(CurrentKey, TAG_NTFS);
        if (RootNode)
            ExFreePoolWithTag(RootNode, TAG_NTFS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Tree, sizeof(B_TREE));
    RtlZeroMemory(RootNode, sizeof(B_TREE_FILENAME_NODE));
    RtlZeroMemory(CurrentKey, sizeof(B_TREE_KEY));

    // Setup the Tree
    RootNode->FirstKey = CurrentKey;
    Tree->RootNode = RootNode;

    // Create a key for each entry in the node
    CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexRoot
                                                + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header)
                                                + IndexRoot->Header.FirstEntryOffset);
    while (TRUE)
    {
        // Allocate memory for the current entry
        CurrentKey->IndexEntry = ExAllocatePoolWithTag(NonPagedPool, CurrentNodeEntry->Length, TAG_NTFS);
        if (!CurrentKey->IndexEntry)
        {
            DPRINT1("ERROR: Couldn't allocate memory for next key!\n");
            DestroyBTree(Tree);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RootNode->KeyCount++;

        // If this isn't the last entry
        if (!(CurrentNodeEntry->Flags & NTFS_INDEX_ENTRY_END))
        {
            // Create the next key
            PB_TREE_KEY NextKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(PB_TREE_KEY), TAG_NTFS);
            if (!NextKey)
            {
                DPRINT1("ERROR: Couldn't allocate memory for next key!\n");
                DestroyBTree(Tree);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(NextKey, sizeof(PB_TREE_KEY));

            // Add NextKey to the end of the list
            CurrentKey->NextKey = NextKey;

            // Copy the current entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);

            // Make sure this B-Tree is only one level deep (flat list)
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                DPRINT1("TODO: Only directories with single-level B-Trees are supported right now!\n");
                DestroyBTree(Tree);
                return STATUS_NOT_IMPLEMENTED;
            }

            // Advance to the next entry
            CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);
            CurrentKey = NextKey;
        }
        else
        {
            // Copy the final entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);
            CurrentKey->NextKey = NULL;

            // Make sure this B-Tree is only one level deep (flat list)
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                DPRINT1("TODO: Only directories with single-level B-Trees are supported right now!\n");
                DestroyBTree(Tree);
                return STATUS_NOT_IMPLEMENTED;
            }

            break;
        }
    }

    *NewTree = Tree;

    return STATUS_SUCCESS;
}

/**
* @name CreateIndexRootFromBTree
* @implemented
*
* Parse a B-Tree in memory and convert it into an index that can be written to disk.
*
* @param DeviceExt
* Pointer to the DEVICE_EXTENSION of the target drive.
*
* @param Tree
* Pointer to a B_TREE that describes the index to be written.
*
* @param MaxIndexSize
* Describes how large the index can be before it will take too much space in the file record.
* After reaching MaxIndexSize, an index can no longer be represented with just an index root
* attribute, and will require an index allocation and $I30 bitmap (TODO).
*
* @param IndexRoot
* Pointer to a PINDEX_ROOT_ATTRIBUTE that will receive a pointer to the newly-created index.
*
* @param Length
* Pointer to a ULONG which will receive the length of the new index root.
*
* @returns
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_NOT_IMPLEMENTED if the new index can't fit within MaxIndexSize.
* 
* @remarks
* If the function succeeds, it's the caller's responsibility to free IndexRoot with ExFreePoolWithTag().
*/
NTSTATUS
CreateIndexRootFromBTree(PDEVICE_EXTENSION DeviceExt,
                         PB_TREE Tree,
                         ULONG MaxIndexSize,
                         PINDEX_ROOT_ATTRIBUTE *IndexRoot,
                         ULONG *Length)
{
    ULONG i;
    PB_TREE_KEY CurrentKey;
    PINDEX_ENTRY_ATTRIBUTE CurrentNodeEntry;
    PINDEX_ROOT_ATTRIBUTE NewIndexRoot = ExAllocatePoolWithTag(NonPagedPool,
                                                               DeviceExt->NtfsInfo.BytesPerFileRecord,
                                                               TAG_NTFS);

    DPRINT1("CreateIndexRootFromBTree(%p, %p, 0x%lx, %p, %p)\n", DeviceExt, Tree, MaxIndexSize, IndexRoot, Length);

    if (!NewIndexRoot)
    {
        DPRINT1("Failed to allocate memory for Index Root!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Setup the new index root
    RtlZeroMemory(NewIndexRoot, DeviceExt->NtfsInfo.BytesPerFileRecord);

    NewIndexRoot->AttributeType = AttributeFileName;
    NewIndexRoot->CollationRule = COLLATION_FILE_NAME;
    NewIndexRoot->SizeOfEntry = DeviceExt->NtfsInfo.BytesPerIndexRecord;
    // If Bytes per index record is less than cluster size, clusters per index record becomes sectors per index
    if (NewIndexRoot->SizeOfEntry < DeviceExt->NtfsInfo.BytesPerCluster)
        NewIndexRoot->ClustersPerIndexRecord = NewIndexRoot->SizeOfEntry / DeviceExt->NtfsInfo.BytesPerSector;
    else
        NewIndexRoot->ClustersPerIndexRecord = NewIndexRoot->SizeOfEntry / DeviceExt->NtfsInfo.BytesPerCluster;

    // Setup the Index node header
    NewIndexRoot->Header.FirstEntryOffset = sizeof(INDEX_HEADER_ATTRIBUTE);
    NewIndexRoot->Header.Flags = INDEX_ROOT_SMALL;

    // Start summing the total size of this node's entries
    NewIndexRoot->Header.TotalSizeOfEntries = NewIndexRoot->Header.FirstEntryOffset;

    // Setup each Node Entry
    CurrentKey = Tree->RootNode->FirstKey;
    CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)NewIndexRoot 
                                                + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header)
                                                + NewIndexRoot->Header.FirstEntryOffset);
    for (i = 0; i < Tree->RootNode->KeyCount; i++)
    {
        // Would adding the current entry to the index increase the index size beyond the limit we've set?
        ULONG IndexSize = FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header)
                          + NewIndexRoot->Header.FirstEntryOffset
                          + NewIndexRoot->Header.TotalSizeOfEntries
                          + CurrentNodeEntry->Length;
        if (IndexSize > MaxIndexSize)
        {
            DPRINT1("TODO: Adding file would require creating an index allocation!\n");
            ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
            return STATUS_NOT_IMPLEMENTED;
        }

        ASSERT(CurrentKey->IndexEntry->Length != 0);

        // Copy the index entry
        RtlCopyMemory(CurrentNodeEntry, CurrentKey->IndexEntry, CurrentKey->IndexEntry->Length);

        DPRINT1("Index Node Entry Stream Length: %u\nIndex Node Entry Length: %u\n",
                CurrentNodeEntry->KeyLength,
                CurrentNodeEntry->Length);

        // Add Length of Current Entry to Total Size of Entries
        NewIndexRoot->Header.TotalSizeOfEntries += CurrentNodeEntry->Length;

        // Go to the next node
        CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);

        CurrentKey = CurrentKey->NextKey;
    }

    NewIndexRoot->Header.AllocatedSize = NewIndexRoot->Header.TotalSizeOfEntries;

    *IndexRoot = NewIndexRoot;
    *Length = NewIndexRoot->Header.AllocatedSize + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header);

    return STATUS_SUCCESS;
}

VOID
DestroyBTreeKey(PB_TREE_KEY Key)
{
    if (Key->IndexEntry)
        ExFreePoolWithTag(Key->IndexEntry, TAG_NTFS);

    // We'll destroy Key->LesserChild here after we start using it

    ExFreePoolWithTag(Key, TAG_NTFS);
}

VOID
DestroyBTreeNode(PB_TREE_FILENAME_NODE Node)
{
    PB_TREE_KEY NextKey;
    PB_TREE_KEY CurrentKey = Node->FirstKey;
    ULONG i;
    for (i = 0; i < Node->KeyCount; i++)
    {
        NT_ASSERT(CurrentKey);
        NextKey = CurrentKey->NextKey;
        DestroyBTreeKey(CurrentKey);
        CurrentKey = NextKey;
    }

    NT_ASSERT(NextKey == NULL);

    ExFreePoolWithTag(Node, TAG_NTFS);
}

/**
* @name DestroyBTree
* @implemented
*
* Destroys a B-Tree.
*
* @param Tree
* Pointer to the B_TREE which will be destroyed.
*
* @remarks
* Destroys every bit of data stored in the tree.
*/
VOID
DestroyBTree(PB_TREE Tree)
{
    DestroyBTreeNode(Tree->RootNode);
    ExFreePoolWithTag(Tree, TAG_NTFS);
}

VOID
DumpBTreeKey(PB_TREE_KEY Key, ULONG Number, ULONG Depth)
{
    ULONG i;
    for (i = 0; i < Depth; i++)
        DbgPrint(" ");
    DbgPrint(" Key #%d", Number);

    if (!(Key->IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
    {
        UNICODE_STRING FileName;
        FileName.Length = Key->IndexEntry->FileName.NameLength * sizeof(WCHAR);
        FileName.MaximumLength = FileName.Length;
        FileName.Buffer = Key->IndexEntry->FileName.Name;
        DbgPrint(" '%wZ'\n", &FileName);
    }
    else
    {
        DbgPrint(" (Dummy Key)\n");
    }
}

VOID
DumpBTreeNode(PB_TREE_FILENAME_NODE Node, ULONG Number, ULONG Depth)
{
    PB_TREE_KEY CurrentKey;
    ULONG i;
    for (i = 0; i < Depth; i++)
        DbgPrint(" ");
    DbgPrint("Node #%d, Depth %d\n", Number, Depth);

    CurrentKey = Node->FirstKey;
    for (i = 0; i < Node->KeyCount; i++)
    {
        DumpBTreeKey(CurrentKey, i, Depth);
        CurrentKey = CurrentKey->NextKey;
    }
}

/**
* @name DumpBTree
* @implemented
*
* Displays a B-Tree.
*
* @param Tree
* Pointer to the B_TREE which will be displayed.
*
* @remarks
* Displays a diagnostic summary of a B_TREE.
*/
VOID
DumpBTree(PB_TREE Tree)
{
    DbgPrint("B_TREE @ %p\n", Tree);
    DumpBTreeNode(Tree->RootNode, 0, 0);
}

/**
* @name NtfsInsertKey
* @implemented
*
* Inserts a FILENAME_ATTRIBUTE into a B-Tree node.
*
* @param FileReference
* Reference number to the file being added. This will be a combination of the MFT index and update sequence number.
*
* @param FileNameAttribute
* Pointer to a FILENAME_ATTRIBUTE which is the data for the key that will be added to the tree. A copy will be made.
*
* @param Node
* Pointer to a B_TREE_FILENAME_NODE into which a new key will be inserted, in order.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application created the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @remarks
* A node is always sorted, with the least comparable filename stored first and a dummy key to mark the end.
*/
NTSTATUS
NtfsInsertKey(ULONGLONG FileReference,
              PFILENAME_ATTRIBUTE FileNameAttribute,
              PB_TREE_FILENAME_NODE Node,
              BOOLEAN CaseSensitive)
{
    // Calculate size of Attribute and Index Entry
    ULONG AttributeSize = GetFileNameAttributeLength(FileNameAttribute);
    ULONG EntrySize = ALIGN_UP_BY(AttributeSize + FIELD_OFFSET(INDEX_ENTRY_ATTRIBUTE, FileName), 8);
    PINDEX_ENTRY_ATTRIBUTE NewEntry;
    PB_TREE_KEY NewKey, CurrentKey, PreviousKey;
    ULONG i;

    DPRINT1("NtfsInsertKey(0x%02I64, %p, %p, %s)\n",
            FileReference,
            FileNameAttribute,
            Node,
            CaseSensitive ? "TRUE" : "FALSE");

    // Create a new Index Entry for the file
    NewEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_NTFS);
    if (!NewEntry)
    {
        DPRINT1("ERROR: Failed to allocate memory for Index Entry!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Setup the Index Entry
    RtlZeroMemory(NewEntry, EntrySize);
    NewEntry->Data.Directory.IndexedFile = FileReference;
    NewEntry->Length = EntrySize;
    NewEntry->KeyLength = AttributeSize;

    // Copy the FileNameAttribute
    RtlCopyMemory(&NewEntry->FileName, FileNameAttribute, AttributeSize);

    // Setup the New Key
    NewKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);
    if (!NewKey)
    {
        DPRINT1("ERROR: Failed to allocate memory for new key!\n");
        ExFreePoolWithTag(NewEntry, TAG_NTFS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewKey->IndexEntry = NewEntry;
    NewKey->NextKey = NULL;

    // Find where to insert the key
    CurrentKey = Node->FirstKey;
    PreviousKey = NULL;
    for (i = 0; i < Node->KeyCount; i++)
    {
        // Should the New Key go before the current key?
        LONG Comparison = CompareTreeKeys(NewKey, CurrentKey, CaseSensitive);
        if (Comparison == 0)
        {
            DPRINT1("DRIVER ERROR: Asked to insert key into tree that already has it!\n");
            ExFreePoolWithTag(NewKey, TAG_NTFS);
            ExFreePoolWithTag(NewEntry, TAG_NTFS);
            return STATUS_INVALID_PARAMETER;
        }
        if (Comparison < 0)
        {
            // NewKey is < CurrentKey
            // Insert New Key before Current Key
            NewKey->NextKey = CurrentKey;

            // was CurrentKey the first key?
            if (CurrentKey == Node->FirstKey)
                Node->FirstKey = NewKey;
            else
                PreviousKey->NextKey = NewKey;
            break;
        }

        PreviousKey = CurrentKey;
        CurrentKey = CurrentKey->NextKey;
    }

    Node->KeyCount++;

    // NewEntry and NewKey will be destroyed later by DestroyBTree()

    return STATUS_SUCCESS;
}