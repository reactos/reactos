/*
 * GdiPlusBrush.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSBRUSH_H
#define _GDIPLUSBRUSH_H

class Brush : public GdiplusBase
{
  public:
    friend class Graphics;
    friend class Pen;

    virtual ~Brush()
    {
        DllExports::GdipDeleteBrush(nativeBrush);
    }

    Brush *
    Clone() const
    {
        GpBrush *brush = NULL;
        SetStatus(DllExports::GdipCloneBrush(nativeBrush, &brush));
        if (lastStatus != Ok)
            return NULL;

        Brush *newBrush = new Brush(brush, lastStatus);
        if (newBrush == NULL)
        {
            DllExports::GdipDeleteBrush(brush);
        }
        return newBrush;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    BrushType
    GetType() const
    {
        BrushType type;
        SetStatus(DllExports::GdipGetBrushType(nativeBrush, &type));
        return type;
    }

  protected:
    GpBrush *nativeBrush;
    mutable Status lastStatus;

    Brush()
    {
    }

    Brush(GpBrush *brush, Status status) : nativeBrush(brush), lastStatus(status)
    {
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    void
    SetNativeBrush(GpBrush *brush)
    {
        nativeBrush = brush;
    }

  private:
    // Brush is not copyable
    Brush(const Brush &);
    Brush &
    operator=(const Brush &);

    // get native
    friend inline GpBrush *&
    getNat(const Brush *brush)
    {
        return const_cast<Brush *>(brush)->nativeBrush;
    }
};

class HatchBrush : public Brush
{
  public:
    friend class Pen;

    HatchBrush(HatchStyle hatchStyle, const Color &foreColor, const Color &backColor)
    {
        GpHatch *brush = NULL;
        lastStatus = DllExports::GdipCreateHatchBrush(hatchStyle, foreColor.GetValue(), backColor.GetValue(), &brush);
        SetNativeBrush(brush);
    }

    Status
    GetBackgroundColor(Color *color) const
    {
        if (!color)
            return SetStatus(InvalidParameter);

        ARGB argb;
        GpHatch *hatch = GetNativeHatch();
        SetStatus(DllExports::GdipGetHatchBackgroundColor(hatch, &argb));

        color->SetValue(argb);
        return lastStatus;
    }

    Status
    GetForegroundColor(Color *color) const
    {
        if (!color)
            return SetStatus(InvalidParameter);

        ARGB argb;
        GpHatch *hatch = GetNativeHatch();
        SetStatus(DllExports::GdipGetHatchForegroundColor(hatch, &argb));

        color->SetValue(argb);
        return lastStatus;
    }

    HatchStyle
    GetHatchStyle() const
    {
        HatchStyle hatchStyle;
        GpHatch *hatch = GetNativeHatch();
        SetStatus(DllExports::GdipGetHatchStyle(hatch, &hatchStyle));
        return hatchStyle;
    }

  protected:
    HatchBrush()
    {
    }

    GpHatch *
    GetNativeHatch() const
    {
        return static_cast<GpHatch *>(nativeBrush);
    }
};

class LinearGradientBrush : public Brush
{
  public:
    friend class Pen;

    LinearGradientBrush(const PointF &point1, const PointF &point2, const Color &color1, const Color &color2)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrush(
            &point1, &point2, color1.GetValue(), color2.GetValue(), WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    LinearGradientBrush(
        const Rect &rect,
        const Color &color1,
        const Color &color2,
        REAL angle,
        BOOL isAngleScalable = FALSE)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrushFromRectWithAngleI(
            &rect, color1.GetValue(), color2.GetValue(), angle, isAngleScalable, WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    LinearGradientBrush(const Rect &rect, const Color &color1, const Color &color2, LinearGradientMode mode)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrushFromRectI(
            &rect, color1.GetValue(), color2.GetValue(), mode, WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    LinearGradientBrush(const Point &point1, const Point &point2, const Color &color1, const Color &color2)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrushI(
            &point1, &point2, color1.GetValue(), color2.GetValue(), WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    LinearGradientBrush(
        const RectF &rect,
        const Color &color1,
        const Color &color2,
        REAL angle,
        BOOL isAngleScalable = FALSE)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrushFromRectWithAngle(
            &rect, color1.GetValue(), color2.GetValue(), angle, isAngleScalable, WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    LinearGradientBrush(const RectF &rect, const Color &color1, const Color &color2, LinearGradientMode mode)
    {
        GpLineGradient *brush = NULL;
        lastStatus = DllExports::GdipCreateLineBrushFromRect(
            &rect, color1.GetValue(), color2.GetValue(), mode, WrapModeTile, &brush);
        SetNativeBrush(brush);
    }

    Status
    GetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLineBlend(gradient, blendFactors, blendPositions, count));
    }

    INT
    GetBlendCount() const
    {
        INT count = 0;
        GpLineGradient *gradient = GetNativeGradient();
        SetStatus(DllExports::GdipGetLineBlendCount(gradient, &count));
        return count;
    }

    BOOL
    GetGammaCorrection() const
    {
        BOOL useGammaCorrection;
        GpLineGradient *gradient = GetNativeGradient();
        SetStatus(DllExports::GdipGetLineGammaCorrection(gradient, &useGammaCorrection));
        return useGammaCorrection;
    }

    INT
    GetInterpolationColorCount() const
    {
        INT count = 0;
        GpLineGradient *gradient = GetNativeGradient();
        SetStatus(DllExports::GdipGetLinePresetBlendCount(gradient, &count));
        return count;
    }

    Status
    GetInterpolationColors(Color *presetColors, REAL *blendPositions, INT count) const
    {
        return SetStatus(NotImplemented);
    }

    Status
    GetLinearColors(Color *colors) const
    {
        if (!colors)
            return SetStatus(InvalidParameter);

        GpLineGradient *gradient = GetNativeGradient();

        ARGB argb[2];
        SetStatus(DllExports::GdipGetLineColors(gradient, argb));
        if (lastStatus == Ok)
        {
            colors[0] = Color(argb[0]);
            colors[1] = Color(argb[1]);
        }
        return lastStatus;
    }

    Status
    GetRectangle(Rect *rect) const
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLineRectI(gradient, rect));
    }

    Status
    GetRectangle(RectF *rect) const
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLineRect(gradient, rect));
    }

    Status
    GetTransform(Matrix *matrix) const
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLineTransform(gradient, getNat(matrix)));
    }

    WrapMode
    GetWrapMode() const
    {

        WrapMode wrapMode;
        GpLineGradient *gradient = GetNativeGradient();
        SetStatus(DllExports::GdipGetLineWrapMode(gradient, &wrapMode));
        return wrapMode;
    }

    Status
    MultiplyTransform(const Matrix *matrix, MatrixOrder order)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipMultiplyLineTransform(gradient, getNat(matrix), order));
    }

    Status
    ResetTransform()
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipResetLineTransform(gradient));
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipRotateLineTransform(gradient, angle, order));
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order = MatrixOrderPrepend)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipScaleLineTransform(gradient, sx, sy, order));
    }

    Status
    SetBlend(const REAL *blendFactors, const REAL *blendPositions, INT count)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineBlend(gradient, blendFactors, blendPositions, count));
    }

    Status
    SetBlendBellShape(REAL focus, REAL scale)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineSigmaBlend(gradient, focus, scale));
    }

    Status
    SetBlendTriangularShape(REAL focus, REAL scale = 1.0f)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineLinearBlend(gradient, focus, scale));
    }

    Status
    SetGammaCorrection(BOOL useGammaCorrection)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineGammaCorrection(gradient, useGammaCorrection));
    }

    Status
    SetInterpolationColors(const Color *presetColors, const REAL *blendPositions, INT count)
    {
        return SetStatus(NotImplemented);
    }

    Status
    SetLinearColors(const Color &color1, const Color &color2)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineColors(gradient, color1.GetValue(), color2.GetValue()));
    }

    Status
    SetTransform(const Matrix *matrix)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineTransform(gradient, getNat(matrix)));
    }

    Status
    SetWrapMode(WrapMode wrapMode)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLineWrapMode(gradient, wrapMode));
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order = MatrixOrderPrepend)
    {
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipTranslateLineTransform(gradient, dx, dy, order));
    }

    Status
    SetLinearPoints(const PointF &point1, const PointF &point2)
    {
#if 1
        return SetStatus(NotImplemented);
#else
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLinePoints(gradient, &point1, &point2));
#endif
    }

    Status
    GetLinearPoints(PointF *points) const
    {
#if 1
        return SetStatus(NotImplemented);
#else
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLinePoints(gradient, points));
#endif
    }

    Status
    SetLinearPoints(const Point &point1, const Point &point2)
    {
#if 1
        return SetStatus(NotImplemented);
#else
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipSetLinePointsI(gradient, &point1, &point2));
#endif
    }

    Status
    GetLinearPoints(Point *points) const
    {
#if 1
        return SetStatus(NotImplemented);
#else
        GpLineGradient *gradient = GetNativeGradient();
        return SetStatus(DllExports::GdipGetLinePointsI(gradient, points));
#endif
    }

  protected:
    GpLineGradient *
    GetNativeGradient() const
    {
        return static_cast<GpLineGradient *>(nativeBrush);
    }
};

class SolidBrush : Brush
{
  public:
    friend class Pen;

    SolidBrush(const Color &color)
    {
        GpSolidFill *brush = NULL;
        lastStatus = DllExports::GdipCreateSolidFill(color.GetValue(), &brush);
        SetNativeBrush(brush);
    }

    Status
    GetColor(Color *color) const
    {
        if (!color)
            return SetStatus(InvalidParameter);

        ARGB argb;
        GpSolidFill *fill = GetNativeFill();
        SetStatus(DllExports::GdipGetSolidFillColor(fill, &argb));

        *color = Color(argb);
        return lastStatus;
    }

    Status
    SetColor(const Color &color)
    {
        GpSolidFill *fill = GetNativeFill();
        return SetStatus(DllExports::GdipSetSolidFillColor(fill, color.GetValue()));
    }

  protected:
    SolidBrush()
    {
    }

    GpSolidFill *
    GetNativeFill() const
    {
        return static_cast<GpSolidFill *>(nativeBrush);
    }
};

class TextureBrush : Brush
{
  public:
    TextureBrush(Image *image, WrapMode wrapMode, const RectF &dstRect)
    {
        GpTexture *texture = NULL;
        lastStatus = DllExports::GdipCreateTexture2(
            getNat(image), wrapMode, dstRect.X, dstRect.Y, dstRect.Width, dstRect.Height, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, Rect &dstRect, ImageAttributes *imageAttributes)
    {
        GpTexture *texture = NULL;
        GpImageAttributes *attrs = imageAttributes ? getNat(imageAttributes) : NULL;
        lastStatus = DllExports::GdipCreateTextureIA(
            getNat(image), attrs, dstRect.X, dstRect.Y, dstRect.Width, dstRect.Height, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, WrapMode wrapMode, INT dstX, INT dstY, INT dstWidth, INT dstHeight)
    {
        GpTexture *texture = NULL;
        lastStatus =
            DllExports::GdipCreateTexture2I(getNat(image), wrapMode, dstX, dstY, dstWidth, dstHeight, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, WrapMode wrapMode, REAL dstX, REAL dstY, REAL dstWidth, REAL dstHeight)
    {
        GpTexture *texture = NULL;
        lastStatus = DllExports::GdipCreateTexture2(getNat(image), wrapMode, dstX, dstY, dstWidth, dstHeight, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, RectF &dstRect, ImageAttributes *imageAttributes)
    {
        GpTexture *texture = NULL;
        GpImageAttributes *attrs = imageAttributes ? getNat(imageAttributes) : NULL;
        lastStatus = DllExports::GdipCreateTextureIA(
            getNat(image), attrs, dstRect.X, dstRect.Y, dstRect.Width, dstRect.Height, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, WrapMode wrapMode)
    {
        GpTexture *texture = NULL;
        lastStatus = DllExports::GdipCreateTexture(getNat(image), wrapMode, &texture);
        SetNativeBrush(texture);
    }

    TextureBrush(Image *image, WrapMode wrapMode, const Rect &dstRect)
    {
        GpTexture *texture = NULL;
        lastStatus = DllExports::GdipCreateTexture2I(
            getNat(image), wrapMode, dstRect.X, dstRect.Y, dstRect.Width, dstRect.Height, &texture);
        SetNativeBrush(texture);
    }

    // Defined in "gdiplusheaders.h":
    Image *
    GetImage() const;

    Status
    GetTransform(Matrix *matrix) const
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipGetTextureTransform(texture, getNat(matrix)));
    }

    WrapMode
    GetWrapMode() const
    {
        WrapMode wrapMode;
        GpTexture *texture = GetNativeTexture();
        SetStatus(DllExports::GdipGetTextureWrapMode(texture, &wrapMode));
        return wrapMode;
    }

    Status
    MultiplyTransform(Matrix *matrix, MatrixOrder order = MatrixOrderPrepend)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipMultiplyTextureTransform(texture, getNat(matrix), order));
    }

    Status
    ResetTransform()
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipResetTextureTransform(texture));
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipRotateTextureTransform(texture, angle, order));
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipScaleTextureTransform(texture, sx, sy, order));
    }

    Status
    SetTransform(const Matrix *matrix)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipSetTextureTransform(texture, getNat(matrix)));
    }

    Status
    SetWrapMode(WrapMode wrapMode)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipSetTextureWrapMode(texture, wrapMode));
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
    {
        GpTexture *texture = GetNativeTexture();
        return SetStatus(DllExports::GdipTranslateTextureTransform(texture, dx, dy, order));
    }

  protected:
    GpTexture *
    GetNativeTexture() const
    {
        return static_cast<GpTexture *>(nativeBrush);
    }

    TextureBrush()
    {
    }
};

#endif /* _GDIPLUSBRUSH_H */
