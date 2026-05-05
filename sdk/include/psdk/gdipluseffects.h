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

DEFINE_GUID(BlurEffectGuid,                   0x633c80a4, 0x1843, 0x482b, 0x9e, 0xf2, 0xbe, 0x28, 0x34, 0xc5, 0xfd, 0xd4);
DEFINE_GUID(SharpenEffectGuid,                0x63cbf3ee, 0xc526, 0x402c, 0x8f, 0x71, 0x62, 0xc5, 0x40, 0xbf, 0x51, 0x42);
DEFINE_GUID(ColorMatrixEffectGuid,            0x718f2615, 0x7933, 0x40e3, 0xa5, 0x11, 0x5f, 0x68, 0xfe, 0x14, 0xdd, 0x74);
DEFINE_GUID(ColorLUTEffectGuid,               0xa7ce72a9, 0x0f7f, 0x40d7, 0xb3, 0xcc, 0xd0, 0xc0, 0x2d, 0x5c, 0x32, 0x12);
DEFINE_GUID(BrightnessContrastEffectGuid,     0xd3a1dbe1, 0x8ec4, 0x4c17, 0x9f, 0x4c, 0xea, 0x97, 0xad, 0x1c, 0x34, 0x3d);
DEFINE_GUID(HueSaturationLightnessEffectGuid, 0x8b2dd6c3, 0xeb07, 0x4d87, 0xa5, 0xf0, 0x71, 0x08, 0xe2, 0x6a, 0x9c, 0x5f);
DEFINE_GUID(LevelsEffectGuid,                 0x99c354ec, 0x2a31, 0x4f3a, 0x8c, 0x34, 0x17, 0xa8, 0x03, 0xb3, 0x3a, 0x25);
DEFINE_GUID(TintEffectGuid,                   0x1077af00, 0x2848, 0x4441, 0x94, 0x89, 0x44, 0xad, 0x4c, 0x2d, 0x7a, 0x2c);
DEFINE_GUID(ColorBalanceEffectGuid,           0x537e597d, 0x251e, 0x48da, 0x96, 0x64, 0x29, 0xca, 0x49, 0x6b, 0x70, 0xf8);
DEFINE_GUID(RedEyeCorrectionEffectGuid,       0x74d29d05, 0x69a4, 0x4266, 0x95, 0x49, 0x3c, 0xc5, 0x28, 0x36, 0xb6, 0x32);
DEFINE_GUID(ColorCurveEffectGuid,             0xdd6a0022, 0x58e4, 0x4a67, 0x9d, 0x9b, 0xd4, 0x8e, 0xb8, 0x81, 0xa5, 0x3d);

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

#ifdef __cplusplus
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
#endif // __cplusplus

#endif /* _GDIPLUSEFFECTS_H */
