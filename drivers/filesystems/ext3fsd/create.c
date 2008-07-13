/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             create.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   13 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 *                     Fixed some warnings under GCC
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS *****************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2FollowLinkFinal (
    IN PWSTR UniNameBuffer,
    IN PCHAR OemNameBuffer,
    IN BOOLEAN bOemBuffer   );

VOID 
Ext2LookupFileFinal (
    IN PEXT2_VCB Vcb,
    OUT PEXT2_MCB Ext2Mcb,
    IN PNTSTATUS pStatus,
    IN PEXT2_MCB Mcb,
    IN BOOLEAN bDirectory,
    IN BOOLEAN LockAcquired    );

VOID
Ext2ScanDirFinal (
    IN PEXT2_MCB Parent,
    IN PEXT2_DIR_ENTRY2 pDir,
    IN PWSTR InodeFileNameBuffer    );

VOID
Ext2CreateFileFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PNTSTATUS pStatus,
    IN PIO_STACK_LOCATION IrpSp,
    IN PEXT2_FCB Fcb,
    IN PEXT2_MCB Mcb,
    IN PEXT2_MCB SymLink,
    IN PEXT2_CCB Ccb,
    IN PEXT2_FCB ParentFcb,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN bParentFcbCreated,
    IN BOOLEAN bCreated,
    IN BOOLEAN bMainResourceAcquired,
    IN BOOLEAN NoIntermediateBuffering    );

VOID
Ext2CreateFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PNTSTATUS pStatus,
    IN BOOLEAN VcbResourceAcquired    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2IsNameValid)
#pragma alloc_text(PAGE, Ext2FollowLink)
#pragma alloc_text(PAGE, Ext2IsSpecialSystemFile)
#pragma alloc_text(PAGE, Ext2LookupFile)
#pragma alloc_text(PAGE, Ext2ScanDir)
#pragma alloc_text(PAGE, Ext2CreateFile)
#pragma alloc_text(PAGE, Ext2CreateVolume)
#pragma alloc_text(PAGE, Ext2Create)
#pragma alloc_text(PAGE, Ext2CreateInode)
#pragma alloc_text(PAGE, Ext2SupersedeOrOverWriteFile)
#endif


/* FUNCTIONS ***************************************************************/


BOOLEAN
Ext2IsNameValid(PUNICODE_STRING FileName)
{
    USHORT  i = 0;
    PUSHORT pName = (PUSHORT) FileName->Buffer;

    if (FileName == NULL) {
        return FALSE;
    }

    while (i < (FileName->Length / sizeof(WCHAR))) {

        if (pName[i] == 0) {
            break;
        }

        if (pName[i] == L'|'  || pName[i] == L':'  ||
            pName[i] == L'/'  || pName[i] == L'*'  ||
            pName[i] == L'?'  || pName[i] == L'\"' ||
            pName[i] == L'<'  || pName[i] == L'>'   ) {

            return FALSE;
        }

        i++;
    }

    return TRUE;
}

_SEH_DEFINE_LOCALS(Ext2FollowLinkFinal)
{
    PWSTR UniNameBuffer;
    PCHAR OemNameBuffer;
    BOOLEAN bOemBuffer;
};

_SEH_FINALLYFUNC(Ext2FollowLinkFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2FollowLinkFinal);
    Ext2FollowLinkFinal(_SEH_VAR(UniNameBuffer), _SEH_VAR(OemNameBuffer),
                        _SEH_VAR(bOemBuffer));
}

VOID
Ext2FollowLinkFinal (
    IN PWSTR                UniNameBuffer,
    IN PCHAR                OemNameBuffer,
    IN BOOLEAN              bOemBuffer
    )
{
    if (bOemBuffer) {
        ExFreePoolWithTag(OemNameBuffer, TAG('N', 'L', '2', 'E'));
    }

    if (UniNameBuffer) {
        ExFreePoolWithTag(UniNameBuffer, TAG('N', 'L', '2', 'E'));
    }
}

NTSTATUS
Ext2FollowLink (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Parent,
    IN PEXT2_MCB            Mcb,
    IN USHORT               Linkdep
    )
{
    NTSTATUS        Status = STATUS_LINK_FAILED;

    UNICODE_STRING  UniName;
    OEM_STRING      OemName;

    PEXT2_MCB       Target = NULL;

    USHORT          i;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2FollowLinkFinal);
        _SEH_VAR(UniNameBuffer) = UniName.Buffer;
        _SEH_VAR(OemNameBuffer) = OemName.Buffer;
        _SEH_VAR(bOemBuffer) = FALSE;

        RtlZeroMemory(&UniName, sizeof(UNICODE_STRING));
        RtlZeroMemory(&OemName, sizeof(OEM_STRING));

        /* exit if we jump into a possible symlink forever loop */
        if ((Linkdep + 1) > EXT2_MAX_NESTED_LINKS ||
            IoGetRemainingStackSize() < 1024) {
            _SEH_LEAVE;
        }

        /* read the symlink target path */
        if (Mcb->Inode->i_size < EXT2_LINKLEN_IN_INODE) {

            OemName.Buffer = (PUCHAR) (&Mcb->Inode->i_block[0]);
            _SEH_VAR(OemNameBuffer) = OemName.Buffer;
            OemName.Length = (USHORT)Mcb->Inode->i_size;
            OemName.MaximumLength = OemName.Length + 1;

        } else {

            OemName.Length = (USHORT)Mcb->Inode->i_size;
            OemName.MaximumLength = OemName.Length + 1;
            OemName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                    OemName.MaximumLength,
                                    TAG('N', 'L', '2', 'E'));
            _SEH_VAR(OemNameBuffer) = OemName.Buffer;
            if (OemName.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH_LEAVE;
            }
            _SEH_VAR(bOemBuffer) = TRUE;
            RtlZeroMemory(OemName.Buffer, OemName.MaximumLength);

            Status = Ext2ReadInode(
                            IrpContext,
                            Vcb,
                            Mcb,
                            (ULONGLONG)0,
                            OemName.Buffer,
                            Mcb->Inode->i_size,
                            FALSE,
                            NULL);
            if (!NT_SUCCESS(Status)) {
                _SEH_LEAVE;
            }
        }

        /* convert Linux slash to Windows backslash */
        for (i=0; i < OemName.Length; i++) {
            if (OemName.Buffer[i] == '/') {
                OemName.Buffer[i] = '\\';
            }
        }

        /* convert oem string to unicode string */
        UniName.MaximumLength = (USHORT)Ext2OEMToUnicodeSize(Vcb, &OemName);
        if (UniName.MaximumLength <= 0) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }

        UniName.MaximumLength += 2;
        UniName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                    UniName.MaximumLength,
                                    TAG('N', 'L', '2', 'E'));
        _SEH_VAR(UniNameBuffer) = UniName.Buffer;
        if (UniName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }
        RtlZeroMemory(UniName.Buffer, UniName.MaximumLength);
        Status = Ext2OEMToUnicode(Vcb, &UniName, &OemName);
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }

        /* search the real target */
        Status = Ext2LookupFile(
                    IrpContext,
                    Vcb,
                    &UniName,
                    Parent,
                    &Target,
                    Linkdep
                    );
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_LINK_FAILED;
            _SEH_LEAVE;
        }

        /* we get the link target */
        if (IsMcbSymlink(Target)) {
            ASSERT(Target->Refercount > 0);
            Ext2ReferMcb(Target->Target);
            Mcb->Target = Target->Target;
            Ext2DerefMcb(Target);
        } else {
            Mcb->Target = Target;
        }

        ASSERT(Mcb->Target->Refercount > 0);

        SetLongFlag(Mcb->Flags, MCB_IS_SYMLINK);
        Mcb->FileSize = Target->FileSize;
        Mcb->FileAttr = Target->FileAttr;

    }
    _SEH_FINALLY(Ext2FollowLinkFinal_PSEH)
    _SEH_END;

    return Status;
}

BOOLEAN
Ext2IsSpecialSystemFile(
    IN PUNICODE_STRING FileName,
    IN BOOLEAN         bDirectory
    )
{
    PWSTR SpecialFileList[] = {
        L"pagefile.sys",
        NULL
        };

    PWSTR SpecialDirList[] = {
        L"Recycled",
        L"RECYCLER",
        NULL
        };

    PWSTR   entryName;
    ULONG   length;
    int     i;

    for (i = 0; TRUE; i++) {

        if (bDirectory) {
            entryName = SpecialDirList[i];
        } else {
            entryName = SpecialFileList[i];
        }

        if (NULL == entryName) {
            break;
        }

        length = wcslen(entryName) * sizeof(WCHAR);
        if (FileName->Length == length) {
            if ( 0 == _wcsnicmp( entryName, 
                                 FileName->Buffer,
                                 length / sizeof(WCHAR) )) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

_SEH_DEFINE_LOCALS(Ext2LookupFileFinal)
{
    PEXT2_VCB               Vcb;
    PEXT2_MCB               Ext2Mcb;
    PNTSTATUS               pStatus;
    PEXT2_MCB               Mcb;
    BOOLEAN                 bDirectory;
    BOOLEAN                 LockAcquired;
};

_SEH_FINALLYFUNC(Ext2LookupFileFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2LookupFileFinal);
    Ext2LookupFileFinal(_SEH_VAR(Vcb), _SEH_VAR(Ext2Mcb), _SEH_VAR(pStatus),
                        _SEH_VAR(Mcb), _SEH_VAR(bDirectory),
                        _SEH_VAR(LockAcquired));
}

VOID Ext2LookupFileFinal (
    IN PEXT2_VCB            Vcb,
    OUT PEXT2_MCB           Ext2Mcb,
    IN PNTSTATUS            pStatus,
    IN PEXT2_MCB            Mcb,
    IN BOOLEAN              bDirectory,
    IN BOOLEAN              LockAcquired
    )
{
    if (NT_SUCCESS(*pStatus)) {
        if (bDirectory) {
            if (IsMcbDirectory(Mcb)) {
                Ext2Mcb = Mcb;
            } else {
                Ext2DerefMcb(Mcb);
                *pStatus = STATUS_NOT_A_DIRECTORY;
            }
        } else {
            Ext2Mcb = Mcb;
        }
    }

    if (LockAcquired) {
        ExReleaseResourceLite(&Vcb->McbLock);
    }
}

NTSTATUS
Ext2LookupFile (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PUNICODE_STRING      FullName,
    IN PEXT2_MCB            Parent,
    OUT PEXT2_MCB *         Ext2Mcb,
    IN USHORT               Linkdep
    )
{
    NTSTATUS        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    UNICODE_STRING  FileName;
    PEXT2_MCB       Mcb = NULL;

    USHORT          i = 0, End;
    ULONG           Inode;
    ULONG           EntryOffset = 0;

    BOOLEAN         bParent = FALSE;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2LookupFileFinal);
        _SEH_VAR(Vcb) = Vcb;
        _SEH_VAR(Ext2Mcb) = *Ext2Mcb;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(Mcb) = Mcb;
        _SEH_VAR(bDirectory) = FALSE;
        _SEH_VAR(LockAcquired) = FALSE;

        ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);
        _SEH_VAR(LockAcquired) = TRUE;

        *Ext2Mcb = NULL;
        _SEH_VAR(Ext2Mcb) = NULL;

        DEBUG(DL_RES, ("Ext2LookupFile: %wZ\n", FullName));

        /* check names and parameters */
        if (FullName->Buffer[0] == L'\\') {
            Parent = Vcb->McbTree;
        } else if (Parent) {
            bParent = TRUE;
        } else {
            Parent = Vcb->McbTree;
        }

        /* default is the parent Mcb*/
        Mcb = Parent;
        _SEH_VAR(Mcb) = Parent;
        Ext2ReferMcb(Mcb);

        /* is empty file name or root node */
        End = FullName->Length/sizeof(WCHAR);
        if ( (End == 0) || (End == 1 && 
              FullName->Buffer[0] == L'\\')) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        /* is a directory expected ? */
        if (FullName->Buffer[End - 1] == L'\\') {
            _SEH_VAR(bDirectory) = TRUE;
        }

        /* loop with every sub name */
        while (i < End) {

            USHORT Start = 0;

            /* zero the prefix '\' */
            while(i < End && FullName->Buffer[i] == L'\\') i++;
            Start = i;

            /* zero the suffix '\' */
            while(i < End && (FullName->Buffer[i] != L'\\')) i++;

            if (i > Start) {

                FileName = *FullName;
                FileName.Buffer += Start;
                FileName.Length = (USHORT)((i - Start) * 2);

                /* make sure the parent is NULL */
                if (!IsMcbDirectory(Parent)) {
                    Status =  STATUS_NOT_A_DIRECTORY;
                    Ext2DerefMcb(Parent);
                    break;
                }

                /* search cached Mcb nodes */
                Mcb = Ext2SearchMcbWithoutLock(Parent, &FileName);
                _SEH_VAR(Mcb) = Mcb;

                if (Mcb) {

                    /* derefer the parent Mcb */
                    Ext2DerefMcb(Parent);
                    Status = STATUS_SUCCESS;
                    Parent = Mcb;

                } else {

                    /* need create new Mcb node */

                    /* is a valid ext2 name */
                    if (!Ext2IsNameValid(&FileName)) {
                        Status = STATUS_OBJECT_NAME_INVALID;
                        Ext2DerefMcb(Parent);
                        break;
                    }

                    /* seach the disk */
                    Status = Ext2ScanDir (
                                IrpContext,
                                Vcb,
                                Parent,
                                &FileName,
                                &EntryOffset,
                                &Inode);

                    if (NT_SUCCESS(Status)) {

                        PEXT2_MCB  Target = NULL;

                        /* check it's real parent */
                        if (IsMcbSymlink(Parent)) {
                            Target = Parent->Target;
                        } else {
                            Target = Parent;
                        }

                        /* allocate Mcb ... */
                        Mcb = Ext2AllocateMcb(
                                        Vcb, &FileName,
                                        &Target->FullName,
                                        FILE_ATTRIBUTE_NORMAL
                                        );
                        _SEH_VAR(Mcb) = Mcb;
                        if (!Mcb) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            Ext2DerefMcb(Parent);
                            break;
                        }

                        /* load inode information */
                        if (!Ext2LoadInode(Vcb, Inode, Mcb->Inode)) {
                            Status = STATUS_CANT_WAIT;
                            Ext2DerefMcb(Parent);
                            Ext2FreeMcb(Vcb, Mcb);
                            break;
                        }

                        /* set inode attribute */
                        if (Ext2IsReadOnly(Mcb->Inode->i_mode)) {
                            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_READONLY);
                        }

                        if (S_ISDIR(Mcb->Inode->i_mode)) {
                            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                        }

                        /* process special files under root directory */
                        if (IsMcbRoot(Target)) {
                            /* set hidden and system attributes for 
                               Recycled / RECYCLER / pagefile.sys */
                            BOOLEAN IsDirectory = IsMcbDirectory(Mcb);
                            if (Ext2IsSpecialSystemFile(&Mcb->ShortName, IsDirectory)) {
                                SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_HIDDEN);
                                SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_SYSTEM);
                            }
                        }

                        /* setup Mcb */
                        Mcb->CreationTime = Ext2NtTime(Mcb->Inode->i_ctime);
                        Mcb->LastAccessTime = Ext2NtTime(Mcb->Inode->i_atime);
                        Mcb->LastWriteTime = Ext2NtTime(Mcb->Inode->i_mtime);
                        Mcb->ChangeTime = Ext2NtTime(Mcb->Inode->i_mtime);
                        Mcb->iNo = Inode;
                        Mcb->EntryOffset = EntryOffset;
                        Mcb->FileSize.LowPart = Mcb->Inode->i_size;
                        if (S_ISREG(Mcb->Inode->i_mode)) {
                            Mcb->FileSize.HighPart = (LONG) (Mcb->Inode->i_size_high);
                        } else {
                            Mcb->FileSize.HighPart = 0;
                        }

                        /* process symlink */
                        if (S_ISLNK(Mcb->Inode->i_mode)) {
                            Status = Ext2FollowLink(
                                            IrpContext,
                                            Vcb,
                                            Parent,
                                            Mcb,
                                            Linkdep+1
                                        );
                            if (!NT_SUCCESS(Status)) {
                                Ext2DerefMcb(Parent);
                                Ext2FreeMcb(Vcb, Mcb);
                                Mcb = NULL;
                                _SEH_VAR(Mcb) = NULL;
                                break;
                            }
                            /* we got the target of this symlink */
                        }

                        /* add reference ... */
                        Ext2ReferMcb(Mcb);

                        /* add Mcb to it's parent tree*/
                        Ext2InsertMcb(Vcb, Target, Mcb);

                        /* it's safe to deref Parent Mcb */
                        Ext2DerefMcb(Parent);

                        /* linking this Mcb*/
                        Ext2LinkTailMcb(Vcb, Mcb);

                        /* set parent to preare re-scan */
                        Parent = Mcb;

                    } else {

                        /* derefernce it's parent */
                        Ext2DerefMcb(Parent);
                        break;
                    }
                }

            } else {

                /* there seems too many \ or / */
                /* Mcb should be already set to Parent */
                ASSERT(Mcb == Parent);
                Status = STATUS_SUCCESS;
                break;
            }
        }

    }
    _SEH_FINALLY(Ext2LookupFileFinal_PSEH)
    _SEH_END;

    return Status;
}

_SEH_DEFINE_LOCALS(Ext2ScanDirFinal)
{
    PEXT2_MCB Parent;
    PEXT2_DIR_ENTRY2 pDir;
    PWSTR InodeFileNameBuffer;
};

_SEH_FINALLYFUNC(Ext2ScanDirFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2ScanDirFinal);
    Ext2ScanDirFinal(_SEH_VAR(Parent), _SEH_VAR(pDir),
                     _SEH_VAR(InodeFileNameBuffer));
}

VOID
Ext2ScanDirFinal (
    IN PEXT2_MCB            Parent,
    IN PEXT2_DIR_ENTRY2     pDir,
    IN PWSTR                InodeFileNameBuffer
    )
{
    if (InodeFileNameBuffer != NULL) {
        ExFreePoolWithTag(InodeFileNameBuffer, EXT2_INAME_MAGIC);
        DEC_MEM_COUNT(PS_INODE_NAME, InodeFileNameBuffer,
                      (EXT2_NAME_LEN + 1) * 2);
    }

    if (pDir) {
        ExFreePoolWithTag(pDir, EXT2_DENTRY_MAGIC);
        DEC_MEM_COUNT(PS_DIR_ENTRY, pDir, sizeof(EXT2_DIR_ENTRY2));
    }

    Ext2DerefMcb(Parent);
}

NTSTATUS
Ext2ScanDir (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Parent,
    IN PUNICODE_STRING      FileName,
    IN OUT PULONG           Index,
    IN OUT PULONG           Inode
    )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    ULONG                   ByteOffset = 0;
    ULONG                   dwRet;
    ULONG                   RecLen;

    PEXT2_DIR_ENTRY2        pDir = NULL;

    OEM_STRING              OemName;
    UNICODE_STRING          InodeFileName;
    USHORT                  InodeFileNameLength;
    BOOLEAN                 bFound = FALSE;

    DEBUG(DL_RES, ("Ext2ScanDir: %wZ\\%wZ\n", &Parent->FullName, FileName));

    /* initialize InodeFileName */
    InodeFileName.Buffer = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2ScanDirFinal);
        _SEH_VAR(Parent) = Parent;
        _SEH_VAR(pDir) = NULL;
        _SEH_VAR(InodeFileNameBuffer) = NULL;

        /* grab parent's reference first */
        Ext2ReferMcb(Parent);

        /* bad request ! Can a man be pregnant ? Maybe:) */
        if (!IsMcbDirectory(Parent)) {
            Status = STATUS_NOT_A_DIRECTORY;
            _SEH_LEAVE;
        }

        /* parent is a symlink ? */
        if IsMcbSymlink(Parent) {
            if (Parent->Target) {
                Ext2ReferMcb(Parent->Target);
                Ext2DerefMcb(Parent);
                Parent = Parent->Target;
            } else {
                DbgBreak();
                Status = STATUS_NOT_A_DIRECTORY;
                _SEH_LEAVE;
            }
        }

        /* allocate buffer for unicode inode name */
        InodeFileName.Buffer = ExAllocatePoolWithTag(
                    PagedPool,
                    (EXT2_NAME_LEN + 1) * 2,
                    EXT2_INAME_MAGIC
                    );
        _SEH_VAR(InodeFileNameBuffer) = InodeFileName.Buffer;

        if (!InodeFileName.Buffer) {
            DEBUG(DL_ERR, ( "Ex2ScanDir: failed to allocate InodeFileName.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }
        INC_MEM_COUNT(PS_INODE_NAME, InodeFileName.Buffer, (EXT2_NAME_LEN + 1) * 2);

        /* again allocate entry buffer */
        pDir = (PEXT2_DIR_ENTRY2) ExAllocatePoolWithTag(
                                    PagedPool,
                                    sizeof(EXT2_DIR_ENTRY2),
                                    EXT2_DENTRY_MAGIC
                                    );
        _SEH_VAR(pDir) = pDir;
        if (!pDir) {
            DEBUG(DL_ERR, ( "Ex2ScanDir: failed to allocate pDir.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }
        INC_MEM_COUNT(PS_DIR_ENTRY, pDir, sizeof(EXT2_DIR_ENTRY2));

        ByteOffset = 0;

        while (!bFound && ByteOffset < Parent->Inode->i_size) {

            RtlZeroMemory(pDir, sizeof(EXT2_DIR_ENTRY2));

            //
            // reading dir entries from Dcb
            //

            Status = Ext2ReadInode(
                        IrpContext,
                        Vcb,
                        Parent,
                        (ULONGLONG)ByteOffset,
                        (PVOID)pDir,
                        min(sizeof(EXT2_DIR_ENTRY2),
                            Parent->Inode->i_size - ByteOffset),
                        FALSE,
                        &dwRet);


            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_ERR, ( "Ext2ScanDir: failed to read directory.\n"));
                _SEH_LEAVE;
            }

            if (pDir->rec_len == 0) {
                RecLen = BLOCK_SIZE - (ByteOffset & (BLOCK_SIZE - 1));
            } else {
                RecLen = pDir->rec_len;
            }

            if (pDir->inode) {

                if ((pDir->inode >= INODES_COUNT)) {
                    Status = STATUS_FILE_CORRUPT_ERROR;
                    _SEH_LEAVE;
                }

                OemName.Buffer = pDir->name;
                OemName.Length = (pDir->name_len & 0xff);
                OemName.MaximumLength = OemName.Length;

                InodeFileNameLength = (USHORT)
                        Ext2OEMToUnicodeSize(Vcb, &OemName);
                if (InodeFileNameLength <= 0) {
                    DEBUG(DL_CP, ("Ext2ScanDir: failed to count unicode length of %s.\n", OemName.Buffer));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }

                InodeFileName.Length = 0;
                InodeFileName.MaximumLength = (EXT2_NAME_LEN + 1) * 2;

                RtlZeroMemory( InodeFileName.Buffer,
                               InodeFileNameLength + 2);

                Status = Ext2OEMToUnicode(Vcb, &InodeFileName, &OemName);
                if (!NT_SUCCESS(Status)) {
                    DEBUG(DL_CP, ("Ext2ScanDir: failed to convert %s to unicode.\n", OemName.Buffer));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }

                if (!RtlCompareUnicodeString(
                        FileName,
                        &InodeFileName,
                        TRUE ))  {

                    bFound = TRUE;
                    *Index = ByteOffset;
                    *Inode = pDir->inode;
                    Status = STATUS_SUCCESS;

                    DEBUG(DL_INF, ("Ext2ScanDir: Found: Name=%S Inode=%xh\n",
                                InodeFileName.Buffer, pDir->inode ));
                }
            }
    
            ByteOffset += RecLen;

        }

        if (!bFound) {
            Status = STATUS_NO_SUCH_FILE;
        }

    }
    _SEH_FINALLY(Ext2ScanDirFinal_PSEH)
    _SEH_END;
    
    return Status;
}

_SEH_DEFINE_LOCALS(Ext2CreateFileFinal)
{
    PEXT2_IRP_CONTEXT IrpContext;
    PEXT2_VCB Vcb;
    PNTSTATUS pStatus;
    PIO_STACK_LOCATION IrpSp;
    PEXT2_FCB Fcb;
    PEXT2_MCB Mcb;
    PEXT2_MCB SymLink;
    PEXT2_CCB Ccb;
    PEXT2_FCB ParentFcb;
    PUNICODE_STRING FileName;
    BOOLEAN bParentFcbCreated;
    BOOLEAN bCreated;
    BOOLEAN bMainResourceAcquired;
    BOOLEAN NoIntermediateBuffering;
};

_SEH_FINALLYFUNC(Ext2CreateFileFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2CreateFileFinal);
    Ext2CreateFileFinal(_SEH_VAR(IrpContext), _SEH_VAR(Vcb), _SEH_VAR(pStatus), 
                        _SEH_VAR(IrpSp), _SEH_VAR(Fcb), _SEH_VAR(Mcb), 
                        _SEH_VAR(SymLink), _SEH_VAR(Ccb), _SEH_VAR(ParentFcb),
                        _SEH_VAR(FileName), _SEH_VAR(bParentFcbCreated),
                        _SEH_VAR(bCreated), _SEH_VAR(bMainResourceAcquired),
                        _SEH_VAR(NoIntermediateBuffering));
}

_SEH_FINALLYFUNC(Ext2CreateFileFinal2_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2CreateFileFinal);
    ExReleaseResourceLite(&(_SEH_VAR(Fcb)->PagingIoResource));
}

VOID
Ext2CreateFileFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PNTSTATUS            pStatus,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PEXT2_FCB            Fcb,
    IN PEXT2_MCB            Mcb,
    IN PEXT2_MCB            SymLink,
    IN PEXT2_CCB            Ccb,
    IN PEXT2_FCB            ParentFcb,
    IN PUNICODE_STRING      FileName,
    IN BOOLEAN              bParentFcbCreated,
    IN BOOLEAN              bCreated,
    IN BOOLEAN              bMainResourceAcquired,
    IN BOOLEAN              NoIntermediateBuffering
    )
{
    /* cleanup Fcb and Ccb, Mcb if necessary */
    if (!NT_SUCCESS(*pStatus)) {

        if (Ccb != NULL) {

            DbgBreak();

            ASSERT(Fcb != NULL);
            ASSERT(Fcb->Mcb != NULL);

            DEBUG(DL_ERR, ("Ext2CreateFile: failed to create %wZ status = %xh\n",
                            &Fcb->Mcb->FullName, *pStatus));

            Ext2DerefXcb(&Fcb->OpenHandleCount);
            Ext2DerefXcb(&Fcb->ReferenceCount);

            if (!IsDirectory(Fcb)) {
                if (NoIntermediateBuffering) {
                    Fcb->NonCachedOpenCount--;
                } else {
                    ClearFlag(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED);
                }
            }

            Ext2DerefXcb(&Vcb->OpenFileHandleCount);
            Ext2DerefXcb(&Vcb->ReferenceCount);
            
            IoRemoveShareAccess(IrpSp->FileObject, &Fcb->ShareAccess);

            IrpSp->FileObject->FsContext = NULL;
            IrpSp->FileObject->FsContext2 = NULL;
            IrpSp->FileObject->PrivateCacheMap = NULL;
            IrpSp->FileObject->SectionObjectPointer = NULL;

            Ext2FreeCcb(Ccb);
        }
    }

    if (Fcb && Ext2DerefXcb(&Fcb->ReferenceCount) == 0) {

        if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {

            LARGE_INTEGER Size;
            ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
            _SEH_TRY {
                Size.QuadPart = 0;
                Mcb->FileSize = Fcb->RealSize;
                Ext2TruncateFile(IrpContext, Vcb, Mcb, &Size);
            }
            _SEH_FINALLY(Ext2CreateFileFinal2_PSEH)
            _SEH_END;
        }

        if (bCreated) {
            Ext2DeleteFile(IrpContext, Vcb, Mcb);
        }

        Ext2FreeFcb(Fcb);
        Fcb = NULL;
        bMainResourceAcquired = FALSE;
    }

    if (bMainResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }

    /* free file name buffer */
    if (FileName->Buffer) {
        DEC_MEM_COUNT(PS_FILE_NAME, FileName->Buffer, FileName->MaximumLength);
        ExFreePoolWithTag(FileName->Buffer, EXT2_FNAME_MAGIC);
    }

    /* dereference parent Fcb, free it if it goes to zero */
    if (ParentFcb) {
        ClearLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);
        if (bParentFcbCreated) {
            if (Ext2DerefXcb(&ParentFcb->ReferenceCount) == 0) {
                Ext2FreeFcb(ParentFcb);
            }
        }
    }

    /* drop SymLink's refer: If succeeds, Ext2AllocateCcb should refer 
        it already. It fails, we need release the refer to let it freed */
    if (SymLink) {
        Ext2DerefMcb(SymLink);
    }
}

NTSTATUS
Ext2CreateFile (
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PBOOLEAN          OpPostIrp
    )
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  IrpSp;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_MCB           Mcb = NULL;
    PEXT2_MCB           SymLink = NULL;
    PEXT2_CCB           Ccb = NULL;

    PEXT2_FCB           ParentFcb = NULL;
    PEXT2_MCB           ParentMcb = NULL;

    UNICODE_STRING      FileName;
    PIRP                Irp;

    ULONG               Options;
    ULONG               CreateDisposition;

    BOOLEAN             bFcbAllocated = FALSE;

    BOOLEAN             OpenDirectory;
    BOOLEAN             OpenTargetDirectory;
    BOOLEAN             CreateDirectory;
    BOOLEAN             SequentialOnly;
    BOOLEAN             IsPagingFile;
    BOOLEAN             DirectoryFile;
    BOOLEAN             NonDirectoryFile;
    BOOLEAN             NoEaKnowledge;
    BOOLEAN             DeleteOnClose;
    BOOLEAN             TemporaryFile;
    BOOLEAN             CaseSensitive;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;

    RtlZeroMemory(&FileName, sizeof(UNICODE_STRING));

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Options  = IrpSp->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

    NonDirectoryFile = IsFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    SequentialOnly = IsFlagOn(Options, FILE_SEQUENTIAL_ONLY);
    NoEaKnowledge = IsFlagOn(Options, FILE_NO_EA_KNOWLEDGE);
    DeleteOnClose = IsFlagOn(Options, FILE_DELETE_ON_CLOSE);

    CaseSensitive = IsFlagOn(IrpSp->Flags, SL_CASE_SENSITIVE);

    TemporaryFile = IsFlagOn(IrpSp->Parameters.Create.FileAttributes,
                                   FILE_ATTRIBUTE_TEMPORARY );

    CreateDisposition = (Options >> 24) & 0x000000ff;

    IsPagingFile = IsFlagOn(IrpSp->Flags, SL_OPEN_PAGING_FILE);

    CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

    *OpPostIrp = FALSE;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2CreateFileFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(Vcb) = Vcb;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(IrpSp) = IrpSp;
        _SEH_VAR(Fcb) = NULL;
        _SEH_VAR(Mcb) = NULL;
        _SEH_VAR(SymLink) = NULL;
        _SEH_VAR(Ccb) = NULL;
        _SEH_VAR(ParentFcb) = NULL;
        _SEH_VAR(FileName) = &FileName;
        _SEH_VAR(bParentFcbCreated) = FALSE;
        _SEH_VAR(bCreated) = FALSE;
        _SEH_VAR(bMainResourceAcquired) = FALSE;
        _SEH_VAR(NoIntermediateBuffering) = IsFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );;

        FileName.MaximumLength = IrpSp->FileObject->FileName.MaximumLength;
        FileName.Length = IrpSp->FileObject->FileName.Length;

        if (IrpSp->FileObject->RelatedFileObject) {
            ParentFcb = (PEXT2_FCB)(IrpSp->FileObject->RelatedFileObject->FsContext);
            _SEH_VAR(ParentFcb) = ParentFcb;
        }

        if (ParentFcb) {
            ParentMcb = ParentFcb->Mcb;
            SetLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);
            Ext2ReferMcb(ParentMcb);
        }

        if (FileName.Length == 0) {

            if (ParentFcb) {
                Mcb = ParentFcb->Mcb;
                _SEH_VAR(Mcb) = ParentFcb->Mcb;
                Status = STATUS_SUCCESS;
                goto McbExisting;
            } else {
                DbgBreak();
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }
        }

        FileName.Buffer = ExAllocatePoolWithTag(
                                PagedPool,
                                FileName.MaximumLength,
                                EXT2_FNAME_MAGIC
                            );

        if (!FileName.Buffer) {  
            DEBUG(DL_ERR, ( "Ex2CreateFile: failed to allocate FileName.\n")); 
            Status = STATUS_INSUFFICIENT_RESOURCES;
            if (ParentFcb) {
                Ext2DerefMcb(ParentMcb);
            }
            _SEH_LEAVE;
        }

        INC_MEM_COUNT(PS_FILE_NAME, FileName.Buffer, FileName.MaximumLength);

        RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);
        RtlCopyMemory(FileName.Buffer, IrpSp->FileObject->FileName.Buffer, FileName.Length);

        if ((FileName.Length > sizeof(WCHAR)) &&
            (FileName.Buffer[1] == L'\\') &&
            (FileName.Buffer[0] == L'\\')) {
            
            FileName.Length -= sizeof(WCHAR);
            
            RtlMoveMemory( &FileName.Buffer[0],
                &FileName.Buffer[1],
                FileName.Length );
            
            //
            //  Bad Name if there are still beginning backslashes.
            //
            
            if ((FileName.Length > sizeof(WCHAR)) &&
                (FileName.Buffer[1] == L'\\') &&
                (FileName.Buffer[0] == L'\\')) {

                if (ParentFcb) {
                    Ext2DerefMcb(ParentMcb);
                }

                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH_LEAVE;
            }
        }

        if (IsFlagOn(Options, FILE_OPEN_BY_FILE_ID)) {
            Status = STATUS_NOT_IMPLEMENTED;
            if (ParentFcb) {
                Ext2DerefMcb(ParentMcb);
            }
            _SEH_LEAVE;
        }

        DEBUG(DL_INF, ( "Ext2CreateFile: %wZ Paging=%d Option: %xh:"
            "Dir=%d NonDir=%d OpenTarget=%d NC=%d DeleteOnClose=%d\n",
            &FileName, IsPagingFile, IrpSp->Parameters.Create.Options,
            DirectoryFile, NonDirectoryFile, OpenTargetDirectory, 
            _SEH_VAR(NoIntermediateBuffering), DeleteOnClose ));

        DEBUG(DL_RES, ("Ext2CreateFile: Lookup 1st: %wZ at %S\n",
              &FileName, ParentMcb ? ParentMcb->FullName.Buffer : L" "));
        Status = Ext2LookupFile(
                        IrpContext,
                        Vcb,
                        &FileName,
                        ParentMcb,
                        &Mcb,
                        0 );

McbExisting:

        if (!NT_SUCCESS(Status)) {

            UNICODE_STRING  PathName;
            UNICODE_STRING  RealName;
            UNICODE_STRING  RemainName;

            PathName = FileName;
            Mcb = NULL;
            _SEH_VAR(Mcb) = NULL;

            if (Status == STATUS_LINK_FAILED) {
                if (CreateDisposition == FILE_CREATE) {
                    Irp->IoStatus.Information = FILE_EXISTS;
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    _SEH_LEAVE;
                } else {
                    Status = STATUS_OBJECT_NAME_INVALID;
                    _SEH_LEAVE;
                }
            }

            if (PathName.Buffer[PathName.Length/2 - 1] == L'\\') {
                if (DirectoryFile) {
                    PathName.Length -=2;
                    PathName.Buffer[PathName.Length/2] = 0;
                } else {
                    Status = STATUS_NOT_A_DIRECTORY;
                    _SEH_LEAVE;
                }
            }

            if (!ParentMcb) {
                if (PathName.Buffer[0] != L'\\') {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH_LEAVE;
                } else {
                    ParentMcb = Vcb->McbTree;
                }
                Ext2ReferMcb(ParentMcb);
            }

Dissecting:

            FsRtlDissectName(PathName, &RealName, &RemainName);

            if (((RemainName.Length != 0) && (RemainName.Buffer[0] == L'\\')) ||
                (RealName.Length >= 256 * sizeof(WCHAR))) {
                Status = STATUS_OBJECT_NAME_INVALID;
                Ext2DerefMcb(ParentMcb);
                _SEH_LEAVE;
            }

            if (RemainName.Length != 0) {

                PEXT2_MCB   RetMcb;

                DEBUG(DL_RES, ("Ext2CreateFile: Lookup 2nd: %wZ\\%wZ\n",
                                    &ParentMcb->FullName, &RealName));

                Status = Ext2LookupFile (
                                IrpContext,
                                Vcb,
                                &RealName,
                                ParentMcb,
                                &RetMcb,
                                0);

                /* time to dereference ParentMcb */
                Ext2DerefMcb(ParentMcb);

                /* quit name resolving loop */
                if (!NT_SUCCESS(Status)) {
                    _SEH_LEAVE;
                }

                /* RetMcb is already refered */
                ParentMcb = RetMcb;
                PathName  = RemainName;

                goto Dissecting;
            }

            /* is name valid */
            if ( FsRtlDoesNameContainWildCards(&RealName) || 
                !Ext2IsNameValid(&RealName)) {
                Status = STATUS_OBJECT_NAME_INVALID;
                Ext2DerefMcb(ParentMcb);
                _SEH_LEAVE;
            }

            /* symlink must use it's target */
            if (IsMcbSymlink(ParentMcb)) {
                Ext2ReferMcb(ParentMcb->Target);
                Ext2DerefMcb(ParentMcb);
                ParentMcb = ParentMcb->Target;
            }

            /* clear BUSY bit from original ParentFcb */
            if (ParentFcb) {
                ClearLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);
            }

            /* get the ParentFcb, allocate it if needed ... */
            ParentFcb = ParentMcb->Fcb;
            _SEH_VAR(ParentFcb) = ParentMcb->Fcb;
            if (!ParentFcb) {
                ParentFcb = Ext2AllocateFcb(Vcb, ParentMcb);
                _SEH_VAR(ParentFcb) = ParentFcb;
                if (!ParentFcb) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    Ext2DerefMcb(ParentMcb);
                    _SEH_LEAVE;
                }
                _SEH_VAR(bParentFcbCreated) = TRUE;
                Ext2ReferXcb(&ParentFcb->ReferenceCount);
            }
            SetLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);

            /* now it's safe to clear MCB's BUSY flag*/
            Ext2DerefMcb(ParentMcb);

            // We need to create a new one ?
            if ((CreateDisposition == FILE_CREATE ) ||
                (CreateDisposition == FILE_OPEN_IF) ||
                (CreateDisposition == FILE_OVERWRITE_IF)) {

                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH_LEAVE;
                }

                if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                    IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                  Vcb->Vpb->RealDevice );
                    SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
                    Ext2RaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                }

                if (DirectoryFile) {
                    if (TemporaryFile) {
                        DbgBreak();
                        Status = STATUS_INVALID_PARAMETER;
                        _SEH_LEAVE;
                    }
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH_LEAVE;
                }

                /* allocate inode and construct entry for this file */
                Status = Ext2CreateInode(
                            IrpContext,
                            Vcb, 
                            ParentFcb,
                            DirectoryFile ? EXT2_FT_DIR : EXT2_FT_REG_FILE,
                            IrpSp->Parameters.Create.FileAttributes,
                            &RealName
                            );
                
                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                    _SEH_LEAVE;
                }

                _SEH_VAR(bCreated) = TRUE;
                DEBUG(DL_RES, ("Ext2CreateFile: Confirm creation: %wZ\\%wZ\n",
                                &ParentMcb->FullName, &RealName));

                Irp->IoStatus.Information = FILE_CREATED;                    
                Status = Ext2LookupFile (
                                IrpContext,
                                Vcb,
                                &RealName,
                                ParentMcb,
                                &Mcb,
                                0);
                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

            } else if (OpenTargetDirectory) {

                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH_LEAVE;
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH_LEAVE;
                }

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = RealName.Length;

                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer,
                               RealName.Buffer,
                               RealName.Length );

                Fcb = ParentFcb;
                _SEH_VAR(Fcb) = ParentFcb;

                Irp->IoStatus.Information = FILE_DOES_NOT_EXIST;
                Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                _SEH_LEAVE;
            }

        } else { // File / Dir already exists.

            /* here already get Mcb referred */
            if (OpenTargetDirectory) {

                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    Ext2DerefMcb(Mcb);
                    _SEH_LEAVE;
                }

                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_SUCCESS;

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = Mcb->ShortName.Length;

                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer,
                               Mcb->ShortName.Buffer,
                               Mcb->ShortName.Length );

                // use's it's parent since it's open-target operation
                Ext2ReferMcb(Mcb->Parent);
                Ext2DerefMcb(Mcb);
                Mcb = Mcb->Parent;
                _SEH_VAR(Mcb) = Mcb->Parent;

                goto Openit;
            }

            // We can not create if one exists
            if (CreateDisposition == FILE_CREATE) {
                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_OBJECT_NAME_COLLISION;
                Ext2DerefMcb(Mcb);
                _SEH_LEAVE;
            }

            /* directory forbits us to do the followings ... */
            if (IsMcbDirectory(Mcb)) {

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    Ext2DerefMcb(Mcb);
                    _SEH_LEAVE;
                }

                if (NonDirectoryFile) {
                    Status = STATUS_FILE_IS_A_DIRECTORY;
                    Ext2DerefMcb(Mcb);
                    _SEH_LEAVE;
                }

                if (Mcb->iNo == EXT2_ROOT_INO) {

                    if (DeleteOnClose) {
                        Status = STATUS_CANNOT_DELETE;
                        Ext2DerefMcb(Mcb);
                        _SEH_LEAVE;
                    }

                    if (OpenTargetDirectory) {
                        DbgBreak();
                        Status = STATUS_INVALID_PARAMETER;
                        Ext2DerefMcb(Mcb);
                        _SEH_LEAVE;
                    }
                }

            } else {

                if (DirectoryFile) {
                    Status = STATUS_NOT_A_DIRECTORY;;
                    Ext2DerefMcb(Mcb);
                    _SEH_LEAVE;
                }
            }

            Irp->IoStatus.Information = FILE_OPENED;
        }

Openit:
        /* Mcb should already be referred and symlink is too */
        if (Mcb) {

            ASSERT(Mcb->Refercount > 0);

            /* refer it's target if it's a symlink, so both refered */
            if (IsMcbSymlink(Mcb)) {
                Ext2ReferMcb(Mcb->Target);
                SymLink = Mcb;
                _SEH_VAR(SymLink) = Mcb;
                Mcb = Mcb->Target;
                _SEH_VAR(Mcb) = Mcb->Target;
                ASSERT (!IsMcbSymlink(Mcb));
            }

            Fcb = Mcb->Fcb;
            _SEH_VAR(Fcb) = Mcb->Fcb;
            if (Fcb == NULL) {

                /* allocate Fcb for this file */
                Fcb = Ext2AllocateFcb (Vcb, Mcb);
                _SEH_VAR(Fcb) = Fcb;
                if (Fcb) {
                    bFcbAllocated = TRUE;
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                if (IsPagingFile) {
                    Status = STATUS_SHARING_VIOLATION;
                    Fcb = NULL;
                    _SEH_VAR(Fcb) = NULL;
                }
            }

            /* Now it's safe to defer Mcb */
            Ext2DerefMcb(Mcb);
        }
        
        if (Fcb) {

            /* grab Fcb's reference first to avoid the race between
               Ext2Close  (it could free the Fcb we are accessing) */
            Ext2ReferXcb(&Fcb->ReferenceCount);

            ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
            _SEH_VAR(bMainResourceAcquired) = TRUE;

            /* check Mcb reference */
            ASSERT(Fcb->Mcb->Refercount > 0);

            /* file delted ? */
            if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
                Status = STATUS_FILE_DELETED;
                _SEH_LEAVE;
            }

            /* check access and oplock access for opened files */
            if (!bFcbAllocated  && !IsDirectory(Fcb)) {

                /* whether there's batch oplock grabed on the file */
                if (FsRtlCurrentBatchOplock(&Fcb->Oplock)) {

                    Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                    /* break the batch lock if the sharing check fails */
                    Status = FsRtlCheckOplock( &Fcb->Oplock,
                                               IrpContext->Irp,
                                               IrpContext,
                                               Ext2OplockComplete,
                                               Ext2LockIrp );

                    if ( Status != STATUS_SUCCESS &&
                         Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {
                        *OpPostIrp = TRUE;
                        _SEH_LEAVE;
                    }
                }
            }
 
            if (_SEH_VAR(bCreated)) {

                //
                //  This file is just created.
                //

                if (DirectoryFile) {

                    UNICODE_STRING EntryName;
                    USHORT  NameBuf[6];

                    RtlZeroMemory(NameBuf, 6 * sizeof(USHORT));

                    EntryName.Length = EntryName.MaximumLength = 2;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = (USHORT)'.';

                    Ext2AddEntry( IrpContext,
                                  Vcb, Fcb,
                                  EXT2_FT_DIR,
                                  Fcb->Mcb->iNo,
                                  &EntryName,
                                  NULL
                             );

                    Ext2SaveInode( IrpContext,
                                   Vcb,
                                   Fcb->Mcb->iNo,
                                   Fcb->Inode
                            );

                    EntryName.Length = EntryName.MaximumLength = 4;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = NameBuf[1] = (USHORT)'.';

                    Ext2AddEntry( IrpContext,
                                  Vcb, Fcb,
                                  EXT2_FT_DIR,
                                  Fcb->Mcb->Parent->iNo,
                                  &EntryName,
                                  NULL
                            );

                    Ext2SaveInode( IrpContext, Vcb,
                                   Fcb->Mcb->Parent->iNo,
                                   ParentFcb->Inode
                            );
                } else {

                    Fcb->Header.AllocationSize.QuadPart =
                        Irp->Overlay.AllocationSize.QuadPart;

                    if (Fcb->Header.AllocationSize.QuadPart > 0) {
                        Ext2ExpandFile( IrpContext,
                                    Vcb,
                                    Fcb->Mcb,
                                    &(Fcb->Header.AllocationSize)
                                    );
                        Fcb->RealSize = Fcb->Header.AllocationSize;
                        SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                    }
                }

            } else {

                //
                //  This file alreayd exists.
                //

                if (DeleteOnClose) {

                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;

                        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                      Vcb->Vpb->RealDevice );

                        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

                        Ext2RaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                    }

                    SetLongFlag(Fcb->Flags, FCB_DELETE_ON_CLOSE);

                } else {

                    //
                    // Just to Open file (Open/OverWrite ...)
                    //

                    if ((!IsDirectory(Fcb)) && (IsFlagOn(IrpSp->FileObject->Flags,
                                                FO_NO_INTERMEDIATE_BUFFERING))) {
                        Fcb->Header.IsFastIoPossible = FastIoIsPossible;

                        if (Fcb->SectionObject.DataSectionObject != NULL) {

                            if (Fcb->NonCachedOpenCount == Fcb->OpenHandleCount) {

                                if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                                    CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
                                    ClearLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                                }

                                CcPurgeCacheSection(&Fcb->SectionObject,
                                                     NULL,
                                                     0,
                                                     FALSE );
                            }
                        }
                    }
                }
            }

            if (!IsDirectory(Fcb)) {

                if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                    if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile){
                        DesiredAccess |= DELETE;
                    } else if (((CreateDisposition == FILE_OVERWRITE) ||
                            (CreateDisposition == FILE_OVERWRITE_IF)) && !IsPagingFile) {
                        DesiredAccess |= (FILE_WRITE_DATA | FILE_WRITE_EA |
                                          FILE_WRITE_ATTRIBUTES );
                    }
                }

                if (!bFcbAllocated) {

                    //
                    //  check the oplock state of the file
                    //

                    Status = FsRtlCheckOplock(  &Fcb->Oplock,
                                                IrpContext->Irp,
                                                IrpContext,
                                                Ext2OplockComplete,
                                                Ext2LockIrp );

                    if ( Status != STATUS_SUCCESS &&
                         Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {
                        *OpPostIrp = TRUE;
                        _SEH_LEAVE;
                    }
                }
            }

            if (Fcb->OpenHandleCount > 0) {

                /* check the shrae access conflicts */
                Status = IoCheckShareAccess( DesiredAccess,
                                             ShareAccess,
                                             IrpSp->FileObject,
                                             &(Fcb->ShareAccess),
                                             TRUE );
                if (!NT_SUCCESS(Status)) {
                    _SEH_LEAVE;
                }

            } else {

                /* set share access rights */
                IoSetShareAccess( DesiredAccess,
                                  ShareAccess,
                                  IrpSp->FileObject,
                                  &(Fcb->ShareAccess) );
            }

            Ccb = Ext2AllocateCcb(SymLink);
            _SEH_VAR(Ccb) = Ccb;
            if (!Ccb) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DbgBreak();
                _SEH_LEAVE;
            }

            Ext2ReferXcb(&Fcb->OpenHandleCount);
            Ext2ReferXcb(&Fcb->ReferenceCount);

            if (!IsDirectory(Fcb)) {
                if (_SEH_VAR(NoIntermediateBuffering)) {
                    Fcb->NonCachedOpenCount++;
                } else {
                    SetFlag(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED);
                }
            }

            Ext2ReferXcb(&Vcb->OpenFileHandleCount);
            Ext2ReferXcb(&Vcb->ReferenceCount);
            
            IrpSp->FileObject->FsContext = (void*)Fcb;
            IrpSp->FileObject->FsContext2 = (void*) Ccb;
            IrpSp->FileObject->PrivateCacheMap = NULL;
            IrpSp->FileObject->SectionObjectPointer = &(Fcb->SectionObject);

            DEBUG(DL_INF, ( "Ext2CreateFile: %wZ OpenCount=%u ReferCount=%u NonCachedCount=%u\n",
                &Fcb->Mcb->FullName, Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount));

            Status = STATUS_SUCCESS;

            if (_SEH_VAR(bCreated)) {

                if (IsDirectory(Fcb)) {
                    Ext2NotifyReportChange(
                                   IrpContext,
                                   Vcb,
                                   Fcb->Mcb,
                                   FILE_NOTIFY_CHANGE_DIR_NAME,
                                   FILE_ACTION_ADDED );
                } else {
                     Ext2NotifyReportChange(
                                   IrpContext,
                                   Vcb,
                                   Fcb->Mcb,
                                   FILE_NOTIFY_CHANGE_FILE_NAME,
                                   FILE_ACTION_ADDED );
                }

            } else if (!IsDirectory(Fcb)) {

                if ( DeleteOnClose || 
                    IsFlagOn(DesiredAccess, FILE_WRITE_DATA) || 
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {
                    if (!MmFlushImageSection( &Fcb->SectionObject,
                                              MmFlushForWrite )) {

                        Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                                 STATUS_SHARING_VIOLATION;
                        _SEH_LEAVE;
                    }
                }

                if ((CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {

                    if (IsDirectory(Fcb)) {
                        Status = STATUS_FILE_IS_A_DIRECTORY;
                        _SEH_LEAVE;
                    }

                    if (SymLink != NULL) {
                        DbgBreak();
                        Status = STATUS_INVALID_PARAMETER;
                        _SEH_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {

                        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                      Vcb->Vpb->RealDevice );
                        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
                        Ext2RaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                    }

                    Status = Ext2SupersedeOrOverWriteFile(
                                                IrpContext,
                                                IrpSp->FileObject,
                                                Vcb,
                                                Fcb,
                                                &Irp->Overlay.AllocationSize,
                                                CreateDisposition );

                    if (!NT_SUCCESS(Status)) {
                        DbgBreak();
                        _SEH_LEAVE;
                    }

                    Ext2NotifyReportChange(
                               IrpContext,
                               Vcb,
                               Fcb->Mcb,
                               FILE_NOTIFY_CHANGE_LAST_WRITE |
                               FILE_NOTIFY_CHANGE_ATTRIBUTES |
                               FILE_NOTIFY_CHANGE_SIZE,
                               FILE_ACTION_MODIFIED );


                    if (CreateDisposition == FILE_SUPERSEDE) {
                        Irp->IoStatus.Information = FILE_SUPERSEDED;
                    } else {
                        Irp->IoStatus.Information = FILE_OVERWRITTEN;
                    }
                }
            }

        } else {
            DbgBreak();
            _SEH_LEAVE;
        }

    }
    _SEH_FINALLY(Ext2CreateFileFinal_PSEH)
    _SEH_END;
    
    return Status;
}

NTSTATUS
Ext2CreateVolume(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;

    NTSTATUS            Status;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;

    ULONG               Options;
    BOOLEAN             DirectoryFile;
    BOOLEAN             OpenTargetDirectory;

    ULONG               CreateDisposition;

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Options  = IrpSp->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY);

    CreateDisposition = (Options >> 24) & 0x000000ff;

    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = IrpSp->Parameters.Create.ShareAccess;

    if (DirectoryFile) {
        return STATUS_NOT_A_DIRECTORY;
    }

    if (OpenTargetDirectory) {
        DbgBreak();
        return STATUS_INVALID_PARAMETER;
    }

    if ( (CreateDisposition != FILE_OPEN) && 
         (CreateDisposition != FILE_OPEN_IF) ) {
        return STATUS_ACCESS_DENIED;
    }

    Status = STATUS_SUCCESS;

    if (Vcb->OpenHandleCount > 0) {
        Status = IoCheckShareAccess( DesiredAccess, ShareAccess,
                                     IrpSp->FileObject,
                                     &(Vcb->ShareAccess), TRUE);

        if (!NT_SUCCESS(Status)) {
            goto errorout;
        }
    } else {
        IoSetShareAccess( DesiredAccess, ShareAccess,
                          IrpSp->FileObject,
                          &(Vcb->ShareAccess)   );
    }

    if (FlagOn(DesiredAccess, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA)) {
        Ext2FlushFiles(IrpContext, Vcb, FALSE);
        Ext2FlushVolume(IrpContext, Vcb, FALSE);
    }

    {
        PEXT2_CCB   Ccb = Ext2AllocateCcb(NULL);

        if (Ccb == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
        IrpSp->FileObject->FsContext  = Vcb;
        IrpSp->FileObject->FsContext2 = Ccb;
        IrpSp->FileObject->Vpb = Vcb->Vpb;

        Ext2ReferXcb(&Vcb->ReferenceCount);
        Ext2ReferXcb(&Vcb->OpenHandleCount);

        Irp->IoStatus.Information = FILE_OPENED;
    }

errorout:

    return Status;
}

_SEH_DEFINE_LOCALS(Ext2CreateFinal)
{
    PEXT2_IRP_CONTEXT IrpContext;
    PEXT2_VCB Vcb;
    PNTSTATUS pStatus;
    BOOLEAN VcbResourceAcquired;
};

_SEH_FINALLYFUNC(Ext2CreateFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2CreateFinal);
    Ext2CreateFinal(_SEH_VAR(IrpContext), _SEH_VAR(Vcb), _SEH_VAR(pStatus), 
                    _SEH_VAR(VcbResourceAcquired));
}

VOID
Ext2CreateFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PNTSTATUS            pStatus,
    IN BOOLEAN              VcbResourceAcquired
    )
{
    if (VcbResourceAcquired) {
        ExReleaseResourceLite(&Vcb->MainResource);
    }

    if (!IrpContext->ExceptionInProgress)  {
        if ( *pStatus == STATUS_PENDING ||
              *pStatus == STATUS_CANT_WAIT) {
            *pStatus = Ext2QueueRequest(IrpContext);
        } else {
            Ext2CompleteIrpContext(IrpContext, *pStatus);
        }
    }
}

NTSTATUS
Ext2Create (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    PEXT2_VCB           Vcb = 0;
    NTSTATUS            Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PEXT2_FCBVCB        Xcb = NULL;
    BOOLEAN             PostIrp = FALSE;

    DeviceObject = IrpContext->DeviceObject;
    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Xcb = (PEXT2_FCBVCB) (IrpSp->FileObject->FsContext);
    
    if (IsExt2FsDevice(DeviceObject)) {

        DEBUG(DL_INF, ( "Ext2Create: Create on main device object.\n"));

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        Ext2CompleteIrpContext(IrpContext, Status);        

        return Status;
    }
   
    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2CreateFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(Vcb) = 0;
        _SEH_VAR(VcbResourceAcquired) = FALSE;

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        _SEH_VAR(Vcb) = Vcb;
        ASSERT(Vcb->Identifier.Type == EXT2VCB);
        ASSERT(IsMounted(Vcb));
        IrpSp->FileObject->Vpb = Vcb->Vpb;

        if (!ExAcquireResourceExclusiveLite(
                &Vcb->MainResource, TRUE)) {
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }
        _SEH_VAR(VcbResourceAcquired) = TRUE;

        Ext2VerifyVcb(IrpContext, Vcb);

        if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            Status = STATUS_ACCESS_DENIED;
            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                Status = STATUS_VOLUME_DISMOUNTED;
            }
            _SEH_LEAVE;
        }

        if ( ((IrpSp->FileObject->FileName.Length == 0) &&
             (IrpSp->FileObject->RelatedFileObject == NULL)) || 
             (Xcb && Xcb->Identifier.Type == EXT2VCB)  ) {
            Status = Ext2CreateVolume(IrpContext, Vcb);
        } else {

            Status = Ext2CreateFile(IrpContext, Vcb, &PostIrp);
        }

    }
    _SEH_FINALLY(Ext2CreateFinal_PSEH)
    _SEH_END;

    return Status;
}


NTSTATUS
Ext2CreateInode(
    PEXT2_IRP_CONTEXT   IrpContext,
    PEXT2_VCB           Vcb,
    PEXT2_FCB           Parent,
    ULONG               Type,
    ULONG               FileAttr,
    PUNICODE_STRING     FileName)
{
    NTSTATUS    Status;
    ULONG       iGrp;
    ULONG       iNo;
    PEXT2_INODE Inode = NULL;

    LARGE_INTEGER   SysTime;

    Inode = Ext2AllocateInode(Vcb);
    if (Inode == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    iGrp = (Parent->Mcb->iNo - 1) / BLOCKS_PER_GROUP;

    DEBUG(DL_INF, ("Ext2CreateInode: %S in %S(Inode=%xh)\n",
                    FileName->Buffer, 
                    Parent->Mcb->ShortName.Buffer, 
                    Parent->Mcb->iNo ));

    Status = Ext2NewInode(IrpContext, Vcb, iGrp, Type, &iNo);

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    Status = Ext2AddEntry(
                    IrpContext,
                    Vcb,
                    Parent,
                    Type,
                    iNo,
                    FileName,
                    NULL
                    );

    if (!NT_SUCCESS(Status)) {
        DbgBreak();
        Ext2FreeInode(IrpContext, Vcb, iNo, Type);
        goto errorout;
    }

    KeQuerySystemTime(&SysTime);
    Inode->i_ctime = Inode->i_mtime = 
    Inode->i_atime = Ext2LinuxTime(SysTime);

    Inode->i_uid = Parent->Inode->i_uid;
    Inode->i_gid = Parent->Inode->i_gid;

    Inode->i_dir_acl = Parent->Inode->i_dir_acl;
    Inode->i_file_acl = Parent->Inode->i_file_acl;
    Inode->i_generation = Parent->Inode->i_generation;

    Inode->osd2 = Parent->Inode->osd2;

    Inode->i_mode = S_IPERMISSION_MASK &
                     Parent->Inode->i_mode;
    if (Type == EXT2_FT_DIR)  {
        Inode->i_mode |= S_IFDIR;
        Inode->i_links_count = 2;
    } else if (Type == EXT2_FT_REG_FILE) {
        Inode->i_mode &= S_IFATTR;
        Inode->i_mode |= S_IFREG;
        Inode->i_links_count = 1;
    } else {
        DbgBreak();
        Inode->i_links_count = 1;
    }

    Ext2SaveInode(IrpContext, Vcb, iNo, Inode);
    DEBUG(DL_INF, ( "Ext2CreateInode: New Inode = %xh (Type=%xh)\n", iNo, Type));
            
errorout:

    if (Inode) {
        Ext2DestroyInode(Vcb, Inode);
    }

    return Status;
}


NTSTATUS
Ext2SupersedeOrOverWriteFile(
        IN PEXT2_IRP_CONTEXT IrpContext,
        IN PFILE_OBJECT      FileObject,
        IN PEXT2_VCB         Vcb,
        IN PEXT2_FCB         Fcb,
        IN PLARGE_INTEGER    AllocationSize,
        IN ULONG             Disposition
    )
{
    LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   Size;
    
    KeQuerySystemTime(&CurrentTime);

    Size.QuadPart = 0;
    if (!MmCanFileBeTruncated(&(Fcb->SectionObject), &(Size))) {
        return STATUS_USER_MAPPED_FILE;
    }

    /* purge all file cache and shrink cache windows size */
    CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);
    Fcb->Header.AllocationSize.QuadPart = 
    Fcb->Header.FileSize.QuadPart =  
    Fcb->Header.ValidDataLength.QuadPart = 0;
    CcSetFileSizes(FileObject,
                   (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);

    Size.QuadPart = CEILING_ALIGNED(ULONGLONG, 
                        (ULONGLONG)AllocationSize->QuadPart,
                        (ULONGLONG)BLOCK_SIZE);

    if (Size.QuadPart > Fcb->Mcb->FileSize.QuadPart) {
        Ext2ExpandFile(IrpContext, Vcb, Fcb->Mcb, &Size);
    } else {
        Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &Size);
    }

    Fcb->Header.AllocationSize = Size;
    if (Fcb->Header.AllocationSize.QuadPart > 0) {
        SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
        Fcb->RealSize = Fcb->Header.AllocationSize;
        CcSetFileSizes(FileObject,
                   (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );
    }

    /* remove all extent mappings */
    DEBUG(DL_EXT, ("Ext2SuperSede ...: %wZ\n", &Fcb->Mcb->FullName));
    if (Ext2ListExtents(&Fcb->Mcb->Extents)) {
        DbgBreak();
    }

    Fcb->Mcb->FileSize.QuadPart = 0;
    Fcb->Inode->i_size = 0;
    if (S_ISREG(Fcb->Inode->i_mode)) {
       Fcb->Inode->i_size_high = 0;
    }

    if (Disposition == FILE_SUPERSEDE) {
        Fcb->Inode->i_ctime = Ext2LinuxTime(CurrentTime);
    }
    Fcb->Inode->i_atime =
    Fcb->Inode->i_mtime = Ext2LinuxTime(CurrentTime);
    Ext2SaveInode(IrpContext, Vcb, Fcb->Mcb->iNo, Fcb->Inode);

    return STATUS_SUCCESS;
}
