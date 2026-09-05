/*
 * Copyright 2002-2003 Michael Günnewig
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

/* TODO:
 *   - some improvements possible
 *   - implement DecompressSetPalette? -- do we need it for anything?
 */

#include <assert.h>

#include "msrle_private.h"

#include "winnls.h"
#include "winuser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msrle32);

static HINSTANCE MSRLE32_hModule = 0;

#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

static inline WORD ColorCmp(WORD clr1, WORD clr2)
{
  UINT a = clr1 - clr2;
  return a * a;
}
static inline WORD Intensity(RGBQUAD clr)
{
  return (30 * clr.rgbRed + 59 * clr.rgbGreen + 11 * clr.rgbBlue)/4;
}

#define GetRawPixel(lpbi,lp,x) \
  ((lpbi)->biBitCount == 1 ? ((lp)[(x)/8] >> (8 - (x)%8)) & 1 : \
   ((lpbi)->biBitCount == 4 ? ((lp)[(x)/2] >> (4 * (1 - (x)%2))) & 15 : lp[x]))

/*****************************************************************************/

/* utility functions */
static BOOL    isSupportedDIB(LPCBITMAPINFOHEADER lpbi);
static BOOL    isSupportedMRLE(LPCBITMAPINFOHEADER lpbi);
static BYTE    MSRLE32_GetNearestPaletteIndex(UINT count, const RGBQUAD *clrs, RGBQUAD clr);

/* compression functions */
static void    computeInternalFrame(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn, const BYTE *lpIn);
static LONG    MSRLE32_GetMaxCompressedSize(LPCBITMAPINFOHEADER lpbi);
static LRESULT MSRLE32_CompressRLE4(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
                                    const BYTE *lpIn, LPBITMAPINFOHEADER lpbiOut,
                                    LPBYTE lpOut, BOOL isKey);
static LRESULT MSRLE32_CompressRLE8(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
                                    const BYTE *lpIn, LPBITMAPINFOHEADER lpbiOut,
                                    LPBYTE lpOut, BOOL isKey);

/* decompression functions */
static LRESULT MSRLE32_DecompressRLE4(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbi,
				      const BYTE *lpIn, LPBYTE lpOut);
static LRESULT MSRLE32_DecompressRLE8(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbi,
				      const BYTE *lpIn, LPBYTE lpOut);

/* API functions */
static LRESULT CompressGetFormat(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				 LPBITMAPINFOHEADER lpbiOut);
static LRESULT CompressGetSize(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut);
static LRESULT CompressQuery(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			     LPCBITMAPINFOHEADER lpbiOut);
static LRESULT CompressBegin(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			     LPCBITMAPINFOHEADER lpbiOut);
static LRESULT Compress(CodecInfo *pi, ICCOMPRESS* lpic, DWORD dwSize);
static LRESULT CompressEnd(CodecInfo *pi);

static LRESULT DecompressGetFormat(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				   LPBITMAPINFOHEADER lpbiOut);
static LRESULT DecompressQuery(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut);
static LRESULT DecompressBegin(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut);
static LRESULT Decompress(CodecInfo *pi, ICDECOMPRESS *pic, DWORD dwSize);
static LRESULT DecompressEnd(CodecInfo *pi);
static LRESULT DecompressGetPalette(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				    LPBITMAPINFOHEADER lpbiOut);

/*****************************************************************************/

static BOOL isSupportedMRLE(LPCBITMAPINFOHEADER lpbi)
{
  /* pre-conditions */
  assert(lpbi != NULL);

  if (lpbi->biSize < sizeof(BITMAPINFOHEADER) ||
      lpbi->biPlanes != 1)
    return FALSE;

  if (lpbi->biCompression == BI_RLE4) {
    if (lpbi->biBitCount != 4 ||
	(lpbi->biWidth % 2) != 0)
      return FALSE;
  } else if (lpbi->biCompression == BI_RLE8) {
    if (lpbi->biBitCount != 8)
      return FALSE;
  } else
    return FALSE;

  return TRUE;
}

static BOOL  isSupportedDIB(LPCBITMAPINFOHEADER lpbi)
{
  /* pre-conditions */
  assert(lpbi != NULL);

  /* check structure version/planes/compression */
  if (lpbi->biSize < sizeof(BITMAPINFOHEADER) ||
      lpbi->biPlanes != 1)
    return FALSE;
  if (lpbi->biCompression != BI_RGB &&
      lpbi->biCompression != BI_BITFIELDS)
    return FALSE;

  /* check bit-depth */
  if (lpbi->biBitCount != 1 &&
      lpbi->biBitCount != 4 &&
      lpbi->biBitCount != 8 &&
      lpbi->biBitCount != 15 &&
      lpbi->biBitCount != 16 &&
      lpbi->biBitCount != 24 &&
      lpbi->biBitCount != 32)
    return FALSE;

  /* check for size(s) */
  if (!lpbi->biWidth || !lpbi->biHeight)
    return FALSE; /* image with zero size, makes no sense so error ! */
  if (DIBWIDTHBYTES(*lpbi) * (DWORD)lpbi->biHeight >= (1UL << 31) - 1)
    return FALSE; /* image too big ! */

  /* check for nonexistent colortable for hi- and true-color DIB's */
  if (lpbi->biBitCount >= 15 && lpbi->biClrUsed > 0)
    return FALSE;

  return TRUE;
}

static BYTE MSRLE32_GetNearestPaletteIndex(UINT count, const RGBQUAD *clrs, RGBQUAD clr)
{
  INT  diff = 0x00FFFFFF;
  UINT i;
  UINT idx = 0;

  /* pre-conditions */
  assert(clrs != NULL);

  for (i = 0; i < count; i++) {
    int r = ((int)clrs[i].rgbRed   - (int)clr.rgbRed);
    int g = ((int)clrs[i].rgbGreen - (int)clr.rgbGreen);
    int b = ((int)clrs[i].rgbBlue  - (int)clr.rgbBlue);

    r = r*r + g*g + b*b;

    if (r < diff) {
      idx  = i;
      diff = r;
      if (diff == 0)
	break;
    }
  }

  return idx;
}

/*****************************************************************************/

void computeInternalFrame(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn, const BYTE *lpIn)
{
  WORD   wIntensityTbl[256];
  DWORD  lInLine, lOutLine;
  LPWORD lpOut;
  UINT   i;
  LONG   y;

  /* pre-conditions */
  assert(pi != NULL && lpbiIn != NULL && lpIn != NULL);
  assert(pi->pCurFrame != NULL);

  lInLine  = DIBWIDTHBYTES(*lpbiIn);
  lOutLine = WIDTHBYTES((WORD)lpbiIn->biWidth * 8u * sizeof(WORD)) / 2u;
  lpOut    = pi->pCurFrame;

  assert(lpbiIn->biClrUsed != 0);

  {
    const RGBQUAD *lp =
      (const RGBQUAD *)((const BYTE*)lpbiIn + lpbiIn->biSize);

    for (i = 0; i < lpbiIn->biClrUsed; i++)
      wIntensityTbl[i] = Intensity(lp[i]);
  }

  for (y = 0; y < lpbiIn->biHeight; y++) {
    LONG x;

    switch (lpbiIn->biBitCount) {
    case 1:
      for (x = 0; x < lpbiIn->biWidth / 8; x++) {
	for (i = 0; i < 7; i++)
	  lpOut[8 * x + i] = wIntensityTbl[(lpIn[x] >> (7 - i)) & 1];
      }
      break;
    case 4:
      for (x = 0; x < lpbiIn->biWidth / 2; x++) {
	lpOut[2 * x + 0] = wIntensityTbl[(lpIn[x] >> 4)];
	lpOut[2 * x + 1] = wIntensityTbl[(lpIn[x] & 0x0F)];
      }
      break;
    case 8:
      for (x = 0; x < lpbiIn->biWidth; x++)
	lpOut[x] = wIntensityTbl[lpIn[x]];
      break;
    }

    lpIn  += lInLine;
    lpOut += lOutLine;
  }
}

static LONG MSRLE32_GetMaxCompressedSize(LPCBITMAPINFOHEADER lpbi)
{
  LONG a, b, size;

  /* pre-condition */
  assert(lpbi != NULL);

  a = lpbi->biWidth / 255;
  b = lpbi->biWidth % 255;
  if (lpbi->biBitCount <= 4) {
    a /= 2;
    b /= 2;
  }

  size = (2 + a * (2 + ((a + 2) & ~2)) + b * (2 + ((b + 2) & ~2)));
  return size * lpbi->biHeight + 2;
}

/* lpP => current  pos in previous frame
 * lpA => previous pos in current  frame
 * lpB => current  pos in current  frame
 */
static INT countDiffRLE4(const WORD *lpP, const WORD *lpA, const WORD *lpB, INT pos, LONG lDist, LONG width)
{
  INT  count;
  WORD clr1, clr2;

  /* pre-conditions */
  assert(lpA && lpB && lDist >= 0 && width > 0);

  if (pos >= width)
    return 0;
  if (pos+1 == width)
    return 1;

  clr1 = lpB[pos++];
  clr2 = lpB[pos];

  count = 2;
  while (pos + 1 < width) {
    WORD clr3, clr4;

    clr3 = lpB[++pos];
    if (pos + 1 >= width)
      return count + 1;

    clr4 = lpB[++pos];
    if (ColorCmp(clr1, clr3) <= lDist &&
	ColorCmp(clr2, clr4) <= lDist) {
      /* diff at end? -- look-ahead for at least ?? more encodable pixels */
      if (pos + 2 < width && ColorCmp(clr1,lpB[pos+1]) <= lDist &&
	  ColorCmp(clr2,lpB[pos+2]) <= lDist) {
	if (pos + 4 < width && ColorCmp(lpB[pos+1],lpB[pos+3]) <= lDist &&
	    ColorCmp(lpB[pos+2],lpB[pos+4]) <= lDist)
	  return count - 3; /* followed by at least 4 encodable pixels */
	return count - 2;
      }
    } else if (lpP != NULL && ColorCmp(lpP[pos], lpB[pos]) <= lDist) {
      /* 'compare' with previous frame for end of diff */
      INT count2 = 0;

      /* FIXME */

      if (count2 >= 8)
	return count;

      pos -= count2;
    }

    count += 2;
    clr1 = clr3;
    clr2 = clr4;
  }

  return count;
}

/* lpP => current  pos in previous frame
 * lpA => previous pos in current  frame
 * lpB => current  pos in current  frame
 */
static INT countDiffRLE8(const WORD *lpP, const WORD *lpA, const WORD *lpB, INT pos, LONG lDist, LONG width)
{
  INT count;

  for (count = 0; pos < width; pos++, count++) {
    if (ColorCmp(lpA[pos], lpB[pos]) <= lDist) {
      /* diff at end? -- look-ahead for some more encodable pixel */
      if (pos + 1 < width && ColorCmp(lpB[pos], lpB[pos+1]) <= lDist)
	return count - 1;
      if (pos + 2 < width && ColorCmp(lpB[pos+1], lpB[pos+2]) <= lDist)
	return count - 1;
    } else if (lpP != NULL && ColorCmp(lpP[pos], lpB[pos]) <= lDist) {
      /* 'compare' with previous frame for end of diff */
      INT count2 = 0;

      for (count2 = 0, pos++; pos < width && count2 <= 5; pos++, count2++) {
	if (ColorCmp(lpP[pos], lpB[pos]) > lDist)
	  break;
      }
      if (count2 > 4)
	return count;

      pos -= count2;
    }
  }

  return count;
}

static INT MSRLE32_CompressRLE4Line(const CodecInfo *pi, const WORD *lpP,
                                    const WORD *lpC, LPCBITMAPINFOHEADER lpbi,
                                    const BYTE *lpIn, LONG lDist,
                                    INT x, LPBYTE *ppOut,
                                    DWORD *lpSizeImage)
{
  LPBYTE lpOut = *ppOut;
  INT    count, pos;
  WORD   clr1, clr2;

  /* try to encode as many pixel as possible */
  count = 1;
  pos   = x;
  clr1  = lpC[pos++];
  if (pos < lpbi->biWidth) {
    clr2 = lpC[pos];
    for (++count; pos + 1 < lpbi->biWidth; ) {
      ++pos;
      if (ColorCmp(clr1, lpC[pos]) > lDist)
	break;
      count++;
      if (pos + 1 >= lpbi->biWidth)
	break;
      ++pos;
      if (ColorCmp(clr2, lpC[pos]) > lDist)
	break;
      count++;
    }
  }

  if (count < 4) {
    /* add some pixel for absoluting if possible */
    count += countDiffRLE4(lpP, lpC - 1, lpC, pos-1, lDist, lpbi->biWidth);

    assert(count > 0);

    /* check for near end of line */
    if (x + count > lpbi->biWidth)
      count = lpbi->biWidth - x;

    /* absolute pixel(s) in groups of at least 3 and at most 254 pixels */
    while (count > 2) {
      INT  i;
      INT  size       = min(count, 254);
      int  bytes      = ((size + 1) & (~1)) / 2;
      int  extra_byte = bytes & 0x01;

      *lpSizeImage += 2 + bytes + extra_byte;
      assert(((*lpSizeImage) % 2) == 0);
      count -= size;
      *lpOut++ = 0;
      *lpOut++ = size;
      for (i = 0; i < size; i += 2) {
	clr1 = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
	x++;
	if (i + 1 < size) {
	  clr2 = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
	  x++;
	} else
	  clr2 = 0;

	*lpOut++ = (clr1 << 4) | clr2;
      }
      if (extra_byte)
	*lpOut++ = 0;
    }

    if (count > 0) {
      /* too little for absoluting so we must encode them */
      assert(count <= 2);

      *lpSizeImage += 2;
      clr1 = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
      x++;
      if (count == 2) {
	clr2 = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
	x++;
      } else
	clr2 = 0;
      *lpOut++ = count;
      *lpOut++ = (clr1 << 4) | clr2;
    }
  } else {
    /* encode count pixel(s) */
    clr1 = ((pi->palette_map[GetRawPixel(lpbi,lpIn,x)] << 4) |
	    pi->palette_map[GetRawPixel(lpbi,lpIn,x + 1)]);

    x += count;
    while (count > 0) {
      INT size = min(count, 254);

      *lpSizeImage += 2;
      count    -= size;
      *lpOut++  = size;
      *lpOut++  = clr1;
    }
  }

  *ppOut = lpOut;

  return x;
}

static INT MSRLE32_CompressRLE8Line(const CodecInfo *pi, const WORD *lpP,
                                    const WORD *lpC, LPCBITMAPINFOHEADER lpbi,
                                    const BYTE *lpIn, INT x, LPBYTE *ppOut,
                                    DWORD *lpSizeImage)
{
  LPBYTE lpOut = *ppOut;
  INT    count, pos;
  WORD   clr;

  assert(lpbi->biBitCount <= 8);
  assert(lpbi->biCompression == BI_RGB);

  /* try to encode as much as possible */
  pos = x;
  clr = lpC[pos++];
  for (count = 1; pos < lpbi->biWidth; count++) {
    if (ColorCmp(clr, lpC[pos++]) > 0)
      break;
  }

  if (count < 2) {
    /* add some more pixels for absoluting if possible */
    count += countDiffRLE8(lpP, lpC - 1, lpC, pos-1, 0, lpbi->biWidth);

    assert(count > 0);

    /* check for over end of line */
    if (x + count > lpbi->biWidth)
      count = lpbi->biWidth - x;

    /* absolute pixel(s) in groups of at least 3 and at most 255 pixels */
    while (count > 2) {
      INT  i;
      INT  size       = min(count, 255);
      int  extra_byte = size % 2;

      *lpSizeImage += 2 + size + extra_byte;
      count -= size;
      *lpOut++ = 0;
      *lpOut++ = size;
      for (i = 0; i < size; i++) {
	*lpOut++ = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
	x++;
      }
      if (extra_byte)
	*lpOut++ = 0;
    }
    if (count > 0) {
      /* too little for absoluting so we must encode them even if it's expensive! */
      assert(count <= 2);

      *lpSizeImage += 2 * count;
      *lpOut++ = 1;
      *lpOut++ = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
      x++;

      if (count == 2) {
	*lpOut++ = 1;
	*lpOut++ = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];
	x++;
      }
    }
  } else {
    /* encode count pixel(s) */
    clr = pi->palette_map[GetRawPixel(lpbi,lpIn,x)];

    /* optimize end of line */
    if (x + count + 1 == lpbi->biWidth)
      count++;

    x += count;
    while (count > 0) {
      INT size = min(count, 255);

      *lpSizeImage += 2;
      count    -= size;
      *lpOut++  = size;
      *lpOut++  = clr;
    }
  }

  *ppOut = lpOut;

  return x;
}

LRESULT MSRLE32_CompressRLE4(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
                             const BYTE *lpIn, LPBITMAPINFOHEADER lpbiOut,
                             LPBYTE lpOut, BOOL isKey)
{
  LPWORD lpC;
  LONG   lLine, lInLine;
  LPBYTE lpOutStart = lpOut;

  /* pre-conditions */
  assert(pi != NULL && lpbiOut != NULL);
  assert(lpIn != NULL && lpOut != NULL);
  assert(pi->pCurFrame != NULL);

  lpC      = pi->pCurFrame;
  lInLine  = DIBWIDTHBYTES(*lpbiIn);
  lLine    = WIDTHBYTES(lpbiOut->biWidth * 16) / 2;

  lpbiOut->biSizeImage = 0;
  if (isKey) {
    /* keyframe -- convert internal frame to output format */
    INT x, y;

    for (y = 0; y < lpbiOut->biHeight; y++) {
      x = 0;

      do {
	x = MSRLE32_CompressRLE4Line(pi, NULL, lpC, lpbiIn, lpIn, 0, x,
				     &lpOut, &lpbiOut->biSizeImage);
      } while (x < lpbiOut->biWidth);

      lpC   += lLine;
      lpIn  += lInLine;

      /* add EOL -- end of line */
      lpbiOut->biSizeImage += 2;
      *(LPWORD)lpOut = 0;
      lpOut += sizeof(WORD);
      assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
    }
  } else {
    /* delta-frame -- compute delta between last and this internal frame */
    LPWORD lpP;
    INT    x, y;
    INT    jumpx, jumpy;

    assert(pi->pPrevFrame != NULL);

    lpP   = pi->pPrevFrame;
    jumpy = 0;
    jumpx = -1;

    for (y = 0; y < lpbiOut->biHeight; y++) {
      x = 0;

      do {
	INT count, pos;

	if (jumpx == -1)
	  jumpx = x;
	for (count = 0, pos = x; pos < lpbiOut->biWidth; pos++, count++) {
	  if (ColorCmp(lpP[pos], lpC[pos]) > 0)
	    break;
	}

	if (pos == lpbiOut->biWidth && count > 8) {
	  /* (count > 8) secures that we will save space */
	  jumpy++;
	  break;
	} else if (jumpy || jumpx != pos) {
	  /* time to jump */
	  assert(jumpx != -1);

	  if (pos < jumpx) {
	    /* can only jump in positive direction -- jump until EOL, EOL */
	    INT w = lpbiOut->biWidth - jumpx;

	    assert(jumpy > 0);
	    assert(w >= 4);

	    jumpx = 0;
	    jumpy--;
	    /* if (w % 255 == 2) then equal costs
	     * else if (w % 255 < 4 && we could encode all) then 2 bytes too expensive
	     * else it will be cheaper
	     */
	    while (w > 0) {
	      lpbiOut->biSizeImage += 4;
	      *lpOut++ = 0;
	      *lpOut++ = 2;
	      *lpOut   = min(w, 255);
	      w       -= *lpOut++;
	      *lpOut++ = 0;
	    }
	    /* add EOL -- end of line */
	    lpbiOut->biSizeImage += 2;
	    *((LPWORD)lpOut) = 0;
	    lpOut += sizeof(WORD);
	  }

	  /* FIXME: if (jumpy == 0 && could encode all) then jump too expensive */

	  /* write out real jump(s) */
	  while (jumpy || pos != jumpx) {
	    lpbiOut->biSizeImage += 4;
	    *lpOut++ = 0;
	    *lpOut++ = 2;
	    *lpOut   = min(pos - jumpx, 255);
	    x       += *lpOut;
	    jumpx   += *lpOut++;
	    *lpOut   = min(jumpy, 255);
	    jumpy   -= *lpOut++;
	  }

	  jumpy = 0;
	}

	jumpx = -1;

	if (x < lpbiOut->biWidth) {
	  /* skipped the 'same' things corresponding to previous frame */
	  x = MSRLE32_CompressRLE4Line(pi, lpP, lpC, lpbiIn, lpIn, 0, x,
			       &lpOut, &lpbiOut->biSizeImage);
	}
      } while (x < lpbiOut->biWidth);

      lpP   += lLine;
      lpC   += lLine;
      lpIn  += lInLine;

      if (jumpy == 0) {
	assert(jumpx == -1);

	/* add EOL -- end of line */
	lpbiOut->biSizeImage += 2;
	*((LPWORD)lpOut) = 0;
        lpOut += sizeof(WORD);
	assert(lpOut == lpOutStart + lpbiOut->biSizeImage);
      }
    }

    /* add EOL -- will be changed to EOI */
    lpbiOut->biSizeImage += 2;
    *((LPWORD)lpOut) = 0;
    lpOut += sizeof(WORD);
  }

  /* change EOL to EOI -- end of image */
  lpOut[-1] = 1;
  assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));

  return ICERR_OK;
}

LRESULT MSRLE32_CompressRLE8(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
                             const BYTE *lpIn, LPBITMAPINFOHEADER lpbiOut,
                             LPBYTE lpOut, BOOL isKey)
{
  LPWORD lpC;
  LONG   lInLine, lLine;
  LPBYTE lpOutStart = lpOut;

  assert(pi != NULL && lpbiOut != NULL);
  assert(lpIn != NULL && lpOut != NULL);
  assert(pi->pCurFrame != NULL);

  lpC     = pi->pCurFrame;
  lInLine = DIBWIDTHBYTES(*lpbiIn);
  lLine   = WIDTHBYTES(lpbiOut->biWidth * 16) / 2;

  lpbiOut->biSizeImage = 0;
  if (isKey) {
    /* keyframe -- convert internal frame to output format */
    INT x, y;

    for (y = 0; y < lpbiOut->biHeight; y++) {
      x = 0;

      do {
	x = MSRLE32_CompressRLE8Line(pi, NULL, lpC, lpbiIn, lpIn, x,
			     &lpOut, &lpbiOut->biSizeImage);
	assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
      } while (x < lpbiOut->biWidth);

      lpC  += lLine;
      lpIn += lInLine;

      /* add EOL -- end of line */
      lpbiOut->biSizeImage += 2;
      *((LPWORD)lpOut) = 0;
      lpOut += sizeof(WORD);
      assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
    }
  } else {
    /* delta-frame -- compute delta between last and this internal frame */
    LPWORD lpP;
    INT    x, y;
    INT    jumpx, jumpy;

    assert(pi->pPrevFrame != NULL);

    lpP   = pi->pPrevFrame;
    jumpx = -1;
    jumpy = 0;

    for (y = 0; y < lpbiOut->biHeight; y++) {
      x = 0;

      do {
	INT count, pos;

	if (jumpx == -1)
	  jumpx = x;
	for (count = 0, pos = x; pos < lpbiOut->biWidth; pos++, count++) {
	  if (ColorCmp(lpP[pos], lpC[pos]) > 0)
	    break;
	}

	if (pos == lpbiOut->biWidth && count > 4) {
	  /* (count > 4) secures that we will save space */
	  jumpy++;
	  break;
	} else if (jumpy || jumpx != pos) {
	  /* time to jump */
	  assert(jumpx != -1);

	  if (pos < jumpx) {
	    /* can only jump in positive direction -- do an EOL then jump */
	    assert(jumpy > 0);

	    jumpx = 0;
	    jumpy--;

	    /* add EOL -- end of line */
	    lpbiOut->biSizeImage += 2;
	    *((LPWORD)lpOut) = 0;
	    lpOut += sizeof(WORD);
	    assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
	  }

	  /* FIXME: if (jumpy == 0 && could encode all) then jump too expensive */

	  /* write out real jump(s) */
	  while (jumpy || pos != jumpx) {
	    lpbiOut->biSizeImage += 4;
	    *lpOut++ = 0;
	    *lpOut++ = 2;
	    *lpOut   = min(pos - jumpx, 255);
	    jumpx   += *lpOut++;
	    *lpOut   = min(jumpy, 255);
	    jumpy   -= *lpOut++;
	  }
	  x = pos;

	  jumpy = 0;
	}

	jumpx = -1;

	if (x < lpbiOut->biWidth) {
	  /* skip the 'same' things corresponding to previous frame */
          x = MSRLE32_CompressRLE8Line(pi, lpP, lpC, lpbiIn, lpIn, x,
			       &lpOut, &lpbiOut->biSizeImage);
	  assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
	}
      } while (x < lpbiOut->biWidth);

      lpP  += lLine;
      lpC  += lLine;
      lpIn += lInLine;

      if (jumpy == 0) {
	/* add EOL -- end of line */
	lpbiOut->biSizeImage += 2;
	*((LPWORD)lpOut) = 0;
	lpOut += sizeof(WORD);
	assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));
      }
    }

    /* add EOL */
    lpbiOut->biSizeImage += 2;
    *((LPWORD)lpOut) = 0;
    lpOut += sizeof(WORD);
  }

  /* add EOI -- end of image */
  lpbiOut->biSizeImage += 2;
  *lpOut++ = 0;
  *lpOut++ = 1;
  assert(lpOut == (lpOutStart + lpbiOut->biSizeImage));

  return ICERR_OK;
}

/*****************************************************************************/

static LRESULT MSRLE32_DecompressRLE4(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbi,
				      const BYTE *lpIn, LPBYTE lpOut)
{
  int  bytes_per_pixel;
  int  line_size;
  int  pixel_ptr  = 0;
  int  i;
  BOOL bEndFlag   = FALSE;

  assert(pi != NULL);
  assert(lpbi != NULL && lpbi->biCompression == BI_RGB);
  assert(lpIn != NULL && lpOut != NULL);

  bytes_per_pixel = (lpbi->biBitCount + 1) / 8;
  line_size       = DIBWIDTHBYTES(*lpbi);

  do {
    BYTE code0, code1;

    code0 = *lpIn++;
    code1 = *lpIn++;

    if (code0 == 0) {
      int  extra_byte;

      switch (code1) {
      case  0: /* EOL - end of line  */
	pixel_ptr = 0;
	lpOut += line_size;
	break;
      case  1: /* EOI - end of image */
	bEndFlag = TRUE;
	break;
      case  2: /* skip */
	pixel_ptr += *lpIn++ * bytes_per_pixel;
	lpOut     += *lpIn++ * line_size;
	if (pixel_ptr >= lpbi->biWidth * bytes_per_pixel) {
	  pixel_ptr = 0;
	  lpOut    += line_size;
	}
	break;
      default: /* absolute mode */
	extra_byte = (((code1 + 1) & (~1)) / 2) & 0x01;

	if (pixel_ptr/bytes_per_pixel + code1 > lpbi->biWidth)
	  return ICERR_ERROR;

	code0 = code1;
	for (i = 0; i < code0 / 2; i++) {
	  if (bytes_per_pixel == 1) {
	    code1 = lpIn[i];
	    lpOut[pixel_ptr++] = pi->palette_map[(code1 >> 4)];
	    if (2 * i + 1 <= code0)
	      lpOut[pixel_ptr++] = pi->palette_map[(code1 & 0x0F)];
	  } else if (bytes_per_pixel == 2) {
	    code1 = lpIn[i] >> 4;
	    lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 0];
	    lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 1];

	    if (2 * i + 1 <= code0) {
	      code1 = lpIn[i] & 0x0F;
	      lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 0];
	      lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 1];
	    }
	  } else {
	    code1 = lpIn[i] >> 4;
	    lpOut[pixel_ptr + 0] = pi->palette_map[code1 * 4 + 0];
	    lpOut[pixel_ptr + 1] = pi->palette_map[code1 * 4 + 1];
	    lpOut[pixel_ptr + 2] = pi->palette_map[code1 * 4 + 2];
	    pixel_ptr += bytes_per_pixel;

	    if (2 * i + 1 <= code0) {
	      code1 = lpIn[i] & 0x0F;
	      lpOut[pixel_ptr + 0] = pi->palette_map[code1 * 4 + 0];
	      lpOut[pixel_ptr + 1] = pi->palette_map[code1 * 4 + 1];
	      lpOut[pixel_ptr + 2] = pi->palette_map[code1 * 4 + 2];
	      pixel_ptr += bytes_per_pixel;
	    }
	  }
	}
	if (code0 & 0x01) {
	  if (bytes_per_pixel == 1) {
	    code1 = lpIn[i];
	    lpOut[pixel_ptr++] = pi->palette_map[(code1 >> 4)];
	  } else if (bytes_per_pixel == 2) {
	    code1 = lpIn[i] >> 4;
	    lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 0];
	    lpOut[pixel_ptr++] = pi->palette_map[code1 * 2 + 1];
	  } else {
	    code1 = lpIn[i] >> 4;
	    lpOut[pixel_ptr + 0] = pi->palette_map[code1 * 4 + 0];
	    lpOut[pixel_ptr + 1] = pi->palette_map[code1 * 4 + 1];
	    lpOut[pixel_ptr + 2] = pi->palette_map[code1 * 4 + 2];
	    pixel_ptr += bytes_per_pixel;
	  }
	  lpIn++;
	}
	lpIn += code0 / 2;

	/* if the RLE code is odd, skip a byte in the stream */
	if (extra_byte)
	  lpIn++;
      };
    } else {
      /* coded mode */
      if (pixel_ptr/bytes_per_pixel + code0 > lpbi->biWidth)
	return ICERR_ERROR;

      if (bytes_per_pixel == 1) {
	BYTE c1 = pi->palette_map[(code1 >> 4)];
	BYTE c2 = pi->palette_map[(code1 & 0x0F)];

	for (i = 0; i < code0; i++) {
	  if ((i & 1) == 0)
	    lpOut[pixel_ptr++] = c1;
	  else
	    lpOut[pixel_ptr++] = c2;
	}
      } else if (bytes_per_pixel == 2) {
	BYTE hi1 = pi->palette_map[(code1 >> 4) * 2 + 0];
	BYTE lo1 = pi->palette_map[(code1 >> 4) * 2 + 1];

	BYTE hi2 = pi->palette_map[(code1 & 0x0F) * 2 + 0];
	BYTE lo2 = pi->palette_map[(code1 & 0x0F) * 2 + 1];

	for (i = 0; i < code0; i++) {
	  if ((i & 1) == 0) {
	    lpOut[pixel_ptr++] = hi1;
	    lpOut[pixel_ptr++] = lo1;
	  } else {
	    lpOut[pixel_ptr++] = hi2;
	    lpOut[pixel_ptr++] = lo2;
	  }
	}
      } else {
	BYTE b1 = pi->palette_map[(code1 >> 4) * 4 + 0];
	BYTE g1 = pi->palette_map[(code1 >> 4) * 4 + 1];
	BYTE r1 = pi->palette_map[(code1 >> 4) * 4 + 2];

	BYTE b2 = pi->palette_map[(code1 & 0x0F) * 4 + 0];
	BYTE g2 = pi->palette_map[(code1 & 0x0F) * 4 + 1];
	BYTE r2 = pi->palette_map[(code1 & 0x0F) * 4 + 2];

	for (i = 0; i < code0; i++) {
	  if ((i & 1) == 0) {
	    lpOut[pixel_ptr + 0] = b1;
	    lpOut[pixel_ptr + 1] = g1;
	    lpOut[pixel_ptr + 2] = r1;
	  } else {
	    lpOut[pixel_ptr + 0] = b2;
	    lpOut[pixel_ptr + 1] = g2;
	    lpOut[pixel_ptr + 2] = r2;
	  }
	  pixel_ptr += bytes_per_pixel;
	}
      }
    }
  } while (! bEndFlag);

  return ICERR_OK;
}

static LRESULT MSRLE32_DecompressRLE8(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbi,
				      const BYTE *lpIn, LPBYTE lpOut)
{
  int  bytes_per_pixel;
  int  line_size;
  int  pixel_ptr  = 0;
  BOOL bEndFlag   = FALSE;

  assert(pi != NULL);
  assert(lpbi != NULL && lpbi->biCompression == BI_RGB);
  assert(lpIn != NULL && lpOut != NULL);

  bytes_per_pixel = (lpbi->biBitCount + 1) / 8;
  line_size       = DIBWIDTHBYTES(*lpbi);

  do {
    BYTE code0, code1;

    code0 = *lpIn++;
    code1 = *lpIn++;

    if (code0 == 0) {
      int  extra_byte;

      switch (code1) {
      case  0: /* EOL - end of line  */
	pixel_ptr = 0;
	lpOut += line_size;
	break;
      case  1: /* EOI - end of image */
	bEndFlag = TRUE;
	break;
      case  2: /* skip */
	pixel_ptr += *lpIn++ * bytes_per_pixel;
	lpOut     += *lpIn++ * line_size;
	if (pixel_ptr >= lpbi->biWidth * bytes_per_pixel) {
	  pixel_ptr = 0;
	  lpOut    += line_size;
	}
	break;
      default: /* absolute mode */
	if (pixel_ptr/bytes_per_pixel + code1 > lpbi->biWidth) {
          WARN("aborted absolute: (%d=%d/%d+%d) > %ld\n",pixel_ptr/bytes_per_pixel + code1,pixel_ptr,bytes_per_pixel,code1,lpbi->biWidth);
	  return ICERR_ERROR;
	}
	extra_byte = code1 & 0x01;

	code0 = code1;
	while (code0--) {
	  code1 = *lpIn++;
	  if (bytes_per_pixel == 1) {
	    lpOut[pixel_ptr] = pi->palette_map[code1];
	  } else if (bytes_per_pixel == 2) {
	    lpOut[pixel_ptr + 0] = pi->palette_map[code1 * 2 + 0];
	    lpOut[pixel_ptr + 1] = pi->palette_map[code1 * 2 + 1];
	  } else {
	    lpOut[pixel_ptr + 0] = pi->palette_map[code1 * 4 + 0];
	    lpOut[pixel_ptr + 1] = pi->palette_map[code1 * 4 + 1];
	    lpOut[pixel_ptr + 2] = pi->palette_map[code1 * 4 + 2];
	  }
	  pixel_ptr += bytes_per_pixel;
	}

	/* if the RLE code is odd, skip a byte in the stream */
	if (extra_byte)
	  lpIn++;
      };
    } else {
      /* coded mode */
      if (pixel_ptr/bytes_per_pixel + code0 > lpbi->biWidth) {
	WARN("aborted coded: (%d=%d/%d+%d) > %ld\n",pixel_ptr/bytes_per_pixel + code1,pixel_ptr,bytes_per_pixel,code1,lpbi->biWidth);
	return ICERR_ERROR;
      }

      if (bytes_per_pixel == 1) {
	code1 = pi->palette_map[code1];
	while (code0--)
	  lpOut[pixel_ptr++] = code1;
      } else if (bytes_per_pixel == 2) {
	BYTE hi = pi->palette_map[code1 * 2 + 0];
	BYTE lo = pi->palette_map[code1 * 2 + 1];

	while (code0--) {
	  lpOut[pixel_ptr + 0] = hi;
	  lpOut[pixel_ptr + 1] = lo;
	  pixel_ptr += bytes_per_pixel;
	}
      } else {
	BYTE r = pi->palette_map[code1 * 4 + 2];
	BYTE g = pi->palette_map[code1 * 4 + 1];
	BYTE b = pi->palette_map[code1 * 4 + 0];

	while (code0--) {
	  lpOut[pixel_ptr + 0] = b;
	  lpOut[pixel_ptr + 1] = g;
	  lpOut[pixel_ptr + 2] = r;
	  pixel_ptr += bytes_per_pixel;
	}
      }
    }
  } while (! bEndFlag);

  return ICERR_OK;
}

/*****************************************************************************/

static CodecInfo* Open(LPICOPEN icinfo)
{
  CodecInfo* pi = NULL;

  if (icinfo == NULL) {
    TRACE("(NULL)\n");
    return (LPVOID)0xFFFF0000;
  }

  if (compare_fourcc(icinfo->fccType, ICTYPE_VIDEO)) return NULL;

  TRACE("(%p = {%lu,0x%08lX(%4.4s),0x%08lX(%4.4s),0x%lX,0x%lX,...})\n", icinfo,
	icinfo->dwSize,	icinfo->fccType, (char*)&icinfo->fccType,
	icinfo->fccHandler, (char*)&icinfo->fccHandler,
	icinfo->dwVersion,icinfo->dwFlags);

  switch (icinfo->fccHandler) {
  case FOURCC_RLE:
  case FOURCC_RLE4:
  case FOURCC_RLE8:
  case FOURCC_MRLE:
    break;
  case mmioFOURCC('m','r','l','e'):
    icinfo->fccHandler = FOURCC_MRLE;
    break;
  default:
    WARN("unknown FOURCC = 0x%08lX(%4.4s) !\n",
	 icinfo->fccHandler,(char*)&icinfo->fccHandler);
    return NULL;
  }

  pi = LocalAlloc(LPTR, sizeof(CodecInfo));

  if (pi != NULL) {
    pi->fccHandler  = icinfo->fccHandler;

    pi->bCompress   = FALSE;
    pi->nPrevFrame  = -1;
    pi->pPrevFrame  = pi->pCurFrame = NULL;

    pi->bDecompress = FALSE;
    pi->palette_map = NULL;
  }

  icinfo->dwError = (pi != NULL ? ICERR_OK : ICERR_MEMORY);

  return pi;
}

static LRESULT Close(CodecInfo *pi)
{
  TRACE("(%p)\n", pi);

  /* pre-condition */
  assert(pi != NULL);

  if (pi->pPrevFrame != NULL || pi->pCurFrame != NULL)
    CompressEnd(pi);

  LocalFree(pi);
  return 1;
}

static LRESULT GetInfo(const CodecInfo *pi, ICINFO *icinfo, DWORD dwSize)
{
  /* pre-condition */
  assert(pi != NULL);

  /* check parameters */
  if (icinfo == NULL)
    return sizeof(ICINFO);
  if (dwSize < sizeof(ICINFO))
    return 0;

  icinfo->dwSize       = sizeof(ICINFO);
  icinfo->fccType      = ICTYPE_VIDEO;
  icinfo->fccHandler   = (pi != NULL ? pi->fccHandler : FOURCC_MRLE);
  icinfo->dwFlags      = VIDCF_QUALITY | VIDCF_TEMPORAL | VIDCF_CRUNCH;
  icinfo->dwVersion    = ICVERSION;
  icinfo->dwVersionICM = ICVERSION;

  LoadStringW(MSRLE32_hModule, IDS_NAME, icinfo->szName, ARRAY_SIZE(icinfo->szName));
  LoadStringW(MSRLE32_hModule, IDS_DESCRIPTION, icinfo->szDescription, ARRAY_SIZE(icinfo->szDescription));

  return sizeof(ICINFO);
}

static LRESULT Configure(const CodecInfo *pi, HWND hWnd)
{
  /* pre-condition */
  assert(pi != NULL);

  /* FIXME */
  return ICERR_OK;
}

static LRESULT About(CodecInfo *pi, HWND hWnd)
{
  WCHAR szTitle[20];
  WCHAR szAbout[128];

  /* pre-condition */
  assert(MSRLE32_hModule != 0);

  LoadStringW(MSRLE32_hModule, IDS_NAME, szTitle, ARRAY_SIZE(szTitle));
  LoadStringW(MSRLE32_hModule, IDS_ABOUT, szAbout, ARRAY_SIZE(szAbout));

  MessageBoxW(hWnd, szAbout, szTitle, MB_OK|MB_ICONINFORMATION);

  return ICERR_OK;
}

static LRESULT CompressGetFormat(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				 LPBITMAPINFOHEADER lpbiOut)
{
  LRESULT size;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters -- need at least input format */
  if (lpbiIn == NULL) {
    if (lpbiOut != NULL)
      return ICERR_BADPARAM;
    return 0;
  }

  /* handle unsupported input format */
  if (CompressQuery(pi, lpbiIn, NULL) != ICERR_OK)
    return (lpbiOut == NULL ? ICERR_BADFORMAT : 0);

  assert(0 < lpbiIn->biBitCount && lpbiIn->biBitCount <= 8);

  switch (pi->fccHandler) {
  case FOURCC_RLE4:
    size = 1 << 4;
    break;
  case FOURCC_RLE8:
    size = 1 << 8;
    break;
  case FOURCC_RLE:
  case FOURCC_MRLE:
    size = (lpbiIn->biBitCount <= 4 ? 1 << 4 : 1 << 8);
    break;
  default:
    return ICERR_ERROR;
  }

  if (lpbiIn->biClrUsed != 0)
    size = lpbiIn->biClrUsed;

  size = sizeof(BITMAPINFOHEADER) + size * sizeof(RGBQUAD);

  if (lpbiOut != NULL) {
    lpbiOut->biSize          = sizeof(BITMAPINFOHEADER);
    lpbiOut->biWidth         = lpbiIn->biWidth;
    lpbiOut->biHeight        = lpbiIn->biHeight;
    lpbiOut->biPlanes        = 1;
    if (pi->fccHandler == FOURCC_RLE4 ||
	lpbiIn->biBitCount <= 4) {
      lpbiOut->biCompression = BI_RLE4;
      lpbiOut->biBitCount    = 4;
    } else {
      lpbiOut->biCompression = BI_RLE8;
      lpbiOut->biBitCount    = 8;
    }
    lpbiOut->biSizeImage     = MSRLE32_GetMaxCompressedSize(lpbiOut);
    lpbiOut->biXPelsPerMeter = lpbiIn->biXPelsPerMeter;
    lpbiOut->biYPelsPerMeter = lpbiIn->biYPelsPerMeter;
    if (lpbiIn->biClrUsed == 0)
      size = 1<<lpbiIn->biBitCount;
    else
      size = lpbiIn->biClrUsed;
    lpbiOut->biClrUsed       = min(size, 1 << lpbiOut->biBitCount);
    lpbiOut->biClrImportant  = 0;

    memcpy((LPBYTE)lpbiOut + lpbiOut->biSize,
	   (const BYTE*)lpbiIn + lpbiIn->biSize, lpbiOut->biClrUsed * sizeof(RGBQUAD));

    return ICERR_OK;
  } else
    return size;
}

static LRESULT CompressGetSize(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut)
{
  /* pre-condition */
  assert(pi != NULL);

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* check parameter -- need at least one format */
  if (lpbiIn == NULL && lpbiOut == NULL)
    return 0;
  /* check if the given format is supported */
  if (CompressQuery(pi, lpbiIn, lpbiOut) != ICERR_OK)
    return 0;

  /* the worst case is coding the complete image in absolute mode. */
  if (lpbiIn)
    return MSRLE32_GetMaxCompressedSize(lpbiIn);
  else
    return MSRLE32_GetMaxCompressedSize(lpbiOut);
}

static LRESULT CompressQuery(const CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			     LPCBITMAPINFOHEADER lpbiOut)
{
  /* pre-condition */
  assert(pi != NULL);

  /* need at least one format */
  if (lpbiIn == NULL && lpbiOut == NULL)
    return ICERR_BADPARAM;

  /* check input format if given */
  if (lpbiIn != NULL) {
    if (!isSupportedDIB(lpbiIn))
      return ICERR_BADFORMAT;

    /* for 4-bit need an even width */
    if (lpbiIn->biBitCount <= 4 && (lpbiIn->biWidth % 2))
      return ICERR_BADFORMAT;

    if (pi->fccHandler == FOURCC_RLE4 && lpbiIn->biBitCount > 4)
      return ICERR_UNSUPPORTED;
    else if (lpbiIn->biBitCount > 8)
      return ICERR_UNSUPPORTED;
  }

  /* check output format if given */
  if (lpbiOut != NULL) {
    if (!isSupportedMRLE(lpbiOut))
      return ICERR_BADFORMAT;

    if (lpbiIn != NULL) {
      if (lpbiIn->biWidth  != lpbiOut->biWidth)
	return ICERR_UNSUPPORTED;
      if (lpbiIn->biHeight != lpbiOut->biHeight)
	return ICERR_UNSUPPORTED;
      if (lpbiIn->biBitCount > lpbiOut->biBitCount)
	return ICERR_UNSUPPORTED;
    }
  }

  return ICERR_OK;
}

static LRESULT CompressBegin(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			     LPCBITMAPINFOHEADER lpbiOut)
{
  const RGBQUAD *rgbIn;
  const RGBQUAD *rgbOut;
  UINT   i;
  size_t size;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters -- need both formats */
  if (lpbiIn == NULL || lpbiOut == NULL)
    return ICERR_BADPARAM;
  /* And both must be supported */
  if (CompressQuery(pi, lpbiIn, lpbiOut) != ICERR_OK)
    return ICERR_BADFORMAT;

  /* FIXME: cannot compress and decompress at same time! */
  if (pi->bDecompress) {
    FIXME("cannot compress and decompress at same time!\n");
    return ICERR_ERROR;
  }

  if (pi->bCompress)
    CompressEnd(pi);

  size = WIDTHBYTES(lpbiOut->biWidth * 16) / 2 * lpbiOut->biHeight;
  pi->pPrevFrame = GlobalLock(GlobalAlloc(GPTR, size * sizeof(WORD)));
  if (pi->pPrevFrame == NULL)
    return ICERR_MEMORY;
  pi->pCurFrame = GlobalLock(GlobalAlloc(GPTR, size * sizeof(WORD)));
  if (pi->pCurFrame == NULL) {
    CompressEnd(pi);
    return ICERR_MEMORY;
  }
  pi->nPrevFrame = -1;
  pi->bCompress  = TRUE;

  rgbIn  = (const RGBQUAD*)((const BYTE*)lpbiIn  + lpbiIn->biSize);
  rgbOut = (const RGBQUAD*)((const BYTE*)lpbiOut + lpbiOut->biSize);

  switch (lpbiOut->biBitCount) {
  case 4:
  case 8:
    pi->palette_map = LocalAlloc(LPTR, lpbiIn->biClrUsed);
    if (pi->palette_map == NULL) {
      CompressEnd(pi);
      return ICERR_MEMORY;
    }

    for (i = 0; i < lpbiIn->biClrUsed; i++) {
      pi->palette_map[i] = MSRLE32_GetNearestPaletteIndex(lpbiOut->biClrUsed, rgbOut, rgbIn[i]);
    }
    break;
  };

  return ICERR_OK;
}

static LRESULT Compress(CodecInfo *pi, ICCOMPRESS* lpic, DWORD dwSize)
{
  BOOL is_key;
  int i;

  TRACE("(%p,%p,%lu)\n",pi,lpic,dwSize);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters */
  if (lpic == NULL || dwSize < sizeof(ICCOMPRESS))
    return ICERR_BADPARAM;
  if (!lpic->lpbiOutput || !lpic->lpOutput ||
      !lpic->lpbiInput  || !lpic->lpInput)
    return ICERR_BADPARAM;

  TRACE("lpic={0x%lX,%p,%p,%p,%p,%p,%p,%ld,%lu,%lu,%p,%p}\n",lpic->dwFlags,lpic->lpbiOutput,lpic->lpOutput,lpic->lpbiInput,lpic->lpInput,lpic->lpckid,lpic->lpdwFlags,lpic->lFrameNum,lpic->dwFrameSize,lpic->dwQuality,lpic->lpbiPrev,lpic->lpPrev);

  if (! pi->bCompress) {
    LRESULT hr = CompressBegin(pi, lpic->lpbiInput, lpic->lpbiOutput);
    if (hr != ICERR_OK)
      return hr;
  } else if (CompressQuery(pi, lpic->lpbiInput, lpic->lpbiOutput) != ICERR_OK)
    return ICERR_BADFORMAT;

  if (lpic->lFrameNum >= pi->nPrevFrame + 1) {
    /* we continue in the sequence so we need to initialize 
     * our internal framedata */

    computeInternalFrame(pi, lpic->lpbiInput, lpic->lpInput);
  } else if (lpic->lFrameNum == pi->nPrevFrame) {
    /* Oops, compress same frame again ? Okay, as you wish.
     * No need to recompute internal framedata, because we only swapped buffers */
    LPWORD pTmp = pi->pPrevFrame;

    pi->pPrevFrame = pi->pCurFrame;
    pi->pCurFrame  = pTmp;
  } else if ((lpic->dwFlags & ICCOMPRESS_KEYFRAME) == 0) {
    LPWORD pTmp;

    WARN(": prev=%ld cur=%ld gone back? -- untested\n",pi->nPrevFrame,lpic->lFrameNum);
    if (lpic->lpbiPrev == NULL || lpic->lpPrev == NULL)
      return ICERR_GOTOKEYFRAME; /* Need a keyframe if you go back */
    if (CompressQuery(pi, lpic->lpbiPrev, lpic->lpbiOutput) != ICERR_OK)
      return ICERR_BADFORMAT;

    WARN(": prev=%ld cur=%ld compute swapped -- untested\n",pi->nPrevFrame,lpic->lFrameNum);
    computeInternalFrame(pi, lpic->lpbiPrev, lpic->lpPrev);

    /* swap buffers for current and previous frame */
    /* Don't free and alloc new -- costs too much time and they are of equal size ! */
    pTmp = pi->pPrevFrame;
    pi->pPrevFrame = pi->pCurFrame;
    pi->pCurFrame  = pTmp;
    pi->nPrevFrame = lpic->lFrameNum;
  }

  is_key = (lpic->dwFlags & ICCOMPRESS_KEYFRAME) != 0;

  for (i = 0; i < 3; i++) {
    lpic->lpbiOutput->biSizeImage = 0;

    if (lpic->lpbiOutput->biBitCount == 4)
      MSRLE32_CompressRLE4(pi, lpic->lpbiInput, lpic->lpInput, lpic->lpbiOutput, lpic->lpOutput, is_key);
    else
      MSRLE32_CompressRLE8(pi, lpic->lpbiInput, lpic->lpInput, lpic->lpbiOutput, lpic->lpOutput, is_key);

    if (lpic->dwFrameSize == 0 ||
	lpic->lpbiOutput->biSizeImage < lpic->dwFrameSize)
      break;

    if ((*lpic->lpdwFlags & ICCOMPRESS_KEYFRAME) == 0) {
      if (lpic->lpbiOutput->biBitCount == 4)
        MSRLE32_CompressRLE4(pi, lpic->lpbiInput, lpic->lpInput,
                             lpic->lpbiOutput, lpic->lpOutput, TRUE);
      else
        MSRLE32_CompressRLE8(pi, lpic->lpbiInput, lpic->lpInput,
                             lpic->lpbiOutput, lpic->lpOutput, TRUE);

      if (lpic->dwFrameSize == 0 ||
	  lpic->lpbiOutput->biSizeImage < lpic->dwFrameSize) {
	WARN("switched to keyframe, was small enough!\n");
        is_key = TRUE;
	*lpic->lpckid    = MAKEAVICKID(cktypeDIBbits,
				       StreamFromFOURCC(*lpic->lpckid));
	break;
      }
    }

    if (lpic->dwQuality < 1000)
      break;

    lpic->dwQuality -= 1000; /* reduce quality by 10% */
  }

  { /* swap buffer for current and previous frame */
    /* Don't free and alloc new -- costs too much time and they are of equal size ! */
    LPWORD pTmp = pi->pPrevFrame;

    pi->pPrevFrame = pi->pCurFrame;
    pi->pCurFrame  = pTmp;
    pi->nPrevFrame = lpic->lFrameNum;
  }

  /* FIXME: What is AVIIF_TWOCC? */
  *lpic->lpdwFlags |= AVIIF_TWOCC | (is_key ? AVIIF_KEYFRAME : 0);
  return ICERR_OK;
}

static LRESULT CompressEnd(CodecInfo *pi)
{
  TRACE("(%p)\n",pi);

  if (pi != NULL) {
    if (pi->pPrevFrame != NULL)
    {
      GlobalUnlock(GlobalHandle(pi->pPrevFrame));
      GlobalFree(GlobalHandle(pi->pPrevFrame));
    }
    if (pi->pCurFrame != NULL)
    {
      GlobalUnlock(GlobalHandle(pi->pCurFrame));
      GlobalFree(GlobalHandle(pi->pCurFrame));
    }
    pi->pPrevFrame = NULL;
    pi->pCurFrame  = NULL;
    pi->nPrevFrame = -1;
    pi->bCompress  = FALSE;

    if (pi->palette_map != NULL) {
        LocalFree(pi->palette_map);
        pi->palette_map = NULL;
    }
  }

  return ICERR_OK;
}

static LRESULT DecompressGetFormat(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				   LPBITMAPINFOHEADER lpbiOut)
{
  DWORD size;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  if (lpbiIn == NULL)
    return (lpbiOut != NULL ? ICERR_BADPARAM : 0);

  if (DecompressQuery(pi, lpbiIn, NULL) != ICERR_OK)
    return (lpbiOut != NULL ? ICERR_BADFORMAT : 0);

  size = lpbiIn->biSize;

  if (lpbiIn->biBitCount <= 8) {
    int colors;

    if (lpbiIn->biClrUsed == 0)
      colors = 1 << lpbiIn->biBitCount;
    else
      colors = lpbiIn->biClrUsed;

    size += colors * sizeof(RGBQUAD);
  }

  if (lpbiOut != NULL) {
    memcpy(lpbiOut, lpbiIn, size);
    lpbiOut->biCompression  = BI_RGB;
    lpbiOut->biSizeImage    = DIBWIDTHBYTES(*lpbiOut) * lpbiOut->biHeight;

    return ICERR_OK;
  } else
    return size;
}

static LRESULT DecompressQuery(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut)
{
  LRESULT hr = ICERR_OK;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  /* need at least one format */
  if (lpbiIn == NULL && lpbiOut == NULL)
    return ICERR_BADPARAM;

  /* check input format if given */
  if (lpbiIn != NULL) {
    if (!isSupportedMRLE(lpbiIn) && !isSupportedDIB(lpbiIn))
      return ICERR_BADFORMAT;
  }

  /* check output format if given */
  if (lpbiOut != NULL) {
    if (!isSupportedDIB(lpbiOut))
      hr = ICERR_BADFORMAT;

    if (lpbiIn != NULL) {
      if (lpbiIn->biWidth  != lpbiOut->biWidth)
        hr = ICERR_UNSUPPORTED;
      if (lpbiIn->biHeight != lpbiOut->biHeight)
        hr = ICERR_UNSUPPORTED;
      if (lpbiIn->biBitCount > lpbiOut->biBitCount)
        hr = ICERR_UNSUPPORTED;
    }
  }

  return hr;
}

static LRESULT DecompressBegin(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
			       LPCBITMAPINFOHEADER lpbiOut)
{
  const RGBQUAD *rgbIn;
  const RGBQUAD *rgbOut;
  UINT  i;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters */
  if (lpbiIn == NULL || lpbiOut == NULL)
    return ICERR_BADPARAM;
  if (DecompressQuery(pi, lpbiIn, lpbiOut) != ICERR_OK)
    return ICERR_BADFORMAT;

  /* FIXME: cannot compress and decompress at a time! */
  if (pi->bCompress) {
    FIXME("cannot compress and decompress at same time!\n");
    return ICERR_ERROR;
  }

  if (pi->bDecompress)
    DecompressEnd(pi);

  if (lpbiIn->biCompression != BI_RGB)
  {
    int colors;

    if (lpbiIn->biBitCount <= 8 && lpbiIn->biClrUsed == 0)
      colors = 1 << lpbiIn->biBitCount;
    else
      colors = lpbiIn->biClrUsed;

    rgbIn  = (const RGBQUAD*)((const BYTE*)lpbiIn  + lpbiIn->biSize);
    rgbOut = (const RGBQUAD*)((const BYTE*)lpbiOut + lpbiOut->biSize);

    switch (lpbiOut->biBitCount) {
    case 4:
    case 8:
      pi->palette_map = LocalAlloc(LPTR, colors);
      if (pi->palette_map == NULL)
        return ICERR_MEMORY;

      for (i = 0; i < colors; i++)
        pi->palette_map[i] = MSRLE32_GetNearestPaletteIndex(colors, rgbOut, rgbIn[i]);
      break;
    case 15:
    case 16:
      pi->palette_map = LocalAlloc(LPTR, colors * 2);
      if (pi->palette_map == NULL)
        return ICERR_MEMORY;

      for (i = 0; i < colors; i++) {
        WORD color;

        if (lpbiOut->biBitCount == 15)
    color = ((rgbIn[i].rgbRed >> 3) << 10)
      | ((rgbIn[i].rgbGreen >> 3) << 5) | (rgbIn[i].rgbBlue >> 3);
        else
    color = ((rgbIn[i].rgbRed >> 3) << 11)
      | ((rgbIn[i].rgbGreen >> 3) << 5) | (rgbIn[i].rgbBlue >> 3);

        pi->palette_map[i * 2 + 1] = color >> 8;
        pi->palette_map[i * 2 + 0] = color & 0xFF;
      };
      break;
    case 24:
    case 32:
      pi->palette_map = LocalAlloc(LPTR, colors * sizeof(RGBQUAD));
      if (pi->palette_map == NULL)
        return ICERR_MEMORY;
      memcpy(pi->palette_map, rgbIn, colors * sizeof(RGBQUAD));
      break;
    };
  }
  pi->bDecompress = TRUE;

  return ICERR_OK;
}

static LRESULT Decompress(CodecInfo *pi, ICDECOMPRESS *pic, DWORD dwSize)
{
  TRACE("(%p,%p,%lu)\n",pi,pic,dwSize);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters */
  if (pic == NULL)
    return ICERR_BADPARAM;
  if (pic->lpbiInput == NULL || pic->lpInput == NULL ||
      pic->lpbiOutput == NULL || pic->lpOutput == NULL)
    return ICERR_BADPARAM;

  /* check formats */
  if (! pi->bDecompress) {
    LRESULT hr = DecompressBegin(pi, pic->lpbiInput, pic->lpbiOutput);
    if (hr != ICERR_OK)
      return hr;
  } else if (DecompressQuery(pi, pic->lpbiInput, pic->lpbiOutput) != ICERR_OK)
    return ICERR_BADFORMAT;

  assert(pic->lpbiInput->biWidth  == pic->lpbiOutput->biWidth);
  assert(pic->lpbiInput->biHeight == pic->lpbiOutput->biHeight);

  /* Uncompressed frame? */
  if (pic->lpbiInput->biCompression == BI_RGB)
  {
    pic->lpbiOutput->biSizeImage = pic->lpbiInput->biSizeImage;
    memcpy(pic->lpOutput, pic->lpInput, pic->lpbiOutput->biSizeImage);
    return ICERR_OK;
  }

  pic->lpbiOutput->biSizeImage = DIBWIDTHBYTES(*pic->lpbiOutput) * pic->lpbiOutput->biHeight;
  if (pic->lpbiInput->biBitCount == 4)
    return MSRLE32_DecompressRLE4(pi, pic->lpbiOutput, pic->lpInput, pic->lpOutput);
  else
    return MSRLE32_DecompressRLE8(pi, pic->lpbiOutput, pic->lpInput, pic->lpOutput);
}

static LRESULT DecompressEnd(CodecInfo *pi)
{
  TRACE("(%p)\n",pi);

  /* pre-condition */
  assert(pi != NULL);

  pi->bDecompress = FALSE;

  if (pi->palette_map != NULL) {
    LocalFree(pi->palette_map);
    pi->palette_map = NULL;
  }

  return ICERR_OK;
}

static LRESULT DecompressGetPalette(CodecInfo *pi, LPCBITMAPINFOHEADER lpbiIn,
				    LPBITMAPINFOHEADER lpbiOut)
{
  int size;

  TRACE("(%p,%p,%p)\n",pi,lpbiIn,lpbiOut);

  /* pre-condition */
  assert(pi != NULL);

  /* check parameters */
  if (lpbiIn == NULL || lpbiOut == NULL)
    return ICERR_BADPARAM;

  if (DecompressQuery(pi, lpbiIn, lpbiOut) != ICERR_OK)
    return ICERR_BADFORMAT;

  if (lpbiOut->biBitCount > 8)
    return ICERR_ERROR;

  if (lpbiIn->biBitCount <= 8) {
    if (lpbiIn->biClrUsed > 0)
      size = lpbiIn->biClrUsed;
    else
      size = (1 << lpbiIn->biBitCount);

    lpbiOut->biClrUsed = size;

    memcpy((LPBYTE)lpbiOut + lpbiOut->biSize, (const BYTE*)lpbiIn + lpbiIn->biSize, size * sizeof(RGBQUAD));
  } /* else could never occur ! */

  return ICERR_OK;
}

/* DriverProc - entry point for an installable driver */
LRESULT CALLBACK MSRLE32_DriverProc(DWORD_PTR dwDrvID, HDRVR hDrv, UINT uMsg,
				    LPARAM lParam1, LPARAM lParam2)
{
  CodecInfo *pi = (CodecInfo*)dwDrvID;

  TRACE("(%Ix,%p,0x%04X,0x%08IX,0x%08IX)\n", dwDrvID, hDrv, uMsg, lParam1, lParam2);

  switch (uMsg) {
    /* standard driver messages */
  case DRV_LOAD:
    return DRVCNF_OK;
  case DRV_OPEN:
      return (LRESULT)Open((ICOPEN*)lParam2);
  case DRV_CLOSE:
    if (dwDrvID != 0xFFFF0000 && (LPVOID)dwDrvID != NULL)
      Close(pi);
    return DRVCNF_OK;
  case DRV_ENABLE:
  case DRV_DISABLE:
    return DRVCNF_OK;
  case DRV_FREE:
    return DRVCNF_OK;
  case DRV_QUERYCONFIGURE:
    return DRVCNF_CANCEL; /* FIXME */
  case DRV_CONFIGURE:
    return DRVCNF_OK;     /* FIXME */
  case DRV_INSTALL:
  case DRV_REMOVE:
    return DRVCNF_OK;

    /* installable compression manager messages */
  case ICM_CONFIGURE:
    FIXME("ICM_CONFIGURE (%Id)\n",lParam1);
    if (lParam1 == -1)
      return ICERR_UNSUPPORTED; /* FIXME */
    else
      return Configure(pi, (HWND)lParam1);
  case ICM_ABOUT:
    if (lParam1 == -1)
      return ICERR_OK;
    else
      return About(pi, (HWND)lParam1);
  case ICM_GETSTATE:
  case ICM_SETSTATE:
    return 0; /* no state */
  case ICM_GETINFO:
    return GetInfo(pi, (ICINFO*)lParam1, (DWORD)lParam2);
  case ICM_GETDEFAULTQUALITY:
    if ((LPVOID)lParam1 != NULL) {
      *((LPDWORD)lParam1) = MSRLE32_DEFAULTQUALITY;
      return ICERR_OK;
    }
    break;
  case ICM_COMPRESS_GET_FORMAT:
    return CompressGetFormat(pi, (LPCBITMAPINFOHEADER)lParam1,
			     (LPBITMAPINFOHEADER)lParam2);
  case ICM_COMPRESS_GET_SIZE:
    return CompressGetSize(pi, (LPCBITMAPINFOHEADER)lParam1,
			   (LPCBITMAPINFOHEADER)lParam2);
  case ICM_COMPRESS_QUERY:
    return CompressQuery(pi, (LPCBITMAPINFOHEADER)lParam1,
			 (LPCBITMAPINFOHEADER)lParam2);
  case ICM_COMPRESS_BEGIN:
    return CompressBegin(pi, (LPCBITMAPINFOHEADER)lParam1,
			 (LPCBITMAPINFOHEADER)lParam2);
  case ICM_COMPRESS:
    return Compress(pi, (ICCOMPRESS*)lParam1, (DWORD)lParam2);
  case ICM_COMPRESS_END:
    return CompressEnd(pi);
  case ICM_DECOMPRESS_GET_FORMAT:
    return DecompressGetFormat(pi, (LPCBITMAPINFOHEADER)lParam1,
			       (LPBITMAPINFOHEADER)lParam2);
  case ICM_DECOMPRESS_QUERY:
    return DecompressQuery(pi, (LPCBITMAPINFOHEADER)lParam1,
			   (LPCBITMAPINFOHEADER)lParam2);
  case ICM_DECOMPRESS_BEGIN:
    return DecompressBegin(pi, (LPCBITMAPINFOHEADER)lParam1,
			   (LPCBITMAPINFOHEADER)lParam2);
  case ICM_DECOMPRESS:
    return Decompress(pi, (ICDECOMPRESS*)lParam1, (DWORD)lParam2);
  case ICM_DECOMPRESS_END:
    return DecompressEnd(pi);
  case ICM_DECOMPRESS_SET_PALETTE:
    FIXME("(...) -> SetPalette(%p,%p,%p): stub!\n", pi, (LPVOID)lParam1, (LPVOID)lParam2);
    return ICERR_UNSUPPORTED;
  case ICM_DECOMPRESS_GET_PALETTE:
    return DecompressGetPalette(pi, (LPBITMAPINFOHEADER)lParam1,
				(LPBITMAPINFOHEADER)lParam2);
  case ICM_GETDEFAULTKEYFRAMERATE:
    if ((LPVOID)lParam1 != NULL)
      *(LPDWORD)lParam1 = 15;
    return ICERR_OK;
  default:
    if (uMsg < DRV_USER)
      return DefDriverProc(dwDrvID, hDrv, uMsg, lParam1, lParam2);
    else
      FIXME("Unknown message uMsg=0x%08X lParam1=0x%08IX lParam2=0x%08IX\n",uMsg,lParam1,lParam2);
  };

  return ICERR_UNSUPPORTED;
}

/* DllMain - library initialization code */
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
  TRACE("(%p,%ld,%p)\n",hModule,dwReason,lpReserved);

  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls(hModule);
    MSRLE32_hModule = hModule;
    break;
  }

  return TRUE;
}
