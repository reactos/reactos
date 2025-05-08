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


//  alpha texture format: D3DFMT_A8***


// clear type, solid brush
technique CTSB
{
    pass P0
    {
        PixelShader =
        asm
        {
            ps_2_0

            // Constant registers:
            // c1.rg = ds,dt,0,0   // red sub pixel offset in alpha texture coordinates
            // c2.rgb = a,a,a      // brush alpha
            // c3.rgb = 1,1,1
            // c4.rgb = g1*f + g2 // f = brush color (rgb)
            // c5.rgb = g3*f + g4 // f = brush color (rgb)
            // where g1, g2, g3 and g4 are alpha correction ratios

            dcl_2d s0
            dcl t0

            // fetch glyph alpha for green channel
            texld   r1, t0, s0

            // fetch glyph alpha for blue channel
            add     r0, t0, c1  // r0 = sampling position for blue
            texld   r2, r0, s0

            // fetch glyph alpha for red channel
            sub     r0, t0, c1  // r0 = sampling position for red
            texld   r0, r0, s0

            // combine color component alphas to single register
            mov     r0.r, r0.a  // set red
            mov     r0.g, r1.a  // set green
            mov     r0.b, r2.a  // set blue

            mul     r0.rgb, r0, c2    // multiply glyph alphas by brush alpha
            mul     r0.a, r1.a, c2.r  // set final alpha to the green channel alpha to match greyscale behavior
                                      // multiply final alpha by brush alpha

            // Corrected alpha is calculated in r0.rgb as
            // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

            mul     r1.rgb, c4, r0  // r1 = (g1*f + g2)*a
            add     r1.rgb, r1, c5  // r1 = (g1*f + g2)*a + (g3*f + g4)

            sub     r2.rgb, c3, r0  // r2 = 1-a
            mul     r1.rgb, r1, r2  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))
            add     r1.rgb, r1, r1  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*2
            add     r1.rgb, r1, r1  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad     r0.rgb, r0, r1, r0   // r0 = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

            mov     oC0, r0
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
            ps_2_0

            // Constant registers:
            // c2.a = a           // brush alpha
            // c3.a = 1
            // c4.a = g1*f + g2 // f = brush color (rgb)
            // c5.a = g3*f + g4 // f = brush color (rgb)
            // where g1, g2, g3 and g4 are alpha correction ratios

            dcl_2d s0
            dcl t0

            // fetch glyph alpha
            texld   r0, t0, s0

            mul     r0.a, r0, c2  // multiply by alpha

            // Corrected alpha is calculated in r0.rgb as
            // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

            mul     r1.a, c4, r0  // r1 = (g1*f + g2)*a
            add     r1.a, r1, c5  // r1 = (g1*f + g2)*a + (g3*f + g4)

            sub     r2.a, c3, r0  // r2 = 1-a
            mul     r1.a, r1, r2  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))
            add     r1.a, r1, r1  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*2
            add     r1.a, r1, r1  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
            mad     r0, r0.a, r1.a, r0.a   // r0 = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

            mov     oC0, r0
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
            ps_2_0

            // Constant registers:
            // c0: color channel mask: (1,0,0,0) = red; (0,1,0,0) = green; (0,0,1,0) = blue
            // c1: (d4,d4,d4,d1) // gamma
            // c2: (d5,d5,d5,d3) //   correction
            // c3: (d6,d6,d6,d2) //     ratios
            // c4: ( 1, 1, 1, 1)
            // c5.a: effect alpha 

            dcl_2d s0
            dcl_2d s1
            dcl t0
            dcl t1

            texld r0, t0, s0      // fetch brush color (rgba, alpha-premultiplied)
            texld r1, t1, s1      // fetch the alpha (a)

            mul_sat r0, r0, c5.a    // combine brush color with effect alpha

            mul_sat r1, r0, r1.a    // combine alphas

            dp3_sat r1.rgb, r1, c0      // replicate desired channel to r1.rgb (r1.rgb = af, r1.a = a)

            // corrected alpha value is calculated in r0.a as:
            // new a = a + (1-a)*(d2*af + (d3*af + d1*a)*a)*4

            // corrected premultiplied foreground
            // is calculated in r0.rgb as following:
            // new af = af + af*(1-a)*(d4 + d5*a + d6*af)*4

            mul r0.a, c1, r1            // d1*a
            mul r0.rgb, c2, r1.a        // d5*a
            add r0.rgb, r0, c1          // d4 + d5*a
            mad r0.a, c2, r1.b, r0      // d3*af + d1*a
            mad r0.rgb, c3, r1, r0      // d4 + d5*a + d6*af
            mul r0.a, r1, r0            // d3*a*af + d1*a*a
            sub r3, c4, r1
            mul r0.rgb, r3.a, r0        // (1-a)*(d4 + d5*a + d6*af)*4
            mad r0.a, c3, r1.b, r0      // (d2*af + d3*a*af + d1*a*a)*4
            add r0, r0, r0
            add r0, r0, r0
            mad r0.rgb, r1, r0, r1      // af + af*(1-a)*(d4 + d5*a + d6*af)*4
            mad r0.a, r3, r0, r1        // a + (1-a)*(d2*af + d3*a*af + d1*a*a)*4

            mov oC0,r0
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
            ps_2_0

            // Constant registers:
            // c0: color channel weight mask: rgba = (.25,.5,.25,0)
            // c1: (d4,d4,d4,d1) // gamma
            // c2: (d5,d5,d5,d3) //   correction
            // c3: (d6,d6,d6,d2) //     ratios
            // c4: ( 1, 1, 1, 1)
            // c5.a: effect alpha 

            dcl_2d s0
            dcl_2d s1
            dcl t0
            dcl t1

            texld r2, t0, s0      // fetch brush color (rgba, alpha-premultiplied)
            texld r0, t1, s1      // fetch the alpha (a)
            
            mul_sat r2, r2, c5.a    // combine brush color with effect alpha

            mul_sat r1, r2, r0.a    // combine alphas

            dp3 r2.rgb, r1, c0      // calculate average (premultiplied) color in r2.b

            // corrected alpha value is calculated in r0.a as:
            // new a = a + (1-a)*(d2*af_a + (d3*af_a + d1*a)*a)*4

            // corrected premultiplied foreground
            // is calculated in r0.rgb as following:
            // new af = af + af*(1-a)*(d4 + d5*a + d6*af)*4

            mul r0.a, c1, r1            // d1*a
            mul r0.rgb, c2, r1.a        // d5*a
            add r0.rgb, r0, c1          // d4 + d5*a
            mad r0.a, c2, r2.b, r0      // d3*af_a + d1*a
            mad r0.rgb, c3, r1, r0      // d4 + d5*a + d6*af
            mul r0.a, r1, r0            // d3*a*af_a + d1*a*a
            sub r3,c4,r1
            mul r0.rgb, r3.a, r0        // (1-a)*(d4 + d5*a + d6*af)
            mad r0.a, c3, r2.b, r0      // (d2*af_a + d3*a*af_a + d1*a*a)
            add r0, r0, r0
            add r0, r0, r0
            mad r0.rgb, r1, r0, r1      // af + af*(1-a)*(d4 + d5*a + d6*af)*4
            mad r0.a, r3, r0, r1        // a + (1-a)*(d2*af + d3*a*af + d1*a*a)*4

            mov oC0,r0
        };
    }
}


