/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/font.c
 * PURPOSE:         Font Engine support functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "ntrosgdi.h"
#include "winent.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosgdidrv);

static gsCacheEntry *glyphsetCache = NULL;
static DWORD glyphsetCacheSize = 0;
static INT lastfree = -1;
static INT mru = -1;

#define INIT_CACHE_SIZE 10

static int antialias = 1;

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

#define MS_GASP_TAG MS_MAKE_TAG('g', 'a', 's', 'p')

#define GASP_GRIDFIT 0x01
#define GASP_DOGRAY  0x02

#ifdef WORDS_BIGENDIAN
#define get_be_word(x) (x)
#define NATIVE_BYTE_ORDER MSBFirst
#else
//#define get_be_word(x) RtlUshortByteSwap(x)
static __inline USHORT get_be_word(USHORT s)
{
    return (s >> 8) | (s << 8);
}
#define NATIVE_BYTE_ORDER LSBFirst
#endif

/* FUNCTIONS **************************************************************/

/***********************************************************************
 *           RosDrv_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1998 Huw Davies
 */
INT RosDrv_XWStoDS( NTDRV_PDEVICE *physDev, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP( physDev->hUserDC, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           RosDrv_YWStoDS
 *
 * Performs a world-to-viewport transformation on the specified height.
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1998 Huw Davies
 */
INT RosDrv_YWStoDS( NTDRV_PDEVICE *physDev, INT height )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP( physDev->hUserDC, pt, 2 );
    return pt[1].y - pt[0].y;
}

/* from winex11/xrender.c
 * Copyright 2001, 2002 Huw D M Davies for CodeWeavers
 */

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->devsize, &p2->devsize, sizeof(p1->devsize))) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return strcmpiW(p1->lf.lfFaceName, p2->lf.lfFaceName);
}

static int LookupEntry(LFANDSIZE *plfsz)
{
  int i, prev_i = -1;

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    TRACE("%d\n", i);
    if(glyphsetCache[i].count == -1) { /* reached free list so stop */
      i = -1;
      break;
    }

    if(!fontcmp(&glyphsetCache[i].lfsz, plfsz)) {
      glyphsetCache[i].count++;
      if(prev_i >= 0) {
	glyphsetCache[prev_i].next = glyphsetCache[i].next;
	glyphsetCache[i].next = mru;
	mru = i;
      }
      TRACE("found font in cache %d\n", i);
      return i;
    }
    prev_i = i;
  }
  TRACE("font not in cache\n");
  return -1;
}

static void FreeEntry(int entry)
{
    int i, format;
  
    for(format = 0; format < AA_MAXVALUE; format++) {
        gsCacheEntryFormat * formatEntry;

        if( !glyphsetCache[entry].format[format] )
            continue;

        formatEntry = glyphsetCache[entry].format[format];

        //if(formatEntry->glyphset) {
            //wine_tsx11_lock();
            //pXRenderFreeGlyphSet(gdi_display, formatEntry->glyphset);
            //wine_tsx11_unlock();
            //formatEntry->glyphset = 0;
        //}
        if(formatEntry->nrealized) {
            HeapFree(GetProcessHeap(), 0, formatEntry->realized);
            formatEntry->realized = NULL;
            if(formatEntry->bitmaps) {
                for(i = 0; i < formatEntry->nrealized; i++)
                    HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps[i]);
                HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps);
                formatEntry->bitmaps = NULL;
            }
            HeapFree(GetProcessHeap(), 0, formatEntry->gis);
            formatEntry->gis = NULL;
            formatEntry->nrealized = 0;
        }

        HeapFree(GetProcessHeap(), 0, formatEntry);
        glyphsetCache[entry].format[format] = NULL;
    }
}

static int AllocEntry(void)
{
  int best = -1, prev_best = -1, i, prev_i = -1;

  if(lastfree >= 0) {
    //assert(glyphsetCache[lastfree].count == -1);
    glyphsetCache[lastfree].count = 1;
    best = lastfree;
    lastfree = glyphsetCache[lastfree].next;
    //assert(best != mru);
    glyphsetCache[best].next = mru;
    mru = best;

    TRACE("empty space at %d, next lastfree = %d\n", mru, lastfree);
    return mru;
  }

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    if(glyphsetCache[i].count == 0) {
      best = i;
      prev_best = prev_i;
    }
    prev_i = i;
  }

  if(best >= 0) {
    TRACE("freeing unused glyphset at cache %d\n", best);
    FreeEntry(best);
    glyphsetCache[best].count = 1;
    if(prev_best >= 0) {
      glyphsetCache[prev_best].next = glyphsetCache[best].next;
      glyphsetCache[best].next = mru;
      mru = best;
    } else {
      //assert(mru == best);
    }
    return mru;
  }

  TRACE("Growing cache\n");
  
  if (glyphsetCache)
    glyphsetCache = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      glyphsetCache,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));
  else
    glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));

  for(best = i = glyphsetCacheSize; i < glyphsetCacheSize + INIT_CACHE_SIZE;
      i++) {
    glyphsetCache[i].next = i + 1;
    glyphsetCache[i].count = -1;
  }
  glyphsetCache[i-1].next = -1;
  glyphsetCacheSize += INIT_CACHE_SIZE;

  lastfree = glyphsetCache[best].next;
  glyphsetCache[best].count = 1;
  glyphsetCache[best].next = mru;
  mru = best;
  TRACE("new free cache slot at %d\n", mru);
  return mru;
}

static BOOL get_gasp_flags(NTDRV_PDEVICE *physDev, WORD *flags)
{
    DWORD size;
    WORD *gasp, *buffer;
    WORD num_recs;
    DWORD ppem;
    TEXTMETRICW tm;

    *flags = 0;

    size = GetFontData(physDev->hUserDC, MS_GASP_TAG,  0, NULL, 0);
    if(size == GDI_ERROR)
        return FALSE;

    gasp = buffer = HeapAlloc(GetProcessHeap(), 0, size);
    GetFontData(physDev->hUserDC, MS_GASP_TAG,  0, gasp, size);

    GetTextMetricsW(physDev->hUserDC, &tm);
    ppem = abs(RosDrv_YWStoDS(physDev, tm.tmAscent + tm.tmDescent - tm.tmInternalLeading));

    gasp++;
    num_recs = get_be_word(*gasp);
    gasp++;
    while(num_recs--)
    {
        *flags = get_be_word(*(gasp + 1));
        if(ppem <= get_be_word(*gasp))
            break;
        gasp += 2;
    }
    TRACE("got flags %04x for ppem %d\n", *flags, ppem);

    HeapFree(GetProcessHeap(), 0, buffer);
    return TRUE;
}

static AA_Type get_antialias_type( NTDRV_PDEVICE *physDev, BOOL subpixel, BOOL hinter)
{
    AA_Type ret;
    WORD flags;
    UINT font_smoothing_type, font_smoothing_orientation;

    if (SystemParametersInfoW( SPI_GETFONTSMOOTHINGTYPE, 0, &font_smoothing_type, 0) &&
        font_smoothing_type == FE_FONTSMOOTHINGCLEARTYPE)
    {
        if ( SystemParametersInfoW( SPI_GETFONTSMOOTHINGORIENTATION, 0,
                                    &font_smoothing_orientation, 0) &&
             font_smoothing_orientation == FE_FONTSMOOTHINGORIENTATIONBGR)
        {
            ret = AA_BGR;
        }
        else
            ret = AA_RGB;
        /*FIXME
          If the monitor is in portrait mode, ClearType is disabled in the MS Windows (MSDN).
          But, Wine's subpixel rendering can support the portrait mode.
         */
    }
    else if (!hinter || !get_gasp_flags(physDev, &flags) || flags & GASP_DOGRAY)
        ret = AA_Grey;
    else
        ret = AA_None;

    return ret;
}

static int GetCacheEntry(NTDRV_PDEVICE *physDev, LFANDSIZE *plfsz)
{
    int ret;
    int format;
    gsCacheEntry *entry;
    static int hinter = -1;
    static int subpixel = -1;
    BOOL font_smoothing;

    if((ret = LookupEntry(plfsz)) != -1) return ret;

    ret = AllocEntry();
    entry = glyphsetCache + ret;
    entry->lfsz = *plfsz;
    for( format = 0; format < AA_MAXVALUE; format++ ) {
        //assert( !entry->format[format] );
    }

    if(antialias && plfsz->lf.lfQuality != NONANTIALIASED_QUALITY)
    {
        if(hinter == -1 || subpixel == -1)
        {
            RASTERIZER_STATUS status;
            GetRasterizerCaps(&status, sizeof(status));
            hinter = status.wFlags & WINE_TT_HINTER_ENABLED;
            subpixel = status.wFlags & WINE_TT_SUBPIXEL_RENDERING_ENABLED;
        }

        switch (plfsz->lf.lfQuality)
        {
            case ANTIALIASED_QUALITY:
                entry->aa_default = get_antialias_type( physDev, FALSE, hinter );
                break;
            case CLEARTYPE_QUALITY:
            case CLEARTYPE_NATURAL_QUALITY:
                entry->aa_default = get_antialias_type( physDev, subpixel, hinter );
                break;
            case DEFAULT_QUALITY:
            case DRAFT_QUALITY:
            case PROOF_QUALITY:
            default:
                if ( SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0) &&
                     font_smoothing)
                {
                    entry->aa_default = get_antialias_type( physDev, subpixel, hinter );
                }
                else
                    entry->aa_default = AA_None;
                break;
        }
    }
    else
        entry->aa_default = AA_None;

    return ret;
}

static void dec_ref_cache(int index)
{
    //assert(index >= 0);
    TRACE("dec'ing entry %d to %d\n", index, glyphsetCache[index].count - 1);
    //assert(glyphsetCache[index].count > 0);
    glyphsetCache[index].count--;
}

static void lfsz_calc_hash(LFANDSIZE *plfsz)
{
  DWORD hash = 0, *ptr, two_chars;
  WORD *pwc;
  int i;

  hash ^= plfsz->devsize.cx;
  hash ^= plfsz->devsize.cy;
  for(i = 0, ptr = (DWORD*)&plfsz->xform; i < sizeof(XFORM)/sizeof(DWORD); i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)&plfsz->lf; i < 7; i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)plfsz->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
    two_chars = *ptr;
    pwc = (WCHAR *)&two_chars;
    if(!*pwc) break;
    *pwc = toupperW(*pwc);
    pwc++;
    *pwc = toupperW(*pwc);
    hash ^= two_chars;
    if(!*pwc) break;
  }
  plfsz->hash = hash;
  return;
}

/************************************************************************
 *   UploadGlyph
 *
 * Helper to ExtTextOut.  Must be called inside xrender_cs
 */
static BOOL UploadGlyph(NTDRV_PDEVICE *physDev, int glyph, AA_Type format)
{
    unsigned int buflen;
    char *buf;
    //Glyph gid;
    GLYPHMETRICS gm;
    GlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + physDev->cache_index;
    gsCacheEntryFormat *formatEntry;
    UINT ggo_format = GGO_GLYPH_INDEX;
    static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

    switch(format) {
    case AA_Grey:
        ggo_format |= WINE_GGO_GRAY16_BITMAP;
        break;
    case AA_RGB:
        ggo_format |= WINE_GGO_HRGB_BITMAP;
        break;
    case AA_BGR:
        ggo_format |= WINE_GGO_HBGR_BITMAP;
        break;
    case AA_VRGB:
        ggo_format |= WINE_GGO_VRGB_BITMAP;
        break;
    case AA_VBGR:
        ggo_format |= WINE_GGO_VBGR_BITMAP;
        break;

    default:
        ERR("aa = %d - not implemented\n", format);
    case AA_None:
        ggo_format |= GGO_BITMAP;
        break;
    }

    buflen = GetGlyphOutlineW(physDev->hUserDC, glyph, ggo_format, &gm, 0, NULL, &identity);
    if(buflen == GDI_ERROR) {
        if(format != AA_None) {
            format = AA_None;
            entry->aa_default = AA_None;
            ggo_format = GGO_GLYPH_INDEX | GGO_BITMAP;
            buflen = GetGlyphOutlineW(physDev->hUserDC, glyph, ggo_format, &gm, 0, NULL, &identity);
        }
        if(buflen == GDI_ERROR) {
            WARN("GetGlyphOutlineW failed\n");
            return FALSE;
        }
        TRACE("Turning off antialiasing for this monochrome font\n");
    }

    /* If there is nothing for the current type, we create the entry. */
    if( !entry->format[format] ) {
        entry->format[format] = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(gsCacheEntryFormat));
    }
    formatEntry = entry->format[format];

    if(formatEntry->nrealized <= glyph) {
        formatEntry->nrealized = (glyph / 128 + 1) * 128;

        if (formatEntry->realized)
            formatEntry->realized = HeapReAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            formatEntry->realized,
            formatEntry->nrealized * sizeof(BOOL));
        else
            formatEntry->realized = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            formatEntry->nrealized * sizeof(BOOL));

            if (formatEntry->bitmaps)
                formatEntry->bitmaps = HeapReAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                formatEntry->bitmaps,
                formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
            else
                formatEntry->bitmaps = HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
        if (formatEntry->gis)
            formatEntry->gis = HeapReAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            formatEntry->gis,
            formatEntry->nrealized * sizeof(formatEntry->gis[0]));
        else
            formatEntry->gis = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            formatEntry->nrealized * sizeof(formatEntry->gis[0]));
    }

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    GetGlyphOutlineW(physDev->hUserDC, glyph, ggo_format, &gm, buflen, buf, &identity);
    formatEntry->realized[glyph] = TRUE;

    TRACE("buflen = %d. Got metrics: %dx%d adv=%d,%d origin=%d,%d\n",
        buflen,
        gm.gmBlackBoxX, gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY,
        gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    gi.width = gm.gmBlackBoxX;
    gi.height = gm.gmBlackBoxY;
    gi.x = -gm.gmptGlyphOrigin.x;
    gi.y = gm.gmptGlyphOrigin.y;
    gi.xOff = gm.gmCellIncX;
    gi.yOff = gm.gmCellIncY;

#if 0
    if(TRACE_ON(xrender)) {
        int pitch, i, j;
        char output[300];
        unsigned char *line;

        if(format == AA_None) {
            pitch = ((gi.width + 31) / 32) * 4;
            for(i = 0; i < gi.height; i++) {
                line = (unsigned char*) buf + i * pitch;
                output[0] = '\0';
                for(j = 0; j < pitch * 8; j++) {
                    strcat(output, (line[j / 8] & (1 << (7 - (j % 8)))) ? "#" : " ");
                }
                TRACE("%s\n", output);
            }
        } else {
            static const char blks[] = " .:;!o*#";
            char str[2];

            str[1] = '\0';
            pitch = ((gi.width + 3) / 4) * 4;
            for(i = 0; i < gi.height; i++) {
                line = (unsigned char*) buf + i * pitch;
                output[0] = '\0';
                for(j = 0; j < pitch; j++) {
                    str[0] = blks[line[j] >> 5];
                    strcat(output, str);
                }
                TRACE("%s\n", output);
            }
        }
    }
#endif

    formatEntry->bitmaps[glyph] = buf;
    formatEntry->gis[glyph] = gi;

    return TRUE;
}


VOID
FeSelectFont(NTDRV_PDEVICE *physDev, HFONT hfont)
{
    LFANDSIZE lfsz;

    GetObjectW(hfont, sizeof(lfsz.lf), &lfsz.lf);
    TRACE("h=%d w=%d weight=%d it=%d charset=%d name=%s\n",
        lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
        lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.lf.lfWidth = abs( lfsz.lf.lfWidth );
    lfsz.devsize.cx = RosDrv_XWStoDS( physDev, lfsz.lf.lfWidth );
    lfsz.devsize.cy = RosDrv_YWStoDS( physDev, lfsz.lf.lfHeight );
    GetWorldTransform( physDev->hUserDC, &lfsz.xform );
    lfsz_calc_hash(&lfsz);

    /*EnterCriticalSection(&xrender_cs);*/
    if (physDev->cache_index != -1)
        dec_ref_cache(physDev->cache_index);
    physDev->cache_index = GetCacheEntry(physDev, &lfsz);
    /*LeaveCriticalSection(&xrender_cs);*/
}

BOOL FeTextOut( NTDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
               const RECT *lprect, LPCWSTR wstr, UINT count,
               const INT *lpDx )
{
    //RGNDATA *data;
    //XGCValues xgcval;
    gsCacheEntry *entry;
    gsCacheEntryFormat *formatEntry;
    BOOL retv = FALSE;
    //HDC hdc = physDev->hUserDC;
    //int textPixel, backgroundPixel;
    //HRGN saved_region = 0;
    BOOL disable_antialias = FALSE;
    AA_Type aa_type = AA_None;
    //DIBSECTION bmp;
    unsigned int idx;
    double cosEsc, sinEsc;
    LOGFONTW lf;
    //enum drawable_depth_type depth_type = (physDev->depth == 1) ? mono_drawable : color_drawable;
    //Picture tile_pict = 0;

    /* Do we need to disable antialiasing because of palette mode? */
#if 0
    if( !physDev->bitmap || GetObjectW( physDev->bitmap->hbitmap, sizeof(bmp), &bmp ) != sizeof(bmp) ) {
        TRACE("bitmap is not a DIB\n");
    }
    else if (bmp.dsBmih.biBitCount <= 8) {
        TRACE("Disabling antialiasing\n");
        disable_antialias = TRUE;
    }
#endif

    //RosDrv_LockDIBSection( physDev, DIB_Status_GdiMod );

#if 0
    if(physDev->depth == 1) {
        if((physDev->textPixel & 0xffffff) == 0) {
            textPixel = 0;
            backgroundPixel = 1;
        } else {
            textPixel = 1;
            backgroundPixel = 0;
        }
    } else {
        textPixel = physDev->textPixel;
        backgroundPixel = physDev->backgroundPixel;
    }
#endif

    if(flags & ETO_OPAQUE)
    {
#if 0
        wine_tsx11_lock();
        XSetForeground( gdi_display, physDev->gc, backgroundPixel );
        XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
            physDev->dc_rect.left + lprect->left, physDev->dc_rect.top + lprect->top,
            lprect->right - lprect->left, lprect->bottom - lprect->top );
        wine_tsx11_unlock();
#endif
    }

    if(count == 0)
    {
        retv = TRUE;
        goto done_unlock;
    }


    GetObjectW(GetCurrentObject(physDev->hUserDC, OBJ_FONT), sizeof(lf), &lf);
    if(lf.lfEscapement != 0) {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    } else {
        cosEsc = 1;
        sinEsc = 0;
    }

    if (flags & ETO_CLIPPED)
    {
        HRGN clip_region;

        clip_region = CreateRectRgnIndirect( lprect );
#if 0
        /* make a copy of the current device region */
        saved_region = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( saved_region, physDev->region, 0, RGN_COPY );
        RosDrv_SetDeviceClipping( physDev, saved_region, clip_region );
#endif
        DeleteObject( clip_region );
    }

    //EnterCriticalSection(&xrender_cs);

    entry = glyphsetCache + physDev->cache_index;
    if( disable_antialias == FALSE )
        aa_type = entry->aa_default;
    formatEntry = entry->format[aa_type];

    for(idx = 0; idx < count; idx++) {
        if( !formatEntry ) {
            UploadGlyph(physDev, wstr[idx], aa_type);
            /* re-evaluate antialias since aa_default may have changed */
            if( disable_antialias == FALSE )
                aa_type = entry->aa_default;
            formatEntry = entry->format[aa_type];
        } else if( wstr[idx] >= formatEntry->nrealized || formatEntry->realized[wstr[idx]] == FALSE) {
            UploadGlyph(physDev, wstr[idx], aa_type);
        }
    }
    if (!formatEntry)
    {
        WARN("could not upload requested glyphs\n");
        //LeaveCriticalSection(&xrender_cs);
        goto done_unlock;
    }

    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count),
        /*physDev->dc_rect.left +*/ x, /*physDev->dc_rect.top +*/ y);

    RosGdiExtTextOut(physDev->hKernelDC, x, y, flags, lprect, wstr, count, lpDx, formatEntry);

#if 0
    {
        INT offset = 0, xoff = 0, yoff = 0;
        wine_tsx11_lock();
        XSetForeground( gdi_display, physDev->gc, textPixel );

        if(aa_type == AA_None || physDev->depth == 1)
        {
            void (* sharp_glyph_fn)(X11DRV_PDEVICE *, INT, INT, void *, XGlyphInfo *);

            if(aa_type == AA_None)
                sharp_glyph_fn = SharpGlyphMono;
            else
                sharp_glyph_fn = SharpGlyphGray;

            for(idx = 0; idx < count; idx++) {
                sharp_glyph_fn(physDev, physDev->dc_rect.left + x + xoff,
                    physDev->dc_rect.top + y + yoff,
                    formatEntry->bitmaps[wstr[idx]],
                    &formatEntry->gis[wstr[idx]]);
                if(lpDx) {
                    offset += lpDx[idx];
                    xoff = offset * cosEsc;
                    yoff = offset * -sinEsc;
                } else {
                    xoff += formatEntry->gis[wstr[idx]].xOff;
                    yoff += formatEntry->gis[wstr[idx]].yOff;
                }
            }
        } else {
            XImage *image;
            int image_x, image_y, image_off_x, image_off_y, image_w, image_h;
            RECT extents = {0, 0, 0, 0};
            POINT cur = {0, 0};
            int w = physDev->drawable_rect.right - physDev->drawable_rect.left;
            int h = physDev->drawable_rect.bottom - physDev->drawable_rect.top;

            TRACE("drawable %dx%d\n", w, h);

            for(idx = 0; idx < count; idx++) {
                if(extents.left > cur.x - formatEntry->gis[wstr[idx]].x)
                    extents.left = cur.x - formatEntry->gis[wstr[idx]].x;
                if(extents.top > cur.y - formatEntry->gis[wstr[idx]].y)
                    extents.top = cur.y - formatEntry->gis[wstr[idx]].y;
                if(extents.right < cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width)
                    extents.right = cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width;
                if(extents.bottom < cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height)
                    extents.bottom = cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height;
                if(lpDx) {
                    offset += lpDx[idx];
                    cur.x = offset * cosEsc;
                    cur.y = offset * -sinEsc;
                } else {
                    cur.x += formatEntry->gis[wstr[idx]].xOff;
                    cur.y += formatEntry->gis[wstr[idx]].yOff;
                }
            }
            TRACE("glyph extents %d,%d - %d,%d drawable x,y %d,%d\n", extents.left, extents.top,
                extents.right, extents.bottom, physDev->dc_rect.left + x, physDev->dc_rect.top + y);

            if(physDev->dc_rect.left + x + extents.left >= 0) {
                image_x = physDev->dc_rect.left + x + extents.left;
                image_off_x = 0;
            } else {
                image_x = 0;
                image_off_x = physDev->dc_rect.left + x + extents.left;
            }
            if(physDev->dc_rect.top + y + extents.top >= 0) {
                image_y = physDev->dc_rect.top + y + extents.top;
                image_off_y = 0;
            } else {
                image_y = 0;
                image_off_y = physDev->dc_rect.top + y + extents.top;
            }
            if(physDev->dc_rect.left + x + extents.right < w)
                image_w = physDev->dc_rect.left + x + extents.right - image_x;
            else
                image_w = w - image_x;
            if(physDev->dc_rect.top + y + extents.bottom < h)
                image_h = physDev->dc_rect.top + y + extents.bottom - image_y;
            else
                image_h = h - image_y;

            if(image_w <= 0 || image_h <= 0) goto no_image;

            X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
            image = XGetImage(gdi_display, physDev->drawable,
                image_x, image_y, image_w, image_h,
                AllPlanes, ZPixmap);
            X11DRV_check_error();

            TRACE("XGetImage(%p, %x, %d, %d, %d, %d, %lx, %x) depth = %d rets %p\n",
                gdi_display, (int)physDev->drawable, image_x, image_y,
                image_w, image_h, AllPlanes, ZPixmap,
                physDev->depth, image);
            if(!image) {
                Pixmap xpm = XCreatePixmap(gdi_display, root_window, image_w, image_h,
                    physDev->depth);
                GC gc;
                XGCValues gcv;

                gcv.graphics_exposures = False;
                gc = XCreateGC(gdi_display, xpm, GCGraphicsExposures, &gcv);
                XCopyArea(gdi_display, physDev->drawable, xpm, gc, image_x, image_y,
                    image_w, image_h, 0, 0);
                XFreeGC(gdi_display, gc);
                X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
                image = XGetImage(gdi_display, xpm, 0, 0, image_w, image_h, AllPlanes,
                    ZPixmap);
                X11DRV_check_error();
                XFreePixmap(gdi_display, xpm);
            }
            if(!image) goto no_image;

            image->red_mask = visual->red_mask;
            image->green_mask = visual->green_mask;
            image->blue_mask = visual->blue_mask;

            offset = xoff = yoff = 0;
            for(idx = 0; idx < count; idx++) {
                SmoothGlyphGray(image, xoff + image_off_x - extents.left,
                    yoff + image_off_y - extents.top,
                    formatEntry->bitmaps[wstr[idx]],
                    &formatEntry->gis[wstr[idx]],
                    physDev->textPixel);
                if(lpDx) {
                    offset += lpDx[idx];
                    xoff = offset * cosEsc;
                    yoff = offset * -sinEsc;
                } else {
                    xoff += formatEntry->gis[wstr[idx]].xOff;
                    yoff += formatEntry->gis[wstr[idx]].yOff;
                }
            }
            XPutImage(gdi_display, physDev->drawable, physDev->gc, image, 0, 0,
                image_x, image_y, image_w, image_h);
            XDestroyImage(image);
        }
no_image:
        wine_tsx11_unlock();
    }
#endif
    //LeaveCriticalSection(&xrender_cs);

    if (flags & ETO_CLIPPED)
    {
        /* restore the device region */
#if 0
        RosDrv_SetDeviceClipping( physDev, saved_region, 0 );
        DeleteObject( saved_region );
#endif
    }

    retv = TRUE;

done_unlock:
    //RosDrv_UnlockDIBSection( physDev, TRUE );
    return retv;
}

/* EOF */
