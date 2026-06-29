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
 * In order to make life a little bit easier when using the GIF file format,
 * this library was written, and which does all the dirty work...
 *
 *                                        Written by Gershon Elber,  Jun. 1989
 *                                        Hacks by Eric S. Raymond,  Sep. 1992
 ******************************************************************************
 * History:
 * 14 Jun 89 - Version 1.0 by Gershon Elber.
 *  3 Sep 90 - Version 1.1 by Gershon Elber (Support for Gif89, Unique names)
 * 15 Sep 90 - Version 2.0 by Eric S. Raymond (Changes to support GIF slurp)
 * 26 Jun 96 - Version 3.0 by Eric S. Raymond (Full GIF89 support)
 * 17 Dec 98 - Version 4.0 by Toshio Kuratomi (Fix extension writing code)
 *****************************************************************************/

#ifndef _UNGIF_H_
#define _UNGIF_H_ 1

#define GIF_ERROR   0
#define GIF_OK      1

#ifndef TRUE
#define TRUE        1
#endif /* TRUE */
#ifndef FALSE
#define FALSE       0
#endif /* FALSE */

#ifndef NULL
#define NULL        0
#endif /* NULL */

#define GIF_STAMP "GIFVER"          /* First chars in file - GIF stamp.  */
#define GIF_STAMP_LEN sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS 3           /* Version first character in stamp. */
#define GIF87_STAMP "GIF87a"        /* First chars in file - GIF stamp.  */
#define GIF89_STAMP "GIF89a"        /* First chars in file - GIF stamp.  */

#define GIF_FILE_BUFFER_SIZE 16384  /* Files uses bigger buffers than usual. */

typedef int GifBooleanType;
typedef unsigned char GifPixelType;
typedef unsigned char *GifRowType;
typedef unsigned char GifByteType;
typedef unsigned int GifPrefixType;
typedef int GifWord;

typedef struct GifColorType {
    GifByteType Red, Green, Blue;
} GifColorType;

typedef struct ColorMapObject {
    int ColorCount;
    int BitsPerPixel;
    int SortFlag;
    GifColorType *Colors;
} ColorMapObject;

typedef struct GifImageDesc {
    GifWord Left, Top, Width, Height,   /* Current image dimensions. */
      Interlace;                    /* Sequential/Interlaced lines. */
    ColorMapObject *ColorMap;       /* The local color map */
} GifImageDesc;

/* This is the in-core version of an extension record */
typedef struct {
    int Function;   /* Holds the type of the Extension block. */
    int ByteCount;
    char *Bytes;
} ExtensionBlock;

typedef struct {
    int Function;   /* DEPRECATED: Use ExtensionBlocks[x].Function instead */
    int ExtensionBlockCount;
    ExtensionBlock *ExtensionBlocks;
} Extensions;

typedef struct GifFileType {
    GifWord SWidth, SHeight,        /* Screen dimensions. */
      SColorResolution,         /* How many colors can we generate? */
      SColorTableSize,          /* Calculated color table size, even if not present */
      SBackGroundColor,         /* I hope you understand this one... */
      SAspectRatio;             /* Pixel aspect ratio, in 1/64 units, starting at 1:4. */
    ColorMapObject *SColorMap;  /* NULL if not exists. */
    Extensions Extensions;
    int ImageCount;             /* Number of current image */
    GifImageDesc Image;         /* Block describing current image */
    struct SavedImage *SavedImages; /* Use this to accumulate file state */
    void *UserData;             /* hook to attach user data (TVT) */
    void *Private;              /* Don't mess with this! */
} GifFileType;

typedef enum {
    UNDEFINED_RECORD_TYPE,
    SCREEN_DESC_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE, /* Begin with ',' */
    EXTENSION_RECORD_TYPE,  /* Begin with '!' */
    TERMINATE_RECORD_TYPE   /* Begin with ';' */
} GifRecordType;

/* func type to read gif data from arbitrary sources (TVT) */
typedef int (*InputFunc) (GifFileType *, GifByteType *, int);

/*  GIF89 extension function codes */

#define COMMENT_EXT_FUNC_CODE     0xfe    /* comment */
#define GRAPHICS_EXT_FUNC_CODE    0xf9    /* graphics control */
#define PLAINTEXT_EXT_FUNC_CODE   0x01    /* plaintext */
#define APPLICATION_EXT_FUNC_CODE 0xff    /* application block */

/* public interface to ungif.c */
int DGifSlurp(GifFileType * GifFile);
GifFileType *DGifOpen(void *userPtr, InputFunc readFunc);
int DGifCloseFile(GifFileType * GifFile);

#define D_GIF_ERR_OPEN_FAILED    101    /* And DGif possible errors. */
#define D_GIF_ERR_READ_FAILED    102
#define D_GIF_ERR_NOT_GIF_FILE   103
#define D_GIF_ERR_NO_SCRN_DSCR   104
#define D_GIF_ERR_NO_IMAG_DSCR   105
#define D_GIF_ERR_NO_COLOR_MAP   106
#define D_GIF_ERR_WRONG_RECORD   107
#define D_GIF_ERR_DATA_TOO_BIG   108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED   110
#define D_GIF_ERR_NOT_READABLE   111
#define D_GIF_ERR_IMAGE_DEFECT   112
#define D_GIF_ERR_EOF_TOO_SOON   113

/******************************************************************************
 * Support for the in-core structures allocation (slurp mode).
 *****************************************************************************/

/* This holds an image header, its unpacked raster bits, and extensions */
typedef struct SavedImage {
    GifImageDesc ImageDesc;
    unsigned char *RasterBits;
    Extensions Extensions;
} SavedImage;

#endif /* _UNGIF_H_ */
