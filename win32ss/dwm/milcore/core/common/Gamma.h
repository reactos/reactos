// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Common data and routines to handle nonlinear
//      dependency of pixel light energy on video memory value
//
//  Class:      CGammaHandler
//
//  Structs:    GammaTable
//              GammaRatios
//
//------------------------------------------------------------------------

#pragma once


#define MAX_GAMMA_INDEX 12  // Maximum gamma index that can be passed into 
                            // CalculateGammaTable.

//+------------------------------------------------------------------------
//
// Struct:  GammaTable
//
// Synopsis:
//      Tables to convert numerical value in video memory
//      to corresponding level of light source energy, and back
//
//-------------------------------------------------------------------------
struct GammaTable
{
    struct Row
    {
        BYTE f1;
        BYTE f2;
    } Polynom[256];
};

//+------------------------------------------------------------------------
//
// Struct:  GammaRatios
//
// Synopsis:    Ratios of the polynomials for linear-cubic alpha correction.
//      See windows\wcp\Mil\core\hw\pixelshaders for details.
//
//-------------------------------------------------------------------------
struct GammaRatios
{
    float g1,g2,g3,g4;
    float d1,d2,d3,d4,d5,d6;
};

//+------------------------------------------------------------------------
//
// Class:       CGammaHandler
//
// Synopsis:
//      Singleton class to provide gamma calculations and data
//
//-------------------------------------------------------------------------
class CGammaHandler
{
public:

    // 1-bit flag value to keep together with gamma index.
    // Should be greater than MAX_GAMMA_INDEX.
    static const UINT GammaFlag = 0x10;

    //static const UINT HardCodedGammaLevel = 1900;
    //static const UINT HardCodedGammaIndex = 9;
    static const UINT HardCodedGammaLevel = 2200;
    static const UINT HardCodedGammaIndex = 12;

    static const GammaRatios sc_gammaRatios[MAX_GAMMA_INDEX+1];

public:

    CGammaHandler::CGammaHandler();
    static void CalculateGammaTable(
        __out_ecount(1) GammaTable * pTable,
        __range(1, MAX_GAMMA_INDEX) UINT uiGammaIndex);

    GammaTable m_hardCodedGammaTable;
};

// unique static instance of CGammaHandler
extern CGammaHandler g_GammaHandler;

