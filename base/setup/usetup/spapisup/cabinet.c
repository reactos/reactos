/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS text-mode setup
 * FILE:        base/setup/usetup/cabinet.c
 * PURPOSE:     Cabinet routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15/08-2003 Created
 */

#ifndef _USETUP_PCH_
#include "usetup.h"
#endif

#define Z_SOLO
#include <zlib.h>

#define NDEBUG
#include <debug.h>


/* DEFINITIONS **************************************************************/

/* File management definitions */

#define SEEK_BEGIN    0
#define SEEK_CURRENT  1
#ifndef SEEK_END
#define SEEK_END      2
#endif

typedef struct _DOSTIME
{
    WORD Second:5;
    WORD Minute:6;
    WORD Hour:5;
} DOSTIME, *PDOSTIME;

typedef struct _DOSDATE
{
    WORD Day:5;
    WORD Month:4;
    WORD Year:5;
} DOSDATE, *PDOSDATE;


/* Cabinet constants */

#define CAB_SIGNATURE        0x4643534D // "MSCF"
#define CAB_VERSION          0x0103
#define CAB_BLOCKSIZE        32768

#define CAB_COMP_MASK        0x00FF
#define CAB_COMP_NONE        0x0000
#define CAB_COMP_MSZIP       0x0001
#define CAB_COMP_QUANTUM     0x0002
#define CAB_COMP_LZX         0x0003

#define CAB_FLAG_HASPREV     0x0001
#define CAB_FLAG_HASNEXT     0x0002
#define CAB_FLAG_RESERVE     0x0004

#define CAB_ATTRIB_READONLY  0x0001
#define CAB_ATTRIB_HIDDEN    0x0002
#define CAB_ATTRIB_SYSTEM    0x0004
#define CAB_ATTRIB_VOLUME    0x0008
#define CAB_ATTRIB_DIRECTORY 0x0010
#define CAB_ATTRIB_ARCHIVE   0x0020
#define CAB_ATTRIB_EXECUTE   0x0040
#define CAB_ATTRIB_UTF_NAME  0x0080

#define CAB_FILE_MAX_FOLDER  0xFFFC
#define CAB_FILE_CONTINUED   0xFFFD
#define CAB_FILE_SPLIT       0xFFFE
#define CAB_FILE_PREV_NEXT   0xFFFF


/* Cabinet structures */

typedef struct _CFHEADER
{
    ULONG  Signature;       // File signature 'MSCF' (CAB_SIGNATURE)
    ULONG  Reserved1;       // Reserved field
    ULONG  CabinetSize;     // Cabinet file size
    ULONG  Reserved2;       // Reserved field
    ULONG  FileTableOffset; // Offset of first CFFILE
    ULONG  Reserved3;       // Reserved field
    USHORT Version;         // Cabinet version (CAB_VERSION)
    USHORT FolderCount;     // Number of folders
    USHORT FileCount;       // Number of files
    USHORT Flags;           // Cabinet flags (CAB_FLAG_*)
    USHORT SetID;           // Cabinet set id
    USHORT CabinetNumber;   // Zero-based cabinet number
/* Optional fields (depends on Flags)
    USHORT CabinetResSize   // Per-cabinet reserved area size
    CHAR  FolderResSize     // Per-folder reserved area size
    CHAR  FileResSize       // Per-file reserved area size
    CHAR  CabinetReserved[] // Per-cabinet reserved area
    CHAR  CabinetPrev[]     // Name of previous cabinet file
    CHAR  DiskPrev[]        // Name of previous disk
    CHAR  CabinetNext[]     // Name of next cabinet file
    CHAR  DiskNext[]        // Name of next disk
 */
} CFHEADER, *PCFHEADER;

typedef struct _CFFOLDER
{
    ULONG  DataOffset;      // Absolute offset of first CFDATA block in this folder
    USHORT DataBlockCount;  // Number of CFDATA blocks in this folder in this cabinet
    USHORT CompressionType; // Type of compression used for all CFDATA blocks in this folder
/* Optional fields (depends on Flags)
    CHAR   FolderReserved[] // Per-folder reserved area
 */
} CFFOLDER, *PCFFOLDER;

typedef struct _CFFILE
{
    ULONG  FileSize;        // Uncompressed file size in bytes
    ULONG  FileOffset;      // Uncompressed offset of file in the folder
    USHORT FolderIndex;     // Index number of the folder that contains this file
    USHORT FileDate;        // File date stamp, as used by DOS
    USHORT FileTime;        // File time stamp, as used by DOS
    USHORT Attributes;      // File attributes (CAB_ATTRIB_*)
    CHAR   FileName[ANYSIZE_ARRAY];
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;

typedef struct _CFDATA
{
    ULONG  Checksum;        // Checksum of CFDATA entry
    USHORT CompSize;        // Number of compressed bytes in this block
    USHORT UncompSize;      // Number of uncompressed bytes in this block
/* Optional fields (depends on Flags)
    CHAR   DataReserved[]   // Per-datablock reserved area
 */
} CFDATA, *PCFDATA;


/* FUNCTIONS ****************************************************************/

#if !defined(_INC_MALLOC) && !defined(_INC_STDLIB)

/* Needed by zlib, but we don't want the dependency on the CRT */
void *__cdecl
malloc(size_t size)
{
    return RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, size);
}

void __cdecl
free(void *ptr)
{
    RtlFreeHeap(ProcessHeap, 0, ptr);
}

void *__cdecl
calloc(size_t nmemb, size_t size)
{
    return (void *)RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, nmemb * size);
}

#endif // !_INC_MALLOC && !_INC_STDLIB


/* Codecs */

/* Uncompresses a data block */
typedef ULONG (*PCABINET_CODEC_UNCOMPRESS)(
    IN struct _CAB_CODEC* Codec,
    OUT PVOID OutputBuffer,
    IN PVOID InputBuffer,
    IN OUT PLONG InputLength,
    IN OUT PLONG OutputLength);

typedef struct _CAB_CODEC
{
    PCABINET_CODEC_UNCOMPRESS Uncompress;
    z_stream ZStream;
    // Other CODEC-related structures
} CAB_CODEC, *PCAB_CODEC;


/* RAW codec */

/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer before, and amount consumed after
 *                    Negative to indicate that this is not the start of a new block
 *     OutputLength = Length of output buffer before, amount filled after
 *                    Negative to indicate that this is not the end of the block
 */
ULONG
RawCodecUncompress(
    IN OUT PCAB_CODEC Codec,
    OUT PVOID OutputBuffer,
    IN PVOID InputBuffer,
    IN OUT PLONG InputLength,
    IN OUT PLONG OutputLength)
{
    LONG Len = min(abs(*InputLength), abs(*OutputLength));

    memcpy(OutputBuffer, InputBuffer, Len);
    *InputLength = *OutputLength = Len;

    return CS_SUCCESS;
}

static CAB_CODEC RawCodec =
{
    RawCodecUncompress, {0}
};

/* MSZIP codec */

#define MSZIP_MAGIC 0x4B43

/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer before, and amount consumed after
 *                    Negative to indicate that this is not the start of a new block
 *     OutputLength = Length of output buffer before, amount filled after
 *                    Negative to indicate that this is not the end of the block
 */
ULONG
MSZipCodecUncompress(
    IN OUT PCAB_CODEC Codec,
    OUT PVOID OutputBuffer,
    IN PVOID InputBuffer,
    IN OUT PLONG InputLength,
    IN OUT PLONG OutputLength)
{
    USHORT Magic;
    INT Status;

    DPRINT("MSZipCodecUncompress(OutputBuffer = %x, InputBuffer = %x, "
           "InputLength = %d, OutputLength = %d)\n", OutputBuffer,
           InputBuffer, *InputLength, *OutputLength);

    if (*InputLength > 0)
    {
        Magic = *(PUSHORT)InputBuffer;

        if (Magic != MSZIP_MAGIC)
        {
            DPRINT("Bad MSZIP block header magic (0x%X)\n", Magic);
            return CS_BADSTREAM;
        }

        Codec->ZStream.next_in = (PUCHAR)InputBuffer + 2;
        Codec->ZStream.avail_in = *InputLength - 2;
        Codec->ZStream.next_out = (PUCHAR)OutputBuffer;
        Codec->ZStream.avail_out = abs(*OutputLength);

        /* WindowBits is passed < 0 to tell that there is no zlib header.
         * Note that in this case inflate *requires* an extra "dummy" byte
         * after the compressed stream in order to complete decompression and
         * return Z_STREAM_END.
         */
        Status = inflateInit2(&Codec->ZStream, -MAX_WBITS);
        if (Status != Z_OK)
        {
            DPRINT("inflateInit2() returned (%d)\n", Status);
            return CS_BADSTREAM;
        }
        Codec->ZStream.total_in = 2;
    }
    else
    {
        Codec->ZStream.avail_in = -*InputLength;
        Codec->ZStream.next_in = (PUCHAR)InputBuffer;
        Codec->ZStream.next_out = (PUCHAR)OutputBuffer;
        Codec->ZStream.avail_out = abs(*OutputLength);
        Codec->ZStream.total_in = 0;
    }

    Codec->ZStream.total_out = 0;
    Status = inflate(&Codec->ZStream, Z_SYNC_FLUSH);
    if (Status != Z_OK && Status != Z_STREAM_END)
    {
        DPRINT("inflate() returned (%d) (%s)\n", Status, Codec->ZStream.msg);
        if (Status == Z_MEM_ERROR)
            return CS_NOMEMORY;
        return CS_BADSTREAM;
    }

    if (*OutputLength > 0)
    {
        Status = inflateEnd(&Codec->ZStream);
        if (Status != Z_OK)
        {
            DPRINT("inflateEnd() returned (%d)\n", Status);
            return CS_BADSTREAM;
        }
    }

    *InputLength = Codec->ZStream.total_in;
    *OutputLength = Codec->ZStream.total_out;

    return CS_SUCCESS;
}

static CAB_CODEC MSZipCodec =
{
    MSZipCodecUncompress, {0}
};


/* Memory functions */

voidpf
MSZipAlloc(voidpf opaque, uInt items, uInt size)
{
    return (voidpf)RtlAllocateHeap(ProcessHeap, 0, items * size);
}

void
MSZipFree(voidpf opaque, voidpf address)
{
    RtlFreeHeap(ProcessHeap, 0, address);
}

static BOOL
ConvertSystemTimeToFileTime(CONST SYSTEMTIME *lpSystemTime,
                            LPFILETIME lpFileTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER liTime;

    TimeFields.Year = lpSystemTime->wYear;
    TimeFields.Month = lpSystemTime->wMonth;
    TimeFields.Day = lpSystemTime->wDay;
    TimeFields.Hour = lpSystemTime->wHour;
    TimeFields.Minute = lpSystemTime->wMinute;
    TimeFields.Second = lpSystemTime->wSecond;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if (RtlTimeFieldsToTime(&TimeFields, &liTime))
    {
        lpFileTime->dwLowDateTime = liTime.u.LowPart;
        lpFileTime->dwHighDateTime = liTime.u.HighPart;
        return TRUE;
    }

    return FALSE;
}

static BOOL
ConvertDosDateTimeToFileTime(WORD wFatDate,
                             WORD wFatTime,
                             LPFILETIME lpFileTime)
{
    PDOSTIME pdtime = (PDOSTIME)&wFatTime;
    PDOSDATE pddate = (PDOSDATE)&wFatDate;
    SYSTEMTIME SystemTime;

    if (lpFileTime == NULL)
        return FALSE;

    SystemTime.wMilliseconds = 0;
    SystemTime.wSecond = pdtime->Second;
    SystemTime.wMinute = pdtime->Minute;
    SystemTime.wHour = pdtime->Hour;

    SystemTime.wDay = pddate->Day;
    SystemTime.wMonth = pddate->Month;
    SystemTime.wYear = 1980 + pddate->Year;

    ConvertSystemTimeToFileTime(&SystemTime, lpFileTime);

    return TRUE;
}

/*
 * FUNCTION: Returns a pointer to file name
 * ARGUMENTS:
 *     Path = Pointer to string with pathname
 * RETURNS:
 *     Pointer to filename
 */
static PWCHAR
GetFileName(PWCHAR Path)
{
    ULONG i, j;

    j = i = 0;

    while (Path[i++])
    {
        if (Path[i - 1] == L'\\')
            j = i;
    }

    return Path + j;
}

/*
 * FUNCTION: Removes a file name from a path
 * ARGUMENTS:
 *     Path = Pointer to string with path
 */
static VOID
RemoveFileName(PWCHAR Path)
{
    PWCHAR FileName;
    DWORD i;

    i = 0;
    FileName = GetFileName(Path + i);

    if (FileName != Path + i && FileName[-1] == L'\\')
        FileName--;

    if (FileName == Path + i && FileName[0] == L'\\')
        FileName++;

    FileName[0] = 0;
}

/*
 * FUNCTION: Sets attributes on a file
 * ARGUMENTS:
 *      File = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
static BOOL
SetAttributesOnFile(PCFFILE File,
                    HANDLE hFile)
{
    FILE_BASIC_INFORMATION FileBasic;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS NtStatus;
    ULONG Attributes = 0;

    if (File->Attributes & CAB_ATTRIB_READONLY)
        Attributes |= FILE_ATTRIBUTE_READONLY;

    if (File->Attributes & CAB_ATTRIB_HIDDEN)
        Attributes |= FILE_ATTRIBUTE_HIDDEN;

    if (File->Attributes & CAB_ATTRIB_SYSTEM)
        Attributes |= FILE_ATTRIBUTE_SYSTEM;

    if (File->Attributes & CAB_ATTRIB_DIRECTORY)
        Attributes |= FILE_ATTRIBUTE_DIRECTORY;

    if (File->Attributes & CAB_ATTRIB_ARCHIVE)
        Attributes |= FILE_ATTRIBUTE_ARCHIVE;

    NtStatus = NtQueryInformationFile(hFile,
                                      &IoStatusBlock,
                                      &FileBasic,
                                      sizeof(FILE_BASIC_INFORMATION),
                                      FileBasicInformation);
    if (!NT_SUCCESS(NtStatus))
    {
        DPRINT("NtQueryInformationFile() failed (%x)\n", NtStatus);
    }
    else
    {
        FileBasic.FileAttributes = Attributes;

        NtStatus = NtSetInformationFile(hFile,
                                        &IoStatusBlock,
                                        &FileBasic,
                                        sizeof(FILE_BASIC_INFORMATION),
                                        FileBasicInformation);
        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT("NtSetInformationFile() failed (%x)\n", NtStatus);
        }
    }

    return NT_SUCCESS(NtStatus);
}

/*
 * FUNCTION: Closes the current cabinet
 * RETURNS:
 *     Status of operation
 */
static ULONG
CloseCabinet(
    IN PCABINET_CONTEXT CabinetContext)
{
    if (CabinetContext->FileBuffer)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), CabinetContext->FileBuffer);
        NtClose(CabinetContext->FileSectionHandle);
        NtClose(CabinetContext->FileHandle);
        CabinetContext->FileBuffer = NULL;
    }

    return 0;
}

/*
 * FUNCTION: Initialize archiver
 */
VOID
CabinetInitialize(
    IN OUT PCABINET_CONTEXT CabinetContext)
{
    RtlZeroMemory(CabinetContext, sizeof(*CabinetContext));

    CabinetContext->FileOpen = FALSE;
    wcscpy(CabinetContext->DestPath, L"");

    CabinetContext->CodecSelected = FALSE;
    CabinetSelectCodec(CabinetContext, CAB_CODEC_RAW);

    CabinetContext->OverwriteHandler = NULL;
    CabinetContext->ExtractHandler = NULL;
    CabinetContext->DiskChangeHandler = NULL;

    CabinetContext->FolderUncompSize = 0;
    CabinetContext->BytesLeftInBlock = 0;
    CabinetContext->CabinetReserved = 0;
    CabinetContext->FolderReserved = 0;
    CabinetContext->DataReserved = 0;
    CabinetContext->CabinetReservedArea = NULL;
    CabinetContext->LastFileOffset = 0;
}

/*
 * FUNCTION: Cleanup archiver
 */
VOID
CabinetCleanup(
    IN OUT PCABINET_CONTEXT CabinetContext)
{
    CabinetClose(CabinetContext);
}

/*
 * FUNCTION: Normalizes a path
 * ARGUMENTS:
 *     Path   = Pointer to string with pathname
 *     Length = Number of characters in Path
 * RETURNS:
 *     TRUE if there was enough room in Path, or FALSE
 */
static BOOL
CabinetNormalizePath(PWCHAR Path,
                     ULONG Length)
{
    ULONG n;
    BOOL Ok;

    n = wcslen(Path);
    Ok = (n + 1) < Length;

    if (n != 0 && Path[n - 1] != L'\\' && Ok)
    {
        Path[n] = L'\\';
        Path[n + 1] = 0;
    }

    return Ok;
}

/*
 * FUNCTION: Returns pointer to cabinet file name
 * RETURNS:
 *     Pointer to string with name of cabinet
 */
PCWSTR
CabinetGetCabinetName(
    IN PCABINET_CONTEXT CabinetContext)
{
    return CabinetContext->CabinetName;
}

/*
 * FUNCTION: Sets cabinet file name
 * ARGUMENTS:
 *     FileName = Pointer to string with name of cabinet
 */
VOID
CabinetSetCabinetName(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName)
{
    wcscpy(CabinetContext->CabinetName, FileName);
}

/*
 * FUNCTION: Sets destination path
 * ARGUMENTS:
 *    DestinationPath = Pointer to string with name of destination path
 */
VOID
CabinetSetDestinationPath(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR DestinationPath)
{
    wcscpy(CabinetContext->DestPath, DestinationPath);

    if (wcslen(CabinetContext->DestPath) > 0)
        CabinetNormalizePath(CabinetContext->DestPath, MAX_PATH);
}

/*
 * FUNCTION: Returns destination path
 * RETURNS:
 *    Pointer to string with name of destination path
 */
PCWSTR
CabinetGetDestinationPath(
    IN PCABINET_CONTEXT CabinetContext)
{
    return CabinetContext->DestPath;
}

/*
 * FUNCTION: Opens a cabinet file
 * RETURNS:
 *     Status of operation
 */
ULONG
CabinetOpen(
    IN OUT PCABINET_CONTEXT CabinetContext)
{
    PUCHAR Buffer;
    UNICODE_STRING ustring;
    ANSI_STRING astring;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileName;
    USHORT StringLength;
    NTSTATUS NtStatus;

    if (CabinetContext->FileOpen)
    {
        /* Cabinet file already opened */
        DPRINT("CabinetOpen returning SUCCESS\n");
        return CAB_STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&FileName, CabinetContext->CabinetName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL);

    NtStatus = NtOpenFile(&CabinetContext->FileHandle,
                          GENERIC_READ | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          FILE_SHARE_READ,
                          FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(NtStatus))
    {
        DPRINT1("Cannot open file (%S) (%x)\n", CabinetContext->CabinetName, NtStatus);
        return CAB_STATUS_CANNOT_OPEN;
    }

    CabinetContext->FileOpen = TRUE;

    NtStatus = NtCreateSection(&CabinetContext->FileSectionHandle,
                               SECTION_ALL_ACCESS,
                               0, 0,
                               PAGE_READONLY,
                               SEC_COMMIT,
                               CabinetContext->FileHandle);

    if (!NT_SUCCESS(NtStatus))
    {
        DPRINT1("NtCreateSection failed for %ls: %x\n", CabinetContext->CabinetName, NtStatus);
        return CAB_STATUS_NOMEMORY;
    }

    CabinetContext->FileBuffer = 0;
    CabinetContext->FileSize = 0;

    NtStatus = NtMapViewOfSection(CabinetContext->FileSectionHandle,
                                  NtCurrentProcess(),
                                  (PVOID*)&CabinetContext->FileBuffer,
                                  0, 0, 0,
                                  &CabinetContext->FileSize,
                                  ViewUnmap,
                                  0,
                                  PAGE_READONLY);

    if (!NT_SUCCESS(NtStatus))
    {
        DPRINT1("NtMapViewOfSection failed: %x\n", NtStatus);
        return CAB_STATUS_NOMEMORY;
    }

    DPRINT("Cabinet file %S opened and mapped to %x\n",
           CabinetContext->CabinetName, CabinetContext->FileBuffer);
    CabinetContext->PCABHeader = (PCFHEADER)CabinetContext->FileBuffer;

    /* Check header */
    if (CabinetContext->FileSize <= sizeof(CFHEADER) ||
        CabinetContext->PCABHeader->Signature != CAB_SIGNATURE ||
        CabinetContext->PCABHeader->Version != CAB_VERSION ||
        CabinetContext->PCABHeader->FolderCount == 0 ||
        CabinetContext->PCABHeader->FileCount == 0 ||
        CabinetContext->PCABHeader->FileTableOffset < sizeof(CFHEADER))
    {
        CloseCabinet(CabinetContext);
        DPRINT1("File has invalid header\n");
        return CAB_STATUS_INVALID_CAB;
    }

    Buffer = (PUCHAR)(CabinetContext->PCABHeader + 1);

    /* Read/skip any reserved bytes */
    if (CabinetContext->PCABHeader->Flags & CAB_FLAG_RESERVE)
    {
        CabinetContext->CabinetReserved = *(PUSHORT)Buffer;
        Buffer += 2;
        CabinetContext->FolderReserved = *Buffer;
        Buffer++;
        CabinetContext->DataReserved = *Buffer;
        Buffer++;

        if (CabinetContext->CabinetReserved > 0)
        {
            CabinetContext->CabinetReservedArea = Buffer;
            Buffer += CabinetContext->CabinetReserved;
        }
    }

    if (CabinetContext->PCABHeader->Flags & CAB_FLAG_HASPREV)
    {
        /* The previous cabinet file is in
           the same directory as the current */
        wcscpy(CabinetContext->CabinetPrev, CabinetContext->CabinetName);
        RemoveFileName(CabinetContext->CabinetPrev);
        CabinetNormalizePath(CabinetContext->CabinetPrev, sizeof(CabinetContext->CabinetPrev));
        RtlInitAnsiString(&astring, (LPSTR)Buffer);

        /* Initialize ustring with the remaining buffer */
        StringLength = (USHORT)wcslen(CabinetContext->CabinetPrev) * sizeof(WCHAR);
        ustring.Buffer = CabinetContext->CabinetPrev + StringLength;
        ustring.MaximumLength = sizeof(CabinetContext->CabinetPrev) - StringLength;
        ustring.Length = 0;
        RtlAnsiStringToUnicodeString(&ustring, &astring, FALSE);
        Buffer += astring.Length + 1;

        /* Read label of prev disk */
        RtlInitAnsiString(&astring, (LPSTR)Buffer);
        ustring.Length = 0;
        ustring.Buffer = CabinetContext->DiskPrev;
        ustring.MaximumLength = sizeof(CabinetContext->DiskPrev);
        RtlAnsiStringToUnicodeString(&ustring, &astring, FALSE);
        Buffer += astring.Length + 1;
    }
    else
    {
        wcscpy(CabinetContext->CabinetPrev, L"");
        wcscpy(CabinetContext->DiskPrev, L"");
    }

    if (CabinetContext->PCABHeader->Flags & CAB_FLAG_HASNEXT)
    {
        /* The next cabinet file is in
           the same directory as the previous */
        wcscpy(CabinetContext->CabinetNext, CabinetContext->CabinetName);
        RemoveFileName(CabinetContext->CabinetNext);
        CabinetNormalizePath(CabinetContext->CabinetNext, 256);
        RtlInitAnsiString(&astring, (LPSTR)Buffer);

        /* Initialize ustring with the remaining buffer */
        StringLength = (USHORT)wcslen(CabinetContext->CabinetNext) * sizeof(WCHAR);
        ustring.Buffer = CabinetContext->CabinetNext + StringLength;
        ustring.MaximumLength = sizeof(CabinetContext->CabinetNext) - StringLength;
        ustring.Length = 0;
        RtlAnsiStringToUnicodeString(&ustring, &astring, FALSE);
        Buffer += astring.Length + 1;

        /* Read label of next disk */
        RtlInitAnsiString(&astring, (LPSTR)Buffer);
        ustring.Length = 0;
        ustring.Buffer = CabinetContext->DiskNext;
        ustring.MaximumLength = sizeof(CabinetContext->DiskNext);
        RtlAnsiStringToUnicodeString(&ustring, &astring, FALSE);
        Buffer += astring.Length + 1;
    }
    else
    {
        wcscpy(CabinetContext->CabinetNext, L"");
        wcscpy(CabinetContext->DiskNext, L"");
    }
    CabinetContext->CabinetFolders = (PCFFOLDER)Buffer;

    DPRINT("CabinetOpen returning SUCCESS\n");
    return CAB_STATUS_SUCCESS;
}

/*
 * FUNCTION: Closes the cabinet file
 */
VOID
CabinetClose(
    IN OUT PCABINET_CONTEXT CabinetContext)
{
    if (!CabinetContext->FileOpen)
        return;

    CloseCabinet(CabinetContext);
    CabinetContext->FileOpen = FALSE;
}

/*
 * FUNCTION: Finds the first file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     FileName = Pointer to search criteria
 *     Search   = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
ULONG
CabinetFindFirst(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName,
    IN OUT PCAB_SEARCH Search)
{
    DPRINT("CabinetFindFirst(FileName = %S)\n", FileName);
    wcsncpy(Search->Search, FileName, MAX_PATH);
    wcsncpy(Search->Cabinet, CabinetContext->CabinetName, MAX_PATH);
    Search->File = 0;
    return CabinetFindNext(CabinetContext, Search);
}

/*
 * FUNCTION: Finds next file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     Search = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
ULONG
CabinetFindNext(
    IN PCABINET_CONTEXT CabinetContext,
    IN OUT PCAB_SEARCH Search)
{
    PCFFILE Prev;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    WCHAR FileName[MAX_PATH];

    if (wcscmp(Search->Cabinet, CabinetContext->CabinetName) != 0)
    {
        /* restart search of cabinet has changed since last find */
        Search->File = 0;
    }

    if (!Search->File)
    {
        /* starting new search or cabinet */
        Search->File = (PCFFILE)(CabinetContext->FileBuffer + CabinetContext->PCABHeader->FileTableOffset);
        Search->Index = 0;
        Prev = 0;
    }
    else
        Prev = Search->File;

    while (TRUE)
    {
        /* look at each file in the archive and see if we found a match */
        if (Search->File->FolderIndex == 0xFFFD ||
            Search->File->FolderIndex == 0xFFFF)
        {
            /* skip files continued from previous cab */
            DPRINT("Skipping file (%s): FileOffset (0x%X), "
                   "LastFileOffset (0x%X)\n", (char *)(Search->File + 1),
                   Search->File->FileOffset, CabinetContext->LastFileOffset);
        }
        else
        {
            // FIXME: check for match against search criteria
            if (Search->File != Prev)
            {
                if (Prev == NULL || Search->File->FolderIndex != Prev->FolderIndex)
                {
                    Search->CFData = NULL;
                    Search->Offset = 0;
                }

                /* don't match the file we started with */
                if (wcscmp(Search->Search, L"*") == 0)
                {
                    /* take any file */
                    break;
                }
                else
                {
                    /* otherwise, try to match the exact file name */
                    RtlInitAnsiString(&AnsiString, Search->File->FileName);
                    UnicodeString.Buffer = FileName;
                    UnicodeString.Buffer[0] = 0;
                    UnicodeString.Length = 0;
                    UnicodeString.MaximumLength = sizeof(FileName);
                    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);
                    if (wcscmp(Search->Search, UnicodeString.Buffer) == 0)
                        break;
                }
            }
        }

        /* if we make it here we found no match, so move to the next file */
        Search->Index++;
        if (Search->Index >= CabinetContext->PCABHeader->FileCount)
        {
            /* we have reached the end of this cabinet */
            DPRINT("End of cabinet reached\n");
            return CAB_STATUS_NOFILE;
        }
        else
            Search->File = (PCFFILE)(strchr((char *)(Search->File + 1), 0) + 1);
    }

    DPRINT("Found file %s\n", Search->File->FileName);
    return CAB_STATUS_SUCCESS;
}

/*
 * FUNCTION: Finds the next file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     FileName = Pointer to search criteria
 *     Search   = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
ULONG
CabinetFindNextFileSequential(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName,
    IN OUT PCAB_SEARCH Search)
{
    DPRINT("CabinetFindNextFileSequential(FileName = %S)\n", FileName);
    wcsncpy(Search->Search, FileName, MAX_PATH);
    return CabinetFindNext(CabinetContext, Search);
}

#if 0
int
Validate(VOID)
{
    return (int)RtlValidateHeap(ProcessHeap, 0, 0);
}
#endif

/*
 * FUNCTION: Extracts a file from the cabinet
 * ARGUMENTS:
 *     Search = Pointer to PCAB_SEARCH structure used to locate the file
 * RETURNS
 *     Status of operation
 */
ULONG
CabinetExtractFile(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCAB_SEARCH Search)
{
    ULONG Size;                 // remaining file bytes to decompress
    ULONG CurrentOffset;        // current uncompressed offset within the folder
    PUCHAR CurrentBuffer;       // current pointer to compressed data in the block
    LONG RemainingBlock;        // remaining comp data in the block
    HANDLE DestFile;
    HANDLE DestFileSection;
    PVOID DestFileBuffer;       // mapped view of dest file
    PVOID CurrentDestBuffer;    // pointer to the current position in the dest view
    PCFDATA CFData;             // current data block
    ULONG Status;
    FILETIME FileTime;
    WCHAR DestName[MAX_PATH];
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_BASIC_INFORMATION FileBasic;
    PCFFOLDER CurrentFolder;
    LARGE_INTEGER MaxDestFileSize;
    LONG InputLength, OutputLength;
    SIZE_T StringLength;
    char Chunk[512];

    if (wcscmp(Search->Cabinet, CabinetContext->CabinetName) != 0)
    {
        /* the file is not in the current cabinet */
        DPRINT("File is not in this cabinet (%S != %S)\n",
               Search->Cabinet, CabinetContext->CabinetName);
        return CAB_STATUS_NOFILE;
    }

    /* look up the folder that the file specifies */
    if (Search->File->FolderIndex == 0xFFFD ||
        Search->File->FolderIndex == 0xFFFF)
    {
        /* folder is continued from previous cabinet,
           that shouldn't happen here */
        return CAB_STATUS_NOFILE;
    }
    else if (Search->File->FolderIndex == 0xFFFE)
    {
        /* folder is the last in this cabinet and continues into next */
        CurrentFolder = &CabinetContext->CabinetFolders[CabinetContext->PCABHeader->FolderCount - 1];
    }
    else
    {
        /* folder is completely contained within this cabinet */
        CurrentFolder = &CabinetContext->CabinetFolders[Search->File->FolderIndex];
    }

    switch (CurrentFolder->CompressionType & CAB_COMP_MASK)
    {
        case CAB_COMP_NONE:
            CabinetSelectCodec(CabinetContext, CAB_CODEC_RAW);
            break;
        case CAB_COMP_MSZIP:
            CabinetSelectCodec(CabinetContext, CAB_CODEC_MSZIP);
            break;
        default:
            return CAB_STATUS_UNSUPPCOMP;
    }

    DPRINT("Extracting file at uncompressed offset (0x%X) Size (%d bytes)\n",
           (UINT)Search->File->FileOffset, (UINT)Search->File->FileSize);

    if (CabinetContext->CreateFileHandler)
    {
        /* Call create context */
        CurrentDestBuffer = CabinetContext->CreateFileHandler(CabinetContext, Search->File->FileSize);
        if (!CurrentDestBuffer)
        {
            DPRINT1("CreateFileHandler() failed\n");
            return CAB_STATUS_CANNOT_CREATE;
        }
    }
    else
    {
        RtlInitAnsiString(&AnsiString, Search->File->FileName);
        wcscpy(DestName, CabinetContext->DestPath);
        StringLength = wcslen(DestName);
        UnicodeString.MaximumLength = sizeof(DestName) - (USHORT)StringLength * sizeof(WCHAR);
        UnicodeString.Buffer = DestName + StringLength;
        UnicodeString.Length = 0;
        RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);

        /* Create destination file, fail if it already exists */
        RtlInitUnicodeString(&UnicodeString, DestName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, NULL);

        NtStatus = NtCreateFile(&DestFile,
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,
                                FILE_ATTRIBUTE_NORMAL,
                                0,
                                FILE_CREATE,
                                FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL, 0);

        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT("NtCreateFile() failed (%S) (%x)\n", DestName, NtStatus);

            /* If file exists, ask to overwrite file */
            if (CabinetContext->OverwriteHandler == NULL ||
                CabinetContext->OverwriteHandler(CabinetContext, Search->File, DestName))
            {
                /* Create destination file, overwrite if it already exists */
                NtStatus = NtCreateFile(&DestFile,
                                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                        &ObjectAttributes,
                                        &IoStatusBlock,
                                        NULL,
                                        FILE_ATTRIBUTE_NORMAL,
                                        0,
                                        FILE_OVERWRITE,
                                        FILE_SYNCHRONOUS_IO_ALERT,
                                        NULL, 0);

                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT1("NtCreateFile() failed (%S) (%x)\n", DestName, NtStatus);
                    return CAB_STATUS_CANNOT_CREATE;
                }
            }
            else
            {
                DPRINT1("File (%S) exists\n", DestName);
                return CAB_STATUS_FILE_EXISTS;
            }
        }

        if (!ConvertDosDateTimeToFileTime(Search->File->FileDate,
                                          Search->File->FileTime,
                                          &FileTime))
        {
            DPRINT1("DosDateTimeToFileTime() failed\n");
            Status = CAB_STATUS_CANNOT_WRITE;
            goto CloseDestFile;
        }

        NtStatus = NtQueryInformationFile(DestFile,
                                          &IoStatusBlock,
                                          &FileBasic,
                                          sizeof(FILE_BASIC_INFORMATION),
                                          FileBasicInformation);
        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT("NtQueryInformationFile() failed (%x)\n", NtStatus);
        }
        else
        {
            memcpy(&FileBasic.LastAccessTime, &FileTime, sizeof(FILETIME));

            NtStatus = NtSetInformationFile(DestFile,
                                            &IoStatusBlock,
                                            &FileBasic,
                                            sizeof(FILE_BASIC_INFORMATION),
                                            FileBasicInformation);
            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT("NtSetInformationFile() failed (%x)\n", NtStatus);
            }
        }

        SetAttributesOnFile(Search->File, DestFile);

        /* Nothing more to do for 0 sized files */
        if (Search->File->FileSize == 0)
        {
            Status = CAB_STATUS_SUCCESS;
            goto CloseDestFile;
        }

        MaxDestFileSize.QuadPart = Search->File->FileSize;
        NtStatus = NtCreateSection(&DestFileSection,
                                   SECTION_ALL_ACCESS,
                                   0,
                                   &MaxDestFileSize,
                                   PAGE_READWRITE,
                                   SEC_COMMIT,
                                   DestFile);

        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT1("NtCreateSection failed for %ls: %x\n", DestName, NtStatus);
            Status = CAB_STATUS_NOMEMORY;
            goto CloseDestFile;
        }

        DestFileBuffer = 0;
        CabinetContext->DestFileSize = 0;
        NtStatus = NtMapViewOfSection(DestFileSection,
                                      NtCurrentProcess(),
                                      &DestFileBuffer,
                                      0, 0, 0,
                                      &CabinetContext->DestFileSize,
                                      ViewUnmap,
                                      0,
                                      PAGE_READWRITE);

        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT1("NtMapViewOfSection failed: %x\n", NtStatus);
            Status = CAB_STATUS_NOMEMORY;
            goto CloseDestFileSection;
        }

        CurrentDestBuffer = DestFileBuffer;
    }

    /* Call extract event handler */
    if (CabinetContext->ExtractHandler != NULL)
        CabinetContext->ExtractHandler(CabinetContext, Search->File, DestName);

    if (Search->CFData)
        CFData = Search->CFData;
    else
        CFData = (PCFDATA)(CabinetContext->CabinetFolders[Search->File->FolderIndex].DataOffset + CabinetContext->FileBuffer);

    CurrentOffset = Search->Offset;
    while (CurrentOffset + CFData->UncompSize <= Search->File->FileOffset)
    {
        /* walk the data blocks until we reach
           the one containing the start of the file */
        CurrentOffset += CFData->UncompSize;
        CFData = (PCFDATA)((char *)(CFData + 1) + CabinetContext->DataReserved + CFData->CompSize);
    }

    Search->CFData = CFData;
    Search->Offset = CurrentOffset;

    /* now decompress and discard any data in
       the block before the start of the file */

    /* start of comp data */
    CurrentBuffer = ((unsigned char *)(CFData + 1)) + CabinetContext->DataReserved;
    RemainingBlock = CFData->CompSize;
    InputLength = RemainingBlock;

    while (CurrentOffset < Search->File->FileOffset)
    {
        /* compute remaining uncomp bytes to start
           of file, bounded by size of chunk */
        OutputLength = Search->File->FileOffset - CurrentOffset;
        if (OutputLength > (LONG)sizeof(Chunk))
            OutputLength = sizeof(Chunk);

        /* negate to signal NOT end of block */
        OutputLength = -OutputLength;

        CabinetContext->Codec->Uncompress(CabinetContext->Codec,
                                          Chunk,
                                          CurrentBuffer,
                                          &InputLength,
                                          &OutputLength);

        /* add the uncomp bytes extracted to current folder offset */
        CurrentOffset += OutputLength;
        /* add comp bytes consumed to CurrentBuffer */
        CurrentBuffer += InputLength;
        /* subtract bytes consumed from bytes remaining in block */
        RemainingBlock -= InputLength;
        /* neg for resume decompression of the same block */
        InputLength = -RemainingBlock;
    }

    /* now CurrentBuffer points to the first comp byte
       of the file, so we can begin decompressing */

    /* Size = remaining uncomp bytes of the file to decompress */
    Size = Search->File->FileSize;
    while (Size > 0)
    {
        OutputLength = Size;
        DPRINT("Decompressing block at %x with RemainingBlock = %d, Size = %d\n",
               CurrentBuffer, RemainingBlock, Size);

        Status = CabinetContext->Codec->Uncompress(CabinetContext->Codec,
                                                   CurrentDestBuffer,
                                                   CurrentBuffer,
                                                   &InputLength,
                                                   &OutputLength);
        if (Status != CS_SUCCESS)
        {
            DPRINT("Cannot uncompress block\n");
            if (Status == CS_NOMEMORY)
                Status = CAB_STATUS_NOMEMORY;
            else
                Status = CAB_STATUS_INVALID_CAB;
            goto UnmapDestFile;
        }

        /* advance dest buffer by bytes produced */
        CurrentDestBuffer = (PVOID)((ULONG_PTR)CurrentDestBuffer + OutputLength);
        /* advance src buffer by bytes consumed */
        CurrentBuffer += InputLength;
        /* reduce remaining file bytes by bytes produced */
        Size -= OutputLength;
        /* reduce remaining block size by bytes consumed */
        RemainingBlock -= InputLength;
        if (Size > 0 && RemainingBlock == 0)
        {
            /* used up this block, move on to the next */
            DPRINT("Out of block data\n");
            CFData = (PCFDATA)CurrentBuffer;
            RemainingBlock = CFData->CompSize;
            CurrentBuffer = (unsigned char *)(CFData + 1) + CabinetContext->DataReserved;
            InputLength = RemainingBlock;
        }
    }

    Status = CAB_STATUS_SUCCESS;

UnmapDestFile:
    if (!CabinetContext->CreateFileHandler)
        NtUnmapViewOfSection(NtCurrentProcess(), DestFileBuffer);

CloseDestFileSection:
    if (!CabinetContext->CreateFileHandler)
        NtClose(DestFileSection);

CloseDestFile:
    if (!CabinetContext->CreateFileHandler)
        NtClose(DestFile);

    return Status;
}

/*
 * FUNCTION: Selects codec engine to use
 * ARGUMENTS:
 *     Id = Codec identifier
 */
VOID
CabinetSelectCodec(
    IN PCABINET_CONTEXT CabinetContext,
    IN ULONG Id)
{
    if (CabinetContext->CodecSelected)
    {
        if (Id == CabinetContext->CodecId)
            return;

        CabinetContext->CodecSelected = FALSE;
    }

    switch (Id)
    {
        case CAB_CODEC_RAW:
        {
            CabinetContext->Codec = &RawCodec;
            break;
        }

        case CAB_CODEC_MSZIP:
        {
            CabinetContext->Codec = &MSZipCodec;
            CabinetContext->Codec->ZStream.zalloc = MSZipAlloc;
            CabinetContext->Codec->ZStream.zfree = MSZipFree;
            CabinetContext->Codec->ZStream.opaque = (voidpf)0;
            break;
        }

        default:
            return;
    }

    CabinetContext->CodecId = Id;
    CabinetContext->CodecSelected = TRUE;
}

/*
 * FUNCTION: Set event handlers
 * ARGUMENTS:
 *     Overwrite = Handler called when a file is to be overwritten
 *     Extract = Handler called when a file is to be extracted
 *     DiskChange = Handler called when changing the disk
 */
VOID
CabinetSetEventHandlers(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCABINET_OVERWRITE Overwrite,
    IN PCABINET_EXTRACT Extract,
    IN PCABINET_DISK_CHANGE DiskChange,
    IN PCABINET_CREATE_FILE CreateFile)
{
    CabinetContext->OverwriteHandler = Overwrite;
    CabinetContext->ExtractHandler = Extract;
    CabinetContext->DiskChangeHandler = DiskChange;
    CabinetContext->CreateFileHandler = CreateFile;
}

/*
 * FUNCTION: Get pointer to cabinet reserved area. NULL if none
 */
PVOID
CabinetGetCabinetReservedArea(
    IN PCABINET_CONTEXT CabinetContext,
    OUT PULONG Size)
{
    if (CabinetContext->CabinetReservedArea != NULL)
    {
        if (Size != NULL)
        {
            *Size = CabinetContext->CabinetReserved;
        }

        return CabinetContext->CabinetReservedArea;
    }
    else
    {
        if (Size != NULL)
        {
            *Size = 0;
        }

        return NULL;
    }
}
