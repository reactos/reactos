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

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/unicode.h"

#define COBJMACROS
#include "objbase.h"
#include "ocidl.h"
#include "olectl.h"
#include "ole2.h"

#include "winreg.h"
#include "shlwapi.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

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

static GpStatus METAFILE_AllocateRecord(GpMetafile *metafile, DWORD size, void **result)
{
    DWORD size_needed;
    EmfPlusRecordHeader *record;

    if (!metafile->comment_data_size)
    {
        DWORD data_size = max(256, size * 2 + 4);
        metafile->comment_data = GdipAlloc(data_size);

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
        BYTE *new_data = GdipAlloc(data_size);

        if (!new_data)
            return OutOfMemory;

        memcpy(new_data, metafile->comment_data, metafile->comment_data_length);

        metafile->comment_data_size = data_size;
        GdipFree(metafile->comment_data);
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

    switch (frameUnit)
    {
    case MetafileFrameUnitPixel:
        framerect_factor_x = 2540.0 / GetDeviceCaps(hdc, LOGPIXELSX);
        framerect_factor_y = 2540.0 / GetDeviceCaps(hdc, LOGPIXELSY);
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

    *metafile = GdipAlloc(sizeof(GpMetafile));
    if(!*metafile)
    {
        DeleteEnhMetaFile(CloseEnhMetaFile(record_dc));
        return OutOfMemory;
    }

    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->image.picture = NULL;
    (*metafile)->image.flags   = ImageFlagsNone;
    (*metafile)->image.palette_flags = 0;
    (*metafile)->image.palette_count = 0;
    (*metafile)->image.palette_size = 0;
    (*metafile)->image.palette_entries = NULL;
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
        GdipFree(*metafile);
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
        *result = metafile->record_graphics;

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

    GdipFree(metafile->comment_data);
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

GpStatus WINGDIPAPI GdipPlayMetafileRecord(GDIPCONST GpMetafile *metafile,
    EmfPlusRecordType recordType, UINT flags, UINT dataSize, GDIPCONST BYTE *data)
{
    TRACE("(%p,%x,%x,%d,%p)\n", metafile, recordType, flags, dataSize, data);

    if (!metafile || (dataSize && !data) || !metafile->playback_graphics)
        return InvalidParameter;

    if (recordType >= 1 && recordType <= 0x7a)
    {
        /* regular EMF record */
        if (metafile->playback_dc)
        {
            ENHMETARECORD *record;

            record = GdipAlloc(dataSize + 8);

            if (record)
            {
                record->iType = recordType;
                record->nSize = dataSize;
                memcpy(record->dParm, data, dataSize);

                PlayEnhMetaFileRecord(metafile->playback_dc, metafile->handle_table,
                    record, metafile->handle_count);

                GdipFree(record);
            }
            else
                return OutOfMemory;
        }
    }
    else
    {
        METAFILE_PlaybackReleaseDC((GpMetafile*)metafile);

        switch(recordType)
        {
        case EmfPlusRecordTypeHeader:
        case EmfPlusRecordTypeEndOfFile:
            break;
        case EmfPlusRecordTypeGetDC:
            METAFILE_PlaybackGetDC((GpMetafile*)metafile);
            break;
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

    memcpy(real_metafile->playback_points, destPoints, sizeof(PointF) * 3);
    stat = GdipTransformPoints(graphics, CoordinateSpaceDevice, CoordinateSpaceWorld, real_metafile->playback_points, 3);

    if (stat == Ok && metafile->metafile_type == MetafileTypeEmf)
        stat = METAFILE_PlaybackGetDC((GpMetafile*)metafile);

    if (stat == Ok)
        EnumEnhMetaFile(0, metafile->hemf, enum_metafile_proc, &data, NULL);

    METAFILE_PlaybackReleaseDC((GpMetafile*)metafile);

    real_metafile->playback_graphics = NULL;

    return stat;
}
