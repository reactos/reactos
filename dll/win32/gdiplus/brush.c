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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#define COBJMACROS
#include "objbase.h"
#include "olectl.h"
#include "ole2.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

GpStatus WINGDIPAPI GdipCloneBrush(GpBrush *brush, GpBrush **clone)
{
    if(!brush || !clone)
        return InvalidParameter;

    switch(brush->bt){
        case BrushTypeSolidColor:
            *clone = GdipAlloc(sizeof(GpSolidFill));
            if (!*clone) return OutOfMemory;

            memcpy(*clone, brush, sizeof(GpSolidFill));

            (*clone)->gdibrush = CreateBrushIndirect(&(*clone)->lb);
            break;
        case BrushTypePathGradient:{
            GpPathGradient *src, *dest;
            INT count;

            *clone = GdipAlloc(sizeof(GpPathGradient));
            if (!*clone) return OutOfMemory;

            src = (GpPathGradient*) brush,
            dest = (GpPathGradient*) *clone;
            count = src->pathdata.Count;

            memcpy(dest, src, sizeof(GpPathGradient));

            dest->pathdata.Count = count;
            dest->pathdata.Points = GdipAlloc(count * sizeof(PointF));
            dest->pathdata.Types = GdipAlloc(count);

            if(!dest->pathdata.Points || !dest->pathdata.Types){
                GdipFree(dest->pathdata.Points);
                GdipFree(dest->pathdata.Types);
                GdipFree(dest);
                return OutOfMemory;
            }

            memcpy(dest->pathdata.Points, src->pathdata.Points, count * sizeof(PointF));
            memcpy(dest->pathdata.Types, src->pathdata.Types, count);

            break;
        }
        case BrushTypeLinearGradient:
            *clone = GdipAlloc(sizeof(GpLineGradient));
            if(!*clone)    return OutOfMemory;

            memcpy(*clone, brush, sizeof(GpLineGradient));

            (*clone)->gdibrush = CreateSolidBrush((*clone)->lb.lbColor);
            break;
        case BrushTypeTextureFill:
            *clone = GdipAlloc(sizeof(GpTexture));
            if(!*clone)    return OutOfMemory;

            memcpy(*clone, brush, sizeof(GpTexture));

            (*clone)->gdibrush = CreateBrushIndirect(&(*clone)->lb);
            break;
        default:
            ERR("not implemented for brush type %d\n", brush->bt);
            return NotImplemented;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateLineBrush(GDIPCONST GpPointF* startpoint,
    GDIPCONST GpPointF* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    COLORREF col = ARGB2COLORREF(startcolor);

    if(!line || !startpoint || !endpoint || wrap == WrapModeClamp)
        return InvalidParameter;

    *line = GdipAlloc(sizeof(GpLineGradient));
    if(!*line)  return OutOfMemory;

    (*line)->brush.lb.lbStyle = BS_SOLID;
    (*line)->brush.lb.lbColor = col;
    (*line)->brush.lb.lbHatch = 0;
    (*line)->brush.gdibrush = CreateSolidBrush(col);
    (*line)->brush.bt = BrushTypeLinearGradient;

    (*line)->startpoint.X = startpoint->X;
    (*line)->startpoint.Y = startpoint->Y;
    (*line)->endpoint.X = endpoint->X;
    (*line)->endpoint.Y = endpoint->Y;
    (*line)->startcolor = startcolor;
    (*line)->endcolor = endcolor;
    (*line)->wrap = wrap;
    (*line)->gamma = FALSE;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateLineBrushI(GDIPCONST GpPoint* startpoint,
    GDIPCONST GpPoint* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    GpPointF stF;
    GpPointF endF;

    if(!startpoint || !endpoint)
        return InvalidParameter;

    stF.X  = (REAL)startpoint->X;
    stF.Y  = (REAL)startpoint->Y;
    endF.X = (REAL)endpoint->X;
    endF.X = (REAL)endpoint->Y;

    return GdipCreateLineBrush(&stF, &endF, startcolor, endcolor, wrap, line);
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRect(GDIPCONST GpRectF* rect,
    ARGB startcolor, ARGB endcolor, LinearGradientMode mode, GpWrapMode wrap,
    GpLineGradient **line)
{
    GpPointF start, end;

    if(!line || !rect)
        return InvalidParameter;

    start.X = rect->X;
    start.Y = rect->Y;
    end.X = rect->X + rect->Width;
    end.Y = rect->Y + rect->Height;

    return GdipCreateLineBrush(&start, &end, startcolor, endcolor, wrap, line);
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRectI(GDIPCONST GpRect* rect,
    ARGB startcolor, ARGB endcolor, LinearGradientMode mode, GpWrapMode wrap,
    GpLineGradient **line)
{
    GpRectF rectF;

    rectF.X      = (REAL) rect->X;
    rectF.Y      = (REAL) rect->Y;
    rectF.Width  = (REAL) rect->Width;
    rectF.Height = (REAL) rect->Height;

    return GdipCreateLineBrushFromRect(&rectF, startcolor, endcolor, mode, wrap, line);
}

GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    if(!points || !grad)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->pathdata.Count = count;
    (*grad)->pathdata.Points = GdipAlloc(count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, points, count * sizeof(PointF));
    memset((*grad)->pathdata.Types, PathPointTypeLine, count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = wrap;
    (*grad)->gamma = FALSE;
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePathGradientI(GDIPCONST GpPoint* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    GpPointF *pointsF;
    GpStatus ret;
    INT i;

    if(!points || !grad)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    pointsF = GdipAlloc(sizeof(GpPointF) * count);
    if(!pointsF)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        pointsF[i].X = (REAL)points[i].X;
        pointsF[i].Y = (REAL)points[i].Y;
    }

    ret = GdipCreatePathGradient(pointsF, count, wrap, grad);
    GdipFree(pointsF);

    return ret;
}

/* FIXME: path gradient brushes not truly supported (drawn as solid brushes) */
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath* path,
    GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    if(!path || !grad)
        return InvalidParameter;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->pathdata.Count = path->pathdata.Count;
    (*grad)->pathdata.Points = GdipAlloc(path->pathdata.Count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(path->pathdata.Count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, path->pathdata.Points,
           path->pathdata.Count * sizeof(PointF));
    memcpy((*grad)->pathdata.Types, path->pathdata.Types, path->pathdata.Count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = WrapModeClamp;
    (*grad)->gamma = FALSE;
    /* FIXME: this should be set to the "centroid" of the path by default */
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB color, GpSolidFill **sf)
{
    COLORREF col = ARGB2COLORREF(color);

    if(!sf)  return InvalidParameter;

    *sf = GdipAlloc(sizeof(GpSolidFill));
    if (!*sf) return OutOfMemory;

    (*sf)->brush.lb.lbStyle = BS_SOLID;
    (*sf)->brush.lb.lbColor = col;
    (*sf)->brush.lb.lbHatch = 0;

    (*sf)->brush.gdibrush = CreateSolidBrush(col);
    (*sf)->brush.bt = BrushTypeSolidColor;
    (*sf)->color = color;

    return Ok;
}

/* FIXME: imageattr ignored */
GpStatus WINGDIPAPI GdipCreateTextureIA(GpImage *image,
    GDIPCONST GpImageAttributes *imageattr, REAL x, REAL y, REAL width,
    REAL height, GpTexture **texture)
{
    HDC hdc;
    OLE_HANDLE hbm;
    HBITMAP old = NULL;
    BITMAPINFO bmi;
    BITMAPINFOHEADER *bmih;
    INT n_x, n_y, n_width, n_height, abs_height, stride, image_stride, i, bytespp;
    BOOL bm_is_selected;
    BYTE *dibits, *buff, *textbits;

    if(!image || !texture || x < 0.0 || y < 0.0 || width < 0.0 || height < 0.0)
        return InvalidParameter;

    if(image->type != ImageTypeBitmap){
        FIXME("not implemented for image type %d\n", image->type);
        return NotImplemented;
    }

    n_x = roundr(x);
    n_y = roundr(y);
    n_width = roundr(width);
    n_height = roundr(height);

    if(n_x + n_width > ((GpBitmap*)image)->width ||
       n_y + n_height > ((GpBitmap*)image)->height)
        return InvalidParameter;

    IPicture_get_Handle(image->picture, &hbm);
    if(!hbm)   return GenericError;
    IPicture_get_CurDC(image->picture, &hdc);
    bm_is_selected = (hdc != 0);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biBitCount = 0;

    if(!bm_is_selected){
        hdc = CreateCompatibleDC(0);
        old = SelectObject(hdc, (HBITMAP)hbm);
    }

    /* fill out bmi */
    GetDIBits(hdc, (HBITMAP)hbm, 0, 0, NULL, &bmi, DIB_RGB_COLORS);

    bytespp = bmi.bmiHeader.biBitCount / 8;
    abs_height = abs(bmi.bmiHeader.biHeight);

    if(n_x > bmi.bmiHeader.biWidth || n_x + n_width > bmi.bmiHeader.biWidth ||
       n_y > abs_height || n_y + n_height > abs_height)
        return InvalidParameter;

    dibits = GdipAlloc(bmi.bmiHeader.biSizeImage);

    if(dibits)  /* this is not a good place to error out */
        GetDIBits(hdc, (HBITMAP)hbm, 0, abs_height, dibits, &bmi, DIB_RGB_COLORS);

    if(!bm_is_selected){
        SelectObject(hdc, old);
        DeleteDC(hdc);
    }

    if(!dibits)
        return OutOfMemory;

    image_stride = (bmi.bmiHeader.biWidth * bytespp + 3) & ~3;
    stride = (n_width * bytespp + 3) & ~3;
    buff = GdipAlloc(sizeof(BITMAPINFOHEADER) + stride * n_height);
    if(!buff){
        GdipFree(dibits);
        return OutOfMemory;
    }

    bmih = (BITMAPINFOHEADER*)buff;
    textbits = (BYTE*) (bmih + 1);
    bmih->biSize = sizeof(BITMAPINFOHEADER);
    bmih->biWidth = n_width;
    bmih->biHeight = n_height;
    bmih->biCompression = BI_RGB;
    bmih->biSizeImage = stride * n_height;
    bmih->biBitCount = bmi.bmiHeader.biBitCount;
    bmih->biClrUsed = 0;
    bmih->biPlanes = 1;

    /* image is flipped */
    if(bmi.bmiHeader.biHeight > 0){
        dibits += bmi.bmiHeader.biSizeImage;
        image_stride *= -1;
        textbits += stride * (n_height - 1);
        stride *= -1;
    }

    for(i = 0; i < n_height; i++)
        memcpy(&textbits[i * stride],
               &dibits[n_x * bytespp + (n_y + i) * image_stride],
               abs(stride));

    *texture = GdipAlloc(sizeof(GpTexture));
    if (!*texture) return OutOfMemory;

    (*texture)->brush.lb.lbStyle = BS_DIBPATTERNPT;
    (*texture)->brush.lb.lbColor = DIB_RGB_COLORS;
    (*texture)->brush.lb.lbHatch = (ULONG_PTR)buff;

    (*texture)->brush.gdibrush = CreateBrushIndirect(&(*texture)->brush.lb);
    (*texture)->brush.bt = BrushTypeTextureFill;

    GdipFree(dibits);
    GdipFree(buff);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetBrushType(GpBrush *brush, GpBrushType *type)
{
    if(!brush || !type)  return InvalidParameter;

    *type = brush->bt;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush *brush)
{
    if(!brush)  return InvalidParameter;

    switch(brush->bt)
    {
        case BrushTypePathGradient:
            GdipFree(((GpPathGradient*) brush)->pathdata.Points);
            GdipFree(((GpPathGradient*) brush)->pathdata.Types);
            break;
        case BrushTypeSolidColor:
        case BrushTypeLinearGradient:
        case BrushTypeTextureFill:
        default:
            break;
    }

    DeleteObject(brush->gdibrush);
    GdipFree(brush);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineGammaCorrection(GpLineGradient *line,
    BOOL *usinggamma)
{
    if(!line)
        return InvalidParameter;

    *usinggamma = line->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    if(!grad || !point)
        return InvalidParameter;

    point->X = grad->center.X;
    point->Y = grad->center.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPointI(GpPathGradient *grad,
    GpPoint *point)
{
    GpStatus ret;
    GpPointF ptf;

    if(!point)
        return InvalidParameter;

    ret = GdipGetPathGradientCenterPoint(grad,&ptf);

    if(ret == Ok){
        point->X = roundr(ptf.X);
        point->Y = roundr(ptf.Y);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient *grad,
    REAL *x, REAL *y)
{
    if(!grad || !x || !y)
        return InvalidParameter;

    *x = grad->focus.X;
    *y = grad->focus.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL *gamma)
{
    if(!grad || !gamma)
        return InvalidParameter;

    *gamma = grad->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient *grad,
    INT *count)
{
    if(!grad || !count)
        return InvalidParameter;

    *count = grad->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    if(!grad || !argb || !count || (*count < grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetSolidFillColor(GpSolidFill *sf, ARGB *argb)
{
    if(!sf || !argb)
        return InvalidParameter;

    *argb = sf->color;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineBlend(GpLineGradient *brush,
    GDIPCONST REAL *blend, GDIPCONST REAL* positions, INT count)
{
    static int calls;

    if(!brush || !blend || !positions || count <= 0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineGammaCorrection(GpLineGradient *line,
    BOOL usegamma)
{
    if(!line)
        return InvalidParameter;

    line->gamma = usegamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineSigmaBlend(GpLineGradient *line, REAL focus,
    REAL scale)
{
    static int calls;

    if(!line || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetLineWrapMode(GpLineGradient *line,
    GpWrapMode wrap)
{
    if(!line || wrap == WrapModeClamp)
        return InvalidParameter;

    line->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient *grad,
    ARGB argb)
{
    if(!grad)
        return InvalidParameter;

    grad->centercolor = argb;
    grad->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(grad->brush.gdibrush);
    grad->brush.gdibrush = CreateSolidBrush(grad->brush.lb.lbColor);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    if(!grad || !point)
        return InvalidParameter;

    grad->center.X = point->X;
    grad->center.Y = point->Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPointI(GpPathGradient *grad,
    GpPoint *point)
{
    GpPointF ptf;

    if(!point)
        return InvalidParameter;

    ptf.X = (REAL)point->X;
    ptf.Y = (REAL)point->Y;

    return GdipSetPathGradientCenterPoint(grad,&ptf);
}

GpStatus WINGDIPAPI GdipSetPathGradientFocusScales(GpPathGradient *grad,
    REAL x, REAL y)
{
    if(!grad)
        return InvalidParameter;

    grad->focus.X = x;
    grad->focus.Y = y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL gamma)
{
    if(!grad)
        return InvalidParameter;

    grad->gamma = gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient *grad,
    REAL focus, REAL scale)
{
    static int calls;

    if(!grad || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    if(!grad || !argb || !count || (*count <= 0) ||
        (*count > grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient *grad,
    GpWrapMode wrap)
{
    if(!grad)
        return InvalidParameter;

    grad->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill *sf, ARGB argb)
{
    if(!sf)
        return InvalidParameter;

    sf->color = argb;
    sf->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(sf->brush.gdibrush);
    sf->brush.gdibrush = CreateSolidBrush(sf->brush.lb.lbColor);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetTextureTransform(GpTexture *texture,
    GDIPCONST GpMatrix *matrix)
{
    static int calls;

    if(!texture || !matrix)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineColors(GpLineGradient *brush, ARGB color1,
    ARGB color2)
{
    if(!brush)
        return InvalidParameter;

    brush->startcolor = color1;
    brush->endcolor   = color2;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineColors(GpLineGradient *brush, ARGB *colors)
{
    if(!brush || !colors)
        return InvalidParameter;

    colors[0] = brush->startcolor;
    colors[1] = brush->endcolor;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineLinearBlend(GpLineGradient *brush, REAL focus,
    REAL scale)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetLinePresetBlend(GpLineGradient *brush,
    GDIPCONST ARGB *blend, GDIPCONST REAL* positions, INT count)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetLineTransform(GpLineGradient *brush,
    GDIPCONST GpMatrix *matrix)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetLineRect(GpLineGradient *brush, GpRectF *rect)
{
    if(!brush || !rect)
        return InvalidParameter;

    rect->X = (brush->startpoint.X < brush->endpoint.X ? brush->startpoint.X: brush->endpoint.X);
    rect->Y = (brush->startpoint.Y < brush->endpoint.Y ? brush->startpoint.Y: brush->endpoint.Y);

    rect->Width  = fabs(brush->startpoint.X - brush->endpoint.X);
    rect->Height = fabs(brush->startpoint.Y - brush->endpoint.Y);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineRectI(GpLineGradient *brush, GpRect *rect)
{
    GpRectF  rectF;
    GpStatus ret;

    ret = GdipGetLineRect(brush, &rectF);

    if(ret == Ok){
        rect->X      = roundr(rectF.X);
        rect->Y      = roundr(rectF.Y);
        rect->Width  = roundr(rectF.Width);
        rect->Height = roundr(rectF.Height);
    }

    return ret;
}
