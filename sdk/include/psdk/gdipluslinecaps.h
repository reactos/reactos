/*
 * GdiPlusLineCaps.h
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

#ifndef _GDIPLUSLINECAPS_H
#define _GDIPLUSLINECAPS_H

inline CustomLineCap::CustomLineCap(
    const GraphicsPath *fillPath,
    const GraphicsPath *strokePath,
    LineCap baseCap,
    REAL baseInset)
    : nativeCap(NULL)
{
    nativeCap = NULL;
    GpPath *nativeFillPath = fillPath ? getNat(fillPath) : NULL;
    GpPath *nativeStrokePath = strokePath ? getNat(strokePath) : NULL;
    lastStatus = DllExports::GdipCreateCustomLineCap(nativeFillPath, nativeStrokePath, baseCap, baseInset, &nativeCap);
}

inline CustomLineCap::~CustomLineCap()
{
    DllExports::GdipDeleteCustomLineCap(nativeCap);
}

inline CustomLineCap *
CustomLineCap::Clone()
{
    GpCustomLineCap *cap = NULL;
    SetStatus(DllExports::GdipCloneCustomLineCap(nativeCap, &cap));
    if (lastStatus != Ok)
        return NULL;

    CustomLineCap *newLineCap = new CustomLineCap(cap, lastStatus);
    if (newLineCap == NULL)
    {
        SetStatus(DllExports::GdipDeleteCustomLineCap(cap));
    }

    return newLineCap;
}

inline LineCap
CustomLineCap::GetBaseCap()
{
    LineCap baseCap;
    SetStatus(DllExports::GdipGetCustomLineCapBaseCap(nativeCap, &baseCap));
    return baseCap;
}

inline REAL
CustomLineCap::GetBaseInset()
{
    REAL inset;
    SetStatus(DllExports::GdipGetCustomLineCapBaseInset(nativeCap, &inset));
    return inset;
}

inline Status
CustomLineCap::GetLastStatus()
{
    return lastStatus;
}

inline Status
CustomLineCap::GetStrokeCaps(LineCap *startCap, LineCap *endCap)
{
#if 1
    return SetStatus(NotImplemented);
#else
    return SetStatus(DllExports::GdipGetCustomLineCapStrokeCaps(nativeCap, startCap, endCap));
#endif
}

inline LineJoin
CustomLineCap::GetStrokeJoin()
{
    LineJoin lineJoin;
    SetStatus(DllExports::GdipGetCustomLineCapStrokeJoin(nativeCap, &lineJoin));
    return lineJoin;
}

inline REAL
CustomLineCap::GetWidthScale()
{
    REAL widthScale;
    SetStatus(DllExports::GdipGetCustomLineCapWidthScale(nativeCap, &widthScale));
    return widthScale;
}

inline Status
CustomLineCap::SetBaseCap(LineCap baseCap)
{
    return SetStatus(DllExports::GdipSetCustomLineCapBaseCap(nativeCap, baseCap));
}

inline Status
CustomLineCap::SetBaseInset(REAL inset)
{
    return SetStatus(DllExports::GdipSetCustomLineCapBaseInset(nativeCap, inset));
}

inline Status
CustomLineCap::SetStrokeCap(LineCap strokeCap)
{
    return SetStrokeCaps(strokeCap, strokeCap);
}

inline Status
CustomLineCap::SetStrokeCaps(LineCap startCap, LineCap endCap)
{
    return SetStatus(DllExports::GdipSetCustomLineCapStrokeCaps(nativeCap, startCap, endCap));
}

inline Status
CustomLineCap::SetStrokeJoin(LineJoin lineJoin)
{
    return SetStatus(DllExports::GdipSetCustomLineCapStrokeJoin(nativeCap, lineJoin));
}

inline Status
CustomLineCap::SetWidthScale(IN REAL widthScale)
{
    return SetStatus(DllExports::GdipSetCustomLineCapWidthScale(nativeCap, widthScale));
}

class AdjustableArrowCap : public CustomLineCap
{
  public:
    AdjustableArrowCap(REAL height, REAL width, BOOL isFilled)
    {
        GpAdjustableArrowCap *cap = NULL;
        lastStatus = DllExports::GdipCreateAdjustableArrowCap(height, width, isFilled, &cap);
        SetNativeCap(cap);
    }

    REAL
    GetHeight()
    {
        REAL height;
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        SetStatus(DllExports::GdipGetAdjustableArrowCapHeight(cap, &height));
        return height;
    }

    REAL
    GetMiddleInset()
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        REAL middleInset;
        SetStatus(DllExports::GdipGetAdjustableArrowCapMiddleInset(cap, &middleInset));
        return middleInset;
    }

    REAL
    GetWidth()
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        REAL width;
        SetStatus(DllExports::GdipGetAdjustableArrowCapWidth(cap, &width));
        return width;
    }

    BOOL
    IsFilled()
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        BOOL isFilled;
        SetStatus(DllExports::GdipGetAdjustableArrowCapFillState(cap, &isFilled));
        return isFilled;
    }

    Status
    SetFillState(BOOL isFilled)
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        return SetStatus(DllExports::GdipSetAdjustableArrowCapFillState(cap, isFilled));
    }

    Status
    SetHeight(REAL height)
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        return SetStatus(DllExports::GdipSetAdjustableArrowCapHeight(cap, height));
    }

    Status
    SetMiddleInset(REAL middleInset)
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        return SetStatus(DllExports::GdipSetAdjustableArrowCapMiddleInset(cap, middleInset));
    }

    Status
    SetWidth(REAL width)
    {
        GpAdjustableArrowCap *cap = GetNativeAdjustableArrowCap();
        return SetStatus(DllExports::GdipSetAdjustableArrowCapWidth(cap, width));
    }

  protected:
    GpAdjustableArrowCap *
    GetNativeAdjustableArrowCap() const
    {
        return static_cast<GpAdjustableArrowCap *>(nativeCap);
    }

  private:
    // AdjustableArrowCap is not copyable
    AdjustableArrowCap(const AdjustableArrowCap &);
    AdjustableArrowCap &
    operator=(const AdjustableArrowCap &);
};

#endif /* _GDIPLUSLINECAPS_H */
