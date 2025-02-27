/*
 * libtxc_dxtn
 * Version:  1.0
 *
 * Copyright (C) 2004  Roland Scheidegger   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include "txc_dxtn.h"

#define EXP5TO8R(packedcol)					\
   ((((packedcol) >> 8) & 0xf8) | (((packedcol) >> 13) & 0x7))

#define EXP6TO8G(packedcol)					\
   ((((packedcol) >> 3) & 0xfc) | (((packedcol) >>  9) & 0x3))

#define EXP5TO8B(packedcol)					\
   ((((packedcol) << 3) & 0xf8) | (((packedcol) >>  2) & 0x7))

#define EXP4TO8(col)						\
   ((col) | ((col) << 4))

/* inefficient. To be efficient, it would be necessary to decode 16 pixels at once */

static void dxt135_decode_imageblock ( const GLubyte *img_block_src,
                         GLint i, GLint j, GLuint dxt_type, GLvoid *texel ) {
   GLchan *rgba = (GLchan *) texel;
   const GLushort color0 = img_block_src[0] | (img_block_src[1] << 8);
   const GLushort color1 = img_block_src[2] | (img_block_src[3] << 8);
   const GLuint bits = img_block_src[4] | (img_block_src[5] << 8) |
      (img_block_src[6] << 16) | (img_block_src[7] << 24);
   /* What about big/little endian? */
   GLubyte bit_pos = 2 * (j * 4 + i) ;
   GLubyte code = (GLubyte) ((bits >> bit_pos) & 3);

   rgba[ACOMP] = CHAN_MAX;
   switch (code) {
   case 0:
      rgba[RCOMP] = UBYTE_TO_CHAN( EXP5TO8R(color0) );
      rgba[GCOMP] = UBYTE_TO_CHAN( EXP6TO8G(color0) );
      rgba[BCOMP] = UBYTE_TO_CHAN( EXP5TO8B(color0) );
      break;
   case 1:
      rgba[RCOMP] = UBYTE_TO_CHAN( EXP5TO8R(color1) );
      rgba[GCOMP] = UBYTE_TO_CHAN( EXP6TO8G(color1) );
      rgba[BCOMP] = UBYTE_TO_CHAN( EXP5TO8B(color1) );
      break;
   case 2:
      if ((dxt_type > 1) || (color0 > color1)) {
         rgba[RCOMP] = UBYTE_TO_CHAN( ((EXP5TO8R(color0) * 2 + EXP5TO8R(color1)) / 3) );
         rgba[GCOMP] = UBYTE_TO_CHAN( ((EXP6TO8G(color0) * 2 + EXP6TO8G(color1)) / 3) );
         rgba[BCOMP] = UBYTE_TO_CHAN( ((EXP5TO8B(color0) * 2 + EXP5TO8B(color1)) / 3) );
      }
      else {
         rgba[RCOMP] = UBYTE_TO_CHAN( ((EXP5TO8R(color0) + EXP5TO8R(color1)) / 2) );
         rgba[GCOMP] = UBYTE_TO_CHAN( ((EXP6TO8G(color0) + EXP6TO8G(color1)) / 2) );
         rgba[BCOMP] = UBYTE_TO_CHAN( ((EXP5TO8B(color0) + EXP5TO8B(color1)) / 2) );
      }
      break;
   case 3:
      if ((dxt_type > 1) || (color0 > color1)) {
         rgba[RCOMP] = UBYTE_TO_CHAN( ((EXP5TO8R(color0) + EXP5TO8R(color1) * 2) / 3) );
         rgba[GCOMP] = UBYTE_TO_CHAN( ((EXP6TO8G(color0) + EXP6TO8G(color1) * 2) / 3) );
         rgba[BCOMP] = UBYTE_TO_CHAN( ((EXP5TO8B(color0) + EXP5TO8B(color1) * 2) / 3) );
      }
      else {
         rgba[RCOMP] = 0;
         rgba[GCOMP] = 0;
         rgba[BCOMP] = 0;
         if (dxt_type == 1) rgba[ACOMP] = UBYTE_TO_CHAN(0);
      }
      break;
   default:
   /* CANNOT happen (I hope) */
      break;
   }
}


void fetch_2d_texel_rgb_dxt1(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel)
{
   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 8);
   dxt135_decode_imageblock(blksrc, (i&3), (j&3), 0, texel);
}


void fetch_2d_texel_rgba_dxt1(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel)
{
   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 8);
   dxt135_decode_imageblock(blksrc, (i&3), (j&3), 1, texel);
}

void fetch_2d_texel_rgba_dxt3(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel) {

   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   GLchan *rgba = (GLchan *) texel;
   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 16);
#if 0
   /* Simple 32bit version. */
/* that's pretty brain-dead for a single pixel, isn't it? */
   const GLubyte bit_pos = 4 * ((j&3) * 4 + (i&3));
   const GLuint alpha_low = blksrc[0] | (blksrc[1] << 8) | (blksrc[2] << 16) | (blksrc[3] << 24);
   const GLuint alpha_high = blksrc[4] | (blksrc[5] << 8) | (blksrc[6] << 16) | (blksrc[7] << 24);

   dxt135_decode_imageblock(blksrc + 8, (i&3), (j&3), 2, texel);
   if (bit_pos < 32)
      rgba[ACOMP] = UBYTE_TO_CHAN( (GLubyte)(EXP4TO8((alpha_low >> bit_pos) & 15)) );
   else
      rgba[ACOMP] = UBYTE_TO_CHAN( (GLubyte)(EXP4TO8((alpha_high >> (bit_pos - 32)) & 15)) );
#endif
#if 1
/* TODO test this! */
   const GLubyte anibble = (blksrc[((j&3) * 4 + (i&3)) / 2] >> (4 * (i&1))) & 0xf;
   dxt135_decode_imageblock(blksrc + 8, (i&3), (j&3), 2, texel);
   rgba[ACOMP] = UBYTE_TO_CHAN( (GLubyte)(EXP4TO8(anibble)) );
#endif

}

void fetch_2d_texel_rgba_dxt5(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel) {

   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   GLchan *rgba = (GLchan *) texel;
   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 16);
   const GLubyte alpha0 = blksrc[0];
   const GLubyte alpha1 = blksrc[1];
#if 0
   const GLubyte bit_pos = 3 * ((j&3) * 4 + (i&3));
   /* simple 32bit version */
   const GLuint bits_low = blksrc[2] | (blksrc[3] << 8) | (blksrc[4] << 16) | (blksrc[5] << 24);
   const GLuint bits_high = blksrc[6] | (blksrc[7] << 8);
   GLubyte code;

   if (bit_pos < 30)
      code = (GLubyte) ((bits_low >> bit_pos) & 7);
   else if (bit_pos == 30)
      code = (GLubyte) ((bits_low >> 30) & 3) | ((bits_high << 2) & 4);
   else
      code = (GLubyte) ((bits_high >> (bit_pos - 32)) & 7);
#endif
#if 1
/* TODO test this! */
   const GLubyte bit_pos = ((j&3) * 4 + (i&3)) * 3;
   const GLubyte acodelow = blksrc[2 + bit_pos / 8];
   const GLubyte acodehigh = blksrc[3 + bit_pos / 8];
   const GLubyte code = (acodelow >> (bit_pos & 0x7) |
      (acodehigh  << (8 - (bit_pos & 0x7)))) & 0x7;
#endif
   dxt135_decode_imageblock(blksrc + 8, (i&3), (j&3), 2, texel);
#if 0
   if (alpha0 > alpha1) {
      switch (code) {
      case 0:
         rgba[ACOMP] = UBYTE_TO_CHAN( alpha0 );
         break;
      case 1:
         rgba[ACOMP] = UBYTE_TO_CHAN( alpha1 );
         break;
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
         rgba[ACOMP] = UBYTE_TO_CHAN( ((alpha0 * (8 - code) + (alpha1 * (code - 1))) / 7) );
         break;
      }
   }
   else {
      switch (code) {
      case 0:
         rgba[ACOMP] = UBYTE_TO_CHAN( alpha0 );
         break;
      case 1:
         rgba[ACOMP] = UBYTE_TO_CHAN( alpha1 );
         break;
      case 2:
      case 3:
      case 4:
      case 5:
         rgba[ACOMP] = UBYTE_TO_CHAN( ((alpha0 * (6 - code) + (alpha1 * (code - 1))) / 5) );
         break;
      case 6:
         rgba[ACOMP] = 0;
         break;
      case 7:
         rgba[ACOMP] = CHAN_MAX;
         break;
      }
   }
#endif
/* not sure. Which version is faster? */
#if 1
/* TODO test this */
   if (code == 0)
      rgba[ACOMP] = UBYTE_TO_CHAN( alpha0 );
   else if (code == 1)
      rgba[ACOMP] = UBYTE_TO_CHAN( alpha1 );
   else if (alpha0 > alpha1)
      rgba[ACOMP] = UBYTE_TO_CHAN( ((alpha0 * (8 - code) + (alpha1 * (code - 1))) / 7) );
   else if (code < 6)
      rgba[ACOMP] = UBYTE_TO_CHAN( ((alpha0 * (6 - code) + (alpha1 * (code - 1))) / 5) );
   else if (code == 6)
      rgba[ACOMP] = 0;
   else
      rgba[ACOMP] = CHAN_MAX;
#endif
}
