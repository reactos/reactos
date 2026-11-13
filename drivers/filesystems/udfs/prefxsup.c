////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 File: PrefxSup.cpp

 Module: UDF File System Driver (Kernel mode execution only)

 Description:
   This module implements the UDF Prefix support routines.

*/

#include "udffs.h"

// define the file specific bug-check id
#define UDF_BUG_CHECK_ID                UDF_FILE_MISC

/*
    Lexicographic comparison of two UNICODE_STRING names.
    Returns LessThan, EqualTo, or GreaterThan.
    Used for splay tree ordering.
*/
static
FSRTL_COMPARISON_RESULT
UdfCompareNames(
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    )
{
    ULONG MinLength = NameA->Length;
    FSRTL_COMPARISON_RESULT Result = LessThan;

    if (NameA->Length > NameB->Length) {
        MinLength = NameB->Length;
        Result = GreaterThan;
    } else if (NameA->Length == NameB->Length) {
        Result = EqualTo;
    }

    ULONG i = (ULONG)RtlCompareMemory(NameA->Buffer, NameB->Buffer, MinLength);

    if (i < MinLength) {
        return ((NameA->Buffer[i / 2] < NameB->Buffer[i / 2]) ?
                LessThan : GreaterThan);
    }

    return Result;
}


/*
    Search a splay tree for an LCB matching the given name.
    On match, rebalances the tree (splay) and returns the LCB.
    Returns NULL if no match.

    LinksOffset: byte offset of RTL_SPLAY_LINKS within LCB structure
    NameOffset:  byte offset of UNICODE_STRING within LCB structure
*/
static
PLCB
UdfFindNameLink(
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name,
    IN ULONG LinksOffset,
    IN ULONG NameOffset
    )
{
    PRTL_SPLAY_LINKS Links = *RootNode;

    while (Links != NULL) {

        PLCB Node = (PLCB)((PUCHAR)Links - LinksOffset);
        PUNICODE_STRING NodeName = (PUNICODE_STRING)((PUCHAR)Node + NameOffset);

        FSRTL_COMPARISON_RESULT Comparison = UdfCompareNames(NodeName, Name);

        if (Comparison == GreaterThan) {
            Links = RtlLeftChild(Links);
        } else if (Comparison == LessThan) {
            Links = RtlRightChild(Links);
        } else {
            *RootNode = RtlSplay(Links);
            return Node;
        }
    }

    return NULL;
}


/*
    Insert an LCB into a splay tree keyed by the given name.
    Returns TRUE if inserted, FALSE if duplicate exists.

    LinksOffset: byte offset of RTL_SPLAY_LINKS within LCB structure
    NameOffset:  byte offset of UNICODE_STRING within LCB structure
*/
static
BOOLEAN
UdfInsertNameLink(
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PLCB NameLink,
    IN ULONG LinksOffset,
    IN ULONG NameOffset
    )
{
    PRTL_SPLAY_LINKS NewLinks = (PRTL_SPLAY_LINKS)((PUCHAR)NameLink + LinksOffset);
    PUNICODE_STRING NewName = (PUNICODE_STRING)((PUCHAR)NameLink + NameOffset);

    RtlInitializeSplayLinks(NewLinks);

    if (*RootNode == NULL) {
        *RootNode = NewLinks;
        return TRUE;
    }

    PLCB Node = (PLCB)((PUCHAR)*RootNode - LinksOffset);

    while (TRUE) {

        PRTL_SPLAY_LINKS NodeLinks = (PRTL_SPLAY_LINKS)((PUCHAR)Node + LinksOffset);
        PUNICODE_STRING NodeName = (PUNICODE_STRING)((PUCHAR)Node + NameOffset);

        FSRTL_COMPARISON_RESULT Comparison = UdfCompareNames(NodeName, NewName);

        if (Comparison == EqualTo) {
            // Duplicate name — not inserted. Caller will not set IN_TREE flag.
            return FALSE;
        }

        if (Comparison == GreaterThan) {
            if (RtlLeftChild(NodeLinks) == NULL) {
                RtlInsertAsLeftChild(NodeLinks, NewLinks);
                break;
            }
            Node = (PLCB)((PUCHAR)RtlLeftChild(NodeLinks) - LinksOffset);
        } else {
            if (RtlRightChild(NodeLinks) == NULL) {
                RtlInsertAsRightChild(NodeLinks, NewLinks);
                break;
            }
            Node = (PLCB)((PUCHAR)RtlRightChild(NodeLinks) - LinksOffset);
        }
    }

    return TRUE;
}


/*
    Insert LCB into all applicable splay trees of ParentFcb.
    Called after LCB names are initialized.
*/
VOID
UdfInsertNameLinks(
    IN PFCB ParentFcb,
    IN PLCB Lcb
    )
{
    // Exact case tree (keyed by FileName)
    if (Lcb->FileName.Buffer && Lcb->FileName.Length > 0) {
        if (UdfInsertNameLink(&ParentFcb->ExactCaseRoot, Lcb,
                              FIELD_OFFSET(struct LCB, ExactCaseLinks),
                              FIELD_OFFSET(struct LCB, FileName))) {
            SetFlag(Lcb->Flags, UDF_LCB_FLAG_EXACT_CASE_IN_TREE);
        }
    }

    // Ignore case tree (keyed by IgnoreCaseLinkName)
    if (Lcb->IgnoreCaseLinkName.Buffer && Lcb->IgnoreCaseLinkName.Length > 0) {
        if (UdfInsertNameLink(&ParentFcb->IgnoreCaseRoot, Lcb,
                              FIELD_OFFSET(struct LCB, IgnoreCaseLinks),
                              FIELD_OFFSET(struct LCB, IgnoreCaseLinkName))) {
            SetFlag(Lcb->Flags, UDF_LCB_FLAG_IGNORE_CASE_IN_TREE);
        }
    }

    // Short name tree (keyed by ShortName)
    if (Lcb->ShortName.Buffer && Lcb->ShortName.Length > 0) {
        if (UdfInsertNameLink(&ParentFcb->ShortNameRoot, Lcb,
                              FIELD_OFFSET(struct LCB, ShortNameLinks),
                              FIELD_OFFSET(struct LCB, ShortName))) {
            SetFlag(Lcb->Flags, UDF_LCB_FLAG_SHORT_NAME_IN_TREE);
        }
    }
}


/*
    Remove LCB from all splay trees of ParentFcb.
    Uses IN_TREE flags to track which trees the LCB was inserted into.
    After removal, clears all IN_TREE flags.
    Idempotent — safe to call multiple times.
*/
VOID
UdfRemoveNameLinks(
    IN PFCB ParentFcb,
    IN PLCB Lcb
    )
{
    if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_EXACT_CASE_IN_TREE)) {
        ParentFcb->ExactCaseRoot = RtlDelete(&Lcb->ExactCaseLinks);
    }

    if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_IGNORE_CASE_IN_TREE)) {
        ParentFcb->IgnoreCaseRoot = RtlDelete(&Lcb->IgnoreCaseLinks);
    }

    if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_SHORT_NAME_IN_TREE)) {
        ParentFcb->ShortNameRoot = RtlDelete(&Lcb->ShortNameLinks);
    }

    ClearFlag(Lcb->Flags, UDF_LCB_FLAG_EXACT_CASE_IN_TREE |
                           UDF_LCB_FLAG_IGNORE_CASE_IN_TREE |
                           UDF_LCB_FLAG_SHORT_NAME_IN_TREE);
}


/*
    This routine creates and inserts an LCB linking the parent FCB to child FCB.

    Creates LCB with dynamic name storage.
    Names are stored in a buffer immediately after the LCB structure.

    The LCB is linked into:
    - ParentFcb->ChildLcbQueue (via Lcb->ParentFcbLinks)
    - ChildFcb->ParentLcbQueue (via Lcb->ChildFcbLinks)
    - ParentFcb splay trees (ExactCaseRoot, IgnoreCaseRoot, ShortNameRoot)

    Parameters:
      IrpContext - IRP context
      ParentFcb - Parent directory FCB
      ChildFcb - Child file FCB
      FileName - File name (component name, may be NULL)
      CaseFileName - Case-sensitive file name (may be NULL)
      ShortName - 8.3 short name (may be NULL)
      InitialOffset - Directory offset
*/
PLCB
UDFInsertPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PUNICODE_STRING CaseFileName OPTIONAL,
    IN PUNICODE_STRING ShortName OPTIONAL,
    IN ULONGLONG InitialOffset
    )
{
    PLCB Lcb;
    ULONG Flags = 0;
    USHORT NameLength = 0;
    ULONG AllocationSize;
    PWCHAR NameBuffer;

    UNREFERENCED_PARAMETER(IrpContext);

    ASSERT(ParentFcb);
    ASSERT(ChildFcb);

    //
    // Calculate required allocation size for names
    // Layout: FileName + padding(0x18) + CaseFileName + ShortName
    //
    if (FileName != NULL && FileName->Length > 0) {
        // Size = FileName->Length + alignment(0x18) + CaseFileName->Length + ShortName->Length
        NameLength = FileName->Length + 0x18;
        if (CaseFileName != NULL && CaseFileName->Length > 0) {
            NameLength += CaseFileName->Length;
        }
        if (ShortName != NULL && ShortName->Length > 0) {
            NameLength += ShortName->Length;
        }
    }

    AllocationSize = UDF_LCB_BASE_SIZE + NameLength;

    //
    // Allocate LCB - use lookaside for small allocations
    //
    if (AllocationSize > UDF_LCB_LOOKASIDE_SIZE) {

        Lcb = (PLCB)FsRtlAllocatePoolWithTag(PagedPool,
                                              AllocationSize,
                                              TAG_LCB);
        if (!Lcb) {
            return NULL;
        }

        SetFlag(Flags, UDF_LCB_FLAG_POOL_ALLOCATED);

    } else {

        Lcb = (PLCB)ExAllocateFromPagedLookasideList(&UdfData.LcbLookasideList);
        if (!Lcb) {
            return NULL;
        }
    }

    //
    // Initialize LCB structure
    //
    UDFPrint(("UDFInsertPrefix: Lcb=%p, AllocationSize=0x%X, sizeof(LCB)=%u, NameLength=%u\n",
              Lcb, AllocationSize, (ULONG)sizeof(struct LCB), NameLength));

    ULONG BytesToZero = min(AllocationSize, UDF_LCB_BASE_SIZE);
    UDFPrint(("  BytesToZero=0x%X (min of AllocationSize and UDF_LCB_BASE_SIZE=0x%X)\n",
              BytesToZero, UDF_LCB_BASE_SIZE));

    RtlZeroMemory(Lcb, BytesToZero);

    Lcb->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_LCB;
    Lcb->NodeIdentifier.NodeByteSize = (USHORT)AllocationSize;

    // Set up FCB pointers
    Lcb->ParentFcb = ParentFcb;
    Lcb->ChildFcb = ChildFcb;
    Lcb->InitialOffset = InitialOffset;

    // Set flags
    Lcb->Flags = Flags;

    // Initial reference count
    Lcb->Reference = 0;

    //
    // Initialize name buffer pointer
    // Buffer is located immediately after LCB structure
    //
    if (NameLength > 0) {
        Lcb->NameBuffer = (PVOID)((PUCHAR)Lcb + UDF_LCB_BASE_SIZE);
        Lcb->FileName.MaximumLength = NameLength;

        NameBuffer = (PWCHAR)Lcb->NameBuffer;

        //
        // Copy file names into buffer
        // Layout: [FileName][padding 0x18][CaseFileName][ShortName]
        //
        if (FileName != NULL && FileName->Length > 0) {
            // Copy FileName (exact case)
            Lcb->FileName.Buffer = NameBuffer;
            Lcb->FileName.Length = FileName->Length;
            RtlCopyMemory(NameBuffer, FileName->Buffer, FileName->Length);

            // Mirror the exact-case component name. It aliases the same
            // embedded buffer (never freed separately), consistent with
            // UDFRenameMovePrefix. Change-notify offset computation and
            // full-path building read ExactCaseLinkName; without this it
            // stayed zero-length on freshly created links, which forced a
            // TargetNameOffset of 0 in UDFNotifyReportChange — so FsRtl saw
            // an empty parent directory and no registered notify matched,
            // and Explorer would not refresh added/removed entries until F5.
            Lcb->ExactCaseLinkName = Lcb->FileName;

            // Copy CaseFileName (uppercase for ignore-case comparisons)
            if (CaseFileName != NULL && CaseFileName->Length > 0) {
                // Skip alignment padding (0x18 bytes) after FileName
                NameBuffer = (PWCHAR)((PUCHAR)Lcb->NameBuffer + FileName->Length + 0x18);

                Lcb->IgnoreCaseLinkName.Buffer = NameBuffer;
                Lcb->IgnoreCaseLinkName.Length = CaseFileName->Length;
                Lcb->IgnoreCaseLinkName.MaximumLength = CaseFileName->Length;
                RtlCopyMemory(NameBuffer, CaseFileName->Buffer, CaseFileName->Length);

                // Move pointer to next position for ShortName
                NameBuffer += CaseFileName->Length / sizeof(WCHAR);
            } else {
                // No CaseFileName, position after FileName + padding
                NameBuffer = (PWCHAR)((PUCHAR)Lcb->NameBuffer + FileName->Length + 0x18);
            }

            // Copy ShortName (8.3 DOS name)
            if (ShortName != NULL && ShortName->Length > 0) {
                Lcb->ShortName.Buffer = NameBuffer;
                Lcb->ShortName.Length = ShortName->Length;
                Lcb->ShortName.MaximumLength = ShortName->Length;
                RtlCopyMemory(NameBuffer, ShortName->Buffer, ShortName->Length);

                // Set flag indicating short name was created
                SetFlag(Lcb->Flags, UDF_LCB_FLAG_SHORT_NAME_CREATED);
            }
        }
    }

    //
    // Link LCB into the FCB queues
    //
    // Insert into parent's child queue
    InsertHeadList(&ParentFcb->ChildLcbQueue, &Lcb->ParentFcbLinks);
    // Insert into child's parent queue
    InsertHeadList(&ChildFcb->ParentLcbQueue, &Lcb->ChildFcbLinks);

    // Insert into splay trees for O(log n) lookup by name.
    // Caller must hold ParentFcb exclusive for splay tree protection.
    UdfInsertNameLinks(ParentFcb, Lcb);

    return Lcb;
} // end UDFInsertPrefix()


/*
    This routine removes an LCB from FCB queues and frees it.
    Frees name buffer if separately allocated, then frees LCB structure
    itself (from pool or lookaside list).
*/
VOID
UDFRemovePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    )
{
    UNREFERENCED_PARAMETER(IrpContext);

    if (!Lcb) {
        return;
    }

    ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);

    //
    // Remove from parent FCB's splay trees.
    // All callers hold ParentFcb exclusive or VcbResource exclusive.
    //
    if (Lcb->ParentFcb) {
        UdfRemoveNameLinks(Lcb->ParentFcb, Lcb);
    }

    //
    // Remove from parent FCB's ChildLcbQueue
    //
    RemoveEntryList(&Lcb->ParentFcbLinks);
    InitializeListHead(&Lcb->ParentFcbLinks);

    //
    // Remove from child FCB's ParentLcbQueue
    //
    RemoveEntryList(&Lcb->ChildFcbLinks);
    InitializeListHead(&Lcb->ChildFcbLinks);

    //
    // Free name buffer if separately allocated
    // This flag indicates the NameBuffer was allocated independently,
    // not as part of the LCB structure
    //
    if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_HAS_NAME_BUFFER)) {
        if (Lcb->NameBuffer != NULL) {
            ExFreePoolWithTag(Lcb->NameBuffer, 0);
            Lcb->NameBuffer = NULL;
        }
    }

    //
    // Free FileName.Buffer if it was separately allocated (e.g., after rename)
    // Check if it's not pointing to embedded buffer (within LCB structure)
    //
    if (Lcb->FileName.Buffer != NULL) {
        BOOLEAN IsEmbedded = ((PUCHAR)Lcb->FileName.Buffer >= (PUCHAR)Lcb &&
                              (PUCHAR)Lcb->FileName.Buffer < (PUCHAR)Lcb + Lcb->NodeIdentifier.NodeByteSize);
        if (!IsEmbedded) {
            MyFreePool__(Lcb->FileName.Buffer);
            Lcb->FileName.Buffer = NULL;
        }
    }

    //
    // Free Name.Buffer if present (ANSI name)
    //
    if (Lcb->Name.Buffer != NULL) {
        ExFreePoolWithTag(Lcb->Name.Buffer, 0);
        Lcb->Name.Buffer = NULL;
    }

    //
    // Free the LCB structure itself based on how it was allocated
    //
    if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_POOL_ALLOCATED)) {

        ExFreePoolWithTag(Lcb, 0);

    } else {

        ExFreeToPagedLookasideList(&UdfData.LcbLookasideList, Lcb);
    }
} // end UDFRemovePrefix()


/*
    This routine finds an existing LCB linking the parent FCB to child FCB.
    Returns NULL if no such LCB exists.
*/
PLCB
UDFFindPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb
    )
{
    PLIST_ENTRY ListEntry;
    PLCB Lcb;

    UNREFERENCED_PARAMETER(IrpContext);

    ASSERT(ParentFcb);
    ASSERT(ChildFcb);

    //
    // Walk the child FCB's ParentLcbQueue to find LCB pointing to this parent
    //
    for (ListEntry = ChildFcb->ParentLcbQueue.Flink;
         ListEntry != &ChildFcb->ParentLcbQueue;
         ListEntry = ListEntry->Flink) {

        Lcb = CONTAINING_RECORD(ListEntry, struct LCB, ChildFcbLinks);

        ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);

        if (Lcb->ParentFcb == ParentFcb) {
            return Lcb;
        }
    }

    return NULL;
} // end UDFFindPrefix()


/*
    This routine acquires or creates an LCB linking the parent FCB to child FCB.
    If LCB already exists, increments its Reference count.
    If new LCB is created:
    - Increments parent FCB's FcbReference
    - Increments parent's FileInfo->RefCount (if available)

    This is the primary function to call when establishing a parent-child
    relationship during file open.

    Returns the LCB (existing or newly created), or NULL on allocation failure.
*/
PLCB
UDFAcquirePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PFCB ChildFcb,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PUNICODE_STRING CaseFileName OPTIONAL,
    IN PUNICODE_STRING ShortName OPTIONAL,
    IN ULONGLONG InitialOffset
    )
{
    PLCB Lcb;

    UNREFERENCED_PARAMETER(IrpContext);

    ASSERT(ParentFcb);
    ASSERT(ChildFcb);

    //
    // First try to find existing LCB
    //
    Lcb = UDFFindPrefix(IrpContext, ParentFcb, ChildFcb);

    if (Lcb) {
        //
        // Found existing LCB - return it as-is.
        return Lcb;
    }

    //
    // Need to create new LCB with dynamic names
    //
    Lcb = UDFInsertPrefix(IrpContext, ParentFcb, ChildFcb,
                          FileName, CaseFileName, ShortName, InitialOffset);

    if (Lcb) {
        //
        // New LCB created - Reference=0 and stays 0.
        // Lcb->Reference is only used for temporary lock protection,
        // not for counting opens. Teardown checks Reference==0 before removal.
        //
        Lcb->Reference = 0;

        //
        // Increment parent refs immediately after LCB creation.
        // These will be decremented in UDFTeardownStructures when LCB is removed.
        //
        InterlockedIncrement((PLONG)&ParentFcb->FcbReference);
        if (ParentFcb->FileInfo) {
            UDFReferenceFile__(ParentFcb->FileInfo);
        }
    }

    return Lcb;
} // end UDFAcquirePrefix()


/*
    This routine releases a reference to an LCB.
    If Reference goes to zero, the LCB is eligible for removal during teardown.

    Note: This does NOT remove the LCB or decrement parent's FcbReference.
    That is done during teardown when walking the ParentLcbQueue.
*/
VOID
UDFReleasePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    )
{
    UNREFERENCED_PARAMETER(IrpContext);

    if (!Lcb) {
        return;
    }

    ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);
    ASSERT(Lcb->Reference > 0);

    InterlockedDecrement((PLONG)&Lcb->Reference);
} // end UDFReleasePrefix()


/*
    Builds full path from LCB chain with caching support.

    Builds path by walking up the parent LCB chain from current LCB to root.

    Caching strategy:
    - If Lcb->FileName.Buffer already contains full path → use cached value
    - Otherwise, build path recursively and optionally cache it
    - Uses FcbLockThread/FcbLockCount/FcbFastMutex for synchronization

    Parameters:
        IrpContext - IRP context
        Lcb - LCB to build path for (starting point)
        FullPath - Output UNICODE_STRING for full path
        CachePath - If TRUE, cache the built path in LCB->FileName

    Returns:
        STATUS_SUCCESS - Path built successfully
        STATUS_INSUFFICIENT_RESOURCES - Memory allocation failed
*/
NTSTATUS
UDFBuildFullPathFromLcb(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    OUT PUNICODE_STRING FullPath,
    IN BOOLEAN CachePath
    )
{
    PLCB CurrentLcb;
    PFCB CurrentFcb;
    PFCB Fcb;
    USHORT TotalLength = 0;
    USHORT ComponentLength;
    _SEH2_VOLATILE PWCHAR Buffer = NULL;
    PWCHAR CurrentPosition;
    PKTHREAD CurrentThread;
    _SEH2_VOLATILE BOOLEAN CacheLockAcquired = FALSE;
    _SEH2_VOLATILE NTSTATUS Status = STATUS_SUCCESS;

    if (!Lcb) {
        return STATUS_INVALID_PARAMETER;
    }

    CurrentFcb = Lcb->ChildFcb;
    if (!CurrentFcb) {
        return STATUS_INVALID_PARAMETER;
    }

    Fcb = CurrentFcb;

    _SEH2_TRY {

        // Check if path is already cached in this LCB
        CurrentThread = KeGetCurrentThread();

        if (CurrentThread != Fcb->FcbLockThread) {
            ExAcquireFastMutex(&Fcb->FcbNonpaged->FcbFastMutex);
            Fcb->FcbLockThread = CurrentThread;
        }

        Fcb->FcbLockCount++;
        CacheLockAcquired = TRUE;

        // Check if FileName contains cached full path (set by CachePath=TRUE)
        if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_HAS_NAME_BUFFER) &&
            Lcb->FileName.Buffer != NULL && Lcb->FileName.Length > 0) {
            // Cached full path exists - use it
            FullPath->Length = Lcb->FileName.Length;
            FullPath->MaximumLength = Lcb->FileName.MaximumLength;
            FullPath->Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                                             FullPath->MaximumLength,
                                                             TAG_FILE_NAME);
            if (!FullPath->Buffer) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            RtlCopyMemory(FullPath->Buffer, Lcb->FileName.Buffer, Lcb->FileName.Length);
            _SEH2_LEAVE;
        }

        // Release cache lock before building path
        Fcb->FcbLockCount--;
        if (Fcb->FcbLockCount == 0) {
            Fcb->FcbLockThread = NULL;
            ExReleaseFastMutex(&Fcb->FcbNonpaged->FcbFastMutex);
        }
        CacheLockAcquired = FALSE;

        //
        // No cached path - build it from LCB chain
        //

        // Calculate total path length by walking up parent chain
        CurrentLcb = Lcb;
        while (CurrentLcb) {
            // Add component length + separator
            if (CurrentLcb->FileName.Buffer && CurrentLcb->FileName.Length > 0) {
                ComponentLength = CurrentLcb->FileName.Length;
            } else if (CurrentLcb->ExactCaseLinkName.Buffer && CurrentLcb->ExactCaseLinkName.Length > 0) {
                ComponentLength = CurrentLcb->ExactCaseLinkName.Length;
            } else {
                ComponentLength = 0;
            }

            if (ComponentLength > 0) {
                TotalLength += ComponentLength + sizeof(WCHAR); // +1 for backslash
            }

            // Move to parent
            CurrentFcb = CurrentLcb->ParentFcb;
            if (!CurrentFcb || (CurrentFcb->FcbState & UDF_FCB_ROOT_DIRECTORY)) {
                break;
            }

            // Find parent LCB
            if (!IsListEmpty(&CurrentFcb->ParentLcbQueue)) {
                PLIST_ENTRY ListEntry = CurrentFcb->ParentLcbQueue.Flink;
                CurrentLcb = CONTAINING_RECORD(ListEntry, struct LCB, ChildFcbLinks);
            } else {
                break;
            }
        }

        // Add root backslash
        if (TotalLength == 0) {
            TotalLength = sizeof(WCHAR);
        }

        // Allocate buffer for full path
        Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool, TotalLength + sizeof(WCHAR), TAG_FILE_NAME);
        if (!Buffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        // Build path backwards (from end to beginning)
        CurrentPosition = (PWCHAR)((PUCHAR)Buffer + TotalLength);
        *CurrentPosition = 0; // Null terminator

        // Walk up chain again and copy names
        CurrentLcb = Lcb;
        while (CurrentLcb) {
            PUNICODE_STRING ComponentName;

            if (CurrentLcb->FileName.Buffer && CurrentLcb->FileName.Length > 0) {
                ComponentName = &CurrentLcb->FileName;
            } else if (CurrentLcb->ExactCaseLinkName.Buffer && CurrentLcb->ExactCaseLinkName.Length > 0) {
                ComponentName = &CurrentLcb->ExactCaseLinkName;
            } else {
                ComponentName = NULL;
            }

            if (ComponentName && ComponentName->Length > 0) {
                // Prepend "\<component>". The buffer is filled back-to-front,
                // so write the name first, then the separator immediately in
                // front of it. (Writing the separator after the name would
                // produce "<component>\" with no leading backslash — a
                // relative, malformed path that no registered notify matches.)
                CurrentPosition -= (ComponentName->Length / sizeof(WCHAR));
                RtlCopyMemory(CurrentPosition, ComponentName->Buffer, ComponentName->Length);

                CurrentPosition--;
                *CurrentPosition = L'\\';
            }

            // Move to parent
            CurrentFcb = CurrentLcb->ParentFcb;
            if (!CurrentFcb || (CurrentFcb->FcbState & UDF_FCB_ROOT_DIRECTORY)) {
                break;
            }

            // Find parent LCB
            if (!IsListEmpty(&CurrentFcb->ParentLcbQueue)) {
                PLIST_ENTRY ListEntry = CurrentFcb->ParentLcbQueue.Flink;
                CurrentLcb = CONTAINING_RECORD(ListEntry, struct LCB, ChildFcbLinks);
            } else {
                break;
            }
        }

        // Add leading backslash if not already there
        if (CurrentPosition > Buffer) {
            CurrentPosition--;
            *CurrentPosition = L'\\';
        }

        // Set output string
        FullPath->Buffer = Buffer;
        FullPath->Length = (USHORT)((PUCHAR)Buffer + TotalLength - (PUCHAR)CurrentPosition);
        FullPath->MaximumLength = TotalLength + sizeof(WCHAR);

        // Move string to beginning of buffer if needed
        if (CurrentPosition != Buffer) {
            RtlMoveMemory(Buffer, CurrentPosition, FullPath->Length + sizeof(WCHAR));
            FullPath->Length = (USHORT)((PUCHAR)Buffer + TotalLength - (PUCHAR)CurrentPosition);
        }

        // Optionally cache the path in LCB
        if (CachePath && NT_SUCCESS(Status)) {
            CurrentThread = KeGetCurrentThread();
            Fcb = Lcb->ChildFcb;

            if (CurrentThread != Fcb->FcbLockThread) {
                ExAcquireFastMutex(&Fcb->FcbNonpaged->FcbFastMutex);
                Fcb->FcbLockThread = CurrentThread;
            }

            Fcb->FcbLockCount++;
            CacheLockAcquired = TRUE;

            // Check if cache is still empty
            if (Lcb->FileName.Buffer == NULL) {
                // Allocate separate buffer for cache
                PWCHAR CacheBuffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                                                    FullPath->Length + sizeof(WCHAR),
                                                                    TAG_FILE_NAME);
                if (CacheBuffer) {
                    RtlCopyMemory(CacheBuffer, FullPath->Buffer, FullPath->Length + sizeof(WCHAR));
                    Lcb->FileName.Buffer = CacheBuffer;
                    Lcb->FileName.Length = FullPath->Length;
                    Lcb->FileName.MaximumLength = FullPath->Length + sizeof(WCHAR);

                    // Set flag indicating separate buffer needs freeing
                    SetFlag(Lcb->Flags, UDF_LCB_FLAG_HAS_NAME_BUFFER);
                }
            }
        }

    } _SEH2_FINALLY {

        if (CacheLockAcquired) {
            Fcb->FcbLockCount--;
            if (Fcb->FcbLockCount == 0) {
                Fcb->FcbLockThread = NULL;
                ExReleaseFastMutex(&Fcb->FcbNonpaged->FcbFastMutex);
            }
        }

        if (!NT_SUCCESS(Status) && Buffer) {
            ExFreePoolWithTag(Buffer, 0);
            FullPath->Buffer = NULL;
            FullPath->Length = 0;
            FullPath->MaximumLength = 0;
        }
    } _SEH2_END;

    return Status;
} // end UDFBuildFullPathFromLcb()


/*
    This routine releases a reference to an LCB and, if Reference becomes 0,
    immediately removes the LCB and decrements parent references.

    This is used during rename operations where we need immediate cleanup
    of the old parent link rather than waiting for teardown.

    Parameters:
        IrpContext - The IRP context
        Lcb - The LCB to release
        CloseParentFileInfo - If TRUE and LCB is removed, call UDFCloseFile__
                              on parent's FileInfo

    Returns:
        TRUE if the LCB was removed, FALSE if it still has references
*/
BOOLEAN
UDFReleasePrefixImmediate(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN BOOLEAN CloseParentFileInfo
    )
{
    PFCB ParentFcb;
    PUDF_FILE_INFO ParentFileInfo;
    PVCB Vcb;
    LONG NewRef;

    if (!Lcb) {
        return FALSE;
    }

    ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);
    ASSERT(Lcb->Reference > 0);

    //
    // Decrement reference
    //
    NewRef = InterlockedDecrement((PLONG)&Lcb->Reference);

    if (NewRef > 0) {
        //
        // Still has references, leave it in place
        //
        return FALSE;
    }

    //
    // Reference is now 0 - remove LCB and decrement parent refs
    //
    ParentFcb = Lcb->ParentFcb;
    ParentFileInfo = ParentFcb ? ParentFcb->FileInfo : NULL;
    Vcb = ParentFcb ? ParentFcb->Vcb : NULL;

    //
    // Remove LCB from queues and free it
    //
    UDFRemovePrefix(IrpContext, Lcb);

    //
    // Decrement parent's FcbReference
    //
    if (ParentFcb) {
        ASSERT(ParentFcb->FcbReference > 0);
        InterlockedDecrement((PLONG)&ParentFcb->FcbReference);

        //
        // Close parent's FileInfo if requested
        //
        if (CloseParentFileInfo && ParentFileInfo && Vcb) {
            UDFCloseFile__(IrpContext, Vcb, ParentFileInfo);
        }
    }

    return TRUE;
} // end UDFReleasePrefixImmediate()


/*
    This routine searches for the maximum known prefix path in the LCB tree.

    Starting from StartFcb, walks through path components in RemainingName,
    looking for matching LCBs in the ChildLcbQueue. Returns the LCB of the
    deepest matched component and updates RemainingName to the unmatched portion.

    This optimization avoids redundant directory traversal - instead of always
    starting from root, we find the deepest already-opened directory in the
    path and start from there.

    Parameters:
        IrpContext - IRP context
        StartFcb - Starting FCB (usually RootIndexFcb or RelatedFcb)
        IgnoreCase - Whether to use case-insensitive comparison
        CurrentFcb - Output: FCB of the deepest matched component
        RemainingName - Input: full path to search; Output: unmatched portion
        ShortNameMatch - Output: TRUE if match was via short name

    Returns:
        PLCB - LCB of the deepest matched component, or NULL if no match
*/
/*
    Walk path components starting from StartFcb, looking for the longest
    match in the in-memory LCB tree.

    On entry:  *CurrentFcb == StartFcb, caller holds StartFcb FcbResource exclusive.
    On return: *CurrentFcb == deepest matched FCB (locked exclusive),
               all ancestor FCB locks released.
               RemainingName is advanced past matched components.
    Returns the LCB used to reach *CurrentFcb, or NULL if no prefix found.
 */
PLCB
UDFFindPathPrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartFcb,
    IN BOOLEAN IgnoreCase,
    IN OUT PFCB *CurrentFcb,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PBOOLEAN ShortNameMatch
    )
{
    PLCB CurrentLcb = NULL;
    PLCB MatchLcb;
    UNICODE_STRING LocalRemainingName;
    UNICODE_STRING ComponentName;
    WCHAR UpcaseBuffer[256];
    UNICODE_STRING UpcaseName;

    ASSERT(StartFcb);
    ASSERT(CurrentFcb);
    ASSERT(RemainingName);
    ASSERT(ShortNameMatch);

    *ShortNameMatch = FALSE;
    *CurrentFcb = StartFcb;
    LocalRemainingName = *RemainingName;

    while (TRUE) {

        // Stop if no more path components or current FCB is not a directory
        if (LocalRemainingName.Length == 0 ||
            !(*CurrentFcb) ||
            !((*CurrentFcb)->FcbState & UDF_FCB_DIRECTORY)) {
            return CurrentLcb;
        }

        // Skip leading backslashes
        while (LocalRemainingName.Length >= sizeof(WCHAR) &&
               LocalRemainingName.Buffer[0] == L'\\') {
            LocalRemainingName.Buffer++;
            LocalRemainingName.Length -= sizeof(WCHAR);
        }

        if (LocalRemainingName.Length == 0) {
            return CurrentLcb;
        }

        // Stop at stream boundary
        if (LocalRemainingName.Buffer[0] == L':') {
            return CurrentLcb;
        }

        // Extract next component (up to next '\' or ':' or end)
        ComponentName.Buffer = LocalRemainingName.Buffer;
        ComponentName.Length = 0;
        while (ComponentName.Length < LocalRemainingName.Length) {
            WCHAR ch = ComponentName.Buffer[ComponentName.Length / sizeof(WCHAR)];
            if (ch == L'\\' || ch == L':') {
                break;
            }
            ComponentName.Length += sizeof(WCHAR);
        }
        if (ComponentName.Length == 0) {
            return CurrentLcb;
        }
        ComponentName.MaximumLength = ComponentName.Length;

        // Advance local remaining name past this component
        LocalRemainingName.Buffer += ComponentName.Length / sizeof(WCHAR);
        LocalRemainingName.Length -= ComponentName.Length;

        //
        // Search for matching component. Try splay trees first (O(log n)),
        // then fall back to list iteration (O(n)) for LCBs without names,
        // duplicate-name LCBs, or names too long for upcase buffer.
        // *CurrentFcb is held exclusive, so trees and queue are safe.
        //
        MatchLcb = NULL;

        // Splay tree search: try IgnoreCase or ExactCase tree
        if (IgnoreCase && (*CurrentFcb)->IgnoreCaseRoot != NULL) {
            // Upcase component for ignore-case tree lookup
            if (ComponentName.Length <= sizeof(UpcaseBuffer)) {
                UpcaseName.Buffer = UpcaseBuffer;
                UpcaseName.MaximumLength = sizeof(UpcaseBuffer);
                UpcaseName.Length = ComponentName.Length;
                RtlCopyMemory(UpcaseBuffer, ComponentName.Buffer, ComponentName.Length);
                RtlUpcaseUnicodeString(&UpcaseName, &UpcaseName, FALSE);

                MatchLcb = UdfFindNameLink(&(*CurrentFcb)->IgnoreCaseRoot,
                                            &UpcaseName,
                                            FIELD_OFFSET(struct LCB, IgnoreCaseLinks),
                                            FIELD_OFFSET(struct LCB, IgnoreCaseLinkName));
            }
        } else if (!IgnoreCase && (*CurrentFcb)->ExactCaseRoot != NULL) {
            MatchLcb = UdfFindNameLink(&(*CurrentFcb)->ExactCaseRoot,
                                        &ComponentName,
                                        FIELD_OFFSET(struct LCB, ExactCaseLinks),
                                        FIELD_OFFSET(struct LCB, FileName));
        }

        // Try short name splay tree if no match yet
        if (!MatchLcb && (*CurrentFcb)->ShortNameRoot != NULL) {
            if (ComponentName.Length <= sizeof(UpcaseBuffer)) {
                UpcaseName.Buffer = UpcaseBuffer;
                UpcaseName.MaximumLength = sizeof(UpcaseBuffer);
                UpcaseName.Length = ComponentName.Length;
                RtlCopyMemory(UpcaseBuffer, ComponentName.Buffer, ComponentName.Length);
                RtlUpcaseUnicodeString(&UpcaseName, &UpcaseName, FALSE);

                MatchLcb = UdfFindNameLink(&(*CurrentFcb)->ShortNameRoot,
                                            &UpcaseName,
                                            FIELD_OFFSET(struct LCB, ShortNameLinks),
                                            FIELD_OFFSET(struct LCB, ShortName));
                if (MatchLcb) {
                    *ShortNameMatch = TRUE;
                }
            }
        }

        // List fallback: catches LCBs without names in splay trees
        // (NULL names, duplicates, long names) and handles deleted-link filtering.
        if (!MatchLcb) {
            for (PLIST_ENTRY ListEntry = (*CurrentFcb)->ChildLcbQueue.Flink;
                 ListEntry != &(*CurrentFcb)->ChildLcbQueue;
                 ListEntry = ListEntry->Flink) {

                PLCB Lcb = CONTAINING_RECORD(ListEntry, struct LCB, ParentFcbLinks);

                ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);
                ASSERT(Lcb->ParentFcb == *CurrentFcb);

                if (FlagOn(Lcb->Flags, UDF_LCB_FLAG_LINK_DELETED)) {
                    continue;
                }

                // Try FileName (exact or case-insensitive)
                if (Lcb->FileName.Buffer && Lcb->FileName.Length > 0 &&
                    RtlEqualUnicodeString(&ComponentName, &Lcb->FileName, IgnoreCase)) {
                    MatchLcb = Lcb;
                    break;
                }
                // Try IgnoreCaseLinkName
                if (IgnoreCase &&
                    Lcb->IgnoreCaseLinkName.Buffer && Lcb->IgnoreCaseLinkName.Length > 0 &&
                    RtlEqualUnicodeString(&ComponentName, &Lcb->IgnoreCaseLinkName, TRUE)) {
                    MatchLcb = Lcb;
                    break;
                }
                // Try ShortName
                if (Lcb->ShortName.Buffer && Lcb->ShortName.Length > 0 &&
                    RtlEqualUnicodeString(&ComponentName, &Lcb->ShortName, TRUE)) {
                    MatchLcb = Lcb;
                    *ShortNameMatch = TRUE;
                    break;
                }
            }
        }

        //
        // Validate the match: skip deleted links and deleted FCBs.
        // A deleted FCB can still have LCBs in the splay tree/list if
        // teardown hasn't run yet (between cleanup and close).
        // Clean up stale splay entries when found.
        //
        if (MatchLcb && !FlagOn(MatchLcb->Flags, UDF_LCB_FLAG_LINK_DELETED) &&
            MatchLcb->ChildFcb &&
            (MatchLcb->ChildFcb->FcbState & UDF_FCB_DELETED)) {
            // Child FCB was deleted — remove LCB from splay trees
            // (we hold ParentFcb exclusive) and mark as deleted.
            UdfRemoveNameLinks(*CurrentFcb, MatchLcb);
            MatchLcb->Flags |= UDF_LCB_FLAG_LINK_DELETED;
            MatchLcb = NULL;
        }

        if (!MatchLcb || FlagOn(MatchLcb->Flags, UDF_LCB_FLAG_LINK_DELETED)) {
            // No match or deleted link — stop, *CurrentFcb stays locked
            return CurrentLcb;
        }

        // Found a match. Descend into child FCB with proper locking:
        // acquire child exclusive, then release parent.
        CurrentLcb = MatchLcb;
        *RemainingName = LocalRemainingName;

        PFCB ChildFcb = MatchLcb->ChildFcb;

        UDF_CHECK_PAGING_IO_RESOURCE(ChildFcb);
        if (!UDFAcquireFcbExclusive(IrpContext, ChildFcb, TRUE)) {
            // Try-lock failed. Bump references to keep child alive,
            // release parent, then wait-acquire child.
            PVCB Vcb = (PVCB)IrpContext->Vcb;

            UDFLockVcb(IrpContext, Vcb);
            ChildFcb->FcbReference += 1;
            MatchLcb->Reference += 1;
            UDFUnlockVcb(IrpContext, Vcb);

            UDFReleaseFcb(IrpContext, *CurrentFcb);

            UDFAcquireFcbExclusive(IrpContext, ChildFcb, FALSE);

            UDFLockVcb(IrpContext, Vcb);
            ChildFcb->FcbReference -= 1;
            MatchLcb->Reference -= 1;
            UDFUnlockVcb(IrpContext, Vcb);
        } else {
            // Try-lock succeeded. Release parent.
            UDFReleaseFcb(IrpContext, *CurrentFcb);
        }

        *CurrentFcb = ChildFcb;
    }

    return CurrentLcb;
} // end UDFFindPathPrefix()


/*
    Atomically rename an LCB and optionally move it to a new parent directory.
    Updates the file name, moves between parent ChildLcbQueue lists,
    and adjusts parent reference counts — all in one call.

    NewParentFcb = NULL for same-directory rename (name change only).
*/
NTSTATUS
UDFRenameMovePrefix(
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PUNICODE_STRING NewName,
    IN PFCB NewParentFcb OPTIONAL
    )
{
    PWCHAR NewBuffer;
    USHORT NewLength;
    BOOLEAN OldBufferEmbedded;

    if (!Lcb || !NewName || NewName->Length == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);

    NewLength = NewName->Length;

    // Allocate new buffer for the name
    NewBuffer = (PWCHAR)MyAllocatePool__(NonPagedPool, NewLength + sizeof(WCHAR));
    if (!NewBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewBuffer, NewName->Buffer, NewLength);
    NewBuffer[NewLength / sizeof(WCHAR)] = 0;

    // Remove from current parent's splay trees before changing names.
    // Caller must hold ParentFcb exclusive (enforced in fileinfo.cpp).
    if (Lcb->ParentFcb) {
        UdfRemoveNameLinks(Lcb->ParentFcb, Lcb);
    }

    // Check if old buffer was embedded in LCB structure
    // or separately allocated. Only free if separately allocated.
    OldBufferEmbedded = (Lcb->FileName.Buffer != NULL &&
                         (PUCHAR)Lcb->FileName.Buffer >= (PUCHAR)Lcb &&
                         (PUCHAR)Lcb->FileName.Buffer < (PUCHAR)Lcb + Lcb->NodeIdentifier.NodeByteSize);

    if (Lcb->FileName.Buffer && !OldBufferEmbedded) {
        MyFreePool__(Lcb->FileName.Buffer);
    }

    // Set new name (component name, not cached full path)
    Lcb->FileName.Buffer = NewBuffer;
    Lcb->FileName.Length = NewLength;
    Lcb->FileName.MaximumLength = NewLength + sizeof(WCHAR);

    // Clear cached full path flag — FileName now holds component name only
    ClearFlag(Lcb->Flags, UDF_LCB_FLAG_HAS_NAME_BUFFER);

    // Also update ExactCaseLinkName to point to same buffer
    Lcb->ExactCaseLinkName = Lcb->FileName;

    // Free old IgnoreCaseLinkName if separately allocated
    if (Lcb->IgnoreCaseLinkName.Buffer != NULL) {
        BOOLEAN IgnoreCaseEmbedded = ((PUCHAR)Lcb->IgnoreCaseLinkName.Buffer >= (PUCHAR)Lcb &&
                                      (PUCHAR)Lcb->IgnoreCaseLinkName.Buffer < (PUCHAR)Lcb + Lcb->NodeIdentifier.NodeByteSize);
        if (!IgnoreCaseEmbedded) {
            ExFreePoolWithTag(Lcb->IgnoreCaseLinkName.Buffer, 0);
        }
    }

    // Rebuild IgnoreCaseLinkName — upcased version of new name.
    // Required for IgnoreCase splay tree lookup (Windows opens case-insensitive).
    {
        PWCHAR UpcaseBuffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                                             NewLength + sizeof(WCHAR),
                                                             TAG_FILE_NAME);
        if (UpcaseBuffer) {
            Lcb->IgnoreCaseLinkName.Buffer = UpcaseBuffer;
            Lcb->IgnoreCaseLinkName.Length = NewLength;
            Lcb->IgnoreCaseLinkName.MaximumLength = NewLength + sizeof(WCHAR);
            RtlCopyMemory(UpcaseBuffer, NewBuffer, NewLength);
            UpcaseBuffer[NewLength / sizeof(WCHAR)] = 0;
            RtlUpcaseUnicodeString(&Lcb->IgnoreCaseLinkName, &Lcb->IgnoreCaseLinkName, FALSE);
        } else {
            // Allocation failed — clear IgnoreCaseLinkName.
            // File will still be findable via list fallback.
            Lcb->IgnoreCaseLinkName.Buffer = NULL;
            Lcb->IgnoreCaseLinkName.Length = 0;
            Lcb->IgnoreCaseLinkName.MaximumLength = 0;
        }
    }

    // Clear stale ShortName — old 8.3 name is invalid after rename.
    // The embedded buffer data stays in LCB allocation but won't be used.
    // Short name will be regenerated on next open if needed.
    if (Lcb->ShortName.Buffer != NULL) {
        BOOLEAN ShortNameEmbedded = ((PUCHAR)Lcb->ShortName.Buffer >= (PUCHAR)Lcb &&
                                     (PUCHAR)Lcb->ShortName.Buffer < (PUCHAR)Lcb + Lcb->NodeIdentifier.NodeByteSize);
        if (!ShortNameEmbedded) {
            ExFreePoolWithTag(Lcb->ShortName.Buffer, 0);
        }
        Lcb->ShortName.Buffer = NULL;
        Lcb->ShortName.Length = 0;
        Lcb->ShortName.MaximumLength = 0;
    }

    // Move LCB to new parent directory if specified (cross-directory rename)
    if (NewParentFcb && NewParentFcb != Lcb->ParentFcb) {

        PFCB OldParentFcb = Lcb->ParentFcb;
        PUDF_FILE_INFO OldParentFileInfo = OldParentFcb ? OldParentFcb->FileInfo : NULL;

        // Remove from old parent's ChildLcbQueue
        RemoveEntryList(&Lcb->ParentFcbLinks);

        // Insert into new parent's ChildLcbQueue
        InsertHeadList(&NewParentFcb->ChildLcbQueue, &Lcb->ParentFcbLinks);

        // Update LCB to point to new parent
        Lcb->ParentFcb = NewParentFcb;

        // Adjust reference counts
        if (OldParentFcb) {
            ASSERT(OldParentFcb->FcbReference > 0);
            InterlockedDecrement((PLONG)&OldParentFcb->FcbReference);

            if (OldParentFileInfo && OldParentFcb->Vcb) {
                UDFCloseFile__(IrpContext, OldParentFcb->Vcb, OldParentFileInfo);
            }
        }

        InterlockedIncrement((PLONG)&NewParentFcb->FcbReference);

        if (NewParentFcb->FileInfo) {
            UDFReferenceFile__(NewParentFcb->FileInfo);
        }
    }

    // Re-insert into splay trees of (possibly new) parent.
    // Caller must hold the (new) ParentFcb exclusive.
    if (Lcb->ParentFcb) {
        UdfInsertNameLinks(Lcb->ParentFcb, Lcb);
    }

    return STATUS_SUCCESS;
} // end UDFRenameMovePrefix()
