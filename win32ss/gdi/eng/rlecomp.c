/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           RLE compression
 * FILE:              win32ss/gdi/eng/rlecomp.c
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

VOID DecompressBitmap(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits,
                      LONG Delta, ULONG Format, ULONG cjSizeImage)
{
    INT x = 0, y = Size.cy - 1;
    INT i, c, c2, length;
    INT width = Size.cx, height = y;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    BOOL is4bpp = FALSE;

    if ((Format == BMF_4RLE) || (Format == BMF_4BPP))
        is4bpp = TRUE;
    else if ((Format != BMF_8RLE) && (Format != BMF_8BPP))
        return;

    _SEH2_TRY
    {
        while (y >= 0 && (bits - begin) <= cjSizeImage)
        {
            length = *bits++;
            if (length)
            {
                c = *bits++;
                for (i = 0; i < length; i++)
                {
                    if (x >= width) break;
                    temp = UncompressedBits + (height - y) * Delta;
                    if (is4bpp)
                    {
                        temp += x / 2;
                        if (i & 1)
                            c2 = c & 0x0F;
                        else
                            c2 = c >> 4;
                        if (x & 1)
                            *temp |= c2;
                        else
                            *temp |= c2 << 4;
                    }
                    else
                    {
                        temp += x;
                        *temp = c;
                    }
                    x++;
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
                    x += *bits++;
                    y -= *bits++;
                    break;
                default:
                    for (i = 0; i < length; i++)
                    {
                        if (!(is4bpp && i & 1))
                            c = *bits++;

                        if (x < width)
                        {
                            temp = UncompressedBits + (height - y) * Delta;
                            if (is4bpp)
                            {
                                temp += x / 2;
                                if (i & 1)
                                    c2 = c & 0x0F;
                                else
                                    c2 = c >> 4;
                                if (x & 1)
                                    *temp |= c2;
                                else
                                    *temp |= c2 << 4;
                            }
                            else
                            {
                                temp += x;
                                *temp = c;
                            }
                            x++;
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
