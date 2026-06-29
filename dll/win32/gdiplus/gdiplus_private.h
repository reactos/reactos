/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#ifndef __WINE_GP_PRIVATE_H_
#define __WINE_GP_PRIVATE_H_

#include <math.h>
#include <stdarg.h>

#include "windef.h"
#include "wingdi.h"
#include "winbase.h"
#include "winuser.h"

#include "objbase.h"
#include "ocidl.h"
#include "wincodecsdk.h"
#include "wine/list.h"

#include "gdiplus.h"

#define GP_DEFAULT_PENSTYLE (PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER)
#define MAX_ARC_PTS (13)
#define MAX_DASHLEN (16) /* this is a limitation of gdi */
#define INCH_HIMETRIC (2540)

#define VERSION_MAGIC  0xdbc01001
#define VERSION_MAGIC2 0xdbc01002
#define VALID_MAGIC(x) (((x) & 0xfffff000) == 0xdbc01000)
#define TENSION_CONST (0.3)

#define GIF_DISPOSE_UNSPECIFIED 0
#define GIF_DISPOSE_DO_NOT_DISPOSE 1
#define GIF_DISPOSE_RESTORE_TO_BKGND 2
#define GIF_DISPOSE_RESTORE_TO_PREV 3

#define PIXELFORMATBPP(x) ((x) ? ((x) >> 8) & 255 : 24)


COLORREF ARGB2COLORREF(ARGB color);
HBITMAP ARGB2BMP(ARGB color);
extern INT arc2polybezier(GpPointF * points, REAL x1, REAL y1, REAL x2, REAL y2,
    REAL startAngle, REAL sweepAngle);
extern REAL gdiplus_atan2(REAL dy, REAL dx);
extern GpStatus hresult_to_status(HRESULT res);
extern REAL units_to_pixels(REAL units, GpUnit unit, REAL dpi, BOOL printer_display);
extern REAL pixels_to_units(REAL pixels, GpUnit unit, REAL dpi, BOOL printer_display);
extern REAL units_scale(GpUnit from, GpUnit to, REAL dpi, BOOL printer_display);

#define WineCoordinateSpaceGdiDevice ((GpCoordinateSpace)4)

extern GpStatus gdi_dc_acquire(GpGraphics *graphics, HDC *hdc);
extern void gdi_dc_release(GpGraphics *graphics, HDC hdc);
extern GpStatus gdi_transform_acquire(GpGraphics *graphics);
extern GpStatus gdi_transform_release(GpGraphics *graphics);
extern GpStatus get_graphics_transform(GpGraphics *graphics, GpCoordinateSpace dst_space,
        GpCoordinateSpace src_space, GpMatrix *matrix);
extern GpStatus gdip_transform_points(GpGraphics *graphics, GpCoordinateSpace dst_space,
        GpCoordinateSpace src_space, GpPointF *points, INT count);
void transform_properties(GpGraphics *, GDIPCONST GpMatrix *, BOOL, REAL *, REAL *, REAL *);

extern GpStatus graphics_from_image(GpImage *image, GpGraphics **graphics);
extern GpStatus encode_image_png(GpImage *image, IStream* stream, GDIPCONST EncoderParameters* params);
extern GpStatus terminate_encoder_wic(GpImage *image);

extern GpStatus METAFILE_GetGraphicsContext(GpMetafile* metafile, GpGraphics **result);
extern GpStatus METAFILE_GetDC(GpMetafile* metafile, HDC *hdc);
extern GpStatus METAFILE_ReleaseDC(GpMetafile* metafile, HDC hdc);
extern GpStatus METAFILE_GraphicsClear(GpMetafile* metafile, ARGB color);
extern GpStatus METAFILE_FillRectangles(GpMetafile* metafile, GpBrush* brush,
    GDIPCONST GpRectF* rects, INT count);
extern GpStatus METAFILE_SetClipRect(GpMetafile* metafile,
    REAL x, REAL y, REAL width, REAL height, CombineMode mode);
extern GpStatus METAFILE_SetClipRegion(GpMetafile* metafile, GpRegion* region, CombineMode mode);
extern GpStatus METAFILE_SetPageTransform(GpMetafile* metafile, GpUnit unit, REAL scale);
extern GpStatus METAFILE_SetWorldTransform(GpMetafile* metafile, GDIPCONST GpMatrix* transform);
extern GpStatus METAFILE_ScaleWorldTransform(GpMetafile* metafile, REAL sx, REAL sy, MatrixOrder order);
extern GpStatus METAFILE_MultiplyWorldTransform(GpMetafile* metafile, GDIPCONST GpMatrix* matrix, MatrixOrder order);
extern GpStatus METAFILE_RotateWorldTransform(GpMetafile* metafile, REAL angle, MatrixOrder order);
extern GpStatus METAFILE_TranslateWorldTransform(GpMetafile* metafile, REAL dx, REAL dy, MatrixOrder order);
extern GpStatus METAFILE_ResetWorldTransform(GpMetafile* metafile);
extern GpStatus METAFILE_BeginContainer(GpMetafile* metafile, GDIPCONST GpRectF *dstrect,
    GDIPCONST GpRectF *srcrect, GpUnit unit, DWORD StackIndex);
extern GpStatus METAFILE_BeginContainerNoParams(GpMetafile* metafile, DWORD StackIndex);
extern GpStatus METAFILE_EndContainer(GpMetafile* metafile, DWORD StackIndex);
extern GpStatus METAFILE_SaveGraphics(GpMetafile* metafile, DWORD StackIndex);
extern GpStatus METAFILE_RestoreGraphics(GpMetafile* metafile, DWORD StackIndex);
extern GpStatus METAFILE_GraphicsDeleted(GpMetafile* metafile);
extern GpStatus METAFILE_DrawImagePointsRect(GpMetafile* metafile, GpImage *image,
     GDIPCONST GpPointF *points, INT count, REAL srcx, REAL srcy, REAL srcwidth,
     REAL srcheight, GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes,
     DrawImageAbort callback, VOID *callbackData);
extern GpStatus METAFILE_AddSimpleProperty(GpMetafile *metafile, SHORT prop, SHORT val);
extern GpStatus METAFILE_DrawPath(GpMetafile *metafile, GpPen *pen, GpPath *path);
extern GpStatus METAFILE_FillPath(GpMetafile *metafile, GpBrush *brush, GpPath *path);
extern GpStatus METAFILE_DrawDriverString(GpMetafile *metafile, GDIPCONST UINT16 *text, INT length,
    GDIPCONST GpFont *font, GDIPCONST GpStringFormat *format, GDIPCONST GpBrush *brush,
    GDIPCONST PointF *positions, INT flags, GDIPCONST GpMatrix *matrix);
extern GpStatus METAFILE_FillRegion(GpMetafile* metafile, GpBrush* brush,
    GpRegion* region);
extern void METAFILE_Free(GpMetafile *metafile);
extern GpStatus METAFILE_DrawEllipse(GpMetafile *metafile, GpPen *pen, GpRectF *rect);
extern GpStatus METAFILE_FillEllipse(GpMetafile *metafile, GpBrush *brush, GpRectF *rect);
extern GpStatus METAFILE_DrawRectangles(GpMetafile *metafile, GpPen *pen, const GpRectF *rects, INT count);
extern GpStatus METAFILE_FillPie(GpMetafile *metafile, GpBrush *brush, const GpRectF *rect,
    REAL startAngle, REAL sweepAngle);
extern GpStatus METAFILE_DrawArc(GpMetafile *metafile, GpPen *pen, const GpRectF *rect,
    REAL startAngle, REAL sweepAngle);
extern GpStatus METAFILE_OffsetClip(GpMetafile *metafile, REAL dx, REAL dy);
extern GpStatus METAFILE_ResetClip(GpMetafile *metafile);
extern GpStatus METAFILE_SetClipPath(GpMetafile *metafile, GpPath *path, CombineMode mode);
extern GpStatus METAFILE_SetRenderingOrigin(GpMetafile *metafile, INT x, INT y);

extern void calc_curve_bezier(const GpPointF *pts, REAL tension, REAL *x1,
    REAL *y1, REAL *x2, REAL *y2);
extern void calc_curve_bezier_endp(REAL xend, REAL yend, REAL xadj, REAL yadj,
    REAL tension, REAL *x, REAL *y);

extern void get_font_hfont(GpGraphics *graphics, GDIPCONST GpFont *font,
                           GDIPCONST GpStringFormat *format, HFONT *hfont,
                           LOGFONTW *lfw_return, GDIPCONST GpMatrix *matrix);

extern void free_installed_fonts(void);

extern BOOL lengthen_path(GpPath *path, INT len);

extern GpStatus widen_flat_path_anchors(GpPath *flat_path, GpPen *pen, REAL pen_width, GpPath **anchors);

extern DWORD write_region_data(const GpRegion *region, void *data);
extern DWORD write_path_data(GpPath *path, void *data);

extern GpStatus trace_path(GpGraphics *graphics, GpPath *path);

typedef struct region_element region_element;
extern void delete_element(region_element *element);

extern GpStatus get_hatch_data(GpHatchStyle hatchstyle, const unsigned char **result);

static inline INT gdip_round(REAL x)
{
    return (INT) floorf(x + 0.5);
}

static inline INT ceilr(REAL x)
{
    return (INT) ceilf(x);
}

static inline REAL deg2rad(REAL degrees)
{
    return M_PI * degrees / 180.0;
}

static inline ARGB color_over(ARGB bg, ARGB fg)
{
    BYTE b, g, r, a;
    BYTE bg_alpha, fg_alpha;

    fg_alpha = (fg>>24)&0xff;

    if (fg_alpha == 0xff) return fg;

    if (fg_alpha == 0) return bg;

    bg_alpha = (((bg>>24)&0xff) * (0xff-fg_alpha)) / 0xff;

    if (bg_alpha == 0) return fg;

    a = bg_alpha + fg_alpha;
    b = ((bg&0xff)*bg_alpha + (fg&0xff)*fg_alpha)/a;
    g = (((bg>>8)&0xff)*bg_alpha + ((fg>>8)&0xff)*fg_alpha)/a;
    r = (((bg>>16)&0xff)*bg_alpha + ((fg>>16)&0xff)*fg_alpha)/a;

    return (a<<24)|(r<<16)|(g<<8)|b;
}

/* fg is premult, bg and return value are not */
static inline ARGB color_over_fgpremult(ARGB bg, ARGB fg)
{
    BYTE b, g, r, a;
    BYTE bg_alpha, fg_alpha;

    fg_alpha = (fg>>24)&0xff;

    if (fg_alpha == 0) return bg;

    bg_alpha = (((bg>>24)&0xff) * (0xff-fg_alpha)) / 0xff;

    a = bg_alpha + fg_alpha;
    b = ((bg&0xff)*bg_alpha + (fg&0xff)*0xff)/a;
    g = (((bg>>8)&0xff)*bg_alpha + ((fg>>8)&0xff)*0xff)/a;
    r = (((bg>>16)&0xff)*bg_alpha + ((fg>>16)&0xff)*0xff)/a;

    return (a<<24)|(r<<16)|(g<<8)|b;
}

extern const char *debugstr_rectf(const RectF* rc);

extern const char *debugstr_pointf(const PointF* pt);

extern const char *debugstr_matrix(const GpMatrix* matrix);

extern void convert_32bppARGB_to_32bppPARGB(UINT width, UINT height,
    BYTE *dst_bits, INT dst_stride, const BYTE *src_bits, INT src_stride);

extern GpStatus convert_pixels(INT width, INT height,
    INT dst_stride, BYTE *dst_bits, PixelFormat dst_format, ColorPalette *dst_palette,
    INT src_stride, const BYTE *src_bits, PixelFormat src_format, ColorPalette *src_palette);

extern PixelFormat apply_image_attributes(const GpImageAttributes *attributes, LPBYTE data,
    UINT width, UINT height, INT stride, ColorAdjustType type, PixelFormat fmt);

struct GpMatrix{
    REAL matrix[6];
};

struct GpPen{
    UINT style;
    GpUnit unit;
    REAL width;
    GpLineCap endcap;
    GpLineCap startcap;
    GpDashCap dashcap;
    GpCustomLineCap *customstart;
    GpCustomLineCap *customend;
    GpLineJoin join;
    REAL miterlimit;
    GpDashStyle dash;
    REAL *dashes;
    INT numdashes;
    REAL offset;    /* dash offset */
    GpBrush *brush;
    GpPenAlignment align;
    GpMatrix transform;
    REAL *compound_array;
    INT compound_array_size;
};

struct GpGraphics{
    HDC hdc;
    HWND hwnd;
    INT hdc_refs;
    BOOL owndc;
    BOOL alpha_hdc;
    BOOL printer_display;
    GpImage *image;
    ImageType image_type;
    SmoothingMode smoothing;
    CompositingQuality compqual;
    InterpolationMode interpolation;
    PixelOffsetMode pixeloffset;
    CompositingMode compmode;
    TextRenderingHint texthint;
    GpUnit unit;    /* page unit */
    REAL scale;     /* page scale */
    REAL xres, yres;
    GpMatrix worldtrans; /* world transform */
    BOOL busy;      /* hdc handle obtained by GdipGetDC */
    GpRegion *clip; /* in device coords */
    UINT textcontrast; /* not used yet. get/set only */
    struct list containers;
    GraphicsContainer contid; /* last-issued container ID */
    INT origin_x, origin_y;
    INT gdi_transform_acquire_count, gdi_transform_save;
    GpMatrix gdi_transform;
    HRGN gdi_clip;
    /* For giving the caller an HDC when we technically can't: */
    HBITMAP temp_hbitmap;
    int temp_hbitmap_width;
    int temp_hbitmap_height;
    BYTE *temp_bits;
    HDC temp_hdc;
};

struct GpBrush{
    GpBrushType bt;
};

struct GpHatch{
    GpBrush brush;
    GpHatchStyle hatchstyle;
    ARGB forecol;
    ARGB backcol;
};

struct GpSolidFill{
    GpBrush brush;
    ARGB color;
};

struct GpPathGradient{
    GpBrush brush;
    GpPath* path;
    ARGB centercolor;
    GpWrapMode wrap;
    BOOL gamma;
    GpPointF center;
    GpPointF focus;
    REAL* blendfac;  /* blend factors */
    REAL* blendpos;  /* blend positions */
    INT blendcount;
    ARGB *surroundcolors;
    INT surroundcolorcount;
    ARGB* pblendcolor; /* preset blend colors */
    REAL* pblendpos; /* preset blend positions */
    INT pblendcount;
    GpMatrix transform;
};

struct GpLineGradient{
    GpBrush brush;
    ARGB startcolor;
    ARGB endcolor;
    RectF rect;
    GpWrapMode wrap;
    BOOL gamma;
    REAL* blendfac;  /* blend factors */
    REAL* blendpos;  /* blend positions */
    INT blendcount;
    ARGB* pblendcolor; /* preset blend colors */
    REAL* pblendpos; /* preset blend positions */
    INT pblendcount;
    GpMatrix transform;
};

struct GpTexture{
    GpBrush brush;
    GpMatrix transform;
    GpImage *image;
    GpImageAttributes *imageattributes;
    BYTE *bitmap_bits; /* image bits converted to ARGB and run through imageattributes */
};

struct GpPath{
    GpFillMode fill;
    GpPathData pathdata;
    BOOL newfigure; /* whether the next drawing action starts a new figure */
    INT datalen; /* size of the arrays in pathdata */
};

struct GpPathIterator{
    GpPathData pathdata;
    INT subpath_pos;    /* for NextSubpath methods */
    INT marker_pos;     /* for NextMarker methods */
    INT pathtype_pos;   /* for NextPathType methods */
};

struct GpCustomLineCap{
    CustomLineCapType type;
    GpPathData pathdata;
    BOOL fill;      /* TRUE for fill, FALSE for stroke */
    GpLineCap basecap;  /* cap used together with customLineCap */
    REAL inset;      /* distance between line end and cap beginning */
    GpLineCap strokeStartCap;
    GpLineCap strokeEndCap;
    GpLineJoin join; /* joins used for drawing custom cap*/
    REAL scale;
};

struct GpAdjustableArrowCap{
    GpCustomLineCap cap;
    REAL middle_inset;
    REAL height;
    REAL width;
};

typedef enum EffectType {
    NoneEffect,
    BlurEffect,
    SharpenEffect,
    TintEffect,
    RedEyeCorrectionEffect,
    ColorMatrixEffect,
    ColorLUTEffect,
    BrightnessContrastEffect,
    HueSaturationLightnessEffect,
    ColorBalanceEffect,
    LevelsEffect,
    ColorCurveEffect,
} EffectType;

typedef struct CGpEffect{
    EffectType type;
} CGpEffect;

struct GpImage{
    IWICBitmapDecoder *decoder;
    IWICBitmapEncoder *encoder;
    ImageType type;
    GUID format;
    UINT flags;
    UINT frame_count, current_frame;
    ColorPalette *palette;
    REAL xres, yres;
    SRWLOCK lock;
};

#define EmfPlusObjectTableSize 64

typedef enum EmfPlusObjectType
{
    ObjectTypeInvalid,
    ObjectTypeBrush,
    ObjectTypePen,
    ObjectTypePath,
    ObjectTypeRegion,
    ObjectTypeImage,
    ObjectTypeFont,
    ObjectTypeStringFormat,
    ObjectTypeImageAttributes,
    ObjectTypeCustomLineCap,
    ObjectTypeMax = ObjectTypeCustomLineCap,
} EmfPlusObjectType;

/* Deserialized EmfPlusObject record. */
struct emfplus_object {
    EmfPlusObjectType type;
    union {
        GpBrush *brush;
        GpPen *pen;
        GpPath *path;
        GpRegion *region;
        GpImage *image;
        GpFont *font;
        GpImageAttributes *image_attributes;
        void *object;
    } u;
};

struct GpMetafile{
    GpImage image;
    GpRectF bounds;
    GpUnit unit;
    MetafileType metafile_type;
    HENHMETAFILE hemf;
    int preserve_hemf; /* if true, hemf belongs to the app and should not be deleted */

    /* recording */
    HDC record_dc;
    GpGraphics *record_graphics;
    BYTE *comment_data;
    DWORD comment_data_size;
    DWORD comment_data_length;
    IStream *record_stream;
    BOOL auto_frame; /* If true, determine the frame automatically */
    GpPointF auto_frame_min, auto_frame_max;
    DWORD next_object_id;
    UINT limit_dpi;
    BOOL printer_display;
    REAL logical_dpix;
    REAL logical_dpiy;

    /* playback */
    GpGraphics *playback_graphics;
    HDC playback_dc;
    GpPointF playback_points[3];
    GpRectF src_rect;
    HANDLETABLE *handle_table;
    int handle_count;
    GpMatrix *world_transform;
    GpUnit page_unit;
    REAL page_scale;
    GpRegion *base_clip; /* clip region in device space for all metafile output */
    GpRegion *clip; /* clip region within the metafile */
    struct list containers;
    struct emfplus_object objtable[EmfPlusObjectTableSize];
};

struct GpBitmap{
    GpImage image;
    INT width;
    INT height;
    PixelFormat format;
    ImageLockMode lockmode;
    BYTE *bitmapbits;   /* pointer to the buffer we passed in BitmapLockBits */
    BYTE *bits; /* actual image bits if this is a DIB */
    INT stride; /* stride of bits if this is a DIB */
    BYTE *own_bits; /* image bits that need to be freed with this object */
    INT lockx, locky; /* X and Y coordinates of the rect when a bitmap is locked for writing. */
    IWICMetadataReader *metadata_reader; /* NULL if there is no metadata */
    UINT prop_count;
    PropertyItem *prop_item; /* cached image properties */
};

struct GpCachedBitmap{
    GpImage *image;
};

struct color_key{
    BOOL enabled;
    ARGB low;
    ARGB high;
};

struct color_matrix{
    BOOL enabled;
    ColorMatrixFlags flags;
    ColorMatrix colormatrix;
    ColorMatrix graymatrix;
};

struct color_remap_table{
    BOOL enabled;
    INT mapsize;
    ColorMap *colormap;
};

enum imageattr_noop{
    IMAGEATTR_NOOP_UNDEFINED,
    IMAGEATTR_NOOP_SET,
    IMAGEATTR_NOOP_CLEAR,
};

struct GpImageAttributes{
    WrapMode wrap;
    ARGB outside_color;
    BOOL clamp;
    struct color_key colorkeys[ColorAdjustTypeCount];
    struct color_matrix colormatrices[ColorAdjustTypeCount];
    struct color_remap_table colorremaptables[ColorAdjustTypeCount];
    BOOL gamma_enabled[ColorAdjustTypeCount];
    REAL gamma[ColorAdjustTypeCount];
    enum imageattr_noop noop[ColorAdjustTypeCount];
};

struct GpFont{
    GpFontFamily *family;
    OUTLINETEXTMETRICW otm;
    REAL emSize; /* in font units */
    Unit unit;
};

extern const struct GpStringFormat default_drawstring_format;

struct GpStringFormat{
    INT attr;
    LANGID lang;
    LANGID digitlang;
    StringAlignment align;
    StringTrimming trimming;
    HotkeyPrefix hkprefix;
    StringAlignment line_align;
    StringDigitSubstitute digitsub;
    INT tabcount;
    REAL firsttab;
    REAL *tabs;
    CharacterRange *character_ranges;
    INT range_count;
    BOOL generic_typographic;
};

extern void init_generic_string_formats(void);
extern void free_generic_string_formats(void);

struct GpFontCollection{
    GpFontFamily **FontFamilies;
    INT count;
    INT allocated;
};

struct GpFontFamily{
    WCHAR FamilyName[LF_FACESIZE];
    UINT16 em_height, ascent, descent, line_spacing; /* in font units */
    int dpi;
    BOOL installed;
    LONG ref;
};

/* internal use */
typedef enum RegionType
{
    RegionDataRect          = 0x10000000,
    RegionDataPath          = 0x10000001,
    RegionDataEmptyRect     = 0x10000002,
    RegionDataInfiniteRect  = 0x10000003,
} RegionType;

struct region_element
{
    DWORD type; /* Rectangle, Path, SpecialRectangle, or CombineMode */
    union
    {
        GpRectF rect;
        GpPath *path;
        struct
        {
            struct region_element *left;  /* the original region */
            struct region_element *right; /* what *left was combined with */
        } combine;
    } elementdata;
};

struct GpRegion{
    DWORD num_children;
    region_element node;
};

struct memory_buffer
{
    const BYTE *buffer;
    INT size, pos;
};

static inline void init_memory_buffer(struct memory_buffer *mbuf, const BYTE *buffer, INT size)
{
    mbuf->buffer = buffer;
    mbuf->size = size;
    mbuf->pos = 0;
}

static inline const void *buffer_read(struct memory_buffer *mbuf, INT size)
{
    if (mbuf->size - mbuf->pos >= size)
    {
        const void *data = mbuf->buffer + mbuf->pos;
        mbuf->pos += size;
        return data;
    }
    return NULL;
}

/* Represents a string section and the font it should use. */
struct gdip_font_link_section {
    struct list entry;
    DWORD start; /* The starting index of the string where the font applies. */
    DWORD end; /* The end index of the string. */
    GpFont *font;
};

struct gdip_font_link_info {
    GDIPCONST GpFont *base_font;
    struct list sections;
};

struct gdip_format_string_info {
    GpGraphics *graphics;
    HDC hdc;
    GDIPCONST WCHAR *string;
    INT index;
    INT length;
    struct gdip_font_link_info font_link_info;
    GDIPCONST RectF *rect;
    GDIPCONST GpStringFormat *format;
    INT lineno;
    const RectF *bounds;
    INT *underlined_indexes;
    INT underlined_index_count;
    void *user_data;
};

typedef GpStatus (*gdip_format_string_callback)(struct gdip_format_string_info *info);

GpStatus gdip_format_string(GpGraphics *graphics, HDC hdc,
    GDIPCONST WCHAR *string, INT length, GDIPCONST GpFont *font,
    GDIPCONST RectF *rect, GDIPCONST GpStringFormat *format, int ignore_empty_clip,
    gdip_format_string_callback callback, void *user_data);

void get_log_fontW(const GpFont *, GpGraphics *, LOGFONTW *);

static inline BOOL image_lock(GpImage *image)
{
    return TryAcquireSRWLockExclusive(&image->lock);
}

static inline void image_unlock(GpImage *image)
{
    ReleaseSRWLockExclusive(&image->lock);
}

static inline BOOL has_gdi_dc(GpGraphics *graphics)
{
    return graphics->hdc != NULL || graphics->owndc;
}

static inline void set_rect(GpRectF *rect, REAL x, REAL y, REAL width, REAL height)
{
    rect->X = x;
    rect->Y = y;
    rect->Width = width;
    rect->Height = height;
}

#endif
