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

    /* Set names */
    FatSetFcbNames(IrpContext, Fcb);

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

        RtlDowncaseUnicodeString(UnicodeName, UnicodeName, FALSE);
        RtlUpcaseUnicodeString(UnicodeName, UnicodeName, FALSE);

        DPRINT1("Converted long name: %wZ\n", UnicodeName);

        /* Add the long unicode name link */
        FatInsertName(IrpContext, &Fcb->ParentFcb->Dcb.SplayLinksUnicode, &Fcb->LongName);
        Fcb->LongName.Fcb = Fcb;

        /* Indicate that this FCB has a unicode long name */
        SetFlag(Fcb->State, FCB_STATE_HAS_UNICODE_NAME);
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

    DPRINT1("'%s', len %d\n", OutString->Buffer, OutString->Length);
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
        /* Compare prefixes */
        Comparison = FatiCompareNames(&NameLink->Name.Ansi, &Name->Name.Ansi);

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
                RtlInsertAsLeftChild(&NameLink->Links, &NameLink->Links);
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
