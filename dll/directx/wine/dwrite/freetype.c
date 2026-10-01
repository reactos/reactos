/*
 *    FreeType integration
 *
 * Copyright 2014-2017 Nikolay Sivov for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <sys/types.h>
#include <dlfcn.h>

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_GLYPH_H
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SIZES_H
#endif /* HAVE_FT2BUILD_H */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "wine/debug.h"
#include "unixlib.h"

#include "dwrite_private.h"

#ifdef HAVE_FREETYPE

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

static void *ft_handle = NULL;
static FT_Library library = 0;
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Activate_Size);
MAKE_FUNCPTR(FT_Done_Face);
MAKE_FUNCPTR(FT_Done_FreeType);
MAKE_FUNCPTR(FT_Done_Glyph);
MAKE_FUNCPTR(FT_Done_Size);
MAKE_FUNCPTR(FT_Get_First_Char);
MAKE_FUNCPTR(FT_Get_Glyph);
MAKE_FUNCPTR(FT_Get_Kerning);
MAKE_FUNCPTR(FT_Get_Sfnt_Table);
MAKE_FUNCPTR(FT_Glyph_Copy);
MAKE_FUNCPTR(FT_Glyph_Get_CBox);
MAKE_FUNCPTR(FT_Glyph_Transform);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_Matrix_Multiply);
MAKE_FUNCPTR(FT_MulDiv);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_New_Size);
MAKE_FUNCPTR(FT_Outline_Copy);
MAKE_FUNCPTR(FT_Outline_Decompose);
MAKE_FUNCPTR(FT_Outline_Done);
MAKE_FUNCPTR(FT_Outline_Embolden);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_New);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FT_Set_Pixel_Sizes);
#undef MAKE_FUNCPTR
static FT_Error (*pFT_Outline_EmboldenXY)(FT_Outline *, FT_Pos, FT_Pos);

#define FaceFromObject(o) ((FT_Face)(ULONG_PTR)(o))

static FT_Size freetype_set_face_size(FT_Face face, FT_UInt emsize)
{
    FT_Size size;

    if (pFT_New_Size(face, &size)) return NULL;

    pFT_Activate_Size(size);

    if (pFT_Set_Pixel_Sizes(face, emsize, emsize))
    {
        pFT_Done_Size(size);
        return NULL;
    }

    return size;
}

static BOOL freetype_glyph_has_contours(FT_Face face)
{
    return face->glyph->format == FT_GLYPH_FORMAT_OUTLINE && face->glyph->outline.n_contours;
}

static NTSTATUS process_attach(void *args)
{
    FT_Version_t FT_Version;

    ft_handle = dlopen(SONAME_LIBFREETYPE, RTLD_NOW);
    if (!ft_handle)
    {
        WINE_MESSAGE("Wine cannot find the FreeType font library.\n");
        return STATUS_DLL_NOT_FOUND;
    }

#define LOAD_FUNCPTR(f) if((p##f = dlsym(ft_handle, #f)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
    LOAD_FUNCPTR(FT_Activate_Size)
    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Done_FreeType)
    LOAD_FUNCPTR(FT_Done_Glyph)
    LOAD_FUNCPTR(FT_Done_Size)
    LOAD_FUNCPTR(FT_Get_First_Char)
    LOAD_FUNCPTR(FT_Get_Glyph)
    LOAD_FUNCPTR(FT_Get_Kerning)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Glyph_Copy)
    LOAD_FUNCPTR(FT_Glyph_Get_CBox)
    LOAD_FUNCPTR(FT_Glyph_Transform)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_Matrix_Multiply)
    LOAD_FUNCPTR(FT_MulDiv)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_New_Size)
    LOAD_FUNCPTR(FT_Outline_Copy)
    LOAD_FUNCPTR(FT_Outline_Decompose)
    LOAD_FUNCPTR(FT_Outline_Done)
    LOAD_FUNCPTR(FT_Outline_Embolden)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_New)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FT_Set_Pixel_Sizes)
#undef LOAD_FUNCPTR
    pFT_Outline_EmboldenXY = dlsym(ft_handle, "FT_Outline_EmboldenXY");

    if (pFT_Init_FreeType(&library) != 0)
    {
        ERR("Can't init FreeType library\n");
        dlclose(ft_handle);
        ft_handle = NULL;
        return STATUS_UNSUCCESSFUL;
    }
    pFT_Library_Version(library, &FT_Version.major, &FT_Version.minor, &FT_Version.patch);

    TRACE("FreeType version is %d.%d.%d\n", FT_Version.major, FT_Version.minor, FT_Version.patch);
    return STATUS_SUCCESS;

sym_not_found:
    WINE_MESSAGE("Wine cannot find certain functions that it needs from FreeType library.\n");
    dlclose(ft_handle);
    ft_handle = NULL;
    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS process_detach(void *args)
{
    pFT_Done_FreeType(library);
    return STATUS_SUCCESS;
}

static NTSTATUS create_font_object(void *args)
{
    struct create_font_object_params *params = args;
    FT_Face face = NULL;
    FT_Error fterror;

    fterror = pFT_New_Memory_Face(library, params->data, params->size, params->index, &face);
    if (fterror != FT_Err_Ok)
    {
        WARN("Failed to create a face object, error %d.\n", fterror);
        return STATUS_UNSUCCESSFUL;
    }

    *params->object = (ULONG_PTR)face;

    return STATUS_SUCCESS;
}

static NTSTATUS release_font_object(void *args)
{
    struct release_font_object_params *params = args;
    pFT_Done_Face(FaceFromObject(params->object));
    return STATUS_SUCCESS;
}

static NTSTATUS get_design_glyph_metrics(void *args)
{
    struct get_design_glyph_metrics_params *params = args;
    FT_Face face = FaceFromObject(params->object);
    FT_Size size;

    if (!(size = freetype_set_face_size(face, params->upem)))
        return STATUS_UNSUCCESSFUL;

    if (!pFT_Load_Glyph(face, params->glyph, FT_LOAD_NO_SCALE))
    {
        FT_Glyph_Metrics *metrics = &face->glyph->metrics;

        params->metrics->leftSideBearing = metrics->horiBearingX;
        params->metrics->advanceWidth = metrics->horiAdvance;
        params->metrics->rightSideBearing = metrics->horiAdvance - metrics->horiBearingX - metrics->width;

        params->metrics->advanceHeight = metrics->vertAdvance;
        params->metrics->verticalOriginY = params->ascent;
        params->metrics->topSideBearing = params->ascent - metrics->horiBearingY;
        params->metrics->bottomSideBearing = metrics->vertAdvance - metrics->height - params->metrics->topSideBearing;

        /* Adjust in case of bold simulation, glyphs without contours are ignored. */
        if (params->simulations & DWRITE_FONT_SIMULATIONS_BOLD && freetype_glyph_has_contours(face))
        {
            if (params->metrics->advanceWidth)
                params->metrics->advanceWidth += (params->upem + 49) / 50;
        }
    }

    pFT_Done_Size(size);

    return STATUS_SUCCESS;
}

struct decompose_context
{
    struct dwrite_outline *outline;
    BOOL figure_started;
    BOOL move_to;     /* last call was 'move_to' */
    FT_Vector origin; /* 'pen' position from last call */
};

static inline void ft_vector_to_d2d_point(const FT_Vector *v, D2D1_POINT_2F *p)
{
    p->x = v->x / 64.0f;
    p->y = v->y / 64.0f;
}

static int dwrite_outline_push_tag(struct dwrite_outline *outline, unsigned char tag)
{
    if (outline->tags.size < outline->tags.count + 1)
        return 1;

    outline->tags.values[outline->tags.count++] = tag;

    return 0;
}

static int dwrite_outline_push_points(struct dwrite_outline *outline, const D2D1_POINT_2F *points, unsigned int count)
{
    if (outline->points.size < outline->points.count + count)
        return 1;

    memcpy(&outline->points.values[outline->points.count], points, sizeof(*points) * count);
    outline->points.count += count;

    return 0;
}

static int decompose_beginfigure(struct decompose_context *ctxt)
{
    D2D1_POINT_2F point;
    int ret;

    if (!ctxt->move_to)
        return 0;

    ft_vector_to_d2d_point(&ctxt->origin, &point);
    if ((ret = dwrite_outline_push_tag(ctxt->outline, OUTLINE_BEGIN_FIGURE))) return ret;
    if ((ret = dwrite_outline_push_points(ctxt->outline, &point, 1))) return ret;

    ctxt->figure_started = TRUE;
    ctxt->move_to = FALSE;

    return 0;
}

static int decompose_move_to(const FT_Vector *to, void *user)
{
    struct decompose_context *ctxt = (struct decompose_context *)user;
    int ret;

    if (ctxt->figure_started)
    {
        if ((ret = dwrite_outline_push_tag(ctxt->outline, OUTLINE_END_FIGURE))) return ret;
        ctxt->figure_started = FALSE;
    }

    ctxt->move_to = TRUE;
    ctxt->origin = *to;
    return 0;
}

static int decompose_line_to(const FT_Vector *to, void *user)
{
    struct decompose_context *ctxt = (struct decompose_context *)user;
    D2D1_POINT_2F point;
    int ret;

    /* Special case for empty contours, in a way freetype returns them. */
    if (ctxt->move_to && !memcmp(to, &ctxt->origin, sizeof(*to)))
        return 0;

    ft_vector_to_d2d_point(to, &point);

    if ((ret = decompose_beginfigure(ctxt))) return ret;
    if ((ret = dwrite_outline_push_points(ctxt->outline, &point, 1))) return ret;
    if ((ret = dwrite_outline_push_tag(ctxt->outline, OUTLINE_LINE))) return ret;

    ctxt->origin = *to;
    return 0;
}

static int decompose_conic_to(const FT_Vector *control, const FT_Vector *to, void *user)
{
    struct decompose_context *ctxt = (struct decompose_context *)user;
    D2D1_POINT_2F points[3];
    FT_Vector cubic[3];
    int ret;

    if ((ret = decompose_beginfigure(ctxt)))
        return ret;

    /* convert from quadratic to cubic */

    /*
       The parametric eqn for a cubic Bezier is, from PLRM:
       r(t) = at^3 + bt^2 + ct + r0
       with the control points:
       r1 = r0 + c/3
       r2 = r1 + (c + b)/3
       r3 = r0 + c + b + a

       A quadratic Bezier has the form:
       p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

       So equating powers of t leads to:
       r1 = 2/3 p1 + 1/3 p0
       r2 = 2/3 p1 + 1/3 p2
       and of course r0 = p0, r3 = p2
    */

    /* r1 = 1/3 p0 + 2/3 p1
       r2 = 1/3 p2 + 2/3 p1 */
    cubic[0].x = (2 * control->x + 1) / 3;
    cubic[0].y = (2 * control->y + 1) / 3;
    cubic[1] = cubic[0];
    cubic[0].x += (ctxt->origin.x + 1) / 3;
    cubic[0].y += (ctxt->origin.y + 1) / 3;
    cubic[1].x += (to->x + 1) / 3;
    cubic[1].y += (to->y + 1) / 3;
    cubic[2] = *to;

    ft_vector_to_d2d_point(cubic, points);
    ft_vector_to_d2d_point(cubic + 1, points + 1);
    ft_vector_to_d2d_point(cubic + 2, points + 2);
    if ((ret = dwrite_outline_push_points(ctxt->outline, points, 3))) return ret;
    if ((ret = dwrite_outline_push_tag(ctxt->outline, OUTLINE_BEZIER))) return ret;
    ctxt->origin = *to;
    return 0;
}

static int decompose_cubic_to(const FT_Vector *control1, const FT_Vector *control2,
    const FT_Vector *to, void *user)
{
    struct decompose_context *ctxt = (struct decompose_context *)user;
    D2D1_POINT_2F points[3];
    int ret;

    if ((ret = decompose_beginfigure(ctxt)))
        return ret;

    ft_vector_to_d2d_point(control1, points);
    ft_vector_to_d2d_point(control2, points + 1);
    ft_vector_to_d2d_point(to, points + 2);
    ctxt->origin = *to;

    if ((ret = dwrite_outline_push_points(ctxt->outline, points, 3))) return ret;
    if ((ret = dwrite_outline_push_tag(ctxt->outline, OUTLINE_BEZIER))) return ret;

    return 0;
}

static int decompose_outline(FT_Outline *ft_outline, struct dwrite_outline *outline)
{
    static const FT_Outline_Funcs decompose_funcs =
    {
        decompose_move_to,
        decompose_line_to,
        decompose_conic_to,
        decompose_cubic_to,
        0,
        0
    };
    struct decompose_context context = { 0 };
    int ret;

    context.outline = outline;

    ret = pFT_Outline_Decompose(ft_outline, &decompose_funcs, &context);

    if (!ret && context.figure_started)
        ret = dwrite_outline_push_tag(outline, OUTLINE_END_FIGURE);

    return ret;
}

static void embolden_glyph_outline(FT_Outline *outline, FLOAT emsize)
{
    FT_Pos strength;

    strength = pFT_MulDiv(emsize, 1 << 6, 24);
    if (pFT_Outline_EmboldenXY)
        pFT_Outline_EmboldenXY(outline, strength, 0);
    else
        pFT_Outline_Embolden(outline, strength);
}

static void embolden_glyph(FT_Glyph glyph, FLOAT emsize)
{
    FT_OutlineGlyph outline_glyph = (FT_OutlineGlyph)glyph;

    if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return;

    embolden_glyph_outline(&outline_glyph->outline, emsize);
}

static NTSTATUS get_glyph_outline(void *args)
{
    struct get_glyph_outline_params *params = args;
    FT_Face face = FaceFromObject(params->object);
    FT_Size size;

    if (!(size = freetype_set_face_size(face, params->emsize)))
        return STATUS_UNSUCCESSFUL;

    if (!pFT_Load_Glyph(face, params->glyph, FT_LOAD_NO_BITMAP))
    {
        FT_Outline *ft_outline = &face->glyph->outline;
        FT_Matrix m;

        if (params->outline->points.values)
        {
            if (params->simulations & DWRITE_FONT_SIMULATIONS_BOLD)
                embolden_glyph_outline(ft_outline, params->emsize);

            m.xx = 1 << 16;
            m.xy = params->simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE ? (1 << 16) / 3 : 0;
            m.yx = 0;
            m.yy = -(1 << 16); /* flip Y axis */

            pFT_Outline_Transform(ft_outline, &m);

            decompose_outline(ft_outline, params->outline);
        }
        else
        {
            /* Intentionally overestimate numbers to keep it simple. */
            params->outline->points.count = ft_outline->n_points * 3;
            params->outline->tags.count = ft_outline->n_points + ft_outline->n_contours * 2;
        }
    }

    pFT_Done_Size(size);

    return STATUS_SUCCESS;
}

static NTSTATUS get_glyph_count(void *args)
{
    struct get_glyph_count_params *params = args;
    FT_Face face = FaceFromObject(params->object);

    *params->count = face ? face->num_glyphs : 0;

    return STATUS_SUCCESS;
}

static inline void ft_matrix_from_matrix_2x2(const MATRIX_2X2 *m, FT_Matrix *ft_matrix)
{
    ft_matrix->xx =  m->m11 * 0x10000;
    ft_matrix->xy = -m->m21 * 0x10000;
    ft_matrix->yx = -m->m12 * 0x10000;
    ft_matrix->yy =  m->m22 * 0x10000;
}

static BOOL get_glyph_transform(unsigned int simulations, const MATRIX_2X2 *m, FT_Matrix *ret)
{
    FT_Matrix ftm;

    ret->xx = 1 << 16;
    ret->xy = 0;
    ret->yx = 0;
    ret->yy = 1 << 16;

    /* Some fonts provide mostly bitmaps and very few outlines, for example for .notdef.
       Disable transform if that's the case. */
    if (!memcmp(m, &identity_2x2, sizeof(*m)) && !simulations)
        return FALSE;

    if (simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE)
    {
        ftm.xx =  1 << 16;
        ftm.xy = (1 << 16) / 3;
        ftm.yx =  0;
        ftm.yy =  1 << 16;
        pFT_Matrix_Multiply(&ftm, ret);
    }

    ft_matrix_from_matrix_2x2(m, &ftm);
    pFT_Matrix_Multiply(&ftm, ret);

    return TRUE;
}

static NTSTATUS get_glyph_bbox(void *args)
{
    struct get_glyph_bbox_params *params = args;
    FT_Face face = FaceFromObject(params->object);
    FT_Glyph glyph = NULL;
    FT_BBox bbox = { 0 };
    BOOL needs_transform;
    FT_Matrix m;
    FT_Size size;

    SetRectEmpty(params->bbox);

    if (!(size = freetype_set_face_size(face, params->emsize)))
        return STATUS_UNSUCCESSFUL;

    needs_transform = FT_IS_SCALABLE(face) && get_glyph_transform(params->simulations, &params->m, &m);

    if (pFT_Load_Glyph(face, params->glyph, needs_transform ? FT_LOAD_NO_BITMAP : 0))
    {
        WARN("Failed to load glyph %u.\n", params->glyph);
        pFT_Done_Size(size);
        return STATUS_UNSUCCESSFUL;
    }

    pFT_Get_Glyph(face->glyph, &glyph);
    if (needs_transform)
    {
        if (params->simulations & DWRITE_FONT_SIMULATIONS_BOLD)
            embolden_glyph(glyph, params->emsize);

        /* Includes oblique and user transform. */
        pFT_Glyph_Transform(glyph, &m, NULL);
    }

    pFT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &bbox);
    pFT_Done_Glyph(glyph);
    pFT_Done_Size(size);

    /* flip Y axis */
    SetRect(params->bbox, bbox.xMin, -bbox.yMax, bbox.xMax, -bbox.yMin);

    return STATUS_SUCCESS;
}

static NTSTATUS freetype_get_aliased_glyph_bitmap(struct get_glyph_bitmap_params *params, FT_Glyph glyph)
{
    const RECT *bbox = &params->bbox;
    int width = bbox->right - bbox->left;
    int height = bbox->bottom - bbox->top;

    *params->is_1bpp = 1;

    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        FT_OutlineGlyph outline = (FT_OutlineGlyph)glyph;
        const FT_Outline *src = &outline->outline;
        FT_Bitmap ft_bitmap;
        FT_Outline copy;

        ft_bitmap.width = width;
        ft_bitmap.rows = height;
        ft_bitmap.pitch = params->pitch;
        ft_bitmap.pixel_mode = FT_PIXEL_MODE_MONO;
        ft_bitmap.buffer = params->bitmap;

        /* Note: FreeType will only set 'black' bits for us. */
        if (pFT_Outline_New(library, src->n_points, src->n_contours, &copy) == 0) {
            pFT_Outline_Copy(src, &copy);
            pFT_Outline_Translate(&copy, -bbox->left << 6, bbox->bottom << 6);
            pFT_Outline_Get_Bitmap(library, &copy, &ft_bitmap);
            pFT_Outline_Done(library, &copy);
        }
    }
    else if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
        FT_Bitmap *ft_bitmap = &((FT_BitmapGlyph)glyph)->bitmap;
        BYTE *src = ft_bitmap->buffer, *dst = params->bitmap;
        int w = min(params->pitch, (ft_bitmap->width + 7) >> 3);
        int h = min(height, ft_bitmap->rows);

        while (h--) {
            memcpy(dst, src, w);
            src += ft_bitmap->pitch;
            dst += params->pitch;
        }
    }
    else
        FIXME("format %x not handled\n", glyph->format);

    return STATUS_SUCCESS;
}

static NTSTATUS freetype_get_aa_glyph_bitmap(struct get_glyph_bitmap_params *params, FT_Glyph glyph)
{
    const RECT *bbox = &params->bbox;
    int width = bbox->right - bbox->left;
    int height = bbox->bottom - bbox->top;

    *params->is_1bpp = 0;

    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        FT_OutlineGlyph outline = (FT_OutlineGlyph)glyph;
        const FT_Outline *src = &outline->outline;
        FT_Bitmap ft_bitmap;
        FT_Outline copy;

        ft_bitmap.width = width;
        ft_bitmap.rows = height;
        ft_bitmap.pitch = params->pitch;
        ft_bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
        ft_bitmap.buffer = params->bitmap;

        /* Note: FreeType will only set 'black' bits for us. */
        if (pFT_Outline_New(library, src->n_points, src->n_contours, &copy) == 0) {
            pFT_Outline_Copy(src, &copy);
            pFT_Outline_Translate(&copy, -bbox->left << 6, bbox->bottom << 6);
            pFT_Outline_Get_Bitmap(library, &copy, &ft_bitmap);
            pFT_Outline_Done(library, &copy);
        }
    }
    else if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
        FT_Bitmap *ft_bitmap = &((FT_BitmapGlyph)glyph)->bitmap;
        BYTE *src = ft_bitmap->buffer, *dst = params->bitmap;
        int w = min(params->pitch, (ft_bitmap->width + 7) >> 3);
        int h = min(height, ft_bitmap->rows);

        while (h--) {
            memcpy(dst, src, w);
            src += ft_bitmap->pitch;
            dst += params->pitch;
        }

        *params->is_1bpp = 1;
    }
    else
    {
        FIXME("format %x not handled\n", glyph->format);
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS get_glyph_bitmap(void *args)
{
    struct get_glyph_bitmap_params *params = args;
    FT_Face face = FaceFromObject(params->object);
    BOOL needs_transform;
    BOOL ret = FALSE;
    FT_Glyph glyph;
    FT_Size size;
    FT_Matrix m;

    *params->is_1bpp = 0;

    if (!(size = freetype_set_face_size(face, params->emsize)))
        return STATUS_UNSUCCESSFUL;

    needs_transform = FT_IS_SCALABLE(face) && get_glyph_transform(params->simulations, &params->m, &m);

    if (!pFT_Load_Glyph(face, params->glyph, needs_transform ? FT_LOAD_NO_BITMAP : 0))
    {
        pFT_Get_Glyph(face->glyph, &glyph);

        if (needs_transform)
        {
            if (params->simulations & DWRITE_FONT_SIMULATIONS_BOLD)
                embolden_glyph(glyph, params->emsize);

            /* Includes oblique and user transform. */
            pFT_Glyph_Transform(glyph, &m, NULL);
        }

        if (params->mode == DWRITE_RENDERING_MODE1_ALIASED)
            ret = freetype_get_aliased_glyph_bitmap(params, glyph);
        else
            ret = freetype_get_aa_glyph_bitmap(params, glyph);

        pFT_Done_Glyph(glyph);
    }

    pFT_Done_Size(size);

    return ret;
}

static NTSTATUS get_glyph_advance(void *args)
{
    struct get_glyph_advance_params *params = args;
    FT_Face face = FaceFromObject(params->object);
    FT_Size size;

    *params->advance = 0;
    *params->has_contours = FALSE;

    if (!(size = freetype_set_face_size(face, params->emsize)))
        return STATUS_UNSUCCESSFUL;

    if (!pFT_Load_Glyph(face, params->glyph, params->mode == DWRITE_MEASURING_MODE_NATURAL ? FT_LOAD_NO_HINTING : 0))
    {
        *params->advance = face->glyph->advance.x >> 6;
        *params->has_contours = freetype_glyph_has_contours(face);
    }

    pFT_Done_Size(size);

    return STATUS_SUCCESS;
}

#else /* HAVE_FREETYPE */

static NTSTATUS process_attach(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS process_detach(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS create_font_object(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS release_font_object(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_glyph_outline(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_glyph_count(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_glyph_advance(void *args)
{
    struct get_glyph_advance_params *params = args;

    *params->has_contours = 0;
    *params->advance = 0;

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_glyph_bbox(void *args)
{
    struct get_glyph_bbox_params *params = args;
    SetRectEmpty(params->bbox);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_glyph_bitmap(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_design_glyph_metrics(void *args)
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_FREETYPE */

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    process_detach,
    create_font_object,
    release_font_object,
    get_glyph_outline,
    get_glyph_count,
    get_glyph_advance,
    get_glyph_bbox,
    get_glyph_bitmap,
    get_design_glyph_metrics,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_create_font_object(void *args)
{
    struct
    {
        PTR32 data;
        UINT64 size;
        ULONG index;
        PTR32 object;
    } const *params32 = args;
    struct create_font_object_params params =
    {
        ULongToPtr(params32->data),
        params32->size,
        params32->index,
        ULongToPtr(params32->object),
    };

    return create_font_object(&params);
}

static NTSTATUS wow64_release_font_object(void *args)
{
    struct
    {
        UINT64 object;
    } const *params32 = args;
    struct release_font_object_params params =
    {
        params32->object
    };

    return release_font_object(&params);
}

struct dwrite_outline32
{
    struct
    {
        PTR32 values;
        ULONG count;
        ULONG size;
    } tags;

    struct
    {
        PTR32 values;
        ULONG count;
        ULONG size;
    } points;
};

static NTSTATUS wow64_get_glyph_outline(void *args)
{
    struct
    {
        UINT64 object;
        ULONG simulations;
        ULONG glyph;
        float emsize;
        PTR32 outline;
    } const *params32 = args;
    struct dwrite_outline32 *outline32 = ULongToPtr(params32->outline);
    struct dwrite_outline outline =
    {
        .tags.values = ULongToPtr(outline32->tags.values),
        .tags.count = outline32->tags.count,
        .tags.size = outline32->tags.size,
        .points.values = ULongToPtr(outline32->points.values),
        .points.count = outline32->points.count,
        .points.size = outline32->points.size,
    };

    struct get_glyph_outline_params params =
    {
        params32->object,
        params32->simulations,
        params32->glyph,
        params32->emsize,
        &outline,
    };
    NTSTATUS status;

    status = get_glyph_outline(&params);
    outline32->points.count = outline.points.count;
    outline32->tags.count = outline.tags.count;
    return status;
}

static NTSTATUS wow64_get_glyph_count(void *args)
{
    struct
    {
        UINT64 object;
        PTR32 count;
    } const *params32 = args;
    struct get_glyph_count_params params =
    {
        params32->object,
        ULongToPtr(params32->count),
    };

    return get_glyph_count(&params);
}

static NTSTATUS wow64_get_glyph_advance(void *args)
{
    struct
    {
        UINT64 object;
        ULONG glyph;
        ULONG mode;
        float emsize;
        PTR32 advance;
        PTR32 has_contours;
    } const *params32 = args;
    struct get_glyph_advance_params params =
    {
        params32->object,
        params32->glyph,
        params32->mode,
        params32->emsize,
        ULongToPtr(params32->advance),
        ULongToPtr(params32->has_contours),
    };

    return get_glyph_advance(&params);
}

static NTSTATUS wow64_get_glyph_bbox(void *args)
{
    struct
    {
        UINT64 object;
        ULONG simulations;
        ULONG glyph;
        float emsize;
        MATRIX_2X2 m;
        PTR32 bbox;
    } const *params32 = args;
    struct get_glyph_bbox_params params =
    {
        params32->object,
        params32->simulations,
        params32->glyph,
        params32->emsize,
        params32->m,
        ULongToPtr(params32->bbox),
    };

    return get_glyph_bbox(&params);
}

static NTSTATUS wow64_get_glyph_bitmap(void *args)
{
    struct
    {
        UINT64 object;
        ULONG simulations;
        ULONG glyph;
        ULONG mode;
        float emsize;
        MATRIX_2X2 m;
        RECT bbox;
        int pitch;
        PTR32 bitmap;
        PTR32 is_1bpp;
    } const *params32 = args;
    struct get_glyph_bitmap_params params =
    {
        params32->object,
        params32->simulations,
        params32->glyph,
        params32->mode,
        params32->emsize,
        params32->m,
        params32->bbox,
        params32->pitch,
        ULongToPtr(params32->bitmap),
        ULongToPtr(params32->is_1bpp),
    };

    return get_glyph_bitmap(&params);
}

static NTSTATUS wow64_get_design_glyph_metrics(void *args)
{
    struct
    {
        UINT64 object;
        ULONG simulations;
        ULONG glyph;
        ULONG upem;
        ULONG ascent;
        PTR32 metrics;
    } const *params32 = args;
    struct get_design_glyph_metrics_params params =
    {
        params32->object,
        params32->simulations,
        params32->glyph,
        params32->upem,
        params32->ascent,
        ULongToPtr(params32->metrics),
    };

    return get_design_glyph_metrics(&params);
};

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    process_detach,
    wow64_create_font_object,
    wow64_release_font_object,
    wow64_get_glyph_outline,
    wow64_get_glyph_count,
    wow64_get_glyph_advance,
    wow64_get_glyph_bbox,
    wow64_get_glyph_bitmap,
    wow64_get_design_glyph_metrics,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
