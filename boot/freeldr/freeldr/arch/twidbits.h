/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Bit twiddling helpers
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * Based on http://www.graphics.stanford.edu/~seander/bithacks.html
 * and other sources.
 */

// HAVE___BUILTIN_POPCOUNT

#pragma once

/**
 * @brief   Return the number of bits set in a 32-bit integer.
 * @note    Equivalent to __popcnt().
 **/
FORCEINLINE
ULONG
CountNumberOfBits(
    _In_ UINT32 n)
{
#ifdef HAVE___BUILTIN_POPCOUNT
    return __popcnt(n);
#else
    n -= ((n >> 1) & 0x55555555);
    n =  (((n >> 2) & 0x33333333) + (n & 0x33333333));
#if 0
    n =  (((n >> 4) + n) & 0x0f0f0f0f);
    n += (n >> 8);
    n += (n >> 16);
    return (n & 0x3f);
#else
    return (((n >> 4) + n) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
#endif /* HAVE___BUILTIN_POPCOUNT */
}
