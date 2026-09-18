// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    File: MILEffectTypes.h

    Module Name: MILRender

    Created:

        10/09/2001 mbyrd

\*=========================================================================*/

#pragma once

// Predefined filters, aka WPF built-in effects

DEFINE_GUID(CLSID_MILEffectAlphaScale,     0x00000520,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

DEFINE_GUID(CLSID_MILEffectAlphaMask,      0x00000521,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

struct EffectParams
{
};

struct AlphaScaleParams : public EffectParams
{
    AlphaScaleParams() { }
    AlphaScaleParams(FLOAT s) : scale(s) { }
    
    FLOAT scale;
};

// When adding this effect, use AddWithResources to add 1 resource with this struct
// This resource should be of type IMILBitmapSource and is used as the mask image
struct AlphaMaskParams : public EffectParams
{
    D3DMATRIX matTransform;
};



