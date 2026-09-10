/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared memory management definitions for x86 / x64
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#pragma once

/* Macros for portable PTE modification */
#define MI_MAKE_PTE_CACHED(Pte) \
    do { (Pte)->u.Hard.WriteThrough = 0; (Pte)->u.Hard.CacheDisable = 0; } while (0)
/* Both WriteThrough and CacheDisable must be set to select UC in the PAT. */
#define MI_MAKE_PTE_NON_CACHED(Pte) \
    do { (Pte)->u.Hard.WriteThrough = 1; (Pte)->u.Hard.CacheDisable = 1; } while (0)
#define MI_MAKE_PTE_WRITE_COMBINE(Pte) \
    do { (Pte)->u.Hard.WriteThrough = 1; (Pte)->u.Hard.CacheDisable = 0; } while (0)
