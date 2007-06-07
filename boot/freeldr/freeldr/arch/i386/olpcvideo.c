/* $Id: xboxvideo.c 25800 2007-02-14 20:30:33Z ion $
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Programmer: Aleksey Bragin <aleksey reactos org>
 */

#include <freeldr.h>

/* Include for jpg support */
#include "decode-jpg.h"
#include "olpc_logo.h"


extern int chosen_package;
extern FILE *stdout_handle;

int get_int_prop(phandle node, char *key);
int decode_int(UCHAR *p);
VOID XboxVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor);

char *FrameBuffer = NULL;
int ScreenWidth, ScreenHeight, Stride, Depth, BytesPerPixel;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define TOP_BOTTOM_LINES 0


// Private functions
VOID OlpcDisplayLogo();
WORD Convert888to565(ULONG Pixel);

/*
 * Access to machine-specific registers (available on 586 and better only)
 * Note: the rd* operations modify the parameters directly (without using
 * pointer indirection), this allows gcc to optimize better
 */

#define rdmsr(msr,val1,val2) \
	__asm__ __volatile__("rdmsr" \
			  : "=a" (val1), "=d" (val2) \
			  : "c" (msr))

#define wrmsr(msr,val1,val2) \
	__asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

#define MSR_LX_GLIU0_P2D_RO0 0x10000029

unsigned int lx_framebuffer_size(void)
{
    unsigned int val;

    if (/*machine_is_olpc() && !olpc_has_vsa()*/TRUE)
    {
        unsigned int hi,lo;
        rdmsr(MSR_LX_GLIU0_P2D_RO0, lo, hi);

        /* Top page number */
        val = ((hi & 0xff) << 12) | ((lo & 0xfff00000) >> 20);
        val -= (lo & 0x000fffff); /* Subtract bottom page number */
        val += 1; /* Adjust page count */
        return (val << 12);
    }
}


VOID OlpcVideoInit()
{
    int OutDevice;
    unsigned int Node, i;
    char Type[16];
    UCHAR Buffer[4];

    /* Get the stdout device's handle */
    OutDevice = get_int_prop(chosen_package, "stdout");
    Node = OFInstanceToPackage(OutDevice);

    OFGetprop(Node, "device_type", Type, sizeof(Type));

    if (strcmp(Type, "display") != 0)
        return;

    OFGetprop(Node, "depth", (char *)&Buffer, sizeof(int));
    Depth = decode_int(Buffer);

    OFGetprop(Node, "width", (char *)&Buffer, sizeof(int));
    ScreenWidth = decode_int(Buffer);
    OFGetprop(Node, "height", (char *)&Buffer, sizeof(int));
    ScreenHeight = decode_int(Buffer);
    OFGetprop(Node, "linebytes", (char *)&Buffer, sizeof(int));
    Stride = decode_int(Buffer);

    ofwprintf("W: %d, H: %d, Stride: %d, Depth: %d\n", ScreenWidth, ScreenHeight, Stride, Depth);

    OFGetprop(Node, "address", &Buffer, sizeof(int));
    FrameBuffer = decode_int(Buffer);

    ofwprintf("Addr: %x\n", FrameBuffer);

    /* Calculate BPP and Delta based on acquired values */
    BytesPerPixel = Depth / 8;

    //OlpcDisplayLogo();
    //while (TRUE) {};
}



void OlpcVideoClearScreen( UCHAR Attr )
{
}

void OlpcVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth )
{
  *Width = ScreenWidth / CHAR_WIDTH;
  *Height = (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
  *Depth = 16;
}

VIDEODISPLAYMODE OlpcVideoSetDisplayMode( char *DisplayMode, BOOLEAN Init )
{
    //printf( "DisplayMode: %s %s\n", DisplayMode, Init ? "true" : "false" );
    return VideoTextMode;
}

ULONG OlpcVideoGetBufferSize()
{
    ULONG Width, Height, Depth;
    MachVideoGetDisplaySize( &Width, &Height, &Depth );
    return Width * Height * Depth / 8;
}

void OlpcVideoHideShowTextCursor( BOOLEAN Show )
{
    //ofwprintf("HideShowTextCursor(%s)\n", Show ? "true" : "false");
}

VOID OlpcVideoOutputCharInt(UCHAR Char, unsigned X, unsigned Y, WORD FgColor, WORD BgColor)
{
    PUCHAR FontPtr;
    WORD *Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;

    FontPtr = XboxFont8x16 + Char * 16;
    Pixel = (WORD *) ((char *) FrameBuffer + (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * Stride
        + X * CHAR_WIDTH * BytesPerPixel);
    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (WORD *) ((char *) Pixel + Stride);
    }
}

VOID OlpcVideoPutChar( int Ch, UCHAR Attr, unsigned X, unsigned Y )
{
    ULONG FgColor, BgColor;

    XboxVideoAttrToColors(Attr, &FgColor, &BgColor);
    OlpcVideoOutputCharInt(Ch, X, Y, Convert888to565(FgColor), Convert888to565(BgColor));
}

VOID OlpcVideoCopyOffScreenBufferToVRAM( PVOID Buffer )
{
    PUCHAR OffScreenBuffer = (PUCHAR) Buffer;
    ULONG Col, Line;

    for (Line = 0; Line < (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < ScreenWidth / CHAR_WIDTH; Col++)
        {
            OlpcVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += 2;
        }
    }
}

BOOLEAN OlpcVideoIsPaletteFixed()
{
    return FALSE;
}

VOID OlpcVideoSetPaletteColor( UCHAR Color, 
                              UCHAR Red, UCHAR Green, UCHAR Blue )
{
    //ofwprintf( "SetPaletteColor(%x,%x,%x,%x)\n", Color, Red, Green, Blue );
}

VOID OlpcVideoGetPaletteColor( UCHAR Color, 
                              UCHAR *Red, UCHAR *Green, UCHAR *Blue )
{
    //ofwprintf( "GetPaletteColor(%x)\n", Color);
}

VOID OlpcVideoSync()
{
    //ofwprintf( "Sync\n" );
}

VOID OlpcVideoPrepareForReactOS(IN BOOLEAN Setup)
{
}

int VideoPictureDecode(BYTE *pbaJpegFileImage, int nFileLength,JPEG * pJpeg)
{
    struct jpeg_decdata *decdata;
    int size, width, height, depth=0;

    decdata = (struct jpeg_decdata *)malloc(sizeof(struct jpeg_decdata));
    memset(decdata, 0x0, sizeof(struct jpeg_decdata));

    jpeg_get_size(pbaJpegFileImage, &width, &height, &depth);
    size = ((width + 15) & ~15) * ((height + 15) & ~15) * (depth >> 3);

    pJpeg->pData = (unsigned char *)malloc(size);
    memset(pJpeg->pData, 0x0, size);

    pJpeg->width = ((width + 15) & ~15);
    pJpeg->height = ((height +15) & ~ 15);
    pJpeg->bpp = depth >> 3;

    if((jpeg_decode(pbaJpegFileImage, pJpeg->pData, 
        ((width + 15) & ~15), ((height + 15) & ~15), depth, decdata)) != 0) {
            ////		!!!!
            //printk("Error decode picture\n");
            //while(1);
            return 1;
    }

    pJpeg->pBackdrop = pJpeg->pData;

    free(decdata);

    return 0;
}

WORD Convert888to565(ULONG Pixel)
{
    BYTE red, green, blue;

    red = Pixel & 0xFF;
    green = (Pixel & 0xFF00) >> 8;
    blue = (Pixel & 0xFF0000) >> 16;

    red = red >> 3; // 5
    green = green >> 2; // 6
    blue = blue >> 3; // 5

    return (blue & 0x1F)|
        ((green << 5) & 0x7E0) |
        ((red << 11) & 0xF800);
}

void BootVideoClearScreen(JPEG *pJpeg, int nStartLine, int nEndLine)
{
    if(nEndLine>=ScreenHeight) nEndLine=ScreenHeight-1;
    {
        if(pJpeg->pData!=NULL)
        {
            volatile WORD *pdw=((DWORD *)FrameBuffer)+ScreenWidth*nStartLine;
            int n1=pJpeg->bpp * pJpeg->width * nStartLine;
            BYTE *pbJpegBitmapAdjustedDatum=pJpeg->pBackdrop;

            while(nStartLine++<nEndLine)
            {
                int n;
                for(n=0;n<ScreenWidth;n++)
                {
                    BYTE red, green, blue;

                    red = pbJpegBitmapAdjustedDatum[n1] >> 3; // 5
                    green = pbJpegBitmapAdjustedDatum[n1+1] >> 2; // 6
                    blue = pbJpegBitmapAdjustedDatum[n1+2] >> 3; // 5

                    pdw[n]=  (blue & 0x1F)|
                            ((green << 5) & 0x7E0) |
                            ((red << 11) & 0xF800);
                    n1+=pJpeg->bpp;
                }
                n1+=pJpeg->bpp * (pJpeg->width - ScreenWidth);
                pdw = (WORD*)((UCHAR *)pdw + Stride); // adding DWORD footprints
            }
        }
    }
}

VOID OlpcDisplayLogo()
{
    JPEG *jpegBackdrop;

    jpegBackdrop= (JPEG*) malloc(sizeof(JPEG));
    memset(jpegBackdrop, 0x0, sizeof(sizeof(JPEG)));

    // decode and malloc backdrop bitmap (Default)
    VideoPictureDecode(olpc_freeloader_logo, sizeof(olpc_freeloader_logo), jpegBackdrop);
    BootVideoClearScreen(jpegBackdrop, 0, 0xffff);
}
