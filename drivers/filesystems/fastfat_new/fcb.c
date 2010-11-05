/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/fcb.c
 * PURPOSE:         FCB manipulation routines.
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

#define TAG_FILENAME 'fBnF'

/* FUNCTIONS ****************************************************************/

FSRTL_COMPARISON_RESULT
NTAPI
FatiCompareNames(PSTRING NameA,
                 PSTRING NameB)
{
    ULONG MinimumLen, i;

    /* Calc the minimum length */
    MinimumLen = NameA->Length < NameB->Length ? NameA->Length :
                                                NameB->Length;

    /* Actually compare them */
    i = (ULONG)RtlCompareMemory( NameA->Buffer, NameB->Buffer, MinimumLen );

    if (i < MinimumLen)
    {
        /* Compare prefixes */
        if (NameA->Buffer[i] < NameB->Buffer[i])
            return LessThan;
        else
            return GreaterThan;
    }

    /* Final comparison */
    if (NameA->Length < NameB->Length)
        return LessThan;
    else if (NameA->Length > NameB->Length)
        return GreaterThan;
    else
        return EqualTo;
}

PFCB
NTAPI
FatFindFcb(PFAT_IRP_CONTEXT IrpContext,
           PRTL_SPLAY_LINKS *RootNode,
           PSTRING AnsiName,
           PBOOLEAN IsDosName)
{
    PFCB_NAME_LINK Node;
    FSRTL_COMPARISON_RESULT Comparison;
    PRTL_SPLAY_LINKS Links;

    Links = *RootNode;

    while (Links)
    {
        Node = CONTAINING_RECORD(Links, FCB_NAME_LINK, Links);

        /* Compare the prefix */
        if (*(PUCHAR)Node->Name.Ansi.Buffer != *(PUCHAR)AnsiName->Buffer)
        {
            if (*(PUCHAR)Node->Name.Ansi.Buffer < *(PUCHAR)AnsiName->Buffer)
                Comparison = LessThan;
            else
                Comparison = GreaterThan;
        }
        else
        {
            /* Perform real comparison */
            Comparison = FatiCompareNames(&Node->Name.Ansi, AnsiName);
        }

        /* Do they match? */
        if (Comparison == GreaterThan)
        {
            /* No, it's greater, go to the left child */
            Links = RtlLeftChild(Links);
        }
        else if (Comparison == LessThan)
        {
            /* No, it's lesser, go to the right child */
            Links = RtlRightChild(Links);
        }
        else
        {
            /* Exact match, balance the tree */
            *RootNode = RtlSplay(Links);

            /* Save type of the name, if needed */
            if (IsDosName)
                *IsDosName = Node->IsDosName;

            /* Return the found fcb */
            return Node->Fcb;
        }
    }

    /* Nothing found */
    return NULL;
}

PFCB
NTAPI
FatCreateFcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PVCB Vcb,
             IN PFCB ParentDcb,
             IN FF_FILE *FileHandle)
{
    PFCB Fcb;

    /* Allocate it and zero it */
    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(FCB), TAG_FCB);
    RtlZeroMemory(Fcb, sizeof(FCB));

    /* Set node types */
    Fcb->Header.NodeTypeCode = FAT_NTC_FCB;
    Fcb->Header.NodeByteSize = sizeof(FCB);
    Fcb->Condition = FcbGood;

    /* Initialize resources */
    Fcb->Header.Resource = &Fcb->Resource;
    ExInitializeResourceLite(Fcb->Header.Resource);

    Fcb->Header.PagingIoResource = &Fcb->PagingIoResource;
    ExInitializeResourceLite(Fcb->Header.PagingIoResource);

    /* Initialize mutexes */
    Fcb->Header.FastMutex = &Fcb->HeaderMutex;
    ExInitializeFastMutex(&Fcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);

    /* Insert into parent's DCB list */
    InsertTailList(&ParentDcb->Dcb.ParentDcbList, &Fcb->ParentDcbLinks);

    /* Set backlinks */
    Fcb->ParentFcb = ParentDcb;
    Fcb->Vcb = Vcb;

    /* Set file handle and sizes */
    Fcb->Header.FileSize.LowPart = FileHandle->Filesize;
    Fcb->Header.ValidDataLength.LowPart = FileHandle->Filesize;
    Fcb->FatHandle = FileHandle;

    /* Initialize locks */
    FsRtlInitializeFileLock(&Fcb->Fcb.Lock, NULL, NULL);
    FsRtlInitializeOplock(&Fcb->Fcb.Oplock);

    /* Set names */
    FatSetFcbNames(IrpContext, Fcb);

    return Fcb;
}

VOID
NTAPI
FatDeleteFcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PFCB Fcb)
{
    DPRINT("FatDeleteFcb %p\n", Fcb);

    if (Fcb->OpenCount != 0)
    {
        DPRINT1("Trying to delete FCB with OpenCount %d\n", Fcb->OpenCount);
        ASSERT(FALSE);
    }

    if ((Fcb->Header.NodeTypeCode == FAT_NTC_DCB) ||
        (Fcb->Header.NodeTypeCode == FAT_NTC_ROOT_DCB))
    {
        /* Make sure it's a valid deletion */
        ASSERT(Fcb->Dcb.DirectoryFileOpenCount == 0);
        ASSERT(IsListEmpty(&Fcb->Dcb.ParentDcbList));
        ASSERT(Fcb->Dcb.DirectoryFile == NULL);
    }
    else
    {
        /* Free locks */
        FsRtlUninitializeFileLock(&Fcb->Fcb.Lock);
        FsRtlUninitializeOplock(&Fcb->Fcb.Oplock);
    }

    /* Release any possible filter contexts */
    FsRtlTeardownPerStreamContexts(&Fcb->Header);

    /* Remove from parents queue */
    if (Fcb->Header.NodeTypeCode != FAT_NTC_ROOT_DCB)
    {
        RemoveEntryList(&(Fcb->ParentDcbLinks));
    }

    /* Free FullFAT handle */
    if (Fcb->FatHandle) FF_Close(Fcb->FatHandle);

    /* Remove from the splay table */
    if (FlagOn(Fcb->State, FCB_STATE_HAS_NAMES))
        FatRemoveNames(IrpContext, Fcb);

    /* Free file name buffers */
    if (Fcb->Header.NodeTypeCode != FAT_NTC_ROOT_DCB)
    {
        if (Fcb->FullFileName.Buffer)
            ExFreePool(Fcb->FullFileName.Buffer);
    }

    if (Fcb->ExactCaseLongName.Buffer)
        ExFreePool(Fcb->ExactCaseLongName.Buffer);

    /* Free this FCB, finally */
    ExFreePool(Fcb);
}

PCCB
NTAPI
FatCreateCcb()
{
    PCCB Ccb;

    /* Allocate the CCB and zero it */
    Ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(CCB), TAG_CCB);
    RtlZeroMemory(Ccb, sizeof(CCB));

    /* Set mandatory header */
    Ccb->NodeTypeCode = FAT_NTC_FCB;
    Ccb->NodeByteSize = sizeof(CCB);

    return Ccb;
}

VOID
NTAPI
FatDeleteCcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PCCB Ccb)
{
    // TODO: Deallocate CCB strings, if any

    /* Free the CCB */
    ExFreePool(Ccb);
}

IO_STATUS_BLOCK
NTAPI
FatiOpenExistingFcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PFILE_OBJECT FileObject,
                    IN PVCB Vcb,
                    IN PFCB Fcb,
                    IN PACCESS_MASK DesiredAccess,
                    IN USHORT ShareAccess,
                    IN ULONG AllocationSize,
                    IN PFILE_FULL_EA_INFORMATION EaBuffer,
                    IN ULONG EaLength,
                    IN UCHAR FileAttributes,
                    IN ULONG CreateDisposition,
                    IN BOOLEAN NoEaKnowledge,
                    IN BOOLEAN DeleteOnClose,
                    IN BOOLEAN OpenedAsDos,
                    OUT PBOOLEAN OplockPostIrp)
{
    IO_STATUS_BLOCK Iosb = {{0}};
    ACCESS_MASK AddedAccess = 0;
    BOOLEAN Hidden;
    BOOLEAN System;
    PCCB Ccb = NULL;
    NTSTATUS Status, StatusPrev;

    /* Acquire exclusive FCB lock */
    (VOID)FatAcquireExclusiveFcb(IrpContext, Fcb);

    *OplockPostIrp = FALSE;

    /* Check if there is a batch oplock */
    if (FsRtlCurrentBatchOplock(&Fcb->Fcb.Oplock))
    {
        /* Return with a special information field */
        Iosb.Information = FILE_OPBATCH_BREAK_UNDERWAY;

        /* Check the oplock */
        Iosb.Status = FsRtlCheckOplock(&Fcb->Fcb.Oplock,
                                       IrpContext->Irp,
                                       IrpContext,
                                       FatOplockComplete,
                                       FatPrePostIrp);

        if (Iosb.Status != STATUS_SUCCESS &&
            Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS)
        {
            /* The Irp needs to be queued */
            *OplockPostIrp = TRUE;

            /* Release the FCB and return */
            FatReleaseFcb(IrpContext, Fcb);
            return Iosb;
        }
    }

    /* Validate parameters and modify access */
    if (CreateDisposition == FILE_CREATE)
    {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;

        /* Release the FCB and return */
        FatReleaseFcb(IrpContext, Fcb);
        return Iosb;
    }
    else if (CreateDisposition == FILE_SUPERSEDE)
    {
        SetFlag(AddedAccess, DELETE & ~(*DesiredAccess));
        *DesiredAccess |= DELETE;
    }
    else if ((CreateDisposition == FILE_OVERWRITE) ||
             (CreateDisposition == FILE_OVERWRITE_IF))
    {
        SetFlag(AddedAccess,
                (FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES)
                & ~(*DesiredAccess) );

        *DesiredAccess |= FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
    }

    // TODO: Check desired access

    // TODO: Check if this file is readonly and DeleteOnClose is set

    /* Validate disposition information */
    if ((CreateDisposition == FILE_SUPERSEDE) ||
        (CreateDisposition == FILE_OVERWRITE) ||
        (CreateDisposition == FILE_OVERWRITE_IF))
    {
        // TODO: Get this attributes from the dirent
        Hidden = FALSE;
        System = FALSE;

        if ((Hidden && !FlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)) ||
            (System && !FlagOn(FileAttributes, FILE_ATTRIBUTE_SYSTEM)))
        {
            DPRINT1("Hidden/system attributes don't match\n");

            Iosb.Status = STATUS_ACCESS_DENIED;

            /* Release the FCB and return */
            FatReleaseFcb(IrpContext, Fcb);
            return Iosb;
        }

        // TODO: Check for write protected volume
    }

    /* Check share access */
    Iosb.Status = IoCheckShareAccess(*DesiredAccess,
                                     ShareAccess,
                                     FileObject,
                                     &Fcb->ShareAccess,
                                     FALSE);
    if (!NT_SUCCESS(Iosb.Status))
    {
        /* Release the FCB and return */
        FatReleaseFcb(IrpContext, Fcb);
        return Iosb;
    }

    /* Check the oplock status after checking for share access */
    Iosb.Status = FsRtlCheckOplock(&Fcb->Fcb.Oplock,
                                   IrpContext->Irp,
                                   IrpContext,
                                   FatOplockComplete,
                                   FatPrePostIrp );

    if (Iosb.Status != STATUS_SUCCESS &&
        Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS)
    {
        /* The Irp needs to be queued */
        *OplockPostIrp = TRUE;

        /* Release the FCB and return */
        FatReleaseFcb(IrpContext, Fcb);
        return Iosb;
    }

    /* Set Fast I/O flag */
    Fcb->Header.IsFastIoPossible = FALSE; //FatiIsFastIoPossible(Fcb);

    /* Make sure image is not mapped */
    if (DeleteOnClose || FlagOn(*DesiredAccess, FILE_WRITE_DATA))
    {
        /* Try to flush the image section */
        if (!MmFlushImageSection(&Fcb->SectionObjectPointers, MmFlushForWrite))
        {
            /* Yes, image section exists, set correct status code */
            if (DeleteOnClose)
                Iosb.Status = STATUS_CANNOT_DELETE;
            else
                Iosb.Status = STATUS_SHARING_VIOLATION;

            /* Release the FCB and return */
            FatReleaseFcb(IrpContext, Fcb);
            return Iosb;
        }
    }

    /* Flush the cache if it's non-cached non-pagefile access */
    if (FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING) &&
        Fcb->SectionObjectPointers.DataSectionObject &&
        !FlagOn(Fcb->State, FCB_STATE_PAGEFILE))
    {
        /* Set the flag that create is in progress */
        SetFlag(Fcb->Vcb->State, VCB_STATE_CREATE_IN_PROGRESS);

        /* Flush the cache */
        CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, NULL);

        /* Acquire and release Paging I/O resource before purging the cache section
           to let lazy writer finish */
        ExAcquireResourceExclusiveLite( Fcb->Header.PagingIoResource, TRUE);
        ExReleaseResourceLite( Fcb->Header.PagingIoResource );

        /* Delete the cache section */
        CcPurgeCacheSection(&Fcb->SectionObjectPointers, NULL, 0, FALSE);

        /* Clear the flag */
        ClearFlag(Fcb->Vcb->State, VCB_STATE_CREATE_IN_PROGRESS);
    }

    /* Check create disposition flags and branch accordingly */
    if (CreateDisposition == FILE_OPEN ||
        CreateDisposition == FILE_OPEN_IF)
    {
        DPRINT("Opening a file\n");

        /* Check if we need to bother with EA */
        if (NoEaKnowledge && FALSE /* FatIsFat32(Vcb)*/)
        {
            UNIMPLEMENTED;
        }

        /* Set up file object */
        Ccb = FatCreateCcb();
        FatSetFileObject(FileObject,
                         UserFileOpen,
                         Fcb,
                         Ccb);

        FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

        /* The file is opened */
        Iosb.Information = FILE_OPENED;
        goto SuccComplete;
    }
    else if ((CreateDisposition == FILE_SUPERSEDE) ||
             (CreateDisposition == FILE_OVERWRITE) ||
             (CreateDisposition == FILE_OVERWRITE_IF))
    {
        /* Remember previous status */
        StatusPrev = Iosb.Status;

        // TODO: Check system security access

        /* Perform overwrite operation */
        Iosb = FatiOverwriteFile(IrpContext,
                                 FileObject,
                                 Fcb,
                                 AllocationSize,
                                 EaBuffer,
                                 EaLength,
                                 FileAttributes,
                                 CreateDisposition,
                                 NoEaKnowledge);

        /* Restore previous status in case of success */
        if (Iosb.Status == STATUS_SUCCESS)
            Iosb.Status = StatusPrev;

        /* Fall down to completion */
    }
    else
    {
        /* We can't get here */
        KeBugCheckEx(FAT_FILE_SYSTEM, CreateDisposition, 0, 0, 0);
    }


SuccComplete:
    /* If all is fine */
    if (Iosb.Status != STATUS_PENDING &&
        NT_SUCCESS(Iosb.Status))
    {
        /* Update access if needed */
        if (AddedAccess)
        {
            /* Remove added access flags from desired access */
            ClearFlag(*DesiredAccess, AddedAccess);

            /* Check share access */
            Status = IoCheckShareAccess(*DesiredAccess,
                                        ShareAccess,
                                        FileObject,
                                        &Fcb->ShareAccess,
                                        TRUE);

            /* Make sure it's success */
            ASSERT(Status == STATUS_SUCCESS);
        }
        else
        {
            /* Update the share access */
            IoUpdateShareAccess(FileObject, &Fcb->ShareAccess);
        }

        /* Clear the delay close */
        ClearFlag(Fcb->State, FCB_STATE_DELAY_CLOSE);

        /* Increase counters */
        Fcb->UncleanCount++;
        Fcb->OpenCount++;
        Vcb->OpenFileCount++;
        if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;
        if (FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) Fcb->NonCachedUncleanCount++;

        // TODO: Handle DeleteOnClose and OpenedAsDos by storing those flags in CCB
    }

    return Iosb;
}

VOID
NTAPI
FatGetFcbUnicodeName(IN PFAT_IRP_CONTEXT IrpContext,
                     IN PFCB Fcb,
                     OUT PUNICODE_STRING LongName)
{
    FF_DIRENT DirEnt;
    FF_ERROR Err;
    OEM_STRING ShortName;
    CHAR ShortNameBuf[13];
    UCHAR EntryBuffer[32];
    UCHAR NumLFNs;
    OEM_STRING LongNameOem;
    NTSTATUS Status;

    /* Make sure this FCB has a FullFAT handle associated with it */
    if (Fcb->FatHandle == NULL &&
        FatNodeType(Fcb) == FAT_NTC_DCB)
    {
        /* Open the dir with FullFAT */
        Fcb->FatHandle = FF_OpenW(Fcb->Vcb->Ioman, &Fcb->FullFileName, FF_MODE_DIR, NULL);
        if (!Fcb->FatHandle)
        {
            ASSERT(FALSE);
        }
    }

    /* Get the dir entry */
    Err = FF_GetEntry(Fcb->Vcb->Ioman,
                      Fcb->FatHandle->DirEntry,
                      Fcb->FatHandle->DirCluster,
                      &DirEnt);

    if (Err != FF_ERR_NONE)
    {
        DPRINT1("Error %d getting dirent of a file\n", Err);
        return;
    }

    /* Read the dirent to fetch the raw short name */
    FF_FetchEntry(Fcb->Vcb->Ioman,
                  Fcb->FatHandle->DirCluster,
                  Fcb->FatHandle->DirEntry,
                  EntryBuffer);
    NumLFNs = (UCHAR)(EntryBuffer[0] & ~0x40);

    /* Check if we only have a short name.
       Convert it to unicode and return if that's the case */
    if (NumLFNs == 0)
    {
        /* Initialize short name string */
        ShortName.Buffer = ShortNameBuf;
        ShortName.Length = 0;
        ShortName.MaximumLength = 12;

        /* Convert raw short name to a proper string */
        Fati8dot3ToString((PCHAR)EntryBuffer, FALSE, &ShortName);

        /* Convert it to unicode */
        Status = RtlOemStringToCountedUnicodeString(LongName,
                                                    &ShortName,
                                                    FALSE);

        /* Ensure conversion was successful */
        ASSERT(Status == STATUS_SUCCESS);

        /* Exit */
        return;
    }

    /* Convert LFN from OEM to unicode and return */
    LongNameOem.Buffer = DirEnt.FileName;
    LongNameOem.MaximumLength = FF_MAX_FILENAME;
    LongNameOem.Length = strlen(DirEnt.FileName);

    /* Convert it to unicode */
    Status = RtlOemStringToUnicodeString(LongName, &LongNameOem, FALSE);

    /* Ensure conversion was successful */
    ASSERT(Status == STATUS_SUCCESS);
}


VOID
NTAPI
FatSetFullNameInFcb(PFCB Fcb,
                    PUNICODE_STRING Name)
{
    PUNICODE_STRING ParentName;

    /* Make sure this FCB's name wasn't already set */
    ASSERT(Fcb->FullFileName.Buffer == NULL);

    /* First of all, check exact case name */
    if (Fcb->ExactCaseLongName.Buffer)
    {
        ASSERT(Fcb->ExactCaseLongName.Length != 0);

        /* Use exact case name */
        Name = &Fcb->ExactCaseLongName;
    }

    /* Treat root dir different */
    if (FatNodeType(Fcb->ParentFcb) == FAT_NTC_ROOT_DCB)
    {
        /* Set lengths */
        Fcb->FullFileName.MaximumLength = sizeof(WCHAR) + Name->Length;
        Fcb->FullFileName.Length = Fcb->FullFileName.MaximumLength;

        /* Allocate a buffer */
        Fcb->FullFileName.Buffer = FsRtlAllocatePoolWithTag(PagedPool,
                                                            Fcb->FullFileName.Length,
                                                            TAG_FILENAME);

        /* Prefix with a backslash */
        Fcb->FullFileName.Buffer[0] = L'\\';

        /* Copy the name here */
        RtlCopyMemory(&Fcb->FullFileName.Buffer[1],
                      &Name->Buffer[0],
                       Name->Length );
    }
    else
    {
        ParentName = &Fcb->ParentFcb->FullFileName;

        /* Check if parent's name is set */
        if (!ParentName->Buffer)
            return;

        /* Set lengths */
        Fcb->FullFileName.MaximumLength =
            ParentName->Length + sizeof(WCHAR) + Name->Length;
        Fcb->FullFileName.Length = Fcb->FullFileName.MaximumLength;

        /* Allocate a buffer */
        Fcb->FullFileName.Buffer = FsRtlAllocatePoolWithTag(PagedPool,
                                                            Fcb->FullFileName.Length,
                                                            TAG_FILENAME );

        /* Copy parent's name here */
        RtlCopyMemory(&Fcb->FullFileName.Buffer[0],
                      &ParentName->Buffer[0],
                      ParentName->Length );

        /* Add a backslash */
        Fcb->FullFileName.Buffer[ParentName->Length / sizeof(WCHAR)] = L'\\';

        /* Copy given name here */
        RtlCopyMemory(&Fcb->FullFileName.Buffer[(ParentName->Length / sizeof(WCHAR)) + 1],
                      &Name->Buffer[0],
                      Name->Length );
    }
}

VOID
NTAPI
FatSetFullFileNameInFcb(IN PFAT_IRP_CONTEXT IrpContext,
                        IN PFCB Fcb)
{
    UNICODE_STRING LongName;
    PFCB CurFcb = Fcb;
    PFCB StopFcb;
    PWCHAR TmpBuffer;
    ULONG PathLength = 0;

    /* Do nothing if it's already set */
    if (Fcb->FullFileName.Buffer) return;

    /* Allocate a temporary buffer */
    LongName.Length = 0;
    LongName.MaximumLength = FF_MAX_FILENAME * sizeof(WCHAR);
    LongName.Buffer =
        FsRtlAllocatePoolWithTag(PagedPool,
                                 FF_MAX_FILENAME * sizeof(WCHAR),
                                 TAG_FILENAME);

    /* Go through all parents to calculate needed length */
    while (CurFcb != Fcb->Vcb->RootDcb)
    {
        /* Does current FCB have FullFileName set? */
        if (CurFcb != Fcb &&
            CurFcb->FullFileName.Buffer)
        {
            /* Yes, just use it! */
            PathLength += CurFcb->FullFileName.Length;

            Fcb->FullFileName.Buffer =
                FsRtlAllocatePoolWithTag(PagedPool,
                                         PathLength,
                                         TAG_FILENAME);

            RtlCopyMemory(Fcb->FullFileName.Buffer,
                          CurFcb->FullFileName.Buffer,
                          CurFcb->FullFileName.Length);

            break;
        }

        /* Sum up length of a current item */
        PathLength += CurFcb->FileNameLength + sizeof(WCHAR);

        /* Go to the parent */
        CurFcb = CurFcb->ParentFcb;
    }

    /* Allocate FullFileName if it wasn't already allocated above */
    if (!Fcb->FullFileName.Buffer)
    {
        Fcb->FullFileName.Buffer =
            FsRtlAllocatePoolWithTag(PagedPool,
                                     PathLength,
                                     TAG_FILENAME);
    }

    StopFcb = CurFcb;

    CurFcb = Fcb;
    TmpBuffer =  Fcb->FullFileName.Buffer + PathLength / sizeof(WCHAR);

    /* Set lengths */
    Fcb->FullFileName.Length = PathLength;
    Fcb->FullFileName.MaximumLength = PathLength;

    while (CurFcb != StopFcb)
    {
        /* Get its unicode name */
        FatGetFcbUnicodeName(IrpContext,
                             CurFcb,
                             &LongName);

        /* Copy it */
        TmpBuffer -= LongName.Length / sizeof(WCHAR);
        RtlCopyMemory(TmpBuffer, LongName.Buffer, LongName.Length);

        /* Append with a backslash */
        TmpBuffer -= 1;
        *TmpBuffer = L'\\';

        /* Go to the parent */
        CurFcb = CurFcb->ParentFcb;
    }

    /* Free the temp buffer */
    ExFreePool(LongName.Buffer);
}


VOID
NTAPI
FatSetFcbNames(IN PFAT_IRP_CONTEXT IrpContext,
               IN PFCB Fcb)
{
    FF_DIRENT DirEnt;
    FF_ERROR Err;
    POEM_STRING ShortName;
    CHAR ShortNameRaw[13];
    UCHAR EntryBuffer[32];
    UCHAR NumLFNs;
    PUNICODE_STRING UnicodeName;
    OEM_STRING LongNameOem;
    NTSTATUS Status;

    /* Get the dir entry */
    Err = FF_GetEntry(Fcb->Vcb->Ioman,
                      Fcb->FatHandle->DirEntry,
                      Fcb->FatHandle->DirCluster,
                      &DirEnt);

    if (Err != FF_ERR_NONE)
    {
        DPRINT1("Error %d getting dirent of a file\n", Err);
        return;
    }

    /* Read the dirent to fetch the raw short name */
    FF_FetchEntry(Fcb->Vcb->Ioman,
                  Fcb->FatHandle->DirCluster,
                  Fcb->FatHandle->DirEntry,
                  EntryBuffer);
    NumLFNs = (UCHAR)(EntryBuffer[0] & ~0x40);
    RtlCopyMemory(ShortNameRaw, EntryBuffer, 11);

    /* Initialize short name string */
    ShortName = &Fcb->ShortName.Name.Ansi;
    ShortName->Buffer = Fcb->ShortNameBuffer;
    ShortName->Length = 0;
    ShortName->MaximumLength = sizeof(Fcb->ShortNameBuffer);

    /* Convert raw short name to a proper string */
    Fati8dot3ToString(ShortNameRaw, FALSE, ShortName);

    /* Add the short name link */
    FatInsertName(IrpContext, &Fcb->ParentFcb->Dcb.SplayLinksAnsi, &Fcb->ShortName);
    Fcb->ShortName.Fcb = Fcb;

    /* Get the long file name (if any) */
    if (NumLFNs > 0)
    {
        /* Prepare the oem string */
        LongNameOem.Buffer = DirEnt.FileName;
        LongNameOem.MaximumLength = FF_MAX_FILENAME;
        LongNameOem.Length = strlen(DirEnt.FileName);

        /* Prepare the unicode string */
        UnicodeName = &Fcb->LongName.Name.String;
        UnicodeName->Length = (LongNameOem.Length + 1) * sizeof(WCHAR);
        UnicodeName->MaximumLength = UnicodeName->Length;
        UnicodeName->Buffer = FsRtlAllocatePool(PagedPool, UnicodeName->Length);

        /* Convert it to unicode */
        Status = RtlOemStringToUnicodeString(UnicodeName, &LongNameOem, FALSE);
        if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
        }

        /* Set its length */
        Fcb->FileNameLength = UnicodeName->Length;

        /* Save case-preserved copy */
        Fcb->ExactCaseLongName.Length = UnicodeName->Length;
        Fcb->ExactCaseLongName.MaximumLength = UnicodeName->Length;
        Fcb->ExactCaseLongName.Buffer =
            FsRtlAllocatePoolWithTag(PagedPool, UnicodeName->Length, TAG_FILENAME);

        RtlCopyMemory(Fcb->ExactCaseLongName.Buffer,
                      UnicodeName->Buffer,
                      UnicodeName->Length);

        /* Perform a trick which is done by MS's FASTFAT driver to monocase
           the filename */
        RtlDowncaseUnicodeString(UnicodeName, UnicodeName, FALSE);
        RtlUpcaseUnicodeString(UnicodeName, UnicodeName, FALSE);

        DPRINT("Converted long name: %wZ\n", UnicodeName);

        /* Add the long unicode name link */
        FatInsertName(IrpContext, &Fcb->ParentFcb->Dcb.SplayLinksUnicode, &Fcb->LongName);
        Fcb->LongName.Fcb = Fcb;

        /* Indicate that this FCB has a unicode long name */
        SetFlag(Fcb->State, FCB_STATE_HAS_UNICODE_NAME);
    }
    else
    {
        /* No LFN, set exact case name to 0 length */
        Fcb->ExactCaseLongName.Length = 0;
        Fcb->ExactCaseLongName.MaximumLength = 0;

        /* Set the length based on the short name */
        Fcb->FileNameLength = RtlOemStringToCountedUnicodeSize(ShortName);
    }

    /* Mark the fact that names were added to splay trees*/
    SetFlag(Fcb->State, FCB_STATE_HAS_NAMES);
}

VOID
NTAPI
Fati8dot3ToString(IN PCHAR FileName,
                  IN BOOLEAN DownCase,
                  OUT POEM_STRING OutString)
{
    ULONG BaseLen, ExtLen;
    CHAR  *cString = OutString->Buffer;
    ULONG i;

    /* Calc base and ext lens */
    for (BaseLen = 8; BaseLen > 0; BaseLen--)
    {
        if (FileName[BaseLen - 1] != ' ') break;
    }

    for (ExtLen = 3; ExtLen > 0; ExtLen--)
    {
        if (FileName[8 + ExtLen - 1] != ' ') break;
    }

    /* Process base name */
    if (BaseLen)
    {
        RtlCopyMemory(cString, FileName, BaseLen);

        /* Substitute the e5 thing */
        if (cString[0] == 0x05) cString[0] = 0xe5;

        /* Downcase if asked to */
        if (DownCase)
        {
            /* Do it manually */
            for (i = 0; i < BaseLen; i++)
            {
                if (cString[i] >= 'A' &&
                    cString[i] <= 'Z')
                {
                    /* Lowercase it */
                    cString[i] += 'a' - 'A';
                }

            }
        }
    }

    /* Process extension */
    if (ExtLen)
    {
        /* Add the dot */
        cString[BaseLen] = '.';
        BaseLen++;

        /* Copy the extension */
        for (i = 0; i < ExtLen; i++)
        {
            cString[BaseLen + i] = FileName[8 + i];
        }

        /* Lowercase the extension if asked to */
        if (DownCase)
        {
            /* Do it manually */
            for (i = BaseLen; i < BaseLen + ExtLen; i++)
            {
                if (cString[i] >= 'A' &&
                    cString[i] <= 'Z')
                {
                    /* Lowercase it */
                    cString[i] += 'a' - 'A';
                }
            }
        }
    }

    /* Set the length */
    OutString->Length = BaseLen + ExtLen;

    DPRINT("'%s', len %d\n", OutString->Buffer, OutString->Length);
}

VOID
NTAPI
FatInsertName(IN PFAT_IRP_CONTEXT IrpContext,
              IN PRTL_SPLAY_LINKS *RootNode,
              IN PFCB_NAME_LINK Name)
{
    PFCB_NAME_LINK NameLink;
    FSRTL_COMPARISON_RESULT Comparison;

    /* Initialize the splay links */
    RtlInitializeSplayLinks(&Name->Links);

    /* Is this the first entry? */
    if (*RootNode == NULL)
    {
        /* Yes, become root and return */
        *RootNode = &Name->Links;
        return;
    }

    /* Get the name link */
    NameLink = CONTAINING_RECORD(*RootNode, FCB_NAME_LINK, Links);
    while (TRUE)
    {
        /* Compare the prefix */
        if (*(PUCHAR)NameLink->Name.Ansi.Buffer != *(PUCHAR)&Name->Name.Ansi.Buffer)
        {
            if (*(PUCHAR)NameLink->Name.Ansi.Buffer < *(PUCHAR)&Name->Name.Ansi.Buffer)
                Comparison = LessThan;
            else
                Comparison = GreaterThan;
        }
        else
        {
            /* Perform real comparison */
            Comparison = FatiCompareNames(&NameLink->Name.Ansi, &Name->Name.Ansi);
        }

        /* Check the bad case first */
        if (Comparison == EqualTo)
        {
            /* Must not happen */
            ASSERT(FALSE);
        }

        /* Check comparison result */
        if (Comparison == GreaterThan)
        {
            /* Go to the left child */
            if (!RtlLeftChild(&NameLink->Links))
            {
                /* It's absent, insert here and break */
                RtlInsertAsLeftChild(&NameLink->Links, &Name->Links);
                break;
            }
            else
            {
                /* It's present, go inside it */
                NameLink = CONTAINING_RECORD(RtlLeftChild(&NameLink->Links),
                                             FCB_NAME_LINK,
                                             Links);
            }
        }
        else
        {
            /* Go to the right child */
            if (!RtlRightChild(&NameLink->Links))
            {
                /* It's absent, insert here and break */
                RtlInsertAsRightChild(&NameLink->Links, &Name->Links);
                break;
            }
            else
            {
                /* It's present, go inside it */
                NameLink = CONTAINING_RECORD(RtlRightChild(&NameLink->Links),
                                             FCB_NAME_LINK,
                                             Links);
            }
        }
    }
}

VOID
NTAPI
FatRemoveNames(IN PFAT_IRP_CONTEXT IrpContext,
               IN PFCB Fcb)
{
    PRTL_SPLAY_LINKS RootNew;
    PFCB Parent;

    /* Reference the parent for simplicity */
    Parent = Fcb->ParentFcb;

    /* If this FCB hasn't been added to splay trees - just return */
    if (!FlagOn( Fcb->State, FCB_STATE_HAS_NAMES ))
        return;

    /* Delete the short name link */
    RootNew = RtlDelete(&Fcb->ShortName.Links);

    /* Set the new root */
    Parent->Dcb.SplayLinksAnsi = RootNew;

    /* Deal with a unicode name if it exists */
    if (FlagOn( Fcb->State, FCB_STATE_HAS_UNICODE_NAME ))
    {
        /* Delete the long unicode name link */
        RootNew = RtlDelete(&Fcb->LongName.Links);

        /* Set the new root */
        Parent->Dcb.SplayLinksUnicode = RootNew;

        /* Free the long name string's buffer*/
        RtlFreeUnicodeString(&Fcb->LongName.Name.String);

        /* Clear the "has unicode name" flag */
        ClearFlag(Fcb->State, FCB_STATE_HAS_UNICODE_NAME);
    }

    /* This FCB has no names added to splay trees now */
    ClearFlag(Fcb->State, FCB_STATE_HAS_NAMES);
}


/* EOF */
