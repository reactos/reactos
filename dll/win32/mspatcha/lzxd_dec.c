/*
 * LZXD decoder
 *
 * Copyright 2019 Conor McCarthy
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
 *
 * TODO
 * - Implememnt interleaved decoding
 */

#include "config.h"

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "patchapi.h"

#include "lzxd_dec.h"

WINE_DEFAULT_DEBUG_CHANNEL(mspatcha);


#define ELEM_SIZE 2
#define MAX_CODE_LEN 16
#define MAX_ALIGN_CODE_LEN 7
#define PRE_LEN_BITS 4
#define MAX_PRE_CODE_LEN ((1 << PRE_LEN_BITS) - 1)
#define MAIN_TABLE_SIZE (1 << MAX_CODE_LEN)
#define ALIGN_TABLE_SIZE (1 << MAX_ALIGN_CODE_LEN)
#define HUFF_ERROR 0xFFFF
#define REP_COUNT 3
#define MAX_POS_SLOTS 290
#define ALIGN_CODE_COUNT 8
#define PRE_LEN_CODE_COUNT 20
#define MAIN_CODE_COUNT(slots) (256 + 8 * (slots))
#define MAX_MAIN_CODES MAIN_CODE_COUNT(MAX_POS_SLOTS)
#define LEN_CODE_COUNT 249
#define MAX_CHUNK_UNCOMPRESSED_SIZE 0x8000
#define E8_TRANSFORM_LIMIT_BITS 30
#define E8_TRANSFORM_DEAD_ZONE 10

#define my_min(a, b) ((a) < (b) ? (a) : (b))

struct LZXD_dec {
    /* use byte pointers instead of uint16 for simplicity on uncompressed
     * chunks, and the stream is not 16-bit aligned anyway */
    const BYTE *stream_buf;
    /* the next word to load into the bit cache */
    const BYTE *src;
    /* location of the next chunk size field */
    const BYTE *chunk_end;
    /* position in the output where the maximum allowed decompressed chunk size is reached */
    size_t uncomp_chunk_end;
    /* end of the input */
    const BYTE *stream_end;
    /* bit cache */
    UINT32 bits;
    /* number of unused bits in the cache starting from bit 0 */
    unsigned bit_pos;
    /* number of padding bits added trying to read at the chunk end */
    unsigned tail_bits;
    /* repeat matches */
    size_t reps[REP_COUNT];
    /* distance slot count is required for loading code lengths */
    unsigned dist_slot_count;
    /* huffman code lengths */
    BYTE align_lengths[ALIGN_CODE_COUNT];
    BYTE main_lengths[MAX_MAIN_CODES];
    BYTE len_lengths[LEN_CODE_COUNT];
    UINT16 align_table[ALIGN_TABLE_SIZE];
    UINT16 main_table[MAIN_TABLE_SIZE];
    UINT16 len_table[MAIN_TABLE_SIZE];
};

/* PA19 container format is unaligned, so the LZXD stream is not aligned either.
 * None of this is super optimized but it's fast enough for installer work.
 */
static inline UINT16 read_uint16(struct LZXD_dec *dec)
{
    /* bounds check was done before calling */
    UINT16 u = dec->src[0] | (dec->src[1] << 8);
    dec->src += ELEM_SIZE;
    return u;
}

/* load the next chunk size, reset bit_pos and set up limits
 */
static int init_chunk(struct LZXD_dec *dec, size_t index, size_t buf_limit)
{
    UINT32 chunk_size;

    if (dec->src + ELEM_SIZE > dec->stream_end)
        return -1;

    /* error if tail padding bits were decoded as input */
    if (dec->bit_pos < dec->tail_bits)
        return -1;

    chunk_size = read_uint16(dec);

    dec->chunk_end = dec->src + chunk_size;
    if (dec->chunk_end > dec->stream_end)
        return -1;

    dec->bit_pos = 0;
    dec->tail_bits = 0;

    dec->uncomp_chunk_end = my_min(buf_limit, index + MAX_CHUNK_UNCOMPRESSED_SIZE);

    return 0;
}

/* ensure at least 17 bits are loaded but do not advance
 */
static inline void prime_bits(struct LZXD_dec *dec)
{
    while (dec->bit_pos < 17)
    {
        if (dec->src + ELEM_SIZE <= dec->chunk_end)
        {
            dec->bits = (dec->bits << 16) | read_uint16(dec);
        }
        else
        {
            /* Need to pad at the end of the chunk to decode the last codes.
               In an error state, 0xFFFF sends the decoder down the right
               side of the huffman tree to error out sooner. */
            dec->bits = (dec->bits << 16) | 0xFFFF;
            dec->tail_bits += 16;
        }
        dec->bit_pos += 16;
    }
}

/* read and advance n bits
 */
static inline UINT32 read_bits(struct LZXD_dec *dec, unsigned n)
{
    UINT32 bits;

    dec->bit_pos -= n;
    bits = dec->bits >> dec->bit_pos;
    bits &= ((1 << n) - 1);

    while (dec->bit_pos < 17)
    {
        if (dec->src + ELEM_SIZE <= dec->chunk_end)
        {
            dec->bits = (dec->bits << 16) | read_uint16(dec);
        }
        else
        {
            /* tail padding */
            dec->bits = (dec->bits << 16) | 0xFFFF;
            dec->tail_bits += 16;
        }
        dec->bit_pos += 16;
    }

    return bits;
}

/* read n bits but do not advance
 */
static inline UINT32 peek_bits(struct LZXD_dec *dec, unsigned n)
{
    UINT32 bits = dec->bits >> (dec->bit_pos - n);
    return bits & ((1 << n) - 1);
}

static inline void advance_bits(struct LZXD_dec *dec, unsigned length)
{
    dec->bit_pos -= length;
    prime_bits(dec);
}

/* read a 16-bit aligned UINT32
 */
static UINT32 read_uint32(struct LZXD_dec *dec)
{
    UINT32 u = 0;
    unsigned n = 0;

    assert((dec->bit_pos & 0xF) == 0);

    while (dec->bit_pos)
    {
        dec->bit_pos -= 16;
        u |= ((dec->bits >> dec->bit_pos) & 0xFFFF) << n;
        n += 16;
    }
    while (n < 32 && dec->src + ELEM_SIZE <= dec->chunk_end)
    {
        u |= read_uint16(dec) << n;
        n += 16;
    }

    return u;
}

static int make_huffman_codes(unsigned *codes, const BYTE *lengths, unsigned count)
{
    unsigned len_count[MAX_CODE_LEN + 1];
    unsigned next_code[MAX_CODE_LEN + 1];
    unsigned i;
    unsigned code = 0;

    memset(len_count, 0, sizeof(len_count));
    for (i = 0; i < count; ++i)
        ++len_count[lengths[i]];
    len_count[0] = 0;

    for (i = 1; i <= MAX_CODE_LEN; ++i)
    {
        code = (code + len_count[i - 1]) << 1;
        next_code[i] = code;
    }
    for (i = 0; i < count; i++)
    {
        unsigned len = lengths[i];
        if (len)
        {
            /* test for bad code tree */
            if (next_code[len] >= (1U << len))
                return -1;

            codes[i] = next_code[len];
            ++next_code[len];
        }
    }
    return 0;
}

void make_decode_table(UINT16 *table, const unsigned *codes,
    const BYTE *lengths, unsigned max_len, unsigned count)
{
    const size_t table_size = (size_t)1 << max_len;
    size_t i;

    for (i = 0; i < table_size; i++)
        table[i] = HUFF_ERROR;

    for (i = 0; i < count; i++) if (lengths[i])
    {
        BYTE diff = (BYTE)max_len - lengths[i];
        size_t n = codes[i] << diff;
        size_t end = n + ((size_t)1 << diff);
        for (; n < end; ++n)
            table[n] = (UINT16)i;
    }
}

#define ret_if_failed(r_) do { int err_ = r_; if(err_) return err_; } while(0)

static int decode_lengths(struct LZXD_dec *dec,
    BYTE *lengths, unsigned index, unsigned count)
{
    unsigned codes[PRE_LEN_CODE_COUNT];
    BYTE pre_lens[PRE_LEN_CODE_COUNT];
    size_t i;
    unsigned repeats = 1;

    for (i = 0; i < PRE_LEN_CODE_COUNT; ++i)
        pre_lens[i] = (BYTE)read_bits(dec, PRE_LEN_BITS);

    ret_if_failed(make_huffman_codes(codes, pre_lens, PRE_LEN_CODE_COUNT));
    make_decode_table(dec->main_table, codes, pre_lens, MAX_PRE_CODE_LEN, PRE_LEN_CODE_COUNT);

    while (index < count)
    {
        UINT32 bits = peek_bits(dec, MAX_PRE_CODE_LEN);
        UINT16 sym = dec->main_table[bits];
        if (sym == HUFF_ERROR)
            return -1;
        advance_bits(dec, pre_lens[sym]);

        if (sym < 17)
        {
            sym = (lengths[index] + 17 - sym) % 17;
            do
            {
                lengths[index] = (BYTE)sym;
                ++index;
                --repeats;
            } while (repeats && index < count);

            repeats = 1;
        }
        else if (sym < 19)
        {
            unsigned zeros;

            sym -= 13;
            zeros = read_bits(dec, sym) + (1 << sym) - 12;
            do
            {
                lengths[index] = 0;
                ++index;
                --zeros;
            } while (zeros && index < count);
        }
        else
        {
            /* the repeat count applies to the next symbol */
            repeats = 4 + read_bits(dec, 1);
        }
    }
    return 0;
}

/* distance decoder for block_type == 1
 */
static size_t decode_dist_verbatim(struct LZXD_dec *dec, unsigned dist_slot)
{
    size_t dist;
    unsigned footer_bits = 17;

    if (dist_slot < 38)
    {
        footer_bits = (dist_slot >> 1) - 1;
        dist = ((size_t)2 + (dist_slot & 1)) << footer_bits;
    }
    else
    {
        dist = ((size_t)1 << 19) + ((size_t)1 << 17) * (dist_slot - 38);
    }
    return dist + read_bits(dec, footer_bits);
}

/* distance decoder for block_type == 2
 */
static size_t decode_dist_aligned(struct LZXD_dec *dec, unsigned dist_slot)
{
    size_t dist;
    unsigned footer_bits = 17;

    if (dist_slot < 38)
    {
        footer_bits = (dist_slot >> 1) - 1;
        dist = ((size_t)2 + (dist_slot & 1)) << footer_bits;
    }
    else
    {
        dist = ((size_t)1 << 19) + ((size_t)1 << 17) * (dist_slot - 38);
    }
    if (footer_bits >= 3)
    {
        UINT32 bits;
        UINT16 sym;

        footer_bits -= 3;
        if (footer_bits)
            dist += read_bits(dec, footer_bits) << 3;

        bits = peek_bits(dec, MAX_ALIGN_CODE_LEN);
        sym = dec->align_table[bits];
        if (sym == HUFF_ERROR)
            return ~(size_t)0;
        advance_bits(dec, dec->align_lengths[sym]);

        dist += sym;
    }
    else
    {
        dist += read_bits(dec, footer_bits);
    }
    return dist;
}

static inline void align_16_or_maybe_advance_anyway(struct LZXD_dec *dec)
{
    dec->bit_pos &= 0x30;
    /* The specification requires 16 bits of zero padding in some cases where the stream is already aligned, but
     * the logic behind the choice to pad any particular block is undefined (it's a feature!). Fortunately it
     * seems always to coincide with a bit_pos of 0x20, but 0x20 doesn't always mean padding, so we test for zero
     * too. A remote chance of failure may still exist but I've never seen one occur. */
    if (dec->bit_pos == 0x20 && (dec->bits >> 16) == 0)
        dec->bit_pos = 0x10;
}

static int copy_uncompressed(struct LZXD_dec *dec, BYTE *base, size_t *index_ptr, size_t buf_limit, UINT32 block_size)
{
    size_t index = *index_ptr;
    size_t end = index + block_size;
    size_t realign;

    if (end > buf_limit)
        return -1;
    /* save the current alignment */
    realign = (dec->src - dec->stream_buf) & 1;

    while (dec->src < dec->stream_end)
    {
        /* now treat the input as an unaligned byte stream */
        size_t to_copy = my_min(end - index, dec->uncomp_chunk_end - index);
        to_copy = my_min(to_copy, (size_t)(dec->stream_end - dec->src));

        memcpy(base + index, dec->src, to_copy);
        index += to_copy;
        dec->src += to_copy;

        if (index == end)
        {
            /* realign at the end of the block */
            dec->src += ((dec->src - dec->stream_buf) & 1) ^ realign;
            /* fill the bit cache for block header decoding */
            prime_bits(dec);
            break;
        }
        /* chunk sizes are also unaligned */
        ret_if_failed(init_chunk(dec, index, buf_limit));
    }
    *index_ptr = index;
    return 0;
}

static int prime_next_chunk(struct LZXD_dec *dec, size_t index, size_t buf_limit)
{
    if (dec->src < dec->chunk_end)
        return -1;
    ret_if_failed(init_chunk(dec, index, buf_limit));
    prime_bits(dec);
    return 0;
}

#define MAX_LONG_MATCH_CODE_LEN 3
#define LONG_MATCH_TABLE_SIZE (1 << MAX_LONG_MATCH_CODE_LEN)

struct long_match {
    BYTE code_len;
    unsigned extra_bits;
    unsigned base;
};

static const struct long_match long_match_table[LONG_MATCH_TABLE_SIZE] = {
    {1, 8,  0x101},
    {1, 8,  0x101},
    {1, 8,  0x101},
    {1, 8,  0x101},
    {2, 10, 0x201},
    {2, 10, 0x201},
    {3, 12, 0x601},
    {3, 15, 0x101}
};

static int decode_lzxd_block(struct LZXD_dec *dec, BYTE *base, size_t predef_size, size_t *index_ptr, size_t buf_limit)
{
    unsigned codes[MAX_MAIN_CODES];
    unsigned main_code_count;
    UINT32 block_type;
    UINT32 block_size;
    size_t i;
    size_t block_limit;
    size_t index = *index_ptr;
    size_t (*dist_decoder)(struct LZXD_dec *dec, unsigned dist_slot);

    if (index >= dec->uncomp_chunk_end && prime_next_chunk(dec, index, buf_limit))
        return -1;

    block_type = read_bits(dec, 3);

    /* check for invalid block types */
    if (block_type == 0 || block_type > 3)
        return -1;

    block_size = read_bits(dec, 8);
    block_size = (block_size << 8) | read_bits(dec, 8);
    block_size = (block_size << 8) | read_bits(dec, 8);

    if (block_type == 3)
    {
        /* uncompressed block */
        align_16_or_maybe_advance_anyway(dec);
        /* must have run out of coffee at the office */
        for (i = 0; i < REP_COUNT; ++i)
        {
            dec->reps[i] = read_uint32(dec);
            if (dec->reps[i] == 0)
                return -1;
        }
        /* copy the block to output */
        return copy_uncompressed(dec, base, index_ptr, buf_limit, block_size);
    }
    else if (block_type == 2)
    {
        /* distance alignment decoder will be used */
        for (i = 0; i < ALIGN_CODE_COUNT; ++i)
            dec->align_lengths[i] = read_bits(dec, 3);
    }

    main_code_count = MAIN_CODE_COUNT(dec->dist_slot_count);
    ret_if_failed(decode_lengths(dec, dec->main_lengths, 0, 256));
    ret_if_failed(decode_lengths(dec, dec->main_lengths, 256, main_code_count));
    ret_if_failed(decode_lengths(dec, dec->len_lengths, 0, LEN_CODE_COUNT));

    dist_decoder = (block_type == 2) ? decode_dist_aligned : decode_dist_verbatim;

    if (block_type == 2)
    {
        ret_if_failed(make_huffman_codes(codes, dec->align_lengths, ALIGN_CODE_COUNT));
        make_decode_table(dec->align_table, codes, dec->align_lengths, MAX_ALIGN_CODE_LEN, ALIGN_CODE_COUNT);
    }

    ret_if_failed(make_huffman_codes(codes, dec->main_lengths, main_code_count));
    make_decode_table(dec->main_table, codes, dec->main_lengths, MAX_CODE_LEN, main_code_count);

    ret_if_failed(make_huffman_codes(codes, dec->len_lengths, LEN_CODE_COUNT));
    make_decode_table(dec->len_table, codes, dec->len_lengths, MAX_CODE_LEN, LEN_CODE_COUNT);

    block_limit = my_min(buf_limit, index + block_size);

    while (index < block_limit)
    {
        UINT32 bits;
        UINT16 sym;

        if (index >= dec->uncomp_chunk_end && prime_next_chunk(dec, index, buf_limit))
            return -1;

        bits = peek_bits(dec, MAX_CODE_LEN);
        sym = dec->main_table[bits];
        if (sym == HUFF_ERROR)
            return -1;
        advance_bits(dec, dec->main_lengths[sym]);

        if (sym < 256)
        {
            /* literal */
            base[index] = (BYTE)sym;
            ++index;
        }
        else {
            size_t length;
            size_t dist;
            size_t end;
            unsigned dist_slot;

            sym -= 256;
            length = (sym & 7) + 2;
            dist_slot = sym >> 3;

            if (length == 9)
            {
                /* extra length bits */
                bits = peek_bits(dec, MAX_CODE_LEN);
                sym = dec->len_table[bits];
                if (sym == HUFF_ERROR)
                    return -1;
                advance_bits(dec, dec->len_lengths[sym]);

                length += sym;
            }
            dist = dist_slot;
            if (dist_slot > 3)
            {
                /* extra distance bits */
                dist = dist_decoder(dec, dist_slot);
                if (dist == ~(size_t)0)
                    return -1;
            }
            if (length == 257)
            {
                /* extra-long match length */
                bits = peek_bits(dec, MAX_LONG_MATCH_CODE_LEN);
                advance_bits(dec, long_match_table[bits].code_len);

                length = long_match_table[bits].base;
                length += read_bits(dec, long_match_table[bits].extra_bits);
            }
            if (dist < 3)
            {
                /* repeat match */
                size_t rep = dist;
                dist = dec->reps[dist];
                dec->reps[rep] = dec->reps[0];
            }
            else
            {
                dist -= REP_COUNT - 1;
                dec->reps[2] = dec->reps[1];
                dec->reps[1] = dec->reps[0];
            }
            dec->reps[0] = dist;

            while (dist > index && length && index < block_limit)
            {
                /* undocumented: the encoder assumes an imaginary buffer
                 * of zeros exists before the start of the real buffer */
                base[index] = 0;
                ++index;
                --length;
            }

            end = my_min(index + length, block_limit);
            while (index < end)
            {
                base[index] = base[index - dist];
                ++index;
            }
        }
    }
    /* error if tail padding bits were decoded as input */
    if (dec->bit_pos < dec->tail_bits)
        return -1;

    *index_ptr = index;
    return 0;
}

static void reverse_e8_transform(BYTE *decode_buf, ptrdiff_t len, ptrdiff_t e8_file_size)
{
    ptrdiff_t limit = my_min((ptrdiff_t)1 << E8_TRANSFORM_LIMIT_BITS, len);
    ptrdiff_t i;

    for (i = 0; i < limit; )
    {
        ptrdiff_t end = my_min(i + MAX_CHUNK_UNCOMPRESSED_SIZE - E8_TRANSFORM_DEAD_ZONE,
            limit - E8_TRANSFORM_DEAD_ZONE);
        ptrdiff_t next = i + MAX_CHUNK_UNCOMPRESSED_SIZE;

        for (; i < end; ++i)
        {
            if (decode_buf[i] == 0xE8)
            {
                ptrdiff_t delta;
                ptrdiff_t value = (ptrdiff_t)decode_buf[i + 1] |
                    decode_buf[i + 2] << 8 |
                    decode_buf[i + 3] << 16 |
                    decode_buf[i + 4] << 24;

                if (value >= -i && value < e8_file_size)
                {
                    if (value >= 0)
                        delta = value - i;
                    else
                        delta = value + e8_file_size;

                    decode_buf[i + 1] = (BYTE)delta;
                    decode_buf[i + 2] = (BYTE)(delta >> 8);
                    decode_buf[i + 3] = (BYTE)(delta >> 16);
                    decode_buf[i + 4] = (BYTE)(delta >> 24);
                }
                i += 4;
            }
        }
        i = next;
    }
}

DWORD decode_lzxd_stream(const BYTE *src, const size_t input_size,
    BYTE *dst, const size_t output_size,
    const size_t predef_size,
    DWORD large_window,
    PPATCH_PROGRESS_CALLBACK progress_fn,
    PVOID  progress_ctx)
{
    struct LZXD_dec *dec;
    const BYTE *end = src + input_size;
    size_t buf_size = predef_size + output_size;
    UINT32 e8;
    UINT32 e8_file_size = 0;
    DWORD err = ERROR_SUCCESS;

    TRACE("decoding stream of size %u to size %u, starting at %u\n",
        (unsigned)input_size, (unsigned)output_size, (unsigned)predef_size);

    if (input_size == 0)
        return (output_size == 0) ?  ERROR_SUCCESS : ERROR_PATCH_CORRUPT;

    if (progress_fn != NULL && !progress_fn(progress_ctx, 0, (ULONG)output_size))
        return ERROR_CANCELLED;

    dec = heap_alloc(sizeof(*dec));
    if (dec == NULL)
        return ERROR_OUTOFMEMORY;

    memset(dec->main_lengths, 0, sizeof(dec->main_lengths));
    memset(dec->len_lengths, 0, sizeof(dec->len_lengths));
    dec->reps[0] = 1;
    dec->reps[1] = 1;
    dec->reps[2] = 1;

    /* apparently the window size is not recorded and must be deduced from the file sizes */
    {
        unsigned max_window = large_window ? MAX_LARGE_WINDOW : MAX_NORMAL_WINDOW;
        size_t window = (size_t)1 << 17;
        /* round up the old file size per the lzxd spec - correctness verified by fuzz tests */
        size_t total = (predef_size + 0x7FFF) & ~0x7FFF;
        unsigned delta;

        total += output_size;
        dec->dist_slot_count = 34;
        while (window < total && window < ((size_t)1 << 19))
        {
            dec->dist_slot_count += 2;
            window <<= 1;
        }
        delta = 4;
        while (window < total && window < max_window)
        {
            dec->dist_slot_count += delta;
            delta <<= 1;
            window <<= 1;
        }
        TRACE("setting window to 0x%X\n", (unsigned)window);
    }

    dec->bit_pos = 0;
    dec->tail_bits = 0;
    dec->stream_buf = src;
    dec->src = src;
    dec->stream_end = end;
    dec->chunk_end = dec->src;

    /* load the first chunk size and set the end pointer */
    if(init_chunk(dec, predef_size, buf_size))
    {
        err = ERROR_PATCH_DECODE_FAILURE;
        goto free_dec;
    }

    /* fill the bit cache */
    prime_bits(dec);

    e8 = read_bits(dec, 1);
    if (e8)
    {
        /* E8 transform was used */
        e8_file_size = read_bits(dec, 16) << 16;
        e8_file_size |= read_bits(dec, 16);
        TRACE("E8 transform detected; file size %u\n", e8_file_size);
    }

    {
        size_t index = predef_size;
        while (dec->src < dec->stream_end && index < buf_size)
        {
            if (decode_lzxd_block(dec, dst, predef_size, &index, buf_size))
            {
                err = ERROR_PATCH_DECODE_FAILURE;
                goto free_dec;
            }
            if (progress_fn != NULL && !progress_fn(progress_ctx, (ULONG)(index - predef_size), (ULONG)output_size))
            {
                err = ERROR_CANCELLED;
                goto free_dec;
            }
        }
    }

    if (e8)
        reverse_e8_transform(dst + predef_size, output_size, e8_file_size);

free_dec:
    heap_free(dec);

    return err;
}
