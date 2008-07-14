/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             dirctl.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   14 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 *                     Fixed some warnings under GCC
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2QueryDirectoryFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PEXT2_FCB Fcb,
    IN ULONG Length,
    IN BOOLEAN FcbResourceAcquired,
    IN ULONG UsedLength,
    IN PUNICODE_STRING InodeFileName,
    IN PEXT2_DIR_ENTRY2 pDir    );

VOID
Ext2NotifyChangeDirectoryFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN BOOLEAN CompleteRequest,
    IN PNTSTATUS pStatus,
    IN PEXT2_FCB Fcb,
    IN BOOLEAN bFcbAcquired    );

VOID
Ext2IsDirectoryEmptyFinal (
    IN PEXT2_DIR_ENTRY2 pTarget    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2GetInfoLength)
#pragma alloc_text(PAGE, Ext2ProcessDirEntry)
#pragma alloc_text(PAGE, Ext2QueryDirectory)
#pragma alloc_text(PAGE, Ext2NotifyChangeDirectory)
#pragma alloc_text(PAGE, Ext2DirectoryControl)
#pragma alloc_text(PAGE, Ext2IsDirectoryEmpty)
#endif


/* FUNCTIONS ***************************************************************/


ULONG
Ext2GetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass)
{
    switch (FileInformationClass) {

    case FileDirectoryInformation:
        return FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName[0]);

    case FileFullDirectoryInformation:
        return FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName[0]);

    case FileBothDirectoryInformation:
        return FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[0]);

    case FileNamesInformation:
        return FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName[0]);

    case FileIdFullDirectoryInformation:
        return FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName[0]);

    case FileIdBothDirectoryInformation:
        return FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName[0]);

    default:
        break;
    }

    return 0;
}


NTSTATUS
Ext2ProcessDirEntry(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Dcb,
    IN FILE_INFORMATION_CLASS  FileInformationClass,
    IN ULONG                in,
    IN PVOID                Buffer,
    IN ULONG                UsedLength,
    IN ULONG                Length,
    IN ULONG                FileIndex,
    IN PUNICODE_STRING      pName,
    OUT PULONG              EntrySize,
    IN BOOLEAN              Single
    )
{
    PEXT2_MCB   Mcb = NULL;
    PEXT2_MCB   Target = NULL;

    NTSTATUS    Status = STATUS_SUCCESS;
    PEXT2_INODE Inode = NULL;

    ULONG InfoLength = 0;
    ULONG NameLength = 0;
    LONGLONG FileSize = 0;
    LONGLONG AllocationSize;
    ULONG   FileAttributes = 0;

    *EntrySize = 0;
    NameLength = pName->Length;

    InfoLength = Ext2GetInfoLength(FileInformationClass);
    if (InfoLength == 0) {
        DEBUG(DL_ERR, ("Ext2ProcessDirEntry: Invalid Info Class %xh for %wZ in %wZ\n",
                        FileInformationClass, pName, &Dcb->Mcb->FullName ));
        return STATUS_INVALID_INFO_CLASS;
    }

    if (InfoLength + NameLength > Length)  {
        DEBUG(DL_INF, ( "Ext2PricessDirEntry: Buffer is not enough.\n"));
        Status = STATUS_BUFFER_OVERFLOW;
        if (UsedLength || InfoLength > Length) {
            DEBUG(DL_CP, ("Ext2ProcessDirEntry: Buffer overflows for %wZ in %wZ\n",
                            pName, &Dcb->Mcb->FullName ));
            return Status;
        }
        RtlZeroMemory((PCHAR)Buffer + UsedLength, Length);
    } else {
        RtlZeroMemory((PCHAR)Buffer + UsedLength, InfoLength + NameLength);
    }

    DEBUG(DL_CP, ("Ext2ProcessDirEntry: %wZ in %wZ\n", pName, &Dcb->Mcb->FullName ));

    Mcb = Ext2SearchMcb(Vcb, Dcb->Mcb, pName);
    if (NULL == Mcb){

        Inode = Ext2AllocateInode(Vcb);
        if (Inode == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        if (!Ext2LoadInode(Vcb, in, Inode)) {
            DEBUG(DL_ERR, ("Ext2PricessDirEntry: Loading inode %xh (%wZ) error.\n",
                            in, pName ));
            DbgBreak();
            Status = STATUS_SUCCESS;
            goto errorout;
        }

        if (S_ISLNK(Inode->i_mode)) {
            DEBUG(DL_RES, ("Ext2ProcessDirEntry: SymLink: %wZ\\%wZ\n",
                    &Dcb->Mcb->FullName, pName));
            Ext2LookupFile(IrpContext, Vcb, pName, Dcb->Mcb, &Mcb, 0);
        }
    }

    if (Mcb != NULL)  {

        if (IsMcbSymlink(Mcb)) {
            Target = Mcb->Target;
            ASSERT(Target);
            if (IsMcbDirectory(Target)) {
                FileSize = 0;
            } else {
                FileSize = Target->FileSize.QuadPart;
            }
        } else {
            if (IsMcbDirectory(Mcb)) {
                FileSize = 0;
            } else {
                FileSize = Mcb->FileSize.QuadPart;
            }
        }

        FileAttributes = Mcb->FileAttr;

    } else {

        if (S_ISDIR(Inode->i_mode)) {
            FileSize = 0;
        } else {
            FileSize  = (LONGLONG) Inode->i_size;
            if (S_ISREG(Inode->i_mode)) {
                FileSize |= ((LONGLONG)(Inode->i_size_high) << 32);
            }
        }

        FileAttributes = FILE_ATTRIBUTE_NORMAL;
        if (Ext2IsReadOnly(Inode->i_mode)) {
            SetFlag(FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(Inode->i_mode))
                SetFlag(FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }

    AllocationSize = CEILING_ALIGNED(ULONGLONG, FileSize, BLOCK_SIZE);

    /* process special files under root directory */
    if (IsRoot(Dcb)) {
        /* set hidden and system attributes for Recycled /
           RECYCLER / pagefile.sys */
        BOOLEAN IsDirectory = IsFlagOn(FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
        if (Ext2IsSpecialSystemFile(pName, IsDirectory)) {
            SetFlag(FileAttributes, FILE_ATTRIBUTE_HIDDEN);
            SetFlag(FileAttributes, FILE_ATTRIBUTE_SYSTEM);
        }
    }

    /* set hidden attribute for all entries starting with '.' */
    if (( pName->Length >= 4 && pName->Buffer[0] == L'.') &&
        ((pName->Length == 4 && pName->Buffer[1] != L'.') ||
          pName->Length >= 6 )) {
        SetFlag(FileAttributes, FILE_ATTRIBUTE_HIDDEN);
    }

    switch(FileInformationClass) {

    case FileDirectoryInformation:

    {
        PFILE_DIRECTORY_INFORMATION     FDI;

        FDI = (PFILE_DIRECTORY_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FDI->NextEntryOffset = InfoLength + NameLength;
            FDI->NextEntryOffset = CEILING_ALIGNED(ULONG, FDI->NextEntryOffset, 8);
        }

        FDI->FileIndex = FileIndex;

        if (Mcb) {

            FDI->CreationTime = Mcb->CreationTime;
            FDI->LastAccessTime = Mcb->LastAccessTime;
            FDI->LastWriteTime = Mcb->LastWriteTime;
            FDI->ChangeTime = Mcb->ChangeTime;

        } else {

            FDI->CreationTime = Ext2NtTime(Inode->i_ctime);
            FDI->LastAccessTime = Ext2NtTime(Inode->i_atime);
            FDI->LastWriteTime = Ext2NtTime(Inode->i_mtime);
            FDI->ChangeTime = Ext2NtTime(Inode->i_mtime);
        }

        FDI->FileAttributes = FileAttributes;
        FDI->EndOfFile.QuadPart = FileSize;
        FDI->AllocationSize.QuadPart = AllocationSize;

        FDI->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FDI->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }
       
    case FileFullDirectoryInformation:

    {
        PFILE_FULL_DIR_INFORMATION      FFI;

        FFI = (PFILE_FULL_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FFI->NextEntryOffset = InfoLength + NameLength;
            FFI->NextEntryOffset = CEILING_ALIGNED(ULONG, FFI->NextEntryOffset, 8);
        }

        FFI->FileIndex = FileIndex;

        if (Mcb) {

            FFI->CreationTime = Mcb->CreationTime;
            FFI->LastAccessTime = Mcb->LastAccessTime;
            FFI->LastWriteTime = Mcb->LastWriteTime;
            FFI->ChangeTime = Mcb->ChangeTime;

        } else {

            FFI->CreationTime = Ext2NtTime(Inode->i_ctime);
            FFI->LastAccessTime = Ext2NtTime(Inode->i_atime);
            FFI->LastWriteTime = Ext2NtTime(Inode->i_mtime);
            FFI->ChangeTime = Ext2NtTime(Inode->i_mtime);
        }

        FFI->FileAttributes = FileAttributes;
        FFI->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FFI->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }
        
    case FileBothDirectoryInformation:

    {
        PFILE_BOTH_DIR_INFORMATION      FBI;

        FBI = (PFILE_BOTH_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FBI->NextEntryOffset = InfoLength + NameLength;
            FBI->NextEntryOffset = CEILING_ALIGNED(ULONG, FBI->NextEntryOffset, 8); 
        }

        FBI->FileIndex = FileIndex;
        FBI->EndOfFile.QuadPart = FileSize;
        FBI->AllocationSize.QuadPart = AllocationSize;

        if (Mcb) {

            FBI->CreationTime = Mcb->CreationTime;
            FBI->LastAccessTime = Mcb->LastAccessTime;
            FBI->LastWriteTime = Mcb->LastWriteTime;
            FBI->ChangeTime = Mcb->ChangeTime;

        } else {

            FBI->CreationTime = Ext2NtTime(Inode->i_ctime);
            FBI->LastAccessTime = Ext2NtTime(Inode->i_atime);
            FBI->LastWriteTime = Ext2NtTime(Inode->i_mtime);
            FBI->ChangeTime = Ext2NtTime(Inode->i_mtime);
        }

        FBI->FileAttributes = FileAttributes;

#ifdef DIR_ENTRY_SHORT_NAME
        if (NameLength <= 24) {
            RtlCopyMemory(FBI->ShortName, pName->Buffer, NameLength);
            FBI->ShortNameLength = (UCHAR)NameLength;
        }
#endif

        FBI->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FBI->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }
        
    case FileNamesInformation:

    {
        PFILE_NAMES_INFORMATION         FNI;

        FNI = (PFILE_NAMES_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FNI->NextEntryOffset = InfoLength + NameLength;
            FNI->NextEntryOffset = CEILING_ALIGNED(ULONG, FNI->NextEntryOffset, 8);
        }

        FNI->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FNI->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }

    case FileIdFullDirectoryInformation:

    {
        PFILE_ID_FULL_DIR_INFORMATION   FIF;

        FIF = (PFILE_ID_FULL_DIR_INFORMATION)((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FIF->NextEntryOffset = InfoLength + NameLength;
            FIF->NextEntryOffset = CEILING_ALIGNED(ULONG, FIF->NextEntryOffset, 8);
        }

        FIF->FileIndex = FileIndex;

        if (Mcb) {

            FIF->CreationTime = Mcb->CreationTime;
            FIF->LastAccessTime = Mcb->LastAccessTime;
            FIF->LastWriteTime = Mcb->LastWriteTime;
            FIF->ChangeTime = Mcb->ChangeTime;

        } else {

            FIF->CreationTime = Ext2NtTime(Inode->i_ctime);
            FIF->LastAccessTime = Ext2NtTime(Inode->i_atime);
            FIF->LastWriteTime = Ext2NtTime(Inode->i_mtime);
            FIF->ChangeTime = Ext2NtTime(Inode->i_mtime);
        }

        FIF->FileAttributes = FileAttributes;
        FIF->FileId.QuadPart = (LONGLONG)in;
        FIF->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FIF->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }

    case FileIdBothDirectoryInformation:

    {
        PFILE_ID_BOTH_DIR_INFORMATION   FIB;

        FIB = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)Buffer + UsedLength);
        if (!Single) {
            FIB->NextEntryOffset = InfoLength + NameLength;
            FIB->NextEntryOffset = CEILING_ALIGNED(ULONG, FIB->NextEntryOffset, 8);
        }

        FIB->FileIndex = FileIndex;
        FIB->EndOfFile.QuadPart = FileSize;
        FIB->AllocationSize.QuadPart = AllocationSize;

        if (Mcb) {

            FIB->CreationTime = Mcb->CreationTime;
            FIB->LastAccessTime = Mcb->LastAccessTime;
            FIB->LastWriteTime = Mcb->LastWriteTime;
            FIB->ChangeTime = Mcb->ChangeTime;

        } else {

            FIB->CreationTime = Ext2NtTime(Inode->i_ctime);
            FIB->LastAccessTime = Ext2NtTime(Inode->i_atime);
            FIB->LastWriteTime = Ext2NtTime(Inode->i_mtime);
            FIB->ChangeTime = Ext2NtTime(Inode->i_mtime);
        }

        FIB->FileAttributes = FileAttributes;
        FIB->FileId.QuadPart = (LONGLONG)in;

#ifdef DIR_ENTRY_SHORT_NAME
        if (NameLength <= 24) {
            RtlCopyMemory(FIB->ShortName, pName->Buffer, NameLength);
            FIB->ShortNameLength = (UCHAR)NameLength;
        }
#endif

        FIB->FileNameLength = NameLength;
        if (InfoLength + NameLength > Length) {
            NameLength = Length - InfoLength;
        }
        RtlCopyMemory(FIB->FileName, pName->Buffer, NameLength);

        *EntrySize = InfoLength + NameLength; 
        break;
    }

    default:
        Status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    if (Mcb) {
        Ext2DerefMcb(Mcb);
    }

errorout:

    if (Inode) {
        Ext2DestroyInode(Vcb, Inode);
    }

    DEBUG(DL_CP, ("Ext2ProcessDirEntry: Status = %xh for %wZ in %wZ\n",
                    Status, pName, &Dcb->Mcb->FullName ));

    return Status;
}


BOOLEAN
Ext2IsWearingCloak(
    IN  PEXT2_VCB       Vcb,
    IN  POEM_STRING     OemName
    )
{
    size_t  PatLen = 0;

    /* we could not filter the files: "." and ".." */
    if (OemName->Length >= 1 && OemName->Buffer[0] == '.') {

        if ( OemName->Length == 2 && OemName->Buffer[1] == '.') {
            return FALSE;
        } else if (OemName->Length == 1) {
            return FALSE;
        }
    }

    /* checking name prefix */
    if (Vcb->bHidingPrefix) {
        PatLen = strlen(&Vcb->sHidingPrefix[0]);
        if (PatLen <= OemName->Length) {
            if ( _strnicmp( OemName->Buffer, 
                            Vcb->sHidingPrefix,
                            PatLen ) == 0) {
                return TRUE;
            }
        }
    }

    /* checking name suffix */
    if (Vcb->bHidingSuffix) {
        PatLen = strlen(&Vcb->sHidingSuffix[0]);
        if (PatLen <= OemName->Length) {
            if ( _strnicmp(&OemName->Buffer[OemName->Length - PatLen], 
                            Vcb->sHidingSuffix, PatLen ) == 0) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

_SEH_DEFINE_LOCALS(Ext2QueryDirectoryFinal)
{
    PEXT2_IRP_CONTEXT    IrpContext;
    PNTSTATUS            pStatus;
    PEXT2_FCB            Fcb;
    ULONG                Length;
    BOOLEAN              FcbResourceAcquired;
    ULONG                UsedLength;
    PUNICODE_STRING      InodeFileName;
    PEXT2_DIR_ENTRY2     pDir;
};

_SEH_FINALLYFUNC(Ext2QueryDirectoryFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2QueryDirectoryFinal);
    Ext2QueryDirectoryFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus),
                            _SEH_VAR(Fcb), _SEH_VAR(Length),
                            _SEH_VAR(FcbResourceAcquired), 
                            _SEH_VAR(UsedLength), _SEH_VAR(InodeFileName),
                            _SEH_VAR(pDir));
}

VOID
Ext2QueryDirectoryFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PEXT2_FCB            Fcb,
    IN ULONG                Length,
    IN BOOLEAN              FcbResourceAcquired,
    IN ULONG                UsedLength,
    IN PUNICODE_STRING      InodeFileName,
    IN PEXT2_DIR_ENTRY2     pDir
    )
{
    if (FcbResourceAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }
        
    if (pDir != NULL) {
        ExFreePoolWithTag(pDir, EXT2_DENTRY_MAGIC);
        DEC_MEM_COUNT(PS_DIR_ENTRY, pDir, sizeof(EXT2_DIR_ENTRY2)); 
    }
        
    if (InodeFileName->Buffer != NULL) {
        DEC_MEM_COUNT(PS_INODE_NAME, InodeFileName->Buffer,
                      InodeFileName->MaximumLength );
        ExFreePoolWithTag(InodeFileName->Buffer, EXT2_INAME_MAGIC);
    }
        
    if (!IrpContext->ExceptionInProgress) {

        if ( *pStatus == STATUS_PENDING || 
              *pStatus == STATUS_CANT_WAIT) {

            *pStatus = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (NT_SUCCESS(*pStatus)) {
                *pStatus = Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, *pStatus);
            }
        } else {
            IrpContext->Irp->IoStatus.Information = UsedLength;
            Ext2CompleteIrpContext(IrpContext, *pStatus);
        }
    }
}

NTSTATUS
Ext2QueryDirectory (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PFILE_OBJECT            FileObject;
    PEXT2_FCB               Fcb;
    PEXT2_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;

    ULONG                   FileIndex;
    PUNICODE_STRING         FileName;
    PUCHAR                  Buffer;

    BOOLEAN                 RestartScan;
    BOOLEAN                 ReturnSingleEntry;
    BOOLEAN                 IndexSpecified;
    BOOLEAN                 FirstQuery;

    USHORT                  InodeFileNameLength;
    FILE_INFORMATION_CLASS  FileInformationClass;

    OEM_STRING              OemName;
    UNICODE_STRING          InodeFileName;
    PEXT2_DIR_ENTRY2        pDir = NULL;

    ULONG                   ByteOffset;
    ULONG                   RecLen = 0;
    ULONG                   PrevEntry = 0;
    ULONG                   EntrySize = 0;

    InodeFileName.Buffer = NULL;
    
    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2QueryDirectoryFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(FcbResourceAcquired) = FALSE;
        _SEH_VAR(UsedLength) = 0;
        _SEH_VAR(InodeFileName) = &InodeFileName;
        _SEH_VAR(pDir) = NULL;

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        _SEH_VAR(Fcb) = Fcb;
        
        if (Fcb == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }
        
        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        
        if (!IsDirectory(Fcb)) {
            Status = STATUS_NOT_A_DIRECTORY;
            _SEH_LEAVE;
        }
        
        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb);
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
#ifndef _GNU_NTIFS_
        
        FileInformationClass =
            IoStackLocation->Parameters.QueryDirectory.FileInformationClass;
        
        Length = IoStackLocation->Parameters.QueryDirectory.Length;
        
        FileName = (PUNICODE_STRING)IoStackLocation->Parameters.QueryDirectory.FileName;
        
        FileIndex = IoStackLocation->Parameters.QueryDirectory.FileIndex;
        
#else // _GNU_NTIFS_
        
        FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileInformationClass;
        
        _SEH_VAR(Length) = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.Length;
        
        FileName = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileName;
        
        FileIndex = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileIndex;
        
#endif // _GNU_NTIFS_

        RestartScan = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RESTART_SCAN);
        ReturnSingleEntry = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RETURN_SINGLE_ENTRY);
        IndexSpecified = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_INDEX_SPECIFIED);

        Buffer = Ext2GetUserBuffer(Irp);

        if (Buffer == NULL) {
            DbgBreak();
            Status = STATUS_INVALID_USER_BUFFER;
            _SEH_LEAVE;
        }
        
        if (!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }

        if (!ExAcquireResourceSharedLite(
                 &Fcb->MainResource,
                 IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }
        _SEH_VAR(FcbResourceAcquired) = TRUE;
        
        if (FileName != NULL) {

            if (Ccb->DirectorySearchPattern.Buffer != NULL) {

                FirstQuery = FALSE;

            } else {

                FirstQuery = TRUE;

                Ccb->DirectorySearchPattern.Length =
                    Ccb->DirectorySearchPattern.MaximumLength =
                    FileName->Length;
                
                Ccb->DirectorySearchPattern.Buffer =
                    ExAllocatePoolWithTag(PagedPool, FileName->Length,
                                          EXT2_DIRSP_MAGIC);
                
                if (Ccb->DirectorySearchPattern.Buffer == NULL) {
                    DEBUG(DL_ERR, ( "Ex2QueryDirectory: failed to allocate SerarchPattern.\n")); 
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }

                INC_MEM_COUNT( PS_DIR_PATTERN,
                               Ccb->DirectorySearchPattern.Buffer,
                               Ccb->DirectorySearchPattern.MaximumLength);

                Status = RtlUpcaseUnicodeString(
                    &(Ccb->DirectorySearchPattern),
                    FileName,
                    FALSE);

                if (!NT_SUCCESS(Status)) {
                    _SEH_LEAVE;
                }
            }

        } else if (Ccb->DirectorySearchPattern.Buffer != NULL) {

            FirstQuery = FALSE;
            FileName = &Ccb->DirectorySearchPattern;

        } else {

            FirstQuery = TRUE;
            
            Ccb->DirectorySearchPattern.Length =
                Ccb->DirectorySearchPattern.MaximumLength = 2;
            
            Ccb->DirectorySearchPattern.Buffer =
                ExAllocatePoolWithTag(PagedPool, 4, EXT2_DIRSP_MAGIC);
            
            if (Ccb->DirectorySearchPattern.Buffer == NULL) {
                DEBUG(DL_ERR, ( "Ex2QueryDirectory: failed to allocate SerarchPattern (1st).\n")); 
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH_LEAVE;
            }
            
            INC_MEM_COUNT( PS_DIR_PATTERN,
                           Ccb->DirectorySearchPattern.Buffer,
                           Ccb->DirectorySearchPattern.MaximumLength);

            RtlZeroMemory(Ccb->DirectorySearchPattern.Buffer, 4);
            RtlCopyMemory(
                Ccb->DirectorySearchPattern.Buffer,
                L"*\0", 2);
        }

        if (!IndexSpecified) {
            if (RestartScan || FirstQuery) {
                FileIndex = 0;
            } else {
                FileIndex = Ccb->CurrentByteOffset;
            }
        }
       
        RtlZeroMemory(Buffer, _SEH_VAR(Length));

        if (Fcb->Inode->i_size <= FileIndex) {
            Status = STATUS_NO_MORE_FILES;
            _SEH_LEAVE;
        }
        
        pDir = ExAllocatePoolWithTag(
                    PagedPool,
                    sizeof(EXT2_DIR_ENTRY2),
                    EXT2_DENTRY_MAGIC
                );
        _SEH_VAR(pDir) = pDir;

        if (!pDir) {
            DEBUG(DL_ERR, ( "Ex2QueryDirectory: failed to allocate pDir.\n")); 
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }

        INC_MEM_COUNT(PS_DIR_ENTRY, pDir, sizeof(EXT2_DIR_ENTRY2));
        
        ByteOffset = FileIndex;
        PrevEntry = 0;

        DEBUG(DL_CP, ("Ex2QueryDirectory: Dir: %wZ Index=%xh Pattern : %wZ.\n",
                       &Fcb->Mcb->FullName, FileIndex, &Ccb->DirectorySearchPattern)); 

        while ((ByteOffset < Fcb->Inode->i_size) &&
               (CEILING_ALIGNED(ULONG, _SEH_VAR(UsedLength), 8) < _SEH_VAR(Length))) {

            RtlZeroMemory(pDir, sizeof(EXT2_DIR_ENTRY2));

            Status = Ext2ReadInode(
                        IrpContext,
                        Vcb,
                        Fcb->Mcb,
                        (ULONGLONG)ByteOffset,
                        (PVOID)pDir,
                        sizeof(EXT2_DIR_ENTRY2),
                        FALSE,
                        &EntrySize);

            if (!NT_SUCCESS(Status)) {
                _SEH_LEAVE;
            }

            if (pDir->rec_len == 0) {
                RecLen = BLOCK_SIZE - (ByteOffset & (BLOCK_SIZE - 1));
            } else {
                RecLen = pDir->rec_len;
            }

            if (!pDir->inode || pDir->inode >= INODES_COUNT) {
                goto ProcessNextEntry;
            }

            OemName.Buffer = pDir->name;
            OemName.Length = (pDir->name_len & 0xff);
            OemName.MaximumLength = OemName.Length;

            if (Ext2IsWearingCloak(Vcb, &OemName)) {
                goto ProcessNextEntry;
            }

            InodeFileNameLength = (USHORT)
                Ext2OEMToUnicodeSize(Vcb, &OemName);
         
            if (InodeFileNameLength <= 0) {
                DEBUG(DL_CP, ("Ext2QueryDirectory: failed to count unicode length for inode: %xh\n",
                               pDir->inode));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if ( InodeFileName.Buffer != NULL &&
                 InodeFileName.MaximumLength > InodeFileNameLength) {
                /* reuse buffer */
            } else {
                /* free and re-allocate it */
                if (InodeFileName.Buffer) {
                    DEC_MEM_COUNT(PS_INODE_NAME,
                                  InodeFileName.Buffer,
                                  InodeFileName.MaximumLength);
                    ExFreePoolWithTag(InodeFileName.Buffer, EXT2_INAME_MAGIC);
                    InodeFileName.Buffer = NULL;
                }
                InodeFileName.MaximumLength = InodeFileNameLength + 2;
                InodeFileName.Buffer = ExAllocatePoolWithTag(
                        PagedPool,
                        InodeFileName.MaximumLength,
                        EXT2_INAME_MAGIC
                    );

                if (!InodeFileName.Buffer) {
                    DEBUG(DL_ERR, ( "Ex2QueryDirectory: failed to "
                                          "allocate InodeFileName.\n")); 
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }
                INC_MEM_COUNT(PS_INODE_NAME, InodeFileName.Buffer,
                              InodeFileName.MaximumLength);
            }

            InodeFileName.Length = 0;
            RtlZeroMemory(InodeFileName.Buffer, InodeFileName.MaximumLength);

            Status = Ext2OEMToUnicode(Vcb, &InodeFileName, &OemName);
            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_ERR, ( "Ex2QueryDirectory: Ext2OEMtoUnicode failed with %xh.\n", Status)); 
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH_LEAVE;
            }

            DEBUG(DL_CP, ( "Ex2QueryDirectory: process inode: %xh / %wZ (%d).\n",
                            pDir->inode, &InodeFileName, InodeFileName.Length)); 

            if (FsRtlDoesNameContainWildCards(
                &(Ccb->DirectorySearchPattern)) ?
                    FsRtlIsNameInExpression(
                        &(Ccb->DirectorySearchPattern),
                        &InodeFileName,
                        TRUE,
                        NULL) :
                !RtlCompareUnicodeString(
                    &(Ccb->DirectorySearchPattern),
                    &InodeFileName,
                    TRUE)           ) {

                Status = Ext2ProcessDirEntry(
                                IrpContext,
                                Vcb,
                                Fcb,
                                FileInformationClass,
                                pDir->inode,
                                Buffer,
                                CEILING_ALIGNED(ULONG, _SEH_VAR(UsedLength), 8),
                                _SEH_VAR(Length) - CEILING_ALIGNED(ULONG, _SEH_VAR(UsedLength), 8),
                                ByteOffset,
                                &InodeFileName,
                                &EntrySize,
                                ReturnSingleEntry
                                );

                if (NT_SUCCESS(Status)) {
                    if (EntrySize > 0) {
                        PrevEntry  = CEILING_ALIGNED(ULONG, _SEH_VAR(UsedLength), 8);
                        _SEH_VAR(UsedLength) = PrevEntry + EntrySize;
                    } else {
                        DbgBreak();
                    }
                } else {
                    if (Status != STATUS_BUFFER_OVERFLOW) {
                        _SEH_LEAVE;
                    }
                    break;
                }
            }

ProcessNextEntry:
            
            ByteOffset += RecLen;
            Ccb->CurrentByteOffset = ByteOffset;

            if (_SEH_VAR(UsedLength) && ReturnSingleEntry) {
                Status = STATUS_SUCCESS;
                _SEH_LEAVE;
            }
        }

        FileIndex = ByteOffset;
        ((PULONG)((PUCHAR)Buffer + PrevEntry))[0] = 0;

        if (!_SEH_VAR(UsedLength)) {
            if (NT_SUCCESS(Status)) {
                if (FirstQuery) {
                    Status = STATUS_NO_SUCH_FILE;
                } else {
                    Status = STATUS_NO_MORE_FILES;
                }
            } else {
                ASSERT(Status == STATUS_BUFFER_OVERFLOW);
                _SEH_VAR(UsedLength) = EntrySize;
            }
        } else {
            Status = STATUS_SUCCESS;
        }

    }
    _SEH_FINALLY(Ext2QueryDirectoryFinal_PSEH)
    _SEH_END;
    
    return Status;
}

_SEH_DEFINE_LOCALS(Ext2NotifyChangeDirectoryFinal)
{
    PEXT2_IRP_CONTEXT    IrpContext;
    BOOLEAN              CompleteRequest;
    PNTSTATUS            pStatus;
    PEXT2_FCB            Fcb;
    BOOLEAN              bFcbAcquired;
};

_SEH_FINALLYFUNC(Ext2NotifyChangeDirectoryFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2NotifyChangeDirectoryFinal);
    Ext2NotifyChangeDirectoryFinal(_SEH_VAR(IrpContext), _SEH_VAR(CompleteRequest),
                                   _SEH_VAR(pStatus), _SEH_VAR(Fcb),
                                   _SEH_VAR(bFcbAcquired));
}

VOID
Ext2NotifyChangeDirectoryFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN BOOLEAN              CompleteRequest,
    IN PNTSTATUS            pStatus,
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              bFcbAcquired
    )
{
    if (bFcbAcquired) {
        ExReleaseResourceLite(&Fcb->MainResource);
    }

    if (!IrpContext->ExceptionInProgress) {
        if (CompleteRequest) {
            if (*pStatus == STATUS_PENDING) {
                Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, *pStatus);
            }
        } else {
            IrpContext->Irp = NULL;
            Ext2CompleteIrpContext(IrpContext, *pStatus);
        }
    }
}

NTSTATUS
Ext2NotifyChangeDirectory (
    IN PEXT2_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB           Vcb;
    PFILE_OBJECT        FileObject;
    PEXT2_FCB           Fcb;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               CompletionFilter;
    BOOLEAN             WatchTree;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2NotifyChangeDirectoryFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(CompleteRequest) = TRUE;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(bFcbAcquired) = FALSE;

        ASSERT(IrpContext);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        //
        //  Always set the wait flag in the Irp context for the original request.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

        DeviceObject = IrpContext->DeviceObject;

        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));

        FileObject = IrpContext->FileObject;

        Fcb = (PEXT2_FCB) FileObject->FsContext;

        ASSERT(Fcb);

        if (Fcb->Identifier.Type == EXT2VCB) {
            DbgBreak();
            Status = STATUS_INVALID_PARAMETER;
            _SEH_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (!IsDirectory(Fcb)) {
            DbgBreak();
            Status = STATUS_NOT_A_DIRECTORY;
            _SEH_LEAVE;
        }

        if (ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                TRUE ))  {
            _SEH_VAR(bFcbAcquired) = TRUE;
        } else {
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_

        CompletionFilter =
            IrpSp->Parameters.NotifyDirectory.CompletionFilter;

#else // _GNU_NTIFS_

        CompletionFilter = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.NotifyDirectory.CompletionFilter;

#endif // _GNU_NTIFS_

        WatchTree = IsFlagOn(IrpSp->Flags, SL_WATCH_TREE);

        if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
            Status = STATUS_DELETE_PENDING;
            _SEH_LEAVE;
        }

        FsRtlNotifyFullChangeDirectory( Vcb->NotifySync,
                                        &Vcb->NotifyList,
                                        FileObject->FsContext2,
                                        (PSTRING)(&Fcb->Mcb->FullName),
                                        WatchTree,
                                        FALSE,
                                        CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );

        _SEH_VAR(CompleteRequest) = FALSE;

        Status = STATUS_PENDING;

/*
    Currently the driver is read-only but here is an example on how to use the
    FsRtl-functions to report a change:

    ANSI_STRING TestString;
    USHORT      FileNamePartLength;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FileNamePartLength = 7;

    FsRtlNotifyReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        &FileNamePartLength,        // PUSHORT      FileNamePartLength
        FILE_NOTIFY_CHANGE_NAME     // ULONG        FilterMatch
        );

    or

    ANSI_STRING TestString;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FsRtlNotifyFullReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        1,                          // USHORT       TargetNameOffset
        NULL,                       // PSTRING      StreamName OPTIONAL
        NULL,                       // PSTRING      NormalizedParentName OPTIONAL
        FILE_NOTIFY_CHANGE_NAME,    // ULONG        FilterMatch
        0,                          // ULONG        Action
        NULL                        // PVOID        TargetContext
        );
*/

    }
    _SEH_FINALLY(Ext2NotifyChangeDirectoryFinal_PSEH)
    _SEH_END;

    return Status;
}

VOID
Ext2NotifyReportChange (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_MCB         Mcb,
    IN ULONG             Filter,
    IN ULONG             Action   )
{
    USHORT          Offset;

    Offset = (USHORT) ( Mcb->FullName.Length - 
                        Mcb->ShortName.Length);

    FsRtlNotifyFullReportChange( Vcb->NotifySync,
                                 &(Vcb->NotifyList),
                                 (PSTRING) (&Mcb->FullName),
                                 (USHORT) Offset,
                                 (PSTRING)NULL,
                                 (PSTRING) NULL,
                                 (ULONG) Filter,
                                 (ULONG) Action,
                                 (PVOID) NULL );

    // ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
}


NTSTATUS
Ext2DirectoryControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    switch (IrpContext->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:
        Status = Ext2QueryDirectory(IrpContext);
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = Ext2NotifyChangeDirectory(IrpContext);
        break;
        
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Ext2CompleteIrpContext(IrpContext, Status);
    }
    
    return Status;
}

_SEH_DEFINE_LOCALS(Ext2IsDirectoryEmptyFinal)
{
    PEXT2_DIR_ENTRY2     pTarget;
};

_SEH_FINALLYFUNC(Ext2IsDirectoryEmptyFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2IsDirectoryEmptyFinal);
    Ext2IsDirectoryEmptyFinal(_SEH_VAR(pTarget));
}

VOID
Ext2IsDirectoryEmptyFinal (
    IN PEXT2_DIR_ENTRY2     pTarget
    )
{
    if (pTarget != NULL) {
        ExFreePoolWithTag(pTarget, EXT2_DENTRY_MAGIC);
    }
}

BOOLEAN
Ext2IsDirectoryEmpty (
    PEXT2_IRP_CONTEXT   IrpContext,
    PEXT2_VCB           Vcb,
    PEXT2_MCB           Mcb
    )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pTarget = NULL;

    ULONG                   ByteOffset = 0;
    ULONG                   dwRet;

    BOOLEAN                 bEmpty = FALSE;

    if (!IsMcbDirectory(Mcb) || IsMcbSymlink(Mcb)) {
        return TRUE;
    }

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2IsDirectoryEmptyFinal);
        _SEH_VAR(pTarget) = NULL;

        pTarget = (PEXT2_DIR_ENTRY2)
                    ExAllocatePoolWithTag(
                        PagedPool,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        EXT2_DENTRY_MAGIC
                    );
        _SEH_VAR(pTarget) = pTarget;
        if (!pTarget) {
            DEBUG(DL_ERR, ( "Ex2isDirectoryEmpty: failed to allocate pTarget.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH_LEAVE;
        }

        ByteOffset = 0;

        while (ByteOffset < Mcb->Inode->i_size) {

            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            Status = Ext2ReadInode(
                        IrpContext,
                        Vcb,
                        Mcb,
                        (ULONGLONG)ByteOffset,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        FALSE,
                        &dwRet);

            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_ERR, ( "Ext2IsDirectoryEmpty: Reading Directory Content error.\n"));
                _SEH_LEAVE;
            }

            if (pTarget->inode) {
                if (pTarget->name_len == 1 && pTarget->name[0] == '.') {
                } else if (pTarget->name_len == 2 && pTarget->name[0] == '.' && 
                         pTarget->name[1] == '.') {
                } else {
                    break;
                }
            }

            ByteOffset += pTarget->rec_len;
        }

        bEmpty = (ByteOffset >= Mcb->Inode->i_size);

    }
    _SEH_FINALLY(Ext2IsDirectoryEmptyFinal_PSEH)
    _SEH_END;
    
    return bEmpty;
}
