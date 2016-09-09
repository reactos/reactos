/*
 * MetaFile driver DC value functions
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

#include "mfdrv/metafiledrv.h"

INT MFDRV_SaveDC( PHYSDEV dev )
{
    return MFDRV_MetaParam0( dev, META_SAVEDC );
}

BOOL MFDRV_RestoreDC( PHYSDEV dev, INT level )
{
    return MFDRV_MetaParam1( dev, META_RESTOREDC, level );
}

UINT MFDRV_SetTextAlign( PHYSDEV dev, UINT align )
{
    return MFDRV_MetaParam2( dev, META_SETTEXTALIGN, HIWORD(align), LOWORD(align)) ? align : GDI_ERROR;
}

INT MFDRV_SetBkMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETBKMODE, (WORD)mode) ? mode : 0;
}

COLORREF MFDRV_SetBkColor( PHYSDEV dev, COLORREF color )
{
    return MFDRV_MetaParam2(dev, META_SETBKCOLOR, HIWORD(color), LOWORD(color)) ? color : CLR_INVALID;
}

COLORREF MFDRV_SetTextColor( PHYSDEV dev, COLORREF color )
{
    return MFDRV_MetaParam2(dev, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color)) ? color : CLR_INVALID;
}

INT MFDRV_SetROP2( PHYSDEV dev, INT rop )
{
    return MFDRV_MetaParam1( dev, META_SETROP2, (WORD)rop) ? rop : 0;
}

INT MFDRV_SetRelAbs( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETRELABS, (WORD)mode) ? mode : 0;
}

INT MFDRV_SetPolyFillMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETPOLYFILLMODE, (WORD)mode) ? mode : 0;
}

INT MFDRV_SetStretchBltMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETSTRETCHBLTMODE, (WORD)mode) ? mode : 0;
}

INT MFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dev, META_INTERSECTCLIPRECT, left, top, right, bottom );
}

INT MFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dev, META_EXCLUDECLIPRECT, left, top, right, bottom );
}

INT MFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y )
{
    return MFDRV_MetaParam2( dev, META_OFFSETCLIPRGN, x, y );
}

INT MFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETMAPMODE, mode );
}

BOOL MFDRV_SetViewportExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTEXT, x, y );
}

BOOL MFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTORG, x, y );
}

BOOL MFDRV_SetWindowExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWEXT, x, y );
}

BOOL MFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWORG, x, y );
}

BOOL MFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETVIEWPORTORG, x, y );
}

BOOL MFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETWINDOWORG, x, y );
}

BOOL MFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
}

BOOL MFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
}

BOOL MFDRV_SetTextJustification( PHYSDEV dev, INT extra, INT breaks )
{
    return MFDRV_MetaParam2( dev, META_SETTEXTJUSTIFICATION, extra, breaks );
}

INT MFDRV_SetTextCharacterExtra( PHYSDEV dev, INT extra )
{
    return MFDRV_MetaParam1( dev, META_SETTEXTCHAREXTRA, extra ) ? extra : 0x80000000;
}

DWORD MFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags )
{
    return MFDRV_MetaParam2( dev, META_SETMAPPERFLAGS, HIWORD(flags), LOWORD(flags) ) ? flags : GDI_ERROR;
}

BOOL MFDRV_AbortPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_BeginPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_CloseFigure( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_EndPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_FillPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_FlattenPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode )
{
    return FALSE;
}

BOOL MFDRV_StrokeAndFillPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_StrokePath( PHYSDEV dev )
{
    return FALSE;
}

BOOL MFDRV_WidenPath( PHYSDEV dev )
{
    return FALSE;
}

COLORREF MFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    return CLR_INVALID;
}

COLORREF MFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    return CLR_INVALID;
}
