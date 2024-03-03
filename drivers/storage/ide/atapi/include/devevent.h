/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device event queue definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define HSM_BEGIN(State) \
    switch ((DevExt)->WorkerContext.LocalState) \
    { \
        case 0:

#define HSM_END(DevExt) \
    }

#define HSM_SAVE_STATE(DevExt) \
    (DevExt)->WorkerContext.LocalState = __LINE__; break; case __LINE__:

#define HSM_ISSUE_COMMAND(DevExt) \
    (DevExt)->WorkerContext.Flags |= WORKER_NEED_REQUEST; \
    HSM_SAVE_STATE(DevExt);

#define HSM_ISSUE_NEXT_COMMAND(DevExt) \
    (DevExt)->WorkerContext.Flags |= WORKER_NEED_REQUEST; \
    break;

typedef enum _ATA_DEVICE_ACTION
{
    ACTION_NONE              = 0x00000000,
    ACTION_ERROR             = 0x00000001,
    ACTION_RESET             = 0x00000002,
    ACTION_DEV_CHANGE        = 0x00000004,
    ACTION_ENUM              = 0x00000010,
    ACTION_CONFIG            = 0x00000020,
    ACTION_INTERNAL_COMMAND  = 0x00000040,
    ACTION_ALL               = 0xFFFFFFFF,
} ATA_DEVICE_ACTION;

typedef enum _ATA_DEVICE_STATE
{
    DEVICE_STATE_NONE = 0,
    DEVICE_STATE_NEED_RECOVERY,
    DEVICE_STATE_REQUEST_SENSE,
    DEVICE_STATE_NCQ_RECOVERY,
    DEVICE_STATE_RECOVERY_DONE,
    DEVICE_STATE_RESET,
    DEVICE_STATE_RESETTING,
    DEVICE_STATE_ENUM,
    DEVICE_STATE_IDENTIFY,
    DEVICE_STATE_CONFIGURE,
    DEVICE_STATE_INTERNAL_COMMAND,
    DEVICE_STATE_NEXT_ACTION,
} ATA_DEVICE_STATE;

typedef enum _ATA_ERROR_LOG_VALUE
{
    EVENT_CODE_CRC_ERROR = 100,
    EVENT_CODE_BAD_STATE,
    EVENT_CODE_TIMEOUT,
    EVENT_CODE_DOWNSHIFT,
    EVENT_CODE_DMA_DISABLE,
} ATA_ERROR_LOG_VALUE;

typedef enum _ATA_CONNECTION_STATUS
{
    CONN_STATUS_NO_DEVICE,
    CONN_STATUS_DEV_UNKNOWN,
    CONN_STATUS_DEV_ATA,
    CONN_STATUS_DEV_ATAPI,
} ATA_CONNECTION_STATUS;

typedef struct _ATA_WORKER_CONTEXT
{
    ULONG Actions;
    ULONG HandledActions;
    volatile LONG PortWorkerStopped;

    ULONG Flags;
#define WORKER_IS_ACTIVE            0x00000001
#define WORKER_NEED_SYSTEM_THREAD   0x00000002
#define WORKER_NEED_REQUEST         0x00000004
#define WORKER_USE_FAILED_SLOT      0x00000010
#define WORKER_REQUEUE_REQUESTS     0x00000020
#define WORKER_DEVICE_CHANGE        0x00000040

    KEVENT EnumeratedEvent;
    KEVENT ConfiguredEvent;
    KEVENT CompletedEvent;
    ATA_CONNECTION_STATUS ConnectionStatus;
    BOOLEAN CommandSuccess;
    UCHAR State;
    USHORT LocalState;
    ULONG PausedSlotsBitmap;
    PATA_DEVICE_REQUEST CurrentRequest;
    KDPC Dpc;
    PATA_DEVICE_REQUEST OldRequest;
} ATA_WORKER_CONTEXT, *PATA_WORKER_CONTEXT;
