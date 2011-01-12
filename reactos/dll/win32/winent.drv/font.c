/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/font.c
 * PURPOSE:         Font Engine support functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  Some code is taken from winex11.drv (c) Wine project
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
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

static BOOL get_gasp_flags(PDC_ATTR pdcattr, WORD *flags)
{
    DWORD size;
    WORD *gasp, *buffer;
    WORD num_recs;
    DWORD ppem;
    TEXTMETRICW tm;

    *flags = 0;

    size = GetFontData(pdcattr->hdc, MS_GASP_TAG,  0, NULL, 0);
    if(size == GDI_ERROR)
        return FALSE;

    gasp = buffer = HeapAlloc(GetProcessHeap(), 0, size);
    GetFontData(pdcattr->hdc, MS_GASP_TAG,  0, gasp, size);

    GetTextMetricsW(pdcattr->hdc, &tm);
    ppem = abs(RosDrv_YWStoDS(pdcattr, tm.tmAscent + tm.tmDescent - tm.tmInternalLeading));

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

static AA_Type get_antialias_type( PDC_ATTR pdcattr, BOOL subpixel, BOOL hinter)
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
    else if (!hinter || !get_gasp_flags(pdcattr, &flags) || flags & GASP_DOGRAY)
        ret = AA_Grey;
    else
        ret = AA_None;

    return ret;
}

static int GetCacheEntry(PDC_ATTR pdcattr, LFANDSIZE *plfsz)
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
                entry->aa_default = get_antialias_type( pdcattr, FALSE, hinter );
                break;
            case CLEARTYPE_QUALITY:
            case CLEARTYPE_NATURAL_QUALITY:
                entry->aa_default = get_antialias_type( pdcattr, subpixel, hinter );
                break;
            case DEFAULT_QUALITY:
            case DRAFT_QUALITY:
            case PROOF_QUALITY:
            default:
                if ( SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0) &&
                     font_smoothing)
                {
                    entry->aa_default = get_antialias_type( pdcattr, subpixel, hinter );
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
static BOOL UploadGlyph(PDC_ATTR pdcattr, int glyph, AA_Type format)
{
    unsigned int buflen;
    char *buf;
    //Glyph gid;
    GLYPHMETRICS gm;
    GlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + pdcattr->cache_index;
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

    buflen = GetGlyphOutlineW(pdcattr->hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
    if(buflen == GDI_ERROR) {
        if(format != AA_None) {
            format = AA_None;
            entry->aa_default = AA_None;
            ggo_format = GGO_GLYPH_INDEX | GGO_BITMAP;
            buflen = GetGlyphOutlineW(pdcattr->hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
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
    GetGlyphOutlineW(pdcattr->hdc, glyph, ggo_format, &gm, buflen, buf, &identity);
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
FeSelectFont(PDC_ATTR pdcattr, HFONT hfont)
{
    LFANDSIZE lfsz;

    GetObjectW(hfont, sizeof(lfsz.lf), &lfsz.lf);
    TRACE("h=%d w=%d weight=%d it=%d charset=%d name=%s\n",
        lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
        lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.lf.lfWidth = abs( lfsz.lf.lfWidth );
    lfsz.devsize.cx = RosDrv_XWStoDS( pdcattr, lfsz.lf.lfWidth );
    lfsz.devsize.cy = RosDrv_YWStoDS( pdcattr, lfsz.lf.lfHeight );
    GetWorldTransform( pdcattr->hdc, &lfsz.xform );
    lfsz_calc_hash(&lfsz);

    /*EnterCriticalSection(&xrender_cs);*/
    if (pdcattr->cache_index != -1)
        dec_ref_cache(pdcattr->cache_index);
    pdcattr->cache_index = GetCacheEntry(pdcattr, &lfsz);
    /*LeaveCriticalSection(&xrender_cs);*/
}

BOOL FeTextOut( PDC_ATTR pdcattr, INT x, INT y, UINT flags,
               const RECT *lprect, LPCWSTR wstr, UINT count,
               const INT *lpDx )
{
    //RGNDATA *data;
    //XGCValues xgcval;
    gsCacheEntry *entry;
    gsCacheEntryFormat *formatEntry;
    BOOL retv = FALSE;
    //HDC hdc = pdcattr->hdc;
    //int textPixel, backgroundPixel;
    HRGN saved_region = 0;
    BOOL disable_antialias = FALSE;
    AA_Type aa_type = AA_None;
    //DIBSECTION bmp;
    unsigned int idx;
    RECT rect;
    //enum drawable_depth_type depth_type = (pdcattr->depth == 1) ? mono_drawable : color_drawable;
    //Picture tile_pict = 0;

    /* Do we need to disable antialiasing because of palette mode? */
#if 0
    if( !pdcattr->bitmap || GetObjectW( pdcattr->bitmap->hbitmap, sizeof(bmp), &bmp ) != sizeof(bmp) ) {
        TRACE("bitmap is not a DIB\n");
    }
    else if (bmp.dsBmih.biBitCount <= 8) {
        TRACE("Disabling antialiasing\n");
        disable_antialias = TRUE;
    }
#endif

    //RosDrv_LockDIBSection( pdcattr, DIB_Status_GdiMod );

#if 0
    if(pdcattr->depth == 1) {
        if((pdcattr->textPixel & 0xffffff) == 0) {
            textPixel = 0;
            backgroundPixel = 1;
        } else {
            textPixel = 1;
            backgroundPixel = 0;
        }
    } else {
        textPixel = pdcattr->textPixel;
        backgroundPixel = pdcattr->backgroundPixel;
    }
#endif

    if((flags & ETO_OPAQUE) && lprect)
    {
        HBRUSH brush, oldBrush;
        HPEN pen, oldPen;

        brush = CreateSolidBrush(GetBkColor(pdcattr->hdc));
        oldBrush = SelectObject(pdcattr->hdc, brush);

        pen = CreatePen(PS_NULL, 0, 0);
        oldPen = SelectObject(pdcattr->hdc, pen);

        CopyRect(&rect, lprect);
        OffsetRect(&rect, pdcattr->dc_rect.left, pdcattr->dc_rect.top);
        RosGdiRectangle(pdcattr->hKernelDC, &rect);

        DeleteObject(SelectObject(pdcattr->hdc, oldBrush));
        DeleteObject(SelectObject(pdcattr->hdc, oldPen));
    }

    if(count == 0)
    {
        retv = TRUE;
        goto done_unlock;
    }

    if (flags & ETO_CLIPPED)
    {
        HRGN clip_region;

        clip_region = CreateRectRgnIndirect( lprect );
        /* make a copy of the current device region */
        saved_region = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( saved_region, pdcattr->region, 0, RGN_COPY );
        RosDrv_SetDeviceClipping( pdcattr, saved_region, clip_region );
        DeleteObject( clip_region );
    }

    //EnterCriticalSection(&xrender_cs);

    entry = glyphsetCache + pdcattr->cache_index;
    if( disable_antialias == FALSE )
        aa_type = entry->aa_default;
    formatEntry = entry->format[aa_type];

    for(idx = 0; idx < count; idx++) {
        if( !formatEntry ) {
            UploadGlyph(pdcattr, wstr[idx], aa_type);
            /* re-evaluate antialias since aa_default may have changed */
            if( disable_antialias == FALSE )
                aa_type = entry->aa_default;
            formatEntry = entry->format[aa_type];
        } else if( wstr[idx] >= formatEntry->nrealized || formatEntry->realized[wstr[idx]] == FALSE) {
            UploadGlyph(pdcattr, wstr[idx], aa_type);
        }
    }
    if (!formatEntry)
    {
        WARN("could not upload requested glyphs\n");
        //LeaveCriticalSection(&xrender_cs);
        goto done_unlock;
    }

    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count),
        pdcattr->dc_rect.left + x, pdcattr->dc_rect.top + y);

    RosGdiExtTextOut(pdcattr->hKernelDC, pdcattr->dc_rect.left + x, pdcattr->dc_rect.top + y, flags, lprect, wstr, count, lpDx, formatEntry, aa_type);

    //LeaveCriticalSection(&xrender_cs);

    if (flags & ETO_CLIPPED)
    {
        /* restore the device region */
        RosDrv_SetDeviceClipping( pdcattr, saved_region, 0 );
        DeleteObject( saved_region );
    }

    retv = TRUE;

done_unlock:
    //RosDrv_UnlockDIBSection( pdcattr, TRUE );
    return retv;
}

BOOL CDECL RosDrv_ExtTextOut( PDC_ATTR pdcattr, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx )
{
    //if (pdcattr->has_gdi_font)
        return FeTextOut(pdcattr, x, y, flags, lprect, wstr, count, lpDx);

    //UNIMPLEMENTED;
    //return FALSE;
}

BOOL CDECL RosDrv_GetTextExtentExPoint( PDC_ATTR pdcattr, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_GetTextMetrics(PDC_ATTR pdcattr, TEXTMETRICW *metrics)
{
    /* Let GDI font engine do the work */
    return FALSE;
}

BOOL CDECL RosDrv_EnumDeviceFonts( PDC_ATTR pdcattr, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    /* We're always using client-side fonts. */
    return FALSE;
}

HFONT CDECL RosDrv_SelectFont( PDC_ATTR pdcattr, HFONT hfont, HANDLE gdiFont )
{
    /* We don't have a kernelmode font engine */
    if (gdiFont == 0)
    {
        /*RosGdiSelectFont(pdcattr->hKernelDC, hfont, gdiFont);*/
    }
    else
    {
        /* Save information about the selected font */
        FeSelectFont(pdcattr, hfont);
    }

    /* Indicate that gdiFont is good to use */
    return 0;
}

/* EOF */
