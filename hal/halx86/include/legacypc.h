/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Legacy PC Header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyPCCreateArbiter(_In_ PDEVICE_OBJECT BusFdo);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyPCQueryArbInterface(_Out_writes_bytes_(Size) PVOID Interface,
                              _In_                     ULONG Size,
                              _Out_                    PULONG Length);
