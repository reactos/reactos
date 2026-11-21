/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VirtIO sound protocol definitions
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief VirtIO Sound Protocol Definitions
 *
 * This file contains the protocol definitions for the VirtIO Sound Driver.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#ifndef _VIRTIO_SND_PROTOCOL_H_
#define _VIRTIO_SND_PROTOCOL_H_

#include <ntddk.h>

/**
 * @brief VirtIO Sound Device ID
 */
#define VIRTIO_SND_DEVICE_ID 25

/**
 * @brief VirtIO Sound Feature: Controls
 */
#define VIRTIO_SND_F_CTLS 0

/**
 * @brief VirtIO Sound Queue: Control
 */
#define VIRTIO_SND_VQ_CONTROL 0

/**
 * @brief VirtIO Sound Queue: Event
 */
#define VIRTIO_SND_VQ_EVENT 1

/**
 * @brief VirtIO Sound Queue: TX (Playback)
 */
#define VIRTIO_SND_VQ_TX 2

/**
 * @brief VirtIO Sound Queue: RX (Capture)
 */
#define VIRTIO_SND_VQ_RX 3

/**
 * @brief Maximum number of VirtIO Sound queues
 */
#define VIRTIO_SND_VQ_MAX 4

/**
 * @brief VirtIO Sound Request: PCM Info
 */
#define VIRTIO_SND_R_PCM_INFO 0x0100

/**
 * @brief VirtIO Sound Request: PCM Set Parameters
 */
#define VIRTIO_SND_R_PCM_SET_PARAMS 0x0101

/**
 * @brief VirtIO Sound Request: PCM Prepare
 */
#define VIRTIO_SND_R_PCM_PREPARE 0x0102

/**
 * @brief VirtIO Sound Request: PCM Release
 */
#define VIRTIO_SND_R_PCM_RELEASE 0x0103

/**
 * @brief VirtIO Sound Request: PCM Start
 */
#define VIRTIO_SND_R_PCM_START 0x0104

/**
 * @brief VirtIO Sound Request: PCM Stop
 */
#define VIRTIO_SND_R_PCM_STOP 0x0105

/**
 * @brief VirtIO Sound Request: Channel Map Info
 */
#define VIRTIO_SND_R_CHMAP_INFO 0x0200

/**
 * @brief VirtIO Sound PCM Feature: Event XRUNs
 */
#define VIRTIO_SND_PCM_F_EVT_XRUNS (1UL << 4)

/**
 * @brief VirtIO Sound Event: PCM XRUN
 */
#define VIRTIO_SND_EVT_PCM_XRUN 0x1101

/**
 * @brief Maximum size of channel map
 */
#define VIRTIO_SND_CHMAP_MAX_SIZE 18

/**
 * @brief VirtIO Sound Direction: Output
 */
#define VIRTIO_SND_D_OUTPUT 0

/**
 * @brief VirtIO Sound Direction: Input
 */
#define VIRTIO_SND_D_INPUT 1

/**
 * @brief VirtIO Sound Channel Map: None
 */
#define VIRTIO_SND_CHMAP_NONE 0

/**
 * @brief VirtIO Sound Channel Map: Not Available
 */
#define VIRTIO_SND_CHMAP_NA 1

/**
 * @brief VirtIO Sound Channel Map: Mono
 */
#define VIRTIO_SND_CHMAP_MONO 2

/**
 * @brief VirtIO Sound Channel Map: Front Left
 */
#define VIRTIO_SND_CHMAP_FL 3

/**
 * @brief VirtIO Sound Channel Map: Front Right
 */
#define VIRTIO_SND_CHMAP_FR 4

/**
 * @brief VirtIO Sound Channel Map: Rear Left
 */
#define VIRTIO_SND_CHMAP_RL 5

/**
 * @brief VirtIO Sound Channel Map: Rear Right
 */
#define VIRTIO_SND_CHMAP_RR 6

/**
 * @brief VirtIO Sound Channel Map: Front Center
 */
#define VIRTIO_SND_CHMAP_FC 7

/**
 * @brief VirtIO Sound Channel Map: Low Frequency Effects
 */
#define VIRTIO_SND_CHMAP_LFE 8

/**
 * @brief VirtIO Sound Channel Map: Side Left
 */
#define VIRTIO_SND_CHMAP_SL 9

/**
 * @brief VirtIO Sound Channel Map: Side Right
 */
#define VIRTIO_SND_CHMAP_SR 10

/**
 * @brief VirtIO Sound Channel Map: Rear Center
 */
#define VIRTIO_SND_CHMAP_RC 11

/**
 * @brief VirtIO Sound Channel Map: Front Left of Center
 */
#define VIRTIO_SND_CHMAP_FLC 12

/**
 * @brief VirtIO Sound Channel Map: Front Right of Center
 */
#define VIRTIO_SND_CHMAP_FRC 13

/**
 * @brief VirtIO Sound Channel Map: Rear Left of Center
 */
#define VIRTIO_SND_CHMAP_RLC 14

/**
 * @brief VirtIO Sound Channel Map: Rear Right of Center
 */
#define VIRTIO_SND_CHMAP_RRC 15

/**
 * @brief VirtIO Sound Channel Map: Front Left Wide
 */
#define VIRTIO_SND_CHMAP_FLW 16

/**
 * @brief VirtIO Sound Channel Map: Front Right Wide
 */
#define VIRTIO_SND_CHMAP_FRW 17

/**
 * @brief VirtIO Sound Channel Map: Front Left High
 */
#define VIRTIO_SND_CHMAP_FLH 18

/**
 * @brief VirtIO Sound Channel Map: Front Center High
 */
#define VIRTIO_SND_CHMAP_FCH 19

/**
 * @brief VirtIO Sound Channel Map: Front Right High
 */
#define VIRTIO_SND_CHMAP_FRH 20

/**
 * @brief VirtIO Sound Channel Map: Top Center
 */
#define VIRTIO_SND_CHMAP_TC 21

/**
 * @brief VirtIO Sound Channel Map: Top Front Left
 */
#define VIRTIO_SND_CHMAP_TFL 22

/**
 * @brief VirtIO Sound Channel Map: Top Front Right
 */
#define VIRTIO_SND_CHMAP_TFR 23

/**
 * @brief VirtIO Sound Channel Map: Top Front Center
 */
#define VIRTIO_SND_CHMAP_TFC 24

/**
 * @brief VirtIO Sound Channel Map: Top Rear Left
 */
#define VIRTIO_SND_CHMAP_TRL 25

/**
 * @brief VirtIO Sound Channel Map: Top Rear Right
 */
#define VIRTIO_SND_CHMAP_TRR 26

/**
 * @brief VirtIO Sound Channel Map: Top Rear Center
 */
#define VIRTIO_SND_CHMAP_TRC 27

/**
 * @brief VirtIO Sound Channel Map: Top Front Left of Center
 */
#define VIRTIO_SND_CHMAP_TFLC 28

/**
 * @brief VirtIO Sound Channel Map: Top Front Right of Center
 */
#define VIRTIO_SND_CHMAP_TFRC 29

/**
 * @brief VirtIO Sound Channel Map: Top Side Left
 */
#define VIRTIO_SND_CHMAP_TSL 30

/**
 * @brief VirtIO Sound Channel Map: Top Side Right
 */
#define VIRTIO_SND_CHMAP_TSR 31

/**
 * @brief VirtIO Sound Channel Map: Left Rear of Center
 */
#define VIRTIO_SND_CHMAP_LLFE 32

/**
 * @brief VirtIO Sound Channel Map: Right Rear of Center
 */
#define VIRTIO_SND_CHMAP_RLFE 33

/**
 * @brief VirtIO Sound Channel Map: Back Center
 */
#define VIRTIO_SND_CHMAP_BC 34

/**
 * @brief VirtIO Sound Channel Map: Back Left of Center
 */
#define VIRTIO_SND_CHMAP_BLC 35

/**
 * @brief VirtIO Sound Channel Map: Back Right of Center
 */
#define VIRTIO_SND_CHMAP_BRC 36

/**
 * @brief VirtIO Sound PCM Format: Signed 16 bit
 */
#define VIRTIO_SND_PCM_FMT_S16 5

/**
 * @brief VirtIO Sound PCM Format: Signed 32 bit
 */
#define VIRTIO_SND_PCM_FMT_S32 17

/**
 * @brief VirtIO Sound PCM Rate: 44100 Hz
 */
#define VIRTIO_SND_PCM_RATE_44100 6

/**
 * @brief VirtIO Sound PCM Rate: 48000 Hz
 */
#define VIRTIO_SND_PCM_RATE_48000 7

/**
 * @brief VirtIO Sound PCM Rate: 96000 Hz
 */
#define VIRTIO_SND_PCM_RATE_96000 10

/**
 * @brief VirtIO Sound PCM Buffer Size in bytes
 */
#define VIRTIO_SND_PCM_BUFFER_BYTES 4096

/**
 * @brief VirtIO Sound PCM Period Size in bytes
 */
#define VIRTIO_SND_PCM_PERIOD_BYTES 1024

/**
 * @brief VirtIO Sound Status: OK
 */
#define VIRTIO_SND_S_OK 0x8000

/**
 * @brief VirtIO Sound Status: Bad Message
 */
#define VIRTIO_SND_S_BAD_MSG 0x8001

/**
 * @brief VirtIO Sound Status: Not Supported
 */
#define VIRTIO_SND_S_NOT_SUPP 0x8002

/**
 * @brief VirtIO Sound Status: I/O Error
 */
#define VIRTIO_SND_S_IO_ERR 0x8003

/**
 * @brief VirtIO Sound Configuration structure
 *
 * Contains device configuration information.
 */
typedef struct _VIRTIO_SND_CONFIG
{
    ULONG Jacks;       /**< Number of jacks */
    ULONG Streams;     /**< Number of streams */
    ULONG ChannelMaps; /**< Number of channel maps */
    ULONG Controls;    /**< Number of controls */
} VIRTIO_SND_CONFIG, *PVIRTIO_SND_CONFIG;

/**
 * @brief VirtIO Sound Header structure
 *
 * Common header for all VirtIO sound messages.
 */
typedef struct _VIRTIO_SND_HDR
{
    ULONG Code; /**< Message code */
} VIRTIO_SND_HDR, *PVIRTIO_SND_HDR;

/**
 * @brief VirtIO Sound Event structure
 *
 * An event sent from the device to the driver.
 */
typedef struct _VIRTIO_SND_EVENT
{
    VIRTIO_SND_HDR Header; /**< Event header */
    ULONG Data;            /**< Event data */
} VIRTIO_SND_EVENT, *PVIRTIO_SND_EVENT;

/**
 * @brief VirtIO Sound PCM Header structure
 *
 * Header for PCM related messages.
 */
typedef struct _VIRTIO_SND_PCM_HDR
{
    VIRTIO_SND_HDR Header; /**< PCM header */
    ULONG StreamId;        /**< Stream identifier */
} VIRTIO_SND_PCM_HDR, *PVIRTIO_SND_PCM_HDR;

/**
 * @brief VirtIO Sound PCM Set Parameters structure
 *
 * Used to set PCM parameters for a stream.
 */
typedef struct _VIRTIO_SND_PCM_SET_PARAMS
{
    VIRTIO_SND_PCM_HDR Header; /**< PCM header */
    ULONG BufferBytes;         /**< Buffer size in bytes */
    ULONG PeriodBytes;         /**< Period size in bytes */
    ULONG Features;            /**< Features supported */
    UCHAR Channels;            /**< Number of channels */
    UCHAR Format;              /**< PCM format */
    UCHAR Rate;                /**< Sample rate */
    UCHAR Padding;             /**< Padding bytes */
} VIRTIO_SND_PCM_SET_PARAMS, *PVIRTIO_SND_PCM_SET_PARAMS;

/**
 * @brief VirtIO Sound Query Info structure
 *
 * Query information about streams or channel maps.
 */
typedef struct _VIRTIO_SND_QUERY_INFO
{
    VIRTIO_SND_HDR Header; /**< Query header */
    ULONG StartId;         /**< Starting ID */
    ULONG Count;           /**< Number of entries */
    ULONG Size;            /**< Size of each entry */
} VIRTIO_SND_QUERY_INFO, *PVIRTIO_SND_QUERY_INFO;

/**
 * @brief VirtIO Sound Info structure
 *
 * Base structure for information queries.
 */
typedef struct _VIRTIO_SND_INFO
{
    ULONG HdaFunctionNodeId; /**< HDA function node ID */
} VIRTIO_SND_INFO, *PVIRTIO_SND_INFO;

/**
 * @brief VirtIO Sound PCM Info structure
 *
 * Information about a PCM stream
 */
typedef struct _VIRTIO_SND_PCM_INFO
{
    VIRTIO_SND_INFO Header; /**< Info header */
    ULONG Features;         /**< Stream features */
    ULONGLONG Formats;      /**< Supported formats */
    ULONGLONG Rates;        /**< Supported rates */
    UCHAR Direction;        /**< Stream direction */
    UCHAR ChannelsMin;      /**< Minimum channels */
    UCHAR ChannelsMax;      /**< Maximum channels */
    UCHAR Padding[5];       /**< Padding bytes */
} VIRTIO_SND_PCM_INFO, *PVIRTIO_SND_PCM_INFO;

/**
 * @brief VirtIO Sound Channel Map Info structure
 *
 * Information about channel mapping
 */
typedef struct _VIRTIO_SND_CHMAP_INFO
{
    VIRTIO_SND_INFO Header;                     /**< Info header */
    UCHAR Direction;                            /**< Channel direction */
    UCHAR Channels;                             /**< Number of channels */
    UCHAR Positions[VIRTIO_SND_CHMAP_MAX_SIZE]; /**< Channel positions */
} VIRTIO_SND_CHMAP_INFO, *PVIRTIO_SND_CHMAP_INFO;

/**
 * @brief Static assertions for structure sizes
 */
C_ASSERT(sizeof(VIRTIO_SND_QUERY_INFO) == 16);
C_ASSERT(sizeof(VIRTIO_SND_EVENT) == 8);
C_ASSERT(sizeof(VIRTIO_SND_PCM_HDR) == 8);
C_ASSERT(sizeof(VIRTIO_SND_PCM_SET_PARAMS) == 24);
C_ASSERT(sizeof(VIRTIO_SND_PCM_INFO) == 32);
C_ASSERT(sizeof(VIRTIO_SND_CHMAP_INFO) == 24);

/**
 * @brief VirtIO Sound PCM Transfer structure
 *
 * Used for PCM transfer operations.
 */
typedef struct _VIRTIO_SND_PCM_XFER
{
    ULONG StreamId; /**< Stream identifier */
} VIRTIO_SND_PCM_XFER, *PVIRTIO_SND_PCM_XFER;

/**
 * @brief VirtIO Sound PCM Status structure
 *
 * Status information for PCM operations.
 */
typedef struct _VIRTIO_SND_PCM_STATUS
{
    ULONG Status;       /**< Operation status */
    ULONG LatencyBytes; /**< Latency in bytes */
} VIRTIO_SND_PCM_STATUS, *PVIRTIO_SND_PCM_STATUS;

#endif
