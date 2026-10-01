/*
 * Radius Cinepak Video Decoder
 *
 * Copyright 2001 Dr. Tim Ferguson (see below)
 * Portions Copyright 2003 Mike McCormack for CodeWeavers
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

/* Copyright notice from original source:
 * ------------------------------------------------------------------------
 * Radius Cinepak Video Decoder
 *
 * Dr. Tim Ferguson, 2001.
 * For more details on the algorithm:
 *         http://www.csse.monash.edu.au/~timf/videocodec.html
 *
 * This is basically a vector quantiser with adaptive vector density.  The
 * frame is segmented into 4x4 pixel blocks, and each block is coded using
 * either 1 or 4 vectors.
 *
 * There are still some issues with this code yet to be resolved.  In
 * particular with decoding in the strip boundaries.  However, I have not
 * yet found a sequence it doesn't work on.  Ill keep trying :)
 *
 * You may freely use this source code.  I only ask that you reference its
 * source in your projects documentation:
 *       Tim Ferguson: http://www.csse.monash.edu.au/~timf/
 * ------------------------------------------------------------------------ */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "vfw.h"
#include "mmsystem.h"
#include "iccvid_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(iccvid);

static HINSTANCE ICCVID_hModule;

#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)
#define MAX_STRIPS 32

/* ------------------------------------------------------------------------ */
typedef struct
{
    unsigned char y0, y1, y2, y3;
    char u, v;
    unsigned char r[4], g[4], b[4];
} cvid_codebook;

typedef struct {
    cvid_codebook *v4_codebook[MAX_STRIPS];
    cvid_codebook *v1_codebook[MAX_STRIPS];
    unsigned int strip_num;
} cinepak_info;

typedef struct _ICCVID_Info
{
    DWORD         dwMagic;
    int           bits_per_pixel;
    cinepak_info *cvinfo;
} ICCVID_Info;


/* ------------------------------------------------------------------------ */
static unsigned char *in_buffer, uiclip[1024], *uiclp = NULL;

#define get_byte() *(in_buffer++)
#define skip_byte() in_buffer++
#define get_word() ((unsigned short)(in_buffer += 2, \
    (in_buffer[-2] << 8 | in_buffer[-1])))
#define get_long() ((unsigned long)(in_buffer += 4, \
    (in_buffer[-4] << 24 | in_buffer[-3] << 16 | in_buffer[-2] << 8 | in_buffer[-1])))


/* ---------------------------------------------------------------------- */
static inline void read_codebook(cvid_codebook *c, int mode)
{
int uvr, uvg, uvb;

    if(mode)        /* black and white */
        {
        c->y0 = get_byte();
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = c->v = 0;

        c->r[0] = c->g[0] = c->b[0] = c->y0;
        c->r[1] = c->g[1] = c->b[1] = c->y1;
        c->r[2] = c->g[2] = c->b[2] = c->y2;
        c->r[3] = c->g[3] = c->b[3] = c->y3;
        }
    else            /* colour */
        {
        c->y0 = get_byte();  /* luma */
        c->y1 = get_byte();
        c->y2 = get_byte();
        c->y3 = get_byte();
        c->u = get_byte(); /* chroma */
        c->v = get_byte();

        uvr = c->v << 1;
        uvg = -((c->u+1) >> 1) - c->v;
        uvb = c->u << 1;

        c->r[0] = uiclp[c->y0 + uvr]; c->g[0] = uiclp[c->y0 + uvg]; c->b[0] = uiclp[c->y0 + uvb];
        c->r[1] = uiclp[c->y1 + uvr]; c->g[1] = uiclp[c->y1 + uvg]; c->b[1] = uiclp[c->y1 + uvb];
        c->r[2] = uiclp[c->y2 + uvr]; c->g[2] = uiclp[c->y2 + uvg]; c->b[2] = uiclp[c->y2 + uvb];
        c->r[3] = uiclp[c->y3 + uvr]; c->g[3] = uiclp[c->y3 + uvg]; c->b[3] = uiclp[c->y3 + uvb];
        }
}

static inline long get_addr(BOOL inverted, unsigned long x, unsigned long y,
                       int frm_stride, int bpp, unsigned int out_height)
{
    /* Returns the starting position of a line from top-down or bottom-up */
    if (inverted)
        return y * frm_stride + x * bpp;
    else
        return (out_height - 1 - y) * frm_stride + x * bpp;
}

#define MAKECOLOUR32(r,g,b) (((r) << 16) | ((g) << 8) | (b))
/*#define MAKECOLOUR24(r,g,b) (((r) << 16) | ((g) << 8) | (b))*/
#define MAKECOLOUR16(r,g,b) (((r) >> 3) << 11)| (((g) >> 2) << 5)| (((b) >> 3) << 0)
#define MAKECOLOUR15(r,g,b) (((r) >> 3) << 10)| (((g) >> 3) << 5)| (((b) >> 3) << 0)

/* ------------------------------------------------------------------------ */
static void cvid_v1_32(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb)
{
unsigned long *vptr = (unsigned long *)frm;
int row_inc;
int x, y;

    if (!inverted)
        row_inc = -stride/4;
    else
        row_inc = stride/4;

    /* fill 4x4 block of pixels with colour values from codebook */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned long *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR32(cb->r[x/2+(y/2)*2], cb->g[x/2+(y/2)*2], cb->b[x/2+(y/2)*2]);
    }
}

static inline int get_stride(int width, int depth)
{
    return ((depth * width + 31) >> 3) & ~3;
}

/* ------------------------------------------------------------------------ */
static void cvid_v4_32(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb0, cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
unsigned long *vptr = (unsigned long *)frm;
int row_inc;
int x, y;
cvid_codebook * cb[] = {cb0,cb1,cb2,cb3};

    if (!inverted)
        row_inc = -stride/4;
    else
        row_inc = stride/4;

    /* fill 4x4 block of pixels with colour values from codebooks */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned long *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR32(cb[x/2+(y/2)*2]->r[x%2+(y%2)*2], cb[x/2+(y/2)*2]->g[x%2+(y%2)*2], cb[x/2+(y/2)*2]->b[x%2+(y%2)*2]);
    }
}


/* ------------------------------------------------------------------------ */
static void cvid_v1_24(unsigned char *vptr, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb)
{
int row_inc;
int x, y;

    if (!inverted)
        row_inc = -stride;
    else
        row_inc = stride;

    /* fill 4x4 block of pixels with colour values from codebook */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < limit) return;
        for (x = 0; x < 4; x++)
        {
            vptr[y*row_inc + x*3 + 0] = cb->b[x/2+(y/2)*2];
            vptr[y*row_inc + x*3 + 1] = cb->g[x/2+(y/2)*2];
            vptr[y*row_inc + x*3 + 2] = cb->r[x/2+(y/2)*2];
        }
    }
}


/* ------------------------------------------------------------------------ */
static void cvid_v4_24(unsigned char *vptr, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb0, cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
int row_inc;
cvid_codebook * cb[] = {cb0,cb1,cb2,cb3};
int x, y;

    if (!inverted)
        row_inc = -stride;
    else
        row_inc = stride;

    /* fill 4x4 block of pixels with colour values from codebooks */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < limit) return;
        for (x = 0; x < 4; x++)
        {
            vptr[y*row_inc + x*3 + 0] = cb[x/2+(y/2)*2]->b[x%2+(y%2)*2];
            vptr[y*row_inc + x*3 + 1] = cb[x/2+(y/2)*2]->g[x%2+(y%2)*2];
            vptr[y*row_inc + x*3 + 2] = cb[x/2+(y/2)*2]->r[x%2+(y%2)*2];
        }
    }
}


/* ------------------------------------------------------------------------ */
static void cvid_v1_16(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb)
{
unsigned short *vptr = (unsigned short *)frm;
int row_inc;
int x, y;

    if (!inverted)
        row_inc = -stride/2;
    else
        row_inc = stride/2;

    /* fill 4x4 block of pixels with colour values from codebook */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned short *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR16(cb->r[x/2+(y/2)*2], cb->g[x/2+(y/2)*2], cb->b[x/2+(y/2)*2]);
    }
}


/* ------------------------------------------------------------------------ */
static void cvid_v4_16(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb0, cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
unsigned short *vptr = (unsigned short *)frm;
int row_inc;
cvid_codebook * cb[] = {cb0,cb1,cb2,cb3};
int x, y;

    if (!inverted)
        row_inc = -stride/2;
    else
        row_inc = stride/2;

    /* fill 4x4 block of pixels with colour values from codebooks */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned short *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR16(cb[x/2+(y/2)*2]->r[x%2+(y%2)*2], cb[x/2+(y/2)*2]->g[x%2+(y%2)*2], cb[x/2+(y/2)*2]->b[x%2+(y%2)*2]);
    }
}

/* ------------------------------------------------------------------------ */
static void cvid_v1_15(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb)
{
unsigned short *vptr = (unsigned short *)frm;
int row_inc;
int x, y;

    if (!inverted)
        row_inc = -stride/2;
    else
        row_inc = stride/2;

    /* fill 4x4 block of pixels with colour values from codebook */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned short *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR15(cb->r[x/2+(y/2)*2], cb->g[x/2+(y/2)*2], cb->b[x/2+(y/2)*2]);
    }
}


/* ------------------------------------------------------------------------ */
static void cvid_v4_15(unsigned char *frm, unsigned char *limit, int stride, BOOL inverted,
    cvid_codebook *cb0, cvid_codebook *cb1, cvid_codebook *cb2, cvid_codebook *cb3)
{
unsigned short *vptr = (unsigned short *)frm;
int row_inc;
cvid_codebook * cb[] = {cb0,cb1,cb2,cb3};
int x, y;

    if (!inverted)
        row_inc = -stride/2;
    else
        row_inc = stride/2;

    /* fill 4x4 block of pixels with colour values from codebooks */
    for (y = 0; y < 4; y++)
    {
        if (&vptr[y*row_inc] < (unsigned short *)limit) return;
        for (x = 0; x < 4; x++)
            vptr[y*row_inc + x] = MAKECOLOUR15(cb[x/2+(y/2)*2]->r[x%2+(y%2)*2], cb[x/2+(y/2)*2]->g[x%2+(y%2)*2], cb[x/2+(y/2)*2]->b[x%2+(y%2)*2]);
    }
}


/* ------------------------------------------------------------------------
 * Call this function once at the start of the sequence and save the
 * returned context for calls to decode_cinepak().
 */
static cinepak_info *decode_cinepak_init(void)
{
    cinepak_info *cvinfo;
    int i;

    cvinfo = malloc(sizeof(cinepak_info));
    if( !cvinfo )
        return NULL;
    cvinfo->strip_num = 0;

    if(uiclp == NULL)
    {
        uiclp = uiclip+512;
        for(i = -512; i < 512; i++)
            uiclp[i] = (i < 0 ? 0 : (i > 255 ? 255 : i));
    }

    return cvinfo;
}

static void free_cvinfo( cinepak_info *cvinfo )
{
    unsigned int i;

    for( i=0; i<cvinfo->strip_num; i++ )
    {
        free(cvinfo->v4_codebook[i]);
        free(cvinfo->v1_codebook[i]);
    }
    free( cvinfo );
}

typedef void (*fn_cvid_v1)(unsigned char *frm, unsigned char *limit,
                           int stride, BOOL inverted, cvid_codebook *cb);
typedef void (*fn_cvid_v4)(unsigned char *frm, unsigned char *limit,
                           int stride, BOOL inverted,
                           cvid_codebook *cb0, cvid_codebook *cb1,
                           cvid_codebook *cb2, cvid_codebook *cb3);

/* ------------------------------------------------------------------------
 * This function decodes a buffer containing a Cinepak encoded frame.
 *
 * context - the context created by decode_cinepak_init().
 * buf - the input buffer to be decoded
 * size - the size of the input buffer
 * output - the output frame buffer (24 or 32 bit per pixel)
 * out_width - the width of the output frame
 * out_height - the height of the output frame
 * bit_per_pixel - the number of bits per pixel allocated to the output
 *   frame (only 24 or 32 bpp are supported)
 * inverted - if true the output frame is written top-down
 */
static void decode_cinepak(cinepak_info *cvinfo, unsigned char *buf, int size,
           unsigned char *output, unsigned int out_width, unsigned int out_height, int bit_per_pixel, BOOL inverted)
{
    cvid_codebook *v4_codebook, *v1_codebook, *codebook = NULL;
    unsigned long x, y, y_bottom, cnum, strip_id, chunk_id,
                  x0, y0, x1, y1, ci, flag, mask;
    long top_size, chunk_size;
    unsigned char *frm_ptr;
    unsigned int i, cur_strip, addr;
    int d0, d1, d2, d3, frm_stride, bpp = 3;
    fn_cvid_v1 cvid_v1 = cvid_v1_24;
    fn_cvid_v4 cvid_v4 = cvid_v4_24;
    struct frame_header
    {
      unsigned char flags;
      unsigned long length;
      unsigned short width;
      unsigned short height;
      unsigned short strips;
    } frame;

    y = 0;
    y_bottom = 0;
    in_buffer = buf;

    frame.flags = get_byte();
    frame.length = get_byte() << 16;
    frame.length |= get_byte() << 8;
    frame.length |= get_byte();

    switch(bit_per_pixel)
        {
        case 15:
            bpp = 2;
            cvid_v1 = cvid_v1_15;
            cvid_v4 = cvid_v4_15;
            break;
        case 16:
            bpp = 2;
            cvid_v1 = cvid_v1_16;
            cvid_v4 = cvid_v4_16;
            break;
        case 24:
            bpp = 3;
            cvid_v1 = cvid_v1_24;
            cvid_v4 = cvid_v4_24;
            break;
        case 32:
            bpp = 4;
            cvid_v1 = cvid_v1_32;
            cvid_v4 = cvid_v4_32;
            break;
        }

    frm_stride = get_stride(out_width, bpp * 8);
    frm_ptr = output;

    if(frame.length != size)
        {
        if(frame.length & 0x01) frame.length++; /* AVIs tend to have a size mismatch */
        if(frame.length != size)
            {
            ERR("CVID: corruption %d (QT/AVI) != %ld (CV)\n", size, frame.length);
            /* return; */
            }
        }

    frame.width = get_word();
    frame.height = get_word();
    frame.strips = get_word();

    if(frame.strips > cvinfo->strip_num)
        {
        if(frame.strips >= MAX_STRIPS)
            {
            ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
            return;
            }

        for(i = cvinfo->strip_num; i < frame.strips; i++)
            {
            if((cvinfo->v4_codebook[i] = malloc(sizeof(cvid_codebook) * 260)) == NULL)
                {
                ERR("CVID: codebook v4 alloc err\n");
                return;
                }

            if((cvinfo->v1_codebook[i] = malloc(sizeof(cvid_codebook) * 260)) == NULL)
                {
                ERR("CVID: codebook v1 alloc err\n");
                return;
                }
            }
        }
    cvinfo->strip_num = frame.strips;

    TRACE("CVID: %ux%u, strips %u, length %lu\n",
          frame.width, frame.height, frame.strips, frame.length);

    for(cur_strip = 0; cur_strip < frame.strips; cur_strip++)
        {
        v4_codebook = cvinfo->v4_codebook[cur_strip];
        v1_codebook = cvinfo->v1_codebook[cur_strip];

        if((cur_strip > 0) && (!(frame.flags & 0x01)))
            {
            memcpy(cvinfo->v4_codebook[cur_strip], cvinfo->v4_codebook[cur_strip-1], 260 * sizeof(cvid_codebook));
            memcpy(cvinfo->v1_codebook[cur_strip], cvinfo->v1_codebook[cur_strip-1], 260 * sizeof(cvid_codebook));
            }

        strip_id = get_word();        /* 1000 = key strip, 1100 = iter strip */
        top_size = get_word();
        y0 = get_word();        /* FIXME: most of these are ignored at the moment */
        x0 = get_word();
        y1 = get_word();
        x1 = get_word();

        y_bottom += y1;
        top_size -= 12;
        x = 0;
        if(x1 != out_width)
            WARN("CVID: Warning x1 (%ld) != width (%d)\n", x1, out_width);

        TRACE("   %d) %04lx %04ld <%ld,%ld> <%ld,%ld> yt %ld\n",
              cur_strip, strip_id, top_size, x0, y0, x1, y1, y_bottom);

        while(top_size > 0)
            {
            chunk_id  = get_word();
            chunk_size = get_word();

            TRACE("        %04lx %04ld\n", chunk_id, chunk_size);
            top_size -= chunk_size;
            chunk_size -= 4;

            switch(chunk_id)
                {
                    /* -------------------- Codebook Entries -------------------- */
                case 0x2000:
                case 0x2200:
                    codebook = (chunk_id == 0x2200 ? v1_codebook : v4_codebook);
                    cnum = chunk_size/6;
                    for(i = 0; i < cnum; i++) read_codebook(codebook+i, 0);
                    break;

                case 0x2400:
                case 0x2600:        /* 8 bit per pixel */
                    codebook = (chunk_id == 0x2600 ? v1_codebook : v4_codebook);
                    cnum = chunk_size/4;
                    for(i = 0; i < cnum; i++) read_codebook(codebook+i, 1);
                    break;

                case 0x2100:
                case 0x2300:
                    codebook = (chunk_id == 0x2300 ? v1_codebook : v4_codebook);

                    ci = 0;
                    while(chunk_size > 0)
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(flag & 0x80000000)
                                {
                                chunk_size -= 6;
                                read_codebook(codebook+ci, 0);
                                }

                            ci++;
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x2500:
                case 0x2700:        /* 8 bit per pixel */
                    codebook = (chunk_id == 0x2700 ? v1_codebook : v4_codebook);

                    ci = 0;
                    while(chunk_size > 0)
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(flag & 0x80000000)
                                {
                                chunk_size -= 4;
                                read_codebook(codebook+ci, 1);
                                }

                            ci++;
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                    /* -------------------- Frame -------------------- */
                case 0x3000:
                    while((chunk_size > 0) && (y < y_bottom))
                        {
                        flag = get_long();
                        chunk_size -= 4;

                        for(i = 0; i < 32; i++)
                            {
                            if(y >= y_bottom) break;
                            if(flag & 0x80000000)    /* 4 bytes per block */
                                {
                                d0 = get_byte();
                                d1 = get_byte();
                                d2 = get_byte();
                                d3 = get_byte();
                                chunk_size -= 4;

                                addr = get_addr(inverted, x, y, frm_stride, bpp, out_height);
                                cvid_v4(frm_ptr + addr, output, frm_stride, inverted, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
                                }
                            else        /* 1 byte per block */
                                {
                                addr = get_addr(inverted, x, y, frm_stride, bpp, out_height);
                                cvid_v1(frm_ptr + addr, output, frm_stride, inverted, v1_codebook + get_byte());

                                chunk_size--;
                                }

                            x += 4;
                            if(x >= out_width)
                                {
                                x = 0;
                                y += 4;
                                }
                            flag <<= 1;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x3100:
                    while((chunk_size > 0) && (y < y_bottom))
                        {
                            /* ---- flag bits: 0 = SKIP, 10 = V1, 11 = V4 ---- */
                        flag = get_long();
                        chunk_size -= 4;
                        mask = 0x80000000;

                        while((mask) && (y < y_bottom))
                            {
                            if(flag & mask)
                                {
                                if(mask == 1)
                                    {
                                    if(chunk_size < 0) break;
                                    flag = get_long();
                                    chunk_size -= 4;
                                    mask = 0x80000000;
                                    }
                                else mask >>= 1;

                                if(flag & mask)        /* V4 */
                                    {
                                    d0 = get_byte();
                                    d1 = get_byte();
                                    d2 = get_byte();
                                    d3 = get_byte();
                                    chunk_size -= 4;

                                    addr = get_addr(inverted, x, y, frm_stride, bpp, out_height);
                                    cvid_v4(frm_ptr + addr, output, frm_stride, inverted, v4_codebook+d0, v4_codebook+d1, v4_codebook+d2, v4_codebook+d3);
                                    }
                                else        /* V1 */
                                    {
                                    chunk_size--;

                                    addr = get_addr(inverted, x, y, frm_stride, bpp, out_height);
                                    cvid_v1(frm_ptr + addr, output, frm_stride, inverted, v1_codebook + get_byte());
                                    }
                                }        /* else SKIP */

                            mask >>= 1;
                            x += 4;
                            if(x >= out_width)
                                {
                                x = 0;
                                y += 4;
                                }
                            }
                        }

                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                case 0x3200:        /* each byte is a V1 codebook */
                    while((chunk_size > 0) && (y < y_bottom))
                        {
                        addr = get_addr(inverted, x, y, frm_stride, bpp, out_height);
                        cvid_v1(frm_ptr + addr, output, frm_stride, inverted, v1_codebook + get_byte());

                        chunk_size--;
                        x += 4;
                        if(x >= out_width)
                            {
                            x = 0;
                            y += 4;
                            }
                        }
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;

                default:
                    ERR("CVID: unknown chunk_id %08lx\n", chunk_id);
                    while(chunk_size > 0) { skip_byte(); chunk_size--; }
                    break;
                }
            }
        }

    if(frame.length != size)
        {
        if(frame.length & 0x01) frame.length++; /* AVIs tend to have a size mismatch */
        if(frame.length != size)
            {
            long xlen;
            skip_byte();
            xlen = get_byte() << 16;
            xlen |= get_byte() << 8;
            xlen |= get_byte(); /* Read Len */
            WARN("CVID: END INFO chunk size %d cvid size1 %ld cvid size2 %ld\n",
                  size, frame.length, xlen);
            }
        }
}

static void ICCVID_dump_BITMAPINFO(const BITMAPINFO * bmi)
{
    TRACE(
        "planes = %d\n"
        "bpp    = %d\n"
        "height = %ld\n"
        "width  = %ld\n"
        "compr  = %s\n",
        bmi->bmiHeader.biPlanes,
        bmi->bmiHeader.biBitCount,
        bmi->bmiHeader.biHeight,
        bmi->bmiHeader.biWidth,
        debugstr_fourcc(bmi->bmiHeader.biCompression));
}

static inline int ICCVID_CheckMask(RGBQUAD bmiColors[3], COLORREF redMask, COLORREF blueMask, COLORREF greenMask)
{
    COLORREF realRedMask = MAKECOLOUR32(bmiColors[0].rgbRed, bmiColors[0].rgbGreen, bmiColors[0].rgbBlue);
    COLORREF realBlueMask = MAKECOLOUR32(bmiColors[1].rgbRed, bmiColors[1].rgbGreen, bmiColors[1].rgbBlue);
    COLORREF realGreenMask = MAKECOLOUR32(bmiColors[2].rgbRed, bmiColors[2].rgbGreen, bmiColors[2].rgbBlue);

    TRACE("\nbmiColors[0] = 0x%08lx\nbmiColors[1] = 0x%08lx\nbmiColors[2] = 0x%08lx\n",
        realRedMask, realBlueMask, realGreenMask);
        
    if ((realRedMask == redMask) &&
        (realBlueMask == blueMask) &&
        (realGreenMask == greenMask))
        return TRUE;
    return FALSE;
}

static LRESULT ICCVID_DecompressQuery( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_QUERY %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    TRACE("in: ");
    ICCVID_dump_BITMAPINFO(in);

    if( in->bmiHeader.biCompression != ICCVID_MAGIC )
        return ICERR_BADFORMAT;

    if( out )
    {
        TRACE("out: ");
        ICCVID_dump_BITMAPINFO(out);

        if( in->bmiHeader.biPlanes != out->bmiHeader.biPlanes )
            return ICERR_BADFORMAT;
        if( in->bmiHeader.biHeight != out->bmiHeader.biHeight )
        {
            if( in->bmiHeader.biHeight != -out->bmiHeader.biHeight )
                return ICERR_BADFORMAT;
            TRACE("Detected inverted height for video output\n");
        }
        if( in->bmiHeader.biWidth != out->bmiHeader.biWidth )
            return ICERR_BADFORMAT;

        switch( out->bmiHeader.biCompression )
        {
        case BI_RGB:
            if ( out->bmiHeader.biBitCount == 16 || out->bmiHeader.biBitCount == 24 || out->bmiHeader.biBitCount == 32 )
                return ICERR_OK;
            break;
        case BI_BITFIELDS:
            if ( out->bmiHeader.biBitCount == 16 && ICCVID_CheckMask(out->bmiColors, 0x7C00, 0x03E0, 0x001F) )
                return ICERR_OK;
            if ( out->bmiHeader.biBitCount == 16 && ICCVID_CheckMask(out->bmiColors, 0xF800, 0x07E0, 0x001F) )
                return ICERR_OK;
            break;
        }
        TRACE("unsupported output format\n");
        return ICERR_BADFORMAT;
    }

    return ICERR_OK;
}

static LRESULT ICCVID_DecompressGetFormat( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    DWORD size;

    TRACE("ICM_DECOMPRESS_GETFORMAT %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    size = in->bmiHeader.biSize;
    if (in->bmiHeader.biBitCount <= 8)
        size += in->bmiHeader.biClrUsed * sizeof(RGBQUAD);

    if( out )
    {
        memcpy( out, in, size );
        out->bmiHeader.biBitCount = 24;
        out->bmiHeader.biCompression = BI_RGB;
        out->bmiHeader.biSizeImage = get_stride(in->bmiHeader.biWidth, 24) * in->bmiHeader.biHeight;
        return ICERR_OK;
    }
    return size;
}

static LRESULT ICCVID_DecompressBegin( ICCVID_Info *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_BEGIN %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;

    info->bits_per_pixel = out->bmiHeader.biBitCount;

    if (info->bits_per_pixel == 16)
    {
        if ( out->bmiHeader.biCompression == BI_BITFIELDS )
        {
            if ( ICCVID_CheckMask(out->bmiColors, 0x7C00, 0x03E0, 0x001F) )
                info->bits_per_pixel = 15;
            else if ( ICCVID_CheckMask(out->bmiColors, 0xF800, 0x07E0, 0x001F) )
                info->bits_per_pixel = 16;
            else
            {
                TRACE("unsupported output bit field(s) for 16-bit colors\n");
                return ICERR_UNSUPPORTED;
            }
        }
        else
            info->bits_per_pixel = 15;
    }

    TRACE("bit_per_pixel = %d\n", info->bits_per_pixel);

    if( info->cvinfo )
        free_cvinfo( info->cvinfo );
    info->cvinfo = decode_cinepak_init();

    return ICERR_OK;
}

static LRESULT ICCVID_Decompress( ICCVID_Info *info, ICDECOMPRESS *icd, DWORD size )
{
    LONG width, height;
    BOOL inverted;

    TRACE("ICM_DECOMPRESS %p %p %ld\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;
    if (info->cvinfo==NULL)
    {
        ERR("ICM_DECOMPRESS sent after ICM_DECOMPRESS_END\n");
        return ICERR_BADPARAM;
    }

    width  = icd->lpbiInput->biWidth;
    height = icd->lpbiInput->biHeight;
    inverted = -icd->lpbiOutput->biHeight == height;

    decode_cinepak(info->cvinfo, icd->lpInput, icd->lpbiInput->biSizeImage,
                   icd->lpOutput, width, height, info->bits_per_pixel, inverted);

    return ICERR_OK;
}

static LRESULT ICCVID_DecompressEx( ICCVID_Info *info, ICDECOMPRESSEX *icd, DWORD size )
{
    LONG width, height;
    BOOL inverted;

    TRACE("ICM_DECOMPRESSEX %p %p %ld\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return ICERR_BADPARAM;
    if (info->cvinfo==NULL)
    {
        ERR("ICM_DECOMPRESSEX sent after ICM_DECOMPRESS_END\n");
        return ICERR_BADPARAM;
    }

    /* FIXME: flags are ignored */

    width  = icd->lpbiSrc->biWidth;
    height = icd->lpbiSrc->biHeight;
    inverted = -icd->lpbiDst->biHeight == height;

    decode_cinepak(info->cvinfo, icd->lpSrc, icd->lpbiSrc->biSizeImage,
                   icd->lpDst, width, height, info->bits_per_pixel, inverted);

    return ICERR_OK;
}

static LRESULT ICCVID_Close( ICCVID_Info *info )
{
    if( (info==NULL) || (info->dwMagic!=ICCVID_MAGIC) )
        return 0;
    if( info->cvinfo )
        free_cvinfo( info->cvinfo );
    free( info );
    return 1;
}

static LRESULT ICCVID_GetInfo( ICCVID_Info *info, ICINFO *icinfo, DWORD dwSize )
{
    if (!icinfo) return sizeof(ICINFO);
    if (dwSize < sizeof(ICINFO)) return 0;

    icinfo->dwSize = sizeof(ICINFO);
    icinfo->fccType = ICTYPE_VIDEO;
    icinfo->fccHandler = info ? info->dwMagic : ICCVID_MAGIC;
    icinfo->dwFlags = 0;
    icinfo->dwVersion = ICVERSION;
    icinfo->dwVersionICM = ICVERSION;

    LoadStringW(ICCVID_hModule, IDS_NAME, icinfo->szName, ARRAY_SIZE(icinfo->szName));
    LoadStringW(ICCVID_hModule, IDS_DESCRIPTION, icinfo->szDescription, ARRAY_SIZE(icinfo->szDescription));
    /* msvfw32 will fill icinfo->szDriver for us */

    return sizeof(ICINFO);
}

static LRESULT ICCVID_DecompressEnd( ICCVID_Info *info )
{
    if( info->cvinfo )
    {
        free_cvinfo( info->cvinfo );
        info->cvinfo = NULL;
    }
    return ICERR_OK;
}

LRESULT WINAPI ICCVID_DriverProc( DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
                                  LPARAM lParam1, LPARAM lParam2)
{
    ICCVID_Info *info = (ICCVID_Info *) dwDriverId;

    TRACE("%Id %p %d %Id %Id\n", dwDriverId, hdrvr, msg, lParam1, lParam2);

    switch( msg )
    {
    case DRV_LOAD:
        TRACE("Loaded\n");
        return 1;
    case DRV_ENABLE:
        return 0;
    case DRV_DISABLE:
        return 0;
    case DRV_FREE:
        return 0;

    case DRV_OPEN:
    {
        ICINFO *icinfo = (ICINFO *)lParam2;

        TRACE("Opened\n");

        if (icinfo && compare_fourcc(icinfo->fccType, ICTYPE_VIDEO)) return 0;

        info = malloc(sizeof(ICCVID_Info));
        if( info )
        {
            info->dwMagic = ICCVID_MAGIC;
            info->cvinfo = NULL;
        }
        return (LRESULT) info;
    }

    case DRV_CLOSE:
        return ICCVID_Close( info );

    case ICM_GETINFO:
        return ICCVID_GetInfo( info, (ICINFO *)lParam1, (DWORD)lParam2 );

    case ICM_DECOMPRESS_QUERY:
        return ICCVID_DecompressQuery( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS_GET_FORMAT:
        return ICCVID_DecompressGetFormat( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS_BEGIN:
        return ICCVID_DecompressBegin( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
    case ICM_DECOMPRESS:
        return ICCVID_Decompress( info, (ICDECOMPRESS*) lParam1,
                                  (DWORD) lParam2 );
    case ICM_DECOMPRESSEX:
        return ICCVID_DecompressEx( info, (ICDECOMPRESSEX*) lParam1, 
                                  (DWORD) lParam2 );

    case ICM_DECOMPRESS_END:
        return ICCVID_DecompressEnd( info );

    case ICM_COMPRESS_QUERY:
        FIXME("compression not implemented\n");
        return ICERR_BADFORMAT;

    case ICM_CONFIGURE:
        return ICERR_UNSUPPORTED;

    default:
        FIXME("Unknown message: %04x %Id %Id\n", msg, lParam1, lParam2);
    }
    return ICERR_UNSUPPORTED;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    TRACE("(%p,%ld,%p)\n", hModule, dwReason, lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        ICCVID_hModule = hModule;
        break;
    }
    return TRUE;
}
