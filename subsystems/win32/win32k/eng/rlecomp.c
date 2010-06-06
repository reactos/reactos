/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           RLE compression
 * FILE:              subsystems/win32k/eng/rlecomp.c
 * PROGRAMER:         Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

enum Rle_EscapeCodes
{
    RLE_EOL   = 0, /* End of line */
    RLE_END   = 1, /* End of bitmap */
    RLE_DELTA = 2  /* Delta */
};

VOID Decompress4bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta)
{
    int x = 0;
    int y = Size.cy - 1;
    int c;
    int length;
    int width = ((Size.cx+1)/2);
    int height = Size.cy - 1;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    while (y >= 0)
    {
        length = *bits++ / 2;
        if (length)
        {
            c = *bits++;
            while (length--)
            {
                if (x >= width) break;
                temp = UncompressedBits + (((height - y) * Delta) + x);
                x++;
                *temp = c;
            }
        }
        else
        {
            length = *bits++;
            switch (length)
            {
                case RLE_EOL:
                    x = 0;
                    y--;
                    break;
                case RLE_END:
                    return;
                case RLE_DELTA:
                    x += (*bits++)/2;
                    y -= (*bits++)/2;
                    break;
                default:
                    length /= 2;
                    while (length--)
                    {
                        c = *bits++;
                        if (x < width)
                        {
                            temp = UncompressedBits + (((height - y) * Delta) + x);
                            x++;
                            *temp = c;
                        }
                    }
                    if ((bits - begin) & 1)
                    {
                        bits++;
                    }
            }
        }
    }
}

VOID Decompress8bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta)
{
    int x = 0;
    int y = Size.cy - 1;
    int c;
    int length;
    int width = Size.cx;
    int height = Size.cy - 1;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    while (y >= 0)
    {
        length = *bits++;
        if (length)
        {
            c = *bits++;
            while (length--)
            {
                if (x >= width) break;
                temp = UncompressedBits + (((height - y) * Delta) + x);
                x++;
                *temp = c;
            }
        }
        else
        {
            length = *bits++;
            switch (length)
            {
                case RLE_EOL:
                    x = 0;
                    y--;
                    break;
                case RLE_END:
                    return;
                case RLE_DELTA:
                    x += *bits++;
                    y -= *bits++;
                    break;
                default:
                    while (length--)
                    {
                        c = *bits++;
                        if (x < width)
                        {
                            temp = UncompressedBits + (((height - y) * Delta) + x);
                            x++;
                            *temp = c;
                        }
                    }
                    if ((bits - begin) & 1)
                    {
                        bits++;
                    }
            }
        }
    }
}
