/*
 * Enhanced metafile functions
 * Copyright 1998 Douglas Ridgway
 *           1999 Huw D M Davies
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
 *
 * The enhanced format consists of the following elements:
 *
 *    A header
 *    A table of handles to GDI objects
 *    An array of metafile records
 *    A private palette
 *
 *
 *  The standard format consists of a header and an array of metafile records.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);


static CRITICAL_SECTION enhmetafile_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &enhmetafile_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": enhmetafile_cs") }
};
static CRITICAL_SECTION enhmetafile_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef struct
{
    ENHMETAHEADER  *emh;
    BOOL           on_disk;   /* true if metafile is on disk */
} ENHMETAFILEOBJ;

static const struct emr_name {
    DWORD type;
    const char *name;
} emr_names[] = {
#define X(p) {p, #p}
X(EMR_HEADER),
X(EMR_POLYBEZIER),
X(EMR_POLYGON),
X(EMR_POLYLINE),
X(EMR_POLYBEZIERTO),
X(EMR_POLYLINETO),
X(EMR_POLYPOLYLINE),
X(EMR_POLYPOLYGON),
X(EMR_SETWINDOWEXTEX),
X(EMR_SETWINDOWORGEX),
X(EMR_SETVIEWPORTEXTEX),
X(EMR_SETVIEWPORTORGEX),
X(EMR_SETBRUSHORGEX),
X(EMR_EOF),
X(EMR_SETPIXELV),
X(EMR_SETMAPPERFLAGS),
X(EMR_SETMAPMODE),
X(EMR_SETBKMODE),
X(EMR_SETPOLYFILLMODE),
X(EMR_SETROP2),
X(EMR_SETSTRETCHBLTMODE),
X(EMR_SETTEXTALIGN),
X(EMR_SETCOLORADJUSTMENT),
X(EMR_SETTEXTCOLOR),
X(EMR_SETBKCOLOR),
X(EMR_OFFSETCLIPRGN),
X(EMR_MOVETOEX),
X(EMR_SETMETARGN),
X(EMR_EXCLUDECLIPRECT),
X(EMR_INTERSECTCLIPRECT),
X(EMR_SCALEVIEWPORTEXTEX),
X(EMR_SCALEWINDOWEXTEX),
X(EMR_SAVEDC),
X(EMR_RESTOREDC),
X(EMR_SETWORLDTRANSFORM),
X(EMR_MODIFYWORLDTRANSFORM),
X(EMR_SELECTOBJECT),
X(EMR_CREATEPEN),
X(EMR_CREATEBRUSHINDIRECT),
X(EMR_DELETEOBJECT),
X(EMR_ANGLEARC),
X(EMR_ELLIPSE),
X(EMR_RECTANGLE),
X(EMR_ROUNDRECT),
X(EMR_ARC),
X(EMR_CHORD),
X(EMR_PIE),
X(EMR_SELECTPALETTE),
X(EMR_CREATEPALETTE),
X(EMR_SETPALETTEENTRIES),
X(EMR_RESIZEPALETTE),
X(EMR_REALIZEPALETTE),
X(EMR_EXTFLOODFILL),
X(EMR_LINETO),
X(EMR_ARCTO),
X(EMR_POLYDRAW),
X(EMR_SETARCDIRECTION),
X(EMR_SETMITERLIMIT),
X(EMR_BEGINPATH),
X(EMR_ENDPATH),
X(EMR_CLOSEFIGURE),
X(EMR_FILLPATH),
X(EMR_STROKEANDFILLPATH),
X(EMR_STROKEPATH),
X(EMR_FLATTENPATH),
X(EMR_WIDENPATH),
X(EMR_SELECTCLIPPATH),
X(EMR_ABORTPATH),
X(EMR_GDICOMMENT),
X(EMR_FILLRGN),
X(EMR_FRAMERGN),
X(EMR_INVERTRGN),
X(EMR_PAINTRGN),
X(EMR_EXTSELECTCLIPRGN),
X(EMR_BITBLT),
X(EMR_STRETCHBLT),
X(EMR_MASKBLT),
X(EMR_PLGBLT),
X(EMR_SETDIBITSTODEVICE),
X(EMR_STRETCHDIBITS),
X(EMR_EXTCREATEFONTINDIRECTW),
X(EMR_EXTTEXTOUTA),
X(EMR_EXTTEXTOUTW),
X(EMR_POLYBEZIER16),
X(EMR_POLYGON16),
X(EMR_POLYLINE16),
X(EMR_POLYBEZIERTO16),
X(EMR_POLYLINETO16),
X(EMR_POLYPOLYLINE16),
X(EMR_POLYPOLYGON16),
X(EMR_POLYDRAW16),
X(EMR_CREATEMONOBRUSH),
X(EMR_CREATEDIBPATTERNBRUSHPT),
X(EMR_EXTCREATEPEN),
X(EMR_POLYTEXTOUTA),
X(EMR_POLYTEXTOUTW),
X(EMR_SETICMMODE),
X(EMR_CREATECOLORSPACE),
X(EMR_SETCOLORSPACE),
X(EMR_DELETECOLORSPACE),
X(EMR_GLSRECORD),
X(EMR_GLSBOUNDEDRECORD),
X(EMR_PIXELFORMAT),
X(EMR_DRAWESCAPE),
X(EMR_EXTESCAPE),
X(EMR_STARTDOC),
X(EMR_SMALLTEXTOUT),
X(EMR_FORCEUFIMAPPING),
X(EMR_NAMEDESCAPE),
X(EMR_COLORCORRECTPALETTE),
X(EMR_SETICMPROFILEA),
X(EMR_SETICMPROFILEW),
X(EMR_ALPHABLEND),
X(EMR_SETLAYOUT),
X(EMR_TRANSPARENTBLT),
X(EMR_RESERVED_117),
X(EMR_GRADIENTFILL),
X(EMR_SETLINKEDUFI),
X(EMR_SETTEXTJUSTIFICATION),
X(EMR_COLORMATCHTOTARGETW),
X(EMR_CREATECOLORSPACEW)
#undef X
};

/****************************************************************************
 *         get_emr_name
 */
static const char *get_emr_name(DWORD type)
{
    unsigned int i;
    for(i = 0; i < sizeof(emr_names) / sizeof(emr_names[0]); i++)
        if(type == emr_names[i].type) return emr_names[i].name;
    TRACE("Unknown record type %d\n", type);
   return NULL;
}

/***********************************************************************
 *          is_dib_monochrome
 *
 * Returns whether a DIB can be converted to a monochrome DDB.
 *
 * A DIB can be converted if its color table contains only black and
 * white. Black must be the first color in the color table.
 *
 * Note : If the first color in the color table is white followed by
 *        black, we can't convert it to a monochrome DDB with
 *        SetDIBits, because black and white would be inverted.
 */
static inline BOOL is_dib_monochrome( const BITMAPINFO* info )
{
    if (info->bmiHeader.biBitCount != 1) return FALSE;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const RGBTRIPLE *rgb = ((const BITMAPCOREINFO *) info)->bmciColors;

        /* Check if the first color is black */
        if ((rgb->rgbtRed == 0) && (rgb->rgbtGreen == 0) && (rgb->rgbtBlue == 0))
        {
            rgb++;
            /* Check if the second color is white */
            return ((rgb->rgbtRed == 0xff) && (rgb->rgbtGreen == 0xff)
                 && (rgb->rgbtBlue == 0xff));
        }
        else return FALSE;
    }
    else  /* assume BITMAPINFOHEADER */
    {
        const RGBQUAD *rgb = info->bmiColors;

        /* Check if the first color is black */
        if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) &&
            (rgb->rgbBlue == 0) && (rgb->rgbReserved == 0))
        {
            rgb++;

            /* Check if the second color is white */
            return ((rgb->rgbRed == 0xff) && (rgb->rgbGreen == 0xff)
                 && (rgb->rgbBlue == 0xff) && (rgb->rgbReserved == 0));
        }
        else return FALSE;
    }
}

/****************************************************************************
 *          EMF_Create_HENHMETAFILE
 */
HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, DWORD filesize, BOOL on_disk )
{
    HENHMETAFILE hmf;
    ENHMETAFILEOBJ *metaObj;

    if (emh->iType != EMR_HEADER)
    {
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }
    if (emh->dSignature != ENHMETA_SIGNATURE ||
        (emh->nBytes & 3)) /* refuse to load unaligned EMF as Windows does */
    {
        WARN("Invalid emf header type 0x%08x sig 0x%08x.\n",
             emh->iType, emh->dSignature);
        return 0;
    }
    if (filesize < emh->nBytes)
    {
        WARN("File truncated (got %u bytes, header says %u)\n", emh->nBytes, filesize);
        return 0;
    }

    if (!(metaObj = HeapAlloc( GetProcessHeap(), 0, sizeof(*metaObj) ))) return 0;

    metaObj->emh = emh;
    metaObj->on_disk = on_disk;

    if (!(hmf = alloc_gdi_handle( metaObj, OBJ_ENHMETAFILE, NULL )))
        HeapFree( GetProcessHeap(), 0, metaObj );
    return hmf;
}

/****************************************************************************
 *          EMF_Delete_HENHMETAFILE
 */
static BOOL EMF_Delete_HENHMETAFILE( HENHMETAFILE hmf )
{
    ENHMETAFILEOBJ *metaObj;
    BOOL Ret = FALSE;

    EnterCriticalSection( &enhmetafile_cs );
    metaObj = free_gdi_handle( hmf );
    if(metaObj)
    {
        if(metaObj->on_disk)
            UnmapViewOfFile( metaObj->emh );
        else
            HeapFree( GetProcessHeap(), 0, metaObj->emh );
        HeapFree( GetProcessHeap(), 0, metaObj );
        Ret = TRUE;
    }
    LeaveCriticalSection( &enhmetafile_cs );
    return Ret;
}

/******************************************************************
 *         EMF_GetEnhMetaHeader
 *
 * Returns ptr to ENHMETAHEADER associated with HENHMETAFILE
 */
static ENHMETAHEADER *EMF_GetEnhMetaHeader( HENHMETAFILE hmf )
{
    ENHMETAHEADER *ret = NULL;
    ENHMETAFILEOBJ *metaObj;

    EnterCriticalSection( &enhmetafile_cs );
    metaObj = GDI_GetObjPtr( hmf, OBJ_ENHMETAFILE );
    TRACE("hmf %p -> enhmetaObj %p\n", hmf, metaObj);
    if (metaObj)
    {
        ret = metaObj->emh;
        GDI_ReleaseObj( hmf );
    }
    LeaveCriticalSection( &enhmetafile_cs );
    return ret;
}

/*****************************************************************************
 *         EMF_GetEnhMetaFile
 *
 */
static HENHMETAFILE EMF_GetEnhMetaFile( HANDLE hFile )
{
    ENHMETAHEADER *emh;
    HANDLE hMapping;
    HENHMETAFILE hemf;
    DWORD filesize;

    filesize = GetFileSize( hFile, NULL );

    hMapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    emh = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( hMapping );

    if (!emh) return 0;

    hemf = EMF_Create_HENHMETAFILE( emh, filesize, TRUE );
    if (!hemf)
        UnmapViewOfFile( emh );
    return hemf;
}


/*****************************************************************************
 *          GetEnhMetaFileA (GDI32.@)
 *
 *
 */
HENHMETAFILE WINAPI GetEnhMetaFileA(
	     LPCSTR lpszMetaFile  /* [in] filename of enhanced metafile */
    )
{
    HENHMETAFILE hmf;
    HANDLE hFile;

    hFile = CreateFileA(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", lpszMetaFile);
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *          GetEnhMetaFileW  (GDI32.@)
 */
HENHMETAFILE WINAPI GetEnhMetaFileW(
             LPCWSTR lpszMetaFile)  /* [in] filename of enhanced metafile */
{
    HENHMETAFILE hmf;
    HANDLE hFile;

    hFile = CreateFileW(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", debugstr_w(lpszMetaFile));
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *        GetEnhMetaFileHeader  (GDI32.@)
 *
 * Retrieves the record containing the header for the specified
 * enhanced-format metafile.
 *
 * RETURNS
 *  If buf is NULL, returns the size of buffer required.
 *  Otherwise, copy up to bufsize bytes of enhanced metafile header into
 *  buf.
 */
UINT WINAPI GetEnhMetaFileHeader(
       HENHMETAFILE hmf,   /* [in] enhanced metafile */
       UINT bufsize,       /* [in] size of buffer */
       LPENHMETAHEADER buf /* [out] buffer */
    )
{
    LPENHMETAHEADER emh;
    UINT size;

    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) return FALSE;
    size = emh->nSize;
    if (!buf) return size;
    size = min(size, bufsize);
    memmove(buf, emh, size);
    return size;
}


/*****************************************************************************
 *          GetEnhMetaFileDescriptionA  (GDI32.@)
 *
 * See GetEnhMetaFileDescriptionW.
 */
UINT WINAPI GetEnhMetaFileDescriptionA(
       HENHMETAFILE hmf, /* [in] enhanced metafile */
       UINT size,        /* [in] size of buf */
       LPSTR buf         /* [out] buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);
     DWORD len;
     WCHAR *descrW;

     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) return 0;
     descrW = (WCHAR *) ((char *) emh + emh->offDescription);
     len = WideCharToMultiByte( CP_ACP, 0, descrW, emh->nDescription, NULL, 0, NULL, NULL );

     if (!buf || !size ) return len;

     len = min( size, len );
     WideCharToMultiByte( CP_ACP, 0, descrW, emh->nDescription, buf, len, NULL, NULL );
     return len;
}

/*****************************************************************************
 *          GetEnhMetaFileDescriptionW  (GDI32.@)
 *
 *  Copies the description string of an enhanced metafile into a buffer
 *  _buf_.
 *
 * RETURNS
 *  If _buf_ is NULL, returns size of _buf_ required. Otherwise, returns
 *  number of characters copied.
 */
UINT WINAPI GetEnhMetaFileDescriptionW(
       HENHMETAFILE hmf, /* [in] enhanced metafile */
       UINT size,        /* [in] size of buf */
       LPWSTR buf        /* [out] buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);

     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) return 0;
     if (!buf || !size ) return emh->nDescription;

     memmove(buf, (char *) emh + emh->offDescription, min(size,emh->nDescription)*sizeof(WCHAR));
     return min(size, emh->nDescription);
}

/****************************************************************************
 *    SetEnhMetaFileBits (GDI32.@)
 *
 *  Creates an enhanced metafile by copying _bufsize_ bytes from _buf_.
 */
HENHMETAFILE WINAPI SetEnhMetaFileBits(UINT bufsize, const BYTE *buf)
{
    ENHMETAHEADER *emh = HeapAlloc( GetProcessHeap(), 0, bufsize );
    HENHMETAFILE hmf;
    memmove(emh, buf, bufsize);
    hmf = EMF_Create_HENHMETAFILE( emh, bufsize, FALSE );
    if (!hmf)
        HeapFree( GetProcessHeap(), 0, emh );
    return hmf;
}

/*****************************************************************************
 *  GetEnhMetaFileBits (GDI32.@)
 *
 */
UINT WINAPI GetEnhMetaFileBits(
    HENHMETAFILE hmf,
    UINT bufsize,
    LPBYTE buf
)
{
    LPENHMETAHEADER emh = EMF_GetEnhMetaHeader( hmf );
    UINT size;

    if(!emh) return 0;

    size = emh->nBytes;
    if( buf == NULL ) return size;

    size = min( size, bufsize );
    memmove(buf, emh, size);
    return size;
}

typedef struct EMF_dc_state
{
    INT   mode;
    XFORM world_transform;
    INT   wndOrgX;
    INT   wndOrgY;
    INT   wndExtX;
    INT   wndExtY;
    INT   vportOrgX;
    INT   vportOrgY;
    INT   vportExtX;
    INT   vportExtY;
    struct EMF_dc_state *next;
} EMF_dc_state;

typedef struct enum_emh_data
{
    XFORM init_transform;
    EMF_dc_state state;
    INT save_level;
    EMF_dc_state *saved_state;
} enum_emh_data;

#define ENUM_GET_PRIVATE_DATA(ht) \
    ((enum_emh_data*)(((unsigned char*)(ht))-sizeof (enum_emh_data)))

#define WIDTH(rect) ( (rect).right - (rect).left )
#define HEIGHT(rect) ( (rect).bottom - (rect).top )

#define IS_WIN9X() (GetVersion()&0x80000000)

static void EMF_Update_MF_Xform(HDC hdc, const enum_emh_data *info)
{
    XFORM mapping_mode_trans, final_trans;
    double scaleX, scaleY;

    scaleX = (double)info->state.vportExtX / (double)info->state.wndExtX;
    scaleY = (double)info->state.vportExtY / (double)info->state.wndExtY;
    mapping_mode_trans.eM11 = scaleX;
    mapping_mode_trans.eM12 = 0.0;
    mapping_mode_trans.eM21 = 0.0;
    mapping_mode_trans.eM22 = scaleY;
    mapping_mode_trans.eDx  = (double)info->state.vportOrgX - scaleX * (double)info->state.wndOrgX;
    mapping_mode_trans.eDy  = (double)info->state.vportOrgY - scaleY * (double)info->state.wndOrgY;

    CombineTransform(&final_trans, &info->state.world_transform, &mapping_mode_trans);
    CombineTransform(&final_trans, &final_trans, &info->init_transform);

    if (!SetWorldTransform(hdc, &final_trans))
    {
        ERR("World transform failed!\n");
    }
}

static void EMF_RestoreDC( enum_emh_data *info, INT level )
{
    if (abs(level) > info->save_level || level == 0) return;

    if (level < 0) level = info->save_level + level + 1;

    while (info->save_level >= level)
    {
        EMF_dc_state *state = info->saved_state;
        info->saved_state = state->next;
        state->next = NULL;
        if (--info->save_level < level)
            info->state = *state;
        HeapFree( GetProcessHeap(), 0, state );
    }
}

static void EMF_SaveDC( enum_emh_data *info )
{
    EMF_dc_state *state = HeapAlloc( GetProcessHeap(), 0, sizeof(*state));
    if (state)
    {
        *state = info->state;
        state->next = info->saved_state;
        info->saved_state = state;
        info->save_level++;
        TRACE("save_level %d\n", info->save_level);
    }
}

static void EMF_SetMapMode(HDC hdc, enum_emh_data *info)
{
    INT horzSize = GetDeviceCaps( hdc, HORZSIZE );
    INT vertSize = GetDeviceCaps( hdc, VERTSIZE );
    INT horzRes  = GetDeviceCaps( hdc, HORZRES );
    INT vertRes  = GetDeviceCaps( hdc, VERTRES );

    TRACE("%d\n", info->state.mode);

    switch(info->state.mode)
    {
    case MM_TEXT:
        info->state.wndExtX   = 1;
        info->state.wndExtY   = 1;
        info->state.vportExtX = 1;
        info->state.vportExtY = 1;
        break;
    case MM_LOMETRIC:
    case MM_ISOTROPIC:
        info->state.wndExtX   = horzSize * 10;
        info->state.wndExtY   = vertSize * 10;
        info->state.vportExtX = horzRes;
        info->state.vportExtY = -vertRes;
        break;
    case MM_HIMETRIC:
        info->state.wndExtX   = horzSize * 100;
        info->state.wndExtY   = vertSize * 100;
        info->state.vportExtX = horzRes;
        info->state.vportExtY = -vertRes;
        break;
    case MM_LOENGLISH:
        info->state.wndExtX   = MulDiv(1000, horzSize, 254);
        info->state.wndExtY   = MulDiv(1000, vertSize, 254);
        info->state.vportExtX = horzRes;
        info->state.vportExtY = -vertRes;
        break;
    case MM_HIENGLISH:
        info->state.wndExtX   = MulDiv(10000, horzSize, 254);
        info->state.wndExtY   = MulDiv(10000, vertSize, 254);
        info->state.vportExtX = horzRes;
        info->state.vportExtY = -vertRes;
        break;
    case MM_TWIPS:
        info->state.wndExtX   = MulDiv(14400, horzSize, 254);
        info->state.wndExtY   = MulDiv(14400, vertSize, 254);
        info->state.vportExtX = horzRes;
        info->state.vportExtY = -vertRes;
        break;
    case MM_ANISOTROPIC:
        break;
    default:
        return;
    }
}

/***********************************************************************
 *           EMF_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */

static void EMF_FixIsotropic(HDC hdc, enum_emh_data *info)
{
    double xdim = fabs((double)info->state.vportExtX * GetDeviceCaps( hdc, HORZSIZE ) /
                  (GetDeviceCaps( hdc, HORZRES ) * info->state.wndExtX));
    double ydim = fabs((double)info->state.vportExtY * GetDeviceCaps( hdc, VERTSIZE ) /
                  (GetDeviceCaps( hdc, VERTRES ) * info->state.wndExtY));

    if (xdim > ydim)
    {
        INT mincx = (info->state.vportExtX >= 0) ? 1 : -1;
        info->state.vportExtX = floor(info->state.vportExtX * ydim / xdim + 0.5);
        if (!info->state.vportExtX) info->state.vportExtX = mincx;
    }
    else
    {
        INT mincy = (info->state.vportExtY >= 0) ? 1 : -1;
        info->state.vportExtY = floor(info->state.vportExtY * xdim / ydim + 0.5);
        if (!info->state.vportExtY) info->state.vportExtY = mincy;
    }
}

/*****************************************************************************
 *       emr_produces_output
 *
 * Returns TRUE if the record type writes something to the dc.  Used by
 * PlayEnhMetaFileRecord to determine whether it needs to update the
 * dc's xform when in win9x mode.
 *
 * FIXME: need to test which records should be here.
 */
static BOOL emr_produces_output(int type)
{
    switch(type) {
    case EMR_POLYBEZIER:
    case EMR_POLYGON:
    case EMR_POLYLINE:
    case EMR_POLYBEZIERTO:
    case EMR_POLYLINETO:
    case EMR_POLYPOLYLINE:
    case EMR_POLYPOLYGON:
    case EMR_SETPIXELV:
    case EMR_MOVETOEX:
    case EMR_EXCLUDECLIPRECT:
    case EMR_INTERSECTCLIPRECT:
    case EMR_SELECTOBJECT:
    case EMR_ANGLEARC:
    case EMR_ELLIPSE:
    case EMR_RECTANGLE:
    case EMR_ROUNDRECT:
    case EMR_ARC:
    case EMR_CHORD:
    case EMR_PIE:
    case EMR_EXTFLOODFILL:
    case EMR_LINETO:
    case EMR_ARCTO:
    case EMR_POLYDRAW:
    case EMR_GDICOMMENT:
    case EMR_FILLRGN:
    case EMR_FRAMERGN:
    case EMR_INVERTRGN:
    case EMR_PAINTRGN:
    case EMR_BITBLT:
    case EMR_STRETCHBLT:
    case EMR_MASKBLT:
    case EMR_PLGBLT:
    case EMR_SETDIBITSTODEVICE:
    case EMR_STRETCHDIBITS:
    case EMR_EXTTEXTOUTA:
    case EMR_EXTTEXTOUTW:
    case EMR_POLYBEZIER16:
    case EMR_POLYGON16:
    case EMR_POLYLINE16:
    case EMR_POLYBEZIERTO16:
    case EMR_POLYLINETO16:
    case EMR_POLYPOLYLINE16:
    case EMR_POLYPOLYGON16:
    case EMR_POLYDRAW16:
    case EMR_POLYTEXTOUTA:
    case EMR_POLYTEXTOUTW:
    case EMR_SMALLTEXTOUT:
    case EMR_ALPHABLEND:
    case EMR_TRANSPARENTBLT:
        return TRUE;
    default:
        return FALSE;
    }
}


/*****************************************************************************
 *           PlayEnhMetaFileRecord  (GDI32.@)
 *
 *  Render a single enhanced metafile record in the device context hdc.
 *
 *  RETURNS
 *    TRUE (non zero) on success, FALSE on error.
 *  BUGS
 *    Many unimplemented records.
 *    No error handling on record play failures (ie checking return codes)
 *
 * NOTES
 *    WinNT actually updates the current world transform in this function
 *     whereas Win9x does not.
 */
BOOL WINAPI PlayEnhMetaFileRecord(
     HDC hdc,                   /* [in] device context in which to render EMF record */
     LPHANDLETABLE handletable, /* [in] array of handles to be used in rendering record */
     const ENHMETARECORD *mr,   /* [in] EMF record to render */
     UINT handles               /* [in] size of handle array */
     )
{
  int type;
  RECT tmprc;
  enum_emh_data *info = ENUM_GET_PRIVATE_DATA(handletable);

  TRACE("hdc = %p, handletable = %p, record = %p, numHandles = %d\n",
        hdc, handletable, mr, handles);
  if (!mr) return FALSE;

  type = mr->iType;

  TRACE("record %s\n", get_emr_name(type));
  switch(type)
    {
    case EMR_HEADER:
      break;
    case EMR_EOF:
      break;
    case EMR_GDICOMMENT:
      {
        const EMRGDICOMMENT *lpGdiComment = (const EMRGDICOMMENT *)mr;
        /* In an enhanced metafile, there can be both public and private GDI comments */
        GdiComment( hdc, lpGdiComment->cbData, lpGdiComment->Data );
        break;
      }
    case EMR_SETMAPMODE:
      {
        const EMRSETMAPMODE *pSetMapMode = (const EMRSETMAPMODE *)mr;

        if (info->state.mode == pSetMapMode->iMode &&
            (info->state.mode == MM_ISOTROPIC || info->state.mode == MM_ANISOTROPIC))
            break;
        info->state.mode = pSetMapMode->iMode;
        EMF_SetMapMode(hdc, info);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

	break;
      }
    case EMR_SETBKMODE:
      {
        const EMRSETBKMODE *pSetBkMode = (const EMRSETBKMODE *)mr;
	SetBkMode(hdc, pSetBkMode->iMode);
	break;
      }
    case EMR_SETBKCOLOR:
      {
        const EMRSETBKCOLOR *pSetBkColor = (const EMRSETBKCOLOR *)mr;
	SetBkColor(hdc, pSetBkColor->crColor);
	break;
      }
    case EMR_SETPOLYFILLMODE:
      {
        const EMRSETPOLYFILLMODE *pSetPolyFillMode = (const EMRSETPOLYFILLMODE *)mr;
	SetPolyFillMode(hdc, pSetPolyFillMode->iMode);
	break;
      }
    case EMR_SETROP2:
      {
        const EMRSETROP2 *pSetROP2 = (const EMRSETROP2 *)mr;
	SetROP2(hdc, pSetROP2->iMode);
	break;
      }
    case EMR_SETSTRETCHBLTMODE:
      {
	const EMRSETSTRETCHBLTMODE *pSetStretchBltMode = (const EMRSETSTRETCHBLTMODE *)mr;
	SetStretchBltMode(hdc, pSetStretchBltMode->iMode);
	break;
      }
    case EMR_SETTEXTALIGN:
      {
	const EMRSETTEXTALIGN *pSetTextAlign = (const EMRSETTEXTALIGN *)mr;
	SetTextAlign(hdc, pSetTextAlign->iMode);
	break;
      }
    case EMR_SETTEXTCOLOR:
      {
	const EMRSETTEXTCOLOR *pSetTextColor = (const EMRSETTEXTCOLOR *)mr;
	SetTextColor(hdc, pSetTextColor->crColor);
	break;
      }
    case EMR_SAVEDC:
      {
        if (SaveDC( hdc ))
            EMF_SaveDC( info );
	break;
      }
    case EMR_RESTOREDC:
      {
	const EMRRESTOREDC *pRestoreDC = (const EMRRESTOREDC *)mr;
        TRACE("EMR_RESTORE: %d\n", pRestoreDC->iRelative);
        if (RestoreDC( hdc, pRestoreDC->iRelative ))
            EMF_RestoreDC( info, pRestoreDC->iRelative );
	break;
      }
    case EMR_INTERSECTCLIPRECT:
      {
	const EMRINTERSECTCLIPRECT *pClipRect = (const EMRINTERSECTCLIPRECT *)mr;
        TRACE("EMR_INTERSECTCLIPRECT: rect %d,%d - %d, %d\n",
              pClipRect->rclClip.left, pClipRect->rclClip.top,
              pClipRect->rclClip.right, pClipRect->rclClip.bottom);
        IntersectClipRect(hdc, pClipRect->rclClip.left, pClipRect->rclClip.top,
                          pClipRect->rclClip.right, pClipRect->rclClip.bottom);
	break;
      }
    case EMR_SELECTOBJECT:
      {
	const EMRSELECTOBJECT *pSelectObject = (const EMRSELECTOBJECT *)mr;
	if( pSelectObject->ihObject & 0x80000000 ) {
	  /* High order bit is set - it's a stock object
	   * Strip the high bit to get the index.
	   * See MSDN article Q142319
	   */
	  SelectObject( hdc, GetStockObject( pSelectObject->ihObject &
					     0x7fffffff ) );
	} else {
	  /* High order bit wasn't set - not a stock object
	   */
	      SelectObject( hdc,
			(handletable->objectHandle)[pSelectObject->ihObject] );
	}
	break;
      }
    case EMR_DELETEOBJECT:
      {
	const EMRDELETEOBJECT *pDeleteObject = (const EMRDELETEOBJECT *)mr;
	DeleteObject( (handletable->objectHandle)[pDeleteObject->ihObject]);
	(handletable->objectHandle)[pDeleteObject->ihObject] = 0;
	break;
      }
    case EMR_SETWINDOWORGEX:
      {
    	const EMRSETWINDOWORGEX *pSetWindowOrgEx = (const EMRSETWINDOWORGEX *)mr;

        info->state.wndOrgX = pSetWindowOrgEx->ptlOrigin.x;
        info->state.wndOrgY = pSetWindowOrgEx->ptlOrigin.y;

        TRACE("SetWindowOrgEx: %d,%d\n", info->state.wndOrgX, info->state.wndOrgY);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

        break;
      }
    case EMR_SETWINDOWEXTEX:
      {
	const EMRSETWINDOWEXTEX *pSetWindowExtEx = (const EMRSETWINDOWEXTEX *)mr;
	
        if (info->state.mode != MM_ISOTROPIC && info->state.mode != MM_ANISOTROPIC)
	    break;
        info->state.wndExtX = pSetWindowExtEx->szlExtent.cx;
        info->state.wndExtY = pSetWindowExtEx->szlExtent.cy;
        if (info->state.mode == MM_ISOTROPIC)
            EMF_FixIsotropic(hdc, info);

        TRACE("SetWindowExtEx: %d,%d\n",info->state.wndExtX, info->state.wndExtY);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

	break;
      }
    case EMR_SETVIEWPORTORGEX:
      {
	const EMRSETVIEWPORTORGEX *pSetViewportOrgEx = (const EMRSETVIEWPORTORGEX *)mr;

        info->state.vportOrgX = pSetViewportOrgEx->ptlOrigin.x;
        info->state.vportOrgY = pSetViewportOrgEx->ptlOrigin.y;
        TRACE("SetViewportOrgEx: %d,%d\n", info->state.vportOrgX, info->state.vportOrgY);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

	break;
      }
    case EMR_SETVIEWPORTEXTEX:
      {
	const EMRSETVIEWPORTEXTEX *pSetViewportExtEx = (const EMRSETVIEWPORTEXTEX *)mr;

        if (info->state.mode != MM_ISOTROPIC && info->state.mode != MM_ANISOTROPIC)
	    break;
        info->state.vportExtX = pSetViewportExtEx->szlExtent.cx;
        info->state.vportExtY = pSetViewportExtEx->szlExtent.cy;
        if (info->state.mode == MM_ISOTROPIC)
            EMF_FixIsotropic(hdc, info);
        TRACE("SetViewportExtEx: %d,%d\n", info->state.vportExtX, info->state.vportExtY);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

	break;
      }
    case EMR_CREATEPEN:
      {
	const EMRCREATEPEN *pCreatePen = (const EMRCREATEPEN *)mr;
	(handletable->objectHandle)[pCreatePen->ihPen] =
	  CreatePenIndirect(&pCreatePen->lopn);
	break;
      }
    case EMR_EXTCREATEPEN:
      {
	const EMREXTCREATEPEN *pPen = (const EMREXTCREATEPEN *)mr;
	LOGBRUSH lb;
	lb.lbStyle = pPen->elp.elpBrushStyle;
	lb.lbColor = pPen->elp.elpColor;
	lb.lbHatch = pPen->elp.elpHatch;

	if(pPen->offBmi || pPen->offBits)
	  FIXME("EMR_EXTCREATEPEN: Need to copy brush bitmap\n");

	(handletable->objectHandle)[pPen->ihPen] =
	  ExtCreatePen(pPen->elp.elpPenStyle, pPen->elp.elpWidth, &lb,
                       pPen->elp.elpNumEntries, pPen->elp.elpNumEntries ? pPen->elp.elpStyleEntry : NULL);
	break;
      }
    case EMR_CREATEBRUSHINDIRECT:
      {
	const EMRCREATEBRUSHINDIRECT *pBrush = (const EMRCREATEBRUSHINDIRECT *)mr;
        LOGBRUSH brush;
        brush.lbStyle = pBrush->lb.lbStyle;
        brush.lbColor = pBrush->lb.lbColor;
        brush.lbHatch = pBrush->lb.lbHatch;
        (handletable->objectHandle)[pBrush->ihBrush] = CreateBrushIndirect(&brush);
	break;
      }
    case EMR_EXTCREATEFONTINDIRECTW:
      {
	const EMREXTCREATEFONTINDIRECTW *pFont = (const EMREXTCREATEFONTINDIRECTW *)mr;
	(handletable->objectHandle)[pFont->ihFont] =
	  CreateFontIndirectW(&pFont->elfw.elfLogFont);
	break;
      }
    case EMR_MOVETOEX:
      {
	const EMRMOVETOEX *pMoveToEx = (const EMRMOVETOEX *)mr;
	MoveToEx(hdc, pMoveToEx->ptl.x, pMoveToEx->ptl.y, NULL);
	break;
      }
    case EMR_LINETO:
      {
	const EMRLINETO *pLineTo = (const EMRLINETO *)mr;
        LineTo(hdc, pLineTo->ptl.x, pLineTo->ptl.y);
	break;
      }
    case EMR_RECTANGLE:
      {
	const EMRRECTANGLE *pRect = (const EMRRECTANGLE *)mr;
	Rectangle(hdc, pRect->rclBox.left, pRect->rclBox.top,
		  pRect->rclBox.right, pRect->rclBox.bottom);
	break;
      }
    case EMR_ELLIPSE:
      {
	const EMRELLIPSE *pEllipse = (const EMRELLIPSE *)mr;
	Ellipse(hdc, pEllipse->rclBox.left, pEllipse->rclBox.top,
		pEllipse->rclBox.right, pEllipse->rclBox.bottom);
	break;
      }
    case EMR_POLYGON16:
      {
	const EMRPOLYGON16 *pPoly = (const EMRPOLYGON16 *)mr;
	/* Shouldn't use Polygon16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	{
	    pts[i].x = pPoly->apts[i].x;
	    pts[i].y = pPoly->apts[i].y;
	}
	Polygon(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYLINE16:
      {
	const EMRPOLYLINE16 *pPoly = (const EMRPOLYLINE16 *)mr;
	/* Shouldn't use Polyline16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	{
	    pts[i].x = pPoly->apts[i].x;
	    pts[i].y = pPoly->apts[i].y;
	}
	Polyline(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYLINETO16:
      {
	const EMRPOLYLINETO16 *pPoly = (const EMRPOLYLINETO16 *)mr;
	/* Shouldn't use PolylineTo16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	{
	    pts[i].x = pPoly->apts[i].x;
	    pts[i].y = pPoly->apts[i].y;
	}
	PolylineTo(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYBEZIER16:
      {
	const EMRPOLYBEZIER16 *pPoly = (const EMRPOLYBEZIER16 *)mr;
	/* Shouldn't use PolyBezier16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	{
	    pts[i].x = pPoly->apts[i].x;
	    pts[i].y = pPoly->apts[i].y;
	}
	PolyBezier(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYBEZIERTO16:
      {
	const EMRPOLYBEZIERTO16 *pPoly = (const EMRPOLYBEZIERTO16 *)mr;
	/* Shouldn't use PolyBezierTo16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	{
	    pts[i].x = pPoly->apts[i].x;
	    pts[i].y = pPoly->apts[i].y;
	}
	PolyBezierTo(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYPOLYGON16:
      {
        const EMRPOLYPOLYGON16 *pPolyPoly = (const EMRPOLYPOLYGON16 *)mr;
	/* NB POINTS array doesn't start at pPolyPoly->apts it's actually
	   pPolyPoly->aPolyCounts + pPolyPoly->nPolys */

        const POINTS *pts = (const POINTS *)(pPolyPoly->aPolyCounts + pPolyPoly->nPolys);
        POINT *pt = HeapAlloc( GetProcessHeap(), 0, pPolyPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPolyPoly->cpts; i++)
        {
            pt[i].x = pts[i].x;
            pt[i].y = pts[i].y;
        }
	PolyPolygon(hdc, pt, (const INT*)pPolyPoly->aPolyCounts, pPolyPoly->nPolys);
	HeapFree( GetProcessHeap(), 0, pt );
	break;
      }
    case EMR_POLYPOLYLINE16:
      {
        const EMRPOLYPOLYLINE16 *pPolyPoly = (const EMRPOLYPOLYLINE16 *)mr;
	/* NB POINTS array doesn't start at pPolyPoly->apts it's actually
	   pPolyPoly->aPolyCounts + pPolyPoly->nPolys */

        const POINTS *pts = (const POINTS *)(pPolyPoly->aPolyCounts + pPolyPoly->nPolys);
        POINT *pt = HeapAlloc( GetProcessHeap(), 0, pPolyPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPolyPoly->cpts; i++)
        {
            pt[i].x = pts[i].x;
            pt[i].y = pts[i].y;
        }
	PolyPolyline(hdc, pt, pPolyPoly->aPolyCounts, pPolyPoly->nPolys);
	HeapFree( GetProcessHeap(), 0, pt );
	break;
      }

    case EMR_POLYDRAW16:
    {
        const EMRPOLYDRAW16 *pPolyDraw16 = (const EMRPOLYDRAW16 *)mr;
        const POINTS *ptl = pPolyDraw16->apts;
        POINT *pts = HeapAlloc(GetProcessHeap(), 0, pPolyDraw16->cpts * sizeof(POINT));
        DWORD i;

        /* NB abTypes array doesn't start at pPolyDraw16->abTypes. It's actually
           pPolyDraw16->apts + pPolyDraw16->cpts. */
        const BYTE *types = (BYTE*)(pPolyDraw16->apts + pPolyDraw16->cpts);

        if (!pts)
            break;

        for (i = 0; i < pPolyDraw16->cpts; ++i)
        {
            pts[i].x = ptl[i].x;
            pts[i].y = ptl[i].y;
        }

        PolyDraw(hdc, pts, types, pPolyDraw16->cpts);
        HeapFree(GetProcessHeap(), 0, pts);
        break;
    }

    case EMR_STRETCHDIBITS:
      {
	const EMRSTRETCHDIBITS *pStretchDIBits = (const EMRSTRETCHDIBITS *)mr;

	StretchDIBits(hdc,
		      pStretchDIBits->xDest,
		      pStretchDIBits->yDest,
		      pStretchDIBits->cxDest,
		      pStretchDIBits->cyDest,
		      pStretchDIBits->xSrc,
		      pStretchDIBits->ySrc,
		      pStretchDIBits->cxSrc,
		      pStretchDIBits->cySrc,
		      (const BYTE *)mr + pStretchDIBits->offBitsSrc,
		      (const BITMAPINFO *)((const BYTE *)mr + pStretchDIBits->offBmiSrc),
		      pStretchDIBits->iUsageSrc,
		      pStretchDIBits->dwRop);
	break;
      }

    case EMR_EXTTEXTOUTA:
    {
	const EMREXTTEXTOUTA *pExtTextOutA = (const EMREXTTEXTOUTA *)mr;
	RECT rc;
        const INT *dx = NULL;
        int old_mode;

	rc.left = pExtTextOutA->emrtext.rcl.left;
	rc.top = pExtTextOutA->emrtext.rcl.top;
	rc.right = pExtTextOutA->emrtext.rcl.right;
	rc.bottom = pExtTextOutA->emrtext.rcl.bottom;
        TRACE("EMR_EXTTEXTOUTA: x,y = %d, %d. rect = %s. flags %08x\n",
              pExtTextOutA->emrtext.ptlReference.x, pExtTextOutA->emrtext.ptlReference.y,
              wine_dbgstr_rect(&rc), pExtTextOutA->emrtext.fOptions);

        old_mode = SetGraphicsMode(hdc, pExtTextOutA->iGraphicsMode);
        /* Reselect the font back into the dc so that the transformation
           gets updated. */
        SelectObject(hdc, GetCurrentObject(hdc, OBJ_FONT));

        /* Linux version of pstoedit produces EMFs with offDx set to 0.
         * These files can be enumerated and played under Win98 just
         * fine, but at least Win2k chokes on them.
         */
        if (pExtTextOutA->emrtext.offDx)
            dx = (const INT *)((const BYTE *)mr + pExtTextOutA->emrtext.offDx);

	ExtTextOutA(hdc, pExtTextOutA->emrtext.ptlReference.x, pExtTextOutA->emrtext.ptlReference.y,
	    pExtTextOutA->emrtext.fOptions, &rc,
	    (LPCSTR)((const BYTE *)mr + pExtTextOutA->emrtext.offString), pExtTextOutA->emrtext.nChars,
	    dx);

        SetGraphicsMode(hdc, old_mode);
	break;
    }

    case EMR_EXTTEXTOUTW:
    {
	const EMREXTTEXTOUTW *pExtTextOutW = (const EMREXTTEXTOUTW *)mr;
	RECT rc;
        const INT *dx = NULL;
        int old_mode;

	rc.left = pExtTextOutW->emrtext.rcl.left;
	rc.top = pExtTextOutW->emrtext.rcl.top;
	rc.right = pExtTextOutW->emrtext.rcl.right;
	rc.bottom = pExtTextOutW->emrtext.rcl.bottom;
        TRACE("EMR_EXTTEXTOUTW: x,y = %d, %d.  rect = %s. flags %08x\n",
              pExtTextOutW->emrtext.ptlReference.x, pExtTextOutW->emrtext.ptlReference.y,
              wine_dbgstr_rect(&rc), pExtTextOutW->emrtext.fOptions);

        old_mode = SetGraphicsMode(hdc, pExtTextOutW->iGraphicsMode);
        /* Reselect the font back into the dc so that the transformation
           gets updated. */
        SelectObject(hdc, GetCurrentObject(hdc, OBJ_FONT));

        /* Linux version of pstoedit produces EMFs with offDx set to 0.
         * These files can be enumerated and played under Win98 just
         * fine, but at least Win2k chokes on them.
         */
        if (pExtTextOutW->emrtext.offDx)
            dx = (const INT *)((const BYTE *)mr + pExtTextOutW->emrtext.offDx);

	ExtTextOutW(hdc, pExtTextOutW->emrtext.ptlReference.x, pExtTextOutW->emrtext.ptlReference.y,
	    pExtTextOutW->emrtext.fOptions, &rc,
	    (LPCWSTR)((const BYTE *)mr + pExtTextOutW->emrtext.offString), pExtTextOutW->emrtext.nChars,
	    dx);

        SetGraphicsMode(hdc, old_mode);
	break;
    }

    case EMR_CREATEPALETTE:
      {
	const EMRCREATEPALETTE *lpCreatePal = (const EMRCREATEPALETTE *)mr;

	(handletable->objectHandle)[ lpCreatePal->ihPal ] =
		CreatePalette( &lpCreatePal->lgpl );

	break;
      }

    case EMR_SELECTPALETTE:
      {
	const EMRSELECTPALETTE *lpSelectPal = (const EMRSELECTPALETTE *)mr;

	if( lpSelectPal->ihPal & 0x80000000 ) {
		SelectPalette( hdc, GetStockObject(lpSelectPal->ihPal & 0x7fffffff), TRUE);
	} else {
		SelectPalette( hdc, (handletable->objectHandle)[lpSelectPal->ihPal], TRUE);
	}
	break;
      }

    case EMR_REALIZEPALETTE:
      {
	RealizePalette( hdc );
	break;
      }

    case EMR_EXTSELECTCLIPRGN:
      {
	const EMREXTSELECTCLIPRGN *lpRgn = (const EMREXTSELECTCLIPRGN *)mr;
#ifdef __REACTOS__
	const RGNDATA *pRgnData = (const RGNDATA *)lpRgn->RgnData;
	DWORD dwSize = sizeof(RGNDATAHEADER) + pRgnData->rdh.nCount * sizeof(RECT);
#endif
	HRGN hRgn = 0;

        if (mr->nSize >= sizeof(*lpRgn) + sizeof(RGNDATAHEADER))
#ifdef __REACTOS__
            hRgn = ExtCreateRegion( &info->init_transform, dwSize, pRgnData );
#else
            hRgn = ExtCreateRegion( &info->init_transform, 0, (const RGNDATA *)lpRgn->RgnData );
#endif
	ExtSelectClipRgn(hdc, hRgn, (INT)(lpRgn->iMode));
	/* ExtSelectClipRgn created a copy of the region */
	DeleteObject(hRgn);
        break;
      }

    case EMR_SETMETARGN:
      {
        SetMetaRgn( hdc );
        break;
      }

    case EMR_SETWORLDTRANSFORM:
      {
        const EMRSETWORLDTRANSFORM *lpXfrm = (const EMRSETWORLDTRANSFORM *)mr;
        info->state.world_transform = lpXfrm->xform;

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

        break;
      }

    case EMR_POLYBEZIER:
      {
        const EMRPOLYBEZIER *lpPolyBez = (const EMRPOLYBEZIER *)mr;
        PolyBezier(hdc, (const POINT*)lpPolyBez->aptl, (UINT)lpPolyBez->cptl);
        break;
      }

    case EMR_POLYGON:
      {
        const EMRPOLYGON *lpPoly = (const EMRPOLYGON *)mr;
        Polygon( hdc, (const POINT*)lpPoly->aptl, (UINT)lpPoly->cptl );
        break;
      }

    case EMR_POLYLINE:
      {
        const EMRPOLYLINE *lpPolyLine = (const EMRPOLYLINE *)mr;
        Polyline(hdc, (const POINT*)lpPolyLine->aptl, (UINT)lpPolyLine->cptl);
        break;
      }

    case EMR_POLYBEZIERTO:
      {
        const EMRPOLYBEZIERTO *lpPolyBezierTo = (const EMRPOLYBEZIERTO *)mr;
        PolyBezierTo( hdc, (const POINT*)lpPolyBezierTo->aptl,
		      (UINT)lpPolyBezierTo->cptl );
        break;
      }

    case EMR_POLYLINETO:
      {
        const EMRPOLYLINETO *lpPolyLineTo = (const EMRPOLYLINETO *)mr;
        PolylineTo( hdc, (const POINT*)lpPolyLineTo->aptl,
		    (UINT)lpPolyLineTo->cptl );
        break;
      }

    case EMR_POLYPOLYLINE:
      {
        const EMRPOLYPOLYLINE *pPolyPolyline = (const EMRPOLYPOLYLINE *)mr;
	/* NB Points at pPolyPolyline->aPolyCounts + pPolyPolyline->nPolys */

        PolyPolyline(hdc, (const POINT*)(pPolyPolyline->aPolyCounts +
				    pPolyPolyline->nPolys),
		     pPolyPolyline->aPolyCounts,
		     pPolyPolyline->nPolys );

        break;
      }

    case EMR_POLYPOLYGON:
      {
        const EMRPOLYPOLYGON *pPolyPolygon = (const EMRPOLYPOLYGON *)mr;

	/* NB Points at pPolyPolygon->aPolyCounts + pPolyPolygon->nPolys */

        PolyPolygon(hdc, (const POINT*)(pPolyPolygon->aPolyCounts +
				   pPolyPolygon->nPolys),
		    (const INT*)pPolyPolygon->aPolyCounts, pPolyPolygon->nPolys );
        break;
      }

    case EMR_SETBRUSHORGEX:
      {
        const EMRSETBRUSHORGEX *lpSetBrushOrgEx = (const EMRSETBRUSHORGEX *)mr;

        SetBrushOrgEx( hdc,
                       (INT)lpSetBrushOrgEx->ptlOrigin.x,
                       (INT)lpSetBrushOrgEx->ptlOrigin.y,
                       NULL );

        break;
      }

    case EMR_SETPIXELV:
      {
        const EMRSETPIXELV *lpSetPixelV = (const EMRSETPIXELV *)mr;

        SetPixelV( hdc,
                   (INT)lpSetPixelV->ptlPixel.x,
                   (INT)lpSetPixelV->ptlPixel.y,
                   lpSetPixelV->crColor );

        break;
      }

    case EMR_SETMAPPERFLAGS:
      {
        const EMRSETMAPPERFLAGS *lpSetMapperFlags = (const EMRSETMAPPERFLAGS *)mr;

        SetMapperFlags( hdc, lpSetMapperFlags->dwFlags );

        break;
      }

    case EMR_SETCOLORADJUSTMENT:
      {
        const EMRSETCOLORADJUSTMENT *lpSetColorAdjust = (const EMRSETCOLORADJUSTMENT *)mr;

        SetColorAdjustment( hdc, &lpSetColorAdjust->ColorAdjustment );

        break;
      }

    case EMR_OFFSETCLIPRGN:
      {
        const EMROFFSETCLIPRGN *lpOffsetClipRgn = (const EMROFFSETCLIPRGN *)mr;

        OffsetClipRgn( hdc,
                       (INT)lpOffsetClipRgn->ptlOffset.x,
                       (INT)lpOffsetClipRgn->ptlOffset.y );
        FIXME("OffsetClipRgn\n");

        break;
      }

    case EMR_EXCLUDECLIPRECT:
      {
        const EMREXCLUDECLIPRECT *lpExcludeClipRect = (const EMREXCLUDECLIPRECT *)mr;

        ExcludeClipRect( hdc,
                         lpExcludeClipRect->rclClip.left,
                         lpExcludeClipRect->rclClip.top,
                         lpExcludeClipRect->rclClip.right,
                         lpExcludeClipRect->rclClip.bottom  );
        FIXME("ExcludeClipRect\n");

         break;
      }

    case EMR_SCALEVIEWPORTEXTEX:
      {
        const EMRSCALEVIEWPORTEXTEX *lpScaleViewportExtEx = (const EMRSCALEVIEWPORTEXTEX *)mr;

        if ((info->state.mode != MM_ISOTROPIC) && (info->state.mode != MM_ANISOTROPIC))
	    break;
        if (!lpScaleViewportExtEx->xNum || !lpScaleViewportExtEx->xDenom ||
            !lpScaleViewportExtEx->yNum || !lpScaleViewportExtEx->yDenom)
            break;
        info->state.vportExtX = MulDiv(info->state.vportExtX, lpScaleViewportExtEx->xNum,
                                 lpScaleViewportExtEx->xDenom);
        info->state.vportExtY = MulDiv(info->state.vportExtY, lpScaleViewportExtEx->yNum,
                                 lpScaleViewportExtEx->yDenom);
        if (info->state.vportExtX == 0) info->state.vportExtX = 1;
        if (info->state.vportExtY == 0) info->state.vportExtY = 1;
        if (info->state.mode == MM_ISOTROPIC)
            EMF_FixIsotropic(hdc, info);

        TRACE("EMRSCALEVIEWPORTEXTEX %d/%d %d/%d\n",
             lpScaleViewportExtEx->xNum,lpScaleViewportExtEx->xDenom,
             lpScaleViewportExtEx->yNum,lpScaleViewportExtEx->yDenom);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

        break;
      }

    case EMR_SCALEWINDOWEXTEX:
      {
        const EMRSCALEWINDOWEXTEX *lpScaleWindowExtEx = (const EMRSCALEWINDOWEXTEX *)mr;

        if ((info->state.mode != MM_ISOTROPIC) && (info->state.mode != MM_ANISOTROPIC))
	    break;
        if (!lpScaleWindowExtEx->xNum || !lpScaleWindowExtEx->xDenom ||
            !lpScaleWindowExtEx->yNum || !lpScaleWindowExtEx->yDenom)
            break;
        info->state.wndExtX = MulDiv(info->state.wndExtX, lpScaleWindowExtEx->xNum,
                               lpScaleWindowExtEx->xDenom);
        info->state.wndExtY = MulDiv(info->state.wndExtY, lpScaleWindowExtEx->yNum,
                               lpScaleWindowExtEx->yDenom);
        if (info->state.wndExtX == 0) info->state.wndExtX = 1;
        if (info->state.wndExtY == 0) info->state.wndExtY = 1;
        if (info->state.mode == MM_ISOTROPIC)
            EMF_FixIsotropic(hdc, info);

        TRACE("EMRSCALEWINDOWEXTEX %d/%d %d/%d\n",
             lpScaleWindowExtEx->xNum,lpScaleWindowExtEx->xDenom,
             lpScaleWindowExtEx->yNum,lpScaleWindowExtEx->yDenom);

        if (!IS_WIN9X())
            EMF_Update_MF_Xform(hdc, info);

        break;
      }

    case EMR_MODIFYWORLDTRANSFORM:
      {
        const EMRMODIFYWORLDTRANSFORM *lpModifyWorldTrans = (const EMRMODIFYWORLDTRANSFORM *)mr;

        switch(lpModifyWorldTrans->iMode) {
        case MWT_IDENTITY:
            info->state.world_transform.eM11 = info->state.world_transform.eM22 = 1;
            info->state.world_transform.eM12 = info->state.world_transform.eM21 = 0;
            info->state.world_transform.eDx  = info->state.world_transform.eDy  = 0;
            if (!IS_WIN9X())
                EMF_Update_MF_Xform(hdc, info);
            break;
        case MWT_LEFTMULTIPLY:
            CombineTransform(&info->state.world_transform, &lpModifyWorldTrans->xform,
                             &info->state.world_transform);
            if (!IS_WIN9X())
                ModifyWorldTransform(hdc, &lpModifyWorldTrans->xform, MWT_LEFTMULTIPLY);
            break;
        case MWT_RIGHTMULTIPLY:
            CombineTransform(&info->state.world_transform, &info->state.world_transform,
                             &lpModifyWorldTrans->xform);
            if (!IS_WIN9X())
                EMF_Update_MF_Xform(hdc, info);
            break;
        default:
            FIXME("Unknown imode %d\n", lpModifyWorldTrans->iMode);
            break;
        }
        break;
      }

    case EMR_ANGLEARC:
      {
        const EMRANGLEARC *lpAngleArc = (const EMRANGLEARC *)mr;

        AngleArc( hdc,
                 (INT)lpAngleArc->ptlCenter.x, (INT)lpAngleArc->ptlCenter.y,
                 lpAngleArc->nRadius, lpAngleArc->eStartAngle,
                 lpAngleArc->eSweepAngle );

        break;
      }

    case EMR_ROUNDRECT:
      {
        const EMRROUNDRECT *lpRoundRect = (const EMRROUNDRECT *)mr;

        RoundRect( hdc,
                   lpRoundRect->rclBox.left,
                   lpRoundRect->rclBox.top,
                   lpRoundRect->rclBox.right,
                   lpRoundRect->rclBox.bottom,
                   lpRoundRect->szlCorner.cx,
                   lpRoundRect->szlCorner.cy );

        break;
      }

    case EMR_ARC:
      {
        const EMRARC *lpArc = (const EMRARC *)mr;

        Arc( hdc,
             (INT)lpArc->rclBox.left,
             (INT)lpArc->rclBox.top,
             (INT)lpArc->rclBox.right,
             (INT)lpArc->rclBox.bottom,
             (INT)lpArc->ptlStart.x,
             (INT)lpArc->ptlStart.y,
             (INT)lpArc->ptlEnd.x,
             (INT)lpArc->ptlEnd.y );

        break;
      }

    case EMR_CHORD:
      {
        const EMRCHORD *lpChord = (const EMRCHORD *)mr;

        Chord( hdc,
             (INT)lpChord->rclBox.left,
             (INT)lpChord->rclBox.top,
             (INT)lpChord->rclBox.right,
             (INT)lpChord->rclBox.bottom,
             (INT)lpChord->ptlStart.x,
             (INT)lpChord->ptlStart.y,
             (INT)lpChord->ptlEnd.x,
             (INT)lpChord->ptlEnd.y );

        break;
      }

    case EMR_PIE:
      {
        const EMRPIE *lpPie = (const EMRPIE *)mr;

        Pie( hdc,
             (INT)lpPie->rclBox.left,
             (INT)lpPie->rclBox.top,
             (INT)lpPie->rclBox.right,
             (INT)lpPie->rclBox.bottom,
             (INT)lpPie->ptlStart.x,
             (INT)lpPie->ptlStart.y,
             (INT)lpPie->ptlEnd.x,
             (INT)lpPie->ptlEnd.y );

       break;
      }

    case EMR_ARCTO:
      {
        const EMRARC *lpArcTo = (const EMRARC *)mr;

        ArcTo( hdc,
               (INT)lpArcTo->rclBox.left,
               (INT)lpArcTo->rclBox.top,
               (INT)lpArcTo->rclBox.right,
               (INT)lpArcTo->rclBox.bottom,
               (INT)lpArcTo->ptlStart.x,
               (INT)lpArcTo->ptlStart.y,
               (INT)lpArcTo->ptlEnd.x,
               (INT)lpArcTo->ptlEnd.y );

        break;
      }

    case EMR_EXTFLOODFILL:
      {
        const EMREXTFLOODFILL *lpExtFloodFill = (const EMREXTFLOODFILL *)mr;

        ExtFloodFill( hdc,
                      (INT)lpExtFloodFill->ptlStart.x,
                      (INT)lpExtFloodFill->ptlStart.y,
                      lpExtFloodFill->crColor,
                      (UINT)lpExtFloodFill->iMode );

        break;
      }

    case EMR_POLYDRAW:
      {
        const EMRPOLYDRAW *lpPolyDraw = (const EMRPOLYDRAW *)mr;
        PolyDraw( hdc,
                  (const POINT*)lpPolyDraw->aptl,
                  lpPolyDraw->abTypes,
                  (INT)lpPolyDraw->cptl );

        break;
      }

    case EMR_SETARCDIRECTION:
      {
        const EMRSETARCDIRECTION *lpSetArcDirection = (const EMRSETARCDIRECTION *)mr;
        SetArcDirection( hdc, (INT)lpSetArcDirection->iArcDirection );
        break;
      }

    case EMR_SETMITERLIMIT:
      {
        const EMRSETMITERLIMIT *lpSetMiterLimit = (const EMRSETMITERLIMIT *)mr;
        SetMiterLimit( hdc, lpSetMiterLimit->eMiterLimit, NULL );
        break;
      }

    case EMR_BEGINPATH:
      {
        BeginPath( hdc );
        break;
      }

    case EMR_ENDPATH:
      {
        EndPath( hdc );
        break;
      }

    case EMR_CLOSEFIGURE:
      {
        CloseFigure( hdc );
        break;
      }

    case EMR_FILLPATH:
      {
        /*const EMRFILLPATH lpFillPath = (const EMRFILLPATH *)mr;*/
        FillPath( hdc );
        break;
      }

    case EMR_STROKEANDFILLPATH:
      {
        /*const EMRSTROKEANDFILLPATH lpStrokeAndFillPath = (const EMRSTROKEANDFILLPATH *)mr;*/
        StrokeAndFillPath( hdc );
        break;
      }

    case EMR_STROKEPATH:
      {
        /*const EMRSTROKEPATH lpStrokePath = (const EMRSTROKEPATH *)mr;*/
        StrokePath( hdc );
        break;
      }

    case EMR_FLATTENPATH:
      {
        FlattenPath( hdc );
        break;
      }

    case EMR_WIDENPATH:
      {
        WidenPath( hdc );
        break;
      }

    case EMR_SELECTCLIPPATH:
      {
        const EMRSELECTCLIPPATH *lpSelectClipPath = (const EMRSELECTCLIPPATH *)mr;
        SelectClipPath( hdc, (INT)lpSelectClipPath->iMode );
        break;
      }

    case EMR_ABORTPATH:
      {
        AbortPath( hdc );
        break;
      }

    case EMR_CREATECOLORSPACE:
      {
        PEMRCREATECOLORSPACE lpCreateColorSpace = (PEMRCREATECOLORSPACE)mr;
        (handletable->objectHandle)[lpCreateColorSpace->ihCS] =
           CreateColorSpaceA( &lpCreateColorSpace->lcs );
        break;
      }

    case EMR_SETCOLORSPACE:
      {
        const EMRSETCOLORSPACE *lpSetColorSpace = (const EMRSETCOLORSPACE *)mr;
        SetColorSpace( hdc,
                       (handletable->objectHandle)[lpSetColorSpace->ihCS] );
        break;
      }

    case EMR_DELETECOLORSPACE:
      {
        const EMRDELETECOLORSPACE *lpDeleteColorSpace = (const EMRDELETECOLORSPACE *)mr;
        DeleteColorSpace( (handletable->objectHandle)[lpDeleteColorSpace->ihCS] );
        break;
      }

    case EMR_SETICMMODE:
      {
        const EMRSETICMMODE *lpSetICMMode = (const EMRSETICMMODE *)mr;
        SetICMMode( hdc, (INT)lpSetICMMode->iMode );
        break;
      }

    case EMR_PIXELFORMAT:
      {
        INT iPixelFormat;
        const EMRPIXELFORMAT *lpPixelFormat = (const EMRPIXELFORMAT *)mr;

        iPixelFormat = ChoosePixelFormat( hdc, &lpPixelFormat->pfd );
        SetPixelFormat( hdc, iPixelFormat, &lpPixelFormat->pfd );

        break;
      }

    case EMR_SETPALETTEENTRIES:
      {
        const EMRSETPALETTEENTRIES *lpSetPaletteEntries = (const EMRSETPALETTEENTRIES *)mr;

        SetPaletteEntries( (handletable->objectHandle)[lpSetPaletteEntries->ihPal],
                           (UINT)lpSetPaletteEntries->iStart,
                           (UINT)lpSetPaletteEntries->cEntries,
                           lpSetPaletteEntries->aPalEntries );

        break;
      }

    case EMR_RESIZEPALETTE:
      {
        const EMRRESIZEPALETTE *lpResizePalette = (const EMRRESIZEPALETTE *)mr;

        ResizePalette( (handletable->objectHandle)[lpResizePalette->ihPal],
                       (UINT)lpResizePalette->cEntries );

        break;
      }

    case EMR_CREATEDIBPATTERNBRUSHPT:
      {
        const EMRCREATEDIBPATTERNBRUSHPT *lpCreate = (const EMRCREATEDIBPATTERNBRUSHPT *)mr;
        LPVOID lpPackedStruct;

        /* Check that offsets and data are contained within the record
         * (including checking for wrap-arounds).
         */
        if (    lpCreate->offBmi  + lpCreate->cbBmi  > mr->nSize
             || lpCreate->offBits + lpCreate->cbBits > mr->nSize
             || lpCreate->offBmi  + lpCreate->cbBmi  < lpCreate->offBmi
             || lpCreate->offBits + lpCreate->cbBits < lpCreate->offBits )
        {
            ERR("Invalid EMR_CREATEDIBPATTERNBRUSHPT record\n");
            break;
        }

        /* This is a BITMAPINFO struct followed directly by bitmap bits */
        lpPackedStruct = HeapAlloc( GetProcessHeap(), 0,
                                    lpCreate->cbBmi + lpCreate->cbBits );
        if(!lpPackedStruct)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        /* Now pack this structure */
        memcpy( lpPackedStruct,
                ((const BYTE *)lpCreate) + lpCreate->offBmi,
                lpCreate->cbBmi );
        memcpy( ((BYTE*)lpPackedStruct) + lpCreate->cbBmi,
                ((const BYTE *)lpCreate) + lpCreate->offBits,
                lpCreate->cbBits );

        (handletable->objectHandle)[lpCreate->ihBrush] =
           CreateDIBPatternBrushPt( lpPackedStruct,
                                    (UINT)lpCreate->iUsage );

        HeapFree(GetProcessHeap(), 0, lpPackedStruct);
        break;
      }

    case EMR_CREATEMONOBRUSH:
    {
        const EMRCREATEMONOBRUSH *pCreateMonoBrush = (const EMRCREATEMONOBRUSH *)mr;
        const BITMAPINFO *pbi = (const BITMAPINFO *)((const BYTE *)mr + pCreateMonoBrush->offBmi);
        HBITMAP hBmp;

        /* Need to check if the bitmap is monochrome, and if the
           two colors are really black and white */
        if (pCreateMonoBrush->iUsage == DIB_PAL_MONO)
        {
            BITMAP bm;

            /* Undocumented iUsage indicates a mono bitmap with no palette table,
             * aligned to 32 rather than 16 bits.
             */
            bm.bmType = 0;
            bm.bmWidth = pbi->bmiHeader.biWidth;
            bm.bmHeight = abs(pbi->bmiHeader.biHeight);
            bm.bmWidthBytes = 4 * ((pbi->bmiHeader.biWidth + 31) / 32);
            bm.bmPlanes = pbi->bmiHeader.biPlanes;
            bm.bmBitsPixel = pbi->bmiHeader.biBitCount;
            bm.bmBits = (BYTE *)mr + pCreateMonoBrush->offBits;
            hBmp = CreateBitmapIndirect(&bm);
        }
        else if (is_dib_monochrome(pbi))
        {
          /* Top-down DIBs have a negative height */
          LONG height = pbi->bmiHeader.biHeight;

          hBmp = CreateBitmap(pbi->bmiHeader.biWidth, abs(height), 1, 1, NULL);
          SetDIBits(hdc, hBmp, 0, pbi->bmiHeader.biHeight,
              (const BYTE *)mr + pCreateMonoBrush->offBits, pbi, pCreateMonoBrush->iUsage);
        }
        else
        {
            hBmp = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)pbi, CBM_INIT,
              (const BYTE *)mr + pCreateMonoBrush->offBits, pbi, pCreateMonoBrush->iUsage);
        }

	(handletable->objectHandle)[pCreateMonoBrush->ihBrush] = CreatePatternBrush(hBmp);

	/* CreatePatternBrush created a copy of the bitmap */
	DeleteObject(hBmp);
	break;
    }

    case EMR_BITBLT:
    {
	const EMRBITBLT *pBitBlt = (const EMRBITBLT *)mr;

        if(pBitBlt->offBmiSrc == 0) { /* Record is a PatBlt */
            PatBlt(hdc, pBitBlt->xDest, pBitBlt->yDest, pBitBlt->cxDest, pBitBlt->cyDest,
                   pBitBlt->dwRop);
        } else { /* BitBlt */
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HBRUSH hBrush, hBrushOld;
            HBITMAP hBmp = 0, hBmpOld = 0;
            const BITMAPINFO *pbi = (const BITMAPINFO *)((const BYTE *)mr + pBitBlt->offBmiSrc);

            SetGraphicsMode(hdcSrc, GM_ADVANCED);
            SetWorldTransform(hdcSrc, &pBitBlt->xformSrc);

            hBrush = CreateSolidBrush(pBitBlt->crBkColorSrc);
            hBrushOld = SelectObject(hdcSrc, hBrush);
            PatBlt(hdcSrc, pBitBlt->rclBounds.left, pBitBlt->rclBounds.top,
                   pBitBlt->rclBounds.right - pBitBlt->rclBounds.left,
                   pBitBlt->rclBounds.bottom - pBitBlt->rclBounds.top, PATCOPY);
            SelectObject(hdcSrc, hBrushOld);
            DeleteObject(hBrush);

            hBmp = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)pbi, CBM_INIT,
                                  (const BYTE *)mr + pBitBlt->offBitsSrc, pbi, pBitBlt->iUsageSrc);
            hBmpOld = SelectObject(hdcSrc, hBmp);

            BitBlt(hdc, pBitBlt->xDest, pBitBlt->yDest, pBitBlt->cxDest, pBitBlt->cyDest,
                   hdcSrc, pBitBlt->xSrc, pBitBlt->ySrc, pBitBlt->dwRop);

            SelectObject(hdcSrc, hBmpOld);
            DeleteObject(hBmp);
            DeleteDC(hdcSrc);
        }
	break;
    }

    case EMR_STRETCHBLT:
    {
	const EMRSTRETCHBLT *pStretchBlt = (const EMRSTRETCHBLT *)mr;

        TRACE("EMR_STRETCHBLT: %d, %d %dx%d -> %d, %d %dx%d. rop %08x offBitsSrc %d\n",
	       pStretchBlt->xSrc, pStretchBlt->ySrc, pStretchBlt->cxSrc, pStretchBlt->cySrc,
	       pStretchBlt->xDest, pStretchBlt->yDest, pStretchBlt->cxDest, pStretchBlt->cyDest,
	       pStretchBlt->dwRop, pStretchBlt->offBitsSrc);

        if(pStretchBlt->offBmiSrc == 0) { /* Record is a PatBlt */
            PatBlt(hdc, pStretchBlt->xDest, pStretchBlt->yDest, pStretchBlt->cxDest, pStretchBlt->cyDest,
                   pStretchBlt->dwRop);
        } else { /* StretchBlt */
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HBRUSH hBrush, hBrushOld;
            HBITMAP hBmp = 0, hBmpOld = 0;
            const BITMAPINFO *pbi = (const BITMAPINFO *)((const BYTE *)mr + pStretchBlt->offBmiSrc);

            SetGraphicsMode(hdcSrc, GM_ADVANCED);
            SetWorldTransform(hdcSrc, &pStretchBlt->xformSrc);

            hBrush = CreateSolidBrush(pStretchBlt->crBkColorSrc);
            hBrushOld = SelectObject(hdcSrc, hBrush);
            PatBlt(hdcSrc, pStretchBlt->rclBounds.left, pStretchBlt->rclBounds.top,
                   pStretchBlt->rclBounds.right - pStretchBlt->rclBounds.left,
                   pStretchBlt->rclBounds.bottom - pStretchBlt->rclBounds.top, PATCOPY);
            SelectObject(hdcSrc, hBrushOld);
            DeleteObject(hBrush);

            hBmp = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)pbi, CBM_INIT,
                                  (const BYTE *)mr + pStretchBlt->offBitsSrc, pbi, pStretchBlt->iUsageSrc);
            hBmpOld = SelectObject(hdcSrc, hBmp);

            StretchBlt(hdc, pStretchBlt->xDest, pStretchBlt->yDest, pStretchBlt->cxDest, pStretchBlt->cyDest,
                       hdcSrc, pStretchBlt->xSrc, pStretchBlt->ySrc, pStretchBlt->cxSrc, pStretchBlt->cySrc,
                       pStretchBlt->dwRop);

            SelectObject(hdcSrc, hBmpOld);
            DeleteObject(hBmp);
            DeleteDC(hdcSrc);
        }
	break;
    }

    case EMR_ALPHABLEND:
    {
	const EMRALPHABLEND *pAlphaBlend = (const EMRALPHABLEND *)mr;

        TRACE("EMR_ALPHABLEND: %d, %d %dx%d -> %d, %d %dx%d. blendfn %08x offBitsSrc %d\n",
	       pAlphaBlend->xSrc, pAlphaBlend->ySrc, pAlphaBlend->cxSrc, pAlphaBlend->cySrc,
	       pAlphaBlend->xDest, pAlphaBlend->yDest, pAlphaBlend->cxDest, pAlphaBlend->cyDest,
	       pAlphaBlend->dwRop, pAlphaBlend->offBitsSrc);

        if(pAlphaBlend->offBmiSrc == 0) {
            FIXME("EMR_ALPHABLEND: offBmiSrc == 0\n");
        } else {
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HBITMAP hBmp = 0, hBmpOld = 0;
            const BITMAPINFO *pbi = (const BITMAPINFO *)((const BYTE *)mr + pAlphaBlend->offBmiSrc);
            void *bits;

            SetGraphicsMode(hdcSrc, GM_ADVANCED);
            SetWorldTransform(hdcSrc, &pAlphaBlend->xformSrc);

            hBmp = CreateDIBSection(hdc, pbi, pAlphaBlend->iUsageSrc, &bits, NULL, 0);
            memcpy(bits, (const BYTE *)mr + pAlphaBlend->offBitsSrc, pAlphaBlend->cbBitsSrc);
            hBmpOld = SelectObject(hdcSrc, hBmp);

            GdiAlphaBlend(hdc, pAlphaBlend->xDest, pAlphaBlend->yDest, pAlphaBlend->cxDest, pAlphaBlend->cyDest,
                       hdcSrc, pAlphaBlend->xSrc, pAlphaBlend->ySrc, pAlphaBlend->cxSrc, pAlphaBlend->cySrc,
                          *(BLENDFUNCTION *)&pAlphaBlend->dwRop);

            SelectObject(hdcSrc, hBmpOld);
            DeleteObject(hBmp);
            DeleteDC(hdcSrc);
        }
	break;
    }

    case EMR_MASKBLT:
    {
	const EMRMASKBLT *pMaskBlt = (const EMRMASKBLT *)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp, hBmpOld, hBmpMask;
	const BITMAPINFO *pbi;

        SetGraphicsMode(hdcSrc, GM_ADVANCED);
	SetWorldTransform(hdcSrc, &pMaskBlt->xformSrc);

	hBrush = CreateSolidBrush(pMaskBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pMaskBlt->rclBounds.left, pMaskBlt->rclBounds.top,
	       pMaskBlt->rclBounds.right - pMaskBlt->rclBounds.left,
	       pMaskBlt->rclBounds.bottom - pMaskBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	pbi = (const BITMAPINFO *)((const BYTE *)mr + pMaskBlt->offBmiMask);
	hBmpMask = CreateBitmap(pbi->bmiHeader.biWidth, pbi->bmiHeader.biHeight,
	             1, 1, NULL);
	SetDIBits(hdc, hBmpMask, 0, pbi->bmiHeader.biHeight,
	  (const BYTE *)mr + pMaskBlt->offBitsMask, pbi, pMaskBlt->iUsageMask);

	pbi = (const BITMAPINFO *)((const BYTE *)mr + pMaskBlt->offBmiSrc);
	hBmp = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (const BYTE *)mr + pMaskBlt->offBitsSrc, pbi, pMaskBlt->iUsageSrc);
	hBmpOld = SelectObject(hdcSrc, hBmp);
	MaskBlt(hdc,
		pMaskBlt->xDest,
	        pMaskBlt->yDest,
	        pMaskBlt->cxDest,
	        pMaskBlt->cyDest,
	        hdcSrc,
	        pMaskBlt->xSrc,
	        pMaskBlt->ySrc,
	        hBmpMask,
		pMaskBlt->xMask,
		pMaskBlt->yMask,
	        pMaskBlt->dwRop);
	SelectObject(hdcSrc, hBmpOld);
	DeleteObject(hBmp);
	DeleteObject(hBmpMask);
	DeleteDC(hdcSrc);
	break;
    }

    case EMR_PLGBLT:
    {
	const EMRPLGBLT *pPlgBlt = (const EMRPLGBLT *)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp, hBmpOld, hBmpMask;
	const BITMAPINFO *pbi;
	POINT pts[3];

        SetGraphicsMode(hdcSrc, GM_ADVANCED);
	SetWorldTransform(hdcSrc, &pPlgBlt->xformSrc);

	pts[0].x = pPlgBlt->aptlDest[0].x; pts[0].y = pPlgBlt->aptlDest[0].y;
	pts[1].x = pPlgBlt->aptlDest[1].x; pts[1].y = pPlgBlt->aptlDest[1].y;
	pts[2].x = pPlgBlt->aptlDest[2].x; pts[2].y = pPlgBlt->aptlDest[2].y;

	hBrush = CreateSolidBrush(pPlgBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pPlgBlt->rclBounds.left, pPlgBlt->rclBounds.top,
	       pPlgBlt->rclBounds.right - pPlgBlt->rclBounds.left,
	       pPlgBlt->rclBounds.bottom - pPlgBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	pbi = (const BITMAPINFO *)((const BYTE *)mr + pPlgBlt->offBmiMask);
	hBmpMask = CreateBitmap(pbi->bmiHeader.biWidth, pbi->bmiHeader.biHeight,
	             1, 1, NULL);
	SetDIBits(hdc, hBmpMask, 0, pbi->bmiHeader.biHeight,
	  (const BYTE *)mr + pPlgBlt->offBitsMask, pbi, pPlgBlt->iUsageMask);

	pbi = (const BITMAPINFO *)((const BYTE *)mr + pPlgBlt->offBmiSrc);
	hBmp = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (const BYTE *)mr + pPlgBlt->offBitsSrc, pbi, pPlgBlt->iUsageSrc);
	hBmpOld = SelectObject(hdcSrc, hBmp);
	PlgBlt(hdc,
	       pts,
	       hdcSrc,
	       pPlgBlt->xSrc,
	       pPlgBlt->ySrc,
	       pPlgBlt->cxSrc,
	       pPlgBlt->cySrc,
	       hBmpMask,
	       pPlgBlt->xMask,
	       pPlgBlt->yMask);
	SelectObject(hdcSrc, hBmpOld);
	DeleteObject(hBmp);
	DeleteObject(hBmpMask);
	DeleteDC(hdcSrc);
	break;
    }

    case EMR_SETDIBITSTODEVICE:
    {
	const EMRSETDIBITSTODEVICE *pSetDIBitsToDevice = (const EMRSETDIBITSTODEVICE *)mr;

	SetDIBitsToDevice(hdc,
			  pSetDIBitsToDevice->xDest,
			  pSetDIBitsToDevice->yDest,
			  pSetDIBitsToDevice->cxSrc,
			  pSetDIBitsToDevice->cySrc,
			  pSetDIBitsToDevice->xSrc,
			  pSetDIBitsToDevice->ySrc,
			  pSetDIBitsToDevice->iStartScan,
			  pSetDIBitsToDevice->cScans,
			  (const BYTE *)mr + pSetDIBitsToDevice->offBitsSrc,
			  (const BITMAPINFO *)((const BYTE *)mr + pSetDIBitsToDevice->offBmiSrc),
			  pSetDIBitsToDevice->iUsageSrc);
	break;
    }

    case EMR_POLYTEXTOUTA:
    {
	const EMRPOLYTEXTOUTA *pPolyTextOutA = (const EMRPOLYTEXTOUTA *)mr;
	POLYTEXTA *polytextA = HeapAlloc(GetProcessHeap(), 0, pPolyTextOutA->cStrings * sizeof(POLYTEXTA));
	LONG i;
	XFORM xform, xformOld;
	int gModeOld;

	gModeOld = SetGraphicsMode(hdc, pPolyTextOutA->iGraphicsMode);
	GetWorldTransform(hdc, &xformOld);

	xform.eM11 = pPolyTextOutA->exScale;
	xform.eM12 = 0.0;
	xform.eM21 = 0.0;
	xform.eM22 = pPolyTextOutA->eyScale;
	xform.eDx = 0.0;
	xform.eDy = 0.0;
	SetWorldTransform(hdc, &xform);

	/* Set up POLYTEXTA structures */
	for(i = 0; i < pPolyTextOutA->cStrings; i++)
	{
	    polytextA[i].x = pPolyTextOutA->aemrtext[i].ptlReference.x;
	    polytextA[i].y = pPolyTextOutA->aemrtext[i].ptlReference.y;
	    polytextA[i].n = pPolyTextOutA->aemrtext[i].nChars;
	    polytextA[i].lpstr = (LPCSTR)((const BYTE *)mr + pPolyTextOutA->aemrtext[i].offString);
	    polytextA[i].uiFlags = pPolyTextOutA->aemrtext[i].fOptions;
	    polytextA[i].rcl.left = pPolyTextOutA->aemrtext[i].rcl.left;
	    polytextA[i].rcl.right = pPolyTextOutA->aemrtext[i].rcl.right;
	    polytextA[i].rcl.top = pPolyTextOutA->aemrtext[i].rcl.top;
	    polytextA[i].rcl.bottom = pPolyTextOutA->aemrtext[i].rcl.bottom;
	    polytextA[i].pdx = (int *)((BYTE *)mr + pPolyTextOutA->aemrtext[i].offDx);
	}
	PolyTextOutA(hdc, polytextA, pPolyTextOutA->cStrings);
	HeapFree(GetProcessHeap(), 0, polytextA);

	SetWorldTransform(hdc, &xformOld);
	SetGraphicsMode(hdc, gModeOld);
	break;
    }

    case EMR_POLYTEXTOUTW:
    {
	const EMRPOLYTEXTOUTW *pPolyTextOutW = (const EMRPOLYTEXTOUTW *)mr;
	POLYTEXTW *polytextW = HeapAlloc(GetProcessHeap(), 0, pPolyTextOutW->cStrings * sizeof(POLYTEXTW));
	LONG i;
	XFORM xform, xformOld;
	int gModeOld;

	gModeOld = SetGraphicsMode(hdc, pPolyTextOutW->iGraphicsMode);
	GetWorldTransform(hdc, &xformOld);

	xform.eM11 = pPolyTextOutW->exScale;
	xform.eM12 = 0.0;
	xform.eM21 = 0.0;
	xform.eM22 = pPolyTextOutW->eyScale;
	xform.eDx = 0.0;
	xform.eDy = 0.0;
	SetWorldTransform(hdc, &xform);

	/* Set up POLYTEXTW structures */
	for(i = 0; i < pPolyTextOutW->cStrings; i++)
	{
	    polytextW[i].x = pPolyTextOutW->aemrtext[i].ptlReference.x;
	    polytextW[i].y = pPolyTextOutW->aemrtext[i].ptlReference.y;
	    polytextW[i].n = pPolyTextOutW->aemrtext[i].nChars;
	    polytextW[i].lpstr = (LPCWSTR)((const BYTE *)mr + pPolyTextOutW->aemrtext[i].offString);
	    polytextW[i].uiFlags = pPolyTextOutW->aemrtext[i].fOptions;
	    polytextW[i].rcl.left = pPolyTextOutW->aemrtext[i].rcl.left;
	    polytextW[i].rcl.right = pPolyTextOutW->aemrtext[i].rcl.right;
	    polytextW[i].rcl.top = pPolyTextOutW->aemrtext[i].rcl.top;
	    polytextW[i].rcl.bottom = pPolyTextOutW->aemrtext[i].rcl.bottom;
	    polytextW[i].pdx = (int *)((BYTE *)mr + pPolyTextOutW->aemrtext[i].offDx);
	}
	PolyTextOutW(hdc, polytextW, pPolyTextOutW->cStrings);
	HeapFree(GetProcessHeap(), 0, polytextW);

	SetWorldTransform(hdc, &xformOld);
	SetGraphicsMode(hdc, gModeOld);
	break;
    }

    case EMR_FILLRGN:
    {
	const EMRFILLRGN *pFillRgn = (const EMRFILLRGN *)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pFillRgn->cbRgnData, (const RGNDATA *)pFillRgn->RgnData);
	FillRgn(hdc,
		hRgn,
		(handletable->objectHandle)[pFillRgn->ihBrush]);
	DeleteObject(hRgn);
	break;
    }

    case EMR_FRAMERGN:
    {
	const EMRFRAMERGN *pFrameRgn = (const EMRFRAMERGN *)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pFrameRgn->cbRgnData, (const RGNDATA *)pFrameRgn->RgnData);
	FrameRgn(hdc,
		 hRgn,
		 (handletable->objectHandle)[pFrameRgn->ihBrush],
		 pFrameRgn->szlStroke.cx,
		 pFrameRgn->szlStroke.cy);
	DeleteObject(hRgn);
	break;
    }

    case EMR_INVERTRGN:
    {
	const EMRINVERTRGN *pInvertRgn = (const EMRINVERTRGN *)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pInvertRgn->cbRgnData, (const RGNDATA *)pInvertRgn->RgnData);
	InvertRgn(hdc, hRgn);
	DeleteObject(hRgn);
	break;
    }

    case EMR_PAINTRGN:
    {
	const EMRPAINTRGN *pPaintRgn = (const EMRPAINTRGN *)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pPaintRgn->cbRgnData, (const RGNDATA *)pPaintRgn->RgnData);
	PaintRgn(hdc, hRgn);
	DeleteObject(hRgn);
	break;
    }

    case EMR_SETTEXTJUSTIFICATION:
    {
	const EMRSETTEXTJUSTIFICATION *pSetTextJust = (const EMRSETTEXTJUSTIFICATION *)mr;
	SetTextJustification(hdc, pSetTextJust->nBreakExtra, pSetTextJust->nBreakCount);
	break;
    }

    case EMR_SETLAYOUT:
    {
	const EMRSETLAYOUT *pSetLayout = (const EMRSETLAYOUT *)mr;
	SetLayout(hdc, pSetLayout->iMode);
	break;
    }

    case EMR_GRADIENTFILL:
    {
        EMRGRADIENTFILL *grad = (EMRGRADIENTFILL *)mr;
        GdiGradientFill( hdc, grad->Ver, grad->nVer, grad->Ver + grad->nVer,
                         grad->nTri, grad->ulMode );
        break;
    }

    case EMR_DRAWESCAPE:
    {
        PEMRESCAPE pemr = (PEMRESCAPE)mr;
        DrawEscape( hdc, pemr->iEsc, pemr->cjIn, (LPCSTR)pemr->Data );
        break;
    }

    case EMR_EXTESCAPE:
    {
        PEMRESCAPE pemr = (PEMRESCAPE)mr;
        ExtEscape( hdc, pemr->iEsc, pemr->cjIn, (LPCSTR)pemr->Data, 0, NULL );
        break;
    }

    case EMR_NAMEDESCAPE:
    {
        PEMRNAMEDESCAPE pemr = (PEMRNAMEDESCAPE)mr;
        INT rounded_size = (pemr->cjIn+3) & ~3;
        NamedEscape( hdc, (PWCHAR)&pemr->Data[rounded_size], pemr->iEsc, pemr->cjIn, (LPSTR)pemr->Data, 0, NULL );
        break;
    }

    case EMR_GLSRECORD:
    case EMR_GLSBOUNDEDRECORD:
    case EMR_STARTDOC:
    case EMR_SMALLTEXTOUT:
    case EMR_FORCEUFIMAPPING:
    case EMR_COLORCORRECTPALETTE:
    case EMR_SETICMPROFILEA:
    case EMR_SETICMPROFILEW:
    case EMR_TRANSPARENTBLT:
    case EMR_SETLINKEDUFI:
    case EMR_COLORMATCHTOTARGETW:
    case EMR_CREATECOLORSPACEW:

    default:
      /* From docs: If PlayEnhMetaFileRecord doesn't recognize a
                    record then ignore and return TRUE. */
      FIXME("type %d is unimplemented\n", type);
      break;
    }
  tmprc.left = tmprc.top = 0;
  tmprc.right = tmprc.bottom = 1000;
  LPtoDP(hdc, (POINT*)&tmprc, 2);
  TRACE("L:0,0 - 1000,1000 -> D:%s\n", wine_dbgstr_rect(&tmprc));

  return TRUE;
}

/*****************************************************************************
 *
 *        EnumEnhMetaFile  (GDI32.@)
 *
 *  Walk an enhanced metafile, calling a user-specified function _EnhMetaFunc_
 *  for each
 *  record. Returns when either every record has been used or
 *  when _EnhMetaFunc_ returns FALSE.
 *
 *
 * RETURNS
 *  TRUE if every record is used, FALSE if any invocation of _EnhMetaFunc_
 *  returns FALSE.
 *
 * BUGS
 *   Ignores rect.
 *
 * NOTES
 *   This function behaves differently in Win9x and WinNT.
 *
 *   In WinNT, the DC's world transform is updated as the EMF changes
 *    the Window/Viewport Extent and Origin or its world transform.
 *    The actual Window/Viewport Extent and Origin are left untouched.
 *
 *   In Win9x, the DC is left untouched, and PlayEnhMetaFileRecord
 *    updates the scaling itself but only just before a record that
 *    writes anything to the DC.
 *
 *   I'm not sure where the data (enum_emh_data) is stored in either
 *    version. For this implementation, it is stored before the handle
 *    table, but it could be stored in the DC, in the EMF handle or in
 *    TLS.
 *             MJM  5 Oct 2002
 */
BOOL WINAPI EnumEnhMetaFile(
     HDC hdc,                /* [in] device context to pass to _EnhMetaFunc_ */
     HENHMETAFILE hmf,       /* [in] EMF to walk */
     ENHMFENUMPROC callback, /* [in] callback function */
     LPVOID data,            /* [in] optional data for callback function */
     const RECT *lpRect      /* [in] bounding rectangle for rendered metafile */
    )
{
    BOOL ret;
    ENHMETAHEADER *emh;
    ENHMETARECORD *emr;
    DWORD offset;
    UINT i;
    HANDLETABLE *ht;
    INT savedMode = 0;
    XFORM savedXform;
    HPEN hPen = NULL;
    HBRUSH hBrush = NULL;
    HFONT hFont = NULL;
    HRGN hRgn = NULL;
    enum_emh_data *info;
    SIZE vp_size, win_size;
    POINT vp_org, win_org;
    INT mapMode = MM_TEXT, old_align = 0, old_rop2 = 0, old_arcdir = 0, old_polyfill = 0, old_stretchblt = 0;
    COLORREF old_text_color = 0, old_bk_color = 0;

    if(!lpRect && hdc)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    info = HeapAlloc( GetProcessHeap(), 0,
		    sizeof (enum_emh_data) + sizeof(HANDLETABLE) * emh->nHandles );
    if(!info)
    {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return FALSE;
    }
    info->state.mode = MM_TEXT;
    info->state.wndOrgX = 0;
    info->state.wndOrgY = 0;
    info->state.wndExtX = 1;
    info->state.wndExtY = 1;
    info->state.vportOrgX = 0;
    info->state.vportOrgY = 0;
    info->state.vportExtX = 1;
    info->state.vportExtY = 1;
    info->state.world_transform.eM11 = info->state.world_transform.eM22 = 1;
    info->state.world_transform.eM12 = info->state.world_transform.eM21 = 0;
    info->state.world_transform.eDx  = info->state.world_transform.eDy =  0;

    info->state.next = NULL;
    info->save_level = 0;
    info->saved_state = NULL;
    info->init_transform = info->state.world_transform;

    ht = (HANDLETABLE*) &info[1];
    ht->objectHandle[0] = hmf;
    for(i = 1; i < emh->nHandles; i++)
        ht->objectHandle[i] = NULL;

    if(hdc && !is_meta_dc( hdc ))
    {
	savedMode = SetGraphicsMode(hdc, GM_ADVANCED);
	GetWorldTransform(hdc, &savedXform);
        GetViewportExtEx(hdc, &vp_size);
        GetWindowExtEx(hdc, &win_size);
        GetViewportOrgEx(hdc, &vp_org);
        GetWindowOrgEx(hdc, &win_org);
        mapMode = GetMapMode(hdc);

	/* save DC */
	hPen = GetCurrentObject(hdc, OBJ_PEN);
	hBrush = GetCurrentObject(hdc, OBJ_BRUSH);
	hFont = GetCurrentObject(hdc, OBJ_FONT);

        hRgn = CreateRectRgn(0, 0, 0, 0);
        if (!GetClipRgn(hdc, hRgn))
        {
            DeleteObject(hRgn);
            hRgn = 0;
        }

        old_text_color = SetTextColor(hdc, RGB(0,0,0));
        old_bk_color = SetBkColor(hdc, RGB(0xff, 0xff, 0xff));
        old_align = SetTextAlign(hdc, 0);
        old_rop2 = SetROP2(hdc, R2_COPYPEN);
        old_arcdir = SetArcDirection(hdc, AD_COUNTERCLOCKWISE);
        old_polyfill = SetPolyFillMode(hdc, ALTERNATE);
        old_stretchblt = SetStretchBltMode(hdc, BLACKONWHITE);

        if (!IS_WIN9X() )
        {
            /* WinNT combines the vp/win ext/org info into a transform */
            double xscale, yscale;
            xscale = (double)vp_size.cx / (double)win_size.cx;
            yscale = (double)vp_size.cy / (double)win_size.cy;
            info->init_transform.eM11 = xscale;
            info->init_transform.eM12 = 0.0;
            info->init_transform.eM21 = 0.0;
            info->init_transform.eM22 = yscale;
            info->init_transform.eDx  = (double)vp_org.x - xscale * (double)win_org.x;
            info->init_transform.eDy  = (double)vp_org.y - yscale * (double)win_org.y;

            CombineTransform(&info->init_transform, &savedXform, &info->init_transform);
        }

        if ( lpRect && WIDTH(emh->rclFrame) && HEIGHT(emh->rclFrame) )
        {
            double xSrcPixSize, ySrcPixSize, xscale, yscale;
            XFORM xform;

            TRACE("rect: %s. rclFrame: (%d,%d)-(%d,%d)\n", wine_dbgstr_rect(lpRect),
               emh->rclFrame.left, emh->rclFrame.top, emh->rclFrame.right,
               emh->rclFrame.bottom);

            xSrcPixSize = (double) emh->szlMillimeters.cx / emh->szlDevice.cx;
            ySrcPixSize = (double) emh->szlMillimeters.cy / emh->szlDevice.cy;
            xscale = (double) WIDTH(*lpRect) * 100.0 /
                     WIDTH(emh->rclFrame) * xSrcPixSize;
            yscale = (double) HEIGHT(*lpRect) * 100.0 /
                     HEIGHT(emh->rclFrame) * ySrcPixSize;
            TRACE("xscale = %f, yscale = %f\n", xscale, yscale);

            xform.eM11 = xscale;
            xform.eM12 = 0;
            xform.eM21 = 0;
            xform.eM22 = yscale;
            xform.eDx = (double) lpRect->left - (double) WIDTH(*lpRect) / WIDTH(emh->rclFrame) * emh->rclFrame.left;
            xform.eDy = (double) lpRect->top - (double) HEIGHT(*lpRect) / HEIGHT(emh->rclFrame) * emh->rclFrame.top;

            CombineTransform(&info->init_transform, &xform, &info->init_transform);
        }

        /* WinNT resets the current vp/win org/ext */
        if ( !IS_WIN9X() )
        {
            SetMapMode(hdc, MM_TEXT);
            SetWindowOrgEx(hdc, 0, 0, NULL);
            SetViewportOrgEx(hdc, 0, 0, NULL);
            EMF_Update_MF_Xform(hdc, info);
        }
    }

    ret = TRUE;
    offset = 0;
    while(ret && offset < emh->nBytes)
    {
	emr = (ENHMETARECORD *)((char *)emh + offset);

        if (offset + 8 > emh->nBytes ||
            offset > offset + emr->nSize ||
            offset + emr->nSize > emh->nBytes)
        {
            WARN("record truncated\n");
            break;
        }

        /* In Win9x mode we update the xform if the record will produce output */
        if (hdc && IS_WIN9X() && emr_produces_output(emr->iType))
            EMF_Update_MF_Xform(hdc, info);

	TRACE("Calling EnumFunc with record %s, size %d\n", get_emr_name(emr->iType), emr->nSize);
	ret = (*callback)(hdc, ht, emr, emh->nHandles, (LPARAM)data);
	offset += emr->nSize;
    }

    if (hdc && !is_meta_dc( hdc ))
    {
        SetStretchBltMode(hdc, old_stretchblt);
        SetPolyFillMode(hdc, old_polyfill);
        SetArcDirection(hdc, old_arcdir);
        SetROP2(hdc, old_rop2);
        SetTextAlign(hdc, old_align);
        SetBkColor(hdc, old_bk_color);
        SetTextColor(hdc, old_text_color);

	/* restore DC */
	SelectObject(hdc, hBrush);
	SelectObject(hdc, hPen);
	SelectObject(hdc, hFont);
        ExtSelectClipRgn(hdc, hRgn, RGN_COPY);
        DeleteObject(hRgn);

	SetWorldTransform(hdc, &savedXform);
	if (savedMode)
	    SetGraphicsMode(hdc, savedMode);
        SetMapMode(hdc, mapMode);
        SetWindowOrgEx(hdc, win_org.x, win_org.y, NULL);
        SetWindowExtEx(hdc, win_size.cx, win_size.cy, NULL);
        SetViewportOrgEx(hdc, vp_org.x, vp_org.y, NULL);
        SetViewportExtEx(hdc, vp_size.cx, vp_size.cy, NULL);
    }

    for(i = 1; i < emh->nHandles; i++) /* Don't delete element 0 (hmf) */
        if( (ht->objectHandle)[i] )
	    DeleteObject( (ht->objectHandle)[i] );

    while (info->saved_state)
    {
        EMF_dc_state *state = info->saved_state;
        info->saved_state = info->saved_state->next;
        HeapFree( GetProcessHeap(), 0, state );
    }
    HeapFree( GetProcessHeap(), 0, info );
    return ret;
}

static INT CALLBACK EMF_PlayEnhMetaFileCallback(HDC hdc, HANDLETABLE *ht,
						const ENHMETARECORD *emr,
						INT handles, LPARAM data)
{
    return PlayEnhMetaFileRecord(hdc, ht, emr, handles);
}

/**************************************************************************
 *    PlayEnhMetaFile  (GDI32.@)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI PlayEnhMetaFile(
       HDC hdc,           /* [in] DC to render into */
       HENHMETAFILE hmf,  /* [in] metafile to render */
       const RECT *lpRect /* [in] rectangle to place metafile inside */
      )
{
    return EnumEnhMetaFile(hdc, hmf, EMF_PlayEnhMetaFileCallback, NULL, lpRect);
}

/*****************************************************************************
 *  DeleteEnhMetaFile (GDI32.@)
 *
 *  Deletes an enhanced metafile and frees the associated storage.
 */
BOOL WINAPI DeleteEnhMetaFile(HENHMETAFILE hmf)
{
    return EMF_Delete_HENHMETAFILE( hmf );
}

/*****************************************************************************
 *  CopyEnhMetaFileA (GDI32.@)
 *
 * Duplicate an enhanced metafile.
 *
 *
 */
HENHMETAFILE WINAPI CopyEnhMetaFileA(
    HENHMETAFILE hmfSrc,
    LPCSTR file)
{
    ENHMETAHEADER *emrSrc = EMF_GetEnhMetaHeader( hmfSrc ), *emrDst;
    HENHMETAFILE hmfDst;

    if(!emrSrc) return FALSE;
    if (!file) {
        emrDst = HeapAlloc( GetProcessHeap(), 0, emrSrc->nBytes );
	memcpy( emrDst, emrSrc, emrSrc->nBytes );
	hmfDst = EMF_Create_HENHMETAFILE( emrDst, emrSrc->nBytes, FALSE );
	if (!hmfDst)
		HeapFree( GetProcessHeap(), 0, emrDst );
    } else {
        HANDLE hFile;
        DWORD w;
        hFile = CreateFileA( file, GENERIC_WRITE | GENERIC_READ, 0,
			     NULL, CREATE_ALWAYS, 0, 0);
	WriteFile( hFile, emrSrc, emrSrc->nBytes, &w, NULL);
	CloseHandle( hFile );
	/* Reopen file for reading only, so that apps can share
	   read access to the file while hmf is still valid */
        hFile = CreateFileA( file, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE) {
	    ERR("Can't reopen emf for reading\n");
	    return 0;
	}
	hmfDst = EMF_GetEnhMetaFile( hFile );
        CloseHandle( hFile );
    }
    return hmfDst;
}

/*****************************************************************************
 *  CopyEnhMetaFileW (GDI32.@)
 *
 * See CopyEnhMetaFileA.
 *
 *
 */
HENHMETAFILE WINAPI CopyEnhMetaFileW(
    HENHMETAFILE hmfSrc,
    LPCWSTR file)
{
    ENHMETAHEADER *emrSrc = EMF_GetEnhMetaHeader( hmfSrc ), *emrDst;
    HENHMETAFILE hmfDst;

    if(!emrSrc) return FALSE;
    if (!file) {
        emrDst = HeapAlloc( GetProcessHeap(), 0, emrSrc->nBytes );
	memcpy( emrDst, emrSrc, emrSrc->nBytes );
	hmfDst = EMF_Create_HENHMETAFILE( emrDst, emrSrc->nBytes, FALSE );
	if (!hmfDst)
		HeapFree( GetProcessHeap(), 0, emrDst );
    } else {
        HANDLE hFile;
        DWORD w;
        hFile = CreateFileW( file, GENERIC_WRITE | GENERIC_READ, 0,
			     NULL, CREATE_ALWAYS, 0, 0);
	WriteFile( hFile, emrSrc, emrSrc->nBytes, &w, NULL);
	CloseHandle( hFile );
	/* Reopen file for reading only, so that apps can share
	   read access to the file while hmf is still valid */
        hFile = CreateFileW( file, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE) {
	    ERR("Can't reopen emf for reading\n");
	    return 0;
	}
	hmfDst = EMF_GetEnhMetaFile( hFile );
        CloseHandle( hFile );
    }
    return hmfDst;
}


/* Struct to be used to be passed in the LPVOID parameter for cbEnhPaletteCopy */
typedef struct tagEMF_PaletteCopy
{
   UINT cEntries;
   LPPALETTEENTRY lpPe;
} EMF_PaletteCopy;

/***************************************************************
 * Find the EMR_EOF record and then use it to find the
 * palette entries for this enhanced metafile.
 * The lpData is actually a pointer to an EMF_PaletteCopy struct
 * which contains the max number of elements to copy and where
 * to copy them to.
 *
 * NOTE: To be used by GetEnhMetaFilePaletteEntries only!
 */
static INT CALLBACK cbEnhPaletteCopy( HDC a,
                               HANDLETABLE *b,
                               const ENHMETARECORD *lpEMR,
                               INT c,
                               LPARAM lpData )
{

  if ( lpEMR->iType == EMR_EOF )
  {
    const EMREOF *lpEof = (const EMREOF *)lpEMR;
    EMF_PaletteCopy* info = (EMF_PaletteCopy*)lpData;
    DWORD dwNumPalToCopy = min( lpEof->nPalEntries, info->cEntries );

    TRACE( "copying 0x%08x palettes\n", dwNumPalToCopy );

    memcpy( info->lpPe, (LPCSTR)lpEof + lpEof->offPalEntries,
            sizeof( *(info->lpPe) ) * dwNumPalToCopy );

    /* Update the passed data as a return code */
    info->lpPe     = NULL; /* Palettes were copied! */
    info->cEntries = dwNumPalToCopy;

    return FALSE; /* That's all we need */
  }

  return TRUE;
}

/*****************************************************************************
 *  GetEnhMetaFilePaletteEntries (GDI32.@)
 *
 *  Copy the palette and report size
 *
 *  BUGS: Error codes (SetLastError) are not set on failures
 */
UINT WINAPI GetEnhMetaFilePaletteEntries( HENHMETAFILE hEmf,
					  UINT cEntries,
					  LPPALETTEENTRY lpPe )
{
  ENHMETAHEADER* enhHeader = EMF_GetEnhMetaHeader( hEmf );
  EMF_PaletteCopy infoForCallBack;

  TRACE( "(%p,%d,%p)\n", hEmf, cEntries, lpPe );

  if (!enhHeader) return 0;

  /* First check if there are any palettes associated with
     this metafile. */
  if ( enhHeader->nPalEntries == 0 ) return 0;

  /* Is the user requesting the number of palettes? */
  if ( lpPe == NULL ) return enhHeader->nPalEntries;

  /* Copy cEntries worth of PALETTEENTRY structs into the buffer */
  infoForCallBack.cEntries = cEntries;
  infoForCallBack.lpPe     = lpPe;

  if ( !EnumEnhMetaFile( 0, hEmf, cbEnhPaletteCopy,
                         &infoForCallBack, 0 ) )
      return GDI_ERROR;

  /* Verify that the callback executed correctly */
  if ( infoForCallBack.lpPe != NULL )
  {
     /* Callback proc had error! */
     ERR( "cbEnhPaletteCopy didn't execute correctly\n" );
     return GDI_ERROR;
  }

  return infoForCallBack.cEntries;
}

/******************************************************************
 *             extract_emf_from_comment
 *
 * If the WMF was created by GetWinMetaFileBits, then extract the
 * original EMF that is stored in MFCOMMENT chunks.
 */
static HENHMETAFILE extract_emf_from_comment( const BYTE *buf, UINT mf_size )
{
    METAHEADER *mh = (METAHEADER *)buf;
    METARECORD *mr;
    emf_in_wmf_comment *chunk;
    WORD checksum = 0;
    DWORD size = 0, remaining, chunks;
    BYTE *emf_bits = NULL, *ptr;
    UINT offset;
    HENHMETAFILE emf = NULL;

    if (mf_size < sizeof(*mh)) return NULL;

    for (offset = mh->mtHeaderSize * 2; offset < mf_size; offset += (mr->rdSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        chunk = (emf_in_wmf_comment *)(mr->rdParm + 2);

        if (mr->rdFunction != META_ESCAPE || mr->rdParm[0] != MFCOMMENT) goto done;
        if (chunk->magic != WMFC_MAGIC) goto done;

        if (!emf_bits)
        {
            size = remaining = chunk->emf_size;
            chunks = chunk->num_chunks;
            emf_bits = ptr = HeapAlloc( GetProcessHeap(), 0, size );
            if (!emf_bits) goto done;
        }
        if (chunk->chunk_size > remaining) goto done;
        remaining -= chunk->chunk_size;
        if (chunk->remaining_size != remaining) goto done;
        memcpy( ptr, chunk->emf_data, chunk->chunk_size );
        ptr += chunk->chunk_size;
        if (--chunks == 0) break;
    }

    for (offset = 0; offset < mf_size / 2; offset++)
        checksum += *((WORD *)buf + offset);
    if (checksum) goto done;

    emf = SetEnhMetaFileBits( size, emf_bits );

done:
    HeapFree( GetProcessHeap(), 0, emf_bits );
    return emf;
}

typedef struct wmf_in_emf_comment
{
    DWORD ident;
    DWORD iComment;
    DWORD nVersion;
    DWORD nChecksum;
    DWORD fFlags;
    DWORD cbWinMetaFile;
} wmf_in_emf_comment;

/******************************************************************
 *         SetWinMetaFileBits   (GDI32.@)
 *
 *         Translate from old style to new style.
 *
 */
HENHMETAFILE WINAPI SetWinMetaFileBits(UINT cbBuffer, const BYTE *lpbBuffer, HDC hdcRef,
                                       const METAFILEPICT *lpmfp)
{
    static const WCHAR szDisplayW[] = { 'D','I','S','P','L','A','Y','\0' };
    HMETAFILE hmf = NULL;
    HENHMETAFILE ret = NULL;
    HDC hdc = NULL, hdcdisp = NULL;
    RECT rc, *prcFrame = NULL;
    LONG mm, xExt, yExt;
    INT horzsize, vertsize, horzres, vertres;

    TRACE("(%d, %p, %p, %p)\n", cbBuffer, lpbBuffer, hdcRef, lpmfp);

    hmf = SetMetaFileBitsEx(cbBuffer, lpbBuffer);
    if(!hmf)
    {
        WARN("SetMetaFileBitsEx failed\n");
        return NULL;
    }

    ret = extract_emf_from_comment( lpbBuffer, cbBuffer );
    if (ret) return ret;

    if(!hdcRef)
        hdcRef = hdcdisp = CreateDCW(szDisplayW, NULL, NULL, NULL);

    if (lpmfp)
    {
        TRACE("mm = %d %dx%d\n", lpmfp->mm, lpmfp->xExt, lpmfp->yExt);

        mm = lpmfp->mm;
        xExt = lpmfp->xExt;
        yExt = lpmfp->yExt;
    }
    else
    {
        TRACE("lpmfp == NULL\n");

        /* Use the whole device surface */
        mm = MM_ANISOTROPIC;
        xExt = 0;
        yExt = 0;
    }

    if (mm == MM_ISOTROPIC || mm == MM_ANISOTROPIC)
    {
        if (xExt < 0 || yExt < 0)
        {
          /* Use the whole device surface */
          xExt = 0;
          yExt = 0;
        }

        /* Use the x and y extents as the frame box */
        if (xExt && yExt)
        {
            rc.left = rc.top = 0;
            rc.right = xExt;
            rc.bottom = yExt;
            prcFrame = &rc;
        }
    }

    if(!(hdc = CreateEnhMetaFileW(hdcRef, NULL, prcFrame, NULL)))
    {
        ERR("CreateEnhMetaFile failed\n");
        goto end;
    }

    /*
     * Write the original METAFILE into the enhanced metafile.
     * It is encapsulated in a GDICOMMENT_WINDOWS_METAFILE record.
     */
    if (mm != MM_TEXT)
    {
        wmf_in_emf_comment *mfcomment;
        UINT mfcomment_size;

        mfcomment_size = sizeof (*mfcomment) + cbBuffer;
        mfcomment = HeapAlloc(GetProcessHeap(), 0, mfcomment_size);
        if (mfcomment)
        {
            mfcomment->ident = GDICOMMENT_IDENTIFIER;
            mfcomment->iComment = GDICOMMENT_WINDOWS_METAFILE;
            mfcomment->nVersion = 0x00000300;
            mfcomment->nChecksum = 0; /* FIXME */
            mfcomment->fFlags = 0;
            mfcomment->cbWinMetaFile = cbBuffer;
            memcpy(&mfcomment[1], lpbBuffer, cbBuffer);
            GdiComment(hdc, mfcomment_size, (BYTE*) mfcomment);
            HeapFree(GetProcessHeap(), 0, mfcomment);
        }
        SetMapMode(hdc, mm);
    }


    horzsize = GetDeviceCaps(hdcRef, HORZSIZE);
    vertsize = GetDeviceCaps(hdcRef, VERTSIZE);
    horzres = GetDeviceCaps(hdcRef, HORZRES);
    vertres = GetDeviceCaps(hdcRef, VERTRES);

    if (!xExt || !yExt)
    {
        /* Use the whole device surface */
       xExt = horzres;
       yExt = vertres;
    }
    else
    {
        xExt = MulDiv(xExt, horzres, 100 * horzsize);
        yExt = MulDiv(yExt, vertres, 100 * vertsize);
    }

    /* set the initial viewport:window ratio as 1:1 */
    SetViewportExtEx(hdc, xExt, yExt, NULL);
    SetWindowExtEx(hdc,   xExt, yExt, NULL);

    PlayMetaFile(hdc, hmf);

    ret = CloseEnhMetaFile(hdc);
end:
    if (hdcdisp) DeleteDC(hdcdisp);
    DeleteMetaFile(hmf);
    return ret;
}
