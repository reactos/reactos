/*
 * X11 physical font definitions
 *
 * Copyright 1997 Alex Korobka
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

#ifndef __WINE_X11FONT_H
#define __WINE_X11FONT_H

#include "wine/wingdi16.h"
#include "x11drv.h"
#include "pshpack1.h"

/* this is a part of the font resource header, should
 * make it easier to implement dynamic softfont loading */

typedef struct
{
    INT16	dfType;
    INT16	dfPoints;
    INT16	dfVertRes;
    INT16	dfHorizRes;
    INT16	dfAscent;
    INT16	dfInternalLeading;
    INT16	dfExternalLeading;
    CHAR	dfItalic;
    CHAR	dfUnderline;
    CHAR	dfStrikeOut;
    INT16	dfWeight;
    BYTE	dfCharSet;
    INT16	dfPixWidth;
    INT16	dfPixHeight;
    CHAR	dfPitchAndFamily;
    INT16	dfAvgWidth;
    INT16	dfMaxWidth;
    CHAR	dfFirstChar;
    CHAR	dfLastChar;
    CHAR	dfDefaultChar;
    CHAR	dfBreakChar;
    INT16	dfWidthBytes;
    LPCSTR	dfDevice;
    LPCSTR	dfFace;
} IFONTINFO16, *LPIFONTINFO16;

#include "poppack.h"

/* internal flags */

#define FI_POLYWEIGHT	    0x0001
#define FI_POLYSLANT 	    0x0002
#define FI_OBLIQUE	    0x0004
#define FI_SCALABLE  	    0x0008
#define FI_FW_BOOK	    0x0010
#define FI_FW_DEMI	    0x0020
#define FI_VARIABLEPITCH    0x0040
#define FI_FIXEDPITCH       0x0080

#define FI_FIXEDEX	    0x1000
#define FI_NORMAL	    0x2000
#define FI_SUBSET 	    0x4000
#define FI_TRUETYPE	    0x8000

/* code pages */

#define FI_ENC_ANSI		0
#define FI_ENC_ISO8859	    	1
#define FI_ENC_ISO646		2
#define FI_ENC_MICROSOFT	3
#define FI_ENC_KOI8		4
#define FI_ENC_ASCII		5
#define FI_ENC_VISCII		6
#define FI_ENC_TCVN		7
#define FI_ENC_TIS620		8

enum X11DRV_CPTABLE
{
    X11DRV_CPTABLE_SBCS,
    X11DRV_CPTABLE_UNICODE,
    X11DRV_CPTABLE_CP932,
    X11DRV_CPTABLE_CP936,
    X11DRV_CPTABLE_CP949,
    X11DRV_CPTABLE_CP950,
    X11DRV_CPTABLE_SYMBOL,
    X11DRV_CPTABLE_COUNT
};

typedef struct tagFontInfo
{
    struct tagFontInfo*		next;
    UINT16			fi_flags;
    UINT16			fi_encoding;
    UINT16			codepage;
    UINT16			cptable;
    WORD			internal_charset;

 /* LFD parameters can be quite different from the actual metrics */

    UINT16			lfd_height;
    UINT16			lfd_resolution;
    IFONTINFO16			df;
} fontInfo;

/* Font resource list for EnumFont() purposes */

#define FR_SOFTFONT         0x1000              /* - .FON or .FOT file */
#define FR_SOFTRESOURCE     0x2000              /* - resource handle */
#define FR_REMOVED          0x4000              /* delayed remove */
#define FR_NAMESET          0x8000

#define LFD_FIELDS 14
typedef struct
{
    const char* foundry;
    const char* family;
    const char* weight;
    const char* slant;
    const char* set_width;
    const char* add_style;
    const char* pixel_size;
    const char* point_size;
    const char* resolution_x;
    const char* resolution_y;
    const char* spacing;
    const char* average_width;
    const char* charset_registry;
    const char* charset_encoding;
} LFD;

typedef struct tagFontResource
{
  struct tagFontResource*	next;
  UINT16			fr_flags;
  UINT16			fr_penalty;
  UINT16			fi_count;
  UINT16			fo_count;
  fontInfo*			fi;
  LFD*                          resource;
  HANDLE			hOwner;		/*  For FR_SOFTFONT/FR_SOFTRESOURCE fonts */
  CHAR				lfFaceName[LF_FACESIZE];
} fontResource;

typedef struct {
  float		a,b,c,d;	/* pixelsize matrix, FIXME: switch to MAT2 format */
  unsigned long	RAW_ASCENT;
  unsigned long	RAW_DESCENT;
  float		pixelsize;
  float		ascent;
  float		descent;
} XFONTTRANS;

#define FO_RESOURCE_MASK	0x000F
#define FO_SYSTEM		0x0001		/* resident in cache */
#define FO_SOFTFONT		0x0002		/* installed at runtime */
#define FO_SHARED		0x0004		/* MITSHM */
#define FO_REMOVED		0x0008		/* remove when count falls to 0 */

#define FO_MATCH_MASK		0x00F0
#define FO_MATCH_NORASTER	0x0010
#define FO_MATCH_PAF		0x0020
#define FO_MATCH_XYINDEP	0x0040

#define FO_SYNTH_MASK		0xFF00
#define FO_SYNTH_HEIGHT   	0x2000
#define FO_SYNTH_WIDTH		0x4000
#define FO_SYNTH_ROTATE 	0x8000
#define FO_SYNTH_BOLD		0x0100
#define FO_SYNTH_ITALIC		0x0200
#define FO_SYNTH_UNDERLINE	0x0400
#define FO_SYNTH_STRIKEOUT	0x0800

/* Realized screen font */

#define	X11FONT_REFOBJS_MAX		4

typedef struct
{
  XFontStruct*          fs;			/* text metrics */
  fontResource*         fr;			/* font family */
  fontInfo*		fi;			/* font instance info */
  Pixmap*               lpPixmap;		/* optional character bitmasks for synth fonts */
  X_PHYSFONT		prefobjs[X11FONT_REFOBJS_MAX];	/* font objects for DBCS charsets */

  XFONTTRANS		*lpX11Trans;		/* Info for X11R6 transform */
  float                 rescale;                /* Rescale for large fonts */
  INT16			foInternalLeading;
  INT16			foAvgCharWidth;
  INT16			foMaxCharWidth;
  UINT16		fo_flags;

  /* font cache housekeeping */

  UINT16                count;
  UINT16                lru;
  UINT16                lfchecksum;
  LOGFONT16             lf;
} fontObject;

typedef struct
{
  fontResource*		pfr;
  fontInfo*		pfi;
  UINT16		height;
  UINT16		flags;
  LPLOGFONT16		plf;
  WORD			internal_charset;
} fontMatch;

extern fontObject* XFONT_GetFontObject( X_PHYSFONT pFont );
extern XFontStruct* XFONT_GetFontStruct( X_PHYSFONT pFont );

/* internal charset(hibyte must be set) */
/* lobyte is DEFAULT_CHARSET(=0). */
#define X11FONT_JISX0201_CHARSET	0x100
#define X11FONT_JISX0212_CHARSET	0x200

typedef struct tagX11DRV_CP
{
    WORD (*penum_subfont_charset)( UINT index );
    XChar2b* (*punicode_to_char2b)( fontObject* pfo,
                                    LPCWSTR lpwstr, UINT count );
    void (*pDrawString)( fontObject* pfo, Display* pdisp, Drawable d, GC gc,
                         int x, int y, XChar2b* pstr, int count );
    int (*pTextWidth)( fontObject* pfo, XChar2b* pstr, int count );
    void (*pDrawText)( fontObject* pfo, Display* pdisp, Drawable d, GC gc,
                       int x, int y, XTextItem16* pitems, int count );
    void (*pTextExtents)( fontObject* pfo, XChar2b* pstr, int count,
                          int* pdir, int* pascent, int* pdescent,
                          int* pwidth, int max_extent, int *pfit,
                          int* partial_extents );
    void (*pGetTextMetricsW)( fontObject* pfo, LPTEXTMETRICW pTM );
} X11DRV_CP;

extern const X11DRV_CP X11DRV_cptable[X11DRV_CPTABLE_COUNT];

#endif /* __WINE_X11FONT_H */
