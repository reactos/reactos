/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Port event queue definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/* Values are assigned by priority */
typedef enum _ATA_PORT_ACTION
{
    ACTION_PORT_RESET           = (1 << 0), // High priority
    ACTION_ENUM_PORT            = (1 << 1),
    ACTION_ENUM_DEVICE_NEW      = (1 << 2),
    ACTION_ENUM_DEVICE          = (1 << 3),
    ACTION_PORT_TIMING          = (1 << 4), // Set transfer timings after device enumeration only
    ACTION_DEVICE_CONFIG        = (1 << 5), // Also use the timing information for configuration
    ACTION_DEVICE_ERROR         = (1 << 6),
    ACTION_DEVICE_POWER         = (1 << 7),
} ATA_PORT_ACTION;

typedef enum _ATA_DEVICE_STATUS
{
    DEV_STATUS_NO_DEVICE,
    DEV_STATUS_NEW_DEVICE,
    DEV_STATUS_SAME_DEVICE,
    DEV_STATUS_FAILED,
} ATA_DEVICE_STATUS;

typedef enum _ATA_ERROR_LOG_VALUE
{
    EVENT_CODE_CRC_ERROR = 100,
    EVENT_CODE_BAD_STATE,
    EVENT_CODE_TIMEOUT,
    EVENT_CODE_DOWNSHIFT,
    EVENT_CODE_DMA_DISABLE,
    EVENT_CODE_NCQ_DISABLE,
} ATA_ERROR_LOG_VALUE;

typedef struct _ATA_WORKER_CONTEXT
{
    KSPIN_LOCK Lock;
    KEVENT ThreadEvent;
    volatile PATAPORT_DEVICE_EXTENSION EnumDevExt;
    volatile ULONG DeviceCount;
    volatile LONG EventsPending;
    ULONG Flags;
#define WORKER_FLAG_NEED_RESCAN                 0x00000001
#define WORKER_FLAG_COMPLETE_PORT_ENUM_EVENT    0x00000002
    ULONG PausedSlotsBitmap;
    ULONG BadDeviceBitmap;
    ULONG ResetRetryCount;
#if DBG
    ULONG StateLoopCount;
#endif
    KDPC Dpc;
    PATA_DEVICE_REQUEST FailedRequest;
    PATA_DEVICE_REQUEST OldRequest;
    KEVENT CompletionEvent;
    ATA_DEVICE_REQUEST InternalRequest;
    ATAPORT_IO_CONTEXT InternalDevice;
    KEVENT EnumerationEvent;
    KDPC NotificationDpc;
    PKTHREAD Thread;
} ATA_WORKER_CONTEXT, *PATA_WORKER_CONTEXT;

typedef struct _ATA_WORKER_DEVICE_CONTEXT
{
    volatile LONG EventsPending;
    volatile LONG EnumStatus;
    ULONG ResetRetryCount;
    ULONG Flags;
#define DEV_WORKER_FLAG_HOLD_REFERENCE          0x00000001
#define DEV_WORKER_FLAG_REMOVED                 0x00000002
    KEVENT EnumerationEvent;
    KEVENT ConfigureEvent;
} ATA_WORKER_DEVICE_CONTEXT, *PATA_WORKER_DEVICE_CONTEXT;
