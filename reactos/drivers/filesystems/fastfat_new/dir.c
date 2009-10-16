/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
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

    /* Initialize MCB */
    FsRtlInitializeLargeMcb(&Dcb->Mcb, NonPagedPool);

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

    /* Initialize free dirent bitmap */
    RtlInitializeBitMap(&Dcb->Dcb.FreeBitmap, NULL, 0);

    /* Fill the dirent bitmap */
    DPRINT1("Filling the free dirent bitmap is missing\n");
    //FatCheckFreeDirentBitmap( IrpContext, Dcb );
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
        FatSetFcbNames(IrpContext, Fcb);

    return Fcb;
}

/* EOF */
