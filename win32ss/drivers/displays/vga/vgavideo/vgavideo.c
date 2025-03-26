/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/vgavideo/vgavideo.c
 * PURPOSE:
 * PROGRAMMERS:
 */

#include <vgaddi.h>

UCHAR PreCalcReverseByte[256];
int maskbit[640];
int y80[480];
int xconv[640];
int bit8[640];
int startmasks[8];
int endmasks[8];
PBYTE vidmem;
static ULONG UnpackPixel[256];

static unsigned char leftMask;
static int byteCounter;
static unsigned char rightMask;

UCHAR bytesPerPixel(ULONG Format)
{
    /* This function is taken from /subsys/win32k/eng/surface.c
     * FIXME: GDI bitmaps are supposed to be pixel-packed. Right now if the
     * pixel size if < 1 byte we expand it to 1 byte for simplicities sake */

    switch (Format)
    {
        case BMF_1BPP:
            return 1;

        case BMF_4BPP:
        case BMF_4RLE:
            return 1;

        case BMF_8BPP:
        case BMF_8RLE:
            return 1;

        case BMF_16BPP:
            return 2;

        case BMF_24BPP:
            return 3;

        case BMF_32BPP:
            return 4;

        default:
            return 0;
    }
}

VOID vgaPreCalc()
{
    ULONG j;

    startmasks[0] = 255;
    startmasks[1] = 1;
    startmasks[2] = 3;
    startmasks[3] = 7;
    startmasks[4] = 15;
    startmasks[5] = 31;
    startmasks[6] = 63;
    startmasks[7] = 127;

    endmasks[0] = 0;
    endmasks[1] = 128;
    endmasks[2] = 192;
    endmasks[3] = 224;
    endmasks[4] = 240;
    endmasks[5] = 248;
    endmasks[6] = 252;
    endmasks[7] = 254;

    for (j = 0; j < 80; j++)
    {
        maskbit[j*8]   = 128;
        maskbit[j*8+1] = 64;
        maskbit[j*8+2] = 32;
        maskbit[j*8+3] = 16;
        maskbit[j*8+4] = 8;
        maskbit[j*8+5] = 4;
        maskbit[j*8+6] = 2;
        maskbit[j*8+7] = 1;

        bit8[j*8]   = 7;
        bit8[j*8+1] = 6;
        bit8[j*8+2] = 5;
        bit8[j*8+3] = 4;
        bit8[j*8+4] = 3;
        bit8[j*8+5] = 2;
        bit8[j*8+6] = 1;
        bit8[j*8+7] = 0;
    }
    for (j = 0; j < SCREEN_Y; j++)
        y80[j]  = j*80;
    for (j = 0; j < SCREEN_X; j++)
        xconv[j] = j >> 3;

    for (j = 0; j < 256; j++)
    {
        PreCalcReverseByte[j] =
            (((j >> 0) & 0x1) << 7) |
            (((j >> 1) & 0x1) << 6) |
            (((j >> 2) & 0x1) << 5) |
            (((j >> 3) & 0x1) << 4) |
            (((j >> 4) & 0x1) << 3) |
            (((j >> 5) & 0x1) << 2) |
            (((j >> 6) & 0x1) << 1) |
            (((j >> 7) & 0x1) << 0);
    }

    for (j = 0; j < 256; j++)
    {
        UnpackPixel[j] =
            (((j >> 0) & 0x1) << 4) |
            (((j >> 1) & 0x1) << 0) |
            (((j >> 2) & 0x1) << 12) |
            (((j >> 3) & 0x1) << 8) |
            (((j >> 4) & 0x1) << 20) |
            (((j >> 5) & 0x1) << 16) |
            (((j >> 6) & 0x1) << 28) |
            (((j >> 7) & 0x1) << 24);
    }
}

void
get_masks(int x, int w)
{
    register int tmp;

    leftMask = rightMask = 0;
    byteCounter = w;
    /* right margin */
    tmp = (x+w) & 7;
    if (tmp)
    {
        byteCounter -= tmp;
        rightMask = (unsigned char)(0xff00 >> tmp);
    }
    /* left margin */
    tmp = x & 7;
    if (tmp)
    {
        byteCounter -= (8 - tmp);
        leftMask = (0xff >> tmp);
    }
    /* too small ? */
    if (byteCounter < 0)
    {
        leftMask &= rightMask;
        rightMask = 0;
        byteCounter = 0;
    }
    byteCounter /= 8;
}

VOID vgaPutPixel(INT x, INT y, UCHAR c)
{
    ULONG offset;

    offset = xconv[x]+y80[y];

    WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D,maskbit[x]);

    READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

VOID vgaPutByte(INT x, INT y, UCHAR c)
{
    ULONG offset;

    offset = xconv[x]+y80[y];

    /* Set the write mode */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D,0xff);

    WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

VOID vgaGetByte(
    IN ULONG offset,
    OUT UCHAR *b,
    OUT UCHAR *g,
    OUT UCHAR *r,
    OUT UCHAR *i)
{
    WRITE_PORT_USHORT((PUSHORT)GRA_I, 0x0304);
    *i = READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
    *r = READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x01);
    *g = READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
    *b = READ_REGISTER_UCHAR(vidmem + offset);
}

INT vgaGetPixel(
    IN INT x,
    IN INT y)
{
    UCHAR mask, b, g, r, i;
    ULONG offset;

    offset = xconv[x] + y80[y];
    vgaGetByte(offset, &b, &g, &r, &i);

    mask = maskbit[x];
    b = b & mask;
    g = g & mask;
    r = r & mask;
    i = i & mask;

    mask = bit8[x];
    g = g >> mask;
    b = b >> mask;
    r = r >> mask;
    i = i >> mask;

    return (b + 2 * g + 4 * r + 8 * i);
}

BOOL vgaHLine(INT x, INT y, INT len, UCHAR c)
{
    ULONG orgx, pre1, midpre1;
    //ULONG orgpre1;
    LONG ileftpix, imidpix, irightpix;

    orgx = x;

    /*if ( len < 8 )
    {
        for (i = x; i < x+len; i++ )
            vgaPutPixel ( i, y, c );

        return TRUE;
    }*/

    /* Calculate the left mask pixels, middle bytes and right mask pixel */
    ileftpix = 7 - mod8(x-1);
    irightpix = mod8(x+len);
    imidpix = (len-ileftpix-irightpix) / 8;

    pre1 = xconv[(x-1)&~7] + y80[y];
    //orgpre1=pre1;

    /* check for overlap ( very short line ) */
    if ( (ileftpix+irightpix) > len )
    {
        int mask = startmasks[ileftpix] & endmasks[irightpix];
        /* Write left pixels */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D,mask);

        READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, c);

        return TRUE;
    }

    /* Left */
    if ( ileftpix > 0 )
    {
        /* Write left pixels */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D,startmasks[ileftpix]);

        READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, c);

        /* Prepare new x for the middle */
        x = orgx + 8;
    }

    if ( imidpix > 0 )
    {
        midpre1 = xconv[x] + y80[y];

        /* Set mask to all pixels in byte */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xff);
        memset(vidmem+midpre1, c, imidpix); // write middle pixels, no need to read in latch because of the width
    }

    if ( irightpix > 0 )
    {
        x = orgx + len - irightpix;
        pre1 = xconv[x] + y80[y];

        /* Write right pixels */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask bits
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, endmasks[irightpix]);
        READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, c);
    }

    return TRUE;
}

BOOL vgaVLine(INT x, INT y, INT len, UCHAR c)
{
    INT offset, i;

    offset = xconv[x]+y80[y];

#ifdef VGA_PERF
    vgaSetBitMaskRegister ( maskbit[x] );
#else
    WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);       // set the mask
    WRITE_PORT_UCHAR((PUCHAR)GRA_D,maskbit[x]);
#endif

    for(i=y; i<y+len; i++)
    {
        READ_REGISTER_UCHAR(vidmem + offset);
        WRITE_REGISTER_UCHAR(vidmem + offset, c);
        offset += 80;
    }

    return TRUE;
}

static const RECTL rclEmpty = { 0, 0, 0, 0 };

BOOL VGADDIIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)
{
    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    if (prcDst->left < prcDst->right)
    {
        prcDst->top = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

       if (prcDst->top < prcDst->bottom)
           return TRUE;
    }

    *prcDst = rclEmpty;

    return FALSE;
}

void DIB_BltFromVGA(int x, int y, int w, int h, void *b, int Dest_lDelta)
{
    ULONG plane;
    ULONG left = x >> 3;
    ULONG shift = x - (x & ~0x7);
    UCHAR pixel, nextpixel;
    LONG rightcount;
    INT i, j;
    LONG stride = w >> 3;

    /* Calculate the number of rightmost bytes not in a dword block. */
    if (w >= 8)
    {
        rightcount = w % 8;
    }
    else
    {
        stride = 0;
        rightcount = w;
    }
    rightcount = (rightcount + 1) / 2;

    /* Reset the destination. */
    for (j = 0; j < h; j++)
        memset((PVOID)((ULONG_PTR)b + (j * Dest_lDelta)), 0, abs(Dest_lDelta));

    for (plane = 0; plane < 4; plane++)
    {
        PUCHAR dest = b;

        /* Select the plane we are reading in this iteration. */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x04);
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, plane);

        for (j = 0; j < h; j++)
        {
            PULONG destline = (PULONG)dest;
            PUCHAR src = vidmem + (y + j) * SCREEN_STRIDE + left;
            /* Read the data for one plane for an eight aligned pixel block. */
            nextpixel = PreCalcReverseByte[READ_REGISTER_UCHAR(src)];
            for (i = 0; i < stride; i++, src++, destline++)
            {
                /* Form the data for one plane for an aligned block in the destination. */
                pixel = nextpixel;
                pixel >>= shift;

                nextpixel = PreCalcReverseByte[READ_REGISTER_UCHAR(src + 1)];
                pixel |= (nextpixel << (8 - shift));

                /* Expand the plane data to 'chunky' format and store. */
                *destline |= (UnpackPixel[pixel] << plane);
            }
            /* Handle any pixels not falling into a full block. */
            if (rightcount != 0)
            {
                ULONG row;

                /* Form the data for a complete block. */
                pixel = nextpixel;
                pixel >>= shift;

                nextpixel = PreCalcReverseByte[READ_REGISTER_UCHAR(src + 1)];
                pixel |= (nextpixel << (8 - shift));

                row = UnpackPixel[pixel] << plane;

                /* Store the data for each byte in the destination. */
                for (i = 0; i < rightcount; i++)
                {
                    ((PUCHAR)destline)[i] |= (row & 0xFF);
                    row >>= 8;
                }
            }
            dest += Dest_lDelta;
        }
    }

#ifdef VGA_VERIFY
    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i += 2)
        {
            UCHAR c1, c2;
            ULONG mask = (i < (w - 1)) ? 0xFF : 0xF0;

            c1 = (vgaGetPixel(x + i, y + j) << 4) | (vgaGetPixel(x + i + 1, y + j));
            c2 = ((PUCHAR)b)[(j * Dest_lDelta) + (i >> 1)];
            if ((c1 & mask) != (c2 & mask))
                EngDebugBreak();
        }
    }
#endif /* VGA_VERIFY */
}

/* DIB blt to the VGA. */
void DIB_BltToVGA(int x, int y, int w, int h, void *b, int Source_lDelta, int StartMod)
{
    PUCHAR pb, opb = b;
    LONG i, j;
    LONG x2 = x + w;
    LONG y2 = y + h;
    ULONG offset;

    for (i = x; i < x2; i++)
    {
        pb = opb;
        offset = xconv[i] + y80[y];

        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);       // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, maskbit[i]);

        if (StartMod == ((i - x) % 2))
        {
            for (j = y; j < y2; j++)
            {
                READ_REGISTER_UCHAR(vidmem + offset);
                WRITE_REGISTER_UCHAR(vidmem + offset, (*pb & 0xf0) >> 4);
                offset += 80;
                pb += Source_lDelta;
            }
        }
        else
        {
            for (j = y; j < y2; j++)
            {
                READ_REGISTER_UCHAR(vidmem + offset);
                WRITE_REGISTER_UCHAR(vidmem + offset, *pb & 0x0f);
                offset += 80;
                pb += Source_lDelta;
            }
        }

        if (StartMod != ((i - x) % 2))
            opb++;
    }
}


/* DIB blt to the VGA. */
void DIB_BltToVGAWithXlate(int x, int y, int w, int h, void *b, int Source_lDelta, XLATEOBJ* Xlate)
{
    PUCHAR pb, opb = b;
    ULONG i, j;
    ULONG x2 = x + w;
    ULONG y2 = y + h;
    ULONG offset;

    for (i = x; i < x2; i++)
    {
        pb = opb;
        offset = xconv[i] + y80[y];

        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);       // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, maskbit[i]);

        if (0 == ((i - x) % 2))
        {
            for (j = y; j < y2; j++)
            {
                READ_REGISTER_UCHAR(vidmem + offset);
                WRITE_REGISTER_UCHAR(vidmem + offset, XLATEOBJ_iXlate(Xlate, (*pb & 0xf0) >> 4));
                offset += 80;
                pb += Source_lDelta;
            }
        }
        else
        {
            for (j = y; j < y2; j++)
            {
                READ_REGISTER_UCHAR(vidmem + offset);
                WRITE_REGISTER_UCHAR(vidmem + offset, XLATEOBJ_iXlate(Xlate, *pb & 0x0f));
                offset += 80;
                pb += Source_lDelta;
            }
        }

        if (0 != ((i - x) % 2))
            opb++;
    }
}

/* DIB blt to the VGA.
 * For now we just do slow writes -- pixel by pixel,
 * packing each one into the correct 4BPP format. */
void DIB_TransparentBltToVGA(int x, int y, int w, int h, void *b, int Source_lDelta, ULONG trans)

{
    PUCHAR pb = b, opb = b;
    BOOLEAN edgePixel = FALSE;
    ULONG i, j;
    ULONG x2 = x + w;
    ULONG y2 = y + h;
    UCHAR b1, b2;

    /* Check if the width is odd */
    if(mod2(w) > 0)
    {
        edgePixel = TRUE;
        x2 -= 1;
    }

    for (j=y; j<y2; j++)
    {
        for (i=x; i<x2; i+=2)
        {
            b1 = (*pb & 0xf0) >> 4;
            b2 = *pb & 0x0f;
            if(b1 != trans) vgaPutPixel(i,   j, b1);
            if(b2 != trans) vgaPutPixel(i+1, j, b2);
            pb++;
        }

        if (edgePixel)
        {
            b1 = *pb;
            if(b1 != trans) vgaPutPixel(x2, j, b1);
            pb++;
        }

        opb += Source_lDelta;
        pb = opb; // new test code
    }
}

// This algorithm goes from left to right, storing each 4BPP pixel
// in an entire byte.
void FASTCALL
vgaReadScan( int x, int y, int w, void *b )
{
    unsigned char *vp, *vpP;
    unsigned char data, mask, maskP;
    unsigned char *bp;
    unsigned char plane_mask;
    int plane, i;

    ASSIGNVP4(x, y, vpP)
    ASSIGNMK4(x, y, maskP)
    get_masks(x, w);
    WRITE_PORT_USHORT((PUSHORT)GRA_I, 0x0005);  // read mode 0
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x04);  // read map select

    memset ( b, 0, w );

    for ( plane=0, plane_mask=1; plane < 4; plane++, plane_mask<<=1 )
    {
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, plane);  // read map select

        vp = vpP;
        bp = b;
        if ( leftMask )
        {
            mask = maskP;
            data = *vp++;
            do
            {
                if (data & mask)
                    *bp |= plane_mask;
                bp++;
                mask >>= 1;
            } while (mask & leftMask);
        }
        if (byteCounter)
        {
            for (i=byteCounter; i>0; i--)
            {
                data = *vp++;
                if (data & 0x80) *bp |= plane_mask;
                bp++;

                if (data & 0x40) *bp |= plane_mask;
                bp++;
                if (data & 0x20) *bp |= plane_mask;
                bp++;
                if (data & 0x10) *bp |= plane_mask;
                bp++;
                if (data & 0x08) *bp |= plane_mask;
                bp++;
                if (data & 0x04) *bp |= plane_mask;
                bp++;
                if (data & 0x02) *bp |= plane_mask;
                bp++;
                if (data & 0x01) *bp |= plane_mask;
                bp++;
            }
        }
        if (rightMask)
        {
            mask = 0x80;
            data = *vp;
            do
            {
                if (data & mask)
                *bp |= plane_mask;
                bp++;
                mask >>= 1;
            } while (mask & rightMask);
        }
    }
}

/* This algorithm goes from left to right
 * It stores each 4BPP pixel in an entire byte. */
void FASTCALL
vgaWriteScan ( int x, int y, int w, void *b )
{
    unsigned char *bp;
    unsigned char *vp;
    //unsigned char init_mask;
    volatile unsigned char dummy;
    //int byte_per_line;
    int i, j, off, init_off = x&7;

    bp = b;
    ASSIGNVP4(x, y, vp)
    //ASSIGNMK4(x, y, init_mask)
    //byte_per_line = SCREEN_X >> 3;

    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);      // bit mask

    for ( j = 0; j < 8; j++)
    {
        unsigned int mask = 0x80 >> j;
        WRITE_PORT_UCHAR ( (PUCHAR)GRA_D, (unsigned char)mask );
        i = j - init_off;
        off = 0;
        if (j < init_off)
            i += 8, off++;
        while (i < w)
        {
            /*
             * In write mode 2, the incoming data is 4-bit and represents the
             * value of entire bytes on each of the 4 memory planes. First, VGA
             * performs a logical operation on these bytes and the value of the
             * latch register, but in this case there is none. Then, only the
             * bits that are set in the bit mask are used from the resulting
             * bytes, and the other bits are taken from the latch register.
             *
             * The latch register always contains the value previously read from
             * VGA memory, and therefore, we must first read from vp[off] to
             * load the latch register, and then write bp[i] to vp[off], which
             * will be converted to 4 bytes of VGA memory as described.
             */
            dummy = vp[off];
            dummy = bp[i];
            vp[off] = dummy;
            i += 8;
            off++;
        }
    }
}

/* This algorithm goes from left to right, and inside that loop, top to bottom.
 * It also stores each 4BPP pixel in an entire byte. */
void DFB_BltFromVGA(int x, int y, int w, int h, void *b, int bw)
{
    unsigned char *vp, *vpY, *vpP;
    unsigned char data, mask, maskP;
    unsigned char *bp, *bpY;
    unsigned char plane_mask;
    int byte_per_line = SCREEN_X >> 3;
    int plane, i, j;

    ASSIGNVP4(x, y, vpP)
    ASSIGNMK4(x, y, maskP)
    get_masks(x, w);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);  // read mode 0
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x04);  // read map select

    /* clear buffer */
    bp = b;
    for (j = h; j > 0; j--)
    {
        memset(bp, 0, w);
        bp += bw;
    }

    for (plane = 0, plane_mask = 1; plane < 4; plane++, plane_mask <<= 1)
    {
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, plane);  // read map select
        vpY = vpP;
        bpY = b;
        for (j = h; j > 0; j--)
        {
            vp = vpY;
            bp = bpY;
            if (leftMask)
            {
                mask = maskP;
                data = *vp++;
                do
                {
                    if (data & mask)
                        *bp |= plane_mask;
                    bp++;
                    mask >>= 1;
                } while (mask & leftMask);
            }
            if (byteCounter)
            {
                for (i=byteCounter; i>0; i--)
                {
                    data = *vp++;
                    if (data & 0x80) *bp |= plane_mask;
                    bp++;
                    if (data & 0x40) *bp |= plane_mask;
                    bp++;
                    if (data & 0x20) *bp |= plane_mask;
                    bp++;
                    if (data & 0x10) *bp |= plane_mask;
                    bp++;
                    if (data & 0x08) *bp |= plane_mask;
                    bp++;
                    if (data & 0x04) *bp |= plane_mask;
                    bp++;
                    if (data & 0x02) *bp |= plane_mask;
                    bp++;
                    if (data & 0x01) *bp |= plane_mask;
                    bp++;
                }
            }
            if (rightMask)
            {
                mask = 0x80;
                data = *vp;
                do
                {
                    if (data & mask) *bp |= plane_mask;
                    bp++;
                    mask >>= 1;
                } while (mask & rightMask);
            }
            bpY += bw;
            vpY += byte_per_line;
        }
    }

    // We don't need this if the next call is a DFB blt to VGA (as in the case of moving the mouse pointer)
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
}

/* This algorithm goes from left to right, and inside that loop, top to bottom.
 * It also stores each 4BPP pixel in an entire byte. */
void DFB_BltToVGA(int x, int y, int w, int h, void *b, int bw)
{
    unsigned char *bp, *bpX;
    unsigned char *vp, *vpX;
    unsigned char mask;
    //volatile unsigned char dummy;
    int byte_per_line;
    int i, j;

    bpX = b;
    ASSIGNVP4(x, y, vpX)
    ASSIGNMK4(x, y, mask)
    byte_per_line = SCREEN_X >> 3;

    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);      // bit mask

    for (i=w; i>0; i--)
    {
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, mask);
        bp = bpX;
        vp = vpX;
        for (j = h; j > 0; j--)
        {
            //dummy = *vp;
            *vp = *bp;
            bp += bw;
            vp += byte_per_line;
        }
        bpX++;
        if ((mask >>= 1) == 0)
        {
            vpX++;
            mask = 0x80;
        }
    }
}

/* This algorithm goes from goes from left to right, and inside that loop, top to bottom.
 * It also stores each 4BPP pixel in an entire byte. */
void DFB_BltToVGA_Transparent(int x, int y, int w, int h, void *b, int bw, char Trans)
{
    unsigned char *bp, *bpX;
    unsigned char *vp, *vpX;
    unsigned char mask;
    //volatile unsigned char dummy;
    int byte_per_line;
    int i, j;

    bpX = b;
    ASSIGNVP4(x, y, vpX)
    ASSIGNMK4(x, y, mask)
    byte_per_line = SCREEN_X >> 3;

    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);      // bit mask

    for (i=w; i>0; i--)
    {
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, mask);
        bp = bpX;
        vp = vpX;
        for (j=h; j>0; j--)
        {
            if (*bp != Trans)
            {
                //dummy = *vp;
                *vp = *bp;
            }
            bp += bw;
            vp += byte_per_line;
        }
        bpX++;
        if ((mask >>= 1) == 0)
        {
            vpX++;
            mask = 0x80;
        }
    }
}

/* This algorithm converts a DFB into a DIB
 * WARNING: This algorithm is buggy */
void DFB_BltToDIB(int x, int y, int w, int h, void *b, int bw, void *bdib, int dibw)
{
    unsigned char *bp, *bpX, *dib, *dibTmp;
    int i, j, dib_shift;

    bpX = b;
    dib = (unsigned char *)bdib + y * dibw + (x / 2);

    for (i=w; i>0; i--)
    {
        /* determine the bit shift for the DIB pixel */
        dib_shift = mod2(w-i);
        if(dib_shift > 0)
            dib_shift = 4;
        dibTmp = dib;

        bp = bpX;
        for (j = h; j > 0; j--)
        {
            *dibTmp = *bp << dib_shift | *(bp + 1);
            dibTmp += dibw;
            bp += bw;
        }
        bpX++;
        if(dib_shift == 0)
            dib++;
    }
}

/* This algorithm converts a DIB into a DFB */
void DIB_BltToDFB(int x, int y, int w, int h, void *b, int bw, void *bdib, int dibw)
{
    unsigned char *bp, *bpX, *dib, *dibTmp;
    int i, j, dib_shift, dib_and;

    bpX = b;
    dib = (unsigned char *)bdib + y * dibw + (x / 2);

    for (i=w; i>0; i--)
    {
        /* determine the bit shift for the DIB pixel */
        dib_shift = mod2(w-i);
        if(dib_shift > 0)
        {
            dib_shift = 0;
            dib_and = 0x0f;
        }
        else
        {
            dib_shift = 4;
            dib_and = 0xf0;
        }

        dibTmp = dib;
        bp = bpX;

        for (j=h; j>0; j--)
        {
            *bp = (*dibTmp & dib_and) >> dib_shift;
            dibTmp += dibw;
            bp += bw;
        }

        bpX++;
        if (dib_shift == 0)
            dib++;
    }
}
