/*
 * GdiPlusPen.h
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

#ifndef _GDIPLUSPEN_H
#define _GDIPLUSPEN_H

class Pen : public GdiplusBase
{
  public:
    friend class Graphics;
    friend class GraphicsPath;

    Pen(const Brush *brush, REAL width = 1.0f) : nativePen(NULL)
    {
        lastStatus = DllExports::GdipCreatePen2(getNat(brush), width, UnitWorld, &nativePen);
    }

    Pen(const Color &color, REAL width = 1.0f) : nativePen(NULL)
    {
        lastStatus = DllExports::GdipCreatePen1(color.GetValue(), width, UnitWorld, &nativePen);
    }

    ~Pen()
    {
        DllExports::GdipDeletePen(nativePen);
    }

    Pen *
    Clone()
    {
        GpPen *clonePen = NULL;
        SetStatus(DllExports::GdipClonePen(nativePen, &clonePen));
        if (lastStatus != Ok)
            return NULL;
        Pen *newPen = new Pen(clonePen, lastStatus);
        if (!newPen)
            DllExports::GdipDeletePen(clonePen);
        return newPen;
    }

    PenAlignment
    GetAlignment()
    {
        PenAlignment penAlignment;
        SetStatus(DllExports::GdipGetPenMode(nativePen, &penAlignment));
        return penAlignment;
    }

    Brush *
    GetBrush()
    {
        // FIXME
        return NULL;
    }

    Status
    GetColor(Color *color)
    {
        if (!color)
            return SetStatus(InvalidParameter);

        ARGB argb;
        SetStatus(DllExports::GdipGetPenColor(nativePen, &argb));
        color->SetValue(argb);
        return lastStatus;
    }

    Status
    GetCompoundArray(REAL *compoundArray, INT count)
    {
        if (!compoundArray || count <= 0)
            return SetStatus(InvalidParameter);
#if 1
        return SetStatus(NotImplemented);
#else
        return SetStatus(DllExports::GdipGetPenCompoundArray(nativePen, compoundArray, count));
#endif
    }

    INT
    GetCompoundArrayCount()
    {
        INT count = 0;
        SetStatus(DllExports::GdipGetPenCompoundCount(nativePen, &count));
        return count;
    }

    Status
    GetCustomEndCap(CustomLineCap *customCap)
    {
        if (!customCap)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipGetPenCustomEndCap(nativePen, &getNat(customCap)));
    }

    Status
    GetCustomStartCap(CustomLineCap *customCap)
    {
        if (!customCap)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipGetPenCustomStartCap(nativePen, &getNat(customCap)));
    }

    DashCap
    GetDashCap()
    {
        DashCap dashCap;
        SetStatus(DllExports::GdipGetPenDashCap197819(nativePen, &dashCap));
        return dashCap;
    }

    REAL
    GetDashOffset()
    {
        REAL offset;
        SetStatus(DllExports::GdipGetPenDashOffset(nativePen, &offset));
        return offset;
    }

    Status
    GetDashPattern(REAL *dashArray, INT count)
    {
        if (dashArray == NULL || count <= 0)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipGetPenDashArray(nativePen, dashArray, count));
    }

    INT
    GetDashPatternCount()
    {
        INT count = 0;
        SetStatus(DllExports::GdipGetPenDashCount(nativePen, &count));
        return count;
    }

    DashStyle
    GetDashStyle()
    {
        DashStyle dashStyle;
        SetStatus(DllExports::GdipGetPenDashStyle(nativePen, &dashStyle));
        return dashStyle;
    }

    LineCap
    GetEndCap()
    {
        LineCap endCap;
        SetStatus(DllExports::GdipGetPenEndCap(nativePen, &endCap));
        return endCap;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    LineJoin
    GetLineJoin()
    {
        LineJoin lineJoin;
        SetStatus(DllExports::GdipGetPenLineJoin(nativePen, &lineJoin));
        return lineJoin;
    }

    REAL
    GetMiterLimit()
    {
        REAL miterLimit;
        SetStatus(DllExports::GdipGetPenMiterLimit(nativePen, &miterLimit));
        return miterLimit;
    }

    PenType
    GetPenType()
    {
        PenType type;
        SetStatus(DllExports::GdipGetPenFillType(nativePen, &type));
        return type;
    }

    LineCap
    GetStartCap()
    {
        LineCap startCap;
        SetStatus(DllExports::GdipGetPenStartCap(nativePen, &startCap));
        return startCap;
    }

    Status
    GetTransform(Matrix *matrix)
    {
        return SetStatus(DllExports::GdipGetPenTransform(nativePen, getNat(matrix)));
    }

    REAL
    GetWidth()
    {
        REAL width;
        SetStatus(DllExports::GdipGetPenWidth(nativePen, &width));
        return width;
    }

    Status
    MultiplyTransform(Matrix *matrix, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipMultiplyPenTransform(nativePen, getNat(matrix), order));
    }

    Status
    ResetTransform()
    {
        return SetStatus(DllExports::GdipResetPenTransform(nativePen));
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipRotatePenTransform(nativePen, angle, order));
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipScalePenTransform(nativePen, sx, sy, order));
    }

    Status
    SetAlignment(PenAlignment penAlignment)
    {
        return SetStatus(DllExports::GdipSetPenMode(nativePen, penAlignment));
    }

    Status
    SetBrush(const Brush *brush)
    {
        GpBrush *theBrush = brush ? getNat(brush) : NULL;
        return SetStatus(DllExports::GdipSetPenBrushFill(nativePen, theBrush));
    }

    Status
    SetColor(const Color &color)
    {
        return SetStatus(DllExports::GdipSetPenColor(nativePen, color.GetValue()));
    }

    Status
    SetCompoundArray(const REAL *compoundArray, INT count)
    {
        return SetStatus(DllExports::GdipSetPenCompoundArray(nativePen, compoundArray, count));
    }

    Status
    SetCustomEndCap(const CustomLineCap *customCap)
    {
        GpCustomLineCap *cap = customCap ? getNat(customCap) : NULL;
        return SetStatus(DllExports::GdipSetPenCustomEndCap(nativePen, cap));
    }

    Status
    SetCustomStartCap(const CustomLineCap *customCap)
    {
        GpCustomLineCap *cap = customCap ? getNat(customCap) : NULL;
        return SetStatus(DllExports::GdipSetPenCustomStartCap(nativePen, cap));
    }

    Status
    SetDashCap(DashCap dashCap)
    {
        return SetStatus(DllExports::GdipSetPenDashCap197819(nativePen, dashCap));
    }

    Status
    SetDashOffset(REAL dashOffset)
    {
        return SetStatus(DllExports::GdipSetPenDashOffset(nativePen, dashOffset));
    }

    Status
    SetDashPattern(const REAL *dashArray, INT count)
    {
        return SetStatus(DllExports::GdipSetPenDashArray(nativePen, dashArray, count));
    }

    Status
    SetDashStyle(DashStyle dashStyle)
    {
        return SetStatus(DllExports::GdipSetPenDashStyle(nativePen, dashStyle));
    }

    Status
    SetEndCap(LineCap endCap)
    {
        return SetStatus(DllExports::GdipSetPenEndCap(nativePen, endCap));
    }

    Status
    SetLineCap(LineCap startCap, LineCap endCap, DashCap dashCap)
    {
        return SetStatus(DllExports::GdipSetPenLineCap197819(nativePen, startCap, endCap, dashCap));
    }

    Status
    SetLineJoin(LineJoin lineJoin)
    {
        return SetStatus(DllExports::GdipSetPenLineJoin(nativePen, lineJoin));
    }

    Status
    SetMiterLimit(REAL miterLimit)
    {
        return SetStatus(DllExports::GdipSetPenMiterLimit(nativePen, miterLimit));
    }

    Status
    SetStartCap(LineCap startCap)
    {
        return SetStatus(DllExports::GdipSetPenStartCap(nativePen, startCap));
    }

    Status
    SetTransform(const Matrix *matrix)
    {
        GpMatrix *mat = matrix ? getNat(matrix) : NULL;
        return SetStatus(DllExports::GdipSetPenTransform(nativePen, mat));
    }

    Status
    SetWidth(REAL width)
    {
        return SetStatus(DllExports::GdipSetPenWidth(nativePen, width));
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipTranslatePenTransform(nativePen, dx, dy, order));
    }

  protected:
    GpPen *nativePen;
    mutable Status lastStatus;

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    Pen(GpPen *pen, Status status) : nativePen(pen), lastStatus(status)
    {
    }

    VOID
    SetNativePen(GpPen *pen)
    {
        nativePen = pen;
    }

  private:
    // Pen is not copyable
    Pen(const Pen &);
    Pen &
    operator=(const Pen &);

    // get native
    friend inline GpPen *&
    getNat(const Pen *pen)
    {
        return const_cast<Pen *>(pen)->nativePen;
    }
};

#endif /* _GDIPLUSPEN_H */
