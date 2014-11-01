/*
 * PROJECT:         ReactOS NTFS tests/dump
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Query information from NTFS volume using FSCTL and dumps $MFT
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <windows.h>
#include <stdio.h>

typedef struct
{
    ULONG Type;
    USHORT UsaOffset;
    USHORT UsaCount;
    ULONGLONG Lsn;
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

#define NRH_FILE_TYPE  0x454C4946

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
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONG FileAttribute;
    ULONG AlignmentOrReserved[3];
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;

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
    ULONG AlignmentOrReserved;
    UCHAR NameLength;
    UCHAR NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

int main(int argc, char **argv)
{
    static WCHAR VolumeName[] = L"\\\\.\\E:";
    HANDLE VolumeHandle;
    struct {
        NTFS_VOLUME_DATA_BUFFER Data;
        NTFS_EXTENDED_VOLUME_DATA Extended;
    } VolumeInfo;
    DWORD LengthReturned;
    NTFS_FILE_RECORD_INPUT_BUFFER InputBuffer;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer;
    DWORD OutputLength;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_RECORD Attribute;
    DWORD k;

    VolumeHandle = CreateFileW(VolumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (VolumeHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed opening volume!\n");
        return -1;
    }

    if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &VolumeInfo, sizeof(VolumeInfo), &LengthReturned, NULL))
    {
        fprintf(stderr, "Failed requesting volume data!\n");
        CloseHandle(VolumeHandle);
        return -1;
    }

    printf("Using NTFS: %u.%u\n", VolumeInfo.Extended.MajorVersion, VolumeInfo.Extended.MinorVersion);

    InputBuffer.FileReferenceNumber.QuadPart = 0LL;
    OutputLength = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + VolumeInfo.Data.BytesPerFileRecordSegment;
    OutputBuffer = malloc(OutputLength);
    if (OutputBuffer == NULL)
    {
        fprintf(stderr, "Allocation failure!\n");
        CloseHandle(VolumeHandle);
    }

    if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_FILE_RECORD, &InputBuffer, sizeof(InputBuffer), OutputBuffer, OutputLength, &LengthReturned, NULL))
    {
        fprintf(stderr, "Failed opening $MFT!\n");
        free(OutputBuffer);
        CloseHandle(VolumeHandle);
        return -1;
    }

    if (OutputBuffer->FileReferenceNumber.QuadPart != 0LL)
    {
        fprintf(stderr, "Wrong entry read! %I64x\n", OutputBuffer->FileReferenceNumber.QuadPart);
        free(OutputBuffer);
        CloseHandle(VolumeHandle);
        return -1;
    }

    FileRecord = (PFILE_RECORD_HEADER)OutputBuffer->FileRecordBuffer;
    if (FileRecord->Ntfs.Type != NRH_FILE_TYPE)
    {
        fprintf(stderr, "Wrong type read! %lx\n", FileRecord->Ntfs.Type);
        free(OutputBuffer);
        CloseHandle(VolumeHandle);
        return -1;
    }

    printf("File record size: %lu. Read file record size: %lu\n", VolumeInfo.Data.BytesPerFileRecordSegment, OutputBuffer->FileRecordLength);
    printf("SequenceNumber: %u\n", FileRecord->SequenceNumber);
    printf("LinkCount: %u\n", FileRecord->LinkCount);
    printf("AttributeOffset: %u\n", FileRecord->AttributeOffset);
    printf("Flags: %u\n", FileRecord->Flags);
    printf("BytesInUse: %lu\n", FileRecord->BytesInUse);
    printf("BytesAllocated: %lu\n", FileRecord->BytesAllocated);
    printf("BaseFileRecord: %I64x\n", FileRecord->BaseFileRecord);
    printf("NextAttributeNumber: %u\n", FileRecord->NextAttributeNumber);

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
           Attribute->Type != AttributeEnd)
    {
        PSTANDARD_INFORMATION StdInfo;
        PFILENAME_ATTRIBUTE FileName;

        if (!Attribute->IsNonResident)
        {
            switch (Attribute->Type)
            {
                case AttributeStandardInformation:
                    StdInfo = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
                    printf("\t$STANDARD_INFORMATION:\n");
                    printf("\tCreationTime: %I64u\n", StdInfo->CreationTime);
                    printf("\tChangeTime: %I64u\n", StdInfo->ChangeTime);
                    printf("\tLastWriteTime: %I64u\n", StdInfo->LastWriteTime);
                    printf("\tLastAccessTime: %I64u\n", StdInfo->LastAccessTime);
                    printf("\tFileAttribute: %lu\n", StdInfo->FileAttribute);
                    break;

                case AttributeFileName:
                    FileName = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
                    printf("\t$FILE_NAME:\n");
                    printf("\tDirectoryFileReferenceNumber: %I64u\n", FileName->DirectoryFileReferenceNumber);
                    printf("\tCreationTime: %I64u\n", FileName->CreationTime);
                    printf("\tChangeTime: %I64u\n", FileName->ChangeTime);
                    printf("\tLastWriteTime: %I64u\n", FileName->LastWriteTime);
                    printf("\tLastAccessTime: %I64u\n", FileName->LastAccessTime);
                    printf("\tAllocatedSize: %I64u\n", FileName->AllocatedSize);
                    printf("\tDataSize: %I64u\n", FileName->DataSize);
                    printf("\tFileAttributes: %lu\n", FileName->FileAttributes);
                    printf("\tAlignmentOrReserved: %lu\n", FileName->AlignmentOrReserved);
                    printf("\tNameLength: %u\n", FileName->NameLength);
                    printf("\tNameType: %u\n", FileName->NameType);
                    printf("\tName: ");
                    for (k = 0; k < FileName->NameLength; ++k)
                    {
                        wprintf(L"%C", FileName->Name[k]);
                    }
                    printf("\n");
                    break;
            }
        }

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    free(OutputBuffer);
    CloseHandle(VolumeHandle);

    return 0;
}
