/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fcb.c
 * PURPOSE:         FCB manipulation routines.
 * PROGRAMMERS:     Alexey Vlasov
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

/**
 * Locates FCB by the supplied name in the cache trie of fcbs.
 *
 * @param ParentFcb
 * Supplies a pointer to the parent FCB
 *
 * @param Name
 * Supplied a name of the FCB to be located in cache.
 *
 * @return
 * Pointer to the found FCB or NULL.
 */
PFCB
FatLookupFcbByName(
	IN PFCB ParentFcb,
	IN PUNICODE_STRING Name)
{
    PFCB_NAME_LINK Node;
	PRTL_SPLAY_LINKS Links;

    /* Get sub-trie root node from the parent FCB */
	Links = ParentFcb->Dcb.SplayLinks;
	while (Links != NULL)
	{
		LONG Comparison;

		Node = CONTAINING_RECORD(Links, FCB_NAME_LINK, Links);

       /*
        * Compare the name stored in the node
        * and determine the direction to walk.
        */
		Comparison = RtlCompareUnicodeString(&Node->String, Name, TRUE);
		if (Comparison > 0) {
            /* Left child */
			Links = RtlLeftChild(&Node->Links);
		}
		else if (Comparison < 0)
		{
            /* Right child */
			Links = RtlRightChild(&Node->Links);
		}
		else
		{
            /* Strings are equal, we have found the node! */
			break;
		}
	}

    /* The case when nothing was found. */
	if (Links == NULL)
		return NULL;

    /* Cast node to the FCB structure. */
	return CONTAINING_RECORD(Links, FCB, FileName[Node->Type]);
}

/**
 * Inserts FCB into FCBs cache trie.
 *
 * @param ParentFcb
 * Supplies a pointer to the parent FCB
 *
 * @param Fcb
 * Supplied a pointer to the being inserted FCB.
 *
 * @return
 * TRUE if the FCB was successfully inserted,
 * FASLE in the case of name collision.
 */
BOOLEAN
FatLinkFcbNames(
	IN PFCB ParentFcb,
	IN PFCB Fcb)
{
    PFCB_NAME_LINK Name;
	PRTL_SPLAY_LINKS Links;
	
    /* None of the parameters can be NULL */
	ASSERT(ParentFcb != NULL && Fcb != NULL);

    /* Get root links of the parent FCB. */
	Links = ParentFcb->Dcb.SplayLinks;

   /*
    * Get first file name
    * (short name for FAT because it's always there.
    */
	Name = Fcb->FileName;

   /*
    * Check if ParentDcb links are initialized,
    * at least one child FCB is cached.
    */
	if (Links == NULL)
	{
		ParentFcb->Dcb.SplayLinks = Links = &Name->Links;
		RtlInitializeSplayLinks(Links);

        /* Check if we have more names to cache. */
		if ((++Name)->String.Length == 0)
			return TRUE;
	}
    /* Lookup for the insertion point in the trie. */
	do
	{
		LONG Comparison;
		PFCB_NAME_LINK Node;
		PRTL_SPLAY_LINKS PrevLinks;

		PrevLinks = Links;
		Node = CONTAINING_RECORD(Links, FCB_NAME_LINK, Links);
		Comparison = RtlCompareUnicodeString(&Node->String, &Name->String, TRUE);
		if (Comparison > 0) {
			Links = RtlLeftChild(&Node->Links);
			if (Links == NULL)
			{
				RtlInsertAsLeftChild(PrevLinks, &Name->Links);
				break;
			}
		}
		else if (Comparison < 0)
		{
			Links = RtlRightChild(&Node->Links);
			if (Links == NULL)
			{
				RtlInsertAsRightChild(PrevLinks, &Name->Links);
				break;
			}
		}
		else
		{
			return FALSE;
		}

        /* Possibly switch to the second (lfn) name and cache that. */
	} while (Name == Fcb->FileName && (++Name)->String.Length > 0);
	return TRUE;
}

/**
 * Unlinks FCB from the FCBs cache trie.
 *
 * @param ParentFcb
 * Supplies a pointer to the parent FCB
 *
 * @param Fcb
 * Supplied a pointer to the being unlinked FCB.
 *
 * @return
 * VOID
 */
VOID
FatUnlinkFcbNames(
	IN PFCB ParentFcb,
	IN PFCB Fcb)
{
    /* See if there is an lfn and unlink that. */
	if (Fcb->FileName[FcbLongName].String.Length > 0)
		ParentFcb->Dcb.SplayLinks =
			RtlDelete(&Fcb->FileName[FcbLongName].Links);

    /* See if there is a short name and unlink that. */
	if (Fcb->FileName[FcbShortName].String.Length > 0)
		ParentFcb->Dcb.SplayLinks =
			RtlDelete(&Fcb->FileName[FcbShortName].Links);
}

NTSTATUS
FatCreateFcb(
    OUT PFCB* CreatedFcb,
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
	IN PDIR_ENTRY Dirent,
    IN PUNICODE_STRING FileName,
	IN PUNICODE_STRING LongFileName OPTIONAL)
{
	NTSTATUS Status;
	PFCB Fcb;

    /* Allocate FCB structure. */
	Fcb = (PFCB) ExAllocateFromNPagedLookasideList(&FatGlobalData.NonPagedFcbList);
	if (Fcb == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;
	RtlZeroMemory(Fcb, sizeof(FCB));

    /* Setup FCB Advanced Header. */
	Fcb->Header.NodeTypeCode = FAT_NTC_FCB;
	Fcb->Header.NodeByteSize = sizeof(*Fcb);
	ExInitializeResourceLite(&Fcb->Resource);
	Fcb->Header.Resource = &Fcb->Resource;
	ExInitializeResourceLite(&Fcb->PagingIoResource);
	Fcb->Header.PagingIoResource = &Fcb->PagingIoResource;
	ExInitializeFastMutex(&Fcb->HeaderMutex);
	FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);
	Fcb->Header.FileSize.QuadPart = Dirent->FileSize;
	Fcb->Header.ValidDataLength.QuadPart = Dirent->FileSize;
	Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

    /* Setup main fields. */
	FsRtlInitializeFileLock(&Fcb->Lock, NULL, NULL);
	FsRtlInitializeLargeMcb(&Fcb->Mcb, PagedPool);
	Fcb->Vcb = IrpContext->Vcb;
	Fcb->ParentFcb = ParentFcb;
	Fcb->FirstCluster = Dirent->FirstCluster
		| (Dirent->FirstClusterOfFileHi << 0x10);

    /* Setup basic info. */
    Fcb->BasicInfo.FileAttributes = Dirent->Attributes;
    FatQueryFileTimes(&Fcb->BasicInfo.CreationTime, Dirent);

    /* Setup short name since always present in FAT. */
	Fcb->FileName[FcbShortName].Type = FcbShortName;
	Fcb->FileName[FcbShortName].String.Buffer = Fcb->ShortNameBuffer;
	Fcb->FileName[FcbShortName].String.MaximumLength = 0x0c;
	Fcb->FileName[FcbShortName].String.Length = FileName->Length;
    RtlCopyMemory(Fcb->ShortNameBuffer, FileName->Buffer, FileName->Length);

    /* Just swap optional lfn. */
	if (ARGUMENT_PRESENT(LongFileName) && LongFileName->Length > 0)
	{
		Fcb->FileName[FcbLongName].Type = FcbLongName;
        Fcb->FileName[FcbLongName].String = *LongFileName;
        RtlZeroMemory(LongFileName, sizeof(UNICODE_STRING));
	}

    /* Put FCB into cache trie. */
	if (!FatLinkFcbNames(ParentFcb, Fcb))
	{
		Status = STATUS_OBJECT_NAME_COLLISION;
		goto FsdFatCreateFcbCleanup;
	}
    *CreatedFcb = Fcb;

    /* We are done! */
	return STATUS_SUCCESS;

FsdFatCreateFcbCleanup:
    if (ARGUMENT_PRESENT(LongFileName) &&
        Fcb->FileName[FcbLongName].String.Buffer != NULL)
    {
        /* Swap lfn back to the input parameter */
        *LongFileName = Fcb->FileName[FcbLongName].String;
    }
    ExFreeToNPagedLookasideList(&FatGlobalData.NonPagedFcbList, Fcb);
	return Status;
}

NTSTATUS
FatOpenFcb(
    OUT PFCB* Fcb,
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PUNICODE_STRING FileName)
{
    FAT_FIND_DIRENT_CONTEXT Context;
    UNICODE_STRING LongFileName;
    PDIR_ENTRY Dirent;
    NTSTATUS Status;

    // TODO: _SEH_TRY {
    if (ParentFcb->Dcb.StreamFileObject == NULL)
    {
        PFILE_OBJECT FileObject;
        PVPB Vpb;
    
        Vpb = IrpContext->Vcb->Vpb;

        /* Create stream file object */
        FileObject = IoCreateStreamFileObject(NULL, Vpb->RealDevice);
        FileObject->Vpb = Vpb;
        FileObject->SectionObjectPointer = &ParentFcb->SectionObjectPointers;
        FileObject->FsContext = ParentFcb;
        FileObject->FsContext2 = NULL;

        /* Store it in parent fcb */
        ParentFcb->Dcb.StreamFileObject = FileObject;

    }

    /* Check if cache is initialized. */
    if (ParentFcb->Dcb.StreamFileObject->PrivateCacheMap == NULL )
    {
        CcInitializeCacheMap(ParentFcb->Dcb.StreamFileObject,
            (PCC_FILE_SIZES) &ParentFcb->Header.AllocationSize,
            TRUE,
            &FatGlobalData.CacheMgrNoopCallbacks,
            ParentFcb);
    }

    /* Page context */
    Context.Page.FileObject = ParentFcb->Dcb.StreamFileObject;
    Context.Page.EndOfData = ParentFcb->Header.FileSize;
    Context.Page.Offset.QuadPart = -1LL;
    Context.Page.Bcb = NULL;
    Context.Page.CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    Context.Page.EndOfData = ParentFcb->Header.FileSize;

    /* Search context */
    Context.ShortName.Length = 0;
    Context.ShortName.Buffer = Context.ShortNameBuffer;
    Context.ShortName.MaximumLength = sizeof(Context.ShortNameBuffer);
    Context.FileName = FileName;
    Context.Valid8dot3Name = RtlIsNameLegalDOS8Dot3(FileName, NULL, NULL);

    /* Locate the dirent */
    FatFindDirent(&Context, &Dirent, &LongFileName);
 
    Status = FatCreateFcb(Fcb, IrpContext, ParentFcb, Dirent,
        &Context.ShortName, &LongFileName);
    return Status;
}
/* EOF */
