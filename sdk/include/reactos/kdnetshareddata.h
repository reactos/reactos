/*
 * PROJECT:     ReactOS Kernel Debugger over Network extension driver headers
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Provide the types needed to share the underlying transport state with Kernel Debugger extensions
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#if defined(_AMD64_)
#define MAX_HARDWARE_CONTEXT_SIZE (160*1024*1024)
#else
#define MAX_HARDWARE_CONTEXT_SIZE (16*1024*1024)
#endif

#define TRANSMIT_ASYNC  0x80000000
#define TRANSMIT_HANDLE 0x40000000
#define TRANSMIT_LAST   0x20000000
#define HANDLE_FLAGS    (TRANSMIT_ASYNC | TRANSMIT_HANDLE | TRANSMIT_LAST)

#define KDX_EXTENDED_INITIAL_CONNECT 0x1
#define KDX_FORCE_DHCP_OFF           0x2
#define KDX_VALID_FLAGS              (KDX_EXTENDED_INITIAL_CONNECT | KDX_FORCE_DHCP_OFF)

#define MAC_ADDRESS_SIZE 6
#define UNDI_DEFAULT_HARDWARE_CONTEXT_SIZE ((512 + 10) * 1024)

typedef struct _KDNET_SHARED_DATA
{
    PVOID Hardware;
    struct _DEBUG_DEVICE_DESCRIPTOR *Device;
    PUCHAR TargetMacAddress;
    ULONG LinkSpeed;
    ULONG LinkDuplex;
    PUCHAR LinkState;
    ULONG SerialBaudRate;
    ULONG Flags;
    UCHAR RestartKdnet;
    UCHAR Reserved[3];
} KDNET_SHARED_DATA, *PKDNET_SHARED_DATA;
