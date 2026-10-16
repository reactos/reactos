/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared memory management definitions for x86 / x64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* Macros for portable PTE modification */
#define MI_MAKE_PTE_CACHED(Pte)                                        \
    do                                                                 \
    {                                                                  \
        (Pte)->u.Hard.CacheDisable = 0;                                \
        (Pte)->u.Hard.WriteThrough = 0;                                \
    } while (0)

/* Both bits are set so the PAT index lands on UC, not UC-minus. */
#define MI_MAKE_PTE_NON_CACHED(Pte)                                    \
    do                                                                 \
    {                                                                  \
        (Pte)->u.Hard.CacheDisable = 1;                                \
        (Pte)->u.Hard.WriteThrough = 1;                                \
    } while (0)

/* PAT1 is programmed as WC, without PAT fall back to UC. */
#define MI_MAKE_PTE_WRITE_COMBINE(Pte)                                 \
    do                                                                 \
    {                                                                  \
        (Pte)->u.Hard.CacheDisable = !(KeFeatureBits & KF_PAT);        \
        (Pte)->u.Hard.WriteThrough = 1;                                \
    } while (0)
