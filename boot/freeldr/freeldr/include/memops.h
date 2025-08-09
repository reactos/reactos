/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     Safe memory operations for early boot environment
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#pragma once

/* Safe memory operations that avoid problematic compiler optimizations */

VOID
NTAPI
FrLdrZeroMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length);

VOID
NTAPI
FrLdrCopyMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length);

VOID
NTAPI
FrLdrFillMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ UCHAR Fill);

VOID
NTAPI
FrLdrMoveMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length);

/* 
 * Use these safe functions only where specifically needed in early boot.
 * Do NOT override standard functions globally as it causes issues after
 * memory manager initialization.
 */