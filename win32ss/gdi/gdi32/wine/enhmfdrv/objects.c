/*
 * Enhanced MetaFile objects
 *
 * Copyright 1999 Huw D M Davies
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "enhmfdrv/enhmetafiledrv.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);


/******************************************************************
 *         EMFDRV_AddHandle
 */
static UINT EMFDRV_AddHandle( PHYSDEV dev, HGDIOBJ obj )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dev;
    UINT index;

    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index] == 0) break;
    if(index == physDev->handles_size) {
        physDev->handles_size += HANDLE_LIST_INC;
	physDev->handles = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       physDev->handles,
				       physDev->handles_size * sizeof(physDev->handles[0]));
    }
    physDev->handles[index] = get_full_gdi_handle( obj );

    physDev->cur_handles++;
    if(physDev->cur_handles > physDev->emh->nHandles)
        physDev->emh->nHandles++;

    return index + 1; /* index 0 is reserved for the hmf, so we increment everything by 1 */
}

/******************************************************************
 *         EMFDRV_FindObject
 */
static UINT EMFDRV_FindObject( PHYSDEV dev, HGDIOBJ obj )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*) dev;
    UINT index;

    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index] == obj) break;

    if(index == physDev->handles_size) return 0;

    return index + 1;
}


/******************************************************************
 *         EMFDRV_DeleteObject
 */
BOOL EMFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj )
{
    EMRDELETEOBJECT emr;
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*) dev;
    UINT index;
    BOOL ret = TRUE;

    if(!(index = EMFDRV_FindObject(dev, obj))) return FALSE;

    emr.emr.iType = EMR_DELETEOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        ret = FALSE;

    physDev->handles[index - 1] = 0;
    physDev->cur_handles--;
    return ret;
}


/***********************************************************************
 *           EMFDRV_SelectBitmap
 */
HBITMAP EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    return 0;
}


/***********************************************************************
 *           EMFDRV_CreateBrushIndirect
 */
DWORD EMFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush )
{
    DWORD index = 0;
    LOGBRUSH logbrush;

    if (!GetObjectA( hBrush, sizeof(logbrush), &logbrush )) return 0;

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
    case BS_NULL:
      {
	EMRCREATEBRUSHINDIRECT emr;
	emr.emr.iType = EMR_CREATEBRUSHINDIRECT;
	emr.emr.nSize = sizeof(emr);
	emr.ihBrush = index = EMFDRV_AddHandle( dev, hBrush );
	emr.lb.lbStyle = logbrush.lbStyle;
	emr.lb.lbColor = logbrush.lbColor;
	emr.lb.lbHatch = logbrush.lbHatch;

	if(!EMFDRV_WriteRecord( dev, &emr.emr ))
	    index = 0;
      }
      break;
    case BS_PATTERN:
    case BS_DIBPATTERN:
      {
        EMRCREATEDIBPATTERNBRUSHPT *emr;
        char buffer[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)]; // ros!
        //char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        DWORD info_size;
        void *bits;
        UINT usage;

        if (!get_brush_bitmap_info( hBrush, info, &bits, &usage )) break;
        info_size = get_dib_info_size( info, usage );

        emr = HeapAlloc( GetProcessHeap(), 0,
                         sizeof(EMRCREATEDIBPATTERNBRUSHPT)+info_size+info->bmiHeader.biSizeImage );
        if(!emr) break;

        if (logbrush.lbStyle == BS_PATTERN && info->bmiHeader.biBitCount == 1)
        {
            /* Presumably to reduce the size of the written EMF, MS supports an
             * undocumented iUsage value of 2, indicating a mono bitmap without the
             * 8 byte 2 entry black/white palette. Stupidly, they could have saved
             * over 20 bytes more by also ignoring the BITMAPINFO fields that are
             * irrelevant/constant for monochrome bitmaps.
             * FIXME: It may be that the DIB functions themselves accept this value.
             */
            emr->emr.iType = EMR_CREATEMONOBRUSH;
            usage = DIB_PAL_MONO;
            /* FIXME: There is an extra DWORD written by native before the BMI.
             *        Not sure what its meant to contain.
             */
            emr->offBmi = sizeof( EMRCREATEDIBPATTERNBRUSHPT ) + sizeof(DWORD);
            emr->cbBmi = sizeof( BITMAPINFOHEADER );
        }
        else
        {
            emr->emr.iType = EMR_CREATEDIBPATTERNBRUSHPT;
            emr->offBmi = sizeof( EMRCREATEDIBPATTERNBRUSHPT );
            emr->cbBmi = info_size;
        }
        emr->ihBrush = index = EMFDRV_AddHandle( dev, hBrush );
        emr->iUsage = usage;
        emr->offBits = emr->offBmi + emr->cbBmi;
        emr->cbBits = info->bmiHeader.biSizeImage;
        emr->emr.nSize = emr->offBits + emr->cbBits;

        memcpy( (BYTE *)emr + emr->offBmi, info, emr->cbBmi );
        memcpy( (BYTE *)emr + emr->offBits, bits, emr->cbBits );

        if(!EMFDRV_WriteRecord( dev, &emr->emr ))
            index = 0;
        HeapFree( GetProcessHeap(), 0, emr );
      }
      break;

    default:
        FIXME("Unknown style %x\n", logbrush.lbStyle);
	break;
    }
    return index;
}


/***********************************************************************
 *           EMFDRV_SelectBrush
 */
HBRUSH EMFDRV_SelectBrush( PHYSDEV dev, HBRUSH hBrush, const struct brush_pattern *pattern )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    if (physDev->restoring) return hBrush;  /* don't output SelectObject records during RestoreDC */

    /* If the object is a stock brush object, do not need to create it.
     * See definitions in  wingdi.h for range of stock brushes.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */
    for (i = WHITE_BRUSH; i <= DC_BRUSH; i++)
    {
        if (hBrush == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }
    if((index = EMFDRV_FindObject(dev, hBrush)) != 0)
        goto found;

    if (!(index = EMFDRV_CreateBrushIndirect(dev, hBrush ))) return 0;
    GDI_hdc_using_object(hBrush, dev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? hBrush : 0;
}


/******************************************************************
 *         EMFDRV_CreateFontIndirect
 */
static BOOL EMFDRV_CreateFontIndirect(PHYSDEV dev, HFONT hFont )
{
    DWORD index = 0;
    EMREXTCREATEFONTINDIRECTW emr;
    int i;

    if (!GetObjectW( hFont, sizeof(emr.elfw.elfLogFont), &emr.elfw.elfLogFont )) return FALSE;

    emr.emr.iType = EMR_EXTCREATEFONTINDIRECTW;
    emr.emr.nSize = (sizeof(emr) + 3) / 4 * 4;
    emr.ihFont = index = EMFDRV_AddHandle( dev, hFont );
    emr.elfw.elfFullName[0] = '\0';
    emr.elfw.elfStyle[0]    = '\0';
    emr.elfw.elfVersion     = 0;
    emr.elfw.elfStyleSize   = 0;
    emr.elfw.elfMatch       = 0;
    emr.elfw.elfReserved    = 0;
    for(i = 0; i < ELF_VENDOR_SIZE; i++)
        emr.elfw.elfVendorId[i] = 0;
    emr.elfw.elfCulture                 = PAN_CULTURE_LATIN;
    emr.elfw.elfPanose.bFamilyType      = PAN_NO_FIT;
    emr.elfw.elfPanose.bSerifStyle      = PAN_NO_FIT;
    emr.elfw.elfPanose.bWeight          = PAN_NO_FIT;
    emr.elfw.elfPanose.bProportion      = PAN_NO_FIT;
    emr.elfw.elfPanose.bContrast        = PAN_NO_FIT;
    emr.elfw.elfPanose.bStrokeVariation = PAN_NO_FIT;
    emr.elfw.elfPanose.bArmStyle        = PAN_NO_FIT;
    emr.elfw.elfPanose.bLetterform      = PAN_NO_FIT;
    emr.elfw.elfPanose.bMidline         = PAN_NO_FIT;
    emr.elfw.elfPanose.bXHeight         = PAN_NO_FIT;

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        index = 0;
    return index;
}


/***********************************************************************
 *           EMFDRV_SelectFont
 */
HFONT EMFDRV_SelectFont( PHYSDEV dev, HFONT hFont, UINT *aa_flags )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    if (physDev->restoring) goto done;  /* don't output SelectObject records during RestoreDC */

    /* If the object is a stock font object, do not need to create it.
     * See definitions in  wingdi.h for range of stock fonts.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */

    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
    {
        if (i != DEFAULT_PALETTE && hFont == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }

    if((index = EMFDRV_FindObject(dev, hFont)) != 0)
        goto found;

    if (!(index = EMFDRV_CreateFontIndirect(dev, hFont ))) return 0;
    GDI_hdc_using_object(hFont, dev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        return 0;
done:
    *aa_flags = GGO_BITMAP;  /* no point in anti-aliasing on metafiles */
    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    dev->funcs->pSelectFont( dev, hFont, aa_flags );
    return hFont;
}



/******************************************************************
 *         EMFDRV_CreatePenIndirect
 */
static DWORD EMFDRV_CreatePenIndirect(PHYSDEV dev, HPEN hPen)
{
    EMRCREATEPEN emr;
    DWORD index = 0;

    if (!GetObjectW( hPen, sizeof(emr.lopn), &emr.lopn ))
    {
        /* must be an extended pen */
        EXTLOGPEN *elp;
        INT size = GetObjectW( hPen, 0, NULL );

        if (!size) return 0;

        elp = HeapAlloc( GetProcessHeap(), 0, size );

        GetObjectW( hPen, size, elp );
        /* FIXME: add support for user style pens */
        emr.lopn.lopnStyle = elp->elpPenStyle;
        emr.lopn.lopnWidth.x = elp->elpWidth;
        emr.lopn.lopnWidth.y = 0;
        emr.lopn.lopnColor = elp->elpColor;

        HeapFree( GetProcessHeap(), 0, elp );
    }

    emr.emr.iType = EMR_CREATEPEN;
    emr.emr.nSize = sizeof(emr);
    emr.ihPen = index = EMFDRV_AddHandle( dev, hPen );

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        index = 0;
    return index;
}

/******************************************************************
 *         EMFDRV_SelectPen
 */
HPEN EMFDRV_SelectPen(PHYSDEV dev, HPEN hPen, const struct brush_pattern *pattern )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    if (physDev->restoring) return hPen;  /* don't output SelectObject records during RestoreDC */

    /* If the object is a stock pen object, do not need to create it.
     * See definitions in  wingdi.h for range of stock pens.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */

    for (i = WHITE_PEN; i <= DC_PEN; i++)
    {
        if (hPen == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }
    if((index = EMFDRV_FindObject(dev, hPen)) != 0)
        goto found;

    if (!(index = EMFDRV_CreatePenIndirect(dev, hPen))) return 0;
    GDI_hdc_using_object(hPen, dev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? hPen : 0;
}


/******************************************************************
 *         EMFDRV_CreatePalette
 */
static DWORD EMFDRV_CreatePalette(PHYSDEV dev, HPALETTE hPal)
{
    WORD i;
    struct {
        EMRCREATEPALETTE hdr;
        PALETTEENTRY entry[255];
    } pal;

    memset( &pal, 0, sizeof(pal) );

    if (!GetObjectW( hPal, sizeof(pal.hdr.lgpl) + sizeof(pal.entry), &pal.hdr.lgpl ))
        return 0;

    for (i = 0; i < pal.hdr.lgpl.palNumEntries; i++)
        pal.hdr.lgpl.palPalEntry[i].peFlags = 0;

    pal.hdr.emr.iType = EMR_CREATEPALETTE;
    pal.hdr.emr.nSize = sizeof(pal.hdr) + pal.hdr.lgpl.palNumEntries * sizeof(PALETTEENTRY);
    pal.hdr.ihPal = EMFDRV_AddHandle( dev, hPal );

    if (!EMFDRV_WriteRecord( dev, &pal.hdr.emr ))
        pal.hdr.ihPal = 0;
    return pal.hdr.ihPal;
}

/******************************************************************
 *         EMFDRV_SelectPalette
 */
HPALETTE EMFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPal, BOOL force )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTPALETTE emr;
    DWORD index;

    if (physDev->restoring) return hPal;  /* don't output SelectObject records during RestoreDC */

    if (hPal == GetStockObject( DEFAULT_PALETTE ))
    {
        index = DEFAULT_PALETTE | 0x80000000;
        goto found;
    }

    if ((index = EMFDRV_FindObject( dev, hPal )) != 0)
        goto found;

    if (!(index = EMFDRV_CreatePalette( dev, hPal ))) return 0;
    GDI_hdc_using_object( hPal, dev->hdc );

found:
    emr.emr.iType = EMR_SELECTPALETTE;
    emr.emr.nSize = sizeof(emr);
    emr.ihPal = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? hPal : 0;
}

/******************************************************************
 *         EMFDRV_SetDCBrushColor
 */
COLORREF EMFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;

    if (GetCurrentObject( dev->hdc, OBJ_BRUSH ) != GetStockObject( DC_BRUSH )) return color;

    if (physDev->dc_brush) DeleteObject( physDev->dc_brush );
    if (!(physDev->dc_brush = CreateSolidBrush( color ))) return CLR_INVALID;
    if (!(index = EMFDRV_CreateBrushIndirect(dev, physDev->dc_brush ))) return CLR_INVALID;
    GDI_hdc_using_object( physDev->dc_brush, dev->hdc );
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}

/******************************************************************
 *         EMFDRV_SetDCPenColor
 */
COLORREF EMFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    LOGPEN logpen = { PS_SOLID, { 0, 0 }, color };

    if (GetCurrentObject( dev->hdc, OBJ_PEN ) != GetStockObject( DC_PEN )) return color;

    if (physDev->dc_pen) DeleteObject( physDev->dc_pen );
    if (!(physDev->dc_pen = CreatePenIndirect( &logpen ))) return CLR_INVALID;
    if (!(index = EMFDRV_CreatePenIndirect(dev, physDev->dc_pen))) return CLR_INVALID;
    GDI_hdc_using_object( physDev->dc_pen, dev->hdc );
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}

/******************************************************************
 *         EMFDRV_GdiComment
 */
BOOL EMFDRV_GdiComment(PHYSDEV dev, UINT bytes, const BYTE *buffer)
{
    EMRGDICOMMENT *emr;
    UINT total, rounded_size;
    BOOL ret;

    rounded_size = (bytes+3) & ~3;
    total = offsetof(EMRGDICOMMENT,Data) + rounded_size;

    emr = HeapAlloc(GetProcessHeap(), 0, total);
    emr->emr.iType = EMR_GDICOMMENT;
    emr->emr.nSize = total;
    emr->cbData = bytes;
    memset(&emr->Data[bytes], 0, rounded_size - bytes);
    memcpy(&emr->Data[0], buffer, bytes);

    ret = EMFDRV_WriteRecord( dev, &emr->emr );

    HeapFree(GetProcessHeap(), 0, emr);

    return ret;
}
