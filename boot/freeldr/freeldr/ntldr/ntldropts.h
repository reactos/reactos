/*
 * PROJECT:     FreeLoader
 * LICENSE:     Dual-licensed:
 *              GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NT Kernel Load Options Support Functions
 * COPYRIGHT:   Copyright 2020-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

PCSTR
NtLdrGetNextOption(
    _Inout_ PCSTR* Options,
    _Out_opt_ PULONG OptionLength);

PCSTR
NtLdrGetOptionExN(
    _In_ PCSTR Options,
    _In_reads_(OptNameLength) PCCH OptionName,
    _In_ ULONG OptNameLength,
    _Out_opt_ PULONG OptionLength);

PCSTR
NtLdrGetOptionEx(
    _In_ PCSTR Options,
    _In_ PCSTR OptionName,
    _Out_opt_ PULONG OptionLength);

PCSTR
NtLdrGetOption(
    _In_ PCSTR Options,
    _In_ PCSTR OptionName);

VOID
NtLdrAddOptions(
    _Inout_updates_z_(BufferSize) PSTR Options,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Append,
    _In_opt_ PCSTR NewOptions);

VOID
NtLdrUpdateOptions(
    _Inout_updates_z_(BufferSize) PSTR LoadOptions,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Append,
    _In_opt_ PCSTR OptionsToAdd[],
    _In_opt_ PCSTR OptionsToRemove[]);
