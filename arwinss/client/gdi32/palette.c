/*
 * GDI palette objects
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1996 Alex Korobka
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
 *
 * NOTES:
 * PALETTEOBJ is documented in the Dr. Dobbs Journal May 1993.
 * Information in the "Undocumented Windows" is incorrect.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(palette);

typedef struct tagPALETTEOBJ
{
    GDIOBJHDR           header;
    const DC_FUNCTIONS *funcs;      /* DC function table */
    WORD                version;    /* palette version */
    WORD                count;      /* count of palette entries */
    PALETTEENTRY       *entries;
} PALETTEOBJ;

static INT PALETTE_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle );
static BOOL PALETTE_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs palette_funcs =
{
    NULL,                     /* pSelectObject */
    PALETTE_GetObject,        /* pGetObjectA */
    PALETTE_GetObject,        /* pGetObjectW */
    PALETTE_UnrealizeObject,  /* pUnrealizeObject */
    PALETTE_DeleteObject      /* pDeleteObject */
};

/* Pointers to USER implementation of SelectPalette/RealizePalette */
/* they will be patched by USER on startup */
HPALETTE (WINAPI *pfnSelectPalette)(HDC hdc, HPALETTE hpal, WORD bkgnd ) = GDISelectPalette;
UINT (WINAPI *pfnRealizePalette)(HDC hdc) = GDIRealizePalette;

static UINT SystemPaletteUse = SYSPAL_STATIC;  /* currently not considered */

static HPALETTE hPrimaryPalette = 0; /* used for WM_PALETTECHANGED */
static HPALETTE hLastRealizedPalette = 0; /* UnrealizeObject() needs it */

#define NB_RESERVED_COLORS  20   /* number of fixed colors in system palette */

static const PALETTEENTRY sys_pal_template[NB_RESERVED_COLORS] =
{
    /* first 10 entries in the system palette */
    /* red  green blue  flags */
    { 0x00, 0x00, 0x00, 0 },
    { 0x80, 0x00, 0x00, 0 },
    { 0x00, 0x80, 0x00, 0 },
    { 0x80, 0x80, 0x00, 0 },
    { 0x00, 0x00, 0x80, 0 },
    { 0x80, 0x00, 0x80, 0 },
    { 0x00, 0x80, 0x80, 0 },
    { 0xc0, 0xc0, 0xc0, 0 },
    { 0xc0, 0xdc, 0xc0, 0 },
    { 0xa6, 0xca, 0xf0, 0 },

    /* ... c_min/2 dynamic colorcells */

    /* ... gap (for sparse palettes) */

    /* ... c_min/2 dynamic colorcells */

    { 0xff, 0xfb, 0xf0, 0 },
    { 0xa0, 0xa0, 0xa4, 0 },
    { 0x80, 0x80, 0x80, 0 },
    { 0xff, 0x00, 0x00, 0 },
    { 0x00, 0xff, 0x00, 0 },
    { 0xff, 0xff, 0x00, 0 },
    { 0x00, 0x00, 0xff, 0 },
    { 0xff, 0x00, 0xff, 0 },
    { 0x00, 0xff, 0xff, 0 },
    { 0xff, 0xff, 0xff, 0 }     /* last 10 */
};

/***********************************************************************
 *           PALETTE_Init
 *
 * Create the system palette.
 */
HPALETTE PALETTE_Init(void)
{
    HPALETTE          hpalette;
    LOGPALETTE *        palPtr;

    /* create default palette (20 system colors) */

    palPtr = HeapAlloc( GetProcessHeap(), 0,
             sizeof(LOGPALETTE) + (NB_RESERVED_COLORS-1)*sizeof(PALETTEENTRY));
    if (!palPtr) return FALSE;

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    memcpy( palPtr->palPalEntry, sys_pal_template, sizeof(sys_pal_template) );
    hpalette = CreatePalette( palPtr );
    HeapFree( GetProcessHeap(), 0, palPtr );
    return hpalette;
}


/***********************************************************************
 * CreatePalette [GDI32.@]
 *
 * Creates a logical color palette.
 *
 * RETURNS
 *    Success: Handle to logical palette
 *    Failure: NULL
 */
HPALETTE WINAPI CreatePalette(
    const LOGPALETTE* palette) /* [in] Pointer to logical color palette */
{
    PALETTEOBJ * palettePtr;
    HPALETTE hpalette;
    int size;

    if (!palette) return 0;
    TRACE("entries=%i\n", palette->palNumEntries);

    size = sizeof(LOGPALETTE) + (palette->palNumEntries - 1) * sizeof(PALETTEENTRY);

    if (!(palettePtr = HeapAlloc( GetProcessHeap(), 0, sizeof(*palettePtr) ))) return 0;
    palettePtr->funcs   = NULL;
    palettePtr->version = palette->palVersion;
    palettePtr->count   = palette->palNumEntries;
    size = palettePtr->count * sizeof(*palettePtr->entries);
    if (!(palettePtr->entries = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        HeapFree( GetProcessHeap(), 0, palettePtr );
        return 0;
    }
    memcpy( palettePtr->entries, palette->palPalEntry, size );
    if (!(hpalette = alloc_gdi_handle( &palettePtr->header, OBJ_PAL, &palette_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, palettePtr->entries );
        HeapFree( GetProcessHeap(), 0, palettePtr );
    }
    TRACE("   returning %p\n", hpalette);
    return hpalette;
}


/***********************************************************************
 * CreateHalftonePalette [GDI32.@]
 *
 * Creates a halftone palette.
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 *
 * FIXME: This simply creates the halftone palette derived from running
 *        tests on a windows NT machine. This is assuming a color depth
 *        of greater that 256 color. On a 256 color device the halftone
 *        palette will be different and this function will be incorrect
 */
HPALETTE WINAPI CreateHalftonePalette(
    HDC hdc) /* [in] Handle to device context */
{
    int i;
    struct {
	WORD Version;
	WORD NumberOfEntries;
	PALETTEENTRY aEntries[256];
    } Palette;

    Palette.Version = 0x300;
    Palette.NumberOfEntries = 256;
    GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

    Palette.NumberOfEntries = 20;

    for (i = 0; i < Palette.NumberOfEntries; i++)
    {
        Palette.aEntries[i].peRed=0xff;
        Palette.aEntries[i].peGreen=0xff;
        Palette.aEntries[i].peBlue=0xff;
        Palette.aEntries[i].peFlags=0x00;
    }

    Palette.aEntries[0].peRed=0x00;
    Palette.aEntries[0].peBlue=0x00;
    Palette.aEntries[0].peGreen=0x00;

    /* the first 6 */
    for (i=1; i <= 6; i++)
    {
        Palette.aEntries[i].peRed=(i%2)?0x80:0;
        Palette.aEntries[i].peGreen=(i==2)?0x80:(i==3)?0x80:(i==6)?0x80:0;
        Palette.aEntries[i].peBlue=(i>3)?0x80:0;
    }

    for (i=7;  i <= 12; i++)
    {
        switch(i)
        {
            case 7:
                Palette.aEntries[i].peRed=0xc0;
                Palette.aEntries[i].peBlue=0xc0;
                Palette.aEntries[i].peGreen=0xc0;
                break;
            case 8:
                Palette.aEntries[i].peRed=0xc0;
                Palette.aEntries[i].peGreen=0xdc;
                Palette.aEntries[i].peBlue=0xc0;
                break;
            case 9:
                Palette.aEntries[i].peRed=0xa6;
                Palette.aEntries[i].peGreen=0xca;
                Palette.aEntries[i].peBlue=0xf0;
                break;
            case 10:
                Palette.aEntries[i].peRed=0xff;
                Palette.aEntries[i].peGreen=0xfb;
                Palette.aEntries[i].peBlue=0xf0;
                break;
            case 11:
                Palette.aEntries[i].peRed=0xa0;
                Palette.aEntries[i].peGreen=0xa0;
                Palette.aEntries[i].peBlue=0xa4;
                break;
            case 12:
                Palette.aEntries[i].peRed=0x80;
                Palette.aEntries[i].peGreen=0x80;
                Palette.aEntries[i].peBlue=0x80;
        }
    }

   for (i=13; i <= 18; i++)
    {
        Palette.aEntries[i].peRed=(i%2)?0xff:0;
        Palette.aEntries[i].peGreen=(i==14)?0xff:(i==15)?0xff:(i==18)?0xff:0;
        Palette.aEntries[i].peBlue=(i>15)?0xff:0x00;
    }

    return CreatePalette((LOGPALETTE *)&Palette);
}


/***********************************************************************
 * GetPaletteEntries [GDI32.@]
 *
 * Retrieves palette entries.
 *
 * RETURNS
 *    Success: Number of entries from logical palette
 *    Failure: 0
 */
UINT WINAPI GetPaletteEntries(
    HPALETTE hpalette,    /* [in]  Handle of logical palette */
    UINT start,           /* [in]  First entry to receive */
    UINT count,           /* [in]  Number of entries to receive */
    LPPALETTEENTRY entries) /* [out] Address of array receiving entries */
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal = %p, count=%i\n", hpalette, count );

    palPtr = GDI_GetObjPtr( hpalette, OBJ_PAL );
    if (!palPtr) return 0;

    /* NOTE: not documented but test show this to be the case */
    if (count == 0)
    {
        count = palPtr->count;
    }
    else
    {
        numEntries = palPtr->count;
        if (start+count > numEntries) count = numEntries - start;
        if (entries)
        {
            if (start >= numEntries) count = 0;
            else memcpy( entries, &palPtr->entries[start], count * sizeof(PALETTEENTRY) );
        }
    }

    GDI_ReleaseObj( hpalette );
    return count;
}


/***********************************************************************
 * SetPaletteEntries [GDI32.@]
 *
 * Sets color values for range in palette.
 *
 * RETURNS
 *    Success: Number of entries that were set
 *    Failure: 0
 */
UINT WINAPI SetPaletteEntries(
    HPALETTE hpalette,    /* [in] Handle of logical palette */
    UINT start,           /* [in] Index of first entry to set */
    UINT count,           /* [in] Number of entries to set */
    const PALETTEENTRY *entries) /* [in] Address of array of structures */
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal=%p,start=%i,count=%i\n",hpalette,start,count );

    if (hpalette == GetStockObject(DEFAULT_PALETTE)) return 0;
    palPtr = GDI_GetObjPtr( hpalette, OBJ_PAL );
    if (!palPtr) return 0;

    numEntries = palPtr->count;
    if (start >= numEntries)
    {
      GDI_ReleaseObj( hpalette );
      return 0;
    }
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->entries[start], entries, count * sizeof(PALETTEENTRY) );
    GDI_ReleaseObj( hpalette );
    UnrealizeObject( hpalette );
    return count;
}


/***********************************************************************
 * ResizePalette [GDI32.@]
 *
 * Resizes logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ResizePalette(
    HPALETTE hPal, /* [in] Handle of logical palette */
    UINT cEntries) /* [in] Number of entries in logical palette */
{
    PALETTEOBJ * palPtr = GDI_GetObjPtr( hPal, OBJ_PAL );
    PALETTEENTRY *entries;

    if( !palPtr ) return FALSE;
    TRACE("hpal = %p, prev = %i, new = %i\n", hPal, palPtr->count, cEntries );

    if (!(entries = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 palPtr->entries, cEntries * sizeof(*palPtr->entries) )))
    {
        GDI_ReleaseObj( hPal );
        return FALSE;
    }
    palPtr->entries = entries;
    palPtr->count = cEntries;

    GDI_ReleaseObj( hPal );
    PALETTE_UnrealizeObject( hPal );
    return TRUE;
}


/***********************************************************************
 * AnimatePalette [GDI32.@]
 *
 * Replaces entries in logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * FIXME
 *    Should use existing mapping when animating a primary palette
 */
BOOL WINAPI AnimatePalette(
    HPALETTE hPal,              /* [in] Handle to logical palette */
    UINT StartIndex,            /* [in] First entry in palette */
    UINT NumEntries,            /* [in] Count of entries in palette */
    const PALETTEENTRY* PaletteColors) /* [in] Pointer to first replacement */
{
    TRACE("%p (%i - %i)\n", hPal, StartIndex,StartIndex+NumEntries);

    if( hPal != GetStockObject(DEFAULT_PALETTE) )
    {
        PALETTEOBJ * palPtr;
        UINT pal_entries;
        const PALETTEENTRY *pptr = PaletteColors;
        const DC_FUNCTIONS *funcs;

        palPtr = GDI_GetObjPtr( hPal, OBJ_PAL );
        if (!palPtr) return 0;

        pal_entries = palPtr->count;
        if (StartIndex >= pal_entries)
        {
          GDI_ReleaseObj( hPal );
          return 0;
        }
        if (StartIndex+NumEntries > pal_entries) NumEntries = pal_entries - StartIndex;
        
        for (NumEntries += StartIndex; StartIndex < NumEntries; StartIndex++, pptr++) {
          /* According to MSDN, only animate PC_RESERVED colours */
          if (palPtr->entries[StartIndex].peFlags & PC_RESERVED) {
            TRACE("Animating colour (%d,%d,%d) to (%d,%d,%d)\n",
              palPtr->entries[StartIndex].peRed,
              palPtr->entries[StartIndex].peGreen,
              palPtr->entries[StartIndex].peBlue,
              pptr->peRed, pptr->peGreen, pptr->peBlue);
            palPtr->entries[StartIndex] = *pptr;
          } else {
            TRACE("Not animating entry %d -- not PC_RESERVED\n", StartIndex);
          }
        }
        funcs = palPtr->funcs;
        GDI_ReleaseObj( hPal );
        if (funcs && funcs->pRealizePalette) funcs->pRealizePalette( NULL, hPal, hPal == hPrimaryPalette );
    }
    return TRUE;
}


/***********************************************************************
 * SetSystemPaletteUse [GDI32.@]
 *
 * Specify whether the system palette contains 2 or 20 static colors.
 *
 * RETURNS
 *    Success: Previous system palette
 *    Failure: SYSPAL_ERROR
 */
UINT WINAPI SetSystemPaletteUse(
    HDC hdc,  /* [in] Handle of device context */
    UINT use) /* [in] Palette-usage flag */
{
    UINT old = SystemPaletteUse;

    /* Device doesn't support colour palettes */
    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)) {
        return SYSPAL_ERROR;
    }

    switch (use) {
        case SYSPAL_NOSTATIC:
        case SYSPAL_NOSTATIC256:        /* WINVER >= 0x0500 */
        case SYSPAL_STATIC:
            SystemPaletteUse = use;
            return old;
        default:
            return SYSPAL_ERROR;
    }
}


/***********************************************************************
 * GetSystemPaletteUse [GDI32.@]
 *
 * Gets state of system palette.
 *
 * RETURNS
 *    Current state of system palette
 */
UINT WINAPI GetSystemPaletteUse(
    HDC hdc) /* [in] Handle of device context */
{
    return SystemPaletteUse;
}


/***********************************************************************
 * GetSystemPaletteEntries [GDI32.@]
 *
 * Gets range of palette entries.
 *
 * RETURNS
 *    Success: Number of entries retrieved from palette
 *    Failure: 0
 */
UINT WINAPI GetSystemPaletteEntries(
    HDC hdc,              /* [in]  Handle of device context */
    UINT start,           /* [in]  Index of first entry to be retrieved */
    UINT count,           /* [in]  Number of entries to be retrieved */
    LPPALETTEENTRY entries) /* [out] Array receiving system-palette entries */
{
    UINT ret = 0;
    DC *dc;

    TRACE("hdc=%p,start=%i,count=%i\n", hdc,start,count);

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pGetSystemPaletteEntries)
            ret = dc->funcs->pGetSystemPaletteEntries( dc->physDev, start, count, entries );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 * GetNearestPaletteIndex [GDI32.@]
 *
 * Gets palette index for color.
 *
 * NOTES
 *    Should index be initialized to CLR_INVALID instead of 0?
 *
 * RETURNS
 *    Success: Index of entry in logical palette
 *    Failure: CLR_INVALID
 */
UINT WINAPI GetNearestPaletteIndex(
    HPALETTE hpalette, /* [in] Handle of logical color palette */
    COLORREF color)      /* [in] Color to be matched */
{
    PALETTEOBJ* palObj = GDI_GetObjPtr( hpalette, OBJ_PAL );
    UINT index  = 0;

    if( palObj )
    {
        int i, diff = 0x7fffffff;
        int r,g,b;
        PALETTEENTRY* entry = palObj->entries;

        for( i = 0; i < palObj->count && diff ; i++, entry++)
        {
            r = entry->peRed - GetRValue(color);
            g = entry->peGreen - GetGValue(color);
            b = entry->peBlue - GetBValue(color);

            r = r*r + g*g + b*b;

            if( r < diff ) { index = i; diff = r; }
        }
        GDI_ReleaseObj( hpalette );
    }
    TRACE("(%p,%06x): returning %d\n", hpalette, color, index );
    return index;
}


/***********************************************************************
 * GetNearestColor [GDI32.@]
 *
 * Gets a system color to match.
 *
 * RETURNS
 *    Success: Color from system palette that corresponds to given color
 *    Failure: CLR_INVALID
 */
COLORREF WINAPI GetNearestColor(
    HDC hdc,      /* [in] Handle of device context */
    COLORREF color) /* [in] Color to be matched */
{
    unsigned char spec_type;
    COLORREF nearest;
    DC 		*dc;

    if (!(dc = get_dc_ptr( hdc ))) return CLR_INVALID;

    if (dc->funcs->pGetNearestColor)
    {
        nearest = dc->funcs->pGetNearestColor( dc->physDev, color );
        release_dc_ptr( dc );
        return nearest;
    }

    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        release_dc_ptr( dc );
        return color;
    }

    spec_type = color >> 24;
    if (spec_type == 1 || spec_type == 2)
    {
        /* we need logical palette for PALETTERGB and PALETTEINDEX colorrefs */

        UINT index;
        PALETTEENTRY entry;
        HPALETTE hpal = dc->hPalette ? dc->hPalette : GetStockObject( DEFAULT_PALETTE );

        if (spec_type == 2) /* PALETTERGB */
            index = GetNearestPaletteIndex( hpal, color );
        else  /* PALETTEINDEX */
            index = LOWORD(color);

        if (!GetPaletteEntries( hpal, index, 1, &entry ))
        {
            WARN("RGB(%x) : idx %d is out of bounds, assuming NULL\n", color, index );
            if (!GetPaletteEntries( hpal, 0, 1, &entry ))
            {
                release_dc_ptr( dc );
                return CLR_INVALID;
            }
        }
        color = RGB( entry.peRed, entry.peGreen, entry.peBlue );
    }
    nearest = color & 0x00ffffff;
    release_dc_ptr( dc );

    TRACE("(%06x): returning %06x\n", color, nearest );
    return nearest;
}


/***********************************************************************
 *           PALETTE_GetObject
 */
static INT PALETTE_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, OBJ_PAL );

    if (!palette) return 0;

    if (buffer)
    {
        if (count > sizeof(WORD)) count = sizeof(WORD);
        memcpy( buffer, &palette->count, count );
    }
    else count = sizeof(WORD);
    GDI_ReleaseObj( handle );
    return count;
}


/***********************************************************************
 *           PALETTE_UnrealizeObject
 */
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle )
{
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, OBJ_PAL );

    if (palette)
    {
        const DC_FUNCTIONS *funcs = palette->funcs;
        palette->funcs = NULL;
        GDI_ReleaseObj( handle );
        if (funcs && funcs->pUnrealizePalette) funcs->pUnrealizePalette( handle );
    }

    if (InterlockedCompareExchangePointer( (void **)&hLastRealizedPalette, 0, handle ) == handle)
        TRACE("unrealizing palette %p\n", handle);

    return TRUE;
}


/***********************************************************************
 *           PALETTE_DeleteObject
 */
static BOOL PALETTE_DeleteObject( HGDIOBJ handle )
{
    PALETTEOBJ *obj;

    PALETTE_UnrealizeObject( handle );
    if (!(obj = free_gdi_handle( handle ))) return FALSE;
    HeapFree( GetProcessHeap(), 0, obj->entries );
    return HeapFree( GetProcessHeap(), 0, obj );
}


/***********************************************************************
 *           GDISelectPalette    (Not a Windows API)
 */
HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg)
{
    HPALETTE ret;
    DC *dc;

    TRACE("%p %p\n", hdc, hpal );

    if (GetObjectType(hpal) != OBJ_PAL)
    {
      WARN("invalid selected palette %p\n",hpal);
      return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    ret = dc->hPalette;
    if (dc->funcs->pSelectPalette) hpal = dc->funcs->pSelectPalette( dc->physDev, hpal, FALSE );
    if (hpal)
    {
        dc->hPalette = hpal;
        if (!wBkg) hPrimaryPalette = hpal;
    }
    else ret = 0;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GDIRealizePalette    (Not a Windows API)
 */
UINT WINAPI GDIRealizePalette( HDC hdc )
{
    UINT realized = 0;
    DC* dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    TRACE("%p...\n", hdc );

    if( dc->hPalette == GetStockObject( DEFAULT_PALETTE ))
    {
        if (dc->funcs->pRealizeDefaultPalette)
            realized = dc->funcs->pRealizeDefaultPalette( dc->physDev );
    }
    else if (InterlockedExchangePointer( (void **)&hLastRealizedPalette, dc->hPalette ) != dc->hPalette)
    {
        if (dc->funcs->pRealizePalette)
        {
            PALETTEOBJ *palPtr = GDI_GetObjPtr( dc->hPalette, OBJ_PAL );
            if (palPtr)
            {
                realized = dc->funcs->pRealizePalette( dc->physDev, dc->hPalette,
                                                       (dc->hPalette == hPrimaryPalette) );
                palPtr->funcs = dc->funcs;
                GDI_ReleaseObj( dc->hPalette );
            }
        }
    }
    else TRACE("  skipping (hLastRealizedPalette = %p)\n", hLastRealizedPalette);

    release_dc_ptr( dc );
    TRACE("   realized %i colors.\n", realized );
    return realized;
}


/***********************************************************************
 * SelectPalette [GDI32.@]
 *
 * Selects logical palette into DC.
 *
 * RETURNS
 *    Success: Previous logical palette
 *    Failure: NULL
 */
HPALETTE WINAPI SelectPalette(
    HDC hDC,               /* [in] Handle of device context */
    HPALETTE hPal,         /* [in] Handle of logical color palette */
    BOOL bForceBackground) /* [in] Foreground/background mode */
{
    return pfnSelectPalette( hDC, hPal, bForceBackground );
}


/***********************************************************************
 * RealizePalette [GDI32.@]
 *
 * Maps palette entries to system palette.
 *
 * RETURNS
 *    Success: Number of entries in logical palette
 *    Failure: GDI_ERROR
 */
UINT WINAPI RealizePalette(
    HDC hDC) /* [in] Handle of device context */
{
    return pfnRealizePalette( hDC );
}


typedef HWND (WINAPI *WindowFromDC_funcptr)( HDC );
typedef BOOL (WINAPI *RedrawWindow_funcptr)( HWND, const RECT *, HRGN, UINT );

/**********************************************************************
 * UpdateColors [GDI32.@]
 *
 * Remaps current colors to logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI UpdateColors(
    HDC hDC) /* [in] Handle of device context */
{
    HMODULE mod;
    int size = GetDeviceCaps( hDC, SIZEPALETTE );

    if (!size) return 0;

    mod = GetModuleHandleA("user32.dll");
    if (mod)
    {
        WindowFromDC_funcptr pWindowFromDC = (WindowFromDC_funcptr)GetProcAddress(mod,"WindowFromDC");
        if (pWindowFromDC)
        {
            HWND hWnd = pWindowFromDC( hDC );

            /* Docs say that we have to remap current drawable pixel by pixel
             * but it would take forever given the speed of XGet/PutPixel.
             */
            if (hWnd && size)
            {
                RedrawWindow_funcptr pRedrawWindow = (void *)GetProcAddress( mod, "RedrawWindow" );
                if (pRedrawWindow) pRedrawWindow( hWnd, NULL, 0, RDW_INVALIDATE );
            }
        }
    }
    return 0x666;
}

/*********************************************************************
 *           SetMagicColors   (GDI32.@)
 */
BOOL WINAPI SetMagicColors(HDC hdc, ULONG u1, ULONG u2)
{
    FIXME("(%p 0x%08x 0x%08x): stub\n", hdc, u1, u2);
    return TRUE;
}
