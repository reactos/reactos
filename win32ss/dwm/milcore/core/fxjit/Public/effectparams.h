// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:  Effect params
//

#pragma once

// Built in types used in this file so that it can be included in both 
// Silverlight and WPF, i.e., use "int" instead of "INT32".

#define PIXELSHADER_SAMPLERS_MAX  16
#define PIXELSHADER_CONSTANTS_MAX 32

//------------------------------------------------------------------------------
//
//  Class:     CSamplerState
//
//  Synopsis:  Sampler state
//
//------------------------------------------------------------------------------
struct CSamplerState
{
    unsigned *m_pargbSource;
    unsigned  m_nWidth;
    unsigned  m_nHeight;
    unsigned  m_nUseBilinear;
};

//------------------------------------------------------------------------------
//
//  Class:     CPixelShaderState
//
//  Synopsis:  The pixel shader samplers and constant state
//
//------------------------------------------------------------------------------
struct CPixelShaderState
{
    //
    // ctor
    //

    CPixelShaderState()
    {
        //
        // Samplers default to white textures
        //

        m_uWhite = 0xffffffff;
        for (unsigned i = 0; i < PIXELSHADER_SAMPLERS_MAX; i++)
        {
            m_samplers[i].m_pargbSource = &m_uWhite;
            m_samplers[i].m_nWidth = 1;
            m_samplers[i].m_nHeight = 1;
            m_samplers[i].m_nUseBilinear= 0; // default to nearest neighbor
        }

        //
        // Default deltas, but needs to be overriden by the user.
        // 
        // Values provided here so that the caller sees something wrong
        // instead of a blank screen if they forget to set them.
        //

        m_rgDeltaUVDownRight[0] = 0.0f;
        m_rgDeltaUVDownRight[1] = 1.0f/1000.0f;
        m_rgDeltaUVDownRight[2] = 1.0f/1000.0f;
        m_rgDeltaUVDownRight[3] = 0.0f;

        m_rgOffsetUV[0] = 0.0f;
        m_rgOffsetUV[1] = 0.0f;
        m_rgOffsetUV[2] = 0.0f;
        m_rgOffsetUV[3] = 0.0f;

        //
        // Init shader constants to 0
        //

        for (unsigned i = 0; i < PIXELSHADER_CONSTANTS_MAX; i++)
        {
            m_rgShaderConstants[i][0] = 0.0f;
            m_rgShaderConstants[i][1] = 0.0f;
            m_rgShaderConstants[i][2] = 0.0f;
            m_rgShaderConstants[i][3] = 0.0f;
        }

    }

    //
    // Public state
    //

    CSamplerState m_samplers[PIXELSHADER_SAMPLERS_MAX];

    // 
    // Must be set by caller in the form:
    // 
    //    (0, 0, xStart, yStart)
    // 

    float m_rgOffsetUV[4];

    // 
    // Must be set by caller in the form:
    // 
    //    (duDown, dvDown, duRight, dvRight)
    // 
    // duDown, dvDown - Deltas for advancing u,v when we move right one pixel
    // duRight, dvRight - Deltas for advancing u,v when we move down one scaline
    //
    // For example, drawing with bounds (width, height) with an 
    // identity transform, specify:
    // 
    // m_rgDeltaUVDownRight = (0, 1/height, 1/width, 0)
    // 

    float m_rgDeltaUVDownRight[4];

    //
    // Shader constants
    //

    float m_rgShaderConstants[PIXELSHADER_CONSTANTS_MAX][4];

private:
    //
    // Private default texture
    //

    unsigned m_uWhite;
};

//
// GenerateColors function prototype
//

struct GenerateColorsEffectParams
{
    CPixelShaderState *pPixelShaderState;
    int nX;
    int nY;
    int nCount;
    unsigned *pPargbBuffer;
};

typedef void (__stdcall GenerateColorsEffect)(
    __in GenerateColorsEffectParams *pParams
    );




