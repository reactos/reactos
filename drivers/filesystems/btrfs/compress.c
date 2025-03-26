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
#endif // __REACTOS__

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
            ERR("overflow: %x + %x > %x\n", partlen, inoff, inlen);
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
            ERR("do_lzo_decompress returned %08lx\n", Status);
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

NTSTATUS zlib_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, unsigned int level, unsigned int* space_left) {
    z_stream c_stream;
    int ret;

    c_stream.zalloc = zlib_alloc;
    c_stream.zfree = zlib_free;
    c_stream.opaque = (voidpf)0;

    ret = deflateInit(&c_stream, level);

    if (ret != Z_OK) {
        ERR("deflateInit returned %i\n", ret);
        return STATUS_INTERNAL_ERROR;
    }

    c_stream.next_in = inbuf;
    c_stream.avail_in = inlen;

    c_stream.next_out = outbuf;
    c_stream.avail_out = outlen;

    do {
        ret = deflate(&c_stream, Z_FINISH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            ERR("deflate returned %i\n", ret);
            deflateEnd(&c_stream);
            return STATUS_INTERNAL_ERROR;
        }

        if (c_stream.avail_in == 0 || c_stream.avail_out == 0)
            break;
    } while (ret != Z_STREAM_END);

    deflateEnd(&c_stream);

    *space_left = c_stream.avail_in > 0 ? 0 : c_stream.avail_out;

    return STATUS_SUCCESS;
}

NTSTATUS zlib_decompress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen) {
    z_stream c_stream;
    int ret;

    c_stream.zalloc = zlib_alloc;
    c_stream.zfree = zlib_free;
    c_stream.opaque = (voidpf)0;

    ret = inflateInit(&c_stream);

    if (ret != Z_OK) {
        ERR("inflateInit returned %i\n", ret);
        return STATUS_INTERNAL_ERROR;
    }

    c_stream.next_in = inbuf;
    c_stream.avail_in = inlen;

    c_stream.next_out = outbuf;
    c_stream.avail_out = outlen;

    do {
        ret = inflate(&c_stream, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            ERR("inflate returned %i\n", ret);
            inflateEnd(&c_stream);
            return STATUS_INTERNAL_ERROR;
        }

        if (c_stream.avail_out == 0)
            break;
    } while (ret != Z_STREAM_END);

    ret = inflateEnd(&c_stream);

    if (ret != Z_OK) {
        ERR("inflateEnd returned %i\n", ret);
        return STATUS_INTERNAL_ERROR;
    }

    // FIXME - if we're short, should we zero the end of outbuf so we don't leak information into userspace?

    return STATUS_SUCCESS;
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

    do {
        read = ZSTD_decompressStream(stream, &output, &input);

        if (ZSTD_isError(read)) {
            ERR("ZSTD_decompressStream failed: %s\n", ZSTD_getErrorName(read));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }

        if (output.pos == output.size)
            break;
    } while (read != 0);

    Status = STATUS_SUCCESS;

end:
    ZSTD_freeDStream(stream);

    return Status;
}

NTSTATUS lzo_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, unsigned int* space_left) {
    NTSTATUS Status;
    unsigned int num_pages;
    unsigned int comp_data_len;
    uint8_t* comp_data;
    lzo_stream stream;
    uint32_t* out_size;

    num_pages = (unsigned int)sector_align(inlen, LZO_PAGE_SIZE) / LZO_PAGE_SIZE;

    // Four-byte overall header
    // Another four-byte header page
    // Each page has a maximum size of lzo_max_outlen(LZO_PAGE_SIZE)
    // Plus another four bytes for possible padding
    comp_data_len = sizeof(uint32_t) + ((lzo_max_outlen(LZO_PAGE_SIZE) + (2 * sizeof(uint32_t))) * num_pages);

    // FIXME - can we write this so comp_data isn't necessary?

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

    out_size = (uint32_t*)comp_data;
    *out_size = sizeof(uint32_t);

    stream.in = inbuf;
    stream.out = comp_data + (2 * sizeof(uint32_t));

    for (unsigned int i = 0; i < num_pages; i++) {
        uint32_t* pagelen = (uint32_t*)(stream.out - sizeof(uint32_t));

        stream.inlen = (uint32_t)min(LZO_PAGE_SIZE, outlen - (i * LZO_PAGE_SIZE));

        Status = lzo1x_1_compress(&stream);
        if (!NT_SUCCESS(Status)) {
            ERR("lzo1x_1_compress returned %08lx\n", Status);
            ExFreePool(comp_data);
            return Status;
        }

        *pagelen = stream.outlen;
        *out_size += stream.outlen + sizeof(uint32_t);

        stream.in += LZO_PAGE_SIZE;
        stream.out += stream.outlen + sizeof(uint32_t);

        // new page needs to start at a 32-bit boundary
        if (LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE) < sizeof(uint32_t)) {
            RtlZeroMemory(stream.out, LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE));
            stream.out += LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE);
            *out_size += LZO_PAGE_SIZE - (*out_size % LZO_PAGE_SIZE);
        }
    }

    ExFreePool(stream.wrkmem);

    if (*out_size >= outlen)
        *space_left = 0;
    else {
        *space_left = outlen - *out_size;

        RtlCopyMemory(outbuf, comp_data, *out_size);
    }

    ExFreePool(comp_data);

    return STATUS_SUCCESS;
}

NTSTATUS zstd_compress(uint8_t* inbuf, uint32_t inlen, uint8_t* outbuf, uint32_t outlen, uint32_t level, unsigned int* space_left) {
    ZSTD_CStream* stream;
    size_t init_res, written;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    ZSTD_parameters params;

    stream = ZSTD_createCStream_advanced(zstd_mem);

    if (!stream) {
        ERR("ZSTD_createCStream failed.\n");
        return STATUS_INTERNAL_ERROR;
    }

    params = ZSTD_getParams(level, inlen, 0);

    if (params.cParams.windowLog > ZSTD_BTRFS_MAX_WINDOWLOG)
        params.cParams.windowLog = ZSTD_BTRFS_MAX_WINDOWLOG;

    init_res = ZSTD_initCStream_advanced(stream, NULL, 0, params, inlen);

    if (ZSTD_isError(init_res)) {
        ERR("ZSTD_initCStream_advanced failed: %s\n", ZSTD_getErrorName(init_res));
        ZSTD_freeCStream(stream);
        return STATUS_INTERNAL_ERROR;
    }

    input.src = inbuf;
    input.size = inlen;
    input.pos = 0;

    output.dst = outbuf;
    output.size = outlen;
    output.pos = 0;

    while (input.pos < input.size && output.pos < output.size) {
        written = ZSTD_compressStream(stream, &output, &input);

        if (ZSTD_isError(written)) {
            ERR("ZSTD_compressStream failed: %s\n", ZSTD_getErrorName(written));
            ZSTD_freeCStream(stream);
            return STATUS_INTERNAL_ERROR;
        }
    }

    written = ZSTD_endStream(stream, &output);
    if (ZSTD_isError(written)) {
        ERR("ZSTD_endStream failed: %s\n", ZSTD_getErrorName(written));
        ZSTD_freeCStream(stream);
        return STATUS_INTERNAL_ERROR;
    }

    ZSTD_freeCStream(stream);

    if (input.pos < input.size) // output would be larger than input
        *space_left = 0;
    else
        *space_left = output.size - output.pos;

    return STATUS_SUCCESS;
}

typedef struct {
    uint8_t buf[COMPRESSED_EXTENT_SIZE];
    uint8_t compression_type;
    unsigned int inlen;
    unsigned int outlen;
    calc_job* cj;
} comp_part;

NTSTATUS write_compressed(fcb* fcb, uint64_t start_data, uint64_t end_data, void* data, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    uint64_t i;
    unsigned int num_parts = (unsigned int)sector_align(end_data - start_data, COMPRESSED_EXTENT_SIZE) / COMPRESSED_EXTENT_SIZE;
    uint8_t type;
    comp_part* parts;
    unsigned int buflen = 0;
    uint8_t* buf;
    chunk* c = NULL;
    LIST_ENTRY* le;
    uint64_t address, extaddr;
    void* csum = NULL;

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

    Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents returned %08lx\n", Status);
        return Status;
    }

    parts = ExAllocatePoolWithTag(PagedPool, sizeof(comp_part) * num_parts, ALLOC_TAG);
    if (!parts) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < num_parts; i++) {
        if (i == num_parts - 1)
            parts[i].inlen = ((unsigned int)(end_data - start_data) - ((num_parts - 1) * COMPRESSED_EXTENT_SIZE));
        else
            parts[i].inlen = COMPRESSED_EXTENT_SIZE;

        Status = add_calc_job_comp(fcb->Vcb, type, (uint8_t*)data + (i * COMPRESSED_EXTENT_SIZE), parts[i].inlen,
                                   parts[i].buf, parts[i].inlen, &parts[i].cj);
        if (!NT_SUCCESS(Status)) {
            ERR("add_calc_job_comp returned %08lx\n", Status);

            for (unsigned int j = 0; j < i; j++) {
                KeWaitForSingleObject(&parts[j].cj->event, Executive, KernelMode, false, NULL);
                ExFreePool(parts[j].cj);
            }

            ExFreePool(parts);
            return Status;
        }
    }

    Status = STATUS_SUCCESS;

    for (int i = num_parts - 1; i >= 0; i--) {
        calc_thread_main(fcb->Vcb, parts[i].cj);

        KeWaitForSingleObject(&parts[i].cj->event, Executive, KernelMode, false, NULL);

        if (!NT_SUCCESS(parts[i].cj->Status))
            Status = parts[i].cj->Status;
    }

    if (!NT_SUCCESS(Status)) {
        ERR("calc job returned %08lx\n", Status);

        for (unsigned int i = 0; i < num_parts; i++) {
            ExFreePool(parts[i].cj);
        }

        ExFreePool(parts);
        return Status;
    }

    for (unsigned int i = 0; i < num_parts; i++) {
        if (parts[i].cj->space_left >= fcb->Vcb->superblock.sector_size) {
            parts[i].compression_type = type;
            parts[i].outlen = parts[i].inlen - parts[i].cj->space_left;

            if (type == BTRFS_COMPRESSION_LZO)
                fcb->Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_COMPRESS_LZO;
            else if (type == BTRFS_COMPRESSION_ZSTD)
                fcb->Vcb->superblock.incompat_flags |= BTRFS_INCOMPAT_FLAGS_COMPRESS_ZSTD;

            if ((parts[i].outlen & (fcb->Vcb->superblock.sector_size - 1)) != 0) {
                unsigned int newlen = (unsigned int)sector_align(parts[i].outlen, fcb->Vcb->superblock.sector_size);

                RtlZeroMemory(parts[i].buf + parts[i].outlen, newlen - parts[i].outlen);

                parts[i].outlen = newlen;
            }
        } else {
            parts[i].compression_type = BTRFS_COMPRESSION_NONE;
            parts[i].outlen = (unsigned int)sector_align(parts[i].inlen, fcb->Vcb->superblock.sector_size);
        }

        buflen += parts[i].outlen;
        ExFreePool(parts[i].cj);
    }

    // check if first 128 KB of file is incompressible

    if (start_data == 0 && parts[0].compression_type == BTRFS_COMPRESSION_NONE && !fcb->Vcb->options.compress_force) {
        TRACE("adding nocompress flag to subvol %I64x, inode %I64x\n", fcb->subvol->id, fcb->inode);

        fcb->inode_item.flags |= BTRFS_INODE_NOCOMPRESS;
        fcb->inode_item_changed = true;
        mark_fcb_dirty(fcb);
    }

    // join together into continuous buffer

    buf = ExAllocatePoolWithTag(PagedPool, buflen, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        ExFreePool(parts);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    {
        uint8_t* buf2 = buf;

        for (i = 0; i < num_parts; i++) {
            if (parts[i].compression_type == BTRFS_COMPRESSION_NONE)
                RtlCopyMemory(buf2, (uint8_t*)data + (i * COMPRESSED_EXTENT_SIZE), parts[i].outlen);
            else
                RtlCopyMemory(buf2, parts[i].buf, parts[i].outlen);

            buf2 += parts[i].outlen;
        }
    }

    // find an address

    ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, true);

    le = fcb->Vcb->chunks.Flink;
    while (le != &fcb->Vcb->chunks) {
        chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);

        if (!c2->readonly && !c2->reloc) {
            acquire_chunk_lock(c2, fcb->Vcb);

            if (c2->chunk_item->type == fcb->Vcb->data_flags && (c2->chunk_item->size - c2->used) >= buflen) {
                if (find_data_address_in_chunk(fcb->Vcb, c2, buflen, &address)) {
                    c = c2;
                    c->used += buflen;
                    space_list_subtract(c, address, buflen, rollback);
                    release_chunk_lock(c2, fcb->Vcb);
                    break;
                }
            }

            release_chunk_lock(c2, fcb->Vcb);
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

    if (!c) {
        chunk* c2;

        ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, true);

        Status = alloc_chunk(fcb->Vcb, fcb->Vcb->data_flags, &c2, false);

        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);

        if (!NT_SUCCESS(Status)) {
            ERR("alloc_chunk returned %08lx\n", Status);
            ExFreePool(buf);
            ExFreePool(parts);
            return Status;
        }

        acquire_chunk_lock(c2, fcb->Vcb);

        if (find_data_address_in_chunk(fcb->Vcb, c2, buflen, &address)) {
            c = c2;
            c->used += buflen;
            space_list_subtract(c, address, buflen, rollback);
        }

        release_chunk_lock(c2, fcb->Vcb);
    }

    if (!c) {
        WARN("couldn't find any data chunks with %x bytes free\n", buflen);
        ExFreePool(buf);
        ExFreePool(parts);
        return STATUS_DISK_FULL;
    }

    // write to disk

    TRACE("writing %x bytes to %I64x\n", buflen, address);

    Status = write_data_complete(fcb->Vcb, address, buf, buflen, Irp, NULL, false, 0,
                                 fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE ? HighPagePriority : NormalPagePriority);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data_complete returned %08lx\n", Status);
        ExFreePool(buf);
        ExFreePool(parts);
        return Status;
    }

    // FIXME - do rest of the function while we're waiting for I/O to finish?

    // calculate csums if necessary

    if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
        unsigned int sl = buflen >> fcb->Vcb->sector_shift;

        csum = ExAllocatePoolWithTag(PagedPool, sl * fcb->Vcb->csum_size, ALLOC_TAG);
        if (!csum) {
            ERR("out of memory\n");
            ExFreePool(buf);
            ExFreePool(parts);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        do_calc_job(fcb->Vcb, buf, sl, csum);
    }

    ExFreePool(buf);

    // add extents to fcb

    extaddr = address;

    for (i = 0; i < num_parts; i++) {
        EXTENT_DATA* ed;
        EXTENT_DATA2* ed2;
        void* csum2;

        ed = ExAllocatePoolWithTag(PagedPool, offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2), ALLOC_TAG);
        if (!ed) {
            ERR("out of memory\n");
            ExFreePool(parts);

            if (csum)
                ExFreePool(csum);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ed->generation = fcb->Vcb->superblock.generation;
        ed->decoded_size = parts[i].inlen;
        ed->compression = parts[i].compression_type;
        ed->encryption = BTRFS_ENCRYPTION_NONE;
        ed->encoding = BTRFS_ENCODING_NONE;
        ed->type = EXTENT_TYPE_REGULAR;

        ed2 = (EXTENT_DATA2*)ed->data;
        ed2->address = extaddr;
        ed2->size = parts[i].outlen;
        ed2->offset = 0;
        ed2->num_bytes = parts[i].inlen;

        if (csum) {
            csum2 = ExAllocatePoolWithTag(PagedPool, (parts[i].outlen * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift, ALLOC_TAG);
            if (!csum2) {
                ERR("out of memory\n");
                ExFreePool(ed);
                ExFreePool(parts);
                ExFreePool(csum);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(csum2, (uint8_t*)csum + (((extaddr - address) * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift),
                          (parts[i].outlen * fcb->Vcb->csum_size) >> fcb->Vcb->sector_shift);
        } else
            csum2 = NULL;

        Status = add_extent_to_fcb(fcb, start_data + (i * COMPRESSED_EXTENT_SIZE), ed, offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2),
                                   true, csum2, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_extent_to_fcb returned %08lx\n", Status);
            ExFreePool(ed);
            ExFreePool(parts);

            if (csum)
                ExFreePool(csum);

            return Status;
        }

        ExFreePool(ed);

        fcb->inode_item.st_blocks += parts[i].inlen;

        extaddr += parts[i].outlen;
    }

    if (csum)
        ExFreePool(csum);

    // update extent refcounts

    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, true);

    extaddr = address;

    for (i = 0; i < num_parts; i++) {
        add_changed_extent_ref(c, extaddr, parts[i].outlen, fcb->subvol->id, fcb->inode,
                               start_data + (i * COMPRESSED_EXTENT_SIZE), 1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);

        extaddr += parts[i].outlen;
    }

    ExReleaseResourceLite(&c->changed_extents_lock);

    fcb->extents_changed = true;
    fcb->inode_item_changed = true;
    mark_fcb_dirty(fcb);

    ExFreePool(parts);

    return STATUS_SUCCESS;
}
