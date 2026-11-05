////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
    Module:
            Namesup.cpp

    Abstract: FileName support routines
*/

#include "udffs.h"

VOID
UDFDissectName(
    _In_ PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING FinalName,
    OUT PBOOLEAN IsStream
    )
{
    ULONG NameLength;
    PWCHAR NextWchar;

    //
    //  Check for stream prefix (':')
    //
    if (RemainingName->Length >= sizeof(WCHAR) &&
        RemainingName->Buffer[0] == L':') {

        *IsStream = TRUE;

        // Skip the ':' prefix
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    } else {

        *IsStream = FALSE;

        //
        //  Skip leading backslashes (only for non-stream paths)
        //
        while (RemainingName->Length >= sizeof(WCHAR) &&
               RemainingName->Buffer[0] == L'\\') {
            RemainingName->Buffer++;
            RemainingName->Length -= sizeof(WCHAR);
            RemainingName->MaximumLength -= sizeof(WCHAR);
        }
    }

    //
    //  Find the offset of the next component separator (backslash or colon).
    //  Use Length for bounds checking - do NOT rely on null terminator!
    //
    for (NameLength = 0, NextWchar = RemainingName->Buffer;
         (NameLength < RemainingName->Length) && (*NextWchar != L'\\') && (*NextWchar != L':');
         NameLength += sizeof(WCHAR), NextWchar++);

    //
    //  Set FinalName to the extracted component
    //
    FinalName->Buffer = RemainingName->Buffer;
    FinalName->MaximumLength = FinalName->Length = (USHORT)NameLength;

    //
    //  Adjust RemainingName past the component.
    //  For ':' separator: do NOT consume it (keep in RemainingName for stream suffix).
    //  For '\' separator: consume it.
    //
    if (NameLength == RemainingName->Length) {
        // Last component — nothing left
        RemainingName->Length = 0;
    } else if (RemainingName->Buffer[NameLength / sizeof(WCHAR)] == L':') {
        // ':' separator — keep it in RemainingName
        RemainingName->MaximumLength -= (USHORT)NameLength;
        RemainingName->Length -= (USHORT)NameLength;
        RemainingName->Buffer = (PWCHAR)((PCHAR)RemainingName->Buffer + NameLength);
    } else {
        // '\' separator — consume it
        RemainingName->MaximumLength -= (USHORT)(NameLength + sizeof(WCHAR));
        RemainingName->Length -= (USHORT)(NameLength + sizeof(WCHAR));
        RemainingName->Buffer = (PWCHAR)((PCHAR)RemainingName->Buffer + NameLength + sizeof(WCHAR));
    }

} // end UDFDissectName()

BOOLEAN
UDFIsNameValid(
    IN PUNICODE_STRING SearchPattern,
    OUT BOOLEAN* StreamOpen,
    OUT ULONG* SNameIndex
) {
    LONG   Index, l;
    BOOLEAN _StreamOpen = FALSE;
    PWCHAR Buffer;
    WCHAR c, c0;

    if (StreamOpen) (*StreamOpen) = FALSE;
    // We can't create nameless file or too long path
    if (!(l = SearchPattern->Length/sizeof(WCHAR)) ||
        (l>UDF_X_PATH_LEN)) return FALSE;
    Buffer = SearchPattern->Buffer;
    for(Index = 0; Index<l; Index++, Buffer++) {
        // Check for disallowed characters
        c = (*Buffer);
        if ((c == L'*') ||
           (c == L'>') ||
           (c == L'\"') ||
           (c == L'/') ||
           (c == L'<') ||
           (c == L'|') ||
           ((c >= 0x0000) && (c <= 0x001f)) ||
           (c == L'?')) return FALSE;
        // check if this a Stream path (& validate it)
        if (!(_StreamOpen) && // sub-streams are not allowed
            (Index<(l-1)) && // stream name must be specified
           ((_StreamOpen) = (c == L':'))) {
            if (StreamOpen) (*StreamOpen) = TRUE;
            if (SNameIndex) (*SNameIndex) = Index;
        }
        // According to NT IFS documentation neither SPACE nor DOT can be
        // a trailing character
        if (Index && (c == L'\\') ) {
           if ((c0 == L' ') ||
              (_StreamOpen) || // stream is not a directory
              (c0 == L'.')) return FALSE;
        }
        c0 = c;
    }
    // According to NT IFS documentation neither SPACE nor DOT nor COLONS can be
    // a trailing character
    if (c0 == L' ' ||
        c0 == L':' ||
        c0 == L'.') {

        return FALSE;
    }

    return TRUE;
} // end UDFIsNameValid()


/*

Routine Description:

    This routine will compare two Unicode strings.
    PtrSearchPattern may contain wildcards

Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

*/
BOOLEAN
UDFIsNameInExpression(
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING PtrSearchPattern,
    OUT PBOOLEAN DosOpen,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ContainsWC,
    IN BOOLEAN CanBe8dot3,
    IN BOOLEAN KeepIntact, // passed to UDFGetDirNdxShortName
    IN PDIR_INDEX_ITEM DirNdx OPTIONAL // supplies a stored 8.3 alias
    )
{
    BOOLEAN             Match = TRUE;
    UNICODE_STRING      ShortName;
    WCHAR               Buffer[13];

    if (!PtrSearchPattern) return TRUE;
    // we try to open file by LFN by default
    if (DosOpen) (*DosOpen) = FALSE;
    //  If there are wildcards in the expression then we call the
    //  appropriate FsRtlRoutine.
    if (ContainsWC) {
        Match = FsRtlIsNameInExpression( PtrSearchPattern, FileName, IgnoreCase, NULL );
    //  Otherwise do a direct memory comparison for the name string.
    } else if (RtlCompareUnicodeString(FileName, PtrSearchPattern, IgnoreCase)) {
        Match = FALSE;
    }

    if (Match) return TRUE;

    // check if SFN can match this pattern
    if (!CanBe8dot3)
        return FALSE;

    // try to open by SFN
    ShortName.Buffer = (PWCHAR)(&Buffer);
    ShortName.MaximumLength = 13*sizeof(WCHAR);
    if (!DirNdx)
        return FALSE;
    UDFGetDirNdxShortName(Vcb, DirNdx, KeepIntact, &ShortName);
    if (!ShortName.Length)
        return FALSE;

    // PtrSearchPattern is upcased if we are called with IgnoreCase=TRUE
    // the alias is always upcased
    // thus, we can use case-sensetive compare here to improve performance
    if (ContainsWC) {
        Match = FsRtlIsNameInExpression( PtrSearchPattern, &ShortName, FALSE, NULL );
    //  Otherwise do a direct memory comparison for the name string.
    } else if (!RtlCompareUnicodeString(&ShortName, PtrSearchPattern, FALSE)) {
        Match = TRUE;
    }
    if (DosOpen && Match) {
        // remember that we've opened file by SFN
        (*DosOpen) = TRUE;
    }
    return Match;
} // end UDFIsNameInExpression()


BOOLEAN
UDFIsMatchAllMask(
    IN PUNICODE_STRING Name,
   OUT BOOLEAN* DosOpen
    )
{
    USHORT i;
    PWCHAR Buffer;

    if (DosOpen)
        *DosOpen = FALSE;
    Buffer = Name->Buffer;
    if (Name->Length == sizeof(WCHAR)) {
        // Win32-style wildcard
        if ((*Buffer) != L'*')
            return FALSE;
        return TRUE;
    } else
    if (Name->Length == sizeof(WCHAR)*(8+1+3)) {
        // DOS-style wildcard
        for(i=0;i<8;i++,Buffer++) {
            if ((*Buffer) != DOS_QM)
                return FALSE;
        }
        if ((*Buffer) != DOS_DOT)
            return FALSE;
        Buffer++;
        for(i=9;i<12;i++,Buffer++) {
            if ((*Buffer) != DOS_QM)
                return FALSE;
        }
        if (DosOpen)
            *DosOpen = TRUE;
        return TRUE;
    } else
    if (Name->Length == sizeof(WCHAR)*(3)) {
        // DOS-style wildcard
        if (Buffer[0] != DOS_STAR)
            return FALSE;
        if (Buffer[1] != DOS_DOT)
            return FALSE;
        if (Buffer[2] != DOS_STAR)
            return FALSE;
        if (DosOpen)
            *DosOpen = TRUE;
        return TRUE;
    } else {
        return FALSE;
    }
} // end UDFIsMatchAllMask()

BOOLEAN
UDFCanNameBeA8dot3(
    IN PUNICODE_STRING Name
    )
{
    if (Name->Length >= 13 * sizeof(WCHAR))
        return FALSE;

    ULONG i,l;
    ULONG dot_pos=0;
    ULONG ext_len=0;
    PWCHAR buff = Name->Buffer;

    l = Name->Length / sizeof(WCHAR);

    for(i=0; i<l; i++, buff++) {
        if ( ((*buff) == L'.') ||
            ((*buff) == DOS_DOT) ) {
            if (dot_pos)
                return FALSE;
            dot_pos = i+1;
        } else
        if (dot_pos) {
            ext_len++;
            if (ext_len > 3)
                return FALSE;
        } else
        if (i >= 8) {
            return FALSE;
        }
    }
    return TRUE;
} // end UDFCanNameBeA8dot3()

/*
    Extracts the file name from FileInfo's FileIdent descriptor.

    This function:
    1. Gets the pointer to compressed name in FileIdent
    2. Decompresses it using UDFDecompressUnicode
    3. Returns the allocated UNICODE_STRING

    The caller is responsible for freeing FileName->Buffer using MyFreePool__ or ExFreePool.

    Parameters:
        FileInfo - Pointer to UDF_FILE_INFO structure
        FileName - Output UNICODE_STRING structure (caller provides, we allocate Buffer)

    Returns:
        STATUS_SUCCESS - Name extracted successfully
        STATUS_INVALID_PARAMETER - FileInfo or FileIdent is NULL
        STATUS_INSUFFICIENT_RESOURCES - Memory allocation failed
*/
NTSTATUS
UDFGetFileNameFromFileInfo(
    IN PUDF_FILE_INFO FileInfo,
    OUT PUNICODE_STRING FileName
    )
{
    PFILE_IDENT_DESC FileIdent;
    uint8* CompressedName;
    SIZE_T NameLength;

    // Validate parameters
    if (!FileInfo || !FileName) {
        return STATUS_INVALID_PARAMETER;
    }

    FileIdent = FileInfo->FileIdent;
    if (!FileIdent) {
        // No FileIdent - this might be root directory or special case
        // Return empty string
        FileName->Buffer = NULL;
        FileName->Length = 0;
        FileName->MaximumLength = 0;
        return STATUS_SUCCESS;
    }

    // Get name length from FileIdent
    NameLength = FileIdent->lengthFileIdent;
    if (NameLength == 0) {
        // Empty name - return empty string
        FileName->Buffer = NULL;
        FileName->Length = 0;
        FileName->MaximumLength = 0;
        return STATUS_SUCCESS;
    }

    // Get pointer to compressed name
    // Name is stored after FileIdent structure + ImpUse field
    CompressedName = ((uint8*)(FileIdent + 1)) + FileIdent->lengthOfImpUse;

    // Decompress the name
    // UDFDecompressUnicode allocates buffer and fills FileName
    UDFDecompressUnicode(FileName, CompressedName, NameLength, NULL);

    if (!FileName->Buffer) {
        // Decompression failed
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
} // end UDFGetFileNameFromFileInfo()

/*
    Creates an uppercase copy of a Unicode string.

    This function allocates a buffer and converts the source string to uppercase
    for case-insensitive comparisons.

    The caller is responsible for freeing DestName->Buffer using ExFreePool.

    Parameters:
        DestName - Output UNICODE_STRING (caller provides, we allocate Buffer)
        SourceName - Input string to uppercase

    Returns:
        STATUS_SUCCESS - String converted successfully
        STATUS_INVALID_PARAMETER - Invalid parameters
        STATUS_INSUFFICIENT_RESOURCES - Memory allocation failed
*/
NTSTATUS
UDFUpcaseString(
    OUT PUNICODE_STRING DestName,
    IN PUNICODE_STRING SourceName
    )
{
    PWCHAR Buffer;
    ULONG i;

    // Validate parameters
    if (!DestName || !SourceName || !SourceName->Buffer) {
        return STATUS_INVALID_PARAMETER;
    }

    // Handle empty string
    if (SourceName->Length == 0) {
        DestName->Buffer = NULL;
        DestName->Length = 0;
        DestName->MaximumLength = 0;
        return STATUS_SUCCESS;
    }

    // Allocate buffer for uppercase string
    Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                           SourceName->Length + sizeof(WCHAR),
                                           TAG_FILE_NAME);
    if (!Buffer) {
        DestName->Buffer = NULL;
        DestName->Length = 0;
        DestName->MaximumLength = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Copy and uppercase each character
    for (i = 0; i < SourceName->Length / sizeof(WCHAR); i++) {
        Buffer[i] = RtlUpcaseUnicodeChar(SourceName->Buffer[i]);
    }

    // Null terminate
    Buffer[i] = 0;

    // Set output string
    DestName->Buffer = Buffer;
    DestName->Length = SourceName->Length;
    DestName->MaximumLength = SourceName->Length + sizeof(WCHAR);

    return STATUS_SUCCESS;
} // end UDFUpcaseString()

/*
    Generates an 8.3 (DOS) short name from a long file name.

    Returns the Windows-style alias kept in the directory index entry.

    The caller is responsible for freeing ShortName->Buffer using ExFreePool.

    Parameters:
        Vcb - Volume control block (for UDF revision)
        FileName - Long file name to convert
        ShortName - Output short name (8.3 format)

    Returns:
        STATUS_SUCCESS - Short name generated successfully
        STATUS_INVALID_PARAMETER - Invalid parameters
        STATUS_INSUFFICIENT_RESOURCES - Memory allocation failed
*/
NTSTATUS
UDFGenerateShortName(
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    IN PDIR_INDEX_ITEM DirNdx OPTIONAL,
    OUT PUNICODE_STRING ShortName
    )
{
    PWCHAR Buffer;

    // Validate parameters
    if (!Vcb || !FileName || !ShortName || !FileName->Buffer) {
        return STATUS_INVALID_PARAMETER;
    }

    // Handle empty string
    if (FileName->Length == 0) {
        ShortName->Buffer = NULL;
        ShortName->Length = 0;
        ShortName->MaximumLength = 0;
        return STATUS_SUCCESS;
    }

    // Allocate buffer for 8.3 name (max 12 chars + null terminator)
    // Format: 8 chars + '.' + 3 chars = 12 + 1
    Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                           13 * sizeof(WCHAR),
                                           TAG_FILE_NAME);
    if (!Buffer) {
        ShortName->Buffer = NULL;
        ShortName->Length = 0;
        ShortName->MaximumLength = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set up output string structure
    ShortName->Buffer = Buffer;
    ShortName->MaximumLength = 13 * sizeof(WCHAR);

    // Use the entry's stored alias when there is one; otherwise the UDF
    // translation (KeepIntact = FALSE generates a proper 8.3 name).
    if (DirNdx)
        UDFGetDirNdxShortName(Vcb, DirNdx, FALSE, ShortName);
    else
        ShortName->Length = 0;

    // If generation failed, Length will be 0
    if (ShortName->Length == 0) {
        // Failed to generate short name
        ExFreePool(Buffer);
        ShortName->Buffer = NULL;
        ShortName->MaximumLength = 0;
        return STATUS_OBJECT_NAME_INVALID;
    }

    return STATUS_SUCCESS;
} // end UDFGenerateShortName()


/*************************************************************************
*
* Windows-style (NTFS/FAT) 8.3 aliases
*
* A name that is already a legal 8.3 name has no alias. Any other name
* gets one generated by RtlGenerate8dot3Name (DOCUME~1, ADMINI~1, ...),
* unique within its directory. The alias of a new entry is stored in its
* FID. Entries written before aliases were stored get one in memory when
* their directory is read; nothing is written for them.
*
*************************************************************************/

BOOLEAN
UDFNeedsShortName(
    IN PUNICODE_STRING Name
    )
{
    BOOLEAN SpacesFound = FALSE;

    if (!Name || !Name->Buffer || !Name->Length)
        return FALSE;
    return !RtlIsNameLegalDOS8Dot3(Name, NULL, &SpacesFound) || SpacesFound;
}

/* Cache an entry's alias and keep its DOS hash in step with it, so the
 * existing hash prefilters in lookup and enumeration stay valid. */
VOID
UDFSetDirNdxShortName(
    IN PDIR_INDEX_ITEM DirNdx,
    IN PUNICODE_STRING ShortName OPTIONAL
    )
{
    UNICODE_STRING Upcased;
    WCHAR Buffer[12];

    if (!ShortName || !ShortName->Length ||
        ShortName->Length > sizeof(DirNdx->ShortName)) {
        DirNdx->ShortNameLength = 0;
        DirNdx->FI_Flags |= UDF_FI_FLAG_SHORT_NAME | UDF_FI_FLAG_DOS;
        return;
    }
    Upcased.Buffer = Buffer;
    Upcased.Length = 0;
    Upcased.MaximumLength = sizeof(Buffer);
    RtlUpcaseUnicodeString(&Upcased, ShortName, FALSE);
    RtlCopyMemory(DirNdx->ShortName, Upcased.Buffer, Upcased.Length);
    DirNdx->ShortNameLength = (uint8)Upcased.Length;
    DirNdx->hashes.hDos = crc32((uint8*)Upcased.Buffer, Upcased.Length);
    DirNdx->FI_Flags |= UDF_FI_FLAG_SHORT_NAME;
    DirNdx->FI_Flags &= ~UDF_FI_FLAG_DOS;
}

/* Returns the entry's 8.3 name in ShortName (12 WCHARs of storage). For a
 * name that needs no alias this is the upcased long name itself. */
VOID
UDFGetDirNdxShortName(
    IN PVCB Vcb,
    IN PDIR_INDEX_ITEM DirNdx,
    IN BOOLEAN KeepIntact,
    IN OUT PUNICODE_STRING ShortName
    )
{
    if ((DirNdx->FI_Flags & UDF_FI_FLAG_SHORT_NAME) && DirNdx->ShortNameLength &&
        DirNdx->ShortNameLength <= ShortName->MaximumLength) {
        RtlCopyMemory(ShortName->Buffer, DirNdx->ShortName, DirNdx->ShortNameLength);
        ShortName->Length = DirNdx->ShortNameLength;
        return;
    }
    /* No alias: the long name is a valid 8.3 name, or none could be made. */
    UNREFERENCED_PARAMETER(Vcb);
    UNREFERENCED_PARAMETER(KeepIntact);
    ShortName->Length = 0;
}

/* TRUE if Candidate equals the long name or the 8.3 name of a live entry
 * in DirInfo. The name hashes filter the scan, so building aliases for a
 * whole directory stays cheap. */
static BOOLEAN
UDFShortNameInUse(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PUNICODE_STRING Candidate
    )
{
    UDF_DIR_SCAN_CONTEXT ScanContext;
    PDIR_INDEX_ITEM DirNdx;
    UNICODE_STRING Existing, Upcased;
    WCHAR Buffer[13];
    WCHAR UpcasedBuffer[12];
    uint32 Hash;

    if (Candidate->Length > sizeof(UpcasedBuffer))
        return TRUE;
    Upcased.Buffer = UpcasedBuffer;
    Upcased.Length = 0;
    Upcased.MaximumLength = sizeof(UpcasedBuffer);
    RtlUpcaseUnicodeString(&Upcased, Candidate, FALSE);
    Hash = crc32((uint8*)Upcased.Buffer, Upcased.Length);

    if (!UDFDirIndexInitScan(DirInfo, &ScanContext, 2))
        return FALSE;
    while ((DirNdx = UDFDirIndexScan(&ScanContext, NULL))) {
        if (!DirNdx->FName.Buffer || UDFIsDeleted(DirNdx))
            continue;
        if (DirNdx->hashes.hLfn == Hash &&
            RtlEqualUnicodeString(&DirNdx->FName, Candidate, TRUE))
            return TRUE;
        if ((DirNdx->FI_Flags & UDF_FI_FLAG_DOS) || DirNdx->hashes.hDos != Hash)
            continue;
        Existing.Buffer = Buffer;
        Existing.Length = 0;
        Existing.MaximumLength = sizeof(Buffer);
        UDFGetDirNdxShortName(Vcb, DirNdx, FALSE, &Existing);
        if (RtlEqualUnicodeString(&Existing, Candidate, TRUE))
            return TRUE;
    }
    return FALSE;
}

#define UDF_SHORT_NAME_BATCH 16

/* Generate the Windows-style alias for LongName in DirInfo. ShortName must
 * provide 12 WCHARs. Length 0 means LongName needs no alias.
 * The first candidates are checked against the directory in one pass, so
 * a name that collides with many others (~1, ~2, ... then checksum names)
 * costs one directory scan instead of one per candidate. */
NTSTATUS
UDFGenerateUniqueShortName(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PUNICODE_STRING LongName,
    IN OUT PUNICODE_STRING ShortName
    )
{
    GENERATE_NAME_CONTEXT Context;
    WCHAR Names[UDF_SHORT_NAME_BATCH][12];
    USHORT Lengths[UDF_SHORT_NAME_BATCH];
    uint32 Hashes[UDF_SHORT_NAME_BATCH];
    BOOLEAN Used[UDF_SHORT_NAME_BATCH];
    UDF_DIR_SCAN_CONTEXT ScanContext;
    PDIR_INDEX_ITEM DirNdx;
    UNICODE_STRING Candidate, Upcased, Existing;
    WCHAR UpcasedBuffer[12], ExistingBuffer[13];
    ULONG Attempt, Count = 0, i, From, To;

    ShortName->Length = 0;
    if (!UDFNeedsShortName(LongName))
        return STATUS_SUCCESS;

    RtlZeroMemory(&Context, sizeof(Context));
    for (i = 0; i < UDF_SHORT_NAME_BATCH; i++) {
        Candidate.Buffer = Names[i];
        Candidate.Length = 0;
        Candidate.MaximumLength = sizeof(Names[i]);
        RtlGenerate8dot3Name(LongName, FALSE, &Context, &Candidate);
        if (!Candidate.Length)
            break;
        Lengths[i] = Candidate.Length;
        Upcased.Buffer = UpcasedBuffer;
        Upcased.Length = 0;
        Upcased.MaximumLength = sizeof(UpcasedBuffer);
        RtlUpcaseUnicodeString(&Upcased, &Candidate, FALSE);
        Hashes[i] = crc32((uint8*)Upcased.Buffer, Upcased.Length);
        Used[i] = FALSE;
        Count++;
    }

    /* The first candidate is free in almost every directory: check it
     * alone, then the rest together. */
    for (From = 0, To = min(Count, 1); From < Count; From = To, To = Count) {
        if (UDFDirIndexInitScan(DirInfo, &ScanContext, 2)) {
            while ((DirNdx = UDFDirIndexScan(&ScanContext, NULL))) {
                BOOLEAN AliasRead = FALSE;
                if (!DirNdx->FName.Buffer || UDFIsDeleted(DirNdx))
                    continue;
                for (i = From; i < To; i++) {
                    if (Used[i] ||
                        (DirNdx->hashes.hLfn != Hashes[i] && DirNdx->hashes.hDos != Hashes[i]))
                        continue;
                    Candidate.Buffer = Names[i];
                    Candidate.Length = Candidate.MaximumLength = Lengths[i];
                    if (DirNdx->hashes.hLfn == Hashes[i] &&
                        RtlEqualUnicodeString(&DirNdx->FName, &Candidate, TRUE)) {
                        Used[i] = TRUE;
                        continue;
                    }
                    if ((DirNdx->FI_Flags & UDF_FI_FLAG_DOS) || DirNdx->hashes.hDos != Hashes[i])
                        continue;
                    if (!AliasRead) {
                        Existing.Buffer = ExistingBuffer;
                        Existing.Length = 0;
                        Existing.MaximumLength = sizeof(ExistingBuffer);
                        UDFGetDirNdxShortName(Vcb, DirNdx, FALSE, &Existing);
                        AliasRead = TRUE;
                    }
                    if (RtlEqualUnicodeString(&Existing, &Candidate, TRUE))
                        Used[i] = TRUE;
                }
            }
        }
        for (i = From; i < To; i++)
            if (!Used[i])
                break;
        if (i < To)
            break;
    }
    for (i = 0; i < Count; i++) {
        if (!Used[i]) {
            RtlCopyMemory(ShortName->Buffer, Names[i], Lengths[i]);
            ShortName->Length = Lengths[i];
            return STATUS_SUCCESS;
        }
    }
    if (Count < UDF_SHORT_NAME_BATCH) {
        ShortName->Length = 0;
        return Count ? STATUS_OBJECT_NAME_COLLISION : STATUS_SUCCESS;
    }

    /* Every early candidate is taken: continue one at a time. */
    for (Attempt = 0; Attempt < 100000; ++Attempt) {
        ShortName->Length = 0;
        RtlGenerate8dot3Name(LongName, FALSE, &Context, ShortName);
        if (!ShortName->Length)
            break;
        if (!UDFShortNameInUse(Vcb, DirInfo, ShortName))
            return STATUS_SUCCESS;
    }
    ShortName->Length = 0;
    return STATUS_OBJECT_NAME_COLLISION;
}

/* On-disk alias record in a FID's Implementation Use area (UDF 2.01
 * 2.3.4.5): an EntityID naming this implementation, then a version byte,
 * the alias length in characters and up to 12 ASCII characters. Other UDF
 * implementations skip Implementation Use data they do not recognise. */
#define UDF_SHORT_NAME_ID       "*ReactOS 8.3 Name"
#define UDF_SHORT_NAME_VERSION  1

BOOLEAN
UDFWriteShortNameImpUse(
    IN PFILE_IDENT_DESC FileId,
    IN PUNICODE_STRING ShortName
    )
{
    uint8* ImpUse = (uint8*)(FileId + 1);
    uint8* Record = ImpUse + sizeof(EntityID);
    ULONG Chars = ShortName->Length / sizeof(WCHAR), i;

    if (FileId->lengthOfImpUse < UDF_SHORT_NAME_IMPUSE_LEN || !Chars || Chars > 12)
        return FALSE;
    for (i = 0; i < Chars; ++i)
        if (ShortName->Buffer[i] < 0x20 || ShortName->Buffer[i] > 0x7e)
            return FALSE;

    RtlZeroMemory(ImpUse, UDF_SHORT_NAME_IMPUSE_LEN);
    UDFSetEntityID_imp((EntityID*)ImpUse, UDF_SHORT_NAME_ID);
    Record[0] = UDF_SHORT_NAME_VERSION;
    Record[1] = (uint8)Chars;
    for (i = 0; i < Chars; ++i)
        Record[2 + i] = (uint8)ShortName->Buffer[i];
    return TRUE;
}

/* ShortName must provide 12 WCHARs. */
BOOLEAN
UDFReadShortNameImpUse(
    IN PFILE_IDENT_DESC FileId,
    IN OUT PUNICODE_STRING ShortName
    )
{
    const uint8* ImpUse = (const uint8*)(FileId + 1);
    const uint8* Record = ImpUse + sizeof(EntityID);
    const EntityID* Id = (const EntityID*)ImpUse;
    ULONG Chars, i;

    if (FileId->lengthOfImpUse < UDF_SHORT_NAME_IMPUSE_LEN ||
        RtlCompareMemory(Id->ident, UDF_SHORT_NAME_ID, sizeof(UDF_SHORT_NAME_ID) - 1) !=
            sizeof(UDF_SHORT_NAME_ID) - 1 ||
        Record[0] != UDF_SHORT_NAME_VERSION)
        return FALSE;
    Chars = Record[1];
    if (!Chars || Chars > 12 || Chars * sizeof(WCHAR) > ShortName->MaximumLength)
        return FALSE;
    for (i = 0; i < Chars; ++i) {
        if (Record[2 + i] < 0x20 || Record[2 + i] > 0x7e)
            return FALSE;
        ShortName->Buffer[i] = Record[2 + i];
    }
    ShortName->Length = (USHORT)(Chars * sizeof(WCHAR));
    return TRUE;
}

/* Apply the alias stored in FileId, or record that the name needs none.
 * Entries written before aliases were stored get theirs from
 * UDFAssignMissingShortNames once the whole directory is indexed. */
VOID
UDFLoadDirNdxShortName(
    IN PDIR_INDEX_ITEM DirNdx,
    IN PFILE_IDENT_DESC FileId OPTIONAL
    )
{
    WCHAR Buffer[12];
    UNICODE_STRING Alias;

    Alias.Buffer = Buffer;
    Alias.Length = 0;
    Alias.MaximumLength = sizeof(Buffer);
    if (FileId && UDFReadShortNameImpUse(FileId, &Alias))
        UDFSetDirNdxShortName(DirNdx, &Alias);
    else if (!UDFNeedsShortName(&DirNdx->FName))
        UDFSetDirNdxShortName(DirNdx, NULL);
}

/* Give every entry that has no stored alias a Windows-style one, in memory
 * only. Entries are taken in directory order after the stored aliases are
 * known, so an entry gets the same alias on every mount unless an entry
 * before it is added or removed in between. */
VOID
UDFAssignMissingShortNames(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    )
{
    UDF_DIR_SCAN_CONTEXT ScanContext;
    PDIR_INDEX_ITEM DirNdx;
    WCHAR Buffer[12];
    UNICODE_STRING Alias;

    if (UDFIsAStreamDir(DirInfo) || !UDFDirIndexInitScan(DirInfo, &ScanContext, 2))
        return;
    while ((DirNdx = UDFDirIndexScan(&ScanContext, NULL))) {
        if (!DirNdx->FName.Buffer || UDFIsDeleted(DirNdx) ||
            (DirNdx->FI_Flags & (UDF_FI_FLAG_SHORT_NAME | UDF_FI_FLAG_FI_INTERNAL)))
            continue;
        Alias.Buffer = Buffer;
        Alias.Length = 0;
        Alias.MaximumLength = sizeof(Buffer);
        if (NT_SUCCESS(UDFGenerateUniqueShortName(Vcb, DirInfo, &DirNdx->FName, &Alias)) &&
            Alias.Length)
            UDFSetDirNdxShortName(DirNdx, &Alias);
    }
}
