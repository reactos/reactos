/*
 * Copyright (C) 2011 Vincent Povirk for CodeWeavers
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

#include "gdiplus_private.h"

typedef struct EmfPlusRecordHeader
{
    WORD Type;
    WORD Flags;
    DWORD Size;
    DWORD DataSize;
} EmfPlusRecordHeader;

typedef struct EmfPlusHeader
{
    EmfPlusRecordHeader Header;
    DWORD Version;
    DWORD EmfPlusFlags;
    DWORD LogicalDpiX;
    DWORD LogicalDpiY;
} EmfPlusHeader;

typedef struct EmfPlusClear
{
    EmfPlusRecordHeader Header;
    DWORD Color;
} EmfPlusClear;

typedef struct EmfPlusFillRects
{
    EmfPlusRecordHeader Header;
    DWORD BrushID;
    DWORD Count;
} EmfPlusFillRects;

typedef struct EmfPlusSetClipRect
{
    EmfPlusRecordHeader Header;
    GpRectF ClipRect;
} EmfPlusSetClipRect;

typedef struct EmfPlusSetPageTransform
{
    EmfPlusRecordHeader Header;
    REAL PageScale;
} EmfPlusSetPageTransform;

typedef struct EmfPlusRect
{
    SHORT X;
    SHORT Y;
    SHORT Width;
    SHORT Height;
} EmfPlusRect;

typedef struct EmfPlusSetWorldTransform
{
    EmfPlusRecordHeader Header;
    REAL MatrixData[6];
} EmfPlusSetWorldTransform;

typedef struct EmfPlusScaleWorldTransform
{
    EmfPlusRecordHeader Header;
    REAL Sx;
    REAL Sy;
} EmfPlusScaleWorldTransform;

typedef struct EmfPlusMultiplyWorldTransform
{
    EmfPlusRecordHeader Header;
    REAL MatrixData[6];
} EmfPlusMultiplyWorldTransform;

typedef struct EmfPlusRotateWorldTransform
{
    EmfPlusRecordHeader Header;
    REAL Angle;
} EmfPlusRotateWorldTransform;

typedef struct EmfPlusTranslateWorldTransform
{
    EmfPlusRecordHeader Header;
    REAL dx;
    REAL dy;
} EmfPlusTranslateWorldTransform;

typedef struct EmfPlusBeginContainer
{
    EmfPlusRecordHeader Header;
    GpRectF DestRect;
    GpRectF SrcRect;
    DWORD StackIndex;
} EmfPlusBeginContainer;

typedef struct EmfPlusContainerRecord
{
    EmfPlusRecordHeader Header;
    DWORD StackIndex;
} EmfPlusContainerRecord;

enum container_type
{
    BEGIN_CONTAINER,
    SAVE_GRAPHICS
};

typedef struct container
{
    struct list entry;
    DWORD id;
    enum container_type type;
    GraphicsContainer state;
    GpMatrix world_transform;
    GpUnit page_unit;
    REAL page_scale;
    GpRegion *clip;
} container;

static GpStatus METAFILE_AllocateRecord(GpMetafile *metafile, DWORD size, void **result)
{
    DWORD size_needed;
    EmfPlusRecordHeader *record;

    if (!metafile->comment_data_size)
    {
        DWORD data_size = max(256, size * 2 + 4);
        metafile->comment_data = heap_alloc_zero(data_size);

        if (!metafile->comment_data)
            return OutOfMemory;

        memcpy(metafile->comment_data, "EMF+", 4);

        metafile->comment_data_size = data_size;
        metafile->comment_data_length = 4;
    }

    size_needed = size + metafile->comment_data_length;

    if (size_needed > metafile->comment_data_size)
    {
        DWORD data_size = size_needed * 2;
        BYTE *new_data = heap_alloc_zero(data_size);

        if (!new_data)
            return OutOfMemory;

        memcpy(new_data, metafile->comment_data, metafile->comment_data_length);

        metafile->comment_data_size = data_size;
        heap_free(metafile->comment_data);
        metafile->comment_data = new_data;
    }

    *result = metafile->comment_data + metafile->comment_data_length;
    metafile->comment_data_length += size;

    record = (EmfPlusRecordHeader*)*result;
    record->Size = size;
    record->DataSize = size - sizeof(EmfPlusRecordHeader);

    return Ok;
}

static void METAFILE_WriteRecords(GpMetafile *metafile)
{
    if (metafile->comment_data_length > 4)
    {
        GdiComment(metafile->record_dc, metafile->comment_data_length, metafile->comment_data);
        metafile->comment_data_length = 4;
    }
}

static GpStatus METAFILE_WriteHeader(GpMetafile *metafile, HDC hdc)
{
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusHeader *header;

        stat = METAFILE_AllocateRecord(metafile, sizeof(EmfPlusHeader), (void**)&header);
        if (stat != Ok)
            return stat;

        header->Header.Type = EmfPlusRecordTypeHeader;

        if (metafile->metafile_type == MetafileTypeEmfPlusDual)
            header->Header.Flags = 1;
        else
            header->Header.Flags = 0;

        header->Version = 0xDBC01002;

        if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY)
            header->EmfPlusFlags = 1;
        else
            header->EmfPlusFlags = 0;

        header->LogicalDpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        header->LogicalDpiY = GetDeviceCaps(hdc, LOGPIXELSY);

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

static GpStatus METAFILE_WriteEndOfFile(GpMetafile *metafile)
{
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusRecordHeader *record;

        stat = METAFILE_AllocateRecord(metafile, sizeof(EmfPlusRecordHeader), (void**)&record);
        if (stat != Ok)
            return stat;

        record->Type = EmfPlusRecordTypeEndOfFile;
        record->Flags = 0;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipRecordMetafile(HDC hdc, EmfType type, GDIPCONST GpRectF *frameRect,
                                       MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    HDC record_dc;
    REAL dpix, dpiy;
    REAL framerect_factor_x, framerect_factor_y;
    RECT rc, *lprc;
    GpStatus stat;

    TRACE("(%p %d %p %d %p %p)\n", hdc, type, frameRect, frameUnit, desc, metafile);

    if (!hdc || type < EmfTypeEmfOnly || type > EmfTypeEmfPlusDual || !metafile)
        return InvalidParameter;

    dpix = (REAL)GetDeviceCaps(hdc, HORZRES) / GetDeviceCaps(hdc, HORZSIZE) * 25.4;
    dpiy = (REAL)GetDeviceCaps(hdc, VERTRES) / GetDeviceCaps(hdc, VERTSIZE) * 25.4;

    if (frameRect)
    {
        switch (frameUnit)
        {
        case MetafileFrameUnitPixel:
            framerect_factor_x = 2540.0 / dpix;
            framerect_factor_y = 2540.0 / dpiy;
            break;
        case MetafileFrameUnitPoint:
            framerect_factor_x = framerect_factor_y = 2540.0 / 72.0;
            break;
        case MetafileFrameUnitInch:
            framerect_factor_x = framerect_factor_y = 2540.0;
            break;
        case MetafileFrameUnitDocument:
            framerect_factor_x = framerect_factor_y = 2540.0 / 300.0;
            break;
        case MetafileFrameUnitMillimeter:
            framerect_factor_x = framerect_factor_y = 100.0;
            break;
        case MetafileFrameUnitGdi:
            framerect_factor_x = framerect_factor_y = 1.0;
            break;
        default:
            return InvalidParameter;
        }

        rc.left = framerect_factor_x * frameRect->X;
        rc.top = framerect_factor_y * frameRect->Y;
        rc.right = rc.left + framerect_factor_x * frameRect->Width;
        rc.bottom = rc.top + framerect_factor_y * frameRect->Height;

        lprc = &rc;
    }
    else
        lprc = NULL;

    record_dc = CreateEnhMetaFileW(hdc, NULL, lprc, desc);

    if (!record_dc)
        return GenericError;

    *metafile = heap_alloc_zero(sizeof(GpMetafile));
    if(!*metafile)
    {
        DeleteEnhMetaFile(CloseEnhMetaFile(record_dc));
        return OutOfMemory;
    }

    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->image.flags   = ImageFlagsNone;
    (*metafile)->image.palette = NULL;
    (*metafile)->image.xres = dpix;
    (*metafile)->image.yres = dpiy;
    (*metafile)->bounds.X = (*metafile)->bounds.Y = 0.0;
    (*metafile)->bounds.Width = (*metafile)->bounds.Height = 1.0;
    (*metafile)->unit = UnitPixel;
    (*metafile)->metafile_type = type;
    (*metafile)->record_dc = record_dc;
    (*metafile)->comment_data = NULL;
    (*metafile)->comment_data_size = 0;
    (*metafile)->comment_data_length = 0;
    (*metafile)->hemf = NULL;
    list_init(&(*metafile)->containers);

    if (!frameRect)
    {
        (*metafile)->auto_frame = TRUE;
        (*metafile)->auto_frame_min.X = 0;
        (*metafile)->auto_frame_min.Y = 0;
        (*metafile)->auto_frame_max.X = -1;
        (*metafile)->auto_frame_max.Y = -1;
    }

    stat = METAFILE_WriteHeader(*metafile, hdc);

    if (stat != Ok)
    {
        DeleteEnhMetaFile(CloseEnhMetaFile(record_dc));
        heap_free(*metafile);
        *metafile = NULL;
        return OutOfMemory;
    }

    return stat;
}

/*****************************************************************************
 * GdipRecordMetafileI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipRecordMetafileI(HDC hdc, EmfType type, GDIPCONST GpRect *frameRect,
                                        MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    GpRectF frameRectF, *pFrameRectF;

    TRACE("(%p %d %p %d %p %p)\n", hdc, type, frameRect, frameUnit, desc, metafile);

    if (frameRect)
    {
        frameRectF.X = frameRect->X;
        frameRectF.Y = frameRect->Y;
        frameRectF.Width = frameRect->Width;
        frameRectF.Height = frameRect->Height;
        pFrameRectF = &frameRectF;
    }
    else
        pFrameRectF = NULL;

    return GdipRecordMetafile(hdc, type, pFrameRectF, frameUnit, desc, metafile);
}

GpStatus WINGDIPAPI GdipRecordMetafileStream(IStream *stream, HDC hdc, EmfType type, GDIPCONST GpRectF *frameRect,
                                        MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    GpStatus stat;

    TRACE("(%p %p %d %p %d %p %p)\n", stream, hdc, type, frameRect, frameUnit, desc, metafile);

    if (!stream)
        return InvalidParameter;

    stat = GdipRecordMetafile(hdc, type, frameRect, frameUnit, desc, metafile);

    if (stat == Ok)
    {
        (*metafile)->record_stream = stream;
        IStream_AddRef(stream);
    }

    return stat;
}

static void METAFILE_AdjustFrame(GpMetafile* metafile, const GpPointF *points,
    UINT num_points)
{
    int i;

    if (!metafile->auto_frame || !num_points)
        return;

    if (metafile->auto_frame_max.X < metafile->auto_frame_min.X)
        metafile->auto_frame_max = metafile->auto_frame_min = points[0];

    for (i=0; i<num_points; i++)
    {
        if (points[i].X < metafile->auto_frame_min.X)
            metafile->auto_frame_min.X = points[i].X;
        if (points[i].X > metafile->auto_frame_max.X)
            metafile->auto_frame_max.X = points[i].X;
        if (points[i].Y < metafile->auto_frame_min.Y)
            metafile->auto_frame_min.Y = points[i].Y;
        if (points[i].Y > metafile->auto_frame_max.Y)
            metafile->auto_frame_max.Y = points[i].Y;
    }
}

GpStatus METAFILE_GetGraphicsContext(GpMetafile* metafile, GpGraphics **result)
{
    GpStatus stat;

    if (!metafile->record_dc || metafile->record_graphics)
        return InvalidParameter;

    stat = graphics_from_image((GpImage*)metafile, &metafile->record_graphics);

    if (stat == Ok)
    {
        *result = metafile->record_graphics;
        metafile->record_graphics->xres = 96.0;
        metafile->record_graphics->yres = 96.0;
    }

    return stat;
}

GpStatus METAFILE_GetDC(GpMetafile* metafile, HDC *hdc)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusRecordHeader *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile, sizeof(EmfPlusRecordHeader), (void**)&record);
        if (stat != Ok)
            return stat;

        record->Type = EmfPlusRecordTypeGetDC;
        record->Flags = 0;

        METAFILE_WriteRecords(metafile);
    }

    *hdc = metafile->record_dc;

    return Ok;
}

GpStatus METAFILE_GraphicsClear(GpMetafile* metafile, ARGB color)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusClear *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile, sizeof(EmfPlusClear), (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeClear;
        record->Header.Flags = 0;
        record->Color = color;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

static BOOL is_integer_rect(const GpRectF *rect)
{
    SHORT x, y, width, height;
    x = rect->X;
    y = rect->Y;
    width = rect->Width;
    height = rect->Height;
    if (rect->X != (REAL)x || rect->Y != (REAL)y ||
        rect->Width != (REAL)width || rect->Height != (REAL)height)
        return FALSE;
    return TRUE;
}

GpStatus METAFILE_FillRectangles(GpMetafile* metafile, GpBrush* brush,
    GDIPCONST GpRectF* rects, INT count)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusFillRects *record;
        GpStatus stat;
        BOOL integer_rects = TRUE;
        int i;
        DWORD brushid;
        int flags = 0;

        if (brush->bt == BrushTypeSolidColor)
        {
            flags |= 0x8000;
            brushid = ((GpSolidFill*)brush)->color;
        }
        else
        {
            FIXME("brush serialization not implemented\n");
            return NotImplemented;
        }

        for (i=0; i<count; i++)
        {
            if (!is_integer_rect(&rects[i]))
            {
                integer_rects = FALSE;
                break;
            }
        }

        if (integer_rects)
            flags |= 0x4000;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusFillRects) + count * (integer_rects ? sizeof(EmfPlusRect) : sizeof(GpRectF)),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeFillRects;
        record->Header.Flags = flags;
        record->BrushID = brushid;
        record->Count = count;

        if (integer_rects)
        {
            EmfPlusRect *record_rects = (EmfPlusRect*)(record+1);
            for (i=0; i<count; i++)
            {
                record_rects[i].X = (SHORT)rects[i].X;
                record_rects[i].Y = (SHORT)rects[i].Y;
                record_rects[i].Width = (SHORT)rects[i].Width;
                record_rects[i].Height = (SHORT)rects[i].Height;
            }
        }
        else
            memcpy(record+1, rects, sizeof(GpRectF) * count);

        METAFILE_WriteRecords(metafile);
    }

    if (metafile->auto_frame)
    {
        GpPointF corners[4];
        int i;

        for (i=0; i<count; i++)
        {
            corners[0].X = rects[i].X;
            corners[0].Y = rects[i].Y;
            corners[1].X = rects[i].X + rects[i].Width;
            corners[1].Y = rects[i].Y;
            corners[2].X = rects[i].X;
            corners[2].Y = rects[i].Y + rects[i].Height;
            corners[3].X = rects[i].X + rects[i].Width;
            corners[3].Y = rects[i].Y + rects[i].Height;

            GdipTransformPoints(metafile->record_graphics, CoordinateSpaceDevice,
                CoordinateSpaceWorld, corners, 4);

            METAFILE_AdjustFrame(metafile, corners, 4);
        }
    }

    return Ok;
}

GpStatus METAFILE_SetClipRect(GpMetafile* metafile, REAL x, REAL y, REAL width, REAL height, CombineMode mode)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusSetClipRect *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusSetClipRect),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeSetClipRect;
        record->Header.Flags = (mode & 0xf) << 8;
        record->ClipRect.X = x;
        record->ClipRect.Y = y;
        record->ClipRect.Width = width;
        record->ClipRect.Height = height;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_SetPageTransform(GpMetafile* metafile, GpUnit unit, REAL scale)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusSetPageTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusSetPageTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeSetPageTransform;
        record->Header.Flags = unit;
        record->PageScale = scale;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_SetWorldTransform(GpMetafile* metafile, GDIPCONST GpMatrix* transform)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusSetWorldTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusSetWorldTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeSetWorldTransform;
        record->Header.Flags = 0;
        memcpy(record->MatrixData, transform->matrix, sizeof(record->MatrixData));

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_ScaleWorldTransform(GpMetafile* metafile, REAL sx, REAL sy, MatrixOrder order)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusScaleWorldTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusScaleWorldTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeScaleWorldTransform;
        record->Header.Flags = (order == MatrixOrderAppend ? 0x2000 : 0);
        record->Sx = sx;
        record->Sy = sy;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_MultiplyWorldTransform(GpMetafile* metafile, GDIPCONST GpMatrix* matrix, MatrixOrder order)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusMultiplyWorldTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusMultiplyWorldTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeMultiplyWorldTransform;
        record->Header.Flags = (order == MatrixOrderAppend ? 0x2000 : 0);
        memcpy(record->MatrixData, matrix->matrix, sizeof(record->MatrixData));

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_RotateWorldTransform(GpMetafile* metafile, REAL angle, MatrixOrder order)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusRotateWorldTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusRotateWorldTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeRotateWorldTransform;
        record->Header.Flags = (order == MatrixOrderAppend ? 0x2000 : 0);
        record->Angle = angle;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_TranslateWorldTransform(GpMetafile* metafile, REAL dx, REAL dy, MatrixOrder order)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusTranslateWorldTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusTranslateWorldTransform),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeTranslateWorldTransform;
        record->Header.Flags = (order == MatrixOrderAppend ? 0x2000 : 0);
        record->dx = dx;
        record->dy = dy;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_ResetWorldTransform(GpMetafile* metafile)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusRecordHeader *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusRecordHeader),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Type = EmfPlusRecordTypeResetWorldTransform;
        record->Flags = 0;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_BeginContainer(GpMetafile* metafile, GDIPCONST GpRectF *dstrect,
    GDIPCONST GpRectF *srcrect, GpUnit unit, DWORD StackIndex)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusBeginContainer *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile, sizeof(*record), (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeBeginContainer;
        record->Header.Flags = unit & 0xff;
        record->DestRect = *dstrect;
        record->SrcRect = *srcrect;
        record->StackIndex = StackIndex;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_BeginContainerNoParams(GpMetafile* metafile, DWORD StackIndex)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusContainerRecord *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusContainerRecord),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeBeginContainerNoParams;
        record->Header.Flags = 0;
        record->StackIndex = StackIndex;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_EndContainer(GpMetafile* metafile, DWORD StackIndex)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusContainerRecord *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusContainerRecord),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeEndContainer;
        record->Header.Flags = 0;
        record->StackIndex = StackIndex;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_SaveGraphics(GpMetafile* metafile, DWORD StackIndex)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusContainerRecord *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusContainerRecord),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeSave;
        record->Header.Flags = 0;
        record->StackIndex = StackIndex;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_RestoreGraphics(GpMetafile* metafile, DWORD StackIndex)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusContainerRecord *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile,
            sizeof(EmfPlusContainerRecord),
            (void**)&record);
        if (stat != Ok)
            return stat;

        record->Header.Type = EmfPlusRecordTypeRestore;
        record->Header.Flags = 0;
        record->StackIndex = StackIndex;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus METAFILE_ReleaseDC(GpMetafile* metafile, HDC hdc)
{
    if (hdc != metafile->record_dc)
        return InvalidParameter;

    return Ok;
}

GpStatus METAFILE_GraphicsDeleted(GpMetafile* metafile)
{
    GpStatus stat;

    stat = METAFILE_WriteEndOfFile(metafile);
    metafile->record_graphics = NULL;

    metafile->hemf = CloseEnhMetaFile(metafile->record_dc);
    metafile->record_dc = NULL;

    heap_free(metafile->comment_data);
    metafile->comment_data = NULL;
    metafile->comment_data_size = 0;

    if (stat == Ok)
    {
        MetafileHeader header;

        stat = GdipGetMetafileHeaderFromEmf(metafile->hemf, &header);
        if (stat == Ok && metafile->auto_frame &&
            metafile->auto_frame_max.X >= metafile->auto_frame_min.X)
        {
            RECTL bounds_rc, gdi_bounds_rc;
            REAL x_scale = 2540.0 / header.DpiX;
            REAL y_scale = 2540.0 / header.DpiY;
            BYTE* buffer;
            UINT buffer_size;

            bounds_rc.left = floorf(metafile->auto_frame_min.X * x_scale);
            bounds_rc.top = floorf(metafile->auto_frame_min.Y * y_scale);
            bounds_rc.right = ceilf(metafile->auto_frame_max.X * x_scale);
            bounds_rc.bottom = ceilf(metafile->auto_frame_max.Y * y_scale);

            gdi_bounds_rc = header.u.EmfHeader.rclBounds;
            if (gdi_bounds_rc.right > gdi_bounds_rc.left && gdi_bounds_rc.bottom > gdi_bounds_rc.top)
            {
                bounds_rc.left = min(bounds_rc.left, gdi_bounds_rc.left);
                bounds_rc.top = min(bounds_rc.top, gdi_bounds_rc.top);
                bounds_rc.right = max(bounds_rc.right, gdi_bounds_rc.right);
                bounds_rc.bottom = max(bounds_rc.bottom, gdi_bounds_rc.bottom);
            }

            buffer_size = GetEnhMetaFileBits(metafile->hemf, 0, NULL);
            buffer = heap_alloc(buffer_size);
            if (buffer)
            {
                HENHMETAFILE new_hemf;

                GetEnhMetaFileBits(metafile->hemf, buffer_size, buffer);

                ((ENHMETAHEADER*)buffer)->rclFrame = bounds_rc;

                new_hemf = SetEnhMetaFileBits(buffer_size, buffer);

                if (new_hemf)
                {
                    DeleteEnhMetaFile(metafile->hemf);
                    metafile->hemf = new_hemf;
                }
                else
                    stat = OutOfMemory;

                heap_free(buffer);
            }
            else
                stat = OutOfMemory;

            if (stat == Ok)
                stat = GdipGetMetafileHeaderFromEmf(metafile->hemf, &header);
        }
        if (stat == Ok)
        {
            metafile->bounds.X = header.X;
            metafile->bounds.Y = header.Y;
            metafile->bounds.Width = header.Width;
            metafile->bounds.Height = header.Height;
        }
    }

    if (stat == Ok && metafile->record_stream)
    {
        BYTE *buffer;
        UINT buffer_size;

        buffer_size = GetEnhMetaFileBits(metafile->hemf, 0, NULL);

        buffer = heap_alloc(buffer_size);
        if (buffer)
        {
            HRESULT hr;

            GetEnhMetaFileBits(metafile->hemf, buffer_size, buffer);

            hr = IStream_Write(metafile->record_stream, buffer, buffer_size, NULL);

            if (FAILED(hr))
                stat = hresult_to_status(hr);

            heap_free(buffer);
        }
        else
            stat = OutOfMemory;
    }

    if (metafile->record_stream)
    {
        IStream_Release(metafile->record_stream);
        metafile->record_stream = NULL;
    }

    return stat;
}

GpStatus WINGDIPAPI GdipGetHemfFromMetafile(GpMetafile *metafile, HENHMETAFILE *hEmf)
{
    TRACE("(%p,%p)\n", metafile, hEmf);

    if (!metafile || !hEmf || !metafile->hemf)
        return InvalidParameter;

    *hEmf = metafile->hemf;
    metafile->hemf = NULL;

    return Ok;
}

static void METAFILE_GetFinalGdiTransform(const GpMetafile *metafile, XFORM *result)
{
    const GpRectF *rect;
    const GpPointF *pt;

    /* This transforms metafile device space to output points. */
    rect = &metafile->src_rect;
    pt = metafile->playback_points;
    result->eM11 = (pt[1].X - pt[0].X) / rect->Width;
    result->eM21 = (pt[2].X - pt[0].X) / rect->Height;
    result->eDx = pt[0].X - result->eM11 * rect->X - result->eM21 * rect->Y;
    result->eM12 = (pt[1].Y - pt[0].Y) / rect->Width;
    result->eM22 = (pt[2].Y - pt[0].Y) / rect->Height;
    result->eDy = pt[0].Y - result->eM12 * rect->X - result->eM22 * rect->Y;
}

static GpStatus METAFILE_PlaybackUpdateGdiTransform(GpMetafile *metafile)
{
    XFORM combined, final;

    METAFILE_GetFinalGdiTransform(metafile, &final);

    CombineTransform(&combined, &metafile->gdiworldtransform, &final);

    SetGraphicsMode(metafile->playback_dc, GM_ADVANCED);
    SetWorldTransform(metafile->playback_dc, &combined);

    return Ok;
}

static GpStatus METAFILE_PlaybackGetDC(GpMetafile *metafile)
{
    GpStatus stat = Ok;

    stat = GdipGetDC(metafile->playback_graphics, &metafile->playback_dc);

    if (stat == Ok)
    {
        static const XFORM identity = {1, 0, 0, 1, 0, 0};

        metafile->gdiworldtransform = identity;
        METAFILE_PlaybackUpdateGdiTransform(metafile);
    }

    return stat;
}

static void METAFILE_PlaybackReleaseDC(GpMetafile *metafile)
{
    if (metafile->playback_dc)
    {
        GdipReleaseDC(metafile->playback_graphics, metafile->playback_dc);
        metafile->playback_dc = NULL;
    }
}

static GpStatus METAFILE_PlaybackUpdateClip(GpMetafile *metafile)
{
    GpStatus stat;
    stat = GdipCombineRegionRegion(metafile->playback_graphics->clip, metafile->base_clip, CombineModeReplace);
    if (stat == Ok)
        stat = GdipCombineRegionRegion(metafile->playback_graphics->clip, metafile->clip, CombineModeIntersect);
    return stat;
}

static GpStatus METAFILE_PlaybackUpdateWorldTransform(GpMetafile *metafile)
{
    GpMatrix *real_transform;
    GpStatus stat;

    stat = GdipCreateMatrix3(&metafile->src_rect, metafile->playback_points, &real_transform);

    if (stat == Ok)
    {
        REAL scale = units_to_pixels(1.0, metafile->page_unit, 96.0);

        if (metafile->page_unit != UnitDisplay)
            scale *= metafile->page_scale;

        stat = GdipScaleMatrix(real_transform, scale, scale, MatrixOrderPrepend);

        if (stat == Ok)
            stat = GdipMultiplyMatrix(real_transform, metafile->world_transform, MatrixOrderPrepend);

        if (stat == Ok)
            stat = GdipSetWorldTransform(metafile->playback_graphics, real_transform);

        GdipDeleteMatrix(real_transform);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipPlayMetafileRecord(GDIPCONST GpMetafile *metafile,
    EmfPlusRecordType recordType, UINT flags, UINT dataSize, GDIPCONST BYTE *data)
{
    GpStatus stat;
    GpMetafile *real_metafile = (GpMetafile*)metafile;

    TRACE("(%p,%x,%x,%d,%p)\n", metafile, recordType, flags, dataSize, data);

    if (!metafile || (dataSize && !data) || !metafile->playback_graphics)
        return InvalidParameter;

    if (recordType >= 1 && recordType <= 0x7a)
    {
        /* regular EMF record */
        if (metafile->playback_dc)
        {
            ENHMETARECORD *record;

            switch (recordType)
            {
            case EMR_SETMAPMODE:
            case EMR_SAVEDC:
            case EMR_RESTOREDC:
            case EMR_SETWINDOWORGEX:
            case EMR_SETWINDOWEXTEX:
            case EMR_SETVIEWPORTORGEX:
            case EMR_SETVIEWPORTEXTEX:
            case EMR_SCALEVIEWPORTEXTEX:
            case EMR_SCALEWINDOWEXTEX:
            case EMR_MODIFYWORLDTRANSFORM:
                FIXME("not implemented for record type %x\n", recordType);
                break;
            case EMR_SETWORLDTRANSFORM:
            {
                const XFORM* xform = (void*)data;
                real_metafile->gdiworldtransform = *xform;
                METAFILE_PlaybackUpdateGdiTransform(real_metafile);
                break;
            }
            case EMR_EXTSELECTCLIPRGN:
            {
                DWORD rgndatasize = *(DWORD*)data;
                DWORD mode = *(DWORD*)(data + 4);
                const RGNDATA *rgndata = (const RGNDATA*)(data + 8);
                HRGN hrgn = NULL;

                if (dataSize > 8)
                {
                    XFORM final;

                    METAFILE_GetFinalGdiTransform(metafile, &final);

                    hrgn = ExtCreateRegion(&final, rgndatasize, rgndata);
                }

                ExtSelectClipRgn(metafile->playback_dc, hrgn, mode);

                DeleteObject(hrgn);

                return Ok;
            }
            default:
                break;
            }

            record = heap_alloc_zero(dataSize + 8);

            if (record)
            {
                record->iType = recordType;
                record->nSize = dataSize + 8;
                memcpy(record->dParm, data, dataSize);

                PlayEnhMetaFileRecord(metafile->playback_dc, metafile->handle_table,
                    record, metafile->handle_count);

                heap_free(record);
            }
            else
                return OutOfMemory;
        }
    }
    else
    {
        EmfPlusRecordHeader *header = (EmfPlusRecordHeader*)(data)-1;

        METAFILE_PlaybackReleaseDC((GpMetafile*)metafile);

        switch(recordType)
        {
        case EmfPlusRecordTypeHeader:
        case EmfPlusRecordTypeEndOfFile:
            break;
        case EmfPlusRecordTypeGetDC:
            METAFILE_PlaybackGetDC((GpMetafile*)metafile);
            break;
        case EmfPlusRecordTypeClear:
        {
            EmfPlusClear *record = (EmfPlusClear*)header;

            return GdipGraphicsClear(metafile->playback_graphics, record->Color);
        }
        case EmfPlusRecordTypeFillRects:
        {
            EmfPlusFillRects *record = (EmfPlusFillRects*)header;
            GpBrush *brush, *temp_brush=NULL;
            GpRectF *rects, *temp_rects=NULL;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusFillRects))
                return InvalidParameter;

            if (flags & 0x4000)
            {
                if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusFillRects) + sizeof(EmfPlusRect) * record->Count)
                    return InvalidParameter;
            }
            else
            {
                if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusFillRects) + sizeof(GpRectF) * record->Count)
                    return InvalidParameter;
            }

            if (flags & 0x8000)
            {
                stat = GdipCreateSolidFill((ARGB)record->BrushID, (GpSolidFill**)&temp_brush);
                brush = temp_brush;
            }
            else
            {
                FIXME("brush deserialization not implemented\n");
                return NotImplemented;
            }

            if (stat == Ok)
            {
                if (flags & 0x4000)
                {
                    EmfPlusRect *int_rects = (EmfPlusRect*)(record+1);
                    int i;

                    rects = temp_rects = heap_alloc_zero(sizeof(GpRectF) * record->Count);
                    if (rects)
                    {
                        for (i=0; i<record->Count; i++)
                        {
                            rects[i].X = int_rects[i].X;
                            rects[i].Y = int_rects[i].Y;
                            rects[i].Width = int_rects[i].Width;
                            rects[i].Height = int_rects[i].Height;
                        }
                    }
                    else
                        stat = OutOfMemory;
                }
                else
                    rects = (GpRectF*)(record+1);
            }

            if (stat == Ok)
            {
                stat = GdipFillRectangles(metafile->playback_graphics, brush, rects, record->Count);
            }

            GdipDeleteBrush(temp_brush);
            heap_free(temp_rects);

            return stat;
        }
        case EmfPlusRecordTypeSetClipRect:
        {
            EmfPlusSetClipRect *record = (EmfPlusSetClipRect*)header;
            CombineMode mode = (CombineMode)((flags >> 8) & 0xf);
            GpRegion *region;
            GpMatrix world_to_device;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(*record))
                return InvalidParameter;

            stat = GdipCreateRegionRect(&record->ClipRect, &region);

            if (stat == Ok)
            {
                get_graphics_transform(real_metafile->playback_graphics,
                    CoordinateSpaceDevice, CoordinateSpaceWorld, &world_to_device);

                GdipTransformRegion(region, &world_to_device);

                GdipCombineRegionRegion(real_metafile->clip, region, mode);

                GdipDeleteRegion(region);
            }

            return METAFILE_PlaybackUpdateClip(real_metafile);
        }
        case EmfPlusRecordTypeSetPageTransform:
        {
            EmfPlusSetPageTransform *record = (EmfPlusSetPageTransform*)header;
            GpUnit unit = (GpUnit)flags;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusSetPageTransform))
                return InvalidParameter;

            real_metafile->page_unit = unit;
            real_metafile->page_scale = record->PageScale;

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeSetWorldTransform:
        {
            EmfPlusSetWorldTransform *record = (EmfPlusSetWorldTransform*)header;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusSetWorldTransform))
                return InvalidParameter;

            memcpy(real_metafile->world_transform->matrix, record->MatrixData, sizeof(record->MatrixData));

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeScaleWorldTransform:
        {
            EmfPlusScaleWorldTransform *record = (EmfPlusScaleWorldTransform*)header;
            MatrixOrder order = (flags & 0x2000) ? MatrixOrderAppend : MatrixOrderPrepend;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusScaleWorldTransform))
                return InvalidParameter;

            GdipScaleMatrix(real_metafile->world_transform, record->Sx, record->Sy, order);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeMultiplyWorldTransform:
        {
            EmfPlusMultiplyWorldTransform *record = (EmfPlusMultiplyWorldTransform*)header;
            MatrixOrder order = (flags & 0x2000) ? MatrixOrderAppend : MatrixOrderPrepend;
            GpMatrix matrix;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusMultiplyWorldTransform))
                return InvalidParameter;

            memcpy(matrix.matrix, record->MatrixData, sizeof(matrix.matrix));

            GdipMultiplyMatrix(real_metafile->world_transform, &matrix, order);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeRotateWorldTransform:
        {
            EmfPlusRotateWorldTransform *record = (EmfPlusRotateWorldTransform*)header;
            MatrixOrder order = (flags & 0x2000) ? MatrixOrderAppend : MatrixOrderPrepend;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusRotateWorldTransform))
                return InvalidParameter;

            GdipRotateMatrix(real_metafile->world_transform, record->Angle, order);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeTranslateWorldTransform:
        {
            EmfPlusTranslateWorldTransform *record = (EmfPlusTranslateWorldTransform*)header;
            MatrixOrder order = (flags & 0x2000) ? MatrixOrderAppend : MatrixOrderPrepend;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusTranslateWorldTransform))
                return InvalidParameter;

            GdipTranslateMatrix(real_metafile->world_transform, record->dx, record->dy, order);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeResetWorldTransform:
        {
            GdipSetMatrixElements(real_metafile->world_transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeBeginContainer:
        {
            EmfPlusBeginContainer *record = (EmfPlusBeginContainer*)header;
            container* cont;
            GpUnit unit;
            REAL scale_x, scale_y;
            GpRectF scaled_srcrect;
            GpMatrix transform;

            cont = heap_alloc_zero(sizeof(*cont));
            if (!cont)
                return OutOfMemory;

            stat = GdipCloneRegion(metafile->clip, &cont->clip);
            if (stat != Ok)
            {
                heap_free(cont);
                return stat;
            }

            stat = GdipBeginContainer2(metafile->playback_graphics, &cont->state);

            if (stat != Ok)
            {
                GdipDeleteRegion(cont->clip);
                heap_free(cont);
                return stat;
            }

            cont->id = record->StackIndex;
            cont->type = BEGIN_CONTAINER;
            cont->world_transform = *metafile->world_transform;
            cont->page_unit = metafile->page_unit;
            cont->page_scale = metafile->page_scale;
            list_add_head(&real_metafile->containers, &cont->entry);

            unit = record->Header.Flags & 0xff;

            scale_x = units_to_pixels(1.0, unit, metafile->image.xres);
            scale_y = units_to_pixels(1.0, unit, metafile->image.yres);

            scaled_srcrect.X = scale_x * record->SrcRect.X;
            scaled_srcrect.Y = scale_y * record->SrcRect.Y;
            scaled_srcrect.Width = scale_x * record->SrcRect.Width;
            scaled_srcrect.Height = scale_y * record->SrcRect.Height;

            transform.matrix[0] = record->DestRect.Width / scaled_srcrect.Width;
            transform.matrix[1] = 0.0;
            transform.matrix[2] = 0.0;
            transform.matrix[3] = record->DestRect.Height / scaled_srcrect.Height;
            transform.matrix[4] = record->DestRect.X - scaled_srcrect.X;
            transform.matrix[5] = record->DestRect.Y - scaled_srcrect.Y;

            GdipMultiplyMatrix(real_metafile->world_transform, &transform, MatrixOrderPrepend);

            return METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }
        case EmfPlusRecordTypeBeginContainerNoParams:
        case EmfPlusRecordTypeSave:
        {
            EmfPlusContainerRecord *record = (EmfPlusContainerRecord*)header;
            container* cont;

            cont = heap_alloc_zero(sizeof(*cont));
            if (!cont)
                return OutOfMemory;

            stat = GdipCloneRegion(metafile->clip, &cont->clip);
            if (stat != Ok)
            {
                heap_free(cont);
                return stat;
            }

            if (recordType == EmfPlusRecordTypeBeginContainerNoParams)
                stat = GdipBeginContainer2(metafile->playback_graphics, &cont->state);
            else
                stat = GdipSaveGraphics(metafile->playback_graphics, &cont->state);

            if (stat != Ok)
            {
                GdipDeleteRegion(cont->clip);
                heap_free(cont);
                return stat;
            }

            cont->id = record->StackIndex;
            if (recordType == EmfPlusRecordTypeBeginContainerNoParams)
                cont->type = BEGIN_CONTAINER;
            else
                cont->type = SAVE_GRAPHICS;
            cont->world_transform = *metafile->world_transform;
            cont->page_unit = metafile->page_unit;
            cont->page_scale = metafile->page_scale;
            list_add_head(&real_metafile->containers, &cont->entry);

            break;
        }
        case EmfPlusRecordTypeEndContainer:
        case EmfPlusRecordTypeRestore:
        {
            EmfPlusContainerRecord *record = (EmfPlusContainerRecord*)header;
            container* cont;
            enum container_type type;
            BOOL found=FALSE;

            if (recordType == EmfPlusRecordTypeEndContainer)
                type = BEGIN_CONTAINER;
            else
                type = SAVE_GRAPHICS;

            LIST_FOR_EACH_ENTRY(cont, &real_metafile->containers, container, entry)
            {
                if (cont->id == record->StackIndex && cont->type == type)
                {
                    found = TRUE;
                    break;
                }
            }

            if (found)
            {
                container* cont2;

                /* pop any newer items on the stack */
                while ((cont2 = LIST_ENTRY(list_head(&real_metafile->containers), container, entry)) != cont)
                {
                    list_remove(&cont2->entry);
                    GdipDeleteRegion(cont2->clip);
                    heap_free(cont2);
                }

                if (type == BEGIN_CONTAINER)
                    GdipEndContainer(real_metafile->playback_graphics, cont->state);
                else
                    GdipRestoreGraphics(real_metafile->playback_graphics, cont->state);

                *real_metafile->world_transform = cont->world_transform;
                real_metafile->page_unit = cont->page_unit;
                real_metafile->page_scale = cont->page_scale;
                GdipCombineRegionRegion(real_metafile->clip, cont->clip, CombineModeReplace);

                list_remove(&cont->entry);
                GdipDeleteRegion(cont->clip);
                heap_free(cont);
            }

            break;
        }
        default:
            FIXME("Not implemented for record type %x\n", recordType);
            return NotImplemented;
        }
    }

    return Ok;
}

struct enum_metafile_data
{
    EnumerateMetafileProc callback;
    void *callback_data;
    GpMetafile *metafile;
};

static int CALLBACK enum_metafile_proc(HDC hDC, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR,
    int nObj, LPARAM lpData)
{
    BOOL ret;
    struct enum_metafile_data *data = (struct enum_metafile_data*)lpData;
    const BYTE* pStr;

    data->metafile->handle_table = lpHTable;
    data->metafile->handle_count = nObj;

    /* First check for an EMF+ record. */
    if (lpEMFR->iType == EMR_GDICOMMENT)
    {
        const EMRGDICOMMENT *comment = (const EMRGDICOMMENT*)lpEMFR;

        if (comment->cbData >= 4 && memcmp(comment->Data, "EMF+", 4) == 0)
        {
            int offset = 4;

            while (offset + sizeof(EmfPlusRecordHeader) <= comment->cbData)
            {
                const EmfPlusRecordHeader *record = (const EmfPlusRecordHeader*)&comment->Data[offset];

                if (record->DataSize)
                    pStr = (const BYTE*)(record+1);
                else
                    pStr = NULL;

                ret = data->callback(record->Type, record->Flags, record->DataSize,
                    pStr, data->callback_data);

                if (!ret)
                    return 0;

                offset += record->Size;
            }

            return 1;
        }
    }

    if (lpEMFR->nSize != 8)
        pStr = (const BYTE*)lpEMFR->dParm;
    else
        pStr = NULL;

    return data->callback(lpEMFR->iType, 0, lpEMFR->nSize-8,
        pStr, data->callback_data);
}

GpStatus WINGDIPAPI GdipEnumerateMetafileSrcRectDestPoints(GpGraphics *graphics,
    GDIPCONST GpMetafile *metafile, GDIPCONST GpPointF *destPoints, INT count,
    GDIPCONST GpRectF *srcRect, Unit srcUnit, EnumerateMetafileProc callback,
    VOID *callbackData, GDIPCONST GpImageAttributes *imageAttributes)
{
    struct enum_metafile_data data;
    GpStatus stat;
    GpMetafile *real_metafile = (GpMetafile*)metafile; /* whoever made this const was joking */
    GraphicsContainer state;
    GpPath *dst_path;

    TRACE("(%p,%p,%p,%i,%p,%i,%p,%p,%p)\n", graphics, metafile,
        destPoints, count, srcRect, srcUnit, callback, callbackData,
        imageAttributes);

    if (!graphics || !metafile || !destPoints || count != 3 || !srcRect)
        return InvalidParameter;

    if (!metafile->hemf)
        return InvalidParameter;

    if (metafile->playback_graphics)
        return ObjectBusy;

    TRACE("%s %i -> %s %s %s\n", debugstr_rectf(srcRect), srcUnit,
        debugstr_pointf(&destPoints[0]), debugstr_pointf(&destPoints[1]),
        debugstr_pointf(&destPoints[2]));

    data.callback = callback;
    data.callback_data = callbackData;
    data.metafile = real_metafile;

    real_metafile->playback_graphics = graphics;
    real_metafile->playback_dc = NULL;
    real_metafile->src_rect = *srcRect;

    memcpy(real_metafile->playback_points, destPoints, sizeof(PointF) * 3);
    stat = GdipTransformPoints(graphics, CoordinateSpaceDevice, CoordinateSpaceWorld, real_metafile->playback_points, 3);

    if (stat == Ok)
        stat = GdipBeginContainer2(graphics, &state);

    if (stat == Ok)
    {
        stat = GdipSetPageScale(graphics, 1.0);

        if (stat == Ok)
            stat = GdipSetPageUnit(graphics, UnitPixel);

        if (stat == Ok)
            stat = GdipResetWorldTransform(graphics);

        if (stat == Ok)
            stat = GdipCreateRegion(&real_metafile->base_clip);

        if (stat == Ok)
            stat = GdipGetClip(graphics, real_metafile->base_clip);

        if (stat == Ok)
            stat = GdipCreateRegion(&real_metafile->clip);

        if (stat == Ok)
            stat = GdipCreatePath(FillModeAlternate, &dst_path);

        if (stat == Ok)
        {
            GpPointF clip_points[4];

            clip_points[0] = real_metafile->playback_points[0];
            clip_points[1] = real_metafile->playback_points[1];
            clip_points[2].X = real_metafile->playback_points[1].X + real_metafile->playback_points[2].X
                - real_metafile->playback_points[0].X;
            clip_points[2].Y = real_metafile->playback_points[1].Y + real_metafile->playback_points[2].Y
                - real_metafile->playback_points[0].Y;
            clip_points[3] = real_metafile->playback_points[2];

            stat = GdipAddPathPolygon(dst_path, clip_points, 4);

            if (stat == Ok)
                stat = GdipCombineRegionPath(real_metafile->base_clip, dst_path, CombineModeIntersect);

            GdipDeletePath(dst_path);
        }

        if (stat == Ok)
            stat = GdipCreateMatrix(&real_metafile->world_transform);

        if (stat == Ok)
        {
            real_metafile->page_unit = UnitDisplay;
            real_metafile->page_scale = 1.0;
            stat = METAFILE_PlaybackUpdateWorldTransform(real_metafile);
        }

        if (stat == Ok)
        {
            stat = METAFILE_PlaybackUpdateClip(real_metafile);
        }

        if (stat == Ok && (metafile->metafile_type == MetafileTypeEmf ||
            metafile->metafile_type == MetafileTypeWmfPlaceable ||
            metafile->metafile_type == MetafileTypeWmf))
            stat = METAFILE_PlaybackGetDC(real_metafile);

        if (stat == Ok)
            EnumEnhMetaFile(0, metafile->hemf, enum_metafile_proc, &data, NULL);

        METAFILE_PlaybackReleaseDC(real_metafile);

        GdipDeleteMatrix(real_metafile->world_transform);
        real_metafile->world_transform = NULL;

        GdipDeleteRegion(real_metafile->base_clip);
        real_metafile->base_clip = NULL;

        GdipDeleteRegion(real_metafile->clip);
        real_metafile->clip = NULL;

        while (list_head(&real_metafile->containers))
        {
            container* cont = LIST_ENTRY(list_head(&real_metafile->containers), container, entry);
            list_remove(&cont->entry);
            GdipDeleteRegion(cont->clip);
            heap_free(cont);
        }

        GdipEndContainer(graphics, state);
    }

    real_metafile->playback_graphics = NULL;

    return stat;
}

GpStatus WINGDIPAPI GdipEnumerateMetafileDestRect(GpGraphics *graphics,
    GDIPCONST GpMetafile *metafile, GDIPCONST GpRectF *dest,
    EnumerateMetafileProc callback, VOID *cb_data, GDIPCONST GpImageAttributes *attrs)
{
    GpPointF points[3];

    if (!graphics || !metafile || !dest) return InvalidParameter;

    points[0].X = points[2].X = dest->X;
    points[0].Y = points[1].Y = dest->Y;
    points[1].X = dest->X + dest->Width;
    points[2].Y = dest->Y + dest->Height;

    return GdipEnumerateMetafileSrcRectDestPoints(graphics, metafile, points, 3,
        &metafile->bounds, metafile->unit, callback, cb_data, attrs);
}

GpStatus WINGDIPAPI GdipEnumerateMetafileDestRectI(GpGraphics *graphics,
    GDIPCONST GpMetafile *metafile, GDIPCONST GpRect *dest,
    EnumerateMetafileProc callback, VOID *cb_data, GDIPCONST GpImageAttributes *attrs)
{
    GpRectF destf;

    if (!graphics || !metafile || !dest) return InvalidParameter;

    destf.X = dest->X;
    destf.Y = dest->Y;
    destf.Width = dest->Width;
    destf.Height = dest->Height;

    return GdipEnumerateMetafileDestRect(graphics, metafile, &destf, callback, cb_data, attrs);
}

GpStatus WINGDIPAPI GdipEnumerateMetafileDestPoint(GpGraphics *graphics,
    GDIPCONST GpMetafile *metafile, GDIPCONST GpPointF *dest,
    EnumerateMetafileProc callback, VOID *cb_data, GDIPCONST GpImageAttributes *attrs)
{
    GpRectF destf;

    if (!graphics || !metafile || !dest) return InvalidParameter;

    destf.X = dest->X;
    destf.Y = dest->Y;
    destf.Width = units_to_pixels(metafile->bounds.Width, metafile->unit, metafile->image.xres);
    destf.Height = units_to_pixels(metafile->bounds.Height, metafile->unit, metafile->image.yres);

    return GdipEnumerateMetafileDestRect(graphics, metafile, &destf, callback, cb_data, attrs);
}

GpStatus WINGDIPAPI GdipEnumerateMetafileDestPointI(GpGraphics *graphics,
    GDIPCONST GpMetafile *metafile, GDIPCONST GpPoint *dest,
    EnumerateMetafileProc callback, VOID *cb_data, GDIPCONST GpImageAttributes *attrs)
{
    GpPointF ptf;

    if (!graphics || !metafile || !dest) return InvalidParameter;

    ptf.X = dest->X;
    ptf.Y = dest->Y;

    return GdipEnumerateMetafileDestPoint(graphics, metafile, &ptf, callback, cb_data, attrs);
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromMetafile(GpMetafile * metafile,
    MetafileHeader * header)
{
    GpStatus status;

    TRACE("(%p, %p)\n", metafile, header);

    if(!metafile || !header)
        return InvalidParameter;

    if (metafile->hemf)
    {
        status = GdipGetMetafileHeaderFromEmf(metafile->hemf, header);
        if (status != Ok) return status;
    }
    else
    {
        memset(header, 0, sizeof(*header));
        header->Version = 0xdbc01002;
    }

    header->Type = metafile->metafile_type;
    header->DpiX = metafile->image.xres;
    header->DpiY = metafile->image.yres;
    header->Width = gdip_round(metafile->bounds.Width);
    header->Height = gdip_round(metafile->bounds.Height);

    return Ok;
}

static int CALLBACK get_emfplus_header_proc(HDC hDC, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR,
    int nObj, LPARAM lpData)
{
    EmfPlusHeader *dst_header = (EmfPlusHeader*)lpData;

    if (lpEMFR->iType == EMR_GDICOMMENT)
    {
        const EMRGDICOMMENT *comment = (const EMRGDICOMMENT*)lpEMFR;

        if (comment->cbData >= 4 && memcmp(comment->Data, "EMF+", 4) == 0)
        {
            const EmfPlusRecordHeader *header = (const EmfPlusRecordHeader*)&comment->Data[4];

            if (4 + sizeof(EmfPlusHeader) <= comment->cbData &&
                header->Type == EmfPlusRecordTypeHeader)
            {
                memcpy(dst_header, header, sizeof(*dst_header));
            }
        }
    }
    else if (lpEMFR->iType == EMR_HEADER)
        return TRUE;

    return FALSE;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromEmf(HENHMETAFILE hemf,
    MetafileHeader *header)
{
    ENHMETAHEADER3 emfheader;
    EmfPlusHeader emfplusheader;
    MetafileType metafile_type;

    TRACE("(%p,%p)\n", hemf, header);

    if(!hemf || !header)
        return InvalidParameter;

    if (GetEnhMetaFileHeader(hemf, sizeof(emfheader), (ENHMETAHEADER*)&emfheader) == 0)
        return GenericError;

    emfplusheader.Header.Type = 0;

    EnumEnhMetaFile(NULL, hemf, get_emfplus_header_proc, &emfplusheader, NULL);

    if (emfplusheader.Header.Type == EmfPlusRecordTypeHeader)
    {
        if ((emfplusheader.Header.Flags & 1) == 1)
            metafile_type = MetafileTypeEmfPlusDual;
        else
            metafile_type = MetafileTypeEmfPlusOnly;
    }
    else
        metafile_type = MetafileTypeEmf;

    header->Type = metafile_type;
    header->Size = emfheader.nBytes;
    header->DpiX = (REAL)emfheader.szlDevice.cx * 25.4 / emfheader.szlMillimeters.cx;
    header->DpiY = (REAL)emfheader.szlDevice.cy * 25.4 / emfheader.szlMillimeters.cy;
    header->X = gdip_round((REAL)emfheader.rclFrame.left / 2540.0 * header->DpiX);
    header->Y = gdip_round((REAL)emfheader.rclFrame.top / 2540.0 * header->DpiY);
    header->Width = gdip_round((REAL)(emfheader.rclFrame.right - emfheader.rclFrame.left) / 2540.0 * header->DpiX);
    header->Height = gdip_round((REAL)(emfheader.rclFrame.bottom - emfheader.rclFrame.top) / 2540.0 * header->DpiY);
    header->u.EmfHeader = emfheader;

    if (metafile_type == MetafileTypeEmfPlusDual || metafile_type == MetafileTypeEmfPlusOnly)
    {
        header->Version = emfplusheader.Version;
        header->EmfPlusFlags = emfplusheader.EmfPlusFlags;
        header->EmfPlusHeaderSize = emfplusheader.Header.Size;
        header->LogicalDpiX = emfplusheader.LogicalDpiX;
        header->LogicalDpiY = emfplusheader.LogicalDpiY;
    }
    else
    {
        header->Version = emfheader.nVersion;
        header->EmfPlusFlags = 0;
        header->EmfPlusHeaderSize = 0;
        header->LogicalDpiX = 0;
        header->LogicalDpiY = 0;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromWmf(HMETAFILE hwmf,
    GDIPCONST WmfPlaceableFileHeader *placeable, MetafileHeader *header)
{
    GpStatus status;
    GpMetafile *metafile;

    TRACE("(%p,%p,%p)\n", hwmf, placeable, header);

    status = GdipCreateMetafileFromWmf(hwmf, FALSE, placeable, &metafile);
    if (status == Ok)
    {
        status = GdipGetMetafileHeaderFromMetafile(metafile, header);
        GdipDisposeImage(&metafile->image);
    }
    return status;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromFile(GDIPCONST WCHAR *filename,
    MetafileHeader *header)
{
    GpStatus status;
    GpMetafile *metafile;

    TRACE("(%s,%p)\n", debugstr_w(filename), header);

    if (!filename || !header)
        return InvalidParameter;

    status = GdipCreateMetafileFromFile(filename, &metafile);
    if (status == Ok)
    {
        status = GdipGetMetafileHeaderFromMetafile(metafile, header);
        GdipDisposeImage(&metafile->image);
    }
    return status;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromStream(IStream *stream,
    MetafileHeader *header)
{
    GpStatus status;
    GpMetafile *metafile;

    TRACE("(%p,%p)\n", stream, header);

    if (!stream || !header)
        return InvalidParameter;

    status = GdipCreateMetafileFromStream(stream, &metafile);
    if (status == Ok)
    {
        status = GdipGetMetafileHeaderFromMetafile(metafile, header);
        GdipDisposeImage(&metafile->image);
    }
    return status;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromEmf(HENHMETAFILE hemf, BOOL delete,
    GpMetafile **metafile)
{
    GpStatus stat;
    MetafileHeader header;

    TRACE("(%p,%i,%p)\n", hemf, delete, metafile);

    if(!hemf || !metafile)
        return InvalidParameter;

    stat = GdipGetMetafileHeaderFromEmf(hemf, &header);
    if (stat != Ok)
        return stat;

    *metafile = heap_alloc_zero(sizeof(GpMetafile));
    if (!*metafile)
        return OutOfMemory;

    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->image.format = ImageFormatEMF;
    (*metafile)->image.frame_count = 1;
    (*metafile)->image.xres = header.DpiX;
    (*metafile)->image.yres = header.DpiY;
    (*metafile)->bounds.X = (REAL)header.u.EmfHeader.rclFrame.left / 2540.0 * header.DpiX;
    (*metafile)->bounds.Y = (REAL)header.u.EmfHeader.rclFrame.top / 2540.0 * header.DpiY;
    (*metafile)->bounds.Width = (REAL)(header.u.EmfHeader.rclFrame.right - header.u.EmfHeader.rclFrame.left)
                                / 2540.0 * header.DpiX;
    (*metafile)->bounds.Height = (REAL)(header.u.EmfHeader.rclFrame.bottom - header.u.EmfHeader.rclFrame.top)
                                 / 2540.0 * header.DpiY;
    (*metafile)->unit = UnitPixel;
    (*metafile)->metafile_type = header.Type;
    (*metafile)->hemf = hemf;
    (*metafile)->preserve_hemf = !delete;
    list_init(&(*metafile)->containers);

    TRACE("<-- %p\n", *metafile);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromWmf(HMETAFILE hwmf, BOOL delete,
    GDIPCONST WmfPlaceableFileHeader * placeable, GpMetafile **metafile)
{
    UINT read;
    BYTE *copy;
    HENHMETAFILE hemf;
    GpStatus retval = Ok;

    TRACE("(%p, %d, %p, %p)\n", hwmf, delete, placeable, metafile);

    if(!hwmf || !metafile)
        return InvalidParameter;

    *metafile = NULL;
    read = GetMetaFileBitsEx(hwmf, 0, NULL);
    if(!read)
        return GenericError;
    copy = heap_alloc_zero(read);
    GetMetaFileBitsEx(hwmf, read, copy);

    hemf = SetWinMetaFileBits(read, copy, NULL, NULL);
    heap_free(copy);

    /* FIXME: We should store and use hwmf instead of converting to hemf */
    retval = GdipCreateMetafileFromEmf(hemf, TRUE, metafile);

    if (retval == Ok)
    {
        if (placeable)
        {
            (*metafile)->image.xres = (REAL)placeable->Inch;
            (*metafile)->image.yres = (REAL)placeable->Inch;
            (*metafile)->bounds.X = ((REAL)placeable->BoundingBox.Left) / ((REAL)placeable->Inch);
            (*metafile)->bounds.Y = ((REAL)placeable->BoundingBox.Top) / ((REAL)placeable->Inch);
            (*metafile)->bounds.Width = (REAL)(placeable->BoundingBox.Right -
                                               placeable->BoundingBox.Left);
            (*metafile)->bounds.Height = (REAL)(placeable->BoundingBox.Bottom -
                                                placeable->BoundingBox.Top);
            (*metafile)->metafile_type = MetafileTypeWmfPlaceable;
        }
        else
            (*metafile)->metafile_type = MetafileTypeWmf;
        (*metafile)->image.format = ImageFormatWMF;

        if (delete) DeleteMetaFile(hwmf);
    }
    else
        DeleteEnhMetaFile(hemf);
    return retval;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromWmfFile(GDIPCONST WCHAR *file,
    GDIPCONST WmfPlaceableFileHeader * placeable, GpMetafile **metafile)
{
    HMETAFILE hmf = GetMetaFileW(file);

    TRACE("(%s, %p, %p)\n", debugstr_w(file), placeable, metafile);

    if(!hmf) return InvalidParameter;

    return GdipCreateMetafileFromWmf(hmf, TRUE, placeable, metafile);
}

GpStatus WINGDIPAPI GdipCreateMetafileFromFile(GDIPCONST WCHAR *file,
    GpMetafile **metafile)
{
    GpStatus status;
    IStream *stream;

    TRACE("(%p, %p)\n", file, metafile);

    if (!file || !metafile) return InvalidParameter;

    *metafile = NULL;

    status = GdipCreateStreamOnFile(file, GENERIC_READ, &stream);
    if (status == Ok)
    {
        status = GdipCreateMetafileFromStream(stream, metafile);
        IStream_Release(stream);
    }
    return status;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromStream(IStream *stream,
    GpMetafile **metafile)
{
    GpStatus stat;

    TRACE("%p %p\n", stream, metafile);

    stat = GdipLoadImageFromStream(stream, (GpImage **)metafile);
    if (stat != Ok) return stat;

    if ((*metafile)->image.type != ImageTypeMetafile)
    {
        GdipDisposeImage(&(*metafile)->image);
        *metafile = NULL;
        return GenericError;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipSetMetafileDownLevelRasterizationLimit(GpMetafile *metafile,
    UINT limitDpi)
{
    TRACE("(%p,%u)\n", metafile, limitDpi);

    return Ok;
}

GpStatus WINGDIPAPI GdipConvertToEmfPlus(const GpGraphics* ref,
    GpMetafile* metafile, BOOL* succ, EmfType emfType,
    const WCHAR* description, GpMetafile** out_metafile)
{
    static int calls;

    TRACE("(%p,%p,%p,%u,%s,%p)\n", ref, metafile, succ, emfType,
        debugstr_w(description), out_metafile);

    if(!ref || !metafile || !out_metafile || emfType < EmfTypeEmfOnly || emfType > EmfTypeEmfPlusDual)
        return InvalidParameter;

    if(succ)
        *succ = FALSE;
    *out_metafile = NULL;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipEmfToWmfBits(HENHMETAFILE hemf, UINT cbData16,
    LPBYTE pData16, INT iMapMode, INT eFlags)
{
    FIXME("(%p, %d, %p, %d, %d): stub\n", hemf, cbData16, pData16, iMapMode, eFlags);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipRecordMetafileFileName(GDIPCONST WCHAR* fileName,
                            HDC hdc, EmfType type, GDIPCONST GpRectF *pFrameRect,
                            MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc,
                            GpMetafile **metafile)
{
    FIXME("%s %p %d %p %d %s %p stub!\n", debugstr_w(fileName), hdc, type, pFrameRect,
                                 frameUnit, debugstr_w(desc), metafile);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipRecordMetafileFileNameI(GDIPCONST WCHAR* fileName, HDC hdc, EmfType type,
                            GDIPCONST GpRect *pFrameRect, MetafileFrameUnit frameUnit,
                            GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    FIXME("%s %p %d %p %d %s %p stub!\n", debugstr_w(fileName), hdc, type, pFrameRect,
                                 frameUnit, debugstr_w(desc), metafile);

    return NotImplemented;
}

/*****************************************************************************
 * GdipConvertToEmfPlusToFile [GDIPLUS.@]
 */

GpStatus WINGDIPAPI GdipConvertToEmfPlusToFile(const GpGraphics* refGraphics,
                                               GpMetafile* metafile, BOOL* conversionSuccess,
                                               const WCHAR* filename, EmfType emfType,
                                               const WCHAR* description, GpMetafile** out_metafile)
{
    FIXME("stub: %p, %p, %p, %p, %u, %p, %p\n", refGraphics, metafile, conversionSuccess, filename, emfType, description, out_metafile);
    return NotImplemented;
}
