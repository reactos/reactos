/*
 * GdiPlusEffects.h
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

#ifndef _GDIPLUSEFFECTS_H
#define _GDIPLUSEFFECTS_H

typedef enum CurveAdjustments
{
    AdjustExposure = 0,
    AdjustDensity = 1,
    AdjustContrast = 2,
    AdjustHighlight = 3,
    AdjustShadow = 4,
    AdjustMidtone = 5,
    AdjustWhiteSaturation = 6,
    AdjustBlackSaturation = 7
} CurveAdjustments;

typedef enum CurveChannel
{
    CurveChannelAll = 0,
    CurveChannelRed = 1,
    CurveChannelGreen = 2,
    CurveChannelBlue = 3
} CurveChannel;

typedef struct BlurParams
{
    REAL radius;
    BOOL expandEdge;
} BlurParams;

typedef struct BrightnessContrastParams
{
    INT brightnessLevel;
    INT contrastLevel;
} BrightnessContrastParams;

typedef struct ColorBalanceParams
{
    INT cyanRed;
    INT magentaGreen;
    INT yellowBlue;
} ColorBalanceParams;

typedef struct ColorCurveParams
{
    CurveAdjustments adjustment;
    CurveChannel channel;
    INT adjustValue;
} ColorCurveParams;

typedef struct ColorLUTParams
{
    ColorChannelLUT lutB;
    ColorChannelLUT lutG;
    ColorChannelLUT lutR;
    ColorChannelLUT lutA;
} ColorLUTParams;

typedef struct HueSaturationLightnessParams
{
    INT hueLevel;
    INT saturationLevel;
    INT lightnessLevel;
} HueSaturationLightnessParams;

typedef struct LevelsParams
{
    INT highlight;
    INT midtone;
    INT shadow;
} LevelsParams;

typedef struct RedEyeCorrectionParams
{
    UINT numberOfAreas;
    RECT *areas;
} RedEyeCorrectionParams;

typedef struct SharpenParams
{
    REAL radius;
    REAL amount;
} SharpenParams;

typedef struct TintParams
{
    INT hue;
    INT amount;
} TintParams;

class Effect
{
  public:
    Effect()
    {
    }

    VOID *
    GetAuxData() const
    {
        return NULL;
    }

    INT
    GetAuxDataSize()
    {
        return 0;
    }

    Status
    GetParameterSize(UINT *size)
    {
        return NotImplemented;
    }

    VOID
    UseAuxData(const BOOL useAuxDataFlag)
    {
    }
};

class Blur : public Effect
{
  public:
    Blur()
    {
    }

    Status
    GetParameters(UINT *size, BlurParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const BlurParams *parameters)
    {
        return NotImplemented;
    }
};

class BrightnessContrast : public Effect
{
  public:
    BrightnessContrast()
    {
    }

    Status
    GetParameters(UINT *size, BrightnessContrastParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const BrightnessContrastParams *parameters)
    {
        return NotImplemented;
    }
};

class ColorBalance : public Effect
{
  public:
    ColorBalance()
    {
    }

    Status
    GetParameters(UINT *size, ColorBalanceParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(ColorBalanceParams *parameters)
    {
        return NotImplemented;
    }
};

class ColorCurve : public Effect
{
  public:
    ColorCurve()
    {
    }

    Status
    GetParameters(UINT *size, ColorCurveParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const ColorCurveParams *parameters)
    {
        return NotImplemented;
    }
};

class ColorMatrixEffect : public Effect
{
  public:
    ColorMatrixEffect()
    {
    }

    Status
    GetParameters(UINT *size, ColorMatrix *matrix)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const ColorMatrix *matrix)
    {
        return NotImplemented;
    }
};

class HueSaturationLightness : public Effect
{
  public:
    HueSaturationLightness()
    {
    }

    Status
    GetParameters(UINT *size, HueSaturationLightnessParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const HueSaturationLightnessParams *parameters)
    {
        return NotImplemented;
    }
};

class Levels : public Effect
{
  public:
    Levels()
    {
    }

    Status
    GetParameters(UINT *size, LevelsParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const LevelsParams *parameters)
    {
        return NotImplemented;
    }
};

class RedEyeCorrection : public Effect
{
  public:
    RedEyeCorrection()
    {
    }

    Status
    GetParameters(UINT *size, RedEyeCorrectionParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const RedEyeCorrectionParams *parameters)
    {
        return NotImplemented;
    }
};

class Sharpen
{
  public:
    Sharpen()
    {
    }

    Status
    GetParameters(UINT *size, SharpenParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const SharpenParams *parameters)
    {
        return NotImplemented;
    }
};

class Tint : Effect
{
  public:
    Tint()
    {
    }

    Status
    GetParameters(UINT *size, TintParams *parameters)
    {
        return NotImplemented;
    }

    Status
    SetParameters(const TintParams *parameters)
    {
        return NotImplemented;
    }
};

#endif /* _GDIPLUSEFFECTS_H */
