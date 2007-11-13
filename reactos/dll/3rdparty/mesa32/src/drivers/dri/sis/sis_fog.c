/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_fog.c,v 1.3 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_state.h"
#include "swrast/swrast.h"

#include "macros.h"

static GLint convertFtToFogFt( GLfloat dwInValue );
static GLint doFPtoFixedNoRound( GLfloat dwInValue, int nFraction );

void
sisDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   float fArg;
   GLint fogColor;

   switch (pname)
   {
   case GL_FOG_COORDINATE_SOURCE_EXT:
      current->hwFog &= ~MASK_FogMode;
      switch (ctx->Fog.FogCoordinateSource)
      {
      case GL_FOG_COORDINATE_EXT:
         current->hwFog &= ~MASK_FogZLookup;
         break;
      case GL_FRAGMENT_DEPTH_EXT:
         current->hwFog |= MASK_FogZLookup;
         break;
      }
      if (current->hwFog != prev->hwFog) {
         prev->hwFog = current->hwFog;
         smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   case GL_FOG_MODE:
      current->hwFog &= ~MASK_FogMode;
      switch (ctx->Fog.Mode)
      {
      case GL_LINEAR:
         current->hwFog |= FOGMODE_LINEAR;
         break;
      case GL_EXP:
         current->hwFog |= FOGMODE_EXP;
         break;
      case GL_EXP2:
         current->hwFog |= FOGMODE_EXP2;
         break;
      }
      if (current->hwFog != prev->hwFog) {
         prev->hwFog = current->hwFog;
         smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   case GL_FOG_DENSITY:
      current->hwFogDensity = convertFtToFogFt( ctx->Fog.Density );
      if (current->hwFogDensity != prev->hwFogDensity) {
         prev->hwFogDensity = current->hwFogDensity;
         smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   case GL_FOG_START:
   case GL_FOG_END:
      fArg = 1.0 / (ctx->Fog.End - ctx->Fog.Start);
      current->hwFogInverse = doFPtoFixedNoRound( fArg, 10 );
      if (pname == GL_FOG_END)
      {
         if (smesa->Chipset == PCI_CHIP_SIS300)
            current->hwFogFar = doFPtoFixedNoRound( ctx->Fog.End, 10 );
         else
            current->hwFogFar = doFPtoFixedNoRound( ctx->Fog.End, 6 );
      }
      if (current->hwFogFar != prev->hwFogFar ||
          current->hwFogInverse != prev->hwFogInverse)
      {
         prev->hwFogFar = current->hwFogFar;
         prev->hwFogInverse = current->hwFogInverse;
         smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   case GL_FOG_INDEX:
      /* TODO */
      break;
   case GL_FOG_COLOR:
      fogColor  = FLOAT_TO_UBYTE( ctx->Fog.Color[0] ) << 16;
      fogColor |= FLOAT_TO_UBYTE( ctx->Fog.Color[1] ) << 8;
      fogColor |= FLOAT_TO_UBYTE( ctx->Fog.Color[2] );
      current->hwFog &= 0xff000000;
      current->hwFog |= fogColor;
      if (current->hwFog != prev->hwFog) {
          prev->hwFog = current->hwFog;
         smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   }
}

static GLint
doFPtoFixedNoRound( GLfloat dwInValue, int nFraction )
{
   GLint dwMantissa;
   int nTemp;
   union { int i; float f; } u;
   GLint val;

   u.f = dwInValue;
   val = u.i;

   if (val == 0)
      return 0;
   nTemp = (int) (val & 0x7F800000) >> 23;
   nTemp = nTemp - 127 + nFraction - 23;
   dwMantissa = (val & 0x007FFFFF) | 0x00800000;

   if (nTemp < -25)
       return 0;
   if (nTemp > 0)
      dwMantissa <<= nTemp;
   else {
      nTemp = -nTemp;
      dwMantissa >>= nTemp;
   }
   if (val & 0x80000000)
      dwMantissa = ~dwMantissa + 1;
   return dwMantissa;
}

/* s[8].23->s[7].10 */
static GLint
convertFtToFogFt( GLfloat dwInValue )
{
   GLint dwMantissa, dwExp;
   GLint dwRet;
   union { int i; float f; } u;
   GLint val;

   u.f = dwInValue;
   val = u.i;

   if (val == 0)
      return 0;

   /* ----- Standard float Format: s[8].23                          -----
    * -----     = (-1)^S * 2^(E      - 127) * (1 + M        / 2^23) -----
    * -----     = (-1)^S * 2^((E-63) -  64) * (1 + (M/2^13) / 2^10) -----
    * ----- Density float Format:  s[7].10                          -----
    * -----     New Exponential = E - 63                            -----
    * -----     New Mantissa    = M / 2^13                          -----
    * -----                                                         -----
    */

   dwExp = (val & 0x7F800000) >> 23;
   dwExp -= 63;

   if (dwExp < 0)
      return 0;

   if (dwExp <= 0x7F)
      dwMantissa = (val & 0x007FFFFF) >> (23 - 10);
   else {
      /* ----- To Return +Max(or -Max) ----- */
      dwExp = 0x7F;
      dwMantissa = 0x3FF;
   }

   dwRet = (val & 0x80000000) >> (31 - 17);  /* Shift Sign Bit */

   dwRet |= (dwExp << 10) | dwMantissa;

   return dwRet;
}
