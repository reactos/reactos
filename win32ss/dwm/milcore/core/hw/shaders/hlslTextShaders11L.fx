// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Contains pixel shaders for text rendering.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//  alpha texture format:       D3DFMT_L8


// clear type, solid brush
technique CTSB
{
    pass P0
    {
        PixelShader =
        asm
        {
            ps.1.1

            // Constant registers:
            // c1 = a,0,0,0     // brush alpha * red mask
            // c2 = 0,a,0,0     // brush alpha * green mask
            // c3 = 0,0,a,0     // brush alpha * blue mask

            // c4.rgb = g1*f + g2 // f = brush color (rgb)
            // c5.rgb = g3*f + g4 // f = brush color (rgb)
            // where g1, g2, g3 and g4 are alpha correction ratios
            // c4.a = unused
            // c5.a = unused

            tex t0 // fetch glyph alpha for red channel
            tex t1 // fetch alpha alpha for green channel
            tex t2 // fetch alpha alpha for blue channel

            // Compose vector alpha in r0.rgb.
            // Note: this can be done in two lrp commands.
            // Unfortunatelly, some HW (ATI Radeon 7500 and others)
            // executes this command with very bad precision,
            // so following three commands are used instead.
            mul     r0, c1, t0          // set red
            mad     r0, c2, t1, r0      // set green
            mad_sat r0, c3, t2, r0      // set blue

            // Corrected alpha is calculated in r0.rgb as
            // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad    r1, c4, r0, c5   // r1 = (g1*f + g2)*a + (g3*f + g4)
            mul_x4 r1, r1, 1-r0     // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad    r0, r0, r1, r0   // r0 = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4
       };
    }
}

// grey scale, solid brush
technique GSSB
{
    pass P0
    {
        PixelShader =
        asm
        {
            ps.1.1

            // Constant registers:
            // c1 = a,0,0,0     // brush alpha * red mask
            // c2 = unused
            // c3 = unused

            // c4.a = g1*f + g2
            // c5.a = g3*f + g4
            // where:
            // f = average brush color luminance
            // g1, g2, g3, g4 = alpha correction ratios

            // c4.rgb = unused
            // c5.rgb = unused

            tex t0          // fetch glyphs alpha to t0.rgb
            dp3_sat r0, t0, c1  // calculate combined alpha in r0.a

            // Corrected alpha is calculated in r0.a as
            // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad    r1.a, c4, r0, c5   // r1.a = (g1*f + g2)*a + (g3*f + g4)
            mul_x4 r1.a, r1, 1-r0     // r1.a = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad    r0, r0.a, r1.a, r0.a   // r0.a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4
        };
    }
}

// clear type, textured brush
technique CTTB
{
    pass P0
    {
        PixelShader =
        asm
        {
            ps.1.1

            // Constant registers:
            // c0: color channel mask: (1,0,0,0) = red; (0,1,0,0) = green; (0,0,1,0) = blue
            // c1: (d4,d4,d4,d1) // gamma
            // c2: (d5,d5,d5,d3) //   correction
            // c3: (d6,d6,d6,d2) //     ratios
            // c5.a: effect alpha 

            tex t0      // fetch brush color (rgba, alpha-premultiplied)
            tex t1      // fetch the alpha (rgb)
            
            mul_sat t0, t0, c5.a    // combine brush color with effect alpha
            
            mul r1.rgb, t0, t1      // combine alphas in rgb (r1.rgb = af)
            +mul_sat r1.a, t0, t1.b     // combine alphas in alpha channel (r1.a = a)

            dp3_sat r1.rgb, r1, c0      // replicate desired channel to r1.rgb

            // corrected alpha value is calculated in r0.a as:
            // new a = a + (1-a)*(d2*af + (d3*af + d1*a)*a)*4

            // corrected premultiplied foreground
            // is calculated in r0.rgb as following:
            // new af = af + af*(1-a)*(d4 + d5*a + d6*af)*4

            +mul r0.a, c1, r1           // d1*a
            mad r0.rgb, c2, r1.a, c1    // d4 + d5*a
            +mad r0.a, c2, r1.b, r0     // d3*af + d1*a
            mad r0.rgb, c3, r1, r0      // d4 + d5*a + d6*af
            +mul r0.a, r1, r0           // d3*a*af + d1*a*a
            mul_x4 r0.rgb, 1-r1.a, r0   // (1-a)*(d4 + d5*a + d6*af)*4
            +mad_x4 r0.a, c3, r1.b, r0  // (d2*af + d3*a*af + d1*a*a)*4
            mad r0.rgb, r1, r0, r1      // af + af*(1-a)*(d4 + d5*a + d6*af)*4
            +mad r0.a, 1-r1, r0, r1     // a + (1-a)*(d2*af + d3*a*af + d1*a*a)*4
        };
    }
}

// grey scale, textured brush
technique GSTB
{
    pass P0
    {
        PixelShader =
        asm
        {
            ps.1.1

            // Constant registers:
            // c0: color channel weight mask: rgba = (.25,.5,.25,0)
            // c1: (d4,d4,d4,d1) // gamma
            // c2: (d5,d5,d5,d3) //   correction
            // c3: (d6,d6,d6,d2) //     ratios
            // c5.a: effect alpha 

            tex t0      // fetch brush color (rgba, alpha-premultiplied)
            tex t1      // fetch the alpha (rgb)
            
            mul_sat t0, t0, c5.a    // combine brush color with effect alpha
            
            mul_sat r1.rgb, t0, t1      // combine alphas in rgb (r1.rgb = af)
            +mul_sat r1.a, t0, t1.b     // combine alphas in alpha channel (r1.a = a)

            dp3 t0.rgb, r1, c0      // calculate average (premultiplied) color in t0.b

            // corrected alpha value is calculated in r0.a as:
            // new a = a + (1-a)*(d2*af_a + (d3*af_a + d1*a)*a)*4

            // corrected premultiplied foreground
            // is calculated in r0.rgb as following:
            // new af = af + af*(1-a)*(d4 + d5*a + d6*af)*4

            +mul r0.a, c1, r1           // d1*a
            mad r0.rgb, c2, r1.a, c1    // d4 + d5*a
            +mad r0.a, c2, t0.b, r0     // d3*af_a + d1*a
            mad r0.rgb, c3, r1, r0      // d4 + d5*a + d6*af
            +mul r0.a, r1, r0           // d3*a*af_a + d1*a*a
            mul_x4 r0.rgb, 1-r1.a, r0   // (1-a)*(d4 + d5*a + d6*af)*4
            +mad_x4 r0.a, c3, t0.b, r0  // (d2*af_a + d3*a*af_a + d1*a*a)*4
            mad r0.rgb, r1, r0, r1      // af + af*(1-a)*(d4 + d5*a + d6*af)*4
            +mad r0.a, 1-r1, r0, r1     // a + (1-a)*(d2*af + d3*a*af + d1*a*a)*4
        };
    }
}


