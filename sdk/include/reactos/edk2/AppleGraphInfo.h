/*
 * PROJECT:     ReactOS UEFI Support
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Apple Graphics Info Protocol
 * COPYRIGHT:   Copyright 2026 Sylas Hollander <distrohopper39b.business@gmail.com>
 */

/*
 * Implementation based on AppleGraphInfo.h from macosxbootloader, written by "tiamo"
 * which is licensed under BSD-3-Clause (https://spdx.org/licenses/BSD-3-Clause)
 * (see note in macosxbootloader README)
 */

#pragma once

#define APPLE_GRAPH_INFO_PROTOCOL_GUID {0xe316e100, 0x0751, 0x4c49, {0x90, 0x56, 0x48, 0x6c, 0x7e, 0x47, 0x29, 0x03}}

typedef struct _APPLE_GRAPH_INFO_PROTOCOL APPLE_GRAPH_INFO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *GET_INFO)(
    APPLE_GRAPH_INFO_PROTOCOL *This,
    UINT64 *BaseAddress,
    UINT64 *FrameBufferSize,
    UINT32 *BytesPerRow,
    UINT32 *Width,
    UINT32 *Height,
    UINT32 *Depth);

struct _APPLE_GRAPH_INFO_PROTOCOL
{
    GET_INFO GetInfo;
};
