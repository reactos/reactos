// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

inline MilPoint2F MilPoint2FFromMilPoint2F(
    __in_ecount(1) const MilPoint2F &val
    )
{
    return *((MilPoint2F*)&val);
}

inline MilPoint2F MilPoint2FFromMilPoint2D(
    __in_ecount(1) const MilPoint2D &val
    )
{
    MilPoint2F pt;

    pt.X = static_cast<FLOAT>(val.X);
    pt.Y = static_cast<FLOAT>(val.Y);
    
    return pt;
}

inline MilPoint2F MilPoint2FFromDoubles(double x, double y)
{
    MilPoint2F pt;

    pt.X = static_cast<FLOAT>(x);
    pt.Y = static_cast<FLOAT>(y);
    
    return pt;
}

inline MilPointAndSizeF MilPointAndSizeFFromMilPointAndSizeF(
    __in_ecount(1) const MilPointAndSizeF &val
    )
{
    return *((MilPointAndSizeF*)&val);
}

#pragma warning(push)
// enumerator 'FORCE_DWORD' in switch of enum 'MilTileMode::Enum' is not explicitly handled
#pragma warning(disable:4061)

inline MilBitmapWrapMode::Enum MILBitmapWrapModeFromTileMode(
    __in_ecount(1) const MilTileMode::Enum &val
    )
{
    switch(val)
    {
        case MilTileMode::None:
            return MilBitmapWrapMode::Extend;

        case MilTileMode::FlipX:
            return MilBitmapWrapMode::FlipX;

        case MilTileMode::FlipY:
            return MilBitmapWrapMode::FlipY;

        case MilTileMode::FlipXY:
            return MilBitmapWrapMode::FlipXY;

        case MilTileMode::Tile:
            return MilBitmapWrapMode::Tile;

        case MilTileMode::Extend:
            return MilBitmapWrapMode::Extend;

        default:
            NO_DEFAULT("Unknown TileMode");
    }
} 

#pragma warning(pop)

inline MilGradientWrapMode::Enum MILGradientWrapModeFromMIL_GRADIENT_SPREAD_METHOD(
    __in_ecount(1) const MilGradientSpreadMethod::Enum &val
    )
{
    return (MilGradientWrapMode::Enum) val;
}

inline BOOL GammaCorrectedFromMILColorInterpolationMode(
    __in_ecount(1) const MilColorInterpolationMode::Enum &colorInterpolationMode
    )
{
    return (colorInterpolationMode == MilColorInterpolationMode::ScRgbLinearInterpolation);
}

inline void MilPointAndSizeFFromMilPointAndSizeD(
    __out_ecount(1) MilPointAndSizeF *dstRect,
    __in_ecount(1) const MilPointAndSizeD *srcRect
    )
{
    dstRect->X = static_cast<FLOAT>(srcRect->X);
    dstRect->Y = static_cast<FLOAT>(srcRect->Y);
    dstRect->Width = static_cast<FLOAT>(srcRect->Width);
    dstRect->Height = static_cast<FLOAT>(srcRect->Height);
}

inline void MilPointAndSizeFFromMilPointAndSizeL(
    __out_ecount(1) MilPointAndSizeF &dstRect, 
    __in_ecount(1) const MilPointAndSizeL &srcRect
    )
{
    dstRect.X = static_cast<FLOAT>(srcRect.X);
    dstRect.Y = static_cast<FLOAT>(srcRect.Y);
    dstRect.Width = static_cast<FLOAT>(srcRect.Width);
    dstRect.Height = static_cast<FLOAT>(srcRect.Height);
}

// Converting Width and Height could introduce NaNs so watch out
inline void MilRectFFromMilRectF(
    __out_ecount(1) MilPointAndSizeF *dstRect, 
    __in_ecount(1) const MilRectF *srcRect
    )
{
    dstRect->X      = srcRect->left;
    dstRect->Y      = srcRect->top;
    dstRect->Width  = srcRect->right - srcRect->left;
    dstRect->Height = srcRect->bottom - srcRect->top;
}

inline void MilPointAndSizeDFromMilPointAndSizeF(
    __out_ecount(1) MilPointAndSizeD *dstRect,
    __in_ecount(1) const MilPointAndSizeF *srcRect
    )
{
    dstRect->X = static_cast<DOUBLE>(srcRect->X);
    dstRect->Y = static_cast<DOUBLE>(srcRect->Y);
    dstRect->Width = static_cast<DOUBLE>(srcRect->Width);
    dstRect->Height = static_cast<DOUBLE>(srcRect->Height);    
}

// Left,Top-Right,Bottom Converters

//
// Convert double based XYWH rectangle to single based LTRB rectangle.
//
// Note that X+Width and Y+Height can be extremely large and result in +inf
// when stored as right and bottom (especially with less precision).
//
inline void MilRectFFromMilPointAndSizeD(
    __out_ecount(1) MilRectF &dstRect,
    __in_ecount(1) const MilPointAndSizeD &srcRect
    )
{
    dstRect.left = static_cast<FLOAT>(srcRect.X);
    dstRect.top = static_cast<FLOAT>(srcRect.Y);
    dstRect.right = static_cast<FLOAT>(srcRect.X + srcRect.Width);
    dstRect.bottom = static_cast<FLOAT>(srcRect.Y + srcRect.Height);
}

//
// Convert single based LTRB rectangle to double based XYWH rectangle.
//
inline void MilPointAndSizeDFromMilRectF(
    __out_ecount(1) MilPointAndSizeD &dstRect,
    __in_ecount(1) const MilRectF &srcRect
    )
{
    dstRect.X = static_cast<DOUBLE>(srcRect.left);
    dstRect.Y = static_cast<DOUBLE>(srcRect.top);
    // Take care that math is done after conversion to double to avoid overflow
    dstRect.Width = static_cast<DOUBLE>(srcRect.right) - dstRect.X;
    dstRect.Height = static_cast<DOUBLE>(srcRect.bottom) - dstRect.Y;    
}

inline void MilRectFFromMilPointAndSizeL(    
    __out_ecount(1) MilRectF &dstRect,
    __in_ecount(1) const MilPointAndSizeL &srcRect
    )
{
    dstRect.left = static_cast<float> (srcRect.X);
    dstRect.top = static_cast<float> (srcRect.Y);    
    dstRect.right = dstRect.left + static_cast<float> (srcRect.Width);
    dstRect.bottom = dstRect.top + static_cast<float> (srcRect.Height);
}    

inline void MilRectFFromMilRectL(    
    __out_ecount(1) MilRectF &dstRect,
    __in_ecount(1) const MilRectL &srcRect
    )
{
    dstRect.left = static_cast<float>(srcRect.left);
    dstRect.top = static_cast<float>(srcRect.top);    
    dstRect.right = static_cast<float>(srcRect.right);
    dstRect.bottom = static_cast<float>(srcRect.bottom);
}    

inline void MilRectDFromRECT(    
    __out_ecount(1) MilRectD &dstRect,
    __in_ecount(1) const RECT &srcRect
    )
{
    dstRect.left = static_cast<double> (srcRect.left);
    dstRect.top = static_cast<double> (srcRect.top);    
    dstRect.right = static_cast<double> (srcRect.right);
    dstRect.bottom = static_cast<double> (srcRect.bottom);
}    

inline void MilRectDFromMilRectF(    
    __out_ecount(1) MilRectD &dstRect,
    __in_ecount(1) const MilRectF &srcRect
    )
{
    dstRect.left = static_cast<double> (srcRect.left);
    dstRect.top = static_cast<double> (srcRect.top);    
    dstRect.right = static_cast<double> (srcRect.right);
    dstRect.bottom = static_cast<double> (srcRect.bottom);
}    

inline void MilRectFFromMilRectD(    
    __out_ecount(1) MilRectF &dstRect,
    __in_ecount(1) const MilRectD &srcRect
    )
{
    dstRect.left = static_cast<float> (srcRect.left);
    dstRect.top = static_cast<float> (srcRect.top);    
    dstRect.right = static_cast<float> (srcRect.right);
    dstRect.bottom = static_cast<float> (srcRect.bottom);
}    



