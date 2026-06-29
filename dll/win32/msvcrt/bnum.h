/*
 * Copyright 2020 Piotr Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_BNUM_H
#define __WINE_BNUM_H

#define EXP_BITS 11
#define MANT_BITS 53

static const int p10s[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

#define LIMB_DIGITS 9           /* each DWORD stores up to 9 digits */
#define LIMB_MAX 1000000000     /* 10^9 */

#define BNUM_PREC64 128         /* data size needed to store 64-bit double */
#define BNUM_PREC80 2048        /* data size needed to store 80-bit double */

/* bnum represents real number with fixed decimal point */
struct bnum {
    int b; /* least significant digit position */
    int e; /* most significant digit position + 1 */
    int size; /* data buffer size in DWORDS (power of 2) */
    DWORD data[1]; /* circular buffer, base 10 number */
};

static inline int bnum_idx(struct bnum *b, int idx)
{
    return idx & (b->size - 1);
}

/* Returns TRUE if new most significant limb was added */
static inline BOOL bnum_lshift(struct bnum *b, int shift)
{
    DWORD rest = 0;
    ULONGLONG tmp;
    int i;

    /* The limbs number can change by up to 1 so shift <= 29 */
    assert(shift <= 29);

    for(i=b->b; i<b->e; i++) {
        tmp = ((ULONGLONG)b->data[bnum_idx(b, i)] << shift) + rest;
        rest = tmp / LIMB_MAX;
        b->data[bnum_idx(b, i)] = tmp % LIMB_MAX;

        if(i == b->b && !b->data[bnum_idx(b, i)])
            b->b++;
    }

    if(rest) {
        b->data[bnum_idx(b, b->e)] = rest;
        b->e++;

        if(bnum_idx(b, b->b) == bnum_idx(b, b->e)) {
            if(b->data[bnum_idx(b, b->b)]) b->data[bnum_idx(b, b->b+1)] |= 1;
            b->b++;
        }
        return TRUE;
    }
    return FALSE;
}

/* Returns TRUE if most significant limb was removed */
static inline BOOL bnum_rshift(struct bnum *b, int shift)
{
    DWORD tmp, rest = 0;
    BOOL ret = FALSE;
    int i;

    /* Compute LIMB_MAX << shift without accuracy loss */
    assert(shift <= 9);

    for(i=b->e-1; i>=b->b; i--) {
        tmp = b->data[bnum_idx(b, i)] & ((1<<shift)-1);
        b->data[bnum_idx(b, i)] = (b->data[bnum_idx(b, i)] >> shift) + rest;
        rest = (LIMB_MAX >> shift) * tmp;
        if(i==b->e-1 && !b->data[bnum_idx(b, i)]) {
            b->e--;
            ret = TRUE;
        }
    }

    if(rest) {
        if(bnum_idx(b, b->b-1) == bnum_idx(b, b->e)) {
            if(rest) b->data[bnum_idx(b, b->b)] |= 1;
        } else {
            b->b--;
            b->data[bnum_idx(b, b->b)] = rest;
        }
    }
    return ret;
}

static inline void bnum_mult(struct bnum *b, int mult)
{
    DWORD rest = 0;
    ULONGLONG tmp;
    int i;

    assert(mult <= LIMB_MAX);

    for(i=b->b; i<b->e; i++) {
        tmp = ((ULONGLONG)b->data[bnum_idx(b, i)] * mult) + rest;
        rest = tmp / LIMB_MAX;
        b->data[bnum_idx(b, i)] = tmp % LIMB_MAX;

        if(i == b->b && !b->data[bnum_idx(b, i)])
            b->b++;
    }

    if(rest) {
        b->data[bnum_idx(b, b->e)] = rest;
        b->e++;

        if(bnum_idx(b, b->b) == bnum_idx(b, b->e)) {
            if(b->data[bnum_idx(b, b->b)]) b->data[bnum_idx(b, b->b+1)] |= 1;
            b->b++;
        }
    }
}

#endif /* __WINE_BNUM_H */
