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

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "wingdi.h"

#include "objbase.h"

#include "winreg.h"
#include "shlwapi.h"

#include "gdiplus.h"
#include "gdiplus_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

static const REAL mm_per_inch = 25.4;
static const REAL inch_per_mm = 1.0 / 25.4;
static const REAL point_per_inch = 72.0;
static const REAL inch_per_point = 1.0 / 72.0;

static Status WINAPI NotificationHook(ULONG_PTR *token)
{
    TRACE("%p\n", token);
    if(!token)
        return InvalidParameter;

    return Ok;
}

static void WINAPI NotificationUnhook(ULONG_PTR token)
{
    TRACE("%Id\n", token);
}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %ld, %p)\n", hinst, reason, reserved);

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        init_generic_string_formats();
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        free_installed_fonts();
        free_generic_string_formats();
        break;
    }
    return TRUE;
}

/*****************************************************
 *      GdiplusStartup [GDIPLUS.@]
 */
Status WINAPI GdiplusStartup(ULONG_PTR *token, const struct GdiplusStartupInput *input,
                             struct GdiplusStartupOutput *output)
{
    if(!token || !input)
        return InvalidParameter;

    TRACE("%p %p %p\n", token, input, output);
    TRACE("GdiplusStartupInput %d %p %d %d\n", input->GdiplusVersion,
          input->DebugEventCallback, input->SuppressBackgroundThread,
          input->SuppressExternalCodecs);

    if(input->GdiplusVersion < 1 || input->GdiplusVersion > 2)
        return UnsupportedGdiplusVersion;

    if(input->SuppressBackgroundThread){
        if(!output)
            return InvalidParameter;

        output->NotificationHook = NotificationHook;
        output->NotificationUnhook = NotificationUnhook;
    }

    *token = 0xdeadbeef;

    /* FIXME: DebugEventCallback ignored */

    return Ok;
}

GpStatus WINAPI GdiplusNotificationHook(ULONG_PTR *token)
{
    FIXME("%p\n", token);
    return NotificationHook(token);
}

void WINAPI GdiplusNotificationUnhook(ULONG_PTR token)
{
    FIXME("%Id\n", token);
    NotificationUnhook(token);
}

/*****************************************************
 *      GdiplusShutdown [GDIPLUS.@]
 */
ULONG WINAPI GdiplusShutdown_wrapper(ULONG_PTR token)
{
    /* Notice the slightly different prototype from the official
     * signature which forces us to use the _wrapper suffix.
     */

    /* FIXME: no object tracking */

    /* "bricksntiles" expects a return value of 0, which native
     * coincidentally gives.
     */
    return 0;
}

/*****************************************************
 *      GdipAlloc [GDIPLUS.@]
 */
void* WINGDIPAPI GdipAlloc(SIZE_T size)
{
    return calloc(1, size);
}

/*****************************************************
 *      GdipFree [GDIPLUS.@]
 */
void WINGDIPAPI GdipFree(void* ptr)
{
    free(ptr);
}

/* Calculates the bezier points needed to fill in the arc portion starting at
 * angle start and ending at end.  These two angles should be no more than 90
 * degrees from each other.  x1, y1, x2, y2 describes the bounding box (upper
 * left and width and height).  Angles must be in radians. write_first indicates
 * that the first bezier point should be written out (usually this is false).
 * pt is the array of GpPointFs that gets written to.
 **/
static void add_arc_part(GpPointF * pt, REAL x1, REAL y1, REAL x2, REAL y2,
    REAL start, REAL end, BOOL write_first)
{
    REAL center_x, center_y, rad_x, rad_y, cos_start, cos_end,
        sin_start, sin_end, a, half;
    INT i;

    rad_x = x2 / 2.0;
    rad_y = y2 / 2.0;
    center_x = x1 + rad_x;
    center_y = y1 + rad_y;

    cos_start = cos(start);
    cos_end = cos(end);
    sin_start = sin(start);
    sin_end = sin(end);

    half = (end - start) / 2.0;
    a = 4.0 / 3.0 * (1 - cos(half)) / sin(half);

    if(write_first){
        pt[0].X = cos_start;
        pt[0].Y = sin_start;
    }
    pt[1].X = cos_start - a * sin_start;
    pt[1].Y = sin_start + a * cos_start;

    pt[3].X = cos_end;
    pt[3].Y = sin_end;
    pt[2].X = cos_end + a * sin_end;
    pt[2].Y = sin_end - a * cos_end;

    /* expand the points back from the unit circle to the ellipse */
    for(i = (write_first ? 0 : 1); i < 4; i ++){
        pt[i].X = pt[i].X * rad_x + center_x;
        pt[i].Y = pt[i].Y * rad_y + center_y;
    }
}

/* We plot the curve as if it is on a circle then stretch the points.  This
 * adjusts the angles so that when we stretch the points they will end in the
 * right place. This is only complicated because atan and atan2 do not behave
 * conveniently. */
static REAL unstretch_angle(REAL angle, REAL dia_x, REAL dia_y)
{
    REAL stretched;
    INT revs_off;

    if(fabs(cos(angle)) < 0.00001 || fabs(sin(angle)) < 0.00001)
        return angle;

    stretched = gdiplus_atan2(sin(angle) / fabs(dia_y), cos(angle) / fabs(dia_x));
    revs_off = gdip_round(angle / (2.0 * M_PI)) - gdip_round(stretched / (2.0 * M_PI));
    stretched += ((REAL)revs_off) * M_PI * 2.0;
    return stretched;
}

/* Stores the bezier points that correspond to the arc in points.  If points is
 * null, just return the number of points needed to represent the arc. */
INT arc2polybezier(GpPointF * points, REAL left, REAL top, REAL width, REAL height,
    REAL start_angle, REAL sweep_angle)
{
    INT i;
    REAL partial_end_angle, end_angle;

    end_angle = deg2rad(start_angle + sweep_angle);
    start_angle = deg2rad(start_angle);

    if (width != height)
    {
        start_angle = unstretch_angle(start_angle, width, height);
        end_angle = unstretch_angle(end_angle, width, height);
    }

    for(i = 0; i < MAX_ARC_PTS - 1; i += 3){
        /* check if we've overshot the end angle */
        if( sweep_angle > 0.0 )
        {
            if (start_angle >= end_angle) break;
            partial_end_angle = min(start_angle + M_PI_2, end_angle);
        }
        else
        {
            if (start_angle <= end_angle) break;
            partial_end_angle = max(start_angle - M_PI_2, end_angle);
        }

        if (points)
            add_arc_part(&points[i], left, top, width, height, start_angle, partial_end_angle, i == 0);

        start_angle = partial_end_angle;
    }

    if (i == 0) return 0;
    else return i+1;
}

COLORREF ARGB2COLORREF(ARGB color)
{
    /*
    Packing of these color structures:
    COLORREF:   00bbggrr
    ARGB:       aarrggbb
    FIXME:doesn't handle alpha channel
    */
    return ((color & 0x0000ff) << 16) +
           (color & 0x00ff00) +
           ((color & 0xff0000) >> 16);
}

HBITMAP ARGB2BMP(ARGB color)
{
    BITMAPINFO bi;
    HBITMAP result;
    RGBQUAD *bits;
    int alpha;

    if ((color & 0xff000000) == 0xff000000) return 0;

    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = 1;
    bi.bmiHeader.biHeight = 1;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    result = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void*)&bits, NULL, 0);

    bits[0].rgbReserved = alpha = (color>>24)&0xff;
    bits[0].rgbRed = ((color>>16)&0xff)*alpha/255;
    bits[0].rgbGreen = ((color>>8)&0xff)*alpha/255;
    bits[0].rgbBlue = (color&0xff)*alpha/255;

    return result;
}

/* Like atan2, but puts angle in correct quadrant if dx is 0. */
REAL gdiplus_atan2(REAL dy, REAL dx)
{
    if((dx == 0.0) && (dy != 0.0))
        return dy > 0.0 ? M_PI_2 : -M_PI_2;

    return atan2(dy, dx);
}

GpStatus hresult_to_status(HRESULT res)
{
    switch(res){
        case S_OK:
            return Ok;
        case E_OUTOFMEMORY:
            return OutOfMemory;
        case E_INVALIDARG:
            return InvalidParameter;
        default:
            return GenericError;
    }
}

/* converts a given unit to its value in pixels */
REAL units_to_pixels(REAL units, GpUnit unit, REAL dpi, BOOL printer_display)
{
    switch (unit)
    {
    case UnitPixel:
    case UnitWorld:
        return units;
    case UnitDisplay:
        if (printer_display)
            return units * dpi * 0.01f;
        else
            return units;
    case UnitPoint:
        return units * dpi * inch_per_point;
    case UnitInch:
        return units * dpi;
    case UnitDocument:
        return units * dpi * (1.0f / 300.0f); /* Per MSDN */
    case UnitMillimeter:
        return units * dpi * inch_per_mm;
    default:
        FIXME("Unhandled unit type: %d\n", unit);
        return 0;
    }
}

/* converts value in pixels to a given unit */
REAL pixels_to_units(REAL pixels, GpUnit unit, REAL dpi, BOOL printer_display)
{
    switch (unit)
    {
    case UnitPixel:
    case UnitWorld:
        return pixels;
    case UnitDisplay:
        if (printer_display)
            return pixels * 100.0 / dpi;
        else
            return pixels;
    case UnitPoint:
        return pixels * point_per_inch / dpi;
    case UnitInch:
        return pixels / dpi;
    case UnitDocument:
        return pixels * 300.0 / dpi;
    case UnitMillimeter:
        return pixels * mm_per_inch / dpi;
    default:
        FIXME("Unhandled unit type: %d\n", unit);
        return 0;
    }
}

REAL units_scale(GpUnit from, GpUnit to, REAL dpi, BOOL printer_display)
{
    REAL pixels = units_to_pixels(1.0, from, dpi, printer_display);
    return pixels_to_units(pixels, to, dpi, printer_display);
}

/* Calculates Bezier points from cardinal spline points. */
void calc_curve_bezier(const GpPointF *pts, REAL tension, REAL *x1,
    REAL *y1, REAL *x2, REAL *y2)
{
    REAL xdiff, ydiff;

    /* calculate tangent */
    xdiff = pts[2].X - pts[0].X;
    ydiff = pts[2].Y - pts[0].Y;

    /* apply tangent to get control points */
    *x1 = pts[1].X - tension * xdiff;
    *y1 = pts[1].Y - tension * ydiff;
    *x2 = pts[1].X + tension * xdiff;
    *y2 = pts[1].Y + tension * ydiff;
}

/* Calculates Bezier points from cardinal spline endpoints. */
void calc_curve_bezier_endp(REAL xend, REAL yend, REAL xadj, REAL yadj,
    REAL tension, REAL *x, REAL *y)
{
    /* tangent at endpoints is the line from the endpoint to the adjacent point */
    *x = gdip_round(tension * (xadj - xend) + xend);
    *y = gdip_round(tension * (yadj - yend) + yend);
}

/* make sure path has enough space for len more points */
BOOL lengthen_path(GpPath *path, INT len)
{
    /* initial allocation */
    if(path->datalen == 0){
        path->datalen = len * 2;

        path->pathdata.Points = calloc(path->datalen, sizeof(PointF));
        if(!path->pathdata.Points) return FALSE;

        path->pathdata.Types = calloc(1, path->datalen);
        if(!path->pathdata.Types){
            free(path->pathdata.Points);
            return FALSE;
        }
    }
    /* reallocation, double size of arrays */
    else if(path->datalen - path->pathdata.Count < len){
        while(path->datalen - path->pathdata.Count < len)
            path->datalen *= 2;

        path->pathdata.Points = realloc(path->pathdata.Points, path->datalen * sizeof(PointF));
        if(!path->pathdata.Points) return FALSE;

        path->pathdata.Types = realloc(path->pathdata.Types, path->datalen);
        if(!path->pathdata.Types) return FALSE;
    }

    return TRUE;
}

void convert_32bppARGB_to_32bppPARGB(UINT width, UINT height,
    BYTE *dst_bits, INT dst_stride, const BYTE *src_bits, INT src_stride)
{
    INT x, y;
    for (y=0; y<height; y++)
    {
        const BYTE *src=src_bits+y*src_stride;
        BYTE *dst=dst_bits+y*dst_stride;
        for (x=0; x<width; x++)
        {
            BYTE alpha=src[3];
            *dst++ = (*src++ * alpha + 127) / 255;
            *dst++ = (*src++ * alpha + 127) / 255;
            *dst++ = (*src++ * alpha + 127) / 255;
            *dst++ = *src++;
        }
    }
}

/* recursive deletion of GpRegion nodes */
void delete_element(region_element* element)
{
    switch(element->type)
    {
        case RegionDataRect:
            break;
        case RegionDataPath:
            GdipDeletePath(element->elementdata.path);
            break;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            break;
        default:
            delete_element(element->elementdata.combine.left);
            delete_element(element->elementdata.combine.right);
            free(element->elementdata.combine.left);
            free(element->elementdata.combine.right);
            break;
    }
}

const char *debugstr_rectf(const RectF* rc)
{
    if (!rc) return "(null)";
    return wine_dbg_sprintf("(%0.2f,%0.2f,%0.2f,%0.2f)", rc->X, rc->Y, rc->Width, rc->Height);
}

const char *debugstr_pointf(const PointF* pt)
{
    if (!pt) return "(null)";
    return wine_dbg_sprintf("(%0.2f,%0.2f)", pt->X, pt->Y);
}

const char *debugstr_matrix(const GpMatrix *matrix)
{
    if (!matrix) return "(null)";
    return wine_dbg_sprintf("%p(%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f)",matrix, matrix->matrix[0], matrix->matrix[1],
                            matrix->matrix[2], matrix->matrix[3], matrix->matrix[4], matrix->matrix[5]);
}
