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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

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

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT, IWICImagingFactory**);

typedef ARGB EmfPlusARGB;

typedef struct EmfPlusPointF
{
    float X;
    float Y;
} EmfPlusPointF;

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

enum PenDataFlags
{
    PenDataTransform        = 0x0001,
    PenDataStartCap         = 0x0002,
    PenDataEndCap           = 0x0004,
    PenDataJoin             = 0x0008,
    PenDataMiterLimit       = 0x0010,
    PenDataLineStyle        = 0x0020,
    PenDataDashedLineCap    = 0x0040,
    PenDataDashedLineOffset = 0x0080,
    PenDataDashedLine       = 0x0100,
    PenDataNonCenter        = 0x0200,
    PenDataCompoundLine     = 0x0400,
    PenDataCustomStartCap   = 0x0800,
    PenDataCustomEndCap     = 0x1000
};

enum CustomLineCapData
{
    CustomLineCapDataFillPath = 0x1,
    CustomLineCapDataLinePath = 0x2,
};

typedef struct EmfPlusTransformMatrix
{
    REAL TransformMatrix[6];
} EmfPlusTransformMatrix;

enum LineStyle
{
    LineStyleSolid,
    LineStyleDash,
    LineStyleDot,
    LineStyleDashDot,
    LineStyleDashDotDot,
    LineStyleCustom
};

typedef struct EmfPlusDashedLineData
{
    DWORD DashedLineDataSize;
    BYTE data[1];
} EmfPlusDashedLineData;

typedef struct EmfPlusCompoundLineData
{
    DWORD CompoundLineDataSize;
    BYTE data[1];
} EmfPlusCompoundLineData;

typedef struct EmfPlusCustomStartCapData
{
    DWORD CustomStartCapSize;
    BYTE data[1];
} EmfPlusCustomStartCapData;

typedef struct EmfPlusCustomEndCapData
{
    DWORD CustomEndCapSize;
    BYTE data[1];
} EmfPlusCustomEndCapData;

typedef struct EmfPlusPenData
{
    DWORD PenDataFlags;
    DWORD PenUnit;
    REAL PenWidth;
    BYTE OptionalData[1];
} EmfPlusPenData;

enum BrushDataFlags
{
    BrushDataPath             = 1 << 0,
    BrushDataTransform        = 1 << 1,
    BrushDataPresetColors     = 1 << 2,
    BrushDataBlendFactorsH    = 1 << 3,
    BrushDataBlendFactorsV    = 1 << 4,
    BrushDataFocusScales      = 1 << 6,
    BrushDataIsGammaCorrected = 1 << 7,
    BrushDataDoNotTransform   = 1 << 8,
};

typedef struct EmfPlusSolidBrushData
{
    EmfPlusARGB SolidColor;
} EmfPlusSolidBrushData;

typedef struct EmfPlusHatchBrushData
{
    DWORD HatchStyle;
    EmfPlusARGB ForeColor;
    EmfPlusARGB BackColor;
} EmfPlusHatchBrushData;

typedef struct EmfPlusTextureBrushData
{
    DWORD BrushDataFlags;
    INT WrapMode;
    BYTE OptionalData[1];
} EmfPlusTextureBrushData;

typedef struct EmfPlusRectF
{
    float X;
    float Y;
    float Width;
    float Height;
} EmfPlusRectF;

typedef struct EmfPlusLinearGradientBrushData
{
    DWORD BrushDataFlags;
    INT WrapMode;
    EmfPlusRectF RectF;
    EmfPlusARGB StartColor;
    EmfPlusARGB EndColor;
    DWORD Reserved1;
    DWORD Reserved2;
    BYTE OptionalData[1];
} EmfPlusLinearGradientBrushData;

typedef struct EmfPlusBrush
{
    DWORD Version;
    DWORD Type;
    union {
        EmfPlusSolidBrushData solid;
        EmfPlusHatchBrushData hatch;
        EmfPlusTextureBrushData texture;
        EmfPlusLinearGradientBrushData lineargradient;
    } BrushData;
} EmfPlusBrush;

typedef struct EmfPlusCustomLineCapArrowData
{
    REAL Width;
    REAL Height;
    REAL MiddleInset;
    BOOL FillState;
    DWORD LineStartCap;
    DWORD LineEndCap;
    DWORD LineJoin;
    REAL LineMiterLimit;
    REAL WidthScale;
    EmfPlusPointF FillHotSpot;
    EmfPlusPointF LineHotSpot;
} EmfPlusCustomLineCapArrowData;

typedef struct EmfPlusPath
{
    DWORD Version;
    DWORD PathPointCount;
    DWORD PathPointFlags;
    /* PathPoints[] */
    /* PathPointTypes[] */
    /* AlignmentPadding */
    BYTE data[1];
} EmfPlusPath;

typedef struct EmfPlusCustomLineCapDataFillPath
{
    INT FillPathLength;
    /* EmfPlusPath */
    BYTE FillPath[1];
} EmfPlusCustomLineCapDataFillPath;

typedef struct EmfPlusCustomLineCapDataLinePath
{
    INT LinePathLength;
    /* EmfPlusPath */
    BYTE LinePath[1];
} EmfPlusCustomLineCapDataLinePath;

typedef struct EmfPlusCustomLineCapData
{
    DWORD CustomLineCapDataFlags;
    DWORD BaseCap;
    REAL BaseInset;
    DWORD StrokeStartCap;
    DWORD StrokeEndCap;
    DWORD StrokeJoin;
    REAL StrokeMiterLimit;
    REAL WidthScale;
    EmfPlusPointF FillHotSpot;
    EmfPlusPointF LineHotSpot;
    /* EmfPlusCustomLineCapDataFillPath */
    /* EmfPlusCustomLineCapDataLinePath */
    BYTE OptionalData[1];
} EmfPlusCustomLineCapData;

typedef struct EmfPlusCustomLineCap
{
    DWORD Version;
    DWORD Type;
    /* EmfPlusCustomLineCapArrowData */
    /* EmfPlusCustomLineCapData */
    BYTE CustomLineCapData[1];
} EmfPlusCustomLineCap;

typedef struct EmfPlusPen
{
    DWORD Version;
    DWORD Type;
    /* EmfPlusPenData */
    /* EmfPlusBrush */
    BYTE data[1];
} EmfPlusPen;

typedef struct EmfPlusRegionNodePath
{
    DWORD RegionNodePathLength;
    EmfPlusPath RegionNodePath;
} EmfPlusRegionNodePath;

typedef struct EmfPlusRegion
{
    DWORD Version;
    DWORD RegionNodeCount;
    BYTE RegionNode[1];
} EmfPlusRegion;

typedef struct EmfPlusPalette
{
    DWORD PaletteStyleFlags;
    DWORD PaletteCount;
    BYTE PaletteEntries[1];
} EmfPlusPalette;

typedef enum
{
    BitmapDataTypePixel,
    BitmapDataTypeCompressed,
} BitmapDataType;

typedef struct EmfPlusBitmap
{
    DWORD Width;
    DWORD Height;
    DWORD Stride;
    DWORD PixelFormat;
    DWORD Type;
    BYTE BitmapData[1];
} EmfPlusBitmap;

typedef struct EmfPlusMetafile
{
    DWORD Type;
    DWORD MetafileDataSize;
    BYTE MetafileData[1];
} EmfPlusMetafile;

typedef enum ImageDataType
{
    ImageDataTypeUnknown,
    ImageDataTypeBitmap,
    ImageDataTypeMetafile,
} ImageDataType;

typedef struct EmfPlusImage
{
    DWORD Version;
    ImageDataType Type;
    union
    {
        EmfPlusBitmap bitmap;
        EmfPlusMetafile metafile;
    } ImageData;
} EmfPlusImage;

typedef struct EmfPlusImageAttributes
{
    DWORD Version;
    DWORD Reserved1;
    DWORD WrapMode;
    EmfPlusARGB ClampColor;
    DWORD ObjectClamp;
    DWORD Reserved2;
} EmfPlusImageAttributes;

typedef struct EmfPlusFont
{
    DWORD Version;
    float EmSize;
    DWORD SizeUnit;
    DWORD FontStyleFlags;
    DWORD Reserved;
    DWORD Length;
    WCHAR FamilyName[1];
} EmfPlusFont;

typedef struct EmfPlusObject
{
    EmfPlusRecordHeader Header;
    union
    {
        EmfPlusBrush brush;
        EmfPlusPen pen;
        EmfPlusPath path;
        EmfPlusRegion region;
        EmfPlusImage image;
        EmfPlusImageAttributes image_attributes;
        EmfPlusFont font;
    } ObjectData;
} EmfPlusObject;

typedef struct EmfPlusPointR7
{
    BYTE X;
    BYTE Y;
} EmfPlusPointR7;

typedef struct EmfPlusPoint
{
    short X;
    short Y;
} EmfPlusPoint;

typedef struct EmfPlusDrawImage
{
    EmfPlusRecordHeader Header;
    DWORD ImageAttributesID;
    DWORD SrcUnit;
    EmfPlusRectF SrcRect;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusDrawImage;

typedef struct EmfPlusDrawImagePoints
{
    EmfPlusRecordHeader Header;
    DWORD ImageAttributesID;
    DWORD SrcUnit;
    EmfPlusRectF SrcRect;
    DWORD count;
    union
    {
        EmfPlusPointR7 pointsR[3];
        EmfPlusPoint points[3];
        EmfPlusPointF pointsF[3];
    } PointData;
} EmfPlusDrawImagePoints;

typedef struct EmfPlusDrawPath
{
    EmfPlusRecordHeader Header;
    DWORD PenId;
} EmfPlusDrawPath;

typedef struct EmfPlusDrawArc
{
    EmfPlusRecordHeader Header;
    float StartAngle;
    float SweepAngle;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusDrawArc;

typedef struct EmfPlusDrawEllipse
{
    EmfPlusRecordHeader Header;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusDrawEllipse;

typedef struct EmfPlusDrawPie
{
    EmfPlusRecordHeader Header;
    float StartAngle;
    float SweepAngle;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusDrawPie;

typedef struct EmfPlusDrawRects
{
    EmfPlusRecordHeader Header;
    DWORD Count;
    union
    {
        EmfPlusRect rect[1];
        EmfPlusRectF rectF[1];
    } RectData;
} EmfPlusDrawRects;

typedef struct EmfPlusFillPath
{
    EmfPlusRecordHeader Header;
    union
    {
        DWORD BrushId;
        EmfPlusARGB Color;
    } data;
} EmfPlusFillPath;

typedef struct EmfPlusFillClosedCurve
{
    EmfPlusRecordHeader Header;
    DWORD BrushId;
    float Tension;
    DWORD Count;
    union
    {
        EmfPlusPointR7 pointsR[1];
        EmfPlusPoint points[1];
        EmfPlusPointF pointsF[1];
    } PointData;
} EmfPlusFillClosedCurve;

typedef struct EmfPlusFillEllipse
{
    EmfPlusRecordHeader Header;
    DWORD BrushId;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusFillEllipse;

typedef struct EmfPlusFillPie
{
    EmfPlusRecordHeader Header;
    DWORD BrushId;
    float StartAngle;
    float SweepAngle;
    union
    {
        EmfPlusRect rect;
        EmfPlusRectF rectF;
    } RectData;
} EmfPlusFillPie;

typedef struct EmfPlusDrawDriverString
{
    EmfPlusRecordHeader Header;
    union
    {
        DWORD BrushId;
        ARGB Color;
    } brush;
    DWORD DriverStringOptionsFlags;
    DWORD MatrixPresent;
    DWORD GlyphCount;
    BYTE VariableData[1];
} EmfPlusDrawDriverString;

typedef struct EmfPlusFillRegion
{
    EmfPlusRecordHeader Header;
    union
    {
        DWORD BrushId;
        EmfPlusARGB Color;
    } data;
} EmfPlusFillRegion;

typedef struct EmfPlusOffsetClip
{
    EmfPlusRecordHeader Header;
    float dx;
    float dy;
} EmfPlusOffsetClip;

typedef struct EmfPlusSetRenderingOrigin
{
    EmfPlusRecordHeader Header;
    INT x;
    INT y;
} EmfPlusSetRenderingOrigin;

static void metafile_free_object_table_entry(GpMetafile *metafile, BYTE id)
{
    struct emfplus_object *object = &metafile->objtable[id];

    switch (object->type)
    {
    case ObjectTypeInvalid:
        break;
    case ObjectTypeBrush:
        GdipDeleteBrush(object->u.brush);
        break;
    case ObjectTypePen:
        GdipDeletePen(object->u.pen);
        break;
    case ObjectTypePath:
        GdipDeletePath(object->u.path);
        break;
    case ObjectTypeRegion:
        GdipDeleteRegion(object->u.region);
        break;
    case ObjectTypeImage:
        GdipDisposeImage(object->u.image);
        break;
    case ObjectTypeFont:
        GdipDeleteFont(object->u.font);
        break;
    case ObjectTypeImageAttributes:
        GdipDisposeImageAttributes(object->u.image_attributes);
        break;
    default:
        FIXME("not implemented for object type %u.\n", object->type);
        return;
    }

    object->type = ObjectTypeInvalid;
    object->u.object = NULL;
}

void METAFILE_Free(GpMetafile *metafile)
{
    unsigned int i;

    free(metafile->comment_data);
    DeleteEnhMetaFile(CloseEnhMetaFile(metafile->record_dc));
    if (!metafile->preserve_hemf)
        DeleteEnhMetaFile(metafile->hemf);
    if (metafile->record_graphics)
    {
        WARN("metafile closed while recording\n");
        /* not sure what to do here; for now just prevent the graphics from functioning or using this object */
        metafile->record_graphics->image = NULL;
        metafile->record_graphics->busy = TRUE;
    }

    if (metafile->record_stream)
        IStream_Release(metafile->record_stream);

    for (i = 0; i < ARRAY_SIZE(metafile->objtable); i++)
        metafile_free_object_table_entry(metafile, i);
}

static DWORD METAFILE_AddObjectId(GpMetafile *metafile)
{
    return (metafile->next_object_id++) % EmfPlusObjectTableSize;
}

static GpStatus METAFILE_AllocateRecord(GpMetafile *metafile, EmfPlusRecordType record_type,
        DWORD size, void **result)
{
    DWORD size_needed;
    EmfPlusRecordHeader *record;

    if (!metafile->comment_data_size)
    {
        DWORD data_size = max(256, size * 2 + 4);
        metafile->comment_data = calloc(1, data_size);

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
        BYTE *new_data = calloc(1, data_size);

        if (!new_data)
            return OutOfMemory;

        memcpy(new_data, metafile->comment_data, metafile->comment_data_length);

        metafile->comment_data_size = data_size;
        free(metafile->comment_data);
        metafile->comment_data = new_data;
    }

    *result = metafile->comment_data + metafile->comment_data_length;
    metafile->comment_data_length += size;

    record = (EmfPlusRecordHeader*)*result;
    record->Type = record_type;
    record->Flags = 0;
    record->Size = size;
    record->DataSize = size - sizeof(EmfPlusRecordHeader);

    return Ok;
}

static void METAFILE_RemoveLastRecord(GpMetafile *metafile, EmfPlusRecordHeader *record)
{
    assert(metafile->comment_data + metafile->comment_data_length == (BYTE*)record + record->Size);
    metafile->comment_data_length -=  record->Size;
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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeHeader, sizeof(EmfPlusHeader), (void**)&header);
        if (stat != Ok)
            return stat;

        if (metafile->metafile_type == MetafileTypeEmfPlusDual)
            header->Header.Flags = 1;

        header->Version = VERSION_MAGIC2;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeEndOfFile, sizeof(EmfPlusRecordHeader), (void**)&record);
        if (stat != Ok)
            return stat;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipRecordMetafile(HDC hdc, EmfType type, GDIPCONST GpRectF *frameRect,
                                       MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{

    TRACE("(%p %d %s %d %p %p)\n", hdc, type, debugstr_rectf(frameRect), frameUnit, desc, metafile);

    return GdipRecordMetafileFileName(NULL, hdc, type, frameRect, frameUnit, desc, metafile);
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
        set_rect(&frameRectF, frameRect->X, frameRect->Y, frameRect->Width, frameRect->Height);
        pFrameRectF = &frameRectF;
    }
    else
        pFrameRectF = NULL;

    return GdipRecordMetafile(hdc, type, pFrameRectF, frameUnit, desc, metafile);
}

GpStatus WINGDIPAPI GdipRecordMetafileStreamI(IStream *stream, HDC hdc, EmfType type, GDIPCONST GpRect *frameRect,
                                        MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    GpRectF frameRectF, *pFrameRectF;

    TRACE("(%p %p %d %p %d %p %p)\n", stream, hdc, type, frameRect, frameUnit, desc, metafile);

    if (frameRect)
    {
        set_rect(&frameRectF, frameRect->X, frameRect->Y, frameRect->Width, frameRect->Height);
        pFrameRectF = &frameRectF;
    }
    else
        pFrameRectF = NULL;

    return GdipRecordMetafileStream(stream, hdc, type, pFrameRectF, frameUnit, desc, metafile);
}

GpStatus WINGDIPAPI GdipRecordMetafileStream(IStream *stream, HDC hdc, EmfType type, GDIPCONST GpRectF *frameRect,
                                        MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    GpStatus stat;

    TRACE("(%p %p %d %s %d %p %p)\n", stream, hdc, type, debugstr_rectf(frameRect), frameUnit, desc, metafile);

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
        metafile->record_graphics->xres = metafile->logical_dpix;
        metafile->record_graphics->yres = metafile->logical_dpiy;
        metafile->record_graphics->printer_display = metafile->printer_display;
    }

    return stat;
}

GpStatus METAFILE_GetDC(GpMetafile* metafile, HDC *hdc)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusRecordHeader *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeGetDC, sizeof(EmfPlusRecordHeader), (void**)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeClear, sizeof(EmfPlusClear), (void**)&record);
        if (stat != Ok)
            return stat;

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

static GpStatus METAFILE_PrepareBrushData(GDIPCONST GpBrush *brush, DWORD *size)
{
    switch (brush->bt)
    {
    case BrushTypeSolidColor:
        *size = FIELD_OFFSET(EmfPlusBrush, BrushData) + sizeof(EmfPlusSolidBrushData);
        break;
    case BrushTypeHatchFill:
        *size = FIELD_OFFSET(EmfPlusBrush, BrushData) + sizeof(EmfPlusHatchBrushData);
        break;
    case BrushTypeLinearGradient:
    {
        BOOL ignore_xform;
        GpLineGradient *gradient = (GpLineGradient*)brush;

        *size = FIELD_OFFSET(EmfPlusBrush, BrushData.lineargradient.OptionalData);

        GdipIsMatrixIdentity(&gradient->transform, &ignore_xform);
        if (!ignore_xform)
            *size += sizeof(gradient->transform);

        if (gradient->pblendcount > 1 && gradient->pblendcolor && gradient->pblendpos)
            *size += sizeof(DWORD) + gradient->pblendcount *
                (sizeof(*gradient->pblendcolor) + sizeof(*gradient->pblendpos));
        else if (gradient->blendcount > 1 && gradient->blendfac && gradient->blendpos)
            *size += sizeof(DWORD) + gradient->blendcount *
                (sizeof(*gradient->blendfac) + sizeof(*gradient->blendpos));

        break;
    }
    default:
        FIXME("unsupported brush type: %d\n", brush->bt);
        return NotImplemented;
    }

    return Ok;
}

static void METAFILE_FillBrushData(GDIPCONST GpBrush *brush, EmfPlusBrush *data)
{
    data->Version = VERSION_MAGIC2;
    data->Type = brush->bt;

    switch (brush->bt)
    {
    case BrushTypeSolidColor:
    {
        GpSolidFill *solid = (GpSolidFill *)brush;
        data->BrushData.solid.SolidColor = solid->color;
        break;
    }
    case BrushTypeHatchFill:
    {
        GpHatch *hatch = (GpHatch *)brush;
        data->BrushData.hatch.HatchStyle = hatch->hatchstyle;
        data->BrushData.hatch.ForeColor = hatch->forecol;
        data->BrushData.hatch.BackColor = hatch->backcol;
        break;
    }
    case BrushTypeLinearGradient:
    {
        BYTE *cursor;
        BOOL ignore_xform;
        GpLineGradient *gradient = (GpLineGradient*)brush;

        data->BrushData.lineargradient.BrushDataFlags = 0;
        data->BrushData.lineargradient.WrapMode = gradient->wrap;
        data->BrushData.lineargradient.RectF.X = gradient->rect.X;
        data->BrushData.lineargradient.RectF.Y = gradient->rect.Y;
        data->BrushData.lineargradient.RectF.Width = gradient->rect.Width;
        data->BrushData.lineargradient.RectF.Height = gradient->rect.Height;
        data->BrushData.lineargradient.StartColor = gradient->startcolor;
        data->BrushData.lineargradient.EndColor = gradient->endcolor;
        data->BrushData.lineargradient.Reserved1 = gradient->startcolor;
        data->BrushData.lineargradient.Reserved2 = gradient->endcolor;

        if (gradient->gamma)
            data->BrushData.lineargradient.BrushDataFlags |= BrushDataIsGammaCorrected;

        cursor = &data->BrushData.lineargradient.OptionalData[0];

        GdipIsMatrixIdentity(&gradient->transform, &ignore_xform);
        if (!ignore_xform)
        {
            data->BrushData.lineargradient.BrushDataFlags |= BrushDataTransform;
            memcpy(cursor, &gradient->transform, sizeof(gradient->transform));
            cursor += sizeof(gradient->transform);
        }

        if (gradient->pblendcount > 1 && gradient->pblendcolor && gradient->pblendpos)
        {
            const DWORD count = gradient->pblendcount;

            data->BrushData.lineargradient.BrushDataFlags |= BrushDataPresetColors;

            memcpy(cursor, &count, sizeof(count));
            cursor += sizeof(count);

            memcpy(cursor, gradient->pblendpos, count * sizeof(*gradient->pblendpos));
            cursor += count * sizeof(*gradient->pblendpos);

            memcpy(cursor, gradient->pblendcolor, count * sizeof(*gradient->pblendcolor));
        }
        else if (gradient->blendcount > 1 && gradient->blendfac && gradient->blendpos)
        {
            const DWORD count = gradient->blendcount;

            data->BrushData.lineargradient.BrushDataFlags |= BrushDataBlendFactorsH;

            memcpy(cursor, &count, sizeof(count));
            cursor += sizeof(count);

            memcpy(cursor, gradient->blendpos, count * sizeof(*gradient->blendpos));
            cursor += count * sizeof(*gradient->blendpos);

            memcpy(cursor, gradient->blendfac, count * sizeof(*gradient->blendfac));
        }

        break;
    }
    default:
        FIXME("unsupported brush type: %d\n", brush->bt);
    }
}

static void METAFILE_PrepareCustomLineCapData(GDIPCONST GpCustomLineCap *cap, DWORD *ret_cap_size,
                                              DWORD *ret_cap_data_size, DWORD *ret_path_size)
{
    DWORD cap_size, path_size = 0;

    /* EmfPlusCustomStartCapData */
    cap_size = FIELD_OFFSET(EmfPlusCustomStartCapData, data);
    /*   -> EmfPlusCustomLineCap */
    cap_size += FIELD_OFFSET(EmfPlusCustomLineCap, CustomLineCapData);
    /*      -> EmfPlusCustomLineCapArrowData */
    if (cap->type == CustomLineCapTypeAdjustableArrow)
        cap_size += sizeof(EmfPlusCustomLineCapArrowData);
    /*      -> EmfPlusCustomLineCapData */
    else
    {
        /*     -> EmfPlusCustomLineCapOptionalData */
        cap_size += FIELD_OFFSET(EmfPlusCustomLineCapData, OptionalData);
        if (cap->fill)
            /*    -> EmfPlusCustomLineCapDataFillPath */
            cap_size += FIELD_OFFSET(EmfPlusCustomLineCapDataFillPath, FillPath);
        else
            /*    -> EmfPlusCustomLineCapDataLinePath */
            cap_size += FIELD_OFFSET(EmfPlusCustomLineCapDataLinePath, LinePath);

        /*           -> EmfPlusPath in EmfPlusCustomLineCapDataFillPath and EmfPlusCustomLineCapDataLinePath */
        path_size = FIELD_OFFSET(EmfPlusPath, data);
        path_size += sizeof(PointF) * cap->pathdata.Count;
        path_size += sizeof(BYTE) * cap->pathdata.Count;
        path_size = (path_size + 3) & ~3;

        cap_size += path_size;
    }

    *ret_cap_size = cap_size;
    *ret_cap_data_size = cap_size - FIELD_OFFSET(EmfPlusCustomStartCapData, data);
    *ret_path_size = path_size;
}

static void METAFILE_FillCustomLineCapData(GDIPCONST GpCustomLineCap *cap, BYTE *ptr,
                                           REAL line_miter_limit, DWORD data_size, DWORD path_size)
{
    EmfPlusCustomStartCapData *cap_data;
    EmfPlusCustomLineCap *line_cap;
    DWORD i;

    cap_data = (EmfPlusCustomStartCapData *)ptr;
    cap_data->CustomStartCapSize = data_size;
    i = FIELD_OFFSET(EmfPlusCustomStartCapData, data);

    line_cap = (EmfPlusCustomLineCap *)(ptr + i);
    line_cap->Version = VERSION_MAGIC2;
    line_cap->Type = cap->type;
    i += FIELD_OFFSET(EmfPlusCustomLineCap, CustomLineCapData);

    if (cap->type == CustomLineCapTypeAdjustableArrow)
    {
        EmfPlusCustomLineCapArrowData *arrow_data;
        GpAdjustableArrowCap *arrow_cap;

        arrow_data = (EmfPlusCustomLineCapArrowData *)(ptr + i);
        arrow_cap = (GpAdjustableArrowCap *)cap;
        arrow_data->Width = arrow_cap->width;
        arrow_data->Height = arrow_cap->height;
        arrow_data->MiddleInset = arrow_cap->middle_inset;
        arrow_data->FillState = arrow_cap->cap.fill;
        arrow_data->LineStartCap = arrow_cap->cap.strokeStartCap;
        arrow_data->LineEndCap = arrow_cap->cap.strokeEndCap;
        arrow_data->LineJoin = arrow_cap->cap.join;
        arrow_data->LineMiterLimit = line_miter_limit;
        arrow_data->WidthScale = arrow_cap->cap.scale;
        arrow_data->FillHotSpot.X = 0;
        arrow_data->FillHotSpot.Y = 0;
        arrow_data->LineHotSpot.X = 0;
        arrow_data->LineHotSpot.Y = 0;
    }
    else
    {
        EmfPlusCustomLineCapData *line_cap_data = (EmfPlusCustomLineCapData *)(ptr + i);
        EmfPlusPath *path;

        if (cap->fill)
            line_cap_data->CustomLineCapDataFlags = CustomLineCapDataFillPath;
        else
            line_cap_data->CustomLineCapDataFlags = CustomLineCapDataLinePath;
        line_cap_data->BaseCap = cap->basecap;
        line_cap_data->BaseInset = cap->inset;
        line_cap_data->StrokeStartCap = cap->strokeStartCap;
        line_cap_data->StrokeEndCap = cap->strokeEndCap;
        line_cap_data->StrokeJoin = cap->join;
        line_cap_data->StrokeMiterLimit = line_miter_limit;
        line_cap_data->WidthScale = cap->scale;
        line_cap_data->FillHotSpot.X = 0;
        line_cap_data->FillHotSpot.Y = 0;
        line_cap_data->LineHotSpot.X = 0;
        line_cap_data->LineHotSpot.Y = 0;
        i += FIELD_OFFSET(EmfPlusCustomLineCapData, OptionalData);

        if (cap->fill)
        {
            EmfPlusCustomLineCapDataFillPath *fill_path = (EmfPlusCustomLineCapDataFillPath *)(ptr + i);
            fill_path->FillPathLength = path_size;
            i += FIELD_OFFSET(EmfPlusCustomLineCapDataFillPath, FillPath);
        }
        else
        {
            EmfPlusCustomLineCapDataLinePath *line_path = (EmfPlusCustomLineCapDataLinePath *)(ptr + i);
            line_path->LinePathLength = path_size;
            i += FIELD_OFFSET(EmfPlusCustomLineCapDataLinePath, LinePath);
        }

        path = (EmfPlusPath *)(ptr + i);
        path->Version = VERSION_MAGIC2;
        path->PathPointCount = cap->pathdata.Count;
        path->PathPointFlags = 0;
        i += FIELD_OFFSET(EmfPlusPath, data);
        memcpy(ptr + i, cap->pathdata.Points, cap->pathdata.Count * sizeof(PointF));
        i += cap->pathdata.Count * sizeof(PointF);
        memcpy(ptr + i, cap->pathdata.Types, cap->pathdata.Count * sizeof(BYTE));
    }
}

static GpStatus METAFILE_AddBrushObject(GpMetafile *metafile, GDIPCONST GpBrush *brush, DWORD *id)
{
    EmfPlusObject *object_record;
    GpStatus stat;
    DWORD size;

    *id = -1;
    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    stat = METAFILE_PrepareBrushData(brush, &size);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
        FIELD_OFFSET(EmfPlusObject, ObjectData) + size, (void**)&object_record);
    if (stat != Ok) return stat;

    *id = METAFILE_AddObjectId(metafile);
    object_record->Header.Flags = *id | ObjectTypeBrush << 8;
    METAFILE_FillBrushData(brush, &object_record->ObjectData.brush);
    return Ok;
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
            stat = METAFILE_AddBrushObject(metafile, brush, &brushid);
            if (stat != Ok)
                return stat;
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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeFillRects,
            sizeof(EmfPlusFillRects) + count * (integer_rects ? sizeof(EmfPlusRect) : sizeof(GpRectF)),
            (void**)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetClipRect,
                sizeof(EmfPlusSetClipRect), (void **)&record);
        if (stat != Ok)
            return stat;

        record->Header.Flags = (mode & 0xf) << 8;
        record->ClipRect.X = x;
        record->ClipRect.Y = y;
        record->ClipRect.Width = width;
        record->ClipRect.Height = height;

        METAFILE_WriteRecords(metafile);
    }

    return Ok;
}

static GpStatus METAFILE_AddRegionObject(GpMetafile *metafile, GpRegion *region, DWORD *id)
{
    EmfPlusObject *object_record;
    DWORD size;
    GpStatus stat;

    *id = -1;
    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    size = write_region_data(region, NULL);
    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
            FIELD_OFFSET(EmfPlusObject, ObjectData.region) + size, (void**)&object_record);
    if (stat != Ok) return stat;

    *id = METAFILE_AddObjectId(metafile);
    object_record->Header.Flags = *id | ObjectTypeRegion << 8;
    write_region_data(region, &object_record->ObjectData.region);
    return Ok;
}

GpStatus METAFILE_SetClipRegion(GpMetafile* metafile, GpRegion* region, CombineMode mode)
{
    EmfPlusRecordHeader *record;
    DWORD region_id;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddRegionObject(metafile, region, &region_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetClipRegion, sizeof(*record), (void**)&record);
    if (stat != Ok) return stat;

    record->Flags = region_id | mode << 8;

    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_SetPageTransform(GpMetafile* metafile, GpUnit unit, REAL scale)
{
    if (metafile->metafile_type == MetafileTypeEmfPlusOnly || metafile->metafile_type == MetafileTypeEmfPlusDual)
    {
        EmfPlusSetPageTransform *record;
        GpStatus stat;

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetPageTransform,
            sizeof(EmfPlusSetPageTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetWorldTransform,
            sizeof(EmfPlusSetWorldTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeScaleWorldTransform,
            sizeof(EmfPlusScaleWorldTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeMultiplyWorldTransform,
            sizeof(EmfPlusMultiplyWorldTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeRotateWorldTransform,
            sizeof(EmfPlusRotateWorldTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeTranslateWorldTransform,
            sizeof(EmfPlusTranslateWorldTransform), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeResetWorldTransform,
            sizeof(EmfPlusRecordHeader), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeBeginContainer, sizeof(*record), (void**)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeBeginContainerNoParams,
            sizeof(EmfPlusContainerRecord), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeEndContainer,
            sizeof(EmfPlusContainerRecord), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSave,
            sizeof(EmfPlusContainerRecord), (void **)&record);
        if (stat != Ok)
            return stat;

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

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeRestore,
            sizeof(EmfPlusContainerRecord), (void **)&record);
        if (stat != Ok)
            return stat;

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

    free(metafile->comment_data);
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

            gdi_bounds_rc = header.EmfHeader.rclBounds;
            if (gdi_bounds_rc.right > gdi_bounds_rc.left &&
                gdi_bounds_rc.bottom > gdi_bounds_rc.top)
            {
                GpPointF *af_min = &metafile->auto_frame_min;
                GpPointF *af_max = &metafile->auto_frame_max;

                af_min->X = fmin(af_min->X, gdi_bounds_rc.left);
                af_min->Y = fmin(af_min->Y, gdi_bounds_rc.top);
                af_max->X = fmax(af_max->X, gdi_bounds_rc.right);
                af_max->Y = fmax(af_max->Y, gdi_bounds_rc.bottom);
            }

            bounds_rc.left = floorf(metafile->auto_frame_min.X * x_scale);
            bounds_rc.top = floorf(metafile->auto_frame_min.Y * y_scale);
            bounds_rc.right = ceilf(metafile->auto_frame_max.X * x_scale);
            bounds_rc.bottom = ceilf(metafile->auto_frame_max.Y * y_scale);

            buffer_size = GetEnhMetaFileBits(metafile->hemf, 0, NULL);
            buffer = malloc(buffer_size);
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

                free(buffer);
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

        buffer = malloc(buffer_size);
        if (buffer)
        {
            HRESULT hr;

            GetEnhMetaFileBits(metafile->hemf, buffer_size, buffer);

            hr = IStream_Write(metafile->record_stream, buffer, buffer_size, NULL);

            if (FAILED(hr))
                stat = hresult_to_status(hr);

            free(buffer);
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

static GpStatus METAFILE_PlaybackGetDC(GpMetafile *metafile)
{
    GpStatus stat = Ok;

    stat = GdipGetDC(metafile->playback_graphics, &metafile->playback_dc);

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
        REAL scale_x = units_to_pixels(1.0, metafile->page_unit, metafile->logical_dpix, metafile->printer_display);
        REAL scale_y = units_to_pixels(1.0, metafile->page_unit, metafile->logical_dpiy, metafile->printer_display);

        if (metafile->page_unit != UnitDisplay)
        {
            scale_x *= metafile->page_scale;
            scale_y *= metafile->page_scale;
        }

        stat = GdipScaleMatrix(real_transform, scale_x, scale_y, MatrixOrderPrepend);

        if (stat == Ok)
            stat = GdipMultiplyMatrix(real_transform, metafile->world_transform, MatrixOrderPrepend);

        if (stat == Ok)
            stat = GdipSetWorldTransform(metafile->playback_graphics, real_transform);

        GdipDeleteMatrix(real_transform);
    }

    return stat;
}

static void metafile_set_object_table_entry(GpMetafile *metafile, BYTE id, BYTE type, void *object)
{
    metafile_free_object_table_entry(metafile, id);
    metafile->objtable[id].type = type;
    metafile->objtable[id].u.object = object;
}

static GpStatus metafile_deserialize_image(const BYTE *record_data, UINT data_size, GpImage **image)
{
    EmfPlusImage *data = (EmfPlusImage *)record_data;
    GpStatus status;

    *image = NULL;

    if (data_size < FIELD_OFFSET(EmfPlusImage, ImageData))
        return InvalidParameter;
    data_size -= FIELD_OFFSET(EmfPlusImage, ImageData);

    switch (data->Type)
    {
    case ImageDataTypeBitmap:
    {
        EmfPlusBitmap *bitmapdata = &data->ImageData.bitmap;

        if (data_size <= FIELD_OFFSET(EmfPlusBitmap, BitmapData))
            return InvalidParameter;
        data_size -= FIELD_OFFSET(EmfPlusBitmap, BitmapData);

        switch (bitmapdata->Type)
        {
        case BitmapDataTypePixel:
        {
            ColorPalette *palette;
            BYTE *scan0;

            if (bitmapdata->PixelFormat & PixelFormatIndexed)
            {
                EmfPlusPalette *palette_obj = (EmfPlusPalette *)bitmapdata->BitmapData;
                UINT palette_size = FIELD_OFFSET(EmfPlusPalette, PaletteEntries);

                if (data_size <= palette_size)
                    return InvalidParameter;
                palette_size += palette_obj->PaletteCount * sizeof(EmfPlusARGB);

                if (data_size < palette_size)
                    return InvalidParameter;
                data_size -= palette_size;

                palette = (ColorPalette *)bitmapdata->BitmapData;
                scan0 = (BYTE *)bitmapdata->BitmapData + palette_size;
            }
            else
            {
                palette = NULL;
                scan0 = bitmapdata->BitmapData;
            }

            if (data_size < bitmapdata->Height * bitmapdata->Stride)
                return InvalidParameter;

            status = GdipCreateBitmapFromScan0(bitmapdata->Width, bitmapdata->Height, bitmapdata->Stride,
                bitmapdata->PixelFormat, scan0, (GpBitmap **)image);
            if (status == Ok && palette)
            {
                status = GdipSetImagePalette(*image, palette);
                if (status != Ok)
                {
                    GdipDisposeImage(*image);
                    *image = NULL;
                }
            }
            break;
        }
        case BitmapDataTypeCompressed:
        {
            IWICImagingFactory *factory;
            IWICStream *stream;
            HRESULT hr;

            if (WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory) != S_OK)
                return GenericError;

            hr = IWICImagingFactory_CreateStream(factory, &stream);
            IWICImagingFactory_Release(factory);
            if (hr != S_OK)
                return GenericError;

            if (IWICStream_InitializeFromMemory(stream, bitmapdata->BitmapData, data_size) == S_OK)
                status = GdipCreateBitmapFromStream((IStream *)stream, (GpBitmap **)image);
            else
                status = GenericError;

            IWICStream_Release(stream);
            break;
        }
        default:
            WARN("Invalid bitmap type %ld.\n", bitmapdata->Type);
            return InvalidParameter;
        }
        break;
    }
    case ImageDataTypeMetafile:
    {
        EmfPlusMetafile *metafiledata = &data->ImageData.metafile;

        if (data_size <= FIELD_OFFSET(EmfPlusMetafile, MetafileData))
            return InvalidParameter;
        data_size -= FIELD_OFFSET(EmfPlusMetafile, MetafileData);

        switch (metafiledata->Type) {
        case MetafileTypeEmf:
        case MetafileTypeEmfPlusOnly:
        case MetafileTypeEmfPlusDual:
        {
            HENHMETAFILE hemf;

            hemf = SetEnhMetaFileBits(data_size, metafiledata->MetafileData);

            if (!hemf)
                return GenericError;

            status = GdipCreateMetafileFromEmf(hemf, TRUE, (GpMetafile**)image);

            if (status != Ok)
                DeleteEnhMetaFile(hemf);

            break;
        }
        default:
            FIXME("metafile type %ld not supported.\n", metafiledata->Type);
            return NotImplemented;
        }
        break;
    }
    default:
        FIXME("image type %d not supported.\n", data->Type);
        return NotImplemented;
    }

    return status;
}

static GpStatus metafile_deserialize_path(const BYTE *record_data, UINT data_size, GpPath **path)
{
    EmfPlusPath *data = (EmfPlusPath *)record_data;
    BYTE *types;
    UINT size;
    DWORD i;

    *path = NULL;

    if (data_size <= FIELD_OFFSET(EmfPlusPath, data))
        return InvalidParameter;
    data_size -= FIELD_OFFSET(EmfPlusPath, data);

    if (data->PathPointFlags & 0x800) /* R */
    {
        FIXME("RLE encoded path data is not supported.\n");
        return NotImplemented;
    }
    else
    {
        if (data->PathPointFlags & 0x4000) /* C */
            size = sizeof(EmfPlusPoint);
        else
            size = sizeof(EmfPlusPointF);
        size += sizeof(BYTE); /* EmfPlusPathPointType */
        size *= data->PathPointCount;
    }

    if (data_size < size)
        return InvalidParameter;

    if (data->PathPointCount)
    {
        if (data->PathPointFlags & 0x4000) /* C */
        {
            EmfPlusPoint *points = (EmfPlusPoint *)data->data;
            GpPointF *temp = malloc(sizeof(GpPointF) * data->PathPointCount);

            for (i = 0; i < data->PathPointCount; i++)
            {
                temp[i].X = points[i].X;
                temp[i].Y = points[i].Y;
            }

            types = (BYTE *)(points + i);
            GdipCreatePath2(temp, types, data->PathPointCount, FillModeAlternate, path);
            free(temp);
        }
        else
        {
            EmfPlusPointF *points = (EmfPlusPointF *)data->data;
            types = (BYTE *)(points + data->PathPointCount);
            return GdipCreatePath2((GpPointF*)points, types, data->PathPointCount, FillModeAlternate, path);
        }
    }
    else
    {
        return GdipCreatePath(FillModeAlternate, path);
    }

    return Ok;
}

static GpStatus metafile_read_region_node(struct memory_buffer *mbuf, GpRegion *region, region_element *node, UINT *count)
{
    const DWORD *type;
    GpStatus status;

    type = buffer_read(mbuf, sizeof(*type));
    if (!type) return Ok;

    node->type = *type;

    switch (node->type)
    {
    case CombineModeReplace:
    case CombineModeIntersect:
    case CombineModeUnion:
    case CombineModeXor:
    case CombineModeExclude:
    case CombineModeComplement:
    {
        region_element *left, *right;

        left = calloc(1, sizeof(*left));
        if (!left)
            return OutOfMemory;

        right = calloc(1, sizeof(*right));
        if (!right)
        {
            free(left);
            return OutOfMemory;
        }

        status = metafile_read_region_node(mbuf, region, left, count);
        if (status == Ok)
        {
            status = metafile_read_region_node(mbuf, region, right, count);
            if (status == Ok)
            {
                node->elementdata.combine.left = left;
                node->elementdata.combine.right = right;
                region->num_children += 2;
                return Ok;
            }
        }

        free(left);
        free(right);
        return status;
    }
    case RegionDataRect:
    {
        const EmfPlusRectF *rect;

        rect = buffer_read(mbuf, sizeof(*rect));
        if (!rect)
            return InvalidParameter;

        memcpy(&node->elementdata.rect, rect, sizeof(*rect));
        *count += 1;
        return Ok;
    }
    case RegionDataPath:
    {
        const BYTE *path_data;
        const UINT *data_size;
        GpPath *path;

        data_size = buffer_read(mbuf, FIELD_OFFSET(EmfPlusRegionNodePath, RegionNodePath));
        if (!data_size)
            return InvalidParameter;

        path_data = buffer_read(mbuf, *data_size);
        if (!path_data)
            return InvalidParameter;

        status = metafile_deserialize_path(path_data, *data_size, &path);
        if (status == Ok)
        {
            node->elementdata.path = path;
            *count += 1;
        }
        return Ok;
    }
    case RegionDataEmptyRect:
    case RegionDataInfiniteRect:
        *count += 1;
        return Ok;
    default:
        FIXME("element type %#lx is not supported\n", *type);
        break;
    }

    return InvalidParameter;
}

static GpStatus metafile_deserialize_region(const BYTE *record_data, UINT data_size, GpRegion **region)
{
    struct memory_buffer mbuf;
    GpStatus status;
    UINT count;

    *region = NULL;

    init_memory_buffer(&mbuf, record_data, data_size);

    if (!buffer_read(&mbuf, FIELD_OFFSET(EmfPlusRegion, RegionNode)))
        return InvalidParameter;

    status = GdipCreateRegion(region);
    if (status != Ok)
        return status;

    count = 0;
    status = metafile_read_region_node(&mbuf, *region, &(*region)->node, &count);
    if (status == Ok && !count)
        status = InvalidParameter;

    if (status != Ok)
    {
        GdipDeleteRegion(*region);
        *region = NULL;
    }

    return status;
}

static GpStatus metafile_deserialize_brush(const BYTE *record_data, UINT data_size, GpBrush **brush)
{
    static const UINT header_size = FIELD_OFFSET(EmfPlusBrush, BrushData);
    EmfPlusBrush *data = (EmfPlusBrush *)record_data;
    EmfPlusTransformMatrix *transform = NULL;
    DWORD brushflags;
    GpStatus status;
    UINT offset;

    *brush = NULL;

    if (data_size < header_size)
        return InvalidParameter;

    switch (data->Type)
    {
    case BrushTypeSolidColor:
        if (data_size != header_size + sizeof(EmfPlusSolidBrushData))
            return InvalidParameter;

        status = GdipCreateSolidFill(data->BrushData.solid.SolidColor, (GpSolidFill **)brush);
        break;
    case BrushTypeHatchFill:
        if (data_size != header_size + sizeof(EmfPlusHatchBrushData))
            return InvalidParameter;

        status = GdipCreateHatchBrush(data->BrushData.hatch.HatchStyle, data->BrushData.hatch.ForeColor,
            data->BrushData.hatch.BackColor, (GpHatch **)brush);
        break;
    case BrushTypeTextureFill:
    {
        GpImage *image;

        offset = header_size + FIELD_OFFSET(EmfPlusTextureBrushData, OptionalData);
        if (data_size <= offset)
            return InvalidParameter;

        brushflags = data->BrushData.texture.BrushDataFlags;
        if (brushflags & BrushDataTransform)
        {
            if (data_size <= offset + sizeof(EmfPlusTransformMatrix))
                return InvalidParameter;
            transform = (EmfPlusTransformMatrix *)(record_data + offset);
            offset += sizeof(EmfPlusTransformMatrix);
        }

        status = metafile_deserialize_image(record_data + offset, data_size - offset, &image);
        if (status != Ok)
            return status;

        status = GdipCreateTexture(image, data->BrushData.texture.WrapMode, (GpTexture **)brush);
        if (status == Ok && transform && !(brushflags & BrushDataDoNotTransform))
            GdipSetTextureTransform((GpTexture *)*brush, (const GpMatrix *)transform);

        GdipDisposeImage(image);
        break;
    }
    case BrushTypeLinearGradient:
    {
        GpLineGradient *gradient = NULL;
        GpRectF rect;
        UINT position_count = 0;

        offset = header_size + FIELD_OFFSET(EmfPlusLinearGradientBrushData, OptionalData);
        if (data_size < offset)
            return InvalidParameter;

        brushflags = data->BrushData.lineargradient.BrushDataFlags;
        if ((brushflags & BrushDataPresetColors) && (brushflags & (BrushDataBlendFactorsH | BrushDataBlendFactorsV)))
            return InvalidParameter;

        if (brushflags & BrushDataTransform)
        {
            if (data_size < offset + sizeof(EmfPlusTransformMatrix))
                return InvalidParameter;
            transform = (EmfPlusTransformMatrix *)(record_data + offset);
            offset += sizeof(EmfPlusTransformMatrix);
        }

        if (brushflags & (BrushDataPresetColors | BrushDataBlendFactorsH | BrushDataBlendFactorsV))
        {
            if (data_size <= offset + sizeof(DWORD)) /* Number of factors/preset colors. */
                return InvalidParameter;
            position_count = *(DWORD *)(record_data + offset);
            offset += sizeof(DWORD);
        }

        if (brushflags & BrushDataPresetColors)
        {
            if (data_size != offset + position_count * (sizeof(float) + sizeof(EmfPlusARGB)))
                return InvalidParameter;
        }
        else if (brushflags & BrushDataBlendFactorsH)
        {
            if (data_size != offset + position_count * 2 * sizeof(float))
                return InvalidParameter;
        }

        rect.X = data->BrushData.lineargradient.RectF.X;
        rect.Y = data->BrushData.lineargradient.RectF.Y;
        rect.Width = data->BrushData.lineargradient.RectF.Width;
        rect.Height = data->BrushData.lineargradient.RectF.Height;

        status = GdipCreateLineBrushFromRect(&rect, data->BrushData.lineargradient.StartColor,
            data->BrushData.lineargradient.EndColor, LinearGradientModeHorizontal,
            data->BrushData.lineargradient.WrapMode, &gradient);
        if (status == Ok)
        {
            if (transform)
                status = GdipSetLineTransform(gradient, (const GpMatrix *)transform);

            if (status == Ok)
            {
                if (brushflags & BrushDataPresetColors)
                    status = GdipSetLinePresetBlend(gradient, (ARGB *)(record_data + offset +
                        position_count * sizeof(REAL)), (REAL *)(record_data + offset), position_count);
                else if (brushflags & BrushDataBlendFactorsH)
                    status = GdipSetLineBlend(gradient, (REAL *)(record_data + offset + position_count * sizeof(REAL)),
                        (REAL *)(record_data + offset), position_count);

                if (brushflags & BrushDataIsGammaCorrected)
                    FIXME("BrushDataIsGammaCorrected is not handled.\n");
            }
        }

        if (status == Ok)
            *brush = (GpBrush *)gradient;
        else
            GdipDeleteBrush((GpBrush *)gradient);

        break;
    }
    default:
        FIXME("brush type %lu is not supported.\n", data->Type);
        return NotImplemented;
    }

    return status;
}

static GpStatus metafile_deserialize_custom_line_cap(const BYTE *record_data, UINT data_size, GpCustomLineCap **cap)
{
    EmfPlusCustomStartCapData *custom_cap_data = (EmfPlusCustomStartCapData *)record_data;
    EmfPlusCustomLineCap *line_cap;
    GpStatus status;
    UINT offset;

    *cap = NULL;

    if (data_size < FIELD_OFFSET(EmfPlusCustomStartCapData, data))
        return InvalidParameter;
    if (data_size < FIELD_OFFSET(EmfPlusCustomStartCapData, data) + custom_cap_data->CustomStartCapSize)
        return InvalidParameter;
    offset = FIELD_OFFSET(EmfPlusCustomStartCapData, data);
    line_cap = (EmfPlusCustomLineCap *)(record_data + offset);

    if (data_size < offset + FIELD_OFFSET(EmfPlusCustomLineCap, CustomLineCapData))
        return InvalidParameter;
    offset += FIELD_OFFSET(EmfPlusCustomLineCap, CustomLineCapData);

    if (line_cap->Type == CustomLineCapTypeAdjustableArrow)
    {
        EmfPlusCustomLineCapArrowData *arrow_data;
        GpAdjustableArrowCap *arrow_cap;

        arrow_data = (EmfPlusCustomLineCapArrowData *)(record_data + offset);

        if (data_size < offset + sizeof(EmfPlusCustomLineCapArrowData))
            return InvalidParameter;

        if ((status = GdipCreateAdjustableArrowCap(arrow_data->Height, arrow_data->Width,
                                                   arrow_data->FillState, &arrow_cap)))
            return status;

        if ((status = GdipSetAdjustableArrowCapMiddleInset(arrow_cap, arrow_data->MiddleInset)))
            goto arrow_cap_failed;
        if ((status = GdipSetCustomLineCapStrokeCaps((GpCustomLineCap *)arrow_cap, arrow_data->LineStartCap, arrow_data->LineEndCap)))
            goto arrow_cap_failed;
        if ((status = GdipSetCustomLineCapStrokeJoin((GpCustomLineCap *)arrow_cap, arrow_data->LineJoin)))
            goto arrow_cap_failed;
        if ((status = GdipSetCustomLineCapWidthScale((GpCustomLineCap *)arrow_cap, arrow_data->WidthScale)))
            goto arrow_cap_failed;

        *cap = (GpCustomLineCap *)arrow_cap;
        return Ok;

    arrow_cap_failed:
        GdipDeleteCustomLineCap((GpCustomLineCap *)arrow_cap);
        return status;
    }
    else
    {
        GpPath *path, *fill_path = NULL, *stroke_path = NULL;
        EmfPlusCustomLineCapData *line_cap_data;
        GpCustomLineCap *line_cap = NULL;
        GpStatus status;

        line_cap_data = (EmfPlusCustomLineCapData *)(record_data + offset);

        if (data_size < offset + FIELD_OFFSET(EmfPlusCustomLineCapData, OptionalData))
            return InvalidParameter;
        offset += FIELD_OFFSET(EmfPlusCustomLineCapData, OptionalData);

        if (line_cap_data->CustomLineCapDataFlags == CustomLineCapDataFillPath)
        {
            EmfPlusCustomLineCapDataFillPath *fill_path = (EmfPlusCustomLineCapDataFillPath *)(record_data + offset);

            if (data_size < offset + FIELD_OFFSET(EmfPlusCustomLineCapDataFillPath, FillPath))
                return InvalidParameter;
            if (data_size < offset + fill_path->FillPathLength)
                return InvalidParameter;

            offset += FIELD_OFFSET(EmfPlusCustomLineCapDataFillPath, FillPath);
        }
        else
        {
            EmfPlusCustomLineCapDataLinePath *line_path = (EmfPlusCustomLineCapDataLinePath *)(record_data + offset);

            if (data_size < offset + FIELD_OFFSET(EmfPlusCustomLineCapDataLinePath, LinePath))
                return InvalidParameter;
            if (data_size < offset + line_path->LinePathLength)
                return InvalidParameter;

            offset += FIELD_OFFSET(EmfPlusCustomLineCapDataLinePath, LinePath);
        }

        if ((status = metafile_deserialize_path(record_data + offset, data_size - offset, &path)))
            return status;

        if (line_cap_data->CustomLineCapDataFlags == CustomLineCapDataFillPath)
            fill_path = path;
        else
            stroke_path = path;

        if ((status = GdipCreateCustomLineCap(fill_path, stroke_path, line_cap_data->BaseCap,
                                              line_cap_data->BaseInset, &line_cap)))
            goto default_cap_failed;
        if ((status = GdipSetCustomLineCapStrokeCaps(line_cap, line_cap_data->StrokeStartCap, line_cap_data->StrokeEndCap)))
            goto default_cap_failed;
        if ((status = GdipSetCustomLineCapStrokeJoin(line_cap, line_cap_data->StrokeJoin)))
            goto default_cap_failed;
        if ((status = GdipSetCustomLineCapWidthScale(line_cap, line_cap_data->WidthScale)))
            goto default_cap_failed;

        GdipDeletePath(path);
        *cap = line_cap;
        return Ok;

    default_cap_failed:
        if (line_cap)
            GdipDeleteCustomLineCap(line_cap);
        GdipDeletePath(path);
        return status;
    }
}

static GpStatus metafile_get_pen_brush_data_offset(EmfPlusPen *data, UINT data_size, DWORD *ret)
{
    EmfPlusPenData *pendata = (EmfPlusPenData *)data->data;
    DWORD offset = FIELD_OFFSET(EmfPlusPen, data);

    if (data_size <= offset)
        return InvalidParameter;

    offset += FIELD_OFFSET(EmfPlusPenData, OptionalData);
    if (data_size <= offset)
        return InvalidParameter;

    if (pendata->PenDataFlags & PenDataTransform)
        offset += sizeof(EmfPlusTransformMatrix);

    if (pendata->PenDataFlags & PenDataStartCap)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataEndCap)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataJoin)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataMiterLimit)
        offset += sizeof(REAL);

    if (pendata->PenDataFlags & PenDataLineStyle)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataDashedLineCap)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataDashedLineOffset)
        offset += sizeof(REAL);

    if (pendata->PenDataFlags & PenDataDashedLine)
    {
        EmfPlusDashedLineData *dashedline = (EmfPlusDashedLineData *)((BYTE *)data + offset);

        offset += FIELD_OFFSET(EmfPlusDashedLineData, data);
        if (data_size <= offset)
            return InvalidParameter;

        offset += dashedline->DashedLineDataSize * sizeof(float);
    }

    if (pendata->PenDataFlags & PenDataNonCenter)
        offset += sizeof(DWORD);

    if (pendata->PenDataFlags & PenDataCompoundLine)
    {
        EmfPlusCompoundLineData *compoundline = (EmfPlusCompoundLineData *)((BYTE *)data + offset);

        offset += FIELD_OFFSET(EmfPlusCompoundLineData, data);
        if (data_size <= offset)
            return InvalidParameter;

        offset += compoundline->CompoundLineDataSize * sizeof(float);
    }

    if (pendata->PenDataFlags & PenDataCustomStartCap)
    {
        EmfPlusCustomStartCapData *startcap = (EmfPlusCustomStartCapData *)((BYTE *)data + offset);

        offset += FIELD_OFFSET(EmfPlusCustomStartCapData, data);
        if (data_size <= offset)
            return InvalidParameter;

        offset += startcap->CustomStartCapSize;
    }

    if (pendata->PenDataFlags & PenDataCustomEndCap)
    {
        EmfPlusCustomEndCapData *endcap = (EmfPlusCustomEndCapData *)((BYTE *)data + offset);

        offset += FIELD_OFFSET(EmfPlusCustomEndCapData, data);
        if (data_size <= offset)
            return InvalidParameter;

        offset += endcap->CustomEndCapSize;
    }

    *ret = offset;
    return Ok;
}

static GpStatus METAFILE_PlaybackObject(GpMetafile *metafile, UINT flags, UINT data_size, const BYTE *record_data)
{
    BYTE type = (flags >> 8) & 0xff;
    BYTE id = flags & 0xff;
    void *object = NULL;
    GpStatus status;

    if (type > ObjectTypeMax || id >= EmfPlusObjectTableSize)
        return InvalidParameter;

    switch (type)
    {
    case ObjectTypeBrush:
        status = metafile_deserialize_brush(record_data, data_size, (GpBrush **)&object);
        break;
    case ObjectTypePen:
    {
        EmfPlusPen *data = (EmfPlusPen *)record_data;
        EmfPlusPenData *pendata = (EmfPlusPenData *)data->data;
        GpCustomLineCap *custom_line_cap;
        GpBrush *brush;
        DWORD offset;
        GpPen *pen;

        status = metafile_get_pen_brush_data_offset(data, data_size, &offset);
        if (status != Ok)
            return status;

        status = metafile_deserialize_brush(record_data + offset, data_size - offset, &brush);
        if (status != Ok)
            return status;

        status = GdipCreatePen2(brush, pendata->PenWidth, pendata->PenUnit, &pen);
        GdipDeleteBrush(brush);
        if (status != Ok)
            return status;

        offset = FIELD_OFFSET(EmfPlusPenData, OptionalData);

        if (pendata->PenDataFlags & PenDataTransform)
        {
            FIXME("PenDataTransform is not supported.\n");
            offset += sizeof(EmfPlusTransformMatrix);
        }

        if (pendata->PenDataFlags & PenDataStartCap)
        {
            if ((status = GdipSetPenStartCap(pen, *(DWORD *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataEndCap)
        {
            if ((status = GdipSetPenEndCap(pen, *(DWORD *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataJoin)
        {
            if ((status = GdipSetPenLineJoin(pen, *(DWORD *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataMiterLimit)
        {
            if ((status = GdipSetPenMiterLimit(pen, *(REAL *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(REAL);
        }

        if (pendata->PenDataFlags & PenDataLineStyle)
        {
            if ((status = GdipSetPenDashStyle(pen, *(DWORD *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataDashedLineCap)
        {
            FIXME("PenDataDashedLineCap is not supported.\n");
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataDashedLineOffset)
        {
            if ((status = GdipSetPenDashOffset(pen, *(REAL *)((BYTE *)pendata + offset))) != Ok)
                goto penfailed;
            offset += sizeof(REAL);
        }

        if (pendata->PenDataFlags & PenDataDashedLine)
        {
            EmfPlusDashedLineData *dashedline = (EmfPlusDashedLineData *)((BYTE *)pendata + offset);
            FIXME("PenDataDashedLine is not supported.\n");
            offset += FIELD_OFFSET(EmfPlusDashedLineData, data) + dashedline->DashedLineDataSize * sizeof(float);
        }

        if (pendata->PenDataFlags & PenDataNonCenter)
        {
            FIXME("PenDataNonCenter is not supported.\n");
            offset += sizeof(DWORD);
        }

        if (pendata->PenDataFlags & PenDataCompoundLine)
        {
            EmfPlusCompoundLineData *compoundline = (EmfPlusCompoundLineData *)((BYTE *)pendata + offset);
            FIXME("PenDataCompoundLine is not supported.\n");
            offset += FIELD_OFFSET(EmfPlusCompoundLineData, data) + compoundline->CompoundLineDataSize * sizeof(float);
        }

        if (pendata->PenDataFlags & PenDataCustomStartCap)
        {
            EmfPlusCustomStartCapData *startcap = (EmfPlusCustomStartCapData *)((BYTE *)pendata + offset);
            if ((status = metafile_deserialize_custom_line_cap((BYTE *)startcap, data_size, &custom_line_cap)) != Ok)
                goto penfailed;
            status = GdipSetPenCustomStartCap(pen, custom_line_cap);
            GdipDeleteCustomLineCap(custom_line_cap);
            if (status != Ok)
                goto penfailed;
            offset += FIELD_OFFSET(EmfPlusCustomStartCapData, data) + startcap->CustomStartCapSize;
        }

        if (pendata->PenDataFlags & PenDataCustomEndCap)
        {
            EmfPlusCustomEndCapData *endcap = (EmfPlusCustomEndCapData *)((BYTE *)pendata + offset);
            if ((status = metafile_deserialize_custom_line_cap((BYTE *)endcap, data_size, &custom_line_cap)) != Ok)
                goto penfailed;
            status = GdipSetPenCustomEndCap(pen, custom_line_cap);
            GdipDeleteCustomLineCap(custom_line_cap);
            if (status != Ok)
                goto penfailed;
            offset += FIELD_OFFSET(EmfPlusCustomEndCapData, data) + endcap->CustomEndCapSize;
        }

        object = pen;
        break;

    penfailed:
        GdipDeletePen(pen);
        return status;
    }
    case ObjectTypePath:
        status = metafile_deserialize_path(record_data, data_size, (GpPath **)&object);
        break;
    case ObjectTypeRegion:
        status = metafile_deserialize_region(record_data, data_size, (GpRegion **)&object);
        break;
    case ObjectTypeImage:
        status = metafile_deserialize_image(record_data, data_size, (GpImage **)&object);
        break;
    case ObjectTypeFont:
    {
        EmfPlusFont *data = (EmfPlusFont *)record_data;
        GpFontFamily *family;
        WCHAR *familyname;

        if (data_size <= FIELD_OFFSET(EmfPlusFont, FamilyName))
            return InvalidParameter;
        data_size -= FIELD_OFFSET(EmfPlusFont, FamilyName);

        if (data_size < data->Length * sizeof(WCHAR))
            return InvalidParameter;

        if (!(familyname = malloc((data->Length + 1) * sizeof(*familyname))))
            return OutOfMemory;

        memcpy(familyname, data->FamilyName, data->Length * sizeof(*familyname));
        familyname[data->Length] = 0;

        status = GdipCreateFontFamilyFromName(familyname, NULL, &family);
        free(familyname);

        /* If a font family cannot be created from family name, native
           falls back to a sans serif font. */
        if (status != Ok)
            status = GdipGetGenericFontFamilySansSerif(&family);
        if (status != Ok)
            return status;

        status = GdipCreateFont(family, data->EmSize, data->FontStyleFlags, data->SizeUnit, (GpFont **)&object);
        GdipDeleteFontFamily(family);
        break;
    }
    case ObjectTypeImageAttributes:
    {
        EmfPlusImageAttributes *data = (EmfPlusImageAttributes *)record_data;
        GpImageAttributes *attributes = NULL;

        if (data_size != sizeof(*data))
            return InvalidParameter;

        if ((status = GdipCreateImageAttributes(&attributes)) != Ok)
            return status;

        status = GdipSetImageAttributesWrapMode(attributes, data->WrapMode, *(DWORD *)&data->ClampColor,
                !!data->ObjectClamp);
        if (status == Ok)
            object = attributes;
        else
            GdipDisposeImageAttributes(attributes);
        break;
    }
    default:
        FIXME("not implemented for object type %d.\n", type);
        return NotImplemented;
    }

    if (status == Ok)
        metafile_set_object_table_entry(metafile, id, type, object);

    return status;
}

static GpStatus metafile_set_clip_region(GpMetafile *metafile, GpRegion *region, CombineMode mode)
{
    GpMatrix world_to_device;

    get_graphics_transform(metafile->playback_graphics, CoordinateSpaceDevice, CoordinateSpaceWorld, &world_to_device);

    GdipTransformRegion(region, &world_to_device);
    GdipCombineRegionRegion(metafile->clip, region, mode);

    return METAFILE_PlaybackUpdateClip(metafile);
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
            ENHMETARECORD *record = calloc(1, dataSize + 8);

            if (record)
            {
                record->iType = recordType;
                record->nSize = dataSize + 8;
                memcpy(record->dParm, data, dataSize);

                if (record->iType == EMR_BITBLT || record->iType == EMR_STRETCHBLT)
                    SetStretchBltMode(metafile->playback_dc, STRETCH_HALFTONE);

                if(PlayEnhMetaFileRecord(metafile->playback_dc, metafile->handle_table,
                        record, metafile->handle_count) == 0)
                    ERR("PlayEnhMetaFileRecord failed\n");

                free(record);
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

            if (dataSize != sizeof(record->Color))
                return InvalidParameter;

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
                stat = GdipCreateSolidFill(record->BrushID, (GpSolidFill **)&temp_brush);
                brush = temp_brush;
            }
            else
            {
                if (record->BrushID >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[record->BrushID].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[record->BrushID].u.brush;
                stat = Ok;
            }

            if (stat == Ok)
            {
                if (flags & 0x4000)
                {
                    EmfPlusRect *int_rects = (EmfPlusRect*)(record+1);
                    int i;

                    rects = temp_rects = calloc(record->Count, sizeof(GpRectF));
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
            free(temp_rects);

            return stat;
        }
        case EmfPlusRecordTypeSetClipRect:
        {
            EmfPlusSetClipRect *record = (EmfPlusSetClipRect*)header;
            CombineMode mode = (CombineMode)((flags >> 8) & 0xf);
            GpRegion *region;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(*record))
                return InvalidParameter;

            stat = GdipCreateRegionRect(&record->ClipRect, &region);

            if (stat == Ok)
            {
                stat = metafile_set_clip_region(real_metafile, region, mode);
                GdipDeleteRegion(region);
            }

            return stat;
        }
        case EmfPlusRecordTypeSetClipRegion:
        {
            CombineMode mode = (flags >> 8) & 0xf;
            BYTE regionid = flags & 0xff;
            GpRegion *region;

            if (dataSize != 0)
                return InvalidParameter;

            if (regionid >= EmfPlusObjectTableSize || real_metafile->objtable[regionid].type != ObjectTypeRegion)
                return InvalidParameter;

            stat = GdipCloneRegion(real_metafile->objtable[regionid].u.region, &region);
            if (stat == Ok)
            {
                stat = metafile_set_clip_region(real_metafile, region, mode);
                GdipDeleteRegion(region);
            }

            return stat;
        }
        case EmfPlusRecordTypeSetClipPath:
        {
            CombineMode mode = (flags >> 8) & 0xf;
            BYTE pathid = flags & 0xff;
            GpRegion *region;

            if (dataSize != 0)
                return InvalidParameter;

            if (pathid >= EmfPlusObjectTableSize || real_metafile->objtable[pathid].type != ObjectTypePath)
                return InvalidParameter;

            stat = GdipCreateRegionPath(real_metafile->objtable[pathid].u.path, &region);
            if (stat == Ok)
            {
                stat = metafile_set_clip_region(real_metafile, region, mode);
                GdipDeleteRegion(region);
            }

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

            cont = calloc(1, sizeof(*cont));
            if (!cont)
                return OutOfMemory;

            stat = GdipCloneRegion(metafile->clip, &cont->clip);
            if (stat != Ok)
            {
                free(cont);
                return stat;
            }

            stat = GdipBeginContainer2(metafile->playback_graphics, &cont->state);

            if (stat != Ok)
            {
                GdipDeleteRegion(cont->clip);
                free(cont);
                return stat;
            }

            cont->id = record->StackIndex;
            cont->type = BEGIN_CONTAINER;
            cont->world_transform = *metafile->world_transform;
            cont->page_unit = metafile->page_unit;
            cont->page_scale = metafile->page_scale;
            list_add_head(&real_metafile->containers, &cont->entry);

            unit = record->Header.Flags & 0xff;

            scale_x = units_to_pixels(1.0, unit, metafile->image.xres, metafile->printer_display);
            scale_y = units_to_pixels(1.0, unit, metafile->image.yres, metafile->printer_display);

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

            cont = calloc(1, sizeof(*cont));
            if (!cont)
                return OutOfMemory;

            stat = GdipCloneRegion(metafile->clip, &cont->clip);
            if (stat != Ok)
            {
                free(cont);
                return stat;
            }

            if (recordType == EmfPlusRecordTypeBeginContainerNoParams)
                stat = GdipBeginContainer2(metafile->playback_graphics, &cont->state);
            else
                stat = GdipSaveGraphics(metafile->playback_graphics, &cont->state);

            if (stat != Ok)
            {
                GdipDeleteRegion(cont->clip);
                free(cont);
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
                    free(cont2);
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
                free(cont);
            }

            break;
        }
        case EmfPlusRecordTypeSetPixelOffsetMode:
        {
            return GdipSetPixelOffsetMode(real_metafile->playback_graphics, flags & 0xff);
        }
        case EmfPlusRecordTypeSetCompositingQuality:
        {
            return GdipSetCompositingQuality(real_metafile->playback_graphics, flags & 0xff);
        }
        case EmfPlusRecordTypeSetInterpolationMode:
        {
            return GdipSetInterpolationMode(real_metafile->playback_graphics, flags & 0xff);
        }
        case EmfPlusRecordTypeSetTextRenderingHint:
        {
            return GdipSetTextRenderingHint(real_metafile->playback_graphics, flags & 0xff);
        }
        case EmfPlusRecordTypeSetAntiAliasMode:
        {
            return GdipSetSmoothingMode(real_metafile->playback_graphics, (flags >> 1) & 0xff);
        }
        case EmfPlusRecordTypeSetCompositingMode:
        {
            return GdipSetCompositingMode(real_metafile->playback_graphics, flags & 0xff);
        }
        case EmfPlusRecordTypeObject:
        {
            return METAFILE_PlaybackObject(real_metafile, flags, dataSize, data);
        }
        case EmfPlusRecordTypeDrawImage:
        {
            EmfPlusDrawImage *draw = (EmfPlusDrawImage *)header;
            BYTE image = flags & 0xff;
            GpPointF points[3];

            if (image >= EmfPlusObjectTableSize || real_metafile->objtable[image].type != ObjectTypeImage)
                return InvalidParameter;

            if (dataSize != FIELD_OFFSET(EmfPlusDrawImage, RectData) - sizeof(EmfPlusRecordHeader) +
                    (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (draw->ImageAttributesID >= EmfPlusObjectTableSize ||
                    real_metafile->objtable[draw->ImageAttributesID].type != ObjectTypeImageAttributes)
                return InvalidParameter;

            if (flags & 0x4000) /* C */
            {
                points[0].X = draw->RectData.rect.X;
                points[0].Y = draw->RectData.rect.Y;
                points[1].X = points[0].X + draw->RectData.rect.Width;
                points[1].Y = points[0].Y;
                points[2].X = points[1].X;
                points[2].Y = points[1].Y + draw->RectData.rect.Height;
            }
            else
            {
                points[0].X = draw->RectData.rectF.X;
                points[0].Y = draw->RectData.rectF.Y;
                points[1].X = points[0].X + draw->RectData.rectF.Width;
                points[1].Y = points[0].Y;
                points[2].X = points[1].X;
                points[2].Y = points[1].Y + draw->RectData.rectF.Height;
            }

            return GdipDrawImagePointsRect(real_metafile->playback_graphics, real_metafile->objtable[image].u.image,
                points, 3, draw->SrcRect.X, draw->SrcRect.Y, draw->SrcRect.Width, draw->SrcRect.Height, draw->SrcUnit,
                real_metafile->objtable[draw->ImageAttributesID].u.image_attributes, NULL, NULL);
        }
        case EmfPlusRecordTypeDrawImagePoints:
        {
            EmfPlusDrawImagePoints *draw = (EmfPlusDrawImagePoints *)header;
            static const UINT fixed_part_size = FIELD_OFFSET(EmfPlusDrawImagePoints, PointData) -
                FIELD_OFFSET(EmfPlusDrawImagePoints, ImageAttributesID);
            BYTE image = flags & 0xff;
            GpPointF points[3];
            unsigned int i;
            UINT size;

            if (image >= EmfPlusObjectTableSize || real_metafile->objtable[image].type != ObjectTypeImage)
                return InvalidParameter;

            if (dataSize <= fixed_part_size)
                return InvalidParameter;
            dataSize -= fixed_part_size;

            if (draw->ImageAttributesID >= EmfPlusObjectTableSize ||
                    real_metafile->objtable[draw->ImageAttributesID].type != ObjectTypeImageAttributes)
                return InvalidParameter;

            if (draw->count != 3)
                return InvalidParameter;

            if ((flags >> 13) & 1) /* E */
                FIXME("image effects are not supported.\n");

            if ((flags >> 11) & 1) /* P */
                size = sizeof(EmfPlusPointR7) * draw->count;
            else if ((flags >> 14) & 1) /* C */
                size = sizeof(EmfPlusPoint) * draw->count;
            else
                size = sizeof(EmfPlusPointF) * draw->count;

            if (dataSize != size)
                return InvalidParameter;

            if ((flags >> 11) & 1) /* P */
            {
                points[0].X = draw->PointData.pointsR[0].X;
                points[0].Y = draw->PointData.pointsR[0].Y;
                for (i = 1; i < 3; i++)
                {
                    points[i].X = points[i-1].X + draw->PointData.pointsR[i].X;
                    points[i].Y = points[i-1].Y + draw->PointData.pointsR[i].Y;
                }
            }
            else if ((flags >> 14) & 1) /* C */
            {
                for (i = 0; i < 3; i++)
                {
                    points[i].X = draw->PointData.points[i].X;
                    points[i].Y = draw->PointData.points[i].Y;
                }
            }
            else
                memcpy(points, draw->PointData.pointsF, sizeof(points));

            return GdipDrawImagePointsRect(real_metafile->playback_graphics, real_metafile->objtable[image].u.image,
                points, 3, draw->SrcRect.X, draw->SrcRect.Y, draw->SrcRect.Width, draw->SrcRect.Height, draw->SrcUnit,
                real_metafile->objtable[draw->ImageAttributesID].u.image_attributes, NULL, NULL);
        }
        case EmfPlusRecordTypeFillPath:
        {
            EmfPlusFillPath *fill = (EmfPlusFillPath *)header;
            GpSolidFill *solidfill = NULL;
            BYTE path = flags & 0xff;
            GpBrush *brush;

            if (path >= EmfPlusObjectTableSize || real_metafile->objtable[path].type != ObjectTypePath)
                return InvalidParameter;

            if (dataSize != sizeof(fill->data.BrushId))
                return InvalidParameter;

            if (flags & 0x8000)
            {
                stat = GdipCreateSolidFill(fill->data.Color, &solidfill);
                if (stat != Ok)
                    return stat;
                brush = (GpBrush *)solidfill;
            }
            else
            {
                if (fill->data.BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[fill->data.BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[fill->data.BrushId].u.brush;
            }

            stat = GdipFillPath(real_metafile->playback_graphics, brush, real_metafile->objtable[path].u.path);
            GdipDeleteBrush((GpBrush *)solidfill);
            return stat;
        }
        case EmfPlusRecordTypeFillClosedCurve:
        {
            static const UINT fixed_part_size = FIELD_OFFSET(EmfPlusFillClosedCurve, PointData) -
                sizeof(EmfPlusRecordHeader);
            EmfPlusFillClosedCurve *fill = (EmfPlusFillClosedCurve *)header;
            GpSolidFill *solidfill = NULL;
            GpFillMode mode;
            GpBrush *brush;
            UINT size, i;

            if (dataSize <= fixed_part_size)
                return InvalidParameter;

            if (fill->Count == 0)
                return InvalidParameter;

            if (flags & 0x800) /* P */
                size = (fixed_part_size + sizeof(EmfPlusPointR7) * fill->Count + 3) & ~3;
            else if (flags & 0x4000) /* C */
                size = fixed_part_size + sizeof(EmfPlusPoint) * fill->Count;
            else
                size = fixed_part_size + sizeof(EmfPlusPointF) * fill->Count;

            if (dataSize != size)
                return InvalidParameter;

            mode = flags & 0x200 ? FillModeWinding : FillModeAlternate; /* W */

            if (flags & 0x8000) /* S */
            {
                stat = GdipCreateSolidFill(fill->BrushId, &solidfill);
                if (stat != Ok)
                    return stat;
                brush = (GpBrush *)solidfill;
            }
            else
            {
                if (fill->BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[fill->BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[fill->BrushId].u.brush;
            }

            if (flags & (0x800 | 0x4000))
            {
                GpPointF *points = malloc(fill->Count * sizeof(*points));
                if (points)
                {
                    if (flags & 0x800) /* P */
                    {
                        points[0].X = 0;
                        points[0].Y = 0;
                        for (i = 1; i < fill->Count; i++)
                        {
                            points[i].X = points[i - 1].X + fill->PointData.pointsR[i].X;
                            points[i].Y = points[i - 1].Y + fill->PointData.pointsR[i].Y;
                        }
                    }
                    else
                    {
                        for (i = 0; i < fill->Count; i++)
                        {
                            points[i].X = fill->PointData.points[i].X;
                            points[i].Y = fill->PointData.points[i].Y;
                        }
                    }

                    stat = GdipFillClosedCurve2(real_metafile->playback_graphics, brush,
                        points, fill->Count, fill->Tension, mode);
                    free(points);
                }
                else
                    stat = OutOfMemory;
            }
            else
                stat = GdipFillClosedCurve2(real_metafile->playback_graphics, brush,
                    (const GpPointF *)fill->PointData.pointsF, fill->Count, fill->Tension, mode);

            GdipDeleteBrush((GpBrush *)solidfill);
            return stat;
        }
        case EmfPlusRecordTypeFillEllipse:
        {
            EmfPlusFillEllipse *fill = (EmfPlusFillEllipse *)header;
            GpSolidFill *solidfill = NULL;
            GpBrush *brush;

            if (dataSize <= FIELD_OFFSET(EmfPlusFillEllipse, RectData) - sizeof(EmfPlusRecordHeader))
                return InvalidParameter;
            dataSize -= FIELD_OFFSET(EmfPlusFillEllipse, RectData) - sizeof(EmfPlusRecordHeader);

            if (dataSize != (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x8000)
            {
                stat = GdipCreateSolidFill(fill->BrushId, &solidfill);
                if (stat != Ok)
                    return stat;
                brush = (GpBrush *)solidfill;
            }
            else
            {
                if (fill->BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[fill->BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[fill->BrushId].u.brush;
            }

            if (flags & 0x4000)
                stat = GdipFillEllipseI(real_metafile->playback_graphics, brush, fill->RectData.rect.X,
                    fill->RectData.rect.Y, fill->RectData.rect.Width, fill->RectData.rect.Height);
            else
                stat = GdipFillEllipse(real_metafile->playback_graphics, brush, fill->RectData.rectF.X,
                    fill->RectData.rectF.Y, fill->RectData.rectF.Width, fill->RectData.rectF.Height);

            GdipDeleteBrush((GpBrush *)solidfill);
            return stat;
        }
        case EmfPlusRecordTypeFillPie:
        {
            EmfPlusFillPie *fill = (EmfPlusFillPie *)header;
            GpSolidFill *solidfill = NULL;
            GpBrush *brush;

            if (dataSize <= FIELD_OFFSET(EmfPlusFillPie, RectData) - sizeof(EmfPlusRecordHeader))
                return InvalidParameter;
            dataSize -= FIELD_OFFSET(EmfPlusFillPie, RectData) - sizeof(EmfPlusRecordHeader);

            if (dataSize != (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x8000) /* S */
            {
                stat = GdipCreateSolidFill(fill->BrushId, &solidfill);
                if (stat != Ok)
                    return stat;
                brush = (GpBrush *)solidfill;
            }
            else
            {
                if (fill->BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[fill->BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[fill->BrushId].u.brush;
            }

            if (flags & 0x4000) /* C */
                stat = GdipFillPieI(real_metafile->playback_graphics, brush, fill->RectData.rect.X,
                    fill->RectData.rect.Y, fill->RectData.rect.Width, fill->RectData.rect.Height,
                    fill->StartAngle, fill->SweepAngle);
            else
                stat = GdipFillPie(real_metafile->playback_graphics, brush, fill->RectData.rectF.X,
                    fill->RectData.rectF.Y, fill->RectData.rectF.Width, fill->RectData.rectF.Height,
                    fill->StartAngle, fill->SweepAngle);

            GdipDeleteBrush((GpBrush *)solidfill);
            return stat;
        }
        case EmfPlusRecordTypeDrawPath:
        {
            EmfPlusDrawPath *draw = (EmfPlusDrawPath *)header;
            BYTE path = flags & 0xff;

            if (dataSize != sizeof(draw->PenId))
                return InvalidParameter;

            if (path >= EmfPlusObjectTableSize || draw->PenId >= EmfPlusObjectTableSize)
                return InvalidParameter;

            if (real_metafile->objtable[path].type != ObjectTypePath ||
                    real_metafile->objtable[draw->PenId].type != ObjectTypePen)
                return InvalidParameter;

            return GdipDrawPath(real_metafile->playback_graphics, real_metafile->objtable[draw->PenId].u.pen,
                real_metafile->objtable[path].u.path);
        }
        case EmfPlusRecordTypeDrawArc:
        {
            EmfPlusDrawArc *draw = (EmfPlusDrawArc *)header;
            BYTE pen = flags & 0xff;

            if (pen >= EmfPlusObjectTableSize || real_metafile->objtable[pen].type != ObjectTypePen)
                return InvalidParameter;

            if (dataSize != FIELD_OFFSET(EmfPlusDrawArc, RectData) - sizeof(EmfPlusRecordHeader) +
                    (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x4000) /* C */
                return GdipDrawArcI(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rect.X, draw->RectData.rect.Y, draw->RectData.rect.Width,
                    draw->RectData.rect.Height, draw->StartAngle, draw->SweepAngle);
            else
                return GdipDrawArc(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rectF.X, draw->RectData.rectF.Y, draw->RectData.rectF.Width,
                    draw->RectData.rectF.Height, draw->StartAngle, draw->SweepAngle);
        }
        case EmfPlusRecordTypeDrawEllipse:
        {
            EmfPlusDrawEllipse *draw = (EmfPlusDrawEllipse *)header;
            BYTE pen = flags & 0xff;

            if (pen >= EmfPlusObjectTableSize || real_metafile->objtable[pen].type != ObjectTypePen)
                return InvalidParameter;

            if (dataSize != (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x4000) /* C */
                return GdipDrawEllipseI(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rect.X, draw->RectData.rect.Y, draw->RectData.rect.Width,
                    draw->RectData.rect.Height);
            else
                return GdipDrawEllipse(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rectF.X, draw->RectData.rectF.Y, draw->RectData.rectF.Width,
                    draw->RectData.rectF.Height);
        }
        case EmfPlusRecordTypeDrawPie:
        {
            EmfPlusDrawPie *draw = (EmfPlusDrawPie *)header;
            BYTE pen = flags & 0xff;

            if (pen >= EmfPlusObjectTableSize || real_metafile->objtable[pen].type != ObjectTypePen)
                return InvalidParameter;

            if (dataSize != FIELD_OFFSET(EmfPlusDrawPie, RectData) - sizeof(EmfPlusRecordHeader) +
                    (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x4000) /* C */
                return GdipDrawPieI(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rect.X, draw->RectData.rect.Y, draw->RectData.rect.Width,
                    draw->RectData.rect.Height, draw->StartAngle, draw->SweepAngle);
            else
                return GdipDrawPie(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    draw->RectData.rectF.X, draw->RectData.rectF.Y, draw->RectData.rectF.Width,
                    draw->RectData.rectF.Height, draw->StartAngle, draw->SweepAngle);
        }
        case EmfPlusRecordTypeDrawRects:
        {
            EmfPlusDrawRects *draw = (EmfPlusDrawRects *)header;
            BYTE pen = flags & 0xff;
            GpRectF *rects = NULL;

            if (pen >= EmfPlusObjectTableSize || real_metafile->objtable[pen].type != ObjectTypePen)
                return InvalidParameter;

            if (dataSize <= FIELD_OFFSET(EmfPlusDrawRects, RectData) - sizeof(EmfPlusRecordHeader))
                return InvalidParameter;
            dataSize -= FIELD_OFFSET(EmfPlusDrawRects, RectData) - sizeof(EmfPlusRecordHeader);

            if (dataSize != draw->Count * (flags & 0x4000 ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)))
                return InvalidParameter;

            if (flags & 0x4000)
            {
                DWORD i;

                rects = malloc(draw->Count * sizeof(*rects));
                if (!rects)
                    return OutOfMemory;

                for (i = 0; i < draw->Count; i++)
                {
                    rects[i].X = draw->RectData.rect[i].X;
                    rects[i].Y = draw->RectData.rect[i].Y;
                    rects[i].Width = draw->RectData.rect[i].Width;
                    rects[i].Height = draw->RectData.rect[i].Height;
                }
            }

            stat = GdipDrawRectangles(real_metafile->playback_graphics, real_metafile->objtable[pen].u.pen,
                    rects ? rects : (GpRectF *)draw->RectData.rectF, draw->Count);
            free(rects);
            return stat;
        }
        case EmfPlusRecordTypeDrawDriverString:
        {
            GpBrush *brush;
            DWORD expected_size;
            UINT16 *text;
            PointF *positions;
            GpSolidFill *solidfill = NULL;
            void* alignedmem = NULL;
            GpMatrix *matrix = NULL;
            BYTE font = flags & 0xff;
            EmfPlusDrawDriverString *draw = (EmfPlusDrawDriverString*)header;

            if (font >= EmfPlusObjectTableSize ||
                    real_metafile->objtable[font].type != ObjectTypeFont)
                return InvalidParameter;

            expected_size = FIELD_OFFSET(EmfPlusDrawDriverString, VariableData) -
                sizeof(EmfPlusRecordHeader);
            if (dataSize < expected_size || draw->GlyphCount <= 0)
                return InvalidParameter;

            expected_size += draw->GlyphCount * (sizeof(*text) + sizeof(*positions));
            if (draw->MatrixPresent)
                expected_size += sizeof(*matrix);

            /* Pad expected size to DWORD alignment. */
            expected_size = (expected_size + 3) & ~3;

            if (dataSize != expected_size)
                return InvalidParameter;

            if (flags & 0x8000)
            {
                stat = GdipCreateSolidFill(draw->brush.Color, &solidfill);

                if (stat != Ok)
                    return InvalidParameter;

                brush = (GpBrush*)solidfill;
            }
            else
            {
                if (draw->brush.BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[draw->brush.BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[draw->brush.BrushId].u.brush;
            }

            text = (UINT16*)&draw->VariableData[0];

            /* If GlyphCount is odd, all subsequent fields will be 2-byte
               aligned rather than 4-byte aligned, which may lead to access
               issues. Handle this case by making our own copy of positions. */
            if (draw->GlyphCount % 2)
            {
                SIZE_T alloc_size = draw->GlyphCount * sizeof(*positions);

                if (draw->MatrixPresent)
                    alloc_size += sizeof(*matrix);

                positions = alignedmem = malloc(alloc_size);
                if (!positions)
                {
                    GdipDeleteBrush((GpBrush*)solidfill);
                    return OutOfMemory;
                }

                memcpy(positions, &text[draw->GlyphCount], alloc_size);
            }
            else
                positions = (PointF*)&text[draw->GlyphCount];

            if (draw->MatrixPresent)
                matrix = (GpMatrix*)&positions[draw->GlyphCount];

            stat = GdipDrawDriverString(real_metafile->playback_graphics, text, draw->GlyphCount,
                    real_metafile->objtable[font].u.font, brush, positions,
                    draw->DriverStringOptionsFlags, matrix);

            GdipDeleteBrush((GpBrush*)solidfill);
            free(alignedmem);

            return stat;
        }
        case EmfPlusRecordTypeFillRegion:
        {
            EmfPlusFillRegion * const fill = (EmfPlusFillRegion*)header;
            GpSolidFill *solidfill = NULL;
            GpBrush *brush;
            BYTE region = flags & 0xff;

            if (dataSize != sizeof(EmfPlusFillRegion) - sizeof(EmfPlusRecordHeader))
                return InvalidParameter;

            if (region >= EmfPlusObjectTableSize ||
                    real_metafile->objtable[region].type != ObjectTypeRegion)
                return InvalidParameter;

            if (flags & 0x8000)
            {
                stat = GdipCreateSolidFill(fill->data.Color, &solidfill);
                if (stat != Ok)
                    return stat;
                brush = (GpBrush*)solidfill;
            }
            else
            {
                if (fill->data.BrushId >= EmfPlusObjectTableSize ||
                        real_metafile->objtable[fill->data.BrushId].type != ObjectTypeBrush)
                    return InvalidParameter;

                brush = real_metafile->objtable[fill->data.BrushId].u.brush;
            }

            stat = GdipFillRegion(real_metafile->playback_graphics, brush,
                real_metafile->objtable[region].u.region);
            GdipDeleteBrush((GpBrush*)solidfill);

            return stat;
        }
        case EmfPlusRecordTypeSetRenderingOrigin:
        {
            const EmfPlusSetRenderingOrigin *origin = (const EmfPlusSetRenderingOrigin *)header;

            if (dataSize + sizeof(EmfPlusRecordHeader) < sizeof(EmfPlusSetRenderingOrigin))
                return InvalidParameter;

            return GdipSetRenderingOrigin(real_metafile->playback_graphics, origin->x, origin->y);
        }
        default:
            if (recordType >= GDIP_EMFPLUS_RECORD_BASE)
                FIXME("Not implemented for EMF+ record type %u\n", recordType - GDIP_EMFPLUS_RECORD_BASE);
            else
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
    RECT dst_bounds;

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

        if (stat == Ok)
        {
            stat = METAFILE_PlaybackGetDC(real_metafile);

            dst_bounds.left = real_metafile->playback_points[0].X;
            dst_bounds.right = real_metafile->playback_points[1].X;
            dst_bounds.top = real_metafile->playback_points[0].Y;
            dst_bounds.bottom = real_metafile->playback_points[2].Y;
        }

        if (stat == Ok)
            EnumEnhMetaFile(real_metafile->playback_dc, metafile->hemf, enum_metafile_proc,
                &data, &dst_bounds);

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
            free(cont);
        }

        GdipEndContainer(graphics, state);
    }

    real_metafile->playback_graphics = NULL;

    return stat;
}

GpStatus WINGDIPAPI GdipEnumerateMetafileSrcRectDestRect( GpGraphics *graphics,
        GDIPCONST GpMetafile *metafile, GDIPCONST GpRectF *dest,
        GDIPCONST GpRectF *src, Unit srcUnit, EnumerateMetafileProc callback,
        VOID *cb_data, GDIPCONST GpImageAttributes *attrs)
{
    GpPointF points[3];

    if (!graphics || !metafile || !dest) return InvalidParameter;

    points[0].X = points[2].X = dest->X;
    points[0].Y = points[1].Y = dest->Y;
    points[1].X = dest->X + dest->Width;
    points[2].Y = dest->Y + dest->Height;

    return GdipEnumerateMetafileSrcRectDestPoints(graphics, metafile, points, 3,
        src, srcUnit, callback, cb_data, attrs);
}
GpStatus WINGDIPAPI GdipEnumerateMetafileSrcRectDestRectI( GpGraphics * graphics,
	GDIPCONST GpMetafile *metafile, GDIPCONST Rect *destRect,
	GDIPCONST Rect *srcRect, Unit srcUnit, EnumerateMetafileProc callback,
	VOID *cb_data, GDIPCONST GpImageAttributes *attrs )
{
    GpRectF destRectF, srcRectF;

    destRectF.X = destRect->X;
    destRectF.Y = destRect->Y;
    destRectF.Width = destRect->Width;
    destRectF.Height = destRect->Height;

    srcRectF.X = srcRect->X;
    srcRectF.Y = srcRect->Y;
    srcRectF.Width = srcRect->Width;
    srcRectF.Height = srcRect->Height;

    return GdipEnumerateMetafileSrcRectDestRect( graphics, metafile, &destRectF, &srcRectF, srcUnit, callback, cb_data, attrs);
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
    destf.Width = units_to_pixels(metafile->bounds.Width, metafile->unit,
                                  metafile->image.xres, metafile->printer_display);
    destf.Height = units_to_pixels(metafile->bounds.Height, metafile->unit,
                                   metafile->image.yres, metafile->printer_display);

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
        header->Version = VERSION_MAGIC2;
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
    header->EmfHeader = emfheader;

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

    *metafile = calloc(1, sizeof(GpMetafile));
    if (!*metafile)
        return OutOfMemory;

    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->image.format = ImageFormatEMF;
    (*metafile)->image.frame_count = 1;
    (*metafile)->image.xres = header.DpiX;
    (*metafile)->image.yres = header.DpiY;
    (*metafile)->bounds.X = (REAL)header.EmfHeader.rclFrame.left / 2540.0 * header.DpiX;
    (*metafile)->bounds.Y = (REAL)header.EmfHeader.rclFrame.top / 2540.0 * header.DpiY;
    (*metafile)->bounds.Width = (REAL)(header.EmfHeader.rclFrame.right - header.EmfHeader.rclFrame.left)
                                / 2540.0 * header.DpiX;
    (*metafile)->bounds.Height = (REAL)(header.EmfHeader.rclFrame.bottom - header.EmfHeader.rclFrame.top)
                                 / 2540.0 * header.DpiY;
    (*metafile)->unit = UnitPixel;
    (*metafile)->metafile_type = header.Type;
    (*metafile)->hemf = hemf;
    (*metafile)->preserve_hemf = !delete;
    /* If the 31th bit of EmfPlusFlags was set, metafile was recorded with a DC for a video display.
     * If clear, metafile was recorded with a DC for a printer */
    (*metafile)->printer_display = !(header.EmfPlusFlags & (1u << 31));
    (*metafile)->logical_dpix = header.LogicalDpiX;
    (*metafile)->logical_dpiy = header.LogicalDpiY;
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
    copy = malloc(read);
    GetMetaFileBitsEx(hwmf, read, copy);

    hemf = SetWinMetaFileBits(read, copy, NULL, NULL);
    free(copy);

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
    HMETAFILE hmf;
    HENHMETAFILE emf;

    TRACE("(%s, %p, %p)\n", debugstr_w(file), placeable, metafile);

    hmf = GetMetaFileW(file);
    if(hmf)
        return GdipCreateMetafileFromWmf(hmf, TRUE, placeable, metafile);

    emf = GetEnhMetaFileW(file);
    if(emf)
        return GdipCreateMetafileFromEmf(emf, TRUE, metafile);

    return GenericError;
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

GpStatus WINGDIPAPI GdipGetMetafileDownLevelRasterizationLimit(GDIPCONST GpMetafile *metafile,
    UINT *limitDpi)
{
    TRACE("(%p,%p)\n", metafile, limitDpi);

    if (!metafile || !limitDpi)
        return InvalidParameter;

    if (!metafile->record_dc)
        return WrongState;

    *limitDpi = metafile->limit_dpi;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetMetafileDownLevelRasterizationLimit(GpMetafile *metafile,
    UINT limitDpi)
{
    TRACE("(%p,%u)\n", metafile, limitDpi);

    if (limitDpi == 0)
        limitDpi = 96;

    if (!metafile || limitDpi < 10)
        return InvalidParameter;

    if (!metafile->record_dc)
        return WrongState;

    metafile->limit_dpi = limitDpi;

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
    HDC record_dc;
    REAL dpix, dpiy;
    REAL framerect_factor_x, framerect_factor_y;
    RECT rc, *lprc;
    GpStatus stat;

    TRACE("%s %p %d %s %d %s %p\n", debugstr_w(fileName), hdc, type, debugstr_rectf(pFrameRect),
                                 frameUnit, debugstr_w(desc), metafile);

    if (!hdc || type < EmfTypeEmfOnly || type > EmfTypeEmfPlusDual || !metafile)
        return InvalidParameter;

    dpix = (REAL)GetDeviceCaps(hdc, HORZRES) / GetDeviceCaps(hdc, HORZSIZE) * 25.4;
    dpiy = (REAL)GetDeviceCaps(hdc, VERTRES) / GetDeviceCaps(hdc, VERTSIZE) * 25.4;

    if (pFrameRect)
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

        rc.left = framerect_factor_x * pFrameRect->X;
        rc.top = framerect_factor_y * pFrameRect->Y;
        rc.right = rc.left + framerect_factor_x * pFrameRect->Width;
        rc.bottom = rc.top + framerect_factor_y * pFrameRect->Height;

        lprc = &rc;
    }
    else
        lprc = NULL;

    record_dc = CreateEnhMetaFileW(hdc, fileName, lprc, desc);

    if (!record_dc)
        return GenericError;

    *metafile = calloc(1, sizeof(GpMetafile));
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
    (*metafile)->metafile_type = (MetafileType)type;
    (*metafile)->record_dc = record_dc;
    (*metafile)->comment_data = NULL;
    (*metafile)->comment_data_size = 0;
    (*metafile)->comment_data_length = 0;
    (*metafile)->limit_dpi = 96;
    (*metafile)->hemf = NULL;
    (*metafile)->printer_display = (GetDeviceCaps(record_dc, TECHNOLOGY) == DT_RASPRINTER);
    (*metafile)->logical_dpix = (REAL)GetDeviceCaps(record_dc, LOGPIXELSX);
    (*metafile)->logical_dpiy = (REAL)GetDeviceCaps(record_dc, LOGPIXELSY);
    list_init(&(*metafile)->containers);

    if (!pFrameRect)
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
        free(*metafile);
        *metafile = NULL;
        return OutOfMemory;
    }

    return stat;
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

static GpStatus METAFILE_CreateCompressedImageStream(GpImage *image, IStream **stream, DWORD *size)
{
    LARGE_INTEGER zero;
    STATSTG statstg;
    GpStatus stat;
    HRESULT hr;

    *size = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, stream);
    if (FAILED(hr)) return hresult_to_status(hr);

    stat = encode_image_png(image, *stream, NULL);
    if (stat != Ok)
    {
        IStream_Release(*stream);
        return stat;
    }

    hr = IStream_Stat(*stream, &statstg, 1);
    if (FAILED(hr))
    {
        IStream_Release(*stream);
        return hresult_to_status(hr);
    }
    *size = statstg.cbSize.u.LowPart;

    zero.QuadPart = 0;
    hr = IStream_Seek(*stream, zero, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        IStream_Release(*stream);
        return hresult_to_status(hr);
    }

    return Ok;
}

static GpStatus METAFILE_FillEmfPlusBitmap(EmfPlusBitmap *record, IStream *stream, DWORD size)
{
    HRESULT hr;

    record->Width = 0;
    record->Height = 0;
    record->Stride = 0;
    record->PixelFormat = 0;
    record->Type = BitmapDataTypeCompressed;

    hr = IStream_Read(stream, record->BitmapData, size, NULL);
    if (FAILED(hr)) return hresult_to_status(hr);
    return Ok;
}

static GpStatus METAFILE_AddImageObject(GpMetafile *metafile, GpImage *image, DWORD *id)
{
    EmfPlusObject *object_record;
    GpStatus stat;
    DWORD size;

    *id = -1;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    if (image->type == ImageTypeBitmap)
    {
        IStream *stream;
        DWORD aligned_size;

        stat = METAFILE_CreateCompressedImageStream(image, &stream, &size);
        if (stat != Ok) return stat;
        aligned_size = (size + 3) & ~3;

        stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
                FIELD_OFFSET(EmfPlusObject, ObjectData.image.ImageData.bitmap.BitmapData[aligned_size]),
                (void**)&object_record);
        if (stat != Ok)
        {
            IStream_Release(stream);
            return stat;
        }
        memset(object_record->ObjectData.image.ImageData.bitmap.BitmapData + size, 0, aligned_size - size);

        *id = METAFILE_AddObjectId(metafile);
        object_record->Header.Flags = *id | ObjectTypeImage << 8;
        object_record->ObjectData.image.Version = VERSION_MAGIC2;
        object_record->ObjectData.image.Type = ImageDataTypeBitmap;

        stat = METAFILE_FillEmfPlusBitmap(&object_record->ObjectData.image.ImageData.bitmap, stream, size);
        IStream_Release(stream);
        if (stat != Ok) METAFILE_RemoveLastRecord(metafile, &object_record->Header);
        return stat;
    }
    else if (image->type == ImageTypeMetafile)
    {
        HENHMETAFILE hemf = ((GpMetafile*)image)->hemf;
        EmfPlusMetafile *metafile_record;

        if (!hemf) return InvalidParameter;

        size = GetEnhMetaFileBits(hemf, 0, NULL);
        if (!size) return GenericError;

        stat  = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
                FIELD_OFFSET(EmfPlusObject, ObjectData.image.ImageData.metafile.MetafileData[size]),
                (void**)&object_record);
        if (stat != Ok) return stat;

        *id = METAFILE_AddObjectId(metafile);
        object_record->Header.Flags = *id | ObjectTypeImage << 8;
        object_record->ObjectData.image.Version = VERSION_MAGIC2;
        object_record->ObjectData.image.Type = ImageDataTypeMetafile;
        metafile_record = &object_record->ObjectData.image.ImageData.metafile;
        metafile_record->Type = ((GpMetafile*)image)->metafile_type;
        metafile_record->MetafileDataSize = size;
        if (GetEnhMetaFileBits(hemf, size, metafile_record->MetafileData) != size)
        {
            METAFILE_RemoveLastRecord(metafile, &object_record->Header);
            return GenericError;
        }
        return Ok;
    }
    else
    {
        FIXME("not supported image type (%d)\n", image->type);
        return NotImplemented;
    }
}

static GpStatus METAFILE_AddImageAttributesObject(GpMetafile *metafile, const GpImageAttributes *attrs, DWORD *id)
{
    EmfPlusObject *object_record;
    EmfPlusImageAttributes *attrs_record;
    GpStatus stat;

    *id = -1;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    if (!attrs)
        return Ok;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
            FIELD_OFFSET(EmfPlusObject, ObjectData.image_attributes) + sizeof(EmfPlusImageAttributes),
            (void**)&object_record);
    if (stat != Ok) return stat;

    *id = METAFILE_AddObjectId(metafile);
    object_record->Header.Flags = *id | (ObjectTypeImageAttributes << 8);
    attrs_record = &object_record->ObjectData.image_attributes;
    attrs_record->Version = VERSION_MAGIC2;
    attrs_record->Reserved1 = 0;
    attrs_record->WrapMode = attrs->wrap;
    attrs_record->ClampColor = attrs->outside_color;
    attrs_record->ObjectClamp = attrs->clamp;
    attrs_record->Reserved2 = 0;
    return Ok;
}

GpStatus METAFILE_DrawImagePointsRect(GpMetafile *metafile, GpImage *image,
     GDIPCONST GpPointF *points, INT count, REAL srcx, REAL srcy, REAL srcwidth,
     REAL srcheight, GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes,
     DrawImageAbort callback, VOID *callbackData)
{
    EmfPlusDrawImagePoints *draw_image_record;
    DWORD image_id, attributes_id;
    GpStatus stat;

    if (count != 3) return InvalidParameter;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("MetafileTypeEmf metafiles not supported\n");
        return NotImplemented;
    }
    else
        FIXME("semi-stub\n");

    if (!imageAttributes)
    {
        stat = METAFILE_AddImageObject(metafile, image, &image_id);
    }
    else if (image->type == ImageTypeBitmap)
    {
        INT width = ((GpBitmap*)image)->width;
        INT height = ((GpBitmap*)image)->height;
        GpGraphics *graphics;
        GpBitmap *bitmap;

        stat = GdipCreateBitmapFromScan0(width, height,
                0, PixelFormat32bppARGB, NULL, &bitmap);
        if (stat != Ok) return stat;

        stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
        if (stat != Ok)
        {
            GdipDisposeImage((GpImage*)bitmap);
            return stat;
        }

        stat = GdipDrawImageRectRectI(graphics, image, 0, 0, width, height,
                0, 0, width, height, UnitPixel, imageAttributes, NULL, NULL);
        GdipDeleteGraphics(graphics);
        if (stat != Ok)
        {
            GdipDisposeImage((GpImage*)bitmap);
            return stat;
        }

        stat = METAFILE_AddImageObject(metafile, (GpImage*)bitmap, &image_id);
        GdipDisposeImage((GpImage*)bitmap);
    }
    else
    {
        FIXME("imageAttributes not supported (image type %d)\n", image->type);
        return NotImplemented;
    }
    if (stat != Ok) return stat;

    stat = METAFILE_AddImageAttributesObject(metafile, imageAttributes, &attributes_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawImagePoints,
        sizeof(EmfPlusDrawImagePoints), (void **)&draw_image_record);
    if (stat != Ok) return stat;

    draw_image_record->Header.Flags = image_id;
    draw_image_record->ImageAttributesID = attributes_id;
    draw_image_record->SrcUnit = UnitPixel;
    draw_image_record->SrcRect.X = units_to_pixels(srcx, srcUnit, metafile->image.xres, metafile->printer_display);
    draw_image_record->SrcRect.Y = units_to_pixels(srcy, srcUnit, metafile->image.yres, metafile->printer_display);
    draw_image_record->SrcRect.Width = units_to_pixels(srcwidth, srcUnit, metafile->image.xres, metafile->printer_display);
    draw_image_record->SrcRect.Height = units_to_pixels(srcheight, srcUnit, metafile->image.yres, metafile->printer_display);
    draw_image_record->count = 3;
    memcpy(draw_image_record->PointData.pointsF, points, 3 * sizeof(*points));
    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_AddSimpleProperty(GpMetafile *metafile, SHORT prop, SHORT val)
{
    EmfPlusRecordHeader *record;
    GpStatus stat;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    stat = METAFILE_AllocateRecord(metafile, prop, sizeof(*record), (void**)&record);
    if (stat != Ok) return stat;

    record->Flags = val;

    METAFILE_WriteRecords(metafile);
    return Ok;
}

static GpStatus METAFILE_AddPathObject(GpMetafile *metafile, GpPath *path, DWORD *id)
{
    EmfPlusObject *object_record;
    GpStatus stat;
    DWORD size;

    *id = -1;
    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    size = write_path_data(path, NULL);
    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
            FIELD_OFFSET(EmfPlusObject, ObjectData.path) + size,
            (void**)&object_record);
    if (stat != Ok) return stat;

    *id = METAFILE_AddObjectId(metafile);
    object_record->Header.Flags = *id | ObjectTypePath << 8;
    write_path_data(path, &object_record->ObjectData.path);
    return Ok;
}

static GpStatus METAFILE_AddPenObject(GpMetafile *metafile, GpPen *pen, DWORD *id)
{
    DWORD custom_start_cap_size = 0, custom_start_cap_data_size = 0, custom_start_cap_path_size = 0;
    DWORD custom_end_cap_size = 0, custom_end_cap_data_size = 0, custom_end_cap_path_size = 0;
    DWORD i, data_flags, pen_data_size, brush_size;
    EmfPlusObject *object_record;
    EmfPlusPenData *pen_data;
    GpStatus stat;
    BOOL result;

    *id = -1;
    if (metafile->metafile_type != MetafileTypeEmfPlusOnly && metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    data_flags = 0;
    pen_data_size = FIELD_OFFSET(EmfPlusPenData, OptionalData);

    GdipIsMatrixIdentity(&pen->transform, &result);
    if (!result)
    {
        data_flags |= PenDataTransform;
        pen_data_size += sizeof(EmfPlusTransformMatrix);
    }
    if (pen->startcap != LineCapFlat)
    {
        data_flags |= PenDataStartCap;
        pen_data_size += sizeof(DWORD);
    }
    if (pen->endcap != LineCapFlat)
    {
        data_flags |= PenDataEndCap;
        pen_data_size += sizeof(DWORD);
    }
    if (pen->join != LineJoinMiter)
    {
        data_flags |= PenDataJoin;
        pen_data_size += sizeof(DWORD);
    }
    if (pen->miterlimit != 10.0)
    {
        data_flags |= PenDataMiterLimit;
        pen_data_size += sizeof(REAL);
    }
    if (pen->style != GP_DEFAULT_PENSTYLE)
    {
        data_flags |= PenDataLineStyle;
        pen_data_size += sizeof(DWORD);
    }
    if (pen->dashcap != DashCapFlat)
    {
        data_flags |= PenDataDashedLineCap;
        pen_data_size += sizeof(DWORD);
    }
    data_flags |= PenDataDashedLineOffset;
    pen_data_size += sizeof(REAL);
    if (pen->numdashes)
    {
        data_flags |= PenDataDashedLine;
        pen_data_size += sizeof(DWORD) + pen->numdashes*sizeof(REAL);
    }
    if (pen->align != PenAlignmentCenter)
    {
        data_flags |= PenDataNonCenter;
        pen_data_size += sizeof(DWORD);
    }
    /* TODO: Add support for PenDataCompoundLine */
    if (pen->customstart)
    {
        data_flags |= PenDataCustomStartCap;
        METAFILE_PrepareCustomLineCapData(pen->customstart, &custom_start_cap_size,
                                          &custom_start_cap_data_size, &custom_start_cap_path_size);
        pen_data_size += custom_start_cap_size;
    }
    if (pen->customend)
    {
        data_flags |= PenDataCustomEndCap;
        METAFILE_PrepareCustomLineCapData(pen->customend, &custom_end_cap_size,
                                          &custom_end_cap_data_size, &custom_end_cap_path_size);
        pen_data_size += custom_end_cap_size;
    }

    stat = METAFILE_PrepareBrushData(pen->brush, &brush_size);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
            FIELD_OFFSET(EmfPlusObject, ObjectData.pen.data) + pen_data_size + brush_size,
            (void**)&object_record);
    if (stat != Ok) return stat;

    *id = METAFILE_AddObjectId(metafile);
    object_record->Header.Flags = *id | ObjectTypePen << 8;
    object_record->ObjectData.pen.Version = VERSION_MAGIC2;
    object_record->ObjectData.pen.Type = 0;

    pen_data = (EmfPlusPenData*)object_record->ObjectData.pen.data;
    pen_data->PenDataFlags = data_flags;
    pen_data->PenUnit = pen->unit;
    pen_data->PenWidth = pen->width;

    i = 0;
    if (data_flags & PenDataTransform)
    {
        EmfPlusTransformMatrix *m = (EmfPlusTransformMatrix*)(pen_data->OptionalData + i);
        memcpy(m, &pen->transform, sizeof(*m));
        i += sizeof(EmfPlusTransformMatrix);
    }
    if (data_flags & PenDataStartCap)
    {
        *(DWORD*)(pen_data->OptionalData + i) = pen->startcap;
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataEndCap)
    {
        *(DWORD*)(pen_data->OptionalData + i) = pen->endcap;
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataJoin)
    {
        *(DWORD*)(pen_data->OptionalData + i) = pen->join;
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataMiterLimit)
    {
        *(REAL*)(pen_data->OptionalData + i) = pen->miterlimit;
        i += sizeof(REAL);
    }
    if (data_flags & PenDataLineStyle)
    {
        switch (pen->style & PS_STYLE_MASK)
        {
        case PS_SOLID: *(DWORD*)(pen_data->OptionalData + i) = LineStyleSolid; break;
        case PS_DASH: *(DWORD*)(pen_data->OptionalData + i) = LineStyleDash; break;
        case PS_DOT: *(DWORD*)(pen_data->OptionalData + i) = LineStyleDot; break;
        case PS_DASHDOT: *(DWORD*)(pen_data->OptionalData + i) = LineStyleDashDot; break;
        case PS_DASHDOTDOT: *(DWORD*)(pen_data->OptionalData + i) = LineStyleDashDotDot; break;
        default: *(DWORD*)(pen_data->OptionalData + i) = LineStyleCustom; break;
        }
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataDashedLineCap)
    {
        *(DWORD*)(pen_data->OptionalData + i) = pen->dashcap;
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataDashedLineOffset)
    {
        *(REAL*)(pen_data->OptionalData + i) = pen->offset;
        i += sizeof(REAL);
    }
    if (data_flags & PenDataDashedLine)
    {
        int j;

        *(DWORD*)(pen_data->OptionalData + i) = pen->numdashes;
        i += sizeof(DWORD);

        for (j=0; j<pen->numdashes; j++)
        {
            *(REAL*)(pen_data->OptionalData + i) = pen->dashes[j];
            i += sizeof(REAL);
        }
    }
    if (data_flags & PenDataNonCenter)
    {
        *(REAL*)(pen_data->OptionalData + i) = pen->align;
        i += sizeof(DWORD);
    }
    if (data_flags & PenDataCustomStartCap)
    {
        METAFILE_FillCustomLineCapData(pen->customstart, pen_data->OptionalData + i,
                                       pen->miterlimit, custom_start_cap_data_size,
                                       custom_start_cap_path_size);
        i += custom_start_cap_size;
    }
    if (data_flags & PenDataCustomEndCap)
    {
        METAFILE_FillCustomLineCapData(pen->customend, pen_data->OptionalData + i,
                                       pen->miterlimit, custom_end_cap_data_size,
                                       custom_end_cap_path_size);
        i += custom_end_cap_size;
    }

    METAFILE_FillBrushData(pen->brush,
            (EmfPlusBrush*)(object_record->ObjectData.pen.data + pen_data_size));
    return Ok;
}

GpStatus METAFILE_DrawPath(GpMetafile *metafile, GpPen *pen, GpPath *path)
{
    EmfPlusDrawPath *draw_path_record;
    DWORD path_id;
    DWORD pen_id;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddPenObject(metafile, pen, &pen_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AddPathObject(metafile, path, &path_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawPath,
        sizeof(EmfPlusDrawPath), (void **)&draw_path_record);
    if (stat != Ok) return stat;
    draw_path_record->Header.Type = EmfPlusRecordTypeDrawPath;
    draw_path_record->Header.Flags = path_id;
    draw_path_record->PenId = pen_id;

    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_DrawEllipse(GpMetafile *metafile, GpPen *pen, GpRectF *rect)
{
    EmfPlusDrawEllipse *record;
    BOOL is_int_rect;
    GpStatus stat;
    DWORD pen_id;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddPenObject(metafile, pen, &pen_id);
    if (stat != Ok) return stat;

    is_int_rect = is_integer_rect(rect);

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawEllipse,
            FIELD_OFFSET(EmfPlusDrawEllipse, RectData) + (is_int_rect ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)),
            (void **)&record);
    if (stat != Ok) return stat;
    record->Header.Type = EmfPlusRecordTypeDrawEllipse;
    record->Header.Flags = pen_id;
    if (is_int_rect)
    {
        record->Header.Flags |= 0x4000;
        record->RectData.rect.X = (SHORT)rect->X;
        record->RectData.rect.Y = (SHORT)rect->Y;
        record->RectData.rect.Width = (SHORT)rect->Width;
        record->RectData.rect.Height = (SHORT)rect->Height;
    }
    else
        memcpy(&record->RectData.rectF, rect, sizeof(*rect));

    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_FillPath(GpMetafile *metafile, GpBrush *brush, GpPath *path)
{
    EmfPlusFillPath *fill_path_record;
    DWORD brush_id = -1, path_id;
    BOOL inline_color;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    inline_color = brush->bt == BrushTypeSolidColor;
    if (!inline_color)
    {
        stat = METAFILE_AddBrushObject(metafile, brush, &brush_id);
        if (stat != Ok) return stat;
    }

    stat = METAFILE_AddPathObject(metafile, path, &path_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeFillPath,
            sizeof(EmfPlusFillPath), (void**)&fill_path_record);
    if (stat != Ok) return stat;
    if (inline_color)
    {
        fill_path_record->Header.Flags = 0x8000 | path_id;
        fill_path_record->data.Color = ((GpSolidFill *)brush)->color;
    }
    else
    {
        fill_path_record->Header.Flags = path_id;
        fill_path_record->data.BrushId = brush_id;
    }

    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_FillEllipse(GpMetafile *metafile, GpBrush *brush, GpRectF *rect)
{
    BOOL is_int_rect, inline_color;
    EmfPlusFillEllipse *record;
    DWORD brush_id = -1;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    inline_color = brush->bt == BrushTypeSolidColor;
    if (!inline_color)
    {
        stat = METAFILE_AddBrushObject(metafile, brush, &brush_id);
        if (stat != Ok) return stat;
    }

    is_int_rect = is_integer_rect(rect);

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeFillEllipse,
            FIELD_OFFSET(EmfPlusFillEllipse, RectData) + (is_int_rect ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)),
            (void **)&record);
    if (stat != Ok) return stat;
    if (inline_color)
    {
        record->Header.Flags = 0x8000;
        record->BrushId = ((GpSolidFill *)brush)->color;
    }
    else
        record->BrushId = brush_id;

    if (is_int_rect)
    {
        record->Header.Flags |= 0x4000;
        record->RectData.rect.X = (SHORT)rect->X;
        record->RectData.rect.Y = (SHORT)rect->Y;
        record->RectData.rect.Width = (SHORT)rect->Width;
        record->RectData.rect.Height = (SHORT)rect->Height;
    }
    else
        memcpy(&record->RectData.rectF, rect, sizeof(*rect));

    METAFILE_WriteRecords(metafile);
    return Ok;
}

GpStatus METAFILE_FillPie(GpMetafile *metafile, GpBrush *brush, const GpRectF *rect,
        REAL startAngle, REAL sweepAngle)
{
    BOOL is_int_rect, inline_color;
    EmfPlusFillPie *record;
    DWORD brush_id = -1;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    inline_color = brush->bt == BrushTypeSolidColor;
    if (!inline_color)
    {
        stat = METAFILE_AddBrushObject(metafile, brush, &brush_id);
        if (stat != Ok) return stat;
    }

    is_int_rect = is_integer_rect(rect);

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeFillPie,
            FIELD_OFFSET(EmfPlusFillPie, RectData) + (is_int_rect ? sizeof(EmfPlusRect) : sizeof(EmfPlusRectF)),
            (void **)&record);
    if (stat != Ok) return stat;
    if (inline_color)
    {
        record->Header.Flags = 0x8000;
        record->BrushId = ((GpSolidFill *)brush)->color;
    }
    else
        record->BrushId = brush_id;

    record->StartAngle = startAngle;
    record->SweepAngle = sweepAngle;

    if (is_int_rect)
    {
        record->Header.Flags |= 0x4000;
        record->RectData.rect.X = (SHORT)rect->X;
        record->RectData.rect.Y = (SHORT)rect->Y;
        record->RectData.rect.Width = (SHORT)rect->Width;
        record->RectData.rect.Height = (SHORT)rect->Height;
    }
    else
        memcpy(&record->RectData.rectF, rect, sizeof(*rect));

    METAFILE_WriteRecords(metafile);
    return Ok;
}

static GpStatus METAFILE_AddFontObject(GpMetafile *metafile, GDIPCONST GpFont *font, DWORD *id)
{
    EmfPlusObject *object_record;
    EmfPlusFont *font_record;
    GpStatus stat;
    INT fn_len;
    INT style;

    *id = -1;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly &&
            metafile->metafile_type != MetafileTypeEmfPlusDual)
        return Ok;

    /* The following cast is ugly, but GdipGetFontStyle does treat
       its first parameter as const. */
    stat = GdipGetFontStyle((GpFont*)font, &style);
    if (stat != Ok)
        return stat;

    fn_len = lstrlenW(font->family->FamilyName);
    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeObject,
        FIELD_OFFSET(EmfPlusObject, ObjectData.font.FamilyName[(fn_len + 1) & ~1]),
        (void**)&object_record);
    if (stat != Ok)
        return stat;

    *id = METAFILE_AddObjectId(metafile);

    object_record->Header.Flags = *id | ObjectTypeFont << 8;

    font_record = &object_record->ObjectData.font;
    font_record->Version = VERSION_MAGIC2;
    font_record->EmSize = font->emSize;
    font_record->SizeUnit = font->unit;
    font_record->FontStyleFlags = style;
    font_record->Reserved = 0;
    font_record->Length = fn_len;

    memcpy(font_record->FamilyName, font->family->FamilyName,
        fn_len * sizeof(*font->family->FamilyName));

    return Ok;
}

GpStatus METAFILE_DrawDriverString(GpMetafile *metafile, GDIPCONST UINT16 *text, INT length,
    GDIPCONST GpFont *font, GDIPCONST GpStringFormat *format, GDIPCONST GpBrush *brush,
    GDIPCONST PointF *positions, INT flags, GDIPCONST GpMatrix *matrix)
{
    DWORD brush_id;
    DWORD font_id;
    DWORD alloc_size;
    GpStatus stat;
    EmfPlusDrawDriverString *draw_string_record;
    BYTE *cursor;
    BOOL inline_color;
    BOOL include_matrix = FALSE;

    if (length <= 0)
        return InvalidParameter;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly &&
            metafile->metafile_type != MetafileTypeEmfPlusDual)
    {
        FIXME("metafile type not supported: %i\n", metafile->metafile_type);
        return NotImplemented;
    }

    stat = METAFILE_AddFontObject(metafile, font, &font_id);
    if (stat != Ok)
        return stat;

    inline_color = (brush->bt == BrushTypeSolidColor);
    if (!inline_color)
    {
        stat = METAFILE_AddBrushObject(metafile, brush, &brush_id);
        if (stat != Ok)
            return stat;
    }

    if (matrix)
    {
        BOOL identity;

        stat = GdipIsMatrixIdentity(matrix, &identity);
        if (stat != Ok)
           return stat;

        include_matrix = !identity;
    }

    alloc_size = FIELD_OFFSET(EmfPlusDrawDriverString, VariableData) +
        length * (sizeof(*text) + sizeof(*positions));

    if (include_matrix)
        alloc_size += sizeof(*matrix);

    /* Pad record to DWORD alignment. */
    alloc_size = (alloc_size + 3) & ~3;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawDriverString, alloc_size, (void**)&draw_string_record);
    if (stat != Ok)
        return stat;

    draw_string_record->Header.Flags = font_id;
    draw_string_record->DriverStringOptionsFlags = flags;
    draw_string_record->MatrixPresent = include_matrix;
    draw_string_record->GlyphCount = length;

    if (inline_color)
    {
        draw_string_record->Header.Flags |= 0x8000;
        draw_string_record->brush.Color = ((GpSolidFill*)brush)->color;
    }
    else
        draw_string_record->brush.BrushId = brush_id;

    cursor = &draw_string_record->VariableData[0];

    memcpy(cursor, text, length * sizeof(*text));
    cursor += length * sizeof(*text);

    if (flags & DriverStringOptionsRealizedAdvance)
    {
        static BOOL fixme_written = FALSE;

        /* Native never writes DriverStringOptionsRealizedAdvance. Instead,
           in the case of RealizedAdvance, each glyph position is computed
           and serialized.

           While native GDI+ is capable of playing back metafiles with this
           flag set, it is possible that some application might rely on
           metafiles produced from GDI+ not setting this flag. Ideally we
           would also compute the position of each glyph here, serialize those
           values, and not set DriverStringOptionsRealizedAdvance. */
        if (!fixme_written)
        {
            fixme_written = TRUE;
            FIXME("serializing RealizedAdvance flag and single GlyphPos with padding\n");
        }

        *((PointF*)cursor) = *positions;
    }
    else
        memcpy(cursor, positions, length * sizeof(*positions));

    if (include_matrix)
    {
        cursor += length * sizeof(*positions);
        memcpy(cursor, matrix, sizeof(*matrix));
    }

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_FillRegion(GpMetafile* metafile, GpBrush* brush, GpRegion* region)
{
    GpStatus stat;
    DWORD brush_id;
    DWORD region_id;
    EmfPlusFillRegion *fill_region_record;
    BOOL inline_color;

    if (metafile->metafile_type != MetafileTypeEmfPlusOnly &&
            metafile->metafile_type != MetafileTypeEmfPlusDual)
    {
        FIXME("metafile type not supported: %i\n", metafile->metafile_type);
        return NotImplemented;
    }

    inline_color = (brush->bt == BrushTypeSolidColor);
    if (!inline_color)
    {
        stat = METAFILE_AddBrushObject(metafile, brush, &brush_id);
        if (stat != Ok)
            return stat;
    }

    stat = METAFILE_AddRegionObject(metafile, region, &region_id);
    if (stat != Ok)
        return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeFillRegion, sizeof(EmfPlusFillRegion),
        (void**)&fill_region_record);
    if (stat != Ok)
        return stat;

    fill_region_record->Header.Flags = region_id;

    if (inline_color)
    {
        fill_region_record->Header.Flags |= 0x8000;
        fill_region_record->data.Color = ((GpSolidFill*)brush)->color;
    }
    else
        fill_region_record->data.BrushId = brush_id;

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_DrawRectangles(GpMetafile *metafile, GpPen *pen, const GpRectF *rects, INT count)
{
    EmfPlusDrawRects *record;
    GpStatus stat;
    BOOL integer_rects = TRUE;
    DWORD pen_id;
    int i;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddPenObject(metafile, pen, &pen_id);
    if (stat != Ok) return stat;

    for (i = 0; i < count; i++)
    {
        if (!is_integer_rect(&rects[i]))
        {
            integer_rects = FALSE;
            break;
        }
    }

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawRects, FIELD_OFFSET(EmfPlusDrawRects, RectData) +
        count * (integer_rects ? sizeof(record->RectData.rect) : sizeof(record->RectData.rectF)),
        (void **)&record);
    if (stat != Ok)
        return stat;

    record->Header.Flags = pen_id;
    if (integer_rects)
        record->Header.Flags |= 0x4000;
    record->Count = count;

    if (integer_rects)
    {
        for (i = 0; i < count; i++)
        {
            record->RectData.rect[i].X = (SHORT)rects[i].X;
            record->RectData.rect[i].Y = (SHORT)rects[i].Y;
            record->RectData.rect[i].Width = (SHORT)rects[i].Width;
            record->RectData.rect[i].Height = (SHORT)rects[i].Height;
        }
    }
    else
        memcpy(record->RectData.rectF, rects, sizeof(*rects) * count);

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_DrawArc(GpMetafile *metafile, GpPen *pen, const GpRectF *rect, REAL startAngle, REAL sweepAngle)
{
    EmfPlusDrawArc *record;
    GpStatus stat;
    BOOL integer_rect;
    DWORD pen_id;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddPenObject(metafile, pen, &pen_id);
    if (stat != Ok) return stat;

    integer_rect = is_integer_rect(rect);

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeDrawArc, FIELD_OFFSET(EmfPlusDrawArc, RectData) +
        (integer_rect ? sizeof(record->RectData.rect) : sizeof(record->RectData.rectF)),
        (void **)&record);
    if (stat != Ok)
        return stat;

    record->Header.Flags = pen_id;
    if (integer_rect)
        record->Header.Flags |= 0x4000;
    record->StartAngle = startAngle;
    record->SweepAngle = sweepAngle;

    if (integer_rect)
    {
        record->RectData.rect.X = (SHORT)rect->X;
        record->RectData.rect.Y = (SHORT)rect->Y;
        record->RectData.rect.Width = (SHORT)rect->Width;
        record->RectData.rect.Height = (SHORT)rect->Height;
    }
    else
        memcpy(&record->RectData.rectF, rect, sizeof(*rect));

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_OffsetClip(GpMetafile *metafile, REAL dx, REAL dy)
{
    EmfPlusOffsetClip *record;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeOffsetClip,
        sizeof(*record), (void **)&record);
    if (stat != Ok)
        return stat;

    record->dx = dx;
    record->dy = dy;

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_ResetClip(GpMetafile *metafile)
{
    EmfPlusRecordHeader *record;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeResetClip,
        sizeof(*record), (void **)&record);
    if (stat != Ok)
        return stat;

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_SetClipPath(GpMetafile *metafile, GpPath *path, CombineMode mode)
{
    EmfPlusRecordHeader *record;
    DWORD path_id;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AddPathObject(metafile, path, &path_id);
    if (stat != Ok) return stat;

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetClipPath,
        sizeof(*record), (void **)&record);
    if (stat != Ok)
        return stat;

    record->Flags = ((mode & 0xf) << 8) | path_id;

    METAFILE_WriteRecords(metafile);

    return Ok;
}

GpStatus METAFILE_SetRenderingOrigin(GpMetafile *metafile, INT x, INT y)
{
    EmfPlusSetRenderingOrigin *record;
    GpStatus stat;

    if (metafile->metafile_type == MetafileTypeEmf)
    {
        FIXME("stub!\n");
        return NotImplemented;
    }

    stat = METAFILE_AllocateRecord(metafile, EmfPlusRecordTypeSetRenderingOrigin,
        sizeof(*record), (void **)&record);
    if (stat != Ok)
        return stat;

    record->x = x;
    record->y = y;

    METAFILE_WriteRecords(metafile);

    return Ok;
}
