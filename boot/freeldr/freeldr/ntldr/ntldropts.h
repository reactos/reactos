/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT Kernel Load Options Support Functions
 * COPYRIGHT:   Copyright 2020 Hermes Belusca-Maito
 */

#pragma once

PCSTR
NtLdrGetNextOption(
    IN OUT PCSTR* Options,
    OUT PULONG OptionLength OPTIONAL);

PCSTR
NtLdrGetOptionExN(
    IN PCSTR Options,
    IN PCCH OptionName,
    IN ULONG OptNameLength,
    OUT PULONG OptionLength OPTIONAL);

PCSTR
NtLdrGetOptionEx(
    IN PCSTR Options,
    IN PCSTR OptionName,
    OUT PULONG OptionLength OPTIONAL);

PCSTR
NtLdrGetOption(
    IN PCSTR Options,
    IN PCSTR OptionName);

VOID
NtLdrAddOptions(
    IN OUT PSTR LoadOptions,
    IN ULONG BufferSize,
    IN BOOLEAN Append,
    IN PCSTR NewOptions OPTIONAL);
