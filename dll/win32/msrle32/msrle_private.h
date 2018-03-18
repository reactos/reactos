/*
 * Copyright 2002 Michael Günnewig
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

#ifndef __MSRLE32_PRIVATE_H
#define __MSRLE32_PRIVATE_H

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"

#define IDS_NAME        100
#define IDS_DESCRIPTION 101
#define IDS_ABOUT       102

#define MSRLE32_DEFAULTQUALITY (85 * ICQUALITY_HIGH) / 100

#define FOURCC_RLE   mmioFOURCC('R','L','E',' ')
#define FOURCC_RLE4  mmioFOURCC('R','L','E','4')
#define FOURCC_RLE8  mmioFOURCC('R','L','E','8')
#define FOURCC_MRLE  mmioFOURCC('M','R','L','E')

#define WIDTHBYTES(i)     ((WORD)((i+31u)&(~31u))/8u) /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) WIDTHBYTES((WORD)(bi).biWidth * (WORD)(bi).biBitCount)

typedef struct _CodecInfo {
  FOURCC  fccHandler;

  BOOL    bCompress;
  LONG    nPrevFrame;
  LPWORD  pPrevFrame;
  LPWORD  pCurFrame;

  BOOL    bDecompress;
  LPBYTE  palette_map;
} CodecInfo;

typedef const BITMAPINFOHEADER * LPCBITMAPINFOHEADER;

#endif
