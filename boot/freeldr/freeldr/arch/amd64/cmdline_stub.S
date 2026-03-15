/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Command line buffer stub for AMD64
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF
 *
 * NOTE: This provides the cmdline symbol for AMD64 builds.
 * AMD64 doesn't support Multiboot v1 (32-bit only), so we don't include
 * the full multiboot.S which has R_386_32 relocations incompatible with LLD.
 */

#include <asm.inc>

#define CMDLINE_SIZE 256

.code64

PUBLIC cmdline
cmdline:
    .space CMDLINE_SIZE

END
