/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             create.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS *****************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

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
    BOOLEAN         bOemBuffer = FALSE;

    PEXT2_MCB       Target = NULL;

    USHORT          i;

    _SEH2_TRY {

        RtlZeroMemory(&UniName, sizeof(UNICODE_STRING));
        RtlZeroMemory(&OemName, sizeof(OEM_STRING));

        /* exit if we jump into a possible symlink forever loop */
        if ((Linkdep + 1) > EXT2_MAX_NESTED_LINKS ||
                IoGetRemainingStackSize() < 1024) {
            _SEH2_LEAVE;
        }

        /* read the symlink target path */
        if (Mcb->Inode.i_size < EXT2_LINKLEN_IN_INODE) {

            OemName.Buffer = (PUCHAR) (&Mcb->Inode.i_block[0]);
            OemName.Length = (USHORT)Mcb->Inode.i_size;
            OemName.MaximumLength = OemName.Length + 1;

        } else {

            OemName.Length = (USHORT)Mcb->Inode.i_size;
            OemName.MaximumLength = OemName.Length + 1;
            OemName.Buffer = Ext2AllocatePool(PagedPool,
                                              OemName.MaximumLength,
                                              'NL2E');
            if (OemName.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }
            bOemBuffer = TRUE;
            RtlZeroMemory(OemName.Buffer, OemName.MaximumLength);

            Status = Ext2ReadInode(
                         IrpContext,
                         Vcb,
                         Mcb,
                         (ULONGLONG)0,
                         OemName.Buffer,
                         (ULONG)(Mcb->Inode.i_size),
                         FALSE,
                         NULL);
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
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
            _SEH2_LEAVE;
        }

        UniName.MaximumLength += 2;
        UniName.Buffer = Ext2AllocatePool(PagedPool,
                                          UniName.MaximumLength,
                                          'NL2E');
        if (UniName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        RtlZeroMemory(UniName.Buffer, UniName.MaximumLength);
        Status = Ext2OEMToUnicode(Vcb, &UniName, &OemName);
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
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
        if (Target == NULL) {
            Status = STATUS_LINK_FAILED;
        }

        if (Target == NULL /* link target doesn't exist */      ||
                Target == Mcb  /* symlink points to itself */       ||
                IsMcbSpecialFile(Target) /* target not resolved*/   ||
                IsFileDeleted(Target)  /* target deleted */         ) {

            if (Target) {
                ASSERT(Target->Refercount > 0);
                Ext2DerefMcb(Target);
            }
            ClearLongFlag(Mcb->Flags, MCB_TYPE_SYMLINK);
            SetLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
            Mcb->FileAttr = FILE_ATTRIBUTE_NORMAL;
            Mcb->Target = NULL;

        } else if (IsMcbSymLink(Target)) {

            ASSERT(Target->Refercount > 0);
            ASSERT(Target->Target != NULL);
            Ext2ReferMcb(Target->Target);
            Mcb->Target = Target->Target;
            Ext2DerefMcb(Target);
            ASSERT(!IsMcbSymLink(Target->Target));
            SetLongFlag(Mcb->Flags, MCB_TYPE_SYMLINK);
            ClearLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
            ASSERT(Mcb->Target->Refercount > 0);
            Mcb->FileAttr = Target->FileAttr;

        } else {

            Mcb->Target = Target;
            SetLongFlag(Mcb->Flags, MCB_TYPE_SYMLINK);
            ClearLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
            ASSERT(Mcb->Target->Refercount > 0);
            Mcb->FileAttr = Target->FileAttr;
        }

    } _SEH2_FINALLY {

        if (bOemBuffer) {
            Ext2FreePool(OemName.Buffer, 'NL2E');
        }

        if (UniName.Buffer) {
            Ext2FreePool(UniName.Buffer, 'NL2E');
        }
    } _SEH2_END;

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
        L"swapfile.sys",
        L"hiberfil.sys",
        NULL
    };

    PWSTR SpecialDirList[] = {
        L"Recycled",
        L"RECYCLER",
        L"$RECYCLE.BIN",
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
    struct dentry  *de = NULL;

    USHORT          i = 0, End;
    ULONG           Inode;

    BOOLEAN         bParent = FALSE;
    BOOLEAN         bDirectory = FALSE;
    BOOLEAN         LockAcquired = FALSE;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);
        LockAcquired = TRUE;

        *Ext2Mcb = NULL;

        DEBUG(DL_RES, ("Ext2LookupFile: %wZ\n", FullName));

        /* check names and parameters */
        if (FullName->Buffer[0] == L'\\') {
            Parent = Vcb->McbTree;
        } else if (Parent) {
            bParent = TRUE;
        } else {
            Parent = Vcb->McbTree;
        }

        /* make sure the parent is NULL */
        if (!IsMcbDirectory(Parent)) {
            Status =  STATUS_NOT_A_DIRECTORY;
            _SEH2_LEAVE;
        }

        /* use symlink's target as parent directory */
        if (IsMcbSymLink(Parent)) {
            Parent = Parent->Target;
            ASSERT(!IsMcbSymLink(Parent));
            if (IsFileDeleted(Parent)) {
                Status =  STATUS_NOT_A_DIRECTORY;
                _SEH2_LEAVE;
            }
        }

        if (NULL == Parent) {
            Status =  STATUS_NOT_A_DIRECTORY;
            _SEH2_LEAVE;
        }

        /* default is the parent Mcb*/
        Ext2ReferMcb(Parent);
        Mcb = Parent;

        /* is empty file name or root node */
        End = FullName->Length/sizeof(WCHAR);
        if ( (End == 0) || (End == 1 &&
                            FullName->Buffer[0] == L'\\')) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        /* is a directory expected ? */
        if (FullName->Buffer[End - 1] == L'\\') {
            bDirectory = TRUE;
        }

        /* loop with every sub name */
        while (i < End) {

            USHORT Start = 0;

            /* zero the prefix '\' */
            while (i < End && FullName->Buffer[i] == L'\\') i++;
            Start = i;

            /* zero the suffix '\' */
            while (i < End && (FullName->Buffer[i] != L'\\')) i++;

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

                if (IsMcbSymLink(Parent)) {
                    if (IsFileDeleted(Parent->Target)) {
                        Status =  STATUS_NOT_A_DIRECTORY;
                        Ext2DerefMcb(Parent);
                        break;
                    } else {
                        Ext2ReferMcb(Parent->Target);
                        Ext2DerefMcb(Parent);
                        Parent = Parent->Target;
                    }
                }

                /* search cached Mcb nodes */
                Mcb = Ext2SearchMcbWithoutLock(Parent, &FileName);

                if (Mcb) {

                    /* derefer the parent Mcb */
                    Ext2DerefMcb(Parent);
                    Status = STATUS_SUCCESS;
                    Parent = Mcb;

                    if (IsMcbSymLink(Mcb) && IsFileDeleted(Mcb->Target) &&
                            (Mcb->Refercount == 1)) {

                        ASSERT(Mcb->Target);
                        ASSERT(Mcb->Target->Refercount > 0);
                        Ext2DerefMcb(Mcb->Target);
                        Mcb->Target = NULL;
                        ClearLongFlag(Mcb->Flags, MCB_TYPE_SYMLINK);
                        SetLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
                        Mcb->FileAttr = FILE_ATTRIBUTE_NORMAL;
                    }

                } else {

                    /* need create new Mcb node */

                    /* is a valid ext2 name */
                    if (!Ext2IsNameValid(&FileName)) {
                        Status = STATUS_OBJECT_NAME_INVALID;
                        Ext2DerefMcb(Parent);
                        break;
                    }

                    /* seach the disk */
                    de = NULL;
                    Status = Ext2ScanDir (
                                 IrpContext,
                                 Vcb,
                                 Parent,
                                 &FileName,
                                 &Inode,
                                 &de);

                    if (NT_SUCCESS(Status)) {

                        /* check it's real parent */
                        ASSERT (!IsMcbSymLink(Parent));

                        /* allocate Mcb ... */
                        Mcb = Ext2AllocateMcb(Vcb, &FileName, &Parent->FullName, 0);
                        if (!Mcb) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            Ext2DerefMcb(Parent);
                            break;
                        }
                        Mcb->de = de;
                        Mcb->de->d_inode = &Mcb->Inode;
                        Mcb->Inode.i_ino = Inode;
                        Mcb->Inode.i_sb = &Vcb->sb;
                        de = NULL;

                        /* load inode information */
                        if (!Ext2LoadInode(Vcb, &Mcb->Inode)) {
                            Status = STATUS_CANT_WAIT;
                            Ext2DerefMcb(Parent);
                            Ext2FreeMcb(Vcb, Mcb);
                            break;
                        }

                        /* set inode attribute */
                        if (!CanIWrite(Vcb) && Ext2IsOwnerReadOnly(Mcb->Inode.i_mode)) {
                            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_READONLY);
                        }

                        if (S_ISDIR(Mcb->Inode.i_mode)) {
                            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                        } else {
                            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_NORMAL);
                            if (!S_ISREG(Mcb->Inode.i_mode) &&
                                    !S_ISLNK(Mcb->Inode.i_mode)) {
                                SetLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
                            }
                        }

                        /* process special files under root directory */
                        if (IsMcbRoot(Parent)) {
                            /* set hidden and system attributes for
                               Recycled / RECYCLER / pagefile.sys */
                            BOOLEAN IsDirectory = IsMcbDirectory(Mcb);
                            if (Ext2IsSpecialSystemFile(&Mcb->ShortName, IsDirectory)) {
                                SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_HIDDEN);
                                SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_SYSTEM);
                            }
                        }

                        Mcb->CreationTime = Ext2NtTime(Mcb->Inode.i_ctime);
                        Mcb->LastAccessTime = Ext2NtTime(Mcb->Inode.i_atime);
                        Mcb->LastWriteTime = Ext2NtTime(Mcb->Inode.i_mtime);
                        Mcb->ChangeTime = Ext2NtTime(Mcb->Inode.i_mtime);

                        /* process symlink */
                        if (S_ISLNK(Mcb->Inode.i_mode)) {
                            Ext2FollowLink( IrpContext,
                                            Vcb,
                                            Parent,
                                            Mcb,
                                            Linkdep+1
                                          );
                        }

                        /* add reference ... */
                        Ext2ReferMcb(Mcb);

                        /* add Mcb to it's parent tree*/
                        Ext2InsertMcb(Vcb, Parent, Mcb);

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

    } _SEH2_FINALLY {

        if (de) {
            Ext2FreeEntry(de);
        }

        if (NT_SUCCESS(Status)) {
            if (bDirectory) {
                if (IsMcbDirectory(Mcb)) {
                    *Ext2Mcb = Mcb;
                } else {
                    Ext2DerefMcb(Mcb);
                    Status = STATUS_NOT_A_DIRECTORY;
                }
            } else {
                *Ext2Mcb = Mcb;
            }
        }

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2ScanDir (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Parent,
    IN PUNICODE_STRING      FileName,
    OUT PULONG              Inode,
    OUT struct dentry     **dentry
)
{
    struct ext3_dir_entry_2 *dir_entry = NULL;
    struct buffer_head     *bh = NULL;
    struct dentry          *de = NULL;

    NTSTATUS                Status = STATUS_NO_SUCH_FILE;

    DEBUG(DL_RES, ("Ext2ScanDir: %wZ\\%wZ\n", &Parent->FullName, FileName));

    _SEH2_TRY {

        /* grab parent's reference first */
        Ext2ReferMcb(Parent);

        /* bad request ! Can a man be pregnant ? Maybe:) */
        if (!IsMcbDirectory(Parent)) {
            Status = STATUS_NOT_A_DIRECTORY;
            _SEH2_LEAVE;
        }

        /* parent is a symlink ? */
        if IsMcbSymLink(Parent) {
            if (Parent->Target) {
                Ext2ReferMcb(Parent->Target);
                Ext2DerefMcb(Parent);
                Parent = Parent->Target;
                ASSERT(!IsMcbSymLink(Parent));
            } else {
                DbgBreak();
                Status = STATUS_NOT_A_DIRECTORY;
                _SEH2_LEAVE;
            }
        }

        de = Ext2BuildEntry(Vcb, Parent, FileName);
        if (!de) {
            DEBUG(DL_ERR, ( "Ex2ScanDir: failed to allocate dentry.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        bh = ext3_find_entry(IrpContext, de, &dir_entry);
        if (dir_entry) {
            Status = STATUS_SUCCESS;
            *Inode = dir_entry->inode;
            *dentry = de;
        }

    } _SEH2_FINALLY {

        Ext2DerefMcb(Parent);

        if (bh)
            brelse(bh);

        if (!NT_SUCCESS(Status)) {
            if (de)
                Ext2FreeEntry(de);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS Ext2AddDotEntries(struct ext2_icb *icb, struct inode *dir,
                           struct inode *inode)
{
    struct ext3_dir_entry_2 * de;
    struct buffer_head * bh;
    ext3_lblk_t block = 0;
    int rc = 0;

    bh = ext3_append(icb, inode, &block, &rc);
    if (!bh) {
        goto errorout;
    }

    de = (struct ext3_dir_entry_2 *) bh->b_data;
    de->inode = cpu_to_le32(inode->i_ino);
    de->name_len = 1;
    de->rec_len = cpu_to_le16(EXT3_DIR_REC_LEN(de->name_len));
    strcpy (de->name, ".");
    ext3_set_de_type(inode->i_sb, de, S_IFDIR);
    de = (struct ext3_dir_entry_2 *)
         ((char *) de + le16_to_cpu(de->rec_len));
    de->inode = cpu_to_le32(dir->i_ino);
    de->rec_len = cpu_to_le16(inode->i_sb->s_blocksize-EXT3_DIR_REC_LEN(1));
    de->name_len = 2;
    strcpy (de->name, "..");
    ext3_set_de_type(inode->i_sb, de, S_IFDIR);
    inode->i_nlink = 2;
    set_buffer_dirty(bh);
    ext3_mark_inode_dirty(icb, inode);

errorout:
    if (bh)
        brelse (bh);

    return Ext2WinntError(rc);
}

NTSTATUS
Ext2CreateFile(
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

    BOOLEAN             bParentFcbCreated = FALSE;

#ifndef __REACTOS__
    BOOLEAN             bDir = FALSE;
#endif
    BOOLEAN             bFcbAllocated = FALSE;
    BOOLEAN             bCreated = FALSE;
    BOOLEAN             bMainResourceAcquired = FALSE;

    BOOLEAN             OpenDirectory;
    BOOLEAN             OpenTargetDirectory;
    BOOLEAN             CreateDirectory;
    BOOLEAN             SequentialOnly;
    BOOLEAN             NoIntermediateBuffering;
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
    NoIntermediateBuffering = IsFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
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

    _SEH2_TRY {

        FileName.MaximumLength = IrpSp->FileObject->FileName.MaximumLength;
        FileName.Length = IrpSp->FileObject->FileName.Length;

        if (IrpSp->FileObject->RelatedFileObject) {
            ParentFcb = (PEXT2_FCB)(IrpSp->FileObject->RelatedFileObject->FsContext);
        }

        if (ParentFcb) {
            ParentMcb = ParentFcb->Mcb;
            SetLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);
            Ext2ReferMcb(ParentMcb);
        }

        if (FileName.Length == 0) {

            if (ParentFcb) {
                Mcb = ParentFcb->Mcb;
                Ext2ReferMcb(Mcb);
                Status = STATUS_SUCCESS;
                goto McbExisting;
            } else {
                DbgBreak();
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }
        }

        FileName.Buffer = Ext2AllocatePool(
                              PagedPool,
                              FileName.MaximumLength,
                              EXT2_FNAME_MAGIC
                          );

        if (!FileName.Buffer) {
            DEBUG(DL_ERR, ( "Ex2CreateFile: failed to allocate FileName.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        INC_MEM_COUNT(PS_FILE_NAME, FileName.Buffer, FileName.MaximumLength);

        RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);
        RtlCopyMemory(FileName.Buffer, IrpSp->FileObject->FileName.Buffer, FileName.Length);

        if (ParentFcb && FileName.Buffer[0] == L'\\') {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

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

                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH2_LEAVE;
            }
        }

        if (IsFlagOn(Options, FILE_OPEN_BY_FILE_ID)) {
            Status = STATUS_NOT_IMPLEMENTED;
            _SEH2_LEAVE;
        }

        DEBUG(DL_INF, ( "Ext2CreateFile: %wZ Paging=%d Option: %xh:"
                        "Dir=%d NonDir=%d OpenTarget=%d NC=%d DeleteOnClose=%d\n",
                        &FileName, IsPagingFile, IrpSp->Parameters.Create.Options,
                        DirectoryFile, NonDirectoryFile, OpenTargetDirectory,
                        NoIntermediateBuffering, DeleteOnClose ));

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

#ifndef __REACTOS__
            LONG            i = 0;
#endif
            PathName = FileName;
            Mcb = NULL;

            if (PathName.Buffer[PathName.Length/2 - 1] == L'\\') {
                if (DirectoryFile) {
                    PathName.Length -=2;
                    PathName.Buffer[PathName.Length/2] = 0;
                } else {
                    DirectoryFile = TRUE;
                }
            }

            if (!ParentMcb) {
                if (PathName.Buffer[0] != L'\\') {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                } else {
                    ParentMcb = Vcb->McbTree;
                    Ext2ReferMcb(ParentMcb);
                }
            }

Dissecting:

            FsRtlDissectName(PathName, &RealName, &RemainName);

            if (((RemainName.Length != 0) && (RemainName.Buffer[0] == L'\\')) ||
                    (RealName.Length >= 256 * sizeof(WCHAR))) {
                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH2_LEAVE;
            }

            if (RemainName.Length != 0) {

                PEXT2_MCB   RetMcb = NULL;

                DEBUG(DL_RES, ("Ext2CreateFile: Lookup 2nd: %wZ\\%wZ\n",
                               &ParentMcb->FullName, &RealName));

                Status = Ext2LookupFile (
                             IrpContext,
                             Vcb,
                             &RealName,
                             ParentMcb,
                             &RetMcb,
                             0);

                /* quit name resolving loop */
                if (!NT_SUCCESS(Status)) {
                    if (Status == STATUS_NO_SUCH_FILE && RemainName.Length != 0) {
                        Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    }
                    _SEH2_LEAVE;
                }

                /* deref ParentMcb */
                Ext2DerefMcb(ParentMcb);

                /* RetMcb is already refered */
                ParentMcb = RetMcb;
                PathName  = RemainName;

                /* symlink must use it's target */
                if (IsMcbSymLink(ParentMcb)) {
                    Ext2ReferMcb(ParentMcb->Target);
                    Ext2DerefMcb(ParentMcb);
                    ParentMcb = ParentMcb->Target;
                    ASSERT(!IsMcbSymLink(ParentMcb));
                }

                goto Dissecting;
            }

            /* is name valid */
            if ( FsRtlDoesNameContainWildCards(&RealName) ||
                    !Ext2IsNameValid(&RealName)) {
                Status = STATUS_OBJECT_NAME_INVALID;
                _SEH2_LEAVE;
            }

            /* clear BUSY bit from original ParentFcb */
            if (ParentFcb) {
                ClearLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);
            }

            /* get the ParentFcb, allocate it if needed ... */
            ParentFcb = ParentMcb->Fcb;
            if (!ParentFcb) {
                ParentFcb = Ext2AllocateFcb(Vcb, ParentMcb);
                if (!ParentFcb) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH2_LEAVE;
                }
                bParentFcbCreated = TRUE;
                Ext2ReferXcb(&ParentFcb->ReferenceCount);
            }
            SetLongFlag(ParentFcb->Flags, FCB_STATE_BUSY);

            // We need to create a new one ?
            if ((CreateDisposition == FILE_CREATE ) ||
                    (CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OPEN_IF) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) {

                if (IsVcbReadOnly(Vcb)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
                }

                if (!CanIWrite(Vcb) && Ext2IsOwnerReadOnly(ParentFcb->Mcb->Inode.i_mode)) {
                    Status = STATUS_ACCESS_DENIED;
                    _SEH2_LEAVE;
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
                        _SEH2_LEAVE;
                    }
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
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
                    _SEH2_LEAVE;
                }

                bCreated = TRUE;
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

                if (IsVcbReadOnly(Vcb)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    _SEH2_LEAVE;
                }

                if (!ParentFcb) {
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    _SEH2_LEAVE;
                }

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = RealName.Length;

                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer,
                               RealName.Buffer,
                               RealName.Length );

                Fcb = ParentFcb;
                Mcb = Fcb->Mcb;
                Ext2ReferMcb(Mcb);

                Irp->IoStatus.Information = FILE_DOES_NOT_EXIST;
                Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                _SEH2_LEAVE;
            }

        } else { // File / Dir already exists.

            /* here already get Mcb referred */
            if (OpenTargetDirectory) {

                UNICODE_STRING  RealName = FileName;
                USHORT          i = 0;

                while (RealName.Buffer[RealName.Length/2 - 1] == L'\\') {
                    RealName.Length -= sizeof(WCHAR);
                    RealName.Buffer[RealName.Length/2] = 0;
                }
                i = RealName.Length/2;
                while (i > 0 && RealName.Buffer[i - 1] != L'\\')
                    i--;

                if (IsVcbReadOnly(Vcb)) {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    Ext2DerefMcb(Mcb);
                    _SEH2_LEAVE;
                }

                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_SUCCESS;

                RtlZeroMemory( IrpSp->FileObject->FileName.Buffer,
                               IrpSp->FileObject->FileName.MaximumLength);
                IrpSp->FileObject->FileName.Length = RealName.Length - i * sizeof(WCHAR);
                RtlCopyMemory( IrpSp->FileObject->FileName.Buffer, &RealName.Buffer[i],
                               IrpSp->FileObject->FileName.Length );

                // use's it's parent since it's open-target operation
                Ext2ReferMcb(Mcb->Parent);
                Ext2DerefMcb(Mcb);
                Mcb = Mcb->Parent;

                goto Openit;
            }

            // We can not create if one exists
            if (CreateDisposition == FILE_CREATE) {
                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_OBJECT_NAME_COLLISION;
                Ext2DerefMcb(Mcb);
                _SEH2_LEAVE;
            }

            /* directory forbits us to do the followings ... */
            if (IsMcbDirectory(Mcb)) {

                if ((CreateDisposition != FILE_OPEN) &&
                        (CreateDisposition != FILE_OPEN_IF)) {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    Ext2DerefMcb(Mcb);
                    _SEH2_LEAVE;
                }

                if (NonDirectoryFile) {
                    Status = STATUS_FILE_IS_A_DIRECTORY;
                    Ext2DerefMcb(Mcb);
                    _SEH2_LEAVE;
                }

                if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {

                    if (OpenTargetDirectory) {
                        DbgBreak();
                        Status = STATUS_INVALID_PARAMETER;
                        Ext2DerefMcb(Mcb);
                        _SEH2_LEAVE;
                    }
                }

            } else {

                if (DirectoryFile) {
                    Status = STATUS_NOT_A_DIRECTORY;;
                    Ext2DerefMcb(Mcb);
                    _SEH2_LEAVE;
                }
            }

            Irp->IoStatus.Information = FILE_OPENED;
        }

Openit:
        /* Mcb should already be referred and symlink is too */
        if (Mcb) {

            ASSERT(Mcb->Refercount > 0);

            /* refer it's target if it's a symlink, so both refered */
            if (IsMcbSymLink(Mcb)) {
                if (IsFileDeleted(Mcb->Target)) {
                    DbgBreak();
                    SetLongFlag(Mcb->Flags, MCB_TYPE_SPECIAL);
                    ClearLongFlag(Mcb->Flags, MCB_TYPE_SYMLINK);
                    Ext2DerefMcb(Mcb->Target);
                    Mcb->Target = NULL;
                } else {
                    SymLink = Mcb;
                    Mcb = Mcb->Target;
                    Ext2ReferMcb(Mcb);
                    ASSERT (!IsMcbSymLink(Mcb));
                }
            }

            // Check readonly flag
            if (!CanIWrite(Vcb) && Ext2IsOwnerReadOnly(Mcb->Inode.i_mode)) {
                if (BooleanFlagOn(DesiredAccess,  FILE_WRITE_DATA | FILE_APPEND_DATA |
                                  FILE_ADD_SUBDIRECTORY | FILE_DELETE_CHILD)) {
                    Status = STATUS_ACCESS_DENIED;
                    _SEH2_LEAVE;
                } else if (IsFlagOn(Options, FILE_DELETE_ON_CLOSE )) {
                    Status = STATUS_CANNOT_DELETE;
                    _SEH2_LEAVE;
                }
            }

            Fcb = Mcb->Fcb;
            if (Fcb == NULL) {

                /* allocate Fcb for this file */
                Fcb = Ext2AllocateFcb (Vcb, Mcb);
                if (Fcb) {
                    bFcbAllocated = TRUE;
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                if (IsPagingFile) {
                    Status = STATUS_SHARING_VIOLATION;
                    Fcb = NULL;
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
            bMainResourceAcquired = TRUE;

            /* Open target directory ? */
            if (NULL == Mcb) {
                DbgBreak();
                Mcb = Fcb->Mcb;
            }

            /* check Mcb reference */
            ASSERT(Fcb->Mcb->Refercount > 0);

            /* file delted ? */
            if (IsFlagOn(Fcb->Mcb->Flags, MCB_FILE_DELETED)) {
                Status = STATUS_FILE_DELETED;
                _SEH2_LEAVE;
            }

            if (DeleteOnClose && NULL == SymLink) {
                Status = Ext2IsFileRemovable(IrpContext, Vcb, Fcb, Ccb);
                if (!NT_SUCCESS(Status)) {
                    _SEH2_LEAVE;
                }
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
                        _SEH2_LEAVE;
                    }
                }
            }

            if (bCreated) {

                //
                //  This file is just created.
                //

                if (DirectoryFile) {

                    Status = Ext2AddDotEntries(IrpContext, &ParentMcb->Inode, &Mcb->Inode);
                    if (!NT_SUCCESS(Status)) {
                        Ext2DeleteFile(IrpContext, Vcb, Fcb, Mcb);
                        _SEH2_LEAVE;
                    }

                } else {

                    if ((LONGLONG)ext3_free_blocks_count(SUPER_BLOCK) <=
                            Ext2TotalBlocks(Vcb, &Irp->Overlay.AllocationSize, NULL)) {
                        DbgBreak();
                        Status = STATUS_DISK_FULL;
                        _SEH2_LEAVE;
                    }

                    /* disable data blocks allocation */
#if 0
                    Fcb->Header.AllocationSize.QuadPart =
                        Irp->Overlay.AllocationSize.QuadPart;

                    if (Fcb->Header.AllocationSize.QuadPart > 0) {
                        Status = Ext2ExpandFile(IrpContext,
                                                Vcb,
                                                Fcb->Mcb,
                                                &(Fcb->Header.AllocationSize)
                                               );
                        SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                        if (!NT_SUCCESS(Status)) {
                            Fcb->Header.AllocationSize.QuadPart = 0;
                            Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb,
                                             &Fcb->Header.AllocationSize);
                            _SEH2_LEAVE;
                        }
                    }
#endif
                }

            } else {

                //
                //  This file alreayd exists.
                //

                if (DeleteOnClose) {

                    if (IsVcbReadOnly(Vcb)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH2_LEAVE;
                    }

                    if (IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;

                        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                                      Vcb->Vpb->RealDevice );

                        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

                        Ext2RaiseStatus(IrpContext, STATUS_MEDIA_WRITE_PROTECTED);
                    }

                } else {

                    //
                    // Just to Open file (Open/OverWrite ...)
                    //

                    if ((!IsDirectory(Fcb)) && (IsFlagOn(IrpSp->FileObject->Flags,
                                                         FO_NO_INTERMEDIATE_BUFFERING))) {
                        Fcb->Header.IsFastIoPossible = FastIoIsPossible;

                        if (Fcb->SectionObject.DataSectionObject != NULL) {

                            if (Fcb->NonCachedOpenCount == Fcb->OpenHandleCount) {

                                if (!IsVcbReadOnly(Vcb)) {
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

                if (!IsVcbReadOnly(Vcb)) {
                    if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile) {
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
                        _SEH2_LEAVE;
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
                    _SEH2_LEAVE;
                }

            } else {

                /* set share access rights */
                IoSetShareAccess( DesiredAccess,
                                  ShareAccess,
                                  IrpSp->FileObject,
                                  &(Fcb->ShareAccess) );
            }

            Ccb = Ext2AllocateCcb(SymLink);
            if (!Ccb) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DbgBreak();
                _SEH2_LEAVE;
            }

            if (DeleteOnClose)
                SetLongFlag(Ccb->Flags, CCB_DELETE_ON_CLOSE);

            if (SymLink)
                Ccb->filp.f_dentry = SymLink->de;
            else
                Ccb->filp.f_dentry = Fcb->Mcb->de;

            Ccb->filp.f_version = Fcb->Mcb->Inode.i_version;
            Ext2ReferXcb(&Fcb->OpenHandleCount);
            Ext2ReferXcb(&Fcb->ReferenceCount);

            if (!IsDirectory(Fcb)) {
                if (NoIntermediateBuffering) {
                    Fcb->NonCachedOpenCount++;
                    SetFlag(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED);
                } else {
                    SetFlag(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED);
                }
            }

            Ext2ReferXcb(&Vcb->OpenHandleCount);
            Ext2ReferXcb(&Vcb->ReferenceCount);

            IrpSp->FileObject->FsContext = (void*) Fcb;
            IrpSp->FileObject->FsContext2 = (void*) Ccb;
            IrpSp->FileObject->PrivateCacheMap = NULL;
            IrpSp->FileObject->SectionObjectPointer = &(Fcb->SectionObject);

            DEBUG(DL_INF, ( "Ext2CreateFile: %wZ OpenCount=%u ReferCount=%u NonCachedCount=%u\n",
                            &Fcb->Mcb->FullName, Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount));

            Status = STATUS_SUCCESS;

            if (bCreated) {

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
                        _SEH2_LEAVE;
                    }
                }

                if ((CreateDisposition == FILE_SUPERSEDE) ||
                        (CreateDisposition == FILE_OVERWRITE) ||
                        (CreateDisposition == FILE_OVERWRITE_IF)) {

                    if (IsDirectory(Fcb)) {
                        Status = STATUS_FILE_IS_A_DIRECTORY;
                        _SEH2_LEAVE;
                    }

                    if (SymLink != NULL) {
                        DbgBreak();
                        Status = STATUS_INVALID_PARAMETER;
                        _SEH2_LEAVE;
                    }

                    if (IsVcbReadOnly(Vcb)) {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        _SEH2_LEAVE;
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
                        _SEH2_LEAVE;
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
            _SEH2_LEAVE;
        }

    } _SEH2_FINALLY {


        if (ParentMcb) {
            Ext2DerefMcb(ParentMcb);
        }

        /* cleanup Fcb and Ccb, Mcb if necessary */
        if (!NT_SUCCESS(Status)) {

            if (Ccb != NULL) {

                DbgBreak();

                ASSERT(Fcb != NULL);
                ASSERT(Fcb->Mcb != NULL);

                DEBUG(DL_ERR, ("Ext2CreateFile: failed to create %wZ status = %xh\n",
                               &Fcb->Mcb->FullName, Status));

                Ext2DerefXcb(&Fcb->OpenHandleCount);
                Ext2DerefXcb(&Fcb->ReferenceCount);

                if (!IsDirectory(Fcb)) {
                    if (NoIntermediateBuffering) {
                        Fcb->NonCachedOpenCount--;
                    } else {
                        ClearFlag(IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED);
                    }
                }

                Ext2DerefXcb(&Vcb->OpenHandleCount);
                Ext2DerefXcb(&Vcb->ReferenceCount);

                IoRemoveShareAccess(IrpSp->FileObject, &Fcb->ShareAccess);

                IrpSp->FileObject->FsContext = NULL;
                IrpSp->FileObject->FsContext2 = NULL;
                IrpSp->FileObject->PrivateCacheMap = NULL;
                IrpSp->FileObject->SectionObjectPointer = NULL;

                Ext2FreeCcb(Vcb, Ccb);
            }
        }

        if (Fcb && Ext2DerefXcb(&Fcb->ReferenceCount) == 0) {

            if (IsFlagOn(Fcb->Flags, FCB_ALLOC_IN_CREATE)) {

                LARGE_INTEGER Size;
                ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);
                _SEH2_TRY {
                    Size.QuadPart = 0;
                    Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &Size);
                } _SEH2_FINALLY {
                    ExReleaseResourceLite(&Fcb->PagingIoResource);
                } _SEH2_END;
            }

            if (bCreated) {
                Ext2DeleteFile(IrpContext, Vcb, Fcb, Mcb);
            }

            Ext2FreeFcb(Fcb);
            Fcb = NULL;
            bMainResourceAcquired = FALSE;
        }

        if (bMainResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        /* free file name buffer */
        if (FileName.Buffer) {
            DEC_MEM_COUNT(PS_FILE_NAME, FileName.Buffer, FileName.MaximumLength);
            Ext2FreePool(FileName.Buffer, EXT2_FNAME_MAGIC);
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
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2CreateVolume(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;
    PEXT2_CCB           Ccb;

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

    if ( !FlagOn(ShareAccess, FILE_SHARE_READ) &&
            Vcb->OpenVolumeCount  != 0 ) {
        return STATUS_SHARING_VIOLATION;
    }

    Ccb = Ext2AllocateCcb(NULL);
    if (Ccb == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    Status = STATUS_SUCCESS;

    if (Vcb->OpenVolumeCount > 0) {
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


    if (Vcb->OpenVolumeCount == 0 &&
        !IsFlagOn(ShareAccess, FILE_SHARE_READ)  &&
        !IsFlagOn(ShareAccess, FILE_SHARE_WRITE) ){

        if (!IsVcbReadOnly(Vcb)) {
            Ext2FlushFiles(IrpContext, Vcb, FALSE);
            Ext2FlushVolume(IrpContext, Vcb, FALSE);
        }

        SetLongFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        Vcb->LockFile = IrpSp->FileObject;
    } else {
        if (FlagOn(DesiredAccess, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA) ) {
            if (!IsVcbReadOnly(Vcb)) {
                Ext2FlushFiles(IrpContext, Vcb, FALSE);
                Ext2FlushVolume(IrpContext, Vcb, FALSE);
            }
        }
    }

    IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
    IrpSp->FileObject->FsContext  = Vcb;
    IrpSp->FileObject->FsContext2 = Ccb;
    IrpSp->FileObject->Vpb = Vcb->Vpb;

    Ext2ReferXcb(&Vcb->ReferenceCount);
    Ext2ReferXcb(&Vcb->OpenHandleCount);
    Ext2ReferXcb(&Vcb->OpenVolumeCount);

    Irp->IoStatus.Information = FILE_OPENED;

errorout:

    return Status;
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
    BOOLEAN             VcbResourceAcquired = FALSE;

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

    _SEH2_TRY {

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb->Identifier.Type == EXT2VCB);
        IrpSp->FileObject->Vpb = Vcb->Vpb;

        if (!IsMounted(Vcb)) {
            DbgBreak();
            if (IsFlagOn(Vcb->Flags, VCB_DEVICE_REMOVED)) {
                Status = STATUS_NO_SUCH_DEVICE;
            } else {
                Status = STATUS_VOLUME_DISMOUNTED;
            }
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource, TRUE)) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        VcbResourceAcquired = TRUE;

        Ext2VerifyVcb(IrpContext, Vcb);

        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            Status = STATUS_ACCESS_DENIED;
            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                Status = STATUS_VOLUME_DISMOUNTED;
            }
            _SEH2_LEAVE;
        }

        if ( ((IrpSp->FileObject->FileName.Length == 0) &&
                (IrpSp->FileObject->RelatedFileObject == NULL)) ||
                (Xcb && Xcb->Identifier.Type == EXT2VCB)  ) {
            Status = Ext2CreateVolume(IrpContext, Vcb);
        } else {

            Status = Ext2CreateFile(IrpContext, Vcb, &PostIrp);
        }

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress && !PostIrp)  {
            if ( Status == STATUS_PENDING ||
                    Status == STATUS_CANT_WAIT) {
                Status = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

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
    struct inode Inode = { 0 };
    struct dentry *Dentry = NULL;

    LARGE_INTEGER   SysTime;

    iGrp = (Parent->Inode->i_ino - 1) / BLOCKS_PER_GROUP;

    DEBUG(DL_INF, ("Ext2CreateInode: %S in %S(Inode=%xh)\n",
                   FileName->Buffer,
                   Parent->Mcb->ShortName.Buffer,
                   Parent->Inode->i_ino));

    Status = Ext2NewInode(IrpContext, Vcb, iGrp, Type, &iNo);
    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    KeQuerySystemTime(&SysTime);
    Ext2ClearInode(IrpContext, Vcb, iNo);
    Inode.i_sb = &Vcb->sb;
    Inode.i_ino = iNo;
    Inode.i_ctime = Inode.i_mtime =
                        Inode.i_atime = Ext2LinuxTime(SysTime);
    Inode.i_uid = Parent->Inode->i_uid;
    Inode.i_gid = Parent->Inode->i_gid;
    Inode.i_generation = Parent->Inode->i_generation;
    Inode.i_mode = S_IPERMISSION_MASK &
                   Parent->Inode->i_mode;
    if (Type == EXT2_FT_DIR)  {
        Inode.i_mode |= S_IFDIR;
    } else if (Type == EXT2_FT_REG_FILE) {
        Inode.i_mode &= S_IFATTR;
        Inode.i_mode |= S_IFREG;
    } else {
        DbgBreak();
    }

    /* Force using extent */
    if (IsFlagOn(SUPER_BLOCK->s_feature_incompat, EXT4_FEATURE_INCOMPAT_EXTENTS)) {
        Inode.i_flags |= EXT2_EXTENTS_FL;
    }

    /* add new entry to its parent */
    Status = Ext2AddEntry(
                 IrpContext,
                 Vcb,
                 Parent,
                 &Inode,
                 FileName,
                 &Dentry
             );

    if (!NT_SUCCESS(Status)) {
        DbgBreak();
        Ext2FreeInode(IrpContext, Vcb, iNo, Type);
        goto errorout;
    }

    DEBUG(DL_INF, ("Ext2CreateInode: New Inode = %xh (Type=%xh)\n",
                   Inode.i_ino, Type));

errorout:

    if (Dentry)
        Ext2FreeEntry(Dentry);

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

    if ((loff_t)Size.QuadPart > Fcb->Inode->i_size) {
        Ext2ExpandFile(IrpContext, Vcb, Fcb->Mcb, &Size);
    } else {
        Ext2TruncateFile(IrpContext, Vcb, Fcb->Mcb, &Size);
    }

    Fcb->Header.AllocationSize = Size;
    if (Fcb->Header.AllocationSize.QuadPart > 0) {
        SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
        CcSetFileSizes(FileObject,
                       (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );
    }

    /* remove all extent mappings */
    DEBUG(DL_EXT, ("Ext2SuperSede ...: %wZ\n", &Fcb->Mcb->FullName));
    Fcb->Inode->i_size = 0;

    if (Disposition == FILE_SUPERSEDE) {
        Fcb->Inode->i_ctime = Ext2LinuxTime(CurrentTime);
    }
    Fcb->Inode->i_atime =
        Fcb->Inode->i_mtime = Ext2LinuxTime(CurrentTime);
    Ext2SaveInode(IrpContext, Vcb, Fcb->Inode);

    return STATUS_SUCCESS;
}
