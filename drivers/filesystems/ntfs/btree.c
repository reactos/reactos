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

// TEMP FUNCTION for diagnostic purposes.
// Prints VCN of every node in an index allocation
VOID
PrintAllVCNs(PDEVICE_EXTENSION Vcb,
             PNTFS_ATTR_CONTEXT IndexAllocationContext,
             ULONG NodeSize)
{
    ULONGLONG CurrentOffset = 0;
    PINDEX_BUFFER CurrentNode, Buffer;
    ULONGLONG BufferSize = AttributeDataLength(IndexAllocationContext->pRecord);
    ULONG BytesRead;
    ULONGLONG i;
    int Count = 0;

    if (BufferSize == 0)
    {
        DPRINT1("Index Allocation is empty.\n");
        return;
    }

    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG_NTFS);

    BytesRead = ReadAttribute(Vcb, IndexAllocationContext, 0, (PCHAR)Buffer, BufferSize);

    ASSERT(BytesRead = BufferSize);

    CurrentNode = Buffer;

    // loop through all the nodes
    for (i = 0; i < BufferSize; i += NodeSize)
    {
        NTSTATUS Status = FixupUpdateSequenceArray(Vcb, &CurrentNode->Ntfs);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Fixing fixup failed!\n");
            continue;
        }

        DPRINT1("Node #%d, VCN: %I64u\n", Count, CurrentNode->VCN);

        CurrentNode = (PINDEX_BUFFER)((ULONG_PTR)CurrentNode + NodeSize);
        CurrentOffset += NodeSize;
        Count++;
    }

    ExFreePoolWithTag(Buffer, TAG_NTFS);
}

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

    // Key1 must not be the final key (AKA the dummy key)
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

        // If the truncated names are the same length, the shorter one comes first
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

        // If the truncated names are the same length, the shorter one comes first
        if (Comparison == 0)
            return 1;
    }

    return Comparison;
}

PB_TREE_FILENAME_NODE
CreateBTreeNodeFromIndexNode(PDEVICE_EXTENSION Vcb,
                             PINDEX_ROOT_ATTRIBUTE IndexRoot,
                             PNTFS_ATTR_CONTEXT IndexAllocationAttributeCtx,
                             PINDEX_ENTRY_ATTRIBUTE NodeEntry)
{
    PB_TREE_FILENAME_NODE NewNode;
    PINDEX_ENTRY_ATTRIBUTE CurrentNodeEntry;
    PINDEX_ENTRY_ATTRIBUTE FirstNodeEntry;
    ULONG CurrentEntryOffset = 0;
    PINDEX_BUFFER NodeBuffer;
    ULONG IndexBufferSize = Vcb->NtfsInfo.BytesPerIndexRecord;
    PULONGLONG NodeNumber;
    PB_TREE_KEY CurrentKey;
    NTSTATUS Status;
    ULONGLONG IndexNodeOffset;
    ULONG BytesRead;

    if (IndexAllocationAttributeCtx == NULL)
    {
        DPRINT1("ERROR: Couldn't find index allocation attribute even though there should be one!\n");
        return NULL;
    }

    // Get the node number from the end of the node entry
    NodeNumber = (PULONGLONG)((ULONG_PTR)NodeEntry + NodeEntry->Length - sizeof(ULONGLONG));

    // Create the new tree node
    DPRINT1("About to allocate %ld for NewNode\n", sizeof(B_TREE_FILENAME_NODE));
    NewNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_FILENAME_NODE), TAG_NTFS);
    if (!NewNode)
    {
        DPRINT1("ERROR: Couldn't allocate memory for new filename node.\n");
        return NULL;
    }
    RtlZeroMemory(NewNode, sizeof(B_TREE_FILENAME_NODE));

    // Create the first key
    CurrentKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);
    if (!CurrentKey)
    {
        DPRINT1("ERROR: Failed to allocate memory for key!\n");
        ExFreePoolWithTag(NewNode, TAG_NTFS);
        return NULL;
    }
    RtlZeroMemory(CurrentKey, sizeof(B_TREE_KEY));
    NewNode->FirstKey = CurrentKey;

    // Allocate memory for the node buffer
    NodeBuffer = ExAllocatePoolWithTag(NonPagedPool, IndexBufferSize, TAG_NTFS);
    if (!NodeBuffer)
    {
        DPRINT1("ERROR: Couldn't allocate memory for node buffer!\n");
        ExFreePoolWithTag(CurrentKey, TAG_NTFS);
        ExFreePoolWithTag(NewNode, TAG_NTFS);
        return NULL;
    }

    // Calculate offset into index allocation
    IndexNodeOffset = GetAllocationOffsetFromVCN(Vcb, IndexBufferSize, *NodeNumber);

    // TODO: Confirm index bitmap has this node marked as in-use

    // Read the node
    BytesRead = ReadAttribute(Vcb,
                              IndexAllocationAttributeCtx,
                              IndexNodeOffset,
                              (PCHAR)NodeBuffer,
                              IndexBufferSize);

    ASSERT(BytesRead == IndexBufferSize);
    NT_ASSERT(NodeBuffer->Ntfs.Type == NRH_INDX_TYPE);
    NT_ASSERT(NodeBuffer->VCN == *NodeNumber);

    // Apply the fixup array to the node buffer
    Status = FixupUpdateSequenceArray(Vcb, &NodeBuffer->Ntfs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't apply fixup array to index node buffer!\n");
        ExFreePoolWithTag(NodeBuffer, TAG_NTFS);
        ExFreePoolWithTag(CurrentKey, TAG_NTFS);
        ExFreePoolWithTag(NewNode, TAG_NTFS);
        return NULL;
    }

    // Walk through the index and create keys for all the entries
    FirstNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)(&NodeBuffer->Header)
                                               + NodeBuffer->Header.FirstEntryOffset);
    CurrentNodeEntry = FirstNodeEntry;
    while (CurrentEntryOffset < NodeBuffer->Header.TotalSizeOfEntries)
    {
        // Allocate memory for the current entry
        CurrentKey->IndexEntry = ExAllocatePoolWithTag(NonPagedPool, CurrentNodeEntry->Length, TAG_NTFS);
        if (!CurrentKey->IndexEntry)
        {
            DPRINT1("ERROR: Couldn't allocate memory for next key!\n");
            DestroyBTreeNode(NewNode);
            ExFreePoolWithTag(NodeBuffer, TAG_NTFS);
            return NULL;
        }

        NewNode->KeyCount++;

        // If this isn't the last entry
        if (!(CurrentNodeEntry->Flags & NTFS_INDEX_ENTRY_END))
        {
            // Create the next key
            PB_TREE_KEY NextKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);
            if (!NextKey)
            {
                DPRINT1("ERROR: Couldn't allocate memory for next key!\n");
                DestroyBTreeNode(NewNode);
                ExFreePoolWithTag(NodeBuffer, TAG_NTFS);
                return NULL;
            }
            RtlZeroMemory(NextKey, sizeof(B_TREE_KEY));

            // Add NextKey to the end of the list
            CurrentKey->NextKey = NextKey;

            // Copy the current entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);

            // See if the current key has a sub-node
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                DPRINT1("TODO: Only a node with a single-level is supported right now!\n");
                // Needs debugging:
                CurrentKey->LesserChild = CreateBTreeNodeFromIndexNode(Vcb,
                                                                       IndexRoot,
                                                                       IndexAllocationAttributeCtx,
                                                                       CurrentKey->IndexEntry);
            }

            CurrentKey = NextKey;
        }
        else
        {
            // Copy the final entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);
            CurrentKey->NextKey = NULL;

            // See if the current key has a sub-node
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                DPRINT1("TODO: Only a node with a single-level is supported right now!\n");
                // Needs debugging:
                CurrentKey->LesserChild = CreateBTreeNodeFromIndexNode(Vcb,
                                                                       IndexRoot,
                                                                       IndexAllocationAttributeCtx,
                                                                       CurrentKey->IndexEntry);
            }

            break;
        }

        // Advance to the next entry
        CurrentEntryOffset += CurrentNodeEntry->Length;
        CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);
    }

    NewNode->NodeNumber = *NodeNumber;
    NewNode->ExistsOnDisk = TRUE;

    ExFreePoolWithTag(NodeBuffer, TAG_NTFS);

    return NewNode;
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
CreateBTreeFromIndex(PDEVICE_EXTENSION Vcb,
                     PFILE_RECORD_HEADER FileRecordWithIndex,
                     /*PCWSTR IndexName,*/
                     PNTFS_ATTR_CONTEXT IndexRootContext,
                     PINDEX_ROOT_ATTRIBUTE IndexRoot,
                     PB_TREE *NewTree)
{
    PINDEX_ENTRY_ATTRIBUTE CurrentNodeEntry;
    PB_TREE Tree = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE), TAG_NTFS);
    PB_TREE_FILENAME_NODE RootNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_FILENAME_NODE), TAG_NTFS);
    PB_TREE_KEY CurrentKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);
    ULONG CurrentOffset = IndexRoot->Header.FirstEntryOffset;
    PNTFS_ATTR_CONTEXT IndexAllocationContext = NULL;
    NTSTATUS Status;

    DPRINT1("CreateBTreeFromIndex(%p, %p)\n", IndexRoot, NewTree);

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

    // See if the file record has an attribute allocation
    Status = FindAttribute(Vcb,
                           FileRecordWithIndex,
                           AttributeIndexAllocation,
                           L"$I30",
                           4,
                           &IndexAllocationContext,
                           NULL);
    if (!NT_SUCCESS(Status))
        IndexAllocationContext = NULL;
    else
        PrintAllVCNs(Vcb, IndexAllocationContext, IndexRoot->SizeOfEntry);

    // Setup the Tree
    RootNode->FirstKey = CurrentKey;
    Tree->RootNode = RootNode;

    // Make sure we won't try reading past the attribute-end
    if (FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header) + IndexRoot->Header.TotalSizeOfEntries > IndexRootContext->pRecord->Resident.ValueLength)
    {
        DPRINT1("Filesystem corruption detected!\n");
        DestroyBTree(Tree);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    // Start at the first node entry
    CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexRoot
                                                + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header)
                                                + IndexRoot->Header.FirstEntryOffset);

    // Create a key for each entry in the node
    while (CurrentOffset < IndexRoot->Header.TotalSizeOfEntries)
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
            PB_TREE_KEY NextKey = ExAllocatePoolWithTag(NonPagedPool, sizeof(B_TREE_KEY), TAG_NTFS);
            if (!NextKey)
            {
                DPRINT1("ERROR: Couldn't allocate memory for next key!\n");
                DestroyBTree(Tree);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(NextKey, sizeof(B_TREE_KEY));

            // Add NextKey to the end of the list
            CurrentKey->NextKey = NextKey;

            // Copy the current entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);

            // Does this key have a sub-node?
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                // Create the child node
                CurrentKey->LesserChild = CreateBTreeNodeFromIndexNode(Vcb,
                                                                       IndexRoot,
                                                                       IndexAllocationContext,
                                                                       CurrentKey->IndexEntry);
                if (!CurrentKey->LesserChild)
                {
                    DPRINT1("ERROR: Couldn't create child node!\n");
                    DestroyBTree(Tree);
                    return STATUS_NOT_IMPLEMENTED;
                }
            }

            // Advance to the next entry
            CurrentOffset += CurrentNodeEntry->Length;
            CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);
            CurrentKey = NextKey;
        }
        else
        {
            // Copy the final entry to its key
            RtlCopyMemory(CurrentKey->IndexEntry, CurrentNodeEntry, CurrentNodeEntry->Length);
            CurrentKey->NextKey = NULL;

            // Does this key have a sub-node?
            if (CurrentKey->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
            {
                // Create the child node
                CurrentKey->LesserChild = CreateBTreeNodeFromIndexNode(Vcb,
                                                                       IndexRoot,
                                                                       IndexAllocationContext,
                                                                       CurrentKey->IndexEntry);
                if (!CurrentKey->LesserChild)
                {
                    DPRINT1("ERROR: Couldn't create child node!\n");
                    DestroyBTree(Tree);
                    return STATUS_NOT_IMPLEMENTED;
                }
            }

            break;
        }
    }

    *NewTree = Tree;

    if (IndexAllocationContext)
        ReleaseAttributeContext(IndexAllocationContext);

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

        // Does the current key have any sub-nodes?
        if (CurrentKey->LesserChild)
            NewIndexRoot->Header.Flags = INDEX_ROOT_LARGE;

        // Add Length of Current Entry to Total Size of Entries
        NewIndexRoot->Header.TotalSizeOfEntries += CurrentNodeEntry->Length;

        // Go to the next node entry
        CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);

        CurrentKey = CurrentKey->NextKey;
    }

    NewIndexRoot->Header.AllocatedSize = NewIndexRoot->Header.TotalSizeOfEntries;

    *IndexRoot = NewIndexRoot;
    *Length = NewIndexRoot->Header.AllocatedSize + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header);

    return STATUS_SUCCESS;
}

NTSTATUS
CreateIndexBufferFromBTreeNode(PDEVICE_EXTENSION DeviceExt,
                               PB_TREE_FILENAME_NODE Node,
                               ULONG BufferSize,
                               PINDEX_BUFFER IndexBuffer)
{
    ULONG i;
    PB_TREE_KEY CurrentKey;
    PINDEX_ENTRY_ATTRIBUTE CurrentNodeEntry;
    NTSTATUS Status;

    // TODO: Fix magic, do math
    RtlZeroMemory(IndexBuffer, BufferSize);
    IndexBuffer->Ntfs.Type = NRH_INDX_TYPE;
    IndexBuffer->Ntfs.UsaOffset = 0x28;
    IndexBuffer->Ntfs.UsaCount = 9;

    // TODO: Check bitmap for VCN
    ASSERT(Node->ExistsOnDisk);
    IndexBuffer->VCN = Node->NodeNumber;

    IndexBuffer->Header.FirstEntryOffset = 0x28;
    IndexBuffer->Header.AllocatedSize = BufferSize - FIELD_OFFSET(INDEX_BUFFER, Header);

    // Start summing the total size of this node's entries
    IndexBuffer->Header.TotalSizeOfEntries = IndexBuffer->Header.FirstEntryOffset;

    CurrentKey = Node->FirstKey;
    CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)&(IndexBuffer->Header)
                                                + IndexBuffer->Header.FirstEntryOffset);
    for (i = 0; i < Node->KeyCount; i++)
    {
        // Would adding the current entry to the index increase the node size beyond the allocation size?
        ULONG IndexSize = FIELD_OFFSET(INDEX_BUFFER, Header)
            + IndexBuffer->Header.TotalSizeOfEntries
            + CurrentNodeEntry->Length;
        if (IndexSize > BufferSize)
        {
            DPRINT1("TODO: Adding file would require creating a new node!\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        ASSERT(CurrentKey->IndexEntry->Length != 0);

        // Copy the index entry
        RtlCopyMemory(CurrentNodeEntry, CurrentKey->IndexEntry, CurrentKey->IndexEntry->Length);

        DPRINT1("Index Node Entry Stream Length: %u\nIndex Node Entry Length: %u\n",
                CurrentNodeEntry->KeyLength,
                CurrentNodeEntry->Length);

        // Add Length of Current Entry to Total Size of Entries
        IndexBuffer->Header.TotalSizeOfEntries += CurrentNodeEntry->Length;

        // TODO: Check for child nodes

        // Go to the next node entry
        CurrentNodeEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)CurrentNodeEntry + CurrentNodeEntry->Length);
        CurrentKey = CurrentKey->NextKey;
    }

    Status = AddFixupArray(DeviceExt, &IndexBuffer->Ntfs);

    return Status;
}

NTSTATUS
UpdateIndexAllocation(PDEVICE_EXTENSION DeviceExt,
                      PB_TREE Tree,
                      ULONG IndexBufferSize,
                      PFILE_RECORD_HEADER FileRecord)
{
    // Find the index allocation and bitmap
    PNTFS_ATTR_CONTEXT IndexAllocationContext, BitmapContext;
    PB_TREE_KEY CurrentKey;
    NTSTATUS Status;
    BOOLEAN HasIndexAllocation = FALSE;
    ULONG i;

    DPRINT1("UpdateIndexAllocations() called.\n");

    Status = FindAttribute(DeviceExt, FileRecord, AttributeIndexAllocation, L"$I30", 4, &IndexAllocationContext, NULL);
    if (NT_SUCCESS(Status))
        HasIndexAllocation = TRUE;

    // TODO: Handle bitmap
    BitmapContext = NULL;

    // Walk through the root node and update all the sub-nodes
    CurrentKey = Tree->RootNode->FirstKey;
    for (i = 0; i < Tree->RootNode->KeyCount; i++)
    {
        if (CurrentKey->LesserChild)
        {
            if (!HasIndexAllocation)
            {
                DPRINT1("FIXME: Need to add index allocation\n");
                return STATUS_NOT_IMPLEMENTED;
            }
            else
            {
                Status = UpdateIndexNode(DeviceExt, CurrentKey->LesserChild, IndexBufferSize, IndexAllocationContext, BitmapContext);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ERROR: Failed to update index node!\n");
                    ReleaseAttributeContext(IndexAllocationContext);
                    return Status;
                }
            }

        }
        CurrentKey = CurrentKey->NextKey;
    }

    if(HasIndexAllocation)
        ReleaseAttributeContext(IndexAllocationContext);

    return STATUS_SUCCESS;
}

NTSTATUS
UpdateIndexNode(PDEVICE_EXTENSION DeviceExt,
                PB_TREE_FILENAME_NODE Node,
                ULONG IndexBufferSize,
                PNTFS_ATTR_CONTEXT IndexAllocationContext,
                PNTFS_ATTR_CONTEXT BitmapContext)
{
    ULONG i;
    PB_TREE_KEY CurrentKey = Node->FirstKey;
    NTSTATUS Status;

    DPRINT1("UpdateIndexNode(%p, %p, %lu, %p, %p) called for index node with VCN %I64u\n", DeviceExt, Node, IndexBufferSize, IndexAllocationContext, BitmapContext, Node->NodeNumber);

    // Do we need to write this node to disk?
    if (Node->DiskNeedsUpdating)
    {
        ULONGLONG NodeOffset;
        ULONG LengthWritten;

        // Allocate memory for an index buffer
        PINDEX_BUFFER IndexBuffer = ExAllocatePoolWithTag(NonPagedPool, IndexBufferSize, TAG_NTFS);
        if (!IndexBuffer)
        {
            DPRINT1("ERROR: Failed to allocate %lu bytes for index buffer!\n", IndexBufferSize);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Create the index buffer we'll be writing to disk to represent this node
        Status = CreateIndexBufferFromBTreeNode(DeviceExt, Node, IndexBufferSize, IndexBuffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to create index buffer from node!\n");
            ExFreePoolWithTag(IndexBuffer, TAG_NTFS);
            return Status;
        }

        // Get Offset of index buffer in index allocation
        NodeOffset = GetAllocationOffsetFromVCN(DeviceExt, IndexBufferSize, Node->NodeNumber);

        // Write the buffer to the index allocation
        Status = WriteAttribute(DeviceExt, IndexAllocationContext, NodeOffset, (const PUCHAR)IndexBuffer, IndexBufferSize, &LengthWritten, NULL);
        if (!NT_SUCCESS(Status) || LengthWritten != IndexBufferSize)
        {
            DPRINT1("ERROR: Failed to update index allocation!\n");
            ExFreePoolWithTag(IndexBuffer, TAG_NTFS);
            if (!NT_SUCCESS(Status))
                return Status;
            else
                return STATUS_END_OF_FILE;
        }

        Node->DiskNeedsUpdating = FALSE;

        // Free the index buffer
        ExFreePoolWithTag(IndexBuffer, TAG_NTFS);
    }

    // Walk through the node and look for children to update
    for (i = 0; i < Node->KeyCount; i++)
    {
        ASSERT(CurrentKey);

        // If there's a child node
        if (CurrentKey->LesserChild)
        {
            // Update the child node on disk
            Status = UpdateIndexNode(DeviceExt, CurrentKey->LesserChild, IndexBufferSize, IndexAllocationContext, BitmapContext);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ERROR: Failed to update child node!\n");
                return Status;
            }
        }

        CurrentKey = CurrentKey->NextKey;
    }

    return STATUS_SUCCESS;
}

PB_TREE_KEY
CreateBTreeKeyFromFilename(ULONGLONG FileReference, PFILENAME_ATTRIBUTE FileNameAttribute)
{
    PB_TREE_KEY NewKey;
    ULONG AttributeSize = GetFileNameAttributeLength(FileNameAttribute);
    ULONG EntrySize = ALIGN_UP_BY(AttributeSize + FIELD_OFFSET(INDEX_ENTRY_ATTRIBUTE, FileName), 8);

    // Create a new Index Entry for the file
    PINDEX_ENTRY_ATTRIBUTE NewEntry = ExAllocatePoolWithTag(NonPagedPool, EntrySize, TAG_NTFS);
    if (!NewEntry)
    {
        DPRINT1("ERROR: Failed to allocate memory for Index Entry!\n");
        return NULL;
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
        return NULL;
    }
    NewKey->IndexEntry = NewEntry;
    NewKey->NextKey = NULL;

    return NewKey;
}

VOID
DestroyBTreeKey(PB_TREE_KEY Key)
{
    if (Key->IndexEntry)
        ExFreePoolWithTag(Key->IndexEntry, TAG_NTFS);

    if (Key->LesserChild)
        DestroyBTreeNode(Key->LesserChild);

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

    // Is there a child node?
    if (Key->IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
    {
        if (Key->LesserChild)
            DumpBTreeNode(Key->LesserChild, Number, Depth + 1);
        else
        {
            // This will be an assert once nodes with arbitrary depth are debugged
            DPRINT1("DRIVER ERROR: No Key->LesserChild despite Key->IndexEntry->Flags indicating this is a node!\n");
        }
    }
}

VOID
DumpBTreeNode(PB_TREE_FILENAME_NODE Node, ULONG Number, ULONG Depth)
{
    PB_TREE_KEY CurrentKey;
    ULONG i;
    for (i = 0; i < Depth; i++)
        DbgPrint(" ");
    DbgPrint("Node #%d, Depth %d, has %d key%s\n", Number, Depth, Node->KeyCount, Node->KeyCount == 1 ? "" : "s");

    CurrentKey = Node->FirstKey;
    for (i = 1; i <= Node->KeyCount; i++)
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

// Calculates start of Index Buffer relative to the index allocation, given the node's VCN
ULONGLONG
GetAllocationOffsetFromVCN(PDEVICE_EXTENSION DeviceExt,
                           ULONG IndexBufferSize,
                           ULONGLONG Vcn)
{
    if (IndexBufferSize < DeviceExt->NtfsInfo.BytesPerCluster)
        return Vcn * DeviceExt->NtfsInfo.BytesPerSector;

    return Vcn * DeviceExt->NtfsInfo.BytesPerCluster;
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
    PB_TREE_KEY NewKey, CurrentKey, PreviousKey;
    NTSTATUS Status = STATUS_SUCCESS; 
    ULONG NodeSize;
    ULONG AllocatedNodeSize;
    ULONG MaxNodeSizeWithoutHeader;
    ULONG i;

    DPRINT1("NtfsInsertKey(0x%I64x, %p, %p, %s)\n",
            FileReference,
            FileNameAttribute,
            Node,
            CaseSensitive ? "TRUE" : "FALSE");

    // Create the key for the filename attribute
    NewKey = CreateBTreeKeyFromFilename(FileReference, FileNameAttribute);
    if (!NewKey)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Find where to insert the key
    CurrentKey = Node->FirstKey;
    PreviousKey = NULL;
    for (i = 0; i < Node->KeyCount; i++)
    {
        // Should the New Key go before the current key?
        LONG Comparison = CompareTreeKeys(NewKey, CurrentKey, CaseSensitive);

        ASSERT(Comparison != 0);

        // Is NewKey < CurrentKey?
        if (Comparison < 0)
        {

            // Does CurrentKey have a sub-node?
            if (CurrentKey->LesserChild)
            {
                // Insert the key into the child node
                Status = NtfsInsertKey(FileReference, FileNameAttribute, CurrentKey->LesserChild, CaseSensitive);
            }
            else
            {
                // Insert New Key before Current Key
                NewKey->NextKey = CurrentKey;

                // Increase KeyCount and mark node as dirty
                Node->KeyCount++;
                Node->DiskNeedsUpdating = TRUE;

                // was CurrentKey the first key?
                if (CurrentKey == Node->FirstKey)
                    Node->FirstKey = NewKey;
                else
                    PreviousKey->NextKey = NewKey;
                break;
            }
        }

        PreviousKey = CurrentKey;
        CurrentKey = CurrentKey->NextKey;
    }

    // Is the node larger than its allocated size?
    NodeSize = 0; 
    CurrentKey = Node->FirstKey;
    for (i = 0; i < Node->KeyCount; i++)
    {
        NodeSize += CurrentKey->IndexEntry->Length;
        CurrentKey = CurrentKey->NextKey;
    }

    // TEMPTEMP: TODO: MATH
    AllocatedNodeSize = 0xfe8;
    MaxNodeSizeWithoutHeader = AllocatedNodeSize - 0x28;

    if (NodeSize > MaxNodeSizeWithoutHeader)
    {
        DPRINT1("FIXME: Splitting a node is still a WIP!\n");
        //SplitBTreeNode(NULL, Node);
        //DumpBTree(Tree);
        return STATUS_NOT_IMPLEMENTED;
    }

    // NewEntry and NewKey will be destroyed later by DestroyBTree()

    return Status;
}