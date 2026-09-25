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

#include "precomp.hpp"


const GammaRatios
CGammaHandler::sc_gammaRatios[MAX_GAMMA_INDEX+1] =
{
    // note: ratios are divided by 4, in order to avoid overflow in pixel shaders
    { 0.0000f/4.f,  0.0000f/4.f,  0.0000f/4.f,  0.0000f/4.f,     0.0000f,  0.0000f,  0.0000f,  0.0000f,  0.0000f,  0.0000f}, // gamma = 1.0
    { 0.0166f/4.f, -0.0807f/4.f,  0.2227f/4.f, -0.0751f/4.f,    -0.0465f,  0.0296f,  0.0487f,  0.0238f, -0.0610f,  0.0625f}, // gamma = 1.1
    { 0.0350f/4.f, -0.1760f/4.f,  0.4325f/4.f, -0.1370f/4.f,    -0.0913f,  0.0603f,  0.0906f,  0.0482f, -0.1225f,  0.1218f}, // gamma = 1.2
    { 0.0543f/4.f, -0.2821f/4.f,  0.6302f/4.f, -0.1876f/4.f,    -0.1343f,  0.0917f,  0.1266f,  0.0730f, -0.1837f,  0.1779f}, // gamma = 1.3
    { 0.0739f/4.f, -0.3963f/4.f,  0.8167f/4.f, -0.2287f/4.f,    -0.1755f,  0.1233f,  0.1573f,  0.0980f, -0.2443f,  0.2309f}, // gamma = 1.4
    { 0.0933f/4.f, -0.5161f/4.f,  0.9926f/4.f, -0.2616f/4.f,    -0.2149f,  0.1551f,  0.1834f,  0.1229f, -0.3040f,  0.2809f}, // gamma = 1.5
    { 0.1121f/4.f, -0.6395f/4.f,  1.1588f/4.f, -0.2877f/4.f,    -0.2526f,  0.1867f,  0.2053f,  0.1477f, -0.3625f,  0.3279f}, // gamma = 1.6
    { 0.1300f/4.f, -0.7649f/4.f,  1.3159f/4.f, -0.3080f/4.f,    -0.2886f,  0.2180f,  0.2237f,  0.1722f, -0.4196f,  0.3722f}, // gamma = 1.7
    { 0.1469f/4.f, -0.8911f/4.f,  1.4644f/4.f, -0.3234f/4.f,    -0.3229f,  0.2489f,  0.2389f,  0.1964f, -0.4752f,  0.4138f}, // gamma = 1.8
    { 0.1627f/4.f, -1.0170f/4.f,  1.6051f/4.f, -0.3347f/4.f,    -0.3557f,  0.2793f,  0.2513f,  0.2201f, -0.5292f,  0.4530f}, // gamma = 1.9
    { 0.1773f/4.f, -1.1420f/4.f,  1.7385f/4.f, -0.3426f/4.f,    -0.3870f,  0.3091f,  0.2613f,  0.2434f, -0.5815f,  0.4897f}, // gamma = 2.0
    { 0.1908f/4.f, -1.2652f/4.f,  1.8650f/4.f, -0.3476f/4.f,    -0.4168f,  0.3382f,  0.2692f,  0.2661f, -0.6322f,  0.5243f}, // gamma = 2.1
    { 0.2031f/4.f, -1.3864f/4.f,  1.9851f/4.f, -0.3501f/4.f,    -0.4452f,  0.3667f,  0.2751f,  0.2883f, -0.6812f,  0.5567f}, // gamma = 2.2
};

// unique static instance of CGammaHandler
CGammaHandler g_GammaHandler;

//+------------------------------------------------------------------------
//
//  Function:  CGammaHandler::CGammaHandler
//
//  Synopsis:  Construct hard coded gamma table
//
//-------------------------------------------------------------------------
CGammaHandler::CGammaHandler()
{
    CalculateGammaTable(&m_hardCodedGammaTable, HardCodedGammaIndex);
}


//+------------------------------------------------------------------------
//
//  Function:   CGammaHandler::CalculateGammaTable
//
//  Synopsis:   Calculate alpha correction table for software rendering.
//
//              Alpha correction for (non-premultiplied) foreground color:
//              for given
//              a = composed alpha (i.e. <brush alpha>*<glyph alpha>)
//              and
//              f = foreground color
//              we are calculating corrected alpha value as following:
//              new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4)).
//
//              To optimize, the formula above is converted to the following form:
//              new a = a + a*(1-a)*((       g2)*a + (       g4))
//                    +     a*(1-a)*((g1*f     )*a + (g3*f     ));
//  
//              or
//              new a = f1(a) + f*f2(a);
//              where
//              f1(a) = a + a*(1-a)*(g2*a + g4);
//              f2(a) = a*(1-a)*(g1*a + g3);
//
//              The functions f1(a) and f2(a)are represented in a table form.
//-------------------------------------------------------------------------
void
CGammaHandler::CalculateGammaTable(
    __out_ecount(1) GammaTable * pTable,
    __range(1, MAX_GAMMA_INDEX) UINT uiGammaIndex)
{
    // ensure right FPU rounding mode
    CFloatFPU oGuard;

    const GammaRatios *pRatios = &sc_gammaRatios[uiGammaIndex];

    static const float norm13 = (float)(double(0x10000)/(255*255)*4);
    static const float norm24 = (float)(double(0x100  )/(255    )*4);
    float g1 = norm13*pRatios->g1;
    float g2 = norm24*pRatios->g2;
    float g3 = norm13*pRatios->g3;
    float g4 = norm24*pRatios->g4;

    for (int i = 0; i < 256; i++)
    {
        float a = i*float(1./255);

        float f1 = a + a*(1-a)*(g2*a + g4);
        float f2 = a*(1-a)*(g1*a + g3);

        __int32 if1 = CFloatFPU::SmallRound(f1*0xFF);
        __int32 if2 = CFloatFPU::SmallRound(f2*0xFF);

        // Can't use Assert() because this code works at dll startup time
#if DBG
        static bool fAssertionReported = false;
        if (!fAssertionReported && (if1 < 0 || if1 > 255  || if2 < 0 || if2 > 255))
        {
            MessageBox(NULL, _T("CSWGlyphRunPainter::CalculateGammaTable2()"), _T("Assertion"), MB_OK);
            fAssertionReported = true;
        }
#endif //DBG
        pTable->Polynom[i].f1 = (BYTE)if1;
        pTable->Polynom[i].f2 = (BYTE)if2;
    }
}


