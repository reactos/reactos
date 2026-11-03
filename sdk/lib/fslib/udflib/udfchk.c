/*
 * PROJECT:     ReactOS UDF filesystem library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check and repair a UDF 2.50 volume with the UDFS recovery core
 * COPYRIGHT:   Copyright 2026 Fuad Amin Ishtiak
 */

#include "compat.h"
#include <ntddstor.h>
#include <ntstrsafe.h>
#include "recovery_core.h"

typedef struct _UDF_CHECK_IO
{
    HANDLE Volume;
} UDF_CHECK_IO;

static int
CheckRead(void *Context, uint32_t Lba, uint8_t Sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    Offset.QuadPart = (LONGLONG)Lba * UDFS_RECOVERY_SECTOR_SIZE;
    Status = NtReadFile(((UDF_CHECK_IO *)Context)->Volume, NULL, NULL, NULL, &IoStatus,
                        Sector, UDFS_RECOVERY_SECTOR_SIZE, &Offset, NULL);
    return NT_SUCCESS(Status) && IoStatus.Information == UDFS_RECOVERY_SECTOR_SIZE ? 0 : -1;
}

static int
CheckWrite(void *Context, uint32_t Lba, const uint8_t Sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    Offset.QuadPart = (LONGLONG)Lba * UDFS_RECOVERY_SECTOR_SIZE;
    Status = NtWriteFile(((UDF_CHECK_IO *)Context)->Volume, NULL, NULL, NULL, &IoStatus,
                         (PVOID)Sector, UDFS_RECOVERY_SECTOR_SIZE, &Offset, NULL);
    return NT_SUCCESS(Status) && IoStatus.Information == UDFS_RECOVERY_SECTOR_SIZE ? 0 : -1;
}

static int
CheckFlush(void *Context)
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = NtFlushBuffersFile(((UDF_CHECK_IO *)Context)->Volume, &IoStatus);
    /* A dismounted or raw volume handle may not accept a flush; the writes
     * themselves already went to the device. */
    return (NT_SUCCESS(Status) || Status == STATUS_INVALID_DEVICE_REQUEST) ? 0 : -1;
}

static void *
CheckAllocate(void *Context, size_t Bytes)
{
    UNREFERENCED_PARAMETER(Context);
    return RtlAllocateHeap(RtlGetProcessHeap(), 0, Bytes);
}

static void
CheckFree(void *Context, void *Buffer)
{
    UNREFERENCED_PARAMETER(Context);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
}

static void
CheckSay(PFMIFSCALLBACK Callback, PCSTR Format, ...)
{
    char Message[192];
    TEXTOUTPUT Output;
    va_list Args;

    va_start(Args, Format);
    RtlStringCbVPrintfA(Message, sizeof(Message), Format, Args);
    va_end(Args);
    DbgPrint("UdfChkdsk: %s", Message);
    if (Callback)
    {
        Output.Lines = 1;
        Output.Output = Message;
        Callback(OUTPUT, 0, &Output);
    }
}

/* ECMA-167 1/7.3 timestamp in UTC. */
static void
CheckTimestamp(uint8_t Stamp[12])
{
    LARGE_INTEGER Now;
    TIME_FIELDS Time;

    NtQuerySystemTime(&Now);
    RtlTimeToTimeFields(&Now, &Time);
    Stamp[0] = 0x00;
    Stamp[1] = 0x10;    // type 1 (local time), offset 0 minutes
    Stamp[2] = (uint8_t)Time.Year;
    Stamp[3] = (uint8_t)(Time.Year >> 8);
    Stamp[4] = (uint8_t)Time.Month;
    Stamp[5] = (uint8_t)Time.Day;
    Stamp[6] = (uint8_t)Time.Hour;
    Stamp[7] = (uint8_t)Time.Minute;
    Stamp[8] = (uint8_t)Time.Second;
    Stamp[9] = (uint8_t)(Time.Milliseconds / 10);
    Stamp[10] = (uint8_t)((Time.Milliseconds % 10) * 10);
    Stamp[11] = 0;
}

BOOLEAN
NTAPI
UdfChkdsk(
    _In_ PUNICODE_STRING DriveRoot,
    _In_ PFMIFSCALLBACK Callback,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN CheckOnlyIfDirty,
    _In_ BOOLEAN ScanDrive,
    _In_opt_ PVOID pUnknown1,
    _In_opt_ PVOID pUnknown2,
    _In_opt_ PVOID pUnknown3,
    _In_opt_ PVOID pUnknown4,
    _Out_ PULONG ExitStatus)
{
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK IoStatus;
    GET_LENGTH_INFORMATION Length;
    DISK_GEOMETRY Geometry;
    UDF_CHECK_IO Io;
    UDFS_RECOVERY_READER Reader;
    UDFS_RECOVERY_PROBE Before, After;
    UDFS_RECOVERY_REPAIR_PLAN *Plan = NULL;
    UDFS_RECOVERY_PROBE_STATUS Probe;
    uint8_t Integrity, Stamp[12];
    NTSTATUS Status;
    BOOLEAN Locked = FALSE, Changed = FALSE;

    UNREFERENCED_PARAMETER(Verbose);
    UNREFERENCED_PARAMETER(ScanDrive);
    UNREFERENCED_PARAMETER(pUnknown1);
    UNREFERENCED_PARAMETER(pUnknown2);
    UNREFERENCED_PARAMETER(pUnknown3);
    UNREFERENCED_PARAMETER(pUnknown4);

    Io.Volume = NULL;
    InitializeObjectAttributes(&Attributes, DriveRoot, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Io.Volume,
                        FILE_GENERIC_READ | (FixErrors ? FILE_GENERIC_WRITE : 0) | SYNCHRONIZE,
                        &Attributes, &IoStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        goto Done;
    Status = NtDeviceIoControlFile(Io.Volume, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &Length, sizeof(Length));
    if (NT_SUCCESS(Status))
        Status = NtDeviceIoControlFile(Io.Volume, NULL, NULL, NULL, &IoStatus,
                                       IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                                       &Geometry, sizeof(Geometry));
    if (!NT_SUCCESS(Status))
        goto Done;
    if (Geometry.BytesPerSector != UDFS_RECOVERY_SECTOR_SIZE ||
        (ULONGLONG)Length.Length.QuadPart / UDFS_RECOVERY_SECTOR_SIZE > MAXULONG)
    {
        CheckSay(Callback, "UDF: only 512-byte sector disks can be checked.\n");
        Status = STATUS_NOT_SUPPORTED;
        goto Done;
    }
    /* Repairs need the volume to ourselves; a read-only check does not. */
    if (FixErrors)
    {
        Status = NtFsControlFile(Io.Volume, NULL, NULL, NULL, &IoStatus,
                                 FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
        if (!NT_SUCCESS(Status))
        {
            CheckSay(Callback, "UDF: the volume is in use and cannot be repaired now.\n");
            goto Done;
        }
        Locked = TRUE;
    }

    RtlZeroMemory(&Reader, sizeof(Reader));
    Reader.Context = &Io;
    Reader.ReadSector = CheckRead;
    Reader.WriteSector = CheckWrite;
    Reader.Flush = CheckFlush;
    Reader.Allocate = CheckAllocate;
    Reader.Free = CheckFree;
    Reader.SectorCount = (uint32_t)(Length.Length.QuadPart / UDFS_RECOVERY_SECTOR_SIZE);

    Probe = UdfsRecoveryPeekIntegrity(&Reader, &Integrity);
    if (Probe != UdfsRecoveryProbeValidLayout)
    {
        CheckSay(Callback, "UDF: this volume layout cannot be checked (%u).\n", (ULONG)Probe);
        Status = STATUS_NOT_SUPPORTED;
        goto Done;
    }
    if (Integrity != 0 && CheckOnlyIfDirty)
    {
        CheckSay(Callback, "UDF: the volume was shut down cleanly.\n");
        Status = STATUS_SUCCESS;
        goto Done;
    }

    CheckSay(Callback, "UDF: checking the volume...\n");
    Probe = UdfsRecoveryProbeLayout(&Reader, &Before);
    if (Probe == UdfsRecoveryProbeValidLayout)
    {
        CheckSay(Callback, "UDF: %lu files and %lu folders checked, no problems found.\n",
                 (ULONG)Before.FilesVisited, (ULONG)Before.DirectoriesVisited);
        if (Before.LvidIntegrityType == 1 || !FixErrors)
        {
            Status = STATUS_SUCCESS;
            goto Done;
        }
        /* Clean contents with an open integrity record: closing it is the
         * whole repair, done through the same plan below. */
    }
    else if (Probe != UdfsRecoveryProbeGraphMismatch && Probe != UdfsRecoveryProbeLvidMismatch)
    {
        CheckSay(Callback, "UDF: problems found that cannot be repaired (%u, check line %lu).\n",
                 (ULONG)Probe, (ULONG)Before.FailureLine);
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto Done;
    }
    else
    {
        CheckSay(Callback, "UDF: %lu blocks in use but unreferenced, %lu referenced but free.\n",
                 (ULONG)(Before.UnownedPhysicalBlocks + Before.UnownedMetadataBlocks),
                 (ULONG)(Before.ReferencedFreePhysicalBlocks + Before.ReferencedFreeMetadataBlocks));
        if (!FixErrors)
        {
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto Done;
        }
    }

    Plan = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*Plan));
    if (!Plan)
    {
        Status = STATUS_NO_MEMORY;
        goto Done;
    }
    CheckTimestamp(Stamp);
    Probe = UdfsRecoveryPrepareRepair(&Reader, &Before, Stamp, Plan);
    if (Probe != UdfsRecoveryProbeValidLayout)
    {
        CheckSay(Callback, "UDF: these problems cannot be repaired safely (%u).\n", (ULONG)Probe);
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto Done;
    }
    CheckSay(Callback, "UDF: repairing...\n");
    Changed = TRUE;
    Probe = UdfsRecoveryApplyRepair(&Reader, Plan, &After);
    if (Probe != UdfsRecoveryProbeValidLayout)
    {
        CheckSay(Callback, "UDF: the repair failed (%u).\n", (ULONG)Probe);
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto Done;
    }
    CheckSay(Callback, "UDF: the volume was repaired.\n");
    Status = STATUS_SUCCESS;

Done:
    if (Plan)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Plan);
    if (Io.Volume)
    {
        if (Locked)
        {
            /* Make the file system read the repaired structures afresh. */
            if (Changed)
                NtFsControlFile(Io.Volume, NULL, NULL, NULL, &IoStatus,
                                FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
            NtFsControlFile(Io.Volume, NULL, NULL, NULL, &IoStatus,
                            FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
        }
        NtClose(Io.Volume);
    }
    *ExitStatus = (ULONG)Status;
    return NT_SUCCESS(Status);
}
