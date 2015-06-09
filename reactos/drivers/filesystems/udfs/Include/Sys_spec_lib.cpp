////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Sys_Spec.cpp
*
* Module: UDF File System Driver 
* (both User and Kernel mode execution)
*
* Description:
*   Contains system-secific code
*
*************************************************************************/


/*
    This routine converts UDF timestamp to NT time
 */
LONGLONG
UDFTimeToNT(
    IN PUDF_TIME_STAMP UdfTime
    )
{
    LONGLONG NtTime;
    TIME_FIELDS TimeFields;

    TimeFields.Milliseconds = (USHORT)(UdfTime->centiseconds * 10 + UdfTime->hundredsOfMicroseconds / 100);
    TimeFields.Second = (USHORT)(UdfTime->second);
    TimeFields.Minute = (USHORT)(UdfTime->minute);
    TimeFields.Hour = (USHORT)(UdfTime->hour);
    TimeFields.Day = (USHORT)(UdfTime->day);
    TimeFields.Month = (USHORT)(UdfTime->month);
    TimeFields.Year = (USHORT)((UdfTime->year < 1601) ? 1601 : UdfTime->year);

    if (!RtlTimeFieldsToTime(&TimeFields, (PLARGE_INTEGER)&NtTime)) {
        NtTime = 0;
    } else {
        ExLocalTimeToSystemTime( (PLARGE_INTEGER)&NtTime, (PLARGE_INTEGER)&NtTime );
    }

    return NtTime;
} // end UDFTimeToNT()


/*
    This routine converts NT time to UDF timestamp
 */
VOID
UDFTimeToUDF(
    IN LONGLONG NtTime,
    OUT PUDF_TIME_STAMP UdfTime
    )
{
    if(!NtTime) return;
    LONGLONG LocalTime;

    TIME_FIELDS TimeFields;

    ExSystemTimeToLocalTime( (PLARGE_INTEGER)&NtTime, (PLARGE_INTEGER)&LocalTime );
    RtlTimeToTimeFields( (PLARGE_INTEGER)&LocalTime, &TimeFields );

    LocalTime /= 10; // microseconds
    UdfTime->microseconds = (UCHAR)(NtTime % 100);
    LocalTime /= 100; // hundreds of microseconds
    UdfTime->hundredsOfMicroseconds = (UCHAR)(NtTime % 100);
    LocalTime /= 100; // centiseconds
    UdfTime->centiseconds = (UCHAR)(TimeFields.Milliseconds / 10);
    UdfTime->second = (UCHAR)(TimeFields.Second);
    UdfTime->minute = (UCHAR)(TimeFields.Minute);
    UdfTime->hour = (UCHAR)(TimeFields.Hour);
    UdfTime->day = (UCHAR)(TimeFields.Day);
    UdfTime->month = (UCHAR)(TimeFields.Month);
    UdfTime->year = (USHORT)(TimeFields.Year);
    UdfTime->typeAndTimezone = (TIMESTAMP_TYPE_LOCAL << 14);
} // end UDFTimeToUDF()

/*
 */
ULONG
UDFAttributesToNT(
    IN PDIR_INDEX_ITEM FileDirNdx,
    IN tag* FileEntry
    )
{
    ASSERT(FileDirNdx);
    if( (FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) &&
       !(FileDirNdx->FI_Flags & UDF_FI_FLAG_LINKED))
        return FileDirNdx->SysAttr;

    ULONG NTAttr = 0;
    ULONG attr = 0; //permissions
    USHORT Flags = 0;
    USHORT Type = 0;
    UCHAR FCharact = 0;

    if(!FileEntry) {
        if(!FileDirNdx->FileInfo)
            return 0;
        ValidateFileInfo(FileDirNdx->FileInfo);
        FileEntry = FileDirNdx->FileInfo->Dloc->FileEntry;
    }
    if(FileEntry->tagIdent == TID_FILE_ENTRY) {
        attr = ((PFILE_ENTRY)FileEntry)->permissions;
        Flags = ((PFILE_ENTRY)FileEntry)->icbTag.flags;
        Type = ((PFILE_ENTRY)FileEntry)->icbTag.fileType;
        if(((PFILE_ENTRY)FileEntry)->fileLinkCount > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    } else {
        attr = ((PEXTENDED_FILE_ENTRY)FileEntry)->permissions;
        Flags = ((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.flags;
        Type = ((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.fileType;
        if(((PEXTENDED_FILE_ENTRY)FileEntry)->fileLinkCount > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    }
    FCharact = FileDirNdx->FileCharacteristics;

    if(Flags & ICB_FLAG_SYSTEM) NTAttr |= FILE_ATTRIBUTE_SYSTEM;
    if(Flags & ICB_FLAG_ARCHIVE) NTAttr |= FILE_ATTRIBUTE_ARCHIVE;
    if((Type == UDF_FILE_TYPE_DIRECTORY) ||
       (Type == UDF_FILE_TYPE_STREAMDIR) ||
       (FCharact & FILE_DIRECTORY)) {
        NTAttr |= FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
    } else {
        //NTAttr |= FILE_ATTRIBUTE_NORMAL;
#endif
    }
    if(FCharact & FILE_HIDDEN) NTAttr |= FILE_ATTRIBUTE_HIDDEN;
    if( !(attr & PERM_O_WRITE) &&
        !(attr & PERM_G_WRITE) &&
        !(attr & PERM_U_WRITE) &&
        !(attr & PERM_O_DELETE) &&
        !(attr & PERM_G_DELETE) &&
        !(attr & PERM_U_DELETE) ) {
        NTAttr |= FILE_ATTRIBUTE_READONLY;
    }
    FileDirNdx->SysAttr = NTAttr;
    return NTAttr;
} // end UDFAttributesToNT()

/*
 */
VOID
UDFAttributesToUDF(
    IN PDIR_INDEX_ITEM FileDirNdx,
    IN tag* FileEntry,
    IN ULONG NTAttr
    )
{
    PULONG attr; //permissions
    PUSHORT Flags;
    PUCHAR Type;
    PUCHAR FCharact;

    NTAttr &= UDF_VALID_FILE_ATTRIBUTES;

    if(!FileEntry) {
        ASSERT(FileDirNdx);
        if(!FileDirNdx->FileInfo)
            return;
        ValidateFileInfo(FileDirNdx->FileInfo);
        FileEntry = FileDirNdx->FileInfo->Dloc->FileEntry;
        FileDirNdx->FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    }
    if(FileEntry->tagIdent == TID_FILE_ENTRY) {
        attr = &((PFILE_ENTRY)FileEntry)->permissions;
        Flags = &((PFILE_ENTRY)FileEntry)->icbTag.flags;
        Type = &((PFILE_ENTRY)FileEntry)->icbTag.fileType;
    } else {
        attr = &((PEXTENDED_FILE_ENTRY)FileEntry)->permissions;
        Flags = &((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.flags;
        Type = &((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.fileType;
    }
    FCharact = &(FileDirNdx->FileCharacteristics);

    if((*FCharact & FILE_DIRECTORY) ||
       (*Type == UDF_FILE_TYPE_STREAMDIR) || 
       (*Type == UDF_FILE_TYPE_DIRECTORY)) {
        *FCharact |= FILE_DIRECTORY;
        if(*Type != UDF_FILE_TYPE_STREAMDIR)
            *Type = UDF_FILE_TYPE_DIRECTORY;
        *attr |= (PERM_O_EXEC | PERM_G_EXEC | PERM_U_EXEC);
        NTAttr |= FILE_ATTRIBUTE_DIRECTORY;
        NTAttr &= ~FILE_ATTRIBUTE_NORMAL;
    } else {
        *FCharact &= ~FILE_DIRECTORY;
        *Type = UDF_FILE_TYPE_REGULAR;
        *attr &= ~(PERM_O_EXEC | PERM_G_EXEC | PERM_U_EXEC);
    }

    if(NTAttr & FILE_ATTRIBUTE_SYSTEM) {
        *Flags |= ICB_FLAG_SYSTEM;
    } else {
        *Flags &= ~ICB_FLAG_SYSTEM;
    }
    if(NTAttr & FILE_ATTRIBUTE_ARCHIVE) {
        *Flags |= ICB_FLAG_ARCHIVE;
    } else {
        *Flags &= ~ICB_FLAG_ARCHIVE;
    }
    if(NTAttr & FILE_ATTRIBUTE_HIDDEN) {
       *FCharact |= FILE_HIDDEN;
    } else {
       *FCharact &= ~FILE_HIDDEN;
    }
    *attr |= (PERM_O_READ | PERM_G_READ | PERM_U_READ);
    if(!(NTAttr & FILE_ATTRIBUTE_READONLY)) {
        *attr |= (PERM_O_WRITE  | PERM_G_WRITE  | PERM_U_WRITE |
                  PERM_O_DELETE | PERM_G_DELETE | PERM_U_DELETE |
                  PERM_O_CHATTR | PERM_G_CHATTR | PERM_U_CHATTR);
    } else {
        *attr &= ~(PERM_O_WRITE  | PERM_G_WRITE  | PERM_U_WRITE |
                   PERM_O_DELETE | PERM_G_DELETE | PERM_U_DELETE |
                   PERM_O_CHATTR | PERM_G_CHATTR | PERM_U_CHATTR);
    }
    FileDirNdx->SysAttr = NTAttr;
    if(FileDirNdx->FileInfo)
        FileDirNdx->FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    FileDirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
    return;
} // end UDFAttributesToUDF()

#ifndef _CONSOLE
/*
    This routine fills PFILE_BOTH_DIR_INFORMATION structure (NT)
 */
NTSTATUS
UDFFileDirInfoToNT(
    IN PVCB Vcb,
    IN PDIR_INDEX_ITEM FileDirNdx,
    OUT PFILE_BOTH_DIR_INFORMATION NTFileInfo
    )
{
    PFILE_ENTRY FileEntry;
    UNICODE_STRING UdfName;
    UNICODE_STRING DosName;
    PEXTENDED_FILE_ENTRY ExFileEntry;
    USHORT Ident;
    BOOLEAN ReadSizes;
    NTSTATUS status;
    PtrUDFNTRequiredFCB NtReqFcb;

    KdPrint(("@=%#x, FileDirNdx %x\n", &Vcb, FileDirNdx));

    ASSERT((ULONG)NTFileInfo > 0x1000);
    RtlZeroMemory(NTFileInfo, sizeof(FILE_BOTH_DIR_INFORMATION));
    
    DosName.Buffer = (PWCHAR)&(NTFileInfo->ShortName);
    DosName.MaximumLength = sizeof(NTFileInfo->ShortName); // 12*sizeof(WCHAR)

    _SEH2_TRY {
        KdPrint(("  DirInfoToNT: %*.*S\n", FileDirNdx->FName.Length/sizeof(WCHAR), FileDirNdx->FName.Length/sizeof(WCHAR), FileDirNdx->FName));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        KdPrint(("  DirInfoToNT: exception when printing file name\n"));
    } _SEH2_END;

    if(FileDirNdx->FileInfo) {
        KdPrint(("    FileInfo\n"));
        // validate FileInfo
        ValidateFileInfo(FileDirNdx->FileInfo);
        if(UDFGetFileLinkCount(FileDirNdx->FileInfo) > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
        FileEntry = (PFILE_ENTRY)(FileDirNdx->FileInfo->Dloc->FileEntry);
        // read required sizes from Fcb (if any) if file is not linked
        // otherwise we should read them from FileEntry
        if(FileDirNdx->FileInfo->Fcb) {
            KdPrint(("    Fcb\n"));
            NtReqFcb = FileDirNdx->FileInfo->Fcb->NTRequiredFCB;
            NTFileInfo->CreationTime.QuadPart   = NtReqFcb->CreationTime.QuadPart;
            NTFileInfo->LastWriteTime.QuadPart  = NtReqFcb->LastWriteTime.QuadPart;
            NTFileInfo->LastAccessTime.QuadPart = NtReqFcb->LastAccessTime.QuadPart;
            NTFileInfo->ChangeTime.QuadPart     = NtReqFcb->ChangeTime.QuadPart;
//            NTFileInfo->AllocationSize.QuadPart = NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart;
            NTFileInfo->AllocationSize.QuadPart = FileDirNdx->AllocationSize;
/*            FileDirNdx->FileSize =
            NTFileInfo->EndOfFile.QuadPart = NtReqFcb->CommonFCBHeader.FileSize.QuadPart;*/
            NTFileInfo->EndOfFile.QuadPart = FileDirNdx->FileSize;
            if(FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) {
                KdPrint(("    SYS_ATTR\n"));
                NTFileInfo->FileAttributes = FileDirNdx->SysAttr;
                goto get_name_only;
            }
            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart;
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart;
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart;
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart;
            goto get_attr_only;
        }
        ASSERT(FileEntry);
    } else if(!(FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) ||
               (FileDirNdx->FI_Flags & UDF_FI_FLAG_LINKED)) {
        LONG_AD feloc;

        KdPrint(("  !SYS_ATTR\n"));
        FileEntry = (PFILE_ENTRY)MyAllocatePool__(NonPagedPool, Vcb->LBlockSize);
        if(!FileEntry) return STATUS_INSUFFICIENT_RESOURCES;

        feloc.extLength = Vcb->LBlockSize;
        feloc.extLocation = FileDirNdx->FileEntryLoc;

        if(!NT_SUCCESS(status = UDFReadFileEntry(Vcb, &feloc, FileEntry, &Ident))) {
            KdPrint(("    !UDFReadFileEntry\n"));
            MyFreePool__(FileEntry);
            FileEntry = NULL;
            goto get_name_only;
        }
        ReadSizes = TRUE;
    } else {
        KdPrint(("  FileDirNdx\n"));
        NTFileInfo->CreationTime.QuadPart   = FileDirNdx->CreationTime;
        NTFileInfo->LastWriteTime.QuadPart  = FileDirNdx->LastWriteTime;
        NTFileInfo->LastAccessTime.QuadPart = FileDirNdx->LastAccessTime;
        NTFileInfo->ChangeTime.QuadPart     = FileDirNdx->ChangeTime;
        NTFileInfo->FileAttributes = FileDirNdx->SysAttr;
        NTFileInfo->AllocationSize.QuadPart = FileDirNdx->AllocationSize;
        NTFileInfo->EndOfFile.QuadPart = FileDirNdx->FileSize;
        NTFileInfo->EaSize = 0;
        FileEntry = NULL;
        goto get_name_only;
    }

    if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)
        goto get_name_only;

    KdPrint(("  direct\n"));
    if(FileEntry->descTag.tagIdent == TID_FILE_ENTRY) {
        KdPrint(("  TID_FILE_ENTRY\n"));
        if(ReadSizes) {
            KdPrint(("    ReadSizes\n"));
            // Times
            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart   =
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart  = UDFTimeToNT(&(FileEntry->modificationTime));
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = UDFTimeToNT(&(FileEntry->accessTime));
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart     = UDFTimeToNT(&(FileEntry->attrTime));
            // FileSize
            FileDirNdx->FileSize =
            NTFileInfo->EndOfFile.QuadPart =
                FileEntry->informationLength;
            KdPrint(("    informationLength=%I64x, lengthAllocDescs=%I64x\n",
                FileEntry->informationLength,
                FileEntry->lengthAllocDescs
                ));
            // AllocSize
            FileDirNdx->AllocationSize =
            NTFileInfo->AllocationSize.QuadPart =
                (FileEntry->informationLength + Vcb->LBlockSize - 1) & ~((LONGLONG)(Vcb->LBlockSize) - 1);
        }
//        NTFileInfo->EaSize = 0;//FileEntry->lengthExtendedAttr;
    } else if(FileEntry->descTag.tagIdent == TID_EXTENDED_FILE_ENTRY) {
        ExFileEntry = (PEXTENDED_FILE_ENTRY)FileEntry;
        KdPrint(("  PEXTENDED_FILE_ENTRY\n"));
        if(ReadSizes) {
            KdPrint(("    ReadSizes\n"));
            // Times
            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart   = UDFTimeToNT(&(ExFileEntry->createTime));
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart  = UDFTimeToNT(&(ExFileEntry->modificationTime));
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = UDFTimeToNT(&(ExFileEntry->accessTime));
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart     = UDFTimeToNT(&(ExFileEntry->attrTime));
            // FileSize
            FileDirNdx->FileSize =
            NTFileInfo->EndOfFile.QuadPart =
                ExFileEntry->informationLength;
            KdPrint(("    informationLength=%I64x, lengthAllocDescs=%I64x\n",
                FileEntry->informationLength,
                FileEntry->lengthAllocDescs
                ));
            // AllocSize
            FileDirNdx->AllocationSize =
            NTFileInfo->AllocationSize.QuadPart =
                (ExFileEntry->informationLength + Vcb->LBlockSize - 1) & ~((LONGLONG)(Vcb->LBlockSize) - 1);
        }
//        NTFileInfo->EaSize = 0;//ExFileEntry->lengthExtendedAttr;
    } else {
        KdPrint(("  ???\n"));
        goto get_name_only;
    }

get_attr_only:

    KdPrint(("  get_attr"));
    // do some substitutions
    if(!FileDirNdx->CreationTime) {
        FileDirNdx->CreationTime = NTFileInfo->CreationTime.QuadPart = Vcb->VolCreationTime;
    }
    if(!FileDirNdx->LastAccessTime) {
        FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = FileDirNdx->CreationTime;
    }
    if(!FileDirNdx->LastWriteTime) {
        FileDirNdx->LastWriteTime = NTFileInfo->LastWriteTime.QuadPart = FileDirNdx->CreationTime;
    }
    if(!FileDirNdx->ChangeTime) {
        FileDirNdx->ChangeTime = NTFileInfo->ChangeTime.QuadPart = FileDirNdx->CreationTime;
    }

    FileDirNdx->SysAttr =
    NTFileInfo->FileAttributes = UDFAttributesToNT(FileDirNdx, (tag*)FileEntry);
    FileDirNdx->FI_Flags |= UDF_FI_FLAG_SYS_ATTR;

get_name_only:
    // get filename in standard Unicode format
    UdfName = FileDirNdx->FName;
    NTFileInfo->FileNameLength = UdfName.Length;
    RtlCopyMemory((PCHAR)&(NTFileInfo->FileName), (PCHAR)(UdfName.Buffer), UdfName.MaximumLength);
    if(!(FileDirNdx->FI_Flags & UDF_FI_FLAG_DOS)) {
        KdPrint(("  !UDF_FI_FLAG_DOS"));
        UDFDOSName(Vcb, &DosName, &UdfName,
            (FileDirNdx->FI_Flags & UDF_FI_FLAG_KEEP_NAME) ? TRUE : FALSE);
        NTFileInfo->ShortNameLength = (UCHAR)DosName.Length;
    }
    // report zero EOF & AllocSize for Dirs
    if(FileDirNdx->FileCharacteristics & FILE_DIRECTORY) {
        KdPrint(("  FILE_DIRECTORY"));
        NTFileInfo->AllocationSize.QuadPart =
        NTFileInfo->EndOfFile.QuadPart = 0;
    }
    KdPrint(("  AllocationSize=%I64x, NTFileInfo->EndOfFile=%I64x", NTFileInfo->AllocationSize.QuadPart, NTFileInfo->EndOfFile.QuadPart));
    // free tmp buffer (if any)
    KdPrint(("\n"));
    if(FileEntry && !FileDirNdx->FileInfo)
        MyFreePool__(FileEntry);
    return STATUS_SUCCESS;
} // end UDFFileDirInfoToNT()

#endif //_CONSOLE

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine changes xxxTime field(s) in (Ext)FileEntry
 */
VOID
UDFSetFileXTime(
    IN PUDF_FILE_INFO FileInfo,
    IN LONGLONG* CrtTime,
    IN LONGLONG* AccTime,
    IN LONGLONG* AttrTime,
    IN LONGLONG* ChgTime
    )
{
    USHORT Ident;
    PDIR_INDEX_ITEM DirNdx;

    ValidateFileInfo(FileInfo);

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
    Ident = FileInfo->Dloc->FileEntry->tagIdent;

    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if(AccTime) {
            if(DirNdx && *AccTime) DirNdx->LastAccessTime = *AccTime;
            UDFTimeToUDF(*AccTime, &(fe->accessTime));
        }
        if(AttrTime) {
            if(DirNdx && *AttrTime) DirNdx->ChangeTime = *AttrTime;
            UDFTimeToUDF(*AttrTime, &(fe->attrTime));
        }
        if(ChgTime) {
            if(DirNdx && *ChgTime) DirNdx->CreationTime =
                DirNdx->LastWriteTime = *ChgTime;
            UDFTimeToUDF(*ChgTime, &(fe->modificationTime));
        } else
        if(CrtTime) {
            if(DirNdx && *CrtTime) DirNdx->CreationTime =
                DirNdx->LastWriteTime = *CrtTime;
            UDFTimeToUDF(*CrtTime, &(fe->modificationTime));
        }

    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if(AccTime) {
            if(DirNdx && *AccTime) DirNdx->LastAccessTime = *AccTime;
            UDFTimeToUDF(*AccTime, &(fe->accessTime));
        }
        if(AttrTime) {
            if(DirNdx && *AttrTime) DirNdx->ChangeTime = *AttrTime;
            UDFTimeToUDF(*AttrTime, &(fe->attrTime));
        }
        if(ChgTime) {
            if(DirNdx && *ChgTime) DirNdx->LastWriteTime = *ChgTime;
            UDFTimeToUDF(*ChgTime, &(fe->modificationTime));
        }
        if(CrtTime) {
            if(DirNdx && *CrtTime) DirNdx->CreationTime = *CrtTime;
            UDFTimeToUDF(*CrtTime, &(fe->createTime));
        }

    }
} // end UDFSetFileXTime()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine gets xxxTime field(s) in (Ext)FileEntry
 */
VOID
UDFGetFileXTime(
    IN PUDF_FILE_INFO FileInfo,
    OUT LONGLONG* CrtTime,
    OUT LONGLONG* AccTime,
    OUT LONGLONG* AttrTime,
    OUT LONGLONG* ChgTime
    )
{
    USHORT Ident;

    ValidateFileInfo(FileInfo);

    Ident = FileInfo->Dloc->FileEntry->tagIdent;

    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if(AccTime) *AccTime = UDFTimeToNT(&(fe->accessTime));
        if(AttrTime) *AttrTime = UDFTimeToNT(&(fe->attrTime));
        if(ChgTime) *ChgTime = UDFTimeToNT(&(fe->modificationTime));
        if(CrtTime) {
            (*CrtTime) = *ChgTime;
        }

    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if(AccTime) *AccTime = UDFTimeToNT(&(fe->accessTime));
        if(AttrTime) *AttrTime = UDFTimeToNT(&(fe->attrTime));
        if(ChgTime) *ChgTime = UDFTimeToNT(&(fe->modificationTime));
        if(CrtTime) *CrtTime = UDFTimeToNT(&(fe->createTime));

    }
    if(CrtTime) {
        if(!(*CrtTime)) 
            KeQuerySystemTime((PLARGE_INTEGER)CrtTime);
        if(AccTime && !(*AccTime)) (*AccTime) = *CrtTime;
        if(AttrTime && !(*AttrTime)) (*AttrTime) = *CrtTime;
        if(AccTime && !(*AccTime)) (*AccTime) = *CrtTime;
    }
} // end UDFGetFileXTime()

VOID
UDFNormalizeFileName(
    IN PUNICODE_STRING FName,
    IN USHORT valueCRC
    )
{
    PWCHAR buffer;
    USHORT len;

    len = FName->Length/sizeof(WCHAR);
    buffer = FName->Buffer;

    // check for '',  '.'  &  '..'
    if(!len) return;
    if(!buffer[len-1]) {
        FName->Length-=sizeof(WCHAR);
        len--;
    }
    if(!len) return;
    if(buffer[0] == UNICODE_PERIOD) {
        if(len == 1) return;
        if((buffer[1] == UNICODE_PERIOD) && (len == 2)) return;
    }

    // check for trailing '.'
    for(len--;len;len--) {
        if( ((buffer[len] == UNICODE_PERIOD) || (buffer[len] == UNICODE_SPACE)) ) {
            FName->Length-=sizeof(WCHAR);
            buffer[len] = 0;
        } else
            break;
    }
} // end UDFNormalizeFileName()

#ifndef _CONSOLE

void 
__fastcall
UDFDOSNameOsNative(
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact
    )
{
    PWCHAR dosName = DosName->Buffer;
    PWCHAR udfName = UdfName->Buffer;
    uint32 udfLen = UdfName->Length / sizeof(WCHAR);
    GENERATE_NAME_CONTEXT Ctx;

    if(KeepIntact &&
       (udfLen <= 2) && (udfName[0] == UNICODE_PERIOD)) {
        if((udfLen != 2) || (udfName[1] == UNICODE_PERIOD)) {
            RtlCopyMemory(dosName, udfName, UdfName->Length);
            DosName->Length = UdfName->Length;
            return;
        }
    }
    RtlZeroMemory(&Ctx, sizeof(GENERATE_NAME_CONTEXT));
    RtlGenerate8dot3Name(UdfName, FALSE, &Ctx, DosName);
    
} // UDFDOSNameOsNative()

#endif //_CONSOLE

/*VOID
UDFNormalizeFileName(
    IN PUNICODE_STRING FName,
    IN USHORT valueCRC
    )
{
    WCHAR _newName[UDF_NAME_LEN+5];
    PWCHAR newName = (PWCHAR)(&_newName);
    PWCHAR udfName = FName->Buffer;
    LONG udfLen = FName->Length >> 1;

    LONG index, newIndex = 0, extIndex = 0, newExtIndex = 0, trailIndex = 0;
    BOOLEAN needsCRC = FALSE, hasExt = FALSE;
    WCHAR ext[UDF_EXT_SIZE], current;

    // handle CurrentDir ('.') and ParentDir ('..') cases
    if((udfLen <= 2) && (udfName[0] == UNICODE_PERIOD)) {
        if((udfLen != 2) || (udfName[1] == UNICODE_PERIOD))
            return;
    }

    for (index = 0 ; index < udfLen ; index++) {
        current = udfName[index];

        // Look for illegal or unprintable characters.
        if (UDFIsIllegalChar(current) || !UnicodeIsPrint(current)) {
            needsCRC = TRUE;
            current = ILLEGAL_CHAR_MARK;
            // Skip Illegal characters(even spaces),
            // but not periods.
            while(index+1 < udfLen &&
                  (UDFIsIllegalChar(udfName[index+1]) ||
                  !UnicodeIsPrint(udfName[index+1])) &&
                  udfName[index+1] != UNICODE_PERIOD)
                index++;
        }

        // Record position of extension, if one is found.
        if ((current == UNICODE_PERIOD) && ((udfLen - index -1) <= UDF_EXT_SIZE)) {
            if (udfLen == index + 1) {
                // A trailing period is NOT an extension.
                hasExt = FALSE;
            } else {
                hasExt = TRUE;
                extIndex = index;
                newExtIndex = newIndex;
            }
        } else if((current != UNICODE_PERIOD) && (current != UNICODE_SPACE)) {
            trailIndex = index;
        }

//        if (newIndex < MAXLEN)  // tshi is always TRUE for WINNT
        newName[newIndex] = current;
        newIndex++;

        // For OS2, 95 & NT, truncate any trailing periods and\or spaces.
        if (trailIndex != (newIndex - 1)) {
            newIndex = trailIndex + 1;
            needsCRC = TRUE;
            hasExt = FALSE; // Trailing period does not make an extension.
        }
    }

    if (needsCRC) {
        int localExtIndex = 0;
        if (hasExt) {
            int maxFilenameLen;
            // Translate extension, and store it in ext. 
            for(index = 0; index<UDF_EXT_SIZE && extIndex + index +1 < udfLen; index++ ) {
                current = udfName[extIndex + index + 1];
                if (UDFIsIllegalChar(current) //|| !UnicodeIsPrint(current)) {
                    needsCRC = TRUE;
                    // Replace Illegal and non-displayable chars
                    // with underscore.
                    current = ILLEGAL_CHAR_MARK;
                    // Skip any other illegal or non-displayable
                    // characters.
                    while(index + 1 < UDF_EXT_SIZE &&
                          (UDFIsIllegalChar(udfName[extIndex + index + 2]) ||
                          !UnicodeIsPrint(udfName[extIndex + index + 2])) )
                        index++;
                }
                ext[localExtIndex++] = current;
            }
            // Truncate filename to leave room for extension and CRC.
            maxFilenameLen = ((UDF_NAME_LEN - 4) - localExtIndex - 1);
            if (newIndex > maxFilenameLen) {
                newIndex = maxFilenameLen;
            } else {
                newIndex = newExtIndex;
            }
        } else if (newIndex > UDF_NAME_LEN - 5) {
            //If no extension, make sure to leave room for CRC.
            newIndex = UDF_NAME_LEN - 5;
        }
        newName[newIndex++] = UNICODE_CRC_MARK; // Add mark for CRC. 
        //Calculate CRC from original filename from FileIdentifier.
//        valueCRC = UDFUnicodeCksum(fidName, fidNameLen);
//        / Convert 16-bits of CRC to hex characters. 
        newName[newIndex++] = hexChar[(valueCRC & 0xf000) >> 12];
        newName[newIndex++] = hexChar[(valueCRC & 0x0f00) >> 8];
        newName[newIndex++] = hexChar[(valueCRC & 0x00f0) >> 4];
        newName[newIndex++] = hexChar[(valueCRC & 0x000f)];
        // Place a translated extension at end, if found. 
        if (hasExt) {
            newName[newIndex++] = UNICODE_PERIOD;
            for (index = 0;index < localExtIndex ;index++ ) {
                newName[newIndex++] = ext[index];
            }
        }
    }

    if(FName->Length == (USHORT)newIndex*sizeof(WCHAR)) {
        RtlCopyMemory(FName->Buffer, newName, newIndex*sizeof(WCHAR));
        return;
    }
    MyFreePool__(FName->Buffer);
    FName->Buffer = (PWCHAR)MyAllocatePool__(UDF_FILENAME_MT, (newIndex+1)*sizeof(WCHAR));
    if(FName->Buffer) {
        FName->Buffer[newIndex] = 0;
        RtlCopyMemory(FName->Buffer, newName, newIndex*sizeof(WCHAR));
    }
    FName->Length = (USHORT)newIndex*sizeof(WCHAR);
    FName->MaximumLength = (USHORT)(newIndex+1)*sizeof(WCHAR);
}*/

/*PUDF_FILE_INFO
UDFAllocFileInfo(
    return ExAllocateFromZone(&(UDFGlobalData.FileInfoZoneHeader));
)*/

#define STRING_BUFFER_ALIGNMENT  (32)
#define STRING_BUFFER_ALIGN(sz)  (((sz)+STRING_BUFFER_ALIGNMENT)&(~((ULONG)(STRING_BUFFER_ALIGNMENT-1))))

NTSTATUS
MyAppendUnicodeStringToString_(
    IN PUNICODE_STRING Str1,
    IN PUNICODE_STRING Str2
#ifdef UDF_TRACK_UNICODE_STR
   ,IN PCHAR Tag
#endif 
    )
{
    PWCHAR tmp;
    USHORT i;

#ifdef UDF_TRACK_UNICODE_STR
  #define UDF_UNC_STR_TAG Tag
#else
  #define UDF_UNC_STR_TAG "AppUStr"
#endif 

    tmp = Str1->Buffer;
    i = Str1->Length + Str2->Length + sizeof(WCHAR);
    ASSERT(Str1->MaximumLength);
    if(i > Str1->MaximumLength) {
        if(!MyReallocPool__((PCHAR)tmp, Str1->MaximumLength,
                         (PCHAR*)&tmp, STRING_BUFFER_ALIGN(i)*2) ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Str1->MaximumLength = i*2;
        Str1->Buffer = tmp;
    }
    RtlCopyMemory(((PCHAR)tmp)+Str1->Length, Str2->Buffer, Str2->Length);

/*    tmp = (PWCHAR)MyAllocatePoolTag__(NonPagedPool, i = Str1->Length + Str2->Length + sizeof(WCHAR), UDF_UNC_STR_TAG);
    if(!tmp)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(tmp, Str1->Buffer, Str1->Length);
    RtlCopyMemory(((PCHAR)tmp)+Str1->Length, Str2->Buffer, Str2->Length);*/
    tmp[(i / sizeof(WCHAR)) - 1] = 0;
    Str1->Length = i - sizeof(WCHAR);
    //MyFreePool__(Str1->Buffer);
#ifdef UDF_DBG
    if(Str1->Buffer && (Str1->Length >= 2*sizeof(WCHAR))) {
        ASSERT((Str1->Buffer[0] != L'\\') || (Str1->Buffer[1] != L'\\'));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;

#undef UDF_UNC_STR_TAG

} // end MyAppendUnicodeStringToString()

NTSTATUS
MyAppendUnicodeToString_(
    IN PUNICODE_STRING Str1,
    IN PCWSTR Str2
#ifdef UDF_TRACK_UNICODE_STR
   ,IN PCHAR Tag
#endif 
    )
{
    PWCHAR tmp;
    USHORT i;

#ifdef UDF_TRACK_UNICODE_STR
  #define UDF_UNC_STR_TAG Tag
#else
  #define UDF_UNC_STR_TAG "AppStr"
#endif 

//#ifdef _X86_
#ifdef _MSC_VER

    __asm push  ebx
    __asm push  esi

    __asm xor   ebx,ebx
    __asm mov   esi,Str2
Scan_1:
    __asm cmp   [word ptr esi+ebx],0
    __asm je    EO_Scan
    __asm add   ebx,2
    __asm jmp   Scan_1
EO_Scan:
    __asm mov   i,bx

    __asm pop   esi
    __asm pop   ebx

#else   // NO X86 optimization, use generic C/C++

    i=0;
    while(Str2[i]) {
       i++;
    }
    i *= sizeof(WCHAR);

#endif // _X86_

    tmp = Str1->Buffer;
    ASSERT(Str1->MaximumLength);
    if((Str1->Length+i+sizeof(WCHAR)) > Str1->MaximumLength) {
        if(!MyReallocPool__((PCHAR)tmp, Str1->MaximumLength,
                         (PCHAR*)&tmp, STRING_BUFFER_ALIGN(i + Str1->Length + sizeof(WCHAR))*2 ) ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Str1->MaximumLength = STRING_BUFFER_ALIGN(i + sizeof(WCHAR))*2;
        Str1->Buffer = tmp;
    }
    RtlCopyMemory(((PCHAR)tmp)+Str1->Length, Str2, i);
    i+=Str1->Length;
    tmp[(i / sizeof(WCHAR))] = 0;
    Str1->Length = i;
#ifdef UDF_DBG
/*    if(Str1->Buffer && (Str1->Length >= 2*sizeof(WCHAR))) {
        ASSERT((Str1->Buffer[0] != L'\\') || (Str1->Buffer[1] != L'\\'));
    }*/
#endif // UDF_DBG
    return STATUS_SUCCESS;

#undef UDF_UNC_STR_TAG

} // end MyAppendUnicodeToString_()

NTSTATUS
MyInitUnicodeString(
    IN PUNICODE_STRING Str1,
    IN PCWSTR Str2
    )
{

    USHORT i;

//#ifdef _X86_
#ifdef _MSC_VER

    __asm push  ebx
    __asm push  esi

    __asm xor   ebx,ebx
    __asm mov   esi,Str2
Scan_1:
    __asm cmp   [word ptr esi+ebx],0
    __asm je    EO_Scan
    __asm add   ebx,2
    __asm jmp   Scan_1
EO_Scan:
    __asm mov   i,bx

    __asm pop   esi
    __asm pop   ebx

#else   // NO X86 optimization, use generic C/C++

    i=0;
    while(Str2[i]) {
       i++;
    }
    i *= sizeof(WCHAR);

#endif // _X86_

    Str1->MaximumLength = STRING_BUFFER_ALIGN((Str1->Length = i) + sizeof(WCHAR));
    Str1->Buffer = (PWCHAR)MyAllocatePool__(NonPagedPool, Str1->MaximumLength);
    if(!Str1->Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(Str1->Buffer, Str2, i);
    Str1->Buffer[i/sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;

} // end MyInitUnicodeString()

NTSTATUS
MyCloneUnicodeString(
    IN PUNICODE_STRING Str1,
    IN PUNICODE_STRING Str2
    )
{
    Str1->MaximumLength = STRING_BUFFER_ALIGN((Str1->Length = Str2->Length) + sizeof(WCHAR));
    Str1->Buffer = (PWCHAR)MyAllocatePool__(NonPagedPool, Str1->MaximumLength);
    if(!Str1->Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Str2->Buffer);
    RtlCopyMemory(Str1->Buffer, Str2->Buffer, Str2->Length);
    Str1->Buffer[Str1->Length/sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;

} // end MyCloneUnicodeString()

/*
    This routine checks do we needn't read something from disk to
    obtain Attributes & so on
 */
BOOLEAN
UDFIsDirInfoCached(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    )
{
    PDIR_INDEX_HDR hDirNdx = DirInfo->Dloc->DirIndex;
    PDIR_INDEX_ITEM DirNdx;
    for(uint_di i=2; (DirNdx = UDFDirIndex(hDirNdx,i)); i++) {
        if(!(DirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) ||
            (DirNdx->FI_Flags & UDF_FI_FLAG_LINKED)) return FALSE;
    }
    return TRUE;
} // end UDFIsDirInfoCached()

#ifndef UDF_READ_ONLY_BUILD
NTSTATUS
UDFDoesOSAllowFileToBeTargetForRename__(
    IN PUDF_FILE_INFO FileInfo
    )
{
#ifndef _CONSOLE
    NTSTATUS RC;
#endif //_CONSOLE

    if(UDFIsADirectory(FileInfo))
        return STATUS_ACCESS_DENIED;
    if(!FileInfo->ParentFile)
        return STATUS_ACCESS_DENIED;

    if(UDFAttributesToNT(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index),
                         FileInfo->Dloc->FileEntry) & FILE_ATTRIBUTE_READONLY)
        return STATUS_ACCESS_DENIED;

    if(!FileInfo->Fcb)
        return STATUS_SUCCESS;
#ifndef _CONSOLE
    RC = UDFCheckAccessRights(NULL, NULL, FileInfo->Fcb, NULL, DELETE, 0);
    if(!NT_SUCCESS(RC))
        return RC;
#endif //_CONSOLE
    if(!FileInfo->Fcb)
        return STATUS_SUCCESS;
//    RC = UDFMarkStreamsForDeletion(FileInfo->Fcb->Vcb, FileInfo->Fcb, TRUE); // Delete
/*    RC = UDFSetDispositionInformation(FileInfo->Fcb, NULL,
                FileInfo->Fcb->Vcb, NULL, TRUE);
    if(NT_SUCCESS(RC)) {
        FileInfo->Fcb->FCBFlags |= UDF_FCB_DELETED;
        if(UDFGetFileLinkCount(FileInfo) <= 1) {
            FileInfo->Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_DELETED;
        }
    }
    return RC;*/
    return STATUS_ACCESS_DENIED;

} // end UDFDoesOSAllowFileToBeTargetForRename__()

NTSTATUS
UDFDoesOSAllowFileToBeUnlinked__(
    IN PUDF_FILE_INFO FileInfo
    )
{
    PDIR_INDEX_HDR hCurDirNdx;
    PDIR_INDEX_ITEM CurDirNdx;
    uint_di i;
//    IO_STATUS_BLOCK IoStatus;

    ASSERT(FileInfo->Dloc);

    if(!FileInfo->ParentFile)
        return STATUS_CANNOT_DELETE;
    if(FileInfo->Dloc->SDirInfo)
        return STATUS_CANNOT_DELETE;
    if(!UDFIsADirectory(FileInfo))
        return STATUS_SUCCESS;

//    UDFFlushAFile(FileInfo->Fcb, NULL, &IoStatus, 0);
    hCurDirNdx = FileInfo->Dloc->DirIndex;
    // check if we can delete all files
    for(i=2; (CurDirNdx = UDFDirIndex(hCurDirNdx,i)); i++) {
        // try to open Stream
        if(CurDirNdx->FileInfo)
            return STATUS_CANNOT_DELETE;
    }
//    return UDFCheckAccessRights(NULL, NULL, FileInfo->Fcb, NULL, DELETE, 0);
    return STATUS_SUCCESS;
} // end UDFDoesOSAllowFileToBeUnlinked__()

NTSTATUS
UDFDoesOSAllowFilePretendDeleted__(
    IN PUDF_FILE_INFO FileInfo
    )
{
    PDIR_INDEX_HDR hDirNdx = UDFGetDirIndexByFileInfo(FileInfo);
    if(!hDirNdx) return STATUS_CANNOT_DELETE;
    PDIR_INDEX_ITEM DirNdx = UDFDirIndex(hDirNdx, FileInfo->Index);
    if(!DirNdx) return STATUS_CANNOT_DELETE;
    // we can't hide file that is not marked as deleted
    if(!(DirNdx->FileCharacteristics & FILE_DELETED)) {
        BrutePoint();

#ifndef _CONSOLE
        if(!(FileInfo->Fcb->FCBFlags & (UDF_FCB_DELETE_ON_CLOSE |
                                        UDF_FCB_DELETED) ))
#endif //_CONSOLE

            return STATUS_CANNOT_DELETE;
    }
    return STATUS_SUCCESS;
}
#endif //UDF_READ_ONLY_BUILD

