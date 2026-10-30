////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __UDF_NAME_SUP__H__
#define __UDF_NAME_SUP__H__

VOID
UDFDissectName(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PUNICODE_STRING RemainingName,
    _Out_ PUNICODE_STRING FinalName,
    _Out_ PBOOLEAN IsStream
    );

extern BOOLEAN UDFIsNameInExpression(IN PVCB Vcb,
                                     IN PUNICODE_STRING FileName,
                                     IN PUNICODE_STRING PtrSearchPattern,
                                     OUT PBOOLEAN DosOpen,
                                     IN BOOLEAN IgnoreCase,
                                     IN BOOLEAN ContainsWC,
                                     IN BOOLEAN CanBe8dot3,
                                     IN BOOLEAN KeepIntact,
                                     IN PDIR_INDEX_ITEM DirNdx OPTIONAL);

extern BOOLEAN UDFDoesNameContainWildCards(IN PUNICODE_STRING SearchPattern);

BOOLEAN
UDFIsNameValid(
    IN PUNICODE_STRING SearchPattern,
    OUT BOOLEAN* StreamOpen,
    OUT ULONG* SNameIndex
    );

extern BOOLEAN UDFIsMatchAllMask(IN PUNICODE_STRING Name,
                                 OUT BOOLEAN* DosOpen);

extern BOOLEAN UDFCanNameBeA8dot3(IN PUNICODE_STRING Name);

NTSTATUS
UDFGetFileNameFromFileInfo(
    IN PUDF_FILE_INFO FileInfo,
    OUT PUNICODE_STRING FileName
    );

NTSTATUS
UDFUpcaseString(
    OUT PUNICODE_STRING DestName,
    IN PUNICODE_STRING SourceName
    );

NTSTATUS
UDFGenerateShortName(
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    IN PDIR_INDEX_ITEM DirNdx OPTIONAL,
    OUT PUNICODE_STRING ShortName
    );

BOOLEAN
UDFNeedsShortName(
    IN PUNICODE_STRING Name
    );

VOID
UDFSetDirNdxShortName(
    IN PDIR_INDEX_ITEM DirNdx,
    IN PUNICODE_STRING ShortName OPTIONAL
    );

VOID
UDFGetDirNdxShortName(
    IN PVCB Vcb,
    IN PDIR_INDEX_ITEM DirNdx,
    IN BOOLEAN KeepIntact,
    IN OUT PUNICODE_STRING ShortName
    );

NTSTATUS
UDFGenerateUniqueShortName(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PUNICODE_STRING LongName,
    IN OUT PUNICODE_STRING ShortName
    );

// Size of the alias record in a FID's Implementation Use area: an
// EntityID (32 bytes), version, length and 12 characters, DWORD-aligned.
#define UDF_SHORT_NAME_IMPUSE_LEN 48

BOOLEAN
UDFWriteShortNameImpUse(
    IN PFILE_IDENT_DESC FileId,
    IN PUNICODE_STRING ShortName
    );

BOOLEAN
UDFReadShortNameImpUse(
    IN PFILE_IDENT_DESC FileId,
    IN OUT PUNICODE_STRING ShortName
    );

VOID
UDFLoadDirNdxShortName(
    IN PDIR_INDEX_ITEM DirNdx,
    IN PFILE_IDENT_DESC FileId OPTIONAL
    );

VOID
UDFAssignMissingShortNames(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    );

#endif //__UDF_NAME_SUP__H__
