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

typedef struct EmfPlusFillRects
{
    EmfPlusRecordHeader Header;
    DWORD BrushID;
    DWORD Count;
} EmfPlusFillRects;

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
    RECT rc;
    GpStatus stat;

    TRACE("(%p %d %p %d %p %p)\n", hdc, type, frameRect, frameUnit, desc, metafile);

    if (!hdc || type < EmfTypeEmfOnly || type > EmfTypeEmfPlusDual || !metafile)
        return InvalidParameter;

    if (!frameRect)
    {
        FIXME("not implemented for NULL rect\n");
        return NotImplemented;
    }

    dpix = (REAL)GetDeviceCaps(hdc, HORZRES) / GetDeviceCaps(hdc, HORZSIZE) * 25.4;
    dpiy = (REAL)GetDeviceCaps(hdc, VERTRES) / GetDeviceCaps(hdc, VERTSIZE) * 25.4;

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

    record_dc = CreateEnhMetaFileW(hdc, NULL, &rc, desc);

    if (!record_dc)
        return GenericError;

    *metafile = heap_alloc_zero(sizeof(GpMetafile));
    if(!*metafile)
    {
        DeleteEnhMetaFile(CloseEnhMetaFile(record_dc));
        return OutOfMemory;
    }

    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->image.picture = NULL;
    (*metafile)->image.flags   = ImageFlagsNone;
    (*metafile)->image.palette = NULL;
    (*metafile)->image.xres = dpix;
    (*metafile)->image.yres = dpiy;
    (*metafile)->bounds = *frameRect;
    (*metafile)->unit = frameUnit;
    (*metafile)->metafile_type = type;
    (*metafile)->record_dc = record_dc;
    (*metafile)->comment_data = NULL;
    (*metafile)->comment_data_size = 0;
    (*metafile)->comment_data_length = 0;
    (*metafile)->hemf = NULL;

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

static GpStatus METAFILE_PlaybackGetDC(GpMetafile *metafile)
{
    GpStatus stat = Ok;

    stat = GdipGetDC(metafile->playback_graphics, &metafile->playback_dc);

    if (stat == Ok)
    {
        /* The result of GdipGetDC always expects device co-ordinates, but the
         * device co-ordinates of the source metafile do not correspond to
         * device co-ordinates of the destination. Therefore, we set up the DC
         * so that the metafile's bounds map to the destination points where we
         * are drawing this metafile. */
        SetMapMode(metafile->playback_dc, MM_ANISOTROPIC);

        SetWindowOrgEx(metafile->playback_dc, metafile->bounds.X, metafile->bounds.Y, NULL);
        SetWindowExtEx(metafile->playback_dc, metafile->bounds.Width, metafile->bounds.Height, NULL);

        SetViewportOrgEx(metafile->playback_dc, metafile->playback_points[0].X, metafile->playback_points[0].Y, NULL);
        SetViewportExtEx(metafile->playback_dc,
            metafile->playback_points[1].X - metafile->playback_points[0].X,
            metafile->playback_points[2].Y - metafile->playback_points[0].Y, NULL);
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
            stat = GdipCreateMatrix(&real_metafile->world_transform);

        if (stat == Ok)
        {
            real_metafile->page_unit = UnitDisplay;
            real_metafile->page_scale = 1.0;
            stat = METAFILE_PlaybackUpdateWorldTransform(real_metafile);
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
    static int calls;

    TRACE("(%p, %p)\n", metafile, header);

    if(!metafile || !header)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    memset(header, 0, sizeof(MetafileHeader));

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

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromFile(GDIPCONST WCHAR *filename,
    MetafileHeader *header)
{
    static int calls;

    TRACE("(%s,%p)\n", debugstr_w(filename), header);

    if(!filename || !header)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    memset(header, 0, sizeof(MetafileHeader));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromStream(IStream *stream,
    MetafileHeader *header)
{
    static int calls;

    TRACE("(%p,%p)\n", stream, header);

    if(!stream || !header)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    memset(header, 0, sizeof(MetafileHeader));

    return Ok;
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
    FIXME("(%p, %p): stub\n", file, metafile);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromStream(IStream *stream,
    GpMetafile **metafile)
{
    FIXME("(%p, %p): stub\n", stream, metafile);
    return NotImplemented;
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
