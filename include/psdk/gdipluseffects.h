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

typedef struct {
  float radius;
  BOOL expandEdge;
} BlurParams;

typedef struct {
  INT brightnessLevel;
  INT contrastLevel;
} BrightnessContrastParams;

typedef struct {
  INT cyanRed;
  INT magentaGreen;
  INT yellowBlue;
} ColorBalanceParams;

typedef struct {
  CurveAdjustments adjustment;
  CurveChannel channel;
  INT adjustValue;
} ColorCurveParams;

typedef struct {
  INT hueLevel;
  INT saturationLevel;
  INT lightnessLevel;
} HueSaturationLightnessParams;

typedef struct {
  INT highlight;
  INT midtone;
  INT shadow;
} LevelsParams;

typedef struct {
  UINT numberOfAreas;
  RECT *areas;
} RedEyeCorrectionParams;

typedef struct {
  REAL radius;
  REAL amount;
} SharpenParams;

typedef struct {
  INT hue;
  INT amount;
} TintParams;


class Effect
{
public:
  Effect(VOID)
  {
  }

  VOID *GetAuxData(VOID) const
  {
    return NULL;
  }

  INT GetAuxDataSize(VOID)
  {
    return 0;
  }

  Status GetParameterSize(UINT *size)
  {
    return NotImplemented;
  }

  VOID UseAuxData(const BOOL useAuxDataFlag)
  {
  }
};


class Blur : public Effect
{
public:
  Blur(VOID)
  {
  }

  Status GetParameters(UINT *size, BlurParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const BlurParams *parameters)
  {
    return NotImplemented;
  }
};


class BrightnessContrast : public Effect
{
public:
  BrightnessContrast(VOID)
  {
  }

  Status GetParameters(UINT *size, BrightnessContrastParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const BrightnessContrastParams *parameters)
  {
    return NotImplemented;
  }
};


class ColorBalance : public Effect
{
public:
  ColorBalance(VOID)
  {
  }

  Status GetParameters(UINT *size, ColorBalanceParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(ColorBalanceParams *parameters)
  {
    return NotImplemented;
  }
};


class ColorCurve : public Effect
{
public:
  ColorCurve(VOID)
  {
  }

  Status GetParameters(UINT *size, ColorCurveParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const ColorCurveParams *parameters)
  {
    return NotImplemented;
  }
};


class ColorMatrixEffect : public Effect
{
public:
  ColorMatrixEffect(VOID)
  {
  }

  Status GetParameters(UINT *size, ColorMatrix *matrix)
  {
    return NotImplemented;
  }

  Status SetParameters(const ColorMatrix *matrix)
  {
    return NotImplemented;
  }
};


class HueSaturationLightness : public Effect
{
public:
  HueSaturationLightness(VOID)
  {
  }

  Status GetParameters(UINT *size, HueSaturationLightnessParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const HueSaturationLightnessParams *parameters)
  {
    return NotImplemented;
  }
};


class Levels : public Effect
{
public:
  Levels(VOID)
  {
  }

  Status GetParameters(UINT *size, LevelsParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const LevelsParams *parameters)
  {
    return NotImplemented;
  }
};

class RedEyeCorrection : public Effect
{
public:
  RedEyeCorrection(VOID)
  {
  }

  Status GetParameters(UINT *size, RedEyeCorrectionParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const RedEyeCorrectionParams *parameters)
  {
    return NotImplemented;
  }
};


class Sharpen
{
public:
  Sharpen(VOID)
  {
  }

  Status GetParameters(UINT *size, SharpenParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const SharpenParams *parameters)
  {
    return NotImplemented;
  }
};


class Tint : Effect
{
public:
  Tint(VOID)
  {
  }

  Status GetParameters(UINT *size, TintParams *parameters)
  {
    return NotImplemented;
  }

  Status SetParameters(const TintParams *parameters)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSEFFECTS_H */
