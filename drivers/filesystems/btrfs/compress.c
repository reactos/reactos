/* Copyright (c) Mark Harmstone 2016-17
 * Copyright (c) Reimar Doeffinger 2006
 * Copyright (c) Markus Oberhumer 1996
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

// Portions of the LZO decompression code here were cribbed from code in
// libavcodec, also under the LGPL. Thank you, Reimar Doeffinger.

// The LZO compression code comes from v0.22 of lzo, written way back in
// 1996, and available here:
// https://www.ibiblio.org/pub/historic-linux/ftp-archives/sunsite.unc.edu/Sep-29-1996/libs/lzo-0.22.tar.gz
// Modern versions of lzo are licensed under the GPL, but the very oldest
// versions are under the LGPL and hence okay to use here.

#include "btrfs_drv.h"

#define Z_SOLO
#define ZLIB_INTERNAL

#ifndef __REACTOS__
#include "zlib/zlib.h"
#include "zlib/inftrees.h"
#include "zlib/inflate.h"
#else
#include <zlib.h>
#endif

#define ZSTD_STATIC_LINKING_ONLY

#include "zstd/zstd.h"

#define LZO_PAGE_SIZE 4096

typedef struct {
    uint8_t* in;
    uint32_t inlen;
    uint32_t inpos;
    uint8_t* out;
    uint32_t outlen;
    uint32_t outpos;
    bool error;
    void* wrkmem;
} lzo_stream;

#define LZO1X_MEM_COMPRESS ((uint32_t) (16384L * sizeof(uint8_t*)))

#define M1_MAX_OFFSET 0x0400
#define M2_MAX_OFFSET 0x0800
#define M3_MAX_OFFSET 0x4000
#define M4_MAX_OFFSET 0xbfff

#define MX_MAX_OFFSET (M1_MAX_OFFSET + M2_MAX_OFFSET)

#define M1_MARKER 0
#define M2_MARKER 64
#define M3_MARKER 32
#define M4_MARKER 16

#define _DV2(p, shift1, shift2) (((( (uint32_t)(p[2]) << shift1) ^ p[1]) << shift2) ^ p[0])
#define DVAL_NEXT(dv, p) dv ^= p[-1]; dv = (((dv) >> 5) ^ ((uint32_t)(p[2]) << (2*5)))
#define _DV(p, shift) _DV2(p, shift, shift)
#define DVAL_FIRST(dv, p) dv = _DV((p), 5)
#define _DINDEX(dv, p) ((40799u * (dv)) >> 5)
#define DINDEX(dv, p) (((_DINDEX(dv, p)) & 0x3fff) << 0)
#define UPDATE_D(dict, cycle, dv, p) dict[DINDEX(dv, p)] = (p)
#define UPDATE_I(dict, cycle, index, p) dict[index] = (p)

#define LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, max_offset) \
    ((void*) m_pos < (void*) in || \
    (m_off = (uint8_t*) ip - (uint8_t*) m_pos) <= 0 || \
    m_off > max_offset)

#define LZO_BYTE(x) ((unsigned char) (x))

#define ZSTD_ALLOC_TAG 0x6474737a // "zstd"

// needs to be the same as Linux (fs/btrfs/zstd.c)
#define ZSTD_BTRFS_MAX_WINDOWLOG 17

static void* zstd_malloc(void* opaque, size_t size);
static void zstd_free(void* opaque, void* address);

#ifndef __REACTOS__
ZSTD_customMem zstd_mem = { .customAlloc = zstd_malloc, .customFree = zstd_free, .opaque = NULL };
#else
ZSTD_customMem zstd_mem = { zstd_malloc, zstd_free, NULL };
#endif

static uint8_t lzo_nextbyte(lzo_stream* stream) {
    uint8_t c;

    if (stream->inpos >= stream->inlen) {
        stream->error = true;
        return 0;
    }

    c = stream->in[stream->inpos];
    stream->inpos++;

    return c;
}

static int lzo_len(lzo_stream* stream, int byte, int mask) {
    int len = byte & mask;

    if (len == 0) {
        while (!(byte = lzo_nextbyte(stream))) {
            if (stream->error) return 0;

            len += 255;
        }

        len += mask + byte;
    }

    return len;
}

static void lzo_copy(lzo_stream* stream, int len) {
    if (stream->inpos + len > stream->inlen) {
        stream->error = true;
        return;
    }

    if (stream->outpos + len > stream->outlen) {
        stream->error = true;
        return;
    }

    do {
        stream->out[stream->outpos] = stream->in[stream->inpos];
        stream->inpos++;
        stream->outpos++;
        len--;
    } while (len > 0);
}

static void lzo_copyback(lzo_stream* stream, uint32_t back, int len) {
    if (stream->outpos < back) {
        stream->error = true;
        return;
    }

    if (stream->outpos + len > stream->outlen) {
        stream->error = true;
        return;
    }

    do {
        stream->out[stream->outpos] = stream->out[stream->outpos - back];
        stream->outpos++;
        len--;
    } while (len > 0);
}

static NTSTATUS do_lzo_decompress(lzo_stream* stream) {
    uint8_t byte;
    uint32_t len, back;
    bool backcopy = false;

    stream->error = false;

    byte = lzo_nextbyte(stream);
    if (stream->error) return STATUS_INTERNAL_ERROR;

    if (byte > 17) {
        lzo_copy(stream, min((uint8_t)(byte - 17), (uint32_t)(stream->outlen - stream->outpos)));
        if (stream->error) return STATUS_INTERNAL_ERROR;

        if (stream->outlen == stream->outpos)
            return STATUS_SUCCESS;

        byte = lzo_nextbyte(stream);
        if (stream->error) return STATUS_INTERNAL_ERROR;

        if (byte < 16) return STATUS_INTERNAL_ERROR;
    }

    while (1) {
        if (byte >> 4) {
            backcopy = true;
            if (byte >> 6) {
                len = (byte >> 5) - 1;
                back = (lzo_nextbyte(stream) << 3) + ((byte >> 2) & 7) + 1;
                if (stream->error) return STATUS_INTERNAL_ERROR;
            } else if (byte >> 5) {
                len = lzo_len(stream, byte, 31);
                if (stream->error) return STATUS_INTERNAL_ERROR;

                byte = lzo_nextbyte(stream);
                if (stream->error) return STATUS_INTERNAL_ERROR;

                back = (lzo_nextbyte(stream) << 6) + (byte >> 2) + 1;
                if (stream->error) return STATUS_INTERNAL_ERROR;
            } else {
                len = lzo_len(stream, byte, 7);
                if (stream->error) return STATUS_INTERNAL_ERROR;

                back = (1 << 14) + ((byte & 8) << 11);

                byte = lzo_nextbyte(stream);
                if (stream->error) return STATUS_INTERNAL_ERROR;

                back += (lzo_nextbyte(stream) << 6) + (byte >> 2);
                if (stream->error) return STATUS_INTERNAL_ERROR;

                if (back == (1 << 14)) {
                    if (len != 1)
                        return STATUS_INTERNAL_ERROR;
                    break;
                }
            }
        } else if (backcopy) {
            len = 0;
            back = (lzo_nextbyte(stream) << 2) + (byte >> 2) + 1;
            if (stream->error) return STATUS_INTERNAL_ERROR;
        } else {
            len = lzo_len(stream, byte, 15);
            if (stream->error) return STATUS_INTERNAL_ERROR;

            lzo_copy(stream, min(len + 3, stream->outlen - stream->outpos));
            if (stream->error) return STATUS_INTERNAL_ERROR;

            if (stream->outlen == stream->outpos)
                return STATUS_SUCCESS;

            byte = lzo_nextbyte(stream);
            if (stream->error) return STATUS_INTERNAL_ERROR;

            if (byte >> 4)
                continue;

            len = 1;
            back = (1 << 11) + (lzo_nextbyte(stream) << 2) + (byte >> 2) + 1;
            if (stream->error) return STATUS_INTERNAL_ERROR;

            break;
        }

        lzo_copyback(stream, back, min(len + 2, stream->outlen - stream->outpos));
        if (stream->error) return STATUS_INTERNAL_ERROR;

        if (stream->outlen == stream->outpos)
            return STATUS_SUCCESS;

        len = byte & 3;

        if (len) {
            lzo_copy(stream, min(len, stream->outlen - stream->outpos));
            if (stream->error) return STATUS_INTERNAL_ERROR;

            if (stream->outlen == stream->outpos)
                return STATUS_SUCCESS;
        } else
            backcopy = !backcopy;

        byte = lzo_nextbyte(stream);
        if (stream->error) return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS lzo_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, uint32_t inpageoff) {
    NTSTATUS Status;
    uint32_t partlen, inoff, outoff;
    lzo_stream stream;

    inoff = 0;
    outoff = 0;

    do {
        partlen = *(uint32_t*)&inbuf[inoff];

        if (partlen + inoff > inlen) {
            ERR("overflow: %x + %x > %I64x\n", partlen, inoff, inlen);
            return STATUS_INTERNAL_ERROR;
        }

        inoff += sizeof(uint32_t);

        stream.in = &inbuf[inoff];
        stream.inlen = partlen;
        stream.inpos = 0;
        stream.out = &outbuf[outoff];
        stream.outlen = min(outlen, LZO_PAGE_SIZE);
        stream.outpos = 0;

        Status = do_lzo_decompress(&stream);
        if (!NT_SUCCESS(Status)) {
            ERR("do_lzo_decompress returned %08x\n", Status);
            return Status;
        }

        if (stream.outpos < stream.outlen)
            RtlZeroMemory(&stream.out[stream.outpos], stream.outlen - stream.outpos);

        inoff += partlen;
        outoff += stream.outlen;

        if (LZO_PAGE_SIZE - ((inpageoff + inoff) % LZO_PAGE_SIZE) < sizeof(uint32_t))
            inoff = ((((inpageoff + inoff) / LZO_PAGE_SIZE) + 1) * LZO_PAGE_SIZE) - inpageoff;

        outlen -= stream.outlen;
    } while (inoff < inlen && outlen > 0);

    return STATUS_SUCCESS;
}

static void* zlib_alloc(void* opaque, unsigned int items, unsigned int size) {
    UNUSED(opaque);

    return ExAllocatePoolWithTag(PagedPool, items * size, ALLOC_TAG_ZLIB);
}

static void zlib_free(void* opaque, void* ptr) {
    UNUSED(opaque);

    ExFreePool(ptr);
}

NTSTATUS zlib_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen) {
    z_stream c_stream;
    int ret;

    c_stream.zalloc = zlib_alloc;
    c_stream.zfree = zlib_free;
    c_stream.opaque = (voidpf)0;

    ret = inflateInit(&c_stream);

    if (ret != Z_OK) {
        ERR("inflateInit returned %08x\n", ret);
        return STATUS_INTERNAL_ERROR;
    }

    c_stream.next_in = inbuf;
    c_stream.avail_in = inlen;

    c_stream.next_out = outbuf;
    c_stream.avail_out = outlen;

    do {
        ret = inflate(&c_stream, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            ERR("inflate returned %08x\n", ret);
            inflateEnd(&c_stream);
            return STATUS_INTERNAL_ERROR;
        }

        if (c_stream.avail_out == 0)
            break;
    } while (ret != Z_STREAM_END);

    ret = inflateEnd(&c_stream);

    if (ret != Z_OK) {
        ERR("inflateEnd returned %08x\n", ret);
        return STATUS_INTERNAL_ERROR;
    }

    // FIXME - if we're short, should we zero the end of outbuf so we don't leak information into userspace?

    return STATUS_SUCCESS;
}

static NTSTATUS zlib_write_compressed_bit(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, bool* compressed, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    uint8_t compression;
    uint32_t comp_length;
    uint8_t* comp_data;
    uint32_t out_left;
    LIST_ENTRY* le;
    chunk* c;
    z_stream c_stream;
    int ret;

    comp_data = ExAllocatePoolWithTag(PagedPool, (uint32_t)(end_data - start_data), ALLOC_TAG);
    if (!comp_data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08x\n", Status);
        ExFreePool(comp_data);
        return Status;
    }

    c_stream.zalloc = zlib_alloc;
    c_stream.zfree = zlib_free;
    c_stream.opaque = (voidpf)0;

    ret = deflateInit(&c_stream, fcb->Vcb->options.zlib_level);

    if (ret != Z_OK) {
        ERR("deflateInit returned %08x\n", ret);
        ExFreePool(comp_data);
        return STATUS_INTERNAL_ERROR;
    }

    c_stream.avail_in = (uint32_t)(end_data - start_data);
    c_stream.next_in = data;
    c_stream.avail_out = (uint32_t)(end_data - start_data);
    c_stream.next_out = comp_data;

    do {
        ret = deflate(&c_stream, Z_FINISH);

        if (ret == Z_STREAM_ERROR) {
            ERR("deflate returned %x\n", ret);
            ExFreePool(comp_data);
            return STATUS_INTERNAL_ERROR;
        }
    } while (c_stream.avail_in > 0 && c_stream.avail_out > 0);

    out_left = c_stream.avail_out;

    ret = deflateEnd(&c_stream);

    if (ret != Z_OK) {
        ERR("deflateEnd returned %08x\n", ret);
        ExFreePool(comp_data);
        return STATUS_INTERNAL_ERROR;
    }

    if (out_left < fcb->Vcb->superblock.sector_size) { // compressed extent would be larger than or same size as uncompressed extent
        ExFreePool(comp_data);

        comp_length = (uint32_t)(end_data - start_data);
        comp_data = data;
        compression = BTRFS_COMPRESSION_NONE;

        *compressed = false;
    } else {
        uint32_t cl;

        compression = BTRFS_COMPRESSION_ZLIB;
        cl = (uint32_t)(end_data - start_data - out_left);
        comp_length = (uint32_t)sector_align(cl, fcb->Vcb->superblock.sector_size);

        RtlZeroMemory(comp_data + cl, comp_length - cl);

        *compressed = true;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

    le = fcb->Vcb->chunks.Flink;
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, fcb->Vcb);

            if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

                    if (compression != BTRFS_COMPRESSION_NONE)
                        ExFreePool(comp_data);

                    return STATUS_SUCCESS;
                }
            }

            release_chunk_lock(c, fcb->Vcb);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, true);

    Status = alloc_chunk(fcb->Vcb, fcb->Vcb->data_flags, &c, false);

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    if (!NT_SUCCESS(Status)) {
        ERR("alloc_chunk returned %08x\n", Status);

        if (compression != BTRFS_COMPRESSION_NONE)
            ExFreePool(comp_data);

        return Status;
    }

    if (c) {
        acquire_chunk_lock(c, fcb->Vcb);

        if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                if (compression != BTRFS_COMPRESSION_NONE)
                    ExFreePool(comp_data);

                return STATUS_SUCCESS;
            }
        }

        release_chunk_lock(c, fcb->Vcb);
    }

    WARN("couldn't find any data chunks with %I64x bytes free\n", comp_length);

    if (compression != BTRFS_COMPRESSION_NONE)
        ExFreePool(comp_data);

    return STATUS_DISK_FULL;
}

static NTSTATUS lzo_do_compress(const uint8_t* in, uint32_t in_len, uint8_t* out, uint32_t* out_len, void* wrkmem) {
    const uint8_t* ip;
    uint32_t dv;
    uint8_t* op;
    const uint8_t* in_end = in + in_len;
    const uint8_t* ip_end = in + in_len - 9 - 4;
    const uint8_t* ii;
    const uint8_t** dict = (const uint8_t**)wrkmem;

    op = out;
    ip = in;
    ii = ip;

    DVAL_FIRST(dv, ip); UPDATE_D(dict, cycle, dv, ip); ip++;
    DVAL_NEXT(dv, ip);  UPDATE_D(dict, cycle, dv, ip); ip++;
    DVAL_NEXT(dv, ip);  UPDATE_D(dict, cycle, dv, ip); ip++;
    DVAL_NEXT(dv, ip);  UPDATE_D(dict, cycle, dv, ip); ip++;

    while (1) {
        const uint8_t* m_pos;
        uint32_t m_len;
        ptrdiff_t m_off;
        uint32_t lit, dindex;

        dindex = DINDEX(dv, ip);
        m_pos = dict[dindex];
        UPDATE_I(dict, cycle, dindex, ip);

        if (!LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, M4_MAX_OFFSET) && m_pos[0] == ip[0] && m_pos[1] == ip[1] && m_pos[2] == ip[2]) {
            lit = (uint32_t)(ip - ii);
            m_pos += 3;
            if (m_off <= M2_MAX_OFFSET)
                goto match;

            if (lit == 3) { /* better compression, but slower */
                if (op - 2 <= out)
                    return STATUS_INTERNAL_ERROR;

                op[-2] |= LZO_BYTE(3);
                *op++ = *ii++; *op++ = *ii++; *op++ = *ii++;
                goto code_match;
            }

            if (*m_pos == ip[3])
                goto match;
        }

        /* a literal */
        ++ip;
        if (ip >= ip_end)
            break;
        DVAL_NEXT(dv, ip);
        continue;

        /* a match */
match:
        /* store current literal run */
        if (lit > 0) {
            uint32_t t = lit;

            if (t <= 3) {
                if (op - 2 <= out)
                    return STATUS_INTERNAL_ERROR;

                op[-2] |= LZO_BYTE(t);
            } else if (t <= 18)
                *op++ = LZO_BYTE(t - 3);
            else {
                uint32_t tt = t - 18;

                *op++ = 0;
                while (tt > 255) {
                    tt -= 255;
                    *op++ = 0;
                }

                if (tt <= 0)
                    return STATUS_INTERNAL_ERROR;

                *op++ = LZO_BYTE(tt);
            }

            do {
                *op++ = *ii++;
            } while (--t > 0);
        }


        /* code the match */
code_match:
        if (ii != ip)
            return STATUS_INTERNAL_ERROR;

        ip += 3;
        if (*m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++ ||
            *m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++) {
            --ip;
            m_len = (uint32_t)(ip - ii);

            if (m_len < 3 || m_len > 8)
                return STATUS_INTERNAL_ERROR;

            if (m_off <= M2_MAX_OFFSET) {
                m_off -= 1;
                *op++ = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
                *op++ = LZO_BYTE(m_off >> 3);
            } else if (m_off <= M3_MAX_OFFSET) {
                m_off -= 1;
                *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
                goto m3_m4_offset;
            } else {
                m_off -= 0x4000;

                if (m_off <= 0 || m_off > 0x7fff)
                    return STATUS_INTERNAL_ERROR;

                *op++ = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11) | (m_len - 2));
                goto m3_m4_offset;
            }
        } else {
            const uint8_t* end;
            end = in_end;
            while (ip < end && *m_pos == *ip)
                m_pos++, ip++;
            m_len = (uint32_t)(ip - ii);

            if (m_len < 3)
                return STATUS_INTERNAL_ERROR;

            if (m_off <= M3_MAX_OFFSET) {
                m_off -= 1;
                if (m_len <= 33)
                    *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
                else {
                    m_len -= 33;
                    *op++ = M3_MARKER | 0;
                    goto m3_m4_len;
                }
            } else {
                m_off -= 0x4000;

                if (m_off <= 0 || m_off > 0x7fff)
                    return STATUS_INTERNAL_ERROR;

                if (m_len <= 9)
                    *op++ = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11) | (m_len - 2));
                else {
                    m_len -= 9;
                    *op++ = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11));
m3_m4_len:
                    while (m_len > 255) {
                        m_len -= 255;
                        *op++ = 0;
                    }

                    if (m_len <= 0)
                        return STATUS_INTERNAL_ERROR;

                    *op++ = LZO_BYTE(m_len);
                }
            }

m3_m4_offset:
            *op++ = LZO_BYTE((m_off & 63) << 2);
            *op++ = LZO_BYTE(m_off >> 6);
        }

        ii = ip;
        if (ip >= ip_end)
            break;
        DVAL_FIRST(dv, ip);
    }

    /* store final literal run */
    if (in_end - ii > 0) {
        uint32_t t = (uint32_t)(in_end - ii);

        if (op == out && t <= 238)
            *op++ = LZO_BYTE(17 + t);
        else if (t <= 3)
            op[-2] |= LZO_BYTE(t);
        else if (t <= 18)
            *op++ = LZO_BYTE(t - 3);
        else {
            uint32_t tt = t - 18;

            *op++ = 0;
            while (tt > 255) {
                tt -= 255;
                *op++ = 0;
            }

            if (tt <= 0)
                return STATUS_INTERNAL_ERROR;

            *op++ = LZO_BYTE(tt);
        }

        do {
            *op++ = *ii++;
        } while (--t > 0);
    }

    *out_len = (uint32_t)(op - out);

    return STATUS_SUCCESS;
}

static NTSTATUS lzo1x_1_compress(lzo_stream* stream) {
    uint8_t *op = stream->out;
    NTSTATUS Status = STATUS_SUCCESS;

    if (stream->inlen <= 0)
        stream->outlen = 0;
    else if (stream->inlen <= 9 + 4) {
        *op++ = LZO_BYTE(17 + stream->inlen);

        stream->inpos = 0;
        do {
            *op++ = stream->in[stream->inpos];
            stream->inpos++;
        } while (stream->inlen < stream->inpos);
        stream->outlen = (uint32_t)(op - stream->out);
    } else
        Status = lzo_do_compress(stream->in, stream->inlen, stream->out, &stream->outlen, stream->wrkmem);

    if (Status == STATUS_SUCCESS) {
        op = stream->out + stream->outlen;
        *op++ = M4_MARKER | 1;
        *op++ = 0;
        *op++ = 0;
        stream->outlen += 3;
    }

    return Status;
}

static __inline uint32_t lzo_max_outlen(uint32_t inlen) {
    return inlen + (inlen / 16) + 64 + 3; // formula comes from LZO.FAQ
}

static NTSTATUS lzo_write_compressed_bit(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, bool* compressed, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    uint8_t compression;
    uint64_t comp_length;
    ULONG comp_data_len, num_pages, i;
    uint8_t* comp_data;
    bool skip_compression = false;
    lzo_stream stream;
    uint32_t* out_size;
    LIST_ENTRY* le;
    chunk* c;

    num_pages = (ULONG)((sector_align(end_data - start_data, LZO_PAGE_SIZE)) / LZO_PAGE_SIZE);

    // Four-byte overall header
    // Another four-byte header page
    // Each page has a maximum size of lzo_max_outlen(LZO_PAGE_SIZE)
    // Plus another four bytes for possible padding
    comp_data_len = sizeof(uint32_t) + ((lzo_max_outlen(LZO_PAGE_SIZE) + (2 * sizeof(uint32_t))) * num_pages);

    comp_data = ExAllocatePoolWithTag(PagedPool, comp_data_len, ALLOC_TAG);
    if (!comp_data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    stream.wrkmem = ExAllocatePoolWithTag(PagedPool, LZO1X_MEM_COMPRESS, ALLOC_TAG);
    if (!stream.wrkmem) {
        ERR("out of memory\n");
        ExFreePool(comp_data);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08x\n", Status);
        ExFreePool(comp_data);
        ExFreePool(stream.wrkmem);
        return Status;
    }

    out_size = (uint32_t*)comp_data;
    *out_size = sizeof(uint32_t);

    stream.in = data;
    stream.out = comp_data + (2 * sizeof(uint32_t));

    for (i = 0; i < num_pages; i++) {
        uint32_t* pagelen = (uint32_t*)(stream.out - sizeof(uint32_t));

        stream.inlen = (uint32_t)min(LZO_PAGE_SIZE, end_data - start_data - (i * LZO_PAGE_SIZE));

        Status = lzo1x_1_compress(&stream);
        if (!NT_SUCCESS(Status)) {
            ERR("lzo1x_1_compress returned %08x\n", Status);
            skip_compression = true;
            break;
        }

        *pagelen = stream.outlen;
        *out_size += stream.outlen + sizeof(uint32_t);

        stream.in += LZO_PAGE_SIZE;
        stream.out += stream.outlen + sizeof(uint32_t);

        if (LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE) < sizeof(uint32_t)) {
            RtlZeroMemory(stream.out, LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE));
            stream.out += LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE);
            *out_size += LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE);
        }
    }

    ExFreePool(stream.wrkmem);

    if (skip_compression || *out_size >= end_data - start_data - fcb->Vcb->superblock.sector_size) { // compressed extent would be larger than or same size as uncompressed extent
        ExFreePool(comp_data);

        comp_length = end_data - start_data;
        comp_data = data;
        compression = BTRFS_COMPRESSION_NONE;

        *compressed = false;
    } else {
        compression = BTRFS_COMPRESSION_LZO;
        comp_length = sector_align(*out_size, fcb->Vcb->superblock.sector_size);

        RtlZeroMemory(comp_data + *out_size, (ULONG)(comp_length - *out_size));

        *compressed = true;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

    le = fcb->Vcb->chunks.Flink;
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, fcb->Vcb);

            if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

                    if (compression != BTRFS_COMPRESSION_NONE)
                        ExFreePool(comp_data);

                    return STATUS_SUCCESS;
                }
            }

            release_chunk_lock(c, fcb->Vcb);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, true);

    Status = alloc_chunk(fcb->Vcb, fcb->Vcb->data_flags, &c, false);

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    if (!NT_SUCCESS(Status)) {
        ERR("alloc_chunk returned %08x\n", Status);

        if (compression != BTRFS_COMPRESSION_NONE)
            ExFreePool(comp_data);

        return Status;
    }

    if (c) {
        acquire_chunk_lock(c, fcb->Vcb);

        if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                if (compression != BTRFS_COMPRESSION_NONE)
                    ExFreePool(comp_data);

                return STATUS_SUCCESS;
            }
        }

        release_chunk_lock(c, fcb->Vcb);
    }

    WARN("couldn't find any data chunks with %I64x bytes free\n", comp_length);

    if (compression != BTRFS_COMPRESSION_NONE)
        ExFreePool(comp_data);

    return STATUS_DISK_FULL;
}

static NTSTATUS zstd_write_compressed_bit(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, bool* compressed, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    uint8_t compression;
    uint32_t comp_length;
    uint8_t* comp_data;
    uint32_t out_left;
    LIST_ENTRY* le;
    chunk* c;
    ZSTD_CStream* stream;
    size_t init_res, written;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    ZSTD_parameters params;

    comp_data = ExAllocatePoolWithTag(PagedPool, (uint32_t)(end_data - start_data), ALLOC_TAG);
    if (!comp_data) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08x\n", Status);
        ExFreePool(comp_data);
        return Status;
    }

    stream = ZSTD_createCStream_advanced(zstd_mem);

    if (!stream) {
        ERR("ZSTD_createCStream failed.\n");
        ExFreePool(comp_data);
        return STATUS_INTERNAL_ERROR;
    }

    params = ZSTD_getParams(fcb->Vcb->options.zstd_level, (uint32_t)(end_data - start_data), 0);

    if (params.cParams.windowLog > ZSTD_BTRFS_MAX_WINDOWLOG)
        params.cParams.windowLog = ZSTD_BTRFS_MAX_WINDOWLOG;

    init_res = ZSTD_initCStream_advanced(stream, NULL, 0, params, (uint32_t)(end_data - start_data));

    if (ZSTD_isError(init_res)) {
        ERR("ZSTD_initCStream_advanced failed: %s\n", ZSTD_getErrorName(init_res));
        ZSTD_freeCStream(stream);
        ExFreePool(comp_data);
        return STATUS_INTERNAL_ERROR;
    }

    input.src = data;
    input.size = (uint32_t)(end_data - start_data);
    input.pos = 0;

    output.dst = comp_data;
    output.size = (uint32_t)(end_data - start_data);
    output.pos = 0;

    while (input.pos < input.size && output.pos < output.size) {
        written = ZSTD_compressStream(stream, &output, &input);

        if (ZSTD_isError(written)) {
            ERR("ZSTD_compressStream failed: %s\n", ZSTD_getErrorName(written));
            ZSTD_freeCStream(stream);
            ExFreePool(comp_data);
            return STATUS_INTERNAL_ERROR;
        }
    }

    written = ZSTD_endStream(stream, &output);
    if (ZSTD_isError(written)) {
        ERR("ZSTD_endStream failed: %s\n", ZSTD_getErrorName(written));
        ZSTD_freeCStream(stream);
        ExFreePool(comp_data);
        return STATUS_INTERNAL_ERROR;
    }

    ZSTD_freeCStream(stream);

    out_left = output.size - output.pos;

    if (out_left < fcb->Vcb->superblock.sector_size) { // compressed extent would be larger than or same size as uncompressed extent
        ExFreePool(comp_data);

        comp_length = (uint32_t)(end_data - start_data);
        comp_data = data;
        compression = BTRFS_COMPRESSION_NONE;

        *compressed = false;
    } else {
        uint32_t cl;

        compression = BTRFS_COMPRESSION_ZSTD;
        cl = (uint32_t)(end_data - start_data - out_left);
        comp_length = (uint32_t)sector_align(cl, fcb->Vcb->superblock.sector_size);

        RtlZeroMemory(comp_data + cl, comp_length - cl);

        *compressed = true;
    }

    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

    le = fcb->Vcb->chunks.Flink;
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c->readonly && !c->reloc) {
            acquire_chunk_lock(c, fcb->Vcb);

            if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

                    if (compression != BTRFS_COMPRESSION_NONE)
                        ExFreePool(comp_data);

                    return STATUS_SUCCESS;
                }
            }

            release_chunk_lock(c, fcb->Vcb);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, true);

    Status = alloc_chunk(fcb->Vcb, fcb->Vcb->data_flags, &c, false);

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    if (!NT_SUCCESS(Status)) {
        ERR("alloc_chunk returned %08x\n", Status);

        if (compression != BTRFS_COMPRESSION_NONE)
            ExFreePool(comp_data);

        return Status;
    }

    if (c) {
        acquire_chunk_lock(c, fcb->Vcb);

        if (c->chunk_item->type == fcb->Vcb->data_flags && (c->chunk_item->size - c->used) >= comp_length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start_data, comp_length, false, comp_data, Irp, rollback, compression, end_data - start_data, false, 0)) {
                if (compression != BTRFS_COMPRESSION_NONE)
                    ExFreePool(comp_data);

                return STATUS_SUCCESS;
            }
        }

        release_chunk_lock(c, fcb->Vcb);
    }

    WARN("couldn't find any data chunks with %I64x bytes free\n", comp_length);

    if (compression != BTRFS_COMPRESSION_NONE)
        ExFreePool(comp_data);

    return STATUS_DISK_FULL;
}

NTSTATUS write_compressed_bit(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, bool* compressed, PIRP Irp, LIST_ENTRY* rollback) {
    uint8_t type;

    if (fcb->Vcb->options.compress_type != 0 && fcb->prop_compression == PropCompression_None)
        type = fcb->Vcb->options.compress_type;
    else {
        if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_COMPRESS_ZSTD) && fcb->prop_compression == PropCompression_ZSTD)
            type = BTRFS_COMPRESSION_ZSTD;
        else if (fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_COMPRESS_ZSTD && fcb->prop_compression != PropCompression_Zlib && fcb->prop_compression != PropCompression_LZO)
            type = BTRFS_COMPRESSION_ZSTD;
        else if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO) && fcb->prop_compression == PropCompression_LZO)
            type = BTRFS_COMPRESSION_LZO;
        else if (fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO && fcb->prop_compression != PropCompression_Zlib)
            type = BTRFS_COMPRESSION_LZO;
        else
            type = BTRFS_COMPRESSION_ZLIB;
    }

    if (type == BTRFS_COMPRESSION_ZSTD) {
        fcb->Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_COMPRESS_ZSTD;
        return zstd_write_compressed_bit(fcb, start_data, end_data, data, compressed, Irp, rollback);
    } else if (type == BTRFS_COMPRESSION_LZO) {
        fcb->Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO;
        return lzo_write_compressed_bit(fcb, start_data, end_data, data, compressed, Irp, rollback);
    } else
        return zlib_write_compressed_bit(fcb, start_data, end_data, data, compressed, Irp, rollback);
}

static void* zstd_malloc(void* opaque, size_t size) {
    UNUSED(opaque);

    return ExAllocatePoolWithTag(PagedPool, size, ZSTD_ALLOC_TAG);
}

static void zstd_free(void* opaque, void* address) {
    UNUSED(opaque);

    ExFreePool(address);
}

NTSTATUS zstd_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen) {
    NTSTATUS Status;
    ZSTD_DStream* stream;
    size_t init_res, read;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;

    stream = ZSTD_createDStream_advanced(zstd_mem);

    if (!stream) {
        ERR("ZSTD_createDStream failed.\n");
        return STATUS_INTERNAL_ERROR;
    }

    init_res = ZSTD_initDStream(stream);

    if (ZSTD_isError(init_res)) {
        ERR("ZSTD_initDStream failed: %s\n", ZSTD_getErrorName(init_res));
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    input.src = inbuf;
    input.size = inlen;
    input.pos = 0;

    output.dst = outbuf;
    output.size = outlen;
    output.pos = 0;

    read = ZSTD_decompressStream(stream, &output, &input);

    if (ZSTD_isError(read)) {
        ERR("ZSTD_decompressStream failed: %s\n", ZSTD_getErrorName(read));
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }

    Status = STATUS_SUCCESS;

end:
    ZSTD_freeDStream(stream);

    return Status;
}
