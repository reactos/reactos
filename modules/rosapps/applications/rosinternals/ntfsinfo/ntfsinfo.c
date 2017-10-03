/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NTFS Information tool
 * FILE:            cmdutils/ntfsinfo/ntfsinfo.c
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
#define ATTRIBUTE_TYPE_DATA 0x80
#define ATTRIBUTE_TYPE_END 0xFFFFFFFF

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

static TCHAR * MetaDataFiles[] = {
    _T("$MFT\t\t"),
    _T("$MFTMirr\t"),
    _T("$LogFile\t"),
    _T("$Volume\t\t"),
    _T("$AttrDef\t"),
    _T("."),
    _T("$Bitmap\t\t"),
    _T("$Boot\t\t"),
    _T("$BadClus\t"),
    _T("$Quota\t\t"),
    _T("$UpCase\t\t"),
    _T("$Extended\t"),
    NULL,
};

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    TCHAR VolumeName[] = _T("\\\\.\\C:");
    HANDLE VolumeHandle;
    NTFS_VOLUME_DATA_BUFFER VolumeInfo;
    DWORD LengthReturned;
    ULONGLONG VolumeSize;
    ULONGLONG MftClusters;
    UINT File = 0;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER OutputBuffer;

    if (argc > 1)
    {
        TCHAR Letter = argv[1][0];

        if ((Letter >= 'A' && Letter <= 'Z') ||
            (Letter >= 'a' && Letter <= 'z'))
        {
            VolumeName[4] = Letter;
        }
    }

    VolumeHandle = CreateFile(VolumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (VolumeHandle == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Failed opening the volume '%s' (%lx)\n"), VolumeName, GetLastError());
        return 1;
    }

    if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &VolumeInfo, sizeof(VolumeInfo), &LengthReturned, NULL))
    {
        _ftprintf(stderr, _T("Failed requesting volume '%s' data (%lx)\n"), VolumeName, GetLastError());
        CloseHandle(VolumeHandle);
        return 1;
    }

    if (LengthReturned < sizeof(VolumeInfo))
    {
        _ftprintf(stderr, _T("Failed reading volume '%s' data (%lx)\n"), VolumeName, GetLastError());
        CloseHandle(VolumeHandle);
        return 1;
    }

    _tprintf(_T("Volume Size\n-----------\n"));
    VolumeSize = VolumeInfo.TotalClusters.QuadPart * VolumeInfo.BytesPerCluster;
    _tprintf(_T("Volume size\t\t: %I64u MB\n"), VolumeSize >> 20);
    _tprintf(_T("Total sectors\t\t: %I64u\n"), VolumeInfo.NumberSectors.QuadPart);
    _tprintf(_T("Total clusters\t\t: %I64u\n"), VolumeInfo.TotalClusters.QuadPart);
    _tprintf(_T("Free clusters\t\t: %I64u\n"), VolumeInfo.FreeClusters.QuadPart);
    _tprintf(_T("Free space\t\t: %I64u MB (%u%% of drive)\n"), (VolumeInfo.FreeClusters.QuadPart * VolumeInfo.BytesPerCluster) >> 20, (VolumeInfo.FreeClusters.QuadPart * 100) / VolumeInfo.TotalClusters.QuadPart);

    _tprintf(_T("\nAllocation Size\n---------------\n"));
    _tprintf(_T("Bytes per sector\t: %lu\n"), VolumeInfo.BytesPerSector);
    _tprintf(_T("Bytes per cluster\t: %lu\n"), VolumeInfo.BytesPerCluster);
    _tprintf(_T("Bytes per MFT record:\t: %lu\n"), VolumeInfo.BytesPerFileRecordSegment);
    _tprintf(_T("Clusters per MFT record\t: %lu\n"), VolumeInfo.ClustersPerFileRecordSegment);

    _tprintf(_T("\nMFT Information\n---------------\n"));
    _tprintf(_T("MFT size\t\t: %I64u MB (%u%% of drive)\n"), VolumeInfo.MftValidDataLength.QuadPart >> 20, (VolumeInfo.MftValidDataLength.QuadPart * 100) / VolumeSize);
    _tprintf(_T("MFT start cluster\t: %I64u\n"), VolumeInfo.MftStartLcn.QuadPart);
    _tprintf(_T("MFT zone clusters\t: %I64u - %I64u\n"), VolumeInfo.MftZoneStart.QuadPart, VolumeInfo.MftZoneEnd.QuadPart);
    MftClusters = VolumeInfo.MftZoneEnd.QuadPart - VolumeInfo.MftZoneStart.QuadPart;
    _tprintf(_T("MFT zone size\t\t: %I64u MB (%u%% of drive)\n"), (MftClusters * VolumeInfo.BytesPerCluster) >> 20, (MftClusters * 100) / VolumeInfo.TotalClusters.QuadPart);
    _tprintf(_T("MFT mirror start\t: %I64u\n"), VolumeInfo.Mft2StartLcn.QuadPart);

    _tprintf(_T("\nMeta-Data files\n---------------\n"));
    OutputBuffer = HeapAlloc(GetProcessHeap(), 0, VolumeInfo.BytesPerFileRecordSegment + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER));
    while (MetaDataFiles[File] != NULL)
    {
        NTFS_FILE_RECORD_INPUT_BUFFER InputBuffer;
        PFILE_RECORD_HEADER FileRecord;
        PNTFS_ATTR_RECORD Attribute;
        ULONGLONG Size = 0;

        if (File == 5)
        {
            ++File;
            continue;
        }

        InputBuffer.FileReferenceNumber.QuadPart = File;
        if (!DeviceIoControl(VolumeHandle, FSCTL_GET_NTFS_FILE_RECORD, &InputBuffer, sizeof(InputBuffer),
                             OutputBuffer, VolumeInfo.BytesPerFileRecordSegment  + sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER),
                             &LengthReturned, NULL))
        {
            ++File;
            continue;
        }

        if (OutputBuffer->FileReferenceNumber.QuadPart != File)
        {
            ++File;
            continue;
        }

        FileRecord = (PFILE_RECORD_HEADER)OutputBuffer->FileRecordBuffer;
        if (FileRecord->Ntfs.Type != NRH_FILE_TYPE)
        {
            ++File;
            continue;
        }

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
        while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
               Attribute->Type != ATTRIBUTE_TYPE_END)
        {
            if (Attribute->Type == ATTRIBUTE_TYPE_DATA)
            {
                Size = (Attribute->IsNonResident) ? Attribute->NonResident.AllocatedSize : Attribute->Resident.ValueLength;
                break;
            }

            Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
        }

        _tprintf(_T("%s%I64u bytes\n"), MetaDataFiles[File], Size);

        ++File;
    }

    HeapFree(GetProcessHeap(), 0, OutputBuffer);
    CloseHandle(VolumeHandle);
    return 0;
}
