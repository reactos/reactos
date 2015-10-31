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

VOID DecompressBitmap(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta, ULONG Format, ULONG cjSizeImage)
{
    INT x = 0;
    INT y = Size.cy - 1;
    INT c;
    INT length;
    INT width;
    INT height = y;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    INT shift = 0;

    if ((Format == BMF_4RLE) || (Format == BMF_4BPP))
        shift = 1;
    else if ((Format != BMF_8RLE) && (Format != BMF_8BPP))
        return;

    width = ((Size.cx + shift) >> shift);

    _SEH2_TRY
    {
        while (y >= 0 && (bits - begin) <= cjSizeImage)
        {
            length = (*bits++) >> shift;
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
                    _SEH2_YIELD(return);
                case RLE_DELTA:
                    x += (*bits++) >> shift;
                    y -= (*bits++) >> shift;
                    break;
                default:
                    length = length >> shift;
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
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Decoding error\n");
    }
    _SEH2_END;

    return;
}
