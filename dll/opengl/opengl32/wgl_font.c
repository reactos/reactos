/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 1999 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
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

#include "opengl32.h"

#include <math.h>

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

/***********************************************************************
 *		wglUseFontBitmaps_common
 */
static BOOL wglUseFontBitmaps_common( HDC hdc, DWORD first, DWORD count, DWORD listBase, BOOL unicode )
{
    const GLDISPATCHTABLE * funcs = IntGetCurrentDispatchTable();
    GLYPHMETRICS gm;
    unsigned int glyph, size = 0;
    void *bitmap = NULL, *gl_bitmap = NULL;
    int org_alignment;
    BOOL ret = TRUE;

    funcs->GetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
    funcs->PixelStorei(GL_UNPACK_ALIGNMENT, 4);

    for (glyph = first; glyph < first + count; glyph++) {
        static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
        unsigned int needed_size, height, width, width_int;

        if (unicode)
            needed_size = GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);
        else
            needed_size = GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);

        TRACE("Glyph: %3d / List: %d size %d\n", glyph, listBase, needed_size);
        if (needed_size == GDI_ERROR) {
            ret = FALSE;
            break;
        }

        if (needed_size > size) {
            size = needed_size;
            HeapFree(GetProcessHeap(), 0, bitmap);
            HeapFree(GetProcessHeap(), 0, gl_bitmap);
            bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            gl_bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        }
        if (unicode)
            ret = (GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &identity) != GDI_ERROR);
        else
            ret = (GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &identity) != GDI_ERROR);
        if (!ret) break;

        if (TRACE_ON(wgl)) {
            unsigned int bitmask;
            unsigned char *bitmap_ = bitmap;

            TRACE("  - bbox: %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
            TRACE("  - origin: (%d, %d)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
            TRACE("  - increment: %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
            if (needed_size != 0) {
                TRACE("  - bitmap:\n");
                for (height = 0; height < gm.gmBlackBoxY; height++) {
                    TRACE("      ");
                    for (width = 0, bitmask = 0x80; width < gm.gmBlackBoxX; width++, bitmask >>= 1) {
                        if (bitmask == 0) {
                            bitmap_ += 1;
                            bitmask = 0x80;
                        }
                        if (*bitmap_ & bitmask)
                            TRACE("*");
                        else
                            TRACE(" ");
                    }
                    bitmap_ += (4 - ((UINT_PTR)bitmap_ & 0x03));
                    TRACE("\n");
                }
            }
        }

         /* In OpenGL, the bitmap is drawn from the bottom to the top... So we need to invert the
         * glyph for it to be drawn properly.
         */
        if (needed_size != 0) {
            width_int = (gm.gmBlackBoxX + 31) / 32;
            for (height = 0; height < gm.gmBlackBoxY; height++) {
                for (width = 0; width < width_int; width++) {
                    ((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
                    ((int *) bitmap)[height * width_int + width];
                }
            }
        }

        funcs->NewList(listBase++, GL_COMPILE);
        if (needed_size != 0) {
            funcs->Bitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
                    0 - gm.gmptGlyphOrigin.x, (int) gm.gmBlackBoxY - gm.gmptGlyphOrigin.y,
                    gm.gmCellIncX, gm.gmCellIncY,
                    gl_bitmap);
        } else {
            /* This is the case of 'empty' glyphs like the space character */
            funcs->Bitmap(0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL);
        }
        funcs->EndList();
    }

    funcs->PixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
    HeapFree(GetProcessHeap(), 0, bitmap);
    HeapFree(GetProcessHeap(), 0, gl_bitmap);
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, FALSE );
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, TRUE );
}

/* FIXME: should probably have a glu.h header */

typedef struct GLUtesselator GLUtesselator;
typedef void (WINAPI *_GLUfuncptr)(void);

#define GLU_TESS_BEGIN  100100
#define GLU_TESS_VERTEX 100101
#define GLU_TESS_END    100102

static GLUtesselator * (WINAPI *pgluNewTess)(void);
static void (WINAPI *pgluDeleteTess)(GLUtesselator *tess);
static void (WINAPI *pgluTessNormal)(GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z);
static void (WINAPI *pgluTessBeginPolygon)(GLUtesselator *tess, void *polygon_data);
static void (WINAPI *pgluTessEndPolygon)(GLUtesselator *tess);
static void (WINAPI *pgluTessCallback)(GLUtesselator *tess, GLenum which, _GLUfuncptr fn);
static void (WINAPI *pgluTessBeginContour)(GLUtesselator *tess);
static void (WINAPI *pgluTessEndContour)(GLUtesselator *tess);
static void (WINAPI *pgluTessVertex)(GLUtesselator *tess, GLdouble *location, GLvoid* data);

static HMODULE load_libglu(void)
{
    static const WCHAR glu32W[] = {'g','l','u','3','2','.','d','l','l',0};
    static int already_loaded;
    static HMODULE module;

    if (already_loaded) return module;
    already_loaded = 1;

    TRACE("Trying to load GLU library\n");
    module = LoadLibraryW( glu32W );
    if (!module)
    {
        WARN("Failed to load glu32\n");
        return NULL;
    }
#define LOAD_FUNCPTR(f) p##f = (void *)GetProcAddress( module, #f )
    LOAD_FUNCPTR(gluNewTess);
    LOAD_FUNCPTR(gluDeleteTess);
    LOAD_FUNCPTR(gluTessBeginContour);
    LOAD_FUNCPTR(gluTessNormal);
    LOAD_FUNCPTR(gluTessBeginPolygon);
    LOAD_FUNCPTR(gluTessCallback);
    LOAD_FUNCPTR(gluTessEndContour);
    LOAD_FUNCPTR(gluTessEndPolygon);
    LOAD_FUNCPTR(gluTessVertex);
#undef LOAD_FUNCPTR
    return module;
}

static void fixed_to_double(POINTFX fixed, UINT em_size, GLdouble vertex[3])
{
    vertex[0] = (fixed.x.value + (GLdouble)fixed.x.fract / (1 << 16)) / em_size;  
    vertex[1] = (fixed.y.value + (GLdouble)fixed.y.fract / (1 << 16)) / em_size;  
    vertex[2] = 0.0;
}

static void WINAPI tess_callback_vertex(GLvoid *vertex)
{
    const GLDISPATCHTABLE * funcs = IntGetCurrentDispatchTable();
    GLdouble *dbl = vertex;
    TRACE("%f, %f, %f\n", dbl[0], dbl[1], dbl[2]);
    funcs->Vertex3dv(vertex);
}

static void WINAPI tess_callback_begin(GLenum which)
{
    const GLDISPATCHTABLE * funcs = IntGetCurrentDispatchTable();
    TRACE("%d\n", which);
    funcs->Begin(which);
}

static void WINAPI tess_callback_end(void)
{
    const GLDISPATCHTABLE * funcs = IntGetCurrentDispatchTable();
    TRACE("\n");
    funcs->End();
}

typedef struct _bezier_vector {
    GLdouble x;
    GLdouble y;
} bezier_vector;

static double bezier_deviation_squared(const bezier_vector *p)
{
    bezier_vector deviation;
    bezier_vector vertex;
    bezier_vector base;
    double base_length;
    double dot;

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4 - p[0].x;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4 - p[0].y;

    base.x = p[2].x - p[0].x;
    base.y = p[2].y - p[0].y;

    base_length = sqrt(base.x*base.x + base.y*base.y);
    base.x /= base_length;
    base.y /= base_length;

    dot = base.x*vertex.x + base.y*vertex.y;
    dot = min(max(dot, 0.0), base_length);
    base.x *= dot;
    base.y *= dot;

    deviation.x = vertex.x-base.x;
    deviation.y = vertex.y-base.y;

    return deviation.x*deviation.x + deviation.y*deviation.y;
}

static int bezier_approximate(const bezier_vector *p, bezier_vector *points, FLOAT deviation)
{
    bezier_vector first_curve[3];
    bezier_vector second_curve[3];
    bezier_vector vertex;
    int total_vertices;

    if(bezier_deviation_squared(p) <= deviation*deviation)
    {
        if(points)
            *points = p[2];
        return 1;
    }

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4;

    first_curve[0] = p[0];
    first_curve[1].x = (p[0].x + p[1].x)/2;
    first_curve[1].y = (p[0].y + p[1].y)/2;
    first_curve[2] = vertex;

    second_curve[0] = vertex;
    second_curve[1].x = (p[2].x + p[1].x)/2;
    second_curve[1].y = (p[2].y + p[1].y)/2;
    second_curve[2] = p[2];

    total_vertices = bezier_approximate(first_curve, points, deviation);
    if(points)
        points += total_vertices;
    total_vertices += bezier_approximate(second_curve, points, deviation);
    return total_vertices;
}

/***********************************************************************
 *		wglUseFontOutlines_common
 */
static BOOL wglUseFontOutlines_common(HDC hdc,
                                      DWORD first,
                                      DWORD count,
                                      DWORD listBase,
                                      FLOAT deviation,
                                      FLOAT extrusion,
                                      int format,
                                      LPGLYPHMETRICSFLOAT lpgmf,
                                      BOOL unicode)
{
    const GLDISPATCHTABLE * funcs = IntGetCurrentDispatchTable();
    UINT glyph;
    const MAT2 identity = {{0,1},{0,0},{0,0},{0,1}};
    GLUtesselator *tess = NULL;
    LOGFONTW lf;
    HFONT old_font, unscaled_font;
    UINT em_size = 1024;
    RECT rc;

    TRACE("(%p, %d, %d, %d, %f, %f, %d, %p, %s)\n", hdc, first, count,
          listBase, deviation, extrusion, format, lpgmf, unicode ? "W" : "A");

    if(deviation <= 0.0)
        deviation = 1.0/em_size;

    if(format == WGL_FONT_POLYGONS)
    {
        if (!load_libglu())
        {
            ERR("glu32 is required for this function but isn't available\n");
            return FALSE;
        }

        tess = pgluNewTess();
        if(!tess) return FALSE;
        pgluTessCallback(tess, GLU_TESS_VERTEX, (_GLUfuncptr)tess_callback_vertex);
        pgluTessCallback(tess, GLU_TESS_BEGIN, (_GLUfuncptr)tess_callback_begin);
        pgluTessCallback(tess, GLU_TESS_END, tess_callback_end);
    }

    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);
    rc.left = rc.right = rc.bottom = 0;
    rc.top = em_size;
    DPtoLP(hdc, (POINT*)&rc, 2);
    lf.lfHeight = -abs(rc.top - rc.bottom);
    lf.lfOrientation = lf.lfEscapement = 0;
    unscaled_font = CreateFontIndirectW(&lf);
    old_font = SelectObject(hdc, unscaled_font);

    for (glyph = first; glyph < first + count; glyph++)
    {
        DWORD needed;
        GLYPHMETRICS gm;
        BYTE *buf;
        TTPOLYGONHEADER *pph;
        TTPOLYCURVE *ppc;
        GLdouble *vertices = NULL, *vertices_temp = NULL;
        int vertex_total = -1;

        if(unicode)
            needed = GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);
        else
            needed = GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);

        if(needed == GDI_ERROR)
            goto error;

        buf = HeapAlloc(GetProcessHeap(), 0, needed);

        if(!buf)
            goto error;

        if(unicode)
            GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);
        else
            GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);

        TRACE("glyph %d\n", glyph);

        if(lpgmf)
        {
            lpgmf->gmfBlackBoxX = (float)gm.gmBlackBoxX / em_size;
            lpgmf->gmfBlackBoxY = (float)gm.gmBlackBoxY / em_size;
            lpgmf->gmfptGlyphOrigin.x = (float)gm.gmptGlyphOrigin.x / em_size;
            lpgmf->gmfptGlyphOrigin.y = (float)gm.gmptGlyphOrigin.y / em_size;
            lpgmf->gmfCellIncX = (float)gm.gmCellIncX / em_size;
            lpgmf->gmfCellIncY = (float)gm.gmCellIncY / em_size;

            TRACE("%fx%f at %f,%f inc %f,%f\n", lpgmf->gmfBlackBoxX, lpgmf->gmfBlackBoxY,
                  lpgmf->gmfptGlyphOrigin.x, lpgmf->gmfptGlyphOrigin.y, lpgmf->gmfCellIncX, lpgmf->gmfCellIncY); 
            lpgmf++;
        }

        funcs->NewList(listBase++, GL_COMPILE);
        funcs->FrontFace(GL_CCW);
        if(format == WGL_FONT_POLYGONS)
        {
            funcs->Normal3d(0.0, 0.0, 1.0);
            pgluTessNormal(tess, 0, 0, 1);
            pgluTessBeginPolygon(tess, NULL);
        }

        while(!vertices)
        {
            if(vertex_total != -1)
                vertices_temp = vertices = HeapAlloc(GetProcessHeap(), 0, vertex_total * 3 * sizeof(GLdouble));
            vertex_total = 0;

            pph = (TTPOLYGONHEADER*)buf;
            while((BYTE*)pph < buf + needed)
            {
                GLdouble previous[3];
                fixed_to_double(pph->pfxStart, em_size, previous);

                if(vertices)
                    TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

                if(format == WGL_FONT_POLYGONS)
                    pgluTessBeginContour(tess);
                else
                    funcs->Begin(GL_LINE_LOOP);

                if(vertices)
                {
                    fixed_to_double(pph->pfxStart, em_size, vertices);
                    if(format == WGL_FONT_POLYGONS)
                        pgluTessVertex(tess, vertices, vertices);
                    else
                        funcs->Vertex3d(vertices[0], vertices[1], vertices[2]);
                    vertices += 3;
                }
                vertex_total++;

                ppc = (TTPOLYCURVE*)((char*)pph + sizeof(*pph));
                while((char*)ppc < (char*)pph + pph->cb)
                {
                    int i, j;
                    int num;

                    switch(ppc->wType) {
                    case TT_PRIM_LINE:
                        for(i = 0; i < ppc->cpfx; i++)
                        {
                            if(vertices)
                            {
                                TRACE("\t\tline to %d, %d\n",
                                      ppc->apfx[i].x.value, ppc->apfx[i].y.value);
                                fixed_to_double(ppc->apfx[i], em_size, vertices);
                                if(format == WGL_FONT_POLYGONS)
                                    pgluTessVertex(tess, vertices, vertices);
                                else
                                    funcs->Vertex3d(vertices[0], vertices[1], vertices[2]);
                                vertices += 3;
                            }
                            fixed_to_double(ppc->apfx[i], em_size, previous);
                            vertex_total++;
                        }
                        break;

                    case TT_PRIM_QSPLINE:
                        for(i = 0; i < ppc->cpfx-1; i++)
                        {
                            bezier_vector curve[3];
                            bezier_vector *points;
                            GLdouble curve_vertex[3];

                            if(vertices)
                                TRACE("\t\tcurve  %d,%d %d,%d\n",
                                      ppc->apfx[i].x.value,     ppc->apfx[i].y.value,
                                      ppc->apfx[i + 1].x.value, ppc->apfx[i + 1].y.value);

                            curve[0].x = previous[0];
                            curve[0].y = previous[1];
                            fixed_to_double(ppc->apfx[i], em_size, curve_vertex);
                            curve[1].x = curve_vertex[0];
                            curve[1].y = curve_vertex[1];
                            fixed_to_double(ppc->apfx[i + 1], em_size, curve_vertex);
                            curve[2].x = curve_vertex[0];
                            curve[2].y = curve_vertex[1];
                            if(i < ppc->cpfx-2)
                            {
                                curve[2].x = (curve[1].x + curve[2].x)/2;
                                curve[2].y = (curve[1].y + curve[2].y)/2;
                            }
                            num = bezier_approximate(curve, NULL, deviation);
                            points = HeapAlloc(GetProcessHeap(), 0, num*sizeof(bezier_vector));
                            num = bezier_approximate(curve, points, deviation);
                            vertex_total += num;
                            if(vertices)
                            {
                                for(j=0; j<num; j++)
                                {
                                    TRACE("\t\t\tvertex at %f,%f\n", points[j].x, points[j].y);
                                    vertices[0] = points[j].x;
                                    vertices[1] = points[j].y;
                                    vertices[2] = 0.0;
                                    if(format == WGL_FONT_POLYGONS)
                                        pgluTessVertex(tess, vertices, vertices);
                                    else
                                        funcs->Vertex3d(vertices[0], vertices[1], vertices[2]);
                                    vertices += 3;
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, points);
                            previous[0] = curve[2].x;
                            previous[1] = curve[2].y;
                        }
                        break;
                    default:
                        ERR("\t\tcurve type = %d\n", ppc->wType);
                        if(format == WGL_FONT_POLYGONS)
                            pgluTessEndContour(tess);
                        else
                            funcs->End();
                        goto error_in_list;
                    }

                    ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                         (ppc->cpfx - 1) * sizeof(POINTFX));
                }
                if(format == WGL_FONT_POLYGONS)
                    pgluTessEndContour(tess);
                else
                    funcs->End();
                pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
            }
        }

error_in_list:
        if(format == WGL_FONT_POLYGONS)
            pgluTessEndPolygon(tess);
        funcs->Translated((GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0);
        funcs->EndList();

        HeapFree(GetProcessHeap(), 0, buf);

        if(vertices_temp)
            HeapFree(GetProcessHeap(), 0, vertices_temp);
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    if(format == WGL_FONT_POLYGONS)
        pgluDeleteTess(tess);
    return TRUE;

}

/***********************************************************************
 *		wglUseFontOutlinesA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesA(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, FALSE);
}

/***********************************************************************
 *		wglUseFontOutlinesW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesW(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, TRUE);
}
