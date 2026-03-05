/*
 * Copyright (C) 2015 Alistair Leslie-Hughes
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

struct BlurParams
{
    float radius;
    BOOL expandEdge;
};

struct SharpenParams
{
    float radius;
    float amount;
};

struct TintParams
{
    INT hue;
    INT amount;
};

struct RedEyeCorrectionParams
{
    UINT numberOfAreas;
    RECT *areas;
};

struct ColorLUTParams
{
   ColorChannelLUT lutB;
   ColorChannelLUT lutG;
   ColorChannelLUT lutR;
   ColorChannelLUT lutA;
};

struct BrightnessContrastParams
{
    INT brightnessLevel;
    INT contrastLevel;
};

struct HueSaturationLightnessParams
{
    INT hueLevel;
    INT saturationLevel;
    INT lightnessLevel;
};

struct ColorBalanceParams
{
    INT cyanRed;
    INT magentaGreen;
    INT yellowBlue;
};

struct LevelsParams
{
    INT highlight;
    INT midtone;
    INT shadow;
};

enum CurveAdjustments
{
   AdjustExposure,
   AdjustDensity,
   AdjustContrast,
   AdjustHighlight,
   AdjustShadow,
   AdjustMidtone,
   AdjustWhiteSaturation,
   AdjustBlackSaturation
};

enum CurveChannel
{
    CurveChannelAll,
    CurveChannelRed,
    CurveChannelGreen,
    CurveChannelBlue
};

struct ColorCurveParams
{
    enum CurveAdjustments adjustment;
    enum CurveChannel channel;
    INT adjustValue;
};

#ifdef __cplusplus
extern "C" {
#endif

GpStatus WINGDIPAPI GdipCreateEffect(const GUID guid, CGpEffect **effect);
GpStatus WINGDIPAPI GdipDeleteEffect(CGpEffect *effect);

#ifdef __cplusplus
}
#endif

#endif
