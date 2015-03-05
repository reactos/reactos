/*
 * Metafile driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "gdi_private.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

static BOOL MFDRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev );
static BOOL MFDRV_DeleteDC( PHYSDEV dev );


/**********************************************************************
 *           MFDRV_ExtEscape
 */
static INT MFDRV_ExtEscape( PHYSDEV dev, INT nEscape, INT cbInput, LPCVOID in_data,
                            INT cbOutput, LPVOID out_data )
{
    METARECORD *mr;
    DWORD len;
    INT ret;

    if (cbOutput) return 0;  /* escapes that require output cannot work in metafiles */

    len = sizeof(*mr) + sizeof(WORD) + ((cbInput + 1) & ~1);
    mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    mr->rdSize = len / 2;
    mr->rdFunction = META_ESCAPE;
    mr->rdParm[0] = nEscape;
    mr->rdParm[1] = cbInput;
    memcpy(&(mr->rdParm[2]), in_data, cbInput);
    ret = MFDRV_WriteRecord( dev, mr, len);
    HeapFree(GetProcessHeap(), 0, mr);
    return ret;
}


/******************************************************************
 *         MFDRV_GetBoundsRect
 */
static UINT MFDRV_GetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return 0;
}


/******************************************************************
 *         MFDRV_SetBoundsRect
 */
static UINT MFDRV_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    return 0;
}


/******************************************************************
 *         MFDRV_GetDeviceCaps
 *
 *A very simple implementation that returns DT_METAFILE
 */
static INT MFDRV_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    switch(cap)
    {
    case TECHNOLOGY:
        return DT_METAFILE;
    case TEXTCAPS:
        return 0;
    default:
        TRACE(" unsupported capability %d, will return 0\n", cap );
    }
    return 0;
}


static const struct gdi_dc_funcs MFDRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    MFDRV_AbortPath,                 /* pAbortPath */
    NULL,                            /* pAlphaBlend */
    NULL,                            /* pAngleArc */
    MFDRV_Arc,                       /* pArc */
    NULL,                            /* pArcTo */
    MFDRV_BeginPath,                 /* pBeginPath */
    NULL,                            /* pBlendImage */
    MFDRV_Chord,                     /* pChord */
    MFDRV_CloseFigure,               /* pCloseFigure */
    MFDRV_CreateCompatibleDC,        /* pCreateCompatibleDC */
    NULL,                            /* pCreateDC */
    MFDRV_DeleteDC,                  /* pDeleteDC */
    MFDRV_DeleteObject,              /* pDeleteObject */
    NULL,                            /* pDeviceCapabilities */
    MFDRV_Ellipse,                   /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    MFDRV_EndPath,                   /* pEndPath */
    NULL,                            /* pEnumFonts */
    NULL,                            /* pEnumICMProfiles */
    MFDRV_ExcludeClipRect,           /* pExcludeClipRect */
    NULL,                            /* pExtDeviceMode */
    MFDRV_ExtEscape,                 /* pExtEscape */
    MFDRV_ExtFloodFill,              /* pExtFloodFill */
    MFDRV_ExtSelectClipRgn,          /* pExtSelectClipRgn */
    MFDRV_ExtTextOut,                /* pExtTextOut */
    MFDRV_FillPath,                  /* pFillPath */
    MFDRV_FillRgn,                   /* pFillRgn */
    MFDRV_FlattenPath,               /* pFlattenPath */
    NULL,                            /* pFontIsLinked */
    MFDRV_FrameRgn,                  /* pFrameRgn */
    NULL,                            /* pGdiComment */
    NULL,                            /* pGdiRealizationInfo */
    MFDRV_GetBoundsRect,             /* pGetBoundsRect */
    NULL,                            /* pGetCharABCWidths */
    NULL,                            /* pGetCharABCWidthsI */
    NULL,                            /* pGetCharWidth */
    MFDRV_GetDeviceCaps,             /* pGetDeviceCaps */
    NULL,                            /* pGetDeviceGammaRamp */
    NULL,                            /* pGetFontData */
    NULL,                            /* pGetFontUnicodeRanges */
    NULL,                            /* pGetGlyphIndices */
    NULL,                            /* pGetGlyphOutline */
    NULL,                            /* pGetICMProfile */
    NULL,                            /* pGetImage */
    NULL,                            /* pGetKerningPairs */
    NULL,                            /* pGetNearestColor */
    NULL,                            /* pGetOutlineTextMetrics */
    NULL,                            /* pGetPixel */
    NULL,                            /* pGetSystemPaletteEntries */
    NULL,                            /* pGetTextCharsetInfo */
    NULL,                            /* pGetTextExtentExPoint */
    NULL,                            /* pGetTextExtentExPointI */
    NULL,                            /* pGetTextFace */
    NULL,                            /* pGetTextMetrics */
    NULL,                            /* pGradientFill */
    MFDRV_IntersectClipRect,         /* pIntersectClipRect */
    MFDRV_InvertRgn,                 /* pInvertRgn */
    MFDRV_LineTo,                    /* pLineTo */
    NULL,                            /* pModifyWorldTransform */
    MFDRV_MoveTo,                    /* pMoveTo */
    MFDRV_OffsetClipRgn,             /* pOffsetClipRgn */
    MFDRV_OffsetViewportOrgEx,       /* pOffsetViewportOrgEx */
    MFDRV_OffsetWindowOrgEx,         /* pOffsetWindowOrgEx */
    MFDRV_PaintRgn,                  /* pPaintRgn */
    MFDRV_PatBlt,                    /* pPatBlt */
    MFDRV_Pie,                       /* pPie */
    MFDRV_PolyBezier,                /* pPolyBezier */
    MFDRV_PolyBezierTo,              /* pPolyBezierTo */
    NULL,                            /* pPolyDraw */
    MFDRV_PolyPolygon,               /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    MFDRV_Polygon,                   /* pPolygon */
    MFDRV_Polyline,                  /* pPolyline */
    NULL,                            /* pPolylineTo */
    NULL,                            /* pPutImage */
    NULL,                            /* pRealizeDefaultPalette */
    MFDRV_RealizePalette,            /* pRealizePalette */
    MFDRV_Rectangle,                 /* pRectangle */
    NULL,                            /* pResetDC */
    MFDRV_RestoreDC,                 /* pRestoreDC */
    MFDRV_RoundRect,                 /* pRoundRect */
    MFDRV_SaveDC,                    /* pSaveDC */
    MFDRV_ScaleViewportExtEx,        /* pScaleViewportExtEx */
    MFDRV_ScaleWindowExtEx,          /* pScaleWindowExtEx */
    MFDRV_SelectBitmap,              /* pSelectBitmap */
    MFDRV_SelectBrush,               /* pSelectBrush */
    MFDRV_SelectClipPath,            /* pSelectClipPath */
    MFDRV_SelectFont,                /* pSelectFont */
    MFDRV_SelectPalette,             /* pSelectPalette */
    MFDRV_SelectPen,                 /* pSelectPen */
    NULL,                            /* pSetArcDirection */
    MFDRV_SetBkColor,                /* pSetBkColor */
    MFDRV_SetBkMode,                 /* pSetBkMode */
    MFDRV_SetBoundsRect,             /* pSetBoundsRect */
    MFDRV_SetDCBrushColor,           /* pSetDCBrushColor*/
    MFDRV_SetDCPenColor,             /* pSetDCPenColor*/
    MFDRV_SetDIBitsToDevice,         /* pSetDIBitsToDevice */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    NULL,                            /* pSetLayout */
    MFDRV_SetMapMode,                /* pSetMapMode */
    MFDRV_SetMapperFlags,            /* pSetMapperFlags */
    MFDRV_SetPixel,                  /* pSetPixel */
    MFDRV_SetPolyFillMode,           /* pSetPolyFillMode */
    MFDRV_SetROP2,                   /* pSetROP2 */
    MFDRV_SetRelAbs,                 /* pSetRelAbs */
    MFDRV_SetStretchBltMode,         /* pSetStretchBltMode */
    MFDRV_SetTextAlign,              /* pSetTextAlign */
    MFDRV_SetTextCharacterExtra,     /* pSetTextCharacterExtra */
    MFDRV_SetTextColor,              /* pSetTextColor */
    MFDRV_SetTextJustification,      /* pSetTextJustification */
    MFDRV_SetViewportExtEx,          /* pSetViewportExtEx */
    MFDRV_SetViewportOrgEx,          /* pSetViewportOrgEx */
    MFDRV_SetWindowExtEx,            /* pSetWindowExtEx */
    MFDRV_SetWindowOrgEx,            /* pSetWindowOrgEx */
    NULL,                            /* pSetWorldTransform */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    MFDRV_StretchBlt,                /* pStretchBlt */
    MFDRV_StretchDIBits,             /* pStretchDIBits */
    MFDRV_StrokeAndFillPath,         /* pStrokeAndFillPath */
    MFDRV_StrokePath,                /* pStrokePath */
    NULL,                            /* pUnrealizePalette */
    MFDRV_WidenPath,                 /* pWidenPath */
    NULL,                            /* wine_get_wgl_driver */
    GDI_PRIORITY_GRAPHICS_DRV        /* priority */
};



/**********************************************************************
 *	     MFDRV_AllocMetaFile
 */
static DC *MFDRV_AllocMetaFile(void)
{
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;

    if (!(dc = alloc_dc_ptr( OBJ_METADC ))) return NULL;

    physDev = HeapAlloc(GetProcessHeap(),0,sizeof(*physDev));
    if (!physDev)
    {
        free_dc_ptr( dc );
        return NULL;
    }
    if (!(physDev->mh = HeapAlloc( GetProcessHeap(), 0, sizeof(*physDev->mh) )))
    {
        HeapFree( GetProcessHeap(), 0, physDev );
        free_dc_ptr( dc );
        return NULL;
    }

    push_dc_driver( &dc->physDev, &physDev->dev, &MFDRV_Funcs );

    physDev->handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HANDLE_LIST_INC * sizeof(physDev->handles[0]));
    physDev->handles_size = HANDLE_LIST_INC;
    physDev->cur_handles = 0;

    physDev->hFile = 0;

    physDev->mh->mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
    physDev->mh->mtVersion      = 0x0300;
    physDev->mh->mtSize         = physDev->mh->mtHeaderSize;
    physDev->mh->mtNoObjects    = 0;
    physDev->mh->mtMaxRecord    = 0;
    physDev->mh->mtNoParameters = 0;

    SetVirtualResolution( physDev->dev.hdc, 0, 0, 0, 0);

    return dc;
}


/**********************************************************************
 *	     MFDRV_CreateCompatibleDC
 */
static BOOL MFDRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    /* not supported on metafile DCs */
    return FALSE;
}


/**********************************************************************
 *	     MFDRV_DeleteDC
 */
static BOOL MFDRV_DeleteDC( PHYSDEV dev )
{
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dev;
    DWORD index;

    HeapFree( GetProcessHeap(), 0, physDev->mh );
    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index])
            GDI_hdc_not_using_object(physDev->handles[index], dev->hdc);
    HeapFree( GetProcessHeap(), 0, physDev->handles );
    HeapFree( GetProcessHeap(), 0, physDev );
    return TRUE;
}


/**********************************************************************
 *	     CreateMetaFileW   (GDI32.@)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 *
 * PARAMS
 *  filename [I] Filename of disk metafile
 *
 * RETURNS
 *  A handle to the metafile DC if successful, NULL on failure.
 */
HDC WINAPI CreateMetaFileW( LPCWSTR filename )
{
    HDC ret;
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    HANDLE hFile;
    DWORD bytes_written;

    TRACE("%s\n", debugstr_w(filename) );

    if (!(dc = MFDRV_AllocMetaFile())) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    if (filename)  /* disk based metafile */
    {
        physDev->mh->mtType = METAFILE_DISK;
        if ((hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE) {
            free_dc_ptr( dc );
            return 0;
        }
        if (!WriteFile( hFile, physDev->mh, sizeof(*physDev->mh),
                        &bytes_written, NULL )) {
            free_dc_ptr( dc );
            CloseHandle (hFile );
            return 0;
	}
	physDev->hFile = hFile;

	/* Grow METAHEADER to include filename */
	physDev->mh = MF_CreateMetaHeaderDisk(physDev->mh, filename, TRUE);
    }
    else  /* memory based metafile */
	physDev->mh->mtType = METAFILE_MEMORY;

    TRACE("returning %p\n", physDev->dev.hdc);
    ret = physDev->dev.hdc;
    release_dc_ptr( dc );
    return ret;
}

/**********************************************************************
 *          CreateMetaFileA   (GDI32.@)
 *
 * See CreateMetaFileW.
 */
HDC WINAPI CreateMetaFileA(LPCSTR filename)
{
    LPWSTR filenameW;
    DWORD len;
    HDC hReturnDC;

    if (!filename) return CreateMetaFileW(NULL);

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    filenameW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    hReturnDC = CreateMetaFileW(filenameW);

    HeapFree( GetProcessHeap(), 0, filenameW );

    return hReturnDC;
}


/**********************************************************************
 *          MFDRV_CloseMetaFile
 */
static DC *MFDRV_CloseMetaFile( HDC hdc )
{
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    DWORD bytes_written;

    TRACE("(%p)\n", hdc );

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    if (GetObjectType( hdc ) != OBJ_METADC)
    {
        release_dc_ptr( dc );
        return NULL;
    }
    if (dc->refcount != 1)
    {
        FIXME( "not deleting busy DC %p refcount %u\n", hdc, dc->refcount );
        release_dc_ptr( dc );
        return NULL;
    }
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */

    if (!MFDRV_MetaParam0(dc->physDev, META_EOF))
    {
        free_dc_ptr( dc );
	return 0;
    }

    if (physDev->mh->mtType == METAFILE_DISK)  /* disk based metafile */
    {
        if (SetFilePointer(physDev->hFile, 0, NULL, FILE_BEGIN) != 0) {
            free_dc_ptr( dc );
            return 0;
        }

	physDev->mh->mtType = METAFILE_MEMORY; /* This is what windows does */
        if (!WriteFile(physDev->hFile, physDev->mh, sizeof(*physDev->mh),
                       &bytes_written, NULL)) {
            free_dc_ptr( dc );
            return 0;
        }
        CloseHandle(physDev->hFile);
	physDev->mh->mtType = METAFILE_DISK;
    }

    return dc;
}

/******************************************************************
 *	     CloseMetaFile   (GDI32.@)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 *
 * PARAMS
 *  hdc [I] Metafile DC to close 
 *
 * RETURNS
 *  Handle of newly created metafile on success, NULL on failure.
 */
HMETAFILE WINAPI CloseMetaFile(HDC hdc)
{
    HMETAFILE hmf;
    METAFILEDRV_PDEVICE *physDev;
    DC *dc = MFDRV_CloseMetaFile(hdc);
    if (!dc) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    /* Now allocate a global handle for the metafile */

    hmf = MF_Create_HMETAFILE( physDev->mh );

    physDev->mh = NULL;  /* So it won't be deleted */
    free_dc_ptr( dc );
    return hmf;
}


/******************************************************************
 *         MFDRV_WriteRecord
 *
 * Warning: this function can change the pointer to the metafile header.
 */
BOOL MFDRV_WriteRecord( PHYSDEV dev, METARECORD *mr, DWORD rlen)
{
    DWORD len, size;
    METAHEADER *mh;
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dev;

    switch(physDev->mh->mtType)
    {
    case METAFILE_MEMORY:
	len = physDev->mh->mtSize * 2 + rlen;
	/* reallocate memory if needed */
        size = HeapSize( GetProcessHeap(), 0, physDev->mh );
        if (len > size)
        {
            /*expand size*/
            size += size / 2 + rlen;
            mh = HeapReAlloc( GetProcessHeap(), 0, physDev->mh, size);
            if (!mh) return FALSE;
            physDev->mh = mh;
            TRACE("Reallocated metafile: new size is %d\n",size);
        }
	memcpy((WORD *)physDev->mh + physDev->mh->mtSize, mr, rlen);
        break;
    case METAFILE_DISK:
        TRACE("Writing record to disk\n");
        if (!WriteFile(physDev->hFile, mr, rlen, NULL, NULL))
	    return FALSE;
        break;
    default:
        ERR("Unknown metafile type %d\n", physDev->mh->mtType );
        return FALSE;
    }

    physDev->mh->mtSize += rlen / 2;
    physDev->mh->mtMaxRecord = max(physDev->mh->mtMaxRecord, rlen / 2);
    return TRUE;
}


/******************************************************************
 *         MFDRV_MetaParam0
 */

BOOL MFDRV_MetaParam0(PHYSDEV dev, short func)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = 3;
    mr->rdFunction = func;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_MetaParam1
 */
BOOL MFDRV_MetaParam1(PHYSDEV dev, short func, short param1)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    WORD *params = mr->rdParm;

    mr->rdSize = 4;
    mr->rdFunction = func;
    params[0] = param1;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_MetaParam2
 */
BOOL MFDRV_MetaParam2(PHYSDEV dev, short func, short param1, short param2)
{
    char buffer[10];
    METARECORD *mr = (METARECORD *)&buffer;
    WORD *params = mr->rdParm;

    mr->rdSize = 5;
    mr->rdFunction = func;
    params[0] = param2;
    params[1] = param1;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_MetaParam4
 */

BOOL MFDRV_MetaParam4(PHYSDEV dev, short func, short param1, short param2,
		      short param3, short param4)
{
    char buffer[14];
    METARECORD *mr = (METARECORD *)&buffer;
    WORD *params = mr->rdParm;

    mr->rdSize = 7;
    mr->rdFunction = func;
    params[0] = param4;
    params[1] = param3;
    params[2] = param2;
    params[3] = param1;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_MetaParam6
 */

BOOL MFDRV_MetaParam6(PHYSDEV dev, short func, short param1, short param2,
		      short param3, short param4, short param5, short param6)
{
    char buffer[18];
    METARECORD *mr = (METARECORD *)&buffer;
    WORD *params = mr->rdParm;

    mr->rdSize = 9;
    mr->rdFunction = func;
    params[0] = param6;
    params[1] = param5;
    params[2] = param4;
    params[3] = param3;
    params[4] = param2;
    params[5] = param1;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_MetaParam8
 */
BOOL MFDRV_MetaParam8(PHYSDEV dev, short func, short param1, short param2,
		      short param3, short param4, short param5,
		      short param6, short param7, short param8)
{
    char buffer[22];
    METARECORD *mr = (METARECORD *)&buffer;
    WORD *params = mr->rdParm;

    mr->rdSize = 11;
    mr->rdFunction = func;
    params[0] = param8;
    params[1] = param7;
    params[2] = param6;
    params[3] = param5;
    params[4] = param4;
    params[5] = param3;
    params[6] = param2;
    params[7] = param1;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}
