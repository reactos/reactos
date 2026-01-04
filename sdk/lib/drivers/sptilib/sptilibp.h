/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Private header of the ATA and SCSI Pass Through Interface for storage drivers
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/**
 * SPTI allocation tag.
 */
#define TAG_SPTI     'ITPS'

/**
 * Timeouts to wait for a request to be completed, in seconds.
 */
/*@{*/
#define PASSTHROUGH_CMD_TIMEOUT_MIN_SEC    1
#define PASSTHROUGH_CMD_TIMEOUT_MAX_SEC    (30 * 60 * 60) // 30 hours
/*@}*/

#define GET_IOCTL(IoStack)   ((IoStack)->Parameters.DeviceIoControl.IoControlCode)

typedef union _PASSTHROUGH_DATA
{
    PVOID Buffer;
    ULONG_PTR BufferOffset;
} PASSTHROUGH_DATA, *PPASSTHROUGH_DATA;

typedef struct _PASSTHROUGH_IRP_CONTEXT
{
    SCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
} PASSTHROUGH_IRP_CONTEXT, *PPASSTHROUGH_IRP_CONTEXT;
