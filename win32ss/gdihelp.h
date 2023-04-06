/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdihelp.h
 * PURPOSE:         Dib object helper functions
 * PROGRAMMER:
 */

ULONG
FASTCALL
BitmapFormat(ULONG cBits, ULONG iCompression)
{
    switch (iCompression)
    {
        case BI_RGB:
            /* Fall through */
        case BI_BITFIELDS:
            if (cBits <= 1) return BMF_1BPP;
            if (cBits <= 4) return BMF_4BPP;
            if (cBits <= 8) return BMF_8BPP;
            if (cBits <= 16) return BMF_16BPP;
            if (cBits <= 24) return BMF_24BPP;
            if (cBits <= 32) return BMF_32BPP;
            return 0;

        case BI_RLE4:
            return BMF_4RLE;

        case BI_RLE8:
            return BMF_8RLE;

        default:
            return 0;
    }
}

UCHAR
gajBitsPerFormat[11] =
{
    0, /*  0: unused */
    1, /*  1: BMF_1BPP */
    4, /*  2: BMF_4BPP */
    8, /*  3: BMF_8BPP */
   16, /*  4: BMF_16BPP */
   24, /*  5: BMF_24BPP */
   32, /*  6: BMF_32BPP */
    4, /*  7: BMF_4RLE */
    8, /*  8: BMF_8RLE */
    0, /*  9: BMF_JPEG */
    0, /* 10: BMF_PNG */
};

