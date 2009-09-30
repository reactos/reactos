/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fcb.c
 * PURPOSE:         FCB manipulation routines.
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

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
             IN PFCB ParentDcb)
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

    return Fcb;
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

/* EOF */
