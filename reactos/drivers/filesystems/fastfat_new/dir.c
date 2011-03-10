/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/dir.c
 * PURPOSE:         Directory Control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
FatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatDirectoryControl()\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
FatCreateRootDcb(IN PFAT_IRP_CONTEXT IrpContext,
                 IN PVCB Vcb)
{
    PFCB Dcb;

    /* Make sure it's not already created */
    ASSERT(!Vcb->RootDcb);

    /* Allocate the DCB */
    Dcb = FsRtlAllocatePoolWithTag(NonPagedPool,
                                   sizeof(FCB),
                                   TAG_FCB);

    /* Zero it */
    RtlZeroMemory(Dcb, sizeof(FCB));

    /* Assign it to the VCB */
    Vcb->RootDcb = Dcb;

    /* Set its header */
    Dcb->Header.NodeTypeCode = FAT_NTC_ROOT_DCB;
    Dcb->Header.NodeByteSize = sizeof(FCB);

    /* FCB is in a good condition */
    Dcb->Condition = FcbGood;

    /* Initialize FCB's resource */
    Dcb->Header.Resource = &Dcb->Resource;
    ExInitializeResourceLite(&Dcb->Resource);

    /* Initialize Paging Io resource*/
    Dcb->Header.PagingIoResource = &Dcb->PagingIoResource;
    ExInitializeResourceLite(&Dcb->PagingIoResource);

    /* Initialize a list of parent DCBs*/
    InitializeListHead(&Dcb->ParentDcbLinks);

    /* Set VCB */
    Dcb->Vcb = Vcb;

    /* Initialize parent's DCB list */
    InitializeListHead(&Dcb->Dcb.ParentDcbList);

    /* Initialize the full file name */
    Dcb->FullFileName.Buffer = L"\\";
    Dcb->FullFileName.Length = 1 * sizeof(WCHAR);
    Dcb->FullFileName.MaximumLength = 2 * sizeof(WCHAR);

    Dcb->ShortName.Name.Ansi.Buffer = "\\";
    Dcb->ShortName.Name.Ansi.Length = 1;
    Dcb->ShortName.Name.Ansi.MaximumLength = 2 * sizeof(CHAR);

    /* Fill dirent attribute byte copy */
    Dcb->DirentFatFlags = FILE_ATTRIBUTE_DIRECTORY;

    /* Initialize advanced FCB header fields */
    ExInitializeFastMutex(&Dcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Dcb->Header, &Dcb->HeaderMutex);

    /* Set up first cluster field depending on FAT type */
    if (TRUE/*FatIsFat32(Vcb)*/)
    {
        /* First cluster is really the first cluster of this volume */
        Dcb->FirstClusterOfFile = Vcb->Bpb.RootDirFirstCluster;

        /* Calculate size of FAT32 root dir */
        Dcb->Header.AllocationSize.LowPart = 0xFFFFFFFF;
        //FatLookupFileAllocationSize(IrpContext, Dcb);
        DPRINT1("Calculation of a size of a root dir is missing!\n");

        Dcb->Header.FileSize.QuadPart = Dcb->Header.AllocationSize.QuadPart;
    }
    else
    {
#if 0
        /* Add MCB entry */
        FatAddMcbEntry(Vcb,
                       &Dcb->Mcb,
                       0,
                       FatRootDirectoryLbo(&Vcb->Bpb),
                       FatRootDirectorySize(&Vcb->Bpb));

        /* Set a real size of the root directory */
        Dcb->Header.FileSize.QuadPart = FatRootDirectorySize(&Vcb->Bpb);
        Dcb->Header.AllocationSize.QuadPart = Dcb->Header.FileSize.QuadPart;
#endif
        UNIMPLEMENTED;
    }
}

PFCB
NTAPI
FatCreateDcb(IN PFAT_IRP_CONTEXT IrpContext,
             IN PVCB Vcb,
             IN PFCB ParentDcb,
             IN FF_FILE *FileHandle)
{
    PFCB Fcb;

    /* Allocate it and zero it */
    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(FCB), TAG_FCB);
    RtlZeroMemory(Fcb, sizeof(FCB));

    /* Set node types */
    Fcb->Header.NodeTypeCode = FAT_NTC_DCB;
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
    InsertHeadList(&ParentDcb->Dcb.ParentDcbList, &Fcb->ParentDcbLinks);

    /* Set backlinks */
    Fcb->ParentFcb = ParentDcb;
    Fcb->Vcb = Vcb;

    /* Initialize parent dcb list */
    InitializeListHead(&Fcb->Dcb.ParentDcbList);

    /* Set FullFAT handle */
    Fcb->FatHandle = FileHandle;

    /* Set names */
    if (FileHandle)
    {
        FatSetFcbNames(IrpContext, Fcb);

        /* Ensure the full name is set */
        FatSetFullFileNameInFcb(IrpContext, Fcb);
    }

    return Fcb;
}

IO_STATUS_BLOCK
NTAPI
FatiOpenExistingDcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PFILE_OBJECT FileObject,
                    IN PVCB Vcb,
                    IN PFCB Dcb,
                    IN PACCESS_MASK DesiredAccess,
                    IN USHORT ShareAccess,
                    IN ULONG CreateDisposition,
                    IN BOOLEAN NoEaKnowledge,
                    IN BOOLEAN DeleteOnClose)
{
    IO_STATUS_BLOCK Iosb = {{0}};
    PCCB Ccb;

    /* Exclusively lock this FCB */
    FatAcquireExclusiveFcb(IrpContext, Dcb);

    /* Check if it's a delete-on-close of a root DCB */
    if (FatNodeType(Dcb) == FAT_NTC_ROOT_DCB && DeleteOnClose)
    {
        Iosb.Status = STATUS_CANNOT_DELETE;

        /* Release the lock and return */
        FatReleaseFcb(IrpContext, Dcb);
        return Iosb;
    }

    /*if (NoEaKnowledge && NodeType(Dcb) != FAT_NTC_ROOT_DCB &&
        !FatIsFat32(Vcb))
    {
        UNIMPLEMENTED;
    }*/

    /* Check the create disposition and desired access */
    if ((CreateDisposition != FILE_OPEN) &&
        (CreateDisposition != FILE_OPEN_IF))
    {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;

        /* Release the lock and return */
        FatReleaseFcb(IrpContext, Dcb);
        return Iosb;
    }

#if 0
    if (!FatCheckFileAccess(IrpContext,
                            Dcb->DirentFatFlags,
                            DesiredAccess))
    {
        Iosb.Status = STATUS_ACCESS_DENIED;
        try_return( Iosb );
    }
#endif

    /* If it's already opened - check share access */
    if (Dcb->OpenCount > 0)
    {
        Iosb.Status = IoCheckShareAccess(*DesiredAccess,
                                         ShareAccess,
                                         FileObject,
                                         &Dcb->ShareAccess,
                                         TRUE);

        if (!NT_SUCCESS(Iosb.Status))
        {
            /* Release the lock and return */
            FatReleaseFcb(IrpContext, Dcb);
            return Iosb;
        }
    }
    else
    {
        IoSetShareAccess(*DesiredAccess,
                         ShareAccess,
                         FileObject,
                         &Dcb->ShareAccess);
    }

    /* Set the file object */
    Ccb = FatCreateCcb();
    FatSetFileObject(FileObject,
                     UserDirectoryOpen,
                     Dcb,
                     Ccb);

    /* Increase counters */
    Dcb->UncleanCount++;
    Dcb->OpenCount++;
    Vcb->OpenFileCount++;
    if (IsFileObjectReadOnly(FileObject)) Vcb->ReadOnlyCount++;

    /* Set delete on close */
    if (DeleteOnClose)
        SetFlag(Ccb->Flags, CCB_DELETE_ON_CLOSE);

    /* Clear delay close flag */
    ClearFlag(Dcb->State, FCB_STATE_DELAY_CLOSE);

    /* That's it */
    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    /* Release the lock */
    FatReleaseFcb(IrpContext, Dcb);

    return Iosb;
}

/* EOF */
