/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Shared helper for deriving stable RAM disk signatures
 * COPYRIGHT:   Copyright 2025 Ahmed Arif <arif.ing@outlook.com>
 */

#pragma once

#include <freeldr.h>

static inline ULONG
RamDiskDeriveDiskSignature(IN PVOID BaseAddress,
                           IN ULONGLONG DiskSize)
{
    ULONG_PTR Address = (ULONG_PTR)BaseAddress;
    ULONG Signature = 0x52444B58u; /* 'RDKX' */

    Signature ^= (ULONG)(Address & 0xFFFFFFFFu);
#if defined(_M_AMD64) || defined(__x86_64__) || defined(_M_ARM64) || defined(__aarch64__)
    Signature ^= (ULONG)(Address >> 32);
#endif
    Signature ^= (ULONG)(DiskSize & 0xFFFFFFFFu);
    Signature ^= (ULONG)(DiskSize >> 32);

    if (Signature == 0 || Signature == 0xFFFFFFFFu)
        Signature ^= 0xA5A5A5A5u;

    return Signature;
}

