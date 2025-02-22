/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NTFS Information tool
 * FILE:            rosinternals/nfi/nfi.c
 * PURPOSE:         Query information from NTFS volume using FSCTL
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

typedef struct
{
    ULONG Type;
    USHORT UsaOffset;
    USHORT UsaCount;
    ULONGLONG Lsn;
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

#define NRH_FILE_TYPE  0x454C4946

typedef enum
{
    AttributeStandardInformation = 0x10,
    AttributeAttributeList = 0x20,
    AttributeFileName = 0x30,
    AttributeObjectId = 0x40,
    AttributeSecurityDescriptor = 0x50,
    AttributeVolumeName = 0x60,
    AttributeVolumeInformation = 0x70,
    AttributeData = 0x80,
    AttributeIndexRoot = 0x90,
    AttributeIndexAllocation = 0xA0,
    AttributeBitmap = 0xB0,
    AttributeReparsePoint = 0xC0,
    AttributeEAInformation = 0xD0,
    AttributeEA = 0xE0,
    AttributePropertySet = 0xF0,
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd = 0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct _FILE_RECORD_HEADER
{
    NTFS_RECORD_HEADER Ntfs;
    USHORT SequenceNumber;
    USHORT LinkCount;
    USHORT AttributeOffset;
    USHORT Flags;
    ULONG BytesInUse;
    ULONG BytesAllocated;
    ULONGLONG BaseFileRecord;
    USHORT NextAttributeNumber;
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

typedef struct
{
    ULONG Type;
    ULONG Length;
    UCHAR IsNonResident;
    UCHAR NameLength;
    USHORT NameOffset;
    USHORT Flags;
    USHORT Instance;
    union
    {
        struct
        {
            ULONG ValueLength;
            USHORT ValueOffset;
            UCHAR Flags;
            UCHAR Reserved;
        } Resident;
        struct
        {
            ULONGLONG LowestVCN;
            ULONGLONG HighestVCN;
            USHORT MappingPairsOffset;
            USHORT CompressionUnit;
            UCHAR Reserved[4];
            LONGLONG AllocatedSize;
            LONGLONG DataSize;
            LONGLONG InitializedSize;
            LONGLONG CompressedSize;
        } NonResident;
    };
} NTFS_ATTR_RECORD, *PNTFS_ATTR_RECORD;

typedef struct
{
    ULONGLONG DirectoryFileReferenceNumber;
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONG FileAttributes;
    union
    {
        struct
        {
            USHORT PackedEaSize;
            USHORT AlignmentOrReserved;
        } EaInfo;
        ULONG ReparseTag;
    } Extended;
    UCHAR NameLength;
    UCHAR NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

#define NTFS_FILE_NAME_POSIX 0
#define NTFS_FILE_NAME_WIN32 1
#define NTFS_FILE_NAME_DOS 2
#define NTFS_FILE_NAME_WIN32_AND_DOS 3

#define NTFS_FILE_MFT 0
#define NTFS_FILE_MFTMIRR 1
#define NTFS_FILE_LOGFILE 2
#define NTFS_FILE_VOLUME 3
#define NTFS_FILE_ATTRDEF 4
#define NTFS_FILE_ROOT 5
#define NTFS_FILE_BITMAP 6
#define NTFS_FILE_BOOT 7
#define NTFS_FILE_BADCLUS 8
#define NTFS_FILE_QUOTA 9
#define NTFS_FILE_UPCASE 10
#define NTFS_FILE_EXTEND 11

PWSTR KnownEntries[NTFS_FILE_EXTEND + 1] =
{
    _T("Master File Table ($Mft)"),
    _T("Master File Table Mirror ($MftMirr)"),
    _T("Log File ($LogFile)"),
    _T("DASD ($Volume)"),
    _T("Attribute Definition Table ($AttrDef)"),
    _T("Root Directory"),
    _T("Volume Bitmap ($BitMap)"),
    _T("Boot Sectors ($Boot)"),
    _T("Bad Cluster List ($BadClus)"),
    _T("Security ($Secure)"),
    _T("Upcase Table ($UpCase)"),
    _T("Extend Table ($Extend)")
};

#define NTFS_MFT_MASK 0x0000FFFFFFFFFFFFULL

#define NTFS_FILE_TYPE_DIRECTORY  0x10000000

typedef struct _NAME_CACHE_ENTRY
{
    struct _NAME_CACHE_ENTRY * Next;
    ULONGLONG MftId;
    ULONG NameLen;
    WCHAR Name[1];
} NAME_CACHE_ENTRY, *PNAME_CACHE_ENTRY;

PNAME_CACHE_ENTRY CacheHead = NULL;

void PrintUsage(void)
{
    /* FIXME */
}

PNAME_CACHE_ENTRY FindInCache(ULONGLONG MftId)
{
    PNAME_CACHE_ENTRY CacheEntry;

    for (CacheEntry = CacheHead; CacheEntry != NULL; CacheEntry = CacheEntry->Next)
    {
        if (MftId == CacheEntry->MftId)
        {
            return CacheEntry;
        }
    }

    return NULL;
}

PNAME_CACHE_ENTRY AddToCache(PWSTR Name, DWORD Length, ULONGLONG MftId)
{
    PNAME_CACHE_ENTRY CacheEntry;

    /* Don't add in cache if already there! */
    CacheEntry = FindInCache(MftId);
    if (CacheEntry != NULL)
    {
        return CacheEntry;
    }

    /* Allocate an entry big enough to store name and cache info */
    CacheEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(NAME_CACHE_ENTRY) + Length);
    if (CacheEntry == NULL)
    {
        return NULL;
    }

    /* Insert in head (likely more perf) */
    CacheEntry->Next = CacheHead;
    CacheHead = CacheEntry;
    /* Set up entry with full path */
    CacheEntry->MftId = MftId;
    CacheEntry->NameLen = Length;
    CopyMemory(CacheEntry->Name, Name, Length);

    return CacheEntry;
}

PNAME_CACHE_ENTRY HandleFile(HANDLE VolumeHandle, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, ULONGLONG Id, PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer, BOOLEAN Silent);

PNAME_CACHE_ENTRY PrintPrettyName(HANDLE VolumeHandle, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, PNTFS_ATTR_RECORD Attributes, PNTFS_ATTR_RECORD AttributesEnd, ULONGLONG MftId, BOOLEAN Silent)
{
    BOOLEAN FirstRun;
    PNTFS_ATTR_RECORD Attribute;

    FirstRun = TRUE;

    /* Setup name for "standard" files */
    if (MftId <= NTFS_FILE_EXTEND)
    {
        if (!Silent)
        {
            _tprintf(_T("%s\n"), KnownEntries[MftId]);
        }

        /* $Extend can contain entries, add it in cache */
        if (MftId == NTFS_FILE_EXTEND)
        {
            return AddToCache(L"\\$Extend", sizeof(L"\\$Extend") - sizeof(UNICODE_NULL), NTFS_FILE_EXTEND);
        }

        return NULL;
    }

    /* We'll first try to use the Win32 name
     * If we fail finding it, we'll loop again for any other name
     */
TryAgain:
    /* Loop all the attributes */
    Attribute = Attributes;
    while (Attribute < AttributesEnd && Attribute->Type != AttributeEnd)
    {
        WCHAR Display[MAX_PATH];
        PFILENAME_ATTRIBUTE Name;
        ULONGLONG ParentId;
        ULONG Length;
        PNAME_CACHE_ENTRY CacheEntry;

        /* Move to the next arg if:
         * - Not a file name
         * - Not resident (should never happen!)
         */
        if (Attribute->Type != AttributeFileName || Attribute->IsNonResident)
        {
            Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
            continue;
        }

        /* Get the attribute data */
        Name = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
        /* If not Win32, only accept if it wasn't the first run */
        if ((Name->NameType == NTFS_FILE_NAME_POSIX || Name->NameType == NTFS_FILE_NAME_DOS) && FirstRun)
        {
            Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
            continue;
        }

        /* We accepted that name, get the parent ID to setup name */
        ParentId = Name->DirectoryFileReferenceNumber & NTFS_MFT_MASK;
        /* If root, easy, just print \ */
        if (ParentId == NTFS_FILE_ROOT)
        {
            Display[0] = L'\\';
            CopyMemory(&Display[1], Name->Name, Name->NameLength * sizeof(WCHAR));
            Length = Name->NameLength + 1;
            Display[Length] = UNICODE_NULL;
        }
        /* Default case */
        else
        {
            /* Did we already cache the name? */
            CacheEntry = FindInCache(ParentId);

            /* It wasn't in cache? Try to get it in! */
            if (CacheEntry == NULL)
            {
                PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer;

                OutputBuffer = HeapAlloc(GetProcessHeap(), 0, VolumeInfo->BytesPerFileRecordSegment + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER));
                if (OutputBuffer != NULL)
                {
                    CacheEntry = HandleFile(VolumeHandle, VolumeInfo, ParentId, OutputBuffer, TRUE);
                    HeapFree(GetProcessHeap(), 0, OutputBuffer);
                }
            }

            /* Nothing written yet */
            Length = 0;

            /* We cached it */
            if (CacheEntry != NULL)
            {
                /* Set up name. The cache entry contains full path */
                Length = CacheEntry->NameLen / sizeof(WCHAR);
                CopyMemory(Display, CacheEntry->Name, CacheEntry->NameLen);
                Display[Length] = L'\\';
                ++Length;
            }
            else
            {
                _tprintf(_T("Parent: %I64d\n"), ParentId);
            }

            /* Copy our name */
            CopyMemory(Display + Length, Name->Name, Name->NameLength * sizeof(WCHAR));
            Length += Name->NameLength;
            Display[Length] = UNICODE_NULL;
        }

        if (!Silent)
        {
            /* Display the name */
            _tprintf(_T("%s\n"), Display);
        }

        /* Reset cache entry */
        CacheEntry = NULL;

        /* If that's a directory, put it in the cache */
        if (Name->FileAttributes & NTFS_FILE_TYPE_DIRECTORY)
        {
            CacheEntry = AddToCache(Display, Length * sizeof(WCHAR), MftId);
        }

        /* Now, just quit */
        FirstRun = FALSE;

        return CacheEntry;
    }

    /* If was first run (Win32 search), retry with other names */
    if (FirstRun)
    {
        FirstRun = FALSE;
        goto TryAgain;
    }

    /* If we couldn't find a name, print unknown */
    if (!Silent)
    {
        _tprintf(_T("(unknown/unnamed)\n"));
    }

    return NULL;
}

PUCHAR DecodeRun(PUCHAR DataRun, LONGLONG *DataRunOffset, ULONGLONG *DataRunLength)
{
    UCHAR DataRunOffsetSize;
    UCHAR DataRunLengthSize;
    CHAR i;

    /* Get the offset size (in bytes) of the run */
    DataRunOffsetSize = (*DataRun >> 4) & 0xF;
    /* Get the length size (in bytes) of the run */
    DataRunLengthSize = *DataRun & 0xF;

    /* Initialize values */
    *DataRunOffset = 0;
    *DataRunLength = 0;

    /* Move to next byte */
    DataRun++;

    /* First, extract (byte after byte) run length with the size extracted from header */
    for (i = 0; i < DataRunLengthSize; i++)
    {
        *DataRunLength += ((ULONG64)*DataRun) << (i * 8);
        /* Next byte */
        DataRun++;
    }

    /* If offset size is 0, return -1 to show that's sparse run */
    if (DataRunOffsetSize == 0)
    {
        *DataRunOffset = -1;
    }
    /* Otherwise, extract offset */
    else
    {
        /* Extract (byte after byte) run offset with the size extracted from header */
        for (i = 0; i < DataRunOffsetSize - 1; i++)
        {
            *DataRunOffset += ((ULONG64)*DataRun) << (i * 8);
            /* Next byte */
            DataRun++;
        }
        /* The last byte contains sign so we must process it different way. */
        *DataRunOffset = ((LONG64)(CHAR)(*(DataRun++)) << (i * 8)) + *DataRunOffset;
    }

    /* Return next run */
    return DataRun;
}

void PrintAttributeInfo(PNTFS_ATTR_RECORD Attribute, DWORD MaxSize)
{
    BOOL Known = TRUE;
    WCHAR AttributeName[0xFF + 3];

    /* First of all, try to get attribute name */
    if (Attribute->NameLength != 0 && Attribute->NameOffset < MaxSize && Attribute->NameOffset + Attribute->NameLength < MaxSize)
    {
        AttributeName[0] = L' ';
        CopyMemory(AttributeName + 1, (PUCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset), Attribute->NameLength * sizeof(WCHAR));
        AttributeName[Attribute->NameLength + 1] = L' ';
        AttributeName[Attribute->NameLength + 2] = UNICODE_NULL;
    }
    else
    {
        AttributeName[0] = L' ';
        AttributeName[1] = UNICODE_NULL;
    }

    /* Display attribute type, its name (if any) and whether it's resident */
    switch (Attribute->Type)
    {
        case AttributeFileName:
            _tprintf(_T("\t$FILE_NAME%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeStandardInformation:
            _tprintf(_T("\t$STANDARD_INFORMATION%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeData:
            _tprintf(_T("\t$DATA%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeBitmap:
            _tprintf(_T("\t$BITMAP%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeIndexRoot:
            _tprintf(_T("\t$INDEX_ROOT%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeIndexAllocation:
            _tprintf(_T("\t$INDEX_ALLOCATION%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeObjectId:
            _tprintf(_T("\t$OBJECT_ID%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeSecurityDescriptor:
            _tprintf(_T("\t$SECURITY_DESCRIPTOR%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeVolumeName:
            _tprintf(_T("\t$VOLUME_NAME%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeVolumeInformation:
            _tprintf(_T("\t$VOLUME_INFORMATION%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        case AttributeAttributeList:
            _tprintf(_T("\t$ATTRIBUTE_LIST%s(%s)\n"), AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            break;

        default:
            _tprintf(_T("\tUnknown (%x)%s(%s)\n"), Attribute->Type, AttributeName, (Attribute->IsNonResident ? _T("nonresident") : _T("resident")));
            Known = FALSE;
            break;
    }

    /* If attribute is non resident, display the logical sectors it covers */
    if (Known && Attribute->IsNonResident)
    {
        PUCHAR Run;
        ULONGLONG Offset = 0;

        /* Get the runs mapping */
        Run = (PUCHAR)((ULONG_PTR)Attribute + Attribute->NonResident.MappingPairsOffset);
        /* As long as header isn't 0x00, then, there's a run */
        while (*Run != 0)
        {
            LONGLONG CurrOffset;
            ULONGLONG CurrLen;

            /* Decode the run, and move to the next one */
            Run = DecodeRun(Run, &CurrOffset, &CurrLen);

            /* We don't print sparse runs */
            if (CurrOffset != -1)
            {
                Offset += CurrOffset;
                _tprintf(_T("\t\tlogical sectors %I64d-%I64d (0x%I64x-0x%I64x)\n"), Offset, Offset + CurrLen, Offset, Offset + CurrLen);
            }
        }
    }
}

PNAME_CACHE_ENTRY HandleFile(HANDLE VolumeHandle, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, ULONGLONG Id, PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer, BOOLEAN Silent)
{
    NTFS_FILE_RECORD_INPUT_BUFFER InputBuffer;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_RECORD Attribute, AttributesEnd;
    DWORD LengthReturned;
    PNAME_CACHE_ENTRY CacheEntry;

    /* Get the file record */
    InputBuffer.FileReferenceNumber.QuadPart = Id;
    if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_FILE_RECORD, &InputBuffer, sizeof(InputBuffer),
                         OutputBuffer, VolumeInfo->BytesPerFileRecordSegment  + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER),
                         &LengthReturned, NULL))
    {
        return NULL;
    }

    /* Don't deal with it if we already browsed it
     * FSCTL_GET_NTFS_FILE_RECORD always returns previous record if demanded
     * isn't allocated
     */
    if (OutputBuffer->FileReferenceNumber.QuadPart != Id)
    {
        return NULL;
    }

    /* Sanity check */
    FileRecord = (PFILE_RECORD_HEADER)OutputBuffer->FileRecordBuffer;
    if (FileRecord->Ntfs.Type != NRH_FILE_TYPE)
    {
        return NULL;
    }

    if (!Silent)
    {
        /* Print ID */
        _tprintf(_T("\nFile %I64d\n"), OutputBuffer->FileReferenceNumber.QuadPart);
    }

    /* Get attributes list */
    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    AttributesEnd = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse);

    /* Print the file name */
    CacheEntry = PrintPrettyName(VolumeHandle, VolumeInfo, Attribute, AttributesEnd, Id, Silent);

    if (!Silent)
    {
        /* And print attributes information for each attribute */
        while (Attribute < AttributesEnd && Attribute->Type != AttributeEnd)
        {
            PrintAttributeInfo(Attribute, VolumeInfo->BytesPerFileRecordSegment);
            Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
        }
    }

    return CacheEntry;
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    TCHAR VolumeName[] = _T("\\\\.\\C:");
    HANDLE VolumeHandle;
    NTFS_VOLUME_DATA_BUFFER VolumeInfo;
    DWORD LengthReturned;
    ULONGLONG File;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer;
    TCHAR Letter;
    PNAME_CACHE_ENTRY CacheEntry;

    if (argc == 1)
    {
        PrintUsage();
        return 0;
    }

    /* Setup volume name */
    Letter = argv[1][0];
    if ((Letter >= 'A' && Letter <= 'Z') ||
        (Letter >= 'a' && Letter <= 'z'))
    {
        VolumeName[4] = Letter;
    }

    /* Open volume */
    VolumeHandle = CreateFile(VolumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (VolumeHandle == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Failed opening the volume '%s' (%lx)\n"), VolumeName, GetLastError());
        return 1;
    }

    /* Get NTFS volume info */
    if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &VolumeInfo, sizeof(VolumeInfo), &LengthReturned, NULL))
    {
        _ftprintf(stderr, _T("Failed requesting volume '%s' data (%lx)\n"), VolumeName, GetLastError());
        CloseHandle(VolumeHandle);
        return 1;
    }

    /* Validate output */
    if (LengthReturned < sizeof(VolumeInfo))
    {
        _ftprintf(stderr, _T("Failed reading volume '%s' data (%lx)\n"), VolumeName, GetLastError());
        CloseHandle(VolumeHandle);
        return 1;
    }

    /* Allocate a buffer big enough to hold a file record */
    OutputBuffer = HeapAlloc(GetProcessHeap(), 0, VolumeInfo.BytesPerFileRecordSegment + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER));
    if (OutputBuffer == NULL)
    {
        _ftprintf(stderr, _T("Failed to allocate %Ix bytes\n"), VolumeInfo.BytesPerFileRecordSegment + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER));
        CloseHandle(VolumeHandle);
        return 1;
    }

    /* Forever loop, extract all the files! */
    for (File = 0;; ++File)
    {
        HandleFile(VolumeHandle, &VolumeInfo, File, OutputBuffer, FALSE);
    }

    /* Free memory! */
    while (CacheHead != NULL)
    {
        CacheEntry = CacheHead;
        CacheHead = CacheEntry->Next;
        HeapFree(GetProcessHeap(), 0, CacheEntry);
    }

    /* Cleanup and exit */
    HeapFree(GetProcessHeap(), 0, OutputBuffer);
    CloseHandle(VolumeHandle);
    return 0;
}
