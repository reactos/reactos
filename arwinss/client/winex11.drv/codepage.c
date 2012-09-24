/*
 * X11 codepage handling
 *
 * Copyright 2000 Hidenori Takeshima <hidenori@a2.ctktv.ne.jp>
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

#include "config.h"

#include <math.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "x11font.h"

/***********************************************************************
 *           IsLegalDBCSChar for cp932/936/949/950/euc
 */
static inline
int IsLegalDBCSChar_cp932( BYTE lead, BYTE trail )
{
    return ( ( ( lead >= (BYTE)0x81 && lead <= (BYTE)0x9f ) ||
	       ( lead >= (BYTE)0xe0 && lead <= (BYTE)0xfc ) ) &&
	     ( ( trail >= (BYTE)0x40 && trail <= (BYTE)0x7e ) ||
	       ( trail >= (BYTE)0x80 && trail <= (BYTE)0xfc ) ) );
}

static inline
int IsLegalDBCSChar_cp936( BYTE lead, BYTE trail )
{
    return ( ( lead >= (BYTE)0x81 && lead <= (BYTE)0xfe ) &&
	     ( trail >= (BYTE)0x40 && trail <= (BYTE)0xfe ) );
}

static inline
int IsLegalDBCSChar_cp949( BYTE lead, BYTE trail )
{
    return ( ( lead >= (BYTE)0x81 && lead <= (BYTE)0xfe ) &&
	     ( trail >= (BYTE)0x41 && trail <= (BYTE)0xfe ) );
}

static inline
int IsLegalDBCSChar_cp950( BYTE lead, BYTE trail )
{
    return (   ( lead >= (BYTE)0x81 && lead <= (BYTE)0xfe ) &&
	     ( ( trail >= (BYTE)0x40 && trail <= (BYTE)0x7e ) ||
	       ( trail >= (BYTE)0xa1 && trail <= (BYTE)0xfe ) ) );
}


/***********************************************************************
 *           DBCSCharToXChar2b for cp932/euc
 */

static inline
void DBCSCharToXChar2b_cp932( XChar2b* pch, BYTE lead, BYTE trail )
{
    unsigned int  high, low;

    high = (unsigned int)lead;
    low = (unsigned int)trail;

    if ( high <= 0x9f )
	high = (high<<1) - 0xe0;
    else
	high = (high<<1) - 0x160;
    if ( low < 0x9f )
    {
	high --;
	if ( low < 0x7f )
	    low -= 0x1f;
	else
	    low -= 0x20;
    }
    else
    {
	low -= 0x7e;
    }

    pch->byte1 = (unsigned char)high;
    pch->byte2 = (unsigned char)low;
}


static WORD X11DRV_enum_subfont_charset_normal( UINT index )
{
    return DEFAULT_CHARSET;
}

static WORD X11DRV_enum_subfont_charset_cp932( UINT index )
{
    switch ( index )
    {
    case 0: return X11FONT_JISX0201_CHARSET;
    case 1: return X11FONT_JISX0212_CHARSET;
    }

    return DEFAULT_CHARSET;
}

static WORD X11DRV_enum_subfont_charset_cp936( UINT index )
{
    switch ( index )
    {
    case 0: return ANSI_CHARSET;
    }

    return DEFAULT_CHARSET;
}

static WORD X11DRV_enum_subfont_charset_cp949( UINT index )
{
    switch ( index )
    {
    case 0: return ANSI_CHARSET;
    }

    return DEFAULT_CHARSET;
}

static WORD X11DRV_enum_subfont_charset_cp950( UINT index )
{
    switch ( index )
    {
    case 0: return ANSI_CHARSET;
    }

    return DEFAULT_CHARSET;
}


static XChar2b* X11DRV_unicode_to_char2b_sbcs( fontObject* pfo,
                                               LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    UINT i;
    char *str;
    UINT codepage = pfo->fi->codepage;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }

    WideCharToMultiByte( codepage, 0, lpwstr, count, str, count, &ch, NULL );

    for (i = 0; i < count; i++)
    {
	str2b[i].byte1 = 0;
	str2b[i].byte2 = str[i];
    }
    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}

static XChar2b* X11DRV_unicode_to_char2b_unicode( fontObject* pfo,
                                                  LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    UINT i;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;

    for (i = 0; i < count; i++)
    {
	str2b[i].byte1 = lpwstr[i] >> 8;
	str2b[i].byte2 = lpwstr[i] & 0xff;
    }

    return str2b;
}

/* FIXME: handle jisx0212.1990... */
static XChar2b* X11DRV_unicode_to_char2b_cp932( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    char *str;
    BYTE *str_src;
    UINT i;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }

    /* handle jisx0212.1990... */
    WideCharToMultiByte( 932, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = (BYTE*) str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( IsLegalDBCSChar_cp932( *str_src, *(str_src+1) ) )
	{
	    DBCSCharToXChar2b_cp932( str2b_dst, *str_src, *(str_src+1) );
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}


static XChar2b* X11DRV_unicode_to_char2b_cp936( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    char *str;
    BYTE *str_src;
    UINT i;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }
    WideCharToMultiByte( 936, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = (BYTE*) str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( IsLegalDBCSChar_cp936( *str_src, *(str_src+1) ) )
	{
	    str2b_dst->byte1 = *str_src;
	    str2b_dst->byte2 = *(str_src+1);
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}

static XChar2b* X11DRV_unicode_to_char2b_cp949( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    char *str;
    BYTE *str_src;
    UINT i;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }
    WideCharToMultiByte( 949, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = (BYTE*) str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( IsLegalDBCSChar_cp949( *str_src, *(str_src+1) ) )
	{
            str2b_dst->byte1 = *str_src;
	    str2b_dst->byte2 = *(str_src+1);
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}


static XChar2b* X11DRV_unicode_to_char2b_cp950( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    char *str;
    BYTE *str_src;
    UINT i;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }
    WideCharToMultiByte( 950, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = (BYTE*) str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( IsLegalDBCSChar_cp950( *str_src, *(str_src+1) ) )
	{
            str2b_dst->byte1 = *str_src;
	    str2b_dst->byte2 = *(str_src+1);
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}

static XChar2b* X11DRV_unicode_to_char2b_symbol( fontObject* pfo,
						 LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    UINT i;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;

    for (i = 0; i < count; i++)
    {
	str2b[i].byte1 = 0;
	if(lpwstr[i] >= 0xf000 && lpwstr[i] < 0xf100)
	    str2b[i].byte2 = lpwstr[i] - 0xf000;
	else if(lpwstr[i] < 0x100)
	    str2b[i].byte2 = lpwstr[i];
	else
	    str2b[i].byte2 = ch;
    }

    return str2b;
}


static void X11DRV_DrawString_normal( fontObject* pfo, Display* pdisp,
                                      Drawable d, GC gc, int x, int y,
                                      XChar2b* pstr, int count )
{
    wine_tsx11_lock();
    XDrawString16( pdisp, d, gc, x, y, pstr, count );
    wine_tsx11_unlock();
}

static int X11DRV_TextWidth_normal( fontObject* pfo, XChar2b* pstr, int count )
{
    int ret;
    wine_tsx11_lock();
    ret = XTextWidth16( pfo->fs, pstr, count );
    wine_tsx11_unlock();
    return ret;
}

static void X11DRV_DrawText_normal( fontObject* pfo, Display* pdisp, Drawable d,
                                    GC gc, int x, int y, XTextItem16* pitems,
                                    int count )
{
    wine_tsx11_lock();
    XDrawText16( pdisp, d, gc, x, y, pitems, count );
    wine_tsx11_unlock();
}

static void X11DRV_TextExtents_normal( fontObject* pfo, XChar2b* pstr, int count,
                                       int* pdir, int* pascent, int* pdescent,
                                       int* pwidth, int max_extent, int* pfit,
                                       int* partial_extents )
{
    XCharStruct info;
    int ascent, descent, width;
    int i, fit;

    width = 0;
    fit = 0;
    *pascent = 0;
    *pdescent = 0;
    wine_tsx11_lock();
    for ( i = 0; i < count; i++ )
    {
	XTextExtents16( pfo->fs, pstr, 1, pdir, &ascent, &descent, &info );
	if ( *pascent < ascent ) *pascent = ascent;
	if ( *pdescent < descent ) *pdescent = descent;
	width += info.width;
	if ( partial_extents ) partial_extents[i] = width;
	if ( width < max_extent ) fit++;

	pstr++;
    }
    wine_tsx11_unlock();
    *pwidth = width;
    if ( pfit ) *pfit = fit;
}

static void X11DRV_GetTextMetricsW_normal( fontObject* pfo, LPTEXTMETRICW pTM )
{
    LPIFONTINFO16 pdf = &pfo->fi->df;

    if( ! pfo->lpX11Trans ) {
      pTM->tmAscent = pfo->fs->ascent;
      pTM->tmDescent = pfo->fs->descent;
    } else {
      pTM->tmAscent = pfo->lpX11Trans->ascent;
      pTM->tmDescent = pfo->lpX11Trans->descent;
    }

    pTM->tmAscent *= pfo->rescale;
    pTM->tmDescent *= pfo->rescale;

    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    pTM->tmAveCharWidth = pfo->foAvgCharWidth * pfo->rescale;
    pTM->tmMaxCharWidth = pfo->foMaxCharWidth * pfo->rescale;

    pTM->tmInternalLeading = pfo->foInternalLeading * pfo->rescale;
    pTM->tmExternalLeading = pdf->dfExternalLeading * pfo->rescale;

    pTM->tmStruckOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT )
			? 1 : pdf->dfStrikeOut;
    pTM->tmUnderlined = (pfo->fo_flags & FO_SYNTH_UNDERLINE )
			? 1 : pdf->dfUnderline;

    pTM->tmOverhang = 0;
    if( pfo->fo_flags & FO_SYNTH_ITALIC )
    {
	pTM->tmOverhang += pTM->tmHeight/3;
	pTM->tmItalic = 1;
    } else
	pTM->tmItalic = pdf->dfItalic;

    pTM->tmWeight = pdf->dfWeight;
    if( pfo->fo_flags & FO_SYNTH_BOLD )
    {
	pTM->tmOverhang++;
	pTM->tmWeight += 100;
    }

    pTM->tmFirstChar = pdf->dfFirstChar;
    pTM->tmLastChar = pdf->dfLastChar;
    pTM->tmDefaultChar = pdf->dfDefaultChar;
    pTM->tmBreakChar = pdf->dfBreakChar;

    pTM->tmCharSet = pdf->dfCharSet;
    pTM->tmPitchAndFamily = pdf->dfPitchAndFamily;

    pTM->tmDigitizedAspectX = pdf->dfHorizRes;
    pTM->tmDigitizedAspectY = pdf->dfVertRes;
}



static
void X11DRV_DrawString_dbcs( fontObject* pfo, Display* pdisp,
                             Drawable d, GC gc, int x, int y,
                             XChar2b* pstr, int count )
{
    XTextItem16 item;

    item.chars = pstr;
    item.delta = 0;
    item.nchars = count;
    item.font = None;
    X11DRV_cptable[pfo->fi->cptable].pDrawText(
		pfo, pdisp, d, gc, x, y, &item, 1 );
}

static
int X11DRV_TextWidth_dbcs_2fonts( fontObject* pfo, XChar2b* pstr, int count )
{
    int i;
    int width;
    int curfont;
    fontObject* pfos[X11FONT_REFOBJS_MAX+1];

    pfos[0] = XFONT_GetFontObject( pfo->prefobjs[0] );
    pfos[1] = pfo;
    if ( pfos[0] == NULL ) pfos[0] = pfo;

    width = 0;
    wine_tsx11_lock();
    for ( i = 0; i < count; i++ )
    {
	curfont = ( pstr->byte1 != 0 ) ? 1 : 0;
	width += XTextWidth16( pfos[curfont]->fs, pstr, 1 );
	pstr ++;
    }
    wine_tsx11_unlock();
    return width;
}

static
void X11DRV_DrawText_dbcs_2fonts( fontObject* pfo, Display* pdisp, Drawable d,
                                  GC gc, int x, int y, XTextItem16* pitems,
                                  int count )
{
    int i, nitems, prevfont = -1, curfont;
    XChar2b* pstr;
    XTextItem16* ptibuf;
    XTextItem16* pti;
    fontObject* pfos[X11FONT_REFOBJS_MAX+1];

    pfos[0] = XFONT_GetFontObject( pfo->prefobjs[0] );
    pfos[1] = pfo;
    if ( pfos[0] == NULL ) pfos[0] = pfo;

    nitems = 0;
    for ( i = 0; i < count; i++ )
	nitems += pitems->nchars;
    ptibuf = HeapAlloc( GetProcessHeap(), 0, sizeof(XTextItem16) * nitems );
    if ( ptibuf == NULL )
	return; /* out of memory */

    pti = ptibuf;
    while ( count-- > 0 )
    {
	pti->chars = pstr = pitems->chars;
	pti->delta = pitems->delta;
	pti->font = None;
	for ( i = 0; i < pitems->nchars; i++, pstr++ )
	{
	    curfont = ( pstr->byte1 != 0 ) ? 1 : 0;
	    if ( curfont != prevfont )
	    {
		if ( pstr != pti->chars )
		{
		    pti->nchars = pstr - pti->chars;
		    pti ++;
		    pti->chars = pstr;
		    pti->delta = 0;
		}
		pti->font = pfos[curfont]->fs->fid;
		prevfont = curfont;
	    }
	}
	pti->nchars = pstr - pti->chars;
	pitems ++; pti ++;
    }
    wine_tsx11_lock();
    XDrawText16( pdisp, d, gc, x, y, ptibuf, pti - ptibuf );
    wine_tsx11_unlock();
    HeapFree( GetProcessHeap(), 0, ptibuf );
}

static
void X11DRV_TextExtents_dbcs_2fonts( fontObject* pfo, XChar2b* pstr, int count,
                                     int* pdir, int* pascent, int* pdescent,
                                     int* pwidth, int max_extent, int* pfit,
                                     int* partial_extents )
{
    XCharStruct info;
    int ascent, descent, width;
    int i;
    int fit;
    int curfont;
    fontObject* pfos[X11FONT_REFOBJS_MAX+1];

    pfos[0] = XFONT_GetFontObject( pfo->prefobjs[0] );
    pfos[1] = pfo;
    if ( pfos[0] == NULL ) pfos[0] = pfo;

    width = 0;
    fit = 0;
    *pascent = 0;
    *pdescent = 0;
    wine_tsx11_lock();
    for ( i = 0; i < count; i++ )
    {
	curfont = ( pstr->byte1 != 0 ) ? 1 : 0;
	XTextExtents16( pfos[curfont]->fs, pstr, 1, pdir, &ascent, &descent, &info );
	if ( *pascent < ascent ) *pascent = ascent;
	if ( *pdescent < descent ) *pdescent = descent;
	width += info.width;
	if ( partial_extents ) partial_extents[i] = width;
	if ( width <= max_extent ) fit++;

	pstr ++;
    }
    wine_tsx11_unlock();
    *pwidth = width;
    if ( pfit ) *pfit = fit;
}

static void X11DRV_GetTextMetricsW_cp932( fontObject* pfo, LPTEXTMETRICW pTM )
{
    fontObject* pfo_ansi = XFONT_GetFontObject( pfo->prefobjs[0] );
    LPIFONTINFO16 pdf = &pfo->fi->df;
    LPIFONTINFO16 pdf_ansi;

    pdf_ansi = ( pfo_ansi != NULL ) ? (&pfo_ansi->fi->df) : pdf;

    if( ! pfo->lpX11Trans ) {
      pTM->tmAscent = pfo->fs->ascent;
      pTM->tmDescent = pfo->fs->descent;
    } else {
      pTM->tmAscent = pfo->lpX11Trans->ascent;
      pTM->tmDescent = pfo->lpX11Trans->descent;
    }

    pTM->tmAscent *= pfo->rescale;
    pTM->tmDescent *= pfo->rescale;

    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    if ( pfo_ansi != NULL )
    {
	pTM->tmAveCharWidth = floor((pfo_ansi->foAvgCharWidth * 2.0 + pfo->foAvgCharWidth) / 3.0 * pfo->rescale + 0.5);
	pTM->tmMaxCharWidth = max(pfo_ansi->foMaxCharWidth, pfo->foMaxCharWidth) * pfo->rescale;
    }
    else
    {
	pTM->tmAveCharWidth = floor((pfo->foAvgCharWidth * pfo->rescale + 1.0) / 2.0);
	pTM->tmMaxCharWidth = pfo->foMaxCharWidth * pfo->rescale;
    }

    pTM->tmInternalLeading = pfo->foInternalLeading * pfo->rescale;
    pTM->tmExternalLeading = pdf->dfExternalLeading * pfo->rescale;

    pTM->tmStruckOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT )
			? 1 : pdf->dfStrikeOut;
    pTM->tmUnderlined = (pfo->fo_flags & FO_SYNTH_UNDERLINE )
			? 1 : pdf->dfUnderline;

    pTM->tmOverhang = 0;
    if( pfo->fo_flags & FO_SYNTH_ITALIC )
    {
	pTM->tmOverhang += pTM->tmHeight/3;
	pTM->tmItalic = 1;
    } else
	pTM->tmItalic = pdf->dfItalic;

    pTM->tmWeight = pdf->dfWeight;
    if( pfo->fo_flags & FO_SYNTH_BOLD )
    {
	pTM->tmOverhang++;
	pTM->tmWeight += 100;
    }

    pTM->tmFirstChar = pdf_ansi->dfFirstChar;
    pTM->tmLastChar = pdf_ansi->dfLastChar;
    pTM->tmDefaultChar = pdf_ansi->dfDefaultChar;
    pTM->tmBreakChar = pdf_ansi->dfBreakChar;

    pTM->tmCharSet = pdf->dfCharSet;
    pTM->tmPitchAndFamily = pdf->dfPitchAndFamily;

    pTM->tmDigitizedAspectX = pdf->dfHorizRes;
    pTM->tmDigitizedAspectY = pdf->dfVertRes;
}





const X11DRV_CP X11DRV_cptable[X11DRV_CPTABLE_COUNT] =
{
    { /* SBCS */
	X11DRV_enum_subfont_charset_normal,
	X11DRV_unicode_to_char2b_sbcs,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
	X11DRV_GetTextMetricsW_normal,
    },
    { /* UNICODE */
	X11DRV_enum_subfont_charset_normal,
	X11DRV_unicode_to_char2b_unicode,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
        X11DRV_GetTextMetricsW_normal,
    },
    { /* CP932 */
	X11DRV_enum_subfont_charset_cp932,
	X11DRV_unicode_to_char2b_cp932,
	X11DRV_DrawString_dbcs,
	X11DRV_TextWidth_dbcs_2fonts,
	X11DRV_DrawText_dbcs_2fonts,
	X11DRV_TextExtents_dbcs_2fonts,
        X11DRV_GetTextMetricsW_cp932,
    },
    { /* CP936 */
	X11DRV_enum_subfont_charset_cp936,
	X11DRV_unicode_to_char2b_cp936,
	X11DRV_DrawString_dbcs,
	X11DRV_TextWidth_dbcs_2fonts,
	X11DRV_DrawText_dbcs_2fonts,
	X11DRV_TextExtents_dbcs_2fonts,
        X11DRV_GetTextMetricsW_normal, /* FIXME */
    },
    { /* CP949 */
	X11DRV_enum_subfont_charset_cp949,
	X11DRV_unicode_to_char2b_cp949,
	X11DRV_DrawString_dbcs,
	X11DRV_TextWidth_dbcs_2fonts,
	X11DRV_DrawText_dbcs_2fonts,
	X11DRV_TextExtents_dbcs_2fonts,
        X11DRV_GetTextMetricsW_normal, /* FIXME */
    },
    { /* CP950 */
	X11DRV_enum_subfont_charset_cp950,
	X11DRV_unicode_to_char2b_cp950,
	X11DRV_DrawString_dbcs,
	X11DRV_TextWidth_dbcs_2fonts,
	X11DRV_DrawText_dbcs_2fonts,
	X11DRV_TextExtents_dbcs_2fonts,
        X11DRV_GetTextMetricsW_cp932,
    },
    { /* SYMBOL */
	X11DRV_enum_subfont_charset_normal,
	X11DRV_unicode_to_char2b_symbol,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
	X11DRV_GetTextMetricsW_normal,
    }
};
