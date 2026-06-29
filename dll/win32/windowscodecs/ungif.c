/*
 * Gif extracting routines - derived from libungif
 *
 * Portions Copyright 2006 Mike McCormack
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

/*
 * Original copyright notice:
 *
 * The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */


/******************************************************************************
 *   "Gif-Lib" - Yet another gif library.
 *
 * Written by:  Gershon Elber            IBM PC Ver 1.1,    Aug. 1990
 ******************************************************************************
 * The kernel of the GIF Decoding process can be found here.
 ******************************************************************************
 * History:
 * 16 Jun 89 - Version 1.0 by Gershon Elber.
 *  3 Sep 90 - Version 1.1 by Gershon Elber (Support for Gif89, Unique names).
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "ungif.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#define LZ_MAX_CODE         4095    /* Biggest code possible in 12 bits. */
#define LZ_BITS             12

#define NO_SUCH_CODE        4098    /* Impossible code, to signal empty. */

typedef struct GifFilePrivateType {
    GifWord BitsPerPixel,     /* Bits per pixel (Codes uses at least this + 1). */
      ClearCode,   /* The CLEAR LZ code. */
      EOFCode,     /* The EOF LZ code. */
      RunningCode, /* The next code algorithm can generate. */
      RunningBits, /* The number of bits required to represent RunningCode. */
      MaxCode1,    /* 1 bigger than max. possible code, in RunningBits bits. */
      LastCode,    /* The code before the current code. */
      CrntCode,    /* Current algorithm code. */
      StackPtr,    /* For character stack (see below). */
      CrntShiftState;    /* Number of bits in CrntShiftDWord. */
    unsigned long CrntShiftDWord;   /* For bytes decomposition into codes. */
    unsigned long PixelCount;   /* Number of pixels in image. */
    InputFunc Read;     /* function to read gif input (TVT) */
    GifByteType Buf[256];   /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE]; /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE + 1];    /* So we can trace the codes. */
    GifPrefixType Prefix[LZ_MAX_CODE + 1];
} GifFilePrivateType;

/* avoid extra function call in case we use fread (TVT) */
#define READ(_gif,_buf,_len) \
    ((GifFilePrivateType*)_gif->Private)->Read(_gif,_buf,_len)

static int DGifGetWord(GifFileType *GifFile, GifWord *Word);
static int DGifSetupDecompress(GifFileType *GifFile);
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line, int LineLen);
static int DGifGetPrefixChar(const GifPrefixType *Prefix, int Code, int ClearCode);
static int DGifDecompressInput(GifFileType *GifFile, int *Code);
static int DGifBufferedInput(GifFileType *GifFile, GifByteType *Buf,
                             GifByteType *NextByte);

static int DGifGetExtensionNext(GifFileType * GifFile, GifByteType ** GifExtension);
static int DGifGetCodeNext(GifFileType * GifFile, GifByteType ** GifCodeBlock);

/******************************************************************************
 * Miscellaneous utility functions
 *****************************************************************************/

/* return smallest bitfield size n will fit in */
static int
BitSize(int n) {

    register int i;

    for (i = 1; i <= 8; i++)
        if ((1 << i) >= n)
            break;
    return (i);
}

/******************************************************************************
 * Color map object functions
 *****************************************************************************/

/*
 * Allocate a color map of given size; initialize with contents of
 * ColorMap if that pointer is non-NULL.
 */
static ColorMapObject *
MakeMapObject(int ColorCount,
              const GifColorType * ColorMap) {

    ColorMapObject *Object;

    /*** FIXME: Our ColorCount has to be a power of two.  Is it necessary to
     * make the user know that or should we automatically round up instead? */
    if (ColorCount != (1 << BitSize(ColorCount))) {
        return NULL;
    }

    Object = malloc(sizeof(ColorMapObject));
    if (Object == NULL) {
        return NULL;
    }

    Object->Colors = calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == NULL) {
        free(Object);
        return NULL;
    }

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = BitSize(ColorCount);

    if (ColorMap) {
        memcpy(Object->Colors, ColorMap, ColorCount * sizeof(GifColorType));
    }

    return (Object);
}

/*
 * Free a color map object
 */
static void
FreeMapObject(ColorMapObject * Object) {

    if (Object != NULL) {
        free(Object->Colors);
        free(Object);
        /*** FIXME:
         * When we are willing to break API we need to make this function
         * FreeMapObject(ColorMapObject **Object)
         * and do this assignment to NULL here:
         * *Object = NULL;
         */
    }
}

static int
AddExtensionBlock(Extensions *New,
                  int Len,
                  const unsigned char ExtData[]) {

    ExtensionBlock *ep;

    New->ExtensionBlocks = realloc(New->ExtensionBlocks,
                                   sizeof(ExtensionBlock) *
                                   (New->ExtensionBlockCount + 1));

    if (New->ExtensionBlocks == NULL)
        return (GIF_ERROR);

    ep = &New->ExtensionBlocks[New->ExtensionBlockCount++];

    ep->ByteCount=Len + 3;
    ep->Bytes = malloc(ep->ByteCount + 3);
    if (ep->Bytes == NULL)
        return (GIF_ERROR);

    /* Extension Header */
    ep->Bytes[0] = 0x21;
    ep->Bytes[1] = New->Function;
    ep->Bytes[2] = Len;

    if (ExtData) {
        memcpy(ep->Bytes + 3, ExtData, Len);
        ep->Function = New->Function;
    }

    return (GIF_OK);
}

static int
AppendExtensionBlock(Extensions *New,
                     int Len,
                     const unsigned char ExtData[])
{
    ExtensionBlock *ep;

    if (New->ExtensionBlocks == NULL)
        return (GIF_ERROR);

    ep = &New->ExtensionBlocks[New->ExtensionBlockCount - 1];

    ep->Bytes = realloc(ep->Bytes, ep->ByteCount + Len + 1);
    if (ep->Bytes == NULL)
        return (GIF_ERROR);

    ep->Bytes[ep->ByteCount] = Len;

    if (ExtData)
        memcpy(ep->Bytes + ep->ByteCount + 1, ExtData, Len);

    ep->ByteCount += Len + 1;

    return (GIF_OK);
}

static void
FreeExtension(Extensions *Extensions)
{
    ExtensionBlock *ep;

    if ((Extensions == NULL) || (Extensions->ExtensionBlocks == NULL)) {
        return;
    }
    for (ep = Extensions->ExtensionBlocks;
         ep < (Extensions->ExtensionBlocks + Extensions->ExtensionBlockCount); ep++)
        free(ep->Bytes);
    free(Extensions->ExtensionBlocks);
    Extensions->ExtensionBlocks = NULL;
}

/******************************************************************************
 * Image block allocation functions
******************************************************************************/

static void
FreeSavedImages(GifFileType * GifFile) {

    SavedImage *sp;

    if ((GifFile == NULL) || (GifFile->SavedImages == NULL)) {
        return;
    }
    for (sp = GifFile->SavedImages;
         sp < GifFile->SavedImages + GifFile->ImageCount; sp++) {
        if (sp->ImageDesc.ColorMap) {
            FreeMapObject(sp->ImageDesc.ColorMap);
            sp->ImageDesc.ColorMap = NULL;
        }

        free(sp->RasterBits);

        if (sp->Extensions.ExtensionBlocks)
            FreeExtension(&sp->Extensions);
    }
    free(GifFile->SavedImages);
    GifFile->SavedImages=NULL;
}

/******************************************************************************
 * This routine should be called before any other DGif calls. Note that
 * this routine is called automatically from DGif file open routines.
 *****************************************************************************/
static int
DGifGetScreenDesc(GifFileType * GifFile) {

    int i, BitsPerPixel, SortFlag;
    GifByteType Buf[3];

    /* Put the screen descriptor into the file: */
    if (DGifGetWord(GifFile, &GifFile->SWidth) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->SHeight) == GIF_ERROR)
        return GIF_ERROR;

    if (READ(GifFile, Buf, 3) != 3) {
        return GIF_ERROR;
    }
    GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
    SortFlag = (Buf[0] & 0x08) != 0;
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->SColorTableSize = 1 << BitsPerPixel;
    GifFile->SBackGroundColor = Buf[1];
    GifFile->SAspectRatio = Buf[2];
    if (Buf[0] & 0x80) {    /* Do we have global color map? */

        GifFile->SColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->SColorMap == NULL) {
            return GIF_ERROR;
        }

        /* Get the global color map: */
        GifFile->SColorMap->SortFlag = SortFlag;
        for (i = 0; i < GifFile->SColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                FreeMapObject(GifFile->SColorMap);
                GifFile->SColorMap = NULL;
                return GIF_ERROR;
            }
            GifFile->SColorMap->Colors[i].Red = Buf[0];
            GifFile->SColorMap->Colors[i].Green = Buf[1];
            GifFile->SColorMap->Colors[i].Blue = Buf[2];
        }
    } else {
        GifFile->SColorMap = NULL;
    }

    return GIF_OK;
}

/******************************************************************************
 * This routine should be called before any attempt to read an image.
 *****************************************************************************/
static int
DGifGetRecordType(GifFileType * GifFile,
                  GifRecordType * Type) {

    GifByteType Buf;

    if (READ(GifFile, &Buf, 1) != 1) {
        /* Wine-specific behavior: Native accepts broken GIF files that have no
         * terminator, so we match this by treating EOF as a terminator. */
        *Type = TERMINATE_RECORD_TYPE;
        return GIF_OK;
    }

    switch (Buf) {
      case ',':
          *Type = IMAGE_DESC_RECORD_TYPE;
          break;
      case '!':
          *Type = EXTENSION_RECORD_TYPE;
          break;
      case ';':
          *Type = TERMINATE_RECORD_TYPE;
          break;
      default:
          *Type = UNDEFINED_RECORD_TYPE;
          return GIF_ERROR;
    }

    return GIF_OK;
}

/******************************************************************************
 * This routine should be called before any attempt to read an image.
 * Note it is assumed the Image desc. header (',') has been read.
 *****************************************************************************/
static int
DGifGetImageDesc(GifFileType * GifFile) {

    int i, BitsPerPixel, SortFlag;
    GifByteType Buf[3];
    GifFilePrivateType *Private = GifFile->Private;
    SavedImage *sp;

    if (DGifGetWord(GifFile, &GifFile->Image.Left) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Top) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Width) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Height) == GIF_ERROR)
        return GIF_ERROR;
    if (READ(GifFile, Buf, 1) != 1) {
        return GIF_ERROR;
    }
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    SortFlag = (Buf[0] & 0x20) != 0;
    GifFile->Image.Interlace = (Buf[0] & 0x40);
    if (Buf[0] & 0x80) {    /* Does this image have local color map? */

        FreeMapObject(GifFile->Image.ColorMap);

        GifFile->Image.ColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->Image.ColorMap == NULL) {
            return GIF_ERROR;
        }

        /* Get the image local color map: */
        GifFile->Image.ColorMap->SortFlag = SortFlag;
        for (i = 0; i < GifFile->Image.ColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                FreeMapObject(GifFile->Image.ColorMap);
                GifFile->Image.ColorMap = NULL;
                return GIF_ERROR;
            }
            GifFile->Image.ColorMap->Colors[i].Red = Buf[0];
            GifFile->Image.ColorMap->Colors[i].Green = Buf[1];
            GifFile->Image.ColorMap->Colors[i].Blue = Buf[2];
        }
    } else if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if ((GifFile->SavedImages = realloc(GifFile->SavedImages,
                                        sizeof(SavedImage) *
                                        (GifFile->ImageCount + 1))) == NULL) {
        return GIF_ERROR;
    }

    sp = &GifFile->SavedImages[GifFile->ImageCount];
    sp->ImageDesc = GifFile->Image;
    if (GifFile->Image.ColorMap != NULL) {
        sp->ImageDesc.ColorMap = MakeMapObject(
                                 GifFile->Image.ColorMap->ColorCount,
                                 GifFile->Image.ColorMap->Colors);
        if (sp->ImageDesc.ColorMap == NULL) {
            return GIF_ERROR;
        }
        sp->ImageDesc.ColorMap->SortFlag = GifFile->Image.ColorMap->SortFlag;
    }
    sp->RasterBits = NULL;
    sp->Extensions.ExtensionBlockCount = 0;
    sp->Extensions.ExtensionBlocks = NULL;

    GifFile->ImageCount++;

    Private->PixelCount = GifFile->Image.Width * GifFile->Image.Height;

    DGifSetupDecompress(GifFile);  /* Reset decompress algorithm parameters. */

    return GIF_OK;
}

/******************************************************************************
 * Get one full scanned line (Line) of length LineLen from GIF file.
 *****************************************************************************/
static int
DGifGetLine(GifFileType * GifFile,
            GifPixelType * Line,
            int LineLen) {

    GifByteType *Dummy;
    GifFilePrivateType *Private = GifFile->Private;

    if (!LineLen)
        LineLen = GifFile->Image.Width;

    if ((Private->PixelCount -= LineLen) > 0xffff0000UL) {
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, Line, LineLen) == GIF_OK) {
        if (Private->PixelCount == 0) {
            /* We probably would not be called any more, so lets clean
             * everything before we return: need to flush out all rest of
             * image until empty block (size 0) detected. We use GetCodeNext. */
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                {
                    WARN("GIF is not properly terminated\n");
                    break;
                }
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

/******************************************************************************
 * Get an extension block (see GIF manual) from gif file. This routine only
 * returns the first data block, and DGifGetExtensionNext should be called
 * after this one until NULL extension is returned.
 * The Extension should NOT be freed by the user (not dynamically allocated).
 * Note it is assumed the Extension desc. header ('!') has been read.
 *****************************************************************************/
static int
DGifGetExtension(GifFileType * GifFile,
                 int *ExtCode,
                 GifByteType ** Extension) {

    GifByteType Buf;

    if (READ(GifFile, &Buf, 1) != 1) {
        return GIF_ERROR;
    }
    *ExtCode = Buf;

    return DGifGetExtensionNext(GifFile, Extension);
}

/******************************************************************************
 * Get a following extension block (see GIF manual) from gif file. This
 * routine should be called until NULL Extension is returned.
 * The Extension should NOT be freed by the user (not dynamically allocated).
 *****************************************************************************/
static int
DGifGetExtensionNext(GifFileType * GifFile,
                     GifByteType ** Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        return GIF_ERROR;
    }
    if (Buf > 0) {
        *Extension = Private->Buf;    /* Use private unused buffer. */
        (*Extension)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
        if (READ(GifFile, &((*Extension)[1]), Buf) != Buf) {
            return GIF_ERROR;
        }
    } else
        *Extension = NULL;

    return GIF_OK;
}

/******************************************************************************
 * Get 2 bytes (word) from the given file:
 *****************************************************************************/
static int
DGifGetWord(GifFileType * GifFile,
            GifWord *Word) {

    unsigned char c[2];

    if (READ(GifFile, c, 2) != 2) {
        return GIF_ERROR;
    }

    *Word = (((unsigned int)c[1]) << 8) + c[0];
    return GIF_OK;
}

/******************************************************************************
 * Continue to get the image code in compressed form. This routine should be
 * called until NULL block is returned.
 * The block should NOT be freed by the user (not dynamically allocated).
 *****************************************************************************/
static int
DGifGetCodeNext(GifFileType * GifFile,
                GifByteType ** CodeBlock) {

    GifByteType Buf;
    GifFilePrivateType *Private = GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        return GIF_ERROR;
    }

    if (Buf > 0) {
        *CodeBlock = Private->Buf;    /* Use private unused buffer. */
        (*CodeBlock)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
        if (READ(GifFile, &((*CodeBlock)[1]), Buf) != Buf) {
            return GIF_ERROR;
        }
    } else {
        *CodeBlock = NULL;
        Private->Buf[0] = 0;    /* Make sure the buffer is empty! */
        Private->PixelCount = 0;    /* And local info. indicate image read. */
    }

    return GIF_OK;
}

/******************************************************************************
 * Setup the LZ decompression for this image:
 *****************************************************************************/
static int
DGifSetupDecompress(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType CodeSize;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = GifFile->Private;

    READ(GifFile, &CodeSize, 1);    /* Read Code size from file. */
    BitsPerPixel = CodeSize;

    Private->Buf[0] = 0;    /* Input Buffer empty. */
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;    /* Number of bits per code. */
    Private->MaxCode1 = 1 << Private->RunningBits;    /* Max. code + 1. */
    Private->StackPtr = 0;    /* No pixels on the pixel stack. */
    Private->LastCode = NO_SUCH_CODE;
    Private->CrntShiftState = 0;    /* No information in CrntShiftDWord. */
    Private->CrntShiftDWord = 0;

    Prefix = Private->Prefix;
    for (i = 0; i <= LZ_MAX_CODE; i++)
        Prefix[i] = NO_SUCH_CODE;

    return GIF_OK;
}

/******************************************************************************
 * The LZ decompression routine:
 * This version decompress the given gif file into Line of length LineLen.
 * This routine can be called few times (one per scan line, for example), in
 * order the complete the whole image.
 *****************************************************************************/
static int
DGifDecompressLine(GifFileType * GifFile,
                   GifPixelType * Line,
                   int LineLen) {

    int i = 0;
    int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
    GifByteType *Stack, *Suffix;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = GifFile->Private;

    StackPtr = Private->StackPtr;
    Prefix = Private->Prefix;
    Suffix = Private->Suffix;
    Stack = Private->Stack;
    EOFCode = Private->EOFCode;
    ClearCode = Private->ClearCode;
    LastCode = Private->LastCode;

    if (StackPtr != 0) {
        /* Let pop the stack off before continuing to read the gif file: */
        while (StackPtr != 0 && i < LineLen)
            Line[i++] = Stack[--StackPtr];
    }

    while (i < LineLen) {    /* Decode LineLen items. */
        if (DGifDecompressInput(GifFile, &CrntCode) == GIF_ERROR)
            return GIF_ERROR;

        if (CrntCode == EOFCode) {
            /* Note, however, that usually we will not be here as we will stop
             * decoding as soon as we got all the pixel, or EOF code will
             * not be read at all, and DGifGetLine/Pixel clean everything.  */
            if (i != LineLen - 1 || Private->PixelCount != 0) {
                return GIF_ERROR;
            }
            i++;
        } else if (CrntCode == ClearCode) {
            /* We need to start over again: */
            for (j = 0; j <= LZ_MAX_CODE; j++)
                Prefix[j] = NO_SUCH_CODE;
            Private->RunningCode = Private->EOFCode + 1;
            Private->RunningBits = Private->BitsPerPixel + 1;
            Private->MaxCode1 = 1 << Private->RunningBits;
            LastCode = Private->LastCode = NO_SUCH_CODE;
        } else {
            /* It's a regular code - if in pixel range simply add it to output
             * stream, otherwise trace to codes linked list until the prefix
             * is in pixel range: */
            if (CrntCode < ClearCode) {
                /* This is simple - its pixel scalar, so add it to output: */
                Line[i++] = CrntCode;
            } else {
                /* It's a code to be traced: trace the linked list
                 * until the prefix is a pixel, while pushing the suffix
                 * pixels on our stack. If we done, pop the stack in reverse
                 * order (that's what stack is good for!) for output.  */
                if (Prefix[CrntCode] == NO_SUCH_CODE) {
                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    if (CrntCode == Private->RunningCode - 2) {
                        CrntPrefix = LastCode;
                        Suffix[Private->RunningCode - 2] =
                           Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
                                                                 LastCode,
                                                                 ClearCode);
                    } else {
                        return GIF_ERROR;
                    }
                } else
                    CrntPrefix = CrntCode;

                /* Now (if image is O.K.) we should not get a NO_SUCH_CODE
                 * during the trace. As we might loop forever, in case of
                 * defective image, we count the number of loops we trace
                 * and stop if we got LZ_MAX_CODE. Obviously we cannot
                 * loop more than that.  */
                j = 0;
                while (j++ <= LZ_MAX_CODE &&
                       CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
                    Stack[StackPtr++] = Suffix[CrntPrefix];
                    CrntPrefix = Prefix[CrntPrefix];
                }
                if (j >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
                    return GIF_ERROR;
                }
                /* Push the last character on stack: */
                Stack[StackPtr++] = CrntPrefix;

                /* Now lets pop all the stack into output: */
                while (StackPtr != 0 && i < LineLen)
                    Line[i++] = Stack[--StackPtr];
            }
            if (LastCode != NO_SUCH_CODE) {
                Prefix[Private->RunningCode - 2] = LastCode;

                if (CrntCode == Private->RunningCode - 2) {
                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, LastCode, ClearCode);
                } else {
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
                }
            }
            LastCode = CrntCode;
        }
    }

    Private->LastCode = LastCode;
    Private->StackPtr = StackPtr;

    return GIF_OK;
}

/******************************************************************************
 * Routine to trace the Prefixes linked list until we get a prefix which is
 * not code, but a pixel value (less than ClearCode). Returns that pixel value.
 * If image is defective, we might loop here forever, so we limit the loops to
 * the maximum possible if image O.k. - LZ_MAX_CODE times.
 *****************************************************************************/
static int
DGifGetPrefixChar(const GifPrefixType *Prefix,
                  int Code,
                  int ClearCode) {

    int i = 0;

    while (Code > ClearCode && i++ <= LZ_MAX_CODE)
        Code = Prefix[Code];
    return Code;
}

/******************************************************************************
 * The LZ decompression input routine:
 * This routine is responsible for the decompression of the bit stream from
 * 8 bits (bytes) packets, into the real codes.
 * Returns GIF_OK if read successfully.
 *****************************************************************************/
static int
DGifDecompressInput(GifFileType * GifFile,
                    int *Code) {

    GifFilePrivateType *Private = GifFile->Private;

    GifByteType NextByte;
    static const unsigned short CodeMasks[] = {
        0x0000, 0x0001, 0x0003, 0x0007,
        0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff,
        0x0fff
    };
    /* The image can't contain more than LZ_BITS per code. */
    if (Private->RunningBits > LZ_BITS) {
        return GIF_ERROR;
    }

    while (Private->CrntShiftState < Private->RunningBits) {
        /* Needs to get more bytes from input stream for next code: */
        if (DGifBufferedInput(GifFile, Private->Buf, &NextByte) == GIF_ERROR) {
            return GIF_ERROR;
        }
        Private->CrntShiftDWord |=
           ((unsigned long)NextByte) << Private->CrntShiftState;
        Private->CrntShiftState += 8;
    }
    *Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

    Private->CrntShiftDWord >>= Private->RunningBits;
    Private->CrntShiftState -= Private->RunningBits;

    /* If code cannot fit into RunningBits bits, must raise its size. Note
     * however that codes above 4095 are used for special signaling.
     * If we're using LZ_BITS bits already and we're at the max code, just
     * keep using the table as it is, don't increment Private->RunningCode.
     */
    if (Private->RunningCode < LZ_MAX_CODE + 2 &&
            ++Private->RunningCode > Private->MaxCode1 &&
            Private->RunningBits < LZ_BITS) {
        Private->MaxCode1 <<= 1;
        Private->RunningBits++;
    }
    return GIF_OK;
}

/******************************************************************************
 * This routines read one gif data block at a time and buffers it internally
 * so that the decompression routine could access it.
 * The routine returns the next byte from its internal buffer (or read next
 * block in if buffer empty) and returns GIF_OK if successful.
 *****************************************************************************/
static int
DGifBufferedInput(GifFileType * GifFile,
                  GifByteType * Buf,
                  GifByteType * NextByte) {

    if (Buf[0] == 0) {
        /* Needs to read the next buffer - this one is empty: */
        if (READ(GifFile, Buf, 1) != 1) {
            return GIF_ERROR;
        }
        /* There shouldn't be any empty data blocks here as the LZW spec
         * says the LZW termination code should come first.  Therefore we
         * shouldn't be inside this routine at that point.
         */
        if (Buf[0] == 0) {
            return GIF_ERROR;
        }
        if (READ(GifFile, &Buf[1], Buf[0]) != Buf[0]) {
            return GIF_ERROR;
        }
        *NextByte = Buf[1];
        Buf[1] = 2;    /* We use now the second place as last char read! */
        Buf[0]--;
    } else {
        *NextByte = Buf[Buf[1]++];
        Buf[0]--;
    }

    return GIF_OK;
}

/******************************************************************************
 * This routine reads an entire GIF into core, hanging all its state info off
 * the GifFileType pointer.  Call DGifOpenFileName() or DGifOpenFileHandle()
 * first to initialize I/O.  Its inverse is EGifSpew().
 ******************************************************************************/
int
DGifSlurp(GifFileType * GifFile) {

    int ImageSize;
    GifRecordType RecordType;
    SavedImage *sp;
    GifByteType *ExtData;
    Extensions temp_save;

    temp_save.ExtensionBlocks = NULL;
    temp_save.ExtensionBlockCount = 0;

    do {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
            return (GIF_ERROR);

        switch (RecordType) {
          case IMAGE_DESC_RECORD_TYPE:
              if (DGifGetImageDesc(GifFile) == GIF_ERROR)
                  return (GIF_ERROR);

              sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
              ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;

              sp->RasterBits = malloc(ImageSize * sizeof(GifPixelType));
              if (sp->RasterBits == NULL) {
                  return GIF_ERROR;
              }
              if (DGifGetLine(GifFile, sp->RasterBits, ImageSize) ==
                  GIF_ERROR)
                  return (GIF_ERROR);
              if (temp_save.ExtensionBlocks) {
                  sp->Extensions.ExtensionBlocks = temp_save.ExtensionBlocks;
                  sp->Extensions.ExtensionBlockCount = temp_save.ExtensionBlockCount;

                  temp_save.ExtensionBlocks = NULL;
                  temp_save.ExtensionBlockCount = 0;

                  /* FIXME: The following is wrong.  It is left in only for
                   * backwards compatibility.  Someday it should go away. Use
                   * the sp->ExtensionBlocks->Function variable instead. */
                  sp->Extensions.Function = sp->Extensions.ExtensionBlocks[0].Function;
              }
              break;

          case EXTENSION_RECORD_TYPE:
          {
              int Function;
              Extensions *Extensions;

              if (DGifGetExtension(GifFile, &Function, &ExtData) == GIF_ERROR)
                  return (GIF_ERROR);

              if (GifFile->ImageCount || Function == GRAPHICS_EXT_FUNC_CODE)
                  Extensions = &temp_save;
              else
                  Extensions = &GifFile->Extensions;

              Extensions->Function = Function;

              if (ExtData)
              {
                  /* Create an extension block with our data */
                  if (AddExtensionBlock(Extensions, ExtData[0], &ExtData[1]) == GIF_ERROR)
                      return (GIF_ERROR);
              }
              else /* Empty extension block */
              {
                  if (AddExtensionBlock(Extensions, 0, NULL) == GIF_ERROR)
                      return (GIF_ERROR);
              }

              while (ExtData != NULL) {
                  int Len;
                  GifByteType *Data;

                  if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR)
                      return (GIF_ERROR);

                  if (ExtData)
                  {
                      Len = ExtData[0];
                      Data = &ExtData[1];
                  }
                  else
                  {
                      Len = 0;
                      Data = NULL;
                  }

                  if (AppendExtensionBlock(Extensions, Len, Data) == GIF_ERROR)
                      return (GIF_ERROR);
              }
              break;
          }

          case TERMINATE_RECORD_TYPE:
              break;

          default:    /* Should be trapped by DGifGetRecordType */
              break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE);

    /* Just in case the Gif has an extension block without an associated
     * image... (Should we save this into a savefile structure with no image
     * instead? Have to check if the present writing code can handle that as
     * well.... */
    if (temp_save.ExtensionBlocks)
        FreeExtension(&temp_save);

    return (GIF_OK);
}

/******************************************************************************
 * GifFileType constructor with user supplied input function (TVT)
 *****************************************************************************/
GifFileType *
DGifOpen(void *userData,
         InputFunc readFunc) {

    unsigned char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;

    GifFile = malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = malloc(sizeof(GifFilePrivateType));
    if (!Private) {
        free(GifFile);
        return NULL;
    }

    GifFile->Private = (void*)Private;

    Private->Read = readFunc;    /* TVT */
    GifFile->UserData = userData;    /* TVT */

    /* Lets see if this is a GIF file: */
    if (READ(GifFile, Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        free(Private);
        free(GifFile);
        return NULL;
    }

    /* The GIF Version number is ignored at this time. Maybe we should do
     * something more useful with it. */
    Buf[GIF_STAMP_LEN] = 0;
    if (memcmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
        free(Private);
        free(GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        free(Private);
        free(GifFile);
        return NULL;
    }

    return GifFile;
}

/******************************************************************************
 * This routine should be called last, to close the GIF file.
 *****************************************************************************/
int
DGifCloseFile(GifFileType * GifFile) {

    GifFilePrivateType *Private;

    if (GifFile == NULL)
        return GIF_ERROR;

    Private = GifFile->Private;

    if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SColorMap) {
        FreeMapObject(GifFile->SColorMap);
        GifFile->SColorMap = NULL;
    }

    free(Private);
    Private = NULL;

    if (GifFile->SavedImages) {
        FreeSavedImages(GifFile);
        GifFile->SavedImages = NULL;
    }

    FreeExtension(&GifFile->Extensions);

    free(GifFile);

    return GIF_OK;
}
