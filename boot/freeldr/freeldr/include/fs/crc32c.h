/*
 * Copied from Linux kernel crypto/crc32c.c
 * Copyright (c) 2004 Cisco Systems, Inc.
 * Copyright (c) 2008 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#pragma once

/*
 * This is the CRC-32C table
 * Generated with:
 * width = 32 bits
 * poly = 0x1EDC6F41
 * reflect input bytes = true
 * reflect output bytes = true
 */

static u32 crc32c_table[256];

/*
 * Steps through buffer one byte at at time, calculates reflected
 * crc using table.
 */

static inline u32 crc32c_le(u32 crc, const char *data, size_t length)
{
    while (length--)
        crc = crc32c_table[(u8)(crc ^ *data++)] ^ (crc >> 8);

    return crc;
}

static inline void btrfs_init_crc32c(void)
{
    int i, j;
    u32 v;
    const u32 poly = 0x82F63B78; /* Bit-reflected CRC32C polynomial */

    for (i = 0; i < 256; i++) {
        v = i;
        for (j = 0; j < 8; j++) {
            v = (v >> 1) ^ ((v & 1) ? poly : 0);
        }
        crc32c_table[i] = v;
    }
}
