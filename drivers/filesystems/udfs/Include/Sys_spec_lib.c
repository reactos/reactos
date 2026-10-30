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

#include            "udffs.h"

/*
    This routine converts UDF timestamp to NT time
 */
LONGLONG
UDFTimeToNT(
    IN PUDF_TIME_STAMP UdfTime
    )
{
#define TIMESTAMP_ZONE_MIN  (-1440)  // -24 hours in minutes
#define TIMESTAMP_ZONE_MAX  ( 1440)  // +24 hours in minutes

    LONGLONG NtTime;
    TIME_FIELDS TimeFields;

    // Extract Type (bits 15-12) and Zone (bits 11-0, signed 12-bit) from packed field
    USHORT TypeAndTz = UdfTime->typeAndTimezone;
    USHORT Type = (TypeAndTz >> 12) & 0x0F;
    SHORT Zone = (SHORT)(TypeAndTz << 4) >> 4; // sign-extend 12-bit value
    BOOLEAN HasOffset = (TypeAndTz & TIMESTAMP_OFFSET_MASK) != TIMESTAMP_NO_OFFSET;

    TimeFields.Second = (USHORT)(UdfTime->second);
    TimeFields.Minute = (USHORT)(UdfTime->minute);
    TimeFields.Hour = (USHORT)(UdfTime->hour);
    TimeFields.Day = (USHORT)(UdfTime->day);
    TimeFields.Month = (USHORT)(UdfTime->month);
    TimeFields.Year = (USHORT)((UdfTime->year < 1601) ? 1601 : UdfTime->year);
    TimeFields.Milliseconds = 0;

    if (Type <= TIMESTAMP_TYPE_LOCAL &&
        (!HasOffset || (Zone >= TIMESTAMP_ZONE_MIN && Zone <= TIMESTAMP_ZONE_MAX)) &&
        RtlTimeFieldsToTime(&TimeFields, (PLARGE_INTEGER)&NtTime)) {

        // Add sub-second precision: centiseconds (10^-2), hundredsOfMicroseconds (10^-4),
        // microseconds (10^-6), all converted to 100ns units

        NtTime += ((LONGLONG)(UdfTime->centiseconds) * (10 * 1000) +
                   (LONGLONG)(UdfTime->hundredsOfMicroseconds) * 100 +
                   (LONGLONG)(UdfTime->microseconds)) * 10;

        // Convert local time to UTC using timezone offset

        if (Type == TIMESTAMP_TYPE_LOCAL && HasOffset) {
            NtTime += Int32x32To64( -Zone, (60 * 10 * 1000 * 1000) );
        }

    } else {

        NtTime = 0;
    }

    return NtTime;

#undef TIMESTAMP_ZONE_MIN
#undef TIMESTAMP_ZONE_MAX
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
    if (!NtTime) return;

    LONG Zone;
    LONGLONG LocalTime;
    TIME_FIELDS TimeFields;

    ExSystemTimeToLocalTime( (PLARGE_INTEGER)&NtTime, (PLARGE_INTEGER)&LocalTime );
    RtlTimeToTimeFields( (PLARGE_INTEGER)&LocalTime, &TimeFields );

    UdfTime->hundredsOfMicroseconds = (UCHAR)((LocalTime / 1000) % 100);
    UdfTime->centiseconds = (UCHAR)((LocalTime / (100 * 1000)) % 100);
    UdfTime->microseconds = (UCHAR)((LocalTime / 10) % 100);

    UdfTime->second = (UCHAR)(TimeFields.Second);
    UdfTime->minute = (UCHAR)(TimeFields.Minute);
    UdfTime->hour = (UCHAR)(TimeFields.Hour);
    UdfTime->day = (UCHAR)(TimeFields.Day);
    UdfTime->month = (UCHAR)(TimeFields.Month);
    UdfTime->year = (USHORT)(TimeFields.Year);

    Zone = (LONG)((LocalTime - NtTime) / (600 * 1000 * 1000));
    UdfTime->typeAndTimezone = (USHORT)((TIMESTAMP_TYPE_LOCAL << 12) | (Zone & 0x0FFF));
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
    if ( (FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) &&
       !(FileDirNdx->FI_Flags & UDF_FI_FLAG_LINKED))
        return FileDirNdx->SysAttr;

    ULONG NTAttr = 0;
    ULONG attr = 0; //permissions
    USHORT Flags = 0;
    USHORT Type = 0;
    UCHAR FCharact = 0;

    if (!FileEntry) {
        if (!FileDirNdx->FileInfo)
            return 0;
        ValidateFileInfo(FileDirNdx->FileInfo);
        FileEntry = FileDirNdx->FileInfo->Dloc->FileEntry;
    }
    if (FileEntry->tagIdent == TID_FILE_ENTRY) {
        attr = ((PFILE_ENTRY)FileEntry)->permissions;
        Flags = ((PFILE_ENTRY)FileEntry)->icbTag.flags;
        Type = ((PFILE_ENTRY)FileEntry)->icbTag.fileType;
        if (((PFILE_ENTRY)FileEntry)->fileLinkCount > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    } else {
        attr = ((PEXTENDED_FILE_ENTRY)FileEntry)->permissions;
        Flags = ((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.flags;
        Type = ((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.fileType;
        if (((PEXTENDED_FILE_ENTRY)FileEntry)->fileLinkCount > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    }
    FCharact = FileDirNdx->FileCharacteristics;

    if (Flags & ICB_FLAG_SYSTEM) NTAttr |= FILE_ATTRIBUTE_SYSTEM;
    if (Flags & ICB_FLAG_ARCHIVE) NTAttr |= FILE_ATTRIBUTE_ARCHIVE;
    if ((Type == UDF_FILE_TYPE_DIRECTORY) ||
       (Type == UDF_FILE_TYPE_STREAMDIR) ||
       (FCharact & FILE_DIRECTORY)) {
        NTAttr |= FILE_ATTRIBUTE_DIRECTORY;
#ifdef UDF_DBG
    } else {
        //NTAttr |= FILE_ATTRIBUTE_NORMAL;
#endif
    }
    if (FCharact & FILE_HIDDEN) NTAttr |= FILE_ATTRIBUTE_HIDDEN;
    if ( !(attr & PERM_O_WRITE) &&
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
    UCHAR OldCharacteristics;

    NTAttr &= UDF_VALID_FILE_ATTRIBUTES;

    if (!FileEntry) {
        ASSERT(FileDirNdx);
        if (!FileDirNdx->FileInfo)
            return;
        ValidateFileInfo(FileDirNdx->FileInfo);
        FileEntry = FileDirNdx->FileInfo->Dloc->FileEntry;
        FileDirNdx->FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    }
    if (FileEntry->tagIdent == TID_FILE_ENTRY) {
        attr = &((PFILE_ENTRY)FileEntry)->permissions;
        Flags = &((PFILE_ENTRY)FileEntry)->icbTag.flags;
        Type = &((PFILE_ENTRY)FileEntry)->icbTag.fileType;
    } else {
        attr = &((PEXTENDED_FILE_ENTRY)FileEntry)->permissions;
        Flags = &((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.flags;
        Type = &((PEXTENDED_FILE_ENTRY)FileEntry)->icbTag.fileType;
    }
    FCharact = &(FileDirNdx->FileCharacteristics);
    OldCharacteristics = *FCharact;

    if ((*FCharact & FILE_DIRECTORY) ||
       (*Type == UDF_FILE_TYPE_STREAMDIR) ||
       (*Type == UDF_FILE_TYPE_DIRECTORY)) {
        *FCharact |= FILE_DIRECTORY;
        if (*Type != UDF_FILE_TYPE_STREAMDIR)
            *Type = UDF_FILE_TYPE_DIRECTORY;
        *attr |= (PERM_O_EXEC | PERM_G_EXEC | PERM_U_EXEC);
        NTAttr |= FILE_ATTRIBUTE_DIRECTORY;
        NTAttr &= ~FILE_ATTRIBUTE_NORMAL;
    } else {
        *FCharact &= ~FILE_DIRECTORY;
        *Type = UDF_FILE_TYPE_REGULAR;
        *attr &= ~(PERM_O_EXEC | PERM_G_EXEC | PERM_U_EXEC);
    }

    if (NTAttr & FILE_ATTRIBUTE_SYSTEM) {
        *Flags |= ICB_FLAG_SYSTEM;
    } else {
        *Flags &= ~ICB_FLAG_SYSTEM;
    }
    if (NTAttr & FILE_ATTRIBUTE_ARCHIVE) {
        *Flags |= ICB_FLAG_ARCHIVE;
    } else {
        *Flags &= ~ICB_FLAG_ARCHIVE;
    }
    if (NTAttr & FILE_ATTRIBUTE_HIDDEN) {
       *FCharact |= FILE_HIDDEN;
    } else {
       *FCharact &= ~FILE_HIDDEN;
    }
    *attr |= (PERM_O_READ | PERM_G_READ | PERM_U_READ);
    if (!(NTAttr & FILE_ATTRIBUTE_READONLY)) {
        *attr |= (PERM_O_WRITE  | PERM_G_WRITE  | PERM_U_WRITE |
                  PERM_O_DELETE | PERM_G_DELETE | PERM_U_DELETE |
                  PERM_O_CHATTR | PERM_G_CHATTR | PERM_U_CHATTR);
    } else {
        *attr &= ~(PERM_O_WRITE  | PERM_G_WRITE  | PERM_U_WRITE |
                   PERM_O_DELETE | PERM_G_DELETE | PERM_U_DELETE |
                   PERM_O_CHATTR | PERM_G_CHATTR | PERM_U_CHATTR);
    }
    FileDirNdx->SysAttr = NTAttr;
    if (FileDirNdx->FileInfo)
        FileDirNdx->FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    /* Only fileCharacteristics belongs to the FID. Permissions and
     * system/archive flags live in the FE; changing those must not create
     * another FID write. Preserve any pending name/ICB/characteristic update
     * already marked by create, rename, link, retag or a failed flush. */
    if (OldCharacteristics != *FCharact)
        FileDirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
    return;
} // end UDFAttributesToUDF()

/*
    This routine fills PFILE_BOTH_DIR_INFORMATION structure (NT)
 */
NTSTATUS
UDFFileDirInfoToNT(
    IN PIRP_CONTEXT IrpContext,
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
    BOOLEAN ReadSizes = FALSE;
    NTSTATUS status;
    PFCB Fcb;

    UDFPrint(("@=%#x, FileDirNdx %x\n", &Vcb, FileDirNdx));

    ASSERT((ULONG_PTR)NTFileInfo > 0x1000);
    RtlZeroMemory(NTFileInfo, sizeof(FILE_BOTH_DIR_INFORMATION));

    DosName.Buffer = (PWCHAR)&(NTFileInfo->ShortName);
    DosName.MaximumLength = sizeof(NTFileInfo->ShortName); // 12*sizeof(WCHAR)

    _SEH2_TRY {
        UDFPrint(("  DirInfoToNT: %wZ\n", &FileDirNdx->FName));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        UDFPrint(("  DirInfoToNT: exception when printing file name\n"));
    } _SEH2_END;

    if (FileDirNdx->FileInfo) {
        UDFPrint(("    FileInfo\n"));
        // validate FileInfo
        ValidateFileInfo(FileDirNdx->FileInfo);
        if (UDFGetFileNameLinkCount(FileDirNdx->FileInfo) > 1)
            FileDirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
        FileEntry = (PFILE_ENTRY)(FileDirNdx->FileInfo->Dloc->FileEntry);
        // read required sizes from Fcb (if any) if file is not linked
        // otherwise we should read them from FileEntry
        if (FileDirNdx->FileInfo->Fcb) {
            UDFPrint(("    Fcb\n"));
            Fcb = FileDirNdx->FileInfo->Fcb;
            NTFileInfo->CreationTime.QuadPart   = Fcb->CreationTime.QuadPart;
            NTFileInfo->LastWriteTime.QuadPart  = Fcb->LastWriteTime.QuadPart;
            NTFileInfo->LastAccessTime.QuadPart = Fcb->LastAccessTime.QuadPart;
            NTFileInfo->ChangeTime.QuadPart     = Fcb->ChangeTime.QuadPart;

            NTFileInfo->AllocationSize.QuadPart = UDFGetFileAllocationSize(Vcb, FileDirNdx->FileInfo);
            NTFileInfo->EndOfFile.QuadPart = UDFGetFileSize(FileDirNdx->FileInfo);

            if (FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) {
                UDFPrint(("    SYS_ATTR\n"));
                NTFileInfo->FileAttributes = FileDirNdx->SysAttr;
                goto get_name_only;
            }

            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart;
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart;
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart;
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart;
            FileDirNdx->AllocationSize = NTFileInfo->AllocationSize.QuadPart;
            FileDirNdx->FileSize       = NTFileInfo->EndOfFile.QuadPart;

            goto get_attr_only;
        }
        ASSERT(FileEntry);
    } else if (!(FileDirNdx->FI_Flags & UDF_FI_FLAG_SYS_ATTR) ||
               (FileDirNdx->FI_Flags & UDF_FI_FLAG_LINKED)) {
        LONG_AD feloc;

        UDFPrint(("  !SYS_ATTR\n"));
        FileEntry = (PFILE_ENTRY)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
        if (!FileEntry) return STATUS_INSUFFICIENT_RESOURCES;

        feloc.extLength = Vcb->SectorSize;
        feloc.extLocation = FileDirNdx->FileEntryLoc;

        if (!NT_SUCCESS(status = UDFReadFileEntry(IrpContext, Vcb, &feloc, FileEntry, &Ident))) {
            UDFPrint(("    !UDFReadFileEntry\n"));
            MyFreePool__(FileEntry);
            FileEntry = NULL;
            goto get_name_only;
        }
        ReadSizes = TRUE;
    } else {
        UDFPrint(("  FileDirNdx\n"));
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

    UDFPrint(("  direct\n"));
    if (FileEntry->descTag.tagIdent == TID_FILE_ENTRY) {
        UDFPrint(("  TID_FILE_ENTRY\n"));
        if (ReadSizes) {
            UDFPrint(("    ReadSizes\n"));
            // Times
            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart   =
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart  = UDFTimeToNT(&(FileEntry->modificationTime));
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = UDFTimeToNT(&(FileEntry->accessTime));
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart     = UDFTimeToNT(&(FileEntry->attrTime));
            // FileSize
            FileDirNdx->FileSize =
            NTFileInfo->EndOfFile.QuadPart =
                FileEntry->informationLength;
            UDFPrint(("    informationLength=%I64x, lengthAllocDescs=%lx\n",
                FileEntry->informationLength,
                (ULONG)FileEntry->lengthAllocDescs
                ));
            // AllocSize
            FileDirNdx->AllocationSize =
            NTFileInfo->AllocationSize.QuadPart =
                (FileEntry->informationLength + Vcb->SectorSize - 1) & ~((LONGLONG)(Vcb->SectorSize) - 1);
        }
//        NTFileInfo->EaSize = 0;//FileEntry->lengthExtendedAttr;
    } else if (FileEntry->descTag.tagIdent == TID_EXTENDED_FILE_ENTRY) {
        ExFileEntry = (PEXTENDED_FILE_ENTRY)FileEntry;
        UDFPrint(("  PEXTENDED_FILE_ENTRY\n"));
        if (ReadSizes) {
            UDFPrint(("    ReadSizes\n"));
            // Times
            FileDirNdx->CreationTime   = NTFileInfo->CreationTime.QuadPart   = UDFTimeToNT(&(ExFileEntry->createTime));
            FileDirNdx->LastWriteTime  = NTFileInfo->LastWriteTime.QuadPart  = UDFTimeToNT(&(ExFileEntry->modificationTime));
            FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = UDFTimeToNT(&(ExFileEntry->accessTime));
            FileDirNdx->ChangeTime     = NTFileInfo->ChangeTime.QuadPart     = UDFTimeToNT(&(ExFileEntry->attrTime));
            // FileSize
            FileDirNdx->FileSize =
            NTFileInfo->EndOfFile.QuadPart =
                ExFileEntry->informationLength;
            UDFPrint(("    informationLength=%I64x, lengthAllocDescs=%lx\n",
                ExFileEntry->informationLength,
                (ULONG)ExFileEntry->lengthAllocDescs
                ));
            // AllocSize
            FileDirNdx->AllocationSize =
            NTFileInfo->AllocationSize.QuadPart =
                (ExFileEntry->informationLength + Vcb->SectorSize - 1) & ~((LONGLONG)(Vcb->SectorSize) - 1);
        }
//        NTFileInfo->EaSize = 0;//ExFileEntry->lengthExtendedAttr;
    } else {
        UDFPrint(("  ???\n"));
        goto get_name_only;
    }

get_attr_only:

    UDFPrint(("  get_attr"));
    // do some substitutions
    if (!FileDirNdx->CreationTime) {
        FileDirNdx->CreationTime = NTFileInfo->CreationTime.QuadPart = Vcb->VolCreationTime;
    }
    if (!FileDirNdx->LastAccessTime) {
        FileDirNdx->LastAccessTime = NTFileInfo->LastAccessTime.QuadPart = FileDirNdx->CreationTime;
    }
    if (!FileDirNdx->LastWriteTime) {
        FileDirNdx->LastWriteTime = NTFileInfo->LastWriteTime.QuadPart = FileDirNdx->CreationTime;
    }
    if (!FileDirNdx->ChangeTime) {
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
    if (!(FileDirNdx->FI_Flags & UDF_FI_FLAG_DOS)) {
        UDFPrint(("  !UDF_FI_FLAG_DOS"));
        UDFGetDirNdxShortName(Vcb, FileDirNdx,
            (FileDirNdx->FI_Flags & UDF_FI_FLAG_KEEP_NAME) ? TRUE : FALSE, &DosName);
        NTFileInfo->ShortNameLength = (UCHAR)DosName.Length;
    }
    // report zero EOF & AllocSize for Dirs
    if (FileDirNdx->FileCharacteristics & FILE_DIRECTORY) {
        UDFPrint(("  FILE_DIRECTORY"));
        NTFileInfo->AllocationSize.QuadPart =
        NTFileInfo->EndOfFile.QuadPart = 0;
    }
    UDFPrint(("  AllocationSize=%I64x, NTFileInfo->EndOfFile=%I64x", NTFileInfo->AllocationSize.QuadPart, NTFileInfo->EndOfFile.QuadPart));
    // free tmp buffer (if any)
    UDFPrint(("\n"));
    if (FileEntry && !FileDirNdx->FileInfo)
        MyFreePool__(FileEntry);
    return STATUS_SUCCESS;
} // end UDFFileDirInfoToNT()

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

    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if (AccTime) {
            if (DirNdx && *AccTime) DirNdx->LastAccessTime = *AccTime;
            UDFTimeToUDF(*AccTime, &(fe->accessTime));
        }
        if (AttrTime) {
            if (DirNdx && *AttrTime) DirNdx->ChangeTime = *AttrTime;
            UDFTimeToUDF(*AttrTime, &(fe->attrTime));
        }
        if (ChgTime) {
            if (DirNdx && *ChgTime) DirNdx->CreationTime =
                DirNdx->LastWriteTime = *ChgTime;
            UDFTimeToUDF(*ChgTime, &(fe->modificationTime));
        } else
        if (CrtTime) {
            if (DirNdx && *CrtTime) DirNdx->CreationTime =
                DirNdx->LastWriteTime = *CrtTime;
            UDFTimeToUDF(*CrtTime, &(fe->modificationTime));
        }

    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if (AccTime) {
            if (DirNdx && *AccTime) DirNdx->LastAccessTime = *AccTime;
            UDFTimeToUDF(*AccTime, &(fe->accessTime));
        }
        if (AttrTime) {
            if (DirNdx && *AttrTime) DirNdx->ChangeTime = *AttrTime;
            UDFTimeToUDF(*AttrTime, &(fe->attrTime));
        }
        if (ChgTime) {
            if (DirNdx && *ChgTime) DirNdx->LastWriteTime = *ChgTime;
            UDFTimeToUDF(*ChgTime, &(fe->modificationTime));
        }
        if (CrtTime) {
            if (DirNdx && *CrtTime) DirNdx->CreationTime = *CrtTime;
            UDFTimeToUDF(*CrtTime, &(fe->createTime));
        }

    }
} // end UDFSetFileXTime()

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

    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if (AccTime) *AccTime = UDFTimeToNT(&(fe->accessTime));
        if (AttrTime) *AttrTime = UDFTimeToNT(&(fe->attrTime));
        if (ChgTime) *ChgTime = UDFTimeToNT(&(fe->modificationTime));
        if (CrtTime) {
            (*CrtTime) = *ChgTime;
        }

    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);

        if (AccTime) *AccTime = UDFTimeToNT(&(fe->accessTime));
        if (AttrTime) *AttrTime = UDFTimeToNT(&(fe->attrTime));
        if (ChgTime) *ChgTime = UDFTimeToNT(&(fe->modificationTime));
        if (CrtTime) *CrtTime = UDFTimeToNT(&(fe->createTime));

    }
    if (CrtTime) {
        if (!(*CrtTime))
            KeQuerySystemTime((PLARGE_INTEGER)CrtTime);
        if (AccTime && !(*AccTime)) (*AccTime) = *CrtTime;
        if (AttrTime && !(*AttrTime)) (*AttrTime) = *CrtTime;
        if (AccTime && !(*AccTime)) (*AccTime) = *CrtTime;
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
    if (!len) return;
    if (!buffer[len-1]) {
        FName->Length-=sizeof(WCHAR);
        len--;
    }
    if (!len) return;
    if (buffer[0] == UNICODE_PERIOD) {
        if (len == 1) return;
        if ((buffer[1] == UNICODE_PERIOD) && (len == 2)) return;
    }

    // check for trailing '.'
    for(len--;len;len--) {
        if ( ((buffer[len] == UNICODE_PERIOD) || (buffer[len] == UNICODE_SPACE)) ) {
            FName->Length-=sizeof(WCHAR);
            buffer[len] = 0;
        } else
            break;
    }
} // end UDFNormalizeFileName()

NTSTATUS
UDFDoesOSAllowFileToBeTargetForRename__(
    IN PUDF_FILE_INFO FileInfo
    )
{
    NTSTATUS RC;

    if (UDFIsADirectory(FileInfo))
        return STATUS_ACCESS_DENIED;
    if (!FileInfo->ParentFile)
        return STATUS_ACCESS_DENIED;

    if (UDFAttributesToNT(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index),
                         FileInfo->Dloc->FileEntry) & FILE_ATTRIBUTE_READONLY)
        return STATUS_ACCESS_DENIED;

    if (!FileInfo->Fcb)
        return STATUS_SUCCESS;

    RC = UDFCheckAccessRights(NULL, NULL, FileInfo->Fcb, NULL, DELETE, 0);
    if (!NT_SUCCESS(RC))
        return RC;

    if (!FileInfo->Fcb)
        return STATUS_SUCCESS;
//    RC = UDFMarkStreamsForDeletion(FileInfo->Fcb->Vcb, FileInfo->Fcb, TRUE); // Delete
/*    RC = UDFSetDispositionInformation(FileInfo->Fcb, NULL,
                FileInfo->Fcb->Vcb, NULL, TRUE);
    if (NT_SUCCESS(RC)) {
        FileInfo->Fcb->FCBFlags |= UDF_FCB_DELETED;
        if (UDFGetFileLinkCount(FileInfo) <= 1) {
            FileInfo->Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_DELETED;
        }
    }*/
    return RC;

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

    if (!FileInfo->ParentFile)
        return STATUS_CANNOT_DELETE;
    if (FileInfo->Dloc->SDirInfo)
        return STATUS_CANNOT_DELETE;
    if (!UDFIsADirectory(FileInfo))
        return STATUS_SUCCESS;

//    UDFFlushAFile(FileInfo->Fcb, NULL, &IoStatus, 0);
    hCurDirNdx = FileInfo->Dloc->DirIndex;
    // check if we can delete all files
    for(i=2; (CurDirNdx = UDFDirIndex(hCurDirNdx,i)); i++) {
        // try to open Stream
        if (CurDirNdx->FileInfo)
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
    if (!hDirNdx) return STATUS_CANNOT_DELETE;
    PDIR_INDEX_ITEM DirNdx = UDFDirIndex(hDirNdx, FileInfo->Index);
    if (!DirNdx) return STATUS_CANNOT_DELETE;
    // we can't hide file that is not marked as deleted
    if (!(DirNdx->FileCharacteristics & FILE_DELETED)) {
        BrutePoint();

        if (!(FileInfo->Fcb->FcbState & (UDF_FCB_DELETE_ON_CLOSE |
                                        UDF_FCB_DELETED) )) {

            return STATUS_CANNOT_DELETE;
        }
    }
    return STATUS_SUCCESS;
}

